// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "GameRules.h"
#include "GameRules_Campaign.h"
#include "../structures/TeamManager.h"
#include "../Player.h"
#include "../roles/FireTeams.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"

#include "../guis/UserInterfaceLocal.h"
#include "../guis/UserInterfaceTypes.h"
#include "../guis/UIWindow.h"
#include "../guis/UIList.h"
#include "../guis/UserInterfaceManager.h"

#include "../proficiency/StatsTracker.h"

#include "../botai/BotThreadData.h"
#include "../botai/Bot.h"	//mal: needed for the bots.

#include "../Waypoints/LocationMarker.h"
#include "../demos/DemoManager.h"

#include "VoteManager.h"

#include "AdminSystem.h"

idCVar sdGameRules::g_chatDefaultColor( "g_chatDefaultColor", "1 1 1 1", CVAR_GAME | CVAR_NOCHEAT | CVAR_PROFILE, "RGBA value for normal chat prints" );
idCVar sdGameRules::g_chatTeamColor( "g_chatTeamColor", "1 1 0 1", CVAR_GAME | CVAR_NOCHEAT | CVAR_PROFILE, "RGBA value for team chat prints" );
idCVar sdGameRules::g_chatFireTeamColor( "g_chatFireTeamColor", "0.8 0.8 0.8 1", CVAR_GAME | CVAR_NOCHEAT | CVAR_PROFILE, "RGBA value for fire team chat prints" );
idCVar sdGameRules::g_chatLineTimeout( "g_chatLineTimeout", "5", CVAR_GAME | CVAR_FLOAT | CVAR_NOCHEAT | CVAR_PROFILE, "number of seconds that each chat line stays in the history" );


/*
================
sdGameRulesNetworkState::MakeDefault
================
*/
void sdGameRulesNetworkState::MakeDefault( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		pings[ i ] = 0;
		teams[ i ] = NULL;
		userGroups[ i ] = -1;
	}

	state				= 0;
	matchStartedTime	= 0;
	nextStateSwitch		= 0;
}

/*
================
sdGameRulesNetworkState::Write
================
*/
void sdGameRulesNetworkState::Write( idFile* file ) const {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		file->WriteInt( pings[ i ] );
		file->WriteInt( teams[ i ] ? teams[ i ]->GetIndex() : -1 );
		file->WriteInt( userGroups[ i ] );
	}

	file->WriteInt( state );
	file->WriteInt( matchStartedTime );
	file->WriteInt( nextStateSwitch );
}


/*
================
sdGameRulesNetworkState::Read
================
*/
void sdGameRulesNetworkState::Read( idFile* file ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		file->ReadInt( pings[ i ] );

		int teamIndex;
		file->ReadInt( teamIndex );
		teams[ i ] = teamIndex == -1 ? NULL : &sdTeamManager::GetInstance().GetTeamByIndex( teamIndex );

		file->ReadInt( userGroups[ i ] );
	}

	file->ReadInt( state );
	file->ReadInt( matchStartedTime );
	file->ReadInt( nextStateSwitch );
}

/*
===============================================================================

	sdGameRules

===============================================================================
*/

extern const idEventDef EV_SendNetworkEvent;

const idEventDef EV_Rules_SetEndGameCamera( "setEndGameCamera", '\0', DOC_TEXT( "Sets the camera view to be used at the end of the game, before the stats screen is shown." ), 1, NULL, "E", "camera", "Entity to be used as a camera." );
const idEventDef EV_Rules_EndGame( "endGame", '\0', DOC_TEXT( "Finishes the current game in progress, and marks the current winning team as the winners." ), 0, "Use $event:setWinningTeam$ to set the current winning team." );
const idEventDef EV_Rules_SetWinningTeam( "setWinningTeam", '\0', DOC_TEXT( "Sets the current winning team." ), 1, "If the object passed in is not a valid team, it will be treated as $null$.", "o", "team", "Team to set." );
const idEventDef EV_Rules_GetKeySuffix( "getKeySuffix", 's', DOC_TEXT( "Gets the suffix for rules-dependant keys." ), 0, "Append to key names to get versions of keys for different game rulesets." );

ABSTRACT_DECLARATION( idClass, sdGameRules )
	EVENT( EV_SendNetworkEvent,						sdGameRules::Event_SendNetworkEvent )
	EVENT( EV_Rules_SetEndGameCamera,				sdGameRules::Event_SetEndGameCamera )
	EVENT( EV_Rules_EndGame,						sdGameRules::Event_EndGame )
	EVENT( EV_Rules_SetWinningTeam,					sdGameRules::Event_SetWinningTeam )
	EVENT( EV_Rules_GetKeySuffix,					sdGameRules::Event_GetKeySuffix )
END_CLASS

/*
================
sdGameRules::sdGameRules
================
*/
sdGameRules::sdGameRules( void ) {
	scriptObject = NULL;
	commandsAdded = false;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		playerState[ i ].fireTeam		= NULL;
		playerState[ i ].userGroup		= -1;
		playerState[ i ].muteStatus		= 0;
		playerState[ i ].numWarnings	= 0;
		playerState[ i ].backupTeam		= NULL;
		playerState[ i ].backupFireTeamState.fireTeamIndex	= -1;
		playerState[ i ].backupFireTeamState.fireTeamLeader = false;
		playerState[ i ].backupFireTeamState.fireTeamPublic = true;
	}

	Clear();
	noMission = declHolder.declLocStrType.LocalFind( "guis/game/scoreboard/nomission" );
	infinity = declHolder.declLocStrType.LocalFind( "guis/mainmenu/infinity" );
}

/*
================
sdGameRules::~sdGameRules
================
*/
sdGameRules::~sdGameRules( void ) {
	callVotes.DeleteContents( true );

	if( commandsAdded ) {
		RemoveConsoleCommands();

		uiManager->UnregisterListEnumerationCallback( "chatHistory" );
		commandsAdded = false;
	}
}

/*
================
sdGameRules::SavePlayerStates
================
*/
void sdGameRules::SavePlayerStates( playerStateList_t& states ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		states[ i ].backupClass	= playerState[ i ].backupClass;
		states[ i ].backupTeam	= playerState[ i ].backupTeam;
		states[ i ].backupFireTeamState.fireTeamIndex		= playerState[ i ].backupFireTeamState.fireTeamIndex;
		states[ i ].backupFireTeamState.fireTeamLeader		= playerState[ i ].backupFireTeamState.fireTeamLeader;
		states[ i ].backupFireTeamState.fireTeamPublic		= playerState[ i ].backupFireTeamState.fireTeamPublic;
		states[ i ].backupFireTeamState.fireTeamName		= playerState[ i ].backupFireTeamState.fireTeamName;
		states[ i ].fireTeam	= playerState[ i ].fireTeam;
		states[ i ].muteStatus	= playerState[ i ].muteStatus;
		states[ i ].numWarnings	= playerState[ i ].numWarnings;
		states[ i ].ping		= playerState[ i ].ping;
		states[ i ].userGroup	= playerState[ i ].userGroup;
	}
}

/*
================
sdGameRules::RestorePlayerStates
================
*/
void sdGameRules::RestorePlayerStates( const playerStateList_t& states ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		playerState[ i ].backupClass	= states[ i ].backupClass;
		playerState[ i ].backupTeam		= states[ i ].backupTeam;
		playerState[ i ].backupFireTeamState.fireTeamIndex		= states[ i ].backupFireTeamState.fireTeamIndex;
		playerState[ i ].backupFireTeamState.fireTeamLeader		= states[ i ].backupFireTeamState.fireTeamLeader;
		playerState[ i ].backupFireTeamState.fireTeamPublic		= states[ i ].backupFireTeamState.fireTeamPublic;
		playerState[ i ].backupFireTeamState.fireTeamName		= states[ i ].backupFireTeamState.fireTeamName;
		playerState[ i ].fireTeam		= states[ i ].fireTeam;
		playerState[ i ].muteStatus		= states[ i ].muteStatus;
		playerState[ i ].numWarnings	= states[ i ].numWarnings;
		playerState[ i ].ping			= states[ i ].ping;
		playerState[ i ].userGroup		= states[ i ].userGroup;
	}
}


/*
================
sdGameRules::InitConsoleCommands
================
*/
void sdGameRules::InitConsoleCommands( void ) {
	if( !commandsAdded ) {
		cmdSystem->AddCommand( "clientMessageMode",		sdGameRules::MessageMode_f, CMD_FL_GAME, "ingame gui message mode" );
		cmdSystem->AddCommand( "clientQuickChat",		sdGameRules::QuickChat_f,	CMD_FL_GAME, "plays a quickchat declaration", idArgCompletionGameDecl_f< DECLTYPE_QUICKCHAT > );
		cmdSystem->AddCommand( "clientTeam",			sdGameRules::SetTeam_f,		CMD_FL_GAME, "change your team" );
		cmdSystem->AddCommand( "clientClass",			sdGameRules::SetClass_f,	CMD_FL_GAME, "change your class" );
		cmdSystem->AddCommand( "clientDefaultSpawn",	sdGameRules::DefaultSpawn_f,CMD_FL_GAME, "revert to default spawn" );
		cmdSystem->AddCommand( "clientWantSpawn",		sdGameRules::WantSpawn_f,	CMD_FL_GAME, "tap out" );

		uiManager->RegisterListEnumerationCallback( "chatHistory",			CreateChatList );	// input: show expired, CHAT_MODE_ constant, number of items (-1 for all available)

		InitVotes();
	}

	commandsAdded = true;
}

/*
================
sdGameRules::InitVotes
================
*/
void sdGameRules::InitVotes( void ) {
	callVotes.Alloc() = new sdCallVoteMapReset();
}

/*
================
sdGameRules::RemoveConsoleCommands
================
*/
void sdGameRules::RemoveConsoleCommands( void ) {
	cmdSystem->RemoveCommand( "clientMessageMode" );
	cmdSystem->RemoveCommand( "clientQuickChat" );
	cmdSystem->RemoveCommand( "clientTeam" );
	cmdSystem->RemoveCommand( "clientClass" );
	cmdSystem->RemoveCommand( "clientDefaultSpawn" );
	cmdSystem->RemoveCommand( "clientWantSpawn" );
}

/*
================
sdGameRules::MessageMode_f
================
*/
void sdGameRules::MessageMode_f( const idCmdArgs &args ) {
	gameLocal.rules->MessageMode( args );
}

/*
============
sdGameRules::WantSpawn_f
============
*/
void sdGameRules::WantSpawn_f( const idCmdArgs &args ) {
	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_RESPAWN );
		msg.Send();
	} else {
		idPlayer* player = gameLocal.GetLocalPlayer();
		if ( player ) {
			if ( !player->IsSpectator() && player->IsDead() ) {
				player->ServerForceRespawn( false );
			}
		}
	}
}

/*
============
sdGameRules::DefaultSpawn_f
============
*/
void sdGameRules::DefaultSpawn_f( const idCmdArgs &args ) {
	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_DEFAULTSPAWN );
		msg.Send();
	} else {
		idPlayer* player = gameLocal.GetLocalPlayer();
		if ( player ) {
			player->SetSpawnPoint( NULL );
		}
	}
}

