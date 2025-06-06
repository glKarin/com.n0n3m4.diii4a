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

#include <Engine/Base/ThreadLocalStorage.h>  //rcg10242001
#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>
#include <Engine/Network/MessageDispatcher.h>
#include <Engine/Network/Network.h>
#include <Engine/Network/NetworkProfile.h>
#include <Engine/Network/NetworkMessage.h>
#include <Engine/Network/CommunicationInterface.h>
#include <Engine/Base/ErrorTable.h>

#include <Engine/Base/ListIterator.inl>

// define this to randomly drop messages (for debugging of packet-loss recovery)
//#define LOSEPACKETS_THRESHOLD (RAND_MAX/10)

extern INDEX net_bReportTraffic;
extern BOOL _bTempNetwork;

/////////////////////////////////////////////////////////////////////
// CMessageBuffer helper class+implementation

// class for holding received messages (thread locally)
class CMessageBuffer {
public:
  // size of message buffer for one message
  ULONG mb_ulMessageBufferSize;
  // pointer to message buffer for one message
  void *mb_pvMessageBuffer;

  void Allocate(void);
  void Free(void);
};

#if defined(PLATFORM_UNIX)
// the thread's local buffer
THREADLOCAL(CMessageBuffer, mbReceivedMessage, CMessageBuffer());
#else
// the thread's local buffer
static _declspec(thread) CMessageBuffer mbReceivedMessage = { 0, 0 };
#endif

void CMessageBuffer::Allocate(void)
{
  if (mb_ulMessageBufferSize==0) {
    ASSERT(mb_pvMessageBuffer == NULL);
    // allocate message buffer
    mb_ulMessageBufferSize = 16000;
    mb_pvMessageBuffer = AllocMemory(mb_ulMessageBufferSize);
  }
}

void CMessageBuffer::Free(void)
{
  // if message buffer is allocated
  if (mb_ulMessageBufferSize != 0) {
    ASSERT(mb_pvMessageBuffer != NULL);
    // free it
    FreeMemory(mb_pvMessageBuffer);
    mb_ulMessageBufferSize = 0;
    mb_pvMessageBuffer = NULL;
  }
}

/////////////////////////////////////////////////////////////////////
// CNetworkProvider implementation

/*
 * Default constructor.
 */
CNetworkProvider::CNetworkProvider(void)
{
}

/*
 * Create a human description of driver.
 */
const CTString &CNetworkProvider::GetDescription(void) const
{
  return np_Description;
}


/////////////////////////////////////////////////////////////////////
// CNetworkSession implementation

/*
 * Default constructor.
 */
CNetworkSession::CNetworkSession(void)
{
}
/* Construct a session for connecting to certain server. */
CNetworkSession::CNetworkSession(const CTString &strAddress)
{
  ns_strAddress = strAddress;
}
void CNetworkSession::Copy(const CNetworkSession &nsOriginal)
{
  ns_strAddress     = nsOriginal.ns_strAddress     ;
  ns_strSession     = nsOriginal.ns_strSession     ;
  ns_strWorld       = nsOriginal.ns_strWorld       ;
  ns_tmPing         = nsOriginal.ns_tmPing         ;
  ns_ctPlayers      = nsOriginal.ns_ctPlayers      ;
  ns_ctMaxPlayers   = nsOriginal.ns_ctMaxPlayers   ;
  ns_strGameType    = nsOriginal.ns_strGameType    ;
  ns_strMod         = nsOriginal.ns_strMod         ;
  ns_strVer         = nsOriginal.ns_strVer         ;
}

/////////////////////////////////////////////////////////////////////
// CMessageDispatcher -- construction/destruction

/*
 * Default constructor.
 */
CMessageDispatcher::CMessageDispatcher(void) {
  if (!_bTempNetwork) {
    _cmiComm.Init();
  }
  // enumerate network providers
  EnumNetworkProviders_startup(md_lhProviders);
}

/*
 * Destructor.
 */
CMessageDispatcher::~CMessageDispatcher(void)
{
  if (!_bTempNetwork) {
    _cmiComm.Close();
  }
  // destroy the list of network providers
  FORDELETELIST(CNetworkProvider, np_Node, md_lhProviders, litProviders) {
    delete &*litProviders;
  }
}

/*
 * Initialize for a given game.
 */
