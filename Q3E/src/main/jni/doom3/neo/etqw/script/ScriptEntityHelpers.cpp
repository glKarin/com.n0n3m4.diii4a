// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "ScriptEntityHelpers.h"
#include "../ScriptEntity.h"
#include "../IK.h"

/*
===============================================================================

	sdScriptedEntityHelper

===============================================================================
*/

sdScriptedEntityHelper::factoryType_t sdScriptedEntityHelper::helperFactory;

/*
================
sdScriptedEntityHelper::AllocHelper
================
*/
sdScriptedEntityHelper*	sdScriptedEntityHelper::AllocHelper( const char* type ) {
	return helperFactory.CreateType( type );
}

/*
================
sdScriptedEntityHelper::Startup
================
*/
void sdScriptedEntityHelper::Startup( void ) {
	helperFactory.RegisterType( "legIK",				factoryType_t::Allocator< sdScriptedEntityHelper_LegIk > );
	helperFactory.RegisterType( "playerIK",				factoryType_t::Allocator< sdScriptedEntityHelper_PlayerIK > );
}

/*
================
sdScriptedEntityHelper::Shutdown
================
*/
void sdScriptedEntityHelper::Shutdown( void ) {
	helperFactory.Shutdown();
}

/*
================
sdScriptedEntityHelper::Init
================
*/
void sdScriptedEntityHelper::Init( sdScriptEntity* owner, const sdDeclStringMap* map ) {
	_owner = owner;
	_node.AddToEnd( owner->GetHelpers() );
}

/*
===============================================================================

	sdScriptedEntityHelper_LegIk

===============================================================================
*/

/*
================
sdScriptedEntityHelper_LegIk::Update
================
*/
void sdScriptedEntityHelper_LegIk::Update( bool postThink ) {
	if ( !gameLocal.isNewFrame || postThink ) {
		return;
	}

	if ( gameLocal.time - startTime > lifetime ) {
		delete this;
		return;
	}

	idMat3 upperJointAxis,	middleJointAxis,	lowerJointAxis;
	idVec3 upperJointOrg,	middleJointOrg,		lowerJointOrg;

	ClearJointMods();

	idAnimator* animator = _owner->GetAnimator();

	if ( !animator->GetJointTransform( upperLegJoint, gameLocal.time, upperJointOrg, upperJointAxis ) ) {
		delete this;
		return;
	}
	if ( !animator->GetJointTransform( middleLegJoint, gameLocal.time, middleJointOrg, middleJointAxis ) ) {
		delete this;
		return;
	}
	if ( animator->GetJointTransform( lowerLegJoint, gameLocal.time, lowerJointOrg, lowerJointAxis ) ) {
		delete this;
		return;
	}

	{
		idVec3 traceOrg = _owner->GetRenderEntity()->origin + ( lowerJointOrg * _owner->GetRenderEntity()->axis );

		trace_t tr;
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS tr, traceOrg + idVec3( 0, 0, maxUpTrace ), traceOrg - idVec3( 0, 0, maxDownTrace ), CONTENTS_SOLID, _owner );

/*		gameRenderWorld->DebugCircle( colorGreen, tr.endpos, idVec3( 0, 0, 1 ), 8, 16 );
		gameRenderWorld->DebugLine( colorRed, tr.endpos, tr.endpos + idVec3( 0.f, 0.f, currentGroundOffset ) );
		gameRenderWorld->DebugLine( colorBlue, tr.endpos, tr.endpos + idVec3( 0.f, 0.f, lowerJointOrg.z ) );*/

		currentGroundOffset = Lerp( currentGroundOffset, lowerJointOrg.z, blendRate * MS2SEC( gameLocal.msec ) );

		idVec3 jointPos = ( ( tr.endpos + idVec3( 0.f, 0.f, currentGroundOffset ) ) - _owner->GetRenderEntity()->origin ) * _owner->GetRenderEntity()->axis.Transpose();

		idVec3 endPos;
		idIK::SolveTwoBones( upperJointOrg, jointPos, -upDir, upperLength, lowerLength, endPos );

		idMat3 axis;
		idIK::GetBoneAxis( upperJointOrg, endPos, upDir, axis );
		idMat3 upperAxis = midToUpperJoint * axis;

		idIK::GetBoneAxis( endPos, jointPos, upDir, axis );
		idMat3 middleAxis = lowerToMidJoint * axis;

		animator->SetJointAxis( upperLegJoint, JOINTMOD_WORLD_OVERRIDE, upperAxis );
		animator->SetJointAxis( middleLegJoint, JOINTMOD_WORLD_OVERRIDE, middleAxis );		

/*		idVec3 worldOrg;
		_owner->GetWorldOrigin( upperLegJoint, worldOrg );

		gameRenderWorld->DebugLine( colorGreen, worldOrg, worldOrg + ( ( side * _owner->GetRenderEntity()->axis ) * 16 ) );
		gameRenderWorld->DebugLine( colorBlue, worldOrg, worldOrg + ( ( up * _owner->GetRenderEntity()->axis ) * 16 ) );
		gameRenderWorld->DebugLine( colorRed, worldOrg, worldOrg + ( ( temp * _owner->GetRenderEntity()->axis ) * 16 ) );*/
	}
}

