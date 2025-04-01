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

#include "renderer/backend/stages/InteractionStage.h"

#include "renderer/backend/RenderBackend.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/GLSLUniforms.h"
#include "renderer/backend/FrameBuffer.h"
#include "renderer/backend/FrameBufferManager.h"
#include "renderer/backend/stages/AmbientOcclusionStage.h"

idCVar r_shadowMapOnTranslucent(
	"r_shadowMapOnTranslucent", "0", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE,
	"Are shadows cast on translucent surfaces?\n"
	"Note: stencil shadows cannot work on translucent objects."
);


struct InteractionStage::Uniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( Uniforms )

	DEFINE_UNIFORM( sampler, normalTexture )
	DEFINE_UNIFORM( sampler, diffuseTexture )
	DEFINE_UNIFORM( sampler, specularTexture )
	DEFINE_UNIFORM( sampler, lightProjectionTexture )
	DEFINE_UNIFORM( sampler, lightProjectionCubemap )
	DEFINE_UNIFORM( sampler, lightFalloffTexture )
	DEFINE_UNIFORM( int, useNormalIndexedDiffuse )
	DEFINE_UNIFORM( int, useNormalIndexedSpecular )
	DEFINE_UNIFORM( sampler, lightDiffuseCubemap )
	DEFINE_UNIFORM( sampler, lightSpecularCubemap )
	DEFINE_UNIFORM( sampler, ssaoTexture )
	DEFINE_UNIFORM( vec3, globalViewOrigin )
	DEFINE_UNIFORM( vec3, globalLightOrigin )

	DEFINE_UNIFORM( int, cubic )
	DEFINE_UNIFORM( float, gamma )
	DEFINE_UNIFORM( float, minLevel )
	DEFINE_UNIFORM( int, ssaoEnabled )
	DEFINE_UNIFORM( vec2, renderResolution )

	DEFINE_UNIFORM( int, shadows )
	DEFINE_UNIFORM( int, softShadowsQuality )
	DEFINE_UNIFORM( float, softShadowsRadius )
	DEFINE_UNIFORM( vec4, shadowRect )
	DEFINE_UNIFORM( int, shadowMapCullFront )
	DEFINE_UNIFORM( sampler, stencilTexture )
	DEFINE_UNIFORM( sampler, depthTexture )
	DEFINE_UNIFORM( sampler, shadowMap )
	DEFINE_UNIFORM( ivec2, stencilMipmapsLevel )
	DEFINE_UNIFORM( vec4, stencilMipmapsScissor )
	DEFINE_UNIFORM( sampler, stencilMipmapsTexture )

	DEFINE_UNIFORM( mat4, modelMatrix )
	DEFINE_UNIFORM( mat4, modelViewMatrix )
	DEFINE_UNIFORM( mat4, projectionMatrix )
	DEFINE_UNIFORM( vec4, bumpMatrix/*[2]*/ )
	DEFINE_UNIFORM( vec4, diffuseMatrix/*[2]*/ )
	DEFINE_UNIFORM( vec4, specularMatrix/*[2]*/ )
	DEFINE_UNIFORM( mat4, lightProjectionFalloff )
	DEFINE_UNIFORM( vec4, lightTextureMatrix/*[2]*/ )
	DEFINE_UNIFORM( vec4, colorModulate )
	DEFINE_UNIFORM( vec4, colorAdd )
	DEFINE_UNIFORM( vec4, diffuseColor )
	DEFINE_UNIFORM( vec4, specularColor )
	DEFINE_UNIFORM( vec4, hasTextureDNS )
	DEFINE_UNIFORM( int, useBumpmapLightTogglingFix )
	DEFINE_UNIFORM( float, RGTC )
};

enum TextureUnits {
	TU_NORMAL = 0,
	TU_DIFFUSE = 1,
	TU_SPECULAR = 2,
	TU_LIGHT_PROJECT = 3,
	TU_LIGHT_PROJECT_CUBE = 4,
	TU_LIGHT_FALLOFF = 5,
	TU_LIGHT_DIFFUSE_CUBE = 6,
	TU_LIGHT_SPECULAR_CUBE = 7,
	TU_SSAO = 8,
	TU_SHADOW_MAP = 9,
	TU_SHADOW_DEPTH = 10,
	TU_SHADOW_STENCIL = 11,
	TU_SHADOW_STENCIL_MIPMAPS = 12,
};

