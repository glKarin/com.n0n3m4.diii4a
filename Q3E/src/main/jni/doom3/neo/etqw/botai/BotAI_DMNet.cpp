// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idBotAI::Run_LTG_Node

This is the bot's long term goal thinking. 
================
*/
bool idBotAI::Run_LTG_Node() {

//mal: first, lets check if we should be running a different node....
	if ( botWorld->gameLocalInfo.inWarmup && botWorld->gameLocalInfo.botsSillyWarmup != false ) {
		 ROOT_AI_NODE = &idBotAI::Run_Warmup_Node;
		 return false;
	}

	if ( botWorld->gameLocalInfo.inEndGame ) {
		 ROOT_AI_NODE = &idBotAI::Run_Intermission_Node;
		 return false;
	}

	if ( ClientIsDead( botNum ) ) {
		 ROOT_AI_NODE = &idBotAI::Enter_Run_Dead_Node;
		 return false;
	}

	if ( timeTilAttackEnemy != -1 && timeTilAttackEnemy < botWorld->gameLocalInfo.time && enemy != -1 ) {
		Bot_SetTimeOnTarget( false );
		ROOT_AI_NODE = &idBotAI::Run_Combat_Node;
		return false;
	}

	if ( LTG_AI_SUB_NODE == NULL ) {
		if ( !PopAINodeOffStack() ) {
            Bot_FindLongTermGoal();
			assert( LTG_AI_SUB_NODE != NULL );
		}
	}

	aiState = LTG;

	CallFuncPtr( LTG_AI_SUB_NODE );		//mal: run the bot's current Long Term Goal think node

	Bot_Check_NBG_Goals();

	Bot_CheckWeapon();

	Bot_CheckClassState(); //mal: check if we should do some kind of class based action on the move!

	Bot_CheckForDangers( false );

	if ( enemy <= -1 ) {
        Bot_FindEnemy( -1 );
	}

	return true;
}


/*
================
idBotAI::Run_NBG_Node

This is the bot's short term goal thinking. 
================
*/
bool idBotAI::Run_NBG_Node() {

//mal: first, lets check if we should be running a different node....
	if ( botWorld->gameLocalInfo.inWarmup && botWorld->gameLocalInfo.botsSillyWarmup != false ) {
		 ROOT_AI_NODE = &idBotAI::Run_Warmup_Node;
		 return false;
	}

	if ( botWorld->gameLocalInfo.inEndGame ) {
		 ROOT_AI_NODE = &idBotAI::Run_Intermission_Node;
		 return false;
	}

	if ( ClientIsDead( botNum ) ) {
		 ROOT_AI_NODE = &idBotAI::Enter_Run_Dead_Node;
		 return false;
	}

	if ( timeTilAttackEnemy != -1 && timeTilAttackEnemy < botWorld->gameLocalInfo.time && enemy != -1 ) {
		ROOT_AI_NODE = &idBotAI::Run_Combat_Node;
		return false;
	}

	if ( NBG_AI_SUB_NODE == false ) { //mal: if we have no NBG sub node, we're done here and need to reset and find a LTG next frame
		Bot_ResetState( false, false ); 
		return false;
	}

	aiState = NBG;

	Bot_CheckClassState(); //mal: we run this early in NBG, so if medic is reviving, he'll switch weap as needed

	CallFuncPtr( NBG_AI_SUB_NODE ); //mal: run the bot's current short term goal think node

	Bot_Check_NBG_Goals();

	Bot_CheckForDangers( false );

	if ( enemy <= -1 ) {
        Bot_FindEnemy( -1 );
	}

	return true;
}


