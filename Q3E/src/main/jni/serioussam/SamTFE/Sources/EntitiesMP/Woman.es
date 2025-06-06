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

320
%{
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/Enemies/Woman/Woman.h"
#include "Models/Enemies/Headman/headman.h"
#include "EntitiesMP/Headman.h"
%}

uses "EntitiesMP/EnemyFly";

%{
// info structure
static EntityInfo eiWomanStand = {
  EIBT_FLESH, 100.0f,
  0.0f, 1.55f, 0.0f,
  0.0f, 1.0f, 0.0f,
};
static EntityInfo eiWomanFly = {
  EIBT_FLESH, 80.0f,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
};

#define FIRE_AIR      FLOAT3D(0.0f, 0.25f, -0.65f)
#define FIRE_GROUND   FLOAT3D(0.0f, 1.3f, -0.5f)
#define KAMIKAZE_ATTACH FLOAT3D(0.0f, -0.43f, -0.28f)
%}


class CWoman : CEnemyFly {
name      "Woman";
thumbnail "Thumbnails\\Woman.tbn";

properties:

 10 BOOL  m_bKamikazeCarrier      "Kamikaze Carrier" = FALSE,
 11 RANGE m_rKamikazeDropDistance "Kamikaze Drop Range" = 40.0f,
 20 BOOL  m_bKamikazeAttached = FALSE,

components:
  0 class   CLASS_BASE        "Classes\\EnemyFly.ecl",
  1 model   MODEL_WOMAN       "ModelsMP\\Enemies\\Woman\\Woman.mdl",
  2 texture TEXTURE_WOMAN     "Models\\Enemies\\Woman\\Woman.tex",
  3 class   CLASS_PROJECTILE  "Classes\\Projectile.ecl",
  5 class   CLASS_HEADMAN     "Classes\\Headman.ecl",
  7 model   MODEL_HEADMAN     "Models\\Enemies\\Headman\\Headman.mdl",
  8 texture TEXTURE_HEADMAN   "Models\\Enemies\\Headman\\Kamikaze.tex",
  9 model   MODEL_BOMB        "Models\\Enemies\\Headman\\Projectile\\Bomb.mdl",
 10 texture TEXTURE_BOMB      "Models\\Enemies\\Headman\\Projectile\\Bomb.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE      "Models\\Enemies\\Woman\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT     "Models\\Enemies\\Woman\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND     "Models\\Enemies\\Woman\\Sounds\\Wound.wav",
 53 sound   SOUND_FIRE      "Models\\Enemies\\Woman\\Sounds\\Fire.wav",
 54 sound   SOUND_KICK      "Models\\Enemies\\Woman\\Sounds\\Kick.wav",
 55 sound   SOUND_DEATH     "Models\\Enemies\\Woman\\Sounds\\Death.wav",

functions:
  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    if (eDeath.eLastDamage.dmtType==DMT_CLOSERANGE) {
      str.PrintF(TRANSV("%s was beaten by a Scythian Harpy"), (const char *) strPlayerName);
    } else {
      str.PrintF(TRANSV("A Scythian Harpy got %s spellbound"), (const char *) strPlayerName);
    }
    return str;
  }
  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnm,  "Data\\Messages\\Enemies\\Woman.txt");
    return fnm;
  }
  void Precache(void) {
    CEnemyBase::Precache();
    PrecacheSound(SOUND_IDLE );
    PrecacheSound(SOUND_SIGHT);
    PrecacheSound(SOUND_WOUND);
    PrecacheSound(SOUND_FIRE );
    PrecacheSound(SOUND_KICK );
    PrecacheSound(SOUND_DEATH);
    PrecacheClass(CLASS_PROJECTILE, PRT_WOMAN_FIRE);
  };

  /* Entity info */
  void *GetEntityInfo(void) {
    if (m_bInAir) {
      return &eiWomanFly;
    } else {
      return &eiWomanStand;
    }
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // woman can't harm woman
    if (!IsOfClass(penInflictor, "Woman")) {
      CEnemyFly::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };


  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    if (m_bInAir) {
      iAnim = WOMAN_ANIM_AIRWOUND02;
    } else {
      iAnim = WOMAN_ANIM_GROUNDWOUND04;
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    INDEX iAnim;
    if (m_bInAir) {
      iAnim = WOMAN_ANIM_AIRDEATH;
    } else {
      iAnim = WOMAN_ANIM_GROUNDDEATH01;
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  FLOAT WaitForDust(FLOAT3D &vStretch) {
    if(GetModelObject()->GetAnim()==WOMAN_ANIM_AIRDEATH)
    {
      vStretch=FLOAT3D(1,1,2)*1.0f;
      return 0.6f;
    }
    else if(GetModelObject()->GetAnim()==WOMAN_ANIM_GROUNDDEATH01)
    {
      vStretch=FLOAT3D(1,1,2)*0.75f;
      return 0.525f;
    }
    return -1.0f;
  };

  void DeathNotify(void) {
    ChangeCollisionBoxIndexWhenPossible(WOMAN_COLLISION_BOX_DEATH);
    en_fDensity = 500.0f;
  };

  // virtual anim functions
  void StandingAnim(void) {
    if (m_bInAir) {
      StartModelAnim(WOMAN_ANIM_AIRSTAND, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(WOMAN_ANIM_GROUNDSTAND, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void WalkingAnim(void) {
    if (m_bInAir) {
      StartModelAnim(WOMAN_ANIM_AIRFLY, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(WOMAN_ANIM_GROUNDWALK, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RunningAnim(void) {
    if (m_bInAir) {
      StartModelAnim(WOMAN_ANIM_AIRFLY, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(WOMAN_ANIM_GROUNDRUN, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RotatingAnim(void) {
    if (m_bInAir) {
      StartModelAnim(WOMAN_ANIM_AIRFLY, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(WOMAN_ANIM_GROUNDWALK, AOF_LOOPING|AOF_NORESTART);
    }
  };
  FLOAT AirToGroundAnim(void) {
    StartModelAnim(WOMAN_ANIM_AIRTOGROUND, 0);
    return(GetModelObject()->GetAnimLength(WOMAN_ANIM_AIRTOGROUND));
  };
  FLOAT GroundToAirAnim(void) {
    StartModelAnim(WOMAN_ANIM_GROUNDTOAIR, 0);
    return(GetModelObject()->GetAnimLength(WOMAN_ANIM_GROUNDTOAIR));
  };
  void ChangeCollisionToAir() {
    ChangeCollisionBoxIndexWhenPossible(WOMAN_COLLISION_BOX_AIR);
  };
  void ChangeCollisionToGround() {
    ChangeCollisionBoxIndexWhenPossible(WOMAN_COLLISION_BOX_GROUND);
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
  
  void AttachKamikaze()
  {
    AddAttachmentToModel(this, *GetModelObject(), WOMAN_ATTACHMENT_KAMIKAZE, MODEL_HEADMAN, TEXTURE_HEADMAN, 0, 0, 0);
    CModelObject &amo = GetModelObject()->GetAttachmentModel(WOMAN_ATTACHMENT_KAMIKAZE)->amo_moModelObject;
    AddAttachmentToModel(this, amo, HEADMAN_ATTACHMENT_BOMB_RIGHT_HAND, MODEL_BOMB, TEXTURE_BOMB, 0, 0, 0);
    AddAttachmentToModel(this, amo, HEADMAN_ATTACHMENT_BOMB_LEFT_HAND, MODEL_BOMB, TEXTURE_BOMB, 0, 0, 0);
    amo.PlayAnim(HEADMAN_ANIM_KAMIKAZE_ATTACK, AOF_LOOPING);
    m_bKamikazeAttached = TRUE;
  }

  void RemoveKamikaze()
  {
    RemoveAttachmentFromModel(*GetModelObject(), WOMAN_ATTACHMENT_KAMIKAZE);
  }

  void DropKamikaze()
  {
    if (!m_bKamikazeAttached) { return; }

    CEntity *pen = NULL;
    
    CPlacement3D pl;
    pl = GetPlacement();
    pl.pl_PositionVector += KAMIKAZE_ATTACH*GetRotationMatrix();
    pen = CreateEntity(pl, CLASS_HEADMAN);

    ((CHeadman *)&*pen)->m_hdtType = HDT_KAMIKAZE;

    // change needed properties
    pen->End();
    
    CEnemyBase *peb = ((CEnemyBase*)pen);
    peb->m_bTemplate = FALSE;
    pen->Initialize();        
    
    // mark that we don't have the kamikaze any more
    m_bKamikazeAttached = FALSE;

    // deattach the kamikaze model
    RemoveKamikaze();
  }


  void PreMoving() {
    if (m_bKamikazeAttached && m_bKamikazeCarrier) {
      // see if any of players are close enough to drop the kamikaze
      INDEX ctMaxPlayers = GetMaxPlayers();
      CEntity *penPlayer;
      for(INDEX i=0; i<ctMaxPlayers; i++) {
        penPlayer=GetPlayerEntity(i);
        if (penPlayer!=NULL) {
          if (DistanceTo(this, penPlayer)<m_rKamikazeDropDistance && IsVisible(penPlayer)) {
            DropKamikaze();
          }        
        }
      } // end for
    }
    CEnemyFly::PreMoving();
  }

  void BlowUp(void)
  {
    DropKamikaze();
    CEnemyFly::BlowUp();
  }
  

procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  FlyFire(EVoid) : CEnemyFly::FlyFire {
    if (m_bKamikazeAttached) { return EReturn(); }

    // fire projectile
    StartModelAnim(WOMAN_ANIM_AIRATTACK02, 0);
    autowait(0.6f);
    ShootProjectile(PRT_WOMAN_FIRE, FIRE_AIR, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.6f);
    StandingAnim();
    autowait(FRnd()/2 + _pTimer->TickQuantum);

    return EReturn();
  };
  
  FlyHit(EVoid) : CEnemyFly::FlyHit {
    if (m_bKamikazeAttached) { return EReturn(); }

    // if enemy near
    if (CalcDist(m_penEnemy) <= 5.0f) {
      // if enemy is not in water
      CMovableEntity *pen = (CMovableEntity *) m_penEnemy.ep_pen;
      CContentType &ctDn = pen->en_pwoWorld->wo_actContentTypes[pen->en_iDnContent];
      BOOL bEnemySwimming = !(ctDn.ct_ulFlags&CTF_BREATHABLE_LUNGS);
      if (bEnemySwimming) {
        jump FlyFire();
      } else {
        jump FlyOnEnemy();
      }
    }

    // run to enemy
    m_fShootTime = _pTimer->CurrentTick() + 0.25f;
    return EReturn();
  };

/************************************************************
 *                    D  E  A  T  H                         *
 ************************************************************/
  Death(EVoid) : CEnemyBase::Death {
    DropKamikaze();
    jump CEnemyFly::Death();
  };

  AirToGround(EVoid) : CEnemyFly::AirToGround {
    DropKamikaze();
    jump CEnemyFly::AirToGround(EVoid());
  };

  FlyOnEnemy(EVoid) {
    StartModelAnim(WOMAN_ANIM_AIRATTACK01, 0);

    // jump
    FLOAT3D vDir = PlayerDestinationPos();
    vDir = (vDir - GetPlacement().pl_PositionVector).Normalize();
    vDir *= !GetRotationMatrix();
    vDir *= m_fFlyCloseRunSpeed*1.9f;
    SetDesiredTranslation(vDir);
    PlaySound(m_soSound, SOUND_KICK, SOF_3D);

    // animation - IGNORE DAMAGE WOUND -
    SpawnReminder(this, 0.9f, 0);
    m_iChargeHitAnimation = WOMAN_ANIM_AIRATTACK01;
    m_fChargeHitDamage = 20.0f;
    m_fChargeHitAngle = 0.0f;
    m_fChargeHitSpeed = 10.0f;
    autocall CEnemyBase::ChargeHitEnemy() EReturn;

    StandingAnim();
    autowait(0.3f);
    return EReturn();
  }

  GroundFire(EVoid) : CEnemyFly::GroundFire {
    // fire projectile
    StartModelAnim(WOMAN_ANIM_GROUNDATTACK02, 0);
    autowait(0.3f);
    ShootProjectile(PRT_WOMAN_FIRE, FIRE_GROUND, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.3f);
    StandingAnim();
    autowait(FRnd()/2 + _pTimer->TickQuantum);

    return EReturn();
  };

  GroundHit(EVoid) : CEnemyFly::GroundHit {
    StartModelAnim(WOMAN_ANIM_GROUNDATTACK01, 0);

    // jump
    FLOAT3D vDir = (m_penEnemy->GetPlacement().pl_PositionVector -
                    GetPlacement().pl_PositionVector).Normalize();
    vDir *= !GetRotationMatrix();
    vDir *= m_fCloseRunSpeed*1.75f;
    vDir(2) = 2.5f;
    SetDesiredTranslation(vDir);
    PlaySound(m_soSound, SOUND_KICK, SOF_3D);

    // animation - IGNORE DAMAGE WOUND -
    SpawnReminder(this, 0.9f, 0);
    m_iChargeHitAnimation = WOMAN_ANIM_GROUNDATTACK01;
    m_fChargeHitDamage = 20.0f;
    m_fChargeHitAngle = 0.0f;
    m_fChargeHitSpeed = 10.0f;
    autocall CEnemyBase::ChargeHitEnemy() EReturn;

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
    SetHealth(100.0f);
    m_fMaxHealth = 100.0f;
    en_tmMaxHoldBreath = 5.0f;
    en_fDensity = 2000.0f;
    
    m_sptType = SPT_FEATHER;

    // set your appearance
    SetModel(MODEL_WOMAN);
    SetModelMainTexture(TEXTURE_WOMAN);
    // setup moving speed
    m_fWalkSpeed = FRnd() + 1.5f;
    m_aWalkRotateSpeed = FRnd()*10.0f + 25.0f;
    m_fAttackRunSpeed = FRnd()*2.0f + 9.0f;
    m_aAttackRotateSpeed = FRnd()*50 + 245.0f;
    m_fCloseRunSpeed = FRnd()*2.0f + 4.0f;
    m_aCloseRotateSpeed = FRnd()*50 + 245.0f;
    // setup attack distances
    m_fAttackDistance = 50.0f;
    m_fCloseDistance = 5.0f;
    m_fStopDistance = 0.0f;
    m_fAttackFireTime = 3.0f;
    m_fCloseFireTime = 2.0f;
    m_fIgnoreRange = 200.0f;
    // fly moving properties
    m_fFlyWalkSpeed = FRnd()/2 + 1.0f;
    m_aFlyWalkRotateSpeed = FRnd()*10.0f + 25.0f;
    m_fFlyAttackRunSpeed = FRnd()*2.0f + 10.0f;
    m_aFlyAttackRotateSpeed = FRnd()*25 + 150.0f;
    m_fFlyCloseRunSpeed = FRnd()*2.0f + 10.0f;
    m_aFlyCloseRotateSpeed = FRnd()*50 + 500.0f;
    m_fGroundToAirSpeed = m_fFlyCloseRunSpeed;
    m_fAirToGroundSpeed = m_fFlyCloseRunSpeed;
    m_fAirToGroundMin = 0.1f;
    m_fAirToGroundMax = 0.1f;
    // attack properties - CAN BE SET
    m_fFlyAttackDistance = 50.0f;
    m_fFlyCloseDistance = 12.5f;
    m_fFlyStopDistance = 0.0f;
    m_fFlyAttackFireTime = 3.0f;
    m_fFlyCloseFireTime = 2.0f;
    m_fFlyIgnoreRange = 200.0f;
    // damage/explode properties
    m_fBlowUpAmount = 100.0f;
    m_fBodyParts = 4;
    m_fDamageWounded = 20.0f;
    m_iScore = 1000;

    if (m_bKamikazeCarrier) {
      AttachKamikaze();
    }

    autowait(0.05f);

    // continue behavior in base class
    jump CEnemyFly::MainLoop();
  };
};
