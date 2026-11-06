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

#include "precompiled.h"
#pragma hdrstop



#include "renderer/tr_local.h"
#include "renderer/resources/Model_local.h"

// decalFade	filter 5 0.1
// polygonOffset
// {
// map invertColor( textures/splat )
// blend GL_ZERO GL_ONE_MINUS_SRC
// vertexColor
// clamp
// }

/*
==================
idDecalOnRenderModel::idDecalOnRenderModel
==================
*/
idDecalOnRenderModel::idDecalOnRenderModel( void ) {
	memset( &tri, 0, sizeof( tri ) );
	tri.verts = ( idDrawVert* )Mem_Alloc16( MAX_DECAL_VERTS * sizeof( idDrawVert ) );
	tri.indexes = ( glIndex_t* )Mem_Alloc16( MAX_DECAL_INDEXES * sizeof( glIndex_t ) );
	material = NULL;
	nextDecal = NULL;
}

/*
==================
idDecalOnRenderModel::~idDecalOnRenderModel
==================
*/
idDecalOnRenderModel::~idDecalOnRenderModel( void ) {
	Mem_Free16( tri.indexes );
	Mem_Free16( tri.verts );
}

/*
==================
idDecalOnRenderModel::Clear
==================
*/
void idDecalOnRenderModel::Clear( void ) {
	tri.numVerts = 0;
	tri.numIndexes = 0;
	material = NULL;
	nextDecal = NULL;
}

/*
==================
idDecalOnRenderModel::idDecalOnRenderModel
==================
*/
idDecalOnRenderModel *idDecalOnRenderModel::Alloc( void ) {
	return new idDecalOnRenderModel;
}

/*
==================
idDecalOnRenderModel::idDecalOnRenderModel
==================
*/
void idDecalOnRenderModel::Free( idDecalOnRenderModel *decal ) {
	delete decal;
}

/*
=================
idDecalOnRenderModel::CreateProjectionInfo
=================
*/
bool idDecalOnRenderModel::CreateProjectionInfo( decalProjectionInfo_t &info, const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime ) {

	if ( winding.GetNumPoints() != NUM_DECAL_BOUNDING_PLANES - 2 ) {
		common->Printf( "idDecalOnRenderModel::CreateProjectionInfo: winding must have %d points\n", NUM_DECAL_BOUNDING_PLANES - 2 );
		return false;
	}

	assert( material != NULL );

	info.projectionOrigin = projectionOrigin;
	info.material = material;
	info.parallel = parallel;
	info.fadeDepth = fadeDepth;
	info.startTime = startTime;
	info.force = false;

	// get the winding plane and the depth of the projection volume
	idPlane windingPlane;
	winding.GetPlane( windingPlane );
	float depth = windingPlane.Distance( projectionOrigin );

	// find the bounds for the projection
	winding.GetBounds( info.projectionBounds );
	if ( parallel ) {
		info.projectionBounds.ExpandSelf( depth );
	} else {
		info.projectionBounds.AddPoint( projectionOrigin );
	}

	// calculate the world space projection volume bounding planes, positive sides face outside the decal
	if ( parallel ) {
		for ( int i = 0; i < winding.GetNumPoints(); i++ ) {
			idVec3 edge = winding[(i+1)%winding.GetNumPoints()].ToVec3() - winding[i].ToVec3();
			info.boundingPlanes[i].Normal().Cross( windingPlane.Normal(), edge );
			info.boundingPlanes[i].Normalize();
			info.boundingPlanes[i].FitThroughPoint( winding[i].ToVec3() );
		}
	} else {
		for ( int i = 0; i < winding.GetNumPoints(); i++ ) {
			info.boundingPlanes[i].FromPoints( projectionOrigin, winding[i].ToVec3(), winding[(i+1)%winding.GetNumPoints()].ToVec3() );
		}
	}
	info.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 2] = windingPlane;
	info.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 2][3] -= depth;
	info.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 1] = -windingPlane;

	// fades will be from these plane
	info.fadePlanes[0] = windingPlane;
	info.fadePlanes[0][3] -= fadeDepth;
	info.fadePlanes[1] = -windingPlane;
	info.fadePlanes[1][3] += depth - fadeDepth;

	// calculate the texture vectors for the winding
	float	len, texArea, inva;
	idVec3	temp;
	idVec5	d0, d1;

	const idVec5 &a = winding[0];
	const idVec5 &b = winding[1];
	const idVec5 &c = winding[2];

	d0 = b.ToVec3() - a.ToVec3();
	d0.s = b.s - a.s;
	d0.t = b.t - a.t;
	d1 = c.ToVec3() - a.ToVec3();
	d1.s = c.s - a.s;
	d1.t = c.t - a.t;

	texArea = ( d0[3] * d1[4] ) - ( d0[4] * d1[3] );
	inva = 1.0f / texArea;

    temp[0] = ( d0[0] * d1[4] - d0[4] * d1[0] ) * inva;
    temp[1] = ( d0[1] * d1[4] - d0[4] * d1[1] ) * inva;
    temp[2] = ( d0[2] * d1[4] - d0[4] * d1[2] ) * inva;
	len = temp.Normalize();
	info.textureAxis[0].Normal() = temp * ( 1.0f / len );
	info.textureAxis[0][3] = winding[0].s - ( winding[0].ToVec3() * info.textureAxis[0].Normal() );

    temp[0] = ( d0[3] * d1[0] - d0[0] * d1[3] ) * inva;
    temp[1] = ( d0[3] * d1[1] - d0[1] * d1[3] ) * inva;
    temp[2] = ( d0[3] * d1[2] - d0[2] * d1[3] ) * inva;
	len = temp.Normalize();
	info.textureAxis[1].Normal() = temp * ( 1.0f / len );
	info.textureAxis[1][3] = winding[0].t - ( winding[0].ToVec3() * info.textureAxis[1].Normal() );

	return true;
}

