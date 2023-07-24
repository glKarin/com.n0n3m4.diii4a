// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop


/*
=================
UpdateVertexIndex
=================
*/
ID_INLINE int UpdateVertexIndex( int vertexIndexNum[2], int *vertexRemap, vertIndex_t *vertexCopyIndex, int vertNum ) {
	int s = INTSIGNBITSET( vertexRemap[vertNum] );

	vertexIndexNum[0] = vertexRemap[vertNum];
	vertexRemap[vertNum] = vertexIndexNum[s];
	vertexIndexNum[1] += s;
	vertexCopyIndex[vertexRemap[vertNum]] = vertNum;

	return vertexRemap[vertNum];
}

//do away with _alloca calls and replace with range-checked arrays - useful for finding crashes during map compiles, for one
//#define SD_USE_SURFACE_HEAP

#ifdef SD_USE_SURFACE_HEAP
#define DEREF_INDEX( index ) (*index) 
#else
#define DEREF_INDEX( index ) index
#endif

#ifdef SD_USE_SURFACE_HEAP
ID_INLINE int UpdateVertexIndex_Debug( int vertexIndexNum[2], idList< int >& vertexRemap, idList< int >& vertexCopyIndex, int vertNum ) {
	int s = INTSIGNBITSET( vertexRemap[vertNum] );

	vertexIndexNum[0] = vertexRemap[vertNum];
	vertexRemap[vertNum] = vertexIndexNum[s];
	vertexIndexNum[1] += s;
	vertexCopyIndex[vertexRemap[vertNum]] = vertNum;

	return vertexRemap[vertNum];
}
#endif // SD_USE_SURFACE_HEAP

