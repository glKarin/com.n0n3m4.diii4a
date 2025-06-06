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
#include <Engine/Network/Network.h>
#include <Engine/Network/PlayerSource.h>
#include <Engine/Network/PlayerTarget.h>
#include <Engine/Network/CommunicationInterface.h>
#include <Engine/Network/SessionState.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Entities/InternalClasses.h>

extern FLOAT net_tmConnectionTimeout;
extern FLOAT net_tmProblemsTimeOut;

/*
 *  Constructor.
 */
CPlayerSource::CPlayerSource(void) {
  pls_Active = FALSE;
  pls_Index = -2;
  pls_csAction.cs_iIndex = -1;
  // clear action packet
  pls_paAction.Clear();
}

/*
 *  Destructor.
 */
CPlayerSource::~CPlayerSource(void) {
}

/*
 * Activate a new player.
 */
void CPlayerSource::Start_t(CPlayerCharacter &pcCharacter) // throw char *
{
  ASSERT(!pls_Active);

  // set index to -1 what means that you are not yet registered properly
  pls_Index = -1;
  // copy the character data
  pls_pcCharacter = pcCharacter;
  // clear actions
  pls_paAction.Clear();;
  for(INDEX ipa=0; ipa<PLS_MAXLASTACTIONS; ipa++) {
    pls_apaLastActions[ipa].Clear();
  }

  // request player connection
  CNetworkMessage nmRegisterPlayer(MSG_REQ_CONNECTPLAYER);
  nmRegisterPlayer<<pls_pcCharacter;    // player's character data
  _pNetwork->SendToServerReliable(nmRegisterPlayer);

  for(TIME tmWait=0; 
      tmWait<net_tmConnectionTimeout*1000; 
      _pTimer->Sleep(NET_WAITMESSAGE_DELAY), tmWait+=NET_WAITMESSAGE_DELAY) {
    if (_pNetwork->ga_IsServer) {
      _pNetwork->TimerLoop();
    }

    if (_cmiComm.Client_Update() == FALSE) {
			break;
		}
    // wait for message to come
    CNetworkMessage nmReceived;
    if (!_pNetwork->ReceiveFromServerReliable(nmReceived)) {
      continue;
    }

    // if this is the init message
    if (nmReceived.GetType() == MSG_REP_CONNECTPLAYER) {
      // remember your index
      nmReceived>>pls_Index;
      // finish waiting
      pls_Active = TRUE;
      return;
    // if this is disconnect message
    } else if (nmReceived.GetType() == MSG_INF_DISCONNECTED) {
			// confirm disconnect
			CNetworkMessage nmConfirmDisconnect(MSG_REP_DISCONNECTED);			
			_pNetwork->SendToServerReliable(nmConfirmDisconnect);

      // throw exception
      CTString strReason;
      nmReceived>>strReason;
      _pNetwork->ga_sesSessionState.ses_strDisconnected = strReason;
      ThrowF_t(TRANS("Cannot add player because: %s\n"), (const char *) strReason);

    // otherwise
    } else {
      // it is invalid message
      ThrowF_t(TRANS("Invalid message while waiting for player registration"));
    }

    // if client is disconnected
    if (!_cmiComm.Client_IsConnected()) {
      // quit
      ThrowF_t(TRANS("Client disconnected"));
    }

  }

		CNetworkMessage nmConfirmDisconnect(MSG_REP_DISCONNECTED);			
	_pNetwork->SendToServerReliable(nmConfirmDisconnect);

  ThrowF_t(TRANS("Timeout while waiting for player registration"));
}

/*
 * Deactivate removed player.
 */
void CPlayerSource::Stop(void)
{
  ASSERT(pls_Active);
  pls_Active = FALSE;
  pls_Index = -2;
}

