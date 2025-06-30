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
#include <Engine/Network/NetworkMessage.h>
#include <Engine/Network/Diff.h>
#include <Engine/Base/ErrorTable.h>
#include <Engine/Base/Translation.h>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Base/CRCTable.h>
#include <Engine/Base/Shell.h>
#include <Engine/Network/SessionState.h>
#include <Engine/Network/PlayerSource.h>
#include <Engine/Entities/EntityClass.h>
#include <Engine/Math/Float.h>
#include <Engine/Network/PlayerTarget.h>
#include <Engine/Network/NetworkProfile.h>
#include <Engine/World/PhysicsProfile.h>
#include <Engine/Network/CommunicationInterface.h>
#include <Engine/Network/Compression.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/Base/Console.h>
#include <Engine/Entities/EntityProperties.h>
#include <Engine/Network/LevelChange.h>

#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Base/CRC.h>

#define SESSIONSTATEVERSION_OLD 1
#define SESSIONSTATEVERSION_WITHBULLETTIME 2
#define SESSIONSTATEVERSION_CURRENT SESSIONSTATEVERSION_WITHBULLETTIME

//#define DEBUG_LERPING 1

extern INDEX net_bLerping;
extern FLOAT net_tmConnectionTimeout;
extern FLOAT net_tmProblemsTimeOut;
extern FLOAT net_tmDisconnectTimeout;

// this is from ProgresHook.cpp - so we tell the progresshook to run client/srever updates
extern BOOL _bRunNetUpdates;

#if DEBUG_LERPING

FLOAT avfStats[1000][4];
INDEX ctCounter=0;
INDEX ctMax = sizeof(avfStats)/sizeof(avfStats[0]);

#endif // DEBUG_LERPING

// get a pseudo-random number (safe for network gaming)
ULONG CSessionState::Rnd(void) {
  ASSERTMSG(ses_bAllowRandom, "Use RND only in entity AI!");
  // NOTE: 
  // using multiplicative congruent method with Greanberger's lambda = 2^18+3
  ses_ulRandomSeed = ses_ulRandomSeed*262147;
  ASSERT(ses_ulRandomSeed!=0);
  return ses_ulRandomSeed;
}

// reset random number generator (always randomizes to same sequence!)
void CSessionState::ResetRND(void)
{
  BOOL bOldAllow = ses_bAllowRandom;
  ses_bAllowRandom = TRUE;
  // random must start at a number different than zero!
  ses_ulRandomSeed = 0x87654321;
  // run rnd a few times to make it go random
  for(INDEX i=0; i<32; i++) {
    Rnd();
  }
  ses_bAllowRandom = bOldAllow;
}

/*
 * Constructor.
 */
CSessionState::CSessionState(void)
{
  ses_bKeepingUpWithTime = TRUE;
  ses_tmLastUpdated = -100;
  ses_bAllowRandom = TRUE;  // random allowed when not in game
  ses_bPredicting = FALSE;
  ses_tmPredictionHeadTick = -2.0f;
  ses_tmLastSyncCheck = 0;
  ses_tmLastPredictionProcessed = -200;

  ses_bPause = FALSE;
  ses_bWantPause = FALSE;
  ses_bGameFinished = FALSE;
  ses_bWaitingForServer = FALSE;
  ses_strDisconnected = "";
  ses_ctMaxPlayers = 1;
  ses_bWaitAllPlayers = FALSE;
  ses_iLevel = 0;
  ses_fRealTimeFactor = 1.0f;

  ses_pstrm = NULL;
  // reset random number generator
  ResetRND();

  ses_apltPlayers.New(NET_MAXGAMEPLAYERS);
}

/*
 * Destructor.
 */
CSessionState::~CSessionState()
{
}

/*
 * Clear the object.
 */
void CSessionState::Stop(void)
{
#if DEBUG_SYNCSTREAMDUMPING
  ClearDumpStream();
#endif
#if DEBUG_SYNCSTREAMDUMPING
#endif

  ses_bKeepingUpWithTime = TRUE;
  ses_tmLastUpdated = -100;
  ses_bAllowRandom = TRUE;  // random allowed when not in game
  ses_bPredicting = FALSE;
  ses_tmPredictionHeadTick = -2.0f;
  ses_tmLastSyncCheck = 0;
  ses_bPause = FALSE;
  ses_bWantPause = FALSE;
  ses_bGameFinished = FALSE;
  ses_bWaitingForServer = FALSE;
  ses_strDisconnected = "";
  ses_ctMaxPlayers = 1;
  ses_fRealTimeFactor = 1.0f;
  ses_bWaitAllPlayers = FALSE;
  ses_apeEvents.PopAll();

  // disable lerping
  _pTimer->DisableLerp();

#if DEBUG_LERPING

  CTFileStream f;
  f.Create_t(CTFILENAME("Temp\\Lerp.stats"), CTStream::CM_TEXT);
  for (INDEX i=0; i<ctCounter; i++) {
    char strBuffer[256];
    sprintf(strBuffer, "%6.2f %6.2f %6.2f %6.2f",
      avfStats[i][0],
      avfStats[i][1],
      avfStats[i][2],
      avfStats[i][3]);
    f.PutLine_t(strBuffer);
  }
  f.Close();

#endif // DEBUG_LERPING

	CNetworkMessage nmConfirmDisconnect(MSG_REP_DISCONNECTED);
  if (_cmiComm.cci_bClientInitialized) {
	  _pNetwork->SendToServerReliable(nmConfirmDisconnect);
  }
  _cmiComm.Client_Close();

  // clear all old levels
  ForgetOldLevels();

  ses_apltPlayers.Clear();
  ses_apltPlayers.New(NET_MAXGAMEPLAYERS);
}

/*
 * Initialize session state and wait for game to be started.
 */
void CSessionState::Start_t(INDEX ctLocalPlayers) 
{
  ses_bKeepingUpWithTime = TRUE;
  ses_tmLastUpdated = -100;
  // clear message stream
  ses_nsGameStream.Clear();
  ses_bAllowRandom = FALSE;  // random not allowed in game
  ses_bPredicting = FALSE;
  ses_tmPredictionHeadTick = -2.0f;
  ses_tmLastSyncCheck = 0;
  ses_bPause = FALSE;
  ses_bWantPause = FALSE;
  ses_bWaitingForServer = FALSE;
  ses_bGameFinished = FALSE;
  ses_strDisconnected = "";
  ses_ctMaxPlayers = 1;
  ses_fRealTimeFactor = 1.0f;
  ses_bWaitAllPlayers = FALSE;
  ses_iMissingSequence = -1;
  ses_apeEvents.PopAll();
  ses_tvMessageReceived.Clear();
  _pNetwork->ga_strRequiredMod = "";

  // reset random number generator
  ResetRND();

  // clear all old levels
  ForgetOldLevels();

#if DEBUG_LERPING
  // clear lerp stats
  ctCounter = 0;
#endif // DEBUG_LERPING

  // ses_LastProcessedTick and ses_LastReceivedTick tick counters are
  // irrelevant now, will be initialized when initialization message
  // from server is received, no need to set them here

  // if this computer is server
  if (_pNetwork->IsServer()) {
    // initialize local client
    _cmiComm.Client_Init_t((ULONG) 0);
    // connect as main session state
    try {
      Start_AtServer_t();
    } catch (const char *) {
      _cmiComm.Client_Close();
      throw;
    }

  // if this computer is client
  } else {
    // connect client to server computer
    _cmiComm.Client_Init_t((char*)(const char*)_pNetwork->ga_strServerAddress);
    // connect as remote session state
    try {
      Start_AtClient_t(ctLocalPlayers);
    } catch (const char *) {
      // if failed due to wrong mod
      if (strncmp(ses_strDisconnected, "MOD:", 4)==0) {
        // remember the mod
        _pNetwork->ga_strRequiredMod = ses_strDisconnected+4;
        // make sure that the string is never empty
        if (_pNetwork->ga_strRequiredMod=="") {
          _pNetwork->ga_strRequiredMod=" ";
        }
      }
      _cmiComm.Client_Close();
      throw;
    }
  }
}

void CSessionState::Start_AtServer_t(void)     // throw char *
{
  // send registration request
  CNetworkMessage nmRegisterMainSessionState(MSG_REQ_CONNECTLOCALSESSIONSTATE);
  ses_sspParams.Update();
  nmRegisterMainSessionState<<ses_sspParams;
  _pNetwork->SendToServerReliable(nmRegisterMainSessionState);

  for(TIME tmWait=0; tmWait<net_tmConnectionTimeout*1000; 
      _pTimer->Sleep(NET_WAITMESSAGE_DELAY), tmWait+=NET_WAITMESSAGE_DELAY) {
    _pNetwork->TimerLoop();
    if (_cmiComm.Client_Update() == FALSE) {
			break;
		}

    // wait for message to come
    CNetworkMessage nmReceived;
    if (!_pNetwork->ReceiveFromServerReliable(nmReceived)) {
      continue;
    }
    // if this is the init message
    if (nmReceived.GetType() == MSG_REP_CONNECTLOCALSESSIONSTATE) {
      // just adjust your tick counters
      nmReceived>>ses_tmLastProcessedTick;
      nmReceived>>ses_iLastProcessedSequence;
      ses_tmInitializationTick  = -1.0f;
      ses_tmInitializationTick2 = -1.0f;
      // finish waiting
      return;
    // otherwise
    } else {
      // it is invalid message
      ThrowF_t(TRANS("Invalid message while waiting for server session registration"));
    }

    // if client is disconnected
    if (!_cmiComm.Client_IsConnected()) {
      // quit
      ThrowF_t(TRANS("Client disconnected"));
    }
  }
  ThrowF_t(TRANS("Timeout while waiting for server session registration"));
}

