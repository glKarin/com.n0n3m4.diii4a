// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "../../game/ContentMask.h"
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idBotAI::Bot_MedicCheckForWoundedMate

Checks for wounded teammates. Works with both gdf and strogg (since the 2 are so similiar).
If the bot is already in the process of healing/reviving someone, it will pass the clients number in "healClientNum",
in which case, the bot will only look at ones that are closer then their current NBG goal.
"range" is how far bot will look to heal. This will change based on what bot is
doing (if escorting important mate, or carrying OBJ, will not go as far to heal).
================
*/
int idBotAI::Bot_MedicCheckForWoundedMate( int healClientNum, int escortClientNum, float range, int mateHealth ) {
	int i;
	int clientNum = -1;
	int busyClient;
	int matesInArea;
	float closest = idMath::INFINITY; //mal: set it to some large, crazy number
	float dist;
	idVec3	vec;
	
//mal: dont bother doing this if bot has no charge
	if ( !ClassWeaponCharged( HEALTH ) ) {
		botIdealWeapSlot = GUN;
		return - 1;
	}

	if ( botInfo->inWater ) {
		return -1;
	}

	if ( healClientNum != -1 ) {
		vec = botWorld->clientInfo[ healClientNum ].origin - botInfo->origin;
		closest = vec.LengthFast();
		clientNum = healClientNum;
	} //mal: if we already have someone we're trying to heal, only heal someone else if they're closer!

	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) {
			continue; //mal: dont try to heal ourselves!
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( ClientIsIgnored( i ) ) {
			continue;
		}

//mal: some bot has this client tagging for healing.
//mal_NOTE: in this case, we dont care if a human medic is on route or nearby, just because experience shows most humans SUCK at being medics! ;-)
		if ( !Bot_NBGIsAvailable( i, ACTION_NULL, SUPPLY_TEAMMATE, busyClient ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.inLimbo ) {
			continue;
		}

		if ( botInfo->isActor && botThreadData.actorMissionInfo.targetClientNum != i ) {
			continue;
		}

		if ( playerInfo.isNoTarget ) {
			continue;
		}

		if ( playerInfo.classType == MEDIC ) {
			continue; 
		} //mal: if player is a medic, ignore, they can heal themselves!

		if ( playerInfo.team != botInfo->team && playerInfo.isDisguised == false ) {
			continue; //mal: give no comfort to the enemy! can be tricked by disguised enemy.
		}

		if ( playerInfo.health == playerInfo.maxHealth ) {
			continue;
		}

		if ( playerInfo.health >= 100 && playerInfo.lastChatTime[ HEAL_ME ] + 5000 < botWorld->gameLocalInfo.time ) { //mal: conserver our resources if your in good shape, unless you ask for it.
			continue;
		}

		if ( healClientNum != -1 && playerInfo.xySpeed > WALKING_SPEED ) { 
			continue;
		} //mal: ignore ppl moving around a lot, if we're already on way to someone else!

		if ( playerInfo.inWater ) {
			continue; //mal: can't heal ppl who are in water!
		}

		if ( playerInfo.health >= mateHealth && i != escortClientNum ) {
			continue;
		}

		if ( playerInfo.health <= 0 ) { //mal: dont bother if client is dead. we look for dead mates elsewhere
			continue;
		}

		if ( playerInfo.inLimbo ) {
			continue; //mal: dont bother if client already tapped/gibbed
		}

		if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
			if ( Bot_CheckForHumanInteractingWithEntity( i ) == true ) {
				continue;
			}
		}

		if ( !playerInfo.hasGroundContact && !playerInfo.hasJumped ) {
			continue;
		} //mal: ignore ppl flying thru the air (from explosion, dropping from airplane, etc).

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			continue; //mal: ignore players in a vehicle/deployable.
		}

		if ( playerInfo.areaNum == 0 ) {
			continue;
		} //mal: don't bother with ppl who aren't in a valid AAS area!

		matesInArea = ClientsInArea( botNum, playerInfo.origin, 150.0f, botInfo->team, MEDIC, false, false, false, false, true );

		if ( matesInArea > 0 ) {
			continue;
		}

		vec = playerInfo.origin - botInfo->origin;

		dist = vec.LengthSqr();

		bool requestedHelp = false;

		if ( !playerInfo.isBot && ( playerInfo.lastChatTime[ REVIVE_ME ] + MEDIC_REQUEST_CONSIDER_TIME > botWorld->gameLocalInfo.time || playerInfo.lastChatTime[ HEAL_ME ] + MEDIC_REQUEST_CONSIDER_TIME > botWorld->gameLocalInfo.time ) ) {
			if ( botWorld->gameLocalInfo.gameIsBotMatch ) {
				range = MEDIC_RANGE_REQUEST * 2.0f;
			} else {
			range = MEDIC_RANGE_REQUEST;
			}
			requestedHelp = true;
		}

		if ( dist > Square( range ) ) { //mal: too far away - ignore!
			continue;
		}

		if ( escortClientNum != -1 && !requestedHelp ) { 
			vec = botWorld->clientInfo[ escortClientNum ].origin - playerInfo.origin;
			if ( vec.LengthSqr() > Square( MEDIC_RANGE_BUSY ) ) {
				vec = playerInfo.origin - botInfo->origin;
				if ( vec.LengthSqr() > Square( 500.0f ) ) {
					continue; //mal: ignore this client if hes too far away from our escort client, UNLESS the client is REALLY close to us....
				}
			}
		}

		if ( requestedHelp ) {
			if ( botWorld->gameLocalInfo.gameIsBotMatch ) {
				dist = 1.0f;
			} else {
			dist -= Square( 900.0f );
		}
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, playerInfo.origin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) { //mal: find the closest player in need, and help them first!
			clientNum = i;
			closest = dist;
		}
	}

	if ( clientNum == healClientNum && healClientNum != -1 ) {
		clientNum = -1; //mal: dont worry about re-calcing for a client we already have!
	}

	return clientNum;
}

/*
================
idBotAI::Bot_MedicCheckForDeadMateDuringCombat
================
*/
int idBotAI::Bot_MedicCheckForDeadMateDuringCombat( const idVec3 &dangerOrg, float range ) {

	int i;
	int clientNum = -1;
	float closest = idMath::INFINITY; //mal: set it to some large, crazy number
	float dist;
	idVec3	vec;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) {
			continue; //mal: dont try to heal ourselves!
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( ClientIsIgnored( i ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.inLimbo ) {
			continue;
		}		

		if ( playerInfo.team != botInfo->team ) {
			continue; //mal: give no comfort to the enemy!
		}

		if ( playerInfo.xySpeed > 0.0f ) {
			continue;
		} //mal: ignore bodies flying thru the air!

		if ( playerInfo.inWater ) {
			continue; //mal: can't revive ppl who are in water!
		}

		if ( playerInfo.health > 0 ) {
			continue;
		}

		if ( playerInfo.areaNum == 0 ) {
			continue;
		} //mal: can't heal someone not in a valid AAS area!

		vec = playerInfo.origin - botInfo->origin;
		dist = vec.LengthSqr();

		float tempRangeLimit = range;

		if ( botWorld->gameLocalInfo.gameIsBotMatch && !playerInfo.isBot ) { //mal: go out of our way more to revive the player in an SP game.
			tempRangeLimit = 2000.0f;
		}
	
		if ( dist > Square( tempRangeLimit ) ) { //mal: too far away from us - ignore!
			continue;
		}

		if ( botInfo->friendsInArea < 2 || botInfo->enemiesInArea > 1 ) {
			vec = playerInfo.origin - dangerOrg;
			if ( vec.LengthFast() < 500.0f ) {						//mal: this guys too close to danger - have to ignore him
				continue;
			}
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, playerInfo.origin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) { //mal: find the closest player in need, and help them first!
			clientNum = i;
			closest = dist;
		}
	}

	return clientNum;
}

/*
================
idBotAI::Bot_StroggCheckForGDFBodies
================
*/
int idBotAI::Bot_StroggCheckForGDFBodies( float range ) {

	int i;
	int bodyNum = -1;
	int mates;
	float closest = idMath::INFINITY; //mal: set it to some large, crazy number
	float dist;
	idVec3	vec;
	nbgTargetType = NOTYPE;
	int busyClient = -1;

//mal: this is an especially dangerous thing to do - if there are lots of enemies around, DONT spawnhost.
	if ( botInfo->enemiesInArea > 0 ) {
		return bodyNum;
	}

	if ( ClientHasObj( botNum ) ) {
		return bodyNum;
	}

//mal: first, check for wounded clients
	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) {
			continue; //mal: dont try to spawnhost ourselves!
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( ClientIsIgnored( i )) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.inLimbo ) {
			continue;
		}

		if ( playerInfo.team == botInfo->team ) {
			continue; 
		}

		if ( playerInfo.xySpeed > 0.0f ) {
			continue;
		} //mal: ignore bodies flying thru the air!

		if ( playerInfo.inWater ) {
			continue; //mal: can't host ppl who are in water!
		}

		if ( playerInfo.health > 0 ) {
			continue;
		}

		if ( playerInfo.areaNum == 0 ) {
			continue;
		} //mal: can't host someone not in a valid AAS area!

		vec = playerInfo.origin - botInfo->origin;

		dist = vec.LengthFast();

		if ( dist > range ) { //mal: too far away - ignore!
			continue;
		}

		if ( botWorld->botGoalInfo.team_STROGG_PrimaryAction != ACTION_NULL ) {
			vec = playerInfo.origin - botThreadData.botActions[ botWorld->botGoalInfo.team_STROGG_PrimaryAction ]->GetActionOrigin();
			if ( vec.LengthSqr() > Square( SPAWNHOST_RELEVANT_DIST ) ) { //mal: this is too far away from the obj to matter(?)
				continue;
			}
		}

		if ( !Bot_NBGIsAvailable( i, ACTION_NULL, CREATE_SPAWNHOST, busyClient )) {
			continue;
		}

		mates = ClientsInArea( botNum, playerInfo.origin, 150.0f, botInfo->team , MEDIC, false, false, false, false, true );

		if ( mates >= 1 ) {
			continue;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, playerInfo.origin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) { //mal: find the closest player to host!
			bodyNum = i;
			closest = dist;
			nbgTargetType = CLIENT; //mal: this is a client
		}
	}

//mal: now, check for bodies out there, and see if its closer....
	for ( i = 0; i < MAX_PLAYERBODIES; i++ ) {

		if ( !botWorld->playerBodies[ i ].isValid ) {
			continue; //mal: no body in this client slot!
		}

		if ( BodyIsIgnored( i )) {
			continue;
		}

		if ( botWorld->playerBodies[ i ].bodyTeam == botInfo->team ) {
			continue; //mal: dont try to spawnhost our team
		}

		if ( botWorld->playerBodies[ i ].areaNum == 0 ) {
			continue;
		}

		if ( !botWorld->playerBodies[ i ].isSpawnHostAble ) {
			continue;
		}

		if ( botWorld->botGoalInfo.team_STROGG_PrimaryAction != ACTION_NULL ) {
			vec = botWorld->playerBodies[ i ].bodyOrigin - botThreadData.botActions[ botWorld->botGoalInfo.team_STROGG_PrimaryAction ]->GetActionOrigin();
			if ( vec.LengthSqr() > Square( SPAWNHOST_RELEVANT_DIST ) ) { //mal: this is too far away from the obj to matter(?)
				continue;
			}
		}

		//mal_TODO: will want to add a test here that checks if we prefer bodies that are in enemy territory.

		vec = botWorld->playerBodies[ i ].bodyOrigin - botInfo->origin;

		dist = vec.LengthFast();

		if ( dist > range ) { //mal: too far away - ignore!
			continue;
		}

		mates = ClientsInArea( botNum, botWorld->playerBodies[ i ].bodyOrigin, 150.0f, botInfo->team , MEDIC, false, false, false, false, true );

		if ( mates >= 1 ) {
			continue;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, botWorld->playerBodies[ i ].bodyOrigin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) { //mal: find the closest body to host, and get them first
			bodyNum = i;
			closest = dist;
			nbgTargetType = BODY; //mal: this is a body
		}
	}

	return bodyNum;	
}

