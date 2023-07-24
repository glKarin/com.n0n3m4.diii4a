// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "GameRules_Objective.h"
#include "../Player.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../guis/UserInterfaceLocal.h"
#include "../rules/VoteManager.h"

sdGameRulesObjectiveNetworkState::sdGameRulesObjectiveNetworkState( void ) {
}

void sdGameRulesObjectiveNetworkState::MakeDefault( void ) {
	sdGameRulesNetworkState::MakeDefault();

	winningTeam	= NULL;
}

void sdGameRulesObjectiveNetworkState::Write( idFile* file ) const {
	sdGameRulesNetworkState::Write( file );

	file->WriteInt( winningTeam ? winningTeam->GetIndex() : -1 );
}

void sdGameRulesObjectiveNetworkState::Read( idFile* file ) {
	sdGameRulesNetworkState::Read( file );

	int winningTeamIndex;
	file->ReadInt( winningTeamIndex );
	winningTeam = winningTeamIndex == -1 ? NULL : &sdTeamManager::GetInstance().GetTeamByIndex( winningTeamIndex );
}

/*
===============================================================================

	sdGameRulesObjective

===============================================================================
*/

CLASS_DECLARATION( sdGameRules, sdGameRulesObjective )
END_CLASS

/*
================
sdGameRulesObjective::sdGameRulesObjective
================
*/
sdGameRulesObjective::sdGameRulesObjective( void ) : winningTeam( NULL ) {
}

/*
================
sdGameRulesObjective::~sdGameRulesObjective
================
*/
sdGameRulesObjective::~sdGameRulesObjective( void ) {
}

/*
================
sdGameRulesObjective::GameState_Review
================
*/
void sdGameRulesObjective::GameState_Review( void ) {
	if ( nextState != GS_INACTIVE ) {
		return;
	}

	if ( si_gameReviewReadyWait.GetBool() ) {
		if ( ArePlayersReady() != RS_READY ) {
			return;
		}
	}

	NextStateDelayed( GS_NEXTMAP, MINS2MS( g_gameReviewPause.GetFloat() ) );
}

/*
================
sdGameRulesObjective::GameState_NextGame
================
*/
void sdGameRulesObjective::GameState_NextGame( void ) {
	if ( nextState != GS_INACTIVE ) {
		return;
	}

	NewState( GS_WARMUP );

	// put everyone back in from endgame spectate
	CheckRespawns( true );
}

/*
================
sdGameRulesObjective::GameState_Warmup
================
*/
void sdGameRulesObjective::GameState_Warmup( void ) {
	if ( !CanStartMatch() ) {
		needsRestart = true;
		return;
	}
	StartMatch();
}

/*
================
sdGameRulesObjective::GameState_Countdown
================
*/
void sdGameRulesObjective::GameState_Countdown( void ) {
}

/*
================
sdGameRulesObjective::GameState_GameOn
================
*/
void sdGameRulesObjective::GameState_GameOn( void ) {
	if ( gameLocal.GameState() == GAMESTATE_STARTUP || nextState != GS_INACTIVE ) {
		return;
	}

	int timeLimit = GetTimeLimit();
	if ( timeLimit != 0 ) {
		if ( gameLocal.time - matchStartedTime >= timeLimit ) {
			OnTimeLimitHit();
			EndGame();
		}
	}
}

/*
================
sdGameRulesObjective::GameState_NextMap
================
*/
void sdGameRulesObjective::GameState_NextMap( void ) {
	winningTeam = NULL;

	BackupPlayerTeams();

	if ( gameLocal.NextMap() ) {
		return;
	}

	gameLocal.LocalMapRestart();

	sdTeamManagerLocal& manager = sdTeamManager::GetInstance();

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		player->GetProficiencyTable().Clear( false );
		player->Killed( NULL, NULL, 0, vec3_zero, -1, NULL );
		player->ServerForceRespawn( true );
	}

	NewState( GS_NEXTGAME );
}


/*
================
sdGameRulesObjective::OnGameState_Review
================
*/
void sdGameRulesObjective::OnGameState_Review( void ) {
	if ( !gameLocal.isClient ) {
		nextState = GS_INACTIVE;	// used to abort a game. cancel out any upcoming state change
		ClearPlayerReadyFlags();
	}
}

