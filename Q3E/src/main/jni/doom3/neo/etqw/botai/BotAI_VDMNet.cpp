// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../ContentMask.h"
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idBotAI::Run_VLTG_Node

This is the bot's long term goal thinking when he is in a vehicle. 
================
*/
bool idBotAI::Run_VLTG_Node() {
	//mal: first, lets check if we should be running a different node....
	if ( botWorld->gameLocalInfo.inWarmup && botWorld->gameLocalInfo.botsSillyWarmup != false ) {
		Bot_ResetState( true, false );
		ROOT_AI_NODE = &idBotAI::Run_Warmup_Node;
		Bot_ExitVehicle();
		return false;
	}

	if ( botWorld->gameLocalInfo.inEndGame ) {
		Bot_ResetState( true, false );
        ROOT_AI_NODE = &idBotAI::Run_Intermission_Node;
		Bot_ExitVehicle();
		return false;
	}

	if ( ClientIsDead( botNum ) ) {
		Bot_ResetState( true, false );
		ROOT_AI_NODE = &idBotAI::Enter_Run_Dead_Node;
		Bot_ExitVehicle();
		return false;
	}

	if ( botVehicleInfo == NULL ) { //mal: if we're not in a vehicle anymore, run our on-foot thinking
		Bot_ResetState( true, false );
		return false;
	}

	if ( botVehicleInfo->type == MCP && botVehicleInfo->isDeployed ) {
		Bot_ResetState( true, true );
		Bot_ExitVehicle( false );
		return false;
	}

	if ( ( botVehicleInfo->type == ANANSI || botVehicleInfo->type == HORNET ) && botVehicleInfo->driverEntNum != botNum ) {
		if ( botVehicleInfo->driverEntNum > -1 && botVehicleInfo->driverEntNum < MAX_CLIENTS ) {
			const clientInfo_t& pilot = botWorld->clientInfo[ botVehicleInfo->driverEntNum ];

			if ( pilot.isBot ) {
				Bot_ResetState( true, true );
				Bot_ExitVehicle( false );
				return false;
			}
		}
	}

	if ( botVehicleInfo->type == MCP && botVehicleInfo->isImmobilized && !botVehicleInfo->isEMPed ) {
		Bot_ResetState( true, true );
		Bot_ExitVehicle( false );
		return false;
	}

	if ( botVehicleInfo->type == MCP && botWorld->gameLocalInfo.heroMode && TeamHasHuman( botInfo->team ) ) {
		Bot_ExitVehicleAINode( true );
		Bot_ExitVehicle( false );
		return false;
	}

	if ( ( botVehicleInfo->type == HUSKY || botVehicleInfo->type == BADGER || botVehicleInfo->type == ICARUS || botVehicleInfo->type == HOG ) && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
		if ( Bot_VehicleIsUnderAVTAttack() != -1 ) {
			Bot_ExitVehicleAINode( true );
			Bot_ExitVehicle();
			Bot_IgnoreVehicle( botVehicleInfo->entNum, 15000 );
			return false;
		}
	}

	int deployableEnemy = Bot_VehicleIsUnderAVTAttack();

	if ( Bot_IsInHeavyAttackVehicle() && deployableEnemy != -1 && V_LTG_AI_SUB_NODE != NULL && !botVehicleInfo->isAirborneVehicle ) {
		if ( vLTGType != V_DESTROY_DEPLOYABLE ) {
			Bot_ResetState( true, false );
			return false;
		} else if ( deployableEnemy != vLTGTarget ) { //mal: OK, so we're attacking a deployable, so lets make sure its the RIGHT kind in this situation.
			deployableInfo_t deployableInfo;
			GetDeployableInfo( false, vLTGTarget, deployableInfo );

			if ( deployableInfo.entNum != 0 ) {
				if ( deployableInfo.enemyEntNum != botNum && deployableInfo.enemyEntNum != botVehicleInfo->entNum ) {
					Bot_ResetState( true, false );
					return false;
				}
			}
		}
	}

	if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
		if ( Bot_IsInHeavyAttackVehicle() && V_LTG_AI_SUB_NODE != NULL && vLTGType != V_TRAVEL_GOAL && deployableEnemy == -1 && vLTGType != V_DESTROY_DEPLOYABLE && vLTGType != V_GOTO_REVIVE_MATE ) {
			int botDeployableTarget = Bot_HasDeployableTargetGoals( false );

			if ( botDeployableTarget != -1 ) {
				Bot_ResetState( true, false );
				return false;
			}
		}
	}

	if ( timeTilAttackEnemy != -1 && timeTilAttackEnemy < botWorld->gameLocalInfo.time && enemy != -1 ) {
		Bot_SetTimeOnTarget( true );
		V_ROOT_AI_NODE = &idBotAI::Run_VCombat_Node;
		V_LTG_AI_SUB_NODE = NULL;
		return false;
	}

	if ( Bot_CheckHumanRequestingTransport() && Bot_VehicleIsUnderAVTAttack() == -1 ) { //mal: if we've offered to give someone a ride, lets wait here for a second. NOT if under attack!
		return true;
	}

	if ( botVehicleInfo->type != MCP && Bot_CheckForHumanNearByWhoMayWantRide() && Bot_VehicleIsUnderAVTAttack() == -1 ) {
		if ( vehicleCheckForPossibleHumanPassengerChatTime < botWorld->gameLocalInfo.time ) {
			Bot_AddDelayedChat( botNum, NEED_LIFT, 1 );
			botUcmd->botCmds.honkHorn = true;
			vehicleCheckForPossibleHumanPassengerChatTime = botWorld->gameLocalInfo.time + 10000;
		}
		Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, FULL_STOP ); //hit the brakes!
		return true;
	}

	if ( V_LTG_AI_SUB_NODE == NULL && botExitTime < botWorld->gameLocalInfo.time ) { 
		Bot_FindLongTermGoal();
	}

	if ( botVehicleInfo->type == MCP && vLTGType != V_DRIVE_MCP && vLTGType != V_DRIVE_MCP_ROUTE ) {
		Bot_ExitVehicleAINode( true );
		return false;
	}

	botUcmd->botCmds.ackSeatChange = true;

	aiState = VLTG;

	if ( V_LTG_AI_SUB_NODE != NULL ) {
   
		CallFuncPtr( V_LTG_AI_SUB_NODE );		//mal: run the bot's current Long Term Goal think node

		if ( botUcmd->botCmds.exitVehicle == false ) {
            if ( botInfo->classType == MEDIC && vLTGType != V_GOTO_REVIVE_MATE ) { //mal: medics will have NBG in the vehicle think nodes.
				Bot_FindDeadWhileInVehicle();
			}

			if ( vLTGType != V_STOP_VEHICLE ) { 
				if ( botInfo->classType == ENGINEER && ( ( botVehicleInfo->health < ( botVehicleInfo->maxHealth / 2 ) && botVehicleInfo->type != MCP && !( botVehicleInfo->flags & AIR ) ) || ( botVehicleInfo->type == MCP && botVehicleInfo->isImmobilized ) || botVehicleInfo->damagedPartsCount > 0 ) && enemy == -1 && !botVehicleInfo->inWater && ( botVehicleInfo->hasGroundContact || botVehicleInfo->type == DESECRATOR ) ) {
					if ( !Client_IsCriticalForCurrentObj( botNum, -1.0f ) && !VehicleIsIgnored( botInfo->proxyInfo.entNum ) /* && !Bot_IsInHeavyAttackVehicle() */ ) {
						V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_StopVehicle;	//mal: engs will jump out of own vehicle and fix it.
						botWantsVehicleBackTime = botWorld->gameLocalInfo.time + 15000;
					}
				}

				if ( botVehicleInfo->damagedPartsCount > 0 ) { //mal: vehicle is damaged. No way to tell if its major or minor, so just bail out.
					V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_StopVehicle;
				}
			}

			if ( enemy <= -1 ) { 
				Bot_VehicleFindEnemy();
			}
		}
	} else {
//		assert( false ); //mal: panic!
		Bot_ExitVehicle();
	}

	if ( botVehicleInfo->type == PLATYPUS && botVehicleInfo->driverEntNum == botNum && botVehicleInfo->xyspeed < WALKING_SPEED ) {
		if ( ( botVehiclePathList.Num() == 0 && vLTGPauseTime < botWorld->gameLocalInfo.time ) || ( botVehicleInfo->hasGroundContact == true && botVehicleInfo->inWater == false ) ) {
			botPathFailedCounter++;
			
			if ( botPathFailedCounter > MAX_PATH_FAILED_COUNT ) {
				Bot_ExitVehicle();
			}
		}
	}

	return true;

}