/*
================
idBotAI::Bot_MedicCheckForDeadMate

Checks for dead teammates. Works with both gdf and strogg (since the 2 are so similiar).
If the bot is already in the process of healing/reviving someone, reviveClientNum != -1,
in which case, the bot will only look at ones that are 
closer then their current NBG goal. "range" is how far bot will look to heal/revive. This will change based on what bot is
doing (if escorting important mate, or carrying OBJ, will not go as far to heal/revive). origin could be the bot's own origin,
or the origin of the important mate bot wants to cover.
================
*/
int idBotAI::Bot_MedicCheckForDeadMate( int reviveClientNum, int escortClientNum, float range ) {

	int i;
	int clientNum = -1;
	int busyClient;
	float closest = idMath::INFINITY; //mal: set it to some large, crazy number
	float dist;
	idVec3	vec;

	if ( reviveClientNum != -1 ) {
		vec = botWorld->clientInfo[ reviveClientNum ].origin - botInfo->origin;
		closest = vec.LengthFast();
		clientNum = reviveClientNum;
	} //mal: if we already have someone we're trying to revive, only revive someone else if they're closer!

	if ( botInfo->team == STROGG && ( botInfo->enemiesInArea > 1 || enemy != -1 || botInfo->lastAttackerTime + 3000 > botWorld->gameLocalInfo.time ) ) {
		return clientNum;
	}

	if ( range == 0.0f ) {
		return clientNum;
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) {
			continue; //mal: dont try to heal ourselves!
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( ClientIsIgnored( i ) ) {
			continue;
		}

//mal: some other bot has this client tagged for revive, so lets ignore this body.
//mal_NOTE: in this case, we dont care if a human medic is on route or nearby, just because experience shows most humans SUCK at being medics! ;-)
		if ( !Bot_NBGIsAvailable( i, ACTION_NULL, REVIVE_TEAMMATE, busyClient ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.inLimbo ) {
			continue;
		}

		if ( playerInfo.team != botInfo->team ) {
			continue; //mal: give no comfort to the enemy!
		}

		if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
			if ( Bot_CheckForHumanInteractingWithEntity( i ) == true ) {
				continue;
			}
		}

		if ( playerInfo.xySpeed > 0.0f ) {
			continue;
		} //mal: ignore bodies flying thru the air!

		if ( playerInfo.inWater ) {
			continue; //mal: can't revive ppl who are in water!
		}

		if ( playerInfo.health > 0 ) {
			continue;
		}

		if ( playerInfo.areaNum == 0 ) {
			continue;
		} //mal: can't revive someone not in a valid AAS area!

		bool requestedHelp = false;
		float tempRangeSqr = range;

		if ( !playerInfo.isBot && ( playerInfo.lastChatTime[ REVIVE_ME ] + MEDIC_REQUEST_CONSIDER_TIME > botWorld->gameLocalInfo.time || playerInfo.lastChatTime[ HEAL_ME ] + MEDIC_REQUEST_CONSIDER_TIME > botWorld->gameLocalInfo.time ) ) {
			tempRangeSqr = MEDIC_RANGE_REQUEST;
			requestedHelp = true;
		}

		if ( ( botWorld->gameLocalInfo.gameIsBotMatch || botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) && !playerInfo.isBot ) {
			tempRangeSqr = MEDIC_RANGE_REQUEST * 2.0f;
		}

		vec = playerInfo.origin - botInfo->origin;

		dist = vec.LengthSqr();

		if ( dist > Square( tempRangeSqr ) ) { //mal: too far away - ignore!
			continue;
		}

		if ( escortClientNum != -1 && !requestedHelp ) { 
			vec = botWorld->clientInfo[ escortClientNum ].origin - playerInfo.origin;
			if ( vec.LengthSqr() > Square( MEDIC_RANGE_BUSY ) ) {
				vec = playerInfo.origin - botInfo->origin;
				if ( vec.LengthSqr() > Square( 500.0f ) ) {
					continue; //mal: ignore this client if hes too far away from our escort client, UNLESS the client is REALLY close to us....
				}
			}
		}

		if ( requestedHelp ) { //mal: humans asking for help get a bit of priority.
			if ( botWorld->gameLocalInfo.gameIsBotMatch ) {
				dist = 1.0f;
			} else {
			dist -= Square( 900.0f );
			}
		} else if ( Client_IsCriticalForCurrentObj( i, 2500.0f ) ) { //mal: give some priority to critical teammates who are close to the goal!
			dist -= Square( 500.0f );
			if ( dist < 0.0f ) {
				dist = 1.0f;
			}
		} else if ( playerInfo.classType == MEDIC ) { //mal: next, give priority to medics - 2 medics are better then 1!
            dist -= Square( 500.0f );
			if ( dist < 0.0f ) {
				dist = 5.0f;	//mal: medics NEVER outrank critical teammates.
			}
		}

		bool inVehicle = ( botVehicleInfo == NULL ) ? false : true;
		int travelTime;

		if ( !Bot_LocationIsReachable( inVehicle, playerInfo.origin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) { //mal: find the closest player in need, and help them first!
			clientNum = i;
			closest = dist;
		}
	}

	if ( clientNum == reviveClientNum && reviveClientNum != -1 ) {
		clientNum = -1; //mal: dont reset for a client we already have, but DO keep the dist checks up to date, so we know we still have best client!
	}

	return clientNum;
}

/*
================
idBotAI::Bot_LookAtLocation

Instantly points the bot towards "spot".
================
*/
void idBotAI::Bot_LookAtLocation( const idVec3 &spot, const botTurnTypes_t turnType, bool useOriginOnly ) {
	if ( botVehicleInfo != NULL ) {
		botUcmd->moveViewOrigin = spot; //mal: let the vehicle handle the angles to point at, just pass the vector.
		botUcmd->viewType = VIEW_ORIGIN;
	} else {
		if ( useOriginOnly == false ) {
			idVec3 origin = spot - botInfo->viewOrigin;
			botUcmd->moveViewAngles = origin.ToAngles();
			botUcmd->viewType = VIEW_ANGLES;
		} else {
			botUcmd->moveViewOrigin = spot; //mal: let the code handle how to look at this target ( most likely bot is using a grenade ).
			botUcmd->viewType = VIEW_ORIGIN;
		}
	}

    botUcmd->turnType = turnType;

	if ( botThreadData.AllowDebugData() ) {
		idVec3 end = spot;
		end[ 2 ] += 48;
		gameRenderWorld->DebugLine( colorOrange, spot, end, 48 );
	}
}

/*
================
idBotAI::Flyer_LookAtLocation
================
*/
void idBotAI::Flyer_LookAtLocation( const idVec3 &spot ) {
	idVec3 origin = spot - botInfo->weapInfo.covertToolInfo.origin;
	botUcmd->moveViewAngles = origin.ToAngles();

	if ( botThreadData.AllowDebugData() ) {
		idVec3 end = spot;
		end[ 2 ] += 48;
		gameRenderWorld->DebugLine( colorOrange, spot, end, 48 );
	}
}

/*
================
idBotAI::Bot_LookAtEntity
================
*/
void idBotAI::Bot_LookAtEntity( int entityNum, const botTurnTypes_t turnType ) {
	botUcmd->viewType = VIEW_ENTITY;
	botUcmd->viewEntityNum = entityNum;
    botUcmd->turnType = turnType;
	botUcmd->moveViewAngles = ang_zero;
	botUcmd->moveViewOrigin = vec3_zero;
}

/*
================
idBotAI::Bot_LookAtNothing
================
*/
void idBotAI::Bot_LookAtNothing( const botTurnTypes_t turnType ) {
	botUcmd->viewType = VIEW_MOVEMENT;
    botUcmd->turnType = turnType;
}

/*
================
idBotAI::InFrontOfClient
================
*/
bool idBotAI::InFrontOfClient( int clientNum, const idVec3 &origin, bool precise, float preciseValue ) {
	float dotCheck = ( precise == true ) ? preciseValue : 0.0f;
	idVec3 dir = origin - botWorld->clientInfo[ clientNum ].origin;

	if ( precise ) {
		dir.NormalizeFast();
	}

	if ( dir * botWorld->clientInfo[ clientNum ].viewAxis[ 0 ] > dotCheck ) {
		return true;//mal: have someone in front of us..
	} //mal: since we're only calculating for 90 degrees, we don't need to normalize the dir.
	
	return false;
}

/*
================
idBotAI::ClientIsIgnored

Is this client being ignored?
================
*/
bool idBotAI::ClientIsIgnored( int clientNum ) {

	int i;

	for( i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		
		if ( ignoreClients[ i ].num != clientNum  ) {
			continue;
		}

		if ( ignoreClients[ i ].time > botWorld->gameLocalInfo.time ) {
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::EnemyIsIgnored

Is this ( potential ) enemy being ignored?
================
*/
bool idBotAI::EnemyIsIgnored( int clientNum ) {

	int i;

	for( i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		
		if ( ignoreEnemies[ i ].num != clientNum  ) {
			continue;
		}

		if ( ignoreEnemies[ i ].time > botWorld->gameLocalInfo.time ) {
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::VehicleIsIgnored

Is this vehicle being ignored?
================
*/
bool idBotAI::VehicleIsIgnored( int vehicleNum ) {

	int i;

	for( i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		
		if ( ignoreVehicles[ i ].num != vehicleNum  ) {
			continue;
		}

		if ( ignoreVehicles[ i ].time > botWorld->gameLocalInfo.time ) {
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::Bot_IgnoreVehicle

Setups a vehicle to be ignored
================
*/
void idBotAI::Bot_IgnoreVehicle( int vehicleNum, int time ) {

	bool hasSlot = false;
	int i;

	for( i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		if ( ignoreVehicles[ i ].time < botWorld->gameLocalInfo.time ) {
			ignoreVehicles[ i ].num = vehicleNum;
			ignoreVehicles[ i ].time = botWorld->gameLocalInfo.time + time;
			hasSlot = true;
			break;
		}
	}

	if ( hasSlot == false ) { //mal: if can't find a free slot ( shouldn't happen ), then just use the first slot.
		ignoreVehicles[ 0 ].num = vehicleNum;
		ignoreVehicles[ 0 ].time = botWorld->gameLocalInfo.time + time;
	}

	botThreadData.Warning( "Alert! Ignoring vehicle %i for %i msecs", vehicleNum, time );

	if ( hasSlot == false ) {
		botThreadData.Warning( "Alert! Ran out of free Ignore Vehicle Slots! Using the first one!" );
	}
}

/*
================
idBotAI::DeployableIsIgnored

Is this deployable being ignored?
================
*/
bool idBotAI::DeployableIsIgnored( int deployableNum ) {
	for( int i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		
		if ( ignoreDeployables[ i ].num != deployableNum ) {
			continue;
		}

		if ( ignoreDeployables[ i ].time > botWorld->gameLocalInfo.time ) {
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::Bot_IgnoreDeployable

Setups a deployable to be ignored
================
*/
void idBotAI::Bot_IgnoreDeployable( int deployableNum, int time ) {
	bool hasSlot = false;

	for( int i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		if ( ignoreDeployables[ i ].time < botWorld->gameLocalInfo.time ) {
			ignoreDeployables[ i ].num = deployableNum;
			ignoreDeployables[ i ].time = botWorld->gameLocalInfo.time + time;
			hasSlot = true;
			break;
		}
	}

	if ( hasSlot == false ) { //mal: if can't find a free slot ( shouldn't happen ), then just use the first slot.
		ignoreDeployables[ 0 ].num = deployableNum;
		ignoreDeployables[ 0 ].time = botWorld->gameLocalInfo.time + time;
	}

	botThreadData.Warning( "Alert! Ignoring deployable %i for %i msecs", deployableNum, time );

	if ( hasSlot == false ) {
		botThreadData.Warning( "Alert! Ran out of free Ignore Deployable Slots! Using the first one!" );
	}
}

/*
================
idBotAI::BodyIsIgnored

Is this dead body being ignored?
================
*/
bool idBotAI::BodyIsIgnored( int bodyNum ) {

	int i;

	for( i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		
		if ( ignoreBodies[ i ].num != bodyNum  ) {
			continue;
		}

		if ( ignoreBodies[ i ].time > botWorld->gameLocalInfo.time ) {
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::SpawnHostIsIgnored

Is this spawn host being ignored?
================
*/
bool idBotAI::SpawnHostIsIgnored( int bodyNum ) {

	int i;

	for( i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		
		if ( ignoreSpawnHosts[ i ].num != bodyNum  ) {
			continue;
		}

		if ( ignoreSpawnHosts[ i ].time > botWorld->gameLocalInfo.time ) {
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::Bot_IgnoreClient

Setups a client to be ignored
================
*/
void idBotAI::Bot_IgnoreClient( int clientNum, int time ) {

	if ( botWorld->botGoalInfo.isTrainingMap && botThreadData.actorMissionInfo.targetClientNum == clientNum ) { //mal: NEVER ignore the player!
		return;
	}

	bool hasSlot = false;
	int i;

	for( i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		if ( ignoreClients[ i ].time < botWorld->gameLocalInfo.time ) {
			ignoreClients[ i ].num = clientNum;
			ignoreClients[ i ].time = botWorld->gameLocalInfo.time + time;
			hasSlot = true;
			break;
		}
	}

	if ( hasSlot == false ) { //mal: if can't find a free slot ( shouldn't happen ), then just use the first slot.
		ignoreClients[ 0 ].num = clientNum;
		ignoreClients[ 0 ].time = botWorld->gameLocalInfo.time + time;
	}

	botThreadData.Warning( "Alert! Ignoring client %i for %i msecs", clientNum, time );
	if ( hasSlot == false ) {
		botThreadData.Warning( "Alert! Ran out of free Ignore Client Slots! Using the first one!" );
	}
}

/*
================
idBotAI::Bot_IgnoreEnemy

Setups an enemy to be ignored
================
*/
void idBotAI::Bot_IgnoreEnemy( int clientNum, int time ) {
	
	bool hasSlot = false;
	int i;

	for( i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		if ( ignoreEnemies[ i ].time < botWorld->gameLocalInfo.time ) {
			ignoreEnemies[ i ].num = clientNum;
			ignoreEnemies[ i ].time = botWorld->gameLocalInfo.time + time;
			hasSlot = true;
			break;
		}
	}

	if ( hasSlot == false ) { //mal: if can't find a free slot ( shouldn't happen ), then just use the first slot.
		ignoreEnemies[ 0 ].num = clientNum;
		ignoreEnemies[ 0 ].time = botWorld->gameLocalInfo.time + time;
	}

	botThreadData.Warning( "Alert! Ignoring client %i for %i msecs", clientNum, time );
	if ( hasSlot == false) {
		botThreadData.Warning( "Alert! Ran out of free Ignore Enemy Slots! Using the first one!" );
	}
}

/*
================
idBotAI::Bot_IgnoreBody

Setups a body to be ignored - only used by meds and coverts of both teams.
================
*/
void idBotAI::Bot_IgnoreBody( int bodyNum, int time ) {

	bool hasSlot = false;
	int i;

	for( i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		if ( ignoreBodies[ i ].time < botWorld->gameLocalInfo.time ) {
			ignoreBodies[ i ].num = bodyNum;
			ignoreBodies[ i ].time = botWorld->gameLocalInfo.time + time;
			hasSlot = true;
			break;
		}
	}

	if ( hasSlot == false ) { //mal: if can't find a free slot ( shouldn't happen ), then just use the first slot.
		ignoreBodies[ 0 ].num = bodyNum;
		ignoreBodies[ 0 ].time = botWorld->gameLocalInfo.time + time;
	}

	botThreadData.Warning( "Alert! Ignoring body %i for %i msecs", bodyNum, time );
	if ( hasSlot == false ) {
		botThreadData.Warning( "Alert! Ran out of free Ignore Body Slots! Using the first one!" );
	}
}

/*
================
idBotAI::Bot_IgnoreSpawnHost

Setups a spawnhost to be ignored - only used by meds on GDF.
================
*/
void idBotAI::Bot_IgnoreSpawnHost( int bodyNum, int time ) {

	bool hasSlot = false;
	int i;

	for( i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		if ( ignoreSpawnHosts[ i ].time < botWorld->gameLocalInfo.time ) {
			ignoreSpawnHosts[ i ].num = bodyNum;
			ignoreSpawnHosts[ i ].time = botWorld->gameLocalInfo.time + time;
			hasSlot = true;
			break;
		}
	}

	if ( hasSlot == false ) { //mal: if can't find a free slot ( shouldn't happen ), then just use the first slot.
		ignoreSpawnHosts[ 0 ].num = bodyNum;
		ignoreSpawnHosts[ 0 ].time = botWorld->gameLocalInfo.time + time;
	}

	botThreadData.Warning( "Alert! Ignoring body %i for %i msecs", bodyNum, time );
	if ( hasSlot == false ) {
		botThreadData.Warning( "Alert! Ran out of free Ignore Body Slots! Using the first one!" );
	}
}

/*
================
idBotAI::Bot_RandomLook

Randomly picks a spot to look at - will test to see if that the spot is a good one (so they wont be looking at a wall)
For any function that calls this, it is important to call ResetRandomLook() first, so that the random look range can
be reset.
================
*/
bool idBotAI::Bot_RandomLook( idVec3 &position, bool ignoreDelay ) {
	int turnCheck = 0;
	float yaw;
	idAngles ang = botInfo->viewAngles;
	idVec3 end, forward;

	if ( randomLookDelay > botWorld->gameLocalInfo.time && !ignoreDelay ) {
		return false;
	}

	while( turnCheck < 4 ) {

		turnCheck++;

		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			yaw = 45.0f;
		} else {
			yaw = 90.0f;
		}

		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			yaw *= -1.0f;
		}

		ang[ PITCH ] = 0; //mal: zero this out, in case bot is looking up, down, etc.
		ang[ YAW ] += yaw;

		ang.ToVectors( &forward, NULL, NULL );
        
		end = botInfo->viewOrigin;

		end += ( randomLookRange * forward );

		if ( end == randomLookOrg ) {
			continue;
		}

		if ( botThreadData.AllowDebugData() ) { 
			idVec3 test = end;
			test[ 2 ] += 24;
			gameRenderWorld->DebugLine( colorDkRed, end, test, 2500 );
		}

		if ( botThreadData.Nav_IsDirectPath( AAS_PLAYER, botInfo->team, botInfo->areaNum, botInfo->aasOrigin, end ) ) {
			position = end;
			randomLookOrg = end;
			randomLookDelay = botWorld->gameLocalInfo.time + 700; //mal: dont come thru here again for a little while at least.
			return true;
		}
	}

	randomLookRange -= 200.0f; //mal: shorten the look range a bit, so that we can try again next time ( in case we're in a tight area ).

	if ( randomLookRange < 100.0f ) {
		ResetRandomLook();
	}

	return false;
}

/*
================
idBotAI::Bot_ClearAngleMods

Clears out angle modifiers that could interfere with medics doing their job.
Medics are the only class that will really use this one, since they can do several tasks at once.
================
*/
void idBotAI::Bot_ClearAngleMods() {
    botUcmd->botCmds.lookDown = false;
	botUcmd->botCmds.lookUp = false;
	botUcmd->botCmds.throwNade = false;
}

/*
================
idBotAI::Bot_ExitAINode
================
*/
void idBotAI::Bot_ExitAINode() {
	bool resetAIStack = ( aiState == LTG ) ? true : false;
	lastActionNum = actionNum;
    Bot_ResetState( false, resetAIStack );
	ResetRandomLook();
	botIdealWeapSlot = GUN;
	botIdealWeapNum = NULL_WEAP;

	lastAINode = "Exiting AI Node";
}

/*
================
idBotAI::Bot_CheckSelfSupply
================
*/
bool idBotAI::Bot_CheckSelfSupply() {

	if ( botInfo->onLadder ) {
		return false;
	}

	if ( selfShockTime > botWorld->gameLocalInfo.time ) {
		return false;
	}

	if ( botInfo->isActor && ltgPauseTime > botWorld->gameLocalInfo.time ) { //mal: dont heal ourselves if we're briefing the player
		return false;
	}

	if ( !botInfo->inPlayZone ) {
		return false;
	}

	bool supplySelf = false;
	bool safeToExit = ( supplySelfTime < botWorld->gameLocalInfo.time ) ? true : false;
	bool hasCharge = ( botInfo->classType == MEDIC ) ? ClassWeaponCharged( HEALTH ) : ClassWeaponCharged( AMMO_PACK );
	weaponBankTypes_t weapon = ( aiState == LTG || ( aiState == NBG && ( nbgType == CAMP || nbgType == DEFENSE_CAMP ) ) || botInfo->classType == FIELDOPS ) ? GUN : NO_WEAPON;

	if ( ignoreWeapChange ) {
		weapon = botIdealWeapSlot;
	}

	if ( pistolTime > botWorld->gameLocalInfo.time || ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time && !botInfo->revived ) && botThreadData.GetBotSkill() > BOT_SKILL_EASY ) {
		if ( botInfo->isDisguised ) {
			weapon = MELEE;
		} else {
			weapon = SIDEARM;
		}
	}

    if ( !hasCharge && safeToExit ) {
		botIdealWeapSlot = weapon;
		return false;
	}

	if ( aiState == NBG && ( nbgType != CAMP && nbgType != DEFENSE_CAMP && nbgType != REVIVE_TEAMMATE && nbgType != SUPPLY_TEAMMATE && nbgType != CREATE_SPAWNHOST && nbgType != GRAB_SUPPLIES && nbgType != BUG_FOR_SUPPLIES && nbgType != AVOID_DANGER && nbgType != DESTROY_DANGER ) ) {
		botIdealWeapSlot = weapon;
		return false;
	}

	if ( classAbilityDelay > botWorld->gameLocalInfo.time ) {
		botIdealWeapSlot = weapon;
		return false;
	}

	if ( enemy != -1 && ( aiState == LTG || aiState == NBG )) { //mal: dont do it if the bot has an enemy hes aware of.
		supplySelfTime = 0;
		botIdealWeapSlot = weapon;
		return false;
	}

	int idealHealth = ( aiState == LTG || aiState == COMBAT ) ? botInfo->maxHealth : 60;

	if ( aiState == NBG && ( nbgType == CAMP || nbgType == DEFENSE_CAMP ) ) {
		idealHealth = botInfo->maxHealth;
	}

	if ( botInfo->team == STROGG ) {
#ifdef PACKS_HAVE_NO_NADES
		if ( botInfo->health >= idealHealth && botInfo->weapInfo.primaryWeapNeedsAmmo == false ) {
			supplySelfTime = 0;
			botIdealWeapSlot = weapon;
			return false;
		}
#else
		if ( botInfo->health >= idealHealth && botInfo->weapInfo.primaryWeapNeedsAmmo == false && botInfo->weapInfo.hasNadeAmmo ) { //mal: may be rearming just for the nades
			supplySelfTime = 0;
			botIdealWeapSlot = weapon;
			return false;
		}
#endif
	} else {
		if ( botInfo->classType == MEDIC ) {
            if ( botInfo->health >= idealHealth ) {
                supplySelfTime = 0;
				botIdealWeapSlot = weapon;
				return false;
			}
		} else {
#ifdef PACKS_HAVE_NO_NADES
			if ( botInfo->weapInfo.primaryWeapNeedsAmmo == false ) {
				supplySelfTime = 0;
				botIdealWeapSlot = weapon;
				return false;
			}
#else
			if ( botInfo->weapInfo.primaryWeapNeedsAmmo == false && botInfo->weapInfo.hasNadeAmmo ) { //mal: may be rearming just for the nades
				supplySelfTime = 0;
				botIdealWeapSlot = weapon;
				return false;
			}
#endif
		}
	}

	if ( botInfo->xySpeed > 0.0f ) { // was 100.0f
		supplySelf = true;
		botUcmd->botCmds.lookDown = true;
	} else if ( botInfo->xySpeed == 0.0f ) {
		supplySelf = true;
		botUcmd->botCmds.lookUp = true;
	}

	if ( !supplySelf ) {
		classAbilityDelay = botWorld->gameLocalInfo.time + 5000;	// dont let the bot try this again for a while, if something odd is going on to prevent them from doing this!
		supplySelfTime = 0;
		botIdealWeapSlot = weapon;
		return false;
	}

	if ( !hasCharge ) {
		botIdealWeapSlot = weapon;
		return false;
	} else {
        botIdealWeapSlot = NO_WEAPON;

		if ( botInfo->classType == FIELDOPS ) {
			botIdealWeapNum = AMMO_PACK;
		} else {
            botIdealWeapNum = HEALTH;
		}
	}

	skipStrafeJumpTime = botWorld->gameLocalInfo.time + 500; //mal: dont bother to try to strafe jump if the bot wants to resupply himself!

	supplySelfTime = botWorld->gameLocalInfo.time + 1200; //mal: give ourselves a chance to actually give that pack - just in case we ran out of charge with this one.
	
	if ( isStrafeJumping != false ) { //mal: if we're already strafejumping, knock it off, and let us supply ourselves!
        isStrafeJumping = false;
		botUcmd->moveFlag = RUN;
	}

	if ( botInfo->weapInfo.isReady && ( botInfo->weapInfo.weapon == HEALTH || botInfo->weapInfo.weapon == AMMO_PACK ) ) {
		botUcmd->botCmds.launchPacks = true;
	}
    
	return true;
}

/*
================
idBotAI::Bot_NBGIsAvailable

Checks to see if theres another bot out there that has this short term goal (heal, spawnhost, revive, etc)
If so, we'll do something else. For humans, will do a proximity check.
================
*/
bool idBotAI::Bot_NBGIsAvailable( int clientNum, int actionNumber, const bot_NBG_Types_t goalType, int &busyClient ) {
	
	int i;

	for( i = 0; i < MAX_CLIENTS; i++ ) {

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

		if ( playerInfo.team != botInfo->team ) { //mal: coverts ignore meds on enemy team who may try to heal them while they're disguised!
			continue;
		}

		if ( playerInfo.isBot != true ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetAIState() != NBG ) {
			continue;
		}

		if ( clientNum != -1 ) {
			if ( botThreadData.bots[ i ]->GetNBGTarget() != clientNum ) {
				continue;
			}
		}

		if ( actionNumber != ACTION_NULL ) {
			if ( botThreadData.bots[ i ]->GetActionNum() != actionNumber ) {
				continue;
			}
		}
		
		if ( botThreadData.bots[ i ]->GetNBGType() != goalType ) {
			continue;
		}

		busyClient = i;

		return false;
	}

	return true;
}

/*
================
idBotAI::Bot_LTGIsAvailable

Checks to see if theres another bot out there that has this long term goal.
If so, we'll do something else. For humans, will do a proximity check.
================
*/
bool idBotAI::Bot_LTGIsAvailable( int clientNum, int actionNumber, const bot_LTG_Types_t goalType, int minNumClients ) {
	
	int i;
	int numClients = 0;

	for( i = 0; i < MAX_CLIENTS; i++ ) {

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

		if ( botThreadData.bots[ i ]->GetAIState() != LTG ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetLTGType() != goalType ) {
			continue;
		}

		if ( clientNum != -1 ) {
			if ( botThreadData.bots[ i ]->GetLTGTarget() != clientNum ) {
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
================
idBotAI::Bot_NumClientsDoingLTGGoal

Checks to see how many bots are doing a particular LTG.
================
*/
int idBotAI::Bot_NumClientsDoingLTGGoal( int clientNum, int actionNumber, const bot_LTG_Types_t goalType, idList< int >& busyClients ) {
	
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

		if ( botThreadData.bots[ i ]->GetAIState() != LTG ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetLTGType() != goalType ) {
			continue;
		}

		if ( clientNum != -1 ) {
			if ( botThreadData.bots[ i ]->GetLTGTarget() != clientNum ) {
				continue;
			}
		}

		if ( actionNumber != ACTION_NULL ) {
			if ( botThreadData.bots[ i ]->GetActionNum() != actionNumber ) {
                continue;
			}
		}

		numClients++;
		busyClients.Append( i );
	}

	return numClients;
}

/*
================
idBotAI::Bot_CheckWeapon
================
*/
void idBotAI::Bot_CheckWeapon() {
	if ( botInfo->weapInfo.covertToolInfo.entNum != 0 && botInfo->team == STROGG ) {
		return;
	}

	if ( ignoreWeapChange ) {
		return;
	}

	botUcmd->botCmds.altAttackOff = true;

	if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) {
		botIdealWeapSlot = GUN;
	}

	if ( pistolTime > botWorld->gameLocalInfo.time && botThreadData.GetBotSkill() > BOT_SKILL_EASY && !NeedsReload() ) {
		if ( botInfo->isDisguised ) {
			botIdealWeapSlot = MELEE;
		} else {
			botIdealWeapSlot = SIDEARM;
		}
		return;
	}

	if ( botInfo->isDisguised ) { //mal: just use the default weapon for the class we look like.
		if ( botIdealWeapSlot != MELEE ) {
            botIdealWeapSlot = NO_WEAPON;
			return;
		}
	}

	if ( botInfo->classType == SOLDIER && ( botInfo->weapInfo.primaryWeapon == ROCKET || botInfo->weapInfo.primaryWeapon == HEAVY_MG ) && !NeedsReload() ) {
		botIdealWeapSlot = SIDEARM;
	} else {
        botIdealWeapSlot = GUN;
	}

	botIdealWeapNum = NULL_WEAP;
}

/*
================
idBotAI::Bot_CovertCheckForUniforms
================
*/
int idBotAI::Bot_CovertCheckForUniforms( float range ) {

	int i;
	int bodyNum = -1;
	int mates;
	float closest = idMath::INFINITY; //mal: set it to some large, crazy number
	float dist;
	idVec3	vec;
	nbgTargetType = NOTYPE;
	int busyClient = -1;

	if ( botInfo->isDisguised ) { //mal: already disguised, dont need to do this anymore.
		return bodyNum;
	}

	if ( !botWorld->gameLocalInfo.botsUseUniforms ) {
		return bodyNum;
	}

//mal: this is an especially dangerous thing to do - if there are lots of enemies around, DONT steal uniforms.
	if ( botInfo->enemiesInArea > 2 ) {
		return bodyNum;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
		return bodyNum;
	}

	if ( ClientHasObj( botNum ) ) {
		return bodyNum;
	}

//mal: first, check for wounded clients
	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( ClientIsIgnored( i )) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.inLimbo ) {
			continue;
		}

		if ( playerInfo.team == botInfo->team ) {
			continue; 
		}

		if ( LocationDistFromCurrentObj( botInfo->team, playerInfo.origin ) < BODY_IGNORE_RANGE && Client_IsCriticalForCurrentObj( botNum, -1.0f ) ) {
			continue;
		} //mal: if body is close to our current hack goal, then ignore it - just go hack!

		if ( playerInfo.xySpeed > 0.0f ) {
			continue;
		} //mal: ignore bodies flying thru the air!

		if ( playerInfo.inWater ) {
			continue; //mal: can't steal from ppl who are in water!
		}

		if ( playerInfo.health > 0 ) {
			continue;
		}

		if ( playerInfo.areaNum == 0 ) {
			continue;
		}

		vec = playerInfo.origin - botInfo->origin;

		dist = vec.LengthFast();

		if ( dist > range ) { //mal: too far away - ignore!
			continue;
		}

		if ( !Bot_NBGIsAvailable( i, ACTION_NULL, STEAL_UNIFORM, busyClient )) {
			continue;
		}

		mates = ClientsInArea( botNum, playerInfo.origin, 150.0f, botInfo->team , COVERTOPS, false, false, false, true, true );

		if ( mates >= 1 ) {
			continue;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, playerInfo.origin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) { //mal: find the closest player
			bodyNum = i;
			closest = dist;
			nbgTargetType = CLIENT; //mal: this is a client
		}
	}

//mal: now, check for bodies out there, and see if its closer....
	for ( i = 0; i < MAX_PLAYERBODIES; i++ ) {

		if ( !botWorld->playerBodies[ i ].isValid ) {
			continue; //mal: no body in this client slot!
		}

		if ( BodyIsIgnored( i )) {
			continue;
		}

		if ( botWorld->playerBodies[ i ].bodyTeam == botInfo->team ) {
			continue; //mal: dont try to steal from our team
		}

		if ( botWorld->playerBodies[ i ].areaNum == 0 ) {
			continue;
		}

		if ( LocationDistFromCurrentObj( botInfo->team,  botWorld->playerBodies[ i ].bodyOrigin ) < BODY_IGNORE_RANGE && Client_IsCriticalForCurrentObj( botNum, -1.0f ) ) {
			continue;
		} //mal: if body is close to our current hack goal, then ignore it - just go hack!

		if ( botWorld->playerBodies[ i ].bodyTeam == GDF ) {
            if ( !botWorld->playerBodies[ i ].isSpawnHostAble ) {
				continue;
			}
		} else {
			if ( botWorld->playerBodies[ i ].uniformStolen ) {
				continue;
			}
		}

		vec = botWorld->playerBodies[ i ].bodyOrigin - botInfo->origin;

		dist = vec.LengthSqr();

		if ( dist > Square( range ) ) { //mal: too far away - ignore!
			continue;
		}

		mates = ClientsInArea( botNum, botWorld->playerBodies[ i ].bodyOrigin, 150.0f, botInfo->team , COVERTOPS, false, false, false, true, true );

		if ( mates >= 1 ) {
			continue;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, botWorld->playerBodies[ i ].bodyOrigin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) {
			bodyNum = i;
			closest = dist;
			nbgTargetType = BODY; //mal: this is a body
		}
	}

	return bodyNum;	
}

/*
================
idBotAI::Bot_CovertCheckForVictims
================
*/
int idBotAI::Bot_CovertCheckForVictims( float range ) {
	int i;
	int victim = -1;
	int mates;
	float closest = idMath::INFINITY; //mal: set it to some large, crazy number
	float dist;
	idVec3	vec;
	int busyClient = -1;

	if ( !botInfo->isDisguised ) { //mal: not disguised, cant do this anymore.
		return victim;
	}

	if ( ClientHasObj( botNum ) ) {
		return victim;
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( ClientIsIgnored( i ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.inLimbo ) {
			continue;
		}

		if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO && !playerInfo.isBot ) { //mal: the bots won't backstab you in training mode, too confusing.
			continue;
		}

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			continue;
		}

		if ( playerInfo.isNoTarget ) {
			continue;
		}

		if ( playerInfo.team == botInfo->team ) {
			continue; 
		}
 
		if ( botInfo->team == GDF ) {
			if ( botWorld->gameLocalInfo.botSkill != BOT_SKILL_EXPERT ) { //mal: high skill bots are more likely to just backstab anyone they can....
			if ( playerInfo.classType != botWorld->botGoalInfo.team_STROGG_criticalClass && !ClientIsDangerous( i ) ) {
				continue;
			}
			}
		} else {
			if ( botWorld->gameLocalInfo.botSkill != BOT_SKILL_EXPERT ) {
			if ( playerInfo.classType != botWorld->botGoalInfo.team_GDF_criticalClass && !ClientIsDangerous( i ) ) {
				continue;
			}
		}
		}

		if ( botInfo->disguisedClient == i ) { //mal: dont do this to the guy we stole the uniform from, he'll get wise.
			continue;
		}

		if ( playerInfo.isDisguised ) {
			continue;
		}

		if ( playerInfo.inWater ) {
			continue;
		}

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		if ( playerInfo.areaNum == 0 ) {
			continue;
		}

		if ( playerInfo.isActor ) {
			continue;
		}

		vec = playerInfo.origin - botInfo->origin;

		dist = vec.LengthSqr();

		if ( dist > Square( range ) ) { //mal: too far away - ignore!
			continue;
		}

		if ( playerInfo.friendsInArea > MAX_NUM_OF_FRIENDS_OF_VICTIM ) { //mal: this guys got a lot of friends around him - prolly a better idea to try something else....
			return victim;
		}	

		if ( !Bot_NBGIsAvailable( i, ACTION_NULL, STALK_VICTIM, busyClient )) { //mal: some other bot is doing the same thing, so do something else.
			continue;
		}

		mates = ClientsInArea( botNum, playerInfo.origin, 500.0f, botInfo->team , COVERTOPS, false, false, false, false, true );

		if ( mates >= 1 ) { //mal: someone else MAY already have the same idea, so do something else just in case!
			continue;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, playerInfo.origin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) { //mal: find the closest player to knife in the back!
			victim = i;
			closest = dist;
		}
	}

	return victim;	
}

/*
==================
idBotAI::Bot_CheckHasVisibleMedicNearby

Matches the functionality of code in idPlayer::CalculateRenderView, to lock the player's view on a nearby medic
==================
*/
bool idBotAI::Bot_CheckHasVisibleMedicNearby() {

	int i;
	trace_t trace;
	idVec3 vec;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			continue;
		}

		if ( playerInfo.team != botInfo->team ) {
			continue;
		}

		if ( playerInfo.classType != MEDIC ) {
			continue;
		}

		vec = playerInfo.origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( 1024.0f ) ) {
			continue;
		}

		botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS trace, playerInfo.viewOrigin, botInfo->viewOrigin, CONTENTS_SOLID, GetGameEntity( botNum ) );

		if ( trace.fraction >= 1.0f ){
			return true;
		}
	}

	return false;
}

/*
==================
idBotAI::LocationVis2Sky
==================
*/
bool idBotAI::LocationVis2Sky( const idVec3 &loc ) {
	int areaNum = botThreadData.Nav_GetAreaNum( ( botVehicleInfo != NULL ) ? AAS_VEHICLE : AAS_PLAYER, loc );

	if ( areaNum > 0 ) {
		if ( botAAS.aas->GetAreaFlags( areaNum ) & AAS_AREA_OUTSIDE ) {
			return true;
		}
	}

	return false;
}

/*
==================
idBotAI::LocationHasHeadRoom
==================
*/
bool idBotAI::LocationHasHeadRoom( const idVec3 &loc ) {
	int areaNum = botThreadData.Nav_GetAreaNum( ( botVehicleInfo != NULL ) ? AAS_VEHICLE : AAS_PLAYER, loc );

	if ( areaNum > 0 ) {
		if ( botAAS.aas->GetAreaFlags( areaNum ) & AAS_AREA_HIGH_CEILING ) {
			return true;
		}
	}

	return false;
}

/*
==================
idBotAI::ClientIsValid
==================
*/
bool idBotAI::ClientIsValid( int clientNum, int spawnID ) {

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) { //mal: safety check!
		return false;
	}

	if ( spawnID != -1 ) {
		if ( botWorld->clientInfo[ clientNum ].spawnID != spawnID ) {
			assert( false ); //mal: this is a temp check to test new code.		//mal_TODO: REMOVE this!
			return false;
		}
	}

	if ( botWorld->clientInfo[ clientNum ].inGame == false || botWorld->clientInfo[ clientNum ].team == NOTEAM ) {
		return false;
	} else {
		return true;
	}
}

/*
==================
idBotAI::ClientHasChargeInWorld
==================
*/
bool idBotAI::ClientHasChargeInWorld( int clientNum, bool armedOnly, int actionNumber, bool unArmedOnly ) {
	
	bool hasBomb = false;

	for( int i = 0; i < MAX_CLIENT_CHARGES; i++ ) {
		if ( botWorld->chargeInfo[ i ].entNum == 0 ) {
			continue;
		}

		if ( botWorld->chargeInfo[ i ].ownerSpawnID != botWorld->clientInfo[ clientNum ].spawnID ) {
			continue;
		}

		if ( armedOnly ) {
			if ( botWorld->chargeInfo[ i ].state != BOMB_ARMED ) {
				continue;
			}
		}

		if ( unArmedOnly ) {
			if ( botWorld->chargeInfo[ i ].state == BOMB_ARMED ) {
				continue;
			}
		}

		if ( actionNumber != ACTION_NULL ) {
			if ( botThreadData.botActions[ actionNumber ]->EntityIsInsideActionBBox( botWorld->chargeInfo[ i ].entNum, PLANTED_CHARGE ) == false ) {
				continue;
			}
		}
		
		hasBomb = true;
		break;
	}

	return hasBomb;
}

/*
==================
idBotAI::FindChargeInWorld

Look for a bomb out there to arm/disarm.
==================
*/
bool idBotAI::FindChargeInWorld( int actionNumber, plantedChargeInfo_t& bombInfo, bool chargeState, bool ignoreZCheck ) {
	float closest = idMath::INFINITY;
	bombInfo.entNum = 0; //mal: clear this out, so we can tell if we got one or not later.
	playerClassTypes_t bombClassType = ( chargeState != ARM ) ? ENGINEER : SOLDIER;

	for( int i = 0; i < MAX_CLIENT_CHARGES; i++ ) {
		const plantedChargeInfo_t& charge = botWorld->chargeInfo[ i ];

		if ( charge.entNum == 0 ) {
			continue;
		}

		if ( charge.team != botInfo->team && chargeState == ARM ) {
            continue;
		}

		if ( charge.team == botInfo->team && chargeState == DISARM ) {
            continue;
		}

		if ( charge.state == BOMB_NULL && chargeState == DISARM ) {
			continue;
		}

		if ( charge.state == BOMB_ARMED && chargeState == ARM ) {
			continue;
		}

		idVec3 vec = charge.origin - botInfo->origin;

		float dist = vec.LengthSqr();

		if ( dist > Square( 1500.0f ) ) { //mal: this bomb is too far away from us.
			continue;
		}

		if ( chargeState == ARM ) {
			if ( !ignoreZCheck ) {
			if ( vec.z > 80.0f || vec.z < -80.0f ) {
				continue;
			}
		}
		}

//		if ( actionNumber != ACTION_NULL ) {
//			if ( !botThreadData.botActions[ actionNumber ]->actionBBox.ContainsPoint( charge.origin ) ) {
//				continue;
//			}
//		}

		int matesInArea = ClientsInArea( botNum, charge.origin, PLIERS_RANGE, botInfo->team, bombClassType, false, false, false, false, true );

		if ( matesInArea > 1 ) {
			if ( chargeState == ARM && charge.ownerSpawnID != botInfo->spawnID ) { //mal: dont cluster around bombs, DO look for bombs that may have been dropped/planted, and the player died/left before arming/disarming.
				continue;
			} else if ( chargeState == DISARM && Bot_CheckIfClientHasChargeAsGoal( charge.entNum ) ) { //mal: dont ignore a bomb with lots of clients around, if noones disarming it.
				continue;
			}
		}

		if ( dist < closest ) { //mal: find the closest bomb, whether its ours or not.
			bombInfo = charge;
			closest = dist;
		}
	}

	return ( bombInfo.entNum != 0 );
}

/*
================
idBotAI::Bot_IgnoreAction

Setups an action to be ignored
================
*/
void idBotAI::Bot_IgnoreAction( int actionNumber, int time ) {
	bool hasSlot = false;

	for( int i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		if ( ignoreActions[ i ].time < botWorld->gameLocalInfo.time ) {
			ignoreActions[ i ].num = actionNumber;
			ignoreActions[ i ].time = botWorld->gameLocalInfo.time + time;
			hasSlot = true;
			break;
		}
	}

	if ( hasSlot == false ) { //mal: if can't find a free slot ( shouldn't happen ), then just use the first slot.
		ignoreActions[ 0 ].num = actionNumber;
		ignoreActions[ 0 ].time = botWorld->gameLocalInfo.time + time;
	}

	botThreadData.Warning( "Alert! Ignoring Action %i for %i msecs", actionNumber, time );

	if ( hasSlot == false ) {
		botThreadData.Warning( "Alert! Ran out of free Ignore Action Slots! Using the first one!" );
	}
}

/*
================
idBotAI::ActionIsIgnored

Is this action being ignored for some reason?
================
*/
bool idBotAI::ActionIsIgnored( int actionNumber ) {
	for( int i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		
		if ( ignoreActions[ i ].num != actionNumber ) {
			continue;
	}

		if ( ignoreActions[ i ].time > botWorld->gameLocalInfo.time ) {
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::Bot_IgnoreItem

Setups an item to be ignored
================
*/
void idBotAI::Bot_IgnoreItem( int itemNumber, int time ) {
	bool hasSlot = false;

	for( int i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		if ( ignoreItems[ i ].time < botWorld->gameLocalInfo.time ) {
			ignoreItems[ i ].num = itemNumber;
			ignoreItems[ i ].time = botWorld->gameLocalInfo.time + time;
			hasSlot = true;
			break;
		}
	}

	if ( hasSlot == false ) { //mal: if can't find a free slot ( shouldn't happen ), then just use the first slot.
		ignoreItems[ 0 ].num = itemNumber;
		ignoreItems[ 0 ].time = botWorld->gameLocalInfo.time + time;
	}

	botThreadData.Warning( "Alert! Ignoring Item %i for %i msecs", itemNumber, time );

	if ( hasSlot == false ) {
		botThreadData.Warning( "Alert! Ran out of free Ignore Item Slots! Using the first one!" );
	}
}

/*
================
idBotAI::ItemIsIgnored

Is this item being ignored for some reason?
================
*/
bool idBotAI::ItemIsIgnored( int itemNumber ) {
	for( int i = 0; i < MAX_IGNORE_ENTITIES; i++ ) {
		
		if ( ignoreItems[ i ].num != itemNumber ) {
			continue;
		}

		if ( ignoreItems[ i ].time > botWorld->gameLocalInfo.time ) {
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::CheckItemPackIsValid

Is our item pack still valid?
================
*/
bool idBotAI::CheckItemPackIsValid( int entNum, idVec3 &packOrg, idBounds &entBounds, int& spawnID ) {

	bool foundPack = false;
	int i, j;

	for( j = 0; j < MAX_CLIENTS; j++ ) {

		if ( foundPack ) {
			break;
		}

		if ( botWorld->clientInfo[ j ].supplyCrate.entNum == entNum ) { //mal: our target may be a supply crate - so check the player's crates
			packOrg = botWorld->clientInfo[ j ].supplyCrate.origin;
			entBounds = botWorld->clientInfo[ j ].supplyCrate.bbox;
			spawnID = botWorld->clientInfo[ j ].supplyCrate.spawnID;
			return true;
		}

        for( i = 0; i < MAX_ITEMS; i++ ) {

            if ( botWorld->clientInfo[ j ].packs[ i ].entNum != entNum ) {
				continue;
			}

			packOrg = botWorld->clientInfo[ j ].packs[ i ].origin;
			packOrg.z += ITEM_PACK_OFFSET;
			spawnID = botWorld->clientInfo[ j ].packs[ i ].spawnID;
			foundPack = true;
			break;
		}
	}

	return foundPack;
}

/*
================
idBotAI::Bot_CheckForNeedyTeammates

Checks for teammates who need ammo.
================
*/
int idBotAI::Bot_CheckForNeedyTeammates( float range ) {
	int i;
	int clientNum = -1;
	int busyClient;
	int matesInArea;
	float closest = idMath::INFINITY; //mal: set it to some large, crazy number
	float dist;
	idVec3	vec;
	
//mal: dont bother doing this if bot has no charge
	if ( !ClassWeaponCharged( AMMO_PACK ) ) {
		botIdealWeapSlot = GUN;
		return clientNum;
	}

	if ( ClientHasObj( botNum ) ) {
		return clientNum;
	}

	if ( botInfo->inWater ) {
		return clientNum;
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) {
			continue; //mal: dont try to rearm ourselves!
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}
 
		if ( ClientIsIgnored( i )) {
			continue;
		}

//mal: some bot has this client tagging for healing.
//mal_NOTE: in this case, we dont care if a human FOps is on route or nearby, just because experience shows most humans SUCK at giving ammo! ;-)
		if ( !Bot_NBGIsAvailable( i, ACTION_NULL, SUPPLY_TEAMMATE, busyClient ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.inLimbo ) {
			continue;
		}

		if ( playerInfo.isNoTarget ) {
			continue;
		}

		if ( botInfo->team == GDF ) {
            if ( playerInfo.classType == FIELDOPS ) {
				continue; 
			} //mal: if player is a Fops, ignore, they can rearm themselves!
		} else {
			if ( playerInfo.classType == MEDIC ) { //mal: the same for medics on the strogg team!
				continue;
			}
		}

		if ( playerInfo.team != botInfo->team && playerInfo.isDisguised == false ) {
			continue; //mal: give no comfort to the enemy! can be tricked by disguised enemy.
		}

		if ( !playerInfo.weapInfo.primaryWeapNeedsAmmo && playerInfo.lastChatTime[ REARM_ME ] + 5000 < botWorld->gameLocalInfo.time ) {
			continue;
		}

		if ( playerInfo.inWater ) {
			continue; //mal: can't rearm ppl who are in water!
		}

		if ( playerInfo.health <= 0 ) { //mal: dont bother if client is dead.
			continue;
		}

		if ( !playerInfo.hasGroundContact && !playerInfo.hasJumped ) {
			continue;
		} //mal: ignore ppl flying thru the air (from explosion, dropping from airplane, etc).

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			continue; //mal: ignore players in a vehicle/deployable.
		}

		if ( playerInfo.areaNum == 0 ) {
			continue;
		} //mal: don't bother with ppl who aren't in a valid AAS area!

		matesInArea = ClientsInArea( botNum, playerInfo.origin, 150.0f, botInfo->team, ( botInfo->classType == MEDIC ) ? MEDIC : FIELDOPS, false, false, false, false, true );

		if ( matesInArea > 0 ) {
			continue;
		}

		vec = playerInfo.origin - botInfo->origin;

		dist = vec.LengthFast();

		bool requestedHelp = false;
		float tempRange = range;

		if ( !playerInfo.isBot && ( botWorld->gameLocalInfo.gameIsBotMatch || botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) ) {
			tempRange *= 2.0f;
		}

		if ( !playerInfo.isBot && ( playerInfo.lastChatTime[ REVIVE_ME ] + MEDIC_REQUEST_CONSIDER_TIME > botWorld->gameLocalInfo.time || playerInfo.lastChatTime[ HEAL_ME ] + MEDIC_REQUEST_CONSIDER_TIME > botWorld->gameLocalInfo.time || playerInfo.lastChatTime[ REARM_ME ] + MEDIC_REQUEST_CONSIDER_TIME > botWorld->gameLocalInfo.time ) ) {
			tempRange = MEDIC_RANGE_REQUEST;
			requestedHelp = true;
		}

		if ( requestedHelp ) {
			dist -= 900.0f;
		}

		if ( dist > tempRange ) { //mal: too far away - ignore!
			continue;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, playerInfo.origin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) { //mal: find the closest player in need, and help them first!
			clientNum = i;
			closest = dist;
		}
	}

	return clientNum;
}


/*
================
idBotAI::Client_IsCriticalForCurrentObj

Is this client critical to completing our current obj?
If range != -1, will only consider client critical if hes within range of the goal.
================
*/
bool idBotAI::Client_IsCriticalForCurrentObj( int clientNum, float range ) {
	int botActionNum;
	botActionGoals_t actionGoal;
	botActionStates_t actionState;
	idVec3 vec;

	if ( !ClientIsValid( clientNum, -1 ) ) {
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ clientNum ];

	if ( botWorld->gameLocalInfo.heroMode != false ) { //mal: the human has decided to complete the maps goals.
		if ( playerInfo.isBot != false && TeamHasHuman( botInfo->team ) ) {
            return false;
		}
	} //mal: as long as there is at least one human on their team, bots are NEVER considered critical in hero mode.

	if ( playerInfo.classType == MEDIC || playerInfo.classType == FIELDOPS ) { //mal: there currently are no medic/Fops specific objs.
		return false;
	}

	if ( botInfo->classType == ENGINEER && botWorld->botGoalInfo.mapHasMCPGoal ) {
		if ( range != -1.0f ) {
			proxyInfo_t mcp;
			GetVehicleInfo( botWorld->botGoalInfo.botGoal_MCP_VehicleNum, mcp );

			if ( mcp.entNum != 0 ) {
				idVec3 vec = mcp.origin - botInfo->origin;

				if ( vec.LengthSqr() < Square( range ) ) {
					return true;
				}
			}
		} else {
			return true;
		}
	}

//mal: not sure about this below - causes a lot of issues where the a bot thats not in a critical situation, ignores combat when it really shouldn't.
/*

	if ( botInfo->team == GDF ) {
		if ( botWorld->botGoalInfo.team_GDF_criticalClass == botInfo->classType ) {
			return true;
		}
	} else {
		if ( botWorld->botGoalInfo.team_STROGG_criticalClass == botInfo->classType ) {
			return true;
		}
	}
*/

	if ( playerInfo.team == GDF ) {
		botActionNum = botWorld->botGoalInfo.team_GDF_PrimaryAction;

		if ( botActionNum == -1 || botActionNum > botThreadData.botActions.Num() ) {
			return false;
		}
        
		actionGoal = botThreadData.botActions[ botActionNum ]->GetHumanObj();
	} else {
		botActionNum = botWorld->botGoalInfo.team_STROGG_PrimaryAction;
		
		if ( botActionNum == -1 || botActionNum > botThreadData.botActions.Num() ) {
			return false;
		}

		actionGoal = botThreadData.botActions[ botActionNum ]->GetStroggObj();
	}
	
	actionState = botThreadData.botActions[ botActionNum ]->GetActionState();

	if ( range != -1.0f ) {
		vec = botThreadData.botActions[ botActionNum ]->GetActionOrigin() - playerInfo.origin;

		if ( vec.LengthSqr() > Square( range ) ) {
			return false;
		}
	}

	if ( actionGoal == ACTION_HE_CHARGE && playerInfo.classType == SOLDIER && !ClientHasChargeInWorld( clientNum, true, ACTION_NULL ) ) {
		return true;
	} else if ( actionGoal == ACTION_DEFUSE && ( actionState == ACTION_STATE_PLANTED || Bot_CheckChargeExistsOnObjInWorld() ) && playerInfo.classType == ENGINEER ) {
		return true;
	} else if ( actionGoal == ACTION_MAJOR_OBJ_BUILD && playerInfo.classType == ENGINEER ) {
		return true;
	} else if ( actionGoal == ACTION_HACK && playerInfo.classType == COVERTOPS ) {
		return true;
	}

	return false;
}

/*
================
idBotAI::FindBodyOfClient
================
*/
int idBotAI::FindBodyOfClient( int clientNum, bool stealUniform, const idVec3 &org ) {

	int i;
	int bodyNum = -1;
	idVec3 vec;

//mal: look for the body belonging to clientNum.
	for ( i = 0; i < MAX_PLAYERBODIES; i++ ) {

        if ( !botWorld->playerBodies[ i ].isValid ) {
			continue; //mal: no body in this client slot!
		}

		if ( botWorld->playerBodies[ i ].bodyOwnerClientNum != clientNum ) {
			continue; //mal: dont try to steal a diff body
		}

		vec = botWorld->playerBodies[ i ].bodyOrigin - org;

		if ( vec.LengthSqr() > Square( 100.0f ) ) {
			continue;
		}

		if ( stealUniform == true ) {
			if ( botWorld->playerBodies[ i ].uniformStolen == true ) {
				continue;
			}

			if ( botWorld->playerBodies[ i ].isSpawnHost == true ) {
				continue;
			}
		} else { //mal: else, we're trying to spawnhost the body, make sure its not already spawnhosted!
			if ( botWorld->playerBodies[ i ].isSpawnHostAble == false ) {
				continue;
			}
		}

		bodyNum = i;
		break;
	}

	return bodyNum;	
}

/*
================
idBotAI::LocationDistFromCurrentObj
================
*/
float idBotAI::LocationDistFromCurrentObj( const playerTeamTypes_t team, const idVec3 &org ) {

	int botActionNum;
	botActionGoals_t actionGoal;
	idVec3 vec;

	if ( team == GDF ) {
		botActionNum = botWorld->botGoalInfo.team_GDF_PrimaryAction;

		if ( botActionNum == -1 ) {
			return idMath::INFINITY;
		}

		actionGoal = botThreadData.botActions[ botActionNum ]->GetHumanObj();
	} else {
		botActionNum = botWorld->botGoalInfo.team_STROGG_PrimaryAction;

		if ( botActionNum == -1 ) {
			return idMath::INFINITY;
		}

		actionGoal = botThreadData.botActions[ botActionNum ]->GetStroggObj();
	}
	
	vec = org - botThreadData.botActions[ botActionNum ]->GetActionOrigin();

	return vec.LengthFast();
}

/*
================
idBotAI::Bot_IsAwareOfDanger
================
*/
bool idBotAI::Bot_IsAwareOfDanger( int entNum, const idVec3& org, const idMat3& dir ) {
	int i;

	for( i = 0; i < MAX_DANGERS; i++ ) {
		
		if ( currentDangers[ i ].num != entNum ) {
			continue;
		}

		if ( currentDangers[ i ].time > botWorld->gameLocalInfo.time ) {
			if ( !DangerStillExists( currentDangers[ i ].num, currentDangers[ i ].ownerNum ) ) {
				ClearDangerFromDangerList( i, false );
				return false;
			}

			if ( org != vec3_zero ) {
				currentDangers[ i ].origin = org;
			}

			if ( currentDangers[ i ].dir != mat3_identity ) {
				currentDangers[ i ].dir = dir;
			}

			return true;
		}
	}

	return false;
}

/*
================
idBotAI::AddDangerToAwareList

Time is in seconds.
================
*/
int idBotAI::AddDangerToAwareList( int entNum, const idVec3 &org, const dangerTypes_t dangerType, int time, int ownerClientNum, const idMat3& dir ) {

	int i;

	for( i = 0; i < MAX_DANGERS; i++ ) {
		
		if ( currentDangers[ i ].num != 0 ) {
			if ( currentDangers[ i ].time > botWorld->gameLocalInfo.time ) {
				continue;
			}
		}

		currentDangers[ i ].num = entNum;
		currentDangers[ i ].ownerNum = ownerClientNum;
		currentDangers[ i ].origin = org;
		currentDangers[ i ].type = dangerType;
		currentDangers[ i ].time = ( time * 1000 ) + botWorld->gameLocalInfo.time;
		currentDangers[ i ].dir = dir;
		break;
	}

	if ( i == MAX_DANGERS ) {
		botThreadData.Warning( "Danger List ran out of room!" );
		i = -1;
	}

	return i;
}

/*
================
idBotAI::Bot_CheckForDangers

This is called by the AI nodes to have the bots avoid dangers
================
*/
void idBotAI::Bot_CheckForDangers( bool inCombat ) {
	bool inFront;
	bool mcpDanger = false;
	int dangerIndex;
	int nadeDangers = 0;
	int airStrikeDangers = 0;
	int bombDangers = 0;
	int mineDangers = 0;
	int covertToolDangers = 0;
	int	stroyBombDangers = 0;
	int turretDangers = 0;
	int artyDangers	= 0;
	int i, j;
	float dist;
	float maxDist = ( botInfo->team == STROGG ) ? 512.0f : 1024.0f;
    trace_t	tr; //mal: an AAS trace is just too inprecise for something like this, so need to do a costly vis check.
	proxyInfo_t vehicleInfo;
	idVec3 vec;

	if ( ignoreDangersTime > botWorld->gameLocalInfo.time ) {
		return;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) { //mal: low skill bots wont check for dangers at all.
		return;
	}

	ignoreDangersTime = botWorld->gameLocalInfo.time + 100;

	actionNumInsideDanger = ACTION_NULL;

	turretDangerExists = false;
	turretDangerEntNum = -1;

    for( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( inCombat ) {
			continue;
		} //mal: 11 and a half hour change: don't avoid certain dangers in combat, as that causes the jittering issues - 4/16/08

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		if ( i != botNum ) {
			if ( botWorld->gameLocalInfo.friendlyFireOn == false ) {
				if ( botWorld->clientInfo[ i ].team == botInfo->team ) {
					continue; //mal: if FF is off, dont worry about teammate grenades - ALWAYS worry about our own tho!
				}
			}
		}

//mal: first check for grenades
		for( j = 0; j < MAX_GRENADES; j++ ) {
			const grenadeInfo_t& nadeInfo = botWorld->clientInfo[ i ].weapInfo.grenades[ j ];

			if ( nadeInfo.entNum == 0 ) {
				continue;
			}

			if ( Bot_IsAwareOfDanger( nadeInfo.entNum, nadeInfo.origin, mat3_identity ) ) {
				continue;
			}

			if ( nadeInfo.xySpeed > 150.0f ) { //mal: dont worry about nades moving thru the air
				continue;
			}

			vec = nadeInfo.origin - botInfo->origin;

			if ( vec.LengthSqr() > Square( 350.0f ) ) { //mal: if the nade if far enough away, dont worry about it!
				continue;
			}

			inFront = InFrontOfClient( botNum, nadeInfo.origin );

			if ( botWorld->gameLocalInfo.botSkill < BOT_SKILL_EXPERT || inCombat ) { //mal: high skill bots will hear the nade ping at their back, others wont. Unless theyre in combat, in which case noone will hear it.
                if ( !inFront ) {
					continue;
				}
			}

			if ( !inFront && botInfo->xySpeed > WALKING_SPEED ) { //mal: if we're moving, and its behind us, ignore it.
                continue;
			} else {
				vec = nadeInfo.origin;

				vec[ 2 ] += 8; //mal: raise it up a bit, so we can see it on uneven surfaces.

				botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, vec, MASK_EXPLOSIONSOLID, GetGameEntity( botNum ) );

				if ( tr.fraction < 1.0f ) { //mal: its not visible to us, so dont worry about it.
					continue;
				}

				if ( AddDangerToAwareList( nadeInfo.entNum, nadeInfo.origin, HAND_GRENADE, 4, i ) != -1 ) {
                    nadeDangers++;
				}

				botThreadData.Printf( "Bot #%i is aware of a grenade!\n", botNum );
			}
		}
	}
	
//mal: first check for stroybombs
	for( j = 0; j < MAX_STROYBOMBS; j++ ) {
		if ( inCombat ) {
			continue;
		} //mal: 11 and a half hour change: don't avoid certain dangers in combat, as that causes the jittering issues - 4/16/08

		if ( botWorld->gameLocalInfo.friendlyFireOn == false && botInfo->team == STROGG ) {
			continue;
		}

		const stroyBombInfo_t& stroyBomb = botWorld->stroyBombs[ j ];

		if ( stroyBomb.entNum == 0 ) {
			continue;
		}

		if ( Bot_IsAwareOfDanger( stroyBomb.entNum, stroyBomb.origin, mat3_identity ) ) {
			continue;
		}

		if ( stroyBomb.xySpeed > 150.0f ) { //mal: dont worry about nades moving thru the air
			continue;
		}

		vec = stroyBomb.origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( 350.0f ) ) { //mal: if the nade if far enough away, dont worry about it!
			continue;
		}

		inFront = InFrontOfClient( botNum, stroyBomb.origin );

		if ( botWorld->gameLocalInfo.botSkill < BOT_SKILL_EXPERT || inCombat ) { //mal: high skill bots will hear the nade ping at their back, others wont. Unless theyre in combat, in which case noone will hear it.
			if ( !inFront ) {
				continue;
			}
		}

		if ( !inFront && botInfo->xySpeed > WALKING_SPEED ) { //mal: if we're moving, and its behind us, ignore it.
			continue;
		} else {
			vec = stroyBomb.origin;

			vec[ 2 ] += 8; //mal: raise it up a bit, so we can see it on uneven surfaces.

			botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, vec, MASK_EXPLOSIONSOLID, GetGameEntity( botNum ) );

			if ( tr.fraction < 1.0f ) { //mal: its not visible to us, so dont worry about it.
				continue;
			}

			if ( AddDangerToAwareList( stroyBomb.entNum, stroyBomb.origin, STROY_BOMB_DANGER, 4, i ) != -1 ) {
				stroyBombDangers++;
			}

			botThreadData.Printf( "Bot #%i is aware of a stroy bomb!\n", botNum );
		}
	}

//mal: next, look for airstrikes.
	if ( LocationVis2Sky( botInfo->origin ) ) { //mal: airstrike only dangerous if we're outside.
		for( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( !ClientIsValid( i, -1 ) ) {
				continue;
			}

			if ( inCombat ) {
				continue;
			} //mal: 11 and a half hour change: don't avoid certain dangers in combat, as that causes the jittering issues - 4/16/08

			if ( i != botNum ) {
				if ( botWorld->gameLocalInfo.friendlyFireOn == false ) {
					if ( botWorld->clientInfo[ i ].team == botInfo->team ) {
						continue; //mal: if FF is off, dont worry about teammate airstrikes - ALWAYS worry about our own tho!
					}
				}
			}

			const airstrikeInfo_t& airstrikeInfo = botWorld->clientInfo[ i ].weapInfo.airStrikeInfo;

			if ( airstrikeInfo.entNum == 0 && airstrikeInfo.timeTilStrike < botWorld->gameLocalInfo.time ) {
				continue;
			}

			if ( Bot_IsAwareOfDanger( airstrikeInfo.oldEntNum, airstrikeInfo.origin, airstrikeInfo.dir ) ) {
		        continue;
			}

			if ( airstrikeInfo.xySpeed > 500.0f && i != botNum ) {
				continue;
			}

			vec = airstrikeInfo.origin - botInfo->origin;
			dist = vec.LengthSqr();

			if ( dist > Square( 1024.0f ) ) { //mal: too far away to see.
				continue;
			}

			if ( dist > Square( 512.0f ) || botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
		        if ( !InFrontOfClient( botNum, airstrikeInfo.origin ) ) {
					continue;
				}
			}

			vec = airstrikeInfo.origin;

			vec[ 2 ] += 8.0f; //mal: raise it up a bit, so we can see it on uneven surfaces.

			botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, vec, MASK_SHOT_BOUNDINGBOX | CONTENTS_PROJECTILE, GetGameEntity( botNum ) );

			if ( tr.fraction < 1.0f && tr.c.entityNum != airstrikeInfo.oldEntNum ) { //mal: its not visible to us, so dont worry about it.
				continue;
			}
				
			if ( AddDangerToAwareList( airstrikeInfo.oldEntNum, airstrikeInfo.origin, THROWN_AIRSTRIKE, 10, i, airstrikeInfo.dir ) != -1 ) {
		        airStrikeDangers++;
			}

			botThreadData.Printf( "Bot #%i is aware of an airstrike!\n", botNum );
		}
	}

	if ( LocationVis2Sky( botInfo->origin ) ) { //mal: arty only dangerous if we're outside.
		for( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( !ClientIsValid( i, -1 ) ) {
				continue;
			}

			if ( inCombat ) {
				continue;
			} //mal: 11 and a half hour change: don't avoid certain dangers in combat, as that causes the jittering issues - 4/16/08

			if ( i != botNum ) {
				if ( botWorld->gameLocalInfo.friendlyFireOn == false ) {
					if ( botWorld->clientInfo[ i ].team == botInfo->team ) {
						continue; //mal: if FF is off, dont worry about teammate arty strike - ALWAYS worry about our own tho!
					}
				}
			}

			if ( botWorld->clientInfo[ i ].weapInfo.artyAttackInfo.deathTime < botWorld->gameLocalInfo.time ) {
				continue;
			}

			if ( currentArtyDanger.time > botWorld->gameLocalInfo.time ) {
				continue;
			}

			vec = botWorld->clientInfo[ i ].weapInfo.artyAttackInfo.origin - botInfo->origin;

			if ( vec.LengthSqr() > Square( botWorld->clientInfo[ i ].weapInfo.artyAttackInfo.radius ) ) {
				continue;
			}

			vec = botWorld->clientInfo[ i ].weapInfo.artyAttackInfo.origin;

			vec[ 2 ] += 64.0f;

			botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, vec, MASK_EXPLOSIONSOLID, GetGameEntity( botNum ) );

			if ( tr.fraction < 1.0f ) { //mal: its not visible to us, so dont worry about it.
				continue;
			}

			currentArtyDanger.time = botWorld->clientInfo[ i ].weapInfo.artyAttackInfo.deathTime;
			currentArtyDanger.origin = botWorld->clientInfo[ i ].weapInfo.artyAttackInfo.origin;
			currentArtyDanger.num = i;

			botThreadData.Printf( "Bot #%i is aware of an arty danger!\n", botNum );
		}
	}

	if ( botVehicleInfo == NULL ) { //mal: vehicles will need their own way to target these things!

		float attackDist = ATTACK_APT_DIST;

		if ( ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo ) || ( botInfo->classType == FIELDOPS && ClassWeaponCharged( AIRCAN ) && Bot_HasWorkingDeployable() ) ) {
			attackDist = WEAPON_LOCK_DIST;
		} //mal: these guys can safely target the deployable from a long ways away.

		for( i = 0; i < MAX_DEPLOYABLES; i++ ) {

			const deployableInfo_t& deployable = botWorld->deployableInfo[ i ];


			if ( deployable.entNum == 0 ) {
				continue;
			}

			if ( Client_IsCriticalForCurrentObj( botNum, 3500.0f ) && ( botInfo->classType != SOLDIER || botInfo->weapInfo.primaryWeapon != ROCKET || botInfo->weapInfo.primaryWeapHasAmmo == false ) ) {
				if ( deployable.disabled ) {
					continue;
				}
			} //mal: if the APT is disabled, and we're on our way to the goal, go ahead and ignore it so we can get the job done.

			if ( deployable.health <= 0 ) {
				continue;
			}
	
			if ( !deployable.inPlace ) {
				continue;
			}

			if ( deployable.team == botInfo->team ) {
				continue;
			}

			if ( aiState == NBG && nbgType == HACK_DEPLOYABLE && nbgTarget == deployable.entNum ) {
				continue;
			}

			if ( deployable.type != APT ) { //mal: we'll handle taking out strategic deployables elsewhere.
				continue;
			}

			if ( botInfo->isDisguised ) {
				continue;
			}

			if ( Bot_IsAwareOfDanger( deployable.entNum, deployable.origin, mat3_identity ) ) {
			   continue;
			}

			vec = deployable.origin - botInfo->origin;
			float distToDeployableSqr = vec.LengthSqr();

			if ( distToDeployableSqr > Square( attackDist ) ) {
				continue;
			}

			if ( inCombat ) {
				if ( deployable.enemyEntNum == botNum && distToDeployableSqr < Square( ATTACK_APT_DIST ) ) {
					turretDangerEntNum = deployable.entNum;
				}
			}

			vec = deployable.origin;
			vec.z += DEPLOYABLE_ORIGIN_OFFSET;
			
			botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, vec, MASK_SHOT_BOUNDINGBOX | MASK_VEHICLESOLID | CONTENTS_FORCEFIELD, GetGameEntity( botNum ) );

			if ( tr.fraction < 1.0f && tr.c.entityNum != deployable.entNum ) { //mal: its not visible to us, so dont worry about it.
				continue;
			}
			
			if ( AddDangerToAwareList( deployable.entNum, deployable.origin, ANTI_PERSONAL, 10, i ) != -1 ) {
				turretDangers++;
			}

			botThreadData.Printf( "Bot #%i is aware of an anti personal danger!\n", botNum );
		}
	}

	float flyerHiveSightDist = maxDist;
	float flyerHiveHearDist = 512.0f;

	if ( botInfo->team == GDF && botWorld->gameLocalInfo.gameMap == VOLCANO ) { //mal: killing hives on Volcano is a priority.
		flyerHiveSightDist = 2048.0f;
		flyerHiveHearDist = 1024.0f;
	}

//mal: next, look for flyer hives and 3rd eye cameras.
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

        if ( botWorld->clientInfo[ i ].team == botInfo->team ) {
			continue; 
		}
		
		if ( inCombat ) {
			continue;
		} //mal: 11 and a half hour change: don't avoid certain dangers in combat, as that causes the jittering issues - 4/16/08
		
		const covertToolInfo_t& covertToolInfo = botWorld->clientInfo[ i ].weapInfo.covertToolInfo;

		if ( covertToolInfo.entNum == 0 ) {
			continue;
		}

		if ( Bot_IsAwareOfDanger( covertToolInfo.entNum, covertToolInfo.origin, mat3_identity ) ) {
            continue;
		}

		vec = covertToolInfo.origin - botInfo->origin;
		dist = vec.LengthSqr();

		if ( dist > Square( flyerHiveSightDist ) ) { //mal: too far away to see.
			continue;
		}

		if ( botInfo->team == STROGG ) {
			if ( !botInfo->weapInfo.hasNadeAmmo ) { //mal: shooting the 3rd eye is pointless.
				continue;
			}

			if ( inCombat ) {
				continue;
			}

            if ( !InFrontOfClient( botNum, covertToolInfo.origin, true ) ) {
				continue;
			}
		} else {
			if ( dist > Square( flyerHiveHearDist ) || inCombat || botWorld->gameLocalInfo.botSkill == BOT_SKILL_EXPERT ) {
				if ( !InFrontOfClient( botNum, covertToolInfo.origin ) ) {
					continue;
				} // high skill bots, not in combat, will "hear" the hive moving near them, if its close.
			}
		}

		vec = covertToolInfo.origin;

	    botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, vec, MASK_SHOT_BOUNDINGBOX | MASK_SHOT_RENDERMODEL, GetGameEntity( botNum ) );

		if ( tr.fraction < 1.0f && tr.c.entityNum != covertToolInfo.entNum ) { //mal: its not visible to us, so dont worry about it.
			continue;
		}
				
		if ( AddDangerToAwareList( covertToolInfo.entNum, covertToolInfo.origin, ( botInfo->team == STROGG ) ? THIRD_EYE_CAM : STROGG_HIVE, 30, i ) != -1 ) {
            covertToolDangers++;
		}

		botThreadData.Printf( "Bot #%i is aware of an covert tool danger!\n", botNum );
	}

//mal: check if there is an empty MCP in the world - we'll consider it a danger and try to kill it!
	if ( botInfo->team == STROGG ) {
        if ( botWorld->botGoalInfo.mapHasMCPGoal && botWorld->botGoalInfo.botGoal_MCP_VehicleNum > MAX_CLIENTS ) {
			GetVehicleInfo( botWorld->botGoalInfo.botGoal_MCP_VehicleNum, vehicleInfo );
			if ( !vehicleInfo.isImmobilized ) {
				vec = vehicleInfo.origin - botInfo->origin;
				if ( vec.LengthSqr() < Square( 3000.0f ) ) {
					mcpDanger = true;
				}
			}
		}
	}

//mal: now, look for charges
	for( i = 0; i < MAX_CLIENT_CHARGES; i++ ) {
		if ( inCombat ) {
			continue;
		} //mal: 11 and a half hour change: don't avoid certain dangers in combat, as that causes the jittering issues - 4/16/08

		const plantedChargeInfo_t& charge = botWorld->chargeInfo[ i ];

		if ( charge.entNum == 0 ) {
			continue;
		}
		
		if ( botWorld->gameLocalInfo.friendlyFireOn == false ) {
			if ( charge.team == botInfo->team && charge.ownerSpawnID != botInfo->spawnID ) {
				continue;
			}
		} //mal: if FF is off, dont worry about teammate charges - ALWAYS worry about our own tho!

		if ( botInfo->classType == ENGINEER && charge.team != botInfo->team ) {
			continue;
		}
					
		if ( Bot_IsAwareOfDanger( charge.entNum, charge.origin, mat3_identity ) ) {
			continue;
		}

		if ( charge.state != BOMB_ARMED ) {
			continue;
		}

		if ( charge.explodeTime > botWorld->gameLocalInfo.time ) {
			continue; //mal: bomb isn't a danger yet.
		}

		vec = charge.origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( 1024.0f ) ) {
			continue;
		}

		botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, charge.origin, MASK_SHOT_BOUNDINGBOX | CONTENTS_PROJECTILE, GetGameEntity( botNum ) );

		if ( tr.fraction < 1.0f && tr.c.entityNum != charge.entNum ) { //mal: its not visible to us, so dont worry about it.
			continue;
		}

		if ( AddDangerToAwareList( charge.entNum, charge.origin, PLANTED_CHARGE, TIME_BEFORE_CHARGE_BLOWS_AWARENESS_TIME, charge.ownerEntNum ) != -1 ) {
			bombDangers++;
		}

		botThreadData.Printf( "Bot #%i is aware of a bomb!\n", botNum );
	}

//mal: mines come last, because they are static.
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( inCombat ) {
			continue;
		} //mal: 11 and a half hour change: don't avoid certain dangers in combat, as that causes the jittering issues - 4/16/08
		
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		if ( inCombat || !Bot_HasExplosives( inCombat ) ) {
			continue;
		}

		if ( botInfo->isDisguised ) { 
			continue;
		}

		if ( botWorld->clientInfo[ i ].team == botInfo->team ) {
			continue;
		}

		for( j = 0; j < MAX_MINES; j++ ) {
			const plantedMineInfo_t& mineInfo = botWorld->clientInfo[ i ].weapInfo.landMines[ j ];

			if ( mineInfo.entNum == 0 ) {
				continue;
			}

			if ( Bot_IsAwareOfDanger( mineInfo.entNum, vec3_zero, mat3_identity ) ) {
				continue;
			}

			if ( mineInfo.xySpeed > 150.0f ) { //mal: dont worry about mines moving thru the air
				continue;
			}

			if ( mineInfo.state != BOMB_ARMED ) { //mal: mine isn't armed yet, so poses no danger to us.
				continue;
			}

			if ( mineInfo.spotted == false ) { //mal: mine isn't visible to us, so ignore it.
				continue;
			}

			vec = mineInfo.origin - botInfo->origin;

			dist = vec.LengthSqr();

			if ( dist > Square( MINE_TOO_FAR_DIST ) ) { //mal: if the mine if far enough away, dont worry about it!
				continue;
			}

			if ( dist < Square( MINE_TOO_CLOSE_DIST ) ) { //mal: if we're super close to the mine, and didn't see it til now, its too late to react now.
				continue;
			}

			//mal: while we're here, check if our current action is within the radius of this mine, which may make it impossible to complete.
			if ( Bot_CheckActionIsValid( actionNum ) ) {
				vec = mineInfo.origin - botThreadData.botActions[ actionNum ]->GetActionOrigin();
				if ( vec.LengthSqr() < Square( 512.0f ) ) {
					actionNumInsideDanger = actionNum;
				} 

				if ( !Bot_HasExplosives( inCombat ) ) { //mal: if the mine is on top of our action and we dont have the means to destroy it, we don't have any choice but to try to go to our goal.
					continue;
				}
			}

			int matesInArea = ClientsInArea( botNum, mineInfo.origin, MINE_TOO_CLOSE_DIST, botInfo->team, NOCLASS, false, false, false, false, false );

			if ( matesInArea > 0 ) { //mal: dont target a mine if a teammate is already near it, might end up killing him.
				continue;
			}

			inFront = InFrontOfClient( botNum, mineInfo.origin );

			if ( !inFront ) {
				if ( mineInfo.spotted == false ) { //mal: if its behind us, and not vis ( thus not showing up on our cmd map ) ignore it.
					continue;
				} else if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) { //mal: if we're just not too bright, ignore it.
                    continue;
				}
			} else {
				vec = mineInfo.origin;

				vec.z += 8.0f; //mal: raise it up a bit, so we can see it on uneven surfaces.

				botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, vec, MASK_EXPLOSIONSOLID, GetGameEntity( botNum ) );

				if ( tr.fraction < 1.0f ) { //mal: its not visible to us, so dont worry about it.
					continue;
				}

				if ( AddDangerToAwareList( mineInfo.entNum, mineInfo.origin + idVec3( 0.0f, 0.0f, 8.0f ), PLANTED_LANDMINE, 4, i ) != -1 ) {
                    mineDangers++;
				}

				botThreadData.Printf( "Bot #%i is aware of a landmine!\n", botNum );
			}
		}
	}

	if ( inCombat ) {
		if ( turretDangers > 0 && botVehicleInfo == NULL ) {
			combatDangerExists = false; /*true;*/ //mal: 11 and a half hour change: don't avoid certain dangers in combat, as that causes the jittering issues - 4/16/08
			turretDangerExists = true;
		}

		if ( nadeDangers > 0 || airStrikeDangers > 0 || bombDangers > 0 || mineDangers > 0 || covertToolDangers > 0 || stroyBombDangers > 0 || currentArtyDanger.time > botWorld->gameLocalInfo.time ) {
            combatDangerExists = false; /*true;*/ //mal: 11 and a half hour change: don't avoid certain dangers in combat, as that causes the jittering issues - 4/16/08
		} else {
			combatDangerExists = false;
		}
		return;
	}

	if ( aiState == NBG && ( nbgType == AVOID_DANGER || nbgType == DESTROY_DEPLOYABLE ) ) { //mal: we're already trying to avoid/destroy dangers! Keep track of them, but dont keep resetting the bots node.
		return;
	}

	if ( nadeDangers > 0 || airStrikeDangers > 0 || bombDangers > 0 || mineDangers > 0 || covertToolDangers > 0 || turretDangers > 0 || stroyBombDangers > 0 || currentArtyDanger.time > botWorld->gameLocalInfo.time ) {
        dangerIndex = FindClosestDangerIndex();

		if ( dangerIndex == -1 ) {
			return;
		}

//mal: dynamic dangers get priority
		if ( currentDangers[ dangerIndex ].type == HAND_GRENADE || currentDangers[ dangerIndex ].type == PLANTED_CHARGE || currentDangers[ dangerIndex ].type == THROWN_AIRSTRIKE || currentDangers[ dangerIndex ].type == STROY_BOMB_DANGER ) {
			ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
			nbgTarget = dangerIndex;
	
			if ( currentDangers[ dangerIndex ].type == THROWN_AIRSTRIKE ) {
				nbgTime = botWorld->gameLocalInfo.time + 15000;
			} else if ( currentDangers[ dangerIndex ].type == PLANTED_CHARGE ) {
				nbgTime = nbgTime = botWorld->gameLocalInfo.time + 11000;
			} else {
				nbgTime = botWorld->gameLocalInfo.time + 5000;
			}

			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_AvoidDanger;
			return;
		}

		if ( turretDangers > 0 ) {
			if ( Bot_HasExplosives( false ) ) {
				ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
				nbgTarget = currentDangers [ dangerIndex ].num;
				nbgTargetType = DEPLOYABLE; //mal: this is a deployable danger.
				NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_DestroyAPTDanger; //mal: APT specific function to destroy them.
				return;
			}
		}

		if ( currentDangers[ dangerIndex ].type == PLANTED_LANDMINE || currentDangers[ dangerIndex ].type == STROGG_HIVE || currentDangers[ dangerIndex ].type == THIRD_EYE_CAM ) {
			if ( Bot_HasExplosives( false ) ) {
				ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
				nbgTarget = dangerIndex;

				if ( currentDangers[ dangerIndex ].type == PLANTED_LANDMINE ) {
                    nbgTargetType = LANDMINE_DANGER; //mal: this is a landmine.
				} else if ( currentDangers[ dangerIndex ].type == STROGG_HIVE ) {
					nbgTargetType = STROGG_HIVE_DANGER; //mal: this is a landmine.
				} else if ( currentDangers[ dangerIndex ].type == THIRD_EYE_CAM ) {
					nbgTargetType = GDF_CAM_DANGER; //mal: this is a landmine.
				}

				NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_DestroyDanger; //mal: generic function to destroy dangers
				return;
			}
		}
	}

	if ( mcpDanger == true ) { //mal_TODO: make it where engs can destroy with nade launcher, and Cvops can destroy with hives.
		if ( Bot_HasExplosives( false ) || ( botInfo->team == STROGG && botInfo->classType == MEDIC ) || ( botInfo->team == GDF && botInfo->classType == FIELDOPS ) ) {
			nbgTargetType = MCP_DANGER;
			ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_DestroyMCP;
			return;
		}
	}

	return;
}

/*
==================
idBotAI::ClearDangerFromDangerList
==================
*/
void idBotAI::ClearDangerFromDangerList( int dangerIndex, bool isEntNum ) {
	int i;

	if ( !isEntNum ) {
		currentDangers[ dangerIndex ].num = 0;
		currentDangers[ dangerIndex ].time = 0;
	} else {
		for( i = 0; i < MAX_DANGERS; i++ ) {
			if ( currentDangers[ i ].num != dangerIndex ) {
				continue;
			}

			currentDangers[ i ].num = 0;
			currentDangers[ i ].time = 0;
			break;
		}
	}
}

/*
==================
idBotAI::FindClosestDangerIndex
==================
*/
int idBotAI::FindClosestDangerIndex() {
	int i;
	int bestIndex = -1;
	float dist;
	float bestDist = idMath::INFINITY; //mal: set it to an insane number.
	idVec3 vec;
    
	for( i = 0; i < MAX_DANGERS; i++ ) {

		if ( currentDangers[ i ].num == 0 ) {
			continue;
		}

		if ( currentDangers[ i ].type != THROWN_AIRSTRIKE ) { //mal: airstrikes only get a time when theyre about to strike.
            if ( currentDangers[ i ].time < botWorld->gameLocalInfo.time ) {
				continue;
			}
		}

		vec = currentDangers[ i ].origin - botInfo->origin;
		dist = vec.LengthSqr();

		if ( currentDangers[ i ].type == ANTI_PERSONAL ) { //mal: need to give a bit of priority to these, since they are so dangerous.
			dist /= 2.0f;

			if ( dist <= 0.0f ) {
				dist = Square( 100.0f );
			}
		}

		if ( dist < bestDist ) {
			bestDist = dist;
			bestIndex = i;
		}
	}

	return bestIndex;
}

/*
==================
idBotAI::FindMineInWorld

Look for a mine out there to arm/disarm.
==================
*/
void idBotAI::FindMineInWorld( plantedMineInfo_t& mineInfo ) {
	int matesInArea;
	float dist;
	float closest = idMath::INFINITY;
	idVec3 vec;
	mineInfo.entNum = 0; //mal: clear this out, so we can tell if we got one or not later.

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

        if ( !ClientIsValid( i, -1 ) ) { //mal: if client is kicked, disconnected, etc....
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.team != botInfo->team ) {
            continue;
		}

		for( int j = 0; j < MAX_MINES; j++ ) {
			if ( playerInfo.weapInfo.landMines[ j ].entNum == 0 ) {
                continue;
			}

            if ( playerInfo.weapInfo.landMines[ j ].state == BOMB_ARMED ) {
				continue;
			}

			if ( playerInfo.weapInfo.landMines[ j ].selfArming ) {
				continue;
			}

			vec = playerInfo.weapInfo.landMines[ j ].origin - botInfo->origin;

			dist = vec.LengthSqr();

			if ( dist > Square( 1000.0f ) ) { //mal: this mine is too far away from us.
				continue;
			}

			matesInArea = ClientsInArea( botNum, playerInfo.weapInfo.landMines[ j ].origin, PLIERS_RANGE / 2, botInfo->team, ENGINEER, false, false, false, false, true );

			if ( matesInArea > 0 ) {
				continue;
			}

			if ( dist < closest ) { //mal: find the closest mine, whether its ours or not.
				mineInfo = playerInfo.weapInfo.landMines[ j ];
				closest = dist;
			}
		}
	}
}

