// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DefenceTurret.h"
#include "../Player.h"
#include "../ContentMask.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../rules/GameRules.h"

/*
===============================================================================

	sdDefenceTurret

===============================================================================
*/

extern const idEventDef EV_GetEnemy;

const idEventDef EV_Turret_SetDisabled( "setDisabled", '\0', DOC_TEXT( "Sets whether the turret is disabled or not." ), 1, NULL, "b", "state", "Whether the turret is disabled." );
const idEventDef EV_Turret_IsDisabled( "isDisabled", 'b', DOC_TEXT( "Returns whether the turret is disabled." ), 0, NULL );
const idEventDef EV_Turret_GetTargetPosition( "getTargetPosition", 'v', DOC_TEXT( "Returns the position in world space where the turret would target the specified entity." ), 1, NULL, "e", "target", "Entity to check the target position of." );
const idEventDef EV_Turret_SetEnemy( "setTurretEnemy", '\0', DOC_TEXT( "Sets the target entity for this object." ), 2, NULL, "E", "target", "New target to set.", "f", "startTurn", "Delay before aiming at target." );

CLASS_DECLARATION( sdScriptEntity, sdDefenceTurret )
	EVENT( EV_Turret_SetDisabled,			sdDefenceTurret::Event_SetDisabled )
	EVENT( EV_Turret_IsDisabled,			sdDefenceTurret::Event_IsDisabled )

	EVENT( EV_Turret_GetTargetPosition,		sdDefenceTurret::Event_GetTargetPosition )

	EVENT( EV_GetEnemy,						sdDefenceTurret::Event_GetEnemy )
	EVENT( EV_Turret_SetEnemy,				sdDefenceTurret::Event_SetEnemy )
END_CLASS

/*
================
sdDefenceTurret::sdDefenceTurret
================
*/
sdDefenceTurret::sdDefenceTurret( void ) : nextTargetAcquireTime( 0 ) {
	turretFlags.disabled	= false;
	turretFlags.hasTarget	= false;
	turretFlags.attacking	= false;

	isDeployedFunc			= NULL;
	deployableType			= NULL_DEPLOYABLE;

	acquireWaitTime			= SEC2MS( 0.5f );
	startTurn				= 0;
}

/*
================
sdDefenceTurret::~sdDefenceTurret
================
*/
sdDefenceTurret::~sdDefenceTurret( void ) {
	SetTargetEntity( NULL );
}

/*
================
sdDefenceTurret::InitAngleInfo
================
*/
void sdDefenceTurret::InitAngleInfo( const char* name, angleClamp_t& info, const sdDeclStringMap* map ) {
	idDict dict = map->GetDict();

	info.filter = 0.f;

	info.sound = gameLocal.declSoundShaderType[ dict.GetString( va( "snd_%s_turn", name ) ) ];

	info.flags.limitRate = false;

	bool minRate = dict.GetFloat( va( "min_%s_turn", name ), "", info.rate[ 0 ] );
	bool maxRate = dict.GetFloat( va( "max_%s_turn", name ), "", info.rate[ 1 ] );
	if ( minRate ) {
		if ( maxRate ) {
			info.flags.limitRate = true;
		} else {
			gameLocal.Error( "sdDefenceTurret::InitAngleInfo Min rate set with no Max rate set in '%s'", map->GetName() );
		}
	} else if ( maxRate ) {
		gameLocal.Error( "sdDefenceTurret::InitAngleInfo Max rate set with no Min rate set in '%s'", map->GetName() );
	}

	info.flags.enabled			= false;	
	bool minLimit	= dict.GetFloat( va( "min_%s", name ), "0", info.extents[ 0 ] );
	bool maxLimit	= dict.GetFloat( va( "max_%s", name ), "0", info.extents[ 1 ] );
	if ( minLimit ) {
		if ( maxLimit ) {
			info.flags.enabled	= true;
		} else {
			gameLocal.Error( "sdDefenceTurret::InitAngleInfo Min limit set with no Max limit set in '%s'", map->GetName() );
		}
	} else if ( maxLimit ) {
		gameLocal.Error( "sdDefenceTurret::InitAngleInfo Max limit set with no Min limit set in '%s'", map->GetName() );
	}
}

