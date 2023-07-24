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
idBotAI::Enter_NBG_SupplyTeammate
================
*/
bool idBotAI::Enter_NBG_SupplyTeammate() {
    
	nbgType = SUPPLY_TEAMMATE;

	NBG_AI_SUB_NODE = &idBotAI::NBG_SupplyTeammate;
	
	nbgTime = botWorld->gameLocalInfo.time + 15000; //mal: 15 seconds to heal this guy!

	nbgReached = false;

	nbgExit = false;

	bool chatIsGood = false;

	if ( ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
		const clientInfo_t& playerInfo = botWorld->clientInfo[ nbgTarget ];
		if ( !playerInfo.isBot ) {
			chatIsGood = true;
		}

		idVec3 vec = playerInfo.origin - botInfo->origin;
		float distSqrToHuman = vec.LengthSqr();

		if ( distSqrToHuman < Square( MEDIC_ACK_MIN_DIST ) ) {
			chatIsGood = false;
		}

		if ( distSqrToHuman > Square( 2000.0f ) ) {
			nbgTime = botWorld->gameLocalInfo.time + 30000; //mal: 30 seconds to revive this human if hes far away!
		}
	}

	if ( botInfo->tkReviveTime < botWorld->gameLocalInfo.time && chatIsGood ) {
		if ( nbgTargetType == HEAL_REQUESTED || nbgTargetType == HEAL ) {
			Bot_AddDelayedChat( botNum, MEDIC_ACK, 1 );
		} else if ( nbgTargetType == REARM_REQUESTED ) {
			if ( botInfo->team == GDF ) {
				if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
					Bot_AddDelayedChat( botNum, ACKNOWLEDGE_YES, 1 );
				} else {
					Bot_AddDelayedChat( botNum, MY_CLASS, 1 );
				}
			} else {
				if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			        Bot_AddDelayedChat( botNum, ACKNOWLEDGE_YES, 1 );
				} 
			}
		}
	}

	lastAINode = "Supplying Mate";

	return true;
}

/*
================
idBotAI::NBG_SupplyTeammate
================
*/
bool idBotAI::NBG_SupplyTeammate() {
	if ( !ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
        Bot_ExitAINode();
		return false;
	}

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		if ( !nbgExit ) {
			Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME ); //mal: this client isn't easy to heal, so just ignore him for a while
		}
		Bot_ExitAINode();
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ nbgTarget ];

	if ( playerInfo.inWater || botInfo->inWater ) {
		Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME ); //mal: this client isn't easy to heal, so just ignore him for a while
		Bot_ExitAINode();
		return false;
	}

	if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: client jumped into a vehicle/deployable - forget them
		Bot_ExitAINode();
		return false;
	}

	if ( ClientIsDead( nbgTarget ) || playerInfo.inLimbo ) { //mal: hes gone jim! Do something else!
		Bot_ExitAINode();
		return false;
	}

	if ( playerInfo.team != botInfo->team && !playerInfo.isDisguised ) { //mal: we were tricked, but now see his true colors!
		Bot_ExitAINode();
		return false;
	}

	if ( nbgTargetType == HEAL || nbgTargetType == HEAL_REQUESTED ) {
        if ( playerInfo.health == playerInfo.maxHealth ) { //mal: mission accomplished - leave!
			Bot_ExitAINode();
			return false;
		}
	} //mal: for ammo, always just pop them 2 packs. They may want ammo for nades or other weap types, so we dont worry about it.

//mal: dont bother doing this if bot ran out of charge
	if ( !ClassWeaponCharged( HEALTH ) && nbgExit == false ) {
		nbgTime = botWorld->gameLocalInfo.time + 1000;
		nbgExit = true;
	}

	idVec3 vec = playerInfo.origin - botInfo->origin;
	float distSqr = vec.LengthSqr();

	botUcmd->actionEntityNum = nbgTarget; //mal: let the game and obstacle avoidance know we want to interact with this entity.
	botUcmd->actionEntitySpawnID = playerInfo.spawnID;

	bool isVisible = true;

	if ( distSqr < Square( 200.0f ) ) {
		trace_t	tr;
		botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, playerInfo.viewOrigin, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ) );

		if ( tr.fraction < 1.0f && tr.c.entityNum != nbgTarget ) {
			isVisible = false;
		}
	}

	if ( distSqr > Square( 150.0f ) || botInfo->onLadder || !isVisible ) {
		Bot_SetupMove( vec3_zero, nbgTarget, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ExitAINode();
			return false;
		}
		
		Bot_MoveAlongPath( ( distSqr > Square( 200.0f ) ) ? SPRINT : RUN );
		return true;
	}

	if ( distSqr < Square( 75.0f ) ) { // too close, back up some!
		if ( Bot_CanMove( BACK, 50.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		}
	}

	if ( !nbgReached ) {
		nbgReached = true;
		nbgTime = botWorld->gameLocalInfo.time + ( ( nbgTargetType == HEAL || nbgTargetType == HEAL_REQUESTED ) ? 9000 : 2500 );
		nbgTimer = botWorld->gameLocalInfo.time + 500; //mal: wait just a sec to get target in our sights

		if ( nbgTargetType == REARM_REVIVE ) {
			nbgTime = botWorld->gameLocalInfo.time + 3000;
		}
	}

	Bot_ClearAngleMods();
	Bot_LookAtEntity( nbgTarget, SMOOTH_TURN );

	botIdealWeapSlot = NO_WEAPON;
	
	if ( botInfo->team == GDF ) {
        if ( nbgTargetType == HEAL || nbgTargetType == HEAL_REQUESTED ) {
			botIdealWeapNum = HEALTH;
		} else {
			botIdealWeapNum = AMMO_PACK; 
		}
	} else {
		botIdealWeapNum = HEALTH;
	}

	if ( botInfo->weapInfo.isReady && ( botInfo->weapInfo.weapon == HEALTH || botInfo->weapInfo.weapon == AMMO_PACK ) && nbgTimer < botWorld->gameLocalInfo.time ) {
		botUcmd->botCmds.launchPacks = true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_ReviveTeammate
================
*/
bool idBotAI::Enter_NBG_ReviveTeammate() {
    
	nbgType = REVIVE_TEAMMATE;

	NBG_AI_SUB_NODE = &idBotAI::NBG_ReviveTeammate;
	
	nbgTime = botWorld->gameLocalInfo.time + 15000; //mal: 15 seconds to revive this guy!

	nbgReached = false;

	lefty = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? true : false;

	if ( botInfo->tkReviveTime < botWorld->gameLocalInfo.time ) {
		if ( ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
			const clientInfo_t& playerInfo = botWorld->clientInfo[ nbgTarget ];

			if ( !playerInfo.isBot ) {
				idVec3 vec = playerInfo.origin - botInfo->origin;

				float distSqrToHuman = vec.LengthSqr();
					
				if ( distSqrToHuman > Square( MEDIC_ACK_MIN_DIST ) ) {
					Bot_AddDelayedChat( botNum, MEDIC_ACK, 1 );
				}

				if ( distSqrToHuman > Square( 2000.0f ) ) {
					nbgTime = botWorld->gameLocalInfo.time + 30000; //mal: 30 seconds to revive this human if hes far away!
				}
			}
		}
	}

	lastAINode = "Reviving Mate";

	return true;	
}

/*
================
idBotAI::NBG_ReviveTeammate
================
*/
bool idBotAI::NBG_ReviveTeammate() {
 	float dist;
	idVec3 vec;

	if ( !ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
		Bot_ExitAINode();
		return false;
	}

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ nbgTarget ];

	if ( playerInfo.inLimbo ) { //mal: hes gone jim! Do something else
		Bot_ExitAINode();
		return false;
	}

	vec = playerInfo.origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( playerInfo.health > 0 ) { //mal: mission accomplished - now decide to resupply him, or just leave
		if ( dist > Square( 75.0f ) || playerInfo.health == playerInfo.maxHealth || enemy != -1 || playerInfo.classType == MEDIC || botInfo->isActor ) { //mal: someone else revived him, hes at full health, hes a medic (who can heal himself) or we got an enemy - so just leave!
			Bot_ExitAINode();
		} else { //mal: he was our revive, its safe, and hes in need - so lets give him some health now
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_SupplyTeammate;
			nbgTargetType = HEAL;
		}
		return false;
	}

	botUcmd->actionEntityNum = nbgTarget; ///mal: dont avoid this client.
	botUcmd->actionEntitySpawnID = nbgTargetSpawnID;

	if ( dist < Square( 700.0f ) && enemy == -1 ) {
		botIdealWeapNum = NEEDLE;
	}

	Bot_CheckAdrenaline( playerInfo.origin );	

	if ( dist > Square( 65.0f ) || botInfo->onLadder ) {
		Bot_SetupMove( vec3_zero, nbgTarget, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ExitAINode();
			return false;
		}
		
		Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? SPRINT : RUN );

		if ( dist < Square( LOOK_AT_GOAL_DIST ) ) {
			Bot_LookAtEntity( nbgTarget, SMOOTH_TURN );
		}

		return true;
	}

	nbgPosture = Bot_FindStanceForLocation( playerInfo.origin, false );

	if ( dist < Square( 10.0f ) ) {
        if ( Bot_CanMove( BACK, 100.0f, true )) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
			nbgReached = false; //mal: if have to move to reaquire target, give ourselves some time
			return true;
		}
	}

	if ( !nbgReached ) { 
		nbgReached = true;
		nbgTime = botWorld->gameLocalInfo.time + 9000; //mal: give ourselves 9 seconds to complete this task once reach target
	}

	if ( BodyIsObstructed( nbgTarget, false ) ) {
        if ( lefty == true ) {
			if ( Bot_CanMove( LEFT, 50.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
			} else {
				lefty = false;
			}
		} else {
			if ( Bot_CanMove( RIGHT, 50.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
			} else {
				lefty = true;
			}
		}
	} else {
		Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE );
	}

	Bot_ClearAngleMods();
	Bot_LookAtLocation( playerInfo.origin, SMOOTH_TURN );

	weaponLocked = true;
	
	botUcmd->botCmds.activate = true;

	if ( botInfo->weapInfo.weapon == NEEDLE && botInfo->team == STROGG ) {
		botUcmd->botCmds.activateHeld = true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_CreateSpawnHost
================
*/
bool idBotAI::Enter_NBG_CreateSpawnHost() {
    
	nbgType = CREATE_SPAWNHOST;

	NBG_AI_SUB_NODE = &idBotAI::NBG_CreateSpawnHost;
	
	nbgTime = botWorld->gameLocalInfo.time + 9000; //mal: 9 seconds to reach this guy!

	nbgReached = false;

	lefty = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? true : false;

	nbgTimer = 0;

	if ( nbgTargetType == CLIENT ) {
		nbgClientNum = nbgTarget;
		nbgOrigin = botWorld->clientInfo[ nbgTarget ].origin;
		nbgTargetSpawnID = botWorld->clientInfo[ nbgTarget ].spawnID;
	} else {
		nbgClientNum = botWorld->playerBodies[ nbgTarget ].bodyOwnerClientNum;
		nbgOrigin = botWorld->playerBodies[ nbgTarget ].bodyOrigin;
		nbgTargetSpawnID = -1;
	}

	lastAINode = "Creating SpawnHost";

	return true;	
}

/*
================
idBotAI::NBG_CreateSpawnHost
================
*/
bool idBotAI::NBG_CreateSpawnHost() {
	bool isCorpse = ( nbgTargetType == CLIENT ) ? false : true;
	int areaNum;
	int mates;
	float dist;
	idVec3 vec;
	idVec3 origin;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		if ( nbgTargetType == CLIENT ) {
			Bot_IgnoreClient( nbgTarget, CLIENT_IGNORE_TIME * 2 );
		} else {
			Bot_IgnoreBody( nbgTarget, BODY_IGNORE_TIME * 2 );
		}

		Bot_ExitAINode();
		return false;
	}

	if ( nbgTargetType == CLIENT ) {

        if ( !ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
			Bot_ExitAINode();
			return false;
		}

		if ( botWorld->clientInfo[ nbgTarget ].health > 0 && botWorld->clientInfo[ nbgTarget ].revived ) {
            Bot_ExitAINode(); //mal: he got revived, leave here now incase his buddies are around
			return false;
		}

		if ( botWorld->clientInfo[ nbgTarget ].inLimbo || botWorld->clientInfo[ nbgTarget ].health > 0 ) {

			int newBody = -1;
	
			nbgTimer++; //mal: it may be a few frames before the body shows up, since there are delays coming from running in a seperate thread, and from the script system itself.

			if ( nbgTimer > 10 ) { //mal: this is taking too long!
                Bot_ExitAINode();
				return false;
			}

			newBody = FindBodyOfClient( nbgTarget, false, nbgOrigin ); //mal: find the body of the client we were originally gunning for.

			if ( newBody != -1 ) {
				nbgTarget = newBody;
				nbgTargetType = BODY;
				nbgClientNum = botWorld->playerBodies[ nbgTarget ].bodyOwnerClientNum;
				nbgOrigin = botWorld->playerBodies[ nbgTarget ].bodyOrigin;
				return true;
			}

			if ( nbgReached == true ) { //mal: stay in the posture we were in when we asked for the new body
				Bot_MoveToGoal( vec3_zero, vec3_zero, Bot_FindStanceForLocation( nbgOrigin, false ), NULLMOVETYPE );
			}

			return true;
		}

		mates = ClientsInArea( botNum, botWorld->clientInfo[ nbgTarget ].origin, 150.0f, botInfo->team, MEDIC, false, false, false, false, true );

		if ( mates >= 1 ) {    //mal: if we have a couple teammates in the area, they may be trying to do the same thing as us, so leave.
            Bot_ExitAINode();
			return false;
		}

		origin = botWorld->clientInfo[ nbgTarget ].origin;
		areaNum = botWorld->clientInfo[ nbgTarget ].areaNum;

	} else { //mal: type must be a body

		if ( botWorld->playerBodies[ nbgTarget ].bodyOwnerClientNum != nbgClientNum || botWorld->playerBodies[ nbgTarget ].isValid == false ) { //mal: list could have been resorted, especially if a lot of action is going on, so check to make sure we're still up to date.

			int newBody = -1;
	
			newBody = FindBodyOfClient( nbgClientNum, false, nbgOrigin ); //mal: find the body of the client we were originally gunning for.

			if ( newBody == -1 ) {
				Bot_ExitAINode();
				return false;
			}
		
			nbgTarget = newBody;
			nbgClientNum = botWorld->playerBodies[ nbgTarget ].bodyOwnerClientNum;
			nbgOrigin = botWorld->playerBodies[ nbgTarget ].bodyOrigin;
		}

		if ( botWorld->playerBodies[ nbgTarget ].isValid == false ) {
            Bot_ExitAINode(); //mal: got gibbed, expired, etc.
			return false;
		}

		if ( !botWorld->playerBodies[ nbgTarget ].isSpawnHostAble ) { //mal: already spawnhosted, gibbed, uniform stolen, etc.
            Bot_ExitAINode(); 
			return false;
		}

		mates = ClientsInArea( botNum, botWorld->playerBodies[ nbgTarget ].bodyOrigin, 150.0f, botInfo->team , MEDIC, false, false, false, false, true );

		if ( mates >= 1 ) { // whether they're spawnhosting or not, this goal isn't critical enough to have 2+ medics in the area!
            Bot_ExitAINode();
			return false;
		}

		origin = botWorld->playerBodies[ nbgTarget ].bodyOrigin;
		areaNum = botWorld->playerBodies[ nbgTarget ].areaNum;
	}

    vec = origin - botInfo->origin;
	dist = vec.LengthSqr();

	botUcmd->actionEntityNum = ( nbgTargetType == CLIENT ) ? nbgTarget :  botWorld->playerBodies[ nbgTarget ].bodyNum; ///mal: dont avoid this client.
	botUcmd->actionEntitySpawnID = ( nbgTargetType == CLIENT ) ? nbgTargetSpawnID :  botWorld->playerBodies[ nbgTarget ].spawnID; ///mal: dont avoid this client.

	if ( dist > Square( 65.0f ) || botInfo->onLadder ) {
		Bot_SetupMove( origin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			if ( nbgTargetType == CLIENT ) {
				Bot_IgnoreClient( nbgTarget, CLIENT_IGNORE_TIME * 2 );
			} else {
				Bot_IgnoreBody( nbgTarget, BODY_IGNORE_TIME * 2 );
			}
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? SPRINT : RUN );
		nbgReached = false;
		return true;
	}

	nbgPosture = Bot_FindStanceForLocation( nbgOrigin, false );

	if ( dist < Square( 10.0f ) ) {
        if ( Bot_CanMove( BACK, 100.0f, true )) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
			nbgReached = false;
			return true;
		}
	}

	if ( !nbgReached ) {
		nbgReached = true;
		nbgTime = botWorld->gameLocalInfo.time + 7000; //mal: give ourselves 7 seconds to complete this task once reach target
	}

	Bot_ClearAngleMods();
	Bot_LookAtLocation( origin, SMOOTH_TURN );

	if ( BodyIsObstructed( nbgTarget, isCorpse ) ) {
        if ( botInfo->hasCrosshairHint == false && botInfo->weapInfo.weapon != NEEDLE ) { //mal: body may be sitting on another body, so shift around to get a better view, if possible.
			if ( lefty == true ) {
				if ( Bot_CanMove( LEFT, 50.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
				} else {
					lefty = false;
				}
			} else {
				if ( Bot_CanMove( RIGHT, 50.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
				} else {
					lefty = true;
				}
			}
		} else {
			Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE );
		}
	} else {
		Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE );
	}

	weaponLocked = true;

	classAbilityDelay = nbgTime;

	botUcmd->botCmds.activate = true;

	if ( botInfo->weapInfo.weapon == NEEDLE ) {
		botUcmd->botCmds.activateHeld = true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_HumiliateEnemy
================
*/
bool idBotAI::Enter_NBG_HumiliateEnemy() {
    
	nbgType = HAZE_ENEMY;

	NBG_AI_SUB_NODE = &idBotAI::NBG_HumiliateEnemy;
	
	nbgTime = botWorld->gameLocalInfo.time + 10000; //mal: 10 seconds to reach this guy!

	nbgReached = false;

	nbgChat = true;

	lastAINode = "Humiliate Enemy";

	return true;	
}

/*
================
idBotAI::NBG_HumiliateEnemy
================
*/
bool idBotAI::NBG_HumiliateEnemy() {
	float dist;
	idVec3 vec;

	if ( !ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
		Bot_ExitAINode();
		return false;
	}

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ nbgTarget ];

	if ( playerInfo.inLimbo ) { //mal: no point in hazing this guy if hes tapped out.
		Bot_ExitAINode();
		return false;
	}
	
	if ( playerInfo.health > 0 ) {
        Bot_ExitAINode(); //mal: he got revived, leave here now incase his buddies are around
		return false;
	}

	vec = playerInfo.origin - botInfo->origin;
	dist = vec.LengthSqr();

	botUcmd->actionEntityNum = nbgTarget;
	botUcmd->actionEntitySpawnID = nbgTargetSpawnID;

	if ( dist > Square( 35.0f ) || botInfo->onLadder ) {
		Bot_SetupMove( vec3_zero, nbgTarget, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}
		
		Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? SPRINT : RUN );
		return true;
	}

	nbgPosture = Bot_FindStanceForLocation( playerInfo.origin, false );

	if ( dist < Square( 10.0f ) ) {
        if ( Bot_CanMove( BACK, 100.0f, true )) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
			nbgReached = false;
			return true;
		}
	}

	if ( !nbgReached ) {
		nbgReached = true;
		nbgTime = botWorld->gameLocalInfo.time + 7000; //mal: give ourselves 7 seconds to complete this task once reach target

		if ( nbgChat ) {
			if ( !playerInfo.isBot ) {
				Bot_AddDelayedChat( botNum, GENERAL_TAUNT, 1, true ); //mal: rub it in the guys face!
			}
			nbgChat = false;
		}
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE );
	Bot_LookAtLocation( playerInfo.origin, SMOOTH_TURN );

	botIdealWeapSlot = MELEE;
	botIdealWeapNum = NULL_WEAP;

	if ( botInfo->weapInfo.isReady && botInfo->weapInfo.weapon == KNIFE ) { //eat this biatch! >:-)
		botUcmd->botCmds.attack = true;
		botUcmd->botCmds.constantFire = true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_Camp
================
*/
bool idBotAI::Enter_NBG_Camp() {
    
	if ( ltgType == DEFENSE_CAMP_GOAL ) {
		nbgType = DEFENSE_CAMP;
	} else if ( ltgType == INVESTIGATE_ACTION ) {
		nbgType = INVESTIGATE_CAMP;
	} else {
		nbgType = CAMP;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	bool isCamping = ( botThreadData.botActions[ actionNum ]->GetObjForTeam( botInfo->team ) == ACTION_CAMP || botThreadData.botActions[ actionNum ]->GetObjForTeam( botInfo->team ) == ACTION_DEFENSE_CAMP ) ? true : false;

	NBG_AI_SUB_NODE = &idBotAI::NBG_Camp;

	nbgReached = false;

	ResetRandomLook(); //mal: start out looking this far at random targets

	stayInPosition = false;

	botIdealWeapSlot = GUN;

	if ( isCamping ) {
        nbgPosture = botThreadData.botActions[ actionNum ]->posture;
	} else {
		nbgPosture = RANDOM_STANCE;
	}

	if ( nbgPosture == RANDOM_STANCE ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			nbgPosture = CROUCH;
		} else {
			nbgPosture = WALK;
		}
	}

	nbgExit = true;

	nbgTimer = 0;

	nbgTryMoveCounter = 0;
	
	nbgMoveTime = 0;

	nbgMoveType = NULLMOVETYPE;

#ifndef _XENON
	leanTypes_t lean = botThreadData.botActions[ actionNum ]->leanDir;

	if ( lean != NULL_LEAN ) {
		if ( lean == LEAN_BODY_RIGHT ) {
			nbgMoveType = LEAN_RIGHT;		
		} else {
			nbgMoveType = LEAN_LEFT;
		}
	}
#endif

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
		lastAINode = "Defense Camping";
		stayInPosition = false;
	} else {
		lastAINode = "Camping";
	}

	return true;	
}