/*
============
sdGameRules::SetClass_f
============
*/
void sdGameRules::SetClass_f( const idCmdArgs &args ) {
	const char* className = args.Argv( 1 );
	const sdDeclPlayerClass* pc = gameLocal.declPlayerClassType[ className ];
	if ( !pc ) {
		gameLocal.Warning( "sdGameRules::SetClass_f Invalid Class '%s'", className );
		return;
	}

	int classOption = 0;
	if ( args.Argc() > 2 ) {
		const char* classOptionStr = args.Argv( 2 );
		classOption = atoi( classOptionStr );
	}

	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_CLASSSWITCH );
		msg.WriteLong( pc->Index() );
		msg.WriteLong( classOption );
		msg.Send();
	} else {
		idPlayer* player = gameLocal.GetLocalPlayer();
		if ( player != NULL ) {
			player->ChangeClass( pc, classOption );
		}
	}
}

/*
============
sdGameRules::MuteStatus
============
*/
int sdGameRules::MuteStatus( idPlayer* player ) const {
	return playerState[ player->entityNumber ].muteStatus;
}

/*
============
sdGameRules::Mute
============
*/
void sdGameRules::Mute( idPlayer* player, int flags ) {
	playerState[ player->entityNumber ].muteStatus |= flags;
}

/*
============
sdGameRules::UnMute
============
*/
void sdGameRules::UnMute( idPlayer* player, int flags ) {
	playerState[ player->entityNumber ].muteStatus &= ~flags;
}

/*
============
sdGameRules::Warn
============
*/
void sdGameRules::Warn( idPlayer* player ) {
	playerState[ player->entityNumber ].numWarnings++;
	int maxWarnings = g_maxPlayerWarnings.GetInteger();
	if ( maxWarnings > 0 ) {
		if ( playerState[ player->entityNumber ].numWarnings >= maxWarnings ) {
			networkSystem->ServerKickClient( player->entityNumber, "engine/disconnect/toomanywarnings", true );
			return;
		}

		int left = maxWarnings - playerState[ player->entityNumber ].numWarnings;

		idWStrList parms;
		parms.Append( va( L"%d", left ) );
		player->SendLocalisedMessage( declHolder.declLocStrType[ "rules/messages/warning/xleft" ], parms );
		return;
	}

	player->SendLocalisedMessage( declHolder.declLocStrType[ "rules/messages/warning" ], idWStrList() );
}

/*
============
sdGameRules::TeamIndexForName
============
*/
int sdGameRules::TeamIndexForName( const char* teamname ) {
	int index = -1;

	if ( !idStr::Icmp( teamname, "s" ) ||
		 !idStr::Icmp( teamname, "spec" ) ||
		 !idStr::Icmp( teamname, "spectator" ) ) {
			index = 0;
	} else {
		sdTeamManagerLocal& manager = sdTeamManager::GetInstance();
		for ( int i = 0; i < manager.GetNumTeams(); i++ ) {
			if ( !idStr::Icmp( teamname, manager.GetTeamByIndex( i ).GetLookupName() ) ) {
				index = i + 1;
			}
		}
	}

	return index;
}

/*
============
sdGameRules::SetTeam_f
============
*/
void sdGameRules::SetTeam_f( const idCmdArgs &args ) {
	if ( args.Argc() < 1 ) {
		return;
	}

	const char* teamName = args.Argv( 1 );

	int index;

	if ( !idStr::Icmp( teamName, "auto" ) ) {
		// set to the end of the array
		index = sdTeamManager::GetInstance().GetNumTeams() + 1;
	} else {
		index = TeamIndexForName( teamName );
	}

	if ( index == -1 ) {
		gameLocal.Warning( "Invalid Team '%s'", teamName );
		return;
	}

	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_TEAMSWITCH );
		msg.WriteLong( index );
		if ( index == 0 ) {
			msg.WriteString( "" );
		} else if ( index == sdTeamManager::GetInstance().GetNumTeams() + 1 ) {
			sdTeamInfo& team1 = sdTeamManager::GetInstance().GetTeamByIndex( 0 );
			idCVar* passwordCVar = team1.GetPasswordCVar();
			msg.WriteString( passwordCVar ? passwordCVar->GetString() : "" );
			sdTeamInfo& team2 = sdTeamManager::GetInstance().GetTeamByIndex( 1 );
			passwordCVar = team2.GetPasswordCVar();
			msg.WriteString( passwordCVar ? passwordCVar->GetString() : "" );
		} else {
			sdTeamInfo& teamInfo = sdTeamManager::GetInstance().GetTeamByIndex( index - 1 );
			idCVar* passwordCVar = teamInfo.GetPasswordCVar();
			msg.WriteString( passwordCVar ? passwordCVar->GetString() : "" );
		}
		msg.Send();
	} else {
		idPlayer* player = gameLocal.GetLocalPlayer();
		if ( player ) {
			if ( index == sdTeamManager::GetInstance().GetNumTeams() + 1 ) {
				// auto join team with the least number of players
				int lowest = -1;
				int clientTeamIndex = player->GetTeam() != NULL ? player->GetTeam()->GetIndex() : -1;
				for ( int i = 0; i < sdTeamManager::GetInstance().GetNumTeams(); i++ ) {
					int num = sdTeamManager::GetInstance().GetTeamByIndex( i ).GetNumPlayers();
					if ( clientTeamIndex == i ) {
						num--;
					}
					if ( num < lowest || lowest == -1 ) {
						lowest = num;
						index = i + 1;
					}
				}
			}

			gameLocal.rules->SetClientTeam( player, index, false, "" );
		}
	}
}

/*
============
sdGameRules::SetClientTeam
============
*/
void sdGameRules::SetClientTeam( idPlayer* player, int index, bool force, const char* password ) {
	idPlayer* botPlayer = NULL;

	if ( index == 0 ) {
		if ( si_spectators.GetBool() ) {
			player->SetWantSpectate( true );
		}
	} else {
		index--;
		sdTeamManagerLocal& manager = sdTeamManager::GetInstance();
		int numTeams = manager.GetNumTeams();
		if ( index >= 0 && index < numTeams ) {
			sdTeamInfo* team = &manager.GetTeamByIndex( index );
			sdTeamInfo* currentTeam = player->GetTeam();
			if ( team == currentTeam ) {
				return;
			}

			int currentTeamIndex = currentTeam == NULL ? -1 : currentTeam->GetIndex();

			if ( !force	) {
				const sdDeclLocStr* reason;
				if ( !team->CanJoin( player, password, reason ) ) {
					player->SendLocalisedMessage( reason, idWStrList() );
					return;
				}

				if ( si_teamForceBalance.GetBool() ) {
					int* teamCounts = ( int* )_alloca( numTeams * sizeof( int ) );

					int low = -1;
					for ( int i = 0; i < numTeams; i++ ) {
						teamCounts[ i ] = manager.GetTeamByIndex( i ).GetNumPlayers();
						if ( i == currentTeamIndex ) {
							teamCounts[ i ]--;
						}
						if ( low == -1 || teamCounts[ i ] < low ) {
							low = teamCounts[ i ];
						}
					}

					if ( teamCounts[ index ] != low ) {
						// Gordon: Try to find a bot for the client to replace
						for ( int i = 0; i < MAX_CLIENTS; i++ ) {
							idPlayer* other = gameLocal.GetClient( i );
							if ( other == NULL ) {
								continue;
							}

							if ( other->GetGameTeam() != team ) {
								continue;
							}

							if ( !other->userInfo.isBot ) {
								continue;
							}

							botPlayer = other;
							break;
						}

						if ( botPlayer == NULL ) {
							player->SendLocalisedMessage( declHolder.declLocStrType[ "teams/messages/toomanyplayers" ], idWStrList() );
							return;
						}
					}
				}

				if ( GetState() == GS_GAMEON ) {
					if ( !si_allowLateJoin.GetBool() ) {
						player->SendLocalisedMessage( declHolder.declLocStrType[ "teams/messages/nojoin" ], idWStrList() );
						return;
					}
				}

				if ( player->GetNextTeamSwitchTime() > gameLocal.time ) {
					player->SendLocalisedMessage( declHolder.declLocStrType[ "teams/messages/switchwait" ], idWStrList() );
					return;
				}
			}

			player->Kill( NULL );
			player->SetGameTeam( team );
			if ( currentTeam != NULL && team != NULL && player->GetInventory().GetClass() != NULL ) {
				const sdDeclPlayerClass* remappedClass = currentTeam->GetEquivalentClass( *player->GetInventory().GetClass(), *team );
				if( remappedClass != NULL  ) {
					player->GetInventory().SetCachedClass( remappedClass );
				} else {
					gameLocal.Warning( "sdGameRules::SetClientTeam: could not find equivalent class for '%s' on team '%s'", player->GetInventory().GetClass()->GetName(), team->GetLookupName() );
				}
			}
			player->SetWantSpectate( false );
			player->ServerForceRespawn( false );

			if ( botPlayer != NULL ) {
				sdTeamInfo* newBotTeam = FindNeedyTeam( botPlayer );

				botPlayer->Kill( NULL );
				botPlayer->SetGameTeam( newBotTeam );
				botPlayer->SetWantSpectate( false );
				botPlayer->ServerForceRespawn( false );
			}
		}
	}
}

/*
============
sdGameRules::QuickChat_f
============
*/
void sdGameRules::QuickChat_f( const idCmdArgs &args ) {
	if ( args.Argc() < 2 || args.Argc() > 3 ) {
		gameLocal.Printf( "Usage: clientQuickChat [quickchatname] [target spawnID]\n" );
		return;
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		return;
	}

	const char* quickChatName = args.Argv( 1 );

	const sdDeclQuickChat* quickChat;
	if ( localPlayer == NULL || localPlayer->GetInventory().GetClass() == NULL  ) {
		quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
	} else {
		quickChatName = localPlayer->GetInventory().GetClass()->BuildQuickChatDeclName( args.Argv( 1 ) );
		quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
	}

	if ( quickChat == NULL ) {
		gameLocal.Warning( "Could not find quickchat: %s (%s)", quickChatName, args.Argv( 1 ) );
		return;
	}

	int spawnId = -1;
	if ( args.Argc() == 3 && idStr::Icmp( args.Argv( 2 ), "invalid" ) != 0 ) {
		spawnId = sdTypeFromString< int >( args.Argv( 2 ) );
	}

	localPlayer->RequestQuickChat( quickChat, spawnId );
}

/*
============
sdGameRules::EnterGame
============
*/
void sdGameRules::EnterGame( idPlayer* player ) {
	player->SetInGame( true );
}


/*
================
sdGameRules::NewMap
================
*/
void sdGameRules::NewMap( bool isUserChange ) {
	if ( isUserChange ) {
		Reset();
	}

	pingUpdateTime = 0;
}

/*
================
sdGameRules::Shutdown
================
*/
void sdGameRules::Shutdown( void ) {
	Clear();
}

/*
================
sdGameRules::Reset
================
*/
void sdGameRules::Reset( void ) {
	Clear();
	sdProficiencyManager::GetInstance().ClearProficiency();
}