/*
==================
idBotAI::NumPlayerMines
==================
*/
int idBotAI::NumPlayerMines() {

	int i;
	int m = 0;

	for( i = 0; i < MAX_MINES; i++ ) {
		if ( botInfo->weapInfo.landMines[ i ].entNum == 0 ) {
			continue;
		}

		if ( botInfo->weapInfo.landMines[ i ].state != BOMB_ARMED ) {
			continue;
		}

		m++;
	}

	return m;
}

/*
================
idBotAI::DangerStillExists
================
*/
bool idBotAI::DangerStillExists( int entNum, int ownerClientNum ) {

	bool dangerExists = false;
	int i;

	for( i = 0; i < MAX_CLIENT_CHARGES; i++ ) {
		if ( botWorld->chargeInfo[ i ].entNum == 0 ) {
			continue;
		}

		if ( botWorld->chargeInfo[ i ].entNum != entNum ) {
			continue;
		}

		if ( botWorld->chargeInfo[ i ].state == BOMB_NULL ) {
			continue;
		}

		dangerExists = true;
		break;
	}

	if ( dangerExists ) {
		return true;
	}

	for( i = 0; i < MAX_DEPLOYABLES; i++ ) {
		if ( botWorld->deployableInfo[ i ].entNum == 0 ) {
			continue;
		}

		if ( botWorld->deployableInfo[ i ].entNum != entNum ) {
			continue;
		}

		dangerExists = true;
		break;
	}

	if ( dangerExists ) {
		return true;
	}

	for( i = 0; i < MAX_STROYBOMBS; i++ ) {
		if ( botWorld->stroyBombs[ i ].entNum == 0 ) {
			continue;
		}

		if ( botWorld->stroyBombs[ i ].entNum != entNum ) {
			continue;
		}

		dangerExists = true;
		break;
	}

	if ( dangerExists ) {
		return true;
	}

//mal: from here on down, check dangers attached to a particular player.

	if ( !ClientIsValid( ownerClientNum, -1 ) ) {
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ ownerClientNum ];

	for( i = 0; i < MAX_GRENADES; i++ ) {
		if ( playerInfo.weapInfo.grenades[ i ].entNum == 0 ) {
			continue;
		}

		if ( playerInfo.weapInfo.grenades[ i ].entNum != entNum ) {
			continue;
		}

		dangerExists = true;
		break;
	}

	if ( dangerExists ) {
		return true;
	}

	for( i = 0; i < MAX_MINES; i++ ) {
		if ( playerInfo.weapInfo.landMines[ i ].entNum == 0 ) {
			continue;
		}

		if ( playerInfo.weapInfo.landMines[ i ].entNum != entNum ) {
			continue;
		}

		dangerExists = true;
		break;
	}

	if ( dangerExists ) {
		return true;
	}

	if ( playerInfo.weapInfo.airStrikeInfo.timeTilStrike > botWorld->gameLocalInfo.time || playerInfo.weapInfo.airStrikeInfo.entNum != 0 ) {
		dangerExists = true;
	}

	if ( playerInfo.weapInfo.covertToolInfo.entNum != 0 ) {
		dangerExists = true;
	}

	return dangerExists;
}