/*
================
sdDefenceTurret::Spawn
================
*/
void sdDefenceTurret::Spawn( void ) {
	const char* aimDataName = spawnArgs.GetString( "str_aim_data" );
	const sdDeclStringMap* aimData = gameLocal.declStringMapType[ aimDataName ];
	if ( aimData ) {

		angleClamp_t yawInfo;
		angleClamp_t pitchInfo;

		InitAngleInfo( "yaw", yawInfo, aimData );
		InitAngleInfo( "pitch", pitchInfo, aimData );

		int anim = 1;

		const char* deployedAnim = aimData->GetDict().GetString( "deployed_anim" );
		if ( *deployedAnim ) {
			anim = animator.GetAnim( deployedAnim );
		}

		jointHandle_t yawJoint		= animator.GetJointHandle( aimData->GetDict().GetString( "joint_yaw" ) );
		jointHandle_t pitchJoint	= animator.GetJointHandle( aimData->GetDict().GetString( "joint_pitch" ) );
		jointHandle_t barrelJoint	= animator.GetJointHandle( aimData->GetDict().GetString( "joint_barrel" ) );
		
		aimer.Init( aimData->GetDict().GetBool( "fix_barrel" ), aimData->GetDict().GetBool( "invert_pitch" ), this, anim, yawJoint, pitchJoint, barrelJoint, INVALID_JOINT, yawInfo, pitchInfo );
	} else {
		gameLocal.Error( "sdDefenceTurret::Spawn No Aim Data" );
	}

	isDeployedFunc		= scriptObject->GetFunction( "IsDeployed" );
	getOwnerFunc		= scriptObject->GetFunction( "GetOwner" );
	validateTargetFunc	= scriptObject->GetFunction( "OnValidateTarget" );
	getMinRangeFunc		= scriptObject->GetFunction( "GetTurretMinRange" );
	getMaxRangeFunc		= scriptObject->GetFunction( "GetTurretMaxRange" );

	int temp;

	if ( spawnArgs.GetInt( "deployable_type", "", temp ) ) {
		deployableType = temp;
	}

	postThinkEntNode.AddToEnd( gameLocal.postThinkEntities );
}

/*
================
sdDefenceTurret::PostThink
================
*/
void sdDefenceTurret::PostThink( void ) {
	sdScriptEntity::PostThink();

	if ( !turretFlags.disabled ) {
		UpdateTarget();
		if ( gameLocal.time >= startTurn ) {
			aimer.Update();
		}
	}
}

/*
================
sdDefenceTurret::UpdatePlayerTarget
================
*/
void sdDefenceTurret::UpdatePlayerTarget( idPlayer* player ) {
	idVec3 start		= player->renderView.vieworg;
	idVec3 dir			= player->renderView.viewaxis[ 0 ];

	idVec3 targetPos	= start + ( dir * 4096 );

	trace_t trace;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, start, targetPos, MASK_SHOT_RENDERMODEL, this );
	
	aimer.SetTarget( trace.endpos );
	aimer.LockTarget();

	if ( gameLocal.usercmds[ player->entityNumber ].buttons.btn.attack ) {
		BeginAttack();
	} else {
		EndAttack();
	}
}

/*
================
sdDefenceTurret::ValidateTarget
================
*/
bool sdDefenceTurret::ValidateTarget( void ) const {
	if ( validateTargetFunc == NULL ) {
		return false;
	}

	sdScriptHelper h1;
	return CallFloatNonBlockingScriptEvent( validateTargetFunc, h1 ) != 0.f;
}

/*
================
sdDefenceTurret::GetTurretOwner
================
*/
idEntity* sdDefenceTurret::GetTurretOwner( void ) const {
	if ( getOwnerFunc == NULL ) {
		return NULL;
	}

	sdScriptHelper h1;
	CallNonBlockingScriptEvent( getOwnerFunc, h1 );

	idScriptObject* object = gameLocal.program->GetReturnedObject();	
	if ( object == NULL ) {
		return NULL;
	}

	return object->GetClass()->Cast< idEntity >();
}

/*
================
sdDefenceTurret::GetTurretMaxRange
================
*/
float sdDefenceTurret::GetTurretMaxRange( void ) const {
	if ( getMaxRangeFunc == NULL ) {
		return 0.f;
	}

	sdScriptHelper h1;
	return CallFloatNonBlockingScriptEvent( getMaxRangeFunc, h1 );
}

