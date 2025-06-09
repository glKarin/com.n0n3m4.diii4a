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

703
%{
#include "EntitiesMP/StdH/StdH.h"
%}

// input parameter for timer
event EReminderInit {
  CEntityPointer penOwner,    // who owns it
  FLOAT fWaitTime,            // wait time
  INDEX iValue,               // reminder event value
};

class export CReminder : CRationalEntity {
name      "Reminder";
thumbnail "";

properties:
  1 CEntityPointer m_penOwner,    // entity which owns it
  2 FLOAT m_fWaitTime = 0.0f,     // wait time
  3 INDEX m_iValue = 0,           // reminder event value

components:
functions:
procedures:
  Main(EReminderInit eri) {
    // remember the initial parameters
    ASSERT(eri.penOwner!=NULL);
    m_penOwner = eri.penOwner;
    m_fWaitTime = eri.fWaitTime;
    m_iValue = eri.iValue;
    
    // init as nothing
    InitAsVoid();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // wait
    if (m_fWaitTime > 0.0f) {
      autowait(m_fWaitTime);
      EReminder er;
      er.iValue = m_iValue;
      if (m_penOwner!=NULL) {
        m_penOwner->SendEvent(er);
      }
    }

    // cease to exist
    Destroy();

    return;
  };
};