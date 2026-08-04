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

#ifndef __MODELDECAL_H__
#define __MODELDECAL_H__

/*
===============================================================================

	Decals are lightweight primitives for bullet / blood marks.
	Decals with common materials will be merged together, but additional
	decals will be allocated as needed. The material should not be
	one that receives lighting, because no interactions are generated
	for these lightweight surfaces.

	FIXME:	Decals on models in portalled off areas do not get freed
			until the area becomes visible again.

===============================================================================
*/

const int NUM_DECAL_BOUNDING_PLANES = 6;

typedef struct decalProjectionInfo_s {
	idVec3						projectionOrigin;
	idBounds					projectionBounds;
	idPlane						boundingPlanes[6];
	idPlane						fadePlanes[2];
	idPlane						textureAxis[2];
	const idMaterial *			material;
	bool						parallel;
	float						fadeDepth;
	int							startTime;
	bool						force;
} decalProjectionInfo_t;


class idRenderModelDecal {
public:
								idRenderModelDecal( void );
								~idRenderModelDecal( void );

	static idRenderModelDecal *	Alloc( void );
	static void					Free( idRenderModelDecal *decal );

								// Creates decal projection info.
	static bool					CreateProjectionInfo( decalProjectionInfo_t &info, const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime );

								// Transform the projection info from global space to local.
	static void					GlobalProjectionInfoToLocal( decalProjectionInfo_t &localInfo, const decalProjectionInfo_t &info, const idVec3 &origin, const idMat3 &axis );

								// Creates a deal on the given model.
	void						CreateDecal( const idRenderModel *model, const decalProjectionInfo_t &localInfo );

								// Remove decals that are completely faded away.
	static idRenderModelDecal *	RemoveFadedDecals( idRenderModelDecal *decals, int time );

								// Updates the vertex colors, removing any faded indexes,
								// then copy the verts to temporary vertex cache and adds a drawSurf.
	void						AddDecalDrawSurf( struct viewEntity_s *space );

								// Returns the next decal in the chain.
	idRenderModelDecal *		Next( void ) const { return nextDecal; }

	void						ReadFromDemoFile( class idDemoFile *f );
	void						WriteToDemoFile( class idDemoFile *f ) const;

private:
	static const int			MAX_DECAL_VERTS = 40;
	static const int			MAX_DECAL_INDEXES = 60;

	const idMaterial *			material;
	srfTriangles_t				tri;
	float						vertDepthFade[MAX_DECAL_VERTS];
	int							indexStartTime[MAX_DECAL_INDEXES];
	idRenderModelDecal *		nextDecal;

								// Adds the winding triangles to the appropriate decal in the
								// chain, creating a new one if necessary.
	void						AddWinding( const idWinding &w, const idMaterial *decalMaterial, const idPlane fadePlanes[2], float fadeDepth, int startTime );

								// Adds depth faded triangles for the winding to the appropriate
								// decal in the chain, creating a new one if necessary.
								// The part of the winding at the front side of both fade planes is not faded.
								// The parts at the back sides of the fade planes are faded with the given depth.
	void						AddDepthFadedWinding( const idWinding &w, const idMaterial *decalMaterial, const idPlane fadePlanes[2], float fadeDepth, int startTime );
};

#endif /* !__MODELDECAL_H__ */
