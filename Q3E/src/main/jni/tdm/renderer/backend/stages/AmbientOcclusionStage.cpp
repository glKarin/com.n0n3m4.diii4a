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

#include "renderer/backend/stages/AmbientOcclusionStage.h"
#include "renderer/resources/Image.h"
#include "renderer/tr_local.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/GLSLUniforms.h"
#include "renderer/backend/FrameBufferManager.h"
#include "renderer/backend/FrameBuffer.h"

idCVar r_ssao("r_ssao", "0", CVAR_INTEGER | CVAR_RENDERER | CVAR_ARCHIVE, "Screen space ambient occlusion: 0 - off, 1 - low, 2 - medium, 3 - high");
idCVar r_ssao_radius("r_ssao_radius", "32", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
	"View space sample radius - larger values provide a softer, spread effect, but risk causing unwanted halo shadows around objects");
idCVar r_ssao_bias("r_ssao_bias", "0.05", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
	"Min depth difference to count for occlusion, used to avoid some acne effects");
idCVar r_ssao_intensity("r_ssao_intensity", "1.0", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
	"SSAO intensity factor, the higher the value, the stronger the effect");
idCVar r_ssao_base("r_ssao_base", "0.1", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
	"Minimum baseline visibility below which AO cannot drop");
idCVar r_ssao_edgesharpness("r_ssao_edgesharpness", "1", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Edge sharpness in SSAO blur");

extern idCVar r_fboResolution;

AmbientOcclusionStage ambientOcclusionImpl;
AmbientOcclusionStage *ambientOcclusion = &ambientOcclusionImpl;

namespace {
	struct AOUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(AOUniforms)

		DEFINE_UNIFORM(sampler, depthTexture)
		DEFINE_UNIFORM(float, sampleRadius)
		DEFINE_UNIFORM(float, depthBias)
		DEFINE_UNIFORM(float, baseValue)
		DEFINE_UNIFORM(int, numSamples)
		DEFINE_UNIFORM(int, numSpiralTurns)
		DEFINE_UNIFORM(float, intensityDivR6)
		DEFINE_UNIFORM(int, maxMipLevel)
	};

	struct BlurUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(BlurUniforms)

		DEFINE_UNIFORM(sampler, source)
		DEFINE_UNIFORM(vec2, axis)
		DEFINE_UNIFORM(float, edgeSharpness)
	};

	struct DepthMipUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(DepthMipUniforms)

		DEFINE_UNIFORM(sampler, depth)
		DEFINE_UNIFORM(int, previousMipLevel)
	};

	void LoadSSAOShader(GLSLProgram *ssaoShader) {
		ssaoShader->LoadFromFiles( "ssao.vert.glsl", "ssao.frag.glsl" );
		AOUniforms *uniforms = ssaoShader->GetUniformGroup<AOUniforms>();
		uniforms->depthTexture.Set(0);
	}

	void LoadSSAOBlurShader(GLSLProgram *blurShader) {
		blurShader->LoadFromFiles( "fullscreen_tri.vert.glsl", "ssao_blur.frag.glsl" );
		BlurUniforms *uniforms = blurShader->GetUniformGroup<BlurUniforms>();
		uniforms->source.Set(0);
	}

	void CreateSSAOColorFBO(FrameBuffer *fbo, idImageScratch *color) {
		fbo->Init( frameBuffers->renderWidth, frameBuffers->renderHeight );
#ifdef __ANDROID__ //karin: only GL_RGBA if texture
		color->GenerateAttachment( frameBuffers->renderWidth, frameBuffers->renderHeight, GL_RGBA, GL_LINEAR, GL_CLAMP_TO_EDGE );
#else
        color->GenerateAttachment( frameBuffers->renderWidth, frameBuffers->renderHeight, GL_RGBA8, GL_LINEAR, GL_CLAMP_TO_EDGE );
#endif
		fbo->AddColorRenderTexture( 0, color );
	}

	void CreateViewspaceDepthFBO(FrameBuffer *fbo, idImageScratch *image, int mipLevel) {
		int w = frameBuffers->renderWidth >> mipLevel;
		int h = frameBuffers->renderHeight >> mipLevel;
		image->GenerateAttachment( w, h, GL_R32F, GL_NEAREST, GL_MIRRORED_REPEAT, mipLevel );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		fbo->Init( w, h );
		fbo->AddColorRenderTexture( 0, image, mipLevel );
	}
}

