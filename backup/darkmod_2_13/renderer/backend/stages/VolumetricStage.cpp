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
#include "renderer/backend/stages/VolumetricStage.h"

#include "renderer/backend/FrameBufferManager.h"
#include "renderer/backend/FrameBuffer.h"
#include "renderer/resources/Image.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/GLSLUniforms.h"

VolumetricStage volumetricImpl;
VolumetricStage *volumetric = &volumetricImpl;

idCVar r_volumetricSamples(
	"r_volumetricSamples", "24", CVAR_ARCHIVE | CVAR_INTEGER | CVAR_RENDERER,
	"How many samples to compute at every pixel of a volumetric light. "
	"Higher values improve quality but severely degrade performance. "
	"Zero value means using average color of projection/falloff textures and no shadows (very cheap).",
	0, 128
);
idCVar r_volumetricDither(
	"r_volumetricDither", "1", CVAR_ARCHIVE | CVAR_BOOL | CVAR_RENDERER,
	"Use randomized sample positions across screen pixels in volumetric lights. "
	"This greatly improves their quality, but adds high-frequency noise."
);
idCVar r_volumetricBlur(
	"r_volumetricBlur", "2", CVAR_ARCHIVE | CVAR_FLOAT | CVAR_RENDERER,
	"Standard deviation of blur applied to volumetric light. "
	"This clears the dither pattern out from result. "
	"Value 0 means no blurring is done.",
	0, 100.0
);
idCVar r_volumetricLowres(
	"r_volumetricLowres", "1", CVAR_ARCHIVE | CVAR_INTEGER | CVAR_RENDERER,
	"Use FBO of lower resolution for volumetric lights rendering. "
	"Value 1 means half-resolution, 2 means 1/4 resolution, etc.",
	0, 10
);


VolumetricStage::VolumetricStage() {}
VolumetricStage::~VolumetricStage() {}

void VolumetricStage::Init() {
	static const char *NAMES[2] = {"volumetric_work0", "volumetric_work1"};

	for (int p = 0; p < 2; p++) {
		workImage[p] = globalImages->ImageScratch( NAMES[p] );

		workFBO[p] = frameBuffers->CreateFromGenerator( NAMES[p], [this, p](FrameBuffer *fbo) {
			int scaleLevel = r_volumetricLowres.GetInteger();
			int fboWidth = frameBuffers->renderWidth >> scaleLevel;
			int fboHeight = frameBuffers->renderHeight >> scaleLevel;
			// projection image can be multicolored, so we need RGB
			// we don't need alpha, since we can premultiply on it
			// we use floats to avoid color banding at low values and capping at 1
			workImage[p]->GenerateAttachment( fboWidth, fboHeight, GL_R11F_G11F_B10F, GL_LINEAR );
			fbo->Init( fboWidth, fboHeight );
			workFBO[p]->AddColorRenderTexture( 0, workImage[p] );
		});
	}

	raymarchingShader = programManager->LoadFromGenerator("volumetric_raymarching", [this](GLSLProgram *shader) {
		shader->LoadFromFiles( "volumetric.vs", "volumetric_raymarching.fs" );
	});
	compositingShader = programManager->LoadFromGenerator("volumetric_compositing", [this](GLSLProgram *shader) {
		shader->LoadFromFiles( "volumetric.vs", "volumetric_compositing.fs" );
	});
	blurShader = programManager->LoadFromGenerator("volumetric_blur", [this](GLSLProgram *shader) {
		shader->LoadFromFiles( "volumetric.vs", "volumetric_blur.fs" );
	});
}

void VolumetricStage::Shutdown() {
	for (int p = 0; p < 2; p++) {
		if (workFBO[p]) {
			workFBO[p]->Destroy();
			workFBO[p] = nullptr;
		}
		if (workImage[p]) {
			workImage[p]->PurgeImage();
			workImage[p] = nullptr;
		}
	}
}

void VolumetricStage::RenderAll(const viewDef_t *viewDef) {
	for ( const viewLight_t *vLight = viewDef->viewLights ; vLight; vLight = vLight->next ) {
		if ( vLight->volumetricDust > 0.0f && !viewDef->IsLightGem() )
			RenderLight( viewDef, vLight );
	}
}

struct VolumetricStage::TemporaryData {
	bool useShadows;
	srfTriangles_t *frustumTris;
	idImage *projectionImage;
	idImage *falloffImage;
	idVec4 lightColor;
	idVec4 lightTexRows[2];
	int samples;			// how many samples to test (0 = no sampling)
	float dust;
	int alphaMode;			// light/fog mode?
	int dstBlend;			// dest factor of blending function on compositing
};

