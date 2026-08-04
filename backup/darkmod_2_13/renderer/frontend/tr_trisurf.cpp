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

/*
==============================================================================

TRIANGLE MESH PROCESSING

The functions in this file have no vertex / index count limits.

Truly identical vertexes that match in position, normal, and texcoord can
be merged away.

Vertexes that match in position and texcoord, but have distinct normals will
remain distinct for all purposes.  This is usually a poor choice for models,
as adding a bevel face will not add any more vertexes, and will tend to
look better.

Match in position and normal, but differ in texcoords are referenced together
for calculating tangent vectors for bump mapping.
Artists should take care to have identical texels in all maps (bump/diffuse/specular)
in this case

Vertexes that only match in position are merged for shadow edge finding.

Degenerate triangles.

Overlapped triangles, even if normals or texcoords differ, must be removed.
for the silhoette based stencil shadow algorithm to function properly.
Is this true???
Is the overlapped triangle problem just an example of the trippled edge problem?

Interpenetrating triangles are not currently clipped to surfaces.
Do they effect the shadows?

if vertexes are intended to deform apart, make sure that no vertexes
are on top of each other in the base frame, or the sil edges may be
calculated incorrectly.

We might be able to identify this from topology.

Dangling edges are acceptable, but three way edges are not.

Are any combinations of two way edges unacceptable, like one facing
the backside of the other?


Topology is determined by a collection of triangle indexes.

The edge list can be built up from this, and stays valid even under
deformations.

Somewhat non-intuitively, concave edges cannot be optimized away, or the
stencil shadow algorithm miscounts.

Face normals are needed for generating shadow volumes and for calculating
the silhouette, but they will change with any deformation.

Vertex normals and vertex tangents will change with each deformation,
but they may be able to be transformed instead of recalculated.

bounding volume, both box and sphere will change with deformation.

silhouette indexes
shade indexes
texture indexes

  shade indexes will only be > silhouette indexes if there is facet shading present

	lookups from texture to sil and texture to shade?

The normal and tangent vector smoothing is simple averaging, no attempt is
made to better handle the cases where the distribution around the shared vertex
is highly uneven.


  we may get degenerate triangles even with the uniquing and removal
  if the vertexes have different texcoords.

==============================================================================
*/

// this shouldn't change anything, but previously renderbumped models seem to need it
#define USE_INVA

// instead of using the texture T vector, cross the normal and S vector for an orthogonal axis
#define DERIVE_UNSMOOTHED_BITANGENT

#if LEGACY_ALLOCATOR
static idBlockAlloc<srfTriangles_t, 1<<8>				srfTrianglesAllocator;
#endif

#ifdef USE_TRI_DATA_ALLOCATOR
static idDynamicBlockAlloc<idDrawVert, 1<<20, 1<<10>	triVertexAllocator;
static idDynamicBlockAlloc<glIndex_t, 1<<18, 1<<10>		triIndexAllocator;
static idDynamicBlockAlloc<shadowCache_t, 1<<18, 1<<10>	triShadowVertexAllocator;
static idDynamicBlockAlloc<idPlane, 1<<17, 1<<10>		triPlaneAllocator;
static idDynamicBlockAlloc<glIndex_t, 1<<17, 1<<10>		triSilIndexAllocator;
static idDynamicBlockAlloc<silEdge_t, 1<<17, 1<<10>		triSilEdgeAllocator;
static idDynamicBlockAlloc<glIndex_t, 1<<17, 1<<10>		triAdjTrisAllocator;
static idDynamicBlockAlloc<dominantTri_t, 1<<16, 1<<10>	triDominantTrisAllocator;
static idDynamicBlockAlloc<int, 1<<16, 1<<10>			triMirroredVertAllocator;
static idDynamicBlockAlloc<int, 1<<16, 1<<10>			triDupVertAllocator;
static idDynamicBlockAlloc<bvhNode_t, 1<<16, 1<<10>		triBvhAllocator;
#else
static idDynamicAlloc<idDrawVert, 1<<20, 1<<10>			triVertexAllocator;
static idDynamicAlloc<glIndex_t, 1<<18, 1<<10>			triIndexAllocator;
static idDynamicAlloc<shadowCache_t, 1<<18, 1<<10>		triShadowVertexAllocator;
static idDynamicAlloc<idPlane, 1<<17, 1<<10>			triPlaneAllocator;
static idDynamicAlloc<glIndex_t, 1<<17, 1<<10>			triSilIndexAllocator;
static idDynamicAlloc<silEdge_t, 1<<17, 1<<10>			triSilEdgeAllocator;
static idDynamicAlloc<glIndex_t, 1<<17, 1<<10>			triAdjTrisAllocator;
static idDynamicAlloc<dominantTri_t, 1<<16, 1<<10>		triDominantTrisAllocator;
static idDynamicAlloc<int, 1<<16, 1<<10>				triMirroredVertAllocator;
static idDynamicAlloc<int, 1<<16, 1<<10>				triDupVertAllocator;
static idDynamicAlloc<bvhNode_t, 1<<16, 1<<10>			triBvhAllocator;
#endif


/*
===============
R_InitTriSurfData
===============
*/
void R_InitTriSurfData( void ) {
	// initialize allocators for triangle surfaces
	triVertexAllocator.Init();
	triIndexAllocator.Init();
	triShadowVertexAllocator.Init();
	triPlaneAllocator.Init();
	triSilIndexAllocator.Init();
	triSilEdgeAllocator.Init();
	triAdjTrisAllocator.Init();
	triDominantTrisAllocator.Init();
	triMirroredVertAllocator.Init();
	triDupVertAllocator.Init();
	triBvhAllocator.Init();

	// never swap out triangle surfaces
	triVertexAllocator.SetLockMemory( true );
	triIndexAllocator.SetLockMemory( true );
	triShadowVertexAllocator.SetLockMemory( true );
	triPlaneAllocator.SetLockMemory( true );
	triSilIndexAllocator.SetLockMemory( true );
	triSilEdgeAllocator.SetLockMemory( true );
	triAdjTrisAllocator.SetLockMemory( true );
	triDominantTrisAllocator.SetLockMemory( true );
	triMirroredVertAllocator.SetLockMemory( true );
	triDupVertAllocator.SetLockMemory( true );
	triBvhAllocator.SetLockMemory( true );
}

/*
===============
R_ShutdownTriSurfData
===============
*/
void R_ShutdownTriSurfData( void ) {
#if LEGACY_ALLOCATOR
	srfTrianglesAllocator.Shutdown();
#endif
	triVertexAllocator.Shutdown();
	triIndexAllocator.Shutdown();
	triShadowVertexAllocator.Shutdown();
	triPlaneAllocator.Shutdown();
	triSilIndexAllocator.Shutdown();
	triSilEdgeAllocator.Shutdown();
	triAdjTrisAllocator.Shutdown();
	triDominantTrisAllocator.Shutdown();
	triMirroredVertAllocator.Shutdown();
	triDupVertAllocator.Shutdown();
	triBvhAllocator.Shutdown();
}

/*
===============
R_PurgeTriSurfData
===============
*/
void R_PurgeTriSurfData( frameData_t *frame ) {
	// free deferred triangle surfaces
	R_FreeDeferredTriSurfs( frame );

	// free empty base blocks
	triVertexAllocator.FreeEmptyBaseBlocks();
	triIndexAllocator.FreeEmptyBaseBlocks();
	triShadowVertexAllocator.FreeEmptyBaseBlocks();
	triPlaneAllocator.FreeEmptyBaseBlocks();
	triSilIndexAllocator.FreeEmptyBaseBlocks();
	triSilEdgeAllocator.FreeEmptyBaseBlocks();
	triAdjTrisAllocator.FreeEmptyBaseBlocks();
	triDominantTrisAllocator.FreeEmptyBaseBlocks();
	triMirroredVertAllocator.FreeEmptyBaseBlocks();
	triDupVertAllocator.FreeEmptyBaseBlocks();
	triBvhAllocator.FreeEmptyBaseBlocks();
}

/*
===============
R_ShowTriMemory_f
===============
*/
void R_ShowTriSurfMemory_f( const idCmdArgs &args ) {
#if LEGACY_ALLOCATOR
	common->Printf( "%6d kB in %d triangle surfaces\n",
		( srfTrianglesAllocator.GetAllocCount() * sizeof( srfTriangles_t ) ) >> 10,
			srfTrianglesAllocator.GetAllocCount() );
#endif
	common->Printf( "%6d kB vertex memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triVertexAllocator.GetBaseBlockMemory() >> 10, triVertexAllocator.GetFreeBlockMemory() >> 10,
			triVertexAllocator.GetNumFreeBlocks(), triVertexAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB index memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triIndexAllocator.GetBaseBlockMemory() >> 10, triIndexAllocator.GetFreeBlockMemory() >> 10,
			triIndexAllocator.GetNumFreeBlocks(), triIndexAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB shadow vert memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triShadowVertexAllocator.GetBaseBlockMemory() >> 10, triShadowVertexAllocator.GetFreeBlockMemory() >> 10,
			triShadowVertexAllocator.GetNumFreeBlocks(), triShadowVertexAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB tri plane memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triPlaneAllocator.GetBaseBlockMemory() >> 10, triPlaneAllocator.GetFreeBlockMemory() >> 10,
			triPlaneAllocator.GetNumFreeBlocks(), triPlaneAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB sil index memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triSilIndexAllocator.GetBaseBlockMemory() >> 10, triSilIndexAllocator.GetFreeBlockMemory() >> 10,
			triSilIndexAllocator.GetNumFreeBlocks(), triSilIndexAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB sil edge memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triSilEdgeAllocator.GetBaseBlockMemory() >> 10, triSilEdgeAllocator.GetFreeBlockMemory() >> 10,
			triSilEdgeAllocator.GetNumFreeBlocks(), triSilEdgeAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB adj tris memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triAdjTrisAllocator.GetBaseBlockMemory() >> 10, triAdjTrisAllocator.GetFreeBlockMemory() >> 10,
			triAdjTrisAllocator.GetNumFreeBlocks(), triAdjTrisAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB mirror vert memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triMirroredVertAllocator.GetBaseBlockMemory() >> 10, triMirroredVertAllocator.GetFreeBlockMemory() >> 10,
			triMirroredVertAllocator.GetNumFreeBlocks(), triMirroredVertAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB dup vert memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triDupVertAllocator.GetBaseBlockMemory() >> 10, triDupVertAllocator.GetFreeBlockMemory() >> 10,
			triDupVertAllocator.GetNumFreeBlocks(), triDupVertAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB BVH memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triBvhAllocator.GetBaseBlockMemory() >> 10, triBvhAllocator.GetFreeBlockMemory() >> 10,
		triBvhAllocator.GetNumFreeBlocks(), triBvhAllocator.GetNumEmptyBaseBlocks() );

#if LEGACY_ALLOCATOR
	common->Printf( "%6d kB total triangle memory\n",
		( srfTrianglesAllocator.GetAllocCount() * sizeof( srfTriangles_t ) +
			triVertexAllocator.GetBaseBlockMemory() +
			triIndexAllocator.GetBaseBlockMemory() +
			triShadowVertexAllocator.GetBaseBlockMemory() +
			triPlaneAllocator.GetBaseBlockMemory() +
			triSilIndexAllocator.GetBaseBlockMemory() +
			triSilEdgeAllocator.GetBaseBlockMemory() +
			triAdjTrisAllocator.GetBaseBlockMemory() +
			triDominantTrisAllocator.GetBaseBlockMemory() +
			triMirroredVertAllocator.GetBaseBlockMemory() +
			triDupVertAllocator.GetBaseBlockMemory() +
			triBvhAllocator.GetBaseBlockMemory()) >> 10 );
#endif
}

