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

303
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Enemies/Headman/headman.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/BasicEffects";

enum HeadmanType {
  0 HDT_FIRECRACKER   "Fire Cracker",
  1 HDT_ROCKETMAN     "Rocketman",
  2 HDT_BOMBERMAN     "Bomberman",
  3 HDT_KAMIKAZE      "Kamikaze",
};


%{
// info structure
static EntityInfo eiHeadman = {
  EIBT_FLESH, 100.0f,
  0.0f, 1.9f, 0.0f,     // source (eyes)
  0.0f, 1.0f, 0.0f,     // target (body)
};

#define EXPLODE_KAMIKAZE   2.5f
#define BOMBERMAN_ANGLE (45.0f)
#define BOMBERMAN_LAUNCH (FLOAT3D(0.0f, 1.5f, 0.0f))
%}


class CHeadman: CEnemyBase {
name      "Headman";
thumbnail "Thumbnails\\Headman.tbn";

properties:
  1 enum HeadmanType m_hdtType "Type" 'Y' = HDT_FIRECRACKER,

  // class internal
  5 BOOL m_bExploded = FALSE,
  6 BOOL m_bAttackSound = FALSE,    // playing kamikaze yelling sound
  
components:
  1 class   CLASS_BASE            "Classes\\EnemyBase.ecl",
  2 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",
  3 class   CLASS_PROJECTILE      "Classes\\Projectile.ecl",

 10 model   MODEL_HEADMAN         "Models\\Enemies\\Headman\\Headman.mdl",
 11 model   MODEL_HEAD            "Models\\Enemies\\Headman\\Head.mdl",
 12 model   MODEL_FIRECRACKERHEAD "Models\\Enemies\\Headman\\FirecrackerHead.mdl",
 13 model   MODEL_CHAINSAW        "Models\\Enemies\\Headman\\ChainSaw.mdl",
 15 model   MODEL_ROCKETLAUNCHER  "Models\\Enemies\\Headman\\RocketLauncher.mdl",
 17 model   MODEL_BOMB            "Models\\Enemies\\Headman\\Projectile\\Bomb.mdl",

 20 texture TEXTURE_BOMBERMAN       "Models\\Enemies\\Headman\\Bomberman.tex",
 21 texture TEXTURE_FIRECRACKER     "Models\\Enemies\\Headman\\Firecracker.tex",
 22 texture TEXTURE_KAMIKAZE        "Models\\Enemies\\Headman\\Kamikaze.tex",
 23 texture TEXTURE_ROCKETMAN       "Models\\Enemies\\Headman\\Rocketman.tex",
 24 texture TEXTURE_HEAD            "Models\\Enemies\\Headman\\Head.tex",
 25 texture TEXTURE_FIRECRACKERHEAD "Models\\Enemies\\Headman\\FirecrackerHead.tex",
 26 texture TEXTURE_CHAINSAW        "Models\\Enemies\\Headman\\Chainsaw.tex",
 28 texture TEXTURE_ROCKETLAUNCHER  "Models\\Enemies\\Headman\\RocketLauncher.tex",
 29 texture TEXTURE_BOMB            "Models\\Enemies\\Headman\\Projectile\\Bomb.tex",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE              "Models\\Enemies\\Headman\\Sounds\\Idle.wav",
 51 sound   SOUND_IDLEKAMIKAZE      "Models\\Enemies\\Headman\\Sounds\\IdleKamikaze.wav",
 52 sound   SOUND_SIGHT             "Models\\Enemies\\Headman\\Sounds\\Sight.wav",
 53 sound   SOUND_WOUND             "Models\\Enemies\\Headman\\Sounds\\Wound.wav",
 54 sound   SOUND_FIREROCKETMAN     "Models\\Enemies\\Headman\\Sounds\\FireRocketman.wav",
 55 sound   SOUND_FIREFIRECRACKER   "Models\\Enemies\\Headman\\Sounds\\FireFirecracker.wav",
 56 sound   SOUND_FIREBOMBERMAN     "Models\\Enemies\\Headman\\Sounds\\FireBomberman.wav",
 57 sound   SOUND_ATTACKKAMIKAZE    "Models\\Enemies\\Headman\\Sounds\\AttackKamikaze.wav",
 58 sound   SOUND_DEATH             "Models\\Enemies\\Headman\\Sounds\\Death.wav",

