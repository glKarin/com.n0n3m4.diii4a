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

332
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Enemies/Devil/Devil.h"
#include "Models/Enemies/Devil/Weapons/Laser.h"
#include "Models/Weapons/RocketLauncher/RocketLauncherItem.h"
#include "EntitiesMP/Effector.h"
#include "EntitiesMP/WorldSettingsController.h"
#include "EntitiesMP/PyramidSpaceShip.h"
#include "EntitiesMP/BackgroundViewer.h"

#define DEVIL_LASER_SPEED 100.0f
#define DEVIL_ROCKET_SPEED 60.0f
%}

uses "EntitiesMP/DevilMarker";
uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/Projectile";
uses "EntitiesMP/Bullet";

event EBrushDestroyedByDevil {
  FLOAT3D vDamageDir,
};

event ERegenerationImpuls {
};

enum DevilCommandType {
  0 DC_GRAB_LOWER_WEAPONS     "Grab lower weapons",
  1 DC_FORCE_ACTION           "Force next action",
  2 DC_STOP_MOVING            "Stop moving",
  3 DC_STOP_ATTACK            "Stop attacking",
  4 DC_JUMP_INTO_PYRAMID      "Jump into pyramid",
  5 DC_FORCE_ATTACK_RADIUS    "Force attack radius",
  6 DC_DECREASE_ATTACK_RADIUS "Decrease attack radius",
  7 DC_TELEPORT_INTO_PYRAMID  "Teleport into pyramid",
};

event EDevilCommand {
  enum DevilCommandType dctType,  
  CEntityPointer penForcedAction,
  FLOAT fAttackRadius,
  FLOAT3D vCenterOfAttack,
};

enum DevilState {
  0 DS_NOT_EXISTING          "Not existing",              // idle
  1 DS_DESTROYING_CITY       "Destroying city",           // process of destroying city
  2 DS_ENEMY                 "Enemy",                     // behave as normal enemy
  3 DS_JUMPING_INTO_PYRAMID  "Jumping into pyramid",      // jumping into pyramid
  4 DS_PYRAMID_FIGHT         "Pyramid fight",             // pyramid fight
  5 DS_REGENERATION_IMPULSE  "Regenerating with impulse", // drinking power
};

enum DevilAttackPower {
  1 DAP_PLAYER_HUNT           "Player hunt",               // process of hunting player
  2 DAP_LOW_POWER_ATTACK      "Low power attack",          // low power attack
  3 DAP_MEDIUM_POWER_ATTACK   "Medium power attack",       // medium power attack
  4 DAP_FULL_POWER_ATTACK     "Full power attack",         // full power attack
  5 DAP_NOT_ATTACKING         "Not attacking",             // not attacking
};

%{
static FLOAT3D vLastStartPosition;
static FLOAT vLastAttackRadius;

extern INDEX cht_bKillFinalBoss;
extern INDEX cht_bDebugFinalBoss;
extern INDEX cht_bDumpFinalBossData;
extern INDEX cht_bDebugFinalBossAnimations;

#define LIGHT_ANIM_FIRE 3
#define LIGHT_ANIM_NONE 5

// parameters defining boss
#define SIZE 50.0f
#define DEVIL_HOOF_RADIUS 0.25f*SIZE
#define DEVIL_HIT_HOOF_OFFSET FLOAT3D(-0.149021f, 0.084463f, -0.294425f)*SIZE
#define DEVIL_WALK_HOOF_RIGHT_OFFSET FLOAT3D(0.374725f, 0.0776713f, 0.0754253f)*SIZE
#define DEVIL_WALK_HOOF_LEFT_OFFSET FLOAT3D(-0.402306f, 0.0864595f, 0.292397f)*SIZE

#define ATT_PROJECTILE_GUN    (FLOAT3D(-0.703544f, 1.12582f, -0.329834f)*SIZE)
#define ATT_LASER             (FLOAT3D(0.63626f, 1.13841f, -0.033062f)*SIZE)
#define ATT_ELECTRICITYGUN    (FLOAT3D(-0.764868f, 1.27992f, -0.311084f)*SIZE)
#define ATT_ROCKETLAUNCHER    (FLOAT3D(0.654788f, 1.30318f, -0.259358f)*SIZE)
#define MAGIC_PROJECTILE_EXIT (FLOAT3D(0.035866f, 1.400f, -0.792264f)*SIZE)

#define ELECTROGUN_PIPE       (FLOAT3D(-0.00423616f, -0.0216781f, -0.506613f)*SIZE)
#define LASER_PIPE            (FLOAT3D(0.0172566f, -0.123152f, -0.232228f)*SIZE)

#define PROJECTILEGUN_PIPE    (FLOAT3D(0.0359023f, -0.000490744f, -0.394403f)*SIZE)
#define ROCKETLAUNCHER_PIPE   (FLOAT3D(4.68194e-005f, 0.0483391f, -0.475134f)*SIZE)
  
#define HEALTH_MULTIPLIER 1.0f
#define BOSS_HEALTH (40000.0f*HEALTH_MULTIPLIER)
#define HEALTH_IMPULSE (10000.0f*HEALTH_MULTIPLIER)
#define HEALTH_CLASS_1 (5000*HEALTH_MULTIPLIER)
#define HEALTH_CLASS_2 (7500*HEALTH_MULTIPLIER)
#define HEALTH_CLASS_3 (10000*HEALTH_MULTIPLIER)
#define HEALTH_CLASS_4 (15000*HEALTH_MULTIPLIER)
#define CLASS_2_CANNON_FACTOR 0.75f
#define CLASS_3_ROCKETLAUNCHER_FACTOR 0.75f
#define CLASS_4_ROCKETLAUNCHER_FACTOR 0.25f
#define TM_HEALTH_IMPULSE 4.0f

// info structure
static EntityInfo eiDevil = {
  EIBT_FLESH, 50000.0f,
  0.0f, 2.0f*SIZE, 0.0f,
  0.0f, 1.4f*SIZE, 0.0f,
};

%}

class CDevil : CEnemyBase {
name      "Devil";
thumbnail "Thumbnails\\Devil.tbn";

properties:
  1 INDEX m_iAttID = 0,                                 // internal temp var
  2 FLOAT m_fDeltaWeaponPitch = 0.0f,                   // for adjusting weapon pitch
  3 FLOAT m_fDeltaWeaponHdg = 0.0f,                     // for adjusting weapon hdg
  4 FLOAT m_fFireTime = 0.0f,                           // time to fire bullets
  5 CAnimObject m_aoLightAnimation,                     // light animation object
  6 CEntityPointer m_penAction "Action" 'O',            // ptr to action marker entity
  8 INDEX m_iFiredProjectiles = 0,                      // internal counter of fired projectiles
  9 INDEX m_iToFireProjectiles = 0,                     // internal counter of projectiles that should be fired
  10 FLOAT m_fPauseStretcher = 0,                       // stretch factor for pauses between fireing
  11 FLOAT m_tmLastPause = 0.0f,                        // last pause between two fireings
  12 enum DevilState m_dsDevilState = DS_NOT_EXISTING,// current devil state
  13 FLOAT m_tmLastAngry = -1.0f,                       // last angry state
  14 CPlacement3D m_plTeleport = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0)),
  16 FLOAT m_tmTemp = 0,
  17 enum DevilState m_dsLastDevilState = DS_REGENERATION_IMPULSE,// last devil state
  18 enum DevilAttackPower m_dapAttackPower = DAP_PLAYER_HUNT,// current devil state
  19 enum DevilAttackPower m_dapLastAttackPower = DAP_NOT_ATTACKING,
  20 BOOL m_bHasUpperWeapons = FALSE,
  21 FLOAT3D m_vElectricitySource = FLOAT3D( 0,0,0),      // position of electricity ray target
  22 FLOAT3D m_vElectricityTarget = FLOAT3D( 0,0,0),      // position of electricity ray target
  23 BOOL m_bRenderElectricity = FALSE,                   // if electricity particles are rendered
  24 FLOAT m_fAdjustWeaponTime = 0.0f,                    // time for weapon to lock enemy
  25 BOOL m_bWasOnceInMainLoop = FALSE,                   // if MainLoop was called at least once 
  26 FLOAT m_tmHitBySpaceShipBeam = -1,                   // last time when was hit by space ship beam
  27 CSoundObject m_soLeft,                               // left foot sound
  28 CSoundObject m_soRight,                              // right foot sound
  29 FLOAT m_fLastWalkTime = -1.0f,                       // last walk time
  30 FLOAT m_tmFireBreathStart = UpperLimit(0.0f),        // time when fire breath started
  31 FLOAT m_tmFireBreathStop = 0.0f,                     // time when fire breath stopped
  32 FLOAT3D m_vFireBreathSource =  FLOAT3D( 0,0,0),      // position of fire breath source
  33 FLOAT3D m_vFireBreathTarget =  FLOAT3D( 0,0,0),      // position of fire breath target
  34 FLOAT m_tmRegenerationStart = UpperLimit(0.0f),      // time when regeneration started
  35 FLOAT m_tmRegenerationStop = 0.0f,                   // time when regeneration stopped
  36 FLOAT m_tmNextFXTime = 0.0f,                         // next effect time
  
  37 INDEX m_iNextChannel = 0,                            // next channel to play sound on
  // weapon sound channels 
  38 CSoundObject m_soWeapon0,
  39 CSoundObject m_soWeapon1,
  40 CSoundObject m_soWeapon2,
  41 CSoundObject m_soWeapon3,
  42 CSoundObject m_soWeapon4,
  
  43 INDEX m_iAngryAnim=0,                                // random angry animation
  44 INDEX m_iAngrySound=0,                               // random angry animation
  45 FLOAT m_tmDeathTime = -1.0f,                         // time of death

  50 INDEX m_iLastCurrentAnim = -1,
  51 INDEX m_iLastScheduledAnim = -1,

  52 enum DevilState m_dsPreRegenerationDevilState = DS_ENEMY,
  60 CSoundObject m_soClimb,                              // sound of climbing
  61 CSoundObject m_soGrabLowerWeapons,
  62 CSoundObject m_soGrabUpperWeapons,
  63 CSoundObject m_soJumpIntoPyramid,

  70 BOOL m_bForMPIntro "MP Intro" = FALSE,

{
  CEntity *penBullet;     // bullet
  CLightSource m_lsLightSource;
}

components:
  0 class   CLASS_BASE        "Classes\\EnemyBase.ecl",
  1 class   CLASS_PROJECTILE  "Classes\\Projectile.ecl",
  2 class   CLASS_EFFECTOR    "Classes\\Effector.ecl",

// ************** DEVIL **************
 10 model   MODEL_DEVIL         "Models\\Enemies\\Devil\\Devil.mdl",
 11 texture TEXTURE_DEVIL       "Models\\Enemies\\Devil\\Devil.tex",

// ************** LASER **************
 20 model   MODEL_LASER               "Models\\Enemies\\Devil\\Weapons\\Laser.mdl",
 21 texture TEXTURE_LASER             "Models\\Enemies\\Devil\\Weapons\\Laser.tex",

// ************** ROCKET LAUNCHER **************
 22 model   MODEL_ROCKETLAUNCHER        "Models\\Enemies\\Devil\\Weapons\\RocketLauncher.mdl",
 23 texture TEXTURE_ROCKETLAUNCHER      "Models\\Enemies\\Devil\\Weapons\\RocketLauncher.tex",

// ************** PROJECTILE GUN **************
 24 model   MODEL_PROJECTILEGUN         "Models\\Enemies\\Devil\\Weapons\\ProjectileGun.mdl",
 25 texture TEXTURE_PROJECTILEGUN       "Models\\Enemies\\Devil\\Weapons\\ProjectileGun.tex",

// ************** ELECTRICITY GUN *************
 26 model   MODEL_ELECTRICITYGUN        "Models\\Enemies\\Devil\\Weapons\\ElectricityGun.mdl",
 27 texture TEXTURE_ELECTRICITYGUN      "Models\\Enemies\\Devil\\Weapons\\ElectricityGun.tex",

