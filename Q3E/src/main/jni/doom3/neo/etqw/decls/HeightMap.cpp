// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "HeightMap.h"

/*
===============================================================================

	sdHeightMap

===============================================================================
*/

/*
==============
sdHeightMap::sdHeightMap
==============
*/
sdHeightMap::sdHeightMap( void ) {
	Clear();
}

/*
==============
sdHeightMap::~sdHeightMap
==============
*/
sdHeightMap::~sdHeightMap( void ) {
	Clear();
}

/*
==============
sdHeightMap::Clear
==============
*/
void sdHeightMap::Clear( void ) {
	data.Clear();

	dimensions[ 0 ] = 0;
	dimensions[ 1 ] = 0;
}

/*
==============
sdHeightMap::Init
==============
*/
void sdHeightMap::Init( int w, int h, byte height ) {
	dimensions[ 0 ] = w;
	dimensions[ 1 ] = h;

	data.AssureSize( dimensions[ 0 ] * dimensions[ 1 ], 0 );
	for (int i=0; i<w; i++) {
		for (int j=0; j<h; j++) {
			data[ i * h + j ] = height;
		}
	}
}

/*
==============
sdHeightMap::Load
==============
*/
void sdHeightMap::Load( const char* filename ) {
	Clear();

	if ( !filename || !*filename ) {
		return;
	}

	byte* pic;
	unsigned timestamp;
	fileSystem->ReadTGA( filename, &pic, &dimensions[ 0 ], &dimensions[ 1 ], &timestamp );

	if ( dimensions[ 0 ] == 0 || dimensions[ 1 ] == 0 ) {
		return;
	}

	data.AssureSize( dimensions[ 0 ] * dimensions[ 1 ], 0 );

	int x;
	for ( x = 0; x < dimensions[ 0 ]; x++ ) {
		int y;
		for ( y = 0; y < dimensions[ 1 ]; y++ ) {
			data[ x + ( y * dimensions[ 0 ] ) ] = pic[ ( x + ( ( dimensions[ 1 ] - 1 - y ) * dimensions[ 0 ] ) ) * 4 ];
		}
	}

	fileSystem->FreeTGA( pic );
}

/*
==============
sdHeightMap::GetInterpolatedHeight
==============
*/
float sdHeightMap::GetInterpolatedHeight( const idVec3& pos, const sdHeightMapScaleData& scale ) const {
	
	// find which points to sample
	int minCoords[ 2 ];
	int maxCoords[ 2 ];
	float blendValues[ 2 ];

	for ( int i = 0; i < 2; i ++ ) {
		float posCoord = idMath::ClampFloat( 0.f, 1.f, ( pos[ i ] - scale.mins[ i ] ) * scale.invSize[ i ] ) * ( dimensions[ i ] - 1 );
		float intCoord = ( int )posCoord;
		float coordLeftover = posCoord - intCoord;

		minCoords[ i ] = intCoord;
		maxCoords[ i ] = idMath::ClampInt( 0, dimensions[ i ] - 1, intCoord + 1 );
		blendValues[ i ] = coordLeftover;
	}

	// sample points
	byte tl = data[ minCoords[ 0 ] + ( maxCoords[ 1 ] * dimensions[ 0 ] ) ];
	byte tr = data[ maxCoords[ 0 ] + ( maxCoords[ 1 ] * dimensions[ 0 ] ) ];
	byte bl = data[ minCoords[ 0 ] + ( minCoords[ 1 ] * dimensions[ 0 ] ) ];
	byte br = data[ maxCoords[ 0 ] + ( minCoords[ 1 ] * dimensions[ 0 ] ) ];

	float combinedTop = Lerp( tl, tr, blendValues[ 0 ] );
	float combinedBottom = Lerp( bl, br, blendValues[ 0 ] );
	float combined = Lerp( combinedBottom, combinedTop, blendValues[ 0 ] );

	return ( combined * scale.heightScale ) + scale.heightOffset;
}

