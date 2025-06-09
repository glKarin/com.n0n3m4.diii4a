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

605
%{
#include "EntitiesMP/StdH/StdH.h"
%}

class CWorldSettingsController: CEntity {
name      "WorldSettingsController";
thumbnail "Thumbnails\\WorldSettingsController.tbn";
features  "IsTargetable", "HasName", "IsImportant";

properties:
  1 FLOAT m_tmStormStart = -1.0f,                 // storm start time
  2 CTString m_strName "Name" 'N' = "World settings controller",         // class name
  3 FLOAT m_tmLightningStart = -1.0f,             // lightning start time
  4 FLOAT m_fLightningPower = 1.0f,               // lightning power
  5 FLOAT m_tmStormEnd = -1.0f,                   // storm end time
  6 FLOAT m_tmPyramidPlatesStart = 1e6f,          // time when pyramid plates blend started
  7 FLOAT m_tmActivatedPlate1 = 1e6f,             // time when plate 1 has been activated
  8 FLOAT m_tmDeactivatedPlate1 = 1e6f,           // time when plate 1 has been deactivated
  9 FLOAT m_tmActivatedPlate2 = 1e6f,             // time when plate 2 has been activated
 10 FLOAT m_tmDeactivatedPlate2 = 1e6f,           // time when plate 2 has been deactivated
 11 FLOAT m_tmActivatedPlate3 = 1e6f,             // time when plate 3 has been activated
 12 FLOAT m_tmDeactivatedPlate3 = 1e6f,           // time when plate 3 has been deactivated
 13 FLOAT m_tmActivatedPlate4 = 1e6f,             // time when plate 4 has been activated
 14 FLOAT m_tmDeactivatedPlate4 = 1e6f,           // time when plate 4 has been deactivated
 15 FLOAT m_tmPyramidMorphRoomActivated = 1e6f,   // time when pyramid morph room has been activated

 20 FLOAT m_tmShakeStarted = -1.0f,       // time when shaking started
 21 FLOAT3D m_vShakePos = FLOAT3D(0,0,0), // shake position
 22 FLOAT m_fShakeFalloff = 100.0f,       // fall off with distance
 23 FLOAT m_fShakeFade = 1.0f,            // fall off with time
 24 FLOAT m_fShakeIntensityY = 1.0f,      // shake strength
 25 FLOAT m_tmShakeFrequencyY = 1.0f,      // shake strength
 26 FLOAT m_fShakeIntensityB = 1.0f,      // shake strength
 27 FLOAT m_tmShakeFrequencyB = 1.0f,      // shake strength
 31 FLOAT m_fShakeIntensityZ = 1.0f,      // shake strength
 32 FLOAT m_tmShakeFrequencyZ = 1.0f,      // shake strength
 33 BOOL  m_bShakeFadeIn = FALSE, // TRUE = intensify over time, FALSE = start immediately and falloff

 28 CEntityPointer m_penEnvPartHolder "Environment Particles Holder" 'T',
 29 BOOL  m_bNoSaveGame "No QuickSave on level start"= FALSE, // TRUE means no quicksave will be made when starting this level
 //28 CTFileName m_fnHeightMap "Height map" 'R' = CTString(""),
 //29 CModelObject m_moHeightMapHolder,
 //30 FLOATaabbox3D m_boxHeightMap "Height map box" 'B' = FLOATaabbox3D(FLOAT3D(0,0,0), FLOAT3D(1,1,1)),

 41 FLOAT m_tmGlaringStarted = -1.0f,          // glaring start time
 42 FLOAT m_tmGlaringEnded = -1.0f,            // glaring end time
 43 FLOAT m_fGlaringFadeInRatio = 0.1f,       // glaring fade in ratio
 44 FLOAT m_fGlaringFadeOutRatio = 0.1f,      // glaring fade out ratio
 45 COLOR m_colGlade = COLOR(C_WHITE|CT_TRANSPARENT),

 50 COLOR m_colBlendStart = COLOR(C_WHITE|CT_TRANSPARENT),
 51 COLOR m_colBlendStop = COLOR(C_WHITE|CT_OPAQUE),
 52 COLOR m_colShadeStart = COLOR(C_WHITE|CT_OPAQUE),
 53 COLOR m_colShadeStop = COLOR(C_GRAY|CT_OPAQUE),