void CSessionState::Start_AtClient_t(INDEX ctLocalPlayers)     // throw char *
{
  // send one unreliable packet to server to make the connection up and running
  CNetworkMessage nmKeepAlive(MSG_KEEPALIVE);
  _pNetwork->SendToServer(nmKeepAlive);

  // send registration request
  CNetworkMessage nmRegisterSessionState(MSG_REQ_CONNECTREMOTESESSIONSTATE);
  #define VTAG 0x56544147  // Looks like 'VTAG' in ASCII.
  nmRegisterSessionState<<INDEX(VTAG)<<INDEX(_SE_BUILD_MAJOR)<<INDEX(_SE_BUILD_MINOR);
  nmRegisterSessionState<<_strModName;
  extern CTString net_strConnectPassword;
  extern CTString net_strVIPPassword;
  CTString strPasw = net_strConnectPassword;
  if (strPasw=="") {
    strPasw = net_strVIPPassword;
  }
  nmRegisterSessionState<<strPasw;
  nmRegisterSessionState<<ctLocalPlayers;
  ses_sspParams.Update();
  nmRegisterSessionState<<ses_sspParams;
  _pNetwork->SendToServerReliable(nmRegisterSessionState);

  // prepare file or memory stream for state
  CTFileStream strmStateFile; CTMemoryStream strmStateMem;
  CTStream *pstrmState;
  extern INDEX net_bDumpConnectionInfo;
  if (net_bDumpConnectionInfo) {
    strmStateFile.Create_t(CTString("Temp\\DefaultState.bin"));
    pstrmState = &strmStateFile;
  } else {
    pstrmState = &strmStateMem;
  }

  {
    // wait for server's response
    CTMemoryStream strmMessage;
    WaitStream_t(strmMessage, "reply", MSG_REP_CONNECTREMOTESESSIONSTATE);
    // get motd
    strmMessage>>ses_strMOTD;
    // get info for creating default state
    CTFileName fnmWorld;
    strmMessage>>fnmWorld;
    ULONG ulSpawnFlags;
    strmMessage>>ulSpawnFlags;
    UBYTE aubProperties[NET_MAXSESSIONPROPERTIES];
    strmMessage.Read_t(aubProperties, NET_MAXSESSIONPROPERTIES);
    // create default state
    NET_MakeDefaultState_t(fnmWorld, ulSpawnFlags, aubProperties, *pstrmState);
    pstrmState->SetPos_t(0);
  }

  // send one unreliable packet to server to make the connection up and running
  {CNetworkMessage nmKeepAlive(MSG_KEEPALIVE);
  _pNetwork->SendToServer(nmKeepAlive); }
  // send data request
  CPrintF(TRANSV("Sending statedelta request\n"));
  CNetworkMessage nmRequestDelta(MSG_REQ_STATEDELTA);
  _pNetwork->SendToServerReliable(nmRequestDelta);

  {
    // wait for server's response
    CTMemoryStream strmMessage;
    WaitStream_t(strmMessage, "data", MSG_REP_STATEDELTA);
    // decompress saved session state
    CTMemoryStream strmDelta;
    CzlibCompressor comp;
    comp.UnpackStream_t(strmMessage, strmDelta);
    CTMemoryStream strmNew;
    DIFF_Undiff_t(pstrmState, &strmDelta, &strmNew);
    strmNew.SetPos_t(0);

    // read the initialization information from the stream
    Read_t(&strmNew);
    ses_tmInitializationTick  = -1.0f;
    ses_tmInitializationTick2 = -1.0f;
  }

  // send one unreliable packet to server to make the connection up and running
  {CNetworkMessage nmKeepAlive(MSG_KEEPALIVE);
  _pNetwork->SendToServer(nmKeepAlive); }

  CPrintF(TRANSV("Sending CRC request\n"));
  // send data request
  CNetworkMessage nmRequestCRC(MSG_REQ_CRCLIST);
  _pNetwork->SendToServerReliable(nmRequestCRC);

  {
    // wait for CRC challenge
    CTMemoryStream strmMessage;
    WaitStream_t(strmMessage, "CRC", MSG_REQ_CRCCHECK);

    // make response
    CNetworkMessage nmCRC(MSG_REP_CRCCHECK);
    nmCRC<<CRCT_MakeCRCForFiles_t(strmMessage)<<ses_iLastProcessedSequence;
    _pNetwork->SendToServerReliable(nmCRC);
  }

  // send one unreliable packet to server to make the connection up and running
  {CNetworkMessage nmKeepAlive(MSG_KEEPALIVE);
  _pNetwork->SendToServer(nmKeepAlive); }
}

// notify entities of level change
void CSessionState::SendLevelChangeNotification(CEntityEvent &ee)
{
  // for each entity in the world
  {FOREACHINDYNAMICCONTAINER(_pNetwork->ga_World.wo_cenEntities, CEntity, iten) {
    // if it should be notified
    if (iten->en_ulFlags&ENF_NOTIFYLEVELCHANGE) {
      // send the event
      iten->SendEvent(ee);
    }
  }}
}

// wait for a stream to come from server
void CSessionState::WaitStream_t(CTMemoryStream &strmMessage, const CTString &strName, 
                                 INDEX iMsgCode)
{

  // start waiting for server's response
  SetProgressDescription(TRANS("waiting for ")+strName);
  CallProgressHook_t(0.0f);
  SLONG slReceivedLast = 0;

  // yes, we need the client/server updates in the progres hook
  _bRunNetUpdates = TRUE;
  // repeat until timed out
  for(TIME tmWait=0; tmWait<net_tmConnectionTimeout*1000;
    _pTimer->Sleep(NET_WAITMESSAGE_DELAY), tmWait+=NET_WAITMESSAGE_DELAY) {
    // update network connection sockets
    if (_cmiComm.Client_Update() == FALSE) {
			break;
		}

    // check how much is received so far
    SLONG slExpectedSize; // slReceivedSoFar;
    SLONG slReceivedSize;
    _cmiComm.Client_PeekSize_Reliable(slExpectedSize,slReceivedSize);
    // if nothing received yet
    if (slExpectedSize==0) {
      // progress with waiting
      CallProgressHook_t(tmWait/(net_tmConnectionTimeout*1000));
    // if something received
    } else {
      // if some new data received
      if (slReceivedSize!=slReceivedLast) {
        slReceivedLast = slReceivedSize;
        // reset timeout
        tmWait=0;
      }
      // progress with receiving
      SetProgressDescription(TRANS("receiving ")+strName+"  ");
      CallProgressHook_t((float)slReceivedSize/slExpectedSize);
    }

    // if not everything received yet
    if (!_pNetwork->ReceiveFromServerReliable(strmMessage)) {
      // continue waiting
      continue;
    }
    // read message identifier
    strmMessage.SetPos_t(0);
    INDEX iID;
    strmMessage>>iID;

    // if this is the message
    if (iID == iMsgCode) {
      // all ok
      CallProgressHook_t(1.0f);
      // no more client/server updates in the progres hook
      _bRunNetUpdates = TRUE;
      return;
    // if disconnected
    } else if (iID == MSG_INF_DISCONNECTED) {
			// confirm disconnect
			CNetworkMessage nmConfirmDisconnect(MSG_REP_DISCONNECTED);			
			_pNetwork->SendToServerReliable(nmConfirmDisconnect);
      // report the reason
      CTString strReason;
      strmMessage>>strReason;
      ses_strDisconnected = strReason;
      // no more client/server updates in the progres hook
      _bRunNetUpdates = FALSE;
      ThrowF_t(TRANS("Disconnected: %s\n"), (const char *) strReason);
	  // otherwise
    } else {
      // no more client/server updates in the progres hook
      _bRunNetUpdates = FALSE;
      // it is invalid message
      ThrowF_t(TRANS("Invalid stream while waiting for %s"), (const char *) strName);
    }

    // if client is disconnected
    if (!_cmiComm.Client_IsConnected()) {
      // no more client/server updates in the progres hook
      _bRunNetUpdates = FALSE;
      // quit
      ThrowF_t(TRANS("Client disconnected"));
    }
  }

  // no more client/server updates in the progres hook
  _bRunNetUpdates = FALSE;

//	CNetworkMessage nmConfirmDisconnect(MSG_REP_DISCONNECTED);			
//	_pNetwork->SendToServerReliable(nmConfirmDisconnect);

  
  ThrowF_t(TRANS("Timeout while waiting for %s"), (const char *) strName);
}

// check if disconnected
BOOL CSessionState::IsDisconnected(void)
{
  return ses_strDisconnected!="";
}

// print an incoming chat message to console
void CSessionState::PrintChatMessage(ULONG ulFrom, const CTString &strFrom, const CTString &strMessage)
{
  CTString strSender;
  // if no sender players
  if (ulFrom==0) {
    // take symbolic sender string
    strSender = strFrom;
  // if there are sender players
  } else {
    // for each sender player
    for(INDEX ipl=0; ipl<ses_apltPlayers.Count(); ipl++) {
      CPlayerTarget &plt = ses_apltPlayers[ipl];
      if (plt.IsActive() && (ulFrom & (1UL<<ipl)) ) {
        // add its name to the sender list
        if (strSender!="") {
          strSender+=", ";
        }
        strSender+=plt.plt_penPlayerEntity->GetPlayerName();
      }
    }
  }

  // let eventual script addon process the message
  extern CTString cmd_strChatSender ;
  extern CTString cmd_strChatMessage;
  extern CTString cmd_cmdOnChat;
  cmd_strChatSender = strSender;
  cmd_strChatMessage = strMessage;
  if (cmd_cmdOnChat!="") {
    _pShell->Execute(cmd_cmdOnChat);
  }

  // if proccessing didn't kill it
  if (cmd_strChatSender!="" && cmd_strChatMessage!="") {
    // print the message
    CPrintF("%s: ^o^cFFFFFF%s^r\n", (const char*)cmd_strChatSender, (const char*)cmd_strChatMessage);
  }
  extern INDEX net_ctChatMessages;
  net_ctChatMessages++;
}

/* NOTES:
1) New thinkers might be added by current ones, but it doesn't matter,
since they must be added forward in time and the list is sorted, so they
cannot be processed in this tick.
2) Thinkers/Movers can be removed during iteration, but the CEntityPointer
guarantee that they are not freed from memory.
*/

// do physics for a game tick
void CSessionState::HandleMovers(void)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_HANDLEMOVERS);

