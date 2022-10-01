
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

#include "AI.h"

#ifndef __GAME_PROJECTILE_H__
#include "../Projectile.h"
#endif

#include "AI_Manager.h"
#include "../vehicle/Vehicle.h"
#include "../spawner.h"


#define idAI_EVENT( eventname, inparms, outparm)					\
	const idEventDef AI_##eventname( #eventname, inparms, outparm);	\
	void idAI::Event_##eventname


/***********************************************************************

	AI Events

***********************************************************************/

// Get / Set
const idEventDef AI_GetLeader						( "getLeader", NULL, 'e' );
const idEventDef AI_SetLeader						( "setLeader", "E" );
const idEventDef AI_GetEnemy						( "getEnemy", NULL, 'e' );
const idEventDef AI_SetEnemy						( "setEnemy", "E" );
const idEventDef AI_SetHealth						( "setHealth", "f" );
const idEventDef AI_SetTalkState					( "setTalkState", "d" );
const idEventDef AI_SetScript						( "setScript", "ss" );
const idEventDef AI_SetMoveSpeed					( "setMoveSpeed", "d" );
const idEventDef AI_SetPassivePrefix				( "setPassivePrefix", "s" );

// Enable / Disable
const idEventDef AI_EnableClip						( "enableClip" );
const idEventDef AI_DisableClip						( "disableClip" );
const idEventDef AI_EnableGravity					( "enableGravity" );
const idEventDef AI_DisableGravity					( "disableGravity" );
const idEventDef AI_EnableAFPush					( "enableAFPush" );
const idEventDef AI_DisableAFPush					( "disableAFPush" );
const idEventDef AI_EnableDamage					( "enableDamage" );
const idEventDef AI_DisableDamage					( "disableDamage" );
const idEventDef AI_EnableTarget					( "enableTarget" );
const idEventDef AI_DisableTarget					( "disableTarget" );
const idEventDef AI_EnableMovement					( "enableMovement" );
const idEventDef AI_DisableMovement					( "disableMovement" );
const idEventDef AI_TakeDamage						( "takeDamage", "f" );
const idEventDef AI_SetUndying						( "setUndying", "f" );
const idEventDef AI_EnableAutoBlink					( "enableAutoBlink" );
const idEventDef AI_DisableAutoBlink				( "disableAutoBlink" );

// Scripted Sequences
const idEventDef AI_ScriptedMove					( "scriptedMove", "efd" );
const idEventDef AI_ScriptedFace					( "scriptedFace", "ed" );
const idEventDef AI_ScriptedAnim					( "scriptedAnim", "sddd" );
const idEventDef AI_ScriptedPlaybackMove			( "scriptedPlaybackMove", "sdd" );
const idEventDef AI_ScriptedPlaybackAim				( "scriptedPlaybackAim", "sdd" );
const idEventDef AI_ScriptedAction					( "scriptedAction", "ed" );
const idEventDef AI_ScriptedDone					( "scriptedDone", NULL, 'd' );
const idEventDef AI_ScriptedStop					( "scriptedStop" );
const idEventDef AI_ScriptedJumpDown				( "scriptedJumpDown", "f" );

// Misc
const idEventDef AI_LookAt							( "lookAt", "E" );
const idEventDef AI_Attack							( "attack", "ss" );
const idEventDef AI_LockEnemyOrigin					( "lockEnemyOrigin" );
const idEventDef AI_StopThinking					( "stopThinking" );
const idEventDef AI_JumpFrame						( "<jumpframe>" );
const idEventDef AI_RealKill						( "<kill>" );
const idEventDef AI_Kill							( "kill" );
const idEventDef AI_RemoveUpdateSpawner				( "removeUpdateSpawner" );
const idEventDef AI_AllowHiddenMovement				( "allowHiddenMovement", "d" );
const idEventDef AI_Speak							( "speak", "s" );
const idEventDef AI_SpeakRandom						( "speakRandom", "s" );
const idEventDef AI_IsSpeaking						( "isSpeaking", NULL, 'f' );
const idEventDef AI_IsTethered						( "isTethered", NULL, 'f' );
const idEventDef AI_IsWithinTether					( "isWithinTether", NULL, 'f' );
const idEventDef AI_LaunchMissile					( "launchMissile", "vv", 'e' );
const idEventDef AI_AttackMelee						( "attackMelee", "s", 'd' );
const idEventDef AI_DirectDamage					( "directDamage", "es" );
const idEventDef AI_RadiusDamageFromJoint			( "radiusDamageFromJoint", "ss" );
const idEventDef AI_MeleeAttackToJoint				( "meleeAttackToJoint", "ss", 'd' );
const idEventDef AI_CanBecomeSolid					( "canBecomeSolid", NULL, 'f' );
const idEventDef AI_BecomeSolid						( "becomeSolid" );
const idEventDef AI_BecomeRagdoll					( "becomeRagdoll", NULL, 'd' );
const idEventDef AI_BecomePassive					( "becomePassive", "d" );
const idEventDef AI_BecomeAggressive				( "becomeAggressive" );
const idEventDef AI_StopRagdoll						( "stopRagdoll" );
const idEventDef AI_FaceEnemy						( "faceEnemy" );
const idEventDef AI_FaceEntity						( "faceEntity", "E" );

