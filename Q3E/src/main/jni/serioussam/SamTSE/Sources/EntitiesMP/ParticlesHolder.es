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

223
%{
#include "EntitiesMP/StdH/StdH.h"
%}

enum ParticlesHolderType {
  1 PHT_SPIRAL        "Spiral",
  2 PHT_EMANATE       "Emanate",
  3 PHT_STARDUST      "Stardust",
  4 PHT_ATOMIC        "Atomic",
  5 PHT_RISING        "Rising",
  6 PHT_FOUNTAIN      "Fountain",
  7 PHT_SMOKE         "Smoke",
  8 PHT_BLOOD         "Blood",
  9 PHT_EMANATEPLANE  "EmanatePlane",
  10 PHT_SANDFLOW     "SandFlow",
  11 PHT_WATERFLOW    "WaterFlow",
  12 PHT_LAVAFLOW     "Lava Flow",
  13 PHT_LAVAERUPTING "Lava Erupting",
  14 PHT_WATERFALLFOAM "Waterfall foam",
  15 PHT_CHIMNEYSMOKE "Chimney smoke",
  16 PHT_WATERFALL    "Waterfall",
  17 PHT_TWISTER      "Twister",
  18 PHT_ROCKETMOTOR  "Rocket motor",
  19 PHT_COLLECT_ENERGY "Collect energy",
};

class CParticlesHolder : CMovableModelEntity {
name      "ParticlesHolder";
thumbnail "Thumbnails\\ParticlesHolder.tbn";
features "HasName", "HasDescription";


properties:

  1 enum ParticlesHolderType m_phtType "Type"    'Y' = PHT_SPIRAL,
  2 enum ParticleTexture m_ptTexture  "Texture" 'T' = PT_STAR01,
  3 INDEX m_ctCount "Count" 'C' = 16,
  4 FLOAT m_fStretchAll       "StretchAll" 'S' = 1.0f,
  5 FLOAT m_fStretchX         "StretchX"   'X' = 1.0f,
  6 FLOAT m_fStretchY         "StretchY"   'Y' = 1.0f,
  7 FLOAT m_fStretchZ         "StretchZ"   'Z' = 1.0f,
  8 CTString m_strName        "Name" 'N' ="",
 12 CTString m_strDescription = "",
 13 BOOL m_bBackground        "Background" 'B' = FALSE,   // set if model is rendered in background
 21 BOOL m_bTargetable        "Targetable" = FALSE, // st if model should be targetable
 30 FLOAT m_fSize             "Size"  = 0.1f,
 31 FLOAT m_fParam1           "Param1" 'P' = 1.0f,
 32 FLOAT m_fParam2           "Param2" = 1.0f,
 33 FLOAT m_fParam3           "Param3" = 1.0f,
 34 BOOL m_bActive            "Active" 'A' = TRUE, // is particles are active
 35 FLOAT m_fActivateTime = 0.0f,
 36 FLOAT m_fDeactivateTime = -10000.0f,
 37 FLOAT m_fMipFactorDisappear "Disappear mip factor" = 8.0f,


components:

  1 model   MODEL_TELEPORT     "Models\\Editor\\Teleport.mdl",
  2 texture TEXTURE_TELEPORT   "Models\\Editor\\BoundingBox.tex",


functions:

  // render particles
  void RenderParticles(void)
  {
    if( !m_bActive)
    {
      return;
    }
    switch (m_phtType)
    {
      case PHT_SPIRAL     :
        Particles_Spiral(this, m_fStretchAll, m_fStretchAll/2, m_ptTexture, m_ctCount);
        break;
      case PHT_EMANATE    :
        Particles_Emanate(this, m_fStretchAll, m_fStretchAll/2, m_ptTexture, m_ctCount, m_fMipFactorDisappear);
        break;
      case PHT_STARDUST   :
        Particles_Stardust(this, m_fStretchAll, m_fStretchAll/2, m_ptTexture, m_ctCount);
        break;
      case PHT_ATOMIC     :
        Particles_Atomic(this, m_fStretchAll, m_fStretchAll/2, m_ptTexture, m_ctCount);
        break;
      case PHT_RISING     :
        Particles_Rising(this, m_fActivateTime, m_fDeactivateTime, m_fStretchAll, m_fStretchX, m_fStretchY, m_fStretchZ, m_fSize, m_ptTexture, m_ctCount);
        break;
      case PHT_FOUNTAIN   :
        Particles_Fountain(this, m_fStretchAll, m_fStretchAll/2, m_ptTexture, m_ctCount);
        break;
      case PHT_SMOKE      :
        Particles_GrenadeTrail(this);
        break;
      case PHT_BLOOD      :
        Particles_BloodTrail(this);
        break;
      case PHT_EMANATEPLANE:
        Particles_EmanatePlane(this, 
          m_fStretchX, m_fStretchY, m_fStretchZ, m_fSize, 
          m_fParam1, m_fParam2, m_ptTexture, m_ctCount, m_fMipFactorDisappear);
        break;
      case PHT_SANDFLOW   :
        Particles_SandFlow(this, m_fStretchAll, m_fSize, m_fParam1, m_fActivateTime, m_fDeactivateTime, m_ctCount);
        break;
      case PHT_WATERFLOW  :
        Particles_WaterFlow(this, m_fStretchAll, m_fSize, m_fParam1, m_fActivateTime, m_fDeactivateTime, m_ctCount);
        break;
      case PHT_LAVAFLOW   :
        Particles_LavaFlow(this, m_fStretchAll, m_fSize, m_fParam1, m_fActivateTime, m_fDeactivateTime, m_ctCount);
        break;
      case PHT_LAVAERUPTING:
        Particles_LavaErupting(this, m_fStretchAll, m_fSize, m_fStretchX, m_fStretchY, m_fStretchZ, m_fActivateTime);
        break;
      case PHT_WATERFALLFOAM:
        Particles_WaterfallFoam(this, 
          m_fStretchX, m_fStretchY, m_fStretchZ, m_fSize, 
          m_fParam1, m_fParam2, m_fParam3, m_ctCount);
        break;
      case PHT_CHIMNEYSMOKE:
        Particles_ChimneySmoke(this, m_ctCount, m_fStretchAll, m_fMipFactorDisappear);
        break;
      case PHT_ROCKETMOTOR:
        Particles_RocketMotorBurning(this, m_ctCount,
          FLOAT3D(m_fStretchX,m_fStretchY,m_fStretchZ), m_fSize, m_ctCount);
        break;
      case PHT_COLLECT_ENERGY:
        Particles_CollectEnergy(this, m_fActivateTime, m_fActivateTime+2.0f);
        break;
      case PHT_TWISTER:
        Particles_Twister(this, 1.0f, 0.0f, 1e6, 1.0f);
        break;
      case PHT_WATERFALL:
        Particles_Waterfall(this, m_ctCount, m_fStretchAll, m_fStretchX, m_fStretchY, m_fStretchZ,
          m_fSize, m_fMipFactorDisappear, m_fParam1);
        break;
    }
  }
  BOOL IsTargetable(void) const
  {
    return m_bTargetable;
  }

