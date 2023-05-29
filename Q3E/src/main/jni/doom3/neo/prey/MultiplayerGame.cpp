// Copyright (C) 2004 Id Software, Inc.
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#if INGAME_DEBUGGER_ENABLED //HUMANHEAD rww - make debugger work in mp too
	#include "../prey/sys_debugger.h"
#endif

// could be a problem if players manage to go down sudden deaths till this .. oh well
#define LASTMAN_NOLIVES -20

#define MAX_SCOREBOARD_NAMES	8 //HUMANHEAD rww - maximum number of players to display on the scoreboard

idCVar g_spectatorChat( "g_spectatorChat", "0", CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "let spectators talk to everyone during game" );

// global sounds transmitted by index - 0 .. SND_COUNT
// sounds in this list get precached on MP start
const char *idMultiplayerGame::GlobalSoundStrings[] = {
	"sound/feedback/voc_youwin.wav",
	"sound/feedback/voc_youlose.wav",
	"sound/feedback/fight.wav",
	"sound/feedback/vote_now.wav",
	"sound/feedback/vote_passed.wav",
	"sound/feedback/vote_failed.wav",
	"sound/feedback/three.wav",
	"sound/feedback/two.wav",
	"sound/feedback/one.wav",
	"sound/feedback/sudden_death.wav",
	//HUMANHEAD rww
	"sound/feedback/lead_taken.wav",
	"sound/feedback/lead_lost.wav",
	"sound/feedback/lead_tied.wav"
	//HUMANHEAD END
};

// handy verbose
const char *idMultiplayerGame::GameStateStrings[] = {
	"INACTIVE",
	"WARMUP",
	"COUNTDOWN",
	"GAMEON",
	"SUDDENDEATH",
	"GAMEREVIEW",
	"NEXTGAME"
};

const char *idMultiplayerGame::MPGuis[] = {
	"guis/mphud.gui",
	"guis/mpmain.gui",
	"guis/mpmsgmode.gui",
	"guis/netmenu.gui",
	NULL
};

const char *idMultiplayerGame::ThrottleVars[] = {
	"ui_spectate",
	"ui_ready",
	"ui_team",
	"ui_modelNum", //HUMANHEAD rww
	NULL
};

const char *idMultiplayerGame::ThrottleVarsInEnglish[] = {
	"#str_06738",
	"#str_06737",
	"#str_01991",
	"#str_06739", //HUMANHEAD rww
	NULL
};

const int idMultiplayerGame::ThrottleDelay[] = {
	8,
	5,
	5,
	5 //HUMANHEAD rww
};

/*
================
idMultiplayerGame::idMultiplayerGame
================
*/
idMultiplayerGame::idMultiplayerGame() {
	scoreBoard = NULL;
	spectateGui = NULL;
	guiChat = NULL;
	mainGui = NULL;
#ifdef _GUITEST_SERVERWAIT
	serverWaitGui = NULL; //HUMANHEAD rww
#endif
	mapList = NULL;
	chatHistoryList = NULL;		// HUMANHEAD pdm
	msgmodeGui = NULL;
	lastGameType = GAME_SP;
	Clear();
}

/*
================
idMultiplayerGame::Shutdown
================
*/
void idMultiplayerGame::Shutdown( void ) {
	Clear();
}

/*
================
idMultiplayerGame::SetMenuSkin
================
*/
/* HUMANHEAD pdm: removed
void idMultiplayerGame::SetMenuSkin( void ) {
	// skins
	idStr str = cvarSystem->GetCVarString( "mod_validSkins" );
	idStr uiSkin = cvarSystem->GetCVarString( "ui_skin" );
	idStr skin;
	int skinId = 1;
	int count = 1;
	while ( str.Length() ) {
		int n = str.Find( ";" );
		if ( n >= 0 ) {
			skin = str.Left( n );
			str = str.Right( str.Length() - n - 1 );
		} else {
			skin = str;
			str = "";
		}
		if ( skin.Icmp( uiSkin ) == 0 ) {
			skinId = count;
		}
		count++;
	}

	for ( int i = 0; i < count; i++ ) {
		mainGui->SetStateInt( va( "skin%i", i+1 ), 0 );
	}
	mainGui->SetStateInt( va( "skin%i", skinId ), 1 );
}
*/

/*
================
idMultiplayerGame::Reset
================
*/
void idMultiplayerGame::Reset() {
	if ( chatHistoryList ) {
		chatHistoryList->Clear();
	}
	Clear();
	assert( !scoreBoard && !spectateGui && !guiChat && !mainGui && !mapList );
	assert( !chatHistoryList );	// HUMANHEAD pdm
	scoreBoard = uiManager->FindGui( "guis/scoreboard.gui", true, false, true );
	spectateGui = uiManager->FindGui( "guis/spectate.gui", true, false, true );
	guiChat = uiManager->FindGui( "guis/chat.gui", true, false, true );
	mainGui = uiManager->FindGui( "guis/mpmain.gui", true, false, true );
#ifdef _GUITEST_SERVERWAIT
	serverWaitGui = uiManager->FindGui( "guis/mpserverwait.gui", true, false, true ); //HUMANHEAD rww
#endif
	mapList = uiManager->AllocListGUI( );
	mapList->Config( mainGui, "mapList" );
	// HUMANHEAD pdm: chat history
	chatHistoryList = uiManager->AllocListGUI();
	chatHistoryList->Config( mainGui, "chatHistory" );
	// HUMANHEAD END
	// set this GUI so that our Draw function is still called when it becomes the active/fullscreen GUI
	mainGui->SetStateBool( "gameDraw", true );
	mainGui->SetKeyBindingNames();
	mainGui->SetStateInt( "com_imageQuality", cvarSystem->GetCVarInteger( "com_imageQuality" ) );
//	SetMenuSkin();	// HUMANHEAD pdm: removed
	msgmodeGui = uiManager->FindGui( "guis/mpmsgmode.gui", true, false, true );
	msgmodeGui->SetStateBool( "gameDraw", true );
	ClearGuis();
	ClearChatData();
	warmupEndTime = 0;
}

/*
================
idMultiplayerGame::ServerClientConnect
================
*/
void idMultiplayerGame::ServerClientConnect( int clientNum ) {
	memset( &playerState[ clientNum ], 0, sizeof( playerState[ clientNum ] ) );
}

/*
================
idMultiplayerGame::SpawnPlayer
================
*/
void idMultiplayerGame::SpawnPlayer( int clientNum ) {

	bool ingame = playerState[ clientNum ].ingame;

	memset( &playerState[ clientNum ], 0, sizeof( playerState[ clientNum ] ) );
	if ( !gameLocal.isClient ) {		
		//HUMANHEAD PCF rww 05/16/06
		playerState[clientNum].wins = persistentPlayerDict.GetInt(va("%s_wins", gameLocal.userInfo[clientNum].GetString( "ui_name" )));
		//HUMANHEAD END

		idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ clientNum ] );
		p->spawnedTime = gameLocal.time;
		if ( gameLocal.gameType == GAME_TDM ) {
			SwitchToTeam( clientNum, -1, p->team );
		}
		p->tourneyRank = 0;
		if ( gameLocal.gameType == GAME_TOURNEY && gameState == GAMEON ) {
			p->tourneyRank++;
		}
		playerState[ clientNum ].ingame = ingame;
	}
}

/*
================
idMultiplayerGame::Clear
================
*/
void idMultiplayerGame::Clear() {
	int i;

	gameState = INACTIVE;
	nextState = INACTIVE;
	pingUpdateTime = 0;
	vote = VOTE_NONE;
	voteTimeOut = 0;
	voteExecTime = 0;
	nextStateSwitch = 0;
	matchStartedTime = 0;
	currentTourneyPlayer[ 0 ] = -1;
	currentTourneyPlayer[ 1 ] = -1;
	one = two = three = false;
	memset( &playerState, 0 , sizeof( playerState ) );
	lastWinner = -1;
	currentMenu = 0;
	bCurrentMenuMsg = false;
	nextMenu = 0;
	pureReady = false;
	scoreBoard = NULL;
	spectateGui = NULL;
	guiChat = NULL;
	mainGui = NULL;
#ifdef _GUITEST_SERVERWAIT
	serverWaitGui = NULL; //HUMANHEAD rww
#endif
	msgmodeGui = NULL;
	if ( mapList ) {
		uiManager->FreeListGUI( mapList );
		mapList = NULL;
	}
	// HUMANHEAD pdm
	if ( chatHistoryList ) {
		uiManager->FreeListGUI( chatHistoryList );
		chatHistoryList = NULL;
	}
	// HUMANHEAD END
	fragLimitTimeout = 0;
	memset( &switchThrottle, 0, sizeof( switchThrottle ) );
	voiceChatThrottle = 0;
	for ( i = 0; i < NUM_CHAT_HISTORY_LINES; i++ ) {
		chatHistory[ i ].line.Clear();
	}
	warmupText.Clear();
	voteValue.Clear();
	voteString.Clear();
	startFragLimit = -1;
}

/*
================
idMultiplayerGame::ClearGuis
	HUMANHEAD: rewritten
================
*/
void idMultiplayerGame::ClearGuis() {
	int i;
	
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		scoreBoard->SetStateString( va( "player%i",i+1 ), "" );
		scoreBoard->SetStateString( va( "player%i_score", i+1 ), "" );
		scoreBoard->SetStateString( va( "player%i_portrait",i+1 ), "" );
		scoreBoard->SetStateString( va( "player%i_wins", i+1 ), "" );
		scoreBoard->SetStateInt( va( "rank%i", i+1 ), 0 );
		scoreBoard->SetStateInt( "rank_self", 0 );

		scoreBoard->SetStateString( va( "red%i",i+1 ), "" );
		scoreBoard->SetStateString( va( "red%i_rank",i+1 ), "" );
		scoreBoard->SetStateString( va( "red%i_portrait", i+1 ), "" );
		scoreBoard->SetStateString( va( "red%i_score", i+1 ), "" );
		scoreBoard->SetStateString( va( "red%i_wins", i+1 ), "" );
		scoreBoard->SetStateString( va( "red%i_ping", i+1 ), "" );
		scoreBoard->SetStateString( va( "blue%i",i+1 ), "" );
		scoreBoard->SetStateString( va( "blue%i_rank",i+1 ), "" );
		scoreBoard->SetStateString( va( "blue%i_portrait", i+1 ), "" );
		scoreBoard->SetStateString( va( "blue%i_score", i+1 ), "" );
		scoreBoard->SetStateString( va( "blue%i_wins", i+1 ), "" );
		scoreBoard->SetStateString( va( "blue%i_ping", i+1 ), "" );

		idPlayer *player = static_cast<idPlayer *>( gameLocal.entities[ i ] );
		if ( !player || !player->hud ) {
			continue;
		}
		player->hud->SetStateString( va( "player%i",i+1 ), "" );
		player->hud->SetStateString( va( "player%i_portrait",i+1 ), "" );
		player->hud->SetStateString( va( "player%i_score", i+1 ), "" );
		player->hud->SetStateString( va( "player%i_ready", i+1 ), "" );
		scoreBoard->SetStateInt( va( "rank%i", i+1 ), 0 );
		player->hud->SetStateInt( "rank_self", 0 );
	}		

	scoreBoard->SetStateString("teamdm", "0");
}

/*
================
idMultiplayerGame::UpdatePlayerRanks
================
*/
void idMultiplayerGame::UpdatePlayerRanks() {
	int i, j, k;
	idPlayer *players[MAX_CLIENTS];
	idEntity *ent;
	idPlayer *player;

	memset( players, 0, sizeof( players ) );
	numRankedPlayers = 0;

	for ( i = 0; i < gameLocal.numClients; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( !ent || !ent->IsType( idPlayer::Type ) ) {
			continue;
		}
		player = static_cast< idPlayer * >( ent );
		if ( !CanPlay( player ) ) {
			continue;
		}
		for ( j = 0; j < numRankedPlayers; j++ ) {
			bool insert = false;
			if ( gameLocal.gameType == GAME_TDM ) {
				if ( player->team != players[ j ]->team ) {
					if ( playerState[ i ].teamFragCount > playerState[ players[ j ]->entityNumber ].teamFragCount ) {
						// team scores
						insert = true;
					} else if ( playerState[ i ].teamFragCount == playerState[ players[ j ]->entityNumber ].teamFragCount && player->team < players[ j ]->team ) {
						// at equal scores, sort by team number
						insert = true;
					}
				} else if ( playerState[ i ].fragCount > playerState[ players[ j ]->entityNumber ].fragCount ) {
					// in the same team, sort by frag count
					insert = true;
				}
			} else {
				insert = ( playerState[ i ].fragCount > playerState[ players[ j ]->entityNumber ].fragCount );
			}
			if ( insert ) {
				for ( k = numRankedPlayers; k > j; k-- ) {
					players[ k ] = players[ k-1 ];
				}
				players[ j ] = player;
				break;
			}
		}
		if ( j == numRankedPlayers ) {
			players[ numRankedPlayers ] = player;
		}
		numRankedPlayers++;
	}

	memcpy( rankedPlayers, players, sizeof( players ) );
}


/*
================
idMultiplayerGame::UpdateTeamScoreboard
	HUMANHEAD pdm
================
*/
void idMultiplayerGame::UpdateTeamScoreboard( idUserInterface *scoreBoard, idPlayer *player ) {
	int i, j, iline, k;
	idEntity *ent;
	idPlayer *p;
	int value;
	idStr playerName;
	idStr playerPortrait;
	idStr playerScore;		// Must be string field because it's used for string info sometimes
	idStr playerWins;		// Must be string field because it's used for string info sometimes
	int playerPing;
	bool playerVisible;
	int redScore = 0;
	int blueScore = 0;
	int team = 0;

	int playersBlue = 0;
	int playersRed = 0;

	scoreBoard->SetStateString("teamdm", "1");

	iline = 0; // the display lines

	// Ranked players first
	if ( gameState != WARMUP ) {
		for ( i = 0; i < numRankedPlayers; i++ ) {
			// ranked player
			iline++;

			// Update player stats
			team = ( idStr::Icmp( rankedPlayers[ i ]->GetUserInfo()->GetString( "ui_team" ), "Red" ) == 0 );
			char *teamStr;
			int n;
			if (team) {
				teamStr = "red";
				redScore = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[ rankedPlayers[ i ]->entityNumber ].teamFragCount );
				n = playersRed++;
			}
			else {
				teamStr = "blue";
				blueScore = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[ rankedPlayers[ i ]->entityNumber ].teamFragCount );
				n = playersBlue++;
			}

			if ( rankedPlayers[ i ]->entityNumber == player->entityNumber ) {
				// highlight who we are
				scoreBoard->SetStateInt( "rank_self", n+1 );
			}

			value = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[ rankedPlayers[ i ]->entityNumber ].fragCount );
			playerScore = va("%i", value);
			value = idMath::ClampInt( 0, MP_PLAYER_MAXWINS, playerState[ rankedPlayers[ i ]->entityNumber ].wins );
			playerWins = va("%i", value);
			playerPing = playerState[ rankedPlayers[ i ]->entityNumber ].ping;

			scoreBoard->SetStateBool( va( "%s%i_rank", teamStr, n+1 ), true );
			scoreBoard->SetStateString( va( "%s%i", teamStr, n+1 ), rankedPlayers[ i ]->GetUserInfo()->GetString( "ui_name" ) );
			scoreBoard->SetStateString( va( "%s%i_portrait", teamStr, n+1 ), rankedPlayers[ i ]->GetModelPortraitName(true) );
			scoreBoard->SetStateString( va( "%s%i_score", teamStr, n+1 ), playerScore.c_str() );
			scoreBoard->SetStateString( va( "%s%i_wins", teamStr, n+1 ), playerWins.c_str() );
			scoreBoard->SetStateInt( va( "%s%i_ping", teamStr, n+1 ), playerPing );
		}
	}

	// if warmup, this draws everyone, otherwise it goes over spectators only
	// when doing warmup we loop twice to draw ready/not ready first *then* spectators
	for ( k = 0; k < ( gameState == WARMUP ? 2 : 1 ); k++ ) {
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			ent = gameLocal.entities[ i ];
			if ( !ent || !ent->IsType( idPlayer::Type ) ) {
				continue;
			}
			if ( gameState != WARMUP ) {
				// check he's not covered by ranks already
				for ( j = 0; j < numRankedPlayers; j++ ) {
					if ( ent == rankedPlayers[ j ] ) {
						break;
					}
				}
				if ( j != numRankedPlayers ) {
					continue;
				}
			}
			p = static_cast< idPlayer * >( ent );
			if ( gameState == WARMUP ) {
				if ( k == 0 && p->spectating ) {
					continue;
				}
				if ( k == 1 && !p->spectating ) {
					continue;
				}
			}
	
			iline++;
			if ( !playerState[ i ].ingame ) {
				playerName = common->GetLanguageDict()->GetString( "#str_04244" );		// "New Player"
				playerPortrait = "";
				playerScore = common->GetLanguageDict()->GetString( "#str_04245" );		// "Connecting"
				playerVisible = true;
			} else {
				playerName = gameLocal.userInfo[ i ].GetString( "ui_name" );
				playerPortrait = p->GetModelPortraitName(true);
				if ( gameState == WARMUP ) {
					if ( p->spectating ) {
						playerScore = common->GetLanguageDict()->GetString( "#str_04246" );		// "Spectating"
						playerVisible = true;
					} else {
						if (p->IsReady()) {
							playerScore = common->GetLanguageDict()->GetString( "#str_04247" );	// "Ready"
						}
						else {
							playerScore = common->GetLanguageDict()->GetString( "#str_04248" );	// "Not Ready"
						}
						playerVisible = true;
					}
				} else {	// Not ranked but in game, must be spectator
					playerScore = common->GetLanguageDict()->GetString( "#str_04246" );			// "Spectating"
					playerVisible = true;
				}
			}

			playerWins = "";
			playerPing = playerState[ i ].ping;

			team = ( idStr::Icmp( gameLocal.userInfo[ ent->entityNumber ].GetString( "ui_team" ), "Red" ) == 0 );
			char *teamStr;
			int n;
			if (!team) {
				teamStr = "blue";
				n = playersBlue++;
			}
			else {
				teamStr = "red";
				n = playersRed++;
			}

			if ( i == player->entityNumber ) {
				// highlight who we are
				scoreBoard->SetStateInt( "rank_self", n+1 );
			}

			scoreBoard->SetStateBool( va( "%s%i_rank", teamStr, n+1 ), playerVisible );
			scoreBoard->SetStateString( va( "%s%i", teamStr, n+1 ), playerName.c_str() );
			scoreBoard->SetStateString( va( "%s%i_portrait", teamStr, n+1 ), playerPortrait.c_str() );
			scoreBoard->SetStateString( va( "%s%i_score", teamStr, n+1 ), playerScore.c_str() );
			scoreBoard->SetStateString( va( "%s%i_wins", teamStr, n+1 ), playerWins.c_str() );
			scoreBoard->SetStateInt( va( "%s%i_ping", teamStr, n+1 ), playerPing );
		}
	}

	// clear empty slots
	while (playersRed < MAX_CLIENTS) {
		scoreBoard->SetStateBool( va( "red%i_rank", playersRed+1 ), false );
		scoreBoard->SetStateString( va( "red%i", playersRed+1 ), "" );
		scoreBoard->SetStateString( va( "red%i_score", playersRed+1 ), "" );
		scoreBoard->SetStateString( va( "red%i_wins", playersRed+1 ), "" );
		scoreBoard->SetStateString( va( "red%i_ping", playersRed+1 ), "" );
		scoreBoard->SetStateString( va( "red%i_portrait", playersRed+1 ), "" );
		playersRed++;
	}
	while (playersBlue < MAX_CLIENTS) {
		//HUMANHEAD PCF rww 05/26/06 - was playersRed, changed to playersBlue
		scoreBoard->SetStateBool( va( "blue%i_rank", playersBlue+1 ), false );
		//HUMANHEAD END
		scoreBoard->SetStateString( va( "blue%i", playersBlue+1 ), "" );
		scoreBoard->SetStateString( va( "blue%i_score", playersBlue+1 ), "" );
		scoreBoard->SetStateString( va( "blue%i_wins", playersBlue+1 ), "" );
		scoreBoard->SetStateString( va( "blue%i_ping", playersBlue+1 ), "" );
		//HUMANHEAD PCF rww 05/26/06 - was playersRed, changed to playersBlue
		scoreBoard->SetStateString( va( "blue%i_portrait", playersBlue+1 ), "" );
		//HUMANHEAD END
		playersBlue++;
	}

	if (gameLocal.GetLocalPlayer()) {
		team = ( idStr::Icmp( gameLocal.GetLocalPlayer()->GetUserInfo()->GetString("ui_team"), "Red" ) == 0 );
		if (team) {
			scoreBoard->SetStateBool("local_team_blue", false);
			scoreBoard->SetStateBool("local_team_red", true);
		}
		else {
			scoreBoard->SetStateBool("local_team_blue", true);
			scoreBoard->SetStateBool("local_team_red", false);
		}
	}

	if (gameState == WARMUP || gameState == COUNTDOWN) {
		scoreBoard->SetStateString("blue_score", "");
		scoreBoard->SetStateString("red_score", "");
	}
	else {
		scoreBoard->SetStateString("red_score", va("%i", redScore));
		scoreBoard->SetStateString("blue_score", va("%i", blueScore));
	}
}

