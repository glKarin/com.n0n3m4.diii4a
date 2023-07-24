// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "RepairDrone.h"
#include "../script/Script_Helper.h"
#include "../physics/Physics_SimpleRigidBody.h"
#include "../ContentMask.h"

/*
==============
sdRepairDroneBroadcastData::MakeDefault
==============
*/
void sdRepairDroneBroadcastData::MakeDefault( void ) {
	sdScriptEntityBroadcastData::MakeDefault();

	repairTarget = NULL;
	ownerEntity = NULL;
}

/*
==============
sdRepairDroneBroadcastData::Write
==============
*/
void sdRepairDroneBroadcastData::Write( idFile* file ) const {
	sdScriptEntityBroadcastData::Write( file );

	file->WriteInt( gameLocal.GetSpawnId( repairTarget ) );
	file->WriteInt( gameLocal.GetSpawnId( ownerEntity ) );
}

/*
==============
sdRepairDroneBroadcastData::Read
==============
*/
void sdRepairDroneBroadcastData::Read( idFile* file ) {
	sdScriptEntityBroadcastData::Read( file );

	int spawnId;
	file->ReadInt( spawnId );
	repairTarget = gameLocal.EntityForSpawnId( spawnId );
	file->ReadInt( spawnId );
	ownerEntity = gameLocal.EntityForSpawnId( spawnId );
}

/*
===============================================================================

	sdRepairDrone

===============================================================================
*/
const idEventDef EV_MoveTo( "setTargetPosition", '\0', DOC_TEXT( "Sets where the drone should move towards." ), 2, NULL, "v", "position", "Target position in world space.", "f", "time", "Time to perform the move in, in seconds." );
const idEventDef EV_SetEffectOrigins( "setEffectOrigins", '\0', DOC_TEXT( "Sets the start and end position of the repair beam, and whether it should be active or not." ), 3, NULL, "v", "start", "Start position of the beam.", "v", "end", "End position of the beam.", "b", "active", "Whether the beam should be active or not." );
const idEventDef EV_SetEntities( "setEntities", '\0', DOC_TEXT( "Sets the owner, and the repair target of the drone." ), 2, NULL, "E", "target", "The repair target to set.", "E", "owner", "The owner to set." );
const idEventDef EV_GetRepairTarget( "getRepairTarget", 'e', DOC_TEXT( "Returns the repair target of the drone, or $null$ if none." ), 0, NULL );
const idEventDef EV_GetOwnerEntity( "getOwnerEntity", 'e', DOC_TEXT( "Returns the owner of the drone, or $null$ if none." ), 0, NULL );
const idEventDef EV_HideThrusters( "hideThrusters", '\0', DOC_TEXT( "Turns off the drone's thruster effects." ), 0, NULL );

extern const idEventDef EV_Disable;

CLASS_DECLARATION( sdScriptEntity, sdRepairDrone )
	EVENT( EV_MoveTo,			sdRepairDrone::Event_MoveTo )
	EVENT( EV_SetEffectOrigins,	sdRepairDrone::Event_SetEffectOrigins )
	EVENT( EV_SetEntities,		sdRepairDrone::Event_SetEntities )
	EVENT( EV_GetRepairTarget,	sdRepairDrone::Event_GetRepairTarget )
	EVENT( EV_GetOwnerEntity,	sdRepairDrone::Event_GetOwnerEntity )
	EVENT( EV_HideThrusters,	sdRepairDrone::Event_HideThrusters )
	EVENT( EV_Disable,			sdRepairDrone::Event_Disable )
END_CLASS

