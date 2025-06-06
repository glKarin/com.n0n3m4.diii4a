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

#include <Engine/Build.h>
#include <Engine/Network/Network.h>
#include <Engine/Network/Server.h>
#include <Engine/Network/SessionState.h>
#include <Engine/Network/PlayerSource.h>
#include <Engine/Network/PlayerBuffer.h>
#include <Engine/Network/PlayerTarget.h>
#include <Engine/Network/NetworkProfile.h>
#include <Engine/Network/ClientInterface.h>
#include <Engine/Network/CommunicationInterface.h>
#include <Engine/Network/Compression.h>
#include <Engine/Network/Diff.h>
#include <Engine/Network/CPacket.h>
#include <Engine/World/WorldSettings.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/Shell.h>
#include <Engine/Math/Functions.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/Base/CRC.h>
#include <Engine/Base/ErrorTable.h>
#include <Engine/GameAgent/GameAgent.h>

#include <Engine/Templates/StaticArray.cpp>

extern INDEX ser_iSyncCheckBuffer;
extern FLOAT net_tmDisconnectTimeout;
extern BOOL con_bCapture;
extern CTString con_strCapture;
extern CTString ser_strIPMask;
extern CTString ser_strNameMask;
extern INDEX ser_bInverseBanning;
extern BOOL MatchesBanMask(const CTString &strString, const CTString &strMask);
extern CClientInterface cm_aciClients[SERVER_CLIENTS];

CSessionSocket::CSessionSocket(void)
{
  sso_bActive = FALSE;
  sso_bVIP = FALSE;
  sso_bSendStream = FALSE;
  sso_iDisconnectedState = 0;
  sso_iLastSentSequence  = -1;
  sso_ctBadSyncs = 0;
  sso_tvLastMessageSent.Clear();
  sso_tvLastPingSent.Clear();
}
CSessionSocket::~CSessionSocket(void)
{
  sso_bActive = FALSE;
  sso_bVIP = FALSE;
  sso_bSendStream = FALSE;
  sso_iLastSentSequence  = -1;
  sso_ctBadSyncs = 0;
  sso_tvLastMessageSent.Clear();
  sso_tvLastPingSent.Clear();
}
void CSessionSocket::Clear(void)
{
  sso_bActive = FALSE;
  sso_bVIP = FALSE;
  sso_bSendStream = FALSE;
  sso_tvMessageReceived.Clear();
  sso_tmLastSyncReceived = -1.0f;
  sso_iLastSentSequence  = -1;
  sso_tvLastMessageSent.Clear();
  sso_tvLastPingSent.Clear();
  sso_nsBuffer.Clear();
  sso_iDisconnectedState = 0;
  sso_ctBadSyncs = 0;
  sso_sspParams.Clear();
}

void CSessionSocket::Activate(void)
{
#if DEBUG_SYNCSTREAMDUMPING
  ClearDumpStream();
#endif

  ASSERT(!sso_bActive);
  sso_bActive = TRUE;
  sso_bVIP = FALSE;
  sso_bSendStream = FALSE;
  sso_tvMessageReceived.Clear();
  sso_tmLastSyncReceived = -1.0f;
  sso_iLastSentSequence  = -1;
  sso_tvLastMessageSent.Clear();
  sso_tvLastPingSent.Clear();
  sso_iDisconnectedState = 0;
  sso_ctBadSyncs = 0;
  sso_sspParams.Clear();
//  sso_nsBuffer.Clear();
}

void CSessionSocket::Deactivate(void)
{
  sso_iDisconnectedState = 0;
  sso_iLastSentSequence  = -1;
  sso_tvLastMessageSent.Clear();
  sso_tvLastPingSent.Clear();
  sso_ctBadSyncs = 0;
  sso_bActive = FALSE;
  sso_nsBuffer.Clear();
  sso_sspParams.Clear();
}
BOOL CSessionSocket::IsActive(void)
{
  return sso_bActive;
}

extern INDEX cli_iBufferActions;
extern INDEX cli_iMaxBPS;
extern INDEX cli_iMinBPS;

CSessionSocketParams::CSessionSocketParams(void)
{
  Clear();
}

void CSessionSocketParams::Clear(void)
{
  ssp_iBufferActions = 2;
  ssp_iMaxBPS = 4000;
  ssp_iMinBPS = 1000;
}

static void ClampParams(void)
{
  cli_iBufferActions = Clamp(cli_iBufferActions, INDEX(1), INDEX(20));
  cli_iMaxBPS = Clamp(cli_iMaxBPS, INDEX(100), INDEX(1000000));
  cli_iMinBPS = Clamp(cli_iMinBPS, INDEX(100), INDEX(1000000));
}

// check if up to date with current params
BOOL CSessionSocketParams::IsUpToDate(void)
{
  ClampParams();
  return 
    ssp_iBufferActions == cli_iBufferActions &&
    ssp_iMaxBPS == cli_iMaxBPS &&
    ssp_iMinBPS == cli_iMinBPS;
}

// update
void CSessionSocketParams::Update(void)
{
  ClampParams();
  ssp_iBufferActions = cli_iBufferActions;
  ssp_iMaxBPS = cli_iMaxBPS;
  ssp_iMinBPS = cli_iMinBPS;
}

// message operations
CNetworkMessage &operator<<(CNetworkMessage &nm, CSessionSocketParams &ssp)
{
  nm<<ssp.ssp_iBufferActions<<ssp.ssp_iMaxBPS<<ssp.ssp_iMinBPS;
  return nm;
}
CNetworkMessage &operator>>(CNetworkMessage &nm, CSessionSocketParams &ssp)
{
  nm>>ssp.ssp_iBufferActions>>ssp.ssp_iMaxBPS>>ssp.ssp_iMinBPS;
  return nm;
}

/*
 * Constructor.
 */
CServer::CServer(void)
{
  srv_bActive = FALSE;

  srv_assoSessions.New(NET_MAXGAMECOMPUTERS);
  srv_aplbPlayers.New(NET_MAXGAMEPLAYERS);
  // initialize player indices
  INDEX iPlayer = 0;
  FOREACHINSTATICARRAY(srv_aplbPlayers, CPlayerBuffer, itplb) {
    itplb->plb_Index = iPlayer;
    iPlayer++;
  }
}

/*
 * Destructor.
 */
CServer::~CServer()
{
  srv_bActive = FALSE;
}

/*
 * Clear the object.
 */
void CServer::Stop(void)
{
  // stop gameagent
  GameAgent_ServerEnd();

  // tell all clients to disconnect
  INDEX ctClients = srv_assoSessions.sa_Count;
  INDEX iClient;
  for (iClient=0;iClient<ctClients;iClient++) {
    if (srv_assoSessions[iClient].sso_bActive == TRUE) {
      SendDisconnectMessage(iClient,"Server stopped.");
    }
  }
  
  // run a few updates, so the message gets sent
  for (int ctUpdates=0;ctUpdates<10;ctUpdates++) {
    BOOL bFound = FALSE;
    for (iClient=0;iClient<ctClients;iClient++) {
      if (srv_assoSessions[iClient].sso_bActive == TRUE) {
        bFound = TRUE;
        break;
      }
    }
    if (bFound == FALSE) {
      break;
    } else {
      _cmiComm.Server_Update();
      _pTimer->Sleep(100);
    }
  }


  // stop network driver server
  _cmiComm.Server_Close();

  // clear all session
  srv_assoSessions.Clear();
  srv_assoSessions.New(NET_MAXGAMECOMPUTERS);
  // clear list of players
  srv_aplbPlayers.Clear();
  srv_aplbPlayers.New(NET_MAXGAMEPLAYERS);
  // initialize player indices
  INDEX iPlayer = 0;
  {FOREACHINSTATICARRAY(srv_aplbPlayers, CPlayerBuffer, itplb) {
    itplb->plb_Index = iPlayer;
    iPlayer++;
  }}

  // init buffer for sync checks
  srv_ascChecks.Clear();

  srv_bActive = FALSE;
};