/*
================
idMultiplayerGame::UpdateDMScoreboard
	HUMANHEAD pdm
================
*/
void idMultiplayerGame::UpdateDMScoreboard( idUserInterface *scoreBoard, idPlayer *player ) {
	int i, j, iline, k;
	idEntity *ent;
	idPlayer *p;
	int value;

	scoreBoard->SetStateString("teamdm", "0");

	iline = 0; // the display lines
	if ( gameState != WARMUP ) {
		for ( i = 0; i < numRankedPlayers; i++ ) {
			// ranked player
			iline++;
			scoreBoard->SetStateString( va( "player%i", iline ), rankedPlayers[ i ]->GetUserInfo()->GetString( "ui_name" ) );

			scoreBoard->SetStateString( va( "player%i_portrait", iline ), rankedPlayers[ i ]->GetModelPortraitName(true) );
			value = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[ rankedPlayers[ i ]->entityNumber ].fragCount );
			scoreBoard->SetStateInt( va( "player%i_score", iline ), value );
			value = idMath::ClampInt( 0, MP_PLAYER_MAXWINS, playerState[ rankedPlayers[ i ]->entityNumber ].wins );
			scoreBoard->SetStateInt( va( "player%i_wins", iline ), value );
			scoreBoard->SetStateInt( va( "player%i_ping", iline ), playerState[ rankedPlayers[ i ]->entityNumber ].ping );

			// set the color band
			scoreBoard->SetStateInt( va( "rank%i", iline ), 1 );
			if ( rankedPlayers[ i ] == player ) {
				// highlight who we are
				scoreBoard->SetStateInt( "rank_self", iline );
			}
		}
	}

	// if warmup, this draws everyone, otherwise it goes over spectators only
	// when doing warmup we loop twice to draw ready/not ready first *then* spectators
	// NOTE: in tourney, shows spectators according to their playing rank order?
	for ( k = 0; k < ( gameState == WARMUP ? 2 : 1 ); k++ ) {
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			ent = gameLocal.entities[ i ];
			if ( !ent || !ent->IsType( idPlayer::Type ) ) {
				continue;
			}
			if ( gameState != WARMUP ) {
				// check he's not covered by ranks already
				for ( j = 0; j < numRankedPlayers; j++ ) {
					if ( ent == rankedPlayers[ j ] ) {
						break;
					}
				}
				if ( j != numRankedPlayers ) {
					continue;
				}
			}
			p = static_cast< idPlayer * >( ent );
			if ( gameState == WARMUP ) {
				if ( k == 0 && p->spectating ) {
					continue;
				}
				if ( k == 1 && !p->spectating ) {
					continue;
				}
			}
	
			iline++;
			if ( !playerState[ i ].ingame ) {
				scoreBoard->SetStateString( va( "player%i", iline ), common->GetLanguageDict()->GetString( "#str_04244" ) );
				scoreBoard->SetStateString( va( "player%i_portrait", iline ), "" ); //HUMANHEAD rww
				scoreBoard->SetStateString( va( "player%i_score", iline ), common->GetLanguageDict()->GetString( "#str_04245" ) );
				scoreBoard->SetStateInt( va( "rank%i", iline ), 0 );
			} else {
				scoreBoard->SetStateString( va( "player%i", iline ), gameLocal.userInfo[ i ].GetString( "ui_name" ) );
				scoreBoard->SetStateString( va( "player%i_portrait", iline ), p->GetModelPortraitName(true) ); //HUMANHEAD rww
				if ( gameState == WARMUP ) {
					if ( p->spectating ) {
						scoreBoard->SetStateString( va( "player%i_score", iline ), common->GetLanguageDict()->GetString( "#str_04246" ) );
						scoreBoard->SetStateInt( va( "rank%i", iline ), 0 );
					} else {
						scoreBoard->SetStateString( va( "player%i_score", iline ), p->IsReady() ? common->GetLanguageDict()->GetString( "#str_04247" ) : common->GetLanguageDict()->GetString( "#str_04248" ) );
						scoreBoard->SetStateInt( va( "rank%i", iline ), 1 );
					}
				} else {
					scoreBoard->SetStateString( va( "player%i_score", iline ), common->GetLanguageDict()->GetString( "#str_04246" ) );
					scoreBoard->SetStateInt( va( "rank%i", iline ), 0 );
				}
			}
			
			scoreBoard->SetStateString( va( "player%i_wins", iline ), "" );
			scoreBoard->SetStateInt( va( "player%i_ping", iline ), playerState[ i ].ping );
			if ( i == player->entityNumber ) {
				// highlight who we are
				scoreBoard->SetStateInt( "rank_self", iline );
			}
		}
	}

	// clear empty slots
	iline++;
	while ( iline <= MAX_SCOREBOARD_NAMES ) {
		scoreBoard->SetStateString( va( "player%i", iline ), "" );
		scoreBoard->SetStateString( va( "player%i_score", iline ), "" );
		scoreBoard->SetStateString( va( "player%i_portrait", iline ), "" );
		scoreBoard->SetStateString( va( "player%i_wins", iline ), "" );
		scoreBoard->SetStateString( va( "player%i_ping", iline ), "" );
		scoreBoard->SetStateInt( va( "rank%i", iline ), 0 );
		iline++;
	}
}

/*
================
idMultiplayerGame::UpdateScoreboard
	HUMANHEAD: rewritten
================
*/
void idMultiplayerGame::UpdateScoreboard( idUserInterface *scoreBoard, idPlayer *player ) {
	idStr gameinfo;
	idStr livesinfo;
	idStr timeinfo;

	if (gameLocal.gameType == GAME_TDM) {
		UpdateTeamScoreboard(scoreBoard, player);
	}
	else {
		UpdateDMScoreboard(scoreBoard, player);
	}

	// Update game info
	gameinfo = va( "%s: %s", common->GetLanguageDict()->GetString( "#str_02376" ), gameLocal.serverInfo.GetString( "si_gameType" ) );
	livesinfo = va( "%s: %i", common->GetLanguageDict()->GetString( "#str_01982" ), gameLocal.serverInfo.GetInt( "si_fragLimit" ) );

	if ( gameLocal.serverInfo.GetInt( "si_timeLimit" ) > 0 ) {
		timeinfo = va( "%s: %i", common->GetLanguageDict()->GetString( "#str_01983" ), gameLocal.serverInfo.GetInt( "si_timeLimit" ) );
	} else {
		timeinfo = va("%s", common->GetLanguageDict()->GetString( "#str_07209" ));
	}
	scoreBoard->SetStateString( "gameinfo", gameinfo );
	scoreBoard->SetStateString( "livesinfo", livesinfo );
	scoreBoard->SetStateString( "timeinfo", timeinfo );

	scoreBoard->Redraw( gameLocal.time );
}

/*
================
idMultiplayerGame::GameTime
================
*/
const char *idMultiplayerGame::GameTime() {
	static char buff[16];
	int m, s, t, ms;

	if ( gameState == COUNTDOWN ) {
		ms = warmupEndTime - gameLocal.realClientTime;
		s = ms / 1000 + 1;
		//HUMANHEAD rww
		/*
		if ( ms <= 0 ) {
			strcpy( buff, "WMP --" );
		} else {
			sprintf( buff, "WMP %i", s );
		}
		*/
		strcpy(buff, "");
		//HUMANHEAD END
	} else {
		int timeLimit = gameLocal.serverInfo.GetInt( "si_timeLimit" );
		if ( timeLimit ) {
			ms = ( timeLimit * 60000 ) - ( gameLocal.time - matchStartedTime );
		} else {
			ms = gameLocal.time - matchStartedTime;
		}
		if ( ms < 0 ) {
			ms = 0;
		}
	
		s = ms / 1000;
		m = s / 60;
		s -= m * 60;
		t = s / 10;
		s -= t * 10;

		sprintf( buff, "%i:%i%i", m, t, s );
	}
	return &buff[0];
}

/*
================
idMultiplayerGame::GameFrags
	HUMANHEAD pdm
	rww - changed to report frags remaining for player instead of frag limit
================
*/
const char *idMultiplayerGame::GameFrags(idPlayer *localPlayer) {
	static char buff[16];

	int fragLimit = gameLocal.serverInfo.GetInt( "si_fragLimit" );
	if (fragLimit) {
		if (localPlayer) {
			if (gameLocal.gameType == GAME_TDM) {
				fragLimit -= playerState[localPlayer->entityNumber].teamFragCount;
			}
			else {
				fragLimit -= playerState[localPlayer->entityNumber].fragCount;
			}
			if (fragLimit < 0) {
				fragLimit = 0;
			}
		}
		sprintf( buff, "%i", fragLimit );
	}
	else {
		sprintf( buff, "" );
	}

	return &buff[0];
}

/*
================
idMultiplayerGame::NumActualClients
================
*/
int idMultiplayerGame::NumActualClients( bool countSpectators, int *teamcounts ) {
	idPlayer *p;
	int c = 0;

	if ( teamcounts ) {
		teamcounts[ 0 ] = teamcounts[ 1 ] = 0;
	}
	for( int i = 0 ; i < gameLocal.numClients ; i++ ) {
		idEntity *ent = gameLocal.entities[ i ];
		if ( !ent || !ent->IsType( idPlayer::Type ) ) {
			continue;
		}
		p = static_cast< idPlayer * >( ent );
		if ( countSpectators || CanPlay( p ) ) {
			c++;
		}
		if ( teamcounts && CanPlay( p ) ) {
			teamcounts[ p->team ]++;
		}
	}
	return c;
}

/*
================
idMultiplayerGame::EnoughClientsToPlay
================
*/
bool idMultiplayerGame::EnoughClientsToPlay() {
	int team[ 2 ];
	int clients = NumActualClients( false, &team[ 0 ] );
	if ( gameLocal.gameType == GAME_TDM ) {
		return clients >= 2 && team[ 0 ] && team[ 1 ];
	} else {
		return clients >= 2;
	}
}

/*
================
idMultiplayerGame::AllPlayersReady
================
*/
bool idMultiplayerGame::AllPlayersReady() {
	int			i;
	idEntity	*ent;
	idPlayer	*p;
	int			team[ 2 ];

	if ( NumActualClients( false, &team[ 0 ] ) <= 1 ) {
		return false;
	}

	if ( gameLocal.gameType == GAME_TDM ) {
		if ( !team[ 0 ] || !team[ 1 ] ) {
			return false;
		}
	}

	if ( !gameLocal.serverInfo.GetBool( "si_warmup" ) ) {
		return true;
	}

	for( i = 0; i < gameLocal.numClients; i++ ) {
		if ( gameLocal.gameType == GAME_TOURNEY && i != currentTourneyPlayer[ 0 ] && i != currentTourneyPlayer[ 1 ] ) {
			continue;
		}
		ent = gameLocal.entities[ i ];
		if ( !ent || !ent->IsType( idPlayer::Type ) ) {
			continue;
		}
		p = static_cast< idPlayer * >( ent );
		if ( CanPlay( p ) && !p->IsReady() ) {
			return false;
		}
		team[ p->team ]++;
	}

	return true;
}

/*
================
idMultiplayerGame::FragLimitHit
return the winning player (team player)
if there is no FragLeader(), the game is tied and we return NULL
================
*/
idPlayer *idMultiplayerGame::FragLimitHit() {
	int i;
	int fragLimit = gameLocal.serverInfo.GetInt( "si_fragLimit" );
	idPlayer *leader;

	leader = FragLeader();
	if ( !leader ) {
		return NULL;
	}

	if ( fragLimit <= 0 ) {
		fragLimit = MP_PLAYER_MAXFRAGS;
	}

	if ( gameLocal.gameType == GAME_LASTMAN ) {
		// we have a leader, check if any other players have frags left
		assert( !static_cast< idPlayer * >( leader )->lastManOver );
		for( i = 0 ; i < gameLocal.numClients ; i++ ) {
			idEntity *ent = gameLocal.entities[ i ];
			if ( !ent || !ent->IsType( idPlayer::Type ) ) {
				continue;
			}
			if ( !CanPlay( static_cast< idPlayer * >( ent ) ) ) {
				continue;
			}
			if ( ent == leader ) {
				continue;
			}
			if ( playerState[ ent->entityNumber ].fragCount > 0 ) {
				return NULL;
			}
		}
		// there is a leader, his score may even be negative, but no one else has frags left or is !lastManOver
		return leader;
	} else if ( gameLocal.gameType == GAME_TDM ) {
		if ( playerState[ leader->entityNumber ].teamFragCount >= fragLimit ) {
			return leader;
		}
	} else {
		if ( playerState[ leader->entityNumber ].fragCount >= fragLimit ) {
			return leader;
		}
	}

	return NULL;
}

/*
================
idMultiplayerGame::TimeLimitHit
================
*/
bool idMultiplayerGame::TimeLimitHit() {	
	int timeLimit = gameLocal.serverInfo.GetInt( "si_timeLimit" );
	if ( timeLimit ) {
		if ( gameLocal.time >= matchStartedTime + timeLimit * 60000 ) {
			return true;
		}
	}
	return false;
}

/*
================
idMultiplayerGame::FragLeader
return the current winner ( or a player from the winning team )
NULL if even
================
*/
idPlayer *idMultiplayerGame::FragLeader( void ) {
	int i;
	int frags[ MAX_CLIENTS ];
	idPlayer *leader = NULL;
	idEntity *ent;
	idPlayer *p;
	int high = -9999;
	int count = 0;
	bool teamLead[ 2 ] = { false, false };

	for ( i = 0 ; i < gameLocal.numClients ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( !ent || !ent->IsType( idPlayer::Type ) ) {
			continue;
		}
		if ( !CanPlay( static_cast< idPlayer * >( ent ) ) ) {
			continue;
		}
		if ( gameLocal.gameType == GAME_TOURNEY && ent->entityNumber != currentTourneyPlayer[ 0 ] && ent->entityNumber != currentTourneyPlayer[ 1 ] ) {
			continue;
		}
		if ( static_cast< idPlayer * >( ent )->lastManOver ) {
			continue;
		}

		int fragc = ( gameLocal.gameType == GAME_TDM ) ? playerState[i].teamFragCount : playerState[i].fragCount;
		if ( fragc > high ) {
			high = fragc;
		}

		frags[ i ] = fragc;
	}

	for ( i = 0; i < gameLocal.numClients; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( !ent || !ent->IsType( idPlayer::Type ) ) {
			continue;
		}
		p = static_cast< idPlayer * >( ent );
		p->SetLeader( false );

		if ( !CanPlay( p ) ) {
			continue;
		}
		if ( gameLocal.gameType == GAME_TOURNEY && ent->entityNumber != currentTourneyPlayer[ 0 ] && ent->entityNumber != currentTourneyPlayer[ 1 ] ) {
			continue;
		}
		if ( p->lastManOver ) {
			continue;
		}
		if ( p->spectating ) {
			continue;
		}

		if ( frags[ i ] >= high ) {
			leader = p;
			count++;
			p->SetLeader( true );
			if ( gameLocal.gameType == GAME_TDM ) {
				teamLead[ p->team ] = true;
			}
		}
	}

	if ( gameLocal.gameType != GAME_TDM ) {
		// more than one player at the highest frags
		//HUMANHEAD rww - no more sudden death concept, just give all winning players score
		/*
		if ( count > 1 ) {
			return NULL;
		} else {
			return leader;
		}
		*/
		return leader;
		//HUMANHEAD END
	} else {
		//HUMANHEAD rww - no more sudden death concept, just give all winning players score
		/*
		if ( teamLead[ 0 ] && teamLead[ 1 ] ) {
			// even game in team play
			return NULL;
		}
		*/
		//HUMANHEAD END
		return leader;
	}
}

/*
================
idGameLocal::UpdateWinsLosses
================
*/
void idMultiplayerGame::UpdateWinsLosses( idPlayer *winner ) {
	if ( winner ) {
		// run back through and update win/loss count
		for( int i = 0; i < gameLocal.numClients; i++ ) {
			idEntity *ent = gameLocal.entities[ i ];
			if ( !ent || !ent->IsType( idPlayer::Type ) ) {
				continue;
			}
			idPlayer *player = static_cast<idPlayer *>(ent);
			if ( gameLocal.gameType == GAME_TDM ) {
				if ( bTeamsTied || player == winner || ( player != winner && player->team == winner->team ) ) { //HUMANHEAD rww - check for bTeamsTied
					playerState[ i ].wins++;
					PlayGlobalSound( player->entityNumber, SND_YOUWIN );
				} else {
					PlayGlobalSound( player->entityNumber, SND_YOULOSE );
				}
			} else if ( gameLocal.gameType == GAME_LASTMAN ) {
				if ( player == winner ) {
					playerState[ i ].wins++;
					PlayGlobalSound( player->entityNumber, SND_YOUWIN );
				} else if ( !player->wantSpectate ) {
					PlayGlobalSound( player->entityNumber, SND_YOULOSE );
				}
			} else if ( gameLocal.gameType == GAME_TOURNEY ) {
				if ( player == winner ) { 
					playerState[ i ].wins++;
					PlayGlobalSound( player->entityNumber, SND_YOUWIN );
				} else if ( i == currentTourneyPlayer[ 0 ] || i == currentTourneyPlayer[ 1 ] ) {
					PlayGlobalSound( player->entityNumber, SND_YOULOSE );
				}
			} else {
				if ( player == winner || playerState[player->entityNumber].fragCount == playerState[winner->entityNumber].fragCount ) { //HUMANHEAD rww - added or condition for ties
					playerState[i].wins++;
					PlayGlobalSound( player->entityNumber, SND_YOUWIN );
				} else if ( !player->wantSpectate ) {
					PlayGlobalSound( player->entityNumber, SND_YOULOSE );
				}
			}
		}
	}
	if ( winner ) {
		lastWinner = winner->entityNumber;
	} else {
		lastWinner = -1;
	}

	//HUMANHEAD rww
	persistentPlayerDict.Clear();
	for( int i = 0; i < gameLocal.numClients; i++ ) {
		idEntity *ent = gameLocal.entities[ i ];
		if ( !ent || !ent->IsType( idPlayer::Type ) ) {
			continue;
		}
		idPlayer *player = static_cast<idPlayer *>(ent);
		persistentPlayerDict.SetInt(va("%s_wins", gameLocal.userInfo[ent->entityNumber].GetString( "ui_name" )), playerState[player->entityNumber].wins);
	}
	//HUMANHEAD END
}

/*
================
idMultiplayerGame::TeamScore
================
*/
void idMultiplayerGame::TeamScore( int entityNumber, int team, int delta ) {
	playerState[ entityNumber ].fragCount += delta;
	for( int i = 0 ; i < gameLocal.numClients ; i++ ) {
		idEntity *ent = gameLocal.entities[ i ];
		if ( !ent || !ent->IsType( idPlayer::Type ) ) {
			continue;
		}
		idPlayer *player = static_cast<idPlayer *>(ent);
		if ( player->team == team ) {
			playerState[ player->entityNumber ].teamFragCount += delta;
		}
	}
}

/*
================
idMultiplayerGame::PlayerDeath
================
*/
void idMultiplayerGame::PlayerDeath( idPlayer *dead, idPlayer *killer, idEntity *inflictor, bool telefrag ) { //HUMANHEAD rww - pass inflictor
	//HUMANHEAD rww
	int oldScore = 0;
	//HUMANHEAD END

	// don't do PrintMessageEvent and shit
	assert( !gameLocal.isClient );

	//HUMANHEAD rww
	if (killer) {
		if (gameLocal.gameType == GAME_TDM) {
			oldScore = playerState[killer->entityNumber].teamFragCount;
		}
		else {
			oldScore = playerState[killer->entityNumber].fragCount;
		}
	}
	//HUMANHEAD END

	if ( killer ) {
		if ( gameLocal.gameType == GAME_LASTMAN ) {
			playerState[ dead->entityNumber ].fragCount--;
		} else if ( gameLocal.gameType == GAME_TDM ) {
			if ( killer == dead || killer->team == dead->team ) {
				// suicide or teamkill
				TeamScore( killer->entityNumber, killer->team, -1 );
			} else {
				TeamScore( killer->entityNumber, killer->team, +1 );
			}
		} else {
			playerState[ killer->entityNumber ].fragCount += ( killer == dead ) ? -1 : 1;
		}
	}

	//HUMANHEAD rww
	int inflictorDefNum = 0;
	if (inflictor && inflictor->IsType(hhProjectile::Type)) { //send the decl num for the projectile if applicable
		const char			*inflictorClass;

		inflictor->spawnArgs.GetString( "classname", NULL, &inflictorClass );
		const idDeclEntityDef *def = gameLocal.FindEntityDef( inflictorClass, false );
		if (def) {
			inflictorDefNum = def->Index();
		}
	}
	//HUMANHEAD END

	if ( killer && killer == dead ) {
		PrintDeathMessageEvent( -1, MSG_SUICIDE, dead->entityNumber, -1, inflictorDefNum ); //HUMANHEAD rww - seperate death message event function
	} else if ( killer ) {
		if ( telefrag ) {
			PrintDeathMessageEvent( -1, MSG_TELEFRAGGED, dead->entityNumber, killer->entityNumber, inflictorDefNum ); //HUMANHEAD rww - seperate death message event function
		} else if ( gameLocal.gameType == GAME_TDM && dead->team == killer->team ) {
			PrintDeathMessageEvent( -1, MSG_KILLEDTEAM, dead->entityNumber, killer->entityNumber, inflictorDefNum ); //HUMANHEAD rww - seperate death message event function
		} else {
			PrintDeathMessageEvent( -1, MSG_KILLED, dead->entityNumber, killer->entityNumber, inflictorDefNum ); //HUMANHEAD rww - seperate death message event function
		}
	} else {
		PrintDeathMessageEvent( -1, MSG_DIED, dead->entityNumber, -1, inflictorDefNum ); //HUMANHEAD rww - seperate death message event function
		playerState[ dead->entityNumber ].fragCount--;
	}

	//HUMANHEAD rww
	if (killer) {
		if (gameLocal.gameType == GAME_TDM) {
			if (oldScore != playerState[killer->entityNumber].teamFragCount) {
				CheckScoreChange(killer->entityNumber, oldScore, playerState[killer->entityNumber].teamFragCount, false);
			}
		}
		else {
			if (oldScore != playerState[killer->entityNumber].fragCount) {
				CheckScoreChange(killer->entityNumber, oldScore, playerState[killer->entityNumber].fragCount, false);
			}
		}
	}
	//HUMANHEAD END
}

