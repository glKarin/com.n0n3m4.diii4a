// RAVEN BEGIN
// ddynerman: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#ifndef __MULTIPLAYERGAME_H__
#define	__MULTIPLAYERGAME_H__

/*
===============================================================================

	Quake IV multiplayer

===============================================================================
*/

#include "mp/Buying.h"
class idPlayer;
class rvCTF_AssaultPoint;
class rvItemCTFFlag;
typedef enum {
	GAME_SP,
	GAME_DM,
	GAME_TOURNEY,
	GAME_TDM,
	// bdube: added ctf
	GAME_CTF,
	// ddynerman: new gametypes
	GAME_1F_CTF,
	GAME_ARENA_CTF,
	GAME_ARENA_1F_CTF,	// is not used, but leaving it in the list so I don't offset GAME_DEADZONE

// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	GAME_DEADZONE,
	NUM_GAME_TYPES,
// RITUAL END
} gameType_t;


// ddynerman: teams
typedef enum {
	TEAM_NONE = -1,
	TEAM_MARINE,
	TEAM_STROGG,
	TEAM_MAX,
} team_t;

// shouchard:  server admin command types
typedef enum {
	SERVER_ADMIN_KICK,
	SERVER_ADMIN_BAN,
	SERVER_ADMIN_REMOVE_BAN,
	SERVER_ADMIN_FORCE_SWITCH,
} serverAdmin_t;

// shouchard:  vote struct for packing up interface values to be handled later
//             note that we have two mechanisms for dealing with vote data that
//             should be consolidated:  this one that handles the interface and
//             multi-field votes and the one that handles console commands and
//             single line votes.  
struct voteStruct_t {
	int				m_fieldFlags;	// flags for which fields are valid
	int				m_kick;			// id of the player
	idStr			m_map;			// name of the map
	int				m_gameType;		// game type enum
	int				m_timeLimit;
	int				m_fragLimit;
	int				m_tourneyLimit;
	int				m_captureLimit;
	int				m_buying;
	int				m_teamBalance;
	int				m_controlTime;
	// restart is a flag only
	// nextmap is a flag only but we don't technically support it (but doom had it so it's here)
};

typedef enum {
	VOTEFLAG_RESTART		= 0x0001,
	VOTEFLAG_BUYING			= 0x0002,
	VOTEFLAG_TEAMBALANCE	= 0x0004,
	VOTEFLAG_SHUFFLE		= 0x0008,
	VOTEFLAG_KICK			= 0x0010,
	VOTEFLAG_MAP			= 0x0020,
	VOTEFLAG_GAMETYPE		= 0x0040,
	VOTEFLAG_TIMELIMIT		= 0x0080,
	VOTEFLAG_TOURNEYLIMIT	= 0x0100,
	VOTEFLAG_CAPTURELIMIT	= 0x0200,
	VOTEFLAG_FRAGLIMIT		= 0x0400,
	VOTEFLAG_CONTROLTIME	= 0x0800,
} voteFlag_t;

#define NUM_VOTES			11
#define MAX_PRINT_LEN 128

// more compact than a chat line
typedef enum {
	MSG_SUICIDE = 0,
	MSG_KILLED,
	MSG_KILLEDTEAM,
	MSG_DIED,
	MSG_VOTE,
	MSG_VOTEPASSED,
	MSG_VOTEFAILED,
	MSG_SUDDENDEATH,
	MSG_FORCEREADY,
	MSG_JOINEDSPEC,
	MSG_TIMELIMIT,
	MSG_FRAGLIMIT,
	MSG_CAPTURELIMIT,
	MSG_TELEFRAGGED,
	MSG_JOINTEAM,
	MSG_HOLYSHIT,
	MSG_COUNT
} msg_evt_t;

typedef enum {
	PLAYER_VOTE_NONE,
	PLAYER_VOTE_NO,
	PLAYER_VOTE_YES,
	PLAYER_VOTE_WAIT	// mark a player allowed to vote
} playerVote_t;

typedef enum {
	PRM_AUTO,
	PRM_SCORE,
	PRM_TEAM_SCORE,
	PRM_TEAM_SCORE_PLUS_SCORE,
	PRM_WINS
} playerRankMode_t;