/*
==============
sdHeightMap::GetHeight
==============
*/
void sdHeightMap::GetHeight( const idBounds& in, idVec2& out, const sdHeightMapScaleData& scale ) const {
	if ( data.Num() == 0 ) {
		gameLocal.Warning( "sdHeightMap::GetHeight() - no heightmap data!" );
		out[ 0 ] = 0.0f;
		out[ 1 ] = 0.0f;
		return;
	}

	int minCoords[ 2 ];
	int maxCoords[ 2 ];

	minCoords[ 0 ] = ( idMath::ClampFloat( 0.f, 1.f, ( in.GetMins()[ 0 ] - scale.mins[ 0 ] ) * scale.invSize[ 0 ] ) * ( dimensions[ 0 ] - 1 ) );
	minCoords[ 1 ] = ( idMath::ClampFloat( 0.f, 1.f, ( in.GetMins()[ 1 ] - scale.mins[ 1 ] ) * scale.invSize[ 1 ] ) * ( dimensions[ 1 ] - 1 ) );

	maxCoords[ 0 ] = ( idMath::ClampFloat( 0.f, 1.f, ( in.GetMaxs()[ 0 ] - scale.mins[ 0 ] ) * scale.invSize[ 0 ] ) * ( dimensions[ 0 ] - 1 ) ) + 1;
	maxCoords[ 1 ] = ( idMath::ClampFloat( 0.f, 1.f, ( in.GetMaxs()[ 1 ] - scale.mins[ 1 ] ) * scale.invSize[ 1 ] ) * ( dimensions[ 1 ] - 1 ) ) + 1;

	int x;
	int y;
	float value;

	out[ 0 ] = idMath::INFINITY;
	out[ 1 ] = -idMath::INFINITY;

	for ( x = minCoords[ 0 ]; x < maxCoords[ 0 ]; x++ ) {
		for ( y = minCoords[ 1 ]; y < maxCoords[ 1 ]; y++ ) {
			value = ( data[ x + ( y * dimensions[ 0 ] ) ] * scale.heightScale ) + scale.heightOffset;
			if ( value < out[ 0 ] ) {
				out[ 0 ] = value;
			}
			if ( value > out[ 1 ] ) {
				out[ 1 ] = value;
			}
		}
	}
}

