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

#include <Engine/StdH.h>

#include <Engine/Base/Console.h>
#include <Engine/Base/CTString.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/ErrorTable.h>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Base/Synchronization.h>
#include <Engine/Base/Translation.h>

#include <Engine/Network/ClientInterface.h>
#include <Engine/Network/CommunicationInterface.h>
#include <Engine/Network/CPacket.h>
#include <Engine/Network/Network.h>
#include <Engine/Network/Server.h>

#include <Engine/GameAgent/GameAgent.h>

#ifdef PLATFORM_WIN32
typedef int socklen_t;
#pragma comment(lib, "wsock32.lib")
#endif

#define SLASHSLASH  0x2F2F   // looks like "//" in ASCII.

#define SERVER_LOCAL_CLIENT     0
extern INDEX net_iPort;
extern CTString net_strLocalHost;
extern INDEX net_bLookupHostNames;
extern INDEX net_bReportICMPErrors;
extern FLOAT net_fDropPackets;
extern FLOAT net_tmConnectionTimeout;
extern INDEX net_bReportPackets;

static struct ErrorCode ErrorCodes[] = {
#ifdef PLATFORM_WIN32
  ERRORCODE(WSAEINTR          , "WSAEINTR"),
  ERRORCODE(WSAEBADF          , "WSAEBADF"),
  ERRORCODE(WSAEACCES         , "WSAEACCES"),
  ERRORCODE(WSAEFAULT         , "WSAEFAULT"),
  ERRORCODE(WSAEINVAL         , "WSAEINVAL"),
  ERRORCODE(WSAEMFILE         , "WSAEMFILE"),
  ERRORCODE(WSAEWOULDBLOCK    , "WSAEWOULDBLOCK"),
  ERRORCODE(WSAEINPROGRESS    , "WSAEINPROGRESS"),
  ERRORCODE(WSAEALREADY       , "WSAEALREADY"),
  ERRORCODE(WSAENOTSOCK       , "WSAENOTSOCK"),
  ERRORCODE(WSAEDESTADDRREQ   , "WSAEDESTADDRREQ"),
  ERRORCODE(WSAEMSGSIZE       , "WSAEMSGSIZE"),
  ERRORCODE(WSAEPROTOTYPE     , "WSAEPROTOTYPE"),
  ERRORCODE(WSAENOPROTOOPT    , "WSAENOPROTOOPT"),
  ERRORCODE(WSAEPROTONOSUPPORT, "WSAEPROTONOSUPPORT"),
  ERRORCODE(WSAESOCKTNOSUPPORT, "WSAESOCKTNOSUPPORT"),
  ERRORCODE(WSAEOPNOTSUPP     , "WSAEOPNOTSUPP"),
  ERRORCODE(WSAEPFNOSUPPORT   , "WSAEPFNOSUPPORT"),
  ERRORCODE(WSAEAFNOSUPPORT   , "WSAEAFNOSUPPORT"),
  ERRORCODE(WSAEADDRINUSE     , "WSAEADDRINUSE"),
  ERRORCODE(WSAEADDRNOTAVAIL  , "WSAEADDRNOTAVAIL"),
  ERRORCODE(WSAENETDOWN       , "WSAENETDOWN"),
  ERRORCODE(WSAENETUNREACH    , "WSAENETUNREACH"),
  ERRORCODE(WSAENETRESET      , "WSAENETRESET"),
  ERRORCODE(WSAECONNABORTED   , "WSAECONNABORTED"),
  ERRORCODE(WSAECONNRESET     , "WSAECONNRESET"),
  ERRORCODE(WSAENOBUFS        , "WSAENOBUFS"),
  ERRORCODE(WSAEISCONN        , "WSAEISCONN"),
  ERRORCODE(WSAENOTCONN       , "WSAENOTCONN"),
  ERRORCODE(WSAESHUTDOWN      , "WSAESHUTDOWN"),
  ERRORCODE(WSAETOOMANYREFS   , "WSAETOOMANYREFS"),
  ERRORCODE(WSAETIMEDOUT      , "WSAETIMEDOUT"),
  ERRORCODE(WSAECONNREFUSED   , "WSAECONNREFUSED"),
  ERRORCODE(WSAELOOP          , "WSAELOOP"),
  ERRORCODE(WSAENAMETOOLONG   , "WSAENAMETOOLONG"),
  ERRORCODE(WSAEHOSTDOWN      , "WSAEHOSTDOWN"),
  ERRORCODE(WSAEHOSTUNREACH   , "WSAEHOSTUNREACH"),
  ERRORCODE(WSASYSNOTREADY    , "WSASYSNOTREADY"),
  ERRORCODE(WSAVERNOTSUPPORTED, "WSAVERNOTSUPPORTED"),
  ERRORCODE(WSANOTINITIALISED , "WSANOTINITIALISED"),
  ERRORCODE(WSAEDISCON        , "WSAEDISCON"),
  ERRORCODE(WSAHOST_NOT_FOUND , "WSAHOST_NOT_FOUND"),
  ERRORCODE(WSATRY_AGAIN      , "WSATRY_AGAIN"),
  ERRORCODE(WSANO_RECOVERY    , "WSANO_RECOVERY"),
  ERRORCODE(WSANO_DATA        , "WSANO_DATA"),

#else

    // these were gleaned from the manpages for various BSD socket calls.
    //  On linux, start with "man ip" for most of these.

