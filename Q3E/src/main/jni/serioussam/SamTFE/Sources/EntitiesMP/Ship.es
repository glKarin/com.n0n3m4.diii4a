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

103
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/ShipMarker";

event EHarbor {
};

%{

// adjust angular velocity
ANGLE AdjustRotationSpeed(ANGLE aDiference, ANGLE aMaxSpeed)
{
  aDiference = NormalizeAngle(aDiference);
  aDiference = Clamp(aDiference, -aMaxSpeed, +aMaxSpeed);
  return aDiference;
}

%}

class CShip : CMovableBrushEntity {
name      "Ship";
thumbnail "Thumbnails\\Ship.tbn";
features  "HasName", "IsTargetable";

properties:
  1 CTString m_strName            "Name" 'N' = "Ship",
  2 CTString m_strDescription = "",
  
  3 CEntityPointer m_penTarget    "Target" 'T',
  4 FLOAT m_fSpeed                "Speed [m/s]" 'S' = 10.0f,
  5 FLOAT m_fRotation             "Rotation [deg/s]" 'R' = 30.0f,
  6 FLOAT m_fRockingV             "Rocking V" 'V' = 10.0f,
  7 FLOAT m_fRockingA             "Rocking A" 'A' = 10.0f,
  8 FLOAT m_fAcceleration         "Acceleration" 'C' = 10.0f,

  10 BOOL m_bMoving = TRUE,
  11 FLOAT m_fRockSign = 1.0f,
  12 FLOAT m_fLastTargetDistance = UpperLimit(0.0f),

  20 CEntityPointer m_penSail      "Sail" 'L',
  21 ANIMATION m_iSailUpAnim       "Sail roll-up anim"=0,
  22 ANIMATION m_iSailDownAnim     "Sail roll-down anim"=0,
  23 ANIMATION m_iSailSailAnim     "Sail sailing anim"=0,
  24 ANIMATION m_iSailWaveingAnim  "Sail wawing anim"=0,

  30 FLOAT m_fOriginalRockingV = 0.0f,
  31 FLOAT m_fOriginalRockingA = 0.0f,
  32 FLOAT m_fNextRockingV = 0.0f,
  33 FLOAT m_fNextRockingA = 0.0f,
  34 FLOAT m_tmRockingChange=1,         // how many second to change rocking parameters
  35 FLOAT m_tmRockingChangeStart=-1,   // when changing of rocking parameters has started
components:
functions:
  /* Check if entity is moved on a route set up by its targets. */
  BOOL MovesByTargetedRoute(CTString &strTargetProperty) const {
    strTargetProperty = "Target";
    return TRUE;
  };
  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker(CTFileName &fnmMarkerClass, CTString &strTargetProperty) const {
    fnmMarkerClass = CTFILENAME("Classes\\ShipMarker.ecl");
    strTargetProperty = "Target";
    return TRUE;
  }
  const CTString &GetDescription(void) const {
    ((CTString&)m_strDescription).PrintF("-><none>");
    if (m_penTarget!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s", (const char *) m_penTarget->GetName());
    }
    return m_strDescription;
  }
  /* Get anim data for given animation property - return NULL for none. */
  CAnimData *GetAnimData(SLONG slPropertyOffset) 
  {
    if((slPropertyOffset==_offsetof(CShip, m_iSailUpAnim)
      ||slPropertyOffset==_offsetof(CShip, m_iSailDownAnim)
      ||slPropertyOffset==_offsetof(CShip, m_iSailSailAnim)
      ||slPropertyOffset==_offsetof(CShip, m_iSailWaveingAnim))
      &&m_penSail!=NULL) {
      return m_penSail->GetModelObject()->GetData();
    } else {
      return CEntity::GetAnimData(slPropertyOffset);
    }
  };

