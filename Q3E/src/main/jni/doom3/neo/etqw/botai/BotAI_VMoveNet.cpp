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
idBotAI::Bot_FindBestVehicleCombatMovement

Finds the best combat movement for the bot while in a vehicle.
================
*/
bool idBotAI::Bot_FindBestVehicleCombatMovement() {
	bool vehicleCanRamClient; //mal: sounds a bit... dirty, doesn't it?
	bool vehicleGunnerSeatOpen;
	bool hasAttackedCriticalMate = false;
	bool enemyReachable = false;
	bool enemyHasVehicle = false;
	proxyInfo_t enemyVehicleInfo;

	const clientInfo_t& enemyPlayerInfo = botWorld->clientInfo[ enemy ];

	if ( enemyPlayerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		GetVehicleInfo( enemyPlayerInfo.proxyInfo.entNum, enemyVehicleInfo );
		enemyHasVehicle = true;
	}

	combatMoveFailedCount = 0;
	combatMoveTime = -1;

	if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON && botVehicleInfo->driverEntNum != botNum ) {
		return false;
	} //mal: this shouldn't really ever happen, but may if the bot is moving between positions in a vehicle. Or if a human entered/exited a vehicle at just the right/wrong time.

	if ( ( botInfo->proxyInfo.weapon == MINIGUN || botInfo->proxyInfo.weapon == LAW || botInfo->proxyInfo.weapon == PERSONAL_WEAPON ) && botVehicleInfo->driverEntNum != botNum ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_NULL_Movement; //mal: just sit there, and shoot the enemy.
		return true;
	}

	if ( botVehicleInfo->type > ICARUS ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Air_Movement;
		return true;
	}

	//mal: this more often then not causes issues
