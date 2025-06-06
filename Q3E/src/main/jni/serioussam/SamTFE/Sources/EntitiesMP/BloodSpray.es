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

603
%{
#include "EntitiesMP/StdH/StdH.h"
%}

// input parameter for spawning a blood spray
event ESpawnSpray {
  enum SprayParticlesType sptType, // type of particles
  FLOAT fDamagePower,              // factor saying how powerfull damage has been
  FLOAT fSizeMultiplier,           // stretch factor
  FLOAT3D vDirection,              // dammage direction  
  CEntityPointer penOwner,         // who spawned the spray
  COLOR colCentralColor,           // central color of particles that is randomized a little
  FLOAT fLaunchPower,
  COLOR colBurnColor,              // burn color
};

class CBloodSpray: CRationalEntity {
name      "Blood Spray";
thumbnail "";
features  "CanBePredictable";

properties:

  1 enum SprayParticlesType m_sptType = SPT_NONE,                    // type of particles
  2 FLOAT m_tmStarted = 0.0f,                                        // time when spawned
  3 FLOAT3D m_vDirection = FLOAT3D(0,0,0),                           // dammage direction
  5 CEntityPointer m_penOwner,                                       // who spawned the spray
  6 FLOAT m_fDamagePower = 1.0f,                                     // power of inflicted damage
  8 FLOATaabbox3D m_boxSizedOwner = FLOATaabbox3D(FLOAT3D(0,0,0), 0.01f), // bounding box of blood spray's owner
  9 FLOAT3D m_vGDir = FLOAT3D(0,0,0),                                // gravity direction
  10 FLOAT m_fGA = 0.0f,                                             // gravity strength
  11 FLOAT m_fLaunchPower = 1.0f,
  12 COLOR m_colCentralColor = COLOR(C_WHITE|CT_OPAQUE),
  13 FLOATaabbox3D m_boxOriginalOwner = FLOATaabbox3D(FLOAT3D(0,0,0), 0.01f),
  14 COLOR m_colBurnColor = COLOR(C_WHITE|CT_OPAQUE),


components:
  1 model   MODEL_MARKER     "Models\\Editor\\Axis.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\Vector.tex"

functions:

  // particles
  void RenderParticles(void)
  {
    switch( m_sptType)
    {
    case SPT_BLOOD:
    case SPT_BONES:
    case SPT_FEATHER:
    case SPT_STONES:
    case SPT_WOOD:
    case SPT_SLIME:
    case SPT_LAVA_STONES:
    case SPT_SMALL_LAVA_STONES:
    case SPT_BEAST_PROJECTILE_SPRAY:
    case SPT_AIRSPOUTS:
    case SPT_GOO:
    {
      Particles_BloodSpray(m_sptType, GetLerpedPlacement().pl_PositionVector, m_vGDir, m_fGA,
        m_boxSizedOwner, m_vDirection, m_tmStarted, m_fDamagePower, m_colBurnColor);
      break;
    }
    case SPT_COLOREDSTONE:
      {
        Particles_BloodSpray(m_sptType, GetLerpedPlacement().pl_PositionVector, m_vGDir, m_fGA,
          m_boxSizedOwner, m_vDirection, m_tmStarted, m_fDamagePower, MulColors(m_colCentralColor, m_colBurnColor) );
        break;
      }
    case SPT_TREE01:
      Particles_BloodSpray(SPT_WOOD, GetLerpedPlacement().pl_PositionVector, m_vGDir, m_fGA/1.5f,
        m_boxSizedOwner, m_vDirection, m_tmStarted, m_fDamagePower/2.0f, m_colBurnColor);
      Particles_Leaves(m_penOwner, m_boxOriginalOwner, GetLerpedPlacement().pl_PositionVector,
        m_fDamagePower, m_fDamagePower*m_fLaunchPower, m_vGDir,
        m_fGA/2.0f, m_tmStarted, MulColors(m_colCentralColor,m_colBurnColor));
      break;
    case SPT_PLASMA:
        Particles_BloodSpray(m_sptType, GetLerpedPlacement().pl_PositionVector, m_vGDir, m_fGA,
          m_boxSizedOwner, m_vDirection, m_tmStarted, m_fDamagePower);
        Particles_DamageSmoke(this, m_tmStarted, m_boxSizedOwner, m_fDamagePower);
        Particles_ElectricitySparks( this, m_tmStarted, 5.0f, 0.0f, 32);
      break;
    case SPT_ELECTRICITY_SPARKS:
      {
        Particles_MetalParts(this, m_tmStarted, m_boxSizedOwner, m_fDamagePower);
        Particles_DamageSmoke(this, m_tmStarted, m_boxSizedOwner, m_fDamagePower);
        Particles_BloodSpray(SPT_BLOOD, GetLerpedPlacement().pl_PositionVector, m_vGDir, m_fGA,
          m_boxSizedOwner, m_vDirection, m_tmStarted, m_fDamagePower/2.0f, C_WHITE|CT_OPAQUE);
        Particles_ElectricitySparks( this, m_tmStarted, 5.0f, 0.0f, 32);
        break;
      }
    case SPT_ELECTRICITY_SPARKS_NO_BLOOD:
      {
        Particles_MetalParts(this, m_tmStarted, m_boxSizedOwner, m_fDamagePower);
        Particles_DamageSmoke(this, m_tmStarted, m_boxSizedOwner, m_fDamagePower);
        Particles_ElectricitySparks( this, m_tmStarted, 5.0f, 0.0f, 32);        
      }
    }
  };

/************************************************************
 *                          MAIN                            *
 ************************************************************/

procedures:

