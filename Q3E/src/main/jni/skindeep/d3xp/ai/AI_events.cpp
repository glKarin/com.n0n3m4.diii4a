/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "Moveable.h"
#include "Misc.h"

#include "bc_meta.h"
#include "bc_searchnode.h"

#include "gamesys/SysCvar.h"
#include "ai/AI.h"



/***********************************************************************

	AI Events

***********************************************************************/

const idEventDef AI_FindEnemy( "findEnemy", "d", 'e' );
const idEventDef AI_FindEnemyAI( "findEnemyAI", "d", 'e' );
const idEventDef AI_FindEnemyInCombatNodes( "findEnemyInCombatNodes", NULL, 'e' );
const idEventDef AI_ClosestReachableEnemyOfEntity( "closestReachableEnemyOfEntity", "E", 'e' );
//const idEventDef AI_HeardSound( "heardSound", "d", 'e' );
const idEventDef AI_SetEnemy( "setEnemy", "E" );
const idEventDef AI_ClearEnemy( "clearEnemy" );
const idEventDef AI_MuzzleFlash( "muzzleFlash", "s" );
const idEventDef AI_CreateMissile( "createMissile", "s", 'e' );
const idEventDef AI_AttackMissile( "attackMissile", "s", 'e' );
const idEventDef AI_FireMissileAtTarget( "fireMissileAtTarget", "ss", 'e' );
const idEventDef AI_LaunchMissile( "launchMissile", "vv", 'e' );
const idEventDef AI_LaunchProjectile( "launchProjectile", "s" );
const idEventDef AI_AttackMelee( "attackMelee", "s", 'd' );
const idEventDef AI_DirectDamage( "directDamage", "es" );
const idEventDef AI_RadiusDamageFromJoint( "radiusDamageFromJoint", "ss" );
const idEventDef AI_BeginAttack( "attackBegin", "s" );
const idEventDef AI_EndAttack( "attackEnd" );
const idEventDef AI_MeleeAttackToJoint( "meleeAttackToJoint", "ss", 'd' );
const idEventDef AI_RandomPath( "randomPath", NULL, 'e' );
const idEventDef AI_CanBecomeSolid( "canBecomeSolid", NULL, 'f' );
const idEventDef AI_BecomeSolid( "becomeSolid" );
const idEventDef AI_BecomeRagdoll( "becomeRagdoll", NULL, 'd' );
const idEventDef AI_StopRagdoll( "stopRagdoll" );
const idEventDef AI_SetHealth( "setHealth", "f" );
const idEventDef AI_GetHealth( "getHealth", NULL, 'f' );
const idEventDef AI_AllowDamage( "allowDamage" );
const idEventDef AI_IgnoreDamage( "ignoreDamage" );
const idEventDef AI_GetCurrentYaw( "getCurrentYaw", NULL, 'f' );
const idEventDef AI_TurnTo( "turnTo", "f" );
const idEventDef AI_TurnToPos( "turnToPos", "v" );
const idEventDef AI_TurnToEntity( "turnToEntity", "E" );
const idEventDef AI_MoveStatus( "moveStatus", NULL, 'd' );
const idEventDef AI_StopMove( "stopMove" );
const idEventDef AI_MoveToCover( "moveToCover" );
const idEventDef AI_MoveToEnemy( "moveToEnemy", NULL, 'd' );
const idEventDef AI_MoveToEnemyHeight( "moveToEnemyHeight" );
const idEventDef AI_MoveOutOfRange( "moveOutOfRange", "ef" );
const idEventDef AI_MoveToAttackPosition( "moveToAttackPosition", "es" );
const idEventDef AI_Wander( "wander" );
const idEventDef AI_MoveToEntity( "moveToEntity", "e" );
const idEventDef AI_MoveToPosition( "moveToPosition", "v" );
const idEventDef AI_SlideTo( "slideTo", "vf" );
const idEventDef AI_FacingIdeal( "facingIdeal", NULL, 'd' );
const idEventDef AI_FaceEnemy( "faceEnemy" );
const idEventDef AI_FaceEntity( "faceEntity", "E" );
const idEventDef AI_GetCombatNode( "getCombatNode", NULL, 'e' );
const idEventDef AI_EnemyInCombatCone( "enemyInCombatCone", "Ed", 'd' );
const idEventDef AI_WaitMove( "waitMove" );
const idEventDef AI_GetJumpVelocity( "getJumpVelocity", "vff", 'v' );
const idEventDef AI_EntityInAttackCone( "entityInAttackCone", "E", 'd' );
const idEventDef AI_CanSeeEntity( "canSee", "Ed", 'd' );
const idEventDef AI_SetTalkTarget( "setTalkTarget", "E" );
const idEventDef AI_GetTalkTarget( "getTalkTarget", NULL, 'e' );
const idEventDef AI_SetTalkState( "setTalkState", "d" );
const idEventDef AI_EnemyRange( "enemyRange", NULL, 'f' );
const idEventDef AI_EnemyRange2D( "enemyRange2D", NULL, 'f' );
const idEventDef AI_GetEnemy( "getEnemy", NULL, 'e' );
const idEventDef AI_GetEnemyPos( "getEnemyPos", NULL, 'v' );
const idEventDef AI_GetEnemyEyePos( "getEnemyEyePos", NULL, 'v' );
const idEventDef AI_PredictEnemyPos( "predictEnemyPos", "f", 'v' );
const idEventDef AI_CanHitEnemy( "canHitEnemy", NULL, 'd' );
const idEventDef AI_CanHitEnemyFromAnim( "canHitEnemyFromAnim", "s", 'd' );
const idEventDef AI_CanHitEnemyFromJoint( "canHitEnemyFromJoint", "s", 'd' );
const idEventDef AI_EnemyPositionValid( "enemyPositionValid", NULL, 'd' );
const idEventDef AI_ChargeAttack( "chargeAttack", "s" );
const idEventDef AI_TestChargeAttack( "testChargeAttack", NULL, 'f' );
const idEventDef AI_TestMoveToPosition( "testMoveToPosition", "v", 'd' );
const idEventDef AI_TestAnimMoveTowardEnemy( "testAnimMoveTowardEnemy", "s", 'd' );
const idEventDef AI_TestAnimMove( "testAnimMove", "s", 'd' );
const idEventDef AI_TestMeleeAttack( "testMeleeAttack", NULL, 'd' );
const idEventDef AI_TestAnimAttack( "testAnimAttack", "s", 'd' );
const idEventDef AI_Shrivel( "shrivel", "f" );
const idEventDef AI_Burn( "burn" );
const idEventDef AI_ClearBurn( "clearBurn" );
const idEventDef AI_PreBurn( "preBurn" );
const idEventDef AI_SetSmokeVisibility( "setSmokeVisibility", "dd" );
const idEventDef AI_NumSmokeEmitters( "numSmokeEmitters", NULL, 'd' );
const idEventDef AI_WaitAction( "waitAction", "s" );
const idEventDef AI_StopThinking( "stopThinking" );
const idEventDef AI_GetTurnDelta( "getTurnDelta", NULL, 'f' );
const idEventDef AI_GetMoveType( "getMoveType", NULL, 'd' );
const idEventDef AI_SetMoveType( "setMoveType", "d" );
const idEventDef AI_SaveMove( "saveMove" );
const idEventDef AI_RestoreMove( "restoreMove" );
const idEventDef AI_AllowMovement( "allowMovement", "f" );
const idEventDef AI_JumpFrame( "<jumpframe>" );
const idEventDef AI_EnableClip( "enableClip" );
const idEventDef AI_DisableClip( "disableClip" );
const idEventDef AI_EnableGravity( "enableGravity" );
const idEventDef AI_DisableGravity( "disableGravity" );
const idEventDef AI_EnableAFPush( "enableAFPush" );
const idEventDef AI_DisableAFPush( "disableAFPush" );
const idEventDef AI_SetFlySpeed( "setFlySpeed", "f" );
const idEventDef AI_SetFlyOffset( "setFlyOffset", "d" );
const idEventDef AI_ClearFlyOffset( "clearFlyOffset" );
const idEventDef AI_GetClosestHiddenTarget( "getClosestHiddenTarget", "s", 'e' );
const idEventDef AI_GetRandomTarget( "getRandomTarget", "s", 'e' );
const idEventDef AI_TravelDistanceToPoint( "travelDistanceToPoint", "v", 'f' );
const idEventDef AI_TravelDistanceToEntity( "travelDistanceToEntity", "e", 'f' );
const idEventDef AI_TravelDistanceBetweenPoints( "travelDistanceBetweenPoints", "vv", 'f' );
const idEventDef AI_TravelDistanceBetweenEntities( "travelDistanceBetweenEntities", "ee", 'f' );
const idEventDef AI_LookAtEntity( "lookAt", "Ef" );
const idEventDef AI_LookAtEnemy( "lookAtEnemy", "f" );
const idEventDef AI_SetJointMod( "setBoneMod", "d" );
const idEventDef AI_ThrowMoveable( "throwMoveable" );
const idEventDef AI_ThrowAF( "throwAF" );
const idEventDef AI_RealKill( "<kill>" );
const idEventDef AI_Kill( "kill" );
const idEventDef AI_WakeOnFlashlight( "wakeOnFlashlight", "d" );
const idEventDef AI_LocateEnemy( "locateEnemy" );
const idEventDef AI_KickObstacles( "kickObstacles", "Ef" );
const idEventDef AI_GetObstacle( "getObstacle", NULL, 'e' );
const idEventDef AI_PushPointIntoAAS( "pushPointIntoAAS", "v", 'v' );
const idEventDef AI_GetTurnRate( "getTurnRate", NULL, 'f' );
const idEventDef AI_SetTurnRate( "setTurnRate", "f" );
const idEventDef AI_AnimTurn( "animTurn", "f" );
const idEventDef AI_AllowHiddenMovement( "allowHiddenMovement", "d" );
const idEventDef AI_TriggerParticles( "triggerParticles", "s" );
const idEventDef AI_FindActorsInBounds( "findActorsInBounds", "vv", 'e' );
const idEventDef AI_CanReachPosition( "canReachPosition", "v", 'd' );
const idEventDef AI_CanReachEntity( "canReachEntity", "E", 'd' );
const idEventDef AI_CanReachEnemy( "canReachEnemy", NULL, 'd' );
const idEventDef AI_GetReachableEntityPosition( "getReachableEntityPosition", "e", 'v' );
const idEventDef AI_MoveToPositionDirect( "moveToPositionDirect", "v" );
const idEventDef AI_AvoidObstacles( "avoidObstacles", "d" );
const idEventDef AI_TriggerFX( "triggerFX", "ss" );
const idEventDef AI_StartEmitter( "startEmitter", "sss", 'e' );
const idEventDef AI_GetEmitter( "getEmitter", "s", 'e' );
const idEventDef AI_StopEmitter( "stopEmitter", "s" );



//BC
const idEventDef AI_GetCoverNode("getCoverNode", NULL, 'e');
const idEventDef AI_GetLastEnemyPosition("getLastEnemyPosition", NULL, 'v');
const idEventDef AI_HeardSuspiciousNoise("heardSuspiciousNoise", NULL, 'v');
const idEventDef AI_HeardSuspiciousPriority("heardSuspiciousPriority", NULL, 'd');
const idEventDef AI_GetThrowableObject("GetThrowableObject", "vvfff", 'e');
const idEventDef AI_ThrowObjectAtEnemy("ThrowObjectAtEnemy", "ef");
const idEventDef AI_ThrowObjectAtPosition("ThrowObjectAtPosition", "ev", 'd');
const idEventDef AI_LookAtPoint("lookAtPoint", "vf");
const idEventDef AI_GetSearchNode("getSearchNode", NULL, 'e');
const idEventDef AI_SetLaserLock("setLaserLock", "v");
const idEventDef AI_SetLaserSkin("setLaserSkin", "s");
const idEventDef AI_GetLaserLock("getLaserLock", NULL, 'v');
const idEventDef AI_GetLaserHitPos("getLaserHitPos", NULL, 'v');
const idEventDef AI_SetLaserActive("setLaserActive", "d");
const idEventDef AI_GetEnemyCenter("getEnemyCenter", NULL, 'v');
const idEventDef AI_SetFlyBobStrength("setFlybobstrength", "d");
const idEventDef AI_GetFlyBobStrength("getFlybobstrength", NULL, 'f');
const idEventDef AI_FindEnemyAIvisible("findEnemyAIvisible", NULL, 'e');
const idEventDef AI_LaunchMissileAtLaser("launchMissileAtLaser", "s");
const idEventDef AI_GetFlySpeed("getFlySpeed", NULL, 'f');
const idEventDef AI_DoDamage("doDamage", "s");
const idEventDef AI_ResetLookpoint("ResetLookpoint");
const idEventDef AI_CanHitFromAnim("CanHitFromAnim", "sv", 'f');
const idEventDef AI_CheckSearchLook("CheckSearchLook", "vd", 'd');
const idEventDef AI_SetLastVisiblePos("setLastVisiblePos", "v");
const idEventDef AI_GetObservationPosition("getObservationPosition", "v", 'v'); //Darkmod.
const idEventDef AI_GetObservationViaNodes("getObservationViaNodes", "v", 'v'); //Iterate through the searchnode positions and see if any of them has LOS to the player.
const idEventDef AI_CheckForwardDot("CheckForwardDot", "v", 'f');
const idEventDef AI_SetCombatState("setCombatState", "d");
const idEventDef AI_GetAIState("getAIState", NULL, 'd');
const idEventDef AI_EjectBrass("ejectbrass");
const idEventDef AI_StartStunState("startStunState", "s");
const idEventDef AI_IsBleedingOut("isBleedingOut", NULL, 'f');
const idEventDef AI_GetSkullsaver("getSkullsaver", NULL, 'e');

const idEventDef AI_SetPathEntity("setPathEntity", "e");

const idEventDef AI_GetLifeState("getLifeState", NULL, 'd');




CLASS_DECLARATION( idActor, idAI )
	EVENT( EV_Activate,							idAI::Event_Activate )
	EVENT( EV_Touch,							idAI::Event_Touch )
	EVENT( AI_FindEnemy,						idAI::Event_FindEnemy )
	EVENT( AI_FindEnemyAI,						idAI::Event_FindEnemyAI )
	EVENT( AI_FindEnemyInCombatNodes,			idAI::Event_FindEnemyInCombatNodes )
	EVENT( AI_ClosestReachableEnemyOfEntity,	idAI::Event_ClosestReachableEnemyOfEntity )
	//EVENT( AI_HeardSound,						idAI::Event_HeardSound )
	EVENT( AI_SetEnemy,							idAI::Event_SetEnemy )
	EVENT( AI_ClearEnemy,						idAI::Event_ClearEnemy )
	EVENT( AI_MuzzleFlash,						idAI::Event_MuzzleFlash )
	EVENT( AI_CreateMissile,					idAI::Event_CreateMissile )
	EVENT( AI_AttackMissile,					idAI::Event_AttackMissile )
	EVENT( AI_FireMissileAtTarget,				idAI::Event_FireMissileAtTarget )
	EVENT( AI_LaunchMissile,					idAI::Event_LaunchMissile )
#ifdef _D3XP
	EVENT( AI_LaunchProjectile,					idAI::Event_LaunchProjectile )
