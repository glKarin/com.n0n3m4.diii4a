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

237
%{
#include "EntitiesMP/StdH/StdH.h"
%}

enum EnvironmentParticlesHolderType {
  0 EPTH_NONE    "None",
  1 EPTH_GROWTH  "Growth",
  2 EPTH_RAIN    "Rain",
  3 EPTH_SNOW    "Snow",
};

class CEnvironmentParticlesHolder: CRationalEntity {
name      "EnvironmentParticlesHolder";
thumbnail "Thumbnails\\EnvironmentParticlesHolder.tbn";
features  "IsTargetable", "HasName", "HasDescription", "IsImportant";

properties:

  1 CTString m_strName "Name" 'N' = "Env. particles holder", // class name
  6 CTString m_strDescription = "",
  2 CTFileName m_fnHeightMap "Height map" 'R' = CTString(""),
  3 FLOATaabbox3D m_boxHeightMap "Height map box" 'B' = FLOATaabbox3D(FLOAT3D(0,0,0), FLOAT3D(1,1,1)),
  4 enum EnvironmentParticlesHolderType m_eptType "Type" 'Y' = EPTH_NONE,
  5 CEntityPointer m_penNextHolder "Next env. particles holder" 'T',

 10 FLOAT m_tmRainStart = -1.0f,                 // Rain start time
 11 FLOAT m_tmRainEnd = -1.0f,                   // Rain end time

 12 FLOAT m_tmSnowStart = -1.0f,                 // Snow start time
 13 FLOAT m_tmSnowEnd = -1.0f,                   // Snow end time

 20 CModelObject m_moHeightMapHolder,
 22 CModelObject m_moParticleTextureHolder,
  
 // particle type specific properties
 
 // shared particles texture
 40 CTFileName m_fnTexture "Particle Texture" = CTString(""),
 
 // growth
 50 FLOAT m_fGrowthRenderingStep        "Growth frequency" = 1.0f,
 51 FLOAT m_fGrowthRenderingRadius      "Growth radius" = 50,
 52 FLOAT m_fGrowthRenderingRadiusFade  "Growth fade radius" = 50,
 53 BOOL  m_bGrowthHighresMap           "Growth high res map" = TRUE,
 54 INDEX m_iGrowthMapX                 "Growth map tiles X" = 1,
 55 INDEX m_iGrowthMapY                 "Growth map tiles Y" = 1,
 56 FLOAT m_fGrowthMinSize              "Growth min. size" = 1.0f,
 57 FLOAT m_fGrowthMaxSize              "Growth max. size" = 1.0f,
 58 FLOAT m_fParticlesSinkFactor        "Growth sink factor" = 0.0,

 // rain
 70 FLOAT m_fRainAppearLen              "Rain start duration" = 10.0f,
 71 FLOAT m_fSnowAppearLen              "Snow start duration" = 10.0f,

  {
    // linked list of growth caches 
    CListHead lhCache;
  }

components:
  1 model   MODEL_ENVIRONMENT_PARTICLES_HOLDER    "ModelsMP\\Editor\\EnvironmentParticlesHolder.mdl",
  2 texture TEXTURE_ENVIRONMENT_PARTICLES_HOLDER  "ModelsMP\\Editor\\EnvironmentParticlesHolder.tex"


functions:

