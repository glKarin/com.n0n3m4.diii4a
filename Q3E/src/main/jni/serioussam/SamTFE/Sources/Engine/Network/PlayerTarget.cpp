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
#include <Engine/Network/Network.h>
#include <Engine/Network/PlayerTarget.h>
#include <Engine/Base/Stream.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/Templates/StaticArray.cpp>

extern INDEX cli_bLerpActions;
/*
 *  Constructor.
 */
CPlayerTarget::CPlayerTarget(void) {
  plt_bActive = FALSE;
  plt_penPlayerEntity = NULL;
  plt_csAction.cs_iIndex = -1;
  plt_paLastAction.Clear();
  plt_paPreLastAction.Clear();
  plt_abPrediction.Clear();
}

/*
 *  Destructor.
 */
CPlayerTarget::~CPlayerTarget(void) {
}

/*
 * Read player information from a stream.
 */
void CPlayerTarget::Read_t(CTStream *pstr) // throw char *
{
  INDEX iEntity;
  ULONG bActive;
  // synchronize access to actions
  CTSingleLock slActions(&plt_csAction, TRUE);

  // read activity flag
  (*pstr)>>bActive;
  // if client is active
  if (bActive) {
    // set it up
    Activate();
    // read data
    (*pstr)>>iEntity>>plt_paLastAction>>plt_paPreLastAction;
    CPlayerEntity *penPlayer = (CPlayerEntity *)&_pNetwork->ga_World.wo_cenAllEntities[iEntity];
    ASSERT(penPlayer != NULL);
    AttachEntity(penPlayer);
  }
  plt_abPrediction.Clear();
}

/*
 * Write client information into a stream.
 */
void CPlayerTarget::Write_t(CTStream *pstr) // throw char *
{
  INDEX iEntity;
  ULONG bActive = plt_bActive;
  // synchronize access to actions
  CTSingleLock slActions(&plt_csAction, TRUE);

  // write activity flag
  (*pstr)<<bActive;
  // if client is active
  if (bActive) {
    // prepare its data
    iEntity = _pNetwork->ga_World.wo_cenAllEntities.Index(plt_penPlayerEntity);
    // write data
    (*pstr)<<iEntity<<plt_paLastAction<<plt_paPreLastAction;
  }
}

/*
 * Activate client data for a new client.
 */
void CPlayerTarget::Activate(void)
{
  ASSERT(!plt_bActive);
  plt_bActive = TRUE;
  plt_abPrediction.Clear();
  plt_paPreLastAction.Clear();
  plt_paLastAction.Clear();

}

/*
 * Deactivate client data for removed client.
 */
void CPlayerTarget::Deactivate(void)
{
  ASSERT(plt_bActive);
  plt_bActive = FALSE;
  plt_penPlayerEntity = NULL;
  plt_abPrediction.Clear();
  plt_paPreLastAction.Clear();
  plt_paLastAction.Clear();
}

/*
 * Attach an entity to this client.
 */
void CPlayerTarget::AttachEntity(CPlayerEntity *penClientEntity)
{
  ASSERT(plt_bActive);
  plt_penPlayerEntity = penClientEntity;
}

/*
 * Apply action packet to current actions.
 */