/*
=================
R_TriSurfMemory

For memory profiling
=================
*/
int R_TriSurfMemory( const srfTriangles_t *tri ) {
	int total = 0;

	if ( !tri ) {
		return total;
	}

	// used as a flag in interations
	if ( tri == LIGHT_TRIS_DEFERRED ) {
		return total;
	}

	if ( tri->shadowVertexes != NULL ) {
		total += tri->numVerts * sizeof( tri->shadowVertexes[0] );
	} else if ( tri->verts != NULL ) {
		if ( tri->ambientSurface == NULL || tri->verts != tri->ambientSurface->verts ) {
			total += tri->numVerts * sizeof( tri->verts[0] );
		}
	}
	if ( tri->facePlanes != NULL ) {
		total += tri->numIndexes / 3 * sizeof( tri->facePlanes[0] );
	}
	if ( tri->indexes != NULL ) {
		if ( tri->ambientSurface == NULL || tri->indexes != tri->ambientSurface->indexes ) {
			total += tri->numIndexes * sizeof( tri->indexes[0] );
		}
	}
	if ( tri->silIndexes != NULL ) {
		total += tri->numIndexes * sizeof( tri->silIndexes[0] );
	}
	if ( tri->silEdges != NULL ) {
		total += tri->numSilEdges * sizeof( tri->silEdges[0] );
	}
	if ( tri->adjTris != NULL ) {
		total += tri->numIndexes * sizeof( tri->adjTris[0] );
	}
	if ( tri->dominantTris != NULL ) {
		total += tri->numVerts * sizeof( tri->dominantTris[0] );
	}
	if ( tri->mirroredVerts != NULL ) {
		total += tri->numMirroredVerts * sizeof( tri->mirroredVerts[0] );
	}
	if ( tri->dupVerts != NULL ) {
		total += tri->numDupVerts * sizeof( tri->dupVerts[0] );
	}
	if ( tri->bvhNodes != NULL ) {
		total += tri->numBvhNodes * sizeof( tri->bvhNodes[0] );
	}

	total += sizeof( *tri );

	return total;
}

/*
==============
R_FreeStaticTriSurfVertexCaches
==============
*/
void R_FreeStaticTriSurfVertexCaches( srfTriangles_t *tri ) {
	if ( tri->ambientSurface == NULL ) {
		// this is a real model surface
		tri->ambientCache = NO_CACHE;
	} else {
		// this is a light interaction surface that references
		// a different ambient model surface
	}
	if( tri->indexCache.IsValid() ) {
		tri->indexCache = NO_CACHE;
	}
	if ( tri->shadowCache.IsValid() && ( tri->shadowVertexes != NULL || tri->verts != NULL ) ) {
		// if we don't have tri->shadowVertexes, these are a reference to a
		// shadowCache on the original surface, which a vertex program
		// will take care of making unique for each light
		tri->shadowCache = NO_CACHE;
	}
}

/*
=================
R_FreeStaticTriSurfIndexes
=================
*/
void R_FreeStaticTriSurfIndexes( srfTriangles_t *tri ) {
	if ( tri->indexes ) {
		triIndexAllocator.Free( tri->indexes );
		tri->indexes = NULL;
	}
}


/*
==============
R_ReallyFreeStaticTriSurf

This does the actual free
==============
*/
void R_ReallyFreeStaticTriSurf( srfTriangles_t *tri ) {
	if ( !tri ) {
		return;
	}

	R_FreeStaticTriSurfVertexCaches( tri );

	if ( tri->verts != NULL ) {
		// R_CreateLightTris points tri->verts at the verts of the ambient surface
		if ( tri->ambientSurface == NULL || tri->verts != tri->ambientSurface->verts ) {
			triVertexAllocator.Free( tri->verts );
		}
	}

	if ( !tri->deformedSurface ) {
		if ( tri->indexes != NULL ) {
			// if a surface is completely inside a light volume R_CreateLightTris points tri->indexes at the indexes of the ambient surface
			if ( tri->ambientSurface == NULL || tri->indexes != tri->ambientSurface->indexes ) {
				triIndexAllocator.Free( tri->indexes );
			}
		}
		if ( tri->silIndexes != NULL ) {
			triSilIndexAllocator.Free( tri->silIndexes );
		}
		if ( tri->silEdges != NULL ) {
			triSilEdgeAllocator.Free( tri->silEdges );
		}
		if ( tri->adjTris != NULL ) {
			triAdjTrisAllocator.Free( tri->adjTris );
		}
		if ( tri->dominantTris != NULL ) {
			triDominantTrisAllocator.Free( tri->dominantTris );
		}
		if ( tri->mirroredVerts != NULL ) {
			triMirroredVertAllocator.Free( tri->mirroredVerts );
		}
		if ( tri->dupVerts != NULL ) {
			triDupVertAllocator.Free( tri->dupVerts );
		}
	}

	if ( tri->facePlanes != NULL ) {
		triPlaneAllocator.Free( tri->facePlanes );
	}

	if ( tri->shadowVertexes != NULL ) {
		triShadowVertexAllocator.Free( tri->shadowVertexes );
	}

	if ( tri->bvhNodes != NULL ) {
		triBvhAllocator.Free( tri->bvhNodes );
	}

#ifdef _DEBUG
	memset( tri, 0, sizeof( srfTriangles_t ) );
#endif

#if LEGACY_ALLOCATOR
	srfTrianglesAllocator.Free( tri );
#else
	// clear the tri out so we don't retain stale data
	memset( tri, 0, sizeof( srfTriangles_t ) );

	Mem_Free( tri );
#endif
}

/*
==============
R_CheckStaticTriSurfMemory
==============
*/
void R_CheckStaticTriSurfMemory( const srfTriangles_t *tri ) {
	if ( !tri ) {
		return;
	}

	if ( tri->verts != NULL ) {
		// R_CreateLightTris points tri->verts at the verts of the ambient surface
		if ( tri->ambientSurface == NULL || tri->verts != tri->ambientSurface->verts ) {
			const char *error = triVertexAllocator.CheckMemory( tri->verts );
			assert( error == NULL );
		}
	}

	if ( !tri->deformedSurface ) {
		if ( tri->indexes != NULL ) {
			// if a surface is completely inside a light volume R_CreateLightTris points tri->indexes at the indexes of the ambient surface
			if ( tri->ambientSurface == NULL || tri->indexes != tri->ambientSurface->indexes ) {
				const char *error = triIndexAllocator.CheckMemory( tri->indexes );
				assert( error == NULL );
			}
		}
	}

	if ( tri->shadowVertexes != NULL ) {
		const char *error = triShadowVertexAllocator.CheckMemory( tri->shadowVertexes );
		assert( error == NULL );
	}
}

/*
==================
R_FreeDeferredTriSurfs
==================
*/
void R_FreeDeferredTriSurfs( frameData_t *frame ) {
	srfTriangles_t	*tri, *next;

	if ( !frame || !frame->firstDeferredFreeTriSurf ) {
		return;
	}

	for ( tri = frame->firstDeferredFreeTriSurf; tri; tri = next ) {
		next = tri->nextDeferredFree;
		R_ReallyFreeStaticTriSurf( tri );
	}

	frame->firstDeferredFreeTriSurf = NULL;
	frame->lastDeferredFreeTriSurf = NULL;
}

/*
==============
R_FreeStaticTriSurf

This will defer the free until the current frame has run through the back end.
==============
*/
void R_FreeStaticTriSurf( srfTriangles_t *tri ) {
	frameData_t		*frame;

	if ( !tri ) {
		return;
	}

	if ( tri->nextDeferredFree ) {
		common->Error( "R_FreeStaticTriSurf: freed a freed triangle" );
	}
	frame = frameData;

	if ( !frame ) {
		// command line utility, or rendering in editor preview mode ( force )
		R_ReallyFreeStaticTriSurf( tri );
	} else {
#ifdef ID_DEBUG_MEMORY
		R_CheckStaticTriSurfMemory( tri );
#endif

		idScopedCriticalSection lock(frame->deferredFreeMutex);
		tri->nextDeferredFree = NULL;
		if ( frame->lastDeferredFreeTriSurf ) {
			frame->lastDeferredFreeTriSurf->nextDeferredFree = tri;
		} else {
			frame->firstDeferredFreeTriSurf = tri;
		}
		frame->lastDeferredFreeTriSurf = tri;
	}
}

/*
==============
R_AllocStaticTriSurf
==============
*/
#if LEGACY_ALLOCATOR
srfTriangles_t *R_AllocStaticTriSurf( void ) {
	srfTriangles_t *tris = srfTrianglesAllocator.Alloc();
	memset( tris, 0, sizeof( srfTriangles_t ) );
	return tris;
}
#else
srfTriangles_t* R_AllocStaticTriSurf()
{
	srfTriangles_t* tris = (srfTriangles_t*)Mem_ClearedAlloc( sizeof( srfTriangles_t ) );
	return tris;
}
#endif

/*
=================
R_CopyStaticTriSurf

This only duplicates the indexes and verts, not any of the derived data.
=================
*/
srfTriangles_t *R_CopyStaticTriSurf( const srfTriangles_t *tri ) {
	srfTriangles_t	*newTri;

	newTri = R_AllocStaticTriSurf();
	R_AllocStaticTriSurfVerts( newTri, tri->numVerts );
	R_AllocStaticTriSurfIndexes( newTri, tri->numIndexes );
	newTri->numVerts = tri->numVerts;
	newTri->numIndexes = tri->numIndexes;
	memcpy( newTri->verts, tri->verts, tri->numVerts * sizeof( newTri->verts[0] ) );
	memcpy( newTri->indexes, tri->indexes, tri->numIndexes * sizeof( newTri->indexes[0] ) );

	return newTri;
}

/*
=================
R_AllocStaticTriSurfVerts
=================
*/
void R_AllocStaticTriSurfVerts( srfTriangles_t *tri, int numVerts ) {
	assert( tri->verts == NULL );
	tri->verts = triVertexAllocator.Alloc( numVerts );
}

/*
=================
R_AllocStaticTriSurfIndexes
=================
*/
void R_AllocStaticTriSurfIndexes( srfTriangles_t *tri, int numIndexes ) {
	assert( tri->indexes == NULL );
	tri->indexes = triIndexAllocator.Alloc( numIndexes );
}

/*
=================
R_AllocStaticTriSurfShadowVerts
=================
*/
void R_AllocStaticTriSurfShadowVerts( srfTriangles_t *tri, int numVerts ) {
	assert( tri->shadowVertexes == NULL );
	tri->shadowVertexes = triShadowVertexAllocator.Alloc( numVerts );
}

/*
=================
R_AllocStaticTriSurfPlanes
=================
*/
void R_AllocStaticTriSurfPlanes( srfTriangles_t *tri, int numIndexes ) {
	if ( tri->facePlanes ) {
		triPlaneAllocator.Free( tri->facePlanes );
	}
	tri->facePlanes = triPlaneAllocator.Alloc( numIndexes / 3 );
}

/*
=================
R_ResizeStaticTriSurfVerts
=================
*/
void R_ResizeStaticTriSurfVerts( srfTriangles_t *tri, int numVerts ) {
#ifdef USE_TRI_DATA_ALLOCATOR
	tri->verts = triVertexAllocator.Resize( tri->verts, numVerts );
#else
	assert( false );
#endif
}