/*
================
idBotAI::NBG_Camp
================
*/
bool idBotAI::NBG_Camp() {

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botInfo->weapInfo.primaryWeapHasAmmo ) { //mal: no ammo - dont camp.
		Bot_ExitAINode();
		return false;
	}

	bool isCamping = ( botThreadData.botActions[ actionNum ]->GetObjForTeam( botInfo->team ) == ACTION_CAMP ) ? true : false;
	int i;  
	int k = 0;
	int blockedClient;
	int linkedTargets[ MAX_LINKACTIONS ];
	float dist;
	idVec3 vec;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) {
        Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	int numEnemiesInArea = ClientsInArea( botNum, botInfo->origin, 512.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, false, false );

	if ( botInfo->isDisguised && numEnemiesInArea > 0 ) {
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( nbgMoveTime < botWorld->gameLocalInfo.time ) {
		nbgTryMoveCounter = 0;
	}

//mal: this should never happen, but just in case bot somehow got bumped away from its camp spot......
	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( nbgReached == true ) {
        blockedClient = Bot_CheckBlockingOtherClients( -1 );
	} else {
		blockedClient = -1;
	}

	if ( dist > Square( botThreadData.botActions[ actionNum ]->radius ) && blockedClient == -1 || botInfo->onLadder ) {
		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			Bot_ClearAIStack();
			return false;
		}

		nbgReached = false; //mal: we're on the move		
		Bot_MoveAlongPath( ( dist > Square( 500.0f ) ) ? SPRINT : RUN );
		ResetRandomLook();
		return true;
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, ( nbgExit == false ) ? nbgMoveType : NULLMOVETYPE ); //mal: assume the posture defined by the action...

	if ( botInfo->classType == COVERTOPS && botInfo->team == STROGG && botWorld->gameLocalInfo.botSkill > BOT_SKILL_EASY ) {
		if ( botInfo->weapInfo.covertToolInfo.entNum == 0 ) {
			if ( botInfo->enemiesInArea == 0 && ClassWeaponCharged( FLYER_HIVE ) && nbgTimer < 30 ) {
				botIdealWeapNum = FLYER_HIVE;
				botIdealWeapSlot = NO_WEAPON;
				nbgTimer++;

				if ( botInfo->weapInfo.weapon == FLYER_HIVE && botThreadData.random.RandomInt( 100 ) > 50 ) {
					botUcmd->botCmds.attack = true;
				}
			}
		} else { //mal: we have a flyer hive out there in the world somewhere.
			if ( botInfo->lastAttackerTime + 1000 > botWorld->gameLocalInfo.time ) {
				botUcmd->botCmds.attack = true;
				nbgTimer = 0;
				botIdealWeapSlot = GUN;
				return true;
			} //mal: we're under attack! Lose the hive and fight!

			int enemyNum = Flyer_FindEnemy( MAX_HIVE_RANGE );

			nbgTimer = 0;

			if ( enemyNum != -1 ) {
				clientInfo_t player = botWorld->clientInfo[ enemyNum ];

				int goalAreaNum = player.areaNum;
				idVec3 goalOrigin = player.origin;

				idVec3 vec = goalOrigin - botInfo->weapInfo.covertToolInfo.origin;
				nbgTime += 100;
				nbgTimer = 0;

				if ( vec.LengthSqr() > Square( FLYER_HIVE_ATTACK_DIST ) ) {
					Bot_SetupFlyerMove( goalOrigin, goalAreaNum );
					idVec3 moveGoal = botAAS.path.moveGoal;
					moveGoal.z += FLYER_HIVE_HEIGHT_OFFSET;
					Bot_MoveToGoal( moveGoal, vec3_zero, RUN, NULLMOVETYPE );
					Flyer_LookAtLocation( moveGoal );
					return true;
				} else {
					botUcmd->botCmds.attack = true;
					tacticalActionTime = botWorld->gameLocalInfo.time + 1000;
					return true;
				}
			}
		}
	}

	if ( Bot_IsUnderAttackFromUnknownEnemy() ) { //mal: dont sit there! Someones shooting you!
		Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME );
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( botThreadData.random.RandomInt( 100 ) > 98 || nbgReached == false ) {

		nbgExit = !nbgExit;

		if ( botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].inuse != false && heardClient == -1 || botInfo->isDisguised && isCamping != false ) { //mal: if the first target action == false, they all must, so skip
			for( i = 0; i < MAX_LINKACTIONS; i++ ) {
				if ( botThreadData.botActions[ actionNum ]->actionTargets[ i ].inuse != false ) {
					linkedTargets[ k++ ] = i;
				}
			}

			i = botThreadData.random.RandomInt( k );

			nbgReached = true;

			Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->actionTargets[ i ].origin, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
		} else {
			if ( heardClient == -1 || botInfo->isDisguised ) {
                if ( botThreadData.random.RandomInt( 100 ) > 98 || nbgReached == false ) {
                    if ( Bot_RandomLook( vec ) )  {
                        Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
                        nbgReached = true; //mal: we reached our goal, and am going to camp now. ONLY after we've looked around a bit.
					}
				}
			} else {
                if ( nbgPosture != PRONE ) { //mal: unless we're prone, be sure to crouch if we hear someone. Try to suprise our incoming "guest".
					Bot_MoveToGoal( vec3_zero, vec3_zero, CROUCH, NULLMOVETYPE );
				}

                Bot_LookAtLocation( botWorld->clientInfo[ heardClient ].origin, SMOOTH_TURN ); //we hear someone - look at where they're at.
				nbgReached = false; //mal: as long as we hear someone, keep this updated.
			}
		}
	}

	if ( blockedClient != -1 && nbgTryMoveCounter < MAX_MOVE_ATTEMPTS ) {
		nbgTryMoveCounter++;

		botMoveFlags_t botMoveFlag = ( botInfo->posture == IS_CROUCHED ) ? CROUCH : WALK;

		if ( Bot_CanMove( BACK, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
		} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
		} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
		}

		Bot_LookAtEntity( blockedClient, SMOOTH_TURN );
		nbgMoveTime = botWorld->gameLocalInfo.time + 1000;
		return true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_Build
================
*/
bool idBotAI::Enter_NBG_Build() {
    
	nbgType = BUILD;

	NBG_AI_SUB_NODE = &idBotAI::NBG_Build;

	if ( ltgType == BUILD_GOAL ) {
		nbgTime = botWorld->gameLocalInfo.time + BOT_INFINITY;
	} else {
		nbgTime = botWorld->gameLocalInfo.time + 15000;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	botThreadData.botActions[ actionNum ]->FindRandomPointInBBox( nbgOrigin, botNum, botInfo->team ); //mal: find a random point within this action's BBox to move towards.

	nbgPosture = botThreadData.botActions[ actionNum ]->posture;

	if ( nbgPosture == RANDOM_STANCE ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			nbgPosture = CROUCH;
		} else {
			nbgPosture = WALK;
		}
	}

	nbgReached = false;

	nbgTimer = 0;

	nbgMoveTimer = botWorld->gameLocalInfo.time + MAX_MOVE_FAILED_TIME;

	lastAINode = "Building";

	return true;	
}

/*
================
idBotAI::NBG_Build
================
*/
bool idBotAI::NBG_Build() {

	idVec3 vec;
	idBox playerBox;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( ltgType == BUILD_GOAL ) {
		if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: player decided to be a hero, so let them.
			Bot_ExitAINode();
			Bot_ClearAIStack();
			return false;
		}
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) {
        Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( nbgReached == false ) {
		if ( !PointIsClearOfTeammates( nbgOrigin ) ) {
			botThreadData.botActions[ actionNum ]->FindRandomPointInBBox( nbgOrigin, botNum, botInfo->team ); //mal: find a random point within this action's BBox to move towards.
			nbgTimer++;
		}
	} else {
		nbgMoveTimer = botWorld->gameLocalInfo.time;
		nbgTimer = 0;
	}

	if ( nbgTimer > 10 ) { //mal: if can't reach our goal after so many tries, just leave.
		int tempActionNum = actionNum;
		Bot_ExitAINode();
		Bot_ClearAIStack();
		Bot_IgnoreAction( tempActionNum, 5000 );
		return false;
	}

	if ( nbgMoveTimer < botWorld->gameLocalInfo.time ) {
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	vec = nbgOrigin - botInfo->origin;

	playerBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );

//mal: move into the bbox for this action, so we know we're close enough to start the build process
	if ( !botThreadData.botActions[ actionNum ]->actionBBox.IntersectsBox( playerBox ) ) {
		Bot_SetupQuickMove( nbgOrigin, false );
//		Bot_SetupMove( nbgOrigin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			Bot_ClearAIStack();
			return false;
		}

		if ( nbgReached == true ) {
			Bot_ExitAINode();
			Bot_ClearAIStack();
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	Bot_LookAtLocation( nbgOrigin, SMOOTH_TURN );

	nbgReached = true;

	if ( botThreadData.AllowDebugData() ) {
		idVec3 temp = nbgOrigin;
		temp[2] += 64.0f;
		gameRenderWorld->DebugLine( colorRed, nbgOrigin, temp, 16 );
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE ); //mal: assume the posture defined by the action...

	weaponLocked = true;

	botUcmd->botCmds.activate = true;

	if ( botInfo->weapInfo.weapon == PLIERS ) {
		botUcmd->botCmds.activateHeld = true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_Hack
================
*/
bool idBotAI::Enter_NBG_Hack() {

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	int enemiesInArea = ClientsInArea( botNum, botThreadData.botActions[ actionNum ]->GetActionOrigin(), 900.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, true, false );
    
	nbgType = HACK;

 	NBG_AI_SUB_NODE = &idBotAI::NBG_Hack;
	 
	nbgTime = botWorld->gameLocalInfo.time + BOT_INFINITY; //mal: do this task until its done!

	nbgMoveDir = NULL_DIR;

	if ( botInfo->team == GDF ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) { //mal: randomly decide to use smoke at obj or not.
			nbgExit = true;

			if ( botThreadData.random.RandomInt( 100 ) > 50 ) { //mal: sometimes we'll throw smoke, and sometimes we'll throw our 3rd eye camera.
				nbgMoveDir = BACK;
			}
		} else {
			nbgExit = false;
		}
	} else {
		nbgExit = false;
	}

	botUcmd->botCmds.dropDisguise = true; //mal: if we're disguised, drop it and start hacking!

	botThreadData.botActions[ actionNum ]->FindRandomPointInBBox( nbgOrigin, botNum, botInfo->team ); //mal: find a random point within this action's BBox to move towards.

	nbgPosture = botThreadData.botActions[ actionNum ]->posture;

	if ( nbgPosture == RANDOM_STANCE ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			nbgPosture = CROUCH;
		} else {
			nbgPosture = WALK;
		}
	}

	nbgTimer = 0;
	nbgReached = false;

	nbgMoveTimer = botWorld->gameLocalInfo.time + MAX_MOVE_FAILED_TIME;

	nbgTimer2 = 0;

	lastAINode = "Hacking";

	return true;	
}

/*
================
idBotAI::NBG_Hack
================
*/
bool idBotAI::NBG_Hack() {
	idVec3 vec;
	idBox playerBox;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: player decided to be a hero, so let them.
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) {
        Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( nbgReached == false ) {
		if ( !PointIsClearOfTeammates( nbgOrigin ) ) {
			botThreadData.botActions[ actionNum ]->FindRandomPointInBBox( nbgOrigin, botNum, botInfo->team ); //mal: find a random point within this action's BBox to move towards.
			nbgTimer++;
		}
	} else {
		nbgTimer = 0;
		nbgMoveTimer = botWorld->gameLocalInfo.time;
	}

	if ( nbgTimer > 10 ) { //mal: if can't reach our goal after so many tries, just leave.
		int tempActionNum = actionNum;
		Bot_ExitAINode();
		Bot_ClearAIStack();
		Bot_IgnoreAction( tempActionNum, 5000 );
		return false;
	}

	if ( nbgMoveTimer < botWorld->gameLocalInfo.time ) {
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	vec = nbgOrigin - botInfo->origin;

	playerBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );

//mal: move into the bbox for this action, so we know we're close enough to start the hack process
	if ( !botThreadData.botActions[ actionNum ]->actionBBox.IntersectsBox( playerBox ) || vec.LengthSqr() > Square( PLIERS_RANGE ) ) {
		Bot_SetupQuickMove( nbgOrigin, false );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			Bot_ClearAIStack();
			return false;
		}

		if ( nbgReached == true ) {
			Bot_ExitAINode();
			Bot_ClearAIStack();
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	nbgReached = true;

	Bot_LookAtLocation( nbgOrigin, SMOOTH_TURN );
	Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE ); //mal: assume the posture defined by the action...

	if ( nbgExit != false ) {
		if ( nbgMoveDir == BACK ) {
			if ( ClassWeaponCharged( THIRD_EYE ) ) {
				nbgTimer2++;
				Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->GetActionOrigin(), SMOOTH_TURN );
				botIdealWeapNum = THIRD_EYE;
				botIdealWeapSlot = NO_WEAPON;

				if ( nbgTimer2 > 30 ) {
					if ( botInfo->weapInfo.weapon == THIRD_EYE ) {
						botUcmd->botCmds.attack = true;
					}
				}
			} else {
				nbgExit = false;
			}
		} else {
			if ( ClassWeaponCharged( SMOKE_NADE ) ) {
				Bot_UseCannister( SMOKE_NADE, botThreadData.botActions[ actionNum ]->actionBBox.GetCenter() );
				botUcmd->botCmds.lookDown = true;
				return true;
			} else {
				nbgExit = false;
			}
		}
	} else {
		if ( !Bot_CheckCovertToolState() ) {
			weaponLocked = true;
			botUcmd->botCmds.activate = true;

			if ( botInfo->weapInfo.weapon == HACK_TOOL ) {
				botUcmd->botCmds.activateHeld = true;
			}
		}
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_PlantBomb
================
*/
bool idBotAI::Enter_NBG_PlantBomb() {
    
	nbgType = PLANT_BOMB;

	NBG_AI_SUB_NODE = &idBotAI::NBG_PlantBomb;
	
	nbgTime = botWorld->gameLocalInfo.time + BOT_INFINITY;

	nbgReached = false;

	nbgTarget = -1; //mal: we'll use this to camp the action, post planting.

	nbgTimer = 0; // track how long we've been trying to plant, if for too long, try something else.

	nbgMoveTimer = botWorld->gameLocalInfo.time + MAX_MOVE_FAILED_TIME;

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	nbgPosture = botThreadData.botActions[ actionNum ]->posture;

	if ( nbgPosture == RANDOM_STANCE ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			nbgPosture = CROUCH;
		} else {
			nbgPosture = WALK;
		}
	}

	botThreadData.botActions[ actionNum ]->FindRandomPointInBBox( nbgOrigin, botNum, botInfo->team ); //mal: find a random point within this action's BBox to move towards.

	ResetRandomLook();

	nbgTryMoveCounter = 0;
	
	nbgMoveTime = 0;

	Bot_AddDelayedChat( botNum, NEED_BACKUP, 3 );

	nbgTimer2 = 0;

	lastAINode = "Planting Bomb";

	return true;	
}

/*
================
idBotAI::NBG_PlantBomb
================
*/
bool idBotAI::NBG_PlantBomb() {
	int i;
	int k = 0;
	int linkedTargets[ MAX_LINKACTIONS ];
	float dist;
	plantedChargeInfo_t bombInfo;
	idVec3 vec;
	idBox playerBox;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: player decided to be a hero, so let them.
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) {
        Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( nbgReached == false ) {
		if ( !PointIsClearOfTeammates( nbgOrigin ) ) {
			botThreadData.botActions[ actionNum ]->FindRandomPointInBBox( nbgOrigin, botNum, botInfo->team ); //mal: find a random point within this action's BBox to move towards.
			nbgTimer2++;
		}
	} else {
		nbgMoveTimer = botWorld->gameLocalInfo.time;
		nbgTimer2 = 0;
	}

	if ( nbgTimer2 > 10 ) { //mal: if can't reach our goal after so many tries, just leave.
		int tempActionNum = actionNum;
		Bot_ExitAINode();
		Bot_ClearAIStack();
		Bot_IgnoreAction( tempActionNum, 5000 );
		return false;
	}

	if ( nbgMoveTimer < botWorld->gameLocalInfo.time ) {
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( nbgMoveTime < botWorld->gameLocalInfo.time ) {
		nbgTryMoveCounter = 0;
	}

	vec = nbgOrigin - botInfo->origin;

	playerBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );

//mal: move into the bbox for this action, so we know we're close enough to start the planting process if we just got here.
	if ( nbgReached == false ) {
		if ( !botThreadData.botActions[ actionNum ]->actionBBox.IntersectsBox( playerBox ) ) {
			Bot_SetupQuickMove( nbgOrigin, false );
//			Bot_SetupMove( nbgOrigin, -1, ACTION_NULL );

			if ( MoveIsInvalid() ) {
				Bot_ExitAINode();
				Bot_ClearAIStack();
				return false;
			}

			if ( nbgReached == true ) {
				Bot_ExitAINode();
				Bot_ClearAIStack();
				return false;
			}
	
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
			return true;
		}
	}

	nbgReached = true;

	FindChargeInWorld( actionNum, bombInfo, ARM );

	if ( !ClientHasChargeInWorld( botNum, false, actionNum ) && bombInfo.entNum == 0 && botInfo->bombChargeUsed > 0 && nbgTarget == -1 ) { //mal: haven't planted yet and have no charge, or have no bomb in world, so just sit tight and wait.
		if ( botThreadData.random.RandomInt( 100 ) > 95 ) {
			if ( Bot_RandomLook( vec ) ) {
                Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
			}
		}

		botIdealWeapSlot = GUN;

		Bot_MoveToGoal( vec3_zero, vec3_zero, (  nbgPosture != WALK ) ? nbgPosture : CROUCH, NULLMOVETYPE );

		int blockedClient = Bot_CheckBlockingOtherClients( -1 ); //mal: try not to block other clients who may also want to plant, or get by.

		if ( blockedClient != -1 && nbgTryMoveCounter < MAX_MOVE_ATTEMPTS ) {
			nbgTryMoveCounter++;

			botMoveFlags_t botMoveFlag = ( botInfo->posture == IS_CROUCHED ) ? CROUCH : WALK;

			if ( Bot_CanMove( BACK, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
			} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
			} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
			}

			Bot_LookAtEntity( blockedClient, SMOOTH_TURN );
			nbgMoveTime = botWorld->gameLocalInfo.time + 1000;
			return true;
		}

		return true;
	}

	if ( bombInfo.entNum > 0 ) {
        Bot_LookAtLocation( bombInfo.origin, SMOOTH_TURN );

		vec = bombInfo.origin - botInfo->origin;

		botUcmd->actionEntityNum = bombInfo.entNum; ///mal: dont avoid this bomb.
		botUcmd->actionEntitySpawnID = bombInfo.spawnID;

		if ( vec.LengthSqr() > Square( PLIERS_RANGE ) ) {
			Bot_MoveToGoal( bombInfo.origin, vec3_zero, RUN, NULLMOVETYPE ); //mal: get there as quick as possible
		} else {
			Bot_MoveToGoal( vec3_zero, vec3_zero, Bot_FindStanceForLocation( bombInfo.origin, true ), NULLMOVETYPE ); //mal: use the posture that fits the bomb's origin...
		}

		if ( vec.LengthSqr() < Square( PLIERS_RANGE * 2.0f ) ) { //mal: dont start using activate til get close, or can get into trouble....
			weaponLocked = true;

			botUcmd->botCmds.activate = true;
		
			if ( botInfo->weapInfo.weapon == PLIERS ) {
				botUcmd->botCmds.activateHeld = true;
			}
		}
		return true;
	}

	if ( botInfo->bombChargeUsed == 0 && nbgTarget == -1 ) {
		if ( nbgTimer < 60 ) {
			if ( !botThreadData.botActions[ actionNum ]->actionBBox.IntersectsBox( playerBox ) ) {
				nbgReached = false;
				return true;
			}

			nbgOrigin.z = botInfo->viewOrigin.z; //mal: this screws up plants on the ground.
			Bot_LookAtLocation( nbgOrigin, INSTANT_TURN );
			Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE ); //mal: assume the posture defined by the action...
			botIdealWeapSlot = NO_WEAPON;
			botIdealWeapNum = HE_CHARGE;
			botUcmd->botCmds.attack = true;
		} else {
			if ( nbgTimer < 70 ) {
                if ( Bot_CanMove( BACK, 100.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
					if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
						Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
					} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
						Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
					}
				} else {
					Bot_ExitAINode();	//mal: if we're stuck for some reason, and cant plant, exit out of here and try again.
					Bot_ClearAIStack();
					return false;
				}
			} else {
				nbgTimer = 0;				//mal: we've moved some, reset everything and try to move to the obj again and plant.
				nbgReached = false;
			}
		}
		nbgTimer++;
		return true;
	}

	if ( ClientHasChargeInWorld( botNum, true, actionNum ) ) { //mal: we've planted, and theres no more bombs around to arm, so decide to camp around the charge or lets leave!
		if ( nbgTarget == -1 ) {

			if ( botThreadData.random.RandomInt( 100 ) > 75 ) {
				Bot_ExitAINode();
				Bot_ClearAIStack();
				return false; //mal: if random, dont camp charge, just run off.
			}
            
			if ( botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].inuse != false ) { //mal: if the first target action == false, they all must, so skip
                for( i = 0; i < MAX_LINKACTIONS; i++ ) {
					if ( botThreadData.botActions[ actionNum ]->actionTargets[ i ].inuse != false ) {
						int friendsAtTarget = ClientsInArea( botNum, botThreadData.botActions[ actionNum ]->actionTargets[ i ].origin, 150.0f, botInfo->team, NOCLASS, false, false, false, false, true );

						if ( friendsAtTarget > 0 ) {
							continue;
						}

						linkedTargets[ k++ ] = i;
					}
				}

				nbgTarget = botThreadData.random.RandomInt( k );
			} else {
                Bot_ExitAINode();
				Bot_ClearAIStack();
				return false;
			}
		}

		botIdealWeapSlot = GUN;

		vec = botThreadData.botActions[ actionNum ]->actionTargets[ nbgTarget ].origin - botInfo->origin;
		dist = vec.LengthSqr();

		if ( dist > Square( 70.0f ) || botInfo->onLadder ) {
			Bot_SetupMove( botThreadData.botActions[ actionNum ]->actionTargets[ nbgTarget ].origin, -1, ACTION_NULL );

			if ( MoveIsInvalid() ) {
				Bot_ExitAINode();
				return false;
			}
		
			Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? SPRINT : RUN );
			return true;
		}

		Bot_MoveToGoal( vec3_zero, vec3_zero, CROUCH, NULLMOVETYPE );

		if ( botThreadData.random.RandomInt( 100 ) > 95 ) {
			if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
                if ( Bot_RandomLook( vec ) )  {
					Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
				}
			} else {
				Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->GetActionOrigin(), SMOOTH_TURN ); //look at our action - make sure noones mucking with our charge.
			}
		}
		return true;
	}

	if ( botThreadData.random.RandomInt( 100 ) > 95 ) {
		if ( Bot_RandomLook( vec ) ) {
			Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
		}
	}

	botIdealWeapSlot = GUN;

	Bot_MoveToGoal( vec3_zero, vec3_zero, (  nbgPosture != WALK ) ? nbgPosture : CROUCH, NULLMOVETYPE );

	int blockedClient = Bot_CheckBlockingOtherClients( -1 ); //mal: try not to block other clients who may also want to plant, or get by.

	if ( blockedClient != -1 && nbgTryMoveCounter < MAX_MOVE_ATTEMPTS ) {
		nbgTryMoveCounter++;

		botMoveFlags_t botMoveFlag = ( botInfo->posture == IS_CROUCHED ) ? CROUCH : WALK;

		if ( Bot_CanMove( BACK, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
		} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
		} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
		}

		Bot_LookAtEntity( blockedClient, SMOOTH_TURN );
		nbgMoveTime = botWorld->gameLocalInfo.time + 1000;
		return true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_StealUniform
