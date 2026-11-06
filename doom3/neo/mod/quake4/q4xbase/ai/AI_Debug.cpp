/*
================

AI_Debug.cpp

================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "AI_Manager.h"
#include "AI_Util.h"

const char *aiMoveCommandString[ NUM_MOVE_COMMANDS ] = {
	"MOVE_NONE",
	"MOVE_FACE_ENEMY",
	"MOVE_FACE_ENTITY",
	"MOVE_TO_ENEMY",
	"MOVE_TO_ENTITY",
	"MOVE_TO_ATTACK",
	"MOVE_TO_HELPER",
	"MOVE_TO_TETHER",
	"MOVE_TO_COVER",
	"MOVE_TO_HIDE",
	"MOVE_TO_POSITION",
	"MOVE_OUT_OF_RANGE",
	"MOVE_SLIDE_TO_POSITION",
	"MOVE_WANDER"
};

const char* aiMoveStatusString[ NUM_MOVE_STATUS ] = {
	"MOVE_STATUS_DONE",
	"MOVE_STATUS_MOVING",
	"MOVE_STATUS_WAITING",
	"MOVE_STATUS_DEST_NOT_FOUND",
	"MOVE_STATUS_DEST_UNREACHABLE",
	"MOVE_STATUS_BLOCKED_BY_WALL",
	"MOVE_STATUS_BLOCKED_BY_OBJECT",
	"MOVE_STATUS_BLOCKED_BY_ENEMY",
	"MOVE_STATUS_BLOCKED_BY_MONSTER",
	"MOVE_STATUS_BLOCKED_BY_PLAYER",
	"MOVE_STATUS_DISABLED"
};

const char* aiMoveDirectionString [ MOVEDIR_MAX ] = {
	"MOVEDIR_FORWARD",
	"MOVEDIR_BACKWARD",
	"MOVEDIR_LEFT",
	"MOVEDIR_RIGHT"
};

const char* aiTacticalString [ AITACTICAL_MAX ] = {
	"AITACTICAL_NONE",
	"AITACTICAL_MELEE",
	"AITACTICAL_MOVE_FOLLOW",
	"AITACTICAL_MOVE_TETHER",
	"AITACTICAL_MOVE_PLAYERPUSH",
	"AITACTICAL_COVER",
 	"AITACTICAL_COVER_FLANK",
 	"AITACTICAL_COVER_ADVANCE",
 	"AITACTICAL_COVER_RETREAT",
 	"AITACTICAL_COVER_AMBUSH",
	"AITACTICAL_RANGED",
	"AITACTICAL_TURRET",
	"AITACTICAL_HIDE",
	"AITACTICAL_PASSIVE",
};

const char* aiFocusString [ AIFOCUS_MAX ] = {
	"AIFOCUS_NONE",
	"AIFOCUS_LEADER",
	"AIFOCUS_TARGET",
	"AIFOCUS_TALK",
	"AIFOCUS_PLAYER",
	"AIFOCUS_ENEMY",
	"AIFOCUS_COVER",
	"AIFOCUS_COVERLOOK",
	"AIFOCUS_HELPER",
	"AIFOCUS_TETHER",
};

const char* aiActionStatusString [ rvAIAction::STATUS_MAX ] = {
	"Unused",
	"OK",
	"Disabled",
	"Failed: timer",
	"Failed: external timer",
	"Failed: within min range",
	"Failed: out of max range",
	"Failed: random chance",
	"Failed: bad animation",
	"Failed: condition",	
	"Failed: no enemy",
};

/*
=====================
idAI::GetDebugInfo
=====================
*/
void idAI::GetDebugInfo ( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idActor::GetDebugInfo ( proc, userData );
	
	proc ( "idAI", "aifl.damage",			aifl.damage ? "true" : "false", userData );
	proc ( "idAI", "aifl.undying",			aifl.undying ? "true" : "false", userData );
	proc ( "idAI", "aifl.pain",				aifl.pain ? "true" : "false", userData );
	proc ( "idAI", "aifl.dead",				aifl.dead ? "true" : "false", userData );
	proc ( "idAI", "aifl.activated",		aifl.activated ? "true" : "false", userData );
	proc ( "idAI", "aifl.jump",				aifl.jump ? "true" : "false", userData );
	proc ( "idAI", "aifl.hitEnemy",			aifl.hitEnemy ? "true" : "false", userData );
	proc ( "idAI", "aifl.pushed",			aifl.pushed ? "true" : "false", userData );
	proc ( "idAI", "aifl.lookAtPlayer",		aifl.lookAtPlayer ? "true" : "false", userData );
	proc ( "idAI", "aifl.disableAttacks",	aifl.disableAttacks ? "true" : "false", userData );
	proc ( "idAI", "aifl.simpleThink",		aifl.simpleThink ? "true" : "false", userData );
	proc ( "idAI", "aifl.action",			aifl.action ? "true" : "false", userData );
	proc ( "idAI", "aifl.scripted",			aifl.scripted? "true" : "false", userData );

	proc ( "idAI", "leader",				leader.GetEntity() ? leader.GetEntity()->GetName() : "<none>", userData );
	proc ( "idAI", "move.followRangeMin",	va("%g",move.followRange[0]), userData );
	proc ( "idAI", "move.followRangeMax",	va("%g",move.followRange[1]), userData );
	proc ( "idAI", "combat.fl.aware",			combat.fl.aware ? "true" : "false", userData );
	proc ( "idAI", "combat.fl.ignoreEnemies",	combat.fl.ignoreEnemies ? "true" : "false", userData );

	proc ( "idAI", "enemy",					enemy.ent ? enemy.ent->GetName() : "<none>", userData );
	proc ( "idAI", "enemy.fl.inFov",		enemy.fl.inFov ? "true" : "false", userData );
	proc ( "idAI", "enemy.fl.visible",		enemy.fl.visible ? "true" : "false", userData );
	proc ( "idAI", "combat.fl.seenEnemyDirectly",combat.fl.seenEnemyDirectly ? "true" : "false", userData );
	proc ( "idAI", "enemy.lastVisibleTime",	va("%d",enemy.lastVisibleTime), userData );
	proc ( "idAI", "combat.maxLostVisTime",	va("%d",combat.maxLostVisTime), userData );
	proc ( "idAI", "enemy.range",			va("%g",enemy.range), userData );
	proc ( "idAI", "enemy.ranged2d",		va("%g",enemy.range2d), userData );
	proc ( "idAI", "lastAttackTime",		va("%d",lastAttackTime), userData );

	proc ( "idAI", "move.fl.done",				move.fl.done ? "true" : "false", userData );
	proc ( "idAI", "move.fl.disabled",			move.fl.disabled ? "true" : "false", userData );
	proc ( "idAI", "move.fl.onGround",			move.fl.onGround ? "true" : "false", userData );
	proc ( "idAI", "move.fl.blocked",			move.fl.blocked ? "true" : "false", userData );
	proc ( "idAI", "move.fl.obstacleInPath",	move.fl.obstacleInPath ? "true" : "false", userData );
	proc ( "idAI", "move.fl.goalUnreachable",	move.fl.goalUnreachable ? "true" : "false", userData );
	proc ( "idAI", "move.fl.moving",			move.fl.moving ? "true" : "false", userData );
	proc ( "idAI", "move.command",				aiMoveCommandString[move.moveCommand], userData );
	proc ( "idAI", "move.status",				aiMoveStatusString[move.moveStatus], userData );
	proc ( "idAI", "move.fl.allowDirectional",	move.fl.allowDirectional ? "true" : "false", userData );
	proc ( "idAI", "move.direction_ideal",		aiMoveDirectionString[move.idealDirection ], userData );
	proc ( "idAI", "move.direction_current",	aiMoveDirectionString[move.currentDirection ], userData );
	proc ( "idAI", "move.yaw_ideal",			va("%d",(int)move.ideal_yaw), userData );
	proc ( "idAI", "move.yaw_current",			va("%d",(int)move.current_yaw), userData );
	proc ( "idAI", "move.fly_roll",				va("%g", move.fly_roll ), userData );
	proc ( "idAI", "move.fly_pitch",			va("%g", move.fly_pitch ), userData );

	proc ( "idAI", "tether",					tether!=NULL?tether->GetName():"<none>", userData );
	proc ( "idAI", "IsTethered()",				IsTethered()?"true":"false", userData );

	proc ( "idAI", "lookTarget",				lookTarget.GetEntity() ? lookTarget.GetEntity()->GetName() : "<none>", userData );
	proc ( "idAI", "talkTarget",				talkTarget.GetEntity() ? talkTarget.GetEntity()->GetName() : "<none>", userData );
	proc ( "idAI", "focusType",					aiFocusString[focusType], userData );
	proc ( "idAI", "look.yaw",					va("%g", lookAng.yaw ), userData );
	proc ( "idAI", "look.pitch",				va("%g", lookAng.pitch ), userData );

	proc ( "idAI", "combat.attackRangeMin",		va("%g",combat.attackRange[0]), userData );
	proc ( "idAI", "combat.attackRangeMax",		va("%g",combat.attackRange[1]), userData );
	proc ( "idAI", "combat.tacticalCurrent",	aiTacticalString[combat.tacticalCurrent], userData );
		
	proc ( "idAI", "action_rangedAttack",		aiActionStatusString[actionRangedAttack.status], userData );
	proc ( "idAI", "action_meleeAttack",		aiActionStatusString[actionMeleeAttack.status], userData );
	proc ( "idAI", "action_leapAttack",			aiActionStatusString[actionLeapAttack.status], userData );
	proc ( "idAI", "action_jumpBack",			aiActionStatusString[actionJumpBack.status], userData );
	proc ( "idAI", "action_evadeLeft",			aiActionStatusString[actionEvadeLeft.status], userData );
	proc ( "idAI", "action_evadeRight",			aiActionStatusString[actionEvadeRight.status], userData );

	proc ( "idAI", "npc_name",					spawnArgs.FindKey("npc_name")?common->GetLocalizedString(spawnArgs.GetString( "npc_name", "<none>")):"<none>", userData );
}