/*
================
sdGameRules::BackupPlayerTeams
================
*/
void sdGameRules::BackupPlayerTeams( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}	

		playerState[ i ].backupTeam = player->GetGameTeam();
		playerState[ i ].backupClass = *player->GetInventory().GetClassSetup();
		playerState[ i ].backupFireTeamState.fireTeamIndex = playerState[ i ].fireTeam != NULL ? playerState[ i ].backupTeam->GetFireTeamIndex( playerState[ i ].fireTeam ) : -1;
		playerState[ i ].backupFireTeamState.fireTeamName.Clear();

		if ( playerState[ i ].backupFireTeamState.fireTeamIndex != -1 ) {
			playerState[ i ].backupFireTeamState.fireTeamLeader = playerState[ i ].fireTeam->GetCommander() == player ? true : false;
			playerState[ i ].backupFireTeamState.fireTeamPublic = playerState[ i ].fireTeam->IsPrivate() == false ? true : false;
			playerState[ i ].backupFireTeamState.fireTeamName = playerState[ i ].fireTeam->GetName();
		}
	}
}

/*
================
sdGameRules::RestoreFireTeam
================
*/
void sdGameRules::RestoreFireTeam( idPlayer* player ) {
	if ( player == NULL ) {
		return;
	}

	playerState_t& state = playerState[ player->entityNumber ];

	sdTeamInfo* team = player->GetGameTeam();
	if ( state.backupFireTeamState.fireTeamIndex != -1 && team != NULL ) {
		if ( !gameLocal.isClient ) {
			sdFireTeam& fireTeam = team->GetFireTeam( state.backupFireTeamState.fireTeamIndex );
			fireTeam.AddMember( player->entityNumber );
			if ( state.backupFireTeamState.fireTeamLeader && fireTeam.GetNumMembers() > 1 ) {
				fireTeam.PromoteMember( player->entityNumber );
			}
			if ( state.backupFireTeamState.fireTeamName.Length() > 0 ) {
				fireTeam.SetName( state.backupFireTeamState.fireTeamName.c_str() );
			}
			fireTeam.SetPrivate( !state.backupFireTeamState.fireTeamPublic );
		}
	}

	state.backupFireTeamState.fireTeamName.Clear();
	state.backupFireTeamState.fireTeamLeader = false;
	state.backupFireTeamState.fireTeamIndex = -1;
}

/*
================
sdGameRules::SpawnPlayer
================
*/
void sdGameRules::SpawnPlayer( idPlayer* player ) {
	playerState_t& state	= playerState[ player->entityNumber ];
	state.ping				= 0;

	if ( state.backupTeam != NULL ) {
		player->SetGameTeam( state.backupTeam );

		if ( state.backupClass.GetClass() != NULL ) {
			const idList< int >& options = state.backupClass.GetOptions();
			int classOption = options.Num() > 0 ? options[ 0 ] : 0;

			player->ChangeClass( state.backupClass.GetClass(), classOption );
		}

		RestoreFireTeam( player );

		state.backupTeam = NULL;
	}

	if ( GetState() == GS_GAMEON ) {
		player->SetGameStartTime( gameLocal.time );
	}

	OnConnect( player );
}

/*
================
sdGameRules::Clear
================
*/
void sdGameRules::Clear( void ) {
	gameState			= GS_INACTIVE;
	nextState			= GS_INACTIVE;
	pingUpdateTime		= 0;
	nextStateSwitch		= 0;
	matchStartedTime	= 0;

	pureReady			= false;

	ClearChatData();
}

//idCVar gui_scoreBoardSort( "gui_scoreBoardSort", "0", CVAR_ARCHIVE | CVAR_PROFILE | CVAR_INTEGER, "0 - group by XP, 1 - group by fireteam, then by XP" );

/*
================
sdGameRules::UpdateScoreboard
================
*/
void sdGameRules::UpdateScoreboard( sdUIList* list, const char* teamName ) {
	sdUIList::ClearItems( list );

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();

	sdTeamManagerLocal& manager = sdTeamManager::GetInstance();

	int index = TeamIndexForName( teamName );

	// spectators
	if( index == 0 ) {
		list->GetUI()->PushScriptVar( 0.0f );
		list->GetUI()->PushScriptVar( 0.0f );
		list->GetUI()->PushScriptVar( 0.0f );
		return;
	}

	sdTeamInfo& teamInfo = manager.GetTeamByIndex( index - 1 );

	sdTeamInfo* localTeam = localPlayer == NULL ? NULL : localPlayer->GetTeam();

	bool showClass = localTeam == NULL ? true : *localTeam == teamInfo;

	idStaticList< idPlayer*, MAX_CLIENTS > players;
	teamInfo.SortPlayers( players, true );

	float averageXP = 0.0f;
	float averagePing = 0.0f;
	int numSpectators = 0;

	if( g_debugPlayerList.GetInteger() ) {
		int i;
		idRandom random;
		random.SetSeed( gameLocal.time );

		for( i = 0; i < g_debugPlayerList.GetInteger() - players.Num(); i++ ) {
			int index = sdUIList::InsertItem( list, L"", 0, 0 );

			sdUIList::SetItemIcon( list, "soldier", index, 0 );
			sdUIList::SetItemForeColor( list, colorWhite, index, 0 );

			sdUIList::SetItemIcon( list, "guis/assets/icons/rank01", index, 1 );
			sdUIList::SetItemForeColor( list, colorWhite, index, 1 );

			sdUIList::SetItemText( list, L"", index, 2 );

			sdUIList::SetItemText( list, va( L"Player %i", i ), index, 3 );
			sdUIList::SetItemText( list, va( L"No mission", i ), index, 4 );

			sdUIList::SetItemText( list, va( L"%i", 100 ), index, 5 );
			averageXP += 100;

			int ping = random.RandomInt( 1000 );
			sdUIList::SetItemText( list, va( L"%i", ping ), index, 6 );
			averagePing += ping;

			sdUIList::SetItemText( list, va( L"%i", i ), index, 7 );
		}
	}

	const wchar_t* noMissionText = L"";

	const sdPlayerTask::nodeType_t& objectiveTasks = sdTaskManager::GetInstance().GetObjectiveTasks( &teamInfo );
	sdPlayerTask* objectiveTask = objectiveTasks.Next();

	int fireTeamColorIndices[ 8 ] = { 1, 3, 8, 15, 17, 18, 28, 31 };

	int i;
	int numInGamePlayers = 0;

	idStaticList< int, MAX_CLIENTS > fireTeamColors;
	fireTeamColors.SetNum( MAX_CLIENTS );

	//bool sortByFireTeams = gui_scoreBoardSort.GetInteger() == 1;
	bool sortByFireTeams = false;
	int count = sdFireTeamManager::GetInstance().NumFireTeams();
	for( i = 0; i < count; i++ ) {
		const sdFireTeam* fireTeam = sdFireTeamManager::GetInstance().FireTeamForIndex( i );
		int leaderIndex = 0;

		idVec4 color = idStr::ColorForIndex( i );

		// insert the leader
		int pi;
		for( pi = 0; pi < players.Num(); pi++ ) {
			idPlayer* player			= players[ pi ];
			const playerState_t& ps		= playerState[ player->entityNumber ];

			if ( objectiveTask != NULL ) {
				noMissionText = objectiveTask->GetTitle( player );
			} else {
				noMissionText = noMission->GetText();
			}

			if( gameLocal.rules->GetPlayerFireTeam( player->entityNumber ) == fireTeam && fireTeam->GetCommander() == player ) {
				if( sortByFireTeams ) {
					leaderIndex = InsertScoreboardPlayer( list, player, fireTeam, ps, noMissionText, showClass, averagePing, averageXP, numInGamePlayers, leaderIndex );
					list->SetItemDataInt( fireTeamColorIndices[ i ], leaderIndex, 0, true );	// used for deriving background color
					leaderIndex++;
				} else {
					fireTeamColors[ pi ] = fireTeamColorIndices[ i ];
				}
				break;
			}
		}

		for( pi = 0; pi < players.Num(); pi++ ) {
			idPlayer* player			= players[ pi ];
			const playerState_t& ps		= playerState[ player->entityNumber ];

			if ( objectiveTask != NULL ) {
				noMissionText = objectiveTask->GetTitle( player );
			} else {
				noMissionText = noMission->GetText();
			}

			if( gameLocal.rules->GetPlayerFireTeam( player->entityNumber ) == fireTeam && fireTeam->GetCommander() != player ) {
				if( sortByFireTeams ) {
					leaderIndex = InsertScoreboardPlayer( list, player, fireTeam, ps, noMissionText, showClass, averagePing, averageXP, numInGamePlayers, leaderIndex );
					list->SetItemDataInt( fireTeamColorIndices[ i ], leaderIndex, 0, true );	// used for deriving background color
				} else {
					fireTeamColors[ pi ] = fireTeamColorIndices[ i ];
				}
			}
		}
	}

	for ( i = 0; i < players.Num(); i++ ) {
		idPlayer* player			= players[ i ];
		const playerState_t& ps		= playerState[ player->entityNumber ];

		sdFireTeam* ft = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
		if( sortByFireTeams && ft != NULL ) {
			continue;
		}

		if ( objectiveTask != NULL ) {
			noMissionText = objectiveTask->GetTitle( player );
		} else {
			noMissionText = noMission->GetText();
		}

		int index = InsertScoreboardPlayer( list, player, ft, ps, noMissionText, showClass, averagePing, averageXP, numInGamePlayers, -1 );
		//if( !sortByFireTeams ) {
		//	list->SetItemDataInt( fireTeamColors[ i ], index, 0, true );	// used for deriving background color
		//}
	}

	if( numInGamePlayers > 0 ) {
		averagePing /= numInGamePlayers;
		averageXP /= numInGamePlayers;
	}

	list->GetUI()->PushScriptVar( averageXP );
	list->GetUI()->PushScriptVar( averagePing );
	list->GetUI()->PushScriptVar( numInGamePlayers );
}

