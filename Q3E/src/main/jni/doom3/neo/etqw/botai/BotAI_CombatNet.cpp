// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idBotAI::Enter_COMBAT_Foot_AttackEnemy
================
*/
bool idBotAI::Enter_COMBAT_Foot_AttackEnemy() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_AttackEnemy;

	combatMoveType = COMBAT_MOVE_NULL;

	lastAINode = "Attack Enemy";

	ignoreNadeTime = 0;

	combatNBGType = NO_COMBAT_TYPE;

	if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
		ignoreNadeTime = botWorld->gameLocalInfo.time + GRENADE_IGNORE_TIME;
	}

	return true;
}

/*
================
idBotAI::COMBAT_Foot_AttackEnemy
================
*/
bool idBotAI::COMBAT_Foot_AttackEnemy() {
	if ( !enemyInfo.enemyVisible && enemyInfo.enemyLastVisTime + 500 < botWorld->gameLocalInfo.time ) {
		if ( !Bot_ShouldChaseHiddenEnemy( false ) ) {
			Bot_ResetEnemy();
			return false; 
		}

		if ( enemyIsHuntGoal ) {
			COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy;
		} else if ( Bot_CheckShouldUseAircan( false ) ) {
			COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy_Aircan;			
		} else if ( Bot_CheckShouldUseGrenade( false )) {
			COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy_Grenade;	
		} else {
			if ( BotLeftEnemysSight() ) {
				if ( botThreadData.random.RandomInt( 100 ) > 80 ) {
					COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_Hide;
				} else {
					Bot_PickChaseType();
				}
			} else {
				if ( botWorld->clientInfo[ enemy ].xySpeed < 225.0f && enemyInfo.enemyDist < 500.0f ) {
                    if ( botThreadData.random.RandomInt( 100 ) > 80 ) {
						COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_Hide;
					} else {
						Bot_PickChaseType();
					}
				} else {
					Bot_PickChaseType();
				}
			}
		}

		return false;
	}

	bool enemyInVehicle = ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) ? true : false;

	Bot_PickBestWeapon( ( ignoreNadeTime > botWorld->gameLocalInfo.time && !enemyInVehicle ) ? false : true );
	Bot_MedicCheckNBGState_InCombat();

	Bot_CheckForNearbyVehicleToGrab();

	if ( combatDangerExists && combatMoveType != AVOID_DANGER_ATTACK ) {
		COMBAT_MOVEMENT_STATE = NULL;
	}

	if ( COMBAT_MOVEMENT_STATE == NULL || shotIsBlockedCounter > MAX_SHOT_BLOCKED_COUNT ) {
		Bot_FindBestCombatMovement();
	}

	if ( COMBAT_MOVEMENT_STATE != NULL ) {
		CallFuncPtr( COMBAT_MOVEMENT_STATE );
	} else {
		assert( false );
	}

	if ( ClientIsValid( enemy, enemySpawnID ) ) {
		if ( botInfo->usingMountedGPMG ) {
			Bot_LookAtEntity( enemy, AIM_TURN ); //mal: aim at the enemy - but let the game side code handle it.
			botUcmd->botCmds.attack = true;
			return true;
		} else if ( botInfo->weapInfo.weapon == BINOCS && hammerTime ) {
			if ( hammerVehicle ) {
				Bot_LookAtEntity( enemy, SMOOTH_TURN );
			} else {
				Bot_LookAtLocation( hammerLocation, SMOOTH_TURN );
			}
		} else if ( botInfo->weapInfo.weapon != KNIFE ) {
			Bot_LookAtEntity( enemy, AIM_TURN ); //mal: aim at the enemy - but let the game side code handle it.
		} else {
			Bot_LookAtEntity( enemy, FAST_TURN ); //mal: aim at the enemy quickly - but let the game side code handle it.
		}
		Bot_CheckAttack();
	}

	return true;
}

/*
================
idBotAI::Enter_COMBAT_Foot_ChaseEnemy
================
*/
bool idBotAI::Enter_COMBAT_Foot_ChaseEnemy() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_ChaseEnemy;

	combatMoveType = COMBAT_MOVE_NULL;

	combatNBGType = NO_COMBAT_TYPE;

	lastAINode = "Chase Enemy";

	return true;
}

