// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETSESSION_H__ )
#define __SDNETSESSION_H__

//===============================================================
//
//	sdNetSession
//
//===============================================================

class sdNetTask;

class sdNetSession {
public:
	enum sessionState_e {
		SS_IDLE,
		SS_ADVERTISING,
		SS_ADVERTISED,
		SS_DEADVERTISING
	};

	static const int MAX_NICKLEN				= 32;

	// client info reported from a session
	struct sessionClientInfo_t {
						sessionClientInfo_t() :
							ping( 999 ),
							connected( false ),
							isBot( false ) {
							nickname[0] = '\0';
						}

		char			nickname[ MAX_NICKLEN ];
		short			ping;
		int				rate;
		bool			connected;
		bool			isBot;
	};

	struct sessionId_t {
		byte	id[8];

		void	Clear() { ::memset( id, 0, sizeof( id ) ); }
	};

	virtual					~sdNetSession() {}

	virtual sessionState_e	GetState() const = 0;

	//
	// Retrieve session details
	//

	// Reverse looked up hostname
	virtual const char*			GetHostName() const = 0;

	// Get the host address as "ip:port"
	virtual const char*			GetHostAddressString() const = 0;

	virtual const netadr_t&		GetAddress() const = 0;

	// Get server info
	virtual const idDict&		GetServerInfo() const = 0;
	virtual idDict&				GetServerInfo() = 0;

	// Get number of clients
	virtual const int			GetNumClients() const = 0;

	// Get number of bot clients
	virtual const int			GetNumBotClients() const = 0;
	
	// Retrieve client data
	virtual const sessionClientInfo_t& GetClientInfo( int clientNum ) const = 0;

	// Get latency
	virtual const int			GetPing() const = 0;

	// Get ranked status
	virtual bool				IsRanked() const = 0;

	// Get session game status
	virtual byte				GetGameState() const = 0;

	// Get how much time is left (in ms)
	virtual int					GetSessionTime() const = 0;

	// Connect to this session
	virtual bool				Join() = 0;

	// Get the unique identifier for this session
	virtual void				GetId( sessionId_t& sessionId ) const = 0;

	// When a client connects to this session
	virtual void				ServerClientConnect( const int clientNum ) = 0;

	// When a client disconnects from this session
	virtual void				ServerClientDisconnect( const int clientNum ) = 0;

	// Get number of clients considering joining
	virtual int					GetNumInterestedClients( void ) const = 0;

	// Get repeater status
	virtual bool				IsRepeater() const = 0;

	// Get number of people on the repeater
	virtual int					GetNumRepeaterClients( void ) const = 0;

	// Get max number of people on the repeater
	virtual int					GetMaxRepeaterClients( void ) const = 0;

protected:

};

#endif /* !__SDNETSESSION_H__ */
