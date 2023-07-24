// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "SDNetManager.h"

#include "../guis/UserInterfaceLocal.h"

#include "../../sdnet/SDNet.h"
#include "../../sdnet/SDNetTask.h"
#include "../../sdnet/SDNetUser.h"
#include "../../sdnet/SDNetAccount.h"
#include "../../sdnet/SDNetProfile.h"
#include "../../sdnet/SDNetSession.h"
#include "../../sdnet/SDNetMessage.h"
#include "../../sdnet/SDNetMessageHistory.h"
#include "../../sdnet/SDNetSessionManager.h"
#include "../../sdnet/SDNetStatsManager.h"
#include "../../sdnet/SDNetFriendsManager.h"
#include "../../sdnet/SDNetTeamManager.h"

#include "../proficiency/StatsTracker.h"
#include "../structures/TeamManager.h"
#include "../rules/GameRules.h"

#include "../guis/UIList.h"

#include "../../framework/Licensee.h"

#include "../../decllib/declTypeHolder.h"

idHashMap< sdNetManager::uiFunction_t* > sdNetManager::uiFunctions;

idCVar net_accountName( "net_accountName", "", CVAR_INIT, "Auto login account name" );
idCVar net_accountPassword( "net_accountPassword", "", CVAR_INIT, "Auto login account password" );
idCVar net_autoConnectServer( "net_autoConnectServer", "", CVAR_INIT, "Server to connect to after auto login is complete" );


/*
================
sdHotServerList::Clear
================
*/
void sdHotServerList::Clear( void ) {
	for ( int i = 0; i < MAX_HOT_SERVERS; i++ ) {
		hotServerIndicies[ i ] = -1;
	}
	nextInterestMessageTime = 0;
}

/*
================
sdHotServerList::IsServerValid
================
*/
bool sdHotServerList::IsServerValid( const sdNetSession& session, sdNetManager& manager ) {
	const int MIN_SENSIBLE_PLAYER_LIMIT		= 16;

	if ( manager.SessionIsFiltered( session, true ) ) {
		return false;
	}

	if( session.IsRepeater() ) {
		return false;
	}

	if ( session.GetServerInfo().GetBool( "si_needPass" ) ) {
		return false;
	}

	int maxPlayers = session.GetServerInfo().GetInt( "si_maxPlayers" );
	if ( maxPlayers < MIN_SENSIBLE_PLAYER_LIMIT ) {
		return false; // Gordon: ignore servers with silly player limit
	}
	if ( session.GetNumClients() == maxPlayers ) {
		return false;
	}

	return true;
}

/*
================
sdHotServerList::Update
================
*/
void sdHotServerList::Update( sdNetManager& manager ) {
	for ( int i = 0; i < MAX_HOT_SERVERS; ) {
		if ( hotServerIndicies[ i ] == -1 ) {
			i++;
			continue;
		}

		if ( !IsServerValid( *sessions[ hotServerIndicies[ i ] ], manager ) ) {
			hotServerIndicies[ i ] = -1;
			if ( i < MAX_HOT_SERVERS - 1 ) {
				Swap( hotServerIndicies[ i ], hotServerIndicies[ i + 1 ] );
			}
		} else {
			i++;
		}
	}

	int offset;
	for ( offset = 0; offset < MAX_HOT_SERVERS; offset++ ) {
		if ( hotServerIndicies[ offset ] == -1 ) {
			break;
		}
	}
	if ( offset != MAX_HOT_SERVERS ) {
		CalcServerScores( manager, &hotServerIndicies[ offset ], MAX_HOT_SERVERS - offset );
	}

	int now = sys->Milliseconds();
	if ( now > nextInterestMessageTime ) {
		SendServerInterestMessages();
		nextInterestMessageTime = now + SEC2MS( 10.f );
	}
}

/*
================
sdHotServerList::GetServerScore
================
*/
int sdHotServerList::GetServerScore( const sdNetSession& session, sdNetManager& manager ) {
	const int MIN_GOOD_PLAYER_COUNT			= 4;
	const int MAX_GOOD_PLAYER_COUNT			= 12;

	const int GOOD_PING_MAX					= 50;
	const int OK_PING_MAX					= 100;

	const int POTENTIAL_PLAYERS_BONUS		= 1;
	const int RANKED_BONUS					= 20;

	const int MAX_POTENTIAL_PLAYERS_BONUS	= 15;

	if ( !IsServerValid( session, manager ) ) {
		return 0;
	}

	int score = 0;

	int numPlayers = session.GetNumClients();
	if ( numPlayers < MIN_GOOD_PLAYER_COUNT ) {
		score += BROWSER_OK_BONUS;
	} else if ( numPlayers < MAX_GOOD_PLAYER_COUNT ) {
		score += BROWSER_GOOD_BONUS;
	}

	int ping = session.GetPing();
	if ( ping < GOOD_PING_MAX ) {
		score += BROWSER_GOOD_BONUS * 2;
	} else if ( ping < OK_PING_MAX ) {
		score += BROWSER_OK_BONUS * 2;
	}

	if ( /* manager.PreferRanked() && */ session.IsRanked() ) {
		score += RANKED_BONUS;
	}

	int playerBonus = session.GetNumInterestedClients();
	if ( playerBonus > 128 ) { // Server is trying to be naughty, ignore it completely
		return 0;
	}
	if ( playerBonus > MAX_POTENTIAL_PLAYERS_BONUS ) {
		playerBonus = MAX_POTENTIAL_PLAYERS_BONUS;
	}
	score += playerBonus;

	sdGameRules* rules = gameLocal.GetRulesInstance( session.GetServerInfo().GetString( "si_rules" ) );
	if ( rules != NULL ) {
		score += rules->GetServerBrowserScore( session );
	}

	return score;
}

idCVar g_debugHotServers( "g_debugHotServers", "0", CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT, "" );

/*
================
sdHotServerList::CalcServerScores
================
*/
void sdHotServerList::CalcServerScores( sdNetManager& manager, int* bestServers, int numBestServers ) {
	int* bestScores = ( int* )_alloca( sizeof( int ) * numBestServers );
	for ( int i = 0; i < numBestServers; i++ ) {
		bestScores[ i ] = 0;
	}

	for ( int i = 0; i < sessions.Num(); i++ ) {
		int check;
		for ( check = 0; check < MAX_HOT_SERVERS; check++ ) {
			if ( hotServerIndicies[ check ] == i ) {
				break;
			}
		}
		if ( check != MAX_HOT_SERVERS ) {
			continue;
		}

		int score = GetServerScore( *sessions[ i ], manager );

		const char* name = sessions[ i ]->GetServerInfo().GetString( "si_name" );
		if ( g_debugHotServers.GetBool() ) {
			gameLocal.Printf( "Server '%s' Score: %d Interested: %d\n", name, score, sessions[ i ]->GetNumInterestedClients() );
		}

		for ( int j = 0; j < numBestServers; j++ ) {
			if ( score <= bestScores[ j ] ) {
				continue;
			}

			for ( int k = numBestServers - 1; k > j; k-- ) {
				Swap( bestScores[ k ], bestScores[ k - 1 ] );
				Swap( bestServers[ k ], bestServers[ k - 1 ] );
			}

			bestScores[ j ] = score;
			bestServers[ j ] = i;
			break;
		}
	}

/*	idFile* hotServersLog = fileSystem->OpenFileWrite( "hotserver.log" );
	if ( hotServersLog != NULL ) {
		for ( int i = 0; i < numBestServers; i++ ) {
			if ( bestServers[ i ] == -1 ) {
				break;
			}

			hotServersLog->WriteFloatString( "Hot Server %d: score %d: %s\n", i, bestScores[ i ], sessions[ bestServers[ i ] ]->GetServerInfo().GetString( "si_name" ) );
		}
		fileSystem->CloseFile( hotServersLog );
	}*/
}

/*
================
sdHotServerList::Locate
================
*/
void sdHotServerList::Locate( sdNetManager& manager ) {
	Clear();
	CalcServerScores( manager, hotServerIndicies, MAX_HOT_SERVERS );
	SendServerInterestMessages();
}

/*
================
sdHotServerList::SendServerInterestMessages
================
*/
void sdHotServerList::SendServerInterestMessages( void ) {
	for ( int i = 0; i < MAX_HOT_SERVERS; i++ ) {
		if ( hotServerIndicies[ i ] == -1 ) {
			break;
		}
		networkSystem->RegisterServerInterest( sessions[ hotServerIndicies[ i ] ]->GetAddress() );
	}
}

/*
================
sdHotServerList::GetNumServers
================
*/
int sdHotServerList::GetNumServers( void ) {
	return MAX_HOT_SERVERS;
}

/*
================
sdHotServerList::GetServer
================
*/
sdNetSession* sdHotServerList::GetServer( int index ) {
	if ( hotServerIndicies[ index ] == -1 ) {
		return NULL;
	}
	return sessions[ hotServerIndicies[ index ] ];
}

/*
================
sdNetManager::sdNetManager
================
*/
sdNetManager::sdNetManager() :
	activeMessage( NULL ),
	refreshServerTask( NULL ),
	findServersTask( NULL ),
	findRepeatersTask( NULL ),
	findLANRepeatersTask( NULL ),
	findHistoryServersTask( NULL ),
	findFavoriteServersTask( NULL ),
	findLANServersTask( NULL ),
	refreshHotServerTask( NULL ),
	gameSession( NULL ),
	lastSessionUpdateTime( -1 ),
	gameTypeNames( NULL ),
	serverRefreshSession( NULL ),
	initFriendsTask( NULL ),
	initTeamsTask( NULL ),
	hotServers( sessions ),
	hotServersLAN( sessionsLAN ),
	hotServersHistory( sessionsHistory ),
	hotServersFavorites( sessionsFavorites ) {
}

/*
================
sdNetManager::Init
================
*/
void sdNetManager::Init() {
	properties.Init();

	gameTypeNames = gameLocal.declStringMapType.LocalFind( "game/rules/names" );
	offlineString = declHolder.declLocStrType.LocalFind( "guis/mainmenu/offline" );
	infinityString = declHolder.declLocStrType.LocalFind( "guis/mainmenu/infinity" );

	// Initialize functions
	InitFunctions();

	// Connect to the master
	Connect();
}


/*
============
sdNetManager::CancelUserTasks
============
*/
void sdNetManager::CancelUserTasks() {
	if ( findServersTask != NULL ) {
		findServersTask->Cancel( true );
		networkService->FreeTask( findServersTask );
		findServersTask = NULL;
	}

	if ( findLANServersTask != NULL ) {
		findLANServersTask->Cancel( true );
		networkService->FreeTask( findLANServersTask );
		findLANServersTask = NULL;
	}

	if ( refreshServerTask != NULL ) {
		refreshServerTask->Cancel( true );
		networkService->FreeTask( refreshServerTask );
		refreshServerTask = NULL;
	}
	if ( refreshHotServerTask != NULL ) {
		refreshHotServerTask->Cancel( true );
		networkService->FreeTask( refreshHotServerTask );
		refreshHotServerTask = NULL;
	}

	if ( findHistoryServersTask != NULL ) {
		findHistoryServersTask->Cancel( true );
		networkService->FreeTask( findHistoryServersTask );
		findHistoryServersTask = NULL;
	}

	if ( findFavoriteServersTask != NULL ) {
		findFavoriteServersTask->Cancel( true );
		networkService->FreeTask( findFavoriteServersTask );
		findFavoriteServersTask = NULL;
	}

	if ( initTeamsTask != NULL ) {
		initTeamsTask->Cancel( true );
		networkService->FreeTask( initTeamsTask );
		initTeamsTask = NULL;
	}

	if ( initFriendsTask != NULL ) {
		initFriendsTask->Cancel( true );
		networkService->FreeTask( initFriendsTask );
		initFriendsTask = NULL;
	}
}

/*
================
sdNetManager::Connect
================
*/
bool sdNetManager::Connect() {
	assert( networkService->GetState() == sdNetService::SS_INITIALIZED );

	task_t* activeTask = GetTaskSlot();
	if ( activeTask == NULL ) {
		return false;
	}

	return ( activeTask->Set( networkService->Connect(), &sdNetManager::OnConnect ) );
}

/*
================
sdNetManager::OnConnect
================
*/
void sdNetManager::OnConnect( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() != SDNET_NO_ERROR ) {
		properties.SetTaskResult( task->GetErrorCode(), declHolder.FindLocStr( va( "sdnet/error/%d", task->GetErrorCode() ) ) );
		properties.ConnectFailed();
	}
}

/*
================
sdNetManager::Shutdown
================
*/
void sdNetManager::Shutdown() {

	// Cancel any outstanding tasks
	for ( int i = 0; i < MAX_ACTIVE_TASKS; i++ ) {
		sdNetTask* activeTask = activeTasks[ i ].task;
		if ( activeTask != NULL ) {
			activeTasks[ i ].Cancel();
			activeTasks[ i ].OnCompleted( this );
			networkService->FreeTask( activeTask );
		}
	}

	if ( activeTask.task != NULL ) {
		activeTask.Cancel();
		activeTask.OnCompleted( this );
		networkService->FreeTask( activeTask.task );
	}

	activeMessage = NULL;

	CancelUserTasks();

	if ( serverRefreshSession != NULL ) {
		networkService->GetSessionManager().FreeSession( serverRefreshSession );
		serverRefreshSession = NULL;
	}

	for ( int i = 0; i < hotServerRefreshSessions.Num(); i++ ) {
		networkService->GetSessionManager().FreeSession( hotServerRefreshSessions[ i ] );
	}
	hotServerRefreshSessions.Clear();

	StopGameSession( true );

	// free outstanding server browser sessions
	for ( int i = 0; i < sessions.Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( sessions[ i ] );
	}
	sessions.Clear();

	for ( int i = 0; i < sessionsLAN.Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( sessionsLAN[ i ] );
	}
	sessionsLAN.Clear();

	for ( int i = 0; i < sessionsLANRepeaters.Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( sessionsLANRepeaters[ i ] );
	}
	sessionsLANRepeaters.Clear();

	for ( int i = 0; i < sessionsRepeaters.Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( sessionsRepeaters[ i ] );
	}
	sessionsRepeaters.Clear();


	for ( int i = 0; i < sessionsHistory.Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( sessionsHistory[ i ] );
	}
	sessionsHistory.Clear();

	for ( int i = 0; i < sessionsFavorites.Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( sessionsFavorites[ i ] );
	}
	sessionsFavorites.Clear();

	tempWStr.Clear();
	builder.Clear();

	properties.Shutdown();
	ShutdownFunctions();
}

/*
================
sdNetManager::InitFunctions
================
*/

