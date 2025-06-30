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

334
%{
#include "Entities/StdH/StdH.h"
%}

uses "Entities/EnemyFly";

enum RobotFlyingChar {
  0 RFC_KAMIKAZE   "Kamikaze",
  1 RFC_FIGHTER    "Fighter",
};

%{
static EntityInfo eiRobotFlying = {
  EIBT_ROBOT, 100.0f, // mass[kg]
  0.0f, 0.0f, 0.0f,  // source  
  0.0f, 0.0f, 0.0f,  // target  
};

#define FIRE_POS      FLOAT3D(0.0f, 0.0f, 0.0f)
%}


class CRobotFlying : CEnemyFly {
name      "RobotFlying";
thumbnail "Thumbnails\\RobotFlying.tbn";

properties:
  1 enum RobotFlyingChar m_rfcChar   "Character" 'C' = RFC_FIGHTER,
components:
  0 class   CLASS_BASE        "Classes\\EnemyFly.ecl",

 10 model   MODEL_KAMIKAZE      "Models\\Enemies\\Robots\\FloatKamikaze\\FloatKamikaze.mdl",
 11 texture TEXTURE_KAMIKAZE    "Models\\Enemies\\Robots\\SentryBall\\Ball.tex",

 12 model   MODEL_FIGHTER       "Models\\Enemies\\Robots\\FlyingFighter\\Ship.mdl",
 13 texture TEXTURE_FIGHTER     "Models\\Enemies\\Robots\\FlyingFighter\\Ship.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE      "Models\\Enemies\\Woman\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT     "Models\\Enemies\\Woman\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND     "Models\\Enemies\\Woman\\Sounds\\Wound.wav",
 53 sound   SOUND_FIRE      "Models\\Enemies\\Woman\\Sounds\\Fire.wav",
 54 sound   SOUND_KICK      "Models\\Enemies\\Woman\\Sounds\\Kick.wav",
 55 sound   SOUND_DEATH     "Models\\Enemies\\Woman\\Sounds\\Death.wav",

functions:
  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiRobotFlying;
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    if (!IsOfSameClass(penInflictor, this)) {
      CEnemyFly::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };

  // virtual anim functions
  void StandingAnim(void) {
//    StartModelAnim(WALKER_ANIM_STAND03, AOF_LOOPING|AOF_NORESTART);
  };
  void WalkingAnim(void) {
//    StartModelAnim(WALKER_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
  };
  void RunningAnim(void) {
//    StartModelAnim(WALKER_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
  };
  void RotatingAnim(void) {
//    StartModelAnim(WALKER_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
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
  FlyHit(EVoid) : CEnemyFly::FlyHit {
    if (m_rfcChar==RFC_FIGHTER) {
      jump FlyFire();
//      m_fShootTime = _pTimer->CurrentTick() + 1.0f;
//      return EReturn();
    }

    // when close enough
    if (CalcDist(m_penEnemy) <= 3.0f) {
      // explode
      SetHealth(-45.0f);
      ReceiveDamage(NULL, DMT_EXPLOSION, 10.0f, FLOAT3D(0,0,0), FLOAT3D(0,1,0));
      InflictRangeDamage(this, DMT_EXPLOSION, 20.0f, GetPlacement().pl_PositionVector, 
        2.75f, 8.0f);
    }

    // run to enemy
    m_fShootTime = _pTimer->CurrentTick() + 0.1f;
    return EReturn();
  };

  FlyFire(EVoid) : CEnemyFly::FlyFire {
    if (m_rfcChar==RFC_KAMIKAZE) {
      m_fShootTime = _pTimer->CurrentTick() + 1.0f;
      return EReturn();
    }

    ShootProjectile(PRT_CYBORG_LASER, FIRE_POS, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);

    return EReturn();
  };

  Death(EVoid) : CEnemyBase::Death {
    // stop moving
    StopMoving();
    DeathSound();     // death sound

    // set physic flags
    SetPhysicsFlags(EPF_MODEL_CORPSE);
    SetCollisionFlags(ECF_CORPSE);
    // robots don't have different collision boxes, so no need to change
    // robots don't have death animations
    return EEnd();
  };

/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // must always be fly-only
    m_EeftType = EFT_FLY_ONLY;

    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    SetHealth(20.0f);
    m_fMaxHealth = 20.0f;
    en_fDensity = 13000.0f;
    // damage/explode properties
    m_fBlowUpAmount = 0.0f;
    m_fBodyParts = 4;
    m_bRobotBlowup = TRUE;
    m_fDamageWounded = 100000.0f;

    // set your appearance
    switch (m_rfcChar) {
    case RFC_KAMIKAZE: {
      SetModel(MODEL_KAMIKAZE);
      SetModelMainTexture(TEXTURE_KAMIKAZE);
      // fly moving properties
      m_fFlyWalkSpeed = FRnd()/2 + 1.0f;
      m_aFlyWalkRotateSpeed = FRnd()*10.0f + 25.0f;
      m_fFlyAttackRunSpeed = FRnd()*2.0f + 8.0f;
      m_aFlyAttackRotateSpeed = FRnd()*25 + 150.0f;
      m_fFlyCloseRunSpeed = FRnd()*2.0f + 6.0f;
      m_aFlyCloseRotateSpeed = FRnd()*50 + 500.0f;
      // attack properties - CAN BE SET
      m_fFlyAttackDistance = 50.0f;
      m_fFlyCloseDistance = 12.5f;
      m_fFlyStopDistance = 0.0f;
      m_fFlyAttackFireTime = 2.0f;
      m_fFlyCloseFireTime = 0.1f;
      m_fFlyIgnoreRange = 200.0f;
      m_fFlyHeight = 1.0f;
      m_iScore = 1000;
          } break;
    case RFC_FIGHTER: {
      SetModel(MODEL_FIGHTER);
      SetModelMainTexture(TEXTURE_FIGHTER);
      // fly moving properties
      m_fFlyWalkSpeed = FRnd()/2 + 1.0f;
      m_aFlyWalkRotateSpeed = FRnd()*10.0f + 25.0f;
      m_fFlyAttackRunSpeed = FRnd()*2.0f + 7.0f;
      m_aFlyAttackRotateSpeed = FRnd()*25 + 150.0f;
      m_fFlyCloseRunSpeed = FRnd()*2.0f + 20.0f;
      m_aFlyCloseRotateSpeed = 150.0f;//FRnd()*50 + 0.0f;
      // attack properties - CAN BE SET
      m_fFlyAttackDistance = 50.0f;
      m_fFlyCloseDistance = 10.0f;
      m_fFlyStopDistance = 0.1f;
      m_fFlyAttackFireTime = 3.0f;
      m_fFlyCloseFireTime = 0.2f;
      m_fFlyIgnoreRange = 200.0f;
      m_fFlyHeight = 2.5f;
          } break;
    default: ASSERT(FALSE);
    }

    // continue behavior in base class
    jump CEnemyFly::MainLoop();
  };
};