/*
================
idBotAI::COMBAT_Foot_ChaseEnemy
================
*/
bool idBotAI::COMBAT_Foot_ChaseEnemy() {

	float ourDist, enemyDist; // the dists relative to the last seen pos of the enemy
	idVec3 vec, enemyLastOrg;

	if ( enemyInfo.enemyVisible ) {
        COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy;
		return false;
	}

	UpdateNonVisEnemyInfo();
	Bot_MedicCheckNBGState_InCombat();
	botIdealWeapSlot = GUN;
	botUcmd->botCmds.reload = true; //mal_TODO: this is lame - fix this when have total control of weapon info.

	if ( botThreadData.AllowDebugData() ) { 
		gameRenderWorld->DebugCircle( colorRed, enemyInfo.enemy_LS_Pos, idVec3( 0, 0, 1 ), 16, 8 );
		gameRenderWorld->DebugCircle( colorBlue, enemyInfo.enemy_NS_Pos, idVec3( 0, 0, 1 ), 16, 8 );
	}

	enemyLastOrg = ( chaseEnemy ) ? enemyInfo.enemy_NS_Pos : enemyInfo.enemy_LS_Pos;
	vec = enemyLastOrg - botInfo->origin;
	ourDist = vec.LengthFast();
	vec = botWorld->clientInfo[ enemy ].origin - enemyLastOrg;
	enemyDist = vec.LengthFast();

	if ( enemyDist > ENEMY_CHASE_DIST && !enemyIsHuntGoal ) {
		Bot_ResetEnemy();
		return false; 
	}

//mal: chase the bastid's last known position!
	if ( ourDist > 75.0f || botInfo->onLadder ) {

		Bot_SetupMove( enemyLastOrg, -1, ACTION_NULL );
 
		if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME );
			Bot_ResetEnemy();
			return false;
		}

        if ( Bot_CheckSelfSupply() || botInfo->weapInfo.isReloading && ourDist > 200.0f ) {
			Bot_MoveAlongPath( RUN );
		} else {
			Bot_MoveAlongPath( SPRINT );
		}

		Bot_LookAtLocation( enemyLastOrg, SMOOTH_TURN );
		return true;
	}

//mal: we're at the place the bastid was last seen - look around for him, or just leave!
	if ( !chaseEnemy ) {
        if ( !Bot_ShouldChaseHiddenEnemy( true ) ) {
			Bot_ResetEnemy();
		}
	} else {
        Bot_ResetEnemy();
	}

	return true;
}

/*
================
idBotAI::Enter_COMBAT_Foot_ChaseEnemy_Grenade
================
*/
bool idBotAI::Enter_COMBAT_Foot_ChaseEnemy_Grenade() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_ChaseEnemy_Grenade;

	combatMoveType = COMBAT_MOVE_NULL;

	combatNBGType = NO_COMBAT_TYPE;

	lastAINode = "Chase w/ nade";

	return true;
}

/*
================
idBotAI::COMBAT_Foot_ChaseEnemy_Grenade
================
*/
bool idBotAI::COMBAT_Foot_ChaseEnemy_Grenade() {
	bool k;
	float ourDist, enemyDist, ourHeight; // the dists relative to the last seen pos of the enemy
	idVec3 vec, enemyLastOrg;

	if ( enemyInfo.enemyVisible ) {
        COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy;
		return false;
	}

	UpdateNonVisEnemyInfo();
	Bot_MedicCheckNBGState_InCombat();

	if ( botThreadData.AllowDebugData() ) { 
		gameRenderWorld->DebugCircle( colorRed, enemyInfo.enemy_LS_Pos, idVec3( 0, 0, 1 ), 16, 8 );
		gameRenderWorld->DebugCircle( colorBlue, enemyInfo.enemy_NS_Pos, idVec3( 0, 0, 1 ), 16, 8 );
	}

	enemyLastOrg = ( chaseEnemy ) ? enemyInfo.enemy_NS_Pos : enemyInfo.enemy_LS_Pos;
	vec = enemyLastOrg - botInfo->origin;
	ourDist = vec.LengthFast();
	ourHeight = idMath::Ftoi( vec[ 2 ] );
	vec = botWorld->clientInfo[ enemy ].origin - enemyLastOrg;
	enemyDist = vec.LengthFast();

	if ( enemyDist > ENEMY_CHASE_DIST ) {
		Bot_ResetEnemy();
		return false; // either way, leave - hes too far away.
	}

	if ( enemyDist > 700.0f ) {
		botIdealWeapSlot = GUN;
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy;
		return false;
	}

	if ( ourDist > 900.0f ) {
		botIdealWeapSlot = GUN;
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy;
		return false;
	}

    if ( safeGrenade ) {

        botIdealWeapSlot = NADE;
		float moveDist = ( ourHeight >= 150.0f ) ? 500.0f : 300.0f;

		if ( ourDist < moveDist ) {
            if ( Bot_CanMove( BACK, 100.0f, true )) {
  				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else if ( Bot_CanMove( RIGHT, 100.0f, true )) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else if ( Bot_CanMove( LEFT, 100.0f, true )) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else {
				botIdealWeapSlot = GUN;
				COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy;
				return false;
			}
		}

		if ( Bot_ThrowGrenade( enemyLastOrg, false ) ) {

			k = ( botThreadData.random.RandomInt( 100 ) > 80 ) ? true : false;

			if ( !k ) {
				botIdealWeapSlot = GUN;
				COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy;
				return false;
			}
		}
	} else {

		Bot_SetupMove( enemyLastOrg, -1, ACTION_NULL );
  
		if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME );
			Bot_ResetEnemy();
			return false;
		}

		botIdealWeapSlot = NADE;

		Bot_MoveAlongPath( RUN );

		if ( Bot_ThrowGrenade( enemyLastOrg, false ) ) {
		
			k = ( botThreadData.random.RandomInt( 100 ) > 80 ) ? true : false;

			if ( !k ) {
                botIdealWeapSlot = GUN;
				COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy;
				return false;
			}
		}
	}

	return true;
}