/*
================
idBotAI::Run_VCombat_Node

This is the bot's combat thinking when he is in a vehicle. 
================
*/
bool idBotAI::Run_VCombat_Node() {
	proxyInfo_t enemyVehicleInfo;
	trace_t	tr;
	idVec3 end;

	if ( ClientIsDead( enemy ) || !ClientIsValid( enemy, enemySpawnID ) ) {
		if ( !Bot_VehicleFindEnemy() ) {
            V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
			Bot_ResetEnemy();
		}
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ enemy ]; 

	if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		GetVehicleInfo( playerInfo.proxyInfo.entNum, enemyVehicleInfo );

		if ( enemyVehicleInfo.type == MCP && enemyVehicleInfo.isImmobilized && enemyVehicleInfo.driverEntNum == enemy ) {
			if ( !Bot_VehicleFindEnemy() ) {
		       Bot_ResetState( true, false ); 
			}
			return false;
		}
	}

    if ( botVehicleInfo->damagedPartsCount > 1 ) { //mal: vehicle is too damamged to drive, so get out of it.
		Bot_ExitVehicle();
		return false;
	}

	if ( botVehicleInfo->type != BUFFALO && Bot_VehicleIsUnderAVTAttack() != -1 && ( ( botVehicleInfo->flags & ARMOR ) || botVehicleInfo->type > ICARUS ) ) {
		Bot_ResetState( true, true ); 
		return false;
	}

	if ( ( botVehicleInfo->type == BADGER || botVehicleInfo->type == ICARUS || botVehicleInfo->type == HOG ) && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY && botVehicleInfo->driverEntNum == botNum ) {
		if ( Bot_VehicleIsUnderAVTAttack() != -1 ) {
			Bot_ExitVehicleAINode( true );
			Bot_ExitVehicle();
			Bot_IgnoreVehicle( botVehicleInfo->entNum, 15000 );
			return false;
		}
	}

	UpdateEnemyInfo();

	if ( !enemyInfo.enemyVisible && enemyInfo.enemyLastVisTime + 9000 < botWorld->gameLocalInfo.time && !chasingEnemy ) {
		if ( !Bot_VehicleFindEnemy() ) {
            Bot_ResetState( true, false ); 
		}
		return false;
	}

	if ( !vehicleEnemyWasInheritedFromFootCombat ) {
		if ( playerInfo.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE && enemyInfo.enemyDist > ENEMY_VEHICLE_SIGHT_DIST && !ClientHasObj( enemy ) && !ClientIsDefusingOurTeamCharge( enemy ) ) { //mal: we wont bother with ppl on foot too much
		if ( !Bot_VehicleFindEnemy() ) {
            Bot_ResetState( true, false ); 
		}
		return false;
	}
	}

	if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE && enemyInfo.enemyDist > ENEMY_VEHICLE_SIGHT_DIST ) {
		if ( ( !enemyVehicleInfo.isAirborneVehicle && !botVehicleInfo->isAirborneVehicle ) && !ClientHasObj( enemy ) || botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) { //nal: if enemy far away, leave him alone.
			if ( !Bot_VehicleFindEnemy() ) {
			    Bot_ResetState( true, false ); 
			}
			return false;
		}
	}

	if ( botVehicleInfo->type == ICARUS ) {
		IcarusCombatMove();
		return true;
	}

	if ( botInfo->proxyInfo.clientChangedSeats ) { //mal: bot was been booted out of his seat - update and see what our new status is.
		VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
		vehicleUpdateTime = 0;
		botUcmd->botCmds.ackSeatChange = true;
	}

	aiState = VCOMBAT;

	if ( VEHICLE_COMBAT_AI_SUB_NODE == NULL ) {
        Bot_CheckCurrentStateForVehicleCombat();
	}

	Bot_CheckForDangers( true );

	if ( ClientIsValid( enemy, -1 ) ) {
		CallFuncPtr( VEHICLE_COMBAT_AI_SUB_NODE ); //mal: run the bot's current combat think node
	}

	Bot_VehicleFindBetterEnemy(); //mal: the last thing to run

	return true;
}

/*
================
idBotAI::Enter_VLTG_RoamGoal

The bot is in a vehicle, and wants to move to a location on the map. 
================
*/
bool idBotAI::Enter_VLTG_RoamGoal() {
	vLTGChat = false;
	vLTGPauseTime = 0;

	if ( botInfo->proxyInfo.time + 3000 > botWorld->gameLocalInfo.time ) {
		if ( !( botVehicleInfo->flags & PERSONAL ) && botVehicleInfo->hasFreeSeat ) {
			int matesInArea = ClientsInArea( botNum, botInfo->origin, 1200.0f, botInfo->team, NOCLASS, false, false, false, true, true );

			if ( matesInArea > 0 ) {
				vLTGPauseTime = botWorld->gameLocalInfo.time + ( botWorld->gameLocalInfo.botPauseInVehicleTime * 1000 );

				if ( TeamHumanNearLocation( botInfo->team, botInfo->origin, 1200.0f, true, NOCLASS, true, true ) ) {
					botUcmd->botCmds.honkHorn = true; //mal: honk our horn to get the humans attention.
					vLTGChat = true;
				}
			} 
		}
	}

	vLTGType = V_ROAM_GOAL;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_RoamGoal;

	vLTGTime = botWorld->gameLocalInfo.time + 60000;

	vLTGDeployableTargetAttackTime = 0;
	
	vLTGDeployableTarget = -1;

	lastAINode = "Vehicle Roam Goal";

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::VLTG_RoamGoal
================
*/
bool idBotAI::VLTG_RoamGoal() {
 	botMoveTypes_t moveType = NULLMOVETYPE;

	if ( vLTGTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( botVehicleInfo->driverEntNum != botNum ) { //mal: if someone moved us out of our seat, either take over the vehicle, or just hang out. We're just looking for trouble anyhow.
		if ( botVehicleInfo->driverEntNum == -1 ) {
			botUcmd->botCmds.becomeDriver = true;
		} else if ( VehicleHasGunnerSeatOpen( botInfo->proxyInfo.entNum ) ) {
			botUcmd->botCmds.becomeGunner = true;
		} else {
			if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON ) {
				Bot_ExitVehicleAINode( true );
				Bot_ExitVehicle();
			} else {
                int deployableTargetToKill = Bot_CheckForDeployableTargetsWhileVehicleGunner();

				if ( deployableTargetToKill != -1 ) {
					Bot_AttackDeployableTargetsWhileVehicleGunner( deployableTargetToKill );
				} else {
					vLTGDeployableTargetAttackTime = 0;
					vLTGDeployableTarget = -1;
					if ( botThreadData.random.RandomInt( 100 ) > RANDOMLY_LOOK_AROUND_WHILE_VEHICLE_GUNNER_CHANCE ) {
						idVec3 vec;
						if ( Bot_RandomLook( vec ) )  {
							Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot. If we're not in a vehicle that lets us look around, this will be pointless.
						}
					}
				}
			}
		}
		return true;
	}

	if ( botWorld->gameLocalInfo.time < vLTGPauseTime ) { //mal: since bot code takes a frame or two to update, make sure still have a free seat before chat and wait....
		if ( botVehicleInfo->hasFreeSeat ) {
			botMoveTypes_t moveType = ( botVehicleInfo->type > ICARUS ) ? LAND : FULL_STOP;
			Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, moveType );
            if ( vLTGChat ) {
				Bot_AddDelayedChat( botNum, NEED_LIFT, 1 );
				vLTGChat = false;
			}
		} else {
			vLTGPauseTime = 0;
			vLTGChat = false;
		}
		return true;
	}

	idVec3 vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;

	if ( botVehicleInfo->type > ICARUS ) {
		vec[ 2 ] = 0.0f;
	}

	float dist = vec.LengthSqr();

	int matesInArea = VehiclesInArea( botNum, botThreadData.botActions[ actionNum ]->GetActionOrigin(), botThreadData.botActions[ actionNum ]->GetRadius(), botInfo->team, false, botVehicleInfo->entNum );

	if ( matesInArea > 0 && dist < Square( botThreadData.botActions[ actionNum ]->GetRadius() + 950.0f ) ) {
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) ) {

		Bot_SetupVehicleMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitVehicleAINode( true );
			return false;
		}

		if ( ( botVehicleInfo->type == ANANSI || botVehicleInfo->type == HORNET ) && botVehicleInfo->forwardSpeed > 500.0f ) {
			if ( dist < Square( botThreadData.botActions[ actionNum ]->radius + HORNET_SLOWDOWN_DIST ) ) {
				moveType = AIR_COAST;
			}
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, ( dist > Square( 500.0f ) ) ? SPRINT : RUN, moveType );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	Bot_ExitVehicleAINode( true ); //mal: reached our roam goal, so just leave.
	return true;
}

