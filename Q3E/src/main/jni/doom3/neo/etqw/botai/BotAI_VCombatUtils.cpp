// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idBotAI::Bot_VehicleFindEnemy

We'll sort thru the clients, and ignore certain clients if we're too busy
to be buggered (carrying obj, planting/hacking, etc) or they're not valid enemies
(in disguise, hidden by smoke, etc).
================
*/
bool idBotAI::Bot_VehicleFindEnemy() {
	bool hasAttackedMate;
	bool hasAttackedCriticalMate;
	bool hasObj;
	bool isDefusingOurBomb;
	bool inFront;
	bool botGotShotRecently;
	bool botIsBigShot;
	bool audible;
	bool isVisible;
	bool isFacingUs;
	bool isFiringWeapon;
	bool isNearOurObj;
	bool isAttackingDeployable = false;
	bool inAttackAirCraft = false;
	int i;
	int entClientNum = -1;
	float dist;
	float botSightDist;
	float tempSightDist;
	float entDist = idMath::INFINITY;
	proxyInfo_t enemyVehicleInfo;
	enemyVehicleInfo.entNum = 0;
	idVec3 vec;

	numVisEnemies = 0;

	vehicleEnemyWasInheritedFromFootCombat = false;

	botSightDist = Square( ENEMY_VEHICLE_SIGHT_DIST ); //mal_FIXME: break this out into a script cmd!

/*
	if ( botSightDist > Square( 8000.0f ) ) {
		botSightDist = Square( 8000.0f );
	} else if ( botSightDist < Square( 3000.0f ) ) {
		botSightDist = Square( 3000.0f );
	}
*/

//mal: some debugging stuff....
	if ( botWorld->gameLocalInfo.botIgnoreEnemies == 1 ) {
			return false;
	} else if ( botWorld->gameLocalInfo.botIgnoreEnemies == 2 ) {
			if ( botInfo->team == GDF ) {
				return false;
			}
	} else if ( botWorld->gameLocalInfo.botIgnoreEnemies == 3 ) {
			if ( botInfo->team == STROGG ) {
				return false;
			}
		}

	if ( botVehicleInfo->type != BUFFALO && Bot_VehicleIsUnderAVTAttack() != -1 && ( ( botVehicleInfo->flags & ARMOR ) || botVehicleInfo->type > ICARUS ) ) {
		return false;
	}

	if ( botVehicleInfo->type > ICARUS ) {
		botSightDist = Square( 6000.0f );
	}

	if ( botVehicleInfo->type == BUFFALO ) {
		botSightDist = Square( 3500.0f );
	}

	if ( botVehicleInfo->type == GOLIATH || botVehicleInfo->type == DESECRATOR ) { //mal: these 2 are really limited
		botSightDist = Square( PLASMA_CANNON_RANGE - 1000.0f );
	}

	if ( botVehicleInfo->type == HUSKY ) { //mal: we're no match for anybody!
		return false;
	}

#ifdef _XENON
	if ( botVehicleInfo->type == MCP && botVehicleInfo->driverEntNum == botNum ) {
		return false;
	}

	if ( botVehicleInfo->type == PLATYPUS && botVehicleInfo->driverEntNum == botNum ) {
		return false;
	}
#endif

	if ( botVehicleInfo->type > ICARUS && Client_IsCriticalForCurrentObj( botNum, -1.0f ) ) {
		return false;
	}

	botIsBigShot = Client_IsCriticalForCurrentObj( botNum, 3500.0f );

	if ( aiState == VLTG && vLTGType == V_DESTROY_DEPLOYABLE ) {
		deployableInfo_t deployable;
		if ( GetDeployableInfo( false, vLTGTarget, deployable ) ) {
			if ( deployable.type == APT || deployable.type == AVT || deployable.type == AIT ) { //mal: these are the priorities
				isAttackingDeployable = true;
			}
		}
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( i == botNum ) {
			continue; //mal: dont try to fight ourselves!
		}

		if ( EnemyIsIgnored( i ) ) {
			continue; //mal: dont try to fight someone we've flagged to ignore for whatever reason!
		}

		if ( !Bot_VehicleCanAttackEnemy( i ) ) { //mal: check if the bot has access to a weapon in the vehicle, that can hit this client.
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.isNoTarget ) {
			continue;
		} //mal: dont target clients that have notarget set - this is useful for debugging, etc.

		bool enemyIsBigShot = Client_IsCriticalForCurrentObj( i, 2500.0f );

		if ( playerInfo.inLimbo ) {
			continue;
		}

		if ( playerInfo.invulnerableEndTime > botWorld->gameLocalInfo.time ) {
			continue; //mal: ignore revived/just spawned in clients - get the ppl around them!
		}

		if ( playerInfo.isActor ) {
			continue;
		}

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		if ( playerInfo.team == botInfo->team ) {
			continue;
		}

		if ( botVehicleInfo->type == MCP && botVehicleInfo->driverEntNum == botNum ) {
			if ( !InFrontOfVehicle( botInfo->proxyInfo.entNum, playerInfo.origin ) ) {
				continue;
			}
		}

		if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO && botVehicleInfo->type > ICARUS && !playerInfo.isBot ) { //mal: don't attack human players in training mode when we're in flyers.
			continue;
		}

		hasAttackedCriticalMate = ClientHasAttackedTeammate( i, true, 3000 );
		hasAttackedMate = ClientHasAttackedTeammate( i, false, 3000 );
		hasObj = ClientHasObj( i );
		isDefusingOurBomb = ClientIsDefusingOurTeamCharge( i );
		inFront = ( botInfo->proxyInfo.weapon == PERSONAL_WEAPON ) ? InFrontOfClient( botNum, playerInfo.origin) : InFrontOfVehicle( botInfo->proxyInfo.entNum, playerInfo.origin );
		isFacingUs = ( playerInfo.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) ? InFrontOfClient( i, botInfo->origin ) : InFrontOfVehicle( playerInfo.proxyInfo.entNum, botInfo->origin );
		botGotShotRecently = ( botInfo->lastAttackerTime + 3000 < botWorld->gameLocalInfo.time ) ? false : true;
		isFiringWeapon = playerInfo.weapInfo.isFiringWeap;
		isNearOurObj = ( LocationDistFromCurrentObj( botInfo->team, playerInfo.origin ) < 2500.0f ) ? true : false;
		bool isCriticalEnemy = Client_IsCriticalForCurrentObj( i, 2500.0f );

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { 
			GetVehicleInfo( playerInfo.proxyInfo.entNum, enemyVehicleInfo );
		}

		if ( isAttackingDeployable ) {
			if ( botInfo->team == botWorld->botGoalInfo.attackingTeam ) {
				if ( playerInfo.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {
					if ( playerInfo.weapInfo.weapon != ROCKET && !isDefusingOurBomb ) {
						continue;
					}
				} else {
					if ( !( enemyVehicleInfo.flags & ARMOR ) && enemyVehicleInfo.type != ANANSI && enemyVehicleInfo.type != HORNET ) {
			continue;
		}
				}
			} else {
				if ( !isDefusingOurBomb && playerInfo.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE && !hasObj && playerInfo.weapInfo.weapon != ROCKET && ( !isCriticalEnemy || !isNearOurObj ) ) {
					continue;
				}
			}
		}

		if ( botIsBigShot && !isFacingUs && ( !botGotShotRecently || botInfo->lastAttacker != i ) && !isFiringWeapon && !hasObj && !isNearOurObj ) {
			continue;
		} //mal: if we're trying to do an important obj, dont get into a fight with everyone.

		vec = playerInfo.origin - botInfo->origin;

		if ( botVehicleInfo->isAirborneVehicle ) {
			vec.z = 0.0f;
		}

		dist = vec.LengthSqr();

		if ( botIsBigShot && !inFront && dist > Square( 2500.0f ) && ( !botGotShotRecently || botInfo->lastAttacker != i ) && botVehicleInfo->driverEntNum == botNum ) {
			continue;
		}

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: pick the driver of a vehicle as the target, NOT passengers, unless there is no driver - then kill whoever.
			if ( enemyVehicleInfo.type == ANANSI || enemyVehicleInfo.type == HORNET ) {
				inAttackAirCraft = true;
			}

			if ( botIsBigShot && enemyVehicleInfo.type <= ICARUS ) {
				continue;
			}

			if ( enemyVehicleInfo.type == BUFFALO && botVehicleInfo->flags & ARMOR && playerInfo.isBot ) {
				continue;
			}

			if ( enemyVehicleInfo.type == MCP && enemyVehicleInfo.isImmobilized && enemyVehicleInfo.driverEntNum == i ) {
				continue;
			}

			if ( botVehicleInfo->type == ANANSI && enemyVehicleInfo.type == ICARUS ) { //mal: this is funny to watch, but is a waste of time. :-)
				continue;
			}

			if ( isAttackingDeployable ) {
				if ( botVehicleInfo->isAirborneVehicle && ( enemyVehicleInfo.type <= ICARUS || enemyVehicleInfo.type == BUFFALO ) ) { //mal: if attacking from the air, only worry about air vehicles.
					continue;
				}
			}

			if ( enemyVehicleInfo.driverEntNum != i && enemyVehicleInfo.driverEntNum != -1 ) {
				continue;
			}

			if ( inAttackAirCraft && enemyVehicleInfo.xyspeed > 500.0f && dist > Square( TANK_MINIGUN_RANGE ) && botVehicleInfo->flags & ARMOR ) { //mal: tanks won't attack fast moving aircraft that are too far away for their MGs
				continue;
			}

			if ( botVehicleInfo->type == BADGER ) {
				if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON && enemyVehicleInfo.flags & ARMOR || enemyVehicleInfo.inWater || ( enemyVehicleInfo.isAirborneVehicle && dist > Square( 3000.0f ) || enemyVehicleInfo.type == HOG ) ) {
                    continue;
				}
			}

			if ( botVehicleInfo->inWater && botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON ) {
				continue;
			}

			if ( botVehicleInfo->type != ANANSI && botVehicleInfo->type != HORNET ) {
                if ( enemyVehicleInfo.xyspeed > 600.0f && dist > Square( 1900.0f ) && !InFrontOfVehicle( enemyVehicleInfo.entNum, botInfo->origin ) && !ClientHasObj( i ) && !enemyIsBigShot ) { //mal: if they're in a mad dash away from us, forget about them!
					continue;
				}
			}
		}

		if ( botVehicleInfo->type == BUFFALO && !inAttackAirCraft ) { //mal_TODO: need to make the buffalo a more effective fighting platform!
			continue;
		}

		tempSightDist = botSightDist;

		if ( !ClientHasObj( i ) && !enemyIsBigShot && playerInfo.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE && !playerInfo.isCamper && playerInfo.killsSinceSpawn < KILLING_SPREE && botVehicleInfo->driverEntNum == botNum && !isDefusingOurBomb ) { //mal: vehicles will prefer to fight other vehicles, not some guy on foot a mile away....
			tempSightDist = Square( 3500.0f );
		}

		if ( inAttackAirCraft && ( botVehicleInfo->type == ANANSI || botVehicleInfo->type == HORNET ) && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
			tempSightDist = Square( AIRCRAFT_ATTACK_DIST );
		}

		if ( dist > tempSightDist ) {
            continue;
		}

		if ( playerInfo.isDisguised ) { //mal: when in a vehicle, bots are much less likely to notice or worry about coverts
			if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) {
				continue;
			} else {
				if ( ( playerInfo.disguisedClient != botNum && !hasAttackedMate ) || dist > Square( 2500.0f ) ) {
                    continue;
				}
			}
		} 

 		if ( !ClientHasObj( i ) ) {
            audible = ClientIsAudibleToVehicle( i ); //mal: if we can hear you, we'll skip the FOV test in the vis check below
		} else {
            audible = true;
			dist = Square( 500.0f ); //mal: if you've got the docs, your our priority target, unless someone else is right on top of us!
		}

		isVisible = ClientIsVisibleToBot( i, !audible, false );

		if ( !isVisible && !ClientHasObj( i ) ) {
			continue;
		}

		if ( botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO || botWorld->botGoalInfo.gameIsOnFinalObjective || botWorld->botGoalInfo.attackingTeam == botInfo->team ) {
			if ( isDefusingOurBomb ) {
				dist = Square( 100.0f );
			}

			if ( hasAttackedCriticalMate && inFront && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
				dist = Square( 600.0f ); //mal: will give higher priority to someone attacking a critical mate, if we can see it happening.
			}

			if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
				if ( Client_IsCriticalForCurrentObj( i, 6000.0f ) ) {
					dist = Square( 700.0f ); //mal: if your a critical client, we're more likely to kill you.
				}
			}

			if ( botWorld->botGoalInfo.mapHasMCPGoal ) {
				if ( playerInfo.proxyInfo.entNum == botWorld->botGoalInfo.botGoal_MCP_VehicleNum ) {
					dist = 400.0f;
				}
			} //mal: if your in MCP, you get higher priority then a normal enemy. Especially when we're in a vehicle!

			if ( botVehicleInfo->type > ICARUS && botVehicleInfo->type != BUFFALO && ( playerInfo.isCamper || playerInfo.killsSinceSpawn >= KILLING_SPREE ) && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY && !playerInfo.isBot ) {
				dist = Square( 600.0f );
			} //mal: target trouble making humans!
		} else {
			if ( !playerInfo.isBot || playerInfo.isActor ) {
				dist += Square( TRAINING_MODE_RANGE_ADDITION );
			}
		}

		numVisEnemies++;

		if ( dist < entDist ) {
			entClientNum = i;
			entDist = dist;
		}
	}

	if ( entClientNum != -1 ) {
		enemy = entClientNum;
		enemySpawnID = botWorld->clientInfo[ entClientNum ].spawnID;

		enemyInfo.enemy_FS_Pos = botWorld->clientInfo[ entClientNum ].origin;
		enemyInfo.enemy_LS_Pos = enemyInfo.enemy_FS_Pos;

		bot_FS_Enemy_Pos = botInfo->origin;
		bot_LS_Enemy_Pos = bot_FS_Enemy_Pos;
		enemyInfo.enemyLastVisTime = botWorld->gameLocalInfo.time;
		enemyAcquireTime = botWorld->gameLocalInfo.time;

		Bot_SetAttackTimeDelay( inFront ); //mal: this sets a delay on how long the bot should take to see enemy, based on bot's state.

		VEHICLE_COMBAT_AI_SUB_NODE = NULL; //mal: reset the bot's combat AI node
		COMBAT_MOVEMENT_STATE = NULL;
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_CheckCurrentStateForVehicleCombat

Check to see if we're doing an important goal that we need to avoid combat for, or if we can just jump into the fray.
================
*/
void idBotAI::Bot_CheckCurrentStateForVehicleCombat() {
	if ( botVehicleInfo->driverEntNum == botNum && ( Client_IsCriticalForCurrentObj( botNum, 4500.0f ) || ClientHasObj( botNum ) || ( AIStack.STACK_AI_NODE != NULL && AIStack.isPriority ) || botVehicleInfo->type == MCP || botVehicleInfo->type == BUFFALO ) ) {
        VEHICLE_COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Vehicle_EvadeEnemy; //mal: if bot has a priority target, and its close, try to avoid combat as much as possible.
		return;
	} //mal: if we're not the driver of this vehicle, we'll always attack ( really, what else can we do? ).

    
	VEHICLE_COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Vehicle_AttackEnemy; 
}

/*
================
idBotAI::Bot_CheckVehicleAttack
================
*/
void idBotAI::Bot_CheckVehicleAttack() {
	float minFOV = ( botInfo->proxyInfo.weapon == TANK_GUN ) ? 0.95f : 0.60f;
	idVec3 dir;

	if ( botVehicleInfo->type == ICARUS ) {
		botUcmd->botCmds.attack = true;
		return;
	}

	if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON ) {
		return;
	}

	if ( gunTargetEntNum > -1 && gunTargetEntNum < MAX_CLIENTS ) {
		if ( botWorld->clientInfo[ gunTargetEntNum ].team == botInfo->team && !botWorld->gameLocalInfo.inWarmup ) { 
			return;
		}
	} //mal: hold your fire! have a friendly in the way.

	if ( timeOnTarget > botWorld->gameLocalInfo.time ) { //mal: haven't had time enough to "react" to this threat...
		return;
	}

	if ( !botInfo->proxyInfo.weaponIsReady && botInfo->proxyInfo.weapon == TANK_GUN ) { //mal: strogg hyperblaster will report not ready between shots, which can confuse the bots
		return;
	}

	if ( botInfo->proxyInfo.weapon == PERSONAL_WEAPON ) {
		if ( enemyInfo.enemyInfont ) {
			if ( botInfo->weapInfo.weapon == ROCKET ) {
				if ( botInfo->targetLocked ) { 
                    botUcmd->botCmds.attack = true;
				}
			} else {
				botUcmd->botCmds.attack = true;
			}
		}
		return;
	}

	if ( botInfo->proxyInfo.weapon == LAW ) {
 		if ( botVehicleInfo->type == TROJAN ) {
			if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
				proxyInfo_t enemyVehicle;
				GetVehicleInfo( botWorld->clientInfo[ enemy ].proxyInfo.entNum, enemyVehicle );

				if ( enemyVehicle.type >= ICARUS ) {
					if ( botInfo->targetLocked ) {
						botUcmd->botCmds.attack = true;
					}
				} else {
					botUcmd->botCmds.attack = true;
				}
			} else {
				botUcmd->botCmds.attack = true;
			}
		} else {
			if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
				if ( botInfo->targetLocked ) {
					botUcmd->botCmds.attack = true;
				}
			}
		}
		return;
	}

	if ( botInfo->proxyInfo.weapon == ROCKETS ) {
		if ( botVehicleInfo->isAirborneVehicle ) {
			minFOV = 0.95f;
		}

		dir = botWorld->clientInfo[ enemy ].origin - botVehicleInfo->origin;
        dir.NormalizeFast(); 
		if ( dir * botVehicleInfo->axis[ 0 ] > minFOV ) { //mal: we have them in our sights - FIRE!
			botUcmd->botCmds.attack = true;
		}
		return;
	}

	if ( botInfo->proxyInfo.hasTurretWeapon ) {
		if ( botVehicleInfo->type > ICARUS && botInfo->proxyInfo.weapon == MINIGUN ) {
			botUcmd->botCmds.attack = true;
		} else {
			dir = botWorld->clientInfo[ enemy ].origin - botInfo->proxyInfo.weaponOrigin;
			dir.NormalizeFast(); 
			if ( dir * botInfo->proxyInfo.weaponAxis[0] > minFOV ) { //mal: we have them in our sights - FIRE!
				botUcmd->botCmds.attack = true;
			}
		}
	} else {
		if ( InFrontOfVehicle( botInfo->proxyInfo.entNum, botWorld->clientInfo[ enemy ].origin ) ) {
			botUcmd->botCmds.attack = true; //mal: they're in front of us - FIRE!
		}
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY && botVehicleInfo->driverEntNum == botNum && botInfo->proxyInfo.weapon != MINIGUN ) { //mal: have a delay between shots for low skill bots.
		if ( botUcmd->botCmds.attack == true ) {
			timeOnTarget = botWorld->gameLocalInfo.time + ( ( botVehicleInfo->isAirborneVehicle ) ? 9500 : 5500 );
		}
	}
}