/*
================
idMultiplayerGame::PlayerStats
================
*/
void idMultiplayerGame::PlayerStats( int clientNum, char *data, const int len ) {

	idEntity *ent;
	int team;

	*data = 0;

	// make sure we don't exceed the client list
	if ( clientNum < 0 || clientNum > gameLocal.numClients ) {
		return;
	}

	// find which team this player is on
	ent = gameLocal.entities[ clientNum ]; 
	if ( ent && ent->IsType( idPlayer::Type ) ) {
		team = static_cast< idPlayer * >(ent)->team;
	} else {
		return;
	}

	idStr::snPrintf( data, len, "team=%d score=%ld tks=%ld", team, playerState[ clientNum ].fragCount, playerState[ clientNum ].teamFragCount );

	return;

}

/*
================
idMultiplayerGame::PlayerVote
================
*/
void idMultiplayerGame::PlayerVote( int clientNum, playerVote_t vote ) {
	playerState[ clientNum ].vote = vote;
}

/*
================
idMultiplayerGame::DumpTourneyLine
================
*/
void idMultiplayerGame::DumpTourneyLine( void ) {
	int i;
	for ( i = 0; i < gameLocal.numClients; i++ ) {
		if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idPlayer::Type ) ) {
			common->Printf( "client %d: rank %d\n", i, static_cast< idPlayer * >( gameLocal.entities[ i ] )->tourneyRank );
		}
	}
}

//HUMANHEAD rww
/*
================
idMultiplayerGame::CheckTeamsTied
================
*/
bool idMultiplayerGame::CheckTeamsTied(void) {
	if ( gameLocal.gameType != GAME_TDM ) {
		return false;
	}

	int highestScore = 0;
	int highScoreTeam = 0;
	bool mutipleTeamsScoredHigh = false;
	bool highestScoreSet = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (gameLocal.entities[i] && gameLocal.entities[i]->IsType(hhPlayer::Type)) {
			hhPlayer *pl = static_cast<hhPlayer *>(gameLocal.entities[i]);
			if (!pl->wantSpectate) {
				if (!highestScoreSet || playerState[i].fragCount > highestScore) { //set the high scorer
					highestScore = playerState[i].fragCount;
					highScoreTeam = pl->team;
					mutipleTeamsScoredHigh = false;
					highestScoreSet = true;
				}
				else if (playerState[i].fragCount == highestScore && pl->team != highScoreTeam) { //someone not on the same team is tied
					mutipleTeamsScoredHigh = true;
				}
			}
		}
	}

	return mutipleTeamsScoredHigh;
}
//HUMANHEAD END

/*
================
idMultiplayerGame::NewState
================
*/
void idMultiplayerGame::NewState( gameState_t news, idPlayer *player ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];
	int			i;

	assert( news != gameState );
	assert( !gameLocal.isClient );
	gameLocal.DPrintf( "%s -> %s\n", GameStateStrings[ gameState ], GameStateStrings[ news ] );
	switch( news ) {
		case GAMEON: {
			gameLocal.LocalMapRestart();
			outMsg.Init( msgBuf, sizeof( msgBuf ) );
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_RESTART );
			outMsg.WriteBits( 0, 1 );
			networkSystem->ServerSendReliableMessage( -1, outMsg );

			PlayGlobalSound( -1, SND_FIGHT );
			matchStartedTime = gameLocal.time;
			fragLimitTimeout = 0;
			for( i = 0; i < gameLocal.numClients; i++ ) {
				idEntity *ent = gameLocal.entities[ i ];
				if ( !ent || !ent->IsType( idPlayer::Type ) ) {
					continue;
				}
				idPlayer *p = static_cast<idPlayer *>( ent );
				p->SetLeader( false ); // don't carry the flag from previous games
				if ( gameLocal.gameType == GAME_TOURNEY && currentTourneyPlayer[ 0 ] != i && currentTourneyPlayer[ 1 ] != i ) {
					p->ServerSpectate( true );
					p->tourneyRank++;
				} else {
					int fragLimit = gameLocal.serverInfo.GetInt( "si_fragLimit" );
					int startingCount = ( gameLocal.gameType == GAME_LASTMAN ) ? fragLimit : 0;
					playerState[ i ].fragCount = startingCount;
					playerState[ i ].teamFragCount = startingCount;
					if ( !static_cast<idPlayer *>(ent)->wantSpectate ) {
						static_cast<idPlayer *>(ent)->ServerSpectate( false );
						if ( gameLocal.gameType == GAME_TOURNEY ) {
							p->tourneyRank = 0;
						}
					}
				}
				if ( CanPlay( p ) ) {
					p->lastManPresent = true;
				} else {
					p->lastManPresent = false;
				}
			}
			cvarSystem->SetCVarString( "ui_ready", "Not Ready" );
			switchThrottle[ 1 ] = 0;	// passby the throttle
			startFragLimit = gameLocal.serverInfo.GetInt( "si_fragLimit" );
			break;
		}
		case GAMEREVIEW: {
			nextState = INACTIVE;	// used to abort a game. cancel out any upcoming state change
			// set all players not ready and spectating
			for( i = 0; i < gameLocal.numClients; i++ ) {
				idEntity *ent = gameLocal.entities[ i ];
				if ( !ent || !ent->IsType( idPlayer::Type ) ) {
					continue;
				}
				static_cast< idPlayer *>( ent )->forcedReady = false;
				static_cast<idPlayer *>(ent)->ServerSpectate( true );
			}
			//HUMANHEAD rww
			bTeamsTied = CheckTeamsTied();
			//HUMANHEAD END
			UpdateWinsLosses( player );
			break;
		}
		case SUDDENDEATH: {
			PrintMessageEvent( -1, MSG_SUDDENDEATH );
			PlayGlobalSound( -1, SND_SUDDENDEATH );
			break;
		}
		case COUNTDOWN: {
			idBitMsg	outMsg;
			byte		msgBuf[ 128 ];

			warmupEndTime = gameLocal.time + 1000*cvarSystem->GetCVarInteger( "g_countDown" );

			outMsg.Init( msgBuf, sizeof( msgBuf ) );
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_WARMUPTIME );
			outMsg.WriteLong( warmupEndTime );
			networkSystem->ServerSendReliableMessage( -1, outMsg );

			break;
		}
		default:
			break;
	}

	gameState = news;
}

/*
================
idMultiplayerGame::FillTourneySlots
NOTE: called each frame during warmup to keep the tourney slots filled
================
*/
void idMultiplayerGame::FillTourneySlots( ) {
	int i, j, rankmax, rankmaxindex;
	idEntity *ent;
	idPlayer *p;

	// fill up the slots based on tourney ranks
	for ( i = 0; i < 2; i++ ) {
		if ( currentTourneyPlayer[ i ] != -1 ) {
			continue;
		}
		rankmax = -1;
		rankmaxindex = -1;
		for ( j = 0; j < gameLocal.numClients; j++ ) {
			ent = gameLocal.entities[ j ];
			if ( !ent || !ent->IsType( idPlayer::Type ) ) {
				continue;
			}
			if ( currentTourneyPlayer[ 0 ] == j || currentTourneyPlayer[ 1 ] == j ) {
				continue;
			}
			p = static_cast< idPlayer * >( ent );
			if ( p->wantSpectate ) {
				continue;
			}
			if ( p->tourneyRank >= rankmax ) {
				// when ranks are equal, use time in game
				if ( p->tourneyRank == rankmax ) {
					assert( rankmaxindex >= 0 );
					if ( p->spawnedTime > static_cast< idPlayer * >( gameLocal.entities[ rankmaxindex ] )->spawnedTime ) {
						continue;
					}
				}
				rankmax = static_cast< idPlayer * >( ent )->tourneyRank;
				rankmaxindex = j;
			}
		}
		currentTourneyPlayer[ i ] = rankmaxindex; // may be -1 if we found nothing
	}
}

/*
================
idMultiplayerGame::UpdateTourneyLine
we manipulate tourneyRank on player entities for internal ranking. it's easier to deal with.
but we need a real wait list to be synced down to clients for GUI
ignore current players, ignore wantSpectate
================
*/
void idMultiplayerGame::UpdateTourneyLine( void ) {
	int i, j, imax, max, globalmax = -1;
	idPlayer *p;

	assert( !gameLocal.isClient );
	if ( gameLocal.gameType != GAME_TOURNEY ) {
		return;
	}

	for ( j = 1; j <= gameLocal.numClients; j++ ) {
		max = -1; imax = -1;
		for ( i = 0; i < gameLocal.numClients; i++ ) {
			if ( currentTourneyPlayer[ 0 ] == i || currentTourneyPlayer[ 1 ] == i ) {
				continue;
			}
			p = static_cast< idPlayer * >( gameLocal.entities[ i ] );
			if ( !p || p->wantSpectate ) {
				continue;
			}
			if ( p->tourneyRank > max && ( globalmax == -1 || p->tourneyRank < globalmax ) ) {
				imax = i;
				max = p->tourneyRank;
			}
		}
		if ( imax == -1 ) {
			break;
		}

		idBitMsg outMsg;
		byte msgBuf[1024];
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_TOURNEYLINE );
		outMsg.WriteByte( j );
		networkSystem->ServerSendReliableMessage( imax, outMsg );

		globalmax = max;
	}
}

/*
================
idMultiplayerGame::CycleTourneyPlayers
================
*/
void idMultiplayerGame::CycleTourneyPlayers( ) {
	int i;
	idEntity *ent;
	idPlayer *player;

	currentTourneyPlayer[ 0 ] = -1;
	currentTourneyPlayer[ 1 ] = -1;
	// if any, winner from last round will play again
	if ( lastWinner != -1 ) {
		idEntity *ent = gameLocal.entities[ lastWinner ];
		if ( ent && ent->IsType( idPlayer::Type ) ) {
			currentTourneyPlayer[ 0 ] = lastWinner;		
		}
	}
	FillTourneySlots( );
	// force selected players in/out of the game and update the ranks
	for ( i = 0 ; i < gameLocal.numClients ; i++ ) {
		if ( currentTourneyPlayer[ 0 ] == i || currentTourneyPlayer[ 1 ] == i ) {
			player = static_cast<idPlayer *>( gameLocal.entities[ i ] );
			player->ServerSpectate( false );
		} else {
			ent = gameLocal.entities[ i ];
			if ( ent && ent->IsType( idPlayer::Type ) ) {
				player = static_cast<idPlayer *>( gameLocal.entities[ i ] );
				player->ServerSpectate( true );
			}
		}
	}
	UpdateTourneyLine();
}

/*
================
idMultiplayerGame::ExecuteVote
the votes are checked for validity/relevance before they are started
we assume that they are still legit when reaching here
================
*/
void idMultiplayerGame::ExecuteVote( void ) {
	bool needRestart;
	switch ( vote ) {
		case VOTE_RESTART:
			gameLocal.MapRestart();
			break;
		case VOTE_TIMELIMIT:
			si_timeLimit.SetInteger( atoi( voteValue ) );
			needRestart = gameLocal.NeedRestart();
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" );
			if ( needRestart ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "nextMap" );
			}
			break;
		case VOTE_FRAGLIMIT:
			si_fragLimit.SetInteger( atoi( voteValue ) );
			needRestart = gameLocal.NeedRestart();
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" );
			if ( needRestart ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "nextMap" );
			}
			break;
		case VOTE_GAMETYPE:
			si_gameType.SetString( voteValue );
			gameLocal.MapRestart();
			break;
		case VOTE_KICK:
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "kick %s", voteValue.c_str() ) );
			break;
		case VOTE_MAP:
			si_map.SetString( voteValue );
			gameLocal.MapRestart();
			break;
		case VOTE_SPECTATORS:
			si_spectators.SetBool( !si_spectators.GetBool() );
			needRestart = gameLocal.NeedRestart();
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" );
			if ( needRestart ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "nextMap" );
			}
			break;
		case VOTE_NEXTMAP:
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "serverNextMap\n" );
			break;
	}
}

//HUMANHEAD rww - to prevent legacy bug found by qa:
/*
"STEPS
1. Start prey, and jump into any MP match, having a total of 4 people.
2. Have player 1 call a vote to kick players 2.
3. Have player 2 vote no, and player 3 vote yes.
4. Let the vote ride out, it will fail.
5. Do this again, but have player 1 disconnect from the server after the 2 votes are in.

RESULTS
Players 2 will be kicked, even though the same vote, with the same result, allowed him to stay moments before."
*/
/*
================
idMultiplayerGame::DisconnectVotingPlayer
================
*/
void idMultiplayerGame::DisconnectVotingPlayer(int clientNum) {
	if (vote == VOTE_NONE) { //no vote in progress, don't care
		return;
	}

	if (playerState[clientNum].vote == PLAYER_VOTE_NONE || playerState[clientNum].vote == PLAYER_VOTE_WAIT) { //player had no voted, don't care
		return;
	}

	if (playerState[clientNum].vote == PLAYER_VOTE_NO) {
		noVotes--;
		if (noVotes < 0) {
			assert(!"noVotes somehow less than 0 on DisconnectVotingPlayer");
			noVotes = 0;
		}
	}
	else if (playerState[clientNum].vote == PLAYER_VOTE_YES) {
		yesVotes--;
		if (yesVotes < 0) {
			assert(!"yesVotes somehow less than 0 on DisconnectVotingPlayer");
			yesVotes = 0;
		}
	}
}
//HUMANHEAD END

/*
================
idMultiplayerGame::CheckVote
================
*/
void idMultiplayerGame::CheckVote( void ) {
	int numVoters, i;

	if ( vote == VOTE_NONE ) {
		return;
	}

	if ( voteExecTime ) {
		if ( gameLocal.time > voteExecTime ) {
			voteExecTime = 0;
			ClientUpdateVote( VOTE_RESET, 0, 0 );
			ExecuteVote();
			vote = VOTE_NONE;
		}
		return;
	}

	// count voting players
	numVoters = 0;
	for ( i = 0; i < gameLocal.numClients; i++ ) {
		idEntity *ent = gameLocal.entities[ i ];
		if ( !ent || !ent->IsType( idPlayer::Type ) ) {
			continue;
		}
		if ( playerState[ i ].vote != PLAYER_VOTE_NONE ) {
			numVoters++;
		}
	}
	if ( !numVoters ) {
		// abort
		vote = VOTE_NONE;
		ClientUpdateVote( VOTE_ABORTED, yesVotes, noVotes );
		return;
	}
	if ( yesVotes / numVoters > 0.5f ) {
		ClientUpdateVote( VOTE_PASSED, yesVotes, noVotes );
		voteExecTime = gameLocal.time + 2000;
		return;
	}
	if ( gameLocal.time > voteTimeOut || noVotes / numVoters >= 0.5f ) {
		ClientUpdateVote( VOTE_FAILED, yesVotes, noVotes );
		vote = VOTE_NONE;
		return;
	}
}

/*
================
idMultiplayerGame::Warmup
================
*/
bool idMultiplayerGame::Warmup() {
	return ( gameState == WARMUP );
}

/*
================
idMultiplayerGame::Run
================
*/
void idMultiplayerGame::Run() {
	int i, timeLeft;
	idPlayer *player;
	int gameReviewPause;

	assert( gameLocal.isMultiplayer );
	assert( !gameLocal.isClient );

	pureReady = true;

	if ( gameState == INACTIVE ) {
		lastGameType = gameLocal.gameType;
		NewState( WARMUP );
	}

	CheckVote();

	CheckRespawns();

	if ( nextState != INACTIVE && gameLocal.time > nextStateSwitch ) {
		NewState( nextState );
		nextState = INACTIVE;
	}

	// don't update the ping every frame to save bandwidth
	if ( gameLocal.time > pingUpdateTime ) {
		for ( i = 0; i < gameLocal.numClients; i++ ) {
			playerState[i].ping = networkSystem->ServerGetClientPing( i );
		}
		pingUpdateTime = gameLocal.time + 1000;
	}

	warmupText = "";

	switch( gameState ) {
		case GAMEREVIEW: {
			if ( nextState == INACTIVE ) {
				gameReviewPause = cvarSystem->GetCVarInteger( "g_gameReviewPause" );
				nextState = NEXTGAME;
				nextStateSwitch = gameLocal.time + 1000 * gameReviewPause;
			}
			break;
		}
		case NEXTGAME: {
			if ( nextState == INACTIVE ) {
				// game rotation, new map, gametype etc.
				if ( gameLocal.NextMap() ) {
					cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "serverMapRestart\n" );
					return;
				}
				NewState( WARMUP );
				if ( gameLocal.gameType == GAME_TOURNEY ) {
					CycleTourneyPlayers();
				}
				// put everyone back in from endgame spectate
				for ( i = 0; i < gameLocal.numClients; i++ ) {
					idEntity *ent = gameLocal.entities[ i ];
					if ( ent && ent->IsType( idPlayer::Type ) ) {
						if ( !static_cast< idPlayer * >( ent )->wantSpectate ) {
							CheckRespawns( static_cast<idPlayer *>( ent ) );
						}
					}
				}
			}
			break;
		}
		case WARMUP: {
			if ( AllPlayersReady() ) {
				NewState( COUNTDOWN );
				nextState = GAMEON;
				nextStateSwitch = gameLocal.time + 1000 * cvarSystem->GetCVarInteger( "g_countDown" );
			}
			//HUMANHEAD rww - use proper lang string
			//warmupText = "Warming up.. waiting for players to get ready";
			warmupText = common->GetLanguageDict()->GetString( "#str_09001" );
			//HUMANHEAD END
			one = two = three = false;
			break;
		}
		case COUNTDOWN: {
			timeLeft = ( nextStateSwitch - gameLocal.time ) / 1000 + 1;
			if ( timeLeft == 3 && !three ) {
				PlayGlobalSound( -1, SND_THREE );
				three = true;
			} else if ( timeLeft == 2 && !two ) {
				PlayGlobalSound( -1, SND_TWO );
				two = true;
			} else if ( timeLeft == 1 && !one ) {
				PlayGlobalSound( -1, SND_ONE );
				one = true;
			}
			//HUMANHEAD rww - use proper lang string
			//warmupText = va( "Match starts in %i", timeLeft );
			warmupText = va( "%s%i", common->GetLanguageDict()->GetString( "#str_09000" ), timeLeft );
			//HUMANHEAD END
			break;
		}
		case GAMEON: {
			player = FragLimitHit();
			if ( player ) {
				// delay between detecting frag limit and ending game. let the death anims play
				//HUMANHEAD rww - no sudden death concept
				/*
				if ( !fragLimitTimeout ) {
					common->DPrintf( "enter FragLimit timeout, player %d is leader\n", player->entityNumber );
					fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
				}
				*/
				if ( gameLocal.time > fragLimitTimeout ) {
					NewState( GAMEREVIEW, player );
					PrintMessageEvent( -1, MSG_FRAGLIMIT, player->entityNumber );
				}
			} else {
				//HUMANHEAD rww - no sudden death concept
				/*
				if ( fragLimitTimeout ) {
					// frag limit was hit and cancelled. means the two teams got even during FRAGLIMIT_DELAY
					// enter sudden death, the next frag leader will win
					SuddenRespawn();
					PrintMessageEvent( -1, MSG_HOLYSHIT );
					fragLimitTimeout = 0;
					NewState( SUDDENDEATH );
				} else*/ if ( TimeLimitHit() ) {
					player = FragLeader();
					if ( !player ) {
						NewState( SUDDENDEATH );
					} else {
						NewState( GAMEREVIEW, player );
						PrintMessageEvent( -1, MSG_TIMELIMIT );
					}
				}
			}
			break;
		}
		case SUDDENDEATH: {
			player = FragLeader();
			if ( player ) {
				if ( !fragLimitTimeout ) {
					common->DPrintf( "enter sudden death FragLeader timeout, player %d is leader\n", player->entityNumber );
					fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
				}
				if ( gameLocal.time > fragLimitTimeout ) {
					NewState( GAMEREVIEW, player );
					PrintMessageEvent( -1, MSG_FRAGLIMIT, player->entityNumber );
				}
			} else if ( fragLimitTimeout ) {
				SuddenRespawn();
				PrintMessageEvent( -1, MSG_HOLYSHIT );
				fragLimitTimeout = 0;
			}
			break;
		}
	}
}