/*
=================
R_ResizeStaticTriSurfIndexes
=================
*/
void R_ResizeStaticTriSurfIndexes( srfTriangles_t *tri, int numIndexes ) {
#ifdef USE_TRI_DATA_ALLOCATOR
	tri->indexes = triIndexAllocator.Resize( tri->indexes, numIndexes );
#else
//	assert( false ); duzenko: is it supposed to do anything?
#endif
}

/*
=================
R_ResizeStaticTriSurfShadowVerts
=================
*/
void R_ResizeStaticTriSurfShadowVerts( srfTriangles_t *tri, int numVerts ) {
#ifdef USE_TRI_DATA_ALLOCATOR
	tri->shadowVertexes = triShadowVertexAllocator.Resize( tri->shadowVertexes, numVerts );
#else
	assert( false );
#endif
}

/*
=================
R_ReferenceStaticTriSurfVerts
=================
*/
void R_ReferenceStaticTriSurfVerts( srfTriangles_t *tri, const srfTriangles_t *reference ) {
	tri->verts = reference->verts;
}

/*
=================
R_ReferenceStaticTriSurfIndexes
=================
*/
void R_ReferenceStaticTriSurfIndexes( srfTriangles_t *tri, const srfTriangles_t *reference ) {
	tri->indexes = reference->indexes;
}

/*
=================
R_FreeStaticTriSurfSilIndexes
=================
*/
void R_FreeStaticTriSurfSilIndexes( srfTriangles_t *tri ) {
	triSilIndexAllocator.Free( tri->silIndexes );
	tri->silIndexes = NULL;
}

/*
===============
R_RangeCheckIndexes

Check for syntactically incorrect indexes, like out of range values.
Does not check for semantics, like degenerate triangles.

No vertexes is acceptable if no indexes.
No indexes is acceptable.
More vertexes than are referenced by indexes are acceptable.
===============
*/
void R_RangeCheckIndexes( const srfTriangles_t *tri ) {
	int		i;

	if ( tri->numIndexes < 0 ) {
		common->Error( "R_RangeCheckIndexes: numIndexes < 0" );
	}
	if ( tri->numVerts < 0 ) {
		common->Error( "R_RangeCheckIndexes: numVerts < 0" );
	}

	// must specify an integral number of triangles
	if ( tri->numIndexes % 3 != 0 ) {
		common->Error( "R_RangeCheckIndexes: numIndexes %% 3" );
	}

	for ( i = 0 ; i < tri->numIndexes ; i++ ) {
		if ( tri->indexes[i] < 0 || tri->indexes[i] >= tri->numVerts ) {
			common->Error( "R_RangeCheckIndexes: index out of range" );
		}
	}

	// this should not be possible unless there are unused verts
	if ( tri->numVerts > tri->numIndexes ) {
		// FIXME: find the causes of these
		// common->Printf( "R_RangeCheckIndexes: tri->numVerts > tri->numIndexes\n" );
	}
}

/*
=================
R_BoundTriSurf
=================
*/
void R_BoundTriSurf( srfTriangles_t *tri ) {
	SIMDProcessor->MinMax( tri->bounds[0], tri->bounds[1], tri->verts, tri->numVerts );
}

/*
=================
R_CreateSilRemap
=================
*/
static int *R_CreateSilRemap( const srfTriangles_t *tri ) {
	int		c_removed, c_unique;
	int		*remap;
	int		i, j, hashKey;
	const idDrawVert *v1, *v2;

	remap = (int *)R_ClearedStaticAlloc( tri->numVerts * sizeof( remap[0] ) );

	if ( !r_useSilRemap.GetBool() ) {
		for ( i = 0 ; i < tri->numVerts ; i++ ) {
			remap[i] = i;
		}
		return remap;
	}

	idHashIndex		hash( 1024, tri->numVerts );

	c_removed = 0;
	c_unique = 0;
	for ( i = 0 ; i < tri->numVerts ; i++ ) {
		v1 = &tri->verts[i];

		// see if there is an earlier vert that it can map to
		hashKey = hash.GenerateKey( v1->xyz );
		for ( j = hash.First( hashKey ); j >= 0; j = hash.Next( j ) ) {
			v2 = &tri->verts[j];
			if ( v2->xyz[0] == v1->xyz[0]
				&& v2->xyz[1] == v1->xyz[1]
				&& v2->xyz[2] == v1->xyz[2] ) {
				c_removed++;
				remap[i] = j;
				break;
			}
		}
		if ( j < 0 ) {
			c_unique++;
			remap[i] = i;
			hash.Add( hashKey, i );
		}
	}

	return remap;
}

/*
=================
R_CreateSilIndexes

Uniquing vertexes only on xyz before creating sil edges reduces
the edge count by about 20% on Q3 models
=================
*/
void R_CreateSilIndexes( srfTriangles_t *tri ) {
	int		i;
	int		*remap;

	if ( tri->silIndexes ) {
		triSilIndexAllocator.Free( tri->silIndexes );
		tri->silIndexes = NULL;
	}

	remap = R_CreateSilRemap( tri );

	// remap indexes to the first one
	tri->silIndexes = triSilIndexAllocator.Alloc( tri->numIndexes );
	for ( i = 0; i < tri->numIndexes; i++ ) {
		tri->silIndexes[i] = remap[tri->indexes[i]];
	}

	R_StaticFree( remap );
}

/*
=====================
R_CreateDupVerts
=====================
*/
void R_CreateDupVerts( srfTriangles_t *tri ) {
	int i;

	int *remap = (int *) _alloca16( tri->numVerts * sizeof( remap[0] ) );

	// initialize vertex remap in case there are unused verts
	for ( i = 0; i < tri->numVerts; i++ ) {
		remap[i] = i;
	}

	// set the remap based on how the silhouette indexes are remapped
	for ( i = 0; i < tri->numIndexes; i++ ) {
		remap[tri->indexes[i]] = tri->silIndexes[i];
	}

	// create duplicate vertex index based on the vertex remap
	int * tempDupVerts = (int *) _alloca16( tri->numVerts * 2 * sizeof( tempDupVerts[0] ) );
	tri->numDupVerts = 0;
	for ( i = 0; i < tri->numVerts; i++ ) {
		if ( remap[i] != i ) {
			tempDupVerts[tri->numDupVerts*2+0] = i;
			tempDupVerts[tri->numDupVerts*2+1] = remap[i];
			tri->numDupVerts++;
		}
	}

	tri->dupVerts = triDupVertAllocator.Alloc( tri->numDupVerts * 2 );
	memcpy( tri->dupVerts, tempDupVerts, tri->numDupVerts * 2 * sizeof( tri->dupVerts[0] ) );
}

/*
=====================
R_DeriveFacePlanes

Writes the facePlanes values, overwriting existing ones if present
=====================
*/
void R_DeriveFacePlanes( srfTriangles_t *tri ) {
	idPlane *	planes;

	if ( !tri->facePlanes ) {
		R_AllocStaticTriSurfPlanes( tri, tri->numIndexes );
	}
	planes = tri->facePlanes;

#if 1

	SIMDProcessor->DeriveTriPlanes( planes, tri->verts, tri->numVerts, tri->indexes, tri->numIndexes );

#else

	for ( int i = 0; i < tri->numIndexes; i+= 3, planes++ ) {
		int		i1, i2, i3;
		idVec3	d1, d2, normal;
		idVec3	*v1, *v2, *v3;

		i1 = tri->indexes[i + 0];
		i2 = tri->indexes[i + 1];
		i3 = tri->indexes[i + 2];

		v1 = &tri->verts[i1].xyz;
		v2 = &tri->verts[i2].xyz;
		v3 = &tri->verts[i3].xyz;

		d1[0] = v2->x - v1->x;
		d1[1] = v2->y - v1->y;
		d1[2] = v2->z - v1->z;

		d2[0] = v3->x - v1->x;
		d2[1] = v3->y - v1->y;
		d2[2] = v3->z - v1->z;

		normal[0] = d2.y * d1.z - d2.z * d1.y;
		normal[1] = d2.z * d1.x - d2.x * d1.z;
		normal[2] = d2.x * d1.y - d2.y * d1.x;

		float sqrLength, invLength;

		sqrLength = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
		invLength = idMath::RSqrt( sqrLength );

		(*planes)[0] = normal[0] * invLength;
		(*planes)[1] = normal[1] * invLength;
		(*planes)[2] = normal[2] * invLength;

		planes->FitThroughPoint( *v1 );
	}

#endif

	tri->facePlanesCalculated = true;
}

/*
=====================
R_CreateVertexNormals

Averages together the contributions of all faces that are
used by a vertex, creating drawVert->normal
=====================
*/
void R_CreateVertexNormals( srfTriangles_t *tri ) {
	int		i, j;
	const idPlane *planes;

	for ( i = 0 ; i < tri->numVerts ; i++ ) {
		tri->verts[i].normal.Zero();
	}

	if ( !tri->facePlanes || !tri->facePlanesCalculated ) {
		R_DeriveFacePlanes( tri );
	}
	if ( !tri->silIndexes ) {
		R_CreateSilIndexes( tri );
	}
	planes = tri->facePlanes;
	for ( i = 0 ; i < tri->numIndexes ; i += 3, planes++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			int index = tri->silIndexes[i+j];
			tri->verts[index].normal += planes->Normal();
		}
	}

	// normalize and replicate from silIndexes to all indexes
	for ( i = 0 ; i < tri->numIndexes ; i++ ) {
		tri->verts[tri->indexes[i]].normal = tri->verts[tri->silIndexes[i]].normal;
		tri->verts[tri->indexes[i]].normal.Normalize();
	}
}

/*
===============
SilEdgeGenerator
===============
*/
struct SilEdgeGenerator {
	int numPlanes = -1;
	idList<silEdge_t> silEdges;
	idHashIndex	silEdgeHash;

	int c_duplicatedEdges = 0;
	int c_tripledEdges = 0;

	void DefineEdge( int v1, int v2, int planeNum ) {
		// check for degenerate edge
		if ( v1 == v2 ) {
			return;
		}
		int hashKey = silEdgeHash.GenerateKey( v1, v2 );
		// search for a matching other side
		for ( int i = silEdgeHash.First( hashKey ); i >= 0; i = silEdgeHash.Next( i ) ) {
			if ( silEdges[i].v1 == v1 && silEdges[i].v2 == v2 ) {
				c_duplicatedEdges++;
				// allow it to still create a new edge
				continue;
			}
			if ( silEdges[i].v2 == v1 && silEdges[i].v1 == v2 ) {
				if ( silEdges[i].p2 != numPlanes )  {
					c_tripledEdges++;
					// allow it to still create a new edge
					continue;
				}
				// this is a matching back side
				silEdges[i].p2 = planeNum;
				return;
			}

		}

		silEdge_t se;
		se.p1 = planeNum;
		se.p2 = numPlanes;
		se.v1 = v1;
		se.v2 = v2;
		silEdgeHash.Add( hashKey, silEdges.Num() );
		silEdges.AddGrow( se );
	}

	void Sort() {
		std::sort(silEdges.begin(), silEdges.end(), [](const silEdge_t &a, const silEdge_t &b) -> bool {
			if ( a.p1 != b.p1 )
				return a.p1 < b.p1;
			if ( a.p2 != b.p2 )
				return a.p2 < b.p2;
			return false;
		});
	}
};