/*
================
idBotAI::Bot_VehicleFindBetterEnemy
================
*/
bool idBotAI::Bot_VehicleFindBetterEnemy() {
	int i;
	int entClientNum = enemy;
	float dist;
	float entDist;
	proxyInfo_t vehicleInfo;
	proxyInfo_t enemyVehicleInfo;
	idVec3 vec;

	if ( enemy == -1 ) { //mal: we lost our enemy for some reason, so just skip finding a new one til next frame.
		return false;
	}

	if ( ignoreNewEnemiesWhileInVehicleTime > botWorld->gameLocalInfo.time ) {
		return false;
	}

	if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: if we're attacking the MCP, its the priority.
		if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum == botWorld->botGoalInfo.botGoal_MCP_VehicleNum ) {
			return false;
		}
	}

	GetVehicleInfo( botWorld->clientInfo[ enemy ].proxyInfo.entNum, enemyVehicleInfo );

	if ( enemyVehicleInfo.entNum > 0 && ( enemyVehicleInfo.type == ANANSI || enemyVehicleInfo.type == HORNET ) ) { //mal: stay in dogfights!
		return false;
	}

	const clientInfo_t& enemyPlayerInfo = botWorld->clientInfo[ enemy ];

    vec = enemyPlayerInfo.origin - botInfo->origin;
	entDist = vec.LengthSqr();

	if ( !enemyInfo.enemyVisible ) { //mal: if we can't see our current enemy, more likely to attack a visible enemy.
		entDist = idMath::INFINITY;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO && !botWorld->botGoalInfo.gameIsOnFinalObjective && ( !enemyPlayerInfo.isBot || enemyPlayerInfo.isActor ) ) { //mal: dont worry about keeping our human target in training mode, unless its the final obj...
		entDist += Square( TRAINING_MODE_RANGE_ADDITION );
	}

	bool curEnemyNotInVehicle = false;

	if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE && !ClientIsDefusingOurTeamCharge( enemy ) && !vehicleEnemyWasInheritedFromFootCombat ) { //mal: if our current enemy is on foot, more likely to pick a better target. Unless they're defusing our charge, or an enemy we jumped in this vehicle for, then we artificially raise their importance.
		curEnemyNotInVehicle = true;
	}
	
	numVisEnemies = 1; //mal: our current enemy is always visible to us

	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( i == botNum ) {
			continue; //mal: dont try to fight ourselves!
		}

		if ( i == enemy ) { //mal: ignore an enemy we already have
			continue;
		}

		if ( EnemyIsIgnored( i ) ) {
			continue; //mal: dont try to fight someone we've flagged to ignore for whatever reason!
		}

