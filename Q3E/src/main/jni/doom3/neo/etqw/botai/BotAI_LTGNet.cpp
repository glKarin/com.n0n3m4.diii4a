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
idBotAI_LTG::Enter_LTG_FollowMate

The bot has a teammate he wants to follow (and protect!). The goal finder will define how long we should be
following our teammate, and can reassign us if for some reason we run out of time.
================
*/
bool idBotAI::Enter_LTG_FollowMate() {

	ltgTime = botWorld->gameLocalInfo.time + FOLLOW_MATE_TIMELIMIT;

	PushAINodeOntoStack( ltgTarget, -1, ACTION_NULL, FOLLOW_MATE_TIMELIMIT, true, false ); //mal: by default, we dont use a vehicle for this node		

	LTG_AI_SUB_NODE = &idBotAI::LTG_FollowMate;
	
	ltgTimer = 0;

	ltgChat = true;

	ltgDist = 175.0f; //mal: want to get close to client the first time

	ltgReached = false;

	ltgUseShield = true;

	routeNode = NULL;

	ltgUseSmoke = true;

	ResetRandomLook();

	blockingTeammateCounter = 0;

	ltgMoveTime = 0;

	ltgPauseTime = 0;

	ltgTryMoveCounter = 0;

	ltgMoveDir = NULL_DIR;

	ltgCounter = 25 + ( botThreadData.random.RandomInt( 45 ) );  //mal: just a cute bit to make their crouching look more random

	if ( ClientIsValid( ltgTarget, ltgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
		const clientInfo_t& playerInfo = botWorld->clientInfo[ ltgTarget ];

		if ( !playerInfo.isBot ) {
			idVec3 vec = playerInfo.origin - botInfo->origin;

			if ( vec.LengthSqr() > Square( MEDIC_ACK_MIN_DIST * 2.0f ) ) {
				Bot_AddDelayedChat( botNum, LETS_GO, 1 );
			}
		}
	}

	lastAINode = "Follow Mate";

	return true;
}

/*
================
idBotAI_LTG::LTG_FollowMate

The bot has a teammate he wants to follow (and protect!).
================
*/
bool idBotAI::LTG_FollowMate() {
	bool rideVehicle = true;
	int attacker;
	float dist;
	float maxDist = 475.0f; //mal: the furthest the bot will let you go, before he reacts.
	proxyInfo_t vehicleInfo;
	botMoveFlags_t defaultMove = SPRINT;
	idVec3 vec;	

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !ClientIsValid( ltgTarget, ltgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
		Bot_ExitAINode();
		return false;
	}

	if ( Client_IsCriticalForCurrentObj( botNum, TOO_CLOSE_TO_GOAL_TO_FOLLOW_DIST ) || ( ClientHasObj( botNum ) && ClientIsCloseToDeliverObj( botNum, TOO_CLOSE_TO_DELIVER_TO_FOLLOW_DIST ) ) ) {
		Bot_ExitAINode();
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ ltgTarget ];

	if ( playerInfo.inLimbo ) { //mal: hes gone jim! Do something else
		Bot_ExitAINode();
		return false;
	}

	if ( ClientIsDead( ltgTarget ) && botInfo->classType != MEDIC ) { //mal: hes gone jim! Do something else. A medic will revive, and continue to escort!
		Bot_ExitAINode();
		return false;
	}

	if ( playerInfo.isDisguised || botInfo->isDisguised ) { //mal: if a covert decided to disguise himself, dont escort anymore - would give them away. Also, don't escort if we're disguised.
		Bot_ExitAINode();
		return false;
	}

	if ( playerInfo.isBot ) { //mal: dont follow a bot thats following someone else.
		if ( botThreadData.bots[ ltgTarget ] != NULL && botThreadData.bots[ ltgTarget ]->GetAIState() == LTG && ( botThreadData.bots[ ltgTarget ]->GetLTGType() == FOLLOW_TEAMMATE || botThreadData.bots[ ltgTarget ]->GetLTGType() == FOLLOW_TEAMMATE_BY_REQUEST ) ) {
			Bot_ExitAINode();
			return false;
		}
	}

	if ( ltgMoveTime < botWorld->gameLocalInfo.time ) {
		ltgTryMoveCounter = 0;
	}

	int moveAwayFromClientCrossHairCounter = ( playerInfo.weapInfo.isFiringWeap ) ? 1 : 10; //mal: if our friend is shooting, move out of the way quicker!

	vec = playerInfo.origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( botWorld->gameLocalInfo.botFollowPlayer == 2 ) {
		rideVehicle = false;
	}

	if ( botInfo->team != playerInfo.team ) {
		rideVehicle = false;
	}

	if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE && rideVehicle ) { //mal: client jumped into a vehicle/deployable - jump in if we can, or just forget them.
		GetVehicleInfo( playerInfo.proxyInfo.entNum, vehicleInfo );

		if ( !VehicleIsValid( vehicleInfo.entNum, false, true ) || vehicleInfo.flags & PERSONAL ) {
			Bot_ExitAINode();
			return false;
		} //mal: theres no room for us on this ride, so forget following this player.

		if ( ltgChat && playerInfo.isBot == false ) {
			Bot_AddDelayedChat( botNum, HOLD_VEHICLE, 0 );
			ltgChat = false;
		} //mal: let the teammate we're escorting know we're going to follow him into the vehicle

		Bot_GetIntoVehicle( vehicleInfo.entNum );
		vLTGTarget = ltgTarget;
		vLTGTargetSpawnID = ltgTargetSpawnID;
	    V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
		V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_RideWithMate;
		return true;
	}

	if ( ( playerInfo.posture == IS_CROUCHED || playerInfo.posture == IS_PRONE ) && playerInfo.crouchCounter > ltgCounter ) {
		if ( dist < Square( maxDist ) ) {
            defaultMove = CROUCH;
		}
	} else if ( playerInfo.xySpeed == 0.0f && dist < Square( ltgDist ) ) {
		defaultMove = WALK;
	} else if ( dist < Square( 900.0f ) ) {
		defaultMove = RUN;
	}

	botUcmd->actionEntityNum = ltgTarget; //mal: let the game and obstacle avoidance know we want to interact with this entity.
	botUcmd->actionEntitySpawnID = playerInfo.spawnID;

	if ( dist > Square( ltgDist ) || botInfo->onLadder ) {
 
		Bot_SetupMove( vec3_zero, ltgTarget, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreClient( ltgTarget, CLIENT_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( maxDist * 2 ) ) ? Bot_ShouldStrafeJump( playerInfo.origin ) : defaultMove );

		if ( dist < Square( maxDist ) && botAAS.blockedByObstacleCounterOnFoot > MAX_FRAMES_BLOCKED_BY_OBSTACLE_ON_FOOT ) {
			ltgDist += 5.0f;
		}//mal: if we've been trying to reach our target for too long, and got caught up on something or someone, relax the dist requirements a bit.

		ltgTimer = 0; //mal: we're chasing our friend, so reset the timer.
		ltgReached = false;
		ltgTryMoveCounter = 0;
		ResetRandomLook();
		return true;
	}

	if ( botInfo->classType == FIELDOPS && botInfo->team == STROGG && ltgUseShield && botInfo->lastShieldDroppedTime + 1500 < botWorld->gameLocalInfo.time ) {
		int numShieldsInWorld = Bot_NumShieldsInWorld();

		if ( numShieldsInWorld < 2 && playerInfo.xySpeed < WALKING_SPEED && ClassWeaponCharged( SHIELD_GUN ) && ( playerInfo.weapInfo.weapon == PLIERS || playerInfo.weapInfo.weapon == NEEDLE || playerInfo.weapInfo.weapon == HACK_TOOL || playerInfo.weapInfo.weapon == HE_CHARGE ) ) {
			bool hasShield = Bot_HasShieldInWorldNearLocation( playerInfo.origin, 300.0f );
			float maxDistToPlayer = ( hasShield ) ? 40.0f : SHIELD_FIRING_RANGE;

			if ( tacticalActionIgnoreTime < botWorld->gameLocalInfo.time ) {
				tacticalActionIgnoreTime = botWorld->gameLocalInfo.time + 1000;
			}

			if ( dist > Square( maxDistToPlayer ) || botInfo->onLadder ) {
				Bot_SetupMove( vec3_zero, ltgTarget, ACTION_NULL );
				
				if ( MoveIsInvalid() ) {
					ltgUseShield = false;
					return true;
				}

				ltgPauseTime = botWorld->gameLocalInfo.time + 1000;

				Bot_MoveAlongPath( RUN );
				return true;
			}

			if ( hasShield ) { //mal: if we've dropped a shield in front of him, drop one behind him to form a bubble of protection.
				Bot_LookAtLocation( ltgOrigin, SMOOTH_TURN );
			} else {
				idVec3 loc = playerInfo.viewOrigin;
				loc.z += 32.0f;
				Bot_LookAtLocation( loc, SMOOTH_TURN );

				if ( ltgOrigin.IsZero() ) {
					ltgOrigin = playerInfo.origin;
					vec = playerInfo.origin - botInfo->origin;
					float distToPlayer = vec.LengthFast();
					float zValue = ltgOrigin.z;
					ltgOrigin += ( -( distToPlayer ) * playerInfo.viewAxis[ 0 ] );
					ltgOrigin.z = ( zValue - 32.0f );
				}
			}

			botIdealWeapNum = SHIELD_GUN;
			botIdealWeapSlot = NO_WEAPON;

			ignoreWeapChange = true;

			ltgTimer = 0; //mal: we're chasing our friend, so reset the timer.
			ltgReached = false;
			ltgTryMoveCounter = 0;
			ResetRandomLook();
	
			if ( botInfo->weapInfo.weapon == SHIELD_GUN && ltgPauseTime < botWorld->gameLocalInfo.time ) {
				botUcmd->botCmds.attack = true;
			}

			return true;
		}
	} else if ( ltgUseSmoke && botInfo->classType == COVERTOPS && botInfo->team == GDF && ClassWeaponCharged( SMOKE_NADE ) ) {
		if ( playerInfo.xySpeed < WALKING_SPEED && ( playerInfo.weapInfo.weapon == PLIERS || playerInfo.weapInfo.weapon == HACK_TOOL || playerInfo.weapInfo.weapon == HE_CHARGE ) ) {
			
			if ( dist > Square( 350.0f ) || botInfo->onLadder ) {
				Bot_SetupMove( vec3_zero, ltgTarget, ACTION_NULL );

				if ( MoveIsInvalid() ) {
					ltgUseSmoke = false;
					return true;
				}

				Bot_MoveAlongPath( RUN );
				return true;
			}

			Bot_LookAtEntity( ltgTarget, SMOOTH_TURN );

			botIdealWeapNum = SMOKE_NADE;
			botIdealWeapSlot = NO_WEAPON;	

			ignoreWeapChange = true;

			ltgTimer = 0; //mal: we're chasing our friend, so reset the timer.
			ltgReached = false;
			ltgTryMoveCounter = 0;
			ResetRandomLook();

			if ( botInfo->weapInfo.weapon == SMOKE_NADE && botThreadData.random.RandomInt( 100 ) > 50 ) {
				botUcmd->botCmds.attack = true;
			}

			return true;
		}
	}

	if ( enemy == -1 ) {
        attacker = CheckClientAttacker( ltgTarget, 1 );

		if ( attacker != -1 ) {
			int travelTime;
			const clientInfo_t& attackerInfo = botWorld->clientInfo[ attacker ];
			if ( Bot_LocationIsReachable( false, attackerInfo.origin, travelTime ) ) {
		        aiState = LTG;
				ltgType = HUNT_GOAL;
				ltgTime = botWorld->gameLocalInfo.time + 15000;
				ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
				LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_HuntGoal;
				ltgTarget = attacker;
				ltgTargetSpawnID = attackerInfo.spawnID;
			}
		}
	}

	//mal: make sure we can see our friend - if not, we may need to move closer.
	if ( !botThreadData.Nav_IsDirectPath( AAS_PLAYER, botInfo->team, botInfo->areaNum, botInfo->aasOrigin, playerInfo.origin ) && dist > Square( 75.0f ) && playerInfo.xySpeed == 0.0f && ltgDist != 130.0f ) {
		ltgDist = 130.0f;
		return true;
	}

	if ( dist < Square( 50.0f ) && ltgTryMoveCounter < MAX_MOVE_ATTEMPTS ) { // too close, back up some!  
		ltgTryMoveCounter++;
		if ( Bot_CanMove( BACK, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, defaultMove, NULLMOVETYPE );
		} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, defaultMove, NULLMOVETYPE );
		} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, defaultMove, NULLMOVETYPE );
		}

		Bot_LookAtEntity( ltgTarget, SMOOTH_TURN );
		ltgTimer = 0; //mal: we're retreating from our friend, so reset the timer.
		ltgReached = false;
		ltgMoveTime = botWorld->gameLocalInfo.time + 1000;

		if ( ltgDist < maxDist ) {
			ltgDist += 5.0f;
		}
		return true;
	}

	int blockedClient = Bot_CheckBlockingOtherClients( ltgTarget );

	if ( blockedClient != -1 && ltgTryMoveCounter < MAX_MOVE_ATTEMPTS ) {
		ltgTryMoveCounter++;
		if ( Bot_CanMove( BACK, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, defaultMove, NULLMOVETYPE );
		} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, defaultMove, NULLMOVETYPE );
		} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, defaultMove, NULLMOVETYPE );
		}

		Bot_LookAtEntity( blockedClient, SMOOTH_TURN );
		ltgTimer = 0;
		ltgReached = false;
		ltgMoveTime = botWorld->gameLocalInfo.time + 1000;
		if ( ltgDist < maxDist ) {
			ltgDist += 5.0f;
		}

		return true;
	}

	bool shouldAvoidTeammate = false;

	if ( !playerInfo.isBot && playerInfo.weapInfo.weapon != HEALTH ) { //mal: dont avoid other bots, or medics trying to help us out.
		shouldAvoidTeammate = true;
	}

	if ( shouldAvoidTeammate ) {
		idVec3 dir = botInfo->origin - playerInfo.origin;
		dir.NormalizeFast();
		if ( dir * playerInfo.viewAxis[ 0 ] > 0.98f ) {
			blockingTeammateCounter++;
		} else {
			blockingTeammateCounter = 0;
			ltgMoveDir = NULL_DIR;
		}
	} else {
		blockingTeammateCounter = 0;
		ltgMoveDir = NULL_DIR;
	}

	if ( blockingTeammateCounter > moveAwayFromClientCrossHairCounter && ltgTryMoveCounter < MAX_MOVE_ATTEMPTS ) {
		ltgTryMoveCounter++;
		if ( ltgMoveDir == NULL_DIR ) {
			if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
				if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
					ltgMoveDir = RIGHT;
				} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
					ltgMoveDir = LEFT;
				} else { 
					ltgMoveDir = BACK;
				}
			} else {
				if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
					ltgMoveDir = LEFT;
				} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
					ltgMoveDir = RIGHT;
				} else { 
					ltgMoveDir = BACK;
				}
			}
		}

		botMoveTypes_t botMoveType;

		if ( ltgMoveDir == RIGHT ) {
			botMoveType = STRAFE_RIGHT;
		} else if ( ltgMoveDir == LEFT ) {
			botMoveType = STRAFE_LEFT;
		} else {
			botMoveType = BACKSTEP;
		}

		Bot_LookAtEntity( ltgTarget, SMOOTH_TURN ); //mal: look at our target for a bit when first reached.
		Bot_MoveToGoal( vec3_zero, vec3_zero, defaultMove, botMoveType );

		ltgTimer = 0;
		ltgReached = false;
		ltgMoveTime = botWorld->gameLocalInfo.time + 1000;
		return true;
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, defaultMove, NULLMOVETYPE );

	if ( ltgTimer == 0 ) { //mal: add a little variety to the bots following dist, so hes not so "robotic" about being near you.
        ltgDist = 175.0f + ( float ) botThreadData.random.RandomInt( 301 );
		ltgTimer = botWorld->gameLocalInfo.time + 1000;
	}

	if ( ltgTimer > botWorld->gameLocalInfo.time ) {
		Bot_LookAtEntity( ltgTarget, SMOOTH_TURN ); //mal: look at our target for a bit when first reached.
	} else {
		if ( botThreadData.random.RandomInt( 100 ) > 97 || ltgReached == false ) {
			idVec3 vec;
			if ( Bot_RandomLook( vec ) ) {
                Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
				ltgReached = true;
			}
		}
	}

	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_RoamGoal

