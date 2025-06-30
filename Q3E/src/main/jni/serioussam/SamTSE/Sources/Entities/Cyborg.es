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

330
%{
#include "Entities/StdH/StdH.h"
#include "Models/Enemies/Cyborg/Cyborg.h"
%}


uses "Entities/EnemyFly";
uses "Entities/Projectile";
uses "Entities/CyborgBike";


enum CyborgType {
  0 CBT_GROUND      "Ground",
  1 CBT_FLY         "Fly",
  2 CBT_FLYGROUND   "Fly-Ground",
};


%{
// info structure
static EntityInfo eiCyborgStand = {
  EIBT_ROBOT, 200.0f,
  0.0f, 1.55f, 0.0f,
  0.0f, 1.0f, 0.0f,
};
static EntityInfo eiCyborgFly = {
  EIBT_ROBOT, 1500.0f,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
};

#define FIRE_BIKE   FLOAT3D(-0.35f, 0.1f, -1.2f)
#define FIRE_LASER  FLOAT3D(-0.5f, 1.7f, -0.75f)

#define IGNORE_RANGE  400.0f

#define BIKE_ATTACHMENT   FLOAT3D(0, 0, -0.271f)
%}


class CCyborg : CEnemyBase {
name      "Cyborg";
thumbnail "Thumbnails\\Cyborg.tbn";

properties:
  1 enum CyborgType m_EctType "Type" 'T' = CBT_GROUND,   // type
  2 INDEX m_iCloseHit = 0,              // close hit hand (left or right)
  3 INDEX m_iFireLaserCount = 0,        // fire laser binary divider
  4 INDEX m_ctBombsToDrop = 0,          // counter of bombs to drop in fly-over
 10 FLOAT m_tmLastBombDropped = -1.0f,  // when last bomb was dropped
  5 FLOAT m_fFlyAboveEnemy = 0.0f,      // fly above enemy height
  6 FLOAT m_fFlySpeed = 0.0f,
  7 FLOAT m_aFlyRotateSpeed = 0.0f,
  8 FLOAT m_fFallStartTime = 0.0f,
  9 BOOL  m_bBombing  "Bombing" 'B' = FALSE,           // enable bombing

{
  CEntity *penBullet;     // bullet
}

components:
  0 class   CLASS_BASE          "Classes\\EnemyFly.ecl",
  1 class   CLASS_BULLET        "Classes\\Bullet.ecl",
  2 class   CLASS_CYBORG_BIKE   "Classes\\CyborgBike.ecl",
  3 class   CLASS_PROJECTILE      "Classes\\Projectile.ecl",
  4 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",

