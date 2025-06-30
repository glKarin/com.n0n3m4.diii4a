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

321
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Enemies/DragonMan/DragonMan.h"
%}

uses "EntitiesMP/EnemyFly";
uses "EntitiesMP/Projectile";

enum DragonmanType {
  0 DT_SOLDIER    "Soldier",
  1 DT_SERGEANT   "Sergeant",
  2 DT_MONSTER    "Monster",
};

%{
// info structure
static EntityInfo eiDragonmanStand1 = {
  EIBT_FLESH, 200.0f,
  0.0f, 1.55f, 0.0f,
  0.0f, 1.0f, 0.0f,
};
static EntityInfo eiDragonmanStand2 = {
  EIBT_FLESH, 200.0f*2,
  0.0f, 1.55f*2, 0.0f,
  0.0f, 1.0f*2, 0.0f,
};
static EntityInfo eiDragonmanStand4 = {
  EIBT_FLESH, 200.0f*4,
  0.0f, 1.55f*4, 0.0f,
  0.0f, 1.0f*4, 0.0f,
};

static EntityInfo eiDragonmanFly1 = {
  EIBT_FLESH, 100.0f*1,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
};
static EntityInfo eiDragonmanFly2 = {
  EIBT_FLESH, 100.0f*2,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
};
static EntityInfo eiDragonmanFly4 = {
  EIBT_FLESH, 100.0f*4,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
};

#define FLY_FIRE1           FLOAT3D( 0.0f, 0.25f, -1.5f)
#define GROUND_RIGHT_FIRE1  FLOAT3D( 0.3f, 2.2f, -0.85f)
#define GROUND_LEFT_FIRE1   FLOAT3D(-0.3f, 1.7f, -0.85f)
#define FLAME_AIR1          FLOAT3D( 0.0f, 0.1f, -1.75f)
#define FLAME_GROUND1       FLOAT3D( 0.0f, 2.7f, -0.85f)
%}


class CDragonman : CEnemyFly {
name      "Dragonman";
thumbnail "Thumbnails\\Dragonman.tbn";

properties:
  1 enum DragonmanType m_EdtType "Character" 'C' = DT_SOLDIER,       // type
  2 FLOAT3D m_vFlameSource = FLOAT3D(0,0,0),
  3 CEntityPointer m_penFlame,
  4 BOOL m_bBurnEnemy = FALSE,
  5 FLOAT m_fFireTime = 0.0f,

components:
  0 class   CLASS_BASE          "Classes\\EnemyFly.ecl",
  1 class   CLASS_PROJECTILE    "Classes\\Projectile.ecl",