bool VolumetricStage::RenderLight(const viewDef_t *viewDef, const viewLight_t *viewLight) {
	if ( !workImage[0] ) {
		Init();
	}

	TRACE_GL_SCOPE( "VolumetricStage" );

	// not sure if it is important to set global variables, but let's require it for now
	this->viewDef = viewDef;
	this->viewLight = viewLight;

	if ( !viewLight->frustumTris->ambientCache.IsValid() ) {
		// ran out of vertex cache memory => skip it
		return false;
	}

	if ( r_volumetricLowres.IsModified() ) {
		Shutdown();
		Init();
		r_volumetricLowres.ClearModified();
	}

	TemporaryData data;
	PrepareRaymarching( data );

	bool useFBO = ( r_volumetricBlur.GetFloat() > 0.0f || r_volumetricLowres.GetBool() ) && data.samples > 0;

	subpixelShift.Zero();
	if ( r_volumetricLowres.GetInteger() > 0 )
		subpixelShift.Set( 0.5f / globalImages->currentDepthImage->uploadWidth, 0.5f / globalImages->currentDepthImage->uploadHeight );

	// out of two fragments, render the farther one
	GL_Cull( CT_BACK_SIDED );

	if ( useFBO ) {
		// sampling is expensive and uses dither: render to separate FBO first
		workFBO[0]->Bind();
		GL_ViewportRelative( 0, 0, 1, 1 );
		SetScissor();
		qglClearColor( 0, 0, 0, 0 );
		qglClear( GL_COLOR_BUFFER_BIT );
		// note: multiply color and alpha together in FBO
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	}
	else {
		// render to final FBO immediately, so enable compositing
		SetScissor();
		GL_State( GLS_SRCBLEND_SRC_ALPHA | data.dstBlend | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	}

	RenderRaymarching( data );

	if ( useFBO ) {
		if ( r_volumetricBlur.GetFloat() > 0.0f ) {
			// depth-aware blur on FBO data
			// note: we use separable implementation, it is not the same as true depth-aware Gauss blur
			workFBO[1]->Bind();
			qglClear( GL_COLOR_BUFFER_BIT );
			Blur( workImage[0], false );
			workFBO[0]->Bind();
			Blur( workImage[1], true );
		}

		// composite the FBO onto final image
		frameBuffers->currentRenderFbo->Bind();
		GL_ViewportRelative( 0, 0, 1, 1 );
		SetScissor();
		GL_State( GLS_SRCBLEND_SRC_ALPHA | data.dstBlend | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		PerformCompositing( workImage[0] );
	}

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

	// restore draw FBO
	frameBuffers->currentRenderFbo->Bind();
	// restore scissor
	GL_ViewportRelative( 0, 0, 1, 1 );
	FB_ApplyScissor();
	// restore culling
	GL_Cull( CT_FRONT_SIDED );	//default?
	// restore program
	GLSLProgram::Deactivate();

	return true;
}

void VolumetricStage::PrepareRaymarching(TemporaryData &data) {
	data.frustumTris = viewLight->frustumTris;

	data.useShadows = true;
	// note: all other noshadows settings already checked in R_SetLightDefViewLight
	if ( viewLight->volumetricNoshadows )
		data.useShadows = false;
	if ( viewLight->shadowMapPage.width <= 0 ) {
		// shadow map missing?
		assert(data.useShadows == false);
		data.useShadows = false;
	}

	const idMaterial *lightShader = viewLight->lightShader;
	const shaderStage_t *lightStage = lightShader->GetStage( 0 );
	data.projectionImage = lightStage->texture.image;
	data.falloffImage = viewLight->falloffImage;

	// light color uniform
	data.lightColor = viewLight->GetStageColor( lightStage );

	// light texture transform
	idMat4 lightTexMatrix = viewLight->GetTextureMatrix( lightStage );
	data.lightTexRows[0] = lightTexMatrix[0];
	data.lightTexRows[1] = lightTexMatrix[1];

	if ( viewLight->lightShader->IsFogLight() ) {
		// fog: use normal translucency-like blending
		data.dstBlend = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		// ignore both textures completely
		data.projectionImage = globalImages->whiteImage;
		data.falloffImage = globalImages->whiteImage;
		// disable sampling (no shadows, const textures => all samples are same)
		data.samples = 0;
		// use alpha generation for exponential attenuation (see shader for details)
		data.alphaMode = 1;
		if ( data.lightColor[3] <= 1.0 ) {
			// (shaderParm3 not specified)
			// cap = 1.0 / "volumetric_dust"
			data.dust = viewLight->volumetricDust;
		} else {
			// cap = "shaderParm3"
			data.dust = 1.0 / data.lightColor[3];
		}
	}
	else {
		// volumetric light: use additive blending, uncapped linear alpha
		data.dstBlend = GLS_DSTBLEND_ONE;
		data.alphaMode = 0;
		// use sampling, take dust parameter from volumetric_dust
		data.samples = r_volumetricSamples.GetInteger();
		data.dust = viewLight->volumetricDust;
		// apply light scale (it is applied to all lights)
		data.lightColor.ToVec3() *= backEnd.lightScale;
	}
}

void VolumetricStage::SetScissor() {
	// might draw to render FBO or to downscaled local FBO
	// so we need to rescale scissor rectangle to FBO resolution
	int w = frameBuffers->activeFbo->Width();
	int h = frameBuffers->activeFbo->Height();
	idScreenRect rect = viewLight->scissorRect;
	rect.x1 = rect.x1 * w / glConfig.vidWidth;
	rect.y1 = rect.y1 * h / glConfig.vidHeight;
	rect.x2 = rect.x2 * w / glConfig.vidWidth + 1;
	rect.y2 = rect.y2 * h / glConfig.vidHeight + 1;
	// blur must not catch uncleared texels outside scissor
	// so this value must not be lower than blur radius in shader
	int margin = int( 5 + 3.0f * r_volumetricBlur.GetFloat() );
	rect.x1 = idMath::Imax( rect.x1 - margin, 0 );
	rect.y1 = idMath::Imax( rect.y1 - margin, 0 );
	rect.x2 = idMath::Imin( rect.x2 + margin, w - 1 );
	rect.y2 = idMath::Imin( rect.y2 + margin, h - 1 );
	if ( r_useScissor.GetBool() ) {
		// apply blur in real FBO pixels
		GL_ScissorAbsolute(
			rect.x1,
			rect.y1,
			rect.x2 - rect.x1 + 1,
			rect.y2 - rect.y1 + 1
		);
	} else {
		GL_ScissorRelative(0, 0, 1, 1);
	}
}

void VolumetricStage::RenderFrustum(GLSLProgram *shader) {
	// note: in principle, we can often do scissored full-screen pass (for blur & compositing)
	// instead, we render frustum with backfaces to get benefits:
	//   1) less fragments to process (only frustum pixels instead of scissor rect)
	//   2) retain sharp edge of light volume boundary (not perfectly due to lower resolution)
	//   3) can use frustum exit distance for clamping depth texture

	// set modelview / projection
	shader->GetUniformGroup<TransformUniforms>()->Set( &viewDef->worldSpace );

	drawSurf_t ds = { 0 };
	ds.space = &viewDef->worldSpace;
	ds.frontendGeo = viewLight->frustumTris;
	ds.numIndexes = viewLight->frustumTris->numIndexes;
	ds.indexCache = viewLight->frustumTris->indexCache;
	ds.ambientCache = viewLight->frustumTris->ambientCache;
	ds.scissorRect = viewDef->scissor;

	RB_T_RenderTriangleSurface( &ds );
}

void VolumetricStage::RenderRaymarching(const TemporaryData &data) {
	TRACE_GL_SCOPE( "Raymarching" );

	raymarchingShader->Activate();
	GL_CheckErrors();

	struct RaymarchingUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( RaymarchingUniforms )

		DEFINE_UNIFORM( vec3, viewOrigin );
		DEFINE_UNIFORM( vec3, lightOrigin );
		DEFINE_UNIFORM( int, sampleCount );
		DEFINE_UNIFORM( int, randomize );
		DEFINE_UNIFORM( int, alphaMode );
		DEFINE_UNIFORM( vec4, lightColor );
		DEFINE_UNIFORM( sampler, depthTexture );
		DEFINE_UNIFORM( float, dust )
		DEFINE_UNIFORM( vec2, invDestResolution );
		DEFINE_UNIFORM( vec2, subpixelShift );

		// light frustum and projection
		DEFINE_UNIFORM( sampler, lightProjectionTexture );
		DEFINE_UNIFORM( sampler, lightFalloffTexture );
		DEFINE_UNIFORM( mat4, lightProject );
		DEFINE_UNIFORM( vec4, lightFrustum );
		DEFINE_UNIFORM( vec4, lightTextureMatrix );

		// shadow mapping
		DEFINE_UNIFORM( int, shadows );
		DEFINE_UNIFORM( vec4, shadowRect );
		DEFINE_UNIFORM( sampler, shadowMap );
	};
	RaymarchingUniforms *uniforms = raymarchingShader->GetUniformGroup<RaymarchingUniforms>();

	uniforms->alphaMode.Set( data.alphaMode );
	uniforms->dust.Set( data.dust );
	uniforms->sampleCount.Set( data.samples );
	uniforms->shadows.Set( data.useShadows );
	uniforms->randomize.Set( r_volumetricDither.GetInteger() );

	uniforms->invDestResolution.Set( 1.0f / frameBuffers->activeDrawFbo->Width(), 1.0f / frameBuffers->activeDrawFbo->Height() );
	uniforms->subpixelShift.Set( subpixelShift );

	uniforms->viewOrigin.Set( viewDef->renderView.vieworg );
	uniforms->lightProject.Set( viewLight->lightProject[0].ToFloatPtr() );
	uniforms->lightFrustum.SetArray( 6, viewLight->lightDef->frustum[0].ToFloatPtr() );
	uniforms->lightOrigin.Set( viewLight->globalLightOrigin );
	uniforms->lightColor.Set( data.lightColor );
	uniforms->lightTextureMatrix.SetArray( 2, data.lightTexRows[0].ToFloatPtr() );

	if ( data.useShadows ) {
		const renderCrop_t &page = viewLight->shadowMapPage;
		idVec4 v( page.x, page.y, 0, page.width );
		v /= 6 * r_shadowMapSize.GetFloat();
		uniforms->shadowRect.Set( v );
	}

	uniforms->lightProjectionTexture.Set( 2 );
	GL_SelectTexture( 2 );
	data.projectionImage->Bind();

	uniforms->lightFalloffTexture.Set( 1 );
	GL_SelectTexture( 1 );
	data.falloffImage->Bind();

	uniforms->depthTexture.Set( 3 );
	GL_SelectTexture( 3 );
	globalImages->currentDepthImage->Bind();

	uniforms->shadowMap.Set( 4 );
	GL_SelectTexture( 4 );
	globalImages->shadowAtlas->Bind();

	RenderFrustum( raymarchingShader );
}

void VolumetricStage::Blur(idImage *sourceTexture, bool vertical) {
	TRACE_GL_SCOPE( "Blur" );

	blurShader->Activate();
	GL_CheckErrors();

	struct BlurUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( BlurUniforms )

		DEFINE_UNIFORM( sampler, sourceTexture );
		DEFINE_UNIFORM( sampler, depthTexture );
		DEFINE_UNIFORM( int, vertical );
		DEFINE_UNIFORM( vec2, invDestResolution );
		DEFINE_UNIFORM( vec2, subpixelShift );
		DEFINE_UNIFORM( float, blurSigma );
	};
	BlurUniforms *uniforms = blurShader->GetUniformGroup<BlurUniforms>();

	uniforms->sourceTexture.Set( 2 );
	GL_SelectTexture( 2 );
	sourceTexture->Bind();

	uniforms->depthTexture.Set( 0 );
	GL_SelectTexture( 0 );
	globalImages->currentDepthImage->Bind();

	uniforms->invDestResolution.Set( 1.0f / frameBuffers->activeDrawFbo->Width(), 1.0f / frameBuffers->activeDrawFbo->Height() );
	uniforms->subpixelShift.Set( subpixelShift );
	uniforms->vertical.Set( vertical );
	uniforms->blurSigma.Set( r_volumetricBlur.GetFloat() );

	RenderFrustum( blurShader );
}