/*
=================
idDecalOnRenderModel::CreateProjectionInfo
=================
*/
void idDecalOnRenderModel::GlobalProjectionInfoToLocal( decalProjectionInfo_t &localInfo, const decalProjectionInfo_t &info, const idVec3 &origin, const idMat3 &axis ) {
	float modelMatrix[16];

	R_AxisToModelMatrix( axis, origin, modelMatrix );

	for ( int j = 0; j < NUM_DECAL_BOUNDING_PLANES; j++ ) {
		R_GlobalPlaneToLocal( modelMatrix, info.boundingPlanes[j], localInfo.boundingPlanes[j] );
	}
	R_GlobalPlaneToLocal( modelMatrix, info.fadePlanes[0], localInfo.fadePlanes[0] );
	R_GlobalPlaneToLocal( modelMatrix, info.fadePlanes[1], localInfo.fadePlanes[1] );
	R_GlobalPlaneToLocal( modelMatrix, info.textureAxis[0], localInfo.textureAxis[0] );
	R_GlobalPlaneToLocal( modelMatrix, info.textureAxis[1], localInfo.textureAxis[1] );
	R_GlobalPointToLocal( modelMatrix, info.projectionOrigin, localInfo.projectionOrigin );
	localInfo.projectionBounds = info.projectionBounds;
	localInfo.projectionBounds.TranslateSelf( -origin );
	localInfo.projectionBounds.RotateSelf( axis.Transpose() );
	localInfo.material = info.material;
	localInfo.parallel = info.parallel;
	localInfo.fadeDepth = info.fadeDepth;
	localInfo.startTime = info.startTime;
	localInfo.force = info.force;
}