//mal: if we're in the middle of a critical obj, dont go looking for trouble, unless they're shooting us!
		if ( Client_IsCriticalForCurrentObj( botNum, -1.0f ) && ( botInfo->lastAttacker != i || botInfo->lastAttackerTime + 3000 < botWorld->gameLocalInfo.time ) && !ClientIsDefusingOurTeamCharge( i ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.isNoTarget ) {
			continue;
		} //mal: dont target clients that have notarget set - this is useful for debugging, etc.

		if ( playerInfo.isDisguised && playerInfo.disguisedClient != botNum ) {
			continue; //mal: won't "see" disguised clients, unless they look like us!
		}

		if ( playerInfo.inLimbo ) {
			continue;
		}

		if ( playerInfo.isActor ) {
			continue;
		}

		if ( playerInfo.invulnerableEndTime > botWorld->gameLocalInfo.time ) {
			continue; //mal: ignore revived/just spawned in clients - get the ppl around them!
		}

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		if ( playerInfo.team == botInfo->team ) {
			continue;
		}

		if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO && !botWorld->botGoalInfo.gameIsOnFinalObjective && !playerInfo.isBot && enemyPlayerInfo.isBot ) { //mal: dont worry about human targets in training mode if we have a bot one, unless its the final obj...
			continue;
		}

		if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: st00pid bot - your not smart enough to pick your targets wisely
			if ( enemyVehicleInfo.entNum > 0 && enemyInfo.enemyVisible ) {

				if ( !( enemyVehicleInfo.flags & PERSONAL ) && !( enemyVehicleInfo.flags & WATER ) && ( enemyVehicleInfo.isAirborneVehicle && enemyVehicleInfo.xyspeed > 900.0f && entDist > Square( 2000.0f ) ) ) {
                    if ( playerInfo.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE && !Client_IsCriticalForCurrentObj( i, 1500.0f ) && !ClientHasObj( i ) ) {
						continue;
					} //mal: dont worry about an enemy in a vehicle if the vehicle is far away, moving too fast, is not a real threat
				}
			}
		} //mal: if our current enemy is in a vehicle, and this guy isn't, and this guy doesn't have an obj, or isn't important, hes not worth fighting.

		bool enemyIsInAirAttackVehicle = false;

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: pick the driver of a vehicle as the target, NOT passengers, unless there is no driver - then kill whoever.
			
			GetVehicleInfo( playerInfo.proxyInfo.entNum, vehicleInfo );

			if ( vehicleInfo.driverEntNum != i && vehicleInfo.driverEntNum != -1 ) {
				continue;
			}

			if ( vehicleInfo.type == ANANSI || vehicleInfo.type == HORNET ) {
				enemyIsInAirAttackVehicle = true;
			} else {
				vec = vehicleInfo.origin - botInfo->origin;				

				if ( vehicleInfo.xyspeed > 600.0f && vec.LengthSqr() > Square( 1900.0f ) && !InFrontOfVehicle( vehicleInfo.entNum, botInfo->origin ) ) { //mal: if they're in a mad dash away from us, forget about them!
					continue;
				}
			}
		}

		vec = playerInfo.origin - botInfo->origin;
		dist = vec.LengthSqr();

		if ( !enemyIsInAirAttackVehicle ) {
			if ( dist > Square( ENEMY_SIGHT_BUSY_DIST * 2.0f ) ) {
				continue;
			}
		}

		float tempDist = entDist;

		if ( curEnemyNotInVehicle && playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			tempDist += Square( 3000.0f );
		}

		if ( !enemyIsInAirAttackVehicle ) {
			if ( dist > tempDist ) {
				continue;
			}
		} else {
			if ( dist > Square( AIRCRAFT_ATTACK_DIST ) ) {
				continue;
			}
		}

		if ( !ClientIsVisibleToBot ( i, true, false ) ) {
			continue;
		}

		if ( Client_IsCriticalForCurrentObj( i, 1500.0f ) && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: if a critical class, get high priority
			dist = Square( 600.0f );
			ignoreNewEnemiesWhileInVehicleTime = botWorld->gameLocalInfo.time + IGNORE_NEW_ENEMIES_TIME;
		}

		if ( ClientHasObj( i ) && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: if have docs, get HIGHER priority.
			dist = Square( 500.0f );
			ignoreNewEnemiesWhileInVehicleTime = botWorld->gameLocalInfo.time + IGNORE_NEW_ENEMIES_TIME;
		}

		if ( ClientIsDefusingOurTeamCharge( i ) && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: if defusing our charge, get HIGHER priority.
			dist = Square( 100.0f );
			ignoreNewEnemiesWhileInVehicleTime = botWorld->gameLocalInfo.time + IGNORE_NEW_ENEMIES_TIME;
		}

		if ( botWorld->botGoalInfo.mapHasMCPGoal && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
			if ( playerInfo.proxyInfo.entNum == botWorld->botGoalInfo.botGoal_MCP_VehicleNum ) {
				dist = Square( 400.0f );
				ignoreNewEnemiesWhileInVehicleTime = botWorld->gameLocalInfo.time + IGNORE_NEW_ENEMIES_TIME;
			}
		}

		if ( enemyIsInAirAttackVehicle && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: dont ignore a chance to dogfight!
			dist = Square( 100.0f );
			ignoreNewEnemiesWhileInVehicleTime = botWorld->gameLocalInfo.time + IGNORE_NEW_ENEMIES_TIME;
		}

		numVisEnemies++;
		entClientNum = i;
		entDist = dist;
	}

	if ( entClientNum != enemy ) {
		enemy = entClientNum;
		enemySpawnID = botWorld->clientInfo[ entClientNum ].spawnID;

		enemyInfo.enemy_FS_Pos = botWorld->clientInfo[ entClientNum ].origin;
		enemyInfo.enemy_LS_Pos = enemyInfo.enemy_FS_Pos;

		enemyInfo.enemyLastVisTime = botWorld->gameLocalInfo.time;

		enemyAcquireTime = botWorld->gameLocalInfo.time;

		bot_FS_Enemy_Pos = botInfo->origin;
		bot_LS_Enemy_Pos = bot_FS_Enemy_Pos;

		VEHICLE_COMBAT_AI_SUB_NODE = NULL; //mal: reset the bot's combat AI node and movement state.
		COMBAT_MOVEMENT_STATE = NULL;
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_ShouldVehicleChaseHiddenEnemy
================
*/
bool idBotAI::Bot_ShouldVehicleChaseHiddenEnemy() {

	proxyInfo_t enemyVehicleInfo;
	idVec3 vec;
	chasingEnemy = false;

	if ( ClientHasObj( botNum ) || Client_IsCriticalForCurrentObj( botNum, -1.0f ) ) { //mal: never chase if we are important!
		return false;
	}

	if ( botVehicleInfo->driverEntNum != botNum ) { //mal: we dont get the choice to chase someone if we're not the driver!
		return false;
	}

	if ( botVehicleInfo->health <= ( botVehicleInfo->maxHealth / 4 ) && !ClientHasObj( enemy ) ) { //mal: if we're in pretty bad shape, randomly decide to sometimes give up the fight....
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
            return false;
		}
	}

	if ( ( botVehicleInfo->flags & PERSONAL ) || botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON && !ClientHasObj( enemy ) ) {
		return false;
	} //mal: if we dont have a vehicle weapon to fight with, or we're in a personal transport, dont go chasing ppl.

	if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {

		GetVehicleInfo( botWorld->clientInfo[ enemy ].proxyInfo.entNum, enemyVehicleInfo );

		if ( enemyVehicleInfo.inWater && !botVehicleInfo->isAirborneVehicle || !( botVehicleInfo->flags & WATER ) ) {
			return false;
		} //mal: hes in the water, but we can't chase him, so forget him!

		if ( !ClientHasObj( enemy ) && !Client_IsCriticalForCurrentObj( enemy, 2000.0f ) ) {
            if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
				return false;
			}
		}
	}

	if ( AIStack.STACK_AI_NODE != NULL && AIStack.isPriority != false && !ClientHasObj( enemy ) ) {
		return false;
	} // we have something important on our mind, and cant chase ATM - UNLESS they have the obj! Always chase them!

	vec = enemyInfo.enemy_LS_Pos - botInfo->origin;
	
	if ( vec.LengthSqr() > Square( ENEMY_CHASE_DIST ) ) {
		return false;
	} // too far away to chase

	vec = enemyInfo.enemy_LS_Pos - botWorld->clientInfo[ enemy ].origin;

	if ( vec.LengthSqr() > Square( 900.0f ) && !ClientHasObj( enemy ) && !Client_IsCriticalForCurrentObj( enemy, 1500.0f ) ) { // unless he's REALLY close or a threat, sometimes we'll stop the chase, just because....
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
            return false;
		}
	}

	chasingEnemy = true;

	if ( ClientHasObj( enemy ) ) {
		chaseEnemyTime = botWorld->gameLocalInfo.time + 30000;
	} else if ( Client_IsCriticalForCurrentObj( enemy, 1500.0f ) ) {
		chaseEnemyTime = botWorld->gameLocalInfo.time + 15000;
	} else {
		chaseEnemyTime = botWorld->gameLocalInfo.time + 7000;
	}

	return true;
}

