// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

//===============================================================
//
//	sdTraceSurface
//
//===============================================================

/*
====================
sdTraceSurface::sdTraceSurface
====================
*/
sdTraceSurface::sdTraceSurface( const idDrawVert *verts, const int numVerts, const vertIndex_t *indexes, const int numIndexes, const int hashBinsPerAxis ) :
	verts( verts ),
	numVerts( numVerts ),
	indexes( indexes ),
	numIndexes( numIndexes ) {

	binsPerAxis = idMath::FloorPowerOfTwo( hashBinsPerAxis );
	binLinks = NULL;

	// find the bounding volume for the mesh
	SIMDProcessor->MinMax( _bounds.GetMins(), _bounds.GetMaxs(), verts, numVerts );
	_bounds.ExpandSelf( 64.f );

	// create face planes
	facePlanes = (idPlane*)Mem_AllocAligned( (numIndexes / 3) * sizeof(idPlane), ALIGN_16 );
	SIMDProcessor->DeriveTriPlanes( facePlanes, verts, numVerts, indexes, numIndexes );

	CreateHash();
}

/*
====================
sdTraceSurface::~sdTraceSurface
====================
*/
sdTraceSurface::~sdTraceSurface() {
	int i, j;

	if ( binLinks ) {
		for ( i = 0; i < binsPerAxis; i++ ) {
			for ( j = 0; j < binsPerAxis; j++ ) {
				Mem_Free( binLinks[i][j] );
			}
			Mem_Free( binLinks[i] );
		}
		Mem_Free( binLinks );
	}

	for ( i = 0; i < numLinkBlocks; i++ ) {
		Mem_Free( linkBlocks[i] );
	}

	Mem_FreeAligned( facePlanes );
}

/*
====================
sdTraceSurface::RayIntersection
====================
*/
bool sdTraceSurface::RayIntersection( const idVec3& start, const idVec3& end, idDrawVert& dv ) const {
	idVec3			p;
	int				linkNum;
	unsigned int	i;
	unsigned int	maxTraceBins;
	int				hashBin[3];
	int				separatorAxis;
	float			faceDist, traceDist;
	float			separatorDist;
	idVec3			normal, invNormal;

	// clear the result in case nothing is hit
	dv.Clear();

	if ( numLinkBlocks == 0 ) {
		return false;
	}

	// get trace normal and normalize
	normal = end - start;
	faceDist = traceDist = normal.Normalize();

	// inverse normal
	invNormal[0] = ( normal[0] != 0.0f ) ? 1.0f / normal[0] : 1.0f;
	invNormal[1] = ( normal[1] != 0.0f ) ? 1.0f / normal[1] : 1.0f;
	invNormal[2] = ( normal[2] != 0.0f ) ? 1.0f / normal[2] : 1.0f;

	// maximum number of hash bins the trace can go through
	maxTraceBins = 1 + idMath::Ftoi( traceDist * (
							fabs( normal[0] ) * invBinSize[0] +
								fabs( normal[1] ) * invBinSize[1] +
									fabs( normal[2] ) * invBinSize[2] ) );

	// get the first hash bin for the trace
	p = start - _bounds[0];
	hashBin[0] = idMath::Ftoi( idMath::Floor( p[0] * invBinSize[0] ) );
	hashBin[1] = idMath::Ftoi( idMath::Floor( p[1] * invBinSize[1] ) );
	hashBin[2] = idMath::Ftoi( idMath::Floor( p[2] * invBinSize[2] ) );

	separatorAxis = 0;
	separatorDist = 0.0f;

	bool insideHash = false;

	for ( i = 0; i < maxTraceBins; i++, GetNextHashBin( start, normal, hashBin, separatorAxis, separatorDist ) ) {

		// if this bin is outside the valid hash space
		if ( ( hashBin[0] | hashBin[1] | hashBin[2] ) & ~( binsPerAxis - 1 ) ) {
			if ( insideHash ) {
				break;		// left valid hash space
			}
			continue;		// not yet in valid hash space
		}

		insideHash = true;

		// if a triangle was hit before the plane that separates this hash bin from the previous hash bin
		if ( faceDist < traceDist && faceDist < separatorDist * invNormal[separatorAxis] ) {
			break;
		}

		const triLink_t *link;

		// test all the triangles in this hash bin
		for ( linkNum = binLinks[hashBin[0]][hashBin[1]][hashBin[2]]; linkNum != -1; linkNum = link->nextLink ) {
			link = &linkBlocks[ linkNum / MAX_LINKS_PER_BLOCK ][ linkNum % MAX_LINKS_PER_BLOCK ];

			faceDist = TraceToMeshFace( link->faceNum, start, normal, faceDist, traceDist, dv );
		}
	}

	// FIXME: SD_USE_DRAWVERT_SIZE_32
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	idVec3 n = dv.GetNormal();
	n.Normalize();
	dv.SetNormal( n );
#else
	dv._normal.Normalize();
#endif

	return ( faceDist < traceDist );
}