/*
================
sdScriptedEntityHelper_LegIk::Init
================
*/
void sdScriptedEntityHelper_LegIk::Init( sdScriptEntity* owner, const sdDeclStringMap* map ) {
	sdScriptedEntityHelper::Init( owner, map );

	const idDict& dict = map->GetDict();
	idAnimator* animator = _owner->GetAnimator();

	upperLegJoint	= animator->GetJointHandle( dict.GetString( "joint_upper" ) );
	if ( upperLegJoint == INVALID_JOINT ) {
		gameLocal.Warning( "sdScriptedEntityHelper_LegIk::Init Invalid Upper Leg Joint" );
	}

	middleLegJoint	= animator->GetJointHandle( dict.GetString( "joint_middle" ) );
	if ( middleLegJoint == INVALID_JOINT ) {
		gameLocal.Warning( "sdScriptedEntityHelper_LegIk::Init Invalid Middle Leg Joint" );
	}

	lowerLegJoint	= animator->GetJointHandle( dict.GetString( "joint_lower" ) );
	if ( lowerLegJoint == INVALID_JOINT ) {
		gameLocal.Warning( "sdScriptedEntityHelper_LegIk::Init Invalid Lower Leg Joint" );
	}

	upDir			= dict.GetVector( "direction", "0 0 1" );

	groundOffset	= dict.GetFloat( "ground_offset" );
	blendRate		= dict.GetFloat( "blend_rate", "0.95" );

	maxUpTrace		= dict.GetFloat( "max_up_trace", "64" );
	maxDownTrace	= dict.GetFloat( "max_down_trace", "64" );

	lifetime		= dict.GetInt( "lifetime", "5000" );
	startTime		= gameLocal.time;



	idMat3 axis;

	idMat3 upperJointAxis,	middleJointAxis,	lowerJointAxis;
	idVec3 upperJointOrg,	middleJointOrg,		lowerJointOrg;

	animator->GetJointTransform( upperLegJoint,		gameLocal.time, upperJointOrg,	upperJointAxis );
	animator->GetJointTransform( middleLegJoint,	gameLocal.time, middleJointOrg, middleJointAxis );
	animator->GetJointTransform( lowerLegJoint,		gameLocal.time, lowerJointOrg,	lowerJointAxis );

	upperLength		= idIK::GetBoneAxis( upperJointOrg, middleJointOrg, upDir, axis );
	midToUpperJoint = upperJointAxis * axis.Transpose();

	lowerLength		= idIK::GetBoneAxis( middleJointOrg, lowerJointOrg, upDir, axis );
	lowerToMidJoint = middleJointAxis * axis.Transpose();

	{
		idVec3 lowerJointOrg;

		animator->GetJointTransform( lowerLegJoint,		gameLocal.time, lowerJointOrg );

		idVec3 traceOrg = _owner->GetRenderEntity()->origin + ( lowerJointOrg * _owner->GetRenderEntity()->axis );

		trace_t tr;
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS tr, traceOrg + idVec3( 0, 0, maxUpTrace ), traceOrg - idVec3( 0, 0, maxDownTrace ), CONTENTS_SOLID, _owner );

		currentGroundOffset = traceOrg.z - tr.endpos.z;
	}

	owner->IncForcedAnimUpdate();
}