void InteractionStage::LoadInteractionShader( GLSLProgram *shader, const idStr &baseName ) {
	shader->LoadFromFiles( "stages/interaction/" + baseName + ".vs.glsl", "stages/interaction/" + baseName + ".fs.glsl" );
	Uniforms *uniforms = shader->GetUniformGroup<Uniforms>();
	uniforms->lightProjectionCubemap.Set( TU_LIGHT_PROJECT_CUBE );
	uniforms->lightProjectionTexture.Set( TU_LIGHT_PROJECT );
	uniforms->lightFalloffTexture.Set( TU_LIGHT_FALLOFF );
	uniforms->lightDiffuseCubemap.Set( TU_LIGHT_DIFFUSE_CUBE );
	uniforms->lightSpecularCubemap.Set( TU_LIGHT_SPECULAR_CUBE );
	uniforms->ssaoTexture.Set( TU_SSAO );
	uniforms->stencilTexture.Set( TU_SHADOW_STENCIL );
	uniforms->stencilMipmapsTexture.Set( TU_SHADOW_STENCIL_MIPMAPS );
	uniforms->depthTexture.Set( TU_SHADOW_DEPTH );
	uniforms->shadowMap.Set( TU_SHADOW_MAP );
	uniforms->normalTexture.Set( TU_NORMAL );
	uniforms->diffuseTexture.Set( TU_DIFFUSE );
	uniforms->specularTexture.Set( TU_SPECULAR );
}


InteractionStage::InteractionStage() {}

void InteractionStage::Init() {
	ambientInteractionShader = programManager->LoadFromGenerator( "interaction_ambient", 
		[this](GLSLProgram *shader) { LoadInteractionShader( shader, "interaction.ambient" ); } );
	stencilInteractionShader = programManager->LoadFromGenerator( "interaction_stencil", 
		[this](GLSLProgram *shader) { LoadInteractionShader( shader, "interaction.stencil" ); } );
	shadowMapInteractionShader = programManager->LoadFromGenerator( "interaction_shadowmap", 
		[this](GLSLProgram *shader) { LoadInteractionShader( shader, "interaction.shadowmap" ); } );
}

void InteractionStage::Shutdown() {}

bool InteractionStage::ShouldDrawLight( const viewLight_t *vLight ) const {
	if ( vLight->lightShader->IsFogLight() || vLight->lightShader->IsBlendLight() )
		return false;

	// if there are no interactions, get out!
	if ( !vLight->localInteractions && !vLight->globalInteractions && !vLight->translucentInteractions )
		return false;

	return true;
}