/*
====================
sdTraceSurface::CreateHash
====================
*/
void sdTraceSurface::CreateHash() {
	int			i, j, k, l;
	idBounds	triBounds;
	int			iBounds[2][3];
	int			maxLinks, numLinks;

	numLinkBlocks = 0;

	// divide each axis as needed
	for ( i = 0; i < 3; i++ ) {
		binSize[i] = ( _bounds[1][i] - _bounds[0][i] ) / binsPerAxis;
		if ( binSize[i] <= 0.0f ) {
			common->Warning( "CreateTriHash: bad bounds: (%f %f %f) to (%f %f %f)",
										_bounds[0][0], _bounds[0][1], _bounds[0][2],
											_bounds[1][0], _bounds[1][1], _bounds[1][2] );
			return;
		}
		invBinSize[i] = 1.0f / binSize[i];
	}

	// a -1 link number terminated the link chain
	binLinks = (int ***) Mem_Alloc( binsPerAxis * sizeof( int ** ) );
	for ( i = 0; i < binsPerAxis; i++ ) {
		binLinks[i] = (int **) Mem_Alloc( binsPerAxis * sizeof( int * ) );
		for ( j = 0; j < binsPerAxis; j++ ) {
			binLinks[i][j] = (int *) Mem_Alloc( binsPerAxis * sizeof( int ) );
			memset( binLinks[i][j], -1, binsPerAxis * sizeof( int ) );
		}
	}

	numLinks = 0;

	linkBlocks[numLinkBlocks] = (triLink_t *)Mem_Alloc( MAX_LINKS_PER_BLOCK * sizeof( triLink_t ) );
	numLinkBlocks++;
	maxLinks = numLinkBlocks * MAX_LINKS_PER_BLOCK;

	// for each triangle, place a triLink in each bin that might reference it
	for ( i = 0; i < numIndexes; i += 3 ) {
		// determine which hash bins the triangle will need to be in
		triBounds.Clear();
		for ( j = 0; j < 3; j++ ) {
			triBounds.AddPoint( verts[ indexes[i+j] ].xyz );
		}
		for ( j = 0; j < 3; j++ ) {
			iBounds[0][j] = idMath::Ftoi( ( triBounds[0][j] - _bounds[0][j] - 0.001f ) * invBinSize[j] );
			if ( iBounds[0][j] < 0 ) {
				iBounds[0][j] = 0;
			} else if ( iBounds[0][j] >= binsPerAxis ) {
				iBounds[0][j] = binsPerAxis - 1;
			}

			iBounds[1][j] = idMath::Ftoi( ( triBounds[1][j] - _bounds[0][j] + 0.001f ) * invBinSize[j] );
			if ( iBounds[1][j] < 0 ) {
				iBounds[1][j] = 0;
			} else if ( iBounds[1][j] >= binsPerAxis ) {
				iBounds[1][j] = binsPerAxis - 1;
			}
		}

		// add the links
		for ( j = iBounds[0][0]; j <= iBounds[1][0]; j++ ) {
			for ( k = iBounds[0][1]; k <= iBounds[1][1]; k++ ) {
				for ( l = iBounds[0][2]; l <= iBounds[1][2]; l++ ) {
					if ( numLinks == maxLinks ) {
						linkBlocks[numLinkBlocks] = (triLink_t *)Mem_Alloc( MAX_LINKS_PER_BLOCK * sizeof( triLink_t ) );
						numLinkBlocks++;
						maxLinks = numLinkBlocks * MAX_LINKS_PER_BLOCK;
					}

					triLink_t	*link = &linkBlocks[ numLinks / MAX_LINKS_PER_BLOCK ][ numLinks % MAX_LINKS_PER_BLOCK ];
					link->faceNum = i / 3;
					link->nextLink = binLinks[j][k][l];
					binLinks[j][k][l] = numLinks;
					numLinks++;
				}
			}
		}
	}
}