/*
============
sdGameRules::InsertScoreboardPlayer
============
*/
int sdGameRules::InsertScoreboardPlayer( sdUIList* list, const idPlayer* player, const sdFireTeam* fireTeam,
										const playerState_t& ps,
										const wchar_t* noMissionText,
										bool showClass,
										float& averagePing,
										float& averageXP,
										int& numInGamePlayers,
										int index ) {

	static idVec4 noDraw( 0, 0, 0, 0 );
	index = sdUIList::InsertItem( list, L"", index, 0 );
	bool spectating = player->IsSpectator();

	// class
	if( showClass ) {
		const sdDeclPlayerClass* pc = player->GetInventory().GetClass() ? player->GetInventory().GetClass() : player->GetInventory().GetCachedClass();
		if( pc != NULL ) {
			sdUIList::SetItemIcon( list, pc->GetName(), index, 0 );
			sdUIList::SetItemForeColor( list, colorWhite, index, 0 );
		}

	}

	// rank/voip
	if ( player->IsSendingVoice() ) {
		sdUIList::SetItemIcon( list, "voip", index, 1 );
		sdUIList::SetItemForeColor( list, colorWhite, index, 1 );
	} else if ( spectating ) {
		sdUIList::SetItemIcon( list, "spectating", index, 1 );
		sdUIList::SetItemForeColor( list, colorWhite, index, 1 );
	} else if ( const sdDeclRank* rank = player->GetProficiencyTable().GetRank() ) {

		bool isBot = player->IsType( idBot::Type );
		bool shouldCheckReady = !isBot && ( ( gameLocal.rules->IsEndGame() && gameLocal.serverInfoData.gameReviewReadyWait ) || ( gameLocal.rules->IsWarmup() && !gameLocal.serverInfoData.adminStart && !gameLocal.rules->IsCountDown() ) );

		if( shouldCheckReady && !player->IsReady() ) {
			sdUIList::SetItemIcon( list, "notready", index, 1 );
			sdUIList::SetItemForeColor( list, colorWhite, index, 1 );
		} else {
			if( player->IsCarryingObjective() ) {
				sdUIList::SetItemIcon( list, "documents", index, 1 );
				sdUIList::SetItemForeColor( list, colorWhite, index, 1 );
			} else {
				if( isBot ) {
					sdUIList::SetItemIcon( list, "bot", index, 1 );
					sdUIList::SetItemForeColor( list, colorWhite, index, 1 );

				} else {
					sdUIList::SetItemIcon( list, rank->GetMaterial(), index, 1 );
					sdUIList::SetItemForeColor( list, colorWhite, index, 1 );
				}
			}
		}
	} else {
		bool isBot = player->IsType( idBot::Type );
		if( isBot ) {
			sdUIList::SetItemIcon( list, "bot", index, 1 );
			sdUIList::SetItemForeColor( list, colorWhite, index, 1 );
		} else {
			sdUIList::SetItemForeColor( list, noDraw, index, 1 );
		}
	}

	// fireteam
	if( fireTeam != NULL ) {
		sdUIList::SetItemText( list, va( L"%hs", fireTeam->GetName() ), index, 2 );
	}

	// player name
	idWStr cleanPlayerName = va( L"%hs", player->GetUserInfo().name.c_str() );
	sdUIList::CleanUserInput( cleanPlayerName );

	sdUIList::SetItemText( list, cleanPlayerName.c_str(), index, 3 );

	// current task
	if( !spectating && ( fireTeam == NULL || fireTeam->GetCommander() == player ) ) {
		sdPlayerTask* task = NULL;
		if( sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber ) ) {
			if( idPlayer* commander = fireTeam->GetCommander() ) {
				task = commander->GetActiveTask();
			}
		}  else {
			task = player->GetActiveTask();
		}

		if( task != NULL  ) {
			sdUIList::SetItemText( list, task->GetTitle(), index, 4 );
		} else {
			sdUIList::SetItemText( list, noMissionText, index, 4 );
		}
	}

	if( !spectating ) {
		sdUIList::SetItemText( list, va( L"%i", idMath::Ftoi( player->GetProficiencyTable().GetXP() ) ), index, 5 );
	}
	sdUIList::SetItemText( list, va( L"%i", ps.ping ), index, 6 );

	// entity number
	list->SetItemDataInt( player->entityNumber, index, 7 );

	if( !spectating ) {
		numInGamePlayers++;
		averagePing += ps.ping;
		averageXP += player->GetProficiencyTable().GetXP();
	}
	if( player == gameLocal.GetLocalPlayer() ) {
		list->SelectItem( index );
	}

	return index;
}

/*
================
sdGameRules::NumActualClients
================
*/
int sdGameRules::NumActualClients( bool countSpectators, bool countBots ) {
	int c = 0;
	for( int i = 0 ; i < gameLocal.numClients ; i++ ) {
		idPlayer* p = gameLocal.GetClient( i );
		if ( !p ) {
			continue;
		}

		if ( !countBots && p->userInfo.isBot ) {
			continue;
		}

		if ( countSpectators || p->CanPlay() ) {
			c++;
		}
	}
	return c;
}

/*
================
sdGameRules::NewState
================
*/
void sdGameRules::NewState( gameState_t news ) {

	assert( news != gameState );
	switch( news ) {
		case GS_GAMEON: {
			sdGlobalStatsTracker::GetInstance().Clear();
			gameLocal.StartAutorecording();

			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				// Gordon: For local play, grab the latest local stats for life stats baseline
				if ( gameLocal.isServer && gameLocal.GetLocalPlayer() != NULL ) {
					gameLocal.clientRanks[ i ].calculated = false;
					gameLocal.clientStatsRequestsPending = true;
				}

				idPlayer* player = gameLocal.GetClient( i );
				if ( player == NULL ) {
					continue;
				}

				player->SetGameStartTime( gameLocal.time );
			}

			OnGameState_GameOn();
			break;
		}
		case GS_GAMEREVIEW: {
			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				idPlayer* player = gameLocal.GetClient( i );
				if ( player == NULL ) {
					continue;
				}

				if ( player->GetHealth() > 0 ) {
					player->RegisterTimeAlive();
				}
				player->CalcLifeStats();
			}

			if ( gameLocal.GetLocalPlayer() != NULL ) {
				sdGlobalStatsTracker::GetInstance().StartStatsRequest();
			}

			if ( gameState == GS_GAMEON ) {
				// we've just finished the match

				for ( int i = 0; i < MAX_CLIENTS; i++ ) {
					idPlayer* player = gameLocal.GetClient( i );
					if ( player == NULL ) {
						continue;
					}

					player->LogTimePlayed();
					player->WriteStats();
				}

#if !defined( SD_DEMO_BUILD )
				gameLocal.GetSDNet().FlushStats( false );
#endif /* !SD_DEMO_BUILD */
				sdProficiencyManager::GetInstance().DumpProficiencyData();
			}

			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				idPlayer* player = gameLocal.GetClient( i );
				if ( player == NULL ) {
					continue;
				}

				idEntity* proxy = player->GetProxyEntity();
				if ( proxy != NULL ) {
					proxy->GetUsableInterface()->OnExit( player, true );
				}

				player->SetRemoteCamera( NULL, false );
			}

			OnGameState_Review();
			break;
		}
		case GS_NEXTMAP: {
			OnGameState_NextMap();
			break;
		}
		case GS_WARMUP: {
			gameLocal.ClearEndGameStats();

			OnGameState_Warmup();
			if( gameLocal.DoClientSideStuff() ) {
				SetWarmupStatusMessage();
			}
			break;
		}
		case GS_COUNTDOWN: {
			gameLocal.ClearEndGameStats();
			gameLocal.StartAutorecording();

			OnGameState_Countdown();
			if( gameLocal.DoClientSideStuff() ) {
				statusText = common->LocalizeText( "game/warmup" );
			}
			break;
		}
	}

	lastStateSwitchTime = gameLocal.time;
	gameState = news;
	if( gameLocal.DoClientSideStuff() ) {
		UpdateClientFromServerInfo( gameLocal.serverInfo, false );
	}

	sdObjectiveManager::GetInstance().OnGameStateChange( gameState );
}

/*
================
sdGameRules::OnLocalMapRestart
================
*/
void sdGameRules::OnLocalMapRestart( void ) {
	if ( gameState == GS_GAMEREVIEW || gameState == GS_GAMEON || gameState == GS_NEXTMAP ) {
		// only stop autorecording if the restart is for the end of the round
		gameLocal.StopAutorecording();
	}
}

/*
================
sdGameRules::NextStateDelayed
================
*/
void sdGameRules::NextStateDelayed( gameState_t state, int delay ) {
	nextState		= state;
	nextStateSwitch = gameLocal.time + delay;
}


/*
============
sdGameRules::SetWarmupStatusMessage
============
*/
void sdGameRules::SetWarmupStatusMessage() {
	if( gameLocal.serverInfoData.adminStart ) {
		statusText = common->LocalizeText( "game/waitingforadmin" );
	} else {
		int numPlayers;
		readyState_e ready = ArePlayersReady( false, true, &numPlayers );

		if ( ready == RS_NOT_ENOUGH_CLIENTS ) {
			statusText = common->LocalizeText( "game/waitingforplayers" );
		} else if( ready == RS_NOT_ENOUGH_READY ) {
			statusText = common->LocalizeText( "game/waitingforreadyplayers" );
		} else {
			statusText = common->LocalizeText( "game/warmup" );
		}
	}
}

/*
================
sdGameRules::Run
================
*/
void sdGameRules::Run( void ) {
	UpdateChatLines();
	pureReady = true;

	if( gameLocal.isClient ) {
		return;
	}

	if ( gameState == GS_INACTIVE ) {
		NewState( GS_WARMUP );
	}

	if ( gameLocal.time > gameLocal.playerSpawnTime && !gameLocal.IsPaused() ) {
		CheckRespawns();
	}

	if ( nextState != GS_INACTIVE && gameLocal.time > nextStateSwitch ) {
		NewState( nextState );
		nextState = GS_INACTIVE;
	}

	// don't update the ping every frame to save bandwidth
	if ( gameLocal.time > pingUpdateTime ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = gameLocal.GetClient( i );
			if ( player == NULL ) {
				playerState[ i ].ping = 0;
			} else if ( player->GetIsLagged() ) {
				playerState[ i ].ping = 999;
			} else {
				playerState[ i ].ping = networkSystem->ServerGetClientPing( i );
			}
		}

		pingUpdateTime = gameLocal.time + 1000;
	}

	switch( gameState ) {
		case GS_GAMEREVIEW: {
			GameState_Review();
			break;
		}
		case GS_NEXTGAME: {
			GameState_NextGame();
			break;
		}
		case GS_WARMUP: {
			GameState_Warmup();
			break;
		}
		case GS_COUNTDOWN: {
			GameState_Countdown();
			break;
		}
		case GS_GAMEON: {
			GameState_GameOn();
			break;
		}
		case GS_NEXTMAP: {
			GameState_NextMap();
			break;
		}
	}
}

/*
===============
sdGameRules::ClearChatData
===============
*/
void sdGameRules::ClearChatData() {
	chatLines.SetNum( MAX_CHAT_LINES );
	for( int i = 0; i < chatLines.Num(); i++ ) {
		chatLines[ i ].GetNode().AddToEnd( chatFree );
	}
}

/*
===============
sdGameRules::L.
===============
*/
void sdGameRules::AddChatLine( chatMode_t chatMode, const idVec4& color, const wchar_t *fmt, ... ) {
	idWStr temp;
	va_list argptr;

	va_start( argptr, fmt );
	vswprintf( temp, fmt, argptr );
	va_end( argptr );

	gameLocal.Printf( "%ls\n", temp.c_str() );

	if ( gameLocal.DoClientSideStuff() ) {
		sdChatLine::node_t* node = chatFree.NextNode();
		sdChatLine* line = NULL;
		if( node != NULL ) {
			line = node->Owner();
		} else {
			int oldestIndex = 0;
			for( int i = 1; i < chatLines.Num(); i++ ) {
				if( chatLines[ i ].GetTime() < chatLines[ oldestIndex ].GetTime() ) {
					oldestIndex = i;
				}
			}
			assert( oldestIndex >= 0 && oldestIndex < chatLines.Num() );
			line = &chatLines[ oldestIndex ];
		}
		if( line != NULL ) {
			line->GetNode().AddToEnd( chatHead );
			line->Set( temp.c_str(), chatMode );
		}
	}
}

idCVar g_showChatLocation( "g_showChatLocation", "1", CVAR_BOOL | CVAR_GAME, "show/hide locations in chat text" );