  ERRORCODE(EINTR             , "EINTR"),
  ERRORCODE(EAGAIN            , "EAGAIN"),
  ERRORCODE(EIO               , "EIO"),
  ERRORCODE(EISDIR            , "EISDIR"),
  ERRORCODE(EBADF             , "EBADF"),
  ERRORCODE(EINVAL            , "EINVAL"),
  ERRORCODE(EFAULT            , "EFAULT"),
  ERRORCODE(EPROTONOSUPPORT   , "EPROTONOSUPPORT"),
  ERRORCODE(ENFILE            , "ENFILE"),
  ERRORCODE(EACCES            , "EACCES"),
  ERRORCODE(ENOBUFS           , "ENOBUFS"),
  ERRORCODE(ENOMEM            , "ENOMEM"),
  ERRORCODE(ENOTSOCK          , "ENOTSOCK"),
  ERRORCODE(EOPNOTSUPP        , "EOPNOTSUPP"),
  ERRORCODE(EPERM             , "EPERM"),
  ERRORCODE(ECONNABORTED      , "ECONNABORTED"),
  ERRORCODE(ECONNREFUSED      , "ECONNREFUSED"),
  ERRORCODE(ENETUNREACH       , "ENETUNREACH"),
  ERRORCODE(EADDRINUSE        , "EADDRINUSE"),
  ERRORCODE(EINPROGRESS       , "EINPROGRESS"),
  ERRORCODE(EALREADY          , "EALREADY"),
  ERRORCODE(EAGAIN            , "EAGAIN"),
  ERRORCODE(EAFNOSUPPORT      , "EAFNOSUPPORT"),
  ERRORCODE(EADDRNOTAVAIL     , "EADDRNOTAVAIL"),
  ERRORCODE(ETIMEDOUT         , "ETIMEDOUT"),
  ERRORCODE(ESOCKTNOSUPPORT   , "ESOCKTNOSUPPORT"),
  ERRORCODE(ENAMETOOLONG      , "ENAMETOOLONG"),
  ERRORCODE(ENOTDIR           , "ENOTDIR"),
  ERRORCODE(ELOOP             , "ELOOP"),
  ERRORCODE(EROFS             , "EROFS"),
  ERRORCODE(EISCONN           , "EISCONN"),
  ERRORCODE(EMSGSIZE          , "EMSGSIZE"),
  ERRORCODE(ENODEV            , "ENODEV"),
  ERRORCODE(ECONNRESET        , "ECONNRESET"),
  ERRORCODE(ENOTCONN          , "ENOTCONN"),
  #ifdef ENOSR
  ERRORCODE(ENOSR             , "ENOSR"),
  #endif
  #ifdef ENOPKG
  ERRORCODE(ENOPKG            , "ENOPKG"),
  #endif
#endif
};
static struct ErrorTable SocketErrors = ERRORTABLE(ErrorCodes);

// rcg10122001
#ifdef PLATFORM_WIN32
#define isWouldBlockError(x) (x == WSAEWOULDBLOCK)
#else
#define isWouldBlockError(x) ((x == EAGAIN) || (x == EWOULDBLOCK))
#define WSAECONNRESET ECONNRESET
#endif


//structures used to emulate bandwidth and latency parameters - shared by all client interfaces
CPacketBufferStats _pbsSend;
CPacketBufferStats _pbsRecv;

ULONG cm_ulLocalHost;			// configured local host address
CTString cm_strAddress;   // local address
CTString cm_strName;			// local address

CTCriticalSection cm_csComm;  // critical section for access to communication data

BOOL cm_bNetworkInitialized;

// index 0 is the server's local client, this is an array used by server only
CClientInterface cm_aciClients[SERVER_CLIENTS];

// Broadcast interface - i.e. interface for 'nonconnected' communication
CClientInterface cm_ciBroadcast;

// this is used by client only
CClientInterface cm_ciLocalClient;

// global communication interface object (there is only one for the entire engine)
CCommunicationInterface _cmiComm;


/*
*
*	Two helper functions - conversion from IP to words
*
*/

// convert address to a printable string
CTString AddressToString(ULONG ulHost)
{
  ULONG ulHostNet = htonl(ulHost);

  // initially not converted
  struct hostent *hostentry = NULL;

  // if DNS lookup is allowed
  if (net_bLookupHostNames) {
    // lookup the host
	  hostentry = gethostbyaddr ((char *)&ulHostNet, sizeof(ulHostNet), AF_INET);
  }

  // if DNS lookup succeeded
	if (hostentry!=NULL) {
    // return its ascii name
    return (char *)hostentry->h_name;
  // if DNS lookup failed
  } else {
    // just convert to dotted number format
    return inet_ntoa((const in_addr &)ulHostNet);
  }
};

// convert string address to a number
ULONG StringToAddress(const CTString &strAddress)
{
  // first try to convert numeric address
  ULONG ulAddress = ntohl(inet_addr(strAddress));
  // if not a valid numeric address
  if (ulAddress==INADDR_NONE) {
    // lookup the host
    #ifdef PLATFORM_FREEBSD
    CPrintF("StringToAddressr: %s.\n",  (const char *) strAddress);
    #endif
    HOSTENT *phe = gethostbyname(strAddress); // crash on FreeBSD
    // if succeeded
    if (phe!=NULL) {
      // get that address
      ulAddress = ntohl(*(ULONG*)phe->h_addr_list[0]);
    }
  }

  // return what we got
  return ulAddress;
};



CCommunicationInterface::CCommunicationInterface(void)
{
  cm_csComm.cs_iIndex = -1;
  CTSingleLock slComm(&cm_csComm, TRUE);

  cci_bInitialized = FALSE;
  cci_bWinSockOpen = FALSE;

  cci_bServerInitialized = FALSE;
  cci_bClientInitialized = FALSE;
  cm_ciLocalClient.ci_bClientLocal = FALSE;

	cci_hSocket=INVALID_SOCKET;

};


// initialize
void CCommunicationInterface::Init(void) 
{
  CTSingleLock slComm(&cm_csComm, TRUE);

  cci_bWinSockOpen = FALSE;
	cci_bInitialized = TRUE;

  // mark as initialized
  cm_bNetworkInitialized = FALSE;

	cci_pbMasterInput.Clear();
	cci_pbMasterOutput.Clear();

};

// close
void CCommunicationInterface::Close(void) 
{
  CTSingleLock slComm(&cm_csComm, TRUE);

  ASSERT(cci_bInitialized);
  ASSERT(!cci_bServerInitialized);
  ASSERT(!cci_bClientInitialized);

  // mark as closed
  cm_bNetworkInitialized = FALSE;
  cci_bInitialized = FALSE;
	cm_ciLocalClient.ci_bClientLocal = FALSE;

	cci_pbMasterInput.Clear();
	cci_pbMasterOutput.Clear();

};

void CCommunicationInterface::InitWinsock(void)
{
#ifdef PLATFORM_WIN32
  if (cci_bWinSockOpen) {
    return;
  }

  // start winsock
  WSADATA	winsockdata;
  WORD wVersionRequested;
  wVersionRequested = MAKEWORD(1, 1);
  int iResult = WSAStartup(wVersionRequested, &winsockdata);
  // if winsock open ok
  if (iResult==0) {
    // remember that
    cci_bWinSockOpen = TRUE;
    CPrintF(TRANSV("  winsock opened ok\n"));
  }
#else
    cci_bWinSockOpen = TRUE;
#endif
};

void CCommunicationInterface::EndWinsock(void)
{
  if (!cci_bWinSockOpen) {
    return;
  }

#ifdef PLATFORM_WIN32
  int iResult = WSACleanup();
  ASSERT(iResult==0);
#endif

  cci_bWinSockOpen = FALSE;
};


