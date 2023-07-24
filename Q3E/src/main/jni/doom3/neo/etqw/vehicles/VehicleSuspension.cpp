// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "VehicleSuspension.h"
#include "Transport.h"
#include "TransportComponents.h"

sdVehicleSuspension::factoryType_t sdVehicleSuspension::suspensionFactory;

// These should be tweaked so stationary vehicles don't cause any reskinning
// These are in degrees and world units
#define SUSPENSION_PIVOT_ANGLE_THRESHOLD			0.5f
#define SUSPENSION_DOUBLEWISHBONE_ANGLE_THRESHOLD	0.5f
#define SUSPENSION_VERTICAL_OFFSET_THRESHOLD		0.25f

/*
===============================================================================

	sdVehicleSuspension

===============================================================================
*/

/*
================
sdVehicleSuspension::Startup
================
*/
void sdVehicleSuspension::Startup( void ) {
	suspensionFactory.RegisterType( sdVehicleSuspension_Pivot::TypeName(),			factoryType_t::Allocator< sdVehicleSuspension_Pivot > );
	suspensionFactory.RegisterType( sdVehicleSuspension_DoubleWishbone::TypeName(),	factoryType_t::Allocator< sdVehicleSuspension_DoubleWishbone > );
	suspensionFactory.RegisterType( sdVehicleSuspension_Vertical::TypeName(),		factoryType_t::Allocator< sdVehicleSuspension_Vertical > );
	suspensionFactory.RegisterType( sdVehicleSuspension_2JointLeg::TypeName(),		factoryType_t::Allocator< sdVehicleSuspension_2JointLeg > );
}

/*
================
sdVehicleView::Shutdown
================
*/

void sdVehicleSuspension::Shutdown( void ) {
	suspensionFactory.Shutdown();
}

/*
================
sdVehicleSuspension::GetSuspension
================
*/
sdVehicleSuspension* sdVehicleSuspension::GetSuspension( const char* name ) {
	return suspensionFactory.CreateType( name );
}

/*
===============================================================================

	sdVehicleSuspension_Pivot

===============================================================================
*/

/*
================
sdVehicleSuspension_Pivot::sdVehicleSuspension_Pivot
================
*/
sdVehicleSuspension_Pivot::sdVehicleSuspension_Pivot( void ) {
	_object					= NULL;
	_suspensionJoint		= INVALID_JOINT;
	_suspensionRadius		= 0;
	_suspensionAngle		= 0;
	_initialOffset			= 0;
	_lastOffset				= 0;
	_idealSuspensionAngle	= 0;
	_oldSuspensionAngle		= 0;
}

/*
================
sdVehicleSuspension_Pivot::~sdVehicleSuspension_Pivot
================
*/
sdVehicleSuspension_Pivot::~sdVehicleSuspension_Pivot( void ) {
}

/*
================
sdVehicleSuspension_Pivot::Init
================
*/
bool sdVehicleSuspension_Pivot::Init( sdVehicleSuspensionInterface* object, const idDict& info ) {
	idVec3 wheelOrg, suspensionOrg;

	_object = object;

	sdTransport* transport = _object->GetParent();
	idAnimator* animator = transport->GetAnimator();

	_suspensionJoint = animator->GetJointHandle( info.GetString( "joint" ) );
	if ( _suspensionJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleSuspension_Pivot::Init Invalid Suspension Joint '%s'", info.GetString( "joint" ) );
	}

	transport->GetJointTransformForAnim( _suspensionJoint, 1, gameLocal.time, suspensionOrg, _suspensionAxis );
	transport->GetJointTransformForAnim( _object->GetJoint(), 1, gameLocal.time, wheelOrg );

	idVec3 dir = wheelOrg - suspensionOrg;
	dir[ 1 ] = 0.f;

	_suspensionRadius = dir.Normalize();

	float v = -dir[ 2 ];
	float h = dir[ 0 ];

	_suspensionAngle = RAD2DEG( atan2( v, h ) );
	_initialOffset = v * _suspensionRadius;
	_lastOffset = -idMath::INFINITY;

	_reverse = info.GetBool( "reverse" );

	return true;
}

/*
================
sdVehicleSuspension_Pivot::ClearIKJoints
================
*/
void sdVehicleSuspension_Pivot::ClearIKJoints( idAnimator* animator ) {
	animator->SetJointAxis( _suspensionJoint, JOINTMOD_NONE, mat3_identity );
}