/*
================
idBotAI::Enter_COMBAT_Foot_RetreatFromEnemy
================
*/
bool idBotAI::Enter_COMBAT_Foot_RetreatFromEnemy() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_RetreatFromEnemy;

	combatMoveType = COMBAT_MOVE_NULL;

	combatNBGType = NO_COMBAT_TYPE;

	lastAINode = "Retreat!";

	return true;
}

/*
================
idBotAI::COMBAT_Foot_RetreatFromEnemy
================
*/
bool idBotAI::COMBAT_Foot_RetreatFromEnemy() {


	//mal_TODO: code me someday plz!


	return true;
}

/*
================
idBotAI::Enter_COMBAT_Foot_EvadeEnemy
================
*/
bool idBotAI::Enter_COMBAT_Foot_EvadeEnemy() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_EvadeEnemy;

	combatMoveType = COMBAT_MOVE_NULL;

	combatNBGType = NO_COMBAT_TYPE;

	combatTryMoveCounter = 0;

	combatTryMoveTime = 0;

	lastAINode = "Evade Enemy";

	ignoreNadeTime = 0;

	if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
		ignoreNadeTime = botWorld->gameLocalInfo.time + GRENADE_IGNORE_TIME;
	}

	return true;
}

/*
================
idBotAI::COMBAT_Foot_EvadeEnemy
================
*/
bool idBotAI::COMBAT_Foot_EvadeEnemy() {
	bool goalIsPriority = false;
	idVec3 vec;

	if ( combatTryMoveTime < botWorld->gameLocalInfo.time ) {
		combatTryMoveCounter = 0;
	}

	if ( AIStack.STACK_AI_NODE != NULL ) {
		if ( AIStack.stackActionNum > ACTION_NULL && AIStack.stackActionNum < botThreadData.botActions.Num() ) {
			vec = botThreadData.botActions[ AIStack.stackActionNum ]->origin - botInfo->origin;

			if ( vec.LengthFast() > 300.0f ) {
				Bot_SetupMove( vec3_zero, -1, AIStack.stackActionNum );
				if ( MoveIsInvalid() ) {
					COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy; //mal: if theres a problem getting to our target, just fight our enemy normally.
					return false;
				}
				Bot_MoveAlongPath( RUN );
			}

			if ( botThreadData.botActions[ AIStack.stackActionNum ]->GetObjForTeam( botInfo->team ) == ACTION_DEFENSE_CAMP ) {
				goalIsPriority = true;
			}
		} else if ( AIStack.stackClientNum != -1 ) {
			vec = botWorld->clientInfo[ AIStack.stackClientNum ].origin - botInfo->origin;

			if ( vec.LengthFast() > 300.0f ) { //mal: dont get too close when we follow a teammate - and if we are too close, move away some....
				Bot_SetupMove( vec3_zero, AIStack.stackClientNum, ACTION_NULL );
				if ( MoveIsInvalid() ) {
					COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy; //mal: if theres a problem getting to our target, just fight our enemy normally.
					return false;
				}
				combatTryMoveCounter = 0;
				Bot_MoveAlongPath( RUN );
			} else {
				int blockedClient = Bot_CheckBlockingOtherClients( -1 );

				if ( blockedClient != -1 && combatTryMoveCounter < MAX_MOVE_ATTEMPTS ) {
					combatTryMoveCounter++;
					if ( Bot_CanMove( BACK, 100.0f, true ) ) {
						Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
					} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
						Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
					} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
						Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
					}

					Bot_LookAtEntity( blockedClient, SMOOTH_TURN );
					combatTryMoveTime = botWorld->gameLocalInfo.time + 1000;
					return true;
				}
			}
		} else if ( AIStack.stackEntNum != -1 ) {
			if ( AIStack.stackEntNum < MAX_CARRYABLES && ( AIStack.stackLTGType == RECOVER_GOAL || AIStack.stackLTGType == STEAL_GOAL ) ) {
				idVec3 goalOrigin = botWorld->botGoalInfo.carryableObjs[ AIStack.stackEntNum ].origin;
				vec = goalOrigin - botInfo->origin;

				if ( vec.LengthFast() > 25.0f ) {
					Bot_SetupMove( goalOrigin, -1, ACTION_NULL );
					if ( MoveIsInvalid() ) {
						COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy; //mal: if theres a problem getting to our target, just fight our enemy normally.
						return false;
					}
					Bot_MoveAlongPath( RUN );
				} else {
					COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy; //mal: we're right at our target, so just fight our enemy normally.
					return false;
				}
			} else {
				assert( false );
				COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy; //mal: we don't currently handle this entity type, so just fight normally.
			}
		} else {
			assert( false );
			COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy; //mal: we're right at our target, so just fight our enemy normally.
			return false;
		}
	} else {
		assert( false );
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy; //mal: we got here without a valid ai node on the stack. Shouldn't happen, but just in case.
		return false;
	}

	if ( vec.LengthFast() > 2500.0f && !goalIsPriority ) {
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy; //mal: if follow target too far away, just fight our enemy normally.
		return false;
	} else if ( vec.LengthFast() < 300.0f ) {
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy; //mal: we're right at our target, so just fight our enemy normally.
		return false;
	}

	bool enemyInVehicle = ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) ? true : false;

	Bot_PickBestWeapon( ( ignoreNadeTime > botWorld->gameLocalInfo.time && !enemyInVehicle ) ? false : true );
	Bot_MedicCheckNBGState_InCombat();

	if ( !enemyInfo.enemyVisible && enemyInfo.enemyLastVisTime + 500 < botWorld->gameLocalInfo.time ) {
        UpdateNonVisEnemyInfo();
		
		if ( BotLeftEnemysSight() ) {
            vec = bot_LS_Enemy_Pos;
		} else {
			vec = enemyInfo.enemy_LS_Pos;
		}

        if ( Bot_CheckShouldUseAircan( false ) ) {
			Bot_UseCannister( AIRCAN, vec );
			Bot_LookAtLocation( vec, INSTANT_TURN );
		} else if ( Bot_CheckShouldUseGrenade( false ) ) {
			Bot_ThrowGrenade( vec, false );	
		}
	} else {

		bool enemyInDangerousGroundVehicle = false;

		if ( enemyInVehicle ) {
			proxyInfo_t vehicle;
			GetVehicleInfo( botWorld->clientInfo[ enemy ].proxyInfo.entNum, vehicle );

			if ( vehicle.flags & ARMOR ) {
				enemyInDangerousGroundVehicle = true;
			}
		}

		if ( botInfo->classType == FIELDOPS && botInfo->team == STROGG && ClassWeaponCharged( SHIELD_GUN ) && !ClientHasShieldInWorld( botNum, SHIELD_CONSIDER_RANGE ) && ( !InFrontOfClient( botNum, botAAS.path.moveGoal ) || enemyInDangerousGroundVehicle ) ) {
			botIdealWeapNum = SHIELD_GUN;
			botIdealWeapSlot = NO_WEAPON;
			Bot_LookAtEntity( enemy, SMOOTH_TURN );
			if ( botInfo->weapInfo.weapon == SHIELD_GUN ) {
				botUcmd->botCmds.attack = true;
			}
		} else {
			Bot_LookAtEntity( enemy, AIM_TURN );
			Bot_CheckAttack();
		}
	}

	return true;
}


