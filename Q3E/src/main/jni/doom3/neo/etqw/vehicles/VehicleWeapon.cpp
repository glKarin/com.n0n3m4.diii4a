// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "VehicleWeapon.h"
#include "Transport.h"
#include "../Weapon.h"
#include "../Player.h"
#include "../Projectile.h"
#include "../ContentMask.h"
#include "../Misc.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"

// Gordon: FIXME: Move all IK chains out to vehicle IK so weapons only deal with weapon issues
/*
===============================================================================

	sdVehicleWeapon

===============================================================================
*/

extern const idEventDef EV_GetVehicle;
extern const idEventDef EV_SetState;

const idEventDef EV_VehicleWeapon_GetPlayer( "getPlayer", 'e', DOC_TEXT( "Returns the player currently manning the weapon, or $null$ if none." ), 0, NULL );
const idEventDef EV_VehicleWeapon_SetTarget( "setTarget", '\0', DOC_TEXT( "Enables/disables fixed position aiming, and sets a fixed position for the weapon to aim at." ), 2, NULL, "v", "position", "Fixed position in the world to aim at.", "b", "state", "Enable/disable fixed aiming." );

ABSTRACT_DECLARATION( idClass, sdVehicleWeapon )
	EVENT( EV_GetKey,								sdVehicleWeapon::Event_GetKey )
	EVENT( EV_GetFloatKey,							sdVehicleWeapon::Event_GetFloatKey )
	EVENT( EV_GetVectorKey,							sdVehicleWeapon::Event_GetVectorKey )
	EVENT( EV_GetVehicle,							sdVehicleWeapon::Event_GetVehicle )
	EVENT( EV_VehicleWeapon_GetPlayer,				sdVehicleWeapon::Event_GetPlayer )
	EVENT( EV_SetState,								sdVehicleWeapon::Event_SetState )
	EVENT( EV_VehicleWeapon_SetTarget,				sdVehicleWeapon::Event_SetTarget )
END_CLASS

/*
================
sdVehicleWeapon::Event_GetKey
================
*/
void sdVehicleWeapon::Event_GetKey( const char* key ) {
	sdProgram::ReturnString( GetSpawnParms().GetString( key ) );
}

/*
================
sdVehicleWeapon::Event_GetFloatKey
================
*/
void sdVehicleWeapon::Event_GetFloatKey( const char* key ) {
	sdProgram::ReturnFloat( GetSpawnParms().GetFloat( key ) );
}

/*
================
sdVehicleWeapon::Event_GetVectorKey
================
*/
void sdVehicleWeapon::Event_GetVectorKey( const char* key ) {
	sdProgram::ReturnVector( GetSpawnParms().GetVector( key ) );
}

/*
================
sdVehicleWeapon::Event_GetVehicle
================
*/
void sdVehicleWeapon::Event_GetVehicle( void ) {	
	sdProgram::ReturnEntity( vehicle );
}

/*
================
sdVehicleWeapon::Event_GetPlayer
================
*/
void sdVehicleWeapon::Event_GetPlayer( void ) {	
	sdProgram::ReturnEntity( GetPlayer() );
}

/*
================
sdVehicleWeapon::Event_SetState
================
*/
void sdVehicleWeapon::Event_SetState( const char* state ) {
	Script_SetState( scriptObject->GetFunction( state ) );
}

/*
================
sdVehicleWeapon::Event_SetTarget
================
*/
void sdVehicleWeapon::Event_SetTarget( const idVec3& target, bool state ) {
	SetTarget( target, state );
}


/*
================
sdVehicleWeapon::CreateScriptThread
================
*/
sdProgramThread* sdVehicleWeapon::CreateScriptThread( const sdProgram::sdFunction* function ) {
	scriptIdealState = function;
	scriptState = function;

	sdProgramThread* thread = gameLocal.program->CreateThread();
	thread->SetName( va( "%s_%s", basePosition->GetTransport()->GetName(), GetName() ) );
	thread->CallFunction( scriptObject, function );
	thread->ManualControl();
	thread->ManualDelete();
	return thread;
}

