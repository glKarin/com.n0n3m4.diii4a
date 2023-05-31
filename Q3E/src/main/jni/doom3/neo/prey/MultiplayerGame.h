// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __MULTIPLAYERGAME_H__
#define	__MULTIPLAYERGAME_H__

/*
===============================================================================

	Basic DOOM multiplayer

===============================================================================
*/

class idPlayer;

typedef enum {
	GAME_SP,
	GAME_DM,
	GAME_TOURNEY,
	GAME_TDM,
	GAME_LASTMAN
} gameType_t;

typedef enum {
		PLAYER_VOTE_NONE,
		PLAYER_VOTE_NO,
		PLAYER_VOTE_YES,
		PLAYER_VOTE_WAIT	// mark a player allowed to vote
} playerVote_t;

typedef struct mpPlayerState_s {
	int				ping;			// player ping
	int				fragCount;		// kills
	int				teamFragCount;	// team kills
	int				wins;			// wins
	playerVote_t	vote;			// player's vote
	bool			scoreBoardUp;	// toggle based on player scoreboard button, used to activate de-activate the scoreboard gui
	bool			ingame;
} mpPlayerState_t;

// HUMANHEAD pdm: Reworked for our chat system
const int NUM_CHAT_HISTORY_LINES	= 10;
const int NUM_CHAT_DISPLAY_LINES	= 10;
const int CHAT_MAX_LIFE_TIME		= 8000;
const int MAX_HISTORY_LIST			= 50;		//fixme: bump up once we fix the wrap around case
// HUMANHEAD END
const int FRAGLIMIT_DELAY			= -1; //HUMANHEAD rww - was 2000, doing away with sudden death

const int MP_PLAYER_MINFRAGS = -100;
const int MP_PLAYER_MAXFRAGS = 100;
const int MP_PLAYER_MAXWINS	= 100;
const int MP_PLAYER_MAXPING	= 999;

// HUMANHEAD pdm: Reworked for our chat system
typedef struct mpChatLine_s {
	idStr			line;
	int				startTime;
} mpChatLine_t;
// HUMANHEAD END

typedef enum {
	SND_YOUWIN = 0,
	SND_YOULOSE,
	SND_FIGHT,
	SND_VOTE,
	SND_VOTE_PASSED,
	SND_VOTE_FAILED,
	SND_THREE,
	SND_TWO,
	SND_ONE,
	SND_SUDDENDEATH,
	//HUMANHEAD rww
	SND_LEADTAKEN,
	SND_LEADLOST,
	SND_LEADTIED,
	//HUMANHEAD END
	SND_COUNT
} snd_evt_t;

//HUMANHEAD PCF rww 05/17/06 - terrible lastminute hackjob for localized vote text
#define _HH_LOCALIZE_VOTESTRINGS 1
//HUMANHEAD END

class idMultiplayerGame {
public:

					idMultiplayerGame();

	void			Shutdown( void );

	// resets everything and prepares for a match
	void			Reset( void );

	// setup local data for a new player
	void			SpawnPlayer( int clientNum );

	// checks rules and updates state of the mp game
	void			Run( void );

	// draws mp hud, scoredboard, etc.. 
	bool			Draw( int clientNum );

	// updates a player vote
	void			PlayerVote( int clientNum, playerVote_t vote );

	// updates frag counts and potentially ends the match in sudden death
	void			PlayerDeath( idPlayer *dead, idPlayer *killer, idEntity *inflictor, bool telefrag ); //HUMANHEAD rww - pass inflictor

	void			AddChatLine( const char *fmt, ... ) id_attribute((format(printf,2,3)));

	void			UpdateMainGui( void );
	idUserInterface*StartMenu( void );
	const char*		HandleGuiCommands( const char *menuCommand );
//	void			SetMenuSkin( void );	// HUMANHEAD pdm: removed

	void			CheckScoreChange(int clientNum, int oldScore, int newScore, bool leavingGame); //HUMANHEAD rww

	void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	void			ReadFromSnapshot( const idBitMsgDelta &msg );

	// game state
	typedef enum {
		INACTIVE = 0,						// not running
		WARMUP,								// warming up
		COUNTDOWN,							// post warmup pre-game
		GAMEON,								// game is on
		SUDDENDEATH,						// game is on but in sudden death, first frag wins
		GAMEREVIEW,							// game is over, scoreboard is up. we wait si_gameReviewPause seconds (which has a min value)
		NEXTGAME,
		STATE_COUNT
	} gameState_t;
	static const char *GameStateStrings[ STATE_COUNT ];
	idMultiplayerGame::gameState_t		GetGameState( void ) const;

	static const char *GlobalSoundStrings[ SND_COUNT ];
	void			PlayGlobalSound( int to, snd_evt_t evt, const char *shader = NULL );

