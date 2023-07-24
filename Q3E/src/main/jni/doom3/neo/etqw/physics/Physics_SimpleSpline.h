// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __PHYSICS_SIMPLESPLINE_H__
#define __PHYSICS_SIMPLESPLINE_H__

#include "Physics_Base.h"

class sdPhysics_SimpleSpline : public idPhysics_Base {
public:
	typedef idCurve_CubicBezier< idVec3 >	splineType_t;

public:
	CLASS_PROTOTYPE( sdPhysics_SimpleSpline );

										sdPhysics_SimpleSpline( void );
										~sdPhysics_SimpleSpline( void );

	void								SetSpline( int _startTime, const idList< splineType_t >& _spline, const idList< int >& _splineTimes );
	const splineType_t&					GetSplineInfo( int endTimeMSec, float& fraction ) const;
	bool								HasSpline( void ) const { return spline.Num() > 0; }

	virtual bool						Evaluate( int timeStepMSec, int endTimeMSec );
	virtual idVec3						EvaluatePosition( void ) const;

	virtual const idVec3&				GetLinearVelocity( int id ) const { return current.velocity; }
	const idVec3&						GetLinearAcceleration( void ) const { return current.acceleration; }

	virtual const idVec3&				GetOrigin( int id ) const { return current.worldOrigin; }
	virtual const idMat3&				GetAxis( int id ) const { return current.worldAxes; }
	virtual const idBounds&				GetAbsBounds( int id = -1 ) const;

	virtual void						SetAxis( const idMat3& newAxis, int id );
	virtual void						SetOrigin( const idVec3& newOrigin, int id );
	virtual void						SetBounds( const idVec3& mins, const idVec3& maxs );

	virtual bool						IsAtRest( void ) const;

	virtual bool						AllowInhibit( void ) const { return false; }

private:
	typedef struct simpleSplinePState_s {
		int								time;
		idVec3							worldOrigin;
		idMat3							worldAxes;

		mutable idBounds				absBounds;
		mutable idVec3					mins;
		mutable idVec3					maxs;

		//
		idVec3							velocity;
		idVec3							acceleration;
	} simpleSplinePState_t;

	simpleSplinePState_t				current;
	simpleSplinePState_t				saved;
	
	idClipModel*						clipModel;

	bool								isOrientated;

	int									totalTime;
	int									startTime;
	idList< splineType_t >				spline;
	idList< int >						splineTimes;
};

#endif // __PHYSICS_SIMPLESPLINE_H__