/*
================
sdScriptedEntityHelper_LegIk::ClearJointMods
================
*/
void sdScriptedEntityHelper_LegIk::ClearJointMods( void ) {
	idAnimator* animator = _owner->GetAnimator();

	animator->SetJointAxis( upperLegJoint, JOINTMOD_NONE, mat3_identity );
	animator->SetJointAxis( middleLegJoint, JOINTMOD_NONE, mat3_identity );
	animator->SetJointAxis( lowerLegJoint, JOINTMOD_NONE, mat3_identity );
}

/*
================
sdScriptedEntityHelper_LegIk::~sdScriptedEntityHelper_LegIk
================
*/
sdScriptedEntityHelper_LegIk::~sdScriptedEntityHelper_LegIk( void ) {
	if ( _owner != NULL ) {
		_owner->DecForcedAnimUpdate();
	}
}









/*
===============================================================================

	sdScriptedEntityHelper_Aimer

===============================================================================
*/

/*
================
sdScriptedEntityHelper_Aimer::Update
================
*/
void sdScriptedEntityHelper_Aimer::Init( bool fixupBarrel, bool invertPitch, sdScriptEntity* owner, int anim, jointHandle_t yawJoint, 
					jointHandle_t pitchJoint, jointHandle_t barrelJoint, jointHandle_t shoulderJoint, const angleClamp_t& _yawInfo, const angleClamp_t& _pitchInfo ) {

	_owner = owner;

	idAnimator* animator = _owner->GetAnimator();

	gunJoints[ AIMER_JOINT_YAW ]		= yawJoint;
	gunJoints[ AIMER_JOINT_PITCH ]		= pitchJoint;
	gunJoints[ AIMER_JOINT_BARREL ]		= barrelJoint;
	gunJoints[ AIMER_JOINT_SHOULDER ]	= shoulderJoint;

	yawInfo.current		= 0.f;
	yawInfo.ideal		= 0.f;
	yawInfo.old			= 0.f;
	yawInfo.clamp		= _yawInfo;
	yawInfo.sound		= NULL;
	if ( yawInfo.clamp.sound != NULL ) {
		yawInfo.sound = owner->GetMotorSounds().Alloc();
		yawInfo.sound->Start( yawInfo.clamp.sound );
	}

	pitchInfo.current	= 0.f;
	pitchInfo.ideal		= 0.f;
	pitchInfo.old		= 0.f;
	pitchInfo.clamp		= _pitchInfo;
	pitchInfo.sound		= NULL;
	if ( pitchInfo.clamp.sound != NULL ) {
		pitchInfo.sound = owner->GetMotorSounds().Alloc();
		pitchInfo.sound->Start( pitchInfo.clamp.sound );
	}

	idVec3 baseJointPositions[ AIMER_NUM_JOINTS ];
	for ( int i = 0; i < AIMER_NUM_JOINTS; i++ ) {
		if ( gunJoints[ i ] == INVALID_JOINT ) {
			baseJointPositions[ i ].Zero();
			baseAxes[ i ].Identity();
		} else {
			_owner->GetJointTransformForAnim( gunJoints[ i ], anim, gameLocal.time, baseJointPositions[ i ], baseAxes[ i ] );
		}
	}

	ikPaths[ AIMER_IK_SHOULDER ]	= baseJointPositions[ AIMER_JOINT_SHOULDER ];
	ikPaths[ AIMER_IK_BASE ]		= baseJointPositions[ AIMER_JOINT_YAW ] - baseJointPositions[ AIMER_JOINT_SHOULDER ];
	ikPaths[ AIMER_IK_YAW ]			= baseJointPositions[ AIMER_JOINT_PITCH ] - baseJointPositions[ AIMER_JOINT_YAW ];
	ikPaths[ AIMER_IK_PITCH ]		= baseJointPositions[ AIMER_JOINT_BARREL ] - baseJointPositions[ AIMER_JOINT_PITCH ];

	yawTranspose	= baseAxes[ AIMER_JOINT_YAW ].Transpose();
	pitchTranspose	= baseAxes[ AIMER_JOINT_PITCH ].Transpose();

	if ( fixupBarrel ) { // Gordon: HACK...
		_owner->GetAnimator()->SetJointAxis( gunJoints[ AIMER_JOINT_BARREL ], JOINTMOD_LOCAL, baseAxes[ AIMER_JOINT_BARREL ].Transpose() );
		animator->GetJointTransform( gunJoints[ AIMER_JOINT_BARREL ], gameLocal.time, baseAxes[ AIMER_JOINT_BARREL ] );
	}

	{
		idVec3 temp = ( baseJointPositions[ AIMER_JOINT_BARREL ] - baseJointPositions[ AIMER_JOINT_YAW ] );
		yawInfo.initialAngle	= RAD2DEG( atan2( temp.y, temp.x ) );

		temp *= baseAxes[ AIMER_JOINT_BARREL ].Transpose();
		temp.z = 0.f;

		yawInfo.arcLength		= temp.Normalize();
		yawInfo.offsetAngle		= 180 - RAD2DEG( atan2( temp.y, temp.x ) );
	}

	{
		idVec3 temp = ( baseJointPositions[ AIMER_JOINT_BARREL ] - baseJointPositions[ AIMER_JOINT_PITCH ] );
		pitchInfo.initialAngle	= RAD2DEG( atan2( temp.z, temp.x ) );

		temp *= baseAxes[ AIMER_JOINT_BARREL ].Transpose();
		temp.y = 0.f;
		
		pitchInfo.arcLength		= temp.Normalize();
		pitchInfo.offsetAngle	= 180 - RAD2DEG( atan2( temp.z, temp.x ) );
		if ( invertPitch ) {
			pitchInfo.offsetAngle = -pitchInfo.offsetAngle;
		}
	}					
}