void InteractionStage::DrawInteractions( const viewDef_t *viewDef, const viewLight_t *vLight, DrawSurfaceList list, const TiledCustomMipmapStage *stencilShadowMipmaps ) {
	drawSurf_t *interactionSurfs = nullptr;
	if ( list == DSL_GLOBAL ) {
		interactionSurfs = vLight->globalInteractions;
	} else if ( list == DSL_LOCAL ) {
		interactionSurfs = vLight->localInteractions;
	} else if ( list == DSL_TRANSLUCENT ) {
		interactionSurfs = vLight->translucentInteractions;
	}

	if ( !interactionSurfs ) {
		return;
	}
	if ( r_skipTranslucent.GetBool() && list == DSL_TRANSLUCENT ) {
		return;
	}

	if ( vLight->lightShader->IsAmbientLight() ) {
		if ( r_skipAmbient.GetInteger() & 2 )
			return;
	} else if ( r_skipInteractions.GetBool() ) {
		if( r_skipInteractions.GetInteger() == 1 || !vLight->lightDef->parms.noShadows )
			return;
	}

	TRACE_GL_SCOPE( "DrawInteractions" );

	this->viewDef = viewDef;
	this->vLight = vLight;

	// if using float buffers, alpha values are not clamped and can stack up quite high, since most interactions add 1 to its value
	// this in turn causes issues with some shader stage materials that use DST_ALPHA blending.
	// masking the alpha channel for interactions seems to fix those issues, but only do it for float buffers in case it has
	// unwanted side effects
	int alphaMask = r_fboColorBits.GetInteger() == 64 ? GLS_ALPHAMASK : 0;

	int depthFunc = list == DSL_TRANSLUCENT ? GLS_DEPTHFUNC_LESS : GLS_DEPTHFUNC_EQUAL;

	// perform setup here that will be constant for all interactions
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | alphaMask | depthFunc );

	backEnd.currentScissor = vLight->scissorRect;
	FB_ApplyScissor();

	// bind the vertex and fragment program
	ChooseInteractionProgram( vLight, list == DSL_TRANSLUCENT );
	uniforms->cubic.Set( vLight->lightShader->IsCubicLight() ? 1 : 0 );
	uniforms->globalLightOrigin.Set( vLight->globalLightOrigin );
	uniforms->globalViewOrigin.Set( viewDef->renderView.vieworg );
	uniforms->renderResolution.Set( frameBuffers->activeFbo->Width(), frameBuffers->activeFbo->Height() );
	uniforms->projectionMatrix.Set( viewDef->projectionMatrix );

	idList<const drawSurf_t *> drawSurfs;
	for ( const drawSurf_t *surf = interactionSurfs; surf; surf = surf->nextOnLight) {
		drawSurfs.AddGrow( surf );
	}
	std::sort( drawSurfs.begin(), drawSurfs.end(), [](const drawSurf_t *a, const drawSurf_t *b) {
		if ( a->ambientCache.isStatic != b->ambientCache.isStatic )
			return a->ambientCache.isStatic;
		if ( a->indexCache.isStatic != b->indexCache.isStatic )
			return a->indexCache.isStatic;

		return a->material < b->material;
	} );

	if ( vLight->lightShader->IsAmbientLight() ) {
		// stgatilov #6090: ambient lights can have cubemaps indexed by surface normal
		// separately for diffuse and specular

		idImage *cubemap = vLight->lightShader->LightAmbientDiffuse();
		if ( cubemap ) {
			uniforms->useNormalIndexedDiffuse.Set( true );
			GL_SelectTexture( TU_LIGHT_DIFFUSE_CUBE );
			cubemap->Bind();
		}
		else {
			uniforms->useNormalIndexedDiffuse.Set( false );
		}

		cubemap = vLight->lightShader->LightAmbientSpecular();
		if ( cubemap ) {
			uniforms->useNormalIndexedSpecular.Set( true );
			GL_SelectTexture( TU_LIGHT_SPECULAR_CUBE );
			cubemap->Bind();
		}
		else {
			uniforms->useNormalIndexedSpecular.Set( false );
		}
	}

	GL_SelectTexture( TU_LIGHT_FALLOFF );
	vLight->falloffImage->Bind();

	if ( r_softShadowsQuality.GetBool() && !viewDef->IsLightGem() || vLight->shadows == LS_MAPS )
		BindShadowTexture( stencilShadowMipmaps );

	if( vLight->lightShader->IsAmbientLight() && ambientOcclusion->ShouldEnableForCurrentView() ) {
		ambientOcclusion->BindSSAOTexture( TU_SSAO );
	}

	if ( stencilShadowMipmaps && stencilShadowMipmaps->IsFilled() ) {
		GL_SelectTexture( TU_SHADOW_STENCIL_MIPMAPS );
		stencilShadowMipmaps->BindMipmapTexture();
		// note: correct level is evaluated when mipmaps object is created
		int useLevel = stencilShadowMipmaps->GetMaxLevel();
		int baseLevel = stencilShadowMipmaps->GetBaseLevel();
		idScreenRect scissor = stencilShadowMipmaps->GetScissorAtLevel( useLevel );
		uniforms->stencilMipmapsLevel.Set( useLevel, baseLevel );
		uniforms->stencilMipmapsScissor.Set(
			(scissor.x1 + 0.5f) * (1 << useLevel),
			(scissor.y1 + 0.5f) * (1 << useLevel),
			(scissor.x2 + 0.5f) * (1 << useLevel),
			(scissor.y2 + 0.5f) * (1 << useLevel)
		);
	}
	else {
		uniforms->stencilMipmapsLevel.Set( -1, -1 );
	}

	const idMaterial	*lightShader = vLight->lightShader;
	for ( int lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ ) {
		const shaderStage_t	*lightStage = lightShader->GetStage( lightStageNum );

		// ignore stages that fail the condition
		if ( !vLight->IsStageEnabled( lightStage ) ) {
			continue;
		}

		if ( vLight->lightShader->IsCubicLight() ) {
			GL_SelectTexture( TU_LIGHT_PROJECT_CUBE );
			lightStage->texture.image->Bind();
		}
		else {
			GL_SelectTexture( TU_LIGHT_PROJECT );
			lightStage->texture.image->Bind();
		}
		// careful - making bindless textures resident could bind an arbitrary texture to the currently active
		// slot, so reset this to something that is safe to override in bindless mode!
		GL_SelectTexture(TU_NORMAL);

		for ( const drawSurf_t *surf : drawSurfs ) {
			if ( !surf->ambientCache.IsValid() || !surf->indexCache.IsValid() ) {
#ifdef _DEBUG
				common->Printf( "InteractionStage: missing vertex or index cache\n" );
#endif
				continue;
			}

			ApplyDepthTweaks depthTweaks( surf );

			ProcessSingleSurface( vLight, lightStage, surf );
		}
	}

	GL_SelectTexture( 0 );

	GLSLProgram::Deactivate();
}

