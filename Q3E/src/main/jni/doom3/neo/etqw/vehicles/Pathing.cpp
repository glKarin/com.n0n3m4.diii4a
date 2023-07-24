// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Pathing.h"

/*
===============================================================================

	sdVehiclePathGrid

===============================================================================
*/

/*
================
sdVehiclePathGrid::sdVehiclePathGrid
================
*/
sdVehiclePathGrid::sdVehiclePathGrid( const sdDeclVehiclePath* path, const idBounds& bounds ) {
	_path = path;
	_bounds = bounds;
	_size = _bounds.Size();
	_size *= 1.f / ( GetSize() + 1 );
}

/*
================
sdVehiclePathGrid::~sdVehiclePathGrid
================
*/
sdVehiclePathGrid::~sdVehiclePathGrid( void ) {
}

/*
================
sdVehiclePathGrid::GetPoint
================
*/
void sdVehiclePathGrid::GetPoint( int x, int y, idVec3& point ) const {
	if ( x < 0 || x > GetSize() || y < 0 || y > GetSize() ) {
		point = vec3_origin;
		return;
	}

	GetPointInternal( x, y, point );
}

/*
================
sdVehiclePathGrid::GetEdgePoint
================
*/
void sdVehiclePathGrid::GetEdgePoint( int x, int y, int& nx, int& ny, int seed, float cornerX, float cornerY ) const {
	nx = x;
	ny = y;

	idRandom random( seed );

	cornerX = cornerX > 0.0f ? 1.0f : cornerX;
	cornerX = cornerX < 0.0f ? -1.0f : cornerX;
	cornerY = cornerY > 0.0f ? 1.0f : cornerY;
	cornerY = cornerY < 0.0f ? -1.0f : cornerY;

	if ( cornerX == 0.0f && cornerY == 0.0f ) {
		bool xleft	= x > ( GetSize() / 2 );
		bool ytop	= y > ( GetSize() / 2 );

		if ( random.RandomFloat() > 0.5f ) {
			nx = xleft ? 0 : GetSize() - 1;
			ny += random.CRandomFloat() * 10;
			ny = idMath::ClampInt( 0, GetSize() - 1, ny );
		} else {
			ny = ytop ? 0 : GetSize() - 1;
			nx += random.CRandomFloat() * 10;
			nx = idMath::ClampInt( 0, GetSize() - 1, nx );
		}
	} else {
		int minX, maxX, minY, maxY;

  		if ( cornerX == -1.0f ) {
			minX = 0;
			maxX = ( int )( GetSize() * 0.4f );
		} else {
			minX = ( int )( GetSize() * 0.6f );
			maxX = GetSize() - 1;
		}
		if ( cornerY == 1.0f ) {
			minY = 0;
			maxY = ( int )( GetSize() * 0.4f );
		} else {
			minY = ( int )( GetSize() * 0.6f );
			maxY = GetSize() - 1;
		}

		if ( random.RandomFloat() > 0.5f ) {
			nx = random.RandomInt( maxX - minX ) + minX;
			ny = cornerY == 1.0f ? minY : maxY;
		} else {
			nx = cornerX == -1.0f ? minX : maxX;
			ny = random.RandomInt( maxY - minY ) + minY;
		}
	}
}

/*
================
sdVehiclePathGrid::GetPointInternal
================
*/
void sdVehiclePathGrid::GetPointInternal( int x, int y, idVec3& point ) const {
	point[ 0 ] = ( ( x + 0.5f ) * _size[ 0 ] ) + _bounds.GetMins()[ 0 ];
	point[ 1 ] = _bounds.GetMaxs()[ 1 ] - ( ( y + 0.5f ) * _size[ 1 ] );
	point[ 2 ] = _path->GetPointHeight( x, y );
}

/*
================
sdVehiclePathGrid::DebugDraw
================
*/
void sdVehiclePathGrid::DebugDraw( void ) const {
	if ( g_showVehiclePathNodes.GetInteger() & 1 ) {
		int x;
		for ( x = 0; x < GetSize() - 1; x++ ) {
			int y;
			for ( y = 0; y < GetSize(); y++ ) {
				int xn = x + 1;

				idVec3 start;
				idVec3 end;
				GetPointInternal( x, y, start );
				GetPointInternal( xn, y, end );
				gameRenderWorld->DebugLine( colorRed, start, end );
			}
		}

		for ( x = 0; x < GetSize(); x++ ) {
			int y;
			for ( y = 0; y < GetSize() - 1; y++ ) {
				int yn = y + 1;

				idVec3 start;
				idVec3 end;
				GetPointInternal( x, y, start );
				GetPointInternal( x, yn, end );
				gameRenderWorld->DebugLine( colorRed, start, end );
			}
		}
	}
}

