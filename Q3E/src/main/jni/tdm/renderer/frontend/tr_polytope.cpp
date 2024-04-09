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

#define MAX_POLYTOPE_PLANES		6

/*
=====================
R_PolytopeSurface

Generate vertexes and indexes for a polytope, and optionally returns the polygon windings.
The positive sides of the planes will be visible.
=====================
*/
srfTriangles_t *R_PolytopeSurface( int numPlanes, const idPlane *planes, idWinding *windings ) {
	int i, j;
	srfTriangles_t *tri;
	idFixedWinding planeWindings[MAX_POLYTOPE_PLANES];
	int numVerts, numIndexes;

	if ( numPlanes > MAX_POLYTOPE_PLANES ) {
		common->Error( "R_PolytopeSurface: more than %d planes", MAX_POLYTOPE_PLANES );
	}

	numVerts = 0;
	numIndexes = 0;
	for ( i = 0; i < numPlanes; i++ ) {
		const idPlane &plane = planes[i];
		idFixedWinding &w = planeWindings[i];

		w.BaseForPlane( plane );
		for ( j = 0; j < numPlanes; j++ ) {
			const idPlane &plane2 = planes[j];
			if ( j == i ) {
				continue;
			} else if ( !w.ClipInPlace( -plane2, ON_EPSILON ) ) {
				break;
			}
		}
		if ( w.GetNumPoints() <= 2 ) {
			continue;
		}
		numVerts += w.GetNumPoints();
		numIndexes += ( w.GetNumPoints() - 2 ) * 3;
	}

	// allocate the surface
	tri = R_AllocStaticTriSurf();
	R_AllocStaticTriSurfVerts( tri, numVerts );
	R_AllocStaticTriSurfIndexes( tri, numIndexes );

	// copy the data from the windings
	for ( i = 0; i < numPlanes; i++ ) {
		idFixedWinding &w = planeWindings[i];
		if ( !w.GetNumPoints() ) {
			continue;
		}
		for ( j = 0 ; j < w.GetNumPoints() ; j++ ) {
			tri->verts[tri->numVerts + j ].Clear();
			tri->verts[tri->numVerts + j ].xyz = w[j].ToVec3();
		}

		for ( j = 1 ; j < w.GetNumPoints() - 1 ; j++ ) {
			tri->indexes[ tri->numIndexes + 0 ] = tri->numVerts;
			tri->indexes[ tri->numIndexes + 1 ] = tri->numVerts + j;
			tri->indexes[ tri->numIndexes + 2 ] = tri->numVerts + j + 1;
			tri->numIndexes += 3;
		}
		tri->numVerts += w.GetNumPoints();

		// optionally save the winding
		if ( windings ) {
			//windings[i] = new idWinding( w.GetNumPoints() );
			windings[i] = w;
		}
	}

	R_BoundTriSurf( tri );

	return tri;
}

/*
=====================
R_PolytopeSurfaceFrustumLike

stgatilov: equivalent to R_PolytopeSurface, except that:
  1) There must be six planes forming a polytope topologically equivalent to frustum.
    The order of planes must be exactly as in R_SetLightFrustum: s=0, t=0, s=1, t=1, near, far
  2) The returned windings are always perfectly watertight.
  3) If specified frustum is unbounded, then false is returned, and contents of outputs is undefined.
  4) All output parameters are optional.
=====================
*/
bool R_PolytopeSurfaceFrustumLike( const idPlane planes[6], idVec3 vertices[8], idWinding windings[6], srfTriangles_t* *surface ) {
	// (coord idx = s,t,depth) x (side = min,max)
	static const int PLANE_IDX[3][2] = {{0, 2}, {1, 3}, {4, 5}};	//yeah, R_SetLightFrustum order is weird

	// compute all vertices as planes intersections
	idVec3 verts[8];
	for (int x = 0; x < 2; x++)
		for (int y = 0; y < 2; y++)
			for (int z = 0; z < 2; z++) {
				const idPlane &a = planes[PLANE_IDX[0][x]];
				const idPlane &b = planes[PLANE_IDX[1][y]];
				const idPlane &c = planes[PLANE_IDX[2][z]];

				idMat3 matr(a.Normal(), b.Normal(), c.Normal());
				matr.TransposeSelf();	//now each normal is a row
				idVec3 right(a.Dist(), b.Dist(), c.Dist());
				if ( !matr.InverseSelf() )
					return false;

				int v = x + y*2 + z*4;
				verts[v] = matr * right;

				for (int p = 0; p < 6; p++) {
					float dist = planes[p].Distance(verts[v]);
					if (dist > ON_EPSILON)	//if outside frustum
						return false;	//unbounded
				}
			}

	//check if X/Y/Z planes are oriented right-handedly
	bool leftHanded = false;
	{
		const idPlane &a = planes[PLANE_IDX[0][1]];
		const idPlane &b = planes[PLANE_IDX[1][1]];
		const idPlane &c = planes[PLANE_IDX[2][1]];
		float triple = a.Normal().Cross(b.Normal()) * c.Normal();
		if (triple < 0.0f)
			leftHanded = true;
	}

	// create index set
	int rectIds[6][4];
	for (int c = 0; c < 3; c++) {
		int a = (c + 1) % 3;
		int b = (c + 2) % 3;
		for (int s = 0; s < 2; s++) {
			int *ids = rectIds[PLANE_IDX[c][s]];
			ids[0] = (s << c);
			ids[1] = (s << c) + (1 << a);
			ids[2] = (s << c) + (1 << a) + (1 << b);
			ids[3] = (s << c) + (1 << b);
			// deduced via experimentation on test_5815_spotlights
			if (s ^ int(leftHanded)) {
				idSwap(ids[0], ids[3]);
				idSwap(ids[1], ids[2]);
			}
		}
	}

	if (vertices) {
		// save vertices
		memcpy(vertices, verts, sizeof(verts));
	}


	if (windings) {
		// fill windings
		for (int p = 0; p < 6; p++) {
			idWinding &w = windings[p];
			const int *ids = rectIds[p];
			w.SetNumPoints(4);
			for (int i = 0; i < 4; i++)
				w[i] = verts[ids[i]];
#if _DEBUG
			// debug: ensure that windings are oriented CCW when looking from the side with plane normal
			float area = w.GetArea();
			if (area > 0.1f) {	// ignore singular windings
				idPlane ccwPlane;
				w.GetPlane(ccwPlane);
				float dot = ccwPlane.Normal() * planes[p].Normal();
				assert(dot > 0.0);	// should be about +1
			}
#endif
		}
	}

	if (surface) {
		// create surface mesh
		srfTriangles_t *tri = R_AllocStaticTriSurf();
		tri->numVerts = 8;
		tri->numIndexes = 36;
		R_AllocStaticTriSurfVerts( tri, tri->numVerts );
		R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );

		for (int v = 0; v < 8; v++) {
			tri->verts[v].Clear();
			tri->verts[v].xyz = verts[v];
		}
		for (int p = 0; p < 6; p++) {
			const int *ids = rectIds[p];
			tri->indexes[6 * p + 0] = ids[0];
			tri->indexes[6 * p + 1] = ids[1];
			tri->indexes[6 * p + 2] = ids[2];
			tri->indexes[6 * p + 3] = ids[0];
			tri->indexes[6 * p + 4] = ids[2];
			tri->indexes[6 * p + 5] = ids[3];
		}

		R_BoundTriSurf( tri );

		*surface = tri;
	}

	return true;
}
