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

351
%{
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/Enemies/ExotechLarva/Charger/WallCharger.h"
%}

uses "EntitiesMP/BloodSpray";
uses "EntitiesMP/Projectile";

class CExotechLarvaBattery : CRationalEntity {
name      "ExotechLarvaBattery";
thumbnail "Thumbnails\\ExotechLarvaBattery.tbn";
features  "HasName", "IsTargetable";

properties:
  
  1 BOOL  m_bActive  = TRUE,
  2 FLOAT m_fMaxHealth "Health" 'H' = 100.0f,
  3 FLOAT m_fStretch "Stretch" 'S' = 1.0f,
  4 FLOAT m_fBurnTreshold = 0.0f,
  5 CEntityPointer m_penSpray,       // debris spray
  6 FLOAT m_tmSpraySpawned = 0.0f,
  7 CTString m_strName  "Name" 'N' = "ExotechLarva Wall Battery",

 10 BOOL m_bCustomShading "Custom Shading" = FALSE,
 11 ANGLE3D m_aShadingDirection "Light direction" 'D' = ANGLE3D( AngleDeg(45.0f),AngleDeg(45.0f),AngleDeg(45.0f)),
 12 COLOR m_colLight "Light Color" = C_WHITE,
 13 COLOR m_colAmbient "Ambient Light Color" = C_BLACK,
 
 20 CSoundObject m_soSound,

components:

  1 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",
  2 class   CLASS_BLOOD_SPRAY   "Classes\\BloodSpray.ecl",
  
  5 model   MODEL_BATTERY       "ModelsMP\\Enemies\\ExotechLarva\\Charger\\WallCharger.mdl",
  6 texture TEXTURE_BATTERY     "ModelsMP\\Enemies\\ExotechLarva\\Charger\\WallCharger.tex",

  7 model   MODEL_BEAM          "ModelsMP\\Enemies\\ExotechLarva\\Charger\\Beam.mdl",
  8 texture TEXTURE_BEAM        "ModelsMP\\Enemies\\ExotechLarva\\Charger\\Beam.tex",
   
 10 model   MODEL_PLASMA        "ModelsMP\\Enemies\\ExotechLarva\\Charger\\PlasmaBeam.mdl",
 11 texture TEXTURE_PLASMA      "ModelsMP\\Effects\\Laser\\Laser_Red.tex",
 
 12 model   MODEL_ELECTRO       "ModelsMP\\Enemies\\ExotechLarva\\Charger\\Electricity.mdl",
 13 texture TEXTURE_ELECTRO     "ModelsMP\\Enemies\\ExotechLarva\\Projectile\\Projectile.tex",

 50 sound   SOUND_SHUTDOWN      "ModelsMP\\Enemies\\ExotechLarva\\Charger\\Sounds\\WallChargerShutdown.wav",

functions:
  
  void Precache(void) {
    CRationalEntity::Precache();
    PrecacheSound( SOUND_SHUTDOWN       );
    PrecacheModel( MODEL_BATTERY        );
    PrecacheTexture( TEXTURE_BATTERY    );
    PrecacheModel( MODEL_PLASMA         );
    PrecacheTexture( TEXTURE_PLASMA     );
    PrecacheModel( MODEL_ELECTRO        );
    PrecacheTexture( TEXTURE_ELECTRO    );
  }
  

  /* Adjust model shading parameters if needed. */
  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    if (m_bCustomShading)
    {
      colLight   = m_colLight;
      colAmbient = m_colAmbient;
         
      AnglesToDirectionVector(m_aShadingDirection, vLightDirection);
      vLightDirection = -vLightDirection;
    }
    