The bot has a location on the map he wants to visit. When he reaches it, he'll move on to somewhere else.
================
*/
bool idBotAI::Enter_LTG_RoamGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_RoamGoal;

	ltgType = ROAM_GOAL;
	
	stayInPosition = false;

//mal: higher skill bots are more likely to respond to heard sounds, and go after the person making them, then lower skilled bots.
	if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) {
		stayInPosition = true;
	} else if ( botThreadData.GetBotSkill() == BOT_SKILL_NORMAL ) {
		if ( botThreadData.random.RandomInt( 100 ) > 30 ) {
			stayInPosition = true;
		}
	} else {
		if ( botThreadData.random.RandomInt( 100 ) > 70 ) {
			stayInPosition = true;
		}
	}

	lastAINode = "Roam Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_RoamGoal
================
*/
bool idBotAI::LTG_RoamGoal() {
	int vehicleNum;
	float dist;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( actionNumInsideDanger == actionNum ) { //mal: if our action is encased inside a danger ( landmine ), and we're not destroying it, must not be able to, so leave.
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

		if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}
		return true;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );

		return true;
	}

	Bot_ExitAINode(); //mal: reached our roam goal, so just leave.

	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_CampGoal

The bot has a location on the map he wants to camp. When he reaches it, he'll pop into the NBG AI node for camping.
================
*/
bool idBotAI::Enter_LTG_CampGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_CampGoal;

	stayInPosition = false;

//mal: higher skill bots are more likely to respond to heard sounds, and go after the person making them, then lower skilled bots.
	if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) {
		stayInPosition = true;
	} else if ( botThreadData.GetBotSkill() == BOT_SKILL_NORMAL ) {
		if ( botThreadData.random.RandomInt( 100 ) > 30 ) {
			stayInPosition = true;
		}
	} else {
		if ( botThreadData.random.RandomInt( 100 ) > 70 ) {
			stayInPosition = true;
		}
	}

	if ( ltgType == DEFENSE_CAMP_GOAL ) {
		lastAINode = "Defense Camp Goal";
		stayInPosition = true;
	} else {
		lastAINode = "Camp Goal";
	}
	
	return true;
}

/*
================
idBotAI_LTG::LTG_CampGoal
================
*/
bool idBotAI::LTG_CampGoal() {

	int mates, vehicleNum;
	float dist;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( botThreadData.AllowDebugData() && bot_debugActionGoalNumber.GetInteger() == actionNum ) {
		goto debugSkipChecks;
	}

	if ( !botInfo->weapInfo.primaryWeapHasAmmo ) { //mal: no ammo - dont camp.
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( actionNumInsideDanger == actionNum ) { //mal: if our action is encased inside a danger ( landmine ), and we're not destroying it, must not be able to, so leave.
		Bot_ExitAINode();
		return false;
	}

	mates = ClientsInArea( botNum, botThreadData.botActions[ actionNum ]->GetActionOrigin(), ( ltgType == DEFENSE_CAMP_GOAL ) ? DEFENSE_CAMP_MATE_RANGE : 150.0f, botInfo->team, NOCLASS, false, false, false, false, true );

	if ( mates > 0 ) {		//mal: don't try to camp a spot someone else may already be at.
		Bot_ExitAINode();
		return false;
	}

debugSkipChecks:

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );

		return true;
	}

    ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_Camp;
    nbgTime = botWorld->gameLocalInfo.time + ( botThreadData.botActions[ actionNum ]->actionTimeInSeconds * 1000 ); //mal: timelimit is specified by action itself!
	return false;
}

/*
================
idBotAI_LTG::LTG_ErrorThink

A special, safety node for the bots if they have nothing to think about ( i.e. the map has no goals yet ).
================
*/
bool idBotAI::LTG_ErrorThink() {

	idVec3 vec;

	lastAINode = "Error Think";

	if ( botThreadData.random.RandomInt( 100 ) > 98 ) {
        if ( Bot_RandomLook( vec ) ) {
            Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
		}
	}

	if ( botThreadData.random.RandomInt( 100 ) > 98 ) {
		botUcmd->botCmds.hasNoGoals = true; //mal: pass a warning to the player to let them know we have no goals
	}

	Bot_ResetState( true, true );
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_BuildGoal

The bot has an objective on the map that he wants to build.
================
*/
bool idBotAI::Enter_LTG_BuildGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_BuildGoal;

	lastAINode = "Build Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_BuildGoal
================
*/
bool idBotAI::LTG_BuildGoal() {

	int vehicleNum;
	float dist;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( ltgType == BUILD_GOAL ) {
		if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: player decided to be a hero, so let them.
			Bot_ExitAINode();
			return false;
		}
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );

		return true;
	}

//mal: reached our action - pop into the short term goal AI node to execute it!
    ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_Build;
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_PlantGoal

The bot has an objective on the map that he wants to plant explosives on.
================
*/
bool idBotAI::Enter_LTG_PlantGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_PlantGoal;

	ltgType = PLANT_GOAL;

//mal: let everyone know what he wants to do
	if ( botThreadData.botActions[ actionNum ]->GetVOChat() != NULL_CHAT ) {
		Bot_AddDelayedChat( botNum, botThreadData.botActions[ actionNum ]->GetVOChat(), 3 );
	} else {
		Bot_AddDelayedChat( botNum, GENERIC_PLANT, 3 );
	}

	ltgMoveTime = 0;

	ltgTryMoveCounter = 0;

	lastAINode = "Plant Goal";
	
	return true;
}

/*
================
idBotAI_LTG::LTG_PlantGoal
================
*/
bool idBotAI::LTG_PlantGoal() {

	int vehicleNum;
	float dist;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: player decided to be a hero, so let them.
		Bot_ExitAINode();
		return false;
	}


	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	if ( ltgMoveTime < botWorld->gameLocalInfo.time ) {
		ltgTryMoveCounter = 0;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	idBox playerBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );

	if ( dist < Square( ACTION_CHECK_TEAMMATE_DIST ) ) {
		if ( botThreadData.botActions[ actionNum ]->CheckTeamMemberIsInsideAction( botNum, botInfo->team, botInfo->classType, true ) && !botThreadData.botActions[ actionNum ]->actionBBox.IntersectsBox( playerBox ) ) {
			if ( botThreadData.random.RandomInt( 100 ) > 95 ) {
				if ( Bot_RandomLook( vec ) ) {
					Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
				}
			}

			botIdealWeapSlot = GUN;
			ignoreWeapChange = true;

			Bot_MoveToGoal( vec3_zero, vec3_zero, CROUCH, NULLMOVETYPE );

			int blockedClient = Bot_CheckBlockingOtherClients( -1 ); //mal: try not to block other clients who may also want to plant, or get by.

			if ( blockedClient != -1 && ltgTryMoveCounter < MAX_MOVE_ATTEMPTS ) {
				ltgTryMoveCounter++;
				if ( Bot_CanMove( BACK, 100.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, CROUCH, NULLMOVETYPE );
				} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, CROUCH, NULLMOVETYPE );
				} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, CROUCH, NULLMOVETYPE );
				}
				
				Bot_LookAtEntity( blockedClient, SMOOTH_TURN );
				ltgMoveTime = botWorld->gameLocalInfo.time + 1000;
			}

			return true;
		}
	}

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
		return true;
	}

//mal: reached our action - pop into the short term goal AI node to execute it!
    ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_PlantBomb;
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_HackGoal

