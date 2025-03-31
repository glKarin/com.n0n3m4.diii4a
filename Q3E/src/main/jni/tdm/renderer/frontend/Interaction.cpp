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
#include "renderer/frontend/Interaction.h"

/*
===========================================================================

idInteraction implementation

===========================================================================
*/

// FIXME: use private allocator for srfCullInfo_t
idCVar r_useInteractionTriCulling(
	"r_useInteractionTriCulling", "1",
	CVAR_RENDERER | CVAR_INTEGER,
	"  1 = cull interactions tris\n"
	"  0 = skip some culling of interactions tris\n"
	" -1 = disable all culling\n"
, -1, 1);
idCVarInt r_singleShadowEntity( "r_singleShadowEntity", "-1", CVAR_RENDERER, "suppress all but one shadowing entity" );

void idInteraction::PrepareLightSurf( linkLocation_t link, const srfTriangles_t *tri, const viewEntity_s *space,
		const idMaterial *material, const idScreenRect &scissor, bool viewInsideShadow, bool blockSelfShadows ) {

	drawSurf_t *drawSurf = R_PrepareLightSurf( tri, space, material, scissor, viewInsideShadow, blockSelfShadows );
	drawSurf->nextOnLight = surfsToLink[link];
	surfsToLink[link] = drawSurf;
}

/*
================
R_CalcInteractionFacing

Determines which triangles of the surface are facing towards the light origin.

The facing array should be allocated with one extra index than
the number of surface triangles, which will be used to handle dangling
edge silhouettes.
================
*/
void R_CalcInteractionFacing( const idRenderEntityLocal *ent, const srfTriangles_t *tri, const idRenderLightLocal *light, srfCullInfo_t &cullInfo ) {
	idVec3 localLightOrigin;

	if ( cullInfo.facing != NULL ) {
		return;
	}
	R_GlobalPointToLocal( ent->modelMatrix, light->globalLightOrigin, localLightOrigin );
	const int numFaces = tri->numIndexes / 3;

	cullInfo.facing = ( byte* )R_StaticAlloc( ( numFaces + 1 ) * sizeof( cullInfo.facing[0] ) );

	// exact geometric cull against face
	SIMDProcessor->CalcTriFacing(tri->verts, tri->numVerts, tri->indexes, tri->numIndexes, localLightOrigin, cullInfo.facing);
	cullInfo.facing[numFaces] = 1;	// for dangling edges to reference
}

/*
=====================
R_CalcInteractionCullBits

We want to cull a little on the sloppy side, because the pre-clipping
of geometry to the lights in dmap will give many cases that are right
at the border we throw things out on the border, because if any one
vertex is clearly inside, the entire triangle will be accepted.
=====================
*/
void R_CalcInteractionCullBits( const idRenderEntityLocal *ent, const srfTriangles_t *tri, const idRenderLightLocal *light, srfCullInfo_t &cullInfo ) {
	if ( r_useAnonreclaimer.GetBool() ) {
		if ( cullInfo.cullBits != NULL ) {
			return;
		}
		idPlane frustumPlanes[6];
		idRenderMatrix::GetFrustumPlanes( frustumPlanes, light->baseLightProject, true, true );
		int frontBits = 0;

		// cull the triangle surface bounding box
		for ( int i = 0; i < 6; i++ ) {
			R_GlobalPlaneToLocal( ent->modelMatrix, frustumPlanes[i], cullInfo.localClipPlanes[i] );

			// get front bits for the whole surface
			if ( tri->bounds.PlaneDistance( cullInfo.localClipPlanes[i] ) >= LIGHT_CLIP_EPSILON ) {
				frontBits |= 1 << i;
			}
		}

		// if the surface is completely inside the light frustum
		if ( frontBits == ( ( 1 << 6 ) - 1 ) ) {
			cullInfo.cullBits = LIGHT_CULL_ALL_FRONT;
			return;
		}
		cullInfo.cullBits = ( byte* )R_StaticAlloc( tri->numVerts * sizeof( cullInfo.cullBits[0] ) );
		memset( cullInfo.cullBits, 0, tri->numVerts * sizeof( cullInfo.cullBits[0] ) );

		for ( int i = 0; i < 6; i++ ) {
			// if completely infront of this clipping plane
			if ( frontBits & ( 1 << i ) ) {
				continue;
			}
			for ( int j = 0; j < tri->numVerts; j++ ) {
				float d = cullInfo.localClipPlanes[i].Distance( tri->verts[j].xyz );
				cullInfo.cullBits[j] |= ( d < LIGHT_CLIP_EPSILON ) << i;
			}
		}
	} else {
		int i, frontBits;

		if ( cullInfo.cullBits != NULL ) {
			return;
		}
		frontBits = 0;

		// cull the triangle surface bounding box
		for ( i = 0; i < 6; i++ ) {

			R_GlobalPlaneToLocal( ent->modelMatrix, -light->frustum[i], cullInfo.localClipPlanes[i] );

			// get front bits for the whole surface
			if ( tri->bounds.PlaneDistance( cullInfo.localClipPlanes[i] ) >= LIGHT_CLIP_EPSILON ) {
				frontBits |= 1 << i;
			}
		}

		// if the surface is completely inside the light frustum
		if ( frontBits == ( ( 1 << 6 ) - 1 ) || r_useInteractionTriCulling.GetInteger() == 0 ) {
			cullInfo.cullBits = LIGHT_CULL_ALL_FRONT;
			return;
		}
		cullInfo.cullBits = ( byte * )R_StaticAlloc( tri->numVerts * sizeof( cullInfo.cullBits[0] ) );
		// duzenko #4848
		// revelator: removed old code path, keep source clean.
		SIMDProcessor->CullByFrustum( tri->verts, tri->numVerts, cullInfo.localClipPlanes, cullInfo.cullBits, LIGHT_CLIP_EPSILON );
	}
}

/*
================
R_FreeInteractionCullInfo
================
*/
void R_FreeInteractionCullInfo( srfCullInfo_t &cullInfo ) {
	if ( cullInfo.facing != NULL ) {
		R_StaticFree( cullInfo.facing );
		cullInfo.facing = NULL;
	}
	if ( cullInfo.cullBits != NULL ) {
		if ( cullInfo.cullBits != LIGHT_CULL_ALL_FRONT ) {
			R_StaticFree( cullInfo.cullBits );
		}
		cullInfo.cullBits = NULL;
	}
}

#define	MAX_CLIPPED_POINTS	20
typedef struct {
	int		numVerts;
	idVec3	verts[MAX_CLIPPED_POINTS];
} clipTri_t;

/*
=============
R_ChopWinding

Clips a triangle from one buffer to another, setting edge flags
The returned buffer may be the same as inNum if no clipping is done
If entirely clipped away, clipTris[returned].numVerts == 0

I have some worries about edge flag cases when polygons are clipped
multiple times near the epsilon.
=============
*/
static int R_ChopWinding( clipTri_t clipTris[2], int inNum, const idPlane plane ) {
	clipTri_t	*in, *out;
	float	dists[MAX_CLIPPED_POINTS];
	int		sides[MAX_CLIPPED_POINTS];
	int		counts[3];
	float	dot;
	int		i, j;
	idVec3	mid;
	bool	front;

	in = &clipTris[inNum];
	out = &clipTris[inNum ^ 1];
	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	front = false;
	for ( i = 0; i < in->numVerts; i++ ) {
		dot = in->verts[i] * plane.Normal() + plane[3];
		dists[i] = dot;
		if ( dot < LIGHT_CLIP_EPSILON ) {	// slop onto the back
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_FRONT;
			if ( dot > LIGHT_CLIP_EPSILON ) {
				front = true;
			}
		}
		counts[sides[i]]++;
	}

	// if none in front, it is completely clipped away
	if ( !front ) {
		in->numVerts = 0;
		return inNum;
	}
	if ( !counts[SIDE_BACK] ) {
		return inNum;		// inout stays the same
	}

	// avoid wrapping checks by duplicating first value to end
	sides[i] = sides[0];
	dists[i] = dists[0];

	in->verts[in->numVerts] = in->verts[0];

	out->numVerts = 0;

	for ( i = 0 ; i < in->numVerts ; i++ ) {
		idVec3 &p1 = in->verts[i];

		if ( sides[i] == SIDE_FRONT ) {
			out->verts[out->numVerts] = p1;
			out->numVerts++;
		}

		if ( sides[i + 1] == sides[i] ) {
			continue;
		}

		// generate a split point
		idVec3 &p2 = in->verts[i + 1];

		dot = dists[i] / ( dists[i] - dists[i + 1] );

		for ( j = 0; j < 3; j++ ) {
			mid[j] = p1[j] + dot * ( p2[j] - p1[j] );
		}
		out->verts[out->numVerts] = mid;
		out->numVerts++;
	}
	return inNum ^ 1;
}

