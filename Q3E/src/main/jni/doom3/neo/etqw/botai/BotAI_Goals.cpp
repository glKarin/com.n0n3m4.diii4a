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
idBotAI::Bot_FindLongTermGoal
================
*/
void idBotAI::Bot_FindLongTermGoal() {
	bool botHasObj = ClientHasObj( botNum );

	if ( Bot_CheckForDroppedObjGoals() ) { //mal: first, check for dropped objs, and what we should do about them.
		return;
	}

	if ( Bot_CheckMapScripts() ) { //mal: next, check for any map specific goals the bots should be mindful of.
		return;
	}

	if ( botWorld->botGoalInfo.mapHasMCPGoal && botInfo->team == GDF && !botHasObj ) {
		if ( Bot_CheckMCPGoals() ) {
			return;
		}
	}

	if ( botInfo->classType == ENGINEER && !botHasObj ) { //mal: check eng's goals
		if ( Bot_CheckForEngineerGoals() ) {
			return;
		}
	}

	if ( botInfo->classType == SOLDIER && !botHasObj ) { //mal: check soldier's goals
		if ( Bot_CheckForSoldierGoals() ) {
			return;
		}
	}

	if ( botInfo->classType == FIELDOPS && !botHasObj ) { //mal: check Fops's goals - only have 1 goal.
		if ( Bot_CheckForFieldOpsGoals() ) {
			return;
		}
	}

	if ( botInfo->classType == COVERTOPS && !botHasObj ) { //mal: check covert's goals
		if ( Bot_CheckForCovertGoals() ) {
			return;
		}
	}

	if ( !botHasObj ) { //mal: check for escort goals. Any class may call this
		if ( Bot_CheckForEscortGoals() ) {
			return;
		}
	}

	if ( Bot_CheckForTeamGoals() ) {
		return;
	} else {
		if ( botVehicleInfo != NULL ) {
			if ( Bot_CheckForVehicleFallBackGoals() ) {
				return;
			}
		} else {
			if ( Bot_CheckForFallBackGoals() ) {
				return;
			}
		}

		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;		//mal: this should never run anymore - unless there is no goals on the map for the bot to do.
		LTG_AI_SUB_NODE = &idBotAI::LTG_ErrorThink;
	}
}