/*
================
sdRepairDrone::Spawn
================
*/
void sdRepairDrone::Spawn( void ) {
	moveToOrigin.Zero();
	moveFromOrigin.Zero();
	moveVelocity.Zero();
	moveFromTime = 0.f;
	moveToTime = 0.f;

	jet1Offset = spawnArgs.GetVector( "jet1_position" );
	jet2Offset = spawnArgs.GetVector( "jet2_position" );
	jet3Offset = spawnArgs.GetVector( "jet3_position" );
	jet4Offset = spawnArgs.GetVector( "jet4_position" );

	throttleVelScale = spawnArgs.GetFloat( "throttle_vel_scale" );
	throttleMin = spawnArgs.GetFloat( "throttle_min" );
	throttleMax = spawnArgs.GetFloat( "throttle_max" );

	velocityToAngle = spawnArgs.GetFloat( "velocity_to_angle" );
	directionRecovery = spawnArgs.GetFloat( "direction_recovery" );
	angleMax = spawnArgs.GetFloat( "angle_max" );
	angleToForce = spawnArgs.GetFloat( "angle_to_force" );
	effectVelocityScale = spawnArgs.GetFloat( "effect_velocity_scale", "0.0125" );

	maxSideVel = spawnArgs.GetFloat( "max_side_velocity", "60" );
	maxUpVel = spawnArgs.GetFloat( "max_up_velocity", "80" );

	lastThrottle = 0.f;
	effectActive = false;

	const char *damageName;

	lastWaterDamageTime = 0;
	submergeTime		= 0;
	damageName			= spawnArgs.GetString( "dmg_water" );	
	waterDamageDecl		= gameLocal.declDamageType[ damageName ];
	if ( !waterDamageDecl ) {
		gameLocal.Warning( "sdRepairDrone::Spawn Couldn't find water Damage Type '%s'", damageName );
	}

	repairTarget = NULL;
	ownerEntity = NULL;

	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( CONTENTS_SOLID | CONTENTS_RENDERMODEL | CONTENTS_PROJECTILE );

	// Setup initial effect values
	renderEffect_t &bEffect 					= repairBeamEffect.GetRenderEffect();
	bEffect.declEffect							= gameLocal.FindEffect( spawnArgs.GetString( "fx_repairbeam" ), false );
	bEffect.axis								= mat3_identity;
	bEffect.attenuation							= 1.f;
	bEffect.hasEndOrigin						= true;
	bEffect.loop								= true;
	bEffect.shaderParms[SHADERPARM_RED]			= 1.0f;
	bEffect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
	bEffect.shaderParms[SHADERPARM_BLUE]		= 1.0f;
	bEffect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
	bEffect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;

	renderEffect_t &eEffect						= engineEffect.GetRenderEffect();
	eEffect.declEffect							= gameLocal.FindEffect( spawnArgs.GetString( "fx_engine" ), false );
	eEffect.axis								= mat3_identity;
	eEffect.attenuation							= 0.f;
	eEffect.hasEndOrigin						= false;
	eEffect.loop								= true;
	eEffect.shaderParms[SHADERPARM_RED]			= 1.0f;
	eEffect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
	eEffect.shaderParms[SHADERPARM_BLUE]		= 1.0f;
	eEffect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
	eEffect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;
	engineEffect.Start( gameLocal.time );

	postThinkEntNode.AddToEnd( gameLocal.postThinkEntities );

	sdPhysics_SimpleRigidBody* rbPhysics = GetPhysics()->Cast< sdPhysics_SimpleRigidBody >();
	if ( rbPhysics != NULL ) {
		rbPhysics->SetBouncyness( 0.0f, 1.0f, 1.0f );
		rbPhysics->SetStopSpeed( 0.1f );
	}
}

/*
================
sdRepairDrone::Think
================
*/
void sdRepairDrone::Think( void ) {
	UpdateMove();
	sdScriptEntity::Think();
	UpdateEffects();
}

/*
================
sdRepairDrone::Think
================
*/
void sdRepairDrone::PostThink( void ) {
	UpdateWaterDamage();

	Present();
}


/*
================
sdRepairDrone::Event_MoveTo
================
*/
void sdRepairDrone::Event_MoveTo( const idVec3& newOrigin, float timeDelta ) {
	moveFromTime = MS2SEC( gameLocal.GetTime() );
	moveToTime = moveFromTime + timeDelta;
	moveToOrigin = newOrigin;

	// without this slight offset, it has a tendency to fall!
	moveToOrigin.z += 0.45f;	
	moveFromOrigin = GetPhysics()->GetOrigin();
	moveVelocity = ( moveToOrigin - moveFromOrigin ) * ( 1 / timeDelta );
}