/*
===================
R_ClipTriangleToLight

Returns false if nothing is left after clipping
===================
*/
static bool	R_ClipTriangleToLight( const idVec3 &a, const idVec3 &b, const idVec3 &c, int planeBits, const idPlane frustum[6] ) {
	int			i;
	clipTri_t	pingPong[2];
	int			p;

	pingPong[0].numVerts = 3;
	pingPong[0].verts[0] = a;
	pingPong[0].verts[1] = b;
	pingPong[0].verts[2] = c;

	p = 0;
	for ( i = 0 ; i < 6 ; i++ ) {
		if ( planeBits & ( 1 << i ) ) {
			p = R_ChopWinding( pingPong, p, frustum[i] );
			if ( pingPong[p].numVerts < 1 ) {
				return false;
			}
		}
	}
	return true;
}

idCVar r_modelBvhLightTris(
	"r_modelBvhLightTris", "3",
	CVAR_RENDERER | CVAR_INTEGER,
	"  0 = noBVH, precise triangle culling (Doom 3 default)\n"
	"  1 = use BVH, do NOT cull triangles precisely\n"
	"      (some out-of-volume and backfacing triangles are allowed)\n"
	"  2 = use BVH, cull by light volume precisely\n"
	"      (some backfacing triangles are allowed)\n"
	"  3 = use BVH + cull by light and facing precisely\n"
, 0, 3);
idCVar r_modelBvhLightTrisGranularity(
	"r_modelBvhLightTrisGranularity", "32",
	CVAR_RENDERER | CVAR_INTEGER,
	"Don't recurse into nodes of less size (overhead tweaking)."
, 0, 1<<20);

static srfTriangles_t *R_FinishLightTrisWithBvh(
	const idRenderEntityLocal *ent, const idRenderLightLocal *light,
	const srfTriangles_t *tri, srfTriangles_t *newTri,
	bool includeBackFaces
);

/*
====================
R_CreateLightTris

The resulting surface will be a subset of the original triangles,
it will never clip triangles, but it may cull on a per-triangle basis.
====================
*/
static srfTriangles_t *R_CreateLightTris( const idRenderEntityLocal *ent,
        const srfTriangles_t *tri, const idRenderLightLocal *light,
        const idMaterial *shader, srfCullInfo_t &cullInfo
) {
	TRACE_CPU_SCOPE("R_CreateLightTris");

	int			i;
	int			numIndexes;
	glIndex_t	*indexes;
	srfTriangles_t	*newTri;
	int			c_backfaced;
	int			c_distance;
	idBounds	bounds;
	bool		includeBackFaces;
	int			faceNum;

	tr.pc.c_createLightTris++;
	c_backfaced = 0;
	c_distance = 0;

	numIndexes = 0;
	indexes = NULL;

	// it is debatable if non-shadowing lights should light back faces. we aren't at the moment
	includeBackFaces = r_lightAllBackFaces.GetBool() || light->shadows == LS_MAPS ||  // need the back faces for shadows
										light->lightShader->LightEffectsBackSides() || 
										shader->ReceivesLightingOnBackSides() || 
										ent->parms.noSelfShadow || ent->parms.noShadow;

	// allocate a new surface for the lit triangles
	newTri = R_AllocStaticTriSurf();

	// save a reference to the original surface
	newTri->ambientSurface = const_cast<srfTriangles_t *>( tri );

	// the light surface references the verts of the ambient surface
	newTri->numVerts = tri->numVerts;
	R_ReferenceStaticTriSurfVerts( newTri, tri );

	if ( r_modelBvhLightTris.GetInteger() > 0 && tri->bvhNodes && tri->numIndexes / 3 > r_modelBvhLightTrisGranularity.GetInteger() ) {
		// stgatilov #5886: BVH-optimized culling by light volume and facing
		return R_FinishLightTrisWithBvh( ent, light, tri, newTri, includeBackFaces );
	}

	if ( r_useInteractionTriCulling.GetInteger() == -1 ) {
		// stgatilov: don't waste CPU time on individual triangles
		// send them all to GPU and let it do its thing!
		// TODO: does it break anything?...
		newTri->numIndexes = tri->numIndexes;
		R_ReferenceStaticTriSurfIndexes( newTri, tri );
		newTri->bounds = tri->bounds;
		return newTri;
	}

	// calculate cull information
	if ( !includeBackFaces ) {
		R_CalcInteractionFacing( ent, tri, light, cullInfo );
	}
	R_CalcInteractionCullBits( ent, tri, light, cullInfo );

	// if the surface is completely inside the light frustum
	if ( cullInfo.cullBits == LIGHT_CULL_ALL_FRONT ) {

		// if we aren't self shadowing, let back facing triangles get
		// through so the smooth shaded bump maps light all the way around
		if ( includeBackFaces ) {

			// the whole surface is lit so the light surface just references the indexes of the ambient surface
			R_ReferenceStaticTriSurfIndexes( newTri, tri );
			numIndexes = tri->numIndexes;
			bounds = tri->bounds;

		} else {

			// the light tris indexes are going to be a subset of the original indexes so we generally
			// allocate too much memory here but we decrease the memory block when the number of indexes is known
			R_AllocStaticTriSurfIndexes( newTri, tri->numIndexes );

			// back face cull the individual triangles
			indexes = newTri->indexes;
			const byte *facing = cullInfo.facing;
			for ( faceNum = i = 0; i < tri->numIndexes; i += 3, faceNum++ ) {
				if ( !facing[ faceNum ] ) {
					c_backfaced++;
					continue;
				}
				indexes[numIndexes + 0] = tri->indexes[i + 0];
				indexes[numIndexes + 1] = tri->indexes[i + 1];
				indexes[numIndexes + 2] = tri->indexes[i + 2];
				numIndexes += 3;
			}

			// get bounds for the surface
			SIMDProcessor->MinMax( bounds[0], bounds[1], tri->verts, indexes, numIndexes );

			// decrease the size of the memory block to the size of the number of used indexes
			R_ResizeStaticTriSurfIndexes( newTri, numIndexes );
		}

	} else {

		// the light tris indexes are going to be a subset of the original indexes so we generally
		// allocate too much memory here but we decrease the memory block when the number of indexes is known
		R_AllocStaticTriSurfIndexes( newTri, tri->numIndexes );

		// cull individual triangles
		indexes = newTri->indexes;
		const byte *facing = cullInfo.facing;
		const byte *cullBits = cullInfo.cullBits;
		for ( faceNum = i = 0; i < tri->numIndexes; i += 3, faceNum++ ) {
			int i1, i2, i3;

			// if we aren't self shadowing, let back facing triangles get
			// through so the smooth shaded bump maps light all the way around
			if ( !includeBackFaces ) {
				// back face cull
				if ( !facing[ faceNum ] ) {
					c_backfaced++;
					continue;
				}
			}
			i1 = tri->indexes[i + 0];
			i2 = tri->indexes[i + 1];
			i3 = tri->indexes[i + 2];

			// fast cull outside the frustum
			// if all three points are off one plane side, it definately isn't visible
			if ( cullBits[i1] & cullBits[i2] & cullBits[i3] ) {
				c_distance++;
				continue;
			}

			if ( r_usePreciseTriangleInteractions.GetBool() ) {
				// do a precise clipped cull if none of the points is completely inside the frustum
				// note that we do not actually use the clipped triangle, which would have Z fighting issues.
				if ( cullBits[i1] && cullBits[i2] && cullBits[i3] ) {
					int cull = cullBits[i1] | cullBits[i2] | cullBits[i3];
					if ( !R_ClipTriangleToLight( tri->verts[i1].xyz, tri->verts[i2].xyz, tri->verts[i3].xyz, cull, cullInfo.localClipPlanes ) ) {
						continue;
					}
				}
			}

			// add to the list
			indexes[numIndexes + 0] = i1;
			indexes[numIndexes + 1] = i2;
			indexes[numIndexes + 2] = i3;
			numIndexes += 3;
		}

		// get bounds for the surface
		SIMDProcessor->MinMax( bounds[0], bounds[1], tri->verts, indexes, numIndexes );

		// decrease the size of the memory block to the size of the number of used indexes
		R_ResizeStaticTriSurfIndexes( newTri, numIndexes );
	}

	if ( !numIndexes ) {
		R_ReallyFreeStaticTriSurf( newTri );
		return NULL;
	}
	if ( tri->indexes != newTri->indexes && numIndexes == tri->numIndexes ) {
		// stgatilov: better draw from the same index array
		// don't waste time to transfer and load exact copy
		R_FreeStaticTriSurfIndexes( newTri );
		R_ReferenceStaticTriSurfIndexes( newTri, tri );
	}
	newTri->numIndexes = numIndexes;
	newTri->bounds = bounds;

	return newTri;
}

