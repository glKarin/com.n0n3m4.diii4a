// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_DECLS_HEIGHTMAP_H__
#define __GAME_DECLS_HEIGHTMAP_H__

class sdHeightMapScaleData {
public:
	void Init( const idBounds& bounds ) {
		mins			= bounds.GetMins();
		size			= bounds.Size();

		for ( int i = 0; i < 3; i++ ) {
			invSize[ i ] = 1 / size[ i ];
		}

		heightScale		= size[ 2 ] / 256.f;
		heightOffset	= mins[ 2 ];
	}

	idVec3							mins;
	idVec3							size;
	idVec3							invSize;
	float							heightScale;
	float							heightOffset;
};

class sdHeightMap {
public:
									sdHeightMap( void );
									~sdHeightMap( void );

	void							Clear( void );
	void							Init( int w, int h, byte height );

	void							Load( const char* filename );

	float GetHeight( const idVec3& pos, const sdHeightMapScaleData& scale ) const {
		int coords[ 2 ];
		coords[ 0 ] = idMath::ClampFloat( 0.f, 1.f, ( pos[ 0 ] - scale.mins[ 0 ] ) * scale.invSize[ 0 ] ) * ( dimensions[ 0 ] - 1 );
		coords[ 1 ] = idMath::ClampFloat( 0.f, 1.f, ( pos[ 1 ] - scale.mins[ 1 ] ) * scale.invSize[ 1 ] ) * ( dimensions[ 1 ] - 1 );
		return ( data[ coords[ 0 ] + ( coords[ 1 ] * dimensions[ 0 ] ) ] * scale.heightScale ) + scale.heightOffset;
	}

	float							GetInterpolatedHeight( const idVec3& pos, const sdHeightMapScaleData& scale ) const;
	void							GetHeight( const idBounds& pos, idVec2& out, const sdHeightMapScaleData& scale ) const;
	float							GetHeight( const idVec3& start, const idVec3& end, const sdHeightMapScaleData& scale ) const;

	// this does a kind-of-trace through the heightmap for really rough approximation work
	// where you don't want to do it in the full physics world
	// heightOffset acts as if you're tracing through an offset world
	float							TracePoint( const idVec3& start, const idVec3& end, idVec3& result, float heightOffset, const sdHeightMapScaleData& scale ) const;

	bool							IsValid( void ) const { return !data.Empty(); }

private:
	idList< byte >					data;
	int								dimensions[ 2 ];
};

#endif // __GAME_DECLS_HEIGHTMAP_H__