================
*/
bool idBotAI::Enter_NBG_StealUniform() {
    
	nbgType = STEAL_UNIFORM;

	NBG_AI_SUB_NODE = &idBotAI::NBG_StealUniform;
	
	nbgTime = botWorld->gameLocalInfo.time + 15000; //mal: 15 seconds to reach this guy!

	nbgReached = false;

	nbgTimer = 0;

	lefty = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? true : false;

	if ( nbgTargetType == CLIENT ) {
		nbgClientNum = nbgTarget;
		nbgOrigin = botWorld->clientInfo[ nbgTarget ].origin;
		nbgTargetSpawnID = botWorld->clientInfo[ nbgTarget ].spawnID;
	} else {
		nbgClientNum = botWorld->playerBodies[ nbgTarget ].bodyOwnerClientNum;
		nbgOrigin = botWorld->playerBodies[ nbgTarget ].bodyOrigin;
		nbgTargetSpawnID = -1;
	}

	lastAINode = "Stealing Uniform";

	return true;	
}

/*
================
idBotAI::NBG_StealUniform
================
*/
bool idBotAI::NBG_StealUniform() {
	
	bool isCorpse = ( nbgTargetType == CLIENT ) ? false : true;
    int areaNum;
	int mates;
	float dist;
	idVec3 vec;
	idVec3 origin;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		if ( nbgTargetType == CLIENT ) {
            Bot_IgnoreClient( nbgTarget, CLIENT_IGNORE_TIME * 2 );
		} else {
			Bot_IgnoreBody( nbgTarget, BODY_IGNORE_TIME * 2 );
		}

		Bot_ExitAINode();
		return false;
	}

	if ( botInfo->isDisguised ) {
		if ( botThreadData.random.RandomInt( 100 ) > 75 ) {
            Bot_AddDelayedChat( botNum, IM_DISGUISED, 1 ); //mal: let everyone know we're in disguise!
		}

		if ( botThreadData.GetBotSkill() > BOT_SKILL_EASY && botWorld->clientInfo[ nbgClientNum ].isBot && botWorld->clientInfo[ nbgClientNum ].health <= 0 ) {
            Bot_AddDelayedChat( nbgClientNum, ENEMY_DISGUISED_AS_ME, 2 );
		}
		Bot_ExitAINode();
		return false;
	}

	if ( nbgTargetType == CLIENT ) {

		if ( !ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) {
			Bot_ExitAINode();
			return false;
		}

		if ( botWorld->clientInfo[ nbgTarget ].health > 0 && botWorld->clientInfo[ nbgTarget ].revived == true ) {
            Bot_ExitAINode(); //mal: he got revived, leave here now incase his buddies are around
			return false;
		}

		if ( botWorld->clientInfo[ nbgTarget ].inLimbo || botWorld->clientInfo[ nbgTarget ].health > 0 ) {

			int newBody = -1;
	
			nbgTimer++; //mal: it may be a few frames before the body shows up, since there are delays coming from running in a seperate thread, and from the script system itself.

			if ( nbgTimer > 10 ) { //mal: this is taking too long!
                Bot_ExitAINode();
				return false;
			}

			newBody = FindBodyOfClient( nbgTarget, true, nbgOrigin ); //mal: find the body of the client we were originally gunning for.

			if ( newBody != -1 ) {
				nbgTarget = newBody;
				nbgTargetType = BODY;
				nbgClientNum = botWorld->playerBodies[ nbgTarget ].bodyOwnerClientNum;
				nbgOrigin = botWorld->playerBodies[ nbgTarget ].bodyOrigin;
				return true;
			}

			if ( nbgReached == true ) { //mal: if we already reached the goal, stay down, dont fidget!
				Bot_MoveToGoal( vec3_zero, vec3_zero, Bot_FindStanceForLocation( nbgOrigin, false ), NULLMOVETYPE );
			}

			return true;
		}

		mates = ClientsInArea( botNum, botWorld->clientInfo[ nbgTarget ].origin, 150.0f, botInfo->team, COVERTOPS, false, false, false, true, true );

		if ( mates >= 1 ) {
            Bot_ExitAINode();
			return false;
		}

		if ( botInfo->team == STROGG ) {
			mates = ClientsInArea( botNum, botWorld->clientInfo[ nbgTarget ].origin, 150.0f, botInfo->team, MEDIC, false, false, false, false, true );

			if ( mates >= 1 ) {
				Bot_ExitAINode();
				return false;
			}
		}

		origin = botWorld->clientInfo[ nbgTarget ].origin;
		areaNum = botWorld->clientInfo[ nbgTarget ].areaNum;

	} else { //mal: type must be a body

		if ( botWorld->playerBodies[ nbgTarget ].bodyOwnerClientNum != nbgClientNum || botWorld->playerBodies[ nbgTarget ].isValid == false ) { //mal: list could have been resorted, especially if a lot of action is going on, so check to make sure we're still up to date.

			int newBody = -1;
	
			newBody = FindBodyOfClient( nbgClientNum, true, nbgOrigin ); //mal: find the body of the client we were originally gunning for.

			if ( newBody == -1 ) {
				Bot_ExitAINode();
				return false;
			}
		
			nbgTarget = newBody;
			nbgClientNum = botWorld->playerBodies[ nbgTarget ].bodyOwnerClientNum;
			nbgOrigin = botWorld->playerBodies[ nbgTarget ].bodyOrigin;
		}

		if ( botWorld->playerBodies[ nbgTarget ].uniformStolen ) { //mal: already spawnhosted, gibbed, uniform stolen, etc.
            Bot_ExitAINode(); 
			return false;
		}

		mates = ClientsInArea( botNum, botWorld->playerBodies[ nbgTarget ].bodyOrigin, 150.0f, botInfo->team, COVERTOPS, false, false, false, true, true );

		if ( mates >= 1 ) {
            Bot_ExitAINode();
			return false;
		}

		if ( botInfo->team == STROGG ) {
			mates = ClientsInArea( botNum, botWorld->playerBodies[ nbgTarget ].bodyOrigin, 150.0f, botInfo->team, MEDIC, false, false, false, false, true );

			if ( mates >= 1 ) {
				Bot_ExitAINode();
				return false;
			}
		}

		origin = botWorld->playerBodies[ nbgTarget ].bodyOrigin;
		areaNum = botWorld->playerBodies[ nbgTarget ].areaNum;
	}

    vec = origin - botInfo->origin;
	dist = vec.LengthSqr();

	botUcmd->actionEntityNum = ( nbgTargetType == CLIENT ) ? nbgTarget :  botWorld->playerBodies[ nbgTarget ].bodyNum; ///mal: dont avoid this client.
	botUcmd->actionEntitySpawnID = ( nbgTargetType == CLIENT ) ? nbgTargetSpawnID : botWorld->playerBodies[ nbgTarget ].spawnID;

	if ( dist > Square( 45.0f ) || botInfo->onLadder ) {
		Bot_SetupMove( origin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}
		
		Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? SPRINT : RUN );
		return true;
	}

	nbgPosture = Bot_FindStanceForLocation( nbgOrigin, false );

	if ( !nbgReached ) {
		nbgReached = true;
		nbgTime = botWorld->gameLocalInfo.time + 7000; //mal: give ourselves 7 seconds to complete this task once reach target
	}

	if ( BodyIsObstructed( nbgTarget, isCorpse ) ) {
        if ( botInfo->hasCrosshairHint == false ) { //mal: body may be sitting on another body, so shift around to get a better view, if possible.
			if ( lefty == true ) {
				if ( Bot_CanMove( LEFT, 50.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
				} else {
					lefty = false;
				}
			} else {
				if ( Bot_CanMove( RIGHT, 50.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
				} else {
					lefty = true;
				}
			}
		} else {
			Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE );
		}
	} else {
		Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE );
	}

	Bot_LookAtLocation( origin, INSTANT_TURN );

	weaponLocked = true;

	botUcmd->botCmds.activate = true;
	botUcmd->botCmds.activateHeld = true;

	return true;	
}

/*
================
idBotAI::Enter_NBG_HuntVictim
================
*/
bool idBotAI::Enter_NBG_HuntVictim() {
    
	nbgType = STALK_VICTIM;

	NBG_AI_SUB_NODE = &idBotAI::NBG_HuntVictim;
	
	nbgTime = botWorld->gameLocalInfo.time + 60000; // a mintute to hunt this guy down - too long?

	nbgTimer = 0;

	nbgReached = false;

	fastAwareness = true;

	ResetRandomLook();

	nbgSwitch = true; //mal: just always go for the back stab.

	lastAINode = "Hunting Victim";

	return true;	
}

/*
================
idBotAI::NBG_HuntVictim
================
*/
bool idBotAI::NBG_HuntVictim() {

	bool isFacingUs;
	float dist;
	idVec3 vec;

	if ( !ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
        Bot_ExitAINode();
		return false;
	}

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up
		Bot_IgnoreClient( nbgTarget, CLIENT_IGNORE_TIME ); //mal: this client isn't easy to kill, so just ignore him for a while
		Bot_ExitAINode();
		fastAwareness = false;
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ nbgTarget ];

	if ( !botInfo->isDisguised ) { //mal: lost our uniform! Prolly going to have to fight VERY soon.
		Bot_ExitAINode();
		return false;
	}

	if ( ClientIsDead( nbgTarget ) ) { //mal: hes dead jim!
        Bot_ExitAINode();

		if ( !playerInfo.isBot ) {
			Bot_AddDelayedChat( botNum, GENERAL_TAUNT, 1, true ); //rub it in his face
		}
		fastAwareness = false;
		return false;
	}

	if ( playerInfo.friendsInArea > MAX_NUM_OF_FRIENDS_OF_VICTIM || playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: he found some backup or a ride, not safe anymore!
        Bot_ExitAINode();
		fastAwareness = false;
		return false;
	}

    vec = playerInfo.origin - botInfo->origin;
	dist = vec.LengthSqr();

	botIdealWeapSlot = GUN;

	if ( nbgReached != false && dist > Square( 300.0f ) ) { //we tried to attack him, but he got away somehow - so just kill him.
		Bot_ExitAINode();
		botUcmd->botCmds.dropDisguise = true;
		return false;
	}