//jshepard
const idEventDef AI_FindEnemy						( "findEnemy", "f", 'e');

void idAI::Event_Activate( idEntity *activator )											{ Activate( activator );}
void idAI::Event_Touch( idEntity *other, trace_t *trace )									{ OnTouch( other, trace ); }
void idAI::Event_SetEnemy( idEntity *ent )													{ if ( !ent ) ClearEnemy(); else SetEnemy( ent );}
void idAI::Event_DirectDamage( idEntity *damageTarget, const char *damageDefName )			{ DirectDamage( damageDefName, damageTarget ); }
void idAI::Event_RadiusDamageFromJoint( const char *jointname, const char *damageDefName )	{ RadiusDamageFromJoint( jointname, damageDefName ); }
void idAI::Event_CanBecomeSolid( void )														{ idThread::ReturnFloat( CanBecomeSolid() ); }
void idAI::Event_BecomeSolid( void )														{ BecomeSolid(); }
void idAI::Event_BecomeNonSolid( void )														{ BecomeNonSolid(); }
void idAI::Event_BecomeRagdoll( void )														{ idThread::ReturnInt( StartRagdoll() ); }
void idAI::Event_StopRagdoll( void )														{ StopRagdoll(); SetPhysics( &physicsObj ); }
void idAI::Event_SetHealth( float newHealth )												{ health = newHealth; fl.takedamage = true; if( health > 0 ) aifl.dead = false; else aifl.dead = true; }
void idAI::Event_FaceEnemy( void )															{ FaceEnemy(); }
void idAI::Event_FaceEntity( idEntity *ent )												{ FaceEntity( ent ); }
void idAI::Event_SetTalkState( int state )													{ SetTalkState ( (talkState_t)state );  }
void idAI::Event_Speak( const char *speechDecl )											{ Speak( speechDecl ); }
void idAI::Event_SpeakRandom( const char *speechDecl )										{ Speak( speechDecl, true ); }
void idAI::Event_GetLeader( void )															{ idThread::ReturnEntity( leader ); }
void idAI::Event_SetLeader( idEntity* ent )													{ SetLeader ( ent ); }
void idAI::Event_GetEnemy( void )															{ idThread::ReturnEntity( enemy.ent ); }
void idAI::Event_TakeDamage( float takeDamage )												{ fl.takedamage = ( takeDamage ) ? true : false; }
void idAI::Event_SetUndying( float setUndying )												{ aifl.undying = ( setUndying ) ? true : false; }


void idAI::Event_IsSpeaking ( void )														{ idThread::ReturnFloat ( IsSpeaking ( ) ); }
void idAI::Event_IsTethered ( void )														{ idThread::ReturnFloat ( IsTethered ( ) ); }
void idAI::Event_IsWithinTether ( void )													{ idThread::ReturnFloat ( IsWithinTether ( ) ); }
void idAI::Event_IsMoving ( void )															{ idThread::ReturnFloat ( move.fl.moving  ); }