/*
=================
idSurface::Split
=================
*/
int idSurface::Split( const idPlane &plane, const float epsilon, idSurface **front, idSurface **back, int *frontOnPlaneEdges, int *backOnPlaneEdges ) const {
#ifdef SD_USE_SURFACE_HEAP	
	sdAutoPtr< float, sdArrayCleanupPolicy< float > >			dists;	
	sdAutoPtr< byte, sdArrayCleanupPolicy< byte > >			sides;

	dists.Reset( new float[ verts.Num() ], verts.Num() );
	sides.Reset( new byte[ verts.Num() ], verts.Num() );
#else
	float *			dists = (float *) _alloca( verts.Num() * sizeof( float ) );
	byte *			sides = (byte *) _alloca( verts.Num() * sizeof( byte ) );
#endif // SD_USE_SURFACE_HEAP

#ifdef SD_USE_SURFACE_HEAP
#undef UpdateVertexIndex
#define UpdateVertexIndex UpdateVertexIndex_Debug
#endif // SD_USE_SURFACE_HEAP

	float			f;
	int				i;
	int				counts[3];

	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	// determine side for each vertex
	for ( i = 0; i < verts.Num(); i++ ) {
		dists[i] = f = plane.Distance( verts[i].xyz );
		if ( f > epsilon ) {
			sides[i] = SIDE_FRONT;
		} else if ( f < -epsilon ) {
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	
	*front = *back = NULL;

	// if coplanar, put on the front side if the normals match
	if ( !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {

		f = ( verts[indexes[1]].xyz - verts[indexes[0]].xyz ).Cross( verts[indexes[0]].xyz - verts[indexes[2]].xyz ) * plane.Normal();
		if ( FLOATSIGNBITSET( f ) ) {
			*back = new idSurface( *this );
			return SIDE_BACK;
		} else {
			*front = new idSurface( *this );
			return SIDE_FRONT;
		}
	}
	// if nothing at the front of the clipping plane
	if ( !counts[SIDE_FRONT] ) {
		*back = new idSurface( *this );
		return SIDE_BACK;
	}
	// if nothing at the back of the clipping plane
	if ( !counts[SIDE_BACK] ) {
		*front = new idSurface( *this );
		return SIDE_FRONT;
	}

#ifdef SD_USE_SURFACE_HEAP
	typedef sdAutoPtr< int, sdArrayCleanupPolicy< int > > intArray_t;
	intArray_t		edgeSplitVertex;
	intArray_t		onPlaneEdges[2];
	idList< int >	vertexRemap[2];
	idList< int >	vertexCopyIndex[2];

	idList< int >*	indexPtr[2];
	idList< int >*	index;
#else
	int *			edgeSplitVertex;
	int *			vertexRemap[2];
	int *			onPlaneEdges[2];
	vertIndex_t *	vertexCopyIndex[2];
	vertIndex_t *	indexPtr[2];
	vertIndex_t *	index;

#endif 
	int				numEdgeSplitVertexes;	
	int				vertexIndexNum[2][2];	
	int				indexNum[2];
	
	int				numOnPlaneEdges[2];
	int				maxOnPlaneEdges;
	idSurface *		surface[2];
	idDrawVert		v;	

	// allocate front and back surface
	*front = surface[0] = new idSurface();
	*back = surface[1] = new idSurface();

#ifdef SD_USE_SURFACE_HEAP
	edgeSplitVertex.Reset( new int[ edges.Num() ], edges.Num() );
#else
	edgeSplitVertex = (int *) _alloca( edges.Num() * sizeof( int ) );
#endif // SD_USE_SURFACE_HEAP

	numEdgeSplitVertexes = 0;

	maxOnPlaneEdges = 4 * counts[SIDE_ON];

	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	// split edges
	for ( i = 0; i < edges.Num(); i++ ) {
		int v0 = edges[i].verts[0];
		int v1 = edges[i].verts[1];
		int sidesOr = ( sides[v0] | sides[v1] );

		// if both vertexes are on the same side or one is on the clipping plane
		if ( !( sides[v0] ^ sides[v1] ) || ( sidesOr & SIDE_ON ) ) {
			edgeSplitVertex[i] = -1;
			counts[sidesOr & SIDE_BACK]++;
			counts[SIDE_ON] += ( sidesOr & SIDE_ON ) >> 1;
		} else {
			f = dists[v0] / ( dists[v0] - dists[v1] );
			v.LerpAll( verts[v0], verts[v1], f );
			edgeSplitVertex[i] = numEdgeSplitVertexes++;
			surface[0]->verts.Append( v );
			surface[1]->verts.Append( v );
		}
	}

	// each edge is shared by at most two triangles, as such there can never be more indexes than twice the number of edges
	int surfaceIndexCount[ 2 ];

	// jrad - there are some cases where this isn't enough, sometimes by a factor of 30%
	// bite the bullet and allocate extra memory rather than corrupting the heap later on...
	surfaceIndexCount[ 0 ] = ( ( counts[SIDE_FRONT] + counts[SIDE_ON] ) * 2 ) + ( numEdgeSplitVertexes * 4 );
	surfaceIndexCount[ 1 ] = ( ( counts[SIDE_BACK] + counts[SIDE_ON] ) * 2 ) + ( numEdgeSplitVertexes * 4 );

	surface[0]->indexes.SetNum( surfaceIndexCount[ 0 ] + ( 3 * numEdgeSplitVertexes ) );
	surface[1]->indexes.SetNum( surfaceIndexCount[ 1 ] + ( 3 * numEdgeSplitVertexes ) );

	// allocate indexes to construct the triangle indexes for the front and back surface
#ifdef SD_USE_SURFACE_HEAP
	vertexRemap[0].Fill( verts.Num(), -1 );
	vertexRemap[1].Fill( verts.Num(), -1 );

	vertexCopyIndex[0].SetNum( numEdgeSplitVertexes + verts.Num() );
	vertexCopyIndex[1].SetNum( numEdgeSplitVertexes + verts.Num() );

	indexPtr[0] = &surface[0]->indexes;
	indexPtr[1] = &surface[1]->indexes;

#else
	vertexRemap[0] = (int *) _alloca( verts.Num() * sizeof( int ) );
	memset( vertexRemap[0], -1, verts.Num() * sizeof( int ) );

	vertexRemap[1] = (int *) _alloca( verts.Num() * sizeof( int ) );
	memset( vertexRemap[1], -1, verts.Num() * sizeof( int ) );

	vertexCopyIndex[0] = (vertIndex_t *) _alloca( ( numEdgeSplitVertexes + verts.Num() ) * sizeof( vertIndex_t ) );
	vertexCopyIndex[1] = (vertIndex_t *) _alloca( ( numEdgeSplitVertexes + verts.Num() ) * sizeof( vertIndex_t ) );

	indexPtr[0] = surface[0]->indexes.Begin();
	indexPtr[1] = surface[1]->indexes.Begin();

#endif // SD_USE_SURFACE_HEAP

	vertexIndexNum[0][0] = vertexIndexNum[1][0] = 0;
	vertexIndexNum[0][1] = vertexIndexNum[1][1] = numEdgeSplitVertexes;

	indexNum[0] = 0;
	indexNum[1] = 0;

	maxOnPlaneEdges += 4 * numEdgeSplitVertexes;

	// allocate one more in case no triangles are actually split which may happen for a disconnected surface

#ifdef SD_USE_SURFACE_HEAP
	onPlaneEdges[0].Reset( new int[ maxOnPlaneEdges + 1 ], maxOnPlaneEdges + 1 );
	onPlaneEdges[1].Reset( new int[ maxOnPlaneEdges + 1 ], maxOnPlaneEdges + 1 );
#else
	onPlaneEdges[0] = (int *) _alloca( ( maxOnPlaneEdges + 1 ) * sizeof( int ) );
	onPlaneEdges[1] = (int *) _alloca( ( maxOnPlaneEdges + 1 ) * sizeof( int ) );
#endif // SD_USE_SURFACE_HEAP
	numOnPlaneEdges[0] = numOnPlaneEdges[1] = 0;

#ifdef SD_USE_SURFACE_HEAP
	int resizeCount[2];
	resizeCount[0] = resizeCount[1] = 0;
#endif // SD_USE_SURFACE_HEAP

	// split surface triangles
	for ( i = 0; i < edgeIndexes.Num(); i += 3 ) {
		int e0, e1, e2, v0, v1, v2, s, n;

		e0 = idMath::Abs( edgeIndexes[i+0] );
		e1 = idMath::Abs( edgeIndexes[i+1] );
		e2 = idMath::Abs( edgeIndexes[i+2] );

		v0 = indexes[i+0];
		v1 = indexes[i+1];
		v2 = indexes[i+2];
#if 0
		if( indexNum[ 0 ] >= surface[ 0 ]->indexes.Num() ) {
			surface[ 0 ]->indexes.SetNum( indexNum[ 0 ] + 3 );
			resizeCount[0] += 3;
			//assert( 0 );
		}
		if( indexNum[ 1 ] >= surface[ 1 ]->indexes.Num() ) {
			surface[ 1 ]->indexes.SetNum( indexNum[ 1 ] + 3 );
			resizeCount[1] += 3;
			//assert( 0 );
		}
#ifdef SD_USE_SURFACE_HEAP
#else
		indexPtr[0] = surface[0]->indexes.Begin();
		indexPtr[1] = surface[1]->indexes.Begin();

#endif // SD_USE_SURFACE_HEAP
#endif // 0

		switch( ( INTSIGNBITSET( edgeSplitVertex[e0] ) | ( INTSIGNBITSET( edgeSplitVertex[e1] ) << 1 ) | ( INTSIGNBITSET( edgeSplitVertex[e2] ) << 2 ) ) ^ 7 ) {
			case 0: {	// no edges split
				if ( ( sides[v0] & sides[v1] & sides[v2] ) & SIDE_ON ) {
					// coplanar
					f = ( verts[v1].xyz - verts[v0].xyz ).Cross( verts[v0].xyz - verts[v2].xyz ) * plane.Normal();
					s = FLOATSIGNBITSET( f );
				} else {
					s = ( sides[v0] | sides[v1] | sides[v2] ) & SIDE_BACK;
				}
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]] = n;
				numOnPlaneEdges[s] += ( sides[v0] & sides[v1] ) >> 1;
				onPlaneEdges[s][numOnPlaneEdges[s]] = n+1;
				numOnPlaneEdges[s] += ( sides[v1] & sides[v2] ) >> 1;
				onPlaneEdges[s][numOnPlaneEdges[s]] = n+2;
				numOnPlaneEdges[s] += ( sides[v2] & sides[v0] ) >> 1;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				indexNum[s] = n;				

				break;
			}
			case 1: {	// first edge split
				s = sides[v0] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = edgeSplitVertex[e0];
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				
				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];				

				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;				
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				DEREF_INDEX( index )[n++] = edgeSplitVertex[e0];
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				
				indexNum[s] = n;			

				break;
			}
			case 2: {	// second edge split
				s = sides[v1] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = edgeSplitVertex[e1];
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				
				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				DEREF_INDEX( index )[n++] = edgeSplitVertex[e1];
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				
				indexNum[s] = n;

				break;
			}
			case 3: {	// first and second edge split
				s = sides[v1] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = edgeSplitVertex[e1];				
				DEREF_INDEX( index )[n++] = edgeSplitVertex[e0];				
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				

				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = edgeSplitVertex[e0];
				DEREF_INDEX( index )[n++] = edgeSplitVertex[e1];				
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );

				DEREF_INDEX( index )[n++] = edgeSplitVertex[e1];				
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );				
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );				
				
				indexNum[s] = n;

				break;
			}
			case 4: {	// third edge split
				s = sides[v2] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = edgeSplitVertex[e2];				
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );				
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				
				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				DEREF_INDEX( index )[n++] = edgeSplitVertex[e2];				
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				
				indexNum[s] = n;

				break;
			}
			case 5: {	// first and third edge split
				s = sides[v0] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = edgeSplitVertex[e0];
				DEREF_INDEX( index )[n++] = edgeSplitVertex[e2];			
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				
				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = edgeSplitVertex[e2];			
				DEREF_INDEX( index )[n++] = edgeSplitVertex[e0];				
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );				
				DEREF_INDEX( index )[n++] = edgeSplitVertex[e2];
				indexNum[s] = n;

				break;
			}
			case 6: {	// second and third edge split
				s = sides[v2] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = edgeSplitVertex[e2];				
				DEREF_INDEX( index )[n++] = edgeSplitVertex[e1];				
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				
				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];

				DEREF_INDEX( index )[n++] = edgeSplitVertex[e1];				
				DEREF_INDEX( index )[n++] = edgeSplitVertex[e2];				
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				DEREF_INDEX( index )[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );				
				DEREF_INDEX( index )[n++] = edgeSplitVertex[e2];
				
				indexNum[s] = n;

				break;
			}
		}
	}

	surface[0]->indexes.SetNum( indexNum[0], false );
	surface[1]->indexes.SetNum( indexNum[1], false );

	// copy vertexes
	surface[0]->verts.SetNum( vertexIndexNum[0][1], false );