/*
================
idBotAI::Enter_COMBAT_Foot_LostEnemyInSmoke
================
*/
bool idBotAI::Enter_COMBAT_Foot_LostEnemyInSmoke() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_LostEnemyInSmoke;

	combatMoveType = COMBAT_MOVE_NULL;

	combatNBGType = NO_COMBAT_TYPE;

	lastAINode = "Lost Enemy In Smoke";

	return true;
}

/*
================
idBotAI::COMBAT_Foot_LostEnemyInSmoke
================
*/
bool idBotAI::COMBAT_Foot_LostEnemyInSmoke() {



	//mal_TODO: code me someday!

	return true;
}

/*
================
idBotAI::Enter_COMBAT_Foot_ReviveTeammate
================
*/
bool idBotAI::Enter_COMBAT_Foot_ReviveTeammate() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_ReviveTeammate;

	combatMoveType = REVIVE_MATE_ATTACK;

	lastAINode = "Combat Reviving";

	return true;
}

/*
================
idBotAI::COMBAT_Foot_ReviveTeammate
================
*/
bool idBotAI::COMBAT_Foot_ReviveTeammate() {

	bool exitNode = false;
	idVec3 vec, enemyLastOrg;

	if ( combatNBGTime < botWorld->gameLocalInfo.time ) {
		exitNode = true;
	}

	if ( botWorld->clientInfo[ combatNBGTarget ].health > 0 ) {
		exitNode = true;
	}

	if ( !botWorld->clientInfo[ combatNBGTarget ].inGame ) { //mal: client disconnected/got kicked/etc.
		exitNode = true;
	}

	if ( botWorld->clientInfo[ combatNBGTarget ].inLimbo ) { //mal: hes gone jim! Do something else
		exitNode = true;
	}

	if ( exitNode ) {
		botIdealWeapSlot = GUN;
		botIdealWeapNum = SMG;
		COMBAT_AI_SUB_NODE = NULL;
		combatNBGType = NO_COMBAT_TYPE;
		Bot_IgnoreClient( combatNBGTarget, MEDIC_IGNORE_TIME );
		return false;
	}

	bool enemyInVehicle = ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) ? true : false;
	Bot_PickBestWeapon( ( !enemyInVehicle ) ? false : true );

	vec = botWorld->clientInfo[ combatNBGTarget ].origin - botInfo->origin;

	if ( vec.LengthFast() > 65.0f || botInfo->onLadder ) {
		Bot_SetupMove( vec3_zero, combatNBGTarget, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreClient( combatNBGTarget, MEDIC_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			COMBAT_AI_SUB_NODE = NULL;
			combatNBGType = NO_COMBAT_TYPE;
			return false;
		}
		
		Bot_MoveAlongPath( RUN );

		if ( !enemyInfo.enemyVisible && enemyInfo.enemyLastVisTime + 500 < botWorld->gameLocalInfo.time ) {
			UpdateNonVisEnemyInfo();
			Bot_LookAtLocation( enemyInfo.enemy_LS_Pos, SMOOTH_TURN );
		} else {
			Bot_LookAtEntity( enemy, AIM_TURN );
			Bot_CheckAttack();
		} 
		return true;
	}

	if ( !combatNBGReached ) {
		combatNBGReached = true;
		combatNBGTime = botWorld->gameLocalInfo.time + 7000; //mal: give ourselves 7 seconds to complete this task once reach target
	}

	Bot_ClearAngleMods();
	Bot_LookAtEntity( combatNBGTarget, INSTANT_TURN );

	botIdealWeapSlot = SPECIAL1;
	botIdealWeapNum = NEEDLE;

	if ( botInfo->weapInfo.isReady && botInfo->weapInfo.weapon == NEEDLE ) {
		botUcmd->botCmds.attack = true;
		botUcmd->botCmds.constantFire = true;
	}

	return true;
}

