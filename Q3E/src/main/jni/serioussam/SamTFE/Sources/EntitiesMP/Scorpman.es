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

306
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Enemies/SCORPMAN/scorpman.h"
#include "Models/Enemies/SCORPMAN/Gun.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/Bullet";
uses "EntitiesMP/Reminder";

enum ScorpmanType {
  0 SMT_SOLDIER    "Soldier",
  1 SMT_GENERAL    "General",
  2 SMT_MONSTER    "Obsolete",
};


%{
#define GUN_X  0.375f
#define GUN_Y  0.6f
#define GUN_Z -1.85f
#define STRETCH_SOLDIER 2
#define STRETCH_GENERAL 3
#define STRETCH_MONSTER 4
// info structure
static EntityInfo eiScorpman = {
  EIBT_FLESH, 1000.0f,
  0, 1.6f*STRETCH_SOLDIER, 0,     // source (eyes)
  0.0f, 1.0f*STRETCH_SOLDIER, 0.0f,     // target (body)
};

static EntityInfo eiScorpmanGeneral = {
  EIBT_FLESH, 1500.0f,
  0, 1.6f*STRETCH_GENERAL, 0,     // source (eyes)
  0.0f, 1.0f*STRETCH_GENERAL, 0.0f,     // target (body)
};

static EntityInfo eiScorpmanMonster = {
  EIBT_FLESH, 2000.0f,
  0, 1.6f*STRETCH_MONSTER, 0,     // source (eyes)
  0.0f, 1.0f*STRETCH_MONSTER, 0.0f,     // target (body)
};
#define LIGHT_ANIM_FIRE 3
#define LIGHT_ANIM_NONE 5
%}


