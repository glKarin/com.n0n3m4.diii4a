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
idBotAI::Bot_FindBestCombatMovement

Finds the best combat movement for the bot while on foot.
================
*/
void idBotAI::Bot_FindBestCombatMovement() {		
	int result, k;
	int travelFlags = ( botInfo->team == GDF ) ? TFL_VALID_GDF : TFL_VALID_STROGG;

	combatMoveFailedCount = 0;
	combatMoveTime = -1;

	if ( enemy == -1 ) {
		assert( false );
		return;
	}

	if ( botInfo->usingMountedGPMG ) {
		COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Null_Move_Attack;
		return;
	}

	if ( hammerTime ) {
		COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
		return;
	}

	if ( botThreadData.GetBotSkill() == BOT_SKILL_DEMO ) { //mal: silly bot! Don't move around too much or be too hard to hit in training mode.
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			COMBAT_MOVEMENT_STATE = &idBotAI::Stand_Ground_Attack_Movement;
		} else {
			COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Run_And_Gun_Movement;
		}

		return;
	}

	if ( botInfo->weapInfo.weapon == KNIFE ) {
        COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Knife_Attack_Movement;
		return;
	}

	if ( botInfo->weapInfo.weapon == GRENADE || botInfo->weapInfo.weapon == EMP ) {
		COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Grenade_Attack_Movement;
		return;
	}
	
	if ( combatDangerExists ) {
		COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Avoid_Danger_Movement;
		return;
	}

	const clientInfo_t& enemyClient = botWorld->clientInfo[ enemy ];

	if ( botInfo->inWater && !enemyClient.inWater ) {
		COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Run_And_Gun_Movement;
		return;
	}

	if ( botInfo->weapInfo.weapon == SNIPERRIFLE ) {
		if ( enemyInfo.enemyDist > 900.0f ) {
			if ( Bot_CanProne( enemy ) ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Prone_Attack_Movement;
				return;
			} else if ( Bot_CanCrouch( enemy ) ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;
				return;
			} else {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
				return;
			}
		} else {
			if ( ( botInfo->lastAttacker != enemy || botInfo->lastAttackerTime + 3000 < botWorld->gameLocalInfo.time ) || !enemyInfo.enemyFacingBot ) {
				if ( Bot_CanCrouch( enemy ) ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;
					return;
				} else {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
					return;
				}
			} else {
                if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Hal_Strafe_Attack_Movement;
					return;
				} else {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Side_Strafe_Attack_Movement;
					return;
				}
			}
		}
	}

	if ( enemyClient.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: dont be a easy target if fighting someone in a vehicle.
		if ( enemyInfo.enemyDist < 4000 ) {
			result = botThreadData.random.RandomInt( 4 );

			if ( result == 0 ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crazy_Jump_Attack_Movement;
			} else if ( result == 1 ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Hal_Strafe_Attack_Movement;
			} else if ( result == 2 ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Side_Strafe_Attack_Movement;
			} else {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Circle_Strafe_Attack_Movement;
			}
			return;
		}
	}


	if ( enemyInfo.enemyDist > 1000.0f ) { //mal: if our enemy is a fair distance away, but we can't reach them, just stand or crouch.
		int travelTime;
		bool canReach = Bot_LocationIsReachable( false, enemyClient.aasOrigin, travelTime );

		if ( !canReach || travelTime > Bot_ApproxTravelTimeToLocation( botInfo->origin, enemyClient.origin, false ) * TRAVEL_TIME_MULTIPLY ) {
			if ( Bot_CanCrouch( enemy ) && botInfo->weapInfo.primaryWeapon != ROCKET ) {
                COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;
			} else {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
			}
			return;
		}
	}

//mal: next, check if we have the height advantage - in which case, we'll keep it! ONLY smarter bots will do this....
	if ( botThreadData.GetBotSkill() > BOT_SKILL_EASY && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
        if ( enemyInfo.enemyHeight < -150 && enemyInfo.enemyDist > 900.0f ) { //mal: if we have the height advantage, run some special checks!
			aasTraceFloor_t trace;
			idVec3 enemyOrg = enemyClient.origin;
			travelFlags &= ~TFL_WALKOFFLEDGE;

			botAAS.aas->TraceFloor( trace, botInfo->aasOrigin, botInfo->areaNum, enemyOrg, travelFlags );

			if ( trace.fraction < 1.0f ) {
				if ( Bot_CanCrouch( enemy ) && botInfo->weapInfo.primaryWeapon != ROCKET ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;
				} else {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
				}
				return;
			} else {

                result = botThreadData.random.RandomInt( 3 );

				if ( result == 0 ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Hal_Strafe_Attack_Movement;
					return;
				} else if ( result == 1 ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Side_Strafe_Attack_Movement;
					return;
				} else {
					if ( Bot_CanCrouch( enemy ) && botInfo->weapInfo.primaryWeapon != ROCKET ) {
						COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;	//mal: if can't crouch, find something better to do....
						return;
					}
				}
			}
		}
	}

	if ( botInfo->weapInfo.isReloading && ( botInfo->weapInfo.weapon != SNIPERRIFLE && enemyInfo.enemyDist > 900.0f ) ) {	//mal: what should the bot do while they're reloading their gun.

		if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) { //mal: low skill bots aren't too smart
			COMBAT_MOVEMENT_STATE = &idBotAI::Stand_Ground_Attack_Movement;
			return;
		}

		idVec3 shieldOrg;
		float distToShieldSqr = Bot_DistSqrToClosestForceShield( shieldOrg ); //mal: GDF and Strogg will use shields on the ground, if noone else is.

		if ( distToShieldSqr != -1.0f && distToShieldSqr <= Square( SHIELD_CONSIDER_RANGE ) ) {
			int clientsInArea = ClientsInArea( botNum, shieldOrg, 150.0f, NOTEAM, NOCLASS, false, false, false, false, false );

			if ( clientsInArea > 0 ) { //mal: shields aren't big enough for too many players
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_MoveTo_Shield_Attack_Movement;
				return;
			}
		}


		result = botThreadData.random.RandomInt( 4 );
		
//mal: the point here is we want to be constantly moving around and making ourselves a harder target, when we're so vulnerable.
		if ( result == 0 ) {
			COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crazy_Jump_Attack_Movement;
		} else if ( result == 1 ) {
			COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Hal_Strafe_Attack_Movement;
		} else if ( result == 2 ) {
			COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Side_Strafe_Attack_Movement;
		} else {
			COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Circle_Strafe_Attack_Movement;
		}

		return;
	}
 
	if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) { //mal: stupid bot! Just stand there and take your punishment. >:-D
		COMBAT_MOVEMENT_STATE = &idBotAI::Stand_Ground_Attack_Movement;
		return;
	}

	if ( botInfo->weapInfo.weapon == GRENADE || botInfo->weapInfo.weapon == EMP ) {
		COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crazy_Jump_Attack_Movement;
		return;
	}

	if ( botInfo->weapInfo.weapon == HEAVY_MG ) {
		if ( enemyInfo.enemyDist > 1000.0f && !Client_HasMultipleAttackers( botNum ) ) {
			if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
				if ( Bot_CanCrouch( enemy ) ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;
					return;
				} else {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
					return;
				}
			} else {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
				return;
			}
		} else {
			if ( !enemyInfo.enemyFacingBot && ( botInfo->lastAttacker != enemy || botInfo->lastAttackerTime + 3000 < botWorld->gameLocalInfo.time ) && !Client_HasMultipleAttackers( botNum ) ) {
				if ( Bot_CanCrouch( enemy ) ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;
					return;
				} else {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
					return;
				}
			} //mal: if we got the drop on you, just crouch down, or stand, and unload into you.

			if ( enemyInfo.enemyDist < 900.0f ) {
				k = 4;
			} else {
				k = 3;
			}

			result = botThreadData.random.RandomInt( k );

			if ( result == 0 ) {
		        COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Run_And_Gun_Movement;
			} else if ( result == 1 ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Hal_Strafe_Attack_Movement;
			} else if ( result == 2 ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Side_Strafe_Attack_Movement;
			} else {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Circle_Strafe_Attack_Movement; //mal: this will be skipped if enemy too far away.
			} 

			return;
		}
	}

	if ( botInfo->weapInfo.weapon == ROCKET ) {
		if ( enemyClient.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: always try to use rockets for vehicles
			if ( ( botInfo->lastAttacker != enemy || botInfo->lastAttackerTime + 3000 < botWorld->gameLocalInfo.time ) && !enemyInfo.enemyFacingBot && !Client_HasMultipleAttackers( botNum ) ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
				return;
			} else {

				if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Hal_Strafe_Attack_Movement;
				} else {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Side_Strafe_Attack_Movement;
				} 

				return;
			}
		} else {

			if ( enemyInfo.enemyDist < 700.0f ) {
				k = 4;
			} else {
				k = 3;
			}

			result = botThreadData.random.RandomInt( k );

			if ( result == 0 ) {
		        COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Run_And_Gun_Movement;
			} else if ( result == 1 ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Hal_Strafe_Attack_Movement;
			} else if ( result == 2 ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Side_Strafe_Attack_Movement;
			} else {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Circle_Strafe_Attack_Movement; //mal: this will only be run if dist < 700
			}

			return;
		}
	}

	if ( botInfo->weapInfo.weapon == SHOTGUN ) {
		if ( enemyInfo.enemyDist < 700.0f ) {
			if ( ( botInfo->lastAttacker != enemy || botInfo->lastAttackerTime + 3000 < botWorld->gameLocalInfo.time ) && !enemyInfo.enemyFacingBot && !Client_HasMultipleAttackers( botNum ) ) {
				if ( Bot_CanCrouch( enemy ) ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;
					return;
				} else {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
					return;
				}
			} else {

                result = botThreadData.random.RandomInt( 3 );

				if ( result == 0 ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Hal_Strafe_Attack_Movement;
				} else if ( result == 1 ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Circle_Strafe_Attack_Movement;
				} else {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Side_Strafe_Attack_Movement;
				}

				return;
			}
		} else {

			result = botThreadData.random.RandomInt( 3 );

			if ( result == 0 ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Hal_Strafe_Attack_Movement;
			} else if ( result == 1 ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Run_And_Gun_Movement;
			} else {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Side_Strafe_Attack_Movement;
			}

            return;
		}
	}

