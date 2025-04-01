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

#include "renderer/tr_local.h"

class FrameBuffer;

class VolumetricStage {
public:
	VolumetricStage();
	~VolumetricStage();

	void Init();
	void Shutdown();

	bool RenderLight(const viewDef_t *viewDef, const viewLight_t *viewLight);
	void RenderAll(const viewDef_t *viewDef);

private:
	struct TemporaryData;

	void PrepareRaymarching(TemporaryData &data);
	void RenderRaymarching(const TemporaryData &data);
	void Blur(idImage *sourceTexture, bool vertical);
	void PerformCompositing(idImage *colorTexture);
	void SetScissor();
	void RenderFrustum(GLSLProgram *shader);

	idImageScratch *workImage[2] = {nullptr, nullptr};
	FrameBuffer *workFBO[2] = {nullptr, nullptr};
	GLSLProgram *raymarchingShader = nullptr;
	GLSLProgram *blurShader = nullptr;
	GLSLProgram *compositingShader = nullptr;

	// temporary data
	mutable const viewDef_t *viewDef;
	mutable const viewLight_t *viewLight;
	// whenever we sample depth texture at low-res pixel (aka "block"), we shift texcoord from block center by this amount
	// since lowres texture is 2/4/8 times smaller, block center is always exactly between 2x2 high-res pixels
	// depth is sampled with nearest filter, so one of them is chosen depending on roundoff errors
	// however, we want to ensure that the same depth texel is used for block in all shaders, otherwise depth-aware blur/upsampling will look bad
	// adding shift by half of high-res texel means that upper-right depth texel our of 2x2 square is always chosen
	mutable idVec2 subpixelShift;
};

extern VolumetricStage *volumetric;
