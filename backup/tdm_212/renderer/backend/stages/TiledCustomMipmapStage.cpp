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

#include "renderer/backend/stages/TiledCustomMipmapStage.h"
#include "renderer/resources/Image.h"
#include "renderer/tr_local.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/GLSLUniforms.h"
#include "renderer/backend/FrameBufferManager.h"
#include "renderer/backend/FrameBuffer.h"

TiledCustomMipmapStage tiledCustomMipmapImpl;
TiledCustomMipmapStage *tiledCustomMipmap = &tiledCustomMipmapImpl;

namespace {
	struct MipmapUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(MipmapUniforms)

		DEFINE_UNIFORM(sampler, sourceTexture)
		DEFINE_UNIFORM(sampler, mipmapTexture)
		DEFINE_UNIFORM(int, level)
		DEFINE_UNIFORM(ivec4, clampRegion)
	};
}

static inline int DivDown(int size, int levels) {
	return ( size >> levels );
}
static inline int DivUp(int size, int levels) {
	return ( ( (size - 1) >> levels ) + 1 );
}

TiledCustomMipmapStage::~TiledCustomMipmapStage() {}
TiledCustomMipmapStage::TiledCustomMipmapStage() {
	scissor.Clear();
}

void TiledCustomMipmapStage::SetName(const char *name) {
	this->name = name;
}

void TiledCustomMipmapStage::Init(MipmapMode mode, GLenum imageFormat, int width, int height, int maxLevel, int skipLevels) {
	assert(maxLevel <= MAX_LEVEL);
	Shutdown();

	width = DivUp(width, maxLevel) << maxLevel;
	height = DivUp(height, maxLevel) << maxLevel;

	this->mode = mode;
	this->maxLevel = maxLevel;
	this->skipLevels = skipLevels;
	this->width = width;
	this->height = height;

	mipmapImage = globalImages->ImageScratch( idStr("Custom Tiled Mipmap ") + name );
	for (int level = 0; level <= maxLevel; level++) {
		if (level < skipLevels)
			continue;

		int levelWidth = width >> level;
		int levelHeight = height >> level;
		int textureLevel = level - skipLevels;

		mipmapFBO[level] = frameBuffers->CreateFromGenerator( idStr("custom_tiled_downsample") + level,
			[this, textureLevel, imageFormat, levelWidth, levelHeight](FrameBuffer *fbo) {
				mipmapImage->GenerateAttachment( levelWidth, levelHeight, imageFormat, GL_LINEAR, GL_CLAMP_TO_EDGE, textureLevel );
				qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
				fbo->Init( levelWidth, levelHeight );
				fbo->AddColorRenderTexture( 0, mipmapImage, textureLevel );
			}
		);
	}
	frameBuffers->defaultFbo->Bind();

	firstShader = programManager->LoadFromGenerator("tiled_mipmap_first", [this](GLSLProgram *shader) {
		LoadShader(shader, true);
	});
	subsequentShader = programManager->LoadFromGenerator("tiled_mipmap_subsequent", [this](GLSLProgram *shader) {
		LoadShader(shader, false);
	});
}

void TiledCustomMipmapStage::LoadShader(GLSLProgram *shader, bool first) const {
	idHashMapDict defines;
	defines["MIPMAP_MODE"] = idStr(int(mode));
	defines["MIPMAP_FIRST"] = first ? "1" : "0";
	defines["MIPMAP_SKIP"] = idStr(skipLevels);
	shader->LoadFromFiles( "fullscreen_tri.vert.glsl", "tiled_custom_mipmap.frag.glsl", defines );
}

void TiledCustomMipmapStage::Shutdown() {
	for (int i = 0; i <= MAX_LEVEL; ++i) {
		if (mipmapFBO[i] != nullptr) {
			mipmapFBO[i]->Destroy();
			mipmapFBO[i] = nullptr;
		}
	}
	if (mipmapImage != nullptr) {
		mipmapImage->PurgeImage();
		mipmapImage = nullptr;
	}
	scissor.Clear();
}

void TiledCustomMipmapStage::FillFrom(idImage *image, int x, int y, int w, int h) {
	TRACE_GL_SCOPE("TiledCustomMipmap");

	// cannot use for images of larger size than requested during Init
	assert(image->uploadWidth <= width);
	assert(image->uploadHeight <= height);

	// clamp scissor rectangle by image bounds and save it
	scissor.x1 = idMath::Imax(x, 0);
	scissor.y1 = idMath::Imax(y, 0);
	scissor.x2 = idMath::Imin(x + w, image->uploadWidth - 1);
	scissor.y2 = idMath::Imin(y + h, image->uploadHeight - 1);
	scissor.zmin = 0.0f;	// not used
	scissor.zmax = 1.0f;	// not used

	GL_State( GLS_DEPTHFUNC_ALWAYS );

	// bind images
	GL_SelectTexture(0);
	mipmapImage->Bind();
	GL_SelectTexture(1);
	image->Bind();

	// full viewport even if we render subview in reduced viewport
	// note that we take scissor into account explicitly
	GL_ViewportRelative( 0, 0, 1, 1 );

	for (int level = skipLevels; level <= maxLevel; level++) {
		// we are going to fill current level
		mipmapFBO[level]->BindDraw();

		MipmapUniforms *uniforms;
		if (level == skipLevels) {
			// fetch source from caller-specified image
			// either by copying image, or by downsampling it
			firstShader->Activate();
			uniforms = firstShader->GetUniformGroup<MipmapUniforms>();
			// set source texture (image)
			uniforms->sourceTexture.Set(1);
		}
		else {
			// fetch source from previous LOD level of our mipmap image
			subsequentShader->Activate();
			uniforms = subsequentShader->GetUniformGroup<MipmapUniforms>();
			// set source texture (mipmapImage)
			uniforms->mipmapTexture.Set(0);
		}

		// size of destination and source scissors
		idScreenRect dstRect = GetScissorAtLevel(level);
		idScreenRect srcRect = GetScissorAtLevel(level == skipLevels ? 0 : level - 1);

		uniforms->level.Set(level);
		uniforms->clampRegion.Set(srcRect.x1, srcRect.y1, srcRect.x2, srcRect.y2);

		// limit rendering to scissor
		// TODO: should we respect r_useScissor (disabled for screenshots) ?
		qglEnable(GL_SCISSOR_TEST);
		qglScissor(dstRect.x1, dstRect.y1, dstRect.GetWidth(), dstRect.GetHeight());
		// fullscreen pass
		RB_DrawFullScreenTri();
	}

	frameBuffers->currentRenderFbo->Bind();
}

idScreenRect TiledCustomMipmapStage::GetScissorAtLevel(int lodLevel) const {
	idScreenRect rect;
	rect.x1 = DivDown(scissor.x1, lodLevel);
	rect.y1 = DivDown(scissor.y1, lodLevel);
	rect.x2 = DivUp(scissor.x2 + 1, lodLevel) - 1;
	rect.y2 = DivUp(scissor.y2 + 1, lodLevel) - 1;
	rect.zmin = 0.0f;	// not used
	rect.zmax = 1.0f;	// not used
	return rect;
}

bool TiledCustomMipmapStage::IsFilled() const {
	return !scissor.IsEmpty();
}

void TiledCustomMipmapStage::BindMipmapTexture() const {
	if (mipmapImage)
		mipmapImage->Bind();
}
