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
idBotAI::Bot_FindEnemy

We'll sort thru the clients, and ignore certain clients if we're too busy
to be buggered (carrying obj, planting/hacking, etc) or they're not valid enemies
(in disguise, hidden by smoke, etc).
================
*/
bool idBotAI::Bot_FindEnemy( int ignoreClientNum ) {
	bool hasAttackedMate;
	bool hasAttackedCriticalMate;
	bool hasObj;
	bool isTouchingItems;
	bool isDefusingOurBomb;
	bool inFront;
	bool botGotShotRecently;
	bool botIsBigShot;
	bool audible;
	bool isVisible;
	bool isFacingUs;
	bool isFiringWeapon;
	bool isNearOurObj;
	bool hasBeenNarced;
	int i;
	int infantry = 0;
	int vehicle = 0;
	int aircraft = 0;
	int heardPriority;
	int entClientNum = -1;
	int heardClientNum = -1;
	float dist;
	float botSightDist = botWorld->botGoalInfo.botSightDist;
	float entDist = idMath::INFINITY;
	float heardDist = idMath::INFINITY;
	proxyInfo_t vehicleInfo;
	idVec3 vec;

	hammerTime = false;
	hammerClient = -1;
	testFireShot = false;
	numVisEnemies = 0;

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
	
	if ( Bot_ShouldIgnoreEnemies() ) {
		return false;
	}
	
#ifdef _XENON
	if ( briefPlayerTime > botWorld->gameLocalInfo.time ) {
		return false;
	}
#endif

	idVec3 turretLoc;

	if ( Bot_IsUnderAttackByAPT( turretLoc ) && Bot_IsAttackingDeployables() ) {
		return false;
	}

	if ( botInfo->weapInfo.covertToolInfo.entNum != 0 && botInfo->team == STROGG && botInfo->weapInfo.covertToolInfo.clientIsUsing == true ) { //mal: if using the flyer hive, dont worry about enemies.
		return false;
	}

	if ( botWorld->botGoalInfo.teamRetreatInfo[ botInfo->team ].retreatTime > botWorld->gameLocalInfo.time ) {
		botSightDist = 1000.0f;
	}

	botIsBigShot = Bot_CheckCombatExceptions();

	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( ignoreClientNum == i ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( i == botNum ) {
			continue; //mal: dont try to fight ourselves!
		}

		if ( EnemyIsIgnored( i )) {
			continue; //mal: dont try to fight someone we've flagged to ignore for whatever reason!
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.isNoTarget ) {
			continue;
		} //mal: dont target clients that have notarget set - this is useful for debugging, etc.

		if ( playerInfo.inLimbo ) {
			continue;
		}

		if ( playerInfo.isActor ) {
			continue;
		}

		if ( playerInfo.isDisguised && botThreadData.GetBotSkill() == BOT_SKILL_EASY ) {
			continue;
		}

		if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
			if ( Bot_CheckForHumanInteractingWithEntity( i ) == true ) {
				continue;
			}
		}

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: pick the driver of a vehicle as the target, NOT passengers, unless there is no driver - then kill whoever.

			GetVehicleInfo( playerInfo.proxyInfo.entNum, vehicleInfo );

			if ( vehicleInfo.type != MCP ) {
				if ( vehicleInfo.driverEntNum != i && vehicleInfo.driverEntNum != -1 ) {
					continue;
				}

				vec = vehicleInfo.origin - botInfo->origin;

				float vehicleDist = vec.LengthSqr();

				if ( vehicleInfo.xyspeed > 600.0f && vehicleDist > Square( 1900.0f ) && !InFrontOfVehicle( playerInfo.proxyInfo.entNum, botInfo->origin ) ) { //mal: if they're in a mad dash away from us, forget about them!
					continue;
				}

				if ( ( vehicleInfo.type == GOLIATH || vehicleInfo.type == TITAN || vehicleInfo.type == DESECRATOR ) && vehicleInfo.health > ( vehicleInfo.maxHealth / 3 ) ) {
			
					bool canNadeAttack = ( !botInfo->weapInfo.hasNadeAmmo || vehicleDist > Square( GRENADE_ATTACK_DIST ) ) ? false : true;

					if ( botInfo->classType == FIELDOPS ) {
						if ( vehicleDist > Square( GRENADE_ATTACK_DIST ) ) {
							if ( !Bot_HasWorkingDeployable() || !ClassWeaponCharged( AIRCAN ) || Bot_EnemyAITInArea( vehicleInfo.origin ) ) {
								continue;
							}
						} else {
							if ( !canNadeAttack && !ClassWeaponCharged( AIRCAN ) && !botInfo->weapInfo.primaryWeapHasAmmo ) {
								continue;
							}
						}
					} else if ( botInfo->classType == COVERTOPS ) {
						if ( !canNadeAttack && ( botInfo->weapInfo.primaryWeapon != SNIPERRIFLE || !botInfo->weapInfo.primaryWeapHasAmmo ) ) {
							continue;
						}
					} else if ( botInfo->classType == SOLDIER ) {
						if ( !canNadeAttack && ( botInfo->weapInfo.primaryWeapon != ROCKET || !botInfo->weapInfo.primaryWeapHasAmmo ) ) {
							continue;
						}
					} else {
						if ( !canNadeAttack && vehicleDist > Square( INFANTRY_ATTACK_HEAVY_DIST ) && !botInfo->weapInfo.primaryWeapHasAmmo ) {		
							continue;
						}
					}
				}
			} else {
				if ( vehicleInfo.isImmobilized && vehicleInfo.driverEntNum == i ) {
					continue;
				}
			}
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

		hasAttackedCriticalMate = ClientHasAttackedTeammate( i, true, 3000 );

		int attackeMateAwarenessTime = ( playerInfo.isDisguised ) ? 3000 : 9000;
	
		hasAttackedMate = ClientHasAttackedTeammate( i, false, attackeMateAwarenessTime );
		hasObj = ClientHasObj( i );
		isTouchingItems =  ( playerInfo.touchingItemTime + BOT_THINK_DELAY_TIME < botWorld->gameLocalInfo.time ) ? false : true; 
		isDefusingOurBomb = ClientIsDefusingOurTeamCharge( i );
		inFront = InFrontOfClient( botNum, playerInfo.origin );
		isFacingUs = InFrontOfClient( i, botInfo->origin );
		botGotShotRecently = ( botInfo->lastAttackerTime + 3000 < botWorld->gameLocalInfo.time ) ? false : true;
		isFiringWeapon = playerInfo.weapInfo.isFiringWeap;
		isNearOurObj = ( LocationDistFromCurrentObj( botInfo->team, playerInfo.origin ) < 2500.0f ) ? true : false;
		hasBeenNarced = ( playerInfo.covertWarningTime + 5000 < botWorld->gameLocalInfo.time ) ? false : true;
		bool hasAttackedActorsFriend = ClientHasAttackedActorsMate( i, 3000 );

		if ( botIsBigShot && botInfo->classType == MEDIC && !hasObj && !isDefusingOurBomb ) { //mal: medics need to do their jobs, with a LOT less restrictions then others.
			continue;
		}

		if ( botIsBigShot && ( !isFacingUs || !inFront ) && !hasObj && !isNearOurObj && !isDefusingOurBomb ) {
			continue;
		} //mal: if we're trying to do an important obj, dont get into a fight with everyone.

		if ( botInfo->isDisguised ) { //mal: dont attack, unless they have the obj on them, or they shot us, or they're attacking an important mate, or they're defusing our team's bomb!
			if ( !hasObj && ( !botGotShotRecently || ( botInfo->lastAttacker != i && botInfo->lastAttacker != botInfo->disguisedClient ) || botThreadData.random.RandomInt( 100 ) < 75 ) &&
				!hasAttackedCriticalMate && !isDefusingOurBomb && Client_IsCriticalForCurrentObj( i, 2500.0f ) == false ) {
			 	continue;
			}
		}

		if ( botInfo->isDisguised ) {
			if ( !botGotShotRecently && aiState == NBG && nbgType == STALK_VICTIM && nbgTarget == i ) {
				continue;
			}
		}

		vec = playerInfo.viewOrigin - botInfo->viewOrigin;
		dist = vec.LengthSqr();

		if ( botInfo->isActor && ( !hasAttackedActorsFriend && dist > Square( 900.0f ) ) ) {
			continue;
		}

		if ( botWorld->botGoalInfo.teamRetreatInfo[ botInfo->team ].retreatTime > botWorld->gameLocalInfo.time ) { //mal: the bots should try to fall back, not get caught up in combat.
			if ( !inFront && ( !botGotShotRecently || botInfo->lastAttacker != i ) ) {
				continue;
			}

			if ( dist > Square( botSightDist ) ) {
				continue;
			}
		}

        if ( dist > Square( botSightDist ) ) {
			if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo && playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
				if ( dist > Square( WEAPON_LOCK_DIST ) ) {
					continue;
				}
			} else if ( botInfo->classType == FIELDOPS && ClassWeaponCharged( AIRCAN ) && Bot_HasWorkingDeployable() && !Bot_EnemyAITInArea( playerInfo.origin ) ) {
				if ( !LocationVis2Sky( playerInfo.origin ) ) {
					continue;
				}

				if ( dist > Square( WEAPON_LOCK_DIST ) && !ClientIsDangerous( i ) ) {
					continue;
				}

				deployableInfo_t deployableInfo;

				GetDeployableInfo( true, 0, deployableInfo );

				if ( dist > Square( deployableInfo.maxAttackRange ) ) {
					continue;
				}

				bool vis2Sky = LocationVis2Sky( playerInfo.origin );

				if ( ClientIsDangerous( i ) && vis2Sky ) { //mal: we'll slam campers, just because!
					hammerTime = true;
					hammerVehicle = false;
					hammerLocation = playerInfo.origin;
					hammerClient = i;

					if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
						GetVehicleInfo( playerInfo.proxyInfo.entNum, vehicleInfo );

						if ( deployableInfo.type == ROCKET_ARTILLERY ) {
							hammerVehicle = true;
						}

						hammerLocation = vehicleInfo.origin;
						hammerLocation.z += ( vehicleInfo.bbox[ 1 ][ 2 ] - vehicleInfo.bbox[ 0 ][ 2 ] ) * 0.2f;//mal: 20%
					}
				} else if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE && deployableInfo.type == ROCKET_ARTILLERY && vis2Sky ) {
					GetVehicleInfo( playerInfo.proxyInfo.entNum, vehicleInfo );

					if ( vehicleInfo.type != HUSKY && vehicleInfo.type != ICARUS && vehicleInfo.type != PLATYPUS ) {
						float vehicleOffset = botThreadData.GetVehicleTargetOffset( vehicleInfo.type );
						hammerTime = true;
						hammerVehicle = true;
						hammerLocation = vehicleInfo.origin;
						hammerLocation.z += ( vehicleInfo.bbox[ 1 ][ 2 ] - vehicleInfo.bbox[ 0 ][ 2 ] ) * vehicleOffset;
						hammerClient = i;
					} else {
						continue;
					}				
				} else { //mal: need to have some ppl around to make it worth our while....

					int numEnemiesInArea = ClientsInArea( botNum, playerInfo.origin, 1024.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, true, false, false, false );
					int numFriendsInArea = ClientsInArea( botNum, playerInfo.origin, 1024.0f, botInfo->team, NOCLASS, false, true, false, false, false );

					if ( numEnemiesInArea < 2 || numFriendsInArea > 2 ) {
						continue;
					}

					hammerTime = true;
					hammerVehicle = false;
					hammerLocation = playerInfo.origin;
					hammerClient = i;
				}
			} else if ( botInfo->classType == COVERTOPS && botInfo->weapInfo.primaryWeapon == SNIPERRIFLE && botInfo->weapInfo.primaryWeapHasAmmo && !botIsBigShot ) {
				if ( dist > Square( MAX_SNIPER_VIEW_DIST ) ) {
					continue;
				}
			} else {
				if ( aiState == LTG && ltgType == HUNT_GOAL && ltgTarget == i ) {
					continue;
				}

				if ( dist > Square( botSightDist * 2.0f ) || !ClientIsMarkedForDeath( i, false ) ) {
					if ( botInfo->xySpeed == 0.0f && botInfo->lastAttacker == i && botInfo->lastAttackerTime + 100 > botWorld->gameLocalInfo.time ) { //mal: if getting hit, dont just stand there, move around.
						Bot_ResetState( false, true );
					}
					continue;
				}
			}
		}

		if ( playerInfo.isDisguised ) {
			if ( ( playerInfo.disguisedClient != botNum || dist > Square( COVERT_SIGHT_DIST * 2.0f ) ) && !hasAttackedMate && !DisguisedClientIsActingOdd( i ) ) {
				if ( inFront && botThreadData.GetBotSkill() == BOT_SKILL_EXPERT ) {
					if ( ( ( !isTouchingItems || ( playerInfo.health == playerInfo.maxHealth && !playerInfo.weapInfo.primaryWeapNeedsAmmo ) || dist > Square( COVERT_SIGHT_DIST ) ) && ( !hasBeenNarced || dist > Square( COVERT_SIGHT_DIST ) ) ) ) {
						continue; //mal: won't "see" disguised clients, unless they look like us, or theyre in front of us touching our/their items, or someone warned us about them!
					}
				} else {
					continue;
				}
			}
		}

		if ( !ClientHasObj( i ) ) {
            audible = ClientIsAudibleToBot( i ); //mal: if we can hear you, we'll skip the FOV test in the vis check below
		} else {
            audible = true;
			dist = Square( 500.0f ); //mal: if you've got the docs, your our priority target, unless someone else is right on top of us!
		}

		isVisible = ClientIsVisibleToBot( i, !audible, false );

