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
class TiledCustomMipmapStage;

enum DrawSurfaceList {
	DSL_GLOBAL,
	DSL_LOCAL,
	DSL_TRANSLUCENT,
};

class InteractionStage {
public:
	InteractionStage();

	void Init();
	void Shutdown();

	bool ShouldDrawLight( const viewLight_t *vLight ) const;

	void DrawInteractions( const viewDef_t *viewDef, const viewLight_t *vLight, DrawSurfaceList list, const TiledCustomMipmapStage *stencilShadowMipmaps );

private:
	struct Uniforms;

	GLSLProgram *stencilInteractionShader = nullptr;
	GLSLProgram *shadowMapInteractionShader = nullptr;
	GLSLProgram *ambientInteractionShader = nullptr;
	GLSLProgram *interactionShader = nullptr;

	Uniforms *uniforms = nullptr;
	idList<idVec2> poissonSamples;

	const viewDef_t *viewDef = nullptr;
	const viewLight_t *vLight = nullptr;

	void LoadInteractionShader(GLSLProgram *shader, const idStr &baseName);
	void BindShadowTexture( const TiledCustomMipmapStage *stencilShadowMipmaps );
	void ChooseInteractionProgram( const viewLight_t *vLight, bool translucent, int &lightFlags );
	void ProcessSingleSurface( const viewLight_t *vLight, const shaderStage_t *lightStage, const drawSurf_t *surf, int lightFlags );
	void PrepareDrawCommand( drawInteraction_t *inter, int lightFlags );
	void SetupLightProperties( drawInteraction_t *inter, const viewLight_t *vLight, const shaderStage_t *lightStage, const drawSurf_t *surf );

	void PreparePoissonSamples();
};
