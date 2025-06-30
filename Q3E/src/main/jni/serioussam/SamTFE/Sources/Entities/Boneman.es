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

305
%{
#include "Entities/StdH/StdH.h"
#include "Models/Enemies/Boneman/Boneman.h"
%}

uses "Entities/EnemyBase";

%{
// info structure
static EntityInfo eiBoneman = {
 EIBT_BONES, 250.0f,
 0.0f, 1.9f, 0.0f,    // source (eyes)
 0.0f, 1.9f, 0.0f,    // target (body)
};

#define BONES_HIT 2.8f
#define FIRE_RIGHT_HAND     FLOAT3D( 0.25f, 1.5f, 0.0f)
#define FIRE_LEFT_HAND      FLOAT3D(-0.25f, 1.5f, 0.0f)
%}


class CBoneman : CEnemyBase {
name      "Boneman";
thumbnail "Thumbnails\\Boneman.tbn";

properties:
  2 BOOL m_bFistHit = FALSE,          // used for close attack
  3 BOOL m_bTouchAnother = FALSE,     // another entity touched on far attack
  4 CSoundObject m_soFeet,            // for running sound
  5 BOOL m_bRunSoundPlaying = FALSE,

components:
  0 class   CLASS_BASE        "Classes\\EnemyBase.ecl",
  1 model   MODEL_BONEMAN     "Models\\Enemies\\Boneman\\Boneman.mdl",
  2 texture TEXTURE_BONEMAN   "Models\\Enemies\\Boneman\\Boneman.tex",
  3 class   CLASS_PROJECTILE  "Classes\\Projectile.ecl",

// ************** BONEMAN PARTS **************
 10 model     MODEL_BONEMAN_BODY   "Models\\Enemies\\Boneman\\Debris\\Body.mdl",
 11 model     MODEL_BONEMAN_HAND   "Models\\Enemies\\Boneman\\Debris\\Hand.mdl",
 12 model     MODEL_BONEMAN_LEGS   "Models\\Enemies\\Boneman\\Debris\\Legs.mdl",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE      "Models\\Enemies\\Boneman\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT     "Models\\Enemies\\Boneman\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND     "Models\\Enemies\\Boneman\\Sounds\\Wound.wav",
 53 sound   SOUND_FIRE      "Models\\Enemies\\Boneman\\Sounds\\Fire.wav",
 54 sound   SOUND_KICK      "Models\\Enemies\\Boneman\\Sounds\\Kick.wav",
 55 sound   SOUND_PUNCH     "Models\\Enemies\\Boneman\\Sounds\\Punch.wav",
 56 sound   SOUND_DEATH     "Models\\Enemies\\Boneman\\Sounds\\Death.wav",
 57 sound   SOUND_RUN       "Models\\Enemies\\Boneman\\Sounds\\Run.wav",

functions:
  void Precache(void) {
    CEnemyBase::Precache();
    PrecacheSound(SOUND_IDLE );
    PrecacheSound(SOUND_SIGHT);
    PrecacheSound(SOUND_WOUND);
    PrecacheSound(SOUND_FIRE );
    PrecacheSound(SOUND_KICK );
    PrecacheSound(SOUND_PUNCH);
    PrecacheSound(SOUND_DEATH);
    PrecacheSound(SOUND_RUN  );

    PrecacheModel(MODEL_BONEMAN_BODY);
    PrecacheModel(MODEL_BONEMAN_HAND);
    PrecacheModel(MODEL_BONEMAN_LEGS);

    PrecacheClass(CLASS_PROJECTILE, PRT_BONEMAN_FIRE);
  };

  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    if (eDeath.eLastDamage.dmtType==DMT_CLOSERANGE) {
      str.PrintF(TRANS("%s was ripped apart by a Kleer"),(const char*) strPlayerName);
    } else {
      str.PrintF(TRANS("%s was killed by a Kleer"),(const char*) strPlayerName);
    }
    return str;
  }

  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnm, "Data\\Messages\\Enemies\\Boneman.txt");
    return fnm;
  };

  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiBoneman;
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // boneman can't harm boneman
    if (!IsOfClass(penInflictor, "Boneman")) {
      CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };

  void LeaveStain(BOOL bGrow)
  {
    // boneman doesn't leave bloody stain
  }

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    switch (IRnd()%5) {
      case 0: iAnim = BONEMAN_ANIM_WOUNDCRITICAL01; break;
      case 1: iAnim = BONEMAN_ANIM_WOUNDCRITICAL02; break;
      case 2: iAnim = BONEMAN_ANIM_WOUNDCRITICAL03; break;
      case 3: iAnim = BONEMAN_ANIM_FALL01; break;
      case 4: iAnim = BONEMAN_ANIM_FALL02; break;
      default: iAnim = BONEMAN_ANIM_WOUNDCRITICAL01; //ASSERTALWAYS("Boneman unknown damage");
    }
    StartModelAnim(iAnim, 0);
    DeactivateRunningSound();
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    INDEX iAnim;
    switch (IRnd()%2) {
      case 0: iAnim = BONEMAN_ANIM_DEATHTOBACK; break;
      case 1: iAnim = BONEMAN_ANIM_DEATHTOFRONT; break;
      default: iAnim = BONEMAN_ANIM_DEATHTOBACK; //ASSERTALWAYS("Boneman unknown death");
    }
    StartModelAnim(iAnim, 0);
    DeactivateRunningSound();
    return iAnim;
  };

  void DeathNotify(void) {
    ChangeCollisionBoxIndexWhenPossible(BONEMAN_COLLISION_BOX_DEATH);
  };

  // virtual anim functions
  void StandingAnim(void) {
    StartModelAnim(BONEMAN_ANIM_STANDLOOP, AOF_LOOPING|AOF_NORESTART);
    DeactivateRunningSound();
  };
  void WalkingAnim(void) {
    StartModelAnim(BONEMAN_ANIM_WALKLOOP, AOF_LOOPING|AOF_NORESTART);
    DeactivateRunningSound();
  };
  void RunningAnim(void) {
    StartModelAnim(BONEMAN_ANIM_RUNLOOP, AOF_LOOPING|AOF_NORESTART);
    ActivateRunningSound();
  };
  void RotatingAnim(void) {
    StartModelAnim(BONEMAN_ANIM_WALKLOOP, AOF_LOOPING|AOF_NORESTART);
    DeactivateRunningSound();
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


  // running sounds
  void ActivateRunningSound(void)
  {
    if (!m_bRunSoundPlaying) {
      PlaySound(m_soFeet, SOUND_RUN, SOF_3D|SOF_LOOP);
      m_bRunSoundPlaying = TRUE;
    }
  }
  void DeactivateRunningSound(void)
  {
    m_soFeet.Stop();
    m_bRunSoundPlaying = FALSE;
  }

/************************************************************
 *                 BLOW UP FUNCTIONS                        *
 ************************************************************/
  // spawn body parts
  void BlowUp(void) {
    // get your size
    FLOATaabbox3D box;
    GetBoundingBox(box);
    FLOAT fEntitySize = box.Size().MaxNorm();

    FLOAT3D vNormalizedDamage = m_vDamage-m_vDamage*(m_fBlowUpAmount/m_vDamage.Length());
    vNormalizedDamage /= Sqrt(vNormalizedDamage.Length());

    vNormalizedDamage *= 0.75f;

    FLOAT3D vBodySpeed = en_vCurrentTranslationAbsolute-en_vGravityDir*(en_vGravityDir%en_vCurrentTranslationAbsolute);

    // spawn debris
    Debris_Begin(EIBT_BONES, DPT_NONE, BET_NONE, fEntitySize, vNormalizedDamage, vBodySpeed, 5.0f, 2.0f);
    
    Debris_Spawn(this, this, MODEL_BONEMAN_BODY, TEXTURE_BONEMAN, 0, 0, 0, 0, 0.0f,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_BONEMAN_HAND, TEXTURE_BONEMAN, 0, 0, 0, 0, 0.0f,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_BONEMAN_HAND, TEXTURE_BONEMAN, 0, 0, 0, 0, 0.0f,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_BONEMAN_LEGS, TEXTURE_BONEMAN, 0, 0, 0, 0, 0.0f,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));

    // hide yourself (must do this after spawning debris)
    SwitchToEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);
  };





procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  Fire(EVoid) : CEnemyBase::Fire {
    // fire projectile
    StartModelAnim(BONEMAN_ANIM_ATTACKCLOSELOOP, 0);
    DeactivateRunningSound();
    autowait(0.35f);
    ShootProjectile(PRT_BONEMAN_FIRE, FIRE_RIGHT_HAND, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(0.45f);
    ShootProjectile(PRT_BONEMAN_FIRE, FIRE_LEFT_HAND, ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    autowait(FRnd()/3+0.6f);

    return EReturn();
  };

  Hit(EVoid) : CEnemyBase::Hit {
    // hit
    if (CalcDist(m_penEnemy) < BONES_HIT) {
      jump HitWithBones();

    // jump
    } else if (CalcDist(m_penEnemy) < 10.0f) {
      jump JumpOnEnemy();
    }

    // run to enemy
    m_fShootTime = _pTimer->CurrentTick() + 0.5f;
    return EReturn();
  };

  // jump on enemy
  JumpOnEnemy(EVoid) {
    StartModelAnim(BONEMAN_ANIM_ATTACKFAR, 0);
    DeactivateRunningSound();

    // jump
    FLOAT3D vDir = (m_penEnemy->GetPlacement().pl_PositionVector -
                    GetPlacement().pl_PositionVector).Normalize();
    vDir *= !GetRotationMatrix();
    vDir *= m_fCloseRunSpeed*1.5f;
    vDir(2) = 2.5f;
    SetDesiredTranslation(vDir);
    PlaySound(m_soSound, SOUND_KICK, SOF_3D);

    // animation - IGNORE DAMAGE WOUND -
    SpawnReminder(this, 0.5f, 0);
    m_iChargeHitAnimation = BONEMAN_ANIM_ATTACKFAR;
    m_fChargeHitDamage = 20.0f;
    m_fChargeHitAngle = 0.0f;
    m_fChargeHitSpeed = 15.0f;
    autocall CEnemyBase::ChargeHitEnemy() EReturn;
    autowait(0.3f);
    return EReturn();
  };

  // hit with bones
  HitWithBones(EVoid) {
    // attack with bones
    StartModelAnim(BONEMAN_ANIM_ATTACKCLOSELOOP, 0);
    DeactivateRunningSound();

    // right hand
    m_bFistHit = FALSE;
    autowait(0.35f);
    if (CalcDist(m_penEnemy)<BONES_HIT) { m_bFistHit = TRUE; }
    PlaySound(m_soSound, SOUND_PUNCH, SOF_3D);
    autowait(0.10f);
    if (CalcDist(m_penEnemy)<BONES_HIT) { m_bFistHit = TRUE; }
    if (m_bFistHit) {
      FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
      vDirection.Normalize();
      // damage enemy
      InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 10.0f, FLOAT3D(0, 0, 0), vDirection);
      // push target left
      FLOAT3D vSpeed;
      GetHeadingDirection(AngleDeg(90.0f), vSpeed);
      vSpeed = vSpeed * 5.0f;
      KickEntity(m_penEnemy, vSpeed);
    }

    // left hand
    m_bFistHit = FALSE;
    autowait(0.25f);
    if (CalcDist(m_penEnemy)<BONES_HIT) { m_bFistHit = TRUE; }
    PlaySound(m_soSound, SOUND_PUNCH, SOF_3D);
    autowait(0.10f);
    if (CalcDist(m_penEnemy)<BONES_HIT) { m_bFistHit = TRUE; }
    if (m_bFistHit) {
      // damage enemy
      FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
      vDirection.Normalize();
      InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 10.0f, FLOAT3D(0, 0, 0), vDirection);
      // push target left
      FLOAT3D vSpeed;
      GetHeadingDirection(AngleDeg(-90.0f), vSpeed);
      vSpeed = vSpeed * 5.0f;
      KickEntity(m_penEnemy, vSpeed);
    }
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
    SetHealth(125.0f);
    m_fMaxHealth = 125.0f;
    en_fDensity = 2000.0f;

    // set your appearance
    SetModel(MODEL_BONEMAN);
    SetModelMainTexture(TEXTURE_BONEMAN);
    StandingAnim();
    m_sptType = SPT_BONES;
    // setup moving speed
    m_fWalkSpeed = FRnd() + 2.5f;
    m_aWalkRotateSpeed = FRnd()*25.0f + 45.0f;
    m_fAttackRunSpeed = FRnd()*3.0f + 10.0f;
    m_aAttackRotateSpeed = FRnd()*200 + 600.0f;
    m_fCloseRunSpeed = FRnd() + 13.0f;
    m_aCloseRotateSpeed = FRnd()*100 + 1000.0f;
    // setup attack distances
    m_fAttackDistance = 100.0f;
    m_fCloseDistance = 30.0f;
    m_fStopDistance = 2.0f;
    m_fAttackFireTime = 3.0f;
    m_fCloseFireTime = 2.0f;
    m_fIgnoreRange = 200.0f;
    // damage/explode properties
    m_fBlowUpAmount = 70.0f;
    m_fBodyParts = 4;
    m_fDamageWounded = 80.0f;
    m_iScore = 1000;
    if (m_fStepHeight==-1) {
      m_fStepHeight = 4.0f;
    }

    // set stretch factors for height and width
    CEnemyBase::StretchModel();
    m_soFeet.Set3DParameters(80.0f, 5.0f, 1.0f, 1.0f);
    m_bRunSoundPlaying = FALSE;

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
