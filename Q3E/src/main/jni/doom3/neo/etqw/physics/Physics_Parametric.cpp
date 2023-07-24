// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Parametric.h"
#include "../Entity.h"

CLASS_DECLARATION( idPhysics_Base, idPhysics_Parametric )
END_CLASS

#pragma warning ( disable : 4244 )

/*
================
idPhysics_Parametric::Activate
================
*/
void idPhysics_Parametric::Activate( void ) {
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
idPhysics_Parametric::TestIfAtRest
================
*/
bool idPhysics_Parametric::TestIfAtRest( void ) const {

	if ( ( current.linearExtrapolation.GetExtrapolationType() & ~EXTRAPOLATION_NOSTOP ) == EXTRAPOLATION_NONE &&
			( current.angularExtrapolation.GetExtrapolationType() & ~EXTRAPOLATION_NOSTOP ) == EXTRAPOLATION_NONE &&
				current.linearInterpolation.GetDuration() == 0 &&
					current.angularInterpolation.GetDuration() == 0 &&
						current.spline == NULL ) {
		return true;
	}

	if ( !current.linearExtrapolation.IsDone( static_cast< float >( current.time ) ) ) {
		return false;
	}

	if ( !current.angularExtrapolation.IsDone( static_cast< float >( current.time ) ) ) {
		return false;
	}

	if ( !current.linearInterpolation.IsDone( static_cast< float >( current.time ) ) ) {
		return false;
	}

	if ( !current.angularInterpolation.IsDone( static_cast< float >( current.time ) ) ) {
		return false;
	}

	if ( current.spline != NULL && !current.spline->IsDone( static_cast< float >( current.time ) ) ) {
		return false;
	}

	return true;
}

/*
================
idPhysics_Parametric::Rest
================
*/
void idPhysics_Parametric::Rest( void ) {
	current.atRest = gameLocal.time;
//	self->BecomeInactive( TH_PHYSICS );
}

/*
================
idPhysics_Parametric::idPhysics_Parametric
================
*/
idPhysics_Parametric::idPhysics_Parametric( void ) {

	current.time = gameLocal.time;
	current.atRest = -1;
	current.useSplineAngles = false;
	current.origin.Zero();
	current.angles.Zero();
	current.axis.Identity();
	current.localOrigin.Zero();
	current.localAngles.Zero();
	current.linearExtrapolation.Init( 0, 0, vec3_zero, vec3_zero, vec3_zero, EXTRAPOLATION_NONE );
	current.angularExtrapolation.Init( 0, 0, ang_zero, ang_zero, ang_zero, EXTRAPOLATION_NONE );
	current.linearInterpolation.Init( 0, 0, 0, 0, vec3_zero, vec3_zero );
	current.angularInterpolation.Init( 0, 0, 0, 0, ang_zero, ang_zero );
	current.spline = NULL;
	current.splineInterpolate.Init( 0, 1, 1, 2, 0, 0 );

	saved = current;

	isPusher = false;
	pushFlags = 0;
	clipModel = NULL;
	isBlocked = false;
	memset( &pushResults, 0, sizeof( pushResults ) );

	hasMaster = false;
	isOrientated = false;
}

/*
================
idPhysics_Parametric::~idPhysics_Parametric
================
*/
idPhysics_Parametric::~idPhysics_Parametric( void ) {
	if ( clipModel != NULL ) {
		gameLocal.clip.DeleteClipModel( clipModel );
		clipModel = NULL;
	}
	if ( current.spline != NULL ) {
		delete current.spline;
		current.spline = NULL;
	}
}

/*
================
idPhysics_Parametric::SetPusher
================
*/
void idPhysics_Parametric::SetPusher( int flags ) {
	assert( clipModel );
	isPusher = true;
	pushFlags = flags;
}

/*
================
idPhysics_Parametric::IsPusher
================
*/
bool idPhysics_Parametric::IsPusher( void ) const {
	return isPusher;
}

/*
================
idPhysics_Parametric::SetLinearExtrapolation
================
*/
void idPhysics_Parametric::SetLinearExtrapolation( extrapolation_t type, int time, int duration, const idVec3 &base, const idVec3 &speed, const idVec3 &baseSpeed ) {
	current.time = gameLocal.time;
	current.linearExtrapolation.Init( time, duration, base, baseSpeed, speed, type );
	current.localOrigin = base;
	Activate();
}

/*
================
idPhysics_Parametric::SetAngularExtrapolation
================
*/
void idPhysics_Parametric::SetAngularExtrapolation( extrapolation_t type, int time, int duration, const idAngles &base, const idAngles &speed, const idAngles &baseSpeed ) {
	current.time = gameLocal.time;
	current.angularExtrapolation.Init( time, duration, base, baseSpeed, speed, type );
	current.localAngles = base;
	Activate();
}

/*
================
idPhysics_Parametric::GetLinearExtrapolationType
================
*/
extrapolation_t idPhysics_Parametric::GetLinearExtrapolationType( void ) const {
	return current.linearExtrapolation.GetExtrapolationType();
}

/*
================
idPhysics_Parametric::GetAngularExtrapolationType
================
*/
extrapolation_t idPhysics_Parametric::GetAngularExtrapolationType( void ) const {
	return current.angularExtrapolation.GetExtrapolationType();
}

/*
================
idPhysics_Parametric::SetLinearInterpolation
================
*/
void idPhysics_Parametric::SetLinearInterpolation( int time, int accelTime, int decelTime, int duration, const idVec3 &startPos, const idVec3 &endPos ) {
	current.time = gameLocal.time;
	current.linearInterpolation.Init( time, accelTime, decelTime, duration, startPos, endPos );
	current.localOrigin = startPos;
	Activate();
}

/*
================
idPhysics_Parametric::SetAngularInterpolation
================
*/
void idPhysics_Parametric::SetAngularInterpolation( int time, int accelTime, int decelTime, int duration, const idAngles &startAng, const idAngles &endAng ) {
	current.time = gameLocal.time;
	current.angularInterpolation.Init( time, accelTime, decelTime, duration, startAng, endAng );
	current.localAngles = startAng;
	Activate();
}

/*
================
idPhysics_Parametric::SetSpline
================
*/
void idPhysics_Parametric::SetSpline( idCurve_Spline<idVec3> *spline, int accelTime, int decelTime, bool useSplineAngles ) {
	if ( current.spline != NULL ) {
		delete current.spline;
		current.spline = NULL;
	}
	current.spline = spline;
	if ( current.spline != NULL ) {
		float startTime = current.spline->GetTime( 0 );
		float endTime = current.spline->GetTime( current.spline->GetNumValues() - 1 );
		float length = current.spline->GetLengthForTime( endTime );
		current.splineInterpolate.Init( startTime, accelTime, decelTime, endTime - startTime, 0.0f, length );
	}
	current.useSplineAngles = useSplineAngles;
	Activate();
}

/*
================
idPhysics_Parametric::GetSpline
================
*/
idCurve_Spline<idVec3> *idPhysics_Parametric::GetSpline( void ) const {
	return current.spline;
}

/*
================
idPhysics_Parametric::GetSplineAcceleration
================
*/
int idPhysics_Parametric::GetSplineAcceleration( void ) const {
	return current.splineInterpolate.GetAcceleration();
}

/*
================
idPhysics_Parametric::GetSplineDeceleration
================
*/
int idPhysics_Parametric::GetSplineDeceleration( void ) const {
	return current.splineInterpolate.GetDeceleration();
}

/*
================
idPhysics_Parametric::UsingSplineAngles
================
*/
bool idPhysics_Parametric::UsingSplineAngles( void ) const {
	return current.useSplineAngles;
}

/*
================
idPhysics_Parametric::GetLocalOrigin
================
*/
void idPhysics_Parametric::GetLocalOrigin( idVec3 &curOrigin ) const {
	curOrigin = current.localOrigin;
}

/*
================
idPhysics_Parametric::GetLocalAngles
================
*/
void idPhysics_Parametric::GetLocalAngles( idAngles &curAngles ) const {
	curAngles = current.localAngles;
}

/*
================
idPhysics_Parametric::SetClipModel
================
*/
void idPhysics_Parametric::SetClipModel( idClipModel *model, float density, int id, bool freeOld ) {

	assert( self );
	assert( model );

	if ( clipModel != NULL && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;
	clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
}

/*
================
idPhysics_Parametric::GetClipModel
================
*/
idClipModel *idPhysics_Parametric::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
idPhysics_Parametric::GetNumClipModels
================
*/
int idPhysics_Parametric::GetNumClipModels( void ) const {
	return ( clipModel != NULL );
}

/*
================
idPhysics_Parametric::SetMass
================
*/
void idPhysics_Parametric::SetMass( float mass, int id ) {
}

/*
================
idPhysics_Parametric::GetMass
================
*/
float idPhysics_Parametric::GetMass( int id ) const {
	return 0.0f;
}

/*
================
idPhysics_Parametric::SetClipMask
================
*/
void idPhysics_Parametric::SetContents( int contents, int id ) {
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
idPhysics_Parametric::SetClipMask
================
*/
int idPhysics_Parametric::GetContents( int id ) const {
	if ( clipModel ) {
		return clipModel->GetContents();
	}
	return 0;
}

/*
================
idPhysics_Parametric::GetBounds
================
*/
const idBounds &idPhysics_Parametric::GetBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetBounds();
	}
	return idPhysics_Base::GetBounds();
}

/*
================
idPhysics_Parametric::GetAbsBounds
================
*/
const idBounds &idPhysics_Parametric::GetAbsBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetAbsBounds();
	}
	return idPhysics_Base::GetAbsBounds();
}