class CScorpman : CEnemyBase {
name      "Scorpman";
thumbnail "Thumbnails\\Scorpman.tbn";

properties:
  1 enum ScorpmanType m_smtType "Type" 'Y' = SMT_SOLDIER,
  2 INDEX m_bFireBulletCount = 0,       // fire bullet binary divider
  3 INDEX m_iSpawnEffect = 0,           // counter for spawn effect every 'x' times
  4 FLOAT m_fFireTime = 0.0f,           // time to fire bullets
  5 CAnimObject m_aoLightAnimation,     // light animation object
  6 BOOL m_bSleeping "Sleeping" 'S' = FALSE,  // set to make scorpman sleep initally
  
{
  CEntity *penBullet;     // bullet
  CLightSource m_lsLightSource;
}

components:
  0 class   CLASS_BASE        "Classes\\EnemyBase.ecl",
  1 class   CLASS_BULLET      "Classes\\Bullet.ecl",
  5 model   MODEL_SCORPMAN    "Models\\Enemies\\Scorpman\\Scorpman.mdl",
  6 texture TEXTURE_SOLDIER   "Models\\Enemies\\Scorpman\\Soldier.tex",
  7 texture TEXTURE_GENERAL   "Models\\Enemies\\Scorpman\\General.tex",
//  8 texture TEXTURE_MONSTER   "Models\\Enemies\\Scorpman\\Monster.tex",
 12 texture TEXTURE_SPECULAR  "Models\\SpecularTextures\\Medium.tex",
  9 model   MODEL_GUN         "Models\\Enemies\\Scorpman\\Gun.mdl",
 10 model   MODEL_FLARE       "Models\\Enemies\\Scorpman\\Flare.mdl",
 11 texture TEXTURE_GUN       "Models\\Enemies\\Scorpman\\Gun.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE      "Models\\Enemies\\Scorpman\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT     "Models\\Enemies\\Scorpman\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND     "Models\\Enemies\\Scorpman\\Sounds\\Wound.wav",
 53 sound   SOUND_FIRE      "Models\\Enemies\\Scorpman\\Sounds\\Fire.wav",
 54 sound   SOUND_KICK      "Models\\Enemies\\Scorpman\\Sounds\\Kick.wav",
 55 sound   SOUND_DEATH     "Models\\Enemies\\Scorpman\\Sounds\\Death.wav",

functions:
  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    if (eDeath.eLastDamage.dmtType==DMT_CLOSERANGE) {
      str.PrintF(TRANSV("%s was stabbed by an Arachnoid"), (const char *) strPlayerName);
    } else {
      str.PrintF(TRANSV("An Arachnoid poured lead into %s"), (const char *) strPlayerName);
    }
    return str;
  }
  void Precache(void) {
    CEnemyBase::Precache();
    PrecacheModel(MODEL_FLARE);
    PrecacheSound(SOUND_IDLE );
    PrecacheSound(SOUND_SIGHT);
    PrecacheSound(SOUND_WOUND);
    PrecacheSound(SOUND_FIRE );
    PrecacheSound(SOUND_KICK );
    PrecacheSound(SOUND_DEATH);
  };

  /* Read from stream. */
  void Read_t( CTStream *istr) { // throw char *
    CEnemyBase::Read_t(istr);

    // setup light source
    SetupLightSource();
  }

  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    CEnemyBase::FillEntityStatistics(pes);
    switch(m_smtType) {
    case SMT_MONSTER: { pes->es_strName+=" Monster"; } break;
    case SMT_GENERAL: { pes->es_strName+=" General"; } break;
    case SMT_SOLDIER: { pes->es_strName+=" Soldier"; } break;
    }
    return TRUE;
  }

  virtual const CTFileName &GetComputerMessageName(void) const {
    //static DECLARE_CTFILENAME(fnmMonster, "Data\\Messages\\Enemies\\ScorpmanMonster.txt");
    static DECLARE_CTFILENAME(fnmGeneral, "Data\\Messages\\Enemies\\ScorpmanGeneral.txt");
    static DECLARE_CTFILENAME(fnmSoldier, "Data\\Messages\\Enemies\\ScorpmanSoldier.txt");
    switch(m_smtType) {
    default: ASSERT(FALSE);
    case SMT_MONSTER: //return fnmMonster;
    case SMT_GENERAL: return fnmGeneral;
    case SMT_SOLDIER: return fnmSoldier;
    }
  };

  /* Get static light source information. */
  CLightSource *GetLightSource(void) {
    if (!IsPredictor()) {
      return &m_lsLightSource;
    } else {
      return NULL;
    }
  }

  BOOL ForcesCannonballToExplode(void)
  {
    if (m_smtType!=SMT_SOLDIER) {
      return TRUE;
    }
    return CEnemyBase::ForcesCannonballToExplode();
  }

  // Setup light source
  void SetupLightSource(void) {
    // setup light source
    CLightSource lsNew;
    lsNew.ls_ulFlags = LSF_NONPERSISTENT|LSF_DYNAMIC;
    lsNew.ls_rHotSpot = 2.0f;
    lsNew.ls_rFallOff = 8.0f;
    lsNew.ls_colColor = RGBToColor(128, 128, 128);
    lsNew.ls_plftLensFlare = NULL;
    lsNew.ls_ubPolygonalMask = 0;
    lsNew.ls_paoLightAnimation = &m_aoLightAnimation;

    m_lsLightSource.ls_penEntity = this;
    m_lsLightSource.SetLightSource(lsNew);
  }
  // play light animation
  void PlayLightAnim(INDEX iAnim, ULONG ulFlags) {
    if (m_aoLightAnimation.GetData()!=NULL) {
      m_aoLightAnimation.PlayAnim(iAnim, ulFlags);
    }
  };

  // fire minigun on/off
  void MinigunOn(void)
  {
    PlayLightAnim(LIGHT_ANIM_FIRE, AOF_LOOPING);
    CModelObject *pmoGun = &GetModelObject()->GetAttachmentModel(SCORPMAN_ATTACHMENT_MINIGUN)->
      amo_moModelObject;
    pmoGun->PlayAnim(GUN_ANIM_FIRE, AOF_LOOPING);
    AddAttachmentToModel(this, *pmoGun, GUN_ATTACHMENT_FLAME, MODEL_FLARE, TEXTURE_GUN, 0, 0, 0);
    switch (m_smtType) {
      case SMT_SOLDIER: pmoGun->StretchModel(FLOAT3D(2.0f, 2.0f, 2.0f)); break;
      case SMT_GENERAL: pmoGun->StretchModel(FLOAT3D(3.0f, 3.0f, 3.0f)); break;
      case SMT_MONSTER: pmoGun->StretchModel(FLOAT3D(4.0f, 4.0f, 4.0f)); break;
    }
  }
  void MinigunOff(void)
  {
    PlayLightAnim(LIGHT_ANIM_NONE, 0);
    CModelObject *pmoGun = &GetModelObject()->GetAttachmentModel(SCORPMAN_ATTACHMENT_MINIGUN)->
      amo_moModelObject;
    pmoGun->PlayAnim(GUN_ANIM_IDLE, AOF_LOOPING);
    pmoGun->RemoveAttachmentModel(GUN_ATTACHMENT_FLAME);
  }
  /* Entity info */
  void *GetEntityInfo(void) {
    if (m_smtType == SMT_MONSTER) {
      return &eiScorpmanMonster;
    } else if (m_smtType == SMT_GENERAL) {
      return &eiScorpmanGeneral;
    } else {
      return &eiScorpman;
    }
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // scorpman can't harm scorpman
    if (!IsOfClass(penInflictor, "Scorpman")) {
      CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };


  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    switch (IRnd()%3) {
      case 0: iAnim = SCORPMAN_ANIM_WOUND01; break;
      case 1: iAnim = SCORPMAN_ANIM_WOUND02; break;
      case 2: iAnim = SCORPMAN_ANIM_WOUND03; break;
      default: iAnim = SCORPMAN_ANIM_WOUND01; //ASSERTALWAYS("Scorpman unknown damage");
    }
    StartModelAnim(iAnim, 0);
    MinigunOff();
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    StartModelAnim(SCORPMAN_ANIM_DEATH, 0);
    MinigunOff();
    return SCORPMAN_ANIM_DEATH;
  };

  FLOAT WaitForDust(FLOAT3D &vStretch) {
    if(GetModelObject()->GetAnim()==SCORPMAN_ANIM_DEATH)
    {
      vStretch=FLOAT3D(1,1,1)*1.5f;
      return 1.3f;
    }
    return -1.0f;
  };

  void DeathNotify(void) {
    ChangeCollisionBoxIndexWhenPossible(SCORPMAN_COLLISION_BOX_DEATH);
    SetCollisionFlags(ECF_MODEL);
  };

  // virtual anim functions
  void StandingAnim(void) {
    StartModelAnim(SCORPMAN_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);
  };
  void WalkingAnim(void) {
    StartModelAnim(SCORPMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
  };
  void RunningAnim(void) {
    StartModelAnim(SCORPMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
  };
  void RotatingAnim(void) {
    StartModelAnim(SCORPMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
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
 *                   FIRE BULLET / RAIL                     *
 ************************************************************/
  BOOL CanFireAtPlayer(void)
  {
    // get ray source and target
    FLOAT3D vSource, vTarget;
    GetPositionCastRay(this, m_penEnemy, vSource, vTarget);

    // bullet start position
    CPlacement3D plBullet;
    plBullet.pl_OrientationAngle = ANGLE3D(0,0,0);
    plBullet.pl_PositionVector = FLOAT3D(GUN_X, GUN_Y, 0);
    // offset are changed according to stretch factor
    if (m_smtType == SMT_MONSTER) {
      plBullet.pl_PositionVector*=STRETCH_MONSTER;
    } else if (m_smtType == SMT_GENERAL) {
      plBullet.pl_PositionVector*=STRETCH_GENERAL;
    } else {
      plBullet.pl_PositionVector*=STRETCH_SOLDIER;
    }
    plBullet.RelativeToAbsolute(GetPlacement());
    vSource = plBullet.pl_PositionVector;

    // cast the ray
    CCastRay crRay(this, vSource, vTarget);
    crRay.cr_ttHitModels = CCastRay::TT_NONE;     // check for brushes only
    crRay.cr_bHitTranslucentPortals = FALSE;
    en_pwoWorld->CastRay(crRay);

    // if hit nothing (no brush) the entity can be seen
    return (crRay.cr_penHit==NULL);     
  }

  void PrepareBullet(FLOAT fDamage) {
    // bullet start position
    CPlacement3D plBullet;
    plBullet.pl_OrientationAngle = ANGLE3D(0,0,0);
    plBullet.pl_PositionVector = FLOAT3D(GUN_X, GUN_Y, 0);
    // offset are changed according to stretch factor
    if (m_smtType == SMT_MONSTER) {
      plBullet.pl_PositionVector*=STRETCH_MONSTER;
    } else if (m_smtType == SMT_GENERAL) {
      plBullet.pl_PositionVector*=STRETCH_GENERAL;
    } else {
      plBullet.pl_PositionVector*=STRETCH_SOLDIER;
    }
    plBullet.RelativeToAbsolute(GetPlacement());
    // create bullet
    penBullet = CreateEntity(plBullet, CLASS_BULLET);
    // init bullet
    EBulletInit eInit;
    eInit.penOwner = this;
    eInit.fDamage = fDamage;
    penBullet->Initialize(eInit);
  };

  // fire bullet
  void FireBullet(void) {
    // binary divide counter
    m_bFireBulletCount++;
    if (m_bFireBulletCount>1) { m_bFireBulletCount = 0; }
    if (m_bFireBulletCount==1) { return; }
    // bullet
    PrepareBullet(3.0f);
    ((CBullet&)*penBullet).CalcTarget(m_penEnemy, 250);
    ((CBullet&)*penBullet).CalcJitterTarget(10);
    ((CBullet&)*penBullet).LaunchBullet( TRUE, TRUE, TRUE);
    ((CBullet&)*penBullet).DestroyBullet();
  };


  // adjust sound and watcher parameters here if needed
  void EnemyPostInit(void) 
  {
    // set sound default parameters
    m_soSound.Set3DParameters(160.0f, 50.0f, 1.0f, 1.0f);
  };

procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  // shoot
  Fire(EVoid) : CEnemyBase::Fire{
    if (!CanFireAtPlayer()) {
      return EReturn();
    }

    // confused amount
    switch (m_smtType) {
      case SMT_MONSTER:
        m_fDamageConfused = 200;
        m_fFireTime = 8.0f;
        break;
      case SMT_GENERAL:
        m_fDamageConfused = 100;
        m_fFireTime = 4.0f;
        break;
      case SMT_SOLDIER:
        m_fDamageConfused = 50;
        m_fFireTime = 2.0f;
        break;
    }
    if (GetSP()->sp_gdGameDifficulty<=CSessionProperties::GD_EASY) {
      m_fFireTime *= 0.5f;
    }
    // to fire
    StartModelAnim(SCORPMAN_ANIM_STANDTOFIRE, 0);
    m_fLockOnEnemyTime = GetModelObject()->GetAnimLength(SCORPMAN_ANIM_STANDTOFIRE) + 0.5f + FRnd()/3;
    autocall CEnemyBase::LockOnEnemy() EReturn;

    // fire
    m_iSpawnEffect = 0;                         // effect every 'x' frames
    m_fFireTime += _pTimer->CurrentTick();
    m_bFireBulletCount = 0;
    PlaySound(m_soSound, SOUND_FIRE, SOF_3D|SOF_LOOP);
    MinigunOn();

    while (m_fFireTime > _pTimer->CurrentTick()) {
      m_fMoveFrequency = 0.1f;
      wait(m_fMoveFrequency) {
        on (EBegin) : {
          // make fuss
          AddToFuss();
          // fire bullet
          FireBullet();
          m_vDesiredPosition = m_penEnemy->GetPlacement().pl_PositionVector;
          // rotate to enemy
          if (!IsInPlaneFrustum(m_penEnemy, CosFast(5.0f))) {
            m_fMoveSpeed = 0.0f;
            m_aRotateSpeed = 4000.0f;
            StartModelAnim(SCORPMAN_ANIM_WALK_AND_FIRE, AOF_LOOPING|AOF_NORESTART);
          // stand in place
          } else {
            m_fMoveSpeed = 0.0f;
            m_aRotateSpeed = 0.0f;
            StartModelAnim(SCORPMAN_ANIM_FIRE_MINIGUN, AOF_LOOPING|AOF_NORESTART);
          }
          // adjust direction and speed
          SetDesiredMovement(); 
          resume;
        }
        on (ETimer) : { stop; }
      }
    }
    m_soSound.Stop();
    MinigunOff();
    // set next shoot time
    m_fShootTime = _pTimer->CurrentTick() + m_fAttackFireTime*(1.0f + FRnd()/3.0f);

    // from fire
    StartModelAnim(SCORPMAN_ANIM_FIRETOSTAND, 0);
    autowait(GetModelObject()->GetAnimLength(SCORPMAN_ANIM_FIRETOSTAND));

    MaybeSwitchToAnotherPlayer();

    // shoot completed
    return EReturn();
  };

  // hit enemy
  Hit(EVoid) : CEnemyBase::Hit {
    // close attack
    StartModelAnim(SCORPMAN_ANIM_SPIKEHIT, 0);
    autowait(0.5f);
    PlaySound(m_soSound, SOUND_KICK, SOF_3D);
    if (CalcDist(m_penEnemy) < m_fCloseDistance) {
      FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
      vDirection.Normalize();
      if (m_smtType == SMT_MONSTER) {
        InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 80.0f, FLOAT3D(0, 0, 0), vDirection);
      } else if (m_smtType == SMT_GENERAL) {
        InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 40.0f, FLOAT3D(0, 0, 0), vDirection);
      } else {
        InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 20.0f, FLOAT3D(0, 0, 0), vDirection);
      }
    }
    autowait(0.3f);
    MaybeSwitchToAnotherPlayer();
    return EReturn();
  };

  Sleep(EVoid)
  {
    // start sleeping anim
    StartModelAnim(SCORPMAN_ANIM_SLEEP, AOF_LOOPING);
    // repeat
    wait() {
      // if triggered
      on(ETrigger eTrigger) : {
        // remember enemy
        SetTargetSoft(eTrigger.penCaused);
        // wake up
        jump WakeUp();
      }
      // if damaged
      on(EDamage eDamage) : {
        // wake up
        jump WakeUp();
      }
      otherwise() : {
        resume;
      }
    }
  }

  WakeUp(EVoid)
  {
    // wakeup anim
    SightSound();
    StartModelAnim(SCORPMAN_ANIM_WAKEUP, 0);
    autowait(GetModelObject()->GetCurrentAnimLength());

    // trigger your target
    SendToTarget(m_penDeathTarget, m_eetDeathType);
    // proceed with normal functioning
    return EReturn();
  }

  // overridable called before main enemy loop actually begins
  PreMainLoop(EVoid) : CEnemyBase::PreMainLoop
  {
    // if sleeping
    if (m_bSleeping) {
      m_bSleeping = FALSE;
      // go to sleep until waken up
      wait() {
        on (EBegin) : {
          call Sleep();
        }
        on (EReturn) : {
          stop;
        };
        // if dead
        on(EDeath eDeath) : {
          // die
          jump CEnemyBase::Die(eDeath);
        }
      }
    }
    return EReturn();
  }

/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    if (m_smtType==SMT_MONSTER) {
      m_smtType=SMT_GENERAL;
    }

    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING|EPF_HASLUNGS);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    en_tmMaxHoldBreath = 25.0f;
    en_fDensity = 3000.0f;

    // set your appearance
    SetModel(MODEL_SCORPMAN);
    switch (m_smtType) {
      case SMT_SOLDIER:
        // set your texture
        SetModelMainTexture(TEXTURE_SOLDIER);
        SetModelSpecularTexture(TEXTURE_SPECULAR);
        SetHealth(300.0f);
        m_fMaxHealth = 300.0f;
        // damage/explode properties
        m_fDamageWounded = 200.0f;
        m_fBlowUpAmount = 1E10f;
        m_fBodyParts = 30;
        // setup attack distances
        m_fAttackDistance = 200.0f;
        m_fCloseDistance = 5.0f;
        m_fStopDistance = 4.5f;
        m_fAttackFireTime = 0.5f;
        m_fCloseFireTime = 1.0f;
        m_fIgnoreRange = 350.0f;
        m_iScore = 1000;
        break;

      case SMT_GENERAL:
        // set your texture
        SetModelMainTexture(TEXTURE_GENERAL);
        SetModelSpecularTexture(TEXTURE_SPECULAR);
        SetHealth(600.0f);
        m_fMaxHealth = 600.0f;
        // damage/explode properties
        m_fDamageWounded = 400.0f;
        m_fBlowUpAmount = 1E10f;
        m_fBodyParts = 30;
        // setup attack distances
        m_fAttackDistance = 200.0f;
        m_fCloseDistance = 5.0f;
        m_fStopDistance = 4.5f;
        m_fAttackFireTime = 2.0f;
        m_fCloseFireTime = 1.0f;
        m_fIgnoreRange = 350.0f;
        m_iScore = 5000;
        break;

      case SMT_MONSTER:
        // set your texture
        SetModelMainTexture(TEXTURE_GENERAL);
        SetModelSpecularTexture(TEXTURE_SPECULAR);
        SetHealth(1200.0f);
        m_fMaxHealth = 1200.0f;
        // damage/explode properties
        m_fDamageWounded = 800.0f;
        m_fBlowUpAmount = 1E10f;
        m_fBodyParts = 60;
        // setup attack distances
        m_fAttackDistance = 250.0f;
        m_fCloseDistance = 11.0f;
        m_fStopDistance = 9.0f;
        m_fAttackFireTime = 2.0f;
        m_fCloseFireTime = 1.0f;
        m_fIgnoreRange = 500.0f;
        m_iScore = 10000;
        break;
    }
    
    AddAttachment(SCORPMAN_ATTACHMENT_MINIGUN, MODEL_GUN, TEXTURE_GUN);

    // set stretch factors for height and width - MUST BE DONE BEFORE SETTING MODEL!
    switch (m_smtType) {
      case SMT_SOLDIER: GetModelObject()->StretchModel(FLOAT3D(1.0f, 1.0f, 1.0f)*STRETCH_SOLDIER); break;
      case SMT_GENERAL: GetModelObject()->StretchModel(FLOAT3D(1.0f, 1.0f, 1.0f)*STRETCH_GENERAL); break;
      case SMT_MONSTER: GetModelObject()->StretchModel(FLOAT3D(1.0f, 1.0f, 1.0f)*STRETCH_MONSTER); break;
    }

    ModelChangeNotify();

    // setup moving speed
    m_fWalkSpeed = FRnd() + 1.5f;
    m_aWalkRotateSpeed = AngleDeg(FRnd()*20.0f + 550.0f);
    m_fAttackRunSpeed = FRnd()*1.5f + 4.5f;
    m_aAttackRotateSpeed = AngleDeg(FRnd()*50.0f + 275.0f);
    m_fCloseRunSpeed = FRnd()*1.5f + 4.5f;
    m_aCloseRotateSpeed = AngleDeg(FRnd()*50.0f + 275.0f);

    // set stretch factors for height and width
    CEnemyBase::SizeModel();
    // setup light source
    SetupLightSource();
    // set light animation if available
    try {
      m_aoLightAnimation.SetData_t(CTFILENAME("Animations\\BasicEffects.ani"));
    } catch ( const char *strError) {
      WarningMessage(TRANS("Cannot load Animations\\BasicEffects.ani: %s"), strError);
    }
    MinigunOff();

    // continue behavior in base class
    jump CEnemyBase::MainLoop();

  };
};
