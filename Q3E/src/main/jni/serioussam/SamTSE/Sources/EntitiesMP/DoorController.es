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

221
%{
#include "EntitiesMP/StdH/StdH.h"
#include <Engine/CurrentVersion.h>
%}

uses "EntitiesMP/KeyItem";
uses "EntitiesMP/Player";

enum DoorType {
  0 DT_AUTO       "Auto",       // opens automatically
  1 DT_TRIGGERED  "Triggered",  // opens when triggered
  2 DT_LOCKED     "Locked",     // requires a key
  3 DT_TRIGGEREDAUTO     "Triggered Auto",     // opens automatically after being triggered
};

class CDoorController : CRationalEntity {
name      "DoorController";
thumbnail "Thumbnails\\DoorController.tbn";
features  "HasName", "IsTargetable";


properties:

  1 CTString m_strName          "Name" 'N' = "DoorController",
  2 CTString m_strDescription = "",
  3 CEntityPointer m_penTarget1  "Target1" 'T' COLOR(C_MAGENTA|0xFF),
  4 CEntityPointer m_penTarget2  "Target2" COLOR(C_MAGENTA|0xFF),
  5 FLOAT m_fWidth              "Width"  'W' = 2.0f,
  6 FLOAT m_fHeight             "Height" 'H' = 3.0f,
  7 BOOL m_bPlayersOnly         "Players Only" 'P' = TRUE,
  8 enum DoorType m_dtType      "Type" 'Y' = DT_AUTO,
  9 CTStringTrans m_strLockedMessage "Locked message" 'L' = "",
  13 CEntityPointer m_penLockedTarget  "Locked target" COLOR(C_dMAGENTA|0xFF),   // target to trigger when locked
  12 enum KeyItemType m_kitKey  "Key" 'K' = KIT_BOOKOFWISDOM,  // key type (for locked door)
  14 BOOL m_bTriggerOnAnything "Trigger on anything" = FALSE,
  15 BOOL m_bActive "Active" 'A' = TRUE,    // automatic door function can be activated/deactivated

  10 BOOL m_bLocked = FALSE,  // for lock/unlock door
  11 CEntityPointer m_penCaused,    // for trigger relaying


components:

  1 model   MODEL_DOORCONTROLLER     "Models\\Editor\\DoorController.mdl",
  2 texture TEXTURE_DOORCONTROLLER   "Models\\Editor\\DoorController.tex",


functions:

  CEntity *GetTarget(void) const { return m_penTarget1; };

  const CTString &GetDescription(void) const
  {
    if (m_penTarget1!=NULL && m_penTarget2!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s,%s", 
        (const char *) m_penTarget1->GetName(), (const char *) m_penTarget2->GetName());
    } else if (m_penTarget1!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s", 
        (const char *) m_penTarget1->GetName());
    } else {
      ((CTString&)m_strDescription).PrintF("-><none>");
    }
    return m_strDescription;
  }

  // test if this door reacts on this entity
  BOOL CanReactOnEntity(CEntity *pen)
  {
    if (pen==NULL) {
      return FALSE;
    }
    // never react on non-live or dead entities
    if (!(pen->GetFlags()&ENF_ALIVE)) {
      return FALSE;
    }

    if (m_bPlayersOnly && !IsDerivedFromClass(pen, "Player")) {
      return FALSE;
    }

    return TRUE;
  }

  // test if this door can be triggered by this entity
  BOOL CanTriggerOnEntity(CEntity *pen)
  {
    return m_bTriggerOnAnything || CanReactOnEntity(pen);
  }

  void TriggerDoor(void)
  {
    if (m_penTarget1!=NULL) {
      SendToTarget(m_penTarget1, EET_TRIGGER, m_penCaused);
    }
    if (m_penTarget2!=NULL) {
      SendToTarget(m_penTarget2, EET_TRIGGER, m_penCaused);
    }
  }

  // apply mirror and stretch to the entity
  void MirrorAndStretch(FLOAT fStretch, BOOL bMirrorX)
  {
    // stretch its ranges
    m_fWidth*=fStretch;
    m_fHeight*=fStretch;
  }


  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    // initial
    SLONG slUsedMemory = sizeof(CDoorController) - sizeof(CRationalEntity) + CRationalEntity::GetUsedMemory();
    // add some more
    slUsedMemory += m_strDescription.Length();
    slUsedMemory += m_strName.Length();
    slUsedMemory += m_strLockedMessage.Length();
    return slUsedMemory;
  }