/*
================
idBotAI::Run_Combat_Node

This is the bot's combat thinking. 
================
*/
bool idBotAI::Run_Combat_Node() {

	bool needNewEnemy = false;
	bool haveClientToHaze = false;

//mal: first, lets check if we should be running a different node....
	if ( botWorld->gameLocalInfo.inWarmup && botWorld->gameLocalInfo.botsSillyWarmup != false ) {
		 ROOT_AI_NODE = &idBotAI::Run_Warmup_Node;
		 return false;
	}

	if ( botWorld->gameLocalInfo.inEndGame ) {
		 ROOT_AI_NODE = &idBotAI::Run_Intermission_Node;
		 return false;
	}

	if ( ClientIsDead( botNum ) ) {
		 ROOT_AI_NODE = &idBotAI::Enter_Run_Dead_Node;
		 return false;
	}

	if ( botInfo->isDisguised ) {
		botUcmd->botCmds.dropDisguise = true;
		botIdealWeapSlot = RESET_WEAPON;
		return true; //mal: wait til we drop uniform to attack
	}

	if ( !ClientIsValid( enemy, enemySpawnID ) ) { //mal: enemy disconnected/got kicked/ etc
		needNewEnemy = true;
	}

	if ( !EnemyValid() ) {
		needNewEnemy = true;
	}

	if ( ClientIsDead( enemy ) ) {
		needNewEnemy = true;
		haveClientToHaze = true;
	}

	if ( resetEnemy ) {
		Bot_ResetEnemy();
		needNewEnemy = true;
	}

	if ( needNewEnemy ) {
		if ( !Bot_FindEnemy( enemy ) ) {
			if ( haveClientToHaze ) {
                if ( !Bot_PickPostCombatGoal() ) {
					Bot_ResetState( true, false ); 
				}
			} else {
				Bot_ResetState( true, false );
			}
		}
        return false; //mal: no matter what we decide, leave this node for a frame.
	}

	int vehicleNum = botWorld->clientInfo[ enemy ].proxyInfo.entNum;
	needNewEnemy = false;

	if ( vehicleNum != CLIENT_HAS_NO_VEHICLE ) { //mal: dont fight pointless battles with vehicles if we're outgunned.
		proxyInfo_t vehicleInfo;
		GetVehicleInfo( vehicleNum, vehicleInfo );

		if ( vehicleInfo.type == MCP && vehicleInfo.isImmobilized && vehicleInfo.driverEntNum == enemy ) {
			needNewEnemy = true;
		} else if ( ( vehicleInfo.type == GOLIATH || vehicleInfo.type == TITAN || vehicleInfo.type == DESECRATOR ) && vehicleInfo.health > ( vehicleInfo.maxHealth / 3 ) ) {
			
			bool canNadeAttack = ( !botInfo->weapInfo.hasNadeAmmo || enemyInfo.enemyDist > GRENADE_ATTACK_DIST ) ? false : true;

			if ( botInfo->classType == FIELDOPS ) {
				if ( enemyInfo.enemyDist > GRENADE_ATTACK_DIST ) {
					if ( !Bot_HasWorkingDeployable() || !ClassWeaponCharged( AIRCAN ) || Bot_EnemyAITInArea( vehicleInfo.origin ) ) {
						needNewEnemy = true;
					}
				} else {
					if ( !canNadeAttack && !ClassWeaponCharged( AIRCAN ) && !botInfo->weapInfo.primaryWeapHasAmmo ) {
						needNewEnemy = true;
					}
				}
			} else if ( botInfo->classType == COVERTOPS ) {
				if ( !canNadeAttack && ( botInfo->weapInfo.primaryWeapon != SNIPERRIFLE || !botInfo->weapInfo.primaryWeapHasAmmo ) ) {
					needNewEnemy = true;
				}
			} else if ( botInfo->classType == SOLDIER ) {
				if ( !canNadeAttack && ( botInfo->weapInfo.primaryWeapon != ROCKET || !botInfo->weapInfo.primaryWeapHasAmmo ) ) {
					needNewEnemy = true;
				}
			} else {
				if ( ( !canNadeAttack && enemyInfo.enemyDist > INFANTRY_ATTACK_HEAVY_DIST ) && !botInfo->weapInfo.primaryWeapHasAmmo ) {		
					needNewEnemy = true;
				}
			}
		}
	}

	if ( needNewEnemy ) {
		if ( !Bot_FindEnemy( enemy ) ) {
			Bot_ResetState( true, false ); 
			return false;
		}
	}

	aiState = COMBAT;

	UpdateEnemyInfo();

	if ( !enemyInfo.enemyVisible && enemyInfo.enemyLastVisTime + 5000 < botWorld->gameLocalInfo.time && !chasingEnemy ) {
		if ( !Bot_FindEnemy( enemy ) ) {
            Bot_ResetState( true, false ); 
		}
		return false;
	}

	Bot_CheckForDangers( true );

	if ( turretDangerExists && turretDangerEntNum != -1 && combatNBGType != COMBAT_ATTACK_TURRET && enemyInfo.enemyDist > 500.0f && Bot_HasExplosives( true, true ) && !Client_IsCriticalForCurrentObj( botNum, BOT_WILL_ATTACK_TURRETS_IN_COMBAT_DIST ) ) {
		combatNBGTarget = turretDangerEntNum;
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackTurret; 
	}

	if ( COMBAT_AI_SUB_NODE == NULL ) {
        Bot_CheckCurrentStateForCombat();
	}

	CallFuncPtr( COMBAT_AI_SUB_NODE ); //mal: run the bot's current combat think node

	Bot_FindBetterEnemy(); //mal: the last thing to run

	return true;
}