void CMessageDispatcher::Init(const CTString &strGameID)
{
  md_strGameID = strGameID;
}

/////////////////////////////////////////////////////////////////////
// CMessageDispatcher -- network provider management

/*
 * Enumerate all providers at startup (later enumeration just copies this list).
 */
void CMessageDispatcher::EnumNetworkProviders_startup(CListHead &lh)
{
  // create local connection provider
  CNetworkProvider *pnpLocal = new CNetworkProvider;
  pnpLocal->np_Description = "Local";
  lh.AddTail(pnpLocal->np_Node);
  // create TCP/IP connection provider
  CNetworkProvider *pnpTCP = new CNetworkProvider;
  pnpTCP->np_Description = "TCP/IP Server";
  lh.AddTail(pnpTCP->np_Node);

  CNetworkProvider *pnpTCPCl = new CNetworkProvider;
  pnpTCPCl->np_Description = "TCP/IP Client";
  lh.AddTail(pnpTCPCl->np_Node);
}

/*
 * Enumerate all providers.
 */
void CMessageDispatcher::EnumNetworkProviders(CListHead &lh)
{
  // for each provider enumerated at startup
  FOREACHINLIST(CNetworkProvider, np_Node, md_lhProviders, litProvider) {
    // create a copy
    CNetworkProvider *pnpNew = new CNetworkProvider(*litProvider);
    // add the copy to the list
    lh.AddTail(pnpNew->np_Node);
  }
}

/*
 * Start using a service provider.
 */
void CMessageDispatcher::StartProvider_t(const CNetworkProvider &npProvider)
{
  if (npProvider.np_Description=="Local") {
    _cmiComm.PrepareForUse(FALSE, FALSE);
  } else if (npProvider.np_Description=="TCP/IP Server") {
    _cmiComm.PrepareForUse(TRUE, FALSE);
  } else {
    _cmiComm.PrepareForUse(TRUE, TRUE);
  }
}

/*
 * Stop using current service provider.
 */
void CMessageDispatcher::StopProvider(void)
{
  _cmiComm.Unprepare();
}

/////////////////////////////////////////////////////////////////////
// CMessageDispatcher -- network message management
static void UpdateSentMessageStats(const CNetworkMessage &nmMessage)
{
  // increment profile counters
  _pfNetworkProfile.IncrementCounter(CNetworkProfile::PCI_MESSAGESSENT);
  _pfNetworkProfile.IncrementCounter(CNetworkProfile::PCI_BYTESSENT, nmMessage.nm_slSize);
  switch (nmMessage.GetType()) {
  case MSG_GAMESTREAMBLOCKS:
    _pfNetworkProfile.IncrementCounter(CNetworkProfile::PCI_GAMESTREAM_BYTES_SENT, nmMessage.nm_slSize);
    break;
  case MSG_ACTION:
    _pfNetworkProfile.IncrementCounter(CNetworkProfile::PCI_ACTION_BYTES_SENT, nmMessage.nm_slSize);
    break;
  }
  if (net_bReportTraffic) {
    CPrintF("Sent: %d\n", nmMessage.nm_slSize);
  }
}
static void UpdateSentStreamStats(SLONG slSize)
{
  if (net_bReportTraffic) {
    CPrintF("STREAM Sent: %d\n", slSize);
  }
}
static void UpdateReceivedMessageStats(const CNetworkMessage &nmMessage)
{
  // increment profile counters
  _pfNetworkProfile.IncrementCounter(CNetworkProfile::PCI_MESSAGESRECEIVED);
  _pfNetworkProfile.IncrementCounter(CNetworkProfile::PCI_BYTESRECEIVED, nmMessage.nm_slSize);
  switch (nmMessage.GetType()) {
  case MSG_GAMESTREAMBLOCKS:
    _pfNetworkProfile.IncrementCounter(CNetworkProfile::PCI_GAMESTREAM_BYTES_RECEIVED, nmMessage.nm_slSize);
    break;
  case MSG_ACTION:
    _pfNetworkProfile.IncrementCounter(CNetworkProfile::PCI_ACTION_BYTES_RECEIVED, nmMessage.nm_slSize);
    break;
  }
  if (net_bReportTraffic) {
    CPrintF("Rcvd: %d\n", nmMessage.nm_slSize);
  }
}
static void UpdateReceivedStreamStats(SLONG slSize)
{
  if (net_bReportTraffic) {
    CPrintF("STREAM Rcvd: %d\n", slSize);
  }
}

