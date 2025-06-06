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

348
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Twister";

// input parameter for timer
event ESpinnerInit {
  CEntityPointer penParent,   // who owns it
  CEntityPointer penTwister,  // twister who spawned this
  FLOAT3D vRotationAngle,     // rotation angle
  FLOAT tmSpinTime,           // spin time
  FLOAT fUpSpeed,          // up multiply speed
  // for player impulse:
  BOOL  bImpulse,             // one time only spin
  FLOAT tmImpulseDuration,    // impulse duration
};

class export CSpinner : CRationalEntity {
  name      "Spinner";
  thumbnail "";
  
properties:
  1 CEntityPointer m_penParent,    // entity which owns it
  2 FLOAT3D m_aSpinRotation = FLOAT3D(0.0f, 0.0f, 0.0f),
  3 FLOAT3D m_vSpeed = FLOAT3D(0.0f, 0.0f, 0.0f),
  4 FLOAT   m_tmExpire = 0.0f,
  5 FLOAT3D m_vLastSpeed = FLOAT3D(0.0f, 0.0f, 0.0f),
  6 BOOL    m_bImpulse = FALSE,
  7 FLOAT   m_tmWaitAfterImpulse = 0.0f, // wait after impulse
     
 10 FLOAT   m_tmSpawn = 0.0f,
 11 FLOAT3D m_vSpinSpeed = FLOAT3D(0.0f, 0.0f, 0.0f),
    
components:
functions:
procedures:
  Main(ESpinnerInit esi) {
    
    // check some parameters
    if ((!(esi.penParent->GetPhysicsFlags()&EPF_MOVABLE)) ||
      (esi.penParent==NULL) || (esi.penParent==NULL))
    {
      Destroy();
      return;
    }
    ASSERT(esi.penParent!=NULL);
    ASSERT(esi.penTwister!=NULL);
    
    // remember the initial parameters
    CTwister &penTwister = (CTwister &)*esi.penTwister;
    CMovableEntity &penParent  = (CMovableEntity &)*esi.penParent;
    m_penParent = esi.penParent;
    m_aSpinRotation = esi.vRotationAngle;
    m_bImpulse = esi.bImpulse;
    if (m_bImpulse) {
      m_tmWaitAfterImpulse = esi.tmSpinTime - esi.tmImpulseDuration;
      if (m_tmWaitAfterImpulse<=0.0f) { m_tmWaitAfterImpulse = 0.01f; }
    }
    
    m_vSpinSpeed = ((CMovableEntity&)*m_penParent).en_vCurrentTranslationAbsolute;
    m_vSpinSpeed = FLOAT3D(0.0f, 0.0f, m_vSpinSpeed.Length());

    // init as nothing
    InitAsVoid();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);
    
    if (!m_bImpulse) {
      m_tmExpire = _pTimer->CurrentTick() + esi.tmSpinTime;
    } else {
      m_tmExpire = _pTimer->CurrentTick() + esi.tmImpulseDuration;
    }
    m_tmSpawn = _pTimer->CurrentTick();
    
    // throw target parameters
    m_vSpeed = FLOAT3D(penTwister.en_mRotation(1, 2), penTwister.en_mRotation(2, 2), penTwister.en_mRotation(3, 2)) * esi.fUpSpeed;
    
    // give absolute speed some randomness
    ANGLE3D aRnd;
    FLOATmatrix3D m;
    aRnd(1) = FRnd()*360.0f; aRnd(2) = FRnd()*30.0f; aRnd(3) = 0.0f;
    MakeRotationMatrixFast(m, aRnd);
    m_vSpeed = m_vSpeed*m;

    //each tick until the m_tmExpire, reinitialise spin
    while (_pTimer->CurrentTick()<m_tmExpire)
    {
      // if the parent is deleted, stop existing
      if (m_penParent->GetFlags()&ENF_DELETED) {
        Destroy();
        return;      
      }
      
      if (((CMovableEntity&)*m_penParent).en_vCurrentTranslationAbsolute!=m_vLastSpeed ||
          ((CMovableEntity&)*m_penParent).en_vCurrentTranslationAbsolute==FLOAT3D(0.0f, 0.0f, 0.0f)) {
        // give absolute speed
        ((CMovableEntity&)*m_penParent).en_vCurrentTranslationAbsolute += m_vSpeed;
        m_vLastSpeed = ((CMovableEntity&)*m_penParent).en_vCurrentTranslationAbsolute;
      } else {
        // give it some speed
        ((CMovableEntity&)*m_penParent).SetDesiredTranslation(m_vSpinSpeed);
      }
      
      // spin entity if not impulse
      if (!m_bImpulse) {
        ((CMovableEntity&)*m_penParent).en_aDesiredRotationRelative = m_aSpinRotation;
      }
      autowait (_pTimer->TickQuantum);
    }
    // stop spinning the parent entity
    ((CMovableEntity&)*m_penParent).en_aDesiredRotationRelative = ANGLE3D(0.0f, 0.0f, 0.0f);
  
    // wait if necessary and stop the up translation if player
    if (m_bImpulse) {
      ((CMovableEntity&)*m_penParent).SetDesiredTranslation(FLOAT3D(0.0f, 0.0f, 0.0f));
      autowait(m_tmWaitAfterImpulse);
    }
    
    // cease to exist
    Destroy();
    
    return;
  };
};