// prepares the comm interface for use - MUST be invoked before any data can be sent/received
void CCommunicationInterface::PrepareForUse(BOOL bUseNetwork, BOOL bClient)
{

	// clear the network conditions emulation data
  _pbsSend.Clear();
  _pbsRecv.Clear();

	// if the network is already initialized, shut it down before proceeding
  if (cm_bNetworkInitialized) {
    Unprepare();
  }

  // make sure winsock is off (could be on if enumeration was triggered)
  GameAgent_EnumCancel();
  EndWinsock();

  if (bUseNetwork) {
    CPrintF(TRANSV("Initializing TCP/IP...\n"));
    if (bClient) {
      CPrintF(TRANSV("  opening as client\n"));
    } else {
      CPrintF(TRANSV("  opening as server\n"));
    }

    // make sure winsock is on
    InitWinsock();

    // no address by default
    cm_ulLocalHost = 0;
    // if there is a desired local address
    if (net_strLocalHost!="") {
      CPrintF(TRANSV("  user forced local address: %s\n"), (const char*)net_strLocalHost);
      // use that address
      cm_strName = net_strLocalHost;
      cm_ulLocalHost = StringToAddress(cm_strName);
      // if invalid
      if (cm_ulLocalHost==0 || cm_ulLocalHost==(static_cast<ULONG>(-1))) {
        cm_ulLocalHost=0;
        // report it
        CPrintF(TRANSV("  requested local address is invalid\n"));
      }
    }

    // if no valid desired local address
    CPrintF(TRANSV("  getting local addresses\n"));
    // get default
    char hostname[256];
    gethostname(hostname, sizeof(hostname)-1);
    //CPrintF("hostname: %s\n", (const char*) hostname);
    #ifdef PLATFORM_FREEBSD 
    cm_strName = "localhost";
    #else
    cm_strName = hostname;
    #endif
    // lookup the host
    HOSTENT *phe = gethostbyname(cm_strName);
    // if succeeded
    if (phe!=NULL) {
      // get the addresses
      cm_strAddress = "";
      for(INDEX i=0; phe->h_addr_list[i]!=NULL; i++) {
        if (i>0) {
          cm_strAddress += ", ";
        }
        cm_strAddress += inet_ntoa(*(const in_addr *)phe->h_addr_list[i]);
      }
    }

    CPrintF(TRANSV("  local addresses: %s (%s)\n"), (const char *) cm_strName, (const char *) cm_strAddress);
    CPrintF(TRANSV("  port: %d\n"), net_iPort);

    // try to open master UDP socket
    try {
      OpenSocket_t(cm_ulLocalHost, bClient?0:net_iPort);
			cci_pbMasterInput.pb_ppbsStats = NULL;
			cci_pbMasterOutput.pb_ppbsStats = NULL;
      cm_ciBroadcast.SetLocal(NULL);
      CPrintF(TRANSV("  opened socket: \n"));
    } catch (const char *strError) {
      CPrintF(TRANSV("  cannot open UDP socket: %s\n"), strError);
    }
  }
  
  cm_bNetworkInitialized = cci_bWinSockOpen;
};


// shut down the communication interface
void CCommunicationInterface::Unprepare(void)
{
  // close winsock
  if (cci_bWinSockOpen) {
		// if socket is open
		if (cci_hSocket != INVALID_SOCKET) {
			// close it
			closesocket(cci_hSocket);
			cci_hSocket = INVALID_SOCKET;
		}

    cm_ciBroadcast.Clear();
    EndWinsock();
		cci_bBound=FALSE;
  }

  cci_pbMasterInput.Clear();
	cci_pbMasterOutput.Clear();


  cm_bNetworkInitialized = cci_bWinSockOpen;
	
};


BOOL CCommunicationInterface::IsNetworkEnabled(void)
{
  return cm_bNetworkInitialized;
};

// get address of local machine
void CCommunicationInterface::GetHostName(CTString &strName, CTString &strAddress)
{
  strName = cm_strName;
  strAddress = cm_strAddress;
};



/*
*
*
*	Socket functions - creating, binding...
*
*
*/


// create an inet-family socket
void CCommunicationInterface::CreateSocket_t()
{
  ASSERT(cci_hSocket==INVALID_SOCKET);
  // open the socket
  cci_hSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	cci_bBound = FALSE;
  if (cci_hSocket == INVALID_SOCKET) {
    ThrowF_t(TRANS("Cannot open socket. %s"), (const char*)GetSocketError(WSAGetLastError()));
  }

};