/*
=================
idDecalOnRenderModel::AddWinding
=================
*/
void idDecalOnRenderModel::AddWinding( const idWinding &w, const idMaterial *decalMaterial, const idPlane fadePlanes[2], float fadeDepth, int startTime ) {
	int i;
	float invFadeDepth, fade;
	decalInfo_t	decalInfo;

	if ( ( material == NULL || material == decalMaterial ) &&
			tri.numVerts + w.GetNumPoints() < MAX_DECAL_VERTS &&
				tri.numIndexes + ( w.GetNumPoints() - 2 ) * 3 < MAX_DECAL_INDEXES ) {

		material = decalMaterial;

		// add to this decal
		decalInfo = material->GetDecalInfo();
		invFadeDepth = -1.0f / fadeDepth;

		for ( i = 0; i < w.GetNumPoints(); i++ ) {
			fade = fadePlanes[0].Distance( w[i].ToVec3() ) * invFadeDepth;
			if ( fade < 0.0f ) {
				fade = fadePlanes[1].Distance( w[i].ToVec3() ) * invFadeDepth;
			}
			if ( fade < 0.0f ) {
				fade = 0.0f;
			} else if ( fade > 0.99f ) {
				fade = 1.0f;
			}
			fade = 1.0f - fade;
			vertDepthFade[tri.numVerts + i] = fade;
			tri.verts[tri.numVerts + i].xyz = w[i].ToVec3();
			tri.verts[tri.numVerts + i].st[0] = w[i].s;
			tri.verts[tri.numVerts + i].st[1] = w[i].t;
			for ( int k = 0 ; k < 4 ; k++ ) {
				int icolor = idMath::FtoiRound( decalInfo.start[k] * fade * 255.0f );
				if ( icolor < 0 ) {
					icolor = 0;
				} else if ( icolor > 255 ) {
					icolor = 255;
				}
				tri.verts[tri.numVerts + i].color[k] = icolor;
			}
		}
		for ( i = 2; i < w.GetNumPoints(); i++ ) {
			tri.indexes[tri.numIndexes + 0] = tri.numVerts;
			tri.indexes[tri.numIndexes + 1] = tri.numVerts + i - 1;
			tri.indexes[tri.numIndexes + 2] = tri.numVerts + i;
			indexStartTime[tri.numIndexes] =
			indexStartTime[tri.numIndexes + 1] =
			indexStartTime[tri.numIndexes + 2] = startTime;
			tri.numIndexes += 3;
		}
		tri.numVerts += w.GetNumPoints();
		return;
	}

	// if we are at the end of the list, create a new decal
	if ( !nextDecal ) {
		nextDecal = idDecalOnRenderModel::Alloc();
	}
	// let the next decal on the chain take a look
	nextDecal->AddWinding( w, decalMaterial, fadePlanes, fadeDepth, startTime );
}

/*
=================
idDecalOnRenderModel::AddDepthFadedWinding
=================
*/
void idDecalOnRenderModel::AddDepthFadedWinding( const idWinding &w, const idMaterial *decalMaterial, const idPlane fadePlanes[2], float fadeDepth, int startTime ) {
	idFixedWinding front, back;

	front = w;
	if ( front.Split( &back, fadePlanes[0], 0.1f ) == SIDE_CROSS ) {
		AddWinding( back, decalMaterial, fadePlanes, fadeDepth, startTime );
	}

	if ( front.Split( &back, fadePlanes[1], 0.1f ) == SIDE_CROSS ) {
		AddWinding( back, decalMaterial, fadePlanes, fadeDepth, startTime );
	}

	AddWinding( front, decalMaterial, fadePlanes, fadeDepth, startTime );
}