/*
Handy functions
*/
inline float SignedSquare( float value ) {
	return value > 0.f ? value * value : - ( value * value );
}


/*
================
sdRepairDrone::UpdateMove
================
*/
void sdRepairDrone::UpdateMove( void ) {
	if ( moveToTime - moveFromTime < 0.001f ) {
		return;
	}

	// first up, make sure we're not too close to the ground!
	idVec3 origin = GetPhysics()->GetOrigin();
	float tooCloseToGroundMakeup = 0.0f;
	trace_t	tr;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS tr, origin, origin - idVec3(0.f, 0.f, 16.f), MASK_SOLID | MASK_OPAQUE, this );	
	if ( tr.fraction < 1.0f ) {
		tooCloseToGroundMakeup = 16.0f - tr.fraction * 16.0f;
	}

	const float frameDelta = MS2SEC( gameLocal.msec );
	const float invFrameDelta = 1 / frameDelta;
	const float now = MS2SEC( gameLocal.GetTime() ) + frameDelta;
	const float point = idMath::ClampFloat( 0.f, 1.f, ( now - moveFromTime ) / ( moveToTime - moveFromTime ) );
	const idVec3 desiredOrigin = point * ( moveToOrigin - moveFromOrigin ) + moveFromOrigin;

	const idMat3& axis = GetPhysics()->GetAxis();
	const idVec3& forwardVector = axis[ 0 ];
	const idVec3& rightVector = axis[ 1 ];
	const idVec3& upVector = axis[ 2 ];
	const idVec3& linVelocity = GetPhysics()->GetLinearVelocity( 0 );

	const idVec3 jet1Origin = jet1Offset * axis + origin;
	const idVec3 jet2Origin = jet2Offset * axis + origin;
	const idVec3 jet3Origin = jet3Offset * axis + origin;
	const idVec3 jet4Origin = jet4Offset * axis + origin;
	float jet1Force = 0.f;
	float jet2Force = 0.f;
	float jet3Force = 0.f;
	float jet4Force = 0.f;

	const idVec3& angVelocity = GetPhysics()->GetAngularVelocity();
	const float yawVelocity = angVelocity * upVector;
	const float pitchVelocity = angVelocity * rightVector;
	const float rollVelocity = angVelocity * forwardVector;
	
	const float forwardVelocity = linVelocity * forwardVector;
	const float rightVelocity = linVelocity * rightVector;
	const float upVelocity = linVelocity * upVector;
	const float idealForwardVelocity = moveVelocity * forwardVector;
	const float idealRightVelocity = moveVelocity * rightVector;
	const float idealUpVelocity = moveVelocity * upVector;

	//
	// Evaluate the controls required to reach the destination
	//
	float throttle = 0.f;
	float pitch = 0.f;
	float roll = 0.f;

	// clamp the angles
	idAngles angles = axis.ToAngles();
	angles[0] = idMath::ClampFloat( -angleMax, angleMax, angles[0] );
	angles[2] = idMath::ClampFloat( -angleMax, angleMax, angles[2] );
	SetAngles( angles );

	//
	// Up & Down

	// control the throttle based on the difference between the velocities
	float throttleAngleVelocity = ( idealUpVelocity - upVelocity )* throttleVelScale;
	throttleAngleVelocity = SignedSquare( throttleAngleVelocity );
	throttle = throttleAngleVelocity;

	// Clamp the throttle
	throttle = idMath::ClampFloat( throttleMin, throttleMax, throttle );
	
	// try to cap the velocity 
	if ( fabs( upVelocity ) > maxUpVel ) {
		if ( throttle > throttleMin && upVelocity > 0.0f ) {
			throttle = throttleMin;
		} else if ( throttle < throttleMax && upVelocity < 0.0f ) {
			throttle = throttleMax;
		}
	}

	// dampen the throttle
	throttle = throttle*0.5 + lastThrottle*0.5;
	lastThrottle = throttle;

	//
	// Tilt forwards
	
	// ok! calculate the desired pitch angle from the current stats
	float pitchAngleVelocity = ( idealForwardVelocity - forwardVelocity ) * velocityToAngle;
	pitchAngleVelocity = SignedSquare( pitchAngleVelocity );
	float pitchAngle = pitchAngleVelocity * 0.5;
	
	if ( ( pitchAngle < 0 && pitchVelocity < 0 ) || ( pitchAngle > 0 && pitchVelocity > 0 ) ) {
		pitchAngle += pitchVelocity * directionRecovery;
	}	

	pitchAngle = idMath::ClampFloat( -angleMax, angleMax, pitchAngle );

	// try to cap the velocity 
	if ( fabs( forwardVelocity ) > maxSideVel ) {
		if ( pitchAngle * forwardVelocity > 0.0f ) {
			pitchAngle = 0.0f;
		}
	}
	
	float pitchNeeded = pitchAngle - angles[0];
	pitch = pitchNeeded * pitchNeeded * pitchNeeded * angleToForce;

	//
	// Tilt sideways
	
	// ok! calculate the desired roll angle from the current stats
	float rollAngleVelocity = ( idealRightVelocity - rightVelocity ) * velocityToAngle;
	rollAngleVelocity = SignedSquare( rollAngleVelocity );
	float rollAngle = rollAngleVelocity * 0.5;
	
	if ( ( rollAngle < 0 && rollVelocity < 0 ) || ( rollAngle > 0 && rollVelocity > 0 ) ) {
		rollAngle += rollVelocity * directionRecovery;
	}	

	rollAngle = -idMath::ClampFloat( -angleMax, angleMax, rollAngle );
	
	// try to cap the velocity 
	if ( fabs( rightVelocity ) > maxSideVel ) {
		if ( rollAngle * -rightVelocity > 0.0f ) {
			rollAngle = 0.0f;
		}
	}

	float rollNeeded = rollAngle - angles[2];
	roll = rollNeeded * rollNeeded * rollNeeded * angleToForce;

	// scale based on framerate