#endif
	EVENT( AI_AttackMelee,						idAI::Event_AttackMelee )
	EVENT( AI_DirectDamage,						idAI::Event_DirectDamage )
	EVENT( AI_RadiusDamageFromJoint,			idAI::Event_RadiusDamageFromJoint )
	EVENT( AI_BeginAttack,						idAI::Event_BeginAttack )
	EVENT( AI_EndAttack,						idAI::Event_EndAttack )
	EVENT( AI_MeleeAttackToJoint,				idAI::Event_MeleeAttackToJoint )
	EVENT( AI_RandomPath,						idAI::Event_RandomPath )
	EVENT( AI_CanBecomeSolid,					idAI::Event_CanBecomeSolid )
	EVENT( AI_BecomeSolid,						idAI::Event_BecomeSolid )
	EVENT( EV_BecomeNonSolid,					idAI::Event_BecomeNonSolid )
	EVENT( AI_BecomeRagdoll,					idAI::Event_BecomeRagdoll )
	EVENT( AI_StopRagdoll,						idAI::Event_StopRagdoll )
	EVENT( AI_SetHealth,						idAI::Event_SetHealth )
	EVENT( AI_GetHealth,						idAI::Event_GetHealth )
	EVENT( AI_AllowDamage,						idAI::Event_AllowDamage )
	EVENT( AI_IgnoreDamage,						idAI::Event_IgnoreDamage )
	EVENT( AI_GetCurrentYaw,					idAI::Event_GetCurrentYaw )
	EVENT( AI_TurnTo,							idAI::Event_TurnTo )
	EVENT( AI_TurnToPos,						idAI::Event_TurnToPos )
	EVENT( AI_TurnToEntity,						idAI::Event_TurnToEntity )
	EVENT( AI_MoveStatus,						idAI::Event_MoveStatus )
	EVENT( AI_StopMove,							idAI::Event_StopMove )
	EVENT( AI_MoveToCover,						idAI::Event_MoveToCover )
	EVENT( AI_MoveToEnemy,						idAI::Event_MoveToEnemy )
	EVENT( AI_MoveToEnemyHeight,				idAI::Event_MoveToEnemyHeight )
	EVENT( AI_MoveOutOfRange,					idAI::Event_MoveOutOfRange )
	EVENT( AI_MoveToAttackPosition,				idAI::Event_MoveToAttackPosition )
	EVENT( AI_Wander,							idAI::Event_Wander )
	EVENT( AI_MoveToEntity,						idAI::Event_MoveToEntity )
	EVENT( AI_MoveToPosition,					idAI::Event_MoveToPosition )
	EVENT( AI_SlideTo,							idAI::Event_SlideTo )
	EVENT( AI_FacingIdeal,						idAI::Event_FacingIdeal )
	EVENT( AI_FaceEnemy,						idAI::Event_FaceEnemy )
	EVENT( AI_FaceEntity,						idAI::Event_FaceEntity )
	EVENT( AI_WaitAction,						idAI::Event_WaitAction )
	EVENT( AI_GetCombatNode,					idAI::Event_GetCombatNode )
	EVENT( AI_EnemyInCombatCone,				idAI::Event_EnemyInCombatCone )
	EVENT( AI_WaitMove,							idAI::Event_WaitMove )
	EVENT( AI_GetJumpVelocity,					idAI::Event_GetJumpVelocity )
	EVENT( AI_EntityInAttackCone,				idAI::Event_EntityInAttackCone )
	EVENT( AI_CanSeeEntity,						idAI::Event_CanSeeEntity )
	EVENT( AI_SetTalkTarget,					idAI::Event_SetTalkTarget )
	EVENT( AI_GetTalkTarget,					idAI::Event_GetTalkTarget )
	EVENT( AI_SetTalkState,						idAI::Event_SetTalkState )
	EVENT( AI_EnemyRange,						idAI::Event_EnemyRange )
	EVENT( AI_EnemyRange2D,						idAI::Event_EnemyRange2D )
	EVENT( AI_GetEnemy,							idAI::Event_GetEnemy )
	EVENT( AI_GetEnemyPos,						idAI::Event_GetEnemyPos )
	EVENT( AI_GetEnemyEyePos,					idAI::Event_GetEnemyEyePos )
	EVENT( AI_PredictEnemyPos,					idAI::Event_PredictEnemyPos )
	EVENT( AI_CanHitEnemy,						idAI::Event_CanHitEnemy )
	EVENT( AI_CanHitEnemyFromAnim,				idAI::Event_CanHitEnemyFromAnim )
	EVENT( AI_CanHitEnemyFromJoint,				idAI::Event_CanHitEnemyFromJoint )
	EVENT( AI_EnemyPositionValid,				idAI::Event_EnemyPositionValid )
	EVENT( AI_ChargeAttack,						idAI::Event_ChargeAttack )
	EVENT( AI_TestChargeAttack,					idAI::Event_TestChargeAttack )
	EVENT( AI_TestAnimMoveTowardEnemy,			idAI::Event_TestAnimMoveTowardEnemy )
	EVENT( AI_TestAnimMove,						idAI::Event_TestAnimMove )
	EVENT( AI_TestMoveToPosition,				idAI::Event_TestMoveToPosition )
	EVENT( AI_TestMeleeAttack,					idAI::Event_TestMeleeAttack )
	EVENT( AI_TestAnimAttack,					idAI::Event_TestAnimAttack )
	EVENT( AI_Shrivel,							idAI::Event_Shrivel )
	EVENT( AI_Burn,								idAI::Event_Burn )
	EVENT( AI_PreBurn,							idAI::Event_PreBurn )
	EVENT( AI_SetSmokeVisibility,				idAI::Event_SetSmokeVisibility )
	EVENT( AI_NumSmokeEmitters,					idAI::Event_NumSmokeEmitters )
	EVENT( AI_ClearBurn,						idAI::Event_ClearBurn )
	EVENT( AI_StopThinking,						idAI::Event_StopThinking )
	EVENT( AI_GetTurnDelta,						idAI::Event_GetTurnDelta )
	EVENT( AI_GetMoveType,						idAI::Event_GetMoveType )
	EVENT( AI_SetMoveType,						idAI::Event_SetMoveType )
	EVENT( AI_SaveMove,							idAI::Event_SaveMove )
	EVENT( AI_RestoreMove,						idAI::Event_RestoreMove )
	EVENT( AI_AllowMovement,					idAI::Event_AllowMovement )
	EVENT( AI_JumpFrame,						idAI::Event_JumpFrame )
	EVENT( AI_EnableClip,						idAI::Event_EnableClip )
	EVENT( AI_DisableClip,						idAI::Event_DisableClip )
	EVENT( AI_EnableGravity,					idAI::Event_EnableGravity )
	EVENT( AI_DisableGravity,					idAI::Event_DisableGravity )
	EVENT( AI_EnableAFPush,						idAI::Event_EnableAFPush )
	EVENT( AI_DisableAFPush,					idAI::Event_DisableAFPush )
	EVENT( AI_SetFlySpeed,						idAI::Event_SetFlySpeed )
	EVENT( AI_SetFlyOffset,						idAI::Event_SetFlyOffset )
	EVENT( AI_ClearFlyOffset,					idAI::Event_ClearFlyOffset )
	EVENT( AI_GetClosestHiddenTarget,			idAI::Event_GetClosestHiddenTarget )
	EVENT( AI_GetRandomTarget,					idAI::Event_GetRandomTarget )
	EVENT( AI_TravelDistanceToPoint,			idAI::Event_TravelDistanceToPoint )
	EVENT( AI_TravelDistanceToEntity,			idAI::Event_TravelDistanceToEntity )
	EVENT( AI_TravelDistanceBetweenPoints,		idAI::Event_TravelDistanceBetweenPoints )
	EVENT( AI_TravelDistanceBetweenEntities,	idAI::Event_TravelDistanceBetweenEntities )
	EVENT( AI_LookAtEntity,						idAI::Event_LookAtEntity )
	EVENT( AI_LookAtEnemy,						idAI::Event_LookAtEnemy )
	EVENT( AI_SetJointMod,						idAI::Event_SetJointMod )
	EVENT( AI_ThrowMoveable,					idAI::Event_ThrowMoveable )
	EVENT( AI_ThrowAF,							idAI::Event_ThrowAF )
	EVENT( EV_GetAngles,						idAI::Event_GetAngles )
	EVENT( EV_SetAngles,						idAI::Event_SetAngles )
	EVENT( AI_RealKill,							idAI::Event_RealKill )
	EVENT( AI_Kill,								idAI::Event_Kill )
	EVENT( AI_WakeOnFlashlight,					idAI::Event_WakeOnFlashlight )
	EVENT( AI_LocateEnemy,						idAI::Event_LocateEnemy )
	EVENT( AI_KickObstacles,					idAI::Event_KickObstacles )
	EVENT( AI_GetObstacle,						idAI::Event_GetObstacle )
	EVENT( AI_PushPointIntoAAS,					idAI::Event_PushPointIntoAAS )
	EVENT( AI_GetTurnRate,						idAI::Event_GetTurnRate )
	EVENT( AI_SetTurnRate,						idAI::Event_SetTurnRate )
	EVENT( AI_AnimTurn,							idAI::Event_AnimTurn )
	EVENT( AI_AllowHiddenMovement,				idAI::Event_AllowHiddenMovement )
	EVENT( AI_TriggerParticles,					idAI::Event_TriggerParticles )
	EVENT( AI_FindActorsInBounds,				idAI::Event_FindActorsInBounds )
	EVENT( AI_CanReachPosition,					idAI::Event_CanReachPosition )
	EVENT( AI_CanReachEntity,					idAI::Event_CanReachEntity )
	EVENT( AI_CanReachEnemy,					idAI::Event_CanReachEnemy )
	EVENT( AI_GetReachableEntityPosition,		idAI::Event_GetReachableEntityPosition )
	EVENT( AI_MoveToPositionDirect,				idAI::Event_MoveToPositionDirect )
	EVENT( AI_AvoidObstacles,					idAI::Event_AvoidObstacles )
	EVENT( AI_TriggerFX,						idAI::Event_TriggerFX )
	EVENT( AI_StartEmitter,						idAI::Event_StartEmitter )
	EVENT( AI_GetEmitter,						idAI::Event_GetEmitter )
	EVENT( AI_StopEmitter,						idAI::Event_StopEmitter )


	//BC
	EVENT(AI_GetCoverNode,						idAI::Event_GetCoverNode)
	EVENT(AI_GetLastEnemyPosition,				idAI::Event_GetLastEnemyPosition)
	EVENT(AI_HeardSuspiciousNoise,				idAI::Event_HeardSuspiciousNoise)
	EVENT(AI_HeardSuspiciousPriority,			idAI::Event_HeardSuspiciousPriority)
	EVENT(AI_GetThrowableObject,				idAI::Event_GetThrowableObject)
	EVENT(AI_ThrowObjectAtEnemy,				idAI::Event_ThrowObjectAtEnemy)
	EVENT(AI_ThrowObjectAtPosition,				idAI::Event_ThrowObjectAtPosition)
	EVENT(AI_LookAtPoint,						idAI::Event_LookAtPoint)
	EVENT(AI_GetSearchNode,						idAI::Event_GetSearchNode)
	EVENT(AI_SetLaserLock,						idAI::Event_SetLaserLock)
	EVENT(AI_SetLaserSkin,						idAI::Event_SetLaserSkin)
	EVENT(AI_GetLaserHitPos,					idAI::Event_GetLaserHitPos)
	EVENT(AI_GetLaserLock,						idAI::Event_GetLaserLock)
	EVENT(AI_SetLaserActive,					idAI::Event_SetLaserActive)
	EVENT(AI_GetEnemyCenter,					idAI::Event_GetEnemyCenter)
	EVENT(AI_SetFlyBobStrength,					idAI::Event_SetFlyBobStrength)
	EVENT(AI_GetFlyBobStrength,					idAI::Event_GetFlyBobStrength)
	EVENT(AI_FindEnemyAIvisible,				idAI::Event_FindEnemyAIvisible)
	EVENT(AI_LaunchMissileAtLaser,				idAI::Event_LaunchMissileAtLaser)
	EVENT(AI_GetFlySpeed,						idAI::Event_GetFlySpeed)
	EVENT(AI_DoDamage,							idAI::Event_DoDamage)
	EVENT(AI_ResetLookpoint,					idAI::Event_ResetLookPoint)
	EVENT(AI_CanHitFromAnim,					idAI::Event_CanHitFromAnim)
	EVENT(AI_CheckSearchLook,					idAI::Event_CheckSearchLook)
	EVENT(AI_SetLastVisiblePos,					idAI::Event_SetLastVisiblePos)
	EVENT(AI_GetObservationPosition,			idAI::Event_GetObservationPosition) //Darkmod.
	EVENT(AI_CheckForwardDot,					idAI::Event_CheckForwardDot)
	EVENT(AI_GetObservationViaNodes,			idAI::Event_GetObservationViaNodes)
	EVENT(AI_SetCombatState,					idAI::SetCombatState)
	EVENT(AI_GetAIState,						idAI::Event_GetAIState)
	EVENT(AI_EjectBrass,						idAI::EjectBrass)
	EVENT(AI_StartStunState,					idAI::Event_StartStunState)
	EVENT(AI_IsBleedingOut,						idAI::Event_IsBleedingOut)
	EVENT(AI_GetSkullsaver,						idAI::Event_GetSkullsaver)

	EVENT(AI_SetPathEntity,						idAI::Event_SetPathEntity)

	EVENT(AI_GetLifeState,						idAI::Event_GetLifeState)
		

	EVENT(EV_PostSpawn,							idAI::Event_PostSpawn)
		
		
END_CLASS



/*
=====================
idAI::Event_PostSpawn
=====================
*/
void idAI::Event_PostSpawn()
{
	skullSpawnOrigLoc = gameLocal.LocationForEntity(this);
}

/*
=====================
idAI::Event_Activate
=====================
*/
void idAI::Event_Activate( idEntity *activator ) {
	Activate( activator );
}

/*
=====================
idAI::Event_Touch
=====================
*/
void idAI::Event_Touch( idEntity *other, trace_t *trace ) {
	if ( !enemy.GetEntity() && !other->fl.notarget && ( ReactionTo( other ) & ATTACK_ON_ACTIVATE ) ) {
		Activate( other );
	}
	AI_PUSHED = true;
}

/*
=====================
idAI::Event_FindEnemy
=====================
*/
void idAI::Event_FindEnemy( int useFOV ) {
	int			i;
	idEntity	*ent;
	idActor		*actor;

	if ( gameLocal.InPlayerPVS( this ) ) {
		for ( i = 0; i < gameLocal.numClients ; i++ ) {
			ent = gameLocal.entities[ i ];

			if ( !ent || !ent->IsType( idActor::Type ) ) {
				continue;
			}

			actor = static_cast<idActor *>( ent );
			if ( ( actor->health <= 0 ) || !( ReactionTo( actor ) & ATTACK_ON_SIGHT ) ) {
				continue;
			}

			if ( CanSee( actor, useFOV != 0 ) ) {
				idThread::ReturnEntity( actor );
				return;
			}
		}
	}

	idThread::ReturnEntity( NULL );
}