/*
=================
idDecalOnRenderModel::CreateDecal
=================
*/
void idDecalOnRenderModel::CreateDecal( const idRenderModel *model, const decalProjectionInfo_t &localInfo, bool *pAdded ) {

	// check all model surfaces
	for ( int surfNum = 0; surfNum < model->NumSurfaces(); surfNum++ ) {
		const modelSurface_t *surf = model->Surface( surfNum );

		// if no geometry or no shader
		if ( !surf->geometry || !surf->material ) {
			continue;
		}

		// decals and overlays use the same rules
		if ( !localInfo.force && !surf->material->AllowOverlays() ) {
			continue;
		}

		srfTriangles_t *stri = surf->geometry;

		// if the triangle bounds do not overlap with projection bounds
		if ( !localInfo.projectionBounds.IntersectsBounds( stri->bounds ) ) {
			continue;
		}

		// allocate memory for the cull bits
		byte *cullBits = (byte *)_alloca16( stri->numVerts * sizeof( cullBits[0] ) );

		// catagorize all points by the planes
		SIMDProcessor->DecalPointCull( cullBits, localInfo.boundingPlanes, stri->verts, stri->numVerts );

		// find triangles inside the projection volume
		for ( int triNum = 0, index = 0; index < stri->numIndexes; index += 3, triNum++ ) {
			int v1 = stri->indexes[index+0];
			int v2 = stri->indexes[index+1];
			int v3 = stri->indexes[index+2];

			// skip triangles completely off one side
			if ( cullBits[v1] & cullBits[v2] & cullBits[v3] ) {
				continue;
			}

			// skip back facing triangles
			if ( stri->facePlanes && stri->facePlanesCalculated &&
					stri->facePlanes[triNum].Normal() * localInfo.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 2].Normal() < -0.1f ) {
				continue;
			}

			// create a winding with texture coordinates for the triangle
			idFixedWinding fw;
			fw.SetNumPoints( 3 );
			if ( localInfo.parallel ) {
				for ( int j = 0; j < 3; j++ ) {
					fw[j] = stri->verts[stri->indexes[index+j]].xyz;
					fw[j].s = localInfo.textureAxis[0].Distance( fw[j].ToVec3() );
					fw[j].t = localInfo.textureAxis[1].Distance( fw[j].ToVec3() );
				}
			} else {
				for ( int j = 0; j < 3; j++ ) {
					idVec3 dir;
					float scale;

					fw[j] = stri->verts[stri->indexes[index+j]].xyz;
					dir = fw[j].ToVec3() - localInfo.projectionOrigin;
					if (!localInfo.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 1].RayIntersection( fw[j].ToVec3(), dir, scale ))
						scale = 0.0f;
					dir = fw[j].ToVec3() + scale * dir;
					fw[j].s = localInfo.textureAxis[0].Distance( dir );
					fw[j].t = localInfo.textureAxis[1].Distance( dir );
				}
			}

			int orBits = cullBits[v1] | cullBits[v2] | cullBits[v3];

			// clip the exact surface triangle to the projection volume
			for ( int j = 0; j < NUM_DECAL_BOUNDING_PLANES; j++ ) {
				if ( orBits & ( 1 << j ) ) {
					if ( !fw.ClipInPlace( -localInfo.boundingPlanes[j] ) ) {
						break;
					}
				}
			}

			if ( fw.GetNumPoints() == 0 ) {
				continue;
			}

			AddDepthFadedWinding( fw, localInfo.material, localInfo.fadePlanes, localInfo.fadeDepth, localInfo.startTime );

			if ( pAdded )
				*pAdded = true;
		}
	}
}

/*
=====================
idDecalOnRenderModel::RemoveFadedDecals
=====================
*/
idDecalOnRenderModel *idDecalOnRenderModel::RemoveFadedDecals( idDecalOnRenderModel *decals, int time, bool *pRemoved ) {
	if ( decals == NULL ) {
		return NULL;
	}

	// recursively free any next decals
	decals->nextDecal = RemoveFadedDecals( decals->nextDecal, time, pRemoved );

	// free the decals if no material set
	if ( decals->material == NULL ) {
		if ( pRemoved )
			*pRemoved = true;
		idDecalOnRenderModel *nextDecal = decals->nextDecal;
		Free( decals );
		return nextDecal;
	}
	
	decalInfo_t decalInfo = decals->material->GetDecalInfo();
	int minTime = time - ( decalInfo.stayTime + decalInfo.fadeTime );

	int newNumIndexes = 0;
	for ( int i = 0; i < decals->tri.numIndexes; i += 3 ) {
		if ( decals->indexStartTime[i] > minTime ) {
			// keep this triangle
			if ( newNumIndexes != i ) {
				for ( int j = 0; j < 3; j++ ) {
					decals->tri.indexes[newNumIndexes+j] = decals->tri.indexes[i+j];
					decals->indexStartTime[newNumIndexes+j] = decals->indexStartTime[i+j];
				}
			}
			newNumIndexes += 3;
		}
	}
	if ( pRemoved && newNumIndexes != decals->tri.numIndexes )
		*pRemoved = true;

	// free the decals if all trianges faded away
	if ( newNumIndexes == 0 ) {
		assert( !( pRemoved && *pRemoved == false ) );
		idDecalOnRenderModel *nextDecal = decals->nextDecal;
		Free( decals );
		return nextDecal;
	}

	decals->tri.numIndexes = newNumIndexes;

	int inUse[MAX_DECAL_VERTS];
	memset( inUse, 0, sizeof( inUse ) );
	for ( int i = 0; i < decals->tri.numIndexes; i++ ) {
		inUse[decals->tri.indexes[i]] = 1;
	}

	int newNumVerts = 0;
	for ( int i = 0; i < decals->tri.numVerts; i++ ) {
		if ( !inUse[i] ) {
			if ( pRemoved )
				*pRemoved = true;
			continue;
		}
		decals->tri.verts[newNumVerts] = decals->tri.verts[i];
		decals->vertDepthFade[newNumVerts] = decals->vertDepthFade[i];
		inUse[i] = newNumVerts;
		newNumVerts++;
	}
	decals->tri.numVerts = newNumVerts;

	for ( int i = 0; i < decals->tri.numIndexes; i++ ) {
		decals->tri.indexes[i] = inUse[decals->tri.indexes[i]];
	}

	return decals;
}