/*
================
idBotAI::Bot_Check_NBG_Goals

A team neutral short term goal check.
================
*/
bool idBotAI::Bot_Check_NBG_Goals() {

	bool hasGoal = false; //mal: start having no goal

	hasGoal = Bot_CheckSelfState();

	if ( hasGoal ) {
		return true;
	}

	hasGoal = Bot_IsNearDroppedObj();

	if ( hasGoal ) {
		return true;
	}

	hasGoal = Bot_IsNearTeamGoalUnderAttack();

	if ( hasGoal ) {
		return true;		
	}

	if ( aiState == LTG ) {
		if ( LTG_CHECK_FOR_CLASS_NBG != NULL ) {
            hasGoal = CallFuncPtr( LTG_CHECK_FOR_CLASS_NBG );	//mal: check for any valid short term goals for this bot.	
		}

		if ( !hasGoal ) {
			hasGoal = Bot_CheckForHumanWantingEscort();
		}
	} else {
		if ( NBG_CHECK_FOR_CLASS_NBG != NULL ) {
            hasGoal = CallFuncPtr( NBG_CHECK_FOR_CLASS_NBG ); //mal: check for any valid short term goals for this bot.
		}

		if ( !hasGoal ) {
			Bot_CheckForHumanWantingEscort();
		}

		hasGoal = true; //mal: dont want to check for spawnhosts, or any other NBG, if we're in the process of one.
	}

	if ( hasGoal ) {
		return true;
	}

	hasGoal = Bot_IsNearForwardSpawnToGrab();

	if ( hasGoal ) {
		return true;
	}

	if ( botInfo->team == STROGG ) {
		hasGoal = Bot_CheckForSpawnHostToGrab();
	}

	if ( hasGoal ) {
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_CheckSelfState

A bot self check, where they decide if they should request a medic, or Fops or look for one to
bugger for supplies, or if a high ranking strogg - balance their stroyent.
================
*/
bool idBotAI::Bot_CheckSelfState() {

	if ( aiState == NBG && ( nbgType != CAMP ) ) {
		return false;
	}

	if ( Client_IsCriticalForCurrentObj( botNum, 1500.0f ) || ( ClientHasObj( botNum ) && ClientIsCloseToDeliverObj( botNum, 1500.0f ) ) ) { //mal: dont bother with chasing medics if really close to their primary obj - just do it!
		return false;
	}

	if ( botInfo->health < botInfo->maxHealth ) {
		if ( Bot_FindCloseSupplyPack( true, false ) ) {
			return true;
		}

		if ( Bot_IsInvestigatingTeamObj() || Bot_IsDefendingTeamCharge() ) {
			return false;
		}

		if ( botInfo->classType != MEDIC ) {
            if ( Bot_FindCloseSupplyTeammate( true ) ) { //mal: see if we can bugger someone for supplies!
				return true;
			}
		}
	}

	if ( Bot_IsInvestigatingTeamObj() || Bot_IsDefendingTeamCharge() ) {
		return false;
	}

	if ( botInfo->weapInfo.primaryWeapNeedsAmmo == true || ( botInfo->weapInfo.hasNadeAmmo == false && botInfo->isDisguised == false ) ) {
		bool nadesOnly = ( botInfo->weapInfo.primaryWeapNeedsAmmo == false ) ? true : false;
		if ( Bot_FindCloseSupplyPack( false, nadesOnly ) ) {
			return true;
		}

		if ( ( botInfo->team == GDF && botInfo->classType != FIELDOPS ) || ( botInfo->team == STROGG && botInfo->classType != MEDIC ) && botInfo->weapInfo.primaryWeapNeedsAmmo != false ) { //mal: wont bugger for nade ammo
			if ( Bot_FindCloseSupplyTeammate( false ) ) { //mal: see if we can bugger someone for supplies!
				return true;
			}
		}
	}

	return false;

	// also - if strogg, if have low health, but lots of ammo (or vice versa) balance stroylent to compensate as a short term fix!
}

/*
================
idBotAI::Bot_CheckClassState

A class specific check for goals that the bot can do while on the move to a LTG, goals that
dont require a AI Node change (medic healing self, covert throwing smoke, etc). Medics are the only
class that will call this during a NBG.
================
*/
void idBotAI::Bot_CheckClassState() {
	Bot_CheckMountedGPMGState();

	if ( botInfo->classType == MEDIC || ( botInfo->classType == FIELDOPS && botInfo->team == GDF ) ) {
		if ( Bot_CheckSelfSupply() ) {
            return;	// exit out of here if bot is busy healing self.
		}
	}

	if ( botInfo->classType == MEDIC && aiState == LTG ) {
		Bot_CheckAdrenaline( vec3_zero );
	}

	if ( aiState == NBG && botInfo->classType == COVERTOPS ) {
		if ( nbgType == DEFENSE_CAMP || nbgType == CAMP || nbgType == SNIPE || nbgType == GRAB_SUPPLIES || nbgType == BUG_FOR_SUPPLIES || nbgType == AVOID_DANGER || nbgType == INVESTIGATE_CAMP ) {
			Bot_CheckCovertToolState();
		}
	}

	if ( aiState != LTG ) { 
		return;
	}

	if ( botInfo->isActor ) {
		return;
	}

	if ( Bot_CheckForTacticalAction() != -1 ) {
		Bot_RunTacticalAction();
	} else if ( botInfo->classType == COVERTOPS ) {
		Bot_CheckCovertToolState();
	} else if ( botInfo->classType == FIELDOPS && botInfo->team == STROGG ) {
		idVec3 turret_location;
		if ( Bot_IsUnderAttackByAPT( turret_location ) && ClassWeaponCharged( SHIELD_GUN ) ) {
			Bot_LookAtLocation( turret_location, FAST_TURN );

			botIdealWeapNum = SHIELD_GUN;
			botIdealWeapSlot = NO_WEAPON;
			if ( botInfo->weapInfo.weapon == SHIELD_GUN ) {
				botUcmd->botCmds.attack = true;
			}
		}
	} else if ( botInfo->classType == MEDIC && botInfo->team == GDF ) {
		Bot_CheckHealthCrateState();
	}

//mal: GDF FOps will give ammo in the spawn. Strogg dont need this because they start out with SO much ammo. NOT needed anymore as now GDF start with max ammo.
/*
	if ( botInfo->spawnTime + SPAWN_CONSIDER_TIME > botWorld->gameLocalInfo.time && !botInfo->revived && ClassWeaponCharged( AMMO_PACK ) ) {
        if ( botInfo->classType == FIELDOPS && botInfo->team == GDF ) {
			if ( botInfo->xySpeed > WALKING_SPEED ) {

				if ( botInfo->backPedalTime > botWorld->gameLocalInfo.time ) {
					botUcmd->viewType = VIEW_REVERSE;
				} else {
					botUcmd->botCmds.lookUp = true;
				}

                botIdealWeapNum = AMMO_PACK;
				botIdealWeapSlot = NO_WEAPON;
				botUcmd->moveFlag = RUN;  

				if ( botInfo->weapInfo.weapon == AMMO_PACK ) {
                    botUcmd->botCmds.attack = true;
				}
			}
		}
	}
*/

	if ( botInfo->team == GDF && botInfo->classType == FIELDOPS && botInfo->spawnTime + SPAWN_CONSIDER_TIME > botWorld->gameLocalInfo.time && botInfo->revived && ClassWeaponCharged( AMMO_PACK ) && ClientIsValid( botInfo->mySavior, -1 ) && enemy == -1 && !ClientIsIgnored( botInfo->mySavior ) ) {
		nbgTarget = botInfo->mySavior;
		nbgTargetType = REARM_REVIVE;
		nbgTargetSpawnID = botWorld->clientInfo[ botInfo->mySavior ].spawnID;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		nbgType = SUPPLY_TEAMMATE;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_SupplyTeammate;
	} //mal: be a good teammate and supply some ammo to the guy who revived us, as long as we're not in combat!
}

/*
================
idBotAI::Bot_CovertOpsCheckNBGState

A covert specific short term goal check. Works for both teams.
Only run when bot is in active pursuit of a long term goal
================
*/

bool idBotAI::Bot_CovertOpsCheckNBGState() {

//mal: first, setup some defaults.
	int clientNum;
	float range = MEDIC_RANGE;

	if ( aiState == NBG && ( nbgType != CAMP && nbgType != DEFENSE_CAMP ) ) {			//mal: when doing short term goals, only grab uniforms if camping.
		return false;
	}

	if ( botInfo->isActor ) {
		return false;
	}

	//mal: add any exceptions to those defaults here
	if ( aiState == LTG && ( ltgType == FOLLOW_TEAMMATE || ltgType == FOLLOW_TEAMMATE_BY_REQUEST ) ) { //mal_NOTE: coverts will only follow a teammate if they're not disguised.
		range = MEDIC_RANGE_BUSY;
	}

	if ( Client_IsCriticalForCurrentObj( botNum, BODY_IGNORE_RANGE ) ) { //mal: if bot is close to its critical obj, forget about grabbing uniforms!
        return false;
	}
	
	if ( ClientHasObj( botNum ) && ClientIsCloseToDeliverObj( botNum ) ) {
		return false;
	}

	clientNum = Bot_CovertCheckForUniforms( range );

	if ( clientNum != -1 ) {
		nbgTarget = clientNum;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_StealUniform;
		return true;
	}

	if ( Client_IsCriticalForCurrentObj( botNum, -1.0f ) ) { //mal: if bot has a critical obj, leave!
        return false;
	}

	if ( aiState == LTG && ( ltgType == HACK_GOAL || ltgType == FOLLOW_TEAMMATE || ltgType == FOLLOW_TEAMMATE_BY_REQUEST ) ) { //mal_TODO: add other exceptions here
		return false;
	}

	clientNum = Bot_CovertCheckForVictims( range * 2.0f ); 

	if ( clientNum != -1 ) {
		nbgTarget = clientNum;
		nbgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_HuntVictim;
		return true;
	}

	clientNum = Bot_CheckForNearbyTeammateWhoCouldUseSmoke();

	if ( clientNum != -1 ) {
		nbgTarget = clientNum;
		nbgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_SmokeTeammate;
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_MedicCheckNBGState_InLTG

A medic specific short term goal check. Works for both teams.
Only run when bot is in active pursuit of a long term goal
================
*/

bool idBotAI::Bot_MedicCheckNBGState_InLTG () {
	int clientNum = -1;
	int escortClientNum = -1;
	int healthRange = 150; //mal: will heal anyone not at full health
	float range = MEDIC_RANGE;

	if ( ltgType == FOLLOW_TEAMMATE || ltgType == FOLLOW_TEAMMATE_BY_REQUEST || botInfo->isActor ) {
		escortClientNum = ltgTarget;
		range = MEDIC_RANGE_BUSY;
		healthRange = 80; //mal: if already busy, less inclined to heal others
	}

	if ( ClientHasObj( botNum ) ) {
		if ( ClientIsCloseToDeliverObj( botNum ) ) {
			return false;
		}

		if ( botInfo->team == GDF ) {
			range = MEDIC_RANGE_CARRIER;
		} else {
			range = 0.0f;
		}
	}

	clientNum = Bot_MedicCheckForDeadMate( -1, escortClientNum, range );

	if ( clientNum != -1 ) {
		nbgTarget = clientNum;
		nbgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
//mal: bit of a hack, but a self correcting one if the bot isn't able to revive.
		aiState = NBG;
		nbgType = REVIVE_TEAMMATE;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_ReviveTeammate;

		if ( Bot_IsInvestigatingTeamObj() ) {
			lastCheckActionTime = 0;
		}

		return true;
	}

	if ( Bot_IsInvestigatingTeamObj() ) {
		return false;
	}

	if ( ClientHasObj( botNum ) ) {
		range = MEDIC_RANGE_CARRIER;
		healthRange = 50;
	}

	clientNum = Bot_MedicCheckForWoundedMate( -1, escortClientNum, range, healthRange );

	if ( clientNum != -1 ) {
		if ( botWorld->clientInfo[ clientNum ].lastChatTime[ HEAL_ME ] + 5000 > botWorld->gameLocalInfo.time ) {
			nbgTargetType = HEAL_REQUESTED;
		} else {
			nbgTargetType = HEAL;
		}

//mal: bit of a hack, but a self correcting one if the bot isn't able to heal.
		aiState = NBG;
		nbgTarget = clientNum;
		nbgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;

		if ( ClientCanBeTKRevived( clientNum ) ) {
			nbgType = TK_REVIVE_TEAMMATE;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_TKReviveTeammate;
		} else {
            nbgType = SUPPLY_TEAMMATE;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_SupplyTeammate;
		}

		return true;
	}

	if ( ClientHasObj( botNum ) ) {
		return false;
	}

	if ( botInfo->isActor ) {
		return false;
	}

	if ( botInfo->team == STROGG ) { //mal: the sinister strogg will look for helpless GDF to turn into unwilling hosts < cue evile laughter >
		
		clientNum = Bot_CheckForNeedyTeammates( range );

		if ( clientNum != -1 ) {
			nbgTarget = clientNum;
			nbgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
			nbgTargetType = REARM;
			ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_SupplyTeammate;
			return true;
		}

		clientNum = Bot_StroggCheckForGDFBodies( range ); //mal_TODO: is this the best range?

		if ( clientNum != -1 ) {
			nbgTarget = clientNum;
			ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_CreateSpawnHost;
			return true;
		}
	}

	if ( botInfo->team == GDF ) {

		clientNum = Bot_CheckForSpawnHostsToDestroy( range, nbgChat );

		if ( clientNum != -1 ) {
			nbgTarget = clientNum;
			nbgTargetType = BODY; 
			ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_DestroySpawnHost;
			return true;
		}
	}			

	return false;
}

/*
================
idBotAI::Bot_MedicCheckNBGState_InNBG

A medic specific short term goal check. Works for both teams.
Only run when bot is in active pursuit of a short term goal
================
*/
bool idBotAI::Bot_MedicCheckNBGState_InNBG () {
	if ( ClientHasObj( botNum ) ) { //mal: no multi-tasking when we're the carrier!
		return false;
	}

	if ( botInfo->isActor ) {
		return false;
	}

//mal: first, setup some defaults.
	bool isBusy = ( nbgType == REVIVE_TEAMMATE || nbgType == TK_REVIVE_TEAMMATE ); 
	int clientNum;
	int targetClientNum = -1;
	int healthRange = 150; //mal: will heal anyone not at full health
	float range = MEDIC_RANGE;

//mal: add any exceptions to those defaults here
	if ( isBusy || nbgType == SUPPLY_TEAMMATE ) {
		
		if ( Client_IsCriticalForCurrentObj( nbgTarget, -1.0f ) && nbgType != SUPPLY_TEAMMATE ) { //mal: sorry guys - critical teammates get priority!
			return false;
		}
		
		range = MEDIC_RANGE_BUSY;
		targetClientNum = nbgTarget;
		healthRange = 60; //mal: if already busy, less inclined to heal others
	}

	clientNum = Bot_MedicCheckForDeadMate( targetClientNum, -1, range );

	if ( clientNum != targetClientNum && clientNum != -1 ) {
		nbgTarget = clientNum;
		nbgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_ReviveTeammate;
		return true;
	}

	if ( isBusy || ( Client_IsCriticalForCurrentObj( nbgTarget, -1.0f ) && nbgType == SUPPLY_TEAMMATE ) ) { //mal: won't bother healing ppl if we're on our way to revive someone!
		return false;
	}

	clientNum = Bot_MedicCheckForWoundedMate( targetClientNum, -1, range, healthRange );

	if ( clientNum != targetClientNum && clientNum != -1 ) {

		nbgTarget = clientNum;
		nbgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		nbgTargetType = HEAL;

		if ( ClientCanBeTKRevived( clientNum ) ) {
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_TKReviveTeammate;
		} else {
 			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_SupplyTeammate;
		}

		return true;
	}

	if ( ( nbgType == CAMP || nbgType == DEFENSE_CAMP ) && botInfo->team == STROGG ) { //mal: if we're just standing around, camping, look to resupply needy teammates.

        clientNum = Bot_CheckForNeedyTeammates( range );

		if ( clientNum != -1 ) {
			nbgTarget = clientNum;
			nbgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
			nbgTargetType = REARM;
			ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_SupplyTeammate;
			return true;
		}

		clientNum = Bot_StroggCheckForGDFBodies( range ); //mal_TODO: is this the best range?

		if ( clientNum != -1 ) {
			nbgTarget = clientNum;
			ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_CreateSpawnHost;
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::Bot_MedicCheckNBGState_InCombat
================
*/
bool idBotAI::Bot_MedicCheckNBGState_InCombat() {
	if ( botInfo->classType != MEDIC ) {
		return false;
	}

#ifndef STROGG_INSTANT_REVIVE
	if ( botInfo->team == STROGG ) {
		return false;
	}
#endif

	if ( combatNBGType == COMBAT_REVIVE_MATE ) { //mal: dont bother with this if already doing it!
		return false;
	}

	float reviveRange = ( botWorld->gameLocalInfo.gameIsBotMatch ) ? 1500.0f : 900.0f;

	int clientNum = Bot_MedicCheckForDeadMateDuringCombat( botWorld->clientInfo[ enemy ].origin, reviveRange );

	if ( clientNum != -1 ) {
		COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_ReviveTeammate;
		combatNBGTarget = clientNum;
		combatNBGType = COMBAT_REVIVE_MATE;
		combatNBGTime = botWorld->gameLocalInfo.time + 15000; //mal: 15 seconds to make this happen!
		combatNBGReached = false;
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_FopsCheckNBGState

A GDF Fops specific short term goal check.
Only run when bot is in active pursuit of a long term goal, unless hes camping.
================
*/
bool idBotAI::Bot_FopsCheckNBGState() {

	int clientNum = -1;

	if ( botInfo->team == GDF ) {
		float range = MEDIC_RANGE; ///mal: first, setup some defaults.

		//mal: add any exceptions to those defaults here
		if ( aiState == LTG && ( ltgType == FOLLOW_TEAMMATE || ltgType == FOLLOW_TEAMMATE_BY_REQUEST ) ) {
			range = MEDIC_RANGE_BUSY;
		}

		if ( aiState == NBG && ( nbgType != CAMP && nbgType != DEFENSE_CAMP ) ) {
			return false;
		}

		if ( ClientHasObj( botNum ) && ClientIsCloseToDeliverObj( botNum ) ) {
			return false;
		}

		clientNum = Bot_CheckForNeedyTeammates( range );

		if ( clientNum != -1 ) {
			if ( botWorld->clientInfo[ clientNum ].lastChatTime[ REARM_ME ] + 5000 > botWorld->gameLocalInfo.time ) {
				nbgTargetType = REARM_REQUESTED;
			} else {
				nbgTargetType = REARM;
			}

//mal: bit of a hack, but a self correcting one if the bot isn't able to supply.
			aiState = NBG;
			nbgType = SUPPLY_TEAMMATE;

			nbgTarget = clientNum;
			nbgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
			ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_SupplyTeammate;
			return true;
		}
	} else {
		if ( aiState == LTG && ( ltgType == FOLLOW_TEAMMATE || ltgType == FOLLOW_TEAMMATE_BY_REQUEST ) ) {
			return false;
		}

		if ( aiState == NBG && ( nbgType != CAMP && nbgType != DEFENSE_CAMP ) ) {
			return false;
		}

		if ( ClientHasObj( botNum ) && ClientIsCloseToDeliverObj( botNum ) ) {
			return false;
		}

		if ( botInfo->isActor ) {
			return false;
		}

		clientNum = Bot_HasTeammateWhoCouldUseShieldCoverNearby();

		if ( clientNum != -1 ) {
			nbgTarget = clientNum;
			nbgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
			ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
			NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_ShieldTeammate;
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::Bot_EngCheckNBGState

An Engineer specific short term goal check.
Only run when bot is in active pursuit of a long term goal, unless hes camping.
Here, the bot will decide to repair vehicles and deployables.
================
*/
bool idBotAI::Bot_EngCheckNBGState() {

//mal: first, setup some defaults.
	int proxyNum = -1;
	float range = MEDIC_RANGE_REQUEST;

//mal: add any exceptions to those defaults here
	if ( aiState == LTG && ( ltgType == FOLLOW_TEAMMATE || ltgType == FOLLOW_TEAMMATE_BY_REQUEST ) ) {
		range = MEDIC_RANGE_BUSY;
	}

	if ( ClientHasObj( botNum ) ) {
		return false;
	}

	if ( botInfo->isActor ) {
		return false;
	}

	if ( aiState == NBG && ( nbgType != CAMP && nbgType != DEFENSE_CAMP ) ) {
		return false;
	}

	if ( aiState == LTG && ltgType == DEFUSE_GOAL ) { //mal: defusing bombs has to take priority!
		return false;
	}

	bool chatRequested = false;

	proxyNum = Bot_CheckForNeedyVehicles( range, chatRequested );

	if ( proxyNum != -1 ) {
		nbgChat = false;

		if ( chatRequested ) {
			nbgChat = true;
		}

		nbgTarget = proxyNum;
		nbgTargetType = VEHICLE;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_FixProxyEntity;
		return true;
	}

	proxyNum = Bot_CheckForNeedyDeployables( range );

	if ( proxyNum != -1 ) {
		if ( DeployableIsMarkedForRepair( proxyNum, true ) ) {
			Bot_AddDelayedChat( botNum, ACKNOWLEDGE_YES, 2 );
		}

		nbgTarget = proxyNum;
		nbgTargetType = DEPLOYABLE;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_FixDeployable;
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_CheckForNearbyVehicleToGrab
================
*/
void idBotAI::Bot_CheckForNearbyVehicleToGrab() {

	int vehicleNum;

	if ( combatNBGType == COMBAT_GRAB_VEHICLE ) { //mal: dont bother with this if already doing it!
		return;
	}

	if ( !botWorld->gameLocalInfo.botsUseVehicles ) {
		return;
	}
	
	vehicleNum = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, GROUND | AIR, PERSONAL, true );

	if ( vehicleNum != -1 ) {
		int travelTime;
		proxyInfo_t vehicle;
		GetVehicleInfo( vehicleNum, vehicle );

		if ( Bot_LocationIsReachable( false, vehicle.origin, travelTime ) && travelTime < ( Bot_ApproxTravelTimeToLocation( botInfo->origin, vehicle.origin, false ) * TRAVEL_TIME_MULTIPLY ) ) {
			COMBAT_AI_SUB_NODE = &idBotAI::Enter_COMBAT_Foot_GrabVehicle;
			combatNBGTarget = vehicleNum;
			combatNBGType = COMBAT_GRAB_VEHICLE;
			combatNBGTime = botWorld->gameLocalInfo.time + 15000; //mal: 15 seconds to make this happen!
		}
	}
}

/*
================
idBotAI::Bot_CheckMapScripts

This function can do map specific overrides, if needed, or if debugging.
================
*/
bool idBotAI::Bot_CheckMapScripts() {
	if ( botWorld->gameLocalInfo.botFollowPlayer > 0 ) {
			ltgTarget = 0; //mal: client 0 - since this is just a test, I can be evile like this!
			ltgType = FOLLOW_TEAMMATE;
			ltgTargetSpawnID = botWorld->clientInfo[ 0 ].spawnID;
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_FollowMate;
			return true;
		}

	if ( botThreadData.AllowDebugData() ) {
		if ( bot_debugActionGoalNumber.GetInteger() > 0 && bot_debugActionGoalNumber.GetInteger() < botThreadData.botActions.Num() ) {
			actionNum = bot_debugActionGoalNumber.GetInteger();
			ltgUseVehicle = true;
			ltgType = CAMP_GOAL;
  			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			ltgTime = botWorld->gameLocalInfo.time + 30000;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_CampGoal;
			return true;
		}
	}

	if ( botWorld->gameLocalInfo.inWarmup ) {
		return false;
	}

	if ( botWorld->gameLocalInfo.gameMap == SLIPGATE ) {
		if ( botVehicleInfo != NULL && botVehicleInfo->isAirborneVehicle && botVehicleInfo->origin.x < SLIPGATE_DIVIDING_PLANE_X_VALUE ) { //mal: if in air vehicle, on desert side of the map, check where the MCP is.
			proxyInfo_t mcpInfo;
			GetVehicleInfo( botWorld->botGoalInfo.botGoal_MCP_VehicleNum, mcpInfo );

			if ( mcpInfo.entNum == 0 ) {
				return false;
			}

			if ( mcpInfo.origin.x < SLIPGATE_DIVIDING_PLANE_X_VALUE ) {
				return false;
			}

			Bot_ExitVehicle();

//			vLTGOrigin = SLIPGATE_ORIGIN;
//			V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
//			V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_TravelToGoalOrigin; //mal: just no time to work on this at the 11th hour.
		}

//		return true;

	}

	if ( botInfo->isActor ) {
		if ( botThreadData.actorMissionInfo.targetClientNum != -1 && botThreadData.actorMissionInfo.actionNumber != ACTION_NULL ) {
			actionNum = botThreadData.actorMissionInfo.actionNumber;
			ltgTarget = botThreadData.actorMissionInfo.targetClientNum;
			ltgTargetSpawnID = botWorld->clientInfo[ botThreadData.actorMissionInfo.targetClientNum ].spawnID;

			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_ActorEscortPlayerToGoal;
			return true;
		}

		if ( botThreadData.actorMissionInfo.targetClientNum != -1 && botThreadData.actorMissionInfo.playerIsOnFinalMission && botThreadData.actorMissionInfo.playerNeedsFinalBriefing ) {
			ltgTarget = botThreadData.actorMissionInfo.targetClientNum;
			ltgTargetSpawnID = botWorld->clientInfo[ botThreadData.actorMissionInfo.targetClientNum ].spawnID;

			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_ActorGiveFinalBriefingToPlayer;
			return true;
		}
	}

	return false;
}

/*
================
idBotAI::ClientHasObj
================
*/
bool idBotAI::ClientHasObj( int clientNum ) {
	bool hasObj = false;
	int i;

	for( i = 0; i < MAX_CARRYABLES; i++ ) {
		if ( botWorld->botGoalInfo.carryableObjs[ i ].entNum == 0 ) {
			continue;
		}

		if ( botWorld->botGoalInfo.carryableObjs[ i ].carrierEntNum != clientNum ) {
			continue;
		}

		hasObj = true;
		break;
	}

	return hasObj;
}

/*
================
idBotAI::Bot_CheckForTeamGoals
================
*/
bool idBotAI::Bot_CheckForTeamGoals() {
	ltgUseVehicle = false;
	bool mcpGoal = false;
	int botDeployableTarget = -1;
	int	botCamperTarget = -1;
	int i, j, matesInArea, botClass, validClasses;
	int spawnGoal = -1;				//mal: grabbing the spawn point. Theres only 1 spawn goal at a time to worry about.
	int stealSpawnGoal = -1;
	int deliverGoal = -1;
	int mcpActionNum = ACTION_NULL;
//	float dist;
//	float closest = idMath::INFINITY;
	proxyInfo_t mcpVehicleInfo;
	playerTeamTypes_t teamFilter = botInfo->team;
	playerTeamTypes_t covertTeamFilter = botInfo->team;
	idVec3 vec;
	idList< int > campGoals;
	idList< int > priorityCampGoals;
	idList< int > gunCampGoals;
	idList< int > roamGoals;
	idList< int > vRoamGoals;
	idList< int > vCampGoals;
	idList< int > investigateGoals; //mal: these are goals that have the bot investigate their current obj ( ex: if you plant, a non-eng may decide to "investigate" the plant site ).
	idList< int > stealGoals;
	idList< int > vehicleGrabGoals;

	if ( botInfo->classType == SOLDIER ) {
		botClass = 1;
	} else if ( botInfo->classType == MEDIC ) {
		botClass = 2;
	} else if ( botInfo->classType == ENGINEER ) {
		botClass = 4;
	} else if ( botInfo->classType == FIELDOPS ) {
		botClass = 8;
	} else {
		botClass = 16; // COVERT OPS
	}

	if ( botInfo->isDisguised ) { //mal: if the bots disguised, will do the enemys' camp/roam goals.
		covertTeamFilter = ( botInfo->team == GDF ) ? STROGG : GDF;
	}

	if ( botWorld->botGoalInfo.mapHasMCPGoal && botWorld->gameLocalInfo.heroMode == false ) {
		GetVehicleInfo( botWorld->botGoalInfo.botGoal_MCP_VehicleNum, mcpVehicleInfo );
		if ( mcpVehicleInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			if ( !mcpVehicleInfo.isImmobilized && ( mcpVehicleInfo.driverEntNum == -1 || mcpVehicleInfo.driverEntNum == botNum || botInfo->proxyInfo.entNum == mcpVehicleInfo.entNum ) ) {
				if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
					if ( botWorld->gameLocalInfo.botsDoObjsInTrainingMode && ( !TeamHasHuman( botInfo->team ) || ( botWorld->botGoalInfo.mapHasMCPGoalTime + ( botWorld->gameLocalInfo.botTrainingModeObjDelayTime * 1000 ) ) < botWorld->gameLocalInfo.time ) ) {
						mcpGoal = true;
					}
				} else {
					if ( !Bot_IsInHeavyAttackVehicle() ) {
					mcpGoal = true;
				}
			}
		}
	}
	}

	for( i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_STEAL ) {
			if ( ClientHasObj( botNum ) ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: the human has decided to complete the maps goals.
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.botsIgnoreGoals ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetActionObjState() == false ) { //mal: obj is already stolen!
				continue;
			}

			if ( botWorld->gameLocalInfo.inWarmup ) {
				continue;
			}

			stealGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_DELIVER ) { //mal: only 1 deliver point on a map at a time.
			if ( !ClientHasObj( botNum ) ) {
				if ( botThreadData.random.RandomInt( 100 ) < 25 ) {
					continue;
				}

				if ( reachedPatrolPointTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: did this recently, so do something else for a while.
					continue;
				}

				if ( ActionIsIgnored( i ) ) {
					continue;
				}
				
				if ( !Bot_LTGIsAvailable( -1, i, PATROL_DELIVER_GOAL, MAX_PATROL_DELIVER_CLIENTS ) ) { //mal: if just patrolling around the obj, don't need too many there.
					continue;
				}

				if ( botInfo->isDisguised ) {
					continue;
				}

				if ( Bot_IsInHeavyAttackVehicle() ) {
					continue;
				}

				if ( Bot_WantsVehicle() ) {
					continue;
				}

				idVec3 vec = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;

				if ( vec.LengthSqr() > Square( 3500.0f ) ) {
					continue;
				}

				matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), PATROL_DELIVER_TEST_DIST, botInfo->team, NOCLASS, false, false, false, true, true );

				if ( matesInArea > MAX_PATROL_DELIVER_CLIENTS ) {
				continue;
				} //mal: already somebody there, so lets pick a different one to camp.
			}

			if ( botWorld->gameLocalInfo.inWarmup ) {
				continue;
			}

			deliverGoal = i;
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_MCP_OUTPOST ) {

			if ( botInfo->team == STROGG ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( !mcpGoal ) {
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.inWarmup ) {
				continue;
			}

			if ( botWorld->botGoalInfo.botGoal_MCP_VehicleNum == -1 ) {
				continue;
			}

			if ( !botWorld->botGoalInfo.mapHasMCPGoal ) {
				continue;
			}

			proxyInfo_t mcp;

			GetVehicleInfo( botWorld->botGoalInfo.botGoal_MCP_VehicleNum, mcp );

			if ( mcp.entNum == 0 ) {
				continue;
			}

			if ( mcp.isDeployed ) {
				continue;
			}

			mcpActionNum = i;
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( covertTeamFilter ) == ACTION_CAMP || botThreadData.botActions[ i ]->GetObjForTeam( covertTeamFilter ) == ACTION_DEFENSE_CAMP ) {

			bool isPriority = ( botThreadData.botActions[ i ]->GetObjForTeam( covertTeamFilter ) == ACTION_DEFENSE_CAMP ) ? true : false;

			if ( i == lastActionNum ) { //mal: don't repeat ourselves
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( !botInfo->weapInfo.primaryWeapHasAmmo ) { //mal: no ammo - dont camp.
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( !Bot_MeetsVehicleRequirementsForAction( i ) ) {
				continue;
			}

			if ( !botInfo->isDisguised ) {
				validClasses = botThreadData.botActions[ i ]->GetValidClasses();

				if ( validClasses != 0 ) {
					if ( !( validClasses & botClass ) ) { //mal: give the LD's the ability to limit a camp position to a certain class.
						continue;
					}
				}
			}

			if ( botThreadData.botActions[ i ]->GetActionWeapType() != -1 ) {
				if ( botInfo->weapInfo.primaryWeapon != botThreadData.botActions[ i ]->GetActionWeapType() ) { //mal: give LD's ability to limit this camp to a certain weapon.
					continue;
				}
			}

			if ( isPriority ) {
				if ( !Bot_LTGIsAvailable( -1, i, DEFENSE_CAMP_GOAL, 1 ) ) {
					continue;
				}
			} else {
				if ( !Bot_LTGIsAvailable( -1, i, CAMP_GOAL, 1 ) ) {
					continue;
				} //mal: some bot is already on the way to camp there - no need for us to do so as well.
			}

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), ( isPriority ) ? DEFENSE_CAMP_MATE_RANGE : 150.0f, botInfo->team, NOCLASS, false, false, false, true, true );

			if ( matesInArea > 0 ) {
				continue;
			} //mal: already somebody there, so lets pick a different one to camp.

			if ( botInfo->isDisguised ) {
				if ( !botThreadData.botActions[ i ]->disguiseSafe ) { //mal: not a good one for a disguised covert.
					continue;
				}

				int enemiesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 300.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, true, true );
		
				if ( enemiesInArea > 0 ) {
					continue;
				} //mal: if bot is disguised, don't camp if lots of the enemy are around - that raises the chances they'll spot us.
			} 

			if ( isPriority ) {
				if ( botWorld->gameLocalInfo.botsIgnoreGoals ) {
					continue;
				}
				priorityCampGoals.Append( i );
			} else {
				campGoals.Append( i );
			}

			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( covertTeamFilter ) == ACTION_ROAM ) {

			if ( i == lastActionNum ) { //mal: don't repeat ourselves
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( !Bot_MeetsVehicleRequirementsForAction( i ) ) {
				continue;
			}

			if ( botInfo->isDisguised ) {
				if ( !botThreadData.botActions[ i ]->disguiseSafe ) { //mal: not a good one for a disguised covert.
					continue;
				}
			} else {
				validClasses = botThreadData.botActions[ i ]->GetValidClasses();

				if ( validClasses != 0 ) {
					if ( !( validClasses & botClass ) ) { //mal: give the LD's the ability to limit a roam position to a certain class.
						continue;
					}
				}
			}

			roamGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_MG_NEST || botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_MG_NEST_BUILD ) {

			if ( i == lastActionNum ) { //mal: don't repeat ourselves
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( botVehicleInfo != NULL ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetActionState() == ACTION_STATE_GUN_DESTROYED ) {
				continue;
			}

			if ( botInfo->isDisguised ) {
				continue;
			}

			if ( !Bot_LTGIsAvailable( -1, i, MG_CAMP_GOAL, 1 ) ) {
				continue;
			} //mal: some bot is already on the way to grab that gun - no need for us to do so as well.

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 350.0f, botInfo->team, NOCLASS, false, false, false, true, true );

			if ( matesInArea > 0 ) {
				continue;
			} //mal: already somebody there, so lets pick a different one to grab.

			int badGuysInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 3000.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, true, false );

			if ( badGuysInArea == 0 ) {
				continue;
			} //mal: noone around this gun to attack, so just ignore it.

			gunCampGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_FORWARD_SPAWN ) {

			if ( botWorld->gameLocalInfo.inWarmup ) {
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetTeamOwner() == botInfo->team ) { //mal: it already belongs to us.
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
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

				if ( !ActionIsActiveForTrainingMode( i, botWorld->gameLocalInfo.botTrainingModeObjDelayTime ) ) { //mal: wait a while before we try to complete the goal, to give the player time to decide what he wants to do.
					continue;
				}

				if ( !TeamHasHuman( botInfo->team ) ) { //mal: dont keep frustrating the human player by taking our spawn back too quick, in case he took it from us.
					if ( botThreadData.random.RandomInt( 100 ) > TAKE_SPAWN_IN_DEMO_MODE_CHANCE ) {
						continue;
					}
				}

			}

			if ( !Bot_LTGIsAvailable( -1, i, FDA_GOAL, 2 ) ) {
				continue;
			} //mal: some bot(s) is already on the way to grab that spawn - no need for us to do so as well.

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 350.0f, botInfo->team, NOCLASS, false, false, false, true, true );

			if ( matesInArea > 0 ) {
				continue;
			} //mal: already somebody there, so lets pick a different one to grab.

			spawnGoal = i;
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_VEHICLE_GRAB ) {

			if ( i == lastActionNum ) { //mal: don't repeat ourselves
				continue;
			}

			if ( botWorld->gameLocalInfo.inWarmup ) {
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( botVehicleInfo != NULL ) {
				continue;
			}

			if ( botInfo->spawnTime + 5000 < botWorld->gameLocalInfo.time ) {
				continue;
			}

			validClasses = botThreadData.botActions[ i ]->GetValidClasses();

			if ( validClasses != 0 ) {
				if ( !( validClasses & botClass ) ) { //mal: give the LD's the ability to limit a camp position to a certain class.
					continue;
				}
			}

			if ( !Bot_LTGIsAvailable( -1, i, GRAB_VEHICLE_GOAL, 1 ) ) {
				continue;
			} //mal: some bot is already on the way to grab that vehicle - no need for us to do so as well.

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 500.0f, botInfo->team, NOCLASS, false, false, false, true, true );

			if ( matesInArea > 0 ) {
				continue;
			} //mal: already somebody there, so lets pick a different one to grab.

			if ( FindClosestVehicle( MAX_VEHICLE_RANGE, botThreadData.botActions[ i ]->GetActionOrigin(), NULL_VEHICLE, botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ), PERSONAL | AIR_TRANSPORT, true ) == -1 ) {
				continue;
			}

			vehicleGrabGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_DENY_SPAWNPOINT ) {

			if ( i == lastActionNum ) { //mal: don't repeat ourselves
				continue;
			}

			if ( botWorld->gameLocalInfo.inWarmup ) {
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetTeamOwner() == botInfo->team || botThreadData.botActions[ i ]->GetTeamOwner() == NOTEAM ) { //mal: it already belongs to us.
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( botWorld->botGoalInfo.isTrainingMap ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
				if ( Bot_CheckForHumanInteractingWithEntity( botThreadData.botActions[ i ]->GetActionSpawnControllerEntNum() ) == true ) {
					continue;
				}
			}

			if ( !Bot_LTGIsAvailable( -1, i, STEAL_SPAWN_GOAL, 1 ) ) {
				continue;
			} //mal: some bot is already on the way to grab that spawn - no need for us to do so as well.

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 350.0f, botInfo->team, NOCLASS, false, false, false, true, true );

			if ( matesInArea > 0 ) {
				continue;
			} //mal: already somebody there, so lets pick a different one to grab.

			stealSpawnGoal = i;
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_VEHICLE_ROAM ) {

			if ( i == lastActionNum ) { //mal: don't repeat ourselves
				continue;
			}

			if ( botInfo->isDisguised ) {
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( botVehicleInfo == NULL ) {
				if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
					if ( botThreadData.random.RandomInt( 100 ) > EASY_MODE_CHANCE_WILL_BUILD_DEPLOYABLE_OR_USE_VEHICLE ) {
						continue;
					}

					if ( Bot_CheckThereIsHeavyVehicleInUseAlready() ) {
						continue;
					}
				}
			}

			if ( botVehicleInfo != NULL ) {
				if ( botVehicleInfo->flags & PERSONAL ) {
					continue;
				}

				if ( !( botVehicleInfo->flags & botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ) ) && botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ) != 0  ) {
					continue;
				} //mal: the vehicle we're in cant handle doing this action. Bots want to stay in their current vehicle unless they MUST exit it.
			} else {
				if ( FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ), PERSONAL | AIR_TRANSPORT, true ) == -1 ) {
					continue;
				}
			}

			vRoamGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_VEHICLE_CAMP ) {

			if ( i == lastActionNum ) { //mal: don't repeat ourselves
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( botInfo->isDisguised ) {
				continue;
			}

			if ( botVehicleInfo == NULL ) {
				if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
					if ( botThreadData.random.RandomInt( 100 ) > EASY_MODE_CHANCE_WILL_BUILD_DEPLOYABLE_OR_USE_VEHICLE ) {
						continue;
					}

					if ( Bot_CheckThereIsHeavyVehicleInUseAlready() ) {
						continue;
					}
				}
			}

			if ( botVehicleInfo != NULL ) {
				if ( botVehicleInfo->flags & PERSONAL ) {
					continue;
				}

				if ( !( botVehicleInfo->flags & botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ) ) && botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ) != 0 ) {
					continue;
				} //mal: the vehicle we're in cant handle doing this action. Bots want to stay in their current vehicle unless they MUST exit it.
			} else {
				if ( FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ), PERSONAL | AIR_TRANSPORT, true ) == -1 ) {
					continue;
				}
			}

			vCampGoals.Append( i );
			continue;
		}

//mal_TODO: add more invesigate type actions! 
		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_DEFUSE ||
			botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_PREVENT_BUILD ||
			botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_PREVENT_HACK ) {

			if ( botWorld->gameLocalInfo.inWarmup ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO && !botWorld->botGoalInfo.gameIsOnFinalObjective ) { //mal: dont worry about the obj much in training mode, unless its the final obj...
				continue;
			}

			if ( botWorld->gameLocalInfo.gameIsBotMatch && !TeamHasHuman( botInfo->team ) && botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY && botThreadData.random.RandomInt( 100 ) > 25 ) { //mal: very small chance will respond to actions being done in easy mode botmatch
				continue;
			}
 
			if ( lastCheckActionTime > botWorld->gameLocalInfo.time ) { //mal: don't repeat ourselves
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetActionState() == ACTION_STATE_NORMAL ) { //mal: noones done anything to this action yet.
				continue;
			}

			if ( !botThreadData.botActions[ i ]->ActionIsPriority() ) { //mal: not worth worrying about.
				continue;
			}

			vec = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;
			float distSqr = vec.LengthSqr();

			if ( distSqr > Square( 6000.0f ) ) { //mal: its too far away, we'll never make it in time!
				continue;
			}

			if ( distSqr > Square( 3000.0f ) ) { //mal: if fairly close, go to action no matter what!
				if ( !Bot_LTGIsAvailable( -1, i, INVESTIGATE_ACTION, 3 ) ) { //mal: 3 bots already on their way - ignore this action.
					continue;
				}
			}

			if ( botVehicleInfo != NULL ) { //mal: if we're in a vehicle, ignore this goal if we can't reach it.

				if ( botVehicleInfo->type > ICARUS || botVehicleInfo->type == GOLIATH ) {
					continue;
				}

				int travelTime;

				if ( !Bot_LocationIsReachable( true, botThreadData.botActions[ i ]->GetActionOrigin(), travelTime ) ) {
					continue;
				}
			}

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 300.0f, botInfo->team, NOCLASS, false, false, false, false, true );

			if ( matesInArea > MIN_NUM_INVESTIGATE_CLIENTS ) {
				continue;
			} //mal: already some ppl there, so lets do something else.

			investigateGoals.Append( i );
			continue;
		}
	}

	if ( !Bot_WantsVehicle() || botVehicleInfo != NULL ) {
		botDeployableTarget = Bot_HasDeployableTargetGoals( false );
	}

	botCamperTarget = Bot_CheckForGrieferTargetGoals();

	if ( roamGoals.Num() == 0 && campGoals.Num() == 0 && spawnGoal == -1 && stealSpawnGoal == -1 && investigateGoals.Num() == 0 && vRoamGoals.Num() == 0 && vCampGoals.Num() == 0 && mcpGoal == false && stealGoals.Num() == 0 && deliverGoal == -1 && botDeployableTarget != -1 && priorityCampGoals.Num() == 0 && vehicleGrabGoals.Num() == 0 && gunCampGoals.Num() == 0 && botCamperTarget == -1 ) { //TODO: add more conditions here as create them!
		if ( botWorld->botGoalInfo.botGoal_MCP_VehicleNum != -1 && botWorld->botGoalInfo.botGoal_MCP_VehicleNum == botInfo->proxyInfo.entNum ) {
			Bot_ExitVehicle( false );
		}
		return false;
	}