enum announcerSound_t {
	// General announcements
	AS_GENERAL_ONE,
	AS_GENERAL_TWO,
	AS_GENERAL_THREE,
	AS_GENERAL_YOU_WIN,
	AS_GENERAL_YOU_LOSE,
	AS_GENERAL_FIGHT,
	AS_GENERAL_SUDDEN_DEATH,
	AS_GENERAL_VOTE_FAILED,
	AS_GENERAL_VOTE_PASSED,
	AS_GENERAL_VOTE_NOW,
	AS_GENERAL_ONE_FRAG,
	AS_GENERAL_TWO_FRAGS,
	AS_GENERAL_THREE_FRAGS,
	AS_GENERAL_ONE_MINUTE,
	AS_GENERAL_FIVE_MINUTE,
	AS_GENERAL_PREPARE_TO_FIGHT,
	AS_GENERAL_QUAD_DAMAGE,
	AS_GENERAL_REGENERATION,
	AS_GENERAL_HASTE,
	AS_GENERAL_INVISIBILITY,
	// DM announcements
	AS_DM_YOU_TIED_LEAD,
	AS_DM_YOU_HAVE_TAKEN_LEAD,
	AS_DM_YOU_LOST_LEAD,
	// Team announcements
	AS_TEAM_ENEMY_SCORES,
	AS_TEAM_YOU_SCORE,
	AS_TEAM_TEAMS_TIED,
	AS_TEAM_STROGG_LEAD,
	AS_TEAM_MARINES_LEAD,
	AS_TEAM_JOIN_MARINE,
	AS_TEAM_JOIN_STROGG,
	// CTF announcements
	AS_CTF_YOU_HAVE_FLAG,
	AS_CTF_YOUR_TEAM_HAS_FLAG,
	AS_CTF_ENEMY_HAS_FLAG,
	AS_CTF_YOUR_TEAM_DROPS_FLAG,
	AS_CTF_ENEMY_DROPS_FLAG,
	AS_CTF_YOUR_FLAG_RETURNED,
	AS_CTF_ENEMY_RETURNS_FLAG,
	// Tourney announcements
	AS_TOURNEY_ADVANCE,
	AS_TOURNEY_JOIN_ARENA_ONE,
	AS_TOURNEY_JOIN_ARENA_TWO,
	AS_TOURNEY_JOIN_ARENA_THREE,
	AS_TOURNEY_JOIN_ARENA_FOUR,
	AS_TOURNEY_JOIN_ARENA_FIVE,
	AS_TOURNEY_JOIN_ARENA_SIX,
	AS_TOURNEY_JOIN_ARENA_SEVEN,
	AS_TOURNEY_JOIN_ARENA_EIGHT,
	AS_TOURNEY_JOIN_ARENA_WAITING,
	AS_TOURNEY_DONE,
	AS_TOURNEY_START,
	AS_TOURNEY_ELIMINATED,
	AS_TOURNEY_WON,
	AS_TOURNEY_PRELIMS,
	AS_TOURNEY_QUARTER_FINALS,
	AS_TOURNEY_SEMI_FINALS,
	AS_TOURNEY_FINAL_MATCH,
	AS_GENERAL_TEAM_AMMOREGEN,
	AS_GENERAL_TEAM_DOUBLER,
	AS_NUM_SOUNDS
};

typedef struct mpPlayerState_s {
	int				ping;			// player ping
	int				fragCount;		// kills
	int				teamFragCount;	// teamplay awards
	int				deadZoneScore;  // Score in dead zone
	int				wins;
	playerVote_t	vote;			// player's vote
	bool			scoreBoardUp;	// toggle based on player scoreboard button, used to activate de-activate the scoreboard gui
	bool			ingame;
} mpPlayerState_t;

const int MAX_INSTANCES = 8;

const int NUM_CHAT_NOTIFY	= 5;
const int CHAT_FADE_TIME	= 400;
const int FRAGLIMIT_DELAY	= 2000;
const int CAPTURELIMIT_DELAY = 750;

const int MP_PLAYER_MINFRAGS = -100;
const int MP_PLAYER_MAXFRAGS = 999;
const int MP_PLAYER_MAXWINS	= 100;
const int MP_PLAYER_MAXPING	= 999;