void AmbientOcclusionStage::Init() {
	ssaoResult = globalImages->ImageScratch("SSAO ColorBuffer");
	ssaoBlurred = globalImages->ImageScratch("SSAO Blurred");
	viewspaceDepth = globalImages->ImageScratch("SSAO Depth");

	ssaoFBO = frameBuffers->CreateFromGenerator( "ssao_color", [this](FrameBuffer *fbo) { CreateSSAOColorFBO( fbo, ssaoResult ); } );
	ssaoBlurFBO = frameBuffers->CreateFromGenerator( "ssao_blurred", [this](FrameBuffer *fbo) { CreateSSAOColorFBO( fbo, ssaoBlurred ); } );
	for (int i = 0; i <= MAX_DEPTH_MIPS; ++i) {
		depthMipFBOs[i] = frameBuffers->CreateFromGenerator( idStr("ssao_depth_") + i, [this, i](FrameBuffer *fbo) { CreateViewspaceDepthFBO( fbo, viewspaceDepth, i ); } );
	}
	frameBuffers->defaultFbo->Bind();

	ssaoShader = programManager->LoadFromGenerator("ssao", LoadSSAOShader);
	ssaoBlurShader = programManager->LoadFromGenerator("ssao_blur", LoadSSAOBlurShader);
	depthShader = programManager->LoadFromFiles("ssao_depth", "fullscreen_tri.vert.glsl", "ssao_depth.frag.glsl");
	depthMipShader = programManager->LoadFromFiles("ssao_depth_mip", "fullscreen_tri.vert.glsl", "ssao_depthmip.frag.glsl");
	showSSAOShader = programManager->LoadFromFiles("ssao_show", "fullscreen_tri.vert.glsl", "ssao_show.frag.glsl");
}

void AmbientOcclusionStage::Shutdown() {
	if (ssaoFBO != nullptr) {
		ssaoFBO->Destroy();
		ssaoFBO = nullptr;
	}
	if (ssaoBlurFBO != nullptr) {
		ssaoBlurFBO->Destroy();
		ssaoBlurFBO = nullptr;
	}
	for (int i = 0; i <= MAX_DEPTH_MIPS; ++i) {
		if (depthMipFBOs[i] != nullptr) {
			depthMipFBOs[i]->Destroy();
			depthMipFBOs[i] = nullptr;
		}
	}
	if (viewspaceDepth != nullptr) {
		viewspaceDepth->PurgeImage();
		viewspaceDepth = nullptr;
	}
	if (ssaoResult != nullptr) {
		ssaoResult->PurgeImage();
		ssaoResult = nullptr;
	}
	if (ssaoBlurred != nullptr) {
		ssaoBlurred->PurgeImage();
		ssaoBlurred = nullptr;
	}
}

extern GLuint fboPrimary;
extern bool primaryOn;

void AmbientOcclusionStage::ComputeSSAOFromDepth() {
	TRACE_GL_SCOPE("AmbientOcclusionStage");

	if (ssaoFBO == nullptr) {
		Init();
	}

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK );
	PrepareDepthPass();
	SSAOPass();
	BlurPass();

	frameBuffers->currentRenderFbo->Bind();
}

void AmbientOcclusionStage::SSAOPass() {
	TRACE_GL_SCOPE("SSAOPass");

	ssaoFBO->Bind();
	qglClearColor(1, 1, 1, 1);
	qglClear(GL_COLOR_BUFFER_BIT);
	GL_SelectTexture(0);
	viewspaceDepth->Bind();

	ssaoShader->Activate();
	SetQualityLevelUniforms();
	RB_DrawFullScreenTri();
}

