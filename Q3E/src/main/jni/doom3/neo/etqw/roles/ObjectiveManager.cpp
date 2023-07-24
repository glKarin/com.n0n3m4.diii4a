// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "ObjectiveManager.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../structures/TeamManager.h"
#include "../Player.h"
#include "../rules/GameRules.h"
#include "../rules/GameRules_Campaign.h"

#include "../botai/BotThreadData.h"
#include "../botai/BotAI_Actions.h"

idCVar g_logObjectives( "g_logObjectives", "1", CVAR_BOOL | CVAR_GAME | CVAR_NOCHEAT | CVAR_RANKLOCKED, "log objective completion info" );

/*
===============================================================================

	sdObjectiveObject

===============================================================================
*/

/*
================
sdObjectiveObject::sdObjectiveObject
================
*/
sdObjectiveObject::sdObjectiveObject( void ) {
}

/*
================
sdObjectiveObject::Init
================
*/
void sdObjectiveObject::Init( int numTeams ) {
	teamInfo.SetGranularity( 1 );
	teamInfo.SetNum( numTeams );

	for ( int i = 0; i < teamInfo.Num(); i++ ) {
		teamInfo[ i ].iconName	= "";
		teamInfo[ i ].description = NULL;
		teamInfo[ i ].state		= -1;
	}
}

/*
================
sdObjectiveObject::SetState
================
*/
void sdObjectiveObject::SetState( int teamIndex, int state ) {
	if ( teamIndex < 0 || teamIndex >= teamInfo.Num() ) {
		return;
	}

	teamInfo[ teamIndex ].state = state;
}

/*
================
sdObjectiveObject::SetIcon
================
*/
void sdObjectiveObject::SetIcon( int teamIndex, const char* icon ) {
	if ( teamIndex < 0 || teamIndex >= teamInfo.Num() ) {
		return;
	}

	teamInfo[ teamIndex ].iconName = icon;
}

/*
================
sdObjectiveObject::GetState
================
*/
int sdObjectiveObject::GetState( int teamIndex ) const {
	if ( teamIndex < 0 || teamIndex >= teamInfo.Num() ) {
		return -1;
	}

	return teamInfo[ teamIndex ].state;
}

/*
================
sdObjectiveObject::SetDescription
================
*/
void sdObjectiveObject::SetDescription( int teamIndex, const sdDeclLocStr* description ) {
	if ( teamIndex < 0 || teamIndex >= teamInfo.Num() ) {
		return;
	}

	teamInfo[ teamIndex ].description = description;
}

/*
================
sdObjectiveObject::GetDescription
================
*/
int sdObjectiveObject::GetDescription( int teamIndex ) const {
	if ( teamIndex < 0 || teamIndex >= teamInfo.Num() ) {
		return -1;
	}
	if( teamInfo[ teamIndex ].description == NULL ) {
		return -1;
	}

	return teamInfo[ teamIndex ].description->Index();
}


/*
===============================================================================

	sdObjectiveManagerLocal

===============================================================================
*/

extern const idEventDef EV_SendNetworkEvent;

const idEventDef EV_ObjManager_SetObjectiveState( "setObjectiveState", '\0', DOC_TEXT( "Sets the state of the given objective, for the specified team." ), 3, NULL, "o", "team", "Team to set the state for.", "d", "index", "Index of the objective to set the state of.", "d", "state", "Value to set the state to." );
const idEventDef EV_ObjManager_SetObjectiveIcon( "setObjectiveIcon", '\0', DOC_TEXT( "Sets the material to be used by the given objective, for the specified team." ), 3, NULL, "o", "team", "Team to set the icon for.", "d", "index", "Index of the objective to set the icon of.", "s", "material", "Name of the $decl:material$ to set." );
const idEventDef EV_ObjManager_GetObjectiveState( "getObjectiveState", 'd', DOC_TEXT( "Returns the state of the given objective, for the specified team." ), 2, "If the index is out of range, or the object passed is not a team, the result will be -1.", "o", "team", "Team to return the state for.", "d", "index", "Index of the objective to get the state for." );
const idEventDef EV_ObjManager_SetNextObjective( "setNextObjective", '\0', DOC_TEXT( "Sets the next objective index for the specified team." ), 2, "Index may be -1 to indicate no objective.", "o", "team", "Team to set the next objective for.", "d", "index", "Index of the next objective to set." );
const idEventDef EV_ObjManager_GetNextObjective( "getNextObjective", 'd', DOC_TEXT( "Returns the index of the next objective for the specified team." ), 1, "If the object passed in is not a valid team, the result will be -1.", "o", "team", "Team to return the next objective index of." );
const idEventDef EV_ObjManager_SetShortDescription( "setShortDescription", '\0', DOC_TEXT( "Sets the localized text to use for the description of an objective for the specified team." ), 3, NULL, "o", "team", "Team to set the text for.", "d", "index", "Index of the objective to set the text for.", "h", "text", "Handle to the $decl:locStr$ to set." );
const idEventDef EV_ObjManager_GetShortDescription( "getShortDescription", 'h', DOC_TEXT( "Returns the localized text for an objective, for the specified team." ), 2, "If the index is out of range, or the object is not a valid team, the result will be -1.", "o", "team", "Team to get the text for.", "d", "index", "Index of the objective to get the text for." );
const idEventDef EV_ObjManager_CreateMapScript( "createMapScript", 'o', DOC_TEXT( "Allocates a script object for the map script, and returns it." ), 0, "The result will be $null$ if the map script object cannot be created." );
const idEventDef EV_ObjManager_LogObjectiveCompletion( "logObjectiveCompletion", '\0', DOC_TEXT( "Logs player class and XP information against this objective completion." ), 1, NULL, "d", "index", "Index of the objective which was completed." );

//mal: bot specific script cmds
const idEventDef EV_ObjManager_DeactivateBotActionGroup( "deactivateBotActionGroup", "d" );
const idEventDef EV_ObjManager_ActivateBotActionGroup( "activateBotActionGroup", "d" );
const idEventDef EV_ObjManager_SetBotActionStateForEvent( "setBotActionStateForEvent", "de" );
const idEventDef EV_ObjManager_DeactivateBotAction( "deactivateBotAction", "s" );
const idEventDef EV_ObjManager_ActivateBotAction( "activateBotAction", "s" );
const idEventDef EV_ObjManager_SetBotCriticalClass( "setBotCriticalClass", "dd" ); //mal: who do we love?
const idEventDef EV_ObjManager_NotifyBotOfEvent( "botUpdateForEvent", "ddd" );
const idEventDef EV_ObjManager_SetAttackingTeam( "setAttackingTeam", "d" );
const idEventDef EV_ObjManager_SetPrimaryAction( "setPrimaryTeamAction", "ds" );
const idEventDef EV_ObjManager_SetSecondaryction( "setSecondaryTeamAction", "ds" );
const idEventDef EV_ObjManager_SetTeamNeededClass( "setTeamNeededClass", "ddddbb" );
const idEventDef EV_ObjManager_SetBotActionVehicleType( "setBotActionVehicleType", "sd" );
const idEventDef EV_ObjManager_SetBotActionGroupVehicleType( "setBotActionGroupVehicleType", "dd" );
const idEventDef EV_ObjManager_SetBotSightDist( "setBotSightDist", "f" );
const idEventDef EV_ObjManager_EnableRouteGroup( "enableRouteGroup", "f" );
const idEventDef EV_ObjManager_DisableRouteGroup( "disableRouteGroup", "f" );
const idEventDef EV_ObjManager_DisableRoute( "disableRoute", "s" );
const idEventDef EV_ObjManager_EnableRoute( "enableRoute", "s" );
const idEventDef EV_ObjManager_SetMapHasMCPGoal( "setMapHasMCPGoal", "b" );
const idEventDef EV_ObjManager_SetSpawnActionOwner( "setSpawnActionOwner", "de" );
const idEventDef EV_ObjManager_SetActionObjState( "setActionObjState", "ddee" );
const idEventDef EV_ObjManager_DisableNodes( "disableNode", "s" );
const idEventDef EV_ObjManager_EnableNodes( "enableNode", "s" );
const idEventDef EV_ObjManager_TeamSuicideIfNotNearAction( "teamSuicideIfNotNearAction", "sdd" );
const idEventDef EV_ObjManager_IsActionGroupActive( "isActionGroupActive", "d", 'f' );
const idEventDef EV_ObjManager_IsActionActive( "isActionActive", "s", 'f' );
const idEventDef EV_ObjManager_SwitchTeamWeapons( "switchTeamWeapons", "ddddb" );
const idEventDef EV_ObjManager_KillBotActionGroup( "killBotActionGroup", "d" );
const idEventDef EV_ObjManager_KillBotAction( "killBotAction", "s" );
const idEventDef EV_ObjManager_SetTeamUseRearSpawn( "setTeamUseRearSpawn", "db" );
const idEventDef EV_ObjManager_SetTeamUseRearSpawnPercentage( "setTeamUseRearSpawnPercentage", "dd" );
const idEventDef EV_ObjManager_GetNumBotsOnTeam( "getNumBotsOnTeam", "d", 'f' );
const idEventDef EV_ObjManager_SetActionPriority( "setActionPriority", "sb" );
const idEventDef EV_ObjManager_SetTeamAttacksDeployables( "setTeamAttacksDeployables", "db" );
const idEventDef EV_ObjManager_SetActionHumanGoal( "setActionHumanGoal", "sd" );
const idEventDef EV_ObjManager_SetActionStroggGoal( "setActionStroggGoal", "sd" );
const idEventDef EV_ObjManager_ClearBotBoundEntities( "clearTeamBotBoundEntities", "d" );
const idEventDef EV_ObjManager_SetBotTeamRetreatTime( "setBotTeamRetreatTime", "dd" );
const idEventDef EV_ObjManager_SetNodeTeam( "setNodeTeam", "sd" );
const idEventDef EV_ObjManager_SetTeamMinePlantIsPriority( "setTeamMinePlantIsPriority", "db" );
const idEventDef EV_ObjManager_DisableAASAreaInLocation( "disableAASAreaInLocation", "fv" );
const idEventDef EV_ObjManager_GameIsOnFinalObjective( "gameIsOnFinalObjective" );
const idEventDef EV_ObjManager_MapIsTraingingMap( "mapIsTraingingMap" );
const idEventDef EV_ObjManager_SetActorPrimaryAction( "setActorPrimaryAction", "sbd" );
const idEventDef EV_ObjManager_SetBriefingPauseTime( "setBriefingPauseTime", "d" );
const idEventDef EV_ObjManager_SetPlayerIsOnFinalMission( "setPlayerIsOnFinalMission" );
const idEventDef EV_ObjManager_SetTrainingBotsCanGrabForwardSpawns( "setTrainingBotsCanGrabForwardSpawns" );

const idEventDef EV_ObjManager_GetBotCriticalClass( "getBotCriticalClass", "d", 'd' );
const idEventDef EV_ObjManager_GetNumClassPlayers( "getNumClassPlayers", "dd", 'd' );