CLASS_DECLARATION( idActor, idAI )
	EVENT( EV_Activate,							idAI::Event_Activate )
	EVENT( EV_Touch,							idAI::Event_Touch )
	
	// Enable / Disable
	EVENT( AI_EnableClip,						idAI::Event_EnableClip )
	EVENT( AI_DisableClip,						idAI::Event_DisableClip )
	EVENT( AI_EnableGravity,					idAI::Event_EnableGravity )
	EVENT( AI_DisableGravity,					idAI::Event_DisableGravity )
	EVENT( AI_EnableAFPush,						idAI::Event_EnableAFPush )
	EVENT( AI_DisableAFPush,					idAI::Event_DisableAFPush )
	EVENT( AI_EnableDamage,						idAI::Event_EnableDamage )
	EVENT( AI_DisableDamage,					idAI::Event_DisableDamage )
	EVENT( AI_EnablePain,						idAI::Event_EnablePain )
	EVENT( AI_DisablePain,						idAI::Event_DisablePain )
	EVENT( AI_EnableTarget,						idAI::Event_EnableTarget )
	EVENT( AI_DisableTarget,					idAI::Event_DisableTarget )
	EVENT( AI_TakeDamage,						idAI::Event_TakeDamage )
	EVENT( AI_SetUndying,						idAI::Event_SetUndying )
	EVENT( AI_EnableAutoBlink,					idAI::Event_EnableAutoBlink )
	EVENT( AI_DisableAutoBlink,					idAI::Event_DisableAutoBlink )

	// Scripted sequences
	EVENT( AI_ScriptedMove,						idAI::Event_ScriptedMove )
	EVENT( AI_ScriptedFace,						idAI::Event_ScriptedFace )
	EVENT( AI_ScriptedAnim,						idAI::Event_ScriptedAnim )
	EVENT( AI_ScriptedAction,					idAI::Event_ScriptedAction )
	EVENT( AI_ScriptedPlaybackMove,				idAI::Event_ScriptedPlaybackMove )
	EVENT( AI_ScriptedPlaybackAim,				idAI::Event_ScriptedPlaybackAim )
	EVENT( AI_ScriptedDone,						idAI::Event_ScriptedDone )
	EVENT( AI_ScriptedStop,						idAI::Event_ScriptedStop )
	EVENT( AI_ScriptedJumpDown,					idAI::Event_ScriptedJumpDown )

	// Get / Set 
	EVENT( AI_SetTalkState,						idAI::Event_SetTalkState )
	EVENT( AI_SetLeader,						idAI::Event_SetLeader )
	EVENT( AI_GetLeader,						idAI::Event_GetLeader )
	EVENT( AI_SetEnemy,							idAI::Event_SetEnemy )
	EVENT( AI_GetEnemy,							idAI::Event_GetEnemy )
	EVENT( EV_GetAngles,						idAI::Event_GetAngles )
	EVENT( EV_SetAngles,						idAI::Event_SetAngles )
	EVENT( AI_SetScript,						idAI::Event_SetScript )
	EVENT( AI_SetMoveSpeed,						idAI::Event_SetMoveSpeed )
	EVENT( AI_SetPassivePrefix,					idAI::Event_SetPassivePrefix )
	
	// Misc
	EVENT( AI_Attack,							idAI::Event_Attack )
	EVENT( AI_AttackMelee,						idAI::Event_AttackMelee )

	EVENT( AI_LookAt,							idAI::Event_LookAt )
	EVENT( AI_DirectDamage,						idAI::Event_DirectDamage )
	EVENT( AI_RadiusDamageFromJoint,			idAI::Event_RadiusDamageFromJoint )
	EVENT( AI_CanBecomeSolid,					idAI::Event_CanBecomeSolid )
	EVENT( AI_BecomeSolid,						idAI::Event_BecomeSolid )
	EVENT( EV_BecomeNonSolid,					idAI::Event_BecomeNonSolid )
	EVENT( AI_BecomeRagdoll,					idAI::Event_BecomeRagdoll )
	EVENT( AI_BecomePassive,					idAI::Event_BecomePassive )
	EVENT( AI_BecomeAggressive,					idAI::Event_BecomeAggressive )
	EVENT( AI_StopRagdoll,						idAI::Event_StopRagdoll )
	EVENT( AI_SetHealth,						idAI::Event_SetHealth )
	EVENT( AI_FaceEnemy,						idAI::Event_FaceEnemy )
	EVENT( AI_FaceEntity,						idAI::Event_FaceEntity )
	EVENT( AI_StopThinking,						idAI::Event_StopThinking )
	EVENT( AI_LockEnemyOrigin,					idAI::Event_LockEnemyOrigin )
	EVENT( AI_JumpFrame,						idAI::Event_JumpFrame )
	EVENT( AI_RealKill,							idAI::Event_RealKill )
	EVENT( AI_Kill,								idAI::Event_Kill )
	EVENT( AI_RemoveUpdateSpawner,				idAI::Event_RemoveUpdateSpawner )
	EVENT( AI_AllowHiddenMovement,				idAI::Event_AllowHiddenMovement )
	EVENT( AI_Speak,							idAI::Event_Speak )
	EVENT( AI_SpeakRandom,						idAI::Event_SpeakRandom )
	EVENT( AI_IsSpeaking,						idAI::Event_IsSpeaking )
	EVENT( AI_IsTethered,						idAI::Event_IsTethered )
	EVENT( AI_IsWithinTether,					idAI::Event_IsWithinTether )
	EVENT( EV_IsMoving,							idAI::Event_IsMoving )
	EVENT( AI_TakeDamage,						idAI::Event_TakeDamage )
	EVENT( AI_FindEnemy,						idAI::Event_FindEnemy )
	EVENT( EV_SetKey,							idAI::Event_SetKey )
	// RAVEN BEGIN
	// twhitaker: needed this for difficulty settings
	EVENT( EV_PostSpawn,						idAI::Event_PostSpawn )
	// RAVEN END
