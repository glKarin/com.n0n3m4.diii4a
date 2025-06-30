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

333
%{
#include "Entities/StdH/StdH.h"
%}

uses "Entities/EnemyBase";
uses "Entities/Projectile";

enum RobotDrivingChar {
  0 RDC_R2D2      "R2D2",
  1 RDC_SPIDER    "Spider",
};

%{
// info structure
static EntityInfo eiRobotDriving = {
  EIBT_ROBOT, 100.0f, // mass[kg]
  0.0f, 1.5f, 0.0f,   // source
  0.0f, 1.5f, 0.0f,   // target
};

#define FIRE_POS  FLOAT3D(0.0f, 1.0f, -1.0f)
%}


class CRobotDriving : CEnemyBase {
name      "RobotDriving";
thumbnail "Thumbnails\\RobotDriving.tbn";

properties:
  1 enum RobotDrivingChar m_rdcChar   "Character" 'C' = RDC_R2D2,
  2 FLOAT m_fSize = 1.0f,

  10 CSoundObject m_soFire0,
  11 CSoundObject m_soFire1,
  
components:
  0 class   CLASS_BASE          "Classes\\EnemyBase.ecl",
  1 class   CLASS_PROJECTILE    "Classes\\Projectile.ecl",

 10 model   MODEL_R2D2          "Models\\Enemies\\Robots\\DrivingWheel\\Robot.mdl",
 11 texture TEXTURE_R2D2        "Models\\Enemies\\Robots\\DrivingWheel\\Robot.tex",

 12 model   MODEL_SPIDER        "Models\\Enemies\\Robots\\DrivingSpider\\DrivingSpider.mdl",
 13 texture TEXTURE_SPIDER      "Models\\Enemies\\Robots\\SentryBall\\Ball.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE        "Models\\Enemies\\Walker\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT       "Models\\Enemies\\Walker\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND       "Models\\Enemies\\Walker\\Sounds\\Wound.wav",
 53 sound   SOUND_FIRE_LASER  "Models\\Enemies\\Walker\\Sounds\\FireLaser.wav",
 55 sound   SOUND_KICK        "Models\\Enemies\\Walker\\Sounds\\Kick.wav",
 56 sound   SOUND_DEATH       "Models\\Enemies\\Walker\\Sounds\\Death.wav",

functions:
  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiRobotDriving;
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // can't harm each other
    if (!IsOfSameClass(penInflictor, this)) {
      CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
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
  Fire(EVoid) : CEnemyBase::Fire {
    // r2d2
    if (m_rdcChar==RDC_R2D2) {
      // to fire
      //StartModelAnim(WALKER_ANIM_TOFIRE, 0);
      m_fLockOnEnemyTime = 0.5f;//GetModelObject()->GetAnimLenght(WALKER_ANIM_TOFIRE);
      autocall CEnemyBase::LockOnEnemy() EReturn;

      StopMoving();
      //StartModelAnim(WALKER_ANIM_ATTACK02LEFT, AOF_LOOPING);

      ShootProjectile(PRT_CYBORG_LASER, FIRE_POS*m_fSize, ANGLE3D(0, 0, 0));
      PlaySound(m_soFire0, SOUND_FIRE_LASER, SOF_3D);
      autowait(0.25f);
      ShootProjectile(PRT_CYBORG_LASER, FIRE_POS*m_fSize, ANGLE3D(0, 0, 0));
      PlaySound(m_soFire1, SOUND_FIRE_LASER, SOF_3D);

      // wait for a while
      StandingAnim();
      autowait(FRnd()*0.1f+0.1f);

    // spider
    } else if (TRUE) {

      // don't shoot if enemy above or below you more than 5 meters
      if (Abs(en_vGravityDir%CalcDelta(m_penEnemy)) > 5.0f) {
        return EEnd();
      }

      m_fLockOnEnemyTime = 0.5f;//GetModelObject()->GetAnimLenght(WALKER_ANIM_TOFIRE);
      autocall CEnemyBase::LockOnEnemy() EReturn;

      // hit bomb
      
      FLOAT fSpeed = Sqrt(en_fGravityA*(CalcDist(m_penEnemy)*1.25f));
      // target enemy body
      EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
      FLOAT3D vShootTarget;
      GetEntityInfoPosition(m_penEnemy, peiTarget->vTargetCenter, vShootTarget);
      // launch
      CPlacement3D pl;
      PreparePropelledProjectile(pl, vShootTarget, FLOAT3D(0.0f, 1.5f, -0.7f), ANGLE3D(0, 45.0f, 0));
      CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
      ELaunchProjectile eLaunch;
      eLaunch.penLauncher = this;
      eLaunch.prtType = PRT_HEADMAN_BOMBERMAN;
      eLaunch.fSpeed = fSpeed;
      penProjectile->Initialize(eLaunch);

      //StartModelAnim(WALKER_ANIM_ATTACK02LEFT, AOF_LOOPING);
      PlaySound(m_soFire0, SOUND_FIRE_LASER, SOF_3D);
      StopMoving();
      // wait for a while
      StandingAnim();
      autowait(FRnd()*0.1f+0.1f);
    }

    return EReturn();
  };



/************************************************************
 *                    D  E  A  T  H                         *
 ************************************************************/
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
    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);

