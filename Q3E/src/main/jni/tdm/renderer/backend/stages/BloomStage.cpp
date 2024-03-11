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

#pragma hdrstop

#include "renderer/backend/stages/BloomStage.h"
#include "renderer/resources/Image.h"
#include "renderer/tr_local.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/GLSLUniforms.h"
#include "renderer/backend/FrameBufferManager.h"
#include "renderer/backend/FrameBuffer.h"

idCVar r_bloom("r_bloom", "0", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE, "Enable Bloom effect");
idCVar r_bloom_threshold("r_bloom_threshold", "0.7", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Brightness threshold for Bloom effect");
idCVar r_bloom_threshold_falloff("r_bloom_threshold_falloff", "8", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Exponential factor with which values below the brightness threshold fall off");
idCVar r_bloom_detailblend("r_bloom_detailblend", "0.5", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Blend factor for mixing detail into the blurred Bloom result");
idCVar r_bloom_weight("r_bloom_weight", "0.3", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Multiplicative weight factor for adding Bloom to the final image");
idCVar r_bloom_downsample_limit("r_bloom_downsample_limit", "128", CVAR_INTEGER | CVAR_RENDERER | CVAR_ARCHIVE, "Downsample render image until vertical resolution approximately reaches this value");
idCVar r_bloom_blursteps("r_bloom_blursteps", "2", CVAR_INTEGER | CVAR_RENDERER | CVAR_ARCHIVE, "Number of blur steps to perform after downsampling");

extern idCVar r_fboResolution;

BloomStage bloomImpl;
BloomStage *bloom = &bloomImpl;

const int BloomStage::MAX_DOWNSAMPLING_STEPS;

namespace {
	struct BloomDownsampleUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(BloomDownsampleUniforms)

		DEFINE_UNIFORM(sampler, sourceTexture)
		DEFINE_UNIFORM(float, brightnessThreshold)
		DEFINE_UNIFORM(float, thresholdFalloff)
	};

	struct BloomUpsampleUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(BloomUpsampleUniforms)

		DEFINE_UNIFORM(sampler, blurredTexture)
		DEFINE_UNIFORM(sampler, detailTexture)
		DEFINE_UNIFORM(float, detailBlendWeight)
	};

	struct BloomBlurUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(BloomBlurUniforms)

		DEFINE_UNIFORM(sampler, source)
		DEFINE_UNIFORM(vec2, axis)
	};

	struct BloomApplyUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( BloomApplyUniforms )

		DEFINE_UNIFORM(sampler, texture)
		DEFINE_UNIFORM(sampler, bloomTex)
		DEFINE_UNIFORM(float, bloomWeight)
	};

	void LoadBloomDownsampleShader(GLSLProgram *downsampleShader) {
		downsampleShader->LoadFromFiles( "fullscreen_tri.vert.glsl", "bloom_downsample.frag.glsl" );
		BloomDownsampleUniforms *uniforms = downsampleShader->GetUniformGroup<BloomDownsampleUniforms>();
		uniforms->sourceTexture.Set(0);
	}

	void LoadBloomDownsampleWithBrightPassShader(GLSLProgram *downsampleShader) {
		idHashMapDict defines;
		defines.Set( "BLOOM_BRIGHTPASS", "1" );
		downsampleShader->LoadFromFiles( "fullscreen_tri.vert.glsl", "bloom_downsample.frag.glsl", defines );
		BloomDownsampleUniforms *uniforms = downsampleShader->GetUniformGroup<BloomDownsampleUniforms>();
		uniforms->sourceTexture.Set(0);
	}

	void LoadBloomUpsampleShader(GLSLProgram *upsampleShader) {
		upsampleShader->LoadFromFiles( "fullscreen_tri.vert.glsl", "bloom_upsample.frag.glsl" );
		BloomUpsampleUniforms *uniforms = upsampleShader->GetUniformGroup<BloomUpsampleUniforms>();
		uniforms->blurredTexture.Set(0);
		uniforms->detailTexture.Set(1);
	}

	void LoadBloomApplyShader(GLSLProgram *applyShader) {
		applyShader->LoadFromFiles( "fullscreen_tri.vert.glsl", "bloom_apply.frag.glsl" );
		BloomApplyUniforms *uniforms = applyShader->GetUniformGroup<BloomApplyUniforms>();
		uniforms->texture.Set(0);
		uniforms->bloomTex.Set(1);
	}

	int CalculateNumDownsamplingSteps(int imageHeight) {
		int numSteps = 0;
		int downsampleLimit = r_bloom_downsample_limit.GetInteger();
		while( (imageHeight >> numSteps) - downsampleLimit > downsampleLimit - (imageHeight >> (numSteps + 1)) ) {
			++numSteps;
		}
		return std::min(numSteps + 1, BloomStage::MAX_DOWNSAMPLING_STEPS);
	}

	void CreateBloomFBO(FrameBuffer *fbo, idImageScratch *image, int step) {
		int curWidth = frameBuffers->renderWidth >> (step+1);
		int curHeight = frameBuffers->renderHeight >> (step+1);
		fbo->Init( curWidth, curHeight );
		image->GenerateAttachment( curWidth, curHeight, GL_RGBA16F, GL_LINEAR );
		fbo->AddColorRenderTexture( 0, image );
	}
}