// request character change for a player
// NOTE: the request is asynchronious and possible failure cannot be detected
void CPlayerSource::ChangeCharacter(CPlayerCharacter &pcNew)
{
  // if the requested character has different guid
  if (!(pls_pcCharacter==pcNew)) {
    // fail
    CPrintF(TRANSV("Cannot update character - different GUID\n"));
  }

  // just request the change
  CNetworkMessage nmChangeChar(MSG_REQ_CHARACTERCHANGE);
  nmChangeChar<<pls_Index<<pcNew;
  _pNetwork->SendToServerReliable(nmChangeChar);
  // remember new setting
  pls_pcCharacter = pcNew;
}

/*
 * Set player action.
 */
void CPlayerSource::SetAction(const CPlayerAction &paAction)
{
  // synchronize access to action
  CTSingleLock slAction(&pls_csAction, TRUE);
  // set action
  pls_paAction = paAction;
  pls_paAction.pa_llCreated = _pTimer->GetHighPrecisionTimer().GetMilliseconds();
  //CPrintF("%.2f - created: %d\n", _pTimer->GetRealTimeTick(), SLONG(pls_paAction.pa_llCreated));
}

// get mask of this player for chat messages
ULONG CPlayerSource::GetChatMask(void)
{
  return 1UL<<pls_Index;
}

/* Create action packet from current player commands and for sending to server. */
void CPlayerSource::WriteActionPacket(CNetworkMessage &nm)
{
  // synchronize access to actions
  CTSingleLock slActions(&pls_csAction, TRUE);

  // check if active and registered
  CPlayerEntity *ppe = NULL;
  if (IsActive()) {
    ppe = (CPlayerEntity *)_pNetwork->GetLocalPlayerEntity(this);
  }
  // if not
  if (ppe==NULL) {
    // just write a dummy bit
    BOOL bActive = 0;
    nm.WriteBits(&bActive, 1);
    return;
  }

  // normalize action (remove invalid floats like -0)
  pls_paAction.Normalize();

  ASSERT(pls_Active);
  ASSERT(pls_Index>=0);

  // determine ping
  FLOAT tmPing = ppe->en_tmPing;
  INDEX iPing = (INDEX)ceil(tmPing*1000);

  // write all in the message
  BOOL bActive = 1;
  nm.WriteBits(&bActive, 1);
  nm.WriteBits(&pls_Index, 4);  // your index
  nm.WriteBits(&iPing, 10);     // your ping
  nm<<pls_paAction;             // action
  //CPrintF("%.2f - written: %d\n", _pTimer->GetRealTimeTick(), SLONG(pls_paAction.pa_llCreated));

  // get sendbehind parameters
  extern INDEX cli_iSendBehind;
  extern INDEX cli_bPredictIfServer;
  cli_iSendBehind = Clamp(cli_iSendBehind, (INDEX)0, (INDEX)3);
  INDEX iSendBehind = cli_iSendBehind;

  // disable if server
  if (_pNetwork->IsServer() && !cli_bPredictIfServer) {
    iSendBehind = 0;
  }

  // save sendbehind if needed
  nm.WriteBits(&iSendBehind, 2);
  for(INDEX i=0; i<iSendBehind; i++) {
    nm<<pls_apaLastActions[i];
  }

  // remember last action
  for(INDEX ipa=1; ipa<PLS_MAXLASTACTIONS; ipa++) {
    pls_apaLastActions[ipa] = pls_apaLastActions[ipa-1];
  }
  pls_apaLastActions[0] = pls_paAction;

  // if not paused
  if (!_pNetwork->IsPaused() && !_pNetwork->GetLocalPause()) {
    // get the index of the player target in game state
    INDEX iPlayerTarget = pls_Index;
    // if player is added
    if (iPlayerTarget>=0) {
      // get the player target
      CPlayerTarget &plt = _pNetwork->ga_sesSessionState.ses_apltPlayers[iPlayerTarget];
      // let it buffer the packet
      plt.PrebufferActionPacket(pls_paAction);
    }
  }
}