CLASS_DECLARATION( idClass, sdObjectiveManagerLocal )
	EVENT( EV_ObjManager_SetObjectiveState,			sdObjectiveManagerLocal::Event_SetObjectiveState )
	EVENT( EV_ObjManager_GetObjectiveState,			sdObjectiveManagerLocal::Event_GetObjectiveState )
	EVENT( EV_ObjManager_SetObjectiveIcon,			sdObjectiveManagerLocal::Event_SetObjectiveIcon )
	EVENT( EV_SendNetworkEvent,						sdObjectiveManagerLocal::Event_SendNetworkEvent )
	EVENT( EV_ObjManager_SetNextObjective,			sdObjectiveManagerLocal::Event_SetNextObjective )
	EVENT( EV_ObjManager_GetNextObjective,			sdObjectiveManagerLocal::Event_GetNextObjective )
	EVENT( EV_ObjManager_SetShortDescription,		sdObjectiveManagerLocal::Event_SetShortDescription )
	EVENT( EV_ObjManager_GetShortDescription,		sdObjectiveManagerLocal::Event_GetShortDescription )
	EVENT( EV_ObjManager_CreateMapScript,			sdObjectiveManagerLocal::Event_CreateMapScript )
	EVENT( EV_ObjManager_LogObjectiveCompletion,	sdObjectiveManagerLocal::Event_LogObjectiveCompletion )

	EVENT( EV_ObjManager_DeactivateBotActionGroup,  sdObjectiveManagerLocal::Event_DeactivateBotActionGroup )
	EVENT( EV_ObjManager_ActivateBotActionGroup,    sdObjectiveManagerLocal::Event_ActivateBotActionGroup )
	EVENT( EV_ObjManager_SetBotActionStateForEvent,	sdObjectiveManagerLocal::Event_SetBotActionStateForEvent )
	EVENT( EV_ObjManager_DeactivateBotAction,		sdObjectiveManagerLocal::Event_DeactivateBotAction )
	EVENT( EV_ObjManager_ActivateBotAction,			sdObjectiveManagerLocal::Event_ActivateBotAction )
	EVENT( EV_ObjManager_SetBotCriticalClass,		sdObjectiveManagerLocal::Event_SetBotCriticalClass )
	EVENT( EV_ObjManager_NotifyBotOfEvent,			sdObjectiveManagerLocal::Event_NotifyBotOfEvent )
	EVENT( EV_ObjManager_SetAttackingTeam,			sdObjectiveManagerLocal::Event_SetAttackingTeam )
	EVENT( EV_ObjManager_SetPrimaryAction,			sdObjectiveManagerLocal::Event_SetPrimaryAction )
	EVENT( EV_ObjManager_SetSecondaryction,			sdObjectiveManagerLocal::Event_SetSecondaryAction )
	EVENT( EV_ObjManager_SetTeamNeededClass,		sdObjectiveManagerLocal::Event_SetTeamNeededClass )
	EVENT( EV_ObjManager_SetBotSightDist,			sdObjectiveManagerLocal::Event_SetBotSightDist )
	EVENT( EV_ObjManager_EnableRouteGroup,			sdObjectiveManagerLocal::Event_EnableRouteGroup )
	EVENT( EV_ObjManager_DisableRouteGroup,			sdObjectiveManagerLocal::Event_DisableRouteGroup )
	EVENT( EV_ObjManager_DisableRoute,				sdObjectiveManagerLocal::Event_DisableRoute )
	EVENT( EV_ObjManager_EnableRoute,				sdObjectiveManagerLocal::Event_EnableRoute )
	EVENT( EV_ObjManager_SetMapHasMCPGoal,			sdObjectiveManagerLocal::Event_SetMapHasMCPGoal )
	EVENT( EV_ObjManager_SetSpawnActionOwner,		sdObjectiveManagerLocal::Event_SetSpawnActionOwner )
	EVENT( EV_ObjManager_SetActionObjState,			sdObjectiveManagerLocal::Event_SetActionObjState )
	EVENT( EV_ObjManager_DisableNodes,				sdObjectiveManagerLocal::Event_DisableNodes )
	EVENT( EV_ObjManager_EnableNodes,				sdObjectiveManagerLocal::Event_EnableNodes )
	EVENT( EV_ObjManager_TeamSuicideIfNotNearAction,sdObjectiveManagerLocal::Event_TeamSuicideIfNotNearAction )
	EVENT( EV_ObjManager_IsActionGroupActive,		sdObjectiveManagerLocal::Event_IsActionGroupActive )
	EVENT( EV_ObjManager_IsActionActive,			sdObjectiveManagerLocal::Event_IsActionActive )
	EVENT( EV_ObjManager_SwitchTeamWeapons,			sdObjectiveManagerLocal::Event_SwitchTeamWeapons )
	EVENT( EV_ObjManager_KillBotActionGroup,		sdObjectiveManagerLocal::Event_KillBotActionGroup )
	EVENT( EV_ObjManager_KillBotAction,				sdObjectiveManagerLocal::Event_KillBotAction )
	EVENT( EV_ObjManager_SetTeamUseRearSpawn,		sdObjectiveManagerLocal::Event_SetTeamUseRearSpawn )
	EVENT( EV_ObjManager_GetNumBotsOnTeam,			sdObjectiveManagerLocal::Event_GetNumBotsOnTeam )
	EVENT( EV_ObjManager_SetActionPriority,			sdObjectiveManagerLocal::Event_SetActionPriority )
	EVENT( EV_ObjManager_SetTeamAttacksDeployables, sdObjectiveManagerLocal::Event_SetTeamAttacksDeployables )
	EVENT( EV_ObjManager_SetActionHumanGoal,		sdObjectiveManagerLocal::Event_SetActionHumanGoal )
	EVENT( EV_ObjManager_SetActionStroggGoal,		sdObjectiveManagerLocal::Event_SetActionStroggGoal )
	EVENT( EV_ObjManager_ClearBotBoundEntities,		sdObjectiveManagerLocal::Event_ClearBotBoundEntities )
	EVENT( EV_ObjManager_SetBotTeamRetreatTime,		sdObjectiveManagerLocal::Event_SetBotTeamRetreatTime )
	EVENT( EV_ObjManager_SetTeamUseRearSpawnPercentage, sdObjectiveManagerLocal::Event_SetTeamUseRearSpawnPercentage )
	EVENT( EV_ObjManager_SetNodeTeam,				sdObjectiveManagerLocal::Event_SetNodeTeam )
	EVENT( EV_ObjManager_SetTeamMinePlantIsPriority,	sdObjectiveManagerLocal::Event_SetTeamMinePlantIsPriority )
	EVENT( EV_ObjManager_SetBotActionVehicleType,		sdObjectiveManagerLocal::Event_SetBotActionVehicleType )
	EVENT( EV_ObjManager_SetBotActionGroupVehicleType,	sdObjectiveManagerLocal::Event_SetBotActionGroupVehicleType )
	EVENT( EV_ObjManager_DisableAASAreaInLocation,		sdObjectiveManagerLocal::Event_DisableAASAreaInLocation )
	EVENT( EV_ObjManager_GameIsOnFinalObjective,		sdObjectiveManagerLocal::Event_GameIsOnFinalObjective )
	EVENT( EV_ObjManager_MapIsTraingingMap,				sdObjectiveManagerLocal::Event_MapIsTrainingMap )
	EVENT( EV_ObjManager_SetActorPrimaryAction,			sdObjectiveManagerLocal::Event_SetActorPrimaryAction )
	EVENT( EV_ObjManager_SetBriefingPauseTime,			sdObjectiveManagerLocal::Event_SetBriefingPauseTime )
	EVENT( EV_ObjManager_SetPlayerIsOnFinalMission,		sdObjectiveManagerLocal::Event_SetPlayerIsOnFinalMission )
	EVENT( EV_ObjManager_SetTrainingBotsCanGrabForwardSpawns, sdObjectiveManagerLocal::Event_SetTrainingBotsCanGrabForwardSpawns )
	EVENT( EV_ObjManager_GetBotCriticalClass,			sdObjectiveManagerLocal::Event_GetBotCriticalClass )
	EVENT( EV_ObjManager_GetNumClassPlayers,			sdObjectiveManagerLocal::Event_GetNumClassPlayers )
END_CLASS

/*
================
sdObjectiveManagerLocal::sdObjectiveManagerLocal
================
*/
sdObjectiveManagerLocal::sdObjectiveManagerLocal( void ) : scriptObject( NULL ) {
	onCarryableItemStolenFunc = NULL;
	onCarryableItemReturnedFunc = NULL;
	onSpawnCapturedFunc = NULL;
	onSpawnLiberatedFunc = NULL;
	getSpectateEntityFunc = NULL;
	onDeployableDeployedFunc = NULL;
	gdfCriticalClass = NOCLASS;
	stroggCriticalClass = NOCLASS;
}

/*
================
sdObjectiveManagerLocal::Init
================
*/
void sdObjectiveManagerLocal::Init( void ) {
	Shutdown();

	sdTeamManagerLocal& teamManager = sdTeamManager::GetInstance();

	objectives.SetNum( MAX_OBJECTIVES );
	int numTeams = teamManager.GetNumTeams();
	for ( int i = 0; i < MAX_OBJECTIVES; i++ ) {
		objectives[ i ].Init( numTeams );
	}

	nextObjective.SetNum( numTeams );
	for ( int i = 0; i < numTeams; i++ ) {
		nextObjective[ i ] = -1;
	}
	
	SetCriticalClass( GDF, NOCLASS );
	SetCriticalClass( STROGG, NOCLASS );
}

/*
================
sdObjectiveManagerLocal::OnNewScriptLoad
================
*/
void sdObjectiveManagerLocal::OnNewScriptLoad( void ) {
	const char* objectiveManagerObject = gameLocal.GetMapInfo().GetData().GetString( "script_objective_manager" );
	if ( !*objectiveManagerObject ) {
		objectiveManagerObject = "objectiveManager";
	}

	scriptObject = gameLocal.program->AllocScriptObject( this, objectiveManagerObject );

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetPreConstructor(), h1 );

	const sdProgram::sdFunction* constructor = scriptObject->GetConstructor();
	if ( constructor ) {
		sdProgramThread* scriptThread = gameLocal.program->CreateThread();
		scriptThread->SetName( "sdObjectiveManagerLocal" );
		scriptThread->CallFunction( scriptObject, constructor );
		scriptThread->DelayedStart( 0 );
		scriptThread->GetAutoNode().AddToEnd( scriptObject->GetAutoThreads() );
	}

	onCarryableItemStolenFunc = scriptObject->GetFunction( "OnCarryableItemStolen" );
	onCarryableItemReturnedFunc = scriptObject->GetFunction( "OnCarryableItemReturned" );
	onSpawnCapturedFunc = scriptObject->GetFunction( "OnSpawnCaptured" );
	onSpawnLiberatedFunc = scriptObject->GetFunction( "OnSpawnLiberated" );
	getSpectateEntityFunc = scriptObject->GetFunction( "GetSpectateEntity" );
	onDeployableDeployedFunc = scriptObject->GetFunction( "OnDeployableDeployed" );
}

/*
================
sdObjectiveManagerLocal::OnScriptChange
================
*/
void sdObjectiveManagerLocal::OnScriptChange( void ) {
	if ( scriptObject ) {
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetDestructor(), h1 );

		gameLocal.program->FreeScriptObject( scriptObject );
	}
}

/*
================
sdObjectiveManagerLocal::OnScriptChange
================
*/
void sdObjectiveManagerLocal::GetRealTime( idStr& text ) {
	text = va( "Real Time: %s Game Time Left: %s", gameLocal.GetTimeText(), idStr::MS2HMS( gameLocal.rules->GetGameTime() ) );
}

/*
================
sdObjectiveManagerLocal::LogPlayerStats
================
*/
void sdObjectiveManagerLocal::LogPlayerStats( void ) {
	int playerCount = 0;

	sdTeamManagerLocal& manager = sdTeamManager::GetInstance();
	for ( int i = 0; i < manager.GetNumTeams(); i++ ) {
		sdTeamInfo& team = manager.GetTeamByIndex( i );
		gameLocal.LogObjective( va( "Team: %s\n", team.GetLookupName() ) );

		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = gameLocal.GetClient( i );
			if ( player == NULL ) {
				continue;
			}

			if ( player->GetGameTeam() != &team ) {
				continue;
			}

			const sdDeclPlayerClass* cls = player->GetInventory().GetClass();
			gameLocal.LogObjective( va( "Player: '%s' Class '%s'\n", player->userInfo.cleanName.c_str(), cls != NULL ? cls->GetName() : "none" ) );
			playerCount++;

			for ( int j = 0; j < gameLocal.declProficiencyTypeType.Num(); j++ ) {
				float value = player->GetProficiencyTable().GetPoints( j );
				const char* name = gameLocal.declProficiencyTypeType[ j ]->GetName();
				
				gameLocal.LogObjective( va( "XP: %s %f\n", name, value ) );
			}
		}
	}

	gameLocal.LogObjective( va( "Total Players: %d\n", playerCount ) );
}

/*
================
sdObjectiveManagerLocal::OnMapStart
================
*/
void sdObjectiveManagerLocal::OnMapStart( void ) {
	if ( scriptObject ) {
		const sdProgram::sdFunction* callback = scriptObject->GetFunction( "OnMapStart" );
		if ( callback ) {
			sdProgramThread* scriptThread = gameLocal.program->CreateThread();
			scriptThread->SetName( "sdObjectiveManagerLocal" );
			scriptThread->CallFunction( scriptObject, callback );
			scriptThread->DelayedStart( 0 );
			scriptThread->GetAutoNode().AddToEnd( scriptObject->GetAutoThreads() );
		}
	}

	Think();
}

/*
================
sdObjectiveManagerLocal::OnMapShutdown
================
*/
void sdObjectiveManagerLocal::OnMapShutdown( void ) {
	if ( scriptObject != NULL ) {
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnMapShutdown" ), h1 );
	}
}

/*
================
sdObjectiveManagerLocal::OnLocalMapRestart
================
*/
void sdObjectiveManagerLocal::OnLocalMapRestart( void ) {
	if ( scriptObject != NULL ) {
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnLocalMapRestart" ), h1 );
	}
}


