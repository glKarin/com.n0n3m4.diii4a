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

336

%{
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/Enemies/Demon/Demon.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/BasicEffects";


%{
#define REMINDER_DEATTACH_FIREBALL 666
#define CLOSE_ATTACK_RANGE 10.0f
#define DEMON_STRETCH 2.5f
FLOAT3D vFireballLaunchPos = (FLOAT3D(0.06f, 2.6f, 0.15f)*DEMON_STRETCH);
static int _tmLastStandingAnim = 0;

// info structure
static EntityInfo eiDemon = {
  EIBT_FLESH, 1600.0f,
  0.0f, 2.0f, 0.0f,     // source (eyes)
  0.0f, 1.5f, 0.0f,     // target (body)
};
%}

class CDemon : CEnemyBase {
name      "Demon";
thumbnail "Thumbnails\\Demon.tbn";

properties:
  2 INDEX m_iCounter = 0,
  3 CEntityPointer m_penFireFX,

components:
  0 class   CLASS_BASE          "Classes\\EnemyBase.ecl",
  1 class   CLASS_PROJECTILE    "Classes\\Projectile.ecl",
  2 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",

 10 model   MODEL_DEMON         "ModelsMP\\Enemies\\Demon\\Demon.mdl",
 11 texture TEXTURE_DEMON       "ModelsMP\\Enemies\\Demon\\Demon.tex",
 15 model   MODEL_FIREBALL      "ModelsMP\\Enemies\\Demon\\Projectile\\Projectile.mdl",
 16 texture TEXTURE_FIREBALL    "ModelsMP\\Enemies\\Demon\\Projectile\\Projectile.tex",

 // ************** SOUNDS **************
 50 sound   SOUND_IDLE      "ModelsMP\\Enemies\\Demon\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT     "ModelsMP\\Enemies\\Demon\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND     "ModelsMP\\Enemies\\Demon\\Sounds\\Wound.wav",
 55 sound   SOUND_DEATH     "ModelsMP\\Enemies\\Demon\\Sounds\\Death.wav",
 57 sound   SOUND_CAST      "ModelsMP\\Enemies\\Demon\\Sounds\\Cast.wav",

functions:
 
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    // when the shooting of projectile is over, this event comes
    // to make sure we deattach the projectile attachment (in case
    // the shooting was interrupted
    if (ee.ee_slEvent==EVENTCODE_EReminder)
    {
      EReminder eReminder = ((EReminder &) ee);
      if (eReminder.iValue==REMINDER_DEATTACH_FIREBALL)
      {
        RemoveAttachment(DEMON_ATTACHMENT_FIREBALL);
      }
      return TRUE;
    }
    return CEnemyBase::HandleEvent(ee);
  }

  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANSV("A Demon executed %s"), (const char *) strPlayerName);
    return str;
  }
  
  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnmDemon, "DataMP\\Messages\\Enemies\\Demon.txt");
    return fnmDemon;
  }
  
  void Precache(void) {
    CEnemyBase::Precache();
    PrecacheSound(SOUND_IDLE );
    PrecacheSound(SOUND_SIGHT);
    PrecacheSound(SOUND_WOUND);
    PrecacheSound(SOUND_DEATH);
    PrecacheSound(SOUND_CAST);
    PrecacheModel(MODEL_DEMON);
    PrecacheTexture(TEXTURE_DEMON);
    PrecacheModel(MODEL_FIREBALL);
    PrecacheTexture(TEXTURE_FIREBALL);
    PrecacheClass(CLASS_PROJECTILE, PRT_BEAST_PROJECTILE);
  };

  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiDemon;
  };

  BOOL ForcesCannonballToExplode(void)
  {
    return TRUE;
  }

  /*FLOAT GetCrushHealth(void)
  {
    if (m_bcType == BT_BIG) {
      return 100.0f;
    }
    return 0.0f;
  }*/

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // take less damage from heavy bullets (e.g. sniper)
    if(dmtType==DMT_BULLET && fDamageAmmount>100.0f)
    {
      fDamageAmmount*=0.5f;
    }

    // can't harm own class
    if (!IsOfClass(penInflictor, "Demon")) {
      CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };


  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    RemoveAttachment(DEMON_ATTACHMENT_FIREBALL);
    StartModelAnim(DEMON_ANIM_WOUND, 0);
    return DEMON_ANIM_WOUND;
  };

  // death
  INDEX AnimForDeath(void) {
    if( m_penFireFX != NULL)
    {
      m_penFireFX->SendEvent(EStop());
      m_penFireFX = NULL;
    }

    RemoveAttachment(DEMON_ATTACHMENT_FIREBALL);
    StartModelAnim(DEMON_ANIM_DEATHFORWARD, 0);
    return DEMON_ANIM_DEATHFORWARD;
  };

  FLOAT WaitForDust(FLOAT3D &vStretch)
  {
    vStretch=FLOAT3D(1,1,2)*3.0f;
    return 1.1f;
  };

  void DeathNotify(void) {
    ChangeCollisionBoxIndexWhenPossible(DEMON_COLLISION_BOX_DEATH);
    en_fDensity = 500.0f;
  };

  // virtual anim functions
  void StandingAnim(void) {
    //_tmLastStandingAnim = _pTimer->CurrentTick();
    StartModelAnim(DEMON_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);
  };

  void WalkingAnim(void) {
    /*if(_pTimer->CurrentTick()>=_tmLastStandingAnim-_pTimer->TickQuantum &&
       _pTimer->CurrentTick()<=_tmLastStandingAnim+_pTimer->TickQuantum)
    {
      BREAKPOINT;
    }*/
    RunningAnim();
  };

  void RunningAnim(void) {
    StartModelAnim(DEMON_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
  };
  void RotatingAnim(void) {
    StartModelAnim(DEMON_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
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


  // adjust sound and watcher parameters here if needed
  void EnemyPostInit(void) 
  {
    m_soSound.Set3DParameters(160.0f, 50.0f, 2.0f, 1.0f);
  };

procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  Fire(EVoid) : CEnemyBase::Fire
  {
    
    // SetDesiredTranslation???
    if (m_fMoveSpeed>0.0f) {
      SetDesiredTranslation(FLOAT3D(0.0f, 0.0f, -m_fMoveSpeed));
    }
    
    //StartModelAnim(DEMON_ANIM_ATTACK, AOF_SMOOTHCHANGE);
    StartModelAnim(DEMON_ANIM_ATTACK, 0);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;    
    
    SetDesiredTranslation(FLOAT3D(0.0f, 0.0f, 0.0f));
    
    PlaySound(m_soSound, SOUND_CAST, SOF_3D);
    SpawnReminder(this, 3.0f, REMINDER_DEATTACH_FIREBALL);

    autowait(1.0f);

    // spawn particle effect
    CPlacement3D plFX=GetPlacement();
    const FLOATmatrix3D &m = GetRotationMatrix();
    plFX.pl_PositionVector=plFX.pl_PositionVector+vFireballLaunchPos*m;
    ESpawnEffect ese;
    ese.colMuliplier = C_WHITE|CT_OPAQUE;
    ese.betType = BET_COLLECT_ENERGY;
    ese.vStretch = FLOAT3D(1.0f, 1.0f, 1.0f);
    m_penFireFX = CreateEntity(plFX, CLASS_BASIC_EFFECT);
    m_penFireFX->Initialize(ese);

    autowait(1.4f);
    
    AddAttachment(DEMON_ATTACHMENT_FIREBALL, MODEL_FIREBALL, TEXTURE_FIREBALL);
    CModelObject *pmoFire = &GetModelObject()->GetAttachmentModel(DEMON_ATTACHMENT_FIREBALL)->amo_moModelObject;
    pmoFire->StretchModel(FLOAT3D(DEMON_STRETCH, DEMON_STRETCH, DEMON_STRETCH));
    autowait(2.94f-2.4f);
    
    RemoveAttachment(DEMON_ATTACHMENT_FIREBALL);
    MaybeSwitchToAnotherPlayer();

    if (IsVisible(m_penEnemy)) {
      ShootProjectile(PRT_DEMON_FIREBALL, vFireballLaunchPos, ANGLE3D(0.0f, 0.0f, 0.0f));
    }
    else {
      ShootProjectileAt(m_vPlayerSpotted, PRT_DEMON_FIREBALL, vFireballLaunchPos, ANGLE3D(0.0f, 0.0f, 0.0f));
    }
      
    autowait(1.0f);
    
    return EReturn();
  };

  Hit(EVoid) : CEnemyBase::Hit {
    // close attack
    if (CalcDist(m_penEnemy) < 6.0f) {
      StartModelAnim(DEMON_ANIM_WOUND, 0);
      autowait(0.45f);
      PlaySound(m_soSound, SOUND_WOUND, SOF_3D);
      if (CalcDist(m_penEnemy) < CLOSE_ATTACK_RANGE
        && IsInPlaneFrustum(m_penEnemy, CosFast(60.0f)))
      {
        FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
        vDirection.Normalize();
        InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 50.0f, FLOAT3D(0, 0, 0), vDirection);
      }
      autowait(1.5f);
      MaybeSwitchToAnotherPlayer();
    } else {
      // run to enemy
      m_fShootTime = _pTimer->CurrentTick() + 0.5f;
    }
    return EReturn();
  }


/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);

    en_fDensity = 1100.0f;
    // set your appearance
    SetModel(MODEL_DEMON);
    StandingAnim();
    // setup moving speed
    m_fWalkSpeed = FRnd()/1.0f + 12.0f;
    m_aWalkRotateSpeed = AngleDeg(FRnd()*20.0f + 50.0f);
    m_fCloseRunSpeed = FRnd()/1.0f + 13.0f;
    m_aCloseRotateSpeed = AngleDeg(FRnd()*100 + 900.0f);
    m_fAttackRunSpeed = FRnd()/1.0f + 9.0f;
    m_aAttackRotateSpeed = AngleDeg(FRnd()*100.0f + 900.0f);
    // setup attack distances
    m_fAttackDistance = 650.0f;
    m_fCloseDistance = 12.0f;
    m_fStopDistance = 0.0f;
    m_fAttackFireTime = 5.0f;
    m_fCloseFireTime = 1.0f;
    m_fIgnoreRange = 800.0f;
    m_fStopDistance = 5.0f;
    m_tmGiveUp = Max(m_tmGiveUp, 10.0f);

    // damage/explode properties
    SetHealth(500.0f);
    m_fMaxHealth = GetHealth();
    SetModelMainTexture(TEXTURE_DEMON);
    m_fBlowUpAmount = 10000.0f;
    m_fBodyParts = 4;
    m_fDamageWounded = 1000.0f;
    m_iScore = 5000;
    m_fLockOnEnemyTime = 3.0f;

    // set stretch factor
    GetModelObject()->StretchModel(FLOAT3D(4.2f, 4.2f, 4.2f));
    ModelChangeNotify();
    
    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