//mal: if we reach here, bot has a pistol or a smg of some kind

    if ( enemyInfo.enemyDist > 2500.0f && !Client_HasMultipleAttackers( botNum ) && !enemyInfo.enemyFacingBot ) {
        if ( ( botInfo->lastAttacker != enemy || botInfo->lastAttackerTime + 5000 < botWorld->gameLocalInfo.time ) ) { //mal: if we got the drop on someone, just crouch down to shoot.
            if ( Bot_CanCrouch( enemy ) ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;
				return;
			} else {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
				return;
			}
		} else {
            result = botThreadData.random.RandomInt( 3 );

			if ( result == 0 ) {
				if ( Bot_CanCrouch( enemy ) ) {
					COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;
					return;
				} else {
		            if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			            COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
						return;
					} else {
					    COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Run_And_Gun_Movement;
						return;
					}
				}
			} else if ( result == 1) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
				return;
			} else {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Run_And_Gun_Movement;
				return;
			}
		}
	}

	if ( enemyInfo.enemyDist < 1500.0f ) {
		if ( ( botInfo->lastAttacker != enemy || botInfo->lastAttackerTime + 3000 < botWorld->gameLocalInfo.time ) && !enemyInfo.enemyFacingBot ) {
            if ( Bot_CanCrouch( enemy ) ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;
				return;
			} else {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
				return;
			}
		} else {

            result = botThreadData.random.RandomInt( 4 );

			if ( result == 0 ) {
		        COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crazy_Jump_Attack_Movement;	
			} else if ( result == 1 ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Hal_Strafe_Attack_Movement;
			} else if ( result == 2 ) {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Circle_Strafe_Attack_Movement;
			} else {
				COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Side_Strafe_Attack_Movement;
			} 
			return;
		}
	}

	if ( ( botInfo->lastAttacker != enemy || botInfo->lastAttackerTime + 3000 < botWorld->gameLocalInfo.time ) && !enemyInfo.enemyFacingBot && !Client_HasMultipleAttackers( botNum ) ) {
        if ( Bot_CanCrouch( enemy ) ) {
			COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Crouch_Attack_Movement;
			return;
		} else {
			COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Stand_Ground_Attack_Movement;
			return;
		}
	} else {
        result = botThreadData.random.RandomInt( 3 );

		if ( result == 0 ) {
			COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Run_And_Gun_Movement;			
		} else if ( result == 1 ) {
			COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Hal_Strafe_Attack_Movement;
		} else {
			COMBAT_MOVEMENT_STATE = &idBotAI::Enter_Side_Strafe_Attack_Movement;
		}
	}
}