/*
=====================
idDecalOnRenderModel::UpdateAndGetDecalSurface
=====================
*/
const srfTriangles_t &idDecalOnRenderModel::UpdateAndGetDecalSurface( int time ) {
	// fade down all the verts with time
	decalInfo_t decalInfo = material->GetDecalInfo();
	int maxTime = decalInfo.stayTime + decalInfo.fadeTime;

	// set vertex colors and remove faded triangles
	for ( int i = 0 ; i < tri.numIndexes ; i += 3 ) {
		int	deltaTime = time - indexStartTime[i];

		if ( deltaTime > maxTime ) {
			continue;
		}

		if ( deltaTime <= decalInfo.stayTime ) {
			continue;
		}

		deltaTime -= decalInfo.stayTime;
		float f = (float)deltaTime / decalInfo.fadeTime;

		for ( int j = 0; j < 3; j++ ) {
			int	ind = tri.indexes[i+j];

			for ( int k = 0; k < 4; k++ ) {
				float fcolor = decalInfo.start[k] + ( decalInfo.end[k] - decalInfo.start[k] ) * f;
				int icolor = idMath::FtoiRound( fcolor * vertDepthFade[ind] * 255.0f );
				if ( icolor < 0 ) {
					icolor = 0;
				} else if ( icolor > 255 ) {
					icolor = 255;
				}
				tri.verts[ind].color[k] = icolor;
			}
		}
	}

	return tri;
}

/*
=====================
idDecalOnRenderModel::GetMaterial
=====================
*/
const idMaterial *idDecalOnRenderModel::GetMaterial() const {
	return material;
}

/*
=====================
idDecalOnRenderModel::ComputeBoundingBox
=====================
*/
idBounds idDecalOnRenderModel::ComputeBoundingBox() const {
	idBounds bbox;
	bbox.Clear();
	SIMDProcessor->MinMax( bbox[0], bbox[1], tri.verts, tri.numVerts );
	return bbox;
}

/*
=====================
idDecalOnRenderModel::AddDecalDrawSurf
=====================
*/
void idDecalOnRenderModel::AddDecalDrawSurf( viewEntity_t *space ) {
	if ( tri.numIndexes == 0 ) {
		return;
	}

	UpdateAndGetDecalSurface( tr.viewDef->renderView.time );

	// copy the tri and indexes to temp heap memory,
	// because if we are running multi-threaded, we wouldn't
	// be able to reorganize the index list
	srfTriangles_t *newTri = (srfTriangles_t *)R_FrameAlloc( sizeof( *newTri ) );
	*newTri = tri;

	// copy the current vertexes to temp vertex cache
	newTri->ambientCache = vertexCache.AllocVertex( tri.verts, tri.numVerts * sizeof( idDrawVert ) );
	newTri->indexCache = vertexCache.AllocIndex( tri.indexes, tri.numIndexes * sizeof( tri.indexes[0] ) );

	if ( newTri->ambientCache.IsValid() && newTri->indexCache.IsValid() ) {
		// create the drawsurf
		R_AddDrawSurf( newTri, space, &space->entityDef->parms, material, space->scissorRect, -1.0f, true );
	}
}

/*
====================
idDecalOnRenderModel::ReadFromDemoFile
====================
*/
void idDecalOnRenderModel::ReadFromDemoFile( idDemoFile *f ) {
	// FIXME: implement
}

/*
====================
idDecalOnRenderModel::WriteToDemoFile
====================
*/
void idDecalOnRenderModel::WriteToDemoFile( idDemoFile *f ) const {
	// FIXME: implement
}

//==================================================================================

static const char *Decal_SnapshotName = "_Decal_Snapshot_";

/*
====================
idRenderModelDecal::idRenderModelDecal
====================
*/
idRenderModelDecal::idRenderModelDecal() = default;

/*
====================
idRenderModelDecal::~idRenderModelDecal
====================
*/
idRenderModelDecal::~idRenderModelDecal() {
	while( decalsList ) {
		idDecalOnRenderModel *next = decalsList->Next();
		idDecalOnRenderModel::Free( decalsList );
		decalsList = next;
	}
}