//	pitch /= frameDelta * 60.0f;
//	roll /= frameDelta * 60.0f;

	//
	// Calculate resultant forces
	//
	jet1Force = jet2Force = jet3Force = jet4Force = throttle;

	jet1Force -= pitch;
	jet2Force -= pitch;
	jet3Force += pitch;
	jet4Force += pitch;

	jet1Force -= roll;
	jet2Force += roll;
	jet3Force -= roll;
	jet4Force += roll;

	//
	// Cap for flight ceiling
	//
	float height = origin.z;
	if ( height > gameLocal.flightCeilingLower ) {
		if ( height > gameLocal.flightCeilingUpper ) {
			jet1Force = Min( jet1Force, 0.0f );
			jet2Force = Min( jet2Force, 0.0f );
			jet3Force = Min( jet3Force, 0.0f );
			jet4Force = Min( jet4Force, 0.0f );
		}
	}

	//
	// Apply forces
	//
	idVec3 jet1ForceVec = jet1Force * upVector;
	idVec3 jet2ForceVec = jet2Force * upVector;
	idVec3 jet3ForceVec = jet3Force * upVector;
	idVec3 jet4ForceVec = jet4Force * upVector;
	GetPhysics()->AddForce(0, jet1Origin, jet1ForceVec );
	GetPhysics()->AddForce(0, jet2Origin, jet2ForceVec );
	GetPhysics()->AddForce(0, jet3Origin, jet3ForceVec );
	GetPhysics()->AddForce(0, jet4Origin, jet4ForceVec );

	//
	// Too close to ground makeup
	//
	{
		float neededVel = tooCloseToGroundMakeup * invFrameDelta;
		if ( neededVel > 0.0f ) {
			float neededAccel = neededVel - GetPhysics()->GetLinearVelocity().z - GetPhysics()->GetGravity().z * frameDelta;

			float neededForce = neededAccel * GetPhysics()->GetMass() * invFrameDelta;
			idVec3 currentForce = jet1ForceVec+jet2ForceVec+jet3ForceVec+jet4ForceVec;
			float deltaForce = neededForce - currentForce.z;
			if ( deltaForce > 0.0f ) {
				idVec3 worldCoM = GetPhysics()->GetCenterOfMass() + GetPhysics()->GetOrigin();
				GetPhysics()->AddForce( 0, worldCoM, idVec3( 0.0f, 0.0f, deltaForce ) ); 
			}
		}
	}

	// 
	// Print debugging information 
	//
	if ( spawnArgs.GetBool( "debug_info" ) ) {
		idVec4 red( 1.0f, 0.0f, 0.0f, 1.0f );
		idVec4 green( 0.0f, 1.0f, 0.0f, 1.0f );
		idVec4 blue( 0.0f, 0.0f, 1.0f, 1.0f );
		idVec4 yellow( 1.0f, 1.0f, 0.0f, 1.0f );

		float forceScale = spawnArgs.GetFloat( "debug_force_scale" );

		gameRenderWorld->DebugArrow( red, jet1Origin, jet1Origin - ( ( jet1Force * forceScale ) * upVector ), 8, frameDelta );
		gameRenderWorld->DebugArrow( red, jet2Origin, jet2Origin - ( ( jet2Force * forceScale ) * upVector ), 8, frameDelta );
		gameRenderWorld->DebugArrow( red, jet3Origin, jet3Origin - ( ( jet3Force * forceScale ) * upVector ), 8, frameDelta );
		gameRenderWorld->DebugArrow( red, jet4Origin, jet4Origin - ( ( jet4Force * forceScale ) * upVector ), 8, frameDelta );

		gameRenderWorld->DebugCircle( red, moveToOrigin, idVec3( 0.0f, 0.0f, 1.0f ), 32.0f, 18.0f, frameDelta );
		gameRenderWorld->DebugCircle( yellow, desiredOrigin, idVec3( 0.0f, 0.0f, 1.0f ), 32.0f, 18.0f, frameDelta );
		gameRenderWorld->DebugCircle( blue, origin, idVec3( 0.0f, 0.0f, 1.0f ), 32.0f, 18.0f, frameDelta );
	}

	// If moving upwards scale it a bit
	idVec3 velocity = GetPhysics()->GetLinearVelocity();
	if ( velocity.z < idMath::FLT_EPSILON ) {
		velocity.z = 0.0f;
	}

	engineEffect.GetRenderEffect().attenuation = velocity.z * effectVelocityScale;
	engineEffect.GetRenderEffect().origin = GetPhysics()->GetOrigin();
	engineEffect.GetRenderEffect().axis[0] = GetPhysics()->GetAxis()[2];
	engineEffect.GetRenderEffect().axis[2] = GetPhysics()->GetAxis()[0];
	engineEffect.GetRenderEffect().axis[1] = GetPhysics()->GetAxis()[1];
	engineEffect.Update();
}