/*
================
idPhysics_Parametric::Evaluate
================
*/
bool idPhysics_Parametric::Evaluate( int timeStepMSec, int endTimeMSec ) {
	current.axis.FixDenormals();
	current.origin.FixDenormals();

	idVec3 oldLocalOrigin, oldOrigin, masterOrigin;
	idAngles oldLocalAngles, oldAngles;
	idMat3 oldAxis, masterAxis;

	isBlocked = false;
	oldLocalOrigin = current.localOrigin;
	oldOrigin = current.origin;
	oldLocalAngles = current.localAngles;
	oldAngles = current.angles;
	oldAxis = current.axis;

	current.localOrigin.Zero();
	current.localAngles.Zero();

	if ( current.spline != NULL ) {
		float length = current.splineInterpolate.GetCurrentValue( endTimeMSec );
		float t = current.spline->GetTimeForLength( length, 0.01f );
		current.localOrigin = current.spline->GetCurrentValue( t );
		if ( current.useSplineAngles ) {
			current.localAngles = current.spline->GetCurrentFirstDerivative( t ).ToAngles();
		}
	} else if ( current.linearInterpolation.GetDuration() != 0 ) {
		current.localOrigin += current.linearInterpolation.GetCurrentValue( endTimeMSec );
	} else {
		current.localOrigin += current.linearExtrapolation.GetCurrentValue( endTimeMSec );
	}
	current.origin = current.localOrigin;

	if ( current.angularInterpolation.GetDuration() != 0 ) {
		current.localAngles += current.angularInterpolation.GetCurrentValue( endTimeMSec );
	} else {
		current.localAngles += current.angularExtrapolation.GetCurrentValue( endTimeMSec );
	}

	if ( current.localAngles != oldLocalAngles ) {
		current.localAngles.Normalize360();
		current.axis = current.localAngles.ToMat3();
	}
	current.angles = current.localAngles;

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		if ( masterAxis.IsRotated() ) {
			current.origin = current.origin * masterAxis + masterOrigin;
			if ( isOrientated ) {
				current.axis *= masterAxis;
				current.angles = current.axis.ToAngles();
			}
		}
		else {
			current.origin += masterOrigin;
		}
	}

	if ( isPusher && ( oldOrigin != current.origin ) ) {

		gameLocal.push.ClipPush( pushResults, self, pushFlags, oldOrigin, oldAxis, current.origin, current.axis, GetClipModel() );
		if ( pushResults.fraction < 1.0f ) {
			clipModel->Link( gameLocal.clip, self, 0, oldOrigin, oldAxis );
			current.localOrigin = oldLocalOrigin;
			current.origin = oldOrigin;
			current.localAngles = oldLocalAngles;
			current.angles = oldAngles;
			current.axis = oldAxis;
			isBlocked = true;
			return false;
		}

		current.angles = current.axis.ToAngles();
	}

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}

	current.time = endTimeMSec;

	if ( TestIfAtRest() ) {
		Rest();
	}

	return ( current.origin != oldOrigin || current.axis != oldAxis );
}