#define ALLOC_FUNC( name, returntype, parms, function ) uiFunctions.Set( name, new uiFunction_t( returntype, parms, function ) )
#pragma inline_depth( 0 )
#pragma optimize( "", off )
SD_UI_PUSH_CLASS_TAG( sdNetProperties )
void sdNetManager::InitFunctions() {
	SD_UI_FUNC_TAG( connect, "Connecting to online service." )
		SD_UI_FUNC_RETURN_PARM( float, "True if connecting." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "connect",					'f', "",		&sdNetManager::Script_Connect );

	SD_UI_FUNC_TAG( validateUsername, "Validate the user name before creating an account." )
		SD_UI_FUNC_PARM( string, "username", "Username to be validated." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns valid/empty name/duplicate name/invalid name. See Username Validation defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "validateUsername",			'f', "s",		&sdNetManager::Script_ValidateUsername );

	SD_UI_FUNC_TAG( makeRawUsername, "Makes a raw username from the given username." )
		SD_UI_FUNC_PARM( string, "username", "Username to convert." )
		SD_UI_FUNC_RETURN_PARM( string, "Valid formatting for an account name." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "makeRawUsername",			's', "s",		&sdNetManager::Script_MakeRawUsername );

	SD_UI_FUNC_TAG( validateEmail, "Validate an email address." )
		SD_UI_FUNC_PARM( string, "email", "Email address." )
		SD_UI_FUNC_PARM( string, "confirmEmail", "Confirmation of email address." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns valid/empty name/duplicate name/invalid name. See Email Validation defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "validateEmail",			'f', "ss",		&sdNetManager::Script_ValidateEmail );

	SD_UI_FUNC_TAG( createUser, "Create a user." )
		SD_UI_FUNC_PARM( string, "username", "Username to create." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "createUser",				'v', "s",		&sdNetManager::Script_CreateUser );

	SD_UI_FUNC_TAG( deleteUser, "Delete a user. User should be logged out before trying to delete the account." )
		SD_UI_FUNC_PARM( string, "username", "username for user to delete." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "deleteUser",				'v', "s",		&sdNetManager::Script_DeleteUser );

	SD_UI_FUNC_TAG( activateUser, "Log in as a user. No user should be logged in before calling this. Executes the users config." )
		SD_UI_FUNC_PARM( string, "username", "Username for user to log in." )
		SD_UI_FUNC_RETURN_PARM( float, "True if user activated." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "activateUser",				'f', "s",		&sdNetManager::Script_ActivateUser );

	SD_UI_FUNC_TAG( deactivateUser, "Log out, a user should be logged in before calling this." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "deactivateUser",			'v', "",		&sdNetManager::Script_DeactivateUser );

	SD_UI_FUNC_TAG( saveUser, "Save user profile, config and bindings." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "saveUser",					'v', "",		&sdNetManager::Script_SaveUser );

	SD_UI_FUNC_TAG( setDefaultUser, "Sets a default user when starting up, skips login screen." )
		SD_UI_FUNC_PARM( string, "username", "Username for user." )
		SD_UI_FUNC_PARM( float, "set", "True if setting the username as default." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "setDefaultUser",			'v', "sf",		&sdNetManager::Script_SetDefaultUser );

	SD_UI_FUNC_TAG( getNumInterestedInServer, "Sets a default user when starting up, skips login screen." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( string, "address", "Address of the server." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns the number of players that are interested in joining a server." )
		SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getNumInterestedInServer",			'f', "fs",		&sdNetManager::Script_GetNumInterestedInServer );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( createAccount, "Create an account." )
		SD_UI_FUNC_PARM( string, "username", "Username for user." )
		SD_UI_FUNC_PARM( string, "password", "Password for account." )
		SD_UI_FUNC_PARM( string, "keyCode", "Valid Quake Wars key code." )
		SD_UI_FUNC_RETURN_PARM( float, "True if creating the account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "createAccount",			'f', "sss",		&sdNetManager::Script_CreateAccount );

	SD_UI_FUNC_TAG( deleteAccount, "Delete an account." )
		SD_UI_FUNC_PARM( string, "password", "Password for account to delete." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "deleteAccount",			'v', "s",		&sdNetManager::Script_DeleteAccount );

	SD_UI_FUNC_TAG( hasAccount, "Has account." )
		SD_UI_FUNC_RETURN_PARM( float, "True if logged in to an account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "hasAccountImmediate",				'f', "",		&sdNetManager::Script_HasAccount );

	SD_UI_FUNC_TAG( accountSetUsername, "Set a username for an account." )
		SD_UI_FUNC_PARM( string, "username", "Username to set." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "accountSetUsername",		'v', "s",		&sdNetManager::Script_AccountSetUsername );

	SD_UI_FUNC_TAG( accountPasswordSet, "Check if account requires a password." )
		SD_UI_FUNC_RETURN_PARM( float, "True if account has a password associated with it." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "accountPasswordSet",		'f', "",		&sdNetManager::Script_AccountIsPasswordSet );

	SD_UI_FUNC_TAG( accountSetPassword, "Set the password for an account." )
		SD_UI_FUNC_PARM( string, "password", "New password for the account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "accountSetPassword",		'v', "s",		&sdNetManager::Script_AccountSetPassword );

	SD_UI_FUNC_TAG( accountResetPassword, "Reset an account password." )
		SD_UI_FUNC_PARM( string, "keyCode", "Key code associated with the account." )
		SD_UI_FUNC_PARM( string, "newPassword", "New password for the account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "accountResetPassword",		'v', "ss",		&sdNetManager::Script_AccountResetPassword );

	SD_UI_FUNC_TAG( accountChangePassword, "Change account password." )
		SD_UI_FUNC_PARM( string, "password", "Password for the account." )
		SD_UI_FUNC_PARM( string, "newPassword", "New password for the account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "accountChangePassword",	'v', "ss",		&sdNetManager::Script_AccountChangePassword );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( signIn, "Sign in to demonware with the current account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "signIn",					'v', "",		&sdNetManager::Script_SignIn );

	SD_UI_FUNC_TAG( signOut, "Sign out of demonware." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "signOut",					'v', "",		&sdNetManager::Script_SignOut );

	SD_UI_FUNC_TAG( getProfileString, "Get a key/val." )
		SD_UI_FUNC_PARM( string, "key", "Key." )
		SD_UI_FUNC_PARM( string, "defaultValue", "Default value." )
		SD_UI_FUNC_RETURN_PARM( string, "." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getProfileString",			's', "ss",		&sdNetManager::Script_GetProfileString );

	SD_UI_FUNC_TAG( setProfileString, "Set a key/val." )
		SD_UI_FUNC_PARM( string, "key", "Key." )
		SD_UI_FUNC_PARM( string, "value", "Value." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "setProfileString",			'v', "ss",		&sdNetManager::Script_SetProfileString );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( assureProfileExists, "Make sure profile exists at demonware." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "assureProfileExists",		'v', "",		&sdNetManager::Script_AssureProfileExists );

	SD_UI_FUNC_TAG( storeProfile, "Store demonware profile." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "storeProfile",				'v', "",		&sdNetManager::Script_StoreProfile );

	SD_UI_FUNC_TAG( restoreProfile, "Restore demonware profile from demonware." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "restoreProfile",			'v', "",		&sdNetManager::Script_RestoreProfile );

#endif /* !SD_DEMO_BUILD */
	SD_UI_FUNC_TAG( validateProfile, "Validate user profile by requesting all profile properties." )
		SD_UI_FUNC_RETURN_PARM( float, "True on validated." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "validateProfile",			'f', "",		&sdNetManager::Script_ValidateProfile );

	SD_UI_FUNC_TAG( findServers, "Find servers." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "findServers",				'f', "f",		&sdNetManager::Script_FindServers );

	SD_UI_FUNC_TAG( addUnfilteredSession, "Add an unfiltered server with the specified IP:Port." )
		SD_UI_FUNC_PARM( string, "session", "Server IP:Port." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "addUnfilteredSession",		'v', "s",		&sdNetManager::Script_AddUnfilteredSession );

	SD_UI_FUNC_TAG( clearUnfilteredSessions, "Clear all unfiltered sessions." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "clearUnfilteredSessions",	'v', "",		&sdNetManager::Script_ClearUnfilteredSessions );

	SD_UI_FUNC_TAG( stopFindingServers, "Stop task for finding servers." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "stopFindingServers",		'v', "f",		&sdNetManager::Script_StopFindingServers );

	SD_UI_FUNC_TAG( refreshCurrentServers, "Refresh list of servers." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_RETURN_PARM( float, "True on success." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "refreshCurrentServers",	'f', "f",		&sdNetManager::Script_RefreshCurrentServers );

	SD_UI_FUNC_TAG( refreshServer, "Refresh a server." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( float, "index", "session index." )
		SD_UI_FUNC_PARM( string, "string", "Optional session as string (IP:Port)." )
		SD_UI_FUNC_RETURN_PARM( float, "True on success." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "refreshServer",			'f', "ffs",		&sdNetManager::Script_RefreshServer );

	SD_UI_FUNC_TAG( refreshHotServers, "Refresh all the hot servers." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( string, "string", "Optional session to ignore as string (IP:Port)." )
		SD_UI_FUNC_RETURN_PARM( float, "True on success." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "refreshHotServers",		'f', "fs",		&sdNetManager::Script_RefreshHotServers );

	SD_UI_FUNC_TAG( updateHotServers, "Updates the hot servers, removing any ones which are now invalid and finding new ones. Also tells the server the player is interested in them." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "updateHotServers",		'v', "f",		&sdNetManager::Script_UpdateHotServers );

	SD_UI_FUNC_TAG( joinBestServer, "Joins the server that is at the top of the hot server list." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "joinBestServer",		'v', "f",		&sdNetManager::Script_JoinBestServer );

	SD_UI_FUNC_TAG( getNumHotServers, "Gets the number of available hot servers" )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of hot servers." )
		SD_UI_END_FUNC_TAG
		ALLOC_FUNC( "getNumHotServers",		'f', "f",		&sdNetManager::Script_GetNumHotServers );

	SD_UI_FUNC_TAG( joinServer, "Join a server." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( float, "index", "session index." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "joinServer",				'v', "ff",		&sdNetManager::Script_JoinServer );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( checkKey, "Verify key against checksum." )
		SD_UI_FUNC_PARM( string, "keyCode", "Keycode." )
		SD_UI_FUNC_PARM( string, "checksum", "Checksum." )
		SD_UI_FUNC_RETURN_PARM( float, "True if keycode looks valid." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "checkKey",					'f', "ss",		&sdNetManager::Script_CheckKey );

	SD_UI_FUNC_TAG( initFriends, "Create friends task for initializing list of friends." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "initFriends",				'v', "",		&sdNetManager::Script_Friend_Init );

	SD_UI_FUNC_TAG( proposeFriendship, "Propose a friendship to another player." )
		SD_UI_FUNC_PARM( string, "username", "Username for player to propose friendship to." )
		SD_UI_FUNC_PARM( wstring, "reason", "Reason for proposing a friendship." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "proposeFriendship",		'v', "sw",		&sdNetManager::Script_Friend_ProposeFriendShip );

	SD_UI_FUNC_TAG( acceptProposal, "Accept proposal for friendship." )
		SD_UI_FUNC_PARM( string, "username", "Username for player that proposed a friendship." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "acceptProposal",			'v', "s",		&sdNetManager::Script_Friend_AcceptProposal );

	SD_UI_FUNC_TAG( rejectProposal, "Reject proposal for friendship." )
		SD_UI_FUNC_PARM( string, "username", "Username for player that proposed a friendship." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "rejectProposal",			'v', "s",		&sdNetManager::Script_Friend_RejectProposal );

	SD_UI_FUNC_TAG( removeFriend, "Remove friendship." )
		SD_UI_FUNC_PARM( string, "username", "Username for player that proposed a friendship." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "removeFriend",				'v', "s",		&sdNetManager::Script_Friend_RemoveFriend );

	SD_UI_FUNC_TAG( setBlockedStatus, "Set block status for a user." )
		SD_UI_FUNC_PARM( string, "username", "Username for player to set blocked status for." )
		SD_UI_FUNC_PARM( handle, "blockStatus", "See BS_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "setBlockedStatus",			'v', "si",		&sdNetManager::Script_Friend_SetBlockedStatus );

	SD_UI_FUNC_TAG( getBlockedStatus, "Get blocked status for a user. See BS_* for defines." )
		SD_UI_FUNC_PARM( string, "username", "Username for player to get blocked status for." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns blocked status." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getBlockedStatus",			'f', "s",		&sdNetManager::Script_Friend_GetBlockedStatus );

	SD_UI_FUNC_TAG( sendIM, "Send instant message." )
		SD_UI_FUNC_PARM( string, "username", "Username for player to send instant message to." )
		SD_UI_FUNC_PARM( wstring, "message", "Message to send." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "sendIM",					'v', "sw",		&sdNetManager::Script_Friend_SendIM );

	SD_UI_FUNC_TAG( getIMText, "Get instant message text from user." )
		SD_UI_FUNC_PARM( string, "username", "Friend to get instant message from." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Text." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getIMText",				'w', "s",		&sdNetManager::Script_Friend_GetIMText );

	SD_UI_FUNC_TAG( getProposalText, "Get text for friendship proposal." )
		SD_UI_FUNC_PARM( string, "username", "User to get text from." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Text." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getProposalText",			'w', "s",		&sdNetManager::Script_Friend_GetProposalText );

	SD_UI_FUNC_TAG( inviteFriend, "Invite a friend to a session." )
		SD_UI_FUNC_PARM( string, "username", "User to invite." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "inviteFriend",				'v', "s",		&sdNetManager::Script_Friend_InviteFriend );

	SD_UI_FUNC_TAG( isFriend, "Check if a user is a friend." )
		SD_UI_FUNC_PARM( string, "username", "User of friend." )
		SD_UI_FUNC_RETURN_PARM( float, "True if friend." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isFriend",					'f', "s",		&sdNetManager::Script_IsFriend );

	SD_UI_FUNC_TAG( isPendingFriend, "See if a user is a pending friend." )
		SD_UI_FUNC_PARM( string, "username", "User to check." )
		SD_UI_FUNC_RETURN_PARM( float, "True if friendship is pending." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isPendingFriend",			'f', "s",		&sdNetManager::Script_IsPendingFriend );

	SD_UI_FUNC_TAG( isInvitedFriend, "Check if invited to a session by a friend." )
		SD_UI_FUNC_PARM( string, "username", "Friend name." )
		SD_UI_FUNC_RETURN_PARM( float, "True if invited by friend." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isInvitedFriend",			'f', "s",		&sdNetManager::Script_IsInvitedFriend );

	SD_UI_FUNC_TAG( isFriendOnServer, "Check if a friend is on a server. If friend is on a server the server port:ip is stored in profile key currentServer." )
		SD_UI_FUNC_PARM( string, "username", "Friend name." )
		SD_UI_FUNC_RETURN_PARM( float, "True if friend is on a server." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isFriendOnServer",			'f', "s",		&sdNetManager::Script_IsFriendOnServer );

	SD_UI_FUNC_TAG( followFriendToServer, "Follow a friend to a server." )
		SD_UI_FUNC_PARM( string, "username", "Friend to follow." )
		SD_UI_FUNC_RETURN_PARM( float, "True if connecting." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "followFriendToServer",		'f', "s",		&sdNetManager::Script_FollowFriendToServer );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( isSelfOnServer, "Check if local player is on a server." )
		SD_UI_FUNC_RETURN_PARM( float, "True if on a server." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isSelfOnServer",			'f', "",		&sdNetManager::Script_IsSelfOnServer );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( loadMessageHistory, "Load history with a user." )
		SD_UI_FUNC_PARM( float, "source", "Source for message history. See MHS_* defines." )
		SD_UI_FUNC_PARM( string, "username", "User to get message history from." )
		SD_UI_FUNC_RETURN_PARM( float, "True if message history loaded." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "loadMessageHistory",		'f', "fs",		&sdNetManager::Script_LoadMessageHistory );

	SD_UI_FUNC_TAG( unloadMessageHistory, "Unload a previously loaded message history." )
		SD_UI_FUNC_PARM( float, "source", "Source for message history. See MHS_* defines." )
		SD_UI_FUNC_PARM( string, "username", "User to get message history from." )
		SD_UI_FUNC_RETURN_PARM( float, "True if message history unloaded." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "unloadMessageHistory",		'f', "fs",		&sdNetManager::Script_UnloadMessageHistory );

	SD_UI_FUNC_TAG( addToMessageHistory, "Add a message to the message history." )
		SD_UI_FUNC_PARM( float, "source", "Source for message history. See MHS_* defines." )
		SD_UI_FUNC_PARM( string, "username", "User from which message history is with." )
		SD_UI_FUNC_PARM( string, "fromUser", "Message from user." )
		SD_UI_FUNC_PARM( wstring, "message", "Message to add." )
		SD_UI_FUNC_RETURN_PARM( float, "True if message added to message history." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "addToMessageHistory",		'f', "fssw",	&sdNetManager::Script_AddToMessageHistory );

	SD_UI_FUNC_TAG( getUserNamesForKey, "Get usernames for keycode." )
		SD_UI_FUNC_PARM( string, "keyCode", "Keycode." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task was created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getUserNamesForKey",		'f', "s",		&sdNetManager::Script_GetUserNamesForKey );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( getPlayerCount, "Get number of players across all sessions (minus bots)." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of players." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getPlayerCount",			'f', "f",		&sdNetManager::Script_GetPlayerCount );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( chooseContextActionForFriend, "Get current context action for friend." )
		SD_UI_FUNC_PARM( string, "username", "Username for friend." )
		SD_UI_FUNC_RETURN_PARM( float, "Current context action. See FCA_*." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "chooseContextActionForFriend",	'f', "s",	&sdNetManager::Script_Friend_ChooseContextAction );

	SD_UI_FUNC_TAG( numAvailableIMs, "Get number of available instant messsages." )
		SD_UI_FUNC_PARM( string, "username", "Username of friend." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of available instant messages." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "numAvailableIMs",				'f', "s",	&sdNetManager::Script_Friend_NumAvailableIMs );

	SD_UI_FUNC_TAG( deleteActiveMessage, "Delete current active message." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "deleteActiveMessage",		'v', "",		&sdNetManager::Script_DeleteActiveMessage );

	SD_UI_FUNC_TAG( clearActiveMessage, "Clear active message." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "clearActiveMessage",		'v', "",		&sdNetManager::Script_ClearActiveMessage );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( queryServerInfo, "Query a server for key/val." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( float, "index", "Session index." )
		SD_UI_FUNC_PARM( string, "key", "Key to get." )
		SD_UI_FUNC_PARM( string, "defaultValue", "Default value for key." )
		SD_UI_FUNC_RETURN_PARM( string, "Value." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryServerInfo",			's', "ffss",	&sdNetManager::Script_QueryServerInfo );

	SD_UI_FUNC_TAG( queryMapInfo, "Query a server map info for a key/val." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( float, "index", "Session index." )
		SD_UI_FUNC_PARM( string, "key", "Key to get." )
		SD_UI_FUNC_PARM( string, "defaultValue", "Default value for key." )
		SD_UI_FUNC_RETURN_PARM( string, "Value." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryMapInfo",				's', "ffss",	&sdNetManager::Script_QueryMapInfo );

	SD_UI_FUNC_TAG( queryGameType, "Query the game type for a server." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( float, "index", "Session index." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Gametype for session." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryGameType",			'w', "ff",		&sdNetManager::Script_QueryGameType );

	SD_UI_FUNC_TAG( requestStats, "Create request stats task." )
		SD_UI_FUNC_PARM( float, "globalOnly", "If true only use profile stats retrieved from demonware." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "requestStats",				'f', "f",		&sdNetManager::Script_RequestStats );

	SD_UI_FUNC_TAG( getStat, "Get stat for player." )
		SD_UI_FUNC_PARM( string, "key", "Key for stat." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns float or handle depending on key." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getStat",					'f', "s",		&sdNetManager::Script_GetStat );

	SD_UI_FUNC_TAG( getPlayerAward, "Get player award." )
		SD_UI_FUNC_PARM( float, "rewardType", "Reward type." )
		SD_UI_FUNC_PARM( float, "statType", "Stat type, rank/xp/name." )
		SD_UI_FUNC_PARM( float, "isSelf", "If true, returns value for local player" )
		SD_UI_FUNC_RETURN_PARM( wstring, "Award converted to wide string." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getPlayerAward",			'w', "fff",		&sdNetManager::Script_GetPlayerAward );

	SD_UI_FUNC_TAG( queryXPStats, "Query XP stats." )
		SD_UI_FUNC_PARM( string, "prof", "Proficiency category." )
		SD_UI_FUNC_PARM( float, "total", "True if total XP, false if per-map XP." )
		SD_UI_FUNC_RETURN_PARM( string, "XP converted to a string." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryXPStats",				's', "sf",		&sdNetManager::Script_QueryXPStats );

	SD_UI_FUNC_TAG( queryXPTotals, "Query for total XP." )
		SD_UI_FUNC_PARM( float, "total", "True if total XP, false if per-map XP." )
		SD_UI_FUNC_RETURN_PARM( string, "Returns total XP." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryXPTotals",			's', "f",		&sdNetManager::Script_QueryXPTotals );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( initTeams, "Init team manager." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "initTeams",				'f', "",		&sdNetManager::Script_Team_Init );

	SD_UI_FUNC_TAG( createTeam, "Create a team." )
		SD_UI_FUNC_PARM( string, "teamName", "Team name." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "createTeam",				'v', "s",		&sdNetManager::Script_Team_CreateTeam );

	SD_UI_FUNC_TAG( proposeMembership, "Propose team membership to user." )
		SD_UI_FUNC_PARM( string, "username", "User to propose team membership to." )
		SD_UI_FUNC_PARM( wstring, "message", "Message text." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "proposeMembership",		'v', "sw",		&sdNetManager::Script_Team_ProposeMembership );	// username, reason

	SD_UI_FUNC_TAG( acceptMembership, "Accept team membership from username." )
		SD_UI_FUNC_PARM( string, "username", "User that sent membership." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "acceptMembership",			'f', "s",		&sdNetManager::Script_Team_AcceptMembership );
	
	SD_UI_FUNC_TAG( rejectMembership, "Reject a team membership proposal." )
		SD_UI_FUNC_PARM( string, "username", "User that sent membership." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "rejectMembership",			'f', "s",		&sdNetManager::Script_Team_RejectMembership );

	SD_UI_FUNC_TAG( removeMember, "Remove a member from a team." )
		SD_UI_FUNC_PARM( string, "username", "User to remove from team." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "removeMember",				'v', "s",		&sdNetManager::Script_Team_RemoveMember );

	SD_UI_FUNC_TAG( sendTeamMessage, "Send a message to a team member." )
		SD_UI_FUNC_PARM( string, "username", "Username of team player." )
		SD_UI_FUNC_PARM( wstring, "message", "Message to send." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "sendTeamMessage",			'v', "sw",		&sdNetManager::Script_Team_SendMessage );

	SD_UI_FUNC_TAG( broadcastTeamMessage, "Broadcast message to all members of a team. Only team owner/admin may broadcast messages." )
		SD_UI_FUNC_PARM( wstring, "message", "Message to broadcast." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "broadcastTeamMessage",		'v', "w",		&sdNetManager::Script_Team_BroadcastMessage );

	SD_UI_FUNC_TAG( inviteMember, "Invite a member to a game." )
		SD_UI_FUNC_PARM( string, "username", "User to invite to the game." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "inviteMember",				'v', "s",		&sdNetManager::Script_Team_Invite );

	SD_UI_FUNC_TAG( promoteMember, "Promote a member to admin." )
		SD_UI_FUNC_PARM( string, "username", "User to promote." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "promoteMember",			'v', "s",		&sdNetManager::Script_Team_PromoteMember );

	SD_UI_FUNC_TAG( demoteMember, "Demote a player from being an admin." )
		SD_UI_FUNC_PARM( string, "username", "User to demote from admin status." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "demoteMember",				'v', "s",		&sdNetManager::Script_Team_DemoteMember );

	SD_UI_FUNC_TAG( transferOwnership, "Transfer ownership of a team." )
		SD_UI_FUNC_PARM( string, "username", "User to get ownership of team." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "transferOwnership",		'v', "s",		&sdNetManager::Script_Team_TransferOwnership );

	SD_UI_FUNC_TAG( disbandTeam, "Create task to disband a team." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "disbandTeam",				'v', "",		&sdNetManager::Script_Team_DisbandTeam );

	SD_UI_FUNC_TAG( leaveTeam, "Leave a team." )
		SD_UI_FUNC_PARM( string, "username", "User to leave the team." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "leaveTeam",				'v', "s",		&sdNetManager::Script_Team_LeaveTeam );

	SD_UI_FUNC_TAG( chooseTeamContextAction, "Let gamecode choose a team context action." )
		SD_UI_FUNC_PARM( string, "username", "User to choose a team context action on." )
		SD_UI_FUNC_RETURN_PARM( float, "Context action. See the TCA_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "chooseTeamContextAction",	'f', "s",		&sdNetManager::Script_Team_ChooseContextAction );

	SD_UI_FUNC_TAG( isTeamMember, "Check if a player is a team member." )
		SD_UI_FUNC_PARM( string, "membername", "Name of player to check." )
		SD_UI_FUNC_RETURN_PARM( float, "True if player is a team member." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isTeamMember",				'f', "s",		&sdNetManager::Script_Team_IsMember );

	SD_UI_FUNC_TAG( isPendingTeamMember, "Member waiting approval to join." )
		SD_UI_FUNC_PARM( string, "membername", "Member name of pending member." )
		SD_UI_FUNC_RETURN_PARM( float, "True if member is pending approval to join." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isPendingTeamMember",		'f', "s",		&sdNetManager::Script_Team_IsPendingMember );

	SD_UI_FUNC_TAG( getTeamIMText, "Get the instant message from team member." )
		SD_UI_FUNC_PARM( string, "teammember", "User to get instant message from." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Instant message received." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getTeamIMText",			'w', "s",		&sdNetManager::Script_Team_GetIMText );

	SD_UI_FUNC_TAG( numAvailableTeamIMs, "Number of available instant messages from team member." )
		SD_UI_FUNC_PARM( string, "membername", "Team member to check number of messages from." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of available instant messages." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "numAvailableTeamIMs",		'f', "s",		&sdNetManager::Script_Team_NumAvailableIMs );

	SD_UI_FUNC_TAG( getMemberStatus, "Get status of a team member." )
		SD_UI_FUNC_PARM( string, "membername", "member to get status of." )
		SD_UI_FUNC_RETURN_PARM( float, "Member status. See MS_* for defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getMemberStatus",			'f', "s",		&sdNetManager::Script_Team_GetMemberStatus );

	SD_UI_FUNC_TAG( isMemberOnServer, "Check if member is on a server." )
		SD_UI_FUNC_PARM( string, "membmername", "Member to check." )
		SD_UI_FUNC_RETURN_PARM( float, "True if member is on a server." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isMemberOnServer",			'f', "s",		&sdNetManager::Script_IsMemberOnServer );

	SD_UI_FUNC_TAG( followMemberToServer, "Follow a member to a server." )
		SD_UI_FUNC_PARM( string, "membmername", "Member to check." )
		SD_UI_FUNC_RETURN_PARM( float, "True if trying to connecting to server." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "followMemberToServer",		'f', "s",		&sdNetManager::Script_FollowMemberToServer );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( clearFilters, "Clear all server filters (both numeric and string filters)." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "clearFilters",				'v', "",		&sdNetManager::Script_ClearFilters );

	SD_UI_FUNC_TAG( applyNumericFilter, "Apply a numeric filter to servers." )
		SD_UI_FUNC_PARM( float, "type", "See FS_* defines." )
		SD_UI_FUNC_PARM( float, "state", "See SFS_* defines." )
		SD_UI_FUNC_PARM( float, "operation", "See SFO_* defines." )
		SD_UI_FUNC_PARM( float, "resultBin", "Operator when combining with other filters. See SFR_*." )
		SD_UI_FUNC_PARM( float, "value", "Optional additional value." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "applyNumericFilter",		'v', "fffff",	&sdNetManager::Script_ApplyNumericFilter );

	SD_UI_FUNC_TAG( applyStringFilter, "Apply a string filter to servers." )
		SD_UI_FUNC_PARM( float, "cvar", "Cvar to check for string filter." )
		SD_UI_FUNC_PARM( float, "state", "See SFS_* defines." )
		SD_UI_FUNC_PARM( float, "operation", "See SFO_* defines." )
		SD_UI_FUNC_PARM( float, "resultBin", "Operator when combining with other filters. See SFR_*." )
		SD_UI_FUNC_PARM( float, "value", "Optional additional value." )	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "applyStringFilter",		'v', "sfffs",	&sdNetManager::Script_ApplyStringFilter );

	SD_UI_FUNC_TAG( saveFilters, "Save filters in the profile under keys with name 'filter_<prfix>_*'." )
		SD_UI_FUNC_PARM( string, "prefix", "Prefix for filters." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "saveFilters",				'v', "s",		&sdNetManager::Script_SaveFilters );

	SD_UI_FUNC_TAG( loadFilters, "Load filters from profile with the specified prefix." )
		SD_UI_FUNC_PARM( string, "prefix", "Prefix for filters." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "loadFilters",				'v', "s",		&sdNetManager::Script_LoadFilters );

	SD_UI_FUNC_TAG( queryNumericFilterValue, "Query the value of a numeric filter." )
		SD_UI_FUNC_PARM( float, "type", "See SF_* defines." )
		SD_UI_FUNC_RETURN_PARM( float, "Value of filter." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryNumericFilterValue",	'f', "f",		&sdNetManager::Script_QueryNumericFilterValue );

	SD_UI_FUNC_TAG( queryNumericFilterState, "Query state of a numeric filter." )
		SD_UI_FUNC_PARM( float, "type", "See SF_* defines." )
		SD_UI_FUNC_PARM( float, "value", "Value to use for filter." )
		SD_UI_FUNC_RETURN_PARM( float, "See SFS_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryNumericFilterState",	'f', "ff",		&sdNetManager::Script_QueryNumericFilterState );

	SD_UI_FUNC_TAG( queryStringFilterState, "Get state for string filter." )
		SD_UI_FUNC_PARM( string, "cvar", "CVar name for filter." )
		SD_UI_FUNC_PARM( float, "value", "Value to use for filter." )
		SD_UI_FUNC_RETURN_PARM( float, "See SFS_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryStringFilterState",	'f', "ss",		&sdNetManager::Script_QueryStringFilterState );

	SD_UI_FUNC_TAG( queryStringFilterValue, "Get value for string filter." )
		SD_UI_FUNC_PARM( string, "cvar", "CVar name for filter." )
		SD_UI_FUNC_RETURN_PARM( string, "Value for filter." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryStringFilterValue",	's', "s",		&sdNetManager::Script_QueryStringFilterValue );

	SD_UI_FUNC_TAG( joinSession, "Join a session based an invite." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "joinSession",				'v', "",		&sdNetManager::Script_JoinSession );

	SD_UI_FUNC_TAG( getMotdString, "Get message of the day string for current server." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Message of the day." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getMotdString",			'w', "",		&sdNetManager::Script_GetMotdString );

	SD_UI_FUNC_TAG( getServerStatusString, "Get formatted server status (warmup/loading/running)." )
		SD_UI_FUNC_PARM( float, "source", "Source from which server is in." )
		SD_UI_FUNC_PARM( float, "index", "Session index." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Returns status strings." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getServerStatusString",	'w', "ff",		&sdNetManager::Script_GetServerStatusString );

	SD_UI_FUNC_TAG( toggleServerFavoriteState, "Toggle a server as favorite." )
		SD_UI_FUNC_PARM( string, "session", "IP:Port." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "toggleServerFavoriteState",'v', "s",		&sdNetManager::Script_ToggleServerFavoriteState );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( getFriendServerIP, "Get friend server ip." )
		SD_UI_FUNC_PARM( string, "friend", "Friends name." )
		SD_UI_FUNC_RETURN_PARM( string, "IP:Port or empty string." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getFriendServerIP",		's', "s",		&sdNetManager::Script_GetFriendServerIP );

	SD_UI_FUNC_TAG( getMemberServerIP, "Get member server ip." )
		SD_UI_FUNC_PARM( string, "member", "Members name." )
		SD_UI_FUNC_RETURN_PARM( string, "IP:Port or empty string." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getMemberServerIP",		's', "s",		&sdNetManager::Script_GetMemberServerIP );

	SD_UI_FUNC_TAG( getMessageTimeStamp, "Get time left on server." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Time left on server." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getMessageTimeStamp",		'w', "",		&sdNetManager::Script_GetMessageTimeStamp );

	SD_UI_FUNC_TAG( getServerInviteIP, "Get IP of server invite." )
		SD_UI_FUNC_RETURN_PARM( string, "IP:Port of server invite." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getServerInviteIP",		's', "",		&sdNetManager::Script_GetServerInviteIP );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( cancelActiveTask, "Cancel currently active task." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "cancelActiveTask",			'v', "",		&sdNetManager::Script_CancelActiveTask );

	SD_UI_FUNC_TAG( formatSessionInfo, "Print out formatted session info." )
		SD_UI_FUNC_PARM( string, "session", "IP:Port of session." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Formatted session info." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "formatSessionInfo",		'w', "s",		&sdNetManager::Script_FormatSessionInfo );			// IP:Port
}
SD_UI_POP_CLASS_TAG
#pragma optimize( "", on )
#pragma inline_depth()

#undef ALLOC_FUNC

/*
================
sdNetManager::ShutdownFunctions
================
*/
void sdNetManager::ShutdownFunctions() {
	uiFunctions.DeleteContents();
}

/*
================
sdNetManager::FindFunction
================
*/
sdNetManager::uiFunction_t* sdNetManager::FindFunction( const char* name ) {
	uiFunction_t** ptr;
	return uiFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdNetManager::GetFunction
============
*/
sdUIFunctionInstance* sdNetManager::GetFunction( const char* name ) {
	uiFunction_t* function = FindFunction( name );
	if ( function == NULL ) {
		return NULL;
	}

	return new sdUITemplateFunctionInstance< sdNetManager, sdUITemplateFunctionInstance_Identifier >( this, function );
}

/*
================
sdNetManager::RunFrame
================
*/
void sdNetManager::RunFrame() {
	properties.UpdateProperties();

	// task processing

	// process parallel tasks
	for ( int i = 0; i < MAX_ACTIVE_TASKS; i++ ) {
		sdNetTask* activeTask = activeTasks[ i ].task;

		if ( activeTask != NULL && activeTask->GetState() == sdNetTask::TS_DONE ) {
			activeTasks[ i ].OnCompleted( this );
			networkService->FreeTask( activeTask );
		}
	}

	// process single 'serial' task (these are tasks usually started by the player from a gui)
	if ( activeTask.task != NULL ) {
		if ( activeTask.task->GetState() == sdNetTask::TS_DONE ) {
			sdNetTask* completedTask = activeTask.task;

			activeTask.OnCompleted( this );

			properties.SetTaskResult( completedTask->GetErrorCode(), declHolder.FindLocStr( va( "sdnet/error/%d", completedTask->GetErrorCode() ) ) );
			properties.SetTaskActive( false );

			networkService->FreeTask( completedTask );
		}
	}

	if ( findServersTask ) {
		properties.SetNumAvailableDWServers( sessions.Num() );

		if ( findServersTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( findServersTask );
			findServersTask = NULL;
			hotServers.Locate( *this );
		}
	}

	if ( findLANServersTask ) {
		properties.SetNumAvailableLANServers( sessionsLAN.Num() );

		if ( findLANServersTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( findLANServersTask );
			findLANServersTask = NULL;
			hotServersLAN.Locate( *this );
		}
	}

	if ( findLANRepeatersTask ) {
		properties.SetNumAvailableLANRepeaters( sessionsLANRepeaters.Num() );

		if ( findLANRepeatersTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( findLANRepeatersTask );
			findLANRepeatersTask = NULL;
		}
	}

	if ( findRepeatersTask ) {
		properties.SetNumAvailableRepeaters( sessionsRepeaters.Num() );

		if ( findRepeatersTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( findRepeatersTask );
			findRepeatersTask = NULL;
		}
	}

	if ( findHistoryServersTask ) {
		properties.SetNumAvailableHistoryServers( sessionsHistory.Num() );

		if ( findHistoryServersTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( findHistoryServersTask );
			findHistoryServersTask = NULL;
			hotServersHistory.Locate( *this );
		}
	}

	if ( findFavoriteServersTask ) {
		properties.SetNumAvailableFavoritesServers( sessionsFavorites.Num() );

		if ( findFavoriteServersTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( findFavoriteServersTask );
			findFavoriteServersTask = NULL;
			hotServersFavorites.Locate( *this );
		}
	}

	bool findingServers =	findHistoryServersTask != NULL || 
							findServersTask != NULL ||
							findLANServersTask != NULL ||
							findFavoriteServersTask != NULL ||
							findRepeatersTask != NULL ||
							findLANRepeatersTask != NULL;
	properties.SetFindingServers( findingServers );

	if( !findingServers ) {
		lastServerUpdateIndex = 0;
	}

#if !defined( SD_DEMO_BUILD )
	if ( initFriendsTask && initFriendsTask->GetState() == sdNetTask::TS_DONE ) {
		networkService->FreeTask( initFriendsTask );
		initFriendsTask = NULL;
	}

	if ( initTeamsTask && initTeamsTask->GetState() == sdNetTask::TS_DONE ) {
		networkService->FreeTask( initTeamsTask );
		initTeamsTask = NULL;
	}

	properties.SetInitializingFriends( initFriendsTask != NULL );
	properties.SetInitializingTeams( initTeamsTask != NULL );
#endif /* !SD_DEMO_BUILD */

	if( refreshServerTask != NULL ) {
		if( refreshServerTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( refreshServerTask );
			refreshServerTask = NULL;

			if( serverRefreshSession != NULL ) {
				sdNetTask* task;
				idList< sdNetSession* >* netSessions;
				sdHotServerList* netHotServers;
				GetSessionsForServerSource( serverRefreshSource, netSessions, task, netHotServers );

				// replace the existing session with the updated one
				if( netSessions != NULL ) {
					sessionHash_t::Iterator iter = hashedSessions.Find( serverRefreshSession->GetHostAddressString() );
					if( iter != hashedSessions.End() ) {
						if( task != NULL ) {
							task->AcquireLock();
						}

						int index = iter->second.sessionListIndex;
						if( index >= 0 && index < netSessions->Num() ) {
							const char* newIP = serverRefreshSession->GetHostAddressString();
							const char* oldIP = (*netSessions)[ index ]->GetHostAddressString();
							if( idStr::Cmp( newIP, oldIP ) == 0 ) {
								networkService->GetSessionManager().FreeSession( (*netSessions)[ index ] );
								(*netSessions)[ index ] = serverRefreshSession;								
								serverRefreshSession = NULL;
								iter->second.lastUpdateTime = sys->Milliseconds();
							} else {
								assert( false );
							}
						}

						if( task != NULL ) {
							task->ReleaseLock();
						}
					}
				} else {
					assert( false );
				}
			}

			properties.SetServerRefreshComplete( true );
		}
	}

	if ( refreshHotServerTask != NULL ) {
		if( refreshHotServerTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( refreshHotServerTask );
			refreshHotServerTask = NULL;

			for ( int i = 0; i < hotServerRefreshSessions.Num(); i++ ) {
				sdNetTask* task;
				idList< sdNetSession* >* netSessions;
				sdHotServerList* netHotServers;
				GetSessionsForServerSource( hotServersRefreshSource, netSessions, task, netHotServers );

				// replace the existing session with the updated one
				if( netSessions != NULL ) {
					sessionHash_t::Iterator iter = hashedSessions.Find( hotServerRefreshSessions[ i ]->GetHostAddressString() );
					if( iter != hashedSessions.End() ) {
						if( task != NULL ) {
							task->AcquireLock();
						}

						int index = iter->second.sessionListIndex;
						if( index >= 0 && index < netSessions->Num() ) {
							const char* newIP = hotServerRefreshSessions[ i ]->GetHostAddressString();
							const char* oldIP = (*netSessions)[ index ]->GetHostAddressString();
							if( idStr::Cmp( newIP, oldIP ) == 0 ) {
								networkService->GetSessionManager().FreeSession( (*netSessions)[ index ] );
								(*netSessions)[ index ] = hotServerRefreshSessions[ i ];								
								hotServerRefreshSessions[ i ] = NULL;
								iter->second.lastUpdateTime = sys->Milliseconds();
							} else {
								assert( false );
							}
						}

						if( task != NULL ) {
							task->ReleaseLock();
						}
					}
				} else {
					assert( false );
				}
			}
			hotServerRefreshSessions.SetNum( 0, false );

			properties.SetHotServersRefreshComplete( true );
		}
	}

#if !defined( SD_DEMO_BUILD )
	// FIXME: don't update this every frame
	const sdNetTeamManager& manager = networkService->GetTeamManager();
	properties.SetTeamName( manager.GetTeamName() );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& pending = networkService->GetTeamManager().GetPendingInvitesList();
	properties.SetNumPendingClanInvites( pending.Num() );
	properties.SetTeamMemberStatus( manager.GetMemberStatus() );
#endif /* !SD_DEMO_BUILD */
}

/*
============
sdNetManager::GetSessionsForServerSource
============
*/
void sdNetManager::GetSessionsForServerSource( findServerSource_e source, idList< sdNetSession* >*& netSessions, sdNetTask*& task, sdHotServerList*& netHotServers ) {
	switch( source ) {
		case FS_INTERNET:
			task = findServersTask;
			netSessions = &sessions;
			netHotServers = &hotServers;
			break;
#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
		case FS_INTERNET_REPEATER:
			task = findRepeatersTask;
			netSessions = &sessionsRepeaters;
			netHotServers = NULL;
			break;
#endif /* !SD_DEMO_BUILD && !SD_DEMO_BUILD_CONSTRUCTION */
		case FS_LAN:
			task = findLANServersTask;
			netSessions = &sessionsLAN;
			netHotServers = &hotServersLAN;
			break;
#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
		case FS_LAN_REPEATER:
			task = findLANRepeatersTask;
			netSessions = &sessionsLANRepeaters;
			netHotServers = NULL;
			break;
#endif /* !SD_DEMO_BUILD && !SD_DEMO_BUILD_CONSTRUCTION */
		case FS_HISTORY:
			task = findHistoryServersTask;
			netSessions = &sessionsHistory;
			netHotServers = &hotServersHistory;
			break;
		case FS_FAVORITES:
			task = findFavoriteServersTask;
			netSessions = &sessionsFavorites;
			netHotServers = &hotServersFavorites;
			break;
	}
}


/*
============
sdNetManager::GetGameType
============
*/
void sdNetManager::GetGameType( const char* siRules, idWStr& type ) {
	if( siRules[ 0 ] == '\0' ) {
		type = L"";
		return;
	}
	const char* gameTypeLoc = gameTypeNames->GetDict().GetString( siRules, "" );

	if( gameTypeLoc[ 0 ] != '\0' ) {
		type = common->LocalizeText( gameTypeLoc );
	} else {
		type= va( L"%hs", siRules );
	}
}

/*
================
sdNetManager::CreateServerList
================
*/
void sdNetManager::CreateServerList( sdUIList* list, findServerSource_e source, findServerMode_e mode, bool flagOffline ) {
	if( flagOffline ) {
		for( int i = 0; i < list->GetNumItems(); i++ ) {
			if( list->GetItemDataInt( i, 0 ) == -1 ) {
				sdUIList::SetItemText( list, va( L"(%ls) %ls", offlineString->GetText(), list->GetItemText( i, 0, true ) ), i, 4 );
			}
		}
		return;
	}

	sdNetTask* task = NULL;
	idList< sdNetSession* >* netSessions = NULL;
	sdHotServerList* netHotServers;

	GetSessionsForServerSource( source, netSessions, task, netHotServers );

	if( netSessions == NULL ) {
		assert( 0 );
		return;
	}

	if ( task != NULL ) {
		task->AcquireLock();
	}

	list->SetItemGranularity( 1024 );
	list->BeginBatch();

	if( lastServerUpdateIndex == 0 ) {
		if ( mode == FSM_NEW ) {
			hashedSessions.Clear();
			hashedSessions.SetGranularity( 1024 );
			sdUIList::ClearItems( list );
		} else if( mode == FSM_REFRESH ) {
			for( int i = 0; i < list->GetNumItems(); i++ ) {
				list->SetItemDataInt( -1, i, 0 );		// flag all items as not updated so servers that have dropped won't show bad info
			}
		}
	}

	int now = sys->Milliseconds();

	if ( mode == FSM_NEW ) {
#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
		bool ranked = ShowRanked();
		bool tvSource = ( source == FS_INTERNET_REPEATER ) || ( source == FS_LAN_REPEATER );
#else
		bool ranked = false;
		bool tvSource = false;
#endif /* !SD_DEMO_BUILD */
		idStr address;		

		for ( int i = lastServerUpdateIndex; i < netSessions->Num(); i++ ) {
			sdNetSession* netSession = (*netSessions)[ i ];
			
			address = netSession->GetHostAddressString();
			assert( !address.IsEmpty() );

			sessionIndices_t& info = hashedSessions[ address.c_str() ];
			info.sessionListIndex = i;
			info.uiListIndex = -1;
			info.lastUpdateTime = now;

			if( source == FS_INTERNET ||
				source == FS_LAN ||
				tvSource ) {
				if( DoFiltering( *netSession ) ) {
					if( !tvSource ) {	// always pass for TV filters, since ranked status isn't passed along
						if( ranked && !netSession->IsRanked() ) {
							continue;
						}
					}
					if( tvSource && !netSession->IsRepeater() ) {
						continue;
					}
					if ( SessionIsFiltered( *netSession ) ) {
						continue;
					}
				}
			}

			int index = sdUIList::InsertItem( list, va( L"%hs", address.c_str() ), -1, BC_IP );

//			assert( hashedSessions.Find( address.c_str() ) == hashedSessions.End() );
//			assert( hashedSessions.Num() == list->GetNumItems() - 1 );

			info.uiListIndex = index;

			UpdateSession( *list, *netSession, index );
			list->SetItemDataInt( i, index, 0, true );		// store the session index

		}
	} else if ( mode == FSM_REFRESH ) {
		for ( int i = lastServerUpdateIndex; i < netSessions->Num(); i++ ) {
			sdNetSession* netSession = (*netSessions)[ i ];
			const idDict& serverInfo = netSession->GetServerInfo();

			sessionHash_t::Iterator iter = hashedSessions.Find( netSession->GetHostAddressString() );
			if( iter == hashedSessions.End() ) {
				continue;
			} else {
				iter->second.sessionListIndex = i;
				iter->second.lastUpdateTime = now;

				if( iter->second.uiListIndex != -1 ) {
					UpdateSession( *list, *netSession, iter->second.uiListIndex );
					list->SetItemDataInt( i, iter->second.uiListIndex, BC_IP, true );		// store the session index
					
					assert( idWStr::Icmp( list->GetItemText( iter->second.uiListIndex, 0 ), va( L"%hs", netSession->GetHostAddressString() ) ) == 0 );
				}
			}
		}
	}

	lastServerUpdateIndex = netSessions->Num();
//	assert( hashedSessions.Num() == list->GetNumItems() );

	if ( task != NULL ) {
		task->ReleaseLock();
	}
	list->EndBatch();
}

/*
============
sdNetManager::CreateHotServerList
============
*/
void sdNetManager::CreateHotServerList( sdUIList* list, findServerSource_e source ) {
	sdUIList::ClearItems( list );

	sdNetTask* task = NULL;
	idList< sdNetSession* >* netSessions = NULL;
	sdHotServerList* netHotServers = NULL;

	GetSessionsForServerSource( source, netSessions, task, netHotServers );

	if ( netSessions == NULL || netHotServers == NULL ) {
		return;
	}

	idStr address;
	for( int i = 0; i < netHotServers->GetNumServers(); i++ ) {
		const sdNetSession* netSession = netHotServers->GetServer( i );
		if( netSession == NULL ) {
			continue;
		}

		address = netSession->GetHostAddressString();
		int index = sdUIList::InsertItem( list, va( L"%hs", address.c_str() ), -1, BC_IP );
		UpdateSession( *list, *netSession, index );

		sessionHash_t::Iterator iter = hashedSessions.Find( address.c_str() );
		if( iter != hashedSessions.End() ) {
			list->SetItemDataInt( iter->second.sessionListIndex, index, 0, true );		// store the session index
		} else {
			assert( false );
		}
	}
}
/*
============
sdNetManager::UpdateSession
============
*/
void sdNetManager::UpdateSession( sdUIList& list, const sdNetSession& netSession, int index ) {
	if( index < 0 ) {
		return;
	}
	sdNetUser* activeUser = networkService->GetActiveUser();
	bool favorite = activeUser->GetProfile().GetProperties().GetBool( va( "favorite_%s", netSession.GetHostAddressString() ), "0" );

	// Favorite state
	sdUIList::SetItemText( &list, favorite ? L"<material = 'favorite_set'>f" : L"<material = 'favorite_unset'>", index, BC_FAVORITE );

	if ( netSession.GetPing() == 999 ) {
		// Password
		sdUIList::SetItemText( &list, L"", index, BC_PASSWORD );

		// Ranked
		sdUIList::SetItemText( &list, L"", index, BC_RANKED );

		// Server name
		sdUIList::SetItemText( &list, va( L"(%ls) %hs", offlineString->GetText(), netSession.GetHostAddressString() ), index, BC_NAME );

		// Map name
		sdUIList::SetItemText( &list, L"", index, BC_MAP );

		// Game Type Icon
		sdUIList::SetItemText( &list, L"", index, BC_GAMETYPE_ICON );

		// Game Type
		sdUIList::SetItemText( &list, L"", index, BC_GAMETYPE );

		// Time left
		list.SetItemDataInt( 0, index, BC_TIMELEFT, true );
		sdUIList::SetItemText( &list, L"", index, BC_TIMELEFT );

		// Client Count
		list.SetItemDataInt( 0, index, BC_PLAYERS, true );
		sdUIList::SetItemText( &list, L"", index, BC_PLAYERS );

		// Ping
		list.SetItemDataInt( 0, index, BC_PING, true );
		sdUIList::SetItemText( &list, L"-1", index, BC_PING );
	} else {
		const idDict& serverInfo = netSession.GetServerInfo();
		const idDict* mapInfo = GetMapInfo( netSession );

		// Password
		sdUIList::SetItemText( &list, va( L"%hs", serverInfo.GetBool( "si_needPass", "0" ) ? "<material = 'password'>p" : "" ), index, BC_PASSWORD);

		// Ranked
		sdUIList::SetItemText( &list, va( L"%hs", netSession.IsRanked() ? "<material = 'ranked'>r" : "" ), index, BC_RANKED );

		tempWStr = L"";
		const char* serverName = serverInfo.GetString( "si_name" );
		if( serverName[ 0 ] == '\0' ) {
			tempWStr = va( L"(%ls) %hs", offlineString->GetText(), netSession.GetHostAddressString() );
		} else {
			tempWStr = va( L"%hs", serverName );
			sdUIList::CleanUserInput( tempWStr );
		}

		// strip leading spaces
		const wchar_t* nameStr = tempWStr.c_str();
		do {
			if( *nameStr == L'\0' ) {
				break;
			}

			if( *( nameStr + 1 ) != L'\0' && *( nameStr + 2 ) != L'\0' && idWStr::IsColor( nameStr ) && ( *( nameStr + 2 ) == L' ' || *( nameStr + 2 ) == L'\t' ) ) {
				nameStr += 2;
				continue;
			}

			if( ( *nameStr != L' ' && *nameStr != L'\t' ) ) {
				break;
			}
			nameStr++;
		} while( true );			

		// Server name
		sdUIList::SetItemText( &list, nameStr, index, BC_NAME );

		// Map name
		if ( mapInfo == NULL ) {
			tempWStr = va( L"%hs", serverInfo.GetString( "si_map" ) );
			tempWStr.StripFileExtension();

			sdUIList::CleanUserInput( tempWStr );

			// skip the path
			int offset = tempWStr.Length() - 1;
			if( offset >= 0 ) {
				while( offset >= 0 ) {
					if( tempWStr[ offset ] == L'/' || tempWStr[ offset ] == L'\\' ) {
						offset++;
						break;
					}
					offset--;
				}
				// jrad - the case of "path/" should never happen, but let's play it safe
				if( offset >= tempWStr.Length() ) {
					offset = 0;
				}

				sdUIList::SetItemText( &list, &tempWStr[ offset ], index, BC_MAP );
			} else {
				sdUIList::SetItemText( &list, L"", index, BC_MAP );
			}
		} else {
			tempWStr = mapInfo != NULL ? va( L"%hs", mapInfo->GetString( "pretty_name", serverInfo.GetString( "si_map" ) ) ) : L"";
			tempWStr.StripFileExtension();
			sdUIList::CleanUserInput( tempWStr );
			sdUIList::SetItemText( &list, tempWStr.c_str(), index, BC_MAP );
		}


		// Game Type
		const char* fsGame = serverInfo.GetString( "fs_game" );

		if ( *fsGame == '\0' || ( idStr::Icmp( fsGame, BASE_GAMEDIR ) == 0 ) ) {
			const char* siRules = serverInfo.GetString( "si_rules" );
			GetGameType( siRules, tempWStr );

			const char* mat = list.GetUI()->GetMaterial( siRules );
			if( *mat ) {
				sdUIList::SetItemText( &list, va( L"<material = '%hs'>", siRules ), index, BC_GAMETYPE_ICON );
			} else {
				sdUIList::SetItemText( &list, L"", index, BC_GAMETYPE_ICON );
			}
		} else {
			sdUIList::SetItemText( &list, L"<material = '_unknownGameType'>", index, BC_GAMETYPE_ICON );
			tempWStr = va( L"%hs", fsGame );
		}		

		sdUIList::SetItemText( &list, tempWStr.c_str(), index, BC_GAMETYPE );

		// Time left
		if( netSession.GetGameState() & sdGameRules::PGS_WARMUP ) {
			sdUIList::SetItemText( &list, L"<loc = 'guis/mainmenu/server/warmup'>", index, BC_TIMELEFT );
			list.SetItemDataInt( 0, index, BC_TIMELEFT, true );
		} else if( netSession.GetGameState() & sdGameRules::PGS_REVIEWING ) {
			sdUIList::SetItemText( &list, L"<loc = 'guis/mainmenu/server/reviewing'>", index, BC_TIMELEFT );
			list.SetItemDataInt( 0, index, BC_TIMELEFT, true );
		} else if( netSession.GetGameState() & sdGameRules::PGS_LOADING ) {
			sdUIList::SetItemText( &list, L"<loc = 'guis/mainmenu/server/loading'>", index, BC_TIMELEFT );
			list.SetItemDataInt( 0, index, BC_TIMELEFT, true );
		} else {
			if( netSession.GetSessionTime() == 0 ) {
				sdUIList::SetItemText( &list, infinityString->GetText(), index, BC_TIMELEFT );
			} else {
				idWStr::hmsFormat_t format;
				format.showZeroSeconds = false;
				format.showZeroMinutes = true;
				sdUIList::SetItemText( &list, idWStr::MS2HMS( netSession.GetSessionTime(), format ), index, BC_TIMELEFT );
			}
			list.SetItemDataInt( netSession.GetSessionTime(), index, BC_TIMELEFT, true );
		}

		// Client Count
		int numBots = netSession.GetNumBotClients();
		int numClients = 0;
		int maxClients = 0;

		if( netSession.IsRepeater() ) {
			numClients = netSession.GetNumRepeaterClients();
			maxClients = netSession.GetMaxRepeaterClients();
		} else {
			numClients = netSession.GetNumClients();
			maxClients = serverInfo.GetInt( "si_maxPlayers" );
		}

		if( numBots == 0 ) {
			sdUIList::SetItemText( &list, va( L"%d/%d", numClients, maxClients ), index, BC_PLAYERS );
		} else {
			sdUIList::SetItemText( &list, va( L"%d/%d (%d)", numClients, maxClients, numBots ), index, BC_PLAYERS );
		}

		list.SetItemDataInt( numClients, index, BC_PLAYERS, true );	// store the number of clients for proper numeric sorting

		// Ping
		list.SetItemDataInt( netSession.GetPing(), index, BC_PING, true );
		sdUIList::SetItemText( &list, va( L"%d", netSession.GetPing() ), index, BC_PING );
	}
}

/*
================
sdNetManager::SignInDedicated
================
*/
bool sdNetManager::SignInDedicated() {
	assert( networkService->GetDedicatedServerState() == sdNetService::DS_OFFLINE );

	task_t* activeTask = GetTaskSlot();
	if ( activeTask == NULL ) {
		return false;
	}

	return ( activeTask->Set( networkService->SignInDedicated(), &sdNetManager::OnSignInDedicated ) );
}

/*
================
sdNetManager::OnSignInDedicated
================
*/
void sdNetManager::OnSignInDedicated( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() != SDNET_NO_ERROR ) {
		// FIXME: error out?
	}
}

/*
================
sdNetManager::SignOutDedicated
================
*/
bool sdNetManager::SignOutDedicated() {
	if ( networkService->GetDedicatedServerState() != sdNetService::DS_ONLINE ) {
		return false;
	}

	task_t* activeTask = GetTaskSlot();
	if ( activeTask == NULL ) {
		return false;
	}

	return ( activeTask->Set( networkService->SignOutDedicated() ) );
}

/*
================
sdNetManager::StartGameSession
================
*/
bool sdNetManager::StartGameSession() {
	if ( gameSession != NULL ) {
		return false;
	}

	if ( !NeedsGameSession() ) {
		// only advertise dedicated non-LAN games
		return false;
	}

	task_t* activeTask = GetTaskSlot();
	if ( activeTask == NULL ) {
		return false;
	}

	gameSession = networkService->GetSessionManager().AllocSession();

	if ( gameLocal.isServer ) {
		gameSession->GetServerInfo() = gameLocal.serverInfo;
	}
#ifdef SD_SUPPORT_REPEATER
	else {
		gameLocal.BuildRepeaterInfo( gameSession->GetServerInfo() );
	}
#endif // SD_SUPPORT_REPEATER

	if ( activeTask->Set( networkService->GetSessionManager().CreateSession( *gameSession ), &sdNetManager::OnGameSessionCreated ) ) {
		lastSessionUpdateTime = gameLocal.time;
		return true;
	}

	networkService->GetSessionManager().FreeSession( gameSession );
	gameSession = NULL;

	return false;
}

/*
================
sdNetManager::UpdateGameSession
================
*/
bool sdNetManager::UpdateGameSession( bool checkDict, bool throttle ) {
	if ( gameSession == NULL ) {
		return false;
	}

	if ( gameSession->GetState() != sdNetSession::SS_IDLE && throttle ) {
		if ( lastSessionUpdateTime + SESSION_UPDATE_INTERVAL > gameLocal.time ) {
			return false;
		}
	}

#ifdef SD_SUPPORT_REPEATER
	idDict repeaterDict;
	if ( !gameLocal.isServer ) {
		gameLocal.BuildRepeaterInfo( repeaterDict );
	}
#endif // SD_SUPPORT_REPEATER

	if ( checkDict ) {
		unsigned int sessionChecksum = gameSession->GetServerInfo().Checksum();
		unsigned int gameCheckSum = 0;

		if ( gameLocal.isServer ) {
			gameCheckSum = gameLocal.serverInfo.Checksum();
		}
#ifdef SD_SUPPORT_REPEATER
		else {
			gameCheckSum = repeaterDict.Checksum();
		}
#endif // SD_SUPPORT_REPEATER

		if ( sessionChecksum == gameCheckSum ) {
			return false;
		}
	}

	task_t* activeTask = GetTaskSlot();
	if ( activeTask == NULL ) {
		return false;
	}

	if ( gameLocal.isServer ) {
		gameSession->GetServerInfo() = gameLocal.serverInfo;
	}
#ifdef SD_SUPPORT_REPEATER
	else {
		gameSession->GetServerInfo() = repeaterDict;
	}
#endif // SD_SUPPORT_REPEATER

	lastSessionUpdateTime = gameLocal.time;

	if ( gameSession->GetState() == sdNetSession::SS_IDLE ) {
		// TODO: document in which case this can happen
		// One of the cases is when the dedicated server disconnects and
		// reconnects. We still have a game session, but need to advertise
		// it all over again.
		return ( activeTask->Set( networkService->GetSessionManager().CreateSession( *gameSession ), &sdNetManager::OnGameSessionCreated ) );
	} else if ( gameSession->GetState() == sdNetSession::SS_ADVERTISED ) {
		return ( activeTask->Set( networkService->GetSessionManager().UpdateSession( *gameSession ) ) );
	} else {
		return false;
	}
}

/*
================
sdNetManager::OnGameSessionCreated
================
*/
void sdNetManager::OnGameSessionCreated( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() == SDNET_NO_ERROR ) {
		SendSessionId();

		// connect all clients to the server
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			sdNetClientId netClientId;

			networkSystem->ServerGetClientNetId( i, netClientId );

			if ( netClientId.IsValid() ) {
				gameSession->ServerClientConnect( i );
			}
		}
	}
}

/*
================
sdNetManager::StopGameSession
================
*/
bool sdNetManager::StopGameSession( bool blocking ) {
	if ( gameSession == NULL ) {
		return false;
	}

	if ( gameSession->GetState() == sdNetSession::SS_IDLE ) {
		networkService->GetSessionManager().FreeSession( gameSession );
		gameSession = NULL;
		return true;
	}

	if ( blocking ) {
		sdNetTask* task = networkService->GetSessionManager().DeleteSession( *gameSession );

		if ( task != NULL ) {
			while ( task->GetState() != sdNetTask::TS_COMPLETING );

			networkService->FreeTask( task );
			networkService->GetSessionManager().FreeSession( gameSession );
			gameSession = NULL;
		}

		return true;
	} else {
		task_t* activeTask = GetTaskSlot();
		if ( activeTask == NULL ) {
			return false;
		}

		return ( activeTask->Set( networkService->GetSessionManager().DeleteSession( *gameSession ), &sdNetManager::OnGameSessionStopped ) );
	}
}

/*
================
sdNetManager::OnGameSessionStopped
================
*/
void sdNetManager::OnGameSessionStopped( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() == SDNET_NO_ERROR ) {
		networkService->GetSessionManager().FreeSession( gameSession );
		gameSession = NULL;
	}
}

/*
================
sdNetManager::ServerClientConnect
================
*/
void sdNetManager::ServerClientConnect( const int clientNum ) {
	if ( gameSession == NULL ) {
		return;
	}

	gameSession->ServerClientConnect( clientNum );
}

/*
================
sdNetManager::ServerClientDisconnect
================
*/
void sdNetManager::ServerClientDisconnect( const int clientNum ) {
	if ( gameSession == NULL ) {
		return;
	}

	gameSession->ServerClientDisconnect( clientNum );
}

/*
================
sdNetManager::SendSessionId
	clientNum == -1 : broadcast to all clients
================
*/
void sdNetManager::SendSessionId( const int clientNum ) const {
	if ( !gameLocal.isServer || gameSession == NULL ) {
		return;
	}

	sdNetSession::sessionId_t sessionId;

	gameSession->GetId( sessionId );

	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_SESSION_ID );

	for ( int i = 0; i < sizeof( sessionId.id ) / sizeof( sessionId.id[0] ); i++ ) {
		msg.WriteByte( sessionId.id[i] );
	}

	msg.Send( sdReliableMessageClientInfo( clientNum ) );
}

/*
================
sdNetManager::ProcessSessionId
================
*/
void sdNetManager::ProcessSessionId( const idBitMsg& msg ) {
	for ( int i = 0; i < sizeof( sessionId.id ) / sizeof( sessionId.id[0] ); i++ ) {
		sessionId.id[i] = msg.ReadByte();
	}
}

#if !defined( SD_DEMO_BUILD )
/*
================
sdNetManager::FlushStats
================
*/
void sdNetManager::FlushStats( bool blocking ) {
	if ( networkService->GetDedicatedServerState() != sdNetService::DS_ONLINE ) {
		return;
	}

	if ( blocking ) {
		sdNetTask* task = networkService->GetStatsManager().Flush();

		if ( task != NULL ) {
			sdSignal waitSignal;
			int i = 0;

			common->SetRefreshOnPrint( true );
			while ( task->GetState() != sdNetTask::TS_COMPLETING ) {
				const char spinner[4] = { '|', '/', '-', '\\' };
				gameLocal.Printf( "\rWriting pending stats... [ %c ]", spinner[ i % 4 ] );
				waitSignal.Wait( 33 );
				sys->ProcessOSEvents();
				i++;
			}
			networkService->FreeTask( task );
			gameLocal.Printf( "\rWriting pending stats... [ done ]\n" );
			common->SetRefreshOnPrint( false );
		}
	} else {
		task_t* activeTask = GetTaskSlot();
		if ( activeTask == NULL ) {
			return;
		}

		activeTask->allowCancel = false;
		activeTask->Set( networkService->GetStatsManager().Flush() );
	}
}
#endif /* !SD_DEMO_BUILD */

/*
================
sdNetManager::FindUser
================
*/
sdNetUser* sdNetManager::FindUser( const char* username ) {
	for ( int i = 0; i < networkService->NumUsers(); i++ ) {
		sdNetUser* user = networkService->GetUser( i );

		if ( !idStr::Icmp( username, user->GetRawUsername() ) ) {
			return user;
		}
	}

	return NULL;
}

/*
================
sdNetManager::Script_Connect
================
*/
void sdNetManager::Script_Connect( sdUIFunctionStack& stack ) {
	stack.Push( Connect() ? 1.0f : 0.0f );
}

/*
================
sdNetManager::Script_ValidateUsername
================
*/
void sdNetManager::Script_ValidateUsername( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );

	if ( username.IsEmpty() ) {
		stack.Push( sdNetProperties::UV_EMPTY_NAME );
		return;
	}

	if ( FindUser( username.c_str() ) != NULL ) {
		stack.Push( sdNetProperties::UV_DUPLICATE_NAME );
		return;
	}

	for( int i = 0; i < username.Length(); i++ ) {
		if( username[ i ] == '.' || username[ i ] == '_' || username[ i ] == '-' ) {
			continue;
		}
		if( username[ i ] >= 'a' && username[ i ] <= 'z' ) {
			continue;
		}
		if( username[ i ] >= 'A' && username[ i ] <= 'Z' ) {
			continue;
		}
		if( username[ i ] >= '0' && username[ i ] <= '9' ) {
			continue;
		}
		stack.Push( sdNetProperties::UV_INVALID_NAME );
		return;
	}

	stack.Push( sdNetProperties::UV_VALID );
}

/*
================
sdNetManager::Script_MakeRawUsername
================
*/
void sdNetManager::Script_MakeRawUsername( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );

	sdNetUser::MakeRawUsername( username.c_str(), username );

	stack.Push( username.c_str() );
}

/*
================
sdNetManager::Script_ValidateEmail
================
*/
void sdNetManager::Script_ValidateEmail( sdUIFunctionStack& stack ) {
	idStr email;
	idStr confirmEmail;

	stack.Pop( email );
	stack.Pop( confirmEmail );

	if ( email.IsEmpty() || confirmEmail.IsEmpty() ) {
		stack.Push( sdNetProperties::EV_EMPTY_MAIL );
		return;
	}

	if ( !email.IsValidEmailAddress() ) {
		stack.Push( sdNetProperties::EV_INVALID_MAIL );
		return;
	}

	if ( email.Cmp( confirmEmail.c_str() ) != 0 ) {
		stack.Push( sdNetProperties::EV_CONFIRM_MISMATCH );
		return;
	}

	stack.Push( sdNetProperties::EV_VALID );
}

/*
================
sdNetManager::Script_CreateUser
================
*/
void sdNetManager::Script_CreateUser( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );

	sdNetUser* user;

	networkService->CreateUser( &user, username.c_str() );
}

/*
================
sdNetManager::Script_DeleteUser
================
*/
void sdNetManager::Script_DeleteUser( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );

	sdNetUser* user = FindUser( username.c_str() );
	if ( user != NULL ) {
		assert( networkService->GetActiveUser() != user );

		networkService->DeleteUser( user );
	}
}

/*
================
sdNetManager::Script_ActivateUser
================
*/
void sdNetManager::Script_ActivateUser( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );
	username.ToLower();

	assert( networkService->GetActiveUser() == NULL );

	sdNetUser* user = FindUser( username.c_str() );
	if ( user != NULL ) {
		user->Activate();
	}
	stack.Push( user != NULL );
}

/*
================
sdNetManager::Script_DeactivateUser
================
*/
void sdNetManager::Script_DeactivateUser( sdUIFunctionStack& stack ) {
	if( networkService->GetActiveUser() == NULL ) {
		assert( networkService->GetActiveUser() != NULL );
		gameLocal.Warning( "Script_DeactivateUser: no active user" );
		return;
	}

	networkService->GetActiveUser()->Deactivate();
}

/*
================
sdNetManager::Script_SaveUser
================
*/
void sdNetManager::Script_SaveUser( sdUIFunctionStack& stack ) {
	if( networkService->GetActiveUser() == NULL ) {
		assert( networkService->GetActiveUser() != NULL );
		gameLocal.Warning( "Script_SaveUser: no active user" );
		return;
	}

	networkService->GetActiveUser()->Save();
}

#if !defined( SD_DEMO_BUILD )
/*
================
sdNetManager::Script_CreateAccount
================
*/
void sdNetManager::Script_CreateAccount( sdUIFunctionStack& stack ) {
	idStr username, password, keyCode;
	stack.Pop( username );
	stack.Pop( password );
	stack.Pop( keyCode );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::CreateAccount : task pending\n" );
		stack.Push( 0.0f );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( activeUser == NULL ) {
		gameLocal.Printf( "SDNet::CreateAccount : no active user\n" );
		stack.Push( 0.0f );
		return;
	}

	if ( !activeTask.Set( activeUser->GetAccount().CreateAccount( username, password, keyCode ) ) ) {
		gameLocal.Printf( "SDNet::CreateAccount : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( 0.0f );
		return;
	}

	properties.SetTaskActive( true );
	stack.Push( 1.0f );
}

/*
================
sdNetManager::Script_DeleteAccount
================
*/
void sdNetManager::Script_DeleteAccount( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr password;
	stack.Pop( password );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::DeleteAccount : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	activeUser->GetAccount().SetPassword( password.c_str() );

	if ( !activeTask.Set( activeUser->GetAccount().DeleteAccount() ) ) {
		gameLocal.Printf( "SDNet::DeleteAccount : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_HasAccount
================
*/
void sdNetManager::Script_HasAccount( sdUIFunctionStack& stack ) {
	if( networkService->GetActiveUser() == NULL ) {
		stack.Push( 0.0f );
		return;
	}

	stack.Push( *(networkService->GetActiveUser()->GetAccount().GetUsername()) == '\0' ? 0.0f : 1.0f );
}

/*
================
sdNetManager::Script_AccountSetPassword
================
*/
void sdNetManager::Script_AccountSetUsername( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr username;
	stack.Pop( username );

	networkService->GetActiveUser()->GetAccount().SetUsername( username.c_str() );
}

/*
================
sdNetManager::Script_AccountIsPasswordSet
================
*/
void sdNetManager::Script_AccountIsPasswordSet( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	stack.Push( *(networkService->GetActiveUser()->GetAccount().GetPassword()) == '\0' ? 0.0f : 1.0f );
}

/*
================
sdNetManager::Script_AccountSetPassword
================
*/
void sdNetManager::Script_AccountSetPassword( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr password;
	stack.Pop( password );

	networkService->GetActiveUser()->GetAccount().SetPassword( password.c_str() );
}

/*
================
sdNetManager::Script_AccountResetPassword
================
*/
void sdNetManager::Script_AccountResetPassword( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr key;
	stack.Pop( key );

	idStr password;
	stack.Pop( password );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::AccountResetPassword : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetActiveUser()->GetAccount().ResetPassword( key.c_str(), password.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::AccountResetPassword : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_AccountChangePassword
================
*/
void sdNetManager::Script_AccountChangePassword( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr password;
	idStr newPassword;
	stack.Pop( password );
	stack.Pop( newPassword );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::AccountChangePassword : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( !activeTask.Set( activeUser->GetAccount().ChangePassword( password.c_str(), newPassword.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::AccountChangePassword : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}
#endif /* !SD_DEMO_BUILD */

/*
================
sdNetManager::Script_SignIn
================
*/
void sdNetManager::Script_SignIn( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	assert( networkService->GetActiveUser()->GetState() == sdNetUser::US_ACTIVE );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::SignIn : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( !activeTask.Set( activeUser->GetAccount().SignIn() ) ) {
		gameLocal.Printf( "SDNet::SignIn : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_SignOut
================
*/
void sdNetManager::Script_SignOut( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	assert( networkService->GetActiveUser()->GetState() == sdNetUser::US_ONLINE );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::SignOut : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	CancelUserTasks();

	if ( !activeTask.Set( activeUser->GetAccount().SignOut() ) ) {
		gameLocal.Printf( "SDNet::SignOut : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_GetProfileString
================
*/
void sdNetManager::Script_GetProfileString( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr key;
	stack.Pop( key );

	idStr defaultValue;
	stack.Pop( defaultValue );

	sdNetUser* activeUser = networkService->GetActiveUser();

	stack.Push( activeUser->GetProfile().GetProperties().GetString( key.c_str(), defaultValue.c_str() ) );
}

/*
================
sdNetManager::Script_SetProfileString
================
*/
void sdNetManager::Script_SetProfileString( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr key, value;
	stack.Pop( key );
	stack.Pop( value );

	sdNetUser* activeUser = networkService->GetActiveUser();

	activeUser->GetProfile().GetProperties().Set( key.c_str(), value.c_str() );
	activeUser->Save( sdNetUser::SI_PROFILE );
}

#if !defined( SD_DEMO_BUILD )
/*
================
sdNetManager::Script_AssureProfileExists
================
*/
void sdNetManager::Script_AssureProfileExists( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	assert( networkService->GetActiveUser()->GetState() == sdNetUser::US_ONLINE );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::AssureProfileExists : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( !activeTask.Set( activeUser->GetProfile().AssureExists() ) ) {
		gameLocal.Printf( "SDNet::AssureProfileExists : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_StoreProfile
================
*/
void sdNetManager::Script_StoreProfile( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	assert( networkService->GetActiveUser()->GetState() == sdNetUser::US_ONLINE );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::StoreProfile : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( !activeTask.Set( activeUser->GetProfile().Store() ) ) {
		gameLocal.Printf( "SDNet::StoreProfile : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_RestoreProfile
================
*/
void sdNetManager::Script_RestoreProfile( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	assert( networkService->GetActiveUser()->GetState() == sdNetUser::US_ONLINE );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::RestoreProfile : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( !activeTask.Set( activeUser->GetProfile().Restore() ) ) {
		gameLocal.Printf( "SDNet::RestoreProfile : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}
#endif /* !SD_DEMO_BUILD */

/*
================
sdNetManager::Script_ValidateProfile
================
*/
void sdNetManager::Script_ValidateProfile( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idDict& properties = networkService->GetActiveUser()->GetProfile().GetProperties();

	stack.Push( true );
}

/*
================
sdNetManager::Script_FindServers
================
*/
void sdNetManager::Script_FindServers( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	findServerSource_e source;
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Error( "FindServers: source '%i' out of range", iSource );
		stack.Push( false );
		return;
	}

	if( source == FS_INTERNET && networkService->GetState() != sdNetService::SS_ONLINE ) {
		gameLocal.Warning( "FindServers: cannot search for internet servers while offline\n", iSource );
		stack.Push( false );
		return;
	}

	sdNetTask* task = NULL;
	idList< sdNetSession* >* netSessions = NULL;
	sdHotServerList* netHotServers;

	GetSessionsForServerSource( source, netSessions, task, netHotServers );

	if( netSessions == NULL ) {
		stack.Push( false );
		return;
	}

	if( netHotServers != NULL ) {
		netHotServers->Clear();
	}

	if ( task != NULL ) {
		gameLocal.Printf( "SDNet::FindServers : Already scanning for servers\n" );
		stack.Push( false );
		return;
	}

	for ( int i = 0; i < netSessions->Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( (*netSessions)[ i ] );
	}	

	netSessions->SetGranularity( 1024 );
	netSessions->SetNum( 0, false );
	lastServerUpdateIndex = 0;

#if !defined( SD_DEMO_BUILD )
	CacheServersWithFriends();
#endif /* !SD_DEMO_BUILD */

	sdNetSessionManager::sessionSource_e managerSource;
	switch ( source ) {
		case FS_LAN:
			managerSource = sdNetSessionManager::SS_LAN;
			break;
		case FS_INTERNET:
#if !defined( SD_DEMO_BUILD )
			managerSource = ShowRanked() ? sdNetSessionManager::SS_INTERNET_RANKED : sdNetSessionManager::SS_INTERNET_ALL;
#else
			managerSource = sdNetSessionManager::SS_INTERNET_ALL;
#endif /* !SD_DEMO_BUILD */
			break;

#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
		case FS_LAN_REPEATER:
			managerSource = sdNetSessionManager::SS_LAN_REPEATER;
			break;
		case FS_INTERNET_REPEATER:
			managerSource = sdNetSessionManager::SS_INTERNET_REPEATER;
			break;
#endif /* !SD_DEMO_BUILD && !SD_DEMO_BUILD_CONSTRUCTION */
	}
	if ( source != FS_HISTORY && source != FS_FAVORITES ) {
		task = networkService->GetSessionManager().FindSessions( *netSessions, managerSource );
		if ( task == NULL ) {
			gameLocal.Printf( "SDNet::FindServers : failed (%d)\n", networkService->GetLastError() );
			properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
			stack.Push( false );
			return;
		}
	}

	switch( source ) {
		case FS_INTERNET:
			findServersTask = task;
			break;

#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
		case FS_INTERNET_REPEATER:
			findRepeatersTask = task;
			break;
		case FS_LAN_REPEATER:
			findLANRepeatersTask = task;
			break;
#endif /* !SD_DEMO_BUILD && !SD_DEMO_BUILD_CONSTRUCTION */

		case FS_LAN:
			findLANServersTask = task;
			break;
		case FS_HISTORY: {
				assert( networkService->GetActiveUser() != NULL );
				const idDict& dict = networkService->GetActiveUser()->GetProfile().GetProperties();
				sessionsHistory.SetGranularity( sdNetUser::MAX_SERVER_HISTORY );

				for( int i = 0; i < sdNetUser::MAX_SERVER_HISTORY; i++ ) {
					const idKeyValue* kv = dict.FindKey( va( "serverHistory%i", i ) );
					if( kv == NULL || kv->GetValue().IsEmpty() ) {
						continue;
					}
					netadr_t addr;
					if( sys->StringToNetAdr( kv->GetValue(), &addr, false ) ) {
						sdNetSession* session = networkService->GetSessionManager().AllocSession( &addr );
						sessionsHistory.Append( session );
					}
				}
				findHistoryServersTask = task = networkService->GetSessionManager().RefreshSessions( sessionsHistory );
				if ( task == NULL ) {
					gameLocal.Printf( "SDNet::FindServers : failed (%d)\n", networkService->GetLastError() );
					properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
					stack.Push( false );
					return;
				}
			}
			break;
		case FS_FAVORITES: {
			assert( networkService->GetActiveUser() != NULL );
			const idDict& dict = networkService->GetActiveUser()->GetProfile().GetProperties();

			int prefixLength = idStr::Length( "favorite_" );
			const idKeyValue* kv = dict.MatchPrefix( "favorite_" );

			while( kv != NULL ) {
				const char* value = kv->GetKey().c_str();
				value += prefixLength;
				if( *value != '\0' ) {
					netadr_t addr;
					if( sys->StringToNetAdr( value, &addr, false ) ) {
						sdNetSession* session = networkService->GetSessionManager().AllocSession( &addr );
						sessionsFavorites.Append( session );
					}
				}

				kv = dict.MatchPrefix( "favorite_", kv );
			}
			findFavoriteServersTask = task = networkService->GetSessionManager().RefreshSessions( sessionsFavorites );
			if ( task == NULL ) {
				gameLocal.Printf( "SDNet::FindServers : failed (%d)\n", networkService->GetLastError() );
				properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
				stack.Push( false );
				return;
			}
		}
		break;
	}

	stack.Push( true );
}

/*
============
sdNetManager::Script_UpdateHotServers
============
*/
void sdNetManager::Script_UpdateHotServers( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	findServerSource_e source;
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Warning( "UpdateHotServers: invalid enum '%i'", iSource );
		return;
	}

	sdNetTask* task;
	idList< sdNetSession* >* netSessions = NULL;
	sdHotServerList* netHotServers;

	GetSessionsForServerSource( source, netSessions, task, netHotServers );

	if ( netSessions == NULL ) {
		return;
	}

	if( netHotServers != NULL ) {
		netHotServers->Update( *this );
	}	
}

/*
============
sdNetManager::Script_RefreshHotServers
============
*/
void sdNetManager::Script_RefreshHotServers( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	idStr ignore;
	stack.Pop( ignore );

	if ( refreshHotServerTask != NULL ) {
		stack.Push( 0.0f );
		return;
	}

	sdNetTask* task;
	idList< sdNetSession* >* netSessions = NULL;
	sdHotServerList* netHotServers;

	sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, hotServersRefreshSource );

	GetSessionsForServerSource( hotServersRefreshSource, netSessions, task, netHotServers );

	properties.SetHotServersRefreshComplete( false );

	if ( netSessions == NULL ) {
		properties.SetHotServersRefreshComplete( true );
		stack.Push( 0.0f );
		return;
	}

	if ( netHotServers == NULL ) {
		properties.SetHotServersRefreshComplete( true );
		stack.Push( 0.0f );
		return;
	}

	sdNetSession* ignoreSession = NULL;
	if ( ignore.Length() > 0 ) {
		sessionHash_t::Iterator iter = hashedSessions.Find( ignore );
		if( iter != hashedSessions.End() ) {
			if( task != NULL ) {
				task->AcquireLock();
			}

			int index = iter->second.sessionListIndex;
			if( index >= 0 && index < netSessions->Num() ) {
				ignoreSession = ( *netSessions )[ index ];
			}
			if( task != NULL ) {
				task->ReleaseLock();
			}
		}
	}

	for ( int i = 0; i < hotServerRefreshSessions.Num(); i++ ) {
		networkService->GetSessionManager().FreeSession( hotServerRefreshSessions[ i ] );
	}
	hotServerRefreshSessions.SetNum( 0, false );

	int now = sys->Milliseconds();

	int numAddresses = 0;
	netadr_t* addresses = ( netadr_t* )_alloca( sizeof( netadr_t ) * netHotServers->GetNumServers() );
	for ( int i = 0; i < netHotServers->GetNumServers(); i++ ) {
		sdNetSession* check = netHotServers->GetServer( i );
		if ( check == NULL || check == ignoreSession ) {
			continue;
		}

		// don't refresh a server too quickly
		if( ( check != NULL && check == serverRefreshSession ) ) {
			continue;
		}

		sessionHash_t::Iterator iter = hashedSessions.Find( check->GetHostAddressString() );
		if( iter != hashedSessions.End() ) {
			if( iter->second.lastUpdateTime + 2000 > now ) {
				continue;
			}
		}

		sys->StringToNetAdr( check->GetHostAddressString(), &addresses[ numAddresses ], false );
		numAddresses++;
	}

	if ( numAddresses == 0 ) {
		properties.SetHotServersRefreshComplete( true );
		stack.Push( 0.0f );
		return;
	}

	for ( int i = 0; i < numAddresses; i++ ) {
		hotServerRefreshSessions.Alloc() = networkService->GetSessionManager().AllocSession( &addresses[ i ] );
	}
	
	refreshHotServerTask = networkService->GetSessionManager().RefreshSessions( hotServerRefreshSessions );
	if ( refreshHotServerTask == NULL ) {
		gameLocal.Printf( "SDNet::RefreshHotServers : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		properties.SetHotServersRefreshComplete( true );
		stack.Push( 0.0f );
		return;
	}
	stack.Push( 1.0f );
}

/*
============
sdNetManager::Script_RefreshServer
============
*/
void sdNetManager::Script_RefreshServer( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	int sessionIndex;
	stack.Pop( sessionIndex );

	idStr sessionStr;
	stack.Pop( sessionStr );

	netadr_t addr;

	if( sessionStr.IsEmpty() ) {
		sdNetSession* netSession = GetSession( iSource, sessionIndex );

		if ( refreshServerTask != NULL ) {
			refreshServerTask->Cancel();
			networkService->FreeTask( refreshServerTask );
			refreshServerTask = NULL;
		}
		if( netSession == NULL ) {
			stack.Push( 0.0f );
			return;
		}

		sessionStr = netSession->GetHostAddressString();
	}

	sys->StringToNetAdr( sessionStr, &addr, false );	

	if( serverRefreshSession != NULL ) {
		networkService->GetSessionManager().FreeSession( serverRefreshSession );
		serverRefreshSession = NULL;
	}

	properties.SetServerRefreshComplete( false );

	// don't update too soon after a refresh
	sessionHash_t::Iterator iter = hashedSessions.Find( sessionStr );
	if( iter != hashedSessions.End() ) {
		int now = sys->Milliseconds();
		if( iter->second.lastUpdateTime + 2000 > now ) {			
			properties.SetServerRefreshComplete( true );
			stack.Push( 1.0f );
			return;
		}
	}

	serverRefreshSession = networkService->GetSessionManager().AllocSession( &addr );
	sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, serverRefreshSource );
	
	refreshServerTask = networkService->GetSessionManager().RefreshSession( *serverRefreshSession );

	if ( refreshServerTask == NULL ) {
		gameLocal.Printf( "SDNet::RefreshServer : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( 0.0f );
		return;
	}
	stack.Push( 1.0f );
}

/*
================
sdNetManager::Script_JoinServer
================
*/
void sdNetManager::Script_JoinServer( sdUIFunctionStack& stack ) {
	float fSource;
	stack.Pop( fSource );

	float sessionIndexFloat;
	stack.Pop( sessionIndexFloat );

	int sessionIndex = idMath::FtoiFast( sessionIndexFloat );

	sdNetSession* netSession = GetSession( fSource, sessionIndex );
	if( netSession != NULL ) {
		netSession->Join();
	}
}

#if !defined( SD_DEMO_BUILD )
/*
================
sdNetManager::Script_CheckKey
================
*/
void sdNetManager::Script_CheckKey( sdUIFunctionStack& stack ) {
	idStr keyCode;
	idStr checksum;
	stack.Pop( keyCode );
	stack.Pop( checksum );

	stack.Push( networkService->CheckKey( va( "%s %s", keyCode.c_str(), checksum.c_str() ) ) ? 1.0f : 0.0f );
}

/*
============
sdNetManager::Script_Friend_Init
============
*/
void sdNetManager::Script_Friend_Init( sdUIFunctionStack& stack ) {
	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendInit : task pending\n" );
		return;
	}

	initFriendsTask = networkService->GetFriendsManager().Init();
	if ( initFriendsTask == NULL ) {
		gameLocal.Printf( "SDNet::FriendInit : failed (%d)\n", networkService->GetLastError() );
	}
}

/*
============
sdNetManager::Script_Friend_ProposeFriendShip
============
*/
void sdNetManager::Script_Friend_ProposeFriendShip( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	idWStr reason;
	stack.Pop( reason );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendProposeFriendShip : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetFriendsManager().ProposeFriendship( userName, reason.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::FriendProposeFriendShip : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_AcceptProposal
============
*/
void sdNetManager::Script_Friend_AcceptProposal( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendAcceptProposal : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetFriendsManager().AcceptProposal( userName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::FriendAcceptProposal : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_RejectProposal
============
*/
void sdNetManager::Script_Friend_RejectProposal( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendRejectProposal : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetFriendsManager().RejectProposal( userName ) ) ) {
		gameLocal.Printf( "SDNet::FriendRejectProposal : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_RemoveFriend
============
*/
void sdNetManager::Script_Friend_RemoveFriend( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendRemoveFriend : task pending\n" );
		return;
	}
	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& invited = networkService->GetFriendsManager().GetInvitedFriendsList();

	// if the friend is pending, withdraw the proposal
	if( networkService->GetFriendsManager().FindFriend( invited, userName.c_str() ) != NULL ) {
		if ( !activeTask.Set( networkService->GetFriendsManager().WithdrawProposal( userName ) ) ) {
			gameLocal.Printf( "SDNet::FriendRemoveFriend : failed (%d)\n", networkService->GetLastError() );
			properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

			return;
		}
	} else {
		if ( !activeTask.Set( networkService->GetFriendsManager().RemoveFriend( userName ) ) ) {
			gameLocal.Printf( "SDNet::FriendRemoveFriend : failed (%d)\n", networkService->GetLastError() );
			properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

			return;
		}

	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_SetBlockedStatus
============
*/
void sdNetManager::Script_Friend_SetBlockedStatus( sdUIFunctionStack& stack ) {
	idStr userName;
	int status;
	stack.Pop( userName );
	stack.Pop( status );

	assert( status >= sdNetFriend::BS_NO_BLOCK && status < sdNetFriend::BS_END );

	sdNetFriend::blockState_e blockStatus = static_cast< sdNetFriend::blockState_e >( status );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendSetBlockedStatus : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetFriendsManager().SetBlockedStatus( userName, blockStatus ) ) ) {
		gameLocal.Printf( "SDNet::FriendSetBlockedStatus : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_SendIM
============
*/
void sdNetManager::Script_Friend_SendIM( sdUIFunctionStack& stack ) {
	idStr userName;
	idWStr message;

	stack.Pop( userName );
	stack.Pop( message );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendSendIM : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetFriendsManager().SendMessage( userName, message.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::FriendSendIM : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_GetIMText
============
*/
void sdNetManager::Script_Friend_GetIMText( sdUIFunctionStack& stack ) {
	idStr userName;

	stack.Pop( userName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();

	if ( sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, userName ) ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		if( ims.Num() ) {
			stack.Push( ims[ 0 ]->GetText() );
			return;
		}
	}
	stack.Push( L"" );
}

/*
============
sdNetManager::Script_Friend_GetProposalText
============
*/
void sdNetManager::Script_Friend_GetProposalText( sdUIFunctionStack& stack ) {
	idStr userName;

	stack.Pop( userName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetPendingFriendsList();

	if ( sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, userName ) ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_FRIENDSHIP_PROPOSAL );
		if( ims.Num() ) {
			stack.Push( ims[ 0 ]->GetText() );
			return;
		}
	}
	stack.Push( L"" );
}

/*
============
sdNetManager::Script_Friend_InviteFriend
============
*/
void sdNetManager::Script_Friend_InviteFriend( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendInviteFriend : task pending\n" );
		return;
	}

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( networkService->GetFriendsManager().GetFriendsList(), userName.c_str() );

	if ( !activeTask.Set( networkService->GetFriendsManager().Invite( userName.c_str(), networkService->GetSessionManager().GetCurrentSessionAddress() ), &sdNetManager::OnFriendInvited, mate ) ) {
		gameLocal.Printf( "SDNet::FriendInviteFriend : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::OnFriendInvited
============
*/
void sdNetManager::OnFriendInvited( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() == SDNET_NO_ERROR ) {
		sdNetFriend* mate = reinterpret_cast< sdNetFriend* >( parm );

		sdNetClientId netClientId;

		mate->GetNetClientId( netClientId );

		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_RESERVECLIENTSLOT );
		msg.WriteLong( netClientId.id[0] );
		msg.WriteLong( netClientId.id[1] );
		msg.Send();
	}
}

/*
============
sdNetManager::Script_Friend_ChooseContextAction
============
*/
void sdNetManager::Script_Friend_ChooseContextAction( sdUIFunctionStack& stack ) {
	assert( activeMessage == NULL );

	idStr friendName;
	stack.Pop( friendName );
	if ( friendName.IsEmpty() ) {
		stack.Push( sdNetProperties::FCA_NONE );
		return;
	}

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& pending = networkService->GetFriendsManager().GetPendingFriendsList();
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( pending, friendName );
	if ( mate != NULL ) {
		sdNetMessage* message = mate->GetMessageQueue().GetMessages();
		if ( message != NULL ) {
			switch( message->GetType() ) {
				case sdNetMessage::MT_FRIENDSHIP_PROPOSAL:
					activeMessage = message;
					stack.Push( sdNetProperties::FCA_RESPOND_TO_PROPOSAL );
					return;
				default:
					assert( 0 );
			}
		}

		// nothing to do
		stack.Push( sdNetProperties::FCA_NONE );
		return;
	}

	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	mate = networkService->GetFriendsManager().FindFriend( friends, friendName );
	if ( mate != NULL ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		if( ims.Num() ) {
			activeMessage = ims[ 0 ];
			stack.Push( sdNetProperties::FCA_READ_IM );
			return;
		} else {
			sdNetMessage* message = mate->GetMessageQueue().GetMessages();
			if ( message != NULL ) {
				switch( message->GetType() ) {
				case sdNetMessage::MT_BLOCKED:
					activeMessage = message;
					stack.Push( sdNetProperties::FCA_BLOCKED );
					return;
				case sdNetMessage::MT_UNBLOCKED:
					activeMessage = message;
					stack.Push( sdNetProperties::FCA_UNBLOCKED );
					return;
				case sdNetMessage::MT_SESSION_INVITE:
					activeMessage = message;
					stack.Push( sdNetProperties::FCA_RESPOND_TO_INVITE );
					return;
				}
			}
		}

		// nothing to do, send on a message
		stack.Push( sdNetProperties::FCA_SEND_IM );
		return;
	}

	const sdNetFriendsList& blocked = networkService->GetFriendsManager().GetBlockedList();
	mate = networkService->GetFriendsManager().FindFriend( blocked, friendName );
	if ( mate != NULL ) {
		int x = 0;
	}

	stack.Push( sdNetProperties::FCA_NONE );
}

/*
============
sdNetManager::Script_Friend_NumAvailableIMs
============
*/
void sdNetManager::Script_Friend_NumAvailableIMs( sdUIFunctionStack& stack ) {

	idStr friendName;
	stack.Pop( friendName );
	if ( friendName.IsEmpty() ) {
		stack.Push( 0 );
		return;
	}

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName );
	if ( mate != NULL ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		stack.Push( ims.Num() );
		return;
	}

	stack.Push( 0 );
}

/*
============
sdNetManager::Script_DeleteActiveMessage
============
*/
void sdNetManager::Script_DeleteActiveMessage( sdUIFunctionStack& stack ) {
	assert( activeMessage != NULL );
	assert( activeMessage->GetSenderQueue() != NULL );

	if ( sdNetMessageQueue* senderQueue = activeMessage->GetSenderQueue() ) {
		senderQueue->RemoveMessage( activeMessage );
	}
	activeMessage = NULL;
}

/*
============
sdNetManager::Script_IsFriend
============
*/
void sdNetManager::Script_IsFriend( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	float result = 0.0f;

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	const sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );

	stack.Push( mate == NULL ? 0.0f : 1.0f );
}

/*
============
sdNetManager::Script_IsPendingFriend
============
*/
void sdNetManager::Script_IsPendingFriend( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	float result = 0.0f;

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetPendingFriendsList();
	const sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );

	stack.Push( mate != NULL );
}

/*
============
sdNetManager::Script_IsInvitedFriend
============
*/
void sdNetManager::Script_IsInvitedFriend( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	float result = 0.0f;

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetInvitedFriendsList();
	const sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );

	stack.Push( mate != NULL );
}

/*
============
sdNetManager::Script_Friend_GetBlockedStatus
============
*/
void sdNetManager::Script_Friend_GetBlockedStatus( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& blockedFriends = networkService->GetFriendsManager().GetBlockedList();
	const sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( blockedFriends, friendName.c_str() );
	if( mate == NULL ) {
		stack.Push( sdNetFriend::BS_NO_BLOCK );
		return;
	}

	stack.Push( mate->GetBlockedState() );
}
#endif /* !SD_DEMO_BUILD */

/*
============
sdNetManager::Script_QueryServerInfo
============
*/
void sdNetManager::Script_QueryServerInfo( sdUIFunctionStack& stack ) {
	float fSource;
	stack.Pop( fSource );

	float sessionIndexFloat;
	stack.Pop( sessionIndexFloat );

	idStr key;
	stack.Pop( key );

	idStr defaultValue;
	stack.Pop( defaultValue );

	if( fSource == FS_CURRENT ) {
		if( key.Icmp( "_address" ) == 0 ) {
			netadr_t addr = networkSystem->ClientGetServerAddress();
			if( addr.type == NA_BAD ) {
				stack.Push( defaultValue );
				return;
			}
			stack.Push( sys->NetAdrToString( addr ) );
			return;
		}

		if( key.Icmp( "_ranked" ) == 0 ) {
			stack.Push( networkSystem->IsRankedServer() ? "1" : "0" );
			return;
		}

		if( key.Icmp( "_repeater" ) == 0 ) {
			stack.Push( gameLocal.serverIsRepeater ? "1" : "0" );
			return;
		}

		if( key.Icmp( "_time" ) == 0 ) {
			if ( gameLocal.rules != NULL ) {
				stack.Push( idStr::MS2HMS( gameLocal.rules->GetGameTime() ) );
				return;
			}
		}

		stack.Push( gameLocal.serverInfo.GetString( key.c_str(), defaultValue.c_str() ) );
		return;
	}

	sdNetSession* netSession = GetSession( fSource, idMath::FtoiFast( sessionIndexFloat ) );

	if( netSession == NULL ) {
		stack.Push( defaultValue.c_str() );
		return;
	}

	if( key.Icmp( "_time" ) == 0 ) {
		stack.Push( idStr::MS2HMS( netSession->GetSessionTime() ) );
		return;
	}

	if( key.Icmp( "_ranked" ) == 0 ) {
		stack.Push( netSession->IsRanked() ? "1" : "0" );
		return;
	}

	if( key.Icmp( "_repeater" ) == 0 ) {
		stack.Push( netSession->IsRepeater() ? "1" : "0" );
		return;
	}

	if( key.Icmp( "_address" ) == 0 ) {		
		stack.Push( netSession->GetHostAddressString() );
		return;
	}

	stack.Push( netSession->GetServerInfo().GetString( key.c_str(), defaultValue.c_str() ) );
}

/*
============
sdNetManager::Script_QueryMapInfo
============
*/
void sdNetManager::Script_QueryMapInfo( sdUIFunctionStack& stack ) {
	float fSource;
	stack.Pop( fSource );

	float sessionIndexFloat;
	stack.Pop( sessionIndexFloat );

	idStr key;
	stack.Pop( key );

	idStr defaultValue;
	stack.Pop( defaultValue );

	const sdNetSession* netSession = GetSession( fSource, idMath::FtoiFast( sessionIndexFloat ) );

	if( netSession == NULL ) {
		stack.Push( defaultValue.c_str() );
		return;
	}

	const idDict* mapInfo = GetMapInfo( *netSession );

	if( mapInfo == NULL ) {
		stack.Push( defaultValue.c_str() );
		return;
	}

	stack.Push( mapInfo->GetString( key.c_str(), defaultValue.c_str() ) );
}


/*
============
sdNetManager::Script_QueryGameType
============
*/
void sdNetManager::Script_QueryGameType( sdUIFunctionStack& stack ) {
	float fSource;
	stack.Pop( fSource );

	float sessionIndexFloat;
	stack.Pop( sessionIndexFloat );

	sdNetSession* netSession = GetSession( fSource, idMath::FtoiFast( sessionIndexFloat ) );
	if( netSession == NULL ) {
		stack.Push( L"" );
		return;
	}

	const char* siRules = netSession->GetServerInfo().GetString( "si_rules" );
	idWStr gameType;
	GetGameType( siRules, gameType );

	stack.Push( gameType.c_str() );
}


/*
============
sdNetManager::GetMapInfo
============
*/
const idDict* sdNetManager::GetMapInfo( const sdNetSession& netSession ) {
	idStr mapName = netSession.GetServerInfo().GetString( "si_map" );
	mapName.StripFileExtension();

	return gameLocal.mapMetaDataList->FindMetaData( mapName );
}

/*
============
sdNetManager::GetSession
============
*/
sdNetSession* sdNetManager::GetSession( float fSource, int index ) {
	int iSource = idMath::Ftoi( fSource );
	if( iSource < sdNetManager::FS_LAN || iSource >= sdNetManager::FS_MAX ) {
		gameLocal.Error( "GetSession: source '%i' out of range", iSource );
		return NULL;
	}

	if( iSource == FS_CURRENT ) {
		return gameSession;
	}

	idList< sdNetSession* >* netSessions = NULL;
	sdNetTask* task;
	sdHotServerList* netHotServers;

	GetSessionsForServerSource( static_cast< findServerSource_e >( iSource ), netSessions, task, netHotServers );

	if( netSessions == NULL || index < 0 || index >= netSessions->Num() ) {
		return NULL;
	}

	if( task != NULL ) {
		task->AcquireLock();
	}

	sdNetSession* netSession = (*netSessions)[ index ];

	if( task != NULL ) {
		task->ReleaseLock();
	}

	return netSession;
}

/*
============
sdNetManager::Script_RequestStats
============
*/
void sdNetManager::Script_RequestStats( sdUIFunctionStack& stack ) {
	bool globalOnly;
	stack.Pop( globalOnly );
	stack.Push( sdGlobalStatsTracker::GetInstance().StartStatsRequest( globalOnly ) );
}

/*
============
sdNetManager::Script_GetStat
============
*/
void sdNetManager::Script_GetStat( sdUIFunctionStack& stack ) {
	idStr key;
	stack.Pop( key );

	const sdNetStatKeyValList& stats = sdGlobalStatsTracker::GetInstance().GetLocalStats();
	for( int i = 0; i < stats.Num(); i++ ) {
		const sdNetStatKeyValue& kv = stats[ i ];
		if( kv.key->Icmp( key.c_str() ) == 0 ) {
			switch( kv.type ) {
				case sdNetStatKeyValue::SVT_FLOAT_MAX:
				case sdNetStatKeyValue::SVT_FLOAT:
					stack.Push( kv.val.f );
					return;
				case sdNetStatKeyValue::SVT_INT_MAX:
				case sdNetStatKeyValue::SVT_INT:
					stack.Push( kv.val.i );
					return;
			}
		}
	}
	stack.Push( 0.0f );
}

/*
============
sdNetManager::Script_GetPlayerAward
============
*/
void sdNetManager::Script_GetPlayerAward( sdUIFunctionStack& stack ) {
	int iReward;
	stack.Pop( iReward );

	int iStat;
	stack.Pop( iStat );

	bool self;
	stack.Pop( self );

	const idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer == NULL && self ) {
		stack.Push( L"" );
		return;
	}


	playerReward_e reward;
	if( !sdIntToContinuousEnum< playerReward_e >( iReward, PR_MIN, PR_MAX, reward ) || iReward >= gameLocal.endGameStats.Num() ) {
		stack.Push( L"" );
		return;
	}

	if( gameLocal.endGameStats[ iReward ].name.IsEmpty() ) {
		stack.Push( L"" );
		return;
	}

	playerStat_e stat;
	if( sdIntToContinuousEnum< playerStat_e >( iStat, PS_MIN, PS_MAX, stat ) ) {
		switch( stat ) {
			case PS_NAME:
				assert( self == false );
				stack.Push( va( L"%hs", gameLocal.endGameStats[ iReward ].name.c_str() ) );
				break;
			case PS_XP:
				if ( iReward == PR_LEAST_XP ) {
					stack.Push( L"" );
				} else {
					float value = self ? gameLocal.endGameStats[ iReward ].data[ localPlayer->entityNumber ] : gameLocal.endGameStats[ iReward ].value;
					if( self && value < 0.0f ) {
						stack.Push( L"" );
					}
					if ( value >= 1000.f ) {
						idWStrList parms;
						parms.Alloc() = va( L"%.1f", ( value / 1000.f ) );
						idWStr result = common->LocalizeText( "guis/mainmenu/kilo", parms ).c_str();
						if( iReward == PR_ACCURACY_HIGH ) {
							result += L"%";
						}
						stack.Push( result.c_str() );
					} else {
						if( iReward == PR_ACCURACY_HIGH ) {
							stack.Push( va( L"%i%%", idMath::Ftoi( value ) ) );
						} else {
							stack.Push( va( L"%i", idMath::Ftoi( value ) ) );
						}
					}
				}
				break;
			case PS_RANK: {
				assert( self == false );
				const sdDeclRank* rank = gameLocal.endGameStats[ iReward ].rank;
				if( rank != NULL ) {
					stack.Push( va( L"%hs", rank->GetMaterial() ) );
				} else {
					stack.Push( L"_default" );
				}
			}
			break;
			case PS_TEAM: {
				assert( self == false );
				const sdTeamInfo* team = gameLocal.endGameStats[ iReward ].team;
				if( team != NULL ) {
					stack.Push( va( L"%hs", team->GetLookupName() ) );
				} else {
					stack.Push( L"" );
				}
			}
			  break;
		}
	} else {
		stack.Push( L"" );
	}
}

/*
============
sdNetManager::Script_QueryXPStats
============
*/
void sdNetManager::Script_QueryXPStats( sdUIFunctionStack& stack ) {
	idStr profKey;
	stack.Pop( profKey );

	bool total;
	stack.Pop( total );

	if( profKey.IsEmpty() ) {
		gameLocal.Warning( "sdNetManager::Script_QueryXPStats: empty proficiency key" );
		stack.Push( "" );
		return;
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer != NULL ) {
		const sdProficiencyTable& table = localPlayer->GetProficiencyTable();
		const sdDeclProficiencyType* prof = gameLocal.declProficiencyTypeType[ profKey.c_str() ];

		float xp;
		if ( prof == NULL ) {
			xp = 0.f;
		} else {
			int index = prof->Index();
			if( total ) {
				xp = table.GetPoints( index );
			} else {
				xp = table.GetPointsSinceBase( index );
			}
		}
		stack.Push( va( "%i", idMath::Ftoi( xp ) ) );
		return;
	}
	stack.Push( "" );
}

/*
============
sdNetManager::Script_QueryXPTotals
============
*/
void sdNetManager::Script_QueryXPTotals( sdUIFunctionStack& stack ) {
	bool total;
	stack.Pop( total );
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer != NULL ) {
		const sdProficiencyTable& table = localPlayer->GetProficiencyTable();

		float sum = 0;
		const sdGameRules::mapData_t* data = gameLocal.rules->GetCurrentMapData();
		for( int i = 0; i < gameLocal.declProficiencyTypeType.Num(); i++ ) {
			const sdDeclProficiencyType* prof = gameLocal.declProficiencyTypeType.LocalFindByIndex( i );

			if( total ) {
				sum += table.GetPoints( prof->Index() );
			} else {
				sum += table.GetPointsSinceBase( prof->Index() );
			}
		}
		stack.Push( va( "%i", idMath::Ftoi( sum ) ) );
		return;
	}

	stack.Push( "" );
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_Team_Init
============
*/
void sdNetManager::Script_Team_Init( sdUIFunctionStack& stack ) {
	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamInit : task pending\n" );
		return;
	}

	initTeamsTask = networkService->GetTeamManager().Init();
	if ( initTeamsTask == NULL ) {
		gameLocal.Printf( "SDNet::TeamInit : failed (%d)\n", networkService->GetLastError() );
	}
}


/*
============
sdNetManager::Script_Team_CreateTeam
============
*/
void sdNetManager::Script_Team_CreateTeam( sdUIFunctionStack& stack ) {
	idStr teamName;
	stack.Pop( teamName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::CreateTeam : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().CreateTeam( teamName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::CreateTeam : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_ProposeMembership
============
*/
void sdNetManager::Script_Team_ProposeMembership( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	idWStr reason;
	stack.Pop( reason );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamProposeMembership : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().ProposeMembership( userName.c_str(), reason.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamProposeMembership : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_AcceptMembership
============
*/
void sdNetManager::Script_Team_AcceptMembership( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	assert( activeMessage == NULL );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamAcceptMembership : task pending\n" );
		stack.Push( false );
		return;
	}

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& list = networkService->GetTeamManager().GetPendingInvitesList();
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( list, userName.c_str() );
	if( mate == NULL ) {
		gameLocal.Printf( "SDNet::TeamAcceptMembership : couldn't find '%s'\n", userName.c_str() );
		stack.Push( false );
		return;
	}

	sdNetMessage* message = mate->GetMessageQueue().GetMessages();
	if( message == NULL || message->GetType() != sdNetMessage::MT_MEMBERSHIP_PROPOSAL ) {
		gameLocal.Printf( "SDNet::TeamAcceptMembership : message wasn't a membership proposal\n" );
		stack.Push( false );
		return;
	}
	assert( message->GetDataSize() == sizeof( sdNetTeamInvite ) );

	const sdNetTeamInvite& teamInvite = reinterpret_cast< const sdNetTeamInvite& >( *message->GetData() );

	if ( !activeTask.Set( networkService->GetTeamManager().AcceptMembership( userName, teamInvite.netTeamId ) ) ) {
		gameLocal.Printf( "SDNet::TeamAcceptMembership : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( false );
		return;
	}
	activeMessage = message;

	properties.SetTaskActive( true );
	stack.Push( true );
}

/*
============
sdNetManager::Script_Team_RejectMembership
============
*/
void sdNetManager::Script_Team_RejectMembership( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	assert( activeMessage == NULL );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamRejectMembership : task pending\n" );
		stack.Push( 0 );
		return;
	}

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( networkService->GetTeamManager().GetPendingInvitesList(), userName.c_str() );
	if( mate == NULL ) {
		gameLocal.Printf( "SDNet::TeamRejectMembership : couldn't find '%s'\n", userName.c_str() );
		stack.Push( 0 );
		return;
	}

	sdNetMessage* message = mate->GetMessageQueue().GetMessages();
	if( message == NULL || message->GetType() != sdNetMessage::MT_MEMBERSHIP_PROPOSAL ) {
		gameLocal.Printf( "SDNet::TeamRejectMembership : message wasn't a membership proposal\n" );
		stack.Push( 0 );
		return;
	}

	const sdNetTeamInvite& teamInvite = reinterpret_cast< const sdNetTeamInvite& >( *message->GetData() );

	if ( !activeTask.Set( networkService->GetTeamManager().RejectMembership( userName, teamInvite.netTeamId ) ) ) {
		gameLocal.Printf( "SDNet::TeamRejectMembership : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( 0 );
		return;
	}

	activeMessage = message;

	properties.SetTaskActive( true );
	stack.Push( 1 );
}

/*
============
sdNetManager::Script_Team_RemoveMember
============
*/
void sdNetManager::Script_Team_RemoveMember( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamRemoveMember : task pending\n" );
		return;
	}

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& pending = networkService->GetTeamManager().GetPendingMemberList();
	if( networkService->GetTeamManager().FindMember( pending, userName.c_str() ) != NULL ) {
		if ( !activeTask.Set( networkService->GetTeamManager().WithdrawMembership( userName.c_str() ) ) ) {
			gameLocal.Printf( "SDNet::TeamRemoveMember : failed (%d)\n", networkService->GetLastError() );
			properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

			return;
		}
	} else {
		if ( !activeTask.Set( networkService->GetTeamManager().RemoveMember( userName.c_str() ) ) ) {
			gameLocal.Printf( "SDNet::TeamRemoveMember : failed (%d)\n", networkService->GetLastError() );
			properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

			return;
		}
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_SendMessage
============
*/
void sdNetManager::Script_Team_SendMessage( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	idWStr text;
	stack.Pop( text );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamSendMessage : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().SendMessage( userName.c_str(), text.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamSendMessage : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_BroadcastMessage
============
*/
void sdNetManager::Script_Team_BroadcastMessage( sdUIFunctionStack& stack ) {
	idWStr text;
	stack.Pop( text );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamBroadcastMessage : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().BroadcastMessage( text.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamBroadcastMessage : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_Invite
============
*/
void sdNetManager::Script_Team_Invite( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamInvite : task pending\n" );
		return;
	}

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( networkService->GetTeamManager().GetMemberList(), userName.c_str() );

	if ( !activeTask.Set( networkService->GetTeamManager().Invite( userName.c_str(), networkService->GetSessionManager().GetCurrentSessionAddress() ), &sdNetManager::OnMemberInvited, mate ) ) {
		gameLocal.Printf( "SDNet::TeamInvite : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::OnMemberInvited
============
*/
void sdNetManager::OnMemberInvited( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() == SDNET_NO_ERROR ) {
		sdNetTeamMember* mate = reinterpret_cast< sdNetTeamMember* >( parm );

		sdNetClientId netClientId;

		mate->GetNetClientId( netClientId );

		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_RESERVECLIENTSLOT );
		msg.WriteLong( netClientId.id[0] );
		msg.WriteLong( netClientId.id[1] );
		msg.Send();
	}
}

/*
============
sdNetManager::Script_Team_PromoteMember
============
*/
void sdNetManager::Script_Team_PromoteMember( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamPromoteMember : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().PromoteMember( userName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamPromoteMember : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_DemoteMember
============
*/
void sdNetManager::Script_Team_DemoteMember( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamDemoteMember : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().DemoteMember( userName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamDemoteMember : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_TransferOwnership
============
*/
void sdNetManager::Script_Team_TransferOwnership( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamTransferOwnership : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().TransferOwnership( userName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamTransferOwnership : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_DisbandTeam
============
*/
void sdNetManager::Script_Team_DisbandTeam( sdUIFunctionStack& stack ) {
	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamDisbandTeam : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().DisbandTeam() ) ) {
		gameLocal.Printf( "SDNet::TeamDisbandTeam : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_LeaveTeam
============
*/
void sdNetManager::Script_Team_LeaveTeam( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamLeaveTeam : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().LeaveTeam( userName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamLeaveTeam : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}
#endif /* !SD_DEMO_BUILD */

/*
============
sdNetManager::Script_ApplyNumericFilter
============
*/
void sdNetManager::Script_ApplyNumericFilter( sdUIFunctionStack& stack ) {
	float type;
	stack.Pop( type );

	float state;
	stack.Pop( state );

	float op;
	stack.Pop( op );

	float bin;
	stack.Pop( bin );

	bool success = true;
	serverNumericFilter_t& filter = *numericFilters.Alloc();

	success &= sdFloatToContinuousEnum< serverFilter_e >( type, SF_MIN, SF_MAX, filter.type );
	success &= sdFloatToContinuousEnum< serverFilterState_e >( state, SFS_MIN, SFS_MAX, filter.state );
	success &= sdFloatToContinuousEnum< serverFilterOp_e >( op, SFO_MIN, SFO_MAX, filter.op );
	success &= sdFloatToContinuousEnum< serverFilterResult_e >( bin, SFR_MIN, SFR_MAX, filter.resultBin );
	stack.Pop( filter.value );

	if( !success ) {
		gameLocal.Warning( "ApplyNumericFilter: invalid enum, filter not applied" );
		numericFilters.RemoveIndexFast( numericFilters.Num() - 1 );
	}
}

/*
============
sdNetManager::Script_ApplyStringFilter
============
*/
void sdNetManager::Script_ApplyStringFilter( sdUIFunctionStack& stack ) {
	idStr cvar;
	stack.Pop( cvar );

	float state;
	stack.Pop( state );

	float op;
	stack.Pop( op );

	float bin;
	stack.Pop( bin );

	bool success = true;
	serverStringFilter_t& filter = *stringFilters.Alloc();
	success &= sdFloatToContinuousEnum< serverFilterState_e >( state, SFS_MIN, SFS_MAX, filter.state );
	success &= sdFloatToContinuousEnum< serverFilterOp_e >( op, SFO_MIN, SFO_MAX, filter.op );
	success &= sdFloatToContinuousEnum< serverFilterResult_e >( bin, SFR_MIN, SFR_MAX, filter.resultBin );
	filter.cvar = cvar;
	stack.Pop( filter.value );

	if( !success ) {
		gameLocal.Warning( "ApplyStringFilter: invalid enum, filter not applied" );
		stringFilters.RemoveIndexFast( stringFilters.Num() - 1 );
	}
}

/*
============
sdNetManager::Script_ClearFilters
============
*/
void sdNetManager::Script_ClearFilters( sdUIFunctionStack& stack ) {
	numericFilters.Clear();
	stringFilters.Clear();
}


/*
============
sdNetManager::DoFiltering
============
*/
bool sdNetManager::DoFiltering( const sdNetSession& netSession ) const {
	for( int i = 0; i < unfilteredSessions.Num(); i++ ) {
		const netadr_t& addr = unfilteredSessions[ i ];
		const netadr_t& other = netSession.GetAddress();
		if( addr == other ) {
			return false;
		}
	}
	return true;
}

/*
============
sdNetManager::SessionIsFiltered
============
*/
bool sdNetManager::SessionIsFiltered( const sdNetSession& netSession, bool ignoreEmptyFilter ) const {
	if( numericFilters.Empty() && stringFilters.Empty() ) {
		return false;
	}

	assert( networkService->GetActiveUser() != NULL );
	sdNetUser* activeUser = networkService->GetActiveUser();

	bool visible = true;
	bool orFilters = false;
	bool orSet = false;

	for( int i = 0; i < numericFilters.Num() && visible; i++ ) {
		const serverNumericFilter_t& filter = numericFilters[ i ];
		if ( filter.state == SFS_DONTCARE ) {
			continue;
		}

		if ( filter.type == SF_EMPTY && ignoreEmptyFilter ) {
			continue;
		}


		// jrad - this is superseded by SF_MAXBOTS, but left in for backwards compatibility while loading profiles
		if( filter.type == SF_BOTS ) {
			continue;
		}

		// don't filter by most items for repeaters
		if( netSession.IsRepeater() ) {
			if( filter.type != SF_FULL &&
				filter.type != SF_EMPTY &&
				filter.type != SF_PING && 
#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
				filter.type != SF_FRIENDS &&
#endif /* !SD_DEMO_BUILD && !SD_DEMO_BUILD_CONSTRUCTION */
				filter.type != SF_MODS &&
				filter.type != SF_PLAYERCOUNT ) {
				continue;
			}
		}

		float value = 0.0f;
		switch( filter.type ) {
			case SF_PASSWORDED:
				value = netSession.GetServerInfo().GetBool( "si_needPass", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_PUNKBUSTER:
				value = netSession.GetServerInfo().GetBool( "net_serverPunkbusterEnabled", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_FRIENDLYFIRE:
				value = netSession.GetServerInfo().GetBool( "si_teamDamage", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_AUTOBALANCE:	
				value = netSession.GetServerInfo().GetBool( "si_teamForceBalance", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_PURE:
				value = netSession.GetServerInfo().GetBool( "si_pure", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_LATEJOIN:
				value = netSession.GetServerInfo().GetBool( "si_allowLateJoin", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_EMPTY: {
					int num = netSession.IsRepeater() ? netSession.GetNumRepeaterClients() : netSession.GetNumClients();
					value = ( num == 0 ) ? 1.0f : 0.0f;
				}
				break;
			case SF_FULL: {
					int num = netSession.IsRepeater() ? netSession.GetNumRepeaterClients() : netSession.GetNumClients();
					int max = netSession.IsRepeater() ? netSession.GetMaxRepeaterClients() : netSession.GetServerInfo().GetInt( "si_maxPlayers" );
					value = ( num == max ) ? 1.0f : 0.0f;
				}
				break;
			case SF_PING:
				value = netSession.GetPing();
				break;
/* jrad - this is superseded by SF_MAXBOTS, but left in for backwards compatibility while loading profiles
			case SF_BOTS:
				value = ( ( ( netSession.GetNumClients() - netSession.GetNumBotClients() ) == 0 ) && ( netSession.GetNumBotClients() > 0 ) ) ? 1.0f : 0.0f;
				break;
*/
			case SF_MAXBOTS:
				value = netSession.GetNumBotClients();
				break;
			case SF_FAVORITE:
				value = activeUser->GetProfile().GetProperties().GetBool( va( "favorite_%s", netSession.GetHostAddressString() ), "0" );
				break;
#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
			case SF_RANKED:
				value = netSession.IsRanked() ? 1.0f : 0.0f;
				break;
			case SF_FRIENDS:
				value = AnyFriendsOnServer( netSession ) ? 1.0f : 0.0f;
				break;
#endif /* !SD_DEMO_BUILD && !SD_DEMO_BUILD_CONSTRUCTION */
			case SF_PLAYERCOUNT: {
					value = netSession.IsRepeater() ? netSession.GetNumRepeaterClients() : ( netSession.GetNumClients() - netSession.GetNumBotClients() );
				}
				break;
			case SF_MODS:
				value = ( netSession.GetServerInfo().GetString( "fs_game", "" )[ 0 ] != '\0' ) ? 1.0f : 0.0f;
				break;
		}

		bool result = false;
		switch( filter.op ) {
			case SFO_EQUAL:
				result = idMath::Fabs( value - filter.value ) < idMath::FLT_EPSILON;
				break;
			case SFO_NOT_EQUAL:
				result = idMath::Fabs( value - filter.value ) >= idMath::FLT_EPSILON;
				break;
			case SFO_LESS:
				result = value < filter.value;
				break;
			case SFO_GREATER:
				result = value > filter.value;
				break;
		}

		if( filter.state == SFS_SHOWONLY && !result ) {
			visible = false;
			break;
		}

		if( filter.resultBin == SFR_OR ) {
			orFilters |= result;
			orSet = true;
		} else if( result && filter.state == SFS_HIDE ) {
			visible = false;
			break;
		}
	}

	for( int i = 0; i < stringFilters.Num() && visible; i++ ) {
		const serverStringFilter_t& filter = stringFilters[ i ];
		if( filter.state == SFS_DONTCARE ) {
			continue;
		}

		const char* value = netSession.GetServerInfo().GetString( filter.cvar.c_str() );

		// allow for filtering the pretty name
		if( gameLocal.mapMetaDataList != NULL && filter.cvar.Icmp( "si_map" ) == 0 ) {
			idStr prettyName( value );
			prettyName.StripFileExtension();
			if ( const idDict* mapInfo = gameLocal.mapMetaDataList->FindMetaData( prettyName ) ) {
				value = mapInfo->GetString( "pretty_name", value );
			}
		}
		builder.Clear();
		builder.AppendNoColors( value );

		bool result = false;

		switch( filter.op ) {
			case SFO_EQUAL:
				result = idStr::IcmpNoColor( builder.c_str(), filter.value.c_str() ) == 0;
				break;
			case SFO_NOT_EQUAL:
				result = idStr::IcmpNoColor( builder.c_str(), filter.value.c_str() ) != 0;
				break;
			case SFO_CONTAINS:
				result = idStr::FindText( builder.c_str(), filter.value.c_str(), false ) != idStr::INVALID_POSITION;
				break;
			case SFO_NOT_CONTAINS:
				result = idStr::FindText( builder.c_str(), filter.value.c_str(), false ) == idStr::INVALID_POSITION;
				break;
		}

		if( filter.state == SFS_SHOWONLY && !result ) {
 			visible = false;
			break;
		}
		if( result && filter.state != SFS_HIDE && filter.resultBin == SFR_OR ) {
			orFilters |= result;
			orSet = true;
		} else if( result && filter.state == SFS_HIDE ) {
			visible = false;
			break;
		}
	}

	if( orSet ) {
		visible &= orFilters;
	}

	return !visible;
}

/*
============
sdNetManager::Script_SaveFilters
============
*/
void sdNetManager::Script_SaveFilters( sdUIFunctionStack& stack ) {
	idStr prefix;
	stack.Pop( prefix );
	if( networkService->GetActiveUser() == NULL ) {
		assert( networkService->GetActiveUser() != NULL );
		gameLocal.Warning( "Script_SaveUser: no active user" );
		return;
	}

	idDict& dict = networkService->GetActiveUser()->GetProfile().GetProperties();

	idStr temp;
	for( int i = 0; i < numericFilters.Num(); i++ ) {
		const serverNumericFilter_t& filter = numericFilters[ i ];
		temp = va( "%i %i %i %i %f", filter.type, filter.op, filter.state, filter.resultBin, filter.value );
		dict.Set( va( "filter_%s_numeric_%i", prefix.c_str(), i ), temp.c_str() );
	}

	for( int i = 0; i < stringFilters.Num(); i++ ) {
		const serverStringFilter_t& filter = stringFilters[ i ];
		// limit user input to a sensible length
		sdStringBuilder_Heap buffer;
		buffer.Append( filter.value.c_str(), 64 );

		temp = va( "%s %i %i %i %s", filter.cvar.c_str(), filter.op, filter.state, filter.resultBin, buffer.c_str() );
		dict.Set( va( "filter_%s_string_%i", prefix.c_str(), i ), temp.c_str() );
	}
}

/*
============
sdNetManager::Script_LoadFilters
============
*/
void sdNetManager::Script_LoadFilters( sdUIFunctionStack& stack ) {
	idStr prefix;
	stack.Pop( prefix );

	if( networkService->GetActiveUser() == NULL ) {
		assert( networkService->GetActiveUser() != NULL );
		gameLocal.Warning( "Script_SaveUser: no active user" );
		return;
	}

	numericFilters.Clear();
	stringFilters.Clear();

	idDict& dict = networkService->GetActiveUser()->GetProfile().GetProperties();

	idToken token;

	int i = 0;

	const idKeyValue* kv = dict.MatchPrefix( va( "filter_%s_numeric_%i", prefix.c_str(), i ) );
	while( kv != NULL ) {
		serverNumericFilter_t& filter = *numericFilters.Alloc();

		idLexer src( kv->GetValue().c_str(), kv->GetValue().Length(), "Script_LoadFilters", LEXFL_NOERRORS | LEXFL_ALLOWMULTICHARLITERALS );

		bool success = true;
		if( src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilter_e >( token.GetIntValue(), SF_MIN, SF_MAX, filter.type );
		} else {
			success = false;
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterOp_e >( token.GetIntValue(), SFO_MIN, SFO_MAX, filter.op );
		} else {
			success = false;
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterState_e >( token.GetIntValue(), SFS_MIN, SFS_MAX, filter.state );
		} else {
			success = false;
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterResult_e >( token.GetIntValue(), SFR_MIN, SFR_MAX, filter.resultBin );
		} else {
			success = false;
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_FLOAT, &token ) ) {
			filter.value = token.GetFloatValue();
		} else {
			success = false;
		}

		if( !success ) {
			gameLocal.Warning( "LoadFilters: Invalid filter stream for '%s'", kv->GetKey().c_str() );
			numericFilters.RemoveIndexFast( numericFilters.Num() - 1 );
		}

		i++;
		kv = dict.MatchPrefix( va( "filter_%s_numeric_%i", prefix.c_str(), i ) );
	}

	i = 0;
	kv = dict.MatchPrefix( va( "filter_%s_string_%i", prefix.c_str(), i ) );
	while( kv != NULL ) {
		serverStringFilter_t& filter = *stringFilters.Alloc();

		idLexer src( kv->GetValue().c_str(), kv->GetValue().Length(), "Script_LoadFilters", LEXFL_NOERRORS );

		bool success = true;
		if( src.ExpectTokenType( TT_NAME, 0, &token ) ) {
			filter.cvar = token;
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterOp_e >( token.GetIntValue(), SFO_MIN, SFO_MAX, filter.op );
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterState_e >( token.GetIntValue(), SFS_MIN, SFS_MAX, filter.state );
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterResult_e >( token.GetIntValue(), SFR_MIN, SFR_MAX, filter.resultBin );
		}

		if( success ) {
			src.ParseRestOfLine( filter.value );
		}

		if( !success ) {
			gameLocal.Warning( "LoadFilters: Invalid filter stream for '%s'", kv->GetKey().c_str() );
			numericFilters.RemoveIndexFast( stringFilters.Num() - 1 );
		}

		i++;
		kv = dict.MatchPrefix( va( "filter_%s_string_%i", prefix.c_str(), i ) );
	}

}

/*
============
sdNetManager::Script_QueryNumericFilterValue
============
*/
void sdNetManager::Script_QueryNumericFilterValue( sdUIFunctionStack& stack ) {
	float fType;
	stack.Pop( fType );

	serverFilter_e type = SF_MIN;
	if( sdFloatToContinuousEnum< serverFilter_e >( fType, SF_MIN, SF_MAX, type ) ) {
		for( int i = 0; i < numericFilters.Num(); i++ ) {
			const serverNumericFilter_t& filter = numericFilters[ i ];
			if( filter.type == type ) {
				stack.Push( filter.value );
				return;
			}
		}
	}
	stack.Push( 0.0f );
}

/*
============
sdNetManager::Script_QueryNumericFilterState
============
*/
void sdNetManager::Script_QueryNumericFilterState( sdUIFunctionStack& stack ) {
	float fType;
	stack.Pop( fType );

	float value;
	stack.Pop( value );

	serverFilter_e type = SF_MIN;
	if( sdFloatToContinuousEnum< serverFilter_e >( fType, SF_MIN, SF_MAX, type ) ) {
		for( int i = 0; i < numericFilters.Num(); i++ ) {
			const serverNumericFilter_t& filter = numericFilters[ i ];
			if( filter.type == type ) {
				stack.Push( filter.state );
				return;
			}
		}
	}

	stack.Push( SFS_DONTCARE );
}

/*
============
sdNetManager::Script_QueryStringFilterState
============
*/
void sdNetManager::Script_QueryStringFilterState( sdUIFunctionStack& stack ) {
	idStr cvar;
	stack.Pop( cvar );

	idStr value;
	stack.Pop( value );

	bool testValue = !value.IsEmpty();

	for( int i = 0; i < stringFilters.Num(); i++ ) {
		const serverStringFilter_t& filter = stringFilters[ i ];
		if( filter.cvar.Icmp( cvar.c_str() ) == 0 && ( !testValue || filter.value.Icmp( value ) == 0 ) ) {
			stack.Push( filter.state );
			return;
		}
	}

	stack.Push( SFS_DONTCARE );
}

/*
============
sdNetManager::Script_QueryStringFilterValue
============
*/
void sdNetManager::Script_QueryStringFilterValue( sdUIFunctionStack& stack ) {
	idStr cvar;
	stack.Pop( cvar );

	for( int i = 0; i < stringFilters.Num(); i++ ) {
		const serverStringFilter_t& filter = stringFilters[ i ];
		if( filter.cvar.Icmp( cvar.c_str() ) == 0 ) {
			stack.Push( filter.value.c_str() );
			return;
		}
	}

	stack.Push( idStr( "" ) );
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_Team_ChooseContextAction
============
*/
void sdNetManager::Script_Team_ChooseContextAction( sdUIFunctionStack& stack ) {
	idStr memberName;
	stack.Pop( memberName );

	assert( activeMessage == NULL );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, memberName );
	if ( mate != NULL ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		if( ims.Num() ) {
			activeMessage = ims[ 0 ];
			stack.Push( sdNetProperties::TCA_READ_IM );
			return;
		} else {
			sdNetMessage* message = mate->GetMessageQueue().GetMessages();
			if ( message != NULL ) {
				switch( message->GetType() ) {
					case sdNetMessage::MT_MEMBERSHIP_PROMOTION_TO_ADMIN:
						stack.Push( sdNetProperties::TCA_NOTIFY_ADMIN );
						activeMessage = message;
						return;
					case sdNetMessage::MT_MEMBERSHIP_PROMOTION_TO_OWNER:
						stack.Push( sdNetProperties::TCA_NOTIFY_OWNER );
						activeMessage = message;
						return;
					case sdNetMessage::MT_SESSION_INVITE:
						stack.Push( sdNetProperties::TCA_SESSION_INVITE );
						activeMessage = message;
						return;
				}
			}
		}
		stack.Push( sdNetProperties::TCA_SEND_IM );
		return;
	}

	stack.Push( sdNetProperties::TCA_NONE );
}

/*
============
sdNetManager::Script_Team_IsMember
============
*/
void sdNetManager::Script_Team_IsMember( sdUIFunctionStack& stack ) {
	idStr memberName;
	stack.Pop( memberName );

	float result = 0.0f;

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	const sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, memberName.c_str() );

	stack.Push( mate == NULL ? 0.0f : 1.0f );
}

/*
============
sdNetManager::Script_Team_IsPendingMember
============
*/
void sdNetManager::Script_Team_IsPendingMember( sdUIFunctionStack& stack ) {
	idStr memberName;
	stack.Pop( memberName );

	float result = 0.0f;

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetPendingMemberList();
	const sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, memberName.c_str() );

	stack.Push( mate == NULL ? 0.0f : 1.0f );
}

/*
============
sdNetManager::Script_Team_GetIMText
============
*/
void sdNetManager::Script_Team_GetIMText( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();

	if ( sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, userName ) ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		if( ims.Num() ) {
			stack.Push( ims[ 0 ]->GetText() );
			return;
		}
	}
	stack.Push( L"" );
}
#endif /* !SD_DEMO_BUILD */

/*
============
sdNetManager::Script_JoinSession
============
*/
void sdNetManager::Script_JoinSession( sdUIFunctionStack& stack ) {
	assert( activeMessage != NULL );

	const netadr_t* net = reinterpret_cast< const netadr_t* >( activeMessage->GetData() );
	const char* netStr = sys->NetAdrToString( *net );
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "connect %s\n", netStr ) );
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_IsFriendOnServer
============
*/
void sdNetManager::Script_IsFriendOnServer( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );

	if ( mate == NULL || mate->GetState() != sdNetFriend::OS_ONLINE ) {
		stack.Push( false );
		return;
	}

	sdNetClientId id;
	mate->GetNetClientId( id );

	const idDict* profile = networkService->GetProfileProperties( id );
	const char* server = ( profile == NULL ) ? "" : profile->GetString( "currentServer", "0.0.0.0:0" );
	stack.Push( idStr::Cmp( server, "0.0.0.0:0" ) != 0 );
}

/*
============
sdNetManager::Script_FollowFriendToServer
============
*/
void sdNetManager::Script_FollowFriendToServer( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );
	if( mate == NULL ) {
		stack.Push( false );
		return;
	}

	sdNetClientId id;
	mate->GetNetClientId( id );

	const idDict* profile = networkService->GetProfileProperties( id );

	const char* server = ( profile == NULL ) ? "0.0.0.0:0" : profile->GetString( "currentServer", "0.0.0.0:0" );
	if( idStr::Cmp( server, "0.0.0.0:0" ) != 0 ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "connect %s\n", server ) );
		stack.Push( true );
	} else {
		stack.Push( false );
	}
}

/*
============
sdNetManager::Script_IsMemberOnServer
============
*/
void sdNetManager::Script_IsMemberOnServer( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* member = networkService->GetTeamManager().FindMember( members, friendName.c_str() );
	if( member == NULL || member->GetState() != sdNetTeamMember::OS_ONLINE ) {
		stack.Push( false );
		return;
	}

	sdNetClientId id;
	member->GetNetClientId( id );

	const idDict* profile = networkService->GetProfileProperties( id );
	const char* server = ( profile == NULL ) ? "" : profile->GetString( "currentServer", "0.0.0.0:0" );
	stack.Push( idStr::Cmp( server, "0.0.0.0:0" ) != 0 );
}

/*
============
sdNetManager::Script_FollowMemberToServer
============
*/
void sdNetManager::Script_FollowMemberToServer( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* member = networkService->GetTeamManager().FindMember( members, friendName.c_str() );

	if( member == NULL ) {
		stack.Push( false );
		return;
	}
	sdNetClientId id;
	member->GetNetClientId( id );

	const idDict* profile = networkService->GetProfileProperties( id );

	const char* server = ( profile == NULL ) ? "0.0.0.0:0" : profile->GetString( "currentServer", "0.0.0.0:0" );
	if( idStr::Cmp( server, "0.0.0.0:0" ) != 0 ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "connect %s\n", server ) );
		stack.Push( true );
	} else {
		stack.Push( false );
	}
}
#endif /* !SD_DEMO_BUILD */

/*
============
sdNetManager::Script_IsSelfOnServer
============
*/
void sdNetManager::Script_IsSelfOnServer( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	sdNetUser* activeUser = networkService->GetActiveUser();

	if( activeUser == NULL ) {
		stack.Push( false );
		return;
	}

	const char* server = activeUser->GetProfile().GetProperties().GetString( "currentServer", "0.0.0.0:0" );
	stack.Push( idStr::Cmp( server, "0.0.0.0:0" ) != 0 );
}

/*
============
sdNetManager::Script_GetMotdString
============
*/
void sdNetManager::Script_GetMotdString( sdUIFunctionStack& stack ) {
	const sdNetService::motdList_t& motd = networkService->GetMotD();

	sdWStringBuilder_Heap builder;
	for( int i = 0; i < motd.Num(); i++ ) {
		builder += motd[ i ].text.c_str();
		builder += L"            ";
	}
	stack.Push( builder.c_str() );
}

/*
============
sdNetManager::Script_SetDefaultUser
============
*/
void sdNetManager::Script_SetDefaultUser( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );

	int value;
	stack.Pop( value );

	bool set = false;
	for ( int i = 0; i < networkService->NumUsers(); i++ ) {
		sdNetUser* user = networkService->GetUser( i );

		if ( !idStr::Cmp( username, user->GetRawUsername() ) ) {
			set = true;
			user->GetProfile().GetProperties().SetBool( "default", value != 0 );
			user->Save( sdNetUser::SI_PROFILE );
			break;
		}
	}

	// only clear the previous default user if we successfully set the new one
	if( set ) {
		for ( int i = 0; i < networkService->NumUsers(); i++ ) {
			sdNetUser* user = networkService->GetUser( i );

			if ( !idStr::Cmp( username, user->GetRawUsername() ) ) {
				continue;
			}
			user->GetProfile().GetProperties().SetBool( "default", 0 );
			user->Save( sdNetUser::SI_PROFILE );
		}
	}
}


/*
============
sdNetManager::Script_RefreshCurrentServers
============
*/
void sdNetManager::Script_RefreshCurrentServers( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	findServerSource_e source;
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Error( "RefreshCurrentServers: source '%i' out of range", iSource );
		stack.Push( false );
		return;
	}

	StopFindingServers( source );

	sdNetTask* task = NULL;
	idList< sdNetSession* >* netSessions = NULL;
	sdHotServerList* netHotServers;

	GetSessionsForServerSource( source, netSessions, task, netHotServers );

	if( netSessions == NULL ) {
		assert( 0 );
		stack.Push( false );
		return;
	}
	if( task != NULL ) {
		gameLocal.Printf( "SDNet::RefreshCurrentServers : Already scanning for servers\n" );
		stack.Push( false );
		return;
	}

	lastServerUpdateIndex = 0;

	task = networkService->GetSessionManager().RefreshSessions( *netSessions );
	if ( task == NULL ) {
		gameLocal.Printf( "SDNet::RefreshCurrentServers : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( false );
		return;
	}

	stack.Push( true );
}


/*
============
sdNetManager::StopFindingServers
============
*/
void sdNetManager::StopFindingServers( findServerSource_e source ) {
	switch( source ) {
		case FS_INTERNET:
			if( findServersTask != NULL ) {
				findServersTask->Cancel( true );
				networkService->FreeTask( findServersTask );
				findServersTask = NULL;
				hotServers.Locate( *this );
			}
			break;
#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
		case FS_INTERNET_REPEATER:
			if( findRepeatersTask != NULL ) {
				findRepeatersTask->Cancel( true );
				networkService->FreeTask( findRepeatersTask );
				findRepeatersTask = NULL;
			}
			break;
#endif /* !SD_DEMO_BUILD && !SD_DEMO_BUILD_CONSTRUCTION */
		case FS_LAN:
			if( findLANServersTask != NULL ) {
				findLANServersTask->Cancel( true );
				networkService->FreeTask( findLANServersTask );
				findLANServersTask = NULL;
				hotServersLAN.Locate( *this );
			}
			break;
#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
		case FS_LAN_REPEATER:
			if( findLANRepeatersTask != NULL ) {
				findLANRepeatersTask->Cancel( true );
				networkService->FreeTask( findLANRepeatersTask );
				findLANRepeatersTask = NULL;
			}
			break;
#endif /* !SD_DEMO_BUILD && !SD_DEMO_BUILD_CONSTRUCTION */
		case FS_HISTORY:
			if( findHistoryServersTask != NULL ) {
				findHistoryServersTask->Cancel( true );
				networkService->FreeTask( findHistoryServersTask );
				findHistoryServersTask = NULL;
				hotServersHistory.Locate( *this );
			}
			break;
		case FS_FAVORITES:
			if( findFavoriteServersTask != NULL ) {
				findFavoriteServersTask->Cancel( true );
				networkService->FreeTask( findFavoriteServersTask );
				findFavoriteServersTask = NULL;
				hotServersFavorites.Locate( *this );
			}
			break;
	}
	lastServerUpdateIndex = 0;
}

/*
============
sdNetManager::Script_StopFindingServers
============
*/
void sdNetManager::Script_StopFindingServers( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	findServerSource_e source = static_cast< findServerSource_e >( iSource );
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Error( "StopFindingServers: source '%i' out of range", iSource );
		return;
	}
	StopFindingServers( source );
}


/*
============
sdNetManager::Script_ToggleServerFavoriteState
============
*/
void sdNetManager::Script_ToggleServerFavoriteState( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr sessionName;
	stack.Pop( sessionName );

	sdNetUser* activeUser = networkService->GetActiveUser();

	idStr key = va( "favorite_%s", sessionName.c_str() );
	bool set = activeUser->GetProfile().GetProperties().GetBool( key.c_str(), "0" );
	set = !set;
	if( !set ) {
		activeUser->GetProfile().GetProperties().Delete( key.c_str() );
		common->DPrintf( "Removed '%s' as a favorite\n", sessionName.c_str() ) ;
	} else {
		common->DPrintf( "Added '%s' as a favorite\n", sessionName.c_str() ) ;
		activeUser->GetProfile().GetProperties().SetBool( key.c_str(), true );
	}
	activeUser->Save( sdNetUser::SI_PROFILE );
}

/*
============
sdNetManager::UpdateServer
============
*/
void sdNetManager::UpdateServer( sdUIList& list, const char* sessionName, findServerSource_e source ) {
	assert( networkService->GetActiveUser() != NULL );

	sdNetTask* task;
	idList< sdNetSession* >* netSessions;
	sdHotServerList* netHotServers;

	GetSessionsForServerSource( source, netSessions, task, netHotServers );

	if( findServersTask != NULL || findLANServersTask != NULL || 
		findHistoryServersTask != NULL || findFavoriteServersTask != NULL ||
		findRepeatersTask != NULL || findLANRepeatersTask != NULL ) {
		return;
	}

	if( netSessions != NULL ) {
		sessionHash_t::Iterator iter = hashedSessions.Find( sessionName );
		if( iter != hashedSessions.End() ) {
			if( task != NULL ) {
				task->AcquireLock();
			}

			sdNetSession* netSession = (*netSessions)[ iter->second.sessionListIndex ];

			UpdateSession( list, *netSession, iter->second.uiListIndex );

			if( task != NULL ) {
				task->ReleaseLock();
			}
		} else {
			assert( false );
		}
	} else {
		assert( false );
	}
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_Team_NumAvailableIMs
============
*/
void sdNetManager::Script_Team_NumAvailableIMs( sdUIFunctionStack& stack ) {

	idStr friendName;
	stack.Pop( friendName );
	if ( friendName.IsEmpty() ) {
		stack.Push( 0 );
		return;
	}

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& friends = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( friends, friendName );
	if ( mate != NULL ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		stack.Push( ims.Num() );
		return;
	}
	stack.Push( 0 );
}

/*
============
sdNetManager::Script_Team_GetMemberStatus
============
*/
void sdNetManager::Script_Team_GetMemberStatus( sdUIFunctionStack& stack ) {
	idStr memberName;
	stack.Pop( memberName );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, memberName );
	if ( mate != NULL ) {
		stack.Push( mate->GetMemberStatus() );
		return;
	}
	stack.Push( sdNetTeamMember::MS_MEMBER );
}

/*
============
sdNetManager::ShowRanked
============
*/
bool sdNetManager::ShowRanked() const {
#if !defined( SD_DEMO_BUILD_CONSTRUCTION )
	for( int i = 0; i < numericFilters.Num(); i++ ) {
		const serverNumericFilter_t& filter = numericFilters[ i ];
		if ( filter.type == SF_RANKED && filter.state == SFS_SHOWONLY ) {
			return true;
			break;
		}
	}
#endif /* !SD_DEMO_BUILD_CONSTRUCTION */
	return false;
}

#endif /* !SD_DEMO_BUILD */

/*
============
sdNetManager::Script_GetPlayerCount
============
*/
void sdNetManager::Script_GetPlayerCount( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	findServerSource_e source;
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Error( "GetPlayerCount: source '%i' out of range", iSource );
		stack.Push( false );
		return;
	}
	idList< sdNetSession* >* sessions;
	sdNetTask* task = NULL;
	sdHotServerList* netHotServers;

	GetSessionsForServerSource( source, sessions, task, netHotServers );

	int count = 0;
	if( sessions != NULL ) {
		if( task != NULL ) {
			task->AcquireLock();
		}

		for( int i = 0; i < sessions->Num(); i++ ) {
			sdNetSession* netSession = (*sessions)[ i ];
			count += netSession->GetNumClients() - netSession->GetNumBotClients();
		}

		if( task != NULL ) {
			task->ReleaseLock();
		}
	}
	stack.Push( count );
}

/*
============
sdNetManager::Script_GetServerStatusString
============
*/
void sdNetManager::Script_GetServerStatusString( sdUIFunctionStack& stack ) {
	float fSource;
	stack.Pop( fSource );

	float sessionIndexFloat;
	stack.Pop( sessionIndexFloat );

	sdNetSession* netSession = GetSession( fSource, idMath::FtoiFast( sessionIndexFloat ) );

	idWStr status;

	if( netSession != NULL ) {
		const idDict& serverInfo = netSession->GetServerInfo();

		const sdGameRules* rulesInstance = gameLocal.GetRulesInstance( serverInfo.GetString( "si_rules" ) );

		if( rulesInstance != NULL ) {
			rulesInstance->GetBrowserStatusString( status, *netSession );
		}
	}

	stack.Push( status.c_str() );
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_GetFriendServerIP
============
*/
void sdNetManager::Script_GetFriendServerIP( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );
	if( mate == NULL ) {
		stack.Push( "" );
		return;
	}

	sdNetClientId id;
	mate->GetNetClientId( id );
	const idDict* profile = networkService->GetProfileProperties( id );

	const char* server = ( profile == NULL ) ? "" : profile->GetString( "currentServer", "0.0.0.0:0" );
	if( idStr::Cmp( server, "0.0.0.0:0" ) != 0 ) {
		stack.Push( server );
		return;
	}
	stack.Push( "" );
}

/*
============
sdNetManager::Script_GetMemberServerIP
============
*/
void sdNetManager::Script_GetMemberServerIP( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, friendName.c_str() );
	if( mate == NULL ) {
		stack.Push( "" );
		return;
	}
	sdNetClientId id;
	mate->GetNetClientId( id );
	const idDict* profile = networkService->GetProfileProperties( id );

	const char* server = ( profile == NULL ) ? "" : profile->GetString( "currentServer", "0.0.0.0:0" );
	if( idStr::Cmp( server, "0.0.0.0:0" ) != 0 ) {
		stack.Push( server );
		return;
	}
	stack.Push( "" );
}


/*
============
sdNetManager::AnyFriendsOnServer
============
*/
bool sdNetManager::AnyFriendsOnServer( const sdNetSession& netSession ) const {
	sdStringBuilder_Heap address( netSession.GetHostAddressString() );

	int hash = serversWithFriendsHash.GenerateKey( address.c_str(), false );
	int i;
	for ( i = serversWithFriendsHash.GetFirst( hash ); i != idHashIndexUShort::NULL_INDEX; i = serversWithFriendsHash.GetNext( i ) ) {
		if( idStr::Cmp( address.c_str(), serversWithFriends[ i ] ) == 0 ) {
			return true;
		}
	}
	return false;
}

/*
============
sdNetManager::AddFriendServerToHash
============
*/
void sdNetManager::AddFriendServerToHash( const char* server ) {
	if( idStr::Cmp( server, "0.0.0.0:0" ) != 0 ) {
		int j;
		for ( j = 0; j < serversWithFriends.Num(); j++ ) {
			if( idStr::Cmp( server, serversWithFriends[ j ] ) == 0 ) {
				break;
			}
		}
		if( j >= serversWithFriends.Num() ) {
			serversWithFriendsHash.Add( serversWithFriendsHash.GenerateKey( server, false ), serversWithFriends.Append( server ) );
		}
	}
}

/*
============
sdNetManager::CacheServersWithFriends
============
*/
void sdNetManager::CacheServersWithFriends() {
	assert( networkService->GetActiveUser() != NULL );

	serversWithFriendsHash.Clear();
	serversWithFriends.SetNum( 0, false );

	{
		sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
		const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
		for( int i = 0; i < friends.Num(); i++ ) {
			const sdNetFriend* mate = friends[ i ];
			if( mate->GetState() != sdNetFriend::OS_ONLINE ) {
				continue;
			}
			sdNetClientId id;
			mate->GetNetClientId( id );
			const idDict* profile = networkService->GetProfileProperties( id );

			const char* server = ( profile == NULL ) ? "0.0.0.0:0" : profile->GetString( "currentServer", "0.0.0.0:0" );
			AddFriendServerToHash( server );
		}
	}
	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	for( int i = 0; i < members.Num(); i++ ) {
		const sdNetTeamMember* member = members[ i ];
		if( member->GetState() != sdNetFriend::OS_ONLINE ) {
			continue;
		}

		sdNetClientId id;
		member->GetNetClientId( id );
		const idDict* profile = networkService->GetProfileProperties( id );

		const char* server = ( profile == NULL ) ? "0.0.0.0:0" : profile->GetString( "currentServer", "0.0.0.0:0" );
		AddFriendServerToHash( server );
	}
}

/*
============
sdNetManager::Script_GetMessageTimeStamp
============
*/
void sdNetManager::Script_GetMessageTimeStamp( sdUIFunctionStack& stack ) {
	if( activeMessage == NULL ) {
		stack.Push( L"" );
		return;
	}

	const sysTime_t& time = activeMessage->GetTimeStamp();

	sdNetProperties::FormatTimeStamp( time, tempWStr );
	stack.Push( tempWStr.c_str() );
}

/*
============
sdNetManager::Script_ClearActiveMessage
============
*/
void sdNetManager::Script_ClearActiveMessage( sdUIFunctionStack& stack ) {
	assert( activeMessage != NULL );
	activeMessage = NULL;
}

/*
============
sdNetManager::Script_GetServerInviteIP
============
*/
void sdNetManager::Script_GetServerInviteIP( sdUIFunctionStack& stack ) {
	assert( activeMessage != NULL );
	netadr_t net = *reinterpret_cast< const netadr_t* >( activeMessage->GetData() );
	const char* netStr = sys->NetAdrToString( net );

	stack.Push( netStr );
}
#endif /* !SD_DEMO_BUILD */


/*
============
sdNetManager::Script_AddUnfilteredSession
============
*/
void sdNetManager::Script_AddUnfilteredSession( sdUIFunctionStack& stack ) {
	idStr address;
	stack.Pop( address );

	netadr_t addr;
	sys->StringToNetAdr( address.c_str(), &addr, false );
	if( addr.type != NA_BAD ) {
		unfilteredSessions.AddUnique( addr );
	}
}


/*
============
sdNetManager::Script_ClearUnfilteredSessions
============
*/
void sdNetManager::Script_ClearUnfilteredSessions( sdUIFunctionStack& stack ) {
	unfilteredSessions.SetNum( 0, false );
}

/*
============
sdNetManager::Script_FormatSessionInfo
============
*/
void sdNetManager::Script_FormatSessionInfo( sdUIFunctionStack& stack ) {
	idStr addr;
	stack.Pop( addr );

	for( int i = 0; i < sessions.Num(); i++ ) {
		const sdNetSession& netSession = *sessions[ i ];
		if( addr.Cmp( netSession.GetHostAddressString() ) == 0 ) {
			const idDict& serverInfo = netSession.GetServerInfo();

			sdWStringBuilder_Heap builder;

			builder += va( L"%hs\n", serverInfo.GetString( "si_name" ) );

			builder += va( L"%ls: ", common->LocalizeText( "guis/mainmenu/mapname" ).c_str() );

			// Map name
			if ( const idDict* mapInfo = GetMapInfo( netSession ) ) {
				idStr prettyName;
				prettyName = mapInfo != NULL ? mapInfo->GetString( "pretty_name", serverInfo.GetString( "si_map" ) ) : "";
				prettyName.StripFileExtension();
				builder += va( L"%hs\n", prettyName.c_str() );
			} else {
				builder += va( L"%hs\n", serverInfo.GetString( "si_map" ) );
			}

			builder += va( L"%ls: ", common->LocalizeText( "guis/mainmenu/gametype" ).c_str() );

			const char* fsGame = serverInfo.GetString( "fs_game" );

			if( !*fsGame ) {
				const char* siRules = serverInfo.GetString( "si_rules" );
				idWStr gameType;
				GetGameType( siRules, gameType );
				builder += gameType.c_str();
			} else {
				builder += va( L"%hs", fsGame );
			}
			
			builder += L"\n";

			builder += va( L"%ls: ", common->LocalizeText( "guis/mainmenu/time" ).c_str() );

			if( netSession.GetGameState() & sdGameRules::PGS_WARMUP ) {
				builder += common->LocalizeText( "guis/mainmenu/server/warmup" ).c_str();
			} else if( netSession.GetGameState() & sdGameRules::PGS_REVIEWING ) {
				builder += common->LocalizeText( "guis/mainmenu/server/reviewing" ).c_str();
			} else if( netSession.GetGameState() & sdGameRules::PGS_LOADING ) {
				builder += common->LocalizeText( "guis/mainmenu/server/loading" ).c_str();
			} else {
				idWStr::hmsFormat_t format;
				format.showZeroSeconds = false;
				format.showZeroMinutes = true;
				builder += idWStr::MS2HMS( netSession.GetSessionTime(), format );
			}
			builder += L"\n";

			builder += va( L"%ls: ", common->LocalizeText( "guis/mainmenu/players" ).c_str() );

			// Client Count
			int numBots = netSession.GetNumBotClients();
			if( numBots == 0 ) {
				int num = netSession.IsRepeater() ? netSession.GetNumRepeaterClients() : netSession.GetNumClients();
				int max = netSession.IsRepeater() ? netSession.GetMaxRepeaterClients() : netSession.GetServerInfo().GetInt( "si_maxPlayers" );

				builder += va( L"%d/%d\n", num, max );
			} else {
				builder += va( L"%d/%d (%d)\n", netSession.GetNumClients(), serverInfo.GetInt( "si_maxPlayers" ), numBots );
			}

			// Ping
			builder += va( L"%ls: %d", common->LocalizeText( "guis/game/scoreboard/ping" ).c_str(), netSession.GetPing() );

			stack.Push( builder.c_str() );
			return;
		}
	}
	idWStrList args( 1 );
	args.Append( va( L"%hs", addr.c_str() ) );
	stack.Push( common->LocalizeText( "guis/mainmenu/serveroffline", args ).c_str() );
	return;
}

/*
============
sdNetManager::Script_CancelActiveTask
============
*/
void sdNetManager::Script_CancelActiveTask( sdUIFunctionStack& stack ) {
	assert( activeTask.task != NULL );
	if( activeTask.task == NULL ) {
		return;
	}
	activeTask.Cancel();
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_UnloadMessageHistory
============
*/
void sdNetManager::Script_UnloadMessageHistory( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	idStr username;
	stack.Pop( username );

	bool success = false;

	sdNetProperties::messageHistorySource_e source;
	if( sdIntToContinuousEnum< sdNetProperties::messageHistorySource_e >( iSource, sdNetProperties::MHS_MIN, sdNetProperties::MHS_MAX, source ) ) {
		idStr rawUserName;

		sdNetMessageHistory* history = sdNetProperties::GetMessageHistory( source, username.c_str(), rawUserName );

		if( history != NULL ) {
			if( history->IsLoaded() ) {
				history->Store();
				history->Unload();
				success = true;
			}
		}
	}

	stack.Push( success );
}

/*
============
sdNetManager::Script_LoadMessageHistory
============
*/
void sdNetManager::Script_LoadMessageHistory( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	int iSource;
	stack.Pop( iSource );

	idStr username;
	stack.Pop( username );

	bool success = false;

	sdNetProperties::messageHistorySource_e source;
	if( sdIntToContinuousEnum< sdNetProperties::messageHistorySource_e >( iSource, sdNetProperties::MHS_MIN, sdNetProperties::MHS_MAX, source ) ) {
		idStr rawUserName;

		sdNetMessageHistory* history = sdNetProperties::GetMessageHistory( source, username.c_str(), rawUserName );

		if( history != NULL && !history->IsLoaded() ) {
			idStr path;
			sdNetProperties::GenerateMessageHistoryFileName( networkService->GetActiveUser()->GetRawUsername(), rawUserName.c_str(), path );
			history->Load( path.c_str() );
			success = true;
		}
	}

	stack.Push( success );
}

/*
============
sdNetManager::Script_AddToMessageHistory
============
*/
void sdNetManager::Script_AddToMessageHistory( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	idStr username;
	stack.Pop( username );

	idStr fromUser;
	stack.Pop( fromUser );

	idWStr message;
	stack.Pop( message );

	bool success = false;

	sdNetProperties::messageHistorySource_e source;
	if( sdIntToContinuousEnum< sdNetProperties::messageHistorySource_e >( iSource, sdNetProperties::MHS_MIN, sdNetProperties::MHS_MAX, source ) ) {
		idStr rawUserName;

		sdNetMessageHistory* history = sdNetProperties::GetMessageHistory( source, username.c_str(), rawUserName );

		if( history != NULL ) {
			assert( history->IsLoaded() );
			const sdNetUser* activeUser = networkService->GetActiveUser();

			tempWStr = va( L"%hs: %ls", fromUser.c_str(), message.c_str() );
			history->AddEntry( tempWStr.c_str() );
			history->Store();
		} else {
			gameLocal.Warning( "Script_AddToMessageHistory: Could not find '%s'", username.c_str() );
		}
	}
	stack.Push( success );
}

/*
============
sdNetManager::Script_GetUserNamesForKey
============
*/
void sdNetManager::Script_GetUserNamesForKey( sdUIFunctionStack& stack ) {
	idStr key;
	stack.Pop( key );

	if( activeTask.task != NULL ) {
		gameLocal.Printf( "GetUserNamesForKey: task pending\n" );
		stack.Push( false );
		return;
	}

	if ( !activeTask.Set( networkService->GetAccountsForLicense( retrievedAccountNames, key.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::GetUserNamesForKey : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( false );
		return;
	}

	properties.SetTaskActive( true );
	stack.Push( true );
}

/*
============
sdNetManager::CreateRetrievedUserNameList
============
*/
void sdNetManager::CreateRetrievedUserNameList( sdUIList* list ) {
	sdUIList::ClearItems( list );
	
	for( int i = 0; i < retrievedAccountNames.Num(); i++ ) {
		sdUIList::InsertItem( list, va( L"%hs", retrievedAccountNames[ i ].c_str() ), -1, 0 );
	}
}

#endif /* !SD_DEMO_BUILD */


/*
============
sdNetManager::Script_GetNumInterestedInServer
============
*/
void sdNetManager::Script_GetNumInterestedInServer( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	idStr address;
	stack.Pop( address );

	findServerSource_e source;
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Error( "GetNumInterestedInServer: source '%i' out of range", iSource );
		stack.Push( 0.0f );
		return;
	}

	sdNetTask* task;
	idList< sdNetSession* >* netSessions;
	sdHotServerList* netHotServers;
	GetSessionsForServerSource( source, netSessions, task, netHotServers );

	if( netSessions == NULL ) {
		stack.Push( 0.0f );
		return;
	}
	int interested = 0;

	sessionHash_t::Iterator iter = hashedSessions.Find( address );
	if( iter != hashedSessions.End() ) {
		interested = (*netSessions)[ iter->second.sessionListIndex ]->GetNumInterestedClients();		
	}

	stack.Push( interested );
}

/*
============
sdNetManager::Script_JoinBestServer
============
*/
void sdNetManager::Script_JoinBestServer( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	findServerSource_e source;
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Error( "JoinBestServer: source '%i' out of range", iSource );
		return;
	}

	sdNetTask* task;
	idList< sdNetSession* >* netSessions;
	sdHotServerList* netHotServers;
	GetSessionsForServerSource( source, netSessions, task, netHotServers );

	if( netHotServers->GetNumServers() == 0 ) {
		return;
	}
	sdNetSession* session = netHotServers->GetServer( 0 );
	if( session == NULL ) {
		return;
	}
	session->Join();
}

/*
============
sdNetManager::Script_GetNumHotServers
============
*/
void sdNetManager::Script_GetNumHotServers( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	findServerSource_e source;
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Error( "JoinBestServer: source '%i' out of range", iSource );
		stack.Push( 0.0f );
		return;
	}

	sdNetTask* task;
	idList< sdNetSession* >* netSessions;
	sdHotServerList* netHotServers;
	GetSessionsForServerSource( source, netSessions, task, netHotServers );

	stack.Push( netHotServers->GetNumServers() );
}