/*
================
sdScriptedEntityHelper_Aimer::Update
================
*/
void sdScriptedEntityHelper_Aimer::Update( bool force ) {
	if ( gunJoints[ AIMER_JOINT_YAW ] == gunJoints[ AIMER_JOINT_PITCH ] ) {
		bool changed = false;
		changed |= UpdateAngles( yawInfo, force );
		changed |= UpdateAngles( pitchInfo, force );
		if ( changed ) {
			idMat3 yawAxes, pitchAxes;
			idAngles::YawToMat3( yawInfo.current, yawAxes );
			idAngles::PitchToMat3( pitchInfo.current, pitchAxes );

			yawAxes *= pitchAxes;
			yawAxes = baseAxes[ AIMER_JOINT_YAW ] * yawAxes * yawTranspose;

			_owner->GetAnimator()->SetJointAxis( gunJoints[ AIMER_JOINT_YAW ], JOINTMOD_LOCAL, yawAxes );
		}
	} else {
		if ( UpdateAngles( yawInfo, force ) ) {
			idMat3 yawAxes;
			idAngles::YawToMat3( yawInfo.current, yawAxes );

			yawAxes = baseAxes[ AIMER_JOINT_YAW ] * yawAxes  * yawTranspose;

			_owner->GetAnimator()->SetJointAxis( gunJoints[ AIMER_JOINT_YAW ], JOINTMOD_LOCAL, yawAxes );
		}

		if ( UpdateAngles( pitchInfo, force ) ) {
			idMat3 pitchAxes;
			idAngles::PitchToMat3( pitchInfo.current, pitchAxes );

			pitchAxes = baseAxes[ AIMER_JOINT_PITCH ] * pitchAxes * pitchTranspose;

			_owner->GetAnimator()->SetJointAxis( gunJoints[ AIMER_JOINT_PITCH ], JOINTMOD_LOCAL, pitchAxes );		
		}
	}
}