/*
==============
sdHeightMap::GetHeight
==============
*/
float sdHeightMap::GetHeight( const idVec3& start, const idVec3& end, const sdHeightMapScaleData& scale ) const {
	if ( data.Num() == 0 ) {
		gameLocal.Warning( "sdHeightMap::TracePoint() - no heightmap data!" );
		return 0.0f;
	}

	// find the start & end points in heightmap coordinates
	idVec2 startCoords;
	idVec2 endCoords;
	startCoords[ 0 ] = ( start[ 0 ] - scale.mins[ 0 ] ) * ( scale.invSize[ 0 ] * ( dimensions[ 0 ] - 1 ) );
	startCoords[ 1 ] = ( start[ 1 ] - scale.mins[ 1 ] ) * ( scale.invSize[ 1 ] * ( dimensions[ 1 ] - 1 ) );
	endCoords[ 0 ] = ( end[ 0 ] - scale.mins[ 0 ] ) * ( scale.invSize[ 0 ] * ( dimensions[ 0 ] - 1 ) );
	endCoords[ 1 ] = ( end[ 1 ] - scale.mins[ 1 ] ) * ( scale.invSize[ 1 ] * ( dimensions[ 1 ] - 1 ) );

	const float left = 0.0f;
	const float right = dimensions[ 0 ] - 1.0f;
	const float top = 0.0f;
	const float bottom = dimensions[ 1 ] - 1.0f;

	// find if the trace is entirely outside the map
	if ( ( startCoords[ 0 ] < left && endCoords[ 0 ] < left ) || ( startCoords[ 0 ] > right && endCoords[ 0 ] > right ) ) {
		return 0.0f;
	}
	if ( ( startCoords[ 1 ] < top && endCoords[ 1 ] < top ) || ( startCoords[ 1 ] > bottom && endCoords[ 1 ] > bottom ) ) {
		return 0.0f;
	}

	idVec2 coordDelta = endCoords - startCoords;
	idVec2 direction = coordDelta;
	direction.Normalize();

	// clip the start & end coords to the dimensions of the map whilst keeping the same slopes
	if ( startCoords[ 0 ] < left ) startCoords += ( ( left - startCoords[ 0 ] ) / direction[ 0 ] ) * direction;
	if ( startCoords[ 0 ] > right ) startCoords += ( ( right - startCoords[ 0 ] ) / direction[ 0 ] ) * direction;
	if ( startCoords[ 1 ] < top ) startCoords += ( ( top - startCoords[ 1 ] ) / direction[ 1 ] ) * direction;
	if ( startCoords[ 1 ] > bottom ) startCoords += ( ( bottom - startCoords[ 1 ] ) / direction[ 1 ] ) * direction;

	if ( endCoords[ 0 ] < left ) endCoords += ( ( left - endCoords[ 0 ] ) / direction[ 0 ] ) * direction;
	if ( endCoords[ 0 ] > right ) endCoords += ( ( right - endCoords[ 0 ] ) / direction[ 0 ] ) * direction;
	if ( endCoords[ 1 ] < top ) endCoords += ( ( top - endCoords[ 1 ] ) / direction[ 1 ] ) * direction;
	if ( endCoords[ 1 ] > bottom ) endCoords += ( ( bottom - endCoords[ 1 ] ) / direction[ 1 ] ) * direction;;

	// update the coorddelta
	coordDelta = endCoords - startCoords;

	// check if the trace is a vertical one
	if ( idMath::Fabs( coordDelta[ 0 ] ) < 1.0f && idMath::Fabs( coordDelta[ 1 ] ) < 1.0f ) {
		return GetHeight( start, scale );
	}

	// find the step to go by
	int mostSignificantAxis = 0;
	if ( idMath::Fabs( direction[ 1 ] ) > idMath::Fabs( direction[ 0 ] ) ) {
		mostSignificantAxis = 1;
	}

	float axisStep = idMath::Fabs( direction[ mostSignificantAxis ] );
	idVec2 step = direction /  axisStep;
	if ( step[ mostSignificantAxis ] > 0.0f ) {
		step[ mostSignificantAxis ] = 1.0f;		// paranoid
	} else {
		step[ mostSignificantAxis ] = -1.0f;	// paranoid
	}

	int travelled = 0;
	int fullDistance = ( int )endCoords[ mostSignificantAxis ] - ( int )startCoords[ mostSignificantAxis ];
	fullDistance = idMath::Abs( fullDistance );
	byte maxHeight = 0;

	// TODO: Could optimize this a lot by using ints only in here
	idVec2 testCoords = startCoords;
	for ( int travelled = 0; travelled < fullDistance; travelled++ ) {
		int intTestCoords[ 2 ];
		intTestCoords[ 0 ] = testCoords[ 0 ];
		intTestCoords[ 1 ] = testCoords[ 1 ];

		byte height = data[ intTestCoords[ 0 ] + ( intTestCoords[ 1 ] * dimensions[ 0 ] ) ];
		if ( height > maxHeight ) {
			maxHeight = height;
		}

		testCoords += step;
	}

	return ( maxHeight * scale.heightScale ) + scale.heightOffset;
}