/*
================
idBotAI::Enter_COMBAT_Foot_ChaseEnemy_Aircan
================
*/
bool idBotAI::Enter_COMBAT_Foot_ChaseEnemy_Aircan() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_ChaseEnemy_Aircan;

	combatMoveType = COMBAT_MOVE_NULL;

	combatNBGType = NO_COMBAT_TYPE;

	lastAINode = "Chase w/ Aircan";

	return true;
}

/*
================
idBotAI::COMBAT_Foot_ChaseEnemy_Aircan
================
*/
bool idBotAI::COMBAT_Foot_ChaseEnemy_Aircan() {
	float ourDist, enemyDist; // the dists relative to the last seen pos of the enemy
	idVec3 vec, enemyLastOrg;

	if ( enemyInfo.enemyVisible ) {
        COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy;
		return false;
	}

	if ( !ClassWeaponCharged( AIRCAN ) ) {
		botIdealWeapSlot = GUN;
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy;
		return false;
	}

	UpdateNonVisEnemyInfo();

	if ( botThreadData.AllowDebugData() ) { 
		gameRenderWorld->DebugCircle( colorRed, enemyInfo.enemy_LS_Pos, idVec3( 0, 0, 1 ), 16, 8 );
		gameRenderWorld->DebugCircle( colorBlue, enemyInfo.enemy_NS_Pos, idVec3( 0, 0, 1 ), 16, 8 );
	}

	enemyLastOrg = ( chaseEnemy ) ? enemyInfo.enemy_NS_Pos : enemyInfo.enemy_LS_Pos;
	vec = enemyLastOrg - botInfo->origin;
	ourDist = vec.LengthFast();
	vec = botWorld->clientInfo[ enemy ].origin - enemyLastOrg;
	enemyDist = vec.LengthFast();

	if ( enemyDist > ENEMY_CHASE_DIST ) {
		Bot_ResetEnemy();
		return false; 
	}

	if ( enemyDist > 900.0f ) {
		botIdealWeapSlot = GUN;
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy;
		return false;
	}

	if ( ourDist > 1000.0f ) {
		botIdealWeapSlot = GUN;
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy;
		return false;
	}

	if ( ourDist < 600.0f ) {
        if ( Bot_CanMove( BACK, 100.0f, true )) {
  			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		} else if ( Bot_CanMove( RIGHT, 100.0f, true )) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		} else if ( Bot_CanMove( LEFT, 100.0f, true )) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		} else {
			botIdealWeapSlot = GUN;
			COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy;
			return false;
		}
	}

	if ( ourDist > 900.0f || botInfo->onLadder ) {
		
		Bot_SetupMove( enemyLastOrg, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME );
			Bot_ResetEnemy();
			return false;
		}

        Bot_MoveAlongPath( SPRINT );
		Bot_LookAtLocation( enemyLastOrg, SMOOTH_TURN );
		return true;
	}

	Bot_UseCannister( AIRCAN, enemyLastOrg );
	Bot_LookAtLocation( enemyLastOrg, INSTANT_TURN );
	return true;
}


