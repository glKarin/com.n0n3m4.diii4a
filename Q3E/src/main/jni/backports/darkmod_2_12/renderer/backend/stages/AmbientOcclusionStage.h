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

extern idCVar r_ssao;

class FrameBuffer;

class AmbientOcclusionStage {
public:
	void Init();

	void Shutdown();

	void ComputeSSAOFromDepth();

	bool ShouldEnableForCurrentView() const;

	void BindSSAOTexture(int index);

	void ShowSSAO();

	static const int MAX_DEPTH_MIPS = 5;
private:
	FrameBuffer *ssaoFBO = nullptr;
	FrameBuffer *ssaoBlurFBO = nullptr;
	FrameBuffer *depthMipFBOs[MAX_DEPTH_MIPS + 1] = { nullptr };
	idImageScratch *viewspaceDepth = nullptr;
	idImageScratch *ssaoResult = nullptr;
	idImageScratch *ssaoBlurred = nullptr;
	GLSLProgram *ssaoShader = nullptr;
	GLSLProgram *ssaoBlurShader = nullptr;
	GLSLProgram *depthShader = nullptr;
	GLSLProgram *depthMipShader = nullptr;
	GLSLProgram *showSSAOShader = nullptr;

	void PrepareDepthPass();
	void SSAOPass();
	void BlurPass();

	void SetQualityLevelUniforms();
};

extern AmbientOcclusionStage *ambientOcclusion;
