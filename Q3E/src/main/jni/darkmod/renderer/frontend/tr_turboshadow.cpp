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
#include "idlib/containers/BitArray.h"

int	c_turboUsedVerts;
int c_turboUnusedVerts;


/*
=====================
R_CreateVertexProgramTurboShadowVolume

are dangling edges that are outside the light frustum still making planes?
=====================
*/
srfTriangles_t *R_CreateVertexProgramTurboShadowVolume( const idRenderEntityLocal *ent, 
														const srfTriangles_t *tri, const idRenderLightLocal *light,
														srfCullInfo_t &cullInfo ) {
	int		i, j;
	srfTriangles_t	*newTri;
	silEdge_t	*sil;
	const glIndex_t *indexes;
	const byte *facing;

	R_CalcInteractionFacing( ent, tri, light, cullInfo );
	if ( r_useShadowProjectedCull.GetBool() ) {
		R_CalcInteractionCullBits( ent, tri, light, cullInfo );
	}

	int numFaces = tri->numIndexes / 3;
	int	numShadowingFaces = 0;
	facing = cullInfo.facing;

	// if all the triangles are inside the light frustum
	if ( cullInfo.cullBits == LIGHT_CULL_ALL_FRONT || !r_useShadowProjectedCull.GetBool() ) {

		// count the number of shadowing faces
		for ( i = 0; i < numFaces; i++ ) {
			numShadowingFaces += facing[i];
		}
		numShadowingFaces = numFaces - numShadowingFaces;

	} else {

		// make all triangles that are outside the light frustum "facing", so they won't cast shadows
		indexes = tri->indexes;
		byte *modifyFacing = cullInfo.facing;
		const byte *cullBits = cullInfo.cullBits;
		for ( j = i = 0; i < tri->numIndexes; i += 3, j++ ) {
			if ( !modifyFacing[j] ) {
				int	i1 = indexes[i+0];
				int	i2 = indexes[i+1];
				int	i3 = indexes[i+2];
				if ( cullBits[i1] & cullBits[i2] & cullBits[i3] ) {
					modifyFacing[j] = 1;
				} else {
					numShadowingFaces++;
				}
			}
		}
	}

	if ( !numShadowingFaces ) {
		// no faces are inside the light frustum and still facing the right way
		return NULL;
	}

	// shadowVerts will be NULL on these surfaces, so the shadowVerts will be taken from the ambient surface
	newTri = R_AllocStaticTriSurf();

	newTri->numVerts = tri->numVerts * 2;

	// alloc the max possible size
#ifdef USE_TRI_DATA_ALLOCATOR
	R_AllocStaticTriSurfIndexes( newTri, ( numShadowingFaces + tri->numSilEdges ) * 6 );
	glIndex_t *tempIndexes = newTri->indexes;
	glIndex_t *shadowIndexes = newTri->indexes;
#else
	glIndex_t *tempIndexes = (glIndex_t *)_alloca16( tri->numSilEdges * 6 * sizeof( tempIndexes[0] ) );
	glIndex_t *shadowIndexes = tempIndexes;
#endif

	// create new triangles along sil planes
	for ( sil = tri->silEdges, i = tri->numSilEdges; i > 0; i--, sil++ ) {

		int f1 = facing[sil->p1];
		int f2 = facing[sil->p2];

		if ( !( f1 ^ f2 ) ) {
			continue;
		}

		int v1 = sil->v1 << 1;
		int v2 = sil->v2 << 1;

		// set the two triangle winding orders based on facing
		// without using a poorly-predictable branch

		shadowIndexes[0] = v1;
		shadowIndexes[1] = v2 ^ f1;
		shadowIndexes[2] = v2 ^ f2;
		shadowIndexes[3] = v1 ^ f2;
		shadowIndexes[4] = v1 ^ f1;
		shadowIndexes[5] = v2 ^ 1;

		shadowIndexes += 6;
	}

	int	numShadowIndexes = shadowIndexes - tempIndexes;

	// we aren't bothering to separate front and back caps on these
	newTri->numIndexes = newTri->numShadowIndexesNoFrontCaps = numShadowIndexes + numShadowingFaces * 6;
	newTri->numShadowIndexesNoCaps = numShadowIndexes;
	newTri->shadowCapPlaneBits = SHADOW_CAP_INFINITE;

#ifdef USE_TRI_DATA_ALLOCATOR
	// decrease the size of the memory block to only store the used indexes
	R_ResizeStaticTriSurfIndexes( newTri, newTri->numIndexes );
#else
	// allocate memory for the indexes
	R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );
	// copy the indexes we created for the sil planes
	SIMDProcessor->Memcpy( newTri->indexes, tempIndexes, numShadowIndexes * sizeof( tempIndexes[0] ) );