/*
====================
R_FinishLightTrisWithBvh

Almost whole R_CreateLightTris reimplemented with BVH acceleration.
====================
*/
srfTriangles_t *R_FinishLightTrisWithBvh(
	const idRenderEntityLocal *ent, const idRenderLightLocal *light,
	const srfTriangles_t *tri, srfTriangles_t *newTri,
	bool includeBackFaces
) {
	// transform light geometry into model space
	idVec3 localLightOrigin;
	idPlane localLightFrustum[6];
	R_GlobalPointToLocal( ent->modelMatrix, light->globalLightOrigin, localLightOrigin );
	for ( int i = 0; i < 6; i++ )
		R_GlobalPlaneToLocal( ent->modelMatrix, -light->frustum[i], localLightFrustum[i] );

	int forceMask = 0;
	if ( r_modelBvhLightTris.GetInteger() == 1 )
		forceMask = BVH_TRI_SURELY_MATCH;		// don't check triangles in BVH leaves
	else if ( r_modelBvhLightTris.GetInteger() == 2 )
		forceMask = BVH_TRI_SURELY_GOOD_ORI;	// don't check facing of triangles in BVH leaves

	// filter triangles:
	//   1) inside light frustum
	//   2) frontfacing   (or all if flag is set)
	idFlexList<bvhTriRange_t, 128> intervals;
	R_CullBvhByFrustumAndOrigin(
		tri->bounds, tri->bvhNodes,
		localLightFrustum, (includeBackFaces ? 0 : 1), localLightOrigin,
		forceMask, r_modelBvhLightTrisGranularity.GetInteger(),
		intervals
	);

	int totalTris = 0;
	idFlexList<int, 1024> preciseTris;

	// uncertain intervals should usually be short
	idFlexList<byte, 1024> triCull, triFacing;
	// go through all intervals and filter individual triangles in uncertain ones
	for ( int i = 0; i < intervals.Num(); i++ ) {
		int beg = intervals[i].beg;
		int len = intervals[i].end - intervals[i].beg;
		int info = intervals[i].info;

		if ( info == BVH_TRI_SURELY_MATCH ) {
			// all triangles surely match, we'll add them to result later
			totalTris += len;
			continue;
		}

		// see which triangles are within light frustum
		triCull.SetNum( len );
		if ( info & BVH_TRI_SURELY_WITHIN_LIGHT )
			memset( triCull.Ptr(), 0, triCull.Num() * sizeof(triCull[0]) );
		else {
			SIMDProcessor->CullTrisByFrustum(
				tri->verts, tri->numVerts,
				tri->indexes + 3 * beg, 3 * len,
				localLightFrustum, triCull.Ptr(), LIGHT_CLIP_EPSILON
			);
		}

		// see which triangles are frontfacing
		triFacing.SetNum( len );
		if ( info & BVH_TRI_SURELY_GOOD_ORI )
			memset( triFacing.Ptr(), true, triFacing.Num() * sizeof(triFacing[0]) );
		else {
			SIMDProcessor->CalcTriFacing(
				tri->verts, tri->numVerts,
				tri->indexes + 3 * beg, 3 * len,
				localLightOrigin, triFacing.Ptr()
			);
		}

		for ( int t = 0; t < len; t++ ) {
			if ( triCull[t] != 0 || triFacing[t] == false )
				continue;
			// this triangle matches: add it to "precise list"
			preciseTris.AddGrow( beg + t );
			totalTris++;
		}
	}

	// we know total number of triangles in result
	newTri->numIndexes = 3 * totalTris;
	if ( newTri->numIndexes == 0 ) {
		// no triangle taken -> return empty geometry
		R_ReallyFreeStaticTriSurf( newTri );
		return NULL;
	}
	if ( newTri->numIndexes == tri->numIndexes ) {
		// all triangles taken -> return same geometry
		R_ReferenceStaticTriSurfIndexes( newTri, tri );
		newTri->bounds = tri->bounds;
		return newTri;
	}

	// allocate memory for new index buffer
	R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );

	idBounds totalBounds;
	totalBounds.Clear();
	int pos = 0;

	for ( int i = 0; i < intervals.Num(); i++ ) {
		if ( intervals[i].info != BVH_TRI_SURELY_MATCH )
			continue;
		// this interval surely matches, copy its indices to result
		int beg = 3 * intervals[i].beg, end = 3 * intervals[i].end;
		SIMDProcessor->Memcpy( newTri->indexes + pos, tri->indexes + beg, (end - beg) * sizeof(tri->indexes[0]) );
		pos += end - beg;
		totalBounds.AddBounds( intervals[i].GetBox() );
	}

	for ( int i = 0; i < preciseTris.Num(); i++ ) {
		// add "precise list" to results: that's all matching triangles from "not surely matches" intervals
		int t = preciseTris[i];
		int i0 = tri->indexes[3 * t + 0];
		int i1 = tri->indexes[3 * t + 1];
		int i2 = tri->indexes[3 * t + 2];
		newTri->indexes[pos++] = i0;
		newTri->indexes[pos++] = i1;
		newTri->indexes[pos++] = i2;
		// these vertices will be added to totalBounds later
	}

	assert( pos == totalTris * 3 );

	// compute bounds of preciseTris with SIMD
	idBounds preciseBounds;
	SIMDProcessor->MinMax( preciseBounds[0], preciseBounds[1], tri->verts, newTri->indexes + (totalTris - preciseTris.Num()) * 3, preciseTris.Num() * 3 );
	totalBounds.AddBounds( preciseBounds );

	// we have computed bounds partly from BVH nodes information
	// that's much faster than going through all vertices of all filtered triangles
	newTri->bounds = totalBounds;

	return newTri;
}


/*
===============
idInteraction::idInteraction
===============
*/
idInteraction::idInteraction( void ) {
	numSurfaces				= 0;
	surfaces				= NULL;
	entityDef				= NULL;
	lightDef				= NULL;
	lightNext				= NULL;
	lightPrev				= NULL;
	entityNext				= NULL;
	entityPrev				= NULL;
	dynamicModelFrameCount	= 0;
	frustumState			= FRUSTUM_UNINITIALIZED;
	viewCountGenLightSurfs	= 0;
	viewCountGenShadowSurfs	= 0;
}

