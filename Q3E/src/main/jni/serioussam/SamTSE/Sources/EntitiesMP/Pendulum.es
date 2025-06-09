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

106
%{
#include "EntitiesMP/StdH/StdH.h"
%}


class CPendulum: CMovableBrushEntity {
name      "Pendulum";
thumbnail "Thumbnails\\Pendulum.tbn";
features  "HasName", "IsTargetable";
properties:
  1 CTString m_strName            "Name" 'N' = "Pendulum",              // name
  2 BOOL m_bDynamicShadows        "Dynamic shadows" = FALSE,            // if has dynamic shadows
  3 FLOAT m_fMaxAngle             "Maximum angle" = 60.0f,              // pendulum will never go over this angle
  5 FLOAT m_fSpeed = 0.0f,                                              // current speed
  6 FLOAT m_fDampFactor           "Damp factor" = 0.9f,                 // dump factor
  7 FLOAT m_fPendulumFactor       "Pendulum factor" = 1.0f,             // pendulum factor
  8 FLOAT m_fImpulseFactor        "Damage impulse factor" = 0.01f,      // factor applied to damage ammount
  9 FLOAT m_fTriggerImpulse       "Impulse on trigger" = 10.0f,         // ipulse given on trigger
 10 BOOL m_bActive                "Active" 'A' = TRUE,                  // if pendulum is active by default

components:
functions:
  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    if( !m_bActive)
    {
      return;
    }
    // get vector in direction of oscilation
    FLOAT3D vOscilatingDirection;
    GetHeadingDirection( -90.0f, vOscilatingDirection);
    // project damage direction onto oscilating direction
    FLOAT fImpulse = vDirection%vOscilatingDirection;
    // calculate impulse strength
    fImpulse *= fDamageAmmount*m_fImpulseFactor;
    // apply impulse
    m_fSpeed += fImpulse;
    SetDesiredRotation( ANGLE3D(0, 0, m_fSpeed));
  }

  /* Post moving */
  void PostMoving()
  {
    CMovableBrushEntity::PostMoving();
    ANGLE fCurrentBanking = GetPlacement().pl_OrientationAngle(3);
    FLOAT fNewSpeed = m_fSpeed*m_fDampFactor-m_fPendulumFactor*fCurrentBanking;
    
    // if maximum angle achieved, stop in place and turn back
    if( Abs( fCurrentBanking) > m_fMaxAngle && Sgn(fNewSpeed)==Sgn(fCurrentBanking))
    {
      fNewSpeed = 0.0f;
    }

    m_fSpeed = fNewSpeed;
    SetDesiredRotation( ANGLE3D(0, 0, fNewSpeed));

    // if angle is not zero 
    if (Abs( fCurrentBanking) > 1.0f)
    {
      // clear in rendering flag
      SetFlags(GetFlags()&~ENF_INRENDERING);
    }
  };

procedures:
  Main() {
    // declare yourself as a brush
    InitAsBrush();
    SetPhysicsFlags(EPF_BRUSH_MOVING);
    SetCollisionFlags(ECF_BRUSH);
    // non-zoning brush
    SetFlags(GetFlags()&~ENF_ZONING);

    // set dynamic shadows as needed
    if (m_bDynamicShadows) {
      SetFlags(GetFlags()|ENF_DYNAMICSHADOWS);
    } else {
      SetFlags(GetFlags()&~ENF_DYNAMICSHADOWS);
    }
   
    // start moving
    wait() {
      on( EActivate):
      {
        m_bActive = TRUE;
        resume;
      }
      on( EDeactivate):
      {
        m_bActive = FALSE;
        resume;
      }
      on( ETrigger):
      {
        if( m_bActive)
        {
          // apply impulse
          m_fSpeed += m_fTriggerImpulse;
          AddToMovers();
        }
        resume;
      }
    }

    Destroy();
    stop;
    return;
  }
};