/*
================
idBotAI::Enter_VLTG_GotoReviveMate
================
*/
bool idBotAI::Enter_VLTG_GotoReviveMate() {
	vLTGChat = false;
	
	vLTGPauseTime = 0;
    
	vLTGType = V_GOTO_REVIVE_MATE;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_GotoReviveMate;
	
	vLTGTime = botWorld->gameLocalInfo.time + 25000; //mal: 25 seconds to revive this guy!

	vLTGOrigin = botWorld->clientInfo[ vLTGTarget ].origin;

	vLTGOrigin[ 2 ] += 24.0f; //mal: raise the origin up a bit, to make it easier to trace for.

	vLTGReached = false;

	Bot_FindParkingSpotAoundLocaction( vLTGOrigin );
    
	lastAINode = "Vehicle Reviving Mate";

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;	
}

/*
================
idBotAI::VLTG_GotoReviveMate
================
*/
bool idBotAI::VLTG_GotoReviveMate() {
    
	float dist;
	idVec3 vec;

	if ( !ClientIsValid( vLTGTarget, vLTGTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( vLTGTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreClient( vLTGTarget, MEDIC_IGNORE_TIME );
		Bot_ExitVehicleAINode( true );
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ vLTGTarget ];

	if ( playerInfo.inLimbo || playerInfo.health > 0 ) { //mal: hes gone jim or hes already revived/respawned! Do something else
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( playerInfo.areaNum == 0 ) {
		Bot_IgnoreClient( vLTGTarget, MEDIC_IGNORE_TIME );
		Bot_ExitVehicleAINode( true );
		return false; //mal: one of us somehow got in a invalid area - so forget getting them and do something else.
	}

	vec = vLTGOrigin - botInfo->origin;
	dist = vec.LengthSqr();

    if ( botVehicleInfo->driverEntNum != botNum ) { //mal: some human took over control of my vehicle - so can't revive!
		if ( botVehicleInfo->driverEntNum == -1 ) {
			botUcmd->botCmds.becomeDriver = true;
		} else if ( dist < Square( MEDIC_RANGE ) && !botVehicleInfo->inWater && botVehicleInfo->inPlayZone ) {
			Bot_ExitVehicle(); //mal: we're close enough and its safe, we can jump out and still revive.
		} else {
			Bot_IgnoreClient( vLTGTarget, MEDIC_IGNORE_TIME );
			Bot_ExitVehicleAINode( true ); //mal: we're just gonnna ignore this client and do something else....
		}
		return false;
	}

	if ( dist > Square( 600.0f ) && !vLTGReached ) {
		Bot_SetupVehicleMove( vLTGOrigin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreClient( vLTGTarget, MEDIC_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ExitVehicleAINode( true );
			return false;
		}
		
		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );

		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );

		return true;
	}

	vLTGReached = true; //mal: once we reach our goal point, don't come back here again!

	if ( botInfo->xySpeed > 50.0f && !botVehicleInfo->bbox.IntersectsBounds( botWorld->clientInfo[ vLTGTarget ].absBounds ) ) { 
        Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, FULL_STOP ); //hit the brakes!
		Bot_LookAtLocation( vLTGOrigin, SMOOTH_TURN );
		return true;
	} else {
		Bot_MoveToGoal( vLTGOrigin, vec3_zero, RUN, NULLMOVETYPE ); //mal: dont park on top of the player we're trying to revive!
	}


	Bot_ExitVehicle();

	return true;	
}

/*
================
idBotAI::Enter_VLTG_TravelGoal

The bot is in a vehicle, and wants to move to a location on the map. 
The bot won't look for gunners/passengers because he has somewhere important to go to ( which is similiar to human behavior ).
================
*/
bool idBotAI::Enter_VLTG_TravelGoal() {
	vLTGChat = false;
	
	vLTGPauseTime = 0;

	vLTGType = V_TRAVEL_GOAL;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_TravelGoal;

	vLTGTime = botWorld->gameLocalInfo.time + 120000;

	lastAINode = "Vehicle Travel Goal";

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::VLTG_TravelGoal
================
*/
bool idBotAI::VLTG_TravelGoal() {
	float dist;
	float minimumDist = ( botVehicleInfo->type == ICARUS ) ? 700.0f : 500.0f;
	idVec3 vec;
	idVec3 goalOrigin;

	if ( vLTGTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( botThreadData.AllowDebugData() && bot_debugActionGoalNumber.GetInteger() == actionNum ) {
		goto debugSkipChecks;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitVehicleAINode( true );
		return false;
	}

debugSkipChecks:

	if ( botVehicleInfo->driverEntNum != botNum ) { //mal: if someone moved us out of our seat, either take over the vehicle, or leave. We have a goal we want to do.
		if ( botVehicleInfo->driverEntNum == -1 ) {
			botUcmd->botCmds.becomeDriver = true;
		} else {
            Bot_ExitVehicleAINode( true );
			Bot_ClearVehicleOffAIStack();
			Bot_ExitVehicle();
		}
		return false;
	}

	goalOrigin = botThreadData.botActions[ actionNum ]->GetActionOrigin();

	if ( botVehicleInfo->type == ICARUS ) {
		if ( botThreadData.botActions[ actionNum ]->GetStroggObj() == ACTION_STEAL || botThreadData.botActions[ actionNum ]->GetStroggObj() == ACTION_DELIVER ) {
			if ( botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].inuse ) {
				goalOrigin = botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].origin;
			}
		}
	}
	
	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;

	if ( botVehicleInfo->type >= ICARUS ) {
		vec[ 2 ] = 0.0f;
	}

	dist = vec.LengthSqr();

	if ( dist > Square( minimumDist ) ) {

		Bot_SetupVehicleMove( goalOrigin, -1, ACTION_NULL );

		if ( botAAS.hasReachedVehicleNodeGoal && botVehicleInfo->type < ICARUS ) {
			Bot_ExitVehicleAINode( true );
			Bot_ClearVehicleOffAIStack();
			Bot_ExitVehicle();
			Bot_IgnoreVehicle( botInfo->proxyInfo.entNum, 15000 );
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			return true;
		}

		if ( botAAS.hasPath == false && botVehicleInfo->type == ICARUS ) {
			Bot_ExitVehicleAINode( true );
			Bot_ExitVehicle();
			Bot_IgnoreVehicle( botVehicleInfo->entNum, 15000 );
			ignoreIcarusTime = botWorld->gameLocalInfo.time + IGNORE_ICARUS_TIME;
			return false;
		}

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitVehicleAINode( true );
			return false;
		}

		if ( ( botVehicleInfo->type == ANANSI || botVehicleInfo->type == HORNET ) && botVehicleInfo->forwardSpeed > HORNET_TOOFAST_SPEED ) { //mal: ugly special case hack, because hornet is so twitchy.
			if ( dist < Square( botThreadData.botActions[ actionNum ]->radius + HORNET_SLOWDOWN_DIST ) ) {
				botUcmd->botCmds.isBlocked = true;
			}
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, ( dist > Square( 500.0f ) ) ? SPRINT : RUN, ( dist < Square( 500.0f ) && botInfo->xySpeed > RUNNING_SPEED ) ? HAND_BRAKE : NULLMOVETYPE );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	if ( botVehicleInfo->type == ICARUS && !botInfo->hasGroundContact ) { //mal: land before we bail out.
		return true;
	} else {
		Bot_ExitVehicleAINode( true );
		Bot_ClearVehicleOffAIStack();
		Bot_ExitVehicle();
		Bot_IgnoreVehicle( botInfo->proxyInfo.entNum, 15000 );
		if ( botVehicleInfo->type == ICARUS ) {
			ignoreIcarusTime = botWorld->gameLocalInfo.time + IGNORE_ICARUS_TIME;
		}
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		return true;
	}
}

/*
================
idBotAI::Enter_VLTG_RideWithMate

The bot is in a vehicle riding along with someone he wants to escort/protect. 
================
*/
bool idBotAI::Enter_VLTG_RideWithMate() {
	vLTGChat = false;
	
	vLTGPauseTime = 0;

	vLTGType = V_FOLLOW_TEAMMATE;
	
	vLTGTime = botWorld->gameLocalInfo.time + FOLLOW_MATE_TIMELIMIT;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_RideWithMate;

	lastAINode = "Ride With Teammate";

	vehicleAINodeSwitch.nodeSwitchCount++;

	vLTGDeployableTargetAttackTime = 0;
	
	vLTGDeployableTarget = -1;

	return true;
}

/*
================
idBotAI::VLTG_RideWithMate
================
*/
bool idBotAI::VLTG_RideWithMate() {
	if ( !ClientIsValid( vLTGTarget, vLTGTargetSpawnID ) ) {
		Bot_ExitVehicleAINode( true );
		Bot_ExitVehicle();
		return false;
	}

	if ( botWorld->clientInfo[ vLTGTarget ].proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) { //mal: our escort target left our ride, see if hes important enough to follow!
		if ( !Bot_IsInHeavyAttackVehicle() && ( Client_IsCriticalForCurrentObj( vLTGTarget, 3000.0f ) || ClientHasObj( vLTGTarget ) ) ) {
			Bot_ExitVehicle();
			Bot_ExitVehicleAINode( true );
			ltgTarget = vLTGTarget; 
			ltgType = FOLLOW_TEAMMATE;
			ltgTargetSpawnID = botWorld->clientInfo[ vLTGTarget ].spawnID;
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_FollowMate;
		} else {
			Bot_ExitVehicleAINode( true );
		}
		return false;
	}
 
	if ( vLTGTime < botWorld->gameLocalInfo.time ) {
		Bot_ExitVehicleAINode( true );
		Bot_ExitVehicle();
		return false;
	}

	Bot_PickBestVehiclePosition(); //mal: lets us move into the gunners seat, if it becomes available ( like on the badger ).

	if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON && botUcmd->botCmds.becomeGunner == false && botUcmd->botCmds.becomeDriver == false ) {
		if ( !Client_IsCriticalForCurrentObj( vLTGTarget, -1.0f ) && !ClientHasObj( vLTGTarget ) ) {
			Bot_ExitVehicle();
			Bot_ExitVehicleAINode( true );
			return false;
		}
	}

	if ( botVehicleInfo->driverEntNum == botNum ) { //mal: we're in the drivers seat now, take control!
		Bot_ExitVehicleAINode( true );
		return false;
	}	

	if ( Bot_WithObjShouldLeaveVehicle() || Bot_WhoIsCriticalForCurrentObjShouldLeaveVehicle() ) {
		Bot_ExitVehicleAINode( true );
		Bot_ExitVehicle();
		return false;
	}

	int deployableTargetToKill = Bot_CheckForDeployableTargetsWhileVehicleGunner();

	if ( deployableTargetToKill != -1 ) {
		Bot_AttackDeployableTargetsWhileVehicleGunner( deployableTargetToKill );
	} else {
		vLTGDeployableTargetAttackTime = 0;
		vLTGDeployableTarget = -1;
		if ( botThreadData.random.RandomInt( 100 ) > RANDOMLY_LOOK_AROUND_WHILE_VEHICLE_GUNNER_CHANCE ) {
			idVec3 vec;
			if ( Bot_RandomLook( vec ) )  {
				 Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot. If we're not in a vehicle that lets us look around, this will be pointless.
			}
		}
	}

	return true;
}

