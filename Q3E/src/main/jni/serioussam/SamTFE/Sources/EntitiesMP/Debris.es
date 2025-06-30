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

602
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/BasicEffects";

enum DebrisParticlesType {
  0 DPT_NONE        "",   // no particles
  1 DPT_BLOODTRAIL  "",   // blood
  2 DPR_SMOKETRAIL  "",   // smoke
  3 DPR_SPARKS      "",   // sparks (for robots)
  4 DPR_FLYINGTRAIL "",   // just flying object
  5 DPT_AFTERBURNER "",   // afterburner trail
};

// input parameter for spawning a debris
event ESpawnDebris {
  EntityInfoBodyType Eeibt,       // body type
  CModelData *pmd,                // model for this debris
  FLOAT fSize,                    // stretch factor
  CTextureData *ptd,              // texture for this debris
  CTextureData *ptdRefl,          // reflection texture
  CTextureData *ptdSpec,          // specular texture
  CTextureData *ptdBump,          // bump texture
  INDEX iModelAnim,               // animation for debris model
  enum DebrisParticlesType dptParticles,   // particles type
  int /*enum BasicEffectType*/  betStain, // stain left when touching brushes
  COLOR colDebris,                // multiply color for debris
  BOOL bCustomShading,          // if debris uses custom shading
  ANGLE3D aShadingDirection,    // custom shading direction
  COLOR colCustomAmbient,       // custom shading ambient
  COLOR colCustomDiffuse,       // custom shading diffuse
  BOOL bImmaterialASAP,         // if should stop colliding as soon as it hits the ground
  FLOAT fDustStretch,           // if should spawn dust when it hits the ground and size
  FLOAT3D vStretch,             // stretch for spawned model template
  CEntityPointer penFallFXPapa, // parent of all spawned child effects
};

%{
%}

class CDebris: CMovableModelEntity {
name      "Debris";
thumbnail "";

properties:

  1 enum DebrisParticlesType m_dptParticles = DPT_NONE,       // type of particles
  2 INDEX m_iBodyType = 0,                      // body type of this debris
  3 BOOL m_bFade = FALSE,                       // fade debris
  4 FLOAT m_fFadeStartTime = 0.0f,              // fade start time
  5 FLOAT m_fFadeTime = 0.0f,                   // fade time
  6 FLOAT3D m_fLastStainHitPoint = FLOAT3D(0,0,0), // last stain hit point
  7 INDEX /*enum BasicEffectType*/ m_betStain = BET_NONE, // type of stain left
  8 INDEX m_ctLeftStains = 0,                   // count of stains already left
  9 FLOAT m_tmStarted = 0.0f,                   // time when spawned
  10 FLOAT m_fStretch = 1.0f,                   // stretch
  11 ANGLE3D m_aShadingDirection = ANGLE3D(0,0,0),
  12 BOOL m_bCustomShading = FALSE,
  13 COLOR m_colCustomAmbient = COLOR(C_WHITE|CT_OPAQUE),
  14 COLOR m_colCustomDiffuse = COLOR(C_WHITE|CT_OPAQUE),
  15 BOOL m_bImmaterialASAP = FALSE,
  16 FLOAT m_fDustStretch = 0.0f,
  17 BOOL m_bTouchedGround=FALSE,
  18 CEntityPointer m_penFallFXPapa,


components:

  1 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",


functions:

  /* Entity info */
  void *GetEntityInfo(void) {
    return GetStdEntityInfo((EntityInfoBodyType)m_iBodyType);
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // cannot be damaged immediately after spawning
    if ((_pTimer->CurrentTick()-m_tmStarted<1.0f)
      ||((dmtType==DMT_CANNONBALL_EXPLOSION) && (_pTimer->CurrentTick()-m_tmStarted<5.0f))) {
      return;
    }
    CMovableModelEntity::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
  };

/************************************************************
 *                        FADE OUT                          *
 ************************************************************/

  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    if(m_bCustomShading)
    {
      colLight = m_colCustomDiffuse;
      colAmbient = m_colCustomAmbient;
      AnglesToDirectionVector(m_aShadingDirection, vLightDirection);
      vLightDirection = -vLightDirection;
    }

    if (m_bFade) {
      FLOAT fTimeRemain = m_fFadeStartTime + m_fFadeTime - _pTimer->CurrentTick();
      if (fTimeRemain < 0.0f) { fTimeRemain = 0.0f; }
      COLOR colAlpha = GetModelObject()->mo_colBlendColor;
      colAlpha = (colAlpha&0xffffff00) + (COLOR(fTimeRemain/m_fFadeTime*0xff)&0xff);
      GetModelObject()->mo_colBlendColor = colAlpha;
    }
    
