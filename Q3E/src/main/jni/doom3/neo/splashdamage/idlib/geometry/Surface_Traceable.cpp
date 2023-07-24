// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

/*
====================
idSurface_Traceable::sdTraceableTriangleHash::sdTraceableTriangleHash
====================
*/
idSurface_Traceable::sdTraceableTriangleHash::sdTraceableTriangleHash( idSurface_Traceable& surface, const int binsPerAxis, const int snapFractions ) {
	
	this->binsPerAxis = binsPerAxis;
	this->snapFractions = snapFractions;
	bounds = surface.GetBounds();

	// spread the bounds so it will never have a zero size
	for ( int i = 0; i < 3; i++ ) {
		bounds[0][i] = idMath::Floor( bounds[0][i] - 1 );
		bounds[1][i] = idMath::Ceil( bounds[1][i] + 1 );
		scale[i] = ( bounds[1][i] - bounds[0][i] ) / binsPerAxis;

		intMins[i] = idMath::Ftoi( bounds[0][i] * snapFractions );
		intScale[i] = idMath::Ftoi( scale[i] * snapFractions );
		if ( intScale[ i ] < 1 ) {
			intScale[ i ] = 1;
		}
	}

	bins = new hashBin_t[ binsPerAxis * binsPerAxis * binsPerAxis ];
	SIMDProcessor->Memset( bins, 0, binsPerAxis * binsPerAxis * binsPerAxis * sizeof( hashBin_t ) );

    // insert triangles into bins
	hashTriangle_t*		tri;
	idBounds			triBounds;
	int					triBins[2][3];
	const idDrawVert*	verts = surface.GetVertices();
	const vertIndex_t*	indexes = surface.GetIndexes();
	
	for ( int i = 0; i < surface.GetNumIndexes(); i += 3 ) {
		triBounds.Clear();
		for ( int k = 0; k < 3; k++ ) {
			triBounds.AddPoint( verts[ indexes[ i + k ] ].xyz );
		}
		for ( int k = 0; k < 3; k++ ) {
			triBins[ 0 ][ k ] = idMath::Ftoi( idMath::Floor( ( triBounds[ 0 ][ k ] + .5f / snapFractions ) * snapFractions ) );
			triBins[ 0 ][ k ] = ( triBins[ 0 ][ k ] - intMins[ k ] ) / intScale[ k ];
			if ( triBins[ 0 ][ k ] < 0 ) {
				triBins[ 0 ][ k ] = 0;
			} else if ( triBins[ 0 ][ k ] >= binsPerAxis ) {
				triBins[ 0 ][ k ] = binsPerAxis - 1;
			}

			triBins[ 1 ][ k ] = idMath::Ftoi(idMath::Ceil( ( triBounds[ 1 ][ k ] + .5f / snapFractions ) * snapFractions ) );
			triBins[ 1 ][ k ] = ( triBins[ 1 ][ k ] - intMins[ k ] ) / intScale[ k ];
			if ( triBins[ 1 ][ k ] < 0 ) {
				triBins[ 1 ][ k ] = 0;
			} else if ( triBins[ 1 ][ k ] >= binsPerAxis ) {
				triBins[ 1 ][ k ] = binsPerAxis - 1;
			}
		}

		for ( int k = triBins[ 0 ][ 0 ]; k <= triBins[ 1 ][ 0 ]; k++ ) {
			for ( int l = triBins[ 0 ][ 1 ]; l <= triBins[ 1 ][ 1 ]; l++ ) {
				for ( int m = triBins[ 0 ][ 2 ]; m <= triBins[ 1 ][ 2 ]; m++ ) {
					tri = triangleAllocator.Alloc();
					tri->triIndex = i;
					tri->next = bins[ BinIndex( k, l, m ) ].triangleList;
					bins[ BinIndex( k, l, m ) ].triangleList = tri;
				}
			}
		}
	}


}