//mal: use very similiar dists and movement as we would use if we were going to escort someone on our own team. So that you can't tell at first glance something is up...
	if ( dist > Square( 175.0f ) || botInfo->onLadder ) {
		if ( Bot_CanBackStabClient( nbgTarget ) ) {
            Bot_SetupMove( botBackStabMoveGoal, -1, ACTION_NULL );
		} else {
			 Bot_SetupMove( vec3_zero, nbgTarget, ACTION_NULL );
		}

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			fastAwareness = false;
			return false;
		}
		
		Bot_MoveAlongPath( ( dist > Square( 900.0f ) ) ? SPRINT : RUN ); //mal: dont sprint too close to target, our breathing gives us away!
		nbgTimer = 0;
		return true;
	}

	isFacingUs = InFrontOfClient( nbgTarget, botInfo->origin );

	if ( nbgReached == false ) {
		if ( isFacingUs || playerInfo.xySpeed > 250.0f ) {
			if ( nbgSwitch == false ) {
				if ( dist < Square( 150.0f ) ) { // too close, back up some!  
					if ( Bot_CanMove( BACK, 100.0f, true )) {
						Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
					} else if ( Bot_CanMove( RIGHT, 100.0f, true )) {
						Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
					} else if ( Bot_CanMove( LEFT, 100.0f, true )) {
						Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
					}
        
					Bot_LookAtEntity( nbgTarget, SMOOTH_TURN );
					nbgTimer = 0;
					return true;
				} else {
					if ( nbgTimer < 10 ) {
						Bot_LookAtEntity( nbgTarget, SMOOTH_TURN ); //mal: look at our target for a bit when first reached.
						nbgTimer++;
					} else {
						if ( botThreadData.random.RandomInt( 100 ) > 97 ) {
							idVec3 vec;
							if ( Bot_RandomLook( vec ) ) {
						        Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for "enemies" and whatnot - muhahahahahaha!
							}
						}
					}
					return true;
				}
			} else { //mal: sometimes we'll always try to get behind the target, instead of pretending we're his friend.
				if ( Bot_CanBackStabClient( nbgTarget, BACKSTAB_DIST * 1.2f ) ) {
					Bot_SetupMove( botBackStabMoveGoal, -1, ACTION_NULL );

					if ( MoveIsInvalid() ) {
						nbgSwitch = false;
						return true;
					}

					Bot_MoveAlongPath( RUN );
					return true;
				} else {
					nbgSwitch = false;
					return true;
				}
			}
		}
	}

    botIdealWeapSlot = MELEE;
	botIdealWeapNum = NULL_WEAP;

	nbgReached = true; //mal: once we go in for the kill, keep doing it no matter what!

	botUcmd->actionEntityNum = nbgTarget;
	botUcmd->actionEntitySpawnID = nbgTargetSpawnID;

	if ( dist > Square( 45.0f ) ) {
		vec = playerInfo.origin;
		vec += ( -35.0f * playerInfo.viewAxis[ 0 ] );
		Bot_MoveToGoal( vec, vec3_zero, ( playerInfo.posture == IS_PRONE ) ? CROUCH : SPRINT, NULLMOVETYPE );
	} else {
		Bot_MoveToGoal( vec3_zero, vec3_zero, ( playerInfo.posture == IS_PRONE ) ? CROUCH : SPRINT, NULLMOVETYPE );
	}

	Bot_LookAtLocation( ( playerInfo.posture == IS_PRONE ) ? playerInfo.origin : playerInfo.viewOrigin, INSTANT_TURN );
       
	if ( botInfo->weapInfo.isReady && botInfo->weapInfo.weapon == KNIFE ) { //mal: DIE!
		botUcmd->botCmds.activate = true;
/*
		botUcmd->botCmds.attack = true;
		botUcmd->botCmds.constantFire = true;
*/
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_DefuseBomb
================
*/
bool idBotAI::Enter_NBG_DefuseBomb() {
    
	nbgType = DEFUSE_BOMB;

	NBG_AI_SUB_NODE = &idBotAI::NBG_DefuseBomb;
	
	nbgTime = botWorld->gameLocalInfo.time + BOT_INFINITY;

	nbgReached = false;

	nbgMoveTimer = botWorld->gameLocalInfo.time + ( MAX_MOVE_FAILED_TIME * 2 );
	
	lastAINode = "Defusing";

	ResetRandomLook();

	nbgExit = false;

	nbgSwitch = false; //mal: start out scanning from the head of the bot for the charge.

	nbgReachedTarget = false;

	return true;	
}

/*
================
idBotAI::NBG_DefuseBomb
================
*/
bool idBotAI::NBG_DefuseBomb() {
	float dist;
	plantedChargeInfo_t bombInfo;
	idVec3 vec;
	nbgTarget = -1;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: player decided to be a hero, so let them.
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) {
        Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->ArmedChargesInsideActionBBox( -1 ) ) { //mal: someone defused all the charges, so leave.
		Bot_ExitAINode();
		return false;
	}

	if ( nbgReached == false ) {
		if ( nbgMoveTimer < botWorld->gameLocalInfo.time ) {
			Bot_ExitAINode();
			Bot_ClearAIStack();
			return false;
		}
	}

	FindChargeInWorld( actionNum, bombInfo, DISARM );

	if ( bombInfo.entNum > 0 ) {

		nbgTarget = bombInfo.entNum;

 		vec = bombInfo.origin - botInfo->origin;

		float distMultiply = 1.0f;

		if ( vec.z > 100.0f ) {
			distMultiply = 1.5f;
		}

		dist = vec.LengthSqr();

		botUcmd->actionEntityNum = bombInfo.entNum;
		botUcmd->actionEntitySpawnID = bombInfo.spawnID;

		if ( nbgReachedTarget == false ) {
			nbgSwitch = !nbgSwitch;
			
			bool bombIsVisible = Bot_CheckLocationIsVisible( bombInfo.origin, bombInfo.entNum, -1, nbgSwitch );

			if ( dist > Square( PLIERS_RANGE * distMultiply ) || botInfo->onLadder || !botInfo->hasGroundContact || !bombIsVisible ) { //mal: if we're too far away from the charge, use AAS to path to it.
				Bot_SetupMove( bombInfo.origin, -1, ACTION_NULL, bombInfo.areaNum );

				if ( botThreadData.AllowDebugData() ) {
					botAAS.aas->DrawArea( bombInfo.areaNum );
				}

				if ( MoveIsInvalid() ) {
					Bot_ExitAINode();
					return false;
				}
		
				Bot_MoveAlongPath( ( dist > Square( 500.0f ) ) ? SPRINT : RUN );
				return true;
			}

			nbgReachedTarget = true;
		}

		botMoveFlags_t posture;

		if ( nbgSwitch == true && vec.z < 50 ) {
			posture = CROUCH;
		} else {
			posture = WALK;
		}

		if ( !Bot_CheckLocationIsVisible( bombInfo.origin, bombInfo.entNum, -1, nbgSwitch ) ) { //mal: someone may have bumped us away from our charge, so keep rechecking...
			nbgReachedTarget = false;
			return true;
		}

        Bot_LookAtLocation( bombInfo.origin, INSTANT_TURN );

		nbgReached = true;

		Bot_MoveToGoal( vec3_zero, vec3_zero, posture, NULLMOVETYPE );

		weaponLocked = true;

		botUcmd->botCmds.activate = true;
		
		if ( botInfo->weapInfo.weapon == PLIERS ) {
			botUcmd->botCmds.activateHeld = true;
		}
	} else {
		if ( botThreadData.random.RandomInt( 100 ) > 98 || nbgExit == false ) {
			idVec3 vec;
	        if ( Bot_RandomLook( vec ) )  {
		        Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
				nbgExit = true; 
			}
		}
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_GetSupplies
================
*/
bool idBotAI::Enter_NBG_GetSupplies() {
    
	nbgType = GRAB_SUPPLIES;

	NBG_AI_SUB_NODE = &idBotAI::NBG_GetSupplies;
	
	nbgTime = botWorld->gameLocalInfo.time + 9000;

	nbgReached = false;

	ResetRandomLook();

	lastAINode = "Grabbing Item";

	return true;	
}

/*
================
idBotAI::NBG_GetSupplies
================
*/
bool idBotAI::NBG_GetSupplies() {
	bool isTouching = false;
	int packSpawnID = -1;
	idVec3 packOrg;
	idVec3 vec;
	idBounds entBounds;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreItem( nbgTarget, IGNORE_ITEM_TIME );
		Bot_ExitAINode();
		return false;
	}

	if ( nbgTargetType == SUPPLY_CRATE ) {
		if ( botInfo->health == botInfo->maxHealth && botInfo->weapInfo.primaryWeapNeedsAmmo == false && ( botInfo->weapInfo.hasNadeAmmo == true || botInfo->isDisguised ) ) { //mal: might as well make sure we're decked out before we leave....
			Bot_ExitAINode();
			return false;
		}
	} else if ( nbgTargetType == HEAL ) {
        if ( botInfo->health == botInfo->maxHealth ) { 
			Bot_ExitAINode();
			return false;
		}
	} else {
#ifdef PACKS_HAVE_NO_NADES
		if ( botInfo->weapInfo.primaryWeapNeedsAmmo == false ) {
			Bot_ExitAINode();
			return false;
		}
#else
		if ( botInfo->weapInfo.primaryWeapNeedsAmmo == false && ( botInfo->weapInfo.hasNadeAmmo == true || botInfo->isDisguised ) ) {
			Bot_ExitAINode();
			return false;
		}
#endif
	}

	if ( !CheckItemPackIsValid( nbgTarget, packOrg, entBounds, packSpawnID ) ) {
		Bot_ExitAINode();
		return false;
	}

	botUcmd->actionEntityNum = nbgTarget; //mal: dont avoid our target.
	botUcmd->actionEntitySpawnID = packSpawnID;

	vec = packOrg - botInfo->origin;

	if ( nbgTargetType == SUPPLY_CRATE ) { 
		entBounds.TranslateSelf( packOrg );
		entBounds.Expand( NORMAL_BOUNDS_EXPAND );

		if ( botInfo->absBounds.IntersectsBounds( entBounds ) ) {
			isTouching = true;
		}
	}

	if ( isTouching == false || botInfo->onLadder ) {
		Bot_SetupMove( packOrg, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreItem( nbgTarget, IGNORE_ITEM_TIME );
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( vec.LengthSqr() > Square( 100.0f ) ) ? SPRINT : RUN );
		return true;
	}

//mal: if we get here, we're up next to a supply crate, so crouch down and wait...
    Bot_MoveToGoal( vec3_zero, vec3_zero, CROUCH, NULLMOVETYPE ); //mal: crouch down by the crate, hiding ourselves while we resupply.

	if ( botThreadData.random.RandomInt( 100 ) > 98 || nbgReached == false ) {
        if ( Bot_RandomLook( vec ) )  {
            Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
			nbgReached = true; 
		}
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_BugForSupplies
================
*/
bool idBotAI::Enter_NBG_BugForSupplies() {
    
	nbgType = BUG_FOR_SUPPLIES;

	NBG_AI_SUB_NODE = &idBotAI::NBG_BugForSupplies;
	
	nbgTime = botWorld->gameLocalInfo.time + 15000;

	nbgReached = false;

	nbgTimer = 0;

	nbgExit = false;

	ResetRandomLook();

	lastAINode = "Bugging For Supplies";

	return true;	
}

/*
================
idBotAI::NBG_BugForSupplies
================
*/
bool idBotAI::NBG_BugForSupplies() {
	float dist;
	idVec3 vec;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( nbgTargetType == HEAL ) {
        if ( botInfo->health >= 100 ) { 
			Bot_ExitAINode();
			return false;
		}
	} else {
		if ( botInfo->weapInfo.primaryWeapNeedsAmmo == false && nbgExit == false ) {
			nbgTime = botWorld->gameLocalInfo.time + 1000;
			nbgExit = true;
		}
	}

	if ( !ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) {
		Bot_ExitAINode();
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ nbgTarget ];

	if ( ClientIsDead( nbgTarget ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( playerInfo.classChargeUsed > 80 && nbgExit == false ) { //mal: if the mate has no juice, lets leave. Humans will signal this to a player, so we'll let the bots know too.
		nbgTime = botWorld->gameLocalInfo.time + 1000;
		nbgExit = true;
	}

	if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
		Bot_ExitAINode();
		return false;
	}

	if ( enemy == -1 ) { //mal: someone is attacking our mate - so let's kill the SOB!
		int attacker = CheckClientAttacker( nbgTarget, 1 );

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

	vec = playerInfo.origin - botInfo->origin;
	dist = vec.LengthFast();

	botUcmd->actionEntityNum = nbgTarget;
	botUcmd->actionEntitySpawnID = nbgTargetSpawnID;

	if ( dist > 175.0f || botInfo->onLadder ) {
		Bot_SetupMove( playerInfo.origin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( RUN );
		nbgReached = false;
		nbgTimer = 0;
		return true;
	}

	if ( !playerInfo.isBot ) {
		if ( nbgTargetType == HEAL ) {
			botUcmd->desiredChat = HEAL_ME;
		} else if ( nbgTargetType == REARM ) {
			botUcmd->desiredChat = REARM_ME;
		}
	}

	if ( dist < 50.0f ) { // too close, back up some! 
        if ( Bot_CanMove( BACK, 100.0f, true )) {
            Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			Bot_LookAtEntity( nbgTarget, SMOOTH_TURN );
			nbgReached = false;
			nbgTimer = 0;
			return true;
		}
	}

	nbgTimer++;

	if ( nbgTimer > 15 ) { //mal: look at our client for a few seconds, then start to look around for enemies.
        if ( botThreadData.random.RandomInt( 100 ) > 95 || nbgReached == false ) {
			if ( Bot_RandomLook( vec ) )  {
				Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
				nbgReached = true; 
			}
		}
	} else {
		Bot_LookAtEntity( nbgTarget, SMOOTH_TURN );
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_AvoidDanger
================
*/
bool idBotAI::Enter_NBG_AvoidDanger() {
    
	nbgType = AVOID_DANGER;

	NBG_AI_SUB_NODE = &idBotAI::NBG_AvoidDanger;
	
	nbgReached = false;

	nbgSwitch = false;

	nbgReachedTarget = true;

	ResetRandomLook();

	botIdealWeapSlot = GUN;

	lastAINode = "Avoiding Danger";

	return true;	
}

/*
================
idBotAI::NBG_AvoidDanger
================
*/
bool idBotAI::NBG_AvoidDanger() {
	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !DangerStillExists( currentDangers[ nbgTarget ].num, currentDangers[ nbgTarget ].ownerNum ) ) {
		ClearDangerFromDangerList( nbgTarget, false );
		Bot_ExitAINode();
		return false;
	}

	float avoidDist = ( currentDangers[ nbgTarget ].type == THROWN_AIRSTRIKE ) ? 1500.0f : 512.0f;

	if ( nbgReachedTarget ) { //mal: if we find no action, we're screwed.
		if ( !nbgSwitch ) {
			nbgSwitch = true;
			actionNum = Bot_FindNearbySafeActionToMoveToward( currentDangers[ nbgTarget ].origin, avoidDist );
			
			if ( actionNum == ACTION_NULL ) {
				nbgReachedTarget = false;
				return false;
			}
		}

		idVec3 vec = currentDangers[ nbgTarget ].origin - botInfo->origin;

		if ( vec.LengthSqr() < Square( avoidDist ) ) {
			vec = botThreadData.botActions[ actionNum ]->GetActionOrigin() - botInfo->origin;

			if ( vec.LengthSqr() > Square( 50.0f ) || botInfo->onLadder ) {
				Bot_SetupMove( vec3_zero, -1, actionNum );

				if ( MoveIsInvalid() ) {
					nbgReachedTarget = false;
					return false;
				}

				Bot_MoveAlongPath( SPRINT );

				if ( currentDangers[ nbgTarget ].type != THROWN_AIRSTRIKE ) {
					vec = currentDangers[ nbgTarget ].origin;
					idVec3 tempOrigin = currentDangers[ nbgTarget ].origin - botInfo->viewOrigin;
	
					if ( tempOrigin[ 2 ] > -100.0f && tempOrigin[ 2 ] < 100.0f ) { //mal: adjust the origin to eye level, so the bots aren't staring at the ground.
						vec[ 2 ] -= tempOrigin[ 2 ];
					}

					botUcmd->specialMoveType = QUICK_JUMP;
	
					Bot_LookAtLocation( vec, SMOOTH_TURN ); //mal: look at the danger we're aware of - possibly its owner is around.
				}
				return true;
			}
		}
		}

	idVec3 vec;

				if ( Bot_RandomLook( vec ) )  {
					Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_PlantMine
================
*/
bool idBotAI::Enter_NBG_PlantMine() {
    
	nbgType = PLANT_MINE;

	NBG_AI_SUB_NODE = &idBotAI::NBG_PlantMine;
	
	nbgTime = botWorld->gameLocalInfo.time + 15000;

	nbgReached = false;

	nbgTimer = 0; // track how long we've been trying to plant, if for too long, try something else.

	nbgTryMoveCounter = 0;
	
	nbgMoveTime = 0;

	ResetRandomLook();

	botThreadData.botActions[ actionNum ]->FindRandomPointInBBox( nbgOrigin, botNum, botInfo->team ); //mal: find a random point within this action's BBox to move towards.

	nbgPosture = botThreadData.botActions[ actionNum ]->posture;

	if ( nbgPosture == RANDOM_STANCE ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			nbgPosture = CROUCH;
		} else {
			nbgPosture = WALK;
		}
	}

	lastAINode = "Planting Mine";

	return true;	
}

/*
================
idBotAI::NBG_PlantMine
================
*/
bool idBotAI::NBG_PlantMine() {
	plantedMineInfo_t mineInfo;
	idVec3 vec;
	idBox playerBox;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) {
        Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

if ( nbgMoveTime < botWorld->gameLocalInfo.time ) {
		nbgTryMoveCounter = 0;
	}

	vec = nbgOrigin - botInfo->origin;

	playerBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );

//mal: move into the bbox for this action, so we know we're close enough to start the planting process if we just got here.
	if ( nbgReached == false ) {
		if ( !botThreadData.botActions[ actionNum ]->actionBBox.IntersectsBox( playerBox ) ) {
			Bot_SetupQuickMove( nbgOrigin, false );
//			Bot_SetupMove( nbgOrigin, -1, ACTION_NULL );


			if ( MoveIsInvalid() ) {
				Bot_ExitAINode();
				Bot_ClearAIStack();
				return false;
			}

			if ( nbgReached == true ) {
				Bot_ExitAINode();
				Bot_ClearAIStack();
				return false;
			}
	
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
			Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
			return true;
		}
	}

	nbgReached = true;

	FindMineInWorld( mineInfo );

#ifdef _XENON
	if ( mineInfo.entNum != 0 ) {	//mal: mines dont need to be armed on the consoles.
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}
#endif

	if ( botThreadData.botActions[ actionNum ]->ArmedMinesInsideActionBBox() && mineInfo.entNum == 0 ) { //mal: the mine is planted, and theres no other mines, so leave.
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( mineInfo.entNum == 0 && botInfo->deviceChargeUsed > 70 && NumPlayerMines() < MAX_MINES ) { //mal: haven't planted yet and have no charge, or have no mine in world, so just sit tight and wait.
		if ( botThreadData.random.RandomInt( 100 ) > 98 ) {
			if ( Bot_RandomLook( vec ) ) {
                Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
			}
		}

		botIdealWeapSlot = GUN;

		Bot_MoveToGoal( vec3_zero, vec3_zero, (  nbgPosture != WALK ) ? nbgPosture : CROUCH, NULLMOVETYPE );

		int blockedClient = Bot_CheckBlockingOtherClients( -1 ); //mal: try not to block other clients who may also want to plant, or get by.

		if ( blockedClient != -1 && nbgTryMoveCounter < MAX_MOVE_ATTEMPTS ) {
			nbgTryMoveCounter++;
			botMoveFlags_t botMoveFlag = ( botInfo->posture == IS_CROUCHED ) ? CROUCH : WALK;

			if ( Bot_CanMove( BACK, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
			} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
			} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
			}

			Bot_LookAtEntity( blockedClient, SMOOTH_TURN );
			nbgMoveTime = botWorld->gameLocalInfo.time + 1000;
			return true;
		}
		return true;
	}

 	if ( mineInfo.entNum > 0 ) {
        Bot_LookAtLocation( mineInfo.origin, SMOOTH_TURN );

		vec = mineInfo.origin - botInfo->origin;

		botUcmd->actionEntityNum = mineInfo.entNum; ///mal: dont avoid this
		botUcmd->actionEntitySpawnID = mineInfo.spawnID;

		if ( vec.LengthSqr() > Square( PLIERS_RANGE ) ) {
			Bot_MoveToGoal( mineInfo.origin, vec3_zero, RUN, NULLMOVETYPE ); //mal: get there as quick as possible
		} else {
			Bot_MoveToGoal( vec3_zero, vec3_zero, Bot_FindStanceForLocation( mineInfo.origin, ( nbgPosture == PRONE ) ? true : false ), NULLMOVETYPE );
		}

		weaponLocked = true;

		botUcmd->botCmds.activate = true;
		
		if ( botInfo->weapInfo.weapon == PLIERS ) {
			botUcmd->botCmds.activateHeld = true;
		}
		return true;
	}

	if ( botInfo->deviceChargeUsed < 70 ) {
		if ( nbgTimer < 60 ) {
 			botThreadData.botActions[ actionNum ]->FindBBoxCenterLinePoint( nbgOrigin );
			nbgOrigin.z = botInfo->viewOrigin.z;
			Bot_LookAtLocation( nbgOrigin, INSTANT_TURN );
			Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE ); //mal: assume the posture defined by the action...
			botIdealWeapSlot = NO_WEAPON;
			botIdealWeapNum = LANDMINE;
			botUcmd->botCmds.attack = true;
		} else {
			if ( nbgTimer < 70 ) {
                if ( Bot_CanMove( BACK, 100.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
					if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
						Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
					} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
						Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
					}
				} else {
					if ( !Bot_CheckHasUnArmedMineNearby() ) {
						Bot_ExitAINode();	//mal: if we're stuck for some reason, and cant plant, exit out of here and try again.
						Bot_ClearAIStack();
						return false;
					} else {
						nbgTimer = 0;
						nbgReached = false;
					}
				}
			} else {
				nbgTimer = 0;				//mal: we've moved some, reset everything and try to move to the obj again and plant.
				nbgReached = false;
			}
		}
		nbgTimer++;
		return true;
	}

	if ( NumPlayerMines() >= MAX_MINES ) {
        Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

  	return true;	
}

/*
================
idBotAI::Enter_NBG_Snipe
================
*/
bool idBotAI::Enter_NBG_Snipe() {
    
	nbgType = SNIPE;

	NBG_AI_SUB_NODE = &idBotAI::NBG_Snipe;
	
	nbgTime = botWorld->gameLocalInfo.time + ( botThreadData.botActions[ actionNum ]->actionTimeInSeconds * 1000 ); //mal: timelimit is specified by action itself!

	nbgReached = false;

	ResetRandomLook(); //mal: start out looking this far at random targets

	stayInPosition = false;

	nbgTryMoveCounter = 0;
	
	nbgMoveTime = 0;

	nbgPosture = botThreadData.botActions[ actionNum ]->posture;

	if ( nbgPosture == RANDOM_STANCE ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			nbgPosture = CROUCH;
		} else {
			nbgPosture = WALK;
		}
	}

	botIdealWeapSlot = GUN;

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

	lastAINode = "Sniping";

	return true;	
}

/*
================
idBotAI::NBG_Snipe
================
*/
bool idBotAI::NBG_Snipe() {
	int i;  
	int k = 0;
	int blockedClient;
	int linkedTargets[ MAX_LINKACTIONS ];
	float dist;
	idVec3 vec;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		Bot_ClearAIStack();
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

	if ( !botThreadData.botActions[ actionNum ]->active ) {
        Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( botInfo->isDisguised ) {
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( nbgMoveTime < botWorld->gameLocalInfo.time ) {
		nbgTryMoveCounter = 0;
	}

//mal: this should never happen, but just in case bot somehow got bumped away from its snipe spot......
	vec = botThreadData.botActions[ actionNum ]->origin - botInfo->origin;
	dist = vec.LengthFast();

	if ( nbgReached == true ) {
        blockedClient = Bot_CheckBlockingOtherClients( -1 );
	} else {
		blockedClient = -1;
	}

	if ( dist > botThreadData.botActions[ actionNum ]->radius && blockedClient == -1 || botInfo->onLadder ) {
		Bot_SetupMove( vec3_zero, -1, actionNum );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			Bot_ClearAIStack();
			return false;
		}

		nbgReached = false; //mal: we're on the move		
		Bot_MoveAlongPath( ( dist > 100.0f ) ? SPRINT : RUN );
		ResetRandomLook();
		return true;
	}

	if ( Bot_IsUnderAttackFromUnknownEnemy() ) { //mal: dont sit there! Someones shooting you!
		Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME );
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE ); //mal: assume the posture defined by the action...

	if ( botThreadData.random.RandomInt( 100 ) > 98 || nbgReached == false ) {
		if ( botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].inuse != false && heardClient == -1 ) { //mal: if the first target action == false, they all must, so skip
			for( i = 0; i < MAX_LINKACTIONS; i++ ) {
				if ( botThreadData.botActions[ actionNum ]->actionTargets[ i ].inuse != false ) {
					linkedTargets[ k++ ] = i;
				}
			}

			i = botThreadData.random.RandomInt( k );

			nbgReached = true;

			Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->actionTargets[ i ].origin, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
		} else {
			if ( heardClient == -1 || botInfo->isDisguised ) {
                if ( botThreadData.random.RandomInt( 100 ) > 98 || nbgReached == false ) {
                    if ( Bot_RandomLook( vec ) )  {
                        Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
                        nbgReached = true; //mal: we reached our goal, and am going to camp now. ONLY after we've looked around a bit.
					}
				}
			} else {
                if ( nbgPosture != PRONE ) { //mal: unless we're prone, be sure to crouch if we hear someone. Try to suprise our incoming "guest".
					Bot_MoveToGoal( vec3_zero, vec3_zero, CROUCH, NULLMOVETYPE );
				}

                Bot_LookAtLocation( botWorld->clientInfo[ heardClient ].origin, SMOOTH_TURN ); //we hear someone - look at where they're at.
				nbgReached = false; //mal: as long as we hear someone, keep this updated.
			}
		}
	}

	if ( blockedClient != -1 && nbgTryMoveCounter < MAX_MOVE_ATTEMPTS ) {
		nbgTryMoveCounter++;
		botMoveFlags_t botMoveFlag = ( botInfo->posture == IS_CROUCHED ) ? CROUCH : WALK;

		if ( Bot_CanMove( BACK, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
		} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
		} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, botMoveFlag, NULLMOVETYPE );
		}

		Bot_LookAtEntity( blockedClient, SMOOTH_TURN );
		nbgMoveTime = botWorld->gameLocalInfo.time + 1000;
		return true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_DestroyDanger
================
*/
bool idBotAI::Enter_NBG_DestroyDanger() {
    
	nbgType = DESTROY_DANGER;

	NBG_AI_SUB_NODE = &idBotAI::NBG_DestroyDanger;
	
	nbgTime = botWorld->gameLocalInfo.time + BOT_INFINITY; //mal: we'll attack this target until its dead, or we're incapable of hurting it anymore

	nbgReached = false;

	nbgTimer = 0;

	ResetRandomLook(); //mal: start out looking this far at random targets

	if ( nbgTargetType == LANDMINE_DANGER && botThreadData.random.RandomInt( 100 ) > 70 ) { //mal: let everyone know this bot is a bit preoccupied with destroying some mines.
		Bot_AddDelayedChat( botNum, MINES_SPOTTED, 2 );
	}

	lastAINode = "Destroying Danger";

	return true;	
}

/*
================
idBotAI::NBG_DestroyDanger
================
*/
bool idBotAI::NBG_DestroyDanger() {
	float dist;
	float tooCloseDist = ( nbgTargetType == STROGG_HIVE_DANGER ) ? 900.0f : 500.0f; 
	trace_t	tr;
	idVec3 vec;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !DangerStillExists( currentDangers[ nbgTarget ].num, currentDangers[ nbgTarget ].ownerNum ) ) {
		ClearDangerFromDangerList( nbgTarget, false );
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_HasExplosives( false ) && nbgTargetType != STROGG_HIVE_DANGER ) {
#ifdef PACKS_HAVE_NO_NADES
		Bot_ExitAINode();
		return false;
#else
		if ( ( botInfo->classType == MEDIC && botInfo->team == STROGG ) || ( botInfo->classType == FIELDOPS && botInfo->team == GDF ) ) {
			if ( supplySelfTime < botWorld->gameLocalInfo.time ) {
                supplySelfTime = botWorld->gameLocalInfo.time + 5000;
			}
			return true; //mal: we'll wait here while we resupply ourselves.
		} else {
			Bot_ExitAINode();
			return false;
		}
#endif
	}

	idVec3 dangerOrigin = currentDangers[ nbgTarget ].origin;

	vec = dangerOrigin - botInfo->origin;
	dist = vec.LengthSqr();

	botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, dangerOrigin, MASK_SHOT_RENDERMODEL | MASK_SHOT_BOUNDINGBOX /*BOT_VISIBILITY_TRACE_MASK*/, GetGameEntity( botNum ) );

	if ( dist < Square( tooCloseDist ) && ( tr.fraction == 1.0f || tr.c.entityNum == currentDangers[ nbgTarget ].num ) && !botInfo->inWater && !botInfo->onLadder ) {
		if ( Bot_CanMove( BACK, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, SPRINT, NULLMOVETYPE );
		}
	} else if ( tr.fraction != 1.0f && tr.c.entityNum != currentDangers[ nbgTarget ].num || botInfo->inWater || botInfo->onLadder ) {
		Bot_SetupMove( dangerOrigin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( RUN );
		return true;
	}

	if ( nbgTargetType == STROGG_HIVE_DANGER && tr.fraction < 1.0f && tr.c.entityNum != currentDangers[ nbgTarget ].num ) {
		Bot_ExitAINode();
		ClearDangerFromDangerList( nbgTarget, false ); //mal: in case we find it again fairly soon.
		return false;
	}

	nbgTimer++;

	if ( nbgTargetType == LANDMINE_DANGER ) {
		dangerOrigin.z += 16.0f; //mal: aim a bit high of the danger, so that we get a better chance to destroy it.
	}

	if ( nbgTimer < 25 && nbgTargetType != STROGG_HIVE_DANGER ) { //mal: pause for just a bit before we attack the mine, danger, etc.
		Bot_LookAtLocation( dangerOrigin, SMOOTH_TURN );
		return true;
	}

	if ( nbgTargetType == STROGG_HIVE_DANGER ) {
		botIdealWeapNum = NULL_WEAP;
		
		if ( botInfo->weapInfo.primaryWeapHasAmmo && botInfo->weapInfo.primaryWeapon != ROCKET ) {
			botIdealWeapSlot = GUN;
		} else if ( botInfo->weapInfo.sideArmHasAmmo ) {
			botIdealWeapSlot = SIDEARM;
		} else {
			Bot_ExitAINode();
			return false;
		}

		Bot_LookAtLocation( dangerOrigin, INSTANT_TURN ); 
		
		if ( InFrontOfClient( botNum, dangerOrigin ) ) {
			botUcmd->botCmds.attack = true;
		}
	} else if ( dist < Square( 1200.0f ) ) {
        if ( nbgTargetType == LANDMINE_DANGER || nbgTargetType == GDF_CAM_DANGER ) {
			if ( botInfo->weapInfo.hasNadeAmmo ) {
				botIdealWeapSlot = NADE;
				botIdealWeapNum = NULL_WEAP;
			} else if ( botInfo->classType == FIELDOPS && LocationVis2Sky( dangerOrigin ) && ClassWeaponCharged( AIRCAN ) ) {
				botIdealWeapNum = AIRCAN;
				botIdealWeapSlot = NO_WEAPON;
			} else if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo && dist > Square( 500.0f ) ) {
				botIdealWeapSlot = GUN;
			} else { //mal_FIXME: add engs using their nade launchers here, and coverts using hives/3rd eye cameras
				Bot_ExitAINode();
				return false;
			}
		}

		if ( botIdealWeapSlot == NADE ) {
			if ( !ClientHasNadeInWorld( botNum ) ) {
				Bot_ThrowGrenade( dangerOrigin, true );
			} else {
				nbgTimer = 0;
				botIdealWeapSlot = GUN;
				botIdealWeapNum = NULL_WEAP;
			}
		} else if ( botIdealWeapNum == AIRCAN ) {
			Bot_UseCannister( AIRCAN, dangerOrigin );
		} else if ( botIdealWeapSlot == GUN || botIdealWeapSlot == SIDEARM ) {
			Bot_LookAtLocation( dangerOrigin, INSTANT_TURN ); 
			if ( InFrontOfClient( botNum, dangerOrigin ) ) {
                botUcmd->botCmds.attack = true;
			}
		}
	} else if ( botAAS.obstacleNum == -1 ) { //mal: make sure we're not waiting out a current danger ( charge/mine/nade explosion ).
		if ( dist > Square( 350.0f ) || tr.fraction < 1.0f ) {
			Bot_SetupMove( dangerOrigin, -1, ACTION_NULL );

			if ( MoveIsInvalid() ) {
				Bot_ExitAINode();
				return false;
			}

			Bot_MoveAlongPath( ( dist > 100.0f ) ? SPRINT : RUN );
			nbgReached = false;
			return true;
		}
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_FixProxyEntity
================
*/
bool idBotAI::Enter_NBG_FixProxyEntity() {

	proxyInfo_t vehicleInfo;

	GetVehicleInfo( nbgTarget, vehicleInfo ); 

	if ( vehicleInfo.type != MCP ) {
		nbgType = FIX_VEHICLE;
	} else {
		nbgType = FIXING_MCP;
	}
    
	NBG_AI_SUB_NODE = &idBotAI::NBG_FixProxyEntity;
	
	nbgTime = botWorld->gameLocalInfo.time + ( ( botInfo->abilities.stroggRepairDrone ) ? DRONE_REPAIR_TIME : 30000 ); //mal: 30 seconds to fix this vehicle.

	botIdealWeapSlot = GUN;

	nbgReached = false;

	lastAINode = "Fixing Vehicle";

	if ( nbgChat ) {
		Bot_AddDelayedChat( botNum, ACKNOWLEDGE_YES, 1 );
	} else {
		if ( vehicleInfo.driverEntNum > -1 && vehicleInfo.driverEntNum < MAX_CLIENTS ) {
			const clientInfo_t& player = botWorld->clientInfo[ vehicleInfo.driverEntNum ];
			
			if ( !player.isBot ) {
				Bot_AddDelayedChat( botNum, WILL_FIX_RIDE, 1 );
			}
		}
	}

	return true;	
}

/*
================
idBotAI::NBG_FixProxyEntity
================
*/
bool idBotAI::NBG_FixProxyEntity() {
	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreVehicle( nbgTarget, 15000 );						
		Bot_ExitAINode();
		return false;
	}

	proxyInfo_t vehicleInfo;

	GetVehicleInfo( nbgTarget, vehicleInfo ); 

	if ( vehicleInfo.entNum == 0 ) {
		Bot_ExitAINode();
		return false;
	}

	if ( vehicleInfo.health == vehicleInfo.maxHealth ) {
        Bot_ExitAINode();
		return false;
	}

	if ( vehicleInfo.type == MCP && !botWorld->botGoalInfo.mapHasMCPGoal ) {
		Bot_ExitAINode();
		return false;
	}

	if ( vehicleInfo.type == MCP && vehicleInfo.health > ( vehicleInfo.maxHealth / 2 ) && vehicleInfo.driverEntNum == -1 ) {
		Bot_ExitAINode();
		return false;
	} //mal: jump in the MCP once its drivable, unless theres someone in it already - then keep fixing!

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
		if ( Bot_CheckForHumanInteractingWithEntity( vehicleInfo.entNum ) == true ) {
			Bot_ExitAINode();
			return false;
		}
	}

	botUcmd->actionEntityNum = vehicleInfo.entNum; //mal: let the game and obstacle avoidance know we want to interact with this entity.
	botUcmd->actionEntitySpawnID = vehicleInfo.spawnID;

	if ( Bot_UseRepairDrone( vehicleInfo.entNum, vehicleInfo.origin ) ) {
		return true;
	}

	idVec3 vec = vehicleInfo.origin - botInfo->origin; 
	float distSqr = vec.LengthSqr();

	if ( ( vehicleInfo.type == MCP && distSqr > Square( 700.0f ) ) || ( vehicleInfo.type != MCP && ( vehicleInfo.forwardSpeed > WALKING_SPEED || vehicleInfo.forwardSpeed < -WALKING_SPEED ) ) ) { //mal: its moving too fast
        Bot_ExitAINode(); 
		return false;
	} //mal: chase the MCP and fix as we go if we're close, else we'll give up on moving vehicles.

	idBox vehicleBox = idBox( vehicleInfo.bbox, vehicleInfo.origin, vehicleInfo.axis );
	vehicleBox.ExpandSelf( VEHICLE_BOX_EXPAND );

	idBox botBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );

	if ( botThreadData.AllowDebugData() ) {
		gameRenderWorld->DebugBox( colorRed, vehicleBox, 16 );
		gameRenderWorld->DebugBox( colorGreen, botBox, 16 );
	}

	botMoveFlags_t defaultMoveFlag = RUN;
	bool fireOnTheMove = false;

	if ( vehicleInfo.type == MCP && vehicleInfo.xyspeed > WALKING_SPEED && nbgReached ) {
		defaultMoveFlag = SPRINT;
		botIdealWeapNum = PLIERS;
		botIdealWeapSlot = NO_WEAPON;
		fireOnTheMove = true;
	}

//mal: move into the bbox of this proxy entity, so we know we're close enough to start fixing it
	if ( !botBox.IntersectsBox( vehicleBox ) || botInfo->onLadder ) {
		idVec3 vehicleOrigin = vehicleInfo.origin;
		vehicleOrigin.z += VEHICLE_PATH_ORIGIN_OFFSET;

		Bot_SetupMove( vehicleOrigin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( distSqr > Square( 500.0f ) ) ? SPRINT : defaultMoveFlag );

		if ( distSqr < Square( LOOK_AT_GOAL_DIST ) || botInfo->weapInfo.weapon == PLIERS ) {
			Bot_LookAtLocation( vehicleInfo.origin, SMOOTH_TURN );
		}

		return true;
	}

	vec = vehicleInfo.origin;  
	vec.z += 64.0f;

	Bot_LookAtLocation( vec, SMOOTH_TURN );

	nbgReached = true;

	if ( vehicleInfo.forwardSpeed < WALKING_SPEED && vehicleInfo.forwardSpeed > -WALKING_SPEED ) { //mal: only stop and crouch down to fix, for vehicles that aren't moving.
	nbgPosture = Bot_FindStanceForLocation( vehicleInfo.origin, false );
		Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE );
	}

	if ( !fireOnTheMove ) {
	weaponLocked = true;
	botUcmd->botCmds.activate = true;

	if ( botInfo->weapInfo.weapon == PLIERS && botInfo->hasCrosshairHint ) {
		botUcmd->botCmds.activateHeld = true;
	}
	} else {
		if ( botInfo->weapInfo.weapon == PLIERS && botInfo->hasCrosshairHint ) {
			botUcmd->botCmds.attack = true;
			botUcmd->botCmds.constantFire = true;
		}
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_TKReviveTeammate

This is not something your common bot ( or human ) medic will do. But a smart, wise team player will conserve his precious health packs, and take the XP hit, to 
bring a teammate up to full health much faster.
================
*/
bool idBotAI::Enter_NBG_TKReviveTeammate() {
    
	nbgType = TK_REVIVE_TEAMMATE;

	NBG_AI_SUB_NODE = &idBotAI::NBG_TKReviveTeammate;
	
	nbgTime = botWorld->gameLocalInfo.time + 15000; //mal: 15 seconds to heal this guy!

	nbgDist = 175.0f;

	if ( ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
		const clientInfo_t& playerInfo = botWorld->clientInfo[ nbgTarget ];

		if ( !playerInfo.isBot ) {
			Bot_AddDelayedChat( botNum, TK_REVIVE_CHAT, 1 );
		}
	}

	nbgTimer = 0;

	lastAINode = "TK Reviving";

	botUcmd->tkReviveTime = botWorld->gameLocalInfo.time + TK_REVIVE_CHAT_TIME;

	return true;
}

/*
================
idBotAI::NBG_TKReviveTeammate
================
*/
bool idBotAI::NBG_TKReviveTeammate() {
	bool hasClearShot = false;
	float dist;
	idVec3 vec;

	if ( !ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc.
        Bot_ExitAINode();
		return false;
	}

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME ); //mal: this client isn't easy to heal, so just ignore him for a while
		Bot_ExitAINode();
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ nbgTarget ];

	if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: client jumped into a vehicle/deployable - forget them
		Bot_ExitAINode();
		return false;
	}

	if ( enemy != -1 ) {
		Bot_ExitAINode();
		return false;
	}

	if ( playerInfo.enemiesInArea > 1 || botInfo->enemiesInArea > 1 ) {
		Bot_ExitAINode();
		return false;
	}

	if ( ClientIsDead( nbgTarget ) || playerInfo.inLimbo ) { //mal: hes gone jim! Do something else! If we killed him, we'll revive him next
		Bot_ExitAINode();
		return false;
	}

	if ( playerInfo.health > TK_REVIVE_HEALTH ) { //mal: he somehow got health, leave and just heal him rest of the way
		Bot_ExitAINode();
		return false;
	}

	vec = playerInfo.origin - botInfo->origin;
	dist = vec.LengthSqr();

	botUcmd->actionEntityNum = nbgTarget; ///mal: dont avoid this client.  
	botUcmd->actionEntitySpawnID = nbgTargetSpawnID;

	if ( dist < Square( 700.0f ) ) { //mal: get the weapon ready....
        botIdealWeapSlot = NO_WEAPON;

		if ( botInfo->weapInfo.sideArmHasAmmo ) {
			botIdealWeapNum = PISTOL;
		} else if ( botInfo->weapInfo.primaryWeapHasAmmo ) {
			botIdealWeapNum = SMG;
		} else {
			botIdealWeapNum = KNIFE;
			nbgDist = 50.0f;
		}
	}

	if ( dist > Square( nbgDist ) || botInfo->onLadder ) {
		Bot_SetupMove( vec3_zero, nbgTarget, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ExitAINode();
			return false;
		}
		
		Bot_MoveAlongPath( ( dist > Square( 200.0f ) ) ? SPRINT : RUN );

		if ( dist < Square( LOOK_AT_GOAL_DIST ) ) {
			Bot_LookAtEntity( nbgTarget, SMOOTH_TURN );
		}

		return true;
	}

	Bot_ClearAngleMods(); 
	Bot_LookAtEntity( nbgTarget, AIM_TURN );

	hasClearShot = ClientIsVisibleToBot( nbgTarget, true, false );

	if ( !hasClearShot ) {
        if ( lefty == true ) {
            if ( Bot_CanMove( LEFT, 50.0f, true ) ) {
                Bot_MoveToGoal( botCanMoveGoal, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
            } else {
                lefty = false;
			}
		} else {
			if ( Bot_CanMove( RIGHT, 50.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
			} else {
				lefty = true;
			}
		}

		nbgTimer++;

		if ( nbgTimer > 10 ) { //mal: something is blocking our view to our target, so move in REAL close.
			nbgDist = 50.0f;
		}
	}
	
	if ( botInfo->weapInfo.isReady && botInfo->weapInfo.weapon == botIdealWeapNum && hasClearShot ) {
		botUcmd->botCmds.attack = true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_DestroyMCP
================
*/
bool idBotAI::Enter_NBG_DestroyMCP() {
    
	nbgType = DESTROY_MCP;

	NBG_AI_SUB_NODE = &idBotAI::NBG_DestroyMCP;
	
	nbgTime = botWorld->gameLocalInfo.time + 30000;

	nbgReached = false;

	nbgTimer = 0;

	lastAINode = "Destroying MCP";

	return true;	
}

/*
================
idBotAI::NBG_DestroyMCP
================
*/
bool idBotAI::NBG_DestroyMCP() {
	float dist;
	float tooCloseDist = 350.0f; 
	proxyInfo_t	vehicleInfo;
	idVec3 vec;


	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	if ( !botWorld->botGoalInfo.mapHasMCPGoal ) {
		Bot_ExitAINode();
		return false;
	}

	if ( botWorld->botGoalInfo.botGoal_MCP_VehicleNum < MAX_CLIENTS ) {
		Bot_ExitAINode();
		return false;
	}

	GetVehicleInfo( botWorld->botGoalInfo.botGoal_MCP_VehicleNum, vehicleInfo );

	if ( vehicleInfo.entNum == 0 ) {
		Bot_ExitAINode();
		return false;
	}

	if ( vehicleInfo.isImmobilized ) {
		Bot_ExitAINode();
		return false;
	}

	if ( vehicleInfo.driverEntNum != -1 ) { //mal: a MCP with a driver will be attacked differently
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_HasExplosives( false ) ) {
#ifndef PACKS_HAVE_NO_NADES
		if ( ( botInfo->classType == MEDIC && botInfo->team == STROGG ) || ( botInfo->classType == FIELDOPS && botInfo->team == GDF ) ) {
			if ( supplySelfTime < botWorld->gameLocalInfo.time ) {
                supplySelfTime = botWorld->gameLocalInfo.time + 5000;
			}
			return true; //mal: we'll wait here while we resupply ourselves.
		}
#endif
	}

	vec = vehicleInfo.origin - botInfo->origin;
	dist = vec.LengthSqr();

	if ( dist < Square( tooCloseDist ) ) {
		if ( Bot_CanMove( BACK, 100.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, SPRINT, NULLMOVETYPE );
		}
	}

	if ( dist < Square( ATTACK_MCP_DIST ) ) { 
        if ( botInfo->weapInfo.hasNadeAmmo ) {
			botIdealWeapSlot = NADE;
			botIdealWeapNum = NULL_WEAP;
		} else if ( botInfo->classType == FIELDOPS && LocationVis2Sky( vehicleInfo.origin ) && ClassWeaponCharged( AIRCAN ) ) {
			botIdealWeapNum = AIRCAN;
			botIdealWeapSlot = NO_WEAPON;
		} else if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo ) {
			botIdealWeapSlot = GUN;
		} else if ( botInfo->weapInfo.primaryWeapHasAmmo && vehicleInfo.health < ( vehicleInfo.maxHealth / 2 ) ) {
            botIdealWeapSlot = GUN;
		} else {
			Bot_ExitAINode();
			return false;
		}

	    if ( botIdealWeapSlot == NADE ) {
			Bot_ThrowGrenade( vehicleInfo.origin, true );
		} else if ( botIdealWeapNum == AIRCAN ) {
			Bot_UseCannister( AIRCAN, vehicleInfo.origin );
		} else if ( botIdealWeapSlot == GUN || botIdealWeapSlot == SIDEARM ) {
			Bot_LookAtLocation( vehicleInfo.origin, INSTANT_TURN ); 
			
			if ( InFrontOfClient( botNum, vehicleInfo.origin ) ) {
		        botUcmd->botCmds.attack = true;
			}
		}
	} else {
		if ( botInfo->classType == FIELDOPS && LocationVis2Sky( vehicleInfo.origin ) && ClassWeaponCharged( AIRCAN ) && Bot_HasWorkingDeployable() && !Bot_EnemyAITInArea( vehicleInfo.origin ) ) {
			Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
			Bot_LookAtLocation( vehicleInfo.origin, SMOOTH_TURN );
			botIdealWeapNum = BINOCS;

			if ( nbgTimer == 0 ) {
				nbgTimer = botWorld->gameLocalInfo.time + ARTY_LOCKON_TIME;
			}
			
			if ( nbgTimer > ARTY_LOCKON_TIME ) {
				return true;
			}

			if ( botInfo->weapInfo.weapon == BINOCS ) {
				botUcmd->botCmds.attack = true;
				botUcmd->botCmds.constantFire = true;
				return true;
			}
		} else if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo ) {
			botIdealWeapSlot = GUN;

			if ( nbgReached == false ) {
				nbgReached = true;
			} else {
				botUcmd->botCmds.altAttackOn = true;
			}

			if ( botInfo->targetLocked != false ) {
				if ( botInfo->targetLockEntNum == vehicleInfo.entNum ) {
					botUcmd->botCmds.attack = true;
				} else {
					botUcmd->botCmds.altAttackOn = false;
					nbgReached = false;
				}
			}

			Bot_LookAtLocation( vehicleInfo.origin, INSTANT_TURN ); 
		} else {

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
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_DestroySpawnHost
================
*/
bool idBotAI::Enter_NBG_DestroySpawnHost() {
    
	nbgType = DESTORY_SPAWNHOST;

	NBG_AI_SUB_NODE = &idBotAI::NBG_DestroySpawnHost;
	
	nbgTime = botWorld->gameLocalInfo.time + 9000; //mal: 9 seconds to zap this guy!

	nbgReached = false;

	lefty = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? true : false;

	nbgTimer = 0;

	nbgExit = false;

	lastAINode = "Destroying SpawnHost";

	if ( nbgChat ) {
		Bot_AddDelayedChat( botNum, ACKNOWLEDGE_YES, 1 );
		nbgTime += 21000; //mal: give ourselves more time to complete the task since we may be further away from it if ordered to destroy it.
	}

	return true;	
}

/*
================
idBotAI::NBG_DestroySpawnHost
================
*/
bool idBotAI::NBG_DestroySpawnHost() {
 	int mates;
	float dist;
	idVec3 vec;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreSpawnHost( nbgTarget, BODY_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	if ( botWorld->spawnHosts[ nbgTarget ].entNum == 0 ) {
		Bot_ExitAINode(); //mal: got gibbed, expired, etc.
		return false;
	}

	mates = ClientsInArea( botNum, botWorld->spawnHosts[ nbgTarget ].origin, 150.0f, botInfo->team , MEDIC, false, false, false, false, true );

	if ( mates >= 1 ) { // whether they're zapping the spawnhost or not, this goal isn't critical enough to have 2+ medics in the area!
		Bot_ExitAINode();
		return false;
	}

	vec = botWorld->spawnHosts[ nbgTarget ].origin - botInfo->origin;
	dist = vec.LengthSqr();

	botUcmd->actionEntityNum = botWorld->spawnHosts[ nbgTarget ].entNum; ///mal: dont avoid this spawnhost.
	botUcmd->actionEntitySpawnID = botWorld->spawnHosts[ nbgTarget ].spawnID;

	if ( dist > Square( 65.0f ) || botInfo->onLadder ) {
		Bot_SetupMove( botWorld->spawnHosts[ nbgTarget ].origin, -1, ACTION_NULL, botWorld->spawnHosts[ nbgTarget ].areaNum );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? SPRINT : RUN );

		if ( dist < Square( LOOK_AT_GOAL_DIST ) ) {
			Bot_LookAtLocation( botWorld->spawnHosts[ nbgTarget ].origin, SMOOTH_TURN );
		}

		nbgReached = false;
		return true;
	}

	nbgPosture = RUN;

	if ( dist < 10.0f ) {
        if ( Bot_CanMove( BACK, 100.0f, true )) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
			nbgReached = false;
			return true;
		}
	}

	if ( !nbgReached ) {
		nbgReached = true;
		nbgTime = botWorld->gameLocalInfo.time + 3000; //mal: give ourselves 3 seconds to complete this task once reach target
	}

	Bot_ClearAngleMods();
	Bot_LookAtLocation( botWorld->spawnHosts[ nbgTarget ].origin, SMOOTH_TURN );

	if ( BodyIsObstructed( nbgTarget, false, true ) ) {
        if ( botInfo->hasCrosshairHint == false && botInfo->weapInfo.weapon != NEEDLE ) { //mal: body may be sitting on another body, so shift around to get a better view, if possible.
			if ( lefty == true ) {
				if ( Bot_CanMove( LEFT, 50.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
				} else {
					lefty = false;
				}
			} else {
				if ( Bot_CanMove( RIGHT, 50.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
				} else {
					lefty = true;
				}
			}
		} else {
			Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE );
		}
	} else {
		Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE );
	}

	weaponLocked = true;

	botUcmd->botCmds.activate = true; 

	if ( botInfo->weapInfo.weapon == NEEDLE ) {
		botUcmd->botCmds.activateHeld = true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_GrabSpawnPoint
================
*/
bool idBotAI::Enter_NBG_GrabSpawnPoint() {

	nbgType = GRAB_SPAWN;

 	NBG_AI_SUB_NODE = &idBotAI::NBG_GrabSpawnPoint;
	 
	nbgTime = botWorld->gameLocalInfo.time + 15000; //mal: 15 seconds to do this task!

	nbgReached = false;

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	botThreadData.botActions[ actionNum ]->FindRandomPointInBBox( nbgOrigin, botNum, botInfo->team ); //mal: find a random point within this action's BBox to move towards.

	nbgPosture = botThreadData.botActions[ actionNum ]->posture;

	if ( nbgPosture == RANDOM_STANCE ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			nbgPosture = CROUCH;
		} else {
			nbgPosture = WALK;
		}
	}

	lastAINode = "Grabbing Spawn";

	return true;	
}

/*
================
idBotAI::NBG_GrabSpawnPoint
================
*/
bool idBotAI::NBG_GrabSpawnPoint() {

	idVec3 vec;
	idBox playerBox;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
		if ( Bot_CheckForHumanInteractingWithEntity( botThreadData.botActions[ actionNum ]->GetActionSpawnControllerEntNum() ) == true ) {
			Bot_ExitAINode();
			return false;
		}
	}

	if ( !botThreadData.botActions[ actionNum ]->active ) {
        Bot_ExitAINode();
		Bot_ClearAIStack();
		return false;
	}

	if ( ltgType == STEAL_SPAWN_GOAL ) {
		if ( botThreadData.botActions[ actionNum ]->GetTeamOwner() == NOTEAM ) { //mal: its back to normal, we can go about our business.
			Bot_ExitAINode();
			return false;;
		}
	} else {
		if ( botThreadData.botActions[ actionNum ]->GetTeamOwner() == botInfo->team ) { //mal: its ours!
			Bot_ExitAINode();
			return false;;
		}
	}

	vec = nbgOrigin - botInfo->origin;

	playerBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );

//mal: move into the bbox for this action, so we know we're close enough to start the capture process
	if ( !botThreadData.botActions[ actionNum ]->actionBBox.IntersectsBox( playerBox ) || botInfo->onLadder ) {
		Bot_SetupQuickMove( nbgOrigin, false );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			Bot_ClearAIStack();
			return false;
		}

		if ( nbgReached == true ) {
			Bot_ExitAINode();
			Bot_ClearAIStack();
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
		Bot_LookAtLocation( botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	Bot_LookAtLocation( nbgOrigin, SMOOTH_TURN );
	Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE ); //mal: assume the posture defined by the action...

	weaponLocked = true;

	botUcmd->botCmds.activate = true;

	if ( nbgReached == true ) {
		botUcmd->botCmds.activateHeld = true;
	}

	nbgReached = true;
	return true;	
}

/*
================
idBotAI::Enter_NBG_FixDeployable
================
*/
bool idBotAI::Enter_NBG_FixDeployable() {
    
 	nbgType = FIX_DEPLOYABLE;

	NBG_AI_SUB_NODE = &idBotAI::NBG_FixDeployable;
	
	nbgTime = botWorld->gameLocalInfo.time + ( ( botInfo->abilities.stroggRepairDrone ) ? DRONE_REPAIR_TIME : 30000 ); //mal: 30 seconds to fix this deployable.

	botIdealWeapSlot = GUN;

	lastAINode = "Fixing Deployable";

	return true;	
}

/*
================
idBotAI::NBG_FixDeployable
================
*/
bool idBotAI::NBG_FixDeployable() {
	float dist;
	deployableInfo_t deployableInfo;
	idVec3 vec;
	idBox botBox, deployableBox;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreDeployable( nbgTarget, 15000 );						
		Bot_ExitAINode();
		return false;
	}

	if ( !GetDeployableInfo( false, nbgTarget, deployableInfo ) ) { //mal: make sure still exists
		Bot_ExitAINode();
		return false;
	}

	if ( deployableInfo.health == deployableInfo.maxHealth ) {
        Bot_ExitAINode();
		return false;
	}

	if ( deployableInfo.health <= 0 ) {
        Bot_ExitAINode();
		return false;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
		if ( Bot_CheckForHumanInteractingWithEntity( deployableInfo.entNum ) == true ) {
			Bot_ExitAINode();
			return false;
		}
	}

	botUcmd->actionEntityNum = deployableInfo.entNum; //mal: let the game and obstacle avoidance know we want to interact with this entity.
	botUcmd->actionEntitySpawnID = deployableInfo.spawnID;

	if ( Bot_UseRepairDrone( deployableInfo.entNum, deployableInfo.origin ) ) {
		return true;
	}

	vec = deployableInfo.origin - botInfo->origin; 
	dist = vec.LengthSqr();

	deployableBox = idBox( deployableInfo.bbox, deployableInfo.origin, deployableInfo.axis );
	deployableBox.ExpandSelf( VEHICLE_BOX_EXPAND );

	botBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );   

//mal: move into the bbox of this proxy entity, so we know we're close enough to start fixing it
	if ( !botBox.IntersectsBox( deployableBox ) || botInfo->onLadder ) {
		idVec3 moveGoal = deployableInfo.origin;
		moveGoal.z += DEPLOYABLE_PATH_ORIGIN_OFFSET;
		Bot_SetupMove( moveGoal, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 500.0f ) ) ? SPRINT : RUN );

		return true;
	}

	vec = deployableInfo.origin;  

	vec[ 2 ] += 64.0f;

	Bot_LookAtLocation( vec, SMOOTH_TURN );

	nbgPosture = Bot_FindStanceForLocation( deployableInfo.origin, false );

	Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE ); //mal: assume the posture defined by the action...

	weaponLocked = true;

	botUcmd->botCmds.activate = true;

	if ( botInfo->weapInfo.weapon == PLIERS ) {
		botUcmd->botCmds.activateHeld = true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_PlaceDeployable
================
*/
bool idBotAI::Enter_NBG_PlaceDeployable() {

 	nbgType = DROP_DEPLOYABLE;

	NBG_AI_SUB_NODE = &idBotAI::NBG_PlaceDeployable;
	
	nbgTime = botWorld->gameLocalInfo.time + 10000; //mal: 10 seconds to place this deployable.

	nbgTarget = ltgTarget;

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	nbgOrigin = botThreadData.botActions[ actionNum ]->GetActionOrigin();

	nbgTimer = 0;

	if ( botInfo->isDisguised ) {
		botUcmd->botCmds.dropDisguise = true;
	}

	lastAINode = "Placing Deployable";

	return true;	
}

/*
================
idBotAI::NBG_PlaceDeployable
================
*/
bool idBotAI::NBG_PlaceDeployable() {

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreAction( actionNum, 15000 );				
		Bot_ExitAINode();
		return false;
	}

	if ( botInfo->deployDelayTime > botWorld->gameLocalInfo.time ) {
		Bot_IgnoreAction( actionNum, 15000 );
		Bot_ExitAINode();
		return false;
	}

	Bot_LookAtLocation( nbgOrigin, SMOOTH_TURN );

	Bot_MoveToGoal( vec3_zero, vec3_zero, RUN, NULLMOVETYPE ); 

	botIdealWeapSlot = NO_WEAPON;
	botIdealWeapNum = DEPLOY_TOOL;

	if ( nbgTimer == 0  ) {
		nbgTimer = botWorld->gameLocalInfo.time + DEPLOYABLE_PAUSE_TIME;
	}

	if ( nbgTimer > botWorld->gameLocalInfo.time ) {
		return true;
	} else {
		if ( botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].origin != vec3_zero ) {
			botUcmd->deployInfo.aimPoint = botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].origin;
		}

		botUcmd->deployInfo.location = nbgOrigin;
		botUcmd->deployInfo.deployableType = nbgTarget;
		botUcmd->deployInfo.actionNumber = actionNum;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_DestroyAPTDanger