//mal: if client isn't visible, but is audible, see if we should hunt them down - ESPECIALLY if they're attacking a nearby critical teammate or are carrying docs!
		if ( !isVisible ) {
			if ( audible ) {
				heardPriority = Bot_ShouldInvestigateNoise( i );
				if ( heardPriority != 0 ) {
                    if ( dist < Square( heardDist ) ) {
						if ( heardPriority == 2 ) {
							heardDist = Square( 200.0f ); //mal: we heard someone we should worry about, so they get somewhat priority!
						} else if ( heardDist == 3 ) {
							heardDist = Square( 100.0f ); //mal: we heard someone we should REALLY worry about, so they get TOTAL priority!
						} else { 
                            heardDist = dist;
						}
						heardClientNum = i;
					}
				}
			}
			continue;
		}

		if ( botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO || botWorld->botGoalInfo.gameIsOnFinalObjective || botWorld->botGoalInfo.attackingTeam == botInfo->team ) { //mal: don't be too good about picking our targets in training mode if we're the defenders, unless its the final obj....
			if ( isDefusingOurBomb ) {
				dist = Square( 100.0f );
			} 
			
			if ( hasAttackedCriticalMate && inFront && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
				dist = Square( 600.0f ); //mal: will give higher priority to someone attacking a critical mate, if we can see it happening.
			}

			if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
				if ( Client_IsCriticalForCurrentObj( i, 1200.0f ) ) {
					dist = Square( 700.0f ); //mal: if your a critical client, we're more likely to kill you.
				}
			}

			if ( botWorld->botGoalInfo.mapHasMCPGoal ) {
				if ( playerInfo.proxyInfo.entNum == botWorld->botGoalInfo.botGoal_MCP_VehicleNum ) {
					dist = Square( 800.0f );
				}
			} //mal: if your in MCP, you get higher priority then a normal enemy.
		} else {
			if ( !playerInfo.isBot || playerInfo.isActor ) {
				dist += Square( TRAINING_MODE_RANGE_ADDITION );
			}
		}

		numVisEnemies++;

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			if ( vehicleInfo.isAirborneVehicle ) {
				if ( vehicleInfo.type != ICARUS ) {
                    aircraft++;
				} else {
					infantry++; //mal: the icarus doesn't really count as a aircraft threat.
				}
			} else {
				if ( vehicleInfo.type != HUSKY ) {
                    vehicle++; //mal: tanks, water craft, and everything else falls into this group.
				} else {
					infantry++; //mal: ditto for the husky - doesn't really count as much of a vehicle threat.
				}
			}
		} else {
			infantry++;
		}

		if ( dist < entDist ) {
			entClientNum = i;
			entDist = dist;
		}
	}

	if ( entClientNum != -1 ) {

		enemy = entClientNum;
		enemySpawnID = botWorld->clientInfo[ entClientNum ].spawnID;

		if ( hammerClient != -1 && enemy != hammerClient ) {
			hammerTime = false;
		}

		if ( aiState == LTG && ltgType == HUNT_GOAL && ltgTarget == enemy ) {
			enemyIsHuntGoal = true;
		} else {
			enemyIsHuntGoal = false;
		}

		enemyInfo.enemy_FS_Pos = botWorld->clientInfo[ entClientNum ].origin;
		enemyInfo.enemy_LS_Pos = enemyInfo.enemy_FS_Pos;

		bot_FS_Enemy_Pos = botInfo->origin;
		bot_LS_Enemy_Pos = bot_FS_Enemy_Pos;
		enemyInfo.enemyLastVisTime = botWorld->gameLocalInfo.time;

		enemyAcquireTime = botWorld->gameLocalInfo.time;

		Bot_SetAttackTimeDelay( inFront ); //mal: this sets a delay on how long the bot should take to see enemy, based on bot's state.

		COMBAT_AI_SUB_NODE = NULL; //mal: reset the bot's combat AI node
		COMBAT_MOVEMENT_STATE = NULL;
		combatNBGType = NO_COMBAT_TYPE;

		if ( numVisEnemies > 2 ) { //mal: if a big wave of enemies is incoming, let everyone else know!
			if ( botThreadData.random.RandomInt( 100 ) > 90 ) {
				if ( vehicle > infantry && vehicle > aircraft ) {
                    Bot_AddDelayedChat( botNum, INCOMING_VEHICLE, 1 );
				} else if ( aircraft > infantry ) {
					Bot_AddDelayedChat( botNum, INCOMING_AIRCRAFT, 1 );
				} else {
					Bot_AddDelayedChat( botNum, INCOMING_INFANTRY, 1 );
				}					
			}
		}

		return true;
	} else if ( heardClientNum != -1 ) { //mal: we dont see an enemy, but did we find someone we want to investigate?
		if ( ClientHasObj( heardClientNum ) ) {
            ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY;
		} else if ( aiState == NBG && nbgType == DEFENSE_CAMP ) {
			ltgTime = botWorld->gameLocalInfo.time + 15000;
		} else {
			ltgTime = botWorld->gameLocalInfo.time + 60000;
		}

		aiState = LTG;
		ltgType = HUNT_GOAL;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_HuntGoal;
		ltgTarget = heardClientNum;
		ltgTargetSpawnID = botWorld->clientInfo[ heardClientNum ].spawnID;
	}

	return false;
}

/*
================
idBotAI::ClientIsAudibleToBot

Cheaply tells us if the client is question is making enough noise for this bot
to "notice" them and possibly consider them an enemy.
================
*/
bool idBotAI::ClientIsAudibleToBot( int clientNum ) {
	heardClient = -1; // reset this every frame.

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) { // stupid bot - you can't hear anyone!
		return false;
	}

	if ( botWorld->botGoalInfo.teamRetreatInfo[ botInfo->team ].retreatTime > botWorld->gameLocalInfo.time ) {
		return false;
	}

	if ( ClientIsDefusingOurTeamCharge( clientNum ) ) {
		return true;
	}

	if ( aiState == LTG && ( ltgType == DEFENSE_CAMP_GOAL || ltgType == HUNT_GOAL ) ) {
		return false;
	}

	bool  audible = false;
	float hearingRange = PLAYER_HEARING_DIST;
	const clientInfo_t& client = botWorld->clientInfo[ clientNum ];
	idVec3 vec = client.origin - botInfo->origin;
	float dist = vec.LengthSqr();

	bool skipBattleSenseSoundChecks = false;
	bool inVehicle = ( botVehicleInfo != NULL ) ? true : false;

	if ( !inVehicle ) { //mal: if its far enough away, make sure its actually in the same area, and not a floor above/below us.
		bool botOutSide = LocationVis2Sky( botInfo->origin );
		bool enemyOutSide = LocationVis2Sky( client.origin );

		if ( botOutSide != enemyOutSide && botInfo->areaNum != client.areaNum && idMath::Fabs( vec.z ) > 150.0f ) {
			skipBattleSenseSoundChecks = true;
		}
	}

	if ( !skipBattleSenseSoundChecks ) {

		float weapFireHearingRange = hearingRange;

		if ( client.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE && client.weapInfo.weapon == SNIPERRIFLE && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
			weapFireHearingRange = SNIPER_HEARING_DIST;
		}

		if ( client.weapInfo.isFiringWeap && dist < Square( weapFireHearingRange ) ) {
			heardClient = clientNum;
			audible = true;
		}

		if ( dist < Square( FOOTSTEP_DIST ) && client.xySpeed >= RUNNING_SPEED && botInfo->xySpeed < WALKING_SPEED ) {
			if ( client.abilities.stroggCovertNoFootSteps == false || client.isDisguised || client.classType != COVERTOPS ) {
				heardClient = clientNum;
				audible = true;
			}
		}

		if ( !client.isDisguised && dist < Square( FOOTSTEP_DIST ) && client.isPanting && botInfo->xySpeed < WALKING_SPEED && !botInfo->isPanting && botWorld->gameLocalInfo.botSkill == BOT_SKILL_EXPERT ) {
			audible = true;
			heardClient = clientNum;
		} //mal: REALLY high skilled bots will hear the enemy running around, panting. We check for disguised clients elsewhere.
	}

	if ( ClientHasObj( clientNum ) && ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY || client.isInRadar ) ) {
		audible = true;
	}

	if ( client.isInRadar && dist < Square( 1500.0f ) && ( client.abilities.gdfStealthToRadar == false || client.classType != COVERTOPS ) ) { //mal_TODO: test me!!!
		audible = true;
	}

	if ( ClientIsMarkedForDeath( clientNum, false ) ) {
		audible = true;
	}

	if ( ClientIsDangerous( clientNum ) ) { //mal: we're "aware" of campers, and will kill them if they're vis to us.
		heardClient = clientNum;
		audible = true;
	}

	if ( botInfo->lastAttacker == clientNum && botInfo->lastAttackerTime + 3000 > botWorld->gameLocalInfo.time ) {
		audible = true;
	}

	if ( botVehicleInfo != NULL && client.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		if ( ( client.targetLockEntNum == botNum || client.targetLockEntNum == botInfo->proxyInfo.entNum ) && client.targetLockTime + 3000 < botWorld->gameLocalInfo.time ) {
			audible = true;
		}
	}

	if ( botThreadData.AllowDebugData() ) {
		if ( audible != false ) {
//			common->Printf("I'm aware of client #: %i\n", clientNum );
		}
	}

	return audible;
}


/*
================
idBotAI::ClientIsVisibleToBot

Tells us if the client is question is visible to the bot.
Not the cheapest function to call!
================
*/
bool idBotAI::ClientIsVisibleToBot ( int clientNum, bool useFOV, bool saveEnt ) {
	int entNum = -1;
	trace_t	tr;
	proxyInfo_t vehicleInfo;
	const clientInfo_t& client = botWorld->clientInfo[ clientNum ];
	idVec3 enemyView;

	if ( client.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE && !ClientHasObj( clientNum ) ) {
		if ( ClientObscuredBySmokeToBot( clientNum ) ) {
			return false;
		}
	}

	if ( client.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {
		enemyView = GetPlayerViewPosition( clientNum ); //mal: look them in the eye...
	} else {
		GetVehicleInfo( client.proxyInfo.entNum, vehicleInfo );
        enemyView  = vehicleInfo.origin;
		float enemyVehicleOffset = botThreadData.GetVehicleTargetOffset( vehicleInfo.type );
		enemyView.z += ( vehicleInfo.bbox[ 1 ][ 2 ] - vehicleInfo.bbox[ 0 ][ 2 ] ) * enemyVehicleOffset; //mal: look at the top of their vehicle - if we can see it, we can shoot it!
	}

//mal: see if hes in our field of view, if thats desired...
	if ( useFOV ) {
		if ( botVehicleInfo == NULL || botInfo->proxyInfo.hasTurretWeapon ) {
            if ( !InFrontOfClient( botNum, enemyView ) ) {
				return false;
			}
		} else {
			if ( !InFrontOfVehicle( botInfo->proxyInfo.entNum, enemyView ) ) {
				return false;
			}
		}
	}

	if ( botVehicleInfo != NULL ) {
		entNum = botInfo->proxyInfo.entNum;
	}

//mal: hes in our FOV (or we don't care about FOV)  - need to see if hes visible now!
	botThreadData.clip->TracePointExt( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, enemyView, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ), ( entNum == -1 ) ? NULL : GetGameEntity( entNum ) ); 

	if ( saveEnt ) {
		if ( ( client.isLeaning || client.usingMountedGPMG ) && tr.fraction == 1.0f ) {
			gunTargetEntNum = clientNum;
		} else {
			gunTargetEntNum = tr.c.entityNum; //mal: lets save off whos in our gun sights for later... 
		}
	}

	if ( tr.c.entityNum != clientNum && tr.fraction != 1.0f ) {
		if ( client.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: is the bastid in a vehicle?
			if ( tr.c.entityNum != client.proxyInfo.entNum ) {
                return false;
			}
		} else {
			return false;
		}
	}

	if ( botVehicleInfo != NULL && botVehicleInfo->canRotateInPlace && botVehicleInfo->type != MCP ) { //mal: tanks need a 2nd trace, to make sure we can actually see AND shoot the target!
		botThreadData.clip->TracePointExt( CLIP_DEBUG_PARMS tr, botInfo->proxyInfo.weaponOrigin, enemyView, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ), GetGameEntity( botVehicleInfo->entNum ) );

		if ( tr.c.entityNum != clientNum && tr.fraction != 1.0f ) {
			if ( client.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: is the bastid in a vehicle?
				if ( tr.c.entityNum != client.proxyInfo.entNum ) {
				    return false;
				}
			} else {
				return false;
			}
		}
	}

	return true; //mal: we see him!
}