/*
================
idBotAI::Enter_COMBAT_Foot_Hide
================
*/
bool idBotAI::Enter_COMBAT_Foot_Hide() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_Hide;

	combatMoveType = COMBAT_MOVE_NULL;

	combatNBGType = NO_COMBAT_TYPE;

	lastAINode = "Hide";

	return true;
}

/*
================
idBotAI::COMBAT_Foot_Hide
================
*/
bool idBotAI::COMBAT_Foot_Hide() {

	idVec3 vec;

	if ( enemyInfo.enemyVisible ) {
        COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy;
		return false;
	}

	vec = botWorld->clientInfo[ enemy ].origin - bot_LS_Enemy_Pos;

	if ( enemyInfo.enemyLastVisTime + 5000 < botWorld->gameLocalInfo.time || vec.LengthSqr() > Square( 700.0f ) ) {
		Bot_ResetEnemy();
		return false; 
	}

	if ( combatDangerExists ) {
		Bot_SetupQuickMove( botInfo->origin, false ); //mal: just path to itself, if its in an obstacle, it will freak and avoid it.

		if ( MoveIsInvalid() ) {
			COMBAT_AI_SUB_NODE = NULL;
			return false;
		}

		vec = botAAS.path.moveGoal - botInfo->origin;

		if ( vec.LengthSqr() > Square( 25.0f ) ) {
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, SPRINT, NULLMOVETYPE );
		}
	} else {
        Bot_MoveToGoal( vec3_zero, vec3_zero, CROUCH, NULLMOVETYPE );
	}

	Bot_LookAtLocation( bot_LS_Enemy_Pos, SMOOTH_TURN );

	botUcmd->botCmds.reload = true; //mal_TODO: this is lame - fix this when have total control of weapon info.
	
	return true;
}