The bot has an objective on the map that he wants to hack.
================
*/
bool idBotAI::Enter_LTG_HackGoal() {

	if ( botInfo->weapInfo.primaryWeapon == SNIPERRIFLE ) {
		botUcmd->botCmds.switchAwayFromSniperRifle = true;
	}

	LTG_AI_SUB_NODE = &idBotAI::LTG_HackGoal;
	
	lastAINode = "Hack Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_HackGoal
================
*/
bool idBotAI::LTG_HackGoal() {
	int vehicleNum;
	float dist;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: player decided to be a hero, so let them.
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
		return true;
	}

//mal: reached our action - pop into the short term goal AI node to execute it!
    ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_Hack;
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_DefuseGoal

The bot has an objective on the map that hes trying to protect, so he'll defuse any planted charges that may be on it.
================
*/
bool idBotAI::Enter_LTG_DefuseGoal() {

	ltgType = DEFUSE_GOAL;

	PushAINodeOntoStack( -1, -1, actionNum, DEFAULT_LTG_TIME, true, ltgUseVehicle );

	LTG_AI_SUB_NODE = &idBotAI::LTG_DefuseGoal;

	lastAINode = "Defuse Goal";
	
	return true;
}

/*
================
idBotAI_LTG::LTG_DefuseGoal
================
*/
bool idBotAI::LTG_DefuseGoal() {
	float dist;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: player decided to be a hero, so let them.
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->ArmedChargesInsideActionBBox( -1 ) ) { //mal: someone defused all the charges, so leave.
		Bot_ExitAINode();
		return false;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	plantedChargeInfo_t bombInfo;
	bool isNearBomb = FindChargeInWorld( actionNum, bombInfo, DISARM ); //mal: looks silly to walk by charge to move towards the action, so check if we're close to it.

	if ( !isNearBomb && ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
		return true;
	}

//mal: reached our action - pop into the short term goal AI node to execute it!
    ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_DefuseBomb;
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_HuntGoal

The bot has a target on the map he wants to pay a none too frienldy visit. When he reaches it, he'll kill it, unless he runs out of time.
================
*/
bool idBotAI::Enter_LTG_HuntGoal() {

	ltgType = HUNT_GOAL;

	routeNode = NULL;

	if ( ltgTime > 30000 ) { //mal: if we've decided to do this for a while, push it onto the stack.
        PushAINodeOntoStack( -1, -1, actionNum, DEFAULT_LTG_TIME, false, false );
	}

	ltgTime += botWorld->gameLocalInfo.time;

	LTG_AI_SUB_NODE = &idBotAI::LTG_HuntGoal;

	fastAwareness = true;
	
	lastAINode = "Hunt Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_HuntGoal
================
*/
bool idBotAI::LTG_HuntGoal() {
	float dist;
	idVec3 vec;
	int ignoreHuntGoalClientTime = 5000;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ClearAIStack();
		Bot_ExitAINode();
		Bot_IgnoreClient( ltgTarget, ignoreHuntGoalClientTime );
		fastAwareness = false;
		return false;
	}

	if ( !ClientIsValid( ltgTarget, ltgTargetSpawnID ) ) {
		Bot_ClearAIStack();
		Bot_ExitAINode();
		Bot_IgnoreClient( ltgTarget, ignoreHuntGoalClientTime );
		fastAwareness = false;
		return false;
	}

	if ( ClientIsDead( ltgTarget ) ) { //mal: targets dead already
		Bot_ClearAIStack();
		Bot_ExitAINode();
		fastAwareness = false;
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ ltgTarget ];

	botUcmd->actionEntityNum = ltgTarget;
	botUcmd->actionEntitySpawnID = playerInfo.spawnID;

	vec = playerInfo.origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist > Square( 50.0f ) || botInfo->onLadder || botInfo->inWater ) {
		Bot_SetupMove( playerInfo.origin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_ClearAIStack();
			Bot_ExitAINode();
			Bot_IgnoreClient( ltgTarget, ignoreHuntGoalClientTime );
			fastAwareness = false;
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 500.0f ) ) ? Bot_ShouldStrafeJump( playerInfo.origin ) : RUN );
		return true;
	}

	Bot_ClearAIStack();
	Bot_ExitAINode(); //mal: if we get this far, and still no enemy, somethings goofy, so just leave.
	Bot_IgnoreClient( ltgTarget, ignoreHuntGoalClientTime );
	fastAwareness = false;

	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_MineGoal

The bot has a location on the map he wants to plant mines at.
================
*/
bool idBotAI::Enter_LTG_MineGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_MineGoal;

	if ( ltgType == PRIORITY_MINE_GOAL ) {
		lastAINode = "Priority Mine Goal";
	} else {
		lastAINode = "Mine Goal";
	}

	return true;
}

/*
================
idBotAI_LTG::LTG_MineGoal
================
*/
bool idBotAI::LTG_MineGoal() {
	int vehicleNum;
	float dist;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( botThreadData.botActions[ actionNum ]->ArmedMinesInsideActionBBox() ) { //mal: someone already planted at the spot we wanted to, so just leave.
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
		return true;
	}

    ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_PlantMine;

	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_SnipeGoal

The bot has a location on the map he wants to snipe at.
================
*/
bool idBotAI::Enter_LTG_SnipeGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_SnipeGoal;

	ltgType = SNIPE_GOAL;
	
	lastAINode = "Sniper Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_SnipeGoal
================
*/
bool idBotAI::LTG_SnipeGoal() {

	int vehicleNum;
	int matesInArea;
	float dist;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !botInfo->weapInfo.primaryWeapHasAmmo ) { //mal: no ammo - dont camp.
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( botInfo->isDisguised ) {
		Bot_ExitAINode();
		return false;
	}

	matesInArea = ClientsInArea( botNum, botThreadData.botActions[ actionNum ]->GetActionOrigin(), 150.0f, botInfo->team, COVERTOPS, false, false, false, true, true );

	if ( matesInArea > 0 ) { //mal: someone there camping already ( prolly a human ), so just leave.
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
		return true;
	}

    ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_Snipe;
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_InvestigateGoal

The bot has an action that he wants to check out - its prolly under attack.
================
*/
bool idBotAI::Enter_LTG_InvestigateGoal() {

	ltgType = INVESTIGATE_ACTION;

	PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, false );

	LTG_AI_SUB_NODE = &idBotAI::LTG_InvestigateGoal;
	
	stayInPosition = false;

	routeNode = NULL;

//mal: higher skill bots are more likely to respond to heard sounds, and go after the person making them, then lower skilled bots.
	if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) {
		stayInPosition = true;
	} else if ( botThreadData.GetBotSkill() == BOT_SKILL_NORMAL ) {
		if ( botThreadData.random.RandomInt( 100 ) > 30 ) {
			stayInPosition = true;
		}
	} else {
		if ( botThreadData.random.RandomInt( 100 ) > 70 ) {
			stayInPosition = true;
		}
	}

	lastAINode = "Investigate Action";

	return true;
}

/*
================
idBotAI_LTG::LTG_InvestigateGoal
================
*/
bool idBotAI::LTG_InvestigateGoal() {

	int matesInArea;
	float dist;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off, so we dont care about it now.
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->ActionIsPriority() ) { //mal: action has been flagged as not being a priority anymore, so do something else.
		Bot_ExitAINode();
		return false;
	}

	int enemiesInArea = ClientsInArea( botNum, botThreadData.botActions[ actionNum ]->GetActionOrigin(), BOT_INVESTIGATE_RANGE, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, false, false );

	if ( botThreadData.botActions[ actionNum ]->GetActionState() == ACTION_STATE_NORMAL && enemiesInArea == 0 ) { //mal: its back to normal, we can go about our business.
        Bot_ExitAINode();
		return false;
	}

	matesInArea = ClientsInArea( botNum, botThreadData.botActions[ actionNum ]->GetActionOrigin(), 350.0f, botInfo->team, NOCLASS, false, false, false, false, false );

	if ( matesInArea > MIN_NUM_INVESTIGATE_CLIENTS ) {
		Bot_ExitAINode();
		return false;;
	} //mal: already some ppl there, so lets do something else.

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist > Square( CLOSE_ENOUGH_TO_INVESTIGATE_GOAL_RANGE ) || botInfo->onLadder ) { //mal: get REALLY close to the action in question, so we can see thru smoke, other obstalces that may be in the way.

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 500.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
		return true;
	}

//mal: if we get here, it means we've reached our target action. If it was a plant, lets decide to camp it to cover it until an eng arrives.
	if ( botThreadData.botActions[ actionNum ]->GetObjForTeam( botInfo->team ) == ACTION_DEFUSE && botThreadData.random.RandomInt( 100 ) > 50 ) {
		if ( !Bot_NBGIsAvailable( -1, actionNum, CAMP, matesInArea ) ) {
			Bot_ExitAINode();
		} else {
            ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_Camp;
			nbgTime = botWorld->gameLocalInfo.time + 30000; //mal: if we're camping at an action we investigated, only do so for 30 secs ( bomb will be defused/blow by then anyhow ).
		}
	} else {
		int criticalEnemiesInArea = ClientsInArea( botNum, botThreadData.botActions[ actionNum ]->GetActionOrigin(), CRITICAL_ENEMY_CLOSE_TO_GOAL_RANGE, ( botInfo->team == GDF ) ? STROGG : GDF, TeamCriticalClass( ( botInfo->team == GDF ) ? STROGG : GDF ), false, false, false, false, false );

		if ( criticalEnemiesInArea == 0 ) {
			Bot_ExitAINode(); //mal: reached our goal, so just leave.
		} else {
			ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_Camp;
			nbgTime = botWorld->gameLocalInfo.time + 5000; 
		}
	}

	return false;
}

/*
================
idBotAI::Enter_LTG_UseVehicle
================
*/
bool idBotAI::Enter_LTG_UseVehicle() {
    
	LTG_AI_SUB_NODE = &idBotAI::LTG_UseVehicle;
	
	ltgReached = false;

	ResetRandomLook();

	if ( ltgType == DRIVE_MCP ) {
		ltgReached = FindMCPStartAction( ltgOrigin );
		lastAINode = "Entering MCP";
	} else {
		lastAINode = "Entering Vehicle";
	}

	return true;	
}