void VolumetricStage::PerformCompositing(idImage *colorTexture) {
	TRACE_GL_SCOPE( "Compositing" );

	compositingShader->Activate();
	GL_CheckErrors();

	struct CompositingUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( CompositingUniforms )

		DEFINE_UNIFORM( sampler, resultTexture );
		DEFINE_UNIFORM( sampler, depthTexture );
		DEFINE_UNIFORM( vec2, invDestResolution );
		DEFINE_UNIFORM( vec2, subpixelShift );
		DEFINE_UNIFORM( int, upsampling );
	};
	CompositingUniforms *uniforms = compositingShader->GetUniformGroup<CompositingUniforms>();

	uniforms->resultTexture.Set( 2 );
	GL_SelectTexture( 2 );
	colorTexture->Bind();

	uniforms->depthTexture.Set( 0 );
	GL_SelectTexture( 0 );
	globalImages->currentDepthImage->Bind();

	uniforms->invDestResolution.Set( 1.0f / frameBuffers->activeDrawFbo->Width(), 1.0f / frameBuffers->activeDrawFbo->Height() );
	uniforms->subpixelShift.Set( subpixelShift );
	uniforms->upsampling.Set( r_volumetricLowres.GetInteger() > 0 );

	RenderFrustum( compositingShader );
}
