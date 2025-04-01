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



#include "dmap.h"
#include "containers/DisjointSets.h"
#include "containers/FlexList.h"

#include "tjunctionfixer.h"	// new algo!
static TJunctionFixer newTjuncAlgorithm;

/*

  T junction fixing never creates more xyz points, but
  new vertexes will be created when different surfaces
  cause a fix

  The vertex cleaning accomplishes two goals: removing extranious low order
  bits to avoid numbers like 1.000001233, and grouping nearby vertexes
  together.  Straight truncation accomplishes the first foal, but two vertexes
  only a tiny epsilon apart could still be spread to different snap points.
  To avoid this, we allow the merge test to group points together that
  snapped to neighboring integer coordinates.

  Snaping verts can drag some triangles backwards or collapse them to points,
  which will cause them to be removed.
  

  When snapping to ints, a point can move a maximum of sqrt(3)/2 distance
  Two points that were an epsilon apart can then become sqrt(3) apart

  A case that causes recursive overflow with point to triangle fixing:

               A
	C            D
	           B

  Triangle ABC tests against point D and splits into triangles ADC and DBC
  Triangle DBC then tests against point A again and splits into ABC and ADB
  infinite recursive loop


  For a given source triangle
	init the no-check list to hold the three triangle hashVerts

  recursiveFixTriAgainstHash

  recursiveFixTriAgainstHashVert_r
	if hashVert is on the no-check list
		exit
	if the hashVert should split the triangle
		add to the no-check list
		recursiveFixTriAgainstHash(a)
		recursiveFixTriAgainstHash(b)

*/

#define	SNAP_FRACTIONS	32
//#define	SNAP_FRACTIONS	8
//#define	SNAP_FRACTIONS	1

#define	VERTEX_EPSILON	( 1.0 / SNAP_FRACTIONS )
#define	COLINEAR_EPSILON	( 1.8 * VERTEX_EPSILON )
#define	VERTEX_EPSILON_NEW	COLINEAR_EPSILON

#define	HASH_BINS	16

static idBounds	hashBounds;
static idVec3	hashScale;
static hashVert_t	*hashVerts[HASH_BINS][HASH_BINS][HASH_BINS];
static int		numHashVerts, numTotalVerts;
static int		hashIntMins[3], hashIntScale[3];

//stgatilov: equivalence clusters (only used when dmap_fixVertexSnappingTjunc = 2)
struct HashVertexRef {
	hashVert_t *hv;
	const hashVert_t* *ref;
	idVec3 *v;
};
static idList<HashVertexRef> allVertRefs;
static idList<int> allVertDsu;

idCVar dmap_fixVertexSnappingTjunc(
	"dmap_fixVertexSnappingTjunc", "2", CVAR_INTEGER | CVAR_SYSTEM,
	"Controls how vertex snapping in T-junctions removal works (see #5486):\n"
	"  0 - can create very close output vertices\n"
	"      (default in TDM 2.07 and before)\n"
	"  1 - distance between output vertices is no less than 1/32\n"
	"  2 - cluster merge: input vertices closer than 1/32\n"
	"      are surely snapped to the same output vertex\n"
	"      (default in TDM 2.10 and after)",
	0, 2
);
idCVar dmap_disableCellSnappingTjunc(
	"dmap_disableCellSnappingTjunc", "0", CVAR_BOOL | CVAR_SYSTEM,
	"Disables unconditional snapping of all coordinates to [integer/64].\n"
	"The vertices still snap to each other if they are close.\n"
	"This is behavior change ONLY in TDM 2.10"
);
idCVar dmap_tjunctionsAlgorithm(
	"dmap_tjunctionsAlgorithm", "1", CVAR_BOOL | CVAR_SYSTEM,
	"Which algorithm to use for merging close verts and fixing T-junctions:\n"
	"  0 --- old algorithm from Doom 3\n"
	"  1 --- new fast algorithm\n"
	"Old algorithm depends on more cvars, while the new one has new defaults:\n"
	"  dmap_dontSplitWithFuncStaticVertices 1\n"
	"  dmap_fixVertexSnappingTjunc 2\n"
	"  dmap_disableCellSnappingTjunc 1\n"
	"This is behavior change in TDM 2.11"
);