//mal: if have docs, consider getting them to goal first!
	if ( deliverGoal != -1 ) {
		actionNum = deliverGoal;
		
		if ( ClientHasObj( botNum ) ) {
		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum );

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

		ltgType = DELIVER_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_DeliverGoal;

		if ( !ltgUseVehicle ) {
			Bot_FindRouteToCurrentGoal();	
		}
			
		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
		} else {
			aiState = LTG;
			ltgType = PATROL_DELIVER_GOAL;
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			ltgTime = botWorld->gameLocalInfo.time + 60000;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_PatrolDeliverGoal;
			ltgUseVehicle = false;
			routeNode = NULL;
			PushAINodeOntoStack( -1, -1, actionNum, ltgTime, false, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
			return true;
		}
	}

//mal: next, consider whether or not to go to an action of ours thats under attack.
	if ( investigateGoals.Num() > 0 ) {

		if ( investigateGoals.Num() == 1 ) {
			actionNum = investigateGoals[ 0 ];
		} else {
			j = botThreadData.random.RandomInt( investigateGoals.Num() );
			actionNum = investigateGoals[ j ];
		}

        ltgTime = botWorld->gameLocalInfo.time + 45000;
		lastCheckActionTime = botWorld->gameLocalInfo.time + 20000; //mal: dont do this again for a while.
		aiState = LTG;
		ltgType = INVESTIGATE_ACTION;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_InvestigateGoal;
		return true;
	}

//mal: will always consider steal goals next
	if ( stealGoals.Num() > 0 ) {
		if ( stealGoals.Num() == 1 ) {
			actionNum = stealGoals[ 0 ];
		} else {
			j = botThreadData.random.RandomInt( stealGoals.Num() );
			actionNum = stealGoals[ j ];
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum );

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

		ltgType = STEAL_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_StealGoal;

		if ( !ltgUseVehicle ) {
			Bot_FindRouteToCurrentGoal();	
		}
			
		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

//mal: next, MCP goals.
	if ( mcpGoal && mcpActionNum != ACTION_NULL ) {
		if ( botVehicleInfo != NULL ) {
			if ( botVehicleInfo->type == MCP ) {
				actionNum = mcpActionNum;
				vLTGType = V_DRIVE_MCP;
				V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
				V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_DriveMCPToGoal;
				return true;
			}
		} else {
            if ( Bot_LTGIsAvailable( -1, ACTION_NULL, DRIVE_MCP, 2 ) ) { //mal: couple bots already on their way - ignore the MCP.
				vLTGType = V_DRIVE_MCP;
				V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
				V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_DriveMCPToGoal;
				ltgTarget = botWorld->botGoalInfo.botGoal_MCP_VehicleNum;
				ltgType = DRIVE_MCP;
				ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY;
				actionNum = mcpActionNum;
				ROOT_AI_NODE = &idBotAI::Run_LTG_Node;	
				LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_UseVehicle;
				return true;
			}
		}
	}

	bool enemyNearOurGoal = ClientsNearObj( ( botInfo->team == GDF ) ? STROGG : GDF );
	bool needDefenseAtOurObj = ( priorityCampGoals.Num() > 0 ) ? Bot_CheckNeedClientsOnDefense() : false;

	if ( priorityCampGoals.Num() > 0 && ( botThreadData.random.RandomInt( 100 ) > 50 || enemyNearOurGoal || needDefenseAtOurObj ) ) {
		if ( priorityCampGoals.Num() == 1 ) {
			actionNum = priorityCampGoals[ 0 ];
		} else {
			j = botThreadData.random.RandomInt( priorityCampGoals.Num() );
			actionNum = priorityCampGoals[ j ];
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = DEFENSE_CAMP_GOAL;
  		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		ltgTime = botWorld->gameLocalInfo.time + 60000;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_CampGoal;

		if ( !botInfo->isDisguised ) {
			if ( !ltgUseVehicle ) {
				Bot_FindRouteToCurrentGoal();	
			}

			PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		}

		return true;
	}

//mal: killing people who are harassing us takes precedence over deployables and roaming/camping...
	if ( botCamperTarget != -1 ) {
		if ( botVehicleInfo == NULL ) {
			int closestVehicle = FindClosestVehicle( HUNT_MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, ARMOR | AIR, PERSONAL | AIR_TRANSPORT, true );

			if ( closestVehicle != -1 ) { //mal: if we want to kill a camper, and theres a heavy vehicle around - take it instead of hoofing it.
				ltgTime = botWorld->gameLocalInfo.time + 60000;
				ltgTarget = closestVehicle;
				ltgType = ENTER_VEHICLE_GOAL;
				ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
				LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_EnterVehicleGoal;
				return true;
			}

			aiState = LTG;
			ltgType = HUNT_GOAL;
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_HuntGoal;
			ltgTarget = botCamperTarget;
			ltgTargetSpawnID = botWorld->clientInfo[ botCamperTarget ].spawnID;
			return true;
		} else {
			vLTGTarget = botCamperTarget;
			vLTGTargetSpawnID = botWorld->clientInfo[ botCamperTarget ].spawnID;
			V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
			V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_HuntGoal;
			return true;

		}
	}

//mal: killing deployables takes precedence over just roaming/camping around the map.
	if ( botDeployableTarget != -1 ) {
		if ( DeployableIsMarkedForDeath( botDeployableTarget, true ) ) {
			Bot_AddDelayedChat( botNum, ACKNOWLEDGE_YES, 2 );
		}

		if ( botVehicleInfo == NULL ) {
			int closestVehicle = FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, ARMOR | AIR, PERSONAL | AIR_TRANSPORT, true );

			if ( closestVehicle != -1 ) { //mal: if we want to kill a deployable, and theres a heavy vehicle around - take it instead of hoofing it.
				ltgTime = botWorld->gameLocalInfo.time + 60000;
				ltgTarget = closestVehicle;
				ltgType = ENTER_VEHICLE_GOAL;
				ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
				LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_EnterVehicleGoal;
				return true;
			}

			ltgTarget = botDeployableTarget;
			ltgType = DESTROY_DEPLOYABLE_GOAL;
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
            LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_DestroyDeployable;
			return true;
		}  else {
			vLTGTarget = botDeployableTarget;
			
			if ( botVehicleInfo->isAirborneVehicle ) {
				V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
				V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_AircraftDestroyDeployable;
			} else {
				V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
				V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_GroundVehicleDestroyDeployable;
			}
			return true;
		}
	}

	int	randVehicleUse = ( Bot_WantsVehicle() ) ? 100 : 60;

//mal: we'll take vehicle based roams/camps first. The bot will prefer vehicle goals if already started one. Else, its a tossup whether he picks one or not.
	if ( vCampGoals.Num() > 0 || vRoamGoals.Num() > 0 ) {
        if ( !ClientHasVehicleInWorld( botNum, MAX_VEHICLE_RANGE ) ) {
			if ( botThreadData.random.RandomInt( 100 ) > randVehicleUse ) {
				if ( campGoals.Num() > 0 || roamGoals.Num() > 0 ) { //mal: make sure we ALWAYS have something to do before we ditch vehicle goals.
                    vCampGoals.SetNum( 0, false ); //mal: quick and dirty way of wiping out any record of there being goals of this type
					vRoamGoals.SetNum( 0, false );
				}
			}
		} else {
			if ( vCampGoals.Num() > 0 && vRoamGoals.Num() > 0 ) {
				if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
					vCampGoals.SetNum( 0, false ); //mal: quick and dirty way of wiping out any record of there being goals of this type
				} else {
					vRoamGoals.SetNum( 0, false );
				}
			}
		}
	}

	if ( vRoamGoals.Num() > 0 ) {
		if ( vRoamGoals.Num() == 1 ) {
			actionNum = vRoamGoals[ 0 ];
		} else {
			j = botThreadData.random.RandomInt( vRoamGoals.Num() );
			actionNum = vRoamGoals[ j ];
		}

		if ( botVehicleInfo == NULL ) {
			ltgType = ENTER_HEAVY_VEHICLE;
			ltgTime = botWorld->gameLocalInfo.time + 30000;
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
            LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_UseVehicle; //mal: get a vehicle first
			PushAINodeOntoStack( -1, -1, actionNum, DEFAULT_LTG_TIME, false, true );
		}

        V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
		V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_RoamGoal;
	
		return true;
	}

	if ( vCampGoals.Num() > 0 ) {

		if ( vCampGoals.Num() == 1 ) {
			actionNum = vCampGoals[ 0 ];
		} else {
			j = botThreadData.random.RandomInt( vCampGoals.Num() );
			actionNum = vCampGoals[ j ];
		}

		if ( botVehicleInfo == NULL ) {
			ltgType = ENTER_HEAVY_VEHICLE;
			ltgTime = botWorld->gameLocalInfo.time + 30000;
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
            LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_UseVehicle; //mal: get a vehicle first
			PushAINodeOntoStack( -1, -1, actionNum, DEFAULT_LTG_TIME, false, true );
		}

        V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
		V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_CampGoal;	
		return true;
	} 