/*
================
idBotAI::LTG_UseVehicle
================
*/
bool idBotAI::LTG_UseVehicle() {
	float dist;
	proxyInfo_t vehicleInfo;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( ltgType == DRIVE_MCP ) {
		ltgTarget = FindClosestVehicle( idMath::INFINITY, botInfo->origin, MCP, NULL_VEHICLE_FLAGS, NULL_VEHICLE_FLAGS, false ); //mal: let the bots ride shotgun
	} else if ( ltgType == GRAB_VEHICLE_GOAL ) {
		ltgTarget = FindClosestVehicle( MAX_VEHICLE_RANGE, botThreadData.botActions[ actionNum ]->GetActionOrigin(), NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );
	} else if ( ltgType == ENTER_HEAVY_VEHICLE ) {
		ltgTarget = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, AIR, PERSONAL, true ); //mal: try an air vehicle first - then the action's vehicle.

		if ( ltgTarget == -1 ) {
			ltgTarget = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), PERSONAL, true );
		}
	} else {
		ltgTarget = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );
	}

	if ( ltgTarget == -1 ) { //mal: if can't find a vehicle anymore, cant do a vehicle only mission.
		Bot_ExitAINode();
		return false;
	}

	GetVehicleInfo( ltgTarget, vehicleInfo );

	if ( vehicleInfo.entNum == 0 ) {
		Bot_ExitAINode();
		return false;
	}

	if ( ltgType == DRIVE_MCP ) {
		if ( !vehicleInfo.hasGroundContact ) {
			if ( ltgReached ) {
				vec = ltgOrigin - botInfo->origin;
				if ( vec.LengthSqr() > Square( 250.0f ) ) {
					Bot_SetupMove( ltgOrigin, -1, ACTION_NULL );
					if ( MoveIsInvalid() ) {
						Bot_ExitAINode();
						return false;
					}

					Bot_MoveAlongPath( Bot_ShouldStrafeJump( ltgOrigin ) );
					return true;
				} else {
					if ( botThreadData.random.RandomInt( 100 ) > 90 ) {
						if ( Bot_RandomLook( vec ) ) {
							Bot_LookAtLocation( vec, SMOOTH_TURN );
						}
					}
					return true;
				}
			} else { //mal: someone forgot to add a MCP start action, so just stand here and wait for it to drop before we run to it.
				if ( botThreadData.random.RandomInt( 100 ) > 90 ) {
					if ( Bot_RandomLook( vec ) ) {
						Bot_LookAtLocation( vec, SMOOTH_TURN );
					}
				}
				return true;
			}
		}
	}

	if ( vehicleInfo.driverEntNum != -1 ) { //mal: someone took off in our ride!
		Bot_ExitAINode();
		return false;
	}

	botUcmd->actionEntityNum = vehicleInfo.entNum;
	botUcmd->actionEntitySpawnID = vehicleInfo.spawnID;

	idVec3 goalOrigin;

	if ( ltgType == GRAB_VEHICLE_GOAL && ltgReached == false ) {
		vec = botThreadData.botActions[ actionNum ]->GetActionOrigin() - botInfo->origin;

		if ( vec.LengthSqr() > Square( botThreadData.botActions[ actionNum ]->radius ) ) {
			goalOrigin = botThreadData.botActions[ actionNum ]->GetActionOrigin();
		} else {
			goalOrigin = vehicleInfo.origin;
			ltgReached = true;
		}
	} else {
		goalOrigin = vehicleInfo.origin;
	}

	goalOrigin.z += 32.0f;

	Bot_SetupMove( goalOrigin, -1, ACTION_NULL );

	if ( MoveIsInvalid() ) {
		Bot_ExitAINode();
		return false;
	}

	vec = vehicleInfo.origin - botInfo->origin;

	dist = vec.LengthSqr();

	Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? Bot_ShouldStrafeJump( goalOrigin ) : RUN );

//mal: if we're fairly close, start sending the vehicle use cmd
	if ( dist < Square( 350.0f ) ) {
		Bot_LookAtLocation( vehicleInfo.origin, SMOOTH_TURN );
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
            botUcmd->botCmds.enterVehicle = true;
		}
	}

	return true;	
}

/*
================
idBotAI_LTG::Enter_LTG_FixMCP

The bot has a MCP that he wants to check fix.
================
*/
bool idBotAI::Enter_LTG_FixMCP() {

	ltgType = FIX_MCP;
	
	ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY; //mal: do this forever, until we die or complete it!

	LTG_AI_SUB_NODE = &idBotAI::LTG_FixMCP;

	ltgReached = false;

	ltgPauseTime = 0;
	
	lastAINode = "Fix MCP";

	return true;
}

/*
================
idBotAI_LTG::LTG_FixMCP
================
*/
bool idBotAI::LTG_FixMCP() {

	int vehicleNum;
	proxyInfo_t vehicleInfo;
	idVec3 vec;

	if ( !botWorld->botGoalInfo.mapHasMCPGoal ) {
		Bot_ExitAINode();
		return false;
	}

	if ( botWorld->botGoalInfo.botGoal_MCP_VehicleNum == -1 ) {
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, NULL_VEHICLE_FLAGS, NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	GetVehicleInfo( botWorld->botGoalInfo.botGoal_MCP_VehicleNum, vehicleInfo );

	if ( vehicleInfo.entNum == 0 ) {
		Bot_ExitAINode();
		return false;
	}

	if ( vehicleInfo.health == vehicleInfo.maxHealth ) {
		Bot_ExitAINode();
		return false;
	}

	if ( vehicleInfo.xyspeed > WALKING_SPEED ) {
		Bot_ExitAINode();
		return false;
	}

	if ( vehicleInfo.inWater || !vehicleInfo.inPlayZone || vehicleInfo.areaNum == 0 || vehicleInfo.isFlipped ) {
		Bot_ExitAINode();
		return false;
	}

	if ( vehicleInfo.health > ( vehicleInfo.maxHealth / 2 ) && vehicleInfo.driverEntNum == -1 ) { //mal: get the MCP to the outpost once its ready to move!
		Bot_ExitAINode();
		return false;
	}

	botUcmd->actionEntityNum = vehicleInfo.entNum;
	botUcmd->actionEntitySpawnID = vehicleInfo.spawnID;

	vec = vehicleInfo.origin - botInfo->origin;

	if ( vec.LengthSqr() > Square( 700.0f ) || botInfo->onLadder ) {

		idVec3 vehicleOrigin = vehicleInfo.origin;
		vehicleOrigin.z += VEHICLE_PATH_ORIGIN_OFFSET;

		Bot_SetupMove( vehicleOrigin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( Bot_ShouldStrafeJump( vehicleInfo.origin ) );
		return true;
	}

	int avtDanger = Bot_FindClosestAVTDanger( vehicleInfo.origin, AVT_NEAR_MCP_CONSIDER_RANGE, true ); //mal: decide if we should fix the MCP, or attack a nearby AVT, that will just make our job harder anyhow.

	if ( avtDanger != -1 && Bot_HasExplosives( false ) ) {
		nbgTarget = avtDanger;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_EngAttackAVTNearMCP;
	} else {
		nbgTarget = vehicleInfo.entNum;
		nbgTargetType = VEHICLE;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_FixProxyEntity;
	}

	return false;
}

/*
================
idBotAI_LTG::Enter_LTG_SpawnPointGoal

The bot has an action that he wants to check out - its prolly under attack.
================
*/
bool idBotAI::Enter_LTG_SpawnPointGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_SpawnPointGoal;

	if ( ltgType == STEAL_SPAWN_GOAL ) {
		lastAINode = "Grab Enemy Spawn";
	} else {
		lastAINode = "Grab Spawn";
	}

	ltgTimer = 0;

	botTeleporterAttempts = 0;

	return true;
}

/*
================
idBotAI_LTG::LTG_SpawnPointGoal
================
*/
bool idBotAI::LTG_SpawnPointGoal() {

	int vehicleNum;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		if ( Bot_UseTeleporter() ) {
			return true;
		}

		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		if ( Bot_UseTeleporter() ) {
			return true;
		}
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off, so we dont care about it now.
		if ( Bot_UseTeleporter() ) {
			return true;
		}
		Bot_ExitAINode();
		return false;
	}

	if ( ltgType == STEAL_SPAWN_GOAL ) {
		if ( botThreadData.botActions[ actionNum ]->GetTeamOwner() == NOTEAM ) { //mal: its back to normal, we can go about our business.
			if ( Bot_UseTeleporter() ) {
				return true;
			}
			Bot_ExitAINode();
			return false;
		}
	} else {
		if ( botThreadData.botActions[ actionNum ]->GetTeamOwner() == botInfo->team ) { //mal: its ours!
			Bot_ExitAINode();
			return false;
		}
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
		if ( Bot_CheckForHumanInteractingWithEntity( botThreadData.botActions[ actionNum ]->GetActionSpawnControllerEntNum() ) == true ) {
			Bot_ExitAINode();
			return false;
		}
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	float distSqr = vec.LengthSqr();

	if ( ltgType == STEAL_SPAWN_GOAL ) { //mal: drop a teleporter pad down a safe dist away, so we can make a quick exit after we've grabbed the spawn.
		if ( botInfo->team == STROGG && botInfo->classType == COVERTOPS && !botInfo->hasTeleporterInWorld && !botInfo->isDisguised ) {
			if ( ClassWeaponCharged( TELEPORTER ) && distSqr > Square( MIN_TELEPORTER_RANGE ) && distSqr < Square( MAX_TELEPORTER_RANGE ) ) {
				botIdealWeapNum = TELEPORTER;
				botIdealWeapSlot = NO_WEAPON;
				ignoreWeapChange = true;
				ltgTimer++;

				if ( botInfo->weapInfo.weapon == TELEPORTER && ltgTimer > 15 ) {
					botUcmd->botCmds.attack = true;
					botUcmd->botCmds.lookDown = true;
					ltgTimer = 0;
				}
			}
		}
	}

	if ( distSqr > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) );
		return true;
	}

	ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_GrabSpawnPoint;
	return false;
}

/*
================
idBotAI_LTG::Enter_LTG_DeliverGoal

The bot has the docs/gold/parts/key/brain/etc - lets get it where it needs to be.
================
*/
bool idBotAI::Enter_LTG_DeliverGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_DeliverGoal;

	ResetRandomLook();

	ltgReached = false;

	lastAINode = "Deliver Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_DeliverGoal
