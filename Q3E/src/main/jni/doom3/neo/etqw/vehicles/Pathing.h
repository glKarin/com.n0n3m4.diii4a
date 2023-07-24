// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_VEHICLES_PATHING_H__
#define __GAME_VEHICLES_PATHING_H__

#include "../Entity.h"
#include "../ScriptEntity.h"

typedef idCurve_CubicBezier< idVec3 > splineSection_t;

class sdVehiclePathGrid {
public:
									sdVehiclePathGrid( const sdDeclVehiclePath* path, const idBounds& bounds );
									~sdVehiclePathGrid( void );

	const char*						GetName( void ) const { return _path->GetName(); }
	void							GetPoint( int x, int y, idVec3& point ) const;
	void							GetEdgePoint( int x, int y, int& nx, int& ny, int seed, float cornerX, float cornerY ) const;
	float							GetCoordsForPoint( int& x, int& y, int& xmin, int& ymin, const idVec3& pos ) const;

	void							AdjustTargetForStart( const idVec3& start, const idVec3& target, int& _x, int& _y, const int& xmin, const int& ymin ) const;

	void							SetupPath( const idVec3& position, idList< splineSection_t >& spline, idList< splineSection_t >& outSpline, int seed ) const;
	void							AddSection( idVec3& lastPos, idVec3& inVector, const idVec3& newPos, const idVec3& finalPos, bool canSkip, idList< splineSection_t >& spline ) const;

	void							SetupPathPoints( const idVec3& position, idStaticList< idVec3, MAX_SCRIPTENTITY_PATHPOINTS >& pathPoints, int seed, float cornerX, float cornerY ) const;

	int								GetSize( void ) const { return _path->GetSize(); }

	void							DebugDraw( void ) const;

private:
	void							GetPointInternal( int x, int y, idVec3& point ) const;

	const sdDeclVehiclePath*		_path;
	idBounds						_bounds;
	idVec3							_size;
};

#endif // __GAME_VEHICLES_PATHING_H__