/*
================
sdVehiclePathGrid::GetCoordsForPoint
================
*/
float sdVehiclePathGrid::GetCoordsForPoint( int& _x, int& _y, int& xmin, int& ymin, const idVec3& pos ) const {
	float bestLen = -1.f;
	float averageHeight = 0.f;

	_x = 0;
	_y = 0;

	xmin = 0;
	ymin = 0;

	// FIXME: Gordon: These two loops can be optimized
	int x;
	for ( x = 0; x < GetSize() - 1; x++ ) {
		idVec3 point;
		GetPointInternal( x, 0, point );
		idVec3 point2;
		GetPointInternal( x + 1, 0, point2 );

		if ( pos[ 0 ] >= point[ 0 ] && pos[ 0 ] <= point2[ 0 ] ) {
			xmin = x;
			break;
		}
	}

	int y;
	for ( y = GetSize() - 1; y > 0; y-- ) {
		idVec3 point;
		GetPointInternal( 0, y, point );
		idVec3 point2;
		GetPointInternal( 0, y - 1, point2 );

		if ( pos[ 1 ] >= point[ 1 ] && pos[ 1 ] <= point2[ 1 ] ) {
			ymin = y - 1;
			break;
		}
	}

	for ( x = xmin; x <= xmin + 1; x++ ) {
		for ( y = ymin; y <= ymin + 1; y++ ) {
			idVec3 point;
			GetPointInternal( x, y, point );

			averageHeight += point[ 2 ];

			idVec3 dist = point - pos;
			dist[ 2 ] = 0.f;

			float len = dist.LengthSqr();
			if ( bestLen == -1 || len < bestLen ) {
				bestLen = len;
				_x = x;
				_y = y;
			}
		}
	}

	return averageHeight * 0.25f;
}

/*
================
sdVehiclePathGrid::AdjustTargetForStart
================
*/
void sdVehiclePathGrid::AdjustTargetForStart( const idVec3& start, const idVec3& target, int& _x, int& _y, const int& xmin, const int& ymin ) const {
	float bestLen = -1.f;

	idVec3 dist = target - start;
	dist[ 2 ] = 0.f;
	dist.Normalize();

	int x, y;
	for ( x = xmin; x <= xmin + 1; x++ ) {
		for ( y = ymin; y <= ymin + 1; y++ ) {
			idVec3 pos;
			GetPointInternal( x, y, pos );

//			gameRenderWorld->DebugArrow( colorBlue, pos, start, 16, 100000 );

			idVec3 temp = pos - start;
			temp[ 2 ] = 0.f;
			temp = ( temp * dist ) * dist;
			float len = temp.LengthSqr();

			if ( bestLen == -1.f || len < bestLen ) {
				bestLen = len;
				_x = x;
				_y = y;
			}
		}
	}
}