/*
================
idMultiplayerGame::UpdateMainGui
================
*/
void idMultiplayerGame::UpdateMainGui( void ) {
	int i;
	mainGui->SetStateInt( "readyon", gameState == WARMUP ? 1 : 0 );
//	mainGui->SetStateInt( "readyoff", gameState != WARMUP ? 1 : 0 );	//HUMANHEAD pdm
	idStr strReady = cvarSystem->GetCVarString( "ui_ready" );
	if ( strReady.Icmp( "ready") == 0 ){
		strReady = common->GetLanguageDict()->GetString( "#str_04248" );
	} else {
		strReady = common->GetLanguageDict()->GetString( "#str_04247" );
	}
	mainGui->SetStateString( "ui_ready", strReady );
	mainGui->SetStateInt( "teamon", gameLocal.gameType == GAME_TDM ? 1 : 0 );
//	mainGui->SetStateInt( "teamoff", gameLocal.gameType != GAME_TDM ? 1 : 0 );	//HUMANHEAD pdm
	if ( gameLocal.gameType == GAME_TDM ) {
		idPlayer *p = gameLocal.GetClientByNum( gameLocal.localClientNum );
		mainGui->SetStateInt( "team", p->team );
	}
	// setup vote
//	mainGui->SetStateInt( "voteon", ( vote != VOTE_NONE && !voted ) ? 1 : 0 );	//HUMANHEAD pdm
//	mainGui->SetStateInt( "voteoff", ( vote != VOTE_NONE && !voted ) ? 0 : 1 );	//HUMANHEAD pdm
	// last man hack
//	mainGui->SetStateInt( "isLastMan", gameLocal.gameType == GAME_LASTMAN ? 1 : 0 ); //HUMANHEAD pdm
	// send the current serverinfo values
	for ( i = 0; i < gameLocal.serverInfo.GetNumKeyVals(); i++ ) {
		const idKeyValue *keyval = gameLocal.serverInfo.GetKeyVal( i );
		mainGui->SetStateString( keyval->GetKey(), keyval->GetValue() );
	}

	// HUMANHEAD pdm
	mainGui->SetStateInt( "activevote", ( vote != VOTE_NONE ) ? 1 : 0 );
	mainGui->SetStateInt( "registeredvote", ( vote != VOTE_NONE && voted ) ? 1 : 0 );
	if ( vote != VOTE_NONE ) {
		float secondsLeft = MS2SEC(voteTimeOut - gameLocal.time);
		mainGui->SetStateInt( "votetime", (int)secondsLeft );
		const char *voteFormat = common->GetLanguageDict()->GetString( "#str_00627" );
		mainGui->SetStateString( "vote", va( voteFormat, voteString.c_str(), (int)yesVotes, (int)noVotes ) );
	} else {
		mainGui->SetStateString( "vote", "" );
		mainGui->SetStateInt( "votetime", 0 );
	}
	// HUMANHEAD END

	mainGui->StateChanged( gameLocal.time );
#if defined( __linux__ )
	// replacing the oh-so-useful s_reverse with sound backend prompt
	mainGui->SetStateString( "driver_prompt", "1" );
#else
	mainGui->SetStateString( "driver_prompt", "0" );
#endif
}

/*
================
idMultiplayerGame::StartMenu
================
*/
idUserInterface* idMultiplayerGame::StartMenu( void ) {

	if ( mainGui == NULL ) {
		return NULL;
	}

	int i, j;
	if ( currentMenu ) {
		currentMenu = 0;
		cvarSystem->SetCVarBool( "ui_chat", false );
	} else {
		if ( nextMenu >= 2 ) {
			currentMenu = nextMenu;
		} else {
			// for default and explicit
			currentMenu = 1;
		}
		cvarSystem->SetCVarBool( "ui_chat", true );
	}
	nextMenu = 0;
	gameLocal.sessionCommand = "";	// in case we used "game_startMenu" to trigger the menu
	if ( currentMenu == 1 ) {
		UpdateMainGui();

		// UpdateMainGui sets most things, but it doesn't set these because
		// it'd be pointless and/or harmful to set them every frame (for various reasons)
		// Currenty the gui doesn't update properly if they change anyway, so we'll leave it like this.

		// setup callvote
		if ( vote == VOTE_NONE ) {
			bool callvote_ok = false;
			for ( i = 0; i < VOTE_COUNT; i++ ) {
				// flag on means vote is denied, so default value 0 means all votes and -1 disables
				mainGui->SetStateInt( va( "vote%d", i ), g_voteFlags.GetInteger() & ( 1 << i ) ? 0 : 1 );
				if ( !( g_voteFlags.GetInteger() & ( 1 << i ) ) ) {
					callvote_ok = true;
				}
			}
			mainGui->SetStateInt( "callvote", callvote_ok );
		} else {
			mainGui->SetStateInt( "callvote", 2 );
		}

		// player kick data
		idStr kickList;
		j = 0;
		for ( i = 0; i < gameLocal.numClients; i++ ) {
			if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idPlayer::Type ) ) {
				if ( kickList.Length() ) {
					kickList += ";";
				}
				kickList += va( "\"%d - %s\"", i, gameLocal.userInfo[ i ].GetString( "ui_name" ) );
				kickVoteMap[ j ] = i;
				j++;
			}
		}
		mainGui->SetStateString( "kickChoices", kickList );

		mainGui->SetStateString( "chattext", "" );
		mainGui->Activate( true, gameLocal.time );
		return mainGui;
	} else if ( currentMenu == 2 ) {
		// the setup is done in MessageMode
		msgmodeGui->Activate( true, gameLocal.time );
		cvarSystem->SetCVarBool( "ui_chat", true );
		return msgmodeGui;
	}
	return NULL;
}

/*
================
idMultiplayerGame::DisableMenu
================
*/
void idMultiplayerGame::DisableMenu( void ) {
	gameLocal.sessionCommand = "";	// in case we used "game_startMenu" to trigger the menu
	if ( currentMenu == 1 ) {
		mainGui->Activate( false, gameLocal.time );
	} else if ( currentMenu == 2 ) {
		msgmodeGui->Activate( false, gameLocal.time );
	}
	currentMenu = 0;
	nextMenu = 0;
	cvarSystem->SetCVarBool( "ui_chat", false );
}

/*
================
idMultiplayerGame::SetMapShot
================
*/
void idMultiplayerGame::SetMapShot( void ) {
	char screenshot[ MAX_STRING_CHARS ];
	int mapNum = mapList->GetSelection( NULL, 0 );
	const idDict *dict = NULL;
	if ( mapNum >= 0 ) {
		dict = fileSystem->GetMapDecl( mapNum );
	}
	fileSystem->FindMapScreenshot( dict ? dict->GetString( "path" ) : "", screenshot, MAX_STRING_CHARS );
	mainGui->SetStateString( "current_levelshot", screenshot );
}

/*
================
idMultiplayerGame::HandleGuiCommands
================
*/
const char* idMultiplayerGame::HandleGuiCommands( const char *_menuCommand ) {
	idUserInterface	*currentGui;
	const char		*voteValue;
	int				vote_clientNum;
	int				icmd;
	idCmdArgs		args;

	if ( !_menuCommand[ 0 ] ) {
		common->Printf( "idMultiplayerGame::HandleGuiCommands: empty command\n" );
		return "continue";
	}
	assert( currentMenu );
	if ( currentMenu == 1 ) {
		currentGui = mainGui;
	} else {
		currentGui = msgmodeGui;
	}

	args.TokenizeString( _menuCommand, false );

	for( icmd = 0; icmd < args.Argc(); ) {
		const char *cmd = args.Argv( icmd++ );

		if ( !idStr::Icmp( cmd,	";"	) )	{
			continue;
		} else if (	!idStr::Icmp( cmd, "video" ) ) {
			idStr vcmd;
			if ( args.Argc() - icmd	>= 1 ) {
				vcmd = args.Argv( icmd++ );
			}

			int	oldSpec	= cvarSystem->GetCVarInteger( "com_imageQuality" );

			if ( idStr::Icmp( vcmd,	"low" )	== 0 ) {
				cvarSystem->SetCVarInteger(	"com_imageQuality", 0 );
			} else if (	idStr::Icmp( vcmd, "medium"	) == 0 ) {
				cvarSystem->SetCVarInteger(	"com_imageQuality", 1 );
			} else	if ( idStr::Icmp( vcmd,	"high" ) ==	0 )	{
				cvarSystem->SetCVarInteger(	"com_imageQuality", 2 );
			//} else	if ( idStr::Icmp( vcmd,	"ultra"	) == 0 ) {
			//	cvarSystem->SetCVarInteger(	"com_machineSpec", 3 );
			} else if (	idStr::Icmp( vcmd, "recommended" ) == 0	) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW,	"setMachineSpec\n" );
			}

			if ( oldSpec !=	cvarSystem->GetCVarInteger(	"com_imageQuality" ) ) {
				currentGui->SetStateInt( "com_imageQuality",	cvarSystem->GetCVarInteger(	"com_imageQuality" ) );
				currentGui->StateChanged( gameLocal.realClientTime );
				//cmdSystem->BufferCommandText( CMD_EXEC_NOW,	"execMachineSpec\n"	);
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "execImageQuality\n" );
			}

			if ( idStr::Icmp( vcmd,	"restart" )	 ==	0) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
			}

			continue;
		} else if (	!idStr::Icmp( cmd, "play" )	) {
			if ( args.Argc() - icmd	>= 1 ) {
				idStr snd =	args.Argv( icmd++ );
				int	channel	= 1;
				if ( snd.Length() == 1 ) {
					channel	= atoi(	snd	);
					snd	= args.Argv( icmd++	);
				}
				gameSoundWorld->PlayShaderDirectly(	snd, channel );
			}
			continue;
/* HUMANHEAD pdm: removed
		} else if (	!idStr::Icmp( cmd, "mpSkin"	) )	{
			idStr skin;
			if ( args.Argc() - icmd	>= 1 ) {
				skin = args.Argv( icmd++ );
				cvarSystem->SetCVarString( "ui_skin", skin );
			}
			SetMenuSkin();
			continue;
*/
		} else if (	!idStr::Icmp( cmd, "quit" )	) {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n"	);
			return NULL;
		} else if (	!idStr::Icmp( cmd, "disconnect"	) )	{
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "disconnect\n" );
			return NULL;
		} else if (	!idStr::Icmp( cmd, "close" ) ) {
			DisableMenu( );
			return NULL;
		} else if (	!idStr::Icmp( cmd, "spectate" )	) {
			ToggleSpectate();
			DisableMenu( );
			return NULL;
		} else if (	!idStr::Icmp( cmd, "chatmessage" ) ) {
			int	mode = currentGui->State().GetInt( "messagemode" );
			if ( mode )	{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW,	va(	"sayTeam \"%s\"", currentGui->State().GetString( "chattext"	) )	);
			} else {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW,	va(	"say \"%s\"", currentGui->State().GetString( "chattext"	) )	);
			}
			currentGui->SetStateString(	"chattext",	"" );
			if ( currentMenu ==	1 )	{
				return "continue";
			} else {
				DisableMenu();
				return NULL;
			}
		} else if (	!idStr::Icmp( cmd, "readytoggle" ) ) {
			ToggleReady( );
			DisableMenu( );
			return NULL;
		} else if (	!idStr::Icmp( cmd, "teamtoggle"	) )	{
			ToggleTeam(	);
			DisableMenu( );
			return NULL;
		} else if (	!idStr::Icmp( cmd, "callVote" )	) {
			vote_flags_t voteIndex = (vote_flags_t)mainGui->State().GetInt(	"voteIndex"	);
			if ( voteIndex == VOTE_MAP ) {
				int mapNum = mapList->GetSelection( NULL, 0 );
				if ( mapNum >= 0 ) {
					const idDict *dict = fileSystem->GetMapDecl( mapNum );
					if ( dict ) {
						ClientCallVote( VOTE_MAP, dict->GetString( "path" ) );
					}
				}
			} else {
				voteValue =	mainGui->State().GetString(	"str_voteValue"	);
				if ( voteIndex == VOTE_KICK	) {
					vote_clientNum = kickVoteMap[ atoi(	voteValue )	];
					ClientCallVote(	voteIndex, va( "%d", vote_clientNum	) );
				} else {
					ClientCallVote(	voteIndex, voteValue );
				}
			}
			DisableMenu();
			return NULL;
		} else if (	!idStr::Icmp( cmd, "voteyes" ) ) {
			CastVote( gameLocal.localClientNum,	true );
			DisableMenu();
			return NULL;
		} else if (	!idStr::Icmp( cmd, "voteno"	) )	{
			CastVote( gameLocal.localClientNum,	false );
			DisableMenu();
			return NULL;
		} else if ( !idStr::Icmp( cmd, "bind" ) ) {
			if ( args.Argc() - icmd >= 2 ) {
				idStr key = args.Argv( icmd++ );
				idStr bind = args.Argv( icmd++ );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "bindunbindtwo \"%s\" \"%s\"", key.c_str(), bind.c_str() ) );
				mainGui->SetKeyBindingNames();
			}
			continue;
		} else if ( !idStr::Icmp( cmd, "clearbind" ) ) {
			if ( args.Argc() - icmd >= 1 ) {
				idStr bind = args.Argv( icmd++ );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "unbind \"%s\"", bind.c_str() ) );
				mainGui->SetKeyBindingNames();
			}
			continue;

		// HUMANHEAD pdm: Model list
		} else if ( !idStr::Icmp( cmd, "ModelScan" ) ) {
			idStr name;
			idStr text;

			// skip precaching all player models
			bool oldPrecache = g_precache.GetBool();
			g_precache.SetBool(false);
			const idDecl *playerDef = declManager->FindType(DECL_ENTITYDEF, GAME_PLAYERDEFNAME_MP, false);
			g_precache.SetBool(oldPrecache);

			if (playerDef) {
				const idDict *playerDict = &static_cast<const idDeclEntityDef *>(playerDef)->dict;
				const idKeyValue *kv = playerDict->MatchPrefix("model_mp", NULL);
				while (kv != NULL) {
					idStr tmp = kv->GetKey();
					tmp.StripLeading("model_mp");
					int index = atoi(tmp.c_str());

					text = playerDict->GetString(va("mtr_modelPortrait%d", index), "guis/assets/menu/questionmark");
					mainGui->SetStateString(va("mp_modelportrait%d", index), text);

					text = playerDict->GetString(va("text_modelname%d", index));
					text = common->GetLanguageDict()->GetString(text.c_str());
					mainGui->SetStateString(va("mp_modelname%d", index), text);

					kv = playerDict->MatchPrefix("model_mp", kv);
				}

				// set current selection
				int currentModel = cvarSystem->GetCVarInteger("ui_modelNum");
				mainGui->SetStateString("mp_currentmodelportrait", mainGui->GetStateString(va("mp_modelportrait%d", currentModel)));
				mainGui->SetStateString("mp_currentmodelname", mainGui->GetStateString(va("mp_modelname%d", currentModel)));
			}
			continue;
		} else if ( !idStr::Icmp( cmd, "click_modelList" ) ) {
			if ( icmd < args.Argc() ) {
				int modelNum = atoi( args.Argv( icmd++ ) );
				idStr modelPortrait = mainGui->GetStateString(va("mp_modelportrait%d", modelNum));
				if (modelPortrait.Length()) {
					cvarSystem->SetCVarInteger("ui_modelNum", modelNum);
					mainGui->SetStateString("mp_currentmodelportrait", modelPortrait.c_str());
					mainGui->SetStateString("mp_currentmodelname", mainGui->GetStateString(va("mp_modelname%d", modelNum)));
				}
			}
			continue;
		// HUMANHEAD END

		} else if (	!idStr::Icmp( cmd, "MAPScan" ) ) {
			const char *gametype = gameLocal.serverInfo.GetString( "si_gameType" );
			if ( gametype == NULL || *gametype == 0 || idStr::Icmp( gametype, "singleplayer" ) == 0 ) {
				gametype = "Deathmatch";
			}

			int i, num;
			idStr si_map = gameLocal.serverInfo.GetString("si_map");
			const idDict *dict;

			//HUMANHEAD PCF rww 05/09/06 - avoid caching map dictionary media when we run through the map defs
			assert(cvarSystem);
			bool oldPrecache = cvarSystem->GetCVarBool("com_precache");
			cvarSystem->SetCVarBool("com_precache", false);
			//HUMANHEAD END

			mapList->Clear();
			mapList->SetSelection( -1 );
			num = fileSystem->GetNumMaps();
			for ( i = 0; i < num; i++ ) {
				dict = fileSystem->GetMapDecl( i );
				if ( dict ) {
					// any MP gametype supported
					bool isMP = false;
					int igt = GAME_SP + 1;
					while ( si_gameTypeArgs[ igt ] ) {
						if ( dict->GetBool( si_gameTypeArgs[ igt ] ) ) {
							isMP = true;
							break;
						}
						igt++;
					}
					if ( isMP ) {
						//HUMANHEAD PCF rww 05/27/06 - demo map check
						#ifdef ID_DEMO_BUILD
						if (dict->GetBool("indemo")) {
						#endif
						//HUMANHEAD END
						const char *mapName = dict->GetString( "name" );
						if ( mapName[0] == '\0' ) {
							mapName = dict->GetString( "path" );
						}
						mapName = common->GetLanguageDict()->GetString( mapName );
						mapList->Add( i, mapName );
						if ( !si_map.Icmp( dict->GetString( "path" ) ) ) {
							mapList->SetSelection( mapList->Num() - 1 );
						}
						//HUMANHEAD PCF rww 05/27/06 - demo map check
						#ifdef ID_DEMO_BUILD
						}
						#endif
						//HUMANHEAD END
					}
				}
			}
			//HUMANHEAD PCF rww 05/09/06 - avoid caching map dictionary media when we run through the map defs
			cvarSystem->SetCVarBool("com_precache", oldPrecache);
			//HUMANHEAD END

			// set the current level shot
			SetMapShot(	);
			return "continue";
		} else if (	!idStr::Icmp( cmd, "click_maplist" ) ) {
			SetMapShot(	);
			return "continue";
		} else if ( strstr( cmd, "sound" ) == cmd ) {
			// pass that back to the core, will know what to do with it
			return _menuCommand;
		}
		// HUMANHEAD pdm
		else if ( !idStr::Icmp( cmd, "main" ) ) {
			DisableMenu();
			return _menuCommand;
		}
		// HUMANHEAD END
		common->Printf(	"idMultiplayerGame::HandleGuiCommands: '%s'	unknown\n",	cmd	);

	}
	return "continue";
}

/*
================
idMultiplayerGame::Draw
================
*/
bool idMultiplayerGame::Draw( int clientNum ) {
	hhPlayer *player, *viewPlayer; //HUMANHEAD rww - changed to hhPlayer

	// clear the render entities for any players that don't need
	// icons and which might not be thinking because they weren't in
	// the last snapshot.
	for ( int i = 0; i < gameLocal.numClients; i++ ) {
		player = static_cast<hhPlayer *>( gameLocal.entities[ i ] );
		if ( player && !player->NeedsIcon() ) {
			player->HidePlayerIcons();
		}
	}

	player = viewPlayer = static_cast<hhPlayer *>( gameLocal.entities[ clientNum ] );

	if ( player == NULL ) {
		//HUMANHEAD rww - waiting on the server if we don't exist in the snapshot yet.
#ifdef _GUITEST_SERVERWAIT
		if (serverWaitGui) {
			serverWaitGui->Redraw(gameLocal.time);
		}
		//HUMANHEAD END
		return true;
#else
		return false;
#endif
	}

	if ( player->spectating ) {
		viewPlayer = static_cast<hhPlayer *>( gameLocal.entities[ player->spectator ] ); //HUMANHEAD rww - changed to hhPlayer
		if ( viewPlayer == NULL ) {
			return false;
		}
	}

	UpdatePlayerRanks();
	//HUMANHEAD rww
	if (viewPlayer->InVehicle()) {
		UpdateHud( viewPlayer, viewPlayer->GetVehicleInterfaceLocal()->GetHUD() );
		// use the hud of the local player
		viewPlayer->playerView.RenderPlayerView( viewPlayer->GetVehicleInterfaceLocal()->GetHUD() );
	}
	else {
	//HUMANHEAD END
		UpdateHud( viewPlayer, player->hud );
		// use the hud of the local player
		viewPlayer->playerView.RenderPlayerView( player->hud );
	}

	if ( currentMenu ) {
#if 0
		// uncomment this if you want to track when players are in a menu
		if ( !bCurrentMenuMsg ) {
			idBitMsg	outMsg;
			byte		msgBuf[ 128 ];

			outMsg.Init( msgBuf, sizeof( msgBuf ) );
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_MENU );
			outMsg.WriteBits( 1, 1 );
			networkSystem->ClientSendReliableMessage( outMsg );

			bCurrentMenuMsg = true;
		}
#endif
		if ( player->wantSpectate ) {
			mainGui->SetStateString( "spectext", common->GetLanguageDict()->GetString( "#str_04249" ) );
		} else {
			mainGui->SetStateString( "spectext", common->GetLanguageDict()->GetString( "#str_04250" ) );
		}
		DrawChat();
		if ( currentMenu == 1 ) {
			UpdateMainGui();
			mainGui->Redraw( gameLocal.time );
		} else {
			msgmodeGui->Redraw( gameLocal.time );
		}
	} else {
#if 0
		// uncomment this if you want to track when players are in a menu
		if ( bCurrentMenuMsg ) {
			idBitMsg	outMsg;
			byte		msgBuf[ 128 ];

			outMsg.Init( msgBuf, sizeof( msgBuf ) );
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_MENU );
			outMsg.WriteBits( 0, 1 );
			networkSystem->ClientSendReliableMessage( outMsg );

			bCurrentMenuMsg = false;
		}