================
*/
bool idBotAI::LTG_DeliverGoal() {

	int vehicleNum;
	float dist;
	idVec3 vec;

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( !ClientHasObj( botNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
		return true;
	}

//mal: the deliver point may have a delay, in which case we will wait and look around for enemies while our goal is "transmitted".
	Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->actionBBox.GetCenter(), SMOOTH_TURN );
	ltgReached = true;
	Bot_MoveToGoal( vec3_zero, vec3_zero, WALK, NULLMOVETYPE );
	botUcmd->botCmds.activate = true;
	botUcmd->botCmds.activateHeld = true;
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_StealGoal
================
*/
bool idBotAI::Enter_LTG_StealGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_StealGoal;

	ltgReached = false;

	ltgOrigin = botThreadData.botActions[ actionNum ]->actionBBox.GetCenter();

	lastAINode = "Steal Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_StealGoal
================
*/
bool idBotAI::LTG_StealGoal() {
	int vehicleNum;
	float dist;
	idVec3 vec;

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( ClientHasObj( botNum ) ) { //mal: we got it - lets run for it!
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	if ( ltgReached == false ) {
		vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
		dist = vec.LengthSqr();

		if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) ) {

			Bot_SetupMove( vec3_zero, -1, actionNum );

			if ( MoveIsInvalid() ) {
				Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
				Bot_ExitAINode();
				return false;
			}

			Bot_MoveAlongPath( ( enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
			return true;
		}

		ltgReached = true;
	}

	Bot_SetupMove( ltgOrigin, -1, ACTION_NULL );

	if ( MoveIsInvalid() ) {
		Bot_ExitAINode();
		Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
		return false;
	}

	Bot_MoveAlongPath( SPRINT );
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_RecoverDroppedGoal
================
*/
bool idBotAI::Enter_LTG_RecoverDroppedGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_RecoverDroppedGoal;
	
	lastAINode = "Recover Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_RecoverDroppedGoal
================
*/
bool idBotAI::LTG_RecoverDroppedGoal() {

	int vehicleNum;
	float dist;
	idVec3 vec;

	if ( botWorld->botGoalInfo.carryableObjs[ ltgTarget ].onGround == false || botWorld->botGoalInfo.carryableObjs[ ltgTarget ].entNum == 0 ) {
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, GROUND, NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	vec = botWorld->botGoalInfo.carryableObjs[ ltgTarget ].origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist > Square( 15.0f ) || botInfo->onLadder ) {

		Bot_SetupMove( botWorld->botGoalInfo.carryableObjs[ ltgTarget ].origin, -1, ACTION_NULL, botWorld->botGoalInfo.carryableObjs[ ltgTarget ].areaNum );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( enemy == -1 ) ? Bot_ShouldStrafeJump( botWorld->botGoalInfo.carryableObjs[ ltgTarget ].origin ) : RUN );
		return true;
	}

	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_DeployableGoal

The bot has a location on the map he wants to drop a deployable.
================
*/
bool idBotAI::Enter_LTG_DeployableGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_DeployableGoal;

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	ltgTarget = Bot_GetDeployableTypeForAction( actionNum );

	if ( ltgTarget == NULL_DEPLOYABLE ) {
		Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}
	
	if ( ltgType == DROP_PRIORITY_DEPLOYABLE_GOAL ) {
		lastAINode = "Place Priority Deployable Goal";
	} else {
		lastAINode = "Place Deployable Goal";
	}

	return true;
}

/*
================
idBotAI_LTG::LTG_DeployableGoal
================
*/
bool idBotAI::LTG_DeployableGoal() {

	float dist;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( botInfo->isDisguised ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( DeployableAtAction( actionNum, true ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		int vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	deployableInfo_t deployable;

	if ( GetDeployableAtAction( actionNum, deployable ) ) {
		botUcmd->actionEntityNum = deployable.entNum; //mal: let the game and obstacle avoidance know we want to interact with this entity.
		botUcmd->actionEntitySpawnID = deployable.spawnID;
	}

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 500.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
		return true;
	}
	
	ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_PlaceDeployable;
	return true;
}

/*
================
idBotAI::Enter_LTG_DestroyDeployable
================
*/
bool idBotAI::Enter_LTG_DestroyDeployable() {
    
	ltgType = DESTROY_DEPLOYABLE_GOAL;

	LTG_AI_SUB_NODE = &idBotAI::LTG_DestroyDeployable;
	
	ltgTime = botWorld->gameLocalInfo.time + 60000;

	ltgReached = false;

	ltgTimer = 0;

	ltgReachedTarget = false;

	rocketLauncherLockFailedTime = 0;

	ltgMoveDir = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? RIGHT : LEFT;

	lastAINode = "Destroying Deployable";

	return true;	
}

/*
================
idBotAI::LTG_DestroyDeployable
================
*/
bool idBotAI::LTG_DestroyDeployable() {
	bool useArty = false;
	bool wantsMove = false;
	bool isVisible = false;
	bool attackGoal = false;
	float dist;
	deployableInfo_t deployableInfo;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
		Bot_ExitAINode();
		return false;
	}

	if ( !GetDeployableInfo( false, ltgTarget, deployableInfo ) ) { //mal: doesn't exist anymore.
		Bot_ExitAINode();
		return false;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
		if ( Bot_CheckForHumanInteractingWithEntity( deployableInfo.entNum ) == true ) {
			Bot_ExitAINode();
			return false;
		}
	}

	if ( ClientHasFireSupportInWorld( botNum ) ) { //mal: fire and forget if firesupport on its way.
		Bot_ExitAINode();
		Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
		return false;
	}

	if ( ( deployableInfo.disabled && botInfo->classType == COVERTOPS ) || deployableInfo.health < ( deployableInfo.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) { //mal: its dead ( enough ).
		Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_HasExplosives( false ) ) {
#ifdef PACKS_HAVE_NO_NADES
		Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
		Bot_ExitAINode();
		return false;
#else 
		if ( deployableInfo.enemyEntNum != botNum ) { //mal: is it safe to stand here?
			if ( ( botInfo->classType == MEDIC && botInfo->team == STROGG ) || ( botInfo->classType == FIELDOPS && botInfo->team == GDF ) ) {
				if ( supplySelfTime < botWorld->gameLocalInfo.time ) {
					supplySelfTime = botWorld->gameLocalInfo.time + 5000;
				}
				return true; //mal: we'll wait here while we resupply ourselves.
			}
		} else {
			Bot_ExitAINode();
			return false;
		}
#endif
	}

	vec = deployableInfo.origin - botInfo->origin;
	dist = vec.LengthSqr();

	idVec3 deployableOrigin = deployableInfo.origin;
	deployableOrigin.z += DEPLOYABLE_ORIGIN_OFFSET;

	trace_t tr;
	botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, deployableOrigin, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ) );

	if ( tr.fraction == 1.0f || tr.c.entityNum == deployableInfo.entNum ) { 
		isVisible = true;
	}

	if ( isVisible ) {
		if ( botInfo->classType == FIELDOPS && LocationVis2Sky( deployableInfo.origin ) && ClassWeaponCharged( AIRCAN ) && Bot_HasWorkingDeployable() && deployableInfo.type != AIT && !Bot_EnemyAITInArea( deployableInfo.origin ) ) {
			attackGoal = true;
		} else if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo && dist < Square( WEAPON_LOCK_DIST ) ) {
			attackGoal = true;
		} else if ( dist < Square( ATTACK_APT_DIST ) ) {
			attackGoal = true;
		}
	}

	if ( !attackGoal ) {
		botUcmd->actionEntityNum = deployableInfo.entNum;
		botUcmd->actionEntitySpawnID = deployableInfo.spawnID;

		Bot_SetupMove( deployableOrigin, -1, ACTION_NULL );
		
		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? SPRINT : RUN );
		return true;
	}

	ignoreWeapChange = true;

	if ( dist < Square( ATTACK_APT_DIST ) ) {
		if ( botInfo->isDisguised ) {
			botUcmd->botCmds.dropDisguise = true;
		}

		if ( botInfo->classType == FIELDOPS && LocationVis2Sky( deployableInfo.origin ) && ClassWeaponCharged( AIRCAN ) ) {
			botIdealWeapNum = AIRCAN;
			botIdealWeapSlot = NO_WEAPON;
		} else if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo ) {
			botIdealWeapSlot = GUN;
		} else if ( botInfo->classType == COVERTOPS && botInfo->team == GDF && ClassWeaponCharged( THIRD_EYE ) ) {
			botIdealWeapSlot = NO_WEAPON;
			botIdealWeapNum = THIRD_EYE;
		} else if ( botInfo->weapInfo.hasNadeAmmo ) {
			botIdealWeapSlot = NADE;
			botIdealWeapNum = NULL_WEAP;
		} else {
			Bot_ExitAINode();
			return false;
		}
	} else {
		if ( botInfo->classType == FIELDOPS && ClassWeaponCharged( AIRCAN ) && Bot_HasWorkingDeployable() && deployableInfo.type != AIT && !Bot_EnemyAITInArea( deployableInfo.origin ) ) {
			useArty = true;
		} else if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo ) {
			botIdealWeapSlot = GUN;
		} else {
			Bot_ExitAINode();
			return false;
		}
	}

	if ( deployableInfo.enemyEntNum == botNum && !useArty ) {
		wantsMove = true;
	}

	if ( useArty ) {
		Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );

		vec = deployableInfo.origin - botInfo->origin;
		idVec3 org = deployableInfo.origin;

		if ( vec.z < 20.0f ) {
			org.z += 16.0f;
		}

		Bot_LookAtLocation( org, SMOOTH_TURN );
		botIdealWeapNum = BINOCS;
		botIdealWeapSlot = NO_WEAPON;

		if ( ltgTimer == 0 ) {
			ltgTimer = botWorld->gameLocalInfo.time + ARTY_LOCKON_TIME;
		}
		
		if ( ltgTimer > botWorld->gameLocalInfo.time ) {
			return true;
		}

		if ( botInfo->weapInfo.weapon == BINOCS ) {
			botUcmd->botCmds.attack = true;
			botUcmd->botCmds.constantFire = true;
			return true;
		}
	} else if ( botInfo->weapInfo.weapon == GRENADE || botInfo->weapInfo.weapon == EMP ) {
		bool fastNade = true;

		if ( dist > Square( GRENADE_THROW_MAXDIST ) ) {
			Bot_SetupMove( deployableOrigin, -1, ACTION_NULL );
		
			if ( MoveIsInvalid() ) {
				Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
				Bot_ExitAINode();
				return false;
			}

			Bot_MoveAlongPath( SPRINT );
			fastNade = false;
		}

		idVec3 deployableOrigin = deployableInfo.origin;
		deployableOrigin.z += 16.0f;
		Bot_ThrowGrenade( deployableOrigin, fastNade );
	} else if ( botInfo->weapInfo.weapon == AIRCAN ) {
		Bot_UseCannister( AIRCAN, deployableInfo.origin );
	} else if ( botInfo->weapInfo.weapon == THIRD_EYE ) {
		if ( !ClassWeaponCharged( THIRD_EYE ) ) {
			if ( !ltgReachedTarget ) { //mal: if just got here, find a nearby action to move towards, just to get us out of here.
				actionNum = Bot_FindNearbySafeActionToMoveToward( deployableOrigin, THIRD_EYE_SAFE_RANGE );

				if ( actionNum == ACTION_NULL ) { //mal: OH NOES!1 Panic and lets get out of here.
					Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
					Bot_ExitAINode();
					return false;
				}

				ltgReachedTarget = true;
			}

			if ( dist >= Square( THIRD_EYE_SAFE_RANGE ) ) {
				botUcmd->botCmds.attack = true;
				Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
				Bot_LookAtLocation( deployableOrigin, SMOOTH_TURN );
			} else {
				Bot_SetupMove( botThreadData.botActions[ actionNum ]->GetActionOrigin(), -1, ACTION_NULL );
		
				if ( MoveIsInvalid() ) {
					Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
					Bot_ExitAINode();
					return false;
				}

				Bot_MoveAlongPath( SPRINT );
			}
		} else {
			Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
			Bot_LookAtLocation( deployableOrigin, SMOOTH_TURN ); 
			botUcmd->botCmds.attack = true;
		}
	} else if ( botIdealWeapSlot == GUN || botIdealWeapSlot == SIDEARM ) {
		if ( dist < Square( 500.0f ) ) { // too close, back up some!  
			if ( Bot_CanMove( BACK, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			}

			Bot_LookAtLocation( deployableOrigin, SMOOTH_TURN ); 
		} else {
			Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
			Bot_LookAtLocation( deployableOrigin, SMOOTH_TURN ); 
		}

		if ( ltgReached == false ) {
			rocketLauncherLockFailedTime = botWorld->gameLocalInfo.time + 5000;
			ltgReached = true;
		} else {
			botUcmd->botCmds.altAttackOn = true;
		}

		if ( rocketLauncherLockFailedTime < botWorld->gameLocalInfo.time && botInfo->targetLockEntNum != deployableInfo.entNum ) { //mal: can't get a lock for some reason - ignore this deployable.
			Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
			Bot_ExitAINode();
			return false;
		}

		if ( botInfo->targetLocked != false ) {
			if ( botInfo->targetLockEntNum == deployableInfo.entNum ) {
				botUcmd->botCmds.attack = true;
				rocketLauncherLockFailedTime = botWorld->gameLocalInfo.time + 10000;
			} else {
				botUcmd->botCmds.altAttackOn = false;
				ltgReached = false;
			}
		}
	}

	if ( wantsMove ) {
		if ( dist < Square( ATTACK_APT_DIST ) ) {
			vec = deployableOrigin;
			vec += ( -350.0f * deployableInfo.axis[ 0 ] );

			botUcmd->actionEntityNum = deployableInfo.entNum;
			botUcmd->actionEntitySpawnID = deployableInfo.spawnID;

			Bot_SetupMove( vec, -1, ACTION_NULL );

			if ( MoveIsInvalid() ) {
				Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
				Bot_ExitAINode();
				return false;
			}

			Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? SPRINT : RUN );
		} else {
			if ( ltgMoveDir == NULL_DIR ) {
				ltgMoveDir = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? RIGHT : LEFT;
			}

			if ( Bot_CanMove( ltgMoveDir, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, ( ltgMoveDir == RIGHT ) ? RANDOM_JUMP_RIGHT : RANDOM_JUMP_LEFT );
			} else {
				ltgMoveDir = NULL_DIR;
			}
		}
	}

	return true;	
}

/*
================
idBotAI_LTG::Enter_LTG_FireSupportGoal

================
*/
bool idBotAI::Enter_LTG_FireSupportGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_FireSupportGoal;

	ltgType = FIRESUPPORT_CAMP_GOAL;
	
	stayInPosition = false;

	ltgReached = false;
	
	ltgTimer = 0;

	ltgExit = false;

//mal: higher skill bots are more likely to respond to heard sounds, and go after the person making them, then lower skilled bots.
	if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) {
		stayInPosition = true;
	} else if ( botThreadData.GetBotSkill() == BOT_SKILL_NORMAL ) {
		if ( botThreadData.random.RandomInt( 100 ) > 30 ) {
			stayInPosition = true;
		}
	} else {
		if ( botThreadData.random.RandomInt( 100 ) > 70 ) {
			stayInPosition = true;
		}
	}

	lastAINode = "FireSupport Camp Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_FireSupportGoal