//mal: next decide if should grab forward spawn.
	if ( spawnGoal != -1 ) {
		idVec3 distToSpawn = botThreadData.botActions[ spawnGoal ]->GetActionOrigin() - botInfo->origin;
		int randChance;

		if ( distToSpawn.LengthSqr() < Square( 3500.0f ) ) { //mal: if we're close to the spawn, why not just go grab it?
			randChance = 0;
		} else {
			randChance = 50;
		}

		if ( botThreadData.random.RandomInt( 100 ) > ( ( botInfo->isDisguised ) ? 20 : randChance ) ) { //mal: higher chance we'll do this when disguised.
			actionNum = spawnGoal;
			ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum );

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

			ltgType = FDA_GOAL;
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			ltgTime = botWorld->gameLocalInfo.time + 120000;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_SpawnPointGoal;

			if ( !ltgUseVehicle ) {
				Bot_FindRouteToCurrentGoal();	
			}
			
			PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
			return true;
		}
	}

	if ( stealSpawnGoal != -1 ) { //mal: deny our enemy his spawn point
		idVec3 distToSpawn = botThreadData.botActions[ stealSpawnGoal ]->GetActionOrigin() - botInfo->origin;
		int randChance;

		if ( distToSpawn.LengthSqr() < Square( 1500.0f ) ) { //mal: if we're close to the spawn, why not just go grab it?
			randChance = 0;
		} else {
			randChance = 70;
		}

		if ( botThreadData.random.RandomInt( 100 ) > ( ( botInfo->isDisguised ) ? 10 : randChance ) ) { //mal: higher chance we'll do this when disguised.
			actionNum = stealSpawnGoal;
			ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum );

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

			ltgType = STEAL_SPAWN_GOAL;
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			ltgTime = botWorld->gameLocalInfo.time + 120000;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_SpawnPointGoal;

			if ( !ltgUseVehicle ) {
				Bot_FindRouteToCurrentGoal();	
			}
			
			PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
			return true;
		}
	}

	if ( vehicleGrabGoals.Num() > 0 /* && botThreadData.random.RandomInt( 100 ) > 60 */ ) {
		if ( vehicleGrabGoals.Num() == 1 ) {
			actionNum = vehicleGrabGoals[ 0 ];
		} else {
			j = botThreadData.random.RandomInt( vehicleGrabGoals.Num() );
			actionNum = vehicleGrabGoals[ j ];
		}

		ltgType = GRAB_VEHICLE_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + 120000;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
        LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_UseVehicle; //mal: get a vehicle first
		PushAINodeOntoStack( -1, -1, actionNum, DEFAULT_LTG_TIME, false, true );
		return true;
	}

	if ( gunCampGoals.Num() > 0 && botThreadData.random.RandomInt( 100 ) > 70 ) {
		if ( gunCampGoals.Num() == 1 ) {
			actionNum = gunCampGoals[ 0 ];
		} else {
			j = botThreadData.random.RandomInt( gunCampGoals.Num() );
			actionNum = gunCampGoals[ j ];
		}

		ltgType = MG_CAMP_GOAL;
  		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_MG_CampGoal;
		return true;
	}

	//mal: next, decide if we should camp or roam, randomly.
	if ( campGoals.Num() > 0 && roamGoals.Num() > 0 ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			campGoals.SetNum( 0, false ); //mal: quick and dirty way of wiping out any record of there being goals of this type
		} else {
			roamGoals.SetNum( 0, false );
		}
	}

	if ( campGoals.Num() > 0 ) {

		if ( campGoals.Num() == 1 ) {
			actionNum = campGoals[ 0 ];
		} else {
			j = botThreadData.random.RandomInt( campGoals.Num() );
			actionNum = campGoals[ j ];
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = CAMP_GOAL;
  		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		ltgTime = botWorld->gameLocalInfo.time + 30000;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_CampGoal;

		if ( !botInfo->isDisguised ) {
			if ( !ltgUseVehicle ) {
				Bot_FindRouteToCurrentGoal();	
			}

			PushAINodeOntoStack( -1, -1, actionNum, ltgTime, false, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		}

		return true;
	}

	if ( roamGoals.Num() > 0 ) {

		if ( roamGoals.Num() == 1 ) {
			actionNum = roamGoals[ 0 ];
		} else {
			j = botThreadData.random.RandomInt( roamGoals.Num() );
			actionNum = roamGoals[ j ];
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

		if ( ltgUseVehicle ) {
			if ( botVehicleInfo != NULL ) {
				if ( botVehicleInfo->driverEntNum == botNum ) {
                    V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
					V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_RoamGoal;
				} else {
					Bot_ExitVehicle();
				}
			}
		} else {
			if ( botVehicleInfo != NULL ) { //mal: if we're close to the goal, or theres some other reason not to use a vehicle for this goal, then exit our vehicle
                Bot_ExitVehicle();
			}
		}

  		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		ltgTime = botWorld->gameLocalInfo.time + 30000;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_RoamGoal;

		if ( !botInfo->isDisguised ) {
			if ( !ltgUseVehicle ) {
				Bot_FindRouteToCurrentGoal();	
			}
			PushAINodeOntoStack( -1, -1, actionNum, ltgTime, false, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		}

		return true;
	}

	return false;
}

/*
==================
idBotAI::FindCloseSupplyPack

Look for a close ammo/health/supply pack. As a courtesy, medics/Fops won't call this function, so they won't steal each others
packs.
==================
*/
bool idBotAI::Bot_FindCloseSupplyPack( bool healthOnly, bool grenadesOnly ) {
	bool isCrate = false;
	int i, j;
	int entNum = -1;
	int matesInArea;
	float closest = idMath::INFINITY;
	float dist;
	trace_t	tr;
	idVec3 vec;
	idVec3 otherOrg;

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( botWorld->clientInfo[ i ].team != botInfo->team ) {
			continue; //mal: dont try to grab enemies packs
		}

//mal: first, check for crates.
		if ( botWorld->clientInfo[ i ].supplyCrate.entNum != 0 && !ItemIsIgnored( botWorld->clientInfo[ i ].supplyCrate.entNum ) ) {
			if ( botWorld->clientInfo[ i ].supplyCrate.areaNum != 0 ) {
				vec = botWorld->clientInfo[ i ].supplyCrate.origin - botInfo->origin;
				dist = vec.LengthSqr();
				int travelTime;

				if ( dist < Square( ITEM_RANGE ) && Bot_LocationIsReachable( false, botWorld->clientInfo[ i ].supplyCrate.origin, travelTime ) ) { //mal: will go out of our way for crates - just because they're more visible and useful to us.
					closest = dist;
					entNum = botWorld->clientInfo[ i ].supplyCrate.entNum;
					isCrate = true;
				}
			}
		}

#ifdef PACKS_HAVE_NO_NADES
		if ( grenadesOnly ) {
			continue;
		}
#endif
            
		for( j = 0; j < MAX_ITEMS; j++ ) {
			if ( botWorld->clientInfo[ i ].packs[ j ].entNum == 0 ) {
				continue;
			}

			const supplyPackInfo_t& supplyPack = botWorld->clientInfo[ i ].packs[ j ];

			if ( ItemIsIgnored( supplyPack.entNum ) ) {
				continue;
			}

			if ( supplyPack.xySpeed > 0.0f && i != botNum ) { //mal: if the pack is moving somehow, ignore, unless its ours
	 			continue;
			}

			if ( supplyPack.areaNum == 0 ) { //mal: its not in a valid AAS area, ignore!
				continue;
			}

			if ( supplyPack.inWater ) {
				continue;
			}

			if ( !supplyPack.available ) {
				continue;
			}

			if ( !supplyPack.inPlayZone ) {
				continue;
			}

			if ( botInfo->team == GDF ) {
				if ( healthOnly && botWorld->clientInfo[ i ].classType != MEDIC ) {
					continue;
				}

				if ( !healthOnly && botWorld->clientInfo[ i ].classType != FIELDOPS ) {
					continue;
				}
			}

//mal: dont steal packs other classes want to use. If noones around it, its fair game!
			if ( ( botInfo->classType == MEDIC && healthOnly ) || ( botInfo->classType == FIELDOPS && !healthOnly ) ) {
    
				matesInArea = ClientsInArea( botNum, supplyPack.origin, 250.0f, botInfo->team, NOCLASS, false, false, false, false, true );

				if ( matesInArea > 0 ) {
					continue;
				}
			}

			vec = supplyPack.origin - botInfo->origin;

			dist = vec.LengthFast();

			if ( dist > ITEM_RANGE ) {
				continue;
			}

			if ( i != botNum || botInfo->xySpeed > 0.0f ) {
                if ( !InFrontOfClient( botNum, supplyPack.origin ) ) {
					continue;
				}
			} //mal: if its not in front of us, we don't see it, unless its our pack!

			otherOrg = supplyPack.origin;

			otherOrg.z += ITEM_PACK_OFFSET; //mal: raise it up a bit so we can "see" it on uneven terrain.

			botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, otherOrg, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ));

			if ( tr.fraction < 1.0f && tr.c.entityNum != supplyPack.entNum ) {
				continue;
			}

			int travelTime;

			if ( !Bot_LocationIsReachable( false, otherOrg, travelTime ) ) {
				continue;
			}

			if ( dist < closest ) {
				closest = dist;
				entNum = supplyPack.entNum;
			}
		}
	}

	if ( entNum != -1 ) {
		nbgTarget = entNum;

		if ( isCrate ) {
			nbgTargetType = SUPPLY_CRATE;
		} else {
            if ( healthOnly == true ) {
				nbgTargetType = HEAL;
			} else {
				nbgTargetType = REARM;
			}
		}

		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_GetSupplies;
		return true;
	}

	return false;
}


/*
==================
idBotAI::Bot_FindCloseSupplyTeammate

Look for a close mate to bug for supplies.
==================
*/
bool idBotAI::Bot_FindCloseSupplyTeammate( bool buggerForHealth ) {

	int i, busyClient;
	float bestDist = idMath::INFINITY;
	float dist;
	trace_t	tr;
	idVec3 vec;

	if ( enemy != -1 ) { //mal: dont do this if aware of an enemy.
		return false;
	}

	if ( botThreadData.GetBotSkill() == BOT_SKILL_EASY ) { //mal: stupid bot! You aren't smart enough to look for supplies.
		return false;
	}

//mal: first, check to see if a bot is trying to supply me - we'll stop and let him if someone is trying, UNLESS we're important and on the way to goal - he can just follow us.
	if ( !Bot_NBGIsAvailable( botNum, ACTION_NULL, SUPPLY_TEAMMATE, busyClient ) && !Client_IsCriticalForCurrentObj( botNum, 2500.0f ) ) {
		nbgTargetType = NOTYPE;
		nbgTarget = busyClient;
		nbgTargetSpawnID = botWorld->clientInfo[ busyClient ].spawnID;
		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_BugForSupplies;
		return true;
	}

	if ( buggerForHealth ) {
		if ( botInfo->health > 30 ) { //mal: got to be in REALLY bad shape to bug for supplies.
			return false;
		}
	} else {
		if ( botInfo->weapInfo.primaryWeapNeedsAmmo == false ) {
            return false;
		}
	}

	if ( bugForSuppliesDelay > botWorld->gameLocalInfo.time ) {
		return false;
	}

	if ( botThreadData.random.RandomInt( 100 ) > 70 ) {	//mal: randomly decide if we should or shouldn't - if not, dont come here again for a while.
		bugForSuppliesDelay = botWorld->gameLocalInfo.time + 10000;
		return false;
	}

	busyClient = -1;

//mal: noone cares about our pain! Lets find someone to bug into caring! :-P
	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) {
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		if ( botInfo->team == GDF ) {
			if ( buggerForHealth == true ) {
				if ( playerInfo.classType != MEDIC ) {
                    continue;
				}
			} else {
				if ( playerInfo.classType != FIELDOPS ) {
                    continue;
				}
			}
		} else {
			if ( playerInfo.classType != MEDIC ) { //mal: strogg go to the medic for everything.
				continue;
			}
		}

		if ( playerInfo.areaNum == 0 ) {
			continue;
		}

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			continue;
		}

		if ( playerInfo.team != botInfo->team ) {
			continue;
		}

		if ( playerInfo.isActor ) {
			continue;
		}

		vec = playerInfo.origin - botInfo->origin;

		dist = vec.LengthFast();

		if ( dist > 700.0f ) {
			continue;
		}

		botThreadData.clip->TracePoint( CLIP_DEBUG_PARMS tr, botInfo->viewOrigin, playerInfo.viewOrigin, BOT_VISIBILITY_TRACE_MASK, GetGameEntity( botNum ));

		if ( tr.fraction < 1.0f && tr.c.entityNum != i ) { //mal: have to be able to see the mate to chase him for supplies.
			continue;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, playerInfo.origin, travelTime ) ) {
			continue;
		}

		if ( dist < bestDist ) { //mal: find the closest one.
			bestDist = dist;
			busyClient = i;
		}

	}

    if ( busyClient != -1 ) {
        nbgTarget = busyClient;
		nbgTargetSpawnID = botWorld->clientInfo[ busyClient ].spawnID;
		
		if ( buggerForHealth == true ) {
			nbgTargetType = HEAL;
		} else {
			nbgTargetType = REARM;
		}

		ROOT_AI_NODE = &idBotAI::Run_NBG_Node;
		NBG_AI_SUB_NODE = &idBotAI::Enter_NBG_BugForSupplies;
		bugForSuppliesDelay = botWorld->gameLocalInfo.time + 30000; //mal: dont do this again for at LEAST 30 seconds.
        return true;
	}

	return false;
}

/*
==================
idBotAI::Bot_FindDeadWhileInVehicle

Look for a close dead mate to revive while we're in a vehicle.
==================
*/
void idBotAI::Bot_FindDeadWhileInVehicle() {

	int clientNum = -1;

	if ( ( botVehicleInfo->isAirborneVehicle && botVehicleInfo->type != ICARUS ) || botVehicleInfo->inWater ) { //mal_FIXME: do we really want the bots to ignore you if they're in a boat??!
		return;
	}

	if ( aiState == VLTG && vLTGType == V_FOLLOW_TEAMMATE ) { //mal: bots won't jump out of vehicles to get to you if they're escorting
		return;
	}

	if ( Bot_IsInHeavyAttackVehicle() ) { //mal: dont do this if in an attack aircraft or tank.
		return;
	}
	
	if ( botVehicleInfo->driverEntNum != botNum ) { //mal: nothing we can do if we can't control the vehicle.
		return;
	}
	
	if ( botVehicleInfo->type == MCP ) {
		return;
	}

	clientNum = Bot_MedicCheckForDeadMate( -1, -1, 2500.0f );

    if ( clientNum != -1 ) {
		vLTGTarget = clientNum;
		vLTGTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
		V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
		V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_GotoReviveMate;
	}
}