#endif
		if ( player->spectating ) {
			idStr spectatetext[ 2 ];
			int ispecline = 0;
			if ( gameLocal.gameType == GAME_TOURNEY ) {
				if ( !player->wantSpectate ) {
					spectatetext[ 0 ] = common->GetLanguageDict()->GetString( "#str_04246" );
					switch ( player->tourneyLine ) {
						case 0:
							spectatetext[ 0 ] += common->GetLanguageDict()->GetString( "#str_07003" );
							break;
						case 1:
							spectatetext[ 0 ] += common->GetLanguageDict()->GetString( "#str_07004" );
							break;
						case 2:
							spectatetext[ 0 ] += common->GetLanguageDict()->GetString( "#str_07005" );
							break;
						default:
							spectatetext[ 0 ] += va( common->GetLanguageDict()->GetString( "#str_07006" ), player->tourneyLine );
							break;
					}
					ispecline++;
				}
			}
//HUMANHEAD pdm: removed lastman
//			else if ( gameLocal.gameType == GAME_LASTMAN ) {
//				if ( !player->wantSpectate ) {
//					spectatetext[ 0 ] = common->GetLanguageDict()->GetString( "#str_07007" );
//					ispecline++;
//				}
//			}
			if ( player->spectator != player->entityNumber ) {
				spectatetext[ ispecline ] = va( common->GetLanguageDict()->GetString( "#str_07008" ), viewPlayer->GetUserInfo()->GetString( "ui_name" ) );
			} else if ( !ispecline ) {
				spectatetext[ 0 ] = common->GetLanguageDict()->GetString( "#str_04246" );
			}
			spectateGui->SetStateString( "spectatetext0", spectatetext[0].c_str() );
			spectateGui->SetStateString( "spectatetext1", spectatetext[1].c_str() );
			if ( vote != VOTE_NONE ) {
				spectateGui->SetStateString( "vote", va( common->GetLanguageDict()->GetString( "#str_00627" ), voteString.c_str(), (int)yesVotes, (int)noVotes ) );
			} else {
				spectateGui->SetStateString( "vote", "" );
			}
			spectateGui->Redraw( gameLocal.time );
		}
		DrawChat();
		DrawScoreBoard( player );
	}

#if INGAME_DEBUGGER_ENABLED //HUMANHEAD rww - make debugger work in mp too
	debugger.UpdateDebugger();
#endif

	return true;
}

int idMultiplayerGame::GetTeamScore(int team) {
	int count = 0;

	for (int i=0; i<numRankedPlayers; i++) {
		int playerTeam = ( idStr::Icmp( rankedPlayers[ i ]->GetUserInfo()->GetString( "ui_team" ), "Red" ) == 0 );
		if (playerTeam == team) {
			count = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[ rankedPlayers[ i ]->entityNumber ].teamFragCount );
			break;
		}
	}

	return count;
}

/*
================
idMultiplayerGame::UpdateHud
	HUMANHEAD: rewritten
================
*/
void idMultiplayerGame::UpdateHud( idPlayer *player, idUserInterface *hud ) {
	int i;

	if ( !hud ) {
		return;
	}

	// HUMANHEAD pdm
	hud->SetStateBool("ready_self", player->IsReady());
	idVec4 teamColor = player->GetTeamColor();
	hud->SetStateFloat( "team_R", teamColor.x );
	hud->SetStateFloat( "team_G", teamColor.y );
	hud->SetStateFloat( "team_B", teamColor.z );
	hud->SetStateFloat( "team_A", gameLocal.gameType == GAME_TDM ? 1.0f : 0.0f );
	hud->SetStateString( "playername", player->GetUserInfo()->GetString("ui_name") );
	hud->SetStateBool( "ismultiplayer", true );
	// HUMANHEAD END

	hud->SetStateBool( "warmup", Warmup() );
	hud->SetStateBool( "gameon", !(Warmup() || gameState == COUNTDOWN || gameState == GAMEREVIEW));
	hud->SetStateBool( "teamdm", gameLocal.gameType == GAME_TDM );

	hud->SetStateBool( "readytipvisible", false );
	if ( gameState == WARMUP ) {
		hud->SetStateString( "gamestateText", common->GetLanguageDict()->GetString( "#str_09001" ) );

		// HUMANHEAD PCF pdm 05-06-06: added check for g_tips
		// HUMANHEAD pdm: display tip for ready key
		if (!player->IsReady() && g_tips.GetBool()) {
			// Determine key material to display
			char keyMaterial[256];	// No passing idStr between game and engine
			char key[256];			// No passing idStr between game and engine
			const char *material = NULL;
			keyMaterial[0] = '\0';
			key[0] = '\0';
			bool keywide;
			common->MaterialKeyForBinding("_impulse17", keyMaterial, key, keywide);
			material = keyMaterial;
			if (material) {
				hud->SetStateBool( "readytipvisible", true );
				hud->SetStateBool( "readykeywide", keywide );
				hud->SetStateString( "readykey", key ? key : "" );
				hud->SetStateString( "readykeyMaterial", keyMaterial ? keyMaterial : "" );
			}
		}
		// HUMANHEAD END
	}
	else if (gameState == COUNTDOWN) {
		int timeLeft = ( nextStateSwitch - gameLocal.time ) / 1000 + 1;
		hud->SetStateString( "gamestateText", va( "%s%i", common->GetLanguageDict()->GetString( "#str_09000" ), timeLeft ) );
	}
	else {
		hud->SetStateString( "gamestateText", "" );
	}
	//HUMANHEAD END

	// HUMANHEAD pdm: removed sudden death clause, and added frags, fraglimit, timelimit
	hud->SetStateBool( "fraglimit", gameLocal.serverInfo.GetInt("si_fragLimit") != 0 );
	hud->SetStateString( "frags", (Warmup() || gameState == COUNTDOWN) ? "" : GameFrags(player) );
	hud->SetStateBool( "timelimit", gameLocal.serverInfo.GetInt("si_timeLimit") != 0 );
	hud->SetStateString( "timer", (Warmup() || gameState == COUNTDOWN) ? "" : GameTime() );
	if ( vote != VOTE_NONE ) {
		const char *voteFormat = common->GetLanguageDict()->GetString( "#str_00627" );
		hud->SetStateString( "vote", va( voteFormat, voteString.c_str(), (int)yesVotes, (int)noVotes ) );
	} else {
		hud->SetStateString( "vote", "" );
	}

	// HUMANHEAD PCF pdm 05-06-06: Display vote key tips
	if ( vote != VOTE_NONE  && !voted ) {
		if (g_tips.GetBool() && !hud->GetStateBool("voteTipIsUp")) {
			gameLocal.SetTip(hud, "_impulse28", "#str_20254", NULL, NULL, "votetip1");	// Vote Yes
			gameLocal.SetTip(hud, "_impulse29", "#str_20255", NULL, NULL, "votetip2");	// Vote No
			hud->HandleNamedEvent( "voteTipWindowUp" );
			hud->SetStateBool("voteTipIsUp", true);
		}
	}
	else {
		if (hud->GetStateBool("voteTipIsUp")) {
			hud->HandleNamedEvent( "voteTipWindowDown" );
			hud->SetStateBool("voteTipIsUp", false);
		}
	}
	// HUMANHEAD END

	hud->SetStateInt( "rank_self", 0 );
	//HUMANHEAD rww - reworked logic here a little (was all terrible and hardcoded around 4 players)
	i = 0;
	if ( gameState == GAMEON ) {

		if ( gameLocal.gameType == GAME_TDM ) {
			int rank1_team, rank2_team;
			int rank1_score, rank2_score;
			int blueScore = GetTeamScore(0);
			int redScore = GetTeamScore(1);
			if (blueScore > redScore) {
				rank1_team = 0;
				rank1_score = blueScore;
				rank2_team = 1;
				rank2_score = redScore;
			}
			else {
				rank1_team = 1;
				rank1_score = redScore;
				rank2_team = 0;
				rank2_score = blueScore;
			}
			const char *blueStr = common->GetLanguageDict()->GetString( "#str_02500" );
			const char *redStr = common->GetLanguageDict()->GetString( "#str_02499" );
			hud->SetStateString("player1", rank1_team ? redStr : blueStr );	// HUMANHEAD PCF pdm 05/29/06: Made use of localized strings
			hud->SetStateInt("player1_team", rank1_team );
			hud->SetStateInt("player1_score", rank1_score );
			hud->SetStateString("player2", rank2_team ? redStr : blueStr );	// HUMANHEAD PCF pdm 05/29/06: Made use of localized strings
			hud->SetStateInt("player2_team", rank2_team );
			hud->SetStateInt("player2_score", rank2_score );
			i = 2;	// Zero out the rest
		}
		else {
			while (i < numRankedPlayers) {

				hud->SetStateString( va( "player%i", i+1 ), rankedPlayers[ i ]->GetUserInfo()->GetString( "ui_name" ) );
				hud->SetStateInt( va( "player%i_score", i+1 ), playerState[ rankedPlayers[ i ]->entityNumber ].fragCount );
				hud->SetStateString( va( "player%i_portrait", i+1 ), rankedPlayers[ i ]->GetModelPortraitName(true) );
				hud->SetStateInt( va( "rank%i", i+1 ), 1 );
				if ( rankedPlayers[ i ] == player ) {
					hud->SetStateInt( "rank_self", i+1 );
				}
				i++;
			}
		}
	}
	while (i < MAX_SCOREBOARD_NAMES) { //go through the rest of the list and zero out the values
		hud->SetStateString( va( "player%i", i+1 ), "" );
		hud->SetStateString( va( "player%i_portrait", i+1 ), "" ); //HUMANHEAD rww
		hud->SetStateString( va( "player%i_score", i+1 ), "" );
		hud->SetStateInt( va( "rank%i", i+1 ), 0 );
		i++;
	}

	//HUMANHEAD rww
	if (player && player->entityNumber == gameLocal.localClientNum) {
		char *selfRankStr;
		if (gameState == GAMEON) {
			int myPlaceRank = -1;
			for (i = 0; i < numRankedPlayers; i++) {
				if (rankedPlayers[i]->entityNumber == gameLocal.localClientNum) {
					myPlaceRank = i+1;
					break;
				}
			}
			if (myPlaceRank == -1) {
				selfRankStr = "";
			}
			else {
				selfRankStr = va("%i/%i", myPlaceRank, numRankedPlayers);
			}
		}
		else {
			selfRankStr = "";
		}
		hud->SetStateString("self_rankstring", selfRankStr);
	}
	//HUMANHEAD END
}

/*
================
idMultiplayerGame::DrawScoreBoard
================
*/
void idMultiplayerGame::DrawScoreBoard( idPlayer *player ) {
	if ( player->scoreBoardOpen || gameState == GAMEREVIEW ) {
		if ( !playerState[ player->entityNumber ].scoreBoardUp ) {
			scoreBoard->Activate( true, gameLocal.time );
			playerState[ player->entityNumber ].scoreBoardUp = true;
		}
		UpdateScoreboard( scoreBoard, player );
	} else {
		if ( playerState[ player->entityNumber ].scoreBoardUp ) {
			scoreBoard->Activate( false, gameLocal.time );
			playerState[ player->entityNumber ].scoreBoardUp = false;
		}
	}
}

/*
===============
idMultiplayerGame::ClearChatData
===============
*/
void idMultiplayerGame::ClearChatData() {
//	HUMANHEAD pdm: Reworked for our chat system
	chatHistoryIndex	= 0;
//	HUMANHEAD END
}

/*
===============
idMultiplayerGame::AddChatLine
===============
*/
void idMultiplayerGame::AddChatLine( const char *fmt, ... ) {
	idStr temp;
	va_list argptr;
	
	va_start( argptr, fmt );
	vsprintf( temp, fmt, argptr );
	va_end( argptr );
	
	gameLocal.Printf( "%s\n", temp.c_str() );

	if (!chatHistoryList) { //HUMANHEAD rww - can be null if session has shut down
		return;
	}

//	HUMANHEAD pdm: Reworked for our chat system
	int safeIndex = chatHistoryIndex % NUM_CHAT_HISTORY_LINES;
	chatHistory[ safeIndex ].line = temp;
	chatHistory[ safeIndex ].startTime = gameLocal.time;

	if (chatHistoryList->Num() >= MAX_HISTORY_LIST) {					// Don't let the list get out of control
		chatHistoryList->Clear();
	}
	chatHistoryList->Add(chatHistoryIndex % MAX_HISTORY_LIST, temp);	// Wrap around after MAX_HISTORY_LIST entries
	chatHistoryList->SetSelection(chatHistoryIndex % MAX_HISTORY_LIST);
	if (mainGui) {
		mainGui->HandleNamedEvent("viewBottom");
	}

	chatHistoryIndex++;
//	HUMANHEAD END
}

/*
===============
idMultiplayerGame::DrawChat
	HUMANHEAD pdm: Reworked for our chat system
===============
*/
void idMultiplayerGame::DrawChat() {
//	HUMANHEAD pdm: Reworked for our chat system
	if ( guiChat ) {
		// draw any text that was started within CHAT_MAX_LIFE_TIME from currenttime
		for (int ix=0; ix<NUM_CHAT_DISPLAY_LINES; ix++) {
			int index = (chatHistoryIndex - 1) - ix;
			int safeIndex = index % NUM_CHAT_HISTORY_LINES;

			if ((index < 0) || (gameLocal.time - chatHistory[ safeIndex ].startTime > CHAT_MAX_LIFE_TIME)) {
				guiChat->SetStateString( va( "chat%i", ix ), "" );
			}
			else {
				guiChat->SetStateString( va( "chat%i", ix ), chatHistory[ safeIndex ].line );
			}
		}

		guiChat->Redraw( gameLocal.time );
	}
//	HUMANHEAD END
}

//HUMANHEAD rww
/*
================
idMultiplayerGame::ClearFrags
rww - moved from being inline
================
*/
void idMultiplayerGame::ClearFrags( int clientNum ) {
	if (gameLocal.gameType == GAME_TDM) { //HUMANHEAD rww
		CheckScoreChange(clientNum, playerState[clientNum].teamFragCount, 0, true);
	}
	else {
		CheckScoreChange(clientNum, playerState[clientNum].fragCount, 0, true);
	} //HUMANHEAD END
	playerState[ clientNum ].fragCount = 0;
}

/*
================
idMultiplayerGame::CheckScoreChange
================
*/
void idMultiplayerGame::CheckScoreChange(int clientNum, int oldScore, int newScore, bool leavingGame) {
	int highestScore = 0;
	bool highestScoreSet = false;
	int i;
	int numWithHighscore = 0;
	int numInGame = 0;

	if (gameLocal.isClient) { //only do this on the server
		return;
	}

	if (gameState == WARMUP) { //no need to announce score changes during warmup
		return;
	}

	if (oldScore == newScore && !leavingGame) {
		return;
	}

	if (!gameLocal.entities[clientNum] || !gameLocal.entities[clientNum]->IsType(hhPlayer::Type)) {
		return;
	}
	hhPlayer *thisClient = static_cast<hhPlayer *>(gameLocal.entities[clientNum]);

	if (gameLocal.gameType == GAME_TDM && leavingGame) { //when leaving game in tdm, only care if this is the last player on the team.
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (gameLocal.entities[i] && gameLocal.entities[i]->IsType(hhPlayer::Type) && i != clientNum) {
				hhPlayer *pl = static_cast<hhPlayer *>(gameLocal.entities[i]);
				if (!pl->spectating && pl->team == thisClient->team) {
					return;
				}
			}
		}
	}

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (gameLocal.entities[i] && gameLocal.entities[i]->IsType(hhPlayer::Type) && i != clientNum) {
			hhPlayer *pl = static_cast<hhPlayer *>(gameLocal.entities[i]);
			if (!pl->spectating) {
				numInGame++;
				if (gameLocal.gameType == GAME_TDM && pl->team == thisClient->team) { //do not care about my teammates.
					continue;
				}
				int plFrags;
				if (gameLocal.gameType == GAME_TDM) {
					plFrags = playerState[i].teamFragCount;
				}
				else {
					plFrags = playerState[i].fragCount;
				}
				if (!highestScoreSet || plFrags > highestScore) {
					highestScore = plFrags;
					numWithHighscore = 1;
					highestScoreSet = true;
				}
				else if (plFrags == highestScore) { //count up people who are tied
					numWithHighscore++;
				}
			}
		}
	}

	if (numInGame <= 1 || numWithHighscore <= 0) { //if only 1 person or less is actively playing, or no one on the othe team in tdm, we shouldn't say anything
		return;
	}

	if (gameLocal.gameType == GAME_TDM) { //for team dm, just say only one person has the score, so all the teammates hear ties/losses
		numWithHighscore = 1;
	}

	if (newScore > highestScore && oldScore <= highestScore && !leavingGame) { //taken the lead
		PlayGlobalSound(clientNum, SND_LEADTAKEN);
		for (i = 0; i < MAX_CLIENTS; i++) { //see who was ahead, tell them they lost the lead
			if (gameLocal.entities[i] && gameLocal.entities[i]->IsType(hhPlayer::Type) && i != clientNum) {
				int plFrags;
				if (gameLocal.gameType == GAME_TDM) {
					plFrags = playerState[i].teamFragCount;
				}
				else {
					plFrags = playerState[i].fragCount;
				}
				if (plFrags == highestScore) {
					hhPlayer *pl = static_cast<hhPlayer *>(gameLocal.entities[i]);
					if (!pl->spectating) {
						PlayGlobalSound(i, SND_LEADLOST);
					}
				}
				else if (gameLocal.gameType == GAME_TDM && plFrags == newScore) { //teammates are now in the lead too
					hhPlayer *pl = static_cast<hhPlayer *>(gameLocal.entities[i]);
					if (!pl->spectating) {
						PlayGlobalSound(i, SND_LEADTAKEN);
					}
				}
			}
		}
	}
	else if (newScore == highestScore && !leavingGame) { //tied for the lead
		PlayGlobalSound(clientNum, SND_LEADTIED);
		if (numWithHighscore == 1) {
			for (i = 0; i < MAX_CLIENTS; i++) { //see who else is tied, but only if there was not already a tie at this score
				if (gameLocal.entities[i] && gameLocal.entities[i]->IsType(hhPlayer::Type) && i != clientNum) {
					int plFrags;
					if (gameLocal.gameType == GAME_TDM) {
						plFrags = playerState[i].teamFragCount;
					}
					else {
						plFrags = playerState[i].fragCount;
					}
					if (plFrags == highestScore) {
						hhPlayer *pl = static_cast<hhPlayer *>(gameLocal.entities[i]);
						if (!pl->spectating) {
							PlayGlobalSound(i, SND_LEADTIED);
						}
					}
				}
			}
		}
	}
	else if ((newScore < highestScore || leavingGame) && oldScore >= highestScore) { //lost the lead
		if (!leavingGame) {
			PlayGlobalSound(clientNum, SND_LEADLOST);
		}
		if (numWithHighscore == 1 || oldScore > highestScore) { //if they were tied, only report to one person, otherwise report ties to everyone below
			for (i = 0; i < MAX_CLIENTS; i++) { //see who is ahead now
				if (gameLocal.entities[i] && gameLocal.entities[i]->IsType(hhPlayer::Type) && i != clientNum) {
					int plFrags;
					if (gameLocal.gameType == GAME_TDM) {
						plFrags = playerState[i].teamFragCount;
					}
					else {
						plFrags = playerState[i].fragCount;
					}
					if (plFrags == highestScore) {
						hhPlayer *pl = static_cast<hhPlayer *>(gameLocal.entities[i]);
						if (!pl->spectating) {
							if (numWithHighscore > 1) { //if they are tied with someone now
								PlayGlobalSound(i, SND_LEADTIED);
							}
							else { //or, if they took the lead altogether
								PlayGlobalSound(i, SND_LEADTAKEN);
							}
						}
					}
				}
			}
		}
	}
}
//HUMANHEAD END

const int ASYNC_PLAYER_FRAG_BITS = -idMath::BitsForInteger( MP_PLAYER_MAXFRAGS - MP_PLAYER_MINFRAGS );	// player can have negative frags
const int ASYNC_PLAYER_WINS_BITS = idMath::BitsForInteger( MP_PLAYER_MAXWINS );
const int ASYNC_PLAYER_PING_BITS = idMath::BitsForInteger( MP_PLAYER_MAXPING );

