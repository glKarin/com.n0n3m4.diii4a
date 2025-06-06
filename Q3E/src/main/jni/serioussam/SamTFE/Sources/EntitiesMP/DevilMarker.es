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

338
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Marker";
uses "EntitiesMP/Devil";

enum DevilActionType {
  0 DAT_NONE                   "None",
  1 DAT_WALK                   "Walk",
  2 DAT_RISE                   "Rise",
  3 DAT_ROAR                   "Roar",
  4 DAT_PUNCH_LEFT             "Punch left - obsolete",
  5 DAT_PUNCH_RIGHT            "Punch right - obsolete",
  6 DAT_HIT_GROUND             "Hit ground",
  7 DAT_JUMP                   "Jump",
  8 DAT_WAIT                   "Wait",
  9 DAT_STOP_DESTROYING        "Stop destroying",
 10 DAT_NEXT_ACTION            "Next action",
 11 DAT_GRAB_LOWER_WEAPONS     "Grab lower weapons",
 12 DAT_STOP_MOVING            "Stop moving",
 13 DAT_JUMP_INTO_PYRAMID      "Jump into pyramid",
 14 DAT_SMASH_LEFT             "Smash left - obsolete",
 15 DAT_SMASH_RIGHT            "Smash right - obsolete",
 16 DAT_PUNCH                  "Punch",
 17 DAT_SMASH                  "Smash",
 18 DAT_FORCE_ATTACK_RADIUS    "Force attack radius",
 19 DAT_TELEPORT_INTO_PYRAMID  "Teleport into pyramid",
 20 DAT_DECREASE_ATTACK_RADIUS "Decrease attack radius",
};

class CDevilMarker: CMarker {
name      "Devil Marker";
thumbnail "Thumbnails\\EnemyMarker.tbn";

properties:
 1 enum DevilActionType m_datType    "Action"  'A' = DAT_NONE,
 4 INDEX m_iWaitIdles                "Wait idles"  'W' = 2,
 5 CEntityPointer m_penDevil         "Devil"  'D',
 6 CEntityPointer m_penTrigger       "Trigger" 'G',
 7 CEntityPointer m_penToDestroy1    "Destroy target 1"  'E',
 8 CEntityPointer m_penToDestroy2    "Destroy target 2"  'R',
 9 RANGE m_fAttackRadius             "Attack radius"  'S' = 100.0f,

components:
  1 model   MODEL_MARKER     "Models\\Editor\\EnemyMarker.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\DevilMarker.tex"

functions:
  void SetDefaultName(void)
  {
    m_strName = DevilActionType_enum.NameForValue(INDEX(m_datType));
  }

  const CTString &GetDescription(void) const {
    CTString strAction = DevilActionType_enum.NameForValue(INDEX(m_datType));
    if (m_penTarget==NULL) {
      ((CTString&)m_strDescription).PrintF("%s (%s)-><none>", (const char *) m_strName, (const char *) strAction);
    } else {
      ((CTString&)m_strDescription).PrintF("%s (%s)->%s", (const char *) m_strName, (const char *) strAction, 
        (const char *) m_penTarget->GetName());
    }
    return m_strDescription;
  }

  /* Check if entity is moved on a route set up by its targets. */
  BOOL MovesByTargetedRoute(CTString &strTargetProperty) const {
    strTargetProperty = "Target";
    return TRUE;
  };
  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker(CTFileName &fnmMarkerClass, CTString &strTargetProperty) const {
    fnmMarkerClass = CTFILENAME("Classes\\DevilMarker.ecl");
    strTargetProperty = "Target";
    return TRUE;
  }

  /* Handle an event, return false if the event is not handled. */
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    if (ee.ee_slEvent==EVENTCODE_ETrigger)
    {
      if(m_datType==DAT_NEXT_ACTION && m_penDevil!=NULL && m_penTarget!=NULL)
      {
        EDevilCommand eDevilCommand;
        eDevilCommand.dctType = DC_FORCE_ACTION;
        eDevilCommand.penForcedAction = m_penTarget;
        m_penDevil->SendEvent(eDevilCommand);
        return TRUE;
      }
      else if(m_datType==DAT_GRAB_LOWER_WEAPONS && m_penDevil!=NULL)
      {
        EDevilCommand eDevilCommand;
        eDevilCommand.dctType = DC_GRAB_LOWER_WEAPONS;
        m_penDevil->SendEvent(eDevilCommand);
        return TRUE;
      }
      else if(m_datType==DAT_STOP_MOVING && m_penDevil!=NULL)
      {
        EDevilCommand eDevilCommand;
        eDevilCommand.dctType = DC_STOP_MOVING;
        m_penDevil->SendEvent(eDevilCommand);
        return TRUE;
      }
      else if(m_datType==DAT_JUMP_INTO_PYRAMID && m_penDevil!=NULL)
      {
        EDevilCommand eDevilCommand;
        eDevilCommand.dctType = DC_JUMP_INTO_PYRAMID;
        eDevilCommand.penForcedAction = this;
        m_penDevil->SendEvent(eDevilCommand);
        return TRUE;
      }
      else if(m_datType==DAT_TELEPORT_INTO_PYRAMID && m_penDevil!=NULL)
      {
        EDevilCommand eDevilCommand;
        eDevilCommand.dctType = DC_TELEPORT_INTO_PYRAMID;
        eDevilCommand.penForcedAction = this;
        m_penDevil->SendEvent(eDevilCommand);
        return TRUE;
      }
      else if(m_datType==DAT_FORCE_ATTACK_RADIUS && m_penDevil!=NULL)
      {
        EDevilCommand eDevilCommand;
        eDevilCommand.dctType = DC_FORCE_ATTACK_RADIUS;
        eDevilCommand.fAttackRadius = m_fAttackRadius;
        eDevilCommand.vCenterOfAttack = GetPlacement().pl_PositionVector;
        m_penDevil->SendEvent(eDevilCommand);
        return TRUE;
      }
      else if(m_datType==DAT_DECREASE_ATTACK_RADIUS && m_penDevil!=NULL)
      {
        EDevilCommand eDevilCommand;
        eDevilCommand.dctType = DC_DECREASE_ATTACK_RADIUS;
        m_penDevil->SendEvent(eDevilCommand);
        return TRUE;
      }
    }
    return FALSE;
  }

procedures:
  Main() {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    SetDefaultName();
    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);
    return;
  }
};