/*
================
idBotAI::Bot_CheckForSoldierGoals

A soldier specific long term goal check.
================
*/
bool idBotAI::Bot_CheckForSoldierGoals() {
	int i, j, num;
	float dist;
	float closest = idMath::INFINITY;
	proxyInfo_t vehicleInfo;
	idVec3 vec;
	idList< int > plantGoals;
	idList< int > trainingModeRoamGoals;

	if ( botWorld->gameLocalInfo.botsIgnoreGoals ) {
		return false;
	}

	if ( botWorld->gameLocalInfo.inWarmup ) {
		return false;
	}

	for( i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_HE_CHARGE ) {

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
				if ( !botWorld->gameLocalInfo.botsDoObjsInTrainingMode ) {
					trainingModeRoamGoals.Append( i );
					continue;
				}

				if ( TeamHumanNearLocation( botInfo->team, vec3_zero, -1.0f, false, SOLDIER, false ) && TeamHumanMissionIsObjective() && botThreadData.botActions[ i ]->ActionIsPriority() ) {
					trainingModeRoamGoals.Append( i );
					continue;
				}

				int timeDelay = ( botThreadData.botActions[ i ]->ActionIsPriority() ) ? botWorld->gameLocalInfo.botTrainingModeObjDelayTime : 60; //mal: some plants are primary objs, so wait longer.

				if ( !ActionIsActiveForTrainingMode( i, timeDelay ) ) { //mal: wait a while before we try to complete the goal, to give the player time to decide what he wants to do.
					trainingModeRoamGoals.Append( i );
					continue;
				}
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: the human has decided to complete the maps goals.
				continue;
			}

			if ( ClientHasChargeInWorld( botNum, true, ACTION_NULL ) ) { //mal: this client has already planted on this action, so ignore it!
				continue;
			}

			if ( botThreadData.botActions[ i ]->ActionIsPriority() == false ) { //mal: secondary actions are not critical. Only 1 bot needs to worry about them.
				if ( !Bot_LTGIsAvailable( -1, i, PLANT_GOAL, 1 ) ) {
					continue;
				}

				int busyClient;

				if ( !Bot_NBGIsAvailable( -1, i, PLANT_BOMB, busyClient ) ) {
					continue;
				}
			}

            plantGoals.Append( i );
			continue;
		}
	}

	if ( plantGoals.Num() == 0 && trainingModeRoamGoals.Num() == 0 ) {
		return false;
	}

//mal: plant actions ALWAYS come first!
	if ( plantGoals.Num() > 0 ) {
        if ( plantGoals.Num() == 1 ) {
			actionNum = plantGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick
				j = botThreadData.random.RandomInt( plantGoals.Num() );
				actionNum = plantGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
	
				for( i = 0; i < plantGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ plantGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthFast();

					if ( dist < closest ) {
						closest = dist;
						num = plantGoals[ i ];
					}
				}
				
				idList< int > linkedActionList;
				FindLinkedActionsForAction( num, plantGoals, linkedActionList );

				if ( linkedActionList.Num() == 0 ) {
					actionNum = num;
				} else {
					j = botThreadData.random.RandomInt( linkedActionList.Num() );
					actionNum = linkedActionList[ j ];
				}
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = PLANT_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY; //mal: do this forever, until we die or complete it!
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_PlantGoal;

		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}

		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( trainingModeRoamGoals.Num() > 0 ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			if ( trainingModeRoamGoals.Num() == 1 ) {
				actionNum = trainingModeRoamGoals[ 0 ];
			} else {
				j = botThreadData.random.RandomInt( trainingModeRoamGoals.Num() );
				actionNum = trainingModeRoamGoals[ j ];
			}	

			ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

			if ( ltgUseVehicle ) {
				if ( botVehicleInfo != NULL ) {
					if ( botVehicleInfo->driverEntNum == botNum ) {
						V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
						V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_RoamGoal;
					} else {
						Bot_ExitVehicle();
					}
				}	
			} else {
				if ( botVehicleInfo != NULL ) { //mal: if we're close to the goal, or theres some other reason not to use a vehicle for this goal, then exit our vehicle
					Bot_ExitVehicle();
				}
			}

	  		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			ltgTime = botWorld->gameLocalInfo.time + 30000;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_RoamGoal;

			if ( !botInfo->isDisguised ) {
				if ( !ltgUseVehicle ) {
					Bot_FindRouteToCurrentGoal();	
				}

				PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
			}

			return true;
		}
	}

	return false; //mal: no soldier goals, find a non-class specific goal to do.
}