/*
================
idMultiplayerGame::WriteToSnapshot
================
*/
void idMultiplayerGame::WriteToSnapshot( idBitMsgDelta &msg ) const {
	int i;
	int value;

	msg.WriteByte( gameState );
	msg.WriteShort( currentTourneyPlayer[ 0 ] );
	msg.WriteShort( currentTourneyPlayer[ 1 ] );
	msg.WriteBits(nextStateSwitch, 32); //HUMANHEAD rww - cheaper than sending strings
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		// clamp all values to min/max possible value that we can send over
		value = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[i].fragCount );
		msg.WriteBits( value, ASYNC_PLAYER_FRAG_BITS );
		value = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[i].teamFragCount );
		msg.WriteBits( value, ASYNC_PLAYER_FRAG_BITS );
		value = idMath::ClampInt( 0, MP_PLAYER_MAXWINS, playerState[i].wins );
		msg.WriteBits( value, ASYNC_PLAYER_WINS_BITS );
		value = idMath::ClampInt( 0, MP_PLAYER_MAXPING, playerState[i].ping );
		msg.WriteBits( value, ASYNC_PLAYER_PING_BITS );
		msg.WriteBits( playerState[i].ingame, 1 );
	}
}

/*
================
idMultiplayerGame::ReadFromSnapshot
================
*/
void idMultiplayerGame::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	int i;
	gameState_t newState;

	newState = (idMultiplayerGame::gameState_t)msg.ReadByte();
	if ( newState != gameState ) {
		gameLocal.DPrintf( "%s -> %s\n", GameStateStrings[ gameState ], GameStateStrings[ newState ] );
		gameState = newState;
		// these could be gathered in a BGNewState() kind of thing, as we have to do them in NewState as well
		if ( gameState == GAMEON ) {
			matchStartedTime = gameLocal.time;
			cvarSystem->SetCVarString( "ui_ready", "Not Ready" );
			switchThrottle[ 1 ] = 0;	// passby the throttle
			startFragLimit = gameLocal.serverInfo.GetInt( "si_fragLimit" );
		}
	}
	currentTourneyPlayer[ 0 ] = msg.ReadShort();
	currentTourneyPlayer[ 1 ] = msg.ReadShort();
	nextStateSwitch = msg.ReadBits(32); //HUMANHEAD rww - cheaper than sending strings
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		playerState[i].fragCount = msg.ReadBits( ASYNC_PLAYER_FRAG_BITS );
		playerState[i].teamFragCount = msg.ReadBits( ASYNC_PLAYER_FRAG_BITS );
		playerState[i].wins = msg.ReadBits( ASYNC_PLAYER_WINS_BITS );
		playerState[i].ping = msg.ReadBits( ASYNC_PLAYER_PING_BITS );
		playerState[i].ingame = msg.ReadBits( 1 ) != 0;
	}
}

/*
================
idMultiplayerGame::PlayGlobalSound
================
*/
void idMultiplayerGame::PlayGlobalSound( int to, snd_evt_t evt, const char *shader ) {
	const idSoundShader *shaderDecl;

	if ( to == -1 || to == gameLocal.localClientNum ) {
		if ( shader ) {
			gameSoundWorld->PlayShaderDirectly( shader );
		} else {
			gameSoundWorld->PlayShaderDirectly( GlobalSoundStrings[ evt ] );
		}
	}

	if ( !gameLocal.isClient ) {
		idBitMsg outMsg;
		byte msgBuf[1024];
		outMsg.Init( msgBuf, sizeof( msgBuf ) );

		if ( shader ) {
			shaderDecl = declManager->FindSound( shader );
			if ( !shaderDecl ) {
				return;
			}
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SOUND_INDEX );
			outMsg.WriteLong( gameLocal.ServerRemapDecl( to, DECL_SOUND, shaderDecl->Index() ) );
		} else {
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SOUND_EVENT );
			outMsg.WriteByte( evt );
		}

		networkSystem->ServerSendReliableMessage( to, outMsg );
	}
}

//HUMANHEAD rww
const char *idMultiplayerGame::DeathStringForProjectile(int projectileDefNum) {
	if (projectileDefNum) {
		const idDeclEntityDef *projDecl = static_cast<const idDeclEntityDef *>(declManager->DeclByIndex(DECL_ENTITYDEF, projectileDefNum, false));
		if (projDecl) {
			const char *deathStr = projDecl->dict.GetString("str_deathmessage", "");
			if (deathStr && deathStr[0]) {
				return common->GetLanguageDict()->GetString(deathStr);
			}
		}
	}

	return common->GetLanguageDict()->GetString( "#str_04292" );
}

void idMultiplayerGame::PrintDeathMessageEvent( int to, msg_evt_t evt, int parm1, int parm2, int inflictorDef ) {
	switch ( evt ) {
		//HUMANHEAD rww - reworked all of these AddChatLine calls to allow for our coloring-based-on-kill-involvement junk
		char *formatting;
		case MSG_SUICIDE:
			assert( parm1 >= 0 );
			if (parm1 == gameLocal.localClientNum) {
				formatting = "^3%s^1%s^0";
			}
			else {
				formatting = "%s^0%s";
			}
			AddChatLine( formatting, gameLocal.userInfo[ parm1 ].GetString( "ui_name" ), common->GetLanguageDict()->GetString( "#str_04293" ) );
			break;
		case MSG_KILLED:
			assert( parm1 >= 0 && parm2 >= 0 );
			if (parm1 == gameLocal.localClientNum) {
				formatting = "^3%s^1%s^3%s^0";
			}
			else if (parm2 == gameLocal.localClientNum) {
				formatting = "^3%s^2%s^3%s^0";
			}
			else {
				formatting = "%s^0%s%s^0";
			}
			AddChatLine( formatting, gameLocal.userInfo[ parm1 ].GetString( "ui_name" ), DeathStringForProjectile(inflictorDef), gameLocal.userInfo[ parm2 ].GetString( "ui_name" ) );
			break;
		case MSG_KILLEDTEAM:
			assert( parm1 >= 0 && parm2 >= 0 );
			if (parm1 == gameLocal.localClientNum) {
				formatting = "^3%s^1%s^3%s^0";
			}
			else if (parm2 == gameLocal.localClientNum) {
				formatting = "^3%s^1%s^3%s^0";
			}
			else {
				formatting = "%s^0%s%s^0";
			}
			AddChatLine( formatting, gameLocal.userInfo[ parm1 ].GetString( "ui_name" ), common->GetLanguageDict()->GetString( "#str_04291" ), gameLocal.userInfo[ parm2 ].GetString( "ui_name" ) );
			break;
		case MSG_TELEFRAGGED:
			assert( parm1 >= 0 && parm2 >= 0 );
			if (parm1 == gameLocal.localClientNum) {
				formatting = "^3%s^1%s^3%s^0";
			}
			else if (parm2 == gameLocal.localClientNum) {
				formatting = "^3%s^2%s^3%s^0";
			}
			else {
				formatting = "%s^0%s%s^0";
			}
			AddChatLine( formatting, gameLocal.userInfo[ parm1 ].GetString( "ui_name" ), common->GetLanguageDict()->GetString( "#str_04290" ), gameLocal.userInfo[ parm2 ].GetString( "ui_name" ) );
			break;
		case MSG_DIED:
			assert( parm1 >= 0 );
			if (parm1 == gameLocal.localClientNum) {
				formatting = "^3%s^1%s^0";
			}
			else {
				formatting = "%s^0%s";
			}
			AddChatLine( formatting, gameLocal.userInfo[ parm1 ].GetString( "ui_name" ), common->GetLanguageDict()->GetString( "#str_04289" ) );
			break;
		default:
			gameLocal.DPrintf( "PrintDeathMessageEvent: unknown message type %d\n", evt );
			return;
	}

	if ( !gameLocal.isClient ) {
		idBitMsg outMsg;
		byte msgBuf[1024];
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_DB_DEATH );
		outMsg.WriteByte( evt );
		outMsg.WriteByte( parm1 );
		outMsg.WriteByte( parm2 );
		outMsg.WriteBits(gameLocal.ServerRemapDecl(-1, DECL_ENTITYDEF, inflictorDef ), gameLocal.entityDefBits);
		networkSystem->ServerSendReliableMessage( to, outMsg );
	}
}
//HUMANHEAD END

/*
================
idMultiplayerGame::PrintMessageEvent
================
*/
void idMultiplayerGame::PrintMessageEvent( int to, msg_evt_t evt, int parm1, int parm2 ) {
	switch ( evt ) {
		case MSG_VOTE:
			AddChatLine( common->GetLanguageDict()->GetString( "#str_04288" ) );
			break;
#if HUMANHEAD	// HUMANHEAD pdm: Announce individual votes
		case MSG_PLAYERVOTED:
			AddChatLine( common->GetLanguageDict()->GetString( "#str_00835" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ) );
			break;
#endif
		case MSG_SUDDENDEATH:
			AddChatLine( common->GetLanguageDict()->GetString( "#str_04287" ) );
			break;
		case MSG_FORCEREADY:
			AddChatLine( common->GetLanguageDict()->GetString( "#str_04286" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ) );
			if ( gameLocal.entities[ parm1 ] && gameLocal.entities[ parm1 ]->IsType( idPlayer::Type ) ) {
				static_cast< idPlayer * >( gameLocal.entities[ parm1 ] )->forcedReady = true;
			}
			break;
		case MSG_JOINEDSPEC:
			AddChatLine( common->GetLanguageDict()->GetString( "#str_04285" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ) );
			break;
		case MSG_TIMELIMIT:
			AddChatLine( common->GetLanguageDict()->GetString( "#str_04284" ) );
			break;
		case MSG_FRAGLIMIT:
//			if ( gameLocal.gameType == GAME_LASTMAN ) {
//				AddChatLine( common->GetLanguageDict()->GetString( "#str_04283" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ) );
//			} else
			if ( gameLocal.gameType == GAME_TDM ) {
				AddChatLine( common->GetLanguageDict()->GetString( "#str_04282" ), gameLocal.userInfo[ parm1 ].GetString( "ui_team" ) );
			} else {
				AddChatLine( common->GetLanguageDict()->GetString( "#str_04281" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ) );
			}
			break;
		case MSG_JOINTEAM:
			AddChatLine( common->GetLanguageDict()->GetString( "#str_04280" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ), parm2 ? common->GetLanguageDict()->GetString( "#str_02500" ) : common->GetLanguageDict()->GetString( "#str_02499" ) );
			break;
		case MSG_HOLYSHIT:
			AddChatLine( common->GetLanguageDict()->GetString( "#str_06732" ) );
			break;
		default:
			gameLocal.DPrintf( "PrintMessageEvent: unknown message type %d\n", evt );
			return;
	}
	if ( !gameLocal.isClient ) {
		idBitMsg outMsg;
		byte msgBuf[1024];
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_DB );
		outMsg.WriteByte( evt );
		outMsg.WriteByte( parm1 );
		outMsg.WriteByte( parm2 );
		networkSystem->ServerSendReliableMessage( to, outMsg );
	}
}

/*
================
idMultiplayerGame::SuddenRespawns
solely for LMN if an end game ( fragLimitTimeout ) was entered and aborted before expiration
LMN players which still have lives left need to be respawned without being marked lastManOver
================
*/
void idMultiplayerGame::SuddenRespawn( void ) {
	int i;

	if ( gameLocal.gameType != GAME_LASTMAN ) {
		return;
	}

	for ( i = 0; i < gameLocal.numClients; i++ ) {
		if ( !gameLocal.entities[ i ] || !gameLocal.entities[ i ]->IsType( idPlayer::Type ) ) {
			continue;
		}
		if ( !CanPlay( static_cast< idPlayer * >( gameLocal.entities[ i ] ) ) ) {
			continue;
		}
		if ( static_cast< idPlayer * >( gameLocal.entities[ i ] )->lastManOver ) {
			continue;
		}
		static_cast< idPlayer * >( gameLocal.entities[ i ] )->lastManPlayAgain = true;
	}
}

/*
================
idMultiplayerGame::CheckSpawns
================
*/
void idMultiplayerGame::CheckRespawns( idPlayer *spectator ) {
	for( int i = 0 ; i < gameLocal.numClients ; i++ ) {
		idEntity *ent = gameLocal.entities[ i ];
		if ( !ent || !ent->IsType( idPlayer::Type ) ) {
			continue;
		}
		idPlayer *p = static_cast<idPlayer *>(ent);
		// once we hit sudden death, nobody respawns till game has ended
		if ( WantRespawn( p ) || p == spectator ) {
			if ( gameState == SUDDENDEATH && gameLocal.gameType != GAME_LASTMAN ) {
				// respawn rules while sudden death are different
				// sudden death may trigger while a player is dead, so there are still cases where we need to respawn
				// don't do any respawns while we are in end game delay though
				if ( !fragLimitTimeout ) {
					if ( gameLocal.gameType == GAME_TDM || p->IsLeader() ) {
#ifdef _DEBUG
						if ( gameLocal.gameType == GAME_TOURNEY ) {
							assert( p->entityNumber == currentTourneyPlayer[ 0 ] || p->entityNumber == currentTourneyPlayer[ 1 ] );
						}
#endif
						p->ServerSpectate( false );
					} else if ( !p->IsLeader() ) {
						// sudden death is rolling, this player is not a leader, have him spectate
						p->ServerSpectate( true );
						CheckAbortGame();
					}
				}
			} else {
				if (gameLocal.gameType == GAME_SP)
				{ //HUMANHEAD rww - stick players in immediately for coop
					p->ServerSpectate( false );
				}
				else if ( gameLocal.gameType == GAME_DM ||
					gameLocal.gameType == GAME_TDM ) {
					if ( gameState == WARMUP || gameState == COUNTDOWN || gameState == GAMEON ) {
						p->ServerSpectate( false );
					}				
				} else if ( gameLocal.gameType == GAME_TOURNEY ) {
					if ( i == currentTourneyPlayer[ 0 ] || i == currentTourneyPlayer[ 1 ] ) {
						if ( gameState == WARMUP || gameState == COUNTDOWN || gameState == GAMEON ) {
							p->ServerSpectate( false );
						}
					} else if ( gameState == WARMUP ) {
						// make sure empty tourney slots get filled first
						FillTourneySlots( );
						if ( i == currentTourneyPlayer[ 0 ] || i == currentTourneyPlayer[ 1 ] ) {
							p->ServerSpectate( false );
						}
					}
				} else if ( gameLocal.gameType == GAME_LASTMAN ) {
					if ( gameState == WARMUP || gameState == COUNTDOWN ) {
						p->ServerSpectate( false );
					} else if ( gameState == GAMEON || gameState == SUDDENDEATH ) {
						if ( gameState == GAMEON && playerState[ i ].fragCount > 0 && p->lastManPresent ) {
							assert( !p->lastManOver );
							p->ServerSpectate( false );
						} else if ( p->lastManPlayAgain && p->lastManPresent ) {
							assert( gameState == SUDDENDEATH );
							p->ServerSpectate( false );
						} else {
							// if a fragLimitTimeout was engaged, do NOT mark lastManOver as that could mean
							// everyone ends up spectator and game is stalled with no end
							// if the frag limit delay is engaged and cancels out before expiring, LMN players are
							// respawned to play the tie again ( through SuddenRespawn and lastManPlayAgain )
							if ( !fragLimitTimeout && !p->lastManOver ) {
								common->DPrintf( "client %d has lost all last man lives\n", i );
								// end of the game for this guy, send him to spectators
								p->lastManOver = true;
								// clients don't have access to lastManOver
								// so set the fragCount to something silly ( used in scoreboard and player ranking )
								playerState[ i ].fragCount = LASTMAN_NOLIVES;
								p->ServerSpectate( true );
								
								//Check for a situation where the last two player dies at the same time and don't
								//try to respawn manually...This was causing all players to go into spectate mode
								//and the server got stuck
								{
									int j;
									for ( j = 0; j < gameLocal.numClients; j++ ) {
										if ( !gameLocal.entities[ j ] ) {
											continue;
										}
										if ( !CanPlay( static_cast< idPlayer * >( gameLocal.entities[ j ] ) ) ) {
											continue;
										}
										if ( !static_cast< idPlayer * >( gameLocal.entities[ j ] )->lastManOver ) {
											break;
										}
									}
									if( j == gameLocal.numClients) {
										//Everyone is dead so don't allow this player to spectate
										//so the match will end
										p->ServerSpectate( false );
									}
								}
							}
						}
					}
				}
			}
		} else if ( p->wantSpectate && !p->spectating ) {
			playerState[ i ].fragCount = 0; // whenever you willingly go spectate during game, your score resets
			p->ServerSpectate( true );
			UpdateTourneyLine();
			CheckAbortGame();
		}
	}
}

/*
================
idMultiplayerGame::ForceReady
================
*/
void idMultiplayerGame::ForceReady( ) {

	for( int i = 0 ; i < gameLocal.numClients ; i++ ) {
		idEntity *ent = gameLocal.entities[ i ];
		if ( !ent || !ent->IsType( idPlayer::Type ) ) {
			continue;
		}
		idPlayer *p = static_cast<idPlayer *>( ent );
		if ( !p->IsReady() ) {
			PrintMessageEvent( -1, MSG_FORCEREADY, i );
			p->forcedReady = true;
		}
	}
}

/*
================
idMultiplayerGame::ForceReady_f
================
*/
void idMultiplayerGame::ForceReady_f( const idCmdArgs &args ) {
	if ( !gameLocal.isMultiplayer || gameLocal.isClient ) {
		common->Printf( "forceReady: multiplayer server only\n" );
		return;
	}
	gameLocal.mpGame.ForceReady();
}

/*
================
idMultiplayerGame::DropWeapon
================
*/
void idMultiplayerGame::DropWeapon( int clientNum ) {
	assert( !gameLocal.isClient );
	idEntity *ent = gameLocal.entities[ clientNum ];
	if ( !ent || !ent->IsType( idPlayer::Type ) ) {
		return;
	}
	static_cast< idPlayer* >( ent )->DropWeapon( false );
}

/*
================
idMultiplayerGame::DropWeapon_f
================
*/
void idMultiplayerGame::DropWeapon_f( const idCmdArgs &args ) {
	//HUMANHEAD rww - disabling functionality for this, for now (and probably for good)
	/*
	if ( !gameLocal.isMultiplayer ) {
		common->Printf( "clientDropWeapon: only valid in multiplayer\n" );
		return;
	}
	idBitMsg	outMsg;
	byte		msgBuf[128];
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_DROPWEAPON );
	networkSystem->ClientSendReliableMessage( outMsg );
	*/
	//HUMANHEAD END
}

/*
================
idMultiplayerGame::MessageMode_f
================
*/
void idMultiplayerGame::MessageMode_f( const idCmdArgs &args ) {
	gameLocal.mpGame.MessageMode( args );
}

/*
================
idMultiplayerGame::MessageMode
================
*/
void idMultiplayerGame::MessageMode( const idCmdArgs &args ) {
	const char *mode;
	int imode;

	if ( !gameLocal.isMultiplayer ) {
		common->Printf( "clientMessageMode: only valid in multiplayer\n" );
		return;
	}
	if ( !mainGui ) {
		common->Printf( "no local client\n" );
		return;
	}
	mode = args.Argv( 1 );
	if ( !mode[ 0 ] ) {
		imode = 0;
	} else {
		imode = atoi( mode );
	}
	msgmodeGui->SetStateString( "messagemode", imode ? "1" : "0" );
	msgmodeGui->SetStateString( "chattext", "" );
	nextMenu = 2;
	// let the session know that we want our ingame main menu opened
	gameLocal.sessionCommand = "game_startmenu";
}

/*
================
idMultiplayerGame::Vote_f
FIXME: voting from console
================
*/
void idMultiplayerGame::Vote_f( const idCmdArgs &args ) { }

/*
================
idMultiplayerGame::CallVote_f
FIXME: voting from console
================
*/
void idMultiplayerGame::CallVote_f( const idCmdArgs &args ) { }

/*
================
idMultiplayerGame::ServerStartVote
================
*/
void idMultiplayerGame::ServerStartVote( int clientNum, vote_flags_t voteIndex, const char *value ) {
	int i;

	assert( vote == VOTE_NONE );

	// setup
	yesVotes = 1;
	noVotes = 0;
	vote = voteIndex;
	voteValue = value;
	voteTimeOut = gameLocal.time + 20000;
	// mark players allowed to vote - only current ingame players, players joining during vote will be ignored
	for ( i = 0; i < gameLocal.numClients; i++ ) {
		if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idPlayer::Type ) ) {
			playerState[ i ].vote = ( i == clientNum ) ? PLAYER_VOTE_YES : PLAYER_VOTE_WAIT;
		} else {
			playerState[i].vote = PLAYER_VOTE_NONE;
		}
	}
}