 /*
 60 model     MODEL_HEADMAN_BODY   "Models\\Enemies\\Headman\\Debris\\Torso.mdl",
 61 model     MODEL_HEADMAN_HAND   "Models\\Enemies\\Headman\\Debris\\Arm.mdl",
 62 model     MODEL_HEADMAN_LEGS   "Models\\Enemies\\Headman\\Debris\\Leg.mdl",
 */

functions:
  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    if (eDeath.eLastDamage.dmtType==DMT_EXPLOSION) {
      if (m_hdtType==HDT_BOMBERMAN) {
        str.PrintF(TRANSV("%s was bombed by a Bomberman"), (const char *) strPlayerName);
      } else {
        str.PrintF(TRANSV("%s fell victim of a Kamikaze"), (const char *) strPlayerName);
      }
    } else if (m_hdtType==HDT_ROCKETMAN) {
      str.PrintF(TRANSV("A Rocketeer tickled %s to death"), (const char *) strPlayerName);
    } else if (m_hdtType==HDT_FIRECRACKER) {
      str.PrintF(TRANSV("A Firecracker tickled %s to death"), (const char *) strPlayerName);
    }
    return str;
  }

  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiHeadman;
  };

  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnmRocketman,   "Data\\Messages\\Enemies\\Rocketman.txt");
    static DECLARE_CTFILENAME(fnmFirecracker, "Data\\Messages\\Enemies\\Firecracker.txt");
    static DECLARE_CTFILENAME(fnmBomberman,   "Data\\Messages\\Enemies\\Bomberman.txt");
    static DECLARE_CTFILENAME(fnmKamikaze,    "Data\\Messages\\Enemies\\Kamikaze.txt");
    switch(m_hdtType) {
    default: ASSERT(FALSE);
    case HDT_ROCKETMAN:   return fnmRocketman;
    case HDT_FIRECRACKER: return fnmFirecracker;
    case HDT_BOMBERMAN:   return fnmBomberman;
    case HDT_KAMIKAZE:    return fnmKamikaze;
    }
  };

  void Precache(void) {
    CEnemyBase::Precache();
    PrecacheSound(SOUND_IDLE);
    PrecacheSound(SOUND_SIGHT);
    PrecacheSound(SOUND_WOUND);
    PrecacheSound(SOUND_DEATH);

    switch(m_hdtType) {
    case HDT_FIRECRACKER: { 
      PrecacheSound(SOUND_FIREFIRECRACKER);
      PrecacheClass(CLASS_PROJECTILE, PRT_HEADMAN_FIRECRACKER);
                          } break;
    case HDT_ROCKETMAN:   {  
      PrecacheSound(SOUND_FIREROCKETMAN);
      PrecacheClass(CLASS_PROJECTILE, PRT_HEADMAN_ROCKETMAN);
                          } break;
    case HDT_BOMBERMAN:   {  
      PrecacheSound(SOUND_FIREBOMBERMAN);
      PrecacheClass(CLASS_PROJECTILE, PRT_HEADMAN_BOMBERMAN);
      PrecacheModel(MODEL_BOMB);
      PrecacheTexture(TEXTURE_BOMB);  
                          } break;
    case HDT_KAMIKAZE:    { 
      PrecacheSound(SOUND_ATTACKKAMIKAZE);
      PrecacheSound(SOUND_IDLEKAMIKAZE);
      PrecacheClass(CLASS_BASIC_EFFECT, BET_BOMB);
      PrecacheModel(MODEL_BOMB);
      PrecacheTexture(TEXTURE_BOMB);  
                          } break;
    }
  };

  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    CEnemyBase::FillEntityStatistics(pes);
    switch(m_hdtType) {
    case HDT_FIRECRACKER: { pes->es_strName+=" Firecracker"; } break;
    case HDT_ROCKETMAN:   { pes->es_strName+=" Rocketman"; } break;
    case HDT_BOMBERMAN:   { pes->es_strName+=" Bomberman"; } break;
    case HDT_KAMIKAZE:    { pes->es_strName+=" Kamikaze"; } break;
    }
    return TRUE;
  }

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // firecracker and rocketman can't harm headman
    if (!IsOfClass(penInflictor, "Headman") || 
        !(((CHeadman*)penInflictor)->m_hdtType==HDT_FIRECRACKER || 
          ((CHeadman*)penInflictor)->m_hdtType==HDT_ROCKETMAN)) {
      CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);

      // if died of chainsaw
      if (dmtType==DMT_CHAINSAW && GetHealth()<=0) {
        // must always blowup
        m_fBlowUpAmount = 0;
      }
    }
  };


  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    if (IRnd()%2) {
      iAnim = HEADMAN_ANIM_WOUND1;
    } else {
      iAnim = HEADMAN_ANIM_WOUND2;
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    INDEX iAnim;
    FLOAT3D vFront;
    GetHeadingDirection(0, vFront);
    FLOAT fDamageDir = m_vDamage%vFront;
    if (fDamageDir<0) {
      if (Abs(fDamageDir)<10.0f) {
        iAnim = HEADMAN_ANIM_DEATH_EASY_FALL_BACK;
      } else {
        iAnim = HEADMAN_ANIM_DEATH_FALL_BACK;
      }
    } else {
      if (Abs(fDamageDir)<10.0f) {
        iAnim = HEADMAN_ANIM_DEATH_EASY_FALL_FORWARD;
      } else {
        iAnim = HEADMAN_ANIM_DEATH_FALL_ON_KNEES;
      }
    }

    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  FLOAT WaitForDust(FLOAT3D &vStretch) {
    vStretch=FLOAT3D(1,1,2);
    if(GetModelObject()->GetAnim()==HEADMAN_ANIM_DEATH_EASY_FALL_BACK)
    {
      vStretch=vStretch*0.3f;
      return 0.864f;
    }
    if(GetModelObject()->GetAnim()==HEADMAN_ANIM_DEATH_FALL_BACK)
    {
      vStretch=vStretch*0.75f;
      return 0.48f;
    }    
    if(GetModelObject()->GetAnim()==HEADMAN_ANIM_DEATH_EASY_FALL_FORWARD)
    {
      vStretch=vStretch*0.3f;
      return 1.12f;
    }
    else if(GetModelObject()->GetAnim()==HEADMAN_ANIM_DEATH_FALL_ON_KNEES)
    {
      vStretch=vStretch*0.75f;
      return 1.035f;
    }
    return -1.0f;
  };

  // should this enemy blow up (spawn debris)
  BOOL ShouldBlowUp(void) 
  {
    if (m_hdtType==HDT_KAMIKAZE && GetHealth()<=0) {
      return TRUE;
    } else {
      return CEnemyBase::ShouldBlowUp();
    }
  }

  void DeathNotify(void) {
    ChangeCollisionBoxIndexWhenPossible(HEADMAN_COLLISION_BOX_DEATH);
    en_fDensity = 500.0f;
  };

  // virtual anim functions
  void StandingAnim(void) {
    StartModelAnim(HEADMAN_ANIM_IDLE_PATROL, AOF_LOOPING|AOF_NORESTART);
    if (m_hdtType==HDT_KAMIKAZE) {
      KamikazeSoundOff();
    }
  };
  void StandingAnimFight(void)
  {
    StartModelAnim(HEADMAN_ANIM_IDLE_FIGHT, AOF_LOOPING|AOF_NORESTART);
    if (m_hdtType==HDT_KAMIKAZE) {
      KamikazeSoundOff();
    }
  }
  void WalkingAnim(void) {
    StartModelAnim(HEADMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
  };
  void RunningAnim(void) {
    if (m_hdtType==HDT_KAMIKAZE) {
      KamikazeSoundOn();
      StartModelAnim(HEADMAN_ANIM_KAMIKAZE_ATTACK, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(HEADMAN_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RotatingAnim(void) {
    RunningAnim();
  };

  // virtual sound functions
  void IdleSound(void) {
    if (m_bAttackSound) {
      return;
    }
    if (m_hdtType==HDT_KAMIKAZE) {
      PlaySound(m_soSound, SOUND_IDLEKAMIKAZE, SOF_3D);
    } else {
      PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
    }
  };
  void SightSound(void) {
    if (m_bAttackSound) {
      return;
    }
    PlaySound(m_soSound, SOUND_SIGHT, SOF_3D);
  };
  void WoundSound(void) {
    if (m_bAttackSound) {
      return;
    }
    PlaySound(m_soSound, SOUND_WOUND, SOF_3D);
  };
  void DeathSound(void) {
    if (m_bAttackSound) {
      return;
    }
    PlaySound(m_soSound, SOUND_DEATH, SOF_3D);
  };

  void KamikazeSoundOn(void) {
    if (!m_bAttackSound) {
      m_bAttackSound = TRUE;
      PlaySound(m_soSound, SOUND_ATTACKKAMIKAZE, SOF_3D|SOF_LOOP);
    }
  }
  void KamikazeSoundOff(void) {
    if (m_bAttackSound) {
      m_soSound.Stop();
      m_bAttackSound = FALSE;
    }
  }

/************************************************************
 *                 BLOW UP FUNCTIONS                        *
 ************************************************************/
  void BlowUpNotify(void) {
    // kamikaze and bomberman explode if is not already exploded
    if (m_hdtType==HDT_KAMIKAZE || m_hdtType==HDT_BOMBERMAN) {
      Explode();
    }
  };

  // spawn body parts
  /*void BlowUp(void)
  {
    if( m_hdtType==HDT_FIRECRACKER || m_hdtType==HDT_ROCKETMAN)
    {
      // get your size
      FLOATaabbox3D box;
      GetBoundingBox(box);
      FLOAT fEntitySize = box.Size().MaxNorm();

      FLOAT3D vNormalizedDamage = m_vDamage-m_vDamage*(m_fBlowUpAmount/m_vDamage.Length());
      vNormalizedDamage /= Sqrt(vNormalizedDamage.Length());

      vNormalizedDamage *= 0.75f;

      FLOAT3D vBodySpeed = en_vCurrentTranslationAbsolute-en_vGravityDir*(en_vGravityDir%en_vCurrentTranslationAbsolute);

      // spawn debris
      Debris_Begin(EIBT_FLESH, DPT_BLOODTRAIL, BET_BLOODSTAIN, fEntitySize, vNormalizedDamage, vBodySpeed, 5.0f, 2.0f);

      INDEX iTextureID = TEXTURE_ROCKETMAN;
      if( m_hdtType==HDT_FIRECRACKER)
      {
        iTextureID = TEXTURE_FIRECRACKER;
      }

      Debris_Spawn(this, this, MODEL_HEADMAN_BODY, iTextureID, 0, 0, 0, 0, 0.0f,
        FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
      Debris_Spawn(this, this, MODEL_HEADMAN_HAND, iTextureID, 0, 0, 0, 0, 0.0f,
        FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
      Debris_Spawn(this, this, MODEL_HEADMAN_HAND, iTextureID, 0, 0, 0, 0, 0.0f,
        FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
      Debris_Spawn(this, this, MODEL_HEADMAN_LEGS, iTextureID, 0, 0, 0, 0, 0.0f,
        FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));

      // hide yourself (must do this after spawning debris)
      SwitchToEditorModel();
      SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
      SetCollisionFlags(ECF_IMMATERIAL);
    }
    else
    {
      CEnemyBase::BlowUp();
    }
  };*/

  // bomberman and kamikaze explode only once
  void Explode(void) {
    if (!m_bExploded) {
      m_bExploded = TRUE;

      // inflict damage
      FLOAT3D vSource;
      GetEntityInfoPosition(this, eiHeadman.vTargetCenter, vSource);
      if (m_hdtType==HDT_BOMBERMAN) {
        InflictDirectDamage(this, this, DMT_EXPLOSION, 100.0f, vSource, 
          -en_vGravityDir);
        InflictRangeDamage(this, DMT_EXPLOSION, 15.0f, vSource, 1.0f, 6.0f);
      } else {
        InflictDirectDamage(this, this, DMT_CLOSERANGE, 100.0f, vSource, 
          -en_vGravityDir);
        InflictRangeDamage(this, DMT_EXPLOSION, 30.0f, vSource, 2.75f, 8.0f);
      }

      // spawn explosion
      CPlacement3D plExplosion = GetPlacement();
      CEntityPointer penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
      ESpawnEffect eSpawnEffect;
      eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
      eSpawnEffect.betType = BET_BOMB;
      eSpawnEffect.vStretch = FLOAT3D(1.0f,1.0f,1.0f);
      penExplosion->Initialize(eSpawnEffect);

      // explosion debris
      eSpawnEffect.betType = BET_EXPLOSION_DEBRIS;
      CEntityPointer penExplosionDebris = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
      penExplosionDebris->Initialize(eSpawnEffect);

      // explosion smoke
      eSpawnEffect.betType = BET_EXPLOSION_SMOKE;
      CEntityPointer penExplosionSmoke = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
      penExplosionSmoke->Initialize(eSpawnEffect);
    }
  };

// ******
// overrides from CEnemyBase to provide exploding on close range

  // set speeds for movement towards desired position
  void SetSpeedsToDesiredPosition(const FLOAT3D &vPosDelta, FLOAT fPosDistance, BOOL bGoingToPlayer)
  {
    // if very close to player
    if (m_hdtType==HDT_KAMIKAZE && CalcDist(m_penEnemy) < EXPLODE_KAMIKAZE) {
      // explode
      SetHealth(-10000.0f);
      m_vDamage = FLOAT3D(0,10000,0);
      SendEvent(EDeath());

    // if not close
    } else {
      // behave as usual
      CEnemyBase::SetSpeedsToDesiredPosition(vPosDelta, fPosDistance, bGoingToPlayer);
    }
  }

  // get movement frequency for attack
  virtual FLOAT GetAttackMoveFrequency(FLOAT fEnemyDistance)
  {
    // kamikaze must have sharp reflexes when close
    if (m_hdtType==HDT_KAMIKAZE && fEnemyDistance < m_fCloseDistance) {
      return 0.1f;
    } else {
      return CEnemyBase::GetAttackMoveFrequency(fEnemyDistance);
    }
  }

procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  InitializeAttack(EVoid) : CEnemyBase::InitializeAttack {
    if (m_hdtType==HDT_KAMIKAZE) {
      KamikazeSoundOn();
    }
    jump CEnemyBase::InitializeAttack();
  };

  StopAttack(EVoid) : CEnemyBase::StopAttack {
    KamikazeSoundOff();
    jump CEnemyBase::StopAttack();
  };

  Fire(EVoid) : CEnemyBase::Fire {
    // firecracker
    if (m_hdtType == HDT_FIRECRACKER) {
      autocall FirecrackerAttack() EEnd;
    // rocketman
    } else if (m_hdtType == HDT_ROCKETMAN) {
      autocall RocketmanAttack() EEnd;
    // bomber
    } else if (m_hdtType == HDT_BOMBERMAN) {
      autocall BombermanAttack() EEnd;
    // kamikaze
    } else if (m_hdtType == HDT_KAMIKAZE) {
    }

    return EReturn();
  };

  // Bomberman attack
  BombermanAttack(EVoid) {
    // don't shoot if enemy above or below you too much
    if ( !IsInFrustum(m_penEnemy, CosFast(80.0f)) ) {
      return EEnd();
    }

    autowait(0.2f + FRnd()/4);

    StartModelAnim(HEADMAN_ANIM_BOMBERMAN_ATTACK, 0);
    PlaySound(m_soSound, SOUND_FIREBOMBERMAN, SOF_3D);
    autowait(0.15f);

    AddAttachment(HEADMAN_ATTACHMENT_BOMB_RIGHT_HAND, MODEL_BOMB, TEXTURE_BOMB);
    autowait(0.30f);
    RemoveAttachment(HEADMAN_ATTACHMENT_BOMB_RIGHT_HAND);

    // hit bomb
    // calculate launch velocity and heading correction for angular launch
    FLOAT fLaunchSpeed;
    FLOAT fRelativeHdg;
    CalculateAngularLaunchParams(
      GetPlacement().pl_PositionVector, BOMBERMAN_LAUNCH(2)-1.5f,
      m_penEnemy->GetPlacement().pl_PositionVector, FLOAT3D(0,0,0),
      BOMBERMAN_ANGLE,
      fLaunchSpeed,
      fRelativeHdg);
    
    // target enemy body
    EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
    FLOAT3D vShootTarget;
    GetEntityInfoPosition(m_penEnemy, peiTarget->vTargetCenter, vShootTarget);
    // launch
    CPlacement3D pl;
    PrepareFreeFlyingProjectile(pl, vShootTarget, BOMBERMAN_LAUNCH, ANGLE3D(0, BOMBERMAN_ANGLE, 0));
    CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = PRT_HEADMAN_BOMBERMAN;
    eLaunch.fSpeed = fLaunchSpeed;
    penProjectile->Initialize(eLaunch);

    // safety remove - if hitted (EWounded) while have bomb in his hand, bomb will never be removed
    RemoveAttachment(HEADMAN_ATTACHMENT_BOMB_RIGHT_HAND);

    autowait(0.45f + FRnd()/2);
    return EEnd();
  };

  // Firecraker attack
  FirecrackerAttack(EVoid) {
    // don't shoot if enemy above you more than quare of two far from you
    if (-en_vGravityDir%CalcDelta(m_penEnemy) > CalcDist(m_penEnemy)/1.41421f) {
      return EEnd();
    }

    autowait(0.2f + FRnd()/4);

    StartModelAnim(HEADMAN_ANIM_FIRECRACKER_ATTACK, 0);
    autowait(0.15f);
    PlaySound(m_soSound, SOUND_FIREFIRECRACKER, SOF_3D);
    autowait(0.52f);
    ShootProjectile(PRT_HEADMAN_FIRECRACKER, FLOAT3D(0.0f, 0.5f, 0.0f), ANGLE3D(-16.0f, 0, 0));

    autowait(0.05f);
    ShootProjectile(PRT_HEADMAN_FIRECRACKER, FLOAT3D(0.0f, 0.5f, 0.0f), ANGLE3D(-8, 0, 0));

    autowait(0.05f);
    ShootProjectile(PRT_HEADMAN_FIRECRACKER, FLOAT3D(0.0f, 0.5f, 0.0f), ANGLE3D(0.0f, 0, 0));

    autowait(0.05f);
    ShootProjectile(PRT_HEADMAN_FIRECRACKER, FLOAT3D(0.0f, 0.5f, 0.0f), ANGLE3D(8.0f, 0, 0));

    autowait(0.05f);
    ShootProjectile(PRT_HEADMAN_FIRECRACKER, FLOAT3D(0.0f, 0.5f, 0.0f), ANGLE3D(16.0f, 0, 0));

    autowait(0.5f + FRnd()/3);
    return EEnd();
  };

  // Rocketman attack
  RocketmanAttack(EVoid) {
    StandingAnimFight();   //StartModelAnim(_ANIM_STAND, AOF_LOOPING|AOF_NORESTART);
    autowait(0.2f + FRnd()/4);

    StartModelAnim(HEADMAN_ANIM_ROCKETMAN_ATTACK, 0);
    ShootProjectile(PRT_HEADMAN_ROCKETMAN, FLOAT3D(0.0f, 1.0f, 0.0f), ANGLE3D(0, 0, 0));
    PlaySound(m_soSound, SOUND_FIREROCKETMAN, SOF_3D);

    autowait(1.0f + FRnd()/3);
    return EEnd();
  };



/************************************************************
 *                    D  E  A  T  H                         *
 ************************************************************/
  Death(EVoid) : CEnemyBase::Death {
    // don't check this because summoner can send death event even to kamikaze
    // ASSERT(m_hdtType!=HDT_KAMIKAZE);
    // instead, stop playing the yelling sound
    if (m_hdtType==HDT_KAMIKAZE) {
      KamikazeSoundOff();
    }
    // death
    autocall CEnemyBase::Death() EEnd;
    // bomberman explode
    if (m_hdtType==HDT_BOMBERMAN) {
      Explode();
    }
    return EEnd();
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
    SetHealth(19.5f);
    m_fMaxHealth = 19.5f;
    en_tmMaxHoldBreath = 5.0f;
    en_fDensity = 2000.0f;
    m_fBlowUpSize = 2.0f;

    // set your appearance
    SetModel(MODEL_HEADMAN);
    switch (m_hdtType) {
      case HDT_FIRECRACKER:
        // set your texture
        SetModelMainTexture(TEXTURE_FIRECRACKER);
        AddAttachment(HEADMAN_ATTACHMENT_HEAD, MODEL_FIRECRACKERHEAD, TEXTURE_FIRECRACKERHEAD);
        AddAttachment(HEADMAN_ATTACHMENT_CHAINSAW, MODEL_CHAINSAW, TEXTURE_CHAINSAW);
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
        // damage/explode properties
        m_fBlowUpAmount = 65.0f;
        m_fBodyParts = 4;
        m_fDamageWounded = 0.0f;
        m_iScore = 200;
        break;
  
      case HDT_ROCKETMAN:
        // set your texture
        SetModelMainTexture(TEXTURE_ROCKETMAN);
        AddAttachment(HEADMAN_ATTACHMENT_HEAD, MODEL_HEAD, TEXTURE_HEAD);
        AddAttachment(HEADMAN_ATTACHMENT_ROCKET_LAUNCHER, MODEL_ROCKETLAUNCHER, TEXTURE_ROCKETLAUNCHER);
        // setup moving speed
        m_fWalkSpeed = FRnd() + 1.5f;
        m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 500.0f);
        m_fAttackRunSpeed = FRnd()*2.0f + 6.0f;
        m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
        m_fCloseRunSpeed = FRnd()*2.0f + 6.0f;
        m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
        // setup attack distances
        m_fAttackDistance = 50.0f;
        m_fCloseDistance = 0.0f;
        m_fStopDistance = 8.0f;
        m_fAttackFireTime = 2.0f;
        m_fCloseFireTime = 1.0f;
        m_fIgnoreRange = 200.0f;
        // damage/explode properties
        m_fBlowUpAmount = 65.0f;
        m_fBodyParts = 4;
        m_fDamageWounded = 0.0f;
        m_iScore = 100;
        break;

      case HDT_BOMBERMAN:
        // set your texture
        SetModelMainTexture(TEXTURE_BOMBERMAN);
        AddAttachment(HEADMAN_ATTACHMENT_HEAD, MODEL_HEAD, TEXTURE_HEAD);
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
        // damage/explode properties
        m_fBlowUpAmount = 65.0f;
        m_fBodyParts = 4;
        m_fDamageWounded = 0.0f;
        m_iScore = 500;
        break;

      case HDT_KAMIKAZE:
        // set your texture
        SetModelMainTexture(TEXTURE_KAMIKAZE);
        AddAttachment(HEADMAN_ATTACHMENT_BOMB_RIGHT_HAND, MODEL_BOMB, TEXTURE_BOMB);
        AddAttachment(HEADMAN_ATTACHMENT_BOMB_LEFT_HAND, MODEL_BOMB, TEXTURE_BOMB);
        // setup moving speed
        m_fWalkSpeed = FRnd() + 1.5f;
        m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 500.0f);
        m_fAttackRunSpeed = FRnd()*2.0f + 10.0f;
        m_aAttackRotateSpeed = AngleDeg(FRnd()*100 + 600.0f);
        m_fCloseRunSpeed = FRnd()*2.0f + 10.0f;
        m_aCloseRotateSpeed = AngleDeg(FRnd()*100 + 600.0f);
        // setup attack distances
        m_fAttackDistance = 50.0f;
        m_fCloseDistance = 10.0f;
        m_fStopDistance = 0.0f;
        m_fAttackFireTime = 2.0f;
        m_fCloseFireTime = 0.5f;
        m_fIgnoreRange = 250.0f;
        // damage/explode properties
        m_fBlowUpAmount = 0.0f;
        m_fBodyParts = 4;
        m_fDamageWounded = 0.0f;
        m_iScore = 2500;
        break;
    }

    // set stretch factors for height and width
    GetModelObject()->StretchModel(FLOAT3D(1.25f, 1.25f, 1.25f));
    ModelChangeNotify();
    StandingAnim();

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
