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

#include "renderer/backend/stages/SurfacePassesStage.h"

#include "renderer/backend/RenderBackend.h"
#include "renderer/backend/GLSLUniforms.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/FrameBuffer.h"
#include "renderer/backend/FrameBufferManager.h"
#ifdef __ANDROID__ //karin: using glslprops/ for avoid override
#include "glslprogs/stages/surface_passes/texgen_shared.glsl"
#else
#include "glprogs/stages/surface_passes/texgen_shared.glsl"
#endif

struct SimpleTextureUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( SimpleTextureUniforms )

	DEFINE_UNIFORM( mat4, modelViewMatrix )
	DEFINE_UNIFORM( mat4, projectionMatrix )
	DEFINE_UNIFORM( vec4, colorMul )
	DEFINE_UNIFORM( vec4, colorAdd )
	DEFINE_UNIFORM( int, texgen )
	DEFINE_UNIFORM( mat4, textureMatrix )
	DEFINE_UNIFORM( vec3, viewOrigin )
	DEFINE_UNIFORM( sampler, texture )
	DEFINE_UNIFORM( sampler, cubemap )
};

struct EnvironmentUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( EnvironmentUniforms )

	DEFINE_UNIFORM( mat4, modelViewMatrix )
	DEFINE_UNIFORM( mat4, projectionMatrix )
	DEFINE_UNIFORM( vec3, globalViewOrigin )
	DEFINE_UNIFORM( mat4, modelMatrix )
	DEFINE_UNIFORM( sampler, environmentMap )
	DEFINE_UNIFORM( sampler, normalMap )
	DEFINE_UNIFORM( int, RGTC )
	DEFINE_UNIFORM( vec4, constant )
	DEFINE_UNIFORM( vec4, fresnel )
	DEFINE_UNIFORM( int, tonemapOutputColor )
};

struct SoftParticleUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( SoftParticleUniforms )

	DEFINE_UNIFORM( mat4, modelViewMatrix )
	DEFINE_UNIFORM( mat4, projectionMatrix )
	DEFINE_UNIFORM( sampler, particleColorTexture )
	DEFINE_UNIFORM( sampler, depthTexture )
	DEFINE_UNIFORM( vec2, invDepthTextureSize )
	DEFINE_UNIFORM( vec4, colorMul )
	DEFINE_UNIFORM( vec4, colorAdd )
	DEFINE_UNIFORM( float, invParticleRadius )
	DEFINE_UNIFORM( float, invSceneFadeCoeff )
	DEFINE_UNIFORM( vec4, fadeMask )
};

struct CustomShaderUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( CustomShaderUniforms )

	DEFINE_UNIFORM( mat4, modelViewMatrix )
	DEFINE_UNIFORM( mat4, projectionMatrix )

	DEFINE_UNIFORM( vec4, scalePotToWindow )
	DEFINE_UNIFORM( vec4, scaleWindowToUnit )
	DEFINE_UNIFORM( vec4, scaleDepthCoords )
	DEFINE_UNIFORM( vec4, viewOriginGlobal )
	DEFINE_UNIFORM( vec4, viewOriginLocal )
	DEFINE_UNIFORM( vec4, modelMatrixRow0 )
	DEFINE_UNIFORM( vec4, modelMatrixRow1 )
	DEFINE_UNIFORM( vec4, modelMatrixRow2 )

	DEFINE_UNIFORM( vec4, localParam0 )
	DEFINE_UNIFORM( vec4, localParam1 )
	DEFINE_UNIFORM( vec4, localParam2 )
	DEFINE_UNIFORM( vec4, localParam3 )
	GLSLUniform_vec4 *localParams[4] = { &localParam0, &localParam1, &localParam2, &localParam3 };

	DEFINE_UNIFORM( sampler, texture0 )
	DEFINE_UNIFORM( sampler, texture1 )
	DEFINE_UNIFORM( sampler, texture2 )
	DEFINE_UNIFORM( sampler, texture3 )
	DEFINE_UNIFORM( sampler, texture4 )
	DEFINE_UNIFORM( sampler, texture5 )
	DEFINE_UNIFORM( sampler, texture6 )
	DEFINE_UNIFORM( sampler, texture7 )
	GLSLUniform_sampler *textures[8] = { &texture0, &texture1, &texture2, &texture3, &texture4, &texture5, &texture6, &texture7 };
};