/*
	if ( enemyHasVehicle ) {
		if ( enemyVehicleInfo.type == HOG && enemyInfo.enemyDist < 3000.0f && botVehicleInfo->driverEntNum == botNum ) { //mal: never stand around if a hog has us in his sights.
			VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Random_Movement;
			return true;
		}
	}
*/

	vehicleGunnerSeatOpen = VehicleHasGunnerSeatOpen( botInfo->proxyInfo.entNum );

	if ( enemyPlayerInfo.lastAttackClient > - 1 && enemyPlayerInfo.lastAttackClient < MAX_CLIENTS ) {
		if ( botWorld->clientInfo[ enemyPlayerInfo.lastAttackClient ].team == botInfo->team && enemyPlayerInfo.lastAttackClientTime + 3000 > botWorld->gameLocalInfo.time ) {
			if ( Client_IsCriticalForCurrentObj( enemyPlayerInfo.lastAttackClient, 2000.0f ) || ClientHasObj( enemyPlayerInfo.lastAttackClient ) ) {
				hasAttackedCriticalMate = true;
			}
		}
	}

	Bot_SetupVehicleMove( vec3_zero, enemy, ACTION_NULL );

    if ( botAAS.hasPath && botAAS.path.travelTime < Bot_ApproxTravelTimeToLocation( botInfo->origin, enemyPlayerInfo.origin, true ) * TRAVEL_TIME_MULTIPLY ) {
		enemyReachable = true;
	}

	if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON && botVehicleInfo->driverEntNum == botNum ) {

		vehicleCanRamClient = Bot_VehicleCanRamClient( enemy );

		if ( botVehicleInfo->type == HOG ) {
			if ( vehicleCanRamClient && InFrontOfVehicle( botVehicleInfo->entNum, enemyPlayerInfo.origin, true ) ) {
				VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Ram_Attack_Movement; //mal: ram them! Thats our only weapon.
				return true;
			}

			if ( enemyReachable && enemyHasVehicle ) {
				if ( enemyVehicleInfo.type != HUSKY ) {
					VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Chase_Movement; //mal: move in for a closer kill, THEN ram them.
					return true;
				}
			}
		}

		if ( botVehicleInfo->type == BADGER && enemyInfo.enemyDist < 1500.0f && vehicleCanRamClient && enemyPlayerInfo.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE && botVehicleInfo->driverEntNum == botNum ) {
			VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Ram_Attack_Movement; //mal: 10 points! Fun for the whole family.
			return true;
		}

		if ( !vehicleGunnerSeatOpen ) {
			if ( enemyInfo.enemyDist < 2500.0f ) {
				VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Stand_Ground_Movement; //mal: just sit there, let our gunner get a chance to kill the enemy.
			} else {
				if ( enemyReachable || enemyInfo.enemyDist > 1500.0f ) {
                    VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Chase_Movement; //mal: move in for a closer kill.
				} else {
					VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Stand_Ground_Movement; //mal: just sit there, let our gunner get a chance to kill the enemy.
				}
			}

			return true;
		} else {
			if ( enemyPlayerInfo.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {
				if ( enemyInfo.enemyDist < 6000.0f && ( vehicleCanRamClient || hasAttackedCriticalMate ) ) { //mal: if an important mate is under attack, we'll fight to the end, even if its just to draw the enemies fire.
					VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Ram_Attack_Movement; //mal: ram them! Thats our only weapon.
					return true;
				}
			} else {
				Bot_IgnoreEnemy( enemy, 15000 );
				return false;
			} //mal: if all else fails, we have no choice but to ignore this enemy.
		}
	}

	if ( ( botInfo->proxyInfo.weapon == MINIGUN || botInfo->proxyInfo.weapon == LAW || botInfo->proxyInfo.weapon == PERSONAL_WEAPON ) && botVehicleInfo->driverEntNum != botNum ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_NULL_Movement; //mal: just sit there, and shoot the enemy.
		return true;
	}

	if ( ( botInfo->proxyInfo.weapon == MINIGUN || botInfo->proxyInfo.weapon == LAW || botInfo->proxyInfo.weapon == PERSONAL_WEAPON ) && botVehicleInfo->driverEntNum == botNum ) {

		if ( botThreadData.random.RandomInt( 100 ) > 50 && enemyInfo.enemyDist < 1500.0f ) {
   			VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Stand_Ground_Movement; 
		} else {
			VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Chase_Movement; //mal: move around a bit to make ourselves harder to hit.
		}
		return true;
	}

	if ( botVehicleInfo->type == GOLIATH && botVehicleInfo->driverEntNum == botNum ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Stand_Ground_Movement;
		return true;
	}

	if ( botInfo->proxyInfo.weapon == TANK_GUN && botVehicleInfo->driverEntNum == botNum ) {
		if ( !enemyReachable && enemyInfo.enemyDist < 3000.0f ) { //mal: dont do chase movement of our enemy, if we cant reach him.
           VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Stand_Ground_Movement; 
		} else {
			if ( enemyAcquireTime + 10000 < botWorld->gameLocalInfo.time && botInfo->lastAttackClientTime + 10000 < botWorld->gameLocalInfo.time ) {
				proxyInfo_t enemyVehicleInfo;
				GetVehicleInfo( enemyPlayerInfo.proxyInfo.entNum, enemyVehicleInfo );

				if ( enemyVehicleInfo.entNum != 0 && enemyVehicleInfo.type >= ICARUS ) { //mal: dont chase air vehicles around - this looks retarded.
					VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Stand_Ground_Movement;
				} else {
					VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Chase_Movement; //mal: haven't had much success hitting our enemy - so chase him for a better shot.
				}
			} else {
//				if ( botThreadData.random.RandomInt( 100 ) > 40 ) {
					VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Stand_Ground_Movement;
//				} else {
//					VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Vehicle_Chase_Movement;  //mal: move around a bit to make ourselves harder to hit.
//				}
			}
		}

		return true;
	}


	//mal_TODO: add support for air vehicles too......

	botThreadData.Warning( "Can't find a move that suits bot %i and his vehicle!", botNum );

	return false;
}

/*
================
idBotAI::Enter_Vehicle_Stand_Ground_Movement
================
*/
bool idBotAI::Enter_Vehicle_Stand_Ground_Movement() {
 
	VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Vehicle_Stand_Ground_Movement;

	combatMoveType = VEHICLE_STAND_GROUND;

	combatMoveDir = NULL_DIR;

	combatMoveFailedCount = 0;
	
	combatMoveTime = 0;

	lastMoveNode = "Vehicle Stand Ground";

	vehicleAINodeSwitch.nodeSwitchCount++;

	 return true;
}

/*
================
idBotAI::Vehicle_Stand_Ground_Movement
================
*/
bool idBotAI::Vehicle_Stand_Ground_Movement() {

//	int result;
	bool enemyHasVehicle = false;
	float tooCloseDist = 750.0f;
	proxyInfo_t enemyVehicleInfo;
	idVec3 vec;

	if ( combatMoveFailedCount >= 10 ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		GetVehicleInfo( botWorld->clientInfo[ enemy ].proxyInfo.entNum, enemyVehicleInfo );
		enemyHasVehicle = true;
	}

	if ( combatDangerExists ) {
		Bot_SetupVehicleQuickMove( botInfo->origin, false ); //mal: just path to itself, if its in an obstacle, it will freak and avoid it.

		if ( MoveIsInvalid() ) {
			VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}

		vec = botAAS.path.moveGoal - botInfo->origin;

		if ( vec.LengthSqr() > Square( 25.0f ) ) {
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, SPRINT, NULLMOVETYPE );
		}

		return true;
	}

	if ( botVehicleInfo->type == TROJAN && botVehicleInfo->driverEntNum == botNum ) {
		if ( !InFrontOfVehicle( botVehicleInfo->entNum, botWorld->clientInfo[ enemy ].origin ) ) {
			Bot_SetupVehicleMove( vec3_zero, enemy, ACTION_NULL );
            
			if ( MoveIsInvalid() ) {
				VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
				return false;
			}

            Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
			return true;
		}
	}