/* Send a message from server to client. */
void CMessageDispatcher::SendToClient(INDEX iClient, const CNetworkMessage &nmMessage)
{
// if testing for packet-loss recovery
#ifdef LOSEPACKETS_THRESHOLD
  // every once a while
  if (rand()<LOSEPACKETS_THRESHOLD) {
    // don't send it
    return;
  }
#endif

  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_SENDMESSAGE);
  // send the message
  _cmiComm.Server_Send_Unreliable(iClient, (void*)nmMessage.nm_pubMessage, nmMessage.nm_slSize);
	
  UpdateSentMessageStats(nmMessage);
  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SENDMESSAGE);
}
void CMessageDispatcher::SendToClientReliable(INDEX iClient, const CNetworkMessage &nmMessage)
{
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_SENDMESSAGE);
	
  // send the message
  _cmiComm.Server_Send_Reliable(iClient, (void*)nmMessage.nm_pubMessage, nmMessage.nm_slSize);
  UpdateSentMessageStats(nmMessage);
  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SENDMESSAGE);
}
void CMessageDispatcher::SendToClientReliable(INDEX iClient, CTMemoryStream &strmMessage)
{
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_SENDMESSAGE);

  // get size and buffer from the stream
  void *pvBuffer;
  SLONG slSize;
  strmMessage.LockBuffer(&pvBuffer, &slSize);
  // send the message
  _cmiComm.Server_Send_Reliable(iClient, pvBuffer, slSize);
  strmMessage.UnlockBuffer();
  UpdateSentStreamStats(slSize);
  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SENDMESSAGE);
}

/* Send a message from client to server. */
void CMessageDispatcher::SendToServer(const CNetworkMessage &nmMessage)
{
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_SENDMESSAGE);
  // send the message
  _cmiComm.Client_Send_Unreliable((void*)nmMessage.nm_pubMessage, nmMessage.nm_slSize);
	UpdateSentMessageStats(nmMessage);
  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SENDMESSAGE);
}

void CMessageDispatcher::SendToServerReliable(const CNetworkMessage &nmMessage)
{
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_SENDMESSAGE);
  // send the message
  _cmiComm.Client_Send_Reliable((void*)nmMessage.nm_pubMessage, nmMessage.nm_slSize);
  UpdateSentMessageStats(nmMessage);
  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SENDMESSAGE);
}

/* Receive next message from server to client. */
BOOL CMessageDispatcher::ReceiveFromServer(CNetworkMessage &nmMessage)
{
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_RECEIVEMESSAGE);
  // receive message in static buffer
  nmMessage.nm_slSize = nmMessage.nm_slMaxSize;
  BOOL bReceived = _cmiComm.Client_Receive_Unreliable(
    (void*)nmMessage.nm_pubMessage, nmMessage.nm_slSize);

  // if there is message
  if (bReceived) {
    // init the message structure
    nmMessage.nm_pubPointer = nmMessage.nm_pubMessage;
    nmMessage.nm_iBit = 0;
    UBYTE ubType;
    nmMessage.Read(&ubType, sizeof(ubType));
    nmMessage.nm_mtType = (MESSAGETYPE)ubType;

    UpdateReceivedMessageStats(nmMessage);
  }
  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_RECEIVEMESSAGE);
  return bReceived;
}

BOOL CMessageDispatcher::ReceiveFromServerReliable(CNetworkMessage &nmMessage)
{
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_RECEIVEMESSAGE);
  // receive message in static buffer
  nmMessage.nm_slSize = nmMessage.nm_slMaxSize;
  BOOL bReceived = _cmiComm.Client_Receive_Reliable(
    (void*)nmMessage.nm_pubMessage, nmMessage.nm_slSize);

  // if there is message
  if (bReceived) {
    // init the message structure
    nmMessage.nm_pubPointer = nmMessage.nm_pubMessage;
    nmMessage.nm_iBit = 0;
    UBYTE ubType;
    nmMessage.Read(&ubType, sizeof(ubType));
    nmMessage.nm_mtType = (MESSAGETYPE)ubType;
    UpdateReceivedMessageStats(nmMessage);

  }
  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_RECEIVEMESSAGE);
  return bReceived;
}