/*
================
idBotAI::Bot_SetupMove

Sets up the bot's path goal 
================
*/
void idBotAI::Bot_SetupMove( const idVec3 &org, int clientNum, int actionNumber, int areaNum ) {
	idVec3 goalOrigin;

	if ( botAAS.aas == NULL ) {
		return;
	}

	botAAS.triedToMoveThisFrame = true;

	BuildObstacleList( false, false );

	int travelFlags, walkTravelFlags;

	if ( !botInfo->isDisguised ) { //mal: if disguised, we can go anywhere we want.
		if ( botInfo->team == GDF ) {
			travelFlags = TravelFlagForTeam();
			walkTravelFlags = TravelFlagWalkForTeam();
		} else {
			travelFlags = TravelFlagForTeam();
			walkTravelFlags = TravelFlagWalkForTeam();
		}
	} else {
		travelFlags = TFL_VALID_GDF_AND_STROGG;
		walkTravelFlags = TFL_VALID_WALK_GDF_AND_STROGG;
	}

	if ( Bot_ReachedCurrentRouteNode() ) {
		Bot_FindNextRouteToGoal();
	}

	idBounds bbox = botAAS.aas->GetSettings()->boundingBox;

	if ( routeNode != NULL && aiState == LTG ) {
		areaNum = botAAS.aas->PointReachableAreaNum( routeNode->origin, bbox, AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
		goalOrigin = routeNode->origin;

		if ( botInfo->areaNum != areaNum ) {
			botAAS.aas->PushPointIntoArea( areaNum, goalOrigin );
		}
	} else {
        if ( clientNum != -1 ) {
			areaNum = botWorld->clientInfo[ clientNum ].areaNum;
			if ( botInfo->areaNum != areaNum ) {
				goalOrigin = botWorld->clientInfo[ clientNum ].aasOrigin;	
			} else {
				goalOrigin = botWorld->clientInfo[ clientNum ].origin;
			}
		} else if ( actionNumber != -1 ) {
			areaNum = botThreadData.botActions[ actionNumber ]->areaNum;
			goalOrigin = botThreadData.botActions[ actionNumber ]->origin;

			if ( botInfo->areaNum != areaNum ) {
				botAAS.aas->PushPointIntoArea( areaNum, goalOrigin );
			}
		} else {
			if ( areaNum == 0 ) {
				areaNum = botAAS.aas->PointReachableAreaNum( org, bbox, AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
			}

			goalOrigin = org;

			if ( botInfo->areaNum != areaNum ) {
				botAAS.aas->PushPointIntoArea( areaNum, goalOrigin );
			}
		}
	}

	idObstacleAvoidance::obstaclePath_t path;

	botAAS.hasPath = botAAS.aas->WalkPathToGoal( botAAS.path, botInfo->areaNum, botInfo->aasOrigin, areaNum, goalOrigin, travelFlags, walkTravelFlags );

	if ( botThreadData.AllowDebugData() ) {
		if ( bot_showPath.GetInteger() == botNum ) {
			botAAS.aas->ShowWalkPath( botInfo->areaNum, botInfo->aasOrigin, areaNum, goalOrigin, travelFlags, walkTravelFlags );
			gameRenderWorld->DebugCircle( colorGreen, org, idVec3( 0, 0, 1 ), 32, 32 );
		}
	}

	botAAS.hasClearPath = obstacles.FindPathAroundObstacles( botInfo->localBounds, botAAS.aas->GetSettings()->obstaclePVSRadius, botAAS.aas, botInfo->aasOrigin, botAAS.path.moveGoal, path );

	if ( !botAAS.hasClearPath && routeNode != NULL && aiState == LTG ) { //mal: our route is inside of an obstacle we can't get around, so pick the next route in the path chain.
		Bot_FindNextRouteToGoal();
	}

	botAAS.path.moveGoal = path.seekPos; 

	if ( path.firstObstacle != -1 && botWorld->gameLocalInfo.botsCanDecayObstacles ) {
		proxyInfo_t vehicle;
		GetVehicleInfo( path.firstObstacle, vehicle );

		if ( vehicle.entNum != 0 && vehicle.type != MCP && vehicle.isEmpty && vehicle.xyspeed <= WALKING_SPEED && !vehicle.neverDriven ) {
			int humansInArea = ClientsInArea( botNum, botInfo->origin, 300.0f, vehicle.team, NOCLASS, false, false , false, true, true, true );

			if ( humansInArea == 0 && OtherBotsWantVehicle( vehicle ) == false ) {
				if ( vehicle.spawnID != vehicleObstacleSpawnID ) {
					vehicleObstacleSpawnID = vehicle.spawnID;
					vehicleObstacleTime = botWorld->gameLocalInfo.time;
				} else if ( vehicleObstacleTime + 5000 < botWorld->gameLocalInfo.time ) {
					botUcmd->decayObstacleSpawnID = vehicle.spawnID;
					vehicleObstacleSpawnID = -1;
					vehicleObstacleTime = -1;
				}
			}
		}
	}

	if ( path.firstObstacle != -1 ) {
		botAAS.path.type = PATHTYPE_WALK;
	}

/*
	if ( botAAS.hasClearPath && path.firstObstacle != -1 ) {
        botAAS.path.moveGoal = path.seekPos;
		botAAS.path.type = PATHTYPE_WALK;
	}
*/

	botAAS.obstacleNum = path.firstObstacle;

	if ( !botWorld->gameLocalInfo.inWarmup && botThreadData.random.RandomInt( 100 ) > 98 ) {
        if ( path.firstObstacle > -1 && path.firstObstacle < MAX_CLIENTS ) {
			const clientInfo_t& blockingClient = botWorld->clientInfo[ path.firstObstacle ];
			if ( blockingClient.team == botInfo->team && !blockingClient.isBot ) {

//mal: ONLY say move if both the bot and the client in question are not just spawning in!
				if ( blockingClient.invulnerableEndTime < botWorld->gameLocalInfo.time && botInfo->invulnerableEndTime < botWorld->gameLocalInfo.time ) {
                    idVec3 tempOrigin = blockingClient.origin - botInfo->origin;

					if ( tempOrigin.LengthSqr() < Square( 50.0f ) ) {
                        botUcmd->desiredChat = MOVE;
						botUcmd->botCmds.shoveClient = true;
					}
				}
		   }
	   }
   }

	//mal: play with the origin just a bit, so that its more at eye level. Gets us realistic view goals on the cheap.
	botAAS.path.viewGoal = botAAS.path.moveGoal;
	idVec3 viewDelta = botAAS.path.moveGoal - botInfo->viewOrigin;
	if ( idMath::Fabs( viewDelta.z ) < 100.0f ) {
		botAAS.path.viewGoal.z = botInfo->viewOrigin.z;
	}
}

/*
================
idBotAI::Bot_SetupQuickMove

A fast version of SetupMove, that just has the bot blindly move towards its goal pos, only doing dynamic obstacle avoidance checks.
================
*/
void idBotAI::Bot_SetupQuickMove( const idVec3 &org, bool largePlayerBBox ) {
	idVec3 tempOrigin;

	BuildObstacleList( largePlayerBBox, false );

	if ( Bot_ReachedCurrentRouteNode() ) {
		Bot_FindNextRouteToGoal();
	}

	idObstacleAvoidance::obstaclePath_t path;

	botAAS.hasClearPath = obstacles.FindPathAroundObstacles( botInfo->localBounds, botAAS.aas->GetSettings()->obstaclePVSRadius, botAAS.aas, botInfo->origin, org, path, true );

    botAAS.path.moveGoal = path.seekPos;

   if ( path.firstObstacle > -1 && path.firstObstacle < MAX_CLIENTS && botThreadData.random.RandomInt( 100 ) > 98 ) {
	   if ( botWorld->clientInfo[ path.firstObstacle ].team == botInfo->team && !botWorld->clientInfo[ path.firstObstacle ].isBot ) {
           tempOrigin = botWorld->clientInfo[ path.firstObstacle ].origin - botInfo->origin;

           if ( tempOrigin.LengthSqr() < Square( 50.0f ) ) {
               botUcmd->desiredChat = MOVE;
			   botUcmd->botCmds.shoveClient = true;
		   }
	   }
   }

   botAAS.obstacleNum = path.firstObstacle;
   botAAS.hasPath = path.hasValidPath; //mal: safety check - let the bot know if its blind wanderings just isn't working out.

//mal: play with the origin just a bit, so that its more at eye level. Gets us realistic view goals on the cheap.
	botAAS.path.viewGoal = botAAS.path.moveGoal;
	idVec3 viewDelta = botAAS.path.moveGoal - botInfo->viewOrigin;
	if ( idMath::Fabs( viewDelta.z ) < 100.0f ) {
		botAAS.path.viewGoal.z = botInfo->viewOrigin.z;
	}
}

/*
================
idBotAI::Bot_CanMove
================
*/
bool idBotAI::Bot_CanMove( const moveDirections_t direction, float gUnits, bool copyEndPos, bool endPosMustBeOutside ) {
	idVec3 end;

	end = botInfo->origin;
	
	switch ( direction ) {
	
	case FORWARD:
		end += ( gUnits * botInfo->viewAxis[ 0 ] );
		break;
	
	case BACK:
		end += ( -gUnits * botInfo->viewAxis[ 0 ] );
		break;

	case LEFT:
		end += ( -gUnits * ( botInfo->viewAxis[ 1 ] * -1 ) );
		break;

	case RIGHT:
		end += ( gUnits * ( botInfo->viewAxis[ 1 ] * -1 ) );
		break;
	}

	if ( botThreadData.AllowDebugData() ) {
		gameRenderWorld->DebugLine(colorGreen, botInfo->origin, end, 16 );
	}

	if ( botThreadData.Nav_IsDirectPath( AAS_PLAYER, botInfo->team, botInfo->areaNum, botInfo->aasOrigin, end ) ) {
		if ( endPosMustBeOutside ) {
			if ( !LocationVis2Sky( end ) ) {
				return false;
			}
		}
		botCanMoveGoal = end;
        return true;
	}
	
	return false;
}

/*
================
idBotAI::Bot_MoveToGoal

Sets where the bot would like to move. 
================
*/
void idBotAI::Bot_MoveToGoal( const idVec3 &spot1, const idVec3 &spot2, const botMoveFlags_t moveFlag, const botMoveTypes_t moveType ) {
	if ( botUcmd->specialMoveType != SKIP_MOVE ) {
		botUcmd->moveFlag = moveFlag;
 		botUcmd->moveType = moveType;
		botUcmd->moveGoal = spot1;
		botUcmd->moveGoal2 = spot2;

		float movementSpeed = SPRINTING_SPEED;

		if ( spot1.IsZero() ) {
			movementSpeed = 0.0f;
		} else if ( moveFlag == WALK || moveFlag == CROUCH || moveFlag == PRONE ) {
			movementSpeed = WALKING_SPEED;
		} else if ( moveFlag == RUN ) {
			movementSpeed = RUNNING_SPEED;
		}

		if ( botAAS.obstacleNum != -1 && botInfo->xySpeed < movementSpeed ) {
			botAAS.blockedByObstacleCounterOnFoot++;
		} else {
			botAAS.blockedByObstacleCounterOnFoot = 0;
		}

		if ( botThreadData.AllowDebugData() ) {
			if ( bot_debug.GetBool() ) { 
				idVec3 test = spot1;
				test[ 2 ] += 24;
				gameRenderWorld->DebugLine(colorYellow, spot1, test, 16 );
			}
		}
	}
}

/*
================
idBotAI::Bot_MoveAlongPath
moves the bot along the current botAAS.path, performing any special moves as required
================
*/
void idBotAI::Bot_MoveAlongPath( botMoveFlags_t defaultMoveFlags ) {
	if ( botAAS.hasPath == false ) {
		Bot_MoveToGoal( vec3_zero, vec3_zero, defaultMoveFlags, NULLMOVETYPE );
		Bot_LookAtNothing( SMOOTH_TURN );
		return;
	}

	switch ( botAAS.path.type ) {

		case PATHTYPE_JUMP: {
			Bot_MoveToGoal( botAAS.path.moveGoal, botAAS.path.reachability->GetEnd(), SPRINT, LOCATION_JUMP );
			Bot_LookAtNothing( SMOOTH_TURN );
			break;
		}

		case PATHTYPE_BARRIERJUMP: {
			// look at some arbitrary point 30 units past the end of the reachability, this prevents them from spinning once they jump up on the barrier
			idVec2 reachVec = ( botAAS.path.reachability->GetEnd() - botAAS.path.reachability->GetStart() ).ToVec2();
			reachVec.Normalize();
			reachVec *= 30.0f;
			idVec3 lookAtPoint = botAAS.path.reachability->GetEnd();
			lookAtPoint.x += reachVec.x;
			lookAtPoint.y += reachVec.y;
			Bot_MoveToGoal( botAAS.path.moveGoal, botAAS.path.reachability->GetEnd(), RUN, LOCATION_BARRIERJUMP );
			Bot_LookAtLocation( lookAtPoint, SMOOTH_TURN );
			break;
		}

		case PATHTYPE_LADDER: {
			idVec3 end = botAAS.path.reachability->GetEnd();
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
			Bot_LookAtLocation( end, SMOOTH_TURN );
			break;
		}

		case PATHTYPE_WALKOFFBARRIER: {
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, defaultMoveFlags, LOCATION_WALKOFFLEDGE );
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
			break;
		}

		case PATHTYPE_WALKOFFLEDGE: {
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, defaultMoveFlags, LOCATION_WALKOFFLEDGE );
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
			break;
		}

		default: { //mal: check to see if we're under attack from an enemy we can't see - if so, move around a bit to make ourselves harder to hit.

			bool safeToJump = false; 
			botMoveTypes_t defaultMoveType = NULLMOVETYPE;

			idVec3 vec = botAAS.path.moveGoal - botInfo->origin;

			float moveDist = 300.0f;

			if ( vec.LengthSqr() > Square( moveDist ) ) {
				safeToJump = true;
			}

			if ( strafeToAvoidDangerTime < botWorld->gameLocalInfo.time && LocationVis2Sky( botInfo->origin ) && safeToJump && ( Bot_IsUnderAttackFromUnknownEnemy() || Bot_HasEnemySniperInArea( MAX_SNIPER_VIEW_DIST ) ) ) {
				if ( ClientIsValid( botInfo->lastAttacker, -1 ) ) {
					if ( strafeToAvoidDangerTime < botWorld->gameLocalInfo.time ) {
						strafeToAvoidDangerTime = botWorld->gameLocalInfo.time + 500;
						bool strafeRight = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? true : false;

						if ( strafeRight ) {
							if ( Bot_CanMove( RIGHT, moveDist, true ) ) {
								strafeToAvoidDangerDir = RIGHT;
							} else if ( Bot_CanMove( LEFT, moveDist, true ) ) {
								strafeToAvoidDangerDir = LEFT;
							}
						} else {
							if ( Bot_CanMove( LEFT, moveDist, true ) ) {
								strafeToAvoidDangerDir = LEFT;
							} else if ( Bot_CanMove( RIGHT, moveDist, true ) ) {
								strafeToAvoidDangerDir = RIGHT;
							}
						}
					}
				}
			}

			if ( strafeToAvoidDangerTime > botWorld->gameLocalInfo.time ) {
				if ( strafeToAvoidDangerDir == RIGHT ) {
					defaultMoveType = RANDOM_JUMP_RIGHT;
				} else if ( strafeToAvoidDangerDir == LEFT ) {
					defaultMoveType = RANDOM_JUMP_LEFT;
				}
			}

			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, defaultMoveFlags, defaultMoveType );
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
			break;
		}
	}

	if ( botAAS.path.type != PATHTYPE_LADDER && botInfo->onLadder && botInfo->xySpeed < 25.0f ) {
		botUcmd->viewType = VIEW_RANDOM;
		botUcmd->turnType = INSTANT_TURN;
	}
}