/*
================
idBotAI::ClientObscuredBySmokeToBot

Check to see if the client in question is obscured from the bot's view
by a cloud of smoke.
================
*/
bool idBotAI::ClientObscuredBySmokeToBot ( int clientNum ) {
	int smokeExplodeDelay = 6000;
	float maxSmokeRadius = 320.0f;
	float maxSmokeRadiusTime = 5000;
	float smokeRadius;
	float dist;
	idVec3 point;
	idVec3 start = botInfo->viewOrigin;
	idVec3 end = botWorld->clientInfo[ clientNum ].viewOrigin;
	idVec3 temp;
	idVec3 vec, pVec, vProj;

	point = botWorld->clientInfo[ clientNum ].origin - botInfo->origin;

	if ( point.LengthSqr() < Square( 450.0f ) ) {
		return false;
	} //mal: too close for it to matter.

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		const smokeBombInfo_t &smokeNade = botWorld->smokeGrenades[ i ];

		if ( smokeNade.entNum == 0 && smokeNade.birthTime + 30000 < botWorld->gameLocalInfo.time  ) {
			continue;
		}

		if ( smokeNade.birthTime + smokeExplodeDelay > botWorld->gameLocalInfo.time ) {
			continue;
		} //mal: takes a few seconds for the effect to be generated

		if ( smokeNade.xySpeed > 50.0f ) {
			continue;
		} //mal: dont worry about it if its still moving
		
		point = smokeNade.origin;

		point[ 2 ] += 32.0f; //mal: raise it off the ground.

		smokeRadius = maxSmokeRadius * ( ( botWorld->gameLocalInfo.time - ( smokeNade.birthTime + smokeExplodeDelay - 3000 ) ) / maxSmokeRadiusTime );

		if ( smokeRadius > maxSmokeRadius ) {
			smokeRadius = maxSmokeRadius;
		}

		if ( botThreadData.AllowDebugData() ) {
			gameRenderWorld->DebugCircle( colorRed, point, idVec3( 0, 0, 1 ), smokeRadius, 24, 16, true );
			gameRenderWorld->DebugLine( colorOrange, point, point + idVec3( 0, 0, 128.0f ) );
		}
		
		pVec = point - start;
		vec = end - start;
		float lengthSqr = vec.LengthSqr();
		float f = ( pVec * vec ) / lengthSqr;
		f = idMath::ClampFloat( 0.0f, 1.0f, f );
		vProj = pVec - ( f * vec );

		dist = vProj.LengthSqr();

		if ( dist < Square( smokeRadius ) ) {
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::Bot_SetAttackTimeDelay

Sets how long the bot should delay "awareness" of an enemy.
Is influenced by what state the bot is currently in.
================
*/
void idBotAI::Bot_SetAttackTimeDelay( bool inFront ) {

	if ( aiState == NBG && ( nbgType != CAMP && nbgType != DEFENSE_CAMP ) ) {
		timeTilAttackEnemy = botWorld->gameLocalInfo.time + 450; // a bit longer, because bot was busy doing something.
	} else if ( aiState == COMBAT ) {
		timeTilAttackEnemy = botWorld->gameLocalInfo.time + 100; // really fast if already in a fighting mood
	} else { // LTG, etc
		timeTilAttackEnemy = botWorld->gameLocalInfo.time + 300; // not too long normally.
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
		timeTilAttackEnemy += 1000; //mal: if training mode, add a bit of "lag" in our targettting of enemies
	} else if ( botWorld->gameLocalInfo.botAimSkill == 0 ) {
		timeTilAttackEnemy += 1000; //mal: if low skill, add a bit of "lag" in our targettting of enemies
	} else if ( botWorld->gameLocalInfo.botAimSkill < 2 ) {
		timeTilAttackEnemy += 500; //mal: if low skill, add a bit of "lag" in our targettting of enemies
	} else {

        if ( botInfo->isDisguised ) { //mal: if we were disguised and decided to fight, obviously we already see our enemy, so be quicker about attacking.
			fastAwareness = true;
		}

		if ( ClientIsDangerous( enemy ) ) { //mal: be quicker about catching campers.
			fastAwareness = true;
		}

		if ( Client_IsCriticalForCurrentObj( botNum, 1500.0f ) && inFront != false ) { //mal: if the guys in front of us, we're prolly aware of him already, just didnt want a distracting fight.
			fastAwareness = true;
		}

		if ( aiState == NBG && ( nbgType == CAMP || nbgType == DEFENSE_CAMP ) && heardClient == enemy ) { //mal: if we were camping and heard this guy coming, we're gonna react faster.
			fastAwareness = true;
		}
	}

	if ( fastAwareness != false ) {
		timeTilAttackEnemy = botWorld->gameLocalInfo.time + 50; //REALLY fast - only used for enemies that we already "spotted" somewhere else, or had purposely ignored for some reason.
		fastAwareness = false;
	}

}

/*
================
idBotAI::Bot_CheckCurrentStateForCombat
================
*/
void idBotAI::Bot_CheckCurrentStateForCombat() {
	float evadeDist = Square( 1500.0f );
	idVec3 vec;

//mal: this code will decide if the bot should chase, flank, flee, etc.
//mal: will make this decsion based on health, weapon, ammo, what the enemy is/has, how many enemies are vis, and what the bot is trying to do ( heal/revive, get some obj, etc).
	if ( AIStack.STACK_AI_NODE != NULL ) {
		if ( ( AIStack.isPriority && !botInfo->isDisguised ) || ClientHasObj( botNum ) || Client_IsCriticalForCurrentObj( botNum, 2500.0f ) ) {
			if ( AIStack.stackActionNum != ACTION_NULL ) {
				vec = botThreadData.botActions[ AIStack.stackActionNum ]->origin - botInfo->origin;

				if ( botThreadData.botActions[ AIStack.stackActionNum ]->GetObjForTeam( botInfo->team ) == ACTION_DEFENSE_CAMP ) {
					evadeDist = Square( 10000.0f );
				}
			} else if ( AIStack.stackClientNum != -1 ) {
				vec = botWorld->clientInfo[ AIStack.stackClientNum ].origin - botInfo->origin;
			} else if ( AIStack.stackEntNum != -1 ) {
				if ( AIStack.stackEntNum < MAX_CARRYABLES && ( AIStack.stackLTGType == RECOVER_GOAL || AIStack.stackLTGType == STEAL_GOAL ) ) {
					vec = botWorld->botGoalInfo.carryableObjs[ AIStack.stackEntNum ].origin;
					evadeDist = Square( 3500.0f );
				} else {
					vec	= botInfo->origin;
				}
			} else { //mal: not currently supported - so just fight.
				vec	= botInfo->origin;
			}
		}
		
		if ( ClientHasObj( botNum ) || Client_IsCriticalForCurrentObj( botNum, 2500.0f ) ) {
			evadeDist = Square( 3500.0f );
		}

        if ( vec.LengthSqr() < evadeDist ) {
			COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_EvadeEnemy; //mal: if bot has a priority target, and its close, try to avoid combat as much as possible.
			return;
		}
	}

	COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_AttackEnemy;
}

/*
================
idBotAI::ClassWeaponCharged
================
*/
bool idBotAI::ClassWeaponCharged( const playerWeaponTypes_t weaponNum ) {

//mal_TODO: add skill based code someday! also class code for other classes! also exceptions that may arise when bot gets combat awards

	if ( weaponNum == HEALTH || weaponNum == AMMO_PACK ) {
		int baseCharge = 80;

		if ( weaponNum == HEALTH && botInfo->abilities.fasterMedicCharge ) {
			baseCharge = 90;
		}

		if ( botInfo->supplyChargeUsed < baseCharge ) {
			return true;
		} else {
			return false;
		}
	}

	if ( weaponNum == NEEDLE ) {
		if ( botInfo->classChargeUsed == 0 ) {
			return true;
		} else {
			return false;
		}
	}

	if ( weaponNum == SHIELD_GUN ) {
		if ( botInfo->deviceChargeUsed < 50 ) {
			return true;
		} else {
			return false;
		}
	}

	if ( weaponNum == AIRCAN ) {
		if ( botInfo->fireSupportChargedUsed == 0 ) {
			return true;
		} else {
			return false;
		}
	}

	if ( weaponNum == THIRD_EYE ) {
		if ( botInfo->deviceChargeUsed == 0 && botInfo->weapInfo.covertToolInfo.entNum == 0 ) {
			return true;
		} else {
			return false;
		}
	}

	if ( weaponNum == TELEPORTER ) {
		if ( botInfo->deviceChargeUsed == 0 && botInfo->hasTeleporterInWorld == false ) {
			return true;
		} else {
			return false;
		}
	}

	if ( weaponNum == SMOKE_NADE || weaponNum == FLYER_HIVE ) {
		if ( botInfo->deviceChargeUsed == 0 ) {
			return true;
		} else {
			return false;
		}
	}

	if ( weaponNum == SUPPLY_MARKER ) {
		if ( botInfo->supplyChargeUsed == 0 ) {
			return true;
		} else {
			return false;
		}
	}

	return false;
}

/*
================
idBotAI::UpdateEnemyInfo
================
*/
void idBotAI::UpdateEnemyInfo() {

	idVec3 vec;

	if ( !ClientIsVisibleToBot( enemy, false, true ) ) {
		enemyInfo.enemyVisible = false;
	} else {
		enemyInfo.enemyVisible = true;
	}

	if ( enemyInfo.enemyVisible ) {
		enemyInfo.enemy_LS_Pos = botWorld->clientInfo[ enemy ].origin;
		bot_LS_Enemy_Pos = botInfo->origin;
		enemyInfo.enemy_NS_Pos = vec3_zero;
		enemyInfo.enemyLastVisTime = botWorld->gameLocalInfo.time;
	}

	vec = botWorld->clientInfo[ enemy ].origin - botInfo->origin;

	enemyInfo.enemyHeight = idMath::Ftoi( vec.z );

	enemyInfo.enemyDist = vec.LengthFast();

	enemyInfo.enemyInfont = ( botVehicleInfo == NULL ) ? InFrontOfClient( botNum, botWorld->clientInfo[ enemy ].origin ) : InFrontOfVehicle( botInfo->proxyInfo.entNum, botWorld->clientInfo[ enemy ].origin );

	enemyInfo.enemyFacingBot = ( botWorld->clientInfo[ enemy ].proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) ? InFrontOfClient( enemy, botInfo->origin ) : InFrontOfVehicle( botWorld->clientInfo[ enemy ].proxyInfo.entNum, botInfo->origin );
}

/*
================
idBotAI::Bot_FindBetterEnemy

We'll sort thru the clients, and ignore certain clients if we're too busy
to be buggered (carrying obj, planting/hacking, etc) or they're not valid enemies
(in disguise, hidden by smoke, etc). Our current enemy will be the standard we measure
others against.
================
*/
bool idBotAI::Bot_FindBetterEnemy() {
	bool useFOV = true;
	int i;
	int entClientNum = enemy;
	float dist;
	float entDist;
	float sightDist = ENEMY_SIGHT_BUSY_DIST;
	idVec3 vec;

	if ( enemy == -1 ) { //mal: we lost our enemy for some reason, so just skip finding a new one til next frame.
		return false;
	}

	if ( ignoreNewEnemiesTime > botWorld->gameLocalInfo.time ) {
		return false;
	}

	if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: if we're attacking the MCP, its the priority.
		if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum == botWorld->botGoalInfo.botGoal_MCP_VehicleNum ) {
			return false;
		}
	}

	vec = botWorld->clientInfo[ enemy ].origin - botInfo->origin;
	entDist = vec.LengthSqr();

	if ( !enemyInfo.enemyVisible ) {
		entDist = idMath::INFINITY;
	}

	const clientInfo_t& enemyPlayerInfo = botWorld->clientInfo[ enemy ];

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO && !botWorld->botGoalInfo.gameIsOnFinalObjective && ( !enemyPlayerInfo.isBot || enemyPlayerInfo.isActor ) ) { //mal: dont worry about keeping our human target in training mode, unless its the final obj...
		entDist += Square( TRAINING_MODE_RANGE_ADDITION );
		sightDist += TRAINING_MODE_RANGE_ADDITION;
	}
 
	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
		sightDist = 700.0f;
	}

	numVisEnemies = 1; //mal: our current enemy is always visible to us

