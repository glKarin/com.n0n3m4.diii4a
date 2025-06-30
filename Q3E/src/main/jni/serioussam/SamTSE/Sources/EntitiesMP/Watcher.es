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

700
%{
#include "EntitiesMP/StdH/StdH.h"

#include "EntitiesMP/EnemyBase.h"
%}

// input parameter for watcher
event EWatcherInit {
  CEntityPointer penOwner,        // who owns it
};

// entity is seen
event EWatch {
  CEntityPointer penSeen,
};

class export CWatcher : CRationalEntity {
name      "Watcher";
thumbnail "";
features  "CanBePredictable";


properties:

  1 CEntityPointer m_penOwner,  // entity which owns it
  2 FLOAT m_tmDelay = 5.0f,     // delay between checking moments - set depending on distance of closest player

 20 FLOAT m_fClosestPlayer = UpperLimit(0.0f),  // distance from closest player to owner of this watcher
 21 INDEX m_iPlayerToCheck = 0,   // sequence number for checking next player in each turn


components:


functions:

  class CEnemyBase *GetOwner(void)
  {
    ASSERT(m_penOwner!=NULL);
    return (CEnemyBase*) m_penOwner.ep_pen;
  }

  // find one player number by random
  INDEX GetRandomPlayer(void)
  {
//    CPrintF("Getting random number... ");
    // get maximum number of players in game
    INDEX ctMaxPlayers = GetMaxPlayers();
    // find actual number of players
    INDEX ctActivePlayers = 0;
    {for(INDEX i=0; i<ctMaxPlayers; i++) {
      if (GetPlayerEntity(i)!=NULL) {
        ctActivePlayers++;
      }
    }}
//    CPrintF("active players %d, ", ctActivePlayers);
    // if none
    if (ctActivePlayers==0) {
      // return first index anyway
      return 0;
    }


    // choose one by random
    INDEX iChosenActivePlayer = IRnd()%ctActivePlayers;
//    CPrintF("chosen %d, ", iChosenActivePlayer);

    // find its physical index
    INDEX iActivePlayer = 0;
    {for(INDEX i=0; i<ctMaxPlayers; i++) {
      if (GetPlayerEntity(i)!=NULL) {
        if (iActivePlayer==iChosenActivePlayer) {
//          CPrintF("actual index %d\n", iActivePlayer);
          return i;
        }
        iActivePlayer++;
      }
    }}
    ASSERT(FALSE);
    return 0;
  }

  // find closest player
  CEntity *FindClosestPlayer(void)
  {
    CEntity *penClosestPlayer = NULL;
    FLOAT fClosestPlayer = UpperLimit(0.0f);
    // for all players
    for (INDEX iPlayer=0; iPlayer<GetMaxPlayers(); iPlayer++) {
      CEntity *penPlayer = GetPlayerEntity(iPlayer);
      // if player is alive and visible
      if (penPlayer!=NULL && penPlayer->GetFlags()&ENF_ALIVE && !(penPlayer->GetFlags()&ENF_INVISIBLE)) {
        // calculate distance to player
        FLOAT fDistance = 
          (penPlayer->GetPlacement().pl_PositionVector-m_penOwner->GetPlacement().pl_PositionVector).Length();
        // update if closer
        if (fDistance<fClosestPlayer) {
          fClosestPlayer = fDistance;
          penClosestPlayer = penPlayer;
        }
      }
    }
    // if no players found
    if (penClosestPlayer==NULL) {
      // behave as if very close - must check for new ones
      fClosestPlayer = 10.0f;
    }
    m_fClosestPlayer = fClosestPlayer;
    return penClosestPlayer;
  }

  // notify owner that a player has been seen
  void SendWatchEvent(CEntity *penPlayer)
  {
    EWatch eWatch;
    eWatch.penSeen = penPlayer;
    m_penOwner->SendEvent(eWatch);
  }