/*
==================
idBotAI::Bot_VehicleCanRamClient

See if the vehicle can ram the client in question, in the vehicle the bot is in.
==================
*/
bool idBotAI::Bot_VehicleCanRamClient( int clientNum ) {
	int areaNum;
	proxyInfo_t enemyVehicleInfo;
	idVec3 vec;
	idVec3 enemyOrg;

	if ( botVehicleInfo->type != HOG && botVehicleInfo->health < ( botVehicleInfo->maxHealth / 4 ) && botWorld->clientInfo[ clientNum ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		return false;
	} //mal: most vehicles can't handle ramming other vehicles while damaged - clients on foot are ALWAYS ok tho! >:-D

	if ( botVehicleInfo->flags & WATER || botVehicleInfo->inWater || botVehicleInfo->type == GOLIATH || botVehicleInfo->type == DESECRATOR || botVehicleInfo->flags & PERSONAL || botVehicleInfo->flags & AIR ) {
		return false;
	}

	if ( botVehicleInfo->type != HOG && botVehicleInfo->forwardSpeed < 100.0f && enemyInfo.enemyDist < 700.0f ) {
		return false;
	}

	if ( botWorld->clientInfo[ clientNum ].proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {

		enemyOrg = botWorld->clientInfo[ clientNum ].origin;
		areaNum = botWorld->clientInfo[ clientNum ].areaNumVehicle;

		if ( botWorld->clientInfo[ clientNum ].inWater || !botWorld->clientInfo[ enemy ].inPlayZone ) {
			return false;
		}

		if ( !botWorld->clientInfo[ clientNum ].hasGroundContact ) {
			return false;
		}
	} else {
		GetVehicleInfo( botWorld->clientInfo[ clientNum ].proxyInfo.entNum, enemyVehicleInfo );

		enemyOrg = enemyVehicleInfo.origin;
		areaNum = enemyVehicleInfo.areaNumVehicle;

		if ( enemyVehicleInfo.isAirborneVehicle ) {
			return false;
		}

		if ( enemyVehicleInfo.isEmpty ) {
			return false;
		}

		if ( !enemyVehicleInfo.inPlayZone ) {
			return false;
		}

		if ( enemyVehicleInfo.inWater ) {
			return false;
		}
	}

	bool canReach = botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, botInfo->areaNumVehicle, botVehicleInfo->origin, enemyOrg );

	if ( !canReach ) {
		return false;
	}

	return true;
}

/*
==================
idBotAI::Bot_PickVehicleChaseType

Decide how we'll chase our enemy, based on our situation, and what the enemy has and is doing.

NOTE: just dont have the bloody time to flesh this out as much as I'd like ATM - aren't milestones great?!
==================
*/
void idBotAI::Bot_PickVehicleChaseType() {
	VEHICLE_COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Vehicle_ChaseEnemy;
	return;	
}

/*
==================
idBotAI::Bot_PickBestVehicleWeapon

See what the best weapon is that we have at our disposal - we'll change our position in this vehicle if need be to get access to a ( better ) weapon.
==================
*/
void idBotAI::Bot_PickBestVehicleWeapon() {

	bool enemyInAirborneVehicle = false;
	bool enemyInGroundVehicle = false;
	bool needMovementUpdate = false;
	bool enemyOnFoot = false;
	bool hasGunnerSeatOpen;
	bool enemyReachable = false;

	if ( weapSwitchTime > botWorld->gameLocalInfo.time ) {
		return;
	}

	if ( botVehicleInfo->type == ICARUS ) {
		return;
	}

	proxyInfo_t enemyVehicleInfo;

	idVec3 vec = botWorld->clientInfo[ enemy ].origin - botInfo->origin;

	float dist = vec.LengthSqr();

	hasGunnerSeatOpen = VehicleHasGunnerSeatOpen( botInfo->proxyInfo.entNum );

	if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
        GetVehicleInfo( botWorld->clientInfo[ enemy ].proxyInfo.entNum, enemyVehicleInfo );

		if ( enemyVehicleInfo.isAirborneVehicle ) {
			enemyInAirborneVehicle = true;
		} else {
			enemyInGroundVehicle = true;
		}
	} else {
		enemyOnFoot = true;
	}

	Bot_SetupVehicleMove( vec3_zero, enemy, ACTION_NULL );

    if ( botAAS.hasPath && botAAS.path.travelTime < Bot_ApproxTravelTimeToLocation( botInfo->origin, botWorld->clientInfo[ enemy ].origin, true ) * TRAVEL_TIME_MULTIPLY ) {
		enemyReachable = true;
	}

	switch( botVehicleInfo->type ) {

		case TITAN:
			if ( botVehicleInfo->driverEntNum == botNum ) {
				if ( enemyInAirborneVehicle && enemyInfo.enemyDist <= TANK_MINIGUN_RANGE && hasGunnerSeatOpen ) {
					botUcmd->botCmds.becomeGunner = true; 
					needMovementUpdate = true;
					break;
				}
			} else {
				if ( ( enemyInAirborneVehicle && enemyInfo.enemyDist > TANK_MINIGUN_RANGE && botVehicleInfo->driverEntNum == -1 ) || !enemyInAirborneVehicle ) {
					botUcmd->botCmds.becomeDriver = true;
					needMovementUpdate = true;
					break;
				}
			}

			break;

		case TROJAN:
			if ( botInfo->proxyInfo.weapon == LAW ) {
				if ( botVehicleInfo->driverEntNum == -1 ) {
					if ( ( !enemyInAirborneVehicle && !enemyInGroundVehicle ) || enemyInfo.enemyDist < 700.0f || enemyInfo.enemyDist > WEAPON_LOCK_DIST ) {
                        botUcmd->botCmds.becomeDriver = true; //mal: using the LAW to lock onto our vehicle enemy makes sense ( the drivers MG is a joke in this case ).
						needMovementUpdate = true;
						break;
					}
				}
			}

			if ( botVehicleInfo->driverEntNum == botNum ) {
				if ( ( enemyInAirborneVehicle || enemyInGroundVehicle ) && hasGunnerSeatOpen && dist < Square( WEAPON_LOCK_DIST ) ) {
					botUcmd->botCmds.becomeGunner = true; //mal: using the LAW to lock onto our airborne enemy makes sense ( the drivers MG is a joke in this case ).
					needMovementUpdate = true;
					break;
				}
			}

			if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON ) {
				if ( botVehicleInfo->driverEntNum == -1 ) {
					botUcmd->botCmds.becomeDriver = true;
					needMovementUpdate = true;
				} else if ( VehicleHasGunnerSeatOpen( botVehicleInfo->entNum ) ) {
					botUcmd->botCmds.becomeGunner = true;
					needMovementUpdate = true;
				}
				break;
			}//mal: if we're the driver, stay as the driver!

			break;

		case BADGER:
			if ( botVehicleInfo->driverEntNum == botNum ) { //mal: we're the driver, if the turret is available, we may jump into it to fight enemies.
				if ( hasGunnerSeatOpen && vehicleDriverTime < botWorld->gameLocalInfo.time && enemyReachable && enemyInfo.enemyDist > 700.0f && enemyVehicleInfo.type != HOG ) {
                    if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
						vehicleDriverTime = botWorld->gameLocalInfo.time + 15000;
						break;
					}
					
					if ( ( enemyInAirborneVehicle || ( enemyInfo.enemyDist < 3000.0f && enemyInfo.enemyDist > 700.0f ) ) && vehicleGunnerTime + 30000 < botWorld->gameLocalInfo.time && enemyInfo.enemyVisible ) { //mal: dont do this if enemy too far away, or its been too soon since we last did this.
						botUcmd->botCmds.becomeGunner = true;
						vehicleGunnerTime = botWorld->gameLocalInfo.time + 15000;
						needMovementUpdate = true;
						break;
					}
				}
			}

			if ( botInfo->proxyInfo.weapon == PERSONAL_WEAPON ) {
				if ( hasGunnerSeatOpen && botVehicleInfo->driverEntNum != -1 ) {
					botUcmd->botCmds.becomeGunner = true;
					needMovementUpdate = true;
					break;
				} //mal: our gunner may have been killed, or maybe he jumped out some time ago - take his place.

				if ( botVehicleInfo->driverEntNum == -1 ) {
                    botUcmd->botCmds.becomeDriver = true;
					needMovementUpdate = true;
					break;
				}
			}

			if ( botInfo->proxyInfo.weapon == MINIGUN ) {
				if ( botVehicleInfo->driverEntNum == -1 ) {
					if ( vehicleGunnerTime < botWorld->gameLocalInfo.time || !enemyInfo.enemyVisible ) { //mal: even if we had a driver and he bailed, become the driver for a better shot, then can go back to the gun.
						botUcmd->botCmds.becomeDriver = true;
						needMovementUpdate = true;
						break;
					}
				}
			}

			if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON && botVehicleInfo->driverEntNum != botNum ) {
				if ( botVehicleInfo->driverEntNum == -1 ) {
					botUcmd->botCmds.becomeDriver = true;
					needMovementUpdate = true;
				} else if ( VehicleHasGunnerSeatOpen( botVehicleInfo->entNum ) ) {
					botUcmd->botCmds.becomeGunner = true;
					needMovementUpdate = true;
				} else if ( botVehicleInfo->hasFreeSeat ) {
					needMovementUpdate = true;
					botUcmd->botCmds.activate = true; //mal: rotate to one of the back seats.
				}
			}//mal: if we're the driver, stay as the driver!

			break;

		case PLATYPUS:
			if ( botVehicleInfo->driverEntNum == botNum ) { //mal: we're the driver, if the turret is available, we may jump into it to fight enemies.
				if ( hasGunnerSeatOpen ) {
					if ( enemyInfo.enemyDist < 1500.0f && vehicleGunnerTime + 30000 < botWorld->gameLocalInfo.time && enemyInfo.enemyVisible ) { //mal: dont do this if enemy too far away, or its been too soon since we last did this.
						botUcmd->botCmds.becomeGunner = true;
						vehicleGunnerTime = botWorld->gameLocalInfo.time + 10000;
						needMovementUpdate = true;
					}
				} //mal: if we have a gunner, we'll try to get a better shot for him, so that he can cut down our enemies.
			}

			if ( botInfo->proxyInfo.weapon == MINIGUN ) {
				if ( botVehicleInfo->driverEntNum == -1 ) {
					if ( vehicleGunnerTime < botWorld->gameLocalInfo.time || !enemyInfo.enemyVisible ) { //mal: even if we had a driver and he bailed, become the driver for a better shot, then can go back to the gun.
						botUcmd->botCmds.becomeDriver = true;
						needMovementUpdate = true;
					}
				}
			}

			break;

		case DESECRATOR:
			if ( botVehicleInfo->driverEntNum == botNum ) {
				if ( enemyInAirborneVehicle && enemyInfo.enemyDist <= TANK_MINIGUN_RANGE && hasGunnerSeatOpen ) {
					botUcmd->botCmds.becomeGunner = true; 
					needMovementUpdate = true;
					break;
				}
			} else {
				if ( ( enemyInAirborneVehicle && enemyInfo.enemyDist > TANK_MINIGUN_RANGE && botVehicleInfo->driverEntNum == -1 ) || !enemyInAirborneVehicle ) {
					botUcmd->botCmds.becomeDriver = true;
					needMovementUpdate = true;
					break;
				}
			}

			break;

		case HOG:
			if ( botInfo->proxyInfo.weapon == MINIGUN ) {
				if ( botVehicleInfo->driverEntNum == -1 ) {
					if ( vehicleGunnerTime < botWorld->gameLocalInfo.time || !enemyInfo.enemyVisible || botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE && !enemyInAirborneVehicle ) { //mal: even if we had a driver and he bailed, become the driver for a better shot, then can go back to the gun.
						botUcmd->botCmds.becomeDriver = true;
						needMovementUpdate = true;
					} //mal: we'll prefer to ram our enemies who are in vehicles
				}
			}

			if ( botVehicleInfo->driverEntNum == botNum && ( !Bot_VehicleCanRamClient( enemy ) || VEHICLE_COMBAT_MOVEMENT_STATE == NULL || combatMoveType != VEHICLE_RAM_ATTACK_MOVEMENT ) ) {
				if ( ( enemyInAirborneVehicle || ( enemyInfo.enemyDist < 3000.0f && enemyInfo.enemyDist > 700.0f ) ) && vehicleGunnerTime + 30000 < botWorld->gameLocalInfo.time && enemyInfo.enemyVisible ) {
					botUcmd->botCmds.becomeGunner = true;
					vehicleGunnerTime = botWorld->gameLocalInfo.time + 10000;
					needMovementUpdate = true;
				}
			}

			break;

		case ANANSI:
		case HORNET:
			if ( botInfo->proxyInfo.weapon == MINIGUN ) {
				if ( botVehicleInfo->driverEntNum == -1 ) {
					botUcmd->botCmds.becomeDriver = true; //mal: our coward of a pilot bailed on us... take the controls!
					needMovementUpdate = true;
				}
			}

			if ( botVehicleInfo->driverEntNum == botNum ) {
				if ( botInfo->proxyInfo.weapon == LAW ) { //mal: law is better against other vehicles
					if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {
						botUcmd->botCmds.switchVehicleWeap = true;
					} else {
						if ( botInfo->proxyInfo.weaponIsReady == false && rocketTime < botWorld->gameLocalInfo.time && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: use rockets if LAW outta charge
							rocketTime = botWorld->gameLocalInfo.time + 2000;
							botUcmd->botCmds.switchVehicleWeap = true;
						}
					}
				}

				if ( botInfo->proxyInfo.weapon == ROCKETS ) { //mal: rockets are nice against slow moving ground targets.
					if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE && rocketTime < botWorld->gameLocalInfo.time ) {
						botUcmd->botCmds.switchVehicleWeap = true;
					}
				}
			}

			break;		
	}

	if ( needMovementUpdate ) {
		COMBAT_MOVEMENT_STATE = NULL;
		VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
		vehicleUpdateTime = botWorld->gameLocalInfo.time + 500;
	}

	weapSwitchTime = botWorld->gameLocalInfo.time + 1500;
}