/*
	if ( ( !botInfo->proxyInfo.weaponIsReady && botInfo->proxyInfo.weapon == TANK_GUN ) || combatMoveTime < botWorld->gameLocalInfo.time && !vehicleInfo.inSiegeMode ) {

		if ( combatMoveDir == NULL_DIR ) {
            result = botThreadData.random.RandomInt( 4 );

			if ( result == 0 ) {
				combatMoveDir = FORWARD;
			} else if ( result == 1 ) {
				combatMoveDir = BACK;
			} else if ( result == 2 ) {
				combatMoveDir = RIGHT;
			} else {
				combatMoveDir = LEFT;
			}

			combatMoveTime = botWorld->gameLocalInfo.time + 2000;
		}

		if ( Bot_CanMove( combatMoveDir, 300.0f, true ) ) {
            Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		} else {
			combatMoveDir = NULL_DIR;
			combatMoveFailedCount++;
		}

		return true;
	}
*/

	if ( enemyInfo.enemyDist < tooCloseDist ) {
		if ( botVehicleInfo->type == GOLIATH ) {
			const clientInfo_t& enemyPlayer = botWorld->clientInfo[ enemy ];
			botUcmd->botCmds.exitSiegeMode = true;
			if ( enemyPlayer.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {
				Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, TAUNT_MOVE );
				return true;
			}
		}

		if ( enemyInfo.enemyInfont ) {
            if ( Bot_VehicleCanMove( BACK, 100.0f, true )) {
  				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, REVERSE, NULLMOVETYPE );
			} else if ( Bot_VehicleCanMove( RIGHT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, REVERSE, NULLMOVETYPE );
			} else if ( Bot_VehicleCanMove( LEFT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, REVERSE, NULLMOVETYPE );
			} else {
				combatMoveFailedCount++;
			}
		} else {
			if ( Bot_VehicleCanMove( FORWARD, 100.0f, true )) {
  				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else if ( Bot_VehicleCanMove( RIGHT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else if ( Bot_VehicleCanMove( LEFT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else {
				combatMoveFailedCount++;
			}
		}

		return true;
	}

	if ( botVehicleInfo->type == GOLIATH && !botVehicleInfo->inSiegeMode ) {
		idVec3 enemyOrg = botWorld->clientInfo[ enemy ].origin;
		trace_t trace;
		botThreadData.clip->TracePointExt( CLIP_DEBUG_PARMS trace, botVehicleInfo->origin, enemyOrg, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ), GetGameEntity( botVehicleInfo->entNum ) );
		if ( enemyInfo.enemyDist > 1200.0f && trace.fraction == 1.0f ) {
            botUcmd->botCmds.enterSiegeMode = true;
		}
	} else if ( botVehicleInfo->type == DESECRATOR ) {
        if ( enemyHasVehicle && enemyVehicleInfo.type != TITAN ) {
			if ( enemyInfo.enemyDist > 1500.0f ) {
                botUcmd->botCmds.enterSiegeMode = true;
			}
		} /* else {												
			if ( combatMoveTime < botWorld->gameLocalInfo.time ) {		//mal_NOTE: this just caused more problems then its worth.
				if ( combatMoveDir == NULL_DIR ) {
                    if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
						combatMoveDir = RIGHT;
					} else {
						combatMoveDir = LEFT;
					}
				} else {
                    if ( combatMoveDir == RIGHT ) {
						combatMoveDir = LEFT;
					} else {
						combatMoveDir = RIGHT;
					}
				}

				combatMoveTime = botWorld->gameLocalInfo.time + 5000;
			}

            if ( combatMoveDir == RIGHT ) {
				Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, LEAN_RIGHT );
			} else {
				Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, LEAN_LEFT );
			}
			return true;
		} */
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, FULL_STOP ); //mal: hit the brakes!

	return true;
}

/*
================
idBotAI::Enter_Vehicle_NULL_Movement

Used by vehicle gunners.
================
*/
bool idBotAI::Enter_Vehicle_NULL_Movement() {
 
	VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Vehicle_NULL_Movement;

	combatMoveType = VEHICLE_NULL_MOVEMENT;

	lastMoveNode = "Vehicle NULL Move";

	vehicleAINodeSwitch.nodeSwitchCount++;

	 return true;
}

/*
================
idBotAI::Vehicle_NULL_Movement
================
*/
bool idBotAI::Vehicle_NULL_Movement() {
	if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = NULL;								//mal:- instead of doing this, just forget enemy!?? OR
		return false;
	}

	if ( botVehicleInfo->driverEntNum == botNum ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = NULL; //mal: this just isn't working out, try something else.
		return false;
	}

	return true;
}

/*
================
idBotAI::Enter_Vehicle_Random_Movement
================
*/
bool idBotAI::Enter_Vehicle_Random_Movement() {
 
	VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Vehicle_Random_Movement;

	combatMoveType = VEHICLE_RANDOM_MOVEMENT;

	combatMoveDir = NULL_DIR;

	combatMoveFailedCount = 0;

	combatMoveTime = 0;

	lastMoveNode = "Vehicle Random Move";

	vehicleAINodeSwitch.nodeSwitchCount++;

	 return true;
}

/*
================
idBotAI::Vehicle_Random_Movement
================
*/
bool idBotAI::Vehicle_Random_Movement() { 

	float tooCloseDist = 750.0f;
	idVec3 vec;

	if ( combatMoveFailedCount >= 10 ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = NULL; //mal: this just isn't working out, try something else.
		return false;
	}

	if ( combatDangerExists ) {
		Bot_SetupVehicleQuickMove( botInfo->origin, false ); //mal: just path to itself, if its in an obstacle, it will freak and avoid it.

		if ( MoveIsInvalid() ) {
			VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}

		vec = botAAS.path.moveGoal - botInfo->origin;

		if ( vec.LengthSqr() > Square( 25.0f ) ) {
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, SPRINT, NULLMOVETYPE );
		}

		return true;
	}

	if ( combatMoveDir == NULL_DIR || combatMoveTime < botWorld->gameLocalInfo.time ) {

		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
                combatMoveDir = RIGHT;
			} else {
				combatMoveDir = LEFT;
			}
		} else {
			if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
                combatMoveDir = FORWARD;
			} else {
				combatMoveDir = BACK;
			}
		}

		if ( Bot_VehicleCanMove( combatMoveDir, 300.0f, true ) ) {
            combatMoveTime = botWorld->gameLocalInfo.time + 7000;
		} else {
			combatMoveFailedCount++;
		}

		return true;
	}

	if ( enemyInfo.enemyDist < tooCloseDist ) {
		if ( enemyInfo.enemyInfont ) {
            if ( Bot_VehicleCanMove( BACK, 100.0f, true )) {
  				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else {
				VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
			}
		} else {
			if ( Bot_VehicleCanMove( FORWARD, 100.0f, true )) {
  				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else {
				VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
			}
		}

		return true;
	}

	if ( Bot_VehicleCanMove( combatMoveDir, 300.0f, true ) ) {
        Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
	} else {
		combatMoveDir = NULL_DIR;
		combatMoveFailedCount++;
	}

	return true;
}



/*
================
idBotAI::Enter_Vehicle_Ram_Attack_Movement
================
*/
bool idBotAI::Enter_Vehicle_Ram_Attack_Movement() {
 
	VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Vehicle_Ram_Attack_Movement;

	combatMoveType = VEHICLE_RAM_ATTACK_MOVEMENT;

	lastMoveNode = "Vehicle Ram Attack";

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::Vehicle_Ram_Attack_Movement
================
*/
bool idBotAI::Vehicle_Ram_Attack_Movement() {
	proxyInfo_t enemyVehicleInfo;
	idBox enemyBox;

	if ( enemyInfo.enemyDist < 700.0f && botVehicleInfo->forwardSpeed < 100.0f && botVehicleInfo->type != HOG ) { //mal: somethings holding us up, we can't ram at this speed/dist!
		VEHICLE_COMBAT_MOVEMENT_STATE = NULL; 
		return false;
	}

	if ( !Bot_VehicleCanRamClient( enemy ) ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		GetVehicleInfo( botWorld->clientInfo[ enemy ].proxyInfo.entNum, enemyVehicleInfo );
		enemyBox = idBox( enemyVehicleInfo.bbox, enemyVehicleInfo.origin, enemyVehicleInfo.axis );
	} else {
		enemyBox = idBox( botWorld->clientInfo[ enemy ].localBounds, botWorld->clientInfo[ enemy ].origin, botWorld->clientInfo[ enemy ].bodyAxis );
	}

	if ( !InFrontOfVehicle( botVehicleInfo->entNum, enemyBox.GetCenter(), false ) ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	Bot_MoveToGoal( enemyBox.GetCenter(), vec3_zero, SPRINT_ATTACK, NULLMOVETYPE ); //mal: Prepare for ramming speed! Ram them until they're dead, or we are....
	Bot_LookAtLocation( enemyBox.GetCenter(), SMOOTH_TURN );

    return true;
}

/*
================
idBotAI::Enter_Vehicle_Chase_Movement
================
*/
bool idBotAI::Enter_Vehicle_Chase_Movement() {
 
	VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Vehicle_Chase_Movement;

	combatMoveType = VEHICLE_CHASE_MOVEMENT;

	lastMoveNode = "Vehicle Chase Move";

	vehicleAINodeSwitch.nodeSwitchCount++;

	return true;
}

/*
================
idBotAI::Vehicle_Chase_Movement
================
*/
bool idBotAI::Vehicle_Chase_Movement() {

	if ( enemyInfo.enemyDist > 900.0f ) {
		Bot_SetupVehicleMove( vec3_zero, enemy, ACTION_NULL );

        if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ResetEnemy();
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
	}

	if ( botVehicleInfo->type == HOG && Bot_VehicleCanRamClient( enemy ) ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
		return true;
	}

	if ( enemyInfo.enemyDist < 700.0f ) {
		VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
	}

	return true;
}

/*
================
idBotAI::Enter_Vehicle_Air_Movement
================
*/
bool idBotAI::Enter_Vehicle_Air_Movement() {
	proxyInfo_t enemyVehicleInfo;
 
	combatMoveType = VEHICLE_AIR_MOVEMENT;
	combatMoveActionGoal = ACTION_NULL;
	combatMoveTooCloseRange = 0.0f;

	if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE  ) {
		GetVehicleInfo( botWorld->clientInfo[ enemy ].proxyInfo.entNum, enemyVehicleInfo );
		if ( enemyVehicleInfo.isAirborneVehicle ) {
			if ( botVehicleInfo->type == BUFFALO ) {
				VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Buffalo_Air_To_Air_Movement;
				lastMoveNode = "Buffalo Air To Air Combat";
				combatMoveTime = botWorld->gameLocalInfo.time + 500;
			} else {
				VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Vehicle_Air_To_Air_Movement;
				lastMoveNode = "Vehicle Air To Air Combat";
			}
		} else {
			if ( botVehicleInfo->type == BUFFALO ) {
				VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Buffalo_Air_To_Ground_Movement;
				lastMoveNode = "Buffalo Air To Ground Combat";
				combatMoveTime = botWorld->gameLocalInfo.time + 500;
			} else {
				combatKeepMovingTime = 0;
				VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Vehicle_Air_To_Ground_Movement;
				lastMoveNode = "Vehicle Air To Ground Combat";
			}
		}
	} else {
		if ( botVehicleInfo->type == BUFFALO ) {
			VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Buffalo_Air_To_Ground_Movement;
			lastMoveNode = "Buffalo Air To Ground Combat";
			combatMoveTime = botWorld->gameLocalInfo.time + 500;
		} else {
			combatKeepMovingTime = 0;
			VEHICLE_COMBAT_MOVEMENT_STATE = &idBotAI::Vehicle_Air_To_Ground_Movement;
			lastMoveNode = "Vehicle Air To Ground Combat";
		}
	}

	rocketTime = 0;

	return true;
}

/*
================
idBotAI::Vehicle_Air_To_Air_Movement

Air to Air specific attack case.
Special case: Air vehicles can't attack one way, while move in another. So they need to have their attack/move unified.
This is a bit of a hack, but theres no time to do anything else. :-(
================
*/
bool idBotAI::Vehicle_Air_To_Air_Movement() {
	bool overRideLook = false;
	bool enemyInFront;
	bool botInFrontOfEnemy;
	float desiredRange;
	float tooCloseDist;
	float dist;
	proxyInfo_t enemyVehicleInfo;
	idVec3 enemyOrg;
	idVec3 vec;

	if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {
		Bot_ResetEnemy();
		return false;
	}

	GetVehicleInfo( botWorld->clientInfo[ enemy ].proxyInfo.entNum, enemyVehicleInfo );
	enemyOrg = enemyVehicleInfo.origin;

	vec = enemyOrg - botVehicleInfo->origin;
	vec.z = 0.0f;
	dist = vec.LengthSqr();
	float tempDist = vec.LengthFast();

	Bot_CheckVehicleAttack(); //mal: always see if we can get a shot off.

	if ( botInfo->enemyHasLockon && dist < Square( 6000.0f ) && botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) { //mal: OH NOES!1 Panic and run for it.
		Bot_IgnoreEnemy( enemy, 3000 );
		Bot_ResetEnemy();
		return false;
	}

	if ( botVehicleInfo->type == ANANSI ) {
		desiredRange = 5000.0f;
		tooCloseDist = 2500.0f;

		if ( Bot_CheckEnemyHasLockOn( enemy ) && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) { //mal: they need a bit of an edge
			botUcmd->botCmds.launchDecoysNow = true;
		}

		if ( dist < Square( tooCloseDist ) && combatMoveTime < botWorld->gameLocalInfo.time && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
			combatMoveDir = BACK;
			combatMoveTime = botWorld->gameLocalInfo.time + 5000;
			botUcmd->botCmds.launchDecoys = true; //mal: we're exposing our flank, so fire some decoys to cover our move.
		}

		if ( combatMoveTime > botWorld->gameLocalInfo.time ) {
			if ( !InAirVehicleGunSights( enemyVehicleInfo.entNum, botInfo->origin ) ) { //mal: he can't see us anymore, so attack!
				combatMoveTime = 0;
				overRideLook = true;
			}
		}

		if ( combatMoveTime > 0 ) {
			vec = enemyOrg;
			vec += ( -desiredRange * enemyVehicleInfo.axis[ 0 ] ); 

			if ( !botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, botInfo->areaNumVehicle, botInfo->aasVehicleOrigin, vec ) ) {
				combatMoveTime = 0;
				return true;
			}

			Bot_SetupVehicleMove( vec, -1, ACTION_NULL );

			if ( MoveIsInvalid() ) {
				combatMoveTime = 0;
				return true;
			}

			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
			return true;
		}
	} else { //must be in a hornet
		 desiredRange = 2500.0f; //mal: was 1500
		 tooCloseDist = 0.0f;

		if ( dist < Square( desiredRange ) && combatMoveTime < botWorld->gameLocalInfo.time && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
			int n = botThreadData.random.RandomInt( 3 ); //mal: we're too close, so manuever around our enemy and fire. low skill bots need not apply.

			if ( n == 0 ) {
				combatMoveDir = BACK;
			} else if ( n == 1 ) {
				combatMoveDir = RIGHT;
			} else {
				combatMoveDir = LEFT;
			}

			combatMoveTime = botWorld->gameLocalInfo.time + 5000;
			botUcmd->botCmds.launchDecoys = true; //mal: we're exposing our flank, so fire some decoys to cover our move.
		}

		if ( combatMoveTime > botWorld->gameLocalInfo.time ) {
			if ( !InAirVehicleGunSights( enemyVehicleInfo.entNum, botInfo->origin ) ) { //mal: he can't see us anymore, so attack!
				combatMoveTime = 0;
				overRideLook = true;
			}
		}

		if ( combatMoveTime > 0 ) {
			vec = enemyOrg;
			if ( combatMoveDir == BACK ) {
				vec += ( -desiredRange * botInfo->viewAxis[ 0 ] ); //mal: this seems wrong, but it works SO well.
			} else if ( combatMoveDir == RIGHT ) {
				vec += ( desiredRange * ( botInfo->viewAxis[ 1 ] * -1 ) );
			} else if ( combatMoveDir == LEFT ) {
				vec += ( -desiredRange * ( botInfo->viewAxis[ 1 ] * -1 ) );
			}

			if ( !botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, botInfo->areaNumVehicle, botInfo->aasVehicleOrigin, vec ) ) {
				combatMoveTime = 0;
				return true;
			}

			Bot_SetupVehicleMove( vec, -1, ACTION_NULL );

			if ( MoveIsInvalid() ) {
				combatMoveTime = 0;
				return true;
			}

			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
			return true;
		}
	}

	enemyInFront = InFrontOfVehicle( botVehicleInfo->entNum, enemyOrg );
	botInFrontOfEnemy = InFrontOfVehicle( enemyVehicleInfo.entNum, botInfo->origin );

	if ( dist > Square( desiredRange ) )  {
		Bot_SetupVehicleMove( vec3_zero, enemy, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ResetEnemy();
			return false;
		}

//mal: if we're just moving into range of the target, we'll try to get a lock. If not ( or in danger ) we'll gun it. Fighter pilot mantra: speed is life.
		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, ( botInFrontOfEnemy && enemyInFront && dist > Square( 6000.0f ) && botVehicleInfo->forwardSpeed > 1500.0f && !botInfo->enemyHasLockon ) ? AIR_COAST : NULLMOVETYPE );

		if ( enemyInFront || overRideLook ) {
			Bot_LookAtEntity( enemy, SMOOTH_TURN );
		} else {
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		}
	} else {
		Bot_SetupVehicleMove( vec3_zero, enemy, ACTION_NULL ); //mal: still want to take into account obstacles, so do a move check.

		if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ResetEnemy();
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, ( botAAS.obstacleNum == -1 ) ? AIR_BRAKE : NULLMOVETYPE );
		Bot_LookAtEntity( enemy, SMOOTH_TURN );
	}

	return true;
}