//mal_TODO: this will need to be VASTLY improved as time goes on!!!!

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
		if ( Client_IsCriticalForCurrentObj( botNum, -1.0f ) && ( botInfo->lastAttacker != i || botInfo->lastAttackerTime + 3000 < botWorld->gameLocalInfo.time ) ) {
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

		vec = playerInfo.viewOrigin - botInfo->viewOrigin;

		dist = vec.LengthSqr();

		if ( dist > Square( sightDist ) ) {
			continue;
		}

		if ( botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO || botWorld->botGoalInfo.gameIsOnFinalObjective ) { //mal: dont worry about picking the best targets in training mode, unless its the final obj...
			if ( Client_IsCriticalForCurrentObj( i, sightDist ) ) { //mal: if a critical class, get high priority
				dist = Square( 100 );
				ignoreNewEnemiesTime = botWorld->gameLocalInfo.time + IGNORE_NEW_ENEMIES_TIME;
	
				if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: high skill bots are aware of enemies near obj.
					useFOV = false;
				}
			}

			if ( ClientHasAttackedTeammate( i, true, 3000 ) ) {
				dist = Square( 150 );
				ignoreNewEnemiesTime = botWorld->gameLocalInfo.time + IGNORE_NEW_ENEMIES_TIME;

				if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: high skill bots are aware of enemies attacking our critical friends.
					useFOV = false;
				}
			}

			if ( ClientHasObj( i ) ) { //mal: if have docs, get HIGHER priority.
				dist = Square( 50 );
				ignoreNewEnemiesTime = botWorld->gameLocalInfo.time + IGNORE_NEW_ENEMIES_TIME;
	
				if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: high skill bots are aware of enemies near obj.
					useFOV = false;
				}
			}

			if ( botWorld->botGoalInfo.mapHasMCPGoal ) {
				if ( playerInfo.proxyInfo.entNum == botWorld->botGoalInfo.botGoal_MCP_VehicleNum ) {
					dist = Square( 200.0f );
					ignoreNewEnemiesTime = botWorld->gameLocalInfo.time + IGNORE_NEW_ENEMIES_TIME;
			
					if ( botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) { //mal: high skill bots are aware of enemies near obj.
						useFOV = false;
					}
				}
			}
		}

		if ( !ClientIsVisibleToBot( i, useFOV, false ) ) {
			continue;
		}

		numVisEnemies++;

		if ( dist < entDist )
		{
			entClientNum = i;
			entDist = dist;
		}
	}

	if ( entClientNum != enemy ) {

		enemy = entClientNum;
		enemySpawnID = botWorld->clientInfo[ entClientNum ].spawnID;

		enemyInfo.enemy_FS_Pos = botWorld->clientInfo[ entClientNum ].origin;
		enemyInfo.enemy_LS_Pos = enemyInfo.enemy_FS_Pos;

		enemyIsHuntGoal = false;

		enemyAcquireTime = botWorld->gameLocalInfo.time;

		enemyInfo.enemyLastVisTime = botWorld->gameLocalInfo.time;

		bot_FS_Enemy_Pos = botInfo->origin;
		bot_LS_Enemy_Pos = bot_FS_Enemy_Pos;

		COMBAT_AI_SUB_NODE = NULL; //mal: reset the bot's combat AI node and movement state.
		COMBAT_MOVEMENT_STATE = NULL;
		combatNBGType = NO_COMBAT_TYPE;

		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_PickBestWeapon
================
*/
void idBotAI::Bot_PickBestWeapon( bool useNades ) {
	bool isAirborneVehicle = false;
	bool isGroundVehicle = false;
	bool enemyIsMovingSlow;
	int ourEnemiesAroundEnemy;
	int ourFriendsAroundEnemy;

	if ( weapSwitchTime > botWorld->gameLocalInfo.time ) {
		return;
	}

	if ( botWorld->gameLocalInfo.inWarmup ) {
		return;
	}

	if ( botInfo->usingMountedGPMG ) {
		return;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO || botWorld->gameLocalInfo.botSkill <= BOT_SKILL_NORMAL ) {
		if ( botThreadData.random.RandomInt( 100 ) > 30 ) {
			useNades = false;
		}
	}

	if ( botWorld->gameLocalInfo.botKnifeOnly != false ) {
		botIdealWeapNum = NULL_WEAP;
		botIdealWeapSlot = MELEE;
		return;
	}

	const clientInfo_t& enemyClient = botWorld->clientInfo[ enemy ];

	if ( botInfo->classType == COVERTOPS ) {
		if ( !enemyInfo.enemyFacingBot && enemyInfo.enemyDist < 300.0f && enemyClient.xySpeed <= RUNNING_SPEED && enemyClient.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {
			if ( botInfo->lastAttacker != enemy || botInfo->lastAttackerTime + 3000 < botWorld->gameLocalInfo.time ) {
				botIdealWeapSlot = MELEE;
				return;
			}
		}
	} //mal: we got the drop on someone - knife them in the back for major humiliation points!

	ourEnemiesAroundEnemy = ClientsInArea( botNum, enemyClient.origin, 700.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, false, false );
	ourFriendsAroundEnemy = ClientsInArea( botNum, enemyClient.origin, 700.0f, botInfo->team, NOCLASS, false, false, false, false, false );

	enemyIsMovingSlow = ( enemyClient.xySpeed < 600.0f ) ? true : false; //mal: if enemy is moving too fast, cant target with things like nades, etc.

	if ( enemyClient.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
        proxyInfo_t vehicleInfo;
			
		GetVehicleInfo( enemyClient.proxyInfo.entNum, vehicleInfo );

		isAirborneVehicle = ( vehicleInfo.isAirborneVehicle != false ) ? true : false;
		isGroundVehicle = ( vehicleInfo.type < ICARUS && !( vehicleInfo.flags & WATER ) ) ? true : false;
	}

	switch( botInfo->classType ) {
	case MEDIC:
		if ( useNades && useNadeOnEnemy == false ) {
			if ( ( !enemyInfo.enemyFacingBot || ourEnemiesAroundEnemy > 2 || isGroundVehicle == true && ( isAirborneVehicle == false && enemyIsMovingSlow ) ) && enemyInfo.enemyDist > 500.0f && enemyInfo.enemyDist < GRENADE_ATTACK_DIST && enemyInfo.enemyHeight < 100.0f && botInfo->weapInfo.hasNadeAmmo ) {
                useNadeOnEnemy = true;
			}
		}
        
		if ( useNadeOnEnemy == false ) {
			if ( botInfo->weapInfo.primaryWeapon == SHOTGUN ) {
				if ( enemyInfo.enemyDist < SHOTGUN_DIST ) {
                    botIdealWeapSlot = GUN; 
				} else {
					botIdealWeapSlot = SIDEARM;
				}
			} else {
				botIdealWeapSlot = GUN;
			}
		}

		break;

	case FIELDOPS:
		if ( hammerTime ) {
			if ( ClassWeaponCharged( AIRCAN ) || ( hammerVehicle && botInfo->weapInfo.artyAttackInfo.deathTime > botWorld->gameLocalInfo.time ) ) {
				botIdealWeapNum = BINOCS;
				botIdealWeapSlot = NO_WEAPON;
				break;
			} else {
				hammerTime = false;
				hammerVehicle = false;
				resetEnemy = true;
				break;					//mal: dont do anything for a frame.
			}
		}

        if ( useNades && useNadeOnEnemy == false && useAircanOnEnemy == false) {
			if ( ( !enemyInfo.enemyFacingBot || ClientIsDefusingOurTeamCharge( enemy ) || ourEnemiesAroundEnemy > 2 || isGroundVehicle == true && ( isAirborneVehicle == false && enemyIsMovingSlow ) ) && enemyInfo.enemyDist > 700.0f && enemyInfo.enemyDist < 1700.0f && enemyInfo.enemyHeight < 100.0f && ClassWeaponCharged( AIRCAN ) && LocationVis2Sky( enemyClient.origin ) ) {
                useAircanOnEnemy = true;
            } else if ( ( !enemyInfo.enemyFacingBot || ourEnemiesAroundEnemy > 2 || isGroundVehicle == true && ( isAirborneVehicle == false && enemyIsMovingSlow ) ) && enemyInfo.enemyDist > 500.0f && enemyInfo.enemyDist < GRENADE_ATTACK_DIST && enemyInfo.enemyHeight < 100.0f && botInfo->weapInfo.hasNadeAmmo ) {
                useNadeOnEnemy = true;
			}
		}

        if ( useNadeOnEnemy == false && useAircanOnEnemy == false ) {
            botIdealWeapSlot = GUN; //mal: the weap autoswitch code will handle switching to pistol if gun is out of ammo.
		}

		break;

	case ENGINEER: //mal_TODO: need to add code here for the eng's nade launcher. Currently its a upgrade, but it may change.
		if ( useNades && useNadeOnEnemy == false ) {
			if ( ( !enemyInfo.enemyFacingBot || ourEnemiesAroundEnemy > 2 || isGroundVehicle == true && ( isAirborneVehicle == false && enemyIsMovingSlow ) ) && enemyInfo.enemyDist > 500.0f && enemyInfo.enemyDist < GRENADE_ATTACK_DIST && enemyInfo.enemyHeight < 100.0f && botInfo->weapInfo.hasNadeAmmo ) {
				useNadeOnEnemy = true;
			}
		}
		
		if ( useNadeOnEnemy == false ) {
			if ( botInfo->weapInfo.primaryWeapon == SHOTGUN ) {
				if ( enemyInfo.enemyDist < SHOTGUN_DIST ) {
                    botIdealWeapSlot = GUN; //mal: the weap autoswitch code will handle switching to pistol if gun is out of ammo.
				} else {
					botIdealWeapSlot = SIDEARM;
				}
			} else {
				botIdealWeapSlot = GUN;
			}
		}
	
		break;

	case COVERTOPS:
		if ( useNades && useNadeOnEnemy == false ) {
			if ( ( !enemyInfo.enemyFacingBot || ourEnemiesAroundEnemy > 2 || isGroundVehicle == true && ( isAirborneVehicle == false && enemyIsMovingSlow ) ) && enemyInfo.enemyDist > 500.0f && enemyInfo.enemyDist < GRENADE_ATTACK_DIST && enemyInfo.enemyHeight < 100.0f && botInfo->weapInfo.hasNadeAmmo ) {
                useNadeOnEnemy = true;
			}
		}

        if ( useNadeOnEnemy == false ) {
			if ( botInfo->weapInfo.primaryWeapon == SNIPERRIFLE ) {
				if ( ( enemyInfo.enemyDist > 900.0f || enemyInfo.enemyDist < 300.0f ) && botInfo->enemiesInArea < 2 && ( botInfo->weapInfo.primaryWeapClipEmpty == false || enemyInfo.enemyDist > 700.0f || botInfo->friendsInArea > 0 ) ) {
					botIdealWeapSlot = GUN; //mal: if my enemy is far away, or REALLY close, and I'm not surrounded, and I have a bullet in the clip, or my enemy is far enough away for me to safely reload or I have some friends near me to cover me while I reload, I'll use my sniperrifle.
				} else {
					botIdealWeapSlot = SIDEARM;
				}
			} else {
				botIdealWeapSlot = GUN;
			}
		}

		break;

	case SOLDIER:
        if ( useNades && useNadeOnEnemy == false ) {
			if ( ( !enemyInfo.enemyFacingBot || ourEnemiesAroundEnemy > 2 || isGroundVehicle == true && ( isAirborneVehicle == false && enemyIsMovingSlow ) ) && enemyInfo.enemyDist > 500.0f && enemyInfo.enemyDist < GRENADE_ATTACK_DIST && enemyInfo.enemyHeight < 100.0f && botInfo->weapInfo.hasNadeAmmo && ( botInfo->weapInfo.primaryWeapon != ROCKET && botInfo->weapInfo.primaryWeapon != HEAVY_MG ) ) {
                useNadeOnEnemy = true;
			}
		}
		
		if ( useNadeOnEnemy == false ) {
			if ( botInfo->weapInfo.primaryWeapon == SHOTGUN ) {
				if ( enemyInfo.enemyDist < SHOTGUN_DIST ) {
                    botIdealWeapSlot = GUN; //mal: the weap autoswitch code will handle switching to pistol if gun is out of ammo.
				} else {
					if ( botInfo->team == STROGG ) {
						if ( enemyInfo.enemyDist > LIGHTNING_GUN_DIST ) {
							botIdealWeapSlot = GUN;
						} else {
							botIdealWeapSlot = SIDEARM; //mal: lightning gun is ranged.
						}
					} else {
						botIdealWeapSlot = SIDEARM;
					}
				}
			} else if ( botInfo->weapInfo.primaryWeapon == SMG ) { //mal: the easiest of the bunch! :-D
				botIdealWeapSlot = GUN;
			} else if ( botInfo->weapInfo.primaryWeapon == HEAVY_MG ) {
				if ( enemyInfo.enemyDist < 2500.0f && ( botInfo->weapInfo.primaryWeapClipEmpty == false || enemyInfo.enemyDist > 700.0f || botInfo->friendsInArea > 0 ) ) {
                    botIdealWeapSlot = GUN; //mal: the weap autoswitch code will handle switching to pistol if gun is out of ammo.
				} else {
					botIdealWeapSlot = SIDEARM;
				}
			} else if ( botInfo->weapInfo.primaryWeapon == ROCKET ) {
				if ( enemyClient.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE && ourFriendsAroundEnemy == 0 && enemyInfo.enemyDist < WEAPON_LOCK_DIST ) { //mal: dont kill our friends!
					botIdealWeapSlot = GUN;
				} else {
					if ( enemyInfo.enemyDist > 500.0f && enemyInfo.enemyDist < 2500.0f && ( botInfo->weapInfo.primaryWeapClipEmpty == false || enemyInfo.enemyDist > 1200.0f || enemyClient.friendsInArea > 1 ) && ourFriendsAroundEnemy == 0 ) {
						botIdealWeapSlot = GUN;
					} else {
						if ( botInfo->team == STROGG ) {
							if ( enemyInfo.enemyDist > LIGHTNING_GUN_DIST ) {
                                botIdealWeapSlot = GUN;
							} else {
								botIdealWeapSlot = SIDEARM; //mal: lightning gun is ranged.
							}
						} else {
							botIdealWeapSlot = SIDEARM;
						}
					}
				}
			}
		}

		break;

	default:
		botIdealWeapSlot = GUN;
		break;
	}

	weapSwitchTime = botWorld->gameLocalInfo.time + 1500; //mal: dont constantly switch back and forth between weapons.

	if ( useAircanOnEnemy != false ) {
		botIdealWeapSlot = NO_WEAPON;
		botIdealWeapNum = AIRCAN;
	} else if ( useNadeOnEnemy != false ) {
		botIdealWeapSlot = NADE;
		botIdealWeapNum = NULL_WEAP;

		if ( combatMoveType != GRENADE_ATTACK ) {
			COMBAT_MOVEMENT_STATE = NULL;
		}
	}
}

/*
================
idBotAI::Bot_PickPostCombatGoal

Decide if we should do something to our recently deceased enemy.

TODO: add more possible post combat behaviors?
================
*/
bool idBotAI::Bot_PickPostCombatGoal() {
	if ( !ClientIsValid( enemy, -1 ) ) {
		return false;
	}

	if ( Client_IsCriticalForCurrentObj( botNum, -1.0f ) ) { //mal: bots who are critical to the war effort will ignore such sillyness.
		return false;
	}

	if ( AIStack.STACK_AI_NODE != NULL && AIStack.isPriority == true ) { //mal: we were doing something important before we got in combat - get back to it!
		return false;
	}

	if ( ( botInfo->classType != MEDIC || botInfo->team != STROGG ) && botInfo->classType != COVERTOPS && botThreadData.random.RandomInt( 100 ) > 80 ) {
		if ( !botWorld->clientInfo[ enemy ].inLimbo && !botWorld->clientInfo[ enemy ].inWater && !botWorld->clientInfo[ enemy ].isBot ) {
			if ( botWorld->clientInfo[ enemy ].areaNum > 0 ) {
				idVec3 vec =  botWorld->clientInfo[ enemy ].origin - botInfo->origin;
				if ( vec.LengthSqr() < Square( 700.0f ) ) {
					nbgTarget = enemy;
					nbgTargetSpawnID = enemySpawnID;
					ROOT_AI_NODE = &idBotAI::Run_NBG_Node;		
					NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_HumiliateEnemy;
					Bot_ResetEnemy();
					return true;
				}
			}
		}
	}

	//mal: slows the game down too much in ETQW.
/*
	if ( botThreadData.random.RandomInt( 100 ) > 80 && !LocationVis2Sky( botInfo->origin ) ) { //mal: sometimes, when indoors, take a sec to look around for more enemies.
		nbgOrigin = enemyInfo.enemy_FS_Pos;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;		
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_Pause;
		Bot_ResetEnemy();
		return true;
	}
*/

	return false;
}

/*
================
idBotAI::Bot_ShouldChaseHiddenEnemy
================
*/
bool idBotAI::Bot_ShouldChaseHiddenEnemy( bool chase ) {

	proxyInfo_t vehicleInfo;
	idVec3 vec, origin;
	chaseEnemy = false;
	chasingEnemy = false;

	if ( enemyIsHuntGoal ) {
		return true;
	}

	if ( botInfo->usingMountedGPMG ) {
		return false;
	}

	if ( ClientHasObj( botNum ) || Client_IsCriticalForCurrentObj( botNum, -1.0f ) ) { //mal: never chase if we are important!
		return false;
	}

	if ( botInfo->health <= 30 ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) { //mal: if we're in bad shape, sometimes we'll give up the fight
            return false;
		}
	}

	if ( !botInfo->weapInfo.primaryWeapHasAmmo && !ClientHasObj( enemy ) ) { //mal: we don't have any ammo, and hes not that important, so let him go...
		return false;
	}

	if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {

		GetVehicleInfo( botWorld->clientInfo[ enemy ].proxyInfo.entNum, vehicleInfo );

		if ( vehicleInfo.inWater ) {
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

	origin = ( chase ) ? enemyInfo.enemy_NS_Pos : enemyInfo.enemy_LS_Pos;
	vec = origin - botInfo->origin;
	
	if ( vec.LengthSqr() > Square( ENEMY_CHASE_DIST ) ) {
		return false;
	} // too far away to chase

	vec = origin - botWorld->clientInfo[ enemy ].origin;

	if ( vec.LengthSqr() > Square( 900.0f ) && !ClientHasObj( enemy ) && !Client_IsCriticalForCurrentObj( enemy, 1500.0f ) && !ClientIsDangerous( enemy ) ) { // unless he's REALLY close or a major threat, sometimes we'll stop the chase, just because....
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
            return false;
		}
	}

	//mal_TODO: add more stuff here as we need to.

	if ( chase ) {
		chaseEnemy = true;
	}

	chasingEnemy = true;

	return true;
}

/*
================
idBotAI::Bot_CheckShouldUseGrenade
================
*/
bool idBotAI::Bot_CheckShouldUseGrenade( bool targetVisible ) {
	bool useGrenade = false;
    idVec3 vec;
	float dist;

	if ( botInfo->weapInfo.hasNadeAmmo == false ) {
		 return false;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO || botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
		if ( botThreadData.random.RandomInt( 100 ) > 30 ) {
			return false;
		}
	}

	if ( !targetVisible ) {
        if ( ( lastGrenadeTime + 15000 ) > botWorld->gameLocalInfo.time ) { //mal: we already threw a nade recently - dont do so again.
			return false;
		}

		if ( enemyInfo.enemy_LS_Pos == vec3_zero ) {
			return false;
		}

		vec = enemyInfo.enemy_LS_Pos - botInfo->origin;

		if ( vec.LengthSqr() > Square( GRENADE_THROW_MAXDIST ) ) {
			return false;
		}

		if ( botThreadData.random.RandomInt( 100 ) > 85 ) {
			return false;
		}

		vec = enemyInfo.enemy_LS_Pos - botWorld->clientInfo[ enemy ].origin;
	
		if ( vec.LengthSqr() > Square( 900.0f ) ) {
			return false;
		}

		trace_t tr;

		botThreadData.clip->TracePointExt( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, enemyInfo.enemy_LS_Pos, MASK_SHOT_BOUNDINGBOX | MASK_VEHICLESOLID | CONTENTS_FORCEFIELD, GetGameEntity( botNum ), NULL ); 

		if ( tr.fraction < 1.0f ) {
			return false;
		}

        useGrenade = true;

		safeGrenade = ( botThreadData.random.RandomInt( 100 ) > 40 ) ? true : false; // for safe nade - will stand back and throw, else will charge up to enemy and toss at them

	} else { //mal: target is visible!

		int clients;
		
		vec = botWorld->clientInfo[ enemy ].origin - botInfo->origin;
		dist = vec.LengthFast();

		if ( dist > GRENADE_THROW_MAXDIST ) { // too far away!
			return false;
		}

		if ( !enemyInfo.enemyFacingBot ) { //sneak attack!
			return true;
		}

		clients = ClientsInArea( -1, botWorld->clientInfo[ enemy ].origin, 600.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, false, false );

		if ( clients > 3 ) { // bunch of them in the same area - take advantage of this! need more clients in same area to make this worthwhile
			return true;
		}

		if ( enemyInfo.enemyHeight <= -150 ) { // we have the height advantage - so use it!
			return true;
		}
	}

	return useGrenade;
}

/*
================
idBotAI::UpdateNonVisEnemyInfo
================
*/
void idBotAI::UpdateNonVisEnemyInfo() {
	if ( !ClientIsValid( enemy, -1 ) ) {
		return;
	}

	trace_t	tr;
	botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, enemyInfo.enemy_LS_Pos, botWorld->clientInfo[ enemy ].origin, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( enemy ) );
	
	if ( tr.fraction == 1.0f ) {
		enemyInfo.enemy_NS_Pos = botWorld->clientInfo[ enemy ].origin;
	}
}

/*
================
idBotAI::Bot_ThrowGrenade
================
*/
bool idBotAI::Bot_ThrowGrenade( const idVec3 &origin, bool fastNade ) {
	int fuseTime = ( fastNade == true ) ? 250 : 3000;
	float height;
	idVec3 vec = origin - botInfo->origin;
	float dist = vec.LengthSqr();

//	Bot_LookAtLocation( origin, INSTANT_TURN );

	if ( botInfo->weapInfo.weapon == GRENADE || botInfo->weapInfo.weapon == EMP ) {			
		botUcmd->botCmds.attack = true;
		botUcmd->botCmds.constantFire = true;

		height = vec[ 2 ];

		idVec3 nadeTarget = origin;

		if ( dist > Square( GRENADE_THROW_MINDIST ) && height < 150.0f ) {
			nadeTarget.z += 32.0f; 
			botUcmd->botCmds.throwNade = true;
		}

		Bot_LookAtLocation( nadeTarget, INSTANT_TURN, true );

		if ( botThreadData.AllowDebugData() ) {
			gameRenderWorld->DebugLine( colorLtGrey, botInfo->viewOrigin, nadeTarget, 99 );
		}

		if ( botInfo->weapInfo.weapon == EMP ) { 
			if ( botThreadData.random.RandomInt( 100 ) > 90 ) {
				botUcmd->botCmds.attack = false;
				lastGrenadeTime = botWorld->gameLocalInfo.time;
				useNadeOnEnemy = false;
				return true;
			}
		}


		if ( botInfo->weapInfo.grenadeFuseStart != -1 ) {
			if ( botWorld->gameLocalInfo.time > ( ( botInfo->weapInfo.grenadeFuseStart * 1000 ) + fuseTime ) ) {
				botUcmd->botCmds.attack = false;
				lastGrenadeTime = botWorld->gameLocalInfo.time;
				useNadeOnEnemy = false;
				return true;
			}
		}
	}

	return false;
}

/*
================
idBotAI::Bot_CheckCombatExceptions
================
*/
bool idBotAI::Bot_CheckCombatExceptions() {
	int attackerHealth;
	float distToAttackerSqr = Square( 99999.0f );
	float ignoreDangerDist = 2500.0f;
	idVec3 vec;

	if ( botInfo->lastAttacker > -1 && botInfo->lastAttacker < MAX_CLIENTS ) {
		attackerHealth = botWorld->clientInfo[ botInfo->lastAttacker ].health;
		vec = botWorld->clientInfo[ botInfo->lastAttacker ].origin - botInfo->origin;
		distToAttackerSqr = vec.LengthSqr();
	} else { //mal: paranoid safety check!
		attackerHealth = 0;
	} //mal: check to see if the guy who attacked us last is dead - in which case we won't worry about having been shot by him recently!

	if ( botInfo->onLadder ) {
		if ( botInfo->lastAttackerTime + 1500 < botWorld->gameLocalInfo.time || attackerHealth <= 0 || distToAttackerSqr > Square( ignoreDangerDist ) ) {
			return true;
		}
	}

	if ( aiState == LTG && ( ltgType == STEAL_GOAL || ltgType == DELIVER_GOAL || ltgType == RECOVER_GOAL ) ) {
		if ( botInfo->lastAttackerTime + 1500 < botWorld->gameLocalInfo.time || attackerHealth <= 0 || distToAttackerSqr > Square( ignoreDangerDist ) ) {
			return true;
		}
	}

	switch ( botInfo->classType ) {

		case MEDIC: {
			if ( ( aiState == NBG && nbgType == SUPPLY_TEAMMATE ) ) { // medics will try to heal humans no matter what!
				if ( botInfo->isActor ) {
					return false;
				}

				if ( ClientIsValid( nbgTarget, -1 ) ) {
					const clientInfo_t& player = botWorld->clientInfo[ nbgTarget ];
					
					if ( !player.isBot ) { //mal: always make the human feel special and try to heal him no matter what, even tho it will likely get us killed.
						return true;
					}
				}
			}

			if ( ( aiState == NBG && nbgType == REVIVE_TEAMMATE ) ) { // medics will try to revive before fighting, unless they are under attack!

				if ( DisguisedKillerInArea() && botInfo->team != GDF ) {
					return false;
				}

				if ( botInfo->team == GDF && distToAttackerSqr > Square( 1500.0f ) ) { //mal: our intstant revive means we can get the job done before without dying ( hopefully! ).
					return true;
				}

				if ( ( botInfo->lastAttackerTime + 1000 ) < botWorld->gameLocalInfo.time || attackerHealth <= 0 || distToAttackerSqr > Square( ignoreDangerDist ) ) {
					return true;
				} // if not under direct attack - try to revive, the more friendlies we can bring to bear on our enemy, the better chance we've got of winning!

				if ( ClientIsValid( nbgTarget, -1 ) ) {
					const clientInfo_t& player = botWorld->clientInfo[ nbgTarget ];

					if ( !player.isBot ) {
						return true;
					}
				}

				if ( botInfo->team == STROGG ) { //mal: strogg take so much longer to revive, that if we're under attack, we MUST decide whether to engage!
					return false;
				}

				if ( attackerHealth > 0 ) { //mal: if the guy who shot us is in front of us, NEVER ignore him!
                    if ( InFrontOfClient( botNum, botWorld->clientInfo[ botInfo->lastAttacker ].origin ) ) {
						return false;
					}
				}

				vec = botWorld->clientInfo[ nbgTarget ].origin - botInfo->origin;

				if ( vec.LengthFast() < 500.0f ) { // really close to our target - take a chance to revive them
					return true;
				}

				if ( botInfo->enemiesInArea == 0 || botInfo->friendsInArea > 1 ) {
					return true;
				}
			}

			if ( aiState == LTG && ( ltgType == DEFENSE_CAMP_GOAL || ltgType == DESTROY_DEPLOYABLE_GOAL || ltgType == FDA_GOAL || ltgType == STEAL_SPAWN_GOAL ) ) {
				if ( ( botInfo->lastAttackerTime + 1000 ) < botWorld->gameLocalInfo.time || attackerHealth <= 0 || distToAttackerSqr > Square( ignoreDangerDist ) ) {
					return true;
				} // if not under direct attack - try to build. This is important!

				if ( attackerHealth > 0 ) { //mal: if the guy who shot us is in front of us, NEVER ignore him!
                    if ( InFrontOfClient( botNum, botWorld->clientInfo[ botInfo->lastAttacker ].origin ) ) {
						return false;
					}
				}

				if ( botInfo->enemiesInArea < 2 || botInfo->friendsInArea > 0 ) { //mal: we'll ignore only 1 enemy, hopefully theres some backup around here.
					return true;
				}

				break;
			}

			break;
		}

		case ENGINEER: {
			if ( ( aiState == NBG && ( nbgType == BUILD || nbgType == DEFUSE_BOMB || nbgType == PLANT_MINE ) ) || Client_IsCriticalForCurrentObj( botNum, 900.0f ) || ( aiState == LTG && ( ltgType == DEFENSE_CAMP_GOAL || ltgType == DESTROY_DEPLOYABLE_GOAL || ltgType == FDA_GOAL || ltgType == FIX_MCP || ltgType == STEAL_SPAWN_GOAL ) ) ) { // eng is actually in the process of building ATM - lets see if he should ignore any enemies - engs will try to build at all costs, unless they are under attack!

				if ( actionNum != -1 ) {
					if ( botThreadData.botActions[ actionNum ]->GetHumanObj() == ACTION_MINOR_OBJ_BUILD || botThreadData.botActions[ actionNum ]->GetStroggObj() == ACTION_MINOR_OBJ_BUILD ) {
						return false; //mal: dont care about minor build objs - they're not worth dying for!
					}
				}

				if ( ( botInfo->lastAttackerTime + 1000 ) < botWorld->gameLocalInfo.time || attackerHealth <= 0 || distToAttackerSqr > Square( ignoreDangerDist ) ) {
					return true;
				} // if not under direct attack - try to build. This is important!

				if ( attackerHealth > 0 ) { //mal: if the guy who shot us is in front of us, NEVER ignore him!
                    if ( InFrontOfClient( botNum, botWorld->clientInfo[ botInfo->lastAttacker ].origin ) ) {
						return false;
					}
				}

				if ( CriticalEnemyClientNearUs( SOLDIER ) ) {
					return false;
				}

				if ( aiState == LTG && ltgType == FIX_MCP ) {
					if ( attackerHealth > 0 ) {
						proxyInfo_t enemyVehicle;
						GetVehicleInfo( botWorld->clientInfo[ botInfo->lastAttacker ].proxyInfo.entNum, enemyVehicle );

						if ( enemyVehicle.entNum != 0 ) {
							if ( enemyVehicle.isAirborneVehicle ) {
								return true;
							}

							idVec3 vec = enemyVehicle.origin - botInfo->origin;

							if ( vec.LengthSqr() > Square( 1500.0f ) ) {
								return true;
							}
						}
					}
				}						

				if ( botInfo->enemiesInArea < 2 || botInfo->friendsInArea > 0 ) { //mal: we'll ignore only 1 enemy, hopefully theres some backup around here.
					return true;
				}

				break;
			}

			if ( ( aiState == NBG && nbgType == DROP_DEPLOYABLE ) || ( aiState == LTG && ltgType == DROP_DEPLOYABLE_GOAL ) ) {
				if ( actionNum != -1 ) {
					if ( botThreadData.botActions[ actionNum ]->ActionIsPriority() == false ) {
						return false;
					}
				}

				if ( ( botInfo->lastAttackerTime + 1000 ) < botWorld->gameLocalInfo.time || attackerHealth <= 0 || distToAttackerSqr > Square( ignoreDangerDist ) ) {
					return true;
				} // if not under direct attack - try to build. This is important!

				if ( attackerHealth > 0 ) { //mal: if the guy who shot us is in front of us, NEVER ignore him!
                    if ( InFrontOfClient( botNum, botWorld->clientInfo[ botInfo->lastAttacker ].origin ) ) {
						return false;
					}
				}

				if ( botInfo->enemiesInArea == 0 ) {
					return true;
				}

				break;
			}

			break;
		}

		case COVERTOPS: {
			if ( Client_IsCriticalForCurrentObj( botNum, 900.0f ) || ( aiState == NBG && nbgType == HACK ) || ( aiState == LTG && ( ltgType == DEFENSE_CAMP_GOAL || ltgType == DESTROY_DEPLOYABLE_GOAL || ltgType == FDA_GOAL || ltgType == STEAL_SPAWN_GOAL ) ) ) {
                
				if ( ( botInfo->lastAttackerTime + 1000 ) < botWorld->gameLocalInfo.time || attackerHealth <= 0 || distToAttackerSqr > Square( ignoreDangerDist ) ) {
					return true;
				} // if not under direct attack - try to hack. This is important!

				if ( attackerHealth > 0 ) { //mal: if the guy who shot us is in front of us, NEVER ignore him!
                    if ( InFrontOfClient( botNum, botWorld->clientInfo[ botInfo->lastAttacker ].origin ) ) {
						return false;
					}
				}

				if ( botInfo->enemiesInArea < 2 || botInfo->friendsInArea > 0 ) { //mal: we'll ignore only 1 enemy, hopefully theres some backup around here.
					return true;
				}

				break;
			}

			break;
		}

		case FIELDOPS: {
			if ( aiState == LTG && ( ltgType == DESTROY_DEPLOYABLE_GOAL || ltgType == FDA_GOAL || ltgType == STEAL_SPAWN_GOAL || ltgType == DEFENSE_CAMP_GOAL || ( ltgType == FIRESUPPORT_CAMP_GOAL && ltgReached == false ) ) ) {
				if ( ( botInfo->lastAttackerTime + 1000 ) < botWorld->gameLocalInfo.time || attackerHealth <= 0 || distToAttackerSqr > Square( ignoreDangerDist ) ) {
					return true;
				} // if not under direct attack - try to destroy. This is important!

				if ( attackerHealth > 0 ) { //mal: if the guy who shot us is in front of us, NEVER ignore him!
                    if ( InFrontOfClient( botNum, botWorld->clientInfo[ botInfo->lastAttacker ].origin ) ) {
						return false;
					}
				}

				if ( botInfo->enemiesInArea < 2 || botInfo->friendsInArea > 0 ) { //mal: we'll ignore only 1 enemy, hopefully theres some backup around here.
					return true;
				}

				break;
			}
	   }

		case SOLDIER: {
			if ( Client_IsCriticalForCurrentObj( botNum, 900.0f ) || ( aiState == NBG && nbgType == PLANT_BOMB ) || ( aiState == LTG && ( ltgType == DEFENSE_CAMP_GOAL || ltgType == DESTROY_DEPLOYABLE_GOAL || ltgType == FDA_GOAL || ltgType == STEAL_SPAWN_GOAL ) ) ) {

				if ( actionNum != ACTION_NULL ) {
					if ( ClientHasChargeInWorld( botNum, true, actionNum ) ) { //mal: if we've planted, and are just camping nearby, DO attack anyone we see!
						return false;
					}
				}
                
				if ( ( botInfo->lastAttackerTime + 1000 ) < botWorld->gameLocalInfo.time || attackerHealth <= 0 || distToAttackerSqr > Square( ignoreDangerDist ) ) {
					return true;
				} // if not under direct attack - try to plant. This is important!

				if ( attackerHealth > 0 ) { //mal: if the guy who shot us is in front of us, NEVER ignore him!
                    if ( InFrontOfClient( botNum, botWorld->clientInfo[ botInfo->lastAttacker ].origin ) ) {
						return false;
					}
				}

				if ( CriticalEnemyClientNearUs( ENGINEER ) ) {
					return false;
				}

				if ( botInfo->enemiesInArea < 2 || botInfo->friendsInArea > 0 ) { //mal: we'll ignore only 1 enemy, if we've got some backup around here.
					return true;
				}

				break;
			}

			break;
		}
	}

	return false;
}

/*
================
idBotAI::ClientsInArea
================
*/
int	idBotAI::ClientsInArea( int ignoreClientNum, const idVec3 &org, float range, int team, const playerClassTypes_t clientClass, bool inFront, bool vis2Sky, bool ignoreInvulnerable, bool ignoreDisguised, bool ignoreInVehicle, bool humanOnly ) {
	int i;
	int clients = 0;
	idVec3 vec;

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == ignoreClientNum ) { // dont scan the client who started this
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( !playerInfo.inGame ) {
			continue;
		}

		if ( playerInfo.health <= 0 ) { //mal: dont count dead clients!
			continue;
		}

		if ( playerInfo.isNoTarget ) { //mal: let me debug behavior!
			continue;
		}

		if ( humanOnly ) {
			if ( playerInfo.isBot ) {
				continue;
			}
		}

		if ( clientClass != NOCLASS ) {
			if ( playerInfo.classType != clientClass ) {
				continue;
			}
		}

		if ( ignoreInVehicle ) {
			if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
				continue;
			}
		}

		if ( ignoreDisguised ) {
			if ( playerInfo.isDisguised ) {
				continue;
			}
		}

		if ( ignoreInvulnerable ) {
            if ( playerInfo.invulnerableEndTime > botWorld->gameLocalInfo.time ) {
                continue; //mal: ignore revived/just spawned in clients
			}
		}

		if ( vis2Sky ) {
			if ( !LocationVis2Sky( playerInfo.origin ) ) {
				continue;
			}
		}

		if ( team != NOTEAM ) {
            if ( playerInfo.team != team ) {
				continue;
			}
		}

		vec = playerInfo.origin - org;

		if ( vec.LengthSqr() > Square( range ) ) {
			continue;
		}

		if ( inFront && ignoreClientNum != -1 ) {			
			if ( !InFrontOfClient( ignoreClientNum, playerInfo.origin )) {
				continue;
			}
		}

		clients++;
	}

	return clients;
}