  5 model   MODEL_DRAGONMAN     "Models\\Enemies\\Dragonman\\Dragonman.mdl",
  6 texture TEXTURE_DRAGONMAN1  "Models\\Enemies\\Dragonman\\Dragonman01.tex",
  7 texture TEXTURE_DRAGONMAN2  "Models\\Enemies\\Dragonman\\Dragonman02.tex",
  8 texture TEXTURE_DRAGONMAN3  "Models\\Enemies\\Dragonman\\Dragonman03.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE      "Models\\Enemies\\Dragonman\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT     "Models\\Enemies\\Dragonman\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND     "Models\\Enemies\\Dragonman\\Sounds\\Wound.wav",
 53 sound   SOUND_FIRE      "Models\\Enemies\\Dragonman\\Sounds\\Fire.wav",
 54 sound   SOUND_KICK      "Models\\Enemies\\Dragonman\\Sounds\\Kick.wav",
 55 sound   SOUND_DEATH     "Models\\Enemies\\Dragonman\\Sounds\\Death.wav",

functions:
  /* Entity info */
  void *GetEntityInfo(void) {
    if (m_bInAir) {
      switch(m_EdtType) {
      case DT_SOLDIER:
        return &eiDragonmanFly1;
        break;
      case DT_SERGEANT:
        return &eiDragonmanFly2;
        break;
      case DT_MONSTER:
        return &eiDragonmanFly4;
        break;
      default: {
        return &eiDragonmanFly1;
               } break;
      }
    } else {
      switch(m_EdtType) {
      case DT_SOLDIER:
        return &eiDragonmanStand1;
        break;
      case DT_SERGEANT:
        return &eiDragonmanStand2;
        break;
      case DT_MONSTER:
        return &eiDragonmanStand4;
        break;
      default: {
        return &eiDragonmanStand1;
               } break;
      }
    }
  };

  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    CEnemyBase::FillEntityStatistics(pes);
    switch(m_EdtType) {
    case DT_SOLDIER  : { pes->es_strName+=" Soldier"; } break;
    case DT_SERGEANT : { pes->es_strName+=" Sergeant"; } break;
    case DT_MONSTER  : { pes->es_strName+=" Monster"; } break;
    }
    return TRUE;
  }

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // woman can't harm woman
    if (!IsOfClass(penInflictor, "Dragonman")) {
      CEnemyFly::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };

  FLOAT3D GetStretchedVector(const FLOAT3D&v)
  {
    switch(m_EdtType) {
    case DT_SOLDIER:
      return v;
      break;
    case DT_SERGEANT:
      return v*2.0f;
      break;
    case DT_MONSTER:
      return v*4.0f;
      break;
    default:
      ASSERT(FALSE);
      return v;
    }
  }

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    if (m_bInAir) {
      switch (IRnd()%2) {
        case 0: iAnim = DRAGONMAN_ANIM_AIRWOUNDSLIGHT; break;
        case 1: iAnim = DRAGONMAN_ANIM_AIRWOUND02CRITICAL; break;
        default: iAnim = DRAGONMAN_ANIM_AIRWOUNDSLIGHT; //ASSERTALWAYS("Dragonman unknown fly damage");
      }
    } else {
      switch (IRnd()%3) {
        case 0: iAnim = DRAGONMAN_ANIM_GROUNDWOUNDCRITICALBACK; break;
        case 1: iAnim = DRAGONMAN_ANIM_GROUNDWOUNDCRITICALFRONT; break;
        case 2: iAnim = DRAGONMAN_ANIM_GROUNDWOUNDCRITICALBACK2; break;
        default: iAnim = DRAGONMAN_ANIM_GROUNDWOUNDCRITICALBACK; //ASSERTALWAYS("Dragonman unknown ground damage");
      }
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    INDEX iAnim;
    if (m_bInAir) {
      iAnim = DRAGONMAN_ANIM_AIRDEATH;
    } else {
      switch (IRnd()%2) {
        case 0: iAnim = DRAGONMAN_ANIM_GROUNDDEATHBACK; break;
        case 1: iAnim = DRAGONMAN_ANIM_GROUNDDEATHFRONT; break;
        default: iAnim = DRAGONMAN_ANIM_GROUNDDEATHBACK; //ASSERTALWAYS("Dragonman unknown ground death");
      }
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  void DeathNotify(void) {
    ChangeCollisionBoxIndexWhenPossible(DRAGONMAN_COLLISION_BOX_DEATH);
    en_fDensity = 500.0f;
  };

  // virtual anim functions
  void StandingAnim(void) {
    if (m_bInAir) {
      StartModelAnim(DRAGONMAN_ANIM_AIRSTANDLOOP, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(DRAGONMAN_ANIM_GROUNDSTANDLOOP, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void WalkingAnim(void) {
    if (m_bInAir) {
      StartModelAnim(DRAGONMAN_ANIM_AIRFLYLOOP, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(DRAGONMAN_ANIM_GROUNDWALK, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RunningAnim(void) {
    if (m_bInAir) {
      StartModelAnim(DRAGONMAN_ANIM_AIRFLYLOOP, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(DRAGONMAN_ANIM_GROUNDRUN, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RotatingAnim(void) {
    if (m_bInAir) {
      StartModelAnim(DRAGONMAN_ANIM_AIRFLYLOOP, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(DRAGONMAN_ANIM_GROUNDWALK, AOF_LOOPING|AOF_NORESTART);
    }
  };
  FLOAT AirToGroundAnim(void) {
    StartModelAnim(DRAGONMAN_ANIM_AIRTOGROUND, 0);
    return(GetModelObject()->GetAnimLength(DRAGONMAN_ANIM_AIRTOGROUND));
  };
  FLOAT GroundToAirAnim(void) {
    StartModelAnim(DRAGONMAN_ANIM_GROUNDTOAIR, 0);
    return(GetModelObject()->GetAnimLength(DRAGONMAN_ANIM_GROUNDTOAIR));
  };
  void ChangeCollisionToAir() {
    ChangeCollisionBoxIndexWhenPossible(DRAGONMAN_COLLISION_BOX_AIR);
  };
  void ChangeCollisionToGround() {
    ChangeCollisionBoxIndexWhenPossible(DRAGONMAN_COLLISION_BOX_GROUND);
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

  // flame source
  void GetFlamerSourcePlacement(CPlacement3D &plFlame) {
    plFlame.pl_PositionVector = m_vFlameSource;
  };

  // fire flame
  void FireFlame(void) {
    FLOAT3D vFlamePos;
    if (m_bInAir) {
      vFlamePos = GetStretchedVector(FLAME_AIR1);
    } else {
      vFlamePos = GetStretchedVector(FLAME_GROUND1);
    }

    // create flame
    CEntityPointer penFlame = ShootProjectile(PRT_FLAME, vFlamePos, ANGLE3D(0, 0, 0));
    // link last flame with this one (if not NULL or deleted)
    if (m_penFlame!=NULL && !(m_penFlame->GetFlags()&ENF_DELETED)) {
      ((CProjectile&)*m_penFlame).m_penParticles = penFlame;
    }
    // link to player weapons
    ((CProjectile&)*penFlame).m_penParticles = this;
    // store last flame
    m_penFlame = penFlame;
    // flame source position
    m_vFlameSource = GetPlacement().pl_PositionVector + vFlamePos*GetRotationMatrix();
  };


procedures:
/************************************************************
 *                PROCEDURES WHEN HARMED                    *
 ************************************************************/
  BeWounded(EDamage eDamage) : CEnemyFly::BeWounded { 
    m_penFlame = NULL;
    jump CEnemyFly::BeWounded(eDamage);
  };



/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  FlyFire(EVoid) : CEnemyFly::FlyFire {
    // fire projectile
    StartModelAnim(DRAGONMAN_ANIM_AIRATTACK02, 0);
    autowait(0.4f);
    if (m_EdtType != DT_MONSTER) {
      ShootProjectile(PRT_DRAGONMAN_FIRE, GetStretchedVector(FLY_FIRE1), ANGLE3D(0, 0, 0));
    } else {
      ShootProjectile(PRT_DRAGONMAN_STRONG_FIRE, GetStretchedVector(FLY_FIRE1), ANGLE3D(0, 0, 0));
    }
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.15f);
    if (m_EdtType != DT_MONSTER) {
      ShootProjectile(PRT_DRAGONMAN_FIRE, GetStretchedVector(FLY_FIRE1), ANGLE3D(0, 0, 0));
    } else if (m_EdtType == DT_MONSTER) {
      ShootProjectile(PRT_DRAGONMAN_STRONG_FIRE, GetStretchedVector(FLY_FIRE1), ANGLE3D(0, 0, 0));
    }
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.85f);
/*    StandingAnim();
    autowait(FRnd() + _pTimer->TickQuantum);
    */

    return EReturn();
  };

  FlyHit(EVoid) : CEnemyFly::FlyHit {
    // fly
    if (CalcDist(m_penEnemy) <= 7.5f) {
      if (m_EdtType == DT_SOLDIER) {
        jump FlyOnEnemy();
      } else {
        jump FlyBurn();
      }
    }

    // run to enemy
    m_fShootTime = _pTimer->CurrentTick() + 0.25f;
    return EReturn();
  };

  FlyOnEnemy(EVoid) {
    StartModelAnim(DRAGONMAN_ANIM_AIRATTACKCLOSELOOP, 0);

    // jump
    FLOAT3D vDir = PlayerDestinationPos();
    vDir = (vDir - GetPlacement().pl_PositionVector).Normalize();
    vDir *= !GetRotationMatrix();
    vDir *= m_fFlyCloseRunSpeed*1.9f;
    SetDesiredTranslation(vDir);
    PlaySound(m_soSound, SOUND_KICK, SOF_3D);

    // animation - IGNORE DAMAGE WOUND -
    SpawnReminder(this, 0.9f, 0);
    m_iChargeHitAnimation = DRAGONMAN_ANIM_AIRATTACKCLOSELOOP;
    if (m_EdtType == DT_SOLDIER) {
      m_fChargeHitDamage = 25.0f;
      m_fChargeHitSpeed = 15.0f;
    } else if (m_EdtType == DT_SERGEANT) {
      m_fChargeHitDamage = 30.0f;
      m_fChargeHitSpeed = 20.0f;
    } else if (TRUE) {
      m_fChargeHitDamage = 30.0f;
      m_fChargeHitSpeed = 20.0f;
    }
    m_fChargeHitAngle = 0.0f;
    autocall CEnemyBase::ChargeHitEnemy() EReturn;

    StandingAnim();
    autowait(0.3f);
    return EReturn();
  };

  FlyBurn(EVoid) {
    StartModelAnim(DRAGONMAN_ANIM_AIRATTACK02, 0);

    // burn
    m_fFireTime = _pTimer->CurrentTick();
    FireFlame();
    m_bBurnEnemy = TRUE;
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D|SOF_LOOP);
    while (m_bBurnEnemy) {
      m_fMoveFrequency = 0.1f;
      wait(m_fMoveFrequency) {
        // flame
        on (EBegin) : {
          m_vDesiredPosition = m_penEnemy->GetPlacement().pl_PositionVector;
          // rotate to enemy
          m_aRotateSpeed = 10000.0f;
          m_fMoveSpeed = 0.0f;
          // flame
          FireFlame();
          // stop
          if (_pTimer->CurrentTick()-m_fFireTime >= 1.29f) {
            m_bBurnEnemy = FALSE;
            stop;
          }
          // adjust direction and speed
          ULONG ulFlags = SetDesiredMovement(); 
          MovementAnimation(ulFlags);
          resume;
        }
        on (ETimer) : { stop; }
      }
    }
    m_soSound.Stop();

    // link last flame with nothing (if not NULL or deleted)
    if (m_penFlame!=NULL && !(m_penFlame->GetFlags()&ENF_DELETED)) {
      ((CProjectile&)*m_penFlame).m_penParticles = NULL;
      m_penFlame = NULL;
    }

    StandingAnim();
    autowait(0.3f);
    return EReturn();
  };

  Fire(EVoid) : CEnemyBase::Fire {
    // fire projectile
    StartModelAnim(DRAGONMAN_ANIM_GROUNDATTACKCLOSELOOP, 0);
    autowait(0.3f);
    if (m_EdtType != DT_MONSTER) {
      ShootProjectile(PRT_DRAGONMAN_FIRE, GetStretchedVector(GROUND_RIGHT_FIRE1), ANGLE3D(0, 0, 0));
    } else {
      ShootProjectile(PRT_DRAGONMAN_STRONG_FIRE, GetStretchedVector(GROUND_RIGHT_FIRE1), ANGLE3D(0, 0, 0));
    }
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.8f);
    if (m_EdtType != DT_MONSTER) {
      ShootProjectile(PRT_DRAGONMAN_FIRE, GetStretchedVector(GROUND_LEFT_FIRE1), ANGLE3D(0, 0, 0));
    } else if (m_EdtType == DT_MONSTER) {
      ShootProjectile(PRT_DRAGONMAN_STRONG_FIRE, GetStretchedVector(GROUND_LEFT_FIRE1), ANGLE3D(0, 0, 0));
    }
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.55f);
    StandingAnim();
    autowait(FRnd() + _pTimer->TickQuantum);
    return EReturn();
  };

  Hit(EVoid) : CEnemyBase::Hit {
    // burn enemy
    if ((m_EdtType == DT_SERGEANT && CalcDist(m_penEnemy) <= 6.0f)  ||
        (m_EdtType == DT_MONSTER  && CalcDist(m_penEnemy) <= 20.0f)) {
      jump BurnEnemy();
    }

    // run to enemy
    m_fShootTime = _pTimer->CurrentTick() + 0.25f;
    return EReturn();
  };

  BurnEnemy(EVoid) {
    StartModelAnim(DRAGONMAN_ANIM_GROUNDATTACKDISTANT, 0);

    // burn
    m_fFireTime = _pTimer->CurrentTick();
    FireFlame();
    m_bBurnEnemy = TRUE;
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D|SOF_LOOP);
    while (m_bBurnEnemy) {
      m_fMoveFrequency = 0.1f;
      wait(m_fMoveFrequency) {
        // flame
        on (EBegin) : {
          m_vDesiredPosition = m_penEnemy->GetPlacement().pl_PositionVector;
          // rotate to enemy
          m_fMoveSpeed = 0.0f;
          m_aRotateSpeed = 10000.0f;
          // adjust direction and speed
          SetDesiredMovement(); 
          // flame
          FireFlame();
          // stop
          if (_pTimer->CurrentTick()-m_fFireTime >= 1.29f) {
            m_bBurnEnemy = FALSE;
            stop;
          }
          resume;
        }
        on (ETimer) : { stop; }
      }
    }
    m_soSound.Stop();

    // link last flame with nothing (if not NULL or deleted)
    if (m_penFlame!=NULL && !(m_penFlame->GetFlags()&ENF_DELETED)) {
      ((CProjectile&)*m_penFlame).m_penParticles = NULL;
      m_penFlame = NULL;
    }

    StandingAnim();
    autowait(0.3f);
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

    if (m_EdtType == DT_SOLDIER) {
      GetModelObject()->StretchModel(FLOAT3D(1.0f, 1.0f, 1.0f));
      ModelChangeNotify();
      SetHealth(150.0f);
      m_fMaxHealth = 150.0f;
      m_fDamageWounded = 100.0f;
    } else if (m_EdtType == DT_SERGEANT) {
      GetModelObject()->StretchModel(FLOAT3D(2.0f, 2.0f, 2.0f));
      ModelChangeNotify();
      SetHealth(450.0f);
      m_fMaxHealth = 450.0f;
      m_fDamageWounded = 300.0f;
    } else if (TRUE) {
      GetModelObject()->StretchModel(FLOAT3D(4.0f, 4.0f, 4.0f));
      ModelChangeNotify();
      SetHealth(1350.0f);
      m_fMaxHealth = 1350.0f;
      m_fDamageWounded = 1000.0f;
    }
    en_tmMaxHoldBreath = 10.0f;
    en_fDensity = 2000.0f;

    // set your appearance
    SetModel(MODEL_DRAGONMAN);
    if (m_EdtType == DT_SOLDIER) {
      SetModelMainTexture(TEXTURE_DRAGONMAN1);
    } else if (m_EdtType == DT_SERGEANT) {
      SetModelMainTexture(TEXTURE_DRAGONMAN2);
    } else if (TRUE) {
      SetModelMainTexture(TEXTURE_DRAGONMAN3);
    }
    // setup moving speed
    if (m_EdtType == DT_SOLDIER) {
      m_fWalkSpeed = FRnd()*1.5f + 2.5f;
      m_aWalkRotateSpeed = FRnd()*20.0f + 50.0f;
      m_fAttackRunSpeed = FRnd()*2.0f + 11.0f;
      m_aAttackRotateSpeed = FRnd()*75 + 350.0f;
      m_fCloseRunSpeed = FRnd()*2.0f + 6.0f;
      m_aCloseRotateSpeed = FRnd()*50 + 500.0f;
    } else if (m_EdtType == DT_SERGEANT) {
      m_fWalkSpeed = (FRnd()*1.5f + 2.5f)*1.5f;
      m_aWalkRotateSpeed = FRnd()*20.0f + 50.0f;
      m_fAttackRunSpeed = (FRnd()*2.0f + 11.0f)*2;
      m_aAttackRotateSpeed = FRnd()*75 + 350.0f;
      m_fCloseRunSpeed = (FRnd()*2.0f + 6.0f)*1.5f;
      m_aCloseRotateSpeed = FRnd()*50 + 500.0f;
    } else if (TRUE) {
      m_fWalkSpeed = (FRnd()*1.5f + 2.5f)*2;
      m_aWalkRotateSpeed = FRnd()*20.0f + 50.0f;
      m_fAttackRunSpeed = (FRnd()*2.0f + 11.0f)*4;
      m_aAttackRotateSpeed = FRnd()*75 + 350.0f;
      m_fCloseRunSpeed = (FRnd()*2.0f + 6.0f)*2;
      m_aCloseRotateSpeed = FRnd()*50 + 500.0f;
    }
    // setup attack distances
    m_fAttackDistance = 100.0f;
    if (m_EdtType == DT_SOLDIER) {
      m_fCloseDistance = 0.0f;
      m_fStopDistance = 10.0f;
      m_fFlyCloseDistance = 12.5f;
      m_fFlyStopDistance = 0.0f;
      m_iScore = 1000;
    } else if (m_EdtType == DT_SERGEANT) {
      m_fCloseDistance = 20.0f;
      m_fStopDistance = 0.0f;
      m_fFlyCloseDistance = 12.5f*2;
      m_fFlyStopDistance = 0.0f;
      m_iScore = 2000;
    } else {
      m_fCloseDistance = 40.0f;
      m_fStopDistance = 0.0f;
      m_fFlyCloseDistance = 12.5f*4;
      m_fFlyStopDistance = 0.0f;
      m_iScore = 10000;
    }
    m_fAttackFireTime = 3.0f;
    m_fCloseFireTime = 2.0f;
    m_fIgnoreRange = 200.0f;
    // fly moving properties
    m_fFlyWalkSpeed = FRnd()/2 + 2.0f;
    m_aFlyWalkRotateSpeed = FRnd()*10.0f + 50.0f;
    m_fFlyAttackRunSpeed = FRnd()*2.0f + 10.0f;
    m_aFlyAttackRotateSpeed = FRnd()*75 + 350.0f;
    m_fFlyCloseRunSpeed = FRnd()*2.0f + 9.0f;
    m_aFlyCloseRotateSpeed = FRnd()*50 + 600.0f;
    // attack properties - CAN BE SET
    m_fFlyAttackDistance = 100.0f;
    m_fFlyAttackFireTime = 3.0f;
    m_fFlyCloseFireTime = 2.0f;
    m_fFlyIgnoreRange = 200.0f;
    // damage/explode properties
    m_fBlowUpAmount = 100.0f;
    m_fBodyParts = 8;
    // flame source
    m_vFlameSource = FLOAT3D(0, 0, 0);
    m_fGroundToAirSpeed = m_fFlyAttackRunSpeed;
    m_fAirToGroundSpeed = m_fFlyAttackRunSpeed*2;
    m_fAirToGroundMin = 0.1f;
    m_fAirToGroundMax = 0.1f;

    // continue behavior in base class
    jump CEnemyFly::MainLoop();
  };
};
