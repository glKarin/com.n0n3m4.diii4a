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

class GLSLProgram;

class SurfacePassesStage {
public:
	void Init();
	void Shutdown();

	void DrawSurfaces( const viewDef_t *viewDef, const drawSurf_t **drawSurfs, int numDrawSurfs );

	// if no surface uses _currentRender, then we can skip frame color copy
	bool NeedCurrentRenderTexture( const viewDef_t *viewDef, const drawSurf_t **drawSurfs, int numDrawSurfs );

private:
	enum StageType {
		ST_SIMPLE_TEXTURE,	// "old stage", cubemap sky, screen-textured
		ST_ENVIRONMENT,
		ST_SOFT_PARTICLE,
		ST_CUSTOM_SHADER,	// "new stage"
	};

	GLSLProgram *simpleTextureShader = nullptr;
	GLSLProgram *softParticleShader = nullptr;
	GLSLProgram *environmentShader = nullptr;

	const viewDef_t *viewDef = nullptr;

	bool ShouldDrawSurf( const drawSurf_t *drawSurf ) const;
	void DrawSurf( const drawSurf_t *drawSurf );
	bool ShouldDrawStage( const drawSurf_t *drawSurf, const shaderStage_t *pStage ) const;
	void DrawStage( const drawSurf_t *drawSurf, const shaderStage_t *pStage );

	static StageType ChooseType( const drawSurf_t *drawSurf, const shaderStage_t *pStage );

	void DrawSimpleTexture( const drawSurf_t *drawSurf, const shaderStage_t *pStage );
	void DrawEnvironment( const drawSurf_t *drawSurf, const shaderStage_t *pStage );
	void DrawSoftParticle( const drawSurf_t *drawSurf, const shaderStage_t *pStage );
	void DrawCustomShader( const drawSurf_t *drawSurf, const shaderStage_t *pStage );

	void BindVariableStageImage( const textureStage_t *texture, const float *regs );
};