/*
=================
R_IdentifySilEdges

If the surface will not deform, coplanar edges (polygon interiors)
can never create silhouette plains, and can be omited
=================
*/
void R_IdentifySilEdges( srfTriangles_t *tri, bool omitCoplanarEdges ) {
	omitCoplanarEdges = false;	// optimization doesn't work for some reason

	int numTris = tri->numIndexes / 3;

	SilEdgeGenerator gen;
	gen.numPlanes = numTris;
	int hashCells = idMath::Imax( idMath::CeilPowerOfTwo( numTris ), 32 );
	int hashItems = int( tri->numIndexes * 0.6 + 32 );		// 50% of index count for closed mesh
	gen.silEdgeHash.ClearFree( hashCells, hashItems );

	for ( int i = 0 ; i < numTris ; i++ ) {
		int		i1, i2, i3;

		i1 = tri->silIndexes[ i*3 + 0 ];
		i2 = tri->silIndexes[ i*3 + 1 ];
		i3 = tri->silIndexes[ i*3 + 2 ];

		// create the edges
		gen.DefineEdge( i1, i2, i );
		gen.DefineEdge( i2, i3, i );
		gen.DefineEdge( i3, i1, i );
	}

	if ( gen.c_duplicatedEdges || gen.c_tripledEdges ) {
		common->DWarning( "%i duplicated edge directions, %i tripled edges", gen.c_duplicatedEdges, gen.c_tripledEdges );
	}

	// stgatilov #5886: fill adjTris data with connectivity information
	tri->adjTris = triSilIndexAllocator.Alloc( tri->numIndexes );
	for ( int i = 0; i < tri->numIndexes; i++ )
		tri->adjTris[i] = numTris;
	for ( int i = 0; i < gen.silEdges.Num(); i++ ) {
		silEdge_t se = gen.silEdges[i];
		for ( int j = 0; j < 2; j++ ) {
			int fThis  = (j == 0 ? se.p1 : se.p2);
			int fOther = (j == 0 ? se.p2 : se.p1);
			if (fThis == numTris)
				continue;
			int cntFound = 0;
			for (int u = 0; u < 3; u++) {
				int va = tri->silIndexes[3 * fThis + (u+0)%3];
				int vb = tri->silIndexes[3 * fThis + (u+1)%3];
				if ( va == se.v1 && vb == se.v2 || va == se.v2 && vb == se.v1 ) {
					tri->adjTris[3 * fThis + (u+2)%3] = fOther;
					cntFound++;
				}
			}
		}
	}

	// if we know that the vertexes aren't going
	// to deform, we can remove interior triangulation edges
	// on otherwise planar polygons.
	// I earlier believed that I could also remove concave
	// edges, because they are never silhouettes in the conventional sense,
	// but they are still needed to balance out all the true sil edges
	// for the shadow algorithm to function
	int		c_coplanarCulled;

	c_coplanarCulled = 0;
	if ( omitCoplanarEdges ) {
		for ( int i = 0 ; i < gen.silEdges.Num() ; i++ ) {
			silEdge_t se = gen.silEdges[i];
			int			i1, i2, i3;
			int			base;
			int			j;

			if ( se.p2 == gen.numPlanes ) {	// the fake dangling edge
				continue;
			}

			base = se.p1 * 3;
			i1 = tri->silIndexes[ base + 0 ];
			i2 = tri->silIndexes[ base + 1 ];
			i3 = tri->silIndexes[ base + 2 ];

			//plane.FromPoints(tri->verts[i1].xyz , tri->verts[i2].xyz, tri->verts[i3].xyz );
			idVec3d pnt1(tri->verts[i1].xyz);
			idVec3d pnt2(tri->verts[i2].xyz);
			idVec3d pnt3(tri->verts[i3].xyz);
			idVec3d normal = (pnt2 - pnt1).Cross(pnt3 - pnt1);
			if (normal.LengthSqr() == 0.0) {
				continue;
			}

			// check to see if points of second triangle are not coplanar
			base = se.p2 * 3;
			for ( j = 0 ; j < 3 ; j++ ) {
				i1 = tri->silIndexes[ base + j ];
				//float d = plane.Distance( tri->verts[i1].xyz );
				idVec3d pntOther(tri->verts[i1].xyz);
				double d = (pntOther - pnt1).Dot(normal);
				if ( d != 0 ) {		// even a small epsilon causes problems
					break;
				}
			}

			if ( j == 3 ) {
				// we can cull this sil edge
				gen.silEdges.RemoveIndex( i );
				c_coplanarCulled++;
				i--;
			}
		}
	}

	// sort the sil edges based on plane number
	gen.Sort();

	// count up the distribution.
	// a perfectly built model should only have shared
	// edges, but most models will have some interpenetration
	// and dangling edges
	int shared = 0;
	int single = 0;
	for ( int i = 0 ; i < gen.silEdges.Num() ; i++ ) {
		if ( gen.silEdges[i].p2 == gen.numPlanes ) {
			single++;
		} else {
			shared++;
		}
	}

	if ( !single ) {
		tri->perfectHull = true;
	} else {
		tri->perfectHull = false;
	}

	tri->numSilEdges = gen.silEdges.Num();
	tri->silEdges = triSilEdgeAllocator.Alloc( tri->numSilEdges );
	memcpy( tri->silEdges, gen.silEdges.Ptr(), tri->numSilEdges * sizeof( tri->silEdges[0] ) );
}

/*
===============
R_FaceNegativePolarity

Returns true if the texture polarity of the face is negative, false if it is positive or zero
===============
*/
static bool R_FaceNegativePolarity( const srfTriangles_t *tri, int firstIndex ) {
	idDrawVert	*a, *b, *c;
	float	area;
	float		d0[5], d1[5];

	a = tri->verts + tri->indexes[firstIndex + 0];
	b = tri->verts + tri->indexes[firstIndex + 1];
	c = tri->verts + tri->indexes[firstIndex + 2];

	d0[3] = b->st[0] - a->st[0];
	d0[4] = b->st[1] - a->st[1];

	d1[3] = c->st[0] - a->st[0];
	d1[4] = c->st[1] - a->st[1];

	area = d0[3] * d1[4] - d0[4] * d1[3];
	if ( area >= 0 ) {
		return false;
	}
	return true;
}

/*
==================
R_DeriveFaceTangents
==================
*/
typedef struct {
	idVec3		tangents[2];
	bool	negativePolarity;
	bool	degenerate;
} faceTangents_t;

static void	R_DeriveFaceTangents( const srfTriangles_t *tri, faceTangents_t *faceTangents ) {
	int		i;
	int			c_textureDegenerateFaces;
	int			c_positive, c_negative;
	faceTangents_t	*ft;
	idDrawVert	*a, *b, *c;

	//
	// calculate tangent vectors for each face in isolation
	//
	c_positive = 0;
	c_negative = 0;
	c_textureDegenerateFaces = 0;
	for ( i = 0 ; i < tri->numIndexes ; i+=3 ) {
		float	area;
		idVec3	temp;
		float		d0[5], d1[5];

		ft = &faceTangents[i/3];

		a = tri->verts + tri->indexes[i + 0];
		b = tri->verts + tri->indexes[i + 1];
		c = tri->verts + tri->indexes[i + 2];

		d0[0] = b->xyz[0] - a->xyz[0];
		d0[1] = b->xyz[1] - a->xyz[1];
		d0[2] = b->xyz[2] - a->xyz[2];
		d0[3] = b->st[0] - a->st[0];
		d0[4] = b->st[1] - a->st[1];

		d1[0] = c->xyz[0] - a->xyz[0];
		d1[1] = c->xyz[1] - a->xyz[1];
		d1[2] = c->xyz[2] - a->xyz[2];
		d1[3] = c->st[0] - a->st[0];
		d1[4] = c->st[1] - a->st[1];

		area = d0[3] * d1[4] - d0[4] * d1[3];
		if ( fabs( area ) < 1e-20f ) {
			ft->negativePolarity = false;
			ft->degenerate = true;
			ft->tangents[0].Zero();
			ft->tangents[1].Zero();
			c_textureDegenerateFaces++;
			continue;
		}
		if ( area > 0.0f ) {
			ft->negativePolarity = false;
			c_positive++;
		} else {
			ft->negativePolarity = true;
			c_negative++;
		}
		ft->degenerate = false;

#ifdef USE_INVA
		float inva = area < 0.0f ? -1 : 1;		// was = 1.0f / area;

        temp[0] = (d0[0] * d1[4] - d0[4] * d1[0]) * inva;
        temp[1] = (d0[1] * d1[4] - d0[4] * d1[1]) * inva;
        temp[2] = (d0[2] * d1[4] - d0[4] * d1[2]) * inva;
		temp.Normalize();
		ft->tangents[0] = temp;
        
        temp[0] = (d0[3] * d1[0] - d0[0] * d1[3]) * inva;
        temp[1] = (d0[3] * d1[1] - d0[1] * d1[3]) * inva;
        temp[2] = (d0[3] * d1[2] - d0[2] * d1[3]) * inva;
		temp.Normalize();
		ft->tangents[1] = temp;
#else
        temp[0] = (d0[0] * d1[4] - d0[4] * d1[0]);
        temp[1] = (d0[1] * d1[4] - d0[4] * d1[1]);
        temp[2] = (d0[2] * d1[4] - d0[4] * d1[2]);
		temp.Normalize();
		ft->tangents[0] = temp;
        
        temp[0] = (d0[3] * d1[0] - d0[0] * d1[3]);
        temp[1] = (d0[3] * d1[1] - d0[1] * d1[3]);
        temp[2] = (d0[3] * d1[2] - d0[2] * d1[3]);
		temp.Normalize();
		ft->tangents[1] = temp;
#endif
	}
}



/*
===================
R_DuplicateMirroredVertexes

Modifies the surface to bust apart any verts that are shared by both positive and
negative texture polarities, so tangent space smoothing at the vertex doesn't
degenerate.

This will create some identical vertexes (which will eventually get different tangent
vectors), so never optimize the resulting mesh, or it will get the mirrored edges back.

Reallocates tri->verts and changes tri->indexes in place
Silindexes are unchanged by this.

sets mirroredVerts and mirroredVerts[]

===================
*/
typedef struct {
	bool	polarityUsed[2];
	int			negativeRemap;
} tangentVert_t;

static void	R_DuplicateMirroredVertexes( srfTriangles_t *tri ) {
	tangentVert_t	*tverts, *vert;
	int				i, j;
	int				totalVerts;
	int				numMirror;

	tverts = (tangentVert_t *)_alloca16( tri->numVerts * sizeof( *tverts ) );
	memset( tverts, 0, tri->numVerts * sizeof( *tverts ) );

	// determine texture polarity of each surface

	// mark each vert with the polarities it uses
	for ( i = 0 ; i < tri->numIndexes ; i+=3 ) {
		int	polarity;

		polarity = R_FaceNegativePolarity( tri, i );
		for ( j = 0 ; j < 3 ; j++ ) {
			tverts[tri->indexes[i+j]].polarityUsed[ polarity ] = true;
		}
	}

	// now create new verts as needed
	totalVerts = tri->numVerts;
	for ( i = 0 ; i < tri->numVerts ; i++ ) {
		vert = &tverts[i];
		if ( vert->polarityUsed[0] && vert->polarityUsed[1] ) {
			vert->negativeRemap = totalVerts;
			totalVerts++;	
		}
	}

	tri->numMirroredVerts = totalVerts - tri->numVerts;

	// now create the new list
	if ( totalVerts == tri->numVerts ) {
		tri->mirroredVerts = NULL;
		return;
	}

	tri->mirroredVerts = triMirroredVertAllocator.Alloc( tri->numMirroredVerts );

#ifdef USE_TRI_DATA_ALLOCATOR
	tri->verts = triVertexAllocator.Resize( tri->verts, totalVerts );
#else
	idDrawVert *oldVerts = tri->verts;
	tri->verts = NULL; // to pass internal R_AllocStaticTriSurfVerts check
	R_AllocStaticTriSurfVerts( tri, totalVerts );
	memcpy( tri->verts, oldVerts, tri->numVerts * sizeof( tri->verts[0] ) );
	triVertexAllocator.Free( oldVerts );
#endif

	// create the duplicates
	numMirror = 0;
	for ( i = 0 ; i < tri->numVerts ; i++ ) {
		j = tverts[i].negativeRemap;
		if ( j ) {
			tri->verts[j] = tri->verts[i];
			tri->mirroredVerts[numMirror] = i;
			numMirror++;
		}
	}

	tri->numVerts = totalVerts;
	// change the indexes
	for ( i = 0 ; i < tri->numIndexes ; i++ ) {
		if ( tverts[tri->indexes[i]].negativeRemap && 
			R_FaceNegativePolarity( tri, 3*(i/3) ) ) {
			tri->indexes[i] = tverts[tri->indexes[i]].negativeRemap;
		}
	}

	tri->numVerts = totalVerts;
}