/*
================
sdVehicleWeapon::ConstructScriptObject
================
*/
void sdVehicleWeapon::ConstructScriptObject( void ) {
	const char* weaponScript = GetSpawnParms().GetString( "scriptobject" );
	if ( !*weaponScript ) {
		gameLocal.Warning( "sdVehicleWeapon::ConstructScriptObject No Script Object Specified" );
		return;
	}

	scriptObject = gameLocal.program->AllocScriptObject( this, weaponScript );

	sdScriptHelper h2;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetSyncFunc(), h2 );

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetPreConstructor(), h1 );

	if ( scriptThread != NULL ) {
		gameLocal.Warning( "sdVehicleWeapon::ConstructScriptObject SetState Called before object construction is complete" );
	} else {
		const sdProgram::sdFunction* constructor = scriptObject->GetConstructor();
		if ( constructor ) {
			scriptThread = CreateScriptThread( constructor );
		}
	}
}

/*
================
sdVehicleWeapon::DeconstructScriptObject
================
*/
void sdVehicleWeapon::DeconstructScriptObject( void ) {
	if ( scriptObject == NULL ) {
		return;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetDestructor(), h1 );
	gameLocal.program->FreeScriptObject( scriptObject );
}


/*
================
sdVehicleWeapon::Setup
================
*/
bool sdVehicleWeapon::Setup( sdTransport* _vehicle, const sdDeclStringMap& weaponParms, const angleClamp_t& clampYaw, const angleClamp_t& clampPitch ) {
	vehicle = _vehicle;

	name = weaponParms.GetDict().GetString( "weapon_name" );
	noTophatCrosshair = weaponParms.GetDict().GetBool( "no_tophat_crosshair" );

	lockInfo.Load( weaponParms.GetDict() );

	if ( !Spawn( weaponParms, clampYaw, clampPitch ) ) {
		return false;
	}

	ConstructScriptObject();

	const idDict& weaponDict = weaponParms.GetDict();

	gunJointHandle = vehicle->GetAnimator()->GetJointHandle( weaponDict.GetString( "muzzle", "" ) );

	if ( gunJointHandle == INVALID_JOINT ) {
		gunJointHandle = vehicle->GetAnimator()->GetJointHandle( weaponDict.GetString( "joint_muzzle", "" ) );
		if ( gunJointHandle == INVALID_JOINT ) {
			gunJointHandle = vehicle->GetAnimator()->GetJointHandle( weaponDict.GetString( "muzzle_right", "" ) );
		}
	}

	weaponReadyFunc = scriptObject->GetFunction( "WeaponCanFire" );

	gameLocal.ParseClamp( lockClampYaw, "lock_clamp_yaw", weaponDict );
	gameLocal.ParseClamp( lockClampPitch, "lock_clamp_pitch", weaponDict );

	return true;
}

/*
=====================
sdVehicleWeapon::IsValidLockDirection
=====================
*/
bool sdVehicleWeapon::IsValidLockDirection( const idVec3& worldDirection ) const {
	idVec3 localDirection = vehicle->GetAxis().TransposeMultiply( worldDirection );
	idAngles localAngles = localDirection.ToAngles();
	localAngles.Normalize180();

	if ( lockClampYaw.flags.enabled 
		&& ( localAngles.yaw < lockClampYaw.extents[ 0 ] || localAngles.yaw > lockClampYaw.extents[ 1 ] ) ) {
		return false;
	}

	if ( lockClampPitch.flags.enabled 
		&& ( localAngles.pitch < lockClampPitch.extents[ 0 ] || localAngles.pitch > lockClampPitch.extents[ 1 ] ) ) {
		return false;
	}
	
	return true;
}

