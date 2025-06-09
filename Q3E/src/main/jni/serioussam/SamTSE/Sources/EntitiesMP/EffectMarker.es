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

611
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/Effector.h"
#include "EntitiesMP/BackgroundViewer.h"
#include "EntitiesMP/WorldSettingsController.h"
%}

uses "EntitiesMP/Marker";
uses "EntitiesMP/BasicEffects";
enum EffectMarkerType {
  0 EMT_NONE                      "None",                      // no FX
  1 EMT_PLAYER_APPEAR             "Player appear",             // effect of player appearing
  2 EMT_APPEARING_BIG_BLUE_FLARE  "Appear big blue flare",     // appear big blue flare
  3 EMT_BLEND_MODELS              "Blend two models",          // blend between two models
  4 EMT_DISAPPEAR_MODEL           "Disappear model",           // disappear model
  5 EMT_APPEAR_MODEL              "Appear model",              // appear model
  6 EMT_HIDE_ENTITY               "Hide entity",               // hide entity
  7 EMT_SHOW_ENTITY               "Show entity",               // show entity
  8 EMT_SHAKE_IT_BABY             "Shake it baby",             // earth quaker
  9 EMT_APPEAR_DISAPPEAR          "Appear or Disappear model", // appear/disappear model
 10 EMT_SET_PLACEMENT             "Teleport",                  // teleport entity
 11 EMT_SET_PARENT                "Set parent",                // set parent 
 12 EMT_CLEAR_PARENT              "Clear parent",              // clear parent 
 13 EMT_BASIC_EFFECT              "Basic effect",              // spawn basic effect
 14 EMT_GLARE                     "Glare",                     // glare screen
};

%{
extern void CBasicEffect_OnPrecache(CDLLEntityClass *pdec, INDEX iUser);
%}

class CEffectMarker: CMarker
{
name      "Effect Marker";
thumbnail "Thumbnails\\EffectMarker.tbn";

properties:

  1 enum EffectMarkerType m_emtType  "Effect type" 'Y' = EMT_NONE,        // type of effect
  2 CEntityPointer m_penModel        "FX Entity 1" 'E',                  // model holder used in this effect
  3 FLOAT m_tmEffectLife             "FX Life time" 'L' = 10.0f,      // life time of this effect
  4 CEntityPointer m_penModel2       "FX Entity 2" 'R',                // second model holder used in this effect
  5 CEntityPointer m_penEffector,                                     // ptr to spawned effector
  6 FLOAT m_fShakeFalloff            "Shake fall off" = 250.0f,       // ShakeFalloff
  7 FLOAT m_fShakeFade               "Shake fade" = 3.0f,             // ShakeFade
  8 FLOAT m_fShakeIntensityY         "Shake intensity Y" = 0.1f,      // ShakeIntensityY
  9 FLOAT m_fShakeFrequencyY         "Shake frequency Y" = 5.0f,      // ShakeFrequencyY
  10 FLOAT m_fShakeIntensityB        "Shake intensity B" = 2.5f,      // ShakeIntensityB
  11 FLOAT m_fShakeFrequencyB        "Shake frequency B" = 7.2f,      // ShakeFrequencyB
  12 FLOAT m_fShakeIntensityZ        "Shake intensity Z" = 0.0f,      // ShakeIntensityZ
  13 FLOAT m_fShakeFrequencyZ        "Shake frequency Z" = 5.0f,      // ShakeFrequencyZ
  20 enum BasicEffectType m_betType  "Basic effect type" 'P' = BET_ROCKET,    // type of basic effect
  21 FLOAT m_fStretch                "Stretch" 'S' = 1.0f,
  22 COLOR m_colColor                "FX color" 'C' = COLOR(C_WHITE|CT_TRANSPARENT),

components:

  1 model   MODEL_MARKER          "Models\\Editor\\Axis.mdl",
  2 texture TEXTURE_MARKER        "Models\\Editor\\Vector.tex",
  3 class   CLASS_EFFECTOR        "Classes\\Effector.ecl",
  4 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",


functions:
  void Precache(void)
  {
    switch(m_betType)
    {
      case EMT_BASIC_EFFECT:
        CDLLEntityClass *pdec = &CBasicEffect_DLLClass;
        // precache
        CBasicEffect_OnPrecache(pdec, m_emtType);
        break;
    }
  }

