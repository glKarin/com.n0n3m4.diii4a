/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"

#include "renderer/tr_local.h"
#include "renderer/backend/stages/DepthStage.h"
#include "renderer/backend/RenderBackend.h"
#include "renderer/backend/FrameBuffer.h"
#include "renderer/backend/FrameBufferManager.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/GLSLUniforms.h"


struct DepthStage::DepthUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( DepthUniforms )

	DEFINE_UNIFORM( mat4, modelViewMatrix )
	DEFINE_UNIFORM( mat4, projectionMatrix )
	DEFINE_UNIFORM( mat4, textureMatrix )
	DEFINE_UNIFORM( vec4, clipPlane )
	DEFINE_UNIFORM( mat4, modelMatrix )
	DEFINE_UNIFORM( sampler, texture )
	DEFINE_UNIFORM( float, alphaTest )
	DEFINE_UNIFORM( vec4, color )
};

void DepthStage::LoadShader( GLSLProgram *shader ) {
	shader->LoadFromFiles( "stages/depth/depth.vert.glsl", "stages/depth/depth.frag.glsl" );
	DepthUniforms *uniforms = shader->GetUniformGroup<DepthUniforms>();
	uniforms->texture.Set( 0 );
}

void DepthStage::CalcScissorParam( uint32_t scissor[4], const idScreenRect &screenRect ) {
	// get [L..R) ranges
	int x1 = backEnd.viewDef->viewport.x1 + screenRect.x1;
	int y1 = backEnd.viewDef->viewport.y1 + screenRect.y1;
	int x2 = backEnd.viewDef->viewport.x1 + screenRect.x2 + 1;
	int y2 = backEnd.viewDef->viewport.y1 + screenRect.y2 + 1;
	// convert to FBO resolution with conservative outwards rounding
	int width = frameBuffers->activeFbo->Width();
	int height = frameBuffers->activeFbo->Height();
	x1 = x1 * width  / glConfig.vidWidth;
	y1 = y1 * height / glConfig.vidHeight;
	x2 = (x2 * width  + glConfig.vidWidth  - 1) / glConfig.vidWidth;
	y2 = (y2 * height + glConfig.vidHeight - 1) / glConfig.vidHeight;
	// get width/height and apply scissor
	scissor[0] = x1;
	scissor[1] = y1;
	scissor[2] = x2 - x1;
	scissor[3] = y2 - y1;
}

void DepthStage::Init() {
	depthShader = programManager->LoadFromGenerator( "depth", [=](GLSLProgram *program) { LoadShader(program); } );
}

void DepthStage::Shutdown() {}

void DepthStage::DrawDepth( const viewDef_t *viewDef, drawSurf_t **drawSurfs, int numDrawSurfs ) {
	if ( numDrawSurfs == 0 ) {
		return;
	}

	TRACE_GL_SCOPE( "DepthStage" );

	GLSLProgram *shader = depthShader;
	shader->Activate();
	uniforms = shader->GetUniformGroup<DepthUniforms>();

	uniforms->projectionMatrix.Set( viewDef->projectionMatrix );

	// pass mirror clip plane details to vertex shader if needed
	if ( viewDef->numClipPlanes ) {
		uniforms->clipPlane.Set( *viewDef->clipPlane );
	} else {
		uniforms->clipPlane.Set( colorBlack );
	}

	// the first texture will be used for alpha tested surfaces
	GL_SelectTexture( 0 );
	uniforms->texture.Set( 0 );

	GL_State( GLS_DEPTHFUNC_LESS );

	// Enable stencil test if we are going to be using it for shadows.
	// If we didn't do this, it would be legal behavior to get z fighting
	// from the ambient pass and the light passes.
	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	for ( int i = 0; i < numDrawSurfs; ++i ) {
		const drawSurf_t *drawSurf = drawSurfs[i];
		if ( !ShouldDrawSurf( drawSurf ) ) {
			continue;
		}

		DrawSurf( drawSurf );
	}

	// Make the early depth pass available to shaders. #3877
	if ( !viewDef->IsLightGem() && !r_skipDepthCapture.GetBool() ) {
		if ( ( !viewDef->isSubview || viewDef->isXray || viewDef->isMirror ) && !viewDef->renderWorld->mapName.IsEmpty() ) // compass
			frameBuffers->UpdateCurrentDepthCopy();
	}
}

