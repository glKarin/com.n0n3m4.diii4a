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
#include "Entities/StdH/StdH.h"
%}

enum SprayParticlesType {
  0 SPT_NONE        "None",         // no particles
  1 SPT_BLOOD       "Blood",        // blood
  2 SPT_BONES       "Bones",        // bones
  3 SPT_FEATHER     "Feather",      // feather
  4 SPT_STONES      "Stones",       // stones
  5 SPT_WOOD        "Wood",         // wood
  6 SPT_SLIME       "Slime",        // gizmo/beast slime
  7 SPT_LAVA_STONES "Lava Stones",  // lava stones
  8 SPT_ELECTRICITY_SPARKS "Electricity sparks",  // electricity sparks
  9 SPT_BEAST_PROJECTILE_SPRAY "Beast projectile spray", // beast projectile explosion sparks
 10 SPT_SMALL_LAVA_STONES "Small Lava Stones",  // small lava stones
};

// input parameter for spawning a blood spray
event ESpawnSpray {
  enum SprayParticlesType sptType, // type of particles
  FLOAT fDamagePower,              // factor saying how powerfull damage has been
  FLOAT fSizeMultiplier,           // stretch factor
  FLOAT3D vDirection,              // dammage direction  
  CEntityPointer penOwner,         // who spawned the spray
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
  8 FLOATaabbox3D m_boxOwner = FLOATaabbox3D(FLOAT3D(0,0,0), 0.01f), // bounding box of blood spray's owner
  9 FLOAT3D m_vGDir = FLOAT3D(0,0,0),                                // gravity direction
  10 FLOAT m_fGA = 0.0f,                                             // gravity strength


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
      Particles_BloodSpray(m_sptType, this, m_vGDir, m_fGA, m_boxOwner, m_vDirection, m_tmStarted, m_fDamagePower);
      break;
    case SPT_ELECTRICITY_SPARKS:
      {
        Particles_MetalParts(this, m_tmStarted, m_boxOwner, m_fDamagePower);
        Particles_DamageSmoke(this, m_tmStarted, m_boxOwner, m_fDamagePower);
        Particles_BloodSpray(SPT_BLOOD, this, m_vGDir, m_fGA, m_boxOwner, m_vDirection, m_tmStarted, m_fDamagePower/2.0f);
        Particles_ElectricitySparks( this, m_tmStarted, 5.0f, 0.0f, 32);
        break;
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
    m_tmStarted = _pTimer->CurrentTick();
    // if owner doesn't exist (could be destroyed in initialization)
    if( eSpawn.penOwner->en_pmoModelObject == NULL)
    {
      // don't do anything
      Destroy();
      return;
    }
    eSpawn.penOwner->en_pmoModelObject->GetCurrentFrameBBox( m_boxOwner);
    m_boxOwner.StretchByVector(eSpawn.penOwner->en_pmoModelObject->mo_Stretch*eSpawn.fSizeMultiplier);
    if (m_penOwner->GetPhysicsFlags()&EPF_MOVABLE) {
      m_vGDir = ((CMovableEntity *) m_penOwner.ep_pen)->en_vGravityDir;
      m_fGA = ((CMovableEntity *) m_penOwner.ep_pen)->en_fGravityA;
    } else {
      FLOATmatrix3D &m = m_penOwner->en_mRotation;
      m_vGDir = FLOAT3D(-m(1,2), -m(2,2), -m(3,2));
      m_fGA = 30.0f;
    }

    FLOAT fWaitTime = 2.0f;
    if( m_sptType==SPT_ELECTRICITY_SPARKS)
    {
      fWaitTime = 4.0f;
    }
    autowait(fWaitTime);
    Destroy();

    return;
  }
};