	// more compact than a chat line
	typedef enum {
		MSG_SUICIDE = 0,
		MSG_KILLED,
		MSG_KILLEDTEAM,
		MSG_DIED,
		MSG_VOTE,
		MSG_PLAYERVOTED,	// HUMANHEAD pdm
		MSG_VOTEPASSED,
		MSG_VOTEFAILED,
		MSG_SUDDENDEATH,
		MSG_FORCEREADY,
		MSG_JOINEDSPEC,
		MSG_TIMELIMIT,
		MSG_FRAGLIMIT,
		MSG_TELEFRAGGED,
		MSG_JOINTEAM,
		MSG_HOLYSHIT,
		MSG_COUNT
	} msg_evt_t;
	//HUMANHEAD rww
	const char		*DeathStringForProjectile(int projectileDefNum);
	void			PrintDeathMessageEvent( int to, msg_evt_t evt, int parm1, int parm2, int inflictorDef );
	//HUMANHEAD END
	void			PrintMessageEvent( int to, msg_evt_t evt, int parm1 = -1, int parm2 = -1 );

	void			DisconnectClient( int clientNum );
	static void		ForceReady_f( const idCmdArgs &args );
	static void		DropWeapon_f( const idCmdArgs &args );
	static void		MessageMode_f( const idCmdArgs &args );
	static void		VoiceChat_f( const idCmdArgs &args );
	static void		VoiceChatTeam_f( const idCmdArgs &args );

	typedef enum {
		VOTE_RESTART = 0,
		VOTE_TIMELIMIT,
		VOTE_FRAGLIMIT,
		VOTE_GAMETYPE,
		VOTE_KICK,
		VOTE_MAP,
		VOTE_SPECTATORS,
		VOTE_NEXTMAP,
		VOTE_COUNT,
		VOTE_NONE
	} vote_flags_t;

	typedef enum {
		VOTE_UPDATE,
		VOTE_FAILED,
		VOTE_PASSED,	// passed, but no reset yet
		VOTE_ABORTED,
		VOTE_RESET		// tell clients to reset vote state
	} vote_result_t;

	static void		Vote_f( const idCmdArgs &args );
	static void		CallVote_f( const idCmdArgs &args );
	void			ClientCallVote( vote_flags_t voteIndex, const char *voteValue );
	void			ServerCallVote( int clientNum, const idBitMsg &msg );
#if _HH_LOCALIZE_VOTESTRINGS
	typedef enum {
		VOTESTR_JUSTONE,
		VOTESTR_TIMELIMIT,
		VOTESTR_FRAGLIMIT,
		VOTESTR_VALUE,
		VOTESTR_CLNUMNAME,
		VOTESTR_NAMEVAL
	} voteStringType_e;
	void			ClientStartVote( int clientNum, voteStringType_e strType, const int numStrings, const char **strings );
#else
	void			ClientStartVote( int clientNum, const char *voteString );
#endif
	void			ServerStartVote( int clientNum, vote_flags_t voteIndex, const char *voteValue );
	void			ClientUpdateVote( vote_result_t result, int yesCount, int noCount );
	void			CastVote( int clientNum, bool vote );
	void			ExecuteVote( void );

	void			WantKilled( int clientNum );
	int				NumActualClients( bool countSpectators, int *teamcount = NULL );
	void			DropWeapon( int clientNum );
	void			MapRestart( void );
	// called by idPlayer whenever it detects a team change (init or switch)
	void			SwitchToTeam( int clientNum, int oldteam, int newteam );
	bool			IsPureReady( void ) const;
	void			ProcessChatMessage( int clientNum, bool team, const char *name, const char *text, const char *sound );
	void			ProcessVoiceChat( int clientNum, bool team, int index );

	void			Precache( void );
	
	// throttle UI switch rates
	void			ThrottleUserInfo( void );
	void			ToggleSpectate( void );
	void			ToggleReady( void );
	void			ToggleTeam( void );

	void			ClearFrags( int clientNum );

	void			EnterGame( int clientNum );
	bool			CanPlay( idPlayer *p );
	bool			IsInGame( int clientNum );
	bool			WantRespawn( idPlayer *p );

	void			ServerWriteInitialReliableMessages( int clientNum );
	void			ClientReadStartState( const idBitMsg &msg );
	void			ClientReadWarmupTime( const idBitMsg &msg );

	void			ServerClientConnect( int clientNum );

	void			PlayerStats( int clientNum, char *data, const int len );

private:
	static const char	*MPGuis[];
	static const char	*ThrottleVars[];
	static const char	*ThrottleVarsInEnglish[];
	static const int	ThrottleDelay[];

	// state vars
	gameState_t		gameState;				// what state the current game is in
	gameState_t		nextState;				// state to switch to when nextStateSwitch is hit
	int				pingUpdateTime;			// time to update ping

	mpPlayerState_t	playerState[ MAX_CLIENTS ];

											// keep track of clients which are willingly in spectator mode

	// vote vars
	vote_flags_t	vote;					// active vote or VOTE_NONE
	int				voteTimeOut;			// when the current vote expires
	int				voteExecTime;			// delay between vote passed msg and execute
	float			yesVotes;				// counter for yes votes
	float			noVotes;				// and for no votes
	idStr			voteValue;				// the data voted upon ( server )
	idStr			voteString;				// the vote string ( client )
	bool			voted;					// hide vote box ( client )
	int				kickVoteMap[ MAX_CLIENTS ];