/*
====================
sdTraceSurface::GetNextHashBin
====================
*/
ID_INLINE void sdTraceSurface::GetNextHashBin( const idVec3 &point, const idVec3 &normal, int hashBin[3], int &separatorAxis, float &separatorDist ) const {
	int		n[3];
	int		b[3];
	int		axis;
	float	d[3];
	idVec3	dir;

	// get the normal sign bits
	n[0] = FLOATSIGNBITSET( normal.x );
	n[1] = FLOATSIGNBITSET( normal.y );
	n[2] = FLOATSIGNBITSET( normal.z );

	// get the direction from the ray start point to the hash bin corner the normal is pointing at
	dir.x = _bounds[0][0] + ( hashBin[0] + ( n[0] ^ 1 ) ) * binSize[0] - point[0];
	dir.y = _bounds[0][1] + ( hashBin[1] + ( n[1] ^ 1 ) ) * binSize[1] - point[1];
	dir.z = _bounds[0][2] + ( hashBin[2] + ( n[2] ^ 1 ) ) * binSize[2] - point[2];

	// determine at which side the ray passes each edge coming from the has bin corner
	d[0] = dir.y * normal.x - dir.x * normal.y;		// x-y plane: edge parallel to z-axis
	d[1] = dir.z * normal.x - dir.x * normal.z;		// x-z plane: edge parallel to y-axis
	d[2] = dir.z * normal.y - dir.y * normal.z;		// y-z plane: edge parallel to x-axis

	b[0] = FLOATSIGNBITSET( d[0] ) ^ n[0] ^ n[1];
	b[1] = FLOATSIGNBITSET( d[1] ) ^ n[0] ^ n[2];
	b[2] = FLOATSIGNBITSET( d[2] ) ^ n[1] ^ n[2];

	// determine the hash bin boundary plane the ray goes through based on which side the ray passes each of the edges
	axis = ( b[0] & ( b[2] ^ 1 ) ) | ( ( b[1] & b[2] ) << 1 );	// 0 = x, 1 = y, 2 = z

	// increment/decrement the axis which separates this hash bin from the next
	hashBin[axis] += 1 - ( n[axis] << 1 );

	// save the separator axis and separator plane distance relative to ray start point
	separatorAxis = axis;
	separatorDist = dir[axis];
}

/*
====================
sdTraceSurface::TraceToMeshFace
====================
*/
const float TRIANGLE_NORMAL_EPSILON	= 0.0001f;
const float TRIANGLE_EDGE_EPSILON	= 0.001f;