/*
================
sdRepairDrone::Event_SetEffectOrigins
================
*/
void sdRepairDrone::Event_SetEffectOrigins( const idVec3& start, const idVec3& end, bool active ) {
	effectStart = start;
	effectEnd = end;
	effectActive = active;
}

/*
================
sdRepairDrone::Event_Disable
================
*/
void sdRepairDrone::Event_Disable( void ) {
	sdPhysics_SimpleRigidBody* rbPhysics = GetPhysics()->Cast< sdPhysics_SimpleRigidBody >();
	if ( rbPhysics != NULL ) {
		// make it bounce a little & stop
		rbPhysics->SetBouncyness( 0.2f, 0.2f, 0.0f );
		rbPhysics->SetStopSpeed( 40.0f );
	}
}

/*
================
sdRepairDrone::UpdateEffects
================
*/
void sdRepairDrone::UpdateEffects() {
	if ( effectActive ) {
		repairBeamEffect.GetRenderEffect().origin = effectStart;
		repairBeamEffect.GetRenderEffect().endOrigin = effectEnd;
		repairBeamEffect.Start( gameLocal.time );
		repairBeamEffect.Update();
	} else {
		repairBeamEffect.Stop();
	}
}

/*
================
sdRepairDrone::UpdateWaterDamage
================
*/
void sdRepairDrone::UpdateWaterDamage() {
	if ( lastWaterDamageTime > gameLocal.time - 1000 ) {
		return;
	}

	lastWaterDamageTime = gameLocal.time;

	if ( waterDamageDecl && GetPhysics()->InWater() > 0.5f ) {

		if( submergeTime == 0 ) {
			submergeTime = gameLocal.time + 3000;
		} else if( gameLocal.time > submergeTime ) {
			submergeTime = gameLocal.time + 3000;
			Damage( NULL, this, vec3_origin, waterDamageDecl, 1.0f, NULL );
		}

	} else {
		submergeTime = 0;
	}
}