/*
================
idBotAI::Bot_HasExplosives
================
*/
bool idBotAI::Bot_HasExplosives( bool inCombat, bool makeSureSoldierWeaponReady ) {

	if ( botInfo->weapInfo.hasNadeAmmo ) {
		return true;
	}

	if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo ) {
		if ( makeSureSoldierWeaponReady ) {
			if ( !botInfo->weapInfo.primaryWeapNeedsReload ) {
				return true;
			}
		} else {
			return true;
		}
	}

	if ( botInfo->classType == FIELDOPS && ClassWeaponCharged( AIRCAN ) ) {
		return true;
	}

	if ( !inCombat ) {
#ifndef PACKS_HAVE_NO_NADES
		if ( ( botInfo->classType == MEDIC && botInfo->team == STROGG ) || ( botInfo->classType == FIELDOPS && botInfo->team == GDF ) ) {
			return true;
		} //mal: these classes can resupply themselves to destroy any dangers they may find, so ALWAYS return true.
#endif

		if ( botInfo->classType == COVERTOPS && botInfo->team == GDF && ClassWeaponCharged( THIRD_EYE ) ) {
			return true;
		} //mal: coverts with camera ready can use that to destroy the danger. Strogg hives are totally worthless for this task.
	}

	//mal_FIXME: handle cases where the bot is an eng ( with nade launcher ).

	return false;
}

/*
================
idBotAI::Bot_RunTacticalAction
================
*/
void idBotAI::Bot_RunTacticalAction() {
	if ( !Bot_CheckActionIsValid( tacticalActionNum ) ) {
		return;
	}

	if ( tacticalActionPauseTime > botWorld->gameLocalInfo.time ) { //mal: wait a sec.
		Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
		return;
	}

	idVec3 actionOrg = botThreadData.botActions[ tacticalActionNum ]->GetActionOrigin();
	idVec3 vec = actionOrg - botInfo->origin;

	if ( vec[ 2 ] > 150.0f ) {
        actionOrg[ 2 ] += 65.0f;
	}

	botActionGoals_t actionType = botThreadData.botActions[ tacticalActionNum ]->GetObjForTeam( botInfo->team );

	BotAI_ResetUcmd();
	Bot_LookAtLocation( actionOrg, SMOOTH_TURN );

	if ( actionType == ACTION_AIRCAN_HINT ) { 
		Bot_UseCannister( AIRCAN, actionOrg );
	} else if ( actionType == ACTION_SMOKE_HINT ) {
		Bot_UseCannister( SMOKE_NADE, actionOrg );
	} else if ( actionType == ACTION_NADE_HINT ) {
		botIdealWeapNum = NULL_WEAP;
		botIdealWeapSlot = NADE;
		Bot_ThrowGrenade( actionOrg, true );
	} else if ( actionType == ACTION_SUPPLY_HINT ) {
		Bot_UseCannister( SUPPLY_MARKER, actionOrg );
	} else if ( actionType == ACTION_SHIELD_HINT ) {
		botIdealWeapNum = SHIELD_GUN;
		botIdealWeapSlot = NO_WEAPON;
		if ( botInfo->weapInfo.weapon == SHIELD_GUN ) {
			botUcmd->botCmds.attack = true;
		}
	} else if ( actionType == ACTION_THIRDEYE_HINT ) {
		botIdealWeapNum = THIRD_EYE;
		botIdealWeapSlot = NO_WEAPON;
		if ( botInfo->weapInfo.weapon == THIRD_EYE ) {
			botUcmd->botCmds.attack = true;
		}
	} else if ( actionType == ACTION_TELEPORTER_HINT ) {
		botIdealWeapNum = TELEPORTER;
		botIdealWeapSlot = NO_WEAPON;

		if ( tacticalActionTimer == 0 ) {
			tacticalActionTimer = botWorld->gameLocalInfo.time + 1500;
			tacticalActionTimer2 = tacticalActionTimer + 2500;
		}

		if ( botInfo->weapInfo.weapon == TELEPORTER && tacticalActionTimer < botWorld->gameLocalInfo.time && ClassWeaponCharged( TELEPORTER ) ) {
			botUcmd->botCmds.attack = true;
		}
	} else if ( actionType == ACTION_FLYER_HIVE_HINT ) {
		if ( tacticalActionTimer == 0 && botInfo->weapInfo.covertToolInfo.entNum == 0 ) {
			botIdealWeapNum = FLYER_HIVE;
			botIdealWeapSlot = NO_WEAPON;

			if ( tacticalActionOrigin == vec3_zero ) {
				ResetRandomLook();
				Bot_RandomLook( tacticalActionOrigin, true );
			}

			Bot_LookAtLocation( tacticalActionOrigin, INSTANT_TURN );
			
			if ( botInfo->weapInfo.weapon == FLYER_HIVE && ClassWeaponCharged( FLYER_HIVE ) && botThreadData.random.RandomInt( 100 ) > 50 ) {
				botUcmd->botCmds.attack = true;
			}

			return;
		} else {
			tacticalActionTimer++;
			int enemyNum = Flyer_FindEnemy( FLYER_HIVE_SIGHT_DIST );

			if ( enemyNum == -1 ) {
				int goalAreaNum = botThreadData.botActions[ tacticalActionNum ]->GetActionAreaNum();
				idVec3 goalOrigin = botThreadData.botActions[ tacticalActionNum ]->GetActionOrigin();
				idVec3 vec = goalOrigin - botInfo->weapInfo.covertToolInfo.origin;

				if ( vec.LengthSqr() > Square( botThreadData.botActions[ tacticalActionNum ]->radius ) ) {
					Bot_SetupFlyerMove( goalOrigin, goalAreaNum );
	
					idVec3 moveGoal = botAAS.path.moveGoal;

					moveGoal.z += FLYER_HIVE_HEIGHT_OFFSET;

					Bot_MoveToGoal( moveGoal, vec3_zero, RUN, NULLMOVETYPE );
					Flyer_LookAtLocation( moveGoal );
					return;
				}
			} else {
				clientInfo_t player = botWorld->clientInfo[ enemyNum ];

				int goalAreaNum = player.areaNum;
				idVec3 goalOrigin = player.origin;

				idVec3 vec = goalOrigin - botInfo->weapInfo.covertToolInfo.origin;

				if ( vec.LengthSqr() > Square( FLYER_HIVE_ATTACK_DIST ) ) {
					Bot_SetupFlyerMove( goalOrigin, goalAreaNum );

					idVec3 moveGoal = botAAS.path.moveGoal;

					moveGoal.z += FLYER_HIVE_HEIGHT_OFFSET;

					Bot_MoveToGoal( moveGoal, vec3_zero, RUN, NULLMOVETYPE );
					Flyer_LookAtLocation( moveGoal );
					return;
				} else {
					botUcmd->botCmds.attack = true;
					tacticalActionTime = botWorld->gameLocalInfo.time + 1000;
				}
			}
		}
	}
}

/*
================
idBotAI::TeamHasHuman
================
*/
bool idBotAI::TeamHasHuman( const playerTeamTypes_t playerTeam ) {
    return ( playerTeam == GDF ) ? botWorld->gameLocalInfo.teamGDFHasHuman : botWorld->gameLocalInfo.teamStroggHasHuman;
}

/*
================
idBotAI::NeedsReload
================
*/
bool idBotAI::NeedsReload() {
	if ( botInfo->weapInfo.primaryWeapNeedsReload && botInfo->weapInfo.hasAmmoForReload ) {
		return true;
	}

	return false;
}

/*
================
idBotAI::TeamHumanNearLocation

if range == -1, it becomes a general "does a huamn exist on this team at all" check.
================
*/
bool idBotAI::TeamHumanNearLocation( const playerTeamTypes_t playerTeam, const idVec3 &loc, float range, bool ignorePlayersInVehicle, const playerClassTypes_t playerClass, bool ignoreDeadMates, bool ignoreIfJustLeftAVehicle ) {

	bool hasHuman = ( playerTeam == GDF ) ? botWorld->gameLocalInfo.teamGDFHasHuman : botWorld->gameLocalInfo.teamStroggHasHuman;

	if ( !hasHuman ) { //mal: if theres not even a human on this team ATM, dont worrry about it.
		return false;
	}

	hasHuman = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

        if ( i == botNum ) {
			continue;
		}			
			
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.team != botInfo->team ) {
			continue;
		}

		if ( playerClass != NOCLASS ) {
			if ( playerInfo.classType != playerClass ) {
				continue;
			}
		}

		if ( ignoreIfJustLeftAVehicle ) {
			if ( playerInfo.lastOwnedVehicleTime + IGNORE_IF_JUST_LEFT_VEHICLE_TIME > botWorld->gameLocalInfo.time ) {
				continue;
			}
		}

		if ( ignorePlayersInVehicle == true ) {
			if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: this player is in a vehicle, dont worry about him
				continue;
			}
		}

		if ( ignoreDeadMates == true ) { 
			if ( playerInfo.health <= 0 ) {
				continue;
			}
		}

		if ( playerInfo.isBot ) {
			continue;
		}

		if ( range != -1.0f ) {
			idVec3 vec = playerInfo.origin - loc;

			if ( vec.LengthSqr() > Square( range ) ) {
				continue;
			}
		}

		hasHuman = true;
		break;
	}

	return hasHuman;
}

/*
================
idBotAI::Bot_ShouldUseVehicleForAction
================
*/
bool idBotAI::Bot_ShouldUseVehicleForAction( int actionNumber, bool ignoreArmor ) {
	if ( !Bot_CheckActionIsValid( actionNumber ) ) {
		return false;
	}

	if ( botInfo->isDisguised ) {
		return false;
	}

	if ( botWorld->gameLocalInfo.botsUseVehicles == false ) { //mal: if the admin doesn't want the bots driving around, dont let them.
		return false;
	}

	if ( botThreadData.botActions[ actionNumber ]->GetActionVehicleFlags( botInfo->team ) == NO_VEHICLE ) { //mal: this action doesn't want us to use vehicles.
		return false;
	}

	if ( botVehicleInfo != NULL && ( ( botVehicleInfo->flags & botThreadData.botActions[ actionNumber ]->GetActionVehicleFlags( botInfo->team ) ) || botThreadData.botActions[ actionNumber ]->GetActionVehicleFlags( botInfo->team ) == NULL_VEHICLE_FLAGS ) ) {
		return true;
	}

	if ( botWorld->gameLocalInfo.gameMap == ISLAND && ClientHasObj( botNum ) && botVehicleInfo == NULL ) { //mal: hack.
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			return false;
		}
	}