/*
================
idPhysics_Parametric::UpdateTime
================
*/
void idPhysics_Parametric::UpdateTime( int endTimeMSec ) {
	int timeLeap = endTimeMSec - current.time;

	current.time = endTimeMSec;
	// move the trajectory start times to sync the trajectory with the current endTime
	current.linearExtrapolation.SetStartTime( current.linearExtrapolation.GetStartTime() + timeLeap );
	current.angularExtrapolation.SetStartTime( current.angularExtrapolation.GetStartTime() + timeLeap );
	current.linearInterpolation.SetStartTime( current.linearInterpolation.GetStartTime() + timeLeap );
	current.angularInterpolation.SetStartTime( current.angularInterpolation.GetStartTime() + timeLeap );
	if ( current.spline != NULL ) {
		current.spline->ShiftTime( timeLeap );
		current.splineInterpolate.SetStartTime( current.splineInterpolate.GetStartTime() + timeLeap );
	}
}

/*
================
idPhysics_Parametric::GetTime
================
*/
int idPhysics_Parametric::GetTime( void ) const {
	return current.time;
}

/*
================
idPhysics_Parametric::IsAtRest
================
*/
bool idPhysics_Parametric::IsAtRest( void ) const {
	return current.atRest >= 0;
}

/*
================
idPhysics_Parametric::GetRestStartTime
================
*/
int idPhysics_Parametric::GetRestStartTime( void ) const {
	return current.atRest;
}