/*
===============
idInteraction::AllocAndLink
===============
*/
idInteraction *idInteraction::AllocAndLink( idRenderEntityLocal *edef, idRenderLightLocal *ldef ) {
	if ( !edef || !ldef ) {
		common->Error( "idInteraction::AllocAndLink: NULL parm" );
	}
	idRenderWorldLocal *renderWorld = edef->world;
	idInteraction *interaction = renderWorld->interactionAllocator.Alloc();
	interaction->flagMakeEmpty = false;

	// link and initialize
	interaction->dynamicModelFrameCount = 0;

	interaction->lightDef = ldef;
	interaction->entityDef = edef;

	interaction->numSurfaces = -1;		// not checked yet
	interaction->surfaces = NULL;

	interaction->frustumState = idInteraction::FRUSTUM_UNINITIALIZED;

	// link at the start of the entity's list
	interaction->lightNext = ldef->firstInteraction;
	interaction->lightPrev = NULL;
	ldef->firstInteraction = interaction;

	if ( interaction->lightNext != NULL ) {
		interaction->lightNext->lightPrev = interaction;
	} else {
		ldef->lastInteraction = interaction;
	}

	// link at the start of the light's list
	interaction->entityNext = edef->firstInteraction;
	interaction->entityPrev = NULL;
	edef->firstInteraction = interaction;

	if ( interaction->entityNext != NULL ) {
		interaction->entityNext->entityPrev = interaction;
	} else {
		edef->lastInteraction = interaction;
	}

	// update the interaction table
	bool added = renderWorld->interactionTable.Add(interaction);
	if ( !added ) { 
		common->Error( "idInteraction::AllocAndLink: interaction already in table" ); 
	}
	
	return interaction;
}

/*
===============
idInteraction::FreeSurfaces

Frees the surfaces, but leaves the interaction linked in, so it
will be regenerated automatically
===============
*/
void idInteraction::FreeSurfaces( void ) {
	if ( this->surfaces ) {
		for ( int i = 0 ; i < this->numSurfaces ; i++ ) {
			surfaceInteraction_t *sint = &this->surfaces[i];

			if ( sint->shadowMapTris ) {
				if ( sint->shadowMapTris != sint->lightTris )
					R_FreeStaticTriSurf( sint->shadowMapTris );
				sint->shadowMapTris = NULL;
			}
			if ( sint->lightTris ) {
				if ( sint->lightTris != LIGHT_TRIS_DEFERRED ) {
					R_FreeStaticTriSurf( sint->lightTris );
				}
				sint->lightTris = NULL;
			}
			if ( sint->shadowVolumeTris ) {
				// if it doesn't have an entityDef, it is part of a prelight
				// model, not a generated interaction
				if ( this->entityDef ) {
					R_FreeStaticTriSurf( sint->shadowVolumeTris );
					sint->shadowVolumeTris = NULL;
				}
			}
			R_FreeInteractionCullInfo( sint->cullInfo );
		}
		R_StaticFree( this->surfaces );
		this->surfaces = NULL;
	}
	this->numSurfaces = -1;
}

/*
===============
idInteraction::Unlink
===============
*/
void idInteraction::Unlink( void ) {

	// unlink from the entity's list
	if ( this->entityPrev ) {
		this->entityPrev->entityNext = this->entityNext;
	} else {
		this->entityDef->firstInteraction = this->entityNext;
	}

	if ( this->entityNext ) {
		this->entityNext->entityPrev = this->entityPrev;
	} else {
		this->entityDef->lastInteraction = this->entityPrev;
	}
	this->entityNext = this->entityPrev = NULL;

	// unlink from the light's list
	if ( this->lightPrev ) {
		this->lightPrev->lightNext = this->lightNext;
	} else {
		this->lightDef->firstInteraction = this->lightNext;
	}

	if ( this->lightNext ) {
		this->lightNext->lightPrev = this->lightPrev;
	} else {
		this->lightDef->lastInteraction = this->lightPrev;
	}
	this->lightNext = this->lightPrev = NULL;
}

/*
===============
idInteraction::UnlinkAndFree

Removes links and puts it back on the free list.
===============
*/
void idInteraction::UnlinkAndFree( void ) {

	// clear the table pointer
	idRenderWorldLocal *renderWorld = this->lightDef->world;
	bool removed = renderWorld->interactionTable.Remove(this);
	if ( !removed ) { 
		common->Error( "idInteraction::UnlinkAndFree: interaction not in table" ); 
	}

	Unlink();

	FreeSurfaces();

	// put it back on the free list
	renderWorld->interactionAllocator.Free( this );
}

/*
===============
idInteraction::MakeEmpty

Makes the interaction empty and links it at the end of the entity's and light's interaction lists.
===============
*/
void idInteraction::MakeEmpty( void ) {

	// an empty interaction has no surfaces
	numSurfaces = 0;

	Unlink();

	// relink at the end of the entity's list
	this->entityNext = NULL;
	this->entityPrev = this->entityDef->lastInteraction;
	this->entityDef->lastInteraction = this;

	if ( this->entityPrev ) {
		this->entityPrev->entityNext = this;
	} else {
		this->entityDef->firstInteraction = this;
	}

	// relink at the end of the light's list
	this->lightNext = NULL;
	this->lightPrev = this->lightDef->lastInteraction;
	this->lightDef->lastInteraction = this;

	if ( this->lightPrev ) {
		this->lightPrev->lightNext = this;
	} else {
		this->lightDef->firstInteraction = this;
	}

	flagMakeEmpty = false;
}

/*
===============
idInteraction::HasShadows
===============
*/
ID_INLINE bool idInteraction::HasShadows( void ) const {
	return ( !lightDef->parms.noShadows && !entityDef->parms.noShadow && lightDef->lightShader->LightCastsShadows() );
}

/*
===============
idInteraction::MemoryUsed

Counts up the memory used by all the surfaceInteractions, which
will be used to determine when we need to start purging old interactions.
===============
*/
int idInteraction::MemoryUsed( void ) {
	int		total = 0;

	for ( int i = 0 ; i < numSurfaces ; i++ ) {
		surfaceInteraction_t *inter = &surfaces[i];

		total += R_TriSurfMemory( inter->lightTris );
		total += R_TriSurfMemory( inter->shadowVolumeTris );
		if ( inter->shadowMapTris && inter->shadowMapTris != inter->lightTris )
			total += R_TriSurfMemory( inter->shadowMapTris );
	}
	return total;
}

/*
==================
idInteraction::CalcInteractionScissorRectangle
==================
*/
idScreenRect idInteraction::CalcInteractionScissorRectangle( const idFrustum &viewFrustum ) {
	idBounds		projectionBounds;
	idScreenRect	portalRect;
	idScreenRect	scissorRect;

	if ( r_useInteractionScissors.GetInteger() == 0 ) {
		return lightDef->viewLight->scissorRect;
	}

	if ( r_useInteractionScissors.GetInteger() < 0 ) {
		// this is the code from Cass at nvidia, it is more precise, but slower
		return R_CalcIntersectionScissor( lightDef, entityDef, tr.viewDef );
	}

	// the following is Mr.E's code
	// frustum must be initialized and valid
	if ( frustumState == idInteraction::FRUSTUM_UNINITIALIZED || frustumState == idInteraction::FRUSTUM_INVALID ) {
		return lightDef->viewLight->scissorRect;
	}
	//stgatilov: single-point frustum happens from full-zero model bounds
	//which happens when model has no surfaces at all (e.g. aasobstacle)
	if ( frustum.GetLeft() == 0.0 && frustum.GetUp() == 0.0 ) {
		scissorRect.ClearWithZ();
		return scissorRect;
	}

	// calculate scissors for the portals through which the interaction is visible
	if ( r_useInteractionScissors.GetInteger() > 1 ) {
		bool fullScissor = false;
		int viewerArea = tr.viewDef->areaNum;

		// retrieve all the areas the entity belongs to
		static const int MAX_AREAS_NUM = 32;
		int areas[MAX_AREAS_NUM], areasCnt = 0;
		for ( areaReference_t *ref = entityDef->entityRefs; ref; ref = ref->next ) {
			int area = ref->areaIdx;
			if (area == viewerArea || areasCnt == MAX_AREAS_NUM) {
				fullScissor = true;
				break;
			}
			areas[areasCnt++] = area;
		}

		// flood into areas along frustum, try to reduce interaction scissor
		portalRect = lightDef->viewLight->scissorRect;
		if (!fullScissor) {
			tr.viewDef->renderWorld->FlowShadowFrustumThroughPortals(portalRect, frustum, areas, areasCnt);
		}
	} else {
		portalRect = lightDef->viewLight->scissorRect;
	}

	// early out if the interaction is not visible through any portals
	if ( portalRect.IsEmpty() ) {
		return portalRect;
	}

	// calculate bounds of the interaction frustum projected into the view frustum
	if ( lightDef->parms.pointLight ) {
		viewFrustum.ClippedProjectionBounds( frustum, idBox( lightDef->parms.origin, lightDef->parms.lightRadius, lightDef->parms.axis ), projectionBounds );
	} else {
		viewFrustum.ClippedProjectionBounds( frustum, idBox( lightDef->frustumTris->bounds ), projectionBounds );
	}

	if ( projectionBounds.IsCleared() ) {
		return portalRect;
	}

	// derive a scissor rectangle from the projection bounds
	scissorRect = R_ScreenRectFromViewFrustumBounds( projectionBounds );

	// intersect with the portal crossing scissor rectangle
	scissorRect.Intersect( portalRect );

	if ( r_showInteractionScissors.GetInteger() > 0 ) {
		R_ShowColoredScreenRect( scissorRect, lightDef->index );
	}
	return scissorRect;
}