/*
================
idBotAI::Bot_CheckShouldUseAircan
================
*/
bool idBotAI::Bot_CheckShouldUseAircan( bool targetVisible ) {

	idVec3 vec;
	float dist;

	if ( botInfo->classType != FIELDOPS ) {
		return false;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO || botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
		if ( botThreadData.random.RandomInt( 100 ) > 30 ) {
			return false;
		}
	}

	if ( !ClassWeaponCharged( AIRCAN )) {
		return false;
	}

	if ( !targetVisible ) {

		if ( !LocationVis2Sky( enemyInfo.enemy_LS_Pos )) {
			return false;
		}

		if ( enemyInfo.enemy_LS_Pos == vec3_zero ) {
			return false;
		}

	    vec = enemyInfo.enemy_LS_Pos - botInfo->origin;
		dist = vec.LengthFast();
		vec = enemyInfo.enemy_LS_Pos - botWorld->clientInfo[ enemy ].origin;
	
		if ( dist > 900.0f ) {
			return false;
		}

		if ( vec.LengthFast() > 900.0f ) {
			return false;
		}

		return true;

	} else { //mal: target is visible!

		int clients;
		
		vec = botWorld->clientInfo[ enemy ].origin - botInfo->origin;
		dist = vec.LengthFast();

		if ( !LocationVis2Sky( botWorld->clientInfo[ enemy ].origin ) ) {
			return false;
		}

		if ( dist > 900.0f ) { // too far away!
			return false;
		}

		if ( !enemyInfo.enemyFacingBot ) { //sneak attack!
			return true;
		}

		clients = ClientsInArea( -1, botWorld->clientInfo[ enemy ].origin, 600.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, true, false, false, false );

		if ( clients > 1 ) { // bunch of them in the same area - take advantage of this!
			return true;
		}

		if ( enemyInfo.enemyHeight <= -100 ) { // we have the height advantage - so use it!
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::Bot_UseCannister
================
*/
void idBotAI::Bot_UseCannister( const playerWeaponTypes_t weapType, const idVec3 &origin ) {
	botIdealWeapNum = weapType;
	botIdealWeapSlot = NO_WEAPON;

	if ( botInfo->weapInfo.isReady && botInfo->weapInfo.weapon == weapType ) {

		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
            botUcmd->botCmds.attack = false;
		} else {
			botUcmd->botCmds.attack = true;
		}

		idVec3 vec = origin - botInfo->origin;

		float height = vec.z;

		if ( vec.LengthSqr() > Square( 700.0f ) && height < 150.0f ) {
			botUcmd->botCmds.throwNade = true;
		}
	}

	Bot_LookAtLocation( origin, FAST_TURN );

	if ( weapType == AIRCAN ) {
        if ( !ClassWeaponCharged( AIRCAN ) ) {
			useAircanOnEnemy = false;
			weapSwitchTime = 0;
		}
	}

	if ( weapType == SUPPLY_MARKER ) {
		botUcmd->botCmds.droppingSupplyCrate = true;
	}
}

/*
================
idBotAI::EnemyValid
================
*/
bool idBotAI::EnemyValid() {
	if ( !ClientIsValid( enemy, -1 ) ) {
		return false;
	}

	const clientInfo_t& playerEnemyInfo = botWorld->clientInfo[ enemy ];

	if ( playerEnemyInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		if ( !botInfo->weapInfo.primaryWeapHasAmmo ) {
			return false;
		}
	}

	if ( enemyInfo.enemyDist > botWorld->botGoalInfo.botSightDist ) {
		if ( botInfo->classType == COVERTOPS && botInfo->weapInfo.primaryWeapon == SNIPERRIFLE && botInfo->weapInfo.primaryWeapHasAmmo ) {
			return true;
		}

		if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo ) {
			if ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
				return true;
			}
		}

		idVec3 enemyOrg = botWorld->clientInfo[ enemy ].origin;

		if ( botInfo->classType == FIELDOPS && ClassWeaponCharged( AIRCAN ) && Bot_HasWorkingDeployable() && !Bot_EnemyAITInArea( enemyOrg ) ) {
			return true;
		}

		return false;
	}

	return true;
}

