// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "../ContentMask.h"
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
==================
idBotAI::GetVehicleInfo

Returns all the info about a particular vehicle.
==================
*/
void idBotAI::GetVehicleInfo( int entNum, proxyInfo_t& vehicleInfo ) const {
	vehicleInfo.entNum = 0;

	for( int i = 0; i < MAX_VEHICLES; i++ ) {

		if ( botWorld->vehicleInfo[ i ].entNum != entNum ) {
			continue;
		}

		vehicleInfo = botWorld->vehicleInfo[ i ];
		break;
	}

	if ( vehicleInfo.entNum == 0 ) {
//		assert( false );
	}
}

/*
==================
idBotAI::GetVehicleInfo

Returns all the info about a particular vehicle.
==================
*/
const proxyInfo_t *idBotAI::GetBotVehicleInfo( int entNum ) const {
	for( int i = 0; i < MAX_VEHICLES; i++ ) {
		if ( botWorld->vehicleInfo[ i ].entNum != entNum ) {
			continue;
		}
		return &botWorld->vehicleInfo[ i ];
	}
	return NULL;
}

/*
==================
idBotAI::VehicleIsValid

Checks to make sure the vehicle in question is still valid.
==================
*/
bool idBotAI::VehicleIsValid( int entNum, bool skipSpeedCheck, bool addDriverCheck ) {

	proxyInfo_t vehicleInfo;

	GetVehicleInfo( entNum, vehicleInfo );

	if ( vehicleInfo.entNum == 0 ) {
		return false;
	}

	if ( !vehicleInfo.hasGroundContact && vehicleInfo.type != DESECRATOR && !vehicleInfo.inWater ) { //mal: desecrator NEVER has groundcontact
		return false;
	}

	if ( !vehicleInfo.hasFreeSeat ) {
		return false;
	}

	if ( vehicleInfo.flags & PERSONAL ) {
		return false;
	}

	if ( vehicleInfo.isBoobyTrapped && vehicleInfo.type != MCP ) {
		return false;
	}

	if ( !vehicleInfo.inPlayZone ) {
		return false;
	}

	if ( !VehicleHasGunnerSeatOpen( entNum ) ) {
		if ( addDriverCheck == false ) {
			return false;
		} else {
			if ( vehicleInfo.driverEntNum != -1 ) {
				return false;
			}
		}
	}

	if ( vehicleInfo.inWater && !( vehicleInfo.flags & WATER ) ) {
		return false;
	}

	if ( vehicleInfo.isFlipped ) {
		return false;
	}

	if ( vehicleInfo.damagedPartsCount > 0 && botInfo->classType != ENGINEER ) {
		return false;
	}

	if ( !skipSpeedCheck ) {
		if ( vehicleInfo.xyspeed > WALKING_SPEED ) {
			return false;
		}
	}

	if ( vehicleInfo.areaNum == 0 ) {
		return false;
	}

	return true;
}


/*
==================
idBotAI::FindClosestVehicle

Returns the entity number of the closest vehicle to the bot, matching vehicleType, within a certain range.
==================
*/
int idBotAI::FindClosestVehicle( float range, const idVec3& org, const playerVehicleTypes_t vehicleType, int vehicleFlags, int vehicleIgnoreFlags, bool emptyOnly ) {
	int entNum = -1;
	int i;
	float closest = idMath::INFINITY;
	float dist;
	idVec3 vec;

	if ( botInfo->isDisguised ) {
		return -1;
	}

	if ( vehicleIgnoreFlags == -1 ) {
		return -1;
	}

	if ( !botWorld->gameLocalInfo.botsUseVehicles && vehicleType != MCP ) {
		return -1;
	}

 //mal_hack: these maps weren't originally setup for boats...
	if ( botWorld->gameLocalInfo.gameMap == ISLAND ) {
		range = 3500.0f;
	} else if ( botWorld->gameLocalInfo.gameMap == VALLEY ) {
		range = 4500.0f;
	} else if ( botWorld->gameLocalInfo.gameMap == VOLCANO ) {
		range = 7000.0f;
	}

	for( i = 0; i < MAX_VEHICLES; i++ ) {

		proxyInfo_t vehicle = botWorld->vehicleInfo[ i ];

		if ( vehicle.entNum == 0 ) {
			continue;
		}

		if ( vehicle.team != botInfo->team ) {
			continue;
		}

		if ( vehicle.flags & PERSONAL  && !( vehicleFlags & PERSONAL ) && vehicleFlags != 0 ) { //mal: have to specify personal vehicles
			continue;
		}

		if ( vehicle.inWater && !( vehicle.flags & WATER ) ) { //mal: someone drove it into the ocean.
			continue;
		}

		if ( vehicle.areaNumVehicle == 0 || vehicle.areaNum == 0 ) {
			continue;
		}

		if ( vehicle.isFlipped ) { //mal: its on its back.
			continue;
		}

		if ( vehicle.isBoobyTrapped && vehicle.type != MCP ) {
			continue;
		}

		if ( vehicle.type == PLATYPUS && !vehicle.neverDriven ) { 
			continue;
		}

		if ( !vehicle.hasGroundContact && vehicle.type != DESECRATOR && vehicle.type != MCP && !vehicle.inWater ) {
			continue;
		}

		if ( vehicle.wheelsAreOnGround != 1.0f && vehicle.type != MCP ) {
			continue;
		}

		if ( vehicle.type == ICARUS && ignoreIcarusTime > botWorld->gameLocalInfo.time ) {
			continue;
		}

		if ( vehicleIgnoreFlags > 0 ) {
			if ( !botWorld->gameLocalInfo.debugPersonalVehicles ) {
				if ( vehicle.flags & vehicleIgnoreFlags ) {
					continue;
				}
			}
		}

		if ( VehicleIsIgnored( vehicle.entNum ) ) {
			continue;
		}

		if ( vehicle.damagedPartsCount > 0 && botInfo->classType != ENGINEER ) { //mal: dont get a vehicle that has a missing wheel, unless we can fix it!
			continue;
		}

		if ( vehicle.type == MCP && botWorld->gameLocalInfo.heroMode && TeamHasHuman( botInfo->team ) ) {
			continue;
		}

		if ( vehicle.isOwned && vehicle.type != MCP && vehicle.ownerNum != botNum ) {
			continue;
		}

		if ( vehicle.type == MCP && vehicleType != MCP ) { //mal: sorry - have to pick the MCP by name.
			continue;
		}

		if ( vehicle.spawnID == lastVehicleSpawnID && lastVehicleTime + MINIMUM_VEHICLE_IGNORE_TIME > botWorld->gameLocalInfo.time ) { //mal: dont jump in and out of the same vehicle.
			continue;
		}

		if ( vehicle.xyspeed > ( RUNNING_SPEED - 50.0f ) && !vehicle.isEmpty ) { //mal: slow enough we could run and catch it.
			continue;
		} //mal: dont worrry about vehicles moving, unless its empty ( and just coasting along ).

		if ( emptyOnly ) {
			if ( vehicle.type == MCP ) {
				if ( vehicle.driverEntNum != -1 ) {
					continue;
				}
			} else {
				if ( !vehicle.isEmpty ) {
					continue;
				}
			}
		} else {
			if ( !vehicle.hasFreeSeat ) {
				continue;
			}
		}

		if ( !vehicle.inPlayZone ) {
			if ( botInfo->inPlayZone ) { //mal: vehicles out of bounds are just a hazard to us - ignore them.
                continue; //mal: unless we're out of bounds too - then they're our ticket out of here!
			}
		}

		if ( vehicleType != NULL_VEHICLE ) { //mal: are we looking for a specific vehicle.....
			if ( vehicle.type != vehicleType ) {
				continue;
			}
		} else if ( vehicleFlags != NULL_VEHICLE_FLAGS ) { //mal: or just a general class of vehicles
			if ( !( vehicle.flags & vehicleFlags ) ) {
				if ( vehicle.type != PLATYPUS || ( botWorld->gameLocalInfo.gameMap != VALLEY && botWorld->gameLocalInfo.gameMap != ISLAND ) ) { //mal_HACK: these maps had boat support added late...
					continue;
				}
			}
		}

//mal: we wont defer to a human if the vehicle is owned by us. We WILL still wait for him when we jump in, so he can have the chance to ride with us.
		if ( vehicle.type != MCP && ( !vehicle.isOwned || vehicle.ownerNum != botNum ) ) {
            if ( TeamHumanNearLocation( botInfo->team, vehicle.origin, HUMAN_OWN_VEHICLE_DIST ) ) { //mal: theres a human nearby, he might want this vehicle so defer to him.
				continue;
			}
		}

		vec = vehicle.origin - org;

		dist = vec.LengthSqr();

		if ( dist > Square( range ) ) {
			continue;
		}

		idVec3 vehicleOrigin = vehicle.origin;
		vehicleOrigin.z += 32.0f; //mal: move it up a bit for safety.

		int travelTime; 

		if ( !Bot_LocationIsReachable( ( botVehicleInfo != NULL ) ? true : false, vehicleOrigin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) {
			entNum = vehicle.entNum;
			closest = dist;
		}
	}

	return entNum;
}

/*
==================
idBotAI::Bot_ExitVehicle
==================
*/
void idBotAI::Bot_ExitVehicle( bool ignoreMCP ) {
	if ( botVehicleInfo != NULL && botVehicleInfo->type == MCP && ignoreMCP ) {
		return;
	}

	if ( botWorld->gameLocalInfo.botsStayInVehicles ) { //mal: someone wants me to stay in here for debugging purposes.
		return;
	}

	if ( botVehicleInfo != NULL ) {
		if ( ( botInfo->classType != ENGINEER || vLTGType != V_STOP_VEHICLE ) && botVehicleInfo->driverEntNum != botNum && ClientIsValid( botVehicleInfo->driverEntNum, -1 ) && botInfo->proxyInfo.weapon != NULL_VEHICLE_WEAPON ) { //mal: if we're riding along with a human, never leave the vehicle - its frustrating for the human, unless we have no weapon to use.
			const clientInfo_t& player = botWorld->clientInfo[ botVehicleInfo->driverEntNum ];
	
			if ( !player.isBot ) {
				if ( Bot_WithObjShouldLeaveVehicle() ) { //mal: if the bot has the obj - they should leave if the player stops the vehicle.
					goto leaveThisVehicle;
				}
				vLTGTarget = botVehicleInfo->driverEntNum;
				vLTGTargetSpawnID = player.spawnID;
				vLTGTime = ( ClientHasObj( botNum ) ) ? botWorld->gameLocalInfo.time + 10000 : botWorld->gameLocalInfo.time + BOT_INFINITY; //mal: if have obj, do constant checks as to whether or not we should leave.
				V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
				V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_RideWithMate;
				return;
			}
		}
	}

leaveThisVehicle:

	if ( botVehicleInfo != NULL ) {
		lastVehicleSpawnID = botVehicleInfo->spawnID;
		lastVehicleTime = botWorld->gameLocalInfo.time;

		if ( botVehicleInfo->type == ICARUS ) {
			ignoreIcarusTime = botWorld->gameLocalInfo.time + IGNORE_ICARUS_TIME;
		}
	}

    botUcmd->botCmds.exitVehicle = true;
	botUcmd->moveType = FULL_STOP;
	botExitTime = botWorld->gameLocalInfo.time + BOT_THINK_DELAY_TIME;
	botPathFailedCounter = 0;
	vehicleAINodeSwitch.nodeSwitchCount = 0;
	vehicleAINodeSwitch.nodeSwitchTime = 0;
}


/*
==================
idBotAI::Bot_FindParkingSpotAoundLocaction
==================
*/
bool idBotAI::Bot_FindParkingSpotAoundLocaction( idVec3 &loc ) {
	float checkDist = 500.0f;
	idAngles ang;
	idVec3 end;

	end = loc;

	ang = end.ToAngles();

	ang[ PITCH ] = 0.0f;

	end += ( checkDist * ang.ToMat3()[ 0 ] );

	if ( !botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, NULL_AREANUM, loc, end ) ) {

		end = loc;
		ang[ YAW ] += 90.0f;
		end += ( checkDist * ang.ToMat3()[ 0 ] );

		if ( !botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, NULL_AREANUM, loc, end ) ) {
			end = loc;
			ang[ YAW ] += 90.0f;
			end += ( checkDist * ang.ToMat3()[ 0 ] );
	
			if ( !botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, NULL_AREANUM, loc, end ) ) {
				end = loc;
				ang[ YAW ] += 90.0f;
				end += ( checkDist * ang.ToMat3()[ 0 ] );
	
				if ( !botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, NULL_AREANUM, loc, end ) ) {
					return false; //mal: no clear parking spot found, so just use the default location.
				}
			}
		}
	}

	loc = end; //mal: found a clear parking spot.
	return true;
}

