// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "BotThreadData.h"
#include "BotAI_Main.h"


/*
================
idBotAI::VThink

This is the start of the bot's thinking process when its in a vehicle, called
every game frame by idBotAI::Think.
================
*/
void idBotAI::VThink() {

	if ( V_ROOT_AI_NODE == NULL ) {		
        V_ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
	}

	if ( botAAS.blockedByObstacleCounterInVehicle > MAX_FRAMES_BLOCKED_BY_OBSTACLE_IN_VEHICLE ) {
		Bot_ResetState( true, true );
		botAAS.blockedByObstacleCounterInVehicle = 0;
		ROOT_AI_NODE = &idBotAI::Run_VLTG_Node;
	}

    CallFuncPtr( V_ROOT_AI_NODE ); //mal: run the bot's current think node

	CheckBotStuckState();

	Bot_CheckDelayedChats();

	if ( vehicleAINodeSwitch.nodeSwitchTime < botWorld->gameLocalInfo.time ) {
		vehicleAINodeSwitch.nodeSwitchTime = botWorld->gameLocalInfo.time + 5000;
		if ( vehicleAINodeSwitch.nodeSwitchCount > MAX_AI_NODE_SWITCH ) {
			Bot_IgnoreVehicle( botVehicleInfo->entNum, 5000 );
			Bot_ExitVehicle();
			vehicleAINodeSwitch.nodeSwitchCount = 0;
		} else {
			vehicleAINodeSwitch.nodeSwitchCount = 0;
		}
	}

	if ( vehiclePauseTime > botWorld->gameLocalInfo.time ) {
		botUcmd->specialMoveType = FULL_STOP;
	}

	if ( vehicleReverseTime > botWorld->gameLocalInfo.time ) {
		botUcmd->specialMoveType = REVERSEMOVE;
	}

	if ( botVehicleInfo->type != MCP && HumanVehicleOwnerNearby( botInfo->team, botVehicleInfo->origin, HUMAN_OWN_VEHICLE_DIST, botVehicleInfo->spawnID ) ) {
		botUcmd->specialMoveType = FULL_STOP; //mal: don't move if the human who own(ed) this vehicle is nearby, but still look and shoot.
	}

	if ( botVehicleInfo->type != MCP && botVehicleInfo->isBoobyTrapped ) {
		botUcmd->botCmds.exitVehicle = true;
	}

	if ( botVehicleInfo->type != MCP && Bot_CheckFriendlyEngineerIsNearbyOurVehicle() == true ) {
		botUcmd->specialMoveType = FULL_STOP; //mal: don't move if there is someone out there ( bot or human ) who is trying to fix our ride
	}

	if ( botInfo->lastRoadKillTime > botWorld->gameLocalInfo.time ) {
		if ( checkRoadKillTime < botWorld->gameLocalInfo.time ) {
			Bot_AddDelayedChat( botNum, GLOBAL_SORRY, 1 ); //mal: say a nice sorry to the poor enemy sap you ran over
			checkRoadKillTime = botWorld->gameLocalInfo.time + 10000;
		}
		botUcmd->botCmds.honkHorn = true;
	}
}