/*
=====================
sdVehicleWeapon::GetWeaponOriginAxis
=====================
*/
void sdVehicleWeapon::GetWeaponOriginAxis( idVec3& org, idMat3& axis ) {
	vehicle->GetWorldOriginAxis( gunJointHandle, org, axis );
}

/*
=====================
sdVehicleWeapon::SetPosition
=====================
*/
void sdVehicleWeapon::SetPosition( sdVehiclePosition* _position ) {
	position = _position;

	if ( position != NULL && position != basePosition ) {
		assert( false );
		gameLocal.Warning( "sdVehicleWeapon::SetPosition Weapon '%s' Assigned to bad Position '%s'", GetName(), position->GetName() );
	}

	OnPositionPlayerChanged();
}

/*
=====================
sdVehicleWeapon::OnPositionPlayerChanged
=====================
*/
void sdVehicleWeapon::OnPositionPlayerChanged( void ) {
	sdScriptHelper helper;
	if ( !position || !position->GetPlayer() ) {
		helper.Push( static_cast< idScriptObject* >( NULL ) );
	} else {
		helper.Push( position->GetPlayer()->GetScriptObject() );
	}

	if ( scriptObject ) {
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnSetPlayer" ), helper );
	}
}

/*
=====================
sdVehicleWeapon::SetState
=====================
*/
bool sdVehicleWeapon::SetState( void ) {
	if ( scriptIdealState == NULL ) {
		gameLocal.Error( "sdVehicleWeapon::SetState NULL state" );
	}

	scriptState = scriptIdealState;
	scriptThread->CallFunction( scriptObject, scriptState );

	return true;
}

/*
=====================
sdVehicleWeapon::UpdateScript
=====================
*/
void sdVehicleWeapon::UpdateScript( void ) {
	if ( scriptThread == NULL || gameLocal.IsPaused() ) {
		return;
	}

	// a series of state changes can happen in a single frame.
	// this loop limits them in case we've entered an infinite loop.
	for( int i = 0; i < 20; i++ ) {
		if ( scriptIdealState != scriptState ) {
			SetState();
		}

		// don't call script until it's done waiting
		if ( scriptThread->IsWaiting() ) {
			return;
		}

		if ( scriptThread->Execute() ) {
			if ( scriptIdealState == scriptState ) {
				gameLocal.program->FreeThread( scriptThread );
				scriptThread = NULL;
				return;
			}
		} else if ( scriptIdealState == scriptState ) {
			return;
		}
	}

	scriptThread->Warning( "sdVehicleWeapon::UpdateScript Exited Loop to Prevent Lockup" );
}

/*
================
sdVehicleWeapon::Update
================
*/
void sdVehicleWeapon::Update( void ) {
	if ( gameLocal.isNewFrame ) {
		UpdateScript();
	}
}

/*
================
sdVehicleWeapon::Script_SetState
================
*/
void sdVehicleWeapon::Script_SetState( const sdProgram::sdFunction* function ) {
	scriptIdealState = function;
	if ( scriptThread != NULL ) {
		scriptThread->DoneProcessing();
	} else {
		scriptThread = CreateScriptThread( scriptIdealState );
	}
}

/*
================
sdVehicleWeapon::sdVehicleWeapon
================
*/
sdVehicleWeapon::sdVehicleWeapon( void ) {
	position			= NULL;
	scriptThread		= NULL;
	scriptIdealState	= NULL;
	scriptState			= NULL;
	scriptObject		= NULL;
	weaponReadyFunc		= NULL;
	gunJointHandle		= INVALID_JOINT;
}

/*
================
sdVehicleWeapon::~sdVehicleWeapon
================
*/
sdVehicleWeapon::~sdVehicleWeapon( void ) {
	if ( scriptThread != NULL ) {
		gameLocal.program->FreeThread( scriptThread );
		scriptThread = NULL;
	}

	DeconstructScriptObject();
}