/*
=====================
idAI::Event_FindEnemyAI
=====================
*/
idEntity* idAI::Event_FindEnemyAI( int useFOV )
{
	idEntity	*ent;
	idEntity	*bestEnemy;
	float		bestDist;
	float		dist;
	idVec3		delta;
	pvsHandle_t pvs;

	pvs = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );

	bestDist = idMath::INFINITY;
	bestEnemy = NULL;

	//BC iterate over all the things that could possibly be my enemy.
	//for (ent = gameLocal.GetLocalPlayer(); ent != NULL; ent = NULL)	
	//for ( ent = gameLocal.activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() )	
	for (ent = gameLocal.aimAssistEntities.Next(); ent != NULL; ent = ent->aimAssistNode.Next())
	{
		

		if (ent->fl.hidden || ent->fl.isDormant ||  ent->health <= 0  || !( ReactionTo( ent ) & ATTACK_ON_SIGHT ) || ent->team == this->team || ent->team == TEAM_NEUTRAL || ent == this)
		{
			continue;
		}

		if (ent->IsType(idPlayer::Type))
		{
			if (static_cast<idPlayer *>(ent)->GetAcroType() != ACROTYPE_NONE && static_cast<idPlayer *>(ent)->wasCaughtEnteringCargoHide == false)
			{
				//player is in some sort of acro hide spot. Skip.
				continue;
			}
		}

		idVec3 lookPoint;
		if (ent->IsType(idActor::Type))
		{
			lookPoint = static_cast<idActor *>(ent)->GetEyePosition();
		}
		else
		{
			lookPoint = ent->GetPhysics()->GetOrigin();
		}

		//If not in PVS, then skip.
		if ( !gameLocal.pvs.InCurrentPVS( pvs, ent->GetPVSAreas(), ent->GetNumPVSAreas() ) )
		{
			if (ai_debugPerception.GetInteger() == 1)
			{
				idVec3 midpoint = (GetEyePosition() + lookPoint) / 2.0f;

				gameRenderWorld->DebugArrow(colorOrange, GetEyePosition(), lookPoint, 2, 1000);
				gameRenderWorld->DrawText("NOT IN PVS", midpoint, .2f, colorOrange, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 100);
			}

			continue;
		}

		
		if (useFOV)
		{
			if (!CheckFOV(lookPoint)) //Do visionbox check.
			{
				continue;
			}
		}

		//Find closest enemy.
		delta = physicsObj.GetOrigin() - ent->GetPhysics()->GetOrigin();
		dist = delta.LengthSqr();
		if ( ( dist < bestDist ) && CanSee( ent, useFOV != 0 ) )
		{
			if (ai_debugPerception.GetInteger() == 1)
			{
				gameRenderWorld->DebugArrow(colorGreen, GetEyePosition(), lookPoint, 2, 1000);
			}

			bestDist = dist;
			bestEnemy = ent;
		}
	}

	gameLocal.pvs.FreeCurrentPVS( pvs );
	idThread::ReturnEntity( bestEnemy );

	return bestEnemy;
}

/*
=====================
idAI::Event_FindEnemyInCombatNodes
=====================
*/
void idAI::Event_FindEnemyInCombatNodes( void ) {
	int				i, j;
	idCombatNode	*node;
	idEntity		*ent;
	idEntity		*targetEnt;
	idActor			*actor;

	if ( !gameLocal.InPlayerPVS( this ) ) {
		// don't locate the player when we're not in his PVS
		idThread::ReturnEntity( NULL );
		return;
	}

	for ( i = 0; i < gameLocal.numClients ; i++ ) {
		ent = gameLocal.entities[ i ];

		if ( !ent || !ent->IsType( idActor::Type ) ) {
			continue;
		}

		actor = static_cast<idActor *>( ent );
		if ( ( actor->health <= 0 ) || !( ReactionTo( actor ) & ATTACK_ON_SIGHT ) ) {
			continue;
		}

		for( j = 0; j < targets.Num(); j++ ) {
			targetEnt = targets[ j ].GetEntity();
			if ( !targetEnt || !targetEnt->IsType( idCombatNode::Type ) ) {
				continue;
			}

			node = static_cast<idCombatNode *>( targetEnt );
			if ( !node->IsDisabled() && node->EntityInView( actor, actor->GetPhysics()->GetOrigin() ) ) {
				idThread::ReturnEntity( actor );
				return;
			}
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

	if ( !team_mate->IsType( idActor::Type ) ) {
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
idAI::Event_HeardSound
=====================
*/
/*
//Returns Entity that made a nearby sound effect.
void idAI::Event_HeardSound( int ignore_team )
{
	// check if we heard any sounds in the last frame
	idActor	*actor = gameLocal.GetAlertEntity();

	if ( actor && ( !ignore_team || ( ReactionTo( actor ) & ATTACK_ON_SIGHT ) ) && gameLocal.InPlayerPVS( this ) )
	{
		idVec3 pos = actor->GetPhysics()->GetOrigin();
		idVec3 org = physicsObj.GetOrigin();
		float dist = ( pos - org ).LengthSqr();
		
		if ( dist < Square( AI_HEARING_RANGE ) )
		{
			idThread::ReturnEntity( actor );
			return;
		}
	}

	idThread::ReturnEntity( NULL );
}
*/
/*
=====================
idAI::Event_SetEnemy
=====================
*/
void idAI::Event_SetEnemy( idEntity *ent ) {
	if ( !ent ) {
		ClearEnemy();
	} else if ( !ent->IsType( idActor::Type ) ) {
		gameLocal.Error( "'%s' is not an idActor (player or ai controlled character)", ent->name.c_str() );
	} else {
		SetEnemy( static_cast<idActor *>( ent ) );
	}
}

/*
=====================
idAI::Event_ClearEnemy
=====================
*/
void idAI::Event_ClearEnemy( void ) {
	ClearEnemy();
}

/*
=====================
idAI::Event_MuzzleFlash
=====================
*/
void idAI::Event_MuzzleFlash( const char *jointname ) {
	idVec3	muzzle;
	idMat3	axis;

	GetMuzzle( jointname, muzzle, axis );
	TriggerWeaponEffects( muzzle );
}

/*
=====================
idAI::Event_CreateMissile
=====================
*/
void idAI::Event_CreateMissile( const char *jointname ) {
	idVec3 muzzle;
	idMat3 axis;

	if ( !projectileDef ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		return idThread::ReturnEntity( NULL );
	}

	GetMuzzle( jointname, muzzle, axis );
	CreateProjectile( muzzle, viewAxis[ 0 ] * physicsObj.GetGravityAxis() );
	if ( projectile.GetEntity() ) {
		if ( !jointname || !jointname[ 0 ] ) {
			projectile.GetEntity()->Bind( this, true );
		} else {
			projectile.GetEntity()->BindToJoint( this, jointname, true );
		}
	}
	idThread::ReturnEntity( projectile.GetEntity() );
}

/*
=====================
idAI::Event_AttackMissile
=====================
*/
void idAI::Event_AttackMissile( const char *jointname )
{
	idProjectile *proj;

	//BC fires projectile.	This is what NPC rifles call.
	proj = LaunchProjectile(jointname, AI_ENEMY_IN_FOV ? enemy.GetEntity() : NULL , true, lastVisibleEnemyPos);	
	
	idThread::ReturnEntity( proj );
}

/*
=====================
idAI::Event_FireMissileAtTarget
=====================
*/
void idAI::Event_FireMissileAtTarget( const char *jointname, const char *targetname ) {
	idEntity		*aent;
	idProjectile	*proj;

	aent = gameLocal.FindEntity( targetname );
	if ( !aent ) {
		gameLocal.Warning( "Entity '%s' not found for 'fireMissileAtTarget'", targetname );
	}

	proj = LaunchProjectile( jointname, aent, false, vec3_zero );
	idThread::ReturnEntity( proj );
}

/*
=====================
idAI::Event_LaunchMissile
=====================
*/
void idAI::Event_LaunchMissile( const idVec3 &org, const idAngles &ang ) {
	idVec3		start;
	trace_t		tr;
	idBounds	projBounds;
	const idClipModel *projClip;
	idMat3		axis;
	float		distance;

	if ( !projectileDef ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		idThread::ReturnEntity( NULL );
		return;
	}

	axis = ang.ToMat3();
	if ( !projectile.GetEntity() ) {
		CreateProjectile( org, axis[ 0 ] );
	}

	// make sure the projectile starts inside the monster bounding box
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	projClip = projectile.GetEntity()->GetPhysics()->GetClipModel();
	projBounds = projClip->GetBounds().Rotate( projClip->GetAxis() );

	// check if the owner bounds is bigger than the projectile bounds
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( org, viewAxis[ 0 ], distance ) ) {
			start = org + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

	gameLocal.clip.Translation( tr, start, org, projClip, projClip->GetAxis(), MASK_SHOT_RENDERMODEL, this );

	// launch the projectile
	idThread::ReturnEntity( projectile.GetEntity() );
	projectile.GetEntity()->Launch( tr.endpos, axis[ 0 ], vec3_origin );
	projectile = NULL;

	TriggerWeaponEffects( tr.endpos );

	lastAttackTime = gameLocal.time;
}


#ifdef _D3XP
/*
=====================
idAI::Event_LaunchProjectile
=====================
*/
void idAI::Event_LaunchProjectile( const char *entityDefName ) {
	idVec3				muzzle, start, dir;
	const idDict		*projDef;
	idMat3				axis;
	const idClipModel	*projClip;
	idBounds			projBounds;
	trace_t				tr;
	idEntity			*ent;
	const char			*clsname;
	float				distance;
	idProjectile		*proj = NULL;

	projDef = gameLocal.FindEntityDefDict( entityDefName );

	gameLocal.SpawnEntityDef( *projDef, &ent, false );
	if ( !ent ) {
		clsname = projectileDef->GetString( "classname" );
		gameLocal.Error( "Could not spawn entityDef '%s'", clsname );
	}

	if ( !ent->IsType( idProjectile::Type ) ) {
		clsname = ent->GetClassname();
		gameLocal.Error( "'%s' is not an idProjectile", clsname );
	}
	proj = ( idProjectile * )ent;

	GetMuzzle( "pistol", muzzle, axis );
	proj->Create( this, muzzle, axis[0] );

	// make sure the projectile starts inside the monster bounding box
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	projClip = proj->GetPhysics()->GetClipModel();
	projBounds = projClip->GetBounds().Rotate( projClip->GetAxis() );
	if ( (ownerBounds - projBounds).RayIntersection( muzzle, viewAxis[ 0 ], distance ) ) {
		start = muzzle + distance * viewAxis[ 0 ];
	} else {
		start = ownerBounds.GetCenter();
	}
	gameLocal.clip.Translation( tr, start, muzzle, projClip, projClip->GetAxis(), MASK_SHOT_RENDERMODEL, this );
	muzzle = tr.endpos;

	GetAimDir( muzzle, enemy.GetEntity(), this, dir );

	proj->Launch( muzzle, dir, vec3_origin );

	TriggerWeaponEffects( muzzle );
}

#endif


/*
=====================
idAI::Event_AttackMelee
=====================
*/
void idAI::Event_AttackMelee( const char *meleeDefName ) {
	bool hit;

	hit = AttackMelee( meleeDefName );
	idThread::ReturnInt( hit );
}

/*
=====================
idAI::Event_DirectDamage
=====================
*/
void idAI::Event_DirectDamage( idEntity *damageTarget, const char *damageDefName ) {
	DirectDamage( damageDefName, damageTarget );
}

/*
=====================
idAI::Event_RadiusDamageFromJoint
=====================
*/
void idAI::Event_RadiusDamageFromJoint( const char *jointname, const char *damageDefName ) {
	jointHandle_t joint;
	idVec3 org;
	idMat3 axis;

	if ( !jointname || !jointname[ 0 ] ) {
		org = physicsObj.GetOrigin();
	} else {
		joint = animator.GetJointHandle( jointname );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Unknown joint '%s' on %s", jointname, GetEntityDefName() );
		}
		GetJointWorldTransform( joint, gameLocal.time, org, axis );
	}

	gameLocal.RadiusDamage( org, this, this, this, this, damageDefName );
}

/*
=====================
idAI::Event_RandomPath
=====================
*/
void idAI::Event_RandomPath( void ) {
	idPathCorner *path;

	path = idPathCorner::RandomPath( this, NULL );
	idThread::ReturnEntity( path );
}

/*
=====================
idAI::Event_BeginAttack
=====================
*/
void idAI::Event_BeginAttack( const char *name ) {
	BeginAttack( name );
}

/*
=====================
idAI::Event_EndAttack
=====================
*/
void idAI::Event_EndAttack( void ) {
	EndAttack();
}

/*
=====================
idAI::Event_MeleeAttackToJoint
=====================
*/
void idAI::Event_MeleeAttackToJoint( const char *jointname, const char *meleeDefName ) {
	jointHandle_t	joint;
	idVec3			start;
	idVec3			end;
	idMat3			axis;
	trace_t			trace;
	idEntity		*hitEnt;

	joint = animator.GetJointHandle( jointname );
	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "Unknown joint '%s' on %s", jointname, GetEntityDefName() );
	}
	animator.GetJointTransform( joint, gameLocal.time, end, axis );
	end = physicsObj.GetOrigin() + ( end + modelOffset ) * viewAxis * physicsObj.GetGravityAxis();
	start = GetEyePosition();

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, start, end, gameLocal.msec );
	}

	gameLocal.clip.TranslationEntities( trace, start, end, NULL, mat3_identity, MASK_SHOT_BOUNDINGBOX, this );
	if ( trace.fraction < 1.0f ) {
		hitEnt = gameLocal.GetTraceEntity( trace );
		if ( hitEnt && hitEnt->IsType( idActor::Type ) ) {
			DirectDamage( meleeDefName, hitEnt );
			idThread::ReturnInt( true );
			return;
		}
	}

	idThread::ReturnInt( false );
}

/*
=====================
idAI::Event_CanBecomeSolid
=====================
*/
void idAI::Event_CanBecomeSolid( void ) {
	int			i;
	int			num;
#ifdef _D3XP
	bool		returnValue = true;
#endif
	idEntity *	hit;
	idClipModel *cm;
	idClipModel *clipModels[ MAX_GENTITIES ];

	num = gameLocal.clip.ClipModelsTouchingBounds( physicsObj.GetAbsBounds(), MASK_MONSTERSOLID, clipModels, MAX_GENTITIES );
	for ( i = 0; i < num; i++ ) {
		cm = clipModels[ i ];

		// don't check render entities
		if ( cm->IsRenderModel() ) {
			continue;
		}

		hit = cm->GetEntity();
		if ( ( hit == this ) || !hit->fl.takedamage ) {
			continue;
		}

#ifdef _D3XP
		if ( (spawnClearMoveables && hit->IsType( idMoveable::Type )) || (hit->IsType( idBarrel::Type ) || hit->IsType( idExplodingBarrel::Type) ) ) {
			idVec3 push;
			push = hit->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
			push.z = 30.f;
			push.NormalizeFast();
			if ( (idMath::Fabs(push.x) < 0.15f) && (idMath::Fabs(push.y) < 0.15f) ) {
				push.x = 10.f; push.y = 10.f; push.z = 15.f;
				push.NormalizeFast();
			}
			push *= 300.f;
			hit->GetPhysics()->SetLinearVelocity( push );
		}
#endif

		if ( physicsObj.ClipContents( cm ) ) {
#ifdef _D3XP
			returnValue = false;
#else
			idThread::ReturnFloat( false );
			return;
#endif
		}
	}

#ifdef _D3XP
	idThread::ReturnFloat( returnValue );
#else
	idThread::ReturnFloat( true );
#endif
}