  // Validate offered target for one property
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if(penTarget==NULL)
    {
      return FALSE;
    }
    // if should be modelobject
    if( slPropertyOffset==_offsetof(CEffectMarker, m_penModel) ||
        slPropertyOffset==_offsetof(CEffectMarker, m_penModel2) )
    {
      return IsOfClass(penTarget, "ModelHolder2");
    }
    return TRUE;
  }

  /* Handle an event, return false if the event is not handled. */
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    if (ee.ee_slEvent==EVENTCODE_ETrigger)
    {
      switch(m_emtType)
      {
        case EMT_SET_PLACEMENT:
        {
          if(m_penModel!=NULL && m_penModel2!=NULL)
          {
            m_penModel->Teleport(m_penModel2->GetPlacement(), FALSE);
          }
          break;
        }
        case EMT_SET_PARENT:
        {
          if(m_penModel!=NULL && m_penModel2!=NULL)
          {
            m_penModel->SetParent(m_penModel2);
          }
          break;
        }
        case EMT_CLEAR_PARENT:
        {
          if(m_penModel!=NULL)
          {
            m_penModel->SetParent(NULL);
          }
          break;
        }
        case EMT_SHAKE_IT_BABY:
        {
          // ---------- Apply shake
          CWorldSettingsController *pwsc = NULL;
          // obtain bcg viewer
          CBackgroundViewer *penBcgViewer = (CBackgroundViewer *) GetWorld()->GetBackgroundViewer();
          if( penBcgViewer!=NULL && penBcgViewer->m_penWorldSettingsController!=NULL)
          {
            pwsc = (CWorldSettingsController *) penBcgViewer->m_penWorldSettingsController.ep_pen;
            pwsc->m_tmShakeStarted = _pTimer->CurrentTick();
            pwsc->m_vShakePos = GetPlacement().pl_PositionVector;
            pwsc->m_fShakeFalloff = m_fShakeFalloff;
            pwsc->m_fShakeFade = m_fShakeFade;
            pwsc->m_fShakeIntensityZ = m_fShakeIntensityZ;
            pwsc->m_tmShakeFrequencyZ = m_fShakeFrequencyZ;
            pwsc->m_fShakeIntensityY = m_fShakeIntensityY;
            pwsc->m_tmShakeFrequencyY = m_fShakeFrequencyY;
            pwsc->m_fShakeIntensityB = m_fShakeIntensityB;
            pwsc->m_tmShakeFrequencyB = m_fShakeFrequencyB;
            pwsc->m_bShakeFadeIn = FALSE;
          }
          break;
        }
        case EMT_HIDE_ENTITY:
        {
          if( m_penTarget!=NULL)
          {
            m_penTarget->SetFlags(m_penTarget->GetFlags()|ENF_HIDDEN);
          }
          break;
        }
        case EMT_SHOW_ENTITY:
        {
          if( m_penTarget!=NULL)
          {
            m_penTarget->SetFlags(m_penTarget->GetFlags()&~ENF_HIDDEN);
          }
          break;
        }
        case EMT_PLAYER_APPEAR:
          if( m_penModel!=NULL && IsOfClass(m_penModel, "ModelHolder2") )
          {
            CModelObject *pmo = m_penModel->GetModelObject();
            if( pmo != NULL) 
            {
              // spawn effect
              CPlacement3D plFX= m_penModel->GetPlacement();
              CEntity *penFX = CreateEntity( plFX, CLASS_EFFECTOR);
              ESpawnEffector eSpawnFX;
              eSpawnFX.tmLifeTime = m_tmEffectLife;
              eSpawnFX.eetType = ET_PORTAL_LIGHTNING;
              eSpawnFX.penModel = m_penModel;
              penFX->Initialize( eSpawnFX);
            }
          }
          break;
        case EMT_APPEARING_BIG_BLUE_FLARE:
          {
            // spawn effect
            CPlacement3D plFX= GetPlacement();
            CEntity *penFX = CreateEntity( plFX, CLASS_EFFECTOR);
            ESpawnEffector eSpawnFX;
            eSpawnFX.tmLifeTime = m_tmEffectLife;
            eSpawnFX.fSize = 1.0f;
            eSpawnFX.eetType = ET_SIZING_BIG_BLUE_FLARE;
            penFX->Initialize( eSpawnFX);
            break;
          }
        case EMT_BLEND_MODELS:
        if(m_penModel!=NULL && IsOfClass(m_penModel, "ModelHolder2") &&
           m_penModel2!=NULL && IsOfClass(m_penModel2, "ModelHolder2") )
        {
          if( m_penEffector == NULL)
          {
            CModelObject *pmo1 = m_penModel->GetModelObject();
            CModelObject *pmo2 = m_penModel2->GetModelObject();
            if( pmo1 != NULL && pmo2 != NULL)
            {
              // spawn effect
              CPlacement3D plFX= m_penModel->GetPlacement();
              CEntity *penFX = CreateEntity( plFX, CLASS_EFFECTOR);
              ESpawnEffector eSpawnFX;
              eSpawnFX.tmLifeTime = m_tmEffectLife;
              eSpawnFX.eetType = ET_MORPH_MODELS;
              eSpawnFX.penModel = m_penModel;
              eSpawnFX.penModel2 = m_penModel2;
              penFX->Initialize( eSpawnFX);
              m_penEffector = penFX;
            }
          }
          else
          {
            m_penEffector->SendEvent(ETrigger());
          }
        }
        break;
        case EMT_DISAPPEAR_MODEL:
        if(m_penModel!=NULL && IsOfClass(m_penModel, "ModelHolder2"))
        {
          if( m_penEffector == NULL)
          {
            CModelObject *pmo = m_penModel->GetModelObject();
            if( pmo != NULL)
            {
              // spawn effect
              CPlacement3D plFX= m_penModel->GetPlacement();
              CEntity *penFX = CreateEntity( plFX, CLASS_EFFECTOR);
              ESpawnEffector eSpawnFX;
              eSpawnFX.tmLifeTime = m_tmEffectLife;
              eSpawnFX.eetType = ET_DISAPPEAR_MODEL;
              eSpawnFX.penModel = m_penModel;
              penFX->Initialize( eSpawnFX);
              m_penEffector = penFX;
            }
          }
          else
          {
            m_penEffector->SendEvent(ETrigger());
          }
        }
        break;
        case EMT_APPEAR_MODEL:
        if(m_penModel!=NULL && IsOfClass(m_penModel, "ModelHolder2"))
        {
          if( m_penEffector == NULL)
          {
            CModelObject *pmo = m_penModel->GetModelObject();
            if( pmo != NULL)
            {
              // spawn effect
              CPlacement3D plFX= m_penModel->GetPlacement();
              CEntity *penFX = CreateEntity( plFX, CLASS_EFFECTOR);
              ESpawnEffector eSpawnFX;
              eSpawnFX.tmLifeTime = m_tmEffectLife;
              eSpawnFX.eetType = ET_APPEAR_MODEL;
              eSpawnFX.penModel = m_penModel;
              penFX->Initialize( eSpawnFX);
              m_penEffector = penFX;
            }
          }
          else
          {
            m_penEffector->SendEvent(ETrigger());
          }
        }
        break;
        case EMT_BASIC_EFFECT:
        {
          // spawn effect
          CPlacement3D plEffect = GetPlacement();
          CEntityPointer penEffect = CreateEntity(plEffect, CLASS_BASIC_EFFECT);
          ESpawnEffect eSpawnEffect;
          eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
          eSpawnEffect.betType = m_betType;
          eSpawnEffect.vStretch = FLOAT3D(m_fStretch,m_fStretch,m_fStretch);
          penEffect->Initialize(eSpawnEffect);
        }
        break;
        case EMT_GLARE:
        CWorldSettingsController *pwsc = GetWSC(this);
        if (pwsc!=NULL)
        {
          pwsc->m_colGlade=m_colColor;
          pwsc->m_tmGlaringStarted = _pTimer->CurrentTick();
          pwsc->m_tmGlaringEnded = pwsc->m_tmGlaringStarted+m_tmEffectLife,
          pwsc->m_fGlaringFadeInRatio = 0.2f;
          pwsc->m_fGlaringFadeOutRatio = 0.7f;
        }
        break;
      }
    }
    else if (ee.ee_slEvent==EVENTCODE_EActivate)
    {
      switch(m_emtType)
      {
        case EMT_APPEAR_DISAPPEAR:
        if(m_penModel!=NULL && IsOfClass(m_penModel, "ModelHolder2"))
        {
          CModelObject *pmo = m_penModel->GetModelObject();
          if( pmo != NULL)
          {
            // spawn effect
            CPlacement3D plFX= m_penModel->GetPlacement();
            CEntity *penFX = CreateEntity( plFX, CLASS_EFFECTOR);
            ESpawnEffector eSpawnFX;
            eSpawnFX.tmLifeTime = m_tmEffectLife;
            eSpawnFX.eetType = ET_APPEAR_MODEL_NOW;
            eSpawnFX.penModel = m_penModel;
            penFX->Initialize( eSpawnFX);
            m_penEffector = penFX;
          }
        }
        break;
      }
    }
    else if (ee.ee_slEvent==EVENTCODE_EDeactivate)
    {
      switch(m_emtType)
      {
        case EMT_APPEAR_DISAPPEAR:
        if(m_penModel!=NULL && IsOfClass(m_penModel, "ModelHolder2"))
        {
          CModelObject *pmo = m_penModel->GetModelObject();
          if( pmo != NULL)
          {
            // spawn effect
            CPlacement3D plFX= m_penModel->GetPlacement();
            CEntity *penFX = CreateEntity( plFX, CLASS_EFFECTOR);
            ESpawnEffector eSpawnFX;
            eSpawnFX.tmLifeTime = m_tmEffectLife;
            eSpawnFX.eetType = ET_DISAPPEAR_MODEL_NOW;
            eSpawnFX.penModel = m_penModel;
            penFX->Initialize( eSpawnFX);
            m_penEffector = penFX;
          }
        }
        break;
      }
    }
    return FALSE;
  }

procedures:

  Main()
  {
    // init model
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);
    // reset entity ptr
    m_penEffector = NULL;
    return;
  }
};