/*
===============
GetHashVert

Also modifies the original vert to the snapped value
===============
*/
void GetHashVert( idVec3 &v, const hashVert_s* &ref ) {
	int		i;
	int		iv[3];
	int		blocks[2][3];
	int		block[3];
	hashVert_t	*hv;

	numTotalVerts++;

	for ( i = 0 ; i < 3 ; i++ ) {
		// snap the vert to integral values
		iv[i] = floor( ( v[i] + 0.5/SNAP_FRACTIONS ) * SNAP_FRACTIONS );

		// find main hash cell: the one which contains this snap-cell
		int x = ( iv[i] - hashIntMins[i] ) / hashIntScale[i];
		block[i] = idMath::ClampInt(0, HASH_BINS - 1, x);

		if (dmap_fixVertexSnappingTjunc.GetInteger() >= 1) {
			// check all hash cells which can contain snappable vertices
			// i.e. the ones which differ by at most 1 snap-cell
			// note: in most cases, only one hash cell needs to be checked
			int l = ( iv[i] - 1 - hashIntMins[i] ) / hashIntScale[i];
			int r = ( iv[i] + 1 - hashIntMins[i] ) / hashIntScale[i];
			blocks[0][i] = idMath::ClampInt(0, HASH_BINS - 1, l);
			blocks[1][i] = idMath::ClampInt(0, HASH_BINS - 1, r);
		}
		else {
			// only check the hash cell containing this snap-area
			// this could still fail to find a near neighbor right at the hash block boundary
			blocks[0][i] = blocks[1][i] = block[i];
		}
	}

	int idx = -1;
	if (dmap_fixVertexSnappingTjunc.GetInteger() == 2) {
		//register new vertex and external pointers
		idx = allVertRefs.AddGrow({nullptr, &ref, &v});
		int q = allVertDsu.AddGrow(idx);
		assert(q == idx);
		assert(numHashVerts == idx);
	}

	// see if a vertex near enough already exists
	for ( int a = blocks[0][0] ; a <= blocks[1][0] ; a++ ) {
		for ( int b = blocks[0][1] ; b <= blocks[1][1] ; b++ ) {
			for ( int c = blocks[0][2] ; c <= blocks[1][2] ; c++ ) {
				for ( hv = hashVerts[a][b][c] ; hv ; hv = hv->next ) {
					for ( i = 0 ; i < 3 ; i++ ) {
						int	d;
						d = hv->iv[i] - iv[i];
						if ( d < -1 || d > 1 ) {
							break;
						}
					}
					if ( i == 3 ) {
						if (dmap_fixVertexSnappingTjunc.GetInteger() == 2) {
							//mark these vertices as equivalent, and continue
							idDisjointSets::Merge(allVertDsu, idx, hv->idx);
						}
						else {
							VectorCopy( hv->v, v );
							ref = hv;
							return;
						}
					}
				}
			}
		}
	}

	// create a new one 
	hv = (hashVert_t *)Mem_Alloc( sizeof( *hv ) );

	hv->idx = idx;
	if (idx >= 0)
		allVertRefs[idx].hv = hv;

	hv->next = hashVerts[block[0]][block[1]][block[2]];
	hashVerts[block[0]][block[1]][block[2]] = hv;

	hv->iv[0] = iv[0];
	hv->iv[1] = iv[1];
	hv->iv[2] = iv[2];

	if (dmap_disableCellSnappingTjunc.GetBool())
		hv->v = v;
	else {
		hv->v[0] = (float)iv[0] / SNAP_FRACTIONS;
		hv->v[1] = (float)iv[1] / SNAP_FRACTIONS;
		hv->v[2] = (float)iv[2] / SNAP_FRACTIONS;
		VectorCopy( hv->v, v );
	}

	numHashVerts++;

	ref = hv;
}