/*
================
sdVehicleSuspension_Pivot::UpdateIKJoints
================
*/
bool sdVehicleSuspension_Pivot::UpdateIKJoints( idAnimator* animator ) {
	float offset = _object->GetOffset();
	if ( _lastOffset != -idMath::INFINITY ) {
		if ( offset < _lastOffset ) { 
			offset = Lerp( _lastOffset, offset, 0.5f );
		}
	}
	_lastOffset = offset;

	float offsetAngle = RAD2DEG( asin( ( _initialOffset - offset ) / _suspensionRadius ) );
	if ( !_reverse ) {
		_idealSuspensionAngle = offsetAngle - _suspensionAngle;
	} else {
		_idealSuspensionAngle = -( offsetAngle - ( 180 - _suspensionAngle ) );
	}

	if ( fabs( _oldSuspensionAngle - _idealSuspensionAngle ) > SUSPENSION_PIVOT_ANGLE_THRESHOLD ) {
		_oldSuspensionAngle = _idealSuspensionAngle;

		idMat3 temp;
		idAngles::PitchToMat3( _idealSuspensionAngle, temp );

		animator->SetJointAxis( _suspensionJoint, JOINTMOD_WORLD_OVERRIDE, _suspensionAxis * temp );

		return true;
	}
	return false;
}


/*
===============================================================================

	sdVehicleSuspension_DoubleWishbone

===============================================================================
*/

/*
================
sdVehicleSuspension_DoubleWishbone::sdVehicleSuspension_DoubleWishbone
================
*/
sdVehicleSuspension_DoubleWishbone::sdVehicleSuspension_DoubleWishbone( void ) {
	_object					= NULL;
	_upperSuspensionJoint	= INVALID_JOINT;
	_lowerSuspensionJoint	= INVALID_JOINT;
	_suspensionRadius		= 0;
	_suspensionAngle		= 0;
	_initialOffset			= 0;
	_lastOffset				= 0;
	_currentSuspensionAngle	= 0;
	_oldSuspensionAngle		= 0;
	_upperSuspensionAxes.Identity();
	_lowerSuspensionAxes.Identity();
}

/*
================
sdVehicleSuspension_DoubleWishbone::~sdVehicleSuspension_DoubleWishbone
================
*/
sdVehicleSuspension_DoubleWishbone::~sdVehicleSuspension_DoubleWishbone( void ) {
}

/*
================
sdVehicleSuspension_DoubleWishbone::Init
================
*/
bool sdVehicleSuspension_DoubleWishbone::Init( sdVehicleSuspensionInterface* object, const idDict& info ) {
	idVec3 wheelOrg, suspensionOrg;
	idMat3 suspensionAxes;

	_object = object;

	sdTransport* transport = _object->GetParent();
	idAnimator* animator = transport->GetAnimator();

	_upperSuspensionJoint = animator->GetJointHandle( info.GetString( "joint_upper" ) );
	if ( _upperSuspensionJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleSuspension_DoubleWishbone::Init Invalid Suspension Joint '%s'", info.GetString( "joint_upper" ) );
	}
	_lowerSuspensionJoint = animator->GetJointHandle( info.GetString( "joint_lower" ) );
	if ( _lowerSuspensionJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleSuspension_DoubleWishbone::Init Invalid Suspension Joint '%s'", info.GetString( "joint_lower" ) );
	}

	_lerpScale			= info.GetFloat( "lerp_scale", "0.5" );

	transport->GetJointTransformForAnim( _lowerSuspensionJoint, 1, gameLocal.time, _lowerSuspensionAxes );
	transport->GetJointTransformForAnim( _upperSuspensionJoint, 1, gameLocal.time, _upperSuspensionAxes );

	transport->GetJointTransformForAnim( _upperSuspensionJoint, 1, gameLocal.time, suspensionOrg );
	transport->GetJointTransformForAnim( _object->GetJoint(), 1, gameLocal.time, wheelOrg );

	_rightWheel = suspensionOrg[ 1 ] < wheelOrg[ 1 ];

	idVec3 dir = wheelOrg - suspensionOrg;
	dir[ 0 ] = 0.f;

	_suspensionRadius = dir.Normalize();

	_suspensionAngle = RAD2DEG( atan2( -dir[ 2 ], dir[ 1 ] ) );
	_initialOffset = -dir[ 2 ] * _suspensionRadius;
	_lastOffset = -idMath::INFINITY;

	return true;
}

/*
================
sdVehicleSuspension_DoubleWishbone::ClearIKJoints
================
*/
void sdVehicleSuspension_DoubleWishbone::ClearIKJoints( idAnimator* animator ) {
	animator->SetJointAxis( _upperSuspensionJoint, JOINTMOD_NONE, mat3_identity );
	animator->SetJointAxis( _lowerSuspensionJoint, JOINTMOD_NONE, mat3_identity );
}