/*
================
idBotAI::Bot_CanCrouch
================
*/
bool idBotAI::Bot_CanCrouch( int clientNum ) {
	trace_t	tr;
	idVec3 	selfView = botInfo->origin;
	idVec3  otherView = botWorld->clientInfo[ clientNum ].viewOrigin;
	selfView[ 2 ] += botWorld->gameLocalInfo.crouchViewHeight;

	botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, selfView, otherView, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ) );

	if ( tr.fraction < 1.0f ) {
		return false;
	}

	return true;
}

/*
================
idBotAI::Bot_CanProne
================
*/
bool idBotAI::Bot_CanProne( int clientNum ) {
#ifdef _XENON
	return false;
#endif

	trace_t	tr;
	idVec3 	selfView = botInfo->origin;
	idVec3  otherView = botWorld->clientInfo[ clientNum ].viewOrigin;

	selfView[ 2 ] += botWorld->gameLocalInfo.proneViewHeight;

	botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, selfView, otherView, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ) );

	if ( tr.fraction < 1.0f ) {
		return false;
	}

	return true;
}

/*
================
idBotAI::Bot_ShouldStrafeJump

Should this bot strafe jump, or just sprint? Will skip checking for a while, if the test ever fails.
ONLY bots doing an long term goal will think about strafe jumping(?).
================
*/
const botMoveFlags_t idBotAI::Bot_ShouldStrafeJump( const idVec3 &targetOrigin ) {
	float strafeDist = Square( ( aiState == LTG && ( ltgType == FOLLOW_TEAMMATE || ltgType == FOLLOW_TEAMMATE_BY_REQUEST ) ) ? 700.0f : 1900.0f );
	idVec3 end;

	int delayTime = 0;//1000; //mal: just a second delay by default

	if ( skipStrafeJumpTime >= botWorld->gameLocalInfo.time ) {
		isStrafeJumping = false;

		if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY || botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
			if ( botWorld->gameLocalInfo.gameIsBotMatch && TeamHasHuman( botInfo->team ) ) { //mal: always sprint in SP mode
				return SPRINT;
			} else {
				return RUN;
			}
		} else {
			return SPRINT;
		}
	}

	end = targetOrigin - botInfo->origin; //mal: dont strafejump close to the target: it gives us away, and we need to have gun out, ready to fight!

	if ( end.LengthSqr() < strafeDist ) {
		isStrafeJumping = false;
		skipStrafeJumpTime = botWorld->gameLocalInfo.time + delayTime;
		
		if ( botThreadData.GetBotSkill() > BOT_SKILL_EASY && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
            return SPRINT;
		} else {
			return RUN;
		}
	}

	if ( !botWorld->gameLocalInfo.botsCanStrafeJump ) { //mal: bot cant strafe jump if server admin won't let them
		skipStrafeJumpTime = botWorld->gameLocalInfo.time + delayTime; //mal: only for a second, in case admin changes mind.....
		isStrafeJumping = false;

		if ( botInfo->enemiesInArea == 0 && enemy == -1 && botThreadData.GetBotSkill() > BOT_SKILL_EASY ) {
			pistolTime = botWorld->gameLocalInfo.time + 1000;
		}

		if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY || botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
			if ( botWorld->gameLocalInfo.gameIsBotMatch && TeamHasHuman( botInfo->team ) ) { //mal: always sprint in SP mode
				return SPRINT;
			}
			return RUN;
		} else {
            return SPRINT;
		}
	}

	if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY || botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) { //mal: low skill bots wont bother to strafe jump, or sprint, just run!
		skipStrafeJumpTime = botWorld->gameLocalInfo.time + delayTime; //mal: in case admin changes mind.
		isStrafeJumping = false;
		pistolTime = 0;
		if ( botWorld->gameLocalInfo.gameIsBotMatch && TeamHasHuman( botInfo->team ) ) { //mal: always sprint in SP mode
			return SPRINT;
		}
		return RUN;
	}

	if ( botThreadData.GetBotSkill() < BOT_SKILL_EXPERT ) { //mal: lower skill bots wont bother to strafe jump, just sprint ( but will use knife for extra speed )
		skipStrafeJumpTime = botWorld->gameLocalInfo.time + delayTime; //mal: in case admin changes mind.
		isStrafeJumping = false;

		if ( botInfo->enemiesInArea == 0 && enemy == -1 ) {
			pistolTime = botWorld->gameLocalInfo.time + 1000;
		}

		return SPRINT;
	}

	if ( botInfo->xySpeed < SPRINTING_SPEED ) {
		skipStrafeJumpTime = botWorld->gameLocalInfo.time + delayTime; 
		isStrafeJumping = false;
		return SPRINT;
	}

	if ( !LocationVis2Sky( botInfo->origin ) ) { //mal: if indoors, make sure we have enough ceiling room
		if ( !LocationHasHeadRoom( botInfo->origin ) ) {
            skipStrafeJumpTime = botWorld->gameLocalInfo.time + delayTime; 
			isStrafeJumping = false;

			if ( botInfo->enemiesInArea == 0 && enemy == -1 ) {
				pistolTime = botWorld->gameLocalInfo.time + 1000;
			}

			return SPRINT;
		}
	}

	end = botInfo->viewOrigin;

	end += ( 300.0f * botInfo->viewAxis[ 0 ] );

	if ( botThreadData.Nav_IsDirectPath( AAS_PLAYER, botInfo->team, botInfo->areaNum, botInfo->aasOrigin, end ) ) {
		isStrafeJumping = true;
		pistolTime = botWorld->gameLocalInfo.time + 5000;
		return STRAFEJUMP;
	}

//mal: we're not going to strafe jump...
	skipStrafeJumpTime = botWorld->gameLocalInfo.time + delayTime; 
	isStrafeJumping = false;

	if ( botInfo->enemiesInArea == 0 && enemy == -1 ) {
        pistolTime = botWorld->gameLocalInfo.time + 1000;
	}

	return SPRINT;
}

/*
================
idBotAI::MoveIsInvalid
================
*/
bool idBotAI::MoveIsInvalid() {
	if ( botAAS.hasPath == false ) {
		if ( botVehicleInfo != NULL && botVehicleInfo->type > ICARUS ) {
			badMoveTime++;
		} else if ( botInfo->hasGroundContact || botInfo->inWater ) { //mal: the "hasGroundContact" check was recently added. 8/20/07
			badMoveTime++;
		}
	} else {
		badMoveTime = 0;
	}

	if ( badMoveTime > 30 ) { //mal: WAS 150
		badMoveTime = 0; //mal: reset this for next time

		moveErrorCounter.moveErrorCount++;

		if ( botThreadData.AllowDebugData() ) {
			if ( bot_debug.GetBool() ) {
				common->Warning("Move is invalid for bot client %i!", botNum );
			}
		}

		return true;
	} else {
		return false;
	}
}

/*
================
idBotAI::Bot_CheckBlockingOtherClients

Gives us a quick and cheap way to find out if the bot is somehow blocking a teammate, and moves them away from us.
This won't be called in AI nodes that have the bot busy doing something.
================
*/
int idBotAI::Bot_CheckBlockingOtherClients( int ignoreClient, float avoidDist ) {
	int blockedClient = -1;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( ClientIsIgnored( i ) ) {
			continue;
		}

		if ( i == botNum ) {
			continue;
		}

		if ( i == ignoreClient ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		if ( playerInfo.team != botInfo->team ) {
			continue;
		}

		if ( playerInfo.weapInfo.isFiringWeap ) { //mal: if we're in someones way, try to move out of the way.

			idVec3 vec = playerInfo.origin - botInfo->origin;
			float avoidTraceDist = 500.0f;

			if ( vec.LengthSqr() > Square( avoidTraceDist ) ) { //the bot is too far away to worry about being in your way.
				continue;
			}

            trace_t	tr;
			idVec3 end = playerInfo.viewOrigin;
			end += ( avoidTraceDist * playerInfo.viewAxis[ 0 ] );
			botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, playerInfo.viewOrigin, end, MASK_SHOT_BOUNDINGBOX, GetGameEntity( i ) );

			if ( tr.c.entityNum == botNum ) {
				return i;
			}
		}

		idVec3 vec = playerInfo.origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( avoidDist ) ) {
			continue;
		}

		blockedClient = i;
		break;
	}

	return blockedClient;
}