void InteractionStage::BindShadowTexture( const TiledCustomMipmapStage *stencilShadowMipmaps ) {
	if ( vLight->shadowMapPage.width > 0 ) {
		GL_SelectTexture( TU_SHADOW_MAP );
		globalImages->shadowAtlas->Bind();
	} else {
		GL_SelectTexture( TU_SHADOW_DEPTH );
		globalImages->currentDepthImage->Bind();

		GL_SelectTexture( TU_SHADOW_STENCIL );
		globalImages->shadowDepthFbo->Bind();
		if (globalImages->shadowDepthFbo->texnum != idImage::TEXTURE_NOT_LOADED)
			qglTexParameteri( GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX );
	}
}

void InteractionStage::ChooseInteractionProgram( const viewLight_t *vLight, bool translucent ) {
	if ( vLight->lightShader->IsAmbientLight() ) {
		interactionShader = ambientInteractionShader;
	} else if ( vLight->shadowMapPage.width > 0 ) {
		interactionShader = shadowMapInteractionShader;
	} else {
		interactionShader = stencilInteractionShader;
	}
	interactionShader->Activate();

	uniforms = interactionShader->GetUniformGroup<Uniforms>();
	uniforms->gamma.Set( viewDef->IsLightGem() ? 1 : r_ambientGamma.GetFloat() );
	uniforms->minLevel.Set( r_ambientMinLevel.GetFloat() );
	uniforms->ssaoEnabled.Set( ambientOcclusion->ShouldEnableForCurrentView() ? 1 : 0 );

	bool doShadows = !vLight->noShadows && vLight->lightShader->LightCastsShadows();
	// stgatilov #6490: stencil shadows cannot properly cast shadows on translucent objects
	// so we force-disable all shadows for stencil, and normally do the same for shadow maps too
	if ( translucent && !( r_shadowMapOnTranslucent.GetBool() && vLight->shadowMapPage.width > 0 ) )
		doShadows = false;

	if ( doShadows && vLight->shadows == LS_MAPS ) {
		// FIXME shadowmap only valid when globalInteractions not empty, otherwise garbage
		doShadows = vLight->globalInteractions != NULL;
	}
	if ( doShadows ) {
		uniforms->shadows.Set(true);
		const renderCrop_t &page = vLight->shadowMapPage;
		// https://stackoverflow.com/questions/5879403/opengl-texture-coordinates-in-pixel-space
		idVec4 v( page.x, page.y, 0, page.width-1 );
		v.ToVec2() = (v.ToVec2() * 2 + idVec2( 1, 1 )) / (2 * 6 * r_shadowMapSize.GetInteger());
		v.w /= 6 * r_shadowMapSize.GetFloat();
		uniforms->shadowRect.Set( v );
	} else {
		uniforms->shadows.Set(false);
	}
	extern idCVarBool r_shadowMapCullFront;
	uniforms->shadowMapCullFront.Set( r_shadowMapCullFront );

	if ( doShadows && ( vLight->globalShadows || vLight->localShadows ) && !viewDef->IsLightGem() ) {
		uniforms->softShadowsQuality.Set( r_softShadowsQuality.GetInteger() );
	} else {
		uniforms->softShadowsQuality.Set( 0 );
	}
	uniforms->softShadowsRadius.Set( r_softShadowsRadius.GetFloat() ); // for soft stencil and all shadow maps

	PreparePoissonSamples();
}