static GLSLProgram *LoadShader(const idStr &name) {
	// TODO: delete the "new" suffix after old backend code is deleted
	// right now both shaders are called "environment"
	return programManager->LoadFromFiles( name + "_new", "stages/surface_passes/" + name + ".vert.glsl", "stages/surface_passes/" + name + ".frag.glsl" );
}

void SurfacePassesStage::Init() {
	simpleTextureShader = LoadShader( "simple_texture" );
	softParticleShader = LoadShader( "soft_particle" );
	environmentShader = LoadShader( "environment" );
}

void SurfacePassesStage::Shutdown() {}

bool SurfacePassesStage::NeedCurrentRenderTexture( const viewDef_t *viewDef, const drawSurf_t **drawSurfs, int numDrawSurfs ) {
	this->viewDef = viewDef;

	for ( int i = 0; i < numDrawSurfs; i++ ) {
		const drawSurf_t *drawSurf = drawSurfs[i];
		if ( !ShouldDrawSurf( drawSurf ) )
			continue;
		const idMaterial *shader = drawSurf->material;

		for ( int stage = 0; stage < shader->GetNumStages() ; stage++ ) {
			const shaderStage_t *pStage = shader->GetStage( stage );
			if ( !ShouldDrawStage( drawSurf, pStage ) )
				continue;

			if ( pStage->newStage ) {
				// typical usage in various heatHaze materials
				for ( int i = 0; i < pStage->newStage->numFragmentProgramImages; i++ ) {
					if ( pStage->newStage->fragmentProgramImages[i] == globalImages->currentRenderImage ) {
						return true;
					}
				}
			}
			if ( pStage->texture.image == globalImages->currentRenderImage ) {
				// this only happens for builtin and special portalsky material!
				assert( drawSurf->material->GetSort() == SS_PORTAL_SKY );
				return true;
			}
		}
	}

	return false;
}

void SurfacePassesStage::DrawSurfaces( const viewDef_t *viewDef, const drawSurf_t **drawSurfs, int numDrawSurfs ) {
	TRACE_GL_SCOPE( "SurfacePassesStage" );

	this->viewDef = viewDef;

	for ( int i = 0; i < numDrawSurfs; i++ ) {
		if ( !ShouldDrawSurf( drawSurfs[i] ) )
			continue;

		DrawSurf( drawSurfs[i] );
	}

	// need to restore for some reason...
	GL_Cull( CT_FRONT_SIDED );
}

bool SurfacePassesStage::ShouldDrawSurf( const drawSurf_t *drawSurf ) const {
	const idMaterial *shader = drawSurf->material;

	if ( !shader->HasAmbient() ) {
		return false;
	}

	if ( shader->IsPortalSky() ) {	// NB TDM portal sky does not use this flag or whatever mechanism
		return false;				// it used to support. Our portalSky is drawn in this procedure using
	}
	// the skybox image captured in _currentRender. -- SteveL working on #4182
	if ( drawSurf->material->GetSort() == SS_PORTAL_SKY && g_enablePortalSky.GetInteger() == 2 ) {
		return false;
	}

	// some deforms may disable themselves by setting numIndexes = 0
	if ( !drawSurf->numIndexes ) {
		return false;
	}

	if ( !drawSurf->ambientCache.IsValid() || !drawSurf->indexCache.IsValid() ) {
#ifdef _DEBUG
		common->Printf( "SurfacePassesStage: missing vertex or index cache\n" );
#endif
		return false;
	}

	return true;
}

void SurfacePassesStage::DrawSurf( const drawSurf_t *drawSurf ) {
	const idMaterial *shader = drawSurf->material;

	for ( int stage = 0; stage < shader->GetNumStages() ; stage++ ) {
		const shaderStage_t *pStage = shader->GetStage( stage );

		if ( !ShouldDrawStage( drawSurf, pStage ) )
			continue;

		DrawStage( drawSurf, pStage );
	}
}

