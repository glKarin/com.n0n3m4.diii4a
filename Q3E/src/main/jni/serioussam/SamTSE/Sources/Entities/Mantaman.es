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

329
%{
#include "Entities/StdH/StdH.h"
#include "Models/Enemies/MANTAMAN/mantaman.h"
%}

uses "Entities/EnemyDive";

%{
static EntityInfo eiMantamanLiquid = {
  EIBT_FLESH, 150.0f,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,

#define FIRE_WATER    FLOAT3D(0.0f, 0.5f, -1.25f)
};
%}


class CMantaman : CEnemyDive {
name      "Mantaman";
thumbnail "Thumbnails\\Mantaman.tbn";

properties:
  1 BOOL m_FixedState         "Fixed state" 'X' = FALSE,      // fixed state on beginning

components:
  0 class   CLASS_BASE        "Classes\\EnemyDive.ecl",
  1 model   MODEL_MANTAMAN    "Models\\Enemies\\Mantaman\\Mantaman.mdl",
  2 texture TEXTURE_MANTAMAN  "Models\\Enemies\\Mantaman\\Mantaman.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE      "Models\\Enemies\\Mantaman\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT     "Models\\Enemies\\Mantaman\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND     "Models\\Enemies\\Mantaman\\Sounds\\Wound.wav",
 53 sound   SOUND_FIRE      "Models\\Enemies\\Mantaman\\Sounds\\Fire.wav",
 54 sound   SOUND_KICK      "Models\\Enemies\\Mantaman\\Sounds\\Kick.wav",
 55 sound   SOUND_DEATH     "Models\\Enemies\\Mantaman\\Sounds\\Death.wav",

functions:
  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiMantamanLiquid;
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // mantaman can't harm mantaman
    if (!IsOfClass(penInflictor, "Mantaman")) {
      CEnemyDive::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };


  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    switch (IRnd()%2) {
      case 0: iAnim = MANTAMAN_ANIM_WOUND01; break;
      case 1: iAnim = MANTAMAN_ANIM_WOUND02; break;
      default: iAnim = MANTAMAN_ANIM_WOUND01; //ASSERTALWAYS("Mantaman unknown liquid damage");
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    StartModelAnim(MANTAMAN_ANIM_DEATH, 0);
    return MANTAMAN_ANIM_DEATH;
  };

  void DeathNotify(void) {
    ChangeCollisionBoxIndexWhenPossible(MANTAMAN_COLLISION_BOX_DEATH);
    en_fDensity = 500.0f;
  };

  // virtual anim functions
  void StandingAnim(void) {
    if (m_FixedState) {
      StartModelAnim(MANTAMAN_ANIM_DEFAULT_ANIMATION02, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(MANTAMAN_ANIM_STANDORANDSWIMSLOW, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void WalkingAnim(void) {
    StartModelAnim(MANTAMAN_ANIM_STANDORANDSWIMSLOW, AOF_LOOPING|AOF_NORESTART);
  };
  void RunningAnim(void) {
    StartModelAnim(MANTAMAN_ANIM_SWIMFAST, AOF_LOOPING|AOF_NORESTART);
  };
  void RotatingAnim(void) {
    StartModelAnim(MANTAMAN_ANIM_STANDORANDSWIMSLOW, AOF_LOOPING|AOF_NORESTART);
  };
  void ChangeCollisionToLiquid() {
    ChangeCollisionBoxIndexWhenPossible(MANTAMAN_COLLISION_BOX_DEAFULT);
  };
  void ChangeCollisionToGround() {
    ChangeCollisionBoxIndexWhenPossible(MANTAMAN_COLLISION_BOX_DEAFULT);
  };

  // virtual sound functions
  void IdleSound(void) {
    PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
  };
  void SightSound(void) {
    PlaySound(m_soSound, SOUND_SIGHT, SOF_3D);
  };
  void WoundSound(void) {
    PlaySound(m_soSound, SOUND_WOUND, SOF_3D);
  };
  void DeathSound(void) {
    PlaySound(m_soSound, SOUND_DEATH, SOF_3D);
  };


procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  AttackEnemy(EVoid) : CEnemyBase::AttackEnemy {
    if (m_FixedState) {
      m_FixedState = FALSE;
      StartModelAnim(MANTAMAN_ANIM_MORPH, 0);
      wait(GetModelObject()->GetAnimLength(MANTAMAN_ANIM_MORPH)) {
        on (EBegin) : { resume; }
        on (ETimer) : { stop; }
        on (EWatch) : { resume; }
        on (EDamage) : { resume; }
      }
    }
    jump CEnemyBase::AttackEnemy();
  };


  DiveFire(EVoid) : CEnemyDive::DiveFire {
    // fire projectile
    StartModelAnim(MANTAMAN_ANIM_ATTACK01, 0);
    autowait(0.3f);
    ShootProjectile(PRT_MANTAMAN_FIRE, FIRE_WATER, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.8f);
    StandingAnim();
    autowait(FRnd()/2 + _pTimer->TickQuantum);

    return EReturn();
  };



/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING|EPF_HASGILLS);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    SetHealth(50.0f);
    m_fMaxHealth = 50.0f;
    en_tmMaxHoldBreath = 5.0f;
    en_fDensity = 1000.0f;

    // set your appearance
    SetModel(MODEL_MANTAMAN);
    SetModelMainTexture(TEXTURE_MANTAMAN);
    // dive moving properties
    m_fDiveWalkSpeed = FRnd() + 2.0f;
    m_aDiveWalkRotateSpeed = FRnd()*10.0f + 500.0f;
    m_fDiveAttackRunSpeed = FRnd()*4.0f + 14.0f;
    m_aDiveAttackRotateSpeed = FRnd()*25 + 250.0f;
    m_fDiveCloseRunSpeed = FRnd()*2.0f + 6.5f;
    m_aDiveCloseRotateSpeed = FRnd()*50 + 250.0f;
    // attack properties
    m_fDiveAttackDistance = 100.0f;
    m_fDiveCloseDistance = 0.0f;
    m_fDiveStopDistance = 5.0f;
    m_fDiveAttackFireTime = 3.0f;
    m_fDiveCloseFireTime = 2.0f;
    m_fDiveIgnoreRange = 200.0f;
    // damage/explode properties
    m_fBlowUpAmount = 140.0f;
    m_fBodyParts = 4;
    m_fDamageWounded = 30.0f;
    m_iScore = 2000;


    // allowed types
    m_EedtType = EDT_DIVE_ONLY;

    // continue behavior in base class
    jump CEnemyDive::MainLoop();
  };
};
