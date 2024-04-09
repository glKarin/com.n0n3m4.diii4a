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

#pragma once

#include "renderer/tr_local.h"	// idScreenRect

class FrameBuffer;

/**
 * Divides image of fixed size into POT-sized tiles.
 * Computes mipmap levels with custom meaning for tiles: min/max, boolean, ...
 **/
class TiledCustomMipmapStage {
public:
	TiledCustomMipmapStage();
	~TiledCustomMipmapStage();

	enum MipmapMode {
		MM_INVALID = -1,
		MM_STENCIL_SHADOW = 0,	// stencil <= 128 = lit, maintain lit/unlit/partly-lit info
	};

	void SetName(const char *name);

	// warning: width and height are padded to numbers divisible by 2^maxLevel
	// if skipLevels > 0, then this number of first mipmaps is not created/copied
	void Init(MipmapMode mode, GLenum imageFormat, int width, int height, int maxLevel, int skipLevels = 0);
	void Shutdown();

	// x, y, w, h define scissor rectangle to process (default: full image)
	// only pixels within scissor are fetched from source texture
	// if tile is partially outside scissor/image, then out-of-bounds values are obtained in GL_CLAMP_TO_EDGE style
	// note: changes viewport and scissor --- must be reset after call
	void FillFrom(idImage *image, int x = 0, int y = 0, int w = 1000000000, int h = 1000000000);

	bool IsFilled() const;
	int GetBaseLevel() const { return skipLevels; }
	int GetMaxLevel() const { return maxLevel; }
	// note: use textureLod(tc, k - skipLevels) in order to query k-th lod level
	void BindMipmapTexture() const;
	// note: you should never fetch mipmap texels outside scissor rectangle
	// since those values were not filled by last FillFrom !
	idScreenRect GetScissorAtLevel(int lodLevel) const;

private:
	void LoadShader(GLSLProgram *shader, bool first) const;

	static const int MAX_LEVEL = 16;

	// init parameters
	idStr name;
	MipmapMode mode = MM_INVALID;
	int width = -1;
	int height = -1;
	int maxLevel = -1;
	int skipLevels = -1;

	// current fill parameters
	idScreenRect scissor;

	// GL objects
	idImageScratch *mipmapImage = nullptr;
	FrameBuffer *mipmapFBO[MAX_LEVEL + 1] = { nullptr };
	GLSLProgram *firstShader = nullptr;
	GLSLProgram *subsequentShader = nullptr;
};

extern TiledCustomMipmapStage *tiledCustomMipmap;
