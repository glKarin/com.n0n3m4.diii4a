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

extern idCVar r_bloom;
extern idCVar r_bloom_weight;

class FrameBuffer;

class BloomStage {
public:
	void Init();

	void Shutdown();

	void ComputeBloomFromRenderImage();
	void BindBloomTexture();
	void ApplyBloom();

	static const int MAX_DOWNSAMPLING_STEPS = 16;
private:
	FrameBuffer *downsampleFBOs[MAX_DOWNSAMPLING_STEPS] = { nullptr };
	FrameBuffer *upsampleFBOs[MAX_DOWNSAMPLING_STEPS] = { nullptr };
	idImageScratch *bloomDownSamplers[MAX_DOWNSAMPLING_STEPS] = { nullptr };
	idImageScratch *bloomUpSamplers[MAX_DOWNSAMPLING_STEPS] = { nullptr };
	GLSLProgram *downsampleShader = nullptr;
	GLSLProgram *downsampleWithBrightPassShader = nullptr;
	GLSLProgram *upsampleShader = nullptr;
	GLSLProgram *applyShader = nullptr;
	int numDownsamplingSteps = 0;

	void Downsample();
	void Blur();
	void Upsample();
};

extern BloomStage *bloom;