/*
================
sdScriptedEntityHelper_Aimer::UpdateAngles
================
*/
bool sdScriptedEntityHelper_Aimer::UpdateAngles( angleInfo_t& info, bool force ) {
	if ( idMath::Fabs( info.current - info.ideal ) > idMath::FLT_EPSILON ) {
		if ( info.clamp.filter > 0.f && !force ) {
			float angle = idMath::AngleNormalize180( info.ideal - info.current );
			angle *= ( 1.f - info.clamp.filter );
			info.ideal = info.current + angle;
		}
		if ( info.clamp.flags.limitRate && !force ) {
			float angle = idMath::AngleNormalize180( info.ideal - info.current );
			float frac = idMath::Fabs( angle / 180.f );

			float maxTurn = Lerp( info.clamp.rate[ 0 ], info.clamp.rate[ 1 ], frac ) * MS2SEC( gameLocal.msec );

			if ( angle < -maxTurn ) {
				info.current -= maxTurn;
			} else if ( angle > maxTurn ) {
				info.current += maxTurn;
			} else {
				info.current = info.ideal;
			}
		} else {
			info.current = info.ideal;
		}
	}

	// Gordon: stop really small deltas updating sounds + animation, we need to cache the old value though to stop them accumulating
	float delta = idMath::Fabs( info.old - info.current );
	bool changed = delta > 0.001f;
	if ( changed ) {
		info.old = info.current;
	} else {
		delta = 0.0f;
	}

	float changeVelocity = delta / MS2SEC( gameLocal.msec );
	float soundValue = changeVelocity / 30.0f;
	if ( soundValue > 1.0f ) {
		soundValue = 1.0f;
	}

	if ( info.sound != NULL ) {
		info.sound->Update( soundValue );
	}

	return changed;
}

/*
================
sdScriptedEntityHelper_Aimer::ClearTarget
================
*/
void sdScriptedEntityHelper_Aimer::ClearTarget( void ) {
	yawInfo.ideal	= 0.f;
	pitchInfo.ideal = 0.f;
}

/*
================
sdScriptedEntityHelper_Aimer::LockTarget
================
*/
void sdScriptedEntityHelper_Aimer::LockTarget( void ) {
	Update( true );
}

/*
================
sdScriptedEntityHelper_Aimer::CalcAngle
================
*/
bool sdScriptedEntityHelper_Aimer::CalcAngle( float targetDistance, float arcLength, float angle, float& out ) {
	float sidea		= arcLength;
	float sideb		= targetDistance;
	float angleb	= DEG2RAD( angle );

	if ( sidea == 0.f || sideb == 0.f ) {
		return false;
	}

	float temp		= sidea * sin( angleb ) / sideb;
	if ( idMath::Fabs( temp ) > 1.f ) {
		return false;
	}

	float anglea	= asin( temp );
	float anglec	= 180 - RAD2DEG( anglea + angleb );

	out = anglec;

	return true;
}