/*
 * Initialize server and start message handlers.
 */
void CServer::Start_t(void)
{
  // init buffer for sync checks
  srv_ascChecks.Clear();

  // set up structures
  srv_tmLastProcessedTick = 0.0f;
  srv_iLastProcessedSequence = -1; // -1 so that next one will be 0
  srv_bActive = TRUE;
  srv_bPause = FALSE;
  srv_bGameFinished = FALSE;
  srv_fServerStep = 0.0f;

  // init network driver server
  _cmiComm.Server_Init_t();

  // init gameagent
  if (_cmiComm.IsNetworkEnabled()) {
    GameAgent_ServerInit();
  }
}

/*
 * Send disconnect message to some node.
 */
void CServer::SendDisconnectMessage(INDEX iClient, const char *strExplanation, BOOL bStream)
{
  CSessionSocket &sso = srv_assoSessions[iClient];

  if (!bStream) {
    CNetworkMessage nmDisconnect(MSG_INF_DISCONNECTED);
    // compose message
    nmDisconnect<<CTString(strExplanation);
    // send it
    _pNetwork->SendToClientReliable(iClient, nmDisconnect);
  } else {
    CTMemoryStream strmDisconnect;
    strmDisconnect<<INDEX(MSG_INF_DISCONNECTED);
    strmDisconnect<<CTString(strExplanation);

    // send the stream to the remote session state
    _pNetwork->SendToClientReliable(iClient, strmDisconnect);
  }
  // report that it has gone away
  CPrintF(TRANSV("Client '%s' ordered to disconnect: %s\n"), 
    (const char *) _cmiComm.Server_GetClientName(iClient), strExplanation);
  // if not disconnected before
  if (sso.sso_iDisconnectedState==0) {
    // mark the disconnection
    sso.sso_iDisconnectedState = 1;
  // if the client was already kicked before, but is still hanging here
  } else {
    // force the disconnection
    CPrintF(TRANSV("Forcing client '%s' to disconnect\n"), 
      (const char *) _cmiComm.Server_GetClientName(iClient));
    sso.sso_iDisconnectedState = 2;
  }
}

// add a new sync check to buffer
void CServer::AddSyncCheck(const CSyncCheck &sc)
{
  ser_iSyncCheckBuffer = ClampDn(ser_iSyncCheckBuffer, INDEX(1));
  if (srv_ascChecks.Count()!=ser_iSyncCheckBuffer) {
    srv_ascChecks.Clear();
    srv_ascChecks.New(ser_iSyncCheckBuffer);
  }
  // find the oldest one
  INDEX iOldest = 0;
  for (INDEX i=1; i<srv_ascChecks.Count(); i++) {
    if (srv_ascChecks[i].sc_tmTick<srv_ascChecks[iOldest].sc_tmTick) {
      iOldest=i;
    }
  }

  // overwrite it
  srv_ascChecks[iOldest] = sc;
}

// try to find a sync check for given time in the buffer (-1==too old, 0==found, 1==toonew)
INDEX CServer::FindSyncCheck(TIME tmTick, CSyncCheck &sc)
{
  BOOL bHasEarlier = FALSE;
  BOOL bHasLater = FALSE;
  for (INDEX i=0; i<srv_ascChecks.Count(); i++) {
    TIME tmInTable = srv_ascChecks[i].sc_tmTick;
    if (tmInTable==tmTick) {
      sc = srv_ascChecks[i];
      return 0;
    } else if (tmInTable<tmTick) {
      bHasEarlier = TRUE;
    } else if (tmInTable>tmTick) {
      bHasLater = TRUE;
    }
  }

  if (!bHasEarlier) {
    ASSERT(bHasLater);
    return -1;
  } else if (!bHasLater) {
    ASSERT(bHasEarlier);
    return +1;
  } else {
    ASSERT(FALSE);  // cannot have both earlier and later and not be found
    return +1;
  }
}

/* Send one regular batch of sequences to a client. */
void CServer::SendGameStreamBlocks(INDEX iClient)
{
  // get corresponding session socket
  CSessionSocket &sso = srv_assoSessions[iClient];

  // gather needed data to decide what to send
  INDEX iLastSent = sso.sso_iLastSentSequence;
  INDEX ctMinBytes = sso.sso_sspParams.ssp_iMinBPS/20;
  INDEX ctMaxBytes = sso.sso_sspParams.ssp_iMaxBPS/20;
  // make sure outgoing message doesn't overflow UDP size
  ctMinBytes = Clamp(ctMinBytes, (INDEX)0, (INDEX)1000);
  ctMaxBytes = Clamp(ctMaxBytes, (INDEX)0, (INDEX)1000);
  // limit the clients BPS by server's local settings
  extern INDEX ser_iMaxAllowedBPS;
  ctMinBytes = ClampUp(ctMinBytes, (INDEX) ( ser_iMaxAllowedBPS/20L - MAX_HEADER_SIZE));
  ctMaxBytes = ClampUp(ctMaxBytes, (INDEX) (ser_iMaxAllowedBPS/20L - MAX_HEADER_SIZE));

  // prevent server/singleplayer from flooding itself
  extern INDEX cli_bPredictIfServer;
  if (iClient==0 && !cli_bPredictIfServer) {
    ctMinBytes = 0;
    ctMaxBytes = (INDEX) 1E6;
  }

//  CPrintF("Send%d(%d, %d, %d): ", iClient, iLastSent, ctMinBytes, ctMaxBytes);

  // start after last sequence that was sent and go upwards
  INDEX iSequence = iLastSent+1;
  INDEX iStep = +1;
//  CPrintF("last=%d -- ", iLastSent);

  // initialize the message that is to be sent
  CNetworkMessage nmGameStreamBlocks(MSG_GAMESTREAMBLOCKS);
  // get one message for last compressed message of valid size
  CNetworkMessage nmPackedBlocks(MSG_GAMESTREAMBLOCKS);
  CNetworkMessage nmPackedBlocksNew(MSG_GAMESTREAMBLOCKS);

  // repeat for max 100 sequences
  INDEX iBlocksOk = 0;
  INDEX iMaxSent = -1;
  for(INDEX i=0; i<100; i++) {
    if (iStep<0 && iBlocksOk>=3) {
//      break;
    }
    // get the stream block with current sequence
//    CPrintF("%d: ", iSequence);
    CNetworkStreamBlock *pnsbBlock;
    CNetworkStream::Result res = sso.sso_nsBuffer.GetBlockBySequence(iSequence, pnsbBlock);
    // if it is not found
    if (res!=CNetworkStream::R_OK) {
      // if going upward
      if (iStep>0 ) {
//        // if this block is missing
//        && res==CNetworkStream::R_BLOCKMISSING
        // if none sent so far
        if (iBlocksOk<=0) {
          // give up
//          CPrintF("giving up\n");
          break; 
        }
//        CPrintF("rewind ", iSequence);
        // rewind and continue downward
        if (iSequence == iLastSent+1) {
          iSequence = iLastSent-1;
        } else {
          iSequence = iLastSent;
        }
        iStep = -1;
        // retry
        continue;
      // otherwise
      } else {
        // stop adding more blocks
        break;
      }
    }
    // if uncompressed message would overflow
    if (nmGameStreamBlocks.nm_slSize+pnsbBlock->nm_slSize+32>MAX_NETWORKMESSAGE_SIZE) {
//      CPrintF("overflow ");
      break;
    }

    // add this block to the message and pack it
    pnsbBlock->WriteToMessage(nmGameStreamBlocks);
    nmPackedBlocksNew.Reinit();
    nmGameStreamBlocks.PackDefault(nmPackedBlocksNew);
    // if some blocks written already and the batch is too large
    if (iBlocksOk>0) {
      if ((iStep>0 && nmPackedBlocksNew.nm_slSize>=ctMaxBytes) ||
          (iStep<0 && nmPackedBlocksNew.nm_slSize>=ctMinBytes) ) {
        // stop
//        CPrintF("toomuch ");
        break;
      }
    }
    // use new pack
//    CPrintF("added ");
    nmPackedBlocks = nmPackedBlocksNew;
    iMaxSent = Max(iMaxSent, iSequence);
    iSequence+= iStep;
    iBlocksOk++;
  }

  // if no blocks to write
  if (iBlocksOk<=0) {
    // if not sent anything for some time
    CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
    extern FLOAT ser_tmKeepAlive;
    if ((tvNow-sso.sso_tvLastMessageSent).GetSeconds()>ser_tmKeepAlive) {
      // send keepalive
      CNetworkMessage nmKeepalive(MSG_KEEPALIVE);
      _pNetwork->SendToClient(iClient, nmKeepalive);
      sso.sso_tvLastMessageSent = tvNow;
    }
    //    CPrintF("nothing\n");
    return;
  }

  // send the message to the client
//  CPrintF("sent: %d=%dB\n", iBlocksOk, nmPackedBlocks.nm_slSize);
  _pNetwork->SendToClient(iClient, nmPackedBlocks);
  sso.sso_iLastSentSequence = Max(sso.sso_iLastSentSequence, iMaxSent);
  sso.sso_tvLastMessageSent = _pTimer->GetHighPrecisionTimer();

  // remove the block(s) that fall out of the buffer
  extern INDEX ser_iRememberBehind;
  sso.sso_nsBuffer.RemoveOlderBlocksBySequence(srv_iLastProcessedSequence-ser_iRememberBehind);


  // if haven't sent pings for some time
  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
  extern FLOAT ser_tmPingUpdate;
  if ((tvNow-sso.sso_tvLastPingSent).GetSeconds()>ser_tmPingUpdate) {
    // send ping info
    CNetworkMessage nmPings(MSG_INF_PINGS);
    for(INDEX i=0; i<NET_MAXGAMEPLAYERS; i++) {
      CPlayerBuffer &plb = srv_aplbPlayers[i];
      if (plb.IsActive()) {
        BOOL b = 1;
        nmPings.WriteBits(&b, 1);
        nmPings.WriteBits(&plb.plb_iPing, 10);
      } else {
        BOOL b = 0;
        nmPings.WriteBits(&b, 1);
      }
    }
    _pNetwork->SendToClient(iClient, nmPings);
    sso.sso_tvLastPingSent = tvNow;
  }
}