void AmbientOcclusionStage::BlurPass() {
	TRACE_GL_SCOPE("BlurPass");

	ssaoBlurShader->Activate();
	BlurUniforms *uniforms = ssaoBlurShader->GetUniformGroup<BlurUniforms>();
	uniforms->edgeSharpness.Set(r_ssao_edgesharpness.GetFloat());
	uniforms->source.Set(0);
	uniforms->axis.Set(1, 0);

	// first horizontal pass
	ssaoBlurFBO->Bind();
	qglClearColor(1, 1, 1, 1);
	qglClear(GL_COLOR_BUFFER_BIT);
	GL_SelectTexture(0);
	ssaoResult->Bind();
	RB_DrawFullScreenTri();

	// second vertical pass
	uniforms->axis.Set(0, 1);
	ssaoFBO->Bind();
	qglClear(GL_COLOR_BUFFER_BIT);
	ssaoBlurred->Bind();
	RB_DrawFullScreenTri();
}

void AmbientOcclusionStage::BindSSAOTexture(int index) {
	GL_SelectTexture(index);
	if (ShouldEnableForCurrentView()) {
		ssaoResult->Bind();
	}
	else {
		globalImages->whiteImage->Bind();
	}
}

bool AmbientOcclusionStage::ShouldEnableForCurrentView() const {
	return r_ssao.GetBool() && !backEnd.viewDef->IsLightGem() && !backEnd.viewDef->isSubview && !backEnd.viewDef->renderWorld->mapName.IsEmpty();
}

void AmbientOcclusionStage::PrepareDepthPass() {
	TRACE_GL_SCOPE("PrepareDepthPass");

	depthMipFBOs[0]->Bind();
	GL_ScissorRelative( 0, 0, 1, 1 );
	qglClear(GL_COLOR_BUFFER_BIT);
	GL_SelectTexture(0);
	globalImages->currentDepthImage->Bind();

	depthShader->Activate();
	RB_DrawFullScreenTri();

	if (r_ssao.GetInteger() > 1) {
		TRACE_GL_SCOPE("DepthMips");
		// generate mip levels - used by the AO shader for distant samples to ensure we hit the texture cache as much as possible
		depthMipShader->Activate();
		DepthMipUniforms *uniforms = depthMipShader->GetUniformGroup<DepthMipUniforms>();
		uniforms->depth.Set(0);
		viewspaceDepth->Bind();
		for (int i = 1; i <= MAX_DEPTH_MIPS; ++i) {
			depthMipFBOs[i]->Bind();
			qglClear(GL_COLOR_BUFFER_BIT);
			uniforms->previousMipLevel.Set(i - 1);
			RB_DrawFullScreenTri();
		}
	}
}

void AmbientOcclusionStage::ShowSSAO() {
	showSSAOShader->Activate();
	BindSSAOTexture(0);
	RB_DrawFullScreenTri();
}

void AmbientOcclusionStage::SetQualityLevelUniforms() {
	AOUniforms *uniforms = ssaoShader->GetUniformGroup<AOUniforms>();
	uniforms->depthBias.Set(r_ssao_bias.GetFloat());
	uniforms->baseValue.Set(r_ssao_base.GetFloat());
	float sampleRadius;
	switch (r_ssao.GetInteger()) {
	case 1:
		 uniforms->maxMipLevel.Set(0);
		 uniforms->numSpiralTurns.Set(5);
		 uniforms->numSamples.Set(7);
		 sampleRadius = 0.5f * r_ssao_radius.GetFloat();
		 break;
	case 2:
		 uniforms->maxMipLevel.Set(MAX_DEPTH_MIPS);
		 uniforms->numSpiralTurns.Set(7);
		 uniforms->numSamples.Set(12);
		 sampleRadius = r_ssao_radius.GetFloat();
		 break;
	case 3:
	default:
		 uniforms->maxMipLevel.Set(MAX_DEPTH_MIPS);
		 uniforms->numSpiralTurns.Set(7);
		 uniforms->numSamples.Set(24);
		 sampleRadius = 2.0f * r_ssao_radius.GetFloat();
		 break;
	}
	uniforms->intensityDivR6.Set(r_ssao_intensity.GetFloat() / pow(sampleRadius * 0.02309f, 6));
	uniforms->sampleRadius.Set(sampleRadius);
}
