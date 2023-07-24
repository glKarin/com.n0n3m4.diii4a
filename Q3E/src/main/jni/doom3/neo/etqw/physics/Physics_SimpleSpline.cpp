// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_SimpleSpline.h"

/*
===============================================================

	sdPhysics_SimpleSpline

===============================================================
*/

CLASS_DECLARATION( idPhysics_Base, sdPhysics_SimpleSpline )
END_CLASS

/*
================
sdPhysics_SimpleSpline::sdPhysics_SimpleSpline
================
*/
sdPhysics_SimpleSpline::sdPhysics_SimpleSpline( void ) {
	current.worldAxes.Identity();
	current.worldOrigin.Zero();
	current.acceleration.Zero();
	current.velocity.Zero();
	current.time = 0;

	saved			= current;

	clipModel		= NULL;
	isOrientated	= false;
	startTime		= -1;
	totalTime		= 0;
}

/*
================
sdPhysics_SimpleSpline::~sdPhysics_SimpleSpline
================
*/
sdPhysics_SimpleSpline::~sdPhysics_SimpleSpline( void ) {
}

/*
================
sdPhysics_SimpleSpline::Evaluate
================
*/
idVec3 sdPhysics_SimpleSpline::EvaluatePosition( void ) const {
	if ( !spline.Num() ) {
		return vec3_origin;
	}

	float frac;
	const splineType_t& section = GetSplineInfo( gameLocal.time, frac );

	return section.GetCurrentValue( frac );
}

/*
================
sdPhysics_SimpleSpline::Evaluate
================
*/
bool sdPhysics_SimpleSpline::Evaluate( int timeStepMSec, int endTimeMSec ) {
	if ( !spline.Num() ) {
		return false;
	}

/*	int i;
	for ( i = 0; i < spline.Num(); i++ ) {
		const idCurve_CubicBezier< idVec3 >& section = spline[ i ];
		float index = 0.f;
		idVec3 lastPos = section.GetCurrentValue( index );
		idVec3 pos;
		for ( index += 0.01f; index <= 1.f; index += 0.01f ) {
			pos = section.GetCurrentValue( index );
			gameRenderWorld->DebugLine( colorGreen, lastPos, pos );
			lastPos = pos;
		}
	}*/


	idVec3 oldOrigin		= current.worldOrigin;
	idMat3 oldWorldAxes		= current.worldAxes;

	float frac;
	const splineType_t& section = GetSplineInfo( endTimeMSec, frac );

	current.acceleration	= section.GetCurrentSecondDerivative( frac );
	current.velocity		= section.GetCurrentFirstDerivative( frac );
	current.worldOrigin		= section.GetCurrentValue( frac );

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.worldOrigin, current.worldAxes );
	}

	current.time = endTimeMSec;

	return ( current.worldOrigin != oldOrigin ) || ( current.worldAxes != oldWorldAxes );
}

/*
================
sdPhysics_SimpleSpline::SetSpline
================
*/
void sdPhysics_SimpleSpline::SetSpline( int _startTime, const idList< splineType_t >& _spline, const idList< int >& _splineTimes ) {
	startTime	= _startTime;
	splineTimes	= _splineTimes;
	spline		= _spline;
	totalTime	= 0;

	int i;
	for ( i = 0; i < _splineTimes.Num(); i++ ) {
		totalTime += _splineTimes[ i ];
	}

	assert( splineTimes.Num() == spline.Num() );
}

/*
================
sdPhysics_SimpleSpline::GetSplineInfo
================
*/
const sdPhysics_SimpleSpline::splineType_t& sdPhysics_SimpleSpline::GetSplineInfo( int endTimeMSec, float& fraction ) const {
	int timeSinceStart = endTimeMSec - startTime;
	int i;
	for ( i = 0; i < spline.Num(); i++ ) {
		if ( timeSinceStart > splineTimes[ i ] ) {
			timeSinceStart -= splineTimes[ i ];
			continue;
		}

		fraction = idMath::ClampFloat( 0.f, 1.f, timeSinceStart / ( float )splineTimes[ i ] );
		return spline[ i ];
	}

	fraction = 1.f;
	return spline[ spline.Num() - 1 ];
}

/*
================
sdPhysics_SimpleSpline::SetAxis
================
*/
void sdPhysics_SimpleSpline::SetAxis( const idMat3& newAxis, int id ) {
	current.worldAxes = newAxis;

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.worldOrigin, current.worldAxes );
	}
}

/*
================
sdPhysics_SimpleSpline::SetOrigin
================
*/
void sdPhysics_SimpleSpline::SetOrigin( const idVec3& newOrigin, int id ) {
	current.worldOrigin = newOrigin;

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.worldOrigin, current.worldAxes );
	}
}

/*
================
sdPhysics_SimpleSpline::IsAtRest
================
*/
bool sdPhysics_SimpleSpline::IsAtRest( void ) const {
	return current.time >= ( startTime + totalTime );
}

/*
================
sdPhysics_SimpleSpline::GetAbsBounds
================
*/
const idBounds& sdPhysics_SimpleSpline::GetAbsBounds( int id ) const {
	current.absBounds.Clear();
	current.absBounds.AddPoint( current.mins + current.worldOrigin );
	current.absBounds.AddPoint( current.maxs + current.worldOrigin );

	return current.absBounds;
}

/*
================
sdPhysics_SimpleSpline::SetBounds
================
*/
void sdPhysics_SimpleSpline::SetBounds( const idVec3& mins, const idVec3& maxs ) {
	current.mins = mins;
	current.maxs = maxs;
}