/*
================
sdVehiclePathGrid::AddSection
================
*/
void sdVehiclePathGrid::AddSection( idVec3& lastPos, idVec3& inVector, const idVec3& newPos, const idVec3& finalPos, bool canSkip, idList< splineSection_t >& spline ) const {
	idVec3 dist = newPos - lastPos;
	idAngles pathAngles = dist.ToAngles();

	float pitch = idMath::AngleNormalize180( pathAngles.pitch );

	if ( fabs( pitch ) > 30 ) {

		idVec3 newInVector;
		splineSection_t& section1 = spline.Alloc();

		idVec3 verticalPos;

		if ( pitch < -30 ) {
			verticalPos[ 0 ] = lastPos[ 0 ] + ( ( newPos[ 0 ] - lastPos[ 0 ] ) * 0.3f );
			verticalPos[ 1 ] = lastPos[ 1 ] + ( ( newPos[ 1 ] - lastPos[ 1 ] ) * 0.3f );
			verticalPos[ 2 ] = lastPos[ 2 ] + ( ( newPos[ 2 ] - lastPos[ 2 ] ) * 0.9f );
		} else if ( pitch > 30 ) {
			verticalPos[ 0 ] = newPos[ 0 ] + ( ( lastPos[ 0 ] - newPos[ 0 ] ) * 0.3f );
			verticalPos[ 1 ] = newPos[ 1 ] + ( ( lastPos[ 1 ] - newPos[ 1 ] ) * 0.3f );
			verticalPos[ 2 ] = newPos[ 2 ] + ( ( lastPos[ 2 ] - newPos[ 2 ] ) * 0.9f );
		}

		newInVector = ( verticalPos - lastPos );
		float scale = inVector.Normalize();

		section1.AddValue( 0.f, lastPos );
		section1.AddValue( 1.f / 3.f, lastPos + ( inVector * ( scale * 0.5f ) ) );
		section1.AddValue( 2.f / 3.f, verticalPos - ( newInVector * ( scale * 0.5f ) ) );
		section1.AddValue( 1.f, verticalPos );



		splineSection_t& section2 = spline.Alloc();

		newInVector = ( newPos - verticalPos );
		scale = newInVector.Normalize();

		section2.AddValue( 0.f, verticalPos );
		section2.AddValue( 1.f / 3.f, verticalPos + ( inVector * ( scale * 0.5f ) ) );
		section2.AddValue( 2.f / 3.f, newPos - ( newInVector * ( scale * 0.5f ) ) );
		section2.AddValue( 1.f, newPos );

		lastPos		= newPos;
		inVector	= newInVector;

		return;

	}

	if ( canSkip ) {
		idVec3 dist;

		dist = newPos - lastPos;
		dist[ 2 ] = 0;

		idAngles angles1 = dist.ToAngles();

		dist = finalPos - lastPos;
		dist[ 2 ] = 0;

		idAngles angles2 = dist.ToAngles();

		float yawdiff = idMath::AngleDelta( angles1.yaw, angles2.yaw );
		if ( fabs( yawdiff ) > 10.f ) {
			return;
		}
	}

	idVec3 newInVector = ( newPos - lastPos );
	float scale = newInVector.Normalize();

	splineSection_t& section = spline.Alloc();

	section.AddValue( 0.f, lastPos );
	section.AddValue( 1.f / 3.f, lastPos + ( inVector * ( scale * 0.5f ) ) );
	section.AddValue( 2.f / 3.f, newPos - ( newInVector * ( scale * 0.5f ) ) );
	section.AddValue( 1.f, newPos );

	lastPos		= newPos;
	inVector	= newInVector;
}

/*
================
sdVehiclePathGrid::SetupPath
================
*/
void sdVehiclePathGrid::SetupPath( const idVec3& position, idList< splineSection_t >& inSpline, idList< splineSection_t >& outSpline, int seed ) const {
	int x, y;
	int xmin, ymin;
	GetCoordsForPoint( x, y, xmin, ymin, position );

	int nx, ny;
	GetEdgePoint( x, y, nx, ny, seed, 0, 0 );

	idVec3 edgePos;
	GetPointInternal( nx, ny, edgePos );

	AdjustTargetForStart( edgePos, position, x, y, xmin, ymin );

	bool ymainaxis = abs( ny - y ) > abs( nx - x );

	int loopPos;
	int loopDir;
	int loopStart;
	int loopEnd;
	float loopLen;
	
	if ( ymainaxis ) {
		loopPos		= ny;
		loopStart	= ny;
		loopDir		= ny > y ? -1 : 1;
		loopLen		= ny - y;
		loopEnd		= y;
	} else {
		loopPos		= nx;
		loopStart	= nx;
		loopDir		= nx > x ? -1 : 1;
		loopLen		= nx - x;
		loopEnd		= x;
	}

	idVec3 endPoint;
	GetPoint( x, y, endPoint );

	idVec3 lastPos;
	GetPoint( nx, ny, lastPos );

	idVec3 inVector = ( endPoint - lastPos );
	inVector[ 2 ] = 0.f;
	inVector.Normalize();
	float scale = 64.f; // inVector.Normalize();

	inSpline.Clear();

	while ( loopPos != loopEnd ) {
		int currentPos[ 2 ];

		loopPos += loopDir;
		float frac = ( loopPos - loopStart ) / loopLen;

		if ( ymainaxis ) {
			currentPos[ 0 ] = nx + ( ( nx - x ) * frac );
			currentPos[ 1 ] = loopPos;
		} else {
			currentPos[ 0 ] = loopPos;
			currentPos[ 1 ] = ny + ( ( ny - y ) * frac );
		}

		idVec3 newPos;
		GetPointInternal( currentPos[ 0 ], currentPos[ 1 ], newPos );

		AddSection( lastPos, inVector, newPos, endPoint, loopPos != loopEnd, inSpline );
	}

	{
		idVec3 hoverPos = position;
		hoverPos[ 2 ] = lastPos[ 2 ];

		idVec3 temp = ( hoverPos - lastPos );
		scale = temp.Normalize() * 0.125f;

		idVec3 newInVector( 0.f, 0.f, -1.f );
		newInVector += temp * 0.2f;
		newInVector.Normalize();

		splineSection_t& section = inSpline.Alloc();

		section.AddValue( 0.f, lastPos );
		section.AddValue( 1.f / 3.f, lastPos + ( inVector * ( scale ) ) );
		section.AddValue( 2.f / 3.f, hoverPos - ( newInVector * ( scale ) ) );
		section.AddValue( 1.f, hoverPos );

		lastPos		= hoverPos;
		inVector	= newInVector;
	}

	{
		GetCoordsForPoint( nx, ny, xmin, ymin, lastPos );

		idVec3 startPoint;
		GetPoint( nx, ny, startPoint );

		idVec3 temp = startPoint - lastPos;
		scale = temp.Normalize() * 0.125f;

		inVector[ 2 ] = 0.f;
		inVector.Normalize();
		inVector *= 0.2f;
		inVector += idVec3( 0.f, 0.f, 1.f );
		inVector.Normalize();

		GetEdgePoint( nx, ny, x, y, seed, 0, 0 );

		GetPoint( x, y, endPoint );
	}

	ymainaxis = abs( ny - y ) > abs( nx - x );

	if ( ymainaxis ) {
		loopPos		= ny;
		loopStart	= ny;
		loopDir		= ny > y ? -1 : 1;
		loopLen		= ny - y;
		loopEnd		= y;
	} else {
		loopPos		= nx;
		loopStart	= nx;
		loopDir		= nx > x ? -1 : 1;
		loopLen		= nx - x;
		loopEnd		= x;
	}

	outSpline.Clear();

	while ( loopPos != loopEnd ) {
		int currentPos[ 2 ];

		loopPos += loopDir;
		float frac = ( loopPos - loopStart ) / loopLen;

		if ( ymainaxis ) {
			currentPos[ 0 ] = nx + ( ( nx - x ) * frac );
			currentPos[ 1 ] = loopPos;
		} else {
			currentPos[ 0 ] = loopPos;
			currentPos[ 1 ] = ny + ( ( ny - y ) * frac );
		}

		idVec3 newPos;
		GetPointInternal( currentPos[ 0 ], currentPos[ 1 ], newPos );

		AddSection( lastPos, inVector, newPos, endPoint, loopPos != loopEnd, outSpline );
	}
}