	// time related
	int				nextStateSwitch;		// time next state switch
	int				warmupEndTime;			// warmup till..
	int				matchStartedTime;		// time current match started

	// tourney
	int				currentTourneyPlayer[2];// our current set of players
	int				lastWinner;				// plays again

	// warmup
	idStr			warmupText;				// text shown in warmup area of screen
	bool			one, two, three;		// keeps count down voice from repeating

	// guis
	idUserInterface *scoreBoard;			// scoreboard
	idUserInterface *spectateGui;			// spectate info
	idUserInterface *guiChat;				// chat text
	idUserInterface *mainGui;				// ready / nick / votes etc.
#ifdef _GUITEST_SERVERWAIT
	idUserInterface *serverWaitGui;			//HUMANHEAD rww
#endif
	idListGUI		*mapList;
	idUserInterface *msgmodeGui;			// message mode
	int				currentMenu;			// 0 - none, 1 - mainGui, 2 - msgmodeGui
	int				nextMenu;				// if 0, will do mainGui
	bool			bCurrentMenuMsg;		// send menu state updates to server

	//HUMANHEAD rww - persistent player data dict, to store wins (and potentially other things) between map changes and player disconnects
	idDict			persistentPlayerDict;
	//HUMANHEAD END

	// chat data
// HUMANHEAD pdm: Reworked for our chat system
	mpChatLine_t	chatHistory[ NUM_CHAT_HISTORY_LINES ];
	int				chatHistoryIndex;
	idListGUI		*chatHistoryList;
// HUMANHEAD END

	// rankings are used by UpdateScoreboard and UpdateHud
	int				numRankedPlayers;		// ranked players, others may be empty slots or spectators
	idPlayer *		rankedPlayers[MAX_CLIENTS];

	bool			pureReady;				// defaults to false, set to true once server game is running with pure checksums
	int				fragLimitTimeout;

	int				switchThrottle[ 3 ];
	int				voiceChatThrottle;

	gameType_t		lastGameType;			// for restarts
	int				startFragLimit;			// synchronize to clients in initial state, set on -> GAMEON

	bool			bTeamsTied;				//HUMANHEAD rww - needs to be set in TDM before any calls to UpdateWinsLosses with removal of suddendeath concept

private:
	void			UpdatePlayerRanks();

	// updates the passed gui with current score information
	void			UpdateScoreboard( idUserInterface *scoreBoard, idPlayer *player );
	void			UpdateDMScoreboard( idUserInterface *scoreBoard, idPlayer *player );
	void			UpdateTeamScoreboard( idUserInterface *scoreBoard, idPlayer *player );

	void			ClearGuis( void );
	void			DrawScoreBoard( idPlayer *player );
	void			UpdateHud( idPlayer *player, idUserInterface *hud );
	bool			Warmup( void );
	void			CheckVote( void );
	//HUMANHEAD rww
	void			DisconnectVotingPlayer(int clientNum);
	//HUMANHEAD END
	bool			AllPlayersReady( void );
	idPlayer *		FragLimitHit( void );
	idPlayer *		FragLeader( void );
	bool			TimeLimitHit( void );
	bool			CheckTeamsTied(void); //HUMANHEAD rww
	void			NewState( gameState_t news, idPlayer *player = NULL );
	void			UpdateWinsLosses( idPlayer *winner );
	// fill any empty tourney slots based on the current tourney ranks
	void			FillTourneySlots( void );
	void			CycleTourneyPlayers( void );
	// walk through the tourneyRank to build a wait list for the clients
	void			UpdateTourneyLine( void );
	const char *	GameTime( void );
	const char *	GameFrags( idPlayer *localPlayer );
	void			Clear( void );
	bool			EnoughClientsToPlay( void );
	void			ClearChatData( void );
	void			DrawChat( void );
	// go through the clients, and see if they want to be respawned, and if the game allows it
	// called during normal gameplay for death -> respawn cycles
	// and for a spectator who want back in the game (see param)
	void			CheckRespawns( idPlayer *spectator = NULL );
	void			ForceReady();
	// when clients disconnect or join spectate during game, check if we need to end the game
	void			CheckAbortGame( void );
	void			MessageMode( const idCmdArgs &args );
	void			DisableMenu( void );
	void			SetMapShot( void );
	// scores in TDM
	int				GetTeamScore(int team);
	void			TeamScore( int entityNumber, int team, int delta );
	void			VoiceChat( const idCmdArgs &args, bool team );
	void			DumpTourneyLine( void );
	void			SuddenRespawn( void );
};

ID_INLINE idMultiplayerGame::gameState_t idMultiplayerGame::GetGameState( void ) const {
	return gameState;
}

ID_INLINE bool idMultiplayerGame::IsPureReady( void ) const {
	return pureReady;
}

//ID_INLINE void idMultiplayerGame::ClearFrags( int clientNum ) //HUMANHEAD rww - no longer inlined

ID_INLINE bool idMultiplayerGame::IsInGame( int clientNum ) {
	return playerState[ clientNum ].ingame;
}

#endif	/* !__MULTIPLAYERGAME_H__ */