/*
=================
R_DeriveTangentsWithoutNormals

Build texture space tangents for bump mapping
If a surface is deformed, this must be recalculated

This assumes that any mirrored vertexes have already been duplicated, so
any shared vertexes will have the tangent spaces smoothed across.

Texture wrapping slightly complicates this, but as long as the normals
are shared, and the tangent vectors are projected onto the normals, the
separate vertexes should wind up with identical tangent spaces.

mirroring a normalmap WILL cause a slightly visible seam unless the normals
are completely flat around the edge's full bilerp support.

Vertexes which are smooth shaded must have their tangent vectors
in the same plane, which will allow a seamless
rendering as long as the normal map is even on both sides of the
seam.

A smooth shaded surface may have multiple tangent vectors at a vertex
due to texture seams or mirroring, but it should only have a single
normal vector.

Each triangle has a pair of tangent vectors in it's plane

Should we consider having vertexes point at shared tangent spaces
to save space or speed transforms?

this version only handles bilateral symetry
=================
*/
void R_DeriveTangentsWithoutNormals( srfTriangles_t *tri ) {
	int			i, j;
	faceTangents_t	*faceTangents;
	faceTangents_t	*ft;
	idDrawVert		*vert;

	faceTangents = (faceTangents_t *)_alloca16( sizeof(faceTangents[0]) * tri->numIndexes/3 );
	R_DeriveFaceTangents( tri, faceTangents );

	// clear the tangents
	for ( i = 0 ; i < tri->numVerts ; i++ ) {
		tri->verts[i].tangents[0].Zero();
		tri->verts[i].tangents[1].Zero();
	}

	// sum up the neighbors
	for ( i = 0 ; i < tri->numIndexes ; i+=3 ) {
		ft = &faceTangents[i/3];

		// for each vertex on this face
		for ( j = 0 ; j < 3 ; j++ ) {
			vert = &tri->verts[tri->indexes[i+j]];

			vert->tangents[0] += ft->tangents[0];
			vert->tangents[1] += ft->tangents[1];
		}
	}

#if 0
	// sum up both sides of the mirrored verts
	// so the S vectors exactly mirror, and the T vectors are equal
	for ( i = 0 ; i < tri->numMirroredVerts ; i++ ) {
		idDrawVert	*v1, *v2;

		v1 = &tri->verts[ tri->numVerts - tri->numMirroredVerts + i ];
		v2 = &tri->verts[ tri->mirroredVerts[i] ];

		v1->tangents[0] -= v2->tangents[0];
		v1->tangents[1] += v2->tangents[1];

		v2->tangents[0] = vec3_origin - v1->tangents[0];
		v2->tangents[1] = v1->tangents[1];
	}
#endif


	// project the summed vectors onto the normal plane
	// and normalize.  The tangent vectors will not necessarily
	// be orthogonal to each other, but they will be orthogonal
	// to the surface normal.
	for ( i = 0 ; i < tri->numVerts ; i++ ) {
		vert = &tri->verts[i];
		for ( j = 0 ; j < 2 ; j++ ) {
			float	d;

			d = vert->tangents[j] * vert->normal;
			vert->tangents[j] = vert->tangents[j] - d * vert->normal;
			vert->tangents[j].Normalize();
		}
	}

	tri->tangentsCalculated = true;
}

static ID_INLINE void VectorNormalizeFast2( const idVec3 &v, idVec3 &out) {
	float	ilength;

	ilength = idMath::RSqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
	out[0] = v[0] * ilength;
	out[1] = v[1] * ilength;
	out[2] = v[2] * ilength;
}

/*
===================
R_BuildDominantTris

Find the largest triangle that uses each vertex
===================
*/
typedef struct {
	int		vertexNum;
	int		faceNum;
} indexSort_t;

static int IndexSort( const void *a, const void *b ) {
	if ( ((indexSort_t *)a)->vertexNum < ((indexSort_t *)b)->vertexNum ) {
		return -1;
	}
	if ( ((indexSort_t *)a)->vertexNum > ((indexSort_t *)b)->vertexNum ) {
		return 1;
	}
	return 0;
}

void R_BuildDominantTris( srfTriangles_t *tri ) {
	int i, j;
	dominantTri_t *dt;
	indexSort_t *ind = (indexSort_t *)R_StaticAlloc( tri->numIndexes * sizeof( *ind ) );

	for ( i = 0; i < tri->numIndexes; i++ ) {
		ind[i].vertexNum = tri->indexes[i];
		ind[i].faceNum = i / 3;
	}
	qsort( ind, tri->numIndexes, sizeof( *ind ), IndexSort );

	tri->dominantTris = dt = triDominantTrisAllocator.Alloc( tri->numVerts );
	memset( dt, 0, tri->numVerts * sizeof( dt[0] ) );

	for ( i = 0; i < tri->numIndexes; i += j ) {
		float	maxArea = 0;
		int		vertNum = ind[i].vertexNum;
		for ( j = 0; i + j < tri->numIndexes && ind[i+j].vertexNum == vertNum; j++ ) {
			float		d0[5], d1[5];
			idDrawVert	*a, *b, *c;
			idVec3		normal, tangent, bitangent;

			int	i1 = tri->indexes[ind[i+j].faceNum * 3 + 0];
			int	i2 = tri->indexes[ind[i+j].faceNum * 3 + 1];
			int	i3 = tri->indexes[ind[i+j].faceNum * 3 + 2];
			
			a = tri->verts + i1;
			b = tri->verts + i2;
			c = tri->verts + i3;

			d0[0] = b->xyz[0] - a->xyz[0];
			d0[1] = b->xyz[1] - a->xyz[1];
			d0[2] = b->xyz[2] - a->xyz[2];
			d0[3] = b->st[0] - a->st[0];
			d0[4] = b->st[1] - a->st[1];

			d1[0] = c->xyz[0] - a->xyz[0];
			d1[1] = c->xyz[1] - a->xyz[1];
			d1[2] = c->xyz[2] - a->xyz[2];
			d1[3] = c->st[0] - a->st[0];
			d1[4] = c->st[1] - a->st[1];

			normal[0] = ( d1[1] * d0[2] - d1[2] * d0[1] );
			normal[1] = ( d1[2] * d0[0] - d1[0] * d0[2] );
			normal[2] = ( d1[0] * d0[1] - d1[1] * d0[0] );

			float area = normal.Length();

			// if this is smaller than what we already have, skip it
			if ( area < maxArea ) {
				continue;
			}
			maxArea = area;

			if ( i1 == vertNum ) {
				dt[vertNum].v2 = i2;
				dt[vertNum].v3 = i3;
			} else if ( i2 == vertNum ) {
				dt[vertNum].v2 = i3;
				dt[vertNum].v3 = i1;
			} else {
				dt[vertNum].v2 = i1;
				dt[vertNum].v3 = i2;
			}

			float	len = area;
			if ( len < 0.001f ) {
				len = 0.001f;
			}
			dt[vertNum].normalizationScale[2] = 1.0f / len;		// normal

			// texture area
			area = d0[3] * d1[4] - d0[4] * d1[3];

			tangent[0] = ( d0[0] * d1[4] - d0[4] * d1[0] );
			tangent[1] = ( d0[1] * d1[4] - d0[4] * d1[1] );
			tangent[2] = ( d0[2] * d1[4] - d0[4] * d1[2] );
			len = tangent.Length();
			if ( len < 0.001f ) {
				len = 0.001f;
			}
			dt[vertNum].normalizationScale[0] = ( area > 0 ? 1 : -1 ) / len;	// tangents[0]
	        
			bitangent[0] = ( d0[3] * d1[0] - d0[0] * d1[3] );
			bitangent[1] = ( d0[3] * d1[1] - d0[1] * d1[3] );
			bitangent[2] = ( d0[3] * d1[2] - d0[2] * d1[3] );
			len = bitangent.Length();
			if ( len < 0.001f ) {
				len = 0.001f;
			}
#ifdef DERIVE_UNSMOOTHED_BITANGENT
			dt[vertNum].normalizationScale[1] = ( area > 0 ? 1 : -1 );
#else
			dt[vertNum].normalizationScale[1] = ( area > 0 ? 1 : -1 ) / len;	// tangents[1]
#endif
		}
	}

	R_StaticFree( ind );
}

/*
====================
R_DeriveUnsmoothedTangents

Uses the single largest area triangle for each vertex, instead of smoothing over all
====================
*/
void R_DeriveUnsmoothedTangents( srfTriangles_t *tri ) {
	if ( tri->tangentsCalculated ) {
		return;
	}

#if 1

	SIMDProcessor->DeriveUnsmoothedTangents( tri->verts, tri->dominantTris, tri->numVerts );

#else

	for ( int i = 0 ; i < tri->numVerts ; i++ ) {
		idVec3		temp;
		float		d0[5], d1[5];
		idDrawVert	*a, *b, *c;
		dominantTri_t	*dt = &tri->dominantTris[i];

		a = tri->verts + i;
		b = tri->verts + dt->v2;
		c = tri->verts + dt->v3;

		d0[0] = b->xyz[0] - a->xyz[0];
		d0[1] = b->xyz[1] - a->xyz[1];
		d0[2] = b->xyz[2] - a->xyz[2];
		d0[3] = b->st[0] - a->st[0];
		d0[4] = b->st[1] - a->st[1];

		d1[0] = c->xyz[0] - a->xyz[0];
		d1[1] = c->xyz[1] - a->xyz[1];
		d1[2] = c->xyz[2] - a->xyz[2];
		d1[3] = c->st[0] - a->st[0];
		d1[4] = c->st[1] - a->st[1];

		a->normal[0] = dt->normalizationScale[2] * ( d1[1] * d0[2] - d1[2] * d0[1] );
		a->normal[1] = dt->normalizationScale[2] * ( d1[2] * d0[0] - d1[0] * d0[2] );
		a->normal[2] = dt->normalizationScale[2] * ( d1[0] * d0[1] - d1[1] * d0[0] );

		a->tangents[0][0] = dt->normalizationScale[0] * ( d0[0] * d1[4] - d0[4] * d1[0] );
		a->tangents[0][1] = dt->normalizationScale[0] * ( d0[1] * d1[4] - d0[4] * d1[1] );
		a->tangents[0][2] = dt->normalizationScale[0] * ( d0[2] * d1[4] - d0[4] * d1[2] );

#ifdef DERIVE_UNSMOOTHED_BITANGENT
		// derive the bitangent for a completely orthogonal axis,
		// instead of using the texture T vector
		a->tangents[1][0] = dt->normalizationScale[1] * ( a->normal[2] * a->tangents[0][1] - a->normal[1] * a->tangents[0][2] );
		a->tangents[1][1] = dt->normalizationScale[1] * ( a->normal[0] * a->tangents[0][2] - a->normal[2] * a->tangents[0][0] );
		a->tangents[1][2] = dt->normalizationScale[1] * ( a->normal[1] * a->tangents[0][0] - a->normal[0] * a->tangents[0][1] );
#else
		// calculate the bitangent from the texture T vector
		a->tangents[1][0] = dt->normalizationScale[1] * ( d0[3] * d1[0] - d0[0] * d1[3] );
		a->tangents[1][1] = dt->normalizationScale[1] * ( d0[3] * d1[1] - d0[1] * d1[3] );
		a->tangents[1][2] = dt->normalizationScale[1] * ( d0[3] * d1[2] - d0[2] * d1[3] );
#endif
	}

#endif

	tri->tangentsCalculated = true;
}