// bind socket to the given address
void CCommunicationInterface::Bind_t(ULONG ulLocalHost, ULONG ulLocalPort)
{
  if (cci_hSocket==INVALID_SOCKET) {
    ASSERT(FALSE);
    return;
  }

  sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(ulLocalPort);
  sin.sin_addr.s_addr = htonl(ulLocalHost);

  // bind socket to server address/port
  if (bind(cci_hSocket, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
    ThrowF_t(TRANS("Cannot bind socket. %s"), (const char*)GetSocketError(WSAGetLastError()));
  }
  cci_bBound = TRUE;
};


// set socket to non-blocking mode
void CCommunicationInterface::SetNonBlocking_t(void)
{
  if (cci_hSocket==INVALID_SOCKET) {
    ASSERT(FALSE);
    return;
  }

#ifdef PLATFORM_WIN32
  ULONG ulArgNonBlocking = 1;
  if (ioctlsocket(cci_hSocket, FIONBIO, &ulArgNonBlocking) == SOCKET_ERROR) {
    ThrowF_t(TRANS("Cannot set socket to non-blocking mode. %s"), 
      (const char*)GetSocketError(WSAGetLastError()));
  }
#else
  int flags = fcntl(cci_hSocket, F_GETFL);
  int failed = flags;
  if (failed != -1) {
      flags |= O_NONBLOCK;
      failed = fcntl(cci_hSocket, F_SETFL, flags);
  }

  if (failed == -1) {
    ThrowF_t(TRANS("Cannot set socket to non-blocking mode. %s"), 
      (const char*)GetSocketError(WSAGetLastError()));
  }
#endif

};


// get generic socket error info string about last error
CTString CCommunicationInterface::GetSocketError(INDEX iError)
{
  CTString strError;
  strError.PrintF(TRANSV("Socket %d, Error %d (%s)"),
    cci_hSocket, iError, ErrorDescription(&SocketErrors, iError));
  return strError;
};


// open an UDP socket at given port
void CCommunicationInterface::OpenSocket_t(ULONG ulLocalHost, ULONG ulLocalPort)
{
  // create the socket as UDP
  CreateSocket_t();
  // bind it to that address/port
  if (ulLocalPort!=0) {
    Bind_t(ulLocalHost, ulLocalPort);
  }
  // go non-blocking
  SetNonBlocking_t();

  // mark as open
  cci_bSocketOpen = TRUE;


};

// get address of this host
void CCommunicationInterface::GetLocalAddress_t(ULONG &ulHost, ULONG &ulPort)
{
  ulHost = 0;
  ulPort = 0;
  if (cci_hSocket==INVALID_SOCKET) {
    ASSERT(FALSE);
    return;
  }

  // get socket local port and address
  sockaddr_in sin;
  socklen_t iSize = sizeof(sin);
  if (getsockname(cci_hSocket, (sockaddr*)&sin, &iSize) == SOCKET_ERROR) {
    ThrowF_t(TRANS("Cannot get local address on socket. %s"), 
      (const char*)GetSocketError(WSAGetLastError()));
  }

#ifdef PLATFORM_WIN32
  ulHost = ntohl(sin.sin_addr.S_un.S_addr);
#else
  ulHost = ntohl(sin.sin_addr.s_addr);
#endif

  ulPort = ntohs(sin.sin_port);
}

// get address of the peer host connected to this socket
void CCommunicationInterface::GetRemoteAddress_t(ULONG &ulHost, ULONG &ulPort)
{
  ulHost = 0;
  ulPort = 0;
  if (cci_hSocket==INVALID_SOCKET) {
    ASSERT(FALSE);
    return;
  }

  // get socket local port
  sockaddr_in sin;
  socklen_t iSize = sizeof(sin);
  if (getpeername(cci_hSocket, (sockaddr*)&sin, &iSize) == SOCKET_ERROR) {
    ThrowF_t(TRANS("Cannot get remote address on socket. %s"), 
      (const char*)GetSocketError(WSAGetLastError()));
  }

#ifdef PLATFORM_WIN32
  ulHost = ntohl(sin.sin_addr.S_un.S_addr);
#else
  ulHost = ntohl(sin.sin_addr.s_addr);
#endif
  ulPort = ntohs(sin.sin_port);
}


/*
 *  ---->>>>  BROADCAST INTERFACE   <<<<----
 */

// broadcast communication
void CCommunicationInterface::Broadcast_Send(const void *pvSend, SLONG slSendSize,CAddress &adrDestination)
{
  CTSingleLock slComm(&cm_csComm, TRUE);

  cm_ciBroadcast.ci_adrAddress.adr_ulAddress = adrDestination.adr_ulAddress;
  cm_ciBroadcast.ci_adrAddress.adr_uwPort = adrDestination.adr_uwPort;
  cm_ciBroadcast.ci_adrAddress.adr_uwID = adrDestination.adr_uwID;

  cm_ciBroadcast.Send(pvSend, slSendSize,FALSE);
}

BOOL CCommunicationInterface::Broadcast_Receive(void *pvReceive, SLONG &slReceiveSize,CAddress &adrAddress)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  return cm_ciBroadcast.ReceiveFrom(pvReceive, slReceiveSize,&adrAddress,FALSE);
}


// update the broadcast input buffer - handle any incoming connection requests
void CCommunicationInterface::Broadcast_Update_t() {
	CPacket* ppaConnectionRequest;
	BOOL bIsAlready;
	BOOL bFoundEmpty;
	ULONG iClient;
	//UBYTE ubDummy=65;

	
	// while there is a connection request packet in the input buffer
	while ((ppaConnectionRequest = cm_ciBroadcast.ci_pbReliableInputBuffer.GetConnectRequestPacket()) != NULL) {
		// see if there is a client already connected at that address and port
		bIsAlready = FALSE;
		for (iClient=1;iClient<SERVER_CLIENTS;iClient++) {
			if (cm_aciClients[iClient].ci_adrAddress.adr_ulAddress == ppaConnectionRequest->pa_adrAddress.adr_ulAddress &&
					cm_aciClients[iClient].ci_adrAddress.adr_uwPort == ppaConnectionRequest->pa_adrAddress.adr_uwPort) {
					bIsAlready = TRUE;
					break;
			}
		}
		// if the client is already connected then just ignore the packet - else, connect it
		if (!bIsAlready) {
			// find an empty client structure
			bFoundEmpty = FALSE;
			for (iClient=1;iClient<SERVER_CLIENTS;iClient++) {
				if (cm_aciClients[iClient].ci_bUsed == FALSE) {
					bFoundEmpty = TRUE;
					// we have an empty slot, so fill it for the client
					cm_aciClients[iClient].ci_adrAddress.adr_ulAddress = ppaConnectionRequest->pa_adrAddress.adr_ulAddress;
					cm_aciClients[iClient].ci_adrAddress.adr_uwPort = ppaConnectionRequest->pa_adrAddress.adr_uwPort;
					// generate the ID
					UWORD uwID = _pTimer->GetHighPrecisionTimer().tv_llValue&0x0FFF;
					if (uwID==0 || uwID==SLASHSLASH) {
						uwID+=1;
					}										
					cm_aciClients[iClient].ci_adrAddress.adr_uwID = (uwID<<4)+iClient;
					// form the connection response packet
					ppaConnectionRequest->pa_adrAddress.adr_uwID = SLASHSLASH;
					ppaConnectionRequest->pa_ubReliable = UDP_PACKET_RELIABLE | UDP_PACKET_RELIABLE_HEAD | UDP_PACKET_RELIABLE_TAIL | UDP_PACKET_CONNECT_RESPONSE;
					// return it to the client
					ppaConnectionRequest->WriteToPacket(&(cm_aciClients[iClient].ci_adrAddress.adr_uwID),sizeof(cm_aciClients[iClient].ci_adrAddress.adr_uwID),ppaConnectionRequest->pa_ubReliable,cm_ciBroadcast.ci_ulSequence++,ppaConnectionRequest->pa_adrAddress.adr_uwID,sizeof(cm_aciClients[iClient].ci_adrAddress.adr_uwID));
					cm_ciBroadcast.ci_pbOutputBuffer.AppendPacket(*ppaConnectionRequest,TRUE);
					cm_aciClients[iClient].ci_bUsed = TRUE;
					return;
				}
			}

			// if none found
			if (!bFoundEmpty) {
				// error
				ThrowF_t(TRANS("Server: Cannot accept new clients, all slots used!\n"));
			}
	
		}
	}


};


/*
 *  ---->>>>  SERVER  <<<<----
 */

/*
 *  Initialize server
 */
