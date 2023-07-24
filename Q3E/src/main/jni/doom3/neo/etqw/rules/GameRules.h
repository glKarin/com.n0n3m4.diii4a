// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_RULES_GAMERULES_H__
#define __GAME_RULES_GAMERULES_H__

#include "../roles/Inventory.h"

class sdTeamInfo;
class idPlayer;
class sdFireTeam;
class sdCallVote;

class sdGameRulesNetworkState : public sdEntityStateNetworkData {
public:
								sdGameRulesNetworkState( void ) { ; }

	virtual void				MakeDefault( void );

	virtual void				Write( idFile* file ) const;
	virtual void				Read( idFile* file );

	int							pings[ MAX_CLIENTS ];
	sdTeamInfo*					teams[ MAX_CLIENTS ];
	int							userGroups[ MAX_CLIENTS ];
	int							state;
	int							matchStartedTime;
	int							nextStateSwitch;
};

enum muteFlags_e {
	MF_CHAT			= BITT< 0 >::VALUE,
	MF_AUDIO		= BITT< 1 >::VALUE,
};

enum readyState_e {
	RS_READY,
	RS_NOT_ENOUGH_CLIENTS,		
	RS_NOT_ENOUGH_READY,
};

/*
============
sdGameRules
============
*/
class sdGameRules : public idClass {
public:
	ABSTRACT_PROTOTYPE( sdGameRules );

	struct fireTeamState_t {
		int					fireTeamIndex;
		bool				fireTeamLeader;
		bool				fireTeamPublic;
		idStr				fireTeamName;	
	};

	struct playerState_t {
		int					ping;			// player ping
		sdFireTeam*			fireTeam;
		fireTeamState_t		backupFireTeamState; // players fire team state
		qhandle_t			userGroup;
		int					muteStatus;
		int					numWarnings;
		sdTeamInfo*			backupTeam;
		sdPlayerClassSetup	backupClass;
	};

	typedef playerState_t playerStateList_t[ MAX_CLIENTS ];

	// end game statistics
	struct teamMapData_t {
		idList< float >		xp;
	};

	struct mapData_t {
		idStr					metaDataName;
		const sdDeclMapInfo*	mapInfo;
		sdTeamInfo*				winner;
		idList< teamMapData_t >	teamData;
		bool					written;
	};

	// game state
	enum gameState_t {
		GS_INACTIVE = 0,						// not running
		GS_WARMUP,								// warming up
		GS_COUNTDOWN,							// post warmup pre-game
		GS_GAMEON,								// game is on
		GS_GAMEREVIEW,							// game is over, scoreboard is up
		GS_NEXTGAME,
		GS_NEXTMAP,
		GS_STATE_COUNT,
	};

	// report a simplified set of states
	// update PGS_NEXT_BIT below if more states are added!
	// the values must fit into a byte
	enum probeGameState_t {
		PGS_WARMUP		= BITT< 0 >::VALUE,
		PGS_RUNNING		= BITT< 1 >::VALUE,
		PGS_REVIEWING	= BITT< 2 >::VALUE,
		PGS_LOADING		= BITT< 3 >::VALUE,
	};
	static const int PGS_NEXT_BIT = 4;

	enum chatMode_t {
		CHAT_MODE_DEFAULT = 0,
		CHAT_MODE_SAY,
		CHAT_MODE_SAY_TEAM,
		CHAT_MODE_SAY_FIRETEAM,
		CHAT_MODE_QUICK,
		CHAT_MODE_QUICK_TEAM,
		CHAT_MODE_QUICK_FIRETEAM,
		CHAT_MODE_MESSAGE,
		CHAT_MODE_OBITUARY,
	};

	enum msgEvent_t {
		MSG_SUICIDE = 0,
		MSG_KILLED,
		MSG_KILLEDTEAM,
		MSG_DIED,
		MSG_COUNT
	};

	enum shuffleMode_t {
		SM_XP = 0,
		SM_RANDOM,
		SM_SWAP,
	};

	enum networkEvent_t {
		EVENT_CREATE = 0,
		EVENT_SETCAMERA,
		MAX_NET_EVENTS,
	};