/*
================
idBotAI::Bot_CheckForEngineerGoals

A engineer specific long term goal check.
================
*/
bool idBotAI::Bot_CheckForEngineerGoals() {
	bool mcpGoal = false;
	bool doDeployableGoals = !Bot_HasWorkingDeployable( true );
	int  minorBuildGoal = -1;
	int i, j, num;
	int	matesInArea;
	float dist;
	float closest = idMath::INFINITY;
	proxyInfo_t vehicleInfo;
	idVec3 vec;
	idList< int > defuseGoals;
	idList< int > buildGoals;
	idList< int > mineGoals;
	idList< int > deployableGoals;
	idList< int > priorityMineGoals;
	idList< int > priorityDeployGoals;
	idList< int > mgNestGoals; // both repair and build.
	idList< int > trainingModeRoamGoals;

	if ( botWorld->gameLocalInfo.inWarmup ) {
		return false;
	}

	if ( botWorld->botGoalInfo.mapHasMCPGoal && botInfo->team == GDF ) {
		GetVehicleInfo( botWorld->botGoalInfo.botGoal_MCP_VehicleNum, vehicleInfo );
		if ( vehicleInfo.entNum != 0 ) {
			if ( vehicleInfo.health < ( vehicleInfo.maxHealth / 2 ) && vehicleInfo.xyspeed < WALKING_SPEED ) {
				mcpGoal = true;
			}
		}
	}

	for( i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_DEFUSE ) {
			if ( botWorld->gameLocalInfo.botsIgnoreGoals ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
				if ( !botWorld->gameLocalInfo.botsDoObjsInTrainingMode ) { 
					continue;
				}

				if ( TeamHumanNearLocation( botInfo->team, botThreadData.botActions[ i ]->GetActionOrigin(), CLOSE_TO_GOAL_RANGE, false, ENGINEER ) ) {
					continue;
				}
			}

			if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: the human has decided to complete the maps goals.
				continue;
			}
	
			if ( !botThreadData.botActions[ i ]->ArmedChargesInsideActionBBox( -1 ) ) {
				continue;
			}

			vec = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;

			if ( vec.LengthSqr() > Square( 8000.0f ) ) { //mal: its too far away, we'll never make it in time!
				continue;
			}

            defuseGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_MAJOR_OBJ_BUILD ) {
			if ( botWorld->gameLocalInfo.botsIgnoreGoals ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
				if ( !botWorld->gameLocalInfo.botsDoObjsInTrainingMode ) {
					trainingModeRoamGoals.Append( i );
					continue;
				}

				if ( TeamHumanNearLocation( botInfo->team, vec3_zero, -1.0f, false, ENGINEER, false ) && TeamHumanMissionIsObjective() ) {
					trainingModeRoamGoals.Append( i );
					continue;
				}

				if ( !ActionIsActiveForTrainingMode( i, botWorld->gameLocalInfo.botTrainingModeObjDelayTime ) ) { //mal: wait a while before we try to complete the goal, to give the player time to decide what he wants to do.
					trainingModeRoamGoals.Append( i );
					continue;
				}
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: the human has decided to complete the maps goals.
				continue;
			}

			if ( botThreadData.botActions[ i ]->ActionIsPriority() == false ) { //mal: secondary actions are not critical. Only 1 bot needs to worry about them.
				if ( !Bot_LTGIsAvailable( -1, i, BUILD_GOAL, 1 ) ) {
					continue;
				}

				int busyClient;

				if ( !Bot_NBGIsAvailable( -1, i, BUILD, busyClient ) ) {
					continue;
				}
			}
	
            buildGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_DROP_DEPLOYABLE ) {

			if ( !doDeployableGoals ) {
				continue;
			}

			if ( i == botThreadData.ignoreActionNumber ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( botInfo->deployDelayTime > botWorld->gameLocalInfo.time ) {
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( !Bot_LTGIsAvailable( -1, i, DROP_DEPLOYABLE_GOAL, 1 ) ) {
				continue;
			}

			if ( Bot_GetDeployableTypeForAction( i ) == NULL_DEPLOYABLE ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
				if ( botThreadData.random.RandomInt( 100 ) > EASY_MODE_CHANCE_WILL_BUILD_DEPLOYABLE_OR_USE_VEHICLE ) {
					continue;
				}

				if ( botThreadData.botActions[ i ]->GetDeployableType() == APT && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, APT, vec3_zero, -1.0f ) ) {
					continue;
				}

				if ( botThreadData.botActions[ i ]->GetDeployableType() == AVT && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, AVT, vec3_zero, -1.0f ) ) {
					continue;
				}

				if ( ( botThreadData.botActions[ i ]->GetDeployableType() & APT ) && ( botThreadData.botActions[ i ]->GetDeployableType() & AVT ) ) {
					if ( Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, AVT, vec3_zero, -1.0f ) && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, APT, vec3_zero, -1.0f ) ) {
						continue;
					}
				}
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) { //mal: in training mode, dont have too many deployables on the ground, as they can overwhelm the player....
				if ( botThreadData.botActions[ i ]->GetDeployableType() == APT && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, APT, vec3_zero, -1.0f ) ) {
					continue;
				}

				if ( botThreadData.botActions[ i ]->GetDeployableType() == AVT && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, AVT, vec3_zero, -1.0f ) ) {
					continue;
				}

				if ( ( botThreadData.botActions[ i ]->GetDeployableType() & APT ) && ( botThreadData.botActions[ i ]->GetDeployableType() & AVT ) ) {
					if ( Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, AVT, vec3_zero, -1.0f ) && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, APT, vec3_zero, -1.0f ) ) {
						continue;
					}
				}

				if ( !Bot_LTGIsAvailable( -1, ACTION_NULL, DROP_DEPLOYABLE_GOAL, 1 ) ) {
					continue;
				}
			}

			vec = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;

			if ( vec.LengthSqr() > Square( DEPLOYABLE_GOAL_DIST ) ) { 
				continue;
			}

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 300.0f, botInfo->team, ENGINEER, false, false, false, false, true, true );

			if ( matesInArea > 0 ) { //mal: theres prolly a human already there doing this, no need to double team it.
				continue;
			}

			if ( DeployableAtAction( i, true ) ) {
				continue;
			}

			deployableGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_MG_NEST_BUILD || botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_MG_NEST ) {
			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( lastActionNum == i ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
				if ( !botWorld->gameLocalInfo.botsDoObjsInTrainingMode ) { 
					continue;
				}
				
				if ( TeamHumanNearLocation( botInfo->team, vec3_zero, -1.0f, false, ENGINEER, false ) ) {
					continue;
				}

				if ( !ActionIsActiveForTrainingMode( i, 60 ) ) { //mal: wait a while before we try to complete the goal, to give the player time to decide what he wants to do.
					trainingModeRoamGoals.Append( i );
					continue;
				}
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetActionState() == ACTION_STATE_GUN_READY ) { //mal: someone else fixed it.
				continue;
			}

			if ( botVehicleInfo != NULL ) {
				continue;
			}

			if ( !Bot_LTGIsAvailable( -1, i, MG_REPAIR_GOAL, 1 ) ) {
				continue;
			}

			vec = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;

			if ( vec.LengthSqr() > Square( MG_REPAIR_MAX_DIST ) ) { 
				continue;
			}

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 300.0f, botInfo->team, ENGINEER, false, false, false, false, true, true );

			if ( matesInArea > 0 ) { //mal: theres prolly a human already there doing this, no need to double team it.
				continue;
			}

			mgNestGoals.Append( i );
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_DROP_PRIORITY_DEPLOYABLE ) {

			if ( !doDeployableGoals ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( i == botThreadData.ignoreActionNumber ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( botInfo->deployDelayTime > botWorld->gameLocalInfo.time ) {
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( !Bot_LTGIsAvailable( -1, i, DROP_PRIORITY_DEPLOYABLE_GOAL, 1 ) ) {
				continue;
			}

			if ( Bot_GetDeployableTypeForAction( i ) == NULL_DEPLOYABLE ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
				if ( botThreadData.random.RandomInt( 100 ) > EASY_MODE_CHANCE_WILL_BUILD_DEPLOYABLE_OR_USE_VEHICLE ) {
					continue;
				}

				if ( botThreadData.botActions[ i ]->GetDeployableType() == APT && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, APT, vec3_zero, -1.0f ) ) {
					continue;
				}

				if ( botThreadData.botActions[ i ]->GetDeployableType() == AVT && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, AVT, vec3_zero, -1.0f ) ) {
					continue;
				}

				if ( ( botThreadData.botActions[ i ]->GetDeployableType() & APT ) && ( botThreadData.botActions[ i ]->GetDeployableType() & AVT ) ) {
					if ( Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, AVT, vec3_zero, -1.0f ) && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, APT, vec3_zero, -1.0f ) ) {
						continue;
					}
				}
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) { //mal: in training mode, dont have too many deployables on the ground, as they can overwhelm the player....
				if ( botThreadData.botActions[ i ]->GetDeployableType() == APT && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, APT, vec3_zero, -1.0f ) ) {
					continue;
				}

				if ( botThreadData.botActions[ i ]->GetDeployableType() == AVT && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, AVT, vec3_zero, -1.0f ) ) {
					continue;
				}

				if ( ( botThreadData.botActions[ i ]->GetDeployableType() & APT ) && ( botThreadData.botActions[ i ]->GetDeployableType() & AVT ) ) {
					if ( Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, AVT, vec3_zero, -1.0f ) && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, APT, vec3_zero, -1.0f ) ) {
						continue;
					}
				}

				if ( !Bot_LTGIsAvailable( -1, ACTION_NULL, DROP_PRIORITY_DEPLOYABLE_GOAL, 1 ) ) {
					continue;
				}
			}

			vec = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;

			if ( vec.LengthSqr() > Square( DEPLOYABLE_GOAL_DIST ) ) { //mal: goal should be back at base, if too far away, then dont drop one here.
				continue;
			}

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 300.0f, botInfo->team, ENGINEER, false, false, false, false, true, true );

			if ( matesInArea > 0 ) { //mal: theres prolly a human already there doing this, no need to double team it.
				continue;
			}

			if ( DeployableAtAction( i, true ) ) {
				continue;
			}

			priorityDeployGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_MINOR_OBJ_BUILD ) {
			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
				if ( !botWorld->gameLocalInfo.botsDoObjsInTrainingMode ) { 
					continue;
				}
				
				if ( TeamHumanNearLocation( botInfo->team, vec3_zero, -1.0f, false, ENGINEER, false ) ) {
					continue;
				}

				if ( !ActionIsActiveForTrainingMode( i, 60 ) ) { //mal: wait a while before we try to complete the goal, to give the player time to decide what he wants to do.
					trainingModeRoamGoals.Append( i );
					continue;
				}
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

	        minorBuildGoal = i;
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_LANDMINE ) {

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( !botWorld->gameLocalInfo.botsUseMines ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( NumPlayerMines() >= MAX_MINES ) { //mal: player has no available mines
				continue;
			}

			if ( botWorld->botGoalInfo.isTrainingMap ) {
				if ( TeamMineInArea( botThreadData.botActions[ i ]->GetActionOrigin(), 1024.0f ) ) {
					continue;
				}
			}

			if ( !Bot_LTGIsAvailable( -1, i, MINE_GOAL, 1 ) ) {
				continue;
			}

			if ( !botWorld->gameLocalInfo.teamMineInfo[ botInfo->team ].isPriority ) {
				if ( !Bot_LTGIsAvailable( -1, ACTION_NULL, MINE_GOAL, 1 ) ) {
					continue;
				}
			}

			if ( botThreadData.botActions[ i ]->ArmedMinesInsideActionBBox() ) { //mal: someone already planted a mine here
				continue;
			}

//			if ( TeamMineInArea( botThreadData.botActions[ i ]->GetActionOrigin(), MAX_LANDMINE_DIST ) ) {
//				continue;
//			}

			mineGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_DEFENSE_MINE ) { //mal: special mine with high priority.

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( !botWorld->gameLocalInfo.botsUseMines ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( botWorld->botGoalInfo.isTrainingMap ) {
				if ( TeamMineInArea( botThreadData.botActions[ i ]->GetActionOrigin(), 1024.0f ) ) {
					continue;
				}
			}

			if ( NumPlayerMines() >= MAX_MINES ) { //mal: player has no available mines
				continue;
			}

			if ( !Bot_LTGIsAvailable( -1, i, PRIORITY_MINE_GOAL, 1 ) ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->ArmedMinesInsideActionBBox() ) { //mal: someone already planted a mine here
				continue;
			}

//			if ( TeamMineInArea( botThreadData.botActions[ i ]->GetActionOrigin(), MAX_LANDMINE_DIST ) ) {
//				continue;
//			}

			priorityMineGoals.Append( i );
			continue;
		}
	}

	if ( defuseGoals.Num() == 0 && buildGoals.Num() == 0 && mineGoals.Num() == 0 && mcpGoal == false && minorBuildGoal == -1 && deployableGoals.Num() == 0 && priorityMineGoals.Num() == 0 && mgNestGoals.Num() == 0 && trainingModeRoamGoals.Num() == 0 ) {
		return false;
	}

//mal: will always consider defuse goals first!
	if ( defuseGoals.Num() > 0 ) {
        if ( defuseGoals.Num() == 1 ) {
			actionNum = defuseGoals[ 0 ]; //mal: easy enough...
		} else {

			closest = idMath::INFINITY;

			for( i = 0; i < defuseGoals.Num(); i++ ) { //mal: find the closest defuse goal.
				vec = botThreadData.botActions[ defuseGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
				dist = vec.LengthSqr();

				if ( dist < closest ) {
					closest = dist;
					num = defuseGoals[ i ];
				}
			}
			actionNum = num;
		}

		if ( botVehicleInfo != NULL ) { 
            Bot_ExitVehicle();
		}

		ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY; //mal: do this forever, until we die or complete it!
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;	
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_DefuseGoal;
		return true;
	}

//mal: MCP goals come next
	if ( mcpGoal ) { 
		GetVehicleInfo( botWorld->botGoalInfo.botGoal_MCP_VehicleNum, vehicleInfo );

		if ( vehicleInfo.entNum != 0 ) {
			if ( botVehicleInfo != NULL && botVehicleInfo->driverEntNum == botNum && botVehicleInfo->type != MCP ) {
				vLTGOrigin = vehicleInfo.origin;
				vLTGTargetType = MCP_GOAL;
				V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
				V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_TravelToGoalOrigin;
			} else {
				Bot_ExitVehicle();
			}

			ltgTarget = botWorld->botGoalInfo.botGoal_MCP_VehicleNum;
			ROOT_AI_NODE = &idBotAI::Run_LTG_Node;	
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_FixMCP; //mal: dont need to stack this goal
			return true;
		}
	}

	if ( priorityDeployGoals.Num() > 0 ) {
		if ( priorityDeployGoals.Num() == 1 ) {
			actionNum = priorityDeployGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick.
				j = botThreadData.random.RandomInt( priorityDeployGoals.Num() );
				actionNum = priorityDeployGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
				for( i = 0; i < priorityDeployGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ priorityDeployGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthSqr();

					if ( dist < closest ) {
						closest = dist;
						num = priorityDeployGoals[ i ];
					}
				}
		
				actionNum = num;
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = DROP_PRIORITY_DEPLOYABLE_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + DEFAULT_LTG_TIME;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_DeployableGoal;

		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}
		
		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( priorityMineGoals.Num() > 0 ) {
		if ( priorityMineGoals.Num() == 1 ) {
			actionNum = priorityMineGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick.
				j = botThreadData.random.RandomInt( priorityMineGoals.Num() );
				actionNum = priorityMineGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
				for( i = 0; i < priorityMineGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ priorityMineGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthSqr();

					if ( dist < closest ) {
						closest = dist;
						num = priorityMineGoals[ i ];
					}
				}
				actionNum = num;
			}
		}

		ltgType = PRIORITY_MINE_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + 30000;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;	
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_MineGoal;

        Bot_FindRouteToCurrentGoal();	
		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, false, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

//mal: we have both a major and minor build available. Sometimes, we'll decide to do the minor if its closer, just for fun
//mal: we'll only do this if the minor build is a secondary action - else it won't get done unless we have NOTHING else to do.
	if ( minorBuildGoal > -1 && buildGoals.Num() > 0 ) {
		if ( minorBuildGoal == ( ( botInfo->team == GDF ) ? botWorld->botGoalInfo.team_GDF_SecondaryAction : botWorld->botGoalInfo.team_STROGG_SecondaryAction ) ) {
			if ( botThreadData.random.RandomInt( 100 ) > 70 ) {
				vec = botThreadData.botActions[ minorBuildGoal ]->GetActionOrigin() - botInfo->origin;
				float dist1 = vec.LengthSqr();
				vec = botThreadData.botActions[ buildGoals[ 0 ] ]->GetActionOrigin() - botInfo->origin;
				float dist2 = vec.LengthSqr();
				
				if ( dist1 < dist2 ) { //mal: it has to be closer to us then the primary build goal, else its no good.
					buildGoals.SetNum( 0, false );
				}
			}
		}
	}

//mal: major build goals come next
	if ( buildGoals.Num() > 0 ) {
        if ( buildGoals.Num() == 1 ) {
			actionNum = buildGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick.
				j = botThreadData.random.RandomInt( buildGoals.Num() );
				actionNum = buildGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.

				closest = idMath::INFINITY;
	
				for( i = 0; i < buildGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ buildGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthSqr();

					if ( dist < closest ) {
						closest = dist;
						num = buildGoals[ i ];
					}
				}

				idList< int > linkedActionList;
				FindLinkedActionsForAction( num, buildGoals, linkedActionList );

				if ( linkedActionList.Num() == 0 ) {
					actionNum = num;
				} else {
					j = botThreadData.random.RandomInt( linkedActionList.Num() );
					actionNum = linkedActionList[ j ];
				}
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = BUILD_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY; //mal: do this forever, until we die or complete it!
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;	
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_BuildGoal;
		
		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}

		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

//mal: dropping a deployable comes next.
	if ( deployableGoals.Num() > 0 ) {
		if ( deployableGoals.Num() == 1 ) {
			actionNum = deployableGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick.
				j = botThreadData.random.RandomInt( deployableGoals.Num() );
				actionNum = deployableGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
				for( i = 0; i < deployableGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ deployableGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthSqr();

					if ( dist < closest ) {
						closest = dist;
						num = deployableGoals[ i ];
					}
				}
	
				actionNum = num;
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = DROP_DEPLOYABLE_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + DEFAULT_LTG_TIME;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_DeployableGoal;

		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}
		
		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( mineGoals.Num() > 0 && mgNestGoals.Num() > 0 ) {
		if ( botThreadData.random.RandomInt( 100 ) > 60 ) {
			mineGoals.SetNum( 0, false );
			minorBuildGoal = -1; //mal: clear this out too - we just want them to build the MG nest at this point.
		}
	}

//mal: next, consider planting mines.
	if ( mineGoals.Num() > 0 ) {
        if ( mineGoals.Num() == 1 ) {
			actionNum = mineGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick.
				j = botThreadData.random.RandomInt( mineGoals.Num() );
				actionNum = mineGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
	
				for( i = 0; i < mineGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ mineGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthSqr();

					if ( dist < closest ) {
						closest = dist;
						num = mineGoals[ i ];
					}
				}
				actionNum = num;
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = MINE_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + 30000;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;	
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_MineGoal;

		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}

		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, false, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( minorBuildGoal > -1 ) { //mal: these get considered next to last. This is towers/MG nests/etc.
		actionNum = minorBuildGoal;

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = MINOR_BUILD_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + 60000; //mal: do this for a minute.
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;	
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_BuildGoal;
		
		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}

		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, false, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( mgNestGoals.Num() > 0 ) { //mal: these are last.
        if ( mgNestGoals.Num() == 1 ) {
			actionNum = mgNestGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick.
				j = botThreadData.random.RandomInt( mgNestGoals.Num() );
				actionNum = mgNestGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
				for( i = 0; i < mgNestGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ mgNestGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthSqr();

					if ( dist < closest ) {
						closest = dist;
						num = mgNestGoals[ i ];
					}
				}
				actionNum = num;
			}
		}

		ltgType = MG_REPAIR_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + 60000;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;	
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_RepairGunGoal;
        
		Bot_FindRouteToCurrentGoal();

		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, false, false, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( trainingModeRoamGoals.Num() > 0 ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			if ( trainingModeRoamGoals.Num() == 1 ) {
				actionNum = trainingModeRoamGoals[ 0 ];
			} else {
				j = botThreadData.random.RandomInt( trainingModeRoamGoals.Num() );
				actionNum = trainingModeRoamGoals[ j ];
			}	

			ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

			if ( ltgUseVehicle ) {
				if ( botVehicleInfo != NULL ) {
					if ( botVehicleInfo->driverEntNum == botNum ) {
						V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
						V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_RoamGoal;
					} else {
						Bot_ExitVehicle();
					}
				}	
			} else {
				if ( botVehicleInfo != NULL ) { //mal: if we're close to the goal, or theres some other reason not to use a vehicle for this goal, then exit our vehicle
					Bot_ExitVehicle();
				}
			}

	  		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			ltgTime = botWorld->gameLocalInfo.time + 30000;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_RoamGoal;

			if ( !botInfo->isDisguised ) {
				if ( !ltgUseVehicle ) {
					Bot_FindRouteToCurrentGoal();	
				}

				PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
			}

			return true;
		}
	}

	return false; //mal: else, find a normal non-class specific goal to do.
}


/*
================
idBotAI::Bot_CheckForCovertGoals

A covert ops specific long term goal check.
================
*/
bool idBotAI::Bot_CheckForCovertGoals() {
	bool doDeployableGoals = ( !Bot_HasWorkingDeployable( true ) && botInfo->deployDelayTime < botWorld->gameLocalInfo.time && botInfo->deployChargeUsed == 0 );
	int i, j, num, matesInArea;
	int thirdEyeCameraAction = ACTION_NULL;
	float dist;
	float closest = idMath::INFINITY;
	proxyInfo_t vehicleInfo;
	idVec3 vec;
	idList< int > hackGoals;
	idList< int > sniperGoals;
	idList< int > deployableGoals;
	idList< int > hiveGoals;
	idList< int > trainingModeRoamGoals;

	if ( botWorld->gameLocalInfo.inWarmup ) {
		return false;
	}

	for( i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botWorld->botGoalInfo.attackingTeam == STROGG && botInfo->team == GDF && ( thirdEyeCameraAction == ACTION_NULL ) && !botInfo->isDisguised ) {
			if ( botThreadData.botActions[ i ]->GetObjForTeam( STROGG ) == ACTION_HACK || botThreadData.botActions[ i ]->GetObjForTeam( STROGG ) == ACTION_MAJOR_OBJ_BUILD || botThreadData.botActions[ i ]->GetObjForTeam( STROGG ) == ACTION_HE_CHARGE ) {
				if ( botThreadData.botActions[ i ]->ActionIsPriority() && !ActionIsIgnored( i ) ) {
					if ( ClassWeaponCharged( THIRD_EYE ) && botInfo->weapInfo.covertToolInfo.entNum == 0 ) {
						thirdEyeCameraAction = i;
					}
				}
			}
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_HACK ) {
			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO ) {
				if ( !botWorld->gameLocalInfo.botsDoObjsInTrainingMode ) {
					trainingModeRoamGoals.Append( i );
					continue;
				}

				if ( TeamHumanNearLocation( botInfo->team, vec3_zero, -1.0f, false, COVERTOPS, false ) && TeamHumanMissionIsObjective() ) {
					trainingModeRoamGoals.Append( i );
					continue;
				}

				if ( !ActionIsActiveForTrainingMode( i, botWorld->gameLocalInfo.botTrainingModeObjDelayTime ) ) { //mal: wait a while before we try to complete the goal, to give the player time to decide what he wants to do.
					trainingModeRoamGoals.Append( i );
					continue;
				}
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}
	
			if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: the human has decided to complete the maps goals.
				continue;
			}

			if ( botWorld->gameLocalInfo.botsIgnoreGoals ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->ActionIsPriority() == false ) { //mal: secondary actions are not critical. Only 1 bot needs to worry about them.
				if ( !Bot_LTGIsAvailable( -1, i, HACK_GOAL, 1 ) ) {
					continue;
				}

				int busyClient;

				if ( !Bot_NBGIsAvailable( -1, i, HACK, busyClient ) ) {
					continue;
				}
			}

			if ( botWorld->gameLocalInfo.gameMap == VOLCANO && botInfo->team == STROGG ) { //mal: map specific hack - make sure hack obj is reachable before go for it.
				int travelTime;
				if ( !Bot_LocationIsReachable( false, botThreadData.botActions[ i ]->GetActionOrigin(), travelTime ) ) {
					continue;
				}
			}

			hackGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_FLYER_HIVE_LAUNCH ) {

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.botsIgnoreGoals ) {
				continue;
			}	
	
			if ( botWorld->gameLocalInfo.heroMode != false && TeamHasHuman( botInfo->team ) ) { //mal: the human has decided to complete the maps goals.
				continue;
			}

			if ( !Bot_LTGIsAvailable( -1, i, FLYER_HIVE_GOAL, 1 ) ) {
				continue;
			}

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 500.0f, botInfo->team, COVERTOPS, false, false, false, false, true );

			if ( matesInArea > 0 ) { //mal: theres prolly a human already there doing this, no need to double team it.
				continue;
			}

			hiveGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_DROP_DEPLOYABLE || botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_DROP_PRIORITY_DEPLOYABLE ) {

			if ( !doDeployableGoals ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( botInfo->deployDelayTime > botWorld->gameLocalInfo.time ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( botInfo->isDisguised ) {
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( !Bot_LTGIsAvailable( -1, i, DROP_DEPLOYABLE_GOAL, 1 ) ) {
				continue;
			}

			if ( Bot_GetDeployableTypeForAction( i ) == NULL_DEPLOYABLE ) {
				continue;
			}

			if ( !( botThreadData.botActions[ i ]->GetDeployableType() & RADAR ) ) {
				continue;
			}

			vec = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;

			if ( vec.LengthSqr() > Square( 12000.0f /*DEPLOYABLE_GOAL_DIST*/ ) ) { //mal: goal should be back at base, if too far away, then dont drop one here.
				continue;
			}

			if ( Bot_CheckTeamHasDeployableTypeNearAction( botInfo->team, RADAR, i, DEFAULT_DEPLOYABLE_COVERAGE_RANGE ) ) {
				continue;
			}

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 700.0f, botInfo->team, COVERTOPS, false, false, false, false, true );

			if ( matesInArea > 0 ) { //mal: theres prolly a human already there doing this, no need to double team it.
				continue;
			}

			if ( DeployableAtAction( i, true ) ) {
				continue;
			}

			deployableGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_SNIPE ) {

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( botInfo->isDisguised ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( botInfo->weapInfo.primaryWeapon != SNIPERRIFLE ) { //mal_TODO: is this a good choice?
				continue;
			}

			if ( !botInfo->weapInfo.primaryWeapHasAmmo ) { //mal: no ammo - dont camp.
				continue;
			}

			if ( !Bot_MeetsVehicleRequirementsForAction( i ) ) {
				continue;
			}

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 150.0f, botInfo->team, COVERTOPS, false, false, false, true, true );

			if ( matesInArea > 0 ) {
				continue;
			}

			sniperGoals.Append( i );
			continue;
		}
	}

	int botDeployableTarget = Bot_HasDeployableTargetGoals( true ); //mal: only hacks if in disguise.

	if ( hackGoals.Num() == 0 && sniperGoals.Num() == 0 && deployableGoals.Num() == 0 && hiveGoals.Num() == 0 && botDeployableTarget == -1 && trainingModeRoamGoals.Num() == 0 && thirdEyeCameraAction == ACTION_NULL ) {
		return false;
	}

	if ( deployableGoals.Num() > 0 && Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, RADAR, botInfo->origin, -1.0f ) ) { //mal: if no radar on the map - we NEED to get one down!
		if ( deployableGoals.Num() == 1 ) {
			actionNum = deployableGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick.
				j = botThreadData.random.RandomInt( deployableGoals.Num() );
				actionNum = deployableGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
				for( i = 0; i < deployableGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ deployableGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthFast();

					if ( dist < closest ) {
						closest = dist;
						num = deployableGoals[ i ];
					}
				}
	
				actionNum = num;
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = DROP_DEPLOYABLE_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + DEFAULT_LTG_TIME;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_DeployableGoal;

		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}
		
		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( thirdEyeCameraAction != ACTION_NULL && botThreadData.random.RandomInt( 100 ) > 50 ) { //mal: sometimes, lets just drop the 3rd eye before we do anything else.
		actionNum = thirdEyeCameraAction;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_ThirdEyeCameraGoal;
		return true;
	}

//mal: will always consider hack goals first!
	if ( hackGoals.Num() > 0 ) {
        if ( hackGoals.Num() == 1 ) {
			actionNum = hackGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick. There really shouldn't ever be more then 1 hack goal, but just in case....
				j = botThreadData.random.RandomInt( hackGoals.Num() );
				actionNum = hackGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
	
				for( i = 0; i < hackGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ hackGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthFast();

					if ( dist < closest ) {
						closest = dist;
						num = hackGoals[ i ];
					}
				}
				
				idList< int > linkedActionList;
				FindLinkedActionsForAction( num, hackGoals, linkedActionList );

				if ( linkedActionList.Num() == 0 ) {
					actionNum = num;
				} else {
					j = botThreadData.random.RandomInt( linkedActionList.Num() );
					actionNum = linkedActionList[ j ];
				}
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = HACK_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY; //mal: do this forever, until we die or complete it!
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_HackGoal;

		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}

		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	//mal: consider hive goals next
	if ( hiveGoals.Num() > 0 ) {
        if ( hiveGoals.Num() == 1 ) {
			actionNum = hiveGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick. There really shouldn't ever be more then 1 hive goal, but just in case....
				j = botThreadData.random.RandomInt( hiveGoals.Num() );
				actionNum = hiveGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
	
				for( i = 0; i < hiveGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ hiveGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthFast();

					if ( dist < closest ) {
						closest = dist;
						num = hiveGoals[ i ];
					}
				}
				actionNum = num;
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = FLYER_HIVE_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY; //mal: do this forever, until we die or complete it!
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_FlyerHiveGoal;

		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}

		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( deployableGoals.Num() > 0 ) {
		if ( deployableGoals.Num() == 1 ) {
			actionNum = deployableGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick.
				j = botThreadData.random.RandomInt( deployableGoals.Num() );
				actionNum = deployableGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
				for( i = 0; i < deployableGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ deployableGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthFast();

					if ( dist < closest ) {
						closest = dist;
						num = deployableGoals[ i ];
					}
				}
	
				actionNum = num;
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = DROP_DEPLOYABLE_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + DEFAULT_LTG_TIME;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_DeployableGoal;

		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}
		
		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( botDeployableTarget != -1 ) {
		ltgTarget = botDeployableTarget;
		ltgType = DESTROY_DEPLOYABLE_GOAL;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_HackDeployableGoal;
		return true;
	}

//mal: next, consider sniping.
	if ( sniperGoals.Num() > 0 ) {
        if ( sniperGoals.Num() == 1 ) {
			actionNum = sniperGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick.
				j = botThreadData.random.RandomInt( sniperGoals.Num() );
				actionNum = sniperGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
	
				for( i = 0; i < sniperGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ sniperGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthFast();

					if ( dist < closest ) {
						closest = dist;
						num = sniperGoals[ i ];
					}
				}
				actionNum = num;
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = SNIPE_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + DEFAULT_LTG_TIME; 
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_SnipeGoal;

		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}

		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, false, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( trainingModeRoamGoals.Num() > 0 ) {
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			if ( trainingModeRoamGoals.Num() == 1 ) {
				actionNum = trainingModeRoamGoals[ 0 ];
			} else {
				j = botThreadData.random.RandomInt( trainingModeRoamGoals.Num() );
				actionNum = trainingModeRoamGoals[ j ];
			}	

			ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

			if ( ltgUseVehicle ) {
				if ( botVehicleInfo != NULL ) {
					if ( botVehicleInfo->driverEntNum == botNum ) {
						V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
						V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_RoamGoal;
					} else {
						Bot_ExitVehicle();
					}
				}	
			} else {
				if ( botVehicleInfo != NULL ) { //mal: if we're close to the goal, or theres some other reason not to use a vehicle for this goal, then exit our vehicle
					Bot_ExitVehicle();
				}
			}

	  		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
			ltgTime = botWorld->gameLocalInfo.time + 30000;
			LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_RoamGoal;

			if ( !botInfo->isDisguised ) {
				if ( !ltgUseVehicle ) {
					Bot_FindRouteToCurrentGoal();	
				}

				PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
			}

			return true;
		}
	}

	if ( thirdEyeCameraAction != ACTION_NULL ) {
		actionNum = thirdEyeCameraAction;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_ThirdEyeCameraGoal;
		return true;
	}
	
	return false; //mal: else, find a normal non-class specific goal to do.
}

/*
================
idBotAI::Bot_CheckForEscortGoals

A non-class specific long term goal check for escorting important teammates.
================
*/
bool idBotAI::Bot_CheckForEscortGoals() {
	int i;
	int clientNum = -1;
	int allowedEscorts = 1; //mal: normally, only 1 bot might escort you. In hero/demo mode, upto 3 will follow you.
	float dist;
	float closest = idMath::INFINITY;
	playerClassTypes_t criticalClass = ( botInfo->team == GDF ) ? botWorld->botGoalInfo.team_GDF_criticalClass : botWorld->botGoalInfo.team_STROGG_criticalClass;
	proxyInfo_t vehicleInfo;
	idVec3 vec;

	if ( botVehicleInfo != NULL ) {
		return false;
	}

	if ( botWorld->gameLocalInfo.inWarmup ) {
		return false;
	}

	if ( Bot_WantsVehicle() ) {
		return false;
	}

	if ( botInfo->isDisguised ) { //mal: looks silly to be in disguise, escorting someone
		return false;
	}

	if ( botInfo->weapInfo.primaryWeapon == ROCKET || botInfo->weapInfo.primaryWeapon == SNIPERRIFLE ) { //mal: we're not good at escorting..
		return false;
	}

	if ( botWorld->botGoalInfo.isTrainingMap && ( !botThreadData.actorMissionInfo.playerIsOnFinalMission || botThreadData.actorMissionInfo.playerNeedsFinalBriefing ) ) {
		return false;
	}

	if ( botInfo->classType == criticalClass && botWorld->gameLocalInfo.heroMode == false ) { //mal: we're too important to escort others.
		return false;
	}

	if ( botInfo->classType == MEDIC && Bot_HasTeamWoundedInArea( false ) && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
		return false;
	}

	if ( botWorld->botGoalInfo.attackingTeam == botInfo->team ) {
		if ( botWorld->gameLocalInfo.heroMode || botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO || botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) { 
		allowedEscorts = 2;
		}
	} else {
		if ( botWorld->gameLocalInfo.heroMode == false && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO && botWorld->gameLocalInfo.botSkill != BOT_SKILL_EASY ) { 
			if ( botThreadData.random.RandomInt( 100 ) < 50 ) {
				return false;
			}
		} else {
			allowedEscorts = 2;
		}//mal: if we're not excluded from doing major goals, we'll randomly decide to not do this. If we are excluded, then we'll always want to escort critical teammates!
	}

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( i == botNum ) { //mal: dont scan ourselves.
			continue;
		}

		if ( !ClientIsValid( i, -1 ) ) {
			continue; //mal: no valid client in this client slot!
		}

		const clientInfo_t& playerInfo = botWorld->clientInfo[ i ];

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		if ( playerInfo.team != botInfo->team ) {
			continue;
		}

		if ( playerInfo.isDisguised ) {
			continue;
		}

		if ( playerInfo.isBot ) { //mal: dont follow a bot thats following someone else.
			if ( botThreadData.bots[ i ] == NULL ) {
				continue;
			}

			if ( botThreadData.bots[ i ]->GetAIState() == LTG && ( botThreadData.bots[ i ]->GetLTGType() == FOLLOW_TEAMMATE || botThreadData.bots[ i ]->GetLTGType() == FOLLOW_TEAMMATE_BY_REQUEST ) ) {
				continue;
			}
		}

		vec = playerInfo.origin - botInfo->origin;
		dist = vec.LengthSqr();
		int tempAllowedEscorts = allowedEscorts;
		float maxEscortDist = 1900.0f;
		
		if ( ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO || botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) && !playerInfo.isBot ) {
			maxEscortDist *= 3.5f;
			tempAllowedEscorts = 3;
		}

		if ( dist > Square( maxEscortDist ) ) { //mal: they're too far away for us to start following!
			continue;
		}

		if ( !Client_IsCriticalForCurrentObj( i, -1.0f ) && !ClientHasObj( i ) && playerInfo.proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) { //mal: this bozo isn't worth following! Check if he has a vehicle.
			continue;
		}

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			GetVehicleInfo( playerInfo.proxyInfo.entNum, vehicleInfo ); //mal: we may decide to ride along in his vehicle to cause trouble if he has a gunner seat open.

			if ( vehicleInfo.driverEntNum != i ) {
				continue;
			}

			if ( !VehicleIsValid( vehicleInfo.entNum ) ) { //mal: make sure it has a seat open, and is easy to reach.
				continue;
			}

			if ( VehicleIsIgnored( vehicleInfo.entNum ) ) {
				continue;
			}

			if ( vehicleInfo.type == ANANSI || vehicleInfo.type == HORNET ) { //mal: no point being a gunner in these vehicles.
				continue;
			}
		}

		idList< int > busyClients;

		int botsFollowingThisClient = Bot_NumClientsDoingLTGGoal( i, ACTION_NULL, FOLLOW_TEAMMATE, busyClients );

		botsFollowingThisClient += Bot_NumClientsDoingLTGGoal( i, ACTION_NULL, FOLLOW_TEAMMATE_BY_REQUEST, busyClients );

		if ( botsFollowingThisClient >= tempAllowedEscorts ) {
			continue;
		}

		int travelTime;

		if ( !Bot_LocationIsReachable( false, playerInfo.origin, travelTime ) ) {
			continue;
		}

		if ( dist < closest ) { //mal: pick the closest client to escort.
			closest = dist;
			clientNum = i;
		}
	}

	if ( clientNum != -1 ) {
		aiState = LTG;
		ltgTarget = clientNum;
		ltgType = FOLLOW_TEAMMATE;
		ltgTargetSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_FollowMate;
		return true;
	}

	return false; //mal: else, find a normal non-class specific goal to do.
}


/*
================
idBotAI::Bot_CheckForFieldOpsGoals

A FieldOps specific long term goal check.
They only have one primary LTG - to rain fire support down on you!
================
*/
bool idBotAI::Bot_CheckForFieldOpsGoals() {
	bool doFireSupportGoals = true;
	bool doDeployableGoals = !Bot_HasWorkingDeployable( true );
	int i, j, num;
	int randNum;
	int matesInArea;
	int numFOps = botThreadData.GetNumClassOnTeam( botInfo->team, botInfo->classType );
	float dist;
	float closest = idMath::INFINITY;
	proxyInfo_t vehicleInfo;
	idVec3 vec;
	idList< int > fireSupportGoals;
	idList< int > deployableGoals;

	if ( botWorld->gameLocalInfo.inWarmup ) {
		return false;
	}

	if ( numFOps == 1 ) {
		randNum = 70; 
	} else if ( numFOps == 2 ) {
		randNum = 50;
	} else {
		randNum = 30;
	}

	if ( botThreadData.random.RandomInt( 100 ) > randNum ) { //mal: will randomly decide to do this based on how many FOps we have on the team already.
		doFireSupportGoals = false;
	}

	for( i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_FIRESUPPORT ) {

			if ( !doFireSupportGoals ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( i == lastActionNum ) { //mal: don't repeat ourselves
				continue;
			}

			if ( botThreadData.botActions[ i ]->actionTargets[ 0 ].inuse == false ) {
				if ( botThreadData.AllowDebugData() ) {
					botThreadData.Printf("^1Warning: Fire Support Action %s has no target!\n", botThreadData.botActions[ i ]->name.c_str() );
				}

				continue;
			}

			if ( !Bot_HasWorkingDeployable() ) {
				continue;
			}

			if ( !Bot_MeetsVehicleRequirementsForAction( i ) ) {
				continue;
			}

			idVec3 targetOrg = botThreadData.botActions[ i ]->actionTargets[ 0 ].origin;

			if ( Bot_EnemyAITInArea( targetOrg ) ) {
				continue;
			}

			if ( ClientHasFireSupportInWorld( botNum ) ) {
				continue;
			}

			if ( !Bot_LTGIsAvailable( -1, i, FIRESUPPORT_CAMP_GOAL, 1 ) ) { //mal: some bot is already doing this.
				continue;
			}

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 350.0f, botInfo->team, FIELDOPS, false, false, false, false, true );

			if ( matesInArea > 0 ) { //mal: theres prolly a human already there doing this, no need to double team it.
				continue;
			}
            
			fireSupportGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_DROP_DEPLOYABLE ) {

			if ( !doDeployableGoals ) {
				continue;
			}

			if ( Bot_IsInHeavyAttackVehicle() ) {
				continue;
			}

			if ( Bot_WantsVehicle() ) {
				continue;
			}

			if ( botInfo->deployDelayTime > botWorld->gameLocalInfo.time ) {
				continue;
			}

			if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_EASY ) {
				if ( botThreadData.random.RandomInt( 100 ) > EASY_MODE_CHANCE_WILL_BUILD_DEPLOYABLE_OR_USE_VEHICLE ) {
					continue;
				}

				if ( Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, NUKE, vec3_zero, -1.0f ) || Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, ARTILLERY, vec3_zero, -1.0f ) || Bot_CheckTeamHasDeployableTypeNearLocation( botInfo->team, ROCKET_ARTILLERY, vec3_zero, -1.0f ) ) {
					continue;
				}
			}

			if ( ActionIsIgnored( i ) ) {
				continue;
			}

			if ( !Bot_LTGIsAvailable( -1, i, DROP_DEPLOYABLE_GOAL, 1 ) ) {
				continue;
			}

			if ( !( botThreadData.botActions[ i ]->GetDeployableType() & NUKE ) && !( botThreadData.botActions[ i ]->GetDeployableType() & ARTILLERY ) && !( botThreadData.botActions[ i ]->GetDeployableType() & ROCKET_ARTILLERY ) ) {
				continue;
			}

			if ( Bot_GetDeployableTypeForAction( i ) == NULL_DEPLOYABLE ) {
				continue;
			}

			vec = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;

			if ( vec.LengthSqr() > Square( DEPLOYABLE_GOAL_DIST ) ) { //mal: goal should be back at base, if too far away, then dont drop one here.
				continue;
			}

			matesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 700.0f, botInfo->team, FIELDOPS, false, false, false, false, true );

			if ( matesInArea > 0 ) { //mal: theres prolly a human already there doing this, no need to double team it.
				continue;
			}

			if ( DeployableAtAction( i, true ) ) {
				continue;
			}

			deployableGoals.Append( i );
			continue;
		}
	}

	if ( fireSupportGoals.Num() == 0 && deployableGoals.Num() == 0 ) {
		return false;
	}

	if ( deployableGoals.Num() > 0 ) {
		if ( deployableGoals.Num() == 1 ) {
			actionNum = deployableGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick.
				j = botThreadData.random.RandomInt( deployableGoals.Num() );
				actionNum = deployableGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
				for( i = 0; i < deployableGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ deployableGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthSqr();

					if ( dist < closest ) {
						closest = dist;
						num = deployableGoals[ i ];
					}
				}
	
				actionNum = num;
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = DROP_DEPLOYABLE_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + DEFAULT_LTG_TIME;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_DeployableGoal;

		if ( !ltgUseVehicle ) {
            Bot_FindRouteToCurrentGoal();	
		}
		
		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	if ( fireSupportGoals.Num() > 0 ) {
		if ( fireSupportGoals.Num() == 1 ) {
			actionNum = fireSupportGoals[ 0 ]; //mal: easy enough...
		} else {
			if ( botInfo->spawnTime + 5000 > botWorld->gameLocalInfo.time ) { //mal: if we're just born, randomly pick.
				j = botThreadData.random.RandomInt( fireSupportGoals.Num() );
				actionNum = fireSupportGoals[ j ];
			} else { //mal: else sort thru the list and find the closest one.
				for( i = 0; i < fireSupportGoals.Num(); i++ ) {
					vec = botThreadData.botActions[ fireSupportGoals[ i ] ]->GetActionOrigin() - botInfo->origin;
					dist = vec.LengthFast();
	
					if ( dist < closest ) {
						closest = dist;
						num = fireSupportGoals[ i ];
					}
				}
		        actionNum = num;
			}
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

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

		ltgType = FIRESUPPORT_CAMP_GOAL;
		ltgTime = botWorld->gameLocalInfo.time + DEFAULT_LTG_TIME;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_FireSupportGoal;

		if ( !ltgUseVehicle ) {
			Bot_FindRouteToCurrentGoal();	
		}

		PushAINodeOntoStack( -1, -1, actionNum, ltgTime, true, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_CheckForTacticalAction

This works in two parts - first we check if we're already in the process of doing a tactical action, and whether or not its still valid.
If not, then we check if we should do a tactical action - what what kind of action that should be, based on our class and situation.
================
*/
int idBotAI::Bot_CheckForTacticalAction() {
	botActionGoals_t actionFilter1 = ACTION_NULL;
	botActionGoals_t actionFilter2 = ACTION_NULL; //mal: currently, all classes use this for the grenade.
	botActionGoals_t actionFilter3 = ACTION_NULL;
	botActionGoals_t actionType;
	idBox playerBox;

	if ( tacticalActionIgnoreTime > botWorld->gameLocalInfo.time ) {
		return -1;
	}

	if ( tacticalActionPauseTime > botWorld->gameLocalInfo.time ) {
		return TACTICAL_PAUSE_ACTION;
	}

	if ( botInfo->classType == FIELDOPS ) {
		if ( tacticalActionTime > botWorld->gameLocalInfo.time ) {
			if ( botThreadData.botActions[ tacticalActionNum ]->GetObjForTeam( botInfo->team ) == ACTION_AIRCAN_HINT ) {
                if ( !ClassWeaponCharged( AIRCAN ) ) {
                    return -1;
				}
			}

			if ( botThreadData.botActions[ tacticalActionNum ]->GetObjForTeam( botInfo->team ) == ACTION_SHIELD_HINT ) {
                if ( !ClassWeaponCharged( SHIELD_GUN ) ) {
                    return -1;
				}

				if ( botInfo->deviceChargeUsed > tacticalDeviceCharge ) {
					tacticalActionIgnoreTime = botWorld->gameLocalInfo.time + 1000;
					return -1;
				}
			}
		}

		if ( ClassWeaponCharged( AIRCAN ) ) {
            actionFilter1 = ACTION_AIRCAN_HINT;
		}

		if ( botInfo->team == STROGG && ClassWeaponCharged( SHIELD_GUN ) ) {
            actionFilter3 = ACTION_SHIELD_HINT;
		}
	}
	
	if ( botInfo->classType == COVERTOPS ) {
		if ( tacticalActionTime > botWorld->gameLocalInfo.time ) {
			if ( botThreadData.botActions[ tacticalActionNum ]->GetObjForTeam( botInfo->team ) == ACTION_SMOKE_HINT ) {
                if ( !ClassWeaponCharged( SMOKE_NADE ) ) {
                    return -1;
				}
			}

			if ( botThreadData.botActions[ tacticalActionNum ]->GetObjForTeam( botInfo->team ) == ACTION_THIRDEYE_HINT ) {
                if ( !ClassWeaponCharged( THIRD_EYE ) ) {
                    return -1;
				}
			}

			if ( botThreadData.botActions[ tacticalActionNum ]->GetObjForTeam( botInfo->team ) == ACTION_FLYER_HIVE_HINT ) {
				if ( botInfo->weapInfo.covertToolInfo.entNum == 0 && tacticalActionTimer > 0 ) {
					return -1;
				}
				
				if ( ( tacticalActionTime - botWorld->gameLocalInfo.time ) < 200 ) {
					botUcmd->botCmds.attack = true;
					return -1;
				}
			}

			if ( botThreadData.botActions[ tacticalActionNum ]->GetObjForTeam( botInfo->team ) == ACTION_TELEPORTER_HINT ) {
				if ( !ClassWeaponCharged( TELEPORTER ) && tacticalActionTimer2 < botWorld->gameLocalInfo.time ) {
					botIdealWeapNum = TELEPORTER;
					botIdealWeapSlot = NO_WEAPON;

					if ( botInfo->weapInfo.weapon == TELEPORTER ) {
						botUcmd->botCmds.attack = true;
						return -1;
					}
				}
			}
		}

		if ( botInfo->isDisguised == false && botInfo->team == GDF && ClassWeaponCharged( SMOKE_NADE ) ) {
            actionFilter1 = ACTION_SMOKE_HINT;
		}

		if ( botInfo->isDisguised == false && botInfo->team == GDF && ClassWeaponCharged( THIRD_EYE ) && botInfo->weapInfo.covertToolInfo.entNum == 0 ) {
			actionFilter3 = ACTION_THIRDEYE_HINT;
		}

		if ( botInfo->isDisguised == false && botInfo->team == STROGG && ClassWeaponCharged( TELEPORTER ) && !ClientHasObj( botNum ) ) {
			actionFilter1 = ACTION_TELEPORTER_HINT;
		}

		if ( botInfo->isDisguised == false && botInfo->team == STROGG && ClassWeaponCharged( FLYER_HIVE ) && !ClientHasObj( botNum ) && !Client_IsCriticalForCurrentObj( botNum, -1.0f ) && ClientsInArea( botNum, botInfo->origin, 350.0f, STROGG, COVERTOPS, false, false, false, true, true ) <= 1 ) {
			actionFilter3 = ACTION_FLYER_HIVE_HINT;
		}
	}

	if ( botInfo->classType == MEDIC ) {
		if ( tacticalActionTime > botWorld->gameLocalInfo.time ) {
			if ( botThreadData.botActions[ tacticalActionNum ]->GetObjForTeam( botInfo->team ) == ACTION_SUPPLY_HINT ) {
                if ( !ClassWeaponCharged( SUPPLY_MARKER ) ) {
                    return -1;
				}
			}
		}

		if ( botInfo->supplyCrate.entNum == 0 && botInfo->team == GDF && ClassWeaponCharged( SUPPLY_MARKER ) && !Bot_CheckIfHealthCrateInArea( 1024.0f ) ) {
            actionFilter1 = ACTION_SUPPLY_HINT;
		}
	}

	if ( tacticalActionTime > botWorld->gameLocalInfo.time ) {
        if ( botThreadData.botActions[ tacticalActionNum ]->GetObjForTeam( botInfo->team ) == ACTION_NADE_HINT ) {
			if ( lastGrenadeTime + 5000 > botWorld->gameLocalInfo.time || botInfo->weapInfo.hasNadeAmmo == false && tacticalActionPauseTime < botWorld->gameLocalInfo.time ) {
				tacticalActionPauseTime = botWorld->gameLocalInfo.time + 3000;
				tacticalActionTime = tacticalActionPauseTime;
                return TACTICAL_PAUSE_ACTION;
			}
		}
	}

	if ( lastGrenadeTime + 10000 < botWorld->gameLocalInfo.time ) {
        if ( botInfo->weapInfo.hasNadeAmmo != false ) {
			actionFilter2 = ACTION_NADE_HINT;
		}
	}

	if ( tacticalActionTime > botWorld->gameLocalInfo.time ) {
		return tacticalActionNum;
	}

	if ( actionFilter1 == ACTION_NULL && actionFilter2 == ACTION_NULL && actionFilter3 == ACTION_NULL ) {
		return -1;
	}

	tacticalActionNum = ACTION_NULL;

	for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) { //mal: if action is turned off, ignore
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		actionType = botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team );

		if ( ( actionFilter1 == ACTION_NULL || actionType != actionFilter1 ) && ( actionFilter2 == ACTION_NULL || actionType != actionFilter2 ) && ( actionFilter3 == ACTION_NULL || actionType != actionFilter3 ) ) {
			continue;
		}

		playerBox = idBox( botInfo->localBounds, botInfo->origin, botInfo->bodyAxis );

		if ( !botThreadData.botActions[ i ]->actionBBox.IntersectsBox( playerBox ) ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_AIRCAN_HINT ) {
   
			int enemiesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 1700.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, false, false );
			int friendsInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 1700.0f, botInfo->team , NOCLASS, false, false, false, false, false );

			bool deployablesInArea = Bot_CheckTeamHasDeployableTypeNearAction( ( botInfo->team == GDF ) ? STROGG : GDF, NULL_DEPLOYABLE, i, 1700.0f );

			if ( ( enemiesInArea == 0 && deployablesInArea == false ) || friendsInArea > 1 ) { //mal: no target of value in this area, or too many friendlies, so dont bother.
				continue;
			}
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_NADE_HINT ) {
   			int enemiesInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 500.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, false, false );
			int friendsInArea = ClientsInArea( botNum, botThreadData.botActions[ i ]->GetActionOrigin(), 500.0f, botInfo->team , NOCLASS, false, false, false, false, false );

			bool deployablesInArea = Bot_CheckTeamHasDeployableTypeNearAction( ( botInfo->team == GDF ) ? STROGG : GDF, NULL_DEPLOYABLE, i, 500.0f );

			if ( ( enemiesInArea == 0 && deployablesInArea == false ) || friendsInArea > 1 ) { //mal: no target of value in this area, or too many friendlies, so dont bother.
				continue;
			}
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_SUPPLY_HINT ) { //make sure no crates in area.
			bool dropCrate = true;
			for( int k = 0; k < MAX_CLIENTS; k++ ) {
				const clientInfo_t& player = botWorld->clientInfo[ k ];

				if ( player.supplyCrate.entNum == 0 ) {
					continue;
				}

				idVec3 vec = player.supplyCrate.origin - botThreadData.botActions[ i ]->GetActionOrigin();

				if ( vec.LengthSqr() < Square( ITEM_RANGE * 2.0f ) ) {
					dropCrate = false;
					break;
				}
			}

			if ( !dropCrate ) {
				continue;
			}
		}

		tacticalActionNum = i;
		tacticalActionTimer = 0;
		tacticalActionTimer2 = 0;
		tacticalActionOrigin = vec3_zero;
		tacticalDeviceCharge = botInfo->deviceChargeUsed;
		tacticalActionIgnoreTime = 0;
		tacticalActionPauseTime = 0;

		if ( botThreadData.botActions[ tacticalActionNum ]->GetObjForTeam( botInfo->team ) != ACTION_FLYER_HIVE_HINT ) {
			tacticalActionTime = botWorld->gameLocalInfo.time + TACTICAL_ACTION_TIME;
			ltgTime += TACTICAL_ACTION_TIME; //mal: give us some extra time to complete our long term goal, while we pause for a sec for this tactical one.
		} else {
			tacticalActionTime = botWorld->gameLocalInfo.time + TACTICAL_HIVE_TIME;
			ltgTime += TACTICAL_HIVE_TIME; //mal: give us some extra time to complete our long term goal, while we pause for a sec for this tactical one.
		}

		break;
	}

//mal_FIXME: add more tactical actions for more classes!
	return tacticalActionNum;
}