================
*/
bool idBotAI::Enter_NBG_DestroyAPTDanger() {
    
	nbgType = DESTROY_DEPLOYABLE;

	NBG_AI_SUB_NODE = &idBotAI::NBG_DestroyAPTDanger;
	
	nbgTime = botWorld->gameLocalInfo.time + 30000;

	nbgReached = false;

	nbgTimer = 0;

	nbgMoveDir = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? RIGHT : LEFT;

	lastAINode = "Destroying APT";

	return true;	
}

/*
================
idBotAI::NBG_DestroyAPTDanger
================
*/
bool idBotAI::NBG_DestroyAPTDanger() {
	bool useArty = false;
	bool wantsMove = false;
	float dist;
	deployableInfo_t deployableInfo;
	idVec3 vec;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
		Bot_ExitAINode();
		return false;
	}

	if ( !GetDeployableInfo( false, nbgTarget, deployableInfo ) ) { //mal: doesn't exist anymore.
		if ( !DangerStillExists( nbgTarget, nbgClientNum ) ) {
			ClearDangerFromDangerList( nbgTarget, true );
		}
		Bot_ExitAINode();
		return false;
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

	trace_t tr;
	idVec3 org = deployableInfo.origin;
	org.z += DEPLOYABLE_ORIGIN_OFFSET;
	botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, org, MASK_SHOT_BOUNDINGBOX | MASK_VEHICLESOLID | CONTENTS_FORCEFIELD, GetGameEntity( botNum ) );

	if ( tr.fraction < 1.0f && tr.c.entityNum != deployableInfo.entNum ) { 
		Bot_ExitAINode();
		return false;
	} 

	if ( dist < Square( ATTACK_APT_DIST ) ) { 
		if ( botInfo->classType == FIELDOPS && LocationVis2Sky( deployableInfo.origin ) && ClassWeaponCharged( AIRCAN ) ) {
			botIdealWeapNum = AIRCAN;
			botIdealWeapSlot = NO_WEAPON;
		} else if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo ) {
			botIdealWeapSlot = GUN;
			ignoreWeapChange = true;
		} else if ( botInfo->weapInfo.hasNadeAmmo ) {
			botIdealWeapSlot = NADE;
			botIdealWeapNum = NULL_WEAP;
		} else {
			Bot_ExitAINode();
			return false;
		}
	} else {
		if ( botInfo->classType == FIELDOPS && ClassWeaponCharged( AIRCAN ) && Bot_HasWorkingDeployable() && !Bot_EnemyAITInArea( deployableInfo.origin ) ) {
			useArty = true;
		} else if ( botInfo->classType == SOLDIER && botInfo->weapInfo.primaryWeapon == ROCKET && botInfo->weapInfo.primaryWeapHasAmmo ) {
			botIdealWeapSlot = GUN;
			ignoreWeapChange = true;
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
		Bot_LookAtLocation( deployableInfo.origin, SMOOTH_TURN );
		botIdealWeapNum = BINOCS;
		botIdealWeapSlot = NO_WEAPON;
		ignoreWeapChange = true;

		if ( nbgTimer == 0 ) {
			nbgTimer = botWorld->gameLocalInfo.time + ARTY_LOCKON_TIME;
		}

		if ( nbgTimer > botWorld->gameLocalInfo.time ) {
			return true;
		}

		if ( botInfo->weapInfo.weapon == BINOCS ) {
			botUcmd->botCmds.attack = true;
			botUcmd->botCmds.constantFire = true;
			return true;
		}
	} else if ( botIdealWeapSlot == NADE ) {
		bool fastNade = true;

		if ( dist > Square( GRENADE_THROW_MAXDIST ) ) {
			idVec3 moveGoal = deployableInfo.origin;
			moveGoal.z += DEPLOYABLE_PATH_ORIGIN_OFFSET;
			Bot_SetupMove( moveGoal, -1, ACTION_NULL );
		
			if ( MoveIsInvalid() ) {
				Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
				Bot_ExitAINode();
				return false;
			}

			Bot_MoveAlongPath( SPRINT );
			fastNade = false;
		} else {
			if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			}
		}
		Bot_ThrowGrenade( deployableInfo.origin, fastNade );
	} else if ( botIdealWeapNum == AIRCAN ) {
		Bot_UseCannister( AIRCAN, deployableInfo.origin );
	} else if ( botIdealWeapSlot == GUN || botIdealWeapSlot == SIDEARM ) {
		if ( dist < Square( 500.0f ) ) { // too close, back up some!  
			if ( Bot_CanMove( BACK, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else if ( Bot_CanMove( RIGHT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			} else if ( Bot_CanMove( LEFT, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
			}

			Bot_LookAtLocation( deployableInfo.origin, SMOOTH_TURN ); 
		} else {
			Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
			Bot_LookAtLocation( deployableInfo.origin, SMOOTH_TURN ); 
		}

		if ( nbgReached == false ) {
			rocketLauncherLockFailedTime = botWorld->gameLocalInfo.time + 5000;
			nbgReached = true;
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
				rocketLauncherLockFailedTime = botWorld->gameLocalInfo.time + 10000; //mal: give us time to reload and try again.
			} else {
				botUcmd->botCmds.altAttackOn = false;
				nbgReached = false;
			}
		}
	}

	if ( wantsMove ) {
		if ( dist < Square( ATTACK_APT_DIST ) ) {
			vec = deployableInfo.origin;
			vec.z += DEPLOYABLE_PATH_ORIGIN_OFFSET;

			vec += ( -350.0f * deployableInfo.axis[ 0 ] );

			Bot_SetupMove( vec, -1, ACTION_NULL );

			if ( MoveIsInvalid() ) {
				Bot_IgnoreDeployable( deployableInfo.entNum, 15000 );
				Bot_ExitAINode();
				return false;
			}

			Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? SPRINT : RUN );
		} else {
			if ( nbgMoveDir == NULL_DIR ) {
				nbgMoveDir = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? RIGHT : LEFT;
			}

			if ( Bot_CanMove( nbgMoveDir, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, ( nbgMoveDir == RIGHT ) ? RANDOM_JUMP_RIGHT : RANDOM_JUMP_LEFT );
			} else {
				nbgMoveDir = NULL_DIR;
			}
		}
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_GrabSpawnHost
================
*/
bool idBotAI::Enter_NBG_GrabSpawnHost() {

	nbgType = GRAB_SPAWNHOST;

	NBG_AI_SUB_NODE = &idBotAI::NBG_GrabSpawnHost;
	
	nbgTime = botWorld->gameLocalInfo.time + 9000; //mal: 9 seconds to grab the host

	nbgReached = false;

	lefty = ( botThreadData.random.RandomInt( 100 ) > 50 ) ? true : false;

	lastAINode = "Grabbing SpawnHost";

	return true;	
}

/*
================
idBotAI::NBG_GrabSpawnHost
================
*/
bool idBotAI::NBG_GrabSpawnHost() {
 	int mates;
	float dist;
	idVec3 vec;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreSpawnHost( nbgTarget, BODY_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	if ( botWorld->spawnHosts[ nbgTarget ].entNum == 0 ) {
		Bot_ExitAINode(); //mal: got gibbed, expired, etc.
		return false;
	}

	if ( SpawnHostIsUsed( botWorld->spawnHosts[ nbgTarget ].entNum ) ) {
		Bot_ExitAINode(); 
		return false;;
	}

	if ( botInfo->spawnHostEntNum == botWorld->spawnHosts[ nbgTarget ].entNum ) {
		Bot_ExitAINode(); //mal: success!
		return false;
	}

	mates = ClientsInArea( botNum, botWorld->spawnHosts[ nbgTarget ].origin, 150.0f, botInfo->team , NOCLASS, false, false, false, false, true );

	if ( mates > 0 ) { // whether they're grabbing the spawnhost or not, this goal isn't critical enough to have 2 strogg in the area
		Bot_ExitAINode();
		return false;
	}

	vec = botWorld->spawnHosts[ nbgTarget ].origin - botInfo->origin;
	dist = vec.LengthSqr();

	botUcmd->actionEntityNum = botWorld->spawnHosts[ nbgTarget ].entNum; ///mal: dont avoid this spawnhost.
	botUcmd->actionEntitySpawnID = botWorld->spawnHosts[ nbgTarget ].spawnID;

	if ( dist > Square( 65.0f ) || botInfo->onLadder ) {
		Bot_SetupMove( botWorld->spawnHosts[ nbgTarget ].origin, -1, ACTION_NULL, botWorld->spawnHosts[ nbgTarget ].areaNum );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? SPRINT : RUN );

		if ( dist < Square( LOOK_AT_GOAL_DIST ) ) {
			Bot_LookAtLocation( botWorld->spawnHosts[ nbgTarget ].origin, SMOOTH_TURN );
		}

		return true;
	}

	nbgPosture = RUN;

	if ( dist < 10.0f ) {
        if ( Bot_CanMove( BACK, 100.0f, true )) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
			nbgReached = false;
			return true;
		}
	}

	if ( !nbgReached ) {
		nbgReached = true;
		nbgTime = botWorld->gameLocalInfo.time + 3000; //mal: give ourselves 3 seconds to complete this task once reach target
	}

	Bot_LookAtLocation( botWorld->spawnHosts[ nbgTarget ].origin, SMOOTH_TURN );

	if ( BodyIsObstructed( nbgTarget, false, true ) ) {
        if ( botInfo->hasCrosshairHint == false ) { //mal: body may be sitting on another body, so shift around to get a better view, if possible.
			if ( lefty == true ) {
				if ( Bot_CanMove( LEFT, 50.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
				} else {
					lefty = false;
				}
			} else {
				if ( Bot_CanMove( RIGHT, 50.0f, true ) ) {
					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, nbgPosture, NULLMOVETYPE );
				} else {
					lefty = true;
				}
			}
		} else {
			Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE );
		}
	} else {
		Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE );
	}

	botUcmd->botCmds.activate = true; 
	return true;	
}

