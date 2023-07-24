// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "GameRules_StopWatch.h"
#include "../Player.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../guis/UserInterfaceLocal.h"
#include "../rules/VoteManager.h"

idCVar g_stopWatchMode( "g_stopWatchMode", "0", CVAR_GAME | CVAR_INTEGER, "stopwatch mode, 0 = ABBA, 1 = ABAB" );

sdGameRulesStopWatchNetworkState::sdGameRulesStopWatchNetworkState( void ) {
}

void sdGameRulesStopWatchNetworkState::MakeDefault( void ) {
	sdGameRulesNetworkState::MakeDefault();

	progression = 0;
	timeToBeat	= 0;
	winReason	= 0;
	winningTeam	= NULL;
}

void sdGameRulesStopWatchNetworkState::Write( idFile* file ) const {
	sdGameRulesNetworkState::Write( file );

	file->WriteInt( winningTeam ? winningTeam->GetIndex() : -1 );
	file->WriteInt( progression );
	file->WriteInt( timeToBeat );
	file->WriteInt( winReason );
}

void sdGameRulesStopWatchNetworkState::Read( idFile* file ) {
	sdGameRulesNetworkState::Read( file );

	int winningTeamIndex;
	file->ReadInt( winningTeamIndex );
	winningTeam = winningTeamIndex == -1 ? NULL : &sdTeamManager::GetInstance().GetTeamByIndex( winningTeamIndex );

	file->ReadInt( progression );
	file->ReadInt( timeToBeat );
	file->ReadInt( winReason );
}

/*
===============================================================================

	sdGameRulesStopWatch

===============================================================================
*/

CLASS_DECLARATION( sdGameRules, sdGameRulesStopWatch )
END_CLASS

/*
================
sdGameRulesStopWatch::sdGameRulesStopWatch
================
*/
sdGameRulesStopWatch::sdGameRulesStopWatch( void ) : timeToBeat( 0 ), winningTeam( NULL ), firstMatchWinningTeam( NULL ), progression( GP_FIRST_MATCH ) {
	for ( int i = 0; i < GP_MAX; i++ ) {
		// pre-allocate plenty of objectives
		objectiveCompletionTimes[ 0 ].PreAllocate( 8 );
		objectiveCompletionTimes[ 1 ].PreAllocate( 8 );
		attackingTeams[ i ] = NULL;
		defendingTeams[ i ] = NULL;
		attackingTeamXPs[ i ] = -1;
	}

	winReason = WR_NORMAL;
}

/*
================
sdGameRulesStopWatch::~sdGameRulesStopWatch
================
*/
sdGameRulesStopWatch::~sdGameRulesStopWatch( void ) {
}

