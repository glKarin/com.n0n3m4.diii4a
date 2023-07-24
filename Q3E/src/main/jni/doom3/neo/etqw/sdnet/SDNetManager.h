// Copyright (C) 2007 Id Software, Inc.
//

#if !defined ( __GAME_SDNETMANAGER_H__ )
#define __GAME_SDNETMANAGER_H__

#include "SDNetProperties.h"

#include "../../sdnet/SDNetSession.h"
#include "../../sdnet/SDNetTask.h"

class sdNetUser;
class sdNetMessage;
class sdDeclMapInfo;
class sdDeclStringMap;
class sdNetManager;

class sdHotServerList {
public:
	static const int					MAX_HOT_SERVERS		= 3;
	static const int					BROWSER_GOOD_BONUS	= 10;
	static const int					BROWSER_OK_BONUS	= 5;

										sdHotServerList( idList< sdNetSession* >& _sessions ) : sessions( _sessions ) { Clear(); }

	void								Clear( void );
	void								Update( sdNetManager& manager );
	void								Locate( sdNetManager& manager );
	void								SendServerInterestMessages( void );

	int									GetNumServers( void );
	sdNetSession*						GetServer( int index );

	static int							GetServerScore( const sdNetSession& session, sdNetManager& manager );

private:
	static bool							IsServerValid( const sdNetSession& session, sdNetManager& manager );
	void								CalcServerScores( sdNetManager& manager, int* bestServers, int numBestServers );

	idList< sdNetSession* >&			sessions;
	int									hotServerIndicies[ MAX_HOT_SERVERS ];
	int									nextInterestMessageTime;
};

class sdNetManager {
public:
	typedef sdUITemplateFunction< sdNetManager > uiFunction_t;
	enum findServerSource_e {
		FS_MIN = -1,
		FS_LAN = 0,
		FS_INTERNET,
		FS_HISTORY,
		FS_FAVORITES,
#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
		FS_LAN_REPEATER,
		FS_INTERNET_REPEATER,
#endif /* !SD_DEMO_BUILD */
		FS_CURRENT,
		FS_MAX
	};

	enum serverFilterState_e {
		SFS_MIN = -1,
		SFS_DONTCARE = 0,
		SFS_SHOWONLY,
		SFS_HIDE,		
		SFS_MAX
	};

	enum serverFilter_e {
		SF_MIN = -1,
		SF_PASSWORDED = 0,
		SF_PUNKBUSTER,
		SF_FRIENDLYFIRE,
		SF_AUTOBALANCE,
		SF_EMPTY,
		SF_FULL,
		SF_PING,
		SF_BOTS,
		SF_PURE,
		SF_LATEJOIN,
		SF_FAVORITE,
#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
		SF_RANKED,
		SF_FRIENDS,
#endif /* !SD_DEMO_BUILD && !SD_DEMO_BUILD_CONSTRUCTION */
		SF_PLAYERCOUNT,
		SF_MODS,
		SF_MAXBOTS,
		SF_MAX
	};

	enum findServerMode_e {
		FSM_MIN = -1,
		FSM_NEW = 0,
		FSM_REFRESH,
		FSM_MAX
	};

	enum serverFilterOp_e {
		SFO_MIN = -1,
		SFO_EQUAL = 0,
		SFO_NOT_EQUAL,
		SFO_LESS,
		SFO_GREATER,		
		SFO_NOOP,
		SFO_CONTAINS,
		SFO_NOT_CONTAINS,
		SFO_MAX
	};

	enum serverFilterResult_e {
		SFR_MIN = -1,
		SFR_OR = 0,
		SFR_AND,
		SFR_MAX
	};

	enum playerStat_e {
		PS_MIN = -1,
		PS_NAME = 0,
		PS_RANK,
		PS_XP,
		PS_TEAM,
		PS_MAX
	};

	// these must be in the same order as in rules.script/rules::WriteStats
	enum playerReward_e {
		PR_MIN = -1,
		PR_MOST_XP = 0,
		PR_LEAST_XP,
		PR_BEST_SOLDIER,
		PR_BEST_MEDIC,
		PR_BEST_ENGINEER,
		PR_BEST_FIELDOPS,
		PR_BEST_COVERTOPS,
		PR_BEST_LIGHTWEAPONS,
		PR_BEST_BATTLESENSE,
		PR_BEST_VEHICLE,
		PR_ACCURACY_HIGH,
		PR_MOST_OBJECTIVES,
		PR_PROFICIENCY,
		PR_MOST_KILLS,
		PR_MOST_DAMAGE,
		PR_MOST_TEAMKILLS,
		PR_MAX
	};