/*
==================
HashVertsFinalize

Merge clusters of indistinguishable hash vertices.
Must be called after all GetHashVert and HashVertsFinalize calls are finished.
==================
*/
static void HashVertsFinalize() {
	if (dmap_fixVertexSnappingTjunc.GetInteger() != 2)
		return;

	//all close vertices are marked as equivalent
	//so we can obtain equivalence clusters now
	idDisjointSets::CompressAll(allVertDsu);

	//build vertex lists (by counting sort)
	int n = allVertDsu.Num();
	idFlexList<int, 1024> cnt;
	cnt.SetNum(n);
	memset(cnt.Ptr(), 0, n * sizeof(cnt[0]));
	for (int i = 0; i < n; i++)
		cnt[allVertDsu[i]]++;		//histogram
	int sum = 0;
	for (int i = 0; i < n; i++) {
		int nsum = sum + cnt[i];	//prefix sums
		cnt[i] = sum;
		sum = nsum;
	}
	idFlexList<HashVertexRef*, 1024> lists;
	lists.SetNum(n);
	for (int i = 0; i < n; i++)
		lists[cnt[allVertDsu[i]]++] = &allVertRefs[i];	//scatter

	//merge clusters
	hashVert_t *mergedList = nullptr;
	int k = 0;
	int beg = 0;
	for (int i = 0; i < n; i++) {
		int end = cnt[i];
		if (end - beg == 0)
			continue;	//no cluster here
		k++;

		hashVert_t *hv;
		if (end - beg == 1) {
			//single-vertex cluster: already processed
			hv = lists[beg]->hv;
		}
		else {
			//compute center of cluster's bounding box
			idBounds bbox;
			bbox.Clear();
			for (int j = beg; j < end; j++)
				bbox.AddPoint(*lists[j]->v);
			idVec3 ctr = bbox.GetCenter();
			//create new merged vertex at center's span location
			hv = (hashVert_t *)Mem_Alloc(sizeof(*hv));
			hv->idx = -1;
			hv->v = ctr;
			for (int d = 0; d < 3; d++) {
				hv->iv[d] = floor( ( ctr[d] + 0.5/SNAP_FRACTIONS ) * SNAP_FRACTIONS );
				if (!dmap_disableCellSnappingTjunc.GetBool())
					hv->v[d] = (float)hv->iv[d] / SNAP_FRACTIONS;
			}
			//replace all references to cluster vertices
			for (int j = beg; j < end; j++) {
				Mem_Free(lists[j]->hv);
				lists[j]->hv = hv;
				*lists[j]->v = hv->v;
				*lists[j]->ref = hv;
			}
		}

		//add cluster vertex to new list
		hv->next = mergedList;
		mergedList = hv;

		beg = end;
	}

	//spread resulting hash vertices back into hash cells
	//so that FixTriangleAgainstHash works properly
	memset(hashVerts, 0, sizeof(hashVerts));
	for (hashVert_t *hv = mergedList, *hvnext; hv; hv = hvnext) {
		hvnext = hv->next;
		//find hash cell which contains this vertex
		int block[3];
		for (int i = 0; i < 3; i++) {
			int x = ( hv->iv[i] - hashIntMins[i] ) / hashIntScale[i];
			block[i] = idMath::ClampInt(0, HASH_BINS - 1, x);
		}
		//add hash vertex to this cell
		hv->next = hashVerts[block[0]][block[1]][block[2]];
		hashVerts[block[0]][block[1]][block[2]] = hv;
	}
}


/*
==================
HashBlocksForTri

Returns an inclusive bounding box of hash
bins that should hold the triangle
==================
*/
static void HashBlocksForTri( const mapTri_t *tri, int blocks[2][3] ) {
	idBounds	bounds;
	int			i;

	bounds.Clear();
	bounds.AddPoint( tri->v[0].xyz );
	bounds.AddPoint( tri->v[1].xyz );
	bounds.AddPoint( tri->v[2].xyz );

	// add a 1.0 slop margin on each side
	for ( i = 0 ; i < 3 ; i++ ) {
		blocks[0][i] = ( bounds[0][i] - 1.0 - hashBounds[0][i] ) / hashScale[i];
		blocks[0][i] = idMath::ClampInt( 0, HASH_BINS - 1, blocks[0][i] );
		blocks[1][i] = ( bounds[1][i] + 1.0 - hashBounds[0][i] ) / hashScale[i];
		blocks[1][i] = idMath::ClampInt( 0, HASH_BINS - 1, blocks[1][i] );
	}
}


