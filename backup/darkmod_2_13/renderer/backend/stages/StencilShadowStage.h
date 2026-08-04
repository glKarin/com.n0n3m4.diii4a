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
#include "renderer/backend/stages/TiledCustomMipmapStage.h"

class StencilShadowStage {
public:
	void Init();
	void Shutdown();

	void DrawStencilShadows( const viewDef_t *viewDef, const viewLight_t *vLight, const drawSurf_t *shadowSurfs );

	void FillStencilShadowMipmaps( const viewDef_t *viewDef, const idScreenRect &lightScissor );

	// soft shadows use blurred stencil buffer, which might tap slightly outside scissor rect
	// call this function for every light / shadow scissor before applying it
	idScreenRect ExpandScissorRectForSoftShadows( const viewDef_t *viewDef, const idScreenRect &scissor ) const;

private:
	struct Uniforms;

	void DrawSurfs( const viewDef_t *viewDef, const viewLight_t *vLight, const drawSurf_t **surfs, size_t count );
	void ShutdownMipmaps();

	Uniforms *uniforms = nullptr;
	GLSLProgram *stencilShadowShader = nullptr;

public:
	TiledCustomMipmapStage stencilShadowMipmap;
};
