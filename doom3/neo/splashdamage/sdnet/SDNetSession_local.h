// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETSESSION_LOCAL_H__ )
#define __SDNETSESSION_LOCAL_H__

#include "SDNetSession.h"

//===============================================================
//
//	sdNetSession
//
//===============================================================

class sdNetSession_Local : public sdNetSession {
public:
	sdNetSession_Local();
	virtual					~sdNetSession_Local();

	virtual sessionState_e	GetState() const;

	//
	// Retrieve session details
	//

	// Reverse looked up hostname
	virtual const char*			GetHostName() const;

	// Get the host address as "ip:port"
	virtual const char*			GetHostAddressString() const;

	virtual const netadr_t&		GetAddress() const;

	// Get server info
	virtual const idDict&		GetServerInfo() const;
	virtual idDict&				GetServerInfo();

	// Get number of clients
	virtual const int			GetNumClients() const;

	// Get number of bot clients
	virtual const int			GetNumBotClients() const;
	
	// Retrieve client data
	virtual const sessionClientInfo_t& GetClientInfo( int clientNum ) const;

	// Get latency
	virtual const int			GetPing() const;

	// Get ranked status
	virtual bool				IsRanked() const;

	// Get session game status
	virtual byte				GetGameState() const;

	// Get how much time is left (in ms)
	virtual int					GetSessionTime() const;

	// Connect to this session
	virtual bool				Join();

	// Get the unique identifier for this session
	virtual void				GetId( sessionId_t& sessionId ) const;

	// When a client connects to this session
	virtual void				ServerClientConnect( const int clientNum );

	// When a client disconnects from this session
	virtual void				ServerClientDisconnect( const int clientNum );

	// Get number of clients considering joining
	virtual int					GetNumInterestedClients( void ) const;

	// Get repeater status
	virtual bool				IsRepeater() const;

	// Get number of people on the repeater
	virtual int					GetNumRepeaterClients( void ) const;

	// Get max number of people on the repeater
	virtual int					GetMaxRepeaterClients( void ) const;

private:
	sessionState_e sessionState;
	idDict serverInfo;
	sessionId_t sessionId;
	netadr_t address;
	sessionClientInfo_t clientInfos[32];
};

#endif /* !__SDNETSESSION_LOCAL_H__ */
