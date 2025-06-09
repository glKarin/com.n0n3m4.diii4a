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

328
%{
#include "Entities/StdH/StdH.h"
#include "Models/Enemies/Fishman/fishman.h"
%}

uses "Entities/EnemyDive";

%{
static EntityInfo eiFishmanGround = {
  EIBT_FLESH, 100.0f,
  0.0f, 1.4f, 0.0f,
  0.0f, 1.0f, 0.0f,
};
static EntityInfo eiFishmanLiquid = {
  EIBT_FLESH, 33.3f,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
};

#define FIRE_WATER    FLOAT3D(0.0f, 0.3f, -1.1f)
#define FIRE_GROUND   FLOAT3D(0.0f, 0.8f, -1.25f)
#define SPEAR_HIT   1.75f
%}


class CFishman : CEnemyDive {
name      "Fishman";
thumbnail "Thumbnails\\Fishman.tbn";

properties:
components:
  0 class   CLASS_BASE        "Classes\\EnemyDive.ecl",
  1 model   MODEL_FISHMAN     "Models\\Enemies\\Fishman\\Fishman.mdl",
  2 texture TEXTURE_FISHMAN   "Models\\Enemies\\Fishman\\Fishman.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE_WATER    "Models\\Enemies\\Fishman\\Sounds\\IdleWater.wav",
 51 sound   SOUND_IDLE_GROUND   "Models\\Enemies\\Fishman\\Sounds\\IdleGround.wav",
 52 sound   SOUND_SIGHT_WATER   "Models\\Enemies\\Fishman\\Sounds\\SightWater.wav",
 53 sound   SOUND_SIGHT_GROUND  "Models\\Enemies\\Fishman\\Sounds\\SightGround.wav",
 54 sound   SOUND_WOUND_WATER   "Models\\Enemies\\Fishman\\Sounds\\WoundWater.wav",
 55 sound   SOUND_WOUND_GROUND  "Models\\Enemies\\Fishman\\Sounds\\WoundGround.wav",
 56 sound   SOUND_DEATH_WATER   "Models\\Enemies\\Fishman\\Sounds\\DeathWater.wav",
 57 sound   SOUND_DEATH_GROUND  "Models\\Enemies\\Fishman\\Sounds\\DeathGround.wav",
 58 sound   SOUND_FIRE          "Models\\Enemies\\Fishman\\Sounds\\Fire.wav",
 59 sound   SOUND_KICK          "Models\\Enemies\\Fishman\\Sounds\\Kick.wav",

functions:
  /* Entity info */
  void *GetEntityInfo(void) {
    if (m_bInLiquid) {
      return &eiFishmanLiquid;
    } else {
      return &eiFishmanGround;
    }
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // fishman can't harm fishman
    if (!IsOfClass(penInflictor, "Fishman")) {
      CEnemyDive::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };


  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    if (m_bInLiquid) {
      switch (IRnd()%2) {
        case 0: iAnim = FISHMAN_ANIM_WATERWOUND01; break;
        case 1: iAnim = FISHMAN_ANIM_WATERWOUND02; break;
        default: iAnim = FISHMAN_ANIM_WATERWOUND01; //ASSERTALWAYS("Fishman unknown liquid damage");
      }
    } else {
      switch (IRnd()%3) {
        case 0: iAnim = FISHMAN_ANIM_GROUNDWOUND03; break;
        case 1: iAnim = FISHMAN_ANIM_GROUNDWOUND04; break;
        case 2: iAnim = FISHMAN_ANIM_GROUNDWOUND05; break;
        default: iAnim = FISHMAN_ANIM_GROUNDWOUND03; //ASSERTALWAYS("Fishman unknown ground damage");
      }
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    INDEX iAnim;
    if (m_bInLiquid) {
      iAnim = FISHMAN_ANIM_WATERDEATH;
    } else {
      iAnim = FISHMAN_ANIM_GROUNDDEATH;
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  void DeathNotify(void) {
    if (m_bInLiquid) {
      ChangeCollisionBoxIndexWhenPossible(FISHMAN_COLLISION_BOX_DEATH_WATER);
    } else {
      ChangeCollisionBoxIndexWhenPossible(FISHMAN_COLLISION_BOX_DEATH_GROUND);
    }
    en_fDensity = 500.0f;
  };

  // virtual anim functions
  void StandingAnim(void) {
    if (m_bInLiquid) {
      StartModelAnim(FISHMAN_ANIM_WATERSTAND, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(FISHMAN_ANIM_GROUNDSTAND, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void WalkingAnim(void) {
    if (m_bInLiquid) {
      StartModelAnim(FISHMAN_ANIM_WATERSWIM02, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(FISHMAN_ANIM_GROUNDWALK, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RunningAnim(void) {
    if (m_bInLiquid) {
      StartModelAnim(FISHMAN_ANIM_WATERSWIM01, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(FISHMAN_ANIM_GROUNDRUN, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RotatingAnim(void) {
    if (m_bInLiquid) {
      StartModelAnim(FISHMAN_ANIM_WATERSWIM02, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(FISHMAN_ANIM_GROUNDWALK, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void ChangeCollisionToLiquid() {
    ChangeCollisionBoxIndexWhenPossible(FISHMAN_COLLISION_BOX_WATER);
  };
  void ChangeCollisionToGround() {
    ChangeCollisionBoxIndexWhenPossible(FISHMAN_COLLISION_BOX_GROUND);
  };

  // virtual sound functions
  void IdleSound(void) {
    if (m_bInLiquid) {
      PlaySound(m_soSound, SOUND_IDLE_WATER , SOF_3D);
    } else {
      PlaySound(m_soSound, SOUND_IDLE_GROUND, SOF_3D);
    }
  };
  void SightSound(void) {
    if (m_bInLiquid) {
      PlaySound(m_soSound, SOUND_SIGHT_WATER , SOF_3D);
    } else {                                 
      PlaySound(m_soSound, SOUND_SIGHT_GROUND, SOF_3D);
    }
  };
  void WoundSound(void) {
    if (m_bInLiquid) {
      PlaySound(m_soSound, SOUND_WOUND_WATER , SOF_3D);
    } else {                                        
      PlaySound(m_soSound, SOUND_WOUND_GROUND, SOF_3D);
    }
  };
  void DeathSound(void) {
    if (m_bInLiquid) {
      PlaySound(m_soSound, SOUND_DEATH_WATER , SOF_3D);
    } else {                                        
      PlaySound(m_soSound, SOUND_DEATH_GROUND, SOF_3D);
    }
  };


procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  DiveFire(EVoid) : CEnemyDive::DiveFire {
    // fire projectile
    StartModelAnim(FISHMAN_ANIM_WATERATTACK02, 0);
    autowait(0.4f);
    ShootProjectile(PRT_FISHMAN_FIRE, FIRE_WATER, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.95f);
    StandingAnim();
    autowait(FRnd()/3 + _pTimer->TickQuantum);

    return EReturn();
  };

  DiveHit(EVoid) : CEnemyDive::DiveHit {
    if (CalcDist(m_penEnemy) > SPEAR_HIT) {
      // run to enemy
      m_fShootTime = _pTimer->CurrentTick() + 0.25f;
      return EReturn();
    }

    // attack with spear
    StartModelAnim(FISHMAN_ANIM_WATERATTACK01, 0);

    // to left hit
    autowait(0.5f);
    PlaySound(m_soSound, SOUND_KICK, SOF_3D);
    if (CalcDist(m_penEnemy)<SPEAR_HIT) {
      // damage enemy
      FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
      vDirection.Normalize();
      InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 5.0f, FLOAT3D(0, 0, 0), vDirection);
      // push target away
      FLOAT3D vSpeed;
      GetHeadingDirection(0.0f, vSpeed);
      vSpeed = vSpeed * 5.0f;
      KickEntity(m_penEnemy, vSpeed);
    }
    autowait(0.5f);

    StandingAnim();
    autowait(0.2f + FRnd()/3);
    return EReturn();
  };

  Fire(EVoid) : CEnemyBase::Fire {
    // wait anim end
    if (!GetModelObject()->IsAnimFinished()) {
      autowait(GetModelObject()->GetCurrentAnimLength() - GetModelObject()->GetPassedTime());
    }

    // fire projectile
    StartModelAnim(FISHMAN_ANIM_GROUNDATTACK02, 0);
    autowait(0.3f);
    ShootProjectile(PRT_FISHMAN_FIRE, FIRE_GROUND, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.4f);
    StandingAnim();
    autowait(FRnd()/3 + _pTimer->TickQuantum);

    return EReturn();
  };

  Hit(EVoid) : CEnemyBase::Hit {
    if (CalcDist(m_penEnemy) > SPEAR_HIT) {
      // run to enemy
      m_fShootTime = _pTimer->CurrentTick() + 0.25f;
      return EReturn();
    }

    // wait anim end
    if (!GetModelObject()->IsAnimFinished()) {
      autowait(GetModelObject()->GetCurrentAnimLength() - GetModelObject()->GetPassedTime());
    }

    // attack with spear
    StartModelAnim(FISHMAN_ANIM_GROUNDATTACKLOOP, 0);

    // to left hit
    autowait(0.5f);
    PlaySound(m_soSound, SOUND_KICK, SOF_3D);
    if (CalcDist(m_penEnemy)<SPEAR_HIT) {
      // damage enemy
      FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
      vDirection.Normalize();
      InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 2.5f, FLOAT3D(0, 0, 0), vDirection);
      // push target left
      FLOAT3D vSpeed;
      GetHeadingDirection(90.0f, vSpeed);
      vSpeed = vSpeed * 5.0f;
      KickEntity(m_penEnemy, vSpeed);
    }
    autowait(0.5f);
    PlaySound(m_soSound, SOUND_KICK, SOF_3D);
    if (CalcDist(m_penEnemy)<SPEAR_HIT) {
      // damage enemy
      FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
      vDirection.Normalize();
      InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 2.5f, FLOAT3D(0, 0, 0), vDirection);
      // push target right
      FLOAT3D vSpeed;
      GetHeadingDirection(-90.0f, vSpeed);
      vSpeed = vSpeed * 5.0f;
      KickEntity(m_penEnemy, vSpeed);
    }
    autowait(0.6f);

    StandingAnim();
    autowait(0.2f + FRnd()/3);
    return EReturn();
  };



/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING|EPF_HASLUNGS|EPF_HASGILLS);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    SetHealth(30.0f);
    m_fMaxHealth = 30.0f;
    en_tmMaxHoldBreath = 5.0f;
    en_fDensity = 1000.0f;

    // set your appearance
    SetModel(MODEL_FISHMAN);
    SetModelMainTexture(TEXTURE_FISHMAN);
    // setup moving speed
    m_fWalkSpeed = FRnd() + 1.5f;
    m_aWalkRotateSpeed = FRnd()*10.0f + 500.0f;
    m_fAttackRunSpeed = FRnd()*0.5f + 5.0f;
    m_aAttackRotateSpeed = FRnd()*50 + 245.0f;
    m_fCloseRunSpeed = FRnd()*0.5f + 5.0f;
    m_aCloseRotateSpeed = FRnd()*50 + 245.0f;
    // setup attack distances
    m_fAttackDistance = 50.0f;
    m_fCloseDistance = 5.0f;
    m_fStopDistance = 0.0f;
    m_fAttackFireTime = 3.0f;
    m_fCloseFireTime = 2.0f;
    m_fIgnoreRange = 200.0f;
    // fly moving properties
    m_fDiveWalkSpeed = FRnd() + 3.0f;
    m_aDiveWalkRotateSpeed = FRnd()*10.0f + 500.0f;
    m_fDiveAttackRunSpeed = FRnd()*4.0f + 14.0f;
    m_aDiveAttackRotateSpeed = FRnd()*25 + 500.0f;
    m_fDiveCloseRunSpeed = FRnd()*2.0f + 8.0f;
    m_aDiveCloseRotateSpeed = FRnd()*50 + 800.0f;
    // attack properties
    m_fDiveAttackDistance = 50.0f;
    m_fDiveCloseDistance = 3.0f;
    m_fDiveStopDistance = 0.0f;
    m_fDiveAttackFireTime = 3.0f;
    m_fDiveCloseFireTime = 2.0f;
    m_fDiveIgnoreRange = 200.0f;
    // damage/explode properties
    m_fBlowUpAmount = 60.0f;
    m_fBodyParts = 4;
    m_fDamageWounded = 15.0f;
    m_iScore = 500;

    // continue behavior in base class
    jump CEnemyDive::MainLoop();
  };
};