================
*/
bool idBotAI::LTG_FireSupportGoal() {
	int vehicleNum;
	float dist;
	idVec3 vec;

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( ClientHasFireSupportInWorld( botNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	deployableInfo_t deployableInfo;

	if ( !GetDeployableInfo( true, -1, deployableInfo ) ) { //mal: doesn't exist anymore.
		Bot_ExitAINode();
		return false;
	}

	if ( deployableInfo.health < ( deployableInfo.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( botThreadData.botActions[ actionNum ]->blindFire == true && ltgExit == false ) {
		if ( botWorld->gameLocalInfo.time + 10000 > ltgTime && ClassWeaponCharged( AIRCAN ) ) {
			ltgExit = true;
			int k = 0;
			int i;
			int linkedTargets[ MAX_LINKACTIONS ];
		
			for( i = 0; i < MAX_LINKACTIONS; i++ ) {
				if ( botThreadData.botActions[ actionNum ]->actionTargets[ i ].inuse != false ) {
					linkedTargets[ k++ ] = i;
				}
			}

			i = botThreadData.random.RandomInt( k );
			ltgOrigin = botThreadData.botActions[ actionNum ]->actionTargets[ i ].origin;
		}
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( actionNumInsideDanger == actionNum ) { //mal: if our action is encased inside a danger ( landmine ), and we're not destroying it, must not be able to, so leave.
		Bot_ExitAINode();
		return false;
	}

	if ( ltgUseVehicle ) {
		vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ actionNum ]->GetActionVehicleFlags( botInfo->team ), NULL_VEHICLE_FLAGS, true );

		if ( vehicleNum == -1 ) {
			ltgUseVehicle = false;
			return true;
		}

        if ( !Bot_GetIntoVehicle( vehicleNum ) ) {
			ltgUseVehicle = false;
		}

		return true;
	}

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );

		return true;
	}

	if ( ltgExit == false && ( botThreadData.random.RandomInt( 100 ) > 98 || ltgReached == false ) ) {
		int k = 0;
		int i;
		int linkedTargets[ MAX_LINKACTIONS ];
		
		for( i = 0; i < MAX_LINKACTIONS; i++ ) {
			if ( botThreadData.botActions[ actionNum ]->actionTargets[ i ].inuse != false ) {
				linkedTargets[ k++ ] = i;
			}
		}

		i = botThreadData.random.RandomInt( k );

		Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->actionTargets[ i ].origin, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
		ltgOrigin = botThreadData.botActions[ actionNum ]->actionTargets[ i ].origin;
	}

	if ( ltgReached == false ) {
		ltgTime = botWorld->gameLocalInfo.time + ( botThreadData.botActions[ actionNum ]->actionTimeInSeconds * 1000 );
		ltgReached = true;
	}

	int enemiesInArea = ClientsInArea( botNum, ltgOrigin, 1700.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, false, false );
	int friendsInArea = ClientsInArea( botNum, ltgOrigin, 900.0f, botInfo->team , NOCLASS, false, false, false, false, false );

	bool deployablesInArea = Bot_CheckTeamHasDeployableTypeNearLocation( ( botInfo->team == GDF ) ? STROGG : GDF, NULL_DEPLOYABLE, ltgOrigin, 1700.0f );

	if ( ( enemiesInArea == 0 && deployablesInArea == false && ltgExit == false ) || friendsInArea > 2 ) { //mal: no target of value in this area, or too many friendlies, so dont bother.
		ltgTimer = 0;
		return true;
	}

	botIdealWeapNum = BINOCS;
	botIdealWeapSlot = NO_WEAPON;
	ignoreWeapChange = true;

	Bot_LookAtLocation( ltgOrigin, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.

	if ( ltgTimer == 0 ) {
		ltgTimer = botWorld->gameLocalInfo.time + ARTY_LOCKON_TIME;
	}
	
	if ( ltgTimer > botWorld->gameLocalInfo.time ) {
		return true;
	}

	if ( botInfo->weapInfo.weapon == BINOCS ) {
		botUcmd->botCmds.attack = true;
		botUcmd->botCmds.constantFire = true;
	}

	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_HackDeployableGoal

The bot has a deployable on the map he wants to hack.
================
*/
bool idBotAI::Enter_LTG_HackDeployableGoal() {

	ltgType = HACK_DEPLOYABLE_GOAL;

	ltgTime = botWorld->gameLocalInfo.time + DEFAULT_LTG_TIME;

	LTG_AI_SUB_NODE = &idBotAI::LTG_HackDeployableGoal;
	
	lastAINode = "Hack Deployable Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_HackDeployableGoal
================
*/
bool idBotAI::LTG_HackDeployableGoal() {

	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	deployableInfo_t deployable;
	
	if ( !GetDeployableInfo( false, ltgTarget, deployable ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( deployable.disabled || deployable.health < ( deployable.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) {
		Bot_ExitAINode();
		return false;
	}

	int travelTime;

	if ( !Bot_LocationIsReachable( false, deployable.origin, travelTime ) ) {
		Bot_ExitAINode();
		return false;
	}

	botUcmd->actionEntityNum = ltgTarget; //mal: let the game and obstacle avoidance know we want to interact with this entity.
	botUcmd->actionEntitySpawnID = deployable.spawnID;

	idVec3 vec = deployable.origin - botInfo->origin;
	float distSqr = vec.LengthSqr();

	if ( distSqr > Square( 512.0f ) || botInfo->onLadder ) {
		idVec3 moveGoal = deployable.origin;
		moveGoal.z += DEPLOYABLE_PATH_ORIGIN_OFFSET;

		Bot_SetupMove( moveGoal, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreDeployable( deployable.entNum, DEPLOYABLE_IGNORE_TIME );//mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( distSqr > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( deployable.origin ) : RUN );
		return true;
	}

//mal: reached our action - pop into the short term goal AI node to execute it!
	nbgTarget = ltgTarget;
    ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_HackDeployable;
	return true;
}

/*
================
idBotAI::Enter_LTG_MG_CampGoal

The bot has a MG nest on the map he wants to man.
================
*/
bool idBotAI::Enter_LTG_MG_CampGoal() {

	ltgType = MG_CAMP_GOAL;

	ltgTime = botWorld->gameLocalInfo.time + DEFAULT_LTG_TIME;

	LTG_AI_SUB_NODE = &idBotAI::LTG_MG_CampGoal;
	
	lastAINode = "MG Camp Goal";

	return true;
}

/*
================
idBotAI::LTG_MG_CampGoal
================
*/
bool idBotAI::LTG_MG_CampGoal() {
	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( botInfo->isDisguised ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->ActionIsActive() ) {
		Bot_ExitAINode();
		return false;
	}

	if ( botThreadData.botActions[ actionNum ]->GetActionState() == ACTION_STATE_GUN_DESTROYED ) {
		Bot_ExitAINode();
		return false;
	}

	idVec3 vec = botThreadData.botActions[ actionNum ]->GetActionOrigin() - botInfo->origin;
	float distSqr = vec.LengthSqr();

	if ( distSqr > Square( 150.0f ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME );//mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( distSqr > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->GetActionOrigin() ) : RUN );
		return true;
	}

//mal: reached our action - pop into the short term goal AI node to execute it!
    ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_MG_Camp;
	return true;
}

/*
================
idBotAI::Enter_LTG_FlyerHiveGoal
================
*/
bool idBotAI::Enter_LTG_FlyerHiveGoal() {

	ltgType = FLYER_HIVE_GOAL;

	ltgTime = botWorld->gameLocalInfo.time + DEFAULT_LTG_TIME;

	LTG_AI_SUB_NODE = &idBotAI::LTG_FlyerHiveGoal;
	
	lastAINode = "Flyer Hive Goal";

	return true;
}

/*
================
idBotAI::LTG_FlyerHiveGoal
================
*/
bool idBotAI::LTG_FlyerHiveGoal() {
	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->ActionIsActive() ) {
		Bot_ExitAINode();
		return false;
	}

	idVec3 vec = botThreadData.botActions[ actionNum ]->GetActionOrigin() - botInfo->origin;
	float distSqr = vec.LengthSqr();

	if ( distSqr > Square( 150.0f ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME );//mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( distSqr > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->GetActionOrigin() ) : RUN );
		return true;
	}

	if ( botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].inuse == false ) {
		if ( botThreadData.AllowDebugData() ) {
			botThreadData.Warning("Map Error! No action target for the bot to look at! The flyer hive launch entity needs exactly 1 action target!" );
		}

		Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].origin, INSTANT_TURN );

	int i;
	idList< int > hiveTargets; //mal: look for a target for this hive to fly to.

	for( i = 0; i < botThreadData.botActions.Num(); i++ ) {
		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) != ACTION_FLYER_HIVE_TARGET ) {
			continue;
		}

		hiveTargets.Append( i );
	}

	if ( hiveTargets.Num() == 0 ) {
		if ( botThreadData.AllowDebugData() ) {
			botThreadData.Warning("Map Error! No valid flyer hive target found for flyer hive goal!" );
		}

		Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	if ( hiveTargets.Num() == 1 ) {
		actionNum = hiveTargets[ 0 ];
	} else {
		int j = botThreadData.random.RandomInt( hiveTargets.Num() );
		actionNum = hiveTargets[ j ];
	}

//mal: reached our action - pop into the short term goal AI node to execute it!
    ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_FlyerHive;
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_RepairGunGoal
================
*/
bool idBotAI::Enter_LTG_RepairGunGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_RepairGunGoal;

	ltgType = MG_REPAIR_GOAL;

	lastAINode = "Repair Gun Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_RepairGunGoal
================
*/
bool idBotAI::LTG_RepairGunGoal() {
	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( botThreadData.botActions[ actionNum ]->GetActionState() == ACTION_STATE_GUN_READY ) { //mal: someone else fixed it.
		Bot_ExitAINode();
		return false;
	}

	idVec3 vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	float actionRadius = ( botThreadData.botActions[ actionNum ]->radius <= 40.0f ) ? 50.0f : botThreadData.botActions[ actionNum ]->radius; //mal: hack to override bad radius values. Less then 40 is usually bad.
	float dist = vec.LengthSqr();

	if ( dist > Square( actionRadius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
		return true;
	}

//mal: reached our action - pop into the short term goal AI node to execute it!
    ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
	NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_FixGun;
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_EnterVehicleGoal
================
*/
bool idBotAI::Enter_LTG_EnterVehicleGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_EnterVehicleGoal;

	ltgType = ENTER_VEHICLE_GOAL;

	lastAINode = "Enter Vehicle Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_EnterVehicleGoal
================
*/
bool idBotAI::LTG_EnterVehicleGoal() {
	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreVehicle( ltgTarget, 15000 );
		Bot_ExitAINode();
		return false;
	}

	proxyInfo_t vehicleInfo;
	GetVehicleInfo( ltgTarget, vehicleInfo );

	if ( vehicleInfo.entNum == 0 || !VehicleIsValid( vehicleInfo.entNum ) ) {
		Bot_IgnoreVehicle( vehicleInfo.entNum, 15000 );
		Bot_ExitAINode();
		return false;
	}

	idVec3 vehicleOrigin = vehicleInfo.origin;
	vehicleOrigin.z += VEHICLE_PATH_ORIGIN_OFFSET;

	botUcmd->actionEntityNum = vehicleInfo.entNum;
	botUcmd->actionEntitySpawnID = vehicleInfo.spawnID;

	Bot_SetupMove( vehicleOrigin, -1, ACTION_NULL );

	if ( MoveIsInvalid() ) {
		Bot_IgnoreVehicle( vehicleInfo.entNum, 15000 );
		Bot_ExitAINode();
		return false;
	}

	Bot_MoveAlongPath( Bot_ShouldStrafeJump( vehicleOrigin ) );

	idVec3 vec = vehicleInfo.origin - botInfo->origin;

//mal: if we're fairly close, start sending the vehicle use cmd
	if ( vec.LengthSqr() < Square( 350.0f ) ) {
		Bot_LookAtLocation( vehicleInfo.origin, SMOOTH_TURN );
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
            botUcmd->botCmds.enterVehicle = true;
		}
	}
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_ActorEscortPlayerToGoal
================
*/
bool idBotAI::Enter_LTG_ActorEscortPlayerToGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_ActorEscortPlayerToGoal;

	ltgType = ACTOR_GOAL;

	lastAINode = "Actor Goal";

	ltgReached = false;

	ltgTimer = 0;

	ltgCounter = 0;

	ltgTime = 0;

	ltgReachedTarget = false;

	if ( botThreadData.actorMissionInfo.hasBriefedPlayer ) {
		ltgChat = false;
	} else {
		ltgChat = true;

		if ( !botThreadData.actorMissionInfo.hasEnteredActorNode ) {
			ltgCounter = botWorld->gameLocalInfo.time + botThreadData.actorMissionInfo.goalPauseTime;
			botUcmd->botCmds.actorHasEnteredActorNode = true;
		}
	}

	ltgMoveTime = 0;

	return true;
}

/*
================
idBotAI_LTG::LTG_ActorEscortPlayerToGoal
================
*/
bool idBotAI::LTG_ActorEscortPlayerToGoal() {
	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;	
	}

	if ( botThreadData.botActions[ actionNum ]->GetObjForTeam( botInfo->team ) == ACTION_DENY_SPAWNPOINT && botThreadData.botActions[ actionNum ]->GetTeamOwner() == NOTEAM ) {
		Bot_ExitAINode();
		return false;
	}

	if ( botThreadData.botActions[ actionNum ]->GetObjForTeam( botInfo->team ) == ACTION_FORWARD_SPAWN && botThreadData.botActions[ actionNum ]->GetTeamOwner() == botInfo->team ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !ClientIsValid( ltgTarget, ltgTargetSpawnID ) ) {
		Bot_ExitAINode();
		botUcmd->botCmds.actorSurrenderStatus = true;
		return false;
	}

	if ( ltgCounter > botWorld->gameLocalInfo.time ) {
		if ( botThreadData.random.RandomInt( 100 ) > 94 ) {
			idVec3 vec;
			if ( Bot_RandomLook( vec ) )  {
				Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot. 
			}
		}
		return true;
	}

	const clientInfo_t& player = botWorld->clientInfo[ ltgTarget ];

	if ( ltgTime > botWorld->gameLocalInfo.time ) {
		Bot_LookAtLocation( player.viewOrigin, SMOOTH_TURN );
		return true;
	}	

	if ( player.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		Bot_ExitAINode();
		botUcmd->botCmds.actorSurrenderStatus = true;
		return false;
	}

	if ( player.inLimbo || ( !player.hasGroundContact && player.invulnerableEndTime > botWorld->gameLocalInfo.time ) ) { //mal: if our player gets killed, don't do anything til he respawns.
		if ( botThreadData.random.RandomInt( 100 ) > 95 ) {
			idVec3 vec;
			Bot_RandomLook( vec );
			Bot_LookAtLocation( vec, SMOOTH_TURN );
		}

		ltgReached = false;
		Bot_MoveToGoal( vec3_zero, vec3_zero, CROUCH, NULLMOVETYPE );
		return true;
	}

	if ( player.health <= 0 || ( player.revived && player.spawnTime + 1000 > botWorld->gameLocalInfo.time ) ) { //mal: if our player died, or was just revived, just exit for now.
		return true;
	}

	idVec3 vec = player.origin - botInfo->origin;
	vec.z = 0.0f;
	float botDistToPlayerSqr = vec.LengthSqr();

	vec = botThreadData.botActions[ actionNum ]->origin - player.origin;
	vec.z = 0.0f;
	float playerDistToGoalSqr = vec.LengthSqr();

	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	vec.z = 0.0f;
	float botDistToGoalSqr = vec.LengthSqr();

	if ( botThreadData.botActions[ actionNum ]->GetObjForTeam( botInfo->team ) == ACTION_HE_CHARGE && ClientHasChargeInWorld( ltgTarget, true, actionNum ) ) {
		if ( botDistToPlayerSqr > Square( 175.0f ) ||  botInfo->onLadder ) {
			Bot_SetupMove( vec3_zero, ltgTarget, ACTION_NULL );

			if ( MoveIsInvalid() ) {
				Bot_ExitAINode();
				return false;
			}

			Bot_MoveAlongPath( SPRINT );
			return true;
		}

		
		botMoveFlags_t defaultMove = NULLMOVEFLAG;

		if ( player.posture == IS_CROUCHED  ) {
			defaultMove = CROUCH;
		}

		Bot_MoveToGoal( vec3_zero, vec3_zero, defaultMove, NULLMOVETYPE );

		if ( botThreadData.random.RandomInt( 100 ) > 94 ) {
			if ( Bot_RandomLook( vec ) )  {
				Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
			}
		}
		return true;
	} //mal: cover our player while we wait down the charge counter.

	if ( botThreadData.botActions[ actionNum ]->GetObjForTeam( botInfo->team ) == ACTION_HE_CHARGE && ClientHasChargeInWorld( ltgTarget, false, actionNum, true ) ) {
		plantedChargeInfo_t bombInfo;
		FindChargeInWorld( actionNum, bombInfo, ARM );

		vec = bombInfo.origin - player.origin;

		if ( vec.LengthSqr() > Square( 200.0f ) && botThreadData.random.RandomInt( 1000 ) > 900 ) {
			Bot_AddDelayedChat( botNum, GOT_YOUR_BACK, 1 ); //mal: stay away from that bomb!
		}
	} //mal: woah buddy! Get back there and arm your charge!

	if ( ltgReached == false ) {
		if ( ltgMoveTime < botWorld->gameLocalInfo.time && ( botDistToPlayerSqr > Square( 1000.0f ) || ltgReachedTarget == false || botInfo->onLadder ) ) {
			Bot_SetupMove( vec3_zero, ltgTarget, ACTION_NULL );

			if ( MoveIsInvalid() ) {
				Bot_ExitAINode();
				return false;
			}

			ltgMoveTime = 0;

			if ( ltgReachedTarget == false ) { // was 150 below
				if ( botDistToPlayerSqr < Square( 200.0f ) && player.hasGroundContact && botThreadData.Nav_IsDirectPath( AAS_PLAYER, botInfo->team, botInfo->areaNum, botInfo->origin, player.origin ) ) {
					ltgReachedTarget = true;
					if ( ltgChat == true ) {
						botUcmd->botCmds.actorBriefedPlayer = true; //mal: let the game know we've briefed the player, so we don't do it again.
						ltgPauseTime = botWorld->gameLocalInfo.time + ( 1000 * botThreadData.actorMissionInfo.chatPauseTime );
						timeTilAttackEnemy = -1;
						enemy = -1;
					} else {

					}
				}
			}

			Bot_MoveAlongPath( ( botDistToPlayerSqr < Square( 250.0f ) && player.xySpeed < RUNNING_SPEED ) ? RUN : SPRINT );
			return true;
		} else if ( ltgMoveTime > botWorld->gameLocalInfo.time || ( botDistToPlayerSqr > Square( 900.0f ) && playerDistToGoalSqr > botDistToGoalSqr ) ) {
			if ( botThreadData.random.RandomInt( 100 ) > 98 ) {
			}
			Bot_LookAtLocation( player.viewOrigin, SMOOTH_TURN );

			if ( ltgMoveTime == 0 ) {
				ltgMoveTime = botWorld->gameLocalInfo.time + 1500;
			}
			return true;
		}
	}

	if ( ltgPauseTime > botWorld->gameLocalInfo.time ) {
		Bot_LookAtLocation( player.viewOrigin, SMOOTH_TURN );
		botUcmd->botCmds.actorIsBriefingPlayer = true;
		return true;
	}

	if ( botDistToGoalSqr > Square( 500.0f ) || botInfo->onLadder ) {	// is 500 too close, or too far?

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

//		botMoveFlags_t botMoveFlags = RUN;

		botMoveFlags_t botMoveFlags = SPRINT;

		if ( playerDistToGoalSqr < botDistToGoalSqr ) {
			botMoveFlags = SPRINT;
		}

		ltgMoveTime = 0;


//		if ( botDistToPlayerSqr > Square( 500.0f ) && playerDistToGoalSqr > botDistToGoalSqr ) {
//			botMoveFlags = RUN;
//		} // else if ( ( botDistToPlayerSqr < Square( 350.0f ) && player.xySpeed >= RUNNING_SPEED ) || playerDistToGoalSqr < botDistToGoalSqr ) {
//			botMoveFlags = SPRINT;
//		}

		Bot_MoveAlongPath( botMoveFlags );
		return true;
	}

	//mal: from here on down, we've reached the goal, and are merely defending the player while he does his work....

	int playerAttacker = CheckClientAttacker( ltgTarget, 3 ); //mal: if our friend is under attack, move towards him to protect him.

	if ( playerAttacker != -1 && botDistToPlayerSqr > Square( 75.0f ) ) {
		Bot_SetupMove( vec3_zero, ltgTarget, ACTION_NULL );
		
		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( RUN );
		return true;
	}

	if ( ltgReached == false ) {
		ltgReached = true;
		ltgTimer = botWorld->gameLocalInfo.time + 5000;
	}

	if ( ltgTimer > botWorld->gameLocalInfo.time ) {
		Bot_LookAtEntity( ltgTarget, SMOOTH_TURN ); //look at our friend when first reach goal
	} else if ( botThreadData.random.RandomInt( 100 ) > 94 ) {
		if ( Bot_RandomLook( vec ) )  {
			Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
		}
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, CROUCH, NULLMOVETYPE );
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_ActorGiveFinalBriefingToPlayer
================
*/
bool idBotAI::Enter_LTG_ActorGiveFinalBriefingToPlayer() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_ActorGiveFinalBriefingToPlayer;

	ltgType = ACTOR_GOAL;

	lastAINode = "Last Actor Goal";

	ltgReached = false;

	ltgCounter = botWorld->gameLocalInfo.time + botThreadData.actorMissionInfo.goalPauseTime;

	ltgPauseTime = 0;

	return true;
}

/*
================
idBotAI_LTG::LTG_ActorGiveFinalBriefingToPlayer
================
*/
bool idBotAI::LTG_ActorGiveFinalBriefingToPlayer() {
	if ( !ClientIsValid( ltgTarget, ltgTargetSpawnID ) ) {
		Bot_ExitAINode();
		botUcmd->botCmds.actorSurrenderStatus = true;
		return false;
	}

	if ( ltgCounter > botWorld->gameLocalInfo.time ) {
		if ( botThreadData.random.RandomInt( 100 ) > 94 ) {
			idVec3 vec;
			if ( Bot_RandomLook( vec ) )  {
				Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
			}
		}
		return true;
	}

	const clientInfo_t& player = botWorld->clientInfo[ ltgTarget ];

	if ( player.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		Bot_ExitAINode();
		botUcmd->botCmds.actorSurrenderStatus = true;
		return false;
	}

	if ( player.inLimbo || ( !player.hasGroundContact && player.invulnerableEndTime > botWorld->gameLocalInfo.time ) ) { //mal: if our player gets killed, don't do anything til he respawns.
		if ( botThreadData.random.RandomInt( 100 ) > 95 ) {
			idVec3 vec;
			Bot_RandomLook( vec );
			Bot_LookAtLocation( vec, SMOOTH_TURN );
		}

		ltgReached = false;
		Bot_MoveToGoal( vec3_zero, vec3_zero, CROUCH, NULLMOVETYPE );
		return true;
	}

	idVec3 vec = player.origin - botInfo->origin;
	vec.z = 0.0f;
	float botDistToPlayerSqr = vec.LengthSqr();

	if ( !ltgReached ) {
		if ( botDistToPlayerSqr > Square( 150.0f ) || botInfo->onLadder ) {
			Bot_SetupMove( vec3_zero, ltgTarget, ACTION_NULL );

			if ( MoveIsInvalid() ) {
				Bot_ExitAINode();
				return false;
			}

			Bot_MoveAlongPath( SPRINT );
			return true;
		} else {
			ltgReached = true;
			botUcmd->botCmds.actorBriefedPlayer = true; //mal: let the game know we've briefed the player, so we don't do it again.
			ltgPauseTime = botWorld->gameLocalInfo.time + ( 1000 * botThreadData.actorMissionInfo.chatPauseTime );
		}
	}

	if ( ltgPauseTime > botWorld->gameLocalInfo.time ) {
		Bot_LookAtLocation( player.viewOrigin, SMOOTH_TURN );
		botUcmd->botCmds.actorIsBriefingPlayer = true;
		if ( botDistToPlayerSqr < Square( 100.0f ) ) { // too close, back up some!  
			if ( Bot_CanMove( BACK, 75.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
			} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
			} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
			}
		}
		return true;
	}

	botUcmd->botCmds.actorSurrenderStatus = true;
	Bot_ExitAINode();
	return false;
}

/*
================
idBotAI_LTG::Enter_LTG_ThirdEyeCameraGoal
================
*/
bool idBotAI::Enter_LTG_ThirdEyeCameraGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_ThirdEyeCameraGoal;

	ltgTime = botWorld->gameLocalInfo.time + 60000;

	ltgType = THIRD_EYE_GOAL;
	
	lastAINode = "Third Eye Goal";

	ltgReached = false;

	ltgPauseTime = 0;

	return true;
}

/*
================
idBotAI_LTG::LTG_ThirdEyeCameraGoal
================
*/
bool idBotAI::LTG_ThirdEyeCameraGoal() {
	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	if ( actionNumInsideDanger == actionNum ) { //mal: if our action is encased inside a danger ( landmine ), and we're not destroying it, must not be able to, so leave.
		Bot_ExitAINode();
		return false;
	}

	if ( !ClassWeaponCharged( THIRD_EYE ) || botInfo->isDisguised ) {
		Bot_ExitAINode();
		return false;
	}

	idVec3 vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	float distSqr = vec.LengthSqr();

	if ( distSqr > Square( botThreadData.botActions[ actionNum ]->radius ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( distSqr > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
		return true;
	}

	if ( !ltgReached ) {
		ltgReached = true;
		ltgPauseTime = botWorld->gameLocalInfo.time + 1000;
	}		

	botIdealWeapNum = THIRD_EYE;
	botIdealWeapSlot = NO_WEAPON;
	ignoreWeapChange = true;

	Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->actionBBox.GetCenter(), SMOOTH_TURN );

	if ( botInfo->weapInfo.weapon == THIRD_EYE && ltgPauseTime < botWorld->gameLocalInfo.time ) {
		botUcmd->botCmds.attack = true;
	}

	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_ProtectCharge
================
*/
bool idBotAI::Enter_LTG_ProtectCharge() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_ProtectCharge;

	ltgTime = botWorld->gameLocalInfo.time + 40000;

	ltgType = PROTECT_CHARGE;
	
	lastAINode = "Protect Charge";

	ltgDist = 300.0f + ( ( float ) botThreadData.random.RandomInt( 500 ) );

	ltgPosture = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? CROUCH : WALK;

	ltgReached = false;

	ltgUseMine = true;

	ltgChat = false; //mal: track whether or not we should use airstrike on charge.

	if ( botInfo->classType == FIELDOPS ) {
		plantedChargeInfo_t charge;

		if ( FindChargeBySpawnID( ltgTargetSpawnID, charge ) ) {
			if ( LocationVis2Sky( charge.origin ) ) {
				ltgChat = true;
				ltgDist = 900.0f;
			}
		}
	}

	return true;
}

/*
================
idBotAI_LTG::LTG_ProtectCharge
================
*/
bool idBotAI::LTG_ProtectCharge() {
	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		ignorePlantedChargeTime = botWorld->gameLocalInfo.time + IGNORE_PLANTED_CHARGE_TIME;
		Bot_ExitAINode();
		return false;
	}

	plantedChargeInfo_t charge;

	if ( !FindChargeBySpawnID( ltgTargetSpawnID, charge ) ) {
		ignorePlantedChargeTime = botWorld->gameLocalInfo.time + IGNORE_PLANTED_CHARGE_TIME;
		Bot_ExitAINode();
		return false;
	}

	bool isVisible = false;
	idVec3 vec = charge.origin - botInfo->origin;
	float distSqr = vec.LengthSqr();

	int enemyEngsInArea = ClientsInArea( botNum, charge.origin, 300.0f, ( botInfo->team == GDF ) ? STROGG : GDF, ENGINEER, false, false, false, false, false );

	if ( enemyEngsInArea > 0 ) { //mal: do this in case we aren't visible to the charge we're protecting, and we don't just sit there and wait while its defused.
		ltgDist = 150.0f;
	}

	trace_t tr;
	botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, charge.origin, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ) );

	if ( tr.fraction == 1.0f || tr.c.entityNum == charge.entNum ) { 
		isVisible = true;
	}

	if ( distSqr > Square( ltgDist ) || botInfo->onLadder || botInfo->inWater || !isVisible ) {

		Bot_SetupMove( charge.origin, -1, ACTION_NULL, charge.areaNum );

		if ( MoveIsInvalid() ) {
			ignorePlantedChargeTime = botWorld->gameLocalInfo.time + IGNORE_PLANTED_CHARGE_TIME;
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( distSqr > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( charge.origin ) : RUN );
		return true;
	}

	if ( botInfo->classType == ENGINEER && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY && ltgUseMine && botInfo->abilities.selfArmingMines ) {
		if ( !TeamMineInArea( charge.origin, 300.0f ) ) {
			if ( ltgReached == false ) { //mal: store off our camp location for later.
				ltgOrigin = botInfo->origin;
				ltgReached = true;
			}

			if ( distSqr > Square( 100.0f ) || botInfo->onLadder ) {

				Bot_SetupMove( charge.origin, -1, ACTION_NULL, charge.areaNum );

				if ( MoveIsInvalid() ) {
					ltgUseMine = false;
					return true;
				}

				Bot_MoveAlongPath( RUN );
				return true;
			}

			Bot_LookAtLocation( charge.origin, INSTANT_TURN );
			ignoreWeapChange = true;
			botIdealWeapSlot = NO_WEAPON;
			botIdealWeapNum = LANDMINE;

			if ( botInfo->weapInfo.weapon == LANDMINE ) {
				if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
					botUcmd->botCmds.attack = true;
				}
			}
		} else {
			vec = ltgOrigin - botInfo->origin;
			if ( vec.LengthSqr() > Square( 75.0f ) || botInfo->onLadder ) {

				Bot_SetupMove( ltgOrigin, -1, ACTION_NULL );

				if ( MoveIsInvalid() ) {
					ignorePlantedChargeTime = botWorld->gameLocalInfo.time + IGNORE_PLANTED_CHARGE_TIME;
					Bot_ExitAINode();
					return false;
				}

				Bot_MoveAlongPath( RUN );
				return true;
			}
		}
	} else if ( botInfo->classType == FIELDOPS && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY && ltgChat && ClassWeaponCharged( AIRCAN ) ) {
		
		int enemiesInArea = ClientsInArea( botNum, charge.origin, 1200.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, false, false );
		
		if ( enemiesInArea > 0 ) {
			if ( LocationVis2Sky( botInfo->origin ) ) {
				ignoreWeapChange = true;
				Bot_UseCannister( AIRCAN, charge.origin );
				return true;
			}
		}
	}

	if ( botThreadData.random.RandomInt( 100 ) > 93 ) {
		idVec3 vec;
		
		if ( Bot_RandomLook( vec ) )  {
			Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
		}
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, ltgPosture, NULLMOVETYPE );
	return true;
}