/*
================
idBotAI::Enter_COMBAT_Foot_ChaseUnseenEnemy
================
*/
bool idBotAI::Enter_COMBAT_Foot_ChaseUnseenEnemy() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_ChaseUnseenEnemy;

	combatMoveType = COMBAT_MOVE_NULL;

	combatNBGType = NO_COMBAT_TYPE;

	lastAINode = "Chase Unseen Enemy";

	return true;
}

/*
================
idBotAI::COMBAT_Foot_ChaseUnseenEnemy
================
*/
bool idBotAI::COMBAT_Foot_ChaseUnseenEnemy() {

	if ( enemyInfo.enemyVisible ) {
        COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy;
		return false;
	}

	Bot_SetupMove( botWorld->clientInfo[ enemy ].origin, -1, ACTION_NULL );

	if ( MoveIsInvalid() ) {
		Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME );
		Bot_ResetEnemy();
		return false;
	}

    Bot_MoveAlongPath( SPRINT );
	return true;
}

/*
================
idBotAI::Enter_COMBAT_Foot_GrabVehicle
================
*/
bool idBotAI::Enter_COMBAT_Foot_GrabVehicle() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_GrabVehicle;

	combatMoveType = COMBAT_MOVE_NULL;

	//mal: make sure these are both NULL, so we can calc a new AI node/move type when enter the vehicle.
	VEHICLE_COMBAT_AI_SUB_NODE = NULL;
	VEHICLE_COMBAT_MOVEMENT_STATE = NULL;

	lastAINode = "Combat Grab Vehicle";

	return true;
}

/*
================
idBotAI::COMBAT_Foot_GrabVehicle
================
*/
bool idBotAI::COMBAT_Foot_GrabVehicle() {

	bool exitNode = false;
	proxyInfo_t vehicleInfo;
	idBox botBox, vehicleBox;

	if ( combatNBGTime < botWorld->gameLocalInfo.time ) {
		exitNode = true;
	}

	GetVehicleInfo( combatNBGTarget, vehicleInfo );

	if ( !VehicleIsValid( vehicleInfo.entNum ) || exitNode ) {	
		COMBAT_AI_SUB_NODE = NULL;
		combatNBGType = NO_COMBAT_TYPE;
		return false;
	}

	bool enemyInVehicle = ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) ? true : false;

	Bot_PickBestWeapon( ( enemyInVehicle ) ? true : false );

	botUcmd->actionEntityNum = vehicleInfo.entNum; //mal: let the game and obstacle avoidance know we want to interact with this entity.
	botUcmd->actionEntitySpawnID = vehicleInfo.spawnID;

	vehicleBox = idBox( vehicleInfo.bbox, vehicleInfo.origin, vehicleInfo.axis );
	vehicleBox.ExpandSelf( VEHICLE_BOX_EXPAND );

	botBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );

	if ( !botBox.IntersectsBox( vehicleBox ) || botInfo->onLadder ) {

		idVec3 vehicleOrigin = vehicleInfo.origin;
		vehicleOrigin.z += VEHICLE_PATH_ORIGIN_OFFSET;
        
		Bot_SetupMove( vehicleOrigin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			COMBAT_AI_SUB_NODE = NULL;
			combatNBGType = NO_COMBAT_TYPE;
			return false;
		}
		
		Bot_MoveAlongPath( RUN );

		if ( !enemyInfo.enemyVisible && enemyInfo.enemyLastVisTime + 500 < botWorld->gameLocalInfo.time ) {
			UpdateNonVisEnemyInfo();
			Bot_LookAtLocation( enemyInfo.enemy_LS_Pos, SMOOTH_TURN );
		} else {
			Bot_LookAtEntity( enemy, AIM_TURN );
			Bot_CheckAttack();
		} 
		return true;
	}
   
	Bot_LookAtLocation( vehicleInfo.origin, SMOOTH_TURN );

	botUcmd->botCmds.enterVehicle = true;
	vehicleEnemyWasInheritedFromFootCombat = true;
	V_ROOT_AI_NODE = &idBotAI::Run_VCombat_Node; //mal: jump right into combat once enter the vehicle.
	return true;
}

