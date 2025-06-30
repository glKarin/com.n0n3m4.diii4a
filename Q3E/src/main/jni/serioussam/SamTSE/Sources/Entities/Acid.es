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

509
%{
#include "Entities/StdH/StdH.h"
%}

// input parameter for acid
event EAcid {
  CEntityPointer penOwner,        // entity which owns it
  CEntityPointer penTarget,       // target entity which receive damage
};


class CAcid : CMovableModelEntity {
name      "Acid";
thumbnail "";

properties:
  1 CEntityPointer m_penOwner,    // entity which owns it
  2 CEntityPointer m_penTarget,   // target entity which receive damage
  5 BOOL m_bLoop = FALSE,         // internal for loops

components:
functions:
/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
  // --->>> MAIN
  Main(EAcid ea) {
    // attach to parent (another entity)
    ASSERT(ea.penOwner!=NULL);
    ASSERT(ea.penTarget!=NULL);
    m_penOwner = ea.penOwner;
    m_penTarget = ea.penTarget;

    // initialization
    InitAsVoid();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // acid damage
    SpawnReminder(this, 10.0f, 0);
    m_bLoop = TRUE;
    while(m_bLoop) {
      wait(0.25f) {
        // damage to parent
        on (EBegin) : {
          // inflict damage to parent
          if (m_penTarget!=NULL && !(m_penTarget->GetFlags()&ENF_DELETED)) {
            m_penTarget->InflictDirectDamage(m_penTarget, m_penOwner, DMT_ACID, 0.25f, FLOAT3D(0, 0, 0), FLOAT3D(0, 0, 0));
          // stop existing
          } else {
            m_bLoop = FALSE;
            stop;
          }
          resume;
        }
        on (ETimer) : { stop; }
        on (EReminder) : {
          m_bLoop = FALSE;
          stop;
        }
      }
    }

    // cease to exist
    Destroy();
    return;
  }
};