/*
================
idBotAI::BotLeftEnemysSight
================
*/
bool idBotAI::BotLeftEnemysSight() {        
	trace_t	tr;
	idVec3  otherView = ( botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) ? botWorld->clientInfo[ enemy ].origin : botWorld->clientInfo[ enemy ].viewOrigin;

	botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, bot_LS_Enemy_Pos, otherView, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ) );

	if ( tr.fraction != 1.0f ) {
		return false;
	} else {
		return true; // our enemy can still see the pos we were in when we last saw the enemy!
	}
}

/*
================
idBotAI::Bot_CheckAttack
================
*/
void idBotAI::Bot_CheckAttack() {

	bool useLockon = ( botInfo->weapInfo.weapon == ROCKET && botWorld->clientInfo[ enemy ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) ? true : false;

	float minUseScopeDist = ( botInfo->weapInfo.weapon == HEAVY_MG ) ? 400.0f : 700.0f;

	if ( botWorld->gameLocalInfo.botAimSkill > 0 ) {
		if ( ( botInfo->xySpeed == 0.0f || botInfo->posture == IS_CROUCHED || botInfo->posture == IS_PRONE || combatMoveType == STAND_GROUND_ATTACK || useLockon || botInfo->weapInfo.weapon == HEAVY_MG ) && enemyInfo.enemyDist > minUseScopeDist ) {
			if ( botInfo->weapInfo.weapon == SMG || botInfo->weapInfo.weapon == PISTOL || botInfo->weapInfo.weapon == SHOTGUN || botInfo->weapInfo.weapon == SCOPED_SMG || botInfo->weapInfo.weapon == SNIPERRIFLE || botInfo->weapInfo.weapon == HEAVY_MG || useLockon ) {
                botUcmd->botCmds.altAttackOn = true;
			}
		}
	}

	if ( timeOnTarget > botWorld->gameLocalInfo.time && hammerTime == false ) { //mal: dont fire until we get the enemy in our "sights".
		return;
	}

	bool shotIsBlocked = false;
	bool botWantsToAttack = false;

	if ( botWorld->gameLocalInfo.botAimSkill == 0 && botInfo->weapInfo.weapon != ROCKET && botInfo->weapInfo.weapon != HEAVY_MG && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) { //mal: low skill bots won't shoot that much
		if ( botThreadData.random.RandomInt( 100 ) > 10 ) {
			return;
		}
	}

	if ( gunTargetEntNum != -1 ) {
		if ( EntityIsClient( gunTargetEntNum, true ) || botWorld->gameLocalInfo.inWarmup ) { //mal: always unload on enemies, even if not OUR enemy. In warmup - shoot everyone.
			botWantsToAttack = true;
		} else if ( EntityIsClient( gunTargetEntNum, false ) ) { //mal: never shoot a teammate
			shotIsBlocked = true;
		} else if ( EntityIsVehicle( gunTargetEntNum, true, true ) || botWorld->gameLocalInfo.inWarmup ) { //mal: enemy vehicles are OK to shoot, as long as theres someone in them. In warmup, will shoot any vehicle.
			if ( useLockon ) {
				botUcmd->botCmds.altAttackOn = true;
				if ( botInfo->targetLockEntNum != -1 && botInfo->targetLocked != false ) {
                    botWantsToAttack = true;
				}
			} else {
				botWantsToAttack = true;
			}
		} else if ( EntityIsVehicle( gunTargetEntNum, false, false ) ) { //mal: never shoot friendly vehicles.
			shotIsBlocked = true;
		} else if ( EntityIsDeployable( gunTargetEntNum, false ) ) { //mal: deployable in the way.
			shotIsBlocked = true;
		} else if ( gunTargetEntNum == ENTITYNUM_WORLD ) {
			shotIsBlocked = true;
		} else {
			botWantsToAttack = true;
		}
	}

	if ( !shotIsBlocked ) {
		shotIsBlockedCounter = 0;
	} else {
		shotIsBlockedCounter++;
	}

	if ( !botWantsToAttack ) {
		return;
	}

	if ( testFireShot ) {
		if ( botInfo->weapInfo.weapon == SMG ) {
			timeOnTarget = botWorld->gameLocalInfo.time + 1500; //mal: we want to "test" if our target returns a "ping", so shoot, wait a sec, then blast them!
		}
		testFireShot = false;
	} //mal: no point if we dont have a SMG type weapon.

	if ( botInfo->weapInfo.weapon == KNIFE ) {
		if ( enemyInfo.enemyDist < 125.0f ) {
			botUcmd->botCmds.attack = true;
			botUcmd->botCmds.constantFire = true;
		}
		return;
	}

	if ( ( botInfo->weapInfo.weapon == GRENADE || botInfo->weapInfo.weapon == EMP ) && botInfo->weapInfo.hasNadeAmmo ) {
		bool quickNade = false;

		if ( enemyInfo.enemyFacingBot ) {
			quickNade = true;
		}

		Bot_ThrowGrenade( botWorld->clientInfo[ enemy ].origin, quickNade );
		return;
	}

	if ( botInfo->weapInfo.weapon == BINOCS ) {
		botUcmd->botCmds.attack = true;
		botUcmd->botCmds.constantFire = true;
		return;
	}

	if ( botInfo->weapInfo.weapon == AIRCAN ) {
		if ( ClassWeaponCharged( AIRCAN ) ) {
			Bot_UseCannister( AIRCAN, botWorld->clientInfo[ enemy ].origin );
			return;
		} else {
			weapSwitchTime = 0;
			useAircanOnEnemy = false;
			return;
		}
	}

	botUcmd->botCmds.attack = true;

	if ( botInfo->weapInfo.weapon == PISTOL && botInfo->team == STROGG && botInfo->classType == SOLDIER ) {
		botUcmd->botCmds.constantFire = true;
		return;
	}

	if ( botInfo->weapInfo.weapon == SNIPERRIFLE && botInfo->weapInfo.isFiringWeap ) {
		timeOnTarget = botWorld->gameLocalInfo.time + 900; //mal: wait for nearly a second before fire again, so aim isn't all wonky.
	}
}

/*
================
idBotAI::CheckClientAttacker

check if clientNum has been attacked recently.
Returns the client who did the attacking.
================
*/
int idBotAI::CheckClientAttacker( int clientNum, int checkTimeInSeconds ) {
	int attacker = -1;

	if ( ( botWorld->clientInfo[ clientNum ].lastAttackerTime + ( checkTimeInSeconds * 1000 ) ) > botWorld->gameLocalInfo.time ) {
        attacker = botWorld->clientInfo[ clientNum ].lastAttacker;

		if ( ClientIsDead( attacker ) ) {
			return -1;
		}

		if ( ClientIsIgnored( attacker ) ) {
			return -1;
		}

		if ( botWorld->clientInfo[ attacker ].team == botWorld->clientInfo[ clientNum ].team ) {
			return -1;
		}

		idVec3 vec = botWorld->clientInfo[ attacker ].origin - botInfo->origin;
        
		if ( vec.LengthSqr() > Square( botWorld->botGoalInfo.botSightDist ) ) {
            return -1;
		}
	}

	return attacker;
}

/*
================
idBotAI::Bot_CanBackStabClient
================
*/
bool idBotAI::Bot_CanBackStabClient( int clientNum, float backStabDist ) { 
	int travelFlags = ( botInfo->team == GDF ) ? TFL_VALID_GDF : TFL_VALID_STROGG;
	aasTraceFloor_t trace;
	const clientInfo_t& player = botWorld->clientInfo[ clientNum ];
	idVec3 end = player.viewOrigin; //player.origin;

	end += ( -backStabDist * player.viewAxis[ 0 ] );

	travelFlags &= ~TFL_WALKOFFLEDGE;
	
	botAAS.aas->TraceFloor( trace, player.viewOrigin /*player.origin*/, player.areaNum, end, travelFlags );

	if ( botThreadData.AllowDebugData() ) {
		gameRenderWorld->DebugLine( colorRed, player.viewOrigin, end );
	}

	if ( trace.fraction >= 1.0f ) {
		botBackStabMoveGoal = end;
        return true;
	}
	
	return false;
}

/*
================
idBotAI::ClientHasAttackedMate

Has this client attacked any of our teammates recently?
Can be used to spot coverts who run around knifing teammates or anyone who might be causing trouble. 
Can be filtered to only check if important mates were attacked.
================
*/
bool idBotAI::ClientHasAttackedTeammate( int clientNum, bool criticalOnly, int time ) {

	int i;

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.team != botInfo->team ) {
			continue;
		}

		if ( criticalOnly != false ) {
			if ( botInfo->team == GDF ) {
				if ( playerInfo.classType == botWorld->botGoalInfo.team_GDF_criticalClass && playerInfo.lastAttackerTime + time > botWorld->gameLocalInfo.time && playerInfo.lastAttacker == clientNum ) {
					return true;
				}
			} else {
				if ( playerInfo.classType == botWorld->botGoalInfo.team_STROGG_criticalClass && playerInfo.lastAttackerTime + time > botWorld->gameLocalInfo.time && playerInfo.lastAttacker == clientNum ) {
					return true;
				}
			}
		} else {
			if ( playerInfo.lastAttackerTime + time > botWorld->gameLocalInfo.time && playerInfo.lastAttacker == clientNum ) {
				return true;
			}
		}
	}
	return false;
}

/*
================
idBotAI::DisguisedKillerInArea

Is there a disguised killer ( covert ops ) in the area thats been killing some of our teammates?
If so, the bot should be more aware of its surroundings, and not skip any part of its
enemy checks.
================
*/
bool idBotAI::DisguisedKillerInArea() {

	int i;

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( !playerInfo.isDisguised ) {
			continue;
		}

		if ( playerInfo.team == botInfo->team ) {
			continue;
		}

		if ( ClientHasAttackedTeammate( i , false, 3000 ) ) {
			return true;
		}

	}

	return false;
}