    return FALSE;
  };



/************************************************************
 *                        EFFECTS                           *
 ************************************************************/

  // leave a stain where hit
  void LeaveStain(void)
  {
    // if no stains
    if (m_betStain==BET_NONE) {
      // do nothing
      return;
    }

    // don't allow too many stains to be left
    if (m_ctLeftStains>5) {
      return;
    }
    ESpawnEffect ese;
    FLOAT3D vPoint;
    FLOATplane3D plPlaneNormal;
    FLOAT fDistanceToEdge;

    // on plane
    if (GetNearestPolygon(vPoint, plPlaneNormal, fDistanceToEdge)) {
      // away from last hit point and near to polygon
      if ((m_fLastStainHitPoint-vPoint).Length()>3.0f && 
          (vPoint-GetPlacement().pl_PositionVector).Length()<3.5f) {
        m_fLastStainHitPoint = vPoint;
        // stain
        ese.colMuliplier = C_WHITE|CT_OPAQUE;
        ese.betType = (BasicEffectType) m_betStain;
        ese.vNormal = FLOAT3D(plPlaneNormal);
        GetNormalComponent( en_vCurrentTranslationAbsolute, plPlaneNormal, ese.vDirection);
        FLOAT fLength = ese.vDirection.Length() / 7.5f;
        fLength = Clamp( fLength, 1.0f, 15.0f);
        ese.vStretch = FLOAT3D( 1.0f, fLength*1.0f, 1.0f);
        SpawnEffect(CPlacement3D(vPoint+ese.vNormal/50.0f*(FRnd()+0.5f), ANGLE3D(0, 0, 0)), ese);
        m_ctLeftStains++;
      }
    }
  };

  // spawn effect
  void SpawnEffect(const CPlacement3D &plEffect, const class ESpawnEffect &eSpawnEffect)
  {
    CEntityPointer penEffect = CreateEntity(plEffect, CLASS_BASIC_EFFECT);
    penEffect->Initialize(eSpawnEffect);
  };

  // particles
  void RenderParticles(void)
  {
    // if going too slow
    if (en_vCurrentTranslationAbsolute.Length()<0.1f) {
      // don't render particles
      return;
    }
    switch(m_dptParticles) {
    case DPT_BLOODTRAIL: {
      Particles_BloodTrail( this);
                         } break;
    case DPR_SMOKETRAIL: {
      Particles_GrenadeTrail( this);
                         } break;
    case DPR_SPARKS:     {
      Particles_ColoredStarsTrail( this);
                         } break;
    case DPR_FLYINGTRAIL:{
      //Particles_WhiteLineTrail( this);
      Particles_BombTrail( this);
                         } break;
    case DPT_AFTERBURNER:{
      Particles_AfterBurner( this, m_tmStarted, 0.5f);
                         } break;
    default: ASSERT(FALSE);
    case DPT_NONE:
      return;
    }
  };

  // explode
  void Explode(void)
  {
    // spawn explosion
    CPlacement3D plExplosion = GetPlacement();
    CEntityPointer penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_BOMB;
    eSpawnEffect.vStretch = FLOAT3D(0.3f,0.3f,0.3f);
    penExplosion->Initialize(eSpawnEffect);
  }

/************************************************************
 *                          MAIN                            *
 ************************************************************/