/*
================
idMultiplayerGame::ClientStartVote
================
*/
#if _HH_LOCALIZE_VOTESTRINGS
void idMultiplayerGame::ClientStartVote( int clientNum, voteStringType_e strType, const int numStrings, const char **strings ) {
	if ( !gameLocal.isClient ) {
		idBitMsg	outMsg;
		byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_STARTVOTE );
		outMsg.WriteByte( clientNum );
		assert(strType < (1<<6));
		outMsg.WriteBits(strType, 6);
		assert(numStrings < (1<<4));
		outMsg.WriteBits(numStrings, 4);
		for (int i = 0; i < numStrings; i++) {
			outMsg.WriteString( strings[i] );
		}
		networkSystem->ServerSendReliableMessage( -1, outMsg );
	}

	static char assembledVoteString[MAX_STRING_CHARS];
	switch (strType) {
		case VOTESTR_JUSTONE:
			sprintf(assembledVoteString, common->GetLanguageDict()->GetString(strings[0]));
			break;
		case VOTESTR_TIMELIMIT:
			sprintf(assembledVoteString, common->GetLanguageDict()->GetString(strings[0]), atoi(strings[1]));
			break;
		case VOTESTR_FRAGLIMIT:
			sprintf(assembledVoteString, common->GetLanguageDict()->GetString(strings[0]), common->GetLanguageDict()->GetString(strings[1]), atoi(strings[2]));
			break;
		case VOTESTR_VALUE:
			sprintf(assembledVoteString, common->GetLanguageDict()->GetString(strings[0]), strings[1]);
			break;
		case VOTESTR_CLNUMNAME:
			sprintf(assembledVoteString, common->GetLanguageDict()->GetString(strings[0]), atoi(strings[1]), strings[2]);
			break;
		case VOTESTR_NAMEVAL:
			sprintf(assembledVoteString, common->GetLanguageDict()->GetString(strings[0]), common->GetLanguageDict()->GetString(strings[1]));
			break;
		default:
			sprintf(assembledVoteString, "unknown");
			break;
	}

	voteString = assembledVoteString;
	AddChatLine( va( common->GetLanguageDict()->GetString( "#str_04279" ), gameLocal.userInfo[ clientNum ].GetString( "ui_name" ) ) );
	gameSoundWorld->PlayShaderDirectly( GlobalSoundStrings[ SND_VOTE ] );
	if ( clientNum == gameLocal.localClientNum ) {
		voted = true;
	} else {
		voted = false;
	}
	if ( gameLocal.isClient ) {
		// the the vote value to something so the vote line is displayed
		vote = VOTE_RESTART;
		yesVotes = 1;
		noVotes = 0;
		voteTimeOut = gameLocal.time + 20000;	// HUMANHEAD pdm: predict vote expiration time on clients
	}
}
#else
void idMultiplayerGame::ClientStartVote( int clientNum, const char *_voteString ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	if ( !gameLocal.isClient ) {
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_STARTVOTE );
		outMsg.WriteByte( clientNum );
		outMsg.WriteString( _voteString );
		networkSystem->ServerSendReliableMessage( -1, outMsg );
	}

	voteString = _voteString;
	AddChatLine( va( common->GetLanguageDict()->GetString( "#str_04279" ), gameLocal.userInfo[ clientNum ].GetString( "ui_name" ) ) );
	gameSoundWorld->PlayShaderDirectly( GlobalSoundStrings[ SND_VOTE ] );
	if ( clientNum == gameLocal.localClientNum ) {
		voted = true;
	} else {
		voted = false;
	}
	if ( gameLocal.isClient ) {
		// the the vote value to something so the vote line is displayed
		vote = VOTE_RESTART;
		yesVotes = 1;
		noVotes = 0;
		voteTimeOut = gameLocal.time + 20000;	// HUMANHEAD pdm: predict vote expiration time on clients
	}
}
#endif

/*
================
idMultiplayerGame::ClientUpdateVote
================
*/
void idMultiplayerGame::ClientUpdateVote( vote_result_t status, int yesCount, int noCount ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	if ( !gameLocal.isClient ) {
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_UPDATEVOTE );
		outMsg.WriteByte( status );
		outMsg.WriteByte( yesCount );
		outMsg.WriteByte( noCount );
		networkSystem->ServerSendReliableMessage( -1, outMsg );
	}

	if ( vote == VOTE_NONE ) {
		// clients coming in late don't get the vote start and are not allowed to vote
		return;
	}

	switch ( status ) {
		case VOTE_FAILED:
			AddChatLine( common->GetLanguageDict()->GetString( "#str_04278" ) );
			gameSoundWorld->PlayShaderDirectly( GlobalSoundStrings[ SND_VOTE_FAILED ] );
			if ( gameLocal.isClient ) {
				vote = VOTE_NONE;
			}
			break;
		case VOTE_PASSED:
			AddChatLine( common->GetLanguageDict()->GetString( "#str_04277" ) );
			gameSoundWorld->PlayShaderDirectly( GlobalSoundStrings[ SND_VOTE_PASSED ] );
			break;
		case VOTE_RESET:
			if ( gameLocal.isClient ) {
				vote = VOTE_NONE;
			}
			break;
		case VOTE_ABORTED:
			AddChatLine( common->GetLanguageDict()->GetString( "#str_04276" ) );
			if ( gameLocal.isClient ) {
				vote = VOTE_NONE;
			}
			break;
		default:
			break;
	}
	if ( gameLocal.isClient ) {
		yesVotes = yesCount;
		noVotes = noCount;
	}
}

/*
================
idMultiplayerGame::ClientCallVote
================
*/
void idMultiplayerGame::ClientCallVote( vote_flags_t voteIndex, const char *voteValue ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	// send 
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_CALLVOTE );
	outMsg.WriteByte( voteIndex );
	outMsg.WriteString( voteValue );
	networkSystem->ClientSendReliableMessage( outMsg );
}

/*
================
idMultiplayerGame::CastVote
================
*/
void idMultiplayerGame::CastVote( int clientNum, bool castVote ) {
	idBitMsg	outMsg;
	byte		msgBuf[ 128 ];

	if ( clientNum == gameLocal.localClientNum ) {
		voted = true;
	}

	if ( gameLocal.isClient ) {
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_CASTVOTE );
		outMsg.WriteByte( castVote );
		networkSystem->ClientSendReliableMessage( outMsg );
		return;
	}

	// sanity
	if ( vote == VOTE_NONE ) {
		gameLocal.ServerSendChatMessage( clientNum, "server", common->GetLanguageDict()->GetString( "#str_04275" ) );
		common->DPrintf( "client %d: cast vote while no vote in progress\n", clientNum );
		return;
	}
	if ( playerState[ clientNum ].vote != PLAYER_VOTE_WAIT ) {
		gameLocal.ServerSendChatMessage( clientNum, "server", common->GetLanguageDict()->GetString( "#str_04274" ) );
		common->DPrintf( "client %d: cast vote - vote %d != PLAYER_VOTE_WAIT\n", clientNum, playerState[ clientNum ].vote );
		return;
	}

	if ( castVote ) {
		playerState[ clientNum ].vote = PLAYER_VOTE_YES;
		yesVotes++;
	} else {
		playerState[ clientNum ].vote = PLAYER_VOTE_NO;
		noVotes++;
	}

#if HUMANHEAD // HUMANHEAD pdm: Announce client's vote
	PrintMessageEvent( -1, MSG_PLAYERVOTED, clientNum, (int)castVote );
#endif

	ClientUpdateVote( VOTE_UPDATE, yesVotes, noVotes );
}

/*
================
idMultiplayerGame::ServerCallVote
================
*/
void idMultiplayerGame::ServerCallVote( int clientNum, const idBitMsg &msg ) {
	vote_flags_t	voteIndex;
	int				vote_timeLimit, vote_fragLimit, vote_clientNum, vote_gameTypeIndex; //, vote_kickIndex;
	char			value[ MAX_STRING_CHARS ];

	assert( clientNum != -1 );
	assert( !gameLocal.isClient );

	voteIndex = (vote_flags_t)msg.ReadByte( );
	msg.ReadString( value, sizeof( value ) );

#if _HH_LOCALIZE_VOTESTRINGS
	const  char		*strPtr[16];
	char			strIntHolder[MAX_STRING_CHARS];
#endif

	// sanity checks - setup the vote
	if ( vote != VOTE_NONE ) {
		//HUMANHEAD PCF rww 05/17/06
		//gameLocal.ServerSendChatMessage( clientNum, "server", common->GetLanguageDict()->GetString( "#str_04273" ) );
		strPtr[0] = "#str_04273";
		gameLocal.ServerSendSpecialMessage(idGameLocal::SPECIALMSG_JUSTONE, clientNum, "#str_02047", 1, strPtr);
		//HUMANHEAD END
		common->DPrintf( "client %d: called vote while voting already in progress - ignored\n", clientNum );
		return;
	}
	switch ( voteIndex ) {
		case VOTE_RESTART:
			ServerStartVote( clientNum, voteIndex, "" );
#if _HH_LOCALIZE_VOTESTRINGS
			strPtr[0] = "#str_04271";
			ClientStartVote(clientNum, VOTESTR_JUSTONE, 1, strPtr);
#else
			ClientStartVote( clientNum, common->GetLanguageDict()->GetString( "#str_04271" ) );
#endif
			break;
		case VOTE_NEXTMAP:
			ServerStartVote( clientNum, voteIndex, "" );
#if _HH_LOCALIZE_VOTESTRINGS
			strPtr[0] = "#str_04272";
			ClientStartVote(clientNum, VOTESTR_JUSTONE, 1, strPtr);
#else
			ClientStartVote( clientNum, common->GetLanguageDict()->GetString( "#str_04272" ) );
#endif
			break;
		case VOTE_TIMELIMIT:
			vote_timeLimit = strtol( value, NULL, 10 );
			if ( vote_timeLimit == gameLocal.serverInfo.GetInt( "si_timeLimit" ) ) {
				//HUMANHEAD PCF rww 05/17/06
				//gameLocal.ServerSendChatMessage( clientNum, "server", common->GetLanguageDict()->GetString( "#str_04270" ) );
				strPtr[0] = "#str_04270";
				gameLocal.ServerSendSpecialMessage(idGameLocal::SPECIALMSG_JUSTONE, clientNum, "#str_02047", 1, strPtr);
				//HUMANHEAD END
				common->DPrintf( "client %d: already at the voted Time Limit\n", clientNum );
				return;					
			}
			if ( vote_timeLimit < si_timeLimit.GetMinValue() || vote_timeLimit > si_timeLimit.GetMaxValue() ) {
				//HUMANHEAD PCF rww 05/17/06
				//gameLocal.ServerSendChatMessage( clientNum, "server", common->GetLanguageDict()->GetString( "#str_04269" ) );
				strPtr[0] = "#str_04269";
				gameLocal.ServerSendSpecialMessage(idGameLocal::SPECIALMSG_JUSTONE, clientNum, "#str_02047", 1, strPtr);
				//HUMANHEAD END
				common->DPrintf( "client %d: timelimit value out of range for vote: %s\n", clientNum, value );
				return;
			}
			ServerStartVote( clientNum, voteIndex, value );
#if _HH_LOCALIZE_VOTESTRINGS
			strPtr[0] = "#str_04268";
			sprintf(strIntHolder, "%i", vote_timeLimit);
			strPtr[1] = strIntHolder;
			ClientStartVote(clientNum, VOTESTR_TIMELIMIT, 2, strPtr);
#else
			ClientStartVote( clientNum, va( common->GetLanguageDict()->GetString( "#str_04268" ), vote_timeLimit ) );
#endif
			break;
		case VOTE_FRAGLIMIT:
			vote_fragLimit = strtol( value, NULL, 10 );
			if ( vote_fragLimit == gameLocal.serverInfo.GetInt( "si_fragLimit" ) ) {
				//HUMANHEAD PCF rww 05/17/06
				//gameLocal.ServerSendChatMessage( clientNum, "server", common->GetLanguageDict()->GetString( "#str_04267" ) );
				strPtr[0] = "#str_04267";
				gameLocal.ServerSendSpecialMessage(idGameLocal::SPECIALMSG_JUSTONE, clientNum, "#str_02047", 1, strPtr);
				//HUMANHEAD END
				common->DPrintf( "client %d: already at the voted Frag Limit\n", clientNum );
				return;					
			}
			if ( vote_fragLimit < si_fragLimit.GetMinValue() || vote_fragLimit > si_fragLimit.GetMaxValue() ) {
				//HUMANHEAD PCF rww 05/17/06
				//gameLocal.ServerSendChatMessage( clientNum, "server", common->GetLanguageDict()->GetString( "#str_04266" ) );
				strPtr[0] = "#str_04266";
				gameLocal.ServerSendSpecialMessage(idGameLocal::SPECIALMSG_JUSTONE, clientNum, "#str_02047", 1, strPtr);
				//HUMANHEAD END
				common->DPrintf( "client %d: fraglimit value out of range for vote: %s\n", clientNum, value );
				return;
			}
			ServerStartVote( clientNum, voteIndex, value );
			// HUMANHEAD pdm: removed lastman clause
#if _HH_LOCALIZE_VOTESTRINGS
			strPtr[0] = "#str_04303";
			strPtr[1] = "#str_04265";
			sprintf(strIntHolder, "%i", vote_fragLimit);
			strPtr[2] = strIntHolder;
			ClientStartVote(clientNum, VOTESTR_FRAGLIMIT, 3, strPtr);
#else
			ClientStartVote( clientNum, va( common->GetLanguageDict()->GetString( "#str_04303" ), common->GetLanguageDict()->GetString( "#str_04265" ), vote_fragLimit ) );
#endif
			break;
		case VOTE_GAMETYPE:
			vote_gameTypeIndex = strtol( value, NULL, 10 );
			assert( vote_gameTypeIndex >= 0 && vote_gameTypeIndex <= 3 );
			switch ( vote_gameTypeIndex ) {
				case 0:
					strcpy( value, "Deathmatch" );
					break;
				case 1:
					strcpy( value, "Team DM" ); //HUMANHEAD rww
					break;
				case 2:
					strcpy( value, "Team DM" );
					break;
				case 3:
					strcpy( value, "Team DM" ); //HUMANHEAD rww
					break;
			}
			if ( !idStr::Icmp( value, gameLocal.serverInfo.GetString( "si_gameType" ) ) ) {
				//HUMANHEAD PCF rww 05/17/06
				//gameLocal.ServerSendChatMessage( clientNum, "server", common->GetLanguageDict()->GetString( "#str_04259" ) );
				strPtr[0] = "#str_04259";
				gameLocal.ServerSendSpecialMessage(idGameLocal::SPECIALMSG_JUSTONE, clientNum, "#str_02047", 1, strPtr);
				//HUMANHEAD END
				common->DPrintf( "client %d: already at the voted Game Type\n", clientNum );
				return;
			}
			ServerStartVote( clientNum, voteIndex, value );
#if _HH_LOCALIZE_VOTESTRINGS
			strPtr[0] = "#str_04258";
			strPtr[1] = value;
			ClientStartVote(clientNum, VOTESTR_VALUE, 2, strPtr);
#else
			ClientStartVote( clientNum, va( common->GetLanguageDict()->GetString( "#str_04258" ), value ) );
#endif
			break;
		case VOTE_KICK:
			vote_clientNum = strtol( value, NULL, 10 );
			if ( vote_clientNum == gameLocal.localClientNum ) {
				//HUMANHEAD PCF rww 05/17/06
				//gameLocal.ServerSendChatMessage( clientNum, "server", common->GetLanguageDict()->GetString( "#str_04257" ) );
				strPtr[0] = "#str_04257";
				gameLocal.ServerSendSpecialMessage(idGameLocal::SPECIALMSG_JUSTONE, clientNum, "#str_02047", 1, strPtr);
				//HUMANHEAD END
				common->DPrintf( "client %d: called kick for the server host\n", clientNum );
				return;
			}
			ServerStartVote( clientNum, voteIndex, va( "%d", vote_clientNum ) );
#if _HH_LOCALIZE_VOTESTRINGS
			strPtr[0] = "#str_04302";
			sprintf(strIntHolder, "%i", vote_clientNum);
			strPtr[1] = strIntHolder;
			strPtr[2] = gameLocal.userInfo[ vote_clientNum ].GetString( "ui_name" );
			ClientStartVote(clientNum, VOTESTR_CLNUMNAME, 3, strPtr);
#else
			ClientStartVote( clientNum, va( common->GetLanguageDict()->GetString( "#str_04302" ), vote_clientNum, gameLocal.userInfo[ vote_clientNum ].GetString( "ui_name" ) ) );
#endif
			break;
		case VOTE_MAP: {
			//HUMANHEAD rww - changed from FindText == -1 to Cmp == 0
			if ( idStr::Cmp( gameLocal.serverInfo.GetString( "si_map" ), value ) == 0 ) {
				//HUMANHEAD PCF rww 05/17/06
				//gameLocal.ServerSendChatMessage( clientNum, "server", va( common->GetLanguageDict()->GetString( "#str_04295" ), value ) );
				strPtr[0] = "#str_04295";
				strPtr[1] = value;
				gameLocal.ServerSendSpecialMessage(idGameLocal::SPECIALMSG_ALREADYRUNNINGMAP, clientNum, "#str_02047", 2, strPtr);
				//HUMANHEAD END
				common->DPrintf( "client %d: already running the voted map: %s\n", clientNum, value );
				return;
			}
			//HUMANHEAD PCF rww 05/09/06 - avoid caching map dictionary media when we run through the map defs
			assert(cvarSystem);
			bool oldPrecache = cvarSystem->GetCVarBool("com_precache");
			cvarSystem->SetCVarBool("com_precache", false);
			//HUMANHEAD END
			int				num = fileSystem->GetNumMaps();
			int				i;
			const idDict	*dict;
			bool			haveMap = false;
			for ( i = 0; i < num; i++ ) {
				dict = fileSystem->GetMapDecl( i );
				if ( dict && !idStr::Icmp( dict->GetString( "path" ), value ) ) {
					//HUMANHEAD PCF rww 05/27/06 - demo map check
					#ifdef ID_DEMO_BUILD
					if (dict->GetBool("indemo")) {
					#endif
					//HUMANHEAD END
					haveMap = true;
					//HUMANHEAD PCF rww 05/27/06 - demo map check
					#ifdef ID_DEMO_BUILD
					}
					#endif
					//HUMANHEAD END
					break;
				}
			}
			//HUMANHEAD PCF rww 05/09/06 - avoid caching map dictionary media when we run through the map defs
			cvarSystem->SetCVarBool("com_precache", oldPrecache);
			//HUMANHEAD END

			if ( !haveMap ) {
				//HUMANHEAD PCF rww 05/17/06
				//gameLocal.ServerSendChatMessage( clientNum, "server", va( common->GetLanguageDict()->GetString( "#str_04296" ), value ) );
				strPtr[0] = "#str_04296";
				strPtr[1] = value;
				gameLocal.ServerSendSpecialMessage(idGameLocal::SPECIALMSG_ALREADYRUNNINGMAP, clientNum, "#str_02047", 2, strPtr);
				//HUMANHEAD END
				common->Printf( "client %d: map not found: %s\n", clientNum, value );
				return;
			}
			ServerStartVote( clientNum, voteIndex, value );
#if _HH_LOCALIZE_VOTESTRINGS
			strPtr[0] = "#str_04256";
			strPtr[1] = dict ? dict->GetString( "name" ) : value;
			ClientStartVote(clientNum, VOTESTR_NAMEVAL, 2, strPtr);
#else
			ClientStartVote( clientNum, va( common->GetLanguageDict()->GetString( "#str_04256" ), common->GetLanguageDict()->GetString( dict ? dict->GetString( "name" ) : value ) ) );
#endif
			break;
		}
		case VOTE_SPECTATORS:
			if ( gameLocal.serverInfo.GetBool( "si_spectators" ) ) {
				ServerStartVote( clientNum, voteIndex, "" );
#if _HH_LOCALIZE_VOTESTRINGS
				strPtr[0] = "#str_04255";
				ClientStartVote(clientNum, VOTESTR_JUSTONE, 1, strPtr);
#else
				ClientStartVote( clientNum, common->GetLanguageDict()->GetString( "#str_04255" ) );
#endif
			} else {
				ServerStartVote( clientNum, voteIndex, "" );
#if _HH_LOCALIZE_VOTESTRINGS
				strPtr[0] = "#str_04254";
				ClientStartVote(clientNum, VOTESTR_JUSTONE, 1, strPtr);
#else
				ClientStartVote( clientNum, common->GetLanguageDict()->GetString( "#str_04254" ) );
#endif
			}
			break;
		default:
			gameLocal.ServerSendChatMessage( clientNum, "server", va( common->GetLanguageDict()->GetString( "#str_04297" ), (int)voteIndex ) );
			common->DPrintf( "client %d: unknown vote index %d\n", clientNum, voteIndex );
	}
}

/*
================
idMultiplayerGame::DisconnectClient
================
*/
void idMultiplayerGame::DisconnectClient( int clientNum ) {
	DisconnectVotingPlayer(clientNum); //HUMANHEAD rww

	if (gameLocal.gameType == GAME_TDM) { //HUMANHEAD rww
		CheckScoreChange(clientNum, playerState[clientNum].teamFragCount, 0, true);
	}
	else {
		CheckScoreChange(clientNum, playerState[clientNum].fragCount, 0, true);
	} //HUMANHEAD END
	if ( lastWinner == clientNum ) {
		lastWinner = -1;
	}
	UpdatePlayerRanks();
	CheckAbortGame();
}