/*
==================
idBotAI::VehicleHasGunnerSeatOpen
==================
*/
bool idBotAI::VehicleHasGunnerSeatOpen( int entNum ) {
	bool hasSeat = true;
	int i;
	int j = 0;
	int numSeats = 1;
	proxyInfo_t vehicleInfo;
	botVehicleWeaponInfo_t weaponType = MINIGUN;
	GetVehicleInfo( entNum, vehicleInfo );

	if ( !vehicleInfo.hasFreeSeat ) { //mal: that was fast....
		return false;
	}

	if ( vehicleInfo.type == MCP ) {
		return true;
	}

	if ( vehicleInfo.type == BUFFALO ) { //mal: buffalo actually has 2 gunner seats, where most vehicles only have 1.
		numSeats = 3;
	}

	if ( vehicleInfo.type == TROJAN ) {
		weaponType = LAW;
	}

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum && botVehicleInfo == NULL ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t &playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.team != botInfo->team ) {
			continue;
		}

		if ( playerInfo.proxyInfo.entNum != entNum ) {
			continue;
		}

		if ( playerInfo.proxyInfo.weapon != weaponType ) {
			continue;
		}

		j++;

		if ( j >= numSeats ) {
            hasSeat = false;
			break;
		}
	}

	return hasSeat;
}