procedures:

  // entry point for automatic functioning
  DoorAuto()
  {
    // go into active or inactive state
    if (m_bActive) {
      jump DoorAutoActive();
    } else {
      jump DoorAutoInactive();
    }
  }

  // automatic door active state
  DoorAutoActive()
  {
    ASSERT(m_bActive);
    while (TRUE) {
      // wait 
      wait() {
        // when someone enters
        on (EPass ePass) : {
          // if he can open the door
          if (CanReactOnEntity(ePass.penOther)) {
            // do it
            m_penCaused = ePass.penOther;
            TriggerDoor();
              
            // this is a very ugly fix for cooperative not finishing in the demo level
            // remove this when not needed any more!!!!
            if(_SE_DEMO && GetSP()->sp_bCooperative && !GetSP()->sp_bSinglePlayer) {
              if (m_strName=="Appear gold amon") {
                CPlayer *penPlayer = (CPlayer*)&*ePass.penOther;
                penPlayer->SetGameEnd();
              }
            }

            resume;
          }
          resume;
        }
        // if door is deactivated
        on (EDeactivate) : {
          // go to inactive state
          m_bActive = FALSE;
          jump DoorAutoInactive();
        }
        otherwise() : {
          resume;
        };
      };
      
      // wait a bit to recover
      autowait(0.1f);
    }
  }

  // automatic door inactive state
  DoorAutoInactive()
  {
    ASSERT(!m_bActive);
    while (TRUE) {
      // wait 
      wait() {
        // if door is activated
        on (EActivate) : {
          // go to active state
          m_bActive = TRUE;
          jump DoorAutoActive();
        }
        otherwise() : {
          resume;
        };
      };
      
      // wait a bit to recover
      autowait(0.1f);
    }
  }

  // door when do not function anymore
  DoorDummy()
  {
    wait() {
      on (EBegin) : {
        resume;
      }
      otherwise() : {
        resume;
      };
    }
  }

  // door that wait to be triggered to open
  DoorTriggered()
  {
    while (TRUE) {
      // wait to someone enter
      wait() {
        on (EPass ePass) : {
          if (CanReactOnEntity(ePass.penOther)) {
            if (m_strLockedMessage!="") {
              PrintCenterMessage(this, ePass.penOther, TranslateConst(m_strLockedMessage), 3.0f, MSS_INFO);
            }
            if (m_penLockedTarget!=NULL) {
              SendToTarget(m_penLockedTarget, EET_TRIGGER, ePass.penOther);
            }
            resume;
          }
        }
        on (ETrigger eTrigger) : {
          m_penCaused = eTrigger.penCaused;
          TriggerDoor();
          jump DoorDummy();
        }
        otherwise() : {
          resume;
        };
      };
      
      // wait a bit to recover
      autowait(0.1f);
    }
  }

  // door that need a key to be unlocked to open
  DoorLocked()
  {
    while (TRUE) {
      // wait to someone enter
      wait() {
        on (EPass ePass) : {
          if (IsDerivedFromClass(ePass.penOther, "Player")) {
            CPlayer *penPlayer = (CPlayer*)&*ePass.penOther;
            // if he has the key
            ULONG ulKey = (1<<INDEX(m_kitKey));
            if (penPlayer->m_ulKeys&ulKey) {
              // use the key
              penPlayer->m_ulKeys&=~ulKey;
              // open the dook
              TriggerDoor();

              /*
              // tell the key bearer that the key was used
              CTString strMsg;
              strMsg.PrintF(TRANSV("%s used"), GetKeyName(m_kitKey));
              PrintCenterMessage(this, ePass.penOther, strMsg, 3.0f, MSS_INFO);
              */
              // become automatic door
              jump DoorAuto();
            // if he has no key
            } else {
              if (m_penLockedTarget!=NULL) {
                SendToTarget(m_penLockedTarget, EET_TRIGGER, ePass.penOther);
              }
            }
            resume;
          }
        }
        otherwise() : {
          resume;
        };
      };
      
      // wait a bit to recover
      autowait(0.1f);
    }
  }

  // door that need to be triggered to start working automatically
  DoorTriggeredAuto()
  {
    while (TRUE) {
      // wait to be triggered
      wait() {
        on (ETrigger eTrigger) : {
          // become auto door
          jump DoorAuto();
        }
        on (EPass ePass) : {
          if (CanReactOnEntity(ePass.penOther)) {
            if (m_strLockedMessage!="") {
              PrintCenterMessage(this, ePass.penOther, TranslateConst(m_strLockedMessage), 3.0f, MSS_INFO);
            }
            if (m_penLockedTarget!=NULL) {
              SendToTarget(m_penLockedTarget, EET_TRIGGER, ePass.penOther);
            }
          }
          resume;
        }
        otherwise() : {
          resume;
        };
      };
      
      // wait a bit to recover
      autowait(0.1f);
    }
  }

  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_TOUCHMODEL);

    // set appearance
    GetModelObject()->StretchModel(FLOAT3D(m_fWidth, m_fHeight, m_fWidth));
    SetModel(MODEL_DOORCONTROLLER);
    SetModelMainTexture(TEXTURE_DOORCONTROLLER);
    ModelChangeNotify();

    // don't start in wed
    autowait(0.1f);

    // dispatch to aproppriate loop
    switch(m_dtType) {
    case DT_AUTO: {
      jump DoorAuto();
                  } break;
    case DT_TRIGGERED: {
      jump DoorTriggered();
                       } break;
    case DT_TRIGGEREDAUTO: {
      jump DoorTriggeredAuto();
                       } break;
    case DT_LOCKED: {
      jump DoorLocked();
                    } break;
    }
  }
};