    return TRUE;
  };

  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    if (GetHealth()<0.0f) {
      return;
    }

    if((dmtType!=DMT_BURNING) && (m_tmSpraySpawned<=_pTimer->CurrentTick()-_pTimer->TickQuantum*8))
    {
      // spawn blood spray
      CPlacement3D plSpray = CPlacement3D( vHitPoint, ANGLE3D(0, 0, 0));
      m_penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
      m_penSpray->SetParent(this);
      ESpawnSpray eSpawnSpray;
      eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
      eSpawnSpray.fDamagePower = 3.0f;
      eSpawnSpray.sptType = SPT_ELECTRICITY_SPARKS_NO_BLOOD;
      eSpawnSpray.fSizeMultiplier = 1.0f;
      eSpawnSpray.vDirection = FLOAT3D(0.0f, 1.0f, 0.0f);
      eSpawnSpray.penOwner = this;
      m_penSpray->Initialize( eSpawnSpray);

      m_tmSpraySpawned = _pTimer->CurrentTick();
    }

    FLOAT fLastHealth = GetHealth();
    CRationalEntity::ReceiveDamage(penInflictor, dmtType, fDamageAmmount,
                              vHitPoint, vDirection);    
    FLOAT fNewHealth = GetHealth();
    if (fNewHealth<=0.66f*m_fMaxHealth && fLastHealth>0.66f*m_fMaxHealth) {
      RemoveAttachment(WALLCHARGER_ATTACHMENT_LIGHT);
      GetModelObject()->PlayAnim(WALLCHARGER_ANIM_DAMAGE01, AOF_SMOOTHCHANGE|AOF_NORESTART);
      SpawnExplosions();
    } else if (fNewHealth<=0.33f*m_fMaxHealth && fLastHealth>0.33f*m_fMaxHealth) {
      RemoveAttachment(WALLCHARGER_ATTACHMENT_PLASMA);
      GetModelObject()->PlayAnim(WALLCHARGER_ANIM_DAMAGE02, AOF_SMOOTHCHANGE|AOF_NORESTART);
      SpawnExplosions();
    }
  };

  void RenderParticles(void)
  {
    FLOAT fBurnStrength;
    FLOAT fHealth = GetHealth();
    if (fHealth<m_fBurnTreshold) {
      fBurnStrength = 1.0f - fHealth/m_fBurnTreshold;
      if (fBurnStrength>0.99f) { fBurnStrength=0.99f; }
      Particles_Burning(this, 1.0f, fBurnStrength);
    }
    if (fHealth<1.0f) {
      Particles_Smoke(this, FLOAT3D(0.0f, 0.0f, 0.25f)*m_fStretch, 100, 6.0f, 0.4f, 4.0f*m_fStretch, 2.5f); 
    }
  };

  void SpawnExplosions(void) {
    CPlacement3D pl = GetPlacement();
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_CANNON;
    eSpawnEffect.vStretch = FLOAT3D(m_fStretch*1.5f, m_fStretch*1.5f, m_fStretch*1.5f);
    CEntityPointer penExplosion = CreateEntity(pl, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);
    pl.pl_PositionVector += FLOAT3D(FRnd()*0.5f, FRnd()*0.5f, 0.0f);
    penExplosion = CreateEntity(pl, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);
    pl.pl_PositionVector += FLOAT3D(FRnd()*0.5f, FRnd()*0.5f, 0.0f);
    penExplosion = CreateEntity(pl, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);
    InflictRangeDamage( this, DMT_EXPLOSION, 25.0f, GetPlacement().pl_PositionVector, 5.0f, 25.0f);
  }
  
  void AddAttachments(void) {
    AddAttachmentToModel(this, *GetModelObject(), WALLCHARGER_ATTACHMENT_LIGHT, MODEL_BEAM, TEXTURE_BEAM, 0, 0, 0);
    AddAttachmentToModel(this, *GetModelObject(), WALLCHARGER_ATTACHMENT_PLASMA, MODEL_PLASMA, TEXTURE_PLASMA, 0, 0, 0);
    AddAttachmentToModel(this, *GetModelObject(), WALLCHARGER_ATTACHMENT_ELECTRICITY, MODEL_ELECTRO, TEXTURE_ELECTRO, 0, 0, 0);
    AddAttachmentToModel(this, *GetModelObject(), WALLCHARGER_ATTACHMENT_ELECTRICITY2, MODEL_ELECTRO, TEXTURE_ELECTRO, 0, 0, 0);
  }

  void RemoveAttachment(INDEX iAtt) {
    RemoveAttachmentFromModel(*GetModelObject(), iAtt);
  }

procedures:
  Destroyed()
  {
    m_bActive = FALSE;

    RemoveAttachment(WALLCHARGER_ATTACHMENT_ELECTRICITY);
    RemoveAttachment(WALLCHARGER_ATTACHMENT_ELECTRICITY2);
    PlaySound(m_soSound, SOUND_SHUTDOWN, SOF_3D);
    GetModelObject()->PlayAnim(WALLCHARGER_ANIM_DAMAGE03, AOF_SMOOTHCHANGE|AOF_NORESTART);
    SpawnExplosions();
      
    //wait forever
    while (TRUE) {
      autowait(1.0f);
    }
    return;
  }
  
  Main()
  {
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_FIXED);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    
    // set your appearance
    SetModel(MODEL_BATTERY);
    SetModelMainTexture(TEXTURE_BATTERY);
    
    AddAttachments();
    
    // set stretch factors for height and width
    GetModelObject()->StretchModel(FLOAT3D(m_fStretch, m_fStretch, m_fStretch));
    ModelChangeNotify();
    
    SetHealth(m_fMaxHealth);
    
    autowait(0.05f);

    m_soSound.Set3DParameters(100.0f, 50.0f, 3.5f, 1.0f);
    m_bActive = TRUE;
    m_fBurnTreshold = 0.66f*m_fMaxHealth;
    
    StartModelAnim(WALLCHARGER_ANIM_DEFAULT, 0);

    wait()
    {
      on (EBegin) : {
        resume;
      }
      on (EDeath eDeath) : {
        jump Destroyed();
      }
    }

    return;
  }
};
