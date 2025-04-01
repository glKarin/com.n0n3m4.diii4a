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

#ifndef __MODELOVERLAY_H__
#define __MODELOVERLAY_H__

/*
===============================================================================

	Render model overlay for adding decals on top of dynamic models.

===============================================================================
*/

const int MAX_OVERLAY_SURFACES	= 16;

typedef struct overlayVertex_s {
	int							vertexNum;
	float						st[2];
} overlayVertex_t;

typedef struct overlaySurface_s {
	int							surfaceNum;
	int							surfaceId;
	int							numIndexes;
	glIndex_t *					indexes;
	int							numVerts;
	overlayVertex_t *			verts;
} overlaySurface_t;

typedef struct overlayMaterial_s {
	const idMaterial *			material;
	idList<overlaySurface_t *>	surfaces;
} overlayMaterial_t;


class idRenderModelOverlay {
public:
								idRenderModelOverlay();
								~idRenderModelOverlay();

	static idRenderModelOverlay *Alloc( void );
	static void					Free( idRenderModelOverlay *overlay );

	// Projects an overlay onto deformable geometry and can be added to
	// a render entity to allow decals on top of dynamic models.
	// This does not generate tangent vectors, so it can't be used with
	// light interaction shaders. Materials for overlays should always
	// be clamped, because the projected texcoords can run well off the
	// texture since no new clip vertexes are generated.
	void						CreateOverlay( const idRenderModel *model, const idPlane localTextureAxis[2], const idMaterial *material,
											   const idDeclSkin *customSkin, const idMaterial *customShader); // Skin params added -- SteveL #3844

	// Creates new model surfaces for baseModel, which should be a static instantiation of a dynamic model.
	void						AddOverlaySurfacesToModel( idRenderModel *baseModel );

	// Removes overlay surfaces from the model.
	static void					RemoveOverlaySurfacesFromModel( idRenderModel *baseModel );

	void						ReadFromDemoFile( class idDemoFile *f );
	void						WriteToDemoFile( class idDemoFile *f ) const;

private:
	idList<overlayMaterial_t *>	materials;

	void						FreeSurface( overlaySurface_t *surface );
};

#endif /* !__MODELOVERLAY_H__ */