/*
==================
idBotAI::InAirVehicleGunSights
==================
*/
bool idBotAI::InAirVehicleGunSights( int vehicleNum, const idVec3 &org ) {

	proxyInfo_t vehicleInfo;

	GetVehicleInfo( vehicleNum, vehicleInfo );

	if ( vehicleInfo.entNum == 0 ) {
		return false;
	}

	idVec3 dir = org - vehicleInfo.origin;

	dir.NormalizeFast();

	if ( dir * vehicleInfo.axis[ 0 ] > 0.3f ) {
		return true;//mal: have someone in front of us..
	}

	return false;
}

/*
==================
idBotAI::InFrontOfVehicle
==================
*/
bool idBotAI::InFrontOfVehicle( int vehicleNum, const idVec3 &org, bool precise, float preciseValue ) {
	proxyInfo_t vehicleInfo;

	GetVehicleInfo( vehicleNum, vehicleInfo );

	if ( vehicleInfo.entNum == 0 ) {
		return false;
	}

	idVec3 dir = org - vehicleInfo.origin;
	float dotCheck = 0.0f;

	if ( precise ) {
		dir.NormalizeFast(); 
		dotCheck = preciseValue;
	}

	if ( dir * vehicleInfo.axis[ 0 ] > dotCheck ) {
		return true;//mal: have someone in front of us..
	}
	
	return false;
}

/*
==================
idBotAI::Bot_PickBestVehiclePosition

See what the best seat in this ride is. Will be called when the bots not in combat ( will detect if a gunner's seat opens, or if our driver bails )
==================
*/
void idBotAI::Bot_PickBestVehiclePosition() {

	if ( !botVehicleInfo->hasFreeSeat ) {
		return;
	}

	if ( botVehicleInfo->driverEntNum == botNum ) {
		return;
	} //mal: we're in the best position on this ride.

	if ( botVehicleInfo->driverEntNum == -1 ) {
		botUcmd->botCmds.becomeDriver = true;
	}

	if ( botInfo->proxyInfo.weapon != MINIGUN && botInfo->proxyInfo.weapon != LAW ) { //mal: if we're not already a gunner, check to see if a seat is open.
        if ( VehicleHasGunnerSeatOpen( botVehicleInfo->entNum ) ) {
			botUcmd->botCmds.becomeGunner = true;
		} else {
			if ( botVehicleInfo->type == BADGER ) {
				if ( botInfo->proxyInfo.weapon == NULL_VEHICLE_WEAPON ) { //mal: sitting on the bumper lets us use our SMG at least....
					botUcmd->botCmds.activate = true;
				}
			}
		}
	}
}

/*
================
idBotAI::Bot_ExitVehicleAINode

Vechile specific "Exit AI Node"
================
*/
void idBotAI::Bot_ExitVehicleAINode( bool resetStack ) {
	lastActionNum = actionNum;
    Bot_ResetState( false, resetStack );
	ResetRandomLook();
	botIdealWeapSlot = GUN;
	botIdealWeapNum = NULL_WEAP;

	lastAINode = "Exiting Vehicle AI Node";
}

/*
================
idBotAI::Bot_VehicleCanMove
================
*/
bool idBotAI::Bot_VehicleCanMove( const moveDirections_t direction, float gUnits, bool copyEndPos ) {

	idVec3 end = botVehicleInfo->origin;

	switch ( direction ) {
	
	case FORWARD:
		end += ( gUnits * botVehicleInfo->axis[ 0 ] );
		break;
	
	case BACK:
		end += ( -gUnits * botVehicleInfo->axis[ 0 ] );
		break;

	case LEFT:
		end += ( -gUnits * ( botVehicleInfo->axis[ 1 ] * -1 ) );
		break;

	case RIGHT:
		end += ( gUnits * ( botVehicleInfo->axis[ 1 ] * -1 ) );
		break;
	}

	if ( botThreadData.AllowDebugData() ) { 
		gameRenderWorld->DebugLine( colorGreen, botVehicleInfo->origin, end, 16 );
	}

	if ( botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, botInfo->areaNumVehicle, botVehicleInfo->aasVehicleOrigin, end ) ) {
		botCanMoveGoal = end;
        return true;
	}
	
	return false;
}