bool SurfacePassesStage::ShouldDrawStage( const drawSurf_t *drawSurf, const shaderStage_t *pStage ) const {
	// check the enable condition
	if ( !drawSurf->IsStageEnabled( pStage ) ) {
		return false;
	}
	// skip the stages involved in lighting
	if ( pStage->lighting != SL_AMBIENT ) {
		return false;
	}
	// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
	int blendFunc = pStage->drawStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
	if ( blendFunc == ( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE ) ) {
		return false;
	}

	// sometimes we can optimize away draw if output color is surely zero
	// we do this for "simple" mode only, and only for two typical vertex color modes
	StageType type = ChooseType( drawSurf, pStage );
	if ( type == ST_SIMPLE_TEXTURE && ( pStage->vertexColor == SVC_IGNORE || pStage->vertexColor == SVC_MODULATE ) ) {
		idVec4 regColor = drawSurf->GetStageColor( pStage );
		bool zeroAlpha = regColor[3] == 0.0f;
		bool zeroColor = regColor[0] == 0.0f && regColor[1] == 0.0f && regColor[2] == 0.0f;

		if ( zeroAlpha && blendFunc == ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) ) {
			// zero output alpha = no effect
			return false;
		}
		if ( zeroColor && blendFunc == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE ) ) {
			// zero output color = no effect
			return false;
		}
	}

	/*
	 * TODO: does it even make sense?!
	 * zero register color does not always mean zero output color...
	StageType type = ChooseType(drawSurf, pStage);
	if ( type == ST_SIMPLE_TEXTURE || type == ST_ENVIRONMENT ) {
		// set the color
		float color[4];
		for ( int c = 0; c < 4; c++ )
			color[c] = regs[pStage->color.registers[c]];

		// skip the entire stage if an add would be black
		if ( ( pStage->drawStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE ) && color[0] <= 0 && color[1] <= 0 && color[2] <= 0 ) {
			return false;
		}

		// skip the entire stage if a blend would be completely transparent
		if ( ( pStage->drawStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) && color[3] <= 0 ) {
			return false;
		}
	}
	*/

	return true;
}

SurfacePassesStage::StageType SurfacePassesStage::ChooseType( const drawSurf_t *drawSurf, const shaderStage_t *pStage ) {
	texgen_t texgen = pStage->texture.texgen;

	if ( pStage->newStage ) {
		return ST_CUSTOM_SHADER;
	} else if ( ( drawSurf->dsFlags & DSF_SOFT_PARTICLE ) && drawSurf->particle_radius > 0.0f ) {
		return ST_SOFT_PARTICLE;
	} else if ( texgen == TG_REFLECT_CUBE ) {
		return ST_ENVIRONMENT;
	} else if ( texgen == TG_EXPLICIT || texgen == TG_SCREEN || texgen == TG_SKYBOX_CUBE || texgen == TG_WOBBLESKY_CUBE ) {
		return ST_SIMPLE_TEXTURE;
	} else {
		assert(0);
		return StageType(-1);
	}
}

void SurfacePassesStage::DrawStage( const drawSurf_t *drawSurf, const shaderStage_t *pStage ) {
	const idMaterial *shader = drawSurf->material;
	GL_Cull( shader->GetCullType() );

	// set polygon offset if necessary
	ApplyDepthTweaks depthTweaks( drawSurf );

	// change the scissor if needed
	backEnd.currentScissor = drawSurf->scissorRect;
	FB_ApplyScissor();

	vertexCache.VertexPosition( drawSurf->ambientCache );

	StageType type = ChooseType( drawSurf, pStage );
	if ( type == ST_SIMPLE_TEXTURE ) {
		DrawSimpleTexture( drawSurf, pStage );
	} else if ( type == ST_ENVIRONMENT ) {
		DrawEnvironment( drawSurf, pStage );
	} else if ( type == ST_SOFT_PARTICLE ) {
		DrawSoftParticle( drawSurf, pStage );
	} else if ( type == ST_CUSTOM_SHADER ) {
		DrawCustomShader( drawSurf, pStage );
	} 
}