END_CLASS

/*
=====================
idAI::Event_PredictEnemyPos
=====================
*/
void idAI::Event_PredictEnemyPos( float time ) {
	predictedPath_t path;
	idEntity*		enemyEnt = enemy.ent;

	// if no enemy set
	if ( !enemyEnt ) {
		idThread::ReturnVector( physicsObj.GetOrigin() );
		return;
	}

	// predict the enemy movement
	idAI::PredictPath( enemyEnt, aas, enemy.lastKnownPosition, enemyEnt->GetPhysics()->GetLinearVelocity(), SEC2MS( time ), SEC2MS( time ), ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	idThread::ReturnVector( path.endPos );
}

/*
=====================
idAI::Event_TestAnimMoveTowardEnemy
=====================
*/
void idAI::Event_TestAnimMoveTowardEnemy( const char *animname ) {
	int				anim;
	predictedPath_t path;
	idVec3			moveVec;
	float			yaw;
	idVec3			delta;
	idEntity		*enemyEnt;
	
	enemyEnt = enemy.ent;
	if ( !enemyEnt ) {
		idThread::ReturnInt( false );
		return;
	}

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {
		gameLocal.DWarning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		idThread::ReturnInt( false );
		return;
	}

	delta = enemyEnt->GetPhysics()->GetOrigin() - physicsObj.GetOrigin();
    yaw = delta.ToYaw();

	moveVec = animator.TotalMovementDelta( anim ) * idAngles( 0.0f, yaw, 0.0f ).ToMat3() * physicsObj.GetGravityAxis();
	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), moveVec, 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( DebugFilter(ai_debugMove) ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
		gameRenderWorld->DebugBounds( path.endEvent == 0 ? colorYellow : colorRed, physicsObj.GetBounds(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
	}

	idThread::ReturnInt( path.endEvent == 0 );
}

/*
=====================
idAI::Event_TestAnimMove
=====================
*/
void idAI::Event_TestAnimMove( const char *animname ) {
	int				anim;
	predictedPath_t path;
	idVec3			moveVec;
	int				animLen;

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {
		gameLocal.DWarning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		idThread::ReturnInt( false );
		return;
	}

	moveVec = animator.TotalMovementDelta( anim ) * idAngles( 0.0f, move.ideal_yaw, 0.0f ).ToMat3() * physicsObj.GetGravityAxis();
	animLen = animator.AnimLength( anim );
	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), moveVec, 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if (  DebugFilter(ai_debugMove) ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
		gameRenderWorld->DebugBounds( path.endEvent == 0 ? colorYellow : colorRed, physicsObj.GetBounds(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
	}

	idThread::ReturnInt( path.endEvent == 0 );
}

/*
=====================
idAI::Event_TestMoveToPosition
=====================
*/
void idAI::Event_TestMoveToPosition( const idVec3 &position ) {
	predictedPath_t path;

	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), position - physicsObj.GetOrigin(), 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( DebugFilter(ai_debugMove) ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), position, gameLocal.msec );
 		gameRenderWorld->DebugBounds( colorYellow, physicsObj.GetBounds(), position, gameLocal.msec );
		if ( path.endEvent ) {
			gameRenderWorld->DebugBounds( colorRed, physicsObj.GetBounds(), path.endPos, gameLocal.msec );
		}
	}

	idThread::ReturnInt( path.endEvent == 0 );
}

/*
=====================
idAI::Event_TestMeleeAttack
=====================
*/
void idAI::Event_TestMeleeAttack( void ) {
	bool result = TestMelee();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Event_TestAnimAttack
=====================
*/
void idAI::Event_TestAnimAttack( const char *animname ) {
	int				anim;
	predictedPath_t path;

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {
		gameLocal.DWarning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		idThread::ReturnInt( false );
		return;
	}

	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), animator.TotalMovementDelta( anim ), 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	idThread::ReturnInt( path.blockingEntity && ( path.blockingEntity == enemy.ent ) );
}

/*
=====================
idAI::Event_LockEnemyOrigin
=====================
*/
void idAI::Event_LockEnemyOrigin ( void ) {
	enemy.fl.lockOrigin = true;
}

/*
=====================
idAI::Event_StopThinking
=====================
*/
void idAI::Event_StopThinking( void ) {
	BecomeInactive( TH_THINK );
	idThread *thread = idThread::CurrentThread();
	if ( thread ) {
		thread->DoneProcessing();
	}
}

/*
=====================
idAI::Event_JumpFrame
=====================
*/
void idAI::Event_JumpFrame( void ) {
	aifl.jump = true;
}