procedures:

  Main(ESpawnDebris eSpawn)
  {
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_BOUNCING|EPF_CANFADESPINNING);
    SetCollisionFlags(ECF_DEBRIS);
    SetFlags(GetFlags() | ENF_SEETHROUGH);
    SetHealth(25.0f);
    en_fBounceDampNormal   = 0.15f;
    en_fBounceDampParallel = 0.5f;
    en_fJumpControlMultiplier = 0.0f;

    // set density
    if (eSpawn.Eeibt==EIBT_ICE) {
      en_fDensity = 500.0f;
    } else if (eSpawn.Eeibt==EIBT_WOOD) {
      en_fDensity = 500.0f;
    } else if (eSpawn.Eeibt==EIBT_FLESH) {
      en_fDensity = 5000.0f;
      en_fBounceDampNormal   = 0.25f;
      en_fBounceDampParallel = 0.75f;
    } else if (TRUE) {
      en_fDensity = 5000.0f;
    }

    // set appearance
    m_dptParticles = eSpawn.dptParticles;
    if(m_dptParticles==DPT_AFTERBURNER)
    {
      Particles_AfterBurner_Prepare(this);
    }
    m_betStain = eSpawn.betStain;
    m_iBodyType = (INDEX)eSpawn.Eeibt;
    GetModelObject()->SetData(eSpawn.pmd);
    GetModelObject()->mo_toTexture.SetData(eSpawn.ptd);
    GetModelObject()->mo_toReflection.SetData(eSpawn.ptdRefl);
    GetModelObject()->mo_toSpecular.SetData(eSpawn.ptdSpec);
    GetModelObject()->mo_toBump.SetData(eSpawn.ptdBump);
    GetModelObject()->PlayAnim(eSpawn.iModelAnim, AOF_LOOPING);
    GetModelObject()->mo_Stretch = FLOAT3D( eSpawn.fSize, eSpawn.fSize, eSpawn.fSize);
    GetModelObject()->mo_Stretch(1)*=eSpawn.vStretch(1);
    GetModelObject()->mo_Stretch(2)*=eSpawn.vStretch(2);
    GetModelObject()->mo_Stretch(3)*=eSpawn.vStretch(3);

    // adjust color
    GetModelObject()->mo_colBlendColor = eSpawn.colDebris|CT_OPAQUE;
    m_bCustomShading = eSpawn.bCustomShading;
    if( m_bCustomShading)
    {
      en_ulFlags|=ENF_NOSHADINGINFO;
    }
    m_aShadingDirection = eSpawn.aShadingDirection;
    m_colCustomAmbient = eSpawn.colCustomAmbient;
    m_colCustomDiffuse = eSpawn.colCustomDiffuse;
    m_bImmaterialASAP = eSpawn.bImmaterialASAP;
    m_fDustStretch = eSpawn.fDustStretch;
    m_penFallFXPapa=eSpawn.penFallFXPapa;
    
    ModelChangeNotify();
    FLOATaabbox3D box;
    GetBoundingBox(box);
    FLOAT fEntitySize = box.Size().MaxNorm();
    if (fEntitySize>0.5f) {
      SetCollisionFlags(ECF_MODEL);
    }
    en_fCollisionSpeedLimit+=ClampDn(0.0f, fEntitySize*10.0f);
    m_bFade = FALSE;
    m_fLastStainHitPoint = FLOAT3D(32000.0f, 32000.0f, 32000.0f);
    m_ctLeftStains = 0;
    m_tmStarted = _pTimer->CurrentTick();
    m_bTouchedGround=FALSE;

    // wait some time
    FLOAT fWaitBeforeFade=FRnd()*2.0f + 3.0f;
    wait(fWaitBeforeFade)
    {
      on (EBegin) : { resume; }
      // if touched something
      on (ETouch etouch) : {
        // if it is brush
        if (etouch.penOther->GetRenderType()==RT_BRUSH)
        {
          // shake or some other effect
          if( m_penFallFXPapa!=NULL && !m_bTouchedGround)
          {
            // loop all children of FX papa
            FOREACHINLIST( CEntity, en_lnInParent, m_penFallFXPapa->en_lhChildren, iten)
            {
              // start it
              CEntity *penNew = GetWorld()->CopyEntityInWorld( *iten, GetPlacement());
              penNew->SetParent(NULL);
              if( IsOfClass(&*penNew, "SoundHolder"))
              {
                penNew->SendEvent( EStart());
              }
              else
              {
                penNew->SendEvent( ETrigger());
              }
            }
          }

          if( m_fDustStretch>0 && !m_bTouchedGround)
          {
            // spawn dust
            CPlacement3D plDust=GetPlacement();
            plDust.pl_PositionVector=plDust.pl_PositionVector+FLOAT3D(0,m_fDustStretch*0.25f,0);
            // spawn dust effect
            ESpawnEffect ese;
            ese.colMuliplier = C_WHITE|CT_OPAQUE;
            ese.vStretch = FLOAT3D(m_fDustStretch,m_fDustStretch,m_fDustStretch);
            ese.vNormal = FLOAT3D(0,1,0);
            ese.betType = BET_DUST_FALL;
            CEntityPointer penFX = CreateEntity(plDust, CLASS_BASIC_EFFECT);
            penFX->Initialize(ese);
          }
          m_bTouchedGround=TRUE;

          // maybe leave stain
          LeaveStain();
          // if robot
          if (m_iBodyType==EIBT_ROBOT)
          {
            // explode
            Explode();
            SendEvent(EDeath());
            resume;
          }
        }
        if( m_bImmaterialASAP)
        {
          SetCollisionFlags(ECF_DEBRIS);
        }
        resume;
      }
      on (EDeath) : { Destroy(); return; }
      on (ETimer) : { stop; }
    }

    // fade away
    SetCollisionFlags(ECF_DEBRIS);
    m_fFadeStartTime = _pTimer->CurrentTick();
    m_fFadeTime = 5.0f;
    m_bFade = TRUE;
    autowait(m_fFadeTime);

    // cease to exist
    Destroy();

    return;
  }
};