void CCommunicationInterface::Server_Init_t(void) 
{
  CTSingleLock slComm(&cm_csComm, TRUE);

  ASSERT(cci_bInitialized);
  ASSERT(!cci_bServerInitialized);
	


  // for each client
  for(INDEX iClient=0; iClient<SERVER_CLIENTS; iClient++) {
    // clear its status
    cm_aciClients[iClient].Clear();
		cm_aciClients[iClient].ci_pbOutputBuffer.pb_ppbsStats = &_pbsSend;
		cm_aciClients[iClient].ci_pbInputBuffer.pb_ppbsStats = &_pbsRecv;
  }

	// mark the server's instance of the local client as such
	cm_aciClients[SERVER_LOCAL_CLIENT].ci_bClientLocal = TRUE;
	cm_aciClients[SERVER_LOCAL_CLIENT].ci_bUsed = TRUE;

	// prepare the client part of the local client 
	cm_ciLocalClient.Clear();
	cm_ciLocalClient.ci_bUsed = TRUE;
	cm_ciLocalClient.ci_bClientLocal = TRUE;
	cm_ciLocalClient.ci_pbOutputBuffer.pb_ppbsStats = &_pbsSend;
	cm_ciLocalClient.ci_pbInputBuffer.pb_ppbsStats = &_pbsRecv;



  // mark that the server was initialized
  cci_bServerInitialized = TRUE;
};

/*
 *  Close server
 */
void CCommunicationInterface::Server_Close(void) 
{
  CTSingleLock slComm(&cm_csComm, TRUE);

  // close all clients
  for (INDEX iClient=0; iClient<SERVER_CLIENTS; iClient++) {
    cm_aciClients[iClient].Clear();
  }

  // mark that the server is uninitialized
  cci_bServerInitialized = FALSE;
};



/*
 *  Server clear client and prepare for new connection
 */
void CCommunicationInterface::Server_ClearClient(INDEX iClient)
{
  // synchronize access to communication data
  CTSingleLock slComm(&cm_csComm, TRUE);

  ASSERT(iClient>=0 && iClient<SERVER_CLIENTS);
  cm_aciClients[iClient].Clear();
};

BOOL CCommunicationInterface::Server_IsClientLocal(INDEX iClient)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  ASSERT(iClient>=0 && iClient<SERVER_CLIENTS);
  return iClient==SERVER_LOCAL_CLIENT;
};

BOOL CCommunicationInterface::Server_IsClientUsed(INDEX iClient)
{
  CTSingleLock slComm(&cm_csComm, TRUE);

  ASSERT(iClient>=0 && iClient<SERVER_CLIENTS);
  return cm_aciClients[iClient].ci_bUsed;
};

CTString CCommunicationInterface::Server_GetClientName(INDEX iClient)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  ASSERT(iClient>=0 && iClient<SERVER_CLIENTS);

  if (iClient==SERVER_LOCAL_CLIENT) {
    return TRANS("Local machine");
  }

  cm_aciClients[iClient].ci_strAddress = AddressToString(cm_aciClients[iClient].ci_adrAddress.adr_ulAddress);

  return cm_aciClients[iClient].ci_strAddress;
};

/*
 *  Server Send/Receive Reliable
 */
void CCommunicationInterface::Server_Send_Reliable(INDEX iClient, const void *pvSend, SLONG slSendSize)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  ASSERT(iClient>=0 && iClient<SERVER_CLIENTS);
  cm_aciClients[iClient].Send(pvSend, slSendSize,TRUE);
};

BOOL CCommunicationInterface::Server_Receive_Reliable(INDEX iClient, void *pvReceive, SLONG &slReceiveSize)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  ASSERT(iClient>=0 && iClient<SERVER_CLIENTS);
  return cm_aciClients[iClient].Receive(pvReceive, slReceiveSize,TRUE);
};

/*
 *  Server Send/Receive Unreliable
 */
void CCommunicationInterface::Server_Send_Unreliable(INDEX iClient, const void *pvSend, SLONG slSendSize)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  ASSERT(iClient>=0 && iClient<SERVER_CLIENTS);
  cm_aciClients[iClient].Send(pvSend, slSendSize,FALSE);
};

BOOL CCommunicationInterface::Server_Receive_Unreliable(INDEX iClient, void *pvReceive, SLONG &slReceiveSize)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  ASSERT(iClient>=0 && iClient<SERVER_CLIENTS);
  return cm_aciClients[iClient].Receive(pvReceive, slReceiveSize,FALSE);
};


BOOL CCommunicationInterface::Server_Update()
{

  CTSingleLock slComm(&cm_csComm, TRUE);
	CPacket *ppaPacket;
	CPacket *ppaPacketCopy;
	CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
	INDEX iClient;

	// transfer packets for the local client
	if (cm_ciLocalClient.ci_bUsed && cm_ciLocalClient.ci_pciOther != NULL) {
		cm_ciLocalClient.ExchangeBuffers();
	};

	cm_aciClients[0].UpdateOutputBuffers();

	// if not just playing single player
	if (cci_bServerInitialized) {
		Broadcast_Update_t();
		// for each client transfer packets from the output buffer to the master output buffer
		for (iClient=1; iClient<SERVER_CLIENTS; iClient++) {
			CClientInterface &ci = cm_aciClients[iClient];
			// if not connected
			if (!ci.ci_bUsed) {
				// skip it
				continue;
			}
			// update its buffers, if a reliable packet is overdue (has not been delivered too long)
			// disconnect the client
			if (ci.UpdateOutputBuffers() != FALSE) {
				// transfer packets ready to be sent out to the master output buffer
				while (ci.ci_pbOutputBuffer.pb_ulNumOfPackets > 0) {
					ppaPacket = ci.ci_pbOutputBuffer.PeekFirstPacket();
					if (ppaPacket->pa_tvSendWhen < tvNow) {
						ci.ci_pbOutputBuffer.RemoveFirstPacket(FALSE);
						if (ppaPacket->pa_ubReliable & UDP_PACKET_RELIABLE) {
							ppaPacketCopy = new CPacket;
							*ppaPacketCopy = *ppaPacket;
							ci.ci_pbWaitAckBuffer.AppendPacket(*ppaPacketCopy,FALSE);
						}
						cci_pbMasterOutput.AppendPacket(*ppaPacket,FALSE);
					} else {
						break;
					}
				}
			} else {
        CPrintF(TRANSV("Unable to deliver data to client '%s', disconnecting.\n"),(const char *) AddressToString(cm_aciClients[iClient].ci_adrAddress.adr_ulAddress));
        Server_ClearClient(iClient);
        _pNetwork->ga_srvServer.HandleClientDisconected(iClient);

			}
		}

		// update broadcast output buffers
		// update its buffers
		cm_ciBroadcast.UpdateOutputBuffers();
		// transfer packets ready to be sent out to the master output buffer
		while (cm_ciBroadcast.ci_pbOutputBuffer.pb_ulNumOfPackets > 0) {
			ppaPacket = cm_ciBroadcast.ci_pbOutputBuffer.PeekFirstPacket();
			if (ppaPacket->pa_tvSendWhen < tvNow) {
				cm_ciBroadcast.ci_pbOutputBuffer.RemoveFirstPacket(FALSE);
				cci_pbMasterOutput.AppendPacket(*ppaPacket,FALSE);
			} else {
				break;
			}
		}

		// send/receive packets over the TCP/IP stack
		UpdateMasterBuffers();

		// dispatch all packets from the master input buffer to the clients' input buffers
		while (cci_pbMasterInput.pb_ulNumOfPackets > 0) {
			BOOL bClientFound;
			ppaPacket = cci_pbMasterInput.GetFirstPacket();
			bClientFound = FALSE;
			if (ppaPacket->pa_adrAddress.adr_uwID==SLASHSLASH || ppaPacket->pa_adrAddress.adr_uwID==0) {
				cm_ciBroadcast.ci_pbInputBuffer.AppendPacket(*ppaPacket,FALSE);
				bClientFound = TRUE;
			} else {
				for (iClient=0; iClient<SERVER_CLIENTS; iClient++) {
					if (ppaPacket->pa_adrAddress.adr_uwID == cm_aciClients[iClient].ci_adrAddress.adr_uwID) {
						cm_aciClients[iClient].ci_pbInputBuffer.AppendPacket(*ppaPacket,FALSE);
						bClientFound = TRUE;
						break;
					}
				}
			}
			if (!bClientFound) {
				// warn about possible attack
				extern INDEX net_bReportMiscErrors;
				if (net_bReportMiscErrors) {
					CPrintF(TRANSV("WARNING: Invalid message from: %s\n"), (const char *) AddressToString(ppaPacket->pa_adrAddress.adr_ulAddress));
				}
			}
 		}

		for (iClient=1; iClient<SERVER_CLIENTS; iClient++) {
			cm_aciClients[iClient].UpdateInputBuffers();
		}

		
	}
	cm_aciClients[0].UpdateInputBuffers();
	cm_ciLocalClient.UpdateInputBuffers();
	cm_ciBroadcast.UpdateInputBuffersBroadcast();
	Broadcast_Update_t();

	return TRUE;
};


