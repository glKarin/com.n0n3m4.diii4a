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

#ifndef SE_INCL_SESSIONSTATE_H
#define SE_INCL_SESSIONSTATE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Synchronization.h>
#include <Engine/Templates/StaticStackArray.h>
#include <Engine/Network/NetworkMessage.h>
#include <Engine/Network/PlayerTarget.h>
#include <Engine/Network/SessionSocket.h>
#include <Engine/Base/Timer.h>

#define DEBUG_SYNCSTREAMDUMPING 0

#if DEBUG_SYNCSTREAMDUMPING
  /* 
   * Obtain valid session dump memory stream
   */
  CTMemoryStream *GetDumpStream(void);
  /* 
   * Clear session dump memory stream
   */
  void ClearDumpStream(void);
#endif

#ifdef NETSTRUCTS_PACKED
  #pragma pack(1)
#endif

// checksum of world snapshot at given point in time - used for sync-checking
class CSyncCheck {
public:
  TIME sc_tmTick;       // time of snapshot
  INDEX sc_iSequence;   // sequence number last processed before this checksum
  ULONG sc_ulCRC;       // checksum
  INDEX sc_iLevel;  // checksum of level filename
  CSyncCheck(void) { sc_tmTick = -1.0f; sc_iSequence = -1; sc_ulCRC = 0; sc_iLevel = 0; }
  void Clear(void) { sc_tmTick = -1.0f; sc_iSequence = -1; sc_ulCRC = 0; sc_iLevel = 0; }
};

// info about an event that was predicted to happen
class CPredictedEvent {
public:
  TIME pe_tmTick;
  ULONG pe_ulEntityID;
  ULONG pe_ulTypeID;
  ULONG pe_ulEventID;

  CPredictedEvent(void);
  void Clear(void) {};
};

#ifdef NETSTRUCTS_PACKED
  #pragma pack()
#endif

/*
 * Session state, manipulates local copy of the world
 */
class ENGINE_API CSessionState {
public:
  CStaticArray<CPlayerTarget> ses_apltPlayers; // client targets for all players in game
  CStaticStackArray<CPredictedEvent> ses_apeEvents; // for event prediction

  CTString ses_strMOTD;               // MOTD as sent from the server
  INDEX ses_iLevel;                   // for counting level changes
  INDEX ses_iLastProcessedSequence;   // sequence of last processed stream block
  CNetworkStream ses_nsGameStream;    // stream of blocks from server

  // lerp params
  CTimerValue ses_tvInitialization;  // exact moment when the session state was started
  TIME ses_tmInitializationTick;     // tick when the session state was started
  // secondary lerp params for non-predicted movement
  CTimerValue ses_tvInitialization2;  // exact moment when the session state was started
  TIME ses_tmInitializationTick2;     // tick when the session state was started

  TIME ses_tmLastProcessedTick;      // last tick when all actions were processed
  TIME ses_tmPredictionHeadTick;     // newest tick that was ever predicted
  TIME ses_tmLastSyncCheck;          // last time sync-check was generated
  TIME ses_tmLastPredictionProcessed;  // for determining when to do a new prediction cycle

  INDEX ses_iMissingSequence;       // first missing sequence
  CTimerValue ses_tvResendTime;     // timer for missing sequence retransmission
  TIME ses_tmResendTimeout;         // timeout value for increasing the request interval
  CTimerValue ses_tvMessageReceived;  // exact moment when the session state was started

  TIME ses_tmLastDemoSequence;    // synchronization timer for demo playing
  ULONG ses_ulRandomSeed;         // seed for pseudo-random number generation
  ULONG ses_ulSpawnFlags;         // spawn flags for current game
  TIME ses_tmSyncCheckFrequency;  // frequency of sync-checking
  BOOL ses_iExtensiveSyncCheck;   // set if syncheck should be extensive - for debugging purposes

  BOOL ses_bKeepingUpWithTime;     // set if the session state is keeping up with the time
  TIME ses_tmLastUpdated;
  CListHead ses_lhRememberedLevels;   // list of remembered levels
  BOOL ses_bAllowRandom;            // set while random number generation is valid
  BOOL ses_bPredicting;             // set if the game is currently doing prediction
  
  BOOL ses_bPause;      // set while game is paused
  BOOL ses_bWantPause;  // set while wanting to have paused
  BOOL ses_bGameFinished;  // set when game has finished
  BOOL ses_bWaitingForServer;        // wait for server after level change
  CTString ses_strDisconnected; // explanation of disconnection or empty string if not disconnected