	enum browserColumn_e {
		BC_IP			= 0,
		BC_FAVORITE,
		BC_PASSWORD,
		BC_RANKED,
		BC_NAME,
		BC_MAP,
		BC_GAMETYPE_ICON,
		BC_GAMETYPE,
		BC_TIMELEFT,
		BC_PLAYERS,
		BC_PING,
	};

	struct serverNumericFilter_t {
		serverFilter_e			type;
		serverFilterState_e		state;
		serverFilterOp_e		op;
		serverFilterResult_e	resultBin;
		float					value;

	};

	struct serverStringFilter_t {
		idStr					value;
		idStr					cvar;
		serverFilterOp_e		op;
		serverFilterState_e 	state;
		serverFilterResult_e	resultBin;
	};

									sdNetManager();
									~sdNetManager() {}

	void							Init();
	void							Shutdown();

	void							RunFrame();

	sdUIPropertyHolder&				GetProperties() { return properties; }
	sdUIFunctionInstance*			GetFunction( const char* name );

	const char*						GetName() const { return "sdnet"; }

	void							CreateServerList( sdUIList* list, findServerSource_e source, findServerMode_e mode, bool flagOffline );	
	void							CreateHotServerList( sdUIList* list, findServerSource_e source );
	void							CreateRetrievedUserNameList( sdUIList* list );

	bool							Connect();

	bool							SignInDedicated();
	bool							SignOutDedicated();

	bool							HasGameSession() const { return gameSession != NULL; }
	bool							NeedsGameSession() const { return networkSystem->IsDedicated() && !networkSystem->IsLANServer(); }
	bool							StartGameSession();
	bool							UpdateGameSession( bool checkDict, bool throttle );
	bool							StopGameSession( bool blocking = false );

	void							ServerClientConnect( const int clientNum );
	void							ServerClientDisconnect( const int clientNum );

	void							SendSessionId( const int clientNum = -1 ) const;
	void							ProcessSessionId( const idBitMsg& msg );

#if !defined( SD_DEMO_BUILD )
	void							FlushStats( bool blocking = false );
#endif /* !SD_DEMO_BUILD */

	void							PerformCommand( const idCmdArgs& cmd );

	sdNetSession*					GetSession( float fSource, int index );

	void							UpdateServer( sdUIList& list, const char* sessionName, findServerSource_e source );
#if !defined( SD_DEMO_BUILD )
	bool							AnyFriendsOnServer( const sdNetSession& netSession ) const;
	void							AddFriendServerToHash( const char* server );
	void							CacheServersWithFriends();

	bool							ShowRanked() const;
#endif /* !SD_DEMO_BUILD */

	bool							SessionIsFiltered( const sdNetSession& netSession, bool ignoreEmptyFilter = false ) const;

private:
	struct task_t {
					task_t() :
						task( NULL ),
						allowCancel( true ),
						completed( NULL ),
						parm( NULL ) {
					}

		bool		Set( sdNetTask* task, void (sdNetManager::*completed)( sdNetTask* task, void* parm ) = NULL, void* parm = NULL ) {
						if ( task == NULL ) {
							return false;
						}
						this->task = task;
						this->completed = completed;
						this->parm = parm;

						return true;
					}
		void		Cancel() {
						if ( task == NULL ) {
							return;
						}
						if ( allowCancel ) {
							task->Cancel( true );
						} else {
							while ( task->GetState() != sdNetTask::TS_COMPLETING );
						}
					}
		void		OnCompleted( sdNetManager* manager ) {
						if ( completed != NULL ) {
							CALL_MEMBER_FN_PTR( manager, completed )( task, parm );
						}
						task = NULL;
						parm = NULL;
					}

		sdNetTask*	task;
		bool		allowCancel;
		void		(sdNetManager::*completed)( sdNetTask* task, void* parm );
		void*		parm;
	};

	static void						InitFunctions();
	static void						ShutdownFunctions();
	static uiFunction_t*			FindFunction( const char* name );

	void							OnConnect( sdNetTask* task, void* parm );
	void							OnSignInDedicated( sdNetTask* task, void* parm );