/*
================
idBotAI::Enter_VLTG_StopVehicle

The bots wants to stop/land vehicle NOW! Could be for a variety of reasons. 
================
*/
bool idBotAI::Enter_VLTG_StopVehicle() {
	vLTGChat = false;
	
	vLTGPauseTime = 0;

	vLTGType = V_STOP_VEHICLE;
	
	vLTGTime = botWorld->gameLocalInfo.time + BOT_INFINITY;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_StopVehicle;

	lastAINode = "Stopping Vehicle";

	if ( botVehicleInfo->driverEntNum != botNum && botInfo->classType == ENGINEER ) {
		if ( botVehicleInfo->driverEntNum > -1 && botVehicleInfo->driverEntNum < MAX_CLIENTS ) {
			const clientInfo_t& player = botWorld->clientInfo[ botVehicleInfo->driverEntNum ];
			
			if ( !player.isBot ) {
				Bot_AddDelayedChat( botNum, STOP_WILL_FIX_RIDE, 1 );
			}
		}
	}

	vLTGDeployableTargetAttackTime = 0;
	
	vLTGDeployableTarget = -1;

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::VLTG_StopVehicle
================
*/
bool idBotAI::VLTG_StopVehicle() {
	idVec3 vec;

	if ( botVehicleInfo->type < ICARUS ) {
		if ( botInfo->xySpeed > WALKING_SPEED ) {
			Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, HAND_BRAKE );
		} else {
			Bot_ExitVehicle();
		}	
	} else {
		if ( botInfo->xySpeed > WALKING_SPEED || !botVehicleInfo->hasGroundContact ) {
			Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, LAND );
		} else {
			Bot_ExitVehicle();
		}
	}

	if ( botVehicleInfo->driverEntNum != botNum ) {
        int deployableTargetToKill = Bot_CheckForDeployableTargetsWhileVehicleGunner();

		if ( deployableTargetToKill != -1 ) {
			Bot_AttackDeployableTargetsWhileVehicleGunner( deployableTargetToKill );
		} else {
			vLTGDeployableTargetAttackTime = 0;
			vLTGDeployableTarget = -1;
			if ( botThreadData.random.RandomInt( 100 ) > RANDOMLY_LOOK_AROUND_WHILE_VEHICLE_GUNNER_CHANCE ) {
				idVec3 vec;
				if ( Bot_RandomLook( vec ) )  {
					 Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot. If we're not in a vehicle that lets us look around, this will be pointless.
				}
			}
		}
	}

	return true;
}

/*
================
idBotAI::Enter_VLTG_CampGoal

The bot is in a vehicle, and wants to camp a location on the map. 
================
*/
bool idBotAI::Enter_VLTG_CampGoal() {
	vLTGChat = false;
	vLTGPauseTime = 0;

	if ( botInfo->proxyInfo.time + 3000 > botWorld->gameLocalInfo.time ) {
		if ( !( botVehicleInfo->flags & PERSONAL ) && botVehicleInfo->hasFreeSeat ) {
			int matesInArea = ClientsInArea( botNum, botInfo->origin, 1200.0f, botInfo->team, NOCLASS, false, false, false, true, true );

			if ( matesInArea > 0 ) {
				vLTGPauseTime = botWorld->gameLocalInfo.time + ( botWorld->gameLocalInfo.botPauseInVehicleTime * 1000 );

				if ( TeamHumanNearLocation( botInfo->team, botInfo->origin, 1200.0f, true, NOCLASS, true, true ) ) {
					botUcmd->botCmds.honkHorn = true; //mal: honk our horn to get the humans attention.
					vLTGChat = true;
				}
			} 
		}
	}

	vLTGType = V_CAMP_GOAL;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_CampGoal;

	vLTGTime = botWorld->gameLocalInfo.time + 60000;

	ResetRandomLook(); //mal: start out looking this far at random targets

	vLTGReached = false;

	vLTGDeployableTargetAttackTime = 0;
	
	vLTGDeployableTarget = -1;

	lastAINode = "Vehicle Camp Goal";

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::VLTG_CampGoal
================
*/
bool idBotAI::VLTG_CampGoal() {
 	int i;
	int k = 0;
	int randomChance = 97;
	int linkedTargets[ MAX_LINKACTIONS ];
	float dist;
	botMoveTypes_t moveType = NULLMOVETYPE;
	idVec3 vec;

	if ( vLTGTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave! 
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( botVehicleInfo->driverEntNum != botNum ) { //mal: if someone moved us out of our seat, either take over the vehicle, or just hang out. We're just looking for trouble anyhow.
		if ( botVehicleInfo->driverEntNum == -1 ) {
			botUcmd->botCmds.becomeDriver = true;
		} else if ( VehicleHasGunnerSeatOpen( botInfo->proxyInfo.entNum ) ) {
			botUcmd->botCmds.becomeGunner = true;
		} else {
			if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON ) {
				Bot_ExitVehicleAINode( true );
				Bot_ExitVehicle();
			} else {
                int deployableTargetToKill = Bot_CheckForDeployableTargetsWhileVehicleGunner();

				if ( deployableTargetToKill != -1 ) {
					Bot_AttackDeployableTargetsWhileVehicleGunner( deployableTargetToKill );
				} else {
					vLTGDeployableTargetAttackTime = 0;
					vLTGDeployableTarget = -1;
					if ( botThreadData.random.RandomInt( 100 ) > RANDOMLY_LOOK_AROUND_WHILE_VEHICLE_GUNNER_CHANCE ) {
						idVec3 vec;
						if ( Bot_RandomLook( vec ) )  {
							Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot. If we're not in a vehicle that lets us look around, this will be pointless.
						}
					}
				}
			}
		}
		return true;
	}

	if ( botWorld->gameLocalInfo.time < vLTGPauseTime ) { //mal: since bot code takes a frame or two to update, make sure still have a free seat before chat and wait....
		if ( botVehicleInfo->hasFreeSeat ) {
			botMoveTypes_t moveType = ( botVehicleInfo->type > ICARUS ) ? LAND : FULL_STOP;
			Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, moveType );
            if ( vLTGChat ) {
				Bot_AddDelayedChat( botNum, NEED_LIFT, 1 );
				vLTGChat = false;
			}
		} else {
			vLTGPauseTime = 0;
			vLTGChat = false;
		}
		return true;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;

	if ( botVehicleInfo->type > ICARUS ) {
		vec[ 2 ] = 0.0f;
	}

	dist = vec.LengthSqr();

	int matesInArea = VehiclesInArea( botNum, botThreadData.botActions[ actionNum ]->GetActionOrigin(), botThreadData.botActions[ actionNum ]->GetRadius(), botInfo->team, false, botVehicleInfo->entNum );

	if ( matesInArea > 0 && dist < Square( botThreadData.botActions[ actionNum ]->GetRadius() + 950.0f ) ) {
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) ) {

		Bot_SetupVehicleMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitVehicleAINode( true );
			return false;
		}

		if ( ( botVehicleInfo->type == ANANSI || botVehicleInfo->type == HORNET ) && botVehicleInfo->forwardSpeed > 500.0f ) {
			if ( dist < Square( botThreadData.botActions[ actionNum ]->radius + HORNET_SLOWDOWN_DIST ) ) {
				moveType = AIR_COAST;
			}
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, ( dist > Square( 500.0f ) ) ? SPRINT : RUN, moveType );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	if ( botVehicleInfo->isAirborneVehicle ) { //mal: air vehicles just treat this as a roam.
		Bot_ExitVehicleAINode( true );
		return false;
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, FULL_STOP );

	if ( botThreadData.random.RandomInt( 100 ) > randomChance || vLTGReached == false ) {
		if ( botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].inuse != false ) { //mal: if the first target action == false, they all must, so skip
			for( i = 0; i < MAX_LINKACTIONS; i++ ) {
				if ( botThreadData.botActions[ actionNum ]->actionTargets[ i ].inuse != false ) {
					linkedTargets[ k++ ] = i;
				}
			}

			i = botThreadData.random.RandomInt( k );

			Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->actionTargets[ i ].origin, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
		} else {
			if ( Bot_RandomLook( vec ) )  {
				Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
			}
		}
	}

	if ( !vLTGReached ) {
		vLTGTime = botWorld->gameLocalInfo.time + ( botThreadData.botActions[ actionNum ]->actionTimeInSeconds * 1000 ); //mal: timelimit is specified by action itself!
		vLTGReached = true;
	}

	return true;
}