#endif

	// these have no effect, because they extend to infinity
	newTri->bounds.Clear();

	// put some faces on the model and some on the distant projection
	indexes = tri->indexes;
	shadowIndexes = newTri->indexes + numShadowIndexes;
	for ( i = 0, j = 0; i < tri->numIndexes; i += 3, j++ ) {
		if ( facing[j] ) {
			continue;
		}

		int i0 = indexes[i+0] << 1;
		shadowIndexes[2] = i0;
		shadowIndexes[3] = i0 ^ 1;
		int i1 = indexes[i+1] << 1;
		shadowIndexes[1] = i1;
		shadowIndexes[4] = i1 ^ 1;
		int i2 = indexes[i+2] << 1;
		shadowIndexes[0] = i2;
		shadowIndexes[5] = i2 ^ 1;

		shadowIndexes += 6;
	}

	return newTri;
}

/*
=====================
R_CreateVertexProgramBvhShadowVolume

stgatilov #5886: Similar to "turbo shadow volume", but uses BVH tree for acceleration
=====================
*/
srfTriangles_t *R_CreateVertexProgramBvhShadowVolume( const idRenderEntityLocal *ent, const srfTriangles_t *tri, const idRenderLightLocal *light ) {
	// transform light geometry into model space
	idVec3 localLightOrigin;
	idPlane localLightFrustum[6];
	R_GlobalPointToLocal( ent->modelMatrix, light->globalLightOrigin, localLightOrigin );
	for ( int i = 0; i < 6; i++ )
		R_GlobalPlaneToLocal( ent->modelMatrix, -light->frustum[i], localLightFrustum[i] );

	// filter triangles:
	//   1) inside light frustum
	//   2) backfacing
	// such triangles are called "shadowing" below
	idFlexList<bvhTriRange_t, 128> intervals;
	R_CullBvhByFrustumAndOrigin(
		tri->bounds, tri->bvhNodes,
		localLightFrustum, -1, localLightOrigin,
		0, r_modelBvhShadowsGranularity.GetInteger(),
		intervals
	);

	// note: the bitset persists between calls
	// its memory is never deallocated!
	// we need to avoid clearing whole bitset on every call
	thread_local static idBitArrayDefault shadowing;
	int bsSize = tri->numIndexes / 3 + 1;
	if (shadowing.Num() < bsSize) {
		// reallocate bitset exponentially, clear all contents
		shadowing.Init(idMath::Imax(shadowing.Num() * 2 + 10, bsSize));
		shadowing.SetBitsSameAll(false);
	}
	bool hasShadowing = false;

	// uncertain intervals should usually be short
	idFlexList<byte, 1024> triCull, triFacing;
	// go through all intervals and fill shadowing data
	for ( int i = 0; i < intervals.Num(); i++ ) {
		int beg = intervals[i].beg;
		int len = intervals[i].end - intervals[i].beg;
		int info = intervals[i].info;

		if ( info == BVH_TRI_SURELY_MATCH ) {
			// all triangles surely match
			shadowing.SetBitsTrue(beg, beg + len);
			hasShadowing |= (len > 0);
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

		// see which triangles are backfacing
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

		int t = 0;
#ifdef __SSE2__
		for ( ; t + 16 <= len; t += 16 ) {
			// SSE-optimized version (see main version below for tail)
			__m128i xCull = _mm_loadu_si128( (__m128i*) &triCull[t] );
			__m128i xFacing = _mm_loadu_si128( (__m128i*) &triFacing[t] );
			__m128i xShadow = _mm_cmpeq_epi8(xCull, _mm_setzero_si128());
			xShadow = _mm_and_si128( xShadow, _mm_cmpeq_epi8(xFacing, _mm_setzero_si128()) );
			uint32_t shadow = _mm_movemask_epi8(xShadow);
			shadowing.SetBitsWord(beg + t, beg + t + 16, shadow);
			hasShadowing |= (shadow != 0);
		}
#endif
		for ( ; t < len; t++ ) {
			// detect if triangle is shadowing, and save this data
			bool shadow = ( triCull[t] == 0 && triFacing[t] == false );
			shadowing.SetBit(beg + t, shadow);
			hasShadowing |= int(shadow);
		}
	}

	if ( !hasShadowing ) {
		// no shadowing triangles -> no shadow geometry
		return nullptr;
	}

	srfTriangles_t *newTri = R_AllocStaticTriSurf();
	// shadowVerts will be NULL on these surfaces, so the shadowVerts will be taken from the ambient surface
	newTri->numVerts = tri->numVerts * 2;
	// these have no effect, because they extend to infinity
	newTri->bounds.Clear();

	int totalShadowingTris = 0;

	// for each shadowing triangles, check if its edges are silhouette (separate shadowing from non-shadowing)
	// all silhouette edges with proper orientation are collected in this list
	idFlexList<glIndex_t, 1024> silEdges;

	for ( int i = 0; i < intervals.Num(); i++ ) {
		bvhTriRange_t r = intervals[i];

		for ( int t = r.beg; t < r.end; t++ ) {
			// only consider shadowing triangles
			if ( !shadowing.IfBit( t ) )
				continue;
			totalShadowingTris++;

			// index of adjacent triangle (along edge opposite to s-th vertex)
			int adj0 = tri->adjTris[3 * t + 0];
			int adj1 = tri->adjTris[3 * t + 1];
			int adj2 = tri->adjTris[3 * t + 2];

			// see if adjacent tri is also shadowing
			bool edge0 = !shadowing.IfBit( adj0 );
			bool edge1 = !shadowing.IfBit( adj1 );
			bool edge2 = !shadowing.IfBit( adj2 );

			// this is edge from shadowing tri to non-shadowing tri = silhouette edge
			// store the edge oriented in such way that shadowing triangle is to the left of it
			int add[6], ak = 0;
			if ( edge0 ) {
				static const int s = 0;
				add[ak++] = tri->indexes[3 * t + (s+1) % 3];
				add[ak++] = tri->indexes[3 * t + (s+2) % 3];
			}
			if ( edge1 ) {
				static const int s = 1;
				add[ak++] = tri->indexes[3 * t + (s+1) % 3];
				add[ak++] = tri->indexes[3 * t + (s+2) % 3];
			}
			if ( edge2 ) {
				static const int s = 2;
				add[ak++] = tri->indexes[3 * t + (s+1) % 3];
				add[ak++] = tri->indexes[3 * t + (s+2) % 3];
			}
			// commit: add found silhouette edges to array
			int base = silEdges.Num();
			silEdges.SetNum(base + ak);
			for (int p = 0; p < ak; p++)
				silEdges[base + p] = add[p];
		}
	}

	// we know silhouette edges and number of shadowing tris -> can set all numbers
	newTri->numIndexes = newTri->numShadowIndexesNoFrontCaps = 3 * silEdges.Num() + totalShadowingTris * 6;
	newTri->numShadowIndexesNoCaps = 3 * silEdges.Num();
	newTri->shadowCapPlaneBits = SHADOW_CAP_INFINITE;

	// allocate memory for new shadow index buffer
	R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );
	int pos = 0;

	for ( int i = 0; i < silEdges.Num(); i += 2 ) {
		// generate a quad from each silhouette edge
		int v1 = silEdges[i + 0];
		int v2 = silEdges[i + 1];
		newTri->indexes[pos++] = 2 * v2 + 0;
		newTri->indexes[pos++] = 2 * v1 + 1;
		newTri->indexes[pos++] = 2 * v1 + 0;
		newTri->indexes[pos++] = 2 * v2 + 0;
		newTri->indexes[pos++] = 2 * v2 + 1;
		newTri->indexes[pos++] = 2 * v1 + 1;
	}
	assert( pos == newTri->numShadowIndexesNoCaps );

	for ( int i = 0; i < intervals.Num(); i++ ) {
		bvhTriRange_t r = intervals[i];

		for ( int t = r.beg; t < r.end; t++ ) {
			if ( !shadowing.IfBit( t ) )
				continue;

			// this tri is shadowing
			// add it to front and back caps
			int a = tri->indexes[3 * t + 0];
			int b = tri->indexes[3 * t + 1];
			int c = tri->indexes[3 * t + 2];
			newTri->indexes[pos++] = 2 * c + 0;
			newTri->indexes[pos++] = 2 * b + 0;
			newTri->indexes[pos++] = 2 * a + 0;
			newTri->indexes[pos++] = 2 * a + 1;
			newTri->indexes[pos++] = 2 * b + 1;
			newTri->indexes[pos++] = 2 * c + 1;
		}

		// clear bitset on interval: it would be all false by next call
		shadowing.SetBitsFalse(r.beg, r.end);
	}
	assert( pos == newTri->numIndexes );

	return newTri;
}