/*
================
idBotAI::Bot_SetupVehicleMove

Sets up the bot's path goal while its in a vehicle.
================
*/
void idBotAI::Bot_SetupVehicleMove( const idVec3 &org, int clientNum, int actionNumber, bool ignoreNodes ) {
	if ( vehiclePauseTime > botWorld->gameLocalInfo.time ) {
		botAAS.hasPath = true;
		vLTGTime += 50;
		nodeTimeOut += 50;
		return;
	}
	
	bool moveIsClear = false;
	int areaNum = 0;
	int i;
	float height = -idMath::INFINITY;
	aasTrace_t	tr;
	idVec3 end;
	idVec3 goalOrigin;
	idBounds bbox = botAAS.aas->GetSettings()->boundingBox;

	obstacles.ClearObstacles();

	bool botShouldMoveCautiously = BuildObstacleList( true, true );

	if ( botVehicleInfo->type == ICARUS ) {
		Bot_SetupIcarusMove( org, clientNum, actionNumber );
		return;
	} else if ( botVehicleInfo->type > ICARUS ) { //mal: icarus will use normal path move
		const int MAX_POINTS = 256;
		idVec3 pointsList[ MAX_POINTS ];

		end = botInfo->viewOrigin;
		end.z += 64.0f;
		end += ( 4096.0f * botVehicleInfo->axis[ 0 ] );

		botAAS.hasPath = true;
		botAAS.hasReachedVehicleNodeGoal = false;

		aasTraceHeight_t traceHeight;

		traceHeight.maxPoints = MAX_POINTS;
		traceHeight.numPoints = 0;
		traceHeight.points = pointsList;

		botAAS.aas->TraceHeight( traceHeight, botInfo->viewOrigin, end );

		for( i = 0; i < traceHeight.numPoints; i++ ) {
			if ( traceHeight.points[ i ].z > height ) {
				height = traceHeight.points[ i ].z;

				if ( botThreadData.AllowDebugData() ) {
					if ( bot_showPath.GetInteger() == botNum ) {
						gameRenderWorld->DebugLine( colorGreen, traceHeight.points[ i ], traceHeight.points[ i ] + idVec3( 0.0f, 0.0f, 128.0f ) );
						if ( i > 0 ) {
							gameRenderWorld->DebugLine( colorLtBlue, traceHeight.points[ i - 1 ], traceHeight.points[ i ] );
						}
					}
				}

				if ( height >= botWorld->gameLocalInfo.maxVehicleHeight - 128.0f ) {
					botAAS.hasPath = false;
				}
			}
		}

		if ( botInfo->origin.z < height ) {
			botUcmd->botCmds.isBlocked = true;
		}

		if ( clientNum != -1 ) {
			botAAS.path.moveGoal = botWorld->clientInfo[ clientNum ].origin;
		} else if ( actionNumber != -1 ) {
			botAAS.path.moveGoal = botThreadData.botActions[ actionNumber ]->origin;
		} else {
			botAAS.path.moveGoal = org;
		}

		botAAS.path.moveGoal.z += height;

		idObstacleAvoidance::obstaclePath_t path;

		botAAS.hasClearPath = obstacles.FindPathAroundObstacles( botVehicleInfo->bbox, botAAS.aas->GetSettings()->obstaclePVSRadius, botAAS.aas, botVehicleInfo->origin, botAAS.path.moveGoal, path );

		botAAS.path.moveGoal = path.seekPos;

		botAAS.obstacleNum = path.firstObstacle;

		if ( botAAS.obstacleNum != -1 ) {
			botUcmd->botCmds.isBlocked = true;
		}

		botAAS.path.viewGoal = botAAS.path.moveGoal;
		idVec3 viewDelta = botAAS.path.moveGoal - botInfo->viewOrigin;
		if ( idMath::Fabs( viewDelta.z ) < 100.0f ) {
			botAAS.path.viewGoal.z = botInfo->viewOrigin.z;
		}
	} else { //mal: must be a ground transport of some kind

		int travelFlags, walkTravelFlags;
		float radius = 64.0f;
		if ( botInfo->team == GDF ) {
			travelFlags = TFL_VALID_GDF;
			walkTravelFlags = TFL_VALID_WALK_GDF;
		} else {
			travelFlags = TFL_VALID_STROGG;
			walkTravelFlags = TFL_VALID_WALK_STROGG;
		}

	    if ( clientNum != -1 ) {
			end = botWorld->clientInfo[ clientNum ].aasVehicleOrigin;		
		} else if ( actionNumber != -1 ) {
			end = botThreadData.botActions[ actionNumber ]->GetActionOrigin();
			radius = botThreadData.botActions[ actionNumber ]->GetRadius();
		} else {
			end = org;
		}

		bool resetGoal = true;

		if ( !ignoreNodes ) {
			botAAS.hasReachedVehicleNodeGoal = Bot_CheckVehicleNodePath( end, goalOrigin, resetGoal );
		} else {
			botAAS.hasReachedVehicleNodeGoal = false;
		}

		if ( resetGoal ) {
			badMoveTime = 500;
			botUcmd->specialMoveType = SKIP_MOVE;
			return;
		}

		areaNum = botAAS.aas->PointReachableAreaNum( goalOrigin, bbox, AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );

		if ( botVehicleInfo->areaNumVehicle != areaNum ) {
			botAAS.aas->PushPointIntoArea( areaNum, goalOrigin );
		}

		idObstacleAvoidance::obstaclePath_t path;

		botAAS.hasPath = botAAS.aas->WalkPathToGoal( botAAS.path, botVehicleInfo->areaNumVehicle, botVehicleInfo->aasVehicleOrigin, areaNum, goalOrigin, travelFlags, walkTravelFlags );

		if ( botThreadData.AllowDebugData() ) {
			if ( bot_showPath.GetInteger() == botNum ) {
				botAAS.aas->ShowWalkPath( botVehicleInfo->areaNumVehicle, botVehicleInfo->aasVehicleOrigin, areaNum, goalOrigin, travelFlags, walkTravelFlags );
				gameRenderWorld->DebugCircle( colorGreen, end, idVec3( 0, 0, 1 ), radius, 32 );
				end.z += 8.0f;
				gameRenderWorld->DebugCircle( colorRed, end, idVec3( 0, 0, 1 ), radius, 32 );
				end.z += 8.0f;
				gameRenderWorld->DebugCircle( colorGreen, end, idVec3( 0, 0, 1 ), radius, 32 );
				end.z += 8.0f;
				gameRenderWorld->DebugCircle( colorRed, end, idVec3( 0, 0, 1 ), radius, 32 );
			}
		}

		
		//mal: experimented with a larger bbox for vehicle, now experiment with normal bbox again.

		float halfVehicleLength = botVehicleInfo->bbox[1][0];
		idVec3 vehicleOrg = botVehicleInfo->origin;

		vehicleOrg += ( halfVehicleLength * botVehicleInfo->axis[ 0 ] );

/*
		if ( botVehicleInfo->isAirborneVehicle ) { //mal: do this AFTER calculate vehicleOrg.
			halfVehicleLength *= 1.5f;
		} else {
			halfVehicleLength *= 1.25f;
		}

		idBounds vehicleBounds;
		vehicleBounds[0][0] = vehicleBounds[0][1] = -halfVehicleLength;
		vehicleBounds[1][0] = vehicleBounds[1][1] = halfVehicleLength;
		vehicleBounds[0][2] = botVehicleInfo->bbox[0][2];
		vehicleBounds[1][2] = botVehicleInfo->bbox[1][2];
*/

		botAAS.hasClearPath = obstacles.FindPathAroundObstacles( botVehicleInfo->bbox /*vehicleBounds*/, botAAS.aas->GetSettings()->obstaclePVSRadius, botAAS.aas, vehicleOrg, botAAS.path.moveGoal, path );

		botAAS.path.moveGoal = path.seekPos;

		botAAS.obstacleNum = path.firstObstacle;

		if ( botAAS.obstacleNum != -1 && botVehiclePathList.Num() > 0 ) {
			idVec3 origin;
			idBox obstacleBox;
			bool foundEnt = FindEntityByEntNum( botAAS.obstacleNum, origin, obstacleBox );

			if ( foundEnt ) {
				if ( obstacleBox.ContainsPoint( botVehiclePathList[ botVehiclePathList.Num() - 1 ].node->origin ) ) {
					botVehiclePathList.SetNum( botVehiclePathList.Num() - 1 );
				}
			}
		}

		if ( botVehicleInfo->canRotateInPlace && path.originalSeekPos != goalOrigin && botUcmd->botCmds.isBlocked == true ) { //mal: have to set this here, AFTER we run the path finding, but before we do the "moveIsClear" checks.
			botUcmd->botCmds.isBlocked = false;
		}

		if ( botShouldMoveCautiously && botVehicleInfo->type != GOLIATH ) { //mal: goliath looks nasty when it tries to move slow.
			botUcmd->specialMoveType = SLOWMOVE; 
		}

		moveIsClear = Bot_CheckMoveIsClear( botAAS.path.moveGoal );

		if ( botVehiclePathList.Num() > 0 && botVehicleInfo->canRotateInPlace == false && newPathTime > botWorld->gameLocalInfo.time && botVehicleInfo->type != PLATYPUS ) { //mal: if the first node on our path is directly behind us, back up a bit to reach it.
			idVec3 dir = botVehiclePathList[ botVehiclePathList.Num() - 1 ].node->origin - botVehicleInfo->origin;
			float nodeDist = dir.LengthSqr();
			dir.NormalizeFast();
			if ( -dir * botVehicleInfo->axis[ 0 ] > 0.5f && nodeDist > Square( botVehiclePathList[ botVehiclePathList.Num() - 1 ].node->radius ) ) {
				botUcmd->specialMoveType = REVERSEMOVE;
			}
		}
		
		if ( !moveIsClear ) {
			framesVehicleStuck++;
			int temp = framesVehicleStuck;
			if ( framesVehicleStuck > 10 ) {
				Bot_ExitVehicleAINode( true );
				framesVehicleStuck = temp;
			} else if ( framesVehicleStuck > 150 ) { //mal: we've been stuck for too long, bail out and do something else!
				Bot_ExitVehicleAINode( true );
				Bot_ExitVehicle();
				Bot_IgnoreVehicle( botVehicleInfo->entNum, 15000 ); 
			}
		} else {
			framesVehicleStuck = 0;
		}

		if ( moveIsClear && botInfo->team == GDF && botVehicleInfo->type != MCP && botUcmd->specialMoveType != REVERSEMOVE ) {
			if ( botWorld->botGoalInfo.botGoal_MCP_VehicleNum != -1 && botWorld->botGoalInfo.mapHasMCPGoal ) {
				proxyInfo_t mcpInfo;
				GetVehicleInfo( botWorld->botGoalInfo.botGoal_MCP_VehicleNum, mcpInfo );
				if ( InFrontOfVehicle( botVehicleInfo->entNum, mcpInfo.origin ) ) {
					idVec3 vec = mcpInfo.origin - botVehicleInfo->origin;
					if ( vec.LengthSqr() < Square( botAAS.aas->GetSettings()->obstaclePVSRadius * 2.0f ) ) {
						botUcmd->specialMoveType = SLOWMOVE;
					}
				}
			}
		}

		if ( path.firstObstacle != -1 && botWorld->gameLocalInfo.botsCanDecayObstacles ) {
			proxyInfo_t vehicle;
			GetVehicleInfo( path.firstObstacle, vehicle );

			if ( vehicle.entNum != 0 && vehicle.type != MCP && vehicle.isEmpty && vehicle.xyspeed == 0.0f && InFrontOfVehicle( botVehicleInfo->entNum, vehicle.origin ) && !vehicle.neverDriven ) {
				int humansInArea = ClientsInArea( botNum, botInfo->origin, 1000.0f, vehicle.team, NOCLASS, false, false , false, true, true, true );

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

/*
		if ( path.firstObstacle != -1 ) {
			botAAS.blockedByObstacleCounterInVehicle++;
		} else {
			botAAS.blockedByObstacleCounterInVehicle = 0;
		}
*/

		if ( !botWorld->gameLocalInfo.inWarmup && botThreadData.random.RandomInt( 100 ) > 98 ) {
			if ( path.firstObstacle > -1 && path.firstObstacle < MAX_CLIENTS ) {
				const clientInfo_t& blockingClient = botWorld->clientInfo[ path.firstObstacle ];
				if ( blockingClient.team == botInfo->team && !blockingClient.isBot ) {
//mal: ONLY say move if both the bot and the client in question are not just spawning in!
					if ( blockingClient.invulnerableEndTime < botWorld->gameLocalInfo.time && botInfo->invulnerableEndTime < botWorld->gameLocalInfo.time ) {
						end = blockingClient.origin - botInfo->origin;

						if ( end.LengthSqr() < Square( 1500.0f ) ) {
		                    botUcmd->desiredChat = MOVE;
							botUcmd->botCmds.honkHorn = true;
						}
					}
				}
			}
	   }

		//mal: play with the origin just a bit, so that its more at eye level. Gets us realistic view goals on the cheap.
		botAAS.path.viewGoal = botAAS.path.moveGoal;
		idVec3 viewDelta = botAAS.path.moveGoal - botInfo->viewOrigin;
		if ( viewDelta.z < -100.f || viewDelta.z > 100.f ) {
			botAAS.path.viewGoal.z = botInfo->viewOrigin.z;
		}
	}
}

/*
================
idBotAI::Bot_SetupVehicleQuickMove

A fast version of SetupMove, that just has the bot blindly move towards its goal pos, only doing dynamic obstacle avoidance checks.
================
*/
void idBotAI::Bot_SetupVehicleQuickMove( const idVec3 &org, bool largePlayerBBox ) {

	idVec3 tempOrigin;

	obstacles.ClearObstacles();

	BuildObstacleList( largePlayerBBox, true );

	idObstacleAvoidance::obstaclePath_t path;

	botAAS.hasClearPath = obstacles.FindPathAroundObstacles( botVehicleInfo->bbox, botAAS.aas->GetSettings()->obstaclePVSRadius, botAAS.aas, botVehicleInfo->origin, org, path );

    botAAS.path.moveGoal = path.seekPos;

	if ( path.firstObstacle > -1 && path.firstObstacle < MAX_CLIENTS && botThreadData.random.RandomInt( 100 ) > 98 ) {
		if ( botWorld->clientInfo[ path.firstObstacle ].team == botInfo->team ) {
			tempOrigin = botWorld->clientInfo[ path.firstObstacle ].origin - botInfo->origin;

			if ( tempOrigin.LengthSqr() < Square( 1500.0f ) ) {
				botUcmd->desiredChat = MOVE;
				botUcmd->botCmds.honkHorn = true;
			}
		}
	}

	botAAS.obstacleNum = path.firstObstacle;
	botAAS.hasPath = path.hasValidPath; //mal: safety check - let the bot know if its blind wanderings just isn't working out.

//mal: play with the origin just a bit, so that its more at eye level. Gets us realistic view goals on the cheap.
	tempOrigin = botAAS.path.moveGoal - botInfo->viewOrigin;
	botAAS.path.viewGoal = botAAS.path.moveGoal;

	if ( tempOrigin[ 2 ] > -100.0f && tempOrigin[ 2 ] < 100.0f ) {
        botAAS.path.viewGoal[ 2 ] -= tempOrigin[ 2 ];
	}
}


/*
================
idBotAI::Bot_CheckMoveIsClear

This is going to be one big, nasty function. You've been warned.
================
*/
bool idBotAI::Bot_CheckMoveIsClear( idVec3& goalOrigin ) {
	bool forwardClear = false;
	float vehicleLength;
	float distToGoal;
	moveDirections_t moveDir;
	idVec3 v;
	idVec3 temp;
	idVec3 vehicleOrg;
	idVec3 forwardVehicleAxis;
	idVec3 rightVehicleAxis;

	v = botVehicleInfo->bbox[1] - botVehicleInfo->bbox[0];
	vehicleLength = v[ 0 ];
	moveDir = Bot_DirectionToLocation( goalOrigin, true );

	v = goalOrigin - botVehicleInfo->origin;

	distToGoal = v.LengthSqr();

	vehicleOrg = botVehicleInfo->origin + ( botVehicleInfo->axis * botVehicleInfo->bbox.GetCenter() );
	
	forwardVehicleAxis = botVehicleInfo->axis[ 0 ];
	forwardVehicleAxis.z = 0.0f;
	forwardVehicleAxis.NormalizeFast();

	rightVehicleAxis = botVehicleInfo->axis[ 1 ];
	rightVehicleAxis.z = 0.0f;
	rightVehicleAxis.NormalizeFast();

	v = vehicleOrg;
	v += ( ( vehicleLength * 3.5f ) * forwardVehicleAxis );

	bool moveIsClear = botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, botInfo->areaNumVehicle, vehicleOrg, v );

	if ( !moveIsClear && !botVehicleInfo->canRotateInPlace ) { 
		botUcmd->specialMoveType = SLOWMOVE; //mal: if in a tight area, dont go gunning it.
	}

	v = vehicleOrg;
	v += ( ( vehicleLength * 0.50f ) * forwardVehicleAxis );

	if ( botThreadData.AllowDebugData() ) {
		gameRenderWorld->DebugLine( colorYellow, vehicleOrg, v );
		gameRenderWorld->DebugLine( colorRed, vehicleOrg, vehicleOrg + idVec3( 0, 0, 128 ) );
		gameRenderWorld->DebugLine( colorGreen, goalOrigin, goalOrigin + idVec3( 0, 0, 256 ) );
	}

	moveIsClear = botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, botInfo->areaNumVehicle, vehicleOrg, v );

	bool isBlocked = false;

	temp = botVehicleInfo->aasVehicleOrigin - botVehicleInfo->origin;

	if ( botThreadData.AllowDebugData() ) {
		float tempFloat = temp.LengthFast();
		gameRenderWorld->DrawText( va( "Dist = %.0f", tempFloat ), botVehicleInfo->origin + idVec3( 0, 0, 256 ), .5, colorWhite, mat3_identity );
	}

	if ( temp.LengthSqr() > Square( vehicleLength / 2.f ) ) {
		isBlocked = true;
	}

	if ( botAAS.obstacleNum != -1 && botVehicleInfo->type != MCP ) { //mal: the obstacle avoidance should get us out of most situations, but if we're too close, it might fail. The MCP should just roll over all obstacles.
		idVec3 origin;
		idBox bbox;
		bool foundEnt = FindEntityByEntNum( botAAS.obstacleNum, origin, bbox );

		if ( foundEnt ) {
			idVec3 vec = origin - vehicleOrg;
			if ( vec.LengthSqr() < Square( 1200.0f ) ) { //mal: far enough away to ignore....
				idBox vehicleBBox = idBox( botVehicleInfo->bbox, botVehicleInfo->origin, botVehicleInfo->axis );
				vehicleBBox.ExpandSelf( NORMAL_BOX_EXPAND );

				if ( botThreadData.AllowDebugData() ) {
					gameRenderWorld->DebugBox( colorYellow, vehicleBBox );
					gameRenderWorld->DebugBox( colorBrown, bbox );
				}

				if ( bbox.IntersectsBox( vehicleBBox ) && InFrontOfVehicle( botInfo->proxyInfo.entNum, origin ) ) {
					if ( botAAS.obstacleNum == botWorld->botGoalInfo.botGoal_MCP_VehicleNum ) {
						proxyInfo_t mcpInfo;
						GetVehicleInfo( botAAS.obstacleNum, mcpInfo );

						if ( mcpInfo.entNum != 0 ) {
							if ( !mcpInfo.isImmobilized && mcpInfo.driverEntNum != -1 && !InFrontOfVehicle( mcpInfo.entNum, botVehicleInfo->origin ) ) {
								if ( vehiclePauseTime < botWorld->gameLocalInfo.time ) {
									vehiclePauseTime = botWorld->gameLocalInfo.time + 1500;
									return true;
								}
							} else {
								isBlocked = true;
							}
						}
					} else {
						isBlocked = true;
					}
				}
			}
		}
	} else if ( botVehicleInfo->type != MCP ) {
		float halfVehicleLength = botVehicleInfo->bbox[1][0];
		idVec3 vehicleOrg = botVehicleInfo->origin + ( botVehicleInfo->axis * botVehicleInfo->bbox.GetCenter() );
		
		vehicleOrg += ( halfVehicleLength * botVehicleInfo->axis[ 0 ] );
		trace_t	tr;
		idVec3 end = vehicleOrg;
		end += ( 128.0f * botVehicleInfo->axis[ 0 ] );

		botThreadData.clip->Translation( CLIP_DEBUG_PARMS tr, vehicleOrg, end, botThreadData.GetVehicleTestBounds( botVehicleInfo->type ), botVehicleInfo->axis, MASK_SHOT_RENDERMODEL | MASK_SHOT_BOUNDINGBOX | MASK_VEHICLESOLID, GetGameEntity( botVehicleInfo->entNum ) );

		if ( botThreadData.AllowDebugData() ) {
			gameRenderWorld->DebugBounds( colorWhite, botThreadData.GetVehicleTestBounds( botVehicleInfo->type )->GetBounds(), vehicleOrg, botVehicleInfo->axis );
		}

		if ( tr.fraction < 1.0f ) {
			isBlocked = true;

			if ( botThreadData.AllowDebugData() ) {
				if ( bot_debugGroundVehicles.GetInteger() == botNum ) {
					gameLocal.Printf("Bumper Blocked!\n");
				}
			}
		}
	}

	if ( botVehicleInfo->wheelsAreOnGround != 1.f && !botVehicleInfo->inWater ) { 
		vehicleWheelsAreOffGroundCounter++;
	} else {
		vehicleWheelsAreOffGroundCounter = 0;
	}

	if ( vehicleWheelsAreOffGroundCounter > 250 ) { //mal: bots stuck - get outta here!
		return false;
	}

	if ( ( botVehicleInfo->axis[ 2 ] * botAAS.aas->GetSettings()->invGravityDir ) < 0.80f ) { //mal: because of the physics, the vehicle could roll up a wall. Detect if thats happened, and backup if possible. 36 degrees == 0.80.
		isBlocked = true;
	}

	if ( vehicleWheelsAreOffGroundCounter > 30 ) {
		isBlocked = true;
	}

	if ( botVehicleInfo->type == PLATYPUS ) {
		isBlocked = false;
		moveIsClear = true;
	}
	
	if ( ( moveIsClear && !isBlocked ) && botIsBlockedTime < botWorld->gameLocalInfo.time ) {
		botIsBlocked = 0;
		botIsBlockedTime = 0;
	} else {
		botIsBlocked++;

		if ( botIsBlockedTime == 0 ) {
			botIsBlockedTime = botWorld->gameLocalInfo.time + 1200;
		}
	}

	if ( botIsBlocked < 30 ) { //mal: have a small grace period before we consider ourselves blocked. WAS 20
		return true;
	} else {
		if ( botVehicleInfo->type == GOLIATH || botVehicleInfo->type == TITAN || botVehicleInfo->type == DESECRATOR || botVehicleInfo->type == MCP ) {
			botUcmd->botCmds.isBlocked = true;
			return true;
		}

		//mal: its NOT clear to move forward, see how far back we can move
		v = vehicleOrg;
		v += ( -( vehicleLength ) * forwardVehicleAxis );

		if ( botThreadData.AllowDebugData() ) {
			if ( bot_debugGroundVehicles.GetInteger() == botNum ) {
				gameLocal.Printf("Blocked!\n");
				gameRenderWorld->DebugLine( colorLtBlue, vehicleOrg, v );
			}
		}
	
		int travelFlags = ( botInfo->team == GDF ) ? TFL_VALID_GDF : TFL_VALID_STROGG;
		int areaNum = botAAS.aas->PointReachableAreaNum( vehicleOrg, botAAS.aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
		aasTraceFloor_t tr;
		travelFlags &= ~TFL_WALKOFFLEDGE;
		botAAS.aas->TraceFloor( tr, vehicleOrg, areaNum, v, travelFlags );

		if ( moveDir == BACK || moveDir == FORWARD ) {
			goalOrigin = tr.endpos;
			botUcmd->specialMoveType = REVERSEMOVE;
			return true;
		} else {
			if ( tr.fraction >= 1.0f ) { //mal: its clear to move backward
				temp = v;

				if ( moveDir == LEFT ) {
					v += ( ( vehicleLength * 0.50f ) * ( rightVehicleAxis * -1 ) ); //mal: try the right of the vehicle.
				} else if ( moveDir == RIGHT ) {
					v += ( ( -vehicleLength * 0.50f ) * ( rightVehicleAxis * -1 ) ); //mal: try the left of the vehicle.
				}

				if ( botThreadData.AllowDebugData() ) {
					gameRenderWorld->DebugLine( colorGreen, temp, v );
				}

				moveIsClear = botThreadData.Nav_IsDirectPath( AAS_VEHICLE, botInfo->team, botInfo->areaNumVehicle, vehicleOrg, v );

				if ( moveIsClear ) {
					goalOrigin = v;
					botUcmd->specialMoveType = REVERSEMOVE;
					return true;
				}
			}
		}
	}

	v = vehicleOrg;
	v += ( -( vehicleLength ) * forwardVehicleAxis );
	goalOrigin = v;
	botUcmd->specialMoveType = REVERSEMOVE;
	botUcmd->botCmds.isBlocked = true;
	return false;
}


/*
================
idBotAI::Bot_CheckVehicleNodePath

When the bots in a vehicle just moving to a non-combat goal, they'll use the vehicle node system to find a nice looking path.
They'll still use the aas to get from node to node, but the nodes help define the roads, and the "drivable" areas better.
================
*/
bool idBotAI::Bot_CheckVehicleNodePath( const idVec3& goalOrigin, idVec3& pathPoint, bool& resetGoal ) {
	idVec3 botOrigin = botVehicleInfo->origin;
	idMat3 botAxis = botVehicleInfo->axis;
	resetGoal = false;

	// check for reaching the current node
	if ( botVehiclePathList.Num() > 0 ) {
		idBotNode::botLink_t* curLink = &botVehiclePathList[ botVehiclePathList.Num() - 1 ];
		float curDist = ( botOrigin - curLink->node->origin ).ToVec2().Length();

		if ( botVehiclePathList.Num() > 1 && InFrontOfVehicle( botInfo->proxyInfo.entNum, botVehiclePathList[ botVehiclePathList.Num() - 2 ].node->origin ) && Bot_DirectionToLocation( curLink->node->origin, true ) == BACK ) { //mal: if we passed our node, forget it and go to next.
			botVehiclePathList.SetNum( botVehiclePathList.Num() - 1 );
			curLink = &botVehiclePathList[ botVehiclePathList.Num() - 1 ];
			curDist = ( botOrigin - curLink->node->origin ).ToVec2().Length();
		} else if ( curDist < 500.0f ) {
			// check to see if we should start slowing down
			if ( botVehiclePathList.Num() > 1 ) {
				idVec3 vToNext;
				if ( botVehicleInfo->canRotateInPlace ) {
					vToNext = curLink->node->origin - botVehicleInfo->origin;
				} else {
					vToNext = botVehiclePathList[ botVehiclePathList.Num() - 2 ].node->origin - curLink->node->origin;
				}
				vToNext.Normalize();
				float angleCheck = vToNext * botAxis[ 0 ];
				if ( botVehicleInfo->canRotateInPlace && botAAS.obstacleNum == -1 ) {
					if ( angleCheck < 0.7f ) {
						botUcmd->botCmds.isBlocked = true;
					}
				} else {
					if ( angleCheck < 0.7f ) {
						botUcmd->specialMoveType = SLOWMOVE;
					}
				}
			}
		} else {
			if ( botVehicleInfo->canRotateInPlace && botAAS.obstacleNum == -1 ) {
				idVec3 vToNext;
				vToNext = curLink->node->origin - botVehicleInfo->origin;
				vToNext.Normalize();
				float angleCheck = vToNext * botAxis[ 0 ];
				if ( angleCheck < 0.7f ) {
					botUcmd->botCmds.isBlocked = true;
				}
			} else if ( botVehicleInfo->type == BADGER || botVehicleInfo->type == HOG ) {
				idVec3 vToNext;
				vToNext = curLink->node->origin - botVehicleInfo->origin;
				vToNext.Normalize();
				float angleCheck = vToNext * botAxis[ 0 ];
				if ( angleCheck < 0.7f ) {
					botUcmd->specialMoveType = SLOWMOVE;
				}
			}
		}

		float nodeRadius = ( botVehicleInfo->type == GOLIATH ) ? 150.0f : curLink->node->radius; 

		if ( curDist < nodeRadius ) {
			botVehiclePathList.SetNum( botVehiclePathList.Num() - 1 );
			nodeTimeOut = botWorld->gameLocalInfo.time + VEHICLE_NODE_TIMEOUT;
		} else {
			if ( nodeTimeOut < botWorld->gameLocalInfo.time ) {
				Bot_ExitVehicleAINode( true );
				pathPoint = botVehicleInfo->origin;
				vehicleReverseTime = botWorld->gameLocalInfo.time + 700; //mal: just in case we got stuck, back up a bit.
				return false;
			}
		}

		if ( botVehiclePathList.Num() == 0 ) {
			pathPoint = goalOrigin;
			return true;
		}

		pathPoint = botVehiclePathList[ botVehiclePathList.Num() - 1 ].node->origin;
		return false;
	}

	int nodeAttempts = 1;

	idBotNode* ignoreNode = ( pathNodeTimer.ignorePathNodeTime > botWorld->gameLocalInfo.time ) ? pathNodeTimer.ignorePathNode : NULL;
	
	// We don't have a path, try to create one
	idBotNode* ourNode = botThreadData.botVehicleNodes.GetNearestNode( botAxis, botOrigin, botInfo->team, FORWARD, true, true, true, ignoreNode, botVehicleInfo->flags );

	if ( ourNode == NULL ) {
		ourNode = botThreadData.botVehicleNodes.GetNearestNode( mat3_identity, botOrigin, botInfo->team, BACK, true, true, true, ignoreNode, botVehicleInfo->flags );
		nodeAttempts++;
	}

	if ( ourNode == NULL ) {
		ourNode = botThreadData.botVehicleNodes.GetNearestNode( mat3_identity, botOrigin, botInfo->team, RIGHT, true, true, true, ignoreNode, botVehicleInfo->flags );
		nodeAttempts++;
	}

	if ( ourNode == NULL ) {
		ourNode = botThreadData.botVehicleNodes.GetNearestNode( mat3_identity, botOrigin, botInfo->team, LEFT, true, true, true, ignoreNode, botVehicleInfo->flags );
		nodeAttempts++;
	}

	if ( ourNode == NULL ) {
		ourNode = botThreadData.botVehicleNodes.GetNearestNode( mat3_identity, botOrigin, botInfo->team, NULL_DIR, false, true, false, ignoreNode, botVehicleInfo->flags ); //try again, with even less restraints.
		nodeAttempts++;
	}

	if ( ourNode != NULL ) {
		pathNodeTimer.ignorePathNodeTime = IGNORE_PATH_NODE_TIME;
		pathNodeTimer.ignorePathNode = ourNode;
	}

	if ( botThreadData.AllowDebugData() ) {
		botThreadData.Printf("Bot Client %i tried %i times to find a close node\n", botNum, nodeAttempts );
	}

	idBotNode* goalNode = botThreadData.botVehicleNodes.GetNearestNode( mat3_identity, goalOrigin, botInfo->team, NULL_DIR, false, true, false, NULL, botVehicleInfo->flags ); // always assume the goal node is reachable

	if ( ourNode == NULL || goalNode == NULL ) {
		botThreadData.Warning( "Could not find path. NULL node" );
		pathPoint = goalOrigin;
		resetGoal = true;
		return false; //mal: just use the aas to find us a path.
	}

	if ( goalNode == ourNode ) {
		if ( botVehicleInfo->canRotateInPlace ) {
			idVec3 vToNext;
			vToNext = ourNode->origin - botVehicleInfo->origin;
			float distSqr = vToNext.LengthSqr();
			vToNext.Normalize();
			float angleCheck = vToNext * botAxis[ 0 ];
			if ( angleCheck < 0.7f && distSqr > Square( ourNode->radius ) ) {
				botUcmd->botCmds.isBlocked = true;
				pathPoint = goalOrigin;
				return false;
			}
		}

		pathPoint = goalOrigin;
		return true;
	} //mal: reached our goal.

	botThreadData.botVehicleNodes.CreateNodePath( botInfo, ourNode, goalNode, botVehiclePathList, botVehicleInfo->flags );

	if ( botVehiclePathList.Num() == 0 ) {
		if ( ourNode != NULL && goalNode != NULL ) {
			botThreadData.Warning( "Could not find path from %d to node %d", ourNode->num, goalNode->num );
		} else {
			botThreadData.Warning( "Could not find path. NULL node" );
		}
		pathPoint = goalOrigin;
		resetGoal = true;
		return false; //mal: just use the aas to find a way to reach our goal.
	} else {
		pathPoint = botVehiclePathList[ botVehiclePathList.Num() - 1 ].node->origin;
		nodeTimeOut = botWorld->gameLocalInfo.time + VEHICLE_NODE_TIMEOUT;
		newPathTime = botWorld->gameLocalInfo.time + 1000;
		botPathFailedCounter = 0;
	}


	if ( botThreadData.AllowDebugData() ) {
		idVec3 blah = goalOrigin;
		for ( int i = 0; i < botVehiclePathList.Num(); i++ ) {
			gameRenderWorld->DebugArrow( colorBlue, blah, botVehiclePathList[i].node->origin, 1024 );
			blah = botVehiclePathList[i].node->origin;
		}
		gameRenderWorld->DebugArrow( colorRed, botOrigin, botAAS.path.moveGoal, 1024 );
	}

	return false;
}

/*
==================
idBotAI::FindEntityByEntNum

With threading, there is no way for me to search thru the game's entity lists to find out what entity may be blocking the bot, 
so we'll search thru the entities we track internally.
==================
*/
bool idBotAI::FindEntityByEntNum( int entNum, idVec3& origin, idBox& bbox ) {
	bool foundEnt = false;

	if ( entNum < MAX_CLIENTS ) { //mal: that was easy - just loop thru the clients
		for( int i = 0; i < MAX_CLIENTS; i++ ) {
			if ( i != entNum ) {
				continue;
			}

			const clientInfo_t& clientInfo = botWorld->clientInfo[ i ];

			origin = clientInfo.origin;
			bbox = idBox( clientInfo.localBounds, clientInfo.origin, clientInfo.bodyAxis );
			foundEnt = true;
			break;
		}
	} else if ( entNum < MAX_GENTITIES ) {
		for( int i = 0; i < MAX_VEHICLES; i++ ) {
			const proxyInfo_t& vehicleInfo = botWorld->vehicleInfo[ i ];

			if ( vehicleInfo.entNum != entNum ) {
				continue;
			}

			origin = vehicleInfo.origin;
			bbox = idBox( vehicleInfo.bbox, vehicleInfo.origin, vehicleInfo.axis );
			foundEnt = true;
			break;
		}

		if ( !foundEnt ) {
			for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
				const deployableInfo_t& deployableInfo = botWorld->deployableInfo[ i ];

				if ( deployableInfo.entNum != entNum ) {
					continue;
				}

				origin = deployableInfo.origin;
				bbox = idBox( deployableInfo.bbox, deployableInfo.origin, deployableInfo.axis );
				foundEnt = true;
				break;
			}
		}

		if ( !foundEnt ) {
			for( int i = 0; i < MAX_CLIENTS; i++ ) {
				const clientInfo_t& clientInfo = botWorld->clientInfo[ i ];

				if ( clientInfo.supplyCrate.entNum != entNum ) {
					continue;
				}

				origin = clientInfo.supplyCrate.origin;
				bbox = idBox( clientInfo.supplyCrate.bbox, clientInfo.supplyCrate.origin, mat3_identity );
				foundEnt = true;
				break;
			}
		}
	} else {
		for( int i = 0; i < botThreadData.botObstacles.Num(); i++ ) {

			if ( botThreadData.botObstacles[ i ]->num != entNum ) {
				continue;
			}

			origin = botThreadData.botObstacles[ i ]->bbox.GetCenter();
			bbox = botThreadData.botObstacles[ i ]->bbox;
			foundEnt = true;
			break;
		}
	}

	if ( !foundEnt ) {
		botThreadData.Warning( "Bot %i can't find entity %i in \"FindEntityByEntNum\"!!", botNum, entNum ); //mal: argh!
		return false;
	}

	return true;
}