	void							OnGameSessionCreated( sdNetTask* task, void* parm );
	void							OnGameSessionStopped( sdNetTask* task, void* parm );

#if !defined( SD_DEMO_BUILD )
	void							OnFriendInvited( sdNetTask* task, void* parm );
	void							OnMemberInvited( sdNetTask* task, void* parm );
#endif /* !SD_DEMO_BUILD */

	sdNetUser*						FindUser( const char* user );

	task_t*							GetTaskSlot();

	void							Script_Connect( sdUIFunctionStack& stack );

	void							Script_ValidateUsername( sdUIFunctionStack& stack );
	void							Script_MakeRawUsername( sdUIFunctionStack& stack );

	void							Script_ValidateEmail( sdUIFunctionStack& stack );

	void							Script_CreateUser( sdUIFunctionStack& stack );
	void							Script_DeleteUser( sdUIFunctionStack& stack );
	void							Script_ActivateUser( sdUIFunctionStack& stack );
	void							Script_DeactivateUser( sdUIFunctionStack& stack );
	void							Script_SaveUser( sdUIFunctionStack& stack );
	void							Script_SetDefaultUser( sdUIFunctionStack& stack );

#if !defined( SD_DEMO_BUILD )
	void							Script_CreateAccount( sdUIFunctionStack& stack );
	void							Script_DeleteAccount( sdUIFunctionStack& stack );
	void							Script_HasAccount( sdUIFunctionStack& stack );

	void							Script_AccountSetUsername( sdUIFunctionStack& stack );
	void							Script_AccountIsPasswordSet( sdUIFunctionStack& stack );
	void							Script_AccountSetPassword( sdUIFunctionStack& stack );
	void							Script_AccountChangePassword( sdUIFunctionStack& stack );
	void							Script_AccountResetPassword( sdUIFunctionStack& stack );
#endif /* !SD_DEMO_BUILD */

	void							Script_SignIn( sdUIFunctionStack& stack );
	void							Script_SignOut( sdUIFunctionStack& stack );

	void							Script_GetProfileString( sdUIFunctionStack& stack );
	void							Script_SetProfileString( sdUIFunctionStack& stack );
#if !defined( SD_DEMO_BUILD )
	void							Script_AssureProfileExists( sdUIFunctionStack& stack );
	void							Script_StoreProfile( sdUIFunctionStack& stack );
	void							Script_RestoreProfile( sdUIFunctionStack& stack );
#endif /* !SD_DEMO_BUILD */
	void							Script_ValidateProfile( sdUIFunctionStack& stack );

#if !defined( SD_DEMO_BUILD )
	void							Script_IsFriend( sdUIFunctionStack& stack );
	void							Script_IsPendingFriend( sdUIFunctionStack& stack );
	void							Script_IsInvitedFriend( sdUIFunctionStack& stack );
	void							Script_IsFriendOnServer( sdUIFunctionStack& stack );
	void							Script_FollowFriendToServer( sdUIFunctionStack& stack );
	void							Script_GetFriendServerIP( sdUIFunctionStack& stack );
#endif /* !SD_DEMO_BUILD */
	void							Script_IsSelfOnServer( sdUIFunctionStack& stack );
	void							Script_JoinBestServer( sdUIFunctionStack& stack );
	void							Script_GetNumHotServers( sdUIFunctionStack& stack );

	void							Script_FormatSessionInfo( sdUIFunctionStack& stack );

	void							Script_FindServers( sdUIFunctionStack& stack );	
	void							Script_RefreshHotServers( sdUIFunctionStack& stack );
	void							Script_UpdateHotServers( sdUIFunctionStack& stack );
	void							Script_GetNumInterestedInServer( sdUIFunctionStack& stack );
	void							Script_RefreshServer( sdUIFunctionStack& stack );
	void							Script_RefreshCurrentServers( sdUIFunctionStack& stack );
	void							Script_StopFindingServers( sdUIFunctionStack& stack );
	void							Script_AddUnfilteredSession( sdUIFunctionStack& stack );
	void							Script_ClearUnfilteredSessions( sdUIFunctionStack& stack );
	void							Script_JoinServer( sdUIFunctionStack& stack );
	void							Script_QueryServerInfo( sdUIFunctionStack& stack );
	void							Script_QueryMapInfo( sdUIFunctionStack& stack );
	void							Script_QueryGameType( sdUIFunctionStack& stack );
	void							Script_QueryXPStats( sdUIFunctionStack& stack );
	void							Script_QueryXPTotals( sdUIFunctionStack& stack );

#if !defined( SD_DEMO_BUILD )
	void							Script_CheckKey( sdUIFunctionStack& stack );