/*
================
idBotAI::Vehicle_Air_To_Ground_Movement

Ground Specific attack case.
Special case: Air vehicles can't attack one way, while move in another. So they need to have their attack/move unified.
This is a bit of a hack, but theres no time to do anything else. :-(
================
*/
bool idBotAI::Vehicle_Air_To_Ground_Movement() {
	bool overRideLook = false;
	bool inVehicle = false;
	float desiredRange = 5800.0f;
	float tooCloseRange = 2700.0f;
	float enemySpeed = 0.0f;
	float dist;
	proxyInfo_t enemyVehicleInfo;
	idVec3 enemyOrg;
	idVec3 vec;

	if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		GetVehicleInfo( botWorld->clientInfo[ enemy ].proxyInfo.entNum, enemyVehicleInfo );
		enemyOrg = enemyVehicleInfo.origin;
		inVehicle = true;
	} else {
		enemyOrg = botWorld->clientInfo[ enemy ].origin;
	}

	enemySpeed = botWorld->clientInfo[ enemy ].xySpeed;

	vec = enemyOrg - botVehicleInfo->origin;
	vec[ 2 ] = 0.0f;
	dist = vec.LengthSqr();

	Bot_CheckVehicleAttack(); //mal: always see if we can get a shot off.

	if ( botInfo->enemyHasLockon && dist < Square( 6000.0f ) && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: OH NOES! Panic and run for it. 
		Bot_IgnoreEnemy( enemy, 3000 );
		Bot_ResetEnemy();
		return false;
	}