/*
================
idBotAI::ClientIsDefusingOurTeamCharge

Is this client trying to defuse our team's charge? In which case, he should become a priority target!
================
*/
bool idBotAI::ClientIsDefusingOurTeamCharge( int clientNum ) {

	int botActionNum;
	const clientInfo_t& playerInfo = botWorld->clientInfo[ clientNum ];
	idVec3 vec;

	if ( playerInfo.classType != ENGINEER ) {
		return false;
	}

	if ( botInfo->team == GDF ) {
        botActionNum = botWorld->botGoalInfo.team_GDF_PrimaryAction;

		if ( botActionNum == -1 ) {
			return false;
		}

		if ( botThreadData.botActions[ botActionNum ]->GetHumanObj() == ACTION_HE_CHARGE ) {
			if ( botThreadData.botActions[ botActionNum ]->ArmedChargesInsideActionBBox( -1 ) ) {
				vec = botThreadData.botActions[ botActionNum ]->GetActionOrigin() - playerInfo.origin;

				if ( vec.LengthSqr() < Square( 1500.0f ) ) {
					return true;
				}
			}
		}
	} else {
		botActionNum = botWorld->botGoalInfo.team_STROGG_PrimaryAction;

		if ( botActionNum == -1 ) {
			return false;
		}

		if ( botThreadData.botActions[ botActionNum ]->GetStroggObj() == ACTION_HE_CHARGE ) {
			if ( botThreadData.botActions[ botActionNum ]->ArmedChargesInsideActionBBox( -1 ) ) {
				vec = botThreadData.botActions[ botActionNum ]->GetActionOrigin() - playerInfo.origin;

				if ( vec.LengthSqr() < Square( 1500.0f ) ) {
					return true;
				}
			}
		}
	}

	return false;
}

/*
================
idBotAI::Bot_PickChaseType

Choose how the bot will chase the enemy. 
If the enemy is making a lot of noise, use that info to chase the enemy, ONLY if the bot is highskilled.
================
*/
void idBotAI::Bot_PickChaseType() {
	bool isAudible;
	idVec3 vec;

	vec = enemyInfo.enemy_LS_Pos - botInfo->origin;

	isAudible = ClientIsAudibleToBot( enemy );

	if ( vec.LengthFast() < ENEMY_CHASE_DIST && ( ( isAudible && botThreadData.GetBotSkill() == BOT_SKILL_NORMAL ) || ( botThreadData.random.RandomInt( 100 ) > 40 && botThreadData.GetBotSkill() == BOT_SKILL_EXPERT ) ) ) {
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseUnseenEnemy;
	} else {
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ChaseEnemy;
	}
}

/*
================
idBotAI::Bot_CheckIfShouldInvestigateNoise

Checks to see if a bot should go investigate a client making some noise. Wont do this if busy doing something
more important!
================
*/
int idBotAI::Bot_ShouldInvestigateNoise( int clientNum ) {

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
		return 0;
	} //mal: dumb bots dont worry about this kind of stuff!

	if ( ClientIsIgnored( clientNum ) ) {
		return 0;
	}

	if ( aiState == LTG && ltgType == HUNT_GOAL ) { //mal: already doing something about this!
		return 0;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ clientNum ];

	int travelTime;
	
	if ( !Bot_LocationIsReachable( false, playerInfo.origin, travelTime ) ) { //mal: if we can't reach you, can't really investigate you.
		return 0;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO && !botWorld->botGoalInfo.gameIsOnFinalObjective && !playerInfo.isBot ) { //mal: dont worry about noises made by human players in training mode, unless its the final obj...
		return 0;
	}

	if ( botInfo->isActor ) { //mal: dont ever leave the player!
		return 0;
	}

	if ( ClientIsDefusingOurTeamCharge( clientNum ) ) {
		return 3;
	}

	if ( aiState == LTG && ( ltgType != ROAM_GOAL && ltgType != CAMP_GOAL && ltgType != INVESTIGATE_ACTION ) ) {
		return 0;
	}

	if ( aiState == NBG && ( nbgType != CAMP && nbgType != DESTROY_DANGER && nbgType != SNIPE && nbgType != DEFENSE_CAMP ) ) {
		return 0;
	}

	if ( ClientIsMarkedForDeath( clientNum, true ) ) {
		if ( Bot_LTGIsAvailable( clientNum, ACTION_NULL, HUNT_GOAL, 1 ) ) {
			Bot_AddDelayedChat( botNum, ACKNOWLEDGE_YES, 2 );
			return 3;
		}
	}

	if ( stayInPosition == true ) { //mal: we've decided to hold our ground ( low skills bots do this always, high skill sometimes to be unpredictable ), so leave.
		return 0;
	}

	if ( ClientHasObj( clientNum ) ) {
		return 3;
	}

	if ( Client_IsCriticalForCurrentObj( clientNum, 1500.0f ) ) {
		return 3;
	}

	if ( ClientIsDangerous( clientNum ) ) { //mal: we're not gonna sit back and let someone camp us!
		return 3;
	}

	if ( playerInfo.lastAttackClient > -1 && playerInfo.lastAttackClient < MAX_CLIENTS && playerInfo.lastAttackClientTime + 5000 > botWorld->gameLocalInfo.time ) {
		if ( Client_IsCriticalForCurrentObj( playerInfo.lastAttackClient, -1.0f ) ) {
			return 2;
		}
	}

	return 1;
}

/*
================
idBotAI::Bot_SetTimeOnTarget

Sets how long the bot should delay before he fires his weapon at the enemy.
================
*/
void idBotAI::Bot_SetTimeOnTarget( bool inVehicle ) {
	bool inFront = InFrontOfClient( botNum, botWorld->clientInfo[ enemy ].origin );

	if ( inVehicle ) {
		if ( botVehicleInfo->driverEntNum == botNum ) {
            timeOnTarget = botWorld->gameLocalInfo.time + 1500;
		} else {
			timeOnTarget = botWorld->gameLocalInfo.time + 150;
		}
	} else {
		if ( botInfo->weapInfo.primaryWeapon == SNIPERRIFLE && !botInfo->weapInfo.isScopeUp ) { //mal: give us time to get scope up.
			timeOnTarget = botWorld->gameLocalInfo.time + 1500;
			return;
		}

		if ( botWorld->gameLocalInfo.botAimSkill >= 3 ) {
			timeOnTarget = 50;
			return;
		}

		if ( !inFront ) {
			if ( botWorld->gameLocalInfo.botAimSkill == 0 ) {
				timeOnTarget = botWorld->gameLocalInfo.time + 950;
			} else {
				timeOnTarget = botWorld->gameLocalInfo.time + 250;
			}
		} else {
			if ( botWorld->gameLocalInfo.botAimSkill == 0 ) {
				timeOnTarget = botWorld->gameLocalInfo.time + 550;
			} else {
				timeOnTarget = botWorld->gameLocalInfo.time + 150;
			}
		}
	}
}

/*
================
idBotAI::Client_HasMultipleAttackers
================
*/
bool idBotAI::Client_HasMultipleAttackers( int clientNum ) {
	int n = 0;
	const clientInfo_t& client = botWorld->clientInfo[ clientNum ];

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( i == clientNum ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ clientNum ];

		if ( player.inGame == false || player.team == NOTEAM || player.team != client.team ) {
			continue;
		}

		if ( player.lastAttackClient != clientNum || player.lastAttackClientTime + 5000 < botWorld->gameLocalInfo.time ) {
			continue;
		}

		n++;
	}

	if ( n > 1 ) {
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_IsNearTeamGoalUnderAttack

In the bots travels, he may move near a team goal thats currently under attack. Its possible that when the event of the goal being under attack was first sent,
the bot was too far away, or too busy to answer it. If so, he should stop doing whatever hes doing, and move to that goal to defend it.
================
*/
bool idBotAI::Bot_IsNearTeamGoalUnderAttack() {

	if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) { //mal: only the smartest bots will do this.
		return false;
	}

	if ( Client_IsCriticalForCurrentObj( botNum, -1 ) && botWorld->botGoalInfo.attackingTeam == botInfo->team ) {
		return false;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO && !botWorld->botGoalInfo.gameIsOnFinalObjective ) { //mal: dont worry about the obj if we're in training mode, unless its the final one...
		return false;
	}

	if ( aiState == LTG && ( ltgType == DEFUSE_GOAL || ltgType == PLANT_GOAL || ltgType == BUILD_GOAL || ltgType == HACK_GOAL || ltgType == FOLLOW_TEAMMATE_BY_REQUEST || ltgType == HUNT_GOAL ) ) {
		return false;
	}

	if ( aiState == NBG && ( nbgType == BUILD || nbgType == HACK || nbgType == DEFUSE_BOMB || nbgType == PLANT_BOMB || nbgType == INVESTIGATE_CAMP || nbgType == GRAB_SUPPLIES || nbgType == REVIVE_TEAMMATE || nbgType == TK_REVIVE_TEAMMATE || nbgType == SUPPLY_TEAMMATE ) ) {
		return false;
	}

	if ( ClientHasObj( botNum ) ) {
		return false;
	}

	if ( botInfo->isActor ) {
		return false;
	}

	if ( botVehicleInfo != NULL ) { //mal: if we're in a vehicle, ignore this goal.
		return false;
	}

#ifdef STROGG_INSTANT_REVIVE
	if ( aiState == NBG && ( nbgType == REVIVE_TEAMMATE || nbgType == TK_REVIVE_TEAMMATE || nbgType == SUPPLY_TEAMMATE ) ) { //mal: medics need a chance to finish their job!
		return false;
	}

	if ( botInfo->classType == MEDIC && Bot_HasTeamWoundedInArea( true ) ) { 
		return false;
	}
#else
	if ( aiState == NBG && nbgType == REVIVE_TEAMMATE && botInfo->team == GDF ) { //mal: medics need a chance to finish their job! Strogg take too long to revive.
		return false;
	}

	if ( botInfo->classType == MEDIC && botInfo->team == GDF && Bot_HasTeamWoundedInArea( true ) ) { //mal: strogg medics take too long to revive to make this worthwhile
		return false;
	}
#endif

	if ( botInfo->classType == ENGINEER && nextObjChargeCheckTime < botWorld->gameLocalInfo.time && Bot_CheckChargeExistsOnObjInWorld() ) {
		nextObjChargeCheckTime = botWorld->gameLocalInfo.time + 5000;
		Bot_ResetState( false, true );
		return true;
	}

	if ( aiState == LTG && ltgType == PROTECT_CHARGE ) { //mal: if we're guarding, only heal those close to our goal.
		if ( botInfo->classType == MEDIC && Bot_HasTeamWoundedInArea( false, MEDIC_RANGE_BUSY ) ) {
			return false;
		}
	}

	if ( aiState == LTG && ( ltgType == INVESTIGATE_ACTION || ltgType == PROTECT_CHARGE ) ) { //mal: already doing something about this, so keep doing it.
		return true;
	} //mal: this needs to be checked last.

	if ( botInfo->team == botWorld->botGoalInfo.attackingTeam && ignorePlantedChargeTime < botWorld->gameLocalInfo.time ) { //mal: if we're on the attacking team, check our charge.
		if ( Bot_LTGIsAvailable( -1, -1, PROTECT_CHARGE, MAX_NUM_DEFEND_CHARGE_CLIENTS ) ) {
			for( int i = 0; i < MAX_CHARGES; i++ ) {
				const plantedChargeInfo_t& charge = botWorld->chargeInfo[ i ];

				if ( charge.entNum == 0 ) {
					continue;
				}

				if ( charge.team != botInfo->team ) {
					continue;
				}

				if ( !charge.isOnObjective ) {
					continue;
				}

				if ( charge.areaNum == 0 ) {
					continue;
				}

				if ( charge.state != BOMB_ARMED ) {	
					continue;
				}

				idVec3 vec = charge.origin - botInfo->origin;

				float awareOfChargeDist = 2500.0f;

				if ( vec.LengthSqr() > Square( awareOfChargeDist ) ) {
					continue;
				}

				int matesInArea = ClientsInArea( botNum, charge.origin, 300.0f, botInfo->team, NOCLASS, false, false, false, false, true );

				if ( matesInArea > MAX_NUM_DEFEND_CHARGE_CLIENTS ) {
					continue;
				} //mal: already some ppl there, so lets do something else.

				int enemyEngineersInArea = ClientsInArea( botNum, charge.origin, awareOfChargeDist, ( botInfo->team == GDF ) ? STROGG : GDF, ENGINEER, false, false, false, false, true );
	
				if ( enemyEngineersInArea == 0 ) {
					continue;
				} //mal: noone there to defuse our charge, so why worry about it?

				Bot_ResetState( true, true );
				aiState = LTG;
				ltgType = PROTECT_CHARGE;
				ltgTarget = charge.entNum;
				ltgTargetSpawnID = charge.spawnID;
				ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
				LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_ProtectCharge;
				return true;
			}
		}
	} else {
		for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {
			if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
				continue;
			}	

			if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_NULL ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) != ACTION_DEFUSE && botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) != ACTION_PREVENT_BUILD &&
				botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) != ACTION_PREVENT_HACK && botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) != ACTION_HE_CHARGE &&
				botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) != ACTION_PREVENT_STEAL && botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) != ACTION_PREVENT_DELIVER ) {
					continue;
			}

			if ( !botThreadData.botActions[ i ]->ActionIsPriority() ) {
				if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_HE_CHARGE || botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_DEFUSE && botInfo->classType != ENGINEER )  {
					continue;
				}
			}

			if ( ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_HE_CHARGE || botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_DEFUSE ) && !botThreadData.botActions[ i ]->ArmedChargesInsideActionBBox( -1 ) ) {
				continue;
			}

			int enemiesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), BOT_INVESTIGATE_RANGE, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, false, false );

			if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_PREVENT_BUILD && ( botThreadData.botActions[ i ]->GetActionState() == ACTION_STATE_NORMAL && enemiesInArea == 0 ) ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_PREVENT_HACK && ( botThreadData.botActions[ i ]->GetActionState() == ACTION_STATE_NORMAL && enemiesInArea == 0 ) ) {
				continue;
			}
		
			if ( ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_PREVENT_STEAL || botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_PREVENT_DELIVER ) && enemiesInArea == 0 ) {
				continue;
			}

			if ( lastCheckActionTime > botWorld->gameLocalInfo.time ) { //mal: don't repeat ourselves, unless there is a critical enemy nearby
				if ( botThreadData.GetBotSkill() != BOT_SKILL_EXPERT ) { //mal: only the smartest bots will do this.
					continue;
				}

				int criticalEnemiesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), CRITICAL_ENEMY_CLOSE_TO_GOAL_RANGE, ( botInfo->team == GDF ) ? STROGG : GDF, TeamCriticalClass( ( botInfo->team == GDF ) ? STROGG : GDF ), false, false, false, false, false, true );

				if ( criticalEnemiesInArea == 0 ) {
					continue;
				}
			}

			int matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 300.0f, botInfo->team, NOCLASS, false, false, false, false, true );

			if ( matesInArea > MIN_NUM_INVESTIGATE_CLIENTS ) {
				continue;
			} //mal: already some ppl there, so lets do something else.

			idVec3 vec = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;

			if ( vec.LengthSqr() > Square( BOT_INVESTIGATE_RANGE ) ) { //mal: its too far away, we'll never make it in time!
				continue;
			}

			Bot_ResetState( true, true );
			actionNum = i;
			ltgTime = botWorld->gameLocalInfo.time + 30000;
			lastCheckActionTime = botWorld->gameLocalInfo.time + 20000; //mal: dont do this again for a while.
			aiState = LTG;
			ltgType = INVESTIGATE_ACTION;
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_InvestigateGoal;
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::DisguisedClientIsActingOdd