/*
================
sdVehicleSuspension_DoubleWishbone::UpdateIKJoints
================
*/
bool sdVehicleSuspension_DoubleWishbone::UpdateIKJoints( idAnimator* animator ) {
	float offset = _object->GetOffset();
	if ( _lastOffset != -idMath::INFINITY ) {
		if ( offset < _lastOffset ) { 
			offset = Lerp( _lastOffset, offset, _lerpScale );
		}
	}
	_lastOffset = offset;

	float offsetAngle = RAD2DEG( asin( ( _initialOffset - offset ) / _suspensionRadius ) );
	if ( _rightWheel ) {
		_currentSuspensionAngle = offsetAngle;
	} else {
		_currentSuspensionAngle = 180 - offsetAngle;
	}
	_currentSuspensionAngle -= _suspensionAngle;

	if ( fabs( _oldSuspensionAngle - _currentSuspensionAngle ) > SUSPENSION_DOUBLEWISHBONE_ANGLE_THRESHOLD ) {
		_oldSuspensionAngle = _currentSuspensionAngle;

		idMat3 temp;
		idAngles::RollToMat3( -_currentSuspensionAngle, temp );

		animator->SetJointAxis( _upperSuspensionJoint, JOINTMOD_WORLD_OVERRIDE, _upperSuspensionAxes * temp );
		animator->SetJointAxis( _lowerSuspensionJoint, JOINTMOD_WORLD_OVERRIDE, _lowerSuspensionAxes * temp );

		return true;
	}
	return false;
}


/*
===============================================================================

	sdVehicleSuspension_Vertical

===============================================================================
*/

/*
================
sdVehicleSuspension_Vertical::sdVehicleSuspension_Vertical
================
*/
sdVehicleSuspension_Vertical::sdVehicleSuspension_Vertical( void ) {
	_oldOffset = 0.0f;
	_offset = -idMath::INFINITY;
}

/*
================
sdVehicleSuspension_Vertical::~sdVehicleSuspension_Vertical
================
*/
sdVehicleSuspension_Vertical::~sdVehicleSuspension_Vertical( void ) {
}

/*
================
sdVehicleSuspension_Vertical::Init
================
*/
bool sdVehicleSuspension_Vertical::Init( sdVehicleSuspensionInterface* object, const idDict& info ) {
	return Init( object, info.GetString( "joint" ) );
}

/*
================
sdVehicleSuspension_Vertical::Init
================
*/
bool sdVehicleSuspension_Vertical::Init( sdVehicleSuspensionInterface* object, const char* jointName ) {
	_object = object;

	sdTransport* transport = _object->GetParent();
	idAnimator* animator = transport->GetAnimator();

	_suspensionJoint = animator->GetJointHandle( jointName );
	if ( _suspensionJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleSuspension_Vertical::Init Invalid Suspension Joint '%s'", jointName );
	}

	return true;
}

/*
================
sdVehicleSuspension_Vertical::ClearIKJoints
================
*/
void sdVehicleSuspension_Vertical::ClearIKJoints( idAnimator* animator ) {
	animator->SetJointPos( _suspensionJoint, JOINTMOD_NONE, vec3_origin );
}

/*
================
sdVehicleSuspension_Vertical::UpdateIKJoints
================
*/
bool sdVehicleSuspension_Vertical::UpdateIKJoints( idAnimator* animator ) {
	float offset = _object->GetOffset();
	if ( _offset != -idMath::INFINITY && offset < _offset ) { 
		_offset = Lerp( _offset, offset, 0.3f );
	} else {
		_offset = offset;
	}

	if ( idMath::Fabs( _oldOffset - _offset ) < SUSPENSION_VERTICAL_OFFSET_THRESHOLD ) {
		return false;
	}

	animator->SetJointPos( _suspensionJoint, JOINTMOD_WORLD, idVec3( 0.f, 0.f, _offset ) );
	_oldOffset = _offset;
	return true;
}







/*
===============================================================================

	sdVehicleSuspension_2JointLeg

===============================================================================
*/

/*
================
sdVehicleSuspension_2JointLeg::sdVehicleSuspension_2JointLeg
================
*/
sdVehicleSuspension_2JointLeg::sdVehicleSuspension_2JointLeg( void ) {
	_lastOffset = -idMath::INFINITY;
}