/*
================
sdObjectiveManagerLocal::OnGameStateChange
================
*/
void sdObjectiveManagerLocal::OnGameStateChange( int newState ) {
	if ( scriptObject != NULL ) {
		sdScriptHelper h1;
		h1.Push( newState );
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnGameStateChange" ), h1 );
	}

	if ( g_logObjectives.GetBool() && !gameLocal.isClient ) {
		if ( newState == sdGameRules::GS_GAMEON ) {
			idStr time;
			GetRealTime( time );

			gameLocal.LogObjective( "===============================================\n" );
			gameLocal.LogObjective( "-----------------------------------------------\n" );
			gameLocal.LogObjective( va( "Map Started: %s\n", time.c_str() ) );
			gameLocal.LogObjective( va( "Server: '%s'\n", si_name.GetString() ) );
			gameLocal.LogObjective( va( "Map: '%s'\n", gameLocal.GetMapName() ) );
			gameLocal.LogObjective( va( "Timelimit: %f mins\n", si_timeLimit.GetFloat() ) );
			gameLocal.LogObjective( va( "Ruleset: %s\n", gameLocal.rules->GetType()->classname ) );

			sdGameRulesCampaign* campaignRules = gameLocal.rules->Cast< sdGameRulesCampaign >();
			if ( campaignRules != NULL ) {
				const sdDeclCampaign* campaign = campaignRules->GetCampaign();
				assert( campaign != NULL );
				
				gameLocal.LogObjective( va( "Campaign: %s\n", campaign->GetName() ) );
			}
			LogPlayerStats();
			gameLocal.LogObjective( "-----------------------------------------------\n" );
		} else if ( newState == sdGameRules::GS_GAMEREVIEW ) {
			idStr time;
			GetRealTime( time );

			gameLocal.LogObjective( "-----------------------------------------------\n" );
			gameLocal.LogObjective( va( "Map Finished: %s\n", time.c_str() ) );
			sdTeamInfo* winningTeam = gameLocal.rules->GetWinningTeam();
			gameLocal.LogObjective( va( "Winning Team: %s\n", winningTeam != NULL ? winningTeam->GetLookupName() : "none" ) );
			LogPlayerStats();
			gameLocal.LogObjective( "-----------------------------------------------\n" );
			gameLocal.LogObjective( "===============================================\n\n" );


			idStr fileName = va( "logs/XP Log - %hs - %s.csv", gameLocal.mapMetaData->GetString( "pretty_name" ), gameLocal.GetTimeText() );
			fileName.ReplaceChar( ':', '-' );

			idFile* csvFile = fileSystem->OpenFileWrite( fileName.c_str(), "fs_userpath" );
			if ( csvFile != NULL ) {
				int* categoryTotals = ( int* )_alloca( gameLocal.declProficiencyTypeType.Num() * sizeof( int ) );
				int* categoryTotalsAll = ( int* )_alloca( gameLocal.declProficiencyTypeType.Num() * sizeof( int ) );
				int globalTotal = 0;
				int globalTotalAll = 0;

				csvFile->WriteFloatString( "Name" );
				for ( int i = 0; i < gameLocal.declProficiencyTypeType.Num(); i++ ) {
					categoryTotals[ i ] = 0;
					categoryTotalsAll[ i ] = 0;
					csvFile->WriteFloatString( ",%s", gameLocal.declProficiencyTypeType[ i ]->GetName() );
				}
				csvFile->WriteFloatString( ",Total\n" );

				for ( int i = 0; i < MAX_CLIENTS; i++ ) {
					idPlayer* player = gameLocal.GetClient( i );
					if ( player == NULL ) {
						continue;
					}

					int localTotal = 0;
					int localTotalAll = 0;

					csvFile->WriteFloatString( "\"%s\"", player->GetUserInfo().cleanName.c_str() );

					sdProficiencyTable& table = player->GetProficiencyTable();
					for ( int i = 0; i < gameLocal.declProficiencyTypeType.Num(); i++ ) {
						int value = ( int )table.GetPointsSinceBase( i );
						int valueAll = ( int )table.GetPoints( i );

						categoryTotals[ i ] += value;
						categoryTotalsAll[ i ] += valueAll;
						localTotal += value;
						localTotalAll += valueAll;
						globalTotal += value;
						globalTotalAll += valueAll;

						csvFile->WriteFloatString( ",%d(%d)", value, valueAll );
					}
					csvFile->WriteFloatString( ",%d(%d)\n", localTotal, localTotalAll );
				}

				csvFile->WriteFloatString( "Total" );
				for ( int i = 0; i < gameLocal.declProficiencyTypeType.Num(); i++ ) {
					csvFile->WriteFloatString( ",%d(%d)", categoryTotals[ i ], categoryTotalsAll[ i ] );
				}
				csvFile->WriteFloatString( ",%d(%d)", globalTotal, globalTotalAll );

				fileSystem->CloseFile( csvFile );
			}
		}
	}
}

/*
================
sdObjectiveManagerLocal::OnLocalViewPlayerTeamChanged
================
*/
void sdObjectiveManagerLocal::OnLocalViewPlayerTeamChanged( void ) {
}

/*
================
sdObjectiveManagerLocal::OnLocalViewPlayerChanged
================
*/
void sdObjectiveManagerLocal::OnLocalViewPlayerChanged( void ) {
	OnLocalViewPlayerTeamChanged();
}

/*
================
sdObjectiveManagerLocal::Shutdown
================
*/
void sdObjectiveManagerLocal::Shutdown( void ) {
	objectives.Clear();
}

/*
================
sdObjectiveManagerLocal::Think
================
*/
void sdObjectiveManagerLocal::Think( void ) {
}

/*
================
sdObjectiveManagerLocal::WriteInitialReliableMessages
================
*/
void sdObjectiveManagerLocal::WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) const {
	for ( int i = 0; i < nextObjective.Num(); i++ ) {
		sdTeamInfo& team = sdTeamManager::GetInstance().GetTeamByIndex( i );

		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_SETNEXTOBJECTIVE );
		sdTeamManager::GetInstance().WriteTeamToStream( &team, msg );
		msg.WriteLong( nextObjective[ i ] );
		msg.Send( target );
	}

	if ( scriptObject != NULL ) {
		assert( !target.SendToAll() );

		sdScriptHelper h1;
		h1.Push( target.GetClientNum() );
		h1.Push( target.SendToRepeaterClients() ? 1.f : 0.f );
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnWriteInitialReliableMessages" ), h1 );
	}

	{
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_SETCRITICALCLASS );
		msg.WriteChar( GDF );
		msg.WriteChar( gdfCriticalClass );
		msg.Send( target );
	}

	{
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_SETCRITICALCLASS );
		msg.WriteChar( STROGG );
		msg.WriteChar( stroggCriticalClass );
		msg.Send( target );
	}
}

/*
================
sdObjectiveManagerLocal::SetNextObjective
================
*/
void sdObjectiveManagerLocal::SetNextObjective( sdTeamInfo* team, int objectiveIndex ) {
	assert( team );

	nextObjective[ team->GetIndex() ] = objectiveIndex;

	sdScriptHelper h1;
	h1.Push( team->GetScriptObject() );
	h1.Push( objectiveIndex );
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnNextObjectiveSet" ), h1 );

	OnLocalViewPlayerTeamChanged();

	// if this is the server then send a message
	if ( gameLocal.isServer ) {
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_SETNEXTOBJECTIVE );
		sdTeamManager::GetInstance().WriteTeamToStream( team, msg );
		msg.WriteLong( objectiveIndex );
		msg.Send( sdReliableMessageClientInfoAll() );
	}
}

/*
================
sdObjectiveManagerLocal::Event_SetObjectiveState
================
*/
void sdObjectiveManagerLocal::Event_SetObjectiveState( idScriptObject* object, int objectiveIndex, int state ) {
	if ( objectiveIndex < 0 || objectiveIndex >= objectives.Num() ) {
		return;
	}

	sdTeamInfo* team = object ? object->GetClass()->Cast< sdTeamInfo >() : NULL;
	if ( !team ) {
		return;
	}

	objectives[ objectiveIndex ].SetState( team->GetIndex(), state );
}

/*
================
sdObjectiveManagerLocal::Event_SetObjectiveIcon
================
*/
void sdObjectiveManagerLocal::Event_SetObjectiveIcon( idScriptObject* object, int objectiveIndex, const char* icon ) {
	if ( objectiveIndex < 0 || objectiveIndex >= objectives.Num() ) {
		return;
	}

	sdTeamInfo* team = object ? object->GetClass()->Cast< sdTeamInfo >() : NULL;
	if ( !team ) {
		return;
	}

	objectives[ objectiveIndex ].SetIcon( team->GetIndex(), icon );
}

/*
================
sdObjectiveManagerLocal::Event_GetObjectiveState
================
*/
void sdObjectiveManagerLocal::Event_GetObjectiveState( idScriptObject* object, int objectiveIndex ) {
	if ( objectiveIndex < 0 || objectiveIndex >= objectives.Num() ) {
		sdProgram::ReturnInteger( -1 );
		return;
	}

	sdTeamInfo* team = object ? object->GetClass()->Cast< sdTeamInfo >() : NULL;
	if ( !team ) {
		sdProgram::ReturnInteger( -1 );
		return;
	}

	sdProgram::ReturnInteger( objectives[ objectiveIndex ].GetState( team->GetIndex() ) );
	
}

/*
================
sdObjectiveManagerLocal::OnNetworkEvent
================
*/
void sdObjectiveManagerLocal::OnNetworkEvent( const char* message ) {
	gameLocal.SetActionCommand( message );

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnNetworkEvent" ), h1 );
}

/*
================
sdObjectiveManagerLocal::Event_SendNetworkEvent
================
*/
void sdObjectiveManagerLocal::Event_SendNetworkEvent( int clientIndex, bool isRepeaterClient, const char* message ) {
	if ( !isRepeaterClient ) {
		if ( clientIndex == -1 ) {
			if ( gameLocal.GetLocalPlayer() != NULL ) {
				OnNetworkEvent( message );
			}
		} else {
			idPlayer* player = gameLocal.GetClient( clientIndex );
			if ( player != NULL && gameLocal.IsLocalPlayer( player ) ) {
				OnNetworkEvent( message );
			}
		}
	}

	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_NETWORKEVENT );
	msg.WriteLong( NETWORKEVENT_OBJECTIVE_ID );
	msg.WriteString( message );
	if ( isRepeaterClient ) {
		msg.Send( sdReliableMessageClientInfoRepeater( clientIndex ) );
	} else {
		msg.Send( sdReliableMessageClientInfo( clientIndex ) );
	}
}

/*
================
sdObjectiveManagerLocal::Event_SetNextObjective
================
*/
void sdObjectiveManagerLocal::Event_SetNextObjective( idScriptObject* object, int objectiveIndex ) {
	// -1 is a valid index, since it means "no next objective"
	if ( objectiveIndex < -1 || objectiveIndex >= objectives.Num() ) {
		return;
	}

	sdTeamInfo* team = object ? object->GetClass()->Cast< sdTeamInfo >() : NULL;
	if ( !team ) {
		return;
	}

	SetNextObjective( team, objectiveIndex );
}

/*
================
sdObjectiveManagerLocal::Event_GetNextObjective
================
*/
void sdObjectiveManagerLocal::Event_GetNextObjective( idScriptObject* object ) {
	sdTeamInfo* team = object ? object->GetClass()->Cast< sdTeamInfo >() : NULL;
	if ( !team ) {
		sdProgram::ReturnInteger( -1 );
		return;
	}

	sdProgram::ReturnInteger( nextObjective[ team->GetIndex() ] );
}

/*
================
sdObjectiveManagerLocal::Event_SetShortDescription
================
*/
void sdObjectiveManagerLocal::Event_SetShortDescription( idScriptObject* object, int objectiveIndex, int description ) {
	if ( objectiveIndex < 0 || objectiveIndex >= objectives.Num() ) {
		return;
	}

	sdTeamInfo* team = object ? object->GetClass()->Cast< sdTeamInfo >() : NULL;
	if ( !team ) {
		return;
	}

	objectives[ objectiveIndex ].SetDescription( team->GetIndex(), declHolder.FindLocStrByIndex( description ) );
}

/*
================
sdObjectiveManagerLocal::Event_GetShortDescription
================
*/
void sdObjectiveManagerLocal::Event_GetShortDescription( idScriptObject* object, int objectiveIndex ) {
	if ( objectiveIndex < 0 || objectiveIndex >= objectives.Num() ) {
		sdProgram::ReturnHandle( -1 );
		return;
	}

	sdTeamInfo* team = object ? object->GetClass()->Cast< sdTeamInfo >() : NULL;
	if ( !team ) {
		sdProgram::ReturnHandle( -1 );
		return;
	}

	sdProgram::ReturnHandle( objectives[ objectiveIndex ].GetDescription( team->GetIndex() ) );
}