bool DepthStage::ShouldDrawSurf(const drawSurf_t *surf) const {
    const idMaterial *shader = surf->material;

    if ( !shader->IsDrawn() ) {
        return false;
    }

    // some deforms may disable themselves by setting numIndexes = 0
    if ( !surf->numIndexes ) {
        return false;
    }

    // translucent surfaces don't put anything in the depth buffer and don't
    // test against it, which makes them fail the mirror clip plane operation
    if ( shader->Coverage() == MC_TRANSLUCENT ) {
        return false;
    }

    if ( !surf->ambientCache.IsValid() || !surf->indexCache.IsValid() ) {
#ifdef _DEBUG
        common->Printf( "DepthStage: missing vertex or index cache\n" );
#endif
        return false;
    }

    if ( surf->material->GetSort() == SS_PORTAL_SKY && g_enablePortalSky.GetInteger() == 2 ) {
        return false;
    }

    // if all stages of a material have been conditioned off, don't do anything
    int stage;
    for ( stage = 0; stage < shader->GetNumStages() ; stage++ ) {
        const shaderStage_t *pStage = shader->GetStage( stage );
        // check the stage enable condition
        if ( surf->IsStageEnabled( pStage ) ) {
            break;
        }
    }
    return stage != shader->GetNumStages();
}

void DepthStage::DrawSurf( const drawSurf_t *surf ) {
	ApplyDepthTweaks depthTweaks( surf );

	CreateDrawCommands( surf );
}

void DepthStage::CreateDrawCommands( const drawSurf_t *surf ) {
	const idMaterial		*shader = surf->material;

	vertexCache.VertexPosition( surf->ambientCache );

	bool isSubView = surf->material->GetSort() == SS_SUBVIEW;
	if( isSubView ) {
		GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS );
	} else {
		GL_State( GLS_DEPTHFUNC_LESS );
	}

	bool drawSolid = false;

	if ( shader->Coverage() == MC_OPAQUE ) {
		drawSolid = true;
	}

	// we may have multiple alpha tested stages
	if ( shader->Coverage() == MC_PERFORATED ) {
		// if the only alpha tested stages are condition register omitted,
		// draw a normal opaque surface
		bool	didDraw = false;

		GL_CheckErrors();

		// perforated surfaces may have multiple alpha tested stages
		for ( int stage = 0; stage < shader->GetNumStages(); stage++ ) {
			const shaderStage_t *pStage = shader->GetStage( stage );

			if ( !pStage->hasAlphaTest ) {
				continue;
			}

			// check the stage enable condition
			if ( !surf->IsStageEnabled( pStage ) ) {
				continue;
			}

			// if we at least tried to draw an alpha tested stage,
			// we won't draw the opaque surface
			didDraw = true;

			// skip the entire stage if alpha would be black
			if ( surf->GetStageColor( pStage )[3] <= 0 ) {
				continue;
			}
			IssueDrawCommand( surf, pStage );
		}

		if ( !didDraw ) {
			drawSolid = true;
		}
	}

	if ( drawSolid ) {  // draw the entire surface solid
		IssueDrawCommand( surf, nullptr );
	}
}

void DepthStage::IssueDrawCommand( const drawSurf_t *surf, const shaderStage_t *stage ) {
	if( stage ) {
		stage->texture.image->Bind();
	}

	uniforms->modelViewMatrix.Set( surf->space->modelViewMatrix );
	uniforms->modelMatrix.Set( surf->space->modelMatrix );

	// TODO #6349: Fix FB_ApplyScissor and use it here
	uint32_t scissor[4];
	CalcScissorParam( scissor, surf->scissorRect );
	if (r_useScissor.GetBool()) {
		qglScissor( scissor[0], scissor[1], scissor[2], scissor[3] );
	}

	idVec4 color;
	if ( surf->material->GetSort() == SS_SUBVIEW ) {
		// subviews will just down-modulate the color buffer by overbright
		color[0] = color[1] = color[2] = 1.0f / backEnd.overBright;
		color[3] = 1;
	} else {
		// others just draw black
		color = idVec4( 0, 0, 0, 1 );
	}

	float alphaTest = -1.0f;
	if( stage ) {
		// set the alpha modulate
		color[3] = surf->shaderRegisters[stage->color.registers[3]];
		alphaTest = surf->shaderRegisters[stage->alphaTestRegister];
		uniforms->textureMatrix.Set( surf->GetTextureMatrix( stage ) );
	}
	uniforms->color.Set( color );
	uniforms->alphaTest.Set( alphaTest );

	RB_DrawElementsWithCounters( surf, DCK_DEPTH_PREPASS );
}
