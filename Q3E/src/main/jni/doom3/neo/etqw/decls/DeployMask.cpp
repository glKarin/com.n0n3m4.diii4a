// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DeployMask.h"

/*
===============================================================================

	sdDeployMask

===============================================================================
*/

/*
==============
sdDeployMask::sdDeployMask
==============
*/
sdDeployMask::sdDeployMask( void ) {
	dimensions[ 0 ] = 0;
	dimensions[ 1 ] = 0;
}

/*
==============
sdDeployMask::Load
==============
*/
bool sdDeployMask::Load( const char* _fileName ) {
	fileName = _fileName;

	dimensions[ 0 ] = 0;
	dimensions[ 1 ] = 0;

	byte* pic = NULL;
	fileSystem->ReadTGA( fileName.c_str(), &pic, &dimensions[ 0 ], &dimensions[ 1 ] );

	if ( pic == NULL ) {
		dimensions[ 0 ] = 1;
		dimensions[ 1 ] = 1;
		data.Init( 1 );
		data.Clear();
		return false;
	}

	int size = dimensions[ 0 ] * dimensions[ 1 ];
	data.Init( size );
	data.Clear();

	for ( int x = 0; x < dimensions[ 0 ]; x++ ) {
		for ( int y = 0; y < dimensions[ 1 ]; y++ ) {
			if ( pic[ ( x + ( ( dimensions[ 1 ] - 1 - y ) * dimensions[ 0 ] ) ) * 4 ] > 0 ) {
				data.Set( x + ( y * dimensions[ 0 ] ) );
			}
		}
	}

	fileSystem->FreeTGA( pic );

	return true;
}

/*
==============
sdDeployMask::Clear
==============
*/
void sdDeployMask::Clear( void ) {
	data.Shutdown();
}

/*
==============
sdDeployMask::~sdDeployMask
==============
*/
sdDeployMask::~sdDeployMask( void ) {
}

/*
==============
sdDeployMask::WriteTGA
==============
*/
void sdDeployMask::WriteTGA( void ) const {
	if ( !IsValid() ) {
		return;
	}

	byte* buffer = new byte[ dimensions[ 0 ] * dimensions[ 1 ] * 4 ];

	for ( int x = 0; x < dimensions[ 0 ]; x++ ) {
		for ( int y = 0; y < dimensions[ 1 ]; y++ ) {
			int index = ( x + ( ( dimensions[ 1 ] - 1 - y ) * dimensions[ 0 ] ) ) * 4;

			if ( data.Get( x + ( y * dimensions[ 0 ] ) ) ) {
				buffer[ index + 0 ] = 255;
				buffer[ index + 1 ] = 255;
				buffer[ index + 2 ] = 255;
				buffer[ index + 3 ] = 255;
			} else {
				buffer[ index + 0 ] = 0;
				buffer[ index + 1 ] = 0;
				buffer[ index + 2 ] = 0;
				buffer[ index + 3 ] = 255;
			}
		}
	}
	
	fileSystem->WriteTGA( fileName.c_str(), buffer, dimensions[ 0 ], dimensions[ 1 ] );

	delete[] buffer;
}

/*
==============
sdDeployMask::IsValid
==============
*/
void sdDeployMask::DebugDraw( const sdDeployMaskBounds& bounds ) const {
	idVec3 dims;
	dims[ 2 ] = 0.f;

	idVec2 temp = bounds.worldBounds.GetCenter();
	const sdPlayZone* pz = gameLocal.GetPlayZone( idVec3( temp.x, temp.y, 0.f ), sdPlayZone::PZF_HEIGHTMAP );
	const sdHeightMapInstance* hm = NULL;
	if ( pz != NULL ) {
		hm = &pz->GetHeightMap();
	}

	idVec3 point;
	idVec3 up( 0.f, 0.f, 1.f );

	float fdim[ 2 ];
	fdim[ 0 ] = dimensions[ 0 ];
	fdim[ 1 ] = dimensions[ 1 ];

	for ( int x = 0; x < dimensions[ 0 ]; x++) {
		for ( int y = 0; y < dimensions[ 1 ]; y++ ) {
			dims[ 0 ] = ( x + 0.5f ) / fdim[ 0 ];
			dims[ 1 ] = ( y + 0.5f ) / fdim[ 1 ];
			
			point[ 0 ] = bounds.worldBounds.GetMins()[ 0 ] + ( bounds.worldSize[ 0 ] * dims[ 0 ] );
			point[ 1 ] = bounds.worldBounds.GetMins()[ 1 ] + ( bounds.worldSize[ 1 ] * dims[ 1 ] );
			point[ 2 ] = 0.f;
			
			if ( hm != NULL ) {
				point[ 2 ] = hm->GetHeight( point );
			}

			int value = data.Get( x + ( y * dimensions[ 0 ] ) );
			gameRenderWorld->DebugLine( value ? colorGreen : colorRed, point, point + ( up * 128.f ) );
		}
	}
}

/*
==============
sdDeployMask::SnapToGrid
==============
*/
idVec3 sdDeployMask::SnapToGrid( const idVec3& point, float snapScale, const sdDeployMaskBounds& bounds ) const {
	idVec3 out;
	out.x = point.x - bounds.worldBounds.GetMins().x;
	out.y = point.y - bounds.worldBounds.GetMins().y;
	out.z = point.z;

	float snapResolution = 1 / snapScale;

	float w = GetBoxWidth( bounds ) * snapResolution;
	float h = GetBoxHeight( bounds ) * snapResolution;

	out[ 0 ] = ( idMath::Floor( ( out[ 0 ] / w ) + 0.5f ) * w ) + bounds.worldBounds.GetMins().x;
	out[ 1 ] = ( idMath::Floor( ( out[ 1 ] / h ) + 0.5f ) * h ) + bounds.worldBounds.GetMins().y;

	return out;
}