//  CPrintF("---- tick %g\n", _pTimer->CurrentTick());

  // put all movers in active list, pushing ones first
  CListHead lhActiveMovers, lhDoneMovers, lhDummyMovers;
  {FORDELETELIST(CMovableEntity, en_lnInMovers, _pNetwork->ga_World.wo_lhMovers, itenMover) {
    CMovableEntity *pen = itenMover;
    pen->en_lnInMovers.Remove();
    // if predicting, and it is not a predictor
    if (ses_bPredicting && !pen->IsPredictor()) {
      // skip it
      lhDummyMovers.AddTail(pen->en_lnInMovers);
      continue;
    }
    if (!(pen->en_ulFlags&ENF_DELETED)) {
      if ((pen->en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_PUSH) {
        lhActiveMovers.AddHead(pen->en_lnInMovers);
      } else {
        lhActiveMovers.AddTail(pen->en_lnInMovers);
      }
    }
  }}

  // for each active mover
  {FORDELETELIST(CMovableEntity, en_lnInMovers, lhActiveMovers, itenMover) {
    // let it clear its temporary variables to prevent bad syncs
    itenMover->ClearMovingTemp();
  }}

  // for each active mover
  {FORDELETELIST(CMovableEntity, en_lnInMovers, lhActiveMovers, itenMover) {
    // let it calculate its wanted parameters for this tick
    itenMover->PreMoving();
  }}

  // while there are some active movers
  while(!lhActiveMovers.IsEmpty()) {
    // get first one
    CMovableEntity *penMoving = LIST_HEAD(lhActiveMovers, CMovableEntity, en_lnInMovers);
    CEntityPointer penCurrent = penMoving;  // just to keep it alive around the loop
    // first move it to done list (if not done, it will move back to world's movers)
    penMoving->en_lnInMovers.Remove();
    lhDoneMovers.AddTail(penMoving->en_lnInMovers);

/*
    CPrintF("**%s(%08x)", 
      penMoving->en_pecClass->ec_pdecDLLClass->dec_strName,
      penMoving->en_ulID);
    if (penMoving->IsPredictable()) {
      CPrintF(" predictable");
    }
    if (penMoving->IsPredictor()) {
      CPrintF(" predictor");
    }
    if (penMoving->IsPredicted()) {
      CPrintF(" predicted");
    }
    if (penMoving->en_penReference!=NULL) {
      CPrintF("reference id%08x", penMoving->en_penReference->en_ulID);
    }
    CPrintF("\n");
*/

    // let it do its own physics
    penMoving->DoMoving();
//    CPrintF("\n");

    // if any mover is re-added, put it to the end of active list
    lhActiveMovers.MoveList(_pNetwork->ga_World.wo_lhMovers);
  }

  // for each done mover
  {FORDELETELIST(CMovableEntity, en_lnInMovers, lhDoneMovers, itenMover) {
    // if predicting, and it is not a predictor
    if (ses_bPredicting && !itenMover->IsPredictor()) {
      // skip it
      continue;
    }
    // let it calculate its parameters after all movement has been resolved
    itenMover->PostMoving();
  }}

  // for each done mover

  {FORDELETELIST(CMovableEntity, en_lnInMovers, lhDoneMovers, itenMover) {
    CMovableEntity *pen = itenMover;
    // if predicting, and it is not a predictor
    if (ses_bPredicting && !itenMover->IsPredictor()) {
      // skip it
      continue;
    }
    // if marked for removing from list of movers
    if (pen->en_ulFlags&ENF_INRENDERING) {
      // remove it
      pen->en_ulFlags&=~ENF_INRENDERING;
      pen->en_lnInMovers.Remove();
    }
    // let it clear its temporary variables to prevent bad syncs
    pen->ClearMovingTemp();
  }}
  
  // return all done movers to the world's list
  _pNetwork->ga_World.wo_lhMovers.MoveList(lhDummyMovers);
  _pNetwork->ga_World.wo_lhMovers.MoveList(lhDoneMovers);

  // handle all the sent events
  CEntity::HandleSentEvents();

  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_HANDLEMOVERS);
}

// do thinking for a game tick
void CSessionState::HandleTimers(TIME tmCurrentTick)
{
#define TIME_EPSILON 0.0001f
  //IFDEBUG(TIME tmLast = 0.0f);

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_HANDLETIMERS);
  // repeat
  CListHead &lhTimers = _pNetwork->ga_World.wo_lhTimers;
  FOREVER {
    // no entity found initially
    CRationalEntity *penTimer = NULL;
    // for each entity in list of timers
    FOREACHINLIST(CRationalEntity, en_lnInTimers, lhTimers, iten) {
      // if due after current time
      if(iten->en_timeTimer>tmCurrentTick+TIME_EPSILON) {
        // stop searching
        break;
      }
      // if now predicting and it is not a predictor
      if (ses_bPredicting && !iten->IsPredictor()) {
        // skip it
        continue;
      }
      // remember found entity, and stop searching
      penTimer = iten;
      break;
    }

    // if no entity is found
    if (penTimer==NULL) {
      // stop
      break;
    }

    // check that timers are propertly handled
    ASSERT(penTimer->en_timeTimer>tmCurrentTick-_pTimer->TickQuantum-TIME_EPSILON);
    //ASSERT(penTimer->en_timeTimer>=tmLast);
    //IFDEBUG(tmLast=penTimer->en_timeTimer);

    // remove the timer from the list
    penTimer->en_timeTimer = THINKTIME_NEVER;
    penTimer->en_lnInTimers.Remove();
    // send timer event to the entity
    penTimer->SendEvent(ETimer());
  }

  // handle all the sent events
  CEntity::HandleSentEvents();
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_HANDLETIMERS);
}

// do a warm-up run of the world for a few ticks
void CSessionState::WarmUpWorld(void)
{
#define WORLD_WORMUP_COUNT 20   // run 20 ticks to get stable
  ses_tmLastProcessedTick = _pNetwork->ga_srvServer.srv_tmLastProcessedTick = 0;
  ses_iLastProcessedSequence = _pNetwork->ga_srvServer.srv_iLastProcessedSequence = -1;
  // add a few empty all-action messages to the game stream
  for (INDEX iTick=0; iTick<WORLD_WORMUP_COUNT; iTick++) {
    _pNetwork->ga_srvServer.srv_tmLastProcessedTick += _pTimer->TickQuantum;
    _pNetwork->ga_srvServer.srv_iLastProcessedSequence++;
    CNetworkStreamBlock nsbAllActions(MSG_SEQ_ALLACTIONS, _pNetwork->ga_srvServer.srv_iLastProcessedSequence);
    nsbAllActions<<_pNetwork->ga_srvServer.srv_tmLastProcessedTick;
    nsbAllActions.Rewind();
    ses_nsGameStream.AddBlock(nsbAllActions);
  }

  // process the blocks
  ProcessGameStream();
}

// create a checksum value for sync-check
void CSessionState::ChecksumForSync(ULONG &ulCRC, INDEX iExtensiveSyncCheck)
{
  CRC_AddLONG(ulCRC, ses_iLastProcessedSequence);
  CRC_AddLONG(ulCRC, ses_iLevel);
  CRC_AddLONG(ulCRC, ses_bPause);
  if (iExtensiveSyncCheck>0) {
    CRC_AddLONG(ulCRC, ses_bGameFinished);
    CRC_AddLONG(ulCRC, ses_ulRandomSeed);
  }

  // if all entities should be synced
  if (iExtensiveSyncCheck>1) {
    // for each active entity in the world
    {FOREACHINDYNAMICCONTAINER(_pNetwork->ga_World.wo_cenEntities, CEntity, iten) {
      if (iten->IsPredictor()) {
        continue;
      }
      iten->ChecksumForSync(ulCRC, iExtensiveSyncCheck);
    }}
    // for each entity in the world
    {FOREACHINDYNAMICCONTAINER(_pNetwork->ga_World.wo_cenAllEntities, CEntity, iten) {
      if (iten->IsPredictor()) {
        continue;
      }
      iten->ChecksumForSync(ulCRC, iExtensiveSyncCheck);
    }}
  }

  if (iExtensiveSyncCheck>0) {
    // checksum all movers
    {FOREACHINLIST(CMovableEntity, en_lnInMovers, _pNetwork->ga_World.wo_lhMovers, iten) {
      if (iten->IsPredictor()) {
        continue;
      }
      iten->ChecksumForSync(ulCRC, iExtensiveSyncCheck);
    }}
  }
  // checksum all active players
  {FOREACHINSTATICARRAY(ses_apltPlayers, CPlayerTarget, itclt) {
    CPlayerTarget &clt = *itclt;
    if (clt.IsActive()) {
      clt.plt_paPreLastAction.ChecksumForSync(ulCRC);
      clt.plt_paLastAction.ChecksumForSync(ulCRC);
      clt.plt_penPlayerEntity->ChecksumForSync(ulCRC, iExtensiveSyncCheck);
    }
  }}
}

// dump sync data to text file
void CSessionState::DumpSync_t(CTStream &strm, INDEX iExtensiveSyncCheck)  // throw char *
{
  strm.FPrintF_t("Level: %d\n", ses_iLevel);
  strm.FPrintF_t("Sequence: %d\n", ses_iLastProcessedSequence);
  strm.FPrintF_t("Tick: %g\n", ses_tmLastProcessedTick);
  strm.FPrintF_t("Paused: %d\n", ses_bPause);
  if (iExtensiveSyncCheck>0) {
    strm.FPrintF_t("Finished: %d\n", ses_bGameFinished);
    strm.FPrintF_t("Random seed: 0x%08x\n", ses_ulRandomSeed);
  }

  _pNetwork->ga_World.LockAll();

  strm.FPrintF_t("\n\n======================== players:\n");
  // dump all active players
  {FOREACHINSTATICARRAY(ses_apltPlayers, CPlayerTarget, itclt) {
    CPlayerTarget &clt = *itclt;
    if (clt.IsActive()) {
      clt.plt_penPlayerEntity->DumpSync_t(strm, iExtensiveSyncCheck);
      strm.FPrintF_t("\n -- action:\n");
      clt.plt_paPreLastAction.DumpSync_t(strm);
      clt.plt_paLastAction.DumpSync_t(strm);
    }
  }}

  if (iExtensiveSyncCheck>0) {
    strm.FPrintF_t("\n\n======================== movers:\n");
    // dump all movers
    {FOREACHINLIST(CMovableEntity, en_lnInMovers, _pNetwork->ga_World.wo_lhMovers, iten) {
      if (iten->IsPredictor()) {
        continue;
      }
      iten->DumpSync_t(strm, iExtensiveSyncCheck);
    }}
  }

  // if all entities should be synced
  if (iExtensiveSyncCheck>1) {
    strm.FPrintF_t("\n\n======================== active entities (%d):\n",
      _pNetwork->ga_World.wo_cenEntities.Count());
    // for each entity in the world
    {FOREACHINDYNAMICCONTAINER(_pNetwork->ga_World.wo_cenEntities, CEntity, iten) {
      if (iten->IsPredictor()) {
        continue;
      }
      iten->DumpSync_t(strm, iExtensiveSyncCheck);
    }}
    strm.FPrintF_t("\n\n======================== all entities (%d):\n",
      _pNetwork->ga_World.wo_cenEntities.Count());
    // for each entity in the world
    {FOREACHINDYNAMICCONTAINER(_pNetwork->ga_World.wo_cenAllEntities, CEntity, iten) {
      if (iten->IsPredictor()) {
        continue;
      }
      iten->DumpSync_t(strm, iExtensiveSyncCheck);
    }}
  }

  _pNetwork->ga_World.UnlockAll();
}