/*
================
sdGameRulesStopWatch::GameState_Review
================
*/
void sdGameRulesStopWatch::GameState_Review( void ) {
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
sdGameRulesStopWatch::GameState_NextGame
================
*/
void sdGameRulesStopWatch::GameState_NextGame( void ) {
	if ( nextState != GS_INACTIVE ) {
		return;
	}

	NewState( GS_WARMUP );

	// put everyone back in from endgame spectate
	CheckRespawns( true );
}

/*
================
sdGameRulesStopWatch::GameState_Warmup
================
*/
void sdGameRulesStopWatch::GameState_Warmup( void ) {
	if ( !CanStartMatch() ) {
		needsRestart = true;
		return;
	}
	StartMatch();
}

/*
================
sdGameRulesStopWatch::GameState_Countdown
================
*/
void sdGameRulesStopWatch::GameState_Countdown( void ) {
}

/*
================
sdGameRulesStopWatch::GameState_GameOn
================
*/
void sdGameRulesStopWatch::GameState_GameOn( void ) {
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
sdGameRulesStopWatch::GameState_NextMap
================
*/
void sdGameRulesStopWatch::GameState_NextMap( void ) {
	BackupPlayerTeams();

	switch ( progression ) {
		case GP_FIRST_MATCH:
			progression = GP_RETURN_MATCH;
			firstMatchWinningTeam = winningTeam;
			break;
		case GP_RETURN_MATCH:
			if ( gameLocal.NextMap() ) {
				return;
			}

			firstMatchWinningTeam = NULL;
			progression = GP_FIRST_MATCH;
			timeToBeat = 0;
			ResetTiebreakInfo();
			winReason = WR_NORMAL;
			break;
	}

	if( gameLocal.DoClientSideStuff() ) {
		UpdateClientFromServerInfo( gameLocal.serverInfo, false );				
	}

	winningTeam = NULL;

	gameLocal.LocalMapRestart();

	sdTeamManagerLocal& manager = sdTeamManager::GetInstance();

	bool stopWatchAltMode = g_stopWatchMode.GetInteger() != 0;

	sdProficiencyManager::GetInstance().ClearProficiency();

	// cycle player teams and clear xp
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		sdTeamInfo* currentTeam = player->GetGameTeam();
		if ( currentTeam == NULL ) {
			continue;
		}


		player->Killed( NULL, NULL, 0, vec3_zero, -1, NULL );

		if ( !stopWatchAltMode ) {
			int newTeamIndex = ( currentTeam->GetIndex() + 1 ) % manager.GetNumTeams();
			sdTeamInfo* newTeam = &manager.GetTeamByIndex( newTeamIndex );

			const sdDeclPlayerClass* cls = player->GetInventory().GetClass();
			if ( cls ) {
				cls = currentTeam->GetEquivalentClass( *cls, *newTeam );
			}
			player->SetGameTeam( newTeam );
			if ( cls ) {
				player->GetInventory().GiveClass( cls, true ); 
			}
		}

		player->ServerForceRespawn( true );
		RestoreFireTeam( player );
	}

	NewState( GS_NEXTGAME );
}


/*
================
sdGameRulesStopWatch::OnGameState_Review
================
*/
void sdGameRulesStopWatch::OnGameState_Review( void ) {
	if ( !gameLocal.isClient ) {
		nextState = GS_INACTIVE;	// used to abort a game. cancel out any upcoming state change
		ClearPlayerReadyFlags();
	}
}

/*
================
sdGameRulesStopWatch::OnGameState_Countdown
================
*/
void sdGameRulesStopWatch::OnGameState_Countdown( void ) {
}

/*
================
sdGameRulesStopWatch::OnGameState_GameOn
================
*/
void sdGameRulesStopWatch::OnGameState_GameOn( void ) {
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
sdGameRulesStopWatch::OnGameState_NextMap
================
*/
void sdGameRulesStopWatch::OnGameState_NextMap( void ) {
	ClearPlayerReadyFlags();
}


/*
================
sdGameRulesStopWatch::EndGame
================
*/
void sdGameRulesStopWatch::EndGame( void ) {
	if ( IsWarmup() ) {
		return;
	}

	NextStateDelayed( GS_GAMEREVIEW, 1000 );

	if ( !gameLocal.isClient ) {
		if ( progression == GP_FIRST_MATCH ) {
			timeToBeat = gameLocal.time - matchStartedTime;
			
			if ( attackingTeams[ progression ] != NULL ) {
				attackingTeamXPs[ progression ] = attackingTeams[ progression ]->GetTotalXP();
			}
		} else {
			// figure out who really won the match by comparing the objective completions etc...

			RecordWinningTeam( winningTeam == firstMatchWinningTeam ? winningTeam : NULL, "stopwatch", false );
			timeToBeat = 0;
		}

		CallScriptEndGame();
	}

	if( gameLocal.DoClientSideStuff() ) {
		UpdateClientFromServerInfo( gameLocal.serverInfo, false );
	}
}

/*
================
sdGameRulesStopWatch::ApplyNetworkState
================
*/
void sdGameRulesStopWatch::ApplyNetworkState( const sdEntityStateNetworkData& newState ) {
	sdGameRules::ApplyNetworkState( newState );

	NET_GET_NEW( sdGameRulesStopWatchNetworkState );

	SetWinner( newData.winningTeam );

	bool changed	= ( progression != ( gameProgression_t )newData.progression );
	progression		= ( gameProgression_t )newData.progression;

	changed			|= ( timeToBeat != newData.timeToBeat );
	timeToBeat		= newData.timeToBeat;

	changed			|= ( winReason != ( winReason_t )newData.winReason );
	winReason		= ( winReason_t )newData.winReason;

	if( changed ) {
		UpdateClientFromServerInfo( gameLocal.serverInfo, false );
	}
}

/*
================
sdGameRulesStopWatch::ReadNetworkState
================
*/
void sdGameRulesStopWatch::ReadNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	sdGameRules::ReadNetworkState( baseState, newState, msg );

	NET_GET_STATES( sdGameRulesStopWatchNetworkState );

	sdTeamManagerLocal& teamManager = sdTeamManager::GetInstance();

	newData.winningTeam	= teamManager.ReadTeamFromStream( baseData.winningTeam, msg );
	newData.progression = msg.ReadDeltaLong( baseData.progression );
	newData.timeToBeat	= msg.ReadDeltaLong( baseData.timeToBeat );
	newData.winReason	= msg.ReadDeltaLong( baseData.winReason );
}

/*
================
sdGameRulesStopWatch::WriteNetworkState
================
*/
void sdGameRulesStopWatch::WriteNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	sdGameRules::WriteNetworkState( baseState, newState, msg );

	NET_GET_STATES( sdGameRulesStopWatchNetworkState );

	newData.winningTeam		= GetWinningTeam();
	newData.progression		= progression;
	newData.timeToBeat		= timeToBeat;
	newData.winReason		= winReason;

	sdTeamManagerLocal& teamManager = sdTeamManager::GetInstance();

	teamManager.WriteTeamToStream( baseData.winningTeam, newData.winningTeam, msg );
	msg.WriteDeltaLong( baseData.progression, newData.progression );
	msg.WriteDeltaLong( baseData.timeToBeat, newData.timeToBeat );
	msg.WriteDeltaLong( baseData.winReason, newData.winReason );
}