const int MP_PLAYER_MAXKILLS = 999;
const int MP_PLAYER_MAXDEATHS = 999;

const int MAX_AP = 5;

const int CHAT_HISTORY_SIZE = 2048;
const int RCON_HISTORY_SIZE = 4096;

const int KILL_NOTIFICATION_LEN = 256;
//RAVEN BEGIN
//asalmon: update stats for Xenon
#ifdef _XENON
const int XENON_STAT_UPDATE_INTERVAL = 1000;
#endif

const int ASYNC_PLAYER_FRAG_BITS = -idMath::BitsForInteger( MP_PLAYER_MAXFRAGS - MP_PLAYER_MINFRAGS );	// player can have negative frags
const int ASYNC_PLAYER_WINS_BITS = idMath::BitsForInteger( MP_PLAYER_MAXWINS );
const int ASYNC_PLAYER_PING_BITS = idMath::BitsForInteger( MP_PLAYER_MAXPING );
const int ASYNC_PLAYER_INSTANCE_BITS = idMath::BitsForInteger( MAX_INSTANCES );
const int ASYNC_PLAYER_DEATH_BITS = idMath::BitsForInteger( MP_PLAYER_MAXDEATHS );
const int ASYNC_PLAYER_KILL_BITS = idMath::BitsForInteger( MP_PLAYER_MAXKILLS );
//RAVEN END
//RITUAL BEGIN
const int MAX_TEAM_POWERUPS = 5;
//RITUAL END
// ddynerman: game state
#include "mp/GameState.h"

typedef struct mpChatLine_s {
	idStr			line;
	short			fade;			// starts high and decreases, line is removed once reached 0
} mpChatLine_t;

typedef struct mpBanInfo_s {
	idStr			name;
	char			guid[ CLIENT_GUID_LENGTH ];
//	unsigned char	ip[ 15 ];
} mpBanInfo_t;

class idPhysics_Player;

class idMultiplayerGame {

	// rvGameState manages our state
	friend class rvGameState;

public:

					idMultiplayerGame();

	void			Shutdown( void );

	// resets everything and prepares for a match
	void			Reset( void );

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	// Made this public so that level heap can be emptied.
	void			Clear( void );
// RAVEN END

	// setup local data for a new player
	void			SpawnPlayer( int clientNum );

	// Run the MP Game
	void			Run( void );

	// Run the local client
	void			ClientRun( void );
	void			ClientEndFrame( void );

	// Run common code (client & server)
	void			CommonRun( void );

	// draws mp hud, scoredboard, etc.. 
	bool			Draw( int clientNum );
	

	// updates a player vote
	void			PlayerVote( int clientNum, playerVote_t vote );

	// updates frag counts and potentially ends the match in sudden death
	void			PlayerDeath( idPlayer *dead, idPlayer *killer, int methodOfDeath );

	void			AddChatLine( const char *fmt, ... ) id_attribute((format(printf,2,3)));

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	void			OnBuyModeTeamVictory( int winningTeam );
// squirrel: added DeadZone multiplayer mode
	void			OnDeadZoneTeamVictory( int winningTeam );
// RITUAL END

	void			UpdateMainGui( void );
// RAVEN BEGIN
// bdube: global pickup sounds (powerups, etc)
	// Global item acquire sounds 
	void			PlayGlobalItemAcquireSound ( int entityDefIndex );

	bool			CanTalk( idPlayer *from, idPlayer *to, bool echo );
	void			ReceiveAndForwardVoiceData( int clientNum, const idBitMsg &inMsg, int messageType );

#ifdef _USE_VOICECHAT
// jscott: game side voice comms
	void			XmitVoiceData( void );
	void			ReceiveAndPlayVoiceData( const idBitMsg &inMsg );
#endif

// jshepard: selects a map at random that will run with the current game type
	bool			PickMap( idStr gameType, bool checkOnly = false );
	void			ClearVote( int clientNum = -1 );
	void			ResetRconGuiStatus( void );

// RAVEN END

	idUserInterface*	StartMenu( void );

	const char*		HandleGuiCommands( const char *menuCommand );