// ************** SOUNDS **************
 60 sound   SOUND_ANGER01               "Models\\Enemies\\Devil\\Sounds\\Anger01.wav",
 61 sound   SOUND_ANGER02               "Models\\Enemies\\Devil\\Sounds\\Anger02.wav",
 62 sound   SOUND_ATTACKCLOSE           "Models\\Enemies\\Devil\\Sounds\\AttackClose.wav",
 63 sound   SOUND_CELEBRATE01           "Models\\Enemies\\Devil\\Sounds\\Celebrate01.wav",
 65 sound   SOUND_DEATH                 "Models\\Enemies\\Devil\\Sounds\\Death.wav",
 66 sound   SOUND_DRAW_LOWER_WEAPONS    "Models\\Enemies\\Devil\\Sounds\\GrabWeaponsLower.wav",
 67 sound   SOUND_DRAW_UPPER_WEAPONS    "Models\\Enemies\\Devil\\Sounds\\GrabWeaponsUpper.wav",
 68 sound   SOUND_GETUP                 "Models\\Enemies\\Devil\\Sounds\\Getup.wav",
 69 sound   SOUND_IDLE                  "Models\\Enemies\\Devil\\Sounds\\Idle.wav",
 70 sound   SOUND_PUNCH                 "Models\\Enemies\\Devil\\Sounds\\Punch.wav",
 71 sound   SOUND_SMASH                 "Models\\Enemies\\Devil\\Sounds\\Smash.wav",
 72 sound   SOUND_WALK_LEFT             "Models\\Enemies\\Devil\\Sounds\\WalkL.wav",
 73 sound   SOUND_WALK_RIGHT            "Models\\Enemies\\Devil\\Sounds\\WalkR.wav",
 74 sound   SOUND_WOUND                 "Models\\Enemies\\Devil\\Sounds\\Wound.wav",
 75 sound   SOUND_ATTACK_BREATH_START   "Models\\Enemies\\Devil\\Sounds\\AttackBreathStart.wav",
 76 sound   SOUND_ATTACK_BREATH_FIRE    "Models\\Enemies\\Devil\\Sounds\\BreathProjectile.wav",
 77 sound   SOUND_ATTACK_BREATH_END     "Models\\Enemies\\Devil\\Sounds\\AttackBreathEnd.wav",
 78 sound   SOUND_HEAL                  "Models\\Enemies\\Devil\\Sounds\\Heal.wav",
 79 sound   SOUND_ROCKETLAUNCHER        "Models\\Enemies\\Devil\\Sounds\\RocketLauncher.wav",
 80 sound   SOUND_LASER                 "Models\\Enemies\\Devil\\Sounds\\Laser.wav",
 81 sound   SOUND_LAVABOMB              "Models\\Enemies\\Devil\\Sounds\\LavaBomb.wav",
 82 sound   SOUND_GHOSTBUSTER           "Models\\Enemies\\Devil\\Sounds\\Ghostbuster.wav",
 83 sound   SOUND_ATTACK_BREATH_LOOP    "Models\\Enemies\\Devil\\Sounds\\AttackBreath.wav",
 84 sound   SOUND_CLIMB                 "Models\\Enemies\\Devil\\Sounds\\Enter.wav",
 85 sound   SOUND_DEATHPARTICLES        "Models\\Enemies\\Devil\\Sounds\\DeathParticles.wav",
 86 sound   SOUND_DISAPPEAR             "Models\\Enemies\\Devil\\Sounds\\Disappear.wav",