/*
=====================
idAI::Event_EnableClip
=====================
*/
void idAI::Event_EnableClip( void ) {
	physicsObj.SetClipMask( MASK_MONSTERSOLID );
	Event_EnableGravity ( );
}

/*
=====================
idAI::Event_DisableClip
=====================
*/
void idAI::Event_DisableClip( void ) {
	physicsObj.SetClipMask( 0 );
	Event_DisableGravity ( );
}

/*
=====================
idAI::Event_EnableGravity
=====================
*/
void idAI::Event_EnableGravity( void ) {
	OverrideFlag ( AIFLAGOVERRIDE_NOGRAVITY, false );
}

/*
=====================
idAI::Event_DisableGravity
=====================
*/
void idAI::Event_DisableGravity( void ) {
	OverrideFlag ( AIFLAGOVERRIDE_NOGRAVITY, true );
}

/*
=====================
idAI::Event_EnableAFPush
=====================
*/
void idAI::Event_EnableAFPush( void ) {
	move.fl.allowPushMovables = true;
}

/*
=====================
idAI::Event_DisableAFPush
=====================
*/
void idAI::Event_DisableAFPush( void ) {
	move.fl.allowPushMovables = false;
}

/*
=====================
idAI::Event_EnableDamage
=====================
*/
void idAI::Event_EnableDamage ( void ) { 
	OverrideFlag ( AIFLAGOVERRIDE_DAMAGE, true );
}

/*
=====================
idAI::Event_DisableDamage
=====================
*/
void idAI::Event_DisableDamage ( void ) { 
	OverrideFlag ( AIFLAGOVERRIDE_DAMAGE, false );
}

/*
===============
idAI::Event_DisablePain
===============
*/
void idAI::Event_DisablePain( void ) {
	OverrideFlag ( AIFLAGOVERRIDE_DISABLEPAIN, true );
}

/*
===============
idAI::Event_EnablePain
===============
*/
void idAI::Event_EnablePain( void ) {
	OverrideFlag ( AIFLAGOVERRIDE_DISABLEPAIN, false );
}

/*
===============
idAI::Event_EnableTarget
===============
*/
void idAI::Event_EnableTarget ( void ) {
	fl.notarget = false;
}

/*
===============
idAI::Event_DisableTarget
===============
*/
void idAI::Event_DisableTarget ( void ) {
	fl.notarget = true;
}


/*
=====================
idAI::Event_EnableAutoBlink
=====================
*/
void idAI::Event_EnableAutoBlink( void ) {
	fl.allowAutoBlink = true;
}

/*
=====================
idAI::Event_DisableAutoBlink
=====================
*/
void idAI::Event_DisableAutoBlink( void ) {
	fl.allowAutoBlink = false;
}

/*
=====================
idAI::Event_BecomeAggressive
=====================
*/
void idAI::Event_BecomeAggressive ( void ) {
	combat.fl.ignoreEnemies = false;
	combat.fl.aware = true;
	ForceTacticalUpdate ( );
}

/*
=====================
idAI::Event_BecomePassive
=====================
*/
void idAI::Event_BecomePassive ( int ignoreEnemies ) {
	combat.fl.ignoreEnemies = (ignoreEnemies != 0);
	combat.fl.aware = false;
	SetEnemy ( NULL );
	ForceTacticalUpdate ( );
}

/*
=====================
idAI::Event_LookAt
=====================
*/
void idAI::Event_LookAt	( idEntity* lookAt ) {
	lookTarget = lookAt;
}

/*
=====================
idAI::LookAtEntity
=====================
*/
void idAI::LookAtEntity( idEntity *ent, float duration ) {
	if ( ent == this ) {
		ent = NULL;
	}

	if ( ( ent != focusEntity.GetEntity() ) || ( focusTime < gameLocal.time ) ) {
		focusEntity	= ent;
		alignHeadTime = gameLocal.time;
		forceAlignHeadTime = gameLocal.time + SEC2MS( 1 );
		blink_time = 0;
	}

	focusTime = gameLocal.time + SEC2MS( duration );
}

/*
================
idAI::Event_ThrowMoveable
================
*/
void idAI::Event_ThrowMoveable( void ) {
	idEntity *ent;
	idEntity *moveable = NULL;

	for ( ent = GetNextTeamEntity(); ent != NULL; ent = ent->GetNextTeamEntity() ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent->GetBindMaster() == this && ent->IsType( idMoveable::GetClassType() ) ) {
// RAVEN END
			moveable = ent;
			break;
		}
	}
	if ( moveable ) {
		moveable->Unbind();
		moveable->PostEventMS( &EV_SetOwner, 200, 0/*NULL*/ ); //k 4th arg
	}
}