/*
================
sdRepairDrone::Event_SetEntities
================
*/
void sdRepairDrone::Event_SetEntities( idEntity* repair, idEntity* owner ) {
	repairTarget = repair;
	ownerEntity = owner;
}

/*
================
sdRepairDrone::Event_GetRepairTarget
================
*/
void sdRepairDrone::Event_GetRepairTarget( void ) {
	sdProgram::ReturnEntity( repairTarget );
}

/*
================
sdRepairDrone::Event_GetOwnerEntity
================
*/
void sdRepairDrone::Event_GetOwnerEntity( void ) {
	sdProgram::ReturnEntity( ownerEntity );
}

/*
================
sdRepairDrone::Event_HideThrusters
================
*/
void sdRepairDrone::Event_HideThrusters( void ) {
	engineEffect.Stop();
}

/*
================
sdRepairDrone::ApplyNetworkState
================
*/
void sdRepairDrone::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdRepairDroneBroadcastData );

		repairTarget = newData.repairTarget;
		ownerEntity = newData.ownerEntity;
	}

	sdScriptEntity::ApplyNetworkState( mode, newState );
}

/*
==============
sdRepairDrone::ReadNetworkState
==============
*/
void sdRepairDrone::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdRepairDroneBroadcastData );

		// read state
//		newData.active = msg.ReadBool();
		int spawnId;
		spawnId = msg.ReadBits( 32 );
		newData.repairTarget = gameLocal.EntityForSpawnId( spawnId );
		spawnId = msg.ReadBits( 32 );
		newData.ownerEntity = gameLocal.EntityForSpawnId( spawnId );	
	}

	sdScriptEntity::ReadNetworkState( mode, baseState, newState, msg );
}

/*
==============
sdRepairDrone::WriteNetworkState
==============
*/
void sdRepairDrone::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdRepairDroneBroadcastData );

		// update state
		newData.repairTarget = repairTarget;
		newData.ownerEntity = ownerEntity;

		// write state
		msg.WriteBits( gameLocal.GetSpawnId( repairTarget ), 32 );
		msg.WriteBits( gameLocal.GetSpawnId( ownerEntity ), 32 );
	}

	sdScriptEntity::WriteNetworkState( mode, baseState, newState, msg );
}

/*
==============
sdRepairDrone::CheckNetworkStateChanges
==============
*/
bool sdRepairDrone::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdRepairDroneBroadcastData );

		if ( baseData.repairTarget != repairTarget ) {
			return true;
		}
		if ( baseData.ownerEntity != ownerEntity ) {
			return true;
		}
	}
	return sdScriptEntity::CheckNetworkStateChanges( mode, baseState );
}

/*
==============
sdRepairDrone::CreateNetworkStructure
==============
*/
sdEntityStateNetworkData* sdRepairDrone::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_BROADCAST ) {
		return new sdRepairDroneBroadcastData();
	}
	return sdScriptEntity::CreateNetworkStructure( mode );
}