/*
================
sdGameRulesObjective::OnGameState_Countdown
================
*/
void sdGameRulesObjective::OnGameState_Countdown( void ) {
}

/*
================
sdGameRulesObjective::OnGameState_GameOn
================
*/
void sdGameRulesObjective::OnGameState_GameOn( void ) {
	if ( !gameLocal.isClient ) {
		if ( needsRestart ) {
			needsRestart = false;
			gameLocal.LocalMapRestart();
		}

		matchStartedTime = gameLocal.time;
	}
}

/*
================
sdGameRulesObjective::OnGameState_NextMap
================
*/
void sdGameRulesObjective::OnGameState_NextMap( void ) {
	ClearPlayerReadyFlags();
}


/*
================
sdGameRulesObjective::EndGame
================
*/
void sdGameRulesObjective::EndGame( void ) {
	if ( IsWarmup() ) {
		return;
	}

	NextStateDelayed( GS_GAMEREVIEW, 1000 );

	RecordWinningTeam( winningTeam, "maps", true );

	CallScriptEndGame();
}

/*
================
sdGameRulesObjective::ApplyNetworkState
================
*/
void sdGameRulesObjective::ApplyNetworkState( const sdEntityStateNetworkData& newState ) {
	sdGameRules::ApplyNetworkState( newState );

	NET_GET_NEW( sdGameRulesObjectiveNetworkState );

	SetWinner( newData.winningTeam );
}

/*
================
sdGameRulesObjective::ReadNetworkState
================
*/
void sdGameRulesObjective::ReadNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	sdGameRules::ReadNetworkState( baseState, newState, msg );

	NET_GET_STATES( sdGameRulesObjectiveNetworkState );

	sdTeamManagerLocal& teamManager = sdTeamManager::GetInstance();

	newData.winningTeam	= teamManager.ReadTeamFromStream( baseData.winningTeam, msg );
}

/*
================
sdGameRulesObjective::WriteNetworkState
================
*/
void sdGameRulesObjective::WriteNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	sdGameRules::WriteNetworkState( baseState, newState, msg );

	NET_GET_STATES( sdGameRulesObjectiveNetworkState );

	newData.winningTeam		= GetWinningTeam();

	sdTeamManagerLocal& teamManager = sdTeamManager::GetInstance();

	teamManager.WriteTeamToStream( baseData.winningTeam, newData.winningTeam, msg );
}

/*
================
sdGameRulesObjective::CheckNetworkStateChanges
================
*/
bool sdGameRulesObjective::CheckNetworkStateChanges( const sdEntityStateNetworkData& baseState ) const {
	if ( sdGameRules::CheckNetworkStateChanges( baseState ) ) {
		return true;
	}

	NET_GET_BASE( sdGameRulesObjectiveNetworkState );

	NET_CHECK_FIELD( winningTeam, GetWinningTeam() );
	
	return false;
}

/*
================
sdGameRulesObjective::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdGameRulesObjective::CreateNetworkStructure( void ) const {
	return new sdGameRulesObjectiveNetworkState();
}


/*
================
sdGameRulesObjective::SetWinner
================
*/
void sdGameRulesObjective::SetWinner( sdTeamInfo* team ) {
	if ( !gameLocal.isClient && ( gameState == GS_GAMEREVIEW ) ) {
		gameLocal.Warning( "Attempted to set winning team after game has ended" );
		return;
	}

	if ( winningTeam == team ) {
		return;
	}

	winningTeam = team;
	if( gameLocal.DoClientSideStuff() ) {
		UpdateClientFromServerInfo( gameLocal.serverInfo, false );
	}
}

/*
================
sdGameRulesObjective::ChangeMap
================
*/
bool sdGameRulesObjective::ChangeMap( const char* mapName ) {
	idStr cleanMapName = mapName;
	sdGameRules_SingleMapHelper::SanitizeMapName( cleanMapName, true );

	idStr sessionCommand = va( "usermap %s", cleanMapName.c_str() );
	cmdSystem->PushFrameCommand( sessionCommand.c_str() );
	return true;
}

