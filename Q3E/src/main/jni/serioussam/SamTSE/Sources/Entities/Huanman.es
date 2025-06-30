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

325
%{
#include "Entities/StdH/StdH.h"
#include "Models/Enemies/HuanMan/Huanman.h"
%}

uses "Entities/EnemyBase";

%{
// info structure
static EntityInfo eiHuanman = {
  EIBT_FLESH, 125.0f,
  0.0f, 2.1f, 0.0f,
  0.0f, 1.1f, 0.0f,
};

#define CLOSE_HIT 2.0f
#define FIRE    FLOAT3D( 0.45f, 2.0f, -1.25f)
%}


class CHuanman : CEnemyBase {
name      "Huanman";
thumbnail "Thumbnails\\Huanman.tbn";

properties:
components:
  0 class   CLASS_BASE          "Classes\\EnemyBase.ecl",

 10 model   MODEL_HUANMAN       "Models\\Enemies\\Huanman\\Huanman.mdl",
 11 texture TEXTURE_HUANMAN     "Models\\Enemies\\Huanman\\Huanman.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE      "Models\\Enemies\\Huanman\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT     "Models\\Enemies\\Huanman\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND     "Models\\Enemies\\Huanman\\Sounds\\Wound.wav",
 53 sound   SOUND_FIRE      "Models\\Enemies\\Huanman\\Sounds\\Fire.wav",
 54 sound   SOUND_KICK      "Models\\Enemies\\Huanman\\Sounds\\Kick.wav",
 55 sound   SOUND_DEATH     "Models\\Enemies\\Huanman\\Sounds\\Death.wav",

functions:
  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiHuanman;
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // huanman can't harm huanman
    if (!IsOfClass(penInflictor, "Huanman")) {
      CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };


  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    StartModelAnim(HUANMAN_ANIM_WOUND, 0);
    return HUANMAN_ANIM_WOUND;
  };

  // death
  INDEX AnimForDeath(void) {
    StartModelAnim(HUANMAN_ANIM_DEATH, 0);
    return HUANMAN_ANIM_DEATH;
  };

  void DeathNotify(void) {
    ChangeCollisionBoxIndexWhenPossible(HUANMAN_COLLISION_BOX_DEATH);
    en_fDensity = 500.0f;
  };

  // virtual anim functions
  void StandingAnim(void) {
    StartModelAnim(HUANMAN_ANIM_STAND, AOF_LOOPING|AOF_NORESTART);
  };
  void WalkingAnim(void) {
    StartModelAnim(HUANMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
  };
  void RunningAnim(void) {
    StartModelAnim(HUANMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
  };
  void RotatingAnim(void) {
    StartModelAnim(HUANMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
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
  Fire(EVoid) : CEnemyBase::Fire {
    // fire projectile
    StartModelAnim(HUANMAN_ANIM_ATTACK, 0);
    autowait(0.4f);
    ShootProjectile(PRT_HUANMAN_FIRE, FIRE, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.6f);
    StandingAnim();
    autowait(FRnd()/2 + _pTimer->TickQuantum);

    return EReturn();
  };

  Hit(EVoid) : CEnemyBase::Hit {
    // attack with spear
    StartModelAnim(HUANMAN_ANIM_ATTACK, 0);
    autowait(0.4f);
    if (CalcDist(m_penEnemy) < CLOSE_HIT) {
      // damage enemy
      FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
      vDirection.Normalize();
      InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 7.5f, FLOAT3D(0, 0, 0), vDirection);
    }
    PlaySound(m_soSound, SOUND_KICK, SOF_3D);
    autowait(0.6f);
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
    SetPhysicsFlags(EPF_MODEL_WALKING);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    SetHealth(100.0f);
    m_fMaxHealth = 100.0f;
    en_fDensity = 1100.0f;

    // set your appearance
    SetModel(MODEL_HUANMAN);
    SetModelMainTexture(TEXTURE_HUANMAN);
    StandingAnim();
    // setup moving speed
    m_fWalkSpeed = FRnd() + 3.0f;
    m_aWalkRotateSpeed = AngleDeg(FRnd()*20.0f + 50.0f);
    m_fAttackRunSpeed = FRnd() + 5.0f;
    m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 600.0f);
    m_fCloseRunSpeed = FRnd() + 5.0f;
    m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 600.0f);
    // setup attack distances
    m_fAttackDistance = 50.0f;
    m_fCloseDistance = 2.0f;
    m_fStopDistance = 1.7f;
    m_fAttackFireTime = 3.0f;
    m_fCloseFireTime = 3.0f;
    m_fIgnoreRange = 200.0f;
    // damage/explode properties
    m_fBlowUpAmount = 80.0f;
    m_fBodyParts = 4;
    m_fDamageWounded = 20.0f;
    m_iScore = 500;

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