/*
==================
idBotAI::Bot_VehicleCanAttackEnemy
==================
*/
bool idBotAI::Bot_VehicleCanAttackEnemy( int clientNum ) {

	if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON ) {
		if ( !VehicleHasGunnerSeatOpen( botInfo->proxyInfo.entNum ) && botVehicleInfo->driverEntNum != botNum ) { 
			return false;
		}
	} else if ( botInfo->proxyInfo.weapon == PERSONAL_WEAPON ) { //mal: if we have our personal weapon, we can't control the vehicle in question, so can only attack if client is vis to us, and we have ammo to attack with.
		if ( !InFrontOfClient( botNum, botWorld->clientInfo[ clientNum ].origin ) && !VehicleHasGunnerSeatOpen( botInfo->proxyInfo.entNum ) ) {
			return false;
		}

		if ( !botInfo->weapInfo.hasNadeAmmo && !botInfo->weapInfo.sideArmHasAmmo && !botInfo->weapInfo.primaryWeapHasAmmo ) {
			return false;
		} //mal: dont have any ammo for the 3 weapons the bots can use while riding in a transport, so just sit there.
	} else if ( botVehicleInfo->type == MCP && botVehicleInfo->driverEntNum == botNum ) {
#ifdef _XENON
		return false;
#endif
		if ( !InFrontOfVehicle( botInfo->proxyInfo.entNum, botWorld->clientInfo[ clientNum ].origin ) ) {
			return false;
		} //mal: dont fight ppl if we're driving the mcp, and they're not in front of us - just get the MCP to the outpost!
	}

	return true;	
}