/*
===============
sdGameRules::AddRepeaterChatLine
===============
*/
void sdGameRules::AddRepeaterChatLine( const char* clientName, const int clientNum, const wchar_t *text ) {
	idVec4 color;
	sdProperties::sdFromString( color, g_chatDefaultColor.GetString() );
	AddChatLine( CHAT_MODE_MESSAGE, color, L"%hs^0(%d): %ls", clientName, clientNum, text );
}

/*
===============
sdGameRules::AddChatLine
===============
*/
void sdGameRules::AddChatLine( const idVec3& origin, chatMode_t chatMode, const int clientNum, const wchar_t *text ) {
	const wchar_t* wideName = NULL;
	const char* name = NULL;

	if ( clientNum < 0 ) {
		name = "Server";

		if ( clientNum == -1 ) {
			// Gordon: make this "high command" for your team
			idPlayer* player = gameLocal.GetLocalPlayer();
			if ( player != NULL ) {
				sdTeamInfo* team = player->GetGameTeam();
				if ( team != NULL ) {
					name = NULL;

					wideName = team->GetHighCommandName();
				}
			}
		}
	} else {
		idPlayer* player = gameLocal.GetClient( clientNum );
		if ( player ) {
			name = player->userInfo.name.c_str();
			player->FlashPlayerIcon();
		} else {
			name = "player";
		}
	}

	idVec4 color;
	if ( chatMode == CHAT_MODE_SAY_TEAM || chatMode == CHAT_MODE_QUICK_TEAM ) {
		sdProperties::sdFromString( color, g_chatTeamColor.GetString() );
	} else if ( chatMode == CHAT_MODE_SAY_FIRETEAM || chatMode == CHAT_MODE_QUICK_FIRETEAM ) {
		sdProperties::sdFromString( color, g_chatFireTeamColor.GetString() );
	} else {
		sdProperties::sdFromString( color, g_chatDefaultColor.GetString() );
	}

	if ( chatMode == CHAT_MODE_MESSAGE || chatMode == CHAT_MODE_OBITUARY ) {
		AddChatLine( chatMode, color, L"%ls", text );
	} else {
		const wchar_t* locationString = L"";
		if ( g_showChatLocation.GetBool() && clientNum >= 0 ) {
			idWStr locationName;
			switch ( chatMode ) {
				case CHAT_MODE_SAY_TEAM:
				case CHAT_MODE_SAY_FIRETEAM:
				case CHAT_MODE_QUICK_TEAM:
				case CHAT_MODE_QUICK_FIRETEAM:
					sdLocationMarker::GetLocationText( origin, locationName );
					locationString = va( L", ^L%ls^0", locationName.c_str() );
					break;
			}
		}

		if ( name != NULL ) {
			AddChatLine( chatMode, color, L"%hs%ls: %ls", name, locationString, text );
		} else {
			assert( wideName != NULL );
			AddChatLine( chatMode, color, L"%ls%ls: %ls", wideName, locationString, text );
		}
	}
}

const int ASYNC_PLAYER_PING_BITS = idMath::BitsForInteger( 999 );

/*
================
sdGameRules::FindNeedyTeam
================
*/
sdTeamInfo* sdGameRules::FindNeedyTeam( idPlayer* ignore ) {
	sdTeamManagerLocal& manager = sdTeamManager::GetInstance();

	sdTeamInfo* bestTeam = NULL;
	int bestCount = -1;
	int i, j;

	for ( i = 0; i < manager.GetNumTeams(); i++ ) {
		sdTeamInfo& team = manager.GetTeamByIndex( i );
		int players = 0;

		for ( j = 0; j < gameLocal.numClients; j++ ) {
			idPlayer* player = gameLocal.GetClient( j );
			if ( player == NULL || player == ignore ) {
				continue;
			}

			if ( player->GetGameTeam() == &team ) {
				players++;
			}
		}

		if ( !bestTeam || players < bestCount ) {
			bestTeam	= &team;
			bestCount	= players;
		}
	}

	return bestTeam;
}

/*
================
sdGameRules::CheckRespawns
================
*/
void sdGameRules::CheckRespawns( bool force ) {
	for ( int i = 0 ; i < gameLocal.numClients ; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		if ( player->GetWantSpectate() ) {
			player->ServerSpectate();
			if ( player->IsInLimbo() ) {
				player->SpawnFromSpawnSpot();
			}
			continue;
		}

		if ( player->WantRespawn() || force ) {
			if ( gameState == GS_WARMUP || gameState == GS_COUNTDOWN || gameState == GS_GAMEON ) {
				if ( gameState == GS_GAMEON && !force ) {
					sdTeamInfo* team = player->GetTeam();
					if ( team == NULL ) {
						continue;
					}

					if ( !team->AllowRespawn( player ) ) {
						continue;
					}
				}

				player->SpawnFromSpawnSpot();
			}
		}
	}
}

/*
================
sdGameRules::MessageMode
================
*/
void sdGameRules::MessageMode( const idCmdArgs &args ) {

	if ( sdDemoManager::GetInstance().InPlayBack() ) {
		return;
	}

	if ( networkSystem->IsDedicated() ) {
		gameLocal.Printf( "sdGameRules::MessageMode not valid without a local player\n" );
		return;
	}

	int imode;
	const char* mode = args.Argv( 1 );
	if ( !mode[ 0 ] ) {
		imode = 0;
	} else {
		imode = atoi( mode );
	}

	sdQuickChatMenu* quickChatMenu = gameLocal.localPlayerProperties.GetQuickChatMenu();
	if ( quickChatMenu->Enabled() ) {
		quickChatMenu->Enable( false );
	}

	sdWeaponSelectionMenu* weaponMenu = gameLocal.localPlayerProperties.GetWeaponSelectionMenu();
	if( weaponMenu->Enabled() ) {
		weaponMenu->Enable( false );
	}

	sdQuickChatMenu* contextMenu = gameLocal.localPlayerProperties.GetContextMenu();
	if( contextMenu->Enabled() ) {
		contextMenu->Enable( false );
	}

	sdFireTeamMenu* fireTeamMenu = gameLocal.localPlayerProperties.GetFireTeamMenu();
	if ( fireTeamMenu->Enabled() ) {
		fireTeamMenu->Enable( false );
	}

	sdChatMenu* menu = gameLocal.localPlayerProperties.GetChatMenu();
	menu->Enable( true, true );

	if ( sdUserInterfaceLocal* chatUI = menu->GetGui() ) {
		if ( imode == 1 ) {
			chatUI->PostNamedEvent( "teamChat" );
		} else if ( imode == 2 ) {
			chatUI->PostNamedEvent( "fireteamChat" );
		} else {
			chatUI->PostNamedEvent( "globalChat" );
		}
	}
}

/*
================
sdGameRules::DisconnectClient
================
*/
void sdGameRules::DisconnectClient( int clientNum ) {
	gameLocal.GetProficiencyTable( clientNum ).Clear( true );

	if ( playerState[ clientNum ].fireTeam != NULL ) {
		playerState[ clientNum ].fireTeam->RemoveMember( clientNum ); // this should set it to NULL
		assert( playerState[ clientNum ].fireTeam == NULL );
	}
	playerState[ clientNum ].userGroup		= -1;
	playerState[ clientNum ].muteStatus		= 0;
	playerState[ clientNum ].numWarnings	= 0;
	playerState[ clientNum ].backupTeam		= NULL;
	playerState[ clientNum ].backupFireTeamState.fireTeamIndex	= -1;
	playerState[ clientNum ].backupFireTeamState.fireTeamLeader = false;
	playerState[ clientNum ].backupFireTeamState.fireTeamPublic = true;
	playerState[ clientNum ].backupFireTeamState.fireTeamName.Clear();
}

/*
================
sdGameRules::WantKilled
================
*/
void sdGameRules::WantKilled( idPlayer* player ) {
	player->Suicide();
}

/*
================
sdGameRules::MapRestart
================
*/
void sdGameRules::MapRestart( void ) {
	assert( !gameLocal.isClient );
	nextState = GS_INACTIVE;
	if ( gameState != GS_WARMUP ) {
		ClearPlayerReadyFlags();
		NewState( GS_WARMUP );
	}
}


/*
============
sdGameRules::OnPlayerReady
============
*/
void sdGameRules::OnPlayerReady( idPlayer* player, bool ready ) {
	if( gameLocal.DoClientSideStuff() ) {
		if( gameState == GS_WARMUP ) {
			SetWarmupStatusMessage();
		} else if( gameState == GS_GAMEON ) {
			statusText = common->LocalizeText( "guis/hud/in_progress" );
		}
	}
}

/*
================
sdGameRules::OnTeamChange
================
*/
idCVar g_autoFireTeam( "g_autoFireTeam", "0", CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT, "Prompt to join a fireteam when switching to a new team." );

void sdGameRules::OnTeamChange( idPlayer* player, sdTeamInfo* oldteam, sdTeamInfo* team ) {
	if ( oldteam == team ) {
		assert( false );
		return;
	}

	if ( gameState == GS_WARMUP ) {
		if( gameLocal.DoClientSideStuff() ) {
			SetWarmupStatusMessage();
		}
	}

	if ( !gameLocal.isClient ) {
		sdFireTeam* fireTeam = playerState[ player->entityNumber ].fireTeam;
		if ( fireTeam ) {
			fireTeam->RemoveMember( player->entityNumber );
		}

		sdVoteManager::GetInstance().CancelFireTeamVotesForPlayer( player );
		if ( g_autoFireTeam.GetBool() ) {
			if ( team != NULL ) {
				team->TryFindPrivateFireTeam( player );
			}
		}

		if ( gameState == GS_GAMEON && oldteam != NULL ) {
			// when changing teams during game, kill and respawn
			player->Kill( NULL );
		}
	}
}