/*
================
sdScriptedEntityHelper_Aimer::SetTarget
================
*/
void sdScriptedEntityHelper_Aimer::SetTarget( const idVec3& _target ) {

	const idMat3& bindAxes		= _owner->GetRenderEntity()->axis;
	const idVec3& bindOrg		= _owner->GetRenderEntity()->origin;

	idMat3 yawJointAxis;
	idVec3 yawJointOrg;

	if ( gunJoints[ AIMER_JOINT_SHOULDER ] != INVALID_JOINT ) {
		idMat3 currentAxes;
		idVec3 currentOrigin;
		_owner->GetAnimator()->GetJointTransform( gunJoints[ AIMER_JOINT_SHOULDER ], gameLocal.time, currentOrigin, currentAxes );

		idMat3 transform;
		TransposeMultiply( baseAxes[ AIMER_JOINT_SHOULDER ], currentAxes, transform );

		idMat3 shoulderAxis = bindAxes;
		idVec3 shoulderOrg	= bindOrg + ( currentOrigin * shoulderAxis );

		yawJointAxis	= transform * shoulderAxis;
		yawJointOrg		= shoulderOrg + ( yawJointAxis * ikPaths[ AIMER_IK_BASE ] );
	} else {
		yawJointAxis	= bindAxes;
		yawJointOrg		= bindOrg + ( yawJointAxis * ikPaths[ AIMER_IK_BASE ] );
	}

	{
		idVec3 target = _target;
		target -= yawJointOrg;
		target *= yawJointAxis.Transpose();
		target.z = 0.f;

		yawInfo.ideal = RAD2DEG( atan2( target.y, target.x ) );

		float angle;
		if ( CalcAngle( target.Length(), yawInfo.arcLength, yawInfo.offsetAngle, angle ) ) {
			yawInfo.ideal += angle - yawInfo.initialAngle;
		}
	}

	if ( yawInfo.clamp.flags.enabled ) {
		yawInfo.ideal = idMath::ClampFloat( yawInfo.clamp.extents[ 0 ], yawInfo.clamp.extents[ 1 ], yawInfo.ideal );
	}

	idMat3 yawAxes;
	idAngles::YawToMat3( yawInfo.ideal, yawAxes );

	idMat3 pitchJointAxis	= yawAxes * yawJointAxis;
	idVec3 pitchJointOrg	= yawJointOrg + ( pitchJointAxis * ikPaths[ AIMER_IK_YAW ] );

	{
		idVec3 target = _target;
		target -= pitchJointOrg;
		target *= pitchJointAxis.Transpose();
		target.y = 0.f;

		pitchInfo.ideal = RAD2DEG( atan2( target.z, target.x ) );

		float angle;
		if ( CalcAngle( target.Length(), pitchInfo.arcLength, pitchInfo.offsetAngle, angle ) ) {
			pitchInfo.ideal += angle - pitchInfo.initialAngle;
		}
		pitchInfo.ideal = -pitchInfo.ideal; // pitch is inverted for some reason...
	}

	if ( pitchInfo.clamp.flags.enabled ) {
		pitchInfo.ideal = idMath::ClampFloat( pitchInfo.clamp.extents[ 0 ], pitchInfo.clamp.extents[ 1 ], pitchInfo.ideal );
	}
}

/*
================
sdScriptedEntityHelper_Aimer::TargetClose
================
*/
bool sdScriptedEntityHelper_Aimer::TargetClose( void ) const {
	if ( idMath::Fabs( idMath::AngleNormalize180( yawInfo.current - yawInfo.ideal ) ) > 3 ) {
		return false;
	}

	if ( idMath::Fabs( idMath::AngleNormalize180( pitchInfo.current - pitchInfo.ideal ) ) > 3 ) {
		return false;
	}

	return true;
}

/*
================
sdScriptedEntityHelper_Aimer::CanAimTo
================
*/
bool sdScriptedEntityHelper_Aimer::CanAimTo( const idAngles& angles ) const {
	if ( yawInfo.clamp.flags.enabled ) {
		if ( angles.yaw < yawInfo.clamp.extents[ 0 ] || angles.yaw > yawInfo.clamp.extents[ 1 ] ) {
			return false;
		}
	}
	if ( pitchInfo.clamp.flags.enabled ) {
		if ( angles.pitch < pitchInfo.clamp.extents[ 0 ] || angles.pitch > pitchInfo.clamp.extents[ 1 ] ) {
			return false;
		}
	}

	return true;
}







