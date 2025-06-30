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

326
%{
#include "Entities/StdH/StdH.h"
#include "Models/Enemies/MAMUTMAN/Mamutman.h"
%}

uses "Entities/EnemyBase";
uses "Entities/Bullet";

%{
// info structure
static EntityInfo eiMamutman = {
  EIBT_FLESH, 60.0f,
  0.0f, 2.0f, 0.0f,
  0.0f, 1.5f, 0.0f,
};

#define FIRE    FLOAT3D(0.0f, 0.8f, 0.0f)

#define FRONT 0
#define MIDDLE 1
#define REAR 2
%}


class CMamutman : CEnemyBase {
name      "Mamutman";
thumbnail "Thumbnails\\Mamutman.tbn";

properties:
  1 CEntityPointer m_penBullet,     // bullet
  2 BOOL m_bSpawned = FALSE,
  3 INDEX m_bSpawnedPosition = 0,

components:
  0 class   CLASS_BASE        "Classes\\EnemyBase.ecl",
  1 class   CLASS_BULLET      "Classes\\Bullet.ecl",

  5 model   MODEL_MAMUTMAN    "Models\\Enemies\\Mamutman\\Mamutman.mdl",
  6 texture TEXTURE_MAMUTMAN  "Models\\Enemies\\Mamutman\\Mamutman.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE      "Models\\Enemies\\Mamutman\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT     "Models\\Enemies\\Mamutman\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND     "Models\\Enemies\\Mamutman\\Sounds\\Wound.wav",
 53 sound   SOUND_DEATH     "Models\\Enemies\\Mamutman\\Sounds\\Death.wav",
 54 sound   SOUND_FIRE      "Models\\Weapons\\Colt\\Sounds\\Fire.wav",

functions:
  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiMamutman;
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // mamutman and mamut can't harm mamutman
    if (!IsOfClass(penInflictor, "Mamutman") && !IsOfClass(penInflictor, "Mamut")) {
      CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };


  // death
  INDEX AnimForDeath(void) {
    INDEX iAnim;
    switch (IRnd()%2) {
      case 0: iAnim = MAMUTMAN_ANIM_DEATH01; break;
      case 1: iAnim = MAMUTMAN_ANIM_DEATH02; break;
      default: iAnim = MAMUTMAN_ANIM_DEATH01; //ASSERTALWAYS("Mamutman unknown death");
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  void DeathNotify(void) {
    ChangeCollisionBoxIndexWhenPossible(MAMUTMAN_COLLISION_BOX_DEATH);
    en_fDensity = 500.0f;
  };

  // virtual anim functions
  void StandingAnim(void) {
    StartModelAnim(MAMUTMAN_ANIM_STAND, AOF_LOOPING|AOF_NORESTART);
  };
  void WalkingAnim(void) {
    StartModelAnim(MAMUTMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
  };
  void RunningAnim(void) {
    StartModelAnim(MAMUTMAN_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
  };
  void RotatingAnim(void) {
    StartModelAnim(MAMUTMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
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


/************************************************************
 *                       FIRE BULLET                        *
 ************************************************************/
  void PrepareBullet(void) {
    // bullet start position
    CPlacement3D plBullet;
    plBullet.pl_OrientationAngle = ANGLE3D(0,0,0);
    plBullet.pl_PositionVector = FIRE;
    plBullet.RelativeToAbsolute(GetPlacement());
    // create bullet
    m_penBullet = CreateEntity(plBullet, CLASS_BULLET);
    // init bullet
    EBulletInit eInit;
    eInit.penOwner = this;
    eInit.fDamage = 1.0f;
    m_penBullet->Initialize(eInit);
    ((CBullet&)*m_penBullet).CalcTarget(m_penEnemy, 100);
  };

  // fire bullet
  void FireBullet(void) {
    ((CBullet&)*m_penBullet).LaunchBullet(TRUE, TRUE, TRUE);
    ((CBullet&)*m_penBullet).DestroyBullet();
  };



procedures:
/************************************************************
 *                    CLASS INTERNAL                        *
 ************************************************************/
  FallOnFloor(EVoid) {
    // drop to floor
    m_bSpawned = FALSE;
    switch (m_bSpawnedPosition) {
      case FRONT:
        StartModelAnim(MAMUTMAN_ANIM_FALLFROMMAMUTFIRST, 0);
        GiveImpulseTranslationRelative(FLOAT3D(FRnd()*4+8.0f, FRnd()+10.0f, FRnd()+2.0f));
        break;
      case MIDDLE:
        StartModelAnim(MAMUTMAN_ANIM_FALLFROMMAMUTSECOND, 0);
        GiveImpulseTranslationRelative(FLOAT3D(-FRnd()*4-8.0f, FRnd()+10.0f, FRnd()+2.0f));
        break;
      case REAR:
        StartModelAnim(MAMUTMAN_ANIM_FALLFROMMAMUTTHIRD, 0);
        GiveImpulseTranslationRelative(FLOAT3D(0.0f, FRnd()+10.0f, FRnd()*2+2.0f));
        break;
    }
    // wait to touch brush or time limit
    wait (10.0f) {
      on (EBegin) : { resume; }
      // brush touched
      on (ETouch et) : {
        if (et.penOther->GetRenderType()&RT_BRUSH) {
          StopMoving();
          stop;
        }
        resume;
      }
      // death
      on (EDeath) : {
        StopMoving();
        SendEvent(EDeath());
        jump CEnemyBase::MainLoop();
      }
      on (ETimer) : { stop; }
    }

    // get up
    StopMoving();
    StartModelAnim(MAMUTMAN_ANIM_GETUP01, 0);
    wait (GetModelObject()->GetAnimLength(MAMUTMAN_ANIM_GETUP01)) {
      on (EBegin) : { resume; }
      on (ETimer) : { stop; }
      // death
      on (EDeath) : {
        StopMoving();
        SendEvent(EDeath());
        jump CEnemyBase::MainLoop();
      }
    }
    return EReturn();
  };



/************************************************************
 *                PROCEDURES WHEN HARMED                    *
 ************************************************************/
  // Play wound animation and falling body part
  BeWounded(EDamage eDamage) : CEnemyBase::BeWounded
  { 
    StopMoving();
    // determine damage anim and play the wounding
    if (IRnd()&1) {
      StartModelAnim(MAMUTMAN_ANIM_FALL01, 0);
      autowait(GetModelObject()->GetAnimLength(MAMUTMAN_ANIM_FALL01));
      autowait(0.2f + FRnd()/3);
      StartModelAnim(MAMUTMAN_ANIM_GETUP01, 0);
      autowait(GetModelObject()->GetAnimLength(MAMUTMAN_ANIM_GETUP01));

    } else if (TRUE) {
      StartModelAnim(MAMUTMAN_ANIM_FALL02, 0);
      autowait(GetModelObject()->GetAnimLength(MAMUTMAN_ANIM_FALL02));
      autowait(0.2f + FRnd()/3);
      StartModelAnim(MAMUTMAN_ANIM_GETUP02, 0);
      autowait(GetModelObject()->GetAnimLength(MAMUTMAN_ANIM_GETUP02));
    }
    return EReturn();
  };



/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  Fire(EVoid) : CEnemyBase::Fire {
    // wait for a while
    StandingAnim();
    PrepareBullet();
    m_fLockOnEnemyTime = 0.2f + FRnd()/8;
    autocall CEnemyBase::LockOnEnemy() EReturn;

    // fire bullet
    StartModelAnim(MAMUTMAN_ANIM_ATTACK02, 0);
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    FireBullet();
    autowait(FRnd()/3 + 0.6f);

    return EReturn();
  };



/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING|EPF_HASLUNGS);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    SetHealth(40.0f);
    m_fMaxHealth = 40.0f;
    en_tmMaxHoldBreath = 5.0f;
    en_fDensity = 1000.0f;

    // set your appearance
    SetModel(MODEL_MAMUTMAN);
    SetModelMainTexture(TEXTURE_MAMUTMAN);
    if (!m_bSpawned) {
      StandingAnim();
    }
    // setup moving speed
    m_fWalkSpeed = FRnd() + 1.5f;
    m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 25.0f);
    m_fAttackRunSpeed = FRnd()*2.0f + 10.0f;
    m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
    m_fCloseRunSpeed = FRnd()*2.0f + 10.0f;
    m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
    // setup attack distances
    m_fAttackDistance = 60.0f;
    m_fCloseDistance = 0.0f;
    m_fStopDistance = 17.0f;
    m_fAttackFireTime = 1.0f;
    m_fCloseFireTime = 1.0f;
    m_fIgnoreRange = 200.0f;
    // damage/explode properties
    m_fBlowUpAmount = 20.0f;
    m_fBodyParts = 4;
    m_fDamageWounded = 30.0f;
    m_iScore = 500;

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