/*
==================
idBotAI::Bot_IsInHeavyAttackVehicle

Some vehicles ( tanks or attack aircraft ) should never be left unless the bot is critical to the mission.
==================
*/
bool idBotAI::Bot_IsInHeavyAttackVehicle( bool ignoreGroundVehicles ) {

	if ( botVehicleInfo == NULL ) {
		return false;
	}

	if ( botVehicleInfo->type == MCP ) {
		return false;
	}

	if ( ignoreGroundVehicles == true ) {
		if ( !( botVehicleInfo->flags & AIR ) ) {
			return false;
		}
	} else {
		if ( !( botVehicleInfo->flags & ARMOR ) && !( botVehicleInfo->flags & AIR ) || ( botVehicleInfo->flags & PERSONAL ) || botVehicleInfo->type == BUFFALO ) {
			return false;
		}
	}

	return true;
}

/*
================
idBotAI::Bot_SetupIcarusMove

Sets up the bot's path goal when in an icarus.
================
*/
void idBotAI::Bot_SetupIcarusMove( const idVec3 &org, int clientNum, int actionNumber ) {
	int areaNum;
	idVec3 goalOrigin;

	if ( botAAS.aas == NULL ) {
		return;
	}

	idBounds bbox = botAAS.aas->GetSettings()->boundingBox;
	idObstacleAvoidance::obstaclePath_t path;

	bool canBoost = false;

	if ( clientNum != -1 ) {
		areaNum = botWorld->clientInfo[ clientNum ].areaNumVehicle;
		goalOrigin = botWorld->clientInfo[ clientNum ].aasVehicleOrigin;		
	} else if ( actionNumber != -1 ) {
		areaNum = botThreadData.botActions[ actionNumber ]->areaNumVehicle;
		goalOrigin = botThreadData.botActions[ actionNumber ]->origin;
	} else {
		areaNum = botAAS.aas->PointReachableAreaNum( org, bbox, AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
		goalOrigin = org;
	}

	if ( botInfo->hasGroundContact ) { //mal: when on the ground, the icarus is just like a player movement wise.
		isBoosting = false;

		int travelFlags, walkTravelFlags;
		if ( botInfo->team == GDF ) {
			travelFlags = TFL_VALID_GDF;
			walkTravelFlags = TFL_VALID_WALK_GDF;
		} else {
			travelFlags = TFL_VALID_STROGG;
			walkTravelFlags = TFL_VALID_WALK_STROGG;
		}

		if ( botInfo->areaNumVehicle != areaNum ) {
			botAAS.aas->PushPointIntoArea( areaNum, goalOrigin );
		}

		botAAS.hasPath = botAAS.aas->WalkPathToGoal( botAAS.path, botInfo->areaNumVehicle, botInfo->aasVehicleOrigin, areaNum, goalOrigin, travelFlags, walkTravelFlags );

		if ( botInfo->proxyInfo.boostCharge > 0.90f ) {
			if ( botAAS.aas->ExtendHopPathToGoal( botAAS.path, botInfo->areaNumVehicle, botInfo->aasVehicleOrigin, areaNum, goalOrigin, travelFlags, walkTravelFlags, idAASHopPathParms() ) ) {
				canBoost = true;
			}

			if ( botThreadData.AllowDebugData() ) {
				if ( bot_showPath.GetInteger() == botNum ) {
					botAAS.aas->ShowHopPath( botInfo->areaNumVehicle, botInfo->aasVehicleOrigin, areaNum, goalOrigin, travelFlags, walkTravelFlags, idAASHopPathParms() );
					gameRenderWorld->DebugCircle( colorGreen, goalOrigin, idVec3( 0, 0, 1 ), 32, 32 );
				}
			}
		}
			
		botAAS.hasClearPath = obstacles.FindPathAroundObstacles( botInfo->localBounds, botAAS.aas->GetSettings()->obstaclePVSRadius, botAAS.aas, botInfo->aasVehicleOrigin, botAAS.path.moveGoal, path );

		if ( !botAAS.hasClearPath ) {
			canBoost = true; //mal: try to avoid obstacles by flying around them.
		}

		botAAS.obstacleNum = path.firstObstacle;

		if ( botAAS.obstacleNum != -1 ) {
			idVec3 origin;
			idBox bbox;
			bool foundEnt = FindEntityByEntNum( botAAS.obstacleNum, origin, bbox );

			if ( foundEnt ) {
				idVec3 vec = origin - botVehicleInfo->origin;
				if ( vec.LengthSqr() < Square( 500.0f ) ) { //mal: far enough away to ignore....
					idBox vehicleBBox = idBox( botVehicleInfo->bbox, botVehicleInfo->origin, botVehicleInfo->axis );
					vehicleBBox.ExpandSelf( NORMAL_BOX_EXPAND );

					if ( botThreadData.AllowDebugData() ) {
						gameRenderWorld->DebugBox( colorYellow, vehicleBBox );
						gameRenderWorld->DebugBox( colorBrown, bbox );
					}

					if ( bbox.IntersectsBox( vehicleBBox ) && InFrontOfVehicle( botInfo->proxyInfo.entNum, origin ) ) {
						canBoost = true;
					}
				}
			}
		}

		if ( canBoost ) {
			botUcmd->specialMoveType = ICARUS_BOOST;
			hopMoveGoal = botAAS.path.moveGoal;
			isBoosting = true;
		}

		botAAS.path.moveGoal = path.seekPos;

		if ( path.firstObstacle != -1 ) {
			botAAS.path.type = PATHTYPE_WALK;
		}
	} else if ( isBoosting ) { //mal: we're airborne, so now we handle like an air vehicle....
		const int MAX_POINTS = 256;
		float height = -idMath::INFINITY;
		idVec3 pointsList[ MAX_POINTS ];

		idVec3 end = botInfo->viewOrigin;
		end.z += 64.0f;
		end += ( 2048.0f * botVehicleInfo->axis[ 0 ] );

		botAAS.hasPath = true;
		botAAS.hasReachedVehicleNodeGoal = false;

		aasTraceHeight_t traceHeight;

		traceHeight.maxPoints = MAX_POINTS;
		traceHeight.numPoints = 0;
		traceHeight.points = pointsList;

		botAAS.aas->TraceHeight( traceHeight, botInfo->viewOrigin, end );

		for( int i = 0; i < traceHeight.numPoints; i++ ) {
			if ( traceHeight.points[ i ].z > height ) {
				height = traceHeight.points[ i ].z;
				if ( height >= botInfo->origin.z + idAASHopPathParms().maxHeight ) {
					botAAS.hasPath = false;
				}
			}
		}

		idVec3 vec = hopMoveGoal - botInfo->origin;
		vec.z = 0.0f;
		float goalDistSqr = vec.LengthSqr();

		if ( height < botInfo->origin.z + idAASHopPathParms().maxHeight && botInfo->proxyInfo.boostCharge > 0.0f && goalDistSqr > Square( 1024.0f ) && botAAS.hasPath ) {
			botUcmd->specialMoveType = ICARUS_BOOST;
		} else {
			isBoosting = false;
		}

		botAAS.path.moveGoal = hopMoveGoal;

		botAAS.path.moveGoal.z += height;

		idObstacleAvoidance::obstaclePath_t path;

		botAAS.hasClearPath = obstacles.FindPathAroundObstacles( botVehicleInfo->bbox, botAAS.aas->GetSettings()->obstaclePVSRadius, botAAS.aas, botVehicleInfo->origin, botAAS.path.moveGoal, path );

		botAAS.path.moveGoal = path.seekPos;

		botAAS.obstacleNum = path.firstObstacle;

		if ( botAAS.obstacleNum != -1 && botInfo->proxyInfo.boostCharge > 0.0f ) { //mal: fly over obstacles?
			botUcmd->specialMoveType = ICARUS_BOOST;
			isBoosting = true;
		}

		botAAS.path.viewGoal = botAAS.path.moveGoal;
		idVec3 viewDelta = botAAS.path.moveGoal - botInfo->viewOrigin;
		if ( idMath::Fabs( viewDelta.z ) < 100.0f ) {
			botAAS.path.viewGoal.z = botInfo->viewOrigin.z;
		}
	}

	if ( !botWorld->gameLocalInfo.inWarmup && botThreadData.random.RandomInt( 100 ) > 98 ) {
        if ( path.firstObstacle > -1 && path.firstObstacle < MAX_CLIENTS ) {
			if ( botWorld->clientInfo[ path.firstObstacle ].team == botInfo->team && !botWorld->clientInfo[ path.firstObstacle ].isBot ) {

//mal: ONLY say move if both the bot and the client in question are not just spawning in!
				if ( botWorld->clientInfo[ path.firstObstacle ].invulnerableEndTime < botWorld->gameLocalInfo.time && botInfo->invulnerableEndTime < botWorld->gameLocalInfo.time ) {
                    idVec3 tempOrigin = botWorld->clientInfo[ path.firstObstacle ].origin - botInfo->origin;

					if ( tempOrigin.LengthSqr() < Square( 50.0f ) ) {
                        botUcmd->desiredChat = MOVE;
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
==================
idBotAI::Bot_VehicleIsUnderAVTAttack

Checks to see if an AVT out there in the world is targeting us, in which case we'll attack it.
==================
*/
int idBotAI::Bot_VehicleIsUnderAVTAttack() {
	if ( botVehicleInfo == NULL ) {
		assert( false );
		return false;
	}

	int deployableEnemyNum = -1;

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {

		const deployableInfo_t& deployable = botWorld->deployableInfo[ i ];

		if ( deployable.entNum == 0 ) {
			continue;
		}

		if ( deployable.health == 0 && deployable.maxHealth == 0 ) {
			continue;
		}
	
		if ( !deployable.inPlace ) {
			continue;
		}

		if ( deployable.team == botInfo->team ) {
			continue;
		}

		if ( deployable.type != AVT ) {
			continue;
		}

		if ( deployable.enemyEntNum != botNum && deployable.enemyEntNum != botVehicleInfo->entNum ) {
			continue;
		}

		idVec3 dangerOrigin = deployable.origin; //mal: they can lock onto us before they can really see us, which causes some issues for the bots.
		dangerOrigin.z += ( deployable.bbox[ 1 ][ 2 ] - deployable.bbox[ 0 ][ 2 ] ) * 0.95f;

		trace_t tr;
		botThreadData.clip->TracePointExt( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, dangerOrigin, MASK_SHOT_BOUNDINGBOX | MASK_VEHICLESOLID | CONTENTS_FORCEFIELD, GetGameEntity( botNum ), GetGameEntity( botVehicleInfo->entNum ) ); 

		if ( tr.fraction < 1.0f && tr.c.entityNum != deployable.entNum ) {
			continue;
		}

		deployableEnemyNum = deployable.entNum;
		break;
	}

	return deployableEnemyNum;
}

/*
==================
idBotAI::Bot_VehicleLTGIsAvailable
==================
*/
bool idBotAI::Bot_VehicleLTGIsAvailable( int clientNum, int actionNumber, const bot_Vehicle_LTG_Types_t goalType, int minNumClients ) {
	int numClients = 0;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) { //mal: dont scan ourselves.
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( botThreadData.bots[ i ] == NULL ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		if ( playerInfo.team != botInfo->team ) {
			continue;
		}

		if ( playerInfo.isBot != true ) {
			continue;
		}

		if ( botThreadData.bots[ i ] == NULL ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetAIState() != VLTG ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetVehicleLTGType() != goalType ) {
			continue;
		}

		if ( clientNum != -1 ) {
			if ( botThreadData.bots[ i ]->GetVehicleLTGTarget() != clientNum ) {
				continue;
			}
		}

		if ( actionNumber != ACTION_NULL ) {
			if ( botThreadData.bots[ i ]->GetActionNum() != actionNumber ) {
                continue;
			}
		}

		numClients++;

		if ( minNumClients == 1 ) {
            break;
		}
	}

	if ( numClients >= minNumClients ) {
		return false;
	} else {
		return true;
	}
}

/*
==================
idBotAI::Bot_CheckHumanRequestingTransport
==================
*/
bool idBotAI::Bot_CheckHumanRequestingTransport() {
	if ( botVehicleInfo->type >= ICARUS && !botVehicleInfo->hasGroundContact ) {
		return false;
	}

	if ( botVehicleInfo->flags & PERSONAL ) {
		return false;
	}

	if ( !botVehicleInfo->hasFreeSeat ) {
		return false;
	}

	if ( vehicleSurrenderTime > botWorld->gameLocalInfo.time ) { //mal: already have someone we're waiting on.
		botMoveTypes_t moveType = ( botVehicleInfo->type > ICARUS ) ? LAND : FULL_STOP;
		Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, moveType );
		Bot_LookAtEntity( vehicleSurrenderClient, SMOOTH_TURN );
		if ( !vehicleSurrenderChatSent ) {
			Bot_AddDelayedChat( botNum, NEED_LIFT, 1 );
			vehicleSurrenderChatSent = true;
			botUcmd->botCmds.honkHorn = true;
		}

		return true;
	}

	bool hasEscortRequest = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		if ( i == botNum ) {
			continue;
		}

		if ( ClientIsIgnored( i ) ) {
			continue;
		}

		const clientInfo_t &player = botWorld->clientInfo[ i ];

		if ( player.isBot ) {
			continue;
		}

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( player.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			continue;
		}

		if ( player.pickupRequestTime + 5000 < botWorld->gameLocalInfo.time ) {
			continue;
		}

		idVec3 vec = player.origin - botInfo->origin;
		float ourDistSqr = vec.LengthSqr();

		if ( player.pickupTargetSpawnID != -1 ) {
			if ( player.pickupTargetSpawnID != botVehicleInfo->spawnID ) {
				continue;
			}
		} else {
			for( int j = 0; j < MAX_CLIENTS; j++ ) { //mal: check to make sure the bot is the closest vehicle to the player.
				if ( !ClientIsValid( j, -1 ) ) {
					continue; 	 
				}

				if ( j == botNum ) {
					continue;
				}
				
				const clientInfo_t& bot = botWorld->clientInfo[ j ]; 

				if ( !bot.isBot ) {
					continue;
				}
				
				if ( bot.team != botInfo->team ) {
					continue;
				}

				if ( bot.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {
					continue;
				}

				vec = player.origin - bot.origin; 	 
                float distSqr = vec.LengthSqr();

				if ( distSqr > Square( MAX_RIDE_DIST ) ) {
					continue;
				}

				proxyInfo_t vehicle;
				GetVehicleInfo( bot.proxyInfo.entNum, vehicle );

				if ( vehicle.driverEntNum != j ) {
					continue;
				}

				if ( !vehicle.hasFreeSeat || ( vehicle.type >= ICARUS && !vehicle.hasGroundContact ) || vehicle.flags & PERSONAL ) {
					continue;
				}

				if ( distSqr < ourDistSqr ) { //mal: someones closer to this guy then us, so just let him give the player a ride
					break;
				}
			}
		}

		if ( ourDistSqr > Square( MAX_RIDE_DIST ) ) {
			Bot_AddDelayedChat( botNum, CMD_DECLINED, 1 );
			Bot_IgnoreClient( i, REQUEST_CONSIDER_TIME );
			continue;
		}

		vehicleSurrenderTime = botWorld->gameLocalInfo.time + 10000;
		vehicleSurrenderChatSent = false;
		vehicleSurrenderClient = i;
		hasEscortRequest = true;
		break;
	}

	return hasEscortRequest;
}

/*
==================
idBotAI::Bot_CheckIfClientHasRideWaiting
==================
*/
bool idBotAI::Bot_CheckIfClientHasRideWaiting( int clientNum ) {
	bool hasRide = false;
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		if ( i == botNum ) {
			continue;
		}

		const clientInfo_t &player = botWorld->clientInfo[ i ];

		if ( !player.isBot ) {
			continue;
		}

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( botThreadData.bots[ i ] == NULL ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetVehicleSurrenderTime() < botWorld->gameLocalInfo.time ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetVehicleSurrenderClient() != clientNum ) {
			continue;
		}

		hasRide = true;
		break;
	}

	return hasRide;
}

/*
==================
idBotAI::VehiclesInArea
==================
*/
int idBotAI::VehiclesInArea( int ignoreClientNum, const idVec3 &org, float range, int team, bool vis2Sky, int ignoreVehicleNum, bool humanOnly ) {
	int clients = 0;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

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

		if ( humanOnly ) {
			if ( playerInfo.isBot ) {
				continue;
			}
		}

		if ( playerInfo.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {
			continue;
		}

		if ( playerInfo.proxyInfo.entNum == ignoreVehicleNum ) {
			continue;
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

		idVec3 vec = playerInfo.origin - org;

		if ( vec.LengthSqr() > Square( range ) ) {
			continue;
		}

		clients++;
	}

	return clients;

}

/*
==================
idBotAI::IcarusCombatMove
==================
*/
void idBotAI::IcarusCombatMove() {
	float evadeDist = 550.0f;
	const clientInfo_t& playerInfo = botWorld->clientInfo[ enemy ];
	idVec3 vec = playerInfo.origin - botInfo->origin;
	vec.z = 0.0f;
	float distToEnemySqr = vec.LengthSqr();

	if ( AIStack.stackActionNum != ACTION_NULL && AIStack.isPriority ) {
		idVec3 vec = botThreadData.botActions[ AIStack.stackActionNum ]->origin - botInfo->origin;
		vec.z = 0.0f;

		if ( vec.LengthSqr() > Square( evadeDist ) ) {
			Bot_SetupVehicleMove( vec3_zero, -1, AIStack.stackActionNum );

			if ( MoveIsInvalid() && botVehicleInfo->hasGroundContact ) { //mal: move is failing, jump out when touch down, and fight normally.
				Bot_ExitVehicle();
				return;
			}

			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		} else {
			if ( botVehicleInfo->hasGroundContact ) { //mal: reached our priority goal, now jump out so we can complete it ( hopefully ).
				Bot_ExitVehicle();
				return;
			}
		}
	} else {
		moveDirections_t dirToTarget = Bot_DirectionToLocation( playerInfo.origin, false );
		idVec3 goalOrigin = playerInfo.origin;

		if ( distToEnemySqr < Square( 2500.0f ) ) {
			if ( dirToTarget == FORWARD || dirToTarget == LEFT ) { //mal: move to the right of our enemy
				goalOrigin += ( 1500.0f * playerInfo.viewAxis[ 1 ] * -1 );
			} else {
				goalOrigin += ( -1500.0f * playerInfo.viewAxis[ 1 ] * -1 ); //mal: move to the left of our enemy
			}
		}

		Bot_SetupVehicleMove( goalOrigin, -1, ACTION_NULL );

		if ( MoveIsInvalid() && botVehicleInfo->hasGroundContact ) { //mal: move is failing, jump out when touch down, and fight normally.
			Bot_ExitVehicle();
			return;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
		Bot_LookAtLocation( playerInfo.origin, SMOOTH_TURN );
	}

	if ( distToEnemySqr < Square( 1500.0f ) && InFrontOfVehicle( botVehicleInfo->entNum, playerInfo.origin ) && !botVehicleInfo->hasGroundContact ) {
		botUcmd->botCmds.attack = true;
	}
}

/*
================
idBotAI::HumanVehicleOwnerNearby
================
*/
bool idBotAI::HumanVehicleOwnerNearby( const playerTeamTypes_t playerTeam, const idVec3 &loc, float range, int vehicleSpawnID ) {
	bool humanOwner = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

        if ( i == botNum ) {
			continue;
		}			
			
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.team != playerTeam ) {
			continue;
		}

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: this player is already in a vehicle, dont worry about him
			continue;
		}

		if ( playerInfo.lastOwnedVehicleSpawnID != vehicleSpawnID ) { //mal: the player was never in this vehicle, so dont worry about him
			continue;
		}

		if ( playerInfo.isBot ) {
			continue;
		}

		if ( playerInfo.lastOwnedVehicleTime + MAX_OWN_VEHICLE_TIME < botWorld->gameLocalInfo.time ) { //mal: after a certain amount of time, we don't care anymore.
			continue;
		}

		if ( playerInfo.xySpeed >= RUNNING_SPEED && !InFrontOfClient( i, loc ) ) { //mal: player is running away from this vehicle, so don't worry about him.
			continue;
		}

		idVec3 vec = playerInfo.origin - loc;

		if ( vec.LengthSqr() > Square( range ) ) {
			continue;
		}

		humanOwner = true;
		break;
	}

	return humanOwner;
}

/*
================
idBotAI::Bot_CheckFriendlyEngineerIsNearbyOurVehicle
================
*/
bool idBotAI::Bot_CheckFriendlyEngineerIsNearbyOurVehicle() {
	bool friendNearby = false;

	if ( botVehicleInfo == false ) {
		return false;
	}

	if ( botVehicleInfo->health == botVehicleInfo->maxHealth ) {
		return false;
	}

	if ( enemy != -1 ) {
		return false;
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) { 
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( player.health <= 0 ) {
			continue;
		}

		if ( player.classType != ENGINEER ) {
			continue;
		}

		if ( player.isBot ) {

			if ( botThreadData.bots[ i ] == NULL ) {
				continue;
			}


			if ( botThreadData.bots[ i ]->GetNBGType() != FIX_VEHICLE || botThreadData.bots[ i ]->GetAIState() != NBG ) {
				continue;
			}

			if ( botThreadData.bots[ i ]->GetNBGTarget() != botVehicleInfo->entNum ) {
				continue;
			}

			friendNearby = true;
			break;
		} else {

			if ( player.weapInfo.weapon != PLIERS ) {
				continue;
			}

			if ( !InFrontOfClient( i, botVehicleInfo->origin ) ) {
				continue;
			}

			idVec3 vec = botVehicleInfo->origin - player.origin;

			if ( vec.LengthSqr() > Square( MAX_REPAIR_DIST ) ) {
				continue;
			}

			friendNearby = true;
			break;
		}
	}

	return friendNearby;
}

/*
================
idBotAI::Bot_CheckForHumanNearByWhoMayWantRide
================
*/
bool idBotAI::Bot_CheckForHumanNearByWhoMayWantRide() {
	bool friendNearby = false;

	if ( enemy != -1 ) {
		return false;
	}

	if ( botVehicleInfo == NULL ) {
		return false;
	}

	if ( botVehicleInfo->type == ICARUS && !botVehicleInfo->hasGroundContact ) {
		return false;
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) { 
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( player.health <= 0 ) {
			continue;
		}

		if ( player.isBot ) {
			continue;
		}

		if ( player.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			continue;
		}

		if ( player.xySpeed < RUNNING_SPEED ) {
			continue;
		}

		if ( !InFrontOfClient( i, botVehicleInfo->origin, true, 0.95f ) ) {
			continue;
		}

		idVec3 vec = botVehicleInfo->origin - player.origin;

		if ( vec.LengthSqr() > Square( MAX_CONSIDER_HUMAN_FOR_RIDE_DIST ) ) {
			continue;
		}

		friendNearby = true;
		break;
	}

	return friendNearby;
}

/*
================
idBotAI::Bot_GetVehicleGunnerClientNum
================
*/
int idBotAI::Bot_GetVehicleGunnerClientNum( int vehicleEntNum ) {
	int clientNum = -1;
	proxyInfo_t vehicleInfo;
	botVehicleWeaponInfo_t weaponType = MINIGUN;
	GetVehicleInfo( vehicleEntNum, vehicleInfo );
	
	if ( vehicleInfo.type == TROJAN ) {
		weaponType = LAW;
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t &playerInfo = botWorld->clientInfo[ i ];
	
		if ( playerInfo.proxyInfo.entNum != vehicleEntNum ) {
			continue;
		}

		if ( playerInfo.proxyInfo.weapon != weaponType ) {
			continue;
		}

		clientNum = i;
		break;
	}

	return clientNum;
}

/*
================
idBotAI::VehicleGoalsExistForVehicle
================
*/
bool idBotAI::VehicleGoalsExistForVehicle( const proxyInfo_t& vehicleInfo ) {
	bool goalExists = false;

	for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( vehicleInfo.team ) != ACTION_VEHICLE_ROAM && botThreadData.botActions[ i ]->GetObjForTeam( vehicleInfo.team ) != ACTION_VEHICLE_CAMP ) {
			continue;
		}

		if ( !( botThreadData.botActions[ i ]->GetActionVehicleFlags( vehicleInfo.team ) & vehicleInfo.flags ) && botThreadData.botActions[ i ]->GetActionVehicleFlags( vehicleInfo.team ) != 0  ) {
			continue;
		}

		goalExists = true;
		break;
	}

	return goalExists;
}

/*
================
idBotAI::Bot_WithObjShouldLeaveVehicle
================
*/
bool idBotAI::Bot_WithObjShouldLeaveVehicle() {
	float vehicleExitSpeed = ( botVehicleInfo->isAirborneVehicle ) ? 1024.0f : WALKING_SPEED;

	if ( botVehicleInfo->type == BUFFALO ) {
		vehicleExitSpeed = BASE_VEHICLE_SPEED;
	}

	if ( ClientHasObj( botNum ) && botVehicleInfo->xyspeed < vehicleExitSpeed && ClientIsCloseToDeliverObj( botNum, 3000.0f ) ) { //mal: if the bot has the obj - they should leave if the player stops the vehicle near the obj.
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_CheckForDeployableTargetsWhileVehicleGunner
================
*/
int idBotAI::Bot_CheckForDeployableTargetsWhileVehicleGunner() {
	if ( botInfo->proxyInfo.weapon != MINIGUN && botInfo->proxyInfo.weapon != LAW ) {
		return -1;
	}

	if ( botVehicleInfo == NULL ) {
		return -1;
	}

	bool hasMCP = ( botWorld->botGoalInfo.botGoal_MCP_VehicleNum != -1 && botWorld->botGoalInfo.mapHasMCPGoal ) ? true : false;

	int targetNum = -1;
	float closest = idMath::INFINITY;

	bool botActionValid = false;
	int botActionNum;
	idVec3 botActionOrg;

	if ( botInfo->team == GDF ) {
		botActionNum = botWorld->botGoalInfo.team_GDF_PrimaryAction;
	} else {
		botActionNum = botWorld->botGoalInfo.team_STROGG_PrimaryAction;
	}

	if ( botActionNum > -1 && botActionNum < botThreadData.botActions.Num() ) {
		botActionValid = true;
		botActionOrg = botThreadData.botActions[ botActionNum ]->GetActionOrigin();
	}

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
		const deployableInfo_t& deployable = botWorld->deployableInfo[ i ];

		if ( deployable.entNum == 0 ) {
			continue;
		}

		if ( deployable.team == botInfo->team ) {
			continue;
		}

		if ( DeployableIsIgnored( deployable.entNum ) ) {
			continue;
		}

		if ( deployable.health <= 0 ) {
			continue;
		}

		if ( deployable.ownerClientNum == -1 ) { //mal: dont attack static, map based deployables.
			continue;
		}

		if ( deployable.health < ( deployable.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) {
			continue;
		}

		if ( !deployable.inPlace ) {
			continue;
		}

		if ( botWorld->gameLocalInfo.gameMap == SLIPGATE ) { //mal: need some special love for this unique map.
			if ( !Bot_CheckLocationIsOnSameSideOfSlipGate( deployable.origin ) ) {
				continue;
			}
		}

		bool isTargetingUs = ( deployable.enemyEntNum == botNum || ( botVehicleInfo != NULL && deployable.enemyEntNum == botVehicleInfo->entNum ) ) ? true : false;

		idVec3 vec = deployable.origin - botInfo->origin;
		float distSqr = vec.LengthSqr();

		if ( distSqr > Square( GUNNER_ATTACK_DEPLOYABLE_DIST ) ) {
			continue;
		}

//mal: set some kind of priority depending on the deployable type.
		if ( deployable.type == AVT ) {
			if ( isTargetingUs ) {
				distSqr = 1.0f;
			} else if ( hasMCP ) {
				if ( deployable.enemyEntNum == botWorld->botGoalInfo.botGoal_MCP_VehicleNum ) {
					distSqr = 5.0f;
				}
			} else if ( botActionValid ) { 
				vec = botActionOrg - deployable.origin;
				if ( vec.LengthSqr() < Square( BOT_ATTACK_DEPLOYABLE_RANGE ) ) {
					distSqr -= Square( 1000.0f );
				}
			}
		} else if ( deployable.type == APT ) { //mal: focus more on APTs that are guarding the obj we're trying to attack.
			if ( botActionValid ) { 
				vec = botActionOrg - deployable.origin;
				if ( vec.LengthSqr() < Square( BOT_ATTACK_DEPLOYABLE_RANGE ) ) {
					distSqr -= Square( 1000.0f );
				}
			}
		}

		if ( distSqr < closest ) {
			vec = deployable.origin;
			vec.z += ( deployable.bbox[ 1 ][ 2 ] - deployable.bbox[ 0 ][ 2 ] ) * Bot_GetDeployableOffSet( deployable.type );

			if ( !Bot_CheckLocationIsVisible( vec, deployable.entNum, botVehicleInfo->entNum ) ) {
				continue;
			}
			targetNum = deployable.entNum;
			closest = distSqr;
		}
	}

	return targetNum;
}

/*
================
idBotAI::Bot_AttackDeployableTargetsWhileVehicleGunner
================
*/
void idBotAI::Bot_AttackDeployableTargetsWhileVehicleGunner( int deployableTargetToKill ) {
	if ( deployableTargetToKill != vLTGDeployableTarget ) {
		vLTGDeployableTargetAttackTime = botWorld->gameLocalInfo.time;
		vLTGDeployableTarget = deployableTargetToKill;
	}

	deployableInfo_t deployable;

	if ( GetDeployableInfo( false, deployableTargetToKill, deployable ) ) {
		if ( vLTGDeployableTargetAttackTime + 5000 < botWorld->gameLocalInfo.time && ( botInfo->lastAttackedEntity != deployable.entNum || botInfo->lastAttackedEntityTime + 5000 < botWorld->gameLocalInfo.time ) ) {
			Bot_IgnoreDeployable( deployableTargetToKill, 1000 ); //mal: update often, incase player moves into a better position.
			return;
		}

		idVec3 deployableTargetOrg = deployable.origin;
		deployableTargetOrg.z += ( deployable.bbox[ 1 ][ 2 ] - deployable.bbox[ 0 ][ 2 ] ) * Bot_GetDeployableOffSet( deployable.type );

		Bot_LookAtLocation( deployableTargetOrg, SMOOTH_TURN, true );

		if ( InFrontOfClient( botNum, deployableTargetOrg, true, 0.95f ) ) {
			botUcmd->botCmds.attack = true;
		}
	}
}

/*
================
idBotAI::Bot_WhoIsCriticalForCurrentObjShouldLeaveVehicle
================
*/
bool idBotAI::Bot_WhoIsCriticalForCurrentObjShouldLeaveVehicle() {
	float vehicleExitSpeed = ( botVehicleInfo->isAirborneVehicle ) ? 1024.0f : WALKING_SPEED;

	if ( botVehicleInfo->type == BUFFALO ) {
		vehicleExitSpeed = BASE_VEHICLE_SPEED;
	}

	if ( Client_IsCriticalForCurrentObj( botNum, 3000.0f ) && botVehicleInfo->xyspeed < vehicleExitSpeed ) { //mal: if the bot is critical for the obj - they should leave if the player stops the vehicle near the obj.
		return true;
	}

	return false;
}