/*
================
sdObjectiveManagerLocal::Event_CreateMapScript
================
*/
void sdObjectiveManagerLocal::Event_CreateMapScript( void ) {
	if ( gameLocal.mapInfo == NULL ) {
		return;
	}

	const char* mapScriptEntryPoint = gameLocal.mapInfo->GetData().GetString( "script_entrypoint", "Default_MapScript" );
	if ( *mapScriptEntryPoint == '\0' ) {
		return;
	}

	const sdProgram::sdFunction* function = gameLocal.program->FindFunction( mapScriptEntryPoint );
	if ( function == NULL ) {
		gameLocal.Warning( "sdObjectiveManagerLocal::Event_CreateMapScript Could Not Find Script Entry Point '%s'", mapScriptEntryPoint );
		return;
	}

	gameLocal.CallFrameCommand( function );

	sdProgram::ReturnObject( gameLocal.program->GetReturnedObject() );
}

/*
================
sdObjectiveManagerLocal::Event_LogObjectiveCompletion
================
*/
void sdObjectiveManagerLocal::Event_LogObjectiveCompletion( int objectiveIndex ) {
	if ( g_logObjectives.GetBool() && !gameLocal.isClient && gameLocal.rules->GetState() == sdGameRules::GS_GAMEON ) {
		idStr time;
		GetRealTime( time );

		gameLocal.LogObjective( "-----------------------------------------------\n" );
		gameLocal.LogObjective( va( "Objective %d Completed: %s\n", objectiveIndex, time.c_str() ) );
		LogPlayerStats();
		gameLocal.LogObjective( "-----------------------------------------------\n" );
	}

	gameLocal.rules->OnObjectiveCompletion( objectiveIndex );
}


/*
================
sdObjectiveManagerLocal::Event_DeactivateBotActionGroup
================
*/
void sdObjectiveManagerLocal::Event_DeactivateBotActionGroup( int actionGroupNum ) {

	bool hasActiveForever = false;

	for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->ActionIsActiveForever() ) { //mal: you can't just turn off "active forever" actions with a sweep of the hand. You need to do this directly.
			hasActiveForever = true;
			continue;
		}
		
		if ( botThreadData.botActions[ i ]->GetActionGroup() != actionGroupNum ) {
			continue;
		}

		botThreadData.botActions[ i ]->SetActive( false );
	}

	if ( hasActiveForever ) {
		gameLocal.DWarning("There was 1 or more \"Active Forever\" action(s) in action group num %i that wasn't deactivated. These actions need to be deactivated by name!", actionGroupNum );
	}
}

/*
================
sdObjectiveManagerLocal::Event_KillBotActionGroup

Kills the action group forever - it will never be used again.
================
*/
void sdObjectiveManagerLocal::Event_KillBotActionGroup( int actionGroupNum ) {

	bool hasActiveForever = false;

	for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->ActionIsActiveForever() ) { //mal: you can't just turn off "active forever" actions with a sweep of the hand. You need to do this directly.
			hasActiveForever = true;
			continue;
		}
		
		if ( botThreadData.botActions[ i ]->GetActionGroup() != actionGroupNum ) {
			continue;
		}

		botThreadData.botActions[ i ]->SetHumanObj( ACTION_NULL ); //mal: nuke it!
		botThreadData.botActions[ i ]->SetStroggObj( ACTION_NULL );
		botThreadData.botActions[ i ]->SetActive( false );
		botThreadData.botActions[ i ]->SetActionGroupNum( -1 );
	}

	if ( hasActiveForever ) {
		gameLocal.DWarning("There was 1 or more \"Active Forever\" action(s) in action group num %i that wasn't killed. These actions need to be killed by name!", actionGroupNum );
	}
}

/*
================
sdObjectiveManagerLocal::Event_ActivateBotActionGroup
================
*/
void sdObjectiveManagerLocal::Event_ActivateBotActionGroup( int actionGroupNum ) {
for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}
		
		if ( botThreadData.botActions[ i ]->GetActionGroup() != actionGroupNum ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->ActionIsActive() == false ) { //mal: some maps will activate actions multiple times.
			botThreadData.botActions[ i ]->SetActionActivateTime( gameLocal.time );
		}

		botThreadData.botActions[ i ]->SetActive( true );
	}
}

/*
================
sdObjectiveManagerLocal::Event_SetBotActionGroupVehicleType
================
*/
void sdObjectiveManagerLocal::Event_SetBotActionGroupVehicleType( int actionGroupNum, int actionVehicleFlags ) {
for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}
		
		if ( botThreadData.botActions[ i ]->GetActionGroup() != actionGroupNum ) {
			continue;
		}

		botThreadData.botActions[ i ]->SetActionVehicleFlags( actionVehicleFlags );
	}
}

/*
================
sdObjectiveManagerLocal::Event_DisableAASAreaInLocation
================
*/
void sdObjectiveManagerLocal::Event_DisableAASAreaInLocation( int aasType, const idVec3& location ) {
	botThreadData.DisableAASAreaInLocation( aasType, location );
}

/*
================
sdObjectiveManagerLocal::Event_SetBotActionState
================
*/
void sdObjectiveManagerLocal::Event_SetBotActionStateForEvent( const botActionStates_t state, idEntity* triggerEntity ) {

	if ( triggerEntity == NULL ) {
		gameLocal.DWarning( "An invalid entity was passed to \"setBotActionStateForEvent\"!" );
		return;
	}

	bool foundAction = false;
	bool runSecondPass = false;
	bool expandBox = false;
	int entNum;
	botActionGoals_t filter;
	botActionGoals_t filter2 = ACTION_NULL;

	entNum = triggerEntity->entityNumber;

	if ( state == ACTION_STATE_PLANTED || state == ACTION_STATE_DEFUSED ) {
		filter = ACTION_HE_CHARGE;
		filter2 = ACTION_DEFUSE;
	} else if ( state == ACTION_STATE_START_BUILD || state == ACTION_STATE_BUILD_FIZZLED ) {			//mal_TODO: what about minor builds?? Add support for them too!
		filter = ACTION_MAJOR_OBJ_BUILD;
	} else if ( state == ACTION_STATE_START_HACK || state == ACTION_STATE_HACK_FIZZLED ) {
		filter = ACTION_HACK;
	} else if ( state == ACTION_STATE_OBJ_STOLEN || state == ACTION_STATE_OBJ_RETURNED || state == ACTION_STATE_OBJ_DROPPED ) {
		filter = ACTION_STEAL;
	} else if ( state == ACTION_STATE_GUN_READY || state == ACTION_STATE_GUN_DESTROYED ) {
		filter = ACTION_MG_NEST;
		filter2 = ACTION_MG_NEST_BUILD;
	} else {
		gameLocal.DWarning( "Invalid action state passed to setBotActionStateForEvent!" );
		return;
	}

try_again:

	for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {

		if ( foundAction == true ) {
			break;
		}

        if ( !botThreadData.botActions[ i ]->ActionIsActive() ) { //mal: if action is turned off, ignore
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetHumanObj() != filter && botThreadData.botActions[ i ]->GetStroggObj() != filter ) {
			if ( filter2 != ACTION_NULL ) {
				if ( botThreadData.botActions[ i ]->GetHumanObj() != filter2 && botThreadData.botActions[ i ]->GetStroggObj() != filter2 ) {
					continue;
				}
			} else {
				continue;
			}
		}

//mal_TODO: add a similiar check here for all objs, as they can have more then 1 on a map, at the same time. Builds, steals, and hacks!
		if ( state == ACTION_STATE_PLANTED || state == ACTION_STATE_DEFUSED ) {
           if ( !botThreadData.botActions[ i ]->EntityIsInsideActionBBox( entNum, PLANTED_CHARGE, expandBox ) ) { //mal: in case of multiple objs of the same type
			   continue;
		   }
		}

 		if ( state == ACTION_STATE_GUN_READY || state == ACTION_STATE_GUN_DESTROYED ) {
			if ( !botThreadData.botActions[ i ]->EntityOriginIsInsideActionBBox( triggerEntity->GetPhysics()->GetOrigin() ) ) { //mal: in case of multiple objs of the same type
				continue;
			}
		}

		if ( state == ACTION_STATE_START_BUILD || state == ACTION_STATE_BUILD_FIZZLED ) {
			if ( !botThreadData.botActions[ i ]->EntityOriginIsInsideActionBBox( triggerEntity->GetPhysics()->GetOrigin() ) ) { //mal: in case of multiple objs of the same type
				continue;
			}
		}


		foundAction = true;

		switch ( state ) {
			case ACTION_STATE_PLANTED: {
				botThreadData.botActions[ i ]->SetActionState( ACTION_STATE_PLANTED );

				playerClassTypes_t classFilter = NOCLASS;

				if ( !botThreadData.botActions[ i ]->ActionIsPriority() ) { //mal: if the plant action isn't a priority, only engs will notice it.
					classFilter = ENGINEER;
				}

				if ( !botThreadData.botActions[ i ]->ArmedChargesInsideActionBBox( entNum ) ) {
					Event_NotifyBotOfEvent( NOTEAM, classFilter, ACTION_STATE_PLANTED ); //mal: let the bot's know a major event just happened! Only if its the first one.
				}

				break;
			}

			case ACTION_STATE_DEFUSED: { //mal: a charge was disarmed, make sure no others exist before we give the all clear signal!
                if ( !botThreadData.botActions[ i ]->ArmedChargesInsideActionBBox( entNum ) ) {
					botThreadData.botActions[ i ]->SetActionState( ACTION_STATE_NORMAL );
					Event_NotifyBotOfEvent( NOTEAM, NOCLASS, ACTION_STATE_DEFUSED ); //mal: let the bot's know a major event just happened!
				}
				break;
			}

			case ACTION_STATE_START_BUILD: { //mal: someones building an obj! Let the bots know.
  				botThreadData.botActions[ i ]->SetActionState( ACTION_STATE_START_BUILD );
				Event_NotifyBotOfEvent( NOTEAM, NOCLASS, ACTION_STATE_START_BUILD ); //mal: let the bot's know a major event just happened!
				break;
			}

			case ACTION_STATE_BUILD_FIZZLED: { //mal: the obj was never built, and timed out.
  				botThreadData.botActions[ i ]->SetActionState( ACTION_STATE_NORMAL );
				Event_NotifyBotOfEvent( NOTEAM, NOCLASS, ACTION_STATE_BUILD_FIZZLED ); //mal: let the bot's know a major event just happened!
				break;
			}
			
			case ACTION_STATE_START_HACK: {
				botThreadData.botActions[ i ]->SetActionState( ACTION_STATE_START_HACK );
				Event_NotifyBotOfEvent( NOTEAM, NOCLASS, ACTION_STATE_START_HACK ); //mal: let the bot's know a major event just happened!
				break;
			}

			case ACTION_STATE_HACK_FIZZLED: {
				botThreadData.botActions[ i ]->SetActionState( ACTION_STATE_NORMAL ); //mal: need to update now that hacks can fizzle.
				break;
			}

			case ACTION_STATE_GUN_READY: {
				botThreadData.botActions[ i ]->SetActionState( ACTION_STATE_GUN_READY ); 
				break;
			}

			case ACTION_STATE_GUN_DESTROYED: {
				botThreadData.botActions[ i ]->SetActionState( ACTION_STATE_GUN_DESTROYED );
				break;
			}

			//mal_TODO: add more events/states here as we add them to the code/scripts!


			default: {
				break;
			}
		}
	}

	if ( foundAction == false ) {
		if ( runSecondPass == true ) {
			gameLocal.DWarning( "Couldn't find an bot action for entity #%i and action state: %i", entNum, state );
		} else {
			runSecondPass = true;
			expandBox = true;
			goto try_again;
		}
	}
}