void InteractionStage::ProcessSingleSurface( const viewLight_t *vLight, const shaderStage_t *lightStage, const drawSurf_t *surf ) {
	const idMaterial	*material = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	const idMaterial	*lightShader = vLight->lightShader;
	drawInteraction_t	inter;

	if ( !surf->ambientCache.IsValid() ) {
		return;
	}

	inter.surf = surf;

	inter.cubicLight = lightShader->IsCubicLight(); // nbohr1more #3881: cubemap lights
	inter.ambientLight = lightShader->IsAmbientLight();

	idPlane lightProject[4];
	R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[0], lightProject[0] );
	R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[1], lightProject[1] );
	R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[2], lightProject[2] );
	R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[3], lightProject[3] );

	memcpy( inter.lightProjection, lightProject, sizeof( inter.lightProjection ) );

	idMat4 lightTexMatrix = vLight->GetTextureMatrix( lightStage );
	// stgatilov: we no longer merge two transforms together, since we need light-volume coords in fragment shader
	inter.lightTextureMatrix[0] = lightTexMatrix[0];
	inter.lightTextureMatrix[1] = lightTexMatrix[1];

	inter.bumpImage = NULL;
	inter.specularImage = NULL;
	inter.diffuseImage = NULL;
	inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
	inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;

	// backEnd.lightScale is calculated so that lightColor[] will never exceed
	// tr.backEndRendererMaxLight
	idVec4 lightColor = vLight->GetStageColor( lightStage );
	lightColor.ToVec3() *= backEnd.lightScale;

	// go through the individual stages
	for ( int surfaceStageNum = 0; surfaceStageNum < material->GetNumStages(); surfaceStageNum++ ) {
		const shaderStage_t	*surfaceStage = material->GetStage( surfaceStageNum );

		if ( !surf->IsStageEnabled( surfaceStage ) )
			continue;


		void R_SetDrawInteraction( const shaderStage_t *surfaceStage, const float *surfaceRegs, idImage **image, idVec4 matrix[2], float color[4] );

		switch ( surfaceStage->lighting ) {
		case SL_AMBIENT: {
			// ignore ambient stages while drawing interactions
			break;
		}
		case SL_BUMP: {				
			if ( !r_skipBump.GetBool() ) {
				PrepareDrawCommand( &inter ); // draw any previous interaction
				inter.diffuseImage = NULL;
				inter.specularImage = NULL;
				R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.bumpImage, inter.bumpMatrix, NULL );
			}
			break;
		}
		case SL_DIFFUSE: {
			if ( inter.diffuseImage ) {
				PrepareDrawCommand( &inter );
			}
			R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.diffuseImage,
								  inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr() );
			inter.diffuseColor[0] *= lightColor[0];
			inter.diffuseColor[1] *= lightColor[1];
			inter.diffuseColor[2] *= lightColor[2];
			inter.diffuseColor[3] *= lightColor[3];
			inter.vertexColor = surfaceStage->vertexColor;
			break;
		}
		case SL_SPECULAR: {
			// nbohr1more: #4292 nospecular and nodiffuse fix
			if ( vLight->noSpecular ) {
				break;
			}
			if ( inter.specularImage ) {
				PrepareDrawCommand( &inter );
			}
			R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.specularImage,
								  inter.specularMatrix, inter.specularColor.ToFloatPtr() );
			inter.specularColor[0] *= lightColor[0];
			inter.specularColor[1] *= lightColor[1];
			inter.specularColor[2] *= lightColor[2];
			inter.specularColor[3] *= lightColor[3];
			inter.vertexColor = surfaceStage->vertexColor;
			break;
		}
		}
	}

	// draw the final interaction
	PrepareDrawCommand( &inter );
}

