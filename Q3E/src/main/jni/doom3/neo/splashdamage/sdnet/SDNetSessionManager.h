// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETSESSIONMANAGER_H__ )
#define __SDNETSESSIONMANAGER_H__

//===============================================================
//
//	sdNetSessionManager
//
//===============================================================

class sdNetTask;
class sdNetSession;

class sdNetSessionManager {
public:
	enum sessionSource_e {
		SS_LAN,
		SS_LAN_REPEATER,
		SS_INTERNET_ALL,
		SS_INTERNET_RANKED,
		SS_INTERNET_REPEATER,
	};

	virtual					~sdNetSessionManager() {}

	// Obtain network address of the current active game
	virtual const netadr_t&	GetCurrentSessionAddress() const = 0;

	// Allocate a session
	virtual sdNetSession*	AllocSession( const netadr_t* sessionAddress = NULL ) = 0;

	// Free a session
	virtual void			FreeSession( sdNetSession* session ) = 0;

	//
	// Online functionality
	//

	// Register a session with the master
	virtual sdNetTask*		CreateSession( sdNetSession& session ) = 0;

	// Update session details
	virtual sdNetTask*		UpdateSession( sdNetSession& session ) = 0;

	// Remove session from the master
	virtual sdNetTask*		DeleteSession( sdNetSession& session ) = 0;

	// Retrieve sessions from the master
	virtual sdNetTask*		FindSessions( idList< sdNetSession* >& sessions, sessionSource_e source = SS_INTERNET_ALL ) = 0;

	// Re-queries the already obtained session list to get the most up to date details
	virtual sdNetTask*		RefreshSessions( idList< sdNetSession* >& sessions ) = 0;

	// Re-queries the specified session to get the most up to date details
	virtual sdNetTask*		RefreshSession( sdNetSession& session ) = 0;
};

#endif /* !__SDNETSESSIONMANAGER_H__ */
