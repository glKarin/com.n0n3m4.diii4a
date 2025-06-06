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

205
%{
#include "EntitiesMP/StdH/StdH.h"
extern INDEX ent_bReportBrokenChains;
%}

class CTrigger: CRationalEntity {
name      "Trigger";
thumbnail "Thumbnails\\Trigger.tbn";
features  "HasName", "IsTargetable";

properties:

  1 CTString m_strName              "Name" 'N' = "Trigger",         // class name

  3 CEntityPointer m_penTarget1     "Target 01" 'T' COLOR(C_RED|0xFF),                 // send event to entity
  4 CEntityPointer m_penTarget2     "Target 02" 'Y' COLOR(C_RED|0xFF),
  5 CEntityPointer m_penTarget3     "Target 03" 'U' COLOR(C_RED|0xFF),
  6 CEntityPointer m_penTarget4     "Target 04" 'I' COLOR(C_RED|0xFF),
  7 CEntityPointer m_penTarget5     "Target 05" 'O' COLOR(C_RED|0xFF),
 20 CEntityPointer m_penTarget6     "Target 06" COLOR(C_RED|0xFF),
 21 CEntityPointer m_penTarget7     "Target 07" COLOR(C_RED|0xFF),
 22 CEntityPointer m_penTarget8     "Target 08" COLOR(C_RED|0xFF),
 23 CEntityPointer m_penTarget9     "Target 09" COLOR(C_RED|0xFF),
 24 CEntityPointer m_penTarget10    "Target 10" COLOR(C_RED|0xFF),
  8 enum EventEType m_eetEvent1     "Event type Target 01" 'G' = EET_TRIGGER,  // type of event to send
  9 enum EventEType m_eetEvent2     "Event type Target 02" 'H' = EET_TRIGGER,
 10 enum EventEType m_eetEvent3     "Event type Target 03" 'J' = EET_TRIGGER,
 11 enum EventEType m_eetEvent4     "Event type Target 04" 'K' = EET_TRIGGER,
 12 enum EventEType m_eetEvent5     "Event type Target 05" 'L' = EET_TRIGGER,
 50 enum EventEType m_eetEvent6     "Event type Target 06" = EET_TRIGGER,
 51 enum EventEType m_eetEvent7     "Event type Target 07" = EET_TRIGGER,
 52 enum EventEType m_eetEvent8     "Event type Target 08" = EET_TRIGGER,
 53 enum EventEType m_eetEvent9     "Event type Target 09" = EET_TRIGGER,
 54 enum EventEType m_eetEvent10    "Event type Target 10" = EET_TRIGGER,
 13 CTStringTrans m_strMessage      "Message" 'M' = "",     // message
 14 FLOAT m_fMessageTime            "Message time" = 3.0f,  // how long is message on screen
 15 enum MessageSound m_mssMessageSound "Message sound" = MSS_NONE, // message sound
 16 FLOAT m_fScore                  "Score" 'S' = 0.0f,

 30 FLOAT m_fWaitTime             "Wait" 'W' = 0.0f,          // wait before send events
 31 BOOL m_bAutoStart             "Auto start" 'A' = FALSE,   // trigger auto starts
 32 INDEX m_iCount                "Count" 'C' = 1,            // count before send events
 33 BOOL m_bUseCount              "Count use" = FALSE,        // use count to send events
 34 BOOL m_bReuseCount            "Count reuse" = FALSE,      // reuse counter after reaching 0
 35 BOOL m_bTellCount             "Count tell" = FALSE,       // tell remaining count to player
 36 BOOL m_bActive                "Active" 'V' = TRUE,        // starts in active/inactive state
 37 RANGE m_fSendRange            "Send Range" 'R' = 1.0f,    // for sending event in range
 38 enum EventEType m_eetRange    "Event type Range" = EET_IGNORE,  // type of event to send in range
 
 40 INDEX m_iCountTmp = 0,          // use count value to determine when to send events
 41 CEntityPointer m_penCaused,     // who touched it last time
 42 INDEX m_ctMaxTrigs            "Max trigs" 'X' = -1, // how many times could trig


components:

  1 model   MODEL_MARKER     "Models\\Editor\\Trigger.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\Camera.tex"


functions:                                        

  // get target 1 (there is no property 'm_penTarget')
  CEntity *GetTarget(void) const
  { 
    return m_penTarget1;
  }


  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    // initial
    SLONG slUsedMemory = sizeof(CTrigger) - sizeof(CRationalEntity) + CRationalEntity::GetUsedMemory();
    // add some more
    slUsedMemory += m_strMessage.Length();
    slUsedMemory += m_strName.Length();
    slUsedMemory += 1* sizeof(CSoundObject);
    return slUsedMemory;
  }



