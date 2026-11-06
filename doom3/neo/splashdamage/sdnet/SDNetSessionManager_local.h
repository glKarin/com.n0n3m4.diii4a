// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETSESSIONMANAGER_LOCAL_H__ )
#define __SDNETSESSIONMANAGER_LOCAL_H__

#include "SDNetSessionManager.h"

//===============================================================
//
//	sdNetSessionManager
//
//===============================================================

class sdNetSessionManager_Local : public sdNetSessionManager {
public:
	sdNetSessionManager_Local();
	virtual					~sdNetSessionManager_Local();

	// Obtain network address of the current active game
	virtual const netadr_t&	GetCurrentSessionAddress() const;

	// Allocate a session
	virtual sdNetSession*	AllocSession( const netadr_t* sessionAddress = NULL );

	// Free a session
	virtual void			FreeSession( sdNetSession* session );

	//
	// Online functionality
	//

	// Register a session with the master
	virtual sdNetTask*		CreateSession( sdNetSession& session );

	// Update session details
	virtual sdNetTask*		UpdateSession( sdNetSession& session );

	// Remove session from the master
	virtual sdNetTask*		DeleteSession( sdNetSession& session );

	// Retrieve sessions from the master
	virtual sdNetTask*		FindSessions( idList< sdNetSession* >& sessions, sessionSource_e source = SS_INTERNET_ALL );

	// Re-queries the already obtained session list to get the most up to date details
	virtual sdNetTask*		RefreshSessions( idList< sdNetSession* >& sessions );

	// Re-queries the specified session to get the most up to date details
	virtual sdNetTask*		RefreshSession( sdNetSession& session );

private:
	netadr_t address;
};

#endif /* !__SDNETSESSIONMANAGER_LOCAL_H__ */
