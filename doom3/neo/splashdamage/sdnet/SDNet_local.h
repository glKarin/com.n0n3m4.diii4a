// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SDNET_LOCAL_H__
#define __SDNET_LOCAL_H__

#include "SDNet.h"
#include "SDNetUser_local.h"
#include "SDNetSessionManager_local.h"
#if !defined( SD_DEMO_BUILD )
#include "SDNetStatsManager_local.h"
#include "SDNetFriendsManager_local.h"
#include "SDNetTeamManager_local.h"
#endif /* !SD_DEMO_BUILD */

//===============================================================
//
//	sdNetService
//
//===============================================================

class sdNetService_Local : public sdNetService {
public:
	sdNetService_Local();
	virtual							~sdNetService_Local();

	virtual bool					Init();
	virtual void					Shutdown();

	virtual void					RunFrame();

	virtual serviceState_e			GetState() const;

	virtual disconnectReason_e		GetDisconnectReason() const;

	virtual dedicatedState_e		GetDedicatedServerState() const;

	virtual const motdList_t&		GetMotD() const;

	//
	// Key Code
	//
	virtual bool					CheckKey( const char* key, bool noChecksum = false ) const;

	virtual const char*				GetStoredLicenseCode() const;

	virtual bool					IsSteamActive() const;

	//
	// User management
	//
	virtual sdNetErrorCode_e		CreateUser( sdNetUser** user, const char* username );
	virtual void					DeleteUser( sdNetUser* user );

	virtual int						NumUsers() const;
	virtual sdNetUser*				GetUser( const int index );

	virtual sdNetUser*				GetActiveUser();

	//
	// Session management - deferred to Session Manager
	//
	virtual sdNetSessionManager&	GetSessionManager();

#if !defined( SD_DEMO_BUILD )
	//
	// Stats management - deferred to Stats Manager
	//
	virtual sdNetStatsManager&		GetStatsManager();

	//
	// Friends management - deferred to Friends Manager
	//
	virtual sdNetFriendsManager&	GetFriendsManager();

	//
	// Friends management - deferred to Team Manager
	//
	virtual sdNetTeamManager&		GetTeamManager();
#endif /* !SD_DEMO_BUILD */

	//
	// Task management
	//
	virtual void					FreeTask( sdNetTask* task );

	//
	// Online Services
	//

	virtual sdNetErrorCode_e		GetLastError() const;

	// Start online service and connect to auth system
	virtual sdNetTask*				Connect();

	// Authorize a dedicated server
	virtual sdNetTask*				SignInDedicated();

	// De-authorize a dedicated server
	virtual sdNetTask*				SignOutDedicated();

#if !defined( SD_DEMO_BUILD )
	// Get a list of account names for a license code
	virtual sdNetTask*				GetAccountsForLicense( idStrList& accountNames, const char* licenseCode );

	// Get a user's profile
	virtual const idDict*			GetProfileProperties( sdNetClientId userID ) const;
#endif /* !SD_DEMO_BUILD */

	void							AddTask(sdNetTask *task);

	void							SetActiveUser(sdNetUser_Local *user);
	void							SaveUserModified(void);

private:
	void							LoadOfflineUsers(void);
	
private:
	bool isInitialized;
	serviceState_e serviceState;
	disconnectReason_e disconnectReason;
	dedicatedState_e dedicatedState;
	motdList_t motdList;
	idList<sdNetUser_Local *> userList;
	sdNetSessionManager_Local sessionManager;
#if !defined( SD_DEMO_BUILD )
	sdNetStatsManager_Local statsManager;
	sdNetFriendsManager_Local friendsManager;
	sdNetTeamManager_Local teamManager;
#endif /* !SD_DEMO_BUILD */
	sdNetErrorCode_e lastError;
	sdNetUser_Local *activeUser;

	static idList<sdNetTask *> taskPools;
};

extern sdNetService_Local networkServiceLocal;

#endif /* !__SDNET_LOCAL_H__ */