/*
================
sdVehicleWeapon::Spawn
================
*/
bool sdVehicleWeapon::Spawn( const sdDeclStringMap& weaponParms, const angleClamp_t& clampYaw, const angleClamp_t& clampPitch ) {
	spawnWeaponParms = &weaponParms;
	gunName = declHolder.FindLocStr( weaponParms.GetDict().GetString( "gunName" ) );
	return true;
}

/*
===============
sdVehicleWeapon::IsWeaponReady
===============
*/
bool sdVehicleWeapon::IsWeaponReady( void ) {
	if ( !weaponReadyFunc || scriptObject == NULL ) {
		return false;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( weaponReadyFunc, h1 );
	return gameLocal.program->GetReturnedBoolean();
}

/*
================
sdVehicleWeapon::GetPlayer
================
*/
idPlayer* sdVehicleWeapon::GetPlayer( void ) {
	return position ? position->GetPlayer() : NULL;
}

/*
================
sdVehicleWeapon::GetEnemy
================
*/
idEntity* sdVehicleWeapon::GetEnemy( void ) {
	idPlayer* player = GetPlayer();
	if ( player != NULL ) {
		return NULL;
	}
	
	return player->GetTargetLocked() ? player->targetEntity.GetEntity() : NULL;
}

/*
===============================================================================

	sdVehicleWeaponFixedMinigun

===============================================================================
*/

CLASS_DECLARATION( sdVehicleWeapon, sdVehicleWeaponFixedMinigun )
END_CLASS

/*
================
sdVehicleWeaponFixedMinigun::Spawn
================
*/
bool sdVehicleWeaponFixedMinigun::Spawn( const sdDeclStringMap& weaponParms, const angleClamp_t& clampYaw, const angleClamp_t& clampPitch ) {
	if( !sdVehicleWeapon::Spawn( weaponParms, clampYaw, clampPitch ) ) {
		return false;
	}

	const idDict& weaponDict = weaponParms.GetDict();

	idAnimator* animator	= vehicle->GetAnimator();

	jointHandle_t shoulderJoint	= animator->GetJointHandle( weaponDict.GetString( "gunJointShoulder", "" ) );

	jointHandle_t yawJoint		= animator->GetJointHandle( weaponDict.GetString( "gunJointYaw", "" ) );
	if ( yawJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleWeaponFixedMinigun::Spawn Invalid Yaw Joint" );
	}
	jointHandle_t pitchJoint	= animator->GetJointHandle( weaponDict.GetString( "gunJointPitch", "" ) );
	if ( pitchJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleWeaponFixedMinigun::Spawn Invalid Pitch Joint" );
	}
	jointHandle_t barrelJoint	= animator->GetJointHandle( weaponDict.GetString( "weapon1_muzzle", "" ) );
	if ( barrelJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleWeaponFixedMinigun::Spawn Invalid Muzzle Joint" );
	}

	aimer.Init( weaponDict.GetBool( "fix_barrel" ), weaponDict.GetBool( "invert_pitch" ), vehicle, animator->GetAnim( "base" ), yawJoint, pitchJoint, barrelJoint, shoulderJoint, clampYaw, clampPitch );

	return true;
}

/*
================
sdVehicleWeaponFixedMinigun::Update
================
*/
void sdVehicleWeaponFixedMinigun::Update( void ) {
	sdVehicleWeapon::Update();

	if ( gameLocal.isClient && ( vehicle->aorFlags & AOR_INHIBIT_IK ) ) {
		return;
	}

	idPlayer* player = GetPlayer();

	if ( manualTarget ) {
		aimer.SetTarget( manualTargetPos );
	} else if ( player ) {
		trace_t trace;
		idVec3 end = player->renderView.vieworg + ( 4096 * player->renderView.viewaxis[ 0 ] );

		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, player->renderView.vieworg, end, CONTENTS_SOLID | CONTENTS_OPAQUE, player );

		aimer.SetTarget( trace.endpos );
	} else {
		aimer.ClearTarget();
	}

	aimer.Update();
}