ID_INLINE float sdTraceSurface::TraceToMeshFace( const int faceNum, const idVec3& point, const idVec3 &normal, const float faceDist, const float traceDist, idDrawVert& dv ) const {
	int				i;
	float			dist;
	const idPlane*	plane;
	idVec3			edge;
	float			d, t;
	idVec3			dir[3];
	float			baseArea;
	float			bary[3];
	idVec3			testVert;

	plane = facePlanes + faceNum;

	// only test against planes facing the opposite direction as our trace normal
	d = -( plane->Normal() * normal );
	if ( d <= TRIANGLE_NORMAL_EPSILON ) {
		return faceDist;
	}

	// get the distance to the plane
	dist = plane->Distance( point );

	if ( dist >= traceDist * d ) {
		return faceDist;
	}

	const vertIndex_t* index = indexes + faceNum * 3;
	const idDrawVert &v0 = verts[ index[ 0 ] ];
	const idDrawVert &v1 = verts[ index[ 1 ] ];
	const idDrawVert &v2 = verts[ index[ 2 ] ];

	// if normal is inside all edge planes, this face is hit
	dir[0] = v0.xyz - point;
	dir[1] = v1.xyz - point;
	edge = dir[0].Cross( dir[1] );
	t = normal * edge;
	if ( t < -TRIANGLE_EDGE_EPSILON ) {
		return faceDist;
	}
	dir[2] = v2.xyz - point;
	edge = dir[1].Cross( dir[2] );
	t = normal * edge;
	if ( t < -TRIANGLE_EDGE_EPSILON ) {
		return faceDist;
	}
	edge = dir[2].Cross( dir[0] );
	t = normal * edge;
	if ( t < -TRIANGLE_EDGE_EPSILON ) {
		return faceDist;
	}

	// find the point of impact on the triangle plane
	dist /= d;
	testVert = point + dist * normal;

	// calculate barycentric coordinates of the impact point on the high poly triangle
	baseArea = 1.0f / idWinding::TriangleArea( v0.xyz, v1.xyz, v2.xyz );
	bary[0] = idWinding::TriangleArea( testVert, v1.xyz, v2.xyz ) * baseArea;
	bary[1] = idWinding::TriangleArea( v0.xyz, testVert, v2.xyz ) * baseArea;
	bary[2] = idWinding::TriangleArea( v0.xyz, v1.xyz, testVert ) * baseArea;

	if ( bary[0] + bary[1] + bary[2] > 1.1f ) {
		return faceDist;
	}

	dv.xyz = testVert;

	// triangularly interpolate the texture coordinates, normals and colors to the sample point
	// FIXME: SD_USE_DRAWVERT_SIZE_32
	for ( i = 0; i < 2; i++ ) {
		dv._st[ i ] = bary[0] * v0._st[i] + bary[1] * v1._st[i] + bary[2] * v2._st[i];
	}

#if defined( SD_USE_DRAWVERT_SIZE_32 )
	idVec3 n;
	idVec3 n0 = v0.GetNormal();
	idVec3 n1 = v1.GetNormal();
	idVec3 n2 = v2.GetNormal();
	for ( i = 0; i < 3; i++ ) {
		n[ i ] = bary[0] * n0[i] + bary[1] * n1[i] + bary[2] * n2[i];
	}
	n.Normalize();
	dv.SetNormal( n );

	idVec3 tgnt;
	idVec3 tgnt0 = v0.GetTangent();
	idVec3 tgnt1 = v1.GetTangent();
	idVec3 tgnt2 = v2.GetTangent();
	for ( i = 0; i < 3; i++ ) {
		tgnt[ i ] = bary[0] * tgnt0[i] + bary[1] * tgnt1[i] + bary[2] * tgnt2[i];
	}
	tgnt.Normalize();
	dv.SetTangent( tgnt );

	float s0 = v0.GetBiTangentSign();
	float s1 = v1.GetBiTangentSign();
	float s2 = v2.GetBiTangentSign();
	dv.SetBiTangentSign( bary[0] * s0 + bary[1] * s1 + bary[2] * s2 );

#else
	for ( i = 0; i < 3; i++ ) {
		dv._normal[ i ] = bary[0] * v0._normal[i] + bary[1] * v1._normal[i] + bary[2] * v2._normal[i];
	}
	dv._normal.Normalize();

	for ( i = 0; i < 4; i++ ) {
		dv._tangent[i] = bary[0] * v0._tangent[i] + bary[1] * v1._tangent[i] + bary[2] * v2._tangent[i];
	}

#endif

	for ( i = 0; i < 4; i++ ) {
		dv.color[i] = idMath::ClampInt( 0, 255, idMath::FtoiFast( bary[0] * v0.color[i] + bary[1] * v1.color[i] + bary[2] * v2.color[i] ) );
	}

	return dist;
}