//mal: if the action is an MCP deliever action, we ALWAYS want to use a vehicle!

	int vehicleNum = -1;
	int vehicleIgnoreFlags = ( ignoreArmor ) ? ARMOR : NULL_VEHICLE_FLAGS;
	float checkDist = ( botWorld->gameLocalInfo.debugPersonalVehicles ) ? 100.0f : 4000.0f;
    idVec3 vec = botThreadData.botActions[ actionNumber ]->GetActionOrigin() - botInfo->origin;

	if ( vec.LengthSqr() > Square( checkDist ) ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNumber ]->GetActionVehicleFlags( botInfo->team ), vehicleIgnoreFlags, true );
	}

	if ( vehicleNum != -1 ) {
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_MeetsVehicleRequirementsForAction
================
*/
bool idBotAI::Bot_MeetsVehicleRequirementsForAction( int actionNumber ) {
	if ( !Bot_CheckActionIsValid( actionNumber ) ) {
		return false;
	}

	if ( !botThreadData.botActions[ actionNumber ]->requiresVehicleType ) {
		return true;
	}

	if ( botThreadData.botActions[ actionNumber ]->GetActionVehicleFlags( botInfo->team ) == NO_VEHICLE ) { //mal: this action doesn't want us to use vehicles - it COULD happen.
		return true;
	}

	if ( botWorld->gameLocalInfo.botsUseVehicles == false ) { //mal: if the admin doesn't want the bots driving around, dont let them.
		return false;
	} //mal: a vehicle only action is no good to the bot if the admin won't let the bot drive. :-(

	if ( botVehicleInfo != NULL && ( ( botVehicleInfo->flags & botThreadData.botActions[ actionNumber ]->GetActionVehicleFlags( botInfo->team ) ) || botThreadData.botActions[ actionNumber ]->GetActionVehicleFlags( botInfo->team ) == NULL_VEHICLE_FLAGS ) ) {
		return true;
	}

	int vehicleNum = -1;
    idVec3 vec = botThreadData.botActions[ actionNumber ]->GetActionOrigin() - botInfo->origin;

	if ( vec.LengthSqr() > Square( 6000.0f ) ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNumber ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );
	}

	if ( vehicleNum != -1 ) {
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_GetVehicle
================
*/
bool idBotAI::Bot_GetIntoVehicle( int vehicleNum ) {
	float dist;
	proxyInfo_t vehicleInfo;
	idVec3 vec;

	GetVehicleInfo( vehicleNum, vehicleInfo );

	if ( vehicleInfo.entNum == 0 || !vehicleInfo.hasFreeSeat ) { //mal: its gone, its moving, or its full - ignore!
 		return false;
	}

	idVec3 vehicleOrigin = vehicleInfo.origin;
	vehicleOrigin.z += VEHICLE_PATH_ORIGIN_OFFSET;

	botUcmd->actionEntityNum = vehicleInfo.entNum;
	botUcmd->actionEntitySpawnID = vehicleInfo.spawnID;

	Bot_SetupMove( vehicleOrigin, -1, ACTION_NULL );

	if ( MoveIsInvalid() ) {
		return false;
	}

	Bot_MoveAlongPath( Bot_ShouldStrafeJump( vehicleOrigin ) );

	vec = vehicleInfo.origin - botInfo->origin;
	dist = vec.LengthSqr();

//mal: if we're fairly close, start sending the vehicle use cmd
	if ( dist < Square( 350.0f ) ) {
		Bot_LookAtLocation( vehicleInfo.origin, SMOOTH_TURN );
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
            botUcmd->botCmds.enterVehicle = true;
		}
	}

    V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
	V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_TravelGoal;
	return true;
}

/*
================
idBotAI::Bot_CheckForNeedyVehicles

Checks for vehicles that need fixing.
================
*/
int idBotAI::Bot_CheckForNeedyVehicles( float range, bool& chatRequest ) {
	bool botIsBusy = false;
	bool botIsImportant = false;
	chatRequest = false;
	int vehicleNum = -1;
	float closest = idMath::INFINITY; //mal: set it to some large, crazy number
	float dist;
	idVec3	vec;
	
	if ( Client_IsCriticalForCurrentObj( botNum, CLOSE_TO_GOAL_RANGE ) ) {
		botIsImportant = true;
	}

	for ( int i = 0; i < MAX_VEHICLES; i++ ) {

		const proxyInfo_t& vehicleInfo = botWorld->vehicleInfo[ i ];

		botIsBusy = false;

		if ( vehicleInfo.entNum == 0 ) {
			continue;
		}

		if ( botIsImportant && vehicleInfo.isEmpty && vehicleInfo.type != MCP ) {
			continue;
		}

		if ( botIsImportant && !vehicleInfo.isEmpty && vehicleInfo.type != MCP ) { //mal: need to focus on doing obj, and leave bot teammates to their own devices.
			if ( vehicleInfo.driverEntNum > -1 && vehicleInfo.driverEntNum < MAX_CLIENTS ) {
				const clientInfo_t& player = botWorld->clientInfo[ vehicleInfo.driverEntNum ];

				if ( player.isBot ) {
					continue;
				}
			} else {
				int gunnerEntNum = Bot_GetVehicleGunnerClientNum( vehicleInfo.entNum );

				if ( gunnerEntNum > -1 && gunnerEntNum < MAX_CLIENTS ) {
					const clientInfo_t& player = botWorld->clientInfo[ gunnerEntNum ];

					if ( player.isBot ) {
						continue;
					}
				}
			}
		}

		if ( vehicleInfo.type == MCP && aiState == LTG && ltgType == FIX_MCP ) {
			continue;
		}

		if ( vehicleInfo.type == MCP && vehicleInfo.health > ( vehicleInfo.maxHealth / 2 ) ) { //mal: get a move on!
			continue;
		}

		if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
			if ( Bot_CheckForHumanInteractingWithEntity( vehicleInfo.entNum ) == true ) {
				botIsBusy = true;
			}
		}

		if ( VehicleIsIgnored( vehicleInfo.entNum ) && vehicleInfo.type != MCP ) {
			botIsBusy = true;
		}

		if ( vehicleInfo.team != botInfo->team ) {
			continue;
		}

		if ( vehicleInfo.xyspeed > WALKING_SPEED ) {
			botIsBusy = true;
		}

		if ( vehicleInfo.health == vehicleInfo.maxHealth ) {
			botIsBusy = true;
		}

		int repairMinHealth = ( vehicleInfo.maxHealth / 2 );

		for( int j = 0; j < MAX_CLIENTS; j++ ) {
			if ( !ClientIsValid( j, -1 ) ) {
				continue;
			}

			const clientInfo_t& player = botWorld->clientInfo[ j ];

			if ( player.proxyInfo.entNum != vehicleInfo.entNum ) {
				continue;
			}

			if ( player.lastChatTime[ NEED_REPAIR ] + 5000 < botWorld->gameLocalInfo.time ) {
				continue;
			}

			if ( vehicleInfo.health < vehicleInfo.maxHealth ) { //mal: vehicle may have been fixed since request - or player is being silly.
				chatRequest = true;
			}

			repairMinHealth = ( int ) ( vehicleInfo.maxHealth / 1.20f );
			break;
		}

		bool markedForRepair = VehicleIsMarkedForRepair( vehicleInfo.entNum, false );

		if ( !chatRequest && !markedForRepair && botIsImportant && vehicleInfo.type != MCP ) {
			botIsBusy = true;
		}

		if ( markedForRepair ) {
			chatRequest = true;
			repairMinHealth = ( int ) ( vehicleInfo.maxHealth / 1.10f );
		}

		if ( vehicleInfo.health > repairMinHealth && vehicleInfo.type != MCP && vehicleInfo.damagedPartsCount <= 0 ) { //mal: its in "good enough" shape, unless its an MCP, or its missing wheels.
			botIsBusy = true;
		}

		if ( !vehicleInfo.inPlayZone ) {
			botIsBusy = true;
		}

		if ( vehicleInfo.isFlipped ) {
			botIsBusy = true;
		}

		int busyClient;

		if ( !Bot_NBGIsAvailable( vehicleInfo.entNum, ACTION_NULL, FIX_VEHICLE, busyClient ) ) {
			continue;
		}

		if ( !vehicleInfo.hasGroundContact && vehicleInfo.type != DESECRATOR ) {
			botIsBusy = true;
		}

		if ( vehicleInfo.neverDriven && vehicleInfo.type != MCP ) { //mal: dont bother with a vehicle that noones driving or cares about. Just get moving!
			continue;
		}

		if ( vehicleInfo.type == MCP && !botWorld->botGoalInfo.mapHasMCPGoal ) {
			continue;
		}

		if ( vehicleInfo.inWater ) {
			botIsBusy = true;
		}

		if ( vehicleInfo.areaNum == 0 ) {
			botIsBusy = true;
		}

		vec = vehicleInfo.origin - botInfo->origin;
		dist = vec.LengthSqr();
		float repairDist = range;

		if ( markedForRepair ) {
			repairDist = 6000.0f;
		}

		if ( dist > Square( repairDist ) ) { //mal: too far away - ignore!
			botIsBusy = true;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, vehicleInfo.origin, travelTime ) ) {
			botIsBusy = true;
		}

		if ( botIsBusy ) {
			if ( chatRequest ) {
				Bot_AddDelayedChat( botNum, CMD_DECLINED, 1 );
				break;
			}
			continue;
		}

		if ( dist < closest ) { //mal: find the closest player in need, and help them first!
			vehicleNum = botWorld->vehicleInfo[ i ].entNum;
			closest = dist;
		}
	}

	return vehicleNum;
}

/*
================
idBotAI::ClientHasVehicleInWorld

Checks if client owns a vehicle in the world.
================
*/
bool idBotAI::ClientHasVehicleInWorld( int clientNum, float range ) {
	
	bool hasVehicle = false;
	int i;
	idVec3	vec;

	for ( i = 0; i < MAX_VEHICLES; i++ ) {
        
		const proxyInfo_t& vehicleInfo = botWorld->vehicleInfo[ i ];

		if ( vehicleInfo.entNum == 0 ) {
			continue;
		}

		if ( vehicleInfo.ownerNum != clientNum ) {
			continue;
		}

		if ( !vehicleInfo.inPlayZone ) {
			continue;
		}

		if ( vehicleInfo.isFlipped ) {
			continue;
		}

		if ( vehicleInfo.type == MCP ) { //mal: noone "owns" an MCP.
			continue;
		}

		if ( vehicleInfo.inWater && !( vehicleInfo.flags & WATER ) ) {
			continue;
		}

		if ( vehicleInfo.areaNum == 0 ) {
			continue;
		}

		if ( vehicleInfo.driverEntNum != clientNum && vehicleInfo.driverEntNum != -1 ) { //mal: someone drove off in our vehicle!
			continue;
		}

		vec = vehicleInfo.origin - botWorld->clientInfo[ clientNum ].origin;

		if ( vec.LengthSqr() > Square( range ) ) { //mal: its too far away to be considered "our" vehicle.
			continue;
		}

		hasVehicle = true;
		break;
	}

	return hasVehicle;
}


/*
================
idBotAI::BodyIsObstructed

Checks if the client's body we are currently targeting is blocked by another clients body, or by a vehicle/deployable.
Used by medics and covert from both teams, to figure if we need to get a better position to steal/revive/spawnhost/etc.
Or if we should skip the body totally.
================
*/
bool idBotAI::BodyIsObstructed( int entNum, bool isCorpse, bool isSpawnHost ) {

	int i;
	idVec3 org = ( isCorpse ) ? botWorld->playerBodies[ entNum ].bodyOrigin : botWorld->clientInfo[ entNum ].origin;
	idVec3 vec;

	if ( isSpawnHost ) {
		org = botWorld->spawnHosts[ entNum ].origin;
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( i == botNum ) { //mal: don't worry about yourself!
			continue;
		}

		if ( i == entNum ) {
			continue;
		}

		if ( botWorld->clientInfo[ i ].health > 0 ) {
			continue;
		}

		vec = botWorld->clientInfo[ i ].origin - org;

		if ( vec.LengthSqr() > 50.0f ) {
			continue;
		}

		return true;
	}

	for ( i = 0; i < MAX_PLAYERBODIES; i++ ) {

		if ( !botWorld->playerBodies[ i ].isValid ) {
            continue; //mal: no body in this client slot!
		}

		if ( isCorpse ) {
			if ( i == entNum ) {
				continue;
			}
		}

		vec = botWorld->playerBodies[ i ].bodyOrigin - org;

		if ( vec.LengthSqr() > 50.0f ) {
			continue;
		}

		return true;
	}

	for ( i = 0; i < MAX_VEHICLES; i++ ) {

		if ( !botWorld->vehicleInfo[ i ].entNum == 0 ) {
            continue; //mal: no vehicle in this client slot!
		}

		vec = botWorld->vehicleInfo[ i ].origin - org;

		if ( vec.LengthSqr() > 500.0f ) {
			continue;
		}

		if ( !botWorld->vehicleInfo[ i ].bbox.ContainsPoint( org ) ) {
			continue;
		}

		return true;
	}

	for( i = 0; i < MAX_DEPLOYABLES; i++ ) {
		if ( botWorld->deployableInfo[ i ].entNum == 0 ) {
			continue;
		}

		vec = botWorld->deployableInfo[ i ].origin - org;

		if ( vec.LengthSqr() > 500.0f ) {
			continue;
		}

		if ( !botWorld->deployableInfo[ i ].bbox.ContainsPoint( org ) ) {
			continue;
		}

		return true;
	}

	return false;
}


/*
================
idBotAI::ClientIsDead

Just a quick check, that takes into account the bot's thread delay time, 
to find out if the client in question died while we were in a wait state.
================
*/
bool idBotAI::ClientIsDead( int clientNum ) {
	if ( !ClientIsValid( clientNum, -1 ) ) {
		return true;
	}

	if ( botWorld->clientInfo[ clientNum ].lastKilledTime + BOT_THINK_DELAY_TIME > botWorld->gameLocalInfo.time || botWorld->clientInfo[ clientNum ].health <= 0 ) {
		return true;
	}
	return false;
}

/*
================
idBotAI::ClientCanBeTKRevived

The bots are more courteous then your average human player. :-P
================
*/
bool idBotAI::ClientCanBeTKRevived( int clientNum ) {
	const clientInfo_t& player = botWorld->clientInfo[ clientNum ];

	if ( player.health > TK_REVIVE_HEALTH ) {
		return false;
	}

	if ( botInfo->team == STROGG ) {
		return false;
	} //mal: cant revive fast enough.
	
	if ( botWorld->gameLocalInfo.botSkill != BOT_SKILL_EXPERT || !botWorld->gameLocalInfo.friendlyFireOn ) { //mal: TK reviving is something only the smartest bots will do
		return false;
	}

	if ( !botWorld->gameLocalInfo.botsUseTKRevive ) {
		return false;
	}

	if ( player.lastAttackerTime + 3000 > botWorld->gameLocalInfo.time ) {
		return false;
	}

	if ( player.lastAttackClientTime + 3000 > botWorld->gameLocalInfo.time ) {
		return false;
	}

	if ( botInfo->lastAttackerTime + 3000 > botWorld->gameLocalInfo.time ) {
		return false;
	}

	if ( enemy != -1 ) {
		return false;
	}

	if ( player.isDisguised ) {
		return false;
	}

	if ( player.weapInfo.isFiringWeap ) {
		return false;
	}

	if ( player.targetLocked ) {
		return false;
	} //mal: if they've locked onto a target, don't deny them the kill

	if ( player.weapInfo.weapon == DEPLOY_TOOL || player.weapInfo.weapon == BINOCS ) {
		return false;
	} //mal: dont tk them if they're trying to deploy something, or they're spotting - that would be confusing.

	if ( player.weapInfo.weapon == PLIERS || player.weapInfo.weapon == HACK_TOOL ) {
		return false;
	} //mal: dont tk them if they're trying to build/arm/hack something - that would be confusing.

	if ( player.weapInfo.weapon == HE_CHARGE || player.weapInfo.weapon == LANDMINE ) {
		return false;
	} //mal: dont tk them if they're trying to plant/mine something - that would be confusing.

	if ( player.enemiesInArea > 1 || botInfo->enemiesInArea > 1 ) {
		return false;
	}

	if ( ClientHasObj( botNum ) ) {
		return false;
	}

	if ( ClientHasObj( clientNum ) ) {
		return false;
	}
    
	return true;
}

/*
================
idBotAI::Bot_CheckForSpawnHostsToDestroy

Look around for spawnhosts to destroy. Will only do this if in the process of a LTG.
================
*/
int idBotAI::Bot_CheckForSpawnHostsToDestroy( float range, bool& useChat ) {
	int i, mates;
	int busyClient;
	int bodyNum = -1;
	float closest = idMath::INFINITY;
	float dist;
	idVec3 vec;
	useChat = false;

	if ( enemy != -1 ) {
		return bodyNum;
	}

	if ( ClientHasObj( botNum ) ) {
		return bodyNum;
	}

	if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) { //mal: this bot is too dumb to do this.
		return bodyNum;
	}

	for( i = 0; i < MAX_SPAWNHOSTS; i++ ) {

		if ( SpawnHostIsIgnored( i ) ) {
			continue;
		}

		if ( botWorld->spawnHosts[ i ].entNum == 0 ) {
			continue; //mal: no spawnhost in this slot!
		}

		if ( botWorld->spawnHosts[ i ].areaNum == 0 ) {
			continue;
		}

//mal: someone else is destroying this spawnhost, so leave it be.
		if ( !Bot_NBGIsAvailable( i, ACTION_NULL, DESTORY_SPAWNHOST, busyClient ) ) {
			continue;
		}

		bool spawnHostIsMarkedForDeath = SpawnHostIsMarkedForDeath( botWorld->spawnHosts[ i ].spawnID );
		
		if ( !spawnHostIsMarkedForDeath ) {
			//mal: should only destroy a spawnhost if its fairly close to the strogg's goal.
			if ( botWorld->botGoalInfo.team_STROGG_PrimaryAction != ACTION_NULL ) {
				vec = botWorld->spawnHosts[ i ].origin - botThreadData.botActions[ botWorld->botGoalInfo.team_STROGG_PrimaryAction ]->GetActionOrigin();
				if ( vec.LengthSqr() > Square( SPAWNHOST_RELEVANT_DIST ) ) { //mal: this is too far away from the obj to matter(?)
					continue;
				}
			}
		}

		vec = botWorld->spawnHosts[ i ].origin - botInfo->origin;

		dist = vec.LengthSqr();

		float tempRange = range;

		if ( spawnHostIsMarkedForDeath ) {
			tempRange = SPAWNHOST_DESTROY_ORDER_RANGE;
		}

		if ( dist > Square( tempRange ) ) { //mal: too far away - ignore!
			continue;
		}

		mates = ClientsInArea( botNum, botWorld->spawnHosts[ i ].origin, 350.0f, botInfo->team , MEDIC, false, false, false, false, true );

		if ( mates >= 1 ) { //mal: maybe the medic near this body is arleady zapping it....
			continue;
		}

		if ( dist < closest ) {
			bodyNum = i;
			closest = dist;

			if ( spawnHostIsMarkedForDeath ) {
				useChat = true;
				break;
			}
		}
	}

	return bodyNum;
}

/*
================
idBotAI::ObjIsOnGround

Just a quick check to see if theres an obj on the ground somewhere.
================
*/
bool idBotAI::ObjIsOnGround() {
	bool onGround = false;
	int i;

	for( i = 0; i < MAX_CARRYABLES; i++ ) {
		if ( botWorld->botGoalInfo.carryableObjs[ i ].onGround && botWorld->botGoalInfo.carryableObjs[ i ].entNum != 0 ) {
			onGround = true;
			break;
		}
	}

	return onGround;
}

/*
================
idBotAI::Bot_CheckForNeedyDeployables

Checks for vehicles that need fixing.
================
*/
int idBotAI::Bot_CheckForNeedyDeployables( float range ) {
	bool botIsBusy = false;
	int i;
	int deployableNum = -1;
	float closest = idMath::INFINITY; //mal: set it to some large, crazy number
	float dist;
	idVec3	vec;

	if ( Client_IsCriticalForCurrentObj( botNum, CLOSE_TO_GOAL_RANGE ) && botWorld->botGoalInfo.attackingTeam == botInfo->team ) {
		botIsBusy = true;
	}
	
	for ( i = 0; i < MAX_DEPLOYABLES; i++ ) {

		if ( DeployableIsIgnored( botWorld->deployableInfo[ i ].entNum ) ) {
			continue;
		}

		const deployableInfo_t& deployableInfo = botWorld->deployableInfo[ i ];

		if ( deployableInfo.entNum == 0 ) {
			continue;
		}

		if ( deployableInfo.areaNum == 0 ) {
			continue;
		}

		if ( deployableInfo.health == 0 && deployableInfo.maxHealth == 0 ) {
			continue;
		}

		if ( deployableInfo.health <= 0 ) {
			continue;
		}

		if ( deployableInfo.ownerClientNum == -1 ) {
			continue;
		}

		if ( !deployableInfo.inPlace ) {
			continue;
		}

		if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
			if ( Bot_CheckForHumanInteractingWithEntity( deployableInfo.entNum ) == true ) {
				continue;
			}

			if ( deployableInfo.type == APT || deployableInfo.type == AVT ) { //mal: don't repair deployables when playing against the human in training mode.
				if ( !TeamHasHuman( botInfo->team ) ) {
					continue;
				}
			}

		}

		int busyClient;

		if ( !Bot_NBGIsAvailable( deployableInfo.entNum, ACTION_NULL, FIX_DEPLOYABLE, busyClient ) ) {
			continue;
		}
		
		if ( botWorld->clientInfo[ deployableInfo.ownerClientNum ].team != botInfo->team ) {
			continue;
		}

		if ( deployableInfo.health == deployableInfo.maxHealth ) {
			continue;
		}

		bool markedForRepair = DeployableIsMarkedForRepair( deployableInfo.entNum, false );

		if ( deployableInfo.health > ( deployableInfo.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) && !markedForRepair ) {
			if ( botIsBusy ) {
				continue;
			}

			if ( deployableInfo.type != APT && deployableInfo.type != AVT ) { //mal: keep turrets up at all times.
				continue;
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EXPERT ) { //mal: make the SP game easier for new players, harder for experts. Favor the offense for the all but the last obj.
				if ( ( deployableInfo.type == AVT || deployableInfo.type == APT ) && !botWorld->botGoalInfo.gameIsOnFinalObjective && botWorld->botGoalInfo.attackingTeam != botInfo->team && botWorld->gameLocalInfo.gameIsBotMatch && TeamHasHuman( botInfo->team ) ) {
					continue;
				}
			} else {
			if ( ( deployableInfo.type == AVT || deployableInfo.type == APT ) && !botWorld->botGoalInfo.gameIsOnFinalObjective && botWorld->gameLocalInfo.gameIsBotMatch && botWorld->botGoalInfo.attackingTeam != botInfo->team ) { //mal: game balancing for SP....
				continue;
			}
		}
		}

		vec = deployableInfo.origin - botInfo->origin;
		dist = vec.LengthSqr();
		float repairDist = FIX_DEPLOYABLE_DIST;

		if ( markedForRepair ) {
			repairDist *= 4.0f;
		}

		if ( dist > Square( repairDist ) ) {
			continue;
		}

		if ( !markedForRepair ) {
			if ( dist > Square( range ) ) { //mal: too far away - ignore!
				continue;
			}
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, deployableInfo.origin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) { 
			deployableNum = botWorld->deployableInfo[ i ].entNum;
			closest = dist;
		}
	}

	return deployableNum;
}

/*
==================
idBotAI::GetDeployableInfo

Returns all the info about a particular deployable.
==================
*/
bool idBotAI::GetDeployableInfo( bool selfDeployable, int entNum, deployableInfo_t& deployableInfo ) {
	bool hasSlot = false;
	deployableInfo.entNum = 0;

	if ( selfDeployable ) {
		for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {

			if ( botWorld->deployableInfo[ i ].entNum == 0 ) {
				continue;
			}	

			if ( botWorld->deployableInfo[ i ].ownerClientNum != botNum ) {
				continue;
			}

			if ( !botWorld->deployableInfo[ i ].inPlace ) {
				continue;
			}

			deployableInfo = botWorld->deployableInfo[ i ];
			hasSlot = true;
			break;
		}
	} else {
		for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
	
			if ( botWorld->deployableInfo[ i ].entNum == 0 ) {
				continue;
			}	

			if ( botWorld->deployableInfo[ i ].entNum != entNum ) {
				continue;
			}

			deployableInfo = botWorld->deployableInfo[ i ];
			hasSlot = true;
			break;
		}
	}

	return hasSlot;
}

/*
==================
idBotAI::DeployableAtAction
==================
*/
bool idBotAI::DeployableAtAction( int actionNumber, bool checkOwnDeployable ) {
	bool hasDeployable = false;
	int i;

	for( i = 0; i < MAX_DEPLOYABLES; i++ ) {

		if ( botWorld->deployableInfo[ i ].entNum == 0 ) {
			continue;
		}

		idVec3 vec = botWorld->deployableInfo[ i ].origin - botThreadData.botActions[ actionNumber ]->GetActionOrigin();
		float dist = vec.LengthSqr();

		if ( dist > Square( botThreadData.botActions[ i ]->GetRadius() ) && dist > Square( 256.0f ) ) {
			continue;
		}

		if ( checkOwnDeployable ) {
			if ( botWorld->deployableInfo[ i ].ownerClientNum != botNum ) {
				hasDeployable = true;
				break;
			}

			if ( botWorld->deployableInfo[ i ].health > ( botWorld->deployableInfo[ i ].maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) {
				hasDeployable = true;
				break;
			}
		} else {
			hasDeployable = true;
			break;
		}
	}

	return hasDeployable;
}

/*
==================
idBotAI::Bot_HasWorkingDeployable
==================
*/
bool idBotAI::Bot_HasWorkingDeployable( bool allowDisabledDeployables, int deployableType ) {
	bool hasDeployable = false;
	int i;

	if ( botWorld->gameLocalInfo.botsUseDeployables == false ) { //mal: admin doesn't want us to use deployables.
		return true;
	}

	for( i = 0; i < MAX_DEPLOYABLES; i++ ) {

		if ( botWorld->deployableInfo[ i ].entNum == 0 ) {
			continue;
		}

		if ( botWorld->deployableInfo[ i ].ownerClientNum != botNum ) {
			continue;
		}

		if ( botWorld->deployableInfo[ i ].health < ( botWorld->deployableInfo[ i ].maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) {
			continue;
		}

		if ( !botWorld->deployableInfo[ i ].inPlace ) {
			continue;
		}

		if ( !allowDisabledDeployables ) {
			if ( botWorld->deployableInfo[ i ].disabled ) {
				continue;
			}
		}

		if ( deployableType != NULL_DEPLOYABLE ) {
			if ( botWorld->deployableInfo[ i ].type != deployableType ) {
				continue;
			}
		}

		hasDeployable = true;
	}

	return hasDeployable;
}

/*
==================
idBotAI::FindMCPStartAction
==================
*/
bool idBotAI::FindMCPStartAction( idVec3& origin ) {

	bool hasMCP = false;
	int i;


	for( i = 0; i < botThreadData.botActions.Num(); i++ ) {
		
		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) != ACTION_MCP_START ) {
			continue;
		}

		origin = botThreadData.botActions[ i ]->GetActionOrigin();
		hasMCP = true;
		break;
	}

	return hasMCP;
}

/*
==================
idBotAI::Bot_GetDeployableTypeForAction
==================
*/
int idBotAI::Bot_GetDeployableTypeForAction( int actionNumber ) {

	bool hasABM = Bot_CheckTeamHasDeployableTypeNearAction( botInfo->team, AIT, actionNumber, DEFAULT_DEPLOYABLE_COVERAGE_RANGE );
	int i;
	int actionDeployableFlags = botThreadData.botActions[ actionNumber ]->GetDeployableType();
	idList< int > options;

	if ( actionDeployableFlags == NULL_DEPLOYABLE ) {
		return NULL_DEPLOYABLE;
	}

	if ( botInfo->classType == COVERTOPS ) { //mal: that was easy!
		return RADAR;
	} else if ( botInfo->classType == FIELDOPS ) {
		if ( actionDeployableFlags == NUKE ) {
			return NUKE;
		} else if ( actionDeployableFlags == ARTILLERY ) {
			return ARTILLERY;
		} else if ( actionDeployableFlags == ROCKET_ARTILLERY ) {
			return ROCKET_ARTILLERY;
		} else {
			if ( actionDeployableFlags & NUKE ) {
				options.Append( NUKE );
			}

			if ( actionDeployableFlags & ARTILLERY ) {
				options.Append( ARTILLERY );
			}

			if ( actionDeployableFlags & ROCKET_ARTILLERY ) {
//				if ( !Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, ROCKET_ARTILLERY, botThreadData.botActions[ actionNumber ]->GetActionOrigin(), idMath::INFINITY ) ) { //mal: Kevin wants to try with always at least 1 rocket arty on the ground.
//					options.SetNum( 0, false );
//				}

				options.Append( ROCKET_ARTILLERY );
			}

			if ( options.Num() == 0 ) {
				assert( false );
				return NULL_DEPLOYABLE;
			}

			i = botThreadData.random.RandomInt( options.Num() );

			return options[ i ];
		}
	} else if ( botInfo->classType == ENGINEER ) {
		if ( actionDeployableFlags == APT ) {
			return APT;
		} else if ( actionDeployableFlags == AVT ) {
			return AVT;
		} else if ( actionDeployableFlags == AIT && !hasABM ) { //mal: only need 1 anti missile!
			return AIT;
		} else {
			if ( actionDeployableFlags & APT ) {
				options.Append( APT );
			}

			if ( actionDeployableFlags & AVT ) {
				options.Append( AVT );
			}

			if ( actionDeployableFlags & AIT ) {
				if ( !hasABM ) {
					options.Append( AIT );
				}
			}

			if ( options.Num() == 0 ) {
				return NULL_DEPLOYABLE;
			}

			i = botThreadData.random.RandomInt( options.Num() );

			return options[ i ];
		}
	}

	return NULL_DEPLOYABLE;
}

/*
==================
idBotAI::Bot_CheckTeamHasDeployableTypeNearAction
==================
*/
bool idBotAI::Bot_CheckTeamHasDeployableTypeNearAction( const playerTeamTypes_t playerTeam, int deployableType, int actionNumber, float dist ) {
	bool hasDeploy = false;
	idVec3 vec;

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
		const deployableInfo_t& deployable = botWorld->deployableInfo[ i ];

		if ( deployable.entNum == 0 ) {
			continue;
		}

		if ( playerTeam != NOTEAM ) {
			if ( deployable.team != playerTeam ) {
				continue;
			}
		}

		if ( deployableType != NULL_DEPLOYABLE ) {
			if ( deployable.type != deployableType ) {
				continue;
			}
		}

		if ( deployable.health < ( deployable.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) {
			continue;
		}

		if ( deployable.ownerClientNum != -1 ) {
			if ( deployable.health == 0 && deployable.maxHealth == 0 ) {
				continue;
			}
		}

		vec = deployable.origin - botThreadData.botActions[ actionNumber ]->GetActionOrigin();

		if ( vec.LengthSqr() > Square( dist ) ) {
			continue;
		}

		hasDeploy = true;
		break;
	}

	return hasDeploy;
}

/*
==================
idBotAI::Bot_CheckTeamHasDeployableTypeNearLocation

If dist == -1.0f, this becomes a general "does team have this deployable at all" check.
==================
*/
bool idBotAI::Bot_CheckTeamHasDeployableTypeNearLocation( const playerTeamTypes_t playerTeam, int deployableType, const idVec3& location, float dist ) {
	bool hasDeploy = false;

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
		const deployableInfo_t& deployable = botWorld->deployableInfo[ i ];

		if ( deployable.entNum == 0 ) {
			continue;
		}

		if ( playerTeam != NOTEAM ) {
			if ( deployable.team != playerTeam ) {
				continue;
			}
		}

		if ( deployableType != NULL_DEPLOYABLE ) {
			if ( deployable.type != deployableType ) {
				continue;
			}
		}

		if ( deployable.health < ( deployable.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) {
			continue;
		}

		if ( deployable.health <= 0 ) {
			continue;
		}

		if ( dist != -1.0f ) {
			idVec3 vec = deployable.origin - location;

			if ( vec.LengthSqr() > Square( dist ) ) {
				continue;
			}
		}

		hasDeploy = true;
		break;
	}

	return hasDeploy;
}