	void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	void			ReadFromSnapshot( const idBitMsgDelta &msg );

	void			ShuffleTeams( void );
	void			SetGameType( void );
	void			SetMatchStartedTime( int time ) { matchStartedTime = time; }

	rvGameState*	GetGameState( void );


	void			PrintMessageEvent( int to, msg_evt_t evt, int parm1 = -1, int parm2 = -1 );
	void			PrintMessage( int to, const char* message );

	void			DisconnectClient( int clientNum );
	static void		ForceReady_f( const idCmdArgs &args );
	static void		DropWeapon_f( const idCmdArgs &args );
	static void		MessageMode_f( const idCmdArgs &args );
	static void		VoiceChat_f( const idCmdArgs &args );
	static void		VoiceChatTeam_f( const idCmdArgs &args );


// RAVEN BEGIN
// shouchard:  added console commands to mute/unmute voice chat
	static void		VoiceMute_f( const idCmdArgs &args );
	static void		VoiceUnmute_f( const idCmdArgs &args );

// jshepard: command wrappers
	static void		ForceTeamChange_f( const idCmdArgs& args );
	static void		RemoveClientFromBanList_f( const idCmdArgs& args );
	
// autobalance helper for the guis
	static void		CheckTeamBalance_f( const idCmdArgs &args );

// activates the admin console when a rcon password challenge returns.
	void			ProcessRconReturn( bool success );
// RAVEN END