/*
=====================
idAI::Event_BecomeSolid
=====================
*/
void idAI::Event_BecomeSolid( void ) {
	physicsObj.EnableClip();
	if ( spawnArgs.GetBool( "big_monster" ) ) {
		physicsObj.SetContents( 0 );
	} else if ( use_combat_bbox ) {
		physicsObj.SetContents( CONTENTS_BODY|CONTENTS_SOLID );
	} else {
		physicsObj.SetContents( CONTENTS_BODY );
	}
	physicsObj.GetClipModel()->Link( gameLocal.clip );
	fl.takedamage = !spawnArgs.GetBool( "noDamage" );
}

/*
=====================
idAI::Event_BecomeNonSolid
=====================
*/
void idAI::Event_BecomeNonSolid( void ) {
	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();
}

/*
=====================
idAI::Event_BecomeRagdoll
=====================
*/
void idAI::Event_BecomeRagdoll( void ) {
	bool result;

	result = StartRagdoll();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Event_StopRagdoll
=====================
*/
void idAI::Event_StopRagdoll( void ) {
	StopRagdoll();

	// set back the monster physics
	SetPhysics( &physicsObj );
}

/*
=====================
idAI::Event_SetHealth
=====================
*/
void idAI::Event_SetHealth( float newHealth ) {
	health = newHealth;
	fl.takedamage = true;
	if ( health > 0 ) {
		AI_DEAD = false;
	} else {
		AI_DEAD = true;
	}
}

/*
=====================
idAI::Event_GetHealth
=====================
*/
void idAI::Event_GetHealth( void ) {
	idThread::ReturnFloat( health );
}

/*
=====================
idAI::Event_AllowDamage
=====================
*/
void idAI::Event_AllowDamage( void ) {
	fl.takedamage = true;
}

/*
=====================
idAI::Event_IgnoreDamage
=====================
*/
void idAI::Event_IgnoreDamage( void ) {
	fl.takedamage = false;
}

/*
=====================
idAI::Event_GetCurrentYaw
=====================
*/
void idAI::Event_GetCurrentYaw( void ) {
	idThread::ReturnFloat( current_yaw );
}

/*
=====================
idAI::Event_TurnTo
=====================
*/
void idAI::Event_TurnTo( float angle ) {
	TurnToward( angle );
}

/*
=====================
idAI::Event_TurnToPos
=====================
*/
void idAI::Event_TurnToPos( const idVec3 &pos ) {
	TurnToward( pos );
}

/*
=====================
idAI::Event_TurnToEntity
=====================
*/
void idAI::Event_TurnToEntity( idEntity *ent ) {
	if ( ent ) {
		TurnToward( ent->GetPhysics()->GetOrigin() );
	}
}

/*
=====================
idAI::Event_MoveStatus
=====================
*/
void idAI::Event_MoveStatus( void ) {
	idThread::ReturnInt( move.moveStatus );
}

/*
=====================
idAI::Event_StopMove
=====================
*/
void idAI::Event_StopMove( void ) {
	StopMove( MOVE_STATUS_DONE );
}

/*
=====================
idAI::Event_MoveToCover
=====================
*/
void idAI::Event_MoveToCover( void ) {
	idActor *enemyEnt = enemy.GetEntity();

	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	if ( !enemyEnt || !MoveToCover( enemyEnt, lastVisibleEnemyPos ) ) {
		return;
	}
}

/*
=====================
idAI::Event_MoveToEnemy
=====================
*/
void idAI::Event_MoveToEnemy( void ) {
	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	if ( !enemy.GetEntity() || !MoveToEnemy() )
	{
		if (!enemy.GetEntity())
			idThread::ReturnInt(-1);
		else
			idThread::ReturnInt(0);

		return;
	}

	idThread::ReturnInt(1);
}

/*
=====================
idAI::Event_MoveToEnemyHeight
=====================
*/
void idAI::Event_MoveToEnemyHeight( void ) {
	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	MoveToEnemyHeight();
}

/*
=====================
idAI::Event_MoveOutOfRange
=====================
*/
void idAI::Event_MoveOutOfRange( idEntity *entity, float range ) {
	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	MoveOutOfRange( entity, range );
}

/*
=====================
idAI::Event_MoveToAttackPosition
=====================
*/
void idAI::Event_MoveToAttackPosition( idEntity *entity, const char *attack_anim ) {
	int anim;

	StopMove( MOVE_STATUS_DEST_NOT_FOUND );

	anim = GetAnim( ANIMCHANNEL_LEGS, attack_anim );
	if ( !anim ) {
		gameLocal.Error( "Unknown anim '%s'", attack_anim );
	}

	MoveToAttackPosition( entity, anim );
}

/*
=====================
idAI::Event_MoveToEntity
=====================
*/
void idAI::Event_MoveToEntity( idEntity *ent ) {
	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	if ( ent ) {
		MoveToEntity( ent );
	}
}

/*
=====================
idAI::Event_MoveToPosition
=====================
*/
void idAI::Event_MoveToPosition( const idVec3 &pos ) {
	StopMove( MOVE_STATUS_DONE );
	MoveToPosition( pos );
}

/*
=====================
idAI::Event_SlideTo
=====================
*/
void idAI::Event_SlideTo( const idVec3 &pos, float time ) {
	SlideToPosition( pos, time );
}
/*
=====================
idAI::Event_Wander
=====================
*/
void idAI::Event_Wander( void ) {
	WanderAround();
}

/*
=====================
idAI::Event_FacingIdeal
=====================
*/
void idAI::Event_FacingIdeal( void ) {
	bool facing = FacingIdeal();
	idThread::ReturnInt( facing );
}

/*
=====================
idAI::Event_FaceEnemy
=====================
*/
void idAI::Event_FaceEnemy( void ) {
	FaceEnemy();
}

/*
=====================
idAI::Event_FaceEntity
=====================
*/
void idAI::Event_FaceEntity( idEntity *ent ) {
	FaceEntity( ent );
}

/*
=====================
idAI::Event_WaitAction
=====================
*/
void idAI::Event_WaitAction( const char *waitForState ) {
	if ( idThread::BeginMultiFrameEvent( this, &AI_WaitAction ) ) {
		SetWaitState( waitForState );
	}

	if ( !WaitState() ) {
		idThread::EndMultiFrameEvent( this, &AI_WaitAction );
	}
}

/*
=====================
idAI::Event_GetCombatNode
=====================
*/
void idAI::Event_GetCombatNode( void ) {
	int				i;
	float			dist;
	idEntity		*targetEnt;
	idCombatNode	*node;
	float			bestDist;
	idCombatNode	*bestNode;
	idActor			*enemyEnt = enemy.GetEntity();

	if ( !targets.Num() ) {
		// no combat nodes
		idThread::ReturnEntity( NULL );
		return;
	}

	if ( !enemyEnt || !EnemyPositionValid() ) {
		// don't return a combat node if we don't have an enemy or
		// if we can see he's not in the last place we saw him

#ifdef _D3XP
		if ( team == 0 ) {
			// find the closest attack node to the player
			bestNode = NULL;
			const idVec3 &myPos = physicsObj.GetOrigin();
			const idVec3 &playerPos = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();

			bestDist = ( myPos - playerPos ).LengthSqr();

			for( i = 0; i < targets.Num(); i++ ) {
				targetEnt = targets[ i ].GetEntity();
				if ( !targetEnt || !targetEnt->IsType( idCombatNode::Type ) ) {
					continue;
				}

				node = static_cast<idCombatNode *>( targetEnt );
				if ( !node->IsDisabled() ) {
					idVec3 org = node->GetPhysics()->GetOrigin();
					dist = ( playerPos - org ).LengthSqr();
					if ( dist < bestDist ) {
						bestNode = node;
						bestDist = dist;
					}
				}
			}

			idThread::ReturnEntity( bestNode );
			return;
		}
#endif

		idThread::ReturnEntity( NULL );
		return;
	}

	// find the closest attack node that can see our enemy and is closer than our enemy
	bestNode = NULL;
	const idVec3 &myPos = physicsObj.GetOrigin();
	bestDist = ( myPos - lastVisibleEnemyPos ).LengthSqr();
	for( i = 0; i < targets.Num(); i++ ) {
		targetEnt = targets[ i ].GetEntity();
		if ( !targetEnt || !targetEnt->IsType( idCombatNode::Type ) ) {
			continue;
		}

		node = static_cast<idCombatNode *>( targetEnt );
		if ( !node->IsDisabled() && node->EntityInView( enemyEnt, lastVisibleEnemyPos ) ) {
			idVec3 org = node->GetPhysics()->GetOrigin();
			dist = ( myPos - org ).LengthSqr();
			if ( dist < bestDist ) {
				bestNode = node;
				bestDist = dist;
			}
		}
	}

	idThread::ReturnEntity( bestNode );
}

/*
=====================
idAI::Event_EnemyInCombatCone
=====================
*/
void idAI::Event_EnemyInCombatCone( idEntity *ent, int use_current_enemy_location ) {
	idCombatNode	*node;
	bool			result;
	idActor			*enemyEnt = enemy.GetEntity();

	if ( !targets.Num() ) {
		// no combat nodes
		idThread::ReturnInt( false );
		return;
	}

	if ( !enemyEnt ) {
		// have to have an enemy
		idThread::ReturnInt( false );
		return;
	}

	if ( !ent || !ent->IsType( idCombatNode::Type ) ) {
		// not a combat node
		idThread::ReturnInt( false );
		return;
	}

#ifdef _D3XP
	//Allow the level designers define attack nodes that the enemy should never leave.
	//This is different that the turrent type combat nodes because they can play an animation
	if(ent->spawnArgs.GetBool("neverLeave", "0")) {
		idThread::ReturnInt( true );
		return;
	}
#endif

	node = static_cast<idCombatNode *>( ent );
	if ( use_current_enemy_location ) {
		const idVec3 &pos = enemyEnt->GetPhysics()->GetOrigin();
		result = node->EntityInView( enemyEnt, pos );
	} else {
		result = node->EntityInView( enemyEnt, lastVisibleEnemyPos );
	}

	idThread::ReturnInt( result );
}

/*
=====================
idAI::Event_WaitMove
=====================
*/
void idAI::Event_WaitMove( void ) {
	idThread::BeginMultiFrameEvent( this, &AI_WaitMove );

	if ( MoveDone() ) {
		idThread::EndMultiFrameEvent( this, &AI_WaitMove );
	}
}

/*
=====================
idAI::Event_GetJumpVelocity
=====================
*/
void idAI::Event_GetJumpVelocity( const idVec3 &pos, float speed, float max_height ) {
	idVec3 start;
	idVec3 end;
	idVec3 dir;
	float dist;
	bool result;
	idEntity *enemyEnt = enemy.GetEntity();

	if ( !enemyEnt ) {
		idThread::ReturnVector( vec3_zero );
		return;
	}

	if ( speed <= 0.0f ) {
		gameLocal.Error( "Invalid speed.  speed must be > 0." );
	}

	start = physicsObj.GetOrigin();
	end = pos;
	dir = end - start;
	dist = dir.Normalize();
	if ( dist > 16.0f ) {
		dist -= 16.0f;
		end -= dir * 16.0f;
	}

	result = PredictTrajectory( start, end, speed, physicsObj.GetGravity(), physicsObj.GetClipModel(), MASK_MONSTERSOLID, max_height, this, enemyEnt, ai_debugMove.GetBool() ? 4000 : 0, dir );
	if ( result ) {
		idThread::ReturnVector( dir * speed );
	} else {
		idThread::ReturnVector( vec3_zero );
	}
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
	relYaw = idMath::AngleNormalize180( ideal_yaw - yaw );
	if ( idMath::Fabs( relYaw ) < ( attack_cone * 0.5f ) ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
=====================
idAI::Event_CanSeeEntity
=====================
*/
void idAI::Event_CanSeeEntity( idEntity *ent , bool idleMode)
{
	if ( !ent )
	{
		idThread::ReturnInt( false );
		return;
	}

	bool cansee = CanSee( ent, true );
	idThread::ReturnInt( cansee );
}

/*
=====================
idAI::Event_SetTalkTarget
=====================
*/
void idAI::Event_SetTalkTarget( idEntity *target ) {
	if ( target && !target->IsType( idActor::Type ) ) {
		gameLocal.Error( "Cannot set talk target to '%s'.  Not a character or player.", target->GetName() );
	}
	talkTarget = static_cast<idActor *>( target );
	if ( target ) {
		AI_TALK = true;
	} else {
		AI_TALK = false;
	}
}

/*
=====================
idAI::Event_GetTalkTarget
=====================
*/
void idAI::Event_GetTalkTarget( void ) {
	idThread::ReturnEntity( talkTarget.GetEntity() );
}

/*
================
idAI::Event_SetTalkState
================
*/
void idAI::Event_SetTalkState( int state ) {
	if ( ( state < 0 ) || ( state >= NUM_TALK_STATES ) ) {
		gameLocal.Error( "Invalid talk state (%d)", state );
	}

	talk_state = static_cast<talkState_t>( state );
}

/*
=====================
idAI::Event_EnemyRange
=====================
*/
void idAI::Event_EnemyRange( void ) {
	float dist;
	idActor *enemyEnt = enemy.GetEntity();

	if ( enemyEnt ) {
		dist = ( enemyEnt->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin() ).Length();
	} else {
		// Just some really high number
		dist = idMath::INFINITY;
	}

	idThread::ReturnFloat( dist );
}

/*
=====================
idAI::Event_EnemyRange2D
=====================
*/
void idAI::Event_EnemyRange2D( void ) {
	float dist;
	idActor *enemyEnt = enemy.GetEntity();

	if ( enemyEnt ) {
		dist = ( enemyEnt->GetPhysics()->GetOrigin().ToVec2() - GetPhysics()->GetOrigin().ToVec2() ).Length();
	} else {
		// Just some really high number
		dist = idMath::INFINITY;
	}

	idThread::ReturnFloat( dist );
}

/*
=====================
idAI::Event_GetEnemy
=====================
*/
void idAI::Event_GetEnemy( void ) {
	idThread::ReturnEntity( enemy.GetEntity() );
}

/*
=====================
idAI::Event_GetEnemyPos
=====================
*/
void idAI::Event_GetEnemyPos( void ) {
	idThread::ReturnVector( lastVisibleEnemyPos );
}

/*
=====================
idAI::Event_GetEnemyEyePos
=====================
*/
void idAI::Event_GetEnemyEyePos( void ) {
	idThread::ReturnVector( lastVisibleEnemyPos + lastVisibleEnemyEyeOffset );
}

/*
=====================
idAI::Event_PredictEnemyPos
=====================
*/
void idAI::Event_PredictEnemyPos( float time ) {
	predictedPath_t path;
	idActor *enemyEnt = enemy.GetEntity();

	// if no enemy set
	if ( !enemyEnt ) {
		idThread::ReturnVector( physicsObj.GetOrigin() );
		return;
	}

	// predict the enemy movement
	idAI::PredictPath( enemyEnt, aas, lastVisibleEnemyPos, enemyEnt->GetPhysics()->GetLinearVelocity(), SEC2MS( time ), SEC2MS( time ), ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	idThread::ReturnVector( path.endPos );
}

/*
=====================
idAI::Event_CanHitEnemy
=====================
*/
void idAI::Event_CanHitEnemy( void ) {
	trace_t	tr;
	idEntity *hit;

	idActor *enemyEnt = enemy.GetEntity();
	if ( !AI_ENEMY_VISIBLE || !enemyEnt ) {
		idThread::ReturnInt( false );
		return;
	}

	// don't check twice per frame
	if ( gameLocal.time == lastHitCheckTime ) {
		idThread::ReturnInt( lastHitCheckResult );
		return;
	}

	lastHitCheckTime = gameLocal.time;

	idVec3 toPos = enemyEnt->GetEyePosition();
	idVec3 eye = GetEyePosition();
	idVec3 dir;

	// expand the ray out as far as possible so we can detect anything behind the enemy
	dir = toPos - eye;
	dir.Normalize();
	toPos = eye + dir * MAX_WORLD_SIZE;
	gameLocal.clip.TracePoint( tr, eye, toPos, MASK_SHOT_BOUNDINGBOX, this );
	hit = gameLocal.GetTraceEntity( tr );
	if ( tr.fraction >= 1.0f || ( hit == enemyEnt ) ) {
		lastHitCheckResult = true;
	} else if ( ( tr.fraction < 1.0f ) && ( hit->IsType( idAI::Type ) ) &&
		( static_cast<idAI *>( hit )->team != team ) ) {
		lastHitCheckResult = true;
	} else {
		lastHitCheckResult = false;
	}

	idThread::ReturnInt( lastHitCheckResult );
}

/*
=====================
idAI::Event_CanHitEnemyFromAnim
=====================
*/
void idAI::Event_CanHitEnemyFromAnim( const char *animname ) {
	int		anim;
	idVec3	dir;
	idVec3	local_dir;
	idVec3	fromPos;
	idMat3	axis;
	idVec3	start;
	trace_t	tr;
	float	distance;

	idActor *enemyEnt = enemy.GetEntity();
	if ( !AI_ENEMY_VISIBLE || !enemyEnt ) {
		idThread::ReturnInt( false );
		return;
	}

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {
		idThread::ReturnInt( false );
		return;
	}

	// just do a ray test if close enough
	if ( enemyEnt->GetPhysics()->GetAbsBounds().IntersectsBounds( physicsObj.GetAbsBounds().Expand( 16.0f ) ) ) {
		Event_CanHitEnemy();
		return;
	}

	// calculate the world transform of the launch position
	const idVec3 &org = physicsObj.GetOrigin();
	dir = lastVisibleEnemyPos - org;
	physicsObj.GetGravityAxis().ProjectVector( dir, local_dir );
	local_dir.z = 0.0f;
	local_dir.ToVec2().Normalize();
	axis = local_dir.ToMat3();
	fromPos = physicsObj.GetOrigin() + missileLaunchOffset[ anim ] * axis;

	if ( projectileClipModel == NULL ) {
		CreateProjectileClipModel();
	}

	// check if the owner bounds is bigger than the projectile bounds
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	const idBounds &projBounds = projectileClipModel->GetBounds();
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( org, viewAxis[ 0 ], distance ) ) {
			start = org + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

	gameLocal.clip.Translation( tr, start, fromPos, projectileClipModel, mat3_identity, MASK_SHOT_RENDERMODEL, this );
	fromPos = tr.endpos;

	if ( GetAimDir( fromPos, enemy.GetEntity(), this, dir ) ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
=====================
idAI::Event_CanHitEnemyFromJoint
=====================
*/
void idAI::Event_CanHitEnemyFromJoint( const char *jointname ) {
	trace_t	tr;
	idVec3	muzzle;
	idMat3	axis;
	idVec3	start;
	float	distance;

	idActor *enemyEnt = enemy.GetEntity();
	if ( !AI_ENEMY_VISIBLE || !enemyEnt ) {
		idThread::ReturnInt( false );
		return;
	}

	// don't check twice per frame
	if ( gameLocal.time == lastHitCheckTime ) {
		idThread::ReturnInt( lastHitCheckResult );
		return;
	}

	lastHitCheckTime = gameLocal.time;

	const idVec3 &org = physicsObj.GetOrigin();
	idVec3 toPos = enemyEnt->GetEyePosition();
	jointHandle_t joint = animator.GetJointHandle( jointname );
	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "Unknown joint '%s' on %s", jointname, GetEntityDefName() );
	}
	animator.GetJointTransform( joint, gameLocal.time, muzzle, axis );
	muzzle = org + ( muzzle + modelOffset ) * viewAxis * physicsObj.GetGravityAxis();

	if ( projectileClipModel == NULL ) {
		CreateProjectileClipModel();
	}

	// check if the owner bounds is bigger than the projectile bounds
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	const idBounds &projBounds = projectileClipModel->GetBounds();
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( org, viewAxis[ 0 ], distance ) ) {
			start = org + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

	gameLocal.clip.Translation( tr, start, muzzle, projectileClipModel, mat3_identity, MASK_SHOT_BOUNDINGBOX, this );
	muzzle = tr.endpos;

	gameLocal.clip.Translation( tr, muzzle, toPos, projectileClipModel, mat3_identity, MASK_SHOT_BOUNDINGBOX, this );
	if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == enemyEnt ) ) {
		lastHitCheckResult = true;
	} else {
		lastHitCheckResult = false;
	}

	idThread::ReturnInt( lastHitCheckResult );
}

/*
=====================
idAI::Event_EnemyPositionValid
=====================
*/
void idAI::Event_EnemyPositionValid( void ) {
	bool result;

	result = EnemyPositionValid();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Event_ChargeAttack
=====================
*/
void idAI::Event_ChargeAttack( const char *damageDef ) {
	idActor *enemyEnt = enemy.GetEntity();

	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	if ( enemyEnt ) {
		idVec3 enemyOrg;

		if ( move.moveType == MOVETYPE_FLY ) {
			// position destination so that we're in the enemy's view
			enemyOrg = enemyEnt->GetEyePosition();
			enemyOrg -= enemyEnt->GetPhysics()->GetGravityNormal() * fly_offset;
		} else {
			enemyOrg = enemyEnt->GetPhysics()->GetOrigin();
		}

		BeginAttack( damageDef );
		DirectMoveToPosition( enemyOrg );
		TurnToward( enemyOrg );
	}
}

/*
=====================
idAI::Event_TestChargeAttack
=====================
*/
void idAI::Event_TestChargeAttack( void ) {
	idActor *enemyEnt = enemy.GetEntity();
	predictedPath_t path;
	idVec3 end;

	if ( !enemyEnt ) {
		idThread::ReturnFloat( 0.0f );
		return;
	}

	if ( move.moveType == MOVETYPE_FLY ) {
		// position destination so that we're in the enemy's view
		end = enemyEnt->GetEyePosition();
		end -= enemyEnt->GetPhysics()->GetGravityNormal() * fly_offset;
	} else {
		end = enemyEnt->GetPhysics()->GetOrigin();
	}

	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), end - physicsObj.GetOrigin(), 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), end, gameLocal.msec );
		gameRenderWorld->DebugBounds( path.endEvent == 0 ? colorYellow : colorRed, physicsObj.GetBounds(), end, gameLocal.msec );
	}

	if ( ( path.endEvent == 0 ) || ( path.blockingEntity == enemyEnt ) ) {
		idVec3 delta = end - physicsObj.GetOrigin();
		float time = delta.LengthFast();
		idThread::ReturnFloat( time );
	} else {
		idThread::ReturnFloat( 0.0f );
	}
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
	idActor			*enemyEnt;

	enemyEnt = enemy.GetEntity();
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

	if ( ai_debugMove.GetBool() ) {
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
void idAI::Event_TestAnimMove( const char *animname )
{
	idThread::ReturnInt(TestAnimMove(animname));
}

bool idAI::TestAnimMove(const char *animname)
{
	int				anim;
	predictedPath_t path;
	idVec3			moveVec;

	anim = GetAnim(ANIMCHANNEL_LEGS, animname);
	if (!anim) {
		gameLocal.DWarning("missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName());
		return false;
	}

	moveVec = animator.TotalMovementDelta(anim) * idAngles(0.0f, ideal_yaw, 0.0f).ToMat3() * physicsObj.GetGravityAxis();
	idAI::PredictPath(this, aas, physicsObj.GetOrigin(), moveVec, 1000, 1000, (move.moveType == MOVETYPE_FLY) ? SE_BLOCKED : (SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA), path);

	if (ai_debugMove.GetBool()) {
		gameRenderWorld->DebugLine(colorGreen, physicsObj.GetOrigin(), physicsObj.GetOrigin() + moveVec, gameLocal.msec);
		gameRenderWorld->DebugBounds(path.endEvent == 0 ? colorYellow : colorRed, physicsObj.GetBounds(), physicsObj.GetOrigin() + moveVec, gameLocal.msec);
	}

	return (path.endEvent == 0);
}


/*
=====================
idAI::Event_TestMoveToPosition
=====================
*/
void idAI::Event_TestMoveToPosition( const idVec3 &position ) {
	predictedPath_t path;

	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), position - physicsObj.GetOrigin(), 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( ai_debugMove.GetBool() ) {
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

	idThread::ReturnInt( path.blockingEntity && ( path.blockingEntity == enemy.GetEntity() ) );
}

/*
=====================
idAI::Event_Shrivel
=====================
*/
void idAI::Event_Shrivel( float shrivel_time ) {
	float t;

	if ( idThread::BeginMultiFrameEvent( this, &AI_Shrivel ) ) {
		if ( shrivel_time <= 0.0f ) {
			idThread::EndMultiFrameEvent( this, &AI_Shrivel );
			return;
		}

		shrivel_rate = 0.001f / shrivel_time;
		shrivel_start = gameLocal.time;
	}

	t = ( gameLocal.time - shrivel_start ) * shrivel_rate;
	if ( t > 0.25f ) {
		renderEntity.noShadow = true;
	}
	if ( t > 1.0f ) {
		t = 1.0f;
		idThread::EndMultiFrameEvent( this, &AI_Shrivel );
	}

	renderEntity.shaderParms[ SHADERPARM_MD5_SKINSCALE ] = 1.0f - t * 0.5f;
	UpdateVisuals();
}

/*
=====================
idAI::Event_PreBurn
=====================
*/
void idAI::Event_PreBurn( void ) {
#ifdef _D3XP
	// No grabbing after the burn has started!
	noGrab = true;
#endif

	// for now this just turns shadows off
	renderEntity.noShadow = true;
}

/*
=====================
idAI::Event_Burn
=====================
*/
void idAI::Event_Burn( void ) {
	renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
	SpawnParticles( "smoke_burnParticleSystem" );
	UpdateVisuals();
}

/*
=====================
idAI::Event_ClearBurn
=====================
*/
void idAI::Event_ClearBurn( void ) {
	renderEntity.noShadow = spawnArgs.GetBool( "noshadows" );
	renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = 0.0f;
	UpdateVisuals();
}

/*
=====================
idAI::Event_SetSmokeVisibility
=====================
*/
void idAI::Event_SetSmokeVisibility( int num, int on ) {
	int i;
	int time;

	if ( num >= particles.Num() ) {
		gameLocal.Warning( "Particle #%d out of range (%d particles) on entity '%s'", num, particles.Num(), name.c_str() );
		return;
	}

	if ( on != 0 ) {
		time = gameLocal.time;
		BecomeActive( TH_UPDATEPARTICLES );
	} else {
		time = 0;
	}

	if ( num >= 0 ) {
		particles[ num ].time = time;
	} else {
		for ( i = 0; i < particles.Num(); i++ ) {
			particles[ i ].time = time;
		}
	}

	UpdateVisuals();
}

/*
=====================
idAI::Event_NumSmokeEmitters
=====================
*/
void idAI::Event_NumSmokeEmitters( void ) {
	idThread::ReturnInt( particles.Num() );
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
idAI::Event_GetTurnDelta
=====================
*/
void idAI::Event_GetTurnDelta( void ) {
	float amount;

	if ( turnRate ) {
		amount = idMath::AngleNormalize180( ideal_yaw - current_yaw );
		idThread::ReturnFloat( amount );
	} else {
		idThread::ReturnFloat( 0.0f );
	}
}

/*
=====================
idAI::Event_GetMoveType
=====================
*/
void idAI::Event_GetMoveType( void ) {
	idThread::ReturnInt( move.moveType );
}

/*
=====================
idAI::Event_SetMoveTypes
=====================
*/
void idAI::Event_SetMoveType( int moveType ) {
	if ( ( moveType < 0 ) || ( moveType >= NUM_MOVETYPES ) ) {
		gameLocal.Error( "Invalid movetype %d", moveType );
	}

	move.moveType = static_cast<moveType_t>( moveType );
	if ( move.moveType == MOVETYPE_FLY ) {
		travelFlags = TFL_WALK|TFL_AIR|TFL_FLY;
	} else {
		travelFlags = TFL_WALK|TFL_AIR;
	}
}

/*
=====================
idAI::Event_SaveMove
=====================
*/
void idAI::Event_SaveMove( void ) {
	savedMove = move;
}

/*
=====================
idAI::Event_RestoreMove
=====================
*/
void idAI::Event_RestoreMove( void ) {
	idVec3 goalPos;
	idVec3 dest;

	switch( savedMove.moveCommand ) {
	case MOVE_NONE :
		StopMove( savedMove.moveStatus );
		break;

	case MOVE_FACE_ENEMY :
		FaceEnemy();
		break;

	case MOVE_FACE_ENTITY :
		FaceEntity( savedMove.goalEntity.GetEntity() );
		break;

	case MOVE_TO_ENEMY :
		MoveToEnemy();
		break;

	case MOVE_TO_ENEMYHEIGHT :
		MoveToEnemyHeight();
		break;

	case MOVE_TO_ENTITY :
		MoveToEntity( savedMove.goalEntity.GetEntity() );
		break;

	case MOVE_OUT_OF_RANGE :
		MoveOutOfRange( savedMove.goalEntity.GetEntity(), savedMove.range );
		break;

	case MOVE_TO_ATTACK_POSITION :
		MoveToAttackPosition( savedMove.goalEntity.GetEntity(), savedMove.anim );
		break;

	case MOVE_TO_COVER :
		MoveToCover( savedMove.goalEntity.GetEntity(), lastVisibleEnemyPos );
		break;

	case MOVE_TO_POSITION :
		MoveToPosition( savedMove.moveDest );
		break;

	case MOVE_TO_POSITION_DIRECT :
		DirectMoveToPosition( savedMove.moveDest );
		break;

	case MOVE_SLIDE_TO_POSITION :
		SlideToPosition( savedMove.moveDest, savedMove.duration );
		break;

	case MOVE_WANDER :
		WanderAround();
		break;
	}

	if ( GetMovePos( goalPos ) ) {
		CheckObstacleAvoidance( goalPos, dest );
	}
}

/*
=====================
idAI::Event_AllowMovement
=====================
*/
void idAI::Event_AllowMovement( float flag ) {
	allowMove = ( flag != 0.0f );
}

/*
=====================
idAI::Event_JumpFrame
=====================
*/
void idAI::Event_JumpFrame( void ) {
	AI_JUMP = true;
}

/*
=====================
idAI::Event_EnableClip
=====================
*/
void idAI::Event_EnableClip( void ) {
	physicsObj.SetClipMask( MASK_MONSTERSOLID );
	disableGravity = false;
}

/*
=====================
idAI::Event_DisableClip
=====================
*/
void idAI::Event_DisableClip( void ) {
	physicsObj.SetClipMask( 0 );
	disableGravity = true;
}

/*
=====================
idAI::Event_EnableGravity
=====================
*/
void idAI::Event_EnableGravity( void ) {
	disableGravity = false;
}

/*
=====================
idAI::Event_DisableGravity
=====================
*/
void idAI::Event_DisableGravity( void ) {
	disableGravity = true;
}

/*
=====================
idAI::Event_EnableAFPush
=====================
*/
void idAI::Event_EnableAFPush( void ) {
	af_push_moveables = true;
}

/*
=====================
idAI::Event_DisableAFPush
=====================
*/
void idAI::Event_DisableAFPush( void ) {
	af_push_moveables = false;
}

/*
=====================
idAI::Event_SetFlySpeed
=====================
*/
void idAI::Event_SetFlySpeed( float speed ) {
	if ( move.speed == fly_speed ) {
		move.speed = speed;
	}
	fly_speed = speed;
}

/*
================
idAI::Event_SetFlyOffset
================
*/
void idAI::Event_SetFlyOffset( int offset ) {
	fly_offset = offset;
}

/*
================
idAI::Event_ClearFlyOffset
================
*/
void idAI::Event_ClearFlyOffset( void ) {
	spawnArgs.GetInt( "fly_offset",	"0", fly_offset );
}

/*
=====================
idAI::Event_GetClosestHiddenTarget
=====================
*/
void idAI::Event_GetClosestHiddenTarget( const char *type ) {
	int	i;
	idEntity *ent;
	idEntity *bestEnt;
	float time;
	float bestTime;
	const idVec3 &org = physicsObj.GetOrigin();
	idActor *enemyEnt = enemy.GetEntity();

	if ( !enemyEnt ) {
		// no enemy to hide from
		idThread::ReturnEntity( NULL );
		return;
	}

	if ( targets.Num() == 1 ) {
		ent = targets[ 0 ].GetEntity();
		if ( ent && idStr::Cmp( ent->GetEntityDefName(), type ) == 0 ) {
			if ( !EntityCanSeePos( enemyEnt, lastVisibleEnemyPos, ent->GetPhysics()->GetOrigin() ) ) {
				idThread::ReturnEntity( ent );
				return;
			}
		}
		idThread::ReturnEntity( NULL );
		return;
	}

	bestEnt = NULL;
	bestTime = idMath::INFINITY;
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent && idStr::Cmp( ent->GetEntityDefName(), type ) == 0 ) {
			const idVec3 &destOrg = ent->GetPhysics()->GetOrigin();
			time = TravelDistance( org, destOrg );
			if ( ( time >= 0.0f ) && ( time < bestTime ) ) {
				if ( !EntityCanSeePos( enemyEnt, lastVisibleEnemyPos, destOrg ) ) {
					bestEnt = ent;
					bestTime = time;
				}
			}
		}
	}
	idThread::ReturnEntity( bestEnt );
}

/*
=====================
idAI::Event_GetRandomTarget
=====================
*/
void idAI::Event_GetRandomTarget( const char *type ) {
	int	i;
	int	num;
	int which;
	idEntity *ent;
	idEntity *ents[ MAX_GENTITIES ];

	num = 0;
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent && idStr::Cmp( ent->GetEntityDefName(), type ) == 0 ) {
			ents[ num++ ] = ent;
			if ( num >= MAX_GENTITIES ) {
				break;
			}
		}
	}

	if ( !num ) {
		idThread::ReturnEntity( NULL );
		return;
	}

	which = gameLocal.random.RandomInt( num );
	idThread::ReturnEntity( ents[ which ] );
}