/*
================
sdVehicleSuspension_2JointLeg::Init
================
*/
bool sdVehicleSuspension_2JointLeg::Init( sdVehicleSuspensionInterface* object, const idDict& info ) {
	const char* jointName;

	_object = object;

	idAnimator* animator = object->GetParent()->GetAnimator();

	jointHandle_t _ankleJoint = object->GetJoint();
	if ( _ankleJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleSuspension_2JointLeg::Init: Invalid ankle joint" );
	}

	jointName = info.GetString( "joint_knee" );
	_kneeJoint = animator->GetJointHandle( jointName );
	if ( _kneeJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleSuspension_2JointLeg::Init: invalid knee joint '%s'", jointName );
	}

	jointName = info.GetString( "joint_hip" );
	_hipJoint = animator->GetJointHandle( jointName );
	if ( _hipJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleSuspension_2JointLeg::Init: invalid hip joint '%s'", jointName );
	}

	idVec3 ankleOrigin;
	idMat3 ankleAxis;
	animator->GetJointTransform( _ankleJoint, gameLocal.time, ankleOrigin, ankleAxis );

	idVec3 kneeOrigin;
	idMat3 kneeAxis;
	animator->GetJointTransform( _kneeJoint, gameLocal.time, kneeOrigin, kneeAxis );

	idVec3 hipOrigin;
	idMat3 hipAxis;
	animator->GetJointTransform( _hipJoint, gameLocal.time, hipOrigin, hipAxis );

	animator->GetJointTransform( _object->GetJoint(), gameLocal.time, _offset );

	idVec3 dir = info.GetVector( "dir_hip", "0 0 1" );
	dir.Normalize();

	_hipForward		= dir * hipAxis.Transpose();
	_kneeForward	= dir * kneeAxis.Transpose();

	idMat3 axis;

	// conversion from upper leg bone axis to hip joint axis
	_upperLegLength			= idIK::GetBoneAxis( hipOrigin, kneeOrigin, dir, axis );
	_upperLegToHipJoint		= hipAxis * axis.Transpose();

	// conversion from lower leg bone axis to knee joint axis
	_lowerLegLength			= idIK::GetBoneAxis( kneeOrigin, ankleOrigin, dir, axis );
	_lowerLegToKneeJoint	= kneeAxis * axis.Transpose();

	_lerpScale				= info.GetFloat( "lerp_scale", "0.3" );
	_reverse				= info.GetBool( "reverse", "0" );

	return true;
}

/*
================
sdVehicleSuspension_2JointLeg::UpdateIKJoints
================
*/
bool sdVehicleSuspension_2JointLeg::UpdateIKJoints( idAnimator* animator ) {
	float offset = _object->GetOffset();
	if ( _lastOffset != idMath::INFINITY ) {
		if ( offset < _lastOffset ) { 
			offset = Lerp( _lastOffset, offset, _lerpScale );
		}
	}

	if ( idMath::Fabs( _lastOffset - offset ) < 0.1f ) {
		return false;
	}

	_lastOffset = offset;

	idMat3 axis;

	const idVec3& modelOrigin = _object->GetParent()->GetRenderEntity()->origin;
	const idMat3& modelAxis = _object->GetParent()->GetRenderEntity()->axis;

	idVec3 newOffset = _offset;
	if ( !_reverse ) {
		newOffset[ 2 ] -= _lastOffset;
	} else {
		newOffset[ 2 ] += _lastOffset;
	}

	idVec3 target = modelOrigin + ( newOffset * modelAxis );

	// get the position of the hip in world space
	idVec3 hipOrigin;
	animator->GetJointTransform( _hipJoint, gameLocal.time, hipOrigin, axis );
	hipOrigin = modelOrigin + hipOrigin * modelAxis;
	idVec3 hipDir = _hipForward * axis * modelAxis;

	// get the IK bend direction
	idVec3 kneeOrigin;
	animator->GetJointTransform( _kneeJoint, gameLocal.time, kneeOrigin, axis );
	idVec3 kneeDir = _kneeForward * axis * modelAxis;

	// solve IK and calculate knee position
	idIK::SolveTwoBones( hipOrigin, target, kneeDir, _upperLegLength, _lowerLegLength, kneeOrigin );

	// get the axis for the hip joint
	idIK::GetBoneAxis( hipOrigin, kneeOrigin, hipDir, axis );
	idMat3 hipAxis = _upperLegToHipJoint * ( axis * modelAxis.Transpose() );

	// get the axis for the knee joint
	idIK::GetBoneAxis( kneeOrigin, target, kneeDir, axis );
	idMat3 kneeAxis = _lowerLegToKneeJoint * ( axis * modelAxis.Transpose() );

	animator->SetJointAxis( _hipJoint, JOINTMOD_WORLD_OVERRIDE, hipAxis );
	animator->SetJointAxis( _kneeJoint, JOINTMOD_WORLD_OVERRIDE, kneeAxis );

	return true;
}

/*
================
sdVehicleSuspension_2JointLeg::ClearIKJoints
================
*/
void sdVehicleSuspension_2JointLeg::ClearIKJoints( idAnimator* animator ) {
	animator->SetJointAxis( _hipJoint, JOINTMOD_NONE, mat3_identity );
	animator->SetJointAxis( _kneeJoint, JOINTMOD_NONE, mat3_identity );
}