/*
===================
idInteraction::CullInteractionByViewFrustum
===================
*/
bool idInteraction::CullInteractionByViewFrustum( const idFrustum &viewFrustum ) {

	if ( !r_useInteractionCulling.GetBool() ) {
		return false;
	}

	if ( frustumState == idInteraction::FRUSTUM_INVALID ) {
		return false;
	}

	if ( frustumState == idInteraction::FRUSTUM_UNINITIALIZED ) {

		frustum.FromProjection( idBox( entityDef->referenceBounds, entityDef->parms.origin, entityDef->parms.axis ), lightDef->globalLightOrigin, MAX_WORLD_SIZE );

		if ( !frustum.IsValid() ) {
			frustumState = idInteraction::FRUSTUM_INVALID;
			return false;
		}

		if ( lightDef->parms.pointLight ) {
			frustum.ConstrainToBox( idBox( lightDef->parms.origin, lightDef->parms.lightRadius, lightDef->parms.axis ) );
		} else {
			frustum.ConstrainToBox( idBox( lightDef->frustumTris->bounds ) );
		}
		frustumState = idInteraction::FRUSTUM_VALID;
	}

	if ( !viewFrustum.IntersectsFrustum( frustum ) ) {
		return true;
	}

	if ( r_showInteractionFrustums.GetInteger() ) {
		static idVec4 colors[] = { colorRed, colorGreen, colorBlue, colorYellow, colorMagenta, colorCyan, colorWhite, colorPurple };
		tr.viewDef->renderWorld->DebugFrustum( colors[lightDef->index & 7], frustum, ( r_showInteractionFrustums.GetInteger() > 1 ) );
		if ( r_showInteractionFrustums.GetInteger() > 2 ) {
			tr.viewDef->renderWorld->DebugBox( colorWhite, idBox( entityDef->referenceBounds, entityDef->parms.origin, entityDef->parms.axis ) );
		}
	}
	return false;
}

/*
===================
R_CullModelBoundsToLight
===================
*/
ID_INLINE bool R_CullModelBoundsToLight( const idRenderLightLocal * light, const idBounds & localBounds, const idRenderMatrix & modelRenderMatrix ) {
	idRenderMatrix modelLightProject;
	idRenderMatrix::Multiply( light->baseLightProject, modelRenderMatrix, modelLightProject );
	return idRenderMatrix::CullBoundsToMVP( modelLightProject, localBounds, true );
}

/*
====================
idInteraction::CreateInteraction

Called when a entityDef and a lightDef are both present in a
portalArea, and might be visible.  Performs cull checking before doing the expensive
computations.

References tr.viewCount so lighting surfaces will only be created if the ambient surface is visible,
otherwise it will be marked as deferred.

The results of this are cached and valid until the light or entity change.
====================
*/
void idInteraction::CreateInteraction( const idRenderModel *model ) {
	const idMaterial *	lightShader = lightDef->lightShader;
	const idMaterial*	shader;
	bool				spectrumbypass;
	bool				interactionGenerated;
	idBounds			bounds;

	tr.pc.c_createInteractions++;

	bounds = model->Bounds( &entityDef->parms );

	// if it doesn't contact the light frustum, none of the surfaces will
	if ( R_CullModelBoundsToLight( lightDef, bounds, entityDef->modelRenderMatrix ) ) {
		//MakeEmpty();
		flagMakeEmpty = true;
		return;
	}

	// use the turbo shadow path
	shadowGen_t shadowGen = SG_DYNAMIC;

#if 0	// stgatilov #5886: we should cull large meshes using BVH, which does not support static shadow gen semantics
	// really large models, like outside terrain meshes, should use
	// the more exactly culled static shadow path instead of the turbo shadow path.
	// FIXME: this is a HACK, we should probably have a material flag.
	if ( bounds[1][0] - bounds[0][0] > 3000 ) {
		shadowGen = SG_STATIC;
	}
#endif

	//
	// create slots for each of the model's surfaces
	//
	numSurfaces = model->NumSurfaces();
	surfaces = ( surfaceInteraction_t * )R_ClearedStaticAlloc( sizeof( *surfaces ) * numSurfaces );

	interactionGenerated = false;

	// check each surface in the model
	for ( int c = 0; c < model->NumSurfaces(); c++ ) {
		const modelSurface_t	*surf;
		srfTriangles_t	*tri;

		surf = model->Surface( c );
		tri = surf->geometry;

		if ( !tri ) {
			continue;
		}

		// determine the shader for this surface, possibly by skinning
		shader = surf->material;
		shader = R_RemapShaderBySkin( shader, entityDef->parms.customSkin, entityDef->parms.customShader );

		if ( !shader ) {
			continue;
		}

		// try to cull each surface
		if ( R_CullModelBoundsToLight( lightDef, tri->bounds, entityDef->modelRenderMatrix ) ) {
			continue;
		}
		surfaceInteraction_t *sint = &surfaces[c];

		sint->shader = shader;

		// save the ambient tri pointer so we can reject lightTri interactions
		// when the ambient surface isn't in view, and we can get shared vertex
		// and shadow data from the source surface
		sint->ambientTris = tri;
		
#if 0	// duzenko: interactions disabled this way remain disabled during the main view render
		// nbohr1more: #4379 lightgem culling
		if ( !HasShadows() && !shader->IsLightgemSurf() && tr.viewDef->IsLightGem() ) { 
			continue; 
		}
#endif		

		// "invisible ink" lights and shaders
		if ( shader->Spectrum() != lightShader->Spectrum() ) {
			continue;
		}
		
		spectrumbypass = false;
		
		if ( (entityDef->parms.nospectrum != lightDef->parms.spectrum) && entityDef->parms.nospectrum > 0 ) {
            spectrumbypass = true;
		}
		
		if ( (entityDef->parms.lightspectrum != lightDef->parms.spectrum) && lightShader->IsAmbientLight() ) {
		    spectrumbypass = true;
		}			
		
		//nbohr1more: #4956 spectrum for entities
		if ( ( entityDef->parms.spectrum != lightDef->parms.spectrum ) && !spectrumbypass ) {
		     continue;
		}
		
		
		// nbohr1more: #3662 fix noFog keyword
		if ( (!shader->ReceivesFog() || entityDef->parms.noFog) && lightShader->IsFogLight() ) {
           continue;
        }

		bool genLightTris = shader->ReceivesLighting();
		// if the interaction has shadows and this surface casts a shadow
		bool genShadows = HasShadows() && shader->SurfaceCastsShadow();

		// #6571: save for parallax mapping self-shadows
		sint->blockSelfShadows = !genShadows;

		// generate a lighted surface and add it
		if ( genLightTris ) {
			if ( tri->ambientViewCount == tr.viewCount ) {
				sint->lightTris = R_CreateLightTris( entityDef, tri, lightDef, shader, sint->cullInfo );
			} else {
				// this will be calculated when sint->ambientTris is actually in view
				sint->lightTris = LIGHT_TRIS_DEFERRED;
			}
			interactionGenerated = true;
		}

		if ( genShadows ) {
			if ( lightDef->shadows == LS_STENCIL && tri->silEdges != NULL) {
				// if the light has an optimized shadow volume, don't create shadows for any models that are part of the base areas
				bool precomputedShadows = ( lightDef->parms.prelightModel && model->IsStaticWorldModel() && r_useOptimizedShadows.GetBool() );
				if ( !precomputedShadows ) {
					// this is the only place during gameplay (outside the utilities) that R_CreateShadowVolume() is called
					sint->shadowVolumeTris = R_CreateShadowVolume( entityDef, tri, lightDef, shadowGen, sint->cullInfo );
					if ( sint->shadowVolumeTris ) {
						if ( shader->Coverage() != MC_OPAQUE || ( !r_skipSuppress.GetBool() && entityDef->parms.suppressSurfaceInViewID ) ) {
							// if any surface is a shadow-casting perforated or translucent surface, or the
							// base surface is suppressed in the view (world weapon shadows) we can't use
							// the external shadow optimizations because we can see through some of the faces
							sint->shadowVolumeTris->numShadowIndexesNoCaps = sint->shadowVolumeTris->numIndexes;
							sint->shadowVolumeTris->numShadowIndexesNoFrontCaps = sint->shadowVolumeTris->numIndexes;
						}
					}
					interactionGenerated = true;
				}
			}
			else if ( lightDef->shadows == LS_MAPS ) {
				if ( sint->lightTris && sint->lightTris != LIGHT_TRIS_DEFERRED ) {
					sint->shadowMapTris = sint->lightTris;
				} else {
					sint->shadowMapTris = R_CreateLightTris( entityDef, tri, lightDef, shader, sint->cullInfo );
				}
				interactionGenerated = true;
			}
		}

		// free the cull information when it's no longer needed
		if ( sint->lightTris != LIGHT_TRIS_DEFERRED ) {
			R_FreeInteractionCullInfo( sint->cullInfo );
		}
	}

	// if none of the surfaces generated anything, don't even bother checking?
	if ( !interactionGenerated ) {
		//MakeEmpty();
		flagMakeEmpty = true;
	}
}