/*
================
sdVehiclePathGrid::SetupPathPoints
================
*/
void sdVehiclePathGrid::SetupPathPoints( const idVec3& position, idStaticList< idVec3, MAX_SCRIPTENTITY_PATHPOINTS >& pathPoints, int seed, float cornerX, float cornerY ) const {
	int x, y;
	int xmin, ymin;
	GetCoordsForPoint( x, y, xmin, ymin, position );

	int nx, ny;
	GetEdgePoint( x, y, nx, ny, seed, cornerX, cornerY );

	idVec3 edgePos;
	GetPointInternal( nx, ny, edgePos );

	AdjustTargetForStart( edgePos, position, x, y, xmin, ymin );

	bool ymainaxis = abs( ny - y ) > abs( nx - x );

	int loopPos;
	int loopDir;
	int loopStart;
	int loopEnd;
	float loopLen;
	
	if ( ymainaxis ) {
		loopPos		= ny;
		loopStart	= ny;
		loopDir		= ny > y ? -1 : 1;
		loopLen		= ny - y;
		loopEnd		= y;
	} else {
		loopPos		= nx;
		loopStart	= nx;
		loopDir		= nx > x ? -1 : 1;
		loopLen		= nx - x;
		loopEnd		= x;
	}

	idVec3 endPoint;
	GetPoint( x, y, endPoint );

	idVec3 lastPos;
	GetPoint( nx, ny, lastPos );

	idVec3 inVector = ( endPoint - lastPos );
	inVector[ 2 ] = 0.f;
	inVector.Normalize();
	float scale = 64.f; // inVector.Normalize();

	pathPoints.Clear();

	while ( loopPos != loopEnd ) {
		int currentPos[ 2 ];

		loopPos += loopDir;
		float frac = ( loopPos - loopStart ) / loopLen;

		if ( ymainaxis ) {
			currentPos[ 0 ] = nx + ( ( nx - x ) * frac );
			currentPos[ 1 ] = loopPos;
		} else {
			currentPos[ 0 ] = loopPos;
			currentPos[ 1 ] = ny + ( ( ny - y ) * frac );
		}

		idVec3 newPos;
		GetPointInternal( currentPos[ 0 ], currentPos[ 1 ], newPos );

		//AddSection( lastPos, inVector, newPos, endPoint, loopPos != loopEnd, inSpline );
		pathPoints.Append( newPos );
		lastPos = newPos;
	}

	// add the final point
	idVec3 hoverPos = position;
	hoverPos[ 2 ] = lastPos[ 2 ];
	pathPoints.Append( hoverPos );
}