void CPlayerTarget::ApplyActionPacket(const CPlayerAction &paDelta)
{
  ASSERT(plt_bActive);
  ASSERT(plt_penPlayerEntity != NULL);
  // synchronize access to actions
  CTSingleLock slActions(&plt_csAction, TRUE);

  // create a new action packet from last received packet and given delta
  plt_paPreLastAction = plt_paLastAction;
  __int64 llTag = plt_paLastAction.pa_llCreated += paDelta.pa_llCreated;
  for (INDEX i = 0; i < static_cast<INDEX>(sizeof(CPlayerAction)); i++) {
    ((UBYTE*)&plt_paLastAction)[i] ^= ((UBYTE*)&paDelta)[i];
  }
  plt_paLastAction.pa_llCreated = llTag;

  FLOAT fLatency = 0.0f;
  // if the player is local
  if (_pNetwork->IsPlayerLocal(plt_penPlayerEntity)) {
    // calculate latency
    __int64 llmsNow = _pTimer->GetHighPrecisionTimer().GetMilliseconds();
    __int64 llmsCreated = plt_paLastAction.pa_llCreated;
    fLatency = FLOAT(DOUBLE(llmsNow-llmsCreated)/1000.0f);
    if (plt_paLastAction.pa_llCreated==plt_paPreLastAction.pa_llCreated) {
      _pNetwork->AddNetGraphValue(NGET_REPLICATEDACTION, fLatency);
    } else {
      CPlayerAction *ppaOlder = plt_abPrediction.GetLastOlderThan(plt_paLastAction.pa_llCreated);
      if (ppaOlder!=NULL && ppaOlder->pa_llCreated!=plt_paPreLastAction.pa_llCreated) {
        _pNetwork->AddNetGraphValue(NGET_SKIPPEDACTION, 1.0f);
      }
      extern FLOAT net_tmLatency;
      net_tmLatency = fLatency;
      _pNetwork->AddNetGraphValue(NGET_ACTION, fLatency);
    }
  }

  // if the entity is not deleted
  if (!(plt_penPlayerEntity->en_ulFlags&ENF_DELETED)) {
    // call the player DLL class to apply the new action to the entity
    plt_penPlayerEntity->ApplyAction(plt_paLastAction, fLatency);
  }

  extern INDEX cli_iPredictionFlushing;
  if (cli_iPredictionFlushing==2 || cli_iPredictionFlushing==3) {
    plt_abPrediction.RemoveOldest();
  }
}

/* Remember prediction action. */
void CPlayerTarget::PrebufferActionPacket(const CPlayerAction &paPrediction)
{
  ASSERT(plt_bActive);
  // synchronize access to actions
  CTSingleLock slActions(&plt_csAction, TRUE);

  // buffer the action
  plt_abPrediction.AddAction(paPrediction);
}

// flush prediction actions that were already processed
void CPlayerTarget::FlushProcessedPredictions(void)
{
  CTSingleLock slActions(&plt_csAction, TRUE);
  extern INDEX cli_iPredictionFlushing;
  if (cli_iPredictionFlushing==1) {
    // flush all actions that were already processed
    plt_abPrediction.FlushUntilTime(plt_paLastAction.pa_llCreated);
  } else if (cli_iPredictionFlushing==3) {
    // flush older actions that were already processed
    plt_abPrediction.FlushUntilTime(plt_paPreLastAction.pa_llCreated);
  }
}

// get maximum number of actions that can be predicted
INDEX CPlayerTarget::GetNumberOfPredictions(void)
{
  CTSingleLock slActions(&plt_csAction, TRUE);
  // return current count
  return plt_abPrediction.GetCount();
}
  
/* Apply predicted action with given index. */
void CPlayerTarget::ApplyPredictedAction(INDEX iAction, FLOAT fFactor)
{
  // synchronize access to actions
  CTSingleLock slActions(&plt_csAction, TRUE);

  CPlayerAction pa;

  // if the player is local
  if (_pNetwork->IsPlayerLocal(plt_penPlayerEntity)) {
    // get the action from buffer
    plt_abPrediction.GetActionByIndex(iAction, pa);

  // if the player is not local
  } else {
    // reuse last action
    if (cli_bLerpActions) {
      pa.Lerp(plt_paPreLastAction, plt_paLastAction, fFactor);
    } else {
      pa = plt_paLastAction;
    }
  }

  // get the player's predictor
  if (!plt_penPlayerEntity->IsPredicted()) {
    return;
  }

  CEntity *penPredictor = plt_penPlayerEntity->GetPredictor();
  if (penPredictor==NULL || penPredictor==plt_penPlayerEntity) {
    return;
  }

  // apply a prediction action packet to the entity's predictor
  ((CPlayerEntity*)penPredictor)->ApplyAction(pa, 0.0f);
}