/*
================
sdDefenceTurret::GetTurretMinRange
================
*/
float sdDefenceTurret::GetTurretMinRange( void ) const {
	if ( getMinRangeFunc == NULL ) {
		return 0.f;
	}

	sdScriptHelper h1;
	return CallFloatNonBlockingScriptEvent( getMinRangeFunc, h1 );
}

/*
================
sdDefenceTurret::IsDeployed
================
*/
bool sdDefenceTurret::IsDeployed( void ) {
	if ( isDeployedFunc == NULL ) {
		return false;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( isDeployedFunc, h1 );
	return gameLocal.program->GetReturnedBoolean();
}

/*
================
sdDefenceTurret::GetTargetEntity
================
*/
idEntity* sdDefenceTurret::GetTargetEntity( void ) const {
	return target;
}

/*
================
sdDefenceTurret::InFiringRange
================
*/
bool sdDefenceTurret::InFiringRange( const idVec3& targetPos ) const {
	sdScriptHelper h1;
	h1.Push( targetPos );
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "InFiringRange" ), h1 );
	return gameLocal.program->GetReturnedBoolean();
}

/*
================
sdDefenceTurret::UpdateTarget
================
*/
void sdDefenceTurret::UpdateTarget( void ) {
	if ( gameLocal.rules->IsEndGame() ) {
		SetTargetEntity( NULL );
		return;
	}

	idPlayer* controller = NULL;
	if ( usableInterface != NULL ) {
		controller = usableInterface->GetBoundPlayer( 0 );
	}

	if ( controller ) {
		UpdatePlayerTarget( controller );
		return;
	}

	if ( turretFlags.hasTarget ) {
		idEntity* targetEntity = target;
		if ( targetEntity == NULL ) {
			SetTargetEntity( NULL );
		} else {
			idVec3 targetPos = GetTargetPosition( targetEntity );
			
			aimer.SetTarget( targetPos );
			
			bool inFireRange = InFiringRange( targetPos );
			if ( inFireRange && aimer.TargetClose() ) {
				BeginAttack();
			} else {
				EndAttack();
			}

			if ( gameLocal.time >= nextTargetAcquireTime ) {
				if ( !ValidateTarget() ) {
					SetTargetEntity( NULL );
					return;
				}

				if ( !inFireRange ) {
					AcquireTarget();
				}

				nextTargetAcquireTime = gameLocal.time + acquireWaitTime;
			}
		}

		return;
	}

	if ( gameLocal.time < nextTargetAcquireTime ) {
		return;
	}

	AcquireTarget();

	nextTargetAcquireTime = gameLocal.time + acquireWaitTime;
}

/*
================
sdDefenceTurret::SetTargetEntity
================
*/
void sdDefenceTurret::SetTargetEntity( idEntity* entity ) {
	idEntity* oldEntity = target;

	if ( target.GetSpawnId() == gameLocal.GetSpawnId( entity ) ) {
		return;
	}

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_SETTARGET );
		msg.WriteLong( gameLocal.GetSpawnId( entity ) );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}

	target = entity;

	if ( entity != NULL ) {
		turretFlags.hasTarget = true;
	} else {
		turretFlags.hasTarget = false;
		aimer.ClearTarget();
		EndAttack();
	}

	sdScriptHelper h1;
	h1.Push( oldEntity ? oldEntity->GetScriptObject() : NULL );
	h1.Push( entity ? entity->GetScriptObject() : NULL );
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnSetTarget" ), h1 );
}

/*
================
sdDefenceTurret::OnPlayerEntered
================
*/
void sdDefenceTurret::OnPlayerEntered( idPlayer* player, int index ) {
	if ( index != 0 ) {
		return;
	}

	SetTargetEntity( NULL );
}

/*
================
sdDefenceTurret::OnPlayerExited
================
*/
void sdDefenceTurret::OnPlayerExited( idPlayer* player, int index ) {
	if ( index != 0 ) {
		return;
	}

	aimer.ClearTarget();
	EndAttack();
}

/*
================
sdDefenceTurret::DamageFeedback
================
*/
void sdDefenceTurret::DamageFeedback( idEntity *victim, idEntity *inflictor, int oldHealth, int newHealth, const sdDeclDamage* damageDecl, bool headshot ) {
	if ( usableInterface != NULL ) {
		idPlayer* user = usableInterface->GetBoundPlayer( 0 );	
		if ( user != NULL ) {
			user->DamageFeedback( victim, inflictor, oldHealth, newHealth, damageDecl, headshot );
		}
	}
}