/*
================
sdObjectiveManagerLocal::Event_TeamSuicideIfNotNearAction

Has the bot respawn if hes too far away from this next goal, as defined by requiredDist. This way can often beat the attackers to their next obj.
================
*/
void sdObjectiveManagerLocal::Event_TeamSuicideIfNotNearAction( const char *actionName, float requiredDist, const playerTeamTypes_t playerTeam ) {

	bool haveVehicle = false;
	int actionNum;
	int i, k;
	idVec3 vec;
	int respawnTime = ( playerTeam == GDF ) ? botThreadData.GetGameWorldState()->gameLocalInfo.nextGDFRespawnTime : botThreadData.GetGameWorldState()->gameLocalInfo.nextStroggRespawnTime;

	if ( respawnTime > 10 ) { //mal: only do this if will respawn soon!
		return;
	}

	actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum == -1 ) {
		gameLocal.Error( "No valid bot action found by the name %s in \"teamSuicideIfNotNearAction\"!", actionName );
	}

	if ( playerTeam <= NOTEAM || playerTeam > STROGG ) {
		gameLocal.Error( "Invalid team parameter passsed to \"teamSuicideIfNotNearAction\"!\nTeam must be either \"GDF\" or \"STROGG\"" );
	}

	if ( requiredDist < 0.0f ) {
		gameLocal.Error( "Invalid dist parameter passsed to \"teamSuicideIfNotNearAction\"!\nDist should be greater then 6000 and is measured in game units\nThe smaller the number, the closer the bots will need to be to the action" );
	}

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		clientInfo_t& playerInfo = botThreadData.GetGameWorldState()->clientInfo[ i ];

		if ( playerInfo.inGame == false ) {
			continue;
		}

		if ( playerInfo.team == NOTEAM ) {
			continue;
		}

		if ( playerInfo.isBot == false ) {
			continue;
		}

		if ( playerInfo.team != playerTeam ) {
			continue;
		}

		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) { //mal: dont suicide if in a vehicle.
			continue;
		}

		for( k = 0; k < MAX_VEHICLES; k++ ) {
			if ( botThreadData.GetGameWorldState()->vehicleInfo[ k ].entNum < MAX_CLIENTS ) {
				continue;
			}

			if ( botThreadData.GetGameWorldState()->vehicleInfo[ k ].driverEntNum != -1 ) {
				continue;
			}

			if ( botThreadData.GetGameWorldState()->vehicleInfo[ k ].isFlipped || botThreadData.GetGameWorldState()->vehicleInfo[ k ].isImmobilized || botThreadData.GetGameWorldState()->vehicleInfo[ k ].damagedPartsCount > 0 ) {
				continue;
			}

			if ( botThreadData.GetGameWorldState()->vehicleInfo[ k ].isAmphibious ) {
				continue;
			}

			if ( botThreadData.GetGameWorldState()->vehicleInfo[ k ].inWater ) {
				continue;
			}

			vec = botThreadData.GetGameWorldState()->vehicleInfo[ k ].origin - playerInfo.origin;

			if ( vec.LengthSqr() > MAX_VEHICLE_RANGE ) {
				continue;
			}

			haveVehicle = true;
			break;
		}

		if ( haveVehicle ) { //mal: dont suicide if have vehicle nearby - could use it to reach our goal that much faster!
			continue;
		}


		if ( playerInfo.lastAttackerTime + 3000 > gameLocal.time || playerInfo.lastAttackClientTime + 3000 > gameLocal.time ) { //mal: dont if we're in combat.
			continue;
		}

		if ( playerInfo.isDisguised ) {
			continue;
		}

		vec = botThreadData.botActions[ actionNum ]->GetActionOrigin() - playerInfo.origin;

		if ( vec.LengthSqr() < Square( requiredDist ) ) {
			continue;
		}

		idPlayer* player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		player->Kill( NULL ); //mal: our next defense goal is too far away! Respawn, so can be closer faster.
	}
}

/*
================
sdObjectiveManagerLocal::Event_DeactivateBotAction
================
*/
void sdObjectiveManagerLocal::Event_DeactivateBotAction( const char *actionName ) {

	int actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum != -1 ) {
        botThreadData.botActions[ actionNum ]->SetActive( false );
		return;
	}

	gameLocal.DWarning( "No valid bot action found by the name %s in \"deactivateBotAction\"!", actionName );
}

/*
================
sdObjectiveManagerLocal::Event_KillBotAction
================
*/
void sdObjectiveManagerLocal::Event_KillBotAction( const char *actionName ) {

	int actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum != -1 ) {
        botThreadData.botActions[ actionNum ]->SetActive( false );
		botThreadData.botActions[ actionNum ]->SetHumanObj( ACTION_NULL );
		botThreadData.botActions[ actionNum ]->SetStroggObj( ACTION_NULL );
		botThreadData.botActions[ actionNum ]->SetActionGroupNum( -1 );
		return;
	}

	gameLocal.DWarning( "No valid bot action found by the name %s in \"killBotAction\"!", actionName );
}

/*
================
sdObjectiveManagerLocal::Event_IsActionGroupActive
================
*/
void sdObjectiveManagerLocal::Event_IsActionGroupActive( int actionGroupNum ) {
	int numActive = 0;

	for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {
		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}
		
		if ( botThreadData.botActions[ i ]->GetActionGroup() != actionGroupNum ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->ActionIsActive() && !botThreadData.botActions[ i ]->ActionIsActiveForever() ) {
			numActive++;
		}
	}

	sdProgram::ReturnFloat( ( numActive > 0 ) ? 1.0f : 0.0f );
}

/*
================
sdObjectiveManagerLocal::Event_IsActionActive
================
*/
void sdObjectiveManagerLocal::Event_IsActionActive( const char* actionName ) {
	int	actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum == -1 ) {
		gameLocal.DWarning( "No valid bot action found by the name %s in \"isActionActive\"!", actionName );
		sdProgram::ReturnFloat( 0.0f );
		return;
	}

	bool isActive = botThreadData.botActions[ actionNum ]->ActionIsActive();

	sdProgram::ReturnFloat( ( isActive ) ? 1.0f : 0.0f );
}

/*
================
sdObjectiveManagerLocal::Event_ActivateBotAction
================
*/
void sdObjectiveManagerLocal::Event_ActivateBotAction( const char *actionName ) {

	int actionNum;

	actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum != -1 ) {
		if ( botThreadData.botActions[ actionNum ]->ActionIsActive() == false ) { //mal: some maps will activate actions multiple times.
			botThreadData.botActions[ actionNum ]->SetActionActivateTime( gameLocal.time );
		}
        botThreadData.botActions[ actionNum ]->SetActive( true );
		return;
	}

	gameLocal.DWarning( "No valid bot action found by the name %s in \"activateBotAction\"!", actionName );
}

/*
================
sdObjectiveManagerLocal::Event_SetBotCriticalClass
================
*/
// Gordon: FIXME: Just pass in the team object
void sdObjectiveManagerLocal::Event_SetBotCriticalClass( const playerTeamTypes_t playerTeam, const playerClassTypes_t criticalClass ) {

	if ( criticalClass < NOCLASS || criticalClass > COVERTOPS ) {
		gameLocal.DWarning( "Invalid player class type passed by setBotCriticalClass!" );
		return;
	}

	if ( playerTeam < GDF || playerTeam > STROGG ) {
		gameLocal.DWarning( "Invalid player team type passed by setBotCriticalClass!" );
		return;
	}

	if ( playerTeam == GDF ) {
		botThreadData.GetGameWorldState()->botGoalInfo.team_GDF_criticalClass = criticalClass;
	} else {
		botThreadData.GetGameWorldState()->botGoalInfo.team_STROGG_criticalClass = criticalClass;
	}

	SetCriticalClass( playerTeam, criticalClass );
}


/*
============
sdObjectiveManagerLocal::SetCriticalClass
============
*/
void sdObjectiveManagerLocal::SetCriticalClass( const playerTeamTypes_t playerTeam, const playerClassTypes_t criticalClass ) {
	if ( playerTeam == GDF ) {
		if( criticalClass == gdfCriticalClass ) {
			return;
		}
		gdfCriticalClass = criticalClass;
	} else {
		if( criticalClass == stroggCriticalClass ) {
			return;
		}
		stroggCriticalClass = criticalClass;
	}

	gameLocal.localPlayerProperties.SetCriticalClass( playerTeam, criticalClass );

	if( gameLocal.isServer ) {
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_SETCRITICALCLASS );
		msg.WriteChar( playerTeam );
		msg.WriteChar( criticalClass );
		msg.Send( sdReliableMessageClientInfoAll() );
	}
}

/*
================
sdObjectiveManagerLocal::Event_SwitchTeamWeapons
================
*/
void sdObjectiveManagerLocal::Event_SwitchTeamWeapons( const playerTeamTypes_t playerTeam, const playerClassTypes_t playerClass, const playerWeaponTypes_t curWeapType, const playerWeaponTypes_t desiredWeapType, bool randomly ) {

	int n = 0;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		idPlayer* player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		clientInfo_t& client = botThreadData.GetGameWorldState()->clientInfo[ i ];

		if ( client.team == NOTEAM ) {
			continue;
		}

		if ( !client.isBot ) {
			continue;
		}

		if ( playerClass != NOCLASS && client.classType != playerClass ) {
			continue;
		}

		if ( playerTeam != NOTEAM && client.team != playerTeam ) {
			continue;
		}

		if ( curWeapType != -1 ) {
			if ( client.weapInfo.primaryWeapon != curWeapType ) {
				continue;
			}
		}

		if ( client.weapInfo.primaryWeapon == desiredWeapType ) {
			continue;
		}
		
		if ( randomly ) {
			if ( gameLocal.random.RandomInt( 100 ) > 50 ) {
				continue;
			}
		}

		int classWeapon = -1;

		if ( desiredWeapType == SHOTGUN ) {

			if ( client.classType != MEDIC && client.classType != ENGINEER && client.classType != SOLDIER ) {
				continue;
			}

			classWeapon = 1;
		
			if ( client.classType == SOLDIER ) {
				classWeapon = 3;
			}
		} else if ( desiredWeapType == SNIPERRIFLE ) {
			if ( client.classType != COVERTOPS ) {
				continue;
			}

			classWeapon = 1;
		} else if ( desiredWeapType == SMG ) {
			classWeapon = 0;
		} else if ( desiredWeapType == ROCKET ) {
			if ( client.classType != SOLDIER ) {
				continue;
			}

			classWeapon = 1;
		} else if ( desiredWeapType == HEAVY_MG ) {
			classWeapon = 2;
		}

		//mal: switch to the new weapon, if one was found
		if ( classWeapon != -1 ) {
			player->GetInventory().SetCachedClassOption( 0, classWeapon );
			n++;
		}
	}

	if ( n == 0 ) { //mal: this isn't a critical error - just let the person know they may have goofed the script up.
		gameLocal.DWarning( "No bot class/team type was found in the call to \"switchTeamWeapons\"" );
	}
}

/*
================
sdObjectiveManagerLocal::Event_NotifyBotOfEvent
================
*/
// Gordon: FIXME: Just pass in the team object
void sdObjectiveManagerLocal::Event_NotifyBotOfEvent( const playerTeamTypes_t playerTeam, const playerClassTypes_t playerClass, const botActionStates_t eventType ) {

	if ( botThreadData.GetNumBotsInServer( NOTEAM ) == 0 ) { //mal: not much point to this if theres no bots on the server!
		return;
	}

	int actionNumber;
	botActionGoals_t actionGoal;
	idBox actionBBox;
	int resetState = MAJOR_RESET_EVENT; //mal: by default, all resets are major resets.
	
	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		clientInfo_t& playerInfo = botThreadData.GetGameWorldState()->clientInfo[ i ];

		if ( playerInfo.inGame == false ) {
			continue;
		}

		if ( playerInfo.team == NOTEAM ) {
			continue;
		}

		if ( playerInfo.isBot == false ) {
			continue;
		}

		if ( playerInfo.isActor ) {
			continue;
		}

		if ( playerTeam != NOTEAM ) {
            if ( playerInfo.team != playerTeam ) {
				continue;
			}
		}

		if ( playerClass != NOCLASS ) {
            if ( playerInfo.classType != playerClass ) {
				continue;
			}
		}

		if ( eventType == ACTION_STATE_OBJ_STOLEN || eventType == ACTION_STATE_OBJ_RETURNED || eventType == ACTION_STATE_OBJ_DROPPED ) {
			playerInfo.resetState = resetState;
			continue;
		}

		idBox playerBox = idBox( playerInfo.localBounds, playerInfo.origin, playerInfo.bodyAxis );

		if ( botThreadData.GetGameWorldState()->botGoalInfo.team_GDF_PrimaryAction != -1 && botThreadData.GetGameWorldState()->botGoalInfo.team_STROGG_PrimaryAction != -1 ) {
            if ( eventType != ACTION_STATE_NULL ) {
                
				if ( playerInfo.team == GDF ) {
					actionNumber = botThreadData.GetGameWorldState()->botGoalInfo.team_GDF_PrimaryAction;
			        actionGoal = botThreadData.botActions[ actionNumber ]->GetHumanObj();
					actionBBox = botThreadData.botActions[ actionNumber ]->GetBox();
				} else {
					actionNumber = botThreadData.GetGameWorldState()->botGoalInfo.team_STROGG_PrimaryAction;
					actionGoal = botThreadData.botActions[ actionNumber ]->GetStroggObj();
					actionBBox = botThreadData.botActions[ actionNumber ]->GetBox();
				}

				actionBBox.ExpandSelf( ACTION_BBOX_EXPAND_BIG );

//mal: dont reset the bot that triggered this event, if they're in the process of doing it!
				if ( eventType == ACTION_STATE_START_BUILD ) {
                    if ( actionGoal == ACTION_MAJOR_OBJ_BUILD && playerInfo.classType == ENGINEER ) {
						if ( actionBBox.IntersectsBox( playerBox ) ) {
							continue;
						}

						int currentActionNum = botThreadData.FindActionByTypeForLocation( playerInfo.origin, ACTION_MAJOR_OBJ_BUILD, playerInfo.team );

						if ( currentActionNum != ACTION_NULL ) {
							if ( botThreadData.ActionIsLinkedToAction( currentActionNum, actionNumber ) ) {
								continue;
							}
						}
					}
				}

				if ( eventType == ACTION_STATE_PLANTED ) {
                    if ( actionGoal == ACTION_HE_CHARGE && playerInfo.classType == SOLDIER ) {
						if ( actionBBox.IntersectsBox( playerBox ) ) {
							continue;
						}

						int currentActionNum = botThreadData.FindActionByTypeForLocation( playerInfo.origin, ACTION_HE_CHARGE, playerInfo.team );

						if ( currentActionNum != ACTION_NULL ) {
							if ( botThreadData.ActionIsLinkedToAction( currentActionNum, actionNumber ) ) {
								continue;
							}
						}
					}
				}

				if ( eventType == ACTION_STATE_START_HACK ) {
					if ( actionGoal == ACTION_HACK && playerInfo.classType == COVERTOPS ) {
						if ( actionBBox.IntersectsBox( playerBox ) ) {
							continue;
						}

						int currentActionNum = botThreadData.FindActionByTypeForLocation( playerInfo.origin, ACTION_HACK, playerInfo.team );

						if ( currentActionNum != ACTION_NULL ) {
							if ( botThreadData.ActionIsLinkedToAction( currentActionNum, actionNumber ) ) {
								continue;
							}
						}
					}
				}
			}
		}

        playerInfo.resetState = resetState;
	}
}