void BloomStage::Init() {
	for (int i = 0; i < MAX_DOWNSAMPLING_STEPS; ++i) {
		bloomDownSamplers[i] = globalImages->ImageScratch( idStr("Bloom Downsampling ") + i );		
		bloomUpSamplers[i] = globalImages->ImageScratch( idStr("Bloom Upsampling ") + i );		
		downsampleFBOs[i] = frameBuffers->CreateFromGenerator( idStr("bloom_downsample") + i, [this, i](FrameBuffer *fbo) { CreateBloomFBO( fbo, bloomDownSamplers[i], i ); } );
		upsampleFBOs[i] = frameBuffers->CreateFromGenerator( idStr("bloom_upsample") + i, [this, i](FrameBuffer *fbo) { CreateBloomFBO( fbo, bloomUpSamplers[i], i ); } );
	}
	frameBuffers->defaultFbo->Bind();

	downsampleShader = programManager->LoadFromGenerator("bloom_downsample", LoadBloomDownsampleShader);
	downsampleWithBrightPassShader = programManager->LoadFromGenerator("bloom_downsample_brightpass", LoadBloomDownsampleWithBrightPassShader);
	upsampleShader = programManager->LoadFromGenerator("bloom_upsample", LoadBloomUpsampleShader);
	applyShader = programManager->LoadFromGenerator("bloom_apply", LoadBloomApplyShader);
}

void BloomStage::Shutdown() {
	for (int i = 0; i < MAX_DOWNSAMPLING_STEPS; ++i) {
		if (downsampleFBOs[i] != nullptr) {
			downsampleFBOs[i]->Destroy();
			downsampleFBOs[i] = nullptr;
		}
		if (upsampleFBOs[i] != nullptr) {
			upsampleFBOs[i]->Destroy();
			upsampleFBOs[i] = nullptr;
		}
		if (bloomDownSamplers[i] != nullptr) {
			bloomDownSamplers[i]->PurgeImage();
			bloomDownSamplers[i] = nullptr;
		}
		if (bloomUpSamplers[i] != nullptr) {
			bloomUpSamplers[i]->PurgeImage();
			bloomUpSamplers[i] = nullptr;
		}
	}
}

void BloomStage::ComputeBloomFromRenderImage() {
	TRACE_GL_SCOPE("BloomStage");

	if (downsampleFBOs[0] == nullptr) {
		Init();
	}

	numDownsamplingSteps = CalculateNumDownsamplingSteps( bloomDownSamplers[0]->uploadHeight );

	qglClearColor(0, 0, 0, 0);

	qglDisable(GL_SCISSOR_TEST);
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_ALWAYS );
	// To get a soft, wide blur for the Bloom effect cheaply, we use the following approach:
	// 1. do a bright pass on the original render image and downsample it in several steps
	// 2. apply a Gaussian blur filter multiple times on the downsampled image
	// 3. scale it back up to half resolution
	Downsample();
	for( int i = 0; i < r_bloom_blursteps.GetInteger(); ++i ) {
		Blur();
	}
	Upsample();

	frameBuffers->currentRenderFbo->Bind();
	GL_ViewportRelative( 0, 0, 1, 1 );
	qglEnable(GL_SCISSOR_TEST);
}

void BloomStage::BindBloomTexture() {
	bloomUpSamplers[0]->Bind();
}

