// RAVEN BEGIN
// ddynerman: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

idCVar g_spectatorChat( "g_spectatorChat", "0", CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "let spectators talk to everyone during game" );

const char *idMultiplayerGame::MPGuis[] = {
// RAVEN BEGIN
// bdube: use regular hud for now
	"guis/hud.gui",
// RAVEN END
	"guis/mpmain.gui",
	"guis/mpmsgmode.gui",
	"guis/netmenu.gui",
	"guis/mphud.gui",
	NULL
};

const char *idMultiplayerGame::ThrottleVars[] = {
	"ui_spectate",
	"ui_ready",
	"ui_team",
	NULL
};

const char *idMultiplayerGame::ThrottleVarsInEnglish[] = {
	"#str_106738",
	"#str_106737",
	"#str_101991",
	NULL
};

const int idMultiplayerGame::ThrottleDelay[] = {
	8,
	5,
	5
};

const char* idMultiplayerGame::teamNames[ TEAM_MAX ] = {
	"Marine",
	"Strogg"
};

idCVar gui_ui_name( "gui_ui_name", "", CVAR_GAME | CVAR_NOCHEAT, "copy-over cvar for ui_name" );

/*
================
ComparePlayerByScore
================
*/
int ComparePlayersByScore( const void* left, const void* right ) {
	return ((const rvPair<idPlayer*, int>*)right)->Second() - 
		((const rvPair<idPlayer*, int>*)left)->Second();
}

/*
================
CompareTeamByScore
================
*/
int CompareTeamsByScore( const void* left, const void* right ) {
	return ((const rvPair<int, int>*)right)->Second() - 
	 		((const rvPair<int, int>*)left)->Second();
}

/*
================
idMultiplayerGame::idMultiplayerGame
================
*/
idMultiplayerGame::idMultiplayerGame() {
// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	buyMenu = NULL;
// RITUAL END
	scoreBoard = NULL;
	statSummary = NULL;
	mainGui = NULL;
	mapList = NULL;
	msgmodeGui = NULL;
	defaultWinner = -1;
	deadZonePowerupCount = -1;
	marineScoreBarPulseAmount = 0.0f;
	stroggScoreBarPulseAmount = 0.0f;

	memset( lights, 0, sizeof( lights ) );
	memset( lightHandles, -1, sizeof( lightHandles ) );

	Clear();

	for( int i = 0; i < TEAM_MAX; i++ ) {
		teamScore[ i ] = 0;
		flagEntities[ i ] = NULL;
		teamDeadZoneScore[i] = 0;
	}

	for( int i = 0; i < TEAM_MAX; i++ ) 
	for( int j = 0; j < MAX_TEAM_POWERUPS; j++ ) {
		teamPowerups[i][j].powerup = 0;
		teamPowerups[i][j].time = 0;
		teamPowerups[i][j].endTime = 0;
		teamPowerups[i][j].update = false;
	}

	announcerSoundQueue.Clear();
	announcerPlayTime = 0;

	gameState = NULL;
	currentSoundOverride = false;

	rankTextPlayer = NULL;

	privatePlayers = 0;

	lastAnnouncerSound = AS_NUM_SOUNDS;
}

/*
================
idMultiplayerGame::Shutdown
================
*/
void idMultiplayerGame::Shutdown( void ) {

	Clear();
	statManager->Shutdown();

	if( gameState ) {
		delete gameState;
	}
	gameState = NULL;
}

/*
================
idMultiplayerGame::Reset
================
*/
void idMultiplayerGame::Reset() {
	Clear();
	assert( !scoreBoard && !mainGui && !mapList );

	mpBuyingManager.Reset();
	
// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	buyMenu = uiManager->FindGui( "guis/buymenu.gui", true, false, true );
	buyMenu->SetStateString( "field_credits", "$0.00");
	buyMenu->SetStateBool( "gameDraw", true );
// RITUAL END
	PACIFIER_UPDATE;
	scoreBoard = uiManager->FindGui( "guis/scoreboard.gui", true, false, true );

#ifdef _XENON
	statSummary = scoreBoard;
#else
	statSummary = uiManager->FindGui( "guis/summary.gui", true, false, true );
	statSummary->SetStateBool( "gameDraw", true );
#endif

	PACIFIER_UPDATE;

	mainGui = uiManager->FindGui( "guis/mpmain.gui", true, false, true );
	mapList = uiManager->AllocListGUI( );
	mapList->Config( mainGui, "mapList" );
	
	// set this GUI so that our Draw function is still called when it becomes the active/fullscreen GUI
	mainGui->SetStateBool( "gameDraw", true );
	mainGui->SetKeyBindingNames();
	mainGui->SetStateInt( "com_machineSpec", cvarSystem->GetCVarInteger( "com_machineSpec" ) );

//	SetMenuSkin();
	
	PACIFIER_UPDATE;
	msgmodeGui = uiManager->FindGui( "guis/mpmsgmode.gui", true, false, true );
	msgmodeGui->SetStateBool( "gameDraw", true );

	memset ( lights, 0, sizeof( lights ) );
	memset ( lightHandles, -1, sizeof( lightHandles ) );

	renderLight_t	*light;
	const char		*shader;

	light = &lights[ MPLIGHT_CTF_MARINE ];
	shader = "lights/mpCTFLight";
	if ( shader && *shader ) {
		light->axis.Identity();
		light->shader = declManager->FindMaterial( shader, false );
		light->lightRadius[0] = light->lightRadius[1] = light->lightRadius[2] = 64.0f;
		light->shaderParms[ SHADERPARM_RED ]	= 142.0f / 255.0f;
		light->shaderParms[ SHADERPARM_GREEN ]	= 190.0f / 255.0f;
		light->shaderParms[ SHADERPARM_BLUE ]	= 84.0f / 255.0f;
		light->shaderParms[ SHADERPARM_ALPHA ]	= 1.0f;
		light->detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
		light->pointLight = true;
		light->noShadows = true;
		light->noDynamicShadows = true;
		light->lightId = -MPLIGHT_CTF_MARINE;
		light->allowLightInViewID = 0;
	}

	light = &lights[ MPLIGHT_CTF_STROGG ];
	shader = "lights/mpCTFLight";
	if ( shader && *shader ) {
		light->axis.Identity();
		light->shader = declManager->FindMaterial( shader, false );
		light->lightRadius[0] = light->lightRadius[1] = light->lightRadius[2] = 64.0f;
		light->shaderParms[ SHADERPARM_RED ]	= 255.0f / 255.0f;
		light->shaderParms[ SHADERPARM_GREEN ]	= 153.0f / 255.0f;
		light->shaderParms[ SHADERPARM_BLUE ]	= 0.0f / 255.0f;
		light->shaderParms[ SHADERPARM_ALPHA ]	= 1.0f;
		light->detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
		light->pointLight = true;
		light->noShadows = true;
		light->noDynamicShadows = true;
		light->lightId = -MPLIGHT_CTF_STROGG;
		light->allowLightInViewID = 0;
	}

	light = &lights[ MPLIGHT_QUAD ];
	shader = "lights/mpCTFLight";
	if ( shader && *shader ) {
		light->axis.Identity();
		light->shader = declManager->FindMaterial( shader, false );
		light->lightRadius[0] = light->lightRadius[1] = light->lightRadius[2] = 64.0f;
		light->shaderParms[ SHADERPARM_RED ]	= 0.0f;
		light->shaderParms[ SHADERPARM_GREEN ]	= 128.0f / 255.0f;
		light->shaderParms[ SHADERPARM_BLUE ]	= 255.0f / 255.0f;
		light->shaderParms[ SHADERPARM_ALPHA ]	= 1.0f;
		light->detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
		light->pointLight = true;
		light->noShadows = true;
		light->noDynamicShadows = true;
		light->lightId = -MPLIGHT_CTF_STROGG;
		light->allowLightInViewID = 0;
	}

	light = &lights[ MPLIGHT_HASTE ];
	shader = "lights/mpCTFLight";
	if ( shader && *shader ) {
		light->axis.Identity();
		light->shader = declManager->FindMaterial( shader, false );
		light->lightRadius[0] = light->lightRadius[1] = light->lightRadius[2] = 64.0f;
		light->shaderParms[ SHADERPARM_RED ]	= 225.0f / 255.0f;
		light->shaderParms[ SHADERPARM_GREEN ]	= 255.0f / 255.0f;
		light->shaderParms[ SHADERPARM_BLUE ]	= 0.0f;
		light->shaderParms[ SHADERPARM_ALPHA ]	= 1.0f;
		light->detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
		light->pointLight = true;
		light->noShadows = true;
		light->noDynamicShadows = true;
		light->lightId = -MPLIGHT_CTF_STROGG;
		light->allowLightInViewID = 0;
	}

	light = &lights[ MPLIGHT_REGEN ];
	shader = "lights/mpCTFLight";
	if ( shader && *shader ) {
		light->axis.Identity();
		light->shader = declManager->FindMaterial( shader, false );
		light->lightRadius[0] = light->lightRadius[1] = light->lightRadius[2] = 64.0f;
		light->shaderParms[ SHADERPARM_RED ]	= 255.0f / 255.0f;
		light->shaderParms[ SHADERPARM_GREEN ]	= 0.0f;
		light->shaderParms[ SHADERPARM_BLUE ]	= 0.0f;
		light->shaderParms[ SHADERPARM_ALPHA ]	= 1.0f;
		light->detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
		light->pointLight = true;
		light->noShadows = true;
		light->noDynamicShadows = true;
		light->lightId = -MPLIGHT_CTF_STROGG;
		light->allowLightInViewID = 0;
	}

	PACIFIER_UPDATE;
	ClearGuis();

//asalmon: Need to refresh stats periodically if the player is looking at stats
	currentStatClient = -1;
	currentStatTeam = 0;

	iconManager->Shutdown();

	// update serverinfo
	UpdatePrivatePlayerCount();
	
	lastReadyToggleTime = -1;

	cvarSystem->SetCVarBool( "s_voiceChatTest", false );
}

/*
================
idMultiplayerGame::ServerClientConnect
================
*/
void idMultiplayerGame::ServerClientConnect( int clientNum ) {
	memset( &playerState[ clientNum ], 0, sizeof( playerState[ clientNum ] ) );
	statManager->ClientConnect( clientNum );
}

/*
================
idMultiplayerGame::SpawnPlayer
================
*/
void idMultiplayerGame::SpawnPlayer( int clientNum ) {

	TIME_THIS_SCOPE( __FUNCLINE__);

	idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ clientNum ] );

	if ( !p->IsFakeClient() ) {
		bool ingame = playerState[ clientNum ].ingame;
		// keep ingame to true if needed, that should only happen for local player

		memset( &playerState[ clientNum ], 0, sizeof( playerState[ clientNum ] ) );
		if ( !gameLocal.isClient ) {
			p->spawnedTime = gameLocal.time;
			//if ( gameLocal.IsTeamGame() ) {
			//	SwitchToTeam( clientNum, -1, p->team );
			//}
			playerState[ clientNum ].ingame = ingame;
		}
	}

	if ( p->IsLocalClient() && gameLocal.GetLocalPlayer() ) {
		tourneyGUI.SetupTourneyGUI( gameLocal.GetLocalPlayer()->mphud, scoreBoard );
	}

	lastVOAnnounce = 0;
}

/*
================
idMultiplayerGame::Clear
================
*/
void idMultiplayerGame::Clear() {
	
	int		i;
		
	pingUpdateTime = 0;
	vote = VOTE_NONE;
	voteTimeOut = 0;
	voteExecTime = 0;
	matchStartedTime = 0;
	memset( &playerState, 0 , sizeof( playerState ) );
	currentMenu = 0;
	bCurrentMenuMsg = false;
	nextMenu = 0;
	pureReady = false;
	scoreBoard = NULL;
	buyMenu = NULL;
	isBuyingAllowedRightNow = false;
	statSummary = NULL;
	mainGui = NULL;
	msgmodeGui = NULL;
	if ( mapList ) {
 		uiManager->FreeListGUI( mapList );
		mapList = NULL;
	}
	memset( &switchThrottle, 0, sizeof( switchThrottle ) );
	voiceChatThrottle = 0;

	voteValue.Clear();
	voteString.Clear();

	prevAnnouncerSnd = -1;

	localisedGametype.Clear();

	for( i = 0; i < MAX_CLIENTS; i++ ) {
		kickVoteMapNames[ i ].Clear();
	}

	voteMapDecls.Clear();
	voteMapsWaiting = 0;

	for ( i = 0; i < MPLIGHT_MAX; i ++ ) {
		FreeLight( i );
	}

	chatHistory.Clear();
	rconHistory.Clear();

	memset( rankedTeams, 0, sizeof( rvPair<int, int> ) * TEAM_MAX );

	if( gameState ) {
		gameState->Clear();
	}

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	rankedPlayers.SetAllocatorHeap(rvGetSysHeap(RV_HEAP_ID_MULTIPLE_FRAME));
	unrankedPlayers.SetAllocatorHeap(rvGetSysHeap(RV_HEAP_ID_MULTIPLE_FRAME));
	assaultPoints.SetAllocatorHeap(rvGetSysHeap(RV_HEAP_ID_MULTIPLE_FRAME));
#endif
// RAVEN END

	rankedPlayers.Clear();
	unrankedPlayers.Clear();
	assaultPoints.Clear();

	ClearAnnouncerSounds();

	rankTextPlayer = NULL;

	for ( i = 0; i < TEAM_MAX; i++ ) {
		flagEntities[ i ] = NULL;
	}
}

/*
================
idMultiplayerGame::ClearMap
================
*/
void idMultiplayerGame::ClearMap( void ) {
	assaultPoints.Clear();
	ClearAnnouncerSounds();
	announcerPlayTime = 0;
	powerupCount = 0;
	marineScoreBarPulseAmount = 0.0f;
	stroggScoreBarPulseAmount = 0.0f;
	prevAnnouncerSnd = -1;

	for( int i = 0; i < TEAM_MAX; i++ ) 
	for( int j = 0; j < MAX_TEAM_POWERUPS; j++ ) {
		teamPowerups[i][j].powerup = 0;
		teamPowerups[i][j].time = 0;
		teamPowerups[i][j].endTime = 0;
		teamPowerups[i][j].update = false;
	}

	// Dead Zone uses teamFragCount as the "player score"
	// so we need to clear it at the beginning of every round.
	if ( gameLocal.gameType == GAME_DEADZONE ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			playerState[i].teamFragCount = 0;
			playerState[i].deadZoneScore = 0;
		}
	}
}

/*
================
idMultiplayerGame::ClearGuis
================
*/
void idMultiplayerGame::ClearGuis() {
	int i;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		scoreBoard->SetStateString( va( "player%i",i+1 ), "" );
		scoreBoard->SetStateString( va( "player%i_score", i+1 ), "" );
		scoreBoard->SetStateString( va( "player%i_tdm_tscore", i+1 ), "" );
		scoreBoard->SetStateString( va( "player%i_tdm_score", i+1 ), "" );
		scoreBoard->SetStateString( va( "player%i_wins", i+1 ), "" );
		scoreBoard->SetStateString( va( "player%i_status", i+1 ), "" );
		scoreBoard->SetStateInt( va( "rank%i", i+1 ), 0 );
		scoreBoard->SetStateInt( "rank_self", 0 );

		idPlayer *player = static_cast<idPlayer *>( gameLocal.entities[ i ] );
		if ( !player || !player->hud ) {
			continue;
		}
		player->hud->SetStateString( va( "player%i",i+1 ), "" );
		player->hud->SetStateString( va( "player%i_score", i+1 ), "" );
		player->hud->SetStateString( va( "player%i_ready", i+1 ), "" );
		scoreBoard->SetStateInt( va( "rank%i", i+1 ), 0 );
		player->hud->SetStateInt( "rank_self", 0 );

		player->hud->SetStateInt( "team", TEAM_MARINE );
		player->hud->HandleNamedEvent( "flagReturn" );	
		player->hud->SetStateInt( "team", TEAM_STROGG );
		player->hud->HandleNamedEvent( "flagReturn" );	
	}	

	ClearVote();
}

/*
================
idMultiplayerGame::GetPlayerRank
Returns the player rank (0 best), returning the best rank in the case of a tie
================
*/
int idMultiplayerGame::GetPlayerRank( idPlayer* player, bool& isTied ) {
	int initialRank = -1;
	int rank = -1;

	for( int i = 0; i < rankedPlayers.Num(); i++ ) {
		if( rankedPlayers[ i ].First() == player ) {
			rank = i;
			initialRank = rank;
		}
	}
	
	if( rank == -1 ) {
		return rank;
	}

	if( rank > 0 ) {
		if( rankedPlayers[ rank - 1 ].Second() == rankedPlayers[ rank ].Second() ) {
			rank = rankedPlayers[ rank - 1 ].First()->GetRank();
		} else {
			rank = rankedPlayers[ rank - 1 ].First()->GetRank() + 1;
		}
	}

	// check for tie
	isTied = false;

	for( int i = rank - 1; i <= rank + 1; i++ ) {
		if( i < 0 || i >= rankedPlayers.Num() || rankedPlayers[ i ].First() == player ) {
			continue;
		}

		if( rankedPlayers[ i ].Second() == rankedPlayers[ initialRank ].Second() ) {
			isTied = true;
			break;
		}
	}

	return rank;
}

/*
================
idMultiplayerGame::UpdatePlayerRanks
================
*/
void idMultiplayerGame::UpdatePlayerRanks( playerRankMode_t rankMode ) {
	idEntity* ent = NULL;

	if( rankMode == PRM_AUTO ) {
		if( gameLocal.IsTeamGame() ) {
			rankMode = PRM_TEAM_SCORE_PLUS_SCORE;
		} else if ( gameLocal.gameType == GAME_TOURNEY ) {
			rankMode = PRM_WINS;
		} else {
			rankMode = PRM_SCORE;
		}
	}

	rankedPlayers.Clear();
	unrankedPlayers.Clear();

	for ( int i = 0; i < gameLocal.numClients; i++ ) {
		ent = gameLocal.entities[ i ];
		
		if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
			continue;
		}

		idPlayer* player = (idPlayer*)ent;
		
		if ( !CanPlay( player ) ) {
			unrankedPlayers.Append( player );
		} else {
			int rankingValue = 0;
			switch( rankMode ) {
				case PRM_SCORE: {
					rankingValue = GetScore( player );
					break;
				}
				case PRM_TEAM_SCORE: {
					rankingValue = GetTeamScore( player );
					break;
				}
				case PRM_TEAM_SCORE_PLUS_SCORE: {
					rankingValue = GetScore( player ) + GetTeamScore( player );
					break;
				}
				case PRM_WINS: {
					rankingValue = GetWins( player );
					break;
				}
				default: {
					gameLocal.Error( "idMultiplayerGame::UpdatePlayerRanks() - Bad ranking mode '%d'\n", rankMode );
				}
			}
			rankedPlayers.Append( rvPair<idPlayer*, int>(player, rankingValue ) );
		}
	}

	qsort( rankedPlayers.Ptr(), rankedPlayers.Num(), rankedPlayers.TypeSize(), ComparePlayersByScore );

	for( int i = 0; i < rankedPlayers.Num(); i++ ) {
		bool tied;
		rankedPlayers[ i ].First()->SetRank( GetPlayerRank( rankedPlayers[ i ].First(), tied ) );
	}

	for( int i = 0; i < unrankedPlayers.Num(); i++ ) {
		unrankedPlayers[ i ]->SetRank( -1 );
	}
}

/*
================
idMultiplayerGame::UpdateTeamRanks
================
*/
void idMultiplayerGame::UpdateTeamRanks( void ) {
	for ( int i = 0; i < TEAM_MAX; i++ ) {
		rankedTeams[ i ] = rvPair<int, int>( i, teamScore[ i ] );
	}

	qsort( rankedTeams, TEAM_MAX, sizeof( rvPair<int, int> ), CompareTeamsByScore );
}

/*
================
idMultiplayerGame::UpdateRankColor
================
*/
void idMultiplayerGame::UpdateRankColor( idUserInterface *gui, const char *mask, int i, const idVec3 &vec ) {
	for ( int j = 1; j < 4; j++ ) {
		gui->SetStateFloat( va( mask, i, j ), vec[ j - 1 ] );
	}
}

/*
================
idMultiplayerGame::CanCapture

Determines if the given flag can be captured in the given gamestate
================
*/
bool idMultiplayerGame::CanCapture( int team ) {
	// no AP's in one flag
	if( gameLocal.gameType == GAME_1F_CTF || gameLocal.gameType == GAME_ARENA_1F_CTF ) {
		return true;
	} else if( gameLocal.gameType != GAME_CTF && gameLocal.gameType != GAME_ARENA_CTF ) {
		return false; // no flag caps in none-CTF games
	}

	if ( !assaultPoints.Num() ) {
		return true;
	}

	// since other logic ensures AP's are captured in order, we just need to check the last AP before the enemy flag
	if ( team == TEAM_STROGG ) {
		// AP 0 is always next to the marine flag
		return ((rvCTFGameState*)gameState)->GetAPOwner( 0 ) == TEAM_STROGG;
	}
	if ( team == TEAM_MARINE ) {
		// the last AP is always the one next to the strogg flag
		return ((rvCTFGameState*)gameState)->GetAPOwner( assaultPoints.Num() - 1 ) == TEAM_MARINE;
	}

	return false;
}

void idMultiplayerGame::FlagCaptured( idPlayer *player ) {
	if( !gameLocal.isClient ) {
		AddTeamScore( player->team, 1 );
		AddPlayerTeamScore( player, 5 );
		
// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
		if( gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() )
		{
			float teamCashAward = (float) gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "teamCashAward_flagCapture", 0 );
			GiveCashToTeam( player->team, teamCashAward );

			float cashAward = (float) gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerCashAward_flagCapture", 0 );
			player->GiveCash( cashAward );
		}
// RITUAL END

		gameLocal.ClearForwardSpawns();
		
		for( int i = 0; i < assaultPoints.Num(); i++ ) {
			assaultPoints[ i ]->Reset();
			((rvCTFGameState*)gameState)->SetAPOwner( i, AS_NEUTRAL );
		}

		statManager->FlagCaptured( player, OpposingTeam( player->team ) );
		player->SetEmote( PE_CHEER );
	}
}

/*
================
idMultiplayerGame::SendDeathMessage
================
*/
void idMultiplayerGame::SendDeathMessage( idPlayer* attacker, idPlayer* victim, int methodOfDeath, bool quadKill ) { 
	if( !gameLocal.isClient ) {
		idBitMsg outMsg;
		byte msgBuf[1024];
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_DEATH );
		if( attacker ) {
			outMsg.WriteByte( attacker->entityNumber );
			outMsg.WriteBits( idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[ attacker->entityNumber ].fragCount ), ASYNC_PLAYER_FRAG_BITS );
		} else {
			outMsg.WriteByte( 255 );
		}
		
		if( victim ) {
			outMsg.WriteByte( victim->entityNumber );
			outMsg.WriteBits( idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[ victim->entityNumber ].fragCount ), ASYNC_PLAYER_FRAG_BITS );
		} else {
			outMsg.WriteByte( 255 );
		}
		
		outMsg.WriteByte( methodOfDeath );
		outMsg.WriteBits( quadKill, 1 );
		
		gameLocal.ServerSendInstanceReliableMessage( victim, -1, outMsg );	

		if( gameLocal.isListenServer && gameLocal.GetLocalPlayer() && victim && gameLocal.GetLocalPlayer()->GetInstance() == victim->GetInstance() )
		{
			// This is for listen servers, which won't get to ClientProcessReliableMessage
			ReceiveDeathMessage( attacker, attacker ? playerState[ attacker->entityNumber ].fragCount : -1, victim, victim ? playerState[ victim->entityNumber ].fragCount : -1, methodOfDeath, quadKill );
		}
	}
}

/*
================
idMultiplayerGame::ReceiveDeathMessage
================
*/
void idMultiplayerGame::ReceiveDeathMessage( idPlayer *attacker, int attackerScore, idPlayer *victim, int victimScore, int methodOfDeath, bool quadKill ) {
	idUserInterface *hud = gameLocal.GetLocalPlayer() ? gameLocal.GetLocalPlayer()->hud : NULL;

// RITUAL BEGIN
// squirrel: force buy menu open when you die
	//if( gameLocal.IsMultiplayer() && gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() && victim == gameLocal.GetLocalPlayer() )
	//{
	//	OpenLocalBuyMenu();
	//}
// RITUAL END

	const char* icon = "";

	// if methodOfDeath is in range [0, MAX_WEAPONS - 1] it refers to a specific weapon. MAX_WEAPONS refers to
	// a generic or unknown death (i.e. "Killer killed victim") and values above MAX_WEAPONS + 1 refer
	// to other non-weapon deaths (i.e. telefrags)

	// setup to either use weapon icons for a weapon death, or generic death icons
	if ( methodOfDeath < MAX_WEAPONS ) {
		icon = va( "w%02d", methodOfDeath );
	} else {
		icon = va( "dm%d", methodOfDeath - MAX_WEAPONS );
	}

	char* message = NULL;

	if ( gameLocal.IsTeamGame() ) {
		idStr	attackerStr( ( attacker ? gameLocal.userInfo[ attacker->entityNumber ].GetString( "ui_name" ) : "" ) );
		idStr	victimStr( ( victim ? gameLocal.userInfo[ victim->entityNumber ].GetString( "ui_name" ) : "" ) );

		attackerStr.RemoveEscapes();
		victimStr.RemoveEscapes();

		message = va ( "%s%s ^r%s^i%s %s%s",	(attacker ? (attacker->team ? S_COLOR_STROGG : S_COLOR_MARINE) : ""), 
							attackerStr.c_str(), 
							quadKill ? "^iqad" : "",
							icon,
							(victim ? (victim->team ? S_COLOR_STROGG : S_COLOR_MARINE) : ""), 
							victimStr.c_str() );
	} else {
		message = va ( "%s ^r%s^i%s %s", 	(attacker ? gameLocal.userInfo[ attacker->entityNumber ].GetString( "ui_name" ) : ""), 
										quadKill ? "^iqad" : "",
										icon,
										(victim ? gameLocal.userInfo[ victim->entityNumber ].GetString( "ui_name" ) : "") );
	}

	if( hud ) {
		hud->SetStateString ( "deathinfo", message );
		hud->HandleNamedEvent ( "addDeathLine" );
	}

	// echo to console
	gameLocal.Printf( gameLocal.GetLocalPlayer() ? gameLocal.GetLocalPlayer()->spawnArgs.GetString( va( "%s_text", icon ), "%s killed %s" ) : "%s killed %s", 
					(victim ? gameLocal.userInfo[ victim->entityNumber ].GetString( "ui_name" ) : "world"),
					(attacker ? gameLocal.userInfo[ attacker->entityNumber ].GetString( "ui_name" ) : "world") );
	gameLocal.Printf( "\n" );

	// display message on hud
	if( attacker && victim && (gameLocal.GetLocalPlayer() == attacker || gameLocal.GetLocalPlayer() == victim) && attacker != victim && methodOfDeath < MAX_WEAPONS ) {
		if( gameLocal.GetLocalPlayer() == attacker ) {
// RAVEN BEGIN
// rhummer: Added lang entries for "You fragged %s" and "You were fragged by %s"
			(gameLocal.GetLocalPlayer())->GUIFragNotice( va( common->GetLocalizedString( "#str_107295" ), gameLocal.userInfo[ victim->entityNumber ].GetString( "ui_name" ) ) );
		} else {
			(gameLocal.GetLocalPlayer())->GUIFragNotice( va( common->GetLocalizedString( "#str_107296" ), gameLocal.userInfo[ attacker->entityNumber ].GetString( "ui_name" ) ) );
// RAVEN END
		}

		if( gameLocal.gameType == GAME_DM ) {
			// print rank text next time after we update scores

			// stash the scores on the client so we can print accurate rank info
			if( gameLocal.isClient ) {
				if( victim ) {
					playerState[ victim->entityNumber ].fragCount = victimScore;
				}

				if( attacker ) {
					playerState[ attacker->entityNumber ].fragCount = attackerScore;
				}
			}

			if( victim && (gameLocal.GetLocalPlayer() == victim || (gameLocal.GetLocalPlayer()->spectating && gameLocal.GetLocalPlayer()->spectator == victim->entityNumber)) ) {
				rankTextPlayer = victim;
			}
			
			if( attacker && (gameLocal.GetLocalPlayer() == attacker || (gameLocal.GetLocalPlayer()->spectating && gameLocal.GetLocalPlayer()->spectator == attacker->entityNumber)) ) {
				rankTextPlayer = attacker;
			}
		}
	}
}


// ddynerman: Gametype specific scoreboard
/*
================
idMultiplayerGame::UpdateScoreboard
================
*/
void idMultiplayerGame::UpdateScoreboard( idUserInterface *scoreBoard ) {
	scoreBoard->SetStateInt( "gametype", gameLocal.gameType );

	//statManager->UpdateInGameHud( scoreBoard, true );

	if( gameLocal.IsTeamGame() ) {
		UpdateTeamScoreboard( scoreBoard );
	} else {
		UpdateDMScoreboard( scoreBoard );
	}

	return;
}

/*
================
idMultiplayerGame::UpdateDMScoreboard
================
*/
void idMultiplayerGame::UpdateDMScoreboard( idUserInterface *scoreBoard ) {
	idPlayer* player = gameLocal.GetLocalPlayer();
	int i;

	// bdube: mechanism for testing the scoreboard (populates it with fake names, pings, etc)
	if ( g_testScoreboard.GetInteger() > 0 ) {
		UpdateTestScoreboard ( scoreBoard );
		return;
	}

	if ( !player ) {
		return;
	}

	scoreBoard->SetStateString( "scores_sel_0", "-1" );
	scoreBoard->SetStateString( "spectator_scores_sel_0", "-1" );
	bool useReady = (gameLocal.serverInfo.GetBool( "si_useReady" ) && gameLocal.mpGame.GetGameState()->GetMPGameState() == WARMUP);
	if( gameLocal.gameType == GAME_DM ) {
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if( i < rankedPlayers.Num() ) {
				// ranked player
				idPlayer*	rankedPlayer	= rankedPlayers[ i ].First();
				int			rankedScore		= rankedPlayers[ i ].Second();

				if ( rankedPlayer == player ) {
					// highlight who we are
					scoreBoard->SetStateInt( "scores_sel_0", i );
				}

				scoreBoard->SetStateString ( 
					va("scores_item_%i", i), 
					va("%s\t%s\t%s\t%s\t%s\t%i\t%i\t%i\t",
					( useReady ? (rankedPlayer->IsReady() ? I_READY : I_NOT_READY) : "" ),					// ready icon
					( player->IsPlayerMuted( rankedPlayer ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ),		// mute icon
					( player->IsFriend( rankedPlayer ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),		// friend icon
					rankedPlayer->GetUserInfo()->GetString( "ui_name" ),								// name
					rankedPlayer->GetUserInfo()->GetString( "ui_clan" ),								// clan
					rankedScore,															 			// score
					GetPlayerTime( rankedPlayer ),														// time
					playerState[ rankedPlayer->entityNumber ].ping ) );									// ping
			} else {
				scoreBoard->SetStateString ( va("scores_item_%i", i), "" );
				scoreBoard->SetStateBool( va( "scores_item_%i_greyed", i ), false );
			}

			if( i < unrankedPlayers.Num() ) {
				if ( unrankedPlayers[ i ] == player ) {
					// highlight who we are
					scoreBoard->SetStateInt( "spectator_scores_sel_0", i );
				}

				scoreBoard->SetStateString ( 
					va("spectator_scores_item_%i", i), 
					va("%s\t%s\t%s\t%s\t%s\t%i\t%i\t", 
					( player->spectator && player->IsPlayerMuted( unrankedPlayers[ i ] ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ), // mute icon
					( player->IsFriend( unrankedPlayers[ i ] ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),	// friend icon
					unrankedPlayers[ i ]->GetUserInfo()->GetString( "ui_name" ),							// name
					unrankedPlayers[ i ]->GetUserInfo()->GetString( "ui_clan" ),							// clan
					"",																				 		// score
					GetPlayerTime( unrankedPlayers[ i ] ),													// time
					playerState[ unrankedPlayers[ i ]->entityNumber ].ping ) );								// ping
			} else {
				scoreBoard->SetStateString ( va("spectator_scores_item_%i", i), "" );
				scoreBoard->SetStateBool( va( "scores_item_%i_greyed", i ), false );
			}
		}
	} else if( gameLocal.gameType == GAME_TOURNEY ) {
		// loop through twice listing players who are playing, then players who have been eliminated
		int listIndex = 0;



		for ( i = 0; i < rankedPlayers.Num(); i++ ) {
			// ranked player
			idPlayer*	rankedPlayer	= rankedPlayers[ i ].First();
			int			rankedScore		= rankedPlayers[ i ].Second();

			if( rankedPlayer->GetTourneyStatus() == PTS_ELIMINATED ) {
				continue;
			}

			if ( rankedPlayer == player ) {
				// highlight who we are
				scoreBoard->SetStateInt( "scores_sel_0", listIndex );
			}

			scoreBoard->SetStateString ( 
				va("scores_item_%i", listIndex), 
				va("%s\t%s\t%s\t%s\t%s\t%i\t%i\t%s\t", 
				( useReady ? (rankedPlayer->IsReady() ? I_READY : I_NOT_READY) : "" ),					// ready icon
				( player->IsPlayerMuted( rankedPlayer ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ),		// mute icon
				( player->IsFriend( rankedPlayer ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),		// friend icon
				rankedPlayer->GetUserInfo()->GetString( "ui_name" ),								// name
				rankedPlayer->GetUserInfo()->GetString( "ui_clan" ),								// clan
				rankedScore,															 			// score
				playerState[ rankedPlayer->entityNumber ].ping,										// ping
				rankedPlayer->GetTextTourneyStatus() ) );											// tourney status
			
			scoreBoard->SetStateBool( va( "scores_item_%i_greyed", listIndex ), false );
			listIndex++;
		}

		for ( i = 0; i < rankedPlayers.Num(); i++ ) {
			// ranked player
			idPlayer*	rankedPlayer	= rankedPlayers[ i ].First();
			int			rankedScore		= rankedPlayers[ i ].Second();

			if( rankedPlayer->GetTourneyStatus() != PTS_ELIMINATED ) {
				continue;
			}

			if ( rankedPlayer == player ) {
				// highlight who we are
				scoreBoard->SetStateInt( "scores_sel_0", listIndex );
			}

			scoreBoard->SetStateString ( 
				va("scores_item_%i", listIndex), 
				va("%s\t%s\t%s\t%s\t%s\t%i\t%i\t%s\t", 
				( useReady ? (rankedPlayer->IsReady() ? I_READY : I_NOT_READY) : "" ),					// ready icon
				( player->IsPlayerMuted( rankedPlayer ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ),		// mute icon
				( player->IsFriend( rankedPlayer ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),		// friend icon
				rankedPlayer->GetUserInfo()->GetString( "ui_name" ),								// name
				rankedPlayer->GetUserInfo()->GetString( "ui_clan" ),								// clan
				rankedScore,															 			// score
				playerState[ rankedPlayer->entityNumber ].ping,										// ping
				rankedPlayer->GetTextTourneyStatus() ) );											// tourney status
			
			scoreBoard->SetStateBool( va( "scores_item_%i_greyed", listIndex ), true );
			listIndex++;
		}

		for( i = 0; i < MAX_CLIENTS; i++ ) {
			if( i < unrankedPlayers.Num() ) {
			if ( unrankedPlayers[ i ] == player ) {
				// highlight who we are
				scoreBoard->SetStateInt( "spectator_scores_sel_0", i );
			}

			scoreBoard->SetStateString ( 
				va("spectator_scores_item_%i", i), 
				va("%s\t%s\t%s\t%s\t%s\t%i\t%s\t", 
				( player->spectator && player->IsPlayerMuted( unrankedPlayers[ i ] ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ), // mute icon
				( player->IsFriend( unrankedPlayers[ i ] ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),	// friend icon
				unrankedPlayers[ i ]->GetUserInfo()->GetString( "ui_name" ),							// name
				unrankedPlayers[ i ]->GetUserInfo()->GetString( "ui_clan" ),							// clan
				"",																				 		// score
				playerState[ unrankedPlayers[ i ]->entityNumber ].ping,									// ping
				"" ) );	
			} else {
				scoreBoard->SetStateString( va( "spectator_scores_item_%i", i ), "" );
			}
		}

		for( i = listIndex; i < MAX_CLIENTS; i++ ) {
			scoreBoard->SetStateString( va( "scores_item_%i", i ), "" );
			scoreBoard->SetStateBool( va( "scores_item_%i_greyed", i ), false );
		}
	}

	scoreBoard->SetStateInt ( "num_players", idMath::ClampInt( 0, 16, rankedPlayers.Num() ) );
	scoreBoard->SetStateInt ( "num_spec_players", idMath::ClampInt( 0, 16, unrankedPlayers.Num() ) );
	scoreBoard->SetStateInt ( "num_total_players", idMath::ClampInt( 0, 16, rankedPlayers.Num() + unrankedPlayers.Num() ) );

	idStr serverAddress = networkSystem->GetServerAddress();

	scoreBoard->SetStateString( "servername", gameLocal.serverInfo.GetString( "si_name" ) );

	scoreBoard->SetStateString( "position_text", GetPlayerRankText( player ) );
	// shouchard:  added map name
	// mekberg: localized string
	const char *mapName = gameLocal.serverInfo.GetString( "si_map" );
	const idDict *mapDict = fileSystem->GetMapDecl( mapName );
	if ( mapDict ) {
		mapName = common->GetLocalizedString( mapDict->GetString( "name", mapName ) );
	}
	scoreBoard->SetStateString( "servermap", mapName );
	scoreBoard->SetStateString( "serverip",	serverAddress.c_str() );
	scoreBoard->SetStateString( "servergametype", GetLongGametypeName( gameLocal.serverInfo.GetString( "si_gameType" ) ) );
	scoreBoard->SetStateString( "servertimelimit", va( "%s: %d", common->GetLocalizedString( "#str_107659" ), gameLocal.serverInfo.GetInt( "si_timeLimit" ) ) );
	scoreBoard->SetStateString( "serverlimit", va( "%s: %d", common->GetLocalizedString( "#str_107660" ), gameLocal.serverInfo.GetInt( "si_fragLimit" ) ) );

	int timeLimit = gameLocal.serverInfo.GetInt( "si_timeLimit" );
	mpGameState_t state = gameState->GetMPGameState();

	bool inNonTimedState = (state == SUDDENDEATH) || (state == WARMUP) || (state == GAMEREVIEW);

	if( gameLocal.gameType == GAME_TOURNEY ) {
		if( gameLocal.serverInfo.GetInt( "si_fragLimit" ) == 1 ) {
			// stupid english plurals
			scoreBoard->SetStateString( "tourney_frag_count", va( common->GetLocalizedString( "#str_107712" ), gameLocal.serverInfo.GetInt( "si_fragLimit" ) ) );
		} else {
			scoreBoard->SetStateString( "tourney_frag_count", va( common->GetLocalizedString( "#str_107715" ), gameLocal.serverInfo.GetInt( "si_fragLimit" ) ) );
		}
		
		scoreBoard->SetStateString( "tourney_count", va( common->GetLocalizedString( "#str_107713" ), ((rvTourneyGameState*)gameState)->GetTourneyCount(), gameLocal.serverInfo.GetInt( "si_tourneyLimit" ) ) );
		if( player ) {
			inNonTimedState |= ((rvTourneyGameState*)gameState)->GetArena( player->GetArena() ).GetState() == AS_SUDDEN_DEATH;
		}
	}

	scoreBoard->SetStateString( "timeleft", GameTime() );

	scoreBoard->SetStateBool( "infinity", ( !timeLimit && state != COUNTDOWN ) || inNonTimedState );

	scoreBoard->StateChanged ( gameLocal.time );
	scoreBoard->Redraw( gameLocal.time );
}

/*
================
idMultiplayerGame::UpdateTeamScoreboard
================
*/

// only output 16 clients onto the scoreboard
#define SCOREBOARD_MAX_CLIENTS 16

void idMultiplayerGame::UpdateTeamScoreboard( idUserInterface *scoreBoard ) {
	idStr	gameinfo;
	int		numTeamEntries[ TEAM_MAX ];
	idPlayer* player = gameLocal.GetLocalPlayer();

	// bdube: mechanism for testing the scoreboard (populates it with fake names, pings, etc)
	if ( g_testScoreboard.GetInteger() > 0 ) {
		UpdateTestScoreboard ( scoreBoard );
		return;
	}

	if ( !player ) {
		return;
	}
	
	SIMDProcessor->Memset( numTeamEntries, 0, sizeof( int ) * TEAM_MAX );

	scoreBoard->SetStateString( "team_0_scores_sel_0", "-1" );
	scoreBoard->SetStateString( "team_1_scores_sel_0", "-1" );
	scoreBoard->SetStateString( "spectator_scores_sel_0", "-1" );
	bool useReady = (gameLocal.serverInfo.GetBool( "si_useReady" ) && gameLocal.mpGame.GetGameState()->GetMPGameState() == WARMUP);

	for ( int i = 0; i < SCOREBOARD_MAX_CLIENTS; i++ ) {
		if( i < rankedPlayers.Num() ) {
			// ranked player
			idPlayer*	rankedPlayer	= rankedPlayers[ i ].First();
			int			rankedScore		= rankedPlayers[ i ].Second();

			if ( rankedPlayer == player ) {
				// highlight who we are
				scoreBoard->SetStateInt( va("team_%i_scores_sel_0", rankedPlayer->team ), numTeamEntries[ rankedPlayer->team ] ); 
			}

// RAVEN BEGIN
// mekberg: redid this
			if ( gameLocal.gameType == GAME_TDM )
			{
				scoreBoard->SetStateString ( 
				va("team_%i_scores_item_%i", rankedPlayer->team, numTeamEntries[ rankedPlayer->team ]), 
				va("%s\t%s\t%s\t%s\t%s\t%i\t%i\t%i\t", 
				( useReady ? (rankedPlayer->IsReady() ? I_READY : I_NOT_READY) : "" ),			// ready icon
				( player->IsPlayerMuted( rankedPlayer ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ), // mute icon
				( player->IsFriend( rankedPlayer ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),	// friend icon
				rankedPlayer->GetUserInfo()->GetString( "ui_name" ),							// name
				rankedPlayer->GetUserInfo()->GetString( "ui_clan" ),							// clan
				rankedScore,										 							// score
				GetPlayerTime( rankedPlayer ),													// time
				playerState[ rankedPlayer->entityNumber ].ping ) );								// ping
				numTeamEntries[ rankedPlayer->team ]++;
			}
			//else if ( gameLocal.gameType == GAME_DEADZONE )
			//{
			//	// mekberg: made this check slightly more sane.
			//	const char* flagString = "";
			//	if ( rankedPlayer->PowerUpActive( rankedPlayer->team ? POWERUP_CTF_MARINEFLAG : POWERUP_CTF_STROGGFLAG ) ) {
			//		flagString = ( rankedPlayer->team ? I_FLAG_MARINE : I_FLAG_STROGG );
			//	} else if ( gameLocal.gameType == GAME_ARENA_CTF && player && rankedPlayer->team == player->team ) {
			//		flagString = rankedPlayer->GetArenaPowerupString( );
			//	}
			//	scoreBoard->SetStateString ( 
			//	va("team_%i_scores_item_%i", rankedPlayer->team, numTeamEntries[ rankedPlayer->team ]), 
			//	va("%s\t%s\t%s\t%s\t%s\t%s\t%.01f\t%i\t%i\t%i\t", 
			//	( useReady ? (rankedPlayer->IsReady() ? I_READY : I_NOT_READY) : "" ),			// ready icon
			//	( player->IsPlayerMuted( rankedPlayer ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ), // mute icon
			//	( player->IsFriend( rankedPlayer ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),	// friend icon
			//	flagString,																		// shouchard: twhitaker: updated steve's original flag system 
			//	rankedPlayer->GetUserInfo()->GetString( "ui_name" ),							// name
			//	rankedPlayer->GetUserInfo()->GetString( "ui_clan" ),							// clan
			//	rankedScore * 0.1f,										 						// score
			//	playerState[ rankedPlayer->entityNumber ].fragCount,							// kills
			//	GetPlayerTime( rankedPlayer ),													// time
			//	playerState[ rankedPlayer->entityNumber ].ping ) );								// ping
			//	numTeamEntries[ rankedPlayer->team ]++;
			//}
			else
			{
				// mekberg: made this check slightly more sane.
				const char* flagString = "";
				if ( rankedPlayer->PowerUpActive( rankedPlayer->team ? POWERUP_CTF_MARINEFLAG : POWERUP_CTF_STROGGFLAG ) ) {
					flagString = ( rankedPlayer->team ? I_FLAG_MARINE : I_FLAG_STROGG );
				} else if ( gameLocal.gameType == GAME_ARENA_CTF && player && rankedPlayer->team == player->team ) {
					flagString = rankedPlayer->GetArenaPowerupString( );
				}
				scoreBoard->SetStateString ( 
				va("team_%i_scores_item_%i", rankedPlayer->team, numTeamEntries[ rankedPlayer->team ]), 
				va("%s\t%s\t%s\t%s\t%s\t%s\t%i\t%i\t%i\t%i\t", 
				( useReady ? (rankedPlayer->IsReady() ? I_READY : I_NOT_READY) : "" ),			// ready icon
				( player->IsPlayerMuted( rankedPlayer ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ), // mute icon
				( player->IsFriend( rankedPlayer ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),	// friend icon
				flagString,																		// shouchard: twhitaker: updated steve's original flag system 
				rankedPlayer->GetUserInfo()->GetString( "ui_name" ),							// name
				rankedPlayer->GetUserInfo()->GetString( "ui_clan" ),							// clan
				rankedScore,										 							// score
				playerState[ rankedPlayer->entityNumber ].fragCount,							// kills
				GetPlayerTime( rankedPlayer ),													// time
				playerState[ rankedPlayer->entityNumber ].ping ) );								// ping
				numTeamEntries[ rankedPlayer->team ]++;
			}
// RAVEN END
		}

		if( i < unrankedPlayers.Num() ) {
			if ( unrankedPlayers[ i ] == player ) {
				// highlight who we are
				scoreBoard->SetStateInt( "spectator_scores_sel_0", i );
			}

// RAVEN BEGIN
// mekberg: redid this
			scoreBoard->SetStateString ( 
			va("spectator_scores_item_%i", i), 
			va("%s\t%s\t%s\t%s\t%s\t%i\t%i\t", 
			( player->spectating && player->IsPlayerMuted( unrankedPlayers[ i ] ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ), // mute icon
			( player->IsFriend( unrankedPlayers[ i ] ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),	// friend icon
			unrankedPlayers[ i ]->GetUserInfo()->GetString( "ui_name" ),							// name
			unrankedPlayers[ i ]->GetUserInfo()->GetString( "ui_clan" ),							// clan
			"",																						// score
			GetPlayerTime( unrankedPlayers[ i ] ),													// time
			playerState[ unrankedPlayers[ i ]->entityNumber ].ping ) );								// ping							// ping
// RAVEN END

		} else {
			scoreBoard->SetStateString ( va("spectator_scores_item_%i", i), "" );
		}
	}

	// clear unused space
	for( int k = 0; k < TEAM_MAX; k++ ) {
		for( int i = numTeamEntries[ k ]; i < MAX_CLIENTS; i++ ) {
			scoreBoard->SetStateString ( va("team_%i_scores_item_%i", k, i), "" );
		}
	}

	scoreBoard->SetStateInt ( "playerteam", player ? player->team : TEAM_NONE );

	scoreBoard->SetStateInt ( "strogg_score", teamScore[ TEAM_STROGG ] );
	scoreBoard->SetStateInt ( "marine_score", teamScore[ TEAM_MARINE ] );
	scoreBoard->SetStateInt ( "num_strogg_players", idMath::ClampInt( 0, 16, numTeamEntries[ TEAM_STROGG ] ) );
	scoreBoard->SetStateInt ( "num_marine_players", idMath::ClampInt( 0, 16, numTeamEntries[ TEAM_MARINE ] ) );
	scoreBoard->SetStateInt ( "num_players", idMath::ClampInt( 0, 16, numTeamEntries[ TEAM_STROGG ] + numTeamEntries[ TEAM_MARINE ] ) );
	scoreBoard->SetStateInt ( "num_total_players", idMath::ClampInt( 0, 16, numTeamEntries[ TEAM_STROGG ] + numTeamEntries[ TEAM_MARINE ] + unrankedPlayers.Num() ) );
	scoreBoard->SetStateInt ( "num_spec_players", idMath::ClampInt( 0, 16, unrankedPlayers.Num() ) );

	idStr serverAddress = networkSystem->GetServerAddress();

	scoreBoard->SetStateString( "servername", gameLocal.serverInfo.GetString( "si_name" ) );
// RAVEN BEGIN
// shouchard:  added map name
// mekberg: get localized string.
	const char *mapName = gameLocal.serverInfo.GetString( "si_map" );
	const idDict *mapDict = fileSystem->GetMapDecl( mapName );
	if ( mapDict ) {
		mapName = common->GetLocalizedString( mapDict->GetString( "name", mapName ) );
	}
	scoreBoard->SetStateString( "servermap", mapName );
// RAVEN END
	scoreBoard->SetStateString( "serverip",	serverAddress.c_str() );
	scoreBoard->SetStateString( "servergametype", GetLongGametypeName( gameLocal.serverInfo.GetString( "si_gameType" ) ) );
	scoreBoard->SetStateString( "servertimelimit", va( "%s: %d", common->GetLocalizedString( "#str_107659" ), gameLocal.serverInfo.GetInt( "si_timeLimit" ) ) );
	if ( gameLocal.IsFlagGameType() ) {
		scoreBoard->SetStateString( "serverlimit", va( "%s: %d", common->GetLocalizedString( "#str_107661" ), gameLocal.serverInfo.GetInt( "si_captureLimit" ) ) );
	} else if ( gameLocal.gameType == GAME_DEADZONE ) {
		scoreBoard->SetStateString( "serverlimit", va( "%s: %d", common->GetLocalizedString( "#str_122008" ), gameLocal.serverInfo.GetInt( "si_controlTime" ) ) );		
	} else {
		scoreBoard->SetStateString( "serverlimit", va( "%s: %d", common->GetLocalizedString( "#str_107660" ), gameLocal.serverInfo.GetInt( "si_fragLimit" ) ) );		
	}

	scoreBoard->SetStateString( "timeleft", GameTime() );

	int timeLimit = gameLocal.serverInfo.GetInt( "si_timeLimit" );
	mpGameState_t state = gameState->GetMPGameState();
	scoreBoard->SetStateBool( "infinity", ( !timeLimit && state != COUNTDOWN ) || state == WARMUP || state == GAMEREVIEW || state == SUDDENDEATH );

	scoreBoard->StateChanged( gameLocal.time );
	scoreBoard->Redraw( gameLocal.time );
}

/*
================
idMultiplayerGame::BuildSummaryListString
Returns a summary string for the specified player
================
*/
const char* idMultiplayerGame::BuildSummaryListString( idPlayer* player, int rankedScore ) {
	// track top 3 accuracies
	rvPlayerStat* stat = statManager->GetPlayerStat( player->entityNumber );
	idList<rvPair<int, float> > bestAccuracies;

	for( int j = 0; j < MAX_WEAPONS; j++ ) {
		// only consider weapons we fired more than a few shots
		if( stat->weaponShots[ j ] <= 10 ) {
			continue;
		}

		float accuracy = (float)stat->weaponHits[ j ] / (float)stat->weaponShots[ j ];
		bestAccuracies.Append( rvPair<int, float>( j, accuracy ) );
	}

	bestAccuracies.Sort( rvPair<int, float>::rvPairSecondCompareDirect );

	// hold upto 3 top weapons at 5 chars each
	idStr weaponString;
	for( int j = 0; j < 3; j++ ) {
		if( j >= bestAccuracies.Num() ) {
			continue;
		}

		weaponString += va( "^iw%02d", bestAccuracies[ j ].First() );
	}

	return 	va("%d. %s\t%s\t%d\t%s\t", 
			player->GetRank() + 1,
			player->GetUserInfo()->GetString( "ui_name" ),								// name
			player->GetUserInfo()->GetString( "ui_clan" ),								// clan
			rankedScore,																// score
			weaponString.c_str() );
}

/*
================
idMultiplayerGame::UpdateSummaryBoard
Shows top 10 players if local player is in top 10, otherwise shows top 9 and localplayer
================
*/
void idMultiplayerGame::UpdateSummaryBoard( idUserInterface *scoreBoard ) {
	idPlayer* player = gameLocal.GetLocalPlayer();

	if ( !player ) {
		return;
	}

	int playerIndex = -1;

	// update our ranks in case we call this the same frame it happens
	UpdatePlayerRanks();

	// highlight top 3 players
	idVec4 blueHighlight = idStr::ColorForIndex( C_COLOR_BLUE );
	idVec4 redHighlight = idStr::ColorForIndex( C_COLOR_RED );
	idVec4 yellowHighlight = idStr::ColorForIndex( C_COLOR_YELLOW );
	blueHighlight[ 3 ] = 0.15f;
	redHighlight[ 3 ] = 0.15f;
	yellowHighlight[ 3 ] = 0.15f;

	if( gameLocal.IsTeamGame() ) {
		scoreBoard->HandleNamedEvent( teamScore[ TEAM_MARINE ] > teamScore[ TEAM_STROGG ] ? "marine_wins" : "strogg_wins" );
		// summary is top 5 players on each team
		int lastHighIndices[ TEAM_MAX ];
		memset( lastHighIndices, 0, sizeof( int ) * TEAM_MAX );

		for( int i = 0; i < 5; i++ ) {
			scoreBoard->SetStateString ( va( "%s_item_%i", "summary_marine_names", i ), "" );
			scoreBoard->SetStateString ( va( "%s_item_%i", "summary_strogg_names", i ), "" );
		}

		for( int i = 0; i < TEAM_MAX; i++ ) {
			for( int j = 0; j < 5; j++ ) {
				idPlayer*	rankedPlayer	= NULL;
				int			rankedScore		= 0;
				int k;
				for( k = lastHighIndices[ i ]; k < rankedPlayers.Num(); k++ ) {
					if( rankedPlayers[ k ].First()->team == i ) {
						rankedPlayer = rankedPlayers[ k ].First();
						rankedScore = rankedPlayers[ k ].Second();
						break;	
					}
				}

				// no more teammates
				if( k >= rankedPlayers.Num() ) {
					break;
				}
				
				if( j == 4 && playerIndex == -1 && player->team == i ) {
					int z;
					for( z = 0; z < rankedPlayers.Num(); z++ ) {
						if( rankedPlayers[ z ].First() == player ) {
							rankedPlayer = player;
							rankedScore = rankedPlayers[ z ].Second();
							break;
						}
					}
				}
				
				if ( rankedPlayer == player ) {
					// highlight who we are
					playerIndex = j;
				}

				scoreBoard->SetStateString ( va( "%s_item_%i", i == TEAM_MARINE ? "summary_marine_names" : "summary_strogg_names", j ), BuildSummaryListString( rankedPlayer, rankedScore ) );

				lastHighIndices[ i ] = k + 1;
			}
		}

		if( playerIndex > 0 ) {
			if( player->team == TEAM_MARINE ) {
				scoreBoard->SetStateInt( "summary_marine_names_sel_0", playerIndex ); 	
				scoreBoard->SetStateInt( "summary_strogg_names_sel_0", -1 ); 	
			} else {
				scoreBoard->SetStateInt( "summary_strogg_names_sel_0", playerIndex ); 	
				scoreBoard->SetStateInt( "summary_marine_names_sel_0", -1 ); 	
			}
		} else {
			scoreBoard->SetStateInt( "summary_marine_names_sel_0", -1 ); 	
			scoreBoard->SetStateInt( "summary_strogg_names_sel_0", -1 ); 	
		}
	} else {
		for ( int i = 0; i < 10; i++ ) {

			// mekberg: delete old highlights
			scoreBoard->DeleteStateVar( va( "summary_names_item_%d_highlight", i ) );

			if( i < rankedPlayers.Num() ) {
				// ranked player
				idPlayer*	rankedPlayer	= rankedPlayers[ i ].First();
				int			rankedScore		= rankedPlayers[ i ].Second();

				if( i == 9 && playerIndex == -1 ) {
					// if the player is ranked, substitute them in
					int i;
					for( i = 0; i < rankedPlayers.Num(); i++ ) {
						if( rankedPlayers[ i ].First() == player ) {
							rankedPlayer = player;
							rankedScore = rankedPlayers[ i ].Second();
							break;
						}
					}
				}

				if ( rankedPlayer == player ) {
					// highlight who we are
					playerIndex = i;
				}
	
				scoreBoard->SetStateString ( va( "%s_item_%i", "summary_names", i ), BuildSummaryListString( rankedPlayer, rankedScore ) );

				if( rankedPlayer->GetRank() == 0 ) {
					scoreBoard->SetStateVec4( va( "summary_names_item_%d_highlight", i ), blueHighlight );
				} else if( rankedPlayer->GetRank() == 1 ) {
					scoreBoard->SetStateVec4( va( "summary_names_item_%d_highlight", i ), redHighlight );
				} else if( rankedPlayer->GetRank() == 2 ) {
					scoreBoard->SetStateVec4( va( "summary_names_item_%d_highlight", i ), yellowHighlight );
				}
			} else {
				scoreBoard->SetStateString ( va("summary_names_item_%i", i), "" );
			}
		}

		// highlight who we are (only if not ranked in the top 3)
		if( player->GetRank() >= 0 && player->GetRank() < 3 ) {
			scoreBoard->SetStateInt( "summary_names_sel_0", -1 ); 
		} else {
			scoreBoard->SetStateInt( "summary_names_sel_0", playerIndex ); 
		}
	} 


	scoreBoard->StateChanged ( gameLocal.time );
	scoreBoard->Redraw( gameLocal.time );
}

/*
================
idMultiplayerGame::UpdateTestScoreboard
================
*/
void idMultiplayerGame::UpdateTestScoreboard ( idUserInterface *scoreBoard ) {
	int i;

	gameLocal.random.SetSeed ( g_testScoreboard.GetInteger ( ) );

	if( gameLocal.IsTeamGame() ) {
		for ( i = 0; i < MAX_CLIENTS && i < g_testScoreboard.GetInteger ( ); i ++ ) {
			idStr name = va("Player %d", i + 1 );
			name = va("%s\t%i\t%i", name.c_str(), 
									 gameLocal.random.RandomInt ( 50 ), 
									 gameLocal.random.RandomInt ( 10 ));
			scoreBoard->SetStateString ( va("team_0_scores_item_%i", i), name );
		}
		while ( i < MAX_CLIENTS ) {
			scoreBoard->SetStateString ( va("team_0_scores_item_%i", i), "" );
			i++;
		}
		for ( i = 0; i < MAX_CLIENTS && i < g_testScoreboard.GetInteger ( ); i ++ ) {
			idStr name = va("Player %d", i + 1 );
			name = va("%s\t%i\t%i", name.c_str(), 
									 gameLocal.random.RandomInt ( 50 ), 
									 gameLocal.random.RandomInt ( 10 ));
			scoreBoard->SetStateString ( va("team_1_scores_item_%i", i), name );
		}
		while ( i < MAX_CLIENTS ) {
			scoreBoard->SetStateString ( va("team_1_scores_item_%i", i), "" );
			i++;
		}

		scoreBoard->SetStateInt ( "strogg_score", gameLocal.random.RandomInt ( 10 ) );
		scoreBoard->SetStateInt ( "marine_score", gameLocal.random.RandomInt ( 10 ) );
	} else {
		for ( i = 0; i < MAX_CLIENTS && i < g_testScoreboard.GetInteger ( ); i ++ ) {
			idStr name = va("Player %d", i + 1 );		

			scoreBoard->SetStateString ( 
				va("scores_item_%i", i), 
				va("%s\t%s\t%s\t%s\t%s\t%s\t%i\t%i\t%i\t", 
				( gameLocal.random.RandomInt() % 2 ? I_VOICE_DISABLED : I_VOICE_ENABLED ),		// mute icon
				( gameLocal.random.RandomInt() % 2 ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),		// friend icon
				"",																					// shouchard:  flag
				name.c_str(),								// name
				"Clan",								// clan
				"",																					// team score (unused in DM)
				gameLocal.random.RandomInt ( 50 ),								 										// score
				gameLocal.random.RandomInt ( 10 ),														// time
				gameLocal.random.RandomInt ( 300 ) + 20 ) );		


		}
		// clear remaining lines (empty slots)	
		while ( i < MAX_CLIENTS ) {
			scoreBoard->SetStateString ( va("scores_item_%i", i), "" );
			i++;
		}
	}

	scoreBoard->SetStateInt ( "num_marine_players", g_testScoreboard.GetInteger() );
	scoreBoard->SetStateInt ( "num_strogg_players", g_testScoreboard.GetInteger() );
	scoreBoard->SetStateInt ( "num_players", g_testScoreboard.GetInteger() );

	scoreBoard->SetStateInt( "rank_self", 2 );
	scoreBoard->SetStateInt ( "playercount", g_testScoreboard.GetInteger ( ) );

	scoreBoard->StateChanged ( gameLocal.time  );
	scoreBoard->SetStateString( "gameinfo", va( "Game Type:%s     Frag Limit:%i     Time Limit:%i", gameLocal.serverInfo.GetString( "si_gameType" ), gameLocal.serverInfo.GetInt( "si_fragLimit" ), gameLocal.serverInfo.GetInt( "si_timeLimit" ) ) );
	scoreBoard->Redraw( gameLocal.time );
}
// RAVEN END

/*
================
idMultiplayerGame::GameTime
================
*/
const char *idMultiplayerGame::GameTime( void ) {
	static char buff[32];
	int m, s, t, ms;

	bool inCountdown = false;

	ms = 0;
	if( gameState->GetMPGameState() == COUNTDOWN ) {
		inCountdown = true;
		ms = gameState->GetNextMPGameStateTime() - gameLocal.realClientTime;
	} else if( gameLocal.GetLocalPlayer() && gameLocal.gameType == GAME_TOURNEY && ((rvTourneyGameState*)gameState)->GetArena( gameLocal.GetLocalPlayer()->GetArena() ).GetState() == AS_WARMUP ) {
		inCountdown = true;
		ms = ((rvTourneyGameState*)gameState)->GetArena( gameLocal.GetLocalPlayer()->GetArena() ).GetNextStateTime() - gameLocal.realClientTime;
	}
	if ( inCountdown ) {
		s = ms / 1000 + 1;
		if ( ms <= 0 ) {
			// in tourney mode use a different string since warmups happen before each round
			// (not really before the overall game)
			idStr::snPrintf( buff, sizeof( buff ), "%s --", ( gameState->GetMPGameState() == COUNTDOWN && gameLocal.gameType == GAME_TOURNEY ) ? common->GetLocalizedString( "#str_107721" ) : common->GetLocalizedString( "#str_107706" ) );
		} else {
			idStr::snPrintf( buff, sizeof( buff ), "%s %i", (gameState->GetMPGameState() == COUNTDOWN && gameLocal.gameType == GAME_TOURNEY) ? common->GetLocalizedString( "#str_107721" ) : common->GetLocalizedString( "#str_107706" ), s );
		}
	} else {
		int timeLimit = gameLocal.serverInfo.GetInt( "si_timeLimit" );
		int startTime = matchStartedTime;
		if( gameLocal.gameType == GAME_TOURNEY ) {
			if( gameLocal.GetLocalPlayer() ) {
				startTime = ((rvTourneyGameState*)gameState)->GetArena( gameLocal.GetLocalPlayer()->GetArena() ).GetMatchStartTime();
			}
		}
		if ( timeLimit ) {
			ms = ( timeLimit * 60000 ) - ( gameLocal.time - startTime );
		} else {
			ms = gameLocal.time - startTime;
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
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
	if ( gameLocal.IsTeamGame() ) {
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
bool idMultiplayerGame::AllPlayersReady( idStr* reason ) {
	int			i, minClients, numClients;
	idEntity	*ent;
	idPlayer	*p;
	int			team[ 2 ];
	bool		notReady;

	notReady = false;
	
	minClients = Max( 2, gameLocal.serverInfo.GetInt( "si_minPlayers" ) );
	numClients = NumActualClients( false, &team[ 0 ] );
	if ( numClients < minClients ) { 
		if( reason ) {
			// stupid english plurals
			if( minClients == 2 ) {
				*reason = common->GetLocalizedString( "#str_107674" );
			} else {
				*reason = va( common->GetLocalizedString( "#str_107732" ), minClients - numClients );
			}
			
		}
	
		return false;
	}

	if ( gameLocal.IsTeamGame() ) {
		if ( !team[ 0 ] || !team[ 1 ] ) {
			if( reason ) {
				*reason = common->GetLocalizedString( "#str_107675" );
			}
	
			return false;
		}
	}

	for( i = 0; i < gameLocal.numClients; i++ ) {
		ent = gameLocal.entities[ i ];

		if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
			continue;
		}

		p = static_cast< idPlayer * >( ent );

		if ( CanPlay( p ) && !p->IsReady() ) {
			notReady = true;
		}
		team[ p->team ]++;
	}

	if( notReady ) {
		if( reason ) {
			if( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->IsReady() ) {
				// Tourney has a different hud layout, so needs a different "you are (not)ready" string
				if( gameLocal.gameType == GAME_TOURNEY ) {
					*reason = va( common->GetLocalizedString( "#str_110018" ), common->KeysFromBinding( "_impulse17" ) );
				} else {
					*reason = va( common->GetLocalizedString( "#str_107711" ), common->KeysFromBinding( "_impulse17" ) );
				}
			} else if( gameLocal.GetLocalPlayer() ) {
				if( gameLocal.gameType == GAME_TOURNEY ) {
					*reason = va( common->GetLocalizedString( "#str_110017" ), common->KeysFromBinding( "_impulse17" ) );				
				} else {
					*reason = va( common->GetLocalizedString( "#str_107710" ), common->KeysFromBinding( "_impulse17" ) );
				}
			}
		}
		return false;
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
	int fragLimit = gameLocal.serverInfo.GetInt( "si_fragLimit" );
	idPlayer *leader = NULL;

 	if ( fragLimit <= 0 ) {
 		return NULL; // fraglimit disabled
	}

	leader = FragLeader();
	if ( !leader ) {
		return NULL;
	}

	if ( playerState[ leader->entityNumber ].fragCount >= fragLimit ) {
		return leader;
	}

	return NULL;
}

/*
================
idMultiplayerGame::TimeLimitHit
================
*/
bool idMultiplayerGame::TimeLimitHit( void ) {	
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
return the current winner
NULL if even
relies on UpdatePlayerRanks() being called earlier in frame to sort players
================
*/
idPlayer* idMultiplayerGame::FragLeader( void ) {
	if( rankedPlayers.Num() < 2 ) {
		return NULL;
	}

	// mark leaders
	int i;
	int high = GetScore( rankedPlayers[ 0 ].First() );
	idPlayer* p;
	for ( i = 0; i < rankedPlayers.Num(); i++ ) {
		p = rankedPlayers[ i ].First();
		if ( !p ) {
			continue;
		}
		p->SetLeader( false );

		if ( !CanPlay( p ) ) {
			continue;
		}
		if ( gameLocal.gameType == GAME_TOURNEY ) {
			continue;
		}
		if ( p->spectating ) {
			continue;
		}

		if ( GetScore( p ) >= high ) {
			p->SetLeader( true );
		}
	}

	if( gameLocal.IsTeamGame() ) {
		// in a team game, find the first player not on the leader's team, and make sure they aren't tied
		int i = 0;
		while( i < rankedPlayers.Num() && rankedPlayers[ i ].First()->team == rankedPlayers[ 0 ].First()->team ) {
			i++;
		}
		if( i < rankedPlayers.Num() ) {
			if( GetScore( rankedPlayers[ i ].First()->entityNumber ) == GetScore( rankedPlayers[ 0 ].First()->entityNumber ) ) {
				return NULL;
			}
		}
	} else if( GetScore( rankedPlayers[ 0 ].First()->entityNumber ) == GetScore( rankedPlayers[ 1 ].First()->entityNumber ) ) {
		return NULL;
	}
	
	return rankedPlayers[ 0 ].First();
}

/*
================
idMultiplayerGame::PlayerDeath
================
*/
void idMultiplayerGame::PlayerDeath( idPlayer *dead, idPlayer *killer, int methodOfDeath ) {
	// don't do PrintMessageEvent
	assert( !gameLocal.isClient );

	if ( killer ) {
		if ( gameLocal.IsTeamGame() ) {
			if ( killer == dead || killer->team == dead->team ) {
				// suicide or teamkill

				// in flag games, we subtract suicides from team-score rather than player score, which is the true
				// kill count
				if( gameLocal.IsFlagGameType() ) {
					AddPlayerTeamScore( killer == dead ? dead : killer, -1 );
				} else {
					AddPlayerScore( killer == dead ? dead : killer, -1 );
				}

			} else {
				// mark a kill
				AddPlayerScore( killer, 1 );
			}
			
			// additional CTF points
			if( gameLocal.IsFlagGameType() ) {
				if( dead->PowerUpActive( killer->team ? POWERUP_CTF_STROGGFLAG : POWERUP_CTF_MARINEFLAG ) ) {
					AddPlayerTeamScore( killer, 2 );
				}
			}
			if( gameLocal.gameType == GAME_TDM ) {
				if ( killer == dead || killer->team == dead->team ) {
					// suicide or teamkill
					AddTeamScore( killer->team, -1 );
				} else {
					AddTeamScore( killer->team, 1 );
				}			
			}
		} else {
			// in tourney mode, we don't award points while in the waiting arena
			if( gameLocal.gameType != GAME_TOURNEY || ((rvTourneyGameState*)gameState)->GetArena( killer->GetArena() ).GetState() != AS_WARMUP ) {
				AddPlayerScore( killer, ( killer == dead ) ? -1 : 1 );
			}

			// in tourney mode, frags track performance over the entire level load, team score keeps track of
			// individual rounds
			if( gameLocal.gameType == GAME_TOURNEY ) {
				AddPlayerTeamScore( killer, ( killer == dead ) ? -1 : 1 );
			}
		}
	} else {
		// e.g. an environmental death

		// flag gametypes subtract points from teamscore, not playerscore
		if( gameLocal.IsFlagGameType() ) {
			AddPlayerTeamScore( dead, -1 );
		} else {
			AddPlayerScore( dead, -1 );
		}

		if( gameLocal.gameType == GAME_TOURNEY ) {
			AddPlayerTeamScore( dead, -1 );
		}
		if( gameLocal.gameType == GAME_TDM ) {
			AddTeamScore( dead->team, -1 );
		}
	}
	
	SendDeathMessage( killer, dead, methodOfDeath, killer ? killer->PowerUpActive( POWERUP_QUADDAMAGE ) : false );

	statManager->Kill( dead, killer, methodOfDeath );

// RAVEN BEGIN
// shouchard:  hack for CTF drop messages for listen servers
	if ( dead == gameLocal.GetLocalPlayer() && 
		dead->PowerUpActive( dead->team ? POWERUP_CTF_MARINEFLAG : POWERUP_CTF_STROGGFLAG ) ) {
		if ( dead->mphud ) {
			dead->mphud->SetStateString( "main_notice_text", common->GetLocalizedString( "#str_104420" ) );
			dead->mphud->HandleNamedEvent( "main_notice" );
		}
	}
// RAVEN END
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
	if ( ent && ent->IsType( idPlayer::GetClassType() ) ) {
		team = static_cast< idPlayer * >(ent)->team;
	} else {
		return;
	}

	idStr::snPrintf( data, len, "team=%d score=%ld tks=%ld", team, playerState[ clientNum ].fragCount, playerState[ clientNum ].teamFragCount );
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
idMultiplayerGame::ExecuteVote
the votes are checked for validity/relevance before they are started
we assume that they are still legit when reaching here
================
*/
void idMultiplayerGame::ExecuteVote( void ) {
	bool needRestart;
	ClearVote();
	switch ( vote ) {
		case VOTE_RESTART:
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "serverMapRestart\n");
			break;
		case VOTE_TIMELIMIT:
			si_timeLimit.SetInteger( atoi( voteValue ) );
			needRestart = gameLocal.NeedRestart();
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" " " __FILE__ " " __LINESTR__ );
			if ( needRestart ) {
				gameLocal.sessionCommand = "nextMap";
			}
			break;
		case VOTE_FRAGLIMIT:
			si_fragLimit.SetInteger( atoi( voteValue ) );
			needRestart = gameLocal.NeedRestart();
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" " " __FILE__ " " __LINESTR__ );
			if ( needRestart ) {
				gameLocal.sessionCommand = "nextMap";
			}
			break;
		case VOTE_GAMETYPE:
			cvarSystem->SetCVarString( "si_gametype", voteValue );
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "serverMapRestart\n");
			break;
		case VOTE_KICK:
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "kick %s", voteValue.c_str() ) );
			break;
		case VOTE_MAP:
			cvarSystem->SetCVarString( "si_map", voteValue );
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "serverMapRestart\n");
			break;
		case VOTE_BUYING:
			cvarSystem->SetCVarString( "si_isBuyingEnabled", voteValue );
			//cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" " " __FILE__ " " __LINESTR__ );
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "serverMapRestart\n");
			break;
// RAVEN BEGIN
// shouchard:  added capture limit
		case VOTE_CAPTURELIMIT:
			si_captureLimit.SetInteger( atoi( voteValue ) );
			gameLocal.sessionCommand = "nextMap";
			break;
		// todo:  round limit here (if we add it)
		case VOTE_AUTOBALANCE:
			si_autobalance.SetInteger( atoi( voteValue ) );
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" " " __FILE__ " " __LINESTR__ );
			break;
		case VOTE_MULTIFIELD:
			ExecutePackedVote();
			break;
// RAVEN END
		case VOTE_CONTROLTIME:
			si_controlTime.SetInteger( atoi( voteValue ) );
			gameLocal.sessionCommand = "nextMap";
			break;
		case VOTE_NEXTMAP:
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "serverNextMap\n" );
			break;
	}
}

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
			ClientUpdateVote( VOTE_RESET, 0, 0, currentVoteData );
			ExecuteVote();
			vote = VOTE_NONE;
		}
		return;
	}

	// count voting players
	numVoters = 0;

	for ( i = 0; i < gameLocal.numClients; i++ ) {
		idEntity *ent = gameLocal.entities[ i ];
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
			continue;
		}
		if ( playerState[ i ].vote != PLAYER_VOTE_NONE ) {
			numVoters++;
		}
	}
	if ( !numVoters ) {
		// abort
		vote = VOTE_NONE;
		ClientUpdateVote( VOTE_ABORTED, yesVotes, noVotes, currentVoteData );
		return;
	}
	if ( float(yesVotes) / numVoters > 0.5f ) {
		ClientUpdateVote( VOTE_PASSED, yesVotes, noVotes, currentVoteData );
		voteExecTime = gameLocal.time + 2000;
		return;
	}
	if ( gameLocal.time > voteTimeOut || float(noVotes) / numVoters >= 0.5f ) {
		ClientUpdateVote( VOTE_FAILED, yesVotes, noVotes, currentVoteData );
		vote = VOTE_NONE;
		return;
	}
}

// RAVEN BEGIN
// shouchard:  multifield voting here

/*
================
idMultiplayerGame::ClientCallPackedVote

The assumption is that the zero changes case has been handled above.
================
*/
void idMultiplayerGame::ClientCallPackedVote( const voteStruct_t &voteData ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	assert( 0 != voteData.m_fieldFlags );

	// send 
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_CALLPACKEDVOTE );
	outMsg.WriteShort( voteData.m_fieldFlags );
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_KICK ) ) {
		outMsg.WriteByte( idMath::ClampChar( voteData.m_kick ) );	
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_MAP ) ) {
		outMsg.WriteString( voteData.m_map.c_str() );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_GAMETYPE ) ) {
		outMsg.WriteByte( idMath::ClampChar( voteData.m_gameType ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TIMELIMIT ) ) {
		outMsg.WriteByte( idMath::ClampChar( voteData.m_timeLimit ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TOURNEYLIMIT ) ) {
		outMsg.WriteShort( idMath::ClampShort( voteData.m_tourneyLimit ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_CAPTURELIMIT ) ) {
		outMsg.WriteShort( idMath::ClampShort( voteData.m_captureLimit ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_FRAGLIMIT ) ) {
		outMsg.WriteShort( idMath::ClampShort( voteData.m_fragLimit ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_BUYING ) ) {
		outMsg.WriteShort( idMath::ClampShort( voteData.m_buying) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TEAMBALANCE ) ) {
		outMsg.WriteByte( idMath::ClampChar( voteData.m_teamBalance ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_CONTROLTIME ) ) {
		outMsg.WriteShort( idMath::ClampShort( voteData.m_controlTime ) );
	}
	networkSystem->ClientSendReliableMessage( outMsg );
}

/*
================
idMultiplayerGame::ServerCallPackedVote
================
*/
void idMultiplayerGame::ServerCallPackedVote( int clientNum, const idBitMsg &msg ) {
	voteStruct_t voteData;
	memset( &voteData, 0, sizeof( voteData ) );

	assert( -1 != clientNum );
	
	if( !gameLocal.serverInfo.GetBool( "si_allowVoting" ) ) {
		return;
	}

	// this is set to false if an invalid parameter is asked for-- time limit of -1, or frag limit of "jeff" or whatever.	
	// if it's a multivote, it may still be valid, but this value is only checked if there are no vote parameters changed. 
	bool validVote = true;

	// sanity checks - setup the vote
	if ( vote != VOTE_NONE ) {
		gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104273" );
		common->DPrintf( "client %d: called vote while voting already in progress - ignored\n", clientNum );
		return;
	}

	// flags (short)
	voteData.m_fieldFlags = msg.ReadShort();

	// clear any unallowed votes
	int disallowedVotes = gameLocal.serverInfo.GetInt( "si_voteFlags" );
	for( int i = 0; i < NUM_VOTES; i++ ) {
		if ( disallowedVotes & (1 << i) ) {
			voteData.m_fieldFlags &= ~(1 << i);
		}
	}

	// kick
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_KICK ) ) {
		voteData.m_kick = msg.ReadByte();
		if ( voteData.m_kick == gameLocal.localClientNum ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104257" );
			common->DPrintf( "client %d: called kick for the server host\n", clientNum );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_KICK );
		}
	}

	// map (string)
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_MAP ) ) {
		char buffer[128];
		msg.ReadString( buffer, sizeof( buffer ) );
		voteData.m_map = buffer;
		if ( 0 == idStr::Icmp( buffer, si_map.GetString() ) ) {
			//gameLocal.ServerSendChatMessage( clientNum, "server", "Selected map is the same as current map." );
			// mekberg: localized string
			const char* mapName = si_map.GetString();
			const idDict *mapDict = fileSystem->GetMapDecl( mapName );
			if ( mapDict ) {
				mapName = common->GetLocalizedString( mapDict->GetString( "name", mapName ) );
			}
			gameLocal.ServerSendChatMessage( clientNum, "server", va( common->GetLocalizedString( "#str_104295" ), mapName ) );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_MAP );
		}

		// because of addon pk4's clients may submit votes for maps the server doesn't have - audit here
		const idDict *mapDict = fileSystem->GetMapDecl( voteData.m_map.c_str() );
		if( !mapDict ) {
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_MAP );
			gameLocal.ServerSendChatMessage( clientNum, "server", "Selected map does not exist on the server" );
		}
	}

	// gametype
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_GAMETYPE ) ) {
		voteData.m_gameType = msg.ReadByte();
		const char *voteString = VoteGameTypeToString( voteData.m_gameType );
		if ( !idStr::Icmp( voteString, gameLocal.serverInfo.GetString( "si_gameType" ) ) ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104259" );
			common->DPrintf( "client %d: already at the voted Game Type\n", clientNum );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_GAMETYPE );
		}

		if ( voteData.m_fieldFlags & VOTEFLAG_MAP ) {
			const idDict *mapDict = fileSystem->GetMapDecl( voteData.m_map.c_str() );
			if ( !mapDict || !mapDict->GetInt( voteString ) ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "gametype incompatible with map" );
				validVote = false;
				voteData.m_fieldFlags &= ( ~VOTEFLAG_GAMETYPE );
			}
		}
	} else {
		if ( voteData.m_fieldFlags & VOTEFLAG_MAP ) {
			const idDict *mapDict = fileSystem->GetMapDecl( voteData.m_map.c_str() );
			if ( !mapDict || !mapDict->GetInt( si_gameType.GetString() ) ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "map incompatible with gametype" );
				validVote = false;
				voteData.m_fieldFlags &= ( ~VOTEFLAG_MAP );
			}
		}
	}

	// timelimit
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TIMELIMIT ) ) {
		voteData.m_timeLimit = msg.ReadByte();
		if ( voteData.m_timeLimit < si_timeLimit.GetMinValue() || voteData.m_timeLimit > si_timeLimit.GetMaxValue() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104269" );
			common->DPrintf( "client %d: timelimit value out of range for vote: %d\n", clientNum, voteData.m_timeLimit );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_TIMELIMIT );
		}
		if ( voteData.m_timeLimit == si_timeLimit.GetInteger() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104270" );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_TIMELIMIT );
		}
	}

	// tourneylimit
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TOURNEYLIMIT ) ) {
		voteData.m_tourneyLimit = msg.ReadShort();
		if ( voteData.m_tourneyLimit < si_tourneyLimit.GetMinValue() || voteData.m_tourneyLimit > si_tourneyLimit.GetMaxValue() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104261" );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_TOURNEYLIMIT );
		}
		if ( voteData.m_tourneyLimit == si_tourneyLimit.GetInteger() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104260" );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_TOURNEYLIMIT );
		}
	}

	// capture limit
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_CAPTURELIMIT ) ) {
		voteData.m_captureLimit = msg.ReadShort();
		if ( voteData.m_captureLimit < si_captureLimit.GetMinValue() || voteData.m_captureLimit > si_fragLimit.GetMaxValue() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104402" );
			common->DPrintf( "client %d: caplimit value out of range for vote: %d\n", clientNum, voteData.m_captureLimit );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_CAPTURELIMIT );
		}
		if ( voteData.m_captureLimit == si_captureLimit.GetInteger() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104401" );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_CAPTURELIMIT );
		}
	}

	// fraglimit
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_FRAGLIMIT ) ) {
		voteData.m_fragLimit = msg.ReadShort();
		if ( voteData.m_fragLimit < si_fragLimit.GetMinValue() || voteData.m_fragLimit > si_fragLimit.GetMaxValue() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104266" );
			common->DPrintf( "client %d: fraglimit value out of range for vote: %d\n", clientNum, voteData.m_fragLimit );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_FRAGLIMIT );
		}
		if ( voteData.m_fragLimit == si_fragLimit.GetInteger() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104267" );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_FRAGLIMIT );
		}
	}

	// spectators
/*	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_SPECTATORS ) ) {
		voteData.m_spectators = msg.ReadByte();
		if ( voteData.m_spectators == si_spectators.GetInteger() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104421" );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_SPECTATORS );
		}
	} */

	// buying
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_BUYING ) ) {
		voteData.m_buying = msg.ReadShort();
		if ( voteData.m_buying == si_isBuyingEnabled.GetInteger() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_122013" );
			validVote = false;
			voteData.m_buying &= ( ~VOTEFLAG_BUYING );
		}
	}

	// autobalance teams
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TEAMBALANCE ) ) {
		voteData.m_teamBalance = msg.ReadByte();
		if ( voteData.m_teamBalance == si_autobalance.GetInteger() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104403" );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_TEAMBALANCE );
		}
	}

	// control time
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_CONTROLTIME ) ) {
		voteData.m_controlTime = msg.ReadShort();
		if ( voteData.m_controlTime == si_controlTime.GetInteger() ) {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_122017" );
			validVote = false;
			voteData.m_fieldFlags &= ( ~VOTEFLAG_CONTROLTIME );
		}
	}

	// check for no changes at all
	if ( 0 == voteData.m_fieldFlags ) {
		// If the vote was called empty, announce there were no valid changes. Otherwise, say nothing, there's already been a warning message.
		if( validVote )	{
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104400" );
		}
		return;
	}

	ServerStartPackedVote( clientNum, voteData );
	ClientStartPackedVote( clientNum, voteData );
}

/*
================
idMultiplayerGame::ClientStartPackedVote
================
*/
void idMultiplayerGame::ClientStartPackedVote( int clientNum, const voteStruct_t &voteData ) {
	idUserInterface * mpHud = gameLocal.GetLocalPlayer() ? gameLocal.GetLocalPlayer()->mphud : NULL;

	assert( 0 != voteData.m_fieldFlags );

	if ( !gameLocal.isListenServer && !gameLocal.isClient ) {
		return;
	}

	// "%s has called a vote!"
	AddChatLine( va( common->GetLocalizedString( "#str_104279" ), gameLocal.userInfo[ clientNum ].GetString( "ui_name" ) ) );

	// display the vote called text on the hud and play an announcer sound
	if ( mpHud ) {
		mpHud->SetStateInt( "voteNotice", 1 );
	}
	ScheduleAnnouncerSound( AS_GENERAL_VOTE_NOW, gameLocal.time );

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
	}

	currentVoteData = voteData;

	// push data to the interface
	if ( mpHud && mainGui ) {
		int voteLineCount = 1;
		int menuVoteLineCount = 0;
		bool kickActive = false;
		bool maxWindows = false;
		idStr yesKey = common->KeysFromBinding("_impulse28");

		mainGui->SetStateInt( "vote_going", 1 );

		//dynamic vote yes/no box
		mpHud->SetStateString( "voteNoticeText", va( common->GetLocalizedString( "#str_107242" ), yesKey.c_str(), common->KeysFromBinding("_impulse29") ));

		// kick should always be the highest one
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_KICK ) ) {
			// mpGui here, not mpHud
			//mpHud->SetStateString( "vote_data0", va( common->GetLocalizedString( "#str_104422" ), player->GetName() );
			kickActive = true;
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ), 
				va( common->GetLocalizedString( "#str_104422" ), gameLocal.userInfo[ currentVoteData.m_kick ].GetString( "ui_name" ) ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ), 
				va( common->GetLocalizedString( "#str_104422" ), gameLocal.userInfo[ currentVoteData.m_kick ].GetString( "ui_name" ) ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_RESTART ) ) {
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ), 
				common->GetLocalizedString( "#str_104423" ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ), 
				common->GetLocalizedString( "#str_104423" ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_BUYING ) ) {
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ), 
				va( common->GetLocalizedString( "#str_122011" ), currentVoteData.m_buying ? common->GetLocalizedString( "#str_104341" ) : common->GetLocalizedString( "#str_104342" ) ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ), 
				va( common->GetLocalizedString( "#str_122011" ), currentVoteData.m_buying  ? common->GetLocalizedString( "#str_104341" ) : common->GetLocalizedString( "#str_104342" ) ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_TEAMBALANCE ) ) {
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ),
				va( common->GetLocalizedString( "#str_104427" ), currentVoteData.m_teamBalance ? common->GetLocalizedString( "#str_104341" ) : common->GetLocalizedString( "#str_104342" ) ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ),
				va( common->GetLocalizedString( "#str_104427" ), currentVoteData.m_teamBalance ? common->GetLocalizedString( "#str_104341" ) : common->GetLocalizedString( "#str_104342" ) ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_CONTROLTIME) ) {
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ),
				va( common->GetLocalizedString( "#str_122009" ), currentVoteData.m_controlTime ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ),
				va( common->GetLocalizedString( "#str_122009" ), currentVoteData.m_controlTime ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_SHUFFLE ) ) {
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ),
				common->GetLocalizedString( "#str_110010" ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ),
				common->GetLocalizedString( "#str_110010" ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_MAP ) ) {

			const char *mapName = currentVoteData.m_map.c_str();
			const idDict *mapDict = fileSystem->GetMapDecl( mapName );
			if ( mapDict ) {
				mapName = common->GetLocalizedString( mapDict->GetString( "name", mapName ) );
			}
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ),
				va( common->GetLocalizedString( "#str_104429" ), mapName ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ),
				va( common->GetLocalizedString( "#str_104429" ), mapName ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_GAMETYPE ) ) {
			const char *gameTypeString = common->GetLocalizedString( "#str_110011" );
			switch( currentVoteData.m_gameType ) {
				case VOTE_GAMETYPE_TOURNEY:
					gameTypeString = common->GetLocalizedString( "#str_110012" );
					break;
				case VOTE_GAMETYPE_TDM:
					gameTypeString = common->GetLocalizedString( "#str_110013" );
					break;
				case VOTE_GAMETYPE_CTF:
					gameTypeString = common->GetLocalizedString( "#str_110014" );
					break;
				case VOTE_GAMETYPE_ARENA_CTF:
					gameTypeString = common->GetLocalizedString( "#str_110015" );
					break;
				case VOTE_GAMETYPE_DEADZONE:
					gameTypeString = "DeadZone";
					break;
				case VOTE_GAMETYPE_DM:
				default:
					gameTypeString = common->GetLocalizedString( "#str_110011" );
					break;
			}
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ),
				va( common->GetLocalizedString( "#str_104430" ), gameTypeString ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ),
				va( common->GetLocalizedString( "#str_104430" ), gameTypeString ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_TIMELIMIT ) ) {
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ),
				va( common->GetLocalizedString( "#str_104431" ), currentVoteData.m_timeLimit ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ),
				va( common->GetLocalizedString( "#str_104431" ), currentVoteData.m_timeLimit ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_TOURNEYLIMIT ) ) {
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ),
				va( common->GetLocalizedString( "#str_104432" ), currentVoteData.m_tourneyLimit ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ),
				va( common->GetLocalizedString( "#str_104432" ), currentVoteData.m_tourneyLimit ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_CAPTURELIMIT ) ) {
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ),
				va( common->GetLocalizedString( "#str_104433" ), currentVoteData.m_captureLimit ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ),
				va( common->GetLocalizedString( "#str_104433" ), currentVoteData.m_captureLimit ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}
		if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_FRAGLIMIT ) ) {
			mpHud->SetStateString( va( "voteInfo_%d", voteLineCount ),
				va( common->GetLocalizedString( "#str_104434" ), currentVoteData.m_fragLimit ) );

			mainGui->SetStateString( va( "voteData_item_%d", menuVoteLineCount ),
				va( common->GetLocalizedString( "#str_104434" ), currentVoteData.m_fragLimit ) );

			voteLineCount++;
			menuVoteLineCount++;
			if( voteLineCount == 7)	{
				voteLineCount = 6;
				maxWindows = true;
			}
		}

		//jshep: max of 7 windows and the 7th is always "..."
		if( maxWindows )	{
			mpHud->SetStateString( "voteInfo_7", "..." );
		}

		mainGui->DeleteStateVar( va( "voteData_item_%d", menuVoteLineCount ) );
		mainGui->SetStateInt( "vote_going", 1 );
		mainGui->SetStateString( "voteCount", va( common->GetLocalizedString( "#str_104435" ), yesVotes, noVotes ) );
	}

	ClientUpdateVote( VOTE_UPDATE, yesVotes, noVotes, currentVoteData );
}

/*
================
idMultiplayerGame::ServerStartPackedVote
================
*/
void idMultiplayerGame::ServerStartPackedVote( int clientNum, const voteStruct_t &voteData ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	assert( vote == VOTE_NONE );

	if ( !gameLocal.isServer ) {
		return;
	}

	// #13705: clients passing a vote during server restart could abuse the voting system into passing the vote right away after the new map loads
	if ( !playerState[ clientNum ].ingame ) {
		common->Printf( "ignore vote called by client %d: not in game\n", clientNum );
		return;
	}

	// setup
	yesVotes = 1;
	noVotes = 0;
	vote = VOTE_MULTIFIELD;
	currentVoteData = voteData;
	voteTimeOut = gameLocal.time + 30000;	// 30 seconds?  might need to be longer because it requires fiddling with the GUI
	// mark players allowed to vote - only current ingame players, players joining during vote will be ignored
	for ( int i = 0; i < gameLocal.numClients; i++ ) {
		if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idPlayer::GetClassType() ) ) {
			playerState[ i ].vote = ( i == clientNum ) ? PLAYER_VOTE_YES : PLAYER_VOTE_WAIT;
		} else {
			playerState[i].vote = PLAYER_VOTE_NONE;
		}
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_STARTPACKEDVOTE );
	outMsg.WriteByte( clientNum );
	outMsg.WriteShort( voteData.m_fieldFlags );
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_KICK ) ) {
		outMsg.WriteByte( idMath::ClampChar( voteData.m_kick ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_MAP ) ) {
		outMsg.WriteString( voteData.m_map.c_str() );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_GAMETYPE ) ) {
		outMsg.WriteByte( idMath::ClampChar( voteData.m_gameType ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TIMELIMIT ) ) {
		outMsg.WriteByte( idMath::ClampChar( voteData.m_timeLimit ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_FRAGLIMIT ) ) {
		outMsg.WriteShort( idMath::ClampShort( voteData.m_fragLimit ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TOURNEYLIMIT ) ) {
		outMsg.WriteShort( idMath::ClampShort( voteData.m_tourneyLimit ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_CAPTURELIMIT ) ) {
		outMsg.WriteShort( idMath::ClampShort( voteData.m_captureLimit ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_BUYING ) ) {
		outMsg.WriteShort( idMath::ClampShort( voteData.m_buying ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TEAMBALANCE ) ) {
		outMsg.WriteByte( idMath::ClampChar( voteData.m_teamBalance ) );
	}
	if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_CONTROLTIME ) ) {
		outMsg.WriteShort( idMath::ClampShort( voteData.m_controlTime ) );
	}
	networkSystem->ServerSendReliableMessage( -1, outMsg );
}

/*
================
idMultiplayerGame::ExecutePackedVote
================
*/
void idMultiplayerGame::ExecutePackedVote( void ) {
	assert( VOTE_MULTIFIELD == vote );

	if ( 0 == currentVoteData.m_fieldFlags ) {
		return;
	}

	bool needRestart = false;
	bool needNextMap = false;
	bool needRescanSI = false;

	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_RESTART ) ) {
		needRestart = true;
	}
	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_BUYING ) ) {
		si_isBuyingEnabled.SetInteger( currentVoteData.m_buying );
		needRescanSI = true;
		needRestart = true;
	}
	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_TEAMBALANCE ) ) {
		si_autobalance.SetInteger( currentVoteData.m_teamBalance );
		needRescanSI = true;
	}
	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_CONTROLTIME ) ) {
		si_controlTime.SetInteger( currentVoteData.m_controlTime );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" );
	}
	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_SHUFFLE ) ) {
		ShuffleTeams();
	}
	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_KICK ) ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "kick %d", currentVoteData.m_kick ) );
	}
	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_MAP ) ) {
		si_map.SetString( currentVoteData.m_map.c_str() );
		needNextMap = true;
	}
	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_GAMETYPE ) ) {
		const char *gameTypeString = VoteGameTypeToString( currentVoteData.m_gameType );
		//jshepard: Currently the DM gametypes can be played on any map. The other gametypes require specially configured maps.
		//if further gametypes are added that can be played on any map, don't set the "runPickMap" flag.
		bool runPickMap = (idStr::Cmp( gameTypeString, "DM" ) != 0) ? true : false;

		si_gameType.SetString( gameTypeString );
		//jshepard: run a pick map here in case the packed vote is trying to pick the wrong map type.
		//PickMap returns true if the map has changed (requiring a nextMap call)
		if( runPickMap )	{
			if( PickMap( gameTypeString ) )	{
				needNextMap = true;
			} else {
				needRestart = true;
			}

			const idDict *mapDict = fileSystem->GetMapDecl( si_map.GetString() );
			if ( !mapDict || !mapDict->GetInt( gameTypeString ) ) {
				gameLocal.Warning( "server voted to gametype with no maps; resetting gametype to DM." );
				si_gameType.SetString( "DM" );
				needNextMap = false;
				needRestart = true;
			}
		} else	{
			needRestart = true;
		}
	}
	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_TIMELIMIT ) ) {
		si_timeLimit.SetInteger( currentVoteData.m_timeLimit );
		needRescanSI = true;
	}
	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_TOURNEYLIMIT ) ) {
		si_tourneyLimit.SetInteger( currentVoteData.m_tourneyLimit );
		needRescanSI = true;
	}
	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_CAPTURELIMIT ) ) {
		si_captureLimit.SetInteger( currentVoteData.m_captureLimit );
		needRescanSI = true;
	}
	if ( 0 != ( currentVoteData.m_fieldFlags & VOTEFLAG_FRAGLIMIT ) ) {
		si_fragLimit.SetInteger( currentVoteData.m_fragLimit );
		needRescanSI = true;
	}

	if ( needRescanSI ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" " " __FILE__ " " __LINESTR__ );
	}

	if ( needNextMap ) {
		gameLocal.sessionCommand = "nextMap";
	}
	else if ( needRestart || gameLocal.NeedRestart() ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "serverMapRestart" );	
	}
}

// RAVEN END

/*
================
idMultiplayerGame::SendMapList
================
*/
void idMultiplayerGame::SendMapList( int clientNum ) {
	int numMaps = fileSystem->GetNumMaps();
	const idDict *dict;
	int i;

	idBitMsg outMsg;
	byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_GETVOTEMAPS );

	for ( i = 0; i < numMaps; i++ ) {
		dict = fileSystem->GetMapDecl( i );

		const char *mapName = dict->GetString( "path" );
		assert( mapName[ 0 ] != '\0' );
		outMsg.WriteString( mapName );
	}
	outMsg.WriteString( "" );

	if ( gameLocal.localClientNum == clientNum ) {
		outMsg.BeginReading();
		outMsg.ReadByte();
		ReadMapList( outMsg );
	} else {
		networkSystem->ServerSendReliableMessage( clientNum, outMsg );
	}
}

/*
================
idMultiplayerGame::ReadMapList
================
*/
void idMultiplayerGame::ReadMapList( const idBitMsg &msg ) {
	int numMaps = fileSystem->GetNumMaps();
	const idDict *dict;
	char path[ MAX_STRING_CHARS ];
	int i;

	voteMapDecls.Clear();

	while ( msg.ReadString( path, MAX_STRING_CHARS ) > 0 ) {
		// find the local decl for the path
		for ( i = 0; i < numMaps; i++ ) {
			dict = fileSystem->GetMapDecl( i );

			if ( !idStr::Icmp( path, dict->GetString( "path" ) ) ) {
				break;
			}
		}
		if ( i >= numMaps ) {
			// ignore maps we don't already have
			continue;
		}

		voteMapDecls.Append( i );
	}

	// update any map requests that triggered this
	int flags = voteMapsWaiting;

	voteMapsWaiting = 0;

	if ( flags & VOTEMAPS_WAITING_MAPLIST ) {
		SetVoteMapList();
	}

	if ( flags & VOTEMAPS_WAITING_SAMAPLIST ) {
		SetSAMapList();
	}

	if ( flags & VOTEMAPS_WAITING_LISTMAPS ) {
		ListMaps();
	}
}

/*
================
idMultiplayerGame::RequestVoteMaps
================
*/
bool idMultiplayerGame::RequestVoteMaps( int flags ) {
	if ( voteMapDecls.Num() > 0 ) {
		return true;
	}

	if ( gameLocal.isServer || !gameLocal.isClient ) {
		int i;
		int numMaps = fileSystem->GetNumMaps();

		voteMapDecls.Clear();
		for (i = 0; i < numMaps; i++) {
			voteMapDecls.Append( i );
		}
		return true;
	}

	if ( voteMapsWaiting ) {
		return false;
	}

	idBitMsg outMsg;
	byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];
	
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_GETVOTEMAPS );
	networkSystem->ClientSendReliableMessage( outMsg );

	voteMapsWaiting |= flags;

	return false;
}

/*
================
idMultiplayerGame::ListMaps
================
*/
void idMultiplayerGame::ListMaps( void ) {
	if ( !RequestVoteMaps( VOTEMAPS_WAITING_LISTMAPS ) ) {
		gameLocal.Printf( "Requesting map list...\n" );
		return;
	}

	int i;
	int numMaps = voteMapDecls.Num();

	for (i = 0; i < numMaps; i++) {
		const idDict *dict = fileSystem->GetMapDecl( voteMapDecls[ i ] );
		gameLocal.Printf( "%s", dict->GetBool( "DM" ) ? "DM " : "   " );
		gameLocal.Printf( "%s", dict->GetBool( "Team DM" ) ? "TDM " : "    " );
		gameLocal.Printf( "%s", dict->GetBool( "CTF" ) ? "CTF " : "    " );
		gameLocal.Printf( "%s", dict->GetBool( "Arena CTF" ) ? "ACTF " : "     " );
		gameLocal.Printf( "%s", dict->GetBool( "Tourney" ) ? "Trn " : "    " );
		gameLocal.Printf( "%-20s %s\n", dict->GetString( "path" ), common->GetLocalizedString( dict->GetString( "name" ) ) );
	}
}

/*
================
idMultiplayerGame::SetMapList
================
*/
void idMultiplayerGame::SetMapList( const char *listName, const char *mapName, int gameTypeInt ) {
	int numMaps = voteMapDecls.Num();
	const idDict *dict;
	int numMapsAdded = 0;
	int i;

	if ( !RequestVoteMaps( !idStr::Cmp( listName, "mapList" ) ? VOTEMAPS_WAITING_MAPLIST : VOTEMAPS_WAITING_SAMAPLIST ) ) {
		return;
	}

	const char *gameType = VoteGameTypeToString( gameTypeInt );

	idStr originalMapName = gameLocal.serverInfo.GetString( "si_map" );
	originalMapName.StripFileExtension();

	bool foundOriginalMap = false;
	int originalMapIndex = -1;

	for ( i = 0; i < numMaps; i++ ) {
		dict = fileSystem->GetMapDecl( voteMapDecls[ i ] );

		bool mapOk = false;
		//if the gametype is DM, check for any of these types...
		if( !(strcmp( gameType, "DM")) || !(strcmp( gameType, "Team DM")) ) {
			if ( dict && (
				dict->GetBool( "DM" ) || 
				dict->GetBool( "Team DM" ) || 
				dict->GetBool( "CTF" ) || 
				dict->GetBool( "Tourney" ) ||
				dict->GetBool( "Arena CTF" ))
				) {
			mapOk = true;
			}
		//but if not, match the gametype.
		} else if ( dict && dict->GetBool( gameType ) ) {
			mapOk = true;			
		}
		if( mapOk ) {
			const char *mapName = dict->GetString( "name" );
			if ( '\0' == mapName[ 0 ] ) {
				mapName = dict->GetString( "path" );
			}
			mapName = common->GetLocalizedString( mapName );

			if ( idStr::Icmp(dict->GetString( "path" ), originalMapName) == 0 ) {
				foundOriginalMap = true;
				originalMapIndex = numMapsAdded;
			}

			mainGui->SetStateString( va( "%s_item_%d", listName, numMapsAdded), mapName );
			mainGui->SetStateInt( va( "%s_item_%d_id", listName, numMapsAdded), voteMapDecls[ i ] );

			numMapsAdded++;
		}
	}

	mainGui->DeleteStateVar( va( "%s_item_%d", listName, numMapsAdded ) );

	if ( !foundOriginalMap ) {
		mainGui->SetStateInt( va( "%s_sel_0", listName ), 0 );
		mainGui->SetStateString( mapName, mainGui->GetStateString( va( "%s_item_0", listName ) ) );
	} else {
		mainGui->SetStateInt( va( "%s_sel_0", listName ), originalMapIndex );
		mainGui->SetStateString( mapName, mainGui->GetStateString( va( "%s_item_%d", listName, originalMapIndex ) ) );
	}
}

/*
================
idMultiplayerGame::SetVoteMapList
================
*/
void idMultiplayerGame::SetVoteMapList( void ) {
	SetMapList( "mapList", "mapName", mainGui->GetStateInt( "currentGametype" ) );
}

/*
================
idMultiplayerGame::SetSAMapList
================
*/
void idMultiplayerGame::SetSAMapList( void ) {
	SetMapList( "sa_mapList", "sa_mapName", mainGui->GetStateInt( "adminCurrentGametype" ) );
}

/*
================
idMultiplayerGame::ClientEndFrame
Called once each render frame (client) after all idGameLocal::ClientPredictionThink() calls
================
*/
void idMultiplayerGame::ClientEndFrame( void ) {
	iconManager->UpdateIcons();
}

/*
================
idMultiplayerGame::CommonRun
Called once each render frame (client)/once each game frame (server)
================
*/
void idMultiplayerGame::CommonRun( void ) {
	idPlayer* player = gameLocal.GetLocalPlayer();

	// twhitaker r282
	// TTimo: sure is a nasty way to do it
	if ( gameLocal.isServer && ( gameLocal.serverInfo.GetInt( "net_serverDedicated" ) != cvarSystem->GetCVarInteger( "net_serverDedicated" ) ) ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "spawnServer\n" );
	}

	if ( player && player->mphud ) {
		// update icons
		if ( gameLocal.isServer ) {
			iconManager->UpdateIcons();
		}

#ifdef _USE_VOICECHAT
		float	micLevel;
		bool	sending, testing;

		// jscott: enable the voice recording
		testing = cvarSystem->GetCVarBool( "s_voiceChatTest" );
		sending = soundSystem->EnableRecording( !!( player->usercmd.buttons & BUTTON_VOICECHAT ), testing, micLevel );

		if( mainGui ) {
			mainGui->SetStateFloat( "s_micLevel", micLevel );
			mainGui->SetStateFloat( "s_micInputLevel", cvarSystem->GetCVarFloat( "s_micInputLevel" ) );
		}

// RAVEN BEGIN
// shouchard:  let the UI know about voicechat states
		if ( !testing && sending ) {
			player->mphud->HandleNamedEvent( "show_transmit_self" );
		} else {
			player->mphud->HandleNamedEvent( "hide_transmit_self" );
		}

		if( player->GetUserInfo() && player->GetUserInfo()->GetBool( "s_voiceChatReceive" ) ) {
			int maxChannels = soundSystem->GetNumVoiceChannels();
			int clientNum = -1;
			for (int channels = 0; channels < maxChannels; channels++ ) {
				clientNum = soundSystem->GetCommClientNum( channels );
				if ( -1 != clientNum ) {
					break;
				}
			}

			// Sanity check for network errors
			assert( clientNum > -2 && clientNum < MAX_CLIENTS );

			if ( clientNum > -1 && clientNum < MAX_CLIENTS ) {
				idPlayer *from = ( idPlayer * )gameLocal.entities[clientNum];
				if( from ) {
					player->mphud->SetStateString( "audio_name", from->GetUserInfo()->GetString( "ui_name" ) );
					player->mphud->HandleNamedEvent( "show_transmit" );
				}
			} else {
				player->mphud->HandleNamedEvent( "hide_transmit" );
			}
		}
		else {
			player->mphud->HandleNamedEvent( "hide_transmit" );
		}
#endif // _USE_VOICECHAT
// RAVEN END
	}
#ifdef _USE_VOICECHAT
	// jscott: Send any new voice data
	XmitVoiceData();
#endif

	int oldRank = -1;
	int oldLeadingTeam = -1;
	bool wasTied = false;
	int oldHighScore = idMath::INT_MIN;

	if( player && rankedPlayers.Num() ) {
		if( gameLocal.gameType == GAME_DM ) {
			oldRank = GetPlayerRank( player, wasTied );
			oldHighScore = rankedPlayers[ 0 ].Second();
		} else if( gameLocal.IsTeamGame() ) {
			oldLeadingTeam = rankedTeams[ 0 ].First();
			wasTied = ( rankedTeams[ 0 ].Second() == rankedTeams[ 1 ].Second() );
			oldHighScore = rankedTeams[ 0 ].Second();
		}	
	} 

	UpdatePlayerRanks();
	if ( gameLocal.IsTeamGame() ) {
		UpdateTeamRanks();
	}

	if ( player && rankedPlayers.Num() && gameState->GetMPGameState() == GAMEON ) {
		if ( gameLocal.gameType == GAME_DM ) {
			// leader message
			bool isTied = false;
			int newRank = GetPlayerRank( player, isTied );
         
			if ( newRank == 0 ) {
				if( ( oldRank != 0 || wasTied ) && !isTied ) {
					// we've gained first place or the person we were tied with dropped out of first place		
					ScheduleAnnouncerSound( AS_DM_YOU_HAVE_TAKEN_LEAD, gameLocal.time );
				} else if( oldRank != 0 || (!wasTied && isTied) ) {
					// we tied first place or we were in first and someone else tied
					ScheduleAnnouncerSound( AS_DM_YOU_TIED_LEAD, gameLocal.time );
				}
			} else if ( oldRank == 0 ) {
				// we lost first place
				ScheduleAnnouncerSound( AS_DM_YOU_LOST_LEAD, gameLocal.time );			
			}
		} else if ( gameLocal.IsTeamGame() ) {
			int	leadingTeam = rankedTeams[ 0 ].First();
			bool isTied = ( rankedTeams[ 0 ].Second() == rankedTeams[ 1 ].Second() );

			if ( !wasTied && isTied ) {
				if ( gameLocal.gameType != GAME_DEADZONE )
				ScheduleAnnouncerSound( AS_TEAM_TEAMS_TIED, gameLocal.time );
			} else if ( (leadingTeam != oldLeadingTeam && !isTied) || ( wasTied && !isTied ) ) {
				ScheduleAnnouncerSound( leadingTeam ? AS_TEAM_STROGG_LEAD : AS_TEAM_MARINES_LEAD, gameLocal.time );
			}

			if ( gameLocal.gameType == GAME_TDM && oldHighScore != teamScore[ rankedTeams[ 0 ].First() ] && gameLocal.serverInfo.GetInt( "si_fragLimit" ) > 0 ) {
				if( teamScore[ rankedTeams[ 0 ].First() ] == gameLocal.serverInfo.GetInt( "si_fragLimit" ) - 3 ) {
					ScheduleAnnouncerSound( AS_GENERAL_THREE_FRAGS, gameLocal.time );
				} else if( teamScore[ rankedTeams[ 0 ].First() ] == gameLocal.serverInfo.GetInt( "si_fragLimit" ) - 2 ) {
					ScheduleAnnouncerSound( AS_GENERAL_TWO_FRAGS, gameLocal.time );	
				} else if( teamScore[ rankedTeams[ 0 ].First() ] == gameLocal.serverInfo.GetInt( "si_fragLimit" ) - 1 ) {
					ScheduleAnnouncerSound( AS_GENERAL_ONE_FRAG, gameLocal.time );	
				}
			}
		}

		if( ( gameLocal.gameType == GAME_DM ) && rankedPlayers[ 0 ].Second() != oldHighScore && gameLocal.serverInfo.GetInt( "si_fragLimit" ) > 0 ) {
			// fraglimit warning
			if( rankedPlayers[ 0 ].Second() == gameLocal.serverInfo.GetInt( "si_fragLimit" ) - 3 ) {
				ScheduleAnnouncerSound( AS_GENERAL_THREE_FRAGS, gameLocal.time );
			} else if( rankedPlayers[ 0 ].Second() == gameLocal.serverInfo.GetInt( "si_fragLimit" ) - 2 ) {
				ScheduleAnnouncerSound( AS_GENERAL_TWO_FRAGS, gameLocal.time );
			} else if( rankedPlayers[ 0 ].Second() == gameLocal.serverInfo.GetInt( "si_fragLimit" ) - 1 ) {
				ScheduleAnnouncerSound( AS_GENERAL_ONE_FRAG, gameLocal.time );
			}
		}
	
	}

	if ( rankTextPlayer ) {
		bool tied = false;
		int rank = GetPlayerRank( rankTextPlayer, tied );
		(gameLocal.GetLocalPlayer())->GUIMainNotice( GetPlayerRankText( rank, tied, playerState[ rankTextPlayer->entityNumber ].fragCount ) );		
		rankTextPlayer = NULL;
	}

	PlayAnnouncerSounds();


	// asalmon: Need to refresh stats periodically if the player is looking at stats
	if ( currentStatClient != -1 ) {
		rvPlayerStat* clientStat = statManager->GetPlayerStat( currentStatClient );
		if ( ( gameLocal.time - clientStat->lastUpdateTime ) > 5000 ) {
			statManager->SelectStatWindow(currentStatClient, currentStatTeam);
		} 
	}

	bool updateModels = false;
	if( g_forceModel.IsModified() && !gameLocal.IsTeamGame() ) {
		updateModels = true;
		g_forceModel.ClearModified();
	}

	if( g_forceMarineModel.IsModified() && gameLocal.IsTeamGame() ) {
		updateModels = true;
		g_forceMarineModel.ClearModified();
	}

	if( g_forceStroggModel.IsModified() && gameLocal.IsTeamGame() ) {
		updateModels = true;
		g_forceStroggModel.ClearModified();
	}

	if( updateModels ) {
		for( int i = 0; i < gameLocal.numClients; i++ ) {
			idPlayer* player = (idPlayer*)gameLocal.entities[ i ];
			if( player ) {
				player->UpdateModelSetup();
			}
		}
	}

	// do this here rather than in idItem::Think() because clients don't run Think on ents outside their snap
	if( g_simpleItems.IsModified() ) {

		for( int i = 0; i < MAX_GENTITIES; i++ ) {
			idEntity* ent = gameLocal.entities[ i ];
			if( !ent || !ent->IsType( idItem::GetClassType() ) || ent->IsType( rvItemCTFFlag::GetClassType() ) ) {
				continue;
			}
			
			idItem* item = (idItem*)ent;

			item->FreeModelDef();

			renderEntity_t* renderEntity = item->GetRenderEntity();
			memset( renderEntity, 0, sizeof( renderEntity ) );

			item->simpleItem = g_simpleItems.GetBool() && gameLocal.isMultiplayer && !item->IsType( rvItemCTFFlag::GetClassType() );

			if( item->simpleItem ) {
				renderEntity->shaderParms[ SHADERPARM_RED ]				= 1.0f;
				renderEntity->shaderParms[ SHADERPARM_GREEN ]			= 1.0f;
				renderEntity->shaderParms[ SHADERPARM_BLUE ]			= 1.0f;
				renderEntity->shaderParms[ SHADERPARM_ALPHA ]			= 1.0f;
				renderEntity->shaderParms[ SHADERPARM_SPRITE_WIDTH ]	= item->simpleItemScale;
				renderEntity->shaderParms[ SHADERPARM_SPRITE_HEIGHT ]	= item->simpleItemScale;
				renderEntity->hModel = renderModelManager->FindModel( "_sprite" );
				renderEntity->callback = NULL;
				renderEntity->numJoints = 0;
				renderEntity->joints = NULL;
				renderEntity->customSkin = 0;
				renderEntity->noShadow = true;
				renderEntity->noSelfShadow = true;
				renderEntity->customShader = declManager->FindMaterial( item->spawnArgs.GetString( "mtr_simple_icon" ) );

				renderEntity->referenceShader = 0;
				renderEntity->bounds = renderEntity->hModel->Bounds( renderEntity );
				renderEntity->axis = mat3_identity;

				item->StopEffect( "fx_idle", true );
				item->effectIdle = NULL;
				item->SetAxis( mat3_identity );
				if( item->pickedUp ) {
					item->FreeModelDef();
					item->UpdateVisuals();
				}
			} else {
				gameEdit->ParseSpawnArgsToRenderEntity( &item->spawnArgs, renderEntity );
				item->SetAxis( renderEntity->axis );

				if ( item->spawnArgs.GetString( "fx_idle" ) ) {
					item->UpdateModelTransform();
					item->effectIdle = item->PlayEffect( "fx_idle", renderEntity->origin, renderEntity->axis, true );
				}

				if( item->pickedUp && item->pickupSkin ) {
					item->SetSkin( item->pickupSkin );
				}
			}
			if ( !item->spawnArgs.GetBool( "dropped" ) ) {
				if ( item->spawnArgs.GetBool( "nodrop" ) ) {
					item->GetPhysics()->PutToRest();
				} else {
					item->Event_DropToFloor();
				}
			}
		}

		g_simpleItems.ClearModified();
	}

	if (hud_showSpeed.IsModified()) {
		idPlayer* player = gameLocal.GetLocalPlayer();
		if( player && player->hud) {
			player->hud->HandleNamedEvent( hud_showSpeed.GetBool() ? "showSpeed" : "hideSpeed" );
		}
		hud_showSpeed.ClearModified();
	}
}

/*
================
idMultiplayerGame::ClientRun
Called once each client render frame (before any ClientPrediction frames have been run)
================
*/
void idMultiplayerGame::ClientRun( void ) {
	if ( gameLocal.isRepeater ) {
		assert( !gameLocal.isServer );
		pureReady = true;
	}

	CommonRun();
}


/*
================
idMultiplayerGame::ReportZoneControllingPlayer
================
*/
void idMultiplayerGame::ReportZoneControllingPlayer( idPlayer* player )
{
	assert( gameLocal.gameType == GAME_DEADZONE );

	if ( !player )
		return;

	playerState[player->entityNumber].deadZoneScore += gameLocal.GetMSec();
	playerState[player->entityNumber].teamFragCount = playerState[player->entityNumber].deadZoneScore / 1000;

	float cashPerSecondForDeadZoneControl = (float) gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerCashAward_deadZoneControlPerSecond", 0 );
//	player->GiveCash( cashPerSecondForDeadZoneControl * 0.001f * (float) gameLocal.GetMSec() );
	player->buyMenuCash += ( cashPerSecondForDeadZoneControl * 0.001f * (float) gameLocal.GetMSec() );
}


/*
================
idMultiplayerGame::ReportZoneController
================
*/
void idMultiplayerGame::ReportZoneController(int team, int pCount, int situation, idEntity* zoneTrigger)
{
	assert( gameLocal.gameType == GAME_DEADZONE );
	assert( gameState->IsType( riDZGameState::GetClassType() ) );

	riDZGameState *dzGameState = static_cast<riDZGameState *>( gameState );

	powerupCount = pCount;

	idTrigger_Multi* zTrigger = 0;
	if ( zoneTrigger && zoneTrigger->IsType( idTrigger_Multi::GetClassType() ) ) {
		zTrigger = static_cast<idTrigger_Multi *>( zoneTrigger );
	}

	if ( gameLocal.mpGame.GetGameState()->GetMPGameState() != GAMEON && gameLocal.mpGame.GetGameState()->GetMPGameState() != SUDDENDEATH )
	{
		// We're not playing right now.  However, make sure all the clients are updated to know 
		// that the zone is neutral.
		dzGameState->SetDZState(TEAM_MARINE, DZ_NONE);
		dzGameState->SetDZState(TEAM_STROGG, DZ_NONE);
		if ( zTrigger && zTrigger->spawnArgs.MatchPrefix( "entityAffect" ) ) {
			idEntity* targetEnt = gameLocal.FindEntity(zTrigger->spawnArgs.GetString("entityAffect", ""));
			if ( targetEnt ) {
				dzGameState->dzTriggerEnt = targetEnt->entityNumber;
				dzGameState->dzShaderParm = 2;
				targetEnt->SetShaderParm(7, 2.0f);
			}
		}
		return;
	}

	if ( IsValidTeam(team) ) {
		const int t = gameLocal.serverInfo.GetInt( "si_controlTime" );
		teamDeadZoneScore[team] += gameLocal.GetMSec() * powerupCount;
		teamScore[team] = (int)((float)teamDeadZoneScore[team] / 1000.0f);	

		// We have a winner!
		if ( teamDeadZoneScore[team] > t*1000 ) {
			// Set the shaders and lights back to neutral.  
			if ( zTrigger->spawnArgs.MatchPrefix( "colorTarget" ) ) {
				const idKeyValue *arg;
				int refLength = strlen( "colorTarget" );
				int num = zTrigger->spawnArgs.GetNumKeyVals();
				for( int i = 0; i < num; i++ ) {
					arg = zTrigger->spawnArgs.GetKeyVal( i );
					if ( arg->GetKey().Icmpn( "colorTarget", refLength ) == 0 ) {
						idStr targetStr = arg->GetValue();
						idEntity* targetEnt = gameLocal.FindEntity(targetStr);
						if ( targetEnt ) {
							targetEnt->SetColor(idVec3(0.75f, 0.75f, 0.75f));
						}
					}
				}
			}

			if ( zTrigger && zTrigger->spawnArgs.MatchPrefix( "entityAffect" ) ) {
				idEntity* targetEnt = gameLocal.FindEntity(zTrigger->spawnArgs.GetString("entityAffect", ""));
				if ( targetEnt ) {
					dzGameState->dzTriggerEnt = targetEnt->entityNumber;
					dzGameState->dzShaderParm = 2;
					targetEnt->SetShaderParm(7, 2.0f);
				}
			}

			OnDeadZoneTeamVictory( team );

			return;
		}
	}

	// Someone took control of a zone, report this to the 
	if ( situation == DZ_MARINES_TAKEN || situation == DZ_STROGG_TAKEN || situation == DZ_MARINE_TO_STROGG ||
		situation == DZ_STROGG_TO_MARINE || situation == DZ_MARINE_REGAIN || situation == DZ_STROGG_REGAIN ) {
		dzGameState->SetDZState(TEAM_MARINE, DZ_NONE); // Clear hacked deadlock
		dzGameState->SetDZState(team, DZ_TAKEN);
	}

	const int NOCHANGE = -2;
	const int DEADLOCK = 3;
	int controlSit = NOCHANGE;
	switch ( situation ) {
		case DZ_NONE : 
			controlSit = NOCHANGE;
			break;
		case DZ_MARINES_TAKEN : 
			controlSit = TEAM_MARINE;
			break;
		case DZ_MARINES_LOST :
			controlSit = TEAM_NONE;
			dzGameState->SetDZState(TEAM_MARINE, DZ_LOST);
			break;
		case DZ_STROGG_TAKEN : 
			controlSit = TEAM_STROGG;
			break;
		case DZ_STROGG_LOST : 
			controlSit = TEAM_NONE;
			dzGameState->SetDZState(TEAM_STROGG, DZ_LOST);
			break;
		case DZ_MARINE_TO_STROGG :
			controlSit = TEAM_STROGG;
			break;
		case DZ_STROGG_TO_MARINE :
			controlSit = TEAM_MARINE;
			break;
		case DZ_MARINE_DEADLOCK : 
			controlSit = DEADLOCK;
			dzGameState->SetDZState(TEAM_MARINE, DZ_DEADLOCK);
			break;
		case DZ_STROGG_DEADLOCK : 
			controlSit = DEADLOCK;
			dzGameState->SetDZState(TEAM_MARINE, DZ_DEADLOCK);
			break;
		case DZ_MARINE_REGAIN :
			controlSit = TEAM_MARINE;
			break;
		case DZ_STROGG_REGAIN : 
			controlSit = TEAM_STROGG;
			break;
	}

	if ( zTrigger && controlSit == NOCHANGE && zTrigger->spawnArgs.MatchPrefix( "entityAffect" ) ) {
		// There's been no change in status, but keep these variables updated on the client
		idEntity* targetEnt = gameLocal.FindEntity(zTrigger->spawnArgs.GetString("entityAffect", ""));
		if ( targetEnt ) {
			dzGameState->dzTriggerEnt = targetEnt->entityNumber;
			dzGameState->dzShaderParm = (int)targetEnt->GetRenderEntity()->shaderParms[7];
		}
	}

	if ( controlSit == NOCHANGE || !zTrigger )
		return; // We're done.

	idVec3 colorVec;
	int parmNum = 2;
	if ( controlSit == TEAM_NONE ) {
		colorVec = idVec3(0.75f, 0.75f, 0.75f);
		parmNum = 2;
	}
	else if ( controlSit == TEAM_MARINE ) {
		colorVec = idVec3(0.0f, 1.0f, 0.0f);
		parmNum = 0;
	}
	else if ( controlSit == TEAM_STROGG ) {
		colorVec = idVec3(1.0f, 0.5f, 0.0f);
		parmNum = 1;
	}
	else if ( controlSit == DEADLOCK )  {
		colorVec = idVec3(1.0f, 0.0f, 0.0f);
		parmNum = 3;
	}

	if ( zTrigger->spawnArgs.MatchPrefix( "colorTarget" ) ) {
		const idKeyValue *arg;
		int refLength = strlen( "colorTarget" );
		int num = zTrigger->spawnArgs.GetNumKeyVals();
		for( int i = 0; i < num; i++ ) {
			arg = zTrigger->spawnArgs.GetKeyVal( i );
			if ( arg->GetKey().Icmpn( "colorTarget", refLength ) == 0 ) {
				idStr targetStr = arg->GetValue();
				idEntity* targetEnt = gameLocal.FindEntity(targetStr);
				if ( targetEnt ) {
					targetEnt->SetColor(colorVec);
				}
			}
		}
	}

	if ( zTrigger && zTrigger->spawnArgs.MatchPrefix( "entityAffect" ) ) {
		idEntity* targetEnt = gameLocal.FindEntity(zTrigger->spawnArgs.GetString("entityAffect", ""));
		if ( targetEnt ) {
			dzGameState->dzTriggerEnt = targetEnt->entityNumber;
			dzGameState->dzShaderParm = parmNum;
			targetEnt->SetShaderParm(7, (float)parmNum);
		}
	}
}



bool idMultiplayerGame::IsValidTeam(int team)
{
	if ( team == TEAM_MARINE || team == TEAM_STROGG )
		return true;

	return false;
}


void idMultiplayerGame::OnDeadZoneTeamVictory( int winningTeam )
{
	OnBuyModeTeamVictory( winningTeam );

	gameState->NewState( GAMEREVIEW );
}

void idMultiplayerGame::OnBuyModeTeamVictory( int winningTeam )
  {
  	if( !IsBuyingAllowedInTheCurrentGameMode() )
  		return;
  
  	float teamCashForWin	= (float) gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "teamCashAward_gameModeWin", 0 );
  	float teamCashForTie	= (float) gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "teamCashAward_gameModeTie", 0 );
  	float teamCashForLoss	= (float) gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "teamCashAward_gameModeLoss", 0 );
  
  	if( winningTeam == TEAM_NONE )
  	{
  		GiveCashToTeam( TEAM_MARINE, teamCashForTie );
  		GiveCashToTeam( TEAM_STROGG, teamCashForTie );
  	}
  	else
  	{
  		int losingTeam = 1 - winningTeam;
  		GiveCashToTeam( winningTeam, teamCashForWin );
  		GiveCashToTeam( losingTeam, teamCashForLoss );
  	}
 }

/*
================
idMultiplayerGame::Run
================
*/
void idMultiplayerGame::Run( void ) {
	pureReady = true;

	assert( gameLocal.isMultiplayer && gameLocal.isServer && gameState );

	CommonRun();

	CheckVote();

	CheckRespawns();

	CheckSpecialLights( );

//RITUAL BEGIN
	UpdateTeamPowerups();
//RITUAL END
	gameState->Run();

	gameState->SendState( serverReliableSender.To( -1 ) );

	// don't update the ping every frame to save bandwidth
	if ( gameLocal.time > pingUpdateTime ) {
		for ( int i = 0; i < gameLocal.numClients; i++ ) {
			playerState[i].ping = networkSystem->ServerGetClientPing( i );
		}
		pingUpdateTime = gameLocal.time + 1000;
	}


}

/*
================
idMultiplayerGame::UpdateMainGui
================
*/
void idMultiplayerGame::UpdateMainGui( void ) {
	int i;
	mainGui->SetStateInt( "readyon", gameState->GetMPGameState() == WARMUP ? 1 : 0 );
	mainGui->SetStateInt( "readyoff", gameState->GetMPGameState() != WARMUP ? 1 : 0 );
	idStr strReady = cvarSystem->GetCVarString( "ui_ready" );
	if ( strReady.Icmp( "ready") == 0 ){
		strReady = common->GetLocalizedString( "#str_104248" );
	} else {
		strReady = common->GetLocalizedString( "#str_104247" );
	}
	mainGui->SetStateString( "ui_ready", strReady );
	mainGui->SetStateInt( "num_spec_players", unrankedPlayers.Num() );
	
	mainGui->SetStateInt( "gametype", gameLocal.gameType );
	mainGui->SetStateBool( "s_useOpenAL", cvarSystem->GetCVarBool( "s_useOpenAL" ) );
	mainGui->SetStateBool( "s_loadOpenALFailed", cvarSystem->GetCVarBool( "s_loadOpenALFailed" ) );

	idVec4	hitscanTint;
	idStr	hitScanValue = cvarSystem->GetCVarString( "ui_hitscanTint" );
	sscanf( hitScanValue.c_str(), "%f %f %f %f", &hitscanTint.x, &hitscanTint.y, &hitscanTint.z, &hitscanTint.w );
	mainGui->SetStateFloat( "ui_hitscanTint", hitscanTint.x );

	// RAVEN BEGIN
// bdube: capture the flag
	if ( gameLocal.IsTeamGame() ) {
		idPlayer *p = gameLocal.GetLocalPlayer();
		if ( p ) {
			mainGui->SetStateInt( "team", p->team );
		}
		mainGui->SetStateInt( "teamon", 1 );
		mainGui->SetStateInt( "teamoff", 0 );
	} else {
		mainGui->SetStateInt( "teamon", 0 );
		mainGui->SetStateInt( "teamoff", 1 );
	}
// RAVEN END		
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	mainGui->SetStateInt( "teamon", (gameLocal.gameType == GAME_TDM || gameLocal.gameType == GAME_DEADZONE) ? 1 : 0 );
	mainGui->SetStateInt( "teamoff", !(gameLocal.gameType == GAME_TDM || gameLocal.gameType == GAME_DEADZONE) ? 1 : 0 );
	if ( gameLocal.gameType == GAME_TDM || gameLocal.gameType == GAME_DEADZONE ) {
// RITUAL END
		idPlayer *p = gameLocal.GetLocalPlayer();
		if ( p ) {
			mainGui->SetStateInt( "team", p->team );
		}		
	}
	// setup vote
	mainGui->SetStateInt( "voteon", ( vote != VOTE_NONE && !voted ) ? 1 : 0 );
	mainGui->SetStateInt( "voteoff", ( vote != VOTE_NONE && !voted ) ? 0 : 1 );
	// send the current serverinfo values
	for ( i = 0; i < gameLocal.serverInfo.GetNumKeyVals(); i++ ) {
		const idKeyValue *keyval = gameLocal.serverInfo.GetKeyVal( i );
		mainGui->SetStateString( keyval->GetKey(), keyval->GetValue() );
	}
	mainGui->StateChanged( gameLocal.time );
#if defined( __linux__ )
	// replacing the oh-so-useful s_reverse with sound backend prompt
	mainGui->SetStateString( "driver_prompt", "1" );
#else
	mainGui->SetStateString( "driver_prompt", "0" );
#endif

//RAVEN BEGIN
// cnicholson: Add Custom Crosshair update
	mainGui->SetStateString( "g_crosshairCustom", cvarSystem->GetCVarBool( "g_crosshairCustom" ) ? "1" : "0" );
//RAVEN END

// RAVEN BEGIN
// cnicholson: We need to setup the custom crosshair so it shows up the first time the player enters the MP settings menu.
//			   This block checks the current crosshair, and compares it against the list of crosshairs in player.def (mtr_crosshair*) under the 
//			   player_marine_mp section. If it finds a match, it assigns the crosshair, otherwise, the first found crosshair is used.
#ifndef _XENON
	const idDeclEntityDef *defCH = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, "player_marine_mp", false, true ) );
#else
	bool insideLevelLoad = declManager->GetInsideLoad();
	if ( !insideLevelLoad ) {
		declManager->SetInsideLoad( true );
	}
	const idDeclEntityDef *defCH = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, "player_marine_mp_ui", false, false ) );
	declManager->SetInsideLoad( insideLevelLoad );
#endif

#ifndef _XENON
	idStr currentCrosshair = cvarSystem->GetCVarString("g_crosshairCustomFile");

	const idKeyValue* kv = defCH->dict.MatchPrefix("mtr_crosshair", NULL);

	while ( kv ) {													// Loop through all crosshairs listed in the def
		if ( kv->GetValue() == currentCrosshair.c_str() ) {			// Until a match is found
			break;
		}
		kv = defCH->dict.MatchPrefix("mtr_crosshair", kv );
	}

	if ( !kv ){
		kv = defCH->dict.MatchPrefix("mtr_crosshair", NULL );			// If no natches are found, use the first one.
	}

	idStr newCrosshair(kv->GetValue());

	mainGui->SetStateString ( "crossImage", newCrosshair.c_str());
	const idMaterial *material = declManager->FindMaterial( newCrosshair.c_str() );
	if ( material ) {
		material->SetSort( SS_GUI );
	}			


	cvarSystem->SetCVarString("g_crosshairCustomFile", newCrosshair.c_str());
#endif


//asalmon: Set up a state var for match type of Xbox 360
#ifdef _XENON
	mainGui->SetStateBool("CustomHost", Live()->IsCustomHost());
	mainGui->SetStateInt("MatchType", Live()->GetMatchtype());
	mainGui->SetStateString("si_gametype", gameLocal.serverInfo.GetString("si_gametype"));
	const char *damage; 
	if (gameLocal.serverInfo.GetBool("si_teamdamage")){
		damage = "Yes" ; 
	}
	else {
		damage = "No";
	}
	mainGui->SetStateString("si_teamdamage", damage);
	const char *shuffle; 
	if (gameLocal.serverInfo.GetBool("si_shuffleMaps")){
		shuffle = "Yes" ; 
	}
	else {
		shuffle = "No";
	}
	mainGui->SetStateString("si_shuffleMaps", shuffle);
	mainGui->SetStateString("si_fraglimit", gameLocal.serverInfo.GetString("si_fraglimit"));
	mainGui->SetStateString("si_capturelimit", gameLocal.serverInfo.GetString("si_capturelimit"));
	mainGui->SetStateString("si_timelimit", gameLocal.serverInfo.GetString("si_timelimit"));

// mekberg: send spectating to the mainGui.
	if ( gameLocal.GetLocalPlayer( ) ) {
		mainGui->SetStateBool( "spectating", gameLocal.GetLocalPlayer( )->spectating );
		if( gameLocal.gameType == GAME_TOURNEY ) {
			if ( gameLocal.GetLocalPlayer()->GetUserInfo() ) {
				// additionally in tourney, indicate whether the player is voluntarily spectating
				mainGui->SetStateBool( "tourneyspectating", !idStr::Icmp( gameLocal.GetLocalPlayer()->GetUserInfo()->GetString( "ui_spectate" ), "Spectate" ) );
			} else {
				mainGui->SetStateBool( "tourneyspectating", 1 );
			}
		}
	} else {
		mainGui->SetStateBool( "spectating", false );
	}
#endif
// RAVEN END

}

/*
================
idMultiplayerGame::SetupBuyMenuItems
================
*/
void idMultiplayerGame::SetupBuyMenuItems()
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !player ) 
		return;

	buyMenu->SetStateInt( "buyStatus_shotgun", player->ItemBuyStatus( "weapon_shotgun" ) );
	buyMenu->SetStateInt( "buyStatus_hyperblaster", player->ItemBuyStatus( "weapon_hyperblaster" ) );
	buyMenu->SetStateInt( "buyStatus_grenadelauncher", player->ItemBuyStatus( "weapon_grenadelauncher" ) );
	buyMenu->SetStateInt( "buyStatus_nailgun", player->ItemBuyStatus( "weapon_nailgun" ) );
	buyMenu->SetStateInt( "buyStatus_rocketlauncher", player->ItemBuyStatus( "weapon_rocketlauncher" ) );
	buyMenu->SetStateInt( "buyStatus_railgun", player->ItemBuyStatus( "weapon_railgun" ) );
	buyMenu->SetStateInt( "buyStatus_lightninggun", player->ItemBuyStatus( "weapon_lightninggun" ) );
	//	buyMenu->SetStateInt( "buyStatus_dmg", player->ItemBuyStatus( "weapon_dmg" ) );
	buyMenu->SetStateInt( "buyStatus_napalmgun", player->ItemBuyStatus( "weapon_napalmgun" ) );

	buyMenu->SetStateInt( "buyStatus_lightarmor", player->ItemBuyStatus( "item_armor_small" ) );
	buyMenu->SetStateInt( "buyStatus_heavyarmor", player->ItemBuyStatus( "item_armor_large" ) );
	buyMenu->SetStateInt( "buyStatus_ammorefill", player->ItemBuyStatus( "ammorefill" ) );

	buyMenu->SetStateInt( "buyStatus_special0", player->ItemBuyStatus( "ammo_regen" ) );
	buyMenu->SetStateInt( "buyStatus_special1", player->ItemBuyStatus( "health_regen" ) );
	buyMenu->SetStateInt( "buyStatus_special2", player->ItemBuyStatus( "damage_boost" ) );

	buyMenu->SetStateInt( "playerTeam", player->team );

	if ( player->weapon )
		buyMenu->SetStateString( "ammoIcon", player->weapon->spawnArgs.GetString ( "inv_icon" ) );

	buyMenu->SetStateInt( "player_weapon", player->GetCurrentWeapon() );
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
	//if we're the server, allow access to the admin tab right away. Otherwise, make sure we don't have it.
	if( gameLocal.isServer	)	{
		mainGui->SetStateInt( "password_valid", 1 );
	} else {
		mainGui->SetStateInt( "password_valid", 0 );
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
	
	if( gameLocal.GetLocalPlayer() ) {
		gameLocal.GetLocalPlayer()->disableHud = true;
	}

	nextMenu = 0;
	if ( currentMenu == 1 ) {
		UpdateMainGui();

		// UpdateMainGui sets most things, but it doesn't set these because
		// it'd be pointless and/or harmful to set them every frame (for various reasons)
		// Currenty the gui doesn't update properly if they change anyway, so we'll leave it like this.

		// player kick data
		for ( i = 0; i < 16; i++ ) {
			kickVoteMapNames[ i ].Clear();
			kickVoteMap[ i ] = -1;
		}

		idStr kickList;
		j = 0;
		for ( i = 0; i < gameLocal.numClients; i++ ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
				if ( kickList.Length() ) {
					kickList += ";";
				}
				kickList += va( "\"%d - %s\"", i, gameLocal.userInfo[ i ].GetString( "ui_name" ) );
				kickVoteMap[ j ] = i;
// RAVEN BEGIN
// shouchard:  names for kick vote map
				kickVoteMapNames[ j ] = gameLocal.userInfo[ i ].GetString( "ui_name" );
// RAVEN END
				j++;
			}
		}
		mainGui->SetStateString( "kickChoices", kickList );

		mainGui->SetStateString( "chattext", "" );
		mainGui->Activate( true, gameLocal.time );

		idPlayer				*localP = gameLocal.GetLocalPlayer();
#ifndef _XENON
		const idDeclEntityDef	*def = gameLocal.FindEntityDef( "player_marine_mp", false );
#else
		const idDeclEntityDef	*def = gameLocal.FindEntityDef( "player_marine_mp_ui", false );
#endif
		idStr	buildValues, buildNames;

		int numModels = declManager->GetNumDecls( DECL_PLAYER_MODEL );
		for( int i = 0; i < numModels; i++ ) {
			const rvDeclPlayerModel* playerModel = (const rvDeclPlayerModel*)declManager->DeclByIndex( DECL_PLAYER_MODEL, i, false );

			if( !playerModel ) {
				continue;
			}
			
			const char *resultValue = playerModel->GetName();

			if ( !resultValue || !resultValue[0] ) {
				continue;
			}

			const char *team = playerModel->team.c_str();

			if ( gameLocal.IsTeamGame() ) {
				if ( team && localP && localP->team >= 0 && localP->team < TEAM_MAX && idStr::Icmp( teamNames[ localP->team ], team ) == 0 ) {
				} else {
					// doesn't match, so skip
					continue;
				}
			}

			if ( i ) {
				buildValues += ";";
				buildNames += ";";
			}
			buildValues += resultValue;

			const char *resultName = common->GetLocalizedString( playerModel->description.c_str() );

			if ( !resultName || !resultName[0] ) {
				buildNames += resultValue;
			} else {
				buildNames += resultName;
			}
		}
			
		mainGui->SetStateString( "model_values", buildValues.c_str() );
		mainGui->SetStateString( "model_names", buildNames.c_str() );
		mainGui->SetStateBool( "player_model_updated", true );

		const char *model;
		if ( localP && localP->team >= 0 && localP->team < TEAM_MAX ) {
			model = cvarSystem->GetCVarString( va( "ui_model_%s", teamNames[ localP->team ] ) );
			if( *model == '\0' ) {
				model = def->dict.GetString( va( "def_default_model_%s", teamNames[ localP->team ] ) );
			}
		} else {
			model = cvarSystem->GetCVarString( "ui_model" );
			if( *model == '\0' ) {
				model = def->dict.GetString( "def_default_model" );
			}
		}
		
		const rvDeclPlayerModel* playerModel = (const rvDeclPlayerModel*)declManager->FindType( DECL_PLAYER_MODEL, model, false );
		if ( playerModel ) {
			mainGui->SetStateString( "player_model_name", playerModel->model.c_str() );
			mainGui->SetStateString( "player_head_model_name", playerModel->uiHead.c_str() );
			mainGui->SetStateString( "player_skin_name", playerModel->skin.c_str() );
			if( playerModel->uiHead.Length() ) {
				const idDeclEntityDef* head = (const idDeclEntityDef*)declManager->FindType( DECL_ENTITYDEF, playerModel->uiHead.c_str(), false );
				if( head && head->dict.GetString( "skin" ) ) {
					mainGui->SetStateString( "player_head_skin_name", head->dict.GetString( "skin" ) );
				}
			}
			mainGui->SetStateBool( "need_update", true );
		}

		if( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetUserInfo() ) {
			cvarSystem->SetCVarString( "gui_ui_name", gameLocal.GetLocalPlayer()->GetUserInfo()->GetString( "ui_name" ) );
		} else {
			cvarSystem->SetCVarString( "gui_ui_name", cvarSystem->GetCVarString( "ui_name" ) );
		}
		
		if ( gameLocal.isTVClient ) {
			mainGui->SetStateBool( "is_tv_client", true );
		} else {
			mainGui->SetStateBool( "is_tv_client", false );
		}

		return mainGui;
	} else if ( currentMenu == 2 ) {
		// the setup is done in MessageMode
		if( gameLocal.GetLocalPlayer() ) {
			gameLocal.GetLocalPlayer()->disableHud = false;
		}
		msgmodeGui->Activate( true, gameLocal.time );
 		cvarSystem->SetCVarBool( "ui_chat", true );
		return msgmodeGui;
	} else if ( currentMenu == 3 ) {
		statSummary->Activate( true, gameLocal.time );
		statManager->SetupStatWindow( statSummary );
		UpdateScoreboard( statSummary );
		UpdateSummaryBoard( statSummary );
		statSummary->SetStateFloat( "ready", 0 );
		statSummary->StateChanged( gameLocal.time );

		// Moved the announcer sound here. This way we can be sure the client has updated team score information by this point.
		// #13576 #13544 causing double sounds because it's getting triggered twice at endgame ( from GameStateChanged and from ReceiveAllStats )
		// the move to here was for fixing some problem when running at the previous location ( GameStateChanged )
		// there are too many codepaths leading to various orders of ReceiveAllStats and GameStateChanged
		// various attempts to flag the right call that should trigger the sound failed, so just using a timeout now
		if ( gameLocal.time - lastVOAnnounce > 1000 ) {
			idPlayer* player = gameLocal.GetLocalPlayer();
			if ( gameLocal.IsTeamGame() ) {
				int winningTeam = GetScoreForTeam( TEAM_MARINE ) > GetScoreForTeam( TEAM_STROGG ) ? TEAM_MARINE : TEAM_STROGG;
				if( player->team == winningTeam ) {
					ScheduleAnnouncerSound( AS_GENERAL_YOU_WIN, gameLocal.time );
				} else {
					ScheduleAnnouncerSound( AS_GENERAL_YOU_LOSE, gameLocal.time );
				}
			} else if ( gameLocal.gameType != GAME_TOURNEY ) {
				if( player->GetRank() == 0 ) {
					ScheduleAnnouncerSound( AS_GENERAL_YOU_WIN, gameLocal.time );
				} else {
					ScheduleAnnouncerSound( AS_GENERAL_YOU_LOSE, gameLocal.time );
				}
			}
			lastVOAnnounce = gameLocal.time;
		}

		return statSummary;
// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	} else if ( currentMenu == 4 ) {
		//if( mpClientGameState.gameState.currentState == COUNTDOWN ) {
			idPlayer* player = gameLocal.GetLocalPlayer();
			buyMenu->SetStateString( "field_credits", va("%i", (int)player->buyMenuCash) );
			buyMenu->SetStateInt( "price_shotgun", player->GetItemCost("weapon_shotgun") );
			buyMenu->SetStateInt( "price_hyperblaster", player->GetItemCost("weapon_hyperblaster") );
			buyMenu->SetStateInt( "price_grenadelauncher", player->GetItemCost( "weapon_grenadelauncher" ) );
			buyMenu->SetStateInt( "price_nailgun", player->GetItemCost( "weapon_nailgun" ) );
			buyMenu->SetStateInt( "price_rocketlauncher", player->GetItemCost( "weapon_rocketlauncher" ) );
			buyMenu->SetStateInt( "price_railgun", player->GetItemCost( "weapon_railgun" ) );
			buyMenu->SetStateInt( "price_lightninggun", player->GetItemCost( "weapon_lightninggun" ) );
			//			buyMenu->SetStateInt( "price_dmg", player->GetItemCost( "weapon_dmg" ) );
			buyMenu->SetStateInt( "price_napalmgun", player->GetItemCost( "weapon_napalmgun" ) );

			buyMenu->SetStateInt( "price_lightarmor", player->GetItemCost( "item_armor_small" ) );
			buyMenu->SetStateInt( "price_heavyarmor", player->GetItemCost( "item_armor_large" ) );
			buyMenu->SetStateInt( "price_ammorefill", player->GetItemCost( "ammorefill" ) );

			buyMenu->SetStateInt( "price_special0", player->GetItemCost( "ammo_regen" ) );
			buyMenu->SetStateInt( "price_special1", player->GetItemCost( "health_regen" ) );
			buyMenu->SetStateInt( "price_special2", player->GetItemCost( "damage_boost" ) );
			SetupBuyMenuItems();
			buyMenu->Activate(true, gameLocal.time);
			return buyMenu;
		//}
// RITUAL END
	}

	return NULL;
}

/*
================
idMultiplayerGame::DisableMenu
================
*/
void idMultiplayerGame::DisableMenu( void ) {
	if ( currentMenu == 1 ) {
		mainGui->Activate( false, gameLocal.time );
	} else if ( currentMenu == 2 ) {
		msgmodeGui->Activate( false, gameLocal.time );
	} else if( currentMenu == 3 ) {
		statSummary->Activate( false, gameLocal.time );
// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	} else if( currentMenu == 4 ) {
		buyMenu->Activate( false, gameLocal.time );
// RITUAL END
	}

	// copy over name from temp cvar
	if( currentMenu == 1 && idStr::Cmp( cvarSystem->GetCVarString( "gui_ui_name" ), cvarSystem->GetCVarString( "ui_name" ) ) ) {
		cvarSystem->SetCVarString( "ui_name", cvarSystem->GetCVarString( "gui_ui_name" ) );
	}

	currentMenu = 0;
	nextMenu = 0;
 	cvarSystem->SetCVarBool( "ui_chat", false );
	
	if( gameLocal.GetLocalPlayer() ) {
		gameLocal.GetLocalPlayer()->disableHud = false;
//RAVEN BEGIN
//asalmon: make the scoreboard on Xenon close
#ifdef _XENON
		gameLocal.GetLocalPlayer()->scoreBoardOpen = false;
#endif
//RAVEN END
		if( gameLocal.GetLocalPlayer()->mphud)	{
			gameLocal.GetLocalPlayer()->mphud->Activate( true, gameLocal.time );
		}
	}

	mainGui->DeleteStateVar( va( "sa_playerList_item_%d", 0 ) );
	mainGui->SetStateString( "sa_playerList_sel_0", "-1" );

	mainGui->DeleteStateVar( va( "sa_banList_item_%d", 0 ) );
	mainGui->SetStateString( "sa_banList_sel_0", "-1" );

	// asalmon: Need to refresh stats periodically if the player is looking at stats
	currentStatClient = -1;
	currentStatTeam = -1;
}

/*
================
idMultiplayerGame::SetMapShot
================
*/
void idMultiplayerGame::SetMapShot( void ) {
#ifdef _XENON	
	// Should not be used
	assert( 0 );
#else
	char screenshot[ MAX_STRING_CHARS ];
	int mapNum = mapList->GetSelection( NULL, 0 );
	const idDict *dict = NULL;
	if ( mapNum >= 0 ) {
		dict = fileSystem->GetMapDecl( mapNum );
	}
	fileSystem->FindMapScreenshot( dict ? dict->GetString( "path" ) : "", screenshot, MAX_STRING_CHARS );
	mainGui->SetStateString( "current_levelshot", screenshot );
// RAVEN BEGIN
// cnicholson: Need to sort the material screenshot so it doesn't overlap other things
	const idMaterial *mat = declManager->FindMaterial( screenshot );
	mat->SetSort( SS_GUI );
// RAVEN END
#endif
}

/*
================
LocalServerRedirect
Dummy local redirect for gui rcon functionality on a local server
================
*/
void LocalServerRedirect( const char* string ) {
	gameLocal.mpGame.ReceiveRemoteConsoleOutput( string );
}

/*
================
idMultiplayerGame::HandleGuiCommands
================
*/
const char* idMultiplayerGame::HandleGuiCommands( const char *_menuCommand ) {
	idUserInterface	*currentGui;
// RAVEN BEGIN
// shouchard:  removed the code that deals with these variables
	//const char		*voteValue;
	//int				vote_clientNum;
// RAVEN END
	int				icmd;
	idCmdArgs		args;



	if ( !_menuCommand[ 0 ] ) {
		common->Printf( "idMultiplayerGame::HandleGuiCommands: empty command\n" );
		return "continue";
	}
	
#ifdef _XENON
	if ( currentMenu == 0 && (session->GetActiveGUI() != scoreBoard) ) {
#else
	if ( currentMenu == 0 ) {
#endif
		return NULL; // this will tell session to not send us events/commands anymore
	}

	if ( currentMenu == 1 ) {
		currentGui = mainGui;
	} 
#ifdef _XENON
	else if (session->GetActiveGUI() != scoreBoard) {
		currentGui = msgmodeGui;
	} else {
		currentGui = scoreBoard;
	}
#else
	else if( currentMenu == 2 ) {
		currentGui = msgmodeGui;
// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	} else if ( currentMenu == 4 ) {
		currentGui = buyMenu;
// jmartel: make sure var is initialized (compiler complained)
	} else if( currentMenu == 3 ) {
		currentGui = statSummary;
	} else {
		gameLocal.Warning( "idMultiplayerGame::HandleGuiCommands() - Unknown current menu '%d'\n", currentMenu );
		currentGui = mainGui;
	}
// RITUAL END
#endif
	

 	args.TokenizeString( _menuCommand, false );

	for( icmd = 0; icmd < args.Argc(); ) {
		const char *cmd = args.Argv( icmd++ );

		if ( !idStr::Icmp( cmd,	";"	) )	{
			continue;
		} else if ( !idStr::Icmp( cmd, "inGameMenu" ) ) {
			if ( args.Argc() - icmd	>= 1 ) {
				idStr igArg = args.Argv( icmd++ );
				if( !igArg.Icmp( "init" ) ) {
					currentGui->SetStateString( "chat", chatHistory.c_str() );

					currentGui->SetStateInt( "player_team", gameLocal.GetLocalPlayer() ? gameLocal.GetLocalPlayer()->team : TEAM_NONE );

					// mekberg: added
					UpdateMPSettingsModel ( currentGui );

					if( gameLocal.gameType == GAME_TOURNEY ) {
						if( !idStr::Icmp( cvarSystem->GetCVarString( "ui_spectate" ), "Spectate" ) ) {
							currentGui->SetStateString( "toggleTourneyButton", common->GetLocalizedString( "#str_107699" ) );
						} else {
							currentGui->SetStateString( "toggleTourneyButton", common->GetLocalizedString( "#str_107700" ) );
						}	
					}
					
					currentGui->SetStateBool( "useReady", gameLocal.serverInfo.GetBool( "si_useReady", "0" ) && gameState->GetMPGameState() == WARMUP );
					if( gameLocal.serverInfo.GetBool( "si_useReady" ) && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->IsReady() && gameState->GetMPGameState() == WARMUP ) {
						currentGui->SetStateString( "readyStatus", common->GetLocalizedString( "#str_104247" ) );
					} else if( gameLocal.serverInfo.GetBool( "si_useReady" ) && gameLocal.GetLocalPlayer() && !gameLocal.GetLocalPlayer()->IsReady() && gameState->GetMPGameState() == WARMUP ) {
						currentGui->SetStateString( "readyStatus", common->GetLocalizedString( "#str_104248" ) );
					} else {
						currentGui->SetStateString( "readyStatus", "" );
					}

					currentGui->SetStateBool( "si_allowVoting", gameLocal.serverInfo.GetBool( "si_allowVoting" ) );
					currentGui->SetStateBool( "si_allowVoice", gameLocal.serverInfo.GetBool( "si_voiceChat" ) );

					int disallowedVotes = gameLocal.serverInfo.GetInt( "si_voteFlags" );
					for( int i = 0; i < NUM_VOTES; i++ ) {
						if( disallowedVotes & (1 << i) ) {
							currentGui->SetStateBool( va( "allowvote_%d", i + 1 ), false );
						} else {
							currentGui->SetStateBool( va( "allowvote_%d", i + 1 ), true );
						}
					}
				}
			}
			continue;
		} else if (	!idStr::Icmp( cmd, "video" ) ) {
			idStr vcmd;
			if ( args.Argc() - icmd	>= 1 ) {
				vcmd = args.Argv( icmd++ );
			}

			if ( idStr::Icmp( vcmd,	"low" )	== 0 ) {
				cvarSystem->SetCVarInteger(	"com_machineSpec", 0 );
			} else if (	idStr::Icmp( vcmd, "medium"	) == 0 ) {
				cvarSystem->SetCVarInteger(	"com_machineSpec", 1 );
			} else	if ( idStr::Icmp( vcmd,	"high" ) ==	0 )	{
				cvarSystem->SetCVarInteger(	"com_machineSpec", 2 );
			} else	if ( idStr::Icmp( vcmd,	"ultra"	) == 0 ) {
				cvarSystem->SetCVarInteger(	"com_machineSpec", 3 );
			} else if (	idStr::Icmp( vcmd, "recommended" ) == 0	) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW,	"setMachineSpec\n" );
			}

// RAVEN BEGIN
// mekberg: set the r_mode.
			cvarSystem->SetCVarInteger( "r_aspectRatio", 0 );
			currentGui->SetStateInt( "r_aspectRatio", 0 );
			currentGui->HandleNamedEvent( "forceAspect0" );
			currentGui->SetStateInt( "com_machineSpec", cvarSystem->GetCVarInteger( "com_machineSpec" ) );
			currentGui->StateChanged( gameLocal.realClientTime );
			cvarSystem->SetCVarInteger( "r_mode", common->GetRModeForMachineSpec ( cvarSystem->GetCVarInteger( "com_machineSpec" ) ) );
			common->SetDesiredMachineSpec( cvarSystem->GetCVarInteger( "com_machineSpec" ) );
// RAVEN END

			cmdSystem->BufferCommandText( CMD_EXEC_NOW,	"execMachineSpec" );
			if ( idStr::Icmp( vcmd,	"restart" )	 ==	0) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart\n" );
			}

			continue;
		} else if (	!idStr::Icmp( cmd, "join" )	) {
			if ( args.Argc() - icmd	>= 1 ) {
				JoinTeam( args.Argv( icmd++ ) );
			}
			continue;
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
		} else if ( !idStr::Icmp( cmd, "admin" ) ) {
			if ( args.Argc() - icmd	>= 1 ) {
				idStr igArg = args.Argv( icmd++ );
				idStr input( currentGui->State().GetString( "admin_console_input" ) );
				input.StripTrailing( "\n" );
				//jshepard: check to see if this is a server before using rcon!
				if( gameLocal.isServer ) {
					char redirectBuffer[ RCON_HISTORY_SIZE ];
					common->BeginRedirect( (char *)redirectBuffer, sizeof( redirectBuffer ), LocalServerRedirect );

					if( !igArg.Icmp( "tab" ) ) {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "tabComplete \"%s\"\n", input.c_str() ) );
					} else if( !igArg.Icmp( "command" ) ) {
						currentGui->SetStateString( "admin_console_input", "" );
						ReceiveRemoteConsoleOutput( input.c_str() );
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "%s\n", input.c_str() ) );
					}

					common->EndRedirect();
				} else {
					if( !igArg.Icmp( "tab" ) ) {
						cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "rcon tabComplete \"%s\"\n", input.c_str() ) );
					} else if( !igArg.Icmp( "command" ) ) {
						currentGui->SetStateString( "admin_console_input", "" );
						cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "rcon \"%s\"\n", input.c_str() ) );
						ReceiveRemoteConsoleOutput( input.c_str() );
					}
				}
			}
			continue;
		} else if (	!idStr::Icmp( cmd, "chatmessage" ) ) {
			int	mode = currentGui->State().GetInt( "messagemode" );
			idStr text = currentGui->GetStateString( "chattext" );
// RAVEN BEGIN	
// bdube: dont send chat message if there was no text specified
			if ( !text.IsEmpty() ) {
				text.Replace( "&", "&amp;" );
				text.Replace( "\\", "&bsl;" );
				if ( mode ) {
					cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "sayTeam \"%s\"", text.c_str() ) );
				} else {
					cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "say \"%s\"", text.c_str() ) );
				}
			}
// RAVEN BEGIN		
			currentGui->SetStateString(	"chattext",	"" );
			if ( currentMenu ==	1 || currentMenu == 3 )	{
				return "continue";
			} else {
				DisableMenu();
				return NULL;
			}
		} else if (	!idStr::Icmp( cmd, "toggleReady" ) ) {
			ToggleReady( );
			DisableMenu( );
			return NULL;
		} else if (	!idStr::Icmp( cmd, "play" )	) {
			if ( args.Argc() - icmd	>= 1 ) {
				idStr snd =	args.Argv( icmd++ );
				int	channel	= 1;
				if ( snd.Length() == 1 ) {
					channel	= atoi(	snd	);
					snd	= args.Argv( icmd++	);
				}
				soundSystem->PlayShaderDirectly( SOUNDWORLD_GAME, snd, channel );
			}
			continue;
		} else if (	!idStr::Icmp( cmd, "callVote" )	) {
// RAVEN BEGIN
// shouchard:  new functionality to match the new interface
			voteStruct_t voteData;
			memset( &voteData, 0, sizeof( voteData ) );
			
			// kick
			int uiKickSelection = mainGui->State().GetInt( "playerList_sel_0" );
			if ( -1 != uiKickSelection ) {
				voteData.m_kick = kickVoteMap[ uiKickSelection ];
				voteData.m_fieldFlags |= VOTEFLAG_KICK;
			}
			// restart
			if ( 0 != mainGui->State().GetInt( "vote_val2_sel" ) ) {
				voteData.m_fieldFlags |= VOTEFLAG_RESTART;
			}

			if ( 0 != mainGui->State().GetBool( "si_shuffleteams" ) ) {
				voteData.m_fieldFlags |= VOTEFLAG_SHUFFLE;
			}
			// map
			int uiMapSelection = mainGui->State().GetInt( "mapList_sel_0" );
			if ( -1 != uiMapSelection ) {
// rjohnson: code commented out below would get the text friendly name of the map and not the file name
				int mapNum = mainGui->State().GetInt( va( "mapList_item_%d_id", uiMapSelection ) );
				if ( mapNum >= 0 ) {
					const idDict *dict = fileSystem->GetMapDecl( mapNum );
					voteData.m_map = dict->GetString( "path" );
					voteData.m_fieldFlags |= VOTEFLAG_MAP;
				}
//				const char *mapName = mainGui->State().GetString( va( "mapList_item_%d", uiMapSelection ) );
//				if ( NULL != mapName && '\0' != mapName[0] ) {
//				if ( mapFileName[ 0 ] ) {
//					voteData.m_map = va( "mp/%s", mapName );
//					voteData.m_fieldFlags |= VOTEFLAG_MAP;
//				}
			}
			// gametype
			// todo:  need a function for switching between gametype strings and values
			int uiGameTypeInt = mainGui->GetStateInt( "currentGametype" );
			const char *currentGameTypeString = gameLocal.serverInfo.GetString( "si_gametype" );
			int serverGameTypeInt = GameTypeToVote( currentGameTypeString );

			if ( uiGameTypeInt != serverGameTypeInt ) {
				voteData.m_gameType = uiGameTypeInt;
				voteData.m_fieldFlags |= VOTEFLAG_GAMETYPE;
			}
			// time limit
			int uiTimeLimit = mainGui->GetStateInt( "timeLimit" );
			if ( uiTimeLimit != gameLocal.serverInfo.GetInt( "si_timeLimit" ) ) {
				voteData.m_timeLimit = uiTimeLimit;
				voteData.m_fieldFlags |= VOTEFLAG_TIMELIMIT;
			}
			// autobalance
			int uiBalanceTeams = mainGui->GetStateInt( "vote_val6_sel" );
			if ( uiBalanceTeams != gameLocal.serverInfo.GetInt( "si_autobalance" ) ) {
				voteData.m_teamBalance = uiBalanceTeams;
				voteData.m_fieldFlags |= VOTEFLAG_TEAMBALANCE;
			}
			// allow spectators
			/* int uiAllowSpectators = mainGui->GetStateInt( "vote_val7_sel" );
			if ( uiAllowSpectators != gameLocal.serverInfo.GetInt( "si_spectators" ) ) {
				voteData.m_spectators = uiAllowSpectators;
				voteData.m_fieldFlags |= VOTEFLAG_SPECTATORS;
			} */
			// minimum players 
			int uiBuying = mainGui->GetStateInt( "buying" );
			if ( uiBuying != gameLocal.serverInfo.GetInt( "si_isBuyingEnabled" ) ) {
				voteData.m_buying = uiBuying;
				voteData.m_fieldFlags |= VOTEFLAG_BUYING;
			} 
			// roundlimit (tourney only)
			int uiTourneyLimit = mainGui->GetStateInt( "tourneylimit" );
			if ( uiTourneyLimit != gameLocal.serverInfo.GetInt( "si_tourneyLimit" ) ) {
				voteData.m_tourneyLimit = uiTourneyLimit;
				voteData.m_fieldFlags |= VOTEFLAG_TOURNEYLIMIT;
			}
			// capturelimit (ctf only)
			int uiCaptureLimit = mainGui->GetStateInt( "capturelimit" );
			if ( uiCaptureLimit != gameLocal.serverInfo.GetInt( "si_captureLimit" ) ) {
				voteData.m_captureLimit = uiCaptureLimit;
				voteData.m_fieldFlags |= VOTEFLAG_CAPTURELIMIT;
			}
			// controltime (deadzone only)
			int uiControlTime = mainGui->GetStateInt( "controlTime" );
			if ( uiControlTime != gameLocal.serverInfo.GetInt( "si_controlTime" ) ) {
				voteData.m_controlTime = uiControlTime;
				voteData.m_fieldFlags |= VOTEFLAG_CONTROLTIME;
			}
			// fraglimit (DM & TDM only)
			int uiFragLimit = mainGui->GetStateInt( "fraglimit" );
			if ( uiFragLimit != gameLocal.serverInfo.GetInt( "si_fragLimit" ) ) {
				voteData.m_fragLimit = uiFragLimit;
				voteData.m_fieldFlags |= VOTEFLAG_FRAGLIMIT;
			}
			DisableMenu();

			// clear any disallowed votes
			int disallowedVotes = gameLocal.serverInfo.GetInt( "si_voteFlags" );
			for( int i = 0; i < NUM_VOTES; i++ ) {
				if( disallowedVotes & (1 << i) ) {
					voteData.m_fieldFlags &= ~(1 << i);
				}
			}

			// this means we haven't changed anything
			if ( 0 == voteData.m_fieldFlags ) {
				//AddChatLine( common->GetLocalizedString( "#str_104400" ) );
			} else {
				ClientCallPackedVote( voteData );
			}
			/*
			// sjh:  original doom code here
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
			*/
			return NULL;
		} else if ( !idStr::Icmp( cmd, "voteYes" ) ) {
			gameLocal.mpGame.CastVote( gameLocal.localClientNum, true );
			DisableMenu();
			return NULL;
		} else if ( !idStr::Icmp( cmd, "voteNo" ) ) {
			gameLocal.mpGame.CastVote( gameLocal.localClientNum, false );
			DisableMenu();
			return NULL;
		} else if ( !idStr::Icmp( cmd, "click_playerList" ) ) {
			// push data into the name field
			int sel = mainGui->GetStateInt( "playerList_sel_0" );
			if ( -1 == sel ) {
				mainGui->SetStateString( "playerKick", "" );
			} else { 
				mainGui->SetStateString( "playerKick", kickVoteMapNames[ sel ] );
			}
			continue;
		} else if ( !idStr::Icmp( cmd, "click_voteMapList" ) ) {
			int sel = mainGui->GetStateInt( "mapList_sel_0" );
			if ( -1 == sel ) {
				mainGui->SetStateString( "mapName", "" );
			} else {
				mainGui->SetStateString( "mapName", mainGui->GetStateString( va( "mapList_item_%d", sel ) ) );
			}
			continue;
		} else if ( !idStr::Icmp( cmd, "setVoteMapList" ) ) {
#ifdef _XENON
			// Xenon should not get here
			assert( 0 );
#else
			SetVoteMapList();
#endif
			continue;
		} else if ( !idStr::Icmp( cmd, "setVoteData" ) ) {
#ifdef _XENON
			// Xenon should not get here
			assert( 0 );
#else
			// push data into the vote_ cvars so the UI can start at where we currently are
			int players;
			for ( players=0; players<gameLocal.numClients; players++ ) { 
				mainGui->SetStateString( va( "playerList_item_%d", players ), kickVoteMapNames[players] );
			}
			if ( players < MAX_CLIENTS ) {
				mainGui->DeleteStateVar( va( "playerList_item_%d", players ) );
			}
			mainGui->SetStateString( "playerList_sel_0", "-1" );
			mainGui->SetStateString( "playerKick", "" );
			mainGui->SetStateInt( "vote_val2_sel", 0 );


// RAVEN BEGIN
// mekberg: get localized string.
			const char *mapName = gameLocal.serverInfo.GetString( "si_map" );
			const idDict *mapDict = fileSystem->GetMapDecl( mapName );
			if ( mapDict ) {
				mapName = common->GetLocalizedString( mapDict->GetString( "name", mapName ) );
			}
			mainGui->SetStateString( "mapName", mapName );
// RAVEN END

			const char *currentGameTypeString = gameLocal.serverInfo.GetString( "si_gameType" );
			int uiGameTypeInt = GameTypeToVote( currentGameTypeString );
			mainGui->SetStateInt( "currentGametype", uiGameTypeInt );
			mainGui->SetStateInt( "timelimit", gameLocal.serverInfo.GetInt( "si_timeLimit" ) );
			mainGui->SetStateInt( "tourneylimit", gameLocal.serverInfo.GetInt( "si_tourneyLimit" ) );
			mainGui->SetStateInt( "capturelimit", gameLocal.serverInfo.GetInt( "si_captureLimit" ) );
			mainGui->SetStateInt( "controlTime", gameLocal.serverInfo.GetInt( "si_controlTime" ) );
			mainGui->SetStateInt( "vote_val6_sel", gameLocal.serverInfo.GetInt( "si_autobalance" ) );
			mainGui->SetStateInt( "buying", gameLocal.serverInfo.GetInt( "si_isBuyingEnabled" ) );
			mainGui->SetStateInt( "fraglimit", gameLocal.serverInfo.GetInt( "si_fraglimit" ) );
			mainGui->SetStateInt( "si_shuffleteams", 0 );
			mainGui->StateChanged( gameLocal.time );
			mainGui->HandleNamedEvent( "gametypeChange" );
#endif

			SetVoteMapList();
			continue;
		} else if ( !idStr::Icmp( cmd, "populateServerInfo" ) ) {
#ifdef _XENON
			// Xenon should not get here
			assert( 0 );
#else
			mainGui->SetStateString( "serverInfoList_item_0", va( "%s:\t%s", common->GetLocalizedString( "#str_107725" ), gameLocal.serverInfo.GetString( "si_name" ) ) );
			idStr serverAddress = networkSystem->GetServerAddress( );
			mainGui->SetStateString( "serverInfoList_item_1", va( "%s:\t%s", common->GetLocalizedString( "#str_107726" ), serverAddress.c_str() ) );
			mainGui->SetStateString( "serverInfoList_item_2", va( "%s:\t%s", common->GetLocalizedString( "#str_107727" ), LocalizeGametype() ) );

			const char *mapName = gameLocal.serverInfo.GetString( "si_map" );
			const idDict *mapDict = fileSystem->GetMapDecl( mapName );
			if ( mapDict ) {
				mapName = common->GetLocalizedString( mapDict->GetString( "name", mapName ) );
			}
// rhummer localized "map name"
			mainGui->SetStateString( "serverInfoList_item_3", va( "%s\t%s", common->GetLocalizedString( "#str_107730" ), mapName ) );
			const char *gameType = gameLocal.serverInfo.GetString( "si_gametype" );
			if ( 0 == idStr::Icmp( gameType, "CTF" ) ) {
				mainGui->SetStateString( "serverInfoList_item_4", va( "%s:\t%s", common->GetLocalizedString( "#str_107661" ), gameLocal.serverInfo.GetString( "si_captureLimit" ) ) );
			}
			else if ( 0 == idStr::Icmp( gameType, "DM" ) || 0 == idStr::Icmp( gameType, "Team DM" ) ) {
				mainGui->SetStateString( "serverInfoList_item_4", va( "%s:\t%s", common->GetLocalizedString( "#str_107660" ), gameLocal.serverInfo.GetString( "si_fragLimit" ) ) );
			}
			mainGui->SetStateString( "serverInfoList_item_5", va( "%s:\t%s", common->GetLocalizedString( "#str_107659" ), gameLocal.serverInfo.GetString( "si_timeLimit" ) ) );
			mainGui->SetStateString( "serverInfoList_item_6", va( "%s:\t%s", common->GetLocalizedString( "#str_107662" ), gameLocal.serverInfo.GetString( "si_pure" ) ) );
			mainGui->SetStateString( "serverInfoList_item_7", va( "%s:\t%s", common->GetLocalizedString( "#str_107663" ), gameLocal.serverInfo.GetString( "si_maxPlayers" ) ) );
			mainGui->SetStateString( "serverInfoList_item_8", va( "%s:\t%s", common->GetLocalizedString( "#str_107664" ), gameLocal.serverInfo.GetString( "si_teamDamage" ) ) );
			mainGui->SetStateString( "serverInfoList_item_9", va( "%s:\t%s", common->GetLocalizedString( "#str_104254" ), gameLocal.serverInfo.GetString( "si_spectators" ) ) );
#endif
			continue;
		// handler for the server admin tab (normal stuff)
		} else if ( !idStr::Icmp( cmd, "checkAdminPass" )) {
			//password has been added, so call the rcon verifypassword command
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rcon verifyRconPass" );			
			continue;
	
		} else if ( !idStr::Icmp( cmd, "initServerAdmin" ) ) {
			mainGui->SetStateInt( "admin_server_val1_sel", 0 ); // restart defaults to off
			// maplist handled in initServerAdminMaplist; this needs to be called first
			// to properly set the gametype since we read it back to show an appropriate list
			const char *currentGameTypeString = gameLocal.serverInfo.GetString( "si_gameType" );
			int uiGameTypeInt = GameTypeToVote( currentGameTypeString );
			mainGui->SetStateInt( "admincurrentGametype", uiGameTypeInt );
			mainGui->SetStateInt( "sa_timelimit", gameLocal.serverInfo.GetInt( "si_timeLimit" ) );
			mainGui->SetStateInt( "sa_tourneylimit", gameLocal.serverInfo.GetInt( "si_tourneyLimit" ) );
			mainGui->SetStateInt( "sa_capturelimit", gameLocal.serverInfo.GetInt( "si_captureLimit" ) );
			mainGui->SetStateInt( "sa_controlTime", gameLocal.serverInfo.GetInt( "si_controlTime" ) );
			mainGui->SetStateInt( "sa_autobalance", gameLocal.serverInfo.GetInt( "si_autobalance" ) );
			mainGui->SetStateInt( "sa_buying", gameLocal.serverInfo.GetInt( "si_isBuyingEnabled" ) );
			mainGui->SetStateInt( "sa_fraglimit", gameLocal.serverInfo.GetInt( "si_fraglimit" ) );
			mainGui->SetStateInt( "sa_shuffleteams", 0 );
// mekberg: get the ban list if not server
			if ( !gameLocal.isServer ) {
				idBitMsg	outMsg;
				byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

				outMsg.Init( msgBuf, sizeof( msgBuf ) );
				outMsg.WriteByte( GAME_RELIABLE_MESSAGE_GETADMINBANLIST ) ;
				networkSystem->ClientSendReliableMessage( outMsg );
			}
	
			mainGui->StateChanged( gameLocal.time );

			continue;
		// handler for populating the map list; called both on open and on change gametype so it'll show the right maps
		} else if ( !idStr::Icmp( cmd, "initServerAdminMapList" ) ) {
#ifdef _XENON
			// Xenon should not get here
			assert( 0 );
#else
			SetSAMapList();
#endif
			continue;
		// handler for updating the current map in the name
		} else if ( !idStr::Icmp( cmd, "serverAdminUpdateMap" ) ) {
			int mapSelection = mainGui->GetStateInt( "sa_mapList_sel_0" );
			if ( -1 == mapSelection ) {
				
				const idDict *mapDict = fileSystem->GetMapDecl( gameLocal.serverInfo.GetString( "si_map" ) );
				if ( mapDict ) {
					mainGui->SetStateString( "sa_mapName", common->GetLocalizedString( mapDict->GetString( "name" )) );
				} else {
					mainGui->SetStateString( "sa_mapName", gameLocal.serverInfo.GetString( "si_map" ) );
				}
			
			} else {
				int mapNum = mainGui->State().GetInt( va( "sa_mapList_item_%d_id", mapSelection ) );
				if ( mapNum >= 0 ) {
					const idDict *dict = fileSystem->GetMapDecl( mapNum );
					mainGui->SetStateString( "sa_mapName", common->GetLocalizedString( dict->GetString( "name" )) );
				}
			}
			continue;
		// handler for initializing the player list on the admin player tab
		} else if ( !idStr::Icmp( cmd, "initServerAdminPlayer" ) ) {
			int players;
			for ( players=0; players<gameLocal.numClients; players++ ) { 
				mainGui->SetStateString( va( "sa_playerList_item_%d", players ), kickVoteMapNames[players] );
			}
			if ( players < MAX_CLIENTS ) {
				mainGui->DeleteStateVar( va( "sa_playerList_item_%d", players ) );
				//common->Printf( "DELETING at slot %d\n", players );
			}
			mainGui->SetStateString( "sa_playerList_sel_0", "-1" );
			continue;
		// handler for actually changing something on the server admin tab
		} else if ( !idStr::Icmp( cmd, "handleServerAdmin" ) ) {
			// read in a bunch of data, pack it into the appropriate structure
			serverAdminData_t data;
			memset( &data, 0, sizeof( data ) );
			data.restartMap = 0 != mainGui->GetStateInt( "admin_server_val1_sel" );
			// map list here
			int uiMapSelection = mainGui->State().GetInt( "sa_mapList_sel_0" );
			if (-1 != uiMapSelection ) {
				int mapNum = mainGui->State().GetInt( va( "sa_mapList_item_%d_id", uiMapSelection ) );
				if ( mapNum >= 0 ) {
					const idDict *dict = fileSystem->GetMapDecl( mapNum );
					data.mapName = common->GetLocalizedString( dict->GetString( "path" ));
				} else { 
					data.mapName = gameLocal.serverInfo.GetString( "si_map" );
				}		
			} else { 
				data.mapName = gameLocal.serverInfo.GetString( "si_map" );
			}

			switch ( mainGui->GetStateInt( "admincurrentGametype" ) ) {
				case VOTE_GAMETYPE_DM:
					data.gameType = GAME_DM;
					break;
				case VOTE_GAMETYPE_TOURNEY:
					data.gameType = GAME_TOURNEY;
					break;
				case VOTE_GAMETYPE_TDM:
					data.gameType = GAME_TDM;
					break;
				case VOTE_GAMETYPE_CTF:
					data.gameType = GAME_CTF;
					break;
				case VOTE_GAMETYPE_ARENA_CTF:
					data.gameType = GAME_ARENA_CTF;
					break;
				case VOTE_GAMETYPE_DEADZONE:
					data.gameType = GAME_DEADZONE;
			}
			data.captureLimit = mainGui->GetStateInt( "sa_captureLimit" );
			data.fragLimit = mainGui->GetStateInt( "sa_fragLimit" );
			data.tourneyLimit = mainGui->GetStateInt( "sa_tourneylimit" );
			data.timeLimit = mainGui->GetStateInt( "sa_timeLimit" );
			data.buying = mainGui->GetStateInt( "sa_buying" );
			data.autoBalance = 0 != mainGui->GetStateInt( "sa_autobalance" );
			data.buying = 0 != mainGui->GetStateInt( "sa_buying" );
			data.controlTime = mainGui->GetStateInt( "sa_controlTime" );
			data.shuffleTeams = 0 != mainGui->GetStateInt( "sa_shuffleteams" );

			// make the call to change the server data
			if ( gameLocal.mpGame.HandleServerAdminCommands( data ) ) {
				DisableMenu();
				return NULL;
			}
			continue;
		// handler for the kick button on the player tab of the server admin gui
		} else if ( !idStr::Icmp( cmd, "handleServerAdminKick" ) ) {
			int uiKickSelection = mainGui->State().GetInt( "sa_playerList_sel_0" );
			if ( -1 != uiKickSelection ) {
				HandleServerAdminKickPlayer( kickVoteMap[ uiKickSelection ] );
				DisableMenu();
				return NULL;
			}
			//common->Printf( "HANDLE SERVER ADMIN KICK!\n" );
			continue;
		// handler for the ban button on the player tab of the server admin gui
		} else if ( !idStr::Icmp( cmd, "handleServerAdminBan" ) ) {
			//common->Printf( "HANDLE SERVER ADMIN BAN!\n" );
			int uiBanSelection = mainGui->State().GetInt( "sa_playerList_sel_0" );
			if ( -1 != uiBanSelection ) {
				HandleServerAdminBanPlayer( kickVoteMap[ uiBanSelection ] );
				DisableMenu();
				mainGui->DeleteStateVar( va( "sa_banList_item_%d", 0 ) );
				mainGui->SetStateString( "sa_banList_sel_0", "-1" );
				return NULL;
			}
			continue;
		// handler for the remove ban button on the player tab of the server admin gui
		} else if ( !idStr::Icmp( cmd, "handleServerAdminRemoveBan" ) ) {
			//common->Printf( "HANDLE SERVER ADMIN REMOVE BAN!\n" );
			int uiBanSelection = mainGui->State().GetInt( "sa_banList_sel_0" );
			if ( -1 != uiBanSelection ) {
				idStr guid = &mainGui->GetStateString( va( "sa_banList_item_%d", uiBanSelection ) )[ 4 ];
				guid = guid.ReplaceChar( '\t', '\0' );
				guid = &guid.c_str()[ strlen( guid.c_str() ) + 1 ];
				HandleServerAdminRemoveBan( guid.c_str() );
				DisableMenu();
				return NULL;
			}
			continue;
		// handler for the switch teams button on the player tab of the server admin gui
		} else if ( !idStr::Icmp( cmd, "handleServerAdminSwitchTeams" ) ) {
			if ( gameLocal.IsTeamGame() ) {
				int uiSwitchSelection = mainGui->State().GetInt( "sa_playerList_sel_0" );
				if ( -1 != uiSwitchSelection ) {
					HandleServerAdminForceTeamSwitch( kickVoteMap[ uiSwitchSelection ] );
					DisableMenu();
					return NULL;
				}
			}
			continue;
		// handler for the show ban list button of the server admin gui
		} else if ( !idStr::Icmp( cmd, "populateBanList" ) ) {
			gameLocal.PopulateBanList( mainGui );
			continue;
// RAVEN END
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
		} else if (	!idStr::Icmp( cmd, "MAPScan" ) ) {
#ifdef _XENON
			// Xenon should not get here
			assert( 0 );
#else
			const char *gametype = gameLocal.serverInfo.GetString( "si_gameType" );
			if ( gametype == NULL || *gametype == 0 || idStr::Icmp( gametype, "singleplayer" ) == 0 ) {
				gametype = "DM";
			}

			int i, num;
			idStr si_map = gameLocal.serverInfo.GetString("si_map");
			const idDict *dict;

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
						const char *mapName = dict->GetString( "name" );
						if ( mapName[0] == '\0' ) {
							mapName = dict->GetString( "path" );
						}
						mapName = common->GetLocalizedString( mapName );
						mapList->Add( i, mapName );
						if ( !si_map.Icmp( dict->GetString( "path" ) ) ) {
							mapList->SetSelection( mapList->Num() - 1 );
						}
					}
				}
			}
			// set the current level shot
			SetMapShot(	);
#endif
			return "continue";
		} else if (	!idStr::Icmp( cmd, "click_maplist" ) ) {
			SetMapShot(	);
			return "continue";
		} else if ( !idStr::Icmp( cmd, "sm_select_player" ) ) {
			idStr vcmd;
			if ( args.Argc() - icmd	>= 1 ) {
				vcmd = args.Argv( icmd++ );
			} 

			int index = atoi( vcmd.c_str() );
			if( index > 0 && index < MAX_CLIENTS && statSummary && currentMenu == 3 ) {
				statManager->UpdateEndGameHud( statSummary, index - 1 );
			}
			return "continue";
		} else if ( !idStr::Icmp( cmd, "update_model" ) ) {
			UpdateMPSettingsModel( currentGui );
			continue;
		} else if( !idStr::Icmp( cmd, "ingameStats" ) ) {
			if ( args.Argc() - icmd	>= 1 ) {
				idStr igArg = args.Argv( icmd++ );
				if( !igArg.Icmp( "init" ) ) {
					// setup the player list
					statManager->SetupStatWindow( currentGui );
				} else if( !igArg.Icmp( "spectator" ) ) {
					int currentSel = currentGui->State().GetInt( "spec_names_sel_0", "-1" );
					currentGui->SetStateString( "dm_names_sel_0", "-1" );
					currentGui->SetStateString( "team_1_names_sel_0", "-1" );
					currentGui->SetStateString( "team_2_names_sel_0", "-1" );

					statManager->SelectStatWindow( currentSel, TEAM_MAX );
					// asalmon: Need to refresh stats periodically if the player is looking at stats
					currentStatClient = currentSel;
					currentStatTeam = TEAM_MAX;
				} else if( !igArg.Icmp( "dm" ) ) {
					int currentSel = currentGui->State().GetInt( "dm_names_sel_0", "-1" );
					currentGui->SetStateString( "spec_names_sel_0", "-1" );
					currentGui->SetStateString( "team_1_names_sel_0", "-1" );
					currentGui->SetStateString( "team_2_names_sel_0", "-1" );

					statManager->SelectStatWindow( currentSel, 0 );
					// asalmon: Need to refresh stats periodically if the player is looking at stats
					currentStatClient = currentSel;
					currentStatTeam = 0;
				} else if( !igArg.Icmp( "strogg" ) ) {
					int currentSel = currentGui->State().GetInt( "team_2_names_sel_0", "-1" );
					currentGui->SetStateString( "spec_names_sel_0", "-1" );
					currentGui->SetStateString( "team_1_names_sel_0", "-1" );
					currentGui->SetStateString( "dm_names_sel_0", "-1" );

					statManager->SelectStatWindow( currentSel, TEAM_STROGG );
					// asalmon: Need to refresh stats periodically if the player is looking at stats
					currentStatClient = currentSel;
					currentStatTeam = TEAM_STROGG;
				} else if( !igArg.Icmp( "marine" ) ) {
					int currentSel = currentGui->State().GetInt( "team_1_names_sel_0", "-1" );
					currentGui->SetStateString( "spec_names_sel_0", "-1" );
					currentGui->SetStateString( "team_2_names_sel_0", "-1" );
					currentGui->SetStateString( "dm_names_sel_0", "-1" );

					statManager->SelectStatWindow( currentSel, TEAM_MARINE );
					// asalmon: Need to refresh stats periodically if the player is looking at stats
					currentStatClient = currentSel;
					currentStatTeam = TEAM_MARINE;
				}
			}
			continue;
		} else if( !idStr::Icmp( cmd, "mainMenu" ) ) {
			DisableMenu();
			static idStr menuCmd;
			menuCmd.Clear();						// cnicholson: In order to avoid repeated eventnames from screwing up the menu system, clear it.
			menuCmd.Append( "main" );
			const char* eventName = "";
			if( args.Argc() - icmd >= 1 ) {
				eventName = args.Argv( icmd++ );
				menuCmd.Append( " " );
				menuCmd.Append( eventName );
			}
			return menuCmd.c_str();
		} 
// RAVEN BEGIN
// cnicholson: The menu calls this prior to entering multiplayer settings. What it does is to check the current crosshair, and compare it
//			   agasint the list of crosshairs in player.def under the player_marine_mp section. If it finds a match, it assigns the 
//			   crosshair to the next one in the list. If there isn't one, or if its the end of the list, the first found crosshair is used.
		else if ( !idStr::Icmp( cmd, "chooseCrosshair" ) ) {
#ifndef _XENON

#ifndef _XENON
			const idDeclEntityDef *def = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, "player_marine_mp", false, true ) );
#else
			bool insideLevelLoad = declManager->GetInsideLoad();
			if ( !insideLevelLoad ) {
				declManager->SetInsideLoad( true );
			}
			const idDeclEntityDef *def = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, "player_marine_mp_ui", false, false ) );
			declManager->SetInsideLoad( insideLevelLoad );
#endif

			idStr currentCrosshair = cvarSystem->GetCVarString("g_crosshairCustomFile");

			const idKeyValue* kv = def->dict.MatchPrefix("mtr_crosshair", NULL);
	
			while ( kv ) {
				if ( kv->GetValue() == currentCrosshair.c_str() ) {
					kv = def->dict.MatchPrefix("mtr_crosshair", kv );
					break;
				}
				kv = def->dict.MatchPrefix("mtr_crosshair", kv );
			}

			if ( !kv ){
				kv = def->dict.MatchPrefix("mtr_crosshair", NULL );
			}

			idStr newCrosshair(kv->GetValue());

			mainGui->SetStateString ( "crossImage", newCrosshair.c_str());
			const idMaterial *material = declManager->FindMaterial( newCrosshair.c_str() );
			if ( material ) {
				material->SetSort( SS_GUI );
			}			

			cvarSystem->SetCVarString("g_crosshairCustomFile", newCrosshair.c_str());
#endif		
		}
// RAVEN END
		else if( !idStr::Icmp( cmd, "friend" ) ) {
			// we friend/unfriend from the stat window, so use that to get selection info
			int selectionTeam = -1;
			int selectionIndex = -1;

			// get the selected client num, as well as the selectionIndex/Team from the stat window
			int client = statManager->GetSelectedClientNum( &selectionIndex, &selectionTeam );

			if( ( client < 0 || client >= MAX_CLIENTS ) || !gameLocal.GetLocalPlayer() ) {
				continue;
			}

			// un-mark this client as a friend
			if( gameLocal.GetLocalPlayer() ) {
				if( gameLocal.GetLocalPlayer()->IsFriend( client ) ) {
					networkSystem->RemoveFriend( client );
				} else {
						networkSystem->AddFriend( client );
				}
			}
			
			// refresh with new info
			statManager->SetupStatWindow( currentGui );
			statManager->SelectStatWindow( selectionIndex, selectionTeam );
			continue;
		} else if( !idStr::Icmp( cmd, "mute" ) ) {
			// we mute/unmute from the stat window, so use that to get selection info
			int selectionTeam = -1;
			int selectionIndex = -1;

			// get the selected client num, as well as the selectionIndex/Team from the stat window
			int client = statManager->GetSelectedClientNum( &selectionIndex, &selectionTeam );

			
			if ( gameLocal.GetLocalPlayer() ) {
				ClientVoiceMute( client, !gameLocal.GetLocalPlayer()->IsPlayerMuted( client ) );
			}

			// refresh with new info
			statManager->SetupStatWindow( currentGui );
			statManager->SelectStatWindow( selectionIndex, selectionTeam );

			continue;
		}
//RAVEN BEGIN
//asalmon: pass through some commands that need to be handled in the main menu handle function
		else if(strstr( cmd, "LiveInviteAccept" ) == cmd){
#ifdef _XENON
			Live()->SetInvite();
#endif
		}
		else if ((strstr( cmd, "FilterMPMapList" ) == cmd) 
			|| (strstr( cmd, "AddMapLive" ) == cmd) 
			|| (strstr( cmd, "RemoveMapLive" ) == cmd) 
		) {
			static idStr menuCmd;
			menuCmd.Clear();						
			menuCmd.Append( cmd );
			return menuCmd.c_str();
		} else if( !idStr::Icmp( cmd, "toggleTourney" ) ) {
			if( gameLocal.gameType == GAME_TOURNEY ) {
				ToggleSpectate();
				DisableMenu( );
				return NULL;
			}
			continue;
		}
//RAVEN END
		common->Printf(	"idMultiplayerGame::HandleGuiCommands: '%s'	unknown\n",	cmd	);

	}
	return "continue";
}

/*
===============
idMultiplayerGame::SetShaderParms
===============
*/
void idMultiplayerGame::SetShaderParms( renderView_t *view ) {
	if ( gameLocal.IsFlagGameType() ) {
		view->shaderParms[ 1 ] = ( ((rvCTFGameState*)GetGameState())->GetFlagState( TEAM_MARINE ) != FS_AT_BASE );
		view->shaderParms[ 2 ] = ( ((rvCTFGameState*)GetGameState())->GetFlagState( TEAM_STROGG ) != FS_AT_BASE );
	}	
}

/*
================
idMultiplayerGame::Draw
server demo: clientNum == MAX_CLIENTS
================
*/
bool idMultiplayerGame::Draw( int clientNum ) {
	idPlayer *player, *viewPlayer;
	idUserInterface *hud = NULL;

	if ( clientNum == MAX_CLIENTS ) {
//		assert( gameLocal.GetDemoState() == DEMO_PLAYING );
		clientNum = ENTITYNUM_NONE;
	}

	player = viewPlayer = static_cast<idPlayer *>( gameLocal.entities[ clientNum ] );

	if ( player == NULL ) {
		return false;
	}

	if ( player->spectating ) {
		viewPlayer = static_cast<idPlayer *>( gameLocal.entities[ player->spectator ] );
		if ( viewPlayer == NULL ) {
			return false;
		}
	}

	if ( !viewPlayer->GetRenderView() ) {
		return false;
	}

	SetShaderParms( viewPlayer->GetRenderView() );

	// use the hud of the local player
	if ( !hud ) {
		hud = player->hud;
	}
	viewPlayer->playerView.RenderPlayerView( hud );

	// allow force scoreboard to overwrite a fullscreen menu
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
			mainGui->SetStateString( "spectext", common->GetLocalizedString( "#str_104249" ) );
		} else {
			mainGui->SetStateString( "spectext", common->GetLocalizedString( "#str_104250" ) );
		}
		// if we died, isChatting is cleared, so re-set our chatting cvar
		if ( gameLocal.GetLocalPlayer() && !gameLocal.GetLocalPlayer()->IsFakeClient() && !gameLocal.GetLocalPlayer()->isChatting && !gameLocal.GetLocalPlayer()->pfl.dead ) {
			cvarSystem->SetCVarBool( "ui_chat", true );
			cvarSystem->SetModifiedFlags( CVAR_USERINFO ); // force update
		}
		if ( currentMenu == 1 ) {
			UpdateMainGui();
			mainGui->Redraw( gameLocal.time );
		} else if( currentMenu == 2 ) {
			msgmodeGui->Redraw( gameLocal.time );
		} else if( currentMenu == 3 ) {
			DrawStatSummary();
// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
		} else if( currentMenu == 4 ) {
			SetupBuyMenuItems();
			player->UpdateHudStats( buyMenu );
			buyMenu->HandleNamedEvent( "update_buymenu" );
			idPlayer* player = gameLocal.GetLocalPlayer();
			buyMenu->SetStateString( "field_credits", va("%i", (int)player->buyMenuCash) );
			buyMenu->Redraw(gameLocal.time);
// RITUAL END
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
		DrawScoreBoard( player );
	}

// RAVEN BEGIN
// bdube: debugging HUD
	gameDebug.DrawHud();
// RAVEN END
	return true;
}

/*
================
idMultiplayerGame::UpdateHud
================
*/
void idMultiplayerGame::UpdateHud( idUserInterface* _mphud ) {
	idPlayer *localPlayer;

	if ( !_mphud ) {
		return;
	}

	// server demos don't have a true local player, but need one for hud updates
	localPlayer = gameLocal.GetLocalPlayer();
	if ( !localPlayer ) {
		assert( gameLocal.IsServerDemoPlaying() );
		assert( gameLocal.GetDemoFollowClient() >= 0 );
		assert( gameLocal.entities[ gameLocal.GetDemoFollowClient() ] && gameLocal.entities[ gameLocal.GetDemoFollowClient() ]->IsType( idPlayer::GetClassType() ) );
		localPlayer = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.GetDemoFollowClient() ] );
	}

//RAVEN BEGIN
//asalmon: Turn on/off the lag icon so that clients know that they are losing connection
	if (  networkSystem->ClientGetTimeSinceLastPacket() > 0 && ( networkSystem->ClientGetTimeSinceLastPacket() > cvarSystem->GetCVarInteger("net_clientServerTimeout")*500 ) ) {
		_mphud->SetStateBool("IsLagged", true);
	}
	else{
		_mphud->SetStateBool("IsLagged", false);
	}
//RAVEN END

	_mphud->SetStateInt( "marine_score", teamScore[ TEAM_MARINE ] );
	_mphud->SetStateInt( "strogg_score", teamScore[ TEAM_STROGG ] );

	int timeLimit = gameLocal.serverInfo.GetInt( "si_timeLimit" );
	
	// Always show GameTime() for WARMUP and COUNTDOWN.
	mpGameState_t state = gameState->GetMPGameState();
	_mphud->SetStateString( "timeleft", GameTime() );

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	/// Set "credits" gui element
	if( gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() ) {
		int cash = 0;
		idPlayer* localPlayer = gameLocal.GetLocalPlayer();
		if ( !localPlayer ) {
			assert( gameLocal.IsServerDemoPlaying() );
			assert( gameLocal.GetDemoFollowClient() >= 0 );
			assert( gameLocal.entities[ gameLocal.GetDemoFollowClient() ] && gameLocal.entities[ gameLocal.GetDemoFollowClient() ]->IsType( idPlayer::GetClassType() ) );
			localPlayer = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.GetDemoFollowClient() ] );
		}

		idPlayer* specPlayer = NULL;
		if ( localPlayer->spectating )
			specPlayer = gameLocal.GetClientByNum( localPlayer->spectator );
		
		if ( specPlayer )
			cash = (int)specPlayer->buyMenuCash;
		else
			cash = (int)localPlayer->buyMenuCash;

		if( localPlayer->CanBuy() ) {
			_mphud->SetStateString("credits", va("%s %d  %s", common->GetLocalizedString( "#str_122015" ), 
				cash, common->GetLocalizedString( "#str_122016" )));
		}
		else {
			_mphud->SetStateString("credits", va("%s %d", common->GetLocalizedString( "#str_122015" ), cash));
		}
	}
	else
	{
		_mphud->SetStateString("credits", "");
	}
// RITUAL END


	bool inNonTimedState = (state == SUDDENDEATH) || (state == WARMUP) || (state == GAMEREVIEW);
	bool inCountdownState = (state == COUNTDOWN);
	if( gameLocal.gameType == GAME_TOURNEY ) {
		inNonTimedState |= (((rvTourneyGameState*)gameState)->GetArena( localPlayer->GetArena() ).GetState() == AS_SUDDEN_DEATH);
		inCountdownState |= (((rvTourneyGameState*)gameState)->GetArena( localPlayer->GetArena() ).GetState() == AS_WARMUP);
	}
	_mphud->SetStateBool( "infinity", ( !timeLimit && !inCountdownState ) || inNonTimedState );

	if( gameLocal.gameType == GAME_DM ) {
		if( rankedPlayers.Num() ) {
			_mphud->SetStateString( "player1_name", rankedPlayers[ 0 ].First()->GetUserInfo()->GetString( "ui_name" ) );
			_mphud->SetStateString( "player1_score", va( "%d", GetScore( rankedPlayers[ 0 ].First() ) ) );
			_mphud->SetStateString( "player1_rank", "1." );

			// if we're in the lead or spectating, show the person in 2nd
			if( ( (rankedPlayers[ 0 ].First() == localPlayer) || (localPlayer->spectating) ) && rankedPlayers.Num() > 1 ) {
				_mphud->SetStateString( "player2_name", rankedPlayers[ 1 ].First()->GetUserInfo()->GetString( "ui_name" ) );
				_mphud->SetStateString( "player2_score", va( "%d", GetScore( rankedPlayers[ 1 ].First() ) ) );
				_mphud->SetStateString( "player2_rank", va( "%d.", rankedPlayers[ 1 ].First()->GetRank() + 1 ) );
			} else if( rankedPlayers[ 0 ].First() != localPlayer && !localPlayer->spectating ) {
				// otherwise, show our score
				_mphud->SetStateString( "player2_name", localPlayer->GetUserInfo()->GetString( "ui_name" ) );
				_mphud->SetStateString( "player2_score", va( "%d", GetScore( localPlayer ) ) );
				_mphud->SetStateString( "player2_rank", va( "%d.", localPlayer->GetRank() + 1 ) );
			} else {
				// no person to place in 2nd
				_mphud->SetStateString( "player2_name", "" );
				_mphud->SetStateString( "player2_score", "" );
				_mphud->SetStateString( "player2_rank", "" );
			}
		} else {
			_mphud->SetStateString( "player1_name", "" );
			_mphud->SetStateString( "player1_score", "" );
			_mphud->SetStateString( "player1_rank", "" );

			_mphud->SetStateString( "player2_name", "" );
			_mphud->SetStateString( "player2_score", "" );
			_mphud->SetStateString( "player2_rank", "" );
		}
	} 
	
	// RITUAL BEGIN
	// squirrel: added DeadZone multiplayer mode
	if( gameLocal.gameType == GAME_DEADZONE ) {

		static int lastMarineScore = teamScore[ TEAM_MARINE ];
		static int lastStroggScore = teamScore[ TEAM_STROGG ];
		int marineScore = teamScore[ TEAM_MARINE ];
		int stroggScore = teamScore[ TEAM_STROGG ];
		const float asymptoticAverageWeight = 0.95f;

		/// Check if Marines have scored since last frame
		if( marineScore != lastMarineScore )
		{
			/// Pulse the bar's color
			marineScoreBarPulseAmount = 1.0f;

			// Play the pulse sound
			idStr pulseSnd = "snd_dzpulse_happy";
			if ( localPlayer->team != TEAM_MARINE )
				pulseSnd = "snd_dzpulse_unhappy";

			localPlayer->StartSound( pulseSnd, SND_CHANNEL_ANY, 0, false, NULL );
		}
		else
		{
			/// Asymptotic-average back to the normal color
			marineScoreBarPulseAmount *= asymptoticAverageWeight;
		}

		/// Check if Strogg have scored since last frame
		if( stroggScore != lastStroggScore )
		{
			/// Pulse the bar's color
			stroggScoreBarPulseAmount = 1.0f;

			// Play the pulse sound
			idStr pulseSnd = "snd_dzpulse_happy";
			if ( localPlayer->team != TEAM_STROGG )
				pulseSnd = "snd_dzpulse_unhappy";

			localPlayer->StartSound( pulseSnd, SND_CHANNEL_ANY, 0, false, NULL );
		}
		else
		{
			/// Asymptotic-average back to the normal color
			stroggScoreBarPulseAmount *= asymptoticAverageWeight;
		}

		/// Set "gameStatus" gui element
		_mphud->SetStateString("gameStatus", "" );

		_mphud->SetStateFloat( "marine_pulse_amount", marineScoreBarPulseAmount );
		_mphud->SetStateFloat( "strogg_pulse_amount", stroggScoreBarPulseAmount );

		lastMarineScore = teamScore[ TEAM_MARINE ];
		lastStroggScore = teamScore[ TEAM_STROGG ];
	}
	// RITUAL END

	if( gameLocal.gameType == GAME_TOURNEY && localPlayer->GetArena() == MAX_ARENAS ) {
		int numWaitingArenaPlayers = 0;
		for( int i = 0; i < rankedPlayers.Num(); i++ ) {
			if( rankedPlayers[ i ].First() && rankedPlayers[ i ].First()->GetArena() == MAX_ARENAS ) {
				_mphud->SetStateString( va( "waitRoom_item_%d", numWaitingArenaPlayers++ ), rankedPlayers[ i ].First()->GetUserInfo()->GetString( "ui_name" ) );
			}
		}
		_mphud->SetStateString( va( "waitRoom_item_%d", numWaitingArenaPlayers ), "" );
		_mphud->SetStateBool( "waitroom", true );
		_mphud->SetStateInt( "num_waitroom_players", numWaitingArenaPlayers );
	} else {
		_mphud->SetStateBool( "waitroom", false );
	}

	idStr spectateText0;
	idStr spectateText1;
	idStr spectateText2;

	if( gameLocal.gameType == GAME_TOURNEY ) {
		// line 1 - why we aren't playing
		if( localPlayer->wantSpectate ) {
			if( localPlayer->spectator != localPlayer->entityNumber ) {
				spectateText0 = va( common->GetLocalizedString( "#str_107672" ), gameLocal.GetClientByNum( localPlayer->spectator )->GetUserInfo()->GetString( "ui_name" ) );
			} else if( localPlayer->spectating ) {
				spectateText0 = common->GetLocalizedString( "#str_107673" );
			}
		} else {
			rvTourneyArena& currentArena = ((rvTourneyGameState*)gameState)->GetArena( localPlayer->GetArena() );
			if( gameState->GetMPGameState() == WARMUP ) {
				// grab the reason we aren't playing yet
				AllPlayersReady( &spectateText0 );
			} else if( gameState->GetMPGameState() == COUNTDOWN ) {
				spectateText0 = va( common->GetLocalizedString( "#str_107671" ), Max( ((gameState->GetNextMPGameStateTime() - gameLocal.time) / 1000) + 1, 0 ) );
			} else if( gameState->GetMPGameState() != GAMEREVIEW && localPlayer->GetTourneyStatus() == PTS_ELIMINATED ) { 
				spectateText0 = common->GetLocalizedString( "#str_107687" );
			} else if( gameState->GetMPGameState() != GAMEREVIEW && localPlayer->GetTourneyStatus() == PTS_ADVANCED ) {
				spectateText0 = common->GetLocalizedString( "#str_107688" );
			} else if( ((rvTourneyGameState*)gameState)->HasBye( localPlayer ) ) {
				spectateText0 = common->GetLocalizedString( "#str_107709" );
			} else if( currentArena.IsPlaying( localPlayer ) ) {
				spectateText0 = va( "%s %d; %s", common->GetLocalizedString( "#str_107716" ), localPlayer->GetArena() + 1, ((rvTourneyGameState*)gameState)->GetRoundDescription() );
			} else if( localPlayer->spectating ) {
				// this should only happen if the player was spectating at start of round, but then decides
				// to join the tourney
				spectateText0 = common->GetLocalizedString( "#str_107684" );
			}
		}
		
		// line 2 - will or wont be seeded, how to cycle
		// line 3 - how to enter waiting room
		if( gameState->GetMPGameState() == WARMUP || gameState->GetMPGameState() == COUNTDOWN ) {
			if( localPlayer->wantSpectate ) {
				spectateText1 = common->GetLocalizedString( "#str_107685" );
				spectateText2 = common->GetLocalizedString( "#str_107695" );
			} else {
				spectateText1 = common->GetLocalizedString( "#str_107684" );
				spectateText2 = common->GetLocalizedString( "#str_107694" );
			}
		} else if( localPlayer->spectating ) {
			if( localPlayer->GetArena() == MAX_ARENAS ) {
				spectateText1 = common->GetLocalizedString( "#str_107686" );
			} else {
				spectateText1 = va( common->GetLocalizedString( "#str_107670" ), common->KeysFromBinding( "_impulse14" ), common->KeysFromBinding( "_impulse15" ) );
			}
		}
	} else {
		// non-tourney spectate text
		if( localPlayer->spectating ) {
			if( localPlayer->spectator != localPlayer->entityNumber ) {
				spectateText0 = va( common->GetLocalizedString( "#str_107672" ), gameLocal.GetClientByNum( localPlayer->spectator )->GetUserInfo()->GetString( "ui_name" ) );
			} else if( localPlayer->spectating ) {
				spectateText0 = common->GetLocalizedString( "#str_107673" );
			}

			// spectating instructions
			if( localPlayer->spectator != localPlayer->entityNumber ) {
				//cycle & exit follow
				spectateText1 = va( common->GetLocalizedString( "#str_107698" ), common->KeysFromBinding( "_attack" ), common->KeysFromBinding( "_moveup" )  );
			} else {
				//start follow
				spectateText1 = va( common->GetLocalizedString( "#str_108024" ), common->KeysFromBinding( "_attack" )  );
			}
			
		}

		if( gameState->GetMPGameState() == WARMUP ) {
			AllPlayersReady( &spectateText1 );
		} else if( gameState->GetMPGameState() == COUNTDOWN ) {
			spectateText1 = va( common->GetLocalizedString( "#str_107671" ), Max( ((gameState->GetNextMPGameStateTime() - gameLocal.time) / 1000) + 1, 0 ) );
		}
	}

	_mphud->SetStateString( "spectatetext0", spectateText0 );
	_mphud->SetStateString( "spectatetext1", spectateText1 );
	_mphud->SetStateString( "spectatetext2", spectateText2 );

	if( gameLocal.gameType == GAME_TOURNEY ) {
		gameLocal.mpGame.tourneyGUI.UpdateScores();
	}

	_mphud->StateChanged( gameLocal.time );

	statManager->UpdateInGameHud( _mphud, ( localPlayer->usercmd.buttons & BUTTON_INGAMESTATS ) != 0 );

	//update awards
	if ( gameLocal.isClient || gameLocal.isListenServer) {
		statManager->CheckAwardQueue();
	}
}

/*
================
idMultiplayerGame::DrawScoreBoard
================
*/
void idMultiplayerGame::DrawScoreBoard( idPlayer *player ) {
	if ( player->scoreBoardOpen ) {
		if ( !playerState[ player->entityNumber ].scoreBoardUp ) {
			scoreBoard->Activate( true, gameLocal.time );
			playerState[ player->entityNumber ].scoreBoardUp = true;
			player->disableHud = true;
		}
		if( gameLocal.gameType == GAME_TOURNEY ) {
			((rvTourneyGameState*)gameState)->UpdateTourneyBrackets();
		}
		UpdateScoreboard( scoreBoard );
	} else {
		if ( playerState[ player->entityNumber ].scoreBoardUp ) {
			scoreBoard->Activate( false, gameLocal.time );
			playerState[ player->entityNumber ].scoreBoardUp = false;
			player->disableHud = false;
		}
	}
}

/*
===============
idMultiplayerGame::AddChatLine
===============
*/
void idMultiplayerGame::AddChatLine( const char *fmt, ... )
{
	idStr s;
	va_list argptr;
	va_start( argptr, fmt );
	vsprintf( s, fmt, argptr );
	va_end( argptr );
	PrintChatLine( s, false );
}

void idMultiplayerGame::PrintChatLine( const char *message, const bool teamChat ) {
	idStr text = message;
	text.StripTrailingOnce("\n");
	gameLocal.Printf( "%s\n", text.c_str() );

	wrapInfo_t wrapInfo;
	idStr wrap1;
	idStr wrap2;

	idUserInterface *mpHud = gameLocal.GetLocalPlayer() ? gameLocal.GetLocalPlayer()->mphud : NULL;
	if ( mpHud ) {
		wrap1 = text;
		wrap2 = text;
		do {
			memset( &wrapInfo, -1, sizeof ( wrapInfo_t ) );
			mpHud->GetMaxTextIndex( "history1", wrap1.c_str( ), wrapInfo );

			// If we have a whitespace near the end. Otherwise the user could enter a giant word.
			if ( wrapInfo.lastWhitespace != -1 &&  float( wrapInfo.lastWhitespace ) / float( wrapInfo.maxIndex ) > .75 ) {
				wrap2 = wrap1.Left( wrapInfo.lastWhitespace++ );

			// Just text wrap, no word wrap.
			} else if ( wrapInfo.maxIndex != -1 ) {					
				wrap2 = wrap1.Left( wrapInfo.maxIndex );

			// We fit in less than a line.
			} else {
				wrap2 = wrap1;
			}

			// Recalc the base string.
			wrap1 = wrap2.GetLastColorCode() + wrap1.Right( wrap1.Length( ) - wrap2.Length( ) );

			// Push to gui.
			mpHud->SetStateString( "chattext", wrap2.c_str( ) );
			mpHud->HandleNamedEvent( "addchatline" );
		} while ( wrapInfo.maxIndex != -1 );
	}

	if( chatHistory.Length() + text.Length() > CHAT_HISTORY_SIZE ) {
		int removeLength = chatHistory.Find( '\n' );
		if( removeLength == -1 ) {
			// nuke the whole string
			chatHistory.Empty();
		} else {
			while( (chatHistory.Length() - removeLength) + text.Length() > CHAT_HISTORY_SIZE ) {
				removeLength = chatHistory.Find( '\n', removeLength + 1 );
 				if( removeLength == -1 ) {
					chatHistory.Empty();
					break;
				}
			}
		}
		chatHistory = chatHistory.Right( chatHistory.Length() - removeLength );
	}

	chatHistory.Append( text );
	chatHistory.Append( '\n' );

	if( mainGui ) {
		mainGui->SetStateString( "chat", chatHistory.c_str() );
	}
	if( statSummary ) {
		statSummary->SetStateString( "chat", chatHistory.c_str() );
	}
	
	// play chat sound
	if( gameLocal.GetLocalPlayer() ) {
		if ( teamChat ) {
			gameLocal.GetLocalPlayer()->StartSound( "snd_teamchat", SND_CHANNEL_ANY, 0, false, NULL );
		} else {
			gameLocal.GetLocalPlayer()->StartSound( "snd_chat", SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
}

void idMultiplayerGame::DrawStatSummary( void ) {	
	if ( !statSummary->GetStateFloat( "ready" ) ) {
		statSummary->SetStateFloat( "ready", 1 );
		statSummary->HandleNamedEvent( "chatFocus" );
		statSummary->StateChanged( gameLocal.time );
	}
	statSummary->Redraw( gameLocal.time );
}

void idMultiplayerGame::ShowStatSummary( void ) {
	if ( !gameLocal.GetLocalPlayer() ) {
		assert( false );
		return;
	}
	DisableMenu( );
	nextMenu = 3;
	gameLocal.sessionCommand = "game_startmenu";
	gameLocal.GetLocalPlayer()->GUIMainNotice( "" );
	gameLocal.GetLocalPlayer()->GUIFragNotice( "" );
}

/*
================
idMultiplayerGame::WriteToSnapshot
================
*/
void idMultiplayerGame::WriteToSnapshot( idBitMsgDelta &msg ) const {
	int 		i;
 	int 		value;
	byte		ingame[ MAX_CLIENTS / 8 ];
	idEntity*	ent;

	assert( MAX_CLIENTS % 8 == 0 );
// RITUAL BEGIN - DeadZone Messages
	msg.WriteBits(isBuyingAllowedRightNow, 1);
	msg.WriteShort(powerupCount);
	msg.WriteFloat( marineScoreBarPulseAmount );
	msg.WriteFloat( stroggScoreBarPulseAmount );
// RITUAL END


// RAVEN BEGIN
// ddynerman: CTF scoring
// FIXME - not in the snapshot
	for ( i = 0; i < TEAM_MAX; i++ ) {
		msg.WriteShort( teamScore[i] );
		msg.WriteLong( teamDeadZoneScore[i] );
	}
// RAVEN END

	// write ingame bits first, then we only sync down for ingame clients
	// do a single write, this doesn't change often it's best to deltify in a single shot
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( playerState[i].ingame ) {
			ingame[ i / 8 ] |= 1 << ( i % 8 );
		} else {
			ingame[ i / 8 ] &= ~( 1 << ( i % 8 ) );
		}
	}
	msg.WriteData( ingame, MAX_CLIENTS / 8 );

	// those rarely change as well and will deltify away nicely
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( playerState[i].ingame ) {
			ent = gameLocal.entities[ i ];
			// clamp all values to min/max possible value that we can send over
			value = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[i].fragCount );
			msg.WriteBits( value, ASYNC_PLAYER_FRAG_BITS );
			value = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[i].teamFragCount );
			msg.WriteBits( value, ASYNC_PLAYER_FRAG_BITS );
			msg.WriteLong( playerState[i].deadZoneScore );
			value = idMath::ClampInt( 0, MP_PLAYER_MAXWINS, playerState[i].wins );
			msg.WriteBits( value, ASYNC_PLAYER_WINS_BITS );
			// only transmit instance info in tourney
			if( gameLocal.gameType == GAME_TOURNEY ) {
				if( !ent ) {
					msg.WriteBits( 0, 1 );
				} else {
					msg.WriteBits( 1, 1 );
					value = idMath::ClampInt( 0, MAX_INSTANCES, ent->GetInstance() );
					msg.WriteBits( value, ASYNC_PLAYER_INSTANCE_BITS );
					msg.WriteBits( ((idPlayer*)ent)->GetTourneyStatus(), ASYNC_PLAYER_TOURNEY_STATUS_BITS );
				}
			}
		}
	}

	// those change all the time, keep them in a single pack
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( playerState[i].ingame ) {
			value = idMath::ClampInt( 0, MP_PLAYER_MAXPING, playerState[i].ping );
			msg.WriteBits( value, ASYNC_PLAYER_PING_BITS );
		}
	}
}

/*
================
idMultiplayerGame::ReadFromSnapshot
================
*/
void idMultiplayerGame::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	int 		i, newInstance;
	byte		ingame[ MAX_CLIENTS / 8 ];
	idEntity*	ent;

	isBuyingAllowedRightNow = msg.ReadBits(1);
	powerupCount = msg.ReadShort();
	// TTimo: NOTE: sounds excessive to be transmitting floats for that
	marineScoreBarPulseAmount = msg.ReadFloat();
	stroggScoreBarPulseAmount = msg.ReadFloat();

	// CTF/TDM scoring
	for( i = 0; i < TEAM_MAX; i++ ) {
		teamScore[ i ] = msg.ReadShort( );
		teamDeadZoneScore[ i ] = msg.ReadLong( );
	}

	msg.ReadData( ingame, MAX_CLIENTS / 8 );
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( ingame[ i / 8 ] & ( 1 << ( i % 8 ) ) ) {
			playerState[i].ingame = true;
		} else {
			playerState[i].ingame = false;
		}
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( playerState[i].ingame ) {
			ent = gameLocal.entities[ i ];
			playerState[ i ].fragCount = msg.ReadBits( ASYNC_PLAYER_FRAG_BITS );
			playerState[ i ].teamFragCount = msg.ReadBits( ASYNC_PLAYER_FRAG_BITS );
			playerState[ i ].deadZoneScore = msg.ReadLong();
			playerState[ i ].wins = msg.ReadBits( ASYNC_PLAYER_WINS_BITS );
			if( gameLocal.gameType == GAME_TOURNEY ) {
				if( msg.ReadBits( 1 ) ) {
					newInstance = msg.ReadBits( ASYNC_PLAYER_INSTANCE_BITS );
					if( newInstance != ent->GetInstance() ) {
						ent->SetInstance( newInstance );
						if( gameLocal.GetLocalPlayer() && i != gameLocal.localClientNum ) {
							if( ent->GetInstance() == gameLocal.GetLocalPlayer()->GetInstance() ) {
								((idPlayer*)ent)->ClientInstanceJoin();
							} else {
								((idPlayer*)ent)->ClientInstanceLeave();
							}
						}
					}
					((idPlayer*)ent)->SetTourneyStatus( (playerTourneyStatus_t)msg.ReadBits( ASYNC_PLAYER_TOURNEY_STATUS_BITS ) );
				}
			}
		}
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( playerState[i].ingame ) {
			playerState[ i ].ping = msg.ReadBits( ASYNC_PLAYER_PING_BITS );
		}
	}
}

// RAVEN BEGIN
// bdube: global item sounds
/*
================
idMultiplayerGame::PlayGlobalItemAcquireSound
================
*/
void idMultiplayerGame::PlayGlobalItemAcquireSound( int defIndex ) {
	const idDeclEntityDef*  def;
	def = static_cast<const idDeclEntityDef*>( declManager->DeclByIndex( DECL_ENTITYDEF, defIndex, false ) );
	if ( !def ) {
		gameLocal.Warning ( "NET: invalid entity def index (%d) for global item acquire sound", defIndex );
		return;
	}

	if( !gameLocal.GetLocalPlayer() || !gameLocal.currentThinkingEntity || gameLocal.GetLocalPlayer()->GetInstance() == gameLocal.currentThinkingEntity->GetInstance() ) {
		soundSystem->PlayShaderDirectly ( SOUNDWORLD_GAME, def->dict.GetString ( "snd_acquire" ) );		
	}

	if ( gameLocal.isServer ) {
		idBitMsg outMsg;
		byte msgBuf[1024];
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_ITEMACQUIRESOUND );
		outMsg.WriteBits( defIndex, gameLocal.entityDefBits );
		gameLocal.ServerSendInstanceReliableMessage( gameLocal.currentThinkingEntity, -1, outMsg );
	}	
}
// RAVEN END

/*
================
idMultiplayerGame::PrintMessageEvent
================
*/
void idMultiplayerGame::PrintMessageEvent( int to, msg_evt_t evt, int parm1, int parm2 ) {
	idPlayer *p = gameLocal.GetLocalPlayer();
	if ( to == -1 || ( p && to == p->entityNumber ) ) {
		switch ( evt ) {
		case MSG_SUICIDE:
			assert( parm1 >= 0 );
			AddChatLine( common->GetLocalizedString( "#str_104293" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ) );
			break;
		case MSG_KILLED:
			assert( parm1 >= 0 && parm2 >= 0 );
			AddChatLine( common->GetLocalizedString( "#str_104292" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ), gameLocal.userInfo[ parm2 ].GetString( "ui_name" ) );
			break;
		case MSG_KILLEDTEAM:
			assert( parm1 >= 0 && parm2 >= 0 );
			AddChatLine( common->GetLocalizedString( "#str_104291" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ), gameLocal.userInfo[ parm2 ].GetString( "ui_name" ) );
			break;
		case MSG_TELEFRAGGED:
			assert( parm1 >= 0 && parm2 >= 0 );
			AddChatLine( common->GetLocalizedString( "#str_104290" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ), gameLocal.userInfo[ parm2 ].GetString( "ui_name" ) );
			break;
		case MSG_DIED:
			assert( parm1 >= 0 );
			AddChatLine( common->GetLocalizedString( "#str_104289" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ) );
			break;
		case MSG_VOTE:
			AddChatLine( common->GetLocalizedString( "#str_104288" ) );
			break;
		case MSG_SUDDENDEATH:
			AddChatLine( common->GetLocalizedString( "#str_104287" ) );
			break;
		case MSG_FORCEREADY:
			AddChatLine( common->GetLocalizedString( "#str_104286" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ) );
			// RAVEN BEGIN
			// jnewquist: Use accessor for static class type 
			if ( gameLocal.entities[ parm1 ] && gameLocal.entities[ parm1 ]->IsType( idPlayer::GetClassType() ) ) {
				// RAVEN END
				static_cast< idPlayer * >( gameLocal.entities[ parm1 ] )->forcedReady = true;
			}
			break;
		case MSG_JOINEDSPEC:
			AddChatLine( common->GetLocalizedString( "#str_104285" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ) );
			break;
		case MSG_TIMELIMIT:
			AddChatLine( common->GetLocalizedString( "#str_104284" ) );
			break;
		case MSG_FRAGLIMIT:
			// RITUAL BEGIN
			// squirrel: added DeadZone multiplayer mode
			if ( gameLocal.gameType == GAME_TDM || gameLocal.gameType == GAME_DEADZONE ) {
				// RITUAL END
				// RAVEN BEGIN
				// rhummer: localized "Strogg" and "Marine"
				AddChatLine( common->GetLocalizedString( "#str_107665" ), parm1 ? common->GetLocalizedString( "#str_108025" ) : common->GetLocalizedString( "#str_108026" ) );
				// RAVEN END
			} else {
				AddChatLine( common->GetLocalizedString( "#str_104281" ), gameLocal.userInfo[ parm1 ].GetString( "ui_name" ) );
			}
			break;
		case MSG_CAPTURELIMIT:
			// RAVEN BEGIN
			// rhummer: localized "%s team hit the capture limit." and "Strogg and "Marine"
			AddChatLine( common->GetLocalizedString( "#str_108027" ), parm1 ? common->GetLocalizedString( "#str_108025" ) : common->GetLocalizedString( "#str_108026" ) );
			// RAVEN END
			break;
		case MSG_HOLYSHIT:
			AddChatLine( common->GetLocalizedString( "#str_106732" ) );
			break;
		default:
			gameLocal.DPrintf( "PrintMessageEvent: unknown message type %d\n", evt );
			return;
		}
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
idMultiplayerGame::PrintMessage
================
*/
void idMultiplayerGame::PrintMessage( int to, const char* msg ) {
	if( idStr::Length( msg ) >= MAX_PRINT_LEN ) {
		common->Warning( "idMultiplayerGame::PrintMessage() - Not transmitting message of length %d", idStr::Length( msg ) );
		return;
	}

	AddChatLine( msg );

	if ( !gameLocal.isClient ) {
		idBitMsg outMsg;
		byte msgBuf[1024];
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_PRINT );
		outMsg.WriteString( msg );
		networkSystem->ServerSendReliableMessage( to, outMsg );
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
		
		if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
			continue;
		}

		idPlayer *p = static_cast<idPlayer *>(ent);

		// once we hit sudden death, nobody respawns till game has ended
		// no respawns in tourney mode, the tourney manager manually handles spawns
		if ( (WantRespawn( p ) || p == spectator) ) {
			if ( gameState->GetMPGameState() == SUDDENDEATH && gameLocal.gameType != GAME_TOURNEY ) {
				// respawn rules while sudden death are different
				// sudden death may trigger while a player is dead, so there are still cases where we need to respawn
				// don't do any respawns while we are in end game delay though
				if ( gameLocal.IsTeamGame() || p->IsLeader() ) {
					//everyone respawns in team games, only fragleaders respawn in DM
					p->ServerSpectate( false );
				} else {//if ( !p->IsLeader() ) {
					// sudden death is rolling, this player is not a leader, have him spectate
					p->ServerSpectate( true );
					CheckAbortGame();
				}
			} else {
				if ( gameState->GetMPGameState() == WARMUP || gameState->GetMPGameState() == COUNTDOWN || gameState->GetMPGameState() == GAMEON ) {
					if ( gameLocal.gameType != GAME_TOURNEY ) {
						// wait for team to be set before spawning in
						if( !gameLocal.IsTeamGame() || p->team != -1 ) {
							p->ServerSpectate( false );
						}

					} else {
						if( p->GetArena() >= 0 && p->GetArena() < MAX_ARENAS ) {
							rvTourneyArena& arena = ((rvTourneyGameState*)gameState)->GetArena( p->GetArena() );
							if( ( arena.GetState() != AS_DONE && arena.GetState() != AS_INACTIVE ) && ( p == arena.GetPlayers()[ 0 ] || p == arena.GetPlayers()[ 1 ] ) ) {
								// only allow respawn if the arena we're in is active 
								// and we're one of the assigned players (we're not just spectating it)
								p->ServerSpectate( false );
							}
						} else {
							// always allow respawn in the waiting room
							assert( p->GetArena() == MAX_ARENAS );
							p->ServerSpectate( false );
						}
					}
				}				
			}
		} else if ( p->wantSpectate && !p->spectating ) {
			playerState[ i ].fragCount = 0; // whenever you willingly go spectate during game, your score resets
			p->ServerSpectate( true );
			CheckAbortGame();
		}
	}
}

void idMultiplayerGame::FreeLight ( int lightID ) {
	if ( lightHandles[lightID] != -1 && gameRenderWorld ) {
		gameRenderWorld->FreeLightDef( lightHandles[lightID] );
		lightHandles[lightID] = -1;
	}
}

void idMultiplayerGame::UpdateLight ( int lightID, idPlayer *player ) {
	lights[ lightID ].origin = player->GetPhysics()->GetOrigin() + idVec3( 0, 0, 20 );
	
	if ( lightHandles[ lightID ] == -1 ) {
		lightHandles[ lightID ] = gameRenderWorld->AddLightDef ( &lights[ lightID ] );
	} else {
		gameRenderWorld->UpdateLightDef( lightHandles[ lightID ], &lights[ lightID ] );
	}
}

void idMultiplayerGame::CheckSpecialLights( void ) {
	if ( !gameLocal.isLastPredictFrame ) {
		return;
	}

	idPlayer *marineFlagCarrier = NULL;
	idPlayer *stroggFlagCarrier = NULL;
	idPlayer *quadDamageCarrier = NULL;
	idPlayer *regenerationCarrier = NULL;
	idPlayer *hasteCarrier = NULL;

	for( int i = 0 ; i < gameLocal.numClients ; i++ ) {
		idEntity *ent = gameLocal.entities[ i ];
		if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
			continue;
		}

		idPlayer *p = static_cast<idPlayer *>( ent );

		if( gameLocal.GetLocalPlayer() && p->GetInstance() != gameLocal.GetLocalPlayer()->GetInstance() ) {
			continue;
		}

		if ( p->PowerUpActive( POWERUP_CTF_MARINEFLAG ) ) {
			marineFlagCarrier = p;
		}
		else if ( p->PowerUpActive( POWERUP_CTF_STROGGFLAG ) ) {
			stroggFlagCarrier = p;
		}
		else if( p->PowerUpActive( POWERUP_QUADDAMAGE ) || p->PowerUpActive( POWERUP_TEAM_DAMAGE_MOD )) {
			quadDamageCarrier = p;
		}
		else if( p->PowerUpActive( POWERUP_REGENERATION ) ) {
			regenerationCarrier = p;
		}
		else if( p->PowerUpActive( POWERUP_HASTE ) ) {
			hasteCarrier = p;
		}
	}

	if ( marineFlagCarrier ) {
		UpdateLight( MPLIGHT_CTF_MARINE, marineFlagCarrier );
	} else {
		FreeLight( MPLIGHT_CTF_MARINE );
	}

	if ( stroggFlagCarrier ) {
		UpdateLight( MPLIGHT_CTF_STROGG, stroggFlagCarrier );
	} else {
		FreeLight( MPLIGHT_CTF_STROGG );
	}

	if ( quadDamageCarrier ) {
		UpdateLight( MPLIGHT_QUAD, quadDamageCarrier );
	} else {
		FreeLight( MPLIGHT_QUAD );
	}

	if ( regenerationCarrier ) {
		UpdateLight( MPLIGHT_REGEN, regenerationCarrier );
	} else {
		FreeLight( MPLIGHT_REGEN );
	}

	if ( hasteCarrier ) {
		UpdateLight( MPLIGHT_HASTE, hasteCarrier );
	} else {
		FreeLight( MPLIGHT_HASTE );
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
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
		gameLocal.Printf( "forceReady: multiplayer server only\n" );
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
		return;
	}
// RAVEN BEGIN
// bdube: removed parameter
	static_cast< idPlayer* >( ent )->DropWeapon( );
// RAVEN END
}

/*
================
idMultiplayerGame::DropWeapon_f
================
*/
void idMultiplayerGame::DropWeapon_f( const idCmdArgs &args ) {
	if ( !gameLocal.isMultiplayer ) {
		gameLocal.Printf( "clientDropWeapon: only valid in multiplayer\n" );
		return;
	}
	idBitMsg	outMsg;
	byte		msgBuf[128];
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_DROPWEAPON );
	networkSystem->ClientSendReliableMessage( outMsg );
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
	if ( !mode[ 0 ] || !gameLocal.IsTeamGame() ) {
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
================
*/
void idMultiplayerGame::Vote_f( const idCmdArgs &args ) { 
// RAVEN BEGIN
// shouchard:  implemented for testing	
	if ( args.Argc() < 2 ) {
		common->Printf( common->GetLocalizedString( "#str_104418" ) );
		return;
	}

	const char *szArg1 = args.Argv(1);
	bool voteValue = false;
	if ( 0 == idStr::Icmp( szArg1, "yes" ) ) {
		voteValue = true;
	}
	
	gameLocal.mpGame.CastVote( gameLocal.localClientNum, voteValue );
// RAVEN END
}

/*
================
idMultiplayerGame::CallVote_f
moved this over the use the packed voting
still only does one vote though, can easily be extended to do more
================
*/
void idMultiplayerGame::CallVote_f( const idCmdArgs &args ) { 
	const char *szArg1 = args.Argv(1);
	const char *szArg2 = args.Argv(2);
	if ( '\0' == *szArg1 ) {
		common->Printf( common->GetLocalizedString( "#str_104404" ) );
		common->Printf( common->GetLocalizedString( "#str_104405" ) );
		return;
	}

	voteStruct_t voteData;
	memset( &voteData, 0, sizeof( voteData ) );

	if ( 0 == idStr::Icmp( szArg1, "restart" ) ) {
		voteData.m_fieldFlags |= VOTEFLAG_RESTART;
	} else if ( 0 == idStr::Icmp( szArg1, "timelimit" ) ) {
		if ( '\0' == *szArg2 ) {
			common->Printf( common->GetLocalizedString( "#str_104406" ) );
			return;
		}
		voteData.m_fieldFlags |= VOTEFLAG_TIMELIMIT;
		voteData.m_timeLimit = atoi( szArg2 );
	} else if ( 0 == idStr::Icmp( szArg1, "fraglimit" ) ) {
		if ( '\0' == *szArg2 ) {
			common->Printf( common->GetLocalizedString( "#str_104407" ) );
			return;
		}
		voteData.m_fieldFlags |= VOTEFLAG_FRAGLIMIT;
		voteData.m_fragLimit = atoi( szArg2 );
	} else if ( 0 == idStr::Icmp( szArg1, "gametype" ) ) {
		if ( '\0' == *szArg2 ) {
			common->Printf( common->GetLocalizedString( "#str_104408" ) );
			common->Printf( common->GetLocalizedString( "#str_104409" ) );
			return;
		}
		voteData.m_fieldFlags |= VOTEFLAG_GAMETYPE;
		voteData.m_gameType = gameLocal.mpGame.GameTypeToVote( szArg2 );
	}
	else if ( 0 == idStr::Icmp( szArg1, "kick" ) ) {
		if ( '\0' == *szArg2 ) {
			common->Printf( common->GetLocalizedString( "#str_104412" ) );
			return;
		}
		voteData.m_kick = gameLocal.mpGame.GetClientNumFromPlayerName( szArg2 );
		if ( voteData.m_kick >= 0 ) {
			voteData.m_fieldFlags |= VOTEFLAG_KICK;
		}
	} else if ( 0 == idStr::Icmp( szArg1, "map" ) ) {
		if ( '\0' == *szArg2 ) {
			common->Printf( common->GetLocalizedString( "#str_104413" ) );
			return;
		}
		voteData.m_fieldFlags |= VOTEFLAG_MAP;
		voteData.m_map = szArg2;
	} else if ( 0 == idStr::Icmp( szArg1, "buying" ) ) {
		if ( '\0' == *szArg2 ) {
			common->Printf( common->GetLocalizedString( "#str_122012" ) );
			return;
		}
		voteData.m_fieldFlags |= VOTEFLAG_BUYING;
		voteData.m_buying = atoi( szArg2 );
	} else if ( 0 == idStr::Icmp( szArg1, "capturelimit" ) ) {
		if ( '\0' == *szArg2 ) {
			common->Printf( common->GetLocalizedString( "#str_104415" ) );
			return;
		}
		voteData.m_fieldFlags |= VOTEFLAG_CAPTURELIMIT;
		voteData.m_captureLimit = atoi( szArg2 );
	} else if ( 0 == idStr::Icmp( szArg1, "autobalance" ) ) {
		if ( '\0' == *szArg2 ) {
			common->Printf( common->GetLocalizedString( "#str_104416" ) );
		}
		voteData.m_fieldFlags |= VOTEFLAG_TEAMBALANCE;
		voteData.m_teamBalance = atoi( szArg2 );
	} else if ( 0 == idStr::Icmp( szArg1, "controlTime" ) ) {
		if ( '\0' == *szArg2 ) {
			common->Printf( common->GetLocalizedString( "#str_122002" ) ); // Squirrel@Ritual - Localized for 1.2 Patch
		}
		voteData.m_fieldFlags |= VOTEFLAG_CONTROLTIME;
		voteData.m_controlTime = atoi(szArg2 );
	} else {
		common->Printf( common->GetLocalizedString( "#str_104404" ) );
		common->Printf( common->GetLocalizedString( "#str_104405" ) );
		return;
	}

	if ( voteData.m_fieldFlags != 0 ) {
		gameLocal.mpGame.ClientCallPackedVote( voteData );
	}
}

// RAVEN BEGIN
// shouchard: added voice mute and unmute console commands; sans XBOX to not step on their live voice stuff
#ifndef _XBOX
/*
================
idMultiplayerGame::VoiceMute_f
================
*/
void idMultiplayerGame::VoiceMute_f( const idCmdArgs &args ) {
	if ( args.Argc() < 2 ) {
		common->Printf( "USAGE: clientvoicemute <player>\n" );
		return;
	}
	gameLocal.mpGame.ClientVoiceMute( gameLocal.mpGame.GetClientNumFromPlayerName( args.Argv( 1 ) ), true );
}

/*
================
idMultiplayerGame::VoiceUnmute_f
================
*/
void idMultiplayerGame::VoiceUnmute_f( const idCmdArgs &args ) {
	if ( args.Argc() < 2 ) {
		common->Printf( "USAGE: clientvoiceunmute <player>\n" );
		return;
	}
	gameLocal.mpGame.ClientVoiceMute( gameLocal.mpGame.GetClientNumFromPlayerName( args.Argv( 1 ) ), false );
}

// RAVEN END
#endif // _XBOX

// RAVEN BEGIN
/*
================
idMultiplayerGame::ForceTeamChange_f
================
*/
void idMultiplayerGame::ForceTeamChange_f( const idCmdArgs &args)	{

	if( !gameLocal.isMultiplayer )	{
		common->Printf( "[MP ONLY] Forces player to change teams. Usage: ForceTeamChange <client number>\n" );
		return;
	}

	idStr clientId;
	int clientNum;

	clientId = args.Argv( 1 );
	if ( !clientId.IsNumeric() ) {
		common->Printf( "usage: ForceTeamChange <client number>\n" );
		return;
	}

	clientNum = atoi( clientId );
	
	if ( gameLocal.entities[ clientNum ] && gameLocal.entities[ clientNum ]->IsType( idPlayer::GetClassType() ) )
	{
		idPlayer *player = static_cast< idPlayer *>( gameLocal.entities[ clientNum ] );
		player->GetUserInfo()->Set( "ui_team", player->team ? "Marine" : "Strogg" );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "updateUI %d\n", clientNum ) );
	}

}


/*
================
idMultiplayerGame::RemoveClientFromBanList_f
================
*/
void idMultiplayerGame::RemoveClientFromBanList_f( const idCmdArgs& args )	{

	if( !gameLocal.isMultiplayer )	{
		common->Printf( "[MP ONLY] Remove player from banlist. Usage: RemoveClientFromBanList <client number>\n" );
		return;
	}
	
	idStr clientId;
	clientId = args.Argv( 1 );
	int clientNum;

	if ( !clientId.IsNumeric() ) {
		common->Printf( "Usage: RemoveClientFromBanList <client number>\n" );
		return;
	}

	clientNum = atoi( clientId );

	const char *clientGuid = networkSystem->GetClientGUID( clientNum ); //  gameLocal.GetGuidByClientNum( clientNum );

	if ( NULL == clientGuid || !clientGuid[ 0 ]) {
		common->DPrintf( "idMultiplayerGame::HandleServerAdminRemoveBan:  bad guid!\n" );
		return;
	}

	if ( gameLocal.isServer || gameLocal.isListenServer ) {
		// remove from the ban list
		gameLocal.RemoveGuidFromBanList( clientGuid );
	}

}

/*
================
idMultiplayerGame::ProcessRconReturn
================
*/
void idMultiplayerGame::ProcessRconReturn( bool success )	{

	if( success )	{
		mainGui->HandleNamedEvent("adminPasswordSuccess");
	} else {
		mainGui->HandleNamedEvent("adminPasswordFail");
	}


}


// RAVEN END

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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
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
	AddChatLine( va( common->GetLocalizedString( "#str_104279" ), gameLocal.userInfo[ clientNum ].GetString( "ui_name" ) ) );
// RAVEN BEGIN
// shouchard:  better info when a vote called in the chat buffer
	AddChatLine( voteString ); // TODO:  will push this into a UI field later
// shouchard:  display the vote called text on the hud
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->mphud ) {
		gameLocal.GetLocalPlayer()->mphud->SetStateInt( "voteNotice", 1 );
	}
// RAVEN END
	ScheduleAnnouncerSound( AS_GENERAL_VOTE_NOW, gameLocal.time );
	
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
	}

	ClientUpdateVote( VOTE_UPDATE, yesVotes, noVotes, currentVoteData );
}

/*
================
idMultiplayerGame::ClientUpdateVote
================
*/
void idMultiplayerGame::ClientUpdateVote( vote_result_t status, int yesCount, int noCount, const voteStruct_t &voteData ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];
	const char * localizedString = 0;
	idPlayer* player = gameLocal.GetLocalPlayer( );

	if ( !gameLocal.isClient ) {
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_UPDATEVOTE );
		outMsg.WriteByte( status );
		outMsg.WriteByte( yesCount );
		outMsg.WriteByte( noCount );
// RAVEN BEGIN
// shouchard:  multifield vote support
		if ( VOTE_MULTIFIELD != vote ) {
			outMsg.WriteByte( 0 );
		} else {
			outMsg.WriteByte( 1 );
			outMsg.WriteShort( voteData.m_fieldFlags );
			outMsg.WriteByte( idMath::ClampChar( voteData.m_kick ) );
			outMsg.WriteString( voteData.m_map.c_str() );
			outMsg.WriteByte( idMath::ClampChar( voteData.m_gameType ) );
			outMsg.WriteByte( idMath::ClampChar( voteData.m_timeLimit ) );
			outMsg.WriteShort( idMath::ClampShort( voteData.m_fragLimit ) );
			outMsg.WriteShort( idMath::ClampShort( voteData.m_tourneyLimit ) );
			outMsg.WriteShort( idMath::ClampShort( voteData.m_captureLimit ) );
			outMsg.WriteShort( idMath::ClampShort( voteData.m_buying ) );
			outMsg.WriteByte( idMath::ClampChar( voteData.m_teamBalance ) );
		}
		networkSystem->ServerSendReliableMessage( -1, outMsg );
	} else {
		currentVoteData = voteData;
	}
// RAVEN END

	if ( vote == VOTE_NONE ) {
		// clients coming in late don't get the vote start and are not allowed to vote
		if ( mainGui ) {
			mainGui->SetStateInt( "vote_going", 0 );
		}
		return;
	}

	switch ( status ) {
		case VOTE_FAILED:
			localizedString = common->GetLocalizedString( "#str_104278" );
			AddChatLine( localizedString );
			ScheduleAnnouncerSound( AS_GENERAL_VOTE_FAILED, gameLocal.time );
			if ( gameLocal.isClient ) {
				vote = VOTE_NONE;
			}
			break;
		case VOTE_PASSED:
			localizedString = common->GetLocalizedString( "#str_104277" );
			AddChatLine( localizedString );
			ScheduleAnnouncerSound( AS_GENERAL_VOTE_PASSED, gameLocal.time );
			break;
		case VOTE_RESET:
			if ( gameLocal.isClient ) {
				vote = VOTE_NONE;
			}
			break;
		case VOTE_ABORTED:
			localizedString = common->GetLocalizedString( "#str_104276" );
			AddChatLine( localizedString );
			if ( gameLocal.isClient ) {
				vote = VOTE_NONE;
			}
			break;
		case VOTE_UPDATE:
			if ( player && player->mphud && voted ) {
				player->mphud->SetStateString( "voteNoticeText", va("^:%s\n%s: %d %s: %d", 
					common->GetLocalizedString( "#str_107724" ),
					common->GetLocalizedString( "#str_107703" ),
					yesCount,
					common->GetLocalizedString( "#str_107704" ),
					noCount ) );
			}

			if ( mainGui ) {
				mainGui->SetStateInt( "playerVoted", voted );
			}
			break;
		default:
			break;
	}

	if ( gameLocal.isClient ) {
		yesVotes = yesCount;
		noVotes = noCount;
	}

// RAVEN BEGIN
// shouchard:  remove vote notification
	if ( VOTE_FAILED == status || VOTE_PASSED == status || VOTE_RESET == status ) {
		ClearVote();
	}

	if ( mainGui ) {
		mainGui->SetStateString( "voteCount", va( common->GetLocalizedString( "#str_104435" ), (int)yesVotes, (int)noVotes ) );
	}
// RAVEN END
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
		gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104275" );
		common->DPrintf( "client %d: cast vote while no vote in progress\n", clientNum );
		return;
	}
	if ( playerState[ clientNum ].vote != PLAYER_VOTE_WAIT ) {
		gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104274" );
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

	ClientUpdateVote( VOTE_UPDATE, yesVotes, noVotes, currentVoteData );
}

/*
================
idMultiplayerGame::ServerCallVote
================
*/
void idMultiplayerGame::ServerCallVote( int clientNum, const idBitMsg &msg ) {
	vote_flags_t	voteIndex;
	int				vote_timeLimit, vote_fragLimit, vote_clientNum, vote_gameTypeIndex, vote_buying; //, vote_kickIndex;
// RAVEN BEGIN
// shouchard:  added capture limit and autobalance
	int				vote_captureLimit;
	bool			vote_autobalance;
// RAVEN END
	int			vote_controlTime;
	char			value[ MAX_STRING_CHARS ];

	assert( clientNum != -1 );
	assert( !gameLocal.isClient );

	if( !gameLocal.serverInfo.GetBool( "si_allowVoting" ) ) {
		return;
	}

	voteIndex = (vote_flags_t)msg.ReadByte( );
	msg.ReadString( value, sizeof( value ) );

	// sanity checks - setup the vote
	if ( vote != VOTE_NONE ) {
		gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104273" );
		common->DPrintf( "client %d: called vote while voting already in progress - ignored\n", clientNum );
		return;
	}
	switch ( voteIndex ) {
		case VOTE_RESTART: {
			ServerStartVote( clientNum, voteIndex, "" );
			ClientStartVote( clientNum, common->GetLocalizedString( "#str_104271" ) );
			break;
		}
		case VOTE_NEXTMAP: {
			ServerStartVote( clientNum, voteIndex, "" );
			ClientStartVote( clientNum, common->GetLocalizedString( "#str_104272" ) );
			break;
		}
		case VOTE_TIMELIMIT: {
			vote_timeLimit = strtol( value, NULL, 10 );
			if ( vote_timeLimit == gameLocal.serverInfo.GetInt( "si_timeLimit" ) ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104270" );
				common->DPrintf( "client %d: already at the voted Time Limit\n", clientNum );
				return;					
			}
			if ( vote_timeLimit < si_timeLimit.GetMinValue() || vote_timeLimit > si_timeLimit.GetMaxValue() ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104269" );
				common->DPrintf( "client %d: timelimit value out of range for vote: %s\n", clientNum, value );
				return;
			}
			ServerStartVote( clientNum, voteIndex, value );
			ClientStartVote( clientNum, va( common->GetLocalizedString( "#str_104268" ), vote_timeLimit ) );
			break;
		}
		case VOTE_FRAGLIMIT: {
			vote_fragLimit = strtol( value, NULL, 10 );
			if ( vote_fragLimit == gameLocal.serverInfo.GetInt( "si_fragLimit" ) ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104267" );
				common->DPrintf( "client %d: already at the voted Frag Limit\n", clientNum );
				return;
			}
			if ( vote_fragLimit < si_fragLimit.GetMinValue() || vote_fragLimit > si_fragLimit.GetMaxValue() ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104266" );
				common->DPrintf( "client %d: fraglimit value out of range for vote: %s\n", clientNum, value );
				return;
			}
			ServerStartVote( clientNum, voteIndex, value );
			ClientStartVote( clientNum, va( common->GetLocalizedString( "#str_104303" ), common->GetLocalizedString( "#str_104265" ), vote_fragLimit ) );
			break;
		}
		case VOTE_GAMETYPE: {
// RAVEN BEGIN
// shouchard:  removed magic numbers & added CTF type
			vote_gameTypeIndex = strtol( value, NULL, 10 );
			assert( vote_gameTypeIndex >= 0 && vote_gameTypeIndex < VOTE_GAMETYPE_COUNT );
			idStr::Copynz( value, VoteGameTypeToString( vote_gameTypeIndex ), sizeof( value ) );
			if ( !idStr::Icmp( value, gameLocal.serverInfo.GetString( "si_gameType" ) ) ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104259" );
				common->DPrintf( "client %d: already at the voted Game Type\n", clientNum );
				return;
			}
			ServerStartVote( clientNum, voteIndex, value );
			ClientStartVote( clientNum, va( common->GetLocalizedString( "#str_104258" ), value ) );
			break;
		}
		case VOTE_KICK: {
			vote_clientNum = strtol( value, NULL, 10 );
			if ( vote_clientNum == gameLocal.localClientNum ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104257" );
				common->DPrintf( "client %d: called kick for the server host\n", clientNum );
				return;
			}
			ServerStartVote( clientNum, voteIndex, va( "%d", vote_clientNum ) );
			ClientStartVote( clientNum, va( common->GetLocalizedString( "#str_104302" ), vote_clientNum, gameLocal.userInfo[ vote_clientNum ].GetString( "ui_name" ) ) );
			break;
		}
		case VOTE_MAP: {
#ifdef _XENON
			// Xenon should not get here
			assert( 0 );
#else
			if ( idStr::FindText( gameLocal.serverInfo.GetString( "si_map" ), value ) != -1 ) {

				// mekberg: localized string
				const char* mapName = si_map.GetString();
				const idDict *mapDict = fileSystem->GetMapDecl( mapName );
				if ( mapDict ) {
					mapName = common->GetLocalizedString( mapDict->GetString( "name", mapName ) );
				}
				gameLocal.ServerSendChatMessage( clientNum, "server", va( common->GetLocalizedString( "#str_104295" ), mapName ) );
				common->DPrintf( "client %d: already running the voted map: %s\n", clientNum, value );
				return;
			}
			int				num = fileSystem->GetNumMaps();
			int				i;
			const idDict	*dict = NULL;
			bool			haveMap = false;
			for ( i = 0; i < num; i++ ) {
				dict = fileSystem->GetMapDecl( i );
				if( !dict ) {
					gameLocal.Warning( "idMultiplayerGame::ServerCallVote() - bad map decl index on vote\n"	);
					break;
				}
				if ( dict && !idStr::Icmp( dict->GetString( "path" ), value ) ) {
					haveMap = true;
					break;
				}
			}
			if ( !haveMap ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104296", value );
				common->Printf( "client %d: map not found: %s\n", clientNum, value );
				return;
			}
			ServerStartVote( clientNum, voteIndex, value );
			ClientStartVote( clientNum, va( common->GetLocalizedString( "#str_104256" ), dict ? dict->GetString( "name" ) : value ) );
#endif
			break;
		}
		case VOTE_BUYING: {
			vote_buying = strtol( value, NULL, 10 );
			if ( vote_buying == gameLocal.serverInfo.GetInt( "si_isBuyingEnabled" ) ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_122013" );
				common->DPrintf( "client %d: already at the voted buying mode\n", clientNum );
				return;					
			}
			ServerStartVote( clientNum, voteIndex, value );
			ClientStartVote( clientNum, va( common->GetLocalizedString( "#str_122014" ), vote_buying ) );
			break;
		}
// RAVEN BEGIN
// shouchard:  added capture limit, round limit, and autobalance
		case VOTE_CAPTURELIMIT: {
			vote_captureLimit = strtol( value, NULL, 10 );
			if ( vote_captureLimit == gameLocal.serverInfo.GetInt( "si_captureLimit" ) ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104401" );
				common->DPrintf( "client %d: already at the voted Capture Limit\n", clientNum );
				return;					
			}
			if ( vote_captureLimit < si_captureLimit.GetMinValue() || vote_captureLimit > si_fragLimit.GetMaxValue() ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104402" );
				common->DPrintf( "client %d: fraglimit value out of range for vote: %s\n", clientNum, value );
				return;
			}

			ServerStartVote( clientNum, voteIndex, value );
			ClientStartVote( clientNum, "si_captureLimit" );
			break;
		}
		// round limit is for tourneys
		case VOTE_ROUNDLIMIT: {
			// need a CVar or something to change here
			break;
		}
		case VOTE_AUTOBALANCE: {
			vote_autobalance = (0 != strtol( value, NULL, 10 ) );
			if ( vote_autobalance == gameLocal.serverInfo.GetBool( "si_autobalance" ) ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104403" );
				common->DPrintf( "client %d: already at the voted balance teams\n", clientNum );
				return;					
			}

			ServerStartVote( clientNum, voteIndex, value );
			ClientStartVote( clientNum, "si_autobalance" );
			break;
		}
// RAVEN END
		case VOTE_CONTROLTIME: {
			vote_controlTime = strtol( value, NULL, 10 );
			if ( vote_controlTime == gameLocal.serverInfo.GetInt( "si_controlTime" ) ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_122017" );
				common->DPrintf( "client %d: already at the voted Control Time\n", clientNum );
				return;					
			}
			if ( vote_controlTime < si_controlTime.GetMinValue() || vote_controlTime > si_controlTime.GetMaxValue() ) {
				gameLocal.ServerSendChatMessage( clientNum, "server", "#str_122018" );
				common->DPrintf( "client %d: controlTime value out of range for vote: %s\n", clientNum, value );
				return;
			}

			ServerStartVote( clientNum, voteIndex, value );
			ClientStartVote( clientNum, "si_controlTime" );
			break;
		}
		default: {
			gameLocal.ServerSendChatMessage( clientNum, "server", "#str_104297", va( "%d", ( int )voteIndex ) );
			common->DPrintf( "client %d: unknown vote index %d\n", clientNum, voteIndex );
		}
	}
}


/*
================
idMultiplayerGame::DisconnectClient
================
*/
void idMultiplayerGame::DisconnectClient( int clientNum ) {
	// gameLocal.entities[ clientNum ] could be null if server is shutting down
	if( gameLocal.entities[ clientNum ] ) {
		// only kill non-spectators
		if( !((idPlayer*)gameLocal.entities[ clientNum ])->spectating ) {
			static_cast<idPlayer *>( gameLocal.entities[ clientNum ] )->Kill( true, true );
		}
		statManager->ClientDisconnect( clientNum );
	}

	delete gameLocal.entities[ clientNum ];

	UpdatePlayerRanks();
	CheckAbortGame();

	privatePlayers &= ~( 1 << clientNum );

	// update serverinfo
	UpdatePrivatePlayerCount();
}

/*
================
idMultiplayerGame::CheckAbortGame
================
*/
void idMultiplayerGame::CheckAbortGame( void ) {
	// only checks for aborts -> game review below
	if ( gameState->GetMPGameState() != COUNTDOWN && gameState->GetMPGameState() != GAMEON && gameState->GetMPGameState() != SUDDENDEATH ) {
		return;
	}

	// in tourney, if we don't have enough clients to play we need to cycle back to 
	// warmup to re-seed
	if( gameLocal.gameType == GAME_TOURNEY ) {
		if ( !EnoughClientsToPlay() ) {
			gameState->NewState( WARMUP );
		}
	} else {
		if ( !EnoughClientsToPlay() && TimeLimitHit() ) {
			gameState->NewState( GAMEREVIEW );
		}
	}
}

/*
================
idMultiplayerGame::WantKilled
================
*/
void idMultiplayerGame::WantKilled( int clientNum ) {
	idEntity *ent = gameLocal.entities[ clientNum ];
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( ent && ent->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
		static_cast<idPlayer *>( ent )->Kill( false, false );
	}
}

/*
================
idMultiplayerGame::ClearVote
================
*/
void idMultiplayerGame::ClearVote( int clientNum ) {
	int start = 0;
	int end = MAX_CLIENTS;
	
	if( clientNum != -1 ) {
		start = clientNum;
		end = clientNum + 1;
	}
	
	for ( int i = start; i < end; i++ ) {
		idPlayer *player = static_cast<idPlayer *>( gameLocal.entities[ i ] );
		if ( !player || !player->mphud ) {
			continue;
		}
	
		player->mphud->SetStateInt( "voteNotice", 0 );
		player->mphud->SetStateString( "voteInfo_1", "" );
		player->mphud->SetStateString( "voteInfo_2", "" );
		player->mphud->SetStateString( "voteInfo_3", "" );
		player->mphud->SetStateString( "voteInfo_4", "" );
		player->mphud->SetStateString( "voteInfo_5", "" );
		player->mphud->SetStateString( "voteInfo_6", "" );
		player->mphud->SetStateString( "voteInfo_7", "" );		
		player->mphud->StateChanged( gameLocal.time );
	}
	// clear the local demo player's vote too
	if ( clientNum == -1 && gameLocal.IsServerDemoPlaying() ) do {
		idPlayer *player = gameLocal.GetLocalPlayer();
		if ( !player || !player->mphud ) {
			continue;
		}
	
		player->mphud->SetStateInt( "voteNotice", 0 );
		player->mphud->SetStateString( "voteInfo_1", "" );
		player->mphud->SetStateString( "voteInfo_2", "" );
		player->mphud->SetStateString( "voteInfo_3", "" );
		player->mphud->SetStateString( "voteInfo_4", "" );
		player->mphud->SetStateString( "voteInfo_5", "" );
		player->mphud->SetStateString( "voteInfo_6", "" );
		player->mphud->SetStateString( "voteInfo_7", "" );		
		player->mphud->StateChanged( gameLocal.time );
	} while(0);
	if ( mainGui ) {
		mainGui->SetStateInt( "vote_going", 0 );
		mainGui->StateChanged( gameLocal.time );
	}
}
/*
================
idMultiplayerGame::MapRestart
================
*/
void idMultiplayerGame::MapRestart( void ) {
	int clientNum;
	// jshepard: clean up votes
	ClearVote();

	ClearAnnouncerSounds();

	assert( !gameLocal.isClient );
	if ( gameLocal.GameState() != GAMESTATE_SHUTDOWN && gameState->GetMPGameState() != WARMUP ) {
		gameState->NewState( WARMUP );
		// force an immediate state detection/update, otherwise if we update our state this
		// same frame we'll miss transitions
		gameState->SendState( serverReliableSender.To( -1 ) );

		gameState->SetNextMPGameState( INACTIVE );
		gameState->SetNextMPGameStateTime( 0 );
		
	}

	// mekberg: moved this before the updateUI just in case these values weren't reset.
	for ( int i = 0; i < TEAM_MAX; i++ ) {
		teamScore[ i ] = 0;
		teamDeadZoneScore[i] = 0;
	}

	// mekberg: Re-wrote this loop to always updateUI. Previously the player would be
	//			on a team but the UI wouldn't know about it
	// shouchard:  balance teams extended to CTF	
	for ( clientNum = 0; clientNum < gameLocal.numClients; clientNum++ ) {
		// jnewquist: Use accessor for static class type 
		if ( gameLocal.entities[ clientNum ] && gameLocal.entities[ clientNum ]->IsType( idPlayer::GetClassType() ) ) {
			// mekberg: clear wins only on map restart
			idPlayer *player = static_cast<idPlayer *>( gameLocal.entities[ clientNum ] );
			SetPlayerWin( player, 0 );
			
			/*if( clientNum == gameLocal.localClientNum ) {
				if ( player->alreadyDidTeamAnnouncerSound ) {
					player->alreadyDidTeamAnnouncerSound = false;
				} else {
					if ( gameLocal.IsTeamGame() ) {
						player->alreadyDidTeamAnnouncerSound = true;
						if( player->team == TEAM_STROGG ) {
							ScheduleAnnouncerSound( AS_TEAM_JOIN_STROGG, gameLocal.time + 500 );
						} else if( player->team == TEAM_MARINE ) {
							ScheduleAnnouncerSound( AS_TEAM_JOIN_MARINE, gameLocal.time + 500 );
						}
					}
				}
			}*/
			// let the player rejoin the team through normal channels
			//player->ServerSpectate( true );
			//player->team = -1;
			//player->latchedTeam = -1;

			// shouchard:  BalanceTDM->BalanceTeam
			//if ( gameLocal.serverInfo.GetBool( "si_autoBalance" ) && gameLocal.IsTeamGame() )  {
			//	player->BalanceTeam();
			//}

			// core is in charge of syncing down userinfo changes
			// it will also call back game through SetUserInfo with the current info for update
			/*cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "updateUI %d\n", clientNum ) );*/
		}
	}
}

/*
================
idMultiplayerGame::SwitchToTeam
================
*/
void idMultiplayerGame::SwitchToTeam( int clientNum, int oldteam, int newteam ) {
	assert( gameLocal.IsTeamGame() );

	assert( oldteam != newteam );
	assert( !gameLocal.isClient );

	if ( !gameLocal.isClient && newteam >= 0 ) {
		// clients might not have userinfo of joining client at this point, so
		// send down the player's name
		idPlayer *p = static_cast<idPlayer *>( gameLocal.entities[ clientNum ] );
		if ( !p->wantSpectate ) {		
			PrintMessage( -1, va( common->GetLocalizedString( "#str_104280" ), gameLocal.userInfo[ clientNum ].GetString( "ui_name" ), newteam ? common->GetLocalizedString( "#str_108025" ) : common->GetLocalizedString( "#str_108026" ) ) );
		}
	}
	
	if ( oldteam != -1 ) {
		// kill and respawn
		idPlayer *p = static_cast<idPlayer *>( gameLocal.entities[ clientNum ] );
		if ( p->IsInTeleport() ) {
 			p->ServerSendInstanceEvent( idPlayer::EVENT_ABORT_TELEPORTER, NULL, false, -1 );
			p->SetPrivateCameraView( NULL );
		}
//RITUAL BEGIN
		p->inventory.carryOverWeapons = 0;
		p->ResetCash();
//RITUAL END
		p->Kill( true, true );
		CheckAbortGame();
	}
}

/*
================
idMultiplayerGame::JoinTeam
================
*/
void idMultiplayerGame::JoinTeam( const char* team ) {
	if( !idStr::Icmp( team, "auto" ) ) {
		int			teamCount[ TEAM_MAX ];
		idEntity	*ent;
		
		memset( teamCount, 0, sizeof( int ) * TEAM_MAX );

		for( int i = 0; i < gameLocal.numClients; i++ ) {
			ent = gameLocal.entities[ i ];
			if ( ent && ent->IsType( idPlayer::GetClassType() ) ) {
				if ( !static_cast< idPlayer * >( ent )->spectating ) {
					teamCount[ ((idPlayer*)ent)->team ]++;
				}
			}
		}

		int minCount = idMath::INT_MAX;
		int minCountTeam = -1;
		for( int i = 0; i < TEAM_MAX; i++ ) {
			if( teamCount[ i ] < minCount ) {
				minCount = teamCount[ i ];
				minCountTeam = i;
			}
		}

		if( minCountTeam >= 0 && minCountTeam < TEAM_MAX ) {
			cvarSystem->SetCVarString( "ui_spectate", "Play" );
			cvarSystem->SetCVarString( "ui_team", teamNames[ minCountTeam ] );
		} else {
			cvarSystem->SetCVarString( "ui_spectate", "Play" );
			cvarSystem->SetCVarString( "ui_team", teamNames[ gameLocal.random.RandomInt( TEAM_MAX - 1 ) ] );
		}
	} else if( !idStr::Icmp( team, "spectator" ) ) {
		cvarSystem->SetCVarString( "ui_spectate", "Spectate" );
	} else {
		int i;
		for( i = 0; i < TEAM_MAX; i++ ) {
			if( !idStr::Icmp( team, teamNames[ i ] ) ) {
				cvarSystem->SetCVarString( "ui_spectate", "Play" );
				cvarSystem->SetCVarString( "ui_team", teamNames[ i ] );
				break;
			}
		}
		if( i >= TEAM_MAX ) {
			gameLocal.Warning( "idMultiplayerGame::JoinTeam() - unknown team '%s'\n", team );
		}
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
	const char *suffix = NULL;
	int			send_to; // 0 - all, 1 - specs, 2 - team
	int			i;
	idEntity 	*ent;
	idPlayer	*p;
	idStr		suffixed_name;
	idStr		prefixed_text;

	assert( !gameLocal.isClient );

	if ( clientNum >= 0 ) {
		p = static_cast< idPlayer * >( gameLocal.entities[ clientNum ] );
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !( p && p->IsType( idPlayer::GetClassType() ) ) ) {
// RAVEN END
			return;
		}

		if ( p->spectating && ( p->wantSpectate || gameLocal.gameType == GAME_TOURNEY ) ) {
			suffix = "spectating";
			if ( team || ( !g_spectatorChat.GetBool() && ( gameState->GetMPGameState() == GAMEON || gameState->GetMPGameState() == SUDDENDEATH ) ) ) {
				// to specs
				send_to = 1;
			} else {
				// to all
				send_to = 0;
			}
		} else if ( team ) {
			suffix = va( "%s%s", p->team ? S_COLOR_STROGG : S_COLOR_MARINE, p->team ? "Strogg^0" : "Marine^0" ); 
			// to team
			send_to = 2;
		} else {
			if( gameLocal.gameType == GAME_TOURNEY ) {
				suffix = va( "Arena %d", (p->GetArena() + 1) );
			}
			// to all
			send_to = 0;
		}
	} else {
		p = NULL;
		send_to = 0;
	}
	// put the message together
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( ( send_to == 2 ) ? GAME_RELIABLE_MESSAGE_TCHAT : GAME_RELIABLE_MESSAGE_CHAT );

	if ( suffix ) {
		suffixed_name = va( "^0%s^0 (%s)", name, suffix );
	} else {
		suffixed_name = va( "^0%s^0", name );
	}
	if( p && send_to == 2 ) {
		prefixed_text = va( "%s%s", p->team ? S_COLOR_STROGG : S_COLOR_MARINE, common->GetLocalizedString( text ) );
	} else {
		prefixed_text = common->GetLocalizedString( text );
	}

	if( suffixed_name.Length() + prefixed_text.Length() >= 240 ) {
		gameLocal.Warning( "idMultiplayerGame::ProcessChatMessage() - Chat line too long\n" );
		return;
	}

	outMsg.WriteString( suffixed_name );
 	outMsg.WriteString( prefixed_text );
 	outMsg.WriteString( "" );

	for ( i = 0; i < gameLocal.numClients; i++ ) {
		ent = gameLocal.entities[ i ]; 
		if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
			continue;
		}
		idPlayer *to = static_cast< idPlayer * >( ent );
		switch( send_to ) {
			case 0:
				if ( !p || !to->IsPlayerMuted( p ) ) {
					if ( i == gameLocal.localClientNum ) {
						AddChatLine( "%s^0: %s\n", suffixed_name.c_str(), prefixed_text.c_str() );
					} else {
						networkSystem->ServerSendReliableMessage( i, outMsg );
					}
				}
				break;

			case 1:
				if ( !p || ( to->spectating && !to->IsPlayerMuted( p ) ) ) {
					if ( i == gameLocal.localClientNum ) {
						AddChatLine( "%s^0: %s\n", suffixed_name.c_str(), prefixed_text.c_str() );
					} else {
						networkSystem->ServerSendReliableMessage( i, outMsg );
					}
				}
				break;

			case 2:
				if ( !p || ( to->team == p->team && !to->IsPlayerMuted( p ) ) ) {
					if ( !to->spectating ) {
						if ( i == gameLocal.localClientNum ) {
							PrintChatLine( va( "%s^0: %s\n", suffixed_name.c_str(), prefixed_text.c_str() ), true );
						} else {
							networkSystem->ServerSendReliableMessage( i, outMsg );
						}
					}
				}
				break;
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
	gameLocal.FindEntityDef( "player_marine", false );
	
	// MP game sounds
	for ( i = 0; i < AS_NUM_SOUNDS; i++ ) {
		declManager->FindSound( announcerSoundDefs[ i ], false );
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
 			gameLocal.mpGame.AddChatLine( common->GetLocalizedString( "#str_106747" ) );
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

	if ( lastReadyToggleTime == -1 ) {
		lastReadyToggleTime = gameLocal.time;
	} else {
		int currentTime = gameLocal.time;
		if ( currentTime - lastReadyToggleTime < 500 ) {
			return;
		} else {
			lastReadyToggleTime = currentTime;
		}
	}	

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
	
	// RAVEN BEGIN
	// ddynerman: new multiplayer teams
	team = ( idStr::Icmp( cvarSystem->GetCVarString( "ui_team" ), "Marine" ) == 0 );
	if ( team ) {
		cvarSystem->SetCVarString( "ui_team", "Strogg" );
	} else {
		cvarSystem->SetCVarString( "ui_team", "Marine" );
	}
	// RAVEN END
}

/*
================
idMultiplayerGame::ToggleUserInfo
================
*/
void idMultiplayerGame::ThrottleUserInfo( void ) {
	int i;

	if ( gameLocal.localClientNum == MAX_CLIENTS ) {
		// repeater; UserInfo doesn't get changed in-game anyway.
		return;
	}

	assert( gameLocal.localClientNum >= 0 );

	i = 0;
	while ( ThrottleVars[ i ] ) {
		if ( idStr::Icmp( gameLocal.userInfo[ gameLocal.localClientNum ].GetString( ThrottleVars[ i ] ),
			cvarSystem->GetCVarString( ThrottleVars[ i ] ) ) ) {
			if ( gameLocal.realClientTime < switchThrottle[ i ] ) {
				AddChatLine( common->GetLocalizedString( "#str_104299" ), common->GetLocalizedString( ThrottleVarsInEnglish[ i ] ), ( switchThrottle[ i ] - gameLocal.time ) / 1000 + 1 );
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
 			//gameLocal.ServerSendChatMessage( -1, common->GetLocalizedString( "#str_102047" ), va( common->GetLocalizedString( "#str_107177" ), gameLocal.userInfo[ clientNum ].GetString( "ui_name" ) ) );
 		}
 	
		// mark them as private and update si_numPrivatePlayers
		for( int i = 0; i < privateClientIds.Num(); i++ ) {
			int num = networkSystem->ServerGetClientNum( privateClientIds[ i ] );

			// check for timed out clientids
			if( num < 0 ) {
				privateClientIds.RemoveIndex( i );
				i--;
				continue;
			}

			if( num == clientNum ) {
				privatePlayers |= (1 << clientNum);
			}
		}

		// update serverinfo
		UpdatePrivatePlayerCount();
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
idMultiplayerGame::UpdateMPSettingsModel
================
*/
void idMultiplayerGame::UpdateMPSettingsModel( idUserInterface* currentGui ) {
	if ( !currentGui ) {
		return;
	}

	const char *model;
	idPlayer	*localP = gameLocal.GetLocalPlayer();
	if ( gameLocal.IsTeamGame() && localP && localP->team >= 0 && localP->team < TEAM_MAX ) {
		model = cvarSystem->GetCVarString( va( "ui_model_%s", teamNames[ localP->team ] ) );
		if ( idStr::Cmp( model, "" ) == 0 ) {
			const idDeclEntityDef *def = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, "player_marine_mp_ui", false, true ) );
			model = def->dict.GetString( va( "def_default_model_%s", teamNames[ localP->team ] ) );
			cvarSystem->SetCVarString( va( "ui_model_%s", teamNames[ localP->team ] ), model );
		}
	} else {
		model = cvarSystem->GetCVarString( "ui_model" );

		if ( idStr::Cmp( model, "" ) == 0 ) {
			const idDeclEntityDef *def = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, "player_marine_mp_ui", false, true ) );
			model = def->dict.GetString( "def_default_model" );
			cvarSystem->SetCVarString( "ui_model", model );
		}
	}
	const rvDeclPlayerModel* playerModel = (const rvDeclPlayerModel*)declManager->FindType( DECL_PLAYER_MODEL, model, false );
	if ( playerModel ) {
		currentGui->SetStateString( "player_model_name", playerModel->model.c_str() );
		currentGui->SetStateString( "player_head_model_name", playerModel->uiHead.c_str() );
		currentGui->SetStateString( "player_skin_name", playerModel->skin.c_str() );
		if( playerModel->uiHead.Length() ) {
			const idDeclEntityDef* head = (const idDeclEntityDef*)declManager->FindType( DECL_ENTITYDEF, playerModel->uiHead.c_str(), false );
			if( head && head->dict.GetString( "skin" ) ) {
				mainGui->SetStateString( "player_head_skin_name", head->dict.GetString( "skin" ) );
			}
		}
		currentGui->SetStateBool( "need_update", true );
	}
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
	spawnArgs = gameLocal.FindEntityDefDict( "player_marine", false );
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( !( p && p->IsType( idPlayer::GetClassType() ) ) ) {
// RAVEN END
		return;
	}

	if ( p->spectating ) {
		return;
	}

	// lookup the sound def
	spawnArgs = gameLocal.FindEntityDefDict( "player_marine", false );
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
	if ( team || gameState->GetMPGameState() == COUNTDOWN || gameState->GetMPGameState() == GAMEREVIEW ) {
		ProcessChatMessage( clientNum, team, name, spawnArgs->GetString( text_key ), spawnArgs->GetString( snd_key ) );
	} else {
		p->StartSound( snd_key, SND_CHANNEL_ANY, 0, true, NULL );
		ProcessChatMessage( clientNum, team, name, spawnArgs->GetString( text_key ), NULL );
	}
}

// RAVEN BEGIN
// shouchard:  added commands to mute/unmute voice chat
/*
================
idMultiplayerGame::ClientVoiceMute
================
*/
void idMultiplayerGame::ClientVoiceMute( int muteClient, bool mute ) {
	// clients/listen server only
	assert( gameLocal.isListenServer || gameLocal.isClient );

	if ( NULL == gameLocal.GetLocalPlayer() ) {
		return;
	}

	if ( muteClient == -1 || !gameLocal.mpGame.IsInGame( muteClient ) ) {
		gameLocal.Warning( "idMultiplayerGame::ClientVoiceMute() - Invalid client '%d' specified", muteClient );
		return;
	}

	// do the mute/unmute
	gameLocal.GetLocalPlayer()->MutePlayer( muteClient, mute );

	// tell the server
	if( gameLocal.isClient ) {
		idBitMsg outMsg;
		byte msgBuf[128];
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_VOICECHAT_MUTING );
		outMsg.WriteByte( muteClient );
		outMsg.WriteByte( mute ? 1 : 0 ); // 1 for mute, 0 for unmute
		networkSystem->ClientSendReliableMessage( outMsg );
	}

	// display some niceties
	common->Printf( "Player %s's has been %s.\n", gameLocal.GetUserInfo( muteClient )->GetString( "ui_name" ), mute ? "muted" : "unmuted" );
}

/*
================
idMultiplayerGame::GetClientNumFromPlayerName
================
*/
int idMultiplayerGame::GetClientNumFromPlayerName( const char *playerName ) {
	if ( NULL == playerName || '\0' == *playerName ) {
		return -1;
	}

	int clientNum = -1;

	for ( int i = 0; i < gameLocal.numClients; i++ ) {
		if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idPlayer::GetClassType() ) ) {
			if ( 0 == idStr::Icmp( gameLocal.userInfo[ i ].GetString( "ui_name" ), playerName ) ) {
				clientNum = i;
				break;
			}
		}
	}

	if ( -1 == clientNum ) {
		common->Warning( "idMultiplayerGame::GetClientNumFromPlayerName():  unknown player '%s'", playerName );
	}

	return clientNum;
}

/*
================
idMultiplayerGame::ServerHandleVoiceMuting
================
*/
void idMultiplayerGame::ServerHandleVoiceMuting( int clientSrc, int clientDest, bool mute ) {
	assert( !gameLocal.isClient );

	idPlayer *playerSrc = gameLocal.GetClientByNum( clientSrc );
	idPlayer *playerDest = gameLocal.GetClientByNum( clientDest );

	if ( NULL == playerSrc ) {
		common->DPrintf( "idMultiplayerGame::ServerHandleVoiceMuting:  couldn't map client %d to a player\n", clientSrc );
		return;
	}

	if ( NULL == playerDest ) {
		common->DPrintf( "idMultiplayerGame::ServerHandleVoiceMuting:  couldn't map client %d to a player\n", clientDest );
		return;
	}

	if ( mute ) {
		playerSrc->MutePlayer( playerDest, true );
		common->DPrintf( "DEBUG:  client %s muted to client %s\n", 
			gameLocal.userInfo[ clientDest ].GetString( "ui_name" ),
			gameLocal.userInfo[ clientSrc ].GetString( "ui_name" ) );
	} else {
		playerSrc->MutePlayer( playerDest, false );
		common->DPrintf( "DEBUG:  client %s unmuted to client %s\n", 
			gameLocal.userInfo[ clientDest ].GetString( "ui_name" ),
			gameLocal.userInfo[ clientSrc ].GetString( "ui_name" ) );
	}
}


/*
================
idMultiplayerGame::ClearAnnouncerSounds

This method deletes unplayed announcer sounds at the end of a game round.  
This fixes a bug where the round time warnings were being played from 
previous rounds.
================
*/
void idMultiplayerGame::ClearAnnouncerSounds( void ) {
	announcerSoundNode_t* snd = NULL;	
	announcerSoundNode_t* nextSnd = NULL;	
	
	for ( snd = announcerSoundQueue.Next(); snd != NULL; snd = nextSnd ) {
		nextSnd = snd->announcerSoundNode.Next();
		snd->announcerSoundNode.Remove ( );
		delete snd;
	}

	announcerPlayTime = 0;
}

/*
================
idMultiplayerGame::HandleServerAdminBanPlayer
================
*/	
void idMultiplayerGame::HandleServerAdminBanPlayer( int clientNum ) {
	if ( clientNum < 0 || clientNum >= gameLocal.numClients ) {
		common->DPrintf( "idMultiplayerGame::HandleServerAdminBanPlayer:  bad client num %d\n", clientNum );
		return;
	}

	if ( gameLocal.isServer	|| gameLocal.isListenServer ) {
		if ( gameLocal.isListenServer && clientNum == gameLocal.localClientNum ) {
			common->DPrintf( "idMultiplayerGame::HandleServerAdminBanPlayer: Cannot ban the host!\n" );
			return;
		}
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "kick %i ban", clientNum ) );
	} else {
		if ( clientNum == gameLocal.localClientNum ) {
			common->DPrintf( "idMultiplayerGame::HandleServerAdminBanPlayer: Cannot ban yourserlf!\n" );
			return;
		}
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon kick %i ban", clientNum ) );		
	}
}

/*
================
idMultiplayerGame::HandleServerAdminRemoveBan
================
*/
void idMultiplayerGame::HandleServerAdminRemoveBan( const char * clientGuid ) {
	if ( NULL == clientGuid || !clientGuid[ 0 ]) {
		common->DPrintf( "idMultiplayerGame::HandleServerAdminRemoveBan:  bad guid!\n" );
		return;
	}

	if ( gameLocal.isServer || gameLocal.isListenServer ) {
		gameLocal.RemoveGuidFromBanList( clientGuid );
	} else {
		int clientNum = gameLocal.GetClientNumByGuid( clientGuid );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon removeClientFromBanList %d", clientNum ) ); 
	}
}

/*
================
idMultiplayerGame::HandleServerAdminKickPlayer
================
*/
void idMultiplayerGame::HandleServerAdminKickPlayer( int clientNum ) {
	if ( clientNum < 0 || clientNum >= gameLocal.numClients ) {
		common->DPrintf( "idMultiplayerGame::HandleServerAdminKickPlayer:  bad client num %d\n", clientNum );
		return;
	}

	if ( gameLocal.isServer || gameLocal.isListenServer ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "kick %i", clientNum ) );
	} else { 
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon kick %i", clientNum ) );
	}
}

/*
================
idMultiplayerGame::HandleServerAdminForceTeamSwitch
================
*/
void idMultiplayerGame::HandleServerAdminForceTeamSwitch( int clientNum ) {
	if ( !gameLocal.IsTeamGame() ) {
		return;
	}

	if ( clientNum < 0 || clientNum >= gameLocal.numClients ) {
		common->DPrintf( "idMultiplayerGame::HandleServerAdminForceTeamSwitch:  bad client num %d\n", clientNum );
		return;
	}

	if ( gameLocal.isServer || gameLocal.isListenServer ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "forceTeamChange %d\n", clientNum));

/*		if ( gameLocal.entities[ clientNum ] && gameLocal.entities[ clientNum ]->IsType( idPlayer::GetClassType() ) )
		{
			idPlayer *player = static_cast< idPlayer *>( gameLocal.entities[ clientNum ] );
			player->GetUserInfo()->Set( "ui_team", player->team ? "Marine" : "Strogg" );
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "updateUI %d\n", clientNum ) );
		}*/
	} else {
/*		idBitMsg outMsg;
		byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SERVER_ADMIN );
		outMsg.WriteByte( SERVER_ADMIN_FORCE_SWITCH );
		outMsg.WriteByte( clientNum );
		networkSystem->ClientSendReliableMessage( outMsg ); */

		//jshepard: need to be able to do this via rcon
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon forceTeamChange %d\n", clientNum));

	}
}

/*
================
idMultiplayerGame::HandleServerAdminCommands
================
*/
bool idMultiplayerGame::HandleServerAdminCommands( serverAdminData_t &data ) {
	bool restartNeeded = false;
	bool nextMapNeeded = false;
	bool anyChanges = false;
	bool runPickMap = false;
	int nGameType = 0;
	idStr currentMap = si_map.GetString( );

	const char *szGameType = gameLocal.serverInfo.GetString( "si_gametype" );
	if ( 0 == idStr::Icmp( szGameType, "DM" ) ) {
		nGameType = GAME_DM;
	} else if ( 0 == idStr::Icmp( szGameType, "Team DM" ) ) {
		nGameType = GAME_TDM;
	} else if ( 0 == idStr::Icmp( szGameType, "CTF" ) ) {
		nGameType = GAME_CTF;
	} else if ( 0 == idStr::Icmp( szGameType, "Tourney" ) ) {
		nGameType = GAME_TOURNEY;
	} else if ( 0 == idStr::Icmp( szGameType, "Arena CTF" ) ) {
		nGameType = GAME_ARENA_CTF;
	} else if ( 0 == idStr::Icmp( szGameType, "DeadZone" ) ) {
		nGameType = GAME_DEADZONE;
	} else {
		nGameType = GAME_SP;
	}
	if ( nGameType != data.gameType ) {
		
		switch ( data.gameType ) {
			case GAME_TDM:			szGameType = "Team DM";		runPickMap = true; break;
			case GAME_TOURNEY:		szGameType = "Tourney";		runPickMap = true; break;
			case GAME_CTF:			szGameType = "CTF";			runPickMap = true; break;
			case GAME_ARENA_CTF:	szGameType = "Arena CTF";	runPickMap = true; break;

			// mekberg: hack, if we had 1f ctf the gui index wouldn't be off =(
			case GAME_1F_CTF:		szGameType = "Arena CTF";	runPickMap = true; break;
			case GAME_DEADZONE:		szGameType = "DeadZone";	runPickMap = true; break;
			default:
			case GAME_DM:			szGameType = "DM";			break;
		}

		//we're going to reset the map here, so make sure to kill the active vote.
		ClientUpdateVote( VOTE_RESET, 0, 0, currentVoteData );
		vote = VOTE_NONE;
		restartNeeded = true;
		anyChanges = true;

		si_gameType.SetString( szGameType );
		if( runPickMap && gameLocal.isServer )	{
			//set the selected map to the admin data value, then make sure it can run the selected gametype.
			si_map.SetString( data.mapName.c_str() );
			if( PickMap( szGameType ) || idStr::Icmp( si_map.GetString( ), currentMap.c_str( ) ) )	{
				nextMapNeeded = true;
				restartNeeded = false;
				data.mapName = idStr( si_map.GetString() );
				data.restartMap = true;
			}
		}
	} 

	if ( gameLocal.serverInfo.GetBool( "si_isBuyingEnabled" ) != data.buying )
		restartNeeded = true;

	// Rcon these cvars if this isn't the server. We can trust the input from the gui that the
	// gametype and map always match.
	if ( !gameLocal.isServer ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon si_autoBalance %d",	data.autoBalance ) );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon si_isBuyingEnabled %d", data.buying ) );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon si_captureLimit %d",	data.captureLimit ) );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon si_controlTime %d",	data.controlTime ) );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon si_fragLimit %d",		data.fragLimit ) );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon si_gameType %s",		szGameType ) );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon si_map %s",			data.mapName.c_str() ) );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon si_tourneyLimit %d",	data.tourneyLimit ) );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon si_minPlayers %d",	data.minPlayers ) );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon si_timeLimit %d",		data.timeLimit ) );
		if( runPickMap ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "rcon verifyServerSettings" ) );
		}
		
		if( data.shuffleTeams ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rcon shuffleTeams" );
		}

		if( restartNeeded || data.restartMap || nextMapNeeded || idStr::Icmp( gameLocal.serverInfo.GetString( "si_map" ), data.mapName.c_str() ) ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rcon serverMapRestart" );
		}
		else
		{
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rcon rescanSI" );
		}
		
		return true;
	}

	if ( data.restartMap ) {
		ClientUpdateVote( VOTE_RESET, 0, 0, currentVoteData );
		vote = VOTE_NONE;
		restartNeeded = true;
		anyChanges = true;
	}

	if ( data.shuffleTeams ) {
		ShuffleTeams();
		anyChanges = true;
	}

	//this section won't be encountered if the gametype was changed. But that's ok.
	if ( data.mapName.c_str() && idStr::Icmp( data.mapName.c_str(), si_map.GetString() ) ) {
		ClientUpdateVote( VOTE_RESET, 0, 0, currentVoteData );
		vote = VOTE_NONE;
		si_map.SetString(data.mapName.c_str());
		cvarSystem->SetCVarString( "si_map", data.mapName.c_str() );
		nextMapNeeded = true;
		anyChanges = true;
	}

	if ( data.captureLimit != gameLocal.serverInfo.GetInt( "si_captureLimit" ) ) {
		si_captureLimit.SetInteger( data.captureLimit );
		anyChanges = true;
	}
	if ( data.fragLimit !=  gameLocal.serverInfo.GetInt( "si_fragLimit" ) ) {
		si_fragLimit.SetInteger( data.fragLimit );
		anyChanges = true;
	}
	if ( data.tourneyLimit != gameLocal.serverInfo.GetInt( "si_tourneyLimit" ) ) {
		si_tourneyLimit.SetInteger( data.tourneyLimit );
		anyChanges = true;
	}
	if ( data.timeLimit != gameLocal.serverInfo.GetInt( "si_timeLimit" ) ) {
		si_timeLimit.SetInteger( data.timeLimit );
		anyChanges = true;
	}
	if ( data.buying != gameLocal.serverInfo.GetBool( "si_isBuyingEnabled" ) ) {
		si_isBuyingEnabled.SetInteger( data.buying );
		anyChanges = true;
		restartNeeded = true;
	}
	if ( data.autoBalance != gameLocal.serverInfo.GetBool( "si_autobalance" ) ) {
		si_autobalance.SetBool( data.autoBalance );
		anyChanges = true;
	}
	if ( data.controlTime != gameLocal.serverInfo.GetInt( "si_controlTime" ) ) {
		si_controlTime.SetInteger( data.controlTime );
		anyChanges = true;
	}
	
	if ( gameLocal.NeedRestart() || restartNeeded || nextMapNeeded ) {
		ClientUpdateVote( VOTE_RESET, 0, 0, currentVoteData );
		vote = VOTE_NONE;
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "serverMapRestart" );
	} else {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" " " __FILE__ " " __LINESTR__ );
	}

	return anyChanges;
}


// RAVEN END

/*
===============
idMultiplayerGame::WriteStartState
===============
*/
 void idMultiplayerGame::WriteStartState( int clientNum, idBitMsg &msg, bool withLocalClient ) {
	int			i;
	idEntity	*ent;

	// send the start time
	msg.WriteLong( matchStartedTime );
	// send the powerup states and the spectate states
	for( i = 0; i < gameLocal.numClients; i++ ) {
		ent = gameLocal.entities[ i ]; 
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ( withLocalClient || i != clientNum ) && ent && ent->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
			msg.WriteShort( i );
			msg.WriteShort( static_cast< idPlayer * >( ent )->inventory.powerups );
			msg.WriteBits( ent->GetInstance(), ASYNC_PLAYER_INSTANCE_BITS );
			msg.WriteBits( static_cast< idPlayer * >( ent )->spectating, 1 );
		}
	}
	msg.WriteShort( MAX_CLIENTS );	
}

/*
================
idMultiplayerGame::ServerWriteInitialReliableMessages
================
*/
void idMultiplayerGame::ServerWriteInitialReliableMessages( const idMessageSender &sender, int clientNum ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_STARTSTATE );
	WriteStartState( clientNum, outMsg, false );
	sender.Send( outMsg );

	// we send SI in connectResponse messages, but it may have been modified already
	outMsg.BeginWriting( );
 	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SERVERINFO );
	if ( sender.GetChannelType() == CHANNEL_DEST_RELIABLE_REPEATER ) {
		assert( gameLocal.isRepeater );
		outMsg.WriteDeltaDict( gameLocal.repeaterInfo, NULL );
	} else {
		outMsg.WriteDeltaDict( gameLocal.serverInfo, NULL );
	}
	sender.Send( outMsg );

	gameState->SendInitialState( sender, clientNum );
}

/*
================
idMultiplayerGame::ClientReadStartState
================
*/
void idMultiplayerGame::ClientReadStartState( const idBitMsg &msg ) {
	int i, client, powerup;

	assert( gameLocal.isClient );

	// read the state in preparation for reading snapshot updates
	matchStartedTime = msg.ReadLong( );
	while ( ( client = msg.ReadShort() ) != MAX_CLIENTS ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		assert( gameLocal.entities[ client ] && gameLocal.entities[ client ]->IsType( idPlayer::GetClassType() ) );
// RAVEN END
		powerup = msg.ReadShort();

		int instance = ( msg.ReadBits( ASYNC_PLAYER_INSTANCE_BITS ) );
		static_cast< idPlayer * >( gameLocal.entities[ client ] )->SetInstance( instance );
		bool spectate = ( msg.ReadBits( 1 ) != 0 );
		static_cast< idPlayer * >( gameLocal.entities[ client ] )->Spectate( spectate );

		// set powerups after we get instance information for this client
		for ( i = 0; i < POWERUP_MAX; i++ ) {
			if ( powerup & ( 1 << i ) ) {
				static_cast< idPlayer * >( gameLocal.entities[ client ] )->GivePowerUp( i, 0 );
			}
		}
	}
}

const char* idMultiplayerGame::announcerSoundDefs[ AS_NUM_SOUNDS ] = {
	// General announcements
	"announce_general_one",					// AS_GENERAL_ONE
	"announce_general_two",					// AS_GENERAL_TWO
	"announce_general_three",				// AS_GENERAL_THREE
	"announce_general_you_win",				// AS_GENERAL_YOU_WIN
	"announce_general_you_lose",			// AS_GENERAL_YOU_LOSE
	"announce_general_fight",				// AS_GENERAL_FIGHT
	"announce_general_sudden_death",		// AS_GENERAL_SUDDEN_DEATH
	"announce_general_vote_failed",			// AS_GENERAL_VOTE_FAILED
	"announce_general_vote_passed",			// AS_GENERAL_VOTE_PASSED
	"announce_general_vote_now",			// AS_GENERAL_VOTE_NOW
	"announce_general_one_frag",			// AS_GENERAL_ONE_FRAG
	"announce_general_two_frags",			// AS_GENERAL_TWO_FRAGS
	"announce_general_three_frags",			// AS_GENERAL_THREE_FRAGS
	"announce_general_one_minute",			// AS_GENERAL_ONE_MINUTE
	"announce_general_five_minute",			// AS_GENERAL_FIVE_MINUTE
	"announce_general_prepare_to_fight",	// AS_GENERAL_PREPARE_TO_FIGHT
	"announce_general_quad_damage",			// AS_GENERAL_QUAD_DAMAGE
	"announce_general_regeneration",		// AS_GENERAL_REGENERATION
	"announce_general_haste",				// AS_GENERAL_HASTE
	"announce_general_invisibility",		// AS_GENERAL_INVISIBILITY
	// DM announcements
	"announce_dm_you_tied_lead",			// AS_DM_YOU_TIED_LEAD
	"announce_dm_you_have_taken_lead",		// AS_DM_YOU_HAVE_TAKEN_LEAD
	"announce_dm_you_lost_lead",			// AS_DM_YOU_LOST_LEAD
    // Team announcements
	"announce_team_enemy_score",			// AS_TEAM_ENEMY_SCORES
	"announce_team_you_score",				// AS_TEAM_YOU_SCORE
	"announce_team_teams_tied",				// AS_TEAM_TEAMS_TIED
	"announce_team_strogg_lead",			// AS_TEAM_STROGG_LEAD
	"announce_team_marines_lead",			// AS_TEAM_MARINES_LEAD
	"announce_team_join_marine",			// AS_TEAM_JOIN_MARINE
	"announce_team_join_strogg",			// AS_TEAM_JOIN_STROGG
	// CTF announcements
	"announce_ctf_you_have_flag",			// AS_CTF_YOU_HAVE_FLAG
	"announce_ctf_your_team_has_flag",		// AS_CTF_YOUR_TEAM_HAS_FLAG
	"announce_ctf_enemy_has_flag",			// AS_CTF_ENEMY_HAS_FLAG
	"announce_ctf_your_team_drops_flag",	// AS_CTF_YOUR_TEAM_DROPS_FLAG
	"announce_ctf_enemy_drops_flag",		// AS_CTF_ENEMY_DROPS_FLAG
	"announce_ctf_your_flag_returned",		// AS_CTF_YOUR_FLAG_RETURNED
	"announce_ctf_enemy_returns_flag",		// AS_CTF_ENEMY_RETURNS_FLAG
	// Tourney announcements
	"announce_tourney_advance",				// AS_TOURNEY_ADVANCE
	"announce_tourney_join_arena_one",		// AS_TOURNEY_JOIN_ARENA_ONE
	"announce_tourney_join_arena_two",		// AS_TOURNEY_JOIN_ARENA_TWO
	"announce_tourney_join_arena_three",	// AS_TOURNEY_JOIN_ARENA_THREE
	"announce_tourney_join_arena_four",		// AS_TOURNEY_JOIN_ARENA_FOUR
	"announce_tourney_join_arena_five",		// AS_TOURNEY_JOIN_ARENA_FIVE
	"announce_tourney_join_arena_six",		// AS_TOURNEY_JOIN_ARENA_SIX
	"announce_tourney_join_arena_seven",	// AS_TOURNEY_JOIN_ARENA_SEVEN
	"announce_tourney_join_arena_eight",	// AS_TOURNEY_JOIN_ARENA_EIGHT
	"announce_tourney_join_arena_waiting",	// AS_TOURNEY_JOIN_ARENA_WAITING
	"announce_tourney_done",				// AS_TOURNEY_DONE
	"announce_tourney_start",				// AS_TOURNEY_START
	"announce_tourney_eliminated",			// AS_TOURNEY_ELIMINATED
	"announce_tourney_won",					// AS_TOURNEY_WON
	"announce_tourney_prelims",				// AS_TOURNEY_PRELIMS
	"announce_tourney_quarter_finals",		// AS_TOURNEY_QUARTER_FINALS
	"announce_tourney_semi_finals",			// AS_TOURNEY_SEMI_FINALS
	"announce_tourney_final_match",			// AS_TOURNEY_FINAL_MATCH
	"sound/vo/mp/9_99_320_10",				// AS_GENERAL_TEAM_AMMOREGEN
	"sound/vo/mp/9_99_360_6"				// AS_GENERAL_TEAM_DOUBLER
};

void idMultiplayerGame::ScheduleAnnouncerSound( announcerSound_t sound, float time, int instance, bool allowOverride ) {
	if( !gameLocal.GetLocalPlayer() ) {
		return;
	}

	if ( time < gameLocal.time ) {
		return;
	}

	if ( sound >= AS_NUM_SOUNDS ) {
		return;
	}
	
	announcerSoundNode_t* newSound = new announcerSoundNode_t;
	newSound->soundShader = sound;
	newSound->time = time;
	newSound->announcerSoundNode.SetOwner( newSound );
	newSound->instance = instance;
	newSound->allowOverride = allowOverride;

	announcerSoundNode_t* snd = NULL;
	for ( snd = announcerSoundQueue.Next(); snd != NULL; snd = snd->announcerSoundNode.Next() ) {
		if ( snd->time > newSound->time ) {
			newSound->announcerSoundNode.InsertBefore( snd->announcerSoundNode );
			break;
		}
	}
	if ( snd == NULL ) {
 		newSound->announcerSoundNode.AddToEnd( announcerSoundQueue );
	}
}

void idMultiplayerGame::RemoveAnnouncerSound( int type ) {
	// clean out any preexisting announcer sounds
	announcerSoundNode_t* snd = NULL;	
	announcerSoundNode_t* nextSnd = NULL;	
	for ( snd = announcerSoundQueue.Next(); snd != NULL; snd = nextSnd ) {
		nextSnd = snd->announcerSoundNode.Next();
		if ( snd->soundShader == type ) {
			snd->announcerSoundNode.Remove( );
			delete snd;
			break;
		}
	}

	// if a sound is currently playing, stop it
	if( gameLocal.GetLocalPlayer() && lastAnnouncerSound == type ) {
		gameLocal.GetLocalPlayer()->StopSound( SND_CHANNEL_MP_ANNOUNCER, false );
		lastAnnouncerSound = AS_NUM_SOUNDS;
	}
}

void idMultiplayerGame::RemoveAnnouncerSoundRange( int startType, int endType ) {
	// clean out any preexisting announcer sounds
	announcerSoundNode_t* snd = NULL;	
	announcerSoundNode_t* nextSnd = NULL;	
	for ( snd = announcerSoundQueue.Next(); snd != NULL; snd = nextSnd ) {
		nextSnd = snd->announcerSoundNode.Next();
		for( int i = startType; i <= endType; i++ ) {
			if ( snd->soundShader == i ) {
				snd->announcerSoundNode.Remove( );
				delete snd;
			}
		}
	}

	// if a sound is currently playing, stop it
	if ( gameLocal.GetLocalPlayer() ) {
		for( int i = startType; i <= endType; i++ ) {
			if( lastAnnouncerSound == i ) {
				gameLocal.GetLocalPlayer()->StopSound( SND_CHANNEL_MP_ANNOUNCER, false );
				lastAnnouncerSound = AS_NUM_SOUNDS;
				break;
			}
		}
	}
}


void idMultiplayerGame::ScheduleTimeAnnouncements( void ) {
	if( !gameLocal.GetLocalPlayer() || !gameState ) {
		// too early
		return;
	}

	// clean out any preexisting announcer sounds
	RemoveAnnouncerSound( AS_GENERAL_ONE_MINUTE );
	RemoveAnnouncerSound( AS_GENERAL_FIVE_MINUTE );

	if( gameState->GetMPGameState() != COUNTDOWN && gameState->GetMPGameState() != WARMUP ) {
		int timeLimit = gameLocal.serverInfo.GetInt( "si_timeLimit" );
		int endGameTime = 0;

		if( gameLocal.gameType == GAME_TOURNEY ) {
			int arena = gameLocal.GetLocalPlayer()->GetArena();
			if( !((rvTourneyGameState*)gameState)->GetArena( arena ).IsPlaying() ) {
				return; // arena is not active
			}
			// per-arena timelimits
			endGameTime = ((rvTourneyGameState*)gameState)->GetArena( arena ).GetMatchStartTime() + ( timeLimit * 60000 );
		} else {
			endGameTime = matchStartedTime + ( timeLimit * 60000 );
		}

		if( timeLimit > 5 ) {
			ScheduleAnnouncerSound( AS_GENERAL_FIVE_MINUTE, endGameTime - (5 * 60000) );
		}
		if( timeLimit > 1 ) {
			ScheduleAnnouncerSound( AS_GENERAL_ONE_MINUTE, endGameTime - (60000) );
		}
	}
}

void idMultiplayerGame::PlayAnnouncerSounds( void ) {
	announcerSoundNode_t* snd = NULL;	
	announcerSoundNode_t* nextSnd = NULL;	

	if( !gameLocal.GetLocalPlayer() ) {
		return;
	}

	// if we're done playing the last sound reset override status
	if( announcerPlayTime <= gameLocal.time ) {
		currentSoundOverride = false;
	}

	if ( announcerPlayTime > gameLocal.time && !currentSoundOverride ) {
		return;
	} 

	// in tourney only play sounds scheduled for your current arena
	if ( gameLocal.gameType == GAME_TOURNEY ) {
		// go through and find the first sound to play in our arena, delete any sounds
		// for other arenas we see along the way.
		for ( snd = announcerSoundQueue.Next(); snd != NULL; snd = nextSnd ) {
			nextSnd = snd->announcerSoundNode.Next();

			if( snd->time > gameLocal.time ) {
				return;
			}

			if( snd->instance == -1 || snd->soundShader == AS_GENERAL_VOTE_NOW || snd->soundShader == AS_GENERAL_VOTE_PASSED || snd->soundShader == AS_GENERAL_VOTE_FAILED ) {
				// all-instance sound
				break;
			}

			if( snd->instance == gameLocal.GetLocalPlayer()->GetInstance() ) {
				if( snd->allowOverride && nextSnd && nextSnd->time <= gameLocal.time ) {
					// this sound is OK with being over-ridden, 
					// and the next sound is ready to play, so go ahead and look at the next sound
					snd->announcerSoundNode.Remove ( );
					delete snd;

					continue;
				} else {
					break;
				}
			}

			snd->announcerSoundNode.Remove ( );
			delete snd;
		}
	} else {
		snd = announcerSoundQueue.Next();
		if( snd && snd->time > gameLocal.time ) {
			return;
		}
	}

    // play the sound locally
	if ( snd && snd->soundShader < AS_NUM_SOUNDS ) {
		int length = 0;

		//don't play timelimit countdown announcements if game is already over
		mpGameState_t state = gameState->GetMPGameState();
		if ( state == GAMEREVIEW //game is over, in scoreboard
			&& ( snd->soundShader == AS_GENERAL_ONE_MINUTE
				|| snd->soundShader == AS_GENERAL_FIVE_MINUTE ) ) {
			//ignore scheduled time limit warnings that haven't executed yet
			snd->announcerSoundNode.Remove();
			delete snd;
		} else {
			snd->announcerSoundNode.Remove();

			gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( announcerSoundDefs[ snd->soundShader ], false ), SND_CHANNEL_MP_ANNOUNCER, 0, false, &length );
			currentSoundOverride = snd->allowOverride;
			lastAnnouncerSound = snd->soundShader;

			delete snd;
		}

		// if sounds remain to be played, check again	
		announcerPlayTime = gameLocal.time + length;
	} 
}

void idMultiplayerGame::ClearTeamScores ( void ) {
	for ( int i = 0; i < TEAM_MAX; i++ ) {
		teamScore[ i ] = 0;
		teamDeadZoneScore[i] = 0;
	}
}

void idMultiplayerGame::AddTeamScore ( int team, int amount ) {
	if ( team < 0 || team >= TEAM_MAX ) {
		return;
	}

	teamScore[ team ] += amount;
}

void idMultiplayerGame::AddPlayerScore( idPlayer* player, int amount ) {
	if( player == NULL ) {
		gameLocal.Warning( "idMultiplayerGame::AddPlayerScore() - NULL player specified" );
		return;
	}

	if( player->entityNumber < 0 || player->entityNumber >= MAX_CLIENTS ) {
		gameLocal.Warning( "idMultiplayerGame::AddPlayerScore() - Bad player entityNumber '%d'\n", player->entityNumber );
		return;
	}

	playerState[ player->entityNumber ].fragCount += amount;
	playerState[ player->entityNumber ].fragCount = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[ player->entityNumber ].fragCount );
}

void idMultiplayerGame::AddPlayerTeamScore( idPlayer* player, int amount ) {
	if( player == NULL ) {
		gameLocal.Warning( "idMultiplayerGame::AddPlayerTeamScore() - NULL player specified" );
		return;
	}

	if( player->entityNumber < 0 || player->entityNumber >= MAX_CLIENTS ) {
		gameLocal.Warning( "idMultiplayerGame::AddPlayerTeamScore() - Bad player entityNumber '%d'\n", player->entityNumber );
		return;
	}

	playerState[ player->entityNumber ].teamFragCount += amount;
	playerState[ player->entityNumber ].teamFragCount = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, playerState[ player->entityNumber ].teamFragCount );
}

void idMultiplayerGame::AddPlayerWin( idPlayer* player, int amount ) {
	if( player == NULL ) {
		gameLocal.Warning( "idMultiplayerGame::AddPlayerWin() - NULL player specified" );
		return;
	}

	if( player->entityNumber < 0 || player->entityNumber >= MAX_CLIENTS ) {
		gameLocal.Warning( "idMultiplayerGame::AddPlayerWin() - Bad player entityNumber '%d'\n", player->entityNumber );
		return;
	}

	playerState[ player->entityNumber ].wins += amount;
	playerState[ player->entityNumber ].wins = idMath::ClampInt( 0, MP_PLAYER_MAXWINS, playerState[ player->entityNumber ].wins );
}

void idMultiplayerGame::SetPlayerScore( idPlayer* player, int value ) {
	if( player == NULL ) {
		gameLocal.Warning( "idMultiplayerGame::SetPlayerScore() - NULL player specified" );
		return;
	}

	if( player->entityNumber < 0 || player->entityNumber >= MAX_CLIENTS ) {
		gameLocal.Warning( "idMultiplayerGame::SetPlayerScore() - Bad player entityNumber '%d'\n", player->entityNumber );
		return;
	}

	playerState[ player->entityNumber ].fragCount = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, value );
	
}

void idMultiplayerGame::SetPlayerTeamScore( idPlayer* player, int value ) {
	if( player == NULL ) {
		gameLocal.Warning( "idMultiplayerGame::SetPlayerTeamScore() - NULL player specified" );
		return;
	}

	if( player->entityNumber < 0 || player->entityNumber >= MAX_CLIENTS ) {
		gameLocal.Warning( "idMultiplayerGame::SetPlayerTeamScore() - Bad player entityNumber '%d'\n", player->entityNumber );
		return;
	}

	playerState[ player->entityNumber ].teamFragCount = idMath::ClampInt( MP_PLAYER_MINFRAGS, MP_PLAYER_MAXFRAGS, value );
}

void idMultiplayerGame::SetPlayerDeadZoneScore( idPlayer* player, float value ) {
	if( player == NULL ) {
		gameLocal.Warning( "idMultiplayerGame::SetPlayerDeadZoneScore() - NULL player specified" );
		return;
	}

	if( player->entityNumber < 0 || player->entityNumber >= MAX_CLIENTS ) {
		gameLocal.Warning( "idMultiplayerGame::SetPlayerDeadZoneScore() - Bad player entityNumber '%d'\n", player->entityNumber );
		return;
	}

	playerState[ player->entityNumber ].deadZoneScore = value;
}

void idMultiplayerGame::SetPlayerWin( idPlayer* player, int value ) {
	if( player == NULL ) {
		gameLocal.Warning( "idMultiplayerGame::SetPlayerWin() - NULL player specified" );
		return;
	}

	if( player->entityNumber < 0 || player->entityNumber >= MAX_CLIENTS ) {
		gameLocal.Warning( "idMultiplayerGame::SetPlayerWin() - Bad player entityNumber '%d'\n", player->entityNumber );
		return;
	}

	playerState[ player->entityNumber ].wins = idMath::ClampInt( 0, MP_PLAYER_MAXWINS, value );
}

rvCTF_AssaultPoint* idMultiplayerGame::NextAP( int team ) {
	for( int i = 0; i < assaultPoints.Num(); i++ ) {
		if( assaultPoints[ (team ? (assaultPoints.Num() - 1 - i) : i) ]->GetOwner() == team ) {
			continue;
		}
		return assaultPoints[ (team ? (assaultPoints.Num() - 1 - i) : i) ];
	}
	return NULL;
}

void idMultiplayerGame::ClientSetInstance( const idBitMsg& msg ) {
	idPlayer* player = gameLocal.GetLocalPlayer();

	int instance = msg.ReadByte();

	if ( !player ) {
		gameLocal.Warning( "idMultiplayerGame::ClientSetInstance - NULL local player" );
		return;
	}

	gameLocal.GetInstance( 0 )->SetSpawnInstanceID( instance );
	// on the client, we delete all entities, 
	// the server will send over new ones
	gameLocal.InstanceClear();
	// set the starting offset for repopulation back to matching what the server will have
	// this should be covered by setting indexes when populating the instances as well, but it doesn't hurt
	gameLocal.firstFreeIndex = MAX_CLIENTS;

	player->SetArena( instance );
	player->SetInstance( instance );

	// spawn the instance entities
	gameLocal.GetInstance( 0 )->PopulateFromMessage( msg );

	// players in other instances might have been hidden, update them
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* p = (idPlayer*)gameLocal.entities[ i ];
		if( p ) {
			if( p->GetInstance() == instance ) {
				p->ClientInstanceJoin();
			} else {
				p->ClientInstanceLeave();
			}
		}
	}
}

void idMultiplayerGame::ServerSetInstance( int instance ) {
	for( int i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {
		idEntity* ent = gameLocal.entities[ i ];
		if( ent ) {
			if( ent->GetInstance() != instance ) {
				ent->InstanceLeave();
			} else {
				ent->InstanceJoin();
			}
		}
	}
}

const char* idMultiplayerGame::GetLongGametypeName( const char* gametype ) {
	if( !idStr::Icmp( gametype, "Tourney" ) ) {
		return common->GetLocalizedString( "#str_107676" );
	} else if( !idStr::Icmp( gametype, "Team DM" ) ) {
		return common->GetLocalizedString( "#str_107677" );
	} else if( !idStr::Icmp( gametype, "CTF" ) ) {
		return common->GetLocalizedString( "#str_107678" );
	} else if( !idStr::Icmp( gametype, "DM" ) ) {
		return common->GetLocalizedString( "#str_107679" );
	} else if( !idStr::Icmp( gametype, "One Flag CTF" ) ) {
		return common->GetLocalizedString( "#str_107680" );
	} else if( !idStr::Icmp( gametype, "Arena CTF" ) ) {
		return common->GetLocalizedString( "#str_107681" );
	} else if( !idStr::Icmp( gametype, "Arena One Flag CTF" ) ) {
		return common->GetLocalizedString( "#str_107682" );
	// RITUAL BEGIN
    // squirrel: added DeadZone multiplayer mode
	} else if( !idStr::Icmp( gametype, "DeadZone" ) ) {
		return common->GetLocalizedString( "#str_122001" ); // Squirrel@Ritual - Localized for 1.2 Patch
    // RITUAL END
	}

	return "";
}

/*
================
idMultiplayerGame::VoteGameTypeToString
================
*/
const char *idMultiplayerGame::VoteGameTypeToString( int gameTypeInt ) {
	const char *gameType = NULL;
	switch ( gameTypeInt ) {
		default:
		case VOTE_GAMETYPE_DM:
			gameType = "DM";
			break;
		case VOTE_GAMETYPE_TOURNEY:
			gameType = "Tourney";
			break;
		case VOTE_GAMETYPE_TDM:
			gameType = "Team DM";
			break;
		case VOTE_GAMETYPE_CTF:
			gameType = "CTF";
			break;
		case VOTE_GAMETYPE_ARENA_CTF:
			gameType = "Arena CTF";
			break;
		case VOTE_GAMETYPE_DEADZONE:
			gameType = "DeadZone";
			break;
	}
	return gameType;
}

int	idMultiplayerGame::GameTypeToVote( const char *gameType ) {
	if ( 0 == idStr::Icmp( gameType, "DM" ) ) { 
		return VOTE_GAMETYPE_DM;
	} else if ( 0 == idStr::Icmp( gameType, "Tourney" ) ) {
		return VOTE_GAMETYPE_TOURNEY;
	} else if ( 0 == idStr::Icmp( gameType, "Team DM" ) ) {
		return VOTE_GAMETYPE_TDM;
	} else if ( 0 == idStr::Icmp( gameType, "CTF" ) ) {
		return VOTE_GAMETYPE_CTF;
	} else if ( 0 == idStr::Icmp( gameType, "Arena CTF" ) ) {
		return VOTE_GAMETYPE_ARENA_CTF;
	} else if ( 0 == idStr::Icmp( gameType, "DeadZone" ) ) {
		return VOTE_GAMETYPE_DEADZONE;
	}

	return VOTE_GAMETYPE_DM;
}

float idMultiplayerGame::GetPlayerDeadZoneScore( idPlayer* player ) {
	return playerState[ player->entityNumber ].deadZoneScore;
}

int idMultiplayerGame::GetPlayerTime( idPlayer* player ) {
	return ( gameLocal.time - player->GetConnectTime() ) / 60000;
}

int idMultiplayerGame::GetTeamScore( idPlayer* player ) {
	return GetTeamScore( player->entityNumber );
}

int idMultiplayerGame::GetScore( idPlayer* player ) {
	return GetScore( player->entityNumber );
}

int idMultiplayerGame::GetWins( idPlayer* player ) {
	return GetWins( player->entityNumber );
}

void idMultiplayerGame::EnableDamage( bool enable ) {
	for( int i = 0; i < gameLocal.numClients; i++ ) {
		idPlayer* player = (idPlayer*)gameLocal.entities[ i ];

		if( player == NULL ) {
			continue;
		}

		player->fl.takedamage = enable;
	}
}

void idMultiplayerGame::ReceiveRemoteConsoleOutput( const char* output ) {
	if( mainGui ) {
		idStr newOutput( output );

		if( rconHistory.Length() + newOutput.Length() > RCON_HISTORY_SIZE ) {
			int removeLength = rconHistory.Find( '\n' );
			if( removeLength == -1 ) {
				// nuke the whole string
				rconHistory.Empty();
			} else {
				while( (rconHistory.Length() - removeLength) + newOutput.Length() > RCON_HISTORY_SIZE ) {
					removeLength = rconHistory.Find( '\n', removeLength + 1 );
					if( removeLength == -1 ) {
						rconHistory.Empty();
						break;
					}
				}
			}
			rconHistory = rconHistory.Right( rconHistory.Length() - removeLength );
		}


		int consoleInputStart = newOutput.Find( "Console Input: " );
		if( consoleInputStart != -1 ) {
			idStr consoleInput = newOutput.Right( newOutput.Length() - consoleInputStart - 15 );
			newOutput = newOutput.Left( consoleInputStart );
			newOutput.StripTrailing( "\n" );
			consoleInput.StripTrailing( "\n" );
			mainGui->SetStateString( "admin_console_input", consoleInput.c_str() );
		} 

		if( newOutput.Length() ) {
			rconHistory.Append( newOutput );
			rconHistory.Append( '\n' );
		}

		mainGui->SetStateString( "admin_console_history", rconHistory.c_str() );
	}
}

/*
===============
idMultiplayerGame::ShuffleTeams
===============
*/
void idMultiplayerGame::ShuffleTeams( void ) {
	// turn off autobalance if its on
	bool autoBalance = gameLocal.serverInfo.GetBool( "si_autoBalance" );
	if( autoBalance ) {
		gameLocal.serverInfo.SetBool( "si_autoBalance", false );
	}
	
	int loosingTeam = teamScore[ TEAM_MARINE ] < teamScore[ TEAM_STROGG ] ? TEAM_MARINE : TEAM_STROGG;
	int winningTeam = loosingTeam == TEAM_MARINE ? TEAM_STROGG : TEAM_MARINE;

	for( int i = 0; i < rankedPlayers.Num(); i++ ) {
		if( !(i % 2) ) {
			// switch even players to losing team
			if( rankedPlayers[ i ].First()->team != loosingTeam ) {
				rankedPlayers[ i ].First()->GetUserInfo()->Set( "ui_team", teamNames[ loosingTeam ] );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "updateUI %d\n", rankedPlayers[ i ].First()->entityNumber ) );
			}
		} else {
			if( rankedPlayers[ i ].First()->team != winningTeam ) {
				rankedPlayers[ i ].First()->GetUserInfo()->Set( "ui_team", teamNames[ winningTeam ] );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "updateUI %d\n", rankedPlayers[ i ].First()->entityNumber ) );
			}
		}
	}

	if( autoBalance ) {
		gameLocal.serverInfo.SetBool( "si_autoBalance", true );
	}
}


rvGameState* idMultiplayerGame::GetGameState( void ) { 
	return gameState; 
}

void idMultiplayerGame::SetGameType( void ) {
	if ( gameState != NULL ) {
		delete gameState;
		gameState = NULL;
	}

	if ( ( idStr::Icmp( gameLocal.serverInfo.GetString( "si_gameType" ), "DM" ) == 0 ) ) {
		gameLocal.gameType = GAME_DM;
		gameState = new rvDMGameState();
	} else if ( ( idStr::Icmp( gameLocal.serverInfo.GetString( "si_gameType" ), "Tourney" ) == 0 ) ) {
		gameLocal.gameType = GAME_TOURNEY;
		gameState = new rvTourneyGameState();
	} else if ( ( idStr::Icmp( gameLocal.serverInfo.GetString( "si_gameType" ), "Team DM" ) == 0 ) ) {
		gameLocal.gameType = GAME_TDM;
		gameState = new rvTeamDMGameState();
	} else if ( ( idStr::Icmp( gameLocal.serverInfo.GetString( "si_gameType" ), "CTF" ) == 0 ) ) {
		gameLocal.gameType = GAME_CTF;
		gameState = new rvCTFGameState();
	} else if ( ( idStr::Icmp( gameLocal.serverInfo.GetString( "si_gameType" ), "One Flag CTF" ) == 0 ) ) {
		gameLocal.gameType = GAME_1F_CTF;
		gameState = new rvCTFGameState();
	} else if ( ( idStr::Icmp( gameLocal.serverInfo.GetString( "si_gameType" ), "Arena CTF" ) == 0 ) ) {
		gameLocal.gameType = GAME_ARENA_CTF;
		gameState = new rvCTFGameState();
	} else if ( ( idStr::Icmp( gameLocal.serverInfo.GetString( "si_gameType" ), "Arena One Flag CTF" ) == 0 ) ) {
		gameLocal.gameType = GAME_ARENA_1F_CTF;
		gameState = new rvCTFGameState();
	} else if ( ( idStr::Icmp( gameLocal.serverInfo.GetString( "si_gameType" ), "DeadZone" ) == 0 ) ) {
		gameLocal.gameType = GAME_DEADZONE;
		gameState = new riDZGameState;
	} else {
		gameLocal.Error( "idMultiplayerGame::SetGameType() - Unknown gametype '%s'\n", gameLocal.serverInfo.GetString( "si_gameType" ) );
	}

	// force entity filter to gametype name in multiplayer
	if ( gameLocal.gameType != GAME_SP ) {
		gameLocal.serverInfo.Set( "si_entityFilter", gameLocal.serverInfo.GetString( "si_gameType" ) );
		// also set as a CVar for when serverinfo is rescanned
		cvarSystem->SetCVarString( "si_entityFilter", gameLocal.serverInfo.GetString( "si_gameType" ) );
	}
}

//asalmon: need to access total frags for a team and total score for a team
int idMultiplayerGame::GetTeamsTotalFrags( int i ) {
	if( i < 0 || i > TEAM_MAX ) {
		return 0;
	}
	int total = 0;
	for(int j=0; j <  GetNumRankedPlayers(); j++)
	{
		if(rankedPlayers[ j ].First()->team == i)
		{
			total += GetScore(rankedPlayers[ j ].First()->entityNumber);
		}
	}

	return total;

}

int idMultiplayerGame::GetTeamsTotalScore( int i ) {
	if( i < 0 || i > TEAM_MAX ) {
		return 0;
	}
	int total = 0; 
	for(int j=0; j <  GetNumRankedPlayers(); j++)
	{
		idPlayer foo;
		
		if(rankedPlayers[ j ].First()->team == i)
		{
			total += GetTeamScore(rankedPlayers[ j ].First()->entityNumber);
		}
	}

	return total;

}

/*
===============
idMultiplayerGame::PickMap
===============
*/
bool idMultiplayerGame::PickMap( idStr gameType, bool checkOnly ) {
	
	idStrList maps;
	int miss = 0;
	const idDict *mapDict;
	int index = 0;
	int btype;
	const char* mapName;

	mapName = si_map.GetString();

	// if we didn't set up a gametype, grab the current game type.
	if ( gameType.IsEmpty() )	{
		gameType = si_gameType.GetString();
	}

	// if we're playing a map of this gametype, don't change.
	mapDict = fileSystem->GetMapDecl( mapName );
	if ( mapDict ) {
		btype = mapDict->GetInt( gameType );
		if ( btype ) {
			// ( not sure what the gloubi boulga is about re-setting si_map two ways after reading it at the start of the function already )
			cvarSystem->SetCVarString( "si_map", mapName );
			si_map.SetString( mapName );
			return false;			
		}
	}

	if ( checkOnly ) {
		// always allow switching to DM mode, whatever the settings on the map ( DM should always be possible )
		if ( !idStr::Icmp( si_gameType.GetString(), "DM" ) ) {
			return false;
		}
		// don't actually change anything, indicate we would
		return true;
	}

	int i;
	idFileList *files;
	idStrList fileList;
	
	int count = 0;

	files = fileSystem->ListFiles( "maps/mp", ".map" );
	for ( i = 0; i < files->GetList().Num(); i++, count++ ) {
		fileList.AddUnique( va( "mp/%s", files->GetList()[i].c_str() ) );
	}
	fileSystem->FreeFileList( files );

	files = fileSystem->ListFiles( "maps/mp", ".mapc" );
	for ( i = 0; i < files->GetList().Num(); i++, count++ ) {
		idStr fixedExtension(files->GetList()[i]);
		fixedExtension.SetFileExtension("map");
		fileList.AddUnique( va( "mp/%s", fixedExtension.c_str() ) );
	}

	fileList.Sort();

	idStr name;
	idStr cycle;

	//Populate the map list
	for ( i = 0; i < fileList.Num(); i++) {
		//Add only MP maps.
		if(!idStr::FindText(fileList[i].c_str(), "mp/"))
		{
			maps.AddUnique(fileList[i].c_str());
		}
	}
	maps.Sort();

	if(maps.Num() > 0)
	{
		while(miss < 100)
		{
			index = gameLocal.random.RandomInt( maps.Num() );
			mapName = maps[index].c_str();
			
			mapDict = fileSystem->GetMapDecl( mapName );
			if ( mapDict ) {
				btype = mapDict->GetInt( gameType );
				if(btype)
				{					
					cvarSystem->SetCVarString("si_map",mapName);
					si_map.SetString( mapName );
					return true;
					
				}
			}
			miss++;
		
		}
	
	}

	//something is wrong and there are no maps for this game type.  This should never happen.
	gameLocal.Error( "No maps found for game type: %s.\n", gameType.c_str() );
	return false;
}

/*
===============
idMultiplayerGame::GetPlayerRankText
===============
*/
char* idMultiplayerGame::GetPlayerRankText( int rank, bool tied, int score ) {
	char* placeString;

	if( rank == 0 ) {
		//"1st^0 place with"
		placeString = va( "%s%s %d", S_COLOR_BLUE, common->GetLocalizedString( "#str_107689" ), score );
	} else if( rank == 1 ) {
		//"2nd^0 place with"
		placeString = va( "%s%s %d", S_COLOR_RED, common->GetLocalizedString(  "#str_107690" ), score );
	} else if( rank == 2 ) {
		//"3rd^0 place with"
		placeString = va( "%s%s %d", S_COLOR_YELLOW, common->GetLocalizedString( "#str_107691" ), score );
	} else {
		//"th^0 place with"
		placeString = va( "%d%s %d", rank + 1, common->GetLocalizedString( "#str_107692" ), score );
	}

	if( tied ) {
		//Tied for
		return va( "%s %s", common->GetLocalizedString( "#str_107693" ), placeString );
	} else {
		return placeString;
	}
}

/*
===============
idMultiplayerGame::GetPlayerRankText
===============
*/
char* idMultiplayerGame::GetPlayerRankText( idPlayer* player ) {
	if( player == NULL ) {
		return "";
	}

	bool tied = false;
	int rank = GetPlayerRank( player, tied );
	return GetPlayerRankText( rank, tied, GetScore( player ) );
}

/*
===============
idMultiplayerGame::WriteNetworkInfo
===============
*/
void idMultiplayerGame::WriteNetworkInfo( idFile *file, int clientNum ) {
	idBitMsg	msg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];
	
	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.BeginWriting();
	WriteStartState( clientNum, msg, true );
	file->WriteInt( msg.GetSize() );
	file->Write( msg.GetData(), msg.GetSize() );

	gameState->WriteNetworkInfo( file, clientNum );
}

/*
===============
idMultiplayerGame::ReadNetworkInfo
===============
*/
void idMultiplayerGame::ReadNetworkInfo( idFile* file, int clientNum ) {
	idBitMsg	msg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];
	int			size;

	file->ReadInt( size );
	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.SetSize( size );
	file->Read( msg.GetData(), size );
	ClientReadStartState( msg );

	gameState->ReadNetworkInfo( file, clientNum );
}

void idMultiplayerGame::AddPrivatePlayer( int clientId ) {
	privateClientIds.Append( clientId );
}

void idMultiplayerGame::RemovePrivatePlayer( int clientId ) {
	for( int i = 0; i < privateClientIds.Num(); i++ ) {
		if( clientId == privateClientIds[ i ] ) {
			privateClientIds.RemoveIndex( i );
			i--;
		}
	}
}

void idMultiplayerGame::UpdatePrivatePlayerCount( void ) {
	if ( !gameLocal.isServer ) {
		return;
	}

	int numPrivatePlayers = 0;
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if( privatePlayers & (1 << i) ) {
			if( gameLocal.entities[ i ] ) {
				numPrivatePlayers++;
			} else {
				privatePlayers &= ~( 1 << i );
			}
		}
	}

	cvarSystem->SetCVarInteger( "si_numPrivatePlayers", numPrivatePlayers );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" " " __FILE__ " " __LINESTR__ );
}

void idMultiplayerGame::SetFlagEntity( idEntity* ent, int team ) {
	assert( ( team == TEAM_STROGG || team == TEAM_MARINE ) );

	flagEntities[ team ] = ent;
}

idEntity* idMultiplayerGame::GetFlagEntity( int team ) {
	assert( team >= 0  && team < TEAM_MAX );

	return flagEntities[ team ];
}


// <team to switch to> <named event for yes> <named event for no> <named event for same team>
void idMultiplayerGame::CheckTeamBalance_f( const idCmdArgs &args ) {
	
	if ( args.Argc() < 5 ) {
		return;
	}
	
	idPlayer *localPlayer = gameLocal.GetLocalPlayer();
	
	const char *team = args.Argv(1);
	const char *yesEvent = args.Argv(2);
	const char *noEvent = args.Argv(3);
	const char *sameTeamEvent = args.Argv(4);
	
	if ( !gameLocal.serverInfo.GetBool( "si_autoBalance" ) || !gameLocal.IsTeamGame() ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va("GuiEvent %s", yesEvent) );
		return;
	}
	
	int teamCount[2];
	teamCount[0] = teamCount[1] = 0;
	
	for ( int i = 0; i < gameLocal.numClients; ++i ) {
		idEntity *ent = gameLocal.entities[i];

		if ( ent && ent->IsType( idPlayer::GetClassType() ) && gameLocal.mpGame.IsInGame( i ) ) {
			if ( !static_cast< idPlayer * >( ent )->spectating && ent != localPlayer ) {
				teamCount[ static_cast< idPlayer * >( ent )->team ]++;
			}
		}
	}
	
	if ( idStr::Icmp( team, "marine" ) == 0 ) {
		if ( localPlayer->team == TEAM_MARINE && !localPlayer->spectating ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va("GuiEvent %s", sameTeamEvent) );
		} else {
			if ( teamCount[TEAM_MARINE] > teamCount[TEAM_STROGG] ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, va("GuiEvent %s", noEvent) );
			} else {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, va("GuiEvent %s", yesEvent) );
			}
		}
	} else {
		
		if ( localPlayer->team == TEAM_STROGG && !localPlayer->spectating ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va("GuiEvent %s", sameTeamEvent) );
		} else {
			if ( teamCount[TEAM_STROGG] > teamCount[TEAM_MARINE] ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, va("GuiEvent %s", noEvent) );
			} else {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, va("GuiEvent %s", yesEvent) );
			}
		}
	}
}

/*
================
idMultiplayerGame::LocalizeGametype

dupe of rvServerScanGUI::LocalizeGametype
================
*/
const char *idMultiplayerGame::LocalizeGametype( void ) {

	const char	*gameType;
		
	gameType = gameLocal.serverInfo.GetString( "si_gametype" );
	localisedGametype = gameType;

	if( !idStr::Icmp( gameType, "DM" ) ) {
		localisedGametype = common->GetLocalizedString( "#str_110011" );
	}
	if( !idStr::Icmp( gameType, "Tourney" ) ) {
		localisedGametype = common->GetLocalizedString( "#str_110012" );
	}
	if( !idStr::Icmp( gameType, "Team DM" ) ) {
		localisedGametype = common->GetLocalizedString( "#str_110013" );
	}
	if( !idStr::Icmp( gameType, "CTF" ) ) {
		localisedGametype = common->GetLocalizedString( "#str_110014" );
	}
	if( !idStr::Icmp( gameType, "Arena CTF" ) ) {
		localisedGametype = common->GetLocalizedString( "#str_110015" );
	}
	if( !idStr::Icmp( gameType, "DeadZone" ) ) {
		localisedGametype = common->GetLocalizedString( "#str_122001" ); // Squirrel@Ritual - Localized for 1.2 Patch
	}

	return( localisedGametype.c_str() );
}

int idMultiplayerGame::VerifyTeamSwitch( int wantTeam, idPlayer *player ) {
	idEntity* ent;
	int teamCount[ TEAM_MAX ];
	int balanceTeam = -1;

	if( !gameLocal.serverInfo.GetBool( "si_autoBalance" ) ) {
		return wantTeam;
	}

	teamCount[ TEAM_MARINE ] = teamCount[ TEAM_STROGG ] = 0;

	for( int i = 0; i < gameLocal.numClients; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPlayer::GetClassType() ) && gameLocal.mpGame.IsInGame( i ) ) {
			if ( !static_cast< idPlayer * >( ent )->spectating && ent != player ) {
				teamCount[ static_cast< idPlayer * >( ent )->team ]++;
			}
		}
	}

	balanceTeam = -1;
	if ( teamCount[ TEAM_MARINE ] > teamCount[ TEAM_STROGG ] ) {
		balanceTeam = TEAM_STROGG;
	} else if ( teamCount[ TEAM_STROGG ] > teamCount[ TEAM_MARINE ] ) {
		balanceTeam = TEAM_MARINE;
	}

	return (balanceTeam == -1) ? wantTeam : balanceTeam;
}

// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
/*
================
idMultiplayerGame::NumberOfPlayersOnTeam
================
*/
int idMultiplayerGame::NumberOfPlayersOnTeam( int team )
{
	int teamPlayerCount = 0;

	for ( int i = 0; i < gameLocal.numClients; i++ )
	{
		idEntity *ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPlayer::GetClassType() ) )
		{
			idPlayer* entPlayer = static_cast< idPlayer * >( ent );
			if( entPlayer->team == team )
			{
				teamPlayerCount ++;
			}
		}
	}

	return teamPlayerCount;
}


/*
================
idMultiplayerGame::NumberOfAlivePlayersOnTeam
================
*/
int idMultiplayerGame::NumberOfAlivePlayersOnTeam( int team )
{
	int teamAlivePlayerCount = 0;

	for ( int i = 0; i < gameLocal.numClients; i++ )
	{
		idEntity *ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPlayer::GetClassType() ) )
		{
			idPlayer* entPlayer = static_cast< idPlayer * >( ent );
			if( entPlayer->team == team && entPlayer->allowedToRespawn )
			{
				teamAlivePlayerCount ++;
			}
		}
	}

	return teamAlivePlayerCount;
}


// RITUAL END


// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
/*
================
idMultiplayerGame::OpenLocalBuyMenu
================
*/
void idMultiplayerGame::OpenLocalBuyMenu( void )
{
	// Buy menu work in progress
	//if ( gameLocal.mpGame.GetCurrentMenu() == 4 )
	//{	
	//		return;
	//}

	if ( currentMenu == 4 )
		return; // Already open

	gameLocal.sessionCommand = "game_startmenu";
	gameLocal.mpGame.nextMenu = 4;
}

/*	
================
idMultiplayerGame::RedrawLocalBuyMenu
================
*/
void idMultiplayerGame::RedrawLocalBuyMenu( void )
{
	if ( !buyMenu )
		return;

	SetupBuyMenuItems();
	buyMenu->HandleNamedEvent( "update_buymenu" );
}


/*
================
idMultiplayerGame::GiveCashToTeam
================
*/
void idMultiplayerGame::GiveCashToTeam( int team, float cashAmount )
{
	for ( int i = 0; i < gameLocal.numClients; i++ )
	{
		idEntity *ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPlayer::GetClassType() ) )
		{
			idPlayer* entPlayer = static_cast< idPlayer * >( ent );
			if( entPlayer->team == team )
			{
				entPlayer->GiveCash( cashAmount );
			}
		}
	}

}


/*
================
idMultiplayerGame::IsBuyingAllowedInTheCurrentGameMode
================
*/
bool idMultiplayerGame::IsBuyingAllowedInTheCurrentGameMode( void ) {
	if ( !gameLocal.isMultiplayer ) {
		return false;
	}

	if ( gameLocal.gameType != GAME_TOURNEY ) {
		return gameLocal.serverInfo.GetBool( "si_isBuyingEnabled" );
	}

	return false;
}


/*
================
idMultiplayerGame::IsBuyingAllowedRightNow
================
*/
bool idMultiplayerGame::IsBuyingAllowedRightNow( void )
{
	return ( IsBuyingAllowedInTheCurrentGameMode() && isBuyingAllowedRightNow );
}


void idMultiplayerGame::AddTeamPowerup(int powerup, int time, int team)
{
	int i;
	for ( i=0; i<MAX_TEAM_POWERUPS; i++ )
	{
		if ( teamPowerups[team][i].powerup == powerup )
		{
			//teamPowerups[team][i].time = teamPowerups[team][i].endTime - gameLocal.time + time;
			//teamPowerups[team][i].endTime += time;
			// Just reset the time to it's maximum.  This effectively caps the time
			// from accumulating infinitely if the players are very wealthy.
			teamPowerups[team][i].endTime = gameLocal.time + time;
			teamPowerups[team][i].time = time;
			teamPowerups[team][i].update = true;
			return;
		}
	}

	// If we get here, the powerup wasn't previously active, so find the first
	// empty slot available and activate the powerup
	for ( i=0; i<MAX_TEAM_POWERUPS; i++ )
	{
		if ( teamPowerups[team][i].powerup == 0 )
		{
			teamPowerups[team][i].powerup = powerup;
			teamPowerups[team][i].endTime = gameLocal.time + time;
			teamPowerups[team][i].time = time;
			teamPowerups[team][i].update = true;
			return;
		}
	}
}

void idMultiplayerGame::UpdateTeamPowerups( void ) {
	int i,j;
	for ( i=0; i<TEAM_MAX; i++ )
	for ( j=0; j<MAX_TEAM_POWERUPS; j++ )
	{
		if ( teamPowerups[i][j].powerup == 0 )
			continue;

		if ( teamPowerups[i][j].endTime < gameLocal.time )
		{
			// Expired
			teamPowerups[i][j].powerup = 0;
			teamPowerups[i][j].time = 0;
			teamPowerups[i][j].endTime = 0;
			teamPowerups[i][j].update = false;
		}
		else
		{
			teamPowerups[i][j].time = teamPowerups[i][j].endTime - gameLocal.time;
		}
	}
}

void idMultiplayerGame::SetUpdateForTeamPowerups(int team)
{
	int i;
	for ( i=0; i<MAX_TEAM_POWERUPS; i++ )
	{
		if ( teamPowerups[team][i].powerup != 0 )
			teamPowerups[team][i].update = true;
	}
}

// RITUAL END