/*
==================
R_DeriveTangents

This is called once for static surfaces, and every frame for deforming surfaces

Builds tangents, normals, and face planes
==================
*/
void R_DeriveTangents( srfTriangles_t *tri, bool allocFacePlanes ) {
	int				i;
	idPlane			*planes;

	if ( tri->dominantTris != NULL ) {
		R_DeriveUnsmoothedTangents( tri );
		return;
	}

	if ( tri->tangentsCalculated ) {
		return;
	}

	tr.pc.c_tangentIndexes += tri->numIndexes;

	if ( !tri->facePlanes && allocFacePlanes ) {
		R_AllocStaticTriSurfPlanes( tri, tri->numIndexes );
	}
	planes = tri->facePlanes;

	if ( !planes ) {
		planes = (idPlane *)_alloca16( ( tri->numIndexes / 3 ) * sizeof( planes[0] ) );
	}

	SIMDProcessor->DeriveTangents( planes, tri->verts, tri->numVerts, tri->indexes, tri->numIndexes );

	int *dupVerts = tri->dupVerts;
	idDrawVert *verts = tri->verts;

	// add the normal of a duplicated vertex to the normal of the first vertex with the same XYZ
	for ( i = 0; i < tri->numDupVerts; i++ ) {
		verts[dupVerts[i*2+0]].normal += verts[dupVerts[i*2+1]].normal;
	}

	// copy vertex normals to duplicated vertices
	for ( i = 0; i < tri->numDupVerts; i++ ) {
		verts[dupVerts[i*2+1]].normal = verts[dupVerts[i*2+0]].normal;
	}

	// project the summed vectors onto the normal plane
	// and normalize.  The tangent vectors will not necessarily
	// be orthogonal to each other, but they will be orthogonal
	// to the surface normal.
	SIMDProcessor->NormalizeTangents( tri->verts, tri->numVerts );

	tri->tangentsCalculated = true;
	tri->facePlanesCalculated = true;
}

/*
=================
R_RemoveDuplicatedTriangles

silIndexes must have already been calculated

silIndexes are used instead of indexes, because duplicated
triangles could have different texture coordinates.
=================
*/
void R_RemoveDuplicatedTriangles( srfTriangles_t *tri ) {
	int		c_removed;
	int		i, j, r;
	int		a, b, c;

	c_removed = 0;

	// check for completely duplicated triangles
	// any rotation of the triangle is still the same, but a mirroring
	// is considered different
	for ( i = 0 ; i < tri->numIndexes ; i+=3 ) {
		for ( r = 0 ; r < 3 ; r++ ) {
			a = tri->silIndexes[i+r];
			b = tri->silIndexes[i+(r+1)%3];
			c = tri->silIndexes[i+(r+2)%3];
			for ( j = i + 3 ; j < tri->numIndexes ; j+=3 ) {
				if ( tri->silIndexes[j] == a && tri->silIndexes[j+1] == b && tri->silIndexes[j+2] == c ) {
					c_removed++;
					memmove( tri->indexes + j, tri->indexes + j + 3, ( tri->numIndexes - j - 3 ) * sizeof( tri->indexes[0] ) );
					memmove( tri->silIndexes + j, tri->silIndexes + j + 3, ( tri->numIndexes - j - 3 ) * sizeof( tri->silIndexes[0] ) );
					tri->numIndexes -= 3;
					j -= 3;
				}
			}
		}
	}

	if ( c_removed ) {
		common->Printf( "removed %i duplicated triangles\n", c_removed );
	}

}

/*
=================
R_RemoveDegenerateTriangles

silIndexes must have already been calculated
=================
*/
void R_RemoveDegenerateTriangles( srfTriangles_t *tri ) {
	int		c_removed;
	int		i;
	int		a, b, c;

	// check for completely degenerate triangles
	c_removed = 0;
	for ( i = 0; i < tri->numIndexes; i += 3 ) {
		a = tri->silIndexes[i];
		b = tri->silIndexes[i+1];
		c = tri->silIndexes[i+2];
		if ( a == b || a == c || b == c ) {
			c_removed++;
			memmove( tri->indexes + i, tri->indexes + i + 3, ( tri->numIndexes - i - 3 ) * sizeof( tri->indexes[0] ) );
			if ( tri->silIndexes ) {
				memmove( tri->silIndexes + i, tri->silIndexes + i + 3, ( tri->numIndexes - i - 3 ) * sizeof( tri->silIndexes[0] ) );
			}
			tri->numIndexes -= 3;
			i -= 3;
		}
	}

	// this doesn't free the memory used by the unused verts
	/* grayman - message is useless, since the mapper can't do anything with it.
	   it also confuses new mappers, who always ask about it.
	if ( c_removed ) {
		common->Printf( "removed %i degenerate triangles\n", c_removed );
	}
	*/
}

/*
=================
R_TestDegenerateTextureSpace
=================
*/
void R_TestDegenerateTextureSpace( srfTriangles_t *tri ) {
	int		c_degenerate;
	int		i;

	// check for triangles with a degenerate texture space
	c_degenerate = 0;
	for ( i = 0; i < tri->numIndexes; i += 3 ) {
		const idDrawVert &a = tri->verts[tri->indexes[i+0]];
		const idDrawVert &b = tri->verts[tri->indexes[i+1]];
		const idDrawVert &c = tri->verts[tri->indexes[i+2]];

		if ( a.st == b.st || b.st == c.st || c.st == a.st ) {
			c_degenerate++;
		}
	}

	if ( c_degenerate ) {
//		common->Printf( "%d triangles with a degenerate texture space\n", c_degenerate );
	}
}

/*
=================
R_RemoveUnusedVerts
=================
*/
void R_RemoveUnusedVerts( srfTriangles_t *tri ) {
	int		i;
	int		*mark;
	int		index;
	int		used;

	mark = (int *)R_ClearedStaticAlloc( tri->numVerts * sizeof( *mark ) );

	for ( i = 0 ; i < tri->numIndexes ; i++ ) {
		index = tri->indexes[i];
		if ( index < 0 || index >= tri->numVerts ) {
			common->Error( "R_RemoveUnusedVerts: bad index" );
		}
		mark[ index ] = 1;

		if ( tri->silIndexes ) {
			index = tri->silIndexes[i];
			if ( index < 0 || index >= tri->numVerts ) {
				common->Error( "R_RemoveUnusedVerts: bad index" );
			}
			mark[ index ] = 1;
		}
	}

	used = 0;
	for ( i = 0 ; i < tri->numVerts ; i++ ) {
		if ( !mark[i] ) {
			continue;
		}
		mark[i] = used + 1;
		used++;
	}

	if ( used != tri->numVerts ) {
		for ( i = 0 ; i < tri->numIndexes ; i++ ) {
			tri->indexes[i] = mark[ tri->indexes[i] ] - 1;
			if ( tri->silIndexes ) {
				tri->silIndexes[i] = mark[ tri->silIndexes[i] ] - 1;
			}
		}
		tri->numVerts = used;

		for ( i = 0 ; i < tri->numVerts ; i++ ) {
			index = mark[ i ];
			if ( !index ) {
				continue;
			}
			tri->verts[ index - 1 ] = tri->verts[i];
		}

		// this doesn't realloc the arrays to save the memory used by the unused verts
	}

	R_StaticFree( mark );
}

/*
=================
R_MergeSurfaceList

Only deals with vertexes and indexes, not silhouettes, planes, etc.
Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
=================
*/
srfTriangles_t	*R_MergeSurfaceList( const srfTriangles_t **surfaces, int numSurfaces ) {
	srfTriangles_t	*newTri;
	const srfTriangles_t	*tri;
	int				i, j;
	int				totalVerts;
	int				totalIndexes;

	totalVerts = 0;
	totalIndexes = 0;
	for ( i = 0 ; i < numSurfaces ; i++ ) {
		totalVerts += surfaces[i]->numVerts;
		totalIndexes += surfaces[i]->numIndexes;
	}

	newTri = R_AllocStaticTriSurf();
	newTri->numVerts = totalVerts;
	newTri->numIndexes = totalIndexes;
	R_AllocStaticTriSurfVerts( newTri, newTri->numVerts );
	R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );

	totalVerts = 0;
	totalIndexes = 0;
	for ( i = 0 ; i < numSurfaces ; i++ ) {
		tri = surfaces[i];
		memcpy( newTri->verts + totalVerts, tri->verts, tri->numVerts * sizeof( *tri->verts ) );
		for ( j = 0 ; j < tri->numIndexes ; j++ ) {
			newTri->indexes[ totalIndexes + j ] = totalVerts + tri->indexes[j];
		}
		totalVerts += tri->numVerts;
		totalIndexes += tri->numIndexes;
	}

	return newTri;
}

/*
=================
R_MergeTriangles

Only deals with vertexes and indexes, not silhouettes, planes, etc.
Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
=================
*/
srfTriangles_t	*R_MergeTriangles( const srfTriangles_t *tri1, const srfTriangles_t *tri2 ) {
	const srfTriangles_t	*tris[2];

	tris[0] = tri1;
	tris[1] = tri2;

	return R_MergeSurfaceList( tris, 2 );
}

/*
=================
R_ReverseTriangles

Lit two sided surfaces need to have the triangles actually duplicated,
they can't just turn on two sided lighting, because the normal and tangents
are wrong on the other sides.

This should be called before R_CleanupTriangles
=================
*/
void R_ReverseTriangles( srfTriangles_t *tri ) {
	int			i;

	// flip the normal on each vertex
	// If the surface is going to have generated normals, this won't matter,
	// but if it has explicit normals, this will keep it on the correct side
	for ( i = 0 ; i < tri->numVerts ; i++ ) {
		tri->verts[i].normal = vec3_origin - tri->verts[i].normal;
	}

	// flip the index order to make them back sided
	for ( i = 0 ; i < tri->numIndexes ; i+= 3 ) {
		glIndex_t	temp;

		temp = tri->indexes[ i + 0 ];
		tri->indexes[ i + 0 ] = tri->indexes[ i + 1 ];
		tri->indexes[ i + 1 ] = temp;
	}
}

/*
=================
R_CleanupTriangles

FIXME: allow createFlat and createSmooth normals, as well as explicit
=================
*/
void R_CleanupTriangles( srfTriangles_t *tri, bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents ) {
	R_RangeCheckIndexes( tri );

	R_CreateSilIndexes( tri );

//	R_RemoveDuplicatedTriangles( tri );	// this may remove valid overlapped transparent triangles

	R_RemoveDegenerateTriangles( tri );

	R_TestDegenerateTextureSpace( tri );

//	R_RemoveUnusedVerts( tri );

	if ( identifySilEdges ) {
		R_IdentifySilEdges( tri, true );	// assume it is non-deformable, and omit coplanar edges
	}

	// bust vertexes that share a mirrored edge into separate vertexes
	R_DuplicateMirroredVertexes( tri );

	// optimize the index order (not working?)
//	R_OrderIndexes( tri->numIndexes, tri->indexes );

	// stgatilov #5886: create BVH structure for acceleration
	// note: changes order of triangles!
	R_BuildBvhForTri( tri );

	R_CreateDupVerts( tri );

	R_BoundTriSurf( tri );

	if ( useUnsmoothedTangents ) {
		R_BuildDominantTris( tri );
		R_DeriveUnsmoothedTangents( tri );
	} else if ( !createNormals ) {
		R_DeriveFacePlanes( tri );
		R_DeriveTangentsWithoutNormals( tri );
	} else {
		R_DeriveTangents( tri );
	}
}