/*
================
idBotAI::Enter_NBG_HackDeployable
================
*/
bool idBotAI::Enter_NBG_HackDeployable() {

	deployableInfo_t deployableInfo;
    
 	nbgType = HACK_DEPLOYABLE;

	NBG_AI_SUB_NODE = &idBotAI::NBG_HackDeployable;
	
	nbgTime = botWorld->gameLocalInfo.time + 30000; //mal: 30 seconds to hack this deployable.

	lastAINode = "Hack Deployable";

	botIdealWeapSlot = GUN;

	nbgReachedTarget = false;

	nbgReached = false;

	nbgSwitch = false;

	if ( botInfo->team == GDF && ClassWeaponCharged( THIRD_EYE ) ) {
		if ( GetDeployableInfo( false, nbgTarget, deployableInfo ) ) {
			if ( deployableInfo.type != APT ) { //mal: moving away from the APT may just get us killed.
				int enemiesInArea = ClientsInArea( -1, deployableInfo.origin, 2000.0f, STROGG, NOCLASS, false, false, false, true, false );
				if ( botThreadData.random.RandomInt( 100 ) > 50 || enemiesInArea > 0 ) {
					nbgSwitch = true; //mal: will use 3rd eye on target - its faster this way, and exposes us less.
				}
			}
		}
	}

	return true;	
}