  void Precache(void)
  {
    CTextureData *ptdHeightMap = (CTextureData *) m_moHeightMapHolder.mo_toTexture.GetData();
    if( ptdHeightMap!=NULL) {
      ptdHeightMap->Force(TEX_CONSTANT|TEX_STATIC|TEX_KEEPCOLOR);
    }
  }


  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if( slPropertyOffset == _offsetof(CEnvironmentParticlesHolder, m_penNextHolder))
    {
      if (IsOfClass(penTarget, "EnvironmentParticlesHolder")) { return TRUE; }
      else { return FALSE; }
    }   
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }


  FLOAT GetRainFactor(void)
  {
    FLOAT fRainFactor = 0.0f;
    TIME tmNow = _pTimer->GetLerpedCurrentTick();
    // if we have Rain
    if( tmNow>m_tmRainStart && tmNow<m_tmRainEnd+m_fRainAppearLen)
    {
      // Rain is on
      if( tmNow>m_tmRainStart+m_fRainAppearLen && tmNow<m_tmRainEnd)
      {
        fRainFactor = 1.0f;
      }
      // Rain is turning off
      else if( tmNow>m_tmRainEnd)
      {
        fRainFactor = 1.0f-(tmNow-m_tmRainEnd)/m_fRainAppearLen;
      }
      // Rain is turning on
      else
      {
        fRainFactor = (tmNow-m_tmRainStart)/m_fRainAppearLen;
      }
    }
    return fRainFactor;
  }
  
  
  FLOAT GetSnowFactor(void)
  {
    FLOAT fSnowFactor = 0.0f;
    TIME tmNow = _pTimer->GetLerpedCurrentTick();
    // if we have Snow
    if( tmNow>m_tmSnowStart && tmNow<m_tmSnowEnd+m_fSnowAppearLen)
    {
      // Snow is on
      if( tmNow>m_tmSnowStart+m_fSnowAppearLen && tmNow<m_tmSnowEnd)
      {
        fSnowFactor = 1.0f;
      }
      // Snow is turning off
      else if( tmNow>m_tmSnowEnd)
      {
        fSnowFactor = 1.0f-(tmNow-m_tmSnowEnd)/m_fSnowAppearLen;
      }
      // Snow is turning on
      else
      {
        fSnowFactor = (tmNow-m_tmSnowStart)/m_fSnowAppearLen;
      }
    }
    return fSnowFactor;
  }
  

  void GetHeightMapData(CTextureData *&ptdHeightMap, FLOATaabbox3D &boxHeightMap)
  {
    ptdHeightMap = (CTextureData *) m_moHeightMapHolder.mo_toTexture.GetData();
    if (ptdHeightMap!=NULL) {
      ptdHeightMap->Force(TEX_CONSTANT|TEX_STATIC|TEX_KEEPCOLOR);
    }
    boxHeightMap = m_boxHeightMap;
    boxHeightMap += GetPlacement().pl_PositionVector;
  }

  void GetParticleTexture()
  {
    
  }

procedures:
  Main(EVoid)
  {
    // set appearance
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // try to obtain height map
    if( m_fnHeightMap != CTString(""))
    {
      try
      {
        m_moHeightMapHolder.mo_toTexture.SetData_t(m_fnHeightMap);
      } catch ( const char *strError) {
        WarningMessage(strError);
      }
    }

    if( m_fnTexture != CTString(""))
    {
      try
      {
        m_moParticleTextureHolder.mo_toTexture.SetData_t(m_fnTexture);
      } catch ( const char *strError) {
        WarningMessage(strError);
      }
    }


    // validate parameters
    if (m_fGrowthRenderingRadius < m_fGrowthRenderingRadiusFade) {
      m_fGrowthRenderingRadiusFade = m_fGrowthRenderingRadius;
    }
    m_fParticlesSinkFactor = Clamp(m_fParticlesSinkFactor, 0.0f, 1.0f);

    // set appearance
    SetModel(MODEL_ENVIRONMENT_PARTICLES_HOLDER);
    SetModelMainTexture(TEXTURE_ENVIRONMENT_PARTICLES_HOLDER);
    
    m_tmRainStart = 1e5f-1.0f;
    m_tmRainEnd = 1e5f;
    m_tmSnowStart = 1e5f-1.0f;
    m_tmSnowEnd = 1e5f;

    switch(m_eptType) {
    case EPTH_GROWTH:
      m_strDescription = "Growth";  
      break;
    case EPTH_RAIN:
      m_strDescription = "Rain";  
      break;
    case EPTH_NONE:
      m_strDescription = "None";  
      break;
    }

    wait() {
      
      on (EBegin) :
      {
        resume;
      }
      on (EStart) :
      {
        TIME tmNow = _pTimer->CurrentTick();
        m_tmRainStart = tmNow;
        m_tmRainEnd = 1e6;
        m_tmSnowStart = tmNow;
        m_tmSnowEnd = 1e6;
        resume;
      }
      on (EStop) :
      {
        TIME tmNow = _pTimer->CurrentTick();
        m_tmRainEnd = tmNow;
        m_tmSnowEnd = tmNow;
        resume;
      }
    }
    // do nothing
    return;
  }

};