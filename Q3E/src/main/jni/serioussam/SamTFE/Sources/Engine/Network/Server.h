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

#ifndef SE_INCL_SERVER_H
#define SE_INCL_SERVER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Synchronization.h>
#include <Engine/Network/NetworkMessage.h>
#include <Engine/Network/SessionState.h>
#include <Engine/Templates/StaticArray.h>

/*
 * Server, manages game joining and similar, routes messages from PlayerSource to PlayerTarget
 */
class CServer {
public:
  BOOL srv_bActive;                        // set if started

  CStaticArray<CSessionSocket> srv_assoSessions;  // client computers in game
  CStaticArray<CPlayerBuffer> srv_aplbPlayers;  // player buffers for all clients in game
  CStaticArray<CSyncCheck> srv_ascChecks;       // remembered sync checks

  TIME srv_tmLastProcessedTick;            // last tick when all actions have been resent
  INDEX srv_iLastProcessedSequence;        // sequence of last sent game stream block

  BOOL srv_bPause;      // set while game is paused
  BOOL srv_bGameFinished; // set while game is finished
  FLOAT srv_fServerStep;  // counter for smooth time slowdown/speedup
public:
  /* Send disconnect message to some client. */
  void SendDisconnectMessage(INDEX iClient, const char *strExplanation, BOOL bStream = FALSE);
  /* Get total number of active players. */
  INDEX GetPlayersCount(void);
  /* Get number of active vip players. */
  INDEX GetVIPPlayersCount(void);
  /* Get total number of active clients. */
  INDEX GetClientsCount(void);
  /* Get number of active vip clients. */
  INDEX GetVIPClientsCount(void);
  /* Get number of active observers. */
  INDEX GetObserversCount(void);
  /* Get number of active players on one client. */
  INDEX GetPlayersCountForClient(INDEX iClient);
  /* Find first inactive player. */
  CPlayerBuffer *FirstInactivePlayer(void);
  /* Check if some character name already exists in this session. */
  BOOL CharacterNameIsUsed(CPlayerCharacter &pcCharacter);

  /* Send initialization info to local client. */
  void ConnectLocalSessionState(INDEX iClient, CNetworkMessage &nm);
  /* Send initialization info to remote client. */
  void ConnectRemoteSessionState(INDEX iClient, CNetworkMessage &nm);
  /* Send session state data to remote client. */
  void SendSessionStateData(INDEX iClient);

  /* Send one regular batch of sequences to a client. */
  void SendGameStreamBlocks(INDEX iClient);
  /* Resend a batch of game stream blocks to a client. */
  void ResendGameStreamBlocks(INDEX iClient, INDEX iSequence0, INDEX ctSequences);

  // add a new sync check to buffer
  void AddSyncCheck(const CSyncCheck &sc);
  // try to find a sync check for given time in the buffer (-1==too old, 0==found, 1==toonew)
  INDEX FindSyncCheck(TIME tmTick, CSyncCheck &sc);

  // make allaction messages for one tick
  void MakeAllActions(void);
  // add a block to streams for all sessions
  void AddBlockToAllSessions(CNetworkStreamBlock &nsb);
  // find a mask of all players on a certain client
  ULONG MaskOfPlayersOnClient(INDEX iClient);
public:
  /* Constructor. */
  CServer(void);
  /* Destructor. */
  ~CServer();

  /* Start server. */
  void Start_t(void);
  /* Stop server. */
  void Stop(void);
  /* Run server loop. */
  void ServerLoop(void);
  /* Make synchronization test message and add it to game stream. */
  void MakeSynchronisationCheck(void);

  /* Handle incoming network messages. */
  void HandleAll();
  void HandleAllForAClient(INDEX iClient);
  void HandleClientDisconected(INDEX iClient);
  void Handle(INDEX iClient, CNetworkMessage &nm);
};


#endif  /* include-once check. */