#ifdef SD_USE_SURFACE_HEAP
	index = &vertexCopyIndex[0];
#else
	index = vertexCopyIndex[0];
#endif // SD_USE_SURFACE_HEAP
	
	for ( i = numEdgeSplitVertexes; i < surface[0]->verts.Num(); i++ ) {
#ifdef SD_USE_SURFACE_HEAP
		if( DEREF_INDEX( index )[i] >= verts.Num() || DEREF_INDEX( index )[i] < 0 ) {
			assert( 0 );
			idLib::Error( "Invalid index" );
		}
#endif // SD_USE_SURFACE_HEAP
		surface[0]->verts[i] = verts[DEREF_INDEX( index )[i]];
	}


	surface[1]->verts.SetNum( vertexIndexNum[1][1], false );
#ifdef SD_USE_SURFACE_HEAP
	index = &vertexCopyIndex[1];
#else
	index = vertexCopyIndex[1];
#endif // SD_USE_SURFACE_HEAP
	for ( i = numEdgeSplitVertexes; i < surface[1]->verts.Num(); i++ ) {
#ifdef SD_USE_SURFACE_HEAP
		if( DEREF_INDEX( index )[i] >= verts.Num() || DEREF_INDEX( index )[i] < 0 ) {
			assert( 0 );
			idLib::Error( "Invalid index" );
		}
#endif // SD_USE_SURFACE_HEAP
		surface[1]->verts[i] = verts[DEREF_INDEX( index )[i]];
	}