/*
================
idAI::Event_TravelDistanceToPoint
================
*/
void idAI::Event_TravelDistanceToPoint( const idVec3 &pos ) {
	float time;

	time = TravelDistance( physicsObj.GetOrigin(), pos );
	idThread::ReturnFloat( time );
}

/*
================
idAI::Event_TravelDistanceToEntity
================
*/
void idAI::Event_TravelDistanceToEntity( idEntity *ent ) {
	float time;

	time = TravelDistance( physicsObj.GetOrigin(), ent->GetPhysics()->GetOrigin() );
	idThread::ReturnFloat( time );
}

/*
================
idAI::Event_TravelDistanceBetweenPoints
================
*/
void idAI::Event_TravelDistanceBetweenPoints( const idVec3 &source, const idVec3 &dest ) {
	float time;

	time = TravelDistance( source, dest );
	idThread::ReturnFloat( time );
}

/*
================
idAI::Event_TravelDistanceBetweenEntities
================
*/
void idAI::Event_TravelDistanceBetweenEntities( idEntity *source, idEntity *dest ) {
	float time;

	assert( source );
	assert( dest );
	time = TravelDistance( source->GetPhysics()->GetOrigin(), dest->GetPhysics()->GetOrigin() );
	idThread::ReturnFloat( time );
}

