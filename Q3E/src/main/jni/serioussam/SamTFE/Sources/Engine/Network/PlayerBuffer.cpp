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

#include "Engine/StdH.h"

#include <Engine/Base/Console.h>
#include <Engine/Network/PlayerBuffer.h>

/*
 *  Constructor.
 */
CPlayerBuffer::CPlayerBuffer(void) {
  plb_Active = FALSE;
  plb_abReceived.Clear();
  plb_paLastAction.Clear();
  plb_iClient = -1;
}

/*
 *  Destructor.
 */
CPlayerBuffer::~CPlayerBuffer(void) {
}

/*
 * Activate player buffer for a new player.
 */
void CPlayerBuffer::Activate(INDEX iClient)
{
  ASSERT(!plb_Active);
  plb_Active = TRUE;
  plb_iClient = iClient;
  // make packets dummy before receiving something
  plb_abReceived.Clear();
  plb_paLastAction.Clear();
}

/*
 * Deactivate player data for removed player.
 */
void CPlayerBuffer::Deactivate(void)
{
  ASSERT(plb_Active);
  plb_Active = FALSE;
  plb_iClient = -1;
}

/*
 * Receive action packet from player source.
 */
void CPlayerBuffer::ReceiveActionPacket(CNetworkMessage *pnm, INDEX iMaxBuffer)
{
  ASSERT(plb_Active);
  // receive new action
  CPlayerAction pa;
  (*pnm)>>pa;
  // buffer it
  plb_abReceived.AddAction(pa);
  // read sendbehind 
  INDEX iSendBehind = 0;
  pnm->ReadBits(&iSendBehind, 2);
  // foreach resent action
  for(INDEX i=0; i<iSendBehind; i++) {
    CPlayerAction paOld;
    (*pnm)>>paOld;

    // if not already sent out back to the client
    if (paOld.pa_llCreated>plb_paLastAction.pa_llCreated) {
      // buffer it
      plb_abReceived.AddAction(paOld);
    }
  }

/*
  INDEX ctBuffered = plb_abReceived.GetCount();
  if (ctBuffered>net_iPlayerBufferActions) {
    CPrintF("Receive: BUFFER FULL (%d) ++++++++++++++\n", ctBuffered);
  }
  CPrintF("Receive: buffered %d\n", ctBuffered);
  */
  // while there are more too many actions buffered
  while(plb_abReceived.GetCount()>iMaxBuffer) {
    // purge the oldest one
    plb_abReceived.RemoveOldest();
  }
}

/* Create action packet for player target from oldest buffered action. */
// (prepares lag info for given client number)
void CPlayerBuffer::CreateActionPacket(CNetworkMessage *pnm, INDEX iClient)
{
  ASSERT(plb_Active);
  CPlayerAction paCurrent;

  //CPrintF("Send: buffered %d\n", plb_abReceived.GetCount());

  // if there are any buffered actions
  if (plb_abReceived.GetCount()>0) {
    // retrieve the oldest one
    plb_abReceived.GetActionByIndex(0, paCurrent);
  // if there are no buffered actions
  } else {
    // reuse the last one
    paCurrent = plb_paLastAction;
    //CPrintF("Send: BUFFER EMPTY ---------\n");
  }

  // create a new delta action packet between last sent and current action
  CPlayerAction paDelta;
  for (INDEX i = 0; i < static_cast<INDEX>(sizeof(CPlayerAction)); i++) {
    ((UBYTE*)&paDelta)[i] = ((UBYTE*)&paCurrent)[i] ^ ((UBYTE*)&plb_paLastAction)[i];
  }
  // if the client that message is sent to owns the player
  if (iClient==plb_iClient) {
    // send delta of the timetag
    paDelta.pa_llCreated = paCurrent.pa_llCreated-plb_paLastAction.pa_llCreated;
  // if the client doesn't own the player
  } else {
    // no timetag information
    paDelta.pa_llCreated = 0;
  }
  // send the delta packet
  (*pnm)<<paDelta;
}

/* Advance action buffer by one tick by removing oldest action. */
void CPlayerBuffer::AdvanceActionBuffer(void)
{
  ASSERT(plb_Active);
  // if there are any buffered actions
  if (plb_abReceived.GetCount()>0) {
    // get the oldest one and remove it
    CPlayerAction paCurrent;
    plb_abReceived.GetActionByIndex(0, paCurrent);
    plb_abReceived.RemoveOldest();
    // move current action to last action
    plb_paLastAction = paCurrent;
  }
}
