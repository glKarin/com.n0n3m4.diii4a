/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef SE_INCL_COMMUNICATIONINTERFACE_H
#define SE_INCL_COMMUNICATIONINTERFACE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#ifdef PLATFORM_UNIX
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define closesocket close
typedef int SOCKET;
typedef struct hostent HOSTENT;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr    SOCKADDR;
#define WSAGetLastError() (INDEX) errno
#endif

#define SERVER_CLIENTS 16

#include <Engine/Network/CPacket.h>

// Communication class
class ENGINE_API CCommunicationInterface {
public:
	BOOL cci_bSocketOpen;		// set if socket is open and working
  BOOL cci_bBound;				// set for udp sockets that have been explicitly or implicitly bound
	BOOL cci_bInitialized;	// is the communication interface initialized or not
	BOOL cci_bWinSockOpen;	// is the winsock API initialized
	BOOL cci_bServerInitialized;
	BOOL cci_bClientInitialized;
		
  CPacketBuffer cci_pbMasterOutput;					// master output buffer				 
  CPacketBuffer cci_pbMasterInput;					// master input buffer

  SOCKET cci_hSocket;						// the socket handle itself

public:
  // client
  void Client_OpenLocal(void);
  void Client_OpenNet_t(ULONG ulServerAddress);
  // update master UDP socket and route its messages
  void UpdateMasterBuffers(void);

public:
  CCommunicationInterface(void);
  ~CCommunicationInterface(void){};

  // start/stop protocols
  void Init(void);
  void Close(void);

  void InitWinsock(void);
  void EndWinsock(void);
  void PrepareForUse(BOOL bUseNetwork, BOOL bClient);
  void Unprepare(void);
  BOOL IsNetworkEnabled(void);
  // get address of local machine
  void GetHostName(CTString &strName, CTString &strAddress);

  // create an inet-family socket
  void CreateSocket_t();
  // bind socket to the given address
  void Bind_t(ULONG ulLocalHost, ULONG ulLocalPort);
  // set socket to non-blocking mode
  void SetNonBlocking_t(void);
  // get generic socket error info string and last error
  CTString GetSocketError(INDEX iError);
	// open an UDP socket at given port 
  void OpenSocket_t(ULONG ulLocalHost, ULONG ulLocalPort);

	// get address of this host
  void GetLocalAddress_t(ULONG &ulHost, ULONG &ulPort);
  // get address of the peer host connected to this socket
  void GetRemoteAddress_t(ULONG &ulHost, ULONG &ulPort);

  // broadcast communication
  void Broadcast_Send(const void *pvSend, SLONG slSendSize,CAddress &adrDestination);
  BOOL Broadcast_Receive(void *pvReceive, SLONG &slReceiveSize,CAddress &adrAddress);
	// here we receive connect requests
	void Broadcast_Update_t(void);

  // Server
  void Server_Init_t(void);
  void Server_Close(void);

  void Server_ClearClient(INDEX iClient);
  BOOL Server_IsClientLocal(INDEX iClient);
  BOOL Server_IsClientUsed(INDEX iClient);
  CTString Server_GetClientName(INDEX iClient);

  void Server_Send_Reliable(INDEX iClient, const void *pvSend, SLONG slSendSize);
  BOOL Server_Receive_Reliable(INDEX iClient, void *pvReceive, SLONG &slReceiveSize);
  void Server_Send_Unreliable(INDEX iClient, const void *pvSend, SLONG slSendSize);
  BOOL Server_Receive_Unreliable(INDEX iClient, void *pvReceive, SLONG &slReceiveSize);

  BOOL Server_Update(void);

  // Client
  void Client_Init_t(char* strServerName);
  void Client_Init_t(ULONG ulServerAddress);
  void Client_Close(void);

  void Client_Clear(void);
  BOOL Client_IsConnected(void);

  void Client_Send_Reliable(const void *pvSend, SLONG slSendSize);
  BOOL Client_Receive_Reliable(void *pvReceive, SLONG &slReceiveSize);
  BOOL Client_Receive_Reliable(CTStream &strmReceive);
  void Client_PeekSize_Reliable(SLONG &slExpectedSize,SLONG &slReceivedSize);
  void Client_Send_Unreliable(const void *pvSend, SLONG slSendSize);
  BOOL Client_Receive_Unreliable(void *pvReceive, SLONG &slReceiveSize);

  BOOL Client_Update(void);
};

extern ENGINE_API CCommunicationInterface _cmiComm;
extern CPacketBufferStats _pbsSend;
extern CPacketBufferStats _pbsRecv;

#endif  /* include-once check. */