/*
================
idBotAI::Enter_COMBAT_Foot_AttackTurret
================
*/
bool idBotAI::Enter_COMBAT_Foot_AttackTurret() {

	COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_AttackTurret;

	combatMoveType = COMBAT_MOVE_NULL;

	combatNBGType = COMBAT_ATTACK_TURRET;

	combatMoveDir = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? RIGHT : LEFT;

	lastAINode = "Attack Turret";

	return true;
}

/*
================
idBotAI::COMBAT_Foot_AttackTurret
================
*/
bool idBotAI::COMBAT_Foot_AttackTurret() {
	deployableInfo_t deployableInfo;

	if ( !GetDeployableInfo( false, combatNBGTarget, deployableInfo ) ) { //mal: doesn't exist anymore.
		COMBAT_AI_SUB_NODE = NULL;
		return false;
	}

	if ( deployableInfo.disabled || deployableInfo.health < ( deployableInfo.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) { //mal: its dead ( enough ).
		COMBAT_AI_SUB_NODE = NULL;
		return false;
	}

	if ( !Bot_HasExplosives( true, true ) ) {
		COMBAT_AI_SUB_NODE = NULL;
		return false;
	}

	trace_t tr;
	idVec3 deployableOrg = deployableInfo.origin;
	deployableOrg.z += DEPLOYABLE_ORIGIN_OFFSET;
	botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, deployableOrg, MASK_SHOT_BOUNDINGBOX | MASK_VEHICLESOLID | CONTENTS_FORCEFIELD, GetGameEntity( botNum ) );

	if ( tr.fraction < 1.0f && tr.c.entityNum != deployableInfo.entNum ) {  //mal: can't see it anymore, so go back to fighting our enemy.
		COMBAT_AI_SUB_NODE = NULL;
		return false;
	} 

	if ( botInfo->classType == FIELDOPS && LocationVis2Sky( deployableInfo.origin ) && ClassWeaponCharged( AIRCAN ) ) {
		botIdealWeapNum = AIRCAN;
		botIdealWeapSlot = NO_WEAPON;
	} else if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo && !botInfo->weapInfo.primaryWeapNeedsReload ) {
		botIdealWeapSlot = GUN;
		botIdealWeapNum = NULL_WEAP;
	} else if ( botInfo->weapInfo.hasNadeAmmo ) {
		botIdealWeapSlot = NADE;
		botIdealWeapNum = NULL_WEAP;
	} else {
		COMBAT_AI_SUB_NODE = NULL;
		assert( false );
		return false;
	}

	idVec3 vec = deployableInfo.origin - botInfo->origin;
	float distToTurretSqr = vec.LengthSqr();

	if ( botIdealWeapSlot == NADE ) {
		if ( distToTurretSqr > Square( GRENADE_THROW_MAXDIST ) ) {
			Bot_SetupMove( deployableOrg, -1, ACTION_NULL );
		
			if ( MoveIsInvalid() ) {
				COMBAT_AI_SUB_NODE = NULL;
				return false;
			}

			Bot_MoveAlongPath( SPRINT );
			botUcmd->moveType = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? RANDOM_JUMP_RIGHT : RANDOM_JUMP_LEFT;
			Bot_ThrowGrenade( deployableOrg, true );
			return true;
		}
		Bot_ThrowGrenade( deployableOrg, true );
	} else if ( botIdealWeapNum == AIRCAN ) {
		Bot_UseCannister( AIRCAN, deployableOrg );
	} else if ( botIdealWeapSlot == GUN ) {
		if ( distToTurretSqr < Square( 500.0f ) ) { // too close, back up some! 
			if ( Bot_CanMove( BACK, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, RANDOM_JUMP );
			} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, RANDOM_JUMP_RIGHT );
			} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, RANDOM_JUMP_LEFT );
			}

			Bot_LookAtLocation( deployableOrg, SMOOTH_TURN ); 
			return true;
		}
		
		Bot_LookAtLocation( deployableOrg, SMOOTH_TURN ); 
		botUcmd->botCmds.attack = true;
	}

	if ( combatMoveDir == NULL_DIR ) {
		combatMoveDir = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? RIGHT : LEFT;
	}

	if ( Bot_CanMove( combatMoveDir, 100.0f, true ) ) {
		Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, ( combatMoveDir == RIGHT ) ? RANDOM_JUMP_RIGHT : RANDOM_JUMP_LEFT );
	} else {
		combatMoveDir = NULL_DIR;
	}

	return true;
}