/*
================
idBotAI::Enter_VLTG_DriveMCPToGoal

The bot is in the MCP, taking it to the outpost. 
================
*/
bool idBotAI::Enter_VLTG_DriveMCPToGoal() {

	vLTGType = V_DRIVE_MCP;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_DriveMCPToGoal;

	lastAINode = "Drive MCP Goal";

	vLTGDeployableTargetAttackTime = 0;
	
	vLTGDeployableTarget = -1;

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::VLTG_DriveMCPToGoal
================
*/
bool idBotAI::VLTG_DriveMCPToGoal() {
	idVec3 vec;

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off - shouldn't happen
		Bot_ExitVehicleAINode( true );
		Bot_ExitVehicle( false );
		return false;
	}

	if ( !ignoreMCPRouteAction ) {
		if ( botVehicleInfo->actionRouteNumber != ACTION_NULL && !ActionIsIgnored( botVehicleInfo->actionRouteNumber ) ) {
			Bot_ExitVehicleAINode( true );
			return false;
		}
	}

	if ( botVehicleInfo->driverEntNum != botNum ) { //mal: if someone moved us out of our seat, either take over the vehicle, or just hang out. We're just looking for trouble anyhow.
		if ( botVehicleInfo->driverEntNum == -1 ) {
			botUcmd->botCmds.becomeDriver = true;
		} else {
            int deployableTargetToKill = Bot_CheckForDeployableTargetsWhileVehicleGunner();

			if ( deployableTargetToKill != -1 ) {
				Bot_AttackDeployableTargetsWhileVehicleGunner( deployableTargetToKill );
			} else {
				vLTGDeployableTargetAttackTime = 0;
				vLTGDeployableTarget = -1;
				if ( botThreadData.random.RandomInt( 100 ) > RANDOMLY_LOOK_AROUND_WHILE_VEHICLE_GUNNER_CHANCE ) {
					idVec3 vec;
					if ( Bot_RandomLook( vec ) )  {
						Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot. If we're not in a vehicle that lets us look around, this will be pointless.
					}
				}
			}
		}
		return true;
	}

	if ( botVehicleInfo->isImmobilized && !botVehicleInfo->isEMPed ) {
		Bot_ExitVehicleAINode( true );
		Bot_ExitVehicle( false );
		return false;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botVehicleInfo->origin;
	float distSqr = vec.LengthSqr();

	if ( distSqr > Square( MCP_PARKED_DIST ) ) {

		Bot_SetupVehicleMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) { //mal: this better NEVER happen! If so, something is really messed up!
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitVehicleAINode( true );
			Bot_ExitVehicle( false ); //mal: Its possible MCP is out of bounds, in which case it should self-destruct soon.
			assert( false );
			return false;
		}

		if ( distSqr < Square( 500.0f ) ) { //mal: start slowing down as approach our goal.
			botUcmd->specialMoveType = SLOWMOVE;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	Bot_ExitVehicleAINode( true ); //mal: reached the outpost! Now leave, and let the MCP deploy.
	Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME );
	Bot_ExitVehicle( false );
	return true;
}

/*
================
idBotAI::Enter_VLTG_GroundVehicleDestroyDeployable
================
*/
bool idBotAI::Enter_VLTG_GroundVehicleDestroyDeployable() {

	vLTGType = V_DESTROY_DEPLOYABLE;

	vLTGTimer = 0;

	vLTGTimer2 = 0;

	vLTGMoveTimer = 0;

	vLTGVisTimer = 0;

	vLTGSightTime = 0;
	
	vLTGDriveTime = 0;

	vLTGReached = false; //mal: goliath will do alternating checks from his viewOrigin, and weap origin, to verify the target is visible.

	vLTGTime = botWorld->gameLocalInfo.time + 120000;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_GroundVehicleDestroyDeployable;

	lastAINode = "Destroy Deployable";

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::VLTG_GroundVehicleDestroyDeployable
================
*/
bool idBotAI::VLTG_GroundVehicleDestroyDeployable() { 
	if ( vLTGTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitVehicleAINode( true );
		botUcmd->botCmds.becomeDriver = true;
		return false;
	}

	float attackDist = WEAPON_LOCK_DIST;
	deployableInfo_t deployable;
	botMoveFlags_t vehicleMove = RUN;

	if ( !GetDeployableInfo( false, vLTGTarget, deployable ) ) { //mal: doesn't exist anymore.
		Bot_ExitVehicleAINode( true );
		botUcmd->botCmds.becomeDriver = true;
		return false;
	}

	if ( botInfo->team != botWorld->botGoalInfo.attackingTeam || deployable.type != APT ) {
		if ( deployable.health < ( deployable.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) { //mal: its dead ( enough ).
			Bot_ExitVehicleAINode( true );
			botUcmd->botCmds.becomeDriver = true;
			return false;
		}
	}

	bool isVisible = true;
	idVec3 vec = deployable.origin - botInfo->origin;
	float distSqr = vec.LengthSqr();
	int minimumSightTime = ( botVehicleInfo->type == GOLIATH ) ? 50 : 5;

	if ( botVehicleInfo->type == HOG ) {
		isVisible = false;

		if ( distSqr < Square( 1500.0f ) ) { //mal: if we're close, lets ram it!
			botUcmd->actionEntityNum = deployable.entNum; //mal: dont avoid this entity!
			botUcmd->actionEntitySpawnID = deployable.spawnID;
		}
	} else if ( distSqr > Square( attackDist ) ) {
		isVisible = false;
		vLTGVisTimer = 0;
	} else {
		trace_t tr;
		idVec3 botOrigin = botInfo->viewOrigin;
		idVec3 vec = deployable.origin;
		vec.z += ( deployable.bbox[ 1 ][ 2 ] - deployable.bbox[ 0 ][ 2 ] ) * Bot_GetDeployableOffSet( deployable.type );
		if ( botVehicleInfo->canRotateInPlace && vLTGReached ) {
			botOrigin = botInfo->proxyInfo.weaponOrigin;
		} else if ( botVehicleInfo->type == TROJAN ) {
			botOrigin = botVehicleInfo->origin;
			botOrigin.z += ( botVehicleInfo->bbox[ 1 ][ 2 ] - botVehicleInfo->bbox[ 0 ][ 2 ] ) * 0.90f;
		}			

		vLTGReached = !vLTGReached;

		botThreadData.clip->TracePointExt( CLIP_DEBUG_PARMS tr, botOrigin, vec, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ), GetGameEntity( botVehicleInfo->entNum ) );

		if ( botThreadData.AllowDebugData() ) {
			gameRenderWorld->DebugLine( colorLtBlue, botOrigin, vec );
		}

		if ( tr.fraction < 1.0f && tr.c.entityNum != deployable.entNum ) {
			isVisible = false;
			vLTGVisTimer = 0;
		} else {
			vLTGVisTimer++;
		}
	}

	if ( !isVisible || ( botVehicleInfo->canRotateInPlace && vLTGVisTimer < minimumSightTime ) || vLTGDriveTime > botWorld->gameLocalInfo.time ) {
		if ( botVehicleInfo->driverEntNum != botNum ) { //mal: if someone moved us out of our seat, either take over the vehicle, or leave.
			vLTGMoveTimer++;

			if ( vLTGMoveTimer < 300 ) { //mal: someones blocking us, wait for a bit to have them clear out of the way, else just leave this node.
				if ( botVehicleInfo->driverEntNum == -1 ) {
					botUcmd->botCmds.becomeDriver = true;
				} else {
					Bot_ExitVehicleAINode( true );
					Bot_ExitVehicle();
					return false;
				}
			} else {
				Bot_ExitVehicleAINode( true );
				Bot_ExitVehicle();
				botUcmd->botCmds.becomeDriver = true;
				return false;
			}
		}

		vLTGSightTime = 0;

		idVec3 moveGoal = deployable.origin;
		moveGoal.z += DEPLOYABLE_ORIGIN_OFFSET;

		if ( botVehicleInfo->type == GOLIATH && distSqr > Square( 512.0f ) ) {
			botUcmd->actionEntityNum = deployable.entNum;
			botUcmd->actionEntitySpawnID = deployable.spawnID;
		}

		bool canRamDeployable = false;

		if ( botVehicleInfo->type == HOG ) {
			canRamDeployable = botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, botInfo->areaNumVehicle, botVehicleInfo->origin, moveGoal );
		}

		if ( botVehicleInfo->type == HOG && canRamDeployable && InFrontOfVehicle( botVehicleInfo->entNum, moveGoal, true ) ) {
			idBox deployableBox = idBox( deployable.bbox, deployable.origin, deployable.axis );
			idBox botBox = idBox( botVehicleInfo->bbox, botVehicleInfo->origin, botVehicleInfo->axis );

			if ( !deployableBox.IntersectsBox( botBox ) ) {
				Bot_MoveToGoal( deployableBox.GetCenter(), vec3_zero, SPRINT_ATTACK, NULLMOVETYPE ); //mal: Prepare for ramming speed! Ram them until they're dead, or we are....
				Bot_LookAtLocation( deployableBox.GetCenter(), SMOOTH_TURN );
			} else {
				vLTGTimer2++;
				if ( vLTGTimer2 < 50 ) {
					Bot_MoveToGoal( deployableBox.GetCenter(), vec3_zero, SPRINT_ATTACK, NULLMOVETYPE ); //mal: Prepare for ramming speed! Ram them until they're dead, or we are....
					Bot_LookAtLocation( deployableBox.GetCenter(), SMOOTH_TURN );
				} else {
					Bot_IgnoreDeployable( deployable.entNum, 10000 ); //mal: no valid path to this action for some reason - ignore it for a while
					Bot_ExitVehicleAINode( true );
					return false;
				}
			}
		} else {
			Bot_SetupVehicleMove( moveGoal, -1, ACTION_NULL, false );

			if ( MoveIsInvalid() ) {
				Bot_IgnoreDeployable( deployable.entNum, 15000 ); //mal: no valid path to this action for some reason - ignore it for a while
				Bot_ExitVehicleAINode( true );
				botUcmd->botCmds.becomeDriver = true;
				return false;
			}

			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, vehicleMove, NULLMOVETYPE );
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
			return true;
		}
	}

	if ( vLTGSightTime == 0 ) {
		vLTGSightTime = botWorld->gameLocalInfo.time;
	}

	if ( vLTGSightTime + 5000 < botWorld->gameLocalInfo.time && ( botInfo->lastAttackedEntity != deployable.entNum || botInfo->lastAttackedEntityTime + 5000 < botWorld->gameLocalInfo.time ) ) {
		vLTGDriveTime = botWorld->gameLocalInfo.time + 2000;
	}

	switch( botVehicleInfo->type ) {
		case TITAN:
			if ( botInfo->proxyInfo.weapon == MINIGUN ) {
				if ( botVehicleInfo->driverEntNum == -1 ) {
					botUcmd->botCmds.becomeDriver = true;
				} else {
					Bot_ExitVehicleAINode( true );
					Bot_ExitVehicle();
					return false;
				}
			}

			break;

		case TROJAN:
			if ( botVehicleInfo->driverEntNum == botNum ) {
				if ( VehicleHasGunnerSeatOpen( botVehicleInfo->entNum ) ) {
					botUcmd->botCmds.becomeGunner = true;
				} else {
					Bot_ExitVehicleAINode( true );
					Bot_ExitVehicle();
					return false;
				}
			}

			if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON ) {
				if ( VehicleHasGunnerSeatOpen( botVehicleInfo->entNum ) ) {
					botUcmd->botCmds.becomeGunner = true;
				} else {
					Bot_ExitVehicleAINode( true );
					Bot_ExitVehicle();
					return false;
				}
			}

			break;

		case DESECRATOR:
			if ( botInfo->proxyInfo.weapon == MINIGUN ) {
				if ( botVehicleInfo->driverEntNum == -1 ) {
					botUcmd->botCmds.becomeDriver = true;
				} else {
					Bot_ExitVehicleAINode( true );
					Bot_ExitVehicle();
					return false;
				}
			}

			break;
	}

	vec = deployable.origin;
	vec.z += ( deployable.bbox[ 1 ][ 2 ] - deployable.bbox[ 0 ][ 2 ] ) * Bot_GetDeployableOffSet( deployable.type );

	Bot_LookAtLocation( vec, SMOOTH_TURN );

	if ( botVehicleInfo->type == TITAN ) {
		botUcmd->botCmds.useTankAim = true;
	}

	if ( vLTGTimer == 0 ) {
		vLTGTimer = botWorld->gameLocalInfo.time + ( ( botVehicleInfo->canRotateInPlace && botWorld->gameLocalInfo.botAimSkill > 0 ) ? 1100 : 500 ); //mal: low skill tanks do snap shots, that will usually miss.
	}

	if ( vLTGTimer < botWorld->gameLocalInfo.time ) {
		botUcmd->botCmds.attack = true;
	}

	return true;
}