/*
================
idBotAI::Run_Intermission_Node

This node is run at game end. Bots jeer/cheer based on who won. can also just say silly things for fun.
================
*/
bool idBotAI::Run_Intermission_Node() {

	int randChance = ( botWorld->gameLocalInfo.numClients > 12 ) ? 97 : 70;

	if ( !botWorld->gameLocalInfo.inEndGame ) {
        Bot_ResetState( true, true );
		return false;
	}

	if ( botWorld->gameLocalInfo.winningTeam == NOTEAM ) { //mal: may take a bit to get updated. Ignore til its ready.
		return true;
	}

	if ( botThreadData.random.RandomInt( 100 ) < randChance ) {
		return true;
	}

	if ( botThreadData.random.RandomInt( 100 ) > 98 ) {
		if ( botWorld->gameLocalInfo.winningTeam == botInfo->team ) {
			Bot_AddDelayedChat( botNum, ENDGAME_WIN, 1 ); //I'm a winnah!!!1
		} else {
			Bot_AddDelayedChat( botNum, ENDGAME_LOSE, 1 ); //OMG H4X!!1
		}
	}

	return true;
}

/*
================
idBotAI::Run_Warmup_Node

This is run while the game is in warmup mode. In warmup, the bots just
screw around and shoot each other, do other wacky stuff.
================
*/
bool idBotAI::Run_Warmup_Node() {

	float attackDist = 3500.0f; //mal: the range of the bot's "awareness" during warmup
	float dist;
	idVec3	vec;

	if ( !botWorld->gameLocalInfo.inWarmup || botWorld->gameLocalInfo.botsSillyWarmup == false ) {
		Bot_ResetState( true, true );
        return false;
     }

	if ( botWorld->gameLocalInfo.inEndGame ) {
		 ROOT_AI_NODE = &idBotAI::Run_Intermission_Node;
		 return false;
	}

	if ( enemy == -1 ) { //mal: if we have no enemy, lets look for one! NOTE: if a bot is killed, he keeps his enemy, to get revenge!

        int i, j;

		if ( !botInfo->hasGroundContact && !botInfo->hasJumped && botInfo->invulnerableEndTime > botWorld->gameLocalInfo.time ) {
			return false; //mal: if we're dropping in (and not jumping around) dont pick targets yet (just spawned).
		}

//mal: don't ALWAYS pick on the human! So lets just pick a random client and fight them.
		if ( botThreadData.random.RandomInt( 100 ) > 75 ) {
			j = 0;
		} else {
			j = botThreadData.random.RandomInt( botWorld->gameLocalInfo.numClients );
		}

		COMBAT_MOVEMENT_STATE = NULL;

		for ( i = j; i < MAX_CLIENTS; i++ ) {

			if ( !ClientIsValid( i, -1 ) ) {
				continue; //mal: no valid client in this client slot!
			}

			if ( i == botNum ) { //mal: don't target yourself!
				continue;
			}

			if ( EnemyIsIgnored( i ) ) {
				continue; //mal: dont try to fight someone we've flagged to ignore for whatever reason!
			}

			const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

			if ( playerInfo.isNoTarget ) {
				continue;
			} //mal: dont target clients that have notarget set - this is useful for debugging, etc.

			if ( playerInfo.health <= 0 ) {
				continue;
			}

			if ( !playerInfo.hasGroundContact && !playerInfo.hasJumped && playerInfo.invulnerableEndTime > botWorld->gameLocalInfo.time ) {
				continue; //mal: dont shoot them when they're in the air!
			}

			vec = playerInfo.viewOrigin - botInfo->viewOrigin;
			
			dist = vec.LengthFast();

			if ( dist > attackDist ) { //mal: too far away - ignore!
				continue;
			}

			enemy = i; //mal: just pick the first guy we come across
			enemySpawnID = botWorld->clientInfo[ i ].spawnID;

			botIdealWeapNum = NULL_WEAP;

			if ( botThreadData.random.RandomInt( 100 ) > 50 || dist > 2500.0f ) { //mal: lets have a little fun - sometimes we'll try to knife our enemy! 
				botIdealWeapSlot = GUN;
			} else {
				if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
                    botIdealWeapSlot = MELEE;
				} else {
					if ( botInfo->weapInfo.hasNadeAmmo ) {
                        botIdealWeapSlot = NADE;
					} else {
						botIdealWeapSlot = MELEE;
					}
				}
			}

			warmupUseActionMoveGoalTime = 0;

			if ( botThreadData.random.RandomInt( 100 ) > 75 ) {
				warmupActionMoveGoal = Bot_FindNearbySafeActionToMoveToward( botInfo->origin, 1024.0f, true );

				if ( warmupActionMoveGoal != ACTION_NULL ) {
					warmupUseActionMoveGoalTime = botWorld->gameLocalInfo.time + WARMUP_ACTION_TIME;
				}
			}

			return true;
		}

		return true; //mal: dont do anything til next pass, whether found someone or not.
	}

	if ( warmupUseActionMoveGoalTime > botWorld->gameLocalInfo.time ) { //mal: don't always attack, sometimes move away from the spawn, and attack from a diff position
		
		Bot_SetupMove( vec3_zero, -1, warmupActionMoveGoal );

		if ( MoveIsInvalid() ) {
			warmupUseActionMoveGoalTime = 0;
			return true;
		}

		Bot_MoveAlongPath( SPRINT );

		if ( botInfo->backPedalTime > botWorld->gameLocalInfo.time ) { //mal: add a bit of variety...
			botUcmd->viewType = VIEW_REVERSE;
		}

		return true;
	}

	if ( !botWorld->clientInfo[ enemy ].inGame ) {
		enemy = -1; //mal: our enemy disconnected, got kicked, etc. get another one
		return true;
	}

	if ( ClientIsDead( enemy ) ) {
		enemy = -1; //mal: our enemy is dead, get another one!
		return true;
	}

	vec = botWorld->clientInfo[ enemy ].origin - botInfo->origin;

	dist = vec.LengthFast();

	if ( dist > attackDist ) { //mal: hes too far away (maybe flying), so forget him!
		enemy = -1;
		return true;
	}

	if ( botIdealWeapSlot == MELEE && dist > 2500.0f ) {
		botIdealWeapSlot = GUN;
	}

	if ( botThreadData.random.RandomInt( 100 ) > 98 && botThreadData.random.RandomInt( 100 ) > 50 ) {
        if ( botInfo->classType == FIELDOPS && botInfo->fireSupportChargedUsed == 0 ) {
			botIdealWeapNum = AIRCAN;
			botIdealWeapSlot = NO_WEAPON;
		}
	}

	UpdateEnemyInfo();

	if ( !ClientIsVisibleToBot( enemy, false, false ) ) {
		
		Bot_SetupMove(vec3_zero, enemy, ACTION_NULL );

		if ( MoveIsInvalid() ) { //mal: cant find a valid path - ignore this client for a while!
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME );
			Bot_ResetEnemy();
		}

		Bot_MoveAlongPath( SPRINT );
        return true;
	}

    COMBAT_AI_SUB_NODE = &idBotAI::COMBAT_Foot_AttackEnemy;

	if ( botIdealWeapSlot == MELEE ) {
		COMBAT_MOVEMENT_STATE = &idBotAI::Knife_Attack_Movement;
	}

	bool result = CallFuncPtr( COMBAT_AI_SUB_NODE ); //mal: run the bot's current combat think node

	if ( botInfo->weapInfo.weapon == AIRCAN || botInfo->weapInfo.weapon == GRENADE || botInfo->weapInfo.weapon == EMP ) {

		if ( !botInfo->weapInfo.hasNadeAmmo && ( botInfo->weapInfo.weapon == GRENADE || botInfo->weapInfo.weapon == EMP ) ) {
			botIdealWeapSlot = GUN;
			return false;
		}

		if ( botInfo->weapInfo.weapon == AIRCAN && botInfo->fireSupportChargedUsed > 0 ) {
			botIdealWeapSlot = GUN;
			return false;
		}

		combatMoveFailedCount = 0;
		
		COMBAT_MOVEMENT_STATE = &idBotAI::Crazy_Jump_Attack_Movement;

		if ( botThreadData.random.RandomInt( 100 ) > 95 ) {
			botUcmd->botCmds.attack = false;
		}
	}

	if ( result == false ) {
		Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME );
		Bot_ResetEnemy();
	}

	if ( botThreadData.random.RandomInt( 100 ) > 98 && botThreadData.random.RandomInt( 100 ) > 95 ) {
		botUcmd->desiredChat = WARMUP_TAUNT;
	}

	return true;
}