/* Resend a batch of game stream blocks to a client. */
void CServer::ResendGameStreamBlocks(INDEX iClient, INDEX iSequence0, INDEX ctSequences)
{
  extern INDEX net_bReportMiscErrors;
  if (net_bReportMiscErrors) {
    CPrintF(TRANSV("Server: Resending sequences %d-%d(%d) to '%s'..."), 
      iSequence0, iSequence0+ctSequences-1, ctSequences, (const char *) _cmiComm.Server_GetClientName(iClient));
  }

  // get corresponding session socket
  CSessionSocket &sso = srv_assoSessions[iClient];

  // create a package message
  CNetworkMessage nmGameStreamBlocks(MSG_GAMESTREAMBLOCKS);
  CNetworkMessage nmPackedBlocks(MSG_GAMESTREAMBLOCKS);

  // for each sequence
  INDEX iSequence;
  for(iSequence = iSequence0; iSequence<iSequence0+ctSequences; iSequence++) {
    // get the stream block with that sequence
    CNetworkStreamBlock *pnsbBlock;
    CNetworkStream::Result res = sso.sso_nsBuffer.GetBlockBySequence(iSequence, pnsbBlock);
    // if it is not found
    if (res!=CNetworkStream::R_OK) {
      // tell the requesting session state to disconnect
      SendDisconnectMessage(iClient, TRANS("Gamestream synchronization lost"));
      return;
    }

    CNetworkMessage nmPackedBlocksNew(MSG_GAMESTREAMBLOCKS);
    // pack it in the batch
    pnsbBlock->WriteToMessage(nmGameStreamBlocks);
    nmGameStreamBlocks.PackDefault(nmPackedBlocksNew);
    // if the batch is too large
    if (nmPackedBlocksNew.nm_slSize>512) {
      // stop
      break;
    }
    // use new pack
    nmPackedBlocks = nmPackedBlocksNew;
  }

  // send the last batch of valid size
  _pfNetworkProfile.IncrementCounter(CNetworkProfile::PCI_GAMESTREAMRESENDS);
  _pNetwork->SendToClient(iClient, nmPackedBlocks);
  extern INDEX net_bReportMiscErrors;
  if (net_bReportMiscErrors) {
    CPrintF(TRANSV(" sent %d-%d(%d - %db)\n"), 
      iSequence0, iSequence, iSequence-iSequence0-1, nmPackedBlocks.nm_slSize);
  }
}

/* Get number of active players. */
INDEX CServer::GetPlayersCount(void)
{
  INDEX ctPlayers = 0;
  FOREACHINSTATICARRAY(srv_aplbPlayers, CPlayerBuffer, itplb) {
    if (itplb->IsActive()) {
      ctPlayers++;
    }
  }
  return ctPlayers;
}
/* Get number of active vip players. */
INDEX CServer::GetVIPPlayersCount(void)
{
  INDEX ctPlayers = 0;
  FOREACHINSTATICARRAY(srv_aplbPlayers, CPlayerBuffer, itplb) {
    if (itplb->IsActive() && srv_assoSessions[itplb->plb_iClient].sso_bVIP) {
      ctPlayers++;
    }
  }
  return ctPlayers;
}
/* Get total number of active clients. */
INDEX CServer::GetClientsCount(void)
{
  INDEX ctClients = 0;
  // for each active session
  for(INDEX iSession=0; iSession<srv_assoSessions.Count(); iSession++) {
    CSessionSocket &sso = srv_assoSessions[iSession];
    if (iSession>0 && !sso.IsActive()) {
      continue;
    }
    ctClients++;
  }

  return ctClients;

}
/* Get number of active vip clients. */
INDEX CServer::GetVIPClientsCount(void)
{
  INDEX ctClients = 0;
  // for each active session
  for(INDEX iSession=0; iSession<srv_assoSessions.Count(); iSession++) {
    CSessionSocket &sso = srv_assoSessions[iSession];
    if (iSession>0 && !sso.IsActive()) {
      continue;
    }
    if (sso.sso_bVIP) {
      ctClients++;
    }
  }

  return ctClients;
}
/* Get number of active observers. */
INDEX CServer::GetObserversCount(void)
{
  INDEX ctClients = 0;
  // for each active session
  for(INDEX iSession=0; iSession<srv_assoSessions.Count(); iSession++) {
    CSessionSocket &sso = srv_assoSessions[iSession];
    if (iSession>0 && !sso.IsActive()) {
      continue;
    }
    if (sso.sso_ctLocalPlayers == 0) {
      ctClients++;
    }
  }

  return ctClients;
}