functions:
  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANSV("Ugh Zan killed %s"), (const char *) strPlayerName);
    return str;
  }

  void Precache(void) {
    CEnemyBase::Precache();
// ************** DEVIL **************
    PrecacheModel   (MODEL_DEVIL         );
    PrecacheTexture (TEXTURE_DEVIL       );

// ************** LASER **************
    PrecacheModel   (MODEL_LASER                 );
    PrecacheTexture (TEXTURE_LASER               );

// ************** ROCKET LAUNCHER **************
    PrecacheModel   (MODEL_ROCKETLAUNCHER        );
    PrecacheTexture (TEXTURE_ROCKETLAUNCHER     );

// ************** ELECTRICITY GUN **************
    PrecacheModel   (MODEL_ELECTRICITYGUN);
    PrecacheTexture (TEXTURE_ELECTRICITYGUN);

// ************** PROJECTILE GUN **************
    PrecacheModel   (MODEL_PROJECTILEGUN);
    PrecacheTexture (TEXTURE_PROJECTILEGUN);

// ************** PREDICTED PROJECTILE **************
    PrecacheClass(CLASS_PROJECTILE, PRT_LAVAMAN_BIG_BOMB); 
    PrecacheClass(CLASS_PROJECTILE, PRT_DEVIL_GUIDED_PROJECTILE);
    PrecacheClass(CLASS_PROJECTILE, PRT_DEVIL_LASER);
    PrecacheClass(CLASS_PROJECTILE, PRT_DEVIL_ROCKET);

// ************** SOUNDS **************
    PrecacheSound   (SOUND_ANGER01               );
    PrecacheSound   (SOUND_ANGER02               );
    PrecacheSound   (SOUND_ATTACKCLOSE           );
    PrecacheSound   (SOUND_CELEBRATE01           );
    PrecacheSound   (SOUND_DEATH                 );
    PrecacheSound   (SOUND_DRAW_LOWER_WEAPONS    );
    PrecacheSound   (SOUND_DRAW_UPPER_WEAPONS    );
    PrecacheSound   (SOUND_GETUP                 );
    PrecacheSound   (SOUND_IDLE                  );
    PrecacheSound   (SOUND_PUNCH                 );
    PrecacheSound   (SOUND_SMASH                 );
    PrecacheSound   (SOUND_WALK_LEFT             );
    PrecacheSound   (SOUND_WALK_RIGHT            );
    PrecacheSound   (SOUND_WOUND                 );
    PrecacheSound   (SOUND_ATTACK_BREATH_START   );
    PrecacheSound   (SOUND_ATTACK_BREATH_FIRE    );
    PrecacheSound   (SOUND_ATTACK_BREATH_END     );
    PrecacheSound   (SOUND_HEAL                  );
    PrecacheSound   (SOUND_ROCKETLAUNCHER        );
    PrecacheSound   (SOUND_LASER                 );
    PrecacheSound   (SOUND_LAVABOMB              );
    PrecacheSound   (SOUND_GHOSTBUSTER           );
    PrecacheSound   (SOUND_ATTACK_BREATH_LOOP    );
    PrecacheSound   (SOUND_CLIMB                 );
    PrecacheSound   (SOUND_DEATHPARTICLES        );    
    PrecacheSound   (SOUND_DISAPPEAR             );
  }

  // Validate offered target for one property
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if(penTarget==NULL)
    {
      return FALSE;
    }
    return (IsDerivedFromClass(penTarget, "Devil Marker"));
  }

  /* Read from stream. */
  void Read_t( CTStream *istr) { // throw char *
    CEnemyBase::Read_t(istr);

    // setup light source
    SetupLightSource();
  }

  /* Get static light source information. */
  CLightSource *GetLightSource(void) {
    if (!IsPredictor()) {
      return &m_lsLightSource;
    } else {
      return NULL;
    }
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


  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiDevil;
  };

  BOOL ForcesCannonballToExplode(void)
  {
    return TRUE;
  }

  void SetSpeedsToDesiredPosition(const FLOAT3D &vPosDelta, FLOAT fPosDist, BOOL bGoingToPlayer)
  {
    if(m_penEnemy!=NULL)
    {
      FLOAT fEnemyDistance = CalcDist(m_penEnemy);
      FLOAT fRadius1 = 75.0f;
      FLOAT fRadius2 = 200.0f;
      FLOAT fSpeedRadius1 = 6.0f;
      FLOAT fSpeedRadius2 = 14.0f;

      FLOAT fDistanceRatio = CalculateRatio( fEnemyDistance, fRadius1, fRadius2, 1, 0);
      if( fEnemyDistance>=fRadius2)
      {
        fDistanceRatio = 1.0f;
      }
      m_fAttackRunSpeed = fSpeedRadius1+fDistanceRatio*(fSpeedRadius2-fSpeedRadius1);
      m_fCloseRunSpeed = m_fAttackRunSpeed;
      if( cht_bDebugFinalBoss)
      {
        CPrintF( "Enm dist:%g, Speed=%g\n", fEnemyDistance, m_fAttackRunSpeed);
      }
    }    
    CEnemyBase::SetSpeedsToDesiredPosition(vPosDelta, fPosDist, bGoingToPlayer);
  }

  FLOAT GetCrushHealth(void)
  {
    return 1000.0f;
  }

  void SelectRandomAnger(void)
  {
    if( IRnd()%2) {
      m_iAngryAnim = DEVIL_ANIM_ANGER01;
      m_iAngrySound = SOUND_ANGER01;
    } else {
      m_iAngryAnim = DEVIL_ANIM_ANGER02;
      m_iAngrySound = SOUND_ANGER02;
    }
  }

  virtual FLOAT GetLockRotationSpeed(void)
  {
    return m_aAttackRotateSpeed*4;
  };

  void ShakeItBaby(FLOAT tmShaketime, FLOAT fPower)
  {
    CWorldSettingsController *pwsc = GetWSC(this);
    if (pwsc!=NULL) {
      pwsc->m_tmShakeStarted = tmShaketime;
      pwsc->m_vShakePos = GetPlacement().pl_PositionVector;
      pwsc->m_fShakeFalloff = 400.0f;
      pwsc->m_fShakeFade = 3.0f;

      pwsc->m_fShakeIntensityZ = 0.0f;
      pwsc->m_tmShakeFrequencyZ = 5.0f;
      pwsc->m_fShakeIntensityY = 0.1f*fPower;
      pwsc->m_tmShakeFrequencyY = 5.0f;
      pwsc->m_fShakeIntensityB = 2.5f*fPower;
      pwsc->m_tmShakeFrequencyB = 7.2f;

      pwsc->m_bShakeFadeIn = FALSE;
    }
  }

  void ShakeItFarBaby(FLOAT tmShaketime, FLOAT fPower)
  {
    CWorldSettingsController *pwsc = GetWSC(this);
    if (pwsc!=NULL) {
      pwsc->m_tmShakeStarted = tmShaketime;
      pwsc->m_vShakePos = GetPlacement().pl_PositionVector;
      pwsc->m_fShakeFalloff = 2048.0f;
      pwsc->m_fShakeFade = 2.0f;

      pwsc->m_fShakeIntensityZ = 0.0f;
      pwsc->m_tmShakeFrequencyZ = 5.0f;
      pwsc->m_fShakeIntensityY = 0.1f*fPower;
      pwsc->m_tmShakeFrequencyY = 5.0f;
      pwsc->m_fShakeIntensityB = 2.5f*fPower;
      pwsc->m_tmShakeFrequencyB = 7.2f;

      pwsc->m_bShakeFadeIn = FALSE;
    }
  }

  void InflictHoofDamage( FLOAT3D vOffset)
  {
    // apply range damage for right foot
    FLOAT3D vFootRel = vOffset;
    FLOAT3D vFootAbs = vFootRel*GetRotationMatrix()+GetPlacement().pl_PositionVector;
    InflictRangeDamage(this, DMT_IMPACT, 1000.0f, vFootAbs, DEVIL_HOOF_RADIUS, DEVIL_HOOF_RADIUS);
  }

  void ApplyFootQuake(void)
  {
    CModelObject &mo = *GetModelObject();
    TIME tmNow = _pTimer->CurrentTick();
    FLOAT tmAnim = -1;
    FLOAT tmWalkLen = mo.GetAnimLength(DEVIL_ANIM_WALK);
    FLOAT tmLeftFootOffset = 0.4f;
    FLOAT tmRightFootOffset = 2.05f;
    
    // if we are now playing walk anim, but another anim is scheduled to happen after walk
    if (mo.ao_iLastAnim==DEVIL_ANIM_WALK && mo.ao_tmAnimStart>tmNow)
    {
      // we started one anim time back from anim start time
      tmAnim = mo.ao_tmAnimStart-tmWalkLen;
    }
    else if (mo.ao_iCurrentAnim==DEVIL_ANIM_WALK && mo.ao_tmAnimStart<=tmNow)
    {
      tmAnim = mo.ao_tmAnimStart;
    }
    // if we are now playing walk wo idle anim, but another anim is scheduled to happen after walk to idle
    else if (mo.ao_iLastAnim==DEVIL_ANIM_FROMWALKTOIDLE && mo.ao_tmAnimStart>tmNow)
    {
      // we started one anim time back from anim start time
      tmAnim = mo.ao_tmAnimStart-tmWalkLen;
      tmLeftFootOffset = 0.6f;
      tmRightFootOffset = 1.7f;
    }
    else if (mo.ao_iCurrentAnim==DEVIL_ANIM_FROMWALKTOIDLE && mo.ao_tmAnimStart<=tmNow)
    {
      tmAnim = mo.ao_tmAnimStart;
      tmLeftFootOffset = 0.6f;
      tmRightFootOffset = 1.7f;
    }
    
    // WARNING !!! foot variable names are switched
    if( tmAnim!=-1)
    {
      FLOAT tmAnimLast = tmAnim+INDEX((tmNow-tmAnim)/tmWalkLen)*tmWalkLen;
      FLOAT tmLeftFootDown  = tmAnimLast+tmLeftFootOffset;
      FLOAT tmRightFootDown = tmAnimLast+tmRightFootOffset;
      CWorldSettingsController *pwsc = GetWSC(this);
      if( pwsc!=NULL)
      {
        if( tmNow>=tmRightFootDown && pwsc->m_tmShakeStarted<tmRightFootDown-0.1f)
        {
          // apply range damage for right foot
          InflictHoofDamage( DEVIL_WALK_HOOF_LEFT_OFFSET);
          // shake
          ShakeItBaby(tmRightFootDown, 1.0f);
          PlaySound(m_soRight, SOUND_WALK_RIGHT, SOF_3D);
        }
        else if(tmNow>=tmLeftFootDown && pwsc->m_tmShakeStarted<tmLeftFootDown-0.1f)
        {
          // apply range damage for left foot
          InflictHoofDamage( DEVIL_WALK_HOOF_RIGHT_OFFSET);
          // shake
          ShakeItBaby(tmLeftFootDown, 1.0f);
          PlaySound(m_soLeft, SOUND_WALK_LEFT, SOF_3D);
        }
      }
    }
  }

  void StopFireBreathParticles(void)
  {
    m_tmFireBreathStop = _pTimer->CurrentTick();
  }

  void StopRegenerationParticles(void)
  {
    m_tmRegenerationStop = _pTimer->CurrentTick();
  }

  void TurnOnPhysics(void)
  {
    SetPhysicsFlags(EPF_MODEL_WALKING);
    SetCollisionFlags(ECF_MODEL);
  }

  void TurnOffPhysics(void)
  {
    SetPhysicsFlags(EPF_MODEL_WALKING&~EPF_TRANSLATEDBYGRAVITY);
    SetCollisionFlags(((ECBI_MODEL)<<ECB_TEST) | ((ECBI_MODEL)<<ECB_PASS) | ((ECBI_ITEM)<<ECB_IS));
  }

  // render particles
  void RenderParticles(void)
  {
    if( m_bRenderElectricity)
    {
      // calculate electricity ray source pos
      Particles_Ghostbuster(m_vElectricitySource, m_vElectricityTarget, 24, 2.0f, 2.0f, 96.0f);
    }

    // fire breath particles
    if( _pTimer->CurrentTick()>m_tmFireBreathStart)
    {
      // render fire breath particles
      INDEX ctRendered = Particles_FireBreath(this, m_vFireBreathSource, m_vFireBreathTarget,
        m_tmFireBreathStart, m_tmFireBreathStop);
      // if should stop rendering fire breath particles
      if( _pTimer->CurrentTick()>m_tmFireBreathStop && ctRendered==0)
      {
        m_tmFireBreathStart = UpperLimit(0.0f);
      }
    }

    // regeneration particles
    if( _pTimer->CurrentTick()>m_tmRegenerationStart)
    {
      // render fire breath particles
      INDEX ctRendered = Particles_Regeneration(this, m_tmRegenerationStart, m_tmRegenerationStop, 1.0f, FALSE);
      // if should stop rendering regeneration particles
      if( _pTimer->CurrentTick()>m_tmRegenerationStop && ctRendered==0)
      {
        m_tmRegenerationStart = UpperLimit(0.0f);
      }
    }

    // if is dead
    if( m_tmDeathTime != -1.0f && _pTimer->CurrentTick()>m_tmDeathTime && _pTimer->CurrentTick()<m_tmDeathTime+4.0f)
    {
      INDEX ctRendered = Particles_Regeneration(this, m_tmDeathTime, m_tmDeathTime+2.0f, 0.25f, TRUE);
    }

    CEnemyBase::RenderParticles();
  }

  FLOAT3D GetWeaponPositionRelative(void)
  {
    CAttachmentModelObject &amo = *GetModelObject()->GetAttachmentModel(m_iAttID);
    FLOAT3D vAttachment = FLOAT3D(0,0,0);
    switch(m_iAttID)
    {
    case DEVIL_ATTACHMENT_LASER:
      vAttachment = ATT_LASER;
      break;
    case DEVIL_ATTACHMENT_PROJECTILEGUN:
      vAttachment = ATT_PROJECTILE_GUN;
      break;
    case DEVIL_ATTACHMENT_ELETRICITYGUN:
      vAttachment = ATT_ELECTRICITYGUN;
      break;
    case DEVIL_ATTACHMENT_ROCKETLAUNCHER:
      vAttachment = ATT_ROCKETLAUNCHER;
      break;
    default:
      ASSERTALWAYS("Invalid attachment ID");
    }
    return(vAttachment);
  }

  FLOAT3D GetWeaponPositionAbsolute(void)
  {
    return GetPlacement().pl_PositionVector + GetWeaponPositionRelative()*GetRotationMatrix();
  }
  
  FLOAT3D GetFireingPositionRelative(void)
  {
    CAttachmentModelObject &amo = *GetModelObject()->GetAttachmentModel(m_iAttID);
    FLOAT3D vWeaponPipe = FLOAT3D(0,0,0);
    FLOAT3D vAttachment = FLOAT3D(0,0,0);
    switch(m_iAttID)
    {
    case DEVIL_ATTACHMENT_LASER:
      vWeaponPipe = LASER_PIPE;
      vAttachment = ATT_LASER;
      break;
    case DEVIL_ATTACHMENT_PROJECTILEGUN:
      vWeaponPipe = PROJECTILEGUN_PIPE;
      vAttachment = ATT_PROJECTILE_GUN;
      break;
    case DEVIL_ATTACHMENT_ELETRICITYGUN:
      vWeaponPipe = ELECTROGUN_PIPE;
      vAttachment = ATT_ELECTRICITYGUN;
      break;
    case DEVIL_ATTACHMENT_ROCKETLAUNCHER:
      vWeaponPipe = ROCKETLAUNCHER_PIPE;
      vAttachment = ATT_ROCKETLAUNCHER;
      break;
    default:
      ASSERTALWAYS("Invalid attachment ID");
    }

    // create matrix
    FLOATmatrix3D mWpn;
    MakeRotationMatrixFast(mWpn, amo.amo_plRelative.pl_OrientationAngle);
    return (vAttachment+vWeaponPipe*mWpn);
  }

  FLOAT3D GetFireingPositionAbsolute(void)
  {
    return GetPlacement().pl_PositionVector + GetFireingPositionRelative()*GetRotationMatrix();
  }  

  /* 
  Regeneration legend:
  --------------------
    - if less than    0     -> no generation
    - class 1     -> invoke impulse regeneration +HEALTH_IMPULSE
    - class 2     -> drops when constantly hitted with cannonballs
    - class 3     -> drops when constantly hitted with rockets
    - class 4     -> drops when constantly hitted with rockets
    - if greater than class 5 -> no regeneration
  */
  void ApplyTickRegeneration(void) 
  {
    if( cht_bKillFinalBoss && GetSP()->sp_bSinglePlayer)
    {
      cht_bKillFinalBoss=FALSE;
      SetHealth(-1);
      return;
    }
    // if currently regenerating or died or out of healing range or recently hit by space ship beam
    if(m_dsDevilState == DS_REGENERATION_IMPULSE ||
       GetHealth()<=0 || GetHealth()>=HEALTH_CLASS_4 || 
       _pTimer->CurrentTick()<m_tmHitBySpaceShipBeam+0.5f)
    {
      return;
    }

    FLOAT fDmgRocketsPerTick = 1800.0f/10.0f*_pTimer->TickQuantum;
    FLOAT fDmgCannonsPerTick = 2959.0f/10.0f*_pTimer->TickQuantum;
    FLOAT fRegeneration = 0.0f;

    if(GetHealth()<HEALTH_CLASS_1)
    {
      SendEvent(ERegenerationImpuls());
    }
    else if(GetHealth()<HEALTH_CLASS_2)
    {
      fRegeneration = fDmgCannonsPerTick*CLASS_2_CANNON_FACTOR;
    }
    else if(GetHealth()<HEALTH_CLASS_3)
    {
      fRegeneration = fDmgRocketsPerTick*CLASS_3_ROCKETLAUNCHER_FACTOR;
    }
    else if(GetHealth()<HEALTH_CLASS_4)
    {
      fRegeneration = fDmgRocketsPerTick*CLASS_4_ROCKETLAUNCHER_FACTOR;
    }
    // apply regeneration
    SetHealth(GetHealth()+fRegeneration);
  };

  /* Post moving */
  void PostMoving(void)
  {
    ApplyFootQuake();
    CEnemyBase::PostMoving();
    // discard non-moving optimization
    en_ulFlags &= ~ENF_INRENDERING;
    ApplyTickRegeneration();
  }

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // don't allow telefrag damage
    if( dmtType == DMT_TELEPORT)
    {
       return;
    }

    if( !(m_dsDevilState==DS_ENEMY || m_dsDevilState==DS_PYRAMID_FIGHT) || penInflictor==this)
    {
      return;
    }
    
    if(m_dsDevilState!=DS_PYRAMID_FIGHT)
    {
      if( GetHealth()<1000.0f)
      {
        return;
      }
      fDamageAmmount=ClampUp(fDamageAmmount, GetHealth()/2.0f);
    }

    CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
  };

  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    if( cht_bDebugFinalBoss)
    {
      // print state change
      if( m_dsDevilState!=m_dsLastDevilState)
      {
        m_dsLastDevilState = m_dsDevilState;
        CTString strDevilState = DevilState_enum.NameForValue(INDEX(m_dsDevilState));
        CPrintF( "New devil state: %s\n", (const char *) strDevilState);
      }

      // print fire power state change
      if( m_dapAttackPower!=m_dapLastAttackPower)
      {
        m_dapLastAttackPower = m_dapAttackPower;
        CTString strAttackPower = DevilAttackPower_enum.NameForValue(INDEX(m_dapAttackPower));
        CPrintF( "New attack power: %s\n", (const char *) strAttackPower);
      }

      // print radius of attack change
      if( (vLastStartPosition != m_vStartPosition) ||
          (vLastAttackRadius != m_fAttackRadius) )
      {
        vLastStartPosition = m_vStartPosition;
        vLastAttackRadius = m_fAttackRadius;
        CPrintF( "Coordinate of attack (%g, %g, %g), Radius of attack: %g\n", 
          m_vStartPosition(1), m_vStartPosition(2), m_vStartPosition(3), 
          m_fAttackRadius);
      }
    }

    if(cht_bDebugFinalBossAnimations)
    {
      TIME tmNow = _pTimer->CurrentTick();
      CModelObject &mo = *GetModelObject();
      // obtain current and scheduled animation
      INDEX iCurrentAnim, iScheduledAnim;
      if (mo.ao_tmAnimStart>tmNow)
      {
        iCurrentAnim = mo.ao_iLastAnim;
        iScheduledAnim = mo.ao_iCurrentAnim;
      }
      else
      {
        iCurrentAnim = mo.ao_iCurrentAnim;
        iScheduledAnim = -1;
      }

      if( iCurrentAnim!=m_iLastCurrentAnim || iScheduledAnim!=m_iLastScheduledAnim)
      {
        CAnimData *pad = mo.GetData();
        CAnimInfo aiCurrent;
        mo.GetAnimInfo(iCurrentAnim, aiCurrent);
        CTString strCurrentAnimName = aiCurrent.ai_AnimName;

        CTString strScheduledAnimName = ".....";
        if(iScheduledAnim != -1)
        {
          CAnimInfo aiScheduled;
          mo.GetAnimInfo(iScheduledAnim, aiScheduled);
          strScheduledAnimName = aiScheduled.ai_AnimName;
        }
        CPrintF("Time: %-10g %20s, %s\n",
          _pTimer->GetLerpedCurrentTick(),
          (const char *) strCurrentAnimName,
          (const char *) strScheduledAnimName);
      }
      m_iLastCurrentAnim = iCurrentAnim;
      m_iLastScheduledAnim = iScheduledAnim;
    }
    
    if( cht_bDumpFinalBossData)
    {
      cht_bDumpFinalBossData=FALSE;
      // dump devil data to console
      CPrintF("\n\n\n\n\n\n\n");
      CPrintF("Devil class data ...................\n");
      CPrintF("\n\n");
      
      CTString strAttackPower = DevilAttackPower_enum.NameForValue(INDEX(m_dapAttackPower));
      CPrintF( "Attack power: %s\n", (const char *) strAttackPower);
      CTString strDevilState = DevilState_enum.NameForValue(INDEX(m_dsDevilState));
      CPrintF( "Devil state: %s\n", (const char *) strDevilState);

      CPrintF("m_fFireTime = %g\n", m_fFireTime);
      CPrintF("m_iFiredProjectiles = %d\n", m_iFiredProjectiles);
      CPrintF("m_iToFireProjectiles = %d\n", m_iToFireProjectiles);
      CPrintF("m_tmLastPause = %g\n", m_tmLastPause);
      CPrintF("m_fPauseStretcher = %g\n", m_fPauseStretcher);
      CPrintF("m_tmLastAngry = %g\n", m_tmLastAngry);
      CPrintF("m_bHasUpperWeapons = %d\n", m_bHasUpperWeapons);
      CPrintF("m_fAdjustWeaponTime = %g\n", m_fAdjustWeaponTime);
      CPrintF("m_bWasOnceInMainLoop = %d\n", m_bWasOnceInMainLoop);
      CPrintF("m_tmHitBySpaceShipBeam = %g\n", m_tmHitBySpaceShipBeam);
      CPrintF("m_fLastWalkTime = %g\n", m_fLastWalkTime);
      CPrintF("m_tmFireBreathStart = %g\n", m_tmFireBreathStart);
      CPrintF("m_tmFireBreathStop = %g\n", m_tmFireBreathStop);
      CPrintF("m_tmRegenerationStart = %g\n", m_tmRegenerationStart);
      CPrintF("m_tmRegenerationStop = %g\n", m_tmRegenerationStop);
      CPrintF("m_tmNextFXTime = %g\n", m_tmNextFXTime);
      CPrintF("m_tmDeathTime = %g\n", m_tmDeathTime);
      CPrintF("Health = %g\n", GetHealth());

      CPrintF("\n\n\n\n\n\n\n");
      CPrintF("Enemy base data ...................\n");
      CPrintF("\n\n");

      CPrintF( "m_ttTarget (type): %d\n", INDEX(m_ttTarget));

      CPrintF( "m_penWatcher %x\n", (const char *) (m_penWatcher->GetName()));
      CTString strEnemyName = "Null ptr, no name";
      if( m_penEnemy != NULL) 
      {
        strEnemyName = m_penEnemy->GetName();
      }
      CPrintF( "m_penEnemy %x, enemy name: %s\n", (const char *) (m_penEnemy->GetName()), (const char *) strEnemyName);

      CPrintF( "m_vStartPosition (%g, %g, %g)\n", m_vStartPosition(1), m_vStartPosition(2), m_vStartPosition(3));
      CPrintF( "m_vStartDirection (%g, %g, %g)\n", m_vStartDirection(1), m_vStartDirection(2), m_vStartDirection(3));
      CPrintF( "m_bOnStartPosition = %d\n", m_bOnStartPosition);
      CPrintF( "m_fFallHeight = %g\n", m_fFallHeight);
      CPrintF( "m_fStepHeight = %g\n", m_fStepHeight);
      CPrintF( "m_fSenseRange = %g\n", m_fSenseRange);
      CPrintF( "m_fViewAngle = %g\n", m_fViewAngle);

      CPrintF( "m_fWalkSpeed = %g\n", m_fWalkSpeed);
      CPrintF( "m_aWalkRotateSpeed = %g\n", m_aWalkRotateSpeed);
      CPrintF( "m_fAttackRunSpeed = %g\n", m_fAttackRunSpeed);
      CPrintF( "m_aAttackRotateSpeed = %g\n", m_aAttackRotateSpeed);
      CPrintF( "m_fCloseRunSpeed = %g\n", m_fCloseRunSpeed);
      CPrintF( "m_aCloseRotateSpeed = %g\n", m_aCloseRotateSpeed);
      CPrintF( "m_fAttackDistance = %g\n", m_fAttackDistance);
      CPrintF( "m_fCloseDistance = %g\n", m_fCloseDistance);
      CPrintF( "m_fAttackFireTime = %g\n", m_fAttackFireTime);
      CPrintF( "m_fCloseFireTime = %g\n", m_fCloseFireTime);
      CPrintF( "m_fStopDistance = %g\n", m_fStopDistance);
      CPrintF( "m_fIgnoreRange = %g\n", m_fIgnoreRange);
      CPrintF( "m_fLockOnEnemyTime = %g\n", m_fLockOnEnemyTime);

      CPrintF( "m_fMoveTime = %g\n", m_fMoveTime);
      CPrintF( "m_vDesiredPosition (%g, %g, %g)\n", m_vDesiredPosition(1), m_vDesiredPosition(2), m_vDesiredPosition(3));
  
      CTString strDestinationType = DestinationType_enum.NameForValue(INDEX(m_dtDestination));
      CPrintF( "m_dtDestination: %s\n", (const char *) strDestinationType);
      CPrintF( "m_penPathMarker %x\n", (const char *) (m_penPathMarker->GetName()));

      CPrintF( "m_vPlayerSpotted (%g, %g, %g)\n", m_vPlayerSpotted(1), m_vPlayerSpotted(2), m_vPlayerSpotted(3));
      CPrintF( "m_fMoveFrequency = %g\n", m_fMoveFrequency);
      CPrintF( "m_fMoveSpeed = %g\n", m_fMoveSpeed);
      CPrintF( "m_aRotateSpeed = %g\n", m_aRotateSpeed);
      CPrintF( "m_fLockStartTime = %g\n", m_fLockStartTime);
      CPrintF( "m_fRangeLast = %g\n", m_fRangeLast);
      CPrintF( "m_fShootTime = %g\n", m_fShootTime);
      CPrintF( "m_fAttackRadius = %g\n", m_fAttackRadius);
      CPrintF( "m_tmGiveUp = %g\n", m_tmGiveUp);
      CPrintF( "m_fActivityRange = %g\n", m_fActivityRange);

      CTString strMarkerName = "Null ptr, no name";
      if( m_penMarker != NULL) 
      {
        strMarkerName = m_penMarker->GetName();
      }
      CPrintF( "m_penMarker %x, marker name: %s\n", (const char *) (m_penMarker->GetName()), (const char *) strMarkerName);

      CTString strMainMusicHolderName = "Null ptr, no name";
      if( m_penMainMusicHolder != NULL) 
      {
        strMainMusicHolderName = m_penMainMusicHolder->GetName();
      }
      CPrintF( "m_penMainMusicHolder %x, MainMusicHolder name: %s\n", (const char *) (m_penMainMusicHolder->GetName()), (const char *) strMainMusicHolderName);
      CPrintF( "m_tmLastFussTime = %g\n", m_tmLastFussTime);
      CPrintF( "m_iScore = %d\n", m_iScore);
      CPrintF( "m_fMaxHealth = %g\n", m_fMaxHealth);
      CPrintF( "m_bBoss = %d\n", m_bBoss);
      CPrintF( "m_fSpiritStartTime = %g\n", m_fSpiritStartTime);
      CPrintF( "m_tmSpraySpawned = %g\n", m_tmSpraySpawned);
      CPrintF( "m_fSprayDamage = %g\n", m_fSprayDamage);
      CPrintF( "m_fMaxDamageAmmount  = %g\n", m_fMaxDamageAmmount );
    }
    
    vLightDirection = FLOAT3D(0.0f, 270.0f, 0.0f);
    colAmbient = RGBToColor(32,32,32);
    colLight = RGBToColor(255,235,145);

    return CMovableModelEntity::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
    //return CEnemyBase::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
  };

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    StartModelAnim(DEVIL_ANIM_WOUNDLOOP, 0);
    return DEVIL_ANIM_WOUNDLOOP;
  };

  // death
  INDEX AnimForDeath(void) {
    StartModelAnim(DEVIL_ANIM_DEATH, 0);
    return DEVIL_ANIM_DEATH;
  };

  void DeathNotify(void) {
    StopFireBreathParticles();
    StopRegenerationParticles();
  };

  // virtual anim functions
  void StandingAnim(void) {
    StartModelAnim(DEVIL_ANIM_IDLE, AOF_SMOOTHCHANGE|AOF_LOOPING|AOF_NORESTART);
  };

  void WalkingAnim(void) {
    if( !m_bForMPIntro)
    {
      CModelObject &mo = *GetModelObject();
      INDEX iAnim = mo.GetAnim();
      if (iAnim==DEVIL_ANIM_WALK)
      {
        // do nothing
      } else if (iAnim==DEVIL_ANIM_FROMIDLETOWALK) {
        StartModelAnim(DEVIL_ANIM_WALK, AOF_LOOPING|AOF_SMOOTHCHANGE);
      } else {
        StartModelAnim(DEVIL_ANIM_FROMIDLETOWALK, AOF_SMOOTHCHANGE);
      }
    }
  };
  void RunningAnim(void) {
    WalkingAnim();
  };
  void RotatingAnim(void) {
    WalkingAnim();
  };

  // virtual sound functions
  void IdleSound(void) {
    PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
  };
  void SightSound(void) {
    //PlaySound(m_soSound, SOUND_SIGHT, SOF_3D);
  };
  void WoundSound(void) {
    PlaySound(m_soSound, SOUND_WOUND, SOF_3D);
  };
  void DeathSound(void) {
    PlaySound(m_soSound, SOUND_DEATH, SOF_3D|SOF_VOLUMETRIC);
  };


  // start fire laser
  void StartFireLaser(void) {
    //PlaySound(m_soSound, SOUND_FIRE, SOF_3D|SOF_LOOP);
    PlayLightAnim(LIGHT_ANIM_FIRE, AOF_LOOPING|SOF_3D);
  };

  // fire one laser
  void FireOneLaser(FLOAT fRatio, FLOAT fDeltaPitch)
  {
    PlayWeaponSound( SOUND_LASER);
    FLOAT3D vWpnPipeRel = GetFireingPositionRelative();
    FLOAT3D vWpnPipeAbs = GetFireingPositionAbsolute();
    // calculate predicted position
    FLOAT3D vTarget = m_penEnemy->GetPlacement().pl_PositionVector;
    FLOAT3D vSpeedDst = ((CMovableEntity&) *m_penEnemy).en_vCurrentTranslationAbsolute*fRatio;
    FLOAT fSpeedSrc = DEVIL_LASER_SPEED;
    m_vDesiredPosition = CalculatePredictedPosition(vWpnPipeAbs, vTarget, fSpeedSrc,
      vSpeedDst, GetPlacement().pl_PositionVector(2) );
    // shoot predicted propelled projectile
    ShootPredictedProjectile(PRT_DEVIL_LASER, m_vDesiredPosition, vWpnPipeRel, ANGLE3D(0, fDeltaPitch, 0));
    //PlaySound(m_soSound, SOUND_FIRE, SOF_3D|SOF_LOOP);
    PlayLightAnim(LIGHT_ANIM_FIRE, AOF_LOOPING);
  };

  // stop fire laser
  void StopFireLaser(void) {
    m_soSound.Stop();
    PlayLightAnim(LIGHT_ANIM_NONE, 0);
  };

  // start fire rocket
  void StartFireRocket(void) {
    //PlaySound(m_soSound, SOUND_FIRE, SOF_3D|SOF_LOOP);
    PlayLightAnim(LIGHT_ANIM_FIRE, AOF_LOOPING);
  };

  void PlayWeaponSound( ULONG idSound)
  {
    CSoundObject &so = (&m_soWeapon0)[m_iNextChannel];
    m_iNextChannel = (m_iNextChannel+1)%5;
    PlaySound(so, idSound, SOF_3D);
  }

  // fire one rocket
  void FireOneRocket(FLOAT fRatio)
  {
    PlayWeaponSound( SOUND_ROCKETLAUNCHER);
    FLOAT3D vWpnPipeRel = GetFireingPositionRelative();
    FLOAT3D vWpnPipeAbs = GetFireingPositionAbsolute();
    // calculate predicted position
    FLOAT3D vTarget = m_penEnemy->GetPlacement().pl_PositionVector;
    FLOAT3D vSpeedDst = ((CMovableEntity&) *m_penEnemy).en_vCurrentTranslationAbsolute*fRatio;
    FLOAT fSpeedSrc = DEVIL_ROCKET_SPEED;
    m_vDesiredPosition = CalculatePredictedPosition(vWpnPipeAbs, vTarget, fSpeedSrc,
      vSpeedDst, GetPlacement().pl_PositionVector(2) );
    // shoot predicted propelled projectile
    ShootPredictedProjectile(PRT_DEVIL_ROCKET, m_vDesiredPosition, vWpnPipeRel, ANGLE3D(0, 0, 0));
    //PlaySound(m_soSound, SOUND_FIRE, SOF_3D|SOF_LOOP);
    PlayLightAnim(LIGHT_ANIM_FIRE, AOF_LOOPING);

    //PlaySound(m_soSound, SOUND_FIRE, SOF_3D|SOF_LOOP);
    PlayLightAnim(LIGHT_ANIM_FIRE, AOF_LOOPING);
  };

  // stop fire rocket
  void StopFireRocket(void) {
    m_soSound.Stop();
    PlayLightAnim(LIGHT_ANIM_NONE, 0);
  };

  void AddLowerWeapons(void)
  {
    // laser
    AddAttachmentToModel(this, *GetModelObject(), DEVIL_ATTACHMENT_LASER, MODEL_LASER, TEXTURE_LASER, 0, 0, 0);
    // projectile gun
    AddAttachmentToModel(this, *GetModelObject(), DEVIL_ATTACHMENT_PROJECTILEGUN, MODEL_PROJECTILEGUN, TEXTURE_PROJECTILEGUN, 0, 0, 0);
    GetModelObject()->StretchModel(FLOAT3D(SIZE, SIZE, SIZE));
  };

  void AddUpperWeapons(void)
  {
    // rocket launcher
    AddAttachmentToModel(this, *GetModelObject(), DEVIL_ATTACHMENT_ROCKETLAUNCHER, MODEL_ROCKETLAUNCHER, TEXTURE_ROCKETLAUNCHER, 0, 0, 0);
    // electro gun
    AddAttachmentToModel(this, *GetModelObject(), DEVIL_ATTACHMENT_ELETRICITYGUN, MODEL_ELECTRICITYGUN, TEXTURE_ELECTRICITYGUN, 0, 0, 0);
    GetModelObject()->StretchModel(FLOAT3D(SIZE, SIZE, SIZE));
  };

  void RemoveWeapons(void)
  {
    // remove all weapons
    RemoveAttachmentFromModel(*GetModelObject(), DEVIL_ATTACHMENT_LASER);         
    RemoveAttachmentFromModel(*GetModelObject(), DEVIL_ATTACHMENT_PROJECTILEGUN); 
    RemoveAttachmentFromModel(*GetModelObject(), DEVIL_ATTACHMENT_ELETRICITYGUN); 
    RemoveAttachmentFromModel(*GetModelObject(), DEVIL_ATTACHMENT_ROCKETLAUNCHER);
  }

  class CDevilMarker *GetAction(void)
  {
    CDevilMarker *penAction = (CDevilMarker *) (CEntity*) m_penAction;
    ASSERT( penAction != NULL);
    return penAction;
  };

