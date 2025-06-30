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

#ifndef SE_INCL_PLAYERSOURCE_H
#define SE_INCL_PLAYERSOURCE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Synchronization.h>
#include <Engine/Network/NetworkMessage.h>
#include <Engine/Entities/PlayerCharacter.h>

/*
 * Player source, located on client computer; creating actions and receiving updates
 */
class ENGINE_API CPlayerSource {
public:
  INDEX pls_Index;
  BOOL pls_Active;                        // set if this player exists
  CPlayerCharacter pls_pcCharacter;       // this player's character data

  CTCriticalSection pls_csAction;  // access to player action
  CPlayerAction pls_paAction;      // action that this player is currently doing
#define PLS_MAXLASTACTIONS 3
  CPlayerAction pls_apaLastActions[PLS_MAXLASTACTIONS]; // old actions remembered for resending
public:
  /* Activate a new player. */
  void Start_t(CPlayerCharacter &pcCharacter);
  /* Deactivate removed player. */
  void Stop(void);
  /* Check if this player is active. */
  BOOL IsActive(void) { return pls_Active; };
  // request character change for a player
  // NOTE: the request is asynchronious and possible failure cannot be detected
  void ChangeCharacter(CPlayerCharacter &pcNew);

  /* Create action packet from current player commands and for sending to server. */
  void WriteActionPacket(CNetworkMessage &nm);
public:

  CPlayerSource();
  ~CPlayerSource();

  /* Set current player action. */
  void SetAction(const CPlayerAction &paAction);
  // get mask of this player for chat messages
  ULONG GetChatMask(void);
};


#endif  /* include-once check. */