/*
 *  ---->>>>  CLIENT  <<<<----
 */

/*
 *  Initialize client
 */
void CCommunicationInterface::Client_Init_t(char* strServerName)
{
  CTSingleLock slComm(&cm_csComm, TRUE);

  ASSERT(cci_bInitialized);
  ASSERT(!cci_bClientInitialized);

  // retrieve server address from server name
  ULONG ulServerAddress = StringToAddress(strServerName);
  // if lookup failed
  if (ulServerAddress==INADDR_NONE) {
    ThrowF_t(TRANS("Host '%s' not found!\n"), strServerName);
  }

  // call client init with server address
  Client_Init_t(ulServerAddress);
};

void CCommunicationInterface::Client_Init_t(ULONG ulServerAddress)
{
  CTSingleLock slComm(&cm_csComm, TRUE);

  ASSERT(cci_bInitialized);
  ASSERT(!cci_bClientInitialized);

  cm_ciLocalClient.Clear();
	cm_ciLocalClient.ci_pbOutputBuffer.pb_ppbsStats = &_pbsSend;
	cm_ciLocalClient.ci_pbInputBuffer.pb_ppbsStats = &_pbsRecv;

  // if this computer is not the server
  if (!cci_bServerInitialized) {
    // open with connecting to remote server
    cm_ciLocalClient.ci_bClientLocal= FALSE;
    Client_OpenNet_t(ulServerAddress);

  // if this computer is server
  } else {
    // open local client
    cm_ciLocalClient.ci_bClientLocal = TRUE;
    Client_OpenLocal();
  }

  cci_bClientInitialized = TRUE;
};

/*
 *  Close client
 */
void CCommunicationInterface::Client_Close(void)
{
  CTSingleLock slComm(&cm_csComm, TRUE);

  ASSERT(cci_bInitialized);

	// dispatch remaining packets (keep trying for half a second - 10 attempts)
  for(TIME tmWait=0; tmWait<500;
    _pTimer->Sleep(NET_WAITMESSAGE_DELAY), tmWait+=NET_WAITMESSAGE_DELAY) {
    // if all packets are successfully sent, exit loop
		if  ((cm_ciLocalClient.ci_pbOutputBuffer.pb_ulNumOfPackets == 0) 
			&& (cm_ciLocalClient.ci_pbWaitAckBuffer.pb_ulNumOfPackets == 0)) {
				break;
			}
    if (Client_Update() == FALSE) {
			break;
		}
	}

  cm_ciLocalClient.Clear();

  cm_ciLocalClient.ci_bClientLocal= FALSE;
  cci_bClientInitialized = FALSE;
};


/*
 *  Open client local
 */
void CCommunicationInterface::Client_OpenLocal(void)
{
  CTSingleLock slComm(&cm_csComm, TRUE);

  CClientInterface &ci0 = cm_ciLocalClient;
  CClientInterface &ci1 = cm_aciClients[SERVER_LOCAL_CLIENT];
    
  ci0.ci_bUsed = TRUE;
  ci0.SetLocal(&ci1);
  ci1.ci_bUsed = TRUE;
  ci1.SetLocal(&ci0);
};


/*
 *  Open client remote
 */