#if DEBUG_SYNCSTREAMDUMPING
/* 
 * Obtain valid session dump memory stream
 */
CTMemoryStream *GetDumpStream(void)
{
  if( _pNetwork->ga_sesSessionState.ses_pstrm == NULL)
  {
    _pNetwork->ga_sesSessionState.ses_pstrm = new CTMemoryStream;
  }
  return _pNetwork->ga_sesSessionState.ses_pstrm;
}

void ClearDumpStream(void)
{
  if( _pNetwork->ga_sesSessionState.ses_pstrm != NULL)
  {
    delete _pNetwork->ga_sesSessionState.ses_pstrm;
    _pNetwork->ga_sesSessionState.ses_pstrm = NULL;
  }
}
#endif

/*
 * Process a game tick.
 */

extern INDEX cli_bEmulateDesync;
void CSessionState::ProcessGameTick(CNetworkMessage &nmMessage, TIME tmCurrentTick)
{
  ses_tmLastPredictionProcessed = -1;

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_PROCESSGAMETICK);
  ASSERT(this!=NULL);

#if DEBUG_SYNCSTREAMDUMPING
  try
  {
    CTMemoryStream *pstrm = GetDumpStream();
    pstrm->FPrintF_t("Time tick: %.2f\n", tmCurrentTick);
  }
  catch (const char *strError)
  {
    CPrintF("Cannot dump sync data: %s\n", strError);
  }
#endif
  //CPrintF("normal: %.2f\n", tmCurrentTick);

  // FPU must be in 24-bit mode
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  // copy the tick to process into tick used for all tasks
  _pTimer->SetCurrentTick(tmCurrentTick);
  _pfNetworkProfile.IncrementAveragingCounter();
  _pfPhysicsProfile.IncrementAveragingCounter();

  // random is allowed only here, during entity ai
  ses_bAllowRandom = TRUE;

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_APPLYACTIONS);
  // for all clients
  //INDEX iClient = 0;
  FOREACHINSTATICARRAY(ses_apltPlayers, CPlayerTarget, itplt) {
    // if client is active
    if (itplt->IsActive()) {
// player indices transmission is unneccessary unless if debugging
//      // check that it is present in message
//      INDEX iClientInMessage;
//      nmMessage>>iClientInMessage;     // client index
//      ASSERT(iClient==iClientInMessage);
      // read the action
      CPlayerAction paAction;
      nmMessage>>paAction;
      // apply the action
      itplt->ApplyActionPacket(paAction);

      // desync emulation!
      if( cli_bEmulateDesync) {
        itplt->plt_penPlayerEntity->SetHealth(1.0f);
      }
    }
    //iClient++;
  }
  cli_bEmulateDesync = FALSE;

  // handle all the sent events
  CEntity::HandleSentEvents();
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_APPLYACTIONS);

  // do thinking
  HandleTimers(tmCurrentTick);
  // do physics
  HandleMovers();

  // notify all entities of level change as needed
  if (_lphCurrent==LCP_INITIATED) {
    EPreLevelChange ePreChange;
    ePreChange.iUserData = _pNetwork->ga_iNextLevelUserData;
    SendLevelChangeNotification(ePreChange);
    CEntity::HandleSentEvents();
    _lphCurrent=LCP_SIGNALLED;
  }
  if (_lphCurrent==LCP_CHANGED) {
    EPostLevelChange ePostChange;
    ePostChange.iUserData = _pNetwork->ga_iNextLevelUserData;
    SendLevelChangeNotification(ePostChange);
    CEntity::HandleSentEvents();
    _lphCurrent=LCP_NOCHANGE;
  }

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_WORLDBASETICK);
  // let the worldbase execute its tick function
  if (_pNetwork->ga_World.wo_pecWorldBaseClass!=NULL
    &&_pNetwork->ga_World.wo_pecWorldBaseClass->ec_pdecDLLClass!=NULL
    &&_pNetwork->ga_World.wo_pecWorldBaseClass->ec_pdecDLLClass->dec_OnWorldTick!=NULL) {
    _pNetwork->ga_World.wo_pecWorldBaseClass->ec_pdecDLLClass->
      dec_OnWorldTick(&_pNetwork->ga_World);
  }
  // handle all the sent events
  CEntity::HandleSentEvents();
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_WORLDBASETICK);

  // make sync-check and send to server if needed
  MakeSynchronisationCheck();

#if DEBUG_SYNCSTREAMDUMPING
  extern INDEX cli_bDumpSyncEachTick;
  if( cli_bDumpSyncEachTick)
  {
    DumpSyncToMemory();
  }
#endif
  ses_bAllowRandom = FALSE;

  ses_tmPredictionHeadTick = Max(ses_tmPredictionHeadTick, tmCurrentTick);

  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_PROCESSGAMETICK);

  // assure that FPU precision was low all the rendering time
  ASSERT( GetFPUPrecision()==FPT_24BIT);

  // let eventual script do something on each tick
  extern FLOAT cmd_tmTick;
  extern CTString cmd_cmdOnTick;
  if (cmd_cmdOnTick!="") {
    cmd_tmTick = tmCurrentTick;
    _pShell->Execute(cmd_cmdOnTick);
  }
}

/* Process a predicted game tick. */
void CSessionState::ProcessPredictedGameTick(INDEX iPredictionStep, FLOAT fFactor, TIME tmCurrentTick)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_PROCESSGAMETICK);
  ASSERT(this!=NULL);

  //CPrintF("predicted: %.2f\n", tmCurrentTick);

  // FPU must be in 24-bit mode
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  // now predicting
  ses_bPredicting = TRUE;

  // copy the tick to process into tick used for all tasks
  _pTimer->SetCurrentTick(tmCurrentTick);
  _pfNetworkProfile.IncrementAveragingCounter();
  _pfPhysicsProfile.IncrementAveragingCounter();

  // random is allowed only here, during entity ai
  ses_bAllowRandom = TRUE;

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_APPLYACTIONS);
  // for all clients
  FOREACHINSTATICARRAY(ses_apltPlayers, CPlayerTarget, itplt) {
    // if client is active
    if (itplt->IsActive()) {
      // apply the predicted action
      itplt->ApplyPredictedAction(iPredictionStep, fFactor);
    }
  }

  // handle all the sent events
  CEntity::HandleSentEvents();
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_APPLYACTIONS);

  // do thinking
  HandleTimers(tmCurrentTick);
  // do physics
  HandleMovers();

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_WORLDBASETICK);

  // handle all the sent events
  CEntity::HandleSentEvents();
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_WORLDBASETICK);

  ses_bAllowRandom = FALSE;

  // not predicting any more
  ses_bPredicting = FALSE;

  ses_tmPredictionHeadTick = Max(ses_tmPredictionHeadTick, tmCurrentTick);

  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_PROCESSGAMETICK);

  // assure that FPU precision was low all the rendering time
  ASSERT( GetFPUPrecision()==FPT_24BIT);
}

/*
 * Process all eventual available gamestream blocks.
 */