/*
=====================
idAI::DrawRoute
=====================
*/
void idAI::DrawRoute( void ) const {
	if ( aas && move.toAreaNum && move.moveCommand != MOVE_NONE && move.moveCommand != MOVE_WANDER && move.moveCommand != MOVE_FACE_ENEMY && move.moveCommand != MOVE_FACE_ENTITY ) {
		if ( move.moveType == MOVETYPE_FLY ) {
			aas->ShowFlyPath( physicsObj.GetOrigin(), move.toAreaNum, move.moveDest );
		} else {
			aas->ShowWalkPath( physicsObj.GetOrigin(), move.toAreaNum, move.moveDest );
		}
	}
}

/*
=====================
idAI::DrawTactical

Draw the debug tactical information
=====================
*/
void idAI::DrawTactical ( void ) {
	if ( !DebugFilter(ai_debugTactical) ) {
		return;
	}

	// Colors For Lines In This File
	//-------------------------------
	// Majenta		= Move Dest
	// Green / Red	= Enemy (On Player Side, On Enemy Side)
	// Pink			= Tether Radius
	// Grey			= FOV

	// Colors From AAST Draw Debug Info
	//----------------------------------
	// Blue			= Reserved Feature
	// Orange		= Near Feature
	// Yellow		= Look Feature


	// Get Origin
	//------------
	idVec3 origin = GetPhysics()->GetOrigin();
	origin += (GetPhysics()->GetGravityNormal() * 5.0f);

	// Draw FOV (Must be close to player and on enemy team)
	//------------------------------------------------------
	if (team && DistanceTo(gameLocal.GetLocalPlayer())<500.0f) {
		if (!combat.fl.aware) {
			gameRenderWorld->DebugFOV(colorLtGrey, origin, viewAxis[0], fovDot, 300.0f, fovCloseDot, fovCloseRange, 0.3f, 10);
		} else if (!combat.fl.seenEnemyDirectly) {
			gameRenderWorld->DebugFOV(colorMdGrey, origin, viewAxis[0], fovDot, 300.0f, -1.0f, combat.awareRange, 0.3f, 10);
		} else {
 			float alpha = (1.0f - ((float)(gameLocal.GetTime() - enemy.lastVisibleTime) / (float)combat.maxLostVisTime)) * 0.35f;
			gameRenderWorld->DebugFOV(colorDkGrey, origin, viewAxis[0], fovDot, 300.0f, -1.0f, combat.awareRange, Max(alpha, 0.1f), 10);
		}
	}

	// Move Over Head
	//----------------
	origin -= GetPhysics()->GetGravityNormal() * (GetPhysics()->GetBounds()[ 1 ].z - GetPhysics()->GetBounds()[ 0 ].z);

	// Draw Enemy Related Info
	//-------------------------
	if ( enemy.ent ) {
		// Draw Lost Time
		origin -= (GetPhysics()->GetGravityNormal() * 5.0f);
		if ( enemy.lastVisibleTime && (gameLocal.GetTime() - enemy.lastVisibleTime) > combat.maxLostVisTime ) {
			gameRenderWorld->DrawText ( va("losttime: %d", (gameLocal.GetTime() - enemy.lastVisibleTime)), origin, 0.12f, colorRed, gameLocal.GetLocalPlayer()->viewAxis );
		} else if ( enemy.lastVisibleTime && (gameLocal.GetTime() - enemy.lastVisibleTime) > ( combat.maxLostVisTime/2 ) ) {
			gameRenderWorld->DrawText ( va("losttime: %d", (gameLocal.GetTime() - enemy.lastVisibleTime)), origin, 0.12f, colorYellow, gameLocal.GetLocalPlayer()->viewAxis );
		} else if ( enemy.lastVisibleTime ) {
			gameRenderWorld->DrawText ( va("losttime: %d", (gameLocal.GetTime() - enemy.lastVisibleTime)), origin, 0.12f, colorGreen, gameLocal.GetLocalPlayer()->viewAxis );
		}

		// Enemy Chest position for enemy lines
		idVec3 enemyEyePosition;
		idVec3 offset;
		if ( enemy.ent->IsType ( idActor::GetClassType() ) ){
			enemyEyePosition = static_cast<idActor*>(enemy.ent.GetEntity())->GetEyePosition ( );
		} else {
			enemyEyePosition = enemy.ent->GetPhysics()->GetOrigin();
		}
		
		offset = idVec3(0,0,team*2.0f);
		
		gameRenderWorld->DebugLine  ( aiTeamColor[team], GetEyePosition() + offset, enemy.lastVisibleFromEyePosition + offset, 4.0f );
		if ( enemyEyePosition != enemy.lastVisibleEyePosition ) {
			gameRenderWorld->DebugLine  ( aiTeamColor[team], enemy.lastVisibleFromEyePosition + offset, enemy.lastVisibleEyePosition + offset, 4.0f );
			gameRenderWorld->DebugArrow ( aiTeamColor[team], enemy.lastVisibleEyePosition + offset, enemyEyePosition + offset, 4.0f );		
		} else {
			gameRenderWorld->DebugArrow ( aiTeamColor[team], enemy.lastVisibleFromEyePosition + offset, enemyEyePosition + offset, 4.0f );		
		}		
	}

	// Ivalid Cover Timer
	//--------------------
	if ( IsBehindCover() && combat.coverValidTime && combat.coverValidTime < gameLocal.time) {
		origin -= (GetPhysics()->GetGravityNormal() * 5.0f);
		gameRenderWorld->DrawText ( va("invalid cover time: %d", (gameLocal.GetTime() - combat.coverValidTime)), origin, 0.12f, colorRed, gameLocal.GetLocalPlayer()->viewAxis );
	}


	// Vis Crouch
	gameRenderWorld->DebugArrow ( colorBrown, 
								  GetPhysics()->GetOrigin ( ) - GetPhysics()->GetGravityNormal() * combat.visCrouchHeight, 
								  GetPhysics()->GetOrigin ( ) - GetPhysics()->GetGravityNormal() * combat.visCrouchHeight + viewAxis[0] * 16.0f, 10.0f );

	// Vis Stand
	gameRenderWorld->DebugArrow ( colorBrown, 
								  GetPhysics()->GetOrigin ( ) - GetPhysics()->GetGravityNormal() * combat.visStandHeight, 
								  GetPhysics()->GetOrigin ( ) - GetPhysics()->GetGravityNormal() * combat.visStandHeight + viewAxis[0] * 16.0f, 10.0f );

	// Aggression
	if ( combat.aggressiveScale != 1.0f ) {
		origin -= (GetPhysics()->GetGravityNormal() * 5.0f);
		gameRenderWorld->DrawText ( va("aggressive: %g", combat.aggressiveScale), origin, 0.12f, colorYellow, gameLocal.GetLocalPlayer()->viewAxis );
	}

	// Draw anim prefix
	//-----------------------
	if ( animPrefix.Length ( ) ) {
		origin -= (GetPhysics()->GetGravityNormal() * 5.0f);
		gameRenderWorld->DrawText ( va("anim: %s", animPrefix.c_str()), origin, 0.12f, colorCyan, gameLocal.GetLocalPlayer()->viewAxis );
	}

	// Draw focus type
	//-----------------------
	if ( focusType != AIFOCUS_NONE ) {
		origin -= (GetPhysics()->GetGravityNormal() * 5.0f);
		gameRenderWorld->DrawText ( aiFocusString[focusType], origin, 0.12f, colorCyan, gameLocal.GetLocalPlayer()->viewAxis );
	}

	// Draw Tactical Current
	//-----------------------
	origin -= (GetPhysics()->GetGravityNormal() * 5.0f);
	gameRenderWorld->DrawText ( aiTacticalString[combat.tacticalCurrent], origin, 0.12f, colorYellow, gameLocal.GetLocalPlayer()->viewAxis );

	// Draw My Name
	//--------------
	origin -= (GetPhysics()->GetGravityNormal() * 5.0f);
	gameRenderWorld->DrawText(name, origin, 0.12f, colorWhite, gameLocal.GetLocalPlayer()->viewAxis);

	// Draw the tethered radius
	if ( IsTethered ( ) ) {
		tether->DebugDraw ( );
	}

	// Draw Move Destination
	if ( !move.fl.done ) {
		gameRenderWorld->DebugArrow ( colorMagenta, GetPhysics()->GetOrigin(), move.moveDest, 5 );
	} 
}	