/*
================
idPhysics_Parametric::IsPushable
================
*/
bool idPhysics_Parametric::IsPushable( void ) const {
	return false;
}

/*
================
idPhysics_Parametric::SaveState
================
*/
void idPhysics_Parametric::SaveState( void ) {
	saved = current;
}

/*
================
idPhysics_Parametric::RestoreState
================
*/
void idPhysics_Parametric::RestoreState( void ) {

	current = saved;

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
idPhysics_Parametric::SetOrigin
================
*/
void idPhysics_Parametric::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.linearExtrapolation.SetStartValue( newOrigin );
	current.linearInterpolation.SetStartValue( newOrigin );

	current.localOrigin = current.linearExtrapolation.GetCurrentValue( current.time );
	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
	}
	else {
		current.origin = current.localOrigin;
	}
	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
	Activate();
}

/*
================
idPhysics_Parametric::SetAxis
================
*/
void idPhysics_Parametric::SetAxis( const idMat3 &newAxis, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localAngles = newAxis.ToAngles();

	current.angularExtrapolation.SetStartValue( current.localAngles );
	current.angularInterpolation.SetStartValue( current.localAngles );

	current.localAngles = current.angularExtrapolation.GetCurrentValue( current.time );
	if ( hasMaster && isOrientated ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.axis = current.localAngles.ToMat3() * masterAxis;
		current.angles = current.axis.ToAngles();
	}
	else {
		current.axis = current.localAngles.ToMat3();
		current.angles = current.localAngles;
	}
	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
	Activate();
}

/*
================
idPhysics_Parametric::Move
================
*/
void idPhysics_Parametric::Translate( const idVec3 &translation, int id ) {
}

/*
================
idPhysics_Parametric::Rotate
================
*/
void idPhysics_Parametric::Rotate( const idRotation &rotation, int id ) {
}

/*
================
idPhysics_Parametric::GetOrigin
================
*/
const idVec3 &idPhysics_Parametric::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
idPhysics_Parametric::GetAxis
================
*/
const idMat3 &idPhysics_Parametric::GetAxis( int id ) const {
	return current.axis;
}

/*
================
idPhysics_Parametric::GetAngles
================
*/
void idPhysics_Parametric::GetAngles( idAngles &curAngles ) const {
	curAngles = current.angles;
}

/*
================
idPhysics_Parametric::SetLinearVelocity
================
*/
void idPhysics_Parametric::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
	SetLinearExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, 0, current.origin, newLinearVelocity, vec3_origin );
	current.linearInterpolation.Init( 0, 0, 0, 0, vec3_zero, vec3_zero );
	Activate();
}

/*
================
idPhysics_Parametric::SetAngularVelocity
================
*/
void idPhysics_Parametric::SetAngularVelocity( const idVec3 &newAngularVelocity, int id ) {
	idRotation rotation;
	idVec3 vec;
	float angle;

	vec = newAngularVelocity;
	angle = vec.Normalize();
	rotation.Set( vec3_origin, vec, (float) RAD2DEG( angle ) );

	SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, 0, current.angles, rotation.ToAngles(), ang_zero );
	current.angularInterpolation.Init( 0, 0, 0, 0, ang_zero, ang_zero );
	Activate();
}