/*
================
sdGameRulesObjective::OnUserStartMap
================
*/	
userMapChangeResult_e sdGameRulesObjective::OnUserStartMap( const char* text, idStr& reason, idStr& mapName ) {
	return sdGameRules_SingleMapHelper::OnUserStartMap( text, reason, mapName );
}

/*
================
sdGameRulesObjective::GetGameTime
================
*/
int sdGameRulesObjective::GetGameTime( void ) const {
	int ms;	

	if ( gameState == GS_WARMUP ) {
		ms = 0;
	} else if ( gameState == GS_COUNTDOWN ) {
		ms = nextStateSwitch - gameLocal.time;
	} else {
		ms = GetTimeLimit() - ( gameLocal.time - matchStartedTime );
		if ( ms < 0 ) {
			ms = 0;
		}
	}

	return ms;
}

/*
================
sdGameRulesObjective::GetTypeText
================
*/
const sdDeclLocStr*	sdGameRulesObjective::GetTypeText( void ) const {
	return declHolder.declLocStrType[ "game/gametype/objective" ];
}

/*
================
sdGameRulesObjective::ArgCompletion_StartGame
================
*/
void sdGameRulesObjective::ArgCompletion_StartGame( const idCmdArgs& args, argCompletionCallback_t callback ) {
	sdGameRules_SingleMapHelper::ArgCompletion_StartGame( args, callback );
}

/*
============
sdGameRulesObjective::UpdateClientFromServerInfo
============
*/
void sdGameRulesObjective::UpdateClientFromServerInfo( const idDict& serverInfo, bool allowMedia ) {
	sdGameRules::UpdateClientFromServerInfo( serverInfo, allowMedia );

	idStr mapName = serverInfo.GetString( "si_map" );
	if( mapName.IsEmpty() ) {
		return;
	}
	mapName.StripFileExtension();

	using namespace sdProperties;

	// update status
	if ( sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "campaignInfo" ) ) {
		const idDict* metaData = gameLocal.mapMetaDataList->FindMetaData( mapName, &gameLocal.defaultMetaData );

		if( allowMedia ) {
			const sdDeclMapInfo* mapInfo = gameLocal.declMapInfoType.LocalFind( metaData->GetString( "mapinfo", "_default" ) );
			
			// setup the backdrop
			if ( sdProperty* property = scope->GetProperty( "backdrop", PT_STRING ) ) {
				const char* value = mapInfo->GetData().GetString( "mtr_backdrop", "guis/assets/black" );
				*property->value.stringValue = value;
			}

			const char* status = "current";
			if( winningTeam != NULL ) {
				status = winningTeam->GetLookupName();
			}

			SetupLoadScreenUI( *scope, status, true, 1, *metaData, mapInfo );
		}

		// setup the name
		if ( sdProperty* property = scope->GetProperty( "name", PT_WSTRING ) ) {
			*property->value.wstringValue = va( L"%hs", metaData->GetString( "pretty_name" ) );
		}

		// setup the number of maps
		if ( sdProperty* property = scope->GetProperty( "numMaps", PT_FLOAT ) ) {
			*property->value.floatValue = 1.0f;
		}

		if ( sdProperty* property = scope->GetProperty( "currentMap", PT_FLOAT ) ) {
			*property->value.floatValue = 1.0f;
		}

		// setup the status
		if ( sdProperty* property = scope->GetProperty( "ruleStatus", PT_WSTRING ) ) {
			*property->value.wstringValue = L"";
		}
	}
}

/*
============
sdGameRulesObjective::Clear
============
*/
void sdGameRulesObjective::Clear( void ) {
	sdGameRules::Clear();
	winningTeam = NULL;
}

/*
============
sdGameRulesObjective::GetServerBrowserScore
============
*/
int sdGameRulesObjective::GetServerBrowserScore( const sdNetSession& session ) const {
	int score = 0;

	score += sdHotServerList::BROWSER_GOOD_BONUS - 1;

	score += sdGameRules::GetServerBrowserScore( session );

	return score;
}