/*
================
sdVehicleWeaponFixedMinigun::SetTarget
================
*/
void sdVehicleWeaponFixedMinigun::SetTarget( const idVec3& target, bool enabled ) {
	manualTarget	= enabled;
	manualTargetPos	= target;
}

/*
================
sdVehicleWeaponFixedMinigun::sdVehicleWeaponFixedMinigun
================
*/
sdVehicleWeaponFixedMinigun::sdVehicleWeaponFixedMinigun( void ) {
	manualTarget = false;
}

/*
================
sdVehicleWeaponFixedMinigun::sdVehicleWeaponFixedMinigun
================
*/
sdVehicleWeaponFixedMinigun::~sdVehicleWeaponFixedMinigun( void ) {
}

/*
================
sdVehicleWeaponFixedMinigun::CanAimAt
================
*/
bool sdVehicleWeaponFixedMinigun::CanAimAt( const idVec3& idealAimPosition ) {
	idPlayer* player = position->GetPlayer();
	sdTransport* transport = position->GetTransport();
	assert( player );
	assert( transport );

	trace_t trace;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, player->firstPersonViewOrigin, idealAimPosition, 
								( MASK_SHOT_RENDERMODEL | MASK_SHOT_BOUNDINGBOX )  & ~CONTENTS_FORCEFIELD, transport );
	idVec3 aimPosition = trace.endpos;

	idVec3 weapOrg;
	idMat3 weapAxis;
	GetWeaponOriginAxis( weapOrg, weapAxis );

	idVec3 aimDirection = aimPosition - weapOrg;
	float aimLength = aimDirection.Normalize();
	if ( aimLength < idMath::FLT_EPSILON ) {
		return false;
	}

	const idMat3& baseAxis = transport->GetAxis();
	idVec3 localAim = baseAxis.TransposeMultiply( aimDirection );
	idAngles aimAngles = localAim.ToAngles();
	aimAngles.Normalize180();

	return aimer.CanAimTo( aimAngles );
}




/*
===============================================================================

	sdVehicleWeaponLocked

===============================================================================
*/

CLASS_DECLARATION( sdVehicleWeapon, sdVehicleWeaponLocked )
END_CLASS

/*
================
sdVehicleWeaponLocked::Spawn
================
*/
bool sdVehicleWeaponLocked::Spawn( const sdDeclStringMap& weaponParms, const angleClamp_t& clampYaw, const angleClamp_t& clampPitch ) {
	if( !sdVehicleWeapon::Spawn( weaponParms, clampYaw, clampPitch ) ) {
		return false;
	}

	const idDict& weaponDict = weaponParms.GetDict();

	// list of joints to 
	idAnimator* animator	= vehicle->GetAnimator();
	for ( int i = 1; i <= MAX_CANAIM_JOINTS; i++ ) {
		const char* jointDefName = va( "canaim_joint_%i", i );
		jointHandle_t joint = animator->GetJointHandle( weaponDict.GetString( jointDefName, "" ) );
		if ( joint == INVALID_JOINT ) {
			break;
		}
		canAimJoints.Append( joint );
	}

	notReallyLocked = weaponDict.GetBool( "not_really_locked" );
	if ( notReallyLocked ) {
		nrl_yawClamp.flags.enabled = weaponDict.GetBool( "nrl_yawClamp_enabled" );
		nrl_yawClamp.extents[ 0 ] = weaponDict.GetFloat( "nrl_yawClamp_min" );
		nrl_yawClamp.extents[ 1 ] = weaponDict.GetFloat( "nrl_yawClamp_max" );
		nrl_pitchClamp.flags.enabled = weaponDict.GetBool( "nrl_pitchClamp_enabled" );
		nrl_pitchClamp.extents[ 0 ] = weaponDict.GetFloat( "nrl_pitchClamp_min" );
		nrl_pitchClamp.extents[ 1 ] = weaponDict.GetFloat( "nrl_pitchClamp_max" );
	}

	return true;
}