/*
================
idAI::Event_ThrowAF
================
*/
void idAI::Event_ThrowAF( void ) {
	idEntity *ent;
	idEntity *af = NULL;

	for ( ent = GetNextTeamEntity(); ent != NULL; ent = ent->GetNextTeamEntity() ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent->GetBindMaster() == this && ent->IsType( idAFEntity_Base::GetClassType() ) ) {
// RAVEN END
			af = ent;
			break;
		}
	}
	if ( af ) {
		af->Unbind();
		af->PostEventMS( &EV_SetOwner, 200, 0/*NULL*/ ); //k 4th arg
	}
}

/*
================
idAI::Event_SetAngles
================
*/
void idAI::Event_SetAngles( idAngles const &ang ) {
	move.current_yaw = ang.yaw;
	viewAxis = idAngles( 0, move.current_yaw, 0 ).ToMat3();
}

/*
================
idAI::Event_GetAngles
================
*/
void idAI::Event_GetAngles( void ) {
	idThread::ReturnVector( idVec3( 0.0f, move.current_yaw, 0.0f ) );
}

/*
================
idAI::Event_RealKill
================
*/
void idAI::Event_RealKill( void ) {
	health = 0;

	if ( af.IsLoaded() ) {
		// clear impacts
		af.Rest();

		// physics is turned off by calling af.Rest()
		BecomeActive( TH_PHYSICS );
	}

	Killed( this, this, 0, vec3_zero, INVALID_JOINT );
}

/*
================
idAI::Event_Kill
================
*/
void idAI::Event_Kill( void ) {
	PostEventMS( &AI_RealKill, 0 );
}

/*
================
idAI::Event_RemoveUpdateSpawner
================
*/
void idAI::Event_RemoveUpdateSpawner( void ) {
	// Detach from any spawners
	if( GetSpawner() ) {
		GetSpawner()->Detach( this );
		SetSpawner( NULL );
	}

	PostEventMS( &EV_Remove, 0 );
}
/*
=====================
idAI::Event_FindActorsInBounds
=====================
*/
void idAI::Event_FindActorsInBounds( const idVec3 &mins, const idVec3 &maxs ) {
	idEntity *	ent;
	idEntity *	entityList[ MAX_GENTITIES ];
	int			numListedEntities;
	int			i;

// RAVEN BEGIN
// ddynerman: multiple clip worlds
	numListedEntities = gameLocal.EntitiesTouchingBounds( this, idBounds( mins, maxs ), CONTENTS_BODY, entityList, MAX_GENTITIES );
// RAVEN END
	for( i = 0; i < numListedEntities; i++ ) {
		ent = entityList[ i ];
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent != this && !ent->IsHidden() && ( ent->health > 0 ) && ent->IsType( idActor::GetClassType() ) ) {
// RAVEN END
			idThread::ReturnEntity( ent );
			return;
		}
	}

	idThread::ReturnEntity( NULL );
}

/*
=====================
idAI::Event_ClosestReachableEnemyOfEntity
=====================
*/
void idAI::Event_ClosestReachableEnemyOfEntity( idEntity *team_mate ) {
	idActor *actor;
	idActor *ent;
	idActor	*bestEnt;
	float	bestDistSquared;
	float	distSquared;
	idVec3	delta;
	int		areaNum;
	int		enemyAreaNum;
	aasPath_t path;
	
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( !team_mate->IsType( idActor::GetClassType() ) ) {
// RAVEN END
		gameLocal.Error( "Entity '%s' is not an AI character or player", team_mate->GetName() );
	}

	actor = static_cast<idActor *>( team_mate );

	const idVec3 &origin = physicsObj.GetOrigin();
	areaNum = PointReachableAreaNum( origin );

	bestDistSquared = idMath::INFINITY;
	bestEnt = NULL;
	for( ent = actor->enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( ent->fl.hidden ) {
			continue;
		}
		delta = ent->GetPhysics()->GetOrigin() - origin;
		distSquared = delta.LengthSqr();
		if ( distSquared < bestDistSquared ) {
			const idVec3 &enemyPos = ent->GetPhysics()->GetOrigin();
			enemyAreaNum = PointReachableAreaNum( enemyPos );
			if ( ( areaNum != 0 ) && PathToGoal( path, areaNum, origin, enemyAreaNum, enemyPos ) ) {
				bestEnt = ent;
				bestDistSquared = distSquared;
			}
		}
	}

	idThread::ReturnEntity( bestEnt );
}