void CCommunicationInterface::Client_OpenNet_t(ULONG ulServerAddress)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
	CPacket* ppaInfoPacket;
	CPacket* ppaReadPacket;
	UBYTE ubDummy=65;
	UBYTE ubReliable;

  // check for reconnection
  static ULONG ulLastServerAddress = (ULONG) -1;
  BOOL bReconnecting = ulServerAddress == ulLastServerAddress;
  ulLastServerAddress = ulServerAddress;

  const INDEX iRefresh = 100; // (in miliseconds)
  // determine connection timeout
  INDEX ctRetries = bReconnecting?(180*1000/iRefresh):3;

  // start waiting for server's response
  if (ctRetries>1) {
    SetProgressDescription(TRANS("waiting for server"));
    CallProgressHook_t(0.0f);
  }

	

	// form the connection request packet
	ppaInfoPacket = new CPacket;
	ubReliable = UDP_PACKET_RELIABLE | UDP_PACKET_RELIABLE_HEAD | UDP_PACKET_RELIABLE_TAIL | UDP_PACKET_CONNECT_REQUEST;
	ppaInfoPacket->pa_adrAddress.adr_ulAddress = ulServerAddress;
	ppaInfoPacket->pa_adrAddress.adr_uwPort = net_iPort;
	ppaInfoPacket->pa_ubRetryNumber = 0;
	ppaInfoPacket->WriteToPacket(&ubDummy,1,ubReliable,cm_ciLocalClient.ci_ulSequence++,SLASHSLASH,1);

	cm_ciLocalClient.ci_pbOutputBuffer.AppendPacket(*ppaInfoPacket,TRUE);

	// set client destination address to server address
	cm_ciLocalClient.ci_adrAddress.adr_ulAddress = ulServerAddress;
	cm_ciLocalClient.ci_adrAddress.adr_uwPort = net_iPort;
	
  // for each retry
  for(INDEX iRetry=0; iRetry<ctRetries; iRetry++) {
		// send/receive and juggle the buffers
		if (Client_Update() == FALSE) {
			break;
		}

		// if there is something in the input buffer
		if (cm_ciLocalClient.ci_pbReliableInputBuffer.pb_ulNumOfPackets > 0) {
			ppaReadPacket = cm_ciLocalClient.ci_pbReliableInputBuffer.GetFirstPacket();
			// and it is a connection confirmation
			// DG: replaced && with & as done everywhere else, hope this doesn't rely on being buggy.
			if (ppaReadPacket->pa_ubReliable &  UDP_PACKET_CONNECT_RESPONSE) {
				// the client has succedeed to connect, so read the uwID from the packet
				cm_ciLocalClient.ci_adrAddress.adr_ulAddress = ulServerAddress;
				cm_ciLocalClient.ci_adrAddress.adr_uwPort = net_iPort;
				cm_ciLocalClient.ci_adrAddress.adr_uwID = *((UWORD*) (ppaReadPacket->pa_pubPacketData + MAX_HEADER_SIZE));
				cm_ciLocalClient.ci_bUsed = TRUE;
				cm_ciLocalClient.ci_bClientLocal = FALSE;
				cm_ciLocalClient.ci_pciOther = NULL;

				cm_ciLocalClient.ci_pbReliableInputBuffer.RemoveConnectResponsePackets();

				delete ppaReadPacket;

				// finish waiting
				CallProgressHook_t(1.0f);		    
				return;
			}
		}

    _pTimer->Sleep(iRefresh);
    CallProgressHook_t(FLOAT(iRetry%10)/10);
	}
	
	cci_bBound = FALSE;
	ThrowF_t(TRANS("Client: Timeout receiving UDP port"));
};


/*
 *  Clear local client
 */

void CCommunicationInterface::Client_Clear(void)
{
  // synchronize access to communication data
  CTSingleLock slComm(&cm_csComm, TRUE);

  cm_ciLocalClient.Clear();
};

/*
 *  Client get status
 */
BOOL CCommunicationInterface::Client_IsConnected(void)
{
  // synchronize access to communication data
  CTSingleLock slComm(&cm_csComm, TRUE);

  return cm_ciLocalClient.ci_bUsed;
};

/*
 *  Client Send/Receive Reliable
 */
void CCommunicationInterface::Client_Send_Reliable(const void *pvSend, SLONG slSendSize)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  cm_ciLocalClient.Send(pvSend, slSendSize,TRUE);
};

BOOL CCommunicationInterface::Client_Receive_Reliable(void *pvReceive, SLONG &slReceiveSize)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  return cm_ciLocalClient.Receive(pvReceive, slReceiveSize,TRUE);
};

BOOL CCommunicationInterface::Client_Receive_Reliable(CTStream &strmReceive)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  return cm_ciLocalClient.Receive(strmReceive,TRUE);
};

void CCommunicationInterface::Client_PeekSize_Reliable(SLONG &slExpectedSize,SLONG &slReceivedSize)
{
  slExpectedSize = cm_ciLocalClient.GetExpectedReliableSize();
  slReceivedSize = cm_ciLocalClient.GetCurrentReliableSize();
}


/*
 *  Client Send/Receive Unreliable
 */
void CCommunicationInterface::Client_Send_Unreliable(const void *pvSend, SLONG slSendSize)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  cm_ciLocalClient.Send(pvSend, slSendSize,FALSE);
};

BOOL CCommunicationInterface::Client_Receive_Unreliable(void *pvReceive, SLONG &slReceiveSize)
{
  CTSingleLock slComm(&cm_csComm, TRUE);
  return cm_ciLocalClient.Receive(pvReceive, slReceiveSize,FALSE);
};



BOOL CCommunicationInterface::Client_Update(void)
{
	CTSingleLock slComm(&cm_csComm, TRUE);
	CPacket *ppaPacket;
	CPacket *ppaPacketCopy;
	CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();

	// update local client's output buffers
	if (cm_ciLocalClient.UpdateOutputBuffers() == FALSE) {
		return FALSE;
	}

	// if not playing on the server (i.e. connectet to a remote server)
	if (!cci_bServerInitialized) {
		// put all pending packets in the master output buffer
		while (cm_ciLocalClient.ci_pbOutputBuffer.pb_ulNumOfPackets > 0) {
			ppaPacket = cm_ciLocalClient.ci_pbOutputBuffer.PeekFirstPacket();
			if (ppaPacket->pa_tvSendWhen < tvNow) {
				cm_ciLocalClient.ci_pbOutputBuffer.RemoveFirstPacket(FALSE);
				if (ppaPacket->pa_ubReliable & UDP_PACKET_RELIABLE) {
					ppaPacketCopy = new CPacket;
					*ppaPacketCopy = *ppaPacket;
					cm_ciLocalClient.ci_pbWaitAckBuffer.AppendPacket(*ppaPacketCopy,FALSE);
				}
				cci_pbMasterOutput.AppendPacket(*ppaPacket,FALSE);

			} else {
				break;
			}
		}

		// update broadcast output buffers
		// update its buffers
		cm_ciBroadcast.UpdateOutputBuffers();
		// transfer packets ready to be sent out to the master output buffer
		while (cm_ciBroadcast.ci_pbOutputBuffer.pb_ulNumOfPackets > 0) {
			ppaPacket = cm_ciBroadcast.ci_pbOutputBuffer.PeekFirstPacket();
			if (ppaPacket->pa_tvSendWhen < tvNow) {
				cm_ciBroadcast.ci_pbOutputBuffer.RemoveFirstPacket(FALSE);
				cci_pbMasterOutput.AppendPacket(*ppaPacket,FALSE);
			} else {
				break;
			}
		}

		// send/receive packets over the TCP/IP stack
		UpdateMasterBuffers();

		// dispatch all packets from the master input buffer to the clients' input buffers
		while (cci_pbMasterInput.pb_ulNumOfPackets > 0) {
			BOOL bClientFound;
			ppaPacket = cci_pbMasterInput.GetFirstPacket();
			bClientFound = FALSE;

      // if the packet address is broadcast and it's an unreliable transfer, put it in the broadcast buffer
      if ((ppaPacket->pa_adrAddress.adr_uwID==SLASHSLASH || ppaPacket->pa_adrAddress.adr_uwID==0) && 
           ppaPacket->pa_ubReliable == UDP_PACKET_UNRELIABLE) {
        cm_ciBroadcast.ci_pbInputBuffer.AppendPacket(*ppaPacket,FALSE);
				bClientFound = TRUE;
      // if the packet is for this client, accept it
      } else if ((ppaPacket->pa_adrAddress.adr_uwID == cm_ciLocalClient.ci_adrAddress.adr_uwID) || 
				          ppaPacket->pa_adrAddress.adr_uwID==SLASHSLASH || ppaPacket->pa_adrAddress.adr_uwID==0) { 
				cm_ciLocalClient.ci_pbInputBuffer.AppendPacket(*ppaPacket,FALSE);
				bClientFound = TRUE;
			}
			if (!bClientFound) {
				// warn about possible attack
				extern INDEX net_bReportMiscErrors;
				if (net_bReportMiscErrors) {
					CPrintF(TRANSV("WARNING: Invalid message from: %s\n"), (const char *) AddressToString(ppaPacket->pa_adrAddress.adr_ulAddress));
				}
			}
 		}

	}

	cm_ciLocalClient.UpdateInputBuffers();
	cm_ciBroadcast.UpdateInputBuffersBroadcast();

	return TRUE;
};