/*
=================
HashTriangles

Removes triangles that are degenerated or flipped backwards
=================
*/
static void HashTriangles( optimizeGroup_t *groupList ) {
	mapTri_t	*a;
	int			vert;
	int			i;
	optimizeGroup_t	*group;

	// clear the hash tables
	memset( hashVerts, 0, sizeof( hashVerts ) );

	numHashVerts = 0;
	numTotalVerts = 0;

	// bound all the triangles to determine the bucket size
	hashBounds.Clear();
	for ( group = groupList ; group ; group = group->nextGroup ) {
		for ( a = group->triList ; a ; a = a->next ) {
			hashBounds.AddPoint( a->v[0].xyz );
			hashBounds.AddPoint( a->v[1].xyz );
			hashBounds.AddPoint( a->v[2].xyz );
		}
	}

	// spread the bounds so it will never have a zero size
	for ( i = 0 ; i < 3 ; i++ ) {
		hashBounds[0][i] = floor( hashBounds[0][i] - 1 );
		hashBounds[1][i] = ceil( hashBounds[1][i] + 1 );
		hashIntMins[i] = hashBounds[0][i] * SNAP_FRACTIONS;

		hashScale[i] = ( hashBounds[1][i] - hashBounds[0][i] ) / HASH_BINS;
		hashIntScale[i] = hashScale[i] * SNAP_FRACTIONS;
		if ( hashIntScale[i] < 1 ) {
			hashIntScale[i] = 1;
		}
	}

	// add all the points to the hash buckets
	for ( group = groupList ; group ; group = group->nextGroup ) {
		// don't create tjunctions against discrete surfaces (blood decals, etc)
		if ( group->material != NULL && group->material->IsDiscrete() ) {
			continue;
		}
		for ( a = group->triList ; a ; a = a->next ) {
			for ( vert = 0 ; vert < 3 ; vert++ ) {
				GetHashVert( a->v[vert].xyz, a->hashVert[vert] );
			}
		}
	}
}

/*
=================
FreeTJunctionHash

The optimizer may add some more crossing verts
after t junction processing
=================
*/
void FreeTJunctionHash( void ) {
	int			i, j, k;
	hashVert_t	*hv, *next;

	for ( i = 0 ; i < HASH_BINS ; i++ ) {
		for ( j = 0 ; j < HASH_BINS ; j++ ) {
			for ( k = 0 ; k < HASH_BINS ; k++ ) {
				for ( hv = hashVerts[i][j][k] ; hv ; hv = next ) {
					next = hv->next;
					Mem_Free( hv );
				}
			}
		}
	}
	memset( hashVerts, 0, sizeof( hashVerts ) );

	allVertRefs.ClearFree();
	allVertDsu.ClearFree();
}


/*
==================
FixTriangleAgainstHashVert

Returns a list of two new mapTri if the hashVert is
on an edge of the given mapTri, otherwise returns NULL.
==================
*/
mapTri_t *FixTriangleAgainstHashVert( const mapTri_t *a, const hashVert_t *hv ) {
	int			i;
	const idDrawVert	*v1, *v2;
	idDrawVert	split;
	idVec3		dir;
	float		len;
	float		frac;
	mapTri_t	*new1, *new2;
	idVec3		temp;
	float		d, off;
	const idVec3 *v;
	idPlane		plane1, plane2;

	v = &hv->v;

	// if the triangle already has this hashVert as a vert,
	// it can't be split by it
	if ( a->hashVert[0] == hv || a->hashVert[1] == hv || a->hashVert[2] == hv ) {
		return NULL;
	}

	// we probably should find the edge that the vertex is closest to.
	// it is possible to be < 1 unit away from multiple
	// edges, but we only want to split by one of them
	for ( i = 0 ; i < 3 ; i++ ) {
		v1 = &a->v[i];
		v2 = &a->v[(i+1)%3];
		VectorSubtract( v2->xyz, v1->xyz, dir );
		len = dir.Normalize();

		// if it is close to one of the edge vertexes, skip it
		VectorSubtract( *v, v1->xyz, temp );
		d = DotProduct( temp, dir );
		if ( d <= 0 || d >= len ) {
			continue;
		}

		// make sure it is on the line
		VectorMA( v1->xyz, d, dir, temp );
		VectorSubtract( temp, *v, temp );
		off = temp.Length();
		if ( off <= -COLINEAR_EPSILON || off >= COLINEAR_EPSILON ) {
			continue;
		}

		// take the x/y/z from the splitter,
		// but interpolate everything else from the original tri
		VectorCopy( *v, split.xyz );
		frac = d / len;
		split.st[0] = v1->st[0] + frac * ( v2->st[0] - v1->st[0] );
		split.st[1] = v1->st[1] + frac * ( v2->st[1] - v1->st[1] );
		split.normal[0] = v1->normal[0] + frac * ( v2->normal[0] - v1->normal[0] );
		split.normal[1] = v1->normal[1] + frac * ( v2->normal[1] - v1->normal[1] );
		split.normal[2] = v1->normal[2] + frac * ( v2->normal[2] - v1->normal[2] );
		split.normal.Normalize();

		// split the tri
		new1 = CopyMapTri( a );
		new1->v[(i+1)%3] = split;
		new1->hashVert[(i+1)%3] = hv;
		new1->next = NULL;

		new2 = CopyMapTri( a );
		new2->v[i] = split;
		new2->hashVert[i] = hv;
		new2->next = new1;

		//stgatilov: plane has zero normal if triangle is singular
		plane1.FromPoints( new1->hashVert[0]->v, new1->hashVert[1]->v, new1->hashVert[2]->v, false );
		plane2.FromPoints( new2->hashVert[0]->v, new2->hashVert[1]->v, new2->hashVert[2]->v, false );

		d = DotProduct( plane1, plane2 );

		// if the two split triangle's normals don't face the same way,
		// it should not be split
		if ( d <= 0 ) {
			FreeTriList( new2 );
			continue;
		}

		return new2;
	}


	return NULL;
}