Disguised clients only have access to the knife/gun of the disguised client they impersonate. Most clients won't ever run around with a knife in hand, so if a client
is walking around with a knife, acting goofy, a high skill bot may "spot" them and attack. Also, look around and see if the disguised client he impersonates is nearby.
================
*/
bool idBotAI::DisguisedClientIsActingOdd( int clientNum ) {

	if ( botWorld->gameLocalInfo.botSkill < BOT_SKILL_EXPERT ) {
		return false;
	}

	if ( ignoreSpyTime > botWorld->gameLocalInfo.time ) {
		return false;
	}

	if ( Bot_IsBusy() ) {
		return false;
	}

	ignoreSpyTime = botWorld->gameLocalInfo.time + 1500;

	const clientInfo_t& player = botWorld->clientInfo[ clientNum ];

	int attackingClient;

	if ( Bot_TeammateHasClientAsEnemy( clientNum, attackingClient ) ) { //mal: if bot teammate has covert as enemy - the covert becomes our enemy too.
		return true;
	}

	idVec3 vec = player.origin - botInfo->origin;

	if ( vec.LengthSqr() > Square( COVERT_SIGHT_DIST ) ) {
		return false;
	}

	if ( player.isPanting && !botInfo->isPanting && botInfo->xySpeed < WALKING_SPEED ) {
		return true;
	} //mal: high skill bot will "hear" you making your teams "heavy panting" noise, and will hunt you down.

	if ( !InFrontOfClient( botNum, player.origin ) ) {
		return false;
	}

	float dist = vec.LengthSqr();

	const clientInfo_t& mate = botWorld->clientInfo[ player.disguisedClient ];

	vec = mate.origin - botInfo->origin;

	if ( vec.LengthSqr() < Square( COVERT_SIGHT_DIST ) ) {
		if ( InFrontOfClient( botNum, mate.origin ) ) {
			testFireShot = true;
			return true;
		}
	} //mal: dirty spy - I see you!!

	if ( botInfo->xySpeed >= RUNNING_SPEED ) { //mal: when we're on the move, kinda hard to do this.
		return false;
	}

	int randChance = 90;
	
	if ( player.covertWarningTime + 5000 > botWorld->gameLocalInfo.time ) {
		randChance = 70;
	}

	if ( player.weapInfo.weapon == KNIFE ) { //mal: kinda odd to see someone running around with a knife out. Hmmmm......
		if ( botThreadData.random.RandomInt( 100 ) > randChance ) {
			testFireShot = true;
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::ClientIsDangerous

Target campers, and clients who are on a killing spree.
================
*/
bool idBotAI::ClientIsDangerous( int clientNum ) {

	if ( !ClientIsValid( clientNum, -1 ) ) {
		return false;
	}

	const clientInfo_t& player = botWorld->clientInfo[ clientNum ];

	if ( player.isBot ) {
		return false;
	}

	if ( player.killsSinceSpawn < KILLING_SPREE && !player.isCamper ) {
		return false;
	}

	return true;
}

/*
================
idBotAI::ClientIsMarkedForDeath

Checks to see if this client, or the vehicle hes in, has been marked for death.
================
*/
bool idBotAI::ClientIsMarkedForDeath( int clientNum, bool clearRequest ) {
	if ( !ClientIsValid( clientNum, -1 ) ) {
		return false;
	}

	bool isMarked = false;
	
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		if ( i == botNum ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( player.isBot ) {
			continue;
		}

		if ( player.killTargetNum == -1 ) {
			continue;
		}

		if ( player.killTargetUpdateTime + MAX_TARGET_TIME < botWorld->gameLocalInfo.time ) {
			continue;
		}

		if ( player.killTargetNum < MAX_CLIENTS && player.killTargetNum != clientNum ) {
			continue;
		}

		entityTypes_t entityType = FindEntityType( player.killTargetNum, player.killTargetSpawnID );

		if ( entityType == ENTITY_NULL || entityType == ENTITY_DEPLOYABLE ) {
			continue;
		}

		if ( entityType == ENTITY_VEHICLE && botWorld->clientInfo[ clientNum ].proxyInfo.entNum != player.killTargetNum ) {
			continue;
		}

		if ( clearRequest ) {
			botUcmd->ackKillForClient = i;
		}

		isMarked = true;
		break;
	}

	return isMarked;
}

/*
================
idBotAI::DeployableIsMarkedForDeath

Checks to see if this deployable has been marked for death.
================
*/
bool idBotAI::DeployableIsMarkedForDeath( int entNum, bool clearRequest ) {
	deployableInfo_t deployable;

	if ( GetDeployableInfo( false, entNum, deployable ) ) {
		return false;
	}

	bool isMarked = false;
	
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		if ( i == botNum ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( player.isBot ) {
			continue;
		}

		if ( player.killTargetNum == -1 ) {
			continue;
		}

		if ( player.killTargetUpdateTime + MAX_TARGET_TIME < botWorld->gameLocalInfo.time ) {
			continue;
		}

		if ( player.killTargetNum < MAX_CLIENTS ) {
			continue;
		}

		entityTypes_t entityType = FindEntityType( player.killTargetNum, player.killTargetSpawnID );

		if ( entityType == ENTITY_NULL || entityType == ENTITY_VEHICLE || entityType == ENTITY_PLAYER ) {
			continue;
		}

		if ( deployable.entNum != player.killTargetNum ) {
			continue;
		}

		if ( clearRequest ) {
			botUcmd->ackKillForClient = i;
		}

		isMarked = true;
		break;
	}

	return isMarked;
}

/*
================
idBotAI::Flyer_FindEnemy

A find enemy that only works for the flyer hive.
================
*/
int idBotAI::Flyer_FindEnemy( float range) {
	int enemyNum = -1;
	float closest = idMath::INFINITY;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( i == botNum ) {
			continue; //mal: dont try to fight ourselves!
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.inLimbo ) {
			continue;
		}

		if ( playerInfo.isDisguised ) {
			continue;
		}

		if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO && !playerInfo.isBot ) { //mal: dont harass humans in training mode.
			continue;
		}

		if ( playerInfo.isNoTarget ) {
			continue;
		}

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { 
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

		idVec3 vec = playerInfo.viewOrigin - botInfo->weapInfo.covertToolInfo.origin;
		float distSqr = vec.LengthSqr();

	    if ( distSqr > Square( range ) ) {
			continue;
		}

		bool clientHasLotsOfFriendsInArea = ( ClientsInArea( botNum, playerInfo.origin, 700.0f, GDF, NOCLASS, false, false, false, true, true ) > 0 ) ? true : false;
		bool clientIsDangerous = ClientIsDangerous( i );

		if ( clientHasLotsOfFriendsInArea ) { //mal: juicy target!
			distSqr = 500.0f;
		} else if ( Client_IsCriticalForCurrentObj( i, 3000.0f ) ) { //mal: juicier target!
			distSqr = 400.0f;
		} else if ( clientIsDangerous ) { //mal: juiciest target!
			distSqr = 100.0f;
		}
		
		if ( distSqr < closest ) {
			enemyNum = i;
			closest = distSqr;
		}
	}

	return enemyNum;
}

/*
============
idBotAI::Bot_CanAttackVehicles
============
*/
bool idBotAI::Bot_CanAttackVehicles() {
//mal_TODO: add a check for engs who have the grenade launcher.

	if ( botVehicleInfo == NULL ) {
		if ( ( botInfo->classType != SOLDIER || botInfo->weapInfo.primaryWeapon != ROCKET ) && ( botInfo->classType != FIELDOPS || !Bot_HasWorkingDeployable( false, ROCKET_ARTILLERY ) ) ) {
			return false;
		}
	} else {
		if ( Bot_IsInHeavyAttackVehicle() ) {
			return false;
		}
	}

	return true;
}

/*
============
idBotAI::Bot_HasEnemySniperInArea
============
*/
bool idBotAI::Bot_HasEnemySniperInArea( float range ) {
	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
		return false;
	}

	bool hasSniper = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.isBot ) {
			continue;
		}

		if ( player.team == botInfo->team || player.health <= 0 ) {
			continue;
		}

		if ( player.weapInfo.primaryWeapon != SNIPERRIFLE || !player.weapInfo.primaryWeapHasAmmo ) {
			continue;
		}

		if ( !player.isCamper && player.killsSinceSpawn < KILLING_SPREE ) {
			continue;
		}

		if ( player.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			continue;
		}

		idVec3 vec = player.origin - botInfo->origin;
	
		if ( vec.LengthSqr() > Square( range ) ) {
			continue;
		}

		idVec3 dangerOrg = player.viewOrigin;
		dangerOrg += ( vec.LengthFast() * player.viewAxis[ 0 ] );

		vec = dangerOrg - botInfo->origin;

		if ( vec.LengthSqr() > Square( MAX_SNIPER_CROSSHAIR_DANGER_DIST ) ) {
			continue;
		}

		hasSniper = true;
		break;
	}

	return hasSniper;
}

/*
============
idBotAI::Bot_IsUnderAttackByAPT
============
*/
bool idBotAI::Bot_IsUnderAttackByAPT( idVec3& turretLocation ) {
	bool isUnderAttack = false;

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
		const deployableInfo_t& deployable = botWorld->deployableInfo[ i ];

		if ( deployable.entNum == 0 ) {
			continue;
		}

		if ( deployable.health <= 0 ) {
			continue;
		}

		if ( !deployable.inPlace ) {
			continue;
		}

		if ( deployable.ownerClientNum == -1 ) {
			continue;
		}

		if ( deployable.type != APT ) {
			continue;
		}

		if ( deployable.enemyEntNum != botNum ) {
			continue;
		}

		turretLocation = deployable.origin;
		turretLocation.z += DEPLOYABLE_PATH_ORIGIN_OFFSET;
		isUnderAttack = true;
		break;
	}

	return isUnderAttack;
}

/*
============
idBotAI::Bot_TeammateHasClientAsEnemy
============
*/
bool idBotAI::Bot_TeammateHasClientAsEnemy( int clientNum, int& attackingClient ) {
	bool isTargeted = false;
	attackingClient = -1;


	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		if ( i == botNum ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.isBot == false ) {
			continue;
		}

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( player.health <= 0 ) {
			continue;
		}

		if ( botThreadData.bots[ i ] == NULL ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetEnemyNum() != clientNum ) {
			continue;
		}

		isTargeted = true;
		attackingClient = i;
		break;
	}

	return isTargeted;
}

/*
================
idBotAI::ClientHasAttackedActorsMate

================
*/
bool idBotAI::ClientHasAttackedActorsMate( int clientNum, int time ) {
	if ( !ClientIsValid( botThreadData.actorMissionInfo.targetClientNum, -1 ) ) {
		return false;
	}

	if ( !ClientIsValid( clientNum, -1 ) ) {
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ clientNum ];

	if ( playerInfo.lastAttackerTime + time > botWorld->gameLocalInfo.time && playerInfo.lastAttacker == botThreadData.actorMissionInfo.targetClientNum ) {
		return true;
	}

	return false;
}

/*
============
idBotAI::Bot_ShouldIgnoreEnemies

The latest versions of ETQW make it worthwhile to make suicide runs on objs. Update the bots to let them do so as well.
============
*/
bool idBotAI::Bot_ShouldIgnoreEnemies() {
	if ( ClientHasObj( botNum ) ) {
		if ( botWorld->botGoalInfo.deliverActionNumber > -1 && botWorld->botGoalInfo.deliverActionNumber < botThreadData.botActions.Num() ) {
			idVec3 vec = botThreadData.botActions[ botWorld->botGoalInfo.deliverActionNumber ]->GetActionOrigin() - botInfo->origin;

			if ( vec.LengthSqr() < Square( 1500.0f ) ) {
				return true;
			}
		}
	}

	if ( Client_IsCriticalForCurrentObj( botNum, 900.0f ) && botInfo->classType != SOLDIER ) {
		return true;
	}

	switch ( botInfo->classType ) {

		case MEDIC: {
			if ( aiState == NBG && nbgType == REVIVE_TEAMMATE ) {
				return true;
			}
			break;
		}

		case ENGINEER: {
			if ( aiState == NBG && ( nbgType == BUILD || nbgType == DEFUSE_BOMB || nbgType == FIXING_MCP ) ) {
				return true;
			}
			break;
		}

		case COVERTOPS: {
			if ( aiState == NBG && nbgType == HACK ) {
				return true;
			}
			break;
		}

		case SOLDIER: {
			if ( aiState == NBG && nbgType == PLANT_BOMB && !ClientHasChargeInWorld( botNum, true, ACTION_NULL ) ) {
				return true;
			}
			break;
		}
	}

	return false;
}

/*
============
idBotAI::Bot_CheckIfEnemyHasUsInTheirSightsWhenInAirVehicle
============
*/
bool idBotAI::Bot_CheckIfEnemyHasUsInTheirSightsWhenInAirVehicle() {
	if ( botWorld->gameLocalInfo.botSkill != BOT_SKILL_EXPERT ) {
		return false;
	}

	bool hasEnemy = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team == botInfo->team || player.health <= 0 ) {
			continue;
		}

		if ( player.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			proxyInfo_t enemyVehicle;
			GetVehicleInfo( player.proxyInfo.entNum, enemyVehicle );

			if ( enemyVehicle.entNum == 0 ) {
				continue;
			}

			if ( enemyVehicle.type != TITAN && enemyVehicle.type != DESECRATOR && enemyVehicle.type != GOLIATH && enemyVehicle.type != TROJAN ) {
				continue;
			}

			if ( enemyVehicle.type == TROJAN && enemyVehicle.driverEntNum == i ) { //mal: dont worry about trojan driver
				continue;
			}

			if ( ( enemyVehicle.type == TITAN || enemyVehicle.type == DESECRATOR ) && enemyVehicle.driverEntNum != i ) { //mal: dont worry about tank gunners
				continue;
			}

			idVec3 vec = player.origin - botInfo->origin;
			idVec3 dangerOrg = player.viewOrigin;
			dangerOrg += ( vec.LengthFast() * player.viewAxis[ 0 ] );

			vec = dangerOrg - botInfo->origin;

			if ( vec.LengthSqr() > Square( 500.0f ) ) {
				continue;
			}
		} else {
			if ( player.weapInfo.weapon != ROCKET && player.weapInfo.weapon != HEAVY_MG ) {
				continue;
			}

			idVec3 vec = player.origin - botInfo->origin;
			float distToEnemy = vec.LengthFast();

#ifdef _XENON
			float avoidDist = ( player.weapInfo.weapon == ROCKET ) ? FLYER_WORRY_ABOUT_ROCKETS_MAX_DIST : 3500.0f; //mal: make it a bit easier on the Xbox to take out flyers.
#else
			float avoidDist = 3500.0f;
#endif

			if ( distToEnemy > avoidDist ) {
				continue;
			}

			idVec3 dangerOrg = player.viewOrigin;
			dangerOrg += ( distToEnemy * player.viewAxis[ 0 ] );

			vec = dangerOrg - botInfo->origin;

			if ( vec.LengthSqr() > Square( 300.0f ) ) {
				continue;
			}
		}

		hasEnemy = true;
		break;
	}

	return hasEnemy;
}