	void							Script_Friend_Init( sdUIFunctionStack& stack );
	void							Script_Friend_ProposeFriendShip( sdUIFunctionStack& stack );
	void							Script_Friend_AcceptProposal( sdUIFunctionStack& stack );
	void							Script_Friend_RejectProposal( sdUIFunctionStack& stack );
	void							Script_Friend_RemoveFriend( sdUIFunctionStack& stack );
	void							Script_Friend_SetBlockedStatus( sdUIFunctionStack& stack );
	void							Script_Friend_GetBlockedStatus( sdUIFunctionStack& stack );
	void							Script_Friend_SendIM( sdUIFunctionStack& stack );
	void							Script_Friend_ChooseContextAction( sdUIFunctionStack& stack );
	void							Script_Friend_NumAvailableIMs( sdUIFunctionStack& stack );
	void							Script_DeleteActiveMessage( sdUIFunctionStack& stack );
	void							Script_Friend_GetIMText( sdUIFunctionStack& stack );
	void							Script_Friend_GetProposalText( sdUIFunctionStack& stack );
	void							Script_Friend_InviteFriend( sdUIFunctionStack& stack );
	void							Script_ClearActiveMessage( sdUIFunctionStack& stack );
	void							Script_GetServerInviteIP( sdUIFunctionStack& stack );

	void							Script_Team_Init( sdUIFunctionStack& stack );

	void							Script_Team_CreateTeam( sdUIFunctionStack& stack );
	void							Script_Team_ProposeMembership( sdUIFunctionStack& stack );
	void							Script_Team_AcceptMembership( sdUIFunctionStack& stack );
	void							Script_Team_RejectMembership( sdUIFunctionStack& stack );
	void							Script_Team_RemoveMember( sdUIFunctionStack& stack );
	void							Script_Team_SendMessage( sdUIFunctionStack& stack );
	void							Script_Team_BroadcastMessage( sdUIFunctionStack& stack );
	void							Script_Team_Invite( sdUIFunctionStack& stack );
	void							Script_Team_PromoteMember( sdUIFunctionStack& stack );
	void							Script_Team_DemoteMember( sdUIFunctionStack& stack );
	void							Script_Team_TransferOwnership( sdUIFunctionStack& stack );
	void							Script_Team_DisbandTeam( sdUIFunctionStack& stack );
	void							Script_Team_LeaveTeam( sdUIFunctionStack& stack );
	void							Script_Team_ChooseContextAction( sdUIFunctionStack& stack );
	void							Script_Team_IsMember( sdUIFunctionStack& stack );
	void							Script_Team_IsPendingMember( sdUIFunctionStack& stack );
	void							Script_Team_GetIMText ( sdUIFunctionStack& stack );
	void							Script_Team_NumAvailableIMs( sdUIFunctionStack& stack );
	void							Script_Team_GetMemberStatus( sdUIFunctionStack& stack );
	void							Script_IsMemberOnServer( sdUIFunctionStack& stack );
	void							Script_FollowMemberToServer( sdUIFunctionStack& stack );
	void							Script_GetMemberServerIP( sdUIFunctionStack& stack );
#endif /* !SD_DEMO_BUILD */

	void							Script_RequestStats( sdUIFunctionStack& stack );
	void							Script_GetStat( sdUIFunctionStack& stack );
	void							Script_GetPlayerAward( sdUIFunctionStack& stack );

	void							Script_ApplyNumericFilter( sdUIFunctionStack& stack );
	void							Script_ApplyStringFilter( sdUIFunctionStack& stack );

	void							Script_ClearFilters( sdUIFunctionStack& stack );

	void							Script_SaveFilters( sdUIFunctionStack& stack );
	void							Script_LoadFilters( sdUIFunctionStack& stack );

	void							Script_QueryNumericFilterValue( sdUIFunctionStack& stack );
	void							Script_QueryNumericFilterState( sdUIFunctionStack& stack );
	void							Script_QueryStringFilterState( sdUIFunctionStack& stack );
	void							Script_QueryStringFilterValue( sdUIFunctionStack& stack );
	void							Script_JoinSession( sdUIFunctionStack& stack );
	void							Script_GetMotdString( sdUIFunctionStack& stack );

	void							Script_GetPlayerCount( sdUIFunctionStack& stack );
	void							Script_GetServerStatusString( sdUIFunctionStack& stack );

