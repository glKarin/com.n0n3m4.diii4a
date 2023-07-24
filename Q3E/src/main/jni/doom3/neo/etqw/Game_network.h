// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_NETWORK_H__
#define __GAME_NETWORK_H__

#define ASYNC_BUFFER_SECURITY
const int ASYNC_MAGIC_NUMBER = 12345678;

#include "network/SnapshotState.h"

class sdReliableMessageClientInfoBase {
protected:
								sdReliableMessageClientInfoBase() { ; }

public:
	bool						SendToRepeaterClients( void ) const { return sendToRepeaterClients; }
	bool						SendToClients( void ) const { return sendToClients; }
	bool						SendToAll( void ) const { return clientNum == -1; }
	bool						DontSendToRelays( void ) const { return dontSendToRelays; }
	int							GetClientNum( void ) const { return clientNum; }

protected:
	bool						sendToClients;
	bool						sendToRepeaterClients;
	bool						dontSendToRelays;
	int							clientNum;
};

class sdReliableMessageClientInfo : public sdReliableMessageClientInfoBase {
public:
	sdReliableMessageClientInfo( int clientNum ) {
		this->clientNum = clientNum;
		this->sendToClients = true;
		this->sendToRepeaterClients = false;
		this->dontSendToRelays = false;
	}
};

class sdReliableMessageClientInfoRepeater : public sdReliableMessageClientInfoBase {
public:
	sdReliableMessageClientInfoRepeater( int clientNum ) {
		this->clientNum = clientNum;
		this->sendToClients = false;
		this->sendToRepeaterClients = true;
		this->dontSendToRelays = false;
	}
};

class sdReliableMessageClientInfoAll : public sdReliableMessageClientInfoBase {
public:
	sdReliableMessageClientInfoAll( void ) {
		this->clientNum = -1;
		this->sendToClients = true;
		this->sendToRepeaterClients = true;
		this->dontSendToRelays = false;
	}
};

class sdReliableMessageClientInfoRepeaterLocal : public sdReliableMessageClientInfoBase {
public:
	sdReliableMessageClientInfoRepeaterLocal( void ) {
		this->clientNum = -1;
		this->sendToClients = false;
		this->sendToRepeaterClients = true;
		this->dontSendToRelays = true;
	}
};

class sdReliableServerMessage : public idBitMsg {
public:
	sdReliableServerMessage( gameReliableServerMessage_t _type ) : type( _type ) {
		InitWrite( buffer, sizeof( buffer ) );
		WriteBits( type, idMath::BitsForInteger( GAME_RELIABLE_SMESSAGE_NUM_MESSAGES ) );
	}

	void Send( const sdReliableMessageClientInfoBase& info ) const;

protected:
	byte				buffer[ MAX_GAME_MESSAGE_SIZE ];
	gameReliableServerMessage_t type;
};

class sdReliableClientMessage : public idBitMsg {
public:
	sdReliableClientMessage( gameReliableClientMessage_t type ) {
		InitWrite( buffer, sizeof( buffer ) );
		WriteBits( type, idMath::BitsForInteger( GAME_RELIABLE_CMESSAGE_NUM_MESSAGES ) );
	}

	void Send( void ) const {
		networkSystem->ClientSendReliableMessage( *this );
	}

protected:
	byte				buffer[ MAX_GAME_MESSAGE_SIZE ];
};

#endif // __GAME_NETWORK_H__