//mal: we're too close, so manuever around our enemy and fire. 
	if ( dist < Square( tooCloseRange ) && combatMoveTime < botWorld->gameLocalInfo.time && enemySpeed < SPRINTING_SPEED ) { //mal: if they're moving fast, just let them move into our crosshairs..
		int actionNumber = ACTION_NULL;

		if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
			actionNumber = Bot_FindNearbySafeActionToMoveToward( botInfo->origin, ( desiredRange ) );
		}

		if ( actionNumber != -1 ) {
			combatMoveActionGoal = actionNumber;
			combatMoveTime = botWorld->gameLocalInfo.time + 10000;
			combatMoveTooCloseRange = ( inVehicle ) ? 5000.0f : tooCloseRange + 1000.0f;
		} else {
			int n = botThreadData.random.RandomInt( 3 );

			if ( n == 0 ) {
				combatMoveDir = BACK;
			} else if ( n == 1 ) {
				combatMoveDir = RIGHT;
			} else {
				combatMoveDir = LEFT;
			}

			combatMoveTime = botWorld->gameLocalInfo.time + 5000;
			combatMoveTooCloseRange = tooCloseRange;
		}
	}

	if ( combatMoveTime > botWorld->gameLocalInfo.time ) {
		if ( dist > Square( combatMoveTooCloseRange ) ) { //mal: we're far enough away to get a shot - attack.
			combatMoveTime = 0;
			combatMoveActionGoal = ACTION_NULL;
			overRideLook = true;
		}
	}

	if ( combatMoveTime > botWorld->gameLocalInfo.time ) {
		if ( combatMoveActionGoal != ACTION_NULL ) {
			Bot_SetupVehicleMove( vec3_zero, -1, combatMoveActionGoal );
		} else {
			vec = enemyOrg;
			if ( combatMoveDir == BACK ) {
				vec += ( -tooCloseRange * botWorld->clientInfo[ enemy ].viewAxis[ 0 ] );
			} else if ( combatMoveDir == RIGHT ) {
				vec += ( tooCloseRange * ( botWorld->clientInfo[ enemy ].viewAxis[ 1 ] * -1 ) );
			} else if ( combatMoveDir == LEFT ) {
				vec += ( -tooCloseRange * ( botWorld->clientInfo[ enemy ].viewAxis[ 1 ] * -1 ) );
			}

			vec.z = botVehicleInfo->origin.z;

			if ( !botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, botInfo->areaNumVehicle, botInfo->aasVehicleOrigin, vec ) ) {
				combatMoveTime = 0;
				return true;
			}

			Bot_SetupVehicleMove( vec, -1, ACTION_NULL );
		}

		if ( MoveIsInvalid() ) {
			combatMoveTime = 0;
			combatMoveActionGoal = ACTION_NULL;
			return true;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	if ( dist > Square( desiredRange ) )  {
		Bot_SetupVehicleMove( vec3_zero, enemy, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ResetEnemy();
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );

		if ( InFrontOfVehicle( botVehicleInfo->entNum, enemyOrg ) || overRideLook ) {
			Bot_LookAtEntity( enemy, SMOOTH_TURN );
		} else {
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		}
	} else {
		Bot_SetupVehicleMove( vec3_zero, enemy, ACTION_NULL ); //mal: still want to take into account obstacles, so do a move check.

		if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ResetEnemy();
			return false;
		}

		botMoveTypes_t defaultMoveType = AIR_BRAKE;

		if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY && Bot_VehicleIsUnderAVTAttack() != -1 || Bot_CheckIfEnemyHasUsInTheirSightsWhenInAirVehicle() || Bot_CheckEnemyHasLockOn( -1, true ) || combatKeepMovingTime > botWorld->gameLocalInfo.time ) { //mal: do a bombing run if someone is shooting at us!
			defaultMoveType = NULLMOVETYPE;

			if ( combatKeepMovingTime < botWorld->gameLocalInfo.time ) {
				combatKeepMovingTime = botWorld->gameLocalInfo.time + FLYER_AVOID_DANGER_TIME;
			}
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, ( botAAS.obstacleNum == -1 ) ? defaultMoveType : NULLMOVETYPE );
		Bot_LookAtEntity( enemy, SMOOTH_TURN );
	}

	return true;
}

