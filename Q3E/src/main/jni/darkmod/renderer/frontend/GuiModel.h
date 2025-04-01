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

#include "ui/Rectangle.h"

typedef struct {
	const idMaterial	*material;
	float				color[4];
	int					firstVert;
	int					numVerts;
	int					firstIndex;
	int					numIndexes;
} guiModelSurface_t;

class idGuiModel {
public:
	idGuiModel();

	void	Clear();

	void	WriteToDemo( idDemoFile *demo );
	void	ReadFromDemo( idDemoFile *demo );	
	
	void	EmitToCurrentView( float modelMatrix[16], bool depthHack );
	void	EmitFullScreen();

	// these calls are forwarded from the renderer
	void	SetColor( float r, float g, float b, float a );
	void	DrawStretchPic( const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *hShader,
									bool clip = true, float min_x = 0.0f, float min_y = 0.0f, float max_x = 640.0f, float max_y = 480.0f );
	void	DrawStretchPic( float x, float y, float w, float h,
									float s1, float t1, float s2, float t2, const idMaterial *hShader);
	void	DrawStretchTri ( idVec2 p1, idVec2 p2, idVec2 p3, idVec2 t1, idVec2 t2, idVec2 t3, const idMaterial *material );

	// stgatilov #6434: X-ray in GUI overlay
	void	ProcessOverlaySubviews(drawSurf_t *drawSurf);
	const textureStage_t *NeedXraySubview() const;
	void	SetXrayImageOverride(idImageScratch *image);

	//---------------------------
private:
	void	AdvanceSurf();
	void	EmitSurface( guiModelSurface_t *surf, float modelMatrix[16], float modelViewMatrix[16], bool depthHack );

	guiModelSurface_t		*surf;

	idList<guiModelSurface_t>	surfaces;
	idList<glIndex_t>		indexes;
	idList<idDrawVert>	verts;

	// stgatilov #6434: X-ray in GUI overlay
	const textureStage_t *needXraySubview;
	idImageScratch *xrayImageOverride;
};