/*
================
idMultiplayerGame::CheckAbortGame
================
*/
void idMultiplayerGame::CheckAbortGame( void ) {
	int i;
	if ( gameLocal.gameType == GAME_TOURNEY && gameState == WARMUP ) {
		// if a tourney player joined spectators, let someone else have his spot
		for ( i = 0; i < 2; i++ ) {
			if ( !gameLocal.entities[ currentTourneyPlayer[ i ] ] || static_cast< idPlayer * >( gameLocal.entities[ currentTourneyPlayer[ i ] ] )->spectating ) {
				currentTourneyPlayer[ i ] = -1;
			}
		}
	}
	// only checks for aborts -> game review below
	if ( gameState != COUNTDOWN && gameState != GAMEON && gameState != SUDDENDEATH ) {
		return;
	}
	switch ( gameLocal.gameType ) {
		case GAME_TOURNEY:
			for ( i = 0; i < 2; i++ ) {
				if ( !gameLocal.entities[ currentTourneyPlayer[ i ] ] || static_cast< idPlayer * >( gameLocal.entities[ currentTourneyPlayer[ i ] ] )->spectating ) {
					NewState( GAMEREVIEW );
					return;
				}
			}
			break;
		default:
			if ( !EnoughClientsToPlay() ) {
				NewState( GAMEREVIEW );
			}
			break;
	}
}

/*
================
idMultiplayerGame::WantKilled
================
*/
void idMultiplayerGame::WantKilled( int clientNum ) {
	idEntity *ent = gameLocal.entities[ clientNum ];
	if ( ent && ent->IsType( idPlayer::Type ) ) {
		static_cast<idPlayer *>( ent )->Kill( false, false );
	}
}

/*
================
idMultiplayerGame::MapRestart
================
*/
void idMultiplayerGame::MapRestart( void ) {
	int clientNum;

	assert( !gameLocal.isClient );
	if ( gameState != WARMUP ) {
		NewState( WARMUP );
		nextState = INACTIVE;
		nextStateSwitch = 0;
	}
	if ( g_balanceTDM.GetBool() && lastGameType != GAME_TDM && gameLocal.gameType == GAME_TDM ) {
		for ( clientNum = 0; clientNum < gameLocal.numClients; clientNum++ ) {
			if ( gameLocal.entities[ clientNum ] && gameLocal.entities[ clientNum ]->IsType( idPlayer::Type ) ) {
				if ( static_cast< idPlayer* >( gameLocal.entities[ clientNum ] )->BalanceTDM() ) {
					// core is in charge of syncing down userinfo changes
					// it will also call back game through SetUserInfo with the current info for update
					cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "updateUI %d\n", clientNum ) );
				}
			}
		}
	}
	lastGameType = gameLocal.gameType;
}

/*
================
idMultiplayerGame::SwitchToTeam
================
*/
void idMultiplayerGame::SwitchToTeam( int clientNum, int oldteam, int newteam ) {
	idEntity *ent;
	int i;

	assert( gameLocal.gameType == GAME_TDM );
	assert( oldteam != newteam );
	assert( !gameLocal.isClient );

	if ( !gameLocal.isClient && newteam >= 0 && IsInGame( clientNum ) ) {
		PrintMessageEvent( -1, MSG_JOINTEAM, clientNum, newteam );
	}
	// assign the right teamFragCount
	for( i = 0; i < gameLocal.numClients; i++ ) {
		if ( i == clientNum ) {
			continue;
		}
		ent = gameLocal.entities[ i ]; 
		if ( ent && ent->IsType( idPlayer::Type ) && static_cast< idPlayer * >(ent)->team == newteam ) {
			playerState[ clientNum ].teamFragCount = playerState[ i ].teamFragCount;
			break;
		}	
	}
	if ( i == gameLocal.numClients ) {
		// alone on this team
		playerState[ clientNum ].teamFragCount = 0;
	}
	if ( gameState == GAMEON && oldteam != -1 ) {
		// when changing teams during game, kill and respawn
		idPlayer *p = static_cast<idPlayer *>( gameLocal.entities[ clientNum ] );
		if ( p->IsInTeleport() ) {
			p->ServerSendEvent( idPlayer::EVENT_ABORT_TELEPORTER, NULL, false, -1 );
			p->SetPrivateCameraView( NULL );
		}
		p->Kill( true, true );
		CheckAbortGame();
	}
}

/*
================
idMultiplayerGame::ProcessChatMessage
================
*/
void idMultiplayerGame::ProcessChatMessage( int clientNum, bool team, const char *name, const char *text, const char *sound ) {
	idBitMsg	outMsg;
	byte		msgBuf[ 256 ];
	const char *prefix = NULL;
	int			send_to; // 0 - all, 1 - specs, 2 - team
	int			i;
	idEntity 	*ent;
	idPlayer	*p;
	idStr		prefixed_name;
#if HUMANHEAD	// HUMANHEAD pdm
	idStr		prefixColor = S_COLOR_DEFAULT;
#endif
	assert( !gameLocal.isClient );

	if ( clientNum >= 0 ) {
		p = static_cast< idPlayer * >( gameLocal.entities[ clientNum ] );
		if ( !( p && p->IsType( idPlayer::Type ) ) ) {
			return;
		}

		if ( p->spectating ) {
			// HUMANHEAD PCF pdm 05/29/06: Made use of localized strings
			prefix = common->GetLanguageDict()->GetString( "#str_04246" );	// "Spectating"
#if HUMANHEAD	// HUMANHEAD pdm
			prefixColor = S_COLOR_GREEN;
#endif
			if ( team || ( !g_spectatorChat.GetBool() && ( gameState == GAMEON || gameState == SUDDENDEATH ) ) ) {
				// to specs
				send_to = 1;
			} else {
				// to all
				send_to = 0;
			}
		} else if ( team ) {
			// HUMANHEAD PCF pdm 05/29/06: Made use of localized strings
			prefix = common->GetLanguageDict()->GetString( "#str_01991" );	//"team";
#if HUMANHEAD	// HUMANHEAD pdm
			prefixColor = (p->team==0) ? S_COLOR_RED : S_COLOR_BLUE;
#endif
			// to team
			send_to = 2;
		} else {
			// to all
			send_to = 0;
		}
	} else {
		p = NULL;
		send_to = 0;
	}
	// put the message together
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_CHAT );
	if ( prefix ) {
#if HUMANHEAD	// HUMANHEAD pdm
		prefixed_name = va( "%s(%s) %s", prefixColor.c_str(), prefix, name );
#else
		prefixed_name = va( "(%s) %s", prefix, name );
#endif
	} else {
		prefixed_name = name;
	}
	outMsg.WriteString( prefixed_name );
	outMsg.WriteString( text, -1, false );
	if ( !send_to ) {
		AddChatLine( "%s^0: %s\n", prefixed_name.c_str(), text );
		networkSystem->ServerSendReliableMessage( -1, outMsg );
		if ( sound ) {
			PlayGlobalSound( -1, SND_COUNT, sound );
		}
	} else {
		for ( i = 0; i < gameLocal.numClients; i++ ) {
			ent = gameLocal.entities[ i ]; 
			if ( !ent || !ent->IsType( idPlayer::Type ) ) {
				continue;
			}
			if ( send_to == 1 && static_cast< idPlayer * >( ent )->spectating ) {
				if ( sound ) {
					PlayGlobalSound( i, SND_COUNT, sound );
				}
				if ( i == gameLocal.localClientNum ) {
					AddChatLine( "%s^0: %s\n", prefixed_name.c_str(), text );
				} else {
					networkSystem->ServerSendReliableMessage( i, outMsg );
				}
			} else if ( send_to == 2 && static_cast< idPlayer * >( ent )->team == p->team ) {
				if ( sound ) {
					PlayGlobalSound( i, SND_COUNT, sound );
				}
				if ( i == gameLocal.localClientNum ) {
					AddChatLine( "%s^0: %s\n", prefixed_name.c_str(), text );
				} else {
					networkSystem->ServerSendReliableMessage( i, outMsg );
				}
			}
		}
	}
}

/*
================
idMultiplayerGame::Precache
================
*/
void idMultiplayerGame::Precache( void ) {
	int			i;

	if ( !gameLocal.isMultiplayer ) {
		return;
	}
#if HUMANHEAD	// HUMANHEAD pdm: removed hardcoded name, cached MP specific version
	gameLocal.FindEntityDefDict( GAME_PLAYERDEFNAME_MP, false );
#else
	gameLocal.FindEntityDefDict( "player_doommarine", false );;
#endif
	
	// skins
/* HUMANHEAD pdm: removed
	idStr str = cvarSystem->GetCVarString( "mod_validSkins" );
	idStr skin;
	while ( str.Length() ) {
		int n = str.Find( ";" );
		if ( n >= 0 ) {
			skin = str.Left( n );
			str = str.Right( str.Length() - n - 1 );
		} else {
			skin = str;
			str = "";
		}
		declManager->FindSkin( skin, false );
	}

	for ( i = 0; ui_skinArgs[ i ]; i++ ) {
		declManager->FindSkin( ui_skinArgs[ i ], false );
	}
*/
	// MP game sounds
	for ( i = 0; i < SND_COUNT; i++ ) {
		//HUMANHEAD rww - let's actually cache these sounds instead of just accessing the file.
		//idFile *f = fileSystem->OpenFileRead( GlobalSoundStrings[ i ] );
		//fileSystem->CloseFile( f );
		declManager->FindSound(GlobalSoundStrings[ i ]);
		//HUMANHEAD END
	}
	// MP guis. just make sure we hit all of them
	i = 0;
	while ( MPGuis[ i ] ) {
		uiManager->FindGui( MPGuis[ i ], true );
		i++;
	}
}

/*
================
idMultiplayerGame::ToggleSpectate
================
*/
void idMultiplayerGame::ToggleSpectate( void ) {
	bool spectating;
	assert( gameLocal.isClient || gameLocal.localClientNum == 0 );

	spectating = ( idStr::Icmp( cvarSystem->GetCVarString( "ui_spectate" ), "Spectate" ) == 0 );
	if ( spectating ) {
		// always allow toggling to play
		cvarSystem->SetCVarString( "ui_spectate", "Play" );
	} else {
		// only allow toggling to spectate if spectators are enabled.
		if ( gameLocal.serverInfo.GetBool( "si_spectators" ) ) {
			cvarSystem->SetCVarString( "ui_spectate", "Spectate" );
		} else {
			gameLocal.mpGame.AddChatLine( common->GetLanguageDict()->GetString( "#str_06747" ) );
		}
	}
}

/*
================
idMultiplayerGame::ToggleReady
================
*/
void idMultiplayerGame::ToggleReady( void ) {
	bool ready;
	assert( gameLocal.isClient || gameLocal.localClientNum == 0 );

	ready = ( idStr::Icmp( cvarSystem->GetCVarString( "ui_ready" ), "Ready" ) == 0 );
	if ( ready ) {
		cvarSystem->SetCVarString( "ui_ready", "Not Ready" );
	} else {
		cvarSystem->SetCVarString( "ui_ready", "Ready" );
	}
}

/*
================
idMultiplayerGame::ToggleTeam
================
*/
void idMultiplayerGame::ToggleTeam( void ) {
	bool team;
	assert( gameLocal.isClient || gameLocal.localClientNum == 0 );
	
	team = ( idStr::Icmp( cvarSystem->GetCVarString( "ui_team" ), "Red" ) == 0 );
	if ( team ) {
		cvarSystem->SetCVarString( "ui_team", "Blue" );
	} else {
		cvarSystem->SetCVarString( "ui_team", "Red" );
	}
}

/*
================
idMultiplayerGame::ToggleUserInfo
================
*/
void idMultiplayerGame::ThrottleUserInfo( void ) {
	int i;

	assert( gameLocal.localClientNum >= 0 );

	i = 0;
	while ( ThrottleVars[ i ] ) {
		if ( idStr::Icmp( gameLocal.userInfo[ gameLocal.localClientNum ].GetString( ThrottleVars[ i ] ),
			cvarSystem->GetCVarString( ThrottleVars[ i ] ) ) ) {
			if ( gameLocal.realClientTime < switchThrottle[ i ] ) {
				AddChatLine( common->GetLanguageDict()->GetString( "#str_04299" ), common->GetLanguageDict()->GetString( ThrottleVarsInEnglish[ i ] ), ( switchThrottle[ i ] - gameLocal.time ) / 1000 + 1 );
				cvarSystem->SetCVarString( ThrottleVars[ i ], gameLocal.userInfo[ gameLocal.localClientNum ].GetString( ThrottleVars[ i ] ) );
			} else {
				switchThrottle[ i ] = gameLocal.time + ThrottleDelay[ i ] * 1000;
			}
		}
		i++;
	}
}

/*
================
idMultiplayerGame::CanPlay
================
*/
bool idMultiplayerGame::CanPlay( idPlayer *p ) {
	return !p->wantSpectate && playerState[ p->entityNumber ].ingame;
}

/*
================
idMultiplayerGame::EnterGame
================
*/
void idMultiplayerGame::EnterGame( int clientNum ) {
	assert( !gameLocal.isClient );

	if ( !playerState[ clientNum ].ingame ) {
		playerState[ clientNum ].ingame = true;
		if ( gameLocal.isMultiplayer ) {
			// can't use PrintMessageEvent as clients don't know the nickname yet
			//HUMANHEAD PCF rww 05/10/06 - "fix" for server-localized join messages (this is dumb).
			//gameLocal.ServerSendChatMessage( -1, common->GetLanguageDict()->GetString( "#str_02047" ), va( common->GetLanguageDict()->GetString( "#str_07177" ), gameLocal.userInfo[ clientNum ].GetString( "ui_name" ) ) );
			const char *strs[2];
			strs[0] = "#str_07177";
			strs[1] = gameLocal.userInfo[ clientNum ].GetString( "ui_name" );
			gameLocal.ServerSendSpecialMessage(idGameLocal::SPECIALMSG_JOINED, -1, "#str_02047", 2, strs);
			//HUMANHEAD END
		}

		//HUMANHEAD rww
		playerState[clientNum].wins = persistentPlayerDict.GetInt(va("%s_wins", gameLocal.userInfo[clientNum].GetString( "ui_name" )));
		//HUMANHEAD END
	}
}

/*
================
idMultiplayerGame::WantRespawn
================
*/
bool idMultiplayerGame::WantRespawn( idPlayer *p ) {
	return p->forceRespawn && !p->wantSpectate && playerState[ p->entityNumber ].ingame;
}

/*
================
idMultiplayerGame::VoiceChat
================
*/
void idMultiplayerGame::VoiceChat_f( const idCmdArgs &args ) {
	gameLocal.mpGame.VoiceChat( args, false );
}

/*
================
idMultiplayerGame::VoiceChatTeam
================
*/
void idMultiplayerGame::VoiceChatTeam_f( const idCmdArgs &args ) {
	gameLocal.mpGame.VoiceChat( args, true );
}

/*
================
idMultiplayerGame::VoiceChat
================
*/
void idMultiplayerGame::VoiceChat( const idCmdArgs &args, bool team ) {
	idBitMsg			outMsg;
	byte				msgBuf[128];
	const char			*voc;
	const idDict		*spawnArgs;
	const idKeyValue	*keyval;
	int					index;

	if ( !gameLocal.isMultiplayer ) {
		common->Printf( "clientVoiceChat: only valid in multiplayer\n" );
		return;
	}
	if ( args.Argc() != 2 ) {
		common->Printf( "clientVoiceChat: bad args\n" );
		return;
	}
	// throttle
	if ( gameLocal.realClientTime < voiceChatThrottle ) {
		return;
	}

	voc = args.Argv( 1 );
#if HUMANHEAD	// HUMANHEAD pdm: removed hardcoded name
	spawnArgs = gameLocal.FindEntityDefDict( GAME_PLAYERDEFNAME_MP, false );
#else
	spawnArgs = gameLocal.FindEntityDefDict( "player_doommarine", false );
#endif
	keyval = spawnArgs->MatchPrefix( "snd_voc_", NULL );
	index = 0;
	while ( keyval ) {
		if ( !keyval->GetValue().Icmp( voc ) ) {
			break;
		}
		keyval = spawnArgs->MatchPrefix( "snd_voc_", keyval );
		index++;
	}
	if ( !keyval ) {
		common->Printf( "Voice command not found: %s\n", voc );
		return;
	}
	voiceChatThrottle = gameLocal.realClientTime + 1000;

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_VCHAT );
	outMsg.WriteLong( index );
	outMsg.WriteBits( team ? 1 : 0, 1 );
	networkSystem->ClientSendReliableMessage( outMsg );
}

/*
================
idMultiplayerGame::ProcessVoiceChat
================
*/
void idMultiplayerGame::ProcessVoiceChat( int clientNum, bool team, int index ) {
	const idDict		*spawnArgs;
	const idKeyValue	*keyval;
	idStr				name;
	idStr				snd_key;
	idStr				text_key;
	idPlayer			*p;

	p = static_cast< idPlayer * >( gameLocal.entities[ clientNum ] );
	if ( !( p && p->IsType( idPlayer::Type ) ) ) {
		return;
	}

	if ( p->spectating ) {
		return;
	}

	// lookup the sound def
#if HUMANHEAD	// HUMANHEAD pdm: removed hardcoded name
	spawnArgs = gameLocal.FindEntityDefDict( GAME_PLAYERDEFNAME_MP, false );
#else
	spawnArgs = gameLocal.FindEntityDefDict( "player_doommarine", false );
#endif
	keyval = spawnArgs->MatchPrefix( "snd_voc_", NULL );
	while ( index > 0 && keyval ) {
		keyval = spawnArgs->MatchPrefix( "snd_voc_", keyval );
		index--;
	}
	if ( !keyval ) {
		common->DPrintf( "ProcessVoiceChat: unknown chat index %d\n", index );
		return;
	}
	snd_key = keyval->GetKey();
	name = gameLocal.userInfo[ clientNum ].GetString( "ui_name" );
	sprintf( text_key, "txt_%s", snd_key.Right( snd_key.Length() - 4 ).c_str() );
	if ( team || gameState == COUNTDOWN || gameState == GAMEREVIEW ) {
		ProcessChatMessage( clientNum, team, name, spawnArgs->GetString( text_key ), spawnArgs->GetString( snd_key ) );
	} else {
		p->StartSound( snd_key, SND_CHANNEL_ANY, 0, true, NULL );
		ProcessChatMessage( clientNum, team, name, spawnArgs->GetString( text_key ), NULL );
	}
}

/*
================
idMultiplayerGame::ServerWriteInitialReliableMessages
================
*/
void idMultiplayerGame::ServerWriteInitialReliableMessages( int clientNum ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];
	int			i;
	idEntity	*ent;

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_STARTSTATE );
	// send the game state and start time
	outMsg.WriteByte( gameState );
	outMsg.WriteLong( matchStartedTime );
	outMsg.WriteShort( startFragLimit );
	// send the powerup states and the spectate states
	for( i = 0; i < gameLocal.numClients; i++ ) {
		ent = gameLocal.entities[ i ]; 
		if ( i != clientNum && ent && ent->IsType( idPlayer::Type ) ) {
			outMsg.WriteShort( i );
//			outMsg.WriteShort( static_cast< idPlayer * >( ent )->inventory.powerups );	// HUMANHEAD pdm: not used
			outMsg.WriteBits( static_cast< idPlayer * >( ent )->spectating, 1 );
		}
	}
	outMsg.WriteShort( MAX_CLIENTS );
	networkSystem->ServerSendReliableMessage( clientNum, outMsg );

	// we send SI in connectResponse messages, but it may have been modified already
	outMsg.BeginWriting( );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SERVERINFO );
	outMsg.WriteDeltaDict( gameLocal.serverInfo, NULL );
	networkSystem->ServerSendReliableMessage( clientNum, outMsg );

	// warmup time
	if ( gameState == COUNTDOWN ) {
		outMsg.BeginWriting();
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_WARMUPTIME );
		outMsg.WriteLong( warmupEndTime );
		networkSystem->ServerSendReliableMessage( clientNum, outMsg );
	}
}

/*
================
idMultiplayerGame::ClientReadStartState
================
*/
void idMultiplayerGame::ClientReadStartState( const idBitMsg &msg ) {
	int client;

	// read the state in preparation for reading snapshot updates
	gameState = (idMultiplayerGame::gameState_t)msg.ReadByte();
	matchStartedTime = msg.ReadLong( );
	startFragLimit = msg.ReadShort( );
	while ( ( client = msg.ReadShort() ) != MAX_CLIENTS ) {
		assert( gameLocal.entities[ client ] && gameLocal.entities[ client ]->IsType( idPlayer::Type ) );
/*	// HUMANHEAD pdm: not used
		powerup = msg.ReadShort();
		for ( i = 0; i < MAX_POWERUPS; i++ ) {
			if ( powerup & ( 1 << i ) ) {
				static_cast< idPlayer * >( gameLocal.entities[ client ] )->GivePowerUp( i, 0 );
			}
		}
*/
		bool spectate = ( msg.ReadBits( 1 ) != 0 );
		static_cast< idPlayer * >( gameLocal.entities[ client ] )->Spectate( spectate );
	}
}

/*
================
idMultiplayerGame::ClientReadWarmupTime
================
*/
void idMultiplayerGame::ClientReadWarmupTime( const idBitMsg &msg ) {
	warmupEndTime = msg.ReadLong();
}