/*
===============================================================================

	sdScriptedEntityHelper_PlayerIK

===============================================================================
*/

/*
================
sdScriptedEntityHelper_PlayerIK::Update
================
*/
void sdScriptedEntityHelper_PlayerIK::Update( bool postThink ) {
	if ( !gameLocal.isNewFrame || !postThink ) {
		return;
	}

	idPlayer* player = target;
	if ( player == NULL ) {
		return;
	}

	if ( player->aorFlags & AOR_INHIBIT_IK ) {
		return;
	}

	ik.Update( target, _owner );
}

/*
================
sdScriptedEntityHelper_PlayerIK::Init
================
*/
void sdScriptedEntityHelper_PlayerIK::Init( sdScriptEntity* owner, const sdDeclStringMap* map ) {
	sdScriptedEntityHelper::Init( owner, map );

	playerIndex = map->GetDict().GetInt( "player_index", "0" );
	ik.Init( _owner, map->GetDict() );
}

/*
================
sdScriptedEntityHelper_PlayerIK::WantsToThink
================
*/
bool sdScriptedEntityHelper_PlayerIK::WantsToThink( void ) {
	return target.IsValid();
}







/*
===============================================================================

  sdPlayerArmIK

===============================================================================
*/

/*
================
sdPlayerArmIK::sdPlayerArmIK
================
*/
sdPlayerArmIK::sdPlayerArmIK( void ) {
	for ( int i = 0; i < NUM_ARMS; i++ ) {
		armTargets[ i ].joint = INVALID_JOINT;
	}
}

/*
================
sdPlayerArmIK::Init
================
*/
bool sdPlayerArmIK::Init( idEntity* target, const idDict& parms ) {
	const char* jointName;

	idAnimator* animator = target->GetAnimator();
	assert( animator != NULL );
	
	jointName = parms.GetString( "joint_left" );
	armTargets[ 0 ].joint = animator->GetJointHandle( jointName );
	if ( armTargets[ 0 ].joint == INVALID_JOINT ) {
		gameLocal.Warning( "sdPlayerArmIK::Init Invalid Joint '%s'", jointName );
		return false;
	}

	jointName = parms.GetString( "joint_right" );
	armTargets[ 1 ].joint = animator->GetJointHandle( jointName );
	if ( armTargets[ 1 ].joint == INVALID_JOINT ) {
		gameLocal.Warning( "sdPlayerArmIK::Init Invalid Joint '%s'", jointName );
		return false;
	}

	return true;
}