  void CheckIfPlayerVisible(void)
  {
    // if the owner is blind
    if( GetOwner()->m_bBlind) {
      // don't even bother checking
      return;
    }

    // get maximum number of players in game
    INDEX ctPlayers = GetMaxPlayers();
    // find first one after current sequence
    CEntity *penPlayer = NULL;
    m_iPlayerToCheck = (m_iPlayerToCheck+1)%ctPlayers;
    INDEX iFirstChecked = m_iPlayerToCheck;
    FOREVER {
      penPlayer = GetPlayerEntity(m_iPlayerToCheck);
      if (penPlayer!=NULL) {
        break;
      }
      m_iPlayerToCheck++;
      m_iPlayerToCheck%=ctPlayers;
      if (m_iPlayerToCheck==iFirstChecked) {
        return; // we get here if there are no players at all
      }
    }

    // if this one is dead or invisible
    if (!(penPlayer->GetFlags()&ENF_ALIVE) || (penPlayer->GetFlags()&ENF_INVISIBLE)) {
      // do nothing
      return;
    }

    // if inside view angle and visible
    if (GetOwner()->SeeEntity(penPlayer, Cos(GetOwner()->m_fViewAngle/2.0f))) {
      // send event to owner
      SendWatchEvent(penPlayer);
    }
  };

  // set new watch time
  void SetWatchDelays(void)
  {
    const FLOAT tmMinDelay = 0.1f;   // delay at closest distance
    const FLOAT tmSeeDelay = 5.0f;   // delay at see distance
    const FLOAT tmTick = _pTimer->TickQuantum;
    FLOAT fSeeDistance  = GetOwner()->m_fIgnoreRange;
    FLOAT fNearDistance = Min(GetOwner()->m_fStopDistance, GetOwner()->m_fCloseDistance);

    // if closer than near distance
    if (m_fClosestPlayer<=fNearDistance) {
      // always use minimum delay
      m_tmDelay = tmMinDelay;
    // if further than near distance
    } else {
      // interpolate between near and see
      m_tmDelay = tmMinDelay+
        (m_fClosestPlayer-fNearDistance)*(tmSeeDelay-tmMinDelay)/(fSeeDistance-fNearDistance);
      // round to nearest tick
      m_tmDelay = floor(m_tmDelay/tmTick)*tmTick;
    }
  };

  // watch
  void Watch(void)
  {
    // remember original distance
    FLOAT fOrgDistance = m_fClosestPlayer;

    // find closest player
    CEntity *penClosest = FindClosestPlayer();

    FLOAT fSeeDistance  = GetOwner()->m_fIgnoreRange;
    FLOAT fStopDistance  = Max(fSeeDistance*1.5f, GetOwner()->m_fActivityRange);

    // if players exited enemy's scope
    if (fOrgDistance<fStopDistance && m_fClosestPlayer>=fStopDistance) {
      // stop owner
      m_penOwner->SendEvent(EStop());
    // if players entered enemy's scope
    } else if (fOrgDistance>=fStopDistance && m_fClosestPlayer<fStopDistance) {
      // start owner
      m_penOwner->SendEvent(EStart());
    }

    // if the closest player is close enough to be seen
    if (m_fClosestPlayer<fSeeDistance) {
      // check for seeing any of the players
      CheckIfPlayerVisible();
    }

    // if the closest player is inside sense range
    FLOAT fSenseRange = GetOwner()->m_fSenseRange;
    if (penClosest!=NULL && fSenseRange>0 && m_fClosestPlayer<fSenseRange) {
      // detect it immediately
      SendWatchEvent(penClosest);
    }
  
    // set new watch time
    SetWatchDelays();
  };

  // this is called directly from enemybase to check if another player has come too close
  CEntity *CheckCloserPlayer(CEntity *penCurrentTarget, FLOAT fRange)
  {
    // if the owner is blind
    if( GetOwner()->m_bBlind) {
      // don't even bother checking
      return NULL;
    }

    CEntity *penClosestPlayer = NULL;
    FLOAT fClosestPlayer = 
      (penCurrentTarget->GetPlacement().pl_PositionVector-m_penOwner->GetPlacement().pl_PositionVector).Length();
    fClosestPlayer = Min(fClosestPlayer, fRange);  // this is maximum considered range

    // for all other players
    for (INDEX iPlayer=0; iPlayer<GetMaxPlayers(); iPlayer++) {
      CEntity *penPlayer = GetPlayerEntity(iPlayer);
      if (penPlayer==NULL || penPlayer==penCurrentTarget) {
        continue;
      }
      // if player is alive and visible
      if ((penPlayer->GetFlags()&ENF_ALIVE) && !(penPlayer->GetFlags()&ENF_INVISIBLE)) {
        // calculate distance to player
        FLOAT fDistance = 
          (penPlayer->GetPlacement().pl_PositionVector-m_penOwner->GetPlacement().pl_PositionVector).Length();
        // if closer than current and you can see him
        if (fDistance<fClosestPlayer && 
            GetOwner()->SeeEntity(penPlayer, Cos(GetOwner()->m_fViewAngle/2.0f))) {
          // update
          fClosestPlayer = fDistance;
          penClosestPlayer = penPlayer;
        }
      }
    }

    return penClosestPlayer;
  }