	void							Script_ToggleServerFavoriteState( sdUIFunctionStack& stack );
	void							Script_GetMessageTimeStamp( sdUIFunctionStack& stack );
	void							Script_CancelActiveTask( sdUIFunctionStack& stack );

#if !defined( SD_DEMO_BUILD )
	void							Script_AddToMessageHistory( sdUIFunctionStack& stack );
	void							Script_LoadMessageHistory( sdUIFunctionStack& stack );
	void							Script_UnloadMessageHistory( sdUIFunctionStack& stack );
	void							Script_GetUserNamesForKey( sdUIFunctionStack& stack );
#endif /* !SD_DEMO_BUILD */

	void							GetSessionsForServerSource( findServerSource_e source, idList< sdNetSession* >*& netSessions, sdNetTask*& task, sdHotServerList*& netHotServers );
	const idDict*					GetMapInfo( const sdNetSession& netSession );
	void							GetGameType( const char* siRules, idWStr& type );

	void							StopFindingServers( findServerSource_e source );
	void							UpdateSession( sdUIList& list, const sdNetSession& netSession, int index );

	void							CancelUserTasks();
	bool							DoFiltering( const sdNetSession& netSession ) const;
	

private:
	static idHashMap< uiFunction_t* >	uiFunctions;

	static const int					MAX_ACTIVE_TASKS = 4;
	static const int					SESSION_UPDATE_INTERVAL = 10 * 60 * 1000;

	sdNetProperties						properties;

	task_t								activeTasks[MAX_ACTIVE_TASKS];
	task_t								activeTask;

	sdNetMessage*						activeMessage;

	sdNetTask*							findServersTask;
	sdNetTask*							findRepeatersTask;
	sdNetTask*							findLANServersTask;
	sdNetTask*							findLANRepeatersTask;
	sdNetTask*							findHistoryServersTask;
	sdNetTask*							findFavoriteServersTask;
	sdNetTask*							refreshServerTask;
	sdNetTask*							initFriendsTask;
	sdNetTask*							initTeamsTask;
	sdNetTask*							refreshHotServerTask;

	idList< sdNetSession* >				sessions;
	idList< sdNetSession* >				sessionsLAN;
	idList< sdNetSession* >				sessionsHistory;
	idList< sdNetSession* >				sessionsFavorites;
	idList< sdNetSession* >				sessionsRepeaters;
	idList< sdNetSession* >				sessionsLANRepeaters;

	sdHotServerList						hotServers;
	sdHotServerList						hotServersLAN;
	sdHotServerList						hotServersHistory;
	sdHotServerList						hotServersFavorites;

	sdNetSession*						gameSession;
										// we make a copy of the session to refresh to avoid problems if the user causes the current session list to invalidate
	sdNetSession*						serverRefreshSession;

	idList< sdNetSession* >				hotServerRefreshSessions;
	int									lastSessionUpdateTime;

	findServerSource_e					serverRefreshSource;
	findServerSource_e					hotServersRefreshSource;

	sdNetSession::sessionId_t			sessionId;

	const sdDeclStringMap*				gameTypeNames;
	const sdDeclLocStr*					offlineString;
	const sdDeclLocStr*					infinityString;

	idStaticList< serverNumericFilter_t, 16 >	numericFilters;
	idStaticList< serverStringFilter_t, 8 >		stringFilters;

	int									lastServerUpdateIndex;

	idList< netadr_t >					unfilteredSessions;
	
	struct sessionIndices_t {
		int sessionListIndex;
		int uiListIndex;
		int lastUpdateTime;
	};
	typedef sdHashMapGeneric< idStr, sessionIndices_t, sdHashCompareStrIcmp, sdHashGeneratorIHash > sessionHash_t;
	sessionHash_t						hashedSessions;

	idHashIndexUShort					serversWithFriendsHash;
	idStrList							serversWithFriends;

	mutable sdStringBuilder_Heap		builder;	// use this for any temporary work
	mutable idWStr						tempWStr;	// use this for any temporary work

	idStrList							retrievedAccountNames;
};

ID_INLINE sdNetManager::task_t* sdNetManager::GetTaskSlot() {
	for ( int i = 0; i < MAX_ACTIVE_TASKS; i++ ) {
		if ( activeTasks[ i ].task == NULL ) {
			activeTasks[ i ].allowCancel = false;
			return &activeTasks[ i ];
		}
	}
	return NULL;
}

#endif /* !__GAME_SDNETMANAGER_H__ */