/*
================
idBotAI::NBG_HackDeployable
================
*/
bool idBotAI::NBG_HackDeployable() {
	float dist;
	deployableInfo_t deployableInfo;
	idVec3 vec;
	idBox botBox, deployableBox;

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreDeployable( nbgTarget, 30000 );						
		Bot_ExitAINode();
		return false;
	}

	if ( !GetDeployableInfo( false, nbgTarget, deployableInfo ) ) { //mal: make sure still exists
		Bot_ExitAINode();
		return false;
	}

	if ( ( deployableInfo.disabled && deployableInfo.health == ( deployableInfo.maxHealth / 2.0f ) ) || deployableInfo.health < ( deployableInfo.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) {
		Bot_ExitAINode();
		return false;
	}

	vec = deployableInfo.origin - botInfo->origin; 
	dist = vec.LengthSqr();
	idVec3 deployableOrigin = deployableInfo.origin;
	deployableOrigin.z += DEPLOYABLE_PATH_ORIGIN_OFFSET;

	botUcmd->actionEntityNum = deployableInfo.entNum; //mal: let the game and obstacle avoidance know we want to interact with this entity.
	botUcmd->actionEntitySpawnID = deployableInfo.spawnID;

	deployableBox = idBox( deployableInfo.bbox, deployableInfo.origin, deployableInfo.axis );
	deployableBox.ExpandSelf( NORMAL_BOX_EXPAND );

	botBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis ); 

//mal: move into the bbox of this deployable entity, so we know we're close enough to start hacking it
	if ( !nbgReached && ( !botBox.IntersectsBox( deployableBox ) || botInfo->onLadder ) ) {
		Bot_SetupMove( deployableOrigin, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 300.0f ) ) ? SPRINT : RUN );
		return true;
	}

	if ( nbgSwitch ) { //mal: we want to blow the deployable with our 3rd eye camera....
		botIdealWeapSlot = NO_WEAPON;
		botIdealWeapNum = THIRD_EYE;

		botUcmd->botCmds.dropDisguise = true;

		if ( botInfo->weapInfo.weapon == THIRD_EYE ) {
			if ( !ClassWeaponCharged( THIRD_EYE ) ) {
				if ( !nbgReachedTarget ) { //mal: if just got here, find a nearby action to move towards, just to get us out of here.
					actionNum = Bot_FindNearbySafeActionToMoveToward( deployableInfo.origin, THIRD_EYE_SAFE_RANGE * 2.0f );

					if ( actionNum == ACTION_NULL ) { //mal: can't find a safe action to move towards, so just hack it since we're already here.
						nbgSwitch = false;
						return false;
					}

					nbgReachedTarget = true;
				}

				if ( dist >= Square( THIRD_EYE_SAFE_RANGE ) ) {
					botUcmd->botCmds.attack = true;
					Bot_SetupMove( botThreadData.botActions[ actionNum ]->GetActionOrigin(), -1, ACTION_NULL );
					
					if ( MoveIsInvalid() ) { //mal: can't find a path to our safe action, so just hack it since we're already here.
						nbgSwitch = false;
						return false;
					}

					Bot_MoveAlongPath( SPRINT );
					Bot_LookAtLocation( deployableOrigin, SMOOTH_TURN );
				} else {
					nbgReached = true;

					Bot_SetupMove( botThreadData.botActions[ actionNum ]->GetActionOrigin(), -1, ACTION_NULL );
		
					if ( MoveIsInvalid() ) { //mal: can't find a path to our safe action, so just hack it since we're already here.
						nbgSwitch = false;
						return false;
					}

					Bot_MoveAlongPath( SPRINT );
				}
			} else {
				Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
				Bot_LookAtLocation( deployableOrigin, SMOOTH_TURN ); 
				botUcmd->botCmds.attack = true;
			}
		}
	} else {
		vec = deployableInfo.origin;  
		vec.z += 64.0f;

		Bot_LookAtLocation( vec, SMOOTH_TURN );

		nbgPosture = Bot_FindStanceForLocation( deployableInfo.origin, false );	

		Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE ); //mal: assume the posture defined by the action...

		weaponLocked = true;

		botUcmd->botCmds.activate = true;

		if ( botInfo->weapInfo.weapon == HACK_TOOL ) {
			botUcmd->botCmds.activateHeld = true;
		}
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_MG_Camp

================
*/
bool idBotAI::Enter_NBG_MG_Camp() {

	nbgType = MG_CAMP;

	nbgTime = botWorld->gameLocalInfo.time + ( botThreadData.botActions[ actionNum ]->actionTimeInSeconds * 1000 ); //mal: timelimit is specified by action itself!

	NBG_AI_SUB_NODE = &idBotAI::NBG_MG_Camp;

	nbgMoveTimer = botWorld->gameLocalInfo.time + MAX_MOVE_FAILED_TIME;

	nbgReached = false;

	nbgExit = true;
	
	lastAINode = "MG Camping";

	return true;
}