/*
================
idBotAI_LTG::Enter_LTG_PatrolDeliverGoal

The bot wants to roam to the deliver action, just to make sure its clear and hold it for the carrier.
================
*/
bool idBotAI::Enter_LTG_PatrolDeliverGoal() {

	LTG_AI_SUB_NODE = &idBotAI::LTG_PatrolDeliverGoal;

	ltgType = PATROL_DELIVER_GOAL;
	
	stayInPosition = false;

//mal: higher skill bots are more likely to respond to heard sounds, and go after the person making them, then lower skilled bots.
	if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) {
		stayInPosition = true;
	} else if ( botThreadData.GetBotSkill() == BOT_SKILL_NORMAL ) {
		if ( botThreadData.random.RandomInt( 100 ) > 30 ) {
			stayInPosition = true;
		}
	} else {
		if ( botThreadData.random.RandomInt( 100 ) > 70 ) {
			stayInPosition = true;
		}
	}

	ltgReached = false;

	ltgMoveTime = 0;

	ltgTryMoveCounter = 0;

	lastAINode = "Patrol Deliver Goal";

	return true;
}

/*
================
idBotAI_LTG::LTG_PatrolDeliverGoal
================
*/
bool idBotAI::LTG_PatrolDeliverGoal() {
	if ( ltgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) { //mal: action got turned off
		Bot_ExitAINode();
		return false;
	}

	int matesInArea = ClientsInArea( botNum, botThreadData.botActions[ actionNum ]->GetActionOrigin(), PATROL_DELIVER_TEST_DIST, botInfo->team, NOCLASS, false, false, false, true, true );

	if ( matesInArea > MAX_PATROL_DELIVER_CLIENTS ) {
		Bot_ExitAINode();
		return false;
	} //mal: enough clients there camping

	if ( ltgMoveTime < botWorld->gameLocalInfo.time ) {
		ltgTryMoveCounter = 0;
	}

	idVec3 vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	float distSqr = vec.LengthSqr();

	if ( distSqr > Square( botThreadData.botActions[ actionNum ]->radius + 75.0f ) || botInfo->onLadder ) {

		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME ); //mal: no valid path to this action for some reason - ignore it for a while
			Bot_ExitAINode();
			return false;
		}

		ltgReached = false;
		Bot_MoveAlongPath( ( distSqr > Square( 100.0f ) && enemy == -1 ) ? Bot_ShouldStrafeJump( botThreadData.botActions[ actionNum ]->origin ) : RUN );
		return true;
	}

	reachedPatrolPointTime = botWorld->gameLocalInfo.time;

	int blockedClient = Bot_CheckBlockingOtherClients( -1, 75.0f );
	botMoveFlags_t defaultMove = ( botInfo->posture == IS_CROUCHED ) ? CROUCH : WALK;

	if ( blockedClient != -1 && ltgTryMoveCounter < MAX_MOVE_ATTEMPTS ) {
		ltgTryMoveCounter++;
		if ( Bot_CanMove( BACK, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, defaultMove, NULLMOVETYPE );
		} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, defaultMove, NULLMOVETYPE );
		} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, defaultMove, NULLMOVETYPE );
		}

		Bot_LookAtEntity( blockedClient, SMOOTH_TURN );
		ltgReached = false;
		ltgMoveTime = botWorld->gameLocalInfo.time + 1000;
		return true;
	}

	if ( !ClientIsValid( heardClient, -1 ) ) {
		if ( botThreadData.random.RandomInt( 100 ) > 90 || ltgReached == false ) {
			idVec3 vec;
			if ( Bot_RandomLook( vec ) )  {
				Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
				ltgReached = true; //mal: we reached our goal, and am going to camp now. ONLY after we've looked around a bit.
			}
		}
	} else {
		Bot_MoveToGoal( vec3_zero, vec3_zero, CROUCH, NULLMOVETYPE ); //mal: crouch down, waiting for our enemy....
		Bot_LookAtLocation( botWorld->clientInfo[ heardClient ].origin, SMOOTH_TURN ); //we hear someone - look at where they're at.
		ltgReached = false; //mal: as long as we hear someone, keep this updated.
	}

	return true;
}