/*
================
sdObjectiveManagerLocal::Event_SetAttackingTeam
================
*/
// Gordon: FIXME: Just pass in the team object
void sdObjectiveManagerLocal::Event_SetAttackingTeam( const playerTeamTypes_t playerTeam ) {

	if ( playerTeam < GDF || playerTeam > STROGG ) {
		gameLocal.DWarning( "Invalid player team type passed by setAttackingTeam!" );
		return;
	}

	botThreadData.GetGameWorldState()->botGoalInfo.attackingTeam = playerTeam;

	// pass it to the rules - ugh, hack.
	sdTeamManagerLocal& manager = sdTeamManager::GetInstance();
	sdTeamInfo* teamA = manager.GetTeamSafe( "gdf" );
	sdTeamInfo* teamB = manager.GetTeamSafe( "strogg" );
	if ( playerTeam == STROGG ) {
		Swap( teamA, teamB );
	}
	gameLocal.rules->SetAttacker( teamA );
	gameLocal.rules->SetDefender( teamB );
}

/*
================
sdObjectiveManagerLocal::Event_SetTeamAttacksDeployables
================
*/
void sdObjectiveManagerLocal::Event_SetTeamAttacksDeployables( const playerTeamTypes_t playerTeam, bool attackDeployables ) {

	if ( playerTeam < GDF || playerTeam > STROGG ) {
		gameLocal.DWarning( "Invalid player team type passed by setTeamAttacksDeployables!" );
		return;
	}

	if ( playerTeam == GDF ) {
		botThreadData.GetGameWorldState()->botGoalInfo.team_GDF_AttackDeployables = attackDeployables;
	} else {
		botThreadData.GetGameWorldState()->botGoalInfo.team_STROGG_AttackDeployables = attackDeployables;
	}
}

/*
================
sdObjectiveManagerLocal::Event_SetPrimaryAction
================
*/
// Gordon: FIXME: Just pass in the team object
void sdObjectiveManagerLocal::Event_SetPrimaryAction( const playerTeamTypes_t playerTeam,  const char *actionName ) {

	int actionNum;

	if ( playerTeam < GDF || playerTeam > STROGG ) {
		gameLocal.DWarning( "Invalid player team type passed by setPrimaryTeamAction!" );
		return;
	}

	if ( idStr::Cmp( actionName, "NULL" ) == 0 || idStr::Cmp( actionName, "null" ) == 0 ) { //mal: we dont have a mission for the bots ATM.
		if ( playerTeam == GDF ) {
			botThreadData.GetGameWorldState()->botGoalInfo.team_GDF_PrimaryAction = ACTION_NULL;
		} else {
			botThreadData.GetGameWorldState()->botGoalInfo.team_STROGG_PrimaryAction = ACTION_NULL;
		}
		return;
	}

	actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum != -1 ) {
		if ( playerTeam == GDF ) {
			botThreadData.GetGameWorldState()->botGoalInfo.team_GDF_PrimaryAction = actionNum;
		} else {
			botThreadData.GetGameWorldState()->botGoalInfo.team_STROGG_PrimaryAction = actionNum;
		}
		return;
	}

	gameLocal.DWarning( "Map Script Error! No valid bot action found by the name %s in setPrimaryTeamAction", actionName );
}

/*
================
sdObjectiveManagerLocal::Event_SetSecondaryAction
================
*/
// Gordon: FIXME: Just pass in the team object
void sdObjectiveManagerLocal::Event_SetSecondaryAction( const playerTeamTypes_t playerTeam,  const char *actionName ) {

	int actionNum;

	if ( playerTeam < GDF || playerTeam > STROGG ) {
		gameLocal.DWarning("Invalid player team type passed by setSecondaryTeamAction!\n");
		return;
	}

	actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum != -1 ) {
		if ( playerTeam == GDF ) {
			botThreadData.GetGameWorldState()->botGoalInfo.team_GDF_SecondaryAction = actionNum;
		} else {
			botThreadData.GetGameWorldState()->botGoalInfo.team_STROGG_SecondaryAction = actionNum;
		}
		return;
	}

	gameLocal.DWarning( "Map Script Error! No valid bot action found by the name %s in \"setSecondaryTeamAction\"", actionName );
}

/*
================
sdObjectiveManagerLocal::Event_SetActorPrimaryAction
================
*/
void sdObjectiveManagerLocal::Event_SetActorPrimaryAction( const char* actionName, bool isDeployableMission, int goalPauseTime ) {

	int	actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum != -1 ) {
		botThreadData.actorMissionInfo.actionNumber = actionNum;
		botThreadData.actorMissionInfo.hasBriefedPlayer = false;
		botThreadData.actorMissionInfo.goalPauseTime = ( goalPauseTime * 1000 );
		botThreadData.actorMissionInfo.hasEnteredActorNode = false;

		if ( isDeployableMission ) {
			botThreadData.actorMissionInfo.deployableStageIsActive = true;
		}

		if ( botThreadData.actorMissionInfo.actorClientNum > -1 && botThreadData.actorMissionInfo.actorClientNum < MAX_CLIENTS ) {
			clientInfo_t& player = botThreadData.GetGameWorldState()->clientInfo[ botThreadData.actorMissionInfo.actorClientNum ];
			player.resetState = 2; //mal: now clear the bots AI, so they can prepare for the next mission....
		}
		return;
	}

	gameLocal.DWarning( "Map Script Error! No valid bot action found by the name %s in setActorPrimaryAction", actionName );
}

/*
================
sdObjectiveManagerLocal::Event_SetBriefingPauseTime
================
*/
void sdObjectiveManagerLocal::Event_SetBriefingPauseTime( int pauseTime ) {
	botThreadData.actorMissionInfo.chatPauseTime = pauseTime;
}

/*
================
sdObjectiveManagerLocal::Event_SetTrainingBotsCanGrabForwardSpawns
================
*/
void sdObjectiveManagerLocal::Event_SetTrainingBotsCanGrabForwardSpawns() {
	botThreadData.actorMissionInfo.forwardSpawnIsAllowed = true;
}

/*
================
sdObjectiveManagerLocal::Event_SetBotSightDist
================
*/
void sdObjectiveManagerLocal::Event_SetBotSightDist( float sightDist ) {
	if ( sightDist < 0.0f || sightDist > 6000.0f ) {
		gameLocal.DWarning( "value passed by setBotSightDist \"%f\" is not in the proper range! It should be between 1000 to 6000!\nDefaulting to 3000!", sightDist );
		sightDist = 3000.0f;
	}

	botThreadData.GetGameWorldState()->botGoalInfo.botSightDist = sightDist;
}

/*
================
sdObjectiveManagerLocal::Event_SetMapHasMCPGoal
================
*/
void sdObjectiveManagerLocal::Event_SetMapHasMCPGoal( bool hasMCPGoal ) {
	botThreadData.GetGameWorldState()->botGoalInfo.mapHasMCPGoal = hasMCPGoal;
	if ( botThreadData.GetGameWorldState()->botGoalInfo.mapHasMCPGoalTime == 0 ) {
		botThreadData.GetGameWorldState()->botGoalInfo.mapHasMCPGoalTime = gameLocal.time;
	}
}

/*
================
sdObjectiveManagerLocal::Event_GetNumBotsOnTeam
================
*/
void sdObjectiveManagerLocal::Event_GetNumBotsOnTeam( const playerTeamTypes_t playerTeam ) {
	if ( playerTeam < GDF || playerTeam > STROGG ) {
		gameLocal.DWarning( "Map Script Error! Invalid team passed into \"getNumBotsOnTeam\"!" );
		sdProgram::ReturnFloat( 0.0f );
	}

	sdProgram::ReturnFloat( ( float ) botThreadData.GetNumBotsOnTeam( playerTeam ) );
}

/*
================
sdObjectiveManagerLocal::Event_GameIsOnFinalObjective
================
*/
void sdObjectiveManagerLocal::Event_GameIsOnFinalObjective() {
	botThreadData.GetGameWorldState()->botGoalInfo.gameIsOnFinalObjective = true;
}

/*
================
sdObjectiveManagerLocal::Event_MapIsTrainingMap
================
*/
void sdObjectiveManagerLocal::Event_MapIsTrainingMap() {
	botThreadData.GetGameWorldState()->botGoalInfo.isTrainingMap = true;
}


/*
================
sdObjectiveManagerLocal::Event_SetPlayerIsOnFinalMission 
================
*/
void sdObjectiveManagerLocal::Event_SetPlayerIsOnFinalMission() {
	botThreadData.actorMissionInfo.playerNeedsFinalBriefing = true;
	botThreadData.actorMissionInfo.playerIsOnFinalMission = true;
	botThreadData.actorMissionInfo.actionNumber = ACTION_NULL;
	botThreadData.actorMissionInfo.hasBriefedPlayer = false;
	botThreadData.actorMissionInfo.deployableStageIsActive = false;
	
	if ( botThreadData.actorMissionInfo.actorClientNum > -1 && botThreadData.actorMissionInfo.actorClientNum < MAX_CLIENTS ) {
		clientInfo_t& player = botThreadData.GetGameWorldState()->clientInfo[ botThreadData.actorMissionInfo.actorClientNum ];
		player.resetState = 2; //mal: now clear the bots AI, so they can prepare for the next mission....
	}
}

/*
================
sdObjectiveManagerLocal::Event_SetActionPriority
================
*/
void sdObjectiveManagerLocal::Event_SetActionPriority( const char *actionName, bool isPriority ) {
	int actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum != -1 ) {
		botThreadData.botActions[ actionNum ]->SetActionPriority( isPriority );
		return;
	}

	gameLocal.DWarning( "Map Script Error! No valid bot action found by the name %s in \"setActionPriority\"", actionName );
}

/*
================
sdObjectiveManagerLocal::Event_SetActionHumanGoal
================
*/
void sdObjectiveManagerLocal::Event_SetActionHumanGoal( const char* actionName, const botActionGoals_t humanGoal ) {
	int actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum == -1 ) {
		gameLocal.DWarning( "Map Script Error! No valid bot action found by the name %s in \"setActionHumanGoal\"", actionName );
		return;
	}

	if ( humanGoal < ACTION_NULL || humanGoal >= ACTION_MAX_ACTIONS ) {
		gameLocal.DWarning( "Map Script Error! Invalid action goal passed to action %s in \"setActionHumanGoal\"", actionName );
		return;
	}

	botThreadData.botActions[ actionNum ]->SetHumanObj( humanGoal );
}

/*
================
sdObjectiveManagerLocal::Event_SetActionStroggGoal
================
*/
void sdObjectiveManagerLocal::Event_SetActionStroggGoal( const char* actionName, const botActionGoals_t stroggGoal ) {
	int actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum == -1 ) {
		gameLocal.DWarning( "Map Script Error! No valid bot action found by the name %s in \"setActionStroggGoal\"", actionName );
		return;
	}

	if ( stroggGoal < ACTION_NULL || stroggGoal >= ACTION_MAX_ACTIONS ) {
		gameLocal.DWarning( "Map Script Error! Invalid action goal passed to action %s in \"setActionStroggGoal\"", actionName );
		return;
	}

	botThreadData.botActions[ actionNum ]->SetStroggObj( stroggGoal );
}