void idAI::Event_LookAtPoint(const idVec3 &_point, float duration)
{	
	focusEntity = NULL;
	alignHeadTime = gameLocal.time;
	forceAlignHeadTime = gameLocal.time + SEC2MS(1);
	blink_time = 0;

	this->currentFocusPos = _point;
	//gameRenderWorld->DebugArrow(colorGreen, GetEyePosition(), _point, 3, duration * 1000.0f);

	focusTime = gameLocal.time + SEC2MS(duration);
}

void idAI::LookAtPointMS(idVec3 _point, int durationMS)
{
	focusEntity = NULL;
	alignHeadTime = gameLocal.time;
	forceAlignHeadTime = gameLocal.time + 500;
	blink_time = 0;
	this->currentFocusPos = _point;

	focusTime = gameLocal.time + durationMS;
}

void idAI::Event_ResetLookPoint() //Reset head look; look straight ahead
{
	if (focusTime <= 0)
		return;

	focusEntity = NULL;
	focusTime = 0;
	this->currentFocusPos = GetEyePosition() + viewAxis[0] * 512.0f;
}


/*
=====================
idAI::Event_LookAtEntity
=====================
*/
void idAI::Event_LookAtEntity( idEntity *ent, float duration ) {
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
=====================
idAI::Event_LookAtEnemy
=====================
*/
void idAI::Event_LookAtEnemy( float duration ) {
	idActor *enemyEnt;

	enemyEnt = enemy.GetEntity();
	if ( ( enemyEnt != focusEntity.GetEntity() ) || ( focusTime < gameLocal.time ) ) {
		focusEntity	= enemyEnt;
		alignHeadTime = gameLocal.time;
		forceAlignHeadTime = gameLocal.time + SEC2MS( 1 );
		blink_time = 0;
	}

	focusTime = gameLocal.time + SEC2MS( duration );
}

/*
===============
idAI::Event_SetJointMod
===============
*/
void idAI::Event_SetJointMod( int allow ) {
	allowJointMod = ( allow != 0 );
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
		if ( ent->GetBindMaster() == this && ent->IsType( idMoveable::Type ) ) {
			moveable = ent;
			break;
		}
	}
	if ( moveable ) {
		moveable->Unbind();
		moveable->PostEventMS( &EV_SetOwner, 200, 0 );
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
		if ( ent->GetBindMaster() == this && ent->IsType( idAFEntity_Base::Type ) ) {
			af = ent;
			break;
		}
	}
	if ( af ) {
		af->Unbind();
		af->PostEventMS( &EV_SetOwner, 200, 0 );
	}
}

/*
================
idAI::Event_SetAngles
================
*/
void idAI::Event_SetAngles( idAngles const &ang ) {
	current_yaw = ang.yaw;
	viewAxis = idAngles( 0, current_yaw, 0 ).ToMat3();
}

/*
================
idAI::Event_GetAngles
================
*/
void idAI::Event_GetAngles( void ) {
	idThread::ReturnVector( idVec3( 0.0f, current_yaw, 0.0f ) );
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
idAI::Event_WakeOnFlashlight
================
*/
void idAI::Event_WakeOnFlashlight( int enable ) {
	wakeOnFlashlight = ( enable != 0 );
}

/*
================
idAI::Event_LocateEnemy
================
*/

//BC this seems to give enemy magic ESP powers about where enemy is.... probably don't use this call.
void idAI::Event_LocateEnemy( void )
{
	idActor *enemyEnt;
	int areaNum;

	enemyEnt = enemy.GetEntity();
	if ( !enemyEnt ) {
		return;
	}

	//gameRenderWorld->DebugArrow(colorOrange, lastVisibleReachableEnemyPos + idVec3(0, 0, 128), lastVisibleReachableEnemyPos, 8, 10000);
	//gameRenderWorld->DebugArrow(colorRed, lastReachableEnemyPos + idVec3(0, 0, 96), lastReachableEnemyPos, 8, 10000);
	//gameRenderWorld->DebugArrow(colorCyan, lastVisibleEnemyPos + idVec3(0, 0, 64), lastVisibleEnemyPos, 8, 10000);
	

	enemyEnt->GetAASLocation( aas, lastReachableEnemyPos, areaNum );
	SetEnemyPosition();
	UpdateEnemyPosition();
}

/*
================
idAI::Event_KickObstacles
================
*/
void idAI::Event_KickObstacles( idEntity *kickEnt, float force ) {
	idVec3 dir;
	idEntity *obEnt;

	if ( kickEnt ) {
		obEnt = kickEnt;
	} else {
		obEnt = move.obstacle.GetEntity();
	}

	if ( obEnt ) {
		dir = obEnt->GetPhysics()->GetOrigin() - physicsObj.GetOrigin();
		dir.Normalize();
	} else {
		dir = viewAxis[ 0 ];
	}
	KickObstacles( dir, force, obEnt );
}

/*
================
idAI::Event_GetObstacle
================
*/
void idAI::Event_GetObstacle( void ) {
	idThread::ReturnEntity( move.obstacle.GetEntity() );
}

/*
================
idAI::Event_PushPointIntoAAS
================
*/
void idAI::Event_PushPointIntoAAS( const idVec3 &pos ) {
	int		areaNum;
	idVec3	newPos;

	areaNum = PointReachableAreaNum( pos );
	if ( areaNum ) {
		newPos = pos;
		aas->PushPointIntoAreaNum( areaNum, newPos );
		idThread::ReturnVector( newPos );
	} else {
		idThread::ReturnVector( pos );
	}
}


/*
================
idAI::Event_GetTurnRate
================
*/
void idAI::Event_GetTurnRate( void ) {
	idThread::ReturnFloat( turnRate );
}

/*
================
idAI::Event_SetTurnRate
================
*/
void idAI::Event_SetTurnRate( float rate ) {
	turnRate = rate;
}

/*
================
idAI::Event_AnimTurn
================
*/
void idAI::Event_AnimTurn( float angles ) {
	turnVel = 0.0f;
	anim_turn_angles = angles;
	if ( angles ) {
		anim_turn_yaw = current_yaw;
		anim_turn_amount = idMath::Fabs( idMath::AngleNormalize180( current_yaw - ideal_yaw ) );
		if ( anim_turn_amount > anim_turn_angles ) {
			anim_turn_amount = anim_turn_angles;
		}
	} else {
		anim_turn_amount = 0.0f;
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 0, 1.0f );
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 1, 0.0f );
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 0, 1.0f );
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 1, 0.0f );
	}
}

/*
================
idAI::Event_AllowHiddenMovement
================
*/
void idAI::Event_AllowHiddenMovement( int enable ) {
	allowHiddenMovement = ( enable != 0 );
}

/*
================
idAI::Event_TriggerParticles
================
*/
void idAI::Event_TriggerParticles( const char *jointName ) {
	TriggerParticles( jointName );
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

	numListedEntities = gameLocal.clip.EntitiesTouchingBounds( idBounds( mins, maxs ), CONTENTS_BODY, entityList, MAX_GENTITIES );
	for( i = 0; i < numListedEntities; i++ ) {
		ent = entityList[ i ];
		if ( ent != this && !ent->IsHidden() && ( ent->health > 0 ) && ent->IsType( idActor::Type ) ) {
			idThread::ReturnEntity( ent );
			return;
		}
	}

	idThread::ReturnEntity( NULL );
}

/*
================
idAI::Event_CanReachPosition
================
*/
void idAI::Event_CanReachPosition( const idVec3 &pos )
{
	idThread::ReturnInt(CanReachPosition(pos));
}