/* Get number of active players on one client. */
INDEX CServer::GetPlayersCountForClient(INDEX iClient)
{
  INDEX ctPlayers = 0;
  FOREACHINSTATICARRAY(srv_aplbPlayers, CPlayerBuffer, itplb) {
    if (itplb->IsActive() && itplb->plb_iClient==iClient) {
      ctPlayers++;
    }
  }
  return ctPlayers;
}

/*
 * Find first inactive client.
 */
CPlayerBuffer *CServer::FirstInactivePlayer(void)
{
  // for all players in game
  FOREACHINSTATICARRAY(srv_aplbPlayers, CPlayerBuffer, itplb) {
    // if player is not active
    if (!itplb->IsActive()) {
      // return it
      return &itplb.Current();
    }
  }
  // if no inactive players found, return error
  return NULL;
}

/*
 * Check if some character already exists in this session.
 */
BOOL CServer::CharacterNameIsUsed(CPlayerCharacter &pcCharacter)
{
  // for all players in game
  FOREACHINSTATICARRAY(srv_aplbPlayers, CPlayerBuffer, itplb) {
    // if it is active and has same character as this one
    if (itplb->IsActive() && itplb->plb_pcCharacter == pcCharacter) {
      // it exists
      return TRUE;
    }
  }
  // otherwise, it doesn't exist
  return FALSE;
}

// find a mask of all players on a certain client
ULONG CServer::MaskOfPlayersOnClient(INDEX iClient)
{
  ULONG ulClientPlayers = 0;
  for(INDEX ipl=0; ipl<srv_aplbPlayers.Count(); ipl++) {
    CPlayerBuffer &plb = srv_aplbPlayers[ipl];
    if (plb.IsActive() && plb.plb_iClient==iClient) {
      ulClientPlayers |= 1UL<<ipl;
    }
  }
  return ulClientPlayers;
}

/*
 * Handle network messages.
 */
void CServer::ServerLoop(void)
{
  // if not started
  if (!srv_bActive) {
    ASSERTALWAYS("Running server loop for before starting server!");
    // do not gather/send actions
    return;
  }

  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_SERVER_LOOP);

//  try {
//    _cmiComm.Server_Accept_t();
//  } catch (const char *strError) {
//    CPrintF(TRANSV("Accepting failed, no more clients can connect: %s\n"), strError);
//  }
  // handle all incoming messages
  HandleAll();

  //INDEX iSpeed = 1;
  extern INDEX ser_bWaitFirstPlayer;
  // if the local session is keeping up with time and not paused
  BOOL bPaused = srv_bPause || _pNetwork->ga_bLocalPause || _pNetwork->IsWaitingForPlayers() || 
    srv_bGameFinished || ser_bWaitFirstPlayer;
  if (_pNetwork->ga_sesSessionState.ses_bKeepingUpWithTime
    &&(_pTimer->GetRealTimeTick()-_pNetwork->ga_sesSessionState.ses_tmLastUpdated<=_pTimer->TickQuantum*2.01f)
    && !bPaused ) {

    // advance time
    srv_fServerStep += _pNetwork->ga_fGameRealTimeFactor*_pNetwork->ga_sesSessionState.ses_fRealTimeFactor;
    // if stepped to next tick
    if (srv_fServerStep>=1.0f) {
    
      // find how many ticks were stepped
      INDEX iSpeed = ClampDn(INDEX(srv_fServerStep), (INDEX)1);
      srv_fServerStep -= iSpeed;

      // for each tick
      for( INDEX i=0; i<iSpeed; i++) {
        // make allaction messages for one tick
        MakeAllActions();
      }
    }
  }

  // for each active session
  for(INDEX iSession=0; iSession<srv_assoSessions.Count(); iSession++) {
    CSessionSocket &sso = srv_assoSessions[iSession];
    if (iSession>0 && (!sso.IsActive() || !sso.sso_bSendStream)) {
      continue;
    }
    // send one regular batch of sequences to the client
    SendGameStreamBlocks(iSession);
  }

  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SERVER_LOOP);
}

// make allaction messages for one tick
void CServer::MakeAllActions(void)
{
  // increment tick counter and processed sequence
  srv_tmLastProcessedTick += _pTimer->TickQuantum;
  srv_iLastProcessedSequence++;

  // for each active session
  for(INDEX iSession=0; iSession<srv_assoSessions.Count(); iSession++) {
    CSessionSocket &sso = srv_assoSessions[iSession];
    if (iSession>0 && !sso.IsActive()) {
      continue;
    }

    // create all-actions message
    CNetworkStreamBlock nsbAllActions(MSG_SEQ_ALLACTIONS, srv_iLastProcessedSequence);
    // write time there
    nsbAllActions<<srv_tmLastProcessedTick;

    // for all players in game
#ifndef NDEBUG
    INDEX iPlayer = 0;
#endif // NDEBUG
    FOREACHINSTATICARRAY(srv_aplbPlayers, CPlayerBuffer, itplb) {
      // if player is active
      if (itplb->IsActive()) {
  // player indices transmission is unneccessary unless if debugging
  //            // write its index
  //            nsbAllActions<<iPlayer;
        // write its action
        itplb->CreateActionPacket(&nsbAllActions, iSession);
      }
#ifndef NDEBUG
      iPlayer++;
#endif // NDEBUG
    }

    // add the all-actions block to the buffer
    sso.sso_nsBuffer.AddBlock(nsbAllActions);
  }

  // for all players in game
  {FOREACHINSTATICARRAY(srv_aplbPlayers, CPlayerBuffer, itplb) {
    // if player is active
    if (itplb->IsActive()) {
      // flush oldest action
      itplb->AdvanceActionBuffer();
    }
  }}
}

// add a block to streams for all sessions
void CServer::AddBlockToAllSessions(CNetworkStreamBlock &nsb)
{
  // for each active session
  for(INDEX iSession=0; iSession<srv_assoSessions.Count(); iSession++) {
    CSessionSocket &sso = srv_assoSessions[iSession];
    if (iSession>0 && !sso.IsActive()) {
      continue;
    }

    // add the block to the buffer
    sso.sso_nsBuffer.AddBlock(nsb);
  }
}

/* Send initialization info to local client. */
void CServer::ConnectLocalSessionState(INDEX iClient, CNetworkMessage &nm)
{
  // find session of this client
  CSessionSocket &sso = srv_assoSessions[iClient];
  // activate it
  sso.Activate();
  // prepare his initialization message
  CNetworkMessage nmInitMainServer(MSG_REP_CONNECTLOCALSESSIONSTATE);
  nmInitMainServer<<srv_tmLastProcessedTick;
  nmInitMainServer<<srv_iLastProcessedSequence;
  sso.sso_ctLocalPlayers = -1;
  nm>>sso.sso_sspParams;

  // send him server session state initialization message
  _pNetwork->SendToClientReliable(iClient, nmInitMainServer);
}