/*
================
sdGameRules::ProcessChatMessage
================
*/
void sdGameRules::ProcessChatMessage( idPlayer* player, gameReliableClientMessage_t mode, const wchar_t *text ) {
	assert( !gameLocal.isClient );

	if ( player == NULL && mode != GAME_RELIABLE_CMESSAGE_CHAT ) {
		return;
	}

	if ( player != NULL ) {
		if ( playerState[ player->entityNumber ].muteStatus & MF_CHAT ) {
			const sdUserGroup& userGroup = sdUserGroupManager::GetInstance().GetGroup( player->GetUserGroup() );
			if ( !userGroup.HasPermission( PF_NO_MUTE ) ) {
				player->SendLocalisedMessage( declHolder.declLocStrType[ "rules/messages/muted" ], idWStrList() );
				return;
			}
		}

		if ( mode == GAME_RELIABLE_CMESSAGE_CHAT ) {
			if ( g_muteSpecs.GetBool() && player->GetGameTeam() == NULL ) {
				mode = GAME_RELIABLE_CMESSAGE_TEAM_CHAT; // Gordon: muted spectator global chat goes to team chat instead
			} else if ( si_disableGlobalChat.GetBool() ) {
				const sdUserGroup& userGroup = sdUserGroupManager::GetInstance().GetGroup( player->GetUserGroup() );
				if ( !userGroup.HasPermission( PF_NO_MUTE ) ) {
					player->SendLocalisedMessage( declHolder.declLocStrType[ "rules/messages/globalchatdisabled" ], idWStrList() );
					return;
				}
			}
		}
	}

	gameReliableServerMessage_t outMode;
	switch ( mode ) {
		default:
		case GAME_RELIABLE_CMESSAGE_CHAT:
			outMode = GAME_RELIABLE_SMESSAGE_CHAT;
			break;
		case GAME_RELIABLE_CMESSAGE_TEAM_CHAT:
			outMode = GAME_RELIABLE_SMESSAGE_TEAM_CHAT;
			break;
		case GAME_RELIABLE_CMESSAGE_FIRETEAM_CHAT:
			outMode = GAME_RELIABLE_SMESSAGE_FIRETEAM_CHAT;
			break;
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();

	idVec3 location = player == NULL ? vec3_origin : player->GetPhysics()->GetOrigin();
	int clientIndex = player ? player->entityNumber : -1;

	sdReliableServerMessage outMsg( outMode );
	outMsg.WriteVector( location );
	outMsg.WriteChar( clientIndex );
	outMsg.WriteString( text );

	switch ( mode ) {
		case GAME_RELIABLE_CMESSAGE_CHAT:
			outMsg.Send( sdReliableMessageClientInfoAll() );
			AddChatLine( location, sdGameRules::CHAT_MODE_SAY, clientIndex, text );
			break;
		case GAME_RELIABLE_CMESSAGE_TEAM_CHAT: {
			if ( player != NULL ) {
				for ( int i = 0; i < gameLocal.numClients; i++ ) {
					idPlayer* other = gameLocal.GetClient( i );
					if ( other == NULL ) {
						continue;
					}

					if ( other->GetGameTeam() != player->GetGameTeam() ) {
						continue;
					}

					if ( localPlayer == other ) {
						AddChatLine( location, sdGameRules::CHAT_MODE_SAY_TEAM, clientIndex, text );
					} else {
						outMsg.Send( sdReliableMessageClientInfo( i ) );
					}
				}
				break;
			}
		}
		case GAME_RELIABLE_CMESSAGE_FIRETEAM_CHAT: {
			if ( player != NULL ) {
				sdFireTeam* fireTeam = GetPlayerFireTeam( player->entityNumber );
				if ( fireTeam != NULL ) {
					for ( int i = 0; i < fireTeam->GetNumMembers(); i++ ) {
						idPlayer* other = fireTeam->GetMember( i );
						if ( localPlayer == other ) {
							AddChatLine( location, sdGameRules::CHAT_MODE_SAY_FIRETEAM, clientIndex, text );
						} else {
							outMsg.Send( sdReliableMessageClientInfo( other->entityNumber ) );
						}
					}
				} else {
					taskHandle_t taskHandle = player->GetActiveTaskHandle();
					if ( taskHandle.IsValid() ) {
						for ( int i = 0; i < MAX_CLIENTS; i++ ) {
							idPlayer* other = gameLocal.GetClient( i );
							if ( other == NULL ) {
								continue;
							}

							if ( other->GetActiveTaskHandle() == taskHandle ) {
								if ( localPlayer == other ) {
									AddChatLine( location, sdGameRules::CHAT_MODE_SAY_FIRETEAM, clientIndex, text );
								} else {
									outMsg.Send( sdReliableMessageClientInfo( other->entityNumber ) );
								}
							}
						}
					}
				}
			}
			break;
		}
	}
}

/*
================
sdGameRules::ApplyNetworkState
================
*/
void sdGameRules::ApplyNetworkState( const sdEntityStateNetworkData& newState ) {
	NET_GET_NEW( sdGameRulesNetworkState );

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		playerState[ i ].ping = newData.pings[ i ];

		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		player->SetGameTeam( newData.teams[ i ] );
		player->SetUserGroup( newData.userGroups[ i ] );
	}

	matchStartedTime = newData.matchStartedTime;
	nextStateSwitch = newData.nextStateSwitch;

	gameState_t newGamaState = static_cast< gameState_t >( newData.state );
	if ( newGamaState != gameState ) {
//		gameLocal.DPrintf( "%s -> %s\n", gameStateStrings[ gameState ], gameStateStrings[ newGamaState ] );
		NewState( newGamaState );
	}
}

/*
================
sdGameRules::ReadNetworkState
================
*/
void sdGameRules::ReadNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	NET_GET_STATES( sdGameRulesNetworkState );

	sdTeamManagerLocal& teamManager = sdTeamManager::GetInstance();

	bool teamsChanged = msg.ReadBool();
	bool userGroupsChanged = msg.ReadBool();
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		newData.pings[ i ] = msg.ReadDeltaLong( baseData.pings[ i ] );
	}

	if ( teamsChanged ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			newData.teams[ i ] = teamManager.ReadTeamFromStream( baseData.teams[ i ], msg );
		}
	} else {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			newData.teams[ i ] = baseData.teams[ i ];
		}
	}

	if ( userGroupsChanged ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			newData.userGroups[ i ] = msg.ReadDeltaLong( baseData.userGroups[ i ] );
		}
	} else {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			newData.userGroups[ i ] = baseData.userGroups[ i ];
		}
	}

	newData.state				= msg.ReadDeltaLong( baseData.state );
	newData.matchStartedTime	= msg.ReadDeltaLong( baseData.matchStartedTime );
	newData.nextStateSwitch		= msg.ReadDeltaLong( baseData.nextStateSwitch );
}

/*
================
sdGameRules::WriteNetworkState
================
*/
void sdGameRules::WriteNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	NET_GET_STATES( sdGameRulesNetworkState );

	bool teamsChanged = false;
	bool userGroupsChanged = false;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		newData.pings[ i ] = playerState[ i ].ping;

		newData.teams[ i ] = NULL;
		newData.userGroups[ i ] = playerState[ i ].userGroup;
		idPlayer* player = gameLocal.GetClient( i );
		if ( player ) {
			newData.teams[ i ] = player->GetTeam();
		}
		if ( newData.teams[ i ] != baseData.teams[ i ] ) {
			teamsChanged = true;
		}
		if ( newData.userGroups[ i ] != baseData.userGroups[ i ] ) {
			userGroupsChanged = true;
		}
	}
	newData.state				= gameState;
	newData.matchStartedTime	= matchStartedTime;
	newData.nextStateSwitch		= nextStateSwitch;

	sdTeamManagerLocal& teamManager = sdTeamManager::GetInstance();

	msg.WriteBool( teamsChanged );
	msg.WriteBool( userGroupsChanged );
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		msg.WriteDeltaLong( baseData.pings[ i ], newData.pings[ i ] );
	}
	if ( teamsChanged ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			teamManager.WriteTeamToStream( baseData.teams[ i ], newData.teams[ i ], msg );
		}
	}
	if ( userGroupsChanged ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			msg.WriteDeltaLong( baseData.userGroups[ i ], newData.userGroups[ i ] );
		}
	}

	msg.WriteDeltaLong( baseData.state, newData.state );
	msg.WriteDeltaLong( baseData.matchStartedTime, newData.matchStartedTime );
	msg.WriteDeltaLong( baseData.nextStateSwitch, newData.nextStateSwitch );
}

/*
================
sdGameRules::CheckNetworkStateChanges
================
*/
bool sdGameRules::CheckNetworkStateChanges( const sdEntityStateNetworkData& baseState ) const {
	NET_GET_BASE( sdGameRulesNetworkState );

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( playerState[ i ].ping != baseData.pings[ i ] ) {
			return true;
		}
		if ( playerState[ i ].userGroup != baseData.userGroups[ i ] ) {
			return true;
		}

		sdTeamInfo* teamInfo = NULL;
		idPlayer* player = gameLocal.GetClient( i );
		if ( player ) {
			teamInfo = player->GetTeam();
		}
		if ( teamInfo != baseData.teams[ i ] ) {
			return true;
		}
	}

	NET_CHECK_FIELD( matchStartedTime, matchStartedTime );
	NET_CHECK_FIELD( nextStateSwitch, nextStateSwitch );

	if ( static_cast< gameState_t >( baseData.state ) != gameState ) {
		return true;
	}

	return false;
}

/*
================
sdGameRules::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdGameRules::CreateNetworkStructure( void ) const {
	return new sdGameRulesNetworkState();
}

/*
================
sdGameRules::OnNewScriptLoad
================
*/
void sdGameRules::OnNewScriptLoad( void ) {
	assert( !scriptObject );

	scriptObject = gameLocal.program->AllocScriptObject( this, "rules" );

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetPreConstructor(), h1 );
}

/*
================
sdGameRules::OnScriptChange
================
*/
void sdGameRules::OnScriptChange( void ) {
	if ( scriptObject ) {
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetDestructor(), h1 );

		gameLocal.program->FreeScriptObject( scriptObject );
	}
}

/*
================
sdGameRules::OnConnect
================
*/
void sdGameRules::OnConnect( idPlayer* player ) {
	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnConnect" ), h1 );
}

/*
================
sdGameRules::OnNetworkEvent
================
*/
void sdGameRules::OnNetworkEvent( const char* message ) {
	gameLocal.SetActionCommand( message );

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnNetworkEvent" ), h1 );
}

/*
================
sdGameRules::Event_SendNetworkEvent
================
*/
void sdGameRules::Event_SendNetworkEvent( int clientIndex, bool isRepeaterClient, const char* message ) {
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
	msg.WriteLong( NETWORKEVENT_RULES_ID );
	msg.WriteString( message );
	if ( isRepeaterClient ) {
		msg.Send( sdReliableMessageClientInfoRepeater( clientIndex ) );
	} else {
		msg.Send( sdReliableMessageClientInfo( clientIndex ) );
	}
}

/*
================
sdGameRules::Event_SetEndGameCamera
================
*/
void sdGameRules::Event_SetEndGameCamera( idEntity* other ) {
	if ( gameLocal.isClient ) {
		return;
	}

	endGameCamera = other;
	if ( gameLocal.isServer ) {
		SendCameraEvent( other, sdReliableMessageClientInfoAll() );
	}
}

/*
================
sdGameRules::Event_EndGame
================
*/
void sdGameRules::Event_EndGame( void ) {
	EndGame();
}

/*
================
sdGameRules::Event_SetWinningTeam
================
*/
void sdGameRules::Event_SetWinningTeam( idScriptObject* object ) {
	SetWinner( object ? object->GetClass()->Cast< sdTeamInfo >() : NULL );
}

/*
================
sdGameRules::Event_GetKeySuffix
================
*/
void sdGameRules::Event_GetKeySuffix( void ) {
	sdProgram::ReturnString( GetKeySuffix() );
}