	typedef sdPair< int, idPlayer* > shuffleData_t;

								sdGameRules( void );
	virtual						~sdGameRules( void );

	// general
	virtual void				Clear( void );
	virtual void				InitConsoleCommands( void );
	virtual void				InitVotes( void );
	virtual void				RemoveConsoleCommands( void );
	virtual void				DisconnectClient( int clientNum );
	virtual void				EnterGame( idPlayer* player );
	virtual void				WantKilled( idPlayer* player );
	virtual void				UpdateScoreboard( sdUIList* list, const char* teamName );
	virtual void				OnTeamChange( idPlayer* player, sdTeamInfo* oldteam, sdTeamInfo* team );
	virtual void				OnPlayerReady( idPlayer* player, bool ready );
	void						NewMap( bool isUserChange );
	virtual void				Shutdown( void );
	virtual void				Reset( void );
	virtual void				SpawnPlayer( idPlayer* player );
	virtual void				Run( void );
	virtual void				MapRestart( void );
	virtual byte				GetProbeState( void ) const;
	virtual int					GetServerBrowserScore( const sdNetSession& session ) const;
	virtual void				GetBrowserStatusString( idWStr& str, const sdNetSession& netSession ) const;

	virtual void				OnNewScriptLoad( void );
	virtual void				OnLocalMapRestart( void );

	virtual sdTeamInfo*			GetWinningTeam( void ) const { return NULL; }
	virtual const sdDeclLocStr*	GetWinningReason( void ) const { return NULL; }

	virtual int					GetNumMaps( void ) const { return 0; }
	virtual const mapData_t*	GetMapData( int index ) const { return NULL; }
	virtual const mapData_t*	GetCurrentMapData() const { return NULL; }

	virtual void				UpdateClientFromServerInfo( const idDict& serverInfo, bool allowMedia );

	virtual bool				InhibitEntitySpawn( idDict &spawnArgs ) const;

	virtual const char*			GetKeySuffix( void ) const { return ""; }

	// chat
	virtual void				ProcessChatMessage( idPlayer* player, gameReliableClientMessage_t mode, const wchar_t *text );
	virtual void				AddChatLine( chatMode_t chatMode, const idVec4& color, const wchar_t *fmt, ... );
	virtual void				AddChatLine( const idVec3& origin, chatMode_t chatMode, const int clientNum, const wchar_t *text );
	virtual void				AddRepeaterChatLine( const char* clientName, const int clientNum, const wchar_t *text );
	virtual void				MessageMode( const idCmdArgs &args );