/*
================
idBotAI::Buffalo_Air_To_Ground_Movement

Ground Specific attack case for the buffalo
================
*/
bool idBotAI::Buffalo_Air_To_Ground_Movement() {
	float desiredRange = 4500.0f;
	const clientInfo_t& enemyPlayerInfo = botWorld->clientInfo[ enemy ];

	idVec3 distToTarget = enemyPlayerInfo.origin - botVehicleInfo->origin;
	distToTarget.z = 0.0f;

	if ( distToTarget.LengthSqr() > Square( desiredRange ) )  {
		Bot_SetupVehicleMove( vec3_zero, enemy, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ResetEnemy();
			return false;
		}

		combatMoveTime = botWorld->gameLocalInfo.time + 500;

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	if ( InFrontOfVehicle( botVehicleInfo->entNum, enemyPlayerInfo.origin ) && combatMoveTime < botWorld->gameLocalInfo.time ) {
		if ( botVehicleInfo->xyspeed < 100.0f ) {
			Bot_LookAtEntity( enemy, SMOOTH_TURN );
			botUcmd->botCmds.topHat = true;
		}
		Bot_CheckVehicleAttack(); //mal: always see if we can get a shot off.
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, FULL_STOP );

	return true;
}

/*
================
idBotAI::Buffalo_Air_To_Air_Movement

Air to Air specific attack case for the buffalo.
================
*/
bool idBotAI::Buffalo_Air_To_Air_Movement() {
	float desiredRange = 4500.0f;
	const clientInfo_t& enemyPlayerInfo = botWorld->clientInfo[ enemy ];

	idVec3 vec = enemyPlayerInfo.origin - botVehicleInfo->origin;
	vec[ 2 ] = 0.0f;
	float distSqr = vec.LengthSqr();

	if ( distSqr > Square( desiredRange ) )  {
		Bot_SetupVehicleMove( vec3_zero, enemy, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ResetEnemy();
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );

		if ( InFrontOfVehicle( botVehicleInfo->entNum, enemyPlayerInfo.origin ) ) {
			Bot_LookAtEntity( enemy, SMOOTH_TURN );
			botUcmd->botCmds.topHat = true;
			Bot_CheckVehicleAttack(); //mal: always see if we can get a shot off.
		} else {
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		}

		return true;
	}

	if ( InFrontOfVehicle( botVehicleInfo->entNum, enemyPlayerInfo.origin ) ) {
		Bot_LookAtEntity( enemy, SMOOTH_TURN );
		botUcmd->botCmds.topHat = true;
		Bot_CheckVehicleAttack(); //mal: always see if we can get a shot off.
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, FULL_STOP );

	return true;
}