// update master UDP socket and route its messages
void CCommunicationInterface::UpdateMasterBuffers() 
{

	UBYTE aub[MAX_PACKET_SIZE];
	CAddress adrIncomingAddress;
	SOCKADDR_IN sa;
	socklen_t size = sizeof(sa);
	SLONG slSizeReceived;
	SLONG slSizeSent;
	BOOL bSomethingDone;
	CPacket* ppaNewPacket;
	CTimerValue tvNow;

	if (cci_bBound) {
		// read from the socket while there is incoming data
		do {

			// initially, nothing is done
			bSomethingDone = FALSE;
			slSizeReceived = recvfrom(cci_hSocket,(char*)aub,MAX_PACKET_SIZE,0,(SOCKADDR *)&sa,&size);
			tvNow = _pTimer->GetHighPrecisionTimer();

			adrIncomingAddress.adr_ulAddress = ntohl(sa.sin_addr.s_addr);
			adrIncomingAddress.adr_uwPort = ntohs(sa.sin_port);

			//On error, report it to the console (if error is not a no data to read message)
			if (slSizeReceived == SOCKET_ERROR) {
				int iResult = WSAGetLastError();
				if (!isWouldBlockError(iResult)) {
					// report it
					if (iResult!=WSAECONNRESET || net_bReportICMPErrors) {
						CPrintF(TRANSV("Socket error during UDP receive. %s\n"), 
							(const char*)GetSocketError(iResult));
						return;
					}
				}

			// if block received
			} else {
				// if there is not at least one byte more in the packet than the header size
				if (slSizeReceived <= static_cast<SLONG>(MAX_HEADER_SIZE)) {
					// the packet is in error
          extern INDEX net_bReportMiscErrors;          
          if (net_bReportMiscErrors) {
					  CPrintF(TRANSV("WARNING: Bad UDP packet from '%s'\n"), (const char *) AddressToString(adrIncomingAddress.adr_ulAddress));
          }
					// there might be more to do
					bSomethingDone = TRUE;
				} else if (net_fDropPackets <= 0  || (FLOAT(rand())/(float)(RAND_MAX)) > net_fDropPackets) {
					// if no packet drop emulation (or the packet is not dropped), form the packet 
					// and add it to the end of the UDP Master's input buffer
					ppaNewPacket = new CPacket;
					ppaNewPacket->WriteToPacketRaw(aub,slSizeReceived);
					ppaNewPacket->pa_adrAddress.adr_ulAddress = adrIncomingAddress.adr_ulAddress;
					ppaNewPacket->pa_adrAddress.adr_uwPort = adrIncomingAddress.adr_uwPort;						

					if (net_bReportPackets == TRUE) {
						CPrintF("%lu: Received sequence: %d from ID: %d, reliable flag: %d\n",(ULONG) tvNow.GetMilliseconds(),ppaNewPacket->pa_ulSequence,ppaNewPacket->pa_adrAddress.adr_uwID,ppaNewPacket->pa_ubReliable);
					}

					cci_pbMasterInput.AppendPacket(*ppaNewPacket,FALSE);
					// there might be more to do
					bSomethingDone = TRUE;
				
				}
			}	

		} while (bSomethingDone);
	}

	// write from the output buffer to the socket
	while (cci_pbMasterOutput.pb_ulNumOfPackets > 0) {
		ppaNewPacket = cci_pbMasterOutput.PeekFirstPacket();

		sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(ppaNewPacket->pa_adrAddress.adr_ulAddress);
    sa.sin_port = htons(ppaNewPacket->pa_adrAddress.adr_uwPort);
		
    slSizeSent = sendto(cci_hSocket, (char*) ppaNewPacket->pa_pubPacketData, (int) ppaNewPacket->pa_slSize, 0, (SOCKADDR *)&sa, sizeof(sa));
    cci_bBound = TRUE;   // UDP socket that did a send is considered bound
		tvNow = _pTimer->GetHighPrecisionTimer();

    // if some error
    if (slSizeSent == SOCKET_ERROR) {
      int iResult = WSAGetLastError();
			// if output UDP buffer full, stop sending
			if (isWouldBlockError(iResult)) {
				return;
			// report it
			} else if (iResult!=WSAECONNRESET || net_bReportICMPErrors) {
        CPrintF(TRANSV("Socket error during UDP send. %s\n"), 
          (const char*)GetSocketError(iResult));
      }
			return;    

    } else if (slSizeSent < ppaNewPacket->pa_slSize) {
        //STUBBED("LOST OUTGOING PACKET DATA!");
        ASSERT(0);

    // if all sent ok
    } else {
			
			if (net_bReportPackets == TRUE)	{
				CPrintF("%lu: Sent sequence: %d to ID: %d, reliable flag: %d\n",(ULONG)tvNow.GetMilliseconds(),ppaNewPacket->pa_ulSequence,ppaNewPacket->pa_adrAddress.adr_uwID,ppaNewPacket->pa_ubReliable);
			}

			cci_pbMasterOutput.RemoveFirstPacket(TRUE);
      bSomethingDone=TRUE;
    }

	}




};