/*
================
sdGameRules::ShuffleTeams
================
*/
void sdGameRules::ShuffleTeams( shuffleMode_t sm ) {
	idList< shuffleData_t > players;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		sdTeamInfo* team = player->GetGameTeam();
		if ( team == NULL ) {
			continue;
		}

		shuffleData_t& data = players.Alloc();
		data.second	= player;

		switch ( sm ) {
			case SM_RANDOM:
				data.first	= gameLocal.random.RandomInt( 999999 );
				break;
			case SM_XP:
				data.first	= player->GetProficiencyTable().GetXP();
				break;
			case SM_SWAP:
				data.first = 0;
				break;
		}
	}

	switch ( sm ) {
		case SM_RANDOM:
			players.Sort( SortPlayers_Random );
			break;
		case SM_XP:
			players.Sort( SortPlayers_XP );
			break;
	}

	sdTeamManagerLocal& teamManager = sdTeamManager::GetInstance();
	int numTeams = teamManager.GetNumTeams();

	if ( sm == SM_SWAP ) {
		for ( int i = 0; i < players.Num(); i++ ) {
			idPlayer* player = players[ i ].second;

			int index = player->GetGameTeam()->GetIndex() + 1;
			if ( index >= numTeams ) {
				index = 0;
			}
			SetClientTeam( player, index + 1, true, "" );
		}
	} else {
		int cycle = 0;

		for ( int i = 0; i < players.Num(); i++ ) {
			idPlayer* player = players[ i ].second;
			SetClientTeam( player, cycle + 1, true, "" );

			cycle++;
			if ( cycle >= numTeams ) {
				cycle = 0;
			}
		}
	}
}

/*
================
sdGameRules::SortPlayers_Random
================
*/
int sdGameRules::SortPlayers_Random( const shuffleData_t* a, const shuffleData_t* b ) {
	return b->first - a->first;
}

/*
================
sdGameRules::SortPlayers_XP
================
*/
int sdGameRules::SortPlayers_XP( const shuffleData_t* a, const shuffleData_t* b ) {
	return b->first - a->first;
}

/*
============
sdGameRules::UpdateChatLines
============
*/
void sdGameRules::UpdateChatLines() {
	sdChatLine::node_t* node = chatHead.NextNode();
	while( node != NULL ) {
		sdChatLine::node_t* next = node->NextNode();
		sdChatLine* line = node->Owner();
		line->CheckExpired();
		node = next;
	}
}

/*
============
sdGameRules::CreateChatList
============
*/
void sdGameRules::CreateChatList( sdUIList* list ) {
	sdUIList::ClearItems( list );
	if( gameLocal.rules == NULL ) {
		return;
	}

	assert( list );

	int mode = 0;
	int num = 0;
	int showExpired = 0;
	list->GetUI()->PopScriptVar( num );
	list->GetUI()->PopScriptVar( mode );
	list->GetUI()->PopScriptVar( showExpired );

	if( num <= 0 ) {
		num = MAX_CHAT_LINES;
	}

	sdChatLine::node_t* node = gameLocal.rules->chatHead.PrevNode();

	idWStr cleanedInput;

	int added = 0;
	int index = -1;
	while( node != NULL && added < num ) {
		const sdChatLine& line = *node->Owner();
		node = node->PrevNode();

		if( mode != CHAT_MODE_OBITUARY && line.IsObituary() ) {
			continue;
		}
		if( mode == CHAT_MODE_OBITUARY && !line.IsObituary() ) {
			continue;
		}

		if( !showExpired && line.IsExpired() ) {
			continue;
		}

		cleanedInput = line.GetText();
		sdUIList::CleanUserInput( cleanedInput );
		index = sdUIList::InsertItem( list, cleanedInput.c_str(), 0, 0 );
		sdUIList::SetItemForeColor( list, line.GetColor(), index, -1 );
		added++;
	}
}

/*
============
sdGameRules::ParseNetworkMessage
============
*/
bool sdGameRules::ParseNetworkMessage( const idBitMsg& msg ) {
	int msgType = msg.ReadLong();
	if ( msgType == EVENT_CREATE ) {
		int typeNum = msg.ReadLong();
		idTypeInfo* type = idClass::GetType( typeNum );
		assert( type );
		gameLocal.SetRules( type );
		return true;
	}

	assert( gameLocal.rules );

	return gameLocal.rules->ParseNetworkMessage( msgType, msg );
}

/*
============
sdGameRules::ParseNetworkMessage
============
*/
bool sdGameRules::ParseNetworkMessage( int msgType, const idBitMsg& msg ) {
	switch ( msgType ) {
		case EVENT_SETCAMERA:
			endGameCamera.ForceSpawnId( msg.ReadLong() );
			return true;
	}
	return false;
}

/*
============
sdGameRules::GetStatusText
============
*/
const wchar_t* sdGameRules::GetStatusText() const {
	return statusText.c_str();
}

/*
============
sdGameRules::SetPlayerFireTeam
============
*/
void sdGameRules::SetPlayerFireTeam( int clientNum, sdFireTeam* fireTeam ) {
	playerState[ clientNum ].fireTeam = fireTeam;
	idPlayer* player = gameLocal.GetClient( clientNum );
	if ( player ) {
		player->OnFireTeamJoined( fireTeam );
	}
}

/*
================
sdGameRules::HandleGuiEvent
================
*/
bool sdGameRules::HandleGuiEvent( const sdSysEvent* event ) {
	if ( !IsEndGame() ) {
		return false;
	}

	sdUserInterfaceLocal* scoreboardUI = gameLocal.GetUserInterface( gameLocal.localPlayerProperties.GetScoreBoard() );
	if ( !scoreboardUI ) {
		return false;
	}

	return scoreboardUI->PostEvent( event );
}

/*
================
sdGameRules::TranslateGuiBind
================
*/
bool sdGameRules::TranslateGuiBind( const idKey& key, sdKeyCommand** cmd ) {
	if ( !IsEndGame() ) {
		return false;
	}

	sdUserInterfaceLocal* scoreboardUI = gameLocal.GetUserInterface( gameLocal.localPlayerProperties.GetScoreBoard() );
	if ( scoreboardUI == NULL ) {
		return false;
	}

	if ( scoreboardUI->Translate( key, cmd ) ) {
		return true;
	}
	return false;
}


/*
============
sdGameRules::sdChatLine::Set
============
*/
void sdGameRules::sdChatLine::Set( const wchar_t* text, chatMode_t mode ) {
	this->time = gameLocal.ToGuiTime( gameLocal.time );
	CheckExpired();

	if( flags.expired ) {
		return;
	}

	this->text = text;

	flags.obituary = ( mode == CHAT_MODE_OBITUARY );
	flags.team = ( mode == CHAT_MODE_QUICK_TEAM ) || ( mode == CHAT_MODE_SAY_TEAM ) || ( mode == CHAT_MODE_SAY_FIRETEAM ) || ( mode == CHAT_MODE_QUICK_FIRETEAM );

	if ( mode == CHAT_MODE_SAY_TEAM || mode == CHAT_MODE_QUICK_TEAM ) {
		sdProperties::sdFromString( color, g_chatTeamColor.GetString() );
	} else if ( mode == CHAT_MODE_SAY_FIRETEAM || mode == CHAT_MODE_QUICK_FIRETEAM ) {
		sdProperties::sdFromString( color, g_chatFireTeamColor.GetString() );
	} else {
		sdProperties::sdFromString( color, g_chatDefaultColor.GetString() );
	}
}

/*
============
sdGameRules::ArgCompletion_RuleTypes
============
*/
void sdGameRules::ArgCompletion_RuleTypes( const idCmdArgs &args, void( *callback )( const char *s ) ) {
	for ( int i = 0; i < idClass::GetNumTypes(); i++ ) {
		idTypeInfo* type = idClass::GetType( i );
		if ( !sdGameRules::IsRuleType( *type ) ) {
			continue;
		}

		callback( va( "%s %s", args.Argv( 0 ), type->classname ) );
	}
}

/*
============
sdGameRules::GetWarmupTime
============
*/
int sdGameRules::GetWarmupTime( void ) {
	return Max( 0, MINS2MS( g_warmup.GetFloat() ) );
}

/*
============
sdGameRules::GetTimeLimit
============
*/
int sdGameRules::GetTimeLimit( void ) const {
	return gameLocal.serverInfoData.timeLimit;
}

/*
============
sdGameRules::OnGameState_Warmup
============
*/
void sdGameRules::OnGameState_Warmup( void ) {
	adminStarted = false;
	needsRestart = false;
	autoReadyStartTime = -1;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		player->SetSpawnPoint( NULL );
	}
}


/*
============
sdGameRules::NumReady
============
*/
int sdGameRules::NumReady( int& total ) {
	total = 0;
	int ready = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL || player->GetGameTeam() == NULL || player->IsType( idBot::Type ) ) {
			continue;
		}

		total++;
		if ( player->IsReady() ) {
			ready++;
		}
	}
	return ready;
}

/*
============
sdGameRules::ArePlayersReady
============
*/
readyState_e sdGameRules::ArePlayersReady( bool readyIfNoPlayers, bool checkMin, int* numRequired ) {
	float readyFrac = idMath::ClampFloat( 0.f, 1.f, ( gameLocal.serverInfoData.readyPercent / 100.f ) );

	if ( checkMin ) {
		if ( gameLocal.IsMultiPlayer() ) {
			bool allowBots = false;
			if ( !networkSystem->IsRankedServer() ) {
				allowBots = g_useBotsInPlayerTotal.GetBool();
			}

			int c = NumActualClients( false, allowBots );
			if ( c < gameLocal.serverInfoData.minPlayers ) {
				if ( numRequired != NULL ) {
					*numRequired = gameLocal.serverInfoData.minPlayers - c;
				}
				return RS_NOT_ENOUGH_CLIENTS;
			}
		}
	}

	int total = 0;
	int ready = NumReady( total );

	float amountReady;
	if ( total == 0 ) {
		amountReady = 0;

		if ( readyIfNoPlayers ) {
			return RS_READY;
		}
	} else {
		amountReady = ready / ( float )total;
	}

	if ( amountReady < readyFrac ) {
		if ( numRequired != NULL ) {
			*numRequired = idMath::Ceil( readyFrac * total ) - ready;
		}
		return RS_NOT_ENOUGH_READY;
	}


	return RS_READY;
}

/*
============
sdGameRules::CanStartMatch
============
*/
bool sdGameRules::CanStartMatch( void ) const {
	if ( adminStarted ) {
		return true;
	}

	if ( gameLocal.serverInfoData.adminStart ) {
		return false;
	}

	switch ( ArePlayersReady( false, true ) ) {
	case RS_READY:
		return true;
	case RS_NOT_ENOUGH_CLIENTS:
		return false;
	case RS_NOT_ENOUGH_READY:
		if ( g_autoReadyPercent.GetFloat() != 0.f ) {
			int totalPlaying = NumActualClients( false );
			int playersRequired = si_maxPlayers.GetInteger() * g_autoReadyPercent.GetFloat() / 100.f;

			if ( autoReadyStartTime == -1 ) {
				if ( totalPlaying >= playersRequired ) {
					autoReadyStartTime = gameLocal.time;
				}
			} else {
				if ( totalPlaying < playersRequired ) {
					autoReadyStartTime = -1;
				} else {
					if ( ( gameLocal.time - autoReadyStartTime ) > MINS2MS( g_autoReadyWait.GetFloat() ) ) {
						return true;
					}
				}
			}
		}
		return false;
	default:
		gameLocal.Warning( "CanStartMatch: Unknown ready state" );
		break;
	}

	return false;
}