/************************************************************
 *                PREDICTED PROJECTILE                      *
 ************************************************************/
  void F_FirePredictedProjectile(void)
  {
    PlayWeaponSound( SOUND_LAVABOMB);
    FLOAT3D vFireingRel = GetFireingPositionRelative();
    FLOAT3D vFireingAbs = GetFireingPositionAbsolute();

    FLOAT3D vTarget = m_penEnemy->GetPlacement().pl_PositionVector;
    FLOAT3D vSpeedDest = ((CMovableEntity&) *m_penEnemy).en_vCurrentTranslationAbsolute;
    FLOAT fLaunchSpeed;
    FLOAT fRelativeHdg;
    
    // obtain current gun orientation
    CAttachmentModelObject &amo = *GetModelObject()->GetAttachmentModel(m_iAttID);
    FLOAT fPitch = amo.amo_plRelative.pl_OrientationAngle(2);
    
    // calculate parameters for predicted angular launch curve
    EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
    CalculateAngularLaunchParams( vFireingAbs, 0, vTarget, vSpeedDest, fPitch, fLaunchSpeed, fRelativeHdg);

    // target enemy body
    FLOAT3D vShootTarget;
    GetEntityInfoPosition(m_penEnemy, peiTarget->vTargetCenter, vShootTarget);
    // launch
    CPlacement3D pl;
    PrepareFreeFlyingProjectile(pl, vShootTarget, vFireingRel, ANGLE3D( fRelativeHdg, fPitch, 0));
    CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = PRT_LAVAMAN_BIG_BOMB;
    eLaunch.fSpeed = fLaunchSpeed;
    penProjectile->Initialize(eLaunch);
  }
  
  /* Handle an event, return false if the event is not handled. */
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    if (ee.ee_slEvent==EVENTCODE_EDevilCommand)
    {
      EDevilCommand eDevilCommand = ((EDevilCommand &) ee);
      if( eDevilCommand.dctType == DC_FORCE_ATTACK_RADIUS)
      {
        m_fAttackRadius = eDevilCommand.fAttackRadius;
        m_vStartPosition = eDevilCommand.vCenterOfAttack;
      }
      if( eDevilCommand.dctType == DC_DECREASE_ATTACK_RADIUS)
      {
        if( m_fAttackRadius>21.0f)
        {
          m_fAttackRadius -= 20.0f;
        }
      }
    }

    return CEnemyBase::HandleEvent(ee);
  }

