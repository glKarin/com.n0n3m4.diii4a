// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idBotAI::Enter_COMBAT_Vehicle_EvadeEnemy

Bot has a goal, or some other reason to not want to fight, so try to evade/avoid our enemy,
while still attacking them ( if we can ).
================
*/
bool idBotAI::Enter_COMBAT_Vehicle_EvadeEnemy() {

	VEHICLE_COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Vehicle_EvadeEnemy;

	combatMoveType = COMBAT_MOVE_NULL;

	lastAINode = "Vehicle Evade Enemy";

	return true;
}

/*
================
idBotAI::COMBAT_Vehicle_EvadeEnemy
================
*/
bool idBotAI::COMBAT_Vehicle_EvadeEnemy() {
	bool bailOut = false;
	idVec3 vec;

	if ( !ClientIsValid( enemy, -1 ) ) {
		Bot_ResetEnemy();
		return false;
	}

	if ( botVehicleInfo->type == MCP ) {
		assert( actionNum > -1 );

		if ( Bot_CheckActionIsValid( actionNum ) ) {
			vec = botThreadData.botActions[ actionNum ]->GetActionOrigin() - botVehicleInfo->origin;
			if ( vec.LengthSqr() > Square( MCP_PARKED_DIST ) ) {
				Bot_SetupVehicleMove( vec3_zero, -1, actionNum );

				if ( MoveIsInvalid() ) { //mal: this should NEVER happen - but if it does, the bot is better off leaving.
					Bot_ExitVehicleAINode( true );
					Bot_ExitVehicle( false );
					assert( false );
					return false;
				}
				Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else {
				Bot_ExitVehicleAINode( true ); //mal: reached our goal, so just leave, and let the MCP deploy!
				Bot_ExitVehicle( false );
				return false;
			}
		}
	} else {
        if ( AIStack.stackActionNum != ACTION_NULL ) {
			float evadeDist = 1200.0f;

			vec = botThreadData.botActions[ AIStack.stackActionNum ]->origin - botInfo->origin;

			if ( botVehicleInfo->type > ICARUS ) {
				evadeDist = 550.0f;
				bailOut = true;
				vec.z = 0.0f;
			}			

			if ( vec.LengthSqr() > Square( evadeDist ) ) {
				Bot_SetupVehicleMove( vec3_zero, -1, AIStack.stackActionNum );
				bailOut = false;
				if ( MoveIsInvalid() ) {
					VEHICLE_COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Vehicle_AttackEnemy; //mal: if theres a problem getting to our target, just fight our enemy normally.
					return false;
				}
				Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
			}

			if ( vec.LengthSqr() < Square( 800.0f ) && botVehicleInfo->type != MCP  ) { //mal: get to the outpost if MCP, even to the bitter end!
				VEHICLE_COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Vehicle_AttackEnemy; //mal: we're right at our target, so just fight our enemy normally.
				return false;
			}
		} else {
//			assert( false );
			VEHICLE_COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Vehicle_AttackEnemy; 
			return false;
		}
	}

	if ( bailOut ) {
		Bot_ExitVehicle();
		return false;
	}

	Bot_PickBestVehicleWeapon();

	if ( !enemyInfo.enemyVisible && enemyInfo.enemyLastVisTime + 500 < botWorld->gameLocalInfo.time ) {
        UpdateNonVisEnemyInfo();
		
		if ( BotLeftEnemysSight() ) {
            vec = bot_LS_Enemy_Pos;
		} else {
			vec = enemyInfo.enemy_LS_Pos;
		}

		Bot_LookAtLocation( vec, AIM_TURN );
	} else {
		if ( botVehicleInfo->type > ICARUS ) {
			vec = botWorld->clientInfo[ enemy ].origin - botVehicleInfo->origin;
			vec[ 2 ] = 0.0f;
			if ( vec.LengthSqr() > Square( 2500.0f ) && InFrontOfVehicle( botVehicleInfo->entNum, botWorld->clientInfo[ enemy ].origin ) && botVehicleInfo->type != BUFFALO ) {
				Bot_LookAtEntity( enemy, AIM_TURN );
				Bot_CheckVehicleAttack();
			} else {
				Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
				if ( botVehicleInfo->type != BUFFALO ) {
					Bot_CheckVehicleAttack();
				}
			}
		} else {
			Bot_LookAtEntity( enemy, AIM_TURN );
			Bot_CheckVehicleAttack();
		}
	}

	return true;
}

/*
================
idBotAI::Enter_COMBAT_Vehicle_AttackEnemy
================
*/
bool idBotAI::Enter_COMBAT_Vehicle_AttackEnemy() {

	VEHICLE_COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Vehicle_AttackEnemy;

	combatMoveType = COMBAT_MOVE_NULL;

	lastAINode = "Vehicle Attack Enemy";

	return true;
}

/*
================
idBotAI::COMBAT_Vehicle_AttackEnemy
================
*/
bool idBotAI::COMBAT_Vehicle_AttackEnemy() {

	bool keepEnemy = true;

	if ( !enemyInfo.enemyVisible && enemyInfo.enemyLastVisTime + 5000 < botWorld->gameLocalInfo.time ) {
		if ( !Bot_ShouldVehicleChaseHiddenEnemy() ) {
			Bot_ResetEnemy();
			return false; 
		}

        Bot_PickVehicleChaseType();	
		return false;
	}

	Bot_PickBestVehicleWeapon();
	
	if ( vehicleUpdateTime < botWorld->gameLocalInfo.time ) {
        if ( VEHICLE_COMBAT_MOVEMENT_STATE == NULL ) {
			keepEnemy = Bot_FindBestVehicleCombatMovement();
		}
	}

	if ( !keepEnemy ) { //mal: make sure enemy is reachable by our current move state/abilities/limitations. If not, we have to forget them.
		Bot_IgnoreEnemy( enemy, 3000 ); //mal: perhaps in 3 seconds, we will have moved to a better position for a kill...
		Bot_ResetEnemy();
		return false;
	}

	if ( VEHICLE_COMBAT_MOVEMENT_STATE != NULL ) {
		CallFuncPtr( VEHICLE_COMBAT_MOVEMENT_STATE );
	}

	if ( ClientIsValid( enemy, enemySpawnID ) && enemyInfo.enemyVisible ) {
		if ( botVehicleInfo->type <= ICARUS || botInfo->proxyInfo.weapon == MINIGUN ) {
			Bot_LookAtEntity( enemy, AIM_TURN ); //mal: aim at the enemy - but let the game side code handle it.
			Bot_CheckVehicleAttack();
		}
	}

	return false;
}

/*
================
idBotAI::Enter_COMBAT_Vehicle_ChaseEnemy

Bot has an enemy he wants to chase.
================
*/
bool idBotAI::Enter_COMBAT_Vehicle_ChaseEnemy() {

	VEHICLE_COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Vehicle_ChaseEnemy;

	combatMoveType = COMBAT_MOVE_NULL;

	lastAINode = "Vehicle Chase Enemy";

	return true;
}

/*
================
idBotAI::COMBAT_Vehicle_ChaseEnemy
================
*/
bool idBotAI::COMBAT_Vehicle_ChaseEnemy() {

	if ( enemyInfo.enemyVisible ) {
        VEHICLE_COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Vehicle_AttackEnemy;
		return false;
	}

	if ( chaseEnemyTime < botWorld->gameLocalInfo.time ) {
		Bot_ResetEnemy();
		return false; 
	}

    Bot_SetupVehicleMove( vec3_zero, enemy, ACTION_NULL );

    if ( MoveIsInvalid() ) {
		Bot_ResetEnemy();
		return false;
	}

	Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
	Bot_LookAtEntity( enemy, SMOOTH_TURN );

	return true;
}