/*
============
sdGameRules::StartMatch
============
*/
void sdGameRules::StartMatch( void ) {
	int warmupTime = GetWarmupTime();
	if ( warmupTime == 0 ) {
		NewState( GS_GAMEON );
	} else {
		needsRestart = true;
		NewState( GS_COUNTDOWN );
		NextStateDelayed( GS_GAMEON, warmupTime );
	}
}

/*
============
sdGameRules::ClearPlayerReadyFlags
============
*/
void sdGameRules::ClearPlayerReadyFlags( void ) const {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		player->SetReady( false, true );
	}
}

/*
============
sdGameRules::OnTimeLimitHit
============
*/
void sdGameRules::OnTimeLimitHit( void ) {
	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnTimeLimitHit" ), h1 );
}

/*
============
sdGameRules::WriteInitialReliableMessages
============
*/
void sdGameRules::WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) {
	if ( endGameCamera.IsValid() ) {
		SendCameraEvent( endGameCamera, target );
	}
}

/*
============
sdGameRules::SendCameraEvent
============
*/
void sdGameRules::SendCameraEvent( idEntity* entity, const sdReliableMessageClientInfoBase& target ) {
	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_RULES_DATA );
	msg.WriteLong( sdGameRules::EVENT_SETCAMERA );
	msg.WriteLong( gameLocal.GetSpawnId( entity ) );
	msg.Send( target );
}

/*
============
sdGameRules::CallScriptEndGame
============
*/
void sdGameRules::CallScriptEndGame( void ) {
	if ( !gameLocal.isClient ) {
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnGameEnd" ), h1 );
	}
}

/*
============
sdGameRules::RecordWinningTeam
============
*/
void sdGameRules::RecordWinningTeam( sdTeamInfo* winner, const char* prefix, bool includeTeamName ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL || player->IsSpectator() ) {
			continue;
		}

		sdTeamInfo* playerTeam = player->GetGameTeam();

		const char* statName = NULL;
		if ( winner == NULL ) {
			statName = va( includeTeamName ? "%s_drawn_%s" : "%s_drawn", prefix, playerTeam->GetLookupName() );
		} else {
			if ( winner == playerTeam ) {
				statName = va( includeTeamName ? "%s_won_%s" : "%s_won", prefix, playerTeam->GetLookupName() );
			} else {
				statName = va( includeTeamName ? "%s_lost_%s" : "%s_lost", prefix, playerTeam->GetLookupName() );
			}
		}

		sdPlayerStatEntry* stat = sdGlobalStatsTracker::GetInstance().GetStat( sdGlobalStatsTracker::GetInstance().AllocStat( statName, sdNetStatKeyValue::SVT_INT ) );
		stat->IncreaseValue( i, 1 );
	}
}

/*
============
sdGameRules::IsRuleType
============
*/
bool sdGameRules::IsRuleType( const idTypeInfo& type ) {
	if ( networkSystem->IsRankedServer() ) {
		return &type == &sdGameRulesCampaign::Type;
	}
	return type.IsType( sdGameRules::Type ) && ( &type != &sdGameRules::Type );
}

/*
============
sdGameRules::UpdateClientFromServerInfo
============
*/
void sdGameRules::UpdateClientFromServerInfo( const idDict& serverInfo, bool allowMedia ) {
	if( gameState == GS_WARMUP ) {
		SetWarmupStatusMessage();
	} else if( gameState == GS_GAMEON ) {
		statusText = common->LocalizeText( "guis/hud/in_progress" );
	}
}

/*
============
sdGameRules::SetupLoadScreenUI
============
*/
void sdGameRules::SetupLoadScreenUI( sdUserInterfaceScope& scope, const char* status, bool currentMap, int mapIndex, const idDict& metaData, const sdDeclMapInfo* mapInfo ) {
	using namespace sdProperties;	

	// setup the icon state
	if ( sdProperty* property = scope.GetProperty( va( "status%d", mapIndex ), PT_STRING ) ) {
		*property->value.stringValue = status;
	}

	// setup the name
	if ( sdProperty* property = scope.GetProperty( va( "title%d", mapIndex ), PT_WSTRING ) ) {
		*property->value.wstringValue = va( L"%hs", metaData.GetString( "pretty_name" ) );
	}
	

	if( currentMap ) {
		// setup the name
		if ( sdProperty* property = scope.GetProperty( "mapName", PT_WSTRING ) ) {
			*property->value.wstringValue = va( L"%hs", metaData.GetString( "pretty_name" ) );
		}
	}

	if( mapInfo != NULL ) {
		idStr defaultPosition = va( "%i %i", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 );

		// setup the icon position
		idVec2 position = mapInfo->GetData().GetVec2( "mapPosition", defaultPosition.c_str() );
		if ( sdProperty* property = scope.GetProperty( va( "position%d", mapIndex ), PT_VEC2 ) ) {
			*property->value.vec2Value = position;
		}

		if( currentMap ) {
			// setup the map shot
			if ( sdProperty* property = scope.GetProperty( "mapShot", PT_STRING ) ) {
				*property->value.stringValue = mapInfo->GetServerShot()->GetName();
			}
			// setup the location
			if ( sdProperty* property = scope.GetProperty( "mapLocation", PT_INT ) ) {
				*property->value.intValue = declHolder.declLocStrType.LocalFind( mapInfo->GetData().GetString( "mapLocation", "guis/mainmenu/nointel" ) )->Index();
			}

			// setup the briefing
			if ( sdProperty* property = scope.GetProperty( "mapBriefing", PT_INT ) ) {
				*property->value.intValue = declHolder.declLocStrType.LocalFind( mapInfo->GetData().GetString( "mapBriefing", "guis/mainmenu/nointel" ) )->Index();
			}

			// setup the music
			if ( sdProperty* property = scope.GetProperty( "mapMusic", PT_STRING ) ) {
				*property->value.stringValue = mapInfo->GetData().GetString( "snd_music" );
			}
		}
	}
}

/*
============
sdGameRules::GetProbeState
============
*/
byte sdGameRules::GetProbeState( void ) const {
	switch( gameState ) {
		case GS_WARMUP:		// FALL THROUGH
		case GS_COUNTDOWN:
			return PGS_WARMUP;
		case GS_GAMEON:
			return PGS_RUNNING;
		case GS_GAMEREVIEW:
			return PGS_REVIEWING;
	}
	return PGS_LOADING;
}

/*
============
sdGameRules::GetServerBrowserScore
============
*/
int sdGameRules::GetServerBrowserScore( const sdNetSession& session ) const {
	int score = 0;

	if ( session.GetGameState() & PGS_WARMUP ) {
		score += sdHotServerList::BROWSER_GOOD_BONUS;
	} else if ( session.GetGameState() & PGS_WARMUP ) {
		score += 0;
	} else {
		int minsLeft = ( int )( MS2SEC( session.GetSessionTime() ) / 60.f );
		if ( minsLeft >= 15 ) {
			score += sdHotServerList::BROWSER_OK_BONUS;
		}
	}

	return score;
}

/*
============
sdGameRules::GetBrowserStatusString
============
*/
void sdGameRules::GetBrowserStatusString( idWStr& str, const sdNetSession& netSession ) const {
	str.Clear();

	if( netSession.GetGameState() & PGS_WARMUP ) {
		str = va( L"%ls", declHolder.declLocStrType.LocalFind( "guis/mainmenu/server/warmup" )->GetText() );
		return;
	} else if( netSession.GetGameState() & PGS_LOADING ) {
		str = va( L"%ls", declHolder.declLocStrType.LocalFind( "guis/mainmenu/server/loading" )->GetText() );
		return;
	} else if( netSession.GetGameState() & PGS_REVIEWING ) {
		str = va( L"%ls", declHolder.declLocStrType.LocalFind( "guis/mainmenu/server/reviewing" )->GetText() );
		return;
	} else {
		if( netSession.GetSessionTime() == 0 ) {
			str = infinity->GetText();
		} else {
			idWStr::hmsFormat_t format;
			format.showZeroMinutes = true;
			format.showZeroSeconds = false;
			str = va( L"%ls", idWStr::MS2HMS( netSession.GetSessionTime(), format ) );
		}
	}
}

/*
============
sdGameRules::InhibitEntitySpawn
============
*/
bool sdGameRules::InhibitEntitySpawn( idDict &spawnArgs ) const {
	if ( spawnArgs.GetBool( "stopwatchOnly" ) ) {
		return true;
	}

	return false;
}


/*
============
sdGameRules_SingleMapHelper::ArgCompletion_StartGame
============
*/
void sdGameRules_SingleMapHelper::ArgCompletion_StartGame( const idCmdArgs& args, argCompletionCallback_t callback ) {
	// public builds only allow maps with metadata
	// otherwise, unofficial defs wouldn't be loaded after pure restarts

#if defined( SD_PUBLIC_BUILD )
	if( gameLocal.mapMetaDataList == NULL ) {
		return;
	}

	const char* cmd = args.Argv( 1 );
	int len = idStr::Length( cmd );

	int num = gameLocal.mapMetaDataList->GetNumMetaData();
	for ( int i = 0; i < num; i++ ) {
		const metaDataContext_t& metaData = gameLocal.mapMetaDataList->GetMetaDataContext( i );
		if ( !gameLocal.IsMetaDataValidForPlay( metaData, false ) ) {
			continue;
		}
		const idDict& meta = *metaData.meta;

		const char* metaName = meta.GetString( "metadata_name" );
		if ( idStr::Icmpn( metaName, cmd, len ) ) {
			continue;
		}

		callback( va( "%s %s", args.Argv( 0 ), metaName ) );
	}
#else
	idCmdSystem::ArgCompletion_EntitiesName( args, callback );
#endif
}


/*
============
sdGameRules_SingleMapHelper::OnUserStartMap
============
*/
userMapChangeResult_e sdGameRules_SingleMapHelper::OnUserStartMap( const char* text, idStr& reason, idStr& mapName ) {
	mapName = text;	
	SanitizeMapName( mapName, true );

#if defined( SD_PUBLIC_BUILD )
	idStr metaDataName = text;
	SanitizeMapName( metaDataName, false );

	const metaDataContext_t* metaData = gameLocal.mapMetaDataList->FindMetaDataContext( metaDataName.c_str() );
	if ( metaData == NULL || !gameLocal.IsMetaDataValidForPlay( *metaData, false ) ) {
		reason = va( "Unknown map '%s'", metaDataName.c_str() );
		return UMCR_ERROR;
	}

	if( metaData->addon ) {
		if( !fileSystem->IsAddonPackReferenced( metaData->pak ) ) {
			fileSystem->ReferenceAddonPack( metaData->pak );

			idCmdArgs args;
			args.AppendArg( "spawnServer" );
			args.AppendArg( text );
			cmdSystem->SetupReloadEngine( args );
			return UMCR_STOP;	
		}		
	}	
#endif
	return UMCR_CONTINUE;
}

/*
================
sdGameRules_SingleMapHelper::SanitizeMapName
================
*/
void sdGameRules_SingleMapHelper::SanitizeMapName( idStr& mapName, bool setExtension ) {
	if ( mapName.Icmpn( "maps/", 5 ) ) {
		mapName = "maps/" + mapName;
	}
	if ( setExtension ) {
		mapName.SetFileExtension( "entities" );
	} else {
		mapName.StripFileExtension();
	}
}