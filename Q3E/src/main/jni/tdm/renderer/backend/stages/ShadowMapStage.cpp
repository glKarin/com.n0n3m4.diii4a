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
#include "renderer/backend/stages/ShadowMapStage.h"
#include "renderer/backend/RenderBackend.h"
#include "renderer/backend/FrameBuffer.h"
#include "renderer/backend/FrameBufferManager.h"
#include "renderer/backend/GLSLUniforms.h"
#include "renderer/backend/GLSLProgramManager.h"

idCVarBool r_shadowMapCullFront(
	"r_shadowMapCullFront", "0", CVAR_ARCHIVE | CVAR_RENDERER,
	"Cull front faces in shadow maps"
);
idCVar r_shadowMapSinglePass(
	"r_shadowMapSinglePass", "1", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_BOOL,
	"1 - render shadow maps for all lights in a single pass"
);
idCVar r_shadowMapAlphaTested(
	"r_shadowMapAlphaTested", "1", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE,
	"In case of alpha-tested material, apply alpha test to shadows too?\n"
	"Note: stencil shadows cannot work with alpha test."
);


struct ShadowMapStage::Uniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( Uniforms )
	DEFINE_UNIFORM( mat4, modelMatrix )
	DEFINE_UNIFORM( vec4, lightOrigin )
	DEFINE_UNIFORM( float, maxLightDistance )
	DEFINE_UNIFORM( float, alphaTest )
	DEFINE_UNIFORM( sampler, opaqueTexture )
	DEFINE_UNIFORM( mat4, textureMatrix )
};

ShadowMapStage::ShadowMapStage() {}

void ShadowMapStage::Init() {
	shadowMapShader = programManager->LoadFromFiles( "shadow_map", "stages/shadow_map/shadow_map.vert.glsl", "stages/shadow_map/shadow_map.frag.glsl" );
}

void ShadowMapStage::Shutdown() {}

void ShadowMapStage::Start() {
	GL_CheckErrors();
	frameBuffers->EnterShadowMap();

	GL_Cull( r_shadowMapCullFront ? CT_BACK_SIDED : CT_TWO_SIDED );
	qglPolygonOffset( 0, 0 );
	qglEnable( GL_POLYGON_OFFSET_FILL );
	for ( int i = 0; i < 5; i++ ) {
		qglEnable( GL_CLIP_DISTANCE0 + i );
	}

	shadowMapShader->Activate();
	uniforms = shadowMapShader->GetUniformGroup<Uniforms>();
	uniforms->opaqueTexture.Set( 0 );
}

bool ShadowMapStage::ShouldDrawLight( const viewLight_t *vLight, const DrawMask &mask ) {
	if ( vLight->noShadows ) {
		return false;
	}
	if ( vLight->shadows != LS_MAPS) {
		return false;
	}
	if ( vLight->shadowMapPage.width == 0 ) {
		return false;
	}
	if ( !( mask.clear || mask.global && vLight->globalShadows || mask.local && vLight->localShadows ) ) {
		return false;
	}
	return true;
}

void ShadowMapStage::DrawLight( const viewLight_t *vLight, const DrawMask &mask ) {
	idVec4 lightOrigin;
	lightOrigin.x = vLight->globalLightOrigin.x;
	lightOrigin.y = vLight->globalLightOrigin.y;
	lightOrigin.z = vLight->globalLightOrigin.z;
	lightOrigin.w = 0;
	uniforms->lightOrigin.Set( lightOrigin );
	uniforms->maxLightDistance.Set( vLight->maxLightDistance );

	const renderCrop_t &page = vLight->shadowMapPage;
	qglViewport( page.x, page.y, 6*page.width, page.width );
	// note: scissor must be always applied for correctness, also it is not view-dependent
	qglScissor( page.x, page.y, 6*page.width, page.width );

	if ( mask.clear )
		qglClear( GL_DEPTH_BUFFER_BIT );

	if ( mask.global )
		DrawLightInteractions( vLight->globalShadows );

	if ( mask.local )
		DrawLightInteractions( vLight->localShadows );
}