#ifdef _DEBUG
	for ( i = 0; i < surface[0]->indexes.Num(); i++ ) {
		if( surface[0]->indexes[ i ] >= surface[0]->verts.Num() || surface[0]->indexes[ i ] < 0 ) {
			assert( 0 );
		}
	}
	for ( i = 0; i < surface[1]->indexes.Num(); i++ ) {
		if( surface[1]->indexes[ i ] >= surface[1]->verts.Num() || surface[1]->indexes[ i ] < 0 ) {
			assert( 0 );
		}
	}
#endif //_DEBUG


	// generate edge indexes
	surface[0]->GenerateEdgeIndexes();
	surface[1]->GenerateEdgeIndexes();

	if ( frontOnPlaneEdges ) {
#ifdef SD_USE_SURFACE_HEAP
		memcpy( frontOnPlaneEdges, onPlaneEdges[0].Get(), numOnPlaneEdges[0] * sizeof( int ) );
#else
		memcpy( frontOnPlaneEdges, onPlaneEdges[0], numOnPlaneEdges[0] * sizeof( int ) );
#endif // SD_USE_SURFACE_HEAP		
		frontOnPlaneEdges[numOnPlaneEdges[0]] = -1;
	}

	if ( backOnPlaneEdges ) {
#ifdef SD_USE_SURFACE_HEAP
		memcpy( backOnPlaneEdges, onPlaneEdges[1].Get(), numOnPlaneEdges[1] * sizeof( int ) );
#else
		memcpy( backOnPlaneEdges, onPlaneEdges[1], numOnPlaneEdges[1] * sizeof( int ) );
#endif // SD_USE_SURFACE_HEAP
		
		backOnPlaneEdges[numOnPlaneEdges[1]] = -1;
	}

#ifdef SD_USE_SURFACE_HEAP
	if( resizeCount[ 0 ] > 0 ) {
		idLib::Warning( "idSurface::Split: had to resize index list 0 by %i elements", resizeCount[0] );
		assert( 0 );
	}
	if( resizeCount[ 1 ] > 0 ) {
		idLib::Warning( "idSurface::Split: had to resize index list 1 by %i elements", resizeCount[1] );
		assert( 0 );
	}