/*
======================
R_PotentiallyInsideInfiniteShadow

If we know that we are "off to the side" of an infinite shadow volume,
we can draw it without caps in zpass mode
======================
*/
static bool R_PotentiallyInsideInfiniteShadow( const srfTriangles_t *occluder,
        const idVec3 &localView, const idVec3 &localLight ) {
	idBounds	exp;

	// expand the bounds to account for the near clip plane, because the
	// view could be mathematically outside, but if the near clip plane
	// chops a volume edge, the zpass rendering would fail.
	float	znear = r_znear.GetFloat();
	if ( tr.viewDef->renderView.cramZNear ) {
		znear *= 0.25f;
	}
	float	stretch = znear * 2;	// in theory, should vary with FOV
	exp[0][0] = occluder->bounds[0][0] - stretch;
	exp[0][1] = occluder->bounds[0][1] - stretch;
	exp[0][2] = occluder->bounds[0][2] - stretch;
	exp[1][0] = occluder->bounds[1][0] + stretch;
	exp[1][1] = occluder->bounds[1][1] + stretch;
	exp[1][2] = occluder->bounds[1][2] + stretch;

	if ( exp.ContainsPoint( localView ) ) {
		return true;
	}
	if ( exp.ContainsPoint( localLight ) ) {
		return true;
	}

	// if the ray from localLight to localView intersects a face of the
	// expanded bounds, we will be inside the projection
	idVec3	ray = localView - localLight;

	// intersect the ray from the view to the light with the near side of the bounds
	for ( int axis = 0; axis < 3; axis++ ) {
		float	d, frac;
		idVec3	hit;

		if ( localLight[axis] < exp[0][axis] ) {
			if ( localView[axis] < exp[0][axis] ) {
				continue;
			}
			d = exp[0][axis] - localLight[axis];
			frac = d / ray[axis];
			hit = localLight + frac * ray;
			hit[axis] = exp[0][axis];
		} else if ( localLight[axis] > exp[1][axis] ) {
			if ( localView[axis] > exp[1][axis] ) {
				continue;
			}
			d = exp[1][axis] - localLight[axis];
			frac = d / ray[axis];
			hit = localLight + frac * ray;
			hit[axis] = exp[1][axis];
		} else {
			continue;
		}

		if ( exp.ContainsPoint( hit ) ) {
			return true;
		}
	}

	// the view is definitely not inside the projected shadow
	return false;
}

/*
==================
idInteraction::IsPotentiallyVisible

==================
*/
bool idInteraction::IsPotentiallyVisible( idScreenRect &shadowScissor ) {
	TRACE_CPU_SCOPE("IsPotentiallyVisible");

	viewLight_t *	vLight;
	viewEntity_t *	vEntity;

	vLight = lightDef->viewLight;
	vEntity = entityDef->viewEntity;

	shadowScissor = vLight->scissorRect;

	// duzenko: cull away interaction if light and entity are in disconnected areas
	// stgatilov #5172: there are many conditions when this should not be done, now we simply set areaNum = -1 in these cases
	if ( lightDef->areaNum != -1 ) {
		// note: this cannot be done for noshadows lights, since they light objects through closed doors!
		assert( !lightDef->parms.noShadows && lightDef->lightShader->LightCastsShadows() );
		// if no part of the model is in an area that is connected to
		// the light center (it is behind a solid, closed door), we can ignore it
		bool areasConnected = false;
		for ( areaReference_t* ref = entityDef->entityRefs; ref != NULL; ref = ref->next ) {
			if ( tr.viewDef->renderWorld->AreasAreConnected( lightDef->areaNum, ref->areaIdx, PS_BLOCK_VIEW ) ) {
				areasConnected = true;
				break;
			}
		}
		if ( !areasConnected ) {
			// can't possibly be lit
			return false;
		}
	}

	// do not waste time culling the interaction frustum if there will be no shadows
	if ( !HasShadows() ) {
		// nbohr1more: #4379 lightgem culling
		if ( !entityDef->parms.isLightgem && tr.viewDef->IsLightGem() )
			return false;

		// use the entity scissor rectangle
		shadowScissor.IntersectWithZ(vEntity->scissorRect);
		if (shadowScissor.IsEmptyWithZ())
			return false;
		return true;
	}

	// sophisticated culling does not seem to be worth it for static world models
	if ( entityDef->parms.hModel->IsStaticWorldModel() ) {
		return true;
	}

	// try to cull by loose shadow bounds
	idBounds shadowBounds;
	extern void R_ShadowBounds( const idBounds& modelBounds, const idBounds& lightBounds, const idVec3& lightOrigin, idBounds& shadowBounds );
	R_ShadowBounds( entityDef->globalReferenceBounds, lightDef->globalLightBounds, lightDef->globalLightOrigin, shadowBounds );
	// duzenko: cull away if shadow is surely out of viewport
	// note: a more precise version of such check is done later inside CullInteractionByViewFrustum
	if ( idRenderMatrix::CullBoundsToMVP( tr.viewDef->worldSpace.mvp, shadowBounds ) )
		return false;

	// duzenko: intersect light scissor with shadow bounds
	idBounds shadowProjectionBounds;
	if ( !tr.viewDef->viewFrustum.ProjectionBounds( shadowBounds, shadowProjectionBounds ) )
		return false;
	idScreenRect shadowRect = R_ScreenRectFromViewFrustumBounds( shadowProjectionBounds );
	shadowScissor.IntersectWithZ(shadowRect);
	if ( shadowScissor.IsEmptyWithZ() )
		return false;

	// try to cull the interaction by view frustum
	// this will also cull the case where the light origin is inside the
	// view frustum and the entity bounds are outside the view frustum
	if ( CullInteractionByViewFrustum( tr.viewDef->viewFrustum ) ) {
		return false;
	}

	// try to cull the interaction by portals (culled away if empty scissor is returned)
	// as a by-product, calculate more precise shadow scissor rectangle
	shadowRect = CalcInteractionScissorRectangle( tr.viewDef->viewFrustum );
	shadowScissor.Intersect(shadowRect);
	if ( shadowScissor.IsEmpty() ) {
		return false;
	}

	return true;
}