 55 BOOL m_bApplyShadingToModels "Apply shading to models" = FALSE,
 
 //60 FLOAT m_tmRainStart = -1.0f,                 // Rain start time
 //61 FLOAT m_tmRainEnd = -1.0f,                   // Rain end time
 62 FLOAT m_tmStormAppearTime = 10.0f,
 63 FLOAT m_tmStormDisappearTime = 10.0f,

 70 CEntityPointer m_penScrollHolder,
 71 CEntityPointer m_penTextFXHolder,
 72 CEntityPointer m_penCreditsHolder,
 73 CEntityPointer m_penHudPicFXHolder,

components:
  1 model   MODEL_WORLD_SETTINGS_CONTROLLER     "Models\\Editor\\WorldSettingsController.mdl",
  2 texture TEXTURE_WORLD_SETTINGS_CONTROLLER   "Models\\Editor\\WorldSettingsController.tex"

functions:
  
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if( slPropertyOffset == _offsetof(CWorldSettingsController, m_penEnvPartHolder))
    {
      if (IsOfClass(penTarget, "EnvironmentParticlesHolder")) { return TRUE; }
      else { return FALSE; }
    }   
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }

  FLOAT GetStormFactor(void)
  {
    FLOAT fStormFactor = 0.0f;
    TIME tmNow = _pTimer->GetLerpedCurrentTick();
    // if we have storm
    if( tmNow>m_tmStormStart && tmNow<m_tmStormEnd+m_tmStormDisappearTime)
    {
      // storm is on
      if( tmNow>m_tmStormStart+m_tmStormAppearTime && tmNow<m_tmStormEnd)
      {
        fStormFactor = 1.0f;
      }
      // storm is turning off
      else if( tmNow>m_tmStormEnd)
      {
        fStormFactor = 1.0f-(tmNow-m_tmStormEnd)/m_tmStormDisappearTime;
      }
      // storm is turning on
      else
      {
        fStormFactor = (tmNow-m_tmStormStart)/m_tmStormAppearTime;
      }
    }
    return fStormFactor;
  }
  
  /* Handle an event, return false if the event is not handled. */
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    if( ee.ee_slEvent==EVENTCODE_EScroll)
    {
      EScroll escr = ((EScroll &) ee);
      if( escr.bStart)
      {
        m_penScrollHolder=escr.penSender;
      }
      else
      {
        if( m_penScrollHolder==escr.penSender)
        {
          m_penScrollHolder=NULL;
        }
      }
      return TRUE;
    }
    if( ee.ee_slEvent==EVENTCODE_ETextFX)
    {
      ETextFX etfx = ((ETextFX &) ee);
      if( etfx.bStart)
      {
        m_penTextFXHolder=etfx.penSender;
      }
      else
      {
        if( m_penTextFXHolder==etfx.penSender)
        {
          m_penTextFXHolder=NULL;
        }
      }
      return TRUE;
    }
    if( ee.ee_slEvent==EVENTCODE_EHudPicFX)
    {
      EHudPicFX ehpfx = ((EHudPicFX &) ee);
      if( ehpfx.bStart)
      {
        m_penHudPicFXHolder=ehpfx.penSender;
      }
      else
      {
        if( m_penHudPicFXHolder==ehpfx.penSender)
        {
          m_penHudPicFXHolder=NULL;
        }
      }
      return TRUE;
    }
    if( ee.ee_slEvent==EVENTCODE_ECredits)
    {
      ECredits ecr = ((ECredits &) ee);
      if( ecr.bStart)
      {
        m_penCreditsHolder=ecr.penSender;
      }
      else
      {
        if( m_penCreditsHolder==ecr.penSender)
        {
          m_penCreditsHolder=NULL;
        }
      }
      return TRUE;
    }    
    return FALSE;
  }
  
procedures:
  Main(EVoid)
  {
    m_penScrollHolder=NULL;
    m_penTextFXHolder=NULL;
    // set appearance
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_WORLD_SETTINGS_CONTROLLER);
    SetModelMainTexture(TEXTURE_WORLD_SETTINGS_CONTROLLER);
    
    m_tmStormStart = 1e5f-1.0f;
    m_tmStormEnd = 1e5f;

    // do nothing
    return;
  }
};