BOOL CMessageDispatcher::ReceiveFromServerReliable(CTMemoryStream &strmMessage)
{
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_RECEIVEMESSAGE);
  // receive message in stream
  BOOL bReceived = _cmiComm.Client_Receive_Reliable(strmMessage);
	
  // if there is message
  if (bReceived) {
    try {
      UpdateReceivedStreamStats(strmMessage.GetPos_t());
    } catch (const char *) {
    }
  }
  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_RECEIVEMESSAGE);
  return bReceived;
}

/* Receive next message from client to server. */
BOOL CMessageDispatcher::ReceiveFromClient(INDEX iClient, CNetworkMessage &nmMessage)
{
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_RECEIVEMESSAGE);
  // receive message in static buffer
  nmMessage.nm_slSize = nmMessage.nm_slMaxSize;
  BOOL bReceived = _cmiComm.Server_Receive_Unreliable(iClient,
    (void*)nmMessage.nm_pubMessage, nmMessage.nm_slSize);

  // if there is message
  if (bReceived) {
    // init the message structure
    nmMessage.nm_pubPointer = nmMessage.nm_pubMessage;
    nmMessage.nm_iBit = 0;
    UBYTE ubType;
    nmMessage.Read(&ubType, sizeof(ubType));
    nmMessage.nm_mtType = (MESSAGETYPE)ubType;

    UpdateReceivedMessageStats(nmMessage);
  }
  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_RECEIVEMESSAGE);
  return bReceived;
}

BOOL CMessageDispatcher::ReceiveFromClientReliable(INDEX iClient, CNetworkMessage &nmMessage)
{
//  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_RECEIVEMESSAGE);  // profile this!!!!
  // receive message in static buffer
  nmMessage.nm_slSize = nmMessage.nm_slMaxSize;
  BOOL bReceived = _cmiComm.Server_Receive_Reliable(iClient,
    (void*)nmMessage.nm_pubMessage, nmMessage.nm_slSize);

  // if there is a message
  if (bReceived) {
    // init the message structure
    nmMessage.nm_pubPointer = nmMessage.nm_pubMessage;
    nmMessage.nm_iBit = 0;
    UBYTE ubType;
    nmMessage.Read(&ubType, sizeof(ubType));
    nmMessage.nm_mtType = (MESSAGETYPE)ubType;

    UpdateReceivedMessageStats(nmMessage);
  }
//  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_RECEIVEMESSAGE);
  return bReceived;
}

#define SLASHSLASH  0x2F2F   // looks like "//" in ASCII.

/* Send/receive broadcast messages. */
void CMessageDispatcher::SendBroadcast(const CNetworkMessage &nmMessage, ULONG ulAddr, UWORD uwPort)
{
  CAddress adrDestination;
  adrDestination.adr_ulAddress = ulAddr;
  adrDestination.adr_uwPort = uwPort;
  adrDestination.adr_uwID = SLASHSLASH;
  // send the message
  _cmiComm.Broadcast_Send((void*)nmMessage.nm_pubMessage, nmMessage.nm_slSize,adrDestination);

  UpdateSentMessageStats(nmMessage);
}

BOOL CMessageDispatcher::ReceiveBroadcast(CNetworkMessage &nmMessage, ULONG &ulAddr, UWORD &uwPort)
{
  CAddress adrSource = {0,0,0};
  // receive message in static buffer
  nmMessage.nm_slSize = nmMessage.nm_slMaxSize;
  BOOL bReceived = _cmiComm.Broadcast_Receive(
    (void*)nmMessage.nm_pubMessage, nmMessage.nm_slSize,adrSource);

  // if there is message
  if (bReceived) {
    // init the message structure
    nmMessage.nm_pubPointer = nmMessage.nm_pubMessage;
    nmMessage.nm_iBit = 0;
    UBYTE ubType;
    nmMessage.Read(&ubType, sizeof(ubType));
    nmMessage.nm_mtType = (MESSAGETYPE)ubType;
    // read address
    ulAddr = adrSource.adr_ulAddress;
    uwPort = adrSource.adr_uwPort;
  }
//  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_RECEIVEMESSAGE);
  return bReceived;
}