void CSessionState::ProcessGameStream(void)
{
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_SESSIONSTATE_PROCESSGAMESTREAM);

  // must be in 24bit mode when managing entities
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  TIME tmDemoNow = _pNetwork->ga_fDemoTimer;
  // if playing a demo
  if (_pNetwork->ga_bDemoPlay) {
    // find how much ticks to step by now
    if (ses_tmLastDemoSequence<0.0f) {
      ses_tmLastDemoSequence=tmDemoNow;
    }
    // if it is finished
    if (_pNetwork->ga_bDemoPlayFinished) {
      _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SESSIONSTATE_PROCESSGAMESTREAM);
      // do nothing
      return;
    }
  }

  // repeat
  FOREVER {
    // if playing a demo
    if (_pNetwork->ga_bDemoPlay) {
      // if finished for this pass
      if (ses_tmLastDemoSequence>=tmDemoNow) {
        _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SESSIONSTATE_PROCESSGAMESTREAM);
        // end the loop
        return;
      }
      // try to
      try {
        // read a stream block from the demo
        CChunkID cid = _pNetwork->ga_strmDemoPlay.PeekID_t();
        if (cid == CChunkID("DTCK")) {
          _pNetwork->ga_strmDemoPlay.ExpectID_t("DTCK");// demo tick
          CNetworkStreamBlock nsbBlock;
          nsbBlock.Read_t(_pNetwork->ga_strmDemoPlay);
          ses_nsGameStream.AddBlock(nsbBlock);
          _pNetwork->ga_bDemoPlayFinished = FALSE;
        } else {
          _pNetwork->ga_strmDemoPlay.ExpectID_t("DEND"); // demo end
          _pNetwork->ga_bDemoPlayFinished = TRUE;
          _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SESSIONSTATE_PROCESSGAMESTREAM);
          return;
        }
      // if not successful
      } catch (const char *strError) {
        // report error
        CPrintF(TRANSV("Error while playing demo: %s"), strError);
        _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SESSIONSTATE_PROCESSGAMESTREAM);
        return;
      }
    }

    // calculate index of next expected sequence
    INDEX iSequence = ses_iLastProcessedSequence+1;
    // get the stream block with that sequence
    CNetworkStreamBlock *pnsbBlock;
    CNetworkStream::Result res = ses_nsGameStream.GetBlockBySequence(iSequence, pnsbBlock);
    // if it is found
    if (res==CNetworkStream::R_OK) {
      // if recording a demo
      if (_pNetwork->ga_bDemoRec) {
        // try to
        try {
          // write the stream block to the demo
          _pNetwork->ga_strmDemoRec.WriteID_t("DTCK");
          pnsbBlock->Write_t(_pNetwork->ga_strmDemoRec);
        // if not successful
        } catch (const char *strError) {
          // report error
          CPrintF(TRANSV("Error while recording demo: %s"), strError);
          // stop recording
          _pNetwork->StopDemoRec();
        }
      }

      // remember the message type
      int iMsgType=pnsbBlock->GetType();
      // remember the processed sequence
      ses_iLastProcessedSequence = iSequence;
      // process the stream block
      ProcessGameStreamBlock(*pnsbBlock);
      // remove the block from the stream
      pnsbBlock->RemoveFromStream();
      delete pnsbBlock;
      // remove eventual resent blocks that have already been processed
      ses_nsGameStream.RemoveOlderBlocksBySequence(ses_iLastProcessedSequence-2);

      // if the message is all actions
      if (iMsgType==MSG_SEQ_ALLACTIONS) {
        // if playing a demo
        if (_pNetwork->ga_bDemoPlay) {
          // step demo sequence
          ses_tmLastDemoSequence+=_pTimer->TickQuantum;
        }
      }

    // if it is not avaliable yet
    } else if (res==CNetworkStream::R_BLOCKNOTRECEIVEDYET) {
      // finish
      _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SESSIONSTATE_PROCESSGAMESTREAM);
      return;
    // if it is missing
    } else if (res==CNetworkStream::R_BLOCKMISSING) {
      
      // if it is a new sequence
      if (iSequence>ses_iMissingSequence) {
        ses_iMissingSequence = iSequence;
        // setup timeout
        ses_tvResendTime = _pTimer->GetHighPrecisionTimer();
        ses_tmResendTimeout = 0.1f;
      }

      // if timeout occured
      if (_pTimer->GetHighPrecisionTimer()>ses_tvResendTime+CTimerValue(ses_tmResendTimeout)) {

        _pNetwork->AddNetGraphValue(NGET_MISSING, 1.0f); // missing sequence

        // find how many are missing
        INDEX iNextValid = ses_nsGameStream.GetOldestSequenceAfter(iSequence);
        INDEX ctSequences = Max(iNextValid-iSequence, INDEX(1));

          // create a request for resending the missing packet
        CNetworkMessage nmResendRequest(MSG_REQUESTGAMESTREAMRESEND);
        nmResendRequest<<iSequence;
        nmResendRequest<<ctSequences;
        // send the request to the server
        _pNetwork->SendToServer(nmResendRequest);

        extern INDEX net_bReportMiscErrors;
        if (net_bReportMiscErrors) {
          CPrintF(TRANSV("Session State: Missing sequences %d-%d(%d) timeout %g\n"), 
            iSequence, iSequence+ctSequences-1, ctSequences, ses_tmResendTimeout);
        }

        // increase the timeout
        ses_tvResendTime = _pTimer->GetHighPrecisionTimer();
        ses_tmResendTimeout *= 2.0f;
      }

      // finish
      _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SESSIONSTATE_PROCESSGAMESTREAM);
      return;
    }
  }
}

// flush prediction actions that were already processed
void CSessionState::FlushProcessedPredictions(void)
{
  // for all clients
  FOREACHINSTATICARRAY(ses_apltPlayers, CPlayerTarget, itplt) {
    // flush
    itplt->FlushProcessedPredictions();
  }
}

/* Find out how many prediction steps are currently pending. */
INDEX CSessionState::GetPredictionStepsCount(void)
{
  // start with no prediction
  INDEX ctPredictionSteps = 0;
  // for all clients
  FOREACHINSTATICARRAY(ses_apltPlayers, CPlayerTarget, itplt) {
    // update maximum number of possible predictions
    ctPredictionSteps = Max(ctPredictionSteps, itplt->GetNumberOfPredictions());
  }

  return ctPredictionSteps;
}

/* Process all eventual available prediction actions. */
void CSessionState::ProcessPrediction(void)
{
  // FPU must be in 24-bit mode
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  // get number of steps that could be predicted
  INDEX ctSteps = GetPredictionStepsCount();

  // limit prediction
  extern INDEX cli_iMaxPredictionSteps;
  ctSteps = ClampUp(ctSteps, cli_iMaxPredictionSteps);

  // if none
  if(ctSteps<=0) {
    // do nothing
    return;
  }

  // if this would not result in any new tick predicted
  TIME tmNow = ses_tmLastProcessedTick+ctSteps*_pTimer->TickQuantum;
  if (Abs(ses_tmLastPredictionProcessed-tmNow)<_pTimer->TickQuantum/10.0f) {
    // do nothing
    return;
  }
  // remeber what was predicted now
  ses_tmLastPredictionProcessed = tmNow;

  // remember random seed and entity ID
  ULONG ulOldRandom = ses_ulRandomSeed;
  ULONG ulEntityID = _pNetwork->ga_World.wo_ulNextEntityID;

  // delete all predictors (if any left from last time)
  _pNetwork->ga_World.DeletePredictors();
  // create new predictors
  _pNetwork->ga_World.CreatePredictors();

  // for each step
  TIME tmPredictedTick = ses_tmLastProcessedTick;
  for(INDEX iPredictionStep=0; iPredictionStep<ctSteps; iPredictionStep++) {
    tmPredictedTick+=_pTimer->TickQuantum;
    //ses_tmPredictionHeadTick = Max(ses_tmPredictionHeadTick, tmPredictedTick);
    // predict it
    ProcessPredictedGameTick(iPredictionStep, FLOAT(iPredictionStep)/ctSteps, tmPredictedTick);
  }
  // restore random seed and entity ID
  ses_ulRandomSeed = ulOldRandom;
  _pNetwork->ga_World.wo_ulNextEntityID = ulEntityID;
}

/*
 * Process a gamestream block.
 */
