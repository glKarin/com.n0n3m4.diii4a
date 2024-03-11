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

class ShadowMapStage
{
public:
	ShadowMapStage();

	void Init();
	void Shutdown();

	// what to draw?
	struct DrawMask {
		bool clear = true;		// clear shadow map page first
		bool global = true;		// draw global shadows
		bool local = true;		// draw local shadows
	};

	bool DrawShadowMapSingleLight( const viewDef_t *viewDef, const viewLight_t *vLight, const DrawMask &mask );
	void DrawShadowMapAllLights( const viewDef_t *viewDef, const DrawMask &mask );

private:
	GLSLProgram *shadowMapShader = nullptr;

	struct Uniforms;
	Uniforms *uniforms = nullptr;

	void Start();
	bool ShouldDrawLight( const viewLight_t *vLight, const DrawMask &mask );
	void DrawLight( const viewLight_t *vLight, const DrawMask &mask );
	void End();

	bool ShouldDrawSurf( const drawSurf_t *surf ) const;
	void DrawLightInteractions( const drawSurf_t *surfs );
	void DrawSurf( const drawSurf_t * drawSurf );
	void IssueDrawCommand( const drawSurf_t *surf, const shaderStage_t *stage );
};