  // this is called directly from enemybase to attack multiple players (for really big enemies)
  CEntity *CheckAnotherPlayer(CEntity *penCurrentTarget)
  {
    // if the owner is blind, or no current target
    if( GetOwner()->m_bBlind || penCurrentTarget==NULL) {
      // don't even check
      return NULL;
    }

    // get allowed distance
    CEntity *penClosestPlayer = NULL;
    FLOAT fCurrentDistance = 
      (penCurrentTarget->GetPlacement().pl_PositionVector-m_penOwner->GetPlacement().pl_PositionVector).Length();
    FLOAT fRange = fCurrentDistance*1.5f;

    // find a random offset to start searching
    INDEX iOffset = GetRandomPlayer();

    // for all other players
    INDEX ctPlayers = GetMaxPlayers();
    for (INDEX iPlayer=0; iPlayer<ctPlayers; iPlayer++) {
      CEntity *penPlayer = GetPlayerEntity((iPlayer+iOffset)%ctPlayers);
      if (penPlayer==NULL || penPlayer==penCurrentTarget) {
        continue;
      }
      // if player is alive and visible
      if ((penPlayer->GetFlags()&ENF_ALIVE) && !(penPlayer->GetFlags()&ENF_INVISIBLE)) {
        // calculate distance to player
        FLOAT fDistance = 
          (penPlayer->GetPlacement().pl_PositionVector-m_penOwner->GetPlacement().pl_PositionVector).Length();
        // if inside allowed range and visible
        if (fDistance<fRange && 
            GetOwner()->SeeEntity(penPlayer, Cos(GetOwner()->m_fViewAngle/2.0f))) {
          // attack that one
          return penPlayer;
        }
      }
    }

    return penCurrentTarget;
  }



  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    return( sizeof(CWatcher) - sizeof(CRationalEntity) + CRationalEntity::GetUsedMemory());
  }


procedures:


  // watching
  Active() {
    // repeat
    while (TRUE) {
      // check all players
      Watch();
      // wait for given delay
      wait(m_tmDelay) {
        on (EBegin) : { resume; }
        on (ETimer) : { stop; }
        // stop looking
        on (EStop) : { jump Inactive(); }
        // force re-checking if receiving start or teleport
        on (EStart) : { stop; }
        on (ETeleport) : { stop; }
      }
    }
  };

  // not watching
  Inactive(EVoid) {
    wait() {
      on (EBegin) : { resume; }
      on (EStart) : { jump Active(); }
    }
  };

  // dummy mode
  Dummy(EVoid)
  {
    // ignores all events forever
    wait() {
      on (EBegin) : { resume; }
      otherwise() : { resume; };
    };
  }

  Main(EWatcherInit eInit) {
    // remember the initial parameters
    ASSERT(eInit.penOwner!=NULL);
    m_penOwner = eInit.penOwner;

    // init as nothing
    InitAsVoid();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // if in flyover game mode
    if (GetSP()->sp_gmGameMode == CSessionProperties::GM_FLYOVER) {
      // go to dummy mode
      jump Dummy();
      // NOTE: must not destroy self, because owner has a pointer
    }

    // generate random number of player to check next 
    // (to provide even distribution of enemies among players)
    m_iPlayerToCheck = GetRandomPlayer()-1;

    // start in disabled state
    autocall Inactive() EEnd;

    // cease to exist
    Destroy();

    return;
  };
};