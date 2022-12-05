// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __NETWORKSYSTEM_H__
#define __NETWORKSYSTEM_H__


/*
===============================================================================

  Network System.

===============================================================================
*/

typedef struct {
	idStr		nickname;
	idStr		clan;
	short		ping;
	int			rate;
} scannedClient_t;

typedef struct {
	netadr_t	adr;
	idDict		serverInfo;
	int			ping;

	int			clients;

	int			OSMask;

//RAVEN BEGIN
// shouchard:  added favorite flag
	bool		favorite;	// true if this has been marked by a user as a favorite
	bool		dedicated;
// shouchard:  added performance filtered flag
	bool		performanceFiltered;	// true if the client machine is too wimpy to have good performance
//RAVEN END
} scannedServer_t;

typedef enum {
	SC_NONE = -1,
	SC_FAVORITE,
	SC_LOCKED,
	SC_DEDICATED,
	SC_PB,
	SC_NAME,
	SC_PING,
	SC_REPEATER,
	SC_PLAYERS,
	SC_GAMETYPE,
	SC_MAP,
	SC_ALL,
	NUM_SC
} sortColumn_t;

typedef struct {
	sortColumn_t			column;
	idList<int>::cmp_t*		compareFn;
	idList<int>::filter_t*	filterFn;
	const char*				description;
} sortInfo_t;

class idNetworkSystem {
public:
	virtual					~idNetworkSystem( void ) {}

	virtual void			Shutdown( void );

	virtual void			ServerSendReliableMessage( int clientNum, const idBitMsg &msg, bool inhibitRepeater = false );
	virtual void			RepeaterSendReliableMessage( int clientNum, const idBitMsg &msg, bool inhibitHeader = false, int including = -1 );
	virtual void			RepeaterSendReliableMessageExcluding( int excluding, const idBitMsg &msg, bool inhibitHeader = false, int clientNum = -1 ); // NOTE: Message is sent to all viewers if clientNum is -1; excluding is used for playback.
	virtual void			ServerSendReliableMessageExcluding( int clientNum, const idBitMsg &msg, bool inhibitRepeater = false );
	virtual int				ServerGetClientPing( int clientNum );
	virtual int				ServerGetClientTimeSinceLastPacket( int clientNum );
	virtual int				ServerGetClientTimeSinceLastInput( int clientNum );
	virtual int				ServerGetClientOutgoingRate( int clientNum );
	virtual int				ServerGetClientIncomingRate( int clientNum );
	virtual float			ServerGetClientIncomingPacketLoss( int clientNum );
	virtual int				ServerGetClientNum( int clientId );
	virtual	int				ServerGetServerTime( void );

	// returns the new clientNum or -1 if there weren't any free.
	virtual int				ServerConnectBot( void );

	virtual int				RepeaterGetClientNum( int clientId );

	virtual void			ClientSendReliableMessage( const idBitMsg &msg );
	virtual int				ClientGetPrediction( void );
	virtual int				ClientGetTimeSinceLastPacket( void );
	virtual int				ClientGetOutgoingRate( void );
	virtual int				ClientGetIncomingRate( void );
	virtual float			ClientGetIncomingPacketLoss( void );
// RAVEN BEGIN
// ddynerman: added some utility functions
	// uses a static buffer, copy it before calling in game again
	virtual const char*		GetServerAddress( void );
	virtual const char*		GetClientAddress( int clientNum );
	virtual	void			AddFriend( int clientNum );
	virtual void			RemoveFriend( int clientNum );
	// for MP games
	virtual void			SetLoadingText( const char* loadingText );
	virtual void			AddLoadingIcon( const char* icon );
	virtual const char*		GetClientGUID( int clientNum );
// RAVEN END

	virtual void			GetTrafficStats(  int &bytesSent, int &packetsSent, int &bytesReceived, int &packetsReceived ) const;

	// server browser
	virtual int				GetNumScannedServers( void );
	virtual const scannedServer_t*	GetScannedServerInfo( int serverNum );
	virtual const scannedClient_t*	GetScannedServerClientInfo( int serverNum, int clientNum );
	virtual void			AddSortFunction( const sortInfo_t &sortInfo );
	virtual bool			RemoveSortFunction( const sortInfo_t &sortInfo );
	virtual void			UseSortFunction( const sortInfo_t &sortInfo, bool use = true );
	virtual bool			SortFunctionIsActive( const sortInfo_t &sortInfo );

	// returns true if enabled
	virtual bool			HTTPEnable( bool enable );

	virtual void			ClientSetServerInfo( const idDict &serverSI );
	virtual void			RepeaterSetInfo( const idDict &info );

	virtual const char*		GetViewerGUID( int clientNum );

private:
	scannedServer_t			scannedServer;
	scannedClient_t			scannedClient;
};

extern idNetworkSystem *	networkSystem;

#endif /* !__NETWORKSYSTEM_H__ */