/*
==================
FixTriangleAgainstHash

Potentially splits a triangle into a list of triangles based on tjunctions
==================
*/
static mapTri_t	*FixTriangleAgainstHash( const mapTri_t *tri ) {
	mapTri_t		*fixed;
	mapTri_t		*a;
	mapTri_t		*test, *next;
	int				blocks[2][3];
	int				i, j, k;
	hashVert_t		*hv;

	// if this triangle is degenerate after point snapping,
	// do nothing (this shouldn't happen, because they should
	// be removed as they are hashed)
	if ( tri->hashVert[0] == tri->hashVert[1]
		|| tri->hashVert[0] == tri->hashVert[2]
		|| tri->hashVert[1] == tri->hashVert[2] ) {
		return NULL;
	}

	fixed = CopyMapTri( tri );
	fixed->next = NULL;

	HashBlocksForTri( tri, blocks );
	for ( i = blocks[0][0] ; i <= blocks[1][0] ; i++ ) {
		for ( j = blocks[0][1] ; j <= blocks[1][1] ; j++ ) {
			for ( k = blocks[0][2] ; k <= blocks[1][2] ; k++ ) {
				for ( hv = hashVerts[i][j][k] ; hv ; hv = hv->next ) {
					// fix all triangles in the list against this point
					test = fixed;
					fixed = NULL;
					for ( ; test ; test = next ) {
						next = test->next;
						a = FixTriangleAgainstHashVert( test, hv );
						if ( a ) {
							// cut into two triangles
							a->next->next = fixed;
							fixed = a;
							FreeTri( test );
						} else {
							test->next = fixed;
							fixed = test;
						}
					}
				}
			}
		}
	}

	return fixed;
}


/*
==================
CountGroupListTris
==================
*/
int CountGroupListTris( const optimizeGroup_t *groupList ) {
	int		c;

	c = 0;
	for ( ; groupList ; groupList = groupList->nextGroup ) {
		c += CountTriList( groupList->triList );
	}

	return c;
}

/*
==================
FixAreaGroupsTjunctions
==================
*/
void	FixAreaGroupsTjunctions( optimizeGroup_t *groupList ) {
	if ( dmapGlobals.noTJunc ) {
		return;
	}

	if (dmap_tjunctionsAlgorithm.GetBool()) {
		newTjuncAlgorithm.Reset();

		for ( optimizeGroup_t *group = groupList ; group ; group = group->nextGroup ) {
			// don't touch discrete surfaces
			if ( group->material != NULL && group->material->IsDiscrete() ) {
				continue;
			}
			newTjuncAlgorithm.AddTriList( &group->triList );
		}
		// do all the hard stuff (lists will get replaced)
		newTjuncAlgorithm.Run( 
			float(VERTEX_EPSILON_NEW), float(COLINEAR_EPSILON),
			(dmap_disableCellSnappingTjunc.GetBool() ? 0 : SNAP_FRACTIONS)
		);

		newTjuncAlgorithm.Reset();
		return;
	}

	const mapTri_t	*tri;
	mapTri_t		*newList;
	mapTri_t		*fixed;
	int				startCount, endCount;
	optimizeGroup_t	*group;

	if ( !groupList ) {
		return;
	}

	startCount = CountGroupListTris( groupList );

	PrintIfVerbosityAtLeast( VL_VERBOSE, "----- FixAreaGroupsTjunctions -----\n" );
	PrintIfVerbosityAtLeast( VL_VERBOSE, "%6i triangles in\n", startCount );

	HashTriangles( groupList );
	HashVertsFinalize();

	for ( group = groupList ; group ; group = group->nextGroup ) {
		// don't touch discrete surfaces
		if ( group->material != NULL && group->material->IsDiscrete() ) {
			continue;
		}

		newList = NULL;
		for ( tri = group->triList ; tri ; tri = tri->next ) {
			fixed = FixTriangleAgainstHash( tri );
			newList = MergeTriLists( newList, fixed );
		}
		FreeTriList( group->triList );
		group->triList = newList;
	}

	endCount = CountGroupListTris( groupList );
	PrintIfVerbosityAtLeast( VL_VERBOSE, "%6i triangles out\n", endCount );
}