/*
================
sdObjectiveManagerLocal::Event_SetBotActionVehicleType
================
*/
void sdObjectiveManagerLocal::Event_SetBotActionVehicleType( const char* actionName, int actionVehicleFlags ) {
	int actionNum = botThreadData.FindActionByName( actionName );

	if ( actionNum == -1 ) {
		gameLocal.DWarning( "Map Script Error! No valid bot action found by the name %s in \"setBotActionVehicleType\"", actionName );
		return;
	}

	botThreadData.botActions[ actionNum ]->SetActionVehicleFlags( actionVehicleFlags );
}

/*
================
sdObjectiveManagerLocal::Event_SetBotTeamRetreatTime
================
*/
void sdObjectiveManagerLocal::Event_SetBotTeamRetreatTime( const playerTeamTypes_t playerTeam, int retreatTime ) {
	if ( playerTeam < GDF || playerTeam > STROGG ) {
		gameLocal.DWarning( "Map Script Error! Invalid team passed into \"setBotTeamRetreatTime\"!" );
		return;
	}

	if ( playerTeam == GDF ) {
		botThreadData.GetGameWorldState()->botGoalInfo.teamRetreatInfo[ GDF ].retreatTime = gameLocal.time + ( retreatTime * 1000 );
	} else {
		botThreadData.GetGameWorldState()->botGoalInfo.teamRetreatInfo[ STROGG ].retreatTime = gameLocal.time + ( retreatTime * 1000 );
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		clientInfo_t& player = botThreadData.GetGameWorldState()->clientInfo[ i ];

		if ( !player.inGame ) {
			continue;
		}

		if ( player.team == NOTEAM ) {
			continue;
		}

		if ( !player.isBot ) {
			continue;
		}

		if ( player.isActor ) {
			continue;
		}

		if ( player.team != playerTeam ) {
			continue;
		}

		if ( player.health <= 0 ) {
			continue;
		}

		player.resetState = 2; //mal: now clear the bots AI, and have them fall back to defensive positions.
	}
}

/*
================
sdObjectiveManagerLocal::Event_ClearBotBoundEntities
================
*/
void sdObjectiveManagerLocal::Event_ClearBotBoundEntities( const playerTeamTypes_t playerTeam ) {
	if ( playerTeam < GDF || playerTeam > STROGG ) {
		gameLocal.DWarning( "Map Script Error! Invalid team passed into \"clearTeamBotBoundEntities\"!" );
		return;
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		const clientInfo_t& playerInfo = botThreadData.GetGameWorldState()->clientInfo[ i ];

		if ( playerInfo.inGame == false ) {
			continue;
		}

		if ( playerInfo.team == NOTEAM ) {
			continue;
		}

		if ( playerInfo.isBot  == false ) {
			continue;
		}

		if ( playerInfo.team != playerTeam ) {
			continue;
		}

		botThreadData.ClearClientBoundEntities( i );
	}
}

/*
================
sdObjectiveManagerLocal::Event_SetActionObjState
================
*/
void sdObjectiveManagerLocal::Event_SetActionObjState( const botActionStates_t state, const playerTeamTypes_t ownerTeam, idEntity* carryableObjective, idEntity* carrier ) {
	bool foundSlot = false;
	bool onGround = false;
	int i;
	int actionNumber = -1;

	if ( carryableObjective == NULL ) {
		gameLocal.DWarning( "An invalid carryable entity was passed to \"setActionObjState\"!");
		return;
	}

	if ( gameLocal.GameState() == GAMESTATE_SHUTDOWN ) {
		return;
	}

	if ( state == ACTION_STATE_OBJ_DELIVERED ) {
		for( i = 0; i < MAX_CARRYABLES; i++ ) {
			if ( botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].entNum != carryableObjective->entityNumber ) {
				continue;
			}

			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].onGround = false;
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].carrierEntNum = -1;
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].entNum = 0;
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].spawnID = -1;
			foundSlot = true;
			break;
		}

		if ( foundSlot == false ) {
			assert( false );
			gameLocal.DWarning( "Can't find matching slot for carryable objective %i in \"setActionObjState\"!", carryableObjective->entityNumber );
			return;
		}
	} else if ( state == ACTION_STATE_OBJ_STOLEN ) {
		if ( carrier == NULL ) {
			gameLocal.DWarning( "An invalid carrier client was passed to \"setActionObjState\"!" );
			return;
		}

		for( i = 0; i < MAX_CARRYABLES; i++ ) {
			if ( botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].entNum != carryableObjective->entityNumber ) {
				continue;
			} 

			onGround = botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].onGround;
			actionNumber = botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].parentActionNum;

			if ( scriptObject && !onGround && actionNumber != -1 ) { //mal: run a script event to let the LDs be able to change the bots state
				sdScriptHelper h1;
				h1.Push( botThreadData.botActions[ actionNumber ]->GetActionName() );
				scriptObject->CallNonBlockingScriptEvent( onCarryableItemStolenFunc, h1 );
			} 

			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].onGround = false;
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].carrierEntNum = carrier->entityNumber;

			Event_NotifyBotOfEvent( NOTEAM, NOCLASS, ACTION_STATE_OBJ_STOLEN );

			if ( botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].parentActionNum != -1 ) {
				botThreadData.botActions[ botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].parentActionNum ]->SetActionObjState( false );
			} else {
				assert( false );
				gameLocal.DWarning( "No action for found for carryable entity in \"setActionObjState\" for state STATE_STOLEN!" );
			}

			foundSlot = true;
			break;
		}

		if ( foundSlot == false ) {
			assert( false );
			gameLocal.DWarning( "Can't find matching slot for carryable objective %i in \"setActionObjState\"!", carryableObjective->entityNumber );
			return;
		}
	} else if ( state == ACTION_STATE_OBJ_RETURNED ) {
		for( i = 0; i < MAX_CARRYABLES; i++ ) {
			if ( botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].entNum != carryableObjective->entityNumber ) {
				continue;
			}

			onGround = botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].onGround;
			actionNumber = botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].parentActionNum;

			if ( scriptObject && actionNumber != -1 ) { //mal: run a script event to let the LDs be able to change the bots state
				sdScriptHelper h1;
				h1.Push( botThreadData.botActions[ actionNumber ]->GetActionName() );
				scriptObject->CallNonBlockingScriptEvent( onCarryableItemReturnedFunc, h1 );
			}

			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].onGround = false;
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].carrierEntNum = -1;

			Event_NotifyBotOfEvent( NOTEAM, NOCLASS, ACTION_STATE_OBJ_RETURNED );

			if ( botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].parentActionNum != -1 ) {
				botThreadData.botActions[ botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].parentActionNum ]->SetActionObjState( true );
			} else {
				assert( false );
				gameLocal.DWarning( "No action for found for carryable entity in \"setActionObjState\" for state STATE_RETURNED!" );
			}

			foundSlot = true;
			break;
		}

		if ( foundSlot == false ) {
			assert( false );
			gameLocal.DWarning("Can't find matching slot for carryable objective %i in \"setActionObjState\"!", carryableObjective->entityNumber );
			return;
		}
	} else if ( state == ACTION_STATE_OBJ_DROPPED ) {
		for( i = 0; i < MAX_CARRYABLES; i++ ) {
			if ( botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].entNum != carryableObjective->entityNumber ) {
				continue;
			}

			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].onGround = true;
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].carrierEntNum = -1;
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].origin = carryableObjective->GetPhysics()->GetOrigin();
			foundSlot = true;
			break;
		}

		Event_NotifyBotOfEvent( NOTEAM, NOCLASS, ACTION_STATE_OBJ_DROPPED );

		if ( foundSlot == false ) {
			assert( false );
			gameLocal.DWarning( "Can't find matching slot for carryable objective %i in \"setActionObjState\"!", carryableObjective->entityNumber );
			return;
		}
	} else if ( state == ACTION_STATE_OBJ_BORN ) {

		for( i = 0; i < MAX_CARRYABLES; i++ ) {
			if ( botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].entNum != 0 ) {
				continue;
			}

			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].entNum = carryableObjective->entityNumber;
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].spawnID = gameLocal.GetSpawnId( carryableObjective );
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].parentActionNum = botThreadData.FindActionByOrigin( ACTION_STEAL, carryableObjective->GetPhysics()->GetOrigin(), false );
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].ownerTeam = ownerTeam;
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].onGround = false;
			botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].carrierEntNum = -1;

			if ( botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].parentActionNum != -1 ) {
				botThreadData.botActions[ botThreadData.GetGameWorldState()->botGoalInfo.carryableObjs[ i ].parentActionNum ]->SetActionObjState( true );
			} else {
				assert( false );
				gameLocal.DWarning( "No action for found for carryable entity in \"setActionObjState\"! for state STATE_BORN" );
			}

			foundSlot = true;
			break;
		}

		if ( foundSlot == false ) {
			assert( false );
			gameLocal.DWarning( "No open slot was found for carryable entity %i in \"setActionObjState\"!", carryableObjective->entityNumber );
			return;
		}
	} else {
		gameLocal.DWarning( "Invalid action state passed to \"setActionObjState\"!" );
		return;
	}
}

/*
================
sdObjectiveManagerLocal::Event_SetSpawnActionOwner
================
*/
void sdObjectiveManagerLocal::Event_SetSpawnActionOwner( const playerTeamTypes_t playerTeam, idEntity* triggerEntity ) {

	int i, entNum;
	botActionGoals_t filter1, filter2;

	if ( triggerEntity == NULL ) {
		gameLocal.DWarning( "An invalid entity was passed to \"setSpawnActionOwner\"!" );
		return;
	}

	entNum = triggerEntity->entityNumber;

	filter1 = ACTION_DENY_SPAWNPOINT;
	filter2 = ACTION_FORWARD_SPAWN;

	for( i = 0; i < botThreadData.botActions.Num(); i++ ) {

        if ( !botThreadData.botActions[ i ]->ActionIsActive() ) { //mal: if action is turned off, ignore
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botThreadData.botActions[ i ]->GetHumanObj() != filter1 && botThreadData.botActions[ i ]->GetStroggObj() != filter1 && botThreadData.botActions[ i ]->GetHumanObj() != filter2 && botThreadData.botActions[ i ]->GetStroggObj() != filter2  ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->EntityIsInsideActionBBox( entNum, INGAME_PLAYER ) ) { //mal: in case of multiple objs of the same type
			continue;
		}

		if ( scriptObject ) { //mal: run a script event to let the LDs be able to change the bots state when this spawn is grabbed.
			sdScriptHelper h1;
			h1.Push( botThreadData.botActions[ i ]->GetActionName() );

			if ( playerTeam == NOTEAM ) {
				scriptObject->CallNonBlockingScriptEvent( onSpawnLiberatedFunc, h1 );
			} else {
				scriptObject->CallNonBlockingScriptEvent( onSpawnCapturedFunc, h1 );
			}
		} 

	   botThreadData.botActions[ i ]->SetActionTeamOwner( playerTeam );
	   break;
	}
}

/*
================
sdObjectiveManagerLocal::Event_EnableRouteGroup
================
*/
void sdObjectiveManagerLocal::Event_EnableRouteGroup( int routeGroupNum ) {

	int i;

	for( i = 0; i < botThreadData.botRoutes.Num(); i++ ) {

		if ( botThreadData.botRoutes[ i ]->GetRouteGroup() != routeGroupNum ) {
			continue;
		}

		botThreadData.botRoutes[ i ]->SetActive( true );
	}
}

/*
================
sdObjectiveManagerLocal::Event_DisableRouteGroup
================
*/
void sdObjectiveManagerLocal::Event_DisableRouteGroup( int routeGroupNum ) {

	int i;

	for( i = 0; i < botThreadData.botRoutes.Num(); i++ ) {

		if ( botThreadData.botRoutes[ i ]->GetRouteGroup() != routeGroupNum ) {
			continue;
		}

		botThreadData.botRoutes[ i ]->SetActive( false );
	}
}

/*
================
sdObjectiveManagerLocal::Event_EnableRoute
================
*/
void sdObjectiveManagerLocal::Event_EnableRoute( const char* routeName ) {

	int routeNum = botThreadData.FindRouteByName( routeName );

	if ( routeNum != -1 ) {
        botThreadData.botRoutes[ routeNum ]->SetActive( true );
		return;
	}

	gameLocal.DWarning( "No valid bot route found by the name %s", routeName );
}

/*
================
sdObjectiveManagerLocal::Event_DisableRoute
================
*/
void sdObjectiveManagerLocal::Event_DisableRoute( const char* routeName ) {

	int routeNum = botThreadData.FindRouteByName( routeName );

	if ( routeNum != -1 ) {
        botThreadData.botRoutes[ routeNum ]->SetActive( false );
		return;
	}

	gameLocal.DWarning( "No valid bot route found by the name %s", routeName );
}