/*
===================================================================================

DEFORMED SURFACES

===================================================================================
*/

/*
===================
R_BuildDeformInfo
===================
*/
deformInfo_t *R_BuildDeformInfo( int numVerts, const idDrawVert *verts, int numIndexes, const int *indexes, bool useUnsmoothedTangents ) {
	deformInfo_t	*deform;
	srfTriangles_t	tri;
	int				i;

	memset( &tri, 0, sizeof( tri ) );

	tri.numVerts = numVerts;
	R_AllocStaticTriSurfVerts( &tri, tri.numVerts );
	SIMDProcessor->Memcpy( tri.verts, verts, tri.numVerts * sizeof( tri.verts[0] ) );

	tri.numIndexes = numIndexes;
	R_AllocStaticTriSurfIndexes( &tri, tri.numIndexes );

	// don't memcpy, so we can change the index type from int to short without changing the interface
	for ( i = 0 ; i < tri.numIndexes ; i++ ) {
		tri.indexes[i] = indexes[i];
	}

	R_RangeCheckIndexes( &tri );
	R_CreateSilIndexes( &tri );

// should we order the indexes here?

//	R_RemoveDuplicatedTriangles( &tri );
//	R_RemoveDegenerateTriangles( &tri );
//	R_RemoveUnusedVerts( &tri );
	R_IdentifySilEdges( &tri, false );			// we cannot remove coplanar edges, because
												// they can deform to silhouettes

	R_DuplicateMirroredVertexes( &tri );		// split mirror points into multiple points

	R_CreateDupVerts( &tri );

	if ( useUnsmoothedTangents ) {
		R_BuildDominantTris( &tri );
	}

	deform = (deformInfo_t *)R_ClearedStaticAlloc( sizeof( *deform ) );

	deform->numSourceVerts = numVerts;
	deform->numOutputVerts = tri.numVerts;

	deform->numIndexes = numIndexes;
	deform->indexes = tri.indexes;

	deform->silIndexes = tri.silIndexes;

	deform->numSilEdges = tri.numSilEdges;
	deform->silEdges = tri.silEdges;

	deform->dominantTris = tri.dominantTris;

	deform->numMirroredVerts = tri.numMirroredVerts;
	deform->mirroredVerts = tri.mirroredVerts;

	deform->numDupVerts = tri.numDupVerts;
	deform->dupVerts = tri.dupVerts;

	if ( tri.verts ) {
		triVertexAllocator.Free( tri.verts );
	}

	if ( tri.facePlanes ) {
		triPlaneAllocator.Free( tri.facePlanes );
	}

	return deform;
}

/*
===================
R_FreeDeformInfo
===================
*/
void R_FreeDeformInfo( deformInfo_t *deformInfo ) {
	if ( deformInfo->indexes != NULL ) {
		triIndexAllocator.Free( deformInfo->indexes );
	}
	if ( deformInfo->silIndexes != NULL ) {
		triSilIndexAllocator.Free( deformInfo->silIndexes );
	}
	if ( deformInfo->silEdges != NULL ) {
		triSilEdgeAllocator.Free( deformInfo->silEdges );
	}
	if ( deformInfo->dominantTris != NULL ) {
		triDominantTrisAllocator.Free( deformInfo->dominantTris );
	}
	if ( deformInfo->mirroredVerts != NULL ) {
		triMirroredVertAllocator.Free( deformInfo->mirroredVerts );
	}
	if ( deformInfo->dupVerts != NULL ) {
		triDupVertAllocator.Free( deformInfo->dupVerts );
	}
	R_StaticFree( deformInfo );
}

/*
===================
R_DeformInfoMemoryUsed
===================
*/
int R_DeformInfoMemoryUsed( deformInfo_t *deformInfo ) {
	int total = 0;

	if ( deformInfo->indexes != NULL ) {
		total += deformInfo->numIndexes * sizeof( deformInfo->indexes[0] );
	}
	if ( deformInfo->silIndexes != NULL ) {
		total += deformInfo->numIndexes * sizeof( deformInfo->silIndexes[0] );
	}
	if ( deformInfo->silEdges != NULL ) {
		total += deformInfo->numSilEdges * sizeof( deformInfo->silEdges[0] );
	}
	if ( deformInfo->dominantTris != NULL ) {
		total += deformInfo->numSourceVerts * sizeof( deformInfo->dominantTris[0] );
	}
	if ( deformInfo->mirroredVerts != NULL ) {
		total += deformInfo->numMirroredVerts * sizeof( deformInfo->mirroredVerts[0] );
	}
	if ( deformInfo->dupVerts != NULL ) {
		total += deformInfo->numDupVerts * sizeof( deformInfo->dupVerts[0] );
	}

	total += sizeof( *deformInfo );
	return total;
}

/*
===================
R_CreateStaticBuffersForTri

For static surfaces, the indexes, ambient, and shadow buffers can be pre-created at load
time, rather than being re-created each frame in the frame temporary buffers.
===================
*/
void R_CreateStaticBuffersForTri( srfTriangles_t & tri ) {
	tri.indexCache = NO_CACHE;
	tri.ambientCache = NO_CACHE;
	tri.shadowCache = NO_CACHE;

	// index cache
	if( tri.indexes != NULL && tri.numIndexes > 0 ) {
		tri.indexCache = vertexCache.AllocStaticIndex( tri.indexes, tri.numIndexes * sizeof( tri.indexes[0] ) );
	}

	// vertex cache
	if( tri.verts != NULL && tri.numVerts > 0 ) {
		tri.ambientCache = vertexCache.AllocStaticVertex( tri.verts, tri.numVerts * sizeof( tri.verts[0] ) );
	}

	// turbo shadow cache
	if( tri.verts != NULL && tri.numVerts > 0 ) {
		size_t shadowSize = ALIGN( tri.numVerts * 2 * sizeof( shadowCache_t ), 16 );
		shadowCache_t *shadowCache = (shadowCache_t *)Mem_Alloc( shadowSize );
		SIMDProcessor->CreateVertexProgramShadowCache( &shadowCache->xyz, tri.verts, tri.numVerts );
		tri.shadowCache = vertexCache.AllocStaticShadow( shadowCache, shadowSize );
	}
}

idCVar r_modelBvhBuild(
	"r_modelBvhBuild", "1",
	CVAR_RENDERER | CVAR_BOOL,
	"If set to 0, then BVH is not built on model load, thus all other BVH features won't work.\n"
	"Note: full game restart is required for the change to take effect!"
);
idCVar r_modelBvhLeafSize(
	"r_modelBvhLeafSize", "32",
	CVAR_RENDERER | CVAR_INTEGER,
	"Controls number of triangles in BVH leafs. "
	"If zero, then default setting is used.\n"
	"Note: takes effect only if set immediately after fresh game start! ",
	1, 1<<20
);

/*
===================
R_BuildBvhForTri

stgatilov #5886: Builds BVH tree for triangles listed in "indexes" array
Note: reorders triangles in process!
===================
*/
void R_BuildBvhForTri( srfTriangles_t *tri ) {
	if (tri->bvhNodes) {
		triBvhAllocator.Free(tri->bvhNodes);
		tri->bvhNodes = nullptr;
		tri->numBvhNodes = 0;
	}
	if (!r_modelBvhBuild.GetBool())
		return;

	TRACE_CPU_SCOPE("BuildBvhForTri");
	assert(!tri->indexCache.IsValid());

	int n = tri->numIndexes / 3;

	// generate one element per triangle
	idList<bvhElement_t> elems;
	elems.SetNum(n);
	for (int i = 0; i < n; i++) {
		int a = tri->indexes[3 * i + 0];
		int b = tri->indexes[3 * i + 1];
		int c = tri->indexes[3 * i + 2];
		idVec3 posA = tri->verts[a].xyz;
		idVec3 posB = tri->verts[b].xyz;
		idVec3 posC = tri->verts[c].xyz;
		bvhElement_t &e = elems[i];
		e.bounds = idBounds(posA);
		e.bounds.AddPoint(posB);
		e.bounds.AddPoint(posC);
		// quite often we see stretched patches, where pairs of triangles (quads) are long but narrow
		// using box center is much better than using triangle centroid in such case
		// (with centroid, odd tris go into one cluster, and even tris go to another)
		e.center = e.bounds.GetCenter();
		//e.center = (posA + posB + posC) * (1.0f / 3.0f);
		idVec3 normal = (posB - posA).Cross(posC - posA);
		normal.Normalize();
		e.direction = normal;
		e.id = i;
	}

	// create BVH tree
	idBvhCreator creator;
	creator.SetLeafSize(r_modelBvhLeafSize.GetInteger());
	creator.Build(n, elems.Ptr());

	// save triangles remap: old index -> new index
	idList<int> triangleRemap;
	triangleRemap.SetNum(n + 1);
	triangleRemap[n] = n;	// missing triangle is denoted as N in silEdges

	// reorder triangle indices
	idList<int> triIdsBuff;
	triIdsBuff.SetNum(3 * n);
	memcpy(triIdsBuff.Ptr(), tri->indexes, triIdsBuff.Allocated());
	for (int i = 0; i < n; i++) {
		int src = creator.GetIdAtPos(i);
		tri->indexes[3 * i + 0] = triIdsBuff[3 * src + 0];
		tri->indexes[3 * i + 1] = triIdsBuff[3 * src + 1];
		tri->indexes[3 * i + 2] = triIdsBuff[3 * src + 2];
		triangleRemap[src] = i;
	}

	if (tri->silEdges) {
		// update triangle indices in silhouette edges
		for (int i = 0; i < tri->numSilEdges; i++) {
			int &a = tri->silEdges[i].p1;
			int &b = tri->silEdges[i].p2;
			a = triangleRemap[a];
			b = triangleRemap[b];
		}
	}

	if (tri->silIndexes) {
		// reorder triangle indices for shadow volumes
		triIdsBuff.SetNum(3 * n);
		memcpy(triIdsBuff.Ptr(), tri->silIndexes, triIdsBuff.Allocated());
		for (int i = 0; i < n; i++) {
			int src = creator.GetIdAtPos(i);
			tri->silIndexes[3 * i + 0] = triIdsBuff[3 * src + 0];
			tri->silIndexes[3 * i + 1] = triIdsBuff[3 * src + 1];
			tri->silIndexes[3 * i + 2] = triIdsBuff[3 * src + 2];
		}
	}

	if (tri->adjTris) {
		// update triangle adjacency information
		triIdsBuff.SetNum(3 * n);
		memcpy(triIdsBuff.Ptr(), tri->adjTris, triIdsBuff.Allocated());
		for (int i = 0; i < n; i++) {
			int src = creator.GetIdAtPos(i);
			tri->adjTris[3 * i + 0] = triangleRemap[triIdsBuff[3 * src + 0]];
			tri->adjTris[3 * i + 1] = triangleRemap[triIdsBuff[3 * src + 1]];
			tri->adjTris[3 * i + 2] = triangleRemap[triIdsBuff[3 * src + 2]];
		}
	}

	assert(tri->facePlanesCalculated == false && tri->facePlanes == NULL);

	// copy compressed BVH into surface
	tri->bounds = creator.GetRootBounds();
	tri->numBvhNodes = creator.GetNodesNumber();
	tri->bvhNodes = (bvhNode_t*)triBvhAllocator.Alloc(tri->numBvhNodes);
	creator.CopyNodes(tri->bvhNodes);
}