	typedef enum {
		VOTE_RESTART = 0,
		VOTE_TIMELIMIT,
		VOTE_FRAGLIMIT,
		VOTE_GAMETYPE,
		VOTE_KICK,
		VOTE_MAP,
		VOTE_BUYING,
		VOTE_NEXTMAP,
// RAVEN BEGIN
// shouchard:  added capturelimit, round limit, and autobalance to vote flags
		VOTE_CAPTURELIMIT,
		VOTE_ROUNDLIMIT,
		VOTE_AUTOBALANCE,
		VOTE_MULTIFIELD,	// all the "packed" vote functions
// RAVEN END
		VOTE_CONTROLTIME,
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

// RAVEN BEGIN
// shouchard:  added enum to remove magic numbers
	typedef enum {
		VOTE_GAMETYPE_DM = 0,
		VOTE_GAMETYPE_TOURNEY,
		VOTE_GAMETYPE_TDM,
		VOTE_GAMETYPE_CTF,
		VOTE_GAMETYPE_ARENA_CTF,
//RITUAL BEGIN
//
		VOTE_GAMETYPE_DEADZONE,
//RITUAL END
		VOTE_GAMETYPE_COUNT
	} vote_gametype_t;
// RAVEN END

	static void		Vote_f( const idCmdArgs &args );
	static void		CallVote_f( const idCmdArgs &args );
	void			ClientCallVote( vote_flags_t voteIndex, const char *voteValue );
	void			ServerCallVote( int clientNum, const idBitMsg &msg );
	void			ClientStartVote( int clientNum, const char *voteString );
	void			ServerStartVote( int clientNum, vote_flags_t voteIndex, const char *voteValue );
// RAVEN BEGIN
// shouchard:  multiline vote support
	void			ClientUpdateVote( vote_result_t result, int yesCount, int noCount, const voteStruct_t &voteData );
// RAVEN END
	void			CastVote( int clientNum, bool vote );
	void			ExecuteVote( void );
// RAVEN BEGIN
// shouchard:  multiline vote handlers
	void			ClientCallPackedVote( const voteStruct_t &voteData );
	void			ServerCallPackedVote( int clientNum, const idBitMsg &msg );
	void			ClientStartPackedVote( int clientNum, const voteStruct_t &voteData );
	void			ServerStartPackedVote( int clientNum, const voteStruct_t &voteData );
	void			ExecutePackedVote( void );
	const char *	LocalizeGametype( void );
// RAVEN END

	void			WantKilled( int clientNum );
	int				NumActualClients( bool countSpectators, int *teamcount = NULL );
	void			DropWeapon( int clientNum );
	void			MapRestart( void );
	void			JoinTeam( const char* team );
	// called by idPlayer whenever it detects a team change (init or switch)
	void			SwitchToTeam( int clientNum, int oldteam, int newteam );
	
	bool			IsPureReady( void ) const;
	void			ProcessChatMessage( int clientNum, bool team, const char *name, const char *text, const char *sound );
	void			ProcessVoiceChat( int clientNum, bool team, int index );
// RAVEN BEGIN
// shouchard:  added commands to mute/unmute voice chat
	void			ClientVoiceMute( int clientNum, bool mute );
	int				GetClientNumFromPlayerName( const char *playerName );
	void			ServerHandleVoiceMuting( int clientSrc, int clientDest, bool mute );
// shouchard:  fixing a bug in multiplayer where round timer sounds (5 minute
//             warning, etc.) don't go away at the end of the round.
	void			ClearAnnouncerSounds( void );
// shouchard:  server admin stuff
	typedef struct 
	{
		bool		restartMap;
		idStr		mapName;
		int			gameType;
		int			captureLimit;
		int			fragLimit;
		int			tourneyLimit;
		int			timeLimit;
		int			minPlayers;
		int			controlTime;
		bool		buying;
		bool		autoBalance;
		bool		shuffleTeams;
	} serverAdminData_t;

	void			HandleServerAdminBanPlayer( int clientNum );
	void			HandleServerAdminRemoveBan( const char * info );
	void			HandleServerAdminKickPlayer( int clientNum );
	void			HandleServerAdminForceTeamSwitch( int clientNum );
	bool			HandleServerAdminCommands( serverAdminData_t &data );
// RAVEN END

// RITUAL BEGIN
	typedef struct mpTeamPowerups_s {
		int powerup;
		int time;
		bool update;
		int endTime;
	} mpTeamPowerups_t;

	mpTeamPowerups_t teamPowerups[TEAM_MAX][MAX_TEAM_POWERUPS];

	void			AddTeamPowerup(int powerup, int time, int team);
	void			UpdateTeamPowerups();
	void			SetUpdateForTeamPowerups(int team);
// RITUAL END

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

	void			ServerClientConnect( int clientNum );

	void			PlayerStats( int clientNum, char *data, const int len );

	void			AddTeamScore ( int team, int amount );
	void			AddPlayerScore( idPlayer* player, int amount );
	void			AddPlayerTeamScore( idPlayer* player, int amount );
	void			AddPlayerWin( idPlayer* player, int amount );
	void			SetPlayerTeamScore( idPlayer* player, int value );
	void			SetPlayerDeadZoneScore( idPlayer* player, float value );
	void			SetPlayerScore( idPlayer* player, int value );
	void			SetPlayerWin( idPlayer* player, int value );
	void			SetHudOverlay( idUserInterface* overlay, int duration );

	void			ClearMap ( void );

	void			EnableDamage( bool enable = true );

	idPlayer*		GetRankedPlayer( int i );
	int				GetRankedPlayerScore( int i );
	int				GetNumRankedPlayers( void );
	
	idPlayer*		GetUnrankedPlayer( int i );
	int				GetNumUnrankedPlayers( void );

	int				GetScore( int i );
	int				GetScore( idPlayer* player );

	int				GetTeamScore( int i );
	int				GetTeamScore( idPlayer* player );

	int				GetWins( int i );
	int				GetWins( idPlayer* player );

	// asalmon: Get the score for a team.
	int				GetScoreForTeam( int i );
	int				GetTeamsTotalFrags( int i );
	int				GetTeamsTotalScore( int i );
	idUserInterface *GetMainGUI() {return mainGui;}

	float			GetPlayerDeadZoneScore(idPlayer* player);

	int				TeamLeader( void );

	int				GetPlayerTime( idPlayer* player );

	const char*		GetLongGametypeName( const char* gametype );
	int				GameTypeToVote( const char *gameType );

	void			ReceiveRemoteConsoleOutput( const char* output );

	void			ClientSetInstance( const idBitMsg& msg );
	void			ServerSetInstance( int instance );

	void			AddPrivatePlayer( int clientId );
	void			RemovePrivatePlayer( int clientId );

//RAVEN BEGIN
//asalmon: Xenon scoreboard update
#ifdef _XENON
	void			UpdateXenonScoreboard( idUserInterface *scoreBoard );
	int				lastScoreUpdate;
// mekberg: for selecting local player
	void			SelectLocalPlayer( idUserInterface *scoreBoard );
#endif
//RAVEN END
	int				VerifyTeamSwitch( int wantTeam, idPlayer *player );

	void			RemoveAnnouncerSound( int type );
	void			RemoveAnnouncerSoundRange( int startType, int endType );
	void			ScheduleAnnouncerSound ( announcerSound_t sound, float time, int instance = -1, bool allowOverride = false );
	void			ScheduleTimeAnnouncements( void );
// RAVEN END

	void			SendDeathMessage( idPlayer *attacker, idPlayer *victim, int methodOfDeath );
	void			ReceiveDeathMessage( idPlayer *attacker, int attackerScore, idPlayer *victim, int victimScore, int methodOfDeath );

	rvCTF_AssaultPoint*		NextAP( int team );
	int						OpposingTeam( int team );

	idList<idEntityPtr<rvCTF_AssaultPoint> > assaultPoints;

	// Buying Manager - authority for buying system game balance constants (awards,
	// costs, etc.)
	riBuyingManager	mpBuyingManager;

	idUserInterface* statSummary;			// stat summary
	rvTourneyGUI	tourneyGUI;

	void			ShowStatSummary( void );
	bool			CanCapture( int team );
	void			FlagCaptured( idPlayer *player );
	
	void			UpdatePlayerRanks( playerRankMode_t rankMode = PRM_AUTO );
	void			UpdateTeamRanks( void );
	void			UpdateHud( idUserInterface* _mphud );
	idPlayer *		FragLimitHit( void );
	idPlayer *		FragLeader( void );
	bool			TimeLimitHit( void );
	int				GetCurrentMenu( void ) { return currentMenu; }

	void			SetFlagEntity( idEntity* ent, int team );
	idEntity*		GetFlagEntity( int team );

	void			WriteNetworkInfo( idFile *file, int clientNum );
	void			ReadNetworkInfo( idFile* file, int clientNum );

	void			SetShaderParms( renderView_t *view );
	
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	int				NumberOfPlayersOnTeam( int team );
	int				NumberOfAlivePlayersOnTeam( int team );
	void			ReportZoneControllingPlayer( idPlayer* player );
	void			ReportZoneController(int team, int pCount, int situation, idEntity* zoneTrigger = 0);
	bool			IsValidTeam(int team);
	void			ControlZoneStateChanged( int team );

	int				powerupCount;
	int				prevAnnouncerSnd;
	int				defaultWinner;
	int				deadZonePowerupCount;
	dzState_t		dzState[ TEAM_MAX ];
	float			marineScoreBarPulseAmount;
	float			stroggScoreBarPulseAmount;
// RITUAL END

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	bool			isBuyingAllowedRightNow;

	void			OpenLocalBuyMenu( void );
	void			RedrawLocalBuyMenu( void );
	void			GiveCashToTeam( int team, float cashAmount );
	bool			IsBuyingAllowedInTheCurrentGameMode( void );
	bool			IsBuyingAllowedRightNow( void );
// RITUAL END
	static const char*	teamNames[ TEAM_MAX ];

private:
	static const char	*MPGuis[];
	static const char	*ThrottleVars[];
	static const char	*ThrottleVarsInEnglish[];
	static const int	ThrottleDelay[];

	char			killNotificationMsg[ KILL_NOTIFICATION_LEN ];

	int				pingUpdateTime;			// time to update ping

	mpPlayerState_t	playerState[ MAX_CLIENTS ];

	// game state
	rvGameState*	gameState;

	// vote vars
	vote_flags_t	vote;					// active vote or VOTE_NONE
	int				voteTimeOut;			// when the current vote expires
	int				voteExecTime;			// delay between vote passed msg and execute
	int				yesVotes;				// counter for yes votes
	int				noVotes;				// and for no votes
	idStr			voteValue;				// the data voted upon ( server )
	idStr			voteString;				// the vote string ( client )
	bool			voted;					// hide vote box ( client )
	int				kickVoteMap[ MAX_CLIENTS ];
// RAVEN BEGIN
// shouchard:  names for kickVoteMap
	idStr			kickVoteMapNames[ MAX_CLIENTS ];
	voteStruct_t	currentVoteData;		// used for multi-field votes
// RAVEN END

	idStr			localisedGametype;

	// time related
	int				matchStartedTime;		// time current match started

	// guis
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	//int				sqRoundNumber;			// round number in DeadZone; match expires when this equals "sq_numRoundsPerMatch" (cvar)
// squirrel: Mode-agnostic buymenus
	idUserInterface *buyMenu;				// buy menu
// RITUAL END
	idUserInterface *scoreBoard;			// scoreboard
	idUserInterface *mainGui;				// ready / nick / votes etc.
	idListGUI		*mapList;
	idUserInterface *msgmodeGui;			// message mode
	int				currentMenu;			// 0 - none, 1 - mainGui, 2 - msgmodeGui
	int				nextMenu;				// if 0, will do mainGui
	bool			bCurrentMenuMsg;		// send menu state updates to server


	enum {
		MPLIGHT_CTF_MARINE,
		MPLIGHT_CTF_STROGG,
		MPLIGHT_QUAD,
		MPLIGHT_HASTE,
		MPLIGHT_REGEN,
		MPLIGHT_MAX
	};

	int				lightHandles[ MPLIGHT_MAX ];
	renderLight_t	lights[ MPLIGHT_MAX ];

	// chat buffer
	idStr			chatHistory;

	// rcon buffer
	idStr			rconHistory;

//RAVEN BEGIN
//asalmon: Need to refresh stats periodically if the player is looking at stats
	int currentStatClient;
	int currentStatTeam;
//RAVEN END

public:
	// current player rankings
	idList<rvPair<idPlayer*, int> > 	rankedPlayers;
	idList<idPlayer*>		unrankedPlayers;

	rvPair<int, int>		rankedTeams[ TEAM_MAX ];

private:

	int				lastVOAnnounce;

	int				lastReadyToggleTime;
	bool			pureReady;				// defaults to false, set to true once server game is running with pure checksums
	bool			currentSoundOverride;
	int				switchThrottle[ 3 ];
	int				voiceChatThrottle;

	void			SetupBuyMenuItems();

	idList<int>		privateClientIds;
	int				privatePlayers;

	// player who's rank info we're displaying
	idEntityPtr<idPlayer>		rankTextPlayer;

	idEntityPtr<idEntity>		flagEntities[ TEAM_MAX ];	
	idEntityPtr<idPlayer>		flagCarriers[ TEAM_MAX ];

	// updates the passed gui with current score information
	void			UpdateRankColor( idUserInterface *gui, const char *mask, int i, const idVec3 &vec );	

	// bdube: test scoreboard
	void			UpdateTestScoreboard( idUserInterface *scoreBoard );
	
	// ddynerman: gametype specific scoreboard
	void			UpdateScoreboard( idUserInterface *scoreBoard );

	void			UpdateDMScoreboard( idUserInterface *scoreBoard );
	void			UpdateTeamScoreboard( idUserInterface *scoreBoard );
	void			UpdateSummaryBoard( idUserInterface *scoreBoard );

	int				GetPlayerRank( idPlayer* player, bool& isTied );
	char*			GetPlayerRankText( idPlayer* player );
	char*			GetPlayerRankText( int rank, bool tied, int score );

	const char*		BuildSummaryListString( idPlayer* player, int rankedScore );

	void			UpdatePrivatePlayerCount( void );

	typedef struct announcerSoundNode_s {
		announcerSound_t					soundShader;
		float								time;
		idLinkList<announcerSoundNode_s>	announcerSoundNode;
		int									instance;
		bool								allowOverride;
	} announcerSoundNode_t;

	idLinkList<announcerSoundNode_t>	announcerSoundQueue;
	announcerSound_t					lastAnnouncerSound;

	static const char* announcerSoundDefs[ AS_NUM_SOUNDS ];

	float			announcerPlayTime;

	void			PlayAnnouncerSounds ( void );

	int				teamScore[ TEAM_MAX ];
	int				teamDeadZoneScore[ TEAM_MAX];
	void			ClearTeamScores ( void );

	void			UpdateLeader( idPlayer* oldLeader );

	void			ClearGuis( void );
	void			DrawScoreBoard( idPlayer *player );
	void			CheckVote( void );
	bool			AllPlayersReady( idStr* reason = NULL );
	
	const char *	GameTime( void );

	bool			EnoughClientsToPlay( void );
	void			DrawStatSummary( void );
	// go through the clients, and see if they want to be respawned, and if the game allows it
	// called during normal gameplay for death -> respawn cycles
	// and for a spectator who want back in the game (see param)
	void			CheckRespawns( idPlayer *spectator = NULL );

	void			FreeLight ( int lightID );
	void			UpdateLight ( int lightID, idPlayer *player );
	void			CheckSpecialLights( void );
	void			ForceReady();
	// when clients disconnect or join spectate during game, check if we need to end the game
	void			CheckAbortGame( void );
	void			MessageMode( const idCmdArgs &args );
	void			DisableMenu( void );
	void			SetMapShot( void );
	// scores in TDM
	void			VoiceChat( const idCmdArgs &args, bool team );

// RAVEN BEGIN
// mekberg: added
	void			UpdateMPSettingsModel( idUserInterface* currentGui );
// RAVEN END

	void			WriteStartState( int clientNum, idBitMsg &msg, bool withLocalClient );
};

ID_INLINE bool idMultiplayerGame::IsPureReady( void ) const {
	return pureReady;
}

ID_INLINE void idMultiplayerGame::ClearFrags( int clientNum ) {
	playerState[ clientNum ].fragCount = 0;
}

ID_INLINE bool idMultiplayerGame::IsInGame( int clientNum ) {
	return playerState[ clientNum ].ingame;
}

ID_INLINE int idMultiplayerGame::OpposingTeam( int team ) {
	return (team == TEAM_STROGG ? TEAM_MARINE : TEAM_STROGG);
}

ID_INLINE idPlayer* idMultiplayerGame::GetRankedPlayer( int i ) {
	if( i >= 0 && i < rankedPlayers.Num() ) {
		return rankedPlayers[ i ].First();
	} else {
		return NULL;
	}
}

ID_INLINE int idMultiplayerGame::GetRankedPlayerScore( int i ) {
	if( i >= 0 && i < rankedPlayers.Num() ) {
		return rankedPlayers[ i ].Second();
	} else {
		return 0;
	}
}

ID_INLINE int idMultiplayerGame::GetNumUnrankedPlayers( void ) {
	return unrankedPlayers.Num();
}

ID_INLINE idPlayer* idMultiplayerGame::GetUnrankedPlayer( int i ) {
	if( i >= 0 && i < unrankedPlayers.Num() ) {
		return unrankedPlayers[ i ];
	} else {
		return NULL;
	}
}

ID_INLINE int idMultiplayerGame::GetNumRankedPlayers( void ) {
	return rankedPlayers.Num();
}

ID_INLINE int idMultiplayerGame::GetTeamScore( int i ) {
	return playerState[ i ].teamFragCount;
}

ID_INLINE int idMultiplayerGame::GetScore( int i ) {
	return playerState[ i ].fragCount;
}

ID_INLINE int idMultiplayerGame::GetWins( int i ) {
	return playerState[ i ].wins;
}

ID_INLINE void idMultiplayerGame::ResetRconGuiStatus( void ) {
	if( mainGui) {
		mainGui->SetStateInt( "password_valid", 0 );
	}
}

// asalmon: needed access team scores for rich presence
ID_INLINE int idMultiplayerGame::GetScoreForTeam( int i ) {
	if( i < 0 || i > TEAM_MAX ) {
		return 0;
	}
	return teamScore[ i ];
}

ID_INLINE int idMultiplayerGame::TeamLeader( void ) {
	if( teamScore[ TEAM_MARINE ] == teamScore[ TEAM_STROGG ] ) {
		return -1;
	} else {
		return ( teamScore[ TEAM_MARINE ] > teamScore[ TEAM_STROGG ] ? TEAM_MARINE : TEAM_STROGG );
	}
}

int ComparePlayersByScore( const void* left, const void* right );
int CompareTeamsByScore( const void* left, const void* right );

#endif	/* !__MULTIPLAYERGAME_H__ */

// RAVEN END