void SurfacePassesStage::DrawSimpleTexture( const drawSurf_t *drawSurf, const shaderStage_t *pStage ) {
	const float *regs = drawSurf->shaderRegisters;

	// specify how to compute texcoords in vertex shader
	int texgen = 0;
	if ( pStage->texture.texgen == TG_SCREEN ) {
		texgen = TEXGEN_SCREEN;
	} else if ( pStage->texture.texgen == TG_EXPLICIT ) {
		texgen = TEXGEN_EXPLICIT;
	} else if ( pStage->texture.texgen == TG_SKYBOX_CUBE || pStage->texture.texgen == TG_WOBBLESKY_CUBE ) {
		texgen = TEXGEN_CUBEMAP;
	} else {
		assert(0);
	}

	simpleTextureShader->Activate();
	SimpleTextureUniforms *uniforms = simpleTextureShader->GetUniformGroup<SimpleTextureUniforms>();

	uniforms->projectionMatrix.Set( viewDef->projectionMatrix );
	uniforms->modelViewMatrix.Set( drawSurf->space->modelViewMatrix );
	idVec3 localViewOrigin;
	R_GlobalPointToLocal( drawSurf->space->modelMatrix, viewDef->renderView.vieworg, localViewOrigin );
	uniforms->viewOrigin.Set( localViewOrigin );
	uniforms->texgen.Set( texgen );
	uniforms->textureMatrix.Set( drawSurf->GetTextureMatrix( pStage ) );

	// bind texture (either 2D texture or cubemap is used)
	GL_SelectTexture( 0 );
	BindVariableStageImage( &pStage->texture, regs );
	if ( texgen == TEXGEN_CUBEMAP ) {
		uniforms->cubemap.Set( 0 );
		uniforms->texture.Set( 1 );
		GL_SelectTexture( 1 );
		globalImages->defaultImage->Bind();
	} else {
		uniforms->cubemap.Set( 1 );
		uniforms->texture.Set( 0 );
		GL_SelectTexture( 1 );
		globalImages->blackCubeMapImage->Bind();
	}

	// set the color
	idVec4 regColor = drawSurf->GetStageColor( pStage );
	if ( pStage->vertexColor == SVC_IGNORE ) {
		uniforms->colorMul.Set( 0, 0, 0, 0 );
		uniforms->colorAdd.Set( regColor );
	} else if ( pStage->vertexColor == SVC_MODULATE ) {
		uniforms->colorMul.Set( regColor );
		uniforms->colorAdd.Set( 0, 0, 0, 0 );
	} else if ( pStage->vertexColor == SVC_INVERSE_MODULATE ) {
		uniforms->colorMul.Set( -regColor[0], -regColor[1], -regColor[2], -1 );
		uniforms->colorAdd.Set( regColor );
	}

	GL_State( pStage->drawStateBits );

	// draw it
	RB_DrawElementsWithCounters( drawSurf, DCK_SURFACE_PASS );
}

void SurfacePassesStage::DrawEnvironment( const drawSurf_t *drawSurf, const shaderStage_t *pStage ) {
	environmentShader->Activate();
	EnvironmentUniforms *uniforms = environmentShader->GetUniformGroup<EnvironmentUniforms>();

	uniforms->projectionMatrix.Set( viewDef->projectionMatrix );
	uniforms->modelViewMatrix.Set( drawSurf->space->modelViewMatrix );
	uniforms->modelMatrix.Set( drawSurf->space->modelMatrix );
	uniforms->globalViewOrigin.Set( viewDef->renderView.vieworg );

	// see if there is also a bump map specified
	const shaderStage_t *bumpStage = drawSurf->material->GetBumpStage();
	idImage *bumpMap = nullptr;
	if ( bumpStage ) {
		bumpMap = bumpStage->texture.image;
	} else {
		bumpMap = globalImages->flatNormalMap;
	}

	// set textures
	uniforms->normalMap.Set( 0 );
	GL_SelectTexture( 0 );
	bumpMap->Bind();
	uniforms->RGTC.Set( bumpMap->internalFormat == GL_COMPRESSED_RG_RGTC2 );

	uniforms->environmentMap.Set( 1 );
	GL_SelectTexture( 1 );
	pStage->texture.image->Bind();

	// look at the color
	idVec4 regColor = drawSurf->GetStageColor( pStage );

	// set settings which different in bumpmapped case
	// TODO: why do they differ?
	if ( bumpStage ) {
		uniforms->constant.Set( idVec4(0.4f) );
		uniforms->fresnel.Set( idVec4(3.0f) );
		uniforms->tonemapOutputColor.Set( true );
	} else {
		uniforms->constant.Set( regColor );
		uniforms->fresnel.Set( idVec4(0.0f) );
		uniforms->tonemapOutputColor.Set( false );
	}

	GL_State( pStage->drawStateBits );

	RB_DrawElementsWithCounters( drawSurf, DCK_SURFACE_PASS );
}