/* Send initialization info to remote client. */
void CServer::ConnectRemoteSessionState(INDEX iClient, CNetworkMessage &nm)
{
  ASSERT(iClient>0);
  // find session of this client
  CSessionSocket &sso = srv_assoSessions[iClient];
  
  // if the IP is banned
  if (!MatchesBanMask(_cmiComm.Server_GetClientName(iClient), ser_strIPMask) != !ser_bInverseBanning) {
    // disconnect the client
    SendDisconnectMessage(iClient, TRANS("You are banned from this server"), /*bStream=*/TRUE);
    return;
  }

  // read version info
  INDEX iTag, iMajor, iMinor;
  nm>>iTag;
  #define VTAG 0x56544147  // Looks like 'VTAG' in ASCII.
  if (iTag==VTAG) {
    nm>>iMajor>>iMinor;
  } else {
    iMajor = 109;
    iMinor = 1;
  }
  // if wrong
  if (iMajor!=_SE_BUILD_MAJOR || iMinor!=_SE_BUILD_MINOR) {
    // disconnect the client
    CTString strExplanation;
    strExplanation.PrintF(TRANSV(
      "This server runs version %d.%d, your version is %d.%d.\n"
      "Please visit http://www.croteam.com for information on version updating."),
      _SE_BUILD_MAJOR, _SE_BUILD_MINOR, iMajor, iMinor);
    SendDisconnectMessage(iClient, strExplanation, /*bStream=*/TRUE);
    return;
  }
  extern CTString net_strConnectPassword;
  extern CTString net_strVIPPassword;
  extern CTString net_strObserverPassword;
  extern INDEX net_iVIPReserve;
  extern INDEX net_iMaxObservers;
  extern INDEX net_iMaxClients;
  CTString strGivenMod;
  CTString strGivenPassword;
  nm>>strGivenMod>>strGivenPassword;
  INDEX ctWantedLocalPlayers;
  nm>>ctWantedLocalPlayers;
  // if wrong mod
  if (_strModName!=strGivenMod) {
    // disconnect the client
    // NOTE: DO NOT TRANSLATE THIS STRING!
    CTString strMod(0, "MOD:%s\\%s", (const char *) _strModName, (const char *) _strModURL);
    SendDisconnectMessage(iClient, strMod, /*bStream=*/TRUE);
    return;
  }

  // get counts of allowed players, clients, vips and  check for connection allowance
  INDEX ctMaxAllowedPlayers = _pNetwork->ga_sesSessionState.ses_ctMaxPlayers;
  INDEX ctMaxAllowedClients = ctMaxAllowedPlayers;
  if (net_iMaxClients>0) {
    ctMaxAllowedClients = ClampUp(net_iMaxClients, (INDEX)NET_MAXGAMECOMPUTERS);
  }
  INDEX ctMaxAllowedVIPPlayers = 0;
  INDEX ctMaxAllowedVIPClients = 0;
  if (net_iVIPReserve>0 && net_strVIPPassword!="") {
    ctMaxAllowedVIPPlayers = ClampDn(net_iVIPReserve-GetVIPPlayersCount(), (INDEX)0);
    ctMaxAllowedVIPClients = ClampDn(net_iVIPReserve-GetVIPClientsCount(), (INDEX)0);
  }
  INDEX ctMaxAllowedObservers = net_iMaxObservers;

  // get current counts
  INDEX ctCurrentPlayers = GetPlayersCount();
  INDEX ctCurrentClients = GetClientsCount();
  INDEX ctCurrentObservers = GetObserversCount();

  // check which passwords this client can satisfy
  BOOL bAutorizedAsVIP = FALSE;
  BOOL bAutorizedAsObserver = FALSE;
  BOOL bAutorizedAsPlayer = FALSE;
  if (net_strVIPPassword!="" && net_strVIPPassword==strGivenPassword) {
    bAutorizedAsVIP = TRUE;
    bAutorizedAsPlayer = TRUE;
    bAutorizedAsObserver = TRUE;
  }
  if (net_strConnectPassword=="" || net_strConnectPassword==strGivenPassword) {
    bAutorizedAsPlayer = TRUE;
  }
  if ((net_strObserverPassword==""&&bAutorizedAsPlayer) || net_strObserverPassword==strGivenPassword) {
    bAutorizedAsObserver = TRUE;
  }

  // if the user is not authorised as a VIP
  if (!bAutorizedAsVIP) {
    // artificially decrease allowed number of players and clients
    ctMaxAllowedPlayers = ClampDn(ctMaxAllowedPlayers-ctMaxAllowedVIPPlayers, (INDEX)0);
    ctMaxAllowedClients = ClampDn(ctMaxAllowedClients-ctMaxAllowedVIPClients, (INDEX)0);
  }

  // if too many clients or players
  if (ctCurrentPlayers+ctWantedLocalPlayers>ctMaxAllowedPlayers
    ||ctCurrentClients+1>ctMaxAllowedClients) {
    // disconnect the client
    SendDisconnectMessage(iClient, TRANS("Server full!"), /*bStream=*/TRUE);
    return;
  }

  // if the user is trying to connect as observer
  if (ctWantedLocalPlayers==0) {
    // if too many observers already
    if (ctCurrentObservers>=ctMaxAllowedObservers && !bAutorizedAsVIP) {
      // disconnect the client
      SendDisconnectMessage(iClient, TRANS("Too many observers!"), /*bStream=*/TRUE);
      return;
    }
    // if observer password is wrong
    if (!bAutorizedAsObserver) {
      // disconnect the client
      if (strGivenPassword=="") {
        SendDisconnectMessage(iClient, TRANS("This server requires password for observers!"), /*bStream=*/TRUE);
      } else {
        SendDisconnectMessage(iClient, TRANS("Wrong observer password!"), /*bStream=*/TRUE);
      }
    }
  // if the user is trying to connect as player
  } else {
    // if player password is wrong
    if (!bAutorizedAsPlayer) {
      // disconnect the client
      if (strGivenPassword=="") {
        SendDisconnectMessage(iClient, TRANS("This server requires password to connect!"), /*bStream=*/TRUE);
      } else {
        SendDisconnectMessage(iClient, TRANS("Wrong password!"), /*bStream=*/TRUE);
      }
    }
    
  }

  // activate it
  sso.Activate();
  // load parameters for it
  sso.sso_ctLocalPlayers = ctWantedLocalPlayers;
  sso.sso_bVIP = bAutorizedAsVIP;
  nm>>sso.sso_sspParams;

  // try to
  try {
    // create base info to be sent
    extern CTString ser_strMOTD;
    CTMemoryStream strmInfo;
    strmInfo<<INDEX(MSG_REP_CONNECTREMOTESESSIONSTATE);
    strmInfo<<ser_strMOTD;
    strmInfo<<_pNetwork->ga_World.wo_fnmFileName;
    strmInfo<<_pNetwork->ga_sesSessionState.ses_ulSpawnFlags;
    strmInfo.Write_t(_pNetwork->ga_aubDefaultProperties, NET_MAXSESSIONPROPERTIES);
    SLONG slSize = strmInfo.GetStreamSize();

    // send the stream to the remote session state
    _pNetwork->SendToClientReliable(iClient, strmInfo);
  
    CPrintF(TRANSV("Server: Sent initialization info to '%s' (%dk)\n"),
      (const char*)_cmiComm.Server_GetClientName(iClient), slSize/1024);
  // if failed
  } catch (const char *strError) {
    // deactivate it
    sso.Deactivate();

    // report error
    CPrintF(TRANSV("Server: Cannot prepare connection data: %s\n"), strError);
  }
}

