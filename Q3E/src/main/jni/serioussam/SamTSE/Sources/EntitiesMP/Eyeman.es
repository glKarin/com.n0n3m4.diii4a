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

323
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Enemies/Eyeman/Eyeman.h"
%}

uses "EntitiesMP/EnemyFly";

enum EyemanChar {
  0 EYC_SOLDIER   "Soldier",    // soldier
  1 EYC_SERGEANT  "Sergeant",   // sergeant
};

enum EyemanEnv {
  0 EYE_NORMAL    "Normal",
  1 EYE_LAVA      "Lava",
};

%{
// info structure
static EntityInfo eiEyemanBig = {
  EIBT_FLESH, 140.0f,
  0.0f, 1.4f, 0.0f,
  0.0f, 1.0f, 0.0f,
};
static EntityInfo eiEyemanSmall = {
  EIBT_FLESH, 120.0f,
  0.0f, 1.4f, 0.0f,
  0.0f, 1.0f, 0.0f,
};

#define BITE_AIR    3.0f
#define HIT_GROUND  2.0f
#define FIRE_GROUND   FLOAT3D(0.75f, 1.5f, -1.25f)
%}


class CEyeman : CEnemyFly {
name      "Eyeman";
thumbnail "Thumbnails\\Eyeman.tbn";

properties:
  1 enum EyemanChar m_EecChar "Character" 'C' = EYC_SOLDIER,      // character
  2 BOOL m_bInvisible "Invisible" 'I'=FALSE,
  3 enum EyemanEnv m_eeEnv "Environment" 'E' = EYE_NORMAL,
  4 BOOL m_bMumbleSoundPlaying = FALSE,
  5 CSoundObject m_soMumble,

components:
  0 class   CLASS_BASE        "Classes\\EnemyFly.ecl",
  1 model   MODEL_EYEMAN      "Models\\Enemies\\Eyeman\\Eyeman.mdl",
  2 texture TEXTURE_EYEMAN_SOLDIER    "Models\\Enemies\\Eyeman\\Eyeman4.tex",
  3 texture TEXTURE_EYEMAN_SERGEANT   "Models\\Enemies\\Eyeman\\Eyeman5.tex",
  5 texture TEXTURE_EYEMAN_LAVA   "Models\\Enemies\\Eyeman\\Eyeman6.tex",
  4 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE      "Models\\Enemies\\Eyeman\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT     "Models\\Enemies\\Eyeman\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND     "Models\\Enemies\\Eyeman\\Sounds\\Wound.wav",
 53 sound   SOUND_BITE      "Models\\Enemies\\Eyeman\\Sounds\\Bite.wav",
 54 sound   SOUND_PUNCH     "Models\\Enemies\\Eyeman\\Sounds\\Punch.wav",
 55 sound   SOUND_DEATH     "Models\\Enemies\\Eyeman\\Sounds\\Death.wav",
 56 sound   SOUND_MUMBLE    "Models\\Enemies\\Eyeman\\Sounds\\Mumble.wav",