  // calculate velocities towards marker
  void SetMovingSpeeds(void)
  {
    // if the brush should not be moving, or there is no target
    if (!m_bMoving || m_penTarget==NULL) {
      // just rock
      SetDesiredRotation(ANGLE3D(0,0,GetRockingSpeed()));
      return;
    }

    CShipMarker *penTarget = (CShipMarker *)(CEntity*)m_penTarget;
    const CPlacement3D &plThis = GetPlacement();

    // get direction to target
    const FLOAT3D &vTarget = penTarget->GetPlacement().pl_PositionVector;
    const FLOAT3D &vNow = plThis.pl_PositionVector;
    FLOAT3D vDirection = vTarget-vNow;
    FLOAT fTargetDistance = vDirection.Length();
    // if got close enough
    if (fTargetDistance<m_fSpeed*5*_pTimer->TickQuantum) {
      // switch to next marker
      NextMarker();
      return;
    }

    vDirection/=fTargetDistance;
    ANGLE3D aAngle;
    DirectionVectorToAngles(vDirection, aAngle);
    aAngle-=plThis.pl_OrientationAngle;
    aAngle(1) = AdjustRotationSpeed(aAngle(1), m_fRotation);
    aAngle(2) = 0;
    aAngle(3) = GetRockingSpeed();

    SetDesiredRotation(aAngle);

    // set speed
    SetDesiredTranslation(FLOAT3D(0,0,-m_fSpeed));

    en_fAcceleration = m_fAcceleration;
    en_fDeceleration = m_fAcceleration;
  }

  // calculate rocking velocity
  ANGLE GetRockingSpeed(void)
  {
    // if rocking changing time has not passed
    TIME tmSinceChangeStarted = _pTimer->CurrentTick()-m_tmRockingChangeStart;
    if (tmSinceChangeStarted<m_tmRockingChange) {
      // calculate current rocking parameters
      FLOAT fFactor = tmSinceChangeStarted/m_tmRockingChange;
      m_fRockingV = Lerp(m_fOriginalRockingV, m_fNextRockingV, fFactor);
      m_fRockingA = Lerp(m_fOriginalRockingA, m_fNextRockingA, fFactor);
    }

    if (m_fRockingV==0) {
      return 0;
    }

    ANGLE aAngle = GetPlacement().pl_OrientationAngle(3);
    ANGLE aRotation = Sqrt(m_fRockingA*m_fRockingA-aAngle*aAngle)*m_fRockingV;
    if (aRotation<2 && aAngle*m_fRockSign>0) {
      m_fRockSign = -m_fRockSign;
    };

    if (aRotation<2) {
      aRotation = 2;
    }
    aRotation *=m_fRockSign;

    return aRotation;
  }

  // switch to next marker
  void NextMarker(void)
  {
    // get next marker
    CShipMarker *penTarget = (CShipMarker *)(CEntity*)m_penTarget;
    CShipMarker *penNextTarget = (CShipMarker *)(CEntity*)penTarget->m_penTarget;

    // if this marker is harbor
    if (penTarget->m_bHarbor) {
      // stop
      StopSailing();
      // start being in harbor
      SendEvent(EHarbor());
    }

    // if got to end
    if (penNextTarget==NULL) {
      // stop
      StopSailing();
      return;
    }

    // get properties from marker
    FLOAT fSpeed = penTarget->m_fSpeed;
    if (fSpeed>=0) {
      m_fSpeed = fSpeed;
    }
    FLOAT fRotation = penTarget->m_fRotation;
    if (fRotation>=0) {
      m_fRotation = fRotation;
    }
    FLOAT fAcceleration = penTarget->m_fAcceleration;
    if (fAcceleration>=0) {
      m_fAcceleration = fAcceleration;
    }

    m_fOriginalRockingV = m_fRockingV;
    m_fOriginalRockingA = m_fRockingA;

    FLOAT fRockingV = penTarget->m_fRockingV;
    if (fRockingV>=0) {
      m_fNextRockingV = fRockingV;
    } else {
      m_fNextRockingV = m_fRockingV;
    }
    FLOAT fRockingA = penTarget->m_fRockingA;
    if (fRockingA>=0) {
      m_fNextRockingA = fRockingA;
    } else {
      m_fNextRockingA = m_fRockingA;
    }
    m_tmRockingChange = penTarget->m_tmRockingChange;
    m_tmRockingChangeStart = _pTimer->CurrentTick();

    // remember next marker as current target
    m_penTarget = penNextTarget;
    SetMovingSpeeds();
  }