void CSessionState::ProcessGameStreamBlock(CNetworkMessage &nmMessage)
{
  // copy the tick to process into tick used for all tasks
  _pTimer->SetCurrentTick(ses_tmLastProcessedTick);

  // check message type
  switch (nmMessage.GetType()) {
  // if adding a new player
  case MSG_SEQ_ADDPLAYER: {
      _pNetwork->AddNetGraphValue(NGET_NONACTION, 1.0f); // non-action sequence
      INDEX iNewPlayer;
      CPlayerCharacter pcCharacter;
      nmMessage>>iNewPlayer;      // player index
      nmMessage>>pcCharacter;     // player character

      // delete all predictors
      _pNetwork->ga_World.DeletePredictors();

      // activate the player
      ses_apltPlayers[iNewPlayer].Activate();

      // if there is no entity with that character in the world
      CPlayerEntity *penNewPlayer = _pNetwork->ga_World.FindEntityWithCharacter(pcCharacter);
      if (penNewPlayer==NULL) {
        // create an entity for it
        CPlacement3D pl(FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
        try {
          CTFileName fnmPlayer = CTString("Classes\\Player.ecl"); // this must not be a dependency!
          penNewPlayer = (CPlayerEntity*)(_pNetwork->ga_World.CreateEntity_t(pl, fnmPlayer));
          // attach entity to client data
          ses_apltPlayers[iNewPlayer].AttachEntity(penNewPlayer);
          // attach the character to it
          penNewPlayer->en_pcCharacter = pcCharacter;
          // prepare the entity
          penNewPlayer->Initialize();
        } catch (const char *strError) {
          (void)strError;
          FatalError(TRANS("Cannot load Player class:\n%s"), strError);
        }
        if (!_pNetwork->IsPlayerLocal(penNewPlayer)) {
          CPrintF(TRANSV("%s joined\n"), (const char *) penNewPlayer->GetPlayerName());
        }
      } else {
        // attach entity to client data
        ses_apltPlayers[iNewPlayer].AttachEntity(penNewPlayer);
        // make it update its character
        penNewPlayer->CharacterChanged(pcCharacter);

        if (!_pNetwork->IsPlayerLocal(penNewPlayer)) {
          CPrintF(TRANSV("%s rejoined\n"), (const char *) penNewPlayer->GetPlayerName());
        }
      }

    } break;
  // if removing a player
  case MSG_SEQ_REMPLAYER: {
      _pNetwork->AddNetGraphValue(NGET_NONACTION, 1.0f); // non-action sequence
      INDEX iPlayer;
      nmMessage>>iPlayer;      // player index

      // delete all predictors
      _pNetwork->ga_World.DeletePredictors();

      // inform entity of disconnnection
      CPrintF(TRANSV("%s left\n"), (const char *) ses_apltPlayers[iPlayer].plt_penPlayerEntity->GetPlayerName());
      ses_apltPlayers[iPlayer].plt_penPlayerEntity->Disconnect();
      // deactivate the player
      ses_apltPlayers[iPlayer].Deactivate();
      // handle all the sent events
      ses_bAllowRandom = TRUE;
      CEntity::HandleSentEvents();
      ses_bAllowRandom = FALSE;

    } break;

  // if changing character
  case MSG_SEQ_CHARACTERCHANGE: {
      _pNetwork->AddNetGraphValue(NGET_NONACTION, 1.0f); // non-action sequence
      INDEX iPlayer;
      CPlayerCharacter pcCharacter;
      nmMessage>>iPlayer>>pcCharacter;

      // delete all predictors
      _pNetwork->ga_World.DeletePredictors();

      // change the character
      ses_apltPlayers[iPlayer].plt_penPlayerEntity->CharacterChanged(pcCharacter);

      // handle all the sent events
      ses_bAllowRandom = TRUE;
      CEntity::HandleSentEvents();
      ses_bAllowRandom = FALSE;

                                } break;
  // if receiving client actions
  case MSG_SEQ_ALLACTIONS: {
      // read time from packet
      TIME tmPacket;
      nmMessage>>tmPacket;    // packet time

      // time must be greater by one than that on the last packet received
      TIME tmTickQuantum = _pTimer->TickQuantum;
      TIME tmPacketDelta = tmPacket-ses_tmLastProcessedTick;
      if(! (Abs(tmPacketDelta-tmTickQuantum) < (tmTickQuantum/10.0f)) ) {
        // report debug info
        CPrintF(
          TRANS("Session state: Mistimed MSG_ALLACTIONS: Last received tick %g, this tick %g\n"),
          ses_tmLastProcessedTick, tmPacket);
      }
      // remember the received tick
      ses_tmLastProcessedTick = tmPacket;

      // NOTE: if we got a tick, it means that all players have joined
      // don't wait for new players any more
      ses_bWaitAllPlayers = FALSE;

      // delete all predictors
      _pNetwork->ga_World.DeletePredictors();
      // process the tick
      ProcessGameTick(nmMessage, tmPacket);

    } break;

  // if receiving pause message
  case MSG_SEQ_PAUSE: {
    _pNetwork->AddNetGraphValue(NGET_NONACTION, 1.0f); // non-action sequence

    // delete all predictors
    _pNetwork->ga_World.DeletePredictors();

    BOOL bPauseBefore = ses_bPause;
    // read the pause state and pauser from it
    nmMessage>>(INDEX&)ses_bPause;
    CTString strPauser;
    nmMessage>>strPauser;
    // if paused by some other machine
    if (strPauser!=TRANS("Local machine")) {
      // report who paused
      if (ses_bPause!=bPauseBefore) {
        if (ses_bPause) {
          CPrintF(TRANSV("Paused by '%s'\n"), (const char *) strPauser);
        } else {
          CPrintF(TRANSV("Unpaused by '%s'\n"), (const char *) strPauser);
        }
      }
    }
    // must keep wanting current state
    ses_bWantPause = ses_bPause;
                          } break;
  // otherwise
  default:
    // error
    ASSERT(FALSE);
  }
}

// Set lerping factor for current frame.
void CSessionState::SetLerpFactor(CTimerValue tvNow)
{
  // if no lerping
  if (!net_bLerping) {
    // set lerping factor without lerping
    _pTimer->SetLerp(1.0f);
    _pTimer->SetCurrentTick(ses_tmPredictionHeadTick);
    return;
  }
  
  FLOAT fFactor  = 0.0f;
  FLOAT fFactor2 = 0.0f;
  
  // ---- primary factor - used for prediction
  {
    TIME tmLastTick = ses_tmPredictionHeadTick;

    // if lerping was never set before
    if (ses_tmInitializationTick<0) {
      // initialize it
      ses_tvInitialization = tvNow;
      ses_tmInitializationTick = tmLastTick;
    }

    // get passed time from session state starting in precise time and in ticks
    FLOAT tmRealDelta = FLOAT((tvNow-ses_tvInitialization).GetSeconds())
      *_pNetwork->ga_fGameRealTimeFactor*_pNetwork->ga_sesSessionState.ses_fRealTimeFactor;
    FLOAT tmTickDelta = tmLastTick-ses_tmInitializationTick;
    // calculate factor
    fFactor = 1.0f-(tmTickDelta-tmRealDelta)/_pTimer->TickQuantum;

    // if the factor starts getting below zero
    if (fFactor<0) {
      //CPrintF("Lerp=%.2f <0 @ %.2fs\n", fFactor, tmLastTick);
      // clamp it
      fFactor = 0.0f;
      // readjust timers so that it gets better
      ses_tvInitialization = tvNow;
      ses_tmInitializationTick = tmLastTick-_pTimer->TickQuantum;
    }
    if (fFactor>1) {
      //CPrintF("Lerp=%.2f >1 @ %.2fs\n", fFactor, tmLastTick);
      // clamp it
      fFactor = 1.0f;
      // readjust timers so that it gets better
      ses_tvInitialization = tvNow;
      ses_tmInitializationTick = tmLastTick;
    }

    #if DEBUG_LERPING

      avfStats[ctCounter][0] = tmRealDelta/_pTimer->TickQuantum;
      avfStats[ctCounter][1] = tmTickDelta/_pTimer->TickQuantum;
      avfStats[ctCounter][2] = fFactor;
      avfStats[ctCounter][3] = (tmLastTick/_pTimer->TickQuantum-1.0f)+fFactor;
      ctCounter++;
      if (ctCounter>=ctMax) {
        ctCounter = 0;
      }
    #endif // DEBUG_LERPING
  }

  // ---- secondary factor - used for non-predicted movement
  {
    TIME tmLastTick = ses_tmLastProcessedTick;
    // if lerping was never set before
    if (ses_tmInitializationTick2<0) {
      // initialize it
      ses_tvInitialization2 = tvNow;
      ses_tmInitializationTick2 = tmLastTick;
    }

    // get passed time from session state starting in precise time and in ticks
    FLOAT tmRealDelta = FLOAT((tvNow-ses_tvInitialization2).GetSeconds())
      *_pNetwork->ga_fGameRealTimeFactor*_pNetwork->ga_sesSessionState.ses_fRealTimeFactor;
    FLOAT tmTickDelta = tmLastTick-ses_tmInitializationTick2;
    // calculate factor
    fFactor2 = 1.0f-(tmTickDelta-tmRealDelta)/_pTimer->TickQuantum;

    // if the factor starts getting below zero
    if (fFactor2<0) {
      //CPrintF("Lerp2=%.2f <0 @ %.2fs\n", fFactor2, tmLastTick);
      // clamp it
      fFactor2 = 0.0f;
      // readjust timers so that it gets better
      ses_tvInitialization2 = tvNow;
      ses_tmInitializationTick2 = tmLastTick-_pTimer->TickQuantum;
    }
    if (fFactor2>1) {
      //CPrintF("Lerp2=%.2f >1 @ %.2fs\n", fFactor2, tmLastTick);
      // clamp it
      fFactor2 = 1.0f;
      // readjust timers so that it gets better
      ses_tvInitialization2 = tvNow;
      ses_tmInitializationTick2 = tmLastTick;
    }
  }

  // set lerping factor2
  _pTimer->SetLerp(fFactor);
  _pTimer->SetLerp2(fFactor2);
  _pTimer->SetCurrentTick(ses_tmPredictionHeadTick);
}

/*
 * Read session state state from a stream.
 */
void CSessionState::Read_t(CTStream *pstr)  // throw char *
{
#if DEBUG_SYNCSTREAMDUMPING
  CPrintF( "Session state read: Sequence %d, Time %g\n", ses_iLastProcessedSequence, ses_tmLastProcessedTick);
#endif
  // read time information and random seed

  INDEX iVersion = SESSIONSTATEVERSION_OLD;
  if (pstr->PeekID_t()==CChunkID("SESV")) {
    pstr->ExpectID_t("SESV");
    (*pstr)>>iVersion;
  }
  (*pstr)>>ses_tmLastProcessedTick;
  (*pstr)>>ses_iLastProcessedSequence;
  (*pstr)>>ses_iLevel;
  (*pstr)>>ses_ulRandomSeed;
  (*pstr)>>ses_ulSpawnFlags;
  (*pstr)>>ses_tmSyncCheckFrequency;
  (*pstr)>>ses_iExtensiveSyncCheck;
  (*pstr)>>ses_tmLastSyncCheck;
  (*pstr)>>ses_ctMaxPlayers;
  (*pstr)>>ses_bWaitAllPlayers;
  (*pstr)>>ses_bPause;
  (*pstr)>>ses_bGameFinished;
  if (iVersion>=SESSIONSTATEVERSION_WITHBULLETTIME) {
    (*pstr)>>ses_fRealTimeFactor;
  }
  ses_bWaitingForServer = FALSE;
  ses_bWantPause = ses_bPause;
  ses_strDisconnected = "";
  _pTimer->SetCurrentTick(ses_tmLastProcessedTick);

  // read session properties from stream
  (*pstr)>>_pNetwork->ga_strSessionName;
  pstr->Read_t(_pNetwork->ga_aubProperties, NET_MAXSESSIONPROPERTIES);

  // read world and its state
  ReadWorldAndState_t(pstr);

#if DEBUG_SYNCSTREAMDUMPING
  ClearDumpStream();

  // dump sync data if needed
  extern INDEX cli_bDumpSyncEachTick;
  if( cli_bDumpSyncEachTick)
  {
    DumpSyncToMemory();
  }
#endif
}

void CSessionState::ReadWorldAndState_t(CTStream *pstr)   // throw char *
{
  // check engine build disallowing reinit
  BOOL bNeedsReinit;
  _pNetwork->CheckVersion_t(*pstr, FALSE, bNeedsReinit);
  ASSERT(!bNeedsReinit);

  // read world filename from stream
  (*pstr)>>_pNetwork->ga_fnmWorld;

  //#### Note:removed check FileTimeStamp for Demo
  /*
  if (CTFileName(pstr->GetDescription()).FileExt()==".dem" &&
    GetFileTimeStamp_t(pstr->GetDescription())<=GetFileTimeStamp_t(_pNetwork->ga_fnmWorld)) {
    ThrowF_t(
      TRANS("Cannot play demo because file '%s'\n"
      "is older than file '%s'!\n"),
      (const char *) CTString(pstr->GetDescription()),
      (const char *) CTString(_pNetwork->ga_fnmWorld));
  }*/

  // prepare the world for loading
  _pNetwork->ga_World.DeletePredictors();
  _pNetwork->ga_World.Clear();
  _pNetwork->ga_World.LockAll();
  // load the world brushes from the world file
  _pNetwork->ga_World.LoadBrushes_t(_pNetwork->ga_fnmWorld);
  // read world situation
  _pNetwork->ga_World.ReadState_t(pstr);

  // create an empty list for relinking timers
  CListHead lhNewTimers;
  // read number of entities in timer list
  pstr->ExpectID_t("TMRS");   // timers
  INDEX ctTimers;
  *pstr>>ctTimers;
//  ASSERT(ctTimers == _pNetwork->ga_World.wo_lhTimers.Count());
  // for each entity in the timer list
  {for(INDEX ienTimer=0; ienTimer<ctTimers; ienTimer++) {
    // read its index in container of all entities
    INDEX ien;
    *pstr>>ien;
    // get the entity
    CRationalEntity *pen = (CRationalEntity*)_pNetwork->ga_World.EntityFromID(ien);
    // remove it from the timer list and add it at the end of the new timer list
    if (pen->en_lnInTimers.IsLinked()) {
      pen->en_lnInTimers.Remove();
      lhNewTimers.AddTail(pen->en_lnInTimers);
    }
  }}
  // use the new timer list instead the old one
  ASSERT(_pNetwork->ga_World.wo_lhTimers.IsEmpty());
  _pNetwork->ga_World.wo_lhTimers.MoveList(lhNewTimers);

  // create an empty list for relinking movers
  CListHead lhNewMovers;
  // read number of entities in mover list
  pstr->ExpectID_t("MVRS");   // movers
  INDEX ctMovers;
  *pstr>>ctMovers;
  ASSERT(ctMovers == _pNetwork->ga_World.wo_lhMovers.Count());
  // for each entity in the mover list
  {for(INDEX ienMover=0; ienMover<ctMovers; ienMover++) {
    // read its index in container of all entities
    INDEX ien;
    *pstr>>ien;
    // get the entity
    CMovableEntity *pen = (CMovableEntity*)_pNetwork->ga_World.EntityFromID(ien);
    // remove it from the mover list and add it at the end of the new mover list
    if (pen->en_lnInMovers.IsLinked()) {
      pen->en_lnInMovers.Remove();
    }
    lhNewMovers.AddTail(pen->en_lnInMovers);
  }}
  // use the new mover list instead the old one
  ASSERT(_pNetwork->ga_World.wo_lhMovers.IsEmpty());
  _pNetwork->ga_World.wo_lhMovers.MoveList(lhNewMovers);

  // read number of players
  INDEX ctPlayers;
  (*pstr)>>ctPlayers;
  ASSERT(ctPlayers==ses_apltPlayers.Count());
  // for all clients
  FOREACHINSTATICARRAY(ses_apltPlayers, CPlayerTarget, itclt) {
    // read from stream
    itclt->Read_t(pstr);
  }

  _pNetwork->ga_World.UnlockAll();
}

void CSessionState::ReadRememberedLevels_t(CTStream *pstr)
{
  pstr->ExpectID_t("RLEV"); // remembered levels
  // read count of remembered levels
  INDEX ctLevels;
  (*pstr)>>ctLevels;
  // for each level
  for(INDEX iLevel=0; iLevel<ctLevels; iLevel++) {
    // create it
    CRememberedLevel *prl = new CRememberedLevel;
    // read it
    (*pstr)>>prl->rl_strFileName;
    //prl->rl_strmSessionState.
    // use readstream() !!!! @@@@

  }
  
};

/*
 * Write session state state into a stream.
 */
void CSessionState::Write_t(CTStream *pstr)  // throw char *
{
#if DEBUG_SYNCSTREAMDUMPING
  CPrintF( "Session state write: Sequence %d, Time %.2f\n", ses_iLastProcessedSequence, ses_tmLastProcessedTick);
#endif
  pstr->WriteID_t("SESV");
  (*pstr)<<INDEX(SESSIONSTATEVERSION_WITHBULLETTIME);
  // write time information and random seed
  (*pstr)<<ses_tmLastProcessedTick;
  (*pstr)<<ses_iLastProcessedSequence;
  (*pstr)<<ses_iLevel;
  (*pstr)<<ses_ulRandomSeed;
  (*pstr)<<ses_ulSpawnFlags;
  (*pstr)<<ses_tmSyncCheckFrequency;
  (*pstr)<<ses_iExtensiveSyncCheck;
  (*pstr)<<ses_tmLastSyncCheck;
  (*pstr)<<ses_ctMaxPlayers;
  (*pstr)<<ses_bWaitAllPlayers;
  (*pstr)<<ses_bPause;
  (*pstr)<<ses_bGameFinished;
  (*pstr)<<ses_fRealTimeFactor;
  // write session properties to stream
  (*pstr)<<_pNetwork->ga_strSessionName;
  pstr->Write_t(_pNetwork->ga_aubProperties, NET_MAXSESSIONPROPERTIES);

  // write world and its state
  WriteWorldAndState_t(pstr);
}

void CSessionState::WriteWorldAndState_t(CTStream *pstr)   // throw char *
{
  // delete all predictor entities before saving
  _pNetwork->ga_World.UnmarkForPrediction();
  _pNetwork->ga_World.DeletePredictors();

  // save engine build
  _pNetwork->WriteVersion_t(*pstr);

  // write world filename to stream
  (*pstr)<<_pNetwork->ga_fnmWorld;

  // write world situation
  _pNetwork->ga_World.LockAll();
  _pNetwork->ga_World.WriteState_t(pstr, TRUE);

  // write number of entities in timer list
  pstr->WriteID_t("TMRS");   // timers
  CListHead &lhTimers = _pNetwork->ga_World.wo_lhTimers;
  *pstr<<lhTimers.Count();
  // for each entity in the timer list
  {FOREACHINLIST(CRationalEntity, en_lnInTimers, lhTimers, iten) {
    // save its index in container
    *pstr<<iten->en_ulID;
  }}

  // write number of entities in mover list
  pstr->WriteID_t("MVRS");   // movers
  CListHead &lhMovers = _pNetwork->ga_World.wo_lhMovers;
  *pstr<<lhMovers.Count();
  // for each entity in the mover list
  {FOREACHINLIST(CMovableEntity, en_lnInMovers, lhMovers, iten) {
    // save its index in container
    *pstr<<iten->en_ulID;
  }}

  // write number of clients
  (*pstr)<<ses_apltPlayers.Count();
  // for all clients
  FOREACHINSTATICARRAY(ses_apltPlayers, CPlayerTarget, itclt) {
    // write to stream
    itclt->Write_t(pstr);
  }

  _pNetwork->ga_World.UnlockAll();
}

void CSessionState::WriteRememberedLevels_t(CTStream *pstr)
{
    // use writestream() !!!! @@@@

};

// remember current level
void CSessionState::RememberCurrentLevel(const CTString &strFileName)
{
  // if level is already remembered
  for(;;) {
    CRememberedLevel *prlOld = FindRememberedLevel(strFileName);
    if (prlOld==NULL) {
      break;
    }
    // remove it
    prlOld->rl_lnInSessionState.Remove();
    delete prlOld;
  }

  // create new remembered level
  CRememberedLevel *prlNew = new CRememberedLevel;
  ses_lhRememberedLevels.AddTail(prlNew->rl_lnInSessionState);
  // remember it
  prlNew->rl_strFileName = strFileName;
  WriteWorldAndState_t(&prlNew->rl_strmSessionState);
}

// find a level if it is remembered
CRememberedLevel *CSessionState::FindRememberedLevel(const CTString &strFileName)
{
  {FOREACHINLIST(CRememberedLevel, rl_lnInSessionState, ses_lhRememberedLevels, itrl) {
    CRememberedLevel &rl = *itrl;
    if (rl.rl_strFileName==strFileName) {
      return &rl;
    }
  }}
  return NULL;
}

// restore some old level
void CSessionState::RestoreOldLevel(const CTString &strFileName)
{
  // find the level
  CRememberedLevel *prlOld = FindRememberedLevel(strFileName);
  // it must exist
  ASSERT(prlOld!=NULL);
  // restore it
  try {
    prlOld->rl_strmSessionState.SetPos_t(0);
    _pTimer->SetCurrentTick(0.0f);
    ReadWorldAndState_t(&prlOld->rl_strmSessionState);
    _pTimer->SetCurrentTick(ses_tmLastProcessedTick);
  } catch (const char *strError) {
    FatalError(TRANS("Cannot restore old level '%s':\n%s"), (const char *) prlOld->rl_strFileName, strError);
  }
  // delete it
  delete prlOld;
}

// make synchronization test message and send it to server (if client), or add to buffer (if server)
void CSessionState::MakeSynchronisationCheck(void)
{
  if (!_cmiComm.cci_bClientInitialized) return;
  // not yet time
  if(ses_tmLastSyncCheck+ses_tmSyncCheckFrequency > ses_tmLastProcessedTick) {
    // don't check yet
    return;
  }

  // make local checksum
  ULONG ulLocalCRC;
  CRC_Start(ulLocalCRC);
  ChecksumForSync(ulLocalCRC, ses_iExtensiveSyncCheck);
  CRC_Finish(ulLocalCRC);

  // create sync-check
  CSyncCheck sc;
  ses_tmLastSyncCheck = ses_tmLastProcessedTick;
  sc.sc_tmTick = ses_tmLastSyncCheck;
  sc.sc_iSequence = ses_iLastProcessedSequence; 
  sc.sc_ulCRC = ulLocalCRC;
  sc.sc_iLevel = ses_iLevel;

  // NOTE: If local client, here we buffer the sync and send it to ourselves.
  // It's because synccheck is piggybacking session message buffer, so we acknowledge
  // the server to flush the buffer up to that sequence.
  
  // if on server
  if (_pNetwork->IsServer()) {
    // buffer the sync-check
    _pNetwork->ga_srvServer.AddSyncCheck(sc);
  }

  // create a message with sync check
  CNetworkMessage nmSyncCheck(MSG_SYNCCHECK);
  nmSyncCheck.Write(&sc, sizeof(sc));
  // send it to server
  _pNetwork->SendToServer(nmSyncCheck);
}

// forget all remembered levels
void CSessionState::ForgetOldLevels(void)
{
  {FORDELETELIST(CRememberedLevel, rl_lnInSessionState, ses_lhRememberedLevels, itrl) {
    delete &*itrl;
  }}
}


extern INDEX cli_bDumpSync;
extern INDEX cli_bDumpSyncEachTick;
void CSessionState::DumpSyncToFile_t(CTStream &strm, INDEX iExtensiveSyncCheck) // throw char *
{
  ULONG ulLocalCRC;
  CRC_Start(ulLocalCRC);
  ChecksumForSync(ulLocalCRC, iExtensiveSyncCheck);
  CRC_Finish(ulLocalCRC);
  strm.FPrintF_t("__________________________________________________________________________________\n", ulLocalCRC);
  strm.FPrintF_t("CRC: 0x%08X\n", ulLocalCRC);
  DumpSync_t(strm, iExtensiveSyncCheck);
}

#if DEBUG_SYNCSTREAMDUMPING
void CSessionState::DumpSyncToMemory(void)
{
  try
  {
    CTMemoryStream *pstrm = GetDumpStream();
    DumpSyncToFile_t(*pstrm);
  }
  catch (const char *strError)
  {
    CPrintF("Cannot dump sync data: %s\n", strError);
  }
}
#endif

/* Session state loop. */
void CSessionState::SessionStateLoop(void)
{
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_SESSIONSTATE_LOOP);

  // while there is something to do
  BOOL bSomethingToDo = TRUE;
  while (bSomethingToDo && !IsDisconnected()) {
    bSomethingToDo = FALSE;

    // if client was disconnected without a notice
    if (!_cmiComm.Client_IsConnected()) {
      // quit
      ses_strDisconnected = TRANS("Link or server is down");
    }

    CNetworkMessage nmMessage;
    // if there is some unreliable message
    if (_pNetwork->ReceiveFromServer(nmMessage)) {
      bSomethingToDo = TRUE;

      // if it is a gamestream message
      if (nmMessage.GetType() == MSG_GAMESTREAMBLOCKS) {
        ses_tvMessageReceived = _pTimer->GetHighPrecisionTimer();
        ses_bWaitingForServer = FALSE;

        // unpack the message
        CNetworkMessage nmUnpackedBlocks(MSG_GAMESTREAMBLOCKS);
        nmMessage.UnpackDefault(nmUnpackedBlocks);

        // while there are some more blocks in the message
        while (!nmUnpackedBlocks.EndOfMessage()) {
          // read a block to the gamestream
          ses_nsGameStream.ReadBlock(nmUnpackedBlocks);
        }

      // if it is a keepalive
      } else if (nmMessage.GetType() == MSG_KEEPALIVE) {
        // just remember time
        ses_tvMessageReceived = _pTimer->GetHighPrecisionTimer();
        _pNetwork->AddNetGraphValue(NGET_NONACTION, 0.5f); // non-action sequence

      // if it is pings message
      } else if (nmMessage.GetType() == MSG_INF_PINGS) {
        for(INDEX i=0; i<NET_MAXGAMEPLAYERS; i++) {
          CPlayerTarget &plt = ses_apltPlayers[i];
          BOOL bHas = 0;
          nmMessage.ReadBits(&bHas, 1);
          if (bHas) {
            if (plt.IsActive() && plt.plt_penPlayerEntity!=NULL) {
              INDEX iPing = 0;
              nmMessage.ReadBits(&iPing, 10);
              plt.plt_penPlayerEntity->en_tmPing = iPing/1000.0f;
            }
          }
        }
      // if it is chat message
      } else if (nmMessage.GetType() == MSG_CHAT_OUT) {
        // read the message
        ULONG ulFrom;
        CTString strFrom;
        nmMessage>>ulFrom;
        if (ulFrom==0) {
          nmMessage>>strFrom;
        }
        CTString strMessage;
        nmMessage>>strMessage;
        // print it
        PrintChatMessage(ulFrom, strFrom, strMessage);
      // otherwise
      } else {
        CPrintF(TRANSV("Session state: Unexpected message during game: %s(%d)\n"),
          ErrorDescription(&MessageTypes, nmMessage.GetType()), nmMessage.GetType());
      }
    }

    CNetworkMessage nmReliable;
    // if there is some reliable message
    if (_pNetwork->ReceiveFromServerReliable(nmReliable)) {
      bSomethingToDo = TRUE;
      // if this is disconnect message
      if (nmReliable.GetType() == MSG_INF_DISCONNECTED) {
				// confirm disconnect
				CNetworkMessage nmConfirmDisconnect(MSG_REP_DISCONNECTED);			
				_pNetwork->SendToServerReliable(nmConfirmDisconnect);
        // report the reason
        CTString strReason;
        nmReliable>>strReason;
        ses_strDisconnected = strReason;
        CPrintF(TRANSV("Disconnected: %s\n"), (const char *) strReason);
        // disconnect
        _cmiComm.Client_Close();
      // if this is recon response
      } else if (nmReliable.GetType() == MSG_ADMIN_RESPONSE) {
        // just print it
        CTString strResponse;
        nmReliable>>strResponse;
        CPrintF("%s", (const char *) ("|"+strResponse+"\n"));
      // otherwise
      } else {
        CPrintF(TRANSV("Session state: Unexpected reliable message during game: %s(%d)\n"),
          ErrorDescription(&MessageTypes, nmReliable.GetType()), nmReliable.GetType());
      }
    }
  }

  // if network client and not waiting for server
  if (_pNetwork->IsNetworkEnabled() && !_pNetwork->IsServer() && !ses_bWaitingForServer) {
    // check when last message was received.
    if (ses_tvMessageReceived.tv_llValue>0 &&
      (_pTimer->GetHighPrecisionTimer()-ses_tvMessageReceived).GetSeconds()>net_tmDisconnectTimeout &&
      ses_strDisconnected=="") {
      ses_strDisconnected = TRANS("Connection timeout");
      CPrintF(TRANSV("Disconnected: %s\n"), (const char*)ses_strDisconnected);
    }
  }

  // if pause state should be changed
  if (ses_bPause!=ses_bWantPause) {
    // send appropriate packet to server
    CNetworkMessage nmReqPause(MSG_REQ_PAUSE);
    nmReqPause<<(INDEX&)ses_bWantPause;
    _pNetwork->SendToServer(nmReqPause);
  }

  // dump sync data if needed
  if( cli_bDumpSync)
  {
    cli_bDumpSync = FALSE;
    try
    {
      CTFileStream strmFile;
      CTString strFileName = CTString("temp\\syncdump.txt");
      strmFile.Create_t(CTString("temp\\syncdump.txt"), CTStream::CM_TEXT);
      
#if DEBUG_SYNCSTREAMDUMPING
      if( cli_bDumpSyncEachTick)
      {
        // get size and buffer from the stream
        void *pvBuffer;
        SLONG slSize;
        ses_pstrm->LockBuffer(&pvBuffer, &slSize);
        strmFile.Write_t(pvBuffer, slSize);
        ses_pstrm->UnlockBuffer();
      }
      else
#endif
      {
        DumpSyncToFile_t(strmFile, ses_iExtensiveSyncCheck);
      }
      // inform user
      CPrintF("Sync data dumped to '%s'\n", (const char *) strFileName);
    }
    catch (const char *strError)
    {
      CPrintF("Cannot dump sync data: %s\n", (const char *) strError);
    }
  }

  // if some client settings changed
  if (!ses_sspParams.IsUpToDate()) {
    // remember new settings
    ses_sspParams.Update();
    // send message to server
    CNetworkMessage nmSet(MSG_SET_CLIENTSETTINGS);
    nmSet<<ses_sspParams;
    _pNetwork->SendToServerReliable(nmSet);
  }

  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_SESSIONSTATE_LOOP);
}