/*
====================
idSurface_Traceable::RayIntersection
====================
*/
bool idSurface_Traceable::RayIntersection( /*idList< int >& tracedTris,*/ const idVec3& start, const idVec3& dir, float& scale, idDrawVert& dv, bool backFaceCull ) const {

	//tracedTris.Clear();

	if ( !hash ) {
		return false;
	}

	if ( !hash->GetBounds().RayIntersection( start, dir, scale ) ) {
		return false;
	}

	traceCount++;

	idVec3 point = start + dir * scale;

	int edgeIndex[ 3 ];
	int side[ 3 ];
	int absEdgeIndex;
	float d;
	sdBitField_Stack sidedness;
	sdBitField_Stack sidednessCalculated;
	idPluecker rayPl, pl;

	int sizeForEdges = sdBitField_Stack::GetSizeForMaxBits( edges.Num() );

	sidedness.Init( (int*)_alloca( sizeForEdges * sizeof( int ) ), sizeForEdges );
	sidedness.Clear();
	sidednessCalculated.Init( (int*)_alloca( sizeForEdges * sizeof( int ) ), sizeForEdges );
	sidednessCalculated.Clear();

	rayPl.FromRay( start, dir );

	//const int RAY_STEPS = 400;
	const int RAY_STEPS = 1000;

	idVec3 step = dir * ( 2.f / static_cast< float >( RAY_STEPS ) ) * traceDist;

	for ( int i = 0; i < RAY_STEPS; i++, point += step ) {
		sdTraceableTriangleHash::hashBin_t* bin = hash->GetHashBin( point );

		if ( !bin || bin->lastTrace == traceCount ) {
			continue;
		}
		bin->lastTrace = traceCount;

		for ( sdTraceableTriangleHash::hashTriangle_t* tri = bin->triangleList; tri; tri = tri->next ) {
			
			if ( lastTriTrace[ tri->triIndex / 3 ] == traceCount ) {
				continue;
			}
			lastTriTrace[ tri->triIndex / 3 ] = traceCount;

			//tracedTris.Append( tri->triIndex );

			for ( int j = 0; j < 3; j++ ) {
				edgeIndex[ j ] = edgeIndexes[ tri->triIndex + j ];

				absEdgeIndex = abs( edgeIndex[ j ] );

				if ( !sidednessCalculated.Get( absEdgeIndex ) ) {
					pl.FromLine( verts[ edges[ absEdgeIndex ].verts[ 1 ] ].xyz, verts[ edges[ absEdgeIndex ].verts[ 0 ] ].xyz );
					d = pl.PermutedInnerProduct( rayPl );
					sidedness.Set( absEdgeIndex, FLOATSIGNBITSET( d ) );
					sidednessCalculated.Set( absEdgeIndex );
				}

				side[ j ] = sidedness.Get( absEdgeIndex ) ^ INTSIGNBITSET( edgeIndex[ j ] );
			}

			if ( side[ 0 ] & side[ 1 ] & side[ 2 ] ) {
				if ( triPlanes[ tri->triIndex / 3 ].RayIntersection( start, dir, scale ) && !FLOATSIGNBITSET( scale ) ) {
					GenerateIntersectionDrawVert( start + dir * scale, tri->triIndex, dv );
					return true;
				}
			} else if ( !backFaceCull && !(side[ 0 ] | side[ 1 ] | side[ 2 ]) ) {
				if ( triPlanes[ tri->triIndex / 3 ].RayIntersection( start, dir, scale ) && !FLOATSIGNBITSET( scale ) ) {
					GenerateIntersectionDrawVert( start + dir * scale, tri->triIndex, dv );
					return true;
				}
			}

		}
	}

	return false;
}

/*
====================
idSurface_Traceable::RayIntersection
====================
*/
void idSurface_Traceable::GenerateIntersectionDrawVert( const idVec3& intersection, const int intersectedTriIndex, idDrawVert& dv ) const {
	float denom;
	float a, b, c;

	const idDrawVert* v0 = &verts[ indexes[ intersectedTriIndex + 0 ] ];
	const idDrawVert* v1 = &verts[ indexes[ intersectedTriIndex + 1 ] ];
	const idDrawVert* v2 = &verts[ indexes[ intersectedTriIndex + 2 ] ];

	// find the barycentric coordinates
	denom = idWinding::TriangleArea( v0->xyz, v1->xyz, v2->xyz );

	a = idWinding::TriangleArea( intersection, v1->xyz, v2->xyz ) / denom;
	b = idWinding::TriangleArea( v0->xyz, intersection, v2->xyz ) / denom;
	c = idWinding::TriangleArea( v0->xyz, v1->xyz, intersection ) / denom;

	// generate the interpolated values
	dv.xyz = intersection;

	// FIXME: SD_USE_DRAWVERT_SIZE_32
	for ( int i = 0; i < 2; i++ ) {
		dv._st[ i ] = a * v0->_st[ i ] + b * v1->_st[ i ] + c * v2->_st[ i ];
	}

#if defined( SD_USE_DRAWVERT_SIZE_32 )
	idVec3 n;
	idVec3 n0 = v0->GetNormal();
	idVec3 n1 = v1->GetNormal();
	idVec3 n2 = v2->GetNormal();
	for ( int i = 0; i < 3; i++ ) {
		n[ i ] = a * n0[i] + b * n1[i] + c * n2[i];
	}
	n.Normalize();
	dv.SetNormal( n );

	idVec3 tgnt;
	idVec3 tgnt0 = v0->GetTangent();
	idVec3 tgnt1 = v1->GetTangent();
	idVec3 tgnt2 = v2->GetTangent();
	for ( int i = 0; i < 3; i++ ) {
		tgnt[ i ] = a * tgnt0[i] + b * tgnt1[i] + c * tgnt2[i];
	}
	tgnt.Normalize();
	dv.SetTangent( tgnt );

	float s0 = v0->GetBiTangentSign();
	float s1 = v1->GetBiTangentSign();
	float s2 = v2->GetBiTangentSign();
	dv.SetBiTangentSign( a * s0 + b * s1 + c * s2 );

#else

	for ( int i = 0; i < 3; i++ ) {
		dv._normal[ i ] = a * v0->_normal[ i ] + b * v1->_normal[ i ] + c * v2->_normal[ i ];
	}
	dv._normal.Normalize();

	for ( int i = 0; i < 4; i++ ) {
		dv._tangent[ i ] = a * v0->_tangent[ i ] + b * v1->_tangent[ i ] + c * v2->_tangent[ i ];
	}
#endif

	for ( int i = 0; i < 4; i++ ) {
		dv.color[ i ] = idMath::Ftob( a * v0->color[ i ] + b * v1->color[ i ] + c * v2->color[ i ] );
	}
}