#endif // SD_USE_SURFACE_HEAP

	return SIDE_CROSS;
}

#undef UpdateVertexIndex
#undef SD_USE_SURFACE_HEAP

/*
=================
idSurface::ClipInPlace
=================
*/
bool idSurface::ClipInPlace( const idPlane &plane, const float epsilon, const bool keepOn ) {
	if( verts.Num() == 0 ) {
		return false;
	}

	float *			dists;
	float			f;
	byte *			sides;
	int				counts[3];
	int				i;
	int *			edgeSplitVertex;
	int *			vertexRemap;
	int				vertexIndexNum[2];
	vertIndex_t *	vertexCopyIndex;
	vertIndex_t *	indexPtr;
	int				indexNum;
	int				numEdgeSplitVertexes;
	idDrawVert		v;
	idList<idDrawVert> newVerts;
	idList<vertIndex_t>	newIndexes;

	dists = (float *) _alloca( verts.Num() * sizeof( float ) );
	sides = (byte *) _alloca( verts.Num() * sizeof( byte ) );

	counts[0] = counts[1] = counts[2] = 0;

	// determine side for each vertex
	for ( i = 0; i < verts.Num(); i++ ) {
		dists[i] = f = plane.Distance( verts[i].xyz );
		if ( f > epsilon ) {
			sides[i] = SIDE_FRONT;
		} else if ( f < -epsilon ) {
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	
	// if coplanar, put on the front side if the normals match
	if ( !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {

		f = ( verts[indexes[1]].xyz - verts[indexes[0]].xyz ).Cross( verts[indexes[0]].xyz - verts[indexes[2]].xyz ) * plane.Normal();
		if ( FLOATSIGNBITSET( f ) ) {
			Clear();
			return false;
		} else {
			return true;
		}
	}
	// if nothing at the front of the clipping plane
	if ( !counts[SIDE_FRONT] ) {
		Clear();
		return false;
	}
	// if nothing at the back of the clipping plane
	if ( !counts[SIDE_BACK] ) {
		return true;
	}

	edgeSplitVertex = (int *) _alloca( edges.Num() * sizeof( int ) );
	numEdgeSplitVertexes = 0;

	counts[SIDE_FRONT] = counts[SIDE_BACK] = 0;

	// split edges
	for ( i = 0; i < edges.Num(); i++ ) {
		int v0 = edges[i].verts[0];
		int v1 = edges[i].verts[1];

		// if both vertexes are on the same side or one is on the clipping plane
		if ( !( sides[v0] ^ sides[v1] ) || ( ( sides[v0] | sides[v1] ) & SIDE_ON ) ) {
			edgeSplitVertex[i] = -1;
			counts[(sides[v0]|sides[v1]) & SIDE_BACK]++;
		} else {
			f = dists[v0] / ( dists[v0] - dists[v1] );
			v.LerpAll( verts[v0], verts[v1], f );
			edgeSplitVertex[i] = numEdgeSplitVertexes++;
			newVerts.Append( v );
		}
	}

	// each edge is shared by at most two triangles, as such there can never be
	// more indexes than twice the number of edges
	newIndexes.Resize( ( counts[SIDE_FRONT] << 1 ) + ( numEdgeSplitVertexes << 2 ) );

	// allocate indexes to construct the triangle indexes for the front and back surface
	vertexRemap = (int *) _alloca( verts.Num() * sizeof( int ) );
	memset( vertexRemap, -1, verts.Num() * sizeof( int ) );

	vertexCopyIndex = (vertIndex_t *) _alloca( ( numEdgeSplitVertexes + verts.Num() ) * sizeof( vertIndex_t ) );

	vertexIndexNum[0] = 0;
	vertexIndexNum[1] = numEdgeSplitVertexes;

	indexPtr = newIndexes.Begin();
	indexNum = newIndexes.Num();

	// split surface triangles
	for ( i = 0; i < edgeIndexes.Num(); i += 3 ) {
		int e0, e1, e2, v0, v1, v2;

		e0 = idMath::Abs( edgeIndexes[i+0] );
		e1 = idMath::Abs( edgeIndexes[i+1] );
		e2 = idMath::Abs( edgeIndexes[i+2] );

		v0 = indexes[i+0];
		v1 = indexes[i+1];
		v2 = indexes[i+2];

		switch( ( INTSIGNBITSET( edgeSplitVertex[e0] ) | ( INTSIGNBITSET( edgeSplitVertex[e1] ) << 1 ) | ( INTSIGNBITSET( edgeSplitVertex[e2] ) << 2 ) ) ^ 7 ) {
			case 0: {	// no edges split
				if ( ( sides[v0] | sides[v1] | sides[v2] ) & SIDE_BACK ) {
					break;
				}
				if ( ( sides[v0] & sides[v1] & sides[v2] ) & SIDE_ON ) {
					// coplanar
					if ( !keepOn ) {
						break;
					}
					f = ( verts[v1].xyz - verts[v0].xyz ).Cross( verts[v0].xyz - verts[v2].xyz ) * plane.Normal();
					if ( FLOATSIGNBITSET( f ) ) {
						break;
					}
				}
				indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
				indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
				indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
				break;
			}
			case 1: {	// first edge split
				if ( !( sides[v0] & SIDE_BACK ) ) {
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
					indexPtr[indexNum++] = edgeSplitVertex[e0];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
				} else {
					indexPtr[indexNum++] = edgeSplitVertex[e0];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
				}
				break;
			}
			case 2: {	// second edge split
				if ( !( sides[v1] & SIDE_BACK ) ) {
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
				} else {
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
				}
				break;
			}
			case 3: {	// first and second edge split
				if ( !( sides[v1] & SIDE_BACK ) ) {
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = edgeSplitVertex[e0];
				} else {
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
					indexPtr[indexNum++] = edgeSplitVertex[e0];
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
				}
				break;
			}
			case 4: {	// third edge split
				if ( !( sides[v2] & SIDE_BACK ) ) {
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
					indexPtr[indexNum++] = edgeSplitVertex[e2];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
				} else {
					indexPtr[indexNum++] = edgeSplitVertex[e2];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
				}
				break;
			}
			case 5: {	// first and third edge split
				if ( !( sides[v0] & SIDE_BACK ) ) {
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
					indexPtr[indexNum++] = edgeSplitVertex[e0];
					indexPtr[indexNum++] = edgeSplitVertex[e2];
				} else {
					indexPtr[indexNum++] = edgeSplitVertex[e0];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = edgeSplitVertex[e2];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
					indexPtr[indexNum++] = edgeSplitVertex[e2];
				}
				break;
			}
			case 6: {	// second and third edge split
				if ( !( sides[v2] & SIDE_BACK ) ) {
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
					indexPtr[indexNum++] = edgeSplitVertex[e2];
					indexPtr[indexNum++] = edgeSplitVertex[e1];
				} else {
					indexPtr[indexNum++] = edgeSplitVertex[e2];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = edgeSplitVertex[e2];
				}
				break;
			}
		}
	}

	newIndexes.SetNum( indexNum, false );

	// copy vertexes
	newVerts.SetNum( vertexIndexNum[1], false );
	for ( i = numEdgeSplitVertexes; i < newVerts.Num(); i++ ) {
		newVerts[i] = verts[vertexCopyIndex[i]];
	}

	// copy back to this surface
	indexes = newIndexes;
	verts = newVerts;

	GenerateEdgeIndexes();

	return true;
}

/*
=============
idSurface::IsConnected
=============
*/
bool idSurface::IsConnected( void ) const {
	int i, j, numIslands, numTris;
	int queueStart, queueEnd;
	int *queue, *islandNum;
	int curTri, nextTri, edgeNum;
	const int *index;

	numIslands = 0;
	numTris = indexes.Num() / 3;
	islandNum = (int *) _alloca16( numTris * sizeof( int ) );
	memset( islandNum, -1, numTris * sizeof( int ) );
	queue = (int *) _alloca16( numTris * sizeof( int ) );

	for ( i = 0; i < numTris; i++ ) {

		if ( islandNum[i] != -1 ) {
			continue;
		}

        queueStart = 0;
		queueEnd = 1;
		queue[0] = i;
		islandNum[i] = numIslands;

		for ( curTri = queue[queueStart]; queueStart < queueEnd; curTri = queue[++queueStart] ) {

			index = &edgeIndexes[curTri * 3];

			for ( j = 0; j < 3; j++ ) {

				edgeNum = index[j];
				nextTri = edges[idMath::Abs(edgeNum)].tris[INTSIGNBITNOTSET(edgeNum)];

				if ( nextTri == -1 ) {
					continue;
				}

				nextTri /= 3;

				if ( islandNum[nextTri] != -1 ) {
					continue;
				}

				queue[queueEnd++] = nextTri;
				islandNum[nextTri] = numIslands;
			}
		}
		numIslands++;
	}

	return ( numIslands == 1 );
}

/*
=================
idSurface::IsClosed
=================
*/
bool idSurface::IsClosed( void ) const {
	for ( int i = 0; i < edges.Num(); i++ ) {
		if ( edges[i].tris[0] < 0 || edges[i].tris[1] < 0 ) {
			return false;
		}
	}
	return true;
}

/*
=============
idSurface::IsPolytope
=============
*/
bool idSurface::IsPolytope( const float epsilon ) const {
	int i, j;
	idPlane plane;

	if ( !IsClosed() ) {
		return false;
	}

	for ( i = 0; i < indexes.Num(); i += 3 ) {
		plane.FromPoints( verts[indexes[i+0]].xyz, verts[indexes[i+1]].xyz, verts[indexes[i+2]].xyz );

		for ( j = 0; j < verts.Num(); j++ ) {
			if ( plane.Side( verts[j].xyz, epsilon ) == SIDE_FRONT ) {
				return false;
			}
		}
	}
	return true;
}

/*
=============
idSurface::PlaneDistance
=============
*/
float idSurface::PlaneDistance( const idPlane &plane ) const {
	int		i;
	float	d, min, max;

	min = idMath::INFINITY;
	max = -min;
	for ( i = 0; i < verts.Num(); i++ ) {
		d = plane.Distance( verts[i].xyz );
		if ( d < min ) {
			min = d;
			if ( FLOATSIGNBITSET( min ) & FLOATSIGNBITNOTSET( max ) ) {
				return 0.0f;
			}
		}
		if ( d > max ) {
			max = d;
			if ( FLOATSIGNBITSET( min ) & FLOATSIGNBITNOTSET( max ) ) {
				return 0.0f;
			}
		}
	}
	if ( FLOATSIGNBITNOTSET( min ) ) {
		return min;
	}
	if ( FLOATSIGNBITSET( max ) ) {
		return max;
	}
	return 0.0f;
}

/*
=============
idSurface::PlaneSide
=============
*/
int idSurface::PlaneSide( const idPlane &plane, const float epsilon ) const {
	bool	front, back;
	int		i;
	float	d;

	front = false;
	back = false;
	for ( i = 0; i < verts.Num(); i++ ) {
		d = plane.Distance( verts[i].xyz );
		if ( d < -epsilon ) {
			if ( front ) {
				return SIDE_CROSS;
			}
			back = true;
			continue;
		}
		else if ( d > epsilon ) {
			if ( back ) {
				return SIDE_CROSS;
			}
			front = true;
			continue;
		}
	}

	if ( back ) {
		return SIDE_BACK;
	}
	if ( front ) {
		return SIDE_FRONT;
	}
	return SIDE_ON;
}

/*
=================
idSurface::LineIntersection
=================
*/
bool idSurface::LineIntersection( const idVec3 &start, const idVec3 &end, bool backFaceCull ) const {
	float scale;

	RayIntersection( start, end - start, scale, false );
	return ( scale >= 0.0f && scale <= 1.0f );
}

/*
=================
idSurface::RayIntersection
=================
*/
bool idSurface::RayIntersection( const idVec3 &start, const idVec3 &dir, float &scale, bool backFaceCull ) const {
	int i, i0, i1, i2, s0, s1, s2;
	float d, s;
	byte *sidedness;
	idPluecker rayPl, pl;
	idPlane plane;

	sidedness = (byte *)_alloca( edges.Num() * sizeof(byte) );
	scale = idMath::INFINITY;

	rayPl.FromRay( start, dir );

	// ray sidedness for edges
	for ( i = 0; i < edges.Num(); i++ ) {
		pl.FromLine( verts[ edges[i].verts[1] ].xyz, verts[ edges[i].verts[0] ].xyz );
		d = pl.PermutedInnerProduct( rayPl );
		sidedness[ i ] = FLOATSIGNBITSET( d );
	}

	// test triangles
	for ( i = 0; i < edgeIndexes.Num(); i += 3 ) {
		i0 = edgeIndexes[i+0];
		i1 = edgeIndexes[i+1];
		i2 = edgeIndexes[i+2];
		s0 = sidedness[idMath::Abs(i0)] ^ INTSIGNBITSET( i0 );
		s1 = sidedness[idMath::Abs(i1)] ^ INTSIGNBITSET( i1 );
		s2 = sidedness[idMath::Abs(i2)] ^ INTSIGNBITSET( i2 );

		if ( s0 & s1 & s2 ) {
			plane.FromPoints( verts[indexes[i+0]].xyz, verts[indexes[i+1]].xyz, verts[indexes[i+2]].xyz );
			plane.RayIntersection( start, dir, s );
			if ( idMath::Fabs( s ) < idMath::Fabs( scale ) ) {
				scale = s;
			}
		} else if ( !backFaceCull && !(s0 | s1 | s2) ) {
			plane.FromPoints( verts[indexes[i+0]].xyz, verts[indexes[i+1]].xyz, verts[indexes[i+2]].xyz );
			plane.RayIntersection( start, dir, s );
			if ( idMath::Fabs( s ) < idMath::Fabs( scale ) ) {
				scale = s;
			}
		}
	}

	if ( idMath::Fabs( scale ) < idMath::INFINITY ) {
		return true;
	}
	return false;
}

/*
=================
idSurface::GenerateEdgeIndexes

  Assumes each edge is shared by at most two triangles.
=================
*/
void idSurface::GenerateEdgeIndexes( void ) {
	if( verts.Num() == 0 || indexes.Num() == 0 ) {
		assert( 0 );
		return;
	}
	int i, j, i0, i1, i2, s, v0, v1, edgeNum;
	vertIndex_t* index;
	int *vertexEdges, *edgeChain;
	surfaceEdge_t e[3];

	// protect against stack overflow
	bool usingStack = true;
	if ( ( verts.Num() * sizeof( int ) ) + ( indexes.Num() * sizeof( int ) ) > 1048576 /*4194304*/ ) {
		usingStack = false;
		vertexEdges = (int *) Mem_AllocAligned( verts.Num() * sizeof( int ), ALIGN_16 );
		memset( vertexEdges, -1, verts.Num() * sizeof( int ) );
		edgeChain = (int *) Mem_AllocAligned( indexes.Num() * sizeof( int ), ALIGN_16 );
	} else {
		vertexEdges = (int *) _alloca16( verts.Num() * sizeof( int ) );
		memset( vertexEdges, -1, verts.Num() * sizeof( int ) );
		edgeChain = (int *) _alloca16( indexes.Num() * sizeof( int ) );
	}

	edgeIndexes.SetNum( indexes.Num(), true );

	edges.Clear();
	// jrad - deal with large models more nicely
	edges.SetGranularity( indexes.Num() / 3 );

	// the first edge is a dummy
	e[0].verts[0] = e[0].verts[1] = e[0].tris[0] = e[0].tris[1] = 0;
	edges.Append( e[0] );

	for ( i = 0; i < indexes.Num(); i += 3 ) {
		index = indexes.Begin() + i;
		// vertex numbers
		i0 = index[0];
		i1 = index[1];
		i2 = index[2];
		// setup edges each with smallest vertex number first
		s = INTSIGNBITSET(i1 - i0);
		e[0].verts[0] = index[s];
		e[0].verts[1] = index[s^1];
		s = INTSIGNBITSET(i2 - i1) + 1;
		e[1].verts[0] = index[s];
		e[1].verts[1] = index[s^3];
		s = INTSIGNBITSET(i2 - i0) << 1;
		e[2].verts[0] = index[s];
		e[2].verts[1] = index[s^2];
		// get edges
		for ( j = 0; j < 3; j++ ) {
			v0 = e[j].verts[0];
			v1 = e[j].verts[1];
			for ( edgeNum = vertexEdges[v0]; edgeNum >= 0; edgeNum = edgeChain[edgeNum] ) {
				if ( edges[edgeNum].verts[1] == v1 ) {
					break;
				}
			}
			// if the edge does not yet exist
			if ( edgeNum < 0 ) {
				e[j].tris[0] = e[j].tris[1] = -1;
				edgeNum = edges.Append( e[j] );
				edgeChain[edgeNum] = vertexEdges[v0];
				vertexEdges[v0] = edgeNum;
			}
			// update edge index and edge tri references
			if ( index[j] == v0 ) {
				//assert( edges[edgeNum].tris[0] == -1 ); // edge may not be shared by more than two triangles
				edges[edgeNum].tris[0] = i;
				edgeIndexes[i+j] = edgeNum;
			} else {
				//assert( edges[edgeNum].tris[1] == -1 ); // edge may not be shared by more than two triangles
				edges[edgeNum].tris[1] = i;
				edgeIndexes[i+j] = -edgeNum;
			}
		}
	}
	
	edges.Resize( edges.Num(), 16 );

	if ( !usingStack ) {
		Mem_FreeAligned( vertexEdges );
		Mem_FreeAligned( edgeChain );
	}
}

/*
=================
idSurface::FindEdge
=================
*/
int idSurface::FindEdge( int v1, int v2 ) const {
	int i, firstVert, secondVert;

	if ( v1 < v2 ) {
		firstVert = v1;
		secondVert = v2;
	} else {
		firstVert = v2;
		secondVert = v1;
	}
	for ( i = 1; i < edges.Num(); i++ ) {
		if ( edges[i].verts[0] == firstVert ) {
			if ( edges[i].verts[1] == secondVert ) {
				break;
			}
		}
	}
	if ( i < edges.Num() ) {
		return v1 < v2 ? i : -i;
	}
	return 0;
}