void SurfacePassesStage::DrawSoftParticle( const drawSurf_t *drawSurf, const shaderStage_t *pStage ) {
	softParticleShader->Activate();
	SoftParticleUniforms *uniforms = softParticleShader->GetUniformGroup<SoftParticleUniforms>();
	uniforms->projectionMatrix.Set( viewDef->projectionMatrix );
	uniforms->modelViewMatrix.Set( drawSurf->space->modelViewMatrix );

	// Bind image and _currentDepth
	GL_SelectTexture( 0 );
	pStage->texture.image->Bind();
	uniforms->particleColorTexture.Set( 0 );
	GL_SelectTexture( 1 );
	globalImages->currentDepthImage->Bind();
	uniforms->depthTexture.Set( 1 );

	// determine the blend mode (used by soft particles #3878)
	int srcBlendFactor = pStage->drawStateBits & GLS_SRCBLEND_BITS;
	int dstBlendFactor = pStage->drawStateBits & GLS_DSTBLEND_BITS;
	// stgatilov: some factors immediately mean we should fade corresponding component to zero
	bool fadeColor = false, fadeAlpha = false;
	if ( srcBlendFactor == GLS_SRCBLEND_ONE || dstBlendFactor == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR )
		fadeColor = true;
	if ( srcBlendFactor == GLS_SRCBLEND_SRC_ALPHA || dstBlendFactor == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA )
		fadeAlpha = true;
	// if can't decide, fade alpha to zero
	if ( !fadeColor && !fadeAlpha )
		fadeAlpha = true;
	// fade = true => value = 0 => fade coeff will take effect
	uniforms->fadeMask.Set( !fadeColor, !fadeColor, !fadeColor, !fadeAlpha );

	// fadeRange is the particle diameter for alpha blends (like smoke), but the particle radius for additive
	// blends (light glares), because additive effects work differently. Fog is half as apparent when a wall
	// is in the middle of it. Light glares lose no visibility when they have something to reflect off. See
	// issue #3878 for diagram
	if ( fadeAlpha ) {
		uniforms->invSceneFadeCoeff.Set( 0.5f );	// half fading speed
	} else {
		uniforms->invSceneFadeCoeff.Set( 1.0f );
	}
	uniforms->invParticleRadius.Set( 1.0f / drawSurf->particle_radius );
	// micro-optimization for fragment shader =(
	uniforms->invDepthTextureSize.Set( 1.0f / globalImages->currentDepthImage->uploadWidth, 1.0f / globalImages->currentDepthImage->uploadHeight );

	idVec4 regColor = drawSurf->GetStageColor( pStage );
	if ( pStage->vertexColor == SVC_IGNORE ) {
		// ignoring vertexColor is not recommended for particles, since particle system uses vertexColor for fading
		// however, there are existing particle effects that don't use it
		uniforms->colorAdd.Set( regColor );
		uniforms->colorMul.Set( 0, 0, 0, 0 );
	} else if ( pStage->vertexColor == SVC_MODULATE ) {
		uniforms->colorAdd.Set( 0, 0, 0, 0 );
		uniforms->colorMul.Set( 1, 1, 1, 1 );
	} else {
		assert(0);
	}

	// Disable depth clipping. The fragment program will handle it to allow overdraw.
	GL_State( pStage->drawStateBits | GLS_DEPTHFUNC_ALWAYS );

	RB_DrawElementsWithCounters( drawSurf, DCK_SURFACE_PASS );
}