/*
================
idBotAI::Bot_MoveAwayFromClient

Moves us away from the client we're blocking.
================
*/
void idBotAI::Bot_MoveAwayFromClient( int clientNum, bool randomStrafe ) {

	botMoveFlags_t botMoveFlag;

	if ( botInfo->posture == IS_CROUCHED || botInfo->posture == IS_PRONE ) {
		botMoveFlag = CROUCH;
	} else { 
		botMoveFlag = WALK;
	}

	botMoveTypes_t botMoveType = NULLMOVETYPE;

	if ( randomStrafe ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			botMoveType = STRAFE_RIGHT;
		} else {
			botMoveType = STRAFE_LEFT;
		}
	}

	Bot_SetupQuickMove( botInfo->origin, false );
	Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, botMoveFlag, botMoveType );
	return;
}


/*
================
idBotAI::Bot_FindStanceForLocation

Finds the correct stance for the bot given a particular location ( used in reviving, planting mines/charges, spawnhosting, etc ).
ex: this way, a medic bot isn't trying to crouch over someone above them, etc.
================
*/
const botMoveFlags_t idBotAI::Bot_FindStanceForLocation( const idVec3 &org, bool allowProne ) {
	idVec3 vec = org - botInfo->origin;
	float locHeight = vec.z;

    if ( locHeight > 60.0f ) { //40
		return WALK;
	} else if ( locHeight > 20.0f ) { //20
		return CROUCH;
	} else if ( allowProne ) {
		return PRONE;
	}

	return CROUCH;
}

#define DYNAMIC_OBSTACLE_CULL_DISTANCE		4096
#define DYNAMIC_OBSTACLE_CONSIDER_DISTANCE	256
#define OBSTACLE_CULL_DISTANCE	2048.0f
//#define USE_OBSTACLE_PVS

/*
================
idBotAI::AddGroundObstacles
================
*/
bool idBotAI::AddGroundObstacles( bool largePlayerBBox, bool inVehicle ) {
	bool botShouldMoveCautiously = false;
	float avoidObstacleRange;

#ifdef USE_OBSTACLE_PVS
	const byte *pvs = botAAS.aas->GetObstaclePVS( inVehicle ? botInfo->areaNumVehicle : botInfo->areaNum );
#endif

	float bboxHeight = botAAS.aas->GetSettings()->boundingBox[ 1 ].z - botAAS.aas->GetSettings()->boundingBox[ 0 ].z;
	idVec3 playerOrigin = ( botVehicleInfo != NULL ) ? botVehicleInfo->origin : botInfo->origin;

	//mal: deployables first
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

		//mal: dont avoid the deployable we're trying to interact with!
		if ( botUcmd->actionEntityNum != -1 && deployable.entNum == botUcmd->actionEntityNum ) {
			continue;
		}

		idVec3 vec = deployable.origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( OBSTACLE_CULL_DISTANCE ) ) {
			continue;
		}

		idBox box( deployable.bbox, deployable.origin, deployable.axis );

		if ( Bot_IsClearOfObstacle( box, bboxHeight, playerOrigin ) ) {
			continue;
		}

		obstacles.AddObstacle( box, deployable.entNum );

		if ( inVehicle ) {
			if ( InFrontOfVehicle( botInfo->proxyInfo.entNum, deployable.origin ) ) {
				botShouldMoveCautiously = true;
			}
		}

		if ( botThreadData.AllowDebugData() ) {
			if ( bot_debugObstacles.GetBool() ) {
				gameRenderWorld->DebugBox( colorGreen, box, 5000 );
			}
		}
	}

	if ( botVehicleInfo != NULL && botVehicleInfo->type == MCP ) {
		return botShouldMoveCautiously;
	}

	//mal: players next
	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		//mal: dont avoid ourselves! :-P
		if ( i == botNum ) {
			continue;
		}

		//mal: dont avoid the client we're trying to interact with!
		if ( botUcmd->actionEntityNum != -1 && i == botUcmd->actionEntityNum ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		//mal: we'll test vehicles seperately.
		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			continue;
		}

		//mal: ignore players outside the valid AAS space
		if ( playerInfo.areaNum == 0 ) {
			continue;
		}

		//mal: dont avoid our enemy - we may want to get in his face.
		if ( i == enemy ) {
			continue;
		}

		if ( playerInfo.team != botInfo->team ) {
			//mal: if we're in a vehicle, run them over! >:-D
			if ( botVehicleInfo != NULL ) {
				continue;
			} else {
				//mal: if on foot - only avoid enemy players if we can see them and we're not disguised.
				if ( !botInfo->isDisguised && !InFrontOfClient( botNum, playerInfo.origin ) ) {
					continue;											
				}
			}
		} else {
			if ( botVehicleInfo == NULL ) {
				if ( !InFrontOfClient( botNum, playerInfo.origin, true ) ) {
					continue;											
				}
			}
		}

		//mal: if in a vehicle, avoid our fellow players better!
		avoidObstacleRange = 1024.0f;
		bool inFrontOfOurVehicle = ( inVehicle == false ) ? false : InFrontOfVehicle( botVehicleInfo->entNum, playerInfo.origin );
		float ourForwardSpeed = ( inVehicle == true ) ? botVehicleInfo->forwardSpeed : botInfo->xySpeed;

		idVec3 vec = playerInfo.origin - botInfo->origin;
		float distToPlayerSqr = vec.LengthSqr();

		//mal: keep the range pretty tight.
		if ( distToPlayerSqr > Square( avoidObstacleRange ) ) {
			if ( inFrontOfOurVehicle && ourForwardSpeed > BASE_VEHICLE_SPEED && distToPlayerSqr < Square( 2000.0f ) ) {
				botShouldMoveCautiously = true;
				if ( !playerInfo.isBot ) {
					botUcmd->desiredChat = MOVE;		//mal: let the player know to move it or lose it!
					botUcmd->botCmds.honkHorn = true;
				}
			}
			continue;
		}

		idBox box;
		if ( largePlayerBBox ) {
			box = idBox( playerInfo.origin, idVec3( 64.0f, 64.0f, 84.0f ), playerInfo.bodyAxis );
		} else {
			box = idBox( playerInfo.localBounds, playerInfo.origin, playerInfo.bodyAxis );
		}

		if ( Bot_IsClearOfObstacle( box, bboxHeight, playerOrigin ) ) {
			continue;
		}

		if ( inVehicle ) {
			if ( inFrontOfOurVehicle ) {
				botShouldMoveCautiously = true;

				if ( botVehicleInfo->flags & PERSONAL ) { //mal: personal vehicles always keep moving, and just avoid.
					obstacles.AddObstacle( box, i );
				} else if ( playerInfo.xySpeed < WALKING_SPEED ) { //mal: if the player is stopped, or moving REALLY slow, avoid him
					obstacles.AddObstacle( box, i );
				} else if ( InFrontOfClient( i, botVehicleInfo->origin ) && distToPlayerSqr < Square( 512.0f ) && InFrontOfVehicle( botVehicleInfo->entNum, playerInfo.origin, true ) ) {
					vehiclePauseTime = botWorld->gameLocalInfo.time + 100; //mal: else, just stop and wait for him to pass.
				}
			}
		} else {
			obstacles.AddObstacle( box, i );
		}

		if ( botThreadData.AllowDebugData() ) {
			if ( bot_debugObstacles.GetBool() ) {
				gameRenderWorld->DebugBox( colorGreen, box, 128 );
			}
		}
	}

	//mal: player's supply crates next
	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.supplyCrate.entNum == 0 ) { 
			continue;
		}

		//mal: this crate hasn't settled yet, and is prolly still dropping from the air.
		if ( playerInfo.supplyCrate.areaNum == 0 ) {
			continue;
		}

		//mal: dont avoid the crate we're trying to interact with!
		if ( botUcmd->actionEntityNum != -1 && playerInfo.supplyCrate.entNum == botUcmd->actionEntityNum ) {
			continue;
		}

		idVec3 vec = playerInfo.supplyCrate.origin - botInfo->origin;

		//mal: if in a vehicle, avoid better!
		if ( botVehicleInfo != NULL ) {
			avoidObstacleRange = 1500.0f;
		} else {
			avoidObstacleRange = 1024.0f;
		}

		//mal: keep the range pretty tight.
		if ( vec.LengthSqr() > Square( avoidObstacleRange ) ) {
			continue;
		}

		idBox box;

		if ( botVehicleInfo != NULL ) {
			box = idBox( playerInfo.supplyCrate.origin, idVec3( 64.0f, 64.0f, 64.0f ), mat3_identity );
		} else {
			box = idBox( playerInfo.supplyCrate.origin, idVec3( 32.0f, 32.0f, 64.0f ), mat3_identity );
		}

		if ( Bot_IsClearOfObstacle( box, bboxHeight, playerOrigin ) ) {
			continue;
		}

		obstacles.AddObstacle( box, playerInfo.supplyCrate.entNum );

		if ( inVehicle ) {
			if ( InFrontOfVehicle( botVehicleInfo->entNum, playerInfo.supplyCrate.origin ) ) {
				botShouldMoveCautiously = true;
			}
		}

		if ( botThreadData.AllowDebugData() ) {
			if ( bot_debugObstacles.GetBool() ) {
				gameRenderWorld->DebugBox( colorGreen, box, 128 );
			}
		}
	}