/*
================
sdPlayerArmIK::Update
================
*/
void sdPlayerArmIK::Update( idPlayer* player, idEntity* target ) {
	struct arm_t {
		idMat3				targetShoulderAxis;
		idMat3				targetElbowAxis;
		idMat3				targetHandAxis;
	};

	arm_t					arms[ NUM_ARMS ];

	ClearJointMods( player );

	idAnimator* animator = player->GetAnimator();

	idMat3 tempAxis;

	idVec3 modelOrigin			= player->GetRenderEntity()->origin;
	idMat3 modelAxis			= player->GetRenderEntity()->axis;
	idMat3 transposeModelAxis	= modelAxis.Transpose();

	// get the arm bone lengths and rotation matrices
	for ( int i = 0; i < NUM_ARMS; i++ ) {
		if ( armTargets[ i ].joint == INVALID_JOINT ) {
			continue;
		}

		idMat3 handAxis;
		idVec3 handOrigin;
		animator->GetJointTransform( player->GetHandJoint( i ), gameLocal.time, handOrigin, handAxis );
		
		idMat3 elbowAxis;
		idVec3 elbowOrigin;
		animator->GetJointTransform( player->GetElbowJoint( i ), gameLocal.time, elbowOrigin, elbowAxis );

		idMat3 shoulderAxis;
		idVec3 shoulderOrigin;
		animator->GetJointTransform( player->GetShoulderJoint( i ), gameLocal.time, shoulderOrigin, shoulderAxis );

		idVec3 t1 = ( elbowOrigin - shoulderOrigin );
//		t1.Normalize();
		idVec3 t2 = ( elbowOrigin - handOrigin );
//		t2.Normalize();

		idVec3 dir = t1 + t2;
		dir.Normalize();

		// conversion from upper arm bone axis to should joint axis
		float upperArmLength				= idIK::GetBoneAxis( shoulderOrigin, elbowOrigin, dir, tempAxis );
		idMat3 upperArmToShoulderJoint		= shoulderAxis * tempAxis.Transpose();

		// conversion from lower arm bone axis to elbow joint axis
		float lowerArmLength				= idIK::GetBoneAxis( elbowOrigin, handOrigin, dir, tempAxis );
		idMat3 lowerArmToElbowJoint			= elbowAxis * tempAxis.Transpose();

		// get target
		idVec3 targetOrigin;
		target->GetWorldOriginAxis( armTargets[ i ].joint, targetOrigin, arms[ i ].targetHandAxis );

		idVec3 targetOriginLocal = targetOrigin - modelOrigin;
		targetOriginLocal *= transposeModelAxis;
		arms[ i ].targetHandAxis *= transposeModelAxis;
		
		// solve IK and calculate elbow position
		idIK::SolveTwoBones( shoulderOrigin, targetOriginLocal, dir, upperArmLength, lowerArmLength, elbowOrigin );

		if ( ik_debug.GetBool() ) {
			idVec3 shoulderWorld	= ( shoulderOrigin * modelAxis ) +  modelOrigin;
			idVec3 elbowWorld		= ( elbowOrigin * modelAxis ) +  modelOrigin;

			gameRenderWorld->DebugLine( colorCyan, shoulderWorld, elbowWorld );
			gameRenderWorld->DebugLine( colorRed, elbowWorld, targetOrigin );
			gameRenderWorld->DebugBox( colorYellow, idBox( targetOrigin, idVec3( 2, 2, 2 ), mat3_identity ) );
			gameRenderWorld->DebugLine( colorGreen, elbowWorld, elbowWorld + ( ( dir * modelAxis ) * 8 ) );
		}

		// get the axis for the shoulder joint
		idIK::GetBoneAxis( shoulderOrigin, elbowOrigin, dir, tempAxis );
		arms[ i ].targetShoulderAxis = upperArmToShoulderJoint * tempAxis;

		// get the axis for the elbow joint
		idIK::GetBoneAxis( elbowOrigin, targetOriginLocal, dir, tempAxis );
		arms[ i ].targetElbowAxis = lowerArmToElbowJoint * tempAxis;
	}

	for ( int i = 0; i < NUM_ARMS; i++ ) {
		if ( armTargets[ i ].joint == INVALID_JOINT ) {
			continue;
		}

		animator->SetJointAxis( player->GetShoulderJoint( i ), JOINTMOD_WORLD_OVERRIDE, arms[ i ].targetShoulderAxis );
		animator->SetJointAxis( player->GetElbowJoint( i ), JOINTMOD_WORLD_OVERRIDE, arms[ i ].targetElbowAxis );
		animator->SetJointAxis( player->GetHandJoint( i ), JOINTMOD_WORLD_OVERRIDE, arms[ i ].targetHandAxis );
	}
}

/*
================
sdPlayerArmIK::ClearJointMods
================
*/
void sdPlayerArmIK::ClearJointMods( idPlayer* player ) {
	idAnimator* animator = player->GetAnimator();
	for ( int i = 0; i < NUM_ARMS; i++ ) {
		animator->SetJointAxis( player->GetShoulderJoint( i ), JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( player->GetElbowJoint( i ), JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( player->GetHandJoint( i ), JOINTMOD_NONE, mat3_identity );
	}
}