/* Send session state data to remote client. */
void CServer::SendSessionStateData(INDEX iClient)
{
  ASSERT(iClient>0);
  // find session of this client
  CSessionSocket &sso = srv_assoSessions[iClient];
  // copy its buffer from local session state
  sso.sso_nsBuffer.Copy(srv_assoSessions[0].sso_nsBuffer);

  // try to
  try {
    // prepare files or memory streams for connection info
    CTFileStream strmStateFile; CTMemoryStream strmStateMem;
    CTFileStream strmDeltaFile; CTMemoryStream strmDeltaMem;
    CTStream *pstrmState; CTMemoryStream *pstrmDelta;
    extern INDEX net_bDumpConnectionInfo;

#ifndef NDEBUG
    UBYTE* pubSrc = NULL;
#endif // NDEBUG
    /*if (net_bDumpConnectionInfo) {
      strmStateFile.Create_t(CTString("Temp\\State.bin"));
      strmDeltaFile.Create_t(CTString("Temp\\Delta.bin"));
      pstrmState = &strmStateFile;
      pstrmDelta = &strmDeltaFile;
    } else {*/
      pstrmState = &strmStateMem;
      pstrmDelta = &strmDeltaMem;

#ifndef NDEBUG
      pubSrc = strmDeltaMem.mstrm_pubBuffer + strmDeltaMem.mstrm_slLocation;
#endif // NDEBUG
    //}

    ASSERT(pubSrc != NULL);

    // write main session state
    _pNetwork->ga_sesSessionState.Write_t(pstrmState);
    pstrmState->SetPos_t(0);
    SLONG slFullSize = pstrmState->GetStreamSize();

    CTMemoryStream strmInfo;
    strmInfo<<INDEX(MSG_REP_STATEDELTA);

    // compress it to another one, using delta from original
    CTMemoryStream strmDefaultState;
    strmDefaultState.Write_t
      (_pNetwork->ga_pubDefaultState, _pNetwork->ga_slDefaultStateSize);
    strmDefaultState.SetPos_t(0);
    DIFF_Diff_t(&strmDefaultState, pstrmState, pstrmDelta);
    pstrmDelta->SetPos_t(0);
    SLONG slDeltaSize = pstrmDelta->GetStreamSize();
    CzlibCompressor comp;
    comp.PackStream_t(*pstrmDelta, strmInfo);

    SLONG slSize = strmInfo.GetStreamSize();

    // send the stream to the remote session state
    _pNetwork->SendToClientReliable(iClient, strmInfo);
  
    CPrintF(TRANSV("Server: Sent connection data to '%s' (%dk->%dk->%dk)\n"),
      (const char*)_cmiComm.Server_GetClientName(iClient), 
      slFullSize/1024, slDeltaSize/1024, slSize/1024);
    if (net_bDumpConnectionInfo) {
      CPrintF(TRANSV("Server: Connection data dumped.\n"));
    }

  // if failed
  } catch (const char *strError) {
    // deactivate it
    sso.Deactivate();

    // report error
    CPrintF(TRANSV("Server: Cannot prepare connection data: %s\n"), strError);
  }
}

/* Handle incoming network messages. */
void CServer::HandleAll()
{
  // clear last accepted client info
  /* INDEX iClient = -1;
  if (_cmiComm.GetLastAccepted(iClient)) {
    CPrintF(TRANSV("Server: Accepted session connection by '%s'\n"),
      (const char *) _cmiComm.Server_GetClientName(iClient));
  }
	*/

  // for each active client
  {for( INDEX iClient=0; iClient<NET_MAXGAMECOMPUTERS; iClient++) {
    // handle all of its messages
    HandleAllForAClient(iClient);
  }}
}




void CServer::HandleAllForAClient(INDEX iClient)
{
  // if the client is not connected
  if (!_cmiComm.Server_IsClientUsed(iClient)) {
    // skip it
    return;
  }

	// update the client's max BPS limit from the session socket parameters
	cm_aciClients[iClient].ci_pbOutputBuffer.pb_pbsLimits.pbs_fBandwidthLimit = srv_assoSessions[iClient].sso_sspParams.ssp_iMaxBPS*8;

  // find session of this client
  CSessionSocket &sso = srv_assoSessions[iClient];
  // if hasn't received sync check for too long
  extern INDEX ser_bKickOnSyncLate;
  if (ser_bKickOnSyncLate &&sso.sso_bActive &&
      sso.sso_tmLastSyncReceived>0 && 
      sso.sso_tmLastSyncReceived<_pNetwork->ga_sesSessionState.ses_tmLastSyncCheck -
      (2*ser_iSyncCheckBuffer*_pNetwork->ga_sesSessionState.ses_tmSyncCheckFrequency)) {
    SendDisconnectMessage(iClient, TRANS("No valid SYNCCHECK received for too long!"));
  }

  if (iClient>0 && sso.sso_bActive && sso.sso_bSendStream && sso.sso_tvMessageReceived.tv_llValue>0 &&
    (_pTimer->GetHighPrecisionTimer()-sso.sso_tvMessageReceived).GetSeconds()>net_tmDisconnectTimeout) {
    SendDisconnectMessage(iClient, TRANS("Connection timeout"));
  }

  // if the client is disconnected
  if (!_cmiComm.Server_IsClientUsed(iClient) || sso.sso_iDisconnectedState>1) {
    CPrintF(TRANSV("Server: Client '%s' disconnected.\n"), (const char *) _cmiComm.Server_GetClientName(iClient));
    // clear it
    _cmiComm.Server_ClearClient(iClient);
    // free all that data that was allocated for the client
    HandleClientDisconected(iClient);
  }

  CNetworkMessage nmReceived;
  // repeat
  FOREVER {
    // if there is some reliable message
    if (_pNetwork->ReceiveFromClientReliable(iClient, nmReceived)) {
      // process it
      Handle(iClient, nmReceived);

    // if there are no more messages
    } else {
      // skip to receiving unreliable
      break;
    }
  }

	// if the client has confirmed disconnect in this loop
  if (!_cmiComm.Server_IsClientUsed(iClient) || sso.sso_iDisconnectedState>1) {
    CPrintF(TRANSV("Server: Client '%s' disconnected.\n"), (const char *) _cmiComm.Server_GetClientName(iClient));
    // clear it
    _cmiComm.Server_ClearClient(iClient);
    // free all that data that was allocated for the client
    HandleClientDisconected(iClient);
  }

  // repeat
  FOREVER {
    // if there is some unreliable message
    if (_pNetwork->ReceiveFromClient(iClient, nmReceived)) {
      // process it
      Handle(iClient, nmReceived);
    // if there are no more messages
    } else {
      // finish with this client
      break;
    }
  }
}

void CServer::HandleClientDisconected(INDEX iClient)
{
  // find session of this client
  CSessionSocket &sso = srv_assoSessions[iClient];
  // deactivate it
  sso.Deactivate();            

  INDEX iPlayer = 0;
  FOREACHINSTATICARRAY(srv_aplbPlayers, CPlayerBuffer, itplb) {
    // if player is on that client
    if (itplb->plb_iClient==iClient) {
      // create message for removing player from all session
      CNetworkStreamBlock nsbRemPlayerData(MSG_SEQ_REMPLAYER, ++srv_iLastProcessedSequence);
      nsbRemPlayerData<<iPlayer;      // player index
      // put the message in buffer to be sent to all sessions
      AddBlockToAllSessions(nsbRemPlayerData);
      // deactivate it
      itplb->Deactivate();
    }
    iPlayer++;
  }
}

// split the rcon response string into lines and send one by one to the client
static void SendAdminResponse(INDEX iClient, const CTString &strResponse)
{
  CTString str = strResponse;

  while (str!="") {
    CTString strLine = str;
    strLine.OnlyFirstLine();
    str.RemovePrefix(strLine);
    str.DeleteChar(0);
    if (strLine.Length()>0) { 
      CNetworkMessage nm(MSG_ADMIN_RESPONSE);
      nm<<strLine;
      _pNetwork->SendToClientReliable(iClient, nm);
    }
  }
}