/*
	//mal: arty strikes next
	if ( currentArtyDanger.time > botWorld->gameLocalInfo.time ) {

		idVec3 vec = currentArtyDanger.origin - botInfo->origin;

		if ( vec.LengthSqr() < Square( 2048.0f ) ) {
			idVec3 vec = currentArtyDanger.origin;
			vec[ 2 ] += 128.0f;
			idBox box( vec, idVec3( 1024.0f, 1024.0f, 128.0f ), mat3_identity );

			if ( !Bot_IsClearOfObstacle( box, bboxHeight, playerOrigin ) ) {
				obstacles.AddObstacle( box, MAX_GENTITIES + currentArtyDanger.num ); //mal: arty strikes dont have an ent num, so we just pass a bogus one for the ob avoid system

				if ( botThreadData.AllowDebugData() ) {
					if ( bot_debugObstacles.GetBool() ) {
						gameRenderWorld->DebugBox( colorGreen, box, 128 );
					}
				}
			}
		}
	}

	//mal: then general dangers we are aware of ( landmines, grenades, airstrikes, charges, etc ).
	for( int i = 0; i < MAX_DANGERS; i++ ) {

		if ( currentDangers[ i ].num == 0 ) {
			continue;
		}
	
		if ( currentDangers[ i ].time < botWorld->gameLocalInfo.time ) {
			continue;
		}

		if ( !DangerStillExists( currentDangers[ i ].num, currentDangers[ i ].ownerNum ) ) {
			ClearDangerFromDangerList( i, false );
			continue;
		}

		idVec3 vec = currentDangers[ i ].origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( DYNAMIC_OBSTACLE_CULL_DISTANCE ) ) {
			continue;
		}

		vec = currentDangers[ i ].origin;

		idBox box;
		if ( currentDangers[ i ].type == HAND_GRENADE ) {
			vec[ 2 ] += 128.0f;
			box = idBox( vec, idVec3( 320.0f, 320.0f, 128.0f ), mat3_identity );
		} else if ( currentDangers[ i ].type == PLANTED_LANDMINE ) {
			box = idBox( vec, idVec3( 320.0f, 320.0f, 128.0f ), mat3_identity );
		} else if ( currentDangers[ i ].type == PLANTED_CHARGE ) {
			box = idBox( vec, idVec3( 512.0f, 512.0f, 128.0f ), mat3_identity );
		} else if ( currentDangers[ i ].type == THROWN_AIRSTRIKE ) {
			box = idBox( vec, idVec3( 1024.0f, 768.0f, 64.0f ), currentDangers[ i ].dir );
			
			if ( botThreadData.AllowDebugData() ) {
				gameRenderWorld->DebugBox( colorBlue, box );
			}
		} else if ( currentDangers[ i ].type == STROY_BOMB_DANGER ) {
			box = idBox( vec, idVec3( 320.0f, 320.0f, 128.0f ), mat3_identity );
		} else {
			//mal: its not a danger, or one that we currently support.
			continue;
		}

		if ( !box.Expand( DYNAMIC_OBSTACLE_CONSIDER_DISTANCE ).ContainsPoint( botInfo->origin ) ) {
			continue;
		}

		obstacles.AddObstacle( box, currentDangers[ i ].num );

		if ( botThreadData.AllowDebugData() ) {
			if ( bot_debugObstacles.GetBool() ) {
				gameRenderWorld->DebugBox( colorGreen, box, 128 );
			}
		}
	}
*/

	return botShouldMoveCautiously;
}

/*
================
idBotAI::AddGroundAndAirObstacles
================
*/
bool idBotAI::AddGroundAndAirObstacles( bool largePlayerBBox, bool inVehicle, bool inAirVehicle ) {
	bool botShouldMoveCautiously = false;

#ifdef USE_OBSTACLE_PVS
	const byte *pvs = botAAS.aas->GetObstaclePVS( inVehicle ? botInfo->areaNumVehicle : botInfo->areaNum );
	int aasType = inVehicle ? AAS_VEHICLE : AAS_PLAYER;
#endif

	float bboxHeight = botAAS.aas->GetSettings()->boundingBox[ 1 ].z - botAAS.aas->GetSettings()->boundingBox[ 0 ].z;
	idVec3 playerOrigin = ( botVehicleInfo != NULL ) ? botVehicleInfo->origin : botInfo->origin;

	//mal: manually placed obstacles
	for( int i = 0; i < botThreadData.botObstacles.Num(); i++ ) {
		idBotObstacle *obstacle = botThreadData.botObstacles[ i ];

#ifdef USE_OBSTACLE_PVS
		if ( !inAirVehicle && !IsInObstaclePVS( pvs, obstacle->areaNum[aasType] ) ) {
			continue;
		}
#endif

		idVec3 vec = obstacle->bbox.GetCenter() - botInfo->origin;
		float radiusSqr = obstacle->bbox.GetExtents().LengthSqr();
		float avoidObstacleRangeSqr;

		//mal: if in a vehicle, avoid better!
		if ( botVehicleInfo != NULL ) {
			avoidObstacleRangeSqr = radiusSqr + Square( 1024.0f );
		} else {
			avoidObstacleRangeSqr = radiusSqr + Square( 512.0f );
		}

		if ( vec.LengthSqr() > avoidObstacleRangeSqr ) {
			continue;
		}

		if ( Bot_IsClearOfObstacle( obstacle->bbox, bboxHeight, playerOrigin ) ) {
			continue;
		}

		obstacles.AddObstacle( obstacle->bbox, obstacle->num );
	}

	if ( botVehicleInfo != NULL && botVehicleInfo->type == MCP ) {
		return false;
	}

	//mal: vehicles
	for( int i = 0; i < MAX_VEHICLES; i++ ) {
		
		const proxyInfo_t& vehicleInfo = botWorld->vehicleInfo[ i ];
	
		if ( vehicleInfo.entNum == 0 ) {
			continue;
		}

		//mal: dont avoid the vehicle we're trying to interact with!
		if ( botUcmd->actionEntityNum != -1 && vehicleInfo.entNum == botUcmd->actionEntityNum ) {
			continue;
		}

		//mal: dont count vehicles we're in.
		if ( vehicleInfo.entNum == botInfo->proxyInfo.entNum ) {
			continue;
		}

#ifdef USE_OBSTACLE_PVS
		if ( !inAirVehicle && !IsInObstaclePVS( pvs, inVehicle ? vehicleInfo.areaNumVehicle : vehicleInfo.areaNum ) ) {
			continue;
		}
#endif

		if ( botVehicleInfo != NULL && botVehicleInfo->type != HUSKY && botInfo->team == GDF && vehicleInfo.type == ICARUS ) {
			continue;
		}

		if ( botVehicleInfo != NULL && botVehicleInfo->type <= ICARUS && vehicleInfo.type > ICARUS && !vehicleInfo.hasGroundContact ) { //mal: ground vehicles shouldn't worry about air vehicles in the air.
			continue;
		}

		if ( botVehicleInfo != NULL && botVehicleInfo->type > ICARUS && vehicleInfo.type < ICARUS ) { //mal: air vehicles shouldn't worry about ground vehicles
			continue;
		}

		idVec3 vec = vehicleInfo.origin - botInfo->origin;
		float distSqr = vec.LengthSqr();

		if ( distSqr > Square( DYNAMIC_OBSTACLE_CULL_DISTANCE ) ) {
			continue;
		}

		//mal: enlarge the box based on the vehicles speed.
		float expand = vehicleInfo.forwardSpeed * 2.0f;
		idBox box( vehicleInfo.bbox, vehicleInfo.origin, vehicleInfo.axis );
		box.ExpandSelf( idMath::Fabs( expand * 0.5f ), 0.0f, 0.0f );
		box.TranslateSelf( box.GetAxis()[0] * expand * 0.5f );

		if ( Bot_IsClearOfObstacle( box, bboxHeight, playerOrigin ) ) {
			continue;
		}

		bool inFront = false;

		if ( botVehicleInfo == NULL ) {
			inFront = true;
		} else {
			if ( InFrontOfVehicle( botInfo->proxyInfo.entNum, vehicleInfo.origin ) && distSqr < Square( OBSTACLE_CULL_DISTANCE ) ) {
				botShouldMoveCautiously = true;
				inFront = true;
			}

			if ( inFront && distSqr < Square( 1024.0f ) ) {
				if ( botVehicleInfo->type == GOLIATH ) {
					if ( vehicleInfo.driverEntNum != -1 && distSqr < Square( 512.0f ) ) {
						vehiclePauseTime = botWorld->gameLocalInfo.time + VEHICLE_PAUSE_TIME; //mal: Goliath just stops and waits for everyone, since its so big, and the AAS bbox for it isn't.
					}
				} else if ( botVehicleInfo->flags & ARMOR && !( vehicleInfo.flags & ARMOR ) ) {
					if ( InFrontOfVehicle( vehicleInfo.entNum, botVehicleInfo->origin, true ) && vehicleInfo.driverEntNum != -1 && vehicleInfo.xyspeed > WALKING_SPEED ) {
						vehiclePauseTime = botWorld->gameLocalInfo.time + VEHICLE_PAUSE_TIME; //mal: just stop and wait for him to pass, since hes smaller, and more nimble.
						continue;
					}
				} else if ( botVehicleInfo->flags & ARMOR && vehicleInfo.flags & ARMOR ) {
					if ( InFrontOfVehicle( vehicleInfo.entNum, botVehicleInfo->origin, true ) ) { //mal: ok - this is the pain: what is the other guy doing?
						if ( vehicleInfo.driverEntNum != -1 ) {
							clientInfo_t driver = botWorld->clientInfo[ vehicleInfo.driverEntNum ];

							if ( !driver.isBot ) {
								if ( vehicleInfo.xyspeed > WALKING_SPEED ) {
									vehiclePauseTime = botWorld->gameLocalInfo.time + VEHICLE_PAUSE_TIME; //mal: always defer to the human.
									continue;
								}
							} else {
								if ( vehicleInfo.xyspeed > WALKING_SPEED && botThreadData.bots[ vehicleInfo.driverEntNum ] != NULL && botThreadData.bots[ vehicleInfo.driverEntNum ]->vehiclePauseTime < botWorld->gameLocalInfo.time ) { //mal: hes not pausing, so we will.
									vehiclePauseTime = botWorld->gameLocalInfo.time + VEHICLE_PAUSE_TIME;
									continue;
								}
							}
						}
					}
				} else if ( !( botVehicleInfo->flags & PERSONAL ) && !( vehicleInfo.flags & ARMOR ) ) { 
					if ( InFrontOfVehicle( vehicleInfo.entNum, botVehicleInfo->origin, true ) ) { //mal: ok - this is the pain: what is the other guy doing?
						if ( vehicleInfo.driverEntNum != -1 ) {
							clientInfo_t driver = botWorld->clientInfo[ vehicleInfo.driverEntNum ];

							if ( !driver.isBot ) {
								if ( vehicleInfo.xyspeed > WALKING_SPEED ) {
									vehiclePauseTime = botWorld->gameLocalInfo.time + VEHICLE_PAUSE_TIME; //mal: always defer to the human.
									continue;
								}
							} else {
								if ( vehicleInfo.xyspeed > WALKING_SPEED && botThreadData.bots[ vehicleInfo.driverEntNum ] != NULL && botThreadData.bots[ vehicleInfo.driverEntNum ]->vehiclePauseTime < botWorld->gameLocalInfo.time ) { //mal: hes not pausing, so we will.
									vehiclePauseTime = botWorld->gameLocalInfo.time + VEHICLE_PAUSE_TIME;
									continue;
								}
							}
						}
					}
				} else if ( !( botVehicleInfo->flags & PERSONAL ) && ( vehicleInfo.flags & ARMOR ) ) { //mal: small vehicles won't worry about armor - unless the driver is human
					if ( InFrontOfVehicle( vehicleInfo.entNum, botVehicleInfo->origin, true ) ) { //mal: ok - this is the pain: what is the other guy doing?
						if ( vehicleInfo.driverEntNum != -1 ) {
							clientInfo_t driver = botWorld->clientInfo[ vehicleInfo.driverEntNum ];

							if ( !driver.isBot ) {
								if ( vehicleInfo.xyspeed > WALKING_SPEED ) {
									vehiclePauseTime = botWorld->gameLocalInfo.time + VEHICLE_PAUSE_TIME; //mal: always defer to the human.
									continue;
								}
							}
						}
					}
				}
			}
		}

		if ( botVehicleInfo != NULL && botVehicleInfo->flags & ARMOR && !( vehicleInfo.flags & ARMOR ) ) {
			if ( inFront && distSqr > Square( 512.0f ) && !InFrontOfVehicle( vehicleInfo.entNum, botVehicleInfo->origin ) && vehicleInfo.xyspeed > 0.0f && vehicleInfo.type != MCP ) {
				continue;
			}
		}

		if ( botVehicleInfo != NULL && distSqr < Square( 1024.0f ) && InFrontOfVehicle( botInfo->proxyInfo.entNum, vehicleInfo.origin, true ) && vehicleInfo.xyspeed > WALKING_SPEED ) {
			vehiclePauseTime = botWorld->gameLocalInfo.time + VEHICLE_PAUSE_TIME; 
			continue;
		}

		if ( !inFront && !box.Expand( DYNAMIC_OBSTACLE_CONSIDER_DISTANCE ).ContainsPoint( botInfo->origin ) ) {
			continue;
		}

		obstacles.AddObstacle( box, vehicleInfo.entNum );
		
		if ( botThreadData.AllowDebugData() ) {
			if ( bot_debugObstacles.GetBool() ) {
				gameRenderWorld->DebugBox( colorGreen, box, 128 );
			}
		}
	}

	return botShouldMoveCautiously;
}