static inline void AddSegmentToList( idFlexList<bvhTriRange_t, 128> &outIntervals, int beg, int num, int info, const idBounds &bounds ) {
	int n = outIntervals.Num();
	// intervals must be added in sorted order
	assert(n == 0 || beg >= outIntervals[n-1].end);
	if ( n && outIntervals[n-1].end == beg && outIntervals[n-1].info == info ) {
		// coalesce new interval with the last one
		outIntervals[n-1].end = beg + num;
		( (idBounds*) &outIntervals[n-1].box )->AddBounds( bounds );
	}
	else {
		// add as a new interval
		bvhTriRange_t interval = { beg, beg + num, info };
		*( (idBounds*) &interval.box ) = bounds;
		outIntervals.AddGrow( interval );
	}
}

/*
===================
R_CullBvhByFrustumAndOrigin

Filters triangles using BVH tree:
  1) Cull away elements outside frustum defined by six planes looking inwards ("frustum").
  2) Drop elements facing away/towards from "origin".
    "filterOri" =  0: skip check;
    "filterOri" =  1: drop backfacing, return frontfacing
    "filterOri" = -1: drop frontfacing, return backfacing

Output is returned as set of contiguous intervals (ranges).

Each interval has "info" bitmask, which says whether p.1 and/or p.2 is surely true for all triangles within it.
If a bit is missing in this bitmask, then it is uncertain whether respective condition is true or false for any of the triangles.
Of course, if p.1 or p.2 is surely false for all triangles in an interval, the interval is dropped and not returned.

"info" mask is ORed with "forceUnknown" parameter.
For instance, you can set "forceUnknown" = BVH_TRI_SURELY_GOOD_ORI if you don't need strict culling of backfacing triangles.
This allows to reduce number of intervals in the output, since intervals with equal "info" can be coalesced.

Also, we turn all nodes with <= "granularity" elements into leaves, which can also be used to reduce number of intervals.
Pass zero granularity is you want to recurse through the whole tree.

Also, bounding box is returned for each interval.
Caller can of course compute them himself, but that would be slower.
===================
*/
void R_CullBvhByFrustumAndOrigin(
	const idBounds &rootBounds, const bvhNode_t *nodes,
	const idPlane frustum[6], int filterOri, const idVec3 &origin,
	int forceUnknown, int granularity,
	idFlexList<bvhTriRange_t, 128> &outIntervals
) {
	outIntervals.Clear();

	idRenderMatrix::CullSixPlanes cull;
	cull.Prepare( frustum );

	struct Traverser {
		const bvhNode_t *nodes;
		const idPlane *frustum;
		int filterOri;
		const idVec3 &origin;
		int forceUnknown;
		int granularity;
		const idRenderMatrix::CullSixPlanes &cull;
		idFlexList<bvhTriRange_t, 128> &intervals;

		void Traverse( int nodeIdx, const idBounds &parentBounds ) {
			const bvhNode_t &node = nodes[nodeIdx];
			idBounds bounds = node.GetBounds( parentBounds );

			// info about which checks surely pass for all triangles of the node
			int info = 0;

			// check node's bounding box against light frustum
			byte allOutBits, anyOutBits;
			cull.CullBounds( frustum, bounds, &allOutBits, &anyOutBits );
			if ( allOutBits != 0 )						// out of light
				return;
			if ( anyOutBits == 0 )
				info |= BVH_TRI_SURELY_WITHIN_LIGHT;	// fully within light

			if ( filterOri != 0 ) {
				// check node's normal cone against light->box directions
				int facing = node.HaveSameDirection( origin, bounds );
				if ( facing == 0 )
					;									// uncertain
				else if ( facing == filterOri )
					info |= BVH_TRI_SURELY_GOOD_ORI;	// surely proper facing
				else
					return;								// surely opposite facing
			}
			else
				info |= BVH_TRI_SURELY_GOOD_ORI;		// filtering by facing disabled: so consider all facing good

			if ( info == BVH_TRI_SURELY_MATCH ) {
				// get leftmost leaf
				while ( nodes[nodeIdx].HasSons() )
					nodeIdx = nodes[nodeIdx].GetSon( nodeIdx, 0 );
				// add whole node as "surely matches" result
				AddSegmentToList( intervals, nodes[nodeIdx].firstElement, node.numElements, BVH_TRI_SURELY_MATCH, bounds );
				return;
			}

			if ( node.HasSons() && node.numElements > granularity ) {
				// uncertain node, but not leaf -> recurse into subnodes
				Traverse( node.GetSon( nodeIdx, 0 ), bounds );
				Traverse( node.GetSon( nodeIdx, 1 ), bounds );
				return;
			}

			if ( node.HasSons() ) {
				// get leftmost leaf
				while ( nodes[nodeIdx].HasSons() )
					nodeIdx = nodes[nodeIdx].GetSon( nodeIdx, 0 );
			}
			// add leaf node to output as "uncertain"
			AddSegmentToList( intervals, nodes[nodeIdx].firstElement, node.numElements, info | forceUnknown, bounds );
		}
	};

	// launch recursive traversal over BVH tree
	Traverser traverser = { nodes, frustum, filterOri, origin, forceUnknown, granularity, cull, outIntervals };
 	traverser.Traverse( 0, rootBounds );
}


#include "../tests/testing.h"

TEST_CASE("BvhChecks:Sphere") {
	srfTriangles_t *tri = R_AllocStaticTriSurf();

	static const float RADIUS = 1000.0f;
	static const int GRID = 128;
#ifdef  _DEBUG
	static const int TRIES = 100;
#else
	static const int TRIES = 1000;
#endif
	static const float LIGHTSIZE = 300.0f;
	static const int EXCESSIVE_CAP_AVERAGE = 300;

	int v = (GRID + 1) * (GRID * 2);
	int f = GRID * (GRID * 2) * 2;

	tri->numVerts = v;
	tri->numIndexes = f * 3;
	R_AllocStaticTriSurfVerts( tri, tri->numVerts );
	R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );

	//create sphere tessellated evenly by spherical coordinates
	for ( int u = 0; u < 2 * GRID; u++ )
		for ( int v = 0; v <= GRID; v++ ) {
			float phi = idMath::PI * v / GRID;
			float theta = idMath::PI * u / GRID;
			int idx = v * (2 * GRID) + u;
			tri->verts[idx].Clear();
			tri->verts[idx].xyz = idVec3(
				idMath::Sin( phi ) * idMath::Cos( theta ),
				idMath::Sin( phi ) * idMath::Sin( theta ),
				idMath::Cos( phi )
			) * RADIUS;
		}
	for ( int u = 0; u < 2 * GRID; u++ ) {
		int nu = ( u + 1 ) % ( 2 * GRID );
		for ( int v = 0; v < GRID; v++ ) {
			int q00 = (v + 0) * (2 * GRID) + u;
			int q01 = (v + 1) * (2 * GRID) + u;
			int q10 = (v + 0) * (2 * GRID) + nu;
			int q11 = (v + 1) * (2 * GRID) + nu;
			tri->indexes[6 * q00 + 0] = q00;
			tri->indexes[6 * q00 + 1] = q10;
			tri->indexes[6 * q00 + 2] = q01;
			tri->indexes[6 * q00 + 3] = q01;
			tri->indexes[6 * q00 + 4] = q10;
			tri->indexes[6 * q00 + 5] = q11;
		}
	}

	//build BVH
	R_BuildBvhForTri( tri );

	for (int mode = 0; mode < 2; mode++) {
		int totalUnknown = 0;
		int totalRendered = 0;

		idRandom rnd;
		for ( int t = 0; t < TRIES; t++ ) {
			//pick light volume aabox (not too far from sphere)
			idVec3 ctr, size;
			do {
				size.x = rnd.RandomFloat() * LIGHTSIZE;
				size.y = rnd.RandomFloat() * LIGHTSIZE;
				size.z = rnd.RandomFloat() * LIGHTSIZE;
				ctr.x = rnd.CRandomFloat() * (RADIUS + 2.0f * LIGHTSIZE);
				ctr.y = rnd.CRandomFloat() * (RADIUS + 2.0f * LIGHTSIZE);
				ctr.z = rnd.CRandomFloat() * (RADIUS + 2.0f * LIGHTSIZE);
			} while ( idMath::Fabs( ctr.Length() - RADIUS ) > 1.5f * LIGHTSIZE );
			idBounds volume( ctr - size, ctr + size );

			//generate frustum planes
			idPlane frustum[6];
			for ( int i = 0; i < 3; i++ )
				for ( int j = 0; j < 2; j++ ) {
					idVec3 pos = ctr;
					pos[i] += (2*j-1) * size[i];
					idVec3 normal( 0.0f );
					normal[i] = -(2*j-1);
					frustum[2 * i + j].Normal() = normal;
					frustum[2 * i + j].FitThroughPoint( pos );
				}

			//filter sphere mesh against light volume
			idFlexList<bvhTriRange_t, 128> intervals;
			R_CullBvhByFrustumAndOrigin(
				tri->bounds, tri->bvhNodes,
				frustum, (mode == 0 ? 0 : 1), ctr,
				0, 0, intervals
			);

			//count various stats
			int numUnkn = 0;
			int numErrors = 0;
			int numRendered = 0;
			for ( int i = 0; i < f; i++ ) {
				idVec3 pa = tri->verts[tri->indexes[3 * i + 0]].xyz;
				idVec3 pb = tri->verts[tri->indexes[3 * i + 1]].xyz;
				idVec3 pc = tri->verts[tri->indexes[3 * i + 2]].xyz;
				idBounds b( pa );
				b.AddPoint( pb );
				b.AddPoint( pc );

				float ori = ( pb - pa ).Cross( pc - pa ) * ( pa - ctr );
				bool shouldBeRendered = volume.IntersectsBounds( b );
				if ( mode == 1 )
					shouldBeRendered = shouldBeRendered && ori >= 0.0f;

				bool inSure = false;
				bool inUnkn = false;
				for ( int j = 0; j < intervals.Num(); j++ )
					if ( i >= intervals[j].beg && i < intervals[j].end ) {
						//filtered bounds must be conservative
						idBounds filterBounds = intervals[j].GetBox();
						filterBounds.ExpandSelf( 0.1f );
						CHECK( filterBounds.ContainsPoint( b[0] ) );
						CHECK( filterBounds.ContainsPoint( b[1] ) );
						if (intervals[j].info == BVH_TRI_SURELY_MATCH)
							inSure = true;
						else
							inUnkn = true;
						break;
					}

				if ( shouldBeRendered && !(inSure || inUnkn) )
					numErrors++;	//triangle within light volume was filtered away!
				if ( !shouldBeRendered && inSure )
					numErrors++;	//triangle out of light volume said to surely be inside!
				numUnkn += int(inUnkn);
				numRendered += int(shouldBeRendered);
			}

			//filtering must be conservative!
			CHECK( numErrors == 0 );

			totalUnknown += numUnkn;
			totalRendered += numRendered;
		}

		//uncertain triangles (which can be inside/outside) are allowed
		//but average number should be pretty low
		CHECK( totalUnknown <= TRIES * EXCESSIVE_CAP_AVERAGE );
	}

	R_FreeStaticTriSurf( tri );
}