//bc
bool idAI::CanReachPosition(const idVec3 &pos)
{
	aasPath_t	path;
	int			toAreaNum;
	int			areaNum;

	toAreaNum = PointReachableAreaNum(pos);	

	if (!toAreaNum)
		return false;
	
    areaNum = PointReachableAreaNum(physicsObj.GetOrigin());

	if (!PathToGoal(path, areaNum, physicsObj.GetOrigin(), toAreaNum, pos))
		return false;

	return true;
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
		if ( ent->IsType( idActor::Type ) && static_cast<idActor *>( ent )->OnLadder() ) {
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
	int			toAreaNum;
	int			areaNum;
	idVec3		pos;
	idActor		*enemyEnt;

	enemyEnt = enemy.GetEntity();
	if ( !enemyEnt ) {
		idThread::ReturnInt( false );
		return;
	}

	if ( move.moveType != MOVETYPE_FLY ) {
		if ( enemyEnt->OnLadder() ) {
			idThread::ReturnInt( false );
			return;
		}
		enemyEnt->GetAASLocation( aas, pos, toAreaNum );
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
			// NOTE: not a good way to return 'false'
			return idThread::ReturnVector( vec3_zero );
		}
		if ( ent->IsType( idActor::Type ) && static_cast<idActor *>( ent )->OnLadder() ) {
			// NOTE: not a good way to return 'false'
			return idThread::ReturnVector( vec3_zero );
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

#ifdef _D3XP
/*
================
idAI::Event_MoveToPositionDirect
================
*/
void idAI::Event_MoveToPositionDirect( const idVec3 &pos ) {
	StopMove( MOVE_STATUS_DONE );
	DirectMoveToPosition( pos );
}

/*
================
idAI::Event_AvoidObstacles
================
*/
void idAI::Event_AvoidObstacles( int ignore) {
	ignore_obstacles = (ignore == 1) ? false : true;
}

/*
================
idAI::Event_TriggerFX
================
*/
void idAI::Event_TriggerFX( const char* joint, const char* fx ) {
	TriggerFX(joint, fx);
}

void idAI::Event_StartEmitter( const char* name, const char* joint, const char* particle ) {
	idEntity *ent = StartEmitter(name, joint, particle);
	idThread::ReturnEntity(ent);
}

void idAI::Event_GetEmitter( const char* name ) {
	idThread::ReturnEntity(GetEmitter(name));
}

void idAI::Event_StopEmitter( const char* name ) {
	StopEmitter(name);
}


// ---------------- S K I N    D E E P ----------------


//BC return vector3 of last enemy position.
void idAI::Event_GetLastEnemyPosition(void)
{
	//gameRenderWorld->DebugArrow(colorOrange, lastVisibleEnemyPos + idVec3(0, 0, 64), lastVisibleEnemyPos, 4, 3000);
	//gameRenderWorld->DrawText("LAST ENEMY POS", lastVisibleEnemyPos + idVec3(0, 0, 64), .5f, colorOrange, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 3000);

	//Do a check to see if it is floating in the sky. If it is floating, then drop it to ground.
	//trace_t tr;
	//gameLocal.clip.TracePoint(tr, lastVisibleEnemyPos + idVec3(0,0,1), lastVisibleEnemyPos + idVec3(0, 0, -1024), MASK_SOLID, this);
	//idThread::ReturnVector(tr.endpos);

	idThread::ReturnVector(lastVisibleEnemyPos);
}

//Find a suitable cover node.
void idAI::Event_GetCoverNode(void)
{
	const int MAXDIST = 512;

	int				nodeList, i;
	idActor			*enemyEnt;
	float			bestScore;
	idEntity		*bestNode;
	idEntity		*entityList[MAX_GENTITIES];
	idVec3			dirToEnemy;

	enemyEnt = enemy.GetEntity();

	if (!enemyEnt)
	{
		//Player has no enemy.
		idThread::ReturnEntity(NULL);
		return;
	}

	dirToEnemy = enemyEnt->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin();
	dirToEnemy.Normalize();
	bestScore = -1000000;
	bestNode = NULL;
	nodeList = gameLocal.EntitiesWithinRadius(GetPhysics()->GetOrigin(), MAXDIST, entityList, MAX_GENTITIES);

	//gameRenderWorld->DebugClearLines(1);

	for (i = 0; i < nodeList; i++)
	{
		trace_t			trace;
		float			candidateScore;
		idEntity		*ent = entityList[i];
		float			movedirDot, facingDot;
		idVec3			moveDirection;

		if (!ent)
			continue;

		if (idStr::Icmp("ai_covernode", ent->spawnArgs.GetString("classname")) != 0)
			continue;

		//gameRenderWorld->DebugCircle(colorCyan, GetPhysics()->GetOrigin(), idVec3(0, 0, 1), MAXDIST, 32, 5000);

		//Only choose nodes that are within player's PVS.
		/*
		if (!gameLocal.InPlayerPVS(entity))
		{
		entity = gameLocal.FindEntityUsingDef(entity, "ai_covernode");
		continue;
		}*/

		gameLocal.clip.TracePoint(trace, ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 32), enemyEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, 32), MASK_SOLID, this);
		if (trace.fraction >= 1.0f)
		{
			//Has a clear LOS to enemy. Therefore, invalid.
			continue;
		}

		candidateScore = 0;
		
		//Okay, we now have a valid candidate node. Generate a score for this node.

		
		//-- DISTANCE CHECK. We value closer nodes. As distance increases, the node's score decreases.
		candidateScore -= (this->GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin()).LengthSqr() * .001f;

		//-- AVOID MOVING TOWARD PLAYER. We value going lateral or away from enemy. Harsh penalty for going toward enemy.
		moveDirection = ent->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
		moveDirection.Normalize();
		movedirDot = DotProduct(moveDirection, dirToEnemy);

		if (movedirDot > 0)
		{
			//Moving toward player. We don't want this, so discourage this. Subtract score.
			candidateScore -= 256; //base penalty.
			candidateScore -= movedirDot * 64;
		}
		else
		{
			//Heading away from player. We want to encourage this. We kinda don't care whether node is behind or lateral to us.
			candidateScore += 128;
			//candidateScore += idMath::Fabs(movedirDot) * 4; 
		}		
		
		//-- NODE FACING. Favor spots that are pointing toward the enemy.
		facingDot = DotProduct(ent->GetPhysics()->GetAxis().ToAngles().ToForward(), dirToEnemy);
		candidateScore += facingDot * 256;	

		


		if (developer.GetInteger() >= 2)
		{
			gameRenderWorld->DrawText(va("%.1f", candidateScore), ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 16), .3f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 0, 4000);
			//gameRenderWorld->DrawText(va("%.2f  ", facingDot), ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 8), .2f, colorOrange, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 0, 4000);
		}

		if (candidateScore > bestScore)
		{
			bestNode = ent;
			bestScore = candidateScore;
		}

	}

	if (bestNode && developer.GetInteger() >= 2)
	{
		gameRenderWorld->DebugLine(colorCyan, this->GetPhysics()->GetOrigin(), bestNode->GetPhysics()->GetOrigin() + idVec3(0,0,64), 4000);
		gameRenderWorld->DebugArrow(colorCyan, bestNode->GetPhysics()->GetOrigin() + idVec3(0, 0, 64), bestNode->GetPhysics()->GetOrigin(), 8, 4000);
	}

	idThread::ReturnEntity(bestNode);
}



//Returns position of nearby suspicious noise.
void idAI::Event_HeardSuspiciousNoise()
{
	float dist;

	if (this->IsHidden() || this->health <= 0)
	{
		idThread::ReturnVector(vec3_zero);
		return;
	}

	if (!gameLocal.InPlayerPVS(this))
	{
		idThread::ReturnVector(vec3_zero);
		return;
	}

	dist = (gameLocal.suspiciousNoisePos - physicsObj.GetOrigin()).LengthSqr();

	//common->Printf("%f    %d\n", dist, gameLocal.suspiciousNoiseRadius);

	if (dist < Square( gameLocal.suspiciousNoiseRadius))
	{
		idThread::ReturnVector(gameLocal.suspiciousNoisePos);
		return;
	}

	idThread::ReturnVector(vec3_zero);
}

void idAI::Event_HeardSuspiciousPriority()
{
	idThread::ReturnInt(gameLocal.lastSuspiciousNoisePriority);
}

//BC Find a suitable object for me to pick up and throw at enemy.
void idAI::Event_GetThrowableObject(const idVec3 &mins, const idVec3 &maxs, float speed, float minDist, float offset)
{
	idEntity *	ent;
	idEntity *	entityList[MAX_GENTITIES];
	int			numListedEntities;
	int			i, index;
	float		dist;
	idVec3		vel;
	idVec3		offsetVec(0, 0, offset);
	idEntity	*enemyEnt = enemy.GetEntity();

	if (!enemyEnt)
	{
		idThread::ReturnEntity(NULL);
	}

	idVec3 enemyEyePos = lastVisibleEnemyPos + lastVisibleEnemyEyeOffset;
	//const idBounds &myBounds = physicsObj.GetAbsBounds();
	idBounds checkBounds(mins, maxs);
	checkBounds.TranslateSelf(physicsObj.GetOrigin());
	numListedEntities = gameLocal.clip.EntitiesTouchingBounds(checkBounds, -1, entityList, MAX_GENTITIES);

	//gameRenderWorld->DebugArrow(colorOrange, physicsObj.GetOrigin(), physicsObj.GetOrigin() + idVec3(0, 0, 64), 16, 1000);
	//gameRenderWorld->DebugBounds(colorOrange, checkBounds, vec3_zero, 1000);

	

	for (i = 0; i < numListedEntities; i++)
	{
		//Pick a random object to throw.
		index = gameLocal.random.RandomInt(numListedEntities);

		if (index >= numListedEntities)
		{
			index = 0;
		}

		ent = entityList[index];
		
		if (!ent->IsType(idMoveableItem::Type))
		{
			continue;
		}

		if (ent->fl.hidden)
		{
			// don't throw hidden objects
			continue;
		}

		idPhysics *entPhys = ent->GetPhysics();
		const idVec3 &entOrg = entPhys->GetOrigin();
		dist = (entOrg - enemyEyePos).LengthFast();

//		gameRenderWorld->DebugLine(colorRed, entOrg, enemyEyePos, 2000);

		if (dist < minDist)
		{
			continue;
		}

		/*
		idBounds expandedBounds = myBounds.Expand(entPhys->GetBounds().GetRadius());
		if (expandedBounds.LineIntersection(entOrg, enemyEyePos)) {
			// ignore objects that are behind us
			continue;
		}*/

		if (PredictTrajectory(entPhys->GetOrigin() + offsetVec, enemyEyePos, speed, entPhys->GetGravity(),
			entPhys->GetClipModel(), entPhys->GetClipMask(), MAX_WORLD_SIZE, NULL, enemyEnt, ai_debugTrajectory.GetBool() ? 4000 : 0, vel))
		{
			//gameRenderWorld->DebugBounds(colorOrange, ent->GetPhysics()->GetBounds(), ent->GetPhysics()->GetOrigin(), 2000);
			idThread::ReturnEntity(ent);
			return;
		}
	}

	idThread::ReturnEntity(NULL);
}

//Throw the object at enemy.
void idAI::Event_ThrowObjectAtEnemy(idEntity *ent, float speed)
{
	idVec3		vel;
	idEntity	*enemyEnt;
	idPhysics	*entPhys;

	entPhys = ent->GetPhysics();
	enemyEnt = enemy.GetEntity();
	if (!enemyEnt)
	{
		vel = (viewAxis[0] * physicsObj.GetGravityAxis()) * speed;
	}
	else
	{
		PredictTrajectory(entPhys->GetOrigin(), lastVisibleEnemyPos + lastVisibleEnemyEyeOffset, speed, entPhys->GetGravity(),
			entPhys->GetClipModel(), entPhys->GetClipMask(), MAX_WORLD_SIZE, NULL, enemyEnt, ai_debugTrajectory.GetBool() ? 400 : 0, vel);

		vel *= speed;
	}

	entPhys->SetLinearVelocity(vel);

	if (ent->IsType(idMoveable::Type))
	{
		idMoveable *ment = static_cast<idMoveable*>(ent);
		ment->EnableDamage(true, 2.5f);
	}
}

bool idAI::Event_ThrowObjectAtPosition(idEntity *ent, const idVec3 &targetPosition)
{
	float		speed;
	idVec3		vel;	
	idPhysics	*entPhys;
	bool		throwSuccess = false;
	idVec3		throwOrigin;


	throwOrigin = this->GetPhysics()->GetOrigin() + idVec3(0, 0, 100);
	entPhys = ent->GetPhysics();

	//gameRenderWorld->DebugArrow(colorGreen, this->GetPhysics()->GetOrigin() + idVec3(0, 0, 200), this->GetPhysics()->GetOrigin() + idVec3(0, 0, 80), 4, 5000);

	for (speed = 800; speed < 1200; speed += 100)
	{
		//TODO: Spawn in hand, not origin.
		if (PredictTrajectory(throwOrigin, targetPosition, speed, entPhys->GetGravity(), entPhys->GetClipModel(), entPhys->GetClipMask(),
			MAX_WORLD_SIZE, this, static_cast<idMeta*>( gameLocal.metaEnt.GetEntity())->lkpEnt, ai_debugTrajectory.GetBool() ? 3000 : 0, vel, false))
		{
			throwSuccess = true;
			vel = vel * (speed * idMath::Lerp(1.20f, 1.25f, gameLocal.random.RandomFloat())); //TODO: find a better fix than this HACK HACK HACK
			break;
		}
	}

	if (throwSuccess)
	{
		//Success.
		entPhys->SetOrigin(throwOrigin);
		entPhys->SetLinearVelocity(vel);

		if (ent->IsType(idMoveable::Type))
		{
			idMoveable *ment = static_cast<idMoveable*>(ent);
			ment->EnableDamage(true, 2.5f);
		}

		idThread::ReturnInt(true);
		return true;
	}
	
	//Failed to throw.
	idThread::ReturnInt(false);	
	return false;
}

const int SEARCHNODE_EXPIRATIONTIME = 5000; //searchnode is valid only if the last time it was used was more than XX milliseconds ago.
const int SEARCHNODE_MAXRADIUS = 1024;

void idAI::Event_GetSearchNode()
{
	idThread::ReturnEntity(GetSearchNode());
}

//This is called when:
// - the ai enters the alerted (combat) state, but only for ai that aren't directly investigating the disturbance.
idEntity *idAI::GetSearchNodeSpreadOut()
{
	//First, we get information on what rooms are (or will be) occupied, and get their room location entity nums.
	idList<int> locationEntNumsToIgnore;
	locationEntNumsToIgnore.Clear();
	idEntity *actor;
	for (actor = gameLocal.aimAssistEntities.Next(); actor != NULL; actor = actor->aimAssistNode.Next())
	{
		if (!actor || actor->IsHidden() || actor->fl.isDormant || actor->team != this->team || !actor->IsType(idAI::Type) || actor == this)
			continue;

		idVec3 actorFinalPosition;
		if (static_cast<idAI *>(actor)->move.moveStatus == MOVE_STATUS_MOVING)
		{
			//Actor is on the move somewhere.
			actorFinalPosition = static_cast<idAI *>(actor)->move.moveDest; //Where they WANT to go.
		}
		else
		{
			//Is standing still, is not moving.
			actorFinalPosition = actor->GetPhysics()->GetOrigin();
		}

		actorFinalPosition += idVec3(0, 0, 1); //bump it up a little to avoid the actor point possibly digging into the ground.

		//Cull out any searchnodes that are in the same room as this actorfinalposition.
		idLocationEntity *locEnt = gameLocal.LocationForPoint(actorFinalPosition);
		if (!locEnt)
			continue; //Location doesn't exist?? Skip....

		if (locationEntNumsToIgnore.Num() > 0)
		{
			if (locationEntNumsToIgnore.Find(locEnt->entityNumber) != NULL)
				continue; //Already in list. Skip.....
		}

		locationEntNumsToIgnore.Append(locEnt->entityNumber);
	}



	//If player hasn't exited pod yet, avoid patrolling into the room that cryo exit is in.
	if (!static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetPlayerExitedCryopod() || static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetCryoExitTime() > gameLocal.time)
	{
		int cryopodLocationEntnum = static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetCryoexitLocationEntnum();
		if (cryopodLocationEntnum >= 0)
		{
			locationEntNumsToIgnore.Append(cryopodLocationEntnum);
		}
	}



	if (locationEntNumsToIgnore.Num() <= 0)
		return GetSearchNode(); //uh oh. got no info. Just do a default getsearchnode...

	if (1)
	{
		//If the room I am currently in is JUST ME and no one else, then just stay in here...

		idLocationEntity *locEnt = gameLocal.LocationForPoint(GetPhysics()->GetOrigin());
		if (locEnt)
		{
			if (locationEntNumsToIgnore.Find(locEnt->entityNumber))
			{
				//This room IS or WILL BE occupied by someone else.... 
			}
			else if (gameLocal.random.RandomInt(100) < 80)
			{
				//This room is JUST ME and no one else.
				return GetSearchNodeInLocEntNum(locEnt->entityNumber);
			}
		}		
	}


	//Then, we create a curated list of searchnodes.
	idList<int>		candidateIndexes;
	candidateIndexes.Clear();
	for (idEntity* node = gameLocal.searchnodeEntities.Next(); node != NULL; node = node->aiSearchNodes.Next())
	{
		if (!node || node->IsHidden() || gameLocal.time <= ((idPathCorner *)node)->lastTimeUsed + SEARCHNODE_EXPIRATIONTIME)
			continue;

		idLocationEntity *locEnt = gameLocal.LocationForEntity(node);
		if (!locEnt)
			continue; //Location doesn't exist?? Skip....

		if (locationEntNumsToIgnore.Find(locEnt->entityNumber) != NULL)
			continue; //Ignore this location!!!! It's been claimed by someone! Skip it!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		candidateIndexes.Append(node->entityNumber);
	}

	if (candidateIndexes.Num() <= 0)
		return GetSearchNode(); //No valid candidates found. Just do default GetSearchNode.......

	//Ok, we now have a list of curated search nodes. Return a random one.
	int randomIndex = gameLocal.random.RandomInt(candidateIndexes.Num());
	int entityNum = candidateIndexes[randomIndex];
	static_cast<idPathCorner *>(gameLocal.entities[entityNum])->lastTimeUsed = gameLocal.time;
	return gameLocal.entities[entityNum];
}