/*
================
idBotAI::ClientIsAudibleToVehicle

Cheaply tells us if the client is question is making enough noise for this vehicle
to "notice" them and possibly consider them an enemy.
================
*/
bool idBotAI::ClientIsAudibleToVehicle( int clientNum ) {
	heardClient = -1; // reset this every frame.

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) { // stupid bot - you can't hear anyone!
		return false;
	}

	if ( botWorld->botGoalInfo.teamRetreatInfo[ botInfo->team ].retreatTime > botWorld->gameLocalInfo.time ) {
		return false;
	}

	const clientInfo_t& client = botWorld->clientInfo[ clientNum ];

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO && !botWorld->botGoalInfo.gameIsOnFinalObjective && !client.isBot ) { //mal: dont worry about hearing human targets in training mode, unless its the final obj...
		return false;
	}

	float hearingRange = ( client.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) ? PLAYER_HEARING_DIST : VEHICLE_HEARING_DIST;

	idVec3 vec = client.origin - botInfo->origin;
	float dist = vec.LengthSqr();

	float distTemp = vec.LengthFast();

	if ( client.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE && client.weapInfo.weapon == SNIPERRIFLE && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
		hearingRange = SNIPER_HEARING_DIST;
	}

	if ( client.weapInfo.isFiringWeap && dist < Square( hearingRange ) ) {
		heardClient = clientNum;
		return true;
	}

	if ( dist < Square( FOOTSTEP_DIST )  ) {
		heardClient = clientNum;
		return true;
	}

	if ( ClientHasObj( clientNum ) && ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY || client.isInRadar ) ) {
		return true;
	}

	if ( client.isInRadar && dist < Square( 1500.0f ) && ( client.abilities.gdfStealthToRadar == false || client.classType != COVERTOPS ) ) { //mal_TODO: test me!!!
		return true;
	}

	if ( ClientIsMarkedForDeath( clientNum, false ) ) {
		return true;
	}

	if ( ClientIsDangerous( clientNum ) ) { //mal: we're "aware" of campers, and will kill them if they're vis to us.
		heardClient = clientNum;
		return true;
	}

	if ( botInfo->lastAttacker == clientNum && botInfo->lastAttackerTime + 3000 > botWorld->gameLocalInfo.time ) {
		return true;
	}

	if ( ( client.targetLockEntNum == botNum || client.targetLockEntNum == botInfo->proxyInfo.entNum ) && client.targetLockTime + 3000 < botWorld->gameLocalInfo.time ) {
		return true;
	}

	return false;
}