/*
================
sdObjectiveManagerLocal::Event_SetTeamUseRearSpawn
================
*/
void sdObjectiveManagerLocal::Event_SetTeamUseRearSpawn( const playerTeamTypes_t playerTeam, bool useSpawn ) {
	if ( playerTeam == GDF ) {
		botThreadData.GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ GDF ].teamUsesSpawn = useSpawn;
	} else if ( playerTeam == STROGG ) {
		botThreadData.GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ STROGG ].teamUsesSpawn = useSpawn;
	} else {
		gameLocal.DWarning( "No valid team passed to \"setTeamUseRearSpawn\"!" );
	}
}

/*
================
sdObjectiveManagerLocal::Event_SetTeamMinePlantIsPriority
================
*/
void sdObjectiveManagerLocal::Event_SetTeamMinePlantIsPriority( const playerTeamTypes_t playerTeam, bool isPriority ) {
	if ( playerTeam == GDF ) {
		botThreadData.GetGameWorldState()->gameLocalInfo.teamMineInfo[ GDF ].isPriority = isPriority;
	} else if ( playerTeam == STROGG ) {
		botThreadData.GetGameWorldState()->gameLocalInfo.teamMineInfo[ STROGG ].isPriority = isPriority;
	} else {
		gameLocal.DWarning( "No valid team passed to \"setTeamMinePlantIsPriority\"!" );
	}
}

/*
================
sdObjectiveManagerLocal::Event_SetTeamUseRearSpawnPercentage
================
*/
void sdObjectiveManagerLocal::Event_SetTeamUseRearSpawnPercentage( const playerTeamTypes_t playerTeam, int percentageUsed ) {
	if ( playerTeam == GDF ) {
		botThreadData.GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ GDF ].percentage = percentageUsed;
	} else if ( playerTeam == STROGG ) {
		botThreadData.GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ STROGG ].percentage = percentageUsed;
	} else {
		gameLocal.DWarning( "No valid team passed to \"setTeamUseRearSpawnPercentage\"!" );
	}
}

/*
================
sdObjectiveManagerLocal::GetPlayerClass
================
*/
const sdDeclPlayerClass* sdObjectiveManagerLocal::GetPlayerClass( const playerTeamTypes_t playerTeam, const playerClassTypes_t neededClass ) {
	if ( playerTeam == GDF ) {
        if ( neededClass == MEDIC ) {
			return gameLocal.declPlayerClassType[ "medic" ];
		} else if ( neededClass == SOLDIER ) {
			return gameLocal.declPlayerClassType[ "soldier" ];
		} else if ( neededClass == ENGINEER ) {
			return gameLocal.declPlayerClassType[ "engineer" ];
		} else if ( neededClass == FIELDOPS ) {
			return gameLocal.declPlayerClassType[ "fieldops" ];
		} else if ( neededClass == COVERTOPS ) {
			return gameLocal.declPlayerClassType[ "covertops" ];
		} else {
			return NULL;
		}
	} else if ( playerTeam == STROGG ) {
		if ( neededClass == MEDIC ) {
			return gameLocal.declPlayerClassType[ "technician" ];
		} else if ( neededClass == SOLDIER ) {
			return gameLocal.declPlayerClassType[ "aggressor" ];
		} else if ( neededClass == ENGINEER ) {
			return gameLocal.declPlayerClassType[ "constructor" ];
		} else if ( neededClass == FIELDOPS ) {
			return gameLocal.declPlayerClassType[ "oppressor" ];
		} else if ( neededClass == COVERTOPS ) {
			return gameLocal.declPlayerClassType[ "infiltrator" ];
		} else {
			return NULL;
		}
	} else {
		return NULL;
	}
}

/*
================
sdObjectiveManagerLocal::Event_SetTeamNeededClass

Sets a bot(s) class to the needed class, if there aren't enough players of that class on the team.
Will look to do this from bots that have the "sampleClass" first - ex, if you know you dont need engs anymore, you could pull an eng and make
him a covert on a hack obj. If no bots of sample class are found, goes thru all the classes, until minNeeded is met.
Will try to avoid pulling medics if can help it. If a priority, bot will suicide to respawn right away as class, else will wait til get killed, and respawn
normally.
================
*/
void sdObjectiveManagerLocal::Event_SetTeamNeededClass( const playerTeamTypes_t playerTeam, const playerClassTypes_t neededClass, const playerClassTypes_t sampleClass, int minNeeded, bool priority, bool storeRequest ) {
	int i;
	int numClassOnTeam;
	int numConverted = 0;
	int weaponNum = 0;

	if ( playerTeam > STROGG || playerTeam < GDF ) {
		gameLocal.DWarning( "No valid team passed to \"setTeamNeededClass\"!" );
		return;
	}

	if ( !bot_allowClassChanges.GetBool() ) {
		return;
	}

	if ( gameLocal.rules->IsWarmup() ) {
		return;
	}

	if ( botThreadData.GetGameWorldState()->botGoalInfo.isTrainingMap ) {
		return;
	}

	if ( storeRequest ) {
		if ( playerTeam == GDF ) {
			botThreadData.GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ GDF ].criticalClass = neededClass;
			botThreadData.GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ GDF ].donatingClass = sampleClass;
			botThreadData.GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ GDF ].numCriticalClass = minNeeded;
		} else {
			botThreadData.GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ STROGG ].criticalClass = neededClass;
			botThreadData.GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ STROGG ].donatingClass = sampleClass;
			botThreadData.GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ STROGG ].numCriticalClass = minNeeded;
		}
	}

	if ( botThreadData.GetNumBotsInServer( playerTeam ) == 0 ) { //mal: not much point to this if theres no bots on the server!
		return;
	}

	numClassOnTeam = botThreadData.GetNumClassOnTeam( playerTeam, neededClass );

	if ( numClassOnTeam >= minNeeded ) { //mal: enough clients playing that class already, so leave.
		return;
	}

	const sdDeclPlayerClass* pc;
	
	if ( playerTeam == GDF ) {
        if ( neededClass == MEDIC ) {
			pc = gameLocal.declPlayerClassType[ "medic" ];
		} else if ( neededClass == SOLDIER ) {
			pc = gameLocal.declPlayerClassType[ "soldier" ];
		} else if ( neededClass == ENGINEER ) {
			pc = gameLocal.declPlayerClassType[ "engineer" ];
		} else if ( neededClass == FIELDOPS ) {
			pc = gameLocal.declPlayerClassType[ "fieldops" ];
		} else {
			pc = gameLocal.declPlayerClassType[ "covertops" ];
		}
	} else {
		if ( neededClass == MEDIC ) {
			pc = gameLocal.declPlayerClassType[ "technician" ];
		} else if ( neededClass == SOLDIER ) {
			pc = gameLocal.declPlayerClassType[ "aggressor" ];
		} else if ( neededClass == ENGINEER ) {
			pc = gameLocal.declPlayerClassType[ "constructor" ];
		} else if ( neededClass == FIELDOPS ) {
			pc = gameLocal.declPlayerClassType[ "oppressor" ];
		} else {
			pc = gameLocal.declPlayerClassType[ "infiltrator" ];
		}
	}

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !botThreadData.GetGameWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->clientInfo[ i ].team != playerTeam ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->clientInfo[ i ].classType == neededClass ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->clientInfo[ i ].classType != sampleClass ) {
			continue;
		}

		if ( !botThreadData.GetGameWorldState()->clientInfo[ i ].isBot ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->clientInfo[ i ].isActor ) {
			continue;
		}

		idPlayer* player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		numConverted++;

		player->ChangeClass( pc, weaponNum ); //mal: default loadout.

		if ( priority ) {
			player->Kill( NULL ); //mal: hurry up and suicide, and get back into the game as our new class!
		}

		if ( numClassOnTeam + numConverted >= minNeeded ) { //mal: found enough to switch over.
			break;
		} //mal: else, go around for another pass.
	}

	if ( numClassOnTeam + numConverted >= minNeeded ) { //mal: found enough to switch over.
		return;
	}

//mal: didn't find enough bots of class sampleClass, so now we'll start looking at other classes, avoiding medics
	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !botThreadData.GetGameWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->clientInfo[ i ].team != playerTeam ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->clientInfo[ i ].classType == neededClass ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->clientInfo[ i ].classType == MEDIC ) {
			continue;
		}

		if ( !botThreadData.GetGameWorldState()->clientInfo[ i ].isBot ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->clientInfo[ i ].isActor ) {
			continue;
		}

		idPlayer* player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		numConverted++;

		player->ChangeClass( pc, weaponNum ); //mal: default loadout.

		if ( priority ) {
			player->Kill( NULL ); //mal: hurry up and suicide, and get back into the game as our new class!
		}

		if ( numClassOnTeam + numConverted >= minNeeded ) { //mal: found enough to switch over.
			break;
		} //mal: else, go around for another pass.
	}

	if ( numClassOnTeam + numConverted >= minNeeded ) { //mal: found enough to switch over.
		return;
	}

//mal: this shouldn't ever really happen on a normal map, but just in case someone added 20 medics on one team, or theres only 1 or 2 bots on the server....
	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !botThreadData.GetGameWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->clientInfo[ i ].team != playerTeam ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->clientInfo[ i ].classType == neededClass ) {
			continue;
		}

		if ( !botThreadData.GetGameWorldState()->clientInfo[ i ].isBot ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->clientInfo[ i ].isActor ) {
			continue;
		}

		idPlayer* player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		numConverted++;

		player->ChangeClass( pc, weaponNum ); //mal: default loadout.

		if ( priority ) {
			player->Kill( NULL ); //mal: hurry up and suicide, and get back into the game as our new class!
		}

		if ( numClassOnTeam + numConverted >= minNeeded ) { //mal: found enough to switch over.
			break;
		} //mal: else, go around for another pass.
	}
}


/*
================
sdObjectiveManagerLocal::Event_EnableNodes
================
*/
void sdObjectiveManagerLocal::Event_EnableNodes( const char * nodeName ) {
	botThreadData.botVehicleNodes.ActivateNodes( nodeName, true );
}

/*
================
sdObjectiveManagerLocal::Event_DisableNodes
================
*/
void sdObjectiveManagerLocal::Event_DisableNodes( const char * nodeName ) {
	botThreadData.botVehicleNodes.ActivateNodes( nodeName, false );
}

/*
================
sdObjectiveManagerLocal::Event_SetNodeTeam
================
*/
void sdObjectiveManagerLocal::Event_SetNodeTeam( const char * nodeName, const playerTeamTypes_t playerTeam ) {
	botThreadData.botVehicleNodes.SetNodeTeam( nodeName, playerTeam );
}

/*
================
sdObjectiveManagerLocal::Event_GetBotCriticalClass
================
*/
void sdObjectiveManagerLocal::Event_GetBotCriticalClass( const playerTeamTypes_t playerTeam ) {
	if ( playerTeam == GDF ) {
		sdProgram::ReturnInteger( gdfCriticalClass );
	} else if ( playerTeam == STROGG ) {
		sdProgram::ReturnInteger( stroggCriticalClass );
	} else {
		sdProgram::ReturnInteger( NOCLASS );
	}
}

/*
================
sdObjectiveManagerLocal::Event_GetNumClassPlayers
================
*/
void sdObjectiveManagerLocal::Event_GetNumClassPlayers( const playerTeamTypes_t playerTeam, const playerClassTypes_t playerClass ) {
	const sdDeclPlayerClass* classDecl = GetPlayerClass( playerTeam, playerClass );
	if ( classDecl == NULL ) {
		sdProgram::ReturnInteger( 0 );
		return;
	}

	int count = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL || player->GetGameTeam() == NULL ) {
			continue;
		}

		if ( player->GetInventory().GetClass() == classDecl ) {
			count++;
		}
	}

	sdProgram::ReturnInteger( count );
}

/*
================
sdObjectiveManagerLocal::PlayerDeployableDeployed
================
*/
void sdObjectiveManagerLocal::PlayerDeployableDeployed() {
	if ( scriptObject ) {
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( onDeployableDeployedFunc, h1 );
	}
}

/*
================
sdObjectiveManagerLocal::GetSpectateEntity
================
*/
idEntity* sdObjectiveManagerLocal::GetSpectateEntity( void ) {
	if ( scriptObject != NULL ) {
		if ( getSpectateEntityFunc != NULL ) {
			sdScriptHelper h1;
			scriptObject->CallNonBlockingScriptEvent( getSpectateEntityFunc, h1 );
			idScriptObject* object = gameLocal.program->GetReturnedObject();
			if ( object != NULL ) {
				return object->GetClass()->Cast< idEntity >();
			}
		}
	}
	return NULL;
}