void ShadowMapStage::End() {
	for ( int i = 0; i < 5; i++ ) {
		qglDisable( GL_CLIP_DISTANCE0 + i );
	}

	qglDisable( GL_POLYGON_OFFSET_FILL );
	GL_Cull( CT_FRONT_SIDED );

	frameBuffers->LeaveShadowMap();
}

bool ShadowMapStage::DrawShadowMapSingleLight( const viewDef_t *viewDef, const viewLight_t *vLight, const DrawMask &mask ) {
	if ( !ShouldDrawLight( vLight, mask ) )
		return false;

	TRACE_GL_SCOPE( "DrawShadowMapSingleLight" );

	Start();
	DrawLight( vLight, mask );
	End();

	return true;
}

void ShadowMapStage::DrawShadowMapAllLights( const viewDef_t *viewDef, const DrawMask &mask ) {
	TRACE_GL_SCOPE( "DrawShadowMapAllLights" );

	Start();
	for ( viewLight_t *vLight = viewDef->viewLights; vLight; vLight = vLight->next ) {
		if ( !ShouldDrawLight( vLight, mask ) )
			continue;
		DrawLight( vLight, mask );
	}
	End();
}

bool ShadowMapStage::ShouldDrawSurf( const drawSurf_t *surf ) const {
    const idMaterial *shader = surf->material;

    // some deforms may disable themselves by setting numIndexes = 0
    if ( !surf->numIndexes ) {
        return false;
    }

    if ( !surf->ambientCache.IsValid() || !surf->indexCache.IsValid() ) {
        #ifdef _DEBUG
        common->Printf( "ShadowMapStage: missing vertex or index cache\n" );
        #endif
        return false;
    }

	// #6571. parallax surfaces with this flag don't work well if the surface is rendered to shadow maps
	if ( auto parallaxStage = surf->material->GetParallaxStage() ) {
		if ( parallaxStage->parallax->offsetExternalShadows )
			return false;
	}

	return true;
}

void ShadowMapStage::DrawLightInteractions( const drawSurf_t *surfs ) {
	for ( const drawSurf_t *surf = surfs; surf; surf = surf->nextOnLight ) {
		if ( !ShouldDrawSurf( surf ) ) {
			continue;
		}
		DrawSurf( surf );
	}
}

void ShadowMapStage::DrawSurf( const drawSurf_t *surf ) {
	const idMaterial *shader = surf->material;

	vertexCache.VertexPosition( surf->ambientCache );

	bool drawSolid = false;

	if ( shader->Coverage() == MC_OPAQUE || shader->Coverage() == MC_TRANSLUCENT ) {
		drawSolid = true;
	}

	// we may have multiple alpha tested stages
	if ( shader->Coverage() == MC_PERFORATED ) {

		if ( r_shadowMapAlphaTested.GetBool() ) {
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
		else {
			drawSolid = true;
		}
	}

	if ( drawSolid ) {  // draw the entire surface solid
		IssueDrawCommand( surf, nullptr );
	}
}

void ShadowMapStage::IssueDrawCommand( const drawSurf_t *surf, const shaderStage_t *stage ) {
	if( stage ) {
		GL_SelectTexture( 0 );
		stage->texture.image->Bind();
	}

	uniforms->modelMatrix.Set( surf->space->modelMatrix );

	if( stage ) {
		// set the alpha modulate
		uniforms->alphaTest.Set( surf->shaderRegisters[stage->alphaTestRegister] );
		uniforms->textureMatrix.Set( surf->GetTextureMatrix( stage ) );
	}
	else {
		uniforms->alphaTest.Set( -1.0f );
	}

	RB_DrawElementsInstanced( surf, 6, DCK_SHADOW );
}