    SetHealth(20.0f);
    m_fMaxHealth = 20.0f;
    m_fBlowUpAmount = 0.0f;
    m_fBodyParts = 4;
    m_bRobotBlowup = TRUE;
    m_fDamageWounded = 100000.0f;

    en_fDensity = 10000.0f;

    switch (m_rdcChar) {
    case RDC_R2D2: {
      // set your appearance
      m_fSize = 1.0f;
      SetComponents(this, *GetModelObject(), 
        MODEL_R2D2, TEXTURE_R2D2, 0,0,0);
      GetModelObject()->StretchModel(FLOAT3D(1,1,1));
      ModelChangeNotify();

      // setup moving speed
      m_fWalkSpeed = FRnd() + 1.5f;
      m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 500.0f);
      m_fAttackRunSpeed = FRnd() + 5.0f;
      m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
      m_fCloseRunSpeed = FRnd() + 5.0f;
      m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
      // setup attack distances
      m_fAttackDistance = 50.0f;
      m_fCloseDistance = 0.0f;
      m_fStopDistance = 8.0f;
      m_fAttackFireTime = 2.0f;
      m_fCloseFireTime = 1.0f;
      m_fIgnoreRange = 200.0f;

      m_iScore = 100;
                   } break;
    case RDC_SPIDER: {
      // set your appearance
      m_fSize = 1.0f;
      SetComponents(this, *GetModelObject(), 
        MODEL_SPIDER, TEXTURE_SPIDER, 0,0,0);
      GetModelObject()->StretchModel(FLOAT3D(1,1,1));
      ModelChangeNotify();

      // setup moving speed
      m_fWalkSpeed = FRnd() + 1.5f;
      m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 500.0f);
      m_fAttackRunSpeed = FRnd() + 4.0f;
      m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
      m_fCloseRunSpeed = FRnd() + 4.0f;
      m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
      // setup attack distances
      m_fAttackDistance = 45.0f;
      m_fCloseDistance = 0.0f;
      m_fStopDistance = 20.0f;
      m_fAttackFireTime = 2.0f;
      m_fCloseFireTime = 1.5f;
      m_fIgnoreRange = 150.0f;

      m_iScore = 500;
          } break;
    default: ASSERT(FALSE);
    }
    StandingAnim();
    // set sound default parameters
    m_soFire0.Set3DParameters(160.0f, 5.0f, 1.0f, 1.0f);
    m_soFire1.Set3DParameters(160.0f, 5.0f, 1.0f, 1.0f);

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