/*
================
idBotAI::Enter_VLTG_AircraftDestroyDeployable
================
*/
bool idBotAI::Enter_VLTG_AircraftDestroyDeployable() {

	vLTGType = V_DESTROY_DEPLOYABLE;

	vLTGTimer = 0;

	vLTGMoveTimer = 0;

	vLTGTime = botWorld->gameLocalInfo.time + 120000;

	rocketTime = 0;

	vLTGMoveActionGoal = ACTION_NULL;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_AircraftDestroyDeployable;

	if ( botInfo->proxyInfo.weapon != LAW ) { //mal: always make sure to have LAW ready to fire.
		botUcmd->botCmds.switchVehicleWeap = true;
	}

	vLTGDistFoundDeployable = 0.0f;

	vLTGUseAltAttackPointOnDeployable = false;

	vLTGAttackDeployableCounter = 0;

	vLTGKeepMovingTime = 0;

	lastAINode = "Destroy Deployable";

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::VLTG_AircraftDestroyDeployable
================
*/
bool idBotAI::VLTG_AircraftDestroyDeployable() {
	if ( vLTGTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitVehicleAINode( true );
		return false;
	}

	deployableInfo_t deployable;

	if ( !GetDeployableInfo( false, vLTGTarget, deployable ) ) { //mal: doesn't exist anymore.
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( botInfo->team != botWorld->botGoalInfo.attackingTeam || deployable.type != APT ) { //mal: when on the attack - destroy APTS - since they are the biggest threat to our mates.
		if ( deployable.health < ( deployable.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) { //mal: its dead ( enough ).
			Bot_ExitVehicleAINode( true );
			return false;
		}
	}

	bool isVisible = true;
	idVec3 vec = deployable.origin - botInfo->origin;

	if ( botVehicleInfo->type == ANANSI ) {
		vec.z = 0.0f;
	}

	float distSqr = vec.LengthSqr();
	idVec3 deployableOrigin = deployable.origin;
	deployableOrigin.z += DEPLOYABLE_ORIGIN_OFFSET;

	if ( distSqr > Square( WEAPON_LOCK_DIST ) ) {
		isVisible = false;
	} else {
		trace_t tr;
		botThreadData.clip->TracePointExt( CLIP_DEBUG_PARMS tr, botVehicleInfo->origin, deployableOrigin, MASK_SHOT_BOUNDINGBOX | MASK_VEHICLESOLID | CONTENTS_FORCEFIELD, GetGameEntity( botNum ), GetGameEntity( botVehicleInfo->entNum ) );

		if ( tr.fraction < 1.0f && tr.c.entityNum != deployable.entNum ) {
			isVisible = false;
		}
	}

	if ( !isVisible ) {
		if ( botVehicleInfo->driverEntNum != botNum ) { //mal: if someone moved us out of our seat, either take over the vehicle, or leave.
			if ( botVehicleInfo->driverEntNum == -1 ) {
				botUcmd->botCmds.becomeDriver = true;
			} else {
				Bot_ExitVehicleAINode( true );
				Bot_ExitVehicle();
				return false;
			}
		}

		Bot_SetupVehicleMove( deployable.origin, -1, ACTION_NULL );

		vLTGDistFoundDeployable = 0.0f;

		if ( MoveIsInvalid() ) {
			Bot_IgnoreDeployable( deployable.entNum, 5000 ); //mal: no valid path to this deployable for some reason - ignore it for a while
			Bot_ExitVehicleAINode( true );
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	bool overRideLook = false;
	float desiredRange = WEAPON_LOCK_DIST;
	float tooCloseRange = ( botVehicleInfo->type == ANANSI ) ? 3500.0f : 4500.0f;

	if ( vLTGDistFoundDeployable == 0.0f ) {
		vLTGDistFoundDeployable = distSqr;
	}

	if ( vLTGDistFoundDeployable <= Square( tooCloseRange ) && vLTGMoveTime < botWorld->gameLocalInfo.time ) { //mal: deployable could be hidden by buildings ( like on Ark ), so give us a chance to take a few shots when find it.
		vLTGAttackDeployableCounter++;
	}

	if ( vLTGAttackDeployableCounter > 1 ) { //mal: we've tried 2 times to hit this deployable, so now try something different.
		vLTGUseAltAttackPointOnDeployable = true;
	}

//mal: pick whats our best weapon
	if ( botInfo->proxyInfo.weapon == LAW ) { 
		if ( botInfo->proxyInfo.weaponIsReady == false && rocketTime < botWorld->gameLocalInfo.time && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: use rockets if LAW outta charge
			rocketTime = botWorld->gameLocalInfo.time + 2000;
			botUcmd->botCmds.switchVehicleWeap = true;
		}
	} else {
		if ( rocketTime < botWorld->gameLocalInfo.time ) {
			botUcmd->botCmds.switchVehicleWeap = true;
		}
	}

//mal: now use the weapon we picked.
	if ( botInfo->proxyInfo.weapon == LAW ) {
		if ( botInfo->targetLocked ) {
			botUcmd->botCmds.attack = true;
		}
	} else {
		idVec3 dir = deployable.origin - botVehicleInfo->origin;
        dir.NormalizeFast(); 
		if ( dir * botVehicleInfo->axis[ 0 ] > 0.95f ) { //mal: we have them in our sights - FIRE!
			botUcmd->botCmds.attack = true;
		}
	}

//mal: we're too close, so manuever around and fire.
	if ( distSqr < Square( tooCloseRange ) && vLTGMoveTime < botWorld->gameLocalInfo.time ) {
		int actionNumber = ACTION_NULL;

		if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
			actionNumber = Bot_FindNearbySafeActionToMoveToward( botInfo->origin, desiredRange, vLTGUseAltAttackPointOnDeployable );
		}

		if ( actionNumber != ACTION_NULL ) {
			vLTGMoveActionGoal = actionNumber;
			vLTGMoveTime = botWorld->gameLocalInfo.time + 10000;
			vLTGMoveTooCloseRange = tooCloseRange + ( ( botVehicleInfo->type == ANANSI ) ? 4500.0f : 3000.0f );
		} else {
			int n = botThreadData.random.RandomInt( 3 );

			if ( n == 0 ) {
				vLTGMoveDir = BACK;
			} else if ( n == 1 ) {
				vLTGMoveDir = RIGHT;
			} else {
				vLTGMoveDir = LEFT;
			}

			vLTGMoveTime = botWorld->gameLocalInfo.time + 5000;
			vLTGMoveTooCloseRange = tooCloseRange;
		}
	}

	if ( vLTGMoveTime > botWorld->gameLocalInfo.time ) {
		if ( distSqr > Square( vLTGMoveTooCloseRange ) ) { //mal: we're far enough away to get a shot - attack.
			vLTGMoveTime = 0;
			vLTGMoveActionGoal = ACTION_NULL;
			overRideLook = true;
		}
	}

	if ( vLTGMoveTime > 0 ) {
		if ( vLTGMoveActionGoal != ACTION_NULL ) {
			Bot_SetupVehicleMove( vec3_zero, -1, vLTGMoveActionGoal );
			if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO && botThreadData.random.RandomInt( 100 ) > 95 ) {
				botUcmd->botCmds.launchDecoysNow = true; //mal: we're exposing our flank, so fire some decoys to cover our move.
			}
		} else {
			vec = deployableOrigin;

			if ( vLTGMoveDir == BACK ) {
				vec += ( -tooCloseRange * botVehicleInfo->axis[ 0 ] );
			} else if ( vLTGMoveDir == RIGHT ) {
				vec += ( tooCloseRange * ( botVehicleInfo->axis[ 1 ] * -1 ) );
			} else if ( vLTGMoveDir == LEFT ) {
				vec += ( -tooCloseRange * ( botVehicleInfo->axis[ 1 ] * -1 ) );
			}

			if ( botVehicleInfo->type != ANANSI ) {
				vec.z = botVehicleInfo->origin.z;
			}

			aasTrace_t trace;
			botAAS.aas->Trace( trace, botInfo->aasVehicleOrigin, vec );

			if ( trace.fraction != 1.0f ) {
				vLTGMoveTime = 0;
				vLTGMoveActionGoal = ACTION_NULL;
				return true;
			}

			Bot_SetupVehicleMove( vec, -1, ACTION_NULL );
		}

		if ( MoveIsInvalid() ) {
			vLTGMoveTime = 0;
			vLTGMoveActionGoal = ACTION_NULL;
			return true;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	if ( distSqr > Square( desiredRange ) )  {
		Bot_SetupVehicleMove( deployableOrigin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreDeployable( deployable.entNum, 5000 ); //mal: no valid path for some reason - ignore for a while
			Bot_ResetEnemy();
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );

		if ( InFrontOfVehicle( botVehicleInfo->entNum, deployableOrigin ) || overRideLook ) {
			Bot_LookAtEntity( deployable.entNum, SMOOTH_TURN );
		} else {
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		}
	} else {
		Bot_SetupVehicleMove( deployableOrigin, -1, ACTION_NULL ); //mal: still want to take into account obstacles, so do a move check.

		if ( MoveIsInvalid() ) {
			Bot_IgnoreDeployable( deployable.entNum, 5000 ); //mal: no valid path for some reason - ignore for a while
			Bot_ResetEnemy();
			return false;
		}

		botMoveTypes_t defaultMoveType = AIR_BRAKE;

		if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY && Bot_VehicleIsUnderAVTAttack() != -1 || Bot_CheckIfEnemyHasUsInTheirSightsWhenInAirVehicle() || Bot_CheckEnemyHasLockOn( -1, true ) || vLTGKeepMovingTime > botWorld->gameLocalInfo.time ) { //mal: do a bombing run if someone is shooting at us!
			defaultMoveType = NULLMOVETYPE;

			if ( vLTGKeepMovingTime < botWorld->gameLocalInfo.time ) {
				vLTGKeepMovingTime = botWorld->gameLocalInfo.time + FLYER_AVOID_DANGER_TIME;
			}
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, ( botAAS.obstacleNum == -1 ) ? defaultMoveType : NULLMOVETYPE );
		Bot_LookAtEntity( deployable.entNum, SMOOTH_TURN );
	}

	return true;
}

/*
================
idBotAI::Enter_VLTG_TravelToGoalOrigin

The bot is in a vehicle, and wants to move to a origin on the map. Only used currently for special case scripting.
================
*/
bool idBotAI::Enter_VLTG_TravelToGoalOrigin() {
	vLTGType = V_ORIGIN_GOAL;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_TravelToGoalOrigin;

	vLTGTime = botWorld->gameLocalInfo.time + 60000;

	lastAINode = "Vehicle Origin Goal";

	vLTGDeployableTargetAttackTime = 0;
	
	vLTGDeployableTarget = -1;

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::VLTG_TravelToGoalOrigin
================
*/
bool idBotAI::VLTG_TravelToGoalOrigin() {
  	float dist;
	idVec3 vec;

	if ( vLTGTime < botWorld->gameLocalInfo.time ) {
		Bot_ExitVehicleAINode( true );
		
		if ( botWorld->gameLocalInfo.gameMap == SLIPGATE ) { 
			Bot_ExitVehicle(); //mal: if we can't reach slipgate - need to leave vehicle
		}
		
		return false;
	}

	if ( botWorld->gameLocalInfo.gameMap == SLIPGATE ) { //mal: special case for slipgate. See if we got to the other side of the slipgate.
		if ( botVehicleInfo->origin.x > SLIPGATE_DIVIDING_PLANE_X_VALUE ) {
			Bot_ExitVehicleAINode( true );
			return false;
		}
	}

	if ( botVehicleInfo->driverEntNum != botNum ) { //mal: if someone moved us out of our seat, either take over the vehicle, or just hang out. We're just looking for trouble anyhow.
		if ( botVehicleInfo->driverEntNum == -1 ) {
			botUcmd->botCmds.becomeDriver = true;
		} else if ( VehicleHasGunnerSeatOpen( botInfo->proxyInfo.entNum ) ) {
			botUcmd->botCmds.becomeGunner = true;
		} else {
			if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON ) {
				Bot_ExitVehicleAINode( true );
				Bot_ExitVehicle();
			} else {
                int deployableTargetToKill = Bot_CheckForDeployableTargetsWhileVehicleGunner();

				if ( deployableTargetToKill != -1 ) {
					Bot_AttackDeployableTargetsWhileVehicleGunner( deployableTargetToKill );
				} else {
					vLTGDeployableTargetAttackTime = 0;
					vLTGDeployableTarget = -1;
					if ( botThreadData.random.RandomInt( 100 ) > RANDOMLY_LOOK_AROUND_WHILE_VEHICLE_GUNNER_CHANCE ) {
						idVec3 vec;
						if ( Bot_RandomLook( vec ) )  {
							Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot. If we're not in a vehicle that lets us look around, this will be pointless.
						}
					}
				}
			}
		}
		return true;
	}

	vec = vLTGOrigin - botInfo->origin;
	dist = vec.LengthSqr();

	float closeEnoughDist = ( vLTGTargetType == MCP_GOAL ) ? 750.0f : GOAL_ORIGIN_DIST;
	botMoveTypes_t botMoveType = ( vLTGTargetType == MCP_GOAL ) ? NULLMOVETYPE : IGNORE_ALTITUDE;

	if ( dist > Square( closeEnoughDist ) ) {

		Bot_SetupVehicleMove( vLTGOrigin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_ExitVehicleAINode( true );
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, ( dist > Square( 500.0f ) ) ? SPRINT : RUN, botMoveType );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	Bot_ExitVehicleAINode( true ); //mal: reached our roam goal, so just leave.

	if ( vLTGTargetType == MCP_GOAL ) {
		Bot_ExitVehicle();
	}

	return true;
}

/*
================
idBotAI::Enter_VLTG_DriveMCPToRouteGoal

The bot is in the MCP, taking it to a route action goal. 
================
*/
bool idBotAI::Enter_VLTG_DriveMCPToRouteGoal() {

	vLTGType = V_DRIVE_MCP_ROUTE;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_DriveMCPToRouteGoal;

	lastAINode = "MCP Route Goal";

	vLTGDeployableTargetAttackTime = 0;
	
	vLTGDeployableTarget = -1;

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::VLTG_DriveMCPToRouteGoal
================
*/
bool idBotAI::VLTG_DriveMCPToRouteGoal() {
	idVec3 vec;

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off - shouldn't happen
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( botVehicleInfo->driverEntNum != botNum ) { //mal: if someone moved us out of our seat, either take over the vehicle, or just hang out. We're just looking for trouble anyhow.
		if ( botVehicleInfo->driverEntNum == -1 ) {
			botUcmd->botCmds.becomeDriver = true;
		} else {
            int deployableTargetToKill = Bot_CheckForDeployableTargetsWhileVehicleGunner();

			if ( deployableTargetToKill != -1 ) {
				Bot_AttackDeployableTargetsWhileVehicleGunner( deployableTargetToKill );
			} else {
				vLTGDeployableTargetAttackTime = 0;
				vLTGDeployableTarget = -1;
				if ( botThreadData.random.RandomInt( 100 ) > RANDOMLY_LOOK_AROUND_WHILE_VEHICLE_GUNNER_CHANCE ) {
					idVec3 vec;
					if ( Bot_RandomLook( vec ) )  {
						Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot. If we're not in a vehicle that lets us look around, this will be pointless.
					}
				}
			}
		}
		return true;
	}

	if ( botVehicleInfo->isImmobilized && !botVehicleInfo->isEMPed ) {
		Bot_ExitVehicleAINode( true );
		Bot_ExitVehicle( false );
		return false;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botVehicleInfo->origin;
	float distSqr = vec.LengthSqr();

	if ( distSqr > Square( botThreadData.botActions[ actionNum ]->GetRadius() ) ) {

		Bot_SetupVehicleMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) { //mal: this better NEVER happen! If so, something is really messed up!
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitVehicleAINode( true );
			assert( false );
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	Bot_ExitVehicleAINode( true ); //mal: reached the route goal! Now, lets move on to the MCP outpost.
	Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME );
	return true;
}

/*
================
idBotAI::Enter_VLTG_HuntGoal
================
*/
bool idBotAI::Enter_VLTG_HuntGoal() {

	vLTGType = V_HUNT_GOAL;

	V_LTG_AI_SUB_NODE = &idBotAI::VLTG_HuntGoal;

	vLTGTime = botWorld->gameLocalInfo.time + 120000;

	lastAINode = "Vehicle Hunt Goal";

	vLTGDeployableTargetAttackTime = 0;
	
	vLTGDeployableTarget = -1;

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::VLTG_HuntGoal
================
*/
bool idBotAI::VLTG_HuntGoal() {
  	if ( vLTGTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreClient( vLTGTarget, CLIENT_IGNORE_TIME );
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( !ClientIsValid( vLTGTarget, vLTGTargetSpawnID ) ){
		Bot_ExitVehicleAINode( true );
		return false;
	}

	if ( botVehicleInfo->driverEntNum != botNum ) { //mal: if someone moved us out of our seat, either take over the vehicle, or just hang out. We're just looking for trouble anyhow.
		if ( botVehicleInfo->driverEntNum == -1 ) {
			botUcmd->botCmds.becomeDriver = true;
		} else if ( VehicleHasGunnerSeatOpen( botInfo->proxyInfo.entNum ) ) {
			botUcmd->botCmds.becomeGunner = true;
		} else {
			if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON ) {
				Bot_ExitVehicleAINode( true );
				Bot_ExitVehicle();
			} else {
                int deployableTargetToKill = Bot_CheckForDeployableTargetsWhileVehicleGunner();

				if ( deployableTargetToKill != -1 ) {
					Bot_AttackDeployableTargetsWhileVehicleGunner( deployableTargetToKill );
				} else {
					vLTGDeployableTargetAttackTime = 0;
					vLTGDeployableTarget = -1;
					if ( botThreadData.random.RandomInt( 100 ) > RANDOMLY_LOOK_AROUND_WHILE_VEHICLE_GUNNER_CHANCE ) {
						idVec3 vec;
						if ( Bot_RandomLook( vec ) )  {
							Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot. If we're not in a vehicle that lets us look around, this will be pointless.
						}
					}
				}
			}
		}
		return true;
	}

	idVec3 vec = botWorld->clientInfo[ vLTGTarget ].origin - botInfo->origin;

	float distSqr = vec.LengthSqr();

	if ( distSqr > Square( 750.0f ) || ( botVehicleInfo->type < ICARUS && botVehiclePathList.Num() > 0 ) ) {

		Bot_SetupVehicleMove( botWorld->clientInfo[ vLTGTarget ].origin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreClient( vLTGTarget, CLIENT_IGNORE_TIME );
			Bot_ExitVehicleAINode( true );
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, ( distSqr > Square( 500.0f ) ) ? SPRINT : RUN, NULLMOVETYPE );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	Bot_IgnoreClient( vLTGTarget, CLIENT_IGNORE_TIME );
	Bot_ExitVehicleAINode( true ); //mal: reached our roam goal, so just leave.
	return true;
}