/*
================
idBotAI::Enter_Run_Dead_Node

This node is run if the bot is killed/dead/waiting for a medic. 
================
*/
bool idBotAI::Enter_Run_Dead_Node() {

	int attackerNum;

	attackerNum = botWorld->clientInfo[ botNum ].lastAttacker;

	Bot_ResetState( true, false ); //mal: clear out the bot's state, but keep any important goals on the stack in case he gets revived.

	memset( delayedChats, 0, sizeof( delayedChats ) );

	ROOT_AI_NODE = &idBotAI::Run_Dead_Node;

	if ( attackerNum > -1 && attackerNum < MAX_CLIENTS ) {
        if ( botWorld->clientInfo[ attackerNum ].team != botInfo->team ) {
			if ( botWorld->clientInfo[ attackerNum ].isDisguised ) {
				if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
					if ( botWorld->clientInfo[ attackerNum ].disguisedClient == botNum ) {
						Bot_AddDelayedChat( botNum, ENEMY_DISGUISED_AS_ME, 3 );
					} else {
						Bot_AddDelayedChat( botNum, ENEMY_DISGUISED, 3 );
					}
				}
			} else if ( !botWorld->clientInfo[ attackerNum ].isBot && botThreadData.random.RandomInt( 100 ) > 85 && botThreadData.random.RandomInt( 100 ) > 50 ) {
				Bot_AddDelayedChat( botNum, KILLED_TAUNT, 2 );
			}
		}
	}

	lastAINode = "Dead Think";

	return true;
}