/*
================
idBotAI::BuildObstacleList

Builds a list of the current obstacles around the player, so that the bot can avoid them.
================
*/
bool idBotAI::BuildObstacleList( bool largePlayerBBox, bool inVehicle ) {
	bool inAirVehicle = false;
	bool botShouldMoveCautiously = false;

	obstacles.ClearObstacles();

	if ( botVehicleInfo != NULL && botVehicleInfo->driverEntNum != botNum ) {
		return false;
	}

	if ( botInfo->onLadder ) {
		return false;
	}

	if ( inVehicle && botVehicleInfo != NULL ) {
		if ( botVehicleInfo->type > ICARUS ) {
			inAirVehicle = true;
		}
	}

	if ( !inAirVehicle ) {
		botShouldMoveCautiously |= AddGroundObstacles( largePlayerBBox, inVehicle );
	}

	botShouldMoveCautiously |= AddGroundAndAirObstacles( largePlayerBBox, inVehicle, inAirVehicle );

	return botShouldMoveCautiously;
}

/*
================
idBotAI::FlyerHive_BuildPlayerObstacleList
================
*/
void idBotAI::FlyerHive_BuildPlayerObstacleList() {

	obstacles.ClearObstacles();

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		//mal: ignore players outside the valid AAS space
		if ( playerInfo.areaNum == 0 ) {
			continue;
		}

		idVec3 vec = playerInfo.origin - botInfo->weapInfo.covertToolInfo.origin;

		//mal: keep the range pretty tight.
		if ( vec.LengthSqr() > Square( 100.0f ) ) {
			continue;
		}

		idBox box = idBox( playerInfo.origin, idVec3( 64.0f, 64.0f, 84.0f ), playerInfo.bodyAxis );
		
		obstacles.AddObstacle( box, i );
	}
}

/*
================
idBotAI::Bot_ApproxTravelTimeToLocation

Just does a "as the crow flies" check of how long it would take, approx, to reach a location.
================
*/
int idBotAI::Bot_ApproxTravelTimeToLocation( const idVec3 & from, const idVec3 &to, bool inVehicle ) const {
	float dist = ( to - from ).LengthFast();

	dist *= 100.0f / ( ( inVehicle ) ? BASE_VEHICLE_SPEED : RUNNING_SPEED ); //mal: just use this speed as a base, since we can't know for sure if the bot can/will sprint/strafejump, or what vehicle its in.

	if ( dist < 1.0f ) {
		return 1;
	}

	return ( int ) dist;
}

/*
================
idBotAI::Bot_FindRouteToCurrentGoal
================
*/
void idBotAI::Bot_FindRouteToCurrentGoal() {

	int i;
	idVec3 vec;
	routeNode = NULL;

	if ( !botWorld->gameLocalInfo.debugAltRoutes ) {
		return;
	}

	if ( actionNum < 0 || actionNum > botThreadData.botActions.Num() ) {
		return;
	}

	if ( botThreadData.botActions[ actionNum ]->routeID == -1 ) {
		return;
	}

	if ( botVehicleInfo != NULL ) {
		return;
	}

	for( i = 0; i < botThreadData.botRoutes.Num(); i++ ) {
		
		if ( botThreadData.botRoutes[ i ]->groupID != botThreadData.botActions[ actionNum ]->routeID ) {
			continue;
		}

		if ( botThreadData.botRoutes[ i ]->isHeadNode == false ) {
			continue;
		}

		if ( botThreadData.botRoutes[ i ]->active == false ) {
			continue;
		}

		if ( botThreadData.botRoutes[ i ]->team != botInfo->team && botThreadData.botRoutes[ i ]->team != NOTEAM ) {
			continue;
		}

		vec = botThreadData.botRoutes[ i ]->origin - botInfo->origin;
		vec.z = 0.0f; //mal: dont take into account height!

		if ( vec.LengthSqr() > Square( botThreadData.botRoutes[ i ]->radius ) ) {
			continue;
		}

		routeNode = botThreadData.botRoutes[ i ];

		break;
	}
}

/*
================
idBotAI::Bot_FindNextRouteToGoal
================
*/
void idBotAI::Bot_FindNextRouteToGoal() {

	bool useStack = false;
	int i;
	idBotRoutes* tempRoute;
	idList< idBotRoutes* >	tempRouteList;

	if ( routeNode == NULL && AIStack.routeNode == NULL ) { //mal: oops!
        return;
	}

	if ( routeNode == AIStack.routeNode || ( routeNode == NULL && AIStack.routeNode != NULL ) ) {
		useStack = true;
	}

	if ( routeNode != NULL ) {
		tempRoute = routeNode;
	} else if ( AIStack.routeNode != NULL ) {
		tempRoute = AIStack.routeNode;
	} else {
		assert( false );
		routeNode = NULL;
		AIStack.routeNode = NULL;
		return;
	}

	if ( tempRoute->routeLinks.Num() == 0 ) { //mal: reached the end of the road
		routeNode = NULL;
		AIStack.routeNode = NULL;
		return;
	}

	tempRouteList.Clear();

	for( i = 0; i < tempRoute->routeLinks.Num(); i++ ) { //mal: go thru and find only the active route links.
		if ( !tempRoute->routeLinks[ i ]->active ) {
			continue;
		}

		tempRouteList.Append( tempRoute->routeLinks[ i ] );
	}

	if ( tempRouteList.Num() == 0 ) {
		routeNode = NULL;
		AIStack.routeNode = NULL;
		return;
	}

    i = botThreadData.random.RandomInt( tempRouteList.Num() );

	if ( tempRouteList[ i ] == NULL ) { //mal: should NEVER happen!
		routeNode = NULL;
		assert( false );
		return;
	}

	routeNode = tempRouteList[ i ];

	if ( useStack ) {
        AIStack.routeNode = tempRouteList[ i ]; //mal: update the stack too, so if we leave our AI node, we'll see the new route when we come back to it.
	}
}