idEntity *idAI::GetSearchNodeInLocEntNum(int locationEntnum)
{
	idList<int>	candidateIndexes;
	candidateIndexes.Clear();
	for (idEntity* node = gameLocal.searchnodeEntities.Next(); node != NULL; node = node->aiSearchNodes.Next())
	{
		if (!node || node->IsHidden() || gameLocal.time <= ((idPathCorner *)node)->lastTimeUsed + SEARCHNODE_EXPIRATIONTIME)
			continue;

		idLocationEntity *locEnt = gameLocal.LocationForEntity(node);
		if (!locEnt)
			continue; //Location doesn't exist?? Skip....

		if (locEnt->entityNumber != locationEntnum)
			continue; //Ignore this location!!!! It's not in the room we're looking for.

		candidateIndexes.Append(node->entityNumber);
	}

	if (candidateIndexes.Num() <= 0)
		return GetSearchNode(); //No valid candidates found. Just do default GetSearchNode.......

	//Ok, we now have a list of curated search nodes. Return a random one.
	int randomIndex = gameLocal.random.RandomInt(candidateIndexes.Num());
	int entityNum = candidateIndexes[randomIndex];
	static_cast<idPathCorner *>(gameLocal.entities[entityNum])->lastTimeUsed = gameLocal.time;
	return gameLocal.entities[entityNum];
}

idEntity *idAI::GetSearchNode()
{
	idList<int>		candidateIndexes;
	int				randomIndex;

	pvsHandle_t		pvs;
	int				localPvsArea;

	localPvsArea = gameLocal.pvs.GetPVSArea(GetPhysics()->GetOrigin() + idVec3(0, 0, 1));
	pvs = gameLocal.pvs.SetupCurrentPVS(localPvsArea);
    
    for (idEntity* ent = gameLocal.searchnodeEntities.Next(); ent != NULL; ent = ent->aiSearchNodes.Next())
	{
		if (!ent)
			continue;

		if (ent->IsHidden() || gameLocal.time <= ((idPathCorner *)ent)->lastTimeUsed + SEARCHNODE_EXPIRATIONTIME)
			continue;

		//Check if the node is within PVS. TODO: Check if this Pvs check actually is working right.....
		if (!gameLocal.pvs.InCurrentPVS(pvs, ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 1)))
			continue;

        //Do distance check to the node.
        #define MAXNODEDISTANCE 1024
        float distToNode = (this->GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin()).LengthFast();
        if (distToNode > MAXNODEDISTANCE)
            continue;

		candidateIndexes.Append(ent->entityNumber);
	}

	gameLocal.pvs.FreeCurrentPVS(pvs);

	if (candidateIndexes.Num() <= 0)
	{
		//Found nothing valid. SO, just pick a random searchnode ANYWHERE in the map.
		for (idEntity* ent = gameLocal.searchnodeEntities.Next(); ent != NULL; ent = ent->aiSearchNodes.Next())
		{
			if (!ent)
				continue;

			if (ent->IsHidden() || gameLocal.time <= ((idPathCorner *)ent)->lastTimeUsed + SEARCHNODE_EXPIRATIONTIME)
				continue;

			candidateIndexes.Append(ent->entityNumber);
		}
	}

	if (candidateIndexes.Num() <= 0)
	{
		//Found nothing valid...
		return NULL;
	}

	//We have candidates. Select a random node amongst the candidates. Return it to script.
	randomIndex = gameLocal.random.RandomInt(candidateIndexes.Num());
    int entityNum = candidateIndexes[randomIndex];
    static_cast<idPathCorner *>(gameLocal.entities[entityNum])->lastTimeUsed = gameLocal.time;

	//gameRenderWorld->DebugArrowSimple(entityList[candidateIndexes[randomIndex]]->GetPhysics()->GetOrigin());

	return gameLocal.entities[entityNum];
}



//Laser will END at this point.
void idAI::Event_SetLaserEndLock(const idVec3 &laserPos)
{
	laserEndLockPosition = laserPos;
}

//Laser will pass through this point.
void idAI::Event_SetLaserLock(const idVec3 &laserPos)
{
	laserLockPosition = laserPos;
}

void idAI::Event_GetLaserLock()
{
	idThread::ReturnVector(laserLockPosition);
}

void idAI::Event_GetLaserHitPos()
{
	idThread::ReturnVector(laserdot->GetPhysics()->GetOrigin());
}

void idAI::Event_SetLaserSkin(const char* laserSkinName)
{
	lasersightbeam->SetSkin(declManager->FindSkin(laserSkinName));
	//lasersightbeam->UpdateVisuals();
}

void idAI::Event_GetEnemyCenter(void)
{
	if (enemy.GetEntity())
	{
		idThread::ReturnVector(enemy.GetEntity()->GetPhysics()->GetAbsBounds().GetCenter());
		return;
	}

	//If fail, fall back to this...
	idThread::ReturnVector(lastVisibleEnemyPos + lastVisibleEnemyEyeOffset);
}

void idAI::Event_SetFlyBobStrength(int value)
{
	fly_bob_strength = value;
}

void idAI::Event_GetFlyBobStrength()
{
	idThread::ReturnFloat(fly_bob_strength);
}

//This is what's called in the monster thug script.
idEntity *idAI::FindEnemyAIVisible()
{
	return Event_FindEnemyAI(1);	

	/*
	int verticalHeightDelta;
	float vdot;
	idVec3 dirToEnemy;
	idActor *enemyEnt = Event_FindEnemyAI(false); //This grabs closest enemy within my PVS.

	if (enemyEnt == NULL)
	{
		//Invalid ent.... exit.
		//common->Printf("Event_FindEnemyAIvisible(): did not find enemy.\n");
		return NULL;
	}	

	//Check if it's in the visionbox.
	if (CheckFOV(enemyEnt->GetPhysics()->GetOrigin())   || CheckFOV(enemyEnt->GetEyePosition())  ) //BC consolidate vision code to CheckFOV() function.
	{
		//common->Printf("Event_FindEnemyAIvisible(): Enemy is in FOV.\n");
		return enemyEnt;
	}

	return NULL;*/
}

void idAI::Event_FindEnemyAIvisible()
{
	idThread::ReturnEntity(FindEnemyAIVisible());
}

//This is what gets called when AI fires weapon. It launches projectile toward direction of laser pointer.
void idAI::Event_LaunchMissileAtLaser(const char *jointname)
{
	//do not allow shooting laser at 0,0,0. because that's a special location that is considered 'null'
	if (laserLockPosition == vec3_zero)
		return;

	LaunchProjectileAtPos(jointname, laserLockPosition);
}

void idAI::Event_GetFlySpeed()
{
	idThread::ReturnFloat(fly_speed);
}

void idAI::Event_DoDamage(const char *damageDefName)
{
	gameLocal.RadiusDamage(GetPhysics()->GetOrigin(), this, this, this, this, damageDefName);
}

bool idAI::CanHitFromAnim(const char *animname, idVec3 targetPos)
{
	int		anim;
	idVec3	dir;
	idVec3	local_dir;
	idVec3	fromPos;
	idMat3	axis;
	idVec3	start;
	trace_t	tr;

	//idActor *enemyEnt = enemy.GetEntity();
	//if (!AI_ENEMY_VISIBLE || !enemyEnt) {
	//	idThread::ReturnInt(false);
	//	return;
	//}

	anim = GetAnim(ANIMCHANNEL_LEGS, animname);
	if (!anim) {
		return false;
	}

	// just do a ray test if close enough
	//if (enemyEnt->GetPhysics()->GetAbsBounds().IntersectsBounds(physicsObj.GetAbsBounds().Expand(16.0f))) {
	//	Event_CanHitEnemy();
	//	return;
	//}

	// calculate the world transform of the launch position
	const idVec3 &org = physicsObj.GetOrigin();
	dir = targetPos - org;
	physicsObj.GetGravityAxis().ProjectVector(dir, local_dir);
	local_dir.z = 0.0f;
	local_dir.ToVec2().Normalize();
	axis = local_dir.ToMat3();
	fromPos = physicsObj.GetOrigin() + missileLaunchOffset[anim] * axis;

	if (projectileClipModel == NULL) {
		CreateProjectileClipModel();
	}

	gameLocal.clip.Translation(tr, fromPos, targetPos, projectileClipModel, mat3_identity, MASK_SHOT_RENDERMODEL, this);

	//gameRenderWorld->DebugArrow(colorGreen, fromPos, tr.endpos, 4, 1000);	
	//common->Printf("tr %f\n", tr.fraction);

	if (ai_debugPerception.GetInteger() > 0)
	{
		gameRenderWorld->DebugArrow(tr.fraction >= .98f ? colorGreen : colorRed, fromPos, tr.endpos, 4, 1000);
		gameRenderWorld->DrawText(animname, fromPos, 0.07f, tr.fraction >= .98f ? colorGreen : colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 1000);
	}

	//gameRenderWorld->DebugArrow(colorRed, fromPos, targetPos + idVec3(0,0,-4), 1, 1000);


	if (tr.fraction >= 0.98f)
	{
		return true;
	}
	
	return false;		
}

void idAI::Event_CanHitFromAnim(const char *animname, const idVec3 &targetPos)
{
	idThread::ReturnInt(CanHitFromAnim(animname, targetPos));
}




void idAI::Event_SetLaserActive(int active)
{
	if (lasersightbeam == NULL || lasersightbeamTarget == NULL)
		return;

	if (active)
	{
		if (health > 0)
		{
			laserdot->Show();
			lasersightbeam->Show();
		}
	}
	else
	{
		laserdot->Hide();
		lasersightbeam->Hide();
	}
}


void idAI::Event_CheckForwardDot(const idVec3 &lookPoint)
{
	float vdot;
	idVec3 dirToEnemy;
	idAngles angToEnemy;

	dirToEnemy = lookPoint - GetPhysics()->GetOrigin();
	angToEnemy = dirToEnemy.ToAngles();
	angToEnemy.pitch = 0;
	angToEnemy.roll = 0;

	vdot = DotProduct(idAngles(0, current_yaw, 0).ToForward(), angToEnemy.ToForward());

	idThread::ReturnFloat(vdot);
}

void idAI::Event_CheckSearchLook(const idVec3 &lookPoint, int useFacing)
{
	idThread::ReturnInt(CheckSearchLook(lookPoint, useFacing, true) ? 1 : 0);
}


//During search mode, do a check to see if we should look at LKP
//  lookPoint = where to look at.
//  useFacing = only look at it if my body is mostly facing toward the point.
//  doProximityCheck = ignore things that are very close to me.
bool idAI::CheckSearchLook(idVec3 lookPoint, int useFacing, bool doProximityCheck)
{
	float vdot;
	idVec3 dirToEnemy;
	idAngles angToEnemy;
	float lengthToPoint;

	//do dotproduct check to see if ai is facing toward the look point. We don't want an awkward situation where ai tries to look at something behind themselves.	

	if (useFacing)
	{
		dirToEnemy = lookPoint - GetPhysics()->GetOrigin();
		angToEnemy = dirToEnemy.ToAngles();
		angToEnemy.pitch = 0;
		angToEnemy.roll = 0;
		//vdot = DotProduct(idAngles(0, visionBox->GetPhysics()->GetAxis().ToAngles().yaw, 0).ToForward(), angToEnemy.ToForward()); //TODO: should we be checking visionbox angle??? should instead be monster forward angle.
		vdot = DotProduct(idAngles(0, current_yaw, 0).ToForward(), angToEnemy.ToForward());
	}
	else
	{
		vdot = AI_CHECKSEARCH_DOT + 1;
	}
	
	//Prevent looking at a point that's basically at the ai's feet.
	if (doProximityCheck)
	{
		lengthToPoint = (lookPoint - GetPhysics()->GetOrigin()).LengthFast();
		if (lengthToPoint <= 96 && idMath::Fabs(lookPoint.z - GetPhysics()->GetOrigin().z) <= 8)
		{
			//gameRenderWorld->DebugArrow(colorOrange, GetEyePosition(), lookPoint, 4, 150);
			return false;
		}

		//if lookpoint is right next to eyeball, ignore it.
		lengthToPoint = (lookPoint - GetEyePosition()).LengthFast();
		if (lengthToPoint <= 48)
		{
			//gameRenderWorld->DebugArrow(colorOrange, GetEyePosition(), lookPoint, 4, 150);
			return false;
		}
	}


	//vdot: 1.0 = directly in front of ai. 0.0 = directly to the side of ai.
	if (vdot >= AI_CHECKSEARCH_DOT)
	{
		//Ok, LKP is mostly in front of ai. Now see if there are other AI folks who are in COMBAT.

		//Now do check to see if we have a clear-ish LOS to the LKP.
		trace_t losTr;

		gameLocal.clip.TracePoint(losTr, GetEyePosition(), lookPoint, CONTENTS_SOLID, this);

		//If traceline can't hit near the lookpoint, then exit.
		if ((losTr.endpos - lookPoint).LengthFast() > 64)
		{
			return false;
		}

		//gameRenderWorld->DebugArrow(colorGreen, GetEyePosition(), lookPoint, 4, 150);
		return true;
	}	

	//gameRenderWorld->DebugArrow(colorRed, GetEyePosition(), lookPoint, 4, 150);
	return false;
}



void idAI::Event_SetLastVisiblePos(const idVec3 &pos)
{
	lastVisibleEnemyPos = pos;
}

//Darkmod.
void idAI::Event_GetObservationPosition(const idVec3 &pointToObserve)
{
	idVec3 observeFromPos = GetObservationPosition(pointToObserve, 1.0f, 0); // grayman #4347
	idThread::ReturnVector(observeFromPos);
	return;
}


void idAI::Event_GetObservationViaNodes(const idVec3 &pointToObserve)
{
	idThread::ReturnVector(GetObservationViaNodes(pointToObserve));
}

idVec3 idAI::GetObservationViaNodes(idVec3 pointToObserve)
{
	//Regenerate the LOS data if it needs to be regenerated.
	static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GenerateNodeLOS(pointToObserve);

	//Choose a random valid point.
	if (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->nodePosArraySize > 0)
	{
		idVec3 returnValue;
		int arraySize ;

		arraySize = static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->nodePosArraySize;
		returnValue = static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->nodePositions[gameLocal.random.RandomInt(arraySize)];
		
		return returnValue;
	}

	//If none found, then exit.
	return vec3_zero;
}

//Alert the world that we should enter global combat state.
void idAI::SetCombatState(int value, bool restartCombatTimer)
{
	combatState = value;

	//static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetCombatState() != COMBATSTATE_COMBAT
	if (value > 0)
	{
		//Sometimes we don't want to restart the combat timer.
		if (!restartCombatTimer && static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetCombatState() == COMBATSTATE_COMBAT)
			return;			

		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->AlertAIFriends(this);
	}	
}

void idAI::Event_GetAIState()
{
	idThread::ReturnInt(aiState);
}

void idAI::Event_StartStunState(const char* damageDefName)
{
	StartStunState(damageDefName);
}

void idAI::Event_IsBleedingOut(void)
{
	idThread::ReturnFloat(GetBleedingOut());
}

void idAI::Event_GetSkullsaver(void)
{
	idThread::ReturnEntity(skullEnt);
}

void idAI::Event_SetPathEntity(idEntity* pathEnt)
{
}



void idAI::Event_GetLifeState()
{
	if (bleedoutState == BLEEDOUT_ACTIVE)
	{
		idThread::ReturnInt(AILS_BLEEDINGOUT);
		return;
	}

	if (skullEnt != nullptr)
	{
		idThread::ReturnInt(AILS_INSKULLSAVER);
		return;
	}

	if (health > 0 && !IsHidden())
	{
		idThread::ReturnInt(AILS_ALIVE);
		return;
	}

	//Technically this doesn't really get called since the actor handle gets wiped when we delete the actor
	//So: make sure AILS_DEFEATED is enum index zero
	idThread::ReturnInt(AILS_DEFEATED);
}

#endif
