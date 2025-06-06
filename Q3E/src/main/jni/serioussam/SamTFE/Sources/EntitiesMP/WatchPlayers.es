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

702
%{
#include "EntitiesMP/StdH/StdH.h"
%}

class CWatchPlayers: CRationalEntity {
name      "Watch Players";
thumbnail "Thumbnails\\WatchPlayers.tbn";
features "HasName", "IsTargetable";

properties:
  1 CEntityPointer m_penOwner "Owner/Target" 'O' COLOR(C_dBROWN|0xFF),             // entity which owns it / target
 10 CEntityPointer m_penFar   "Far Target" 'F' COLOR(C_BLACK|0xFF),                // entity which owns it / target
  2 FLOAT m_fWaitTime         "Wait time" 'W' = 0.1f,         // watch time
  3 RANGE m_fDistance         "Watch distance" 'D' = 100.0f,  // distance when player is seen
  4 BOOL m_bRangeWatcher      "Range watcher" 'R' = TRUE,    // range watcher
  5 enum EventEType m_eetEventClose  "Close Event type" 'T' = EET_TRIGGER,   // type of event to send
  6 enum EventEType m_eetEventFar    "Far Event type" 'Y' = EET_ENVIRONMENTSTOP,      // type of event to send
  7 CEntityPointer m_penCurrentWatch,
  8 BOOL m_bActive  "Active" 'A' = TRUE,
  9 CTString m_strName "Name" 'N' = "",

components:
  1 model   MODEL_WATCHPLAYERS      "Models\\Editor\\WatchPlayers.mdl",
  2 texture TEXTURE_WATCHPLAYERS    "Models\\Editor\\WatchPlayers.tex"

functions:
/************************************************************
 *                      USER FUNCTIONS                      *
 ************************************************************/
  // check if any player is close
  BOOL IsAnyPlayerClose(void) {
    // far enough to not move at all
    FLOAT fClosest = 100000.0f;
    FLOAT fDistance;

    m_penCurrentWatch = NULL;
    // for all players
    for (INDEX iPlayer=0; iPlayer<GetMaxPlayers(); iPlayer++) {
      CEntity *penPlayer = GetPlayerEntity(iPlayer);
      // if player is alive and visible
      if (penPlayer!=NULL && penPlayer->GetFlags()&ENF_ALIVE && !(penPlayer->GetFlags()&ENF_INVISIBLE)) {
        fDistance = 100000.0f;
        if (m_bRangeWatcher) {
          // calculate distance to player from wathcer
          fDistance = (penPlayer->GetPlacement().pl_PositionVector-
                       GetPlacement().pl_PositionVector).Length();
        } else {
          if (m_penOwner!=NULL) {
            // calculate distance to player from owner
            fDistance = (penPlayer->GetPlacement().pl_PositionVector-
                         m_penOwner->GetPlacement().pl_PositionVector).Length();
          }
        }
        if (fDistance<fClosest) {
          fClosest = fDistance;
          m_penCurrentWatch = penPlayer;
        }
      }
    }
    // if close enough start moving
    return (fClosest < m_fDistance);
  };

  // send close event
  void SendCloseEvent(void) {
    // send range event
    if (m_bRangeWatcher && m_penOwner==NULL) {
//      SendInRange(this, m_eetEventClose, FLOATaabbox3D(GetPlacement().pl_PositionVector, m_fDistance));
    // send to owner
    } else {
      SendToTarget(m_penOwner, m_eetEventClose, m_penCurrentWatch);
    }
  };

  // send far event
  void SendFarEvent(void) {
    // send range event
    if (m_bRangeWatcher && m_penOwner==NULL) {
//      SendInRange(this, m_eetEventFar, FLOATaabbox3D(GetPlacement().pl_PositionVector, m_fDistance));
    // send to owner
    } else {
      if (m_penFar!=NULL) {
        SendToTarget(m_penFar, m_eetEventFar);
      } else {
        SendToTarget(m_penOwner, m_eetEventFar);
      }
    }
  };

procedures:
  // main (initialization)
  Main(EVoid) {
    // init as nothing
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_WATCHPLAYERS);
    SetModelMainTexture(TEXTURE_WATCHPLAYERS);

    if (m_fWaitTime<0.1f) {
      m_fWaitTime=0.1f;
    }

    if (m_bActive) {
      jump Active();
    } else {
      jump Inactive();
    }
  };

  Active()
  {
    autocall FarWatch() EDeactivate;
    jump Inactive();
  }

  Inactive()
  {
    wait() {
      on (EActivate) : {
        stop;
      };
      otherwise() : {
        resume;
      };
    }
    jump Active();
  }

  // player is close
  CloseWatch(EVoid) {
    while (TRUE) {
      wait(m_fWaitTime) {
        on (EBegin) : {
          if (!IsAnyPlayerClose()) {
            // notify for player off range
            SendFarEvent();
            jump FarWatch();
          }
          resume;
        }
        on (ETimer) : { stop; }
      }
    }
  };
  
  // player is far
  FarWatch(EVoid) {
    while (TRUE) {
      wait(m_fWaitTime) {
        on (EBegin) : {
          if (IsAnyPlayerClose()) {
            // notify for player in range
            SendCloseEvent();
            jump CloseWatch();
          }
          resume;
        }
        on (ETimer) : { stop; }
      }
    }
  };
};