/*
==============
sdHeightMap::TracePoint
==============
*/
float sdHeightMap::TracePoint( const idVec3& start, const idVec3& end, idVec3& result, float heightOffset, const sdHeightMapScaleData& scale ) const {
	result = end;
	if ( data.Num() == 0 ) {
		gameLocal.Warning( "sdHeightMap::TracePoint() - no heightmap data!" );
		return 1.0f;
	}

	// find the start & end points in heightmap coordinates
	idVec3 startCoords, unclippedStartCoords;
	idVec3 endCoords, unclippedEndCoords;
	startCoords[ 0 ] = ( start[ 0 ] - scale.mins[ 0 ] ) * ( scale.invSize[ 0 ] * ( dimensions[ 0 ] - 1 ) );
	startCoords[ 1 ] = ( start[ 1 ] - scale.mins[ 1 ] ) * ( scale.invSize[ 1 ] * ( dimensions[ 1 ] - 1 ) );
	startCoords[ 2 ] = ( ( start[ 2 ] - heightOffset ) - scale.heightOffset ) / scale.heightScale;
	endCoords[ 0 ] = ( end[ 0 ] - scale.mins[ 0 ] ) * ( scale.invSize[ 0 ] * ( dimensions[ 0 ] - 1 ) );
	endCoords[ 1 ] = ( end[ 1 ] - scale.mins[ 1 ] ) * ( scale.invSize[ 1 ] * ( dimensions[ 1 ] - 1 ) );
	endCoords[ 2 ] = ( ( end[ 2 ] - heightOffset ) - scale.heightOffset ) / scale.heightScale;

	unclippedStartCoords = startCoords;
	unclippedEndCoords = endCoords;

	const float left = 0.0f;
	const float right = dimensions[ 0 ] - 1.0f;
	const float top = 0.0f;
	const float bottom = dimensions[ 1 ] - 1.0f;

	// find if the trace is entirely outside the map
	if ( ( startCoords[ 0 ] < left && endCoords[ 0 ] < left ) || ( startCoords[ 0 ] > right && endCoords[ 0 ] > right ) ) {
		return 1.0f;
	}
	if ( ( startCoords[ 1 ] < top && endCoords[ 1 ] < top ) || ( startCoords[ 1 ] > bottom && endCoords[ 1 ] > bottom ) ) {
		return 1.0f;
	}
	if ( ( startCoords[ 2 ] < 0.0f && endCoords[ 2 ] < 0.0f ) || ( startCoords[ 2 ] > 255.0f && endCoords[ 2 ] > 255.0f ) ) {
		return 1.0f;
	}

	idVec3 coordDelta = endCoords - startCoords;
	idVec3 direction = coordDelta;
	direction.Normalize();

	// clip the start & end coords to the dimensions of the map whilst keeping the same slopes
	// yes, ugly formatting, but it stops it being huuuuge
	if ( startCoords[ 0 ] < left ) startCoords += ( ( left - startCoords[ 0 ] ) / direction[ 0 ] ) * direction;
	if ( startCoords[ 0 ] > right )	startCoords += ( ( right - startCoords[ 0 ] ) / direction[ 0 ] ) * direction;
	if ( startCoords[ 1 ] < top ) startCoords += ( ( top - startCoords[ 1 ] ) / direction[ 1 ] ) * direction;
	if ( startCoords[ 1 ] > bottom ) startCoords += ( ( bottom - startCoords[ 1 ] ) / direction[ 1 ] ) * direction;
	if ( startCoords[ 2 ] < 0.0f ) startCoords += ( ( -startCoords[ 2 ] ) / direction[ 2 ] ) * direction;
	if ( startCoords[ 2 ] > 255.0f ) startCoords += ( ( 255.0f - startCoords[ 2 ] ) / direction[ 2 ] ) * direction;

	if ( endCoords[ 0 ] < left ) endCoords += ( ( left - endCoords[ 0 ] ) / direction[ 0 ] ) * direction;
	if ( endCoords[ 0 ] > right ) endCoords += ( ( right - endCoords[ 0 ] ) / direction[ 0 ] ) * direction;
	if ( endCoords[ 1 ] < top ) endCoords += ( ( top - endCoords[ 1 ] ) / direction[ 1 ] ) * direction;
	if ( endCoords[ 1 ] > bottom ) endCoords += ( ( bottom - endCoords[ 1 ] ) / direction[ 1 ] ) * direction;
	if ( endCoords[ 2 ] < 0.0f ) endCoords += ( ( -endCoords[ 2 ] ) / direction[ 2 ] ) * direction;
	if ( endCoords[ 2 ] > 255.0f ) endCoords += ( ( 255.0f - endCoords[ 2 ] ) / direction[ 2 ] ) * direction;

	// update the coorddelta
	coordDelta = endCoords - startCoords;

	// check if the trace is a vertical one
	if ( idMath::Fabs( coordDelta[ 0 ] ) < 1.0f && idMath::Fabs( coordDelta[ 1 ] ) < 1.0f ) {
		// not tracing far enough to do anything
		if ( idMath::Fabs( coordDelta[ 2 ] ) < 1.0f ) {
			return 1.0f;
		}

		// tracing up can never return a hit
		if ( endCoords[ 2 ] > startCoords[ 2 ] ) {
			return 1.0f;
		}

		int intStart[ 2 ];
		intStart[ 0 ] = startCoords[ 0 ];
		intStart[ 1 ] = startCoords[ 1 ];
		float height = data[ intStart[ 0 ] + ( intStart[ 1 ] * dimensions[ 0 ] ) ];

		if ( height < startCoords[ 2 ] ) {
			float fraction = ( unclippedStartCoords[ 2 ] - height ) / ( unclippedStartCoords[ 2 ] - unclippedEndCoords[ 2 ] );
			if ( fraction > 1.0f ) {
				fraction = 1.0f;
			}
			result = Lerp( start, end, fraction );
			return fraction;
		}
	}

	// Ok, clipped to the heightmap
	// loop along the path & find where this crosses the map
	int mostSignificantAxis = 0;
	int mostSignificantFractionAxis = 0;
	if ( idMath::Fabs( direction[ 1 ] ) > idMath::Fabs( direction[ 0 ] ) ) {
		mostSignificantAxis = 1;
		mostSignificantFractionAxis = 1;
	}
	if ( idMath::Fabs( direction[ 2 ] ) > idMath::Fabs( direction[ mostSignificantFractionAxis ] ) ) {
		mostSignificantFractionAxis = 2;
	}

	// find the step to go by
	float axisStep = idMath::Fabs( direction[ mostSignificantAxis ] );
	idVec3 step = direction /  axisStep;
	if ( step[ mostSignificantAxis ] > 0.0f ) {
		step[ mostSignificantAxis ] = 1.0f;		// paranoid
	} else {
		step[ mostSignificantAxis ] = -1.0f;	// paranoid
	}

	bool first = true;
	bool wasAbove;
	int travelled = 0;
	int fullDistance = ( int )endCoords[ mostSignificantAxis ] - ( int )startCoords[ mostSignificantAxis ];
	fullDistance = idMath::Abs( fullDistance );

	// TODO: Could optimize this a lot by using ints only in here
	idVec3 testCoords = startCoords;
	for ( int travelled = 0; travelled < fullDistance; travelled++ ) {
		int intTestCoords[ 3 ];
		intTestCoords[ 0 ] = testCoords[ 0 ];
		intTestCoords[ 1 ] = testCoords[ 1 ];

		float height = data[ intTestCoords[ 0 ] + ( intTestCoords[ 1 ] * dimensions[ 0 ] ) ];
		intTestCoords[ 2 ] = height;
		bool above = testCoords[ 2 ] >= height;
		if ( first ) {
			wasAbove = above;
			first = false;
		} else {
			if ( !above && wasAbove ) {
				// it crossed through the heightmap
				float fraction = intTestCoords[ mostSignificantFractionAxis ] - unclippedStartCoords[ mostSignificantFractionAxis ];
				fraction /= unclippedEndCoords[ mostSignificantFractionAxis ] - unclippedStartCoords[ mostSignificantFractionAxis ];
				result = Lerp( start, end, fraction );

				return fraction;
			}
		}

		testCoords += step;
	}

	return 1.0f;
}
