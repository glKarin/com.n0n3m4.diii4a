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

212
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Marker";
uses "EntitiesMP/GravityRouter";

enum GravityType {
  0 LT_PARALLEL     "Parallel",
  1 LT_CENTRAL      "Central",
  2 LT_CYLINDRICAL  "Cylindirical",
  3 LT_TORUS        "Torus",
};

class CGravityMarker: CMarker {
name      "Gravity Marker";
thumbnail "Thumbnails\\GravityMarker.tbn";
features "IsImportant";

properties:
  1 enum GravityType m_gtType  "Type" 'Y' =LT_PARALLEL,
  2 FLOAT m_fStrength     "Strength"  'S' = 1,
  3 RANGE m_rFallOff      "FallOff"   'F' = 50,
  4 RANGE m_rHotSpot      "HotSpot"   'H' = 50,
  5 RANGE m_rTorusR       "Torus Radius"  'R' = 100,

  10 FLOAT m_fAcc = 0,
  11 FLOAT m_fSign = 1,
  12 FLOAT m_fStep = 0,

  20 ANGLE3D m_aForceDir    "Forcefield Direction"  'F' = ANGLE3D(0,0,0),
  21 FLOAT m_fForceA        "Forcefield Acceleration" = 0.0f,
  22 FLOAT m_fForceV        "Forcefield Velocity" = 0.0f,
  23 FLOAT3D m_vForceDir = FLOAT3D(1,0,0),

components:
  1 model   MODEL_MARKER     "Models\\Editor\\GravityMarker.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\GravityMarker.tex"

functions:
  // find strength at given distance
  inline FLOAT StrengthAtDistance(FLOAT fDistance)
  {
    FLOAT fStrength = (m_rFallOff-fDistance)*m_fStep;
    return Clamp(fStrength, 0.0f, m_fAcc);
  }

  /* Get force type name, return empty string if not used. */
  const CTString &GetForceName(INDEX i)
  {
    return m_strName;
  }
  /* Get force in given point. */
  void GetForce(INDEX i, const FLOAT3D &vPoint, 
    CForceStrength &fsGravity, CForceStrength &fsField)
  {
    const FLOATmatrix3D &m = GetRotationMatrix();
    switch (m_gtType) {
    case LT_PARALLEL: {
      fsGravity.fs_vDirection(1) = -m(1,2) * m_fSign;
      fsGravity.fs_vDirection(2) = -m(2,2) * m_fSign;
      fsGravity.fs_vDirection(3) = -m(3,2) * m_fSign;
      FLOAT fDistance = (vPoint-GetPlacement().pl_PositionVector)%fsGravity.fs_vDirection;
      fsGravity.fs_fAcceleration = StrengthAtDistance(fDistance);
      fsGravity.fs_fVelocity = 70;
                      } break;
    case LT_CENTRAL: {
      fsGravity.fs_vDirection = (GetPlacement().pl_PositionVector-vPoint)*m_fSign;
      FLOAT fDistance = fsGravity.fs_vDirection.Length();
      if (fDistance>0.01f) {
        fsGravity.fs_vDirection/=fDistance;
      }
      fsGravity.fs_fAcceleration = StrengthAtDistance(fDistance);
      fsGravity.fs_fVelocity = 70;
                     } break;
    case LT_CYLINDRICAL: {
      FLOAT3D vDelta = GetPlacement().pl_PositionVector-vPoint;
      FLOAT3D vAxis;
      vAxis(1) = m(1,2);
      vAxis(2) = m(2,2);
      vAxis(3) = m(3,2);
      GetNormalComponent(vDelta, vAxis, fsGravity.fs_vDirection);
      fsGravity.fs_vDirection*=m_fSign;
      FLOAT fDistance = fsGravity.fs_vDirection.Length();
      if (fDistance>0.01f) {
        fsGravity.fs_vDirection/=fDistance;
      }
      fsGravity.fs_fAcceleration = StrengthAtDistance(fDistance);
      fsGravity.fs_fVelocity = 70;
                         } break;
    case LT_TORUS: {
      // get referent point 
      FLOAT3D vDelta = vPoint-GetPlacement().pl_PositionVector;
      FLOAT3D vAxis;
      vAxis(1) = m(1,2);
      vAxis(2) = m(2,2);
      vAxis(3) = m(3,2);
      FLOAT3D vR;
      GetNormalComponent(vDelta, vAxis, vR);
      vR.Normalize();
      fsGravity.fs_vDirection = (vDelta-vR*m_rTorusR)*m_fSign;
      FLOAT fDistance = fsGravity.fs_vDirection.Length();
      if (fDistance>0.01f) {
        fsGravity.fs_vDirection/=fDistance;
      }
      fsGravity.fs_fAcceleration = StrengthAtDistance(fDistance);
      fsGravity.fs_fVelocity = 70;
      
                   } break;
    default:
      fsGravity.fs_fAcceleration = m_fAcc;
      fsGravity.fs_fVelocity = 70;
      fsGravity.fs_vDirection = FLOAT3D(0,-1,0);
    }

    // calculate forcefield influence
    fsField.fs_fAcceleration = m_fForceA;
    fsField.fs_fVelocity = m_fForceV;
    fsField.fs_vDirection = m_vForceDir;
  }

  /* Handle an event, return false if the event is not handled. */
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    if( ee.ee_slEvent==EVENTCODE_ETrigger)
    {
      EChangeGravity eChangeGravity;
      eChangeGravity.penNewGravity = this;
      m_penTarget->SendEvent( eChangeGravity);
      return TRUE;
    }
    return FALSE;
  }

procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    // set name
    if (m_strName=="Marker") {
      m_strName = "Gravity Marker";
    }
    
    // precalc fast gravity parameters
    m_fAcc = Abs(30*m_fStrength),
    m_fSign = SgnNZ(m_fStrength),
    m_fStep = m_fAcc/(m_rFallOff-m_rHotSpot);

    AnglesToDirectionVector(m_aForceDir, m_vForceDir);

    return;
  }
};

