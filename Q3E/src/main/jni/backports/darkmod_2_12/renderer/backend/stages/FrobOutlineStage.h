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

class FrameBuffer;

class FrobOutlineStage {
public:
	void Init();
	void Shutdown();

	void DrawFrobOutline( drawSurf_t **drawSurfs, int numDrawSurfs );

private:
	GLSLProgram *silhouetteShader = nullptr;
	GLSLProgram *highlightShader = nullptr;
	GLSLProgram *extrudeShader = nullptr;
	GLSLProgram *applyShader = nullptr;

	idImageScratch *colorTex[2] = { nullptr };
	idImageScratch *depthTex = nullptr;
	FrameBuffer *fbo[2] = { nullptr };
	FrameBuffer *drawFbo = nullptr;

	void CreateFbo( int idx );
	void CreateDrawFbo();

	void DrawFrobImageBasedIgnoreDepth( idList<drawSurf_t*> &surfs );
	void DrawFrobImageBased( idList<drawSurf_t*> &surfs );
	void DrawFrobGeometric( idList<drawSurf_t*> &surfs );

	void DrawSurfaces( idList<drawSurf_t*> &surfs, bool highlight, bool enableAlphaTest );
	void MarkOutline( idList<drawSurf_t*> &surfs );
	void DrawImageBasedOutline( idList<drawSurf_t*> &surfs, int stencilMask );
	void DrawGeometricOutline( idList<drawSurf_t*> &surfs );

	void DrawElements( idList<drawSurf_t *> &surfs, GLSLProgram *shader, bool enableAlphaTest );
	void ApplyBlur();
};

extern idCVar r_newFrob;
extern idCVar r_frobOutline;