  Main( ESpawnSpray eSpawn)
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);
    SetPredictable(TRUE);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    // setup variables
    m_sptType = eSpawn.sptType;
    m_vDirection = eSpawn.vDirection;
    m_penOwner = eSpawn.penOwner;
    m_fDamagePower = eSpawn.fDamagePower;
    m_fLaunchPower = eSpawn.fLaunchPower;
    m_colBurnColor = eSpawn.colBurnColor;
    m_tmStarted = _pTimer->CurrentTick();
    m_colCentralColor = eSpawn.colCentralColor;

    // if owner doesn't exist (could be destroyed in initialization)
    if( eSpawn.penOwner==NULL || eSpawn.penOwner->en_pmoModelObject == NULL)
    {
      // don't do anything
      Destroy();
      return;
    }

    if(eSpawn.penOwner->en_RenderType == RT_SKAMODEL) {
      eSpawn.penOwner->GetModelInstance()->GetCurrentColisionBox( m_boxSizedOwner);
    } else {
      eSpawn.penOwner->en_pmoModelObject->GetCurrentFrameBBox( m_boxSizedOwner);
      m_boxOriginalOwner=m_boxSizedOwner;
      m_boxSizedOwner.StretchByVector(eSpawn.penOwner->en_pmoModelObject->mo_Stretch*eSpawn.fSizeMultiplier);
      m_boxOriginalOwner.StretchByVector(eSpawn.penOwner->en_pmoModelObject->mo_Stretch);
    }

      if (m_penOwner->GetPhysicsFlags()&EPF_MOVABLE) {
      m_vGDir = ((CMovableEntity *) m_penOwner.ep_pen)->en_vGravityDir;
      m_fGA = ((CMovableEntity *) m_penOwner.ep_pen)->en_fGravityA;
    } else {
      FLOATmatrix3D &m = m_penOwner->en_mRotation;
      m_vGDir = FLOAT3D(-m(1,2), -m(2,2), -m(3,2));
      m_fGA = 30.0f;
    }

    FLOAT fWaitTime = 4.0f;
    if( m_sptType==SPT_ELECTRICITY_SPARKS || m_sptType==SPT_ELECTRICITY_SPARKS_NO_BLOOD)
    {
      fWaitTime = 4.0f;
    }
    autowait(fWaitTime);
    Destroy();

    return;
  }
};