void idInteraction::LinkPreparedSurfaces() {
	for (int i = 0; i < MAX_LOCATIONS; ++i) {
		if (surfsToLink[i] == nullptr) {
			continue;
		}

		// stgatilov: memorize for debug tools that this interaction generated backend surfaces in this view
		if (i == SHADOW_LOCAL || i == SHADOW_GLOBAL) {
			viewCountGenShadowSurfs = tr.viewCount;
		} else {
			viewCountGenLightSurfs = tr.viewCount;
		}

		drawSurf_t **link = nullptr;
		switch (i) {
		case INTERACTION_TRANSLUCENT: link = &lightDef->viewLight->translucentInteractions; break;
		case INTERACTION_LOCAL: link = &lightDef->viewLight->localInteractions; break;
		case INTERACTION_GLOBAL: link = &lightDef->viewLight->globalInteractions; break;
		case SHADOW_LOCAL: link = &lightDef->viewLight->localShadows; break;
		case SHADOW_GLOBAL: link = &lightDef->viewLight->globalShadows; break;
		}

		drawSurf_t *surf = surfsToLink[i];
		drawSurf_t *end = surf;
		while (end->nextOnLight) {
			end = end->nextOnLight;
		}

		end->nextOnLight = *link;
		*link = surf;

		surfsToLink[i] = nullptr;
	}
}

/*
==================
idInteraction::AddActiveInteraction

Create and add any necessary light and shadow triangles

If the model doesn't have any surfaces that need interactions
with this type of light, it can be skipped, but we might need to
instantiate the dynamic model to find out
==================
*/
void idInteraction::AddActiveInteraction( void ) {
	TRACE_CPU_SCOPE_TEXT("AddActiveInteraction", GetTraceLabel(lightDef->parms));

	viewLight_t *	vLight;
	viewEntity_t *	vEntity;
	idScreenRect	lightScissor;
	idVec3			localLightOrigin;
	idVec3			localViewOrigin;

	vLight = lightDef->viewLight;
	vEntity = entityDef->viewEntity;

	flagMakeEmpty = false;

	// Try to cull the whole interaction away in a multitide of ways
	// Also, reduce scissor rect of the interaction if possible
	// stgatilov: depthbounds also are reduced according to bounds of shadow volume
	idScreenRect shadowScissor = vLight->scissorRect;
	if ( !IsPotentiallyVisible( shadowScissor ) )
		return;

	// We will need the dynamic surface created to make interactions, even if the
	// model itself wasn't visible.  This just returns a cached value after it
	// has been generated once in the view.
	idRenderModel *model = R_EntityDefDynamicModel( entityDef );

	if ( model == NULL || model->NumSurfaces() <= 0 ) {
		return;
	}

	// the dynamic model may have changed since we built the surface list
	if ( !IsDeferred() && entityDef->dynamicModelFrameCount != dynamicModelFrameCount ) {
		FreeSurfaces();
	}
	dynamicModelFrameCount = entityDef->dynamicModelFrameCount;

	// actually create the interaction if needed, building light and shadow surfaces as needed
	if ( IsDeferred() ) {
		CreateInteraction( model );
	}
	R_GlobalPointToLocal( vEntity->modelMatrix, lightDef->globalLightOrigin, localLightOrigin );
	R_GlobalPointToLocal( vEntity->modelMatrix, tr.viewDef->renderView.vieworg, localViewOrigin );

	// calculate the scissor as the intersection of the light and model rects
	// this is used for light triangles, but not for shadow triangles
	lightScissor = vLight->scissorRect;
	lightScissor.IntersectWithZ( vEntity->scissorRect );

	bool lightScissorsEmpty = lightScissor.IsEmpty();

	// for each surface of this entity / light interaction
	for ( int i = 0; i < numSurfaces; i++ ) {
		if ( r_singleSurface.GetInteger() >= 0 && i != r_singleSurface.GetInteger() ) 
			continue;
		surfaceInteraction_t *sint = &surfaces[i];

		// see if the base surface is visible, we may still need to add shadows even if empty
		if ( !lightScissorsEmpty && sint->ambientTris && sint->ambientTris->ambientViewCount == tr.viewCount ) {

			// make sure we have created this interaction, which may have been deferred
			// on a previous use that only needed the shadow
			if ( sint->lightTris == LIGHT_TRIS_DEFERRED ) {
				if ( sint->shadowMapTris ) {
					sint->lightTris = sint->shadowMapTris;
				} else {
					sint->lightTris = R_CreateLightTris( vEntity->entityDef, sint->ambientTris, vLight->lightDef, sint->shader, sint->cullInfo );
				}
				R_FreeInteractionCullInfo( sint->cullInfo );
			}
			
			if ( srfTriangles_t *lightTris = sint->lightTris ) {

				// try to cull before adding
				// FIXME: this may not be worthwhile. We have already done culling on the ambient,
				// but individual surfaces may still be cropped somewhat more
				if ( !R_CullLocalBox( lightTris->bounds, vEntity->modelMatrix, 5, tr.viewDef->frustum ) ) {

					// make sure the original surface has its ambient cache created
					srfTriangles_t *tri = sint->ambientTris;
					if ( !vertexCache.CacheIsCurrent( tri->ambientCache ) ) {
						if ( !R_CreateAmbientCache( tri, sint->shader->ReceivesLighting() ) ) {
							// skip if we were out of vertex memory
							continue;
						}
					}
					// reference the original surface's ambient cache
					lightTris->ambientCache = tri->ambientCache;

					// stgatilov: reuse same cache if same array is referenced
					if ( lightTris->indexes == tri->indexes && lightTris->numIndexes == tri->numIndexes ) {
						lightTris->indexCache = tri->indexCache;
					}
					// put index buffer to vertex cache if not there yet
					if ( !vertexCache.CacheIsCurrent( lightTris->indexCache ) ) {
						lightTris->indexCache = vertexCache.AllocIndex( lightTris->indexes, lightTris->numIndexes * sizeof( lightTris->indexes[0] ) );
						if ( !lightTris->indexCache.IsValid() )
							continue;
					}

					// add the surface to the light list
					const idMaterial *shader = sint->shader;
					R_GlobalShaderOverride( &shader );

					// there will only be localSurfaces if the light casts shadows and there are surfaces with NOSELFSHADOW
					if ( sint->shader->Coverage() == MC_TRANSLUCENT && sint->shader->ReceivesLighting() ) {
						PrepareLightSurf(
							INTERACTION_TRANSLUCENT, lightTris,
							vEntity, shader, lightScissor, false, sint->blockSelfShadows
						);
					} else if ( !lightDef->parms.noShadows && sint->shader->TestMaterialFlag( MF_NOSELFSHADOW ) ) {
						PrepareLightSurf(
							INTERACTION_LOCAL, lightTris,
							vEntity, shader, lightScissor, false, true
						);
					} else {
						PrepareLightSurf(
							INTERACTION_GLOBAL, lightTris,
							vEntity, shader, lightScissor, false, sint->blockSelfShadows
						);
					}
				}
			}
		}

		// the shadows will always have to be added, unless we can tell they
		// are from a surface in an unconnected area

		// check for view specific shadow suppression (player shadows, etc)
		if ( !r_skipSuppress.GetBool() ) {
			if ( entityDef->parms.suppressShadowInViewID &&
				entityDef->parms.suppressShadowInViewID == tr.viewDef->renderView.viewID ) {
				continue;
			}
			if ( entityDef->parms.suppressShadowInLightID &&
				entityDef->parms.suppressShadowInLightID == lightDef->parms.lightId ) {
				continue;
			}
		}
		if ( r_singleShadowEntity >= 0 && r_singleShadowEntity != vEntity->entityDef->index )
			continue;

		// stgatilov: should never generate both on one interaction...
		assert( !sint->shadowMapTris || !sint->shadowVolumeTris );


		if ( srfTriangles_t *shadowTris = sint->shadowVolumeTris ) {

			// cull static shadows that have a non-empty bounds
			// dynamic shadows that use the turboshadow code will not have valid
			// bounds, because the perspective projection extends them to infinity
			if ( r_useShadowCulling.GetBool() && !shadowTris->bounds.IsCleared() ) {
				if ( R_CullLocalBox( shadowTris->bounds, vEntity->modelMatrix, 5, tr.viewDef->frustum ) ) {
					continue;
				}
			}

			// copy the shadow vertexes to the vertex cache if they have been purged
			// if we are using shared shadowVertexes and letting a vertex program fix them up,
			// get the shadowCache from the parent ambient surface
			if ( !shadowTris->shadowVertexes ) {
				// the data may have been purged, so get the latest from the "home position"
				shadowTris->shadowCache = sint->ambientTris->shadowCache;
			}

			// if we have been purged, re-upload the shadowVertexes
			if ( !vertexCache.CacheIsCurrent( shadowTris->shadowCache ) ) {
				if ( shadowTris->shadowVertexes ) {
					// each interaction has unique vertexes
					R_CreatePrivateShadowCache( shadowTris );
				} else {
					R_CreateVertexProgramShadowCache( sint->ambientTris );
					shadowTris->shadowCache = sint->ambientTris->shadowCache;
				}
				// if we are out of vertex cache space, skip the interaction
				if ( !shadowTris->shadowCache.IsValid() )
					continue;
			}

			// put index buffer to vertex cache if not there yet
			if ( !vertexCache.CacheIsCurrent( shadowTris->indexCache ) ) {
				shadowTris->indexCache = vertexCache.AllocIndex( shadowTris->indexes, shadowTris->numIndexes * sizeof( shadowTris->indexes[0] ) );
				if ( !shadowTris->indexCache.IsValid() )
					continue;
			}

			// see if we can avoid using the shadow volume caps
			bool inside = R_PotentiallyInsideInfiniteShadow( sint->ambientTris, localViewOrigin, localLightOrigin );

			// stgatilov: depthbounds of shadow volumes are passed to glDepthBoundsTest in stencil shadows rendering
			// so we cannot reduce them to shadow volume geometry, and here we restore them to light's depthbounds
			idScreenRect shadowScissorLightDepthBounds = shadowScissor;
			shadowScissorLightDepthBounds.zmin = vLight->scissorRect.zmin;
			shadowScissorLightDepthBounds.zmax = vLight->scissorRect.zmax;

			if ( sint->shader->TestMaterialFlag( MF_NOSELFSHADOW ) ) {
				PrepareLightSurf(
					SHADOW_LOCAL, shadowTris,
					vEntity, NULL, shadowScissorLightDepthBounds, inside, false
				);
			} else {
				PrepareLightSurf(
					SHADOW_GLOBAL, shadowTris,
					vEntity, NULL, shadowScissorLightDepthBounds, inside, false
				);
			}
		}
		else if ( srfTriangles_t *shadowTris = sint->shadowMapTris ) {

			// make sure the original surface has its ambient cache created
			srfTriangles_t *tri = sint->ambientTris;
			if ( !vertexCache.CacheIsCurrent( shadowTris->ambientCache ) ) {
				if ( !R_CreateAmbientCache( tri, false ) ) {
					// skip if we were out of vertex memory
					continue;
				}
			}
			// we always use same vertex buffer as the original model
			shadowTris->ambientCache = tri->ambientCache;

			// stgatilov: reuse same cache if same array is referenced
			if ( shadowTris->indexes == tri->indexes && shadowTris->numIndexes == tri->numIndexes ) {
				shadowTris->indexCache = tri->indexCache;
			}
			// put index buffer to vertex cache if not there yet
			if ( !vertexCache.CacheIsCurrent( shadowTris->indexCache ) ) {
				shadowTris->indexCache = vertexCache.AllocIndex( shadowTris->indexes, shadowTris->numIndexes * sizeof( shadowTris->indexes[0] ) );
				if ( !shadowTris->indexCache.IsValid() )
					continue;
			}

			// add the surface to the shadow maps list
			const idMaterial *shader = sint->shader;
			R_GlobalShaderOverride( &shader );
			// note: material is needed for shadow maps to handle perforated surfaces

			if ( sint->shader->TestMaterialFlag( MF_NOSELFSHADOW ) ) {
				PrepareLightSurf(
					SHADOW_LOCAL, shadowTris,
					vEntity, shader, shadowScissor, false, false
				);
			} else {
				PrepareLightSurf(
					SHADOW_GLOBAL, shadowTris,
					vEntity, shader, shadowScissor, false, false
				);
			}
		}

		// stgatilov #5880: there is no reason to have different geometry for lighttris and shadow maps
		// maybe it will appear in future, but for now let's check that we don't duplicate stuff
		if ( sint->lightTris && sint->lightTris != LIGHT_TRIS_DEFERRED && sint->shadowMapTris ) {
			assert( sint->lightTris->ambientCache == sint->shadowMapTris->ambientCache );
			assert( sint->lightTris->indexCache == sint->shadowMapTris->indexCache );
		}
	}
}