/* Get number of active players. */
INDEX CSessionState::GetPlayersCount(void)
{
  INDEX ctPlayers = 0;
  FOREACHINSTATICARRAY(ses_apltPlayers, CPlayerTarget, itplt) {
    if (itplt->IsActive()) {
      ctPlayers++;
    }
  }
  return ctPlayers;
}

/* Remember predictor positions of all players. */
void CSessionState::RememberPlayerPredictorPositions(void)
{
  // for each active player
  FOREACHINSTATICARRAY(ses_apltPlayers, CPlayerTarget, itplt) {
    CPlayerTarget &plt = *itplt;
    if (plt.IsActive()) {
      // remember its current, or predictor position
      CEntity *pen = plt.plt_penPlayerEntity;
      if (pen->IsPredicted()) {
        pen = pen->GetPredictor();
      }
      plt.plt_vPredictorPos = pen->GetPlacement().pl_PositionVector;
    }
  }
}

/* Get player position. */
const FLOAT3D &CSessionState::GetPlayerPredictorPosition(INDEX iPlayer)
{
  return ses_apltPlayers[iPlayer].plt_vPredictorPos;
}

CPredictedEvent::CPredictedEvent(void)
{
}

// check an event for prediction, returns true if already predicted
BOOL CSessionState::CheckEventPrediction(CEntity *pen, ULONG ulTypeID, ULONG ulEventID)
{
  // if prediction is not involved
  if ( !( pen->GetFlags() & (ENF_PREDICTOR|ENF_PREDICTED|ENF_WILLBEPREDICTED) ) ){
    // not predicted
    return FALSE;
  }

  // find eventual prediction tail
  if (pen->IsPredictor()) {
    pen = pen->GetPredictionTail();
  }

  // gather all event relevant data
  ULONG ulEntityID = pen->en_ulID;
  TIME tmNow = _pTimer->CurrentTick();

  BOOL bPredicted = FALSE;
  // for each active event
  INDEX ctpe = ses_apeEvents.Count();
  {for(INDEX ipe=0; ipe<ctpe; ipe++) {
    CPredictedEvent &pe = ses_apeEvents[ipe];
    // if the event is too old
    if (pe.pe_tmTick<tmNow-5.0f) {
      // delete it from list
      pe = ses_apeEvents[ctpe-1];
      ctpe--;
      ipe--;
      continue;
    }
    // if the event is same as the new one
    if (pe.pe_tmTick==tmNow &&
        pe.pe_ulEntityID==ulEntityID && 
        pe.pe_ulTypeID==ulTypeID && 
        pe.pe_ulEventID==ulEventID) {
      // it was already predicted
      bPredicted = TRUE;
      break;
    }
  }}

  // remove unused events at the end
  if (ctpe==0) {
    ses_apeEvents.PopAll();
  } else {
    ses_apeEvents.PopUntil(ctpe-1);
  }

  // if not predicted
  if (!bPredicted) {
    // add the new one
    CPredictedEvent &pe = ses_apeEvents.Push();
    pe.pe_tmTick     = tmNow;
    pe.pe_ulEntityID = ulEntityID;
    pe.pe_ulTypeID   = ulTypeID;
    pe.pe_ulEventID  = ulEventID;
  }

  return bPredicted;
}