/*
================
sdDefenceTurret::AcquireTarget
================
*/
void sdDefenceTurret::AcquireTarget( void ) {
	if ( gameLocal.isClient ) {
		return;
	}

	scriptObject->CallEvent( "OnAcquireTarget" );

	idScriptObject* object = gameLocal.program->GetReturnedObject();
	if ( !object ) {
		return;
	}

	idEntity* entity = object->GetClass()->Cast< idEntity >();
	if ( !entity ) {
		return;
	}

	SetTargetEntity( entity );
}

/*
================
sdDefenceTurret::EndAttack
================
*/
void sdDefenceTurret::EndAttack( void ) {
	if ( !turretFlags.attacking ) {
		return;
	}

	turretFlags.attacking = false;

	scriptObject->CallEvent( "OnEndAttack" );
}

/*
================
sdDefenceTurret::BeginAttack
================
*/
void sdDefenceTurret::BeginAttack( void ) {
	if ( turretFlags.attacking ) {
		return;
	}

	turretFlags.attacking = true;

	scriptObject->CallEvent( "OnBeginAttack" );
}

/*
================
sdDefenceTurret::SetDisabled
================
*/
void sdDefenceTurret::SetDisabled( bool value ) {
	if ( value == turretFlags.disabled ) {
		return;
	}
	turretFlags.disabled = value;

	if ( turretFlags.disabled ) {	
		EndAttack();
		SetTargetEntity( NULL );
	}
}

/*
================
sdDefenceTurret::IsDisabled
================
*/
bool sdDefenceTurret::IsDisabled( void ) {
	return turretFlags.disabled;
}

/*
================
sdDefenceTurret::GetTargetPosition
================
*/
idVec3 sdDefenceTurret::GetTargetPosition( idEntity* entity ) {
	idPhysics* physics	= entity->GetPhysics();
	return physics->GetOrigin() + ( physics->GetBounds().GetCenter() * physics->GetAxis() );
}

/*
================
sdDefenceTurret::Event_SetDisabled
================
*/
void sdDefenceTurret::Event_SetDisabled( bool value ) {
	SetDisabled( value );
}

/*
================
sdDefenceTurret::Event_IsDisabled
================
*/
void sdDefenceTurret::Event_IsDisabled( void ) {
	sdProgram::ReturnBoolean( turretFlags.disabled );
}

/*
================
sdDefenceTurret::Event_GetTargetPosition
================
*/
void sdDefenceTurret::Event_GetTargetPosition( idEntity* entity ) {
	sdProgram::ReturnVector( GetTargetPosition( entity ) );
}

/*
================
sdDefenceTurret::Event_GetEnemy
================
*/
void sdDefenceTurret::Event_GetEnemy( void ) {
	sdProgram::ReturnEntity( target );
}

/*
================
sdDefenceTurret::Event_SetEnemy
================
*/
void sdDefenceTurret::Event_SetEnemy( idEntity* other, float _turnDelay ) {
	if ( gameLocal.isClient ) {
		return;
	}

	startTurn = gameLocal.time + SEC2MS( _turnDelay );
	SetTargetEntity( other );
}

/*
================
sdDefenceTurret::ClientReceiveEvent
================
*/
bool sdDefenceTurret::ClientReceiveEvent( int event, int time, const idBitMsg& msg ) {
	switch ( event ) {
		case EVENT_SETTARGET:
			int spawnId = msg.ReadLong();
			SetTargetEntity( gameLocal.EntityForSpawnId( spawnId ) );
			return true;
	}

	return sdScriptEntity::ClientReceiveEvent( event, time, msg );
}

/*
===============
sdDefenceTurret::WriteDemoBaseData
==============
*/
void sdDefenceTurret::WriteDemoBaseData( idFile* file ) const {
	idEntity::WriteDemoBaseData( file );

	file->WriteInt( target.GetSpawnId() );
}

/*
===============
sdDefenceTurret::ReadDemoBaseData
==============
*/
void sdDefenceTurret::ReadDemoBaseData( idFile* file ) {
	idEntity::ReadDemoBaseData( file );

	int temp;
	file->ReadInt( temp );
	target.ForceSpawnId( temp );
}
