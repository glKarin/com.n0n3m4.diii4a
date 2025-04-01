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

class LightPassesStage {
public:
	void Init();
	void Shutdown();

	// what to draw?
	struct DrawMask {
		bool opaque = true;			// draw local & global opaque surfaces
		bool translucent = true;	// draw translucent surfaces
	};

	void DrawAllFogLights( const viewDef_t *viewDef, const DrawMask &mask );
	void DrawAllBlendLights( const viewDef_t *viewDef );

private:
	GLSLProgram *blendLightShader = nullptr;
	GLSLProgram *fogLightShader = nullptr;

	const viewDef_t *viewDef = nullptr;
	const viewLight_t *vLight = nullptr;

	void DrawFogLight( const DrawMask &mask );
	void DrawBlendLight();

	void DrawSurf( const drawSurf_t *drawSurf, std::function<void(const drawSurf_t *)> perSurfaceSetup );
};
