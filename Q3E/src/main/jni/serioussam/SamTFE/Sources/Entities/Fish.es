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

337
%{
#include "Entities/StdH/StdH.h"
#include "Models/Enemies/Fish/Fish.h"
%}

uses "Entities/EnemyDive";

%{
static EntityInfo eiFish = {
  EIBT_FLESH, 100.0f,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
};

#define DISTANCE_ELECTRICITY   8.0f
%}

class CFish : CEnemyDive {
name      "Fish";
thumbnail "Thumbnails\\Fish.tbn";

properties:
  0 BOOL m_bAttackingByElectricity = FALSE,
  1 FLOAT m_tmElectricityTimeStart = 0.0f,

components:
  0 class   CLASS_BASE        "Classes\\EnemyDive.ecl",
  1 model   MODEL_FISH        "Models\\Enemies\\Fish\\Fish.mdl",
  2 texture TEXTURE_FISH      "Models\\Enemies\\Fish\\Fish1.tex",
  3 model   MODEL_GLOW        "Models\\Enemies\\Fish\\Glow.mdl",
  4 texture TEXTURE_GLOW      "Models\\Enemies\\Fish\\Glow.tex",
  5 texture TEXTURE_SPECULAR  "Models\\SpecularTextures\\Medium.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE    "Models\\Enemies\\Fish\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT   "Models\\Enemies\\Fish\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND   "Models\\Enemies\\Fish\\Sounds\\Wound.wav",
 53 sound   SOUND_DEATH   "Models\\Enemies\\Fish\\Sounds\\Death.wav",
 54 sound   SOUND_ATTACK  "Models\\Enemies\\Fish\\Sounds\\Attack.wav",
 55 sound   SOUND_WOUNDAIR "Models\\Enemies\\Fish\\Sounds\\WoundAir.wav",
 56 sound   SOUND_DEATHAIR "Models\\Enemies\\Fish\\Sounds\\DeathAir.wav",

functions:
  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANS("%s was electrocuted by a fish"),(const char*) strPlayerName);
    return str;
  }
  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnm, "Data\\Messages\\Enemies\\Fish.txt");
    return fnm;
  };
  void Precache(void) {
    CEnemyBase::Precache();
    PrecacheModel(MODEL_GLOW  );
    PrecacheTexture(TEXTURE_GLOW);
    PrecacheSound(SOUND_IDLE);
    PrecacheSound(SOUND_SIGHT);
    PrecacheSound(SOUND_WOUND);
    PrecacheSound(SOUND_DEATH);
    PrecacheSound(SOUND_WOUNDAIR);
    PrecacheSound(SOUND_DEATHAIR);
    PrecacheSound(SOUND_ATTACK);
  };

  /* Entity info */
  void *GetEntityInfo(void)
  {
    return &eiFish;
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    if (dmtType==DMT_DROWNING) {
      //en_tmMaxHoldBreath = -5.0f;
      fDamageAmmount/=2.0f;
    }
    // fish can't harm fish
    if (!IsOfClass(penInflictor, "Fish")) {
      CEnemyDive::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };


  // damage anim
  INDEX AnimForDamage(FLOAT fDamage)
  {
    m_bAttackingByElectricity = FALSE;
    INDEX iAnim = FISH_ANIM_WOUND;
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    if (!m_bInLiquid) {
      return AnimForDamage(10.0f);
    }
    INDEX iAnim;
    switch (IRnd()%3) {
      default: iAnim = FISH_ANIM_DEATH; break;
      case 0: iAnim = FISH_ANIM_DEATH; break;
      case 1: iAnim = FISH_ANIM_DEATH02; break;
      case 2: iAnim = FISH_ANIM_DEATH03; break;
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  void DeathNotify(void)
  {
    m_bAttackingByElectricity = FALSE;
    en_fDensity = 500.0f;
  };

  void RenderParticles(void)
  {
    if( m_bAttackingByElectricity && m_penEnemy!=NULL)
    {
      // render one lightning toward enemy
      FLOAT3D vSource = GetPlacement().pl_PositionVector;
      FLOAT3D vTarget = m_penEnemy->GetPlacement().pl_PositionVector;
      //FLOAT3D vDirection = (vTarget-vSource).Normalize();
      Particles_Ghostbuster(vSource, vTarget, 32, 1.0f);

      // random lightnings arround
      for( INDEX i=0; i<4; i++)
      {
        FLOAT3D vDirection = vSource;
        vDirection(1) += ((FLOAT(rand())/(float)(RAND_MAX))-0.5f) * DISTANCE_ELECTRICITY/1.0f;
        vDirection(2) += ((FLOAT(rand())/(float)(RAND_MAX))-0.5f) * DISTANCE_ELECTRICITY/1.0f;
        vDirection(3) += ((FLOAT(rand())/(float)(RAND_MAX))-0.5f) * DISTANCE_ELECTRICITY/1.0f;
        Particles_Ghostbuster(vSource, vDirection, 32, 1.0f);
      }
    }
    CEnemyBase::RenderParticles();
  }
  // virtual anim functions
  void StandingAnim(void)
  {
    StartModelAnim(FISH_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);
  };
  void WalkingAnim(void)
  {
    if (m_bInLiquid) {
      StartModelAnim(FISH_ANIM_SWIM, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(FISH_ANIM_WOUND, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RunningAnim(void)
  {
    WalkingAnim();
  };
  void RotatingAnim(void)
  {
    WalkingAnim();
  };

  // virtual sound functions
  void IdleSound(void)
  {
    PlaySound(m_soSound, SOUND_IDLE, SOF_3D|SOF_NOFILTER);
  };
  void SightSound(void)
  {
    PlaySound(m_soSound, SOUND_SIGHT, SOF_3D|SOF_NOFILTER);
  };
  void WoundSound(void)
  {
    if (m_bInLiquid) {
      PlaySound(m_soSound, SOUND_WOUND, SOF_3D|SOF_NOFILTER);
    } else {
      PlaySound(m_soSound, SOUND_WOUNDAIR, SOF_3D|SOF_NOFILTER);
    }
  };
  void DeathSound(void)
  {
    if (m_bInLiquid) {
      PlaySound(m_soSound, SOUND_DEATH, SOF_3D|SOF_NOFILTER);
    } else {
      PlaySound(m_soSound, SOUND_DEATHAIR, SOF_3D|SOF_NOFILTER);
    }
  };

  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    FLOAT fTimePassed = _pTimer->GetLerpedCurrentTick()-m_tmElectricityTimeStart;
    if( m_bAttackingByElectricity && (fTimePassed>0))
    {
      FLOAT fDieFactor = 1.0f;
      if( fTimePassed > 0.25f)
      {
        // calculate light dying factor
        fDieFactor = 1.0-(ClampUp(fTimePassed-0.25f,0.5f)/0.5f);
      }
      // adjust light fx
      FLOAT fR = 0.7f+0.1f*(FLOAT(rand())/(float)(RAND_MAX));
      FLOAT fG = 0.7f+0.2f*(FLOAT(rand())/(float)(RAND_MAX));
      FLOAT fB = 0.7f+0.3f*(FLOAT(rand())/(float)(RAND_MAX));
      colAmbient = RGBToColor( fR*128*fDieFactor, fG*128*fDieFactor, fB*128*fDieFactor);
      colLight = C_WHITE;
      return CEnemyBase::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
    }
    return CEnemyBase::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
  };

procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  DiveHit(EVoid) : CEnemyDive::DiveHit
  {
    if (CalcDist(m_penEnemy) > DISTANCE_ELECTRICITY)
    {
      // swim to enemy
      m_fShootTime = _pTimer->CurrentTick() + 0.25f;
      return EReturn();
    }

    // wait to allow enemy to go aoutside bite range
    autowait(0.6f);

    m_bAttackingByElectricity = TRUE;
    m_tmElectricityTimeStart = _pTimer->CurrentTick();

    AddAttachmentToModel(this, *GetModelObject(), FISH_ATTACHMENT_GLOW, MODEL_GLOW, TEXTURE_GLOW, 0, 0, 0);
    CModelObject &moGlow = GetModelObject()->GetAttachmentModel(FISH_ATTACHMENT_GLOW)->amo_moModelObject;
    moGlow.StretchModel(FLOAT3D(4.0f, 4.0f, 4.0f));

    // bite
    StartModelAnim(FISH_ANIM_ATTACK, 0);
    PlaySound(m_soSound, SOUND_ATTACK, SOF_3D|SOF_NOFILTER);
    if (CalcDist(m_penEnemy)<DISTANCE_ELECTRICITY)
    {
      // damage enemy
      InflictRangeDamage(this, DMT_CLOSERANGE, 15.0f, GetPlacement().pl_PositionVector, 1.0f, DISTANCE_ELECTRICITY);
      // push target away
      FLOAT3D vSpeed;
      GetHeadingDirection(0.0f, vSpeed);
      vSpeed = vSpeed * 30.0f;
      KickEntity(m_penEnemy, vSpeed);
    }
    autowait(0.5f);

    m_bAttackingByElectricity = FALSE;
    GetModelObject()->RemoveAttachmentModel(FISH_ATTACHMENT_GLOW);

    StandingAnim();
    autowait(0.2f + FRnd()/3);
    return EReturn();
  };

  Hit(EVoid) : CEnemyBase::Hit
  {
    jump DiveHit();
  }

/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();
    // fish must not go upstairs, or it will get out of water
    SetPhysicsFlags(((EPF_MODEL_WALKING|EPF_HASGILLS)&~EPF_ONBLOCK_CLIMBORSLIDE)|EPF_ONBLOCK_SLIDE);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    SetHealth(30.0f);
    m_fMaxHealth = 30.0f;
    en_tmMaxHoldBreath = 15.0f;
    en_fDensity = 1000.0f;
    m_EedtType = EDT_GROUND_DIVE;

    // set your appearance
    SetModel(MODEL_FISH);
    SetModelMainTexture(TEXTURE_FISH);
    SetModelSpecularTexture(TEXTURE_SPECULAR);
    // rotation speeds
    m_fDiveWalkSpeed = 15.0f;
    m_fDiveAttackRunSpeed = 20.0f;
    m_fDiveCloseRunSpeed = 25.0f;

    m_fWalkSpeed = 15.0f;
    m_aWalkRotateSpeed = 900.0f;
    m_fAttackRunSpeed = 10.0f;
    m_fCloseRunSpeed = 12.0f;
    // translation speeds
    m_aDiveWalkRotateSpeed = 360.0f;
    m_aDiveAttackRotateSpeed = 1800.0f;
    m_aDiveCloseRotateSpeed = 3600.0f;
    // distances
    m_fDiveIgnoreRange = 200.0f;
    m_fDiveAttackDistance = 100.0f;
    m_fDiveCloseDistance = 15.0f;
    m_fDiveStopDistance = 2.0f;
    // frequencies
    m_fDiveAttackFireTime = 0.0f;
    m_fDiveCloseFireTime = 0.0f;
    // damage/explode properties
    m_fBlowUpAmount = 80.0f;
    m_fBodyParts = 2;
    m_fDamageWounded = 1.0f;
    m_iScore = 500;

    {
      // translation speeds
      m_aWalkRotateSpeed = 360.0f;
      m_aAttackRotateSpeed = 1800.0f;
      m_aCloseRotateSpeed = 3600.0f;
      // distances
      m_fIgnoreRange = 200.0f;
      m_fAttackDistance = 100.0f;
      m_fCloseDistance = 15.0f;
      m_fStopDistance = 2.0f;
      // frequencies
      m_fAttackFireTime = 0.0f;
      m_fCloseFireTime = 0.0f;
    }
    // set stretch factors for height and width
    GetModelObject()->StretchModel(FLOAT3D(1.0f, 1.0f, 1.0f));
    ModelChangeNotify();

    en_fAcceleration = 200.0f;
    en_fDeceleration = 200.0f;

    // continue behavior in base class
    jump CEnemyDive::MainLoop();
  };
};
