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

class DepthStage
{
public:
	void Init();
	void Shutdown();

	void DrawDepth( const viewDef_t *viewDef, drawSurf_t **drawSurfs, int numDrawSurfs );

private:
	struct DepthUniforms;

	GLSLProgram *depthShader = nullptr;
	DepthUniforms *uniforms = nullptr;

	bool ShouldDrawSurf( const drawSurf_t *surf ) const;
	void DrawSurf( const drawSurf_t * drawSurf );
	void CreateDrawCommands( const drawSurf_t *surf );
	void IssueDrawCommand( const drawSurf_t *surf, const shaderStage_t *stage );

	static void LoadShader( GLSLProgram *shader );
	static void CalcScissorParam( uint32_t scissor[4], const idScreenRect &screenRect );
};