/*
====================
idRenderModelDecal::IsDynamicModel
====================
*/
dynamicModel_t idRenderModelDecal::IsDynamicModel() const {
	return DM_CONTINUOUS;
}

idCVar r_useCachedDecalModels( "r_useCachedDecalModels", "1", CVAR_RENDERER | CVAR_BOOL, "cache geometry of decal models" );

/*
====================
idRenderModelDecal::IsDynamicModel
====================
*/
idRenderModel *idRenderModelDecal::InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) {
	if ( !view ) {
		// called e.g. from inside idRenderWorldLocal::TraceAll for light estimate system
		return NULL;
	}
	int genTime = view->renderView.time;

	if ( cachedModel && !r_useCachedDecalModels.GetBool() ) {
		delete cachedModel;
		cachedModel = NULL;
	}

	bool needsFullRegen = !cachedModel;
	decalsList = idDecalOnRenderModel::RemoveFadedDecals( decalsList, genTime, &needsFullRegen );

	if ( !needsFullRegen ) {
		int numDecals = 0;
		for ( idDecalOnRenderModel *decal = decalsList; decal; decal = decal->Next() )
			numDecals++;
		if ( cachedModel->NumSurfaces() != numDecals ) {
			// surfaces are duplicated in case of ShouldCreateBackSides inside FinishSurfaces
			needsFullRegen = true;
		}
	}

	idRenderModelStatic	*staticModel;
	if ( needsFullRegen ) {
		// no cached model OR geometry has changed since last generation
		delete cachedModel;
		cachedModel = NULL;
		staticModel = new idRenderModelStatic;
		staticModel->InitEmpty( Decal_SnapshotName );
	} else {
		// can reuse cached previous model, just update colors
		// this allows us to avoid calling costly FinishSurfaces every frame
		staticModel = static_cast<idRenderModelStatic*>( cachedModel );
	}

	int numSurfs = 0;
	for ( idDecalOnRenderModel *decal = decalsList; decal; decal = decal->Next() ) {
		const srfTriangles_t &srcTri = decal->UpdateAndGetDecalSurface( genTime );
		const idMaterial *material = decal->GetMaterial();

		if ( needsFullRegen ) {
			modelSurface_t surf;
			surf.geometry = R_CopyStaticTriSurf( &srcTri );
			surf.geometry->generateNormals = true;

			surf.material = material;
			surf.id = numSurfs++;
			staticModel->AddSurface( surf );
			assert( staticModel->NumSurfaces() == numSurfs );
		}
		else {
			assert( material == staticModel->surfaces[numSurfs].material );
			srfTriangles_t *updatedTri = staticModel->surfaces[numSurfs].geometry;
			numSurfs++;

			assert( updatedTri );
			assert( srcTri.numVerts == updatedTri->numVerts - updatedTri->numMirroredVerts );
			for ( int v = 0; v < srcTri.numVerts; v++ ) {
				const idDrawVert &vert = srcTri.verts[v];
				idDrawVert &updatedVert = updatedTri->verts[v];
				// only colors can change due to fading
				updatedVert.SetColor( vert.GetColor() );
			}

			// some vertexes are duplicated inside FinishSurfaces 
			for ( int m = 0; m < updatedTri->numMirroredVerts; m++ ) {
				int dstV = updatedTri->numVerts - updatedTri->numMirroredVerts + m;
				int srcV = updatedTri->mirroredVerts[m];
				updatedTri->verts[dstV].SetColor( updatedTri->verts[srcV].GetColor() );
			}
		}
	}
	assert( staticModel->NumSurfaces() == numSurfs );

	if ( needsFullRegen ) {
		staticModel->FinishSurfaces();
		// this is freshly computed tight bounding box
		// we can use it in decal model too: it might be a bit smaller due to some faded decals removed
		// however, I think entity bounds and area refs probably won't be regenerated
		bounds = staticModel->bounds;
	}

	return staticModel;
}

/*
====================
idRenderModelDecal::RecomputeBoundingBox
====================
*/
void idRenderModelDecal::RecomputeBoundingBox() {
	bounds.Clear();

	for ( idDecalOnRenderModel *decal = decalsList; decal; decal = decal->Next() ) {
		bounds.AddBounds( decal->ComputeBoundingBox() );
	}

	if ( bounds.IsCleared() )
		bounds.Zero();
}