  // apply mirror and stretch to the entity
  void MirrorAndStretch(FLOAT fStretch, BOOL bMirrorX)
  {
    m_fStretchAll*=fStretch;
    if (bMirrorX) {
      m_fStretchX = -m_fStretchX;
    }
  }

  // Stretch model
  void StretchModel(void) {
    // stretch factors must not have extreme values
    if (Abs(m_fStretchX)  < 0.01f) { m_fStretchX   = 0.01f;  }
    if (Abs(m_fStretchY)  < 0.01f) { m_fStretchY   = 0.01f;  }
    if (Abs(m_fStretchZ)  < 0.01f) { m_fStretchZ   = 0.01f;  }
    if (m_fStretchAll< 0.01f) { m_fStretchAll = 0.01f;  }

    if (Abs(m_fStretchX)  >100.0f) { m_fStretchX   = 100.0f*Sgn(m_fStretchX); }
    if (Abs(m_fStretchY)  >100.0f) { m_fStretchY   = 100.0f*Sgn(m_fStretchY); }
    if (Abs(m_fStretchZ)  >100.0f) { m_fStretchZ   = 100.0f*Sgn(m_fStretchZ); }
    if (m_fStretchAll>100.0f) { m_fStretchAll = 100.0f; }

    GetModelObject()->StretchModel( FLOAT3D(
      m_fStretchAll*m_fStretchX,
      m_fStretchAll*m_fStretchY,
      m_fStretchAll*m_fStretchZ) );
    ModelChangeNotify();
  };

  CPlacement3D GetLerpedPlacement(void) const
  {
    return CEntity::GetLerpedPlacement(); // we never move anyway, so let's be able to be parented
  }


  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    // initial
    SLONG slUsedMemory = sizeof(CParticlesHolder) - sizeof(CMovableModelEntity) + CMovableModelEntity::GetUsedMemory();
    // add some more
    slUsedMemory += m_strName.Length();
    slUsedMemory += m_strDescription.Length();
    return slUsedMemory;
  }


procedures:


  // particles are active
  Active()
  {
    m_bActive = TRUE;
    while( TRUE)
    {
      // wait defined time
      wait( m_fParam2+FRnd()*m_fParam3)
      {
        on (ETimer) : 
        {
          // for randomly spawned particles
          if( m_phtType == PHT_LAVAERUPTING)
          {
            // spawn new particles
            m_fActivateTime = _pTimer->CurrentTick();
          }          
          stop;
        }
        on (EBegin) :
        {
          resume;
        }
        on (EDeactivate) :
        { 
          m_fDeactivateTime = _pTimer->CurrentTick();
          jump Inactive(); 
        }
      }
    }
  };

  // particles are not active
  Inactive() {
    m_bActive = FALSE;
    wait()   
    {
      on (EBegin) : { resume; }
      on (EActivate) :
      { 
        m_fActivateTime = _pTimer->CurrentTick();
        m_fDeactivateTime = _pTimer->CurrentTick()+10000.0f;
        jump Active(); 
      }
    }
  };

  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    StretchModel();
    SetModel(MODEL_TELEPORT);
    ModelChangeNotify();
    SetModelMainTexture(TEXTURE_TELEPORT);

    if (m_bBackground) {
      SetFlags(GetFlags()|ENF_BACKGROUND);
    } else {
      SetFlags(GetFlags()&~ENF_BACKGROUND);
    }

    en_fGravityA = 30.0f;
    GetPitchDirection(-90.0f, en_vGravityDir);

    m_fActivateTime = 0.0f;
    m_fDeactivateTime = -10000.0f;

    if (m_bActive) {
      jump Active();
    } else {
      jump Inactive();
    }


    return;
  }
};