/*
==================
idBotAI::Bot_CheckEnemyHasLockOn
==================
*/
bool idBotAI::Bot_CheckEnemyHasLockOn( int clientNum, bool ignoreTargetIfRangeIsGreat ) {
	if ( clientNum == -1 ) {
		bool enemyHasLockon = false;

		for( int i = 0; i < MAX_CLIENTS; i++ ) {
			if ( !ClientIsValid( i, -1 ) ) {
				continue;
			}

			const clientInfo_t& player = botWorld->clientInfo[ i ];

			if ( player.team == botInfo->team || player.health <= 0 ) {
				continue;
			}

			if ( player.targetLockEntNum == -1 ) {
				continue;
			}

			if ( player.targetLockEntNum != botNum ) {
				if ( player.targetLockEntNum != botInfo->proxyInfo.entNum ) {
					continue;
				}
			}

			if ( ignoreTargetIfRangeIsGreat ) {
				idVec3 vec = player.origin - botInfo->origin;

#ifdef _XENON
				float avoidDist = FLYER_WORRY_ABOUT_ROCKETS_MAX_DIST; //mal: make it a bit easier on the Xbox to take out flyers.
#else
				float avoidDist = 3500.0f;
#endif

				if ( vec.LengthSqr() > Square( avoidDist ) ) {
					continue;
				}
			}

			enemyHasLockon = true;
			break;
		}

		return enemyHasLockon;
	} else {
	if ( botWorld->clientInfo[ clientNum ].targetLockEntNum == -1 ) {
		return false;
	}

	if ( botWorld->clientInfo[ clientNum ].targetLockEntNum != botNum ) {
		if ( botWorld->clientInfo[ clientNum ].targetLockEntNum != botInfo->proxyInfo.entNum ) {
			return false;
		}
	}
	}

	return true;
}

/*
==================
idBotAI::Bot_CheckForSpawnHostToGrab
==================
*/
bool idBotAI::Bot_CheckForSpawnHostToGrab() {

	if ( botInfo->spawnHostEntNum != -1 ) {
		return false;
	}

	if ( !botWorld->gameLocalInfo.botsUseSpawnHosts ) {
		return false;
	}

	if ( Client_IsCriticalForCurrentObj( botNum, -1.0f ) ) {
		return false;
	}

	if ( ClientHasObj( botNum ) ) {
		return false;
	}
	
	int spawnHost = -1;
	int busyClient;
	float closest = idMath::INFINITY;
	idVec3 vec;

	for( int i = 0; i < MAX_SPAWNHOSTS; i++ ) {
		if ( SpawnHostIsIgnored( i ) ) {
			continue;
		}

		if ( botWorld->spawnHosts[ i ].entNum == 0 ) {
			continue;
		}

		if ( botWorld->spawnHosts[ i ].areaNum == 0 ) {
			continue;
		}

		if ( SpawnHostIsUsed( botWorld->spawnHosts[ i ].entNum ) ) {
			continue;
		}

//mal: should only grab a spawnhost if its fairly close to the strogg's goal.
		if ( botWorld->botGoalInfo.team_STROGG_PrimaryAction != ACTION_NULL ) {
			vec = botWorld->spawnHosts[ i ].origin - botThreadData.botActions[ botWorld->botGoalInfo.team_STROGG_PrimaryAction ]->GetActionOrigin();
			if ( vec.LengthSqr() > Square( SPAWNHOST_RELEVANT_DIST ) ) { //mal: this is too far away from the obj to matter(?)
				continue;
			}
		}

		vec = botWorld->spawnHosts[ i ].origin - botInfo->origin;

		float dist = vec.LengthSqr();

		if ( dist > Square( SPAWNHOST_RANGE ) ) { //mal: too far away - ignore!
			continue;
		}

		if ( !Bot_NBGIsAvailable( i, ACTION_NULL, GRAB_SPAWNHOST, busyClient ) ) {
			continue;
		}

		int mates = ClientsInArea( botNum, botWorld->spawnHosts[ i ].origin, 350.0f, botInfo->team , NOCLASS, false, false, false, false, true, true );

		if ( mates > 0 ) { //mal: defer the spawnhost choice to the human
			continue;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, botWorld->spawnHosts[ i ].origin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) {
			spawnHost = i;
			closest = dist;
		}
	}

	if ( spawnHost != -1 ) {
		aiState = NBG;
		nbgTarget = spawnHost;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_GrabSpawnHost;
		return true;
	}

	return false;

}


/*
==================
idBotAI::Bot_HasTeamWoundedInArea
==================
*/
bool idBotAI::Bot_HasTeamWoundedInArea( bool deadOnly, float medicRange ) {
	bool hasWounded = false;
	int playerHealth = ( deadOnly ) ? 0 : 60;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( !player.inGame || player.team == NOTEAM || player.team != botInfo->team ) {
			continue;
		}

		if ( player.health > playerHealth || player.inLimbo ) {
			continue;
		}

		idVec3 vec = player.origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( medicRange ) ) {
			continue;
		}

		hasWounded = true;
		break;
	}

	return hasWounded;
}

/*
==================
idBotAI::ClientHasNadeInWorld

Just a quick and dirty test to see if the client has an active grenade somewhere out there.
==================
*/
bool idBotAI::ClientHasNadeInWorld( int clientNum ) {
	bool hasNade = false;
	const clientInfo_t& client = botWorld->clientInfo[ clientNum ];

	for( int i = 0; i < MAX_GRENADES; i++ ) {
		if ( client.weapInfo.grenades[ i ].entNum == 0 ) {
			continue;
		}

		hasNade = true;
		break;
	}

	return hasNade;
}

/*
==================
idBotAI::PointIsClearOfTeammates
==================
*/
bool idBotAI::PointIsClearOfTeammates( const idVec3& point ) {
	bool isClear = true;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) { //mal: dont scan ourselves.
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.inGame == false || player.team == NOTEAM || player.team != botInfo->team || player.health <= 0 ) {
			continue;
		}

		idVec3 end = player.origin - point;
		float z = end.z;

		if ( idMath::Fabs( z ) > 128.0f ) {
			continue;
		}

		if ( end.LengthSqr() > Square( SAFE_PLAYER_BODY_WIDTH ) ) {
			continue;
		}

		isClear = false;
		break;
	}

	return isClear;
}

/*
==================
idBotAI::EntityIsClient
==================
*/
bool idBotAI::EntityIsClient( int entNum, bool enemyOnly ) {
	if ( entNum > -1 && entNum < MAX_CLIENTS ) {
		if ( enemyOnly ) {
			if ( botWorld->clientInfo[ entNum ].team != botInfo->team ) {
				return true;
			}
		} else {
			return true;
		}
	}

	return false;
}

/*
==================
idBotAI::EntityIsVehicle
==================
*/
bool idBotAI::EntityIsVehicle( int entNum, bool enemyOnly, bool occupiedOnly ) {
	proxyInfo_t vehicleInfo;
	GetVehicleInfo( entNum, vehicleInfo );

	if ( vehicleInfo.entNum == 0 ) {
		return false;
	}

	if ( occupiedOnly ) {
		if ( vehicleInfo.isEmpty ) {
			return false;
		}
	}

	if ( enemyOnly ) {
		if ( vehicleInfo.team == botInfo->team ) {
			return false;
		}
	}
	return true;
}

/*
==================
idBotAI::EntityIsDeployable
==================
*/
bool idBotAI::EntityIsDeployable( int entNum, bool enemyOnly ) {
	deployableInfo_t deployableInfo;

	GetDeployableInfo( false, entNum, deployableInfo );

	if ( deployableInfo.entNum == 0 ) {
		return false;
	}

	if ( enemyOnly ) {
		if ( deployableInfo.team == botInfo->team ) {
			return false;
		}
	}
	return true;
}


/*
==================
idBotAI::Bot_CheckForHumanWantingEscort

Check to see if theres a human out there that wants us to follow them. Even tho we might know we're too busy, still need to check to see if anyone
asked us for a escort, so we can can let them know we heard their request, but are too busy to do so.
==================
*/
bool idBotAI::Bot_CheckForHumanWantingEscort() {
	bool botTooBusy = false;
	bool carrierInWorld = CarrierInWorld();
	bool skipSpeedCheck = false;
	int clientNum = -1;
	int maxEscortClients = 1;
	float escortRange = ESCORT_RANGE;

	if ( botWorld->gameLocalInfo.gameIsBotMatch || botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) { //mal: in SP mode, let the human have more control over the bots, no matter what ( even if the human is wrong ).
		maxEscortClients = 3;
	}

	if ( botVehicleInfo != NULL ) {
		botTooBusy = true;
	}

	if ( botInfo->isActor ) {
		return false;
	}

	int botIsRequestedToEscortClient = Bot_GetRequestedEscortClient(); //mal: first, check to see if someone requested us to escort them using the context menu....

	if ( botVehicleInfo == NULL && botIsRequestedToEscortClient > -1 && botIsRequestedToEscortClient < MAX_CLIENTS && ( aiState != LTG || ( ltgType != FOLLOW_TEAMMATE && ltgType != FOLLOW_TEAMMATE_BY_REQUEST ) || ltgTarget != botIsRequestedToEscortClient ) ) {

		const clientInfo_t& player = botWorld->clientInfo[ botIsRequestedToEscortClient ];

		if ( ClientIsIgnored( botIsRequestedToEscortClient ) ) {
			goto skipRequestedClientCheck;
		}

		if ( Client_IsCriticalForCurrentObj( botNum, TOO_CLOSE_TO_GOAL_TO_FOLLOW_DIST ) || ( ClientHasObj( botNum ) && ClientIsCloseToDeliverObj( botNum, TOO_CLOSE_TO_DELIVER_TO_FOLLOW_DIST ) ) ) {
			Bot_AddDelayedChat( botNum, CMD_DECLINED, 1 );
			Bot_IgnoreClient( botIsRequestedToEscortClient, REQUEST_CONSIDER_TIME );
			return false;
		}

		if ( player.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			proxyInfo_t vehicle;
			GetVehicleInfo( player.proxyInfo.entNum, vehicle );

			if ( vehicle.driverEntNum != botIsRequestedToEscortClient && !VehicleGoalsExistForVehicle( vehicle ) ) { //mal: dont be a driver for a vehicle there are no goals on the map for.
				Bot_AddDelayedChat( botNum, CMD_DECLINED, 1 );
				Bot_IgnoreClient( botIsRequestedToEscortClient, REQUEST_CONSIDER_TIME );
				goto skipRequestedClientCheck;
			}
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, player.origin, travelTime ) ) {
			Bot_AddDelayedChat( botNum, CMD_DECLINED, 1 );
			Bot_IgnoreClient( botIsRequestedToEscortClient, REQUEST_CONSIDER_TIME );
			goto skipRequestedClientCheck;
		}

		idList< int > busyClients;

		int botsFollowingThisClient = Bot_NumClientsDoingLTGGoal( botIsRequestedToEscortClient, ACTION_NULL, FOLLOW_TEAMMATE, busyClients );
		botsFollowingThisClient += Bot_NumClientsDoingLTGGoal( botIsRequestedToEscortClient, ACTION_NULL, FOLLOW_TEAMMATE_BY_REQUEST, busyClients );
		
		if ( botsFollowingThisClient >= maxEscortClients ) { //mal: too many bots following this client ATM - dump one
			int j = botThreadData.random.RandomInt( busyClients.Num() );
			int botToRemoveFromFollowing = busyClients[ j ];
			botThreadData.bots[ botToRemoveFromFollowing ]->ResetBotsAI( true );
		}

		ltgTarget = botIsRequestedToEscortClient;
		ltgTargetSpawnID = botWorld->clientInfo[ botIsRequestedToEscortClient ].spawnID;
		aiState = LTG;
		ltgType = FOLLOW_TEAMMATE_BY_REQUEST;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_FollowMate;
		return true;
	}

skipRequestedClientCheck:

	if ( !botWorld->gameLocalInfo.gameIsBotMatch && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) { //mal: in SP mode, let the human have more control over the bots, no matter what ( even if the human is wrong ).
		if ( Client_IsCriticalForCurrentObj( botNum, 3000.0f ) && botWorld->gameLocalInfo.heroMode == false  ) {
			botTooBusy = true;
		}

		if ( ClientHasObj( botNum ) ) {
			botTooBusy = true;
		}

		if ( botInfo->classType == MEDIC && Bot_HasTeamWoundedInArea( true ) ) {
			botTooBusy = true;
		}

		if ( botInfo->isDisguised ) {
			botTooBusy = true;
		}

		if ( aiState == LTG && ( ltgType != CAMP_GOAL && ltgType != ROAM_GOAL && ltgType != DEFENSE_CAMP_GOAL ) ) {
			botTooBusy = true;
		}

		if ( aiState == NBG && ( nbgType != CAMP && nbgType != HAZE_ENEMY && nbgType != SNIPE ) ) {
			botTooBusy = true;
		}
	} else {
		skipSpeedCheck = true;
		escortRange = ESCORT_RANGE * 2.0f;

		if ( aiState == LTG && ( ltgType == FOLLOW_TEAMMATE || ltgType == FOLLOW_TEAMMATE_BY_REQUEST ) ) {
			if ( ClientIsValid( ltgTarget, ltgTargetSpawnID ) ) {
				if ( !botWorld->clientInfo[ ltgTarget ].isBot ) {
					botTooBusy = true;
				}
			}
		}
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( i == botNum ) { //mal: dont scan ourselves.
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		if ( ClientIsIgnored( i ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		if ( playerInfo.team != botInfo->team ) {
			continue;
		}

		if ( playerInfo.isBot ) {
			continue;
		}

		if ( playerInfo.lastChatTime[ NEED_ESCORT ] + REQUEST_CONSIDER_TIME < botWorld->gameLocalInfo.time ) {
			continue;
		}

		idVec3 vec = playerInfo.origin - botInfo->origin;
		float distSqr = vec.LengthSqr();

		if ( !botTooBusy && distSqr > Square( escortRange ) ) { //mal: they're too far away for us to start following! Let them know tho.
			botTooBusy = true;
		}
		
		if ( botInfo->isDisguised ) {
	        botTooBusy = true;
        }

		if ( !botTooBusy && ( Client_IsCriticalForCurrentObj( botNum, TOO_CLOSE_TO_GOAL_TO_FOLLOW_DIST ) || ( ClientHasObj( botNum ) && ClientIsCloseToDeliverObj( botNum, TOO_CLOSE_TO_DELIVER_TO_FOLLOW_DIST ) ) ) ) {
			botTooBusy = true;
		} //mal: we go thru the trouble of this so that we'll let the player know we're doing busy - instead of just ignoring him ( which frustrates ppl ).

		if ( !botTooBusy && playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			proxyInfo_t vehicleInfo;
            GetVehicleInfo( playerInfo.proxyInfo.entNum, vehicleInfo ); //mal: we may decide to ride along in his vehicle to cause trouble if he has a gunner seat open.

			if ( !VehicleIsValid( vehicleInfo.entNum, skipSpeedCheck, true ) ) { //mal: make sure it has a seat open, and is easy to reach.
				botTooBusy = true;
			}

			if ( vehicleInfo.driverEntNum != i && !VehicleGoalsExistForVehicle( vehicleInfo ) ) { //mal: we can't be the driver of a vehicle if no goals exist on the map for it.
				botTooBusy = true;
			}
			
			if ( !ClientHasObj( botNum ) && carrierInWorld ) {
				continue;
			}
		}

		if ( botTooBusy ) {
			if ( aiState == LTG && ( ltgType == FOLLOW_TEAMMATE || ltgType == FOLLOW_TEAMMATE_BY_REQUEST ) && ltgTarget == i ) {
				break;
			}

			if ( botInfo->isDisguised ) {
				Bot_AddDelayedChat( botNum, IM_DISGUISED, 1 );
			} else {
			Bot_AddDelayedChat( botNum, CMD_DECLINED, 1 );
			}
			Bot_IgnoreClient( i, REQUEST_CONSIDER_TIME );
			break; //mal: only need to do it once, so now leave.
		}

		idList< int > busyClients;

		int botsFollowingThisClient = Bot_NumClientsDoingLTGGoal( i, ACTION_NULL, FOLLOW_TEAMMATE, busyClients );

		botsFollowingThisClient += Bot_NumClientsDoingLTGGoal( i, ACTION_NULL, FOLLOW_TEAMMATE_BY_REQUEST, busyClients );

		if ( botsFollowingThisClient >= maxEscortClients ) {
			continue;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, playerInfo.origin, travelTime ) ) {
			continue;
		}

		clientNum = i;
		break;
	}

	if ( clientNum != -1 ) {
		ltgTarget = clientNum;
		ltgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
		aiState = LTG;
		ltgType = FOLLOW_TEAMMATE_BY_REQUEST;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_FollowMate;
		return true;
	}

	return false;
}


/*
==================
idBotAI::SpawnHostIsUsed
==================
*/
bool idBotAI::SpawnHostIsUsed( int entNum ) {
	bool isUsed = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != STROGG ) {
			continue;
		}

		if ( player.spawnHostEntNum != entNum ) {
			continue;
		}

		isUsed = true;
		break;
	}

	return isUsed;
}

/*
==================
idBotAI::Bot_HasDeployableTargetGoals

Check if the bot should attack enemy deployables.

NOTE: we have code setup to allow the HOG to attack deployables, but it needs some more TLC before its ready for primetime. 
Uncomment the "botVehicleInfo->type != HOG" line below to test.
==================
*/
int idBotAI::Bot_HasDeployableTargetGoals( bool hackDeployable ) {
	bool botIsBigShot = Client_IsCriticalForCurrentObj( botNum, 2500.0f );

	if ( botVehicleInfo == NULL ) {
		if ( botIsBigShot && ( botInfo->classType != SOLDIER || botInfo->weapInfo.primaryWeapon != ROCKET ) ) { //mal: dont go destroy deployables if we're trying to win the game.
			return -1;
		}
		
		if ( ClientHasObj( botNum ) && ( botInfo->classType != SOLDIER || botInfo->weapInfo.primaryWeapon != ROCKET ) ) {
			return -1;
		}
	}

	bool inAttackVehicle = Bot_IsInHeavyAttackVehicle();

	if ( !hackDeployable ) {
		if ( !Bot_HasExplosives( false ) && !inAttackVehicle ) {
			return -1;
		}

		if ( ClientHasFireSupportInWorld( botNum ) && !inAttackVehicle ) {
			return -1;
		}
	}

	bool inVehicle = ( botVehicleInfo != NULL ) ? true : false;
	bool inAirAttackVehicle = ( inVehicle && botVehicleInfo->type > ICARUS ) ? true : false;

	if ( inVehicle && botVehicleInfo->driverEntNum != botNum ) {
		return -1;
	}

	if ( inVehicle && !inAttackVehicle /*&& botVehicleInfo->type != HOG */ && ( !botIsBigShot || !inAirAttackVehicle ) ) { //mal: good idea to skip attacking deployables with weak vehicles?
		return -1;
	}

	if ( botInfo->classType == MEDIC && !inVehicle ) { //mal: not really much a medic can do on foot - they should be out healing ppl!
		return -1;
	}

	idList< int > deployableActions;

	for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {
		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}	

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetBaseObjForTeam( GDF ) != ACTION_DROP_DEPLOYABLE && botThreadData.botActions[ i ]->GetBaseObjForTeam( STROGG ) != ACTION_DROP_DEPLOYABLE && 
			botThreadData.botActions[ i ]->GetBaseObjForTeam( GDF ) != ACTION_DROP_PRIORITY_DEPLOYABLE && botThreadData.botActions[ i ]->GetBaseObjForTeam( STROGG ) != ACTION_DROP_PRIORITY_DEPLOYABLE ) {
			continue;
		}

		deployableActions.Append( i );
	}

	bool hasMCP = ( botWorld->botGoalInfo.botGoal_MCP_VehicleNum != -1 && botWorld->botGoalInfo.mapHasMCPGoal ) ? true : false;

	int targetNum = -1;
	float closest = idMath::INFINITY;

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

		if ( inVehicle ) { //mal: if in a vehicle - attack the APT until its destroyed if we're on the attacking team.
			if ( botInfo->team != botWorld->botGoalInfo.attackingTeam || deployable.type != APT ) {
				if ( deployable.health < ( deployable.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) {
					continue;
				}
			}
		} else {
			if ( deployable.health < ( deployable.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) {
				continue;
			}
		}

		if ( !deployable.inPlace ) {
			continue;
		}

		if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
			if ( Bot_CheckForHumanInteractingWithEntity( deployable.entNum ) == true ) {
				continue;
			}
		}

		if ( hackDeployable || botInfo->classType == COVERTOPS ) { //mal_TODO: remove the 2nd part when get 3rd eye camera attack working!
			if ( deployable.disabled ) {
				continue;
			}
		}

		bool isTargetingUs = ( deployable.enemyEntNum == botNum || ( botVehicleInfo != NULL && deployable.enemyEntNum == botVehicleInfo->entNum ) ) ? true : false;

		if ( botInfo->team == GDF && !botWorld->botGoalInfo.team_GDF_AttackDeployables && !isTargetingUs ) {
			continue;
		}

		if ( botInfo->team == STROGG && !botWorld->botGoalInfo.team_STROGG_AttackDeployables && !isTargetingUs ) {
			continue;
		}

		if ( botWorld->gameLocalInfo.gameMap == SLIPGATE ) { //mal: need some special love for this unique map.
			if ( !Bot_CheckLocationIsOnSameSideOfSlipGate( deployable.origin ) ) {
				continue;
			}
		}

		if ( !inVehicle ) {
			if ( deployable.areaNum == 0 ) {
				continue;
			}
		} else {
			if ( botVehicleInfo->type < ICARUS ) { //mal: air vehicles have no restrictions.
				if ( deployable.areaNumVehicle == 0 ) {
					continue;
				}

				if ( botVehicleInfo->type == HOG ) {
					int travelTime;
					idVec3 deployableOrigin = deployable.origin;
					deployableOrigin.z += DEPLOYABLE_ORIGIN_OFFSET;

					if ( !Bot_LocationIsReachable( true, deployableOrigin, travelTime ) ) {
						continue;
					}
				}
			}
		}

		if ( deployableActions.Num() > 0 && !inVehicle ) {
			int actionNumber = botThreadData.GetDeployableActionNumber( deployableActions, deployable.origin, deployable.team, -1 );

			if ( actionNumber != ACTION_NULL ) {
				if ( botThreadData.botActions[ actionNumber ]->noHack ) { //mal: extended the noHack flag, so that non-coverts on foot wouldn't try to attack them either.
					continue;
				}
			}
		}

		idVec3 vec = deployable.origin - botInfo->origin;
		float dist = vec.LengthSqr();

		int numClients = 1; //mal: by default, most deployables only need 1 client to take them out.

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

//mal: set some kind of priority depending on the deployable type.
		if ( deployable.type == AIT ) {
			if ( botActionValid ) {
				vec = botActionOrg - deployable.origin;
				if ( vec.LengthSqr() < Square( ANTI_MISSILE_RANGE ) ) {
					dist -= Square( 3000.0f ); //mal: becomes a BIG priority, if it guards an area we are interested in.
					numClients = 2;
				}
			} else {
				if ( dist > Square( BOT_ATTACK_DEPLOYABLE_RANGE ) && !inAirAttackVehicle ) {
					continue;
				}
			}
		} else if ( deployable.type == AVT ) {
			if ( hasMCP && botInfo->team == GDF ) {
				if ( deployable.enemyEntNum == botWorld->botGoalInfo.botGoal_MCP_VehicleNum ) {
					dist = 5.0f;
					numClients = 4;
				} else {
					if ( inAirAttackVehicle ) {
						dist = 5.0f;
					} else if ( isTargetingUs ) {
						dist -= Square( 5000.0f );
					} else {
						dist -= Square( 3000.0f ); //mal: AVT get priority in MCP missions, as they're usually what holds up our progress.
					}
					numClients = 3;
				}
			} else {
				if ( botActionValid ) {
					vec = botActionOrg - deployable.origin;

					if ( vec.LengthSqr() > Square( GAME_PLAY_RANGE ) && dist > Square( BOT_ATTACK_DEPLOYABLE_RANGE ) && !inAirAttackVehicle ) {
						continue;
					} else if ( isTargetingUs ) {
						dist -= Square( 5000.0f );
					} else {
						dist -= Square( 3000.0f );
					}
				}
			}
		} else if ( deployable.type == APT ) {
			if ( hackDeployable ) {
				if ( !botInfo->isDisguised ) {
					continue;
				} else {
					if ( botActionValid ) {
						vec = botActionOrg - deployable.origin;

						if ( vec.LengthSqr() < Square( GAME_PLAY_RANGE ) ) {
							dist = 6.0f; //mal: hacking these things when we're disguised is the priority, especially if its guarding our goal!
						}
					}
				}
			} else {
				if ( botWorld->botGoalInfo.attackingTeam == botInfo->team ) {
					if ( botActionValid ) { 
						vec = botActionOrg - deployable.origin;

						if ( vec.LengthSqr() < Square( GAME_PLAY_RANGE ) ) {
							if ( inAirAttackVehicle ) {
								dist = 6.0f;
							} else {
								dist -= Square( 2000.0f );
							}
							numClients = 3;
						} else {
							if ( dist > Square( BOT_ATTACK_DEPLOYABLE_RANGE ) && !inAirAttackVehicle ) {
								continue;
							}
						}
					}
				} else {
					if ( dist > Square( BOT_ATTACK_DEPLOYABLE_RANGE * 2.0f ) && !inAirAttackVehicle ) {
						continue;
					}
				}
			}
		} else if ( deployable.type == RADAR ) { //mal: I REALLY don't want the bots worrying about radar too much, unless theres nothing else out there.
			if ( hackDeployable ) {
				continue;
			}

			if ( botActionValid ) {
				vec = botActionOrg - deployable.origin;
				
				if ( vec.LengthSqr() > Square( BOT_ATTACK_DEPLOYABLE_RANGE * 2.0f ) && dist > Square( BOT_ATTACK_DEPLOYABLE_RANGE ) ) {
					continue;
				} else {
					dist += Square( 9000.0f );
				}
			} else {
				if ( dist > Square( BOT_ATTACK_DEPLOYABLE_RANGE ) && !inAirAttackVehicle ) {
					continue;
				} else {
					dist += Square( 12000.0f );
				}
			}
		} else if ( deployable.type == ARTILLERY || deployable.type == ROCKET_ARTILLERY || deployable.type == NUKE ) {
			if ( !hackDeployable ) {
				if ( dist > Square( BOT_ATTACK_DEPLOYABLE_RANGE ) && !inAirAttackVehicle ) {
					continue;
				}
			}
		}

//		if ( dist <= 1.0f ) {
//			dist = 1.0f;
//		}

		if ( inVehicle ) {
			if ( Bot_VehicleIsUnderAVTAttack() == deployable.entNum && deployable.type == AVT ) {
				dist = 1.0f; //mal: if its attacking us, its RIGHT next to us as far as we're concerned.
			} else {
				if ( !Bot_VehicleLTGIsAvailable( deployable.entNum, ACTION_NULL, V_DESTROY_DEPLOYABLE, numClients ) ) { 
					continue;
				}
			}
		} else {
			if ( !Bot_LTGIsAvailable( deployable.entNum, ACTION_NULL, DESTROY_DEPLOYABLE_GOAL, numClients ) ) { 
				continue;
			}
		}

		if ( DeployableIsMarkedForDeath( deployable.entNum, false ) ) {
			dist = 2.0f; //mal: marked for death, only a deployable attacking us would be more important.
		}

		if ( dist < closest ) {
			if ( !inAirAttackVehicle ) { //mal: assume a flying vehicle can go anywhere.
				int travelTime;
				bool canReach = Bot_LocationIsReachable( false /*inVehicle*/, deployable.origin, travelTime ); //mal: sometimes APTS are in areas vehicles can't reach with AAS, but are reachable in the AAS, and a valid path is possible, so always use the player AAS.

				if ( !canReach ) { //mal: can't reach the target - prolly behind a team specific shield/wall/etc.
					continue;
				}

				if ( inVehicle ) { //mal: see if we can find a node path to our target. If not, could be on a part of the map we don't currently have access to.
					idBotNode* ourNode = botThreadData.botVehicleNodes.GetNearestNode( botVehicleInfo->axis, botVehicleInfo->origin, botInfo->team, NULL_DIR, false, true, false, NULL, botVehicleInfo->flags );
					idBotNode* goalNode = botThreadData.botVehicleNodes.GetNearestNode( mat3_identity, deployable.origin, botInfo->team, NULL_DIR, false, true, false, NULL, botVehicleInfo->flags ); // always assume the goal node is reachable

					if ( ourNode == NULL || goalNode == NULL ) { //mal: crap. :-/
						continue;
					}

					if ( goalNode != ourNode ) {
						idList< idBotNode::botLink_t >	pathList;
						botThreadData.botVehicleNodes.CreateNodePath( botInfo, ourNode, goalNode, pathList, botVehicleInfo->flags );

						if ( pathList.Num() == 0 ) {
							continue;
						}
					}
				}
			}

			targetNum = deployable.entNum;
			closest = dist;
		}
	}

	return targetNum;
}