/*
=====================
idAI::Event_EntityInAttackCone
=====================
*/
void idAI::Event_EntityInAttackCone( idEntity *ent ) {
	float	attack_cone;
	idVec3	delta;
	float	yaw;
	float	relYaw;
	
	if ( !ent ) {
		idThread::ReturnInt( false );
		return;
	}

	delta = ent->GetPhysics()->GetOrigin() - GetEyePosition();

	// get our gravity normal
	const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();

	// infinite vertical vision, so project it onto our orientation plane
	delta -= gravityDir * ( gravityDir * delta );

	delta.Normalize();
	yaw = delta.ToYaw();

	attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	relYaw = idMath::AngleNormalize180( move.ideal_yaw - yaw );
	if ( idMath::Fabs( relYaw ) < ( attack_cone * 0.5f ) ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
================
idAI::Event_CanReachPosition
================
*/
void idAI::Event_CanReachPosition( const idVec3 &pos ) {
	aasPath_t	path;
	int			toAreaNum;
	int			areaNum;

	toAreaNum = PointReachableAreaNum( pos );
	areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
	if ( !toAreaNum || !PathToGoal( path, areaNum, physicsObj.GetOrigin(), toAreaNum, pos ) ) {
		idThread::ReturnInt( false );
	} else {
		idThread::ReturnInt( true );
	}
}

/*
================
idAI::Event_CanReachEntity
================
*/
void idAI::Event_CanReachEntity( idEntity *ent ) {
	aasPath_t	path;
	int			toAreaNum;
	int			areaNum;
	idVec3		pos;

	if ( !ent ) {
		idThread::ReturnInt( false );
		return;
	}

	if ( move.moveType != MOVETYPE_FLY ) {
		if ( !ent->GetFloorPos( 64.0f, pos ) ) {
			idThread::ReturnInt( false );
			return;
		}
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent->IsType( idActor::GetClassType() ) && static_cast<idActor *>( ent )->OnLadder() ) {
// RAVEN END
			idThread::ReturnInt( false );
			return;
		}
	} else {
		pos = ent->GetPhysics()->GetOrigin();
	}

	toAreaNum = PointReachableAreaNum( pos );
	if ( !toAreaNum ) {
		idThread::ReturnInt( false );
		return;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );
	if ( !toAreaNum || !PathToGoal( path, areaNum, org, toAreaNum, pos ) ) {
		idThread::ReturnInt( false );
	} else {
		idThread::ReturnInt( true );
	}
}

/*
================
idAI::Event_CanReachEnemy
================
*/
void idAI::Event_CanReachEnemy( void ) {
	aasPath_t	path;
	int			toAreaNum = 0;
	int			areaNum;
	idVec3		pos;
	idEntity	*enemyEnt;

	enemyEnt = enemy.ent;
	if ( !enemyEnt ) {
		idThread::ReturnInt( false );
		return;
	}

	if ( move.moveType != MOVETYPE_FLY ) {
		if( enemyEnt->IsType( idActor::GetClassType() ) ){
			idActor *enemyAct = static_cast<idActor *>( enemyEnt );
			if ( enemyAct->OnLadder() ) {
				idThread::ReturnInt( false );
				return;
			}
			enemyAct->GetAASLocation( aas, pos, toAreaNum );
		}
	}  else {
		pos = enemyEnt->GetPhysics()->GetOrigin();
		toAreaNum = PointReachableAreaNum( pos );
	}

	if ( !toAreaNum ) {
		idThread::ReturnInt( false );
		return;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );
	if ( !PathToGoal( path, areaNum, org, toAreaNum, pos ) ) {
		idThread::ReturnInt( false );
	} else {
		idThread::ReturnInt( true );
	}
}

/*
================
idAI::Event_GetReachableEntityPosition
================
*/
void idAI::Event_GetReachableEntityPosition( idEntity *ent ) {
	int		toAreaNum;
	idVec3	pos;

	if ( move.moveType != MOVETYPE_FLY ) {
		if ( !ent->GetFloorPos( 64.0f, pos ) ) {
			idThread::ReturnInt( false );
			return;
		}
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent->IsType( idActor::GetClassType() ) && static_cast<idActor *>( ent )->OnLadder() ) {
// RAVEN END
			idThread::ReturnInt( false );
			return;
		}
	} else {
		pos = ent->GetPhysics()->GetOrigin();
	}

	if ( aas ) {
		toAreaNum = PointReachableAreaNum( pos );
		aas->PushPointIntoAreaNum( toAreaNum, pos );
	}

	idThread::ReturnVector( pos );
}

/*
================
idAI::Event_ScriptedMove
================
*/
void idAI::Event_ScriptedMove ( idEntity* destEnt, float minDist, bool endWithIdle ) {
	ScriptedMove ( destEnt, minDist, endWithIdle );
}

/*
================
idAI::Event_ScriptedFace
================
*/
void idAI::Event_ScriptedFace ( idEntity* faceEnt, bool endWithIdle ) {
	ScriptedFace ( faceEnt, endWithIdle );
}

