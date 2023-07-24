// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idBotAI::SetupDebugHud

This is just a simple debug "hud" - printing the different values of a bot's AI onto my hud area, so that I can
see what they're thinking about.
================
*/
void idBotAI::SetupDebugHud() {
	if ( botInfo->justSpawned || botInfo->classType == NOCLASS || botInfo->team == NOTEAM ) {
		return;
	}

	if ( aiState == LTG ) {
		botUcmd->debugInfo.aiState = "LTG";
	} else if ( aiState == NBG ) {
		botUcmd->debugInfo.aiState = "NBG";
	} else if ( aiState == COMBAT ) {
		botUcmd->debugInfo.aiState = "COM";
	} else if ( aiState == VLTG ) {
		botUcmd->debugInfo.aiState = "VBG";
	} else if ( aiState == VCOMBAT ) {
		botUcmd->debugInfo.aiState = "COM";
	} else {
		botUcmd->debugInfo.aiState = "???";
	}

	botUcmd->debugInfo.target = -1;

	if ( aiState == LTG ) {
		botUcmd->debugInfo.target = ltgTarget;
	} else if ( aiState == NBG ) {
		botUcmd->debugInfo.target = nbgTarget;
	}

	botUcmd->debugInfo.enemy = enemy;

	if ( enemy != -1 ) {
		idVec3 vec = botWorld->clientInfo[ enemy ].origin - botInfo->origin;
		botUcmd->debugInfo.entDist = vec.LengthFast();
	} else {
		botUcmd->debugInfo.entDist = -1.0f;  
	}

	if ( routeNode == NULL ) {
		botUcmd->debugInfo.routeNum = -1;
	} else {
		botUcmd->debugInfo.routeNum = routeNode->num;
	}

	if ( botVehiclePathList.Num() > 0 ) {
		botUcmd->debugInfo.vehicleNodeNum = botVehiclePathList[ botVehiclePathList.Num() - 1 ].node->num;
	} else {
		botUcmd->debugInfo.vehicleNodeNum = -1;
	}

	botUcmd->debugInfo.aiNode = lastAINode;
	botUcmd->debugInfo.moveNode = lastMoveNode;
	botUcmd->debugInfo.actionNumber = actionNum;
	botUcmd->debugInfo.botClientNum = botNum;
	botUcmd->debugInfo.tacticalActionNum = tacticalActionNum;

	if ( botVehicleInfo == NULL ) {
		botUcmd->debugInfo.aasAreaNum = botInfo->areaNum;
	} else {
		botUcmd->debugInfo.aasAreaNum = botInfo->areaNumVehicle;
	}

	if ( nextMissionUpdateTime > botWorld->gameLocalInfo.time ) {
		return;
	}

	nextMissionUpdateTime = botWorld->gameLocalInfo.time + 1000;

	botUcmd->debugInfo.botGoalTypeTarget = botUcmd->debugInfo.target;

	if ( Client_IsCriticalForCurrentObj( botNum, -1.0f ) ) {
		botUcmd->debugInfo.botGoalType = DO_OBJECTIVE;
	} else if ( aiState == COMBAT || aiState == VCOMBAT ) {
		if ( ClientIsValid( enemy, -1 ) ) {
			const clientInfo_t& player = botWorld->clientInfo[ enemy ];
			if ( player.proxyInfo.entNum == botWorld->botGoalInfo.botGoal_MCP_VehicleNum && botWorld->botGoalInfo.botGoal_MCP_VehicleNum != -1 ) {
				botUcmd->debugInfo.botGoalType = ATTACK_MCP;
			}
		}
	} else if ( aiState == LTG ) {
		if ( ltgType == DEFENSE_CAMP_GOAL ) {
			botUcmd->debugInfo.botGoalType = DEFEND_AREA;
		} else if ( ltgType == MINE_GOAL ) {
			botUcmd->debugInfo.botGoalType = PLANT_MINES;
		} else if ( ltgType == FDA_GOAL ) {
			botUcmd->debugInfo.botGoalType = GRAB_FORWARD_SPAWN;

			if ( Bot_CheckActionIsValid( actionNum ) ) {
				botUcmd->debugInfo.botGoalTypeTarget = botThreadData.botActions[ actionNum ]->spawnControllerEntityNum;
			}
		} else if ( ltgType == DROP_DEPLOYABLE_GOAL ) {
			if ( botInfo->classType == COVERTOPS ) {
				botUcmd->debugInfo.botGoalType = GO_DEPLOY_RADAR;
			} else if ( botInfo->classType == FIELDOPS ) {
				botUcmd->debugInfo.botGoalType = GO_DEPLOY_FIRESUPPORT;
			} else {
				botUcmd->debugInfo.botGoalType = GO_DEPLOY_DEPLOYABLE;
			}
		} else if ( ltgType == DESTROY_DEPLOYABLE_GOAL ) {
			botUcmd->debugInfo.botGoalType = GO_DESTROY_DEPLOYABLE;
		} else if ( ltgType == HACK_DEPLOYABLE_GOAL ) {
			deployableInfo_t deployable;
			if ( GetDeployableInfo( false, ltgTarget, deployable ) ) {
				if ( deployable.type == AIT ) {
					botUcmd->debugInfo.botGoalType = GO_HACK_SHIELD;
				} else {
					botUcmd->debugInfo.botGoalType = GO_HACK_DEPLOYABLE;
				}
			} else {
				botUcmd->debugInfo.botGoalType = GO_HACK_DEPLOYABLE;
			}
		} else if ( ltgType == STEAL_SPAWN_GOAL ) {
			if ( botInfo->classType != COVERTOPS ) {
				botUcmd->debugInfo.botGoalType = LIBERATE_FORWARD_SPAWN;

				if ( Bot_CheckActionIsValid( actionNum ) ) {
					botUcmd->debugInfo.botGoalTypeTarget = botThreadData.botActions[ actionNum ]->spawnControllerEntityNum;
				}
			} else {
				botUcmd->debugInfo.botGoalType = LIBERATE_FORWARD_SPAWN_COVERTOPS;

				if ( Bot_CheckActionIsValid( actionNum ) ) {
					botUcmd->debugInfo.botGoalTypeTarget = botThreadData.botActions[ actionNum ]->spawnControllerEntityNum;
				}
			}
		} else if ( ltgType == STEAL_GOAL ) {
			botUcmd->debugInfo.botGoalType = GRAB_CARRYABLE;
		} else if ( ltgType == DELIVER_GOAL ) {
			botUcmd->debugInfo.botGoalType = DELIVER_CARRYABLE;
		} else if ( ltgType == RECOVER_GOAL ) {
			botUcmd->debugInfo.botGoalType = RETURN_CARRYABLE;
		} else if ( ltgType == FIX_MCP ) {
			botUcmd->debugInfo.botGoalType = REPAIR_MCP_GOAL;
		} else if ( ltgType == HUNT_GOAL ) {
			if ( ClientIsValid( ltgTarget, -1 ) ) {
				const clientInfo_t& player = botWorld->clientInfo[ ltgTarget ];

				if ( player.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
					botUcmd->debugInfo.botGoalType = DESTROY_VEHICLE;
				} else { 
					botUcmd->debugInfo.botGoalType = KILL_PLAYER;
				}
			} else {
				botUcmd->debugInfo.botGoalType = KILL_PLAYER;
			}
		} else if ( ltgType == MINOR_BUILD_GOAL ) {
			botUcmd->debugInfo.botGoalType = CONTRUCT_TOWER;
		} else if ( ltgType == MINOR_BUILD_MG ) {
			botUcmd->debugInfo.botGoalType = CONSTRUCT_MG_NEST;
		} else if ( ltgType == FIRESUPPORT_CAMP_GOAL ) {
			botUcmd->debugInfo.botGoalType = SEND_FIRESUPPORT;
		}
	} else if ( aiState == NBG ) {
		if ( nbgType == REVIVE_TEAMMATE || nbgType == TK_REVIVE_TEAMMATE ) {
			botUcmd->debugInfo.botGoalType = REVIVE_SOMEONE;
		} else if ( nbgType == SUPPLY_TEAMMATE ) {
			if ( botInfo->classType == MEDIC ) {
				botUcmd->debugInfo.botGoalType = HEAL_SOMEONE;
			} else {
				botUcmd->debugInfo.botGoalType = SUPPLY_AMMO;
			}
		} else if ( nbgType == CREATE_SPAWNHOST ) {
			botUcmd->debugInfo.botGoalType = SPAWNHOST_BODY;
		} else if ( nbgType == DESTORY_SPAWNHOST ) {
			botUcmd->debugInfo.botGoalType = DESTROY_STROGG_SPAWNHOST;
		} else if ( nbgType == STALK_VICTIM ) {
			botUcmd->debugInfo.botGoalType = KILL_PLAYER;
		} else if ( nbgType == FIX_DEPLOYABLE ) {
			botUcmd->debugInfo.botGoalType = REPAIR_DEPLOYABLE;
		} else if ( nbgType == FIX_VEHICLE ) {
			botUcmd->debugInfo.botGoalType = REPAIR_VEHICLE;
		}
	}
}