 10 model   MODEL_CYBORG            "Models\\Enemies\\Cyborg\\Cyborg.mdl",
 11 model   MODEL_ASS               "Models\\Enemies\\Cyborg\\AssHole.mdl",
 12 model   MODEL_TORSO             "Models\\Enemies\\Cyborg\\Torso.mdl",
 13 model   MODEL_HEAD              "Models\\Enemies\\Cyborg\\Head.mdl",
 14 model   MODEL_RIGHT_UPPER_ARM   "Models\\Enemies\\Cyborg\\RightUpperArm.mdl",
 15 model   MODEL_RIGHT_LOWER_ARM   "Models\\Enemies\\Cyborg\\RightLowerArm.mdl",
 16 model   MODEL_LEFT_UPPER_ARM    "Models\\Enemies\\Cyborg\\LeftUpperArm.mdl",
 17 model   MODEL_LEFT_LOWER_ARM    "Models\\Enemies\\Cyborg\\LeftLowerArm.mdl",
 18 model   MODEL_RIGHT_UPPER_LEG   "Models\\Enemies\\Cyborg\\RightUpperLeg.mdl",
 19 model   MODEL_RIGHT_LOWER_LEG   "Models\\Enemies\\Cyborg\\RightLowerLeg.mdl",
 20 model   MODEL_LEFT_UPPER_LEG    "Models\\Enemies\\Cyborg\\LeftUpperLeg.mdl",
 21 model   MODEL_LEFT_LOWER_LEG    "Models\\Enemies\\Cyborg\\LeftLowerLeg.mdl",
 22 model   MODEL_FOOT              "Models\\Enemies\\Cyborg\\Foot.mdl",
 23 model   MODEL_BIKE              "Models\\Enemies\\Cyborg\\Bike.mdl",
 30 texture TEXTURE_CYBORG          "Models\\Enemies\\Cyborg\\Cyborg.tex",
 31 texture TEXTURE_BIKE            "Models\\Enemies\\Cyborg\\Bike.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE      "Models\\Enemies\\Cyborg\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT     "Models\\Enemies\\Cyborg\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND     "Models\\Enemies\\Cyborg\\Sounds\\Wound.wav",
 53 sound   SOUND_FIRE      "Models\\Enemies\\Cyborg\\Sounds\\Fire.wav",
 54 sound   SOUND_KICK      "Models\\Enemies\\Cyborg\\Sounds\\Kick.wav",
 55 sound   SOUND_DEATH     "Models\\Enemies\\Cyborg\\Sounds\\Death.wav",

// ************** REFLECTIONS **************
202 texture TEX_REFL_LIGHTMETAL01       "Models\\ReflectionTextures\\LightMetal01.tex",

// ************** SPECULAR **************
211 texture TEX_SPEC_MEDIUM             "Models\\SpecularTextures\\Medium.tex",
212 texture TEX_SPEC_STRONG             "Models\\SpecularTextures\\Strong.tex",

functions:
  /* Entity info */
  void *GetEntityInfo(void) {
    if (m_EctType!=CBT_GROUND) {
      return &eiCyborgFly;
    } else {
      return &eiCyborgStand;
    }
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // cyborg can't harm cyborg
    if (!IsOfClass(penInflictor, "Cyborg")) {
      CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };


  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    switch (IRnd()%4) {
      case 0: iAnim = CYBORG_ANIM_WOUND01; break;
      case 1: iAnim = CYBORG_ANIM_WOUND02; break;
      case 2: iAnim = CYBORG_ANIM_WOUND03; break;
      case 3: iAnim = CYBORG_ANIM_WOUND04; break;
      default: iAnim = CYBORG_ANIM_WOUND01; //ASSERTALWAYS("Cyborg unknown damage");
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    INDEX iAnim;
    switch (IRnd()%2) {
      case 0: iAnim = CYBORG_ANIM_DEATH01; break;
      case 1: iAnim = CYBORG_ANIM_DEATH02; break;
      default: iAnim = CYBORG_ANIM_DEATH01; //ASSERTALWAYS("Cyborg unknown death");
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };


  // virtual anim functions
  void StandingAnim(void) {
    if (m_EctType!=CBT_GROUND) {
      StartModelAnim(CYBORG_ANIM_BIKEREST, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(CYBORG_ANIM_WAIT01, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void WalkingAnim(void) {
    if (m_EctType!=CBT_GROUND) {
      StartModelAnim(CYBORG_ANIM_BIKEREST, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(CYBORG_ANIM_WALK01, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RunningAnim(void) {
    if (m_EctType!=CBT_GROUND) {
      StartModelAnim(CYBORG_ANIM_BIKEREST, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(CYBORG_ANIM_WALK01, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RotatingAnim(void) {
    if (m_EctType!=CBT_GROUND) {
      StartModelAnim(CYBORG_ANIM_BIKEREST, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(CYBORG_ANIM_WALK01, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void ChangeCollisionToAir() {
    ChangeCollisionBoxIndexWhenPossible(CYBORG_COLLISION_BOX_BIKE);
  };
  void ChangeCollisionToGround() {
    ChangeCollisionBoxIndexWhenPossible(CYBORG_COLLISION_BOX_GROUND);
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
 *                 BLOW UP FUNCTIONS                        *
 ************************************************************/
  // spawn body parts
  void BlowUp(void) {
    // get your size
    FLOATaabbox3D box;
    GetBoundingBox(box);
    FLOAT fEntitySize = box.Size().MaxNorm();

    // spawn debris
    Debris_Begin(EIBT_ROBOT, DPR_SMOKETRAIL, BET_EXPLOSIONSTAIN, fEntitySize, m_vDamage*0.3f,
      en_vCurrentTranslationAbsolute, 1.0f, 0.0f);
    
    Debris_Spawn(this, this, MODEL_ASS, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_TORSO, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_STRONG, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_HEAD, TEXTURE_CYBORG,  TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_RIGHT_UPPER_ARM, TEXTURE_CYBORG,  TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_RIGHT_LOWER_ARM, TEXTURE_CYBORG,  TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_LEFT_UPPER_ARM, TEXTURE_CYBORG,  TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_LEFT_LOWER_ARM, TEXTURE_CYBORG,  TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_RIGHT_UPPER_LEG, TEXTURE_CYBORG,  TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_RIGHT_LOWER_LEG, TEXTURE_CYBORG,  TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_LEFT_UPPER_LEG, TEXTURE_CYBORG,  TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_LEFT_LOWER_LEG, TEXTURE_CYBORG,  TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_FOOT, TEXTURE_CYBORG,  TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_FOOT, TEXTURE_CYBORG,  TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0,
      0, 0.0f, FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));

    // spawn explosion
    CPlacement3D plExplosion = GetPlacement();
    CEntityPointer penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_BOMB;
    FLOAT fSize = fEntitySize*0.3f;
    eSpawnEffect.vStretch = FLOAT3D(fSize,fSize,fSize);
    penExplosion->Initialize(eSpawnEffect);

    // hide yourself (must do this after spawning debris)
    SwitchToEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);
  };



/************************************************************
 *                     MOVING FUNCTIONS                     *
 ************************************************************/
  // fly desired position for attack
  virtual void FlyDesiredPosition(FLOAT3D &vPos) {
    FLOAT fDist = (m_penEnemy->GetPlacement().pl_PositionVector - 
                   GetPlacement().pl_PositionVector).Length();
    vPos = m_penEnemy->GetPlacement().pl_PositionVector;
    vPos += FLOAT3D(m_penEnemy->en_mRotation(1, 2),
                    m_penEnemy->en_mRotation(2, 2),
                    m_penEnemy->en_mRotation(3, 2)) * m_fFlyAboveEnemy;
  };

  // fly move in direction
  void FlyInDirection() {
    /* !!!!
    RotateToAngle();

    // determine translation speed
    FLOAT3D vTranslation = (m_vDesiredPosition - GetPlacement().pl_PositionVector) * !en_mRotation;
    vTranslation(1) = 0.0f;
    vTranslation.Normalize();
    vTranslation *= m_fMoveSpeed;

    // start moving
    SetDesiredTranslation(vTranslation);
    */
  };

  // fly entity to desired position
  void FlyToPosition() {

/*  !!!!
    CalcAngleFromPosition();
    FlyInDirection();
    */
  };



procedures:
/************************************************************
 *                    CLASS INTERNAL                        *
 ************************************************************/
  // fall to floor
  FallToFloor(EVoid) {
    // spawn bike
    CPlacement3D plBike = GetPlacement();
    FLOATmatrix3D mRotation;
    MakeRotationMatrixFast(mRotation, GetPlacement().pl_OrientationAngle);
    plBike.pl_PositionVector += BIKE_ATTACHMENT*mRotation;

    ECyborgBike ecb;
    ecb.fSpeed = m_fFlySpeed*2.0f;
    CEntityPointer penBike = CreateEntity(plBike, CLASS_CYBORG_BIKE);
    penBike->Initialize(ecb);

    // drop to floor
    m_EctType = CBT_GROUND;
    SetPhysicsFlags(EPF_MODEL_WALKING);
    ChangeCollisionToGround();

    // remove bike
    RemoveAttachmentFromModel(*GetModelObject(), CYBORG_ATTACHMENT_BIKE);

    // anim
    if (IRnd()&1) {
      StartModelAnim(CYBORG_ANIM_FALL01, 0);
    } else {
      StartModelAnim(CYBORG_ANIM_FALL02, 0);
    }

    // wait to touch brush or time limit
    m_fFallStartTime = _pTimer->CurrentTick();
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
      on (EDamage) : { resume; }
      on (EWatch) : { resume; }
      on (ETimer) : { stop; }
    }

    // wait fall anim
    if (_pTimer->CurrentTick() < m_fFallStartTime+1.5f) {
      wait(m_fFallStartTime+1.5f - _pTimer->CurrentTick()) {
        on (EBegin) : { resume; }
        on (EDamage) : { resume; }
        on (EWatch) : { resume; }
        on (ETimer) : { stop; }
      }
    }
    return EReturn();
  };

  // get up
  GetUp(EVoid) {
    // get up
    StartModelAnim(CYBORG_ANIM_GETUP, 0);
    wait(GetModelObject()->GetAnimLength(CYBORG_ANIM_GETUP)) {
      on (EBegin) : { resume; }
      on (EDamage) : { resume; }
      on (EWatch) : { resume; }
      on (ETimer) : { stop; }
    }
    return EReturn();
  };



/************************************************************
 *      PROCEDURES WHEN NO ANY SPECIAL ACTION               *
 ************************************************************/
  // Move to destination
/*   !!!!
  MoveToDestination(EVoid) : CEnemyBase::MoveToDestination {
    // animation
    if (m_bRunToMarker) {
      RunningAnim();
    } else {
      WalkingAnim();
    }
    // fly to position
    if (m_EctType!=CBT_GROUND) {
      m_fMoveFrequency = 0.25f;
      m_fMovePrecision = m_fMoveSpeed*m_fMoveFrequency*2.0f;
      while ((m_vDesiredPosition-GetPlacement().pl_PositionVector).Length()>m_fMovePrecision) {
        wait (0.25f) {
          on (EBegin) : { FlyToPosition(); }
          on (ETimer) : { stop; }
        }
      }
      return EReturn();
    // move to position
    } else {
      jump CEnemyBase::MoveToDestination();
    }
  };
  */



/************************************************************
 *                PROCEDURES WHEN HARMED                    *
 ************************************************************/
  // Play wound animation and falling body part
  BeWounded(EDamage eDamage) : CEnemyBase::BeWounded {
    // damage only on ground
    if (m_EctType==CBT_GROUND) {
      jump CEnemyBase::BeWounded(eDamage);
    // fall on ground
    } else if (m_EctType==CBT_FLYGROUND && GetHealth()<=60.0f) {
      SetHealth(60.0f);
      m_fMaxHealth = 60.0f;
      autocall FallToFloor() EReturn;
      autocall GetUp() EReturn;
      SendEvent(ERestartAttack());
    }
    return EReturn();
  };



/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  AttackEnemy(EVoid) : CEnemyBase::AttackEnemy {
    // air attack
    if (m_EctType!=CBT_GROUND) {
      jump FlyAttackEnemy();
    // ground attack
    } else if (TRUE) {
      jump CEnemyBase::AttackEnemy();
    }
  };

  // fly attack enemy
  FlyAttackEnemy(EVoid) {
    // initial preparation
    autocall CEnemyBase::InitializeAttack() EReturn;

    // while you have some enemy
    while (m_penEnemy != NULL) {
      // to far cease attack
      if (CalcDist(m_penEnemy) > IGNORE_RANGE) {
        SetTargetNone();
      }

      if (m_penEnemy != NULL) {
        // attack run
        if (SeeEntity(m_penEnemy, CosFast(90.0f))) {
          autocall FlyAttackRun() EReturn;
        // go away and rotate
        } else if (TRUE) {
          autocall GoAwayAndRotate() EReturn;
        }
      }
    }

    // stop attack
    autocall CEnemyBase::StopAttack() EReturn;

    // return to Move() procedure
    return EBegin();
  };

  // fly attack run
  FlyAttackRun(EVoid) {
    m_iFireLaserCount = 0;
    if (m_bBombing) {
      m_ctBombsToDrop = 3;
    }
    while (SeeEntity(m_penEnemy, CosFast(90.0f))) {
      m_fMoveFrequency = 0.1f;
      wait(m_fMoveFrequency) {
        on (EBegin) : {
          if (IsInFrustum(m_penEnemy, CosFast(55.0f))) {
            // fire laser
            if (m_iFireLaserCount==0) {
              ShootProjectile(PRT_CYBORG_LASER, FIRE_BIKE, ANGLE3D(0, 0, 0));
              PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
            }
            if (m_iFireLaserCount++ == 2) {
              m_iFireLaserCount = 0;
            }
          }

          // if a bomb may be dropped
          if (m_ctBombsToDrop>0 && _pTimer->CurrentTick()>=m_tmLastBombDropped+0.3f) {
            // calculate where it would hit
            FLOAT fV = en_vCurrentTranslationAbsolute.Length();
            FLOAT fD = CalcDist(m_penEnemy);
            FLOAT fDP = CalcPlaneDist(m_penEnemy);
            FLOAT fH = Sqrt(fD*fD-fDP*fDP);
            FLOAT fHitD = fDP-fV*Sqrt(2*fH/en_fGravityA);

            // if close enough
            if( Abs(fHitD)<10.0f) {
              // drop it
              CPlacement3D pl(FLOAT3D(FRnd()*2.0f-1.0f, -1.0f, 0.0f), ANGLE3D(0, 0, 0)); ;
              pl.RelativeToAbsoluteSmooth(GetPlacement());
              CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
              ELaunchProjectile eLaunch;
              eLaunch.penLauncher = this;
              eLaunch.prtType = PRT_CYBORG_BOMB;
              eLaunch.fSpeed = fV;
              penProjectile->Initialize(eLaunch);
              m_ctBombsToDrop--;
              m_tmLastBombDropped = _pTimer->CurrentTick();
            }
          }

          // inside attack radius
          if (MayMoveToAttack()) {
            m_fMoveSpeed = m_fFlySpeed;
            m_aRotateSpeed = m_aFlyRotateSpeed;
            FlyDesiredPosition(m_vDesiredPosition);
            FlyToPosition();
            RunningAnim();
          // outside attack radius
          } else {
            StopMoving();
            StandingAnim();
          }
        }
        on (ETimer) : { stop; }
      }
    }

    return EReturn();
  };

  GoAwayAndRotate(EVoid) {
    // go away
    SetDesiredTranslation(FLOAT3D(0, 1.25f, -m_fFlySpeed));
    StopRotating();
    autowait(1.0f);

    // rotate side
    if (IRnd()&1) {
      SetDesiredRotation(ANGLE3D( 100.0f, 0, 0));
    } else {
      SetDesiredRotation(ANGLE3D(-100.0f, 0, 0));
    }

    // rotate to enemy
    SetDesiredTranslation(FLOAT3D(0, 0, -m_fFlySpeed));
    while (!SeeEntityInPlane(m_penEnemy, CosFast(5.0f))) {
      autowait(_pTimer->TickQuantum);
    }

    return EReturn();
  };

  Fire(EVoid) : CEnemyBase::Fire {
    // to fire
    StartModelAnim(CYBORG_ANIM_TOFIRE, 0);
    m_fLockOnEnemyTime = GetModelObject()->GetAnimLength(CYBORG_ANIM_TOFIRE) + FRnd()/3;
    autocall CEnemyBase::LockOnEnemy() EReturn;

    StartModelAnim(CYBORG_ANIM_FIRE02, 0);
    ShootProjectile(PRT_CYBORG_LASER, FIRE_LASER, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);

    m_fLockOnEnemyTime = 0.5f;
    autocall CEnemyBase::LockOnEnemy() EReturn;
    StartModelAnim(CYBORG_ANIM_FIRE02, 0);
    ShootProjectile(PRT_CYBORG_LASER, FIRE_LASER, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);

    m_fLockOnEnemyTime = 0.5f;
    autocall CEnemyBase::LockOnEnemy() EReturn;
    StartModelAnim(CYBORG_ANIM_FIRE02, 0);
    ShootProjectile(PRT_CYBORG_LASER, FIRE_LASER, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.1f + FRnd()/3);

    m_fShootTime = _pTimer->CurrentTick() + m_fAttackFireTime*(1.0f + FRnd()/3.0f);

    // from fire
    StartModelAnim(CYBORG_ANIM_FROMFIRE, 0);
    autowait(GetModelObject()->GetAnimLength(CYBORG_ANIM_FROMFIRE));

    return EReturn();
  };

  Hit(EVoid) : CEnemyBase::Hit {
    // animation
    m_iCloseHit = IRnd()&1;
    if (m_iCloseHit==0) {
      StartModelAnim(CYBORG_ANIM_ATTACKCLOSE01, 0);
    } else {
      StartModelAnim(CYBORG_ANIM_ATTACKCLOSE02, 0);
    }

    autowait(0.9f);
    PlaySound(m_soSound, SOUND_KICK, SOF_3D);
    if (CalcDist(m_penEnemy)<m_fCloseDistance) {
      // damage enemy
      FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
      vDirection.Normalize();
      InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 15.0f, FLOAT3D(0, 0, 0), vDirection);
      // push target left/right
      FLOAT3D vSpeed;
      if (m_iCloseHit==0) {
        GetHeadingDirection(AngleDeg(90.0f), vSpeed);
      } else {
        GetHeadingDirection(AngleDeg(-90.0f), vSpeed);
      }
      vSpeed = vSpeed * 5.0f;
      KickEntity(m_penEnemy, vSpeed);
    }

    return EReturn();
  };



/************************************************************
 *                    D  E  A  T  H                         *
 ************************************************************/
  Death(EVoid) : CEnemyBase::Death {
    StopMoving();     // stop moving
    DeathSound();     // death sound

    // death notify (usually change collision box and change body density)
    ChangeCollisionBoxIndexWhenPossible(CYBORG_COLLISION_BOX_DEATH);

    // set physic flags
    SetPhysicsFlags(EPF_MODEL_CORPSE);
    SetCollisionFlags(ECF_CORPSE);

    if (m_EctType==CBT_FLY || m_EctType==CBT_FLYGROUND) {
      autocall FallToFloor() EReturn;
    } else if (TRUE) {
      // start death anim
      INDEX iAnim = AnimForDeath();
      autowait(GetModelObject()->GetAnimLength(iAnim));
    }

    // death twist
    StartModelAnim(CYBORG_ANIM_DEATHTWIST, AOF_LOOPING);
    autowait(FRnd()*5.0f + 1.0f);
    StartModelAnim(CYBORG_ANIM_DEATHREST, 0);

    // explode
    SetHealth(-45.0f);
    ReceiveDamage(NULL, DMT_EXPLOSION, 10.0f, FLOAT3D(0,0,0), FLOAT3D(0,1,0));

    return EEnd();
  };



/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    en_fDensity = 5000.0f;

    // set your appearance
    SetModel(MODEL_CYBORG);
    SetModelMainTexture(TEXTURE_CYBORG);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_ASS,
      MODEL_ASS, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_TORSO,
      MODEL_TORSO, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_STRONG, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_HEAD,
      MODEL_HEAD, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_RIGHTUPPERARM,
      MODEL_RIGHT_UPPER_ARM, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_RIGHTLOWERARM,
      MODEL_RIGHT_LOWER_ARM, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_LEFTUPPERARM,
      MODEL_LEFT_UPPER_ARM, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_LEFTLOWERARM,
      MODEL_LEFT_LOWER_ARM, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_RIGHTUPPERLEG,
      MODEL_RIGHT_UPPER_LEG, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_RIGHTLOWERLEG,
      MODEL_RIGHT_LOWER_LEG, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_LEFTUPPERLEG,
      MODEL_LEFT_UPPER_LEG, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_LEFTLOWERLEG,
      MODEL_LEFT_LOWER_LEG, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_FOOTRIGHT,
      MODEL_FOOT, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_FOOTLEFT,
      MODEL_FOOT, TEXTURE_CYBORG, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    if (m_EctType!=CBT_GROUND) {
      AddAttachmentToModel(this, *GetModelObject(), CYBORG_ATTACHMENT_BIKE,
        MODEL_BIKE, TEXTURE_BIKE, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
      // fly in air
      SetHealth(90.0f);
      m_fMaxHealth = 90.0f;
      ChangeCollisionToAir();
      SetPhysicsFlags(EPF_MODEL_FLYING);
      m_iScore = 1000;
    } else {
      // walk on ground
      SetHealth(60.0f);
      m_fMaxHealth = 60.0f;
      ChangeCollisionToGround();
      SetPhysicsFlags(EPF_MODEL_WALKING);
      m_iScore = 500;
    }
    StandingAnim();
    // setup moving speed
    m_fWalkSpeed = FRnd()*3.0f + 6.0f;
    m_aWalkRotateSpeed = FRnd()*20.0f + 700.0f;
    m_fAttackRunSpeed = m_fWalkSpeed;
    m_aAttackRotateSpeed = m_aWalkRotateSpeed;
    m_fCloseRunSpeed = m_fWalkSpeed;
    m_aCloseRotateSpeed = m_aWalkRotateSpeed;
    m_fWalkSpeed/=3;

    // setup attack distances
    m_fAttackDistance = 100.0f;
    m_fCloseDistance = 2.5f;
    m_fStopDistance = 1.5;
    m_fAttackFireTime = 3.0f;
    m_fCloseFireTime = 2.0f;
    m_fIgnoreRange = 200.0f;
    // fly moving properties
    m_fFlyAboveEnemy = 10.0f+FRnd()*1.0f;
    m_fFlySpeed = FRnd()*5.0f + 20.0f;
    m_aFlyRotateSpeed = FRnd()*25.0f + 100.0f;
    // damage/explode properties
    m_fBlowUpAmount = 90.0f;
    m_fDamageWounded = 50.0f;

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