/*
================
idBotAI::Run_Dead_Node

This node is run if the bot is killed/dead/waiting for a medic. 
================
*/
bool idBotAI::Run_Dead_Node() {

	int medicsInArea;

	if ( botWorld->gameLocalInfo.inWarmup && botWorld->gameLocalInfo.botsSillyWarmup != false ) {
		 ROOT_AI_NODE = &idBotAI::Run_Warmup_Node;
		 return false;
	}

	if ( botWorld->gameLocalInfo.inEndGame ) {
		 ROOT_AI_NODE = &idBotAI::Run_Intermission_Node;
		 return false;
	}

	if ( botInfo->health > 0 ) { 
		if ( botInfo->revived ) {
            Bot_ResetState( true, false ); //mal: got revived - get back into the action doing whatever we were doing before.
		} else {
			Bot_ResetState( true, true ); //mal: if respawned, wipe everything and start from scratch.
			lastActionNum = ACTION_NULL;
		}
		return false;
	}

	if ( botInfo->inLimbo ) { //mal: if we're in limbo, just exit out of here for now.
		return true;
	}

	botThreadData.GetBotOutputState()->botOutput[ botNum ].botCmds.hasMedicInFOV = Bot_CheckHasVisibleMedicNearby(); //mal: we'll use this later to decide if we should tap out, or wait.

	if ( botThreadData.random.RandomInt( 100 ) > 98 && botThreadData.random.RandomInt( 100 ) > 50 ) {

		medicsInArea = ClientsInArea( botNum, botInfo->origin, 700.0f, botInfo->team, MEDIC, false, false, false, false, true, true );

		if ( medicsInArea > 0 && botThreadData.random.RandomInt( 100 ) > 98 ) { //mal: no point in begging for a medic, if theres none around ( or they're all dead ).
            botUcmd->desiredChat = REVIVE_ME;
		}
	}

	return true;
}
