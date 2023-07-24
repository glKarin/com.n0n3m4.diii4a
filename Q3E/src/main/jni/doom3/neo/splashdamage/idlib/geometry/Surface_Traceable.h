// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __SURFACE_TRACEABLE_H__
#define __SURFACE_TRACEABLE_H__

/*
===============================================================================

	Traceable surface.

===============================================================================
*/

class idSurface_Traceable : public idSurface {
public:
								idSurface_Traceable( void );
								idSurface_Traceable( const idSurface &surf ) :
									idSurface( surf ),
									hash( NULL ),
									traceCount( 0 ),
									triPlanes( NULL ),
									lastTriTrace( NULL ) {
									bounds.Clear();
								}
								~idSurface_Traceable( void );

	const idBounds&				GetBounds( void );
	void						SetTraceFraction( const float traceFraction );

	void						OptimizeForTracing( const float traceFraction = 1.f );

								// intersection point is start + dir * scale
	bool						RayIntersection( /*idList< int >& tracedTris,*/ const idVec3 &start, const idVec3 &dir, float &scale, idDrawVert& dv, bool backFaceCull = false ) const;

protected:
	class sdTraceableTriangleHash {
	public:
		struct hashTriangle_t {
			int				triIndex;
			hashTriangle_t*	next;
		};

		struct hashBin_t {
			size_t			lastTrace;
			hashTriangle_t*	triangleList;
		};

							sdTraceableTriangleHash( idSurface_Traceable& surface, const int binsPerAxis = 50, const int snapFractions = 32 );
							~sdTraceableTriangleHash( void );

		const idBounds&		GetBounds( void ) const { return bounds; }
		hashBin_t*			GetHashBin( const idVec3& point );

	private:
		int					binsPerAxis;
		int					snapFractions;
		idBounds			bounds;

		hashBin_t*			bins;
		idVec3				scale;
		int					intMins[ 3 ];
		int					intScale[ 3 ];

		idBlockAlloc< hashTriangle_t, 128 >	triangleAllocator;

	private:
		int					BinIndex( const int x, const int y, const int z ) const { return ( ( binsPerAxis * binsPerAxis * z ) + ( binsPerAxis * y ) + x ); }
	};

	idBounds					bounds;
	sdTraceableTriangleHash*	hash;

	float						traceDist;

	mutable size_t				traceCount;

	idPlane*					triPlanes;
	size_t*						lastTriTrace;

protected:
	void						GenerateHash( void );
	void						GenerateIntersectionDrawVert( const idVec3& intersection, const int intersectedTriIndex, idDrawVert& dv ) const;
};

/*
====================
idSurface_Traceable::sdTraceableTriangleHash::~sdTraceableTriangleHash
====================
*/
ID_INLINE idSurface_Traceable::sdTraceableTriangleHash::~sdTraceableTriangleHash( void ) {
	delete [] bins;
	triangleAllocator.Shutdown();
}

/*
====================
idSurface_Traceable::sdTraceableTriangleHash::GetHashBin
====================
*/
ID_INLINE idSurface_Traceable::sdTraceableTriangleHash::hashBin_t* idSurface_Traceable::sdTraceableTriangleHash::GetHashBin( const idVec3& point ) {
	int block[ 3 ];

	// snap the point to integral values
	for ( int i = 0; i < 3; i++ ) {
		block[ i ] = idMath::Ftoi( idMath::Floor( ( point[ i ] + .5f / snapFractions ) * snapFractions ) );
		block[ i ] = ( block[ i ] - intMins[ i ] ) / intScale[ i ];
		if ( block[ i ] < 0 ) {
			block[ i ] = 0;
		} else if ( block[ i ] >= binsPerAxis ) {
			block[ i ] = binsPerAxis - 1;
		}
	}

	return &bins[ BinIndex( block[ 0 ], block[ 1 ], block[ 2 ] ) ];
}

/*
====================
idSurface_Traceable::idSurface_Traceable
====================
*/
ID_INLINE idSurface_Traceable::idSurface_Traceable( void ) :
	hash( NULL ),
	traceCount( 0 ),
	triPlanes( NULL ),
	lastTriTrace( NULL ) {
	bounds.Clear();
}

/*
====================
idSurface_Traceable::~idSurface_Traceable
====================
*/
ID_INLINE idSurface_Traceable::~idSurface_Traceable( void ) {
	delete hash;
	delete [] triPlanes;
	delete [] lastTriTrace;
}

/*
====================
idSurface_Traceable::GetBounds
====================
*/
ID_INLINE const idBounds& idSurface_Traceable::GetBounds( void ) {

	if ( bounds.IsCleared() ) {
		//SIMDProcessor->MinMax( bounds[0], bounds[1], verts.Begin(), indexes.Begin(), indexes.Num() );
	} 

	return bounds;
}

/*
====================
idSurface_Traceable::SetTraceFraction
====================
*/
ID_INLINE void idSurface_Traceable::SetTraceFraction( const float traceFraction ) {
	traceDist = 0.f;

	// the traceDist will be the traceFrac times the largest bounds axis
	for ( int i = 0; i < 3; i++ ) {
		float d;
		d = traceFraction * ( GetBounds()[1][i] - GetBounds()[0][i] );
		if ( d > traceDist ) {
			traceDist = d;
		}
	}
}

/*
====================
idSurface_Traceable::OptimizeForTracing
====================
*/
ID_INLINE void idSurface_Traceable::OptimizeForTracing( const float traceFraction ) {
	delete hash;
	delete [] triPlanes;
	delete [] lastTriTrace;

	hash = new sdTraceableTriangleHash( *this );

	triPlanes = new idPlane[ indexes.Num() / 3 ];
	SIMDProcessor->DeriveTriPlanes( triPlanes, verts.Begin(), verts.Num(), indexes.Begin(), indexes.Num() );

	lastTriTrace = new size_t[ indexes.Num() / 3 ];
	memset( lastTriTrace, 0, indexes.Num() / 3 * sizeof( size_t ) );

	SetTraceFraction( traceFraction );
}

#endif /* !__SURFACE_TRACEABLE_H__ */