/*
================
sdGameRulesStopWatch::CheckNetworkStateChanges
================
*/
bool sdGameRulesStopWatch::CheckNetworkStateChanges( const sdEntityStateNetworkData& baseState ) const {
	if ( sdGameRules::CheckNetworkStateChanges( baseState ) ) {
		return true;
	}

	NET_GET_BASE( sdGameRulesStopWatchNetworkState );

	NET_CHECK_FIELD( winningTeam, GetWinningTeam() );
	NET_CHECK_FIELD( progression, progression );
	NET_CHECK_FIELD( timeToBeat, timeToBeat );
	NET_CHECK_FIELD( winReason, winReason );

	return false;
}

/*
================
sdGameRulesStopWatch::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdGameRulesStopWatch::CreateNetworkStructure( void ) const {
	return new sdGameRulesStopWatchNetworkState();
}


/*
================
sdGameRulesStopWatch::SetWinner
================
*/
void sdGameRulesStopWatch::SetWinner( sdTeamInfo* team ) {
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
sdGameRulesStopWatch::ChangeMap
================
*/
bool sdGameRulesStopWatch::ChangeMap( const char* mapName ) {
	idStr cleanMapName = mapName;
	sdGameRules_SingleMapHelper::SanitizeMapName( cleanMapName, true );

	idStr sessionCommand = va( "usermap %s", cleanMapName.c_str() );
	cmdSystem->PushFrameCommand( sessionCommand.c_str() );
	return true;
}

/*
================
sdGameRulesStopWatch::OnUserStartMap
================
*/
userMapChangeResult_e sdGameRulesStopWatch::OnUserStartMap( const char* text, idStr& reason, idStr& mapName ) {
	return sdGameRules_SingleMapHelper::OnUserStartMap( text, reason, mapName );
}

/*
================
sdGameRulesStopWatch::GetGameTime
================
*/
int sdGameRulesStopWatch::GetGameTime( void ) const {
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
sdGameRulesStopWatch::Reset
================
*/
void sdGameRulesStopWatch::Reset( void ) {
	sdGameRules::Reset();
	progression = GP_FIRST_MATCH;
	firstMatchWinningTeam = NULL;
	timeToBeat = 0;
	winReason = WR_NORMAL;
	if( gameLocal.DoClientSideStuff() ) {
		statusText = common->LocalizeText( "guis/hud/stopwatch_1" );
		UpdateClientFromServerInfo( gameLocal.serverInfo, false );
	}
	ResetTiebreakInfo();
}

/*
================
sdGameRulesStopWatch::GetTimeLimit
================
*/
int sdGameRulesStopWatch::GetTimeLimit( void ) const {
	switch ( progression ) {
		default:
		case GP_FIRST_MATCH:
			return sdGameRules::GetTimeLimit();
		case GP_RETURN_MATCH:
			return timeToBeat;
	}
}

/*
================
sdGameRulesStopWatch::GetTypeText
================
*/
const sdDeclLocStr*	sdGameRulesStopWatch::GetTypeText( void ) const {
	return declHolder.declLocStrType[ "game/gametype/stopwatch" ];
}

/*
================
sdGameRulesStopWatch::ArgCompletion_StartGame
================
*/
void sdGameRulesStopWatch::ArgCompletion_StartGame( const idCmdArgs& args, argCompletionCallback_t callback ) {
	sdGameRules_SingleMapHelper::ArgCompletion_StartGame( args, callback );
}

/*
============
sdGameRulesStopWatch::UpdateClientFromServerInfo
============
*/
void sdGameRulesStopWatch::UpdateClientFromServerInfo( const idDict& serverInfo, bool allowMedia ) {
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
				*property->value.stringValue = mapInfo->GetData().GetString( "mtr_backdrop", "guis/assets/black" );
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

		if ( sdProperty* property = scope->GetProperty( "numMaps", PT_FLOAT ) ) {
			*property->value.floatValue = 1.0f;
		}

		if ( sdProperty* property = scope->GetProperty( "currentMap", PT_FLOAT ) ) {
			*property->value.floatValue = 1.0f;
		}

		idWStr text;
		if( timeToBeat > 0 ) {
			idWStr::hmsFormat_t format;
			format.showZeroMinutes = true;
			idWStrList args( 1 );

			args.Append( idWStr::MS2HMS( timeToBeat, format ) );
			text = common->LocalizeText( "guis/mainmenu/timetobeat", args );
		}
		// setup the status
		if ( sdProperty* property = scope->GetProperty( "ruleStatus", PT_WSTRING ) ) {
			*property->value.wstringValue = text;
		}	
	}
}

/*
============
sdGameRulesStopWatch::Clear
============
*/
void sdGameRulesStopWatch::Clear( void ) {
	sdGameRules::Clear();
	winningTeam = NULL;
	winReason = WR_NORMAL;
}

/*
============
sdGameRulesStopWatch::GetProbeState
============
*/
byte sdGameRulesStopWatch::GetProbeState( void ) const {
	byte value = sdGameRules::GetProbeState();
	if( progression == GP_RETURN_MATCH ) {
		value |= PGS_RETURN;
	}
	return value;
}

/*
============
sdGameRulesStopWatch::GetServerBrowserScore
============
*/
int sdGameRulesStopWatch::GetServerBrowserScore( const sdNetSession& session ) const {
	int score = 0;
	if ( session.GetGameState() & PGS_RETURN ) {
		score += sdHotServerList::BROWSER_OK_BONUS;
	} else {
		score += sdHotServerList::BROWSER_GOOD_BONUS;
	}

	score += sdGameRules::GetServerBrowserScore( session );
	return score;
}

/*
============
sdGameRulesStopWatch::GetBrowserStatusString
============
*/
void sdGameRulesStopWatch::GetBrowserStatusString( idWStr& str, const sdNetSession& netSession ) const {
	str.Clear();

	sdGameRules::GetBrowserStatusString( str, netSession );
	str += L" - ";

	if( netSession.GetGameState() & PGS_RETURN ) {
		str += declHolder.declLocStrType.LocalFind( "guis/hud/stopwatch_2" )->GetText();
	} else {
		str += declHolder.declLocStrType.LocalFind( "guis/hud/stopwatch_1" )->GetText();
	}
}

/*
============
sdGameRulesStopWatch::InhibitEntitySpawn
============
*/
bool sdGameRulesStopWatch::InhibitEntitySpawn( idDict &spawnArgs ) const {
	if ( spawnArgs.GetBool( "noStopwatch" ) ) {
		return true;
	}

	return false;
}

/*
============
sdGameRulesStopWatch::ResetTiebreakInfo
============
*/
void sdGameRulesStopWatch::ResetTiebreakInfo( void ) {
	for ( int i = 0; i < GP_MAX; i++ ) {
		attackingTeams[ i ] = NULL;
		defendingTeams[ i ] = NULL;
		attackingTeamXPs[ i ] = -1;
		objectiveCompletionTimes[ i ].SetNum( 0, false );
	}

	winReason = WR_NORMAL;
}

/*
============
sdGameRulesStopWatch::OnObjectiveCompletion
============
*/
void sdGameRulesStopWatch::OnObjectiveCompletion( int objectiveIndex ) {
	// record the amount of time each team took to do the objectives
	objectiveCompletionTimes[ progression ].AssureSize( objectiveIndex + 1 );
	objectiveCompletionTimes[ progression ][ objectiveIndex ] = gameLocal.time - matchStartedTime;
}

/*
============
sdGameRulesStopWatch::SetAttacker
============
*/
void sdGameRulesStopWatch::SetAttacker( sdTeamInfo* team ) {
	attackingTeams[ progression ] = team;
}

/*
============
sdGameRulesStopWatch::SetDefender
============
*/
void sdGameRulesStopWatch::SetDefender( sdTeamInfo* team ) {
	defendingTeams[ progression ] = team;
}

/*
============
sdGameRulesStopWatch::OnTimeLimitHit
============
*/
void sdGameRulesStopWatch::OnTimeLimitHit( void ) {
	sdGameRules::OnTimeLimitHit();

	if ( !gameLocal.isClient ) {
		if ( progression == GP_RETURN_MATCH ) {
			if ( attackingTeams[ GP_FIRST_MATCH ] == NULL || attackingTeams[ GP_RETURN_MATCH ] == NULL ) {
				assert( false );
				gameLocal.Warning( "sdGameRulesStopWatch::OnTimeLimitHit - hit timelimit in return match with an attacking team being NULL!" );
				return;
			}
			if ( defendingTeams[ GP_FIRST_MATCH ] == NULL || defendingTeams[ GP_RETURN_MATCH ] == NULL ) {
				assert( false );
				gameLocal.Warning( "sdGameRulesStopWatch::OnTimeLimitHit - hit timelimit in return match with a defending team being NULL!" );
				return;
			}

			// record the XP
			attackingTeamXPs[ progression ] = attackingTeams[ progression ]->GetTotalXP();
			
			// time ran out without this team having completed all the objectives!
			sdTeamInfo* firstRoundAttackTeam = defendingTeams[ GP_RETURN_MATCH ];
			sdTeamInfo* returnRoundAttackTeam = attackingTeams[ GP_RETURN_MATCH ];

			// if either team has completed more objectives then they are the winner
			// (this also takes into account finishing the map)
			int firstMatchNum = objectiveCompletionTimes[ GP_FIRST_MATCH ].Num();
			int returnMatchNum = objectiveCompletionTimes[ GP_RETURN_MATCH ].Num();
			if ( firstMatchNum > returnMatchNum ) {
				SetWinner( firstRoundAttackTeam );
				winReason = WR_NORMAL;
				return;
			}
			if ( returnMatchNum > firstMatchNum ) {
				SetWinner( returnRoundAttackTeam );
				winReason = WR_NORMAL;
				return;
			}

			// it'll fall through to here if both teams did the same number of objectives
			for ( int i = objectiveCompletionTimes[ GP_FIRST_MATCH ].Num() - 1; i >= 0; i-- ) {
				// find out who finished the objective first
				int firstTeamTime = objectiveCompletionTimes[ GP_FIRST_MATCH ][ i ];
				int returnTeamTime = objectiveCompletionTimes[ GP_RETURN_MATCH ][ i ];

				if ( firstTeamTime < returnTeamTime ) {
					// first team did it faster
					SetWinner( firstRoundAttackTeam );
					winReason = WR_SPEED;
					return;
				}
				if ( returnTeamTime < firstTeamTime ) {
					// second team did it faster
					SetWinner( returnRoundAttackTeam );
					winReason = WR_SPEED;
					return;
				}
			}

			// it'll only get here if no-one did any objectives or if both teams completed the objectives
			// at the exact same times! (crazy unlikely, but hey...)
			// the team with the most attacking XP wins
			//
			// NOTE: after all this if by some crazy fluke the XP is identical (or if no-one did anything)
			//       it is counted as the first team winning
			if ( attackingTeamXPs[ GP_FIRST_MATCH ] >= attackingTeamXPs[ GP_RETURN_MATCH ] ) {			
				SetWinner( firstRoundAttackTeam );
				winReason = WR_XP;
				return;
			} else {
				SetWinner( returnRoundAttackTeam );
				winReason = WR_XP;
				return;
			}
		}
	}
}

/*
============
sdGameRulesStopWatch::GetWinningReason
============
*/
const sdDeclLocStr* sdGameRulesStopWatch::GetWinningReason( void ) const {
	if ( winReason == WR_NORMAL ) {
		return NULL;
	}

	sdTeamInfo* winner = GetWinningTeam();
	if ( winner == NULL ) {
		return NULL;
	}

	if ( winReason == WR_SPEED ) {
		return winner->GetWinStringSW_Speed();
	}

	if ( winReason == WR_XP ) {
		return winner->GetWinStringSW_XP();
	}

	return NULL;
}

/*
============
sdGameRulesStopWatch::GetDemoNameInfo
============
*/
const char* sdGameRulesStopWatch::GetDemoNameInfo( void ) {
	static idStr demoNameBuffer;
	if ( progression == GP_FIRST_MATCH ) {
		demoNameBuffer = va( "stopwatch_first" );
	} else {
		demoNameBuffer = va( "stopwatch_second" );
	}
	return demoNameBuffer.c_str();
}