void BloomStage::ApplyBloom() {
	GL_State( GLS_DEPTHMASK );
	qglDisable( GL_DEPTH_TEST );

	applyShader->Activate();
	BloomApplyUniforms *uniforms = applyShader->GetUniformGroup<BloomApplyUniforms>();

	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();
	GL_SelectTexture( 1 );
	BindBloomTexture();
	uniforms->bloomWeight.Set( r_bloom_weight.GetFloat() );

	qglClear(GL_COLOR_BUFFER_BIT);
	RB_DrawFullScreenTri();

	qglEnable( GL_DEPTH_TEST );
}

void BloomStage::Downsample() {
	TRACE_GL_SCOPE( "BloomDownsampling" )

	// execute initial downsampling and bright pass on render image
	downsampleWithBrightPassShader->Activate();
	BloomDownsampleUniforms *uniforms = downsampleWithBrightPassShader->GetUniformGroup<BloomDownsampleUniforms>();
	downsampleFBOs[0]->Bind();
	GL_ViewportRelative( 0, 0, 1, 1 );
	uniforms->brightnessThreshold.Set( r_bloom_threshold.GetFloat() );
	uniforms->thresholdFalloff.Set( r_bloom_threshold_falloff.GetFloat() );
	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();
	qglClear(GL_COLOR_BUFFER_BIT);
	RB_DrawFullScreenTri();

	// generate additional downsampled mip levels
	bloomDownSamplers[0]->Bind();
	downsampleShader->Activate();
	uniforms = downsampleShader->GetUniformGroup<BloomDownsampleUniforms>();
	for (int i = 1; i < numDownsamplingSteps; ++i) {
		downsampleFBOs[i]->Bind();
		GL_ViewportRelative( 0, 0, 1, 1 );
		qglClear(GL_COLOR_BUFFER_BIT);
		RB_DrawFullScreenTri();
		bloomDownSamplers[i]->Bind();
	}
}

void BloomStage::Blur() {
	TRACE_GL_SCOPE("BloomBlur")

	int step = numDownsamplingSteps - 1;
	programManager->gaussianBlurShader->Activate();
	BloomBlurUniforms *uniforms = programManager->gaussianBlurShader->GetUniformGroup<BloomBlurUniforms>();

	// first horizontal Gaussian blur goes from downsampler[lowestMip] to upsampler[lowestMip]
	GL_SelectTexture( 0 );
	bloomDownSamplers[step]->Bind();
	uniforms->axis.Set( 1, 0 );
	upsampleFBOs[step]->Bind();
	GL_ViewportRelative( 0, 0, 1, 1 );
	qglClear(GL_COLOR_BUFFER_BIT);
	RB_DrawFullScreenTri();

	// second vertical Gaussian blur goes from upsampler[lowestMip] to downsampler[lowestMip]
	uniforms->axis.Set( 0, 1 );
	downsampleFBOs[step]->Bind();
	bloomUpSamplers[step]->Bind();
	qglClear(GL_COLOR_BUFFER_BIT);
	RB_DrawFullScreenTri();
}

void BloomStage::Upsample() {
	TRACE_GL_SCOPE( "BloomUpsampling" )

	if (numDownsamplingSteps <= 1)
		return;

	upsampleShader->Activate();
	BloomUpsampleUniforms *uniforms = upsampleShader->GetUniformGroup<BloomUpsampleUniforms>();
	float detailBlendWeight = 1.f - pow(1.f - r_bloom_detailblend.GetFloat(), 1.f/(numDownsamplingSteps - 1));
	uniforms->detailBlendWeight.Set(detailBlendWeight);

	// first upsampling step goes from downsampler[lowestMip] to upsampler[lowestMip-1]
	GL_SelectTexture( 0 );
	bloomDownSamplers[numDownsamplingSteps-1]->Bind();

	for (int i = numDownsamplingSteps - 2; i >= 0; --i) {
		GL_SelectTexture( 1 );
		bloomDownSamplers[i]->Bind();
		upsampleFBOs[i]->Bind();
		GL_ViewportRelative( 0, 0, 1, 1 );
		qglClear(GL_COLOR_BUFFER_BIT);
		RB_DrawFullScreenTri();
		// next upsampling steps go from upsampler[mip+1] to upsampler[mip]
		GL_SelectTexture( 0 );
		bloomUpSamplers[i]->Bind();
	}
}