/*
================
sdVehicleWeaponLocked::CanAimAt
================
*/
bool sdVehicleWeaponLocked::CanAimAt( const idVec3& idealAimPosition ) {
	idPlayer* player = position->GetPlayer();
	sdTransport* transport = position->GetTransport();
	assert( player );
	assert( transport );

	trace_t trace;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, player->firstPersonViewOrigin, idealAimPosition, 
								( MASK_SHOT_RENDERMODEL | MASK_SHOT_BOUNDINGBOX )  & ~CONTENTS_FORCEFIELD, transport );
	idVec3 aimPosition = trace.endpos;

	idVec3 org;
	idVec3 fwd;
	if ( gunJointHandle != INVALID_JOINT ) {
		idMat3 axis;
		GetWeaponOriginAxis( org, axis );
		fwd = axis[ 0 ];
	} else {
		if ( canAimJoints.Num() ) {
			org.Zero();
			fwd.Zero();
			for ( int i = 0; i < canAimJoints.Num(); i++ ) {
				idVec3 tempOrg;
				idMat3 tempAxis;
				vehicle->GetWorldOriginAxis( canAimJoints[ i ], tempOrg, tempAxis );

				org += tempOrg;
				fwd += tempAxis[ 0 ];
			}

			org /= canAimJoints.Num();
			fwd.Normalize();
		} else {
			idMat3 axis;
			axis = transport->GetAxis();
			org = player->firstPersonViewOrigin;
			fwd = axis[ 0 ];
		}
	}

	if ( notReallyLocked ) {
		idVec3 aimDirection = aimPosition - org;
		float aimLength = aimDirection.Normalize();
		if ( aimLength < idMath::FLT_EPSILON ) {
			return false;
		}

		const idMat3& baseAxis = transport->GetAxis();
		idVec3 localAim = baseAxis.TransposeMultiply( aimDirection );
		idAngles aimAngles = localAim.ToAngles();
		aimAngles.Normalize180();

		if ( nrl_yawClamp.flags.enabled ) {
			if ( aimAngles.yaw < nrl_yawClamp.extents[ 0 ] || aimAngles.yaw > nrl_yawClamp.extents[ 1 ] ) {
				return false;
			}
		}
		if ( nrl_pitchClamp.flags.enabled ) {
			if ( aimAngles.pitch < nrl_pitchClamp.extents[ 0 ] || aimAngles.pitch > nrl_pitchClamp.extents[ 1 ] ) {
				return false;
			}
		}
	} else {
		idVec3 aimDirection = aimPosition - org;
		float aimLength = aimDirection.Normalize();
		if ( aimLength < idMath::FLT_EPSILON ) {
			return false;
		}

		if ( aimDirection * fwd < 0.999f ) {
			return false;
		}
	}

	return true;
}

/*
===============================================================================

sdVehicleWeaponFactory

===============================================================================
*/

/*
================
sdVehicleWeaponFactory::GetWeapon
================
*/
sdVehicleWeapon* sdVehicleWeaponFactory::GetWeapon( const char* weaponType ) {
	idTypeInfo* type = idClass::GetClass( weaponType );
	if ( !type ) {
		gameLocal.Error( "sdVehicleWeaponFactory::GetWeapon Invalid Class Type '%s'", weaponType );
	}
	if ( !type->IsType( sdVehicleWeapon::Type ) ) {
		gameLocal.Error( "sdVehicleWeaponFactory::GetWeapon '%s' Is Not Of Type sdVehicleWeapon", weaponType );
	}

	return reinterpret_cast< sdVehicleWeapon* >( type->CreateInstance() );
}