procedures:
/************************************************************
 *                TRY TO REACH DESTINATION                  *
 ************************************************************/
  // move to given destination position
  WalkTo(EVoid) 
  {
    autocall WaitCurrentAnimEnd() EReturn;
    WalkingAnim();
    m_vDesiredPosition = GetAction()->GetPlacement().pl_PositionVector;
    // setup parameters
    m_fMoveFrequency = 0.25f;
    m_fMoveSpeed = 15.0f;
    m_aRotateSpeed = 60.0f;
    m_fMoveTime = _pTimer->CurrentTick() + CalcDistanceInPlaneToDestination()/m_fMoveSpeed + 5.0f;
    // while not there and time not expired
    while (CalcDistanceInPlaneToDestination()>m_fMoveSpeed*m_fMoveFrequency*4.0f &&
           m_fMoveTime>_pTimer->CurrentTick()) {
      // every once in a while
      wait (m_fMoveFrequency) {
        on (EBegin) : { 
          // adjust direction and speed
          ULONG ulFlags = SetDesiredMovement(); 
          MovementAnimation(ulFlags);
          resume;
        }
        on (ETimer) : { stop; }
      }
    }

    StopMoving();
    // return to the caller
    return EReturn();
  };

/************************************************************
 *                CITY DESTROYING                           *
 ************************************************************/
  DestroyCity()
  {
    m_soSound.Set3DParameters(1000.0f, 500.0f, 3.0f, 1.0f);

    while(TRUE)
    {
      if( GetAction()->m_datType == DAT_RISE)
      {
        autocall Rise() EReturn;
      }
      else if( GetAction()->m_datType == DAT_ROAR)
      {
        SelectRandomAnger();
        autocall Angry() EReturn;
      }
      else if( GetAction()->m_datType == DAT_SMASH)
      {
        autocall Smash() EReturn;
      }
      else if( GetAction()->m_datType == DAT_PUNCH)
      {
        autocall Punch() EReturn;
      }
      else if( GetAction()->m_datType == DAT_HIT_GROUND)
      {
        autocall HitGround() EReturn;
      }
      else if( GetAction()->m_datType == DAT_JUMP)
      {
        // do nothing, obsolete
      }
      else if( GetAction()->m_datType == DAT_WAIT)
      {
        autocall WaitCurrentAnimEnd() EReturn;
        StartModelAnim(DEVIL_ANIM_IDLE, 0);
        autowait(GetModelObject()->GetAnimLength(DEVIL_ANIM_IDLE)*GetAction()->m_iWaitIdles);
      }
      else if( GetAction()->m_datType == DAT_WALK)
      {
        autocall WalkTo() EReturn;
      }
      else if( GetAction()->m_datType == DAT_STOP_DESTROYING)
      {
        return EReturn();
      }
      else if(TRUE)
      {
        StartModelAnim(DEVIL_ANIM_IDLE, 0);
        autowait(GetModelObject()->GetAnimLength(DEVIL_ANIM_IDLE));
      }
      // switch to next action
      m_penAction = GetAction()->m_penTarget;
      if( GetAction()->m_penTrigger != NULL)
      {
        GetAction()->m_penTrigger->SendEvent(ETrigger());
      }
    }
  };
  
  WaitCurrentAnimEnd()
  {
    autowait(_pTimer->TickQuantum);
    CModelObject &mo = *GetModelObject();
    FLOAT tmWait = mo.GetAnimLength( mo.ao_iCurrentAnim )-mo.GetPassedTime();
    if( tmWait > _pTimer->TickQuantum)
    {
      FLOAT fTimeToWait = tmWait-_pTimer->TickQuantum*2;
      if( fTimeToWait>=_pTimer->TickQuantum)
      {
        autowait(fTimeToWait);
      }
    }
    return EReturn();
  }

  WaitWalkToEnd()
  {
    if(GetModelObject()->GetAnim()==DEVIL_ANIM_WALK) 
    {
      autocall WaitCurrentAnimEnd() EReturn;
      StartModelAnim(DEVIL_ANIM_FROMWALKTOIDLE, AOF_SMOOTHCHANGE);
      autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_FROMWALKTOIDLE)-0.1f);
    }
    autocall WaitCurrentAnimEnd() EReturn;
    return EReturn();
  }

  WaitWalkOrIdleToEnd()
  {
    if(GetModelObject()->GetAnim()==DEVIL_ANIM_WALK) 
    {
      autocall WaitCurrentAnimEnd() EReturn;
      StartModelAnim(DEVIL_ANIM_FROMWALKTOIDLE, AOF_SMOOTHCHANGE);
      autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_FROMWALKTOIDLE)-0.1f);
    }
    else if(GetModelObject()->GetAnim()==DEVIL_ANIM_FROMIDLETOWALK) 
    {
      autocall WaitCurrentAnimEnd() EReturn;
      StartModelAnim(DEVIL_ANIM_FROMWALKTOIDLE, AOF_SMOOTHCHANGE);
      autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_FROMWALKTOIDLE)-0.1f);
    }
    else if(GetModelObject()->GetAnim()==DEVIL_ANIM_IDLE) 
    {
      autocall WaitCurrentAnimEnd() EReturn;
    }
    return EReturn();
  }
  
  Rise()
  {
    autocall WaitCurrentAnimEnd() EReturn;
    PlaySound(m_soSound, SOUND_GETUP, SOF_3D);
    GetModelObject()->PlayAnim(DEVIL_ANIM_GETUP, 0);
    return EReturn();
  }

  Celebrate()
  {
    autocall WaitWalkToEnd() EReturn;

    PlaySound(m_soSound, SOUND_CELEBRATE01, SOF_3D);
    GetModelObject()->PlayAnim(DEVIL_ANIM_CELEBRATE01, AOF_SMOOTHCHANGE);
    autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_CELEBRATE01)-0.1f);
    return EReturn();
  }

  Angry()
  {
    autocall WaitWalkToEnd() EReturn;

    GetModelObject()->PlayAnim(m_iAngryAnim, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    PlaySound(m_soSound, m_iAngrySound, SOF_3D);
    autowait( GetModelObject()->GetAnimLength(m_iAngryAnim)-0.1f);
    return EReturn();
  }


  SubBeamDamage1()
  {
    StopMoving();
    PlaySound(m_soSound, SOUND_WOUND, SOF_3D);
    GetModelObject()->PlayAnim(DEVIL_ANIM_WOUNDSTART, AOF_SMOOTHCHANGE);
    autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_WOUNDSTART)-0.1f);
    GetModelObject()->PlayAnim(DEVIL_ANIM_WOUNDLOOP, AOF_LOOPING|AOF_SMOOTHCHANGE);
    jump SubBeamDamage2();
  }
  SubBeamDamage2()
  {
    while(TRUE) {
      wait(0.1f)
      {
        on (EBegin) : { resume; }
        on (EHitBySpaceShipBeam) : {
          m_tmHitBySpaceShipBeam = _pTimer->CurrentTick();
          stop;
        }
        on (ETimer) : { jump SubBeamDamage3(); }
      }
    }
  }
  SubBeamDamage3()
  {    
    GetModelObject()->PlayAnim(DEVIL_ANIM_WOUNDEND, AOF_SMOOTHCHANGE);
    autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_WOUNDEND)-0.1f);
    GetModelObject()->PlayAnim(DEVIL_ANIM_IDLE, AOF_LOOPING|AOF_SMOOTHCHANGE);
    return EReturn();
  }
  BeamDamage()
  {
    wait()
    {
      on (EBegin) : { call SubBeamDamage1(); }
      on (EHitBySpaceShipBeam) : {
        m_tmHitBySpaceShipBeam = _pTimer->CurrentTick();
        resume;
      }
      on (EReturn) : { stop; }
    }
    return EReturn();
  }

  Smash()
  {
    //autocall WaitCurrentAnimEnd() EReturn;
    //autocall WaitWalkToEnd() EReturn;

    GetModelObject()->PlayAnim(DEVIL_ANIM_FROMWALKTOIDLE, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;

    GetModelObject()->PlayAnim(DEVIL_ANIM_SMASH, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;

    autowait(0.05f);
    PlaySound(m_soSound, SOUND_SMASH, SOF_3D);
    autowait(0.7f);
    if( GetAction()->m_penToDestroy1 != NULL)
    {
      EBrushDestroyedByDevil ebdbd;
      ebdbd.vDamageDir = FLOAT3D( -0.125f, 0.0f, -0.5f);
      GetAction()->m_penToDestroy1->SendEvent(ebdbd);
    }
    autowait(2.8f-1.0f);
    if( GetAction()->m_penToDestroy2 != NULL)
    {
      EBrushDestroyedByDevil ebdbd;
      ebdbd.vDamageDir = FLOAT3D( -0.125f, 0.0f, -0.5f);
      GetAction()->m_penToDestroy2->SendEvent(ebdbd);
    }
    return EReturn();
  }

  Punch()
  {
    //autocall WaitWalkToEnd() EReturn;
    //autocall WaitCurrentAnimEnd() EReturn;
    GetModelObject()->PlayAnim(DEVIL_ANIM_FROMWALKTOIDLE, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;

    GetModelObject()->PlayAnim(DEVIL_ANIM_PUNCH, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;

    autowait(0.05f);
    PlaySound(m_soSound, SOUND_PUNCH, SOF_3D);
    autowait(0.8f);
    if( GetAction()->m_penToDestroy1 != NULL)
    {
      EBrushDestroyedByDevil ebdbd;
      ebdbd.vDamageDir = FLOAT3D( -0.125f, 0.0f, -0.5f);
      GetAction()->m_penToDestroy1->SendEvent(ebdbd);
    }
    autowait(2.8f-1.1f);
    if( GetAction()->m_penToDestroy2 != NULL)
    {
      EBrushDestroyedByDevil ebdbd;
      ebdbd.vDamageDir = FLOAT3D( 0.125f, 0.0f, -0.5f);
      GetAction()->m_penToDestroy2->SendEvent(ebdbd);
    }

    return EReturn();
  }

  HitGround()
  {
    autocall WaitWalkToEnd() EReturn;

    PlaySound(m_soSound, SOUND_ATTACKCLOSE, SOF_3D);
    GetModelObject()->PlayAnim(DEVIL_ANIM_ATTACKCLOSE, AOF_SMOOTHCHANGE);
    autowait(1.44f);
    ShakeItBaby(_pTimer->CurrentTick(), 5.0f);

    CPlacement3D plObelisk = GetPlacement();
    // spawn spray spray
    CEntity *penEffector = CreateEntity( plObelisk, CLASS_EFFECTOR);
    // set spawn parameters
    ESpawnEffector eSpawnEffector;
    eSpawnEffector.tmLifeTime = 6.0f;
    eSpawnEffector.fSize = 1.0f;
    eSpawnEffector.eetType = ET_HIT_GROUND;
    eSpawnEffector.vDamageDir = FLOAT3D( 0.0f, 2.0f, 0.0f);
    // initialize spray
    penEffector->Initialize( eSpawnEffector);

    return EReturn();
  }

  GrabLowerWeapons()
  {
    autocall WaitWalkToEnd() EReturn;
    GetModelObject()->PlayAnim(DEVIL_ANIM_GRABWEAPONS01, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    PlaySound(m_soSound, SOUND_DRAW_LOWER_WEAPONS, SOF_3D);
    autowait(0.84f);
    AddLowerWeapons();
    WalkingAnim();
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    return EReturn();
  }

  GrabUpperWeapons()
  {
    autocall WaitWalkToEnd() EReturn;
    GetModelObject()->PlayAnim(DEVIL_ANIM_GRABWEAPONS02, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    PlaySound(m_soSound, SOUND_DRAW_UPPER_WEAPONS, SOF_3D);
    autowait(0.84f);
    AddUpperWeapons();
    m_bHasUpperWeapons = TRUE;
    WalkingAnim();
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    return EReturn();
  }

  GrabBothWeapons()
  {
    autocall WaitWalkToEnd() EReturn;
    GetModelObject()->PlayAnim(DEVIL_ANIM_GRABWEAPONS01, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    PlaySound(m_soGrabLowerWeapons, SOUND_DRAW_LOWER_WEAPONS, SOF_3D);
    autowait(0.84f);
    AddLowerWeapons();
    GetModelObject()->PlayAnim(DEVIL_ANIM_GRABWEAPONS02, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    PlaySound(m_soGrabUpperWeapons, SOUND_DRAW_UPPER_WEAPONS, SOF_3D);
    autowait(0.84f);
    AddUpperWeapons();
    m_bHasUpperWeapons = TRUE;
    WalkingAnim();
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    return EReturn();
  }

  PreMainLoop(EVoid) : CEnemyBase::PreMainLoop
  {
    m_soSound.Set3DParameters(1000.0f, 500.0f, 2.0f, 1.0f);
    m_soGrabLowerWeapons.Set3DParameters(1000.0f, 500.0f, 2.0f, 1.0f);
    m_soGrabUpperWeapons.Set3DParameters(1000.0f, 500.0f, 2.0f, 1.0f);
    m_soJumpIntoPyramid.Set3DParameters(1000.0f, 500.0f, 2.0f, 1.0f);
    m_soLeft.Set3DParameters(1000.0f, 500.0f, 2.0f, 1.0f);
    m_soRight.Set3DParameters(1000.0f, 500.0f, 2.0f, 1.0f);
    m_soWeapon0.Set3DParameters(1000.0f, 500.0f, 1.0f, 1.0f);
    m_soWeapon1.Set3DParameters(1000.0f, 500.0f, 1.0f, 1.0f);
    m_soWeapon2.Set3DParameters(1000.0f, 500.0f, 1.0f, 1.0f);
    m_soWeapon3.Set3DParameters(1000.0f, 500.0f, 1.0f, 1.0f);
    m_soWeapon4.Set3DParameters(1000.0f, 500.0f, 1.0f, 1.0f);

    TurnOnPhysics();

    if (m_penEnemy==NULL)
    {
      // get some player for trigger source if any is existing
      CEntity *penEnemy = FixupCausedToPlayer(this, m_penEnemy, /*bWarning=*/FALSE);
      if (penEnemy!=m_penEnemy) {
        SetTargetSoft(penEnemy);
      }
    }
    
    return EReturn();
  }

/************************************************************
 *                PROCEDURES WHEN HARMED                    *
 ************************************************************/
  // Play wound animation
  BeWounded(EDamage eDamage) : CEnemyBase::BeWounded 
  {
    StopMoving();
    // determine damage anim and play the wounding
    autowait(GetModelObject()->GetAnimLength(AnimForDamage(eDamage.fAmount)));
    return EReturn();
  };

/************************************************************
 *                C L O S E   A T T A C K                   *
 ************************************************************/
  Hit(EVoid) : CEnemyBase::Hit {
    autocall WaitCurrentAnimEnd() EReturn;
    StartModelAnim(DEVIL_ANIM_ATTACKCLOSE, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    PlaySound(m_soSound, SOUND_ATTACKCLOSE, SOF_3D);
    autowait(1.4f);
    ShakeItBaby(_pTimer->CurrentTick(), 5.0f);
    if( CalcDist(m_penEnemy) < m_fCloseDistance)
    {
      InflictDirectDamage(m_penEnemy, this, DMT_IMPACT, 1000.0f,
        m_penEnemy->GetPlacement().pl_PositionVector, FLOAT3D(0,1,0));
    }
    InflictHoofDamage( DEVIL_HIT_HOOF_OFFSET);

    autowait(GetModelObject()->GetAnimLength(DEVIL_ANIM_ATTACKCLOSE)-1.4f-_pTimer->TickQuantum);  // misplaced ) here ???
    return EReturn();
  };

/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  // initial preparation
  InitializeAttack(EVoid) : CEnemyBase::InitializeAttack {
    jump CEnemyBase::InitializeAttack();
  };

  Fire(EVoid) : CEnemyBase::Fire
  {
    m_iToFireProjectiles = 0;
    m_fAttackFireTime = 10.0f;
    m_fPauseStretcher = 1.0f;

    if( m_dapAttackPower==DAP_MEDIUM_POWER_ATTACK &&
        (_pTimer->CurrentTick()-m_fLastWalkTime) > 6.0f)
    {
      m_fAttackFireTime = 6.0f;
      m_fLastWalkTime = _pTimer->CurrentTick()+6.0f;
      return;
    }
    
    switch( m_dapAttackPower)
    {
      case DAP_PLAYER_HUNT:
        if( _pTimer->CurrentTick()-m_tmLastAngry > 10.0f)
        {
          m_fAttackFireTime = 7.5f+FRnd()*5.0f;
          m_tmLastAngry = _pTimer->CurrentTick();
          SelectRandomAnger();
          jump Angry();
        }
        return;
        break;
      case DAP_LOW_POWER_ATTACK:
        m_iToFireProjectiles = 2;
        m_fAttackFireTime = 5.0f;
        m_fPauseStretcher = 1.0f;
        break;
      case DAP_MEDIUM_POWER_ATTACK:
        m_iToFireProjectiles = 3;
        m_fAttackFireTime = 0.1f;
        m_fPauseStretcher = 0.5f;
        break;
      case DAP_FULL_POWER_ATTACK:
        m_iToFireProjectiles = 4;
        m_fAttackFireTime = 0.1f;
        m_fPauseStretcher = 0.1f;
        break;
    }
    
    INDEX iRnd = IRnd()%5;
    if( !m_bHasUpperWeapons)
    {
      iRnd = IRnd()%3;
    }
   
    /*iRnd = Clamp(INDEX(tmp_af[0]), INDEX(0), INDEX(4));*/
    switch(iRnd)
    {
    case 0:
        jump FirePredictedProjectile();
        break;
    case 1:
        jump FireLaser();
        break;
    case 2:
        jump FireGuidedProjectile();
        break;
    case 3:
        jump FireRocketLauncher();
        break;
    case 4:
        jump FireElectricityGun();
        break;
    }
  };

  // call this to lock on player for some time - set m_fLockOnEnemyTime before calling
  DevilLockOnEnemy(EVoid) 
  {
    // stop moving
    StopMoving();
    // play animation for locking
    ChargeAnim();
    // wait charge time
    m_fLockStartTime = _pTimer->CurrentTick();
    while (m_fLockStartTime+GetProp(m_fLockOnEnemyTime) > _pTimer->CurrentTick()) {
      // each tick
      m_fMoveFrequency = 0.05f;
      wait (m_fMoveFrequency) {
        on (ETimer) : { stop; }
        on (EBegin) : {
          m_vDesiredPosition = PlayerDestinationPos();
          // if not heading towards enemy
          if (!IsInPlaneFrustum(m_penEnemy, CosFast(30.0f))) {
            m_fLockStartTime = -10000.0f;
            stop;
          }

/*          if (!IsInPlaneFrustum(m_penEnemy, CosFast(5.0f))) {
            m_fMoveSpeed = 0.0f;
            m_aRotateSpeed = GetLockRotationSpeed();
          // if heading towards enemy
          } else {
            m_fMoveSpeed = 0.0f;
            m_aRotateSpeed = 0.0f;
          }*/
          // adjust direction and speed
          //ULONG ulFlags = SetDesiredMovement(); 
          //MovementAnimation(ulFlags);
          resume;
        }
      }
    }
    // stop rotating
    StopRotating();

    // return to caller
    return EReturn();
  };

  AdjustWeaponForFire()
  {
    FLOAT3D vRelWeapon = GetWeaponPositionRelative();
    FLOAT3D vAbsWeapon = GetPlacement().pl_PositionVector + vRelWeapon*GetRotationMatrix();
    
    m_fFireTime = _pTimer->CurrentTick()+m_fAdjustWeaponTime;
    FLOAT3D vEnemy = m_penEnemy->GetPlacement().pl_PositionVector;
    FLOAT3D vDir = (vEnemy-vAbsWeapon).Normalize();
    ANGLE3D aAngles;
    DirectionVectorToAngles(vDir, aAngles);
    CPlacement3D plRelPl = CPlacement3D(FLOAT3D(0,0,0),aAngles);
    plRelPl.AbsoluteToRelative(GetPlacement());
    FLOAT fWantedHdg   = plRelPl.pl_OrientationAngle(1);
    FLOAT fWantedPitch = plRelPl.pl_OrientationAngle(2);

    // adjust angle of projectile gun
    if( m_iAttID == DEVIL_ATTACHMENT_PROJECTILEGUN)
    {
      FLOAT3D vShooting = GetPlacement().pl_PositionVector;
      FLOAT3D vTarget = m_penEnemy->GetPlacement().pl_PositionVector;
      FLOAT fDistanceFactor = 1.0f-ClampUp( (vShooting-vTarget).Length()/250.0f, 1.0f);
      fWantedPitch = 20.0f-fDistanceFactor*50.0f;
    }

    CAttachmentModelObject &amo = *GetModelObject()->GetAttachmentModel(m_iAttID);
    m_fDeltaWeaponHdg   = (fWantedHdg  -amo.amo_plRelative.pl_OrientationAngle(1))/(m_fAdjustWeaponTime/_pTimer->TickQuantum);
    m_fDeltaWeaponPitch = (fWantedPitch-amo.amo_plRelative.pl_OrientationAngle(2))/(m_fAdjustWeaponTime/_pTimer->TickQuantum);
    while (m_fFireTime > _pTimer->CurrentTick())
    {
      wait(_pTimer->TickQuantum)
      {
        on (EBegin) :
        {
          CAttachmentModelObject &amo = *GetModelObject()->GetAttachmentModel(m_iAttID);
          amo.amo_plRelative.pl_OrientationAngle(1) += m_fDeltaWeaponHdg;
          amo.amo_plRelative.pl_OrientationAngle(2) += m_fDeltaWeaponPitch;
          resume;
        }
        on (ETimer) : { stop; }
      }
    }
    return EReturn();
  }

  StraightenUpWeapon()
  {
    FLOAT fAdjustWeaponTime = 0.25f;
    m_fFireTime = _pTimer->CurrentTick()+fAdjustWeaponTime;

    CAttachmentModelObject &amo = *GetModelObject()->GetAttachmentModel(m_iAttID);
    m_fDeltaWeaponHdg   = amo.amo_plRelative.pl_OrientationAngle(1)/(fAdjustWeaponTime/_pTimer->TickQuantum);
    m_fDeltaWeaponPitch = amo.amo_plRelative.pl_OrientationAngle(2)/(fAdjustWeaponTime/_pTimer->TickQuantum);
    while (m_fFireTime > _pTimer->CurrentTick())
    {
      wait(_pTimer->TickQuantum)
      {
        on (EBegin) :
        {
          CAttachmentModelObject &amo = *GetModelObject()->GetAttachmentModel(m_iAttID);
          amo.amo_plRelative.pl_OrientationAngle(1) -= m_fDeltaWeaponHdg;
          amo.amo_plRelative.pl_OrientationAngle(2) -= m_fDeltaWeaponPitch;
          resume;
        }
        on (ETimer) : { stop; }
      }
    }
    return EReturn();
  }

  FireLaser(EVoid)
  {
    autocall WaitWalkOrIdleToEnd() EReturn;

    // to fire
    StartModelAnim(DEVIL_ANIM_FROMIDLETOATTACK01, AOF_SMOOTHCHANGE);
    autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_FROMIDLETOATTACK01)-0.1f);
    StartModelAnim(DEVIL_ANIM_ATTACK01, AOF_SMOOTHCHANGE|AOF_LOOPING);

    m_iAttID = DEVIL_ATTACHMENT_LASER;
    m_fAdjustWeaponTime = 0.25f;
    autocall AdjustWeaponForFire() EReturn;

    // fire lasers
    StartFireLaser();
    m_iFiredProjectiles = 0;
    while (m_iFiredProjectiles<m_iToFireProjectiles*10)
    {
      autocall DevilLockOnEnemy() EReturn;
      m_tmLastPause = 0.1f*m_fPauseStretcher;
      autowait(m_tmLastPause);
      // fire laser
      FLOAT fPredictionRatio = (FRnd()-0.5f)*0.25f;
      if(m_iFiredProjectiles&1)
      {
        fPredictionRatio = 1.0f;
      }
      FLOAT fDeltaPitch = (FRnd()-0.5f)*1.0f;
      FireOneLaser(fPredictionRatio, fDeltaPitch);
      m_iFiredProjectiles++;
      if (!IsInFrustum(m_penEnemy, CosFast(30.0f))) {
        m_iFiredProjectiles = 10000;
      }
    }
    autocall StraightenUpWeapon() EReturn;
    StopFireLaser();

    // from fire
    //StartModelAnim(DEVIL_ANIM_IDLE, 0);
    //autowait(0.25f*m_fPauseStretcher);

    MaybeSwitchToAnotherPlayer();

    // shoot completed
    return EReturn();
  };

  FireRocketLauncher(EVoid)
  {
    autocall WaitWalkOrIdleToEnd() EReturn;

    // to fire
    StartModelAnim(DEVIL_ANIM_FROMIDLETOATTACK02, AOF_SMOOTHCHANGE);
    autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_FROMIDLETOATTACK02)-0.1f);
    StartModelAnim(DEVIL_ANIM_ATTACK02, AOF_SMOOTHCHANGE|AOF_LOOPING);

    m_iAttID = DEVIL_ATTACHMENT_ROCKETLAUNCHER;
    m_fAdjustWeaponTime = 0.5f;
    autocall AdjustWeaponForFire() EReturn;

    // fire rockets
    StartFireRocket();
    m_iFiredProjectiles = 0;
    while (m_iFiredProjectiles<m_iToFireProjectiles)
    {
      autocall DevilLockOnEnemy() EReturn;
      m_tmLastPause = 0.5f+0.3f*m_fPauseStretcher;
      autowait( m_tmLastPause);
      FLOAT fPredictionRatio = 0.25f+m_iFiredProjectiles*(0.75f/m_iToFireProjectiles);
      fPredictionRatio = 1.0f;
      m_iFiredProjectiles++;
      // fire rocket
      FireOneRocket(fPredictionRatio);
      if (!IsInFrustum(m_penEnemy, CosFast(30.0f))) {
        m_iFiredProjectiles = 10000;
      }
    }
    autocall StraightenUpWeapon() EReturn;
    StopFireRocket();

    // from fire
    //StartModelAnim(DEVIL_ANIM_IDLE, 0);
    //autowait(0.25f*m_fPauseStretcher);

    MaybeSwitchToAnotherPlayer();

    // shoot completed
    return EReturn();
  };

  FirePredictedProjectile(EVoid)
  {
    autocall WaitWalkOrIdleToEnd() EReturn;

    // to fire
    StartModelAnim(DEVIL_ANIM_FROMIDLETOATTACK01, AOF_SMOOTHCHANGE);
    autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_FROMIDLETOATTACK01)-0.1f);
    StartModelAnim(DEVIL_ANIM_ATTACK01, AOF_SMOOTHCHANGE|AOF_LOOPING);

    m_iAttID = DEVIL_ATTACHMENT_PROJECTILEGUN;
    m_fAdjustWeaponTime = 0.5f;
    autocall AdjustWeaponForFire() EReturn;

    // start fireing predicted magic projectile
    m_iFiredProjectiles = 0;
    while (m_iFiredProjectiles<m_iToFireProjectiles)
    {
      m_fAdjustWeaponTime = 0.45f;
      autocall AdjustWeaponForFire() EReturn;
      F_FirePredictedProjectile();
      autowait( 0.8f-m_fAdjustWeaponTime);
      m_iFiredProjectiles++;
      if (!IsInFrustum(m_penEnemy, CosFast(30.0f))) {
        m_iFiredProjectiles = 10000;
      }
    }
    autocall StraightenUpWeapon() EReturn;

    // from fire
    StartModelAnim(DEVIL_ANIM_FROMATTACK01TOIDLE, AOF_SMOOTHCHANGE);
    //autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_FROMATTACK01TOIDLE)-0.1f);
    //StartModelAnim(DEVIL_ANIM_IDLE, AOF_SMOOTHCHANGE|AOF_LOOPING);

    //autowait(0.25f*m_fPauseStretcher);
    
    MaybeSwitchToAnotherPlayer();

    // shoot completed
    return EReturn();
  }

  FireElectricityGun(EVoid)
  {
    autocall WaitWalkOrIdleToEnd() EReturn;

    // to fire
    StartModelAnim(DEVIL_ANIM_FROMIDLETOATTACK02, AOF_SMOOTHCHANGE);
    autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_FROMIDLETOATTACK02)-0.1f);
    StartModelAnim(DEVIL_ANIM_ATTACK02, AOF_SMOOTHCHANGE|AOF_LOOPING);

    m_iAttID = DEVIL_ATTACHMENT_ELETRICITYGUN;
    m_fAdjustWeaponTime = 0.5f;
    //autocall AdjustWeaponForFire() EReturn;

    // start fireing electricity
    m_iFiredProjectiles = 0;
    while (m_iFiredProjectiles<m_iToFireProjectiles)
    {
      m_fAdjustWeaponTime = 0.45f;
      autocall AdjustWeaponForFire() EReturn;

      // shoot from gun
      const FLOATmatrix3D &m = GetRotationMatrix();
      m_vElectricitySource = GetFireingPositionAbsolute();

      // target enemy body
      EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
      GetEntityInfoPosition(m_penEnemy, peiTarget->vTargetCenter, m_vElectricityTarget);

      // give some time so player can move away from electricity beam
      autowait(0.4f);

      // fire electricity beam
      m_bRenderElectricity = TRUE;
      m_tmTemp = _pTimer->CurrentTick();
      m_tmNextFXTime = m_tmTemp-_pTimer->TickQuantum;
      PlayWeaponSound( SOUND_GHOSTBUSTER);
      while( _pTimer->CurrentTick() < m_tmTemp+0.75f)
      {
        wait(_pTimer->TickQuantum) {
          on (EBegin): {
            // correct electricity beam target
            FLOAT3D vNewTarget;
            EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
            GetEntityInfoPosition(m_penEnemy, peiTarget->vTargetCenter, vNewTarget);
            FLOAT3D vDiff = vNewTarget-m_vElectricityTarget;
            // if we have valid length
            if( vDiff.Length() > 1.0f)
            {
              // calculate adjustment
              m_vElectricityTarget = m_vElectricityTarget+vDiff.Normalize()*10.0f*_pTimer->TickQuantum;
            }

            // cast ray
            CCastRay crRay( this, m_vElectricitySource, m_vElectricityTarget);
            crRay.cr_bHitTranslucentPortals = FALSE;
            crRay.cr_bPhysical = FALSE;
            crRay.cr_ttHitModels = CCastRay::TT_COLLISIONBOX;
            GetWorld()->CastRay(crRay);
            // if entity is hit
            if( crRay.cr_penHit != NULL)
            {
              // apply damage
              InflictDirectDamage( crRay.cr_penHit, this, DMT_BULLET, 50.0f*_pTimer->TickQuantum/0.5f,
                FLOAT3D(0, 0, 0), (m_vElectricitySource-m_vElectricityTarget).Normalize());
            }

            if( _pTimer->CurrentTick()>m_tmNextFXTime)
            {
              m_tmNextFXTime = _pTimer->CurrentTick()+0.125f+FRnd()*0.125f;
              CPlacement3D plElectricityTarget =  CPlacement3D( m_vElectricityTarget, ANGLE3D(0,0,0));
              CEntity *penEffector = CreateEntity( plElectricityTarget, CLASS_EFFECTOR);
              // set spawn parameters
              ESpawnEffector eSpawnEffector;
              eSpawnEffector.tmLifeTime = 6.0f;
              eSpawnEffector.fSize = 0.025f;
              eSpawnEffector.eetType = ET_HIT_GROUND;
              eSpawnEffector.vDamageDir = FLOAT3D( 0.0f, 2.0f, 0.0f);
              // initialize spray
              penEffector->Initialize( eSpawnEffector);
            }

            resume;
          };
          on (ETimer): { stop; };
        }
      }
      m_soSound.Stop();
      m_bRenderElectricity = FALSE;

      autowait( 0.8f-m_fAdjustWeaponTime);
      m_iFiredProjectiles++;
      if (!IsInFrustum(m_penEnemy, CosFast(30.0f))) {
        m_iFiredProjectiles = 10000;
      }
    }
    autocall StraightenUpWeapon() EReturn;

    // from idle
    StartModelAnim(DEVIL_ANIM_FROMATTACK02TOIDLE, AOF_SMOOTHCHANGE);
    //autowait( GetModelObject()->GetAnimLength(DEVIL_ANIM_FROMATTACK02TOIDLE)-0.1f);
    //StartModelAnim(DEVIL_ANIM_IDLE, AOF_SMOOTHCHANGE|AOF_LOOPING);
    
    MaybeSwitchToAnotherPlayer();

    // shoot completed
    return EReturn();
  }

  FireGuidedProjectile(EVoid)
  {
    autocall WaitWalkOrIdleToEnd() EReturn;

    StartModelAnim(DEVIL_ANIM_ATTACKBREATHSTART, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    PlaySound(m_soLeft, SOUND_ATTACK_BREATH_START, SOF_3D);

    StartModelAnim(DEVIL_ANIM_ATTACKBREATH, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    PlaySound(m_soRight, SOUND_ATTACK_BREATH_LOOP, SOF_LOOP|SOF_3D);

    // start fireing predicted magic projectile
    m_iFiredProjectiles = 0;
    m_tmFireBreathStart = _pTimer->CurrentTick();
    m_tmFireBreathStop = UpperLimit(0.0f);
    // calculate breath source and target positions
    const FLOATmatrix3D &m = GetRotationMatrix();
    m_vFireBreathSource = GetPlacement().pl_PositionVector+MAGIC_PROJECTILE_EXIT*m;
    m_vFireBreathTarget = m_penEnemy->GetPlacement().pl_PositionVector-FLOAT3D(0,20.0f,0);
    while (m_iFiredProjectiles<m_iToFireProjectiles)
    {
      m_tmLastPause = 0.45f;
      autowait( m_tmLastPause);
      // fire one guided projectile
      ShootProjectile(PRT_DEVIL_GUIDED_PROJECTILE, MAGIC_PROJECTILE_EXIT, 
        ANGLE3D( AngleDeg(10.0f*Cos(m_iFiredProjectiles*360.0f/6.0f)), -AngleDeg(20.0f*Sin(m_iFiredProjectiles*180.0f/6.0f)), 0));
      PlayWeaponSound( SOUND_ATTACK_BREATH_FIRE);

      autowait(0.8f-m_tmLastPause);
      m_iFiredProjectiles++;
    }
    StopFireBreathParticles();
    // from fire
    autocall WaitCurrentAnimEnd() EReturn;
    PlaySound(m_soSound, SOUND_ATTACK_BREATH_END, SOF_3D);
    m_soRight.Stop();
    StartModelAnim(DEVIL_ANIM_ATTACKBREATHEND, AOF_SMOOTHCHANGE);

    MaybeSwitchToAnotherPlayer();

    // shoot completed
    return EReturn();
  }


  JumpIntoPyramid(EVoid)
  {
    // remove collision and gravity
    TurnOffPhysics();
    StopMoving();
    
    // remove weapons
    RemoveWeapons();
    SetTargetNone();
    SetPlacement(m_plTeleport);
    GetModelObject()->PlayAnim(DEVIL_ANIM_CLIMB, 0);
    PlaySound(m_soJumpIntoPyramid, SOUND_CLIMB, SOF_3D);
    autowait(7.0f);

    // turn it arround
    m_tmTemp = _pTimer->CurrentTick();
    while( _pTimer->CurrentTick() < m_tmTemp+0.7f)
    {
      wait(_pTimer->TickQuantum) {
        on (EBegin): { resume; };
        on (ETimer): { stop; };
        otherwise(): { resume; };
      }
      CPlacement3D plCurrent = GetPlacement();
      FLOAT aDelta = -35.0f/0.7f*_pTimer->TickQuantum;
      plCurrent.pl_OrientationAngle+=FLOAT3D(aDelta,0,0);
      SetPlacement(plCurrent);
    }

    ShakeItFarBaby(_pTimer->CurrentTick(), 1.5f);
    autowait(GetModelObject()->GetAnimLength(DEVIL_ANIM_CLIMB)-7.335f-_pTimer->TickQuantum);
    
    SelectRandomAnger();
    GetModelObject()->PlayAnim(m_iAngryAnim, 0);
    PlaySound(m_soSound, m_iAngrySound, SOF_3D);
    autowait( GetModelObject()->GetAnimLength(m_iAngryAnim)-0.1f);

    StopMoving();
    StopRotating();
    TurnOnPhysics();
    autocall GrabBothWeapons() EReturn;

    m_dsDevilState = DS_PYRAMID_FIGHT;
    m_dapAttackPower = DAP_MEDIUM_POWER_ATTACK;
    m_fAttackRadius = 1e6f;
    m_fAttackRunSpeed = 8.0f;
    
    return EReturn();
  }

  TeleportIntoPyramid(EVoid)
  {
    // remove weapons
    RemoveWeapons();
    SetTargetNone();
    Teleport(m_plTeleport, FALSE);
    StopMoving();
    StopRotating();

    SelectRandomAnger();
    GetModelObject()->PlayAnim(m_iAngryAnim, 0);
    PlaySound(m_soSound, m_iAngrySound, SOF_3D);
    autowait( GetModelObject()->GetAnimLength(m_iAngryAnim)-0.1f);

    TurnOnPhysics();
    autocall GrabBothWeapons() EReturn;

    m_dsDevilState = DS_PYRAMID_FIGHT;
    m_dapAttackPower = DAP_MEDIUM_POWER_ATTACK;
    m_fAttackRadius = 1e6f;
    m_fAttackRunSpeed = 8.0f;
    
    return EReturn();
  }

  RegenerationImpulse(EVoid)
  {
    m_dsPreRegenerationDevilState = m_dsDevilState;
    m_dsDevilState = DS_REGENERATION_IMPULSE;
    GetModelObject()->PlayAnim(DEVIL_ANIM_HEAL, 0);
    PlaySound(m_soSound, SOUND_HEAL, SOF_3D);

    StopFireBreathParticles();
    m_tmRegenerationStart = _pTimer->CurrentTick();
    m_tmRegenerationStop = m_tmRegenerationStart+TM_HEALTH_IMPULSE-1.5f/*Regeneration particle life time*/;
    // apply health impulse
    m_tmTemp = _pTimer->CurrentTick();
    while( _pTimer->CurrentTick() < m_tmTemp+TM_HEALTH_IMPULSE)
    {
      wait(_pTimer->TickQuantum) {
        on (EBegin): { resume; };
        on (ETimer): { stop; };
        otherwise(): { resume; };
      }
      SetHealth(GetHealth()+HEALTH_IMPULSE*_pTimer->TickQuantum/TM_HEALTH_IMPULSE);
    }
    m_dsDevilState = m_dsPreRegenerationDevilState;

    return EReturn();
  }

  StopAttack(EVoid) : CEnemyBase::StopAttack {
    if(m_penEnemy==NULL)
    {
      autocall Celebrate() EReturn;
    }

    jump CEnemyBase::StopAttack();
  };

  ContinueInMainLoop(EVoid)
  {
    SwitchToModel();
    // continue behavior in base class
    if( !m_bWasOnceInMainLoop)
    {
      m_bWasOnceInMainLoop = TRUE;
      jump CEnemyBase::MainLoop();
    }
    else
    {
      // get some player for trigger source if any is existing
      CEntity *penEnemy = FixupCausedToPlayer(this, m_penEnemy, /*bWarning=*/FALSE);
      if (penEnemy!=m_penEnemy) {
        SetTargetSoft(penEnemy);
      }

      jump CEnemyBase::StandardBehavior();
    }
  }

  MPIntro(EVoid)
  {
    m_dsDevilState=DS_PYRAMID_FIGHT;
    jump ContinueInMainLoop();
  }

/************************************************************
 *                    D  E  A  T  H                         *
 ************************************************************/
  Death(EVoid) : CEnemyBase::Death {
    SetFlags(GetFlags()&~ENF_ALIVE);
    StopFireBreathParticles();
    StopRegenerationParticles();
    // stop moving
    StopMoving();
    DeathSound();     // death sound
    // set physic flags
    SetCollisionFlags(ECF_MODEL);
    // start death anim
    AnimForDeath();
    autowait(4.66f);
    ShakeItFarBaby(_pTimer->CurrentTick(), 5.0f);
    autowait(GetModelObject()->GetAnimLength(DEVIL_ANIM_DEATH)-4.66f);
    m_tmDeathTime = _pTimer->CurrentTick();
    
    CWorldSettingsController *pwsc = GetWSC(this);
    if (pwsc!=NULL)
    {
      pwsc->m_colGlade=C_WHITE;
      pwsc->m_tmGlaringStarted = _pTimer->CurrentTick()+1.5f;
      pwsc->m_tmGlaringEnded = pwsc->m_tmGlaringStarted+1.0f,
      pwsc->m_fGlaringFadeInRatio = 0.2f;
      pwsc->m_fGlaringFadeOutRatio = 0.7f;
    }
    autowait(1.5f);
    PlaySound(m_soLeft, SOUND_DISAPPEAR, SOF_3D|SOF_VOLUMETRIC);
    autowait(0.25f);
    SwitchToEditorModel();

    PlaySound(m_soRight, SOUND_DEATHPARTICLES, SOF_3D);
    return EEnd();
  };

/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    m_sptType = SPT_NONE;
    // declare yourself as a model
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_WALKING&~EPF_TRANSLATEDBYGRAVITY);
    SetCollisionFlags(((ECBI_MODEL)<<ECB_TEST) | ((ECBI_MODEL)<<ECB_PASS) | ((ECBI_ITEM)<<ECB_IS));
    SetFlags(GetFlags()|ENF_ALIVE);

    // this one is boss!
    m_bBoss = TRUE;
    if( !m_bForMPIntro)
    {
      SetHealth(BOSS_HEALTH);
    }
    else
    {
      SetHealth(5000);
    }
    m_fMaxHealth = BOSS_HEALTH;
    m_fBlowUpAmount = 1e6f;
    m_fBodyParts = 6.0f;
    m_fDamageWounded = 1e9f;
    en_fDensity = 2500.0f;
    m_bHasUpperWeapons = FALSE;
    m_bRenderElectricity = FALSE;
    m_bOnStartPosition = FALSE;
    m_tmGiveUp = UpperLimit(0.0f);
    m_tmDeathTime = -1.0f;

    // setup default speeds and radiuses
    /*
    if( tmp_af[0]==0)
    {
      tmp_af[0]=100.0f;
      tmp_af[1]=200.0f;
      tmp_af[2]=6.0f;
      tmp_af[3]=12.0f;
      tmp_af[4]=15.0f;
    }
    */

    // set your appearance
    SetComponents(this, *GetModelObject(), MODEL_DEVIL, TEXTURE_DEVIL, 0, 0, 0);

    // stretch devil
    GetModelObject()->StretchModel(FLOAT3D(SIZE, SIZE, SIZE));
    ModelChangeNotify();
    
    StandingAnim();

    // setup moving speed
    m_fWalkSpeed = 10.0f;
    m_aWalkRotateSpeed = AngleDeg(90.0f);
    m_fAttackRunSpeed = 8.0f;
    m_aAttackRotateSpeed = AngleDeg(90.0f);
    m_fCloseRunSpeed = 8.0f;
    m_aCloseRotateSpeed = AngleDeg(90.0f);
    // setup attack distances
    m_fAttackDistance = 1e24f;
    m_fCloseDistance = 50.0f;
    m_fStopDistance = 10.0f;
    m_fAttackFireTime = 10.0f;
    m_fCloseFireTime = 5.0f;
    m_fIgnoreRange = UpperLimit(0.0f);
    en_fAcceleration = en_fDeceleration = 50.0f;
    m_fLockOnEnemyTime = 0.05f;
    m_fPauseStretcher = 1.0f;
    m_bWasOnceInMainLoop = FALSE;
 
    // setup light source
    SetupLightSource();
    // set light animation if available
    try {
      m_aoLightAnimation.SetData_t(CTFILENAME("Animations\\BasicEffects.ani"));
    } catch ( const char *strError) {
      WarningMessage(TRANS("Cannot load Animations\\BasicEffects.ani: %s"), strError);
    }
    PlayLightAnim(LIGHT_ANIM_NONE, 0);

    autowait(0.1f);

    m_iScore = 0;

    if( !m_bForMPIntro)
    {
      StartModelAnim(DEVIL_ANIM_POSEDOWN, 0);
      wait() {
        on (EBegin) : { resume; }
        on (ETrigger) : { stop; }
      }
    }
    else
    {
      StartModelAnim(DEVIL_ANIM_DEFAULT, 0);
    }

    SwitchToModel();

    // set initial state
    m_dsDevilState = DS_NOT_EXISTING;    

    wait()
    {
      on (EBegin) : {
        if( cht_bDebugFinalBoss)
        {
          CPrintF("Main loop, event: Begin\n");
        }
        if( !m_bForMPIntro)
        {
          if(m_dsDevilState == DS_NOT_EXISTING)
          {
            m_dsDevilState = DS_DESTROYING_CITY;
            call DestroyCity();
          }
        }
        resume;
      }
      on (ETrigger) :
      {
        if( cht_bDebugFinalBoss)
        {
          CPrintF("Main loop, event: Trigger\n");
        }
        resume;
      }
      on (EDevilCommand eDevilCommand) :
      {
        if( cht_bDebugFinalBoss)
        {
          CTString strDevilCommand = DevilCommandType_enum.NameForValue(INDEX(eDevilCommand.dctType));
          CPrintF("Main loop, event: Devil command: %s\n", (const char *) strDevilCommand);
        }

        if( eDevilCommand.dctType == DC_GRAB_LOWER_WEAPONS)
        {
          m_dapAttackPower = DAP_LOW_POWER_ATTACK;
          m_dsDevilState = DS_ENEMY;
          call GrabLowerWeapons();
        }
        // force given action marker
        else if( eDevilCommand.dctType == DC_FORCE_ACTION)
        {
          m_penAction = eDevilCommand.penForcedAction;
          call DestroyCity();
        }
        else if( eDevilCommand.dctType == DC_STOP_MOVING)
        {
          m_vStartPosition = GetPlacement().pl_PositionVector;
          m_fAttackRadius = 0.0f;
        }
        else if( eDevilCommand.dctType == DC_STOP_ATTACK)
        {
          SetTargetNone();
        }
        else if( eDevilCommand.dctType == DC_JUMP_INTO_PYRAMID)
        {
          GetModelObject()->PlayAnim( DEVIL_ANIM_IDLE, 0);
          m_plTeleport = eDevilCommand.penForcedAction->GetPlacement();
          m_dsDevilState = DS_JUMPING_INTO_PYRAMID;
          call JumpIntoPyramid();
        }
        else if( eDevilCommand.dctType == DC_TELEPORT_INTO_PYRAMID)
        {
          GetModelObject()->PlayAnim( DEVIL_ANIM_IDLE, 0);
          m_plTeleport = eDevilCommand.penForcedAction->GetPlacement();
          m_dsDevilState = DS_JUMPING_INTO_PYRAMID;
          call TeleportIntoPyramid();
        }
        resume;
      }
      on (ERegenerationImpuls) :
      {
        if( cht_bDebugFinalBoss)
        {
          CPrintF("Main loop, event: Regeneration impulse\n");
        }
        m_bRenderElectricity = FALSE;
        call RegenerationImpulse();
        resume;
      }
      on (EHitBySpaceShipBeam) :
      {
        if( cht_bDebugFinalBoss)
        {
          CPrintF("Main loop, event: Hit by space ship beam\n");
        }
        m_bRenderElectricity = FALSE;
        m_tmHitBySpaceShipBeam = _pTimer->CurrentTick();
        call BeamDamage();
        resume;
      }
      // if dead
      on (EDeath eDeath) : {
        if( !(GetFlags()&ENF_ALIVE))
        {
          resume;
        }

        if( cht_bDebugFinalBoss)
        {
          CPrintF("Main loop, event: Death\n");
        }
        // die
        m_bRenderElectricity = FALSE;
        jump CEnemyBase::Die(eDeath);
      }
      on( EEnvironmentStart):
      {
        call MPIntro();
        resume;
      }
      on (EReturn) :
      {
        if( cht_bDebugFinalBoss)
        {
          CPrintF("Main loop, event: Return\n");
        }
        if( m_dsDevilState==DS_DESTROYING_CITY)
        {
          m_soSound.Set3DParameters(1000.0f, 500.0f, 2.0f, 1.0f);
          m_dsDevilState = DS_ENEMY;
          if( m_dapAttackPower == DAP_NOT_ATTACKING)
          {
            m_dapAttackPower = DAP_PLAYER_HUNT;
          }
        }
        call ContinueInMainLoop();
        resume;
      }
    }
  };
};