/*
==================
idBotAI::ClientHasFireSupportInWorld
==================
*/
bool idBotAI::ClientHasFireSupportInWorld( int clientNum ) {

	if ( !ClientIsValid( clientNum, -1 ) ) {
		return false;
	}

	const clientInfo_t& client = botWorld->clientInfo[ clientNum ];

	if ( client.weapInfo.airStrikeInfo.timeTilStrike > botWorld->gameLocalInfo.time || client.weapInfo.airStrikeInfo.entNum != 0 ) {
		return true;
	}

	if ( client.weapInfo.artyAttackInfo.deathTime > botWorld->gameLocalInfo.time ) {
		return true;
	}

	return false;
}

/*
==================
idBotAI::TeamMineInArea
==================
*/
bool idBotAI::TeamMineInArea( const idVec3& org, float range ) {
	bool mineNearby = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( mineNearby ) {
			break;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != botInfo->team ) {
			continue;
		}

		for( int j = 0; j < MAX_MINES; j++ ) {

			if ( player.weapInfo.landMines[ j ].entNum == 0 ) {
				continue;
			}

			idVec3 vec = player.weapInfo.landMines[ j ].origin - org;

			if ( vec.LengthSqr() > Square( range ) ) {
				continue;
			}

			mineNearby = true;
			break;
		}
	}

	return mineNearby;
}

/*
==================
idBotAI::Bot_IsBusy
==================
*/
bool idBotAI::Bot_IsBusy() {
	if ( aiState == NBG && ( nbgType != CAMP && nbgType != SNIPE && nbgType != DEFENSE_CAMP && nbgType != INVESTIGATE_CAMP ) ) {
		return true;
	}

	return false;
}

/*
==================
idBotAI::ClientsNearObj
==================
*/
bool idBotAI::ClientsNearObj( const playerTeamTypes_t& playerTeam ) {

	bool hasEnemy = true;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != playerTeam ) {
			continue;
		}

		if ( LocationDistFromCurrentObj( botInfo->team, player.origin ) > OBJ_AWARENESS_DIST ) {
			continue;
		}

		hasEnemy = true;
		break;
	}

	return hasEnemy;
}

/*
==================
idBotAI::CriticalEnemyClientNearUs
==================
*/
bool idBotAI::CriticalEnemyClientNearUs( const playerClassTypes_t& playerClass ) {

	bool hasEnemy = true;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team == botInfo->team ) {
			continue;
		}

		if ( playerClass != NOCLASS ) {
			if ( player.classType != playerClass ) {
				continue;
			}
		}

		idVec3 vec = player.origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( BOT_AWARENESS_DIST ) ) {
			continue;
		}

		if ( playerClass == SOLDIER ) {
			if ( !ClientHasChargeInWorld( i, true, -1 ) && !Client_IsCriticalForCurrentObj( i, -1.0f ) ) {
				continue;
			}
		} else {
			if ( !Client_IsCriticalForCurrentObj( i, -1.0f ) ) {
				continue;
			}
		}

		hasEnemy = true;
		break;
	}

	return hasEnemy;
}

/*
==================
idBotAI::Bot_CheckNeedClientsOnDefense
==================
*/
bool idBotAI::Bot_CheckNeedClientsOnDefense() {
	int numBots = botThreadData.GetNumBotsOnTeam( botInfo->team );
	int numDefenders = 0;
	int numNeededDefenders;

	if ( numBots == 1 ) { //mal: we need a defender - and its going to be this bot.
		return true; 
	} else {
		numNeededDefenders = ( numBots / 2 );
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( !player.isBot ) {
			continue;
		}

		if ( botThreadData.bots[ i ] == NULL ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetAIState() == LTG && botThreadData.bots[ i ]->GetLTGType() == DEFENSE_CAMP_GOAL ) {
			numDefenders++;
		}

		if ( botThreadData.bots[ i ]->GetAIState() == NBG && botThreadData.bots[ i ]->GetNBGType() == DEFENSE_CAMP ) {
			numDefenders++;
		}
	}

	if ( numDefenders < numNeededDefenders ) {
		return true;
	}

	return false;
}

/*
==================
idBotAI::Bot_EnemyAITInArea

Look for AIT in range of our target. We count base AIT in here as well.
==================
*/
bool idBotAI::Bot_EnemyAITInArea( const idVec3& org ) {
	bool hasAIT = false;
	float range;

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
		const deployableInfo_t& deployable = botWorld->deployableInfo[ i ];

		if ( deployable.entNum == 0 ) {
			continue;
		}

		if ( deployable.team == botInfo->team ) {
			continue;
		}

		if ( deployable.type != AIT ) {
			continue;
		}

		if ( deployable.ownerClientNum != -1 ) { //mal: turrets with an owner of -1 are base turrets - need to count them too!
			if ( deployable.health <= 0 ) {
				continue;
			}

			if ( deployable.health < ( deployable.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) {
				continue;
			}

			if ( !deployable.inPlace ) {
				continue;
			}

			if ( deployable.disabled ) {
				continue;
			}

			range = ANTI_MISSILE_RANGE;
		} else {
			range = BASE_ANTI_MISSILE_RANGE;
		}

		idVec3 vec = deployable.origin - org;

		if ( vec.LengthSqr() > Square( range ) ) {
			continue;
		}

		hasAIT = true;
		break;
	}

	return hasAIT;
}

/*
==================
idBotAI::Bot_CheckActionIsValid
==================
*/
bool idBotAI::Bot_CheckActionIsValid( int actionNumber ) {
	if ( actionNumber <= ACTION_NULL || actionNumber >= botThreadData.botActions.Num() ) {
//		assert( false );
		return false;
	}

	return true;
}

/*
==================
idBotAI::Bot_IsInvestigatingTeamObj
==================
*/
bool idBotAI::Bot_IsInvestigatingTeamObj() {
	if ( aiState != LTG || ltgType != INVESTIGATE_ACTION ) {
		return false;
	}

	return true;
}

/*
==================
idBotAI::Bot_IsDefendingTeamCharge
==================
*/
bool idBotAI::Bot_IsDefendingTeamCharge() {
	if ( aiState != LTG || ltgType != PROTECT_CHARGE ) {
		return false;
	}

	return true;
}

/*
==================
idBotAI::Bot_IsAttackingDeployables
==================
*/
bool idBotAI::Bot_IsAttackingDeployables() {
	if ( botVehicleInfo != NULL ) {
		if ( aiState == VLTG && vLTGType == V_DESTROY_DEPLOYABLE ) {
			return true;
		}
	} else {
		if ( ( aiState == LTG && ltgType == DESTROY_DEPLOYABLE_GOAL ) || ( aiState == NBG && ( nbgType == DESTROY_DEPLOYABLE || nbgType == ENG_ATTACK_AVT ) ) ) {
			return true;
		}
	}

	return false;
}

/*
==================
idBotAI::FindEntityType
==================
*/
const entityTypes_t idBotAI::FindEntityType( int entNum, int spawnID ) {
	if ( entNum < MAX_CLIENTS ) {
		if ( !ClientIsValid( entNum, -1 ) ) {
			return ENTITY_NULL;
		}

		if ( spawnID != -1 ) {
			if ( botWorld->clientInfo[ entNum ].spawnID != spawnID ) {
				return ENTITY_NULL;
			}
		}

		return ENTITY_PLAYER;
	} 

	for( int i = 0; i < MAX_VEHICLES; i++ ) {
		const proxyInfo_t& vehicleInfo = botWorld->vehicleInfo[ i ];

		if ( vehicleInfo.entNum != entNum ) {
			continue;
		}

		if ( spawnID != -1 ) {
			if ( vehicleInfo.spawnID != spawnID ) {
				continue;
			}
		}

		return ENTITY_VEHICLE;
	}

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
		const deployableInfo_t& deployableInfo = botWorld->deployableInfo[ i ];

		if ( deployableInfo.entNum != entNum ) {
			continue;
		}

		if ( spawnID != -1 ) {
			if ( deployableInfo.spawnID != spawnID ) {
				continue;
			}
		}

		return ENTITY_DEPLOYABLE;
	}

	return ENTITY_NULL;
}

/*
================
idBotAI::DeployableIsMarkedForRepair

Checks to see if this deployable has been marked for repair.
================
*/
bool idBotAI::DeployableIsMarkedForRepair( int entNum, bool clearRequest ) {
	deployableInfo_t deployable;

	if ( !GetDeployableInfo( false, entNum, deployable ) ) {
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

		if ( player.repairTargetNum == -1 ) {
			continue;
		}

		if ( player.repairTargetUpdateTime + MAX_TARGET_TIME < botWorld->gameLocalInfo.time ) {
			continue;
		}

		if ( player.repairTargetNum < MAX_CLIENTS ) {
			continue;
		}

		entityTypes_t entityType = FindEntityType( player.repairTargetNum, player.repairTargetSpawnID );

		if ( entityType == ENTITY_NULL || entityType == ENTITY_VEHICLE || entityType == ENTITY_PLAYER ) {
			continue;
		}

		if ( deployable.entNum != player.repairTargetNum ) {
			continue;
		}

		if ( clearRequest ) {
			botUcmd->ackRepairForClient = i;
		}

		isMarked = true;
		break;
	}

	return isMarked;
}

/*
================
idBotAI::VehicleIsMarkedForRepair

Checks to see if this vehicle has been marked for repair.
================
*/
bool idBotAI::VehicleIsMarkedForRepair( int entNum, bool clearRequest ) {
	proxyInfo_t vehicle;
	GetVehicleInfo( entNum, vehicle );

	if ( vehicle.entNum == 0 ) {
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

		if ( player.repairTargetNum == -1 ) {
			continue;
		}

		if ( player.repairTargetUpdateTime + MAX_TARGET_TIME < botWorld->gameLocalInfo.time ) {
			continue;
		}

		if ( player.repairTargetNum < MAX_CLIENTS ) {
			continue;
		}

		entityTypes_t entityType = FindEntityType( player.repairTargetNum, player.repairTargetSpawnID );

		if ( entityType == ENTITY_NULL || entityType == ENTITY_DEPLOYABLE || entityType == ENTITY_PLAYER ) {
			continue;
		}

		if ( vehicle.entNum != player.repairTargetNum ) {
			continue;
		}

		if ( vehicle.health == vehicle.maxHealth ) {
			botUcmd->ackRepairForClient = i;
			continue;
		}

		if ( clearRequest ) {
			botUcmd->ackRepairForClient = i;
		}

		isMarked = true;
		break;
	}

	return isMarked;
}

/*
================
idBotAI::Bot_CheckAdrenaline

Checks to see if the bot should use its adrenaline ability
================
*/
bool idBotAI::Bot_CheckAdrenaline( const idVec3& goalOrigin ) {
	bool canShockSelf = ( botInfo->team == GDF ) ? botInfo->abilities.gdfAdrenaline : botInfo->abilities.stroggAdrenaline;

	if ( !canShockSelf ) {
		return false;
	}

	if ( botInfo->health < ( botInfo->maxHealth - 20 ) ) { //mal: makes more sense for him to just use packs on himself.
		return false;
	}

	if ( botInfo->classChargeUsed > 0 ) {
		return false;
	}

	if ( goalOrigin == vec3_zero ) {
		if ( enemy == -1 && ( ClientHasObj( botNum ) || botInfo->enemiesInArea > 0 || ( botInfo->lastAttackerTime + 1000 ) > botWorld->gameLocalInfo.time ) ) { //mal: keep ourselves topped off with max health, to help turn the tide in battle.
			botIdealWeapNum = NEEDLE;
			botIdealWeapSlot = NO_WEAPON;

			if ( botInfo->weapInfo.weapon == NEEDLE ) {
				botUcmd->botCmds.altFire = true;
				if ( selfShockTime < botWorld->gameLocalInfo.time ) {
					selfShockTime = botWorld->gameLocalInfo.time + 1000;
				}
			}
		}
	} else {
		if ( botInfo->weapInfo.weapon != NEEDLE ) {
			return false;
		}

		if ( botInfo->enemiesInArea > 0 || enemy != -1 || ( botInfo->lastAttackerTime + 1000 ) > botWorld->gameLocalInfo.time ) {
			idVec3 vec = goalOrigin - botInfo->origin;

			if ( vec.LengthSqr() > Square( 100.0f ) ) {
				botUcmd->botCmds.altFire = true;
				if ( selfShockTime < botWorld->gameLocalInfo.time ) {
					selfShockTime = botWorld->gameLocalInfo.time + 1000;
				}
			}

			return true;
		}
	}
	return false;
}

/*
================
idBotAI::Bot_CheckCovertToolState

Checks to see if the bot should use covert tool.
================
*/
bool idBotAI::Bot_CheckCovertToolState() {
	if ( botInfo->team == GDF && botInfo->weapInfo.covertToolInfo.entNum != 0 ) {
		idVec3 vec = botInfo->weapInfo.covertToolInfo.origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( THIRD_EYE_RANGE ) ) {
			return false;
		}

		int enemiesInArea = ClientsInArea( botNum, botInfo->weapInfo.covertToolInfo.origin, 500.0f, STROGG, NOCLASS, false, false, false, false, false );
		int friendsInArea = ClientsInArea( -1, botInfo->weapInfo.covertToolInfo.origin, 300.0f, GDF, NOCLASS, false, false, false, false, false );
		bool deployablesInArea = Bot_CheckTeamHasDeployableTypeNearLocation( STROGG, NULL_DEPLOYABLE, botInfo->weapInfo.covertToolInfo.origin, 700.0f );

		if ( friendsInArea == 0 && ( enemiesInArea > 0 || deployablesInArea ) ) {
			botIdealWeapNum = THIRD_EYE;
			botIdealWeapSlot = NO_WEAPON;

			if ( botInfo->weapInfo.weapon == THIRD_EYE ) {
				botUcmd->botCmds.attack = true;
			}

			return true;
		}
	} // else if we are strogg - have our flyer hive chase down a nearby enemy and blow up on them. TODO!

	return false;
}

/*
================
idBotAI::CurrentActionIsLinkedToAction
================
*/
bool idBotAI::CurrentActionIsLinkedToAction( int curActionNum, int testActionNum ) {
	if ( !Bot_CheckActionIsValid( curActionNum ) || !Bot_CheckActionIsValid( testActionNum ) ) {
		return false;
	}

	if ( botThreadData.botActions[ curActionNum ]->GetActionGroup() != botThreadData.botActions[ testActionNum ]->GetActionGroup() ) {
		return false;
	}

	if ( ( botThreadData.botActions[ curActionNum ]->GetHumanObj() != botThreadData.botActions[ testActionNum ]->GetHumanObj() ) && ( botThreadData.botActions[ curActionNum ]->GetStroggObj() != botThreadData.botActions[ testActionNum ]->GetStroggObj() ) ) {
		return false;
	}

	return true;
}

/*
================
idBotAI::FindLinkedActionsForAction
================
*/
void idBotAI::FindLinkedActionsForAction( int testActionNum, idList< int >& testActionList, idList< int >& linkedActionList ) {
	for( int i = 0; i < testActionList.Num(); i++ ) {
		if ( testActionList[ i ] == testActionNum ) {
			continue;
		}

		if ( !CurrentActionIsLinkedToAction( testActionList[ i ], testActionNum ) ) {
			continue;
		}

		linkedActionList.Append( testActionList[ i ] );
	}
}

/*
================
idBotAI::Bot_CheckLocationIsOnSameSideOfSlipGate
================
*/
bool idBotAI::Bot_CheckLocationIsOnSameSideOfSlipGate( const idVec3& origin ) {
	bool botOnDesertSide = ( botInfo->origin.x < SLIPGATE_DIVIDING_PLANE_X_VALUE );
	bool locOnDesertSide = ( origin.x < SLIPGATE_DIVIDING_PLANE_X_VALUE );

	if ( botOnDesertSide != locOnDesertSide ) {
		return false;
	}

	return true;
}

/*
================
idBotAI::Bot_CheckLocationIsVisible
================
*/
bool idBotAI::Bot_CheckLocationIsVisible( const idVec3& location, int scanEntityNum, int ignoreEntNum, bool scanFromCrouch ) {
	trace_t	tr;

	idVec3 	botViewOrigin = botInfo->origin;
	botViewOrigin.z += ( scanFromCrouch ) ? 0.0f : botWorld->gameLocalInfo.normalViewHeight;

	botThreadData.clip->TracePointExt( CLIP_DEBUG_PARMS tr, botViewOrigin, location, MASK_SHOT_RENDERMODEL | MASK_SHOT_BOUNDINGBOX, GetGameEntity( botNum ), GetGameEntity( ignoreEntNum ) );

	if ( tr.fraction < 1.0f ) {
		if ( scanEntityNum != -1 && tr.c.entityNum == scanEntityNum ) {
			return true;
		}
		return false;
	}

	return true;
}

/*
================
idBotAI::Bot_CheckHasUnArmedMineNearby
================
*/
bool idBotAI::Bot_CheckHasUnArmedMineNearby() {
	bool hasMine = false;

	for( int i = 0; i < MAX_MINES; i++ ) {
		if ( botInfo->weapInfo.landMines[ i ].entNum == 0 ) {
			continue;
		}

		if ( botInfo->weapInfo.landMines[ i ].state == BOMB_ARMED ) {
			continue;
		}

		idVec3 vec = botInfo->weapInfo.landMines[ i ].origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( CLOSE_MINE_DIST ) ) {
			continue;
		}

		hasMine = true;
		break;
	}

	return hasMine;
}

/*
================
idBotAI::Bot_IsNearDroppedObj
================
*/
bool idBotAI::Bot_IsNearDroppedObj() {
	if ( botWorld->gameLocalInfo.inWarmup ) {
		return false;
	}

	if ( botInfo->isActor ) {
		return false;
	}

	if ( Client_IsCriticalForCurrentObj( botNum, -1 ) ) {
		return false;
	}

	if ( aiState == LTG && ( ltgType == DEFUSE_GOAL || ltgType == PLANT_GOAL || ltgType == BUILD_GOAL || ltgType == HACK_GOAL ) ) {
		return false;
	}

	if ( aiState == NBG && ( nbgType == BUILD || nbgType == HACK || nbgType == DEFUSE_BOMB || nbgType == PLANT_BOMB ) ) {
		return false;
	}

	if ( ClientHasObj( botNum ) ) {
		return false;
	}

	if ( aiState == NBG && nbgType == REVIVE_TEAMMATE && botInfo->team == GDF ) { //mal: medics need a chance to finish their job! Strogg take too long to revive.
		return false;
	}

	if ( aiState == LTG && ( ltgType == RECOVER_GOAL || ltgType == STEAL_GOAL ) ) {
		return true;
	}

	if ( botInfo->classType == MEDIC && botInfo->team == GDF && Bot_HasTeamWoundedInArea( true ) ) { //mal: strogg medics take too long to revive to make this worthwhile
		return false;
	}

	int bestObj = -1;
	int vehicleNum;
	float closest = idMath::INFINITY;

	for( int i = 0; i < MAX_CARRYABLES; i++ ) {
		if ( !botWorld->botGoalInfo.carryableObjs[ i ].onGround || botWorld->botGoalInfo.carryableObjs[ i ].entNum == 0 ) {
			continue;
		}

		if ( botWorld->botGoalInfo.carryableObjs[ i ].areaNum == 0 ) {
			continue;
		}

		idVec3 vec = botWorld->botGoalInfo.carryableObjs[ i ].origin - botInfo->origin;
		float dist = vec.LengthSqr();

		if ( dist > Square( BOT_RECOVER_OBJ_RANGE ) ) {
			continue;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, botWorld->botGoalInfo.carryableObjs[ i ].origin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) {
			bestObj = i;
			closest = dist;
		}
	}

	if ( bestObj == -1 ) {
		return false;
	}

	ltgTarget = bestObj;

	idVec3 vec = botWorld->botGoalInfo.carryableObjs[ ltgTarget ].origin - botInfo->origin;

	if ( vec.LengthSqr() > Square( 3000.0f ) && botWorld->gameLocalInfo.botsUseVehicles ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, GROUND | AIR, NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum != -1 ) {
			ltgUseVehicle = true;
		} else {
			ltgUseVehicle = false;
		}
	} else {
		ltgUseVehicle = false;
	}

	if ( ltgUseVehicle ) {
		if ( botVehicleInfo != NULL ) {
			if ( botVehicleInfo->driverEntNum == botNum ) {
				V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
				V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_TravelGoal;
			} else {
				Bot_ExitVehicle();
			}
		}
	} else {
		if ( botVehicleInfo != NULL ) { //mal: if we're close to the goal, or theres some other reason not to use a vehicle for this goal, then exit our vehicle
			Bot_ExitVehicle();
		}
	}

	if ( botWorld->botGoalInfo.carryableObjs[ ltgTarget ].ownerTeam == botInfo->team ) {
		ltgType = RECOVER_GOAL;
	} else {
		ltgType = STEAL_GOAL;
	}

	ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY;
	ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
	LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_RecoverDroppedGoal;

	if ( !ltgUseVehicle ) {
		Bot_FindRouteToCurrentGoal();	
	}
			
	PushAINodeOntoStack( -1, ltgTarget, ACTION_NULL, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
	return true;
}

/*
================
idBotAI::Bot_CheckMountedGPMGState

Make sure the bot leaves the mountable GPMG if he doesn't want to be on it.
================
*/
void idBotAI::Bot_CheckMountedGPMGState() {
	if ( aiState == LTG && botInfo->usingMountedGPMG ) {
		botUcmd->botCmds.exitVehicle = true;
	} else if ( aiState == NBG && botInfo->usingMountedGPMG && nbgType != MG_CAMP ) {
		botUcmd->botCmds.exitVehicle = true;
	}
}

/*
================
idBotAI::Bot_WantsVehicle
================
*/
bool idBotAI::Bot_WantsVehicle() {
	if ( botInfo->wantsVehicle && botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) {
		return true;
	}

	if ( botWantsVehicleBackTime > botWorld->gameLocalInfo.time ) {
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_CheckIfClientHasChargeAsGoal
================
*/
bool idBotAI::Bot_CheckIfClientHasChargeAsGoal( int chargeEntNum ) {
	bool clientsDefusing = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		if ( i == botNum ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( !player.isBot ) {
			continue;
		}

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( player.classType != ENGINEER ) {
			continue;
		}

		if ( botThreadData.bots[ i ] == NULL ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetAIState() != NBG || botThreadData.bots[ i ]->GetNBGType() != DEFUSE_BOMB ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetNBGTarget() != chargeEntNum ) {
			continue;
		}

		clientsDefusing = true;
		break;
	}

	return clientsDefusing;
}


#define BOX_OFFSET 32.0f


/*
================
idBotAI::Bot_IsClearOfObstacle
================
*/
bool idBotAI::Bot_IsClearOfObstacle( const idBox& box, const float bboxHeight, const idVec3& botOrigin ) {
	float minHeight, maxHeight;
	
	box.AxisProjection( botAAS.aas->GetSettings()->invGravityDir, minHeight, maxHeight );

	if ( ( minHeight > ( botOrigin.z + bboxHeight + BOX_OFFSET ) ) || ( maxHeight < ( botOrigin.z - BOX_OFFSET ) ) ) { //mal: dont see obstacles that are a floor above/below the player/vehicle.
		return true;
	}

	return false;
}

/*
================
idBotAI::OtherBotsWantVehicle
================
*/
bool idBotAI::OtherBotsWantVehicle( const proxyInfo_t& vehicle ) {
	bool vehicleIsWanted = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		if ( i == botNum ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( !player.isBot ) {
			continue;
		}

		if ( botThreadData.bots[ i ] == NULL ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetAIState() != LTG || ( botThreadData.bots[ i ]->GetLTGUseVehicle() == false && botThreadData.bots[ i ]->GetLTGType() != GRAB_VEHICLE_GOAL && botThreadData.bots[ i ]->GetLTGType() != ENTER_HEAVY_VEHICLE ) ) {
			continue;
		}

		if ( botThreadData.bots[ i ]->GetLTGTarget() != vehicle.entNum ) {
			continue;
		}

		vehicleIsWanted = true;
		break;
	}

	return vehicleIsWanted;
}

/*
============
idBotAI::GetPlayerViewPosition
============
*/
idVec3 idBotAI::GetPlayerViewPosition( int clientNum ) {
	if ( !ClientIsValid( clientNum, -1 ) ) {
		return vec3_zero;
	}

	const clientInfo_t& player = botWorld->clientInfo[ clientNum ];

	if ( player.isLeaning ) {
		return player.viewOrigin;
	}

	if ( player.isLeaning || ( player.weapInfo.covertToolInfo.entNum != 0 && player.weapInfo.covertToolInfo.clientIsUsing == true ) ) {
		idVec3 vec = player.origin;
		vec.z += 48.0f;

		if ( botThreadData.AllowDebugData() ) { 
			gameRenderWorld->DebugCircle( colorGreen, vec, idVec3( 0, 0, 1 ), 48, 32 );
		}
		return vec;
	}

	return player.viewOrigin;
}

/*
============
idBotAI::Bot_IsNearForwardSpawnToGrab
============
*/
bool idBotAI::Bot_IsNearForwardSpawnToGrab() {
	if ( Bot_WantsVehicle() ) {
		return false;
	}

	if ( botInfo->isActor ) {
		return false;
	}

	if ( ClientHasObj( botNum ) ) {
		return false;
	}

	if ( ( aiState == LTG && ( ltgType == FIX_MCP || ltgType == DRIVE_MCP ) ) || ( aiState == NBG && ( nbgType == ENG_ATTACK_AVT || nbgType == FIXING_MCP ) ) ) { //mal: the MCP has gotta be the priority!
		return false;
	}

	if ( aiState == LTG && ltgType == DEFUSE_GOAL ) { //mal: defusing is a priority!
		return false;
	}

	if ( aiState == NBG && ( nbgType == BUILD || nbgType == HACK || nbgType == DEFUSE_BOMB || nbgType == PLANT_BOMB ) ) {
		return false;
	}

	if ( botWorld->gameLocalInfo.inWarmup ) {
		return false;
	}

	if ( fdaUpdateTime > botWorld->gameLocalInfo.time ) {
		return false;
	}

	if ( aiState == LTG && ( ltgType == FDA_GOAL || ltgType == STEAL_SPAWN_GOAL ) ) {
		return false;
	}

	if ( aiState == NBG && nbgType == GRAB_SPAWN ) {
		return false;
	}

	fdaUpdateTime = botWorld->gameLocalInfo.time + 5000;
	
	int spawnGoal = ACTION_NULL;
	int stealSpawnGoal = ACTION_NULL;

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

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) != ACTION_FORWARD_SPAWN && botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) != ACTION_DENY_SPAWNPOINT ) {
			continue;
		}

		if ( botWorld->botGoalInfo.isTrainingMap ) {
			if ( !botThreadData.actorMissionInfo.forwardSpawnIsAllowed ) {
				continue;
			}
		} else if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
			if ( Bot_CheckForHumanInteractingWithEntity( botThreadData.botActions[ i ]->GetActionSpawnControllerEntNum() ) == true ) {
				continue;
			}

			if ( botWorld->botGoalInfo.isTrainingMap ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_FORWARD_SPAWN ) {
				 if ( !ActionIsActiveForTrainingMode( i, botWorld->gameLocalInfo.botTrainingModeObjDelayTime ) ) { //mal: wait a while before we try to complete the goal, to give the player time to decide what he wants to do.
					 continue;
				 }

				 if ( !TeamHasHuman( botInfo->team ) ) { //mal: dont keep frustrating the human player by taking our spawn back too quick, in case he took it from us.
					continue;
				 }
			}
		}
		
		if ( ActionIsIgnored( i ) ) {
			break;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_FORWARD_SPAWN ) {
			if ( botThreadData.botActions[ i ]->GetTeamOwner() == botInfo->team ) { //mal: it already belongs to us.
				continue;
			}
		} else {
			if ( botThreadData.botActions[ i ]->GetTeamOwner() == botInfo->team || botThreadData.botActions[ i ]->GetTeamOwner() == NOTEAM ) { //mal: it already belongs to us.
				continue;
			}

			if ( botWorld->botGoalInfo.isTrainingMap ) {
				continue;
			}
		}

		bot_LTG_Types_t ltgGoalType;

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_FORWARD_SPAWN ) {
			ltgGoalType = FDA_GOAL;
		} else {
			ltgGoalType = STEAL_SPAWN_GOAL;
		}

		if ( !Bot_LTGIsAvailable( -1, i, ltgGoalType, 1 ) ) {
			break;
		}

		int busyClient;

		if ( !Bot_NBGIsAvailable( -1, i, GRAB_SPAWN, busyClient ) ) {
			break;
		}

		idVec3 distToSpawn = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;

		if ( distToSpawn.LengthSqr() > Square( FDA_AUTO_GRAB_DIST ) ) {
			break;
		}

		int matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 350.0f, botInfo->team, NOCLASS, false, false, false, true, true );

		if ( matesInArea > 0 ) {
			break;
		} //mal: already somebody there, so lets pick a different one to grab.

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_FORWARD_SPAWN ) {
			spawnGoal = i;
		} else {
			stealSpawnGoal = i;
		}
		break;
	}

	if ( spawnGoal != ACTION_NULL ) {
		ltgType = FDA_GOAL;
		actionNum = spawnGoal;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		ltgTime = botWorld->gameLocalInfo.time + 120000;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_SpawnPointGoal;
		Bot_FindRouteToCurrentGoal();	
		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, false, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( stealSpawnGoal != ACTION_NULL ) {
		ltgType = STEAL_SPAWN_GOAL;
		actionNum = stealSpawnGoal;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		ltgTime = botWorld->gameLocalInfo.time + 120000;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_SpawnPointGoal;
		Bot_FindRouteToCurrentGoal();	
		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, false, ( routeNode != NULL ) ? true : false );
		return true;
	}
	return false;
}