/*
===================
R_ShowInteractionMemory_f
===================
*/
void R_ShowInteractionMemory_f( const idCmdArgs &args ) {
	int total = 0;
	int entities = 0;
	int interactions = 0;
	int deferredInteractions = 0;
	int emptyInteractions = 0;
	int lightTris = 0;
	int lightTriVerts = 0;
	int lightTriIndexes = 0;
	int shadowTris = 0;
	int shadowTriVerts = 0;
	int shadowTriIndexes = 0;

	for ( int i = 0; i < tr.primaryWorld->entityDefs.Num(); i++ ) {
		idRenderEntityLocal	*def = tr.primaryWorld->entityDefs[i];
		if ( !def ) {
			continue;
		}
		if ( def->firstInteraction == NULL ) {
			continue;
		}
		entities++;

		for ( idInteraction *inter = def->firstInteraction; inter != NULL; inter = inter->entityNext ) {
			interactions++;
			total += inter->MemoryUsed();

			if ( inter->IsDeferred() ) {
				deferredInteractions++;
				continue;
			}
			if ( inter->IsEmpty() ) {
				emptyInteractions++;
				continue;
			}

			for ( int j = 0; j < inter->numSurfaces; j++ ) {
				surfaceInteraction_t *srf = &inter->surfaces[j];

				if ( srf->lightTris && srf->lightTris != LIGHT_TRIS_DEFERRED ) {
					lightTris++;
					lightTriVerts += srf->lightTris->numVerts;
					lightTriIndexes += srf->lightTris->numIndexes;
				}
				if ( srf->shadowVolumeTris ) {
					shadowTris++;
					shadowTriVerts += srf->shadowVolumeTris->numVerts;
					shadowTriIndexes += srf->shadowVolumeTris->numIndexes;
				}
				if ( srf->shadowMapTris && srf->shadowMapTris != srf->lightTris ) {
					// TODO: where should be put this case?...
					lightTris++;
					lightTriVerts += srf->shadowMapTris->numVerts;
					lightTriIndexes += srf->shadowMapTris->numIndexes;
				}
			}
		}
	}
	common->Printf( "%i entities with %i total interactions totalling %ik\n", entities, interactions, total / 1024 );
	common->Printf( "%i deferred interactions, %i empty interactions\n", deferredInteractions, emptyInteractions );
	common->Printf( "%5i indexes %5i verts in %5i light tris\n", lightTriIndexes, lightTriVerts, lightTris );
	common->Printf( "%5i indexes %5i verts in %5i shadow tris\n", shadowTriIndexes, shadowTriVerts, shadowTris );
}
