// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SDNET_H__
#define __SDNET_H__

#include "SDNetErrorCode.h"

//===============================================================
//
//	sdNetService
//
//===============================================================

class sdNetUser;
class sdNetSessionManager;
class sdNetStatsManager;
class sdNetFriendsManager;
class sdNetTeamManager;
class sdNetTask;

struct sdNetEntityId {
	unsigned int	id[2];

					sdNetEntityId() { Invalidate(); }
	bool			IsValid() const { return id[0] != 0 || id[1] != 0; }
	void			Invalidate() { id[0] = id[1] = 0; }
	bool			operator ==( const sdNetEntityId& rhs ) const { return id[0] == rhs.id[0] && id[1] == rhs.id[1]; }
	bool			operator !=( const sdNetEntityId& rhs ) const { return id[0] != rhs.id[0] || id[1] != rhs.id[1]; }

	void			FromUInt64( uint64_t value ) { id[0] = ( value & 0xFFFFFFFF00000000 ) >> 32; id[1] = ( value & 0x00000000FFFFFFFF ); }
	void			ToUInt64( uint64_t& value ) const { value = ( ( (uint64_t)id[0] << 32 ) | id[1] ); }
};

struct sdNetClientId : public sdNetEntityId {};

class sdNetService {
public:
	enum serviceState_e {
		SS_DISABLED,
		SS_INITIALIZED,
		SS_CONNECTING,
		SS_ONLINE
	};

	enum disconnectReason_e {
		DR_NONE,
		DR_GRACEFUL,
		DR_CONNECTION_RESET,
		DR_DUPLICATE_AUTH,
		DR_ACCOUNT_DELETION,
	};

	enum dedicatedState_e {
		DS_OFFLINE,
		DS_CONNECTING,
		DS_ONLINE,
	};

	struct motdEntry_t {
		sysTime_t	timestamp;
		idStr		url;
		idWStr		text;
	};

	typedef idList< motdEntry_t > motdList_t;

	virtual							~sdNetService() {}

	virtual bool					Init() = 0;
	virtual void					Shutdown() = 0;

	virtual void					RunFrame() = 0;

	virtual serviceState_e			GetState() const = 0;

	virtual disconnectReason_e		GetDisconnectReason() const = 0;

	virtual dedicatedState_e		GetDedicatedServerState() const = 0;

	virtual const motdList_t&		GetMotD() const = 0;

	//
	// Key Code
	//
	virtual bool					CheckKey( const char* key, bool noChecksum = false ) const = 0;

	virtual const char*				GetStoredLicenseCode() const = 0;

	virtual bool					IsSteamActive() const = 0;

	//
	// User management
	//
	virtual sdNetErrorCode_e		CreateUser( sdNetUser** user, const char* username ) = 0;
	virtual void					DeleteUser( sdNetUser* user ) = 0;

	virtual int						NumUsers() const = 0;
	virtual sdNetUser*				GetUser( const int index ) = 0;

	virtual sdNetUser*				GetActiveUser() = 0;

	//
	// Session management - deferred to Session Manager
	//
	virtual sdNetSessionManager&	GetSessionManager() = 0;

#if !defined( SD_DEMO_BUILD )
	//
	// Stats management - deferred to Stats Manager
	//
	virtual sdNetStatsManager&		GetStatsManager() = 0;

	//
	// Friends management - deferred to Friends Manager
	//
	virtual sdNetFriendsManager&	GetFriendsManager() = 0;

	//
	// Friends management - deferred to Team Manager
	//
	virtual sdNetTeamManager&		GetTeamManager() = 0;
#endif /* !SD_DEMO_BUILD */

	//
	// Task management
	//
	virtual void					FreeTask( sdNetTask* task ) = 0;

	//
	// Online Services
	//

	virtual sdNetErrorCode_e		GetLastError() const = 0;

	// Start online service and connect to auth system
	virtual sdNetTask*				Connect() = 0;

	// Authorize a dedicated server
	virtual sdNetTask*				SignInDedicated() = 0;

	// De-authorize a dedicated server
	virtual sdNetTask*				SignOutDedicated() = 0;

#if !defined( SD_DEMO_BUILD )
	// Get a list of account names for a license code
	virtual sdNetTask*				GetAccountsForLicense( idStrList& accountNames, const char* licenseCode ) = 0;

	// Get a user's profile
	virtual const idDict*			GetProfileProperties( sdNetClientId userID ) const = 0;
#endif /* !SD_DEMO_BUILD */
};

extern sdNetService* networkService;

#endif /* !__SDNET_H__ */