/*
==============
sdDeployMask::ExpandToGrid
==============
*/
void sdDeployMask::ExpandToGrid( idBounds& bounds, const sdDeployMaskBounds& maskBounds ) const {
	extents_t extents;

	CoordsForBounds( bounds, extents, maskBounds );
	GetBounds( extents, bounds, NULL, maskBounds );
}

/*
==============
sdDeployMask::CoordsForBounds
==============
*/
void sdDeployMask::CoordsForBounds( const idBounds& _bounds, extents_t& extents, const sdDeployMaskBounds& maskBounds ) const {
	sdBounds2D bounds( _bounds );

	float fCoords[ 4 ];
	float boxSize[ 2 ];

	boxSize[ 0 ] = maskBounds.worldSize[ 0 ] / dimensions[ 0 ];
	boxSize[ 1 ] = maskBounds.worldSize[ 1 ] / dimensions[ 1 ];

	fCoords[ 0 ] = ( _bounds[ 0 ].x - maskBounds.worldBounds.GetMins().x ) / boxSize[ 0 ];
	fCoords[ 1 ] = ( _bounds[ 0 ].y - maskBounds.worldBounds.GetMins().y ) / boxSize[ 1 ];
	fCoords[ 2 ] = ( _bounds[ 1 ].x - maskBounds.worldBounds.GetMins().x ) / boxSize[ 0 ];
	fCoords[ 3 ] = ( _bounds[ 1 ].y - maskBounds.worldBounds.GetMins().y ) / boxSize[ 1 ];

	extents.minx	= Max( 0,					idMath::FtoiFast( idMath::Floor( fCoords[ 0 ] ) ) );
	extents.miny	= Max( 0,					idMath::FtoiFast( idMath::Floor( fCoords[ 1 ] ) ) );
	extents.maxx	= Min( dimensions[ 0 ] - 1,	idMath::FtoiFast( idMath::Floor( fCoords[ 2 ] ) ) );
	extents.maxy	= Min( dimensions[ 1 ] - 1,	idMath::FtoiFast( idMath::Floor( fCoords[ 3 ] ) ) );
}

/*
==============
sdDeployMask::GetBounds
==============
*/
void sdDeployMask::GetBounds( const extents_t& extents, idBounds& bounds, const sdHeightMapInstance* heightMap, const sdDeployMaskBounds& maskBounds ) const {
	bounds.GetMins()[ 0 ] = maskBounds.worldBounds.GetMins()[ 0 ] + ( ( GetBoxWidth( maskBounds ) ) * ( extents.minx + 0 ) );
	bounds.GetMaxs()[ 0 ] = maskBounds.worldBounds.GetMins()[ 0 ] + ( ( GetBoxWidth( maskBounds ) ) * ( extents.maxx + 1 ) );

	bounds.GetMins()[ 1 ] = maskBounds.worldBounds.GetMins()[ 1 ] + ( ( GetBoxHeight( maskBounds ) ) * ( extents.miny + 0 ) );
	bounds.GetMaxs()[ 1 ] = maskBounds.worldBounds.GetMins()[ 1 ] + ( ( GetBoxHeight( maskBounds ) ) * ( extents.maxy + 1 ) );

	if ( heightMap != NULL ) {
		idVec2 heights;
		heightMap->GetHeight( bounds, heights );

		bounds.GetMins()[ 2 ] = heights[ 0 ];
		bounds.GetMaxs()[ 2 ] = heights[ 1 ];
	}
}

/*
==============
sdDeployMask::GetState
==============
*/
int sdDeployMask::GetState( int x, int y ) const {
	return data.Get( x + ( y * dimensions[ 0 ] ) );
}

/*
==============
sdDeployMask::SetState
==============
*/
void sdDeployMask::SetState( int x, int y, bool state ) {
	if ( state ) {
		data.Set( x + ( y * dimensions[ 0 ] ) );
	} else {
		data.Clear( x + ( y * dimensions[ 0 ] ) );
	}
}

/*
==============
sdDeployMask::IsValid
==============
*/
deployResult_t sdDeployMask::IsValid( int x, int y ) const {
	if ( x < 0 || y < 0 || x >= dimensions[ 0 ] || y >= dimensions[ 1 ] ) {
		return DR_FAILED;
	}

	return data.Get( x + ( y * dimensions[ 0 ] ) ) ? DR_CLEAR : DR_FAILED;
}

/*
==============
sdDeployMask::IsValid
==============
*/
deployResult_t sdDeployMask::IsValid( const idBounds& bounds, const sdDeployMaskBounds& maskBounds ) const {
	extents_t extents;
	CoordsForBounds( bounds, extents, maskBounds );
	return IsValid( extents );
}

/*
==============
sdDeployMask::IsValid
==============
*/
deployResult_t sdDeployMask::IsValid( const extents_t& extents ) const {
	for ( int x = extents.minx; x <= extents.maxx; x++ ) {
		for ( int y = extents.miny; y <= extents.maxy; y++ ) {
			if ( !data.Get( x + ( y * dimensions[ 0 ] ) ) ) {
				return DR_FAILED;
			}
		}
	}

	return DR_CLEAR;
}