  INDEX ses_ctMaxPlayers; // maximum number of players allowed in game
  BOOL ses_bWaitAllPlayers; // if set, wait for all players to join before starting
  FLOAT ses_fRealTimeFactor;  // enables slower or faster time for special effects
  CTMemoryStream *ses_pstrm;  // debug stream for sync check examination
  
  CSessionSocketParams ses_sspParams; // local copy of server-side parameters
public:
  // network message waiters
  void Start_AtServer_t(void);     // throw char *
  void Start_AtClient_t(INDEX ctLocalPlayers);     // throw char *
  // Set lerping factor for current frame.
  void SetLerpFactor(CTimerValue tvNow);
  // notify entities of level change
  void SendLevelChangeNotification(class CEntityEvent &ee);
  // wait for a stream to come from server
  void WaitStream_t(CTMemoryStream &strmMessage, const CTString &strName, INDEX iMsgCode);
  // check if disconnected
  BOOL IsDisconnected(void);

  // print an incoming chat message to console
  void PrintChatMessage(ULONG ulFrom, const CTString &strFrom, const CTString &strMessage);
public:
  /* Constructor. */
  CSessionState(void);
  /* Destructor. */
  ~CSessionState(void);

  // get a pseudo-random number (safe for network gaming)
  ULONG Rnd(void);

  /* Stop the session state. */
  void Stop(void);
  /* Start session state. */
  void Start_t(INDEX ctLocalPlayers); // throw char *

  // do physics for a game tick
  void HandleMovers(void);
  // do thinking for a game tick
  void HandleTimers(TIME tmCurrentTick);
  // do a warm-up run of the world for a few ticks
  void WarmUpWorld(void);
  // reset random number generator (always randomizes to same sequence!)
  void ResetRND(void);
  /* Process a game tick. */
  void ProcessGameTick(CNetworkMessage &nmMessage, TIME tmCurrentTick);
  /* Process a predicted game tick. */
  void ProcessPredictedGameTick(INDEX iPredictionStep, FLOAT fFactor, TIME tmCurrentTick);
  /* Process a gamestream block. */
  void ProcessGameStreamBlock(CNetworkMessage &nmMessage);
  /* Process all eventual avaliable gamestream blocks. */
  void ProcessGameStream(void);
  // flush prediction actions that were already processed
  void FlushProcessedPredictions(void);
  /* Find out how many prediction steps are currently pending. */
  INDEX GetPredictionStepsCount(void);
  /* Process all eventual avaliable prediction actions. */
  void ProcessPrediction(void);
  /* Get number of active players. */
  INDEX GetPlayersCount(void);
  /* Remember predictor positions of all players. */
  void RememberPlayerPredictorPositions(void);
  /* Get player position. */
  const FLOAT3D &GetPlayerPredictorPosition(INDEX iPlayer);

  // check an event for prediction, returns true if already predicted
  BOOL CheckEventPrediction(CEntity *pen, ULONG ulTypeID, ULONG ulEventID);

  // make synchronization test message and send it to server (if client), or add to buffer (if server)
  void MakeSynchronisationCheck(void);
  // create a checksum value for sync-check
  void ChecksumForSync(ULONG &ulCRC, INDEX iExtensiveSyncCheck);
  // dump sync data to text file
  void DumpSync_t(CTStream &strm, INDEX iExtensiveSyncCheck);  // throw char *

  /* Read session state information from a stream. */
  void Read_t(CTStream *pstr);   // throw char *
  void ReadWorldAndState_t(CTStream *pstr);   // throw char *
  void ReadRememberedLevels_t(CTStream *pstr);   // throw char *
  /* Write session state information into a stream. */
  void Write_t(CTStream *pstr);  // throw char *
  void WriteWorldAndState_t(CTStream *pstr);   // throw char *
  void WriteRememberedLevels_t(CTStream *pstr);   // throw char *

  // remember current level
  void RememberCurrentLevel(const CTString &strFileName);
  // find a level if it is remembered
  class CRememberedLevel *FindRememberedLevel(const CTString &strFileName);
  // restore some old level
  void RestoreOldLevel(const CTString &strFileName);
  // forget all remembered levels
  void ForgetOldLevels(void);

  /* Session state loop. */
  void SessionStateLoop(void);
  /* Session sync dump functions. */
  void DumpSyncToFile_t(CTStream &strm, INDEX iExtensiveSyncCheck); // throw char *
#if DEBUG_SYNCSTREAMDUMPING
  void DumpSyncToMemory(void);
#endif
};


#endif  /* include-once check. */