void SurfacePassesStage::DrawCustomShader( const drawSurf_t *drawSurf, const shaderStage_t *pStage ) {
	newShaderStage_t *newStage = pStage->newStage;

	newStage->glslProgram->Activate();
	CustomShaderUniforms *uniforms = newStage->glslProgram->GetUniformGroup<CustomShaderUniforms>();

	// set standard transformations
	uniforms->projectionMatrix.Set( viewDef->projectionMatrix );
	uniforms->modelViewMatrix.Set( drawSurf->space->modelViewMatrix );
	// we need the model matrix without it being combined with the view matrix
	// so we can transform local vectors to global coordinates
	idMat4 modelMatrix;
	memcpy( &modelMatrix, drawSurf->space->modelMatrix, sizeof(modelMatrix) );
	modelMatrix.TransposeSelf();
	uniforms->modelMatrixRow0.Set( modelMatrix[0] );
	uniforms->modelMatrixRow1.Set( modelMatrix[1] );
	uniforms->modelMatrixRow2.Set( modelMatrix[2] );

	// obsolete: screen power of two correction factor, assuming the copy to _currentRender
	// also copied an extra row and column for the bilerp
	uniforms->scalePotToWindow.Set( 1, 1, 0, 1 );
	// window coord to 0.0 to 1.0 conversion
	uniforms->scaleWindowToUnit.Set(
		1.0f / frameBuffers->activeFbo->Width(),
		1.0f / frameBuffers->activeFbo->Height(),
		0, 1
	);
	// #3877: Allow shaders to access depth buffer.
	// Two useful ratios are packed into this parm: [0] and [1] hold the x,y multipliers you need to map a screen
	// coordinate (fragment position) to the depth image: those are simply the reciprocal of the depth
	// image size, which has been rounded up to a power of two. Slots [2] and [3] hold the ratio of the depth image
	// size to the current render image size. These sizes can differ if the game crops the render viewport temporarily
	// during post-processing effects. The depth render is smaller during the effect too, but the depth image doesn't
	// need to be downsized, whereas the current render image does get downsized when it's captured by the game after
	// the skybox render pass. The ratio is needed to map between the two render images.
	uniforms->scaleDepthCoords.Set(
		1.0f / globalImages->currentDepthImage->uploadWidth,
		1.0f / globalImages->currentDepthImage->uploadHeight,
		float( globalImages->currentRenderImage->uploadWidth ) / globalImages->currentDepthImage->uploadWidth,
		float( globalImages->currentRenderImage->uploadHeight ) / globalImages->currentDepthImage->uploadHeight
	);

	// set eye position in global space
	idVec3 viewOriginGlobal = viewDef->renderView.vieworg;
	uniforms->viewOriginGlobal.Set( idVec4( viewOriginGlobal, 1.0f ) );
	// set eye position in model space
	idVec3 viewOriginLocal;
	R_GlobalPointToLocal( drawSurf->space->modelMatrix, viewOriginGlobal, viewOriginLocal );
	uniforms->viewOriginLocal.Set( idVec4( viewOriginLocal, 1.0f ) );

	//============================================================================

	// setting local parameters (specified in material definition)
	const float *regs = drawSurf->shaderRegisters;
	for ( int i = 0; i < newStage->numVertexParms; i++ ) {
		idVec4 parm = drawSurf->GetRegisterVec4( newStage->vertexParms[i] );
		uniforms->localParams[ i ]->Set( parm );
	}

	// setting textures
	// note: the textures are also bound to TUs at this moment
	for ( int i = 0; i < newStage->numFragmentProgramImages; i++ ) {
		if ( newStage->fragmentProgramImages[i] ) {
			GL_SelectTexture( i );
			newStage->fragmentProgramImages[i]->Bind();
			uniforms->textures[ i ]->Set( i );
		}
	}

	GL_State( pStage->drawStateBits );

	RB_DrawElementsWithCounters( drawSurf, DCK_SURFACE_PASS );
}

void SurfacePassesStage::BindVariableStageImage( const textureStage_t *texture, const float *regs ) {
	if ( texture->cinematic ) {
		if ( r_skipDynamicTextures.GetBool() ) {
			globalImages->defaultImage->Bind();
			return;
		}

		// offset time by shaderParm[7] (FIXME: make the time offset a parameter of the shader?)
		// We make no attempt to optimize for multiple identical cinematics being in view, or
		// for cinematics going at a lower framerate than the renderer.
		const cinData_t cin = texture->cinematic->ImageForTime( ( int )( 1000 * ( viewDef->floatTime + viewDef->renderView.shaderParms[11] ) ) );

		if ( cin.image ) {
			globalImages->cinematicImage->UploadScratch( cin.image, cin.imageWidth, cin.imageHeight );
		} else {
			globalImages->blackImage->Bind();
		}
	} else if ( texture->image ) {
		texture->image->Bind();
	}
}
