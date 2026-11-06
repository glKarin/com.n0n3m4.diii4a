// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "SDNetSession_local.h"

//===============================================================
//
//	sdNetSession
//
//===============================================================

sdNetSession_Local::sdNetSession_Local()
	: sessionState(SS_IDLE)
{
	sessionId.Clear();
	address.type = NA_BAD;
	address.ip[0] = 127;
	address.ip[1] = 0;
	address.ip[2] = 0;
	address.ip[3] = 1;
	address.port = 1234;
}

sdNetSession_Local::~sdNetSession_Local() {
}

sdNetSession::sessionState_e sdNetSession_Local::GetState() const {
	return sessionState;
}

	//
	// Retrieve session details
	//

	// Reverse looked up hostname
const char* sdNetSession_Local::GetHostName() const {
	return "localhost";
}

	// Get the host address as "ip:port"
const char* sdNetSession_Local::GetHostAddressString() const {
	return "127.0.0.1";
}

const netadr_t& sdNetSession_Local::GetAddress() const {
	return address;
}

	// Get server info
const idDict& sdNetSession_Local::GetServerInfo() const {
	return serverInfo;
}

idDict& sdNetSession_Local::GetServerInfo() {
	return serverInfo;
}

	// Get number of clients
const int sdNetSession_Local::GetNumClients() const {
	return 0;
}

	// Get number of bot clients
const int sdNetSession_Local::GetNumBotClients() const {
	return 0;
}
	
	// Retrieve client data
const sdNetSession::sessionClientInfo_t& sdNetSession_Local::GetClientInfo( int clientNum ) const {
	return clientInfos[clientNum];
}

	// Get latency
const int sdNetSession_Local::GetPing() const {
	return 0;
}

	// Get ranked status
bool sdNetSession_Local::IsRanked() const {
	return false;
}

	// Get session game status
byte sdNetSession_Local::GetGameState() const {
	return 0;
}

	// Get how much time is left (in ms)
int sdNetSession_Local::GetSessionTime() const {
	return 0;
}

	// Connect to this session
bool sdNetSession_Local::Join() {
	return true;
}

	// Get the unique identifier for this session
void sdNetSession_Local::GetId( sessionId_t& sessionId ) const {
	sessionId = this->sessionId;
}

	// When a client connects to this session
void sdNetSession_Local::ServerClientConnect( const int clientNum ) {
}

	// When a client disconnects from this session
void sdNetSession_Local::ServerClientDisconnect( const int clientNum ) {
}

	// Get number of clients considering joining
int sdNetSession_Local::GetNumInterestedClients( void ) const {
	return 0;
}

	// Get repeater status
bool sdNetSession_Local::IsRepeater() const {
	return false;
}

	// Get number of people on the repeater
int sdNetSession_Local::GetNumRepeaterClients( void ) const {
	return 0;
}

	// Get max number of people on the repeater
int sdNetSession_Local::GetMaxRepeaterClients( void ) const {
	return 0;
}