/*
================
idBotAI::Bot_ReachedCurrentRouteNode
================
*/
bool idBotAI::Bot_ReachedCurrentRouteNode() {

	idVec3 vec;

	if ( routeNode == NULL ) {
		return false;
	}

	if ( routeNode->active == false ) { //mal: someone turned it off while we were on the way - find a different path.
		return true;
	}

	vec = routeNode->origin - botInfo->origin;

	if ( vec.LengthSqr() > Square( routeNode->radius ) || botInfo->onLadder ) {
		return false;
	}

	return true;
}

/*
================
idBotAI::Bot_DirectionToLocation

Short and simple. Doesn't need to be really precise, just the general dir to target.
================
*/
const moveDirections_t idBotAI::Bot_DirectionToLocation( const idVec3& location, bool precise ) {
	idMat3 axis;
	idVec3 org;

	if ( botVehicleInfo == NULL ) {
		axis = botInfo->bodyAxis;
		org = botInfo->origin;
	} else {
		axis = botVehicleInfo->axis;
		org = botVehicleInfo->origin;
	}

	idVec3 dir = location - org;
	dir.NormalizeFast();

	if ( precise ) {
		if ( dir * axis[ 0 ] > 0.95f ) {
			return FORWARD;
		} else if ( -dir * axis[ 0 ] > 0.50f ) {
			return BACK;
		} else if ( dir * axis[ 1 ] > 0.50f ) {
			return LEFT;
		} else { 
			return RIGHT;
		}
	} else {
		if ( dir * axis[ 0 ] > 0.50f ) {
			return FORWARD;
		} else if ( -dir * axis[ 0 ] > 0.50f ) {
			return BACK;
		} else if ( dir * axis[ 1 ] > 0.50f ) {
			return LEFT;
		} else { 
			return RIGHT;
		}
	}
}

/*
================
idBotAI::Bot_LocationIsReachable

Use the AAS to find out if a location is reachable.
================
*/
bool idBotAI::Bot_LocationIsReachable( bool inVehicle, const idVec3& loc, int& travelTime ) {
	int travelFlags = ( botInfo->team == GDF ) ? TFL_VALID_GDF : TFL_VALID_STROGG;
	int botAreaNum = ( inVehicle ) ? botInfo->areaNumVehicle : botInfo->areaNum;
	int locAreaNum = botThreadData.Nav_GetAreaNum( ( inVehicle ) ? AAS_VEHICLE : AAS_PLAYER, loc );
	const aasReachability_t *reach;
	idVec3 botOrigin = ( inVehicle ) ? botInfo->aasVehicleOrigin : botInfo->aasOrigin;

	if ( botInfo->isDisguised ) { //mal: if disguised, we can go anywhere we want.
		travelFlags = TFL_VALID_GDF_AND_STROGG;
	}

	idAAS*	aas;

	if ( inVehicle ) {
		aas = botThreadData.GetAAS( AAS_VEHICLE );
	} else {
		aas = botThreadData.GetAAS( AAS_PLAYER );
	}

	if ( aas == NULL ) {
		return false;
	}

	if ( !aas->RouteToGoalArea( botAreaNum, botOrigin, locAreaNum, travelFlags, travelTime, &reach ) ) {
		return false;
	}//mal: can't reach the target - prolly behind a team specific shield/wall/etc.

	return true;
}

/*
================
idBotAI::Bot_SetupFlyerMove

Sets up the bot's path goal with the flyer hive. 
================
*/
void idBotAI::Bot_SetupFlyerMove( idVec3& goalOrigin, int goalAreaNum ) {
	if ( botAAS.aas == NULL ) {
		return;
	}

	FlyerHive_BuildPlayerObstacleList();

	int travelFlags = TFL_VALID_STROGG;
	int walkTravelFlags = TFL_VALID_WALK_STROGG;

	idVec3 hiveOrigin = botInfo->weapInfo.covertToolInfo.origin;
	int hiveAreaNum = botAAS.aas->PointReachableAreaNum( hiveOrigin, botAAS.aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
	
	botAAS.aas->PushPointIntoArea( hiveAreaNum, hiveOrigin );

	if ( goalAreaNum == 0 ) {
		goalAreaNum = botAAS.aas->PointReachableAreaNum( goalOrigin, botAAS.aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
	}

	if ( hiveAreaNum != goalAreaNum ) {
		botAAS.aas->PushPointIntoArea( goalAreaNum, goalOrigin );
	}

	idObstacleAvoidance::obstaclePath_t path;

	botAAS.hasPath = botAAS.aas->WalkPathToGoal( botAAS.path, hiveAreaNum, hiveOrigin, goalAreaNum, goalOrigin, travelFlags, walkTravelFlags );

	if ( botThreadData.AllowDebugData() ) {
		if ( bot_showPath.GetInteger() == botNum ) {
			botAAS.aas->ShowWalkPath( hiveAreaNum, hiveOrigin, goalAreaNum, goalOrigin, travelFlags, walkTravelFlags );
		}
	}

	botAAS.hasClearPath = obstacles.FindPathAroundObstacles( botInfo->localBounds, botAAS.aas->GetSettings()->obstaclePVSRadius, botAAS.aas, hiveOrigin, botAAS.path.moveGoal, path );

    botAAS.path.moveGoal = path.seekPos;
	botAAS.obstacleNum = path.firstObstacle;
}

/*
============
idBotAI::TravelFlagForTeam
============
*/
int idBotAI::TravelFlagForTeam() const {
	switch( botInfo->team ) {
		case GDF: return TFL_VALID_GDF;
		case STROGG: return TFL_VALID_STROGG;
	}
	return TFL_VALID_GDF_AND_STROGG;
}

/*
============
idBotAI::TravelFlagWalkForTeam
============
*/
int idBotAI::TravelFlagWalkForTeam() const {
	switch( botInfo->team ) {
		case GDF: return TFL_VALID_WALK_GDF;
		case STROGG: return TFL_VALID_WALK_STROGG;
	}
	return TFL_VALID_WALK_GDF_AND_STROGG;
}

/*
============
idBotAI::TravelFlagInvalidForTeam
============
*/
int idBotAI::TravelFlagInvalidForTeam() const {
	switch( botInfo->team ) {
		case GDF: return TFL_INVALID|TFL_INVALID_GDF;
		case STROGG: return TFL_INVALID|TFL_INVALID_STROGG;
	}
	return TFL_INVALID|TFL_INVALID_GDF|TFL_INVALID_STROGG;
}



/*
================
idBotAI::LocationIsReachable

Use the AAS to find out if a location is reachable.
================
*/
bool idBotAI::LocationIsReachable( bool inVehicle, const idVec3& loc1, const idVec3& loc2, int& travelTime ) {
	int travelFlags = ( botInfo->team == GDF ) ? TFL_VALID_GDF : TFL_VALID_STROGG;
	int loc1AreaNum = botThreadData.Nav_GetAreaNum( ( inVehicle ) ? AAS_VEHICLE : AAS_PLAYER, loc1 );
	int loc2AreaNum = botThreadData.Nav_GetAreaNum( ( inVehicle ) ? AAS_VEHICLE : AAS_PLAYER, loc2 );
	const aasReachability_t *reach;
	idVec3 startOrg = loc1;

	idAAS*	aas;

	if ( inVehicle ) {
		aas = botThreadData.GetAAS( AAS_VEHICLE );
	} else {
		aas = botThreadData.GetAAS( AAS_PLAYER );
	}

	if ( aas == NULL ) {
		return false;
	}

	if ( loc1AreaNum != loc2AreaNum ) {
		aas->PushPointIntoArea( loc1AreaNum, startOrg );
	}

	if ( !aas->RouteToGoalArea( loc1AreaNum, startOrg, loc2AreaNum, travelFlags, travelTime, &reach ) ) {
		return false;
	}//mal: can't reach the target - prolly behind a team specific shield/wall/etc.

	return true;
}

/*
================
idBotAI::Bot_CheckIfObstacleInArea
================
*/
bool idBotAI::Bot_CheckIfObstacleInArea( float minAvoidDist ) {
	bool hasObstacle = false;

	for( int i = 0; i < botThreadData.botObstacles.Num(); i++ ) {
		
		idBotObstacle *obstacle = botThreadData.botObstacles[ i ];
		idVec3 vec = obstacle->bbox.GetCenter() - botInfo->origin;

		float distSqr = vec.LengthSqr();
		distSqr -= obstacle->bbox.GetExtents().LengthSqr();

		if ( distSqr > Square( minAvoidDist ) ) {
			continue;
		}

		hasObstacle = true;
		break;
	}

	return hasObstacle;
}