	virtual void				WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target );

	virtual bool				HandleGuiEvent( const sdSysEvent* event );
	virtual bool				TranslateGuiBind( const idKey& key, sdKeyCommand** cmd );

	virtual void						ApplyNetworkState( const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( void ) const;

	virtual void				EndGame( void ) = 0;
	virtual void				SetWinner( sdTeamInfo* team ) = 0;
	virtual void				SetAttacker( sdTeamInfo* team ) { }
	virtual void				SetDefender( sdTeamInfo* team ) { }

	virtual userMapChangeResult_e OnUserStartMap( const char* text, idStr& reason, idStr& mapName ) = 0;

	virtual bool				ChangeMap( const char* mapName ) { return false; }

	virtual void				ReadCampaignInfo( const idBitMsg& msg ) { ; }

	virtual int					GetGameTime( void ) const = 0;
	int							GetNextStateTime( void ) { return nextStateSwitch; }
	void						SetClientTeam( idPlayer* player, int index, bool force, const char* password );

	static bool					IsRuleType( const idTypeInfo& type );

	const wchar_t*				GetStatusText() const;

	virtual const sdDeclLocStr*	GetTypeText( void ) const = 0;

	idEntity*					GetEndGameCamera( void ) const { return endGameCamera; }

	void						SetPlayerFireTeam( int clientNum, sdFireTeam* fireTeam );
	sdFireTeam*					GetPlayerFireTeam( int clientNum ) const { return playerState[ clientNum ].fireTeam; }

	void						SetPlayerUserGroup( int clientNum, qhandle_t handle ) { playerState[ clientNum ].userGroup = handle; }
	qhandle_t					GetPlayerUserGroup( int clientNum ) const { return playerState[ clientNum ].userGroup; }

	bool						IsGameOn( void ) const { return gameState == GS_GAMEON; }
	bool						IsWarmup( void ) const { return gameState == GS_WARMUP || gameState == GS_COUNTDOWN; }
	bool						IsCountDown( void ) const { return gameState == GS_COUNTDOWN; }
	bool						IsEndGame( void ) const { return gameState == GS_GAMEREVIEW; }
	bool						IsPureReady( void ) const { return pureReady; }

	void						BackupPlayerTeams( void );
	void						RestoreFireTeam( idPlayer* player );

	virtual void				OnScriptChange( void );
	virtual void				OnConnect( idPlayer* player );
	virtual void				OnNetworkEvent( const char* message );

	virtual void				OnObjectiveCompletion( int objectiveIndex ) {}

	void						Event_SendNetworkEvent( int clientIndex, bool isRepeaterClient, const char* message );
	void						Event_SetEndGameCamera( idEntity* other );
	void						Event_SetWinningTeam( idScriptObject* object );
	void						Event_EndGame( void );
	void						Event_GetKeySuffix( void );

	int							MuteStatus( idPlayer* player ) const;
	void						Mute( idPlayer* player, int flags );
	void						UnMute( idPlayer* player, int flags );
	void						Warn( idPlayer* player );

	void						AdminStart( void ) { adminStarted = true; }

	void						ShuffleTeams( shuffleMode_t sm );
	static int					SortPlayers_Random( const shuffleData_t* a, const shuffleData_t* b );
	static int					SortPlayers_XP( const shuffleData_t* a, const shuffleData_t* b );

	gameState_t					GetState( void ) const { return gameState; }

	static int					TeamIndexForName( const char* teamname );

	bool						CanStartMatch( void ) const;
	void						StartMatch( void );
	void						ClearPlayerReadyFlags( void ) const;

	void						SavePlayerStates( playerStateList_t& states );
	void						RestorePlayerStates( const playerStateList_t& states );

	const idList< sdCallVote* >& GetCallVotes( void ) const { return callVotes; }

	virtual void				OnTimeLimitHit( void );
	virtual int					GetTimeLimit( void ) const;

	static int					GetWarmupTime( void );
	static readyState_e			ArePlayersReady( bool readyIfNoPlayers = true, bool checkMin = false, int* numRequired = NULL );

	static idCVar 				g_chatDefaultColor;
	static idCVar 				g_chatTeamColor;
	static idCVar 				g_chatFireTeamColor;
	static idCVar 				g_chatLineTimeout;

	static void					ArgCompletion_RuleTypes( const idCmdArgs &args, void( *callback )( const char *s ) );

	virtual void				ArgCompletion_StartGame( const idCmdArgs& args, argCompletionCallback_t callback ) = 0;

	static bool					ParseNetworkMessage( const idBitMsg& msg );
	virtual bool				ParseNetworkMessage( int msgType, const idBitMsg& msg );

	sdTeamInfo*					FindNeedyTeam( idPlayer* ignore = NULL );

	virtual const char*			GetDemoNameInfo( void ) = 0;

protected:
	void						NextStateDelayed( gameState_t state, int delay );
	void						NewState( gameState_t news );

	void						ClearChatData( void );
	void						CheckRespawns( bool force = false );

	static int					NumActualClients( bool countSpectators, bool countBots = false );

	void						UpdateChatLines();
	void						SendCameraEvent( idEntity* entity, const sdReliableMessageClientInfoBase& target );

	void						CallScriptEndGame( void );
	static void					RecordWinningTeam( sdTeamInfo* winner, const char* prefix, bool includeTeamName );	

	static void					SetupLoadScreenUI( sdUserInterfaceScope& scope, const char* status, bool currentMap, int mapIndex, const idDict& metaData, const sdDeclMapInfo* mapInfo );

	void						SetWarmupStatusMessage();
	static int					NumReady( int& total );

protected:
	virtual void				GameState_Review( void ) = 0;
	virtual void				GameState_NextGame( void ) = 0;
	virtual void				GameState_Warmup( void ) = 0;	
	virtual void				GameState_Countdown( void ) = 0;
	virtual void				GameState_GameOn( void ) = 0;
	virtual void				GameState_NextMap( void ) = 0;

	virtual void				OnGameState_Review( void ) = 0;
	virtual void				OnGameState_Warmup( void );
	virtual void				OnGameState_Countdown( void ) = 0;
	virtual void				OnGameState_GameOn( void ) = 0;
	virtual void				OnGameState_NextMap( void ) = 0;

private:
	// commands
	static void					MessageMode_f( const idCmdArgs &args );
	static void					QuickChat_f( const idCmdArgs& args );
	static void					SetTeam_f( const idCmdArgs &args );
	static void					SetClass_f( const idCmdArgs &args );
	static void					DefaultSpawn_f( const idCmdArgs &args );
	static void					WantSpawn_f( const idCmdArgs &args );

	static void					CreateChatList( sdUIList* list );

	static int					InsertScoreboardPlayer( sdUIList* list, const idPlayer* player, const sdFireTeam* fireTeam,
														const playerState_t& ps,
														const wchar_t* noMissionText,
														bool showClass,
														float& averagePing,
														float& averageXP,
														int& numInGamePlayers,
														int index );

protected:
	class sdChatLine {
	public:
		typedef idLinkList< sdChatLine > node_t;
	public:
		sdChatLine() :
			time( 0 ),
			color( colorWhite ),
			text( L"" ) {
			flags.expired	= false;
			flags.team		= false;
			flags.obituary	= false;
			node.SetOwner( this );
		}	

		void Set( const wchar_t* text, chatMode_t mode );

		bool 			IsExpired() const		{ return flags.expired; }
		bool 			IsTeam() const			{ return flags.team; }
		bool 			IsObituary() const		{ return flags.obituary; }

		void 			CheckExpired()			{ flags.expired = ( gameLocal.ToGuiTime( gameLocal.time ) - time ) >= SEC2MS( g_chatLineTimeout.GetFloat() ); }

		const idVec4&	GetColor() const		{ return color; }
		const wchar_t*	GetText() const			{ return text.c_str(); }

		node_t&			GetNode()				{ return node; }
		const node_t&	GetNode() const			{ return node; }
		
		int				GetTime() const			{ return time; }

	private:
		struct flags_t {
			bool expired	: 1;
			bool team		: 1;
			bool obituary	: 1;
		} flags;

		int			time;
		idVec4		color;
		idWStr		text;
		node_t		node;		
	};

protected:
	gameState_t					gameState;				// what state the current game is in
	gameState_t					nextState;				// state to switch to when nextStateSwitch is hit
	int							pingUpdateTime;			// time to update ping

	playerStateList_t			playerState;

	idScriptObject*				scriptObject;

	idEntityPtr< idEntity >		endGameCamera;

	// time related
	int							nextStateSwitch;		// time next state switch
	int							lastStateSwitchTime;
	int							matchStartedTime;		// time current match started

	bool						needsRestart;
	bool						adminStarted;
	bool						pureReady;				// defaults to false, set to true once server game is running with pure checksums
	bool						commandsAdded;
	mutable int					autoReadyStartTime;

//	static const char *			gameStateStrings[ GS_STATE_COUNT ];	

	static const int MAX_CHAT_LINES = 128;
	idStaticList< sdChatLine, MAX_CHAT_LINES >		chatLines;
	sdChatLine::node_t								chatHead;
	sdChatLine::node_t								chatFree;
	
	idList< sdCallVote* >							callVotes;

	idWStr						statusText;
	const sdDeclLocStr*			noMission;
	const sdDeclLocStr*			infinity;
};


// Encapsulate some common behaviour for game types that manage a single map
class sdGameRules_SingleMapHelper {
public:	
	static void						ArgCompletion_StartGame( const idCmdArgs& args, argCompletionCallback_t callback );
	static userMapChangeResult_e	OnUserStartMap( const char* text, idStr& reason, idStr& mapName );
	static void						SanitizeMapName( idStr& mapName, bool setExtension );
};

#endif /* !__GAME_RULES_GAMERULES_H__ */