void CServer::Handle(INDEX iClient, CNetworkMessage &nmMessage)
{
  CSessionSocket &sso = srv_assoSessions[iClient];
  sso.sso_tvMessageReceived = _pTimer->GetHighPrecisionTimer();

  switch (nmMessage.GetType()) {

  // if if it is just keepalive, ignore it
  case MSG_KEEPALIVE: break;
	case MSG_REP_DISCONNECTED: {
		CSessionSocket &sso = srv_assoSessions[iClient];
		sso.sso_iDisconnectedState=2;
		} break;

  // if local session state asks for registration
  case MSG_REQ_CONNECTLOCALSESSIONSTATE: {
    ConnectLocalSessionState(iClient, nmMessage);
		} break;
  // if remote server asks for registration
  case MSG_REQ_CONNECTREMOTESESSIONSTATE: {
    ConnectRemoteSessionState(iClient, nmMessage);
    } break;
  // if remote server asks for data
  case MSG_REQ_STATEDELTA: {
    CPrintF(TRANSV("Sending statedelta response\n"));
    SendSessionStateData(iClient);
    } break;
  // if player asks for registration
  case MSG_REQ_CONNECTPLAYER: {

    // check that someone doesn't add too many players
    if (iClient>0 && GetPlayersCountForClient(iClient)>=sso.sso_ctLocalPlayers) {
      CTString strMessage;
      strMessage.PrintF(TRANSV("Protocol violation"));
      SendDisconnectMessage(iClient, strMessage);
    }

    // read character data from the message
    CPlayerCharacter pcCharacter;
    nmMessage>>pcCharacter;

    // if the name is banned
    if (!MatchesBanMask(pcCharacter.GetName(), ser_strNameMask) != !ser_bInverseBanning) {
      // disconnect the client
      SendDisconnectMessage(iClient, TRANS("You are banned from this server"), /*bStream=*/TRUE);
      return;
    }

    CPlayerBuffer *pplbNewClient;

    // find some inactive player
    pplbNewClient = FirstInactivePlayer();
    // if there is some active player with same character name
    if (CharacterNameIsUsed(pcCharacter)) {
      // send refusal message
      CTString strMessage;
      strMessage.PrintF(TRANSV("Player character '%s' already exists in this session."),
        (const char *) pcCharacter.GetName());
      SendDisconnectMessage(iClient, strMessage);

    // if the max. number of clients is not reached
    } else if (pplbNewClient!=NULL) {
      // activate it
      pplbNewClient->Activate(iClient);
      INDEX iNewPlayer = pplbNewClient->plb_Index;

      // remember its character
      pplbNewClient->plb_pcCharacter = pcCharacter;

      // create message for adding player data to sessions
      CNetworkStreamBlock nsbAddClientData(MSG_SEQ_ADDPLAYER, ++srv_iLastProcessedSequence);
      nsbAddClientData<<iNewPlayer;      // client index
      nsbAddClientData<<pcCharacter;     // client character data
      extern INDEX ser_bWaitFirstPlayer;
      ser_bWaitFirstPlayer = 0; // player is here don't wait any more

      // put the message in buffer to be sent to all servers
      AddBlockToAllSessions(nsbAddClientData);

      // send him client initialization message
      CNetworkMessage nmPlayerRegistered(MSG_REP_CONNECTPLAYER);
      nmPlayerRegistered<<iNewPlayer;   // player index
      _pNetwork->SendToClientReliable(iClient, nmPlayerRegistered);

      // notify gameagent
      GameAgent_ServerStateChanged();
    // if refused
    } else {
      // send him refusal message
      SendDisconnectMessage(iClient, TRANS("Too many players in session."));
    }
    } break;

  // if client source wants to change character
  case MSG_REQ_CHARACTERCHANGE: {
    // read character data from the message
    INDEX iPlayer;
    CPlayerCharacter pcCharacter;
    nmMessage>>iPlayer>>pcCharacter;
    // first check if the request is valid
    if (iPlayer<0 || iPlayer>srv_aplbPlayers.Count() ) {
      break;
    }
    CPlayerBuffer &plb = srv_aplbPlayers[iPlayer];
    if (plb.plb_iClient!=iClient || !(plb.plb_pcCharacter==pcCharacter) ) {
      break;
    }

    // if all was right, add that as change a sequence
    CNetworkStreamBlock nsbChangeChar(MSG_SEQ_CHARACTERCHANGE, ++srv_iLastProcessedSequence);
    nsbChangeChar<<iPlayer;
    nsbChangeChar<<pcCharacter;

    plb.plb_pcCharacter = pcCharacter;

    // put the message in buffer to be sent to all clients
    AddBlockToAllSessions(nsbChangeChar);

                                } break;

  // if client source sends action packet
  case MSG_ACTION: {
    CSessionSocket &sso = srv_assoSessions[iClient];
    // for each possible player on that client
    for(INDEX ipls=0; ipls<NET_MAXLOCALPLAYERS; ipls++) {
      // see if saved in the message
      BOOL bSaved = 0;
      nmMessage.ReadBits(&bSaved, 1);
      // if saved
      if (bSaved) {
        // read client index
        INDEX iPlayer = 0;
        nmMessage.ReadBits(&iPlayer, 4);
        CPlayerBuffer &plb = srv_aplbPlayers[iPlayer];
        // if the player is not on that client
        if (plb.plb_iClient!=iClient) {
          // consider the entire message invalid
          CPrintF("Wrong Client!\n");
          break;
        }
        // read ping
        plb.plb_iPing = 0;
        nmMessage.ReadBits(&plb.plb_iPing, 10);
        // let the corresponding client buffer receive the message
        INDEX iMaxBuffer = sso.sso_sspParams.ssp_iBufferActions;
        extern INDEX cli_bPredictIfServer;
        if (iClient==0 && !cli_bPredictIfServer) {
          iMaxBuffer = 1;
        }
        plb.ReceiveActionPacket(&nmMessage, iMaxBuffer);
      }
    }
    } break;
  // if client sent a synchronization check
  case MSG_SYNCCHECK: {
      extern INDEX ser_bReportSyncOK;
      extern INDEX ser_bReportSyncBad;
      extern INDEX ser_bReportSyncLate;
      extern INDEX ser_bReportSyncEarly;
      extern INDEX ser_bPauseOnSyncBad;
      extern INDEX ser_iKickOnSyncBad;

      // read sync check from the packet
      CSyncCheck scRemote;
      nmMessage.Read(&scRemote, sizeof(scRemote));
    
      // try to find it in buffer
      CSyncCheck scLocal;
      INDEX iFound = FindSyncCheck(scRemote.sc_tmTick, scLocal);
      // if found
      if (iFound==0) {
        // flush the clients stream buffer up to that sequence 
        // (the sync is used as piggy-backed acknowledge of packet receival)
        CSessionSocket &sso = srv_assoSessions[iClient];
        sso.sso_nsBuffer.RemoveOlderBlocksBySequence(scRemote.sc_iSequence);

        // if level was changed
        if (scLocal.sc_iLevel!=scRemote.sc_iLevel) {
          // disconnect the client
          SendDisconnectMessage(iClient, TRANS("Level change in progress. Please retry."));
        // if it is wrong crc
        } else if (scLocal.sc_ulCRC!=scRemote.sc_ulCRC) {
          sso.sso_ctBadSyncs++;
          if( ser_bReportSyncBad) {
            CPrintF( TRANS("SYNCBAD: Client '%s', Sequence %d Tick %.2f - bad %d\n"), 
              (const char *) _cmiComm.Server_GetClientName(iClient), scRemote.sc_iSequence , scRemote.sc_tmTick, sso.sso_ctBadSyncs);
          }
          if (ser_iKickOnSyncBad>0) {
            if (sso.sso_ctBadSyncs>=ser_iKickOnSyncBad) {
              SendDisconnectMessage(iClient, TRANS("Too many bad syncs"));
            }
          } else if( ser_bPauseOnSyncBad) {
            _pNetwork->ga_sesSessionState.ses_bWantPause = TRUE;
          }
        } else {
          sso.sso_ctBadSyncs = 0;
          if (ser_bReportSyncOK) {
            CPrintF( TRANS("SYNCOK: Client '%s', Tick %.2f\n"), 
              (const char *) _cmiComm.Server_GetClientName(iClient), scRemote.sc_tmTick);
          }
        }
        
        // remember that this client has sent sync for that tick
        if (srv_assoSessions[iClient].sso_tmLastSyncReceived<scRemote.sc_tmTick) {
          srv_assoSessions[iClient].sso_tmLastSyncReceived = scRemote.sc_tmTick;
        }

      // if too old
      } else if (iFound<0) {
        // report only if syncs are ok now (so that we don't report a bunch of late syncs on level change
        if( ser_bReportSyncLate && srv_assoSessions[iClient].sso_tmLastSyncReceived>0) {
          CPrintF( TRANS("SYNCLATE: Client '%s', Tick %.2f\n"), 
            (const char *) _cmiComm.Server_GetClientName(iClient), scRemote.sc_tmTick);
        }
      // if too new
      } else {
        if( ser_bReportSyncEarly) {
          CPrintF( TRANS("SYNCEARLY: Client '%s', Tick %.2f\n"), 
            (const char *) _cmiComm.Server_GetClientName(iClient), scRemote.sc_tmTick);
        }
        // remember that this client has sent sync for that tick
        // (even though we cannot really check that it is valid)
        if (srv_assoSessions[iClient].sso_tmLastSyncReceived<scRemote.sc_tmTick) {
          srv_assoSessions[iClient].sso_tmLastSyncReceived = scRemote.sc_tmTick;
        }
      }

    } break;
  // if a server requests resend of some game stream packets
  case MSG_REQUESTGAMESTREAMRESEND: {
    // get the sequences from the block
    INDEX iSequence0, ctSequences;
    nmMessage>>iSequence0;
    nmMessage>>ctSequences;
    // resend the game stream blocks to the server
    ResendGameStreamBlocks(iClient, iSequence0, ctSequences);
    } break;
  // if a client wants to toggle pause
  case MSG_REQ_PAUSE: {
    // read the pause state from the message
    BOOL bWantPause;
    nmMessage>>(INDEX&)bWantPause;
    // if state is new
    if (!srv_bPause != !bWantPause) {

      // if the client may pause
      extern INDEX ser_bClientsMayPause;
      if (_cmiComm.Server_IsClientLocal(iClient) || ser_bClientsMayPause) {
        // change it
        srv_bPause = bWantPause;
        // add the pause state block to the buffer to be sent to all clients
        CNetworkStreamBlock nsbPause(MSG_SEQ_PAUSE, ++srv_iLastProcessedSequence);
        nsbPause<<(INDEX&)srv_bPause;
        nsbPause<<_cmiComm.Server_GetClientName(iClient);
        AddBlockToAllSessions(nsbPause);
      }
    }
  } break;
  // if a player wants to change its buffer settings
  case MSG_SET_CLIENTSETTINGS: {
    // read data
    CSessionSocket &sso = srv_assoSessions[iClient];
    nmMessage>>sso.sso_sspParams;
  } break;
  // if a chat message was sent
  case MSG_CHAT_IN: {
    // get it
    ULONG ulFrom, ulTo;
    CTString strMessage;
    nmMessage>>ulFrom>>ulTo>>strMessage;

    // filter the from address by the client's players
    ulFrom &= MaskOfPlayersOnClient(iClient);
    // if the source has no players
    if (ulFrom==0) {
      // make it public message
      ulTo = (ULONG) -1;
    }

    // make the outgoing message
    CNetworkMessage nmOut(MSG_CHAT_OUT);
    nmOut<<ulFrom;
    if (ulFrom==0) {
      CTString strFrom;
      if (iClient==0) {
        strFrom = TRANS("Server");
      } else {
        strFrom.PrintF(TRANSV("Client %d"), iClient);
      }
      nmOut<<strFrom;
    }
    nmOut<<strMessage;

    // for each active client
    for(INDEX iSession=0; iSession<srv_assoSessions.Count(); iSession++) {
      CSessionSocket &sso = srv_assoSessions[iSession];
      if (iSession>0 && !sso.IsActive()) {
        continue;
      }
      // if message is public or the client has some of destination players
      if (ulTo==(static_cast<ULONG>(-1)) || ulTo&MaskOfPlayersOnClient(iSession)) {
        // send the message to that computer
        _pNetwork->SendToClient(iSession, nmOut);
      }
    }

  } break;
  // if a crc response is received
    case MSG_REQ_CRCLIST: {
      CPrintF(TRANSV("Sending CRC response\n"));
    // create CRC challenge
    CTMemoryStream strmCRC;
    strmCRC<<INDEX(MSG_REQ_CRCCHECK);
    strmCRC.Write_t(_pNetwork->ga_pubCRCList, _pNetwork->ga_slCRCList);
    SLONG slSize = strmCRC.GetStreamSize();

    // send the stream to the remote session state
    _pNetwork->SendToClientReliable(iClient, strmCRC);
    CPrintF(TRANSV("Server: Sent CRC challenge to '%s' (%dk)\n"),
      (const char*)_cmiComm.Server_GetClientName(iClient), slSize/1024);

  } break;
  // if a crc response is received
  case MSG_REP_CRCCHECK: {
    // get it
    ULONG ulCRC;
    INDEX iLastSequence;
    nmMessage>>ulCRC>>iLastSequence;
    // if not same
    if (_pNetwork->ga_ulCRC!=ulCRC) {
      // disconnect the client
      SendDisconnectMessage(iClient, TRANS("Wrong CRC check."));
    // if same
    } else {
      CPrintF(TRANSV("Server: Client '%s', CRC check OK\n"), 
        (const char*)_cmiComm.Server_GetClientName(iClient));
      // use the piggybacked sequence number to initiate sending stream to it
      CSessionSocket &sso = srv_assoSessions[iClient];
      sso.sso_bSendStream = TRUE;
      sso.sso_nsBuffer.RemoveOlderBlocksBySequence(iLastSequence);
      sso.sso_iLastSentSequence = iLastSequence;
    }
   
  } break;
  // if a rcon request is received
  case MSG_ADMIN_COMMAND: {
    extern CTString net_strAdminPassword;
    // get it
    CTString strPassword, strCommand;
    nmMessage>>strPassword>>strCommand;
    if (net_strAdminPassword=="") {
      CNetworkMessage nmRes(MSG_ADMIN_RESPONSE);
      nmRes<<CTString(TRANS("Remote administration not allowed on this server.\n"));
      CPrintF(TRANSV("Server: Client '%s', Tried to use remote administration.\n"), 
        (const char*)_cmiComm.Server_GetClientName(iClient));
      _pNetwork->SendToClientReliable(iClient, nmRes);
    } else if (net_strAdminPassword!=strPassword) {
      CPrintF(TRANSV("Server: Client '%s', Wrong password for remote administration.\n"), 
        (const char*)_cmiComm.Server_GetClientName(iClient));
      SendDisconnectMessage(iClient, TRANS("Wrong admin password. The attempt was logged."));
      break;
    } else {

      CPrintF(TRANSV("Server: Client '%s', Admin cmd: %s\n"), 
        (const char*)_cmiComm.Server_GetClientName(iClient), (const char *) strCommand);

      con_bCapture = TRUE;
      con_strCapture = "";
      _pShell->Execute(strCommand+";");

      CTString strResponse = CTString(">")+strCommand+"\n"+con_strCapture;
      SendAdminResponse(iClient, strResponse);
      con_bCapture = FALSE;
      con_strCapture = "";
    }
  } break;
  // otherwise
  default:
    ASSERT(FALSE);
  }
}
