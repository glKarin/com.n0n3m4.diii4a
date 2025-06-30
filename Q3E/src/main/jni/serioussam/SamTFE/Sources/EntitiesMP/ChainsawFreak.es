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

342
%{
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/Enemies/ChainSawFreak/Freak.h"
#include "ModelsMP/Enemies/ChainSawFreak/Saw.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/EnemyRunInto";

%{
#define FREAK_SIZE 1.05f
  
// info structure
static EntityInfo eiChainsawFreak = {
  EIBT_FLESH, 350.0f,
  0.0f, 2.5f*FREAK_SIZE, 0.0f,     // source (eyes)
  0.0f, 1.5f*FREAK_SIZE, 0.0f,     // target (body)
};

#define HIT_DISTANCE 4.0f
%}


class CChainsawFreak : CEnemyRunInto {
name      "ChainsawFreak";
thumbnail "Thumbnails\\ChainsawFreak.tbn";

properties:
  1 BOOL  m_bRunAttack = FALSE,       // run attack (attack local)
  2 BOOL  m_bSawHit    = FALSE,         // close attack local
  3 CEntityPointer m_penLastTouched,  // last touched
  4 CSoundObject m_soFeet,            // for running sound
  5 BOOL  m_bRunSoundPlaying = FALSE,
  6 INDEX m_iRunType = 0,             // which running animation?

 10 BOOL  m_bAttacking = FALSE,
 11 FLOAT m_fSightSoundBegin = 0.0f,  // when sight sound was played

components:
  0 class   CLASS_BASE        "Classes\\EnemyRunInto.ecl",
  1 model   MODEL_FREAK       "ModelsMP\\Enemies\\ChainsawFreak\\Freak.mdl",
  2 model   MODEL_CHAINSAW    "ModelsMP\\Enemies\\ChainsawFreak\\Saw.mdl",
  3 texture TEXTURE_FREAK     "ModelsMP\\Enemies\\ChainsawFreak\\Freak.tex",
  4 texture TEXTURE_CHAINSAW  "ModelsMP\\Enemies\\ChainsawFreak\\Saw.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE       "ModelsMP\\Enemies\\ChainsawFreak\\Sounds\\Idle.wav",
 51 sound   SOUND_RUN        "ModelsMP\\Enemies\\ChainsawFreak\\Sounds\\Run.wav",
// 53 sound   SOUND_RUNATTACK  "ModelsMP\\Enemies\\ChainsawFreak\\Sounds\\RunAttack.wav",
 54 sound   SOUND_ATTACK     "ModelsMP\\Enemies\\ChainsawFreak\\Sounds\\Attack.wav",
 55 sound   SOUND_WOUND      "ModelsMP\\Enemies\\ChainsawFreak\\Sounds\\Wound.wav",
 56 sound   SOUND_DEATH      "ModelsMP\\Enemies\\ChainsawFreak\\Sounds\\Death.wav",
 57 sound   SOUND_SIGHT      "ModelsMP\\Enemies\\ChainsawFreak\\Sounds\\Sight.wav",

functions:
  
  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANSV("Chainsaw freak dismembered %s"), (const char *) strPlayerName);
    return str;
  }

  void Precache(void) {
    CEnemyBase::Precache();
    PrecacheSound(SOUND_IDLE     );
    PrecacheSound(SOUND_RUN      );
//    PrecacheSound(SOUND_RUNATTACK);
    PrecacheSound(SOUND_ATTACK   );
    PrecacheSound(SOUND_WOUND    );
    PrecacheSound(SOUND_DEATH    );
    PrecacheSound(SOUND_SIGHT    );    
  };

  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiChainsawFreak;
  };

  FLOAT GetCrushHealth(void)
  {
    return 60.0f;
  }

  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnm, "DataMP\\Messages\\Enemies\\ChainsawFreak.txt");
    return fnm;
  };

  // render particles
  /*void RenderParticles(void)
  {
    Particles_RunningDust(this);
  }*/

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // note: chainsawfreaks can't hurt each others
    if (!IsOfClass(penInflictor, "ChainsawFreak")) {
      CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };

  void AdjustDifficulty(void)
  {
    // chainsaw freak must not change his speed at different difficulties
  }

  // death
  INDEX AnimForDeath(void) {
    INDEX iAnim;
    if (en_vCurrentTranslationAbsolute.Length()>5.0f) {
      iAnim = FREAK_ANIM_DEATHRUN;
    } else {
      iAnim = FREAK_ANIM_DEATHSTAND;
    }
    ChangeCollisionBoxIndexWhenPossible(FREAK_COLLISION_BOX_DEATHRUN);
    StartModelAnim(iAnim, 0);
    m_bAttacking = FALSE;
    DeactivateRunningSound();
    return iAnim;
  };

  FLOAT WaitForDust(FLOAT3D &vStretch) {
    if(GetModelObject()->GetAnim()==FREAK_ANIM_DEATHRUN)
    {
      vStretch=FLOAT3D(1,1,2)*1.0f;
      return 0.65f;
    }
    else if(GetModelObject()->GetAnim()==FREAK_ANIM_DEATHSTAND)
    {
      vStretch=FLOAT3D(1,1,2)*1.5f;
      return 0.72f;
    }
    return -1.0f;
  };

  void DeathNotify() {
    ChangeCollisionBoxIndexWhenPossible(FREAK_COLLISION_BOX_DEATH);
    SetCollisionFlags(ECF_MODEL);
  };

  // virtual anim functions
  void StandingAnim(void) {
    StartModelAnim(FREAK_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);
    //DeactivateRunningSound();
  };
  void WalkingAnim(void) {
    StartModelAnim(FREAK_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
    //DeactivateRunningSound();
  };
  void RunningAnim(void) {
    switch(m_iRunType)
    {
    case 0:
      StartModelAnim(FREAK_ANIM_ATTACKRUN, AOF_LOOPING|AOF_NORESTART);
      break;
    case 1:
      StartModelAnim(FREAK_ANIM_ATTACKRUNFAR, AOF_LOOPING|AOF_NORESTART);
      break;
    case 2:
      StartModelAnim(FREAK_ANIM_ATTACKCHARGE, AOF_LOOPING|AOF_NORESTART);
      break;
    default:
      ASSERTALWAYS("Unknown Chainsaw freak run type!");
    }
    //ActivateRunningSound();
  };
  void ChargeAnim(void) {
    StartModelAnim(FREAK_ANIM_RUNSLASHING, AOF_LOOPING|AOF_NORESTART);
  };
  void RotatingAnim(void) {
    m_iRunType = IRnd()%3; 
    StartModelAnim(FREAK_ANIM_ATTACKSTART, AOF_LOOPING|AOF_NORESTART);
  };

  // virtual sound functions
  void IdleSound(void) {
    PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
  };
  /*void SightSound(void) {
    PlaySound(m_soFeet, SOUND_SIGHT, SOF_3D|SOF_SMOOTHCHANGE);
  };*/
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
 *                      ATTACK FUNCTIONS                    *
 ************************************************************/
  // touched another live entity
  void LiveEntityTouched(ETouch etouch) {
    if (m_penLastTouched!=etouch.penOther || _pTimer->CurrentTick()>=m_fLastTouchedTime+0.25f) {
      // hit angle
      FLOAT3D vDirection = en_vCurrentTranslationAbsolute;
      vDirection.Normalize();
      ANGLE aHitAngle = FLOAT3D(etouch.plCollision)%vDirection;
      // only hit target in front of you
      if (aHitAngle < 0.0f) {
        // increase mass - only if not another bull
        if (!IsOfSameClass(this, etouch.penOther)) {
          IncreaseKickedMass(etouch.penOther);
        }
        PlaySound(m_soSound, SOUND_ATTACK, SOF_3D);
        // store last touched
        m_penLastTouched = etouch.penOther;
        m_fLastTouchedTime = _pTimer->CurrentTick();
        // damage
        FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
        vDirection.Normalize();
        InflictDirectDamage(etouch.penOther, this, DMT_CHAINSAW, -aHitAngle*40.0f,
          FLOAT3D(0, 0, 0), vDirection);
        // kick touched entity
        FLOAT3D vSpeed = -FLOAT3D(etouch.plCollision);
        vSpeed = vSpeed*10.0f;
        const FLOATmatrix3D &m = GetRotationMatrix();
        FLOAT3D vSpeedRel = vSpeed*!m;
        if (vSpeedRel(1)<-0.1f) {
          vSpeedRel(1)-=5.0f;
        } else {
          vSpeedRel(1)+=5.0f;
        }
        vSpeedRel(2)=5.0f;

        vSpeed = vSpeedRel*m;
        KickEntity(etouch.penOther, vSpeed);
      }
    }
  };

  // touched entity with higher mass
  BOOL HigherMass(void) {
    return (m_fMassKicked > 500.0f);
  };

  // adjust sound and watcher parameters here if needed
  void EnemyPostInit(void) 
  {
    // set sound default parameters
    m_soFeet.Set3DParameters(500.0f, 50.0f, 1.0f, 1.0f);
    m_bRunSoundPlaying = FALSE;
    m_soSound.Set3DParameters(160.0f, 50.0f, 1.0f, 1.0f);
  };

  void PreMoving() {
    if (!m_bRunSoundPlaying && _pTimer->CurrentTick()>m_fSightSoundBegin+2.0f && m_bAttacking)
    {
      ActivateRunningSound();
    }
    CEnemyBase::PreMoving();
  };

procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  // hit enemy
  Hit(EVoid) : CEnemyBase::Hit {
    if (CalcDist(m_penEnemy) < HIT_DISTANCE) {
      // attack with saw
      StartModelAnim(FREAK_ANIM_ATTACKSLASH, 0);
      // jump
      FLOAT3D vDir = (m_penEnemy->GetPlacement().pl_PositionVector -
                       GetPlacement().pl_PositionVector).Normalize();
      vDir *= !GetRotationMatrix();
      vDir *= m_fCloseRunSpeed*1.5f;
      vDir(2) = 1.0f;
      
      GiveImpulseTranslationRelative(vDir);
      
      //DeactivateRunningSound();
      m_bSawHit = FALSE;
      autowait(0.4f);
      PlaySound(m_soSound, SOUND_ATTACK, SOF_3D);
      if (CalcDist(m_penEnemy) < HIT_DISTANCE) { m_bSawHit = TRUE; }
      autowait(0.1f);
      if (CalcDist(m_penEnemy) < HIT_DISTANCE) { m_bSawHit = TRUE; }
      autowait(0.1f);
      if (CalcDist(m_penEnemy) < HIT_DISTANCE) { m_bSawHit = TRUE; }
      if (m_bSawHit) {
        FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
        vDirection.Normalize();
        InflictDirectDamage(m_penEnemy, this, DMT_CHAINSAW, 20.0f, FLOAT3D(0, 0, 0), vDirection);
        
        vDirection = vDirection * 10.0f;
        //GetPitchDirection(AngleDeg(90.0f), vSpeed);
        FLOATmatrix3D mDirection;
        MakeRotationMatrixFast(mDirection, ANGLE3D(0.0f, 30.0f, 0.0f));
        vDirection = vDirection * mDirection;
        KickEntity(m_penEnemy, vDirection);
      }
      autowait(0.6f);
    }
    
    // run to enemy
    m_fShootTime = _pTimer->CurrentTick() + 0.5f;
    return EReturn();
  };

  AttackEnemy(EVoid) : CEnemyBase::AttackEnemy {
    m_bAttacking = TRUE;
    PlaySound(m_soSound, SOUND_SIGHT, SOF_3D|SOF_SMOOTHCHANGE);
    m_fSightSoundBegin = _pTimer->CurrentTick();
    jump CEnemyBase::AttackEnemy();
  }

  BeIdle(EVoid) : CEnemyBase::BeIdle {
    m_bAttacking = FALSE;
    DeactivateRunningSound();
    jump CEnemyBase::BeIdle();
  }


  PostRunAwayFromEnemy(EVoid) : CEnemyRunInto::PostRunAwayFromEnemy {
    StartModelAnim(FREAK_ANIM_ATTACKRUNFAR, 0);
    autowait(0.25f);
    SetDesiredTranslation(FLOAT3D(0,0,0));
    StartModelAnim(FREAK_ANIM_IDLE, 0);
    autowait(0.25f);
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
    SetHealth(175.0f);
    m_fMaxHealth = 175.0f;
    en_fDensity = 2000.0f;
    // set your appearance
    SetModel(MODEL_FREAK);
    SetModelMainTexture(TEXTURE_FREAK);
    AddAttachment(FREAK_ATTACHMENT_CHAINSAW, MODEL_CHAINSAW, TEXTURE_CHAINSAW);
    StandingAnim();
    // setup moving speed
    m_fWalkSpeed = FRnd() + 2.5f;
    m_aWalkRotateSpeed = AngleDeg(FRnd()*25.0f + 45.0f);
    m_fAttackRunSpeed = FRnd()*2.0f + 13.0f;
    m_fAttackRotateRunInto = AngleDeg(FRnd()*30 + 50.0f);
    m_aAttackRotateSpeed = m_fAttackRotateRunInto;
    m_fCloseRunSpeed = FRnd() + 10.5f;
    m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 250.0f);
    // setup attack distances
    m_fAttackDistance = 50.0f;
    m_fCloseDistance = 7.0f;
    m_fStopDistance = 0.0f;
    m_fAttackFireTime = 0.05f;
    m_fCloseFireTime = 1.0f;
    m_fIgnoreRange = 150.0f;
    // damage/explode properties6
    m_fBlowUpAmount = 1E10f;
    m_fBodyParts = 6;
    m_fDamageWounded = 100000.0f;
    m_iScore = 1500;
    if (m_fStepHeight==-1) {
      m_fStepHeight = 4.0f;
    }
    m_fStopApproachDistance = 0.0f;
    ASSERT(m_fStopApproachDistance<m_fCloseDistance);
    m_bUseChargeAnimation = TRUE;
    m_fChargeDistance = 20.0f;
    m_fInertionRunTime = 0.15f;
    m_iRunType = IRnd()%3;    

    GetModelObject()->StretchModel(FLOAT3D(FREAK_SIZE, FREAK_SIZE, FREAK_SIZE));
    ModelChangeNotify();

    m_bAttacking = FALSE;

    // continue behavior in base class
    jump CEnemyRunInto::MainLoop();
  };
};