procedures:

  SendEventToTargets() {
    // if needed wait some time before event is send
    if (m_fWaitTime > 0.0f) {
      wait (m_fWaitTime) {
        on (EBegin) : { resume; }
        on (ETimer) : { stop; }
        on (EDeactivate) : { pass; }
        otherwise(): { resume; }
      }
    }

    // send event to all targets
    SendToTarget(m_penTarget1, m_eetEvent1, m_penCaused);
    SendToTarget(m_penTarget2, m_eetEvent2, m_penCaused);
    SendToTarget(m_penTarget3, m_eetEvent3, m_penCaused);
    SendToTarget(m_penTarget4, m_eetEvent4, m_penCaused);
    SendToTarget(m_penTarget5, m_eetEvent5, m_penCaused);
    SendToTarget(m_penTarget6, m_eetEvent6, m_penCaused);
    SendToTarget(m_penTarget7, m_eetEvent7, m_penCaused);
    SendToTarget(m_penTarget8, m_eetEvent8, m_penCaused);
    SendToTarget(m_penTarget9, m_eetEvent9, m_penCaused);
    SendToTarget(m_penTarget10, m_eetEvent10, m_penCaused);

    // if there is event to send in range
    if (m_eetRange!=EET_IGNORE) {
      // send in range also
      SendInRange(this, m_eetRange, FLOATaabbox3D(GetPlacement().pl_PositionVector, m_fSendRange));
    }

    // if trigger gives score
    if (m_fScore>0) {
      CEntity *penCaused = FixupCausedToPlayer(this, m_penCaused);

      // if we have causer
      if (penCaused!=NULL) {
        // send the score
        EReceiveScore eScore;
        eScore.iPoints = (INDEX) m_fScore;
        penCaused->SendEvent(eScore);
        penCaused->SendEvent(ESecretFound());
      }

      // kill score to never be reported again
      m_fScore = 0;
    }
    if (m_strMessage!="") {
      PrintCenterMessage(this, m_penCaused, 
        TranslateConst(m_strMessage), 
        m_fMessageTime, m_mssMessageSound);
    }

    // if max trig count is used for counting
    if(m_ctMaxTrigs > 0)
    {
      // decrease count
      m_ctMaxTrigs-=1;
      // if we trigged max times
      if( m_ctMaxTrigs <= 0)
      {
        // cease to exist
        Destroy();
      }
    }
    return;
  };

  Active() {
    ASSERT(m_bActive);
    // store count start value
    m_iCountTmp = m_iCount;

    //main loop
    wait() {
      on (EBegin) : { 
        // if auto start send event on init
        if (m_bAutoStart) {
          call SendEventToTargets();
        }
        resume;
      }
      // re-roots start events as triggers
      on (EStart eStart) : {
        SendToTarget(this, EET_TRIGGER, eStart.penCaused);
        resume;
      }
      // cascade trigger
      on (ETrigger eTrigger) : {
        m_penCaused = eTrigger.penCaused;
        // if using count
        if (m_bUseCount) {
          // count reach lowest value
          if (m_iCountTmp > 0) {
            // decrease count
            m_iCountTmp--;
            // send event if count is less than one (is zero)
            if (m_iCountTmp < 1) {
              if (m_bReuseCount) {
                m_iCountTmp = m_iCount;
              } else {
                m_iCountTmp = 0;
              }
              call SendEventToTargets();
            } else if (m_bTellCount) {
              CTString strRemaining;
              strRemaining.PrintF(TRANSV("%d more to go..."), m_iCountTmp);
              PrintCenterMessage(this, m_penCaused, strRemaining, 3.0f, MSS_INFO);
            }
          }
        // else send event
        } else {
          call SendEventToTargets();
        }
        resume;
      }
      // if deactivated
      on (EDeactivate) : {
        // go to inactive state
        m_bActive = FALSE;
        jump Inactive();
      }
    }
  };
  Inactive() {
    ASSERT(!m_bActive);
    while (TRUE) {
      // wait 
      wait() {
        // if activated
        on (EActivate) : {
          // go to active state
          m_bActive = TRUE;
          jump Active();
        }
        otherwise() : {
          resume;
        };
      };
      
      // wait a bit to recover
      autowait(0.1f);
    }
  }

  Main() {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    m_fSendRange = ClampDn(m_fSendRange, 0.01f);

    // spawn in world editor
    autowait(0.1f);

    // go into active or inactive state
    if (m_bActive) {
      jump Active();
    } else {
      jump Inactive();
    }

    // cease to exist
    Destroy();

    return;
  };
};