/*
==================
FixEntityTjunctions
==================
*/
void	FixEntityTjunctions( uEntity_t *e ) {
	int		i;
	TRACE_CPU_SCOPE_TEXT("FixEntityTjunctions", e->nameEntity)

	for ( i = 0 ; i < e->numAreas ; i++ ) {
		FixAreaGroupsTjunctions( e->areas[i].groups );
		FreeTJunctionHash();
	}
}


idCVar dmap_dontSplitWithFuncStaticVertices(
	"dmap_dontSplitWithFuncStaticVertices", "1", CVAR_BOOL | CVAR_SYSTEM,
	"If set to 0, then dmap uses all vertices of all func_static-s as splitting points for edges of world surfaces. "
	"As number of func_static-s grows up, this step becomes slower and slower. "
	"This behavior was changed in TDM 2.10. "
);
/*
==================
FixGlobalTjunctions
==================
*/
void FixGlobalTjunctions( uEntity_t *e ) {
	if (dmap_tjunctionsAlgorithm.GetBool()) {
		newTjuncAlgorithm.Reset();
		TRACE_CPU_SCOPE_TEXT("FixGlobalTjunctions", e->nameEntity);

		for ( int areaNum = 0 ; areaNum < e->numAreas ; areaNum++ ) {
			for ( optimizeGroup_t *group = e->areas[areaNum].groups ; group ; group = group->nextGroup ) {
				// don't touch discrete surfaces
				if ( group->material != NULL && group->material->IsDiscrete() )
					continue;
				newTjuncAlgorithm.AddTriList( &group->triList );
			}
		}
		// do all the hard stuff (lists will get replaced)
		newTjuncAlgorithm.Run( 
			float(VERTEX_EPSILON_NEW), float(COLINEAR_EPSILON),
			(dmap_disableCellSnappingTjunc.GetBool() ? 0 : SNAP_FRACTIONS)
		);

		TRACE_ATTACH_FORMAT(
			"%d -> %d tris, %d unique verts",
			newTjuncAlgorithm.GetInitialTriCount(), newTjuncAlgorithm.GetSplitTriCount(), newTjuncAlgorithm.GetMergedVertsCount()
		);

		newTjuncAlgorithm.Reset();
		return;
	}

	mapTri_t	*a;
	int			vert;
	int			i;
	optimizeGroup_t	*group;
	int			areaNum;

	TRACE_CPU_SCOPE_TEXT("FixGlobalTjunctions", e->nameEntity)
	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "----- FixGlobalTjunctions -----\n" );

	// clear the hash tables
	memset( hashVerts, 0, sizeof( hashVerts ) );

	numHashVerts = 0;
	numTotalVerts = 0;

	{
		TRACE_CPU_SCOPE( "GTJunc:Bound" )

		// bound all the triangles to determine the bucket size
		hashBounds.Clear();
		for ( areaNum = 0 ; areaNum < e->numAreas ; areaNum++ ) {
			for ( group = e->areas[areaNum].groups ; group ; group = group->nextGroup ) {
				for ( a = group->triList ; a ; a = a->next ) {
					hashBounds.AddPoint( a->v[0].xyz );
					hashBounds.AddPoint( a->v[1].xyz );
					hashBounds.AddPoint( a->v[2].xyz );
				}
			}
		}

		// spread the bounds so it will never have a zero size
		for ( i = 0 ; i < 3 ; i++ ) {
			hashBounds[0][i] = floor( hashBounds[0][i] - 1 );
			hashBounds[1][i] = ceil( hashBounds[1][i] + 1 );
			hashIntMins[i] = hashBounds[0][i] * SNAP_FRACTIONS;

			hashScale[i] = ( hashBounds[1][i] - hashBounds[0][i] ) / HASH_BINS;
			hashIntScale[i] = hashScale[i] * SNAP_FRACTIONS;
			if ( hashIntScale[i] < 1 ) {
				hashIntScale[i] = 1;
			}
		}
	}

	// add all the points to the hash buckets
	for ( areaNum = 0 ; areaNum < e->numAreas ; areaNum++ ) {
		TRACE_CPU_SCOPE_FORMAT( "GTJunc:Hash", "area%d", areaNum )
		for ( group = e->areas[areaNum].groups ; group ; group = group->nextGroup ) {
			// don't touch discrete surfaces
			if ( group->material != NULL && group->material->IsDiscrete() ) {
				continue;
			}

			for ( a = group->triList ; a ; a = a->next ) {
				for ( vert = 0 ; vert < 3 ; vert++ ) {
					GetHashVert( a->v[vert].xyz, a->hashVert[vert] );
				}
			}
		}
	}

	// add all the func_static model vertexes to the hash buckets
	// optionally inline some of the func_static models
	if ( dmapGlobals.entityNum == 0 && !dmap_dontSplitWithFuncStaticVertices.GetBool() && dmap_fixVertexSnappingTjunc.GetInteger() < 2 ) {
		TRACE_CPU_SCOPE( "GTJunc:AddEntities" )

		for ( int eNum = 1 ; eNum < dmapGlobals.num_entities ; eNum++ ) {
			uEntity_t *entity = &dmapGlobals.uEntities[eNum];
			const char *className = entity->mapEntity->epairs.GetString( "classname" );
			if ( idStr::Icmp( className, "func_static" ) ) {
				continue;
			}
			const char *modelName = entity->mapEntity->epairs.GetString( "model" );
			if ( !modelName ) {
				continue;
			}
			if ( !strstr( modelName, ".lwo" ) && !strstr( modelName, ".ase" ) && !strstr( modelName, ".ma" ) ) {
				continue;
			}
			TRACE_CPU_SCOPE_TEXT( "GTJunc:AddEntity", entity->nameEntity );

			idRenderModel	*model = renderModelManager->FindModel( modelName );

//			common->Printf( "adding T junction verts for %s.\n", entity->mapEntity->epairs.GetString( "name" ) );

			idMat3	axis;
			gameEdit->ParseSpawnArgsToAxis( &entity->mapEntity->epairs, axis );
			idVec3	origin = entity->mapEntity->epairs.GetVector( "origin" );

			for ( i = 0 ; i < model->NumSurfaces() ; i++ ) {
				const modelSurface_t *surface = model->Surface( i );
				const srfTriangles_t *tri = surface->geometry;

				mapTri_t	mapTri;
				memset( &mapTri, 0, sizeof( mapTri ) );
				mapTri.material = surface->material;
				// don't let discretes (autosprites, etc) merge together
				if ( mapTri.material->IsDiscrete() ) {
					mapTri.mergeGroup = (void *)surface;
				}
				for ( int j = 0 ; j < tri->numVerts ; j += 3 ) {
					idVec3 v = tri->verts[j].xyz * axis + origin;
					const hashVert_t *hv;
					GetHashVert( v, hv );
				}
			}
		}
	}

	{
		TRACE_CPU_SCOPE( "GTJunc:Finalize" )
		HashVertsFinalize();
	}

	// now fix each area
	for ( areaNum = 0 ; areaNum < e->numAreas ; areaNum++ ) {
		TRACE_CPU_SCOPE_FORMAT( "GTJunc:Split", "area%d", areaNum )
		for ( group = e->areas[areaNum].groups ; group ; group = group->nextGroup ) {
			// don't touch discrete surfaces
			if ( group->material != NULL && group->material->IsDiscrete() ) {
				continue;
			}

			mapTri_t *newList = NULL;
			for ( mapTri_t *tri = group->triList ; tri ; tri = tri->next ) {
				mapTri_t *fixed = FixTriangleAgainstHash( tri );
				newList = MergeTriLists( newList, fixed );
			}
			FreeTriList( group->triList );
			group->triList = newList;
		}
	}


	{
		TRACE_CPU_SCOPE( "GTJunc:Free" )
		// done
		FreeTJunctionHash();
	}
}