/*
================
idBotAI::Bot_CheckForDroppedObjGoals
================
*/
bool idBotAI::Bot_CheckForDroppedObjGoals() {
	if ( botWorld->gameLocalInfo.inWarmup ) {
		return false;
	}

	if ( ClientHasObj( botNum ) ) {
		return false;
	}

	if ( Bot_WantsVehicle() ) {
		return false;
	}

	if ( botInfo->isActor ) {
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
idBotAI::Bot_CheckMCPGoals
================
*/
bool idBotAI::Bot_CheckMCPGoals() {
	if ( botVehicleInfo != NULL && botVehicleInfo->type != MCP ) {
		return false;
	}

	if ( botWorld->gameLocalInfo.heroMode && TeamHasHuman( botInfo->team ) ) {
		return false;
	}

	if ( Bot_WantsVehicle() ) {
		return false;
	}

	if ( botWorld->gameLocalInfo.inWarmup ) {
		return false;
	}

	if ( !botWorld->botGoalInfo.mapHasMCPGoal ) {
		return false;
	}

	if ( Bot_IsInHeavyAttackVehicle() ) {
		return false;
	}

	proxyInfo_t mcp;
	GetVehicleInfo( botWorld->botGoalInfo.botGoal_MCP_VehicleNum, mcp );

	if ( mcp.entNum == 0 ) {
		return false;
	}

	if ( mcp.isDeployed ) {
		return false;
	}

	if ( botWorld->gameLocalInfo.botSkill == BOT_SKILL_DEMO && mcp.driverEntNum != botNum ) {
		if ( !botWorld->gameLocalInfo.botsDoObjsInTrainingMode ) {
			return false;
		}

		if ( TeamHasHuman( botInfo->team ) && ( botWorld->botGoalInfo.mapHasMCPGoalTime + ( botWorld->gameLocalInfo.botTrainingModeObjDelayTime * 1000 ) ) > botWorld->gameLocalInfo.time ) {
			return false;
		}
	}

	if ( mcp.isImmobilized ) {
		return false;
	}

	if ( mcp.driverEntNum != -1 && mcp.driverEntNum != botNum ) {
		return false;
	}

	idVec3 vec = mcp.origin - botInfo->origin;

	if ( vec.LengthSqr() > Square( MAX_MCP_ATTRACT_DIST ) ) {
		return false;
	}

	int mcpActionNum = -1;

	for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) != ACTION_MCP_OUTPOST ) {
			continue;
		}

		mcpActionNum = i;
		break;
	}

	if ( mcpActionNum == -1 ) {
		return false;
	}

	ignoreMCPRouteAction = false;

	if ( botVehicleInfo != NULL && botVehicleInfo->actionRouteNumber != ACTION_NULL && !ActionIsIgnored( botVehicleInfo->actionRouteNumber ) ) { //mal: check to see if we should go to a MCP route goal first, to give us varied paths.
		idVec3 vec = botThreadData.botActions[ botVehicleInfo->actionRouteNumber ]->GetActionOrigin() - botInfo->origin;

		if ( vec.LengthSqr() > Square( botThreadData.botActions[ botVehicleInfo->actionRouteNumber ]->GetRadius() ) ) {
			vec = botThreadData.botActions[ mcpActionNum ]->GetActionOrigin() - botInfo->origin;
			float ourDistToOutPost = vec.LengthSqr();
			vec = botThreadData.botActions[ botVehicleInfo->actionRouteNumber ]->GetActionOrigin() - botThreadData.botActions[ mcpActionNum ]->GetActionOrigin();
			float routeDistToOutPost = vec.LengthSqr();

			if ( routeDistToOutPost < ourDistToOutPost ) {
				actionNum = botVehicleInfo->actionRouteNumber;
				vLTGType = V_DRIVE_MCP_ROUTE;
				V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
				V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_DriveMCPToRouteGoal;
				return true;
			}
		}
	}

	if ( botVehicleInfo != NULL ) {
		ignoreMCPRouteAction = true;
		actionNum = mcpActionNum;
		vLTGType = V_DRIVE_MCP;
		V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
		V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_DriveMCPToGoal;
		return true;
	} else {
		vLTGType = V_DRIVE_MCP;
		V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
		V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_DriveMCPToGoal;
		ltgTarget = botWorld->botGoalInfo.botGoal_MCP_VehicleNum;
		ltgType = DRIVE_MCP;
		ltgTime = botWorld->gameLocalInfo.time + BOT_INFINITY;
		actionNum = mcpActionNum;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;	
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_UseVehicle;
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_CheckForVehicleFallBackGoals

Sometimes theres nothing available for the bots to do. So go thru and find something productive.
================
*/
bool idBotAI::Bot_CheckForVehicleFallBackGoals() {
	if ( botThreadData.AllowDebugData() ) {
		botThreadData.Printf("Bot Vehicle: %i used a fallback goal!\n", botNum );
	}

	idList< int > vehicleRoamGoals;

	for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_VEHICLE_ROAM ) {
			if ( botVehicleInfo != NULL ) {
				if ( botVehicleInfo->flags & PERSONAL ) {
					continue;
				}

				if ( !( botVehicleInfo->flags & botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ) ) && botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ) != 0  ) {
					continue;
				} //mal: the vehicle we're in cant handle doing this action. Bots want to stay in their current vehicle unless they MUST exit it.
			} else {
				if ( FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ), PERSONAL | AIR_TRANSPORT, true ) == -1 ) {
					continue;
				}
			}

			vehicleRoamGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( botInfo->team ) == ACTION_VEHICLE_CAMP ) {
			if ( botVehicleInfo != NULL ) {
				if ( botVehicleInfo->flags & PERSONAL ) {
					continue;
				}

				if ( !( botVehicleInfo->flags & botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ) ) && botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ) != 0 ) {
					continue;
				} //mal: the vehicle we're in cant handle doing this action. Bots want to stay in their current vehicle unless they MUST exit it.
			} else {
				if ( FindClosestVehicle( MAX_VEHICLE_RANGE, botInfo->origin, NULL_VEHICLE, botThreadData.botActions[ i ]->GetActionVehicleFlags( botInfo->team ), PERSONAL | AIR_TRANSPORT, true ) == -1 ) {
					continue;
				}
			}

			vehicleRoamGoals.Append( i );
			continue;
		}
	}

	if ( vehicleRoamGoals.Num() > 0 ) {
		if ( vehicleRoamGoals.Num() == 1 ) {
			actionNum = vehicleRoamGoals[ 0 ];
		} else {
			int j = botThreadData.random.RandomInt( vehicleRoamGoals.Num() );
			actionNum = vehicleRoamGoals[ j ];
		}

        V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
		V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_CampGoal;	
		return true;
	}

	return false;
}