/*
================
idBotAI::NBG_MG_Camp
================
*/
bool idBotAI::NBG_MG_Camp() {
	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		if ( botInfo->usingMountedGPMG ) {
			botUcmd->botCmds.exitVehicle = true;
		}
		return false;
	}

	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		if ( botInfo->usingMountedGPMG ) {
			botUcmd->botCmds.exitVehicle = true;
		}
		return false;
	}

	if ( botInfo->isDisguised ) {
		Bot_ExitAINode();
		if ( botInfo->usingMountedGPMG ) {
			botUcmd->botCmds.exitVehicle = true;
		}
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->ActionIsActive() ) {
		Bot_ExitAINode();
		if ( botInfo->usingMountedGPMG ) {
			botUcmd->botCmds.exitVehicle = true;
		}
		return false;
	}

	if ( botThreadData.botActions[ actionNum ]->GetActionState() == ACTION_STATE_GUN_DESTROYED ) {
		Bot_ExitAINode();
		if ( botInfo->usingMountedGPMG ) {
			botUcmd->botCmds.exitVehicle = true;
		}
		return false;
	}

	int matesInArea = ClientsInArea( botNum, botThreadData.botActions[ actionNum ]->GetActionOrigin(), 350.0f, botInfo->team, NOCLASS, false, false, false, true, true );

	if ( matesInArea > 0 && !botInfo->usingMountedGPMG ) {
		Bot_ExitAINode();
		return false;
	} //mal: already somebody there, so lets pick a different one to grab.

	if ( nbgReached == false ) {
		if ( nbgMoveTimer < botWorld->gameLocalInfo.time ) {
			Bot_ExitAINode();
			if ( botInfo->usingMountedGPMG ) {
				botUcmd->botCmds.exitVehicle = true;
			}
			return false;
		}

		idBox playerBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );

		if ( !botThreadData.botActions[ actionNum ]->actionBBox.IntersectsBox( playerBox ) || botInfo->usingMountedGPMG == false ) {
			Bot_SetupQuickMove( botThreadData.botActions[ actionNum ]->actionBBox.GetCenter(), false );

			if ( MoveIsInvalid() ) {
				Bot_ExitAINode();
				if ( botInfo->usingMountedGPMG ) {
					botUcmd->botCmds.exitVehicle = true;
				}
				return false;
			}

			botUcmd->botCmds.enterVehicle = true;

			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
			Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->actionBBox.GetCenter(), SMOOTH_TURN );
			return true;
		}
	}

	nbgReached = true;

	if ( Bot_IsUnderAttackFromUnknownEnemy() ) { //mal: dont sit there! Someones shooting you!
		Bot_IgnoreAction( actionNum, ACTION_IGNORE_TIME );
		Bot_ExitAINode();
		Bot_ClearAIStack();
		if ( botInfo->usingMountedGPMG ) {
			botUcmd->botCmds.exitVehicle = true;
		}
		return false;
	}

	if ( botThreadData.random.RandomInt( 100 ) > 98 || nbgExit == true ) {
		if ( botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].inuse != false && heardClient == -1 ) { //mal: if the first target action == false, they all must, so skip
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
			nbgExit = false;
		} else {
			if ( heardClient == -1 ) {
				idVec3 vec;
				if ( Bot_RandomLook( vec ) )  {
					Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
					nbgExit = false;
             	}
			} else {
                Bot_LookAtLocation( botWorld->clientInfo[ heardClient ].origin, SMOOTH_TURN ); //we hear someone - look at where they're at.
				nbgExit = true;
			}
		}
	}
	
	return true;
}

/*
================
idBotAI::Enter_NBG_FlyerHive

This is a nasty, horrible, special case hack to get Volcano working.
================
*/
bool idBotAI::Enter_NBG_FlyerHive() {
    
 	nbgType = FLYER_HIVE_ATTACK;

	NBG_AI_SUB_NODE = &idBotAI::NBG_FlyerHive;
	
	nbgTime = botWorld->gameLocalInfo.time + BOT_INFINITY;

	lastAINode = "Flyer Hive";

	botIdealWeapNum = FLYER_HIVE;

	nbgTimer = botWorld->gameLocalInfo.time + 1500;

	nbgTimer2 = 0;

	nbgExit = false;

	botUcmd->botCmds.dropDisguise = true;

	nbgReached = false;

	return true;	
}

/*
================
idBotAI::NBG_FlyerHive
================
*/
bool idBotAI::NBG_FlyerHive() {
	if ( !Bot_CheckActionIsValid( actionNum ) ) {
		Bot_ExitAINode();
		return false;
	}

	if ( !botThreadData.botActions[ actionNum ]->ActionIsActive() ) {
		Bot_ExitAINode();
		return false;
	}

	if ( nbgReached == false && botInfo->weapInfo.covertToolInfo.entNum == 0 ) {
		botIdealWeapNum = FLYER_HIVE;
		botIdealWeapSlot = NO_WEAPON;

		Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->actionTargets[ 0 ].origin, INSTANT_TURN );
		
		if ( botInfo->weapInfo.weapon == FLYER_HIVE && ClassWeaponCharged( FLYER_HIVE ) && botThreadData.random.RandomInt( 100 ) > 50 ) {
			botUcmd->botCmds.attack = true;
		}

		return true;
	}

	nbgReached = true;

	if ( nbgTimer < botWorld->gameLocalInfo.time && botInfo->weapInfo.covertToolInfo.entNum == 0 ) {
		Bot_ExitAINode();
		return false;
	}

	if ( nbgExit == false ) {
		int goalAreaNum = botThreadData.botActions[ actionNum ]->GetActionAreaNum();
		idVec3 goalOrigin = botThreadData.botActions[ actionNum ]->GetActionOrigin();

		idVec3 vec = goalOrigin - botInfo->weapInfo.covertToolInfo.origin;

		if ( vec.LengthSqr() > Square( botThreadData.botActions[ actionNum ]->radius ) ) {
			Bot_SetupFlyerMove( goalOrigin, goalAreaNum );

			idVec3 moveGoal = botAAS.path.moveGoal;
			moveGoal.z += 12.0f;

			if ( botAAS.path.type == PATHTYPE_JUMP || botAAS.path.type == PATHTYPE_BARRIERJUMP ) {
				moveGoal.z += 64.0f;
				vec = moveGoal - botInfo->weapInfo.covertToolInfo.origin;

				if ( vec.LengthSqr() > Square( 16.0f ) ) {
					Bot_MoveToGoal( moveGoal, vec3_zero, RUN, NULLMOVETYPE );
				} else {
					Bot_MoveToGoal( botAAS.path.reachability->GetEnd(), vec3_zero, RUN, NULLMOVETYPE );
				}					
			} else {
				Bot_MoveToGoal( moveGoal, vec3_zero, RUN, NULLMOVETYPE );
			}
			Flyer_LookAtLocation( moveGoal );
			return true;
		}

		nbgExit = true;
		return true;
	}

	if ( nbgTimer2 < MAX_LINKACTIONS && botThreadData.botActions[ actionNum ]->actionTargets[ nbgTimer2 ].inuse != false ) {
		idVec3 moveGoal = botThreadData.botActions[ actionNum ]->actionTargets[ nbgTimer2 ].origin;
		idVec3 vec = moveGoal - botInfo->weapInfo.covertToolInfo.origin;
		float height = vec.z;
		botMoveFlags_t botMoveFlags = RUN;

		if ( nbgTimer2 != 4 ) {
			if ( height > 10.0f ) {
				botMoveFlags = JUMP_MOVE;
			} else if ( height < -10.0f ) {
				botMoveFlags = CROUCH;
			}
		}
		
		if ( vec.LengthSqr() > Square( 25.0f ) ) {
			Bot_MoveToGoal( moveGoal, vec3_zero, botMoveFlags, NULLMOVETYPE );
			Flyer_LookAtLocation( moveGoal );
			return true;
		}

		nbgTimer2++;
		return true;
	}

	if ( !botThreadData.botActions[ actionNum ]->actionBBox.ContainsPoint( botInfo->weapInfo.covertToolInfo.origin ) ) {
		idVec3 moveGoal = botThreadData.botActions[ actionNum ]->actionBBox.GetCenter();
		idVec3 vec = moveGoal - botInfo->weapInfo.covertToolInfo.origin;
		botMoveFlags_t botMoveFlags = RUN;
		float height = vec.z;

		if ( height > 10.0f ) {
			botMoveFlags = JUMP_MOVE;
		} else if ( height < -10.0f ) {
			botMoveFlags = CROUCH;
		}

		Bot_MoveToGoal( moveGoal, vec3_zero, RUN, NULLMOVETYPE );
		Flyer_LookAtLocation( moveGoal );
	} else {
		botUcmd->botCmds.attack = true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_FixGun
================
*/
bool idBotAI::Enter_NBG_FixGun() {
    
 	nbgType = FIX_GUN;

	NBG_AI_SUB_NODE = &idBotAI::NBG_FixGun;
	
	nbgTime = botWorld->gameLocalInfo.time + 10000; //mal: 10 seconds to fix this gun.

	botIdealWeapSlot = GUN;

	lastAINode = "Fixing Gun";

	nbgPosture = botThreadData.botActions[ actionNum ]->posture;

	return true;	
}

/*
================
idBotAI::NBG_FixGun
================
*/
bool idBotAI::NBG_FixGun() {
	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreAction( actionNum, 15000 );						
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

	if ( botThreadData.botActions[ actionNum ]->GetActionState() == ACTION_STATE_GUN_READY ) {
		Bot_ExitAINode();
		return false;
	}

	float pliersDist = 500.0f;

	idBox playerBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );

	idVec3 vec = botThreadData.botActions[ actionNum ]->actionBBox.GetCenter() - botInfo->origin;
	float distSqr = vec.LengthSqr();

	if ( distSqr < Square( pliersDist ) ) {
		weaponLocked = true;
		botUcmd->botCmds.activate = true;
	}

//mal: move into the bbox for this action
	if ( !botThreadData.botActions[ actionNum ]->actionBBox.IntersectsBox( playerBox ) && botInfo->weapInfo.weapon != PLIERS || botInfo->onLadder ) {
		Bot_SetupQuickMove( botThreadData.botActions[ actionNum ]->actionBBox.GetCenter(), false );

		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}
		
		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, ( distSqr < Square( pliersDist ) ) ? WALK : RUN, NULLMOVETYPE ); //mal: assume the posture defined by the action...
		Bot_LookAtLocation( ( distSqr < Square( pliersDist ) ) ? botThreadData.botActions[ actionNum ]->actionBBox.GetCenter() : botAAS.path.viewGoal, SMOOTH_TURN );
		return true;
	}

	Bot_LookAtLocation( botThreadData.botActions[ actionNum ]->actionBBox.GetCenter(), SMOOTH_TURN );

	Bot_MoveToGoal( vec3_zero, vec3_zero, nbgPosture, NULLMOVETYPE ); //mal: assume the posture defined by the action...

	weaponLocked = true;

	botUcmd->botCmds.activate = true;

	if ( botInfo->weapInfo.weapon == PLIERS ) {
		botUcmd->botCmds.activateHeld = true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_Pause
================
*/
bool idBotAI::Enter_NBG_Pause() {
    
 	nbgType = PAUSE;

	NBG_AI_SUB_NODE = &idBotAI::NBG_Pause;
	
	nbgTime = botWorld->gameLocalInfo.time + 2000 + ( botThreadData.random.RandomInt( 5 ) * 1000 );

	botIdealWeapSlot = GUN;

	lastAINode = "Pausing";

	return true;	
}

/*
================
idBotAI::NBG_Pause
================
*/
bool idBotAI::NBG_Pause() {
	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	botUcmd->botCmds.reload = true;

	if ( botThreadData.random.RandomInt( 100 ) > 95 ) {
		bool randomLook = true;

		if ( nbgOrigin != vec3_zero && botThreadData.random.RandomInt( 100 ) > 50 ) {
			randomLook = false;
		}

		if ( randomLook ) {
			idVec3 vec;
			if ( Bot_RandomLook( vec ) ) {
				Bot_LookAtLocation( vec, SMOOTH_TURN ); //randomly look around, for enemies and whatnot.
			}
		} else {
			Bot_LookAtLocation( nbgOrigin, SMOOTH_TURN ); //look at a predetermined point in space ( could be an enemies first seen location, etc ).
		}
	}

	return true;
}

/*
================
idBotAI::Enter_NBG_EngAttackAVTNearMCP
================
*/
bool idBotAI::Enter_NBG_EngAttackAVTNearMCP() {
    
 	nbgType = ENG_ATTACK_AVT;

	NBG_AI_SUB_NODE = &idBotAI::NBG_EngAttackAVTNearMCP;
	
	nbgTime = botWorld->gameLocalInfo.time + 30000;

	lastAINode = "Eng Attack AVT";

	return true;	
}

/*
================
idBotAI::NBG_EngAttackAVTNearMCP
================
*/
bool idBotAI::NBG_EngAttackAVTNearMCP() {
	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_ExitAINode();
		return false;
	}

	deployableInfo_t deployableInfo;

	if ( !GetDeployableInfo( false, nbgTarget, deployableInfo ) ) { //mal: doesn't exist anymore.
		Bot_ExitAINode();
		return false;
	}

	if ( deployableInfo.disabled || deployableInfo.health < ( deployableInfo.maxHealth / DEPLOYABLE_DISABLED_PERCENT ) ) { //mal: its dead ( enough ).
		Bot_ExitAINode();
		return false;
	}

	if ( !Bot_HasExplosives( false ) ) {
		Bot_ExitAINode();
		return false;
	}

	idVec3 vec = deployableInfo.origin - botInfo->origin;
	float dist = vec.LengthSqr();

	idVec3 deployableOrigin = deployableInfo.origin;
	deployableOrigin.z += DEPLOYABLE_ORIGIN_OFFSET;

	trace_t tr;
	botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, deployableOrigin, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ) );

	bool isVisible = false;
	bool attackGoal = false;

	if ( tr.fraction == 1.0f || tr.c.entityNum == deployableInfo.entNum ) { 
		isVisible = true;
	}

	if ( isVisible ) {
		if ( dist < Square( ATTACK_APT_DIST ) ) {
			attackGoal = true;
		}
	}

	if ( !attackGoal ) {
		botUcmd->actionEntityNum = deployableInfo.entNum;
		botUcmd->actionEntitySpawnID = deployableInfo.spawnID;

		Bot_SetupMove( deployableOrigin, -1, ACTION_NULL );
		
		if ( MoveIsInvalid() ) {
			Bot_ExitAINode();
			return false;
		}

		Bot_MoveAlongPath( ( dist > Square( 100.0f ) ) ? SPRINT : RUN );
		return true;
	}

	botIdealWeapSlot = NADE;
	botIdealWeapNum = NULL_WEAP;
	Bot_ThrowGrenade( deployableOrigin, true );
	return true;
}

/*
================
idBotAI::Enter_NBG_ShieldTeammate
================
*/
bool idBotAI::Enter_NBG_ShieldTeammate() {
    
	nbgType = DROPPING_SHIELD;

	NBG_AI_SUB_NODE = &idBotAI::NBG_ShieldTeammate;
	
	nbgTime = botWorld->gameLocalInfo.time + 15000; //mal: 15 seconds to shield this guy!

	lastAINode = "Shield Mate";

	return true;
}

/*
================
idBotAI::NBG_ShieldTeammate
================
*/
bool idBotAI::NBG_ShieldTeammate() {
	if ( !ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc. 
        Bot_ExitAINode();
		return false;
	}

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ nbgTarget ];

	if ( playerInfo.inWater || botInfo->inWater || playerInfo.xySpeed > WALKING_SPEED ) {
		Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: client jumped into a vehicle/deployable - forget them
		Bot_ExitAINode();
		return false;
	}

	if ( ClientIsDead( nbgTarget ) || playerInfo.inLimbo ) {
		Bot_ExitAINode();
		return false;
	}

	if ( playerInfo.weapInfo.weapon != PLIERS && playerInfo.weapInfo.weapon != NEEDLE && playerInfo.weapInfo.weapon != HACK_TOOL && playerInfo.weapInfo.weapon != HE_CHARGE ) {
		Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	if ( !ClassWeaponCharged( SHIELD_GUN ) || ClientHasCloseShieldNearby( nbgTarget, SHIELD_CONSIDER_RANGE ) ) {
		Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	idVec3 vec = playerInfo.origin - botInfo->origin;
	float distSqr = vec.LengthSqr();

	botUcmd->actionEntityNum = nbgTarget; //mal: let the game and obstacle avoidance know we want to interact with this entity.
	botUcmd->actionEntitySpawnID = nbgTargetSpawnID;

	if ( distSqr > Square( SHIELD_FIRING_RANGE ) || botInfo->onLadder || !botThreadData.Nav_IsDirectPath( AAS_PLAYER, botInfo->team, botInfo->areaNum, botInfo->origin, playerInfo.origin ) ) {
		Bot_SetupMove( vec3_zero, nbgTarget, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ExitAINode();
			return false;
		}
		
		Bot_MoveAlongPath( ( distSqr > Square( 200.0f ) ) ? SPRINT : RUN );
		return true;
	}

	if ( distSqr < Square( 125.0f ) ) { // too close, back up some!
		if ( Bot_CanMove( BACK, 50.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		}
	}

	idVec3 targetLoc = playerInfo.viewOrigin;
	targetLoc.z += 32.0f;

	Bot_LookAtLocation( targetLoc, SMOOTH_TURN );

	botIdealWeapNum = SHIELD_GUN;
	botIdealWeapSlot = NO_WEAPON;

	if ( botInfo->weapInfo.weapon == SHIELD_GUN ) {
		botUcmd->botCmds.attack = true;
	}

	return true;	
}

/*
================
idBotAI::Enter_NBG_SmokeTeammate
================
*/
bool idBotAI::Enter_NBG_SmokeTeammate() {
    
	nbgType = DROPPING_SMOKE_FOR_MATE;

	NBG_AI_SUB_NODE = &idBotAI::NBG_SmokeTeammate;
	
	nbgTime = botWorld->gameLocalInfo.time + 15000; //mal: 15 seconds to smoke this guy!

	lastAINode = "Smoke Mate";

	return true;
}

/*
================
idBotAI::NBG_SmokeTeammate
================
*/
bool idBotAI::NBG_SmokeTeammate() {
	if ( !ClientIsValid( nbgTarget, nbgTargetSpawnID ) ) { //mal: client disconnected/got kicked/went spec, etc. 
        Bot_ExitAINode();
		return false;
	}

	if ( nbgTime < botWorld->gameLocalInfo.time ) { //mal: times up - leave!
		Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	const clientInfo_t& playerInfo = botWorld->clientInfo[ nbgTarget ];

	if ( playerInfo.inWater || playerInfo.xySpeed > WALKING_SPEED ) {
		Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: client jumped into a vehicle/deployable - forget them
		Bot_ExitAINode();
		return false;
	}

	if ( ClientIsDead( nbgTarget ) || playerInfo.inLimbo ) {
		Bot_ExitAINode();
		return false;
	}

	if ( playerInfo.weapInfo.weapon != PLIERS ) {
		Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	if ( !ClassWeaponCharged( SMOKE_NADE ) ) {
		Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME );
		Bot_ExitAINode();
		return false;
	}

	idVec3 vec = playerInfo.origin - botInfo->origin;
	float distSqr = vec.LengthSqr();

	botUcmd->actionEntityNum = nbgTarget; //mal: let the game and obstacle avoidance know we want to interact with this entity.
	botUcmd->actionEntitySpawnID = nbgTargetSpawnID;

	if ( distSqr > Square( 350.0f ) || botInfo->onLadder ) {
		Bot_SetupMove( vec3_zero, nbgTarget, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			Bot_IgnoreClient( nbgTarget, MEDIC_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ExitAINode();
			return false;
		}
		
		Bot_MoveAlongPath( ( distSqr > Square( 200.0f ) ) ? SPRINT : RUN );
		return true;
	}

	if ( distSqr < Square( 125.0f ) ) { // too close, back up some!
		if ( Bot_CanMove( BACK, 50.0f, true ) ) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		}
	}

	Bot_LookAtEntity( nbgTarget, SMOOTH_TURN );

	botIdealWeapNum = SMOKE_NADE;
	botIdealWeapSlot = NO_WEAPON;

	if ( botInfo->weapInfo.weapon == SMOKE_NADE && botThreadData.random.RandomInt( 100 ) > 50 ) {
		botUcmd->botCmds.attack = true;
	}

	return true;	
}