void InteractionStage::PrepareDrawCommand( drawInteraction_t *din ) {
	if ( !din->bumpImage ) {
		if ( !r_skipBump.GetBool() )
			return;
		din->bumpImage = globalImages->flatNormalMap;		
	}

	if ( !din->diffuseImage || r_skipDiffuse.GetBool() ) {
		din->diffuseImage = globalImages->blackImage;
	}

	if ( !din->specularImage || r_skipSpecular.GetBool() ) {
		din->specularImage = globalImages->blackImage;
	}

	// bind new material textures
	GL_SelectTexture( TU_NORMAL );
	din->bumpImage->Bind();
	GL_SelectTexture( TU_DIFFUSE );
	din->diffuseImage->Bind();
	GL_SelectTexture( TU_SPECULAR );
	din->specularImage->Bind();

	vertexCache.VertexPosition( din->surf->ambientCache );

	uniforms->modelMatrix.Set( din->surf->space->modelMatrix );
	uniforms->modelViewMatrix.Set( din->surf->space->modelViewMatrix );
	uniforms->bumpMatrix.SetArray( 2, (float*)din->bumpMatrix );
	uniforms->diffuseMatrix.SetArray( 2, (float*)din->diffuseMatrix );
	uniforms->specularMatrix.SetArray( 2, (float*)din->specularMatrix );
	uniforms->lightProjectionFalloff.Set( (float*)din->lightProjection );
	uniforms->lightTextureMatrix.SetArray( 2, (float*)din->lightTextureMatrix );
	switch ( din->vertexColor ) {
	case SVC_IGNORE:
		uniforms->colorModulate.Set( 0, 0, 0, 0 );
		uniforms->colorAdd.Set( 1, 1, 1, 1 );
		break;
	case SVC_MODULATE:
		uniforms->colorModulate.Set( 1, 1, 1, 1 );
		uniforms->colorAdd.Set( 0, 0, 0, 0 );
		break;
	case SVC_INVERSE_MODULATE:
		uniforms->colorModulate.Set( -1, -1, -1, -1 );
		uniforms->colorAdd.Set( 1, 1, 1, 1 );
		break;
	}
	uniforms->diffuseColor.Set( din->diffuseColor );
	uniforms->specularColor.Set( din->specularColor );
	if ( !din->bumpImage ) {
		uniforms->hasTextureDNS.Set( 1, 0, 1, 0 );
	} else {
		uniforms->hasTextureDNS.Set( 1, 1, 1, 0 );
	}
	uniforms->useBumpmapLightTogglingFix.Set( r_useBumpmapLightTogglingFix.GetBool() && !din->surf->material->ShouldCreateBackSides() );
	uniforms->RGTC.Set( din->bumpImage->internalFormat == GL_COMPRESSED_RG_RGTC2 );

	RB_DrawElementsWithCounters( din->surf, DCK_INTERACTION );
}

static void AddPoissonDiskSamples( idList<idVec2> &pts, float dist ) {
	static const int MaxFailStreak = 1000;
	idRandom rnd;
	int fails = 0;
	while ( 1 ) {
		idVec2 np;
		np.x = rnd.CRandomFloat();
		np.y = rnd.CRandomFloat();
		if ( np.LengthFast() > 1.0f ) {
			continue;
		}
		bool ok = true;
		for ( int i = 0; ok && i < pts.Num(); i++ ) {
			if ( (pts[i] - np).LengthFast() < dist ) {
				ok = false;
			}
		}
		if ( !ok ) {
			fails++;
			if ( fails == MaxFailStreak ) {
				break;
			}
		} else {
			pts.Append( np );
		}
	}
}

static void GeneratePoissonDiskSampling( idList<idVec2> &pts, int wantedCount ) {
	pts.Clear();
	pts.Append( idVec2( 0, 0 ) );
	if (wantedCount >= 12) {
		//pre-generate vertices of perfect hexagon
		for ( int i = 0; i < 6; i++ ) {
			float ang = 0.3f + idMath::TWO_PI * i / 6;
			float c, s;
			idMath::SinCos( ang, s, c );
			pts.Append( idVec2( c, s ) );
		}
	}
	float dist = idMath::Sqrt( 2.0f / wantedCount );
	int fixedK = pts.Num();
	do {
		pts.Resize( fixedK );
		AddPoissonDiskSamples( pts, dist );
		dist *= 0.9f;
	} while ( pts.Num() - 1 < wantedCount );
	idSwap( pts[0], pts[wantedCount] );
	pts.Resize( wantedCount );
}

struct PoissonSamplesUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( PoissonSamplesUniforms )
	DEFINE_UNIFORM( vec2, softShadowsSamples/*[???]*/ )
};

void InteractionStage::PreparePoissonSamples() {
	int sampleK = r_softShadowsQuality.GetInteger();
	if ( sampleK > 0 && poissonSamples.Num() != sampleK ) {
		GeneratePoissonDiskSampling( poissonSamples, sampleK );
	}
	PoissonSamplesUniforms *samplesUniforms = interactionShader->GetUniformGroup<PoissonSamplesUniforms>();
	samplesUniforms->softShadowsSamples.SetArray( sampleK, (float*)poissonSamples.Ptr() );
}