/*
================
idBotAI::Bot_CheckForFallBackGoals

Sometimes theres nothing available for the bots to do. So go thru and find something productive.
================
*/
bool idBotAI::Bot_CheckForFallBackGoals() {
	ltgUseVehicle = false;
	int j;
	playerTeamTypes_t teamFilter = botInfo->team;
	playerTeamTypes_t covertTeamFilter = botInfo->team;
	idVec3 vec;
	idList< int > roamGoals;

	if ( botThreadData.AllowDebugData() ) {
		botThreadData.Printf("Bot Client: %i used a fallback goal!\n", botNum );
	}

	if ( botInfo->isDisguised ) { //mal: if the bots disguised, will do the enemys' camp/roam goals.
		covertTeamFilter = ( botInfo->team == GDF ) ? STROGG : GDF;
	}

	for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_DELIVER ) { //mal: only 1 deliver point on a map at a time.
			if ( botWorld->gameLocalInfo.inWarmup ) {
				continue;
			}

			roamGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( covertTeamFilter ) == ACTION_CAMP || botThreadData.botActions[ i ]->GetObjForTeam( covertTeamFilter ) == ACTION_DEFENSE_CAMP ) {

			bool isPriority = ( botThreadData.botActions[ i ]->GetObjForTeam( covertTeamFilter ) == ACTION_DEFENSE_CAMP ) ? true : false;

			if ( !Bot_MeetsVehicleRequirementsForAction( i ) ) {
				continue;
			}

			if ( botInfo->isDisguised ) {
				if ( !botThreadData.botActions[ i ]->disguiseSafe ) { //mal: not a good one for a disguised covert.
					continue;
				}
			} 

			roamGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( covertTeamFilter ) == ACTION_ROAM ) {

			if ( !Bot_MeetsVehicleRequirementsForAction( i ) ) {
				continue;
			}

			if ( botInfo->isDisguised ) {
				if ( !botThreadData.botActions[ i ]->disguiseSafe ) { //mal: not a good one for a disguised covert.
					continue;
				}
			}

			roamGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_FORWARD_SPAWN ) {

			if ( botWorld->gameLocalInfo.inWarmup ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetTeamOwner() == botInfo->team ) { //mal: it already belongs to us.
				continue;
			}

			roamGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_DENY_SPAWNPOINT ) {

			if ( botWorld->gameLocalInfo.inWarmup ) {
				continue;
			}

			if ( botThreadData.botActions[ i ]->GetTeamOwner() == botInfo->team || botThreadData.botActions[ i ]->GetTeamOwner() == NOTEAM ) { //mal: it already belongs to us.
				continue;
			}

			roamGoals.Append( i );
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_DEFUSE ||
			botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_PREVENT_BUILD ||
			botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_PREVENT_HACK ||
			botThreadData.botActions[ i ]->GetObjForTeam( teamFilter ) == ACTION_PREVENT_STEAL ) {

			if ( botWorld->gameLocalInfo.inWarmup ) {
				continue;
			}
 
			if ( !botThreadData.botActions[ i ]->ActionIsPriority() ) { //mal: not worth worrying about.
				continue;
			}

			vec = botThreadData.botActions[ i ]->GetActionOrigin() - botInfo->origin;
			float distSqr = vec.LengthSqr();

			if ( distSqr > Square( 6000.0f ) ) { //mal: its too far away, we'll never make it in time!
				continue;
			}

			roamGoals.Append( i );
			continue;
		}
	}

	if ( roamGoals.Num() == 0 ) { //mal: if this happens, then we are totally hosed, or theres no goals on the map at all.
		if ( botThreadData.AllowDebugData() ) {
			botThreadData.Printf("^1No goals available for bot %i!!!\n", botNum );
		}
		return false;
	}

	if ( roamGoals.Num() > 0 ) {
		if ( roamGoals.Num() == 1 ) {
			actionNum = roamGoals[ 0 ];
		} else {
			j = botThreadData.random.RandomInt( roamGoals.Num() );
			actionNum = roamGoals[ j ];
		}

		ltgUseVehicle = Bot_ShouldUseVehicleForAction( actionNum, true );

		if ( ltgUseVehicle ) {
			if ( botVehicleInfo != NULL ) {
				if ( botVehicleInfo->driverEntNum == botNum ) {
                    V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
					V_LTG_AI_SUB_NODE = &idBotAI::Enter_VLTG_RoamGoal;
				} else {
					Bot_ExitVehicle();
				}
			}
		} else {
			if ( botVehicleInfo != NULL ) { //mal: if we're close to the goal, or theres some other reason not to use a vehicle for this goal, then exit our vehicle
                Bot_ExitVehicle();
			}
		}

  		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
		ltgTime = botWorld->gameLocalInfo.time + 30000;
		LTG_AI_SUB_NODE = &idBotAI::Enter_LTG_RoamGoal;

		if ( !botInfo->isDisguised ) {
			if ( !ltgUseVehicle ) {
				Bot_FindRouteToCurrentGoal();	
			}

			PushAINodeOntoStack( -1, -1, actionNum, ltgTime, false, ltgUseVehicle, ( routeNode != NULL ) ? true : false );
		}

		return true;
	}

	return false;
}