/*
================
idPhysics_Parametric::GetLinearVelocity
================
*/
const idVec3 &idPhysics_Parametric::GetLinearVelocity( int id ) const {
	static idVec3 curLinearVelocity;

	curLinearVelocity = current.linearExtrapolation.GetCurrentSpeed( gameLocal.time );
	return curLinearVelocity;
}

/*
================
idPhysics_Parametric::GetAngularVelocity
================
*/
const idVec3 &idPhysics_Parametric::GetAngularVelocity( int id ) const {
	static idVec3 curAngularVelocity;
	idAngles angles;

	angles = current.angularExtrapolation.GetCurrentSpeed( gameLocal.time );
	curAngularVelocity = angles.ToAngularVelocity();
	return curAngularVelocity;
}

/*
================
idPhysics_Parametric::UnlinkClip
================
*/
void idPhysics_Parametric::UnlinkClip( void ) {
	if ( clipModel != NULL) {
		clipModel->Unlink( gameLocal.clip );
	}
}

/*
================
idPhysics_Parametric::LinkClip
================
*/
void idPhysics_Parametric::LinkClip( void ) {
	if ( clipModel != NULL ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
idPhysics_Parametric::DisableClip
================
*/
void idPhysics_Parametric::DisableClip( bool activateContacting ) {
	if ( clipModel != NULL ) {
		if ( activateContacting ) {
			WakeEntitiesContacting( self, clipModel );
		}
		clipModel->Disable();
	}
}

/*
================
idPhysics_Parametric::EnableClip
================
*/
void idPhysics_Parametric::EnableClip( void ) {
	if ( clipModel != NULL ) {
		clipModel->Enable();
	}
}

/*
================
idPhysics_Parametric::GetBlockingInfo
================
*/
const trace_t *idPhysics_Parametric::GetBlockingInfo( void ) const {
	return ( isBlocked ? &pushResults : NULL );
}

/*
================
idPhysics_Parametric::GetBlockingEntity
================
*/
idEntity *idPhysics_Parametric::GetBlockingEntity( void ) const {
	if ( isBlocked ) {
		return gameLocal.entities[ pushResults.c.entityNum ];
	}
	return NULL;
}

/*
================
idPhysics_Parametric::SetMaster
================
*/
void idPhysics_Parametric::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( master ) {
		if ( !hasMaster ) {

			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			if ( orientated ) {
				current.localAngles = ( current.axis * masterAxis.Transpose() ).ToAngles();
			}
			else {
				current.localAngles = current.axis.ToAngles();
			}

			current.linearExtrapolation.SetStartValue( current.localOrigin );
			current.angularExtrapolation.SetStartValue( current.localAngles );
			hasMaster = true;
			isOrientated = orientated;
		}
	}
	else {
		if ( hasMaster ) {
			// transform from master space to world space
			current.localOrigin = current.origin;
			current.localAngles = current.angles;
			SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, current.origin, vec3_origin, vec3_origin );
			SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, current.angles, ang_zero, ang_zero );
			hasMaster = false;
		}
	}
}

/*
================
idPhysics_Parametric::GetLinearEndTime
================
*/
int idPhysics_Parametric::GetLinearEndTime( void ) const {
	if ( current.spline != NULL ) {
		if ( current.spline->GetBoundaryType() != idCurve_Spline<idVec3>::BT_CLOSED ) {
			return current.spline->GetTime( current.spline->GetNumValues() - 1 );
		} else {
			return 0;
		}
	} else if ( current.linearInterpolation.GetDuration() != 0 ) {
		return current.linearInterpolation.GetEndTime();
	} else {
		return current.linearExtrapolation.GetEndTime();
	}
}

/*
================
idPhysics_Parametric::GetAngularEndTime
================
*/
int idPhysics_Parametric::GetAngularEndTime( void ) const {
	if ( current.angularInterpolation.GetDuration() != 0 ) {
		return current.angularInterpolation.GetEndTime();
	} else {
		return current.angularExtrapolation.GetEndTime();
	}
}