  void StartSailing()
  {
    m_bMoving = TRUE;
    // calculate velocities towards marker
    SetMovingSpeeds();
  }

  void StopSailing(void)
  {
    m_bMoving = FALSE;
    SetDesiredRotation(ANGLE3D(0,0,GetDesiredRotation()(3)));
    SetDesiredTranslation(FLOAT3D(0,0,0));
  }

  // do moving
  void PreMoving(void) {
    // calculate velocities towards marker
    SetMovingSpeeds();
    CMovableBrushEntity::PreMoving();
  };

procedures:
  Sail() {
    // roll the sail down
    m_penSail->GetModelObject()->PlayAnim(m_iSailDownAnim, 0);
    autowait(m_penSail->GetModelObject()->GetAnimLength(m_iSailDownAnim));
    // start sail waveing
    m_penSail->GetModelObject()->PlayAnim(m_iSailWaveingAnim, AOF_LOOPING);

    // wait until touched by a player or started
    wait() {
      on (EBegin) : { resume; }
      on (ETouch eTouch) : { 
        if (IsDerivedFromClass(eTouch.penOther, "PlayerEntity")) {
          stop; 
        }
      }
      on (EStart) : { 
        stop; 
      }
    }

    // blow the sail up
    m_penSail->GetModelObject()->PlayAnim(m_iSailSailAnim, 0);

    // start moving
    StartSailing();
    // sail until we come to harbor
    wait() {
      on (EBegin) : { resume; }
      on (EHarbor) : { stop; }
    }

    // stay in harbor
    jump Harbor();
  }
  Harbor() {
    // roll the sail up
    m_penSail->GetModelObject()->PlayAnim(m_iSailUpAnim, 0);

    // stay in harbor until we are triggered
    wait() {
      on (EBegin) : { resume; }
      on (ETrigger) : { stop; }
    }
    // start sailing
    jump Sail();
  }


  Main() {
    // declare yourself as a brush
    InitAsBrush();
    SetPhysicsFlags(EPF_BRUSH_MOVING&~(EPF_ABSOLUTETRANSLATE|EPF_NOACCELERATION));
    SetCollisionFlags(ECF_BRUSH);

    // stop moving brush
    ForceFullStop();

    // assure valid target
    if (m_penTarget!=NULL && !IsOfClass(m_penTarget, "Ship Marker")) {
      WarningMessage("Target '%s' is not of ShipMarker class!", (const char *) m_penTarget->GetName());
      m_penTarget = NULL;
    }
    // assure valid sail
    if (m_penSail!=NULL && m_penSail->GetRenderType()!=RT_MODEL) {
      WarningMessage("Sail '%s' is not a model!", (const char *) m_penSail->GetName());
      m_penSail = NULL;
    }

    // wait until game starts
    autowait(0.1f);

    // if sail is not valid
    if (m_penSail==NULL) {
      // don't continue
      WarningMessage("Ship will not work without a valid sail!");
      return;
    }

    // forever
    wait() {
      // on the beginning
      on (EBegin) : {
        // start sailing
        call Sail();
      }

      // move is obstructed
      on (EBlock eBlock) : {
        // inflict damage to entity that block brush
        InflictDirectDamage(eBlock.penOther, this, DMT_BRUSH, 10.0f,
          FLOAT3D(0.0f,0.0f,0.0f), (FLOAT3D &)eBlock.plCollision);
        resume;
      }
    }
  }
};
