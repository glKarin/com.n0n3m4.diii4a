// Copyright (C) 2007 Id Software, Inc.
//




#include "../../idlib/precompiled.h"
#pragma hdrstop
#include "AAS_local.h"
#include "AASCallback_AvoidLocation.h"

/*
============
idAASCallback_AvoidLocation::idAASCallback_AvoidLocation
============
*/
idAASCallback_AvoidLocation::idAASCallback_AvoidLocation()
{
	avoidLocation.Zero();
	avoidDist = 0.0f;
	obstacles = NULL;
	numObstacles = 0;
}

/*
============
idAASCallback_AvoidLocation::~idAASCallback_AvoidLocation
============
*/
idAASCallback_AvoidLocation::~idAASCallback_AvoidLocation()
{
}

/*
============
idAASCallback_AvoidLocation::SetAvoidLocation
============
*/
void idAASCallback_AvoidLocation::SetAvoidLocation( const idVec3& start, const idVec3& avoidLocation )
{
	this->avoidLocation = avoidLocation;
	this->avoidDist = ( avoidLocation - start ).Length();
}

/*
============
idAASCallback_AvoidLocation::SetObstacles
============
*/
void idAASCallback_AvoidLocation::SetObstacles( const idAAS* aas, const idAASObstacle* obstacles, int numObstacles )
{
	this->obstacles = obstacles;
	this->numObstacles = numObstacles;

	for( int i = 0; i < numObstacles; i++ )
	{
		obstacles[i].expAbsBounds[0] = obstacles[i].absBounds[0] - aas->GetSettings()->boundingBoxes[0][1];
		obstacles[i].expAbsBounds[1] = obstacles[i].absBounds[1] - aas->GetSettings()->boundingBoxes[0][0];
	}
}

/*
============
idAASCallback_AvoidLocation::PathValid
============
*/
bool idAASCallback_AvoidLocation::PathValid( const idAAS* aas, const idVec3& start, const idVec3& end )
{
	// path may not go through any obstacles
	for( int i = 0; i < numObstacles; i++ )
	{
		// if the movement vector intersects the expanded obstacle bounds
		if( obstacles[i].expAbsBounds.LineIntersection( start, end ) )
		{
			return false;
		}
	}
	return true;
}

/*
============
idAASCallback_AvoidLocation::AdditionalTravelTimeForPath
============
*/
int idAASCallback_AvoidLocation::AdditionalTravelTimeForPath( const idAAS* aas, const idVec3& start, const idVec3& end )
{
	if( avoidDist <= 0.0f )
	{
		return 0;
	}

	// project avoidLocation origin onto movement vector
	idVec3 v1 = end - start;
	v1.Normalize();
	idVec3 v2 = avoidLocation - start;
	idVec3 p = start + ( v2 * v1 ) * v1;

	// get the point on the path closest to the avoidLocation
	int i;
	for( i = 0; i < 3; i++ )
	{
		if( ( p[i] > start[i] + 0.1f && p[i] > end[i] + 0.1f ) ||
				( p[i] < start[i] - 0.1f && p[i] < end[i] - 0.1f ) )
		{
			break;
		}
	}

	float dist;
	if( i >= 3 )
	{
		dist = ( avoidLocation - p ).Length();
	}
	else
	{
		dist = ( avoidLocation - end ).Length();
	}

	// avoid moving closer to the avoidLocation
	if( dist < avoidDist )
	{
		return idMath::Ftoi( ( avoidDist - dist ) * 10.0f );
	}

	return 0;
}