 /*
 60 model   MODEL_EYEMAN_BODY   "Models\\Enemies\\Eyeman\\Debris\\Torso.mdl",
 61 model   MODEL_EYEMAN_HAND   "Models\\Enemies\\Eyeman\\Debris\\Arm.mdl",
 62 model   MODEL_EYEMAN_LEGS   "Models\\Enemies\\Eyeman\\Debris\\Leg.mdl",
 */

functions:
  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    if (m_bInAir) {
      str.PrintF(TRANSV("A Gnaar bit %s to death"), (const char *) strPlayerName);
    } else {
      str.PrintF(TRANSV("%s was beaten up by a Gnaar"), (const char *) strPlayerName);
    }
    return str;
  }
  void Precache(void) {
    CEnemyBase::Precache();
    PrecacheSound(SOUND_IDLE );
    PrecacheSound(SOUND_SIGHT);
    PrecacheSound(SOUND_WOUND);
    PrecacheSound(SOUND_BITE );
    PrecacheSound(SOUND_PUNCH);
    PrecacheSound(SOUND_DEATH);
    PrecacheSound(SOUND_MUMBLE);
  };

  /* Entity info */
  void *GetEntityInfo(void) {
    if (m_EecChar==EYC_SERGEANT) {
      return &eiEyemanBig;
    } else {
      return &eiEyemanSmall;
    }
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // eyeman can't harm eyeman
    if (!IsOfClass(penInflictor, "Eyeman")) {
      CEnemyFly::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
      // if died of chainsaw
      if (dmtType==DMT_CHAINSAW && GetHealth()<=0) {
        // must always blowup
        m_fBlowUpAmount = 0;
      }
    }
  };

  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    CEnemyBase::FillEntityStatistics(pes);
    switch(m_EecChar) {
    case EYC_SERGEANT: { pes->es_strName+=" Sergeant"; } break;
    case EYC_SOLDIER : { pes->es_strName+=" Soldier"; } break;
    }
    if (m_bInvisible) {
      pes->es_strName+=" Invisible";
    }
    return TRUE;
  }

  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnmSergeant, "Data\\Messages\\Enemies\\EyemanGreen.txt");
    static DECLARE_CTFILENAME(fnmSoldier , "Data\\Messages\\Enemies\\EyemanPurple.txt");
    switch(m_EecChar) {
    default: ASSERT(FALSE);
    case EYC_SERGEANT: return fnmSergeant;
    case EYC_SOLDIER : return fnmSoldier;
    }
  };
  /* Adjust model shading parameters if needed. */
  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    // no shadows for invisibles
    if (m_bInvisible) {
      colAmbient = C_WHITE;
      return FALSE;
    } else {
      return CEnemyBase::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
    }
  }

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    DeactivateMumblingSound();
    INDEX iAnim;
    if (m_bInAir) {
      switch (IRnd()%2) {
        case 0: iAnim = EYEMAN_ANIM_MORPHWOUND01; break;
        case 1: iAnim = EYEMAN_ANIM_MORPHWOUND02; break;
        default: iAnim = EYEMAN_ANIM_MORPHWOUND01; //ASSERTALWAYS("Eyeman unknown fly damage");
      }
    } else {
      FLOAT3D vFront;
      GetHeadingDirection(0, vFront);
      FLOAT fDamageDir = m_vDamage%vFront;
      if (Abs(fDamageDir)<=10) {
        switch (IRnd()%3) {
          case 0: iAnim = EYEMAN_ANIM_WOUND03; break;
          case 1: iAnim = EYEMAN_ANIM_WOUND06; break;
          case 2: iAnim = EYEMAN_ANIM_WOUND07; break;
          default: iAnim = EYEMAN_ANIM_WOUND03;
        }
      } else {
        if (fDamageDir<0) {
          iAnim = EYEMAN_ANIM_FALL01;
        } else {
          iAnim = EYEMAN_ANIM_FALL02;
        }
      }
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    DeactivateMumblingSound();
    INDEX iAnim;
    if (m_bInAir) {
      iAnim = EYEMAN_ANIM_MORPHDEATH;
    } else {
      FLOAT3D vFront;
      GetHeadingDirection(0, vFront);
      FLOAT fDamageDir = m_vDamage%vFront;
      if (fDamageDir<0) {
        iAnim = EYEMAN_ANIM_DEATH02;
      } else {
        iAnim = EYEMAN_ANIM_DEATH01;
      }
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  FLOAT WaitForDust(FLOAT3D &vStretch) {
    if(GetModelObject()->GetAnim()==EYEMAN_ANIM_DEATH01)
    {
      vStretch=FLOAT3D(1,1,1)*0.75f;
      return 0.5f;
    }
    else if(GetModelObject()->GetAnim()==EYEMAN_ANIM_DEATH02)
    {
      vStretch=FLOAT3D(1,1,1)*0.75f;
      return 0.5f;
    }
    else if(GetModelObject()->GetAnim()==EYEMAN_ANIM_MORPHDEATH)
    {
      vStretch=FLOAT3D(1,1,1)*1.0f;
      return 0.5f;
    }
    return -1.0f;
  };

  void DeathNotify(void) {
    ChangeCollisionBoxIndexWhenPossible(EYEMAN_COLLISION_BOX_DEATH);
    en_fDensity = 500.0f;
  };

  // mumbling sounds
  void ActivateMumblingSound(void)
  {
    if (!m_bMumbleSoundPlaying) {
      PlaySound(m_soMumble, SOUND_MUMBLE, SOF_3D|SOF_LOOP);
      m_bMumbleSoundPlaying = TRUE;
    }
  }
  void DeactivateMumblingSound(void)
  {
    m_soMumble.Stop();
    m_bMumbleSoundPlaying = FALSE;
  }

  // virtual anim functions
  void StandingAnim(void) {
    DeactivateMumblingSound();
    if (m_bInAir) {
      StartModelAnim(EYEMAN_ANIM_MORPHATTACKFLY, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(EYEMAN_ANIM_STAND, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void WalkingAnim(void) {
    ActivateMumblingSound();
    if (m_bInAir) {
      StartModelAnim(EYEMAN_ANIM_MORPHATTACKFLY, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(EYEMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RunningAnim(void) {
    ActivateMumblingSound();
    if (m_bInAir) {
      StartModelAnim(EYEMAN_ANIM_MORPHATTACKFLY, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(EYEMAN_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RotatingAnim(void) {
    if (m_bInAir) {
      StartModelAnim(EYEMAN_ANIM_MORPHATTACKFLY, AOF_LOOPING|AOF_NORESTART);
    } else {
      StartModelAnim(EYEMAN_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
    }
  };
  FLOAT AirToGroundAnim(void) {
    StartModelAnim(EYEMAN_ANIM_MORPHUP, 0);
    return(GetModelObject()->GetAnimLength(EYEMAN_ANIM_MORPHUP));
  };
  FLOAT GroundToAirAnim(void) {
    StartModelAnim(EYEMAN_ANIM_MORPHDOWN, 0);
    return(GetModelObject()->GetAnimLength(EYEMAN_ANIM_MORPHDOWN));
  };
  void ChangeCollisionToAir() {
    ChangeCollisionBoxIndexWhenPossible(EYEMAN_COLLISION_BOX_AIR);
  };
  void ChangeCollisionToGround() {
    ChangeCollisionBoxIndexWhenPossible(EYEMAN_COLLISION_BOX_GROUND);
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
  /*void BlowUp(void)
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
    Debris_Begin(EIBT_FLESH, DPT_BLOODTRAIL, BET_BLOODSTAIN, fEntitySize, vNormalizedDamage, vBodySpeed, 1.0f, 0.0f);

    INDEX iTextureID = TEXTURE_EYEMAN_SOLDIER;
    if (m_EecChar==EYC_SERGEANT)
    {
      iTextureID = TEXTURE_EYEMAN_SERGEANT;
    }

    Debris_Spawn(this, this, MODEL_EYEMAN_BODY, iTextureID, 0, 0, 0, 0, 0.0f,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_EYEMAN_HAND, iTextureID, 0, 0, 0, 0, 0.0f,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_EYEMAN_HAND, iTextureID, 0, 0, 0, 0, 0.0f,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_EYEMAN_LEGS, iTextureID, 0, 0, 0, 0, 0.0f,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));

    // hide yourself (must do this after spawning debris)
    SwitchToEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);
  };*/

/************************************************************
 *                     MOVING FUNCTIONS                     *
 ************************************************************/
  // check whether may move while attacking
  BOOL MayMoveToAttack(void) 
  {
    if (m_bInAir) {
      return WouldNotLeaveAttackRadius();
    } else {
      return CEnemyBase::MayMoveToAttack();
    }
  }

  // must be more relaxed about hitting then usual enemies
  BOOL CanHitEnemy(CEntity *penTarget, FLOAT fCosAngle) {
    if (IsInPlaneFrustum(penTarget, fCosAngle)) {
      return IsVisibleCheckAll(penTarget);
    }
    return FALSE;
  };
procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/

  FlyHit(EVoid) : CEnemyFly::FlyHit {
    if (CalcDist(m_penEnemy) > BITE_AIR) {
      m_fShootTime = _pTimer->CurrentTick() + 0.25f;
      return EReturn();
    }
    StartModelAnim(EYEMAN_ANIM_MORPHATTACK, 0);
    StopMoving();
    PlaySound(m_soSound, SOUND_BITE, SOF_3D);
    // damage enemy
    autowait(0.4f);
    // damage enemy
    if (CalcDist(m_penEnemy) < BITE_AIR) {
      FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
      vDirection.SafeNormalize();
      InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 3.5f, FLOAT3D(0, 0, 0), vDirection);
      // spawn blood cloud
      ESpawnEffect eSpawnEffect;
      eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
      eSpawnEffect.betType = BET_BLOODEXPLODE;
      eSpawnEffect.vStretch = FLOAT3D(1,1,1);
      CPlacement3D plOne = GetPlacement();
      GetEntityPointRatio(
        FLOAT3D(Lerp(-0.2f, +0.2f, FRnd()), Lerp(-0.2f, +0.2f, FRnd()), -1.0f),
        plOne.pl_PositionVector);
      CEntityPointer penBloodCloud = CreateEntity( plOne, CLASS_BASIC_EFFECT);
      penBloodCloud->Initialize( eSpawnEffect);
    }
    autowait(0.24f);

    StandingAnim();
    return EReturn();
  };

  GroundHit(EVoid) : CEnemyFly::GroundHit {
    if (CalcDist(m_penEnemy) > HIT_GROUND) {
      m_fShootTime = _pTimer->CurrentTick() + 0.25f;
      return EReturn();
    }
    StartModelAnim(EYEMAN_ANIM_ATTACK02, 0);
    StopMoving();
    // damage enemy
    autowait(0.2f);
    // damage enemy
    if (CalcDist(m_penEnemy) < HIT_GROUND) {
      FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
      vDirection.SafeNormalize();
      InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 3.5f, FLOAT3D(0, 0, 0), vDirection);
      PlaySound(m_soSound, SOUND_PUNCH, SOF_3D);
    }
    autowait(0.3f);
    // damage enemy
    if (CalcDist(m_penEnemy) < HIT_GROUND) {
      FLOAT3D vDirection = m_penEnemy->GetPlacement().pl_PositionVector-GetPlacement().pl_PositionVector;
      vDirection.SafeNormalize();
      InflictDirectDamage(m_penEnemy, this, DMT_CLOSERANGE, 3.5f, FLOAT3D(0, 0, 0), vDirection);
      PlaySound(m_soSound, SOUND_PUNCH, SOF_3D);
    }
    autowait(0.4f);

    StandingAnim();
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
    if (m_EecChar==EYC_SERGEANT) {
      SetHealth(90.0f);
      m_fMaxHealth = 90.0f;
      // damage/explode properties
      m_fBlowUpAmount = 130.0f;
      m_fBodyParts = 5;
      m_fBlowUpSize = 2.5f;
      m_fDamageWounded = 40.0f;
    } else {
      SetHealth(60.0f);
      m_fMaxHealth = 60.0f;
      // damage/explode properties
      m_fBlowUpAmount = 100.0f;
      m_fBodyParts = 5;
      m_fBlowUpSize = 2.0f;
      m_fDamageWounded = 25.0f;
    }
    en_fDensity = 2000.0f;
    if (m_EeftType == EFT_GROUND_ONLY) {
      en_tmMaxHoldBreath = 5.0f;
    } else {
      en_tmMaxHoldBreath = 30.0f;
    }

    // set your appearance
    SetModel(MODEL_EYEMAN);
    if (m_EecChar==EYC_SERGEANT) {
      SetModelMainTexture(TEXTURE_EYEMAN_SERGEANT);
      GetModelObject()->StretchModel(FLOAT3D(1.3f, 1.3f, 1.3f));
      ModelChangeNotify();
      m_iScore = 1000;
    } else {
      m_iScore = 500;
      if (m_eeEnv == EYE_LAVA) {
        SetModelMainTexture(TEXTURE_EYEMAN_LAVA);
      } else {
        SetModelMainTexture(TEXTURE_EYEMAN_SOLDIER);
      }
      GetModelObject()->StretchModel(FLOAT3D(1.0f, 1.0f, 1.0f));
      ModelChangeNotify();
    }
    if (m_bInvisible) {
      GetModelObject()->mo_colBlendColor = C_WHITE|0x25;
      m_iScore*=2;
    }
    // setup moving speed
    m_fWalkSpeed = FRnd() + 1.5f;
    m_aWalkRotateSpeed = FRnd()*10.0f + 500.0f;
    if (m_EecChar==EYC_SERGEANT) {
      m_fAttackRunSpeed = FRnd()*2.0f + 10.0f;
      m_aAttackRotateSpeed = AngleDeg(FRnd()*100 + 600.0f);
      m_fCloseRunSpeed = FRnd()*2.0f + 10.0f;
      m_aCloseRotateSpeed = AngleDeg(FRnd()*100 + 600.0f);
    } else {
      m_fAttackRunSpeed = FRnd()*2.0f + 9.0f;
      m_aAttackRotateSpeed = AngleDeg(FRnd()*100 + 600.0f);
      m_fCloseRunSpeed = FRnd()*2.0f + 9.0f;
      m_aCloseRotateSpeed = AngleDeg(FRnd()*100 + 600.0f);
    }
    // setup attack distances
    m_fAttackDistance = 100.0f;
    m_fCloseDistance = 3.5f;
    m_fStopDistance = 1.5f;
    m_fAttackFireTime = 2.0f;
    m_fCloseFireTime = 0.5f;
    m_fIgnoreRange = 200.0f;
    // fly moving properties
    m_fFlyWalkSpeed = FRnd()*2.0f + 3.0f;
    m_aFlyWalkRotateSpeed = FRnd()*20.0f + 600.0f;
    if (m_EecChar==EYC_SERGEANT) {
      m_fFlyAttackRunSpeed = FRnd()*2.0f + 9.5f;
      m_aFlyAttackRotateSpeed = FRnd()*25 + 350.0f;
      m_fFlyCloseRunSpeed = FRnd()*2.0f + 9.5f;
      m_aFlyCloseRotateSpeed = FRnd()*50 + 400.0f;
    } else {
      m_fFlyAttackRunSpeed = FRnd()*2.0f + 9.5f;
      m_aFlyAttackRotateSpeed = FRnd()*25 + 300.0f;
      m_fFlyCloseRunSpeed = FRnd()*2.0f + 9.5f;
      m_aFlyCloseRotateSpeed = FRnd()*50 + 300.0f;
    }
    m_fGroundToAirSpeed = 2.5f;
    m_fAirToGroundSpeed = 2.5f;
    m_fAirToGroundMin = 0.1f;
    m_fAirToGroundMax = 0.1f;
    m_fFlyHeight = 1.0f;
    // attack properties - CAN BE SET
    m_fFlyAttackDistance = 100.0f;
    m_fFlyCloseDistance = 10.0f;
    m_fFlyStopDistance = 1.5f;
    m_fFlyAttackFireTime = 2.0f;
    m_fFlyCloseFireTime = 0.5f;
    m_fFlyIgnoreRange = 200.0f;
    m_soMumble.Set3DParameters(25.0f, 0.0f, 1.0f, 1.0f);

    // continue behavior in base class
    jump CEnemyFly::MainLoop();
  };
};
