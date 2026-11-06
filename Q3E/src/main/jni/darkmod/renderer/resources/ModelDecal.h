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

#include <renderer/resources/Model_local.h>

/*
===============================================================================

	Decals are lightweight primitives for bullet / blood marks.
	Decals with common materials will be merged together, but additional
	decals will be allocated as needed. The material should not be
	one that receives lighting, because no interactions are generated
	for these lightweight surfaces.
	#5867: it is now possible to generate decals with light interactions.

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


class idDecalOnRenderModel {
public:
								idDecalOnRenderModel( void );
								~idDecalOnRenderModel( void );

	void						Clear( void );
	static idDecalOnRenderModel *Alloc( void );
	static void					Free( idDecalOnRenderModel *decal );

								// Creates decal projection info.
	static bool					CreateProjectionInfo( decalProjectionInfo_t &info, const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime );

								// Transform the projection info from global space to local.
	static void					GlobalProjectionInfoToLocal( decalProjectionInfo_t &localInfo, const decalProjectionInfo_t &info, const idVec3 &origin, const idMat3 &axis );

								// Creates a decal on the given model.
	void						CreateDecal( const idRenderModel *model, const decalProjectionInfo_t &localInfo, bool *pAdded = nullptr );

								// Remove decals that are completely faded away.
	static idDecalOnRenderModel *RemoveFadedDecals( idDecalOnRenderModel *decals, int time, bool *pRemoved = nullptr );

								// Updates the vertex colors, removing any faded indexes,
								// then copy the verts to temporary vertex cache and adds a drawSurf.
	void						AddDecalDrawSurf( struct viewEntity_s *space );

	const srfTriangles_t &		UpdateAndGetDecalSurface( int time );
	const idMaterial *			GetMaterial() const;

	idBounds					ComputeBoundingBox() const;

								// Returns the next decal in the chain.
	idDecalOnRenderModel *		Next( void ) const { return nextDecal; }

	void						ReadFromDemoFile( class idDemoFile *f );
	void						WriteToDemoFile( class idDemoFile *f ) const;

private:
	static const int			MAX_DECAL_VERTS = 40;
	static const int			MAX_DECAL_INDEXES = 60;

	const idMaterial *			material;
	srfTriangles_t				tri;
	float						vertDepthFade[MAX_DECAL_VERTS];
	int							indexStartTime[MAX_DECAL_INDEXES];
	idDecalOnRenderModel *		nextDecal;

								// Adds the winding triangles to the appropriate decal in the
								// chain, creating a new one if necessary.
	void						AddWinding( const idWinding &w, const idMaterial *decalMaterial, const idPlane fadePlanes[2], float fadeDepth, int startTime );

								// Adds depth faded triangles for the winding to the appropriate
								// decal in the chain, creating a new one if necessary.
								// The part of the winding at the front side of both fade planes is not faded.
								// The parts at the back sides of the fade planes are faded with the given depth.
	void						AddDepthFadedWinding( const idWinding &w, const idMaterial *decalMaterial, const idPlane fadePlanes[2], float fadeDepth, int startTime );
};


// stgatilov #5867: idRenderModel representation of decals
// decals using it can be properly processed by frontend
class idRenderModelDecal : public idRenderModelStatic {
public:
	idRenderModelDecal();
	~idRenderModelDecal();

	virtual dynamicModel_t IsDynamicModel() const override;
	virtual idRenderModel *InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) override;

	void RecomputeBoundingBox();

	idDecalOnRenderModel *decalsList = nullptr;
};

#endif /* !__MODELDECAL_H__ */