/*
============
idBotAI::Bot_GetDeployableOffSet
============
*/
float idBotAI::Bot_GetDeployableOffSet( int deployableType ) {
	if ( deployableType == AVT ) {
		return 0.45f;
	} else if ( deployableType == APT ) {
		return 0.40f;
	} else if ( deployableType == RADAR ) {
		return 0.30f;
	} else if ( deployableType == NUKE ) {
		return 0.40f;
	} else if ( deployableType == AIT ) {
		return 0.40f;
	}

	return 0.60f;
}

/*
============
idBotAI::ClientIsCloseToDeliverObj
============
*/
bool idBotAI::ClientIsCloseToDeliverObj( int clientNum, float desiredRange ) {
	if ( botWorld->botGoalInfo.deliverActionNumber <= ACTION_NULL || botWorld->botGoalInfo.deliverActionNumber >= botThreadData.botActions.Num() ) {
		return false;
	}

	idVec3 vec = botThreadData.botActions[ botWorld->botGoalInfo.deliverActionNumber ]->GetActionOrigin() - botWorld->clientInfo[ clientNum ].origin;

	if ( botWorld->clientInfo[ clientNum ].proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		proxyInfo_t vehicle;
		GetVehicleInfo( botWorld->clientInfo[ clientNum ].proxyInfo.entNum, vehicle );

		if ( vehicle.isAirborneVehicle ) {
			vec.z = 0.0f;
		}

		if ( vehicle.type == PLATYPUS && vehicle.hasGroundContact && vehicle.xyspeed < WALKING_SPEED ) { //mal: get out of the boat if its on the shore!
			return true;
		}
	}

	float tooCloseDist;

	if ( desiredRange == -1.0f ) {
		tooCloseDist = ( botWorld->gameLocalInfo.gameMap == QUARRY ) ? 5000.0f : 3000.0f;
	} else {
		tooCloseDist = desiredRange;
	}

	if ( vec.LengthSqr() < Square( tooCloseDist ) ) {
		return true;
	}

	return false;
}

/*
============
idBotAI::Bot_FindNearbySafeActionToMoveToward

Find a close, valid action for the bot to move towards. It doesn't matter what the action is, we just want the bot to move away from its current position in a nice, realistic manner.
============
*/
int idBotAI::Bot_FindNearbySafeActionToMoveToward( const idVec3& origin, float minAvoidDist, bool pickRandom ) {
	int actionNumber = ACTION_NULL;
	int listStartNum = 0;
	bool skipInactiveActions = true;

	if ( botVehicleInfo != NULL && botVehicleInfo->type > ICARUS ) {
		skipInactiveActions = false;
	}

	if ( pickRandom ) {
		listStartNum = botThreadData.random.RandomInt( botThreadData.botActions.Num() );
	}

	for( int i = listStartNum; i < botThreadData.botActions.Num(); i++ ) {

		if ( skipInactiveActions ) {
			if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
				continue;
			}
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_NULL ) {
			continue;
		}
		
		if ( botWorld->gameLocalInfo.gameMap == SLIPGATE ) {
			if ( !Bot_CheckLocationIsOnSameSideOfSlipGate( botThreadData.botActions[ i ]->GetActionOrigin() ) ) {
				continue;
			}
		}

		if ( botVehicleInfo == NULL && ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_VEHICLE_CAMP || botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_VEHICLE_ROAM ) ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_AIRCAN_HINT || botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_SMOKE_HINT || 
			botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_NADE_HINT || botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_SUPPLY_HINT || 
			botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_SHIELD_HINT || botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_TELEPORTER_HINT || 
			botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_THIRDEYE_HINT || botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_FLYER_HIVE_HINT ||
			botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_FLYER_HIVE_LAUNCH || botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_FLYER_HIVE_TARGET ) {
			continue;
		}

		if ( i == lastNearbySafeActionToMoveTowardActionNum ) { //mal: try not to repeat ourselves.
			continue;
		}

		idVec3 vec = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;

		if ( vec.LengthSqr() < Square( minAvoidDist ) ) {
			continue;
		}

		actionNumber = i;
		lastNearbySafeActionToMoveTowardActionNum = actionNumber;
		break;
	}

	return actionNumber;
}

/*
============
idBotAI::ClientHasShieldInWorld

if  considerRange != -1, the bots won't count shields that are outside the considerRange from their position.
============
*/
bool idBotAI::ClientHasShieldInWorld( int clientNum, float considerRange ) {
	if ( !ClientIsValid( clientNum, -1 ) ) {
		return false;
	}

	bool hasShield = false;
	const clientInfo_t& player = botWorld->clientInfo[ clientNum ];

	if ( player.classType != FIELDOPS ) {
		return false;
	}

	for( int i = 0; i < MAX_SHIELDS; i++ ) {
		if ( player.forceShields[ i ].entNum == 0 ) {
			continue;
		}

		if ( considerRange != -1.0f ) {
			idVec3 vec = player.forceShields[ i ].origin - botInfo->origin;

			if ( vec.LengthSqr() > Square( considerRange ) ) {
				continue;
			}
		}

		hasShield = true;
		break;
	}

	return hasShield;
}

/*
============
idBotAI::Bot_DistSqrToClosestForceShield
============
*/
float idBotAI::Bot_DistSqrToClosestForceShield( idVec3& shieldOrg ) {
	float closest = idMath::INFINITY;
	shieldOrg = vec3_zero;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}
		
		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != STROGG ) {
			continue;
		}

		if ( player.classType != FIELDOPS ) {
			continue;
		}

		for( int j = 0; j < MAX_SHIELDS; j++ ) {
			if ( player.forceShields[ j ].entNum == 0 ) {
				continue;
			}

			idVec3 vec = player.forceShields[ j ].origin - botInfo->origin;
			float distSqr = vec.LengthSqr();

			if ( distSqr > Square( SHIELD_CONSIDER_RANGE ) ) {
				continue;
			}

			if ( distSqr < closest ) {
				shieldOrg = player.forceShields[ j ].origin;
				closest = distSqr;
			}
		}
	}

	return closest;
}

/*
============
idBotAI::Bot_UseTeleporter
============
*/
bool idBotAI::Bot_UseTeleporter() {
	if ( botInfo->team == STROGG && botInfo->classType == COVERTOPS && botInfo->hasTeleporterInWorld && !botInfo->isDisguised && botTeleporterAttempts < 30 ) {
		botIdealWeapNum = TELEPORTER;
		botIdealWeapSlot = NO_WEAPON;
		ignoreWeapChange = true;
		if ( botInfo->weapInfo.weapon == TELEPORTER ) {
			botTeleporterAttempts++;
			if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
				botUcmd->botCmds.attack = true;
			}
		}
		return true;
	}
	return false;
}

/*
============
idBotAI::Bot_UseRepairDrone
============
*/
bool idBotAI::Bot_UseRepairDrone( int entNum, const idVec3& entOrg ) {
	if ( botInfo->team != STROGG || !botInfo->abilities.stroggRepairDrone || ( botInfo->weapInfo.primaryWeapNeedsAmmo && !botInfo->hasRepairDroneInWorld ) || ( !botInfo->weapInfo.primaryWeapHasAmmo && !botInfo->hasRepairDroneInWorld ) ) {
		return false;
	}

	idVec3 vec = entOrg - botInfo->origin;

	if ( vec.LengthSqr() < Square( REPAIR_DRONE_RANGE ) ) {
		if ( !botInfo->hasRepairDroneInWorld ) {
			trace_t tr;
			botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, entOrg, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ) ); 

			if ( tr.fraction < 1.0f && tr.c.entityNum != entNum ) {
				return false;
			}

			botIdealWeapNum = PLIERS;
			botIdealWeapSlot = NO_WEAPON;

			if ( botInfo->weapInfo.weapon == PLIERS && botInfo->targetLocked && botInfo->targetLockEntNum == entNum ) {
				botUcmd->botCmds.altFire = true;
			}

			Bot_LookAtEntity( entNum, SMOOTH_TURN );
		} else {
			botIdealWeapSlot = GUN;

			if ( Bot_RandomLook( vec ) )  {
				Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
			}
		}
		return true;
	}

	return false;
}

/*
============
idBotAI::Bot_IsUnderAttackFromUnknownEnemy

Bot is under attack from someone, and has had time to react to that enemy, but hasn't - so must not be aware of them for some reason.
============
*/
bool idBotAI::Bot_IsUnderAttackFromUnknownEnemy() {
	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
		return false;
	}

	if ( botInfo->lastAttackerTime + 1000 > botWorld->gameLocalInfo.time && botInfo->lastAttackerTime + 100 < botWorld->gameLocalInfo.time && enemy == -1 ) {
		return true;
	}

	return false;
}

/*
============
idBotAI::Bot_CheckForGrieferTargetGoals

Check to see if the bots are being camped/griefed by some dirty human, and make taking out that human a goal for the bot.
============
*/
int idBotAI::Bot_CheckForGrieferTargetGoals() {
	if ( ClientHasObj( botNum ) || Client_IsCriticalForCurrentObj( botNum, 2500.0f ) ) {
		return -1;
	}

	if ( botVehicleInfo != NULL ) {
		if ( !Bot_IsInHeavyAttackVehicle() ) {
			return -1;
		}

		if ( Bot_VehicleIsUnderAVTAttack() != -1 ) {
			return -1;
		}
	}

	bool inAirAttackVehicle = ( botVehicleInfo != NULL && botVehicleInfo->type > ICARUS ) ? true : false;
	int grieferClientNum = -1;
	int closestAirVehicle = FindClosestVehicle( HUNT_MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, AIR, PERSONAL | AIR_TRANSPORT, true );
	float attackDist = ( inAirAttackVehicle || closestAirVehicle != -1 ) ? idMath::INFINITY : MAX_ATTACK_GRIEFER_RANGE;
	float closest = idMath::INFINITY;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.isBot ) {
			continue;
		}

		if ( player.health <= 0 ) {
			continue;
		}

		if ( player.team == botInfo->team ) {
			continue;
		}

		if ( !player.isCamper && player.killsSinceSpawn < KILLING_SPREE ) {
			continue;
		}

		if ( ClientIsIgnored( i ) ) {
			continue;
		}

		//mal: check to see if anyone else is targeting this bozo.
		if ( botVehicleInfo != NULL ) {
			if ( !Bot_VehicleLTGIsAvailable( i, ACTION_NULL, V_HUNT_GOAL, 1 ) ) { 
				continue;
			}
		} else {
			if ( !Bot_LTGIsAvailable( i, ACTION_NULL, HUNT_GOAL, 1 ) ) { 
				continue;
			}
		}

		if ( botWorld->gameLocalInfo.gameMap == SLIPGATE ) { //mal: need some special love for this unique map.
			if ( !Bot_CheckLocationIsOnSameSideOfSlipGate( player.origin ) ) {
				continue;
			}
		}

		if ( player.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			if ( botVehicleInfo == NULL && !Bot_CanAttackVehicles() ) {
				continue;
			}

			if ( player.areaNumVehicle == 0 ) {
				continue;
			}
		} else {
			if ( player.areaNum == 0 ) {
				continue;
			}
		}

		idVec3 vec = player.origin - botInfo->origin;
		float distSqr = vec.LengthSqr();

		if ( distSqr > Square( attackDist ) ) {
			continue;
		}

		if ( distSqr > closest ) {
			continue;
		}

		if ( botVehicleInfo == NULL ) {
			if ( closestAirVehicle == -1 ) {
				int travelTime;

				if ( !Bot_LocationIsReachable( false, player.origin, travelTime ) ) {
					continue;
				}
			}
		} else if ( !inAirAttackVehicle ) { //mal: aircraft can go anywhere.
			idBotNode* ourNode = botThreadData.botVehicleNodes.GetNearestNode( botVehicleInfo->axis, botVehicleInfo->origin, botInfo->team, NULL_DIR, false, true, false, NULL, botVehicleInfo->flags );
			idBotNode* goalNode = botThreadData.botVehicleNodes.GetNearestNode( mat3_identity, player.origin, botInfo->team, NULL_DIR, false, true, false, NULL, botVehicleInfo->flags ); // always assume the goal node is reachable

			if ( ourNode == NULL || goalNode == NULL ) { //mal: crap. :-/
				continue;
			}

			if ( goalNode != ourNode ) {
				idList< idBotNode::botLink_t >	pathList;
				botThreadData.botVehicleNodes.CreateNodePath( botInfo, ourNode, goalNode, pathList, botVehicleInfo->flags );

				if ( pathList.Num() == 0 ) {
					continue;
				}
			}
		}

		grieferClientNum = i;
		closest = distSqr;
	}

	return grieferClientNum;
}

/*
============
idBotAI::Bot_FindClosestAVTDanger
============
*/
int idBotAI::Bot_FindClosestAVTDanger( const idVec3& location, float range, bool useAngleCheck ) {
	int deployableEntNum = -1;
	float closest = idMath::INFINITY;

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
		const deployableInfo_t& deployable = botWorld->deployableInfo[ i ];

		if ( deployable.entNum == 0 ) {
			continue;
		}

		if ( deployable.type != AVT ) {
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

		if ( deployable.disabled ) {
			continue;
		}
	
		if ( useAngleCheck ) {
			idVec3 dir = location - deployable.origin;
			dir.NormalizeFast();
			if ( dir * deployable.axis[ 0 ] < COS_TURRET_ANGLE_ARC ) {
				continue;
			}
		}

		idVec3 vec = location - deployable.origin;
		float distSqr = vec.LengthSqr();

		if ( distSqr > Square( range ) ) {
			continue;
		}

		if ( distSqr < closest ) {
			deployableEntNum = deployable.entNum;
			closest = distSqr;
		}
	}

	return deployableEntNum;
}

/*
============
idBotAI::SpawnHostIsMarkedForDeath
============
*/
bool idBotAI::SpawnHostIsMarkedForDeath( int spawnHostSpawnID ) {
	bool isMarked = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.spawnHostTargetSpawnID != spawnHostSpawnID ) {
			continue;
		}

		isMarked = true;
		break;
	}

	return isMarked;
}

/*
============
idBotAI::Bot_GetRequestedEscortClient
============
*/
int idBotAI::Bot_GetRequestedEscortClient() { 
	int escortClientNum = -1;
	
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.escortSpawnID == botInfo->spawnID && player.escortRequestTime + REQUEST_CONSIDER_TIME > botWorld->gameLocalInfo.time ) {
			escortClientNum = i;
			break;
		}
	}

	return escortClientNum;
}

/*
============
idBotAI::ActionIsActiveForTrainingMode
============
*/
bool idBotAI::ActionIsActiveForTrainingMode( int actionNumber, int numSecondsToDelay ) {
	if ( botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
		return true;
	}

	numSecondsToDelay *= 1000;

	if ( ( botThreadData.botActions[ actionNumber ]->actionActivateTime + numSecondsToDelay ) < botWorld->gameLocalInfo.time ) {
		return true;
	}

	return false;
}

/*
============
idBotAI::TeamHumanMissionIsObjective
============
*/
bool idBotAI::TeamHumanMissionIsObjective() {
	if ( botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
		return false;
	}

	bool humanHasObjMission = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( botNum == i ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.isBot ) {
			continue;
		}

		if ( player.missionEntNum != MISSION_OBJ ) {
			continue;
		}

		humanHasObjMission = true;
		break;
	}

	return humanHasObjMission;
}

/*
============
idBotAI::Bot_CheckForHumanInteractingWithEntity
============
*/
bool idBotAI::Bot_CheckForHumanInteractingWithEntity( int entNum ) {
	bool hasHuman = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( i == botNum ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.isBot ) {
			continue;
		}

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( player.missionEntNum != entNum ) {
			continue;
		}

		hasHuman = true;
		break;
	}

	return hasHuman;
}

/*
============
idBotAI::Bot_HasTeammateWhoCouldUseShieldCoverNearby
============
*/
int idBotAI::Bot_HasTeammateWhoCouldUseShieldCoverNearby() {
	int entNum = -1;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		
		if ( i == botNum ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		if ( ClientIsIgnored( i ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( player.health <= 0 ) {
			continue;
		}

		if ( player.xySpeed > WALKING_SPEED ) {
			continue;
		}

		if ( player.weapInfo.weapon != PLIERS && player.weapInfo.weapon != NEEDLE && player.weapInfo.weapon != HACK_TOOL && player.weapInfo.weapon != HE_CHARGE ) {
			continue;
		}

		idVec3 vec = player.origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( SHEILD_FIRE_CONSIDER_RANGE ) ) {
			continue;
		}

		if ( ClientHasCloseShieldNearby( i, SHEILD_FIRE_CONSIDER_RANGE ) ) {
			continue;
		}

		entNum = i;
		break;
	}

	return entNum;
}

/*
============
idBotAI::Bot_CheckForNearbyTeammateWhoCouldUseSmoke
============
*/
int idBotAI::Bot_CheckForNearbyTeammateWhoCouldUseSmoke() {
	if ( botInfo->isDisguised ) {
		return -1;
	}

	if ( ClientHasObj( botNum ) ) {
		return -1;
	}

	if ( !ClassWeaponCharged( SMOKE_NADE ) ) {
		return -1;
	}

	int entNum = -1;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		
		if ( i == botNum ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		if ( ClientIsIgnored( i ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( player.classType != ENGINEER ) {
			continue;
		}
		
		if ( player.inWater ) {
			continue;
		}

		if ( player.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: client jumped into a vehicle/deployable - forget them
			continue;
		}
		
		if ( ClientIsDead( i ) || player.inLimbo ) {
			continue;
		}

		if ( player.xySpeed > WALKING_SPEED ) {
			continue;
		}

		if ( player.weapInfo.weapon != PLIERS ) {
			continue;
		}

		idVec3 vec = player.origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( SMOKE_CONSIDER_RANGE ) ) {
			continue;
		}

		entNum = i;
		break;
	}

	return entNum;
}

/*
============
idBotAI::ClientHasCloseShieldNearby
============
*/
bool idBotAI::ClientHasCloseShieldNearby( int clientNum, float considerRange ) {
	bool hasShield = false;
	const clientInfo_t& client = botWorld->clientInfo[ clientNum ];

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}
		
		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( player.team != STROGG ) {
			continue;
		}

		if ( player.classType != FIELDOPS ) {
			continue;
		}

		for( int j = 0; j < MAX_SHIELDS; j++ ) {
			if ( player.forceShields[ j ].entNum == 0 ) {
				continue;
			}

			idVec3 vec = player.forceShields[ j ].origin - client.origin;

			if ( vec.LengthSqr() > Square( considerRange ) ) {
				continue;
			}

			hasShield = true;
			break;
		}
	}

	return hasShield;
}

/*
============
idBotAI::TeamCriticalClass
============
*/
const playerClassTypes_t idBotAI::TeamCriticalClass( const playerTeamTypes_t playerTeam ) {
	if ( playerTeam == GDF ) {
		return botWorld->botGoalInfo.team_GDF_criticalClass;
	} else if ( playerTeam == STROGG ) {
		return botWorld->botGoalInfo.team_STROGG_criticalClass;
	} else {
		return NOCLASS;
	}
}

/*
============
idBotAI::FindHumanOnTeam
============
*/

int idBotAI::FindHumanOnTeam( const playerTeamTypes_t playerTeam ) {
	int clientNum = -1;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.isBot ) {
			continue;
		}

		if ( playerInfo.team != playerTeam ) {
			continue;
		}

		clientNum = i;
		break;
	}

	return clientNum;
}

/*
============
idBotAI::Bot_CheckHealthCrateState
============
*/
void idBotAI::Bot_CheckHealthCrateState() {
	if ( !ClassWeaponCharged( SUPPLY_MARKER ) ) {
		return;
	}

	if ( crateGoodTime > botWorld->gameLocalInfo.time ) {
		idVec3 botOrigin = botInfo->origin;
		botOrigin.z -= 64.0f;
		Bot_UseCannister( SUPPLY_MARKER, botOrigin );
		return;
	}

	int botActionNum = botWorld->botGoalInfo.team_GDF_PrimaryAction;

	if ( botActionNum < 0 || botActionNum > botThreadData.botActions.Num() ) {
		return;
	}

	if ( botInfo->supplyCrate.entNum != 0 ) { //mal: our old crate is too far away from the obj, so just nuke it and drop a new one.
		idVec3 crateDelta = botInfo->supplyCrate.origin - botThreadData.botActions[ botActionNum ]->GetActionOrigin();

		if ( crateDelta.LengthSqr() < Square( 5000.0f ) ) {
			return;
		}

		botUcmd->botCmds.destroySupplyCrate = true;
		return;
	}

	idVec3 actionDelta = botThreadData.botActions[ botActionNum ]->GetActionOrigin() - botInfo->origin;

	float maxRange = ( botWorld->botGoalInfo.attackingTeam == botInfo->team ) ? 3000.0f : 1200.0f;

	if ( actionDelta.LengthSqr() > Square( maxRange ) ) {
		return;
	}

	if ( !LocationVis2Sky( botInfo->origin ) ) {
		return;
	}

	if ( Bot_CheckIfHealthCrateInArea( 1024.0f ) ) {
		return;
	}
	
	if ( botThreadData.botVehicleNodes.ActiveVehicleNodeNearby( botInfo->origin, 640.0f ) ) { //mal: dont drop crates in the middle of bot defined roads.
		return;
	}

	if ( Bot_CheckIfObstacleInArea( 200.0f ) ) {
		return;
	}

	float testDist = 100.0f;
	float maxSlope = 0.93f;

 //mal: make sure we're not dropping it into some tight area that ppl can't path thru. Need to check all 4 directions, to make sure dont end up in a doorway.
	if ( !Bot_CanMove( RIGHT, testDist, false, true ) ) {
		return;
	}

	if ( ( botCanMoveGoal * botAAS.aas->GetSettings()->invGravityDir ) < maxSlope ) {
		return;
	}

	if ( !Bot_CanMove( LEFT, testDist, false, true ) ) {
		return;
	}

	if ( ( botCanMoveGoal * botAAS.aas->GetSettings()->invGravityDir ) < maxSlope ) {
		return;
	}

	if ( !Bot_CanMove( FORWARD, testDist, false, true ) ) {
		return;
	}

	if ( ( botCanMoveGoal * botAAS.aas->GetSettings()->invGravityDir ) < maxSlope ) {
		return;
	}

	if ( !Bot_CanMove( BACK, testDist, false, true ) ) {
		return;
	}

	if ( ( botCanMoveGoal * botAAS.aas->GetSettings()->invGravityDir ) < maxSlope ) {
		return;
	}

	idVec3 botOrigin = botInfo->origin;

	botOrigin.z -= 64.0f;

	Bot_UseCannister( SUPPLY_MARKER, botOrigin );

	crateGoodTime = botWorld->gameLocalInfo.time + 5000;
}

/*
============
idBotAI::Bot_CheckIfHealthCrateInArea
============
*/
bool idBotAI::Bot_CheckIfHealthCrateInArea( float range ) {
	bool crateInArea = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}
		
		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.supplyCrateRequestTime > botWorld->gameLocalInfo.time && botInfo->supplyCrateRequestTime < botWorld->gameLocalInfo.time ) {
			crateInArea = true;
			break;
		}

		if ( playerInfo.supplyCrate.entNum == 0 ) {
			continue;
		}

		idVec3 vec = playerInfo.supplyCrate.origin - botInfo->origin;

		if ( vec.LengthSqr() > Square( range ) ) {
			continue;
		}

		crateInArea = true;
		break;
	}

	return crateInArea;
}

/*
============
idBotAI::Bot_CheckThereIsHeavyVehicleInUseAlready
============
*/
bool idBotAI::Bot_CheckThereIsHeavyVehicleInUseAlready() {
	bool hasHeavyVehicle = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( i == botNum ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& player = botWorld->clientInfo[ i ];

		if ( !player.isBot ) {
			continue;
		}

		if ( player.team != botInfo->team ) {
			continue;
		}

		if ( player.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {
			continue;
		}

		proxyInfo_t vehicle;

		GetVehicleInfo( player.proxyInfo.entNum, vehicle );

		if ( vehicle.entNum == 0 ) {
			continue;
		}

		if ( vehicle.type == MCP ) {
			continue;
		}

		if ( !( vehicle.flags & PERSONAL ) && vehicle.flags & ARMOR || vehicle.flags & AIR ) {
			hasHeavyVehicle = true;
			break;
		}
	}

	return hasHeavyVehicle;
}

/*
============
idBotAI::FindChargeBySpawnID
============
*/
bool idBotAI::FindChargeBySpawnID( int spawnID, plantedChargeInfo_t& bombInfo ) {
	bombInfo.entNum = 0;

	for( int i = 0; i < MAX_CHARGES; i++ ) {
		const plantedChargeInfo_t& charge = botWorld->chargeInfo[ i ];

		if ( charge.spawnID != spawnID ) {
			continue;
		}

		bombInfo = charge;
		return true;
	}

	return false;
}

/*
============
idBotAI::Bot_HasShieldInWorldNearLocation
============
*/
bool idBotAI::Bot_HasShieldInWorldNearLocation( const idVec3& checkOrg, float checkDist ) {
	if ( botInfo->team != STROGG || botInfo->classType != FIELDOPS ) {
		return false;
	}

	bool hasShield = false;
	
	for( int j = 0; j < MAX_SHIELDS; j++ ) {
		if ( botInfo->forceShields[ j ].entNum == 0 ) {
			continue;
		}

		idVec3 vec = botInfo->forceShields[ j ].origin - checkOrg;

		if ( vec.LengthSqr() > Square( checkDist ) ) {
			continue;
		}

		hasShield = true;
		break;
	}

	return hasShield;
}

/*
============
idBotAI::Bot_NumShieldsInWorld
============
*/
int idBotAI::Bot_NumShieldsInWorld() {
	if ( botInfo->team != STROGG || botInfo->classType != FIELDOPS ) {
		return 0;
	}

	int numShields = 0;
	
	for( int j = 0; j < MAX_SHIELDS; j++ ) {
		if ( botInfo->forceShields[ j ].entNum == 0 ) {
			continue;
		}

		numShields++;
	}

	return numShields;
}

/*
============
idBotAI::CarrierInWorld
============
*/
bool idBotAI::CarrierInWorld() {
	bool carrierInWorld = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientHasObj( i ) ) {
			continue;
		}

		carrierInWorld = true;
		break;		
	}

	return carrierInWorld;
}

/*
============
idBotAI::Bot_CheckChargeExistsOnObjInWorld
============
*/
bool idBotAI::Bot_CheckChargeExistsOnObjInWorld() {
	bool chargeExists = false;

	for( int i = 0; i < MAX_CHARGES; i++ ) {
		const plantedChargeInfo_t& charge = botWorld->chargeInfo[ i ];

		if ( charge.entNum == 0 ) {
			continue;
		}

		if ( !charge.isOnObjective ) {
			continue;
		}

		if ( charge.state != BOMB_ARMED ) {
			continue;
		}

		if ( charge.team == botInfo->team ) {
			continue;
		}

		chargeExists = true;
		break;
	}

	return chargeExists;
}

/*
==================
idBotAI::GetDeployableAtAction
==================
*/
bool idBotAI::GetDeployableAtAction( int actionNumber, deployableInfo_t& deployable ) {
	bool hasDeployable = false;

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {

		if ( botWorld->deployableInfo[ i ].entNum == 0 ) {
			continue;
		}

		idVec3 vec = botWorld->deployableInfo[ i ].origin - botThreadData.botActions[ actionNumber ]->GetActionOrigin();
		float distSqr = vec.LengthSqr();

		if ( distSqr > Square( botThreadData.botActions[ i ]->GetRadius() ) && distSqr > Square( 256.0f ) ) {
			continue;
		}

		deployable = botWorld->deployableInfo[ i ];
		hasDeployable = true;
		break;
	}

	return hasDeployable;
}