/*
================
idAI::Event_ScriptedAnim
================
*/
void idAI::Event_ScriptedAnim ( const char* animname, int blendFrames, bool loop, bool endWithIdle ) {
	ScriptedAnim ( animname, blendFrames, loop, endWithIdle );
}

/*
================
idAI::Event_ScriptedAction
================
*/
void idAI::Event_ScriptedAction ( idEntity* actionEnt, bool endWithIdle ) {
	ScriptedAction ( actionEnt, endWithIdle );
}

/*
================
idAI::Event_ScriptedPlaybackMove
================
*/
void idAI::Event_ScriptedPlaybackMove ( const char* playback, int flags, int numFrames ) {
	ScriptedPlaybackMove ( playback, flags, numFrames );
}

/*
================
idAI::Event_ScriptedPlaybackAim
================
*/
void idAI::Event_ScriptedPlaybackAim( const char* playback, int flags, int numFrames ) {
	ScriptedPlaybackAim ( playback, flags, numFrames );
}

/*
================
idAI::Event_ScriptedDone
================
*/
void idAI::Event_ScriptedDone ( void ) {
	idThread::ReturnFloat ( !aifl.scripted );
}

/*
================
idAI::Event_ScriptedStop
================
*/
void idAI::Event_ScriptedStop ( void ) {
	ScriptedStop ( );
}

/*
================
idAI::Event_AllowHiddenMovement
================
*/
void idAI::Event_AllowHiddenMovement( int enable ) {
	move.fl.allowHiddenMove = ( enable != 0 );
}

/*
================
idAI::Event_SetScript
================
*/
void idAI::Event_SetScript ( const char* scriptName, const char* funcName ) {
	SetScript ( scriptName, funcName );
}

/*
================
idAI::Event_SetMoveSpeed
================
*/
void idAI::Event_SetMoveSpeed ( int speed ) {
	switch ( speed ) {
		case AIMOVESPEED_DEFAULT:
			move.fl.noRun = false;
			move.fl.noWalk = false;
			break;
			
		case AIMOVESPEED_RUN:
			move.fl.noRun = false;
			move.fl.noWalk = true;
			break;
			
		case AIMOVESPEED_WALK:
			move.fl.noRun = true;
			move.fl.noWalk = false;
			break;
	}
}

/*
================
idAI::Event_SetPassivePrefix
================
*/
void idAI::Event_SetPassivePrefix ( const char* prefix ) {
	SetPassivePrefix ( prefix );
}

/*
================
idAI::Event_Attack
================
*/
void idAI::Event_Attack ( const char* attackName, const char* jointName ) { 
	Attack ( attackName, animator.GetJointHandle ( jointName ), enemy.ent ); // , physicsObj.GetPushedLinearVelocity ( ) ); 
}

/*
================
idAI::Event_AttackMelee
================
*/
void idAI::Event_AttackMelee( const char* meleeName ) { 
	const idDict* meleeDict;
	meleeDict = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( va("def_attack_%s", meleeName ) ), false );
	if ( !meleeDict ) {
		gameLocal.Error ( "missing meleeDef '%s' for ai entity '%s'", meleeName, GetName() );
	}
	AttackMelee ( meleeName, meleeDict ); 
}

/*
================
idAI::Event_ScriptedJumpDown
================
*/
void idAI::Event_ScriptedJumpDown( float yaw ) { 
	if ( animator.HasAnim( "jumpdown_start" ) )
	{
		aifl.scripted = true;
		move.ideal_yaw = yaw;
		SetState( "State_ScriptedJumpDown" );
	}
}

/*
================
idAI::Event_FindEnemy
================
*/
void idAI::Event_FindEnemy( float distSqr )	{
		idThread::ReturnEntity ( FindEnemy( false, 1, distSqr ));
}

/*
================
idAI::Event_SetKey
================
*/
void idAI::Event_SetKey( const char *key, const char *value ) {	
	spawnArgs.Set( key, value );
	
	OnSetKey ( key, value );
}

/*
================
idAI::Event_PostSpawn
================
*/
void idAI::Event_PostSpawn( void ) {
	// RAVEN BEGIN
	// twhitaker: difficulty levels
	if ( team == TEAM_MARINE ) {
		//health /= 1.0f + gameLocal.GetDifficultyModifier( );

		//buddies are a little more healthy on hard & nightmare since the baddies deal so much more damage
		switch ( g_skill.GetInteger() ) {
		case 3:
			health *= 1.4f;
			break;
		case 2:
			health *= 1.2f;
			break;
		case 0:
			health *= 1.2f;
			break;
		case 1:
		default:
			break;
		}
	} else {
		health *= 1.0f + gameLocal.GetDifficultyModifier( );
	}
	// RAVEN END
}
