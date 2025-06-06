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

346
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/BackgroundViewer.h"
#include "EntitiesMP/WorldSettingsController.h"
#include "EntitiesMP/Common/PathFinding.h"
#include "EntitiesMP/NavigationMarker.h"
#include "ModelsMP/Enemies/ExotechLarva/ExotechLarva.h"
#include "ModelsMP/Enemies/ExotechLarva/Body.h"
#include "ModelsMP/Enemies/ExotechLarva/Arm.h"
#include "ModelsMP/Enemies/ExotechLarva/BackArms.h"
#include "ModelsMP/Enemies/ExotechLarva/Weapons/PlasmaGun.h"
%}

uses "EntitiesMP/ModelHolder2";
uses "EntitiesMP/Projectile";
uses "EntitiesMP/SoundHolder";
uses "EntitiesMP/BloodSpray";
uses "EntitiesMP/CannonBall";
uses "EntitiesMP/ExotechLarvaCharger";
uses "EntitiesMP/LarvaOffspring";

enum LarvaTarget {
  0 LT_NONE      "",   // no target
  1 LT_ENEMY     "",   // follow enemy
  2 LT_RECHARGER "",   // go to a recharger point
};

event ELarvaArmDestroyed {
  INDEX iArm,
};
event ELarvaRechargePose {
  BOOL bStart,  // animate to pose, or return to default
};


%{
// info structure
static EntityInfo eiExotechLarva = {
  EIBT_FLESH, 9999999999.9f,
  0.0f, -1.0f, 0.0f,     // source (eyes)
  0.0f, -1.5f, 0.0f,     // target (body)
  };

#define MF_MOVEZ    (1L<<0)

#define LARVA_HANDLE_TRANSLATE 4.4f
#define FIREPOS_PLASMA_RIGHT  FLOAT3D(+3.08f, -1.20f+LARVA_HANDLE_TRANSLATE, -0.16f)
#define FIREPOS_PLASMA_LEFT   FLOAT3D(-3.08f, -1.20f+LARVA_HANDLE_TRANSLATE, -0.16f)
#define FIREPOS_LASER_RIGHT   FLOAT3D(+2.31f,  0.16f+LARVA_HANDLE_TRANSLATE, -3.57f)
#define FIREPOS_LASER_LEFT    FLOAT3D(-2.20f,  0.18f+LARVA_HANDLE_TRANSLATE, -3.57f)
#define FIREPOS_TAIL          FLOAT3D( 0.00f, -2.64f+LARVA_HANDLE_TRANSLATE, -0.22f)
//#define FIREPOS_MOUTH         FLOAT3D( 0.00f, -0.75f, -2.09f)


// PERCENT_RIGHTBLOW has to be greater then PERCENT_LEFTBLOW or some things
// won't work correctly
#define PERCENT_RIGHTBLOW   0.6666f
#define PERCENT_LEFTBLOW    0.3333f

#define ARM_LEFT    (1L<<0)
#define ARM_RIGHT   (1L<<1)

%}

class CExotechLarva: CEnemyBase {
name      "ExotechLarva";
thumbnail "Thumbnails\\ExotechLarva.tbn";

properties:

 10 CEntityPointer m_penMarkerNew "Larva 1st Grid Marker",
 11 CEntityPointer m_penMarkerOld,
 15 FLOAT m_fStopRadius "Larva MinDist From Player" = 25.0f,
 16 FLOAT m_fStretch = 2.5f,
 17 FLOAT m_fLarvaHealth = 20000.0f,
 19 FLOAT m_fRechargePerSecond "Larva Recharge health/sec" = 100.0f,
 18 enum LarvaTarget m_ltTarget = LT_ENEMY, // type of target
 30 CEntityPointer m_penFirstRechargeTarget "Larva First Recharge target",
 31 BOOL  m_bRechargedAtLeastOnce = FALSE,

 20 FLOAT3D m_vFirePosLeftPlasmaRel  = FLOAT3D(0.0f, 0.0f, 0.0f),
 21 FLOAT3D m_vFirePosRightPlasmaRel = FLOAT3D(0.0f, 0.0f, 0.0f),
 //22 FLOAT3D m_vFirePosMouthRel       = FLOAT3D(0.0f, 0.0f, 0.0f),
 23 FLOAT3D m_vFirePosTailRel        = FLOAT3D(0.0f, 0.0f, 0.0f),
 24 FLOAT3D m_vFirePosLeftLaserAbs   = FLOAT3D(0.0f, 0.0f, 0.0f),
 25 FLOAT3D m_vFirePosRightLaserAbs  = FLOAT3D(0.0f, 0.0f, 0.0f),
 
 40 BOOL m_bLeftArmActive = TRUE,
 41 BOOL m_bRightArmActive = TRUE,
 42 INDEX m_iExplodingArm = 1,
 45 FLOAT m_fMaxRechargedHealth = 1.0f,
 46 BOOL m_bExploding = FALSE,
 47 BOOL m_bActive = TRUE,
 48 BOOL m_bRechargePose = FALSE,
 49 BOOL m_bLaserActive = FALSE,
 
 51 BOOL m_bInitialMove = TRUE,
 
 54 CEntityPointer m_penRecharger "Larva Recharger" COLOR(C_GREEN|0xFF),

 60 FLOAT m_tmLastTargateChange = 0.0f,

 // internal positions for explosions
 70 CPlacement3D m_plExpArmPos = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0)),
 71 FLOAT3D m_aExpArmRot = FLOAT3D(0.0f, 0.0f, 0.0f),
 72 CPlacement3D m_plExpGunPos = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0)),
 73 FLOAT3D m_aExpGunRot = FLOAT3D(0.0f, 0.0f, 0.0f),
 74 FLOAT3D m_vExpDamage = FLOAT3D(0.0f, 0.0f, 0.0f),
 75 INDEX m_iExplosions = 0,

 80 INDEX m_iRnd = 0,  // temporary holder for random variable
 81 BOOL m_bRecharging = FALSE, // internal
 
 // temporary variables for reconstructing lost events
 90 CEntityPointer m_penDeathInflictor,

 100 FLOAT   m_tmDontFireLaserBefore = 0.0f,
 101 FLOAT   m_fMinimumLaserWait "Larva min. laser interval" = 5.0f,
 102 BOOL    m_bRenderLeftLaser  = FALSE,
 103 BOOL    m_bRenderRightLaser = FALSE,
 104 FLOAT3D m_vLeftLaserTarget  = FLOAT3D(0.0f, 0.0f, 0.0f),
 105 FLOAT3D m_vRightLaserTarget = FLOAT3D(0.0f, 0.0f, 0.0f),

 110 BOOL    m_bInvulnerable = FALSE,

 150 CEntityPointer m_penLeftArmDestroyTarget "Larva ArmBlow#1 Target",
 151 CEntityPointer m_penRightArmDestroyTarget "Larva ArmBlow#2 Target",
 152 CEntityPointer m_penDeathTarget "Larva Death Target",

 200 CSoundObject m_soFire1,  // sound channel for firing
 201 CSoundObject m_soFire2,  // sound channel for firing
 202 CSoundObject m_soFire3,  // sound channel for firing
 203 CSoundObject m_soVoice,  // sound channel for voice
 204 CSoundObject m_soChirp,  // sound channel for chirping
 205 CSoundObject m_soLaser,  // sound channel for laser

components:
  1 class CLASS_BASE           "Classes\\EnemyBase.ecl",
  2 class CLASS_BASIC_EFFECT   "Classes\\BasicEffect.ecl",
  3 class CLASS_PROJECTILE     "Classes\\Projectile.ecl",
  4 class CLASS_BLOOD_SPRAY    "Classes\\BloodSpray.ecl",
  5 class CLASS_LARVAOFFSPRING "Classes\\LarvaOffspring.ecl",
  
  10 model   MODEL_EXOTECHLARVA   "ModelsMP\\Enemies\\ExotechLarva\\ExotechLarva.mdl",
  11 texture TEXTURE_EXOTECHLARVA "ModelsMP\\Enemies\\ExotechLarva\\ExotechLarva.tex",
  
  12 model   MODEL_BODY           "ModelsMP\\Enemies\\ExotechLarva\\Body.mdl", 
  13 texture TEXTURE_BODY         "ModelsMP\\Enemies\\ExotechLarva\\Body.tex",  
  
  14 model   MODEL_BEAM           "ModelsMP\\Enemies\\ExotechLarva\\Beam.mdl",  
  15 texture TEXTURE_BEAM         "ModelsMP\\Effects\\Laser\\Laser.tex",  

  16 model   MODEL_ENERGYBEAMS    "ModelsMP\\Enemies\\ExotechLarva\\EnergyBeams.mdl",  
  17 texture TEXTURE_ENERGYBEAMS  "ModelsMP\\Enemies\\ExotechLarva\\EnergyBeams.tex",  

  18 model   MODEL_FLARE          "ModelsMP\\Enemies\\ExotechLarva\\EffectFlare.mdl",  
  19 texture TEXTURE_FLARE        "ModelsMP\\Enemies\\ExotechLarva\\EffectFlare.tex",  
  
  30 model   MODEL_WING           "ModelsMP\\Enemies\\ExotechLarva\\Arm.mdl",
  31 texture TEXTURE_WING         "ModelsMP\\Enemies\\ExotechLarva\\Arm.tex",  

  32 model   MODEL_PLASMAGUN      "ModelsMP\\Enemies\\ExotechLarva\\Weapons\\PlasmaGun.mdl",
  33 texture TEXTURE_PLASMAGUN    "ModelsMP\\Enemies\\ExotechLarva\\Weapons\\PlasmaGun.tex",
  
  34 model   MODEL_BLADES         "ModelsMP\\Enemies\\ExotechLarva\\BackArms.mdl",

  36 model   MODEL_DEBRIS_BODY    "ModelsMP\\Enemies\\ExotechLarva\\Debris\\BodyDebris.mdl",
  37 model   MODEL_DEBRIS_TAIL01  "ModelsMP\\Enemies\\ExotechLarva\\Debris\\TailDebris01.mdl",
  38 model   MODEL_DEBRIS_TAIL02  "ModelsMP\\Enemies\\ExotechLarva\\Debris\\TailDebris02.mdl",

  39 model   MODEL_DEBRIS_FLESH   "Models\\Effects\\Debris\\Flesh\\Flesh.mdl",
  40 texture TEXTURE_DEBRIS_FLESH "Models\\Effects\\Debris\\Flesh\\FleshRed.tex",

  41 model   MODEL_PLASMA         "ModelsMP\\Enemies\\ExotechLarva\\Projectile\\Projectile.mdl",
  42 texture TEXTURE_PLASMA       "ModelsMP\\Enemies\\ExotechLarva\\Projectile\\Projectile.tex",


  // ************** SOUNDS **************
  50 sound   SOUND_FIRE_PLASMA    "ModelsMP\\Enemies\\ExotechLarva\\Sounds\\FirePlasma.wav",
  51 sound   SOUND_FIRE_TAIL      "ModelsMP\\Enemies\\ExotechLarva\\Sounds\\FireTail.wav",
  52 sound   SOUND_LASER_CHARGE   "ModelsMP\\Enemies\\ExotechLarva\\Sounds\\LaserCharge.wav",
  53 sound   SOUND_DEATH          "ModelsMP\\Enemies\\ExotechLarva\\Sounds\\Death.wav",
  54 sound   SOUND_ARMDESTROY     "ModelsMP\\Enemies\\ExotechLarva\\Sounds\\ArmDestroy.wav",
  55 sound   SOUND_CHIRP          "ModelsMP\\Enemies\\ExotechLarva\\Sounds\\Chirp.wav",
  56 sound   SOUND_DEPLOYLASER    "ModelsMP\\Enemies\\ExotechLarva\\Sounds\\DeployLaser.wav",

functions:                                        
  
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if( slPropertyOffset == _offsetof(CExotechLarva, m_penMarkerNew))
    {
      if (IsOfClass(penTarget, "NavigationMarker")) { return TRUE; }
      else { return FALSE; }
    }   
    if( slPropertyOffset == _offsetof(CExotechLarva, m_penRecharger))
    {
      if (IsOfClass(penTarget, "ExotechLarvaCharger")) { return TRUE; }
      else { return FALSE; }
    }   
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }
  
  BOOL DoSafetyChecks(void) {
    if (m_penMarkerNew==NULL) {
      WarningMessage("First ExotechLarva marker not set! Destroying Larva...\n");
      return FALSE;
    }
    if (m_penRecharger==NULL) {
      WarningMessage("ExotechLarva Recharger target not set! Destroying Larva...\n");
      return FALSE;
    }
    return TRUE;
  }

  void FindNewTarget() {
    
    // if we have a valid enemy, return
    if (m_penEnemy!=NULL) {
      if (m_penEnemy->GetFlags()&ENF_ALIVE && !(m_penEnemy->GetFlags()&ENF_DELETED)) {
        return;  
      }
    }

    // find actual number of players
    INDEX ctMaxPlayers = GetMaxPlayers();
    CEntity *penPlayer;
    
    for(INDEX i=0; i<ctMaxPlayers; i++) {
      penPlayer=GetPlayerEntity(i);
      if (penPlayer!=NULL && DistanceTo(this, penPlayer)<200.0f) {
        // if there is no valid enemy
        if (penPlayer!=NULL && (penPlayer->GetFlags()&ENF_ALIVE) && 
          !(penPlayer->GetFlags()&ENF_DELETED)) {
          m_penEnemy = penPlayer;
        }      
      }
    }
  }

  BOOL AnyPlayerCloserThen(FLOAT fDistance) {
    
    BOOL bClose = FALSE;

    // find actual number of players
    INDEX ctMaxPlayers = GetMaxPlayers();
    CEntity *penPlayer;
    
    for(INDEX i=0; i<ctMaxPlayers; i++) {
      penPlayer=GetPlayerEntity(i);
      if (penPlayer!=NULL) {
        if ((penPlayer->GetFlags()&ENF_ALIVE) && 
            !(penPlayer->GetFlags()&ENF_DELETED) &&
            DistanceTo(this, penPlayer)<fDistance)
        {
          bClose = TRUE;
        }      
      }
    }   
    return bClose;
  }

  void PerhapsChangeTarget() {
    // if no current enemy, do nothing
    if (!m_penEnemy) { return; }
    // if enough time passed, try...
    if (m_tmLastTargateChange+5.0f<_pTimer->CurrentTick()) {
      MaybeSwitchToAnotherPlayer();
      m_tmLastTargateChange = _pTimer->CurrentTick();
    }
  }
    
  class CWorldSettingsController *GetWSC(void)
  {
    CWorldSettingsController *pwsc = NULL;
    // obtain bcg viewer
    CBackgroundViewer *penBcgViewer = (CBackgroundViewer *) GetWorld()->GetBackgroundViewer();
    if( penBcgViewer != NULL) {
      // obtain world settings controller 
      pwsc = (CWorldSettingsController *) penBcgViewer->m_penWorldSettingsController.ep_pen;
    }
    return pwsc;
  }

  /* Shake ground */
  void ShakeItBaby(FLOAT tmShaketime, FLOAT fPower, BOOL bFadeIn)
  {
    CWorldSettingsController *pwsc = GetWSC();
    if (pwsc!=NULL) {
      pwsc->m_tmShakeStarted = tmShaketime;
      pwsc->m_vShakePos = GetPlacement().pl_PositionVector;
      pwsc->m_fShakeFalloff = 450.0f;
      pwsc->m_fShakeFade = 3.0f;

      pwsc->m_fShakeIntensityZ = 0;
      pwsc->m_tmShakeFrequencyZ = 5.0f;
      pwsc->m_fShakeIntensityY = 0.1f*fPower;
      pwsc->m_tmShakeFrequencyY = 5.0f;
      pwsc->m_fShakeIntensityB = 2.5f*fPower;
      pwsc->m_tmShakeFrequencyB = 7.2f;

      pwsc->m_bShakeFadeIn = bFadeIn;
    }
  }

  void ShootTailProjectile(void) {
	//ShootProjectile(PRT_LARVA_TAIL_PROJECTILE, m_vFirePosTailRel, ANGLE3D(0, -10, 0));
    if (m_penEnemy == NULL) { return; }

    // target enemy body
    EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
    FLOAT3D vShootTarget;
    GetEntityInfoPosition(m_penEnemy, peiTarget->vTargetCenter, vShootTarget);

    // launch
    CPlacement3D pl;
    PreparePropelledProjectile(pl, vShootTarget, m_vFirePosTailRel, ANGLE3D(0, -10, 0));
    CEntityPointer penProjectile = CreateEntity(pl, CLASS_LARVAOFFSPRING);
    ELaunchLarvaOffspring ello;
    ello.penLauncher = this;
    penProjectile->Initialize(ello);
  }

  BOOL IsOnMarker(CEntity *penMarker)  {
    
    if (penMarker==NULL) { return FALSE; }
    if (DistanceTo(this, penMarker)<0.1f) { return TRUE; }
    // else
    return FALSE;
  }

  FLOAT DistanceXZ(CEntity *E1, CEntity *E2)
  {
    FLOAT3D vE1pos = E1->GetPlacement().pl_PositionVector;
    FLOAT3D vE2pos = E2->GetPlacement().pl_PositionVector;
    vE1pos(2)=0.0f;
    vE2pos(2)=0.0f;
    return (vE2pos - vE1pos).Length();
  }

  void SpawnWingDebris()
  {
    FLOAT3D vTranslation = m_vExpDamage + en_vCurrentTranslationAbsolute;
    
    Debris_Begin(EIBT_FLESH, DPT_BLOODTRAIL, BET_BLOODSTAIN, 1.0f, m_vExpDamage, en_vCurrentTranslationAbsolute, 5.0f, 2.0f);
    Debris_Spawn_Independent(this, this, MODEL_WING, TEXTURE_WING, 0, 0, 0, 0, m_fStretch,
                  m_plExpArmPos, vTranslation , m_aExpArmRot);
    vTranslation += FLOAT3D(FRnd()*4.0f-2.0f, FRnd()*4.0f-2.0f, FRnd()*4.0f-2.0f);
    Debris_Spawn_Independent(this, this, MODEL_PLASMAGUN, TEXTURE_PLASMAGUN, 0, 0, 0, 0, m_fStretch,
                  m_plExpGunPos, vTranslation , m_aExpGunRot);
  }
    

  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    
    if (m_bInvulnerable) {
      return;
    }
    
    // cannot hurt ourselves
    if (IsOfClass(penInflictor, "ExotechLarva")) {
      return;
    }

    // preliminary adjustment of damage
    // take less damage from heavy bullets (e.g. sniper)
    if(dmtType==DMT_BULLET && fDamageAmmount>100.0f)
    {
      fDamageAmmount *= 0.66f;
    }
    // cannonballs inflict less damage then the default
    if(dmtType==DMT_CANNONBALL)
    {
      fDamageAmmount *= 0.5f;
    }
    


    FLOAT fHealthNow = GetHealth();
    FLOAT fHealthAfter = GetHealth() - fDamageAmmount;
    FLOAT fHealthBlow01 = m_fMaxHealth*PERCENT_RIGHTBLOW;
    FLOAT fHealthBlow02 = m_fMaxHealth*PERCENT_LEFTBLOW; 

    // adjust damage
    fDamageAmmount *=DamageStrength( ((EntityInfo*)GetEntityInfo())->Eeibt, dmtType);
    // apply game extra damage per enemy and per player
    fDamageAmmount *=GetGameDamageMultiplier();

    // enough damage to blow both arms
    if (fHealthNow>fHealthBlow01 && fHealthAfter<fHealthBlow02) {
        fDamageAmmount = fHealthNow - fHealthBlow01 - 1;
    } else if (m_bExploding) {
      // if damage would cause the second explosion
      if (fHealthNow>fHealthBlow02 && fHealthAfter<fHealthBlow02) {
        fDamageAmmount = fHealthNow - fHealthBlow02 - 1;
        // make sure we don't die while exploding
      } else if (fHealthAfter<0.0f) {
        fDamageAmmount = fHealthNow - 1;
      }
    } else if (fHealthNow>fHealthBlow02 && fHealthAfter<0) {
      fDamageAmmount = fHealthNow - 1;
    }
        
    // if no damage
    if (fDamageAmmount ==0) {
      // do nothing
      return;
    }

    // spawn blood spray
    CPlacement3D plSpray = CPlacement3D( vHitPoint, ANGLE3D(0, 0, 0));
    m_penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
    ESpawnSpray eSpawnSpray;
    eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
    if( m_fMaxDamageAmmount > 10.0f)
    {
      eSpawnSpray.fDamagePower = 3.0f;
    }
    else if(m_fSprayDamage+fDamageAmmount>50.0f)
    {
      eSpawnSpray.fDamagePower = 2.0f;
    }
    else
    {
      eSpawnSpray.fDamagePower = 1.0f;
    }
    switch(IRnd()%4) {
    case 0: case 1: case 2:
      // blood spray
      m_penSpray->SetParent(this);
      eSpawnSpray.sptType = SPT_BLOOD;
      break;
    case 3:
      // sparks spray
      eSpawnSpray.sptType = SPT_ELECTRICITY_SPARKS;
      break;
    }
    eSpawnSpray.fSizeMultiplier = 1.0f;
    // setup direction of spray
    FLOAT3D vHitPointRelative = vHitPoint - GetPlacement().pl_PositionVector;
    FLOAT3D vReflectingNormal;
    GetNormalComponent( vHitPointRelative, en_vGravityDir, vReflectingNormal);
    vReflectingNormal.Normalize();
    
    vReflectingNormal(1)/=5.0f;
    
    FLOAT3D vProjectedComponent = vReflectingNormal*(vDirection%vReflectingNormal);
    FLOAT3D vSpilDirection = vDirection-vProjectedComponent*2.0f-en_vGravityDir*0.5f;
    
    eSpawnSpray.vDirection = vSpilDirection;
    eSpawnSpray.penOwner = this;
    
    // initialize spray
    m_penSpray->Initialize( eSpawnSpray);
    m_tmSpraySpawned = _pTimer->CurrentTick();
    m_fSprayDamage = 0.0f;
    m_fMaxDamageAmmount = 0.0f;

    // instead of:
    // CMovableModelEntity::ReceiveDamage(penInflictor, dmtType, fDamageAmmount,
    //                          vHitPoint, vDirection);
    // do this (because we don't want an event posted each time we're damaged):
    
    // reduce your health
    en_fHealth-=fDamageAmmount;
    // if health reached zero
    if (en_fHealth<=0) {
      // throw an event that you have died
      EDeath eDeath;
      SendEvent(eDeath);
    }
    
    if (m_bRightArmActive) {
      if (GetHealth()<m_fMaxHealth*PERCENT_RIGHTBLOW) {
        ELarvaArmDestroyed ead;
        ead.iArm = ARM_RIGHT;
        SendEvent(ead);
        m_bExploding = TRUE;
      }
    }
    if (m_bLeftArmActive) {
      if (GetHealth()<m_fMaxHealth*PERCENT_LEFTBLOW) {
        ELarvaArmDestroyed ead;
        ead.iArm = ARM_LEFT;
        SendEvent(ead);
        m_bExploding = TRUE;
      }
    }

    // bosses don't darken when burning
    m_colBurning=COLOR(C_WHITE|CT_OPAQUE);

  }

  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath) {
    CTString str;
    str.PrintF(TRANSV("Exotech larva reduced %s to pulp."), (const char *) strPlayerName);
    return str;
  }

  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiExotechLarva;
  };

  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnmLarva, "DataMP\\Messages\\Enemies\\ExotechLarva.txt");
    return fnmLarva;
  };

  void Precache(void) {
    CEnemyBase::Precache();
    
    PrecacheClass(CLASS_BASIC_EFFECT, BET_ROCKET  );
    PrecacheClass(CLASS_BASIC_EFFECT, BET_CANNON  );
    PrecacheClass(CLASS_BLOOD_SPRAY   );
    PrecacheClass(CLASS_PROJECTILE, PRT_LARVA_TAIL_PROJECTILE);
    PrecacheClass(CLASS_PROJECTILE, PRT_LARVA_PLASMA);

    PrecacheModel  (MODEL_EXOTECHLARVA   );
    PrecacheTexture(TEXTURE_EXOTECHLARVA );
    PrecacheModel  (MODEL_BODY           );
    PrecacheTexture(TEXTURE_BODY         );
    PrecacheModel  (MODEL_BEAM           );
    PrecacheTexture(TEXTURE_BEAM         );
    PrecacheModel  (MODEL_ENERGYBEAMS    );
    PrecacheTexture(TEXTURE_ENERGYBEAMS  );
    PrecacheModel  (MODEL_FLARE          );
    PrecacheTexture(TEXTURE_FLARE        );
    PrecacheModel  (MODEL_WING           );
    PrecacheTexture(TEXTURE_WING         );
    PrecacheModel  (MODEL_PLASMAGUN      );
    PrecacheTexture(TEXTURE_PLASMAGUN    );
    
    PrecacheModel  (MODEL_BLADES         );
    PrecacheModel  (MODEL_DEBRIS_BODY    );
    PrecacheModel  (MODEL_DEBRIS_TAIL01  );
    PrecacheModel  (MODEL_DEBRIS_TAIL02  );
    
    PrecacheModel  (MODEL_DEBRIS_FLESH   );
    PrecacheTexture(TEXTURE_DEBRIS_FLESH );
    PrecacheModel  (MODEL_PLASMA         );
    PrecacheTexture(TEXTURE_PLASMA       );
    PrecacheModel  (MODEL_BODY           );
    PrecacheTexture(TEXTURE_BODY           );

    PrecacheSound(SOUND_FIRE_PLASMA   );
    PrecacheSound(SOUND_FIRE_TAIL     );
    PrecacheSound(SOUND_LASER_CHARGE  );
    PrecacheSound(SOUND_DEATH         );
    PrecacheSound(SOUND_ARMDESTROY    );
    PrecacheSound(SOUND_CHIRP         );
    PrecacheSound(SOUND_DEPLOYLASER   );

  };

  // get the plasma ball attachments
  CModelObject *PlasmaLeftModel(void) {
    CAttachmentModelObject *amo = GetModelObject()->GetAttachmentModel(BODY_ATTACHMENT_ARM_LEFT);
    amo = amo->amo_moModelObject.GetAttachmentModel(ARM_ATTACHMENT_PLASMAGUN);
    return &(amo->amo_moModelObject);
  };
  CModelObject *PlasmaRightModel(void) {
    CAttachmentModelObject *amo = GetModelObject()->GetAttachmentModel(BODY_ATTACHMENT_ARM_RIGHT);
    amo = amo->amo_moModelObject.GetAttachmentModel(ARM_ATTACHMENT_PLASMAGUN);
    return &(amo->amo_moModelObject);
  };

  BOOL RechargerActive() {
    if (((CExotechLarvaCharger *) m_penRecharger.ep_pen)->m_bActive) { 
      return TRUE;
    }
    return FALSE;
  }

  /*void CheckRechargerTargets(void) {
    m_bRechargerExists = FALSE;
    
    CEntityPointer *penModel  = &m_penRCModel01;
    CEntityPointer *penMarker = &m_penRCMarker01;

    for (INDEX i=0; i<4; i++) {
      // if model pointer is valid and model is not destroyed
      if (&*penModel[i]) {
        if (!((*penModel[i]).en_ulFlags&ENF_DELETED)) { 
          // at least one exists
          m_bRechargerExists = TRUE;
        } else if (m_penRechargerTarget==&*penMarker[i]) {
          m_penRechargerTarget=NULL;
          penMarker[i] = NULL;  
        }
      // otherwise make sure it is not the current recharging target
      } else if (m_penRechargerTarget==&*penMarker[i]) {
        m_penRechargerTarget=NULL;
        penMarker[i] = NULL;  
      }
    }
  }

  BOOL CurrentRechargerExists(void) {
    if (m_penRechargerTarget) {
        if (!(m_penRechargerTarget->en_ulFlags&ENF_DELETED)) { return TRUE; };
    }
    return FALSE;
  }

  BOOL FindClosestRecharger(void) {
    FLOAT fDistance = UpperLimit(1.0f);
    m_penRechargerTarget = NULL;
    if (m_penRCMarker01) {
      if (DistanceTo(this, m_penRCMarker01)<fDistance) {
        m_penRechargerTarget = m_penRCMarker01;
        fDistance = DistanceTo(this, m_penRCMarker01);
      }
    }
    if (m_penRCMarker02) {
      if (DistanceTo(this, m_penRCMarker02)<fDistance) {
        m_penRechargerTarget = m_penRCMarker02;
        fDistance = DistanceTo(this, m_penRCMarker02);
      }
    }
    if (m_penRCMarker03) {
      if (DistanceTo(this, m_penRCMarker03)<fDistance) {
        m_penRechargerTarget = m_penRCMarker03;
        fDistance = DistanceTo(this, m_penRCMarker03);
      }
    }
    if (m_penRCMarker04) {
      if (DistanceTo(this, m_penRCMarker04)<fDistance) {
        m_penRechargerTarget = m_penRCMarker04;
      }
    }
    return (m_penRechargerTarget==NULL ? FALSE : TRUE);
  }*/

  void RemoveWing(INDEX iArm) {
    // right arm
    if (iArm==ARM_RIGHT) {
      RemoveAttachmentFromModel(*GetModelObject(), BODY_ATTACHMENT_ARM_RIGHT);
    }
    // left arm
    if (iArm==ARM_LEFT) {
      RemoveAttachmentFromModel(*GetModelObject(), BODY_ATTACHMENT_ARM_LEFT);
    }
  }

  ANGLE GetArmsPitch(void) {
    if (m_bLeftArmActive) {
      CAttachmentModelObject &amo = *GetModelObject()->GetAttachmentModel(BODY_ATTACHMENT_ARM_LEFT);
      return (amo.amo_plRelative.pl_OrientationAngle(2) + GetPlacement().pl_OrientationAngle(2)); 
    }
    return 0.0f;
  }

  ULONG SetDesiredMovement(void) 
  {
    ULONG ulFlags = 0;
    FLOAT3D vPos;
    CEntity *penMarker = m_penMarkerNew;
    CEntity *penTarget;

    if (m_ltTarget==LT_ENEMY && m_penEnemy) { penTarget = m_penEnemy; }
    else if (m_ltTarget==LT_RECHARGER) { penTarget = m_penRecharger; }
    else { return ulFlags; }

    // CPrintF("target = %s at %f\n", penTarget->GetName(), _pTimer->CurrentTick());

    if (IsOnMarker(m_penMarkerNew)) {
      PATH_FindNextMarker(penTarget, GetPlacement().pl_PositionVector,
        penTarget->GetPlacement().pl_PositionVector, penMarker, vPos);
      if (penMarker!=NULL) {
        // remember the old marker
        m_penMarkerOld = m_penMarkerNew;
        // and set the new target
        m_penMarkerNew = penMarker;
      }
      MoveToMarker(m_penMarkerNew);
      ulFlags |= MF_MOVEZ;
    } else {
      MoveToMarker(m_penMarkerNew);
      ulFlags |= MF_MOVEZ;
    }
   
    if (m_ltTarget==LT_ENEMY && DistanceTo(this, penTarget)<m_fStopRadius) {
      ForceFullStop();
    }
    
    return ulFlags;
  };

  void MoveToMarker(CEntity *penMarker) {
    if(penMarker==NULL) { return; }
    FLOAT3D vDesiredDir = penMarker->GetPlacement().pl_PositionVector -
                   GetPlacement().pl_PositionVector;
    if (vDesiredDir.Length()>0.0f) {
      vDesiredDir.Normalize();
      FLOAT3D vSpeed = vDesiredDir*m_fAttackRunSpeed;
      SetDesiredTranslation(vSpeed);
    }
  }

  // pre moving
  void PreMoving() {
    
    if (m_bActive && !m_bRenderLeftLaser && !m_bRenderRightLaser) {
      
      // rotate to enemy
      if (m_penEnemy!=NULL) {
        
        FLOAT3D vToEnemy;
        vToEnemy = (m_penEnemy->GetPlacement().pl_PositionVector - 
          GetPlacement().pl_PositionVector).Normalize();
        ANGLE3D aAngle;
        DirectionVectorToAngles(vToEnemy, aAngle);
        aAngle(1) = aAngle(1) - GetPlacement().pl_OrientationAngle(1);
        aAngle(1) = NormalizeAngle(aAngle(1));
        SetDesiredRotation(FLOAT3D(aAngle(1)*2.0f, 0.0f, 0.0f));         
      } else {
        SetDesiredRotation(FLOAT3D(0.0f, 0.0f, 0.0f));
      }
      
      // lower speed if needed, not to miss the marker
      if (en_vCurrentTranslationAbsolute.Length()*_pTimer->TickQuantum*2.0f >
        DistanceTo(this, m_penMarkerNew)) {
        FLOAT3D vToMarker = m_penMarkerNew->GetPlacement().pl_PositionVector -
          GetPlacement().pl_PositionVector;
        SetDesiredTranslation(vToMarker/_pTimer->TickQuantum) ;        
      }
        
      // stop when on marker
      if (IsOnMarker(m_penMarkerNew)) {
        ForceStopTranslation();
      }
    } else {
        ForceFullStop();
    }
    
    CEnemyBase::PreMoving();
  }

  void RenderParticles(void)
  {
    
    FLOATmatrix3D m;
    CPlacement3D plLarva;

    if (m_bRenderLeftLaser || m_bRenderRightLaser) {
      plLarva = GetLerpedPlacement();
      MakeRotationMatrix(m, plLarva.pl_OrientationAngle);      
    }

    if (m_bRenderLeftLaser) {
      FLOAT3D vSource = (FIREPOS_LASER_LEFT*m_fStretch)*m + plLarva.pl_PositionVector;
      Particles_ExotechLarvaLaser(this, vSource, m_vLeftLaserTarget);
    }
    if (m_bRenderRightLaser) {
      FLOAT3D vSource = (FIREPOS_LASER_RIGHT*m_fStretch)*m + plLarva.pl_PositionVector;
      Particles_ExotechLarvaLaser(this, vSource, m_vRightLaserTarget);
    }
    if (m_bRechargePose && ((CExotechLarvaCharger *) m_penRecharger.ep_pen)->m_bBeamActive)
    {
      Particles_LarvaEnergy(this, FLOAT3D(0.0f, LARVA_HANDLE_TRANSLATE, 0.0f)*m_fStretch);
    }
  }

  void SizeModel(void)
  {
    return;
  }

  void UpdateFiringPos() {    
    m_vFirePosLeftLaserAbs  = (FIREPOS_LASER_LEFT*m_fStretch)*GetRotationMatrix() + GetPlacement().pl_PositionVector;
    m_vFirePosRightLaserAbs = (FIREPOS_LASER_RIGHT*m_fStretch)*GetRotationMatrix() + GetPlacement().pl_PositionVector;    
  }

  void BlowUp(void)
  {
    NOTHING;
  }
  
  // adjust sound and watcher parameters here if needed
  void EnemyPostInit(void) 
  {
    m_soFire1.Set3DParameters(600.0f, 150.0f, 2.0f, 1.0f);
    m_soFire2.Set3DParameters(600.0f, 150.0f, 2.0f, 1.0f);
    m_soFire3.Set3DParameters(600.0f, 150.0f, 2.0f, 1.0f);
    m_soVoice.Set3DParameters(600.0f, 150.0f, 2.0f, 1.0f);
    m_soChirp.Set3DParameters(150.0f, 50.0f, 2.0f, 1.0f);
    m_soLaser.Set3DParameters(300.0f, 200.0f, 3.0f, 1.0f);
  }

  void FireLaser(void)
  {
    
    FLOAT3D vLaserTarget;

    if (!m_penEnemy) { return; }

    if (IsVisible(m_penEnemy)) {
      vLaserTarget = m_penEnemy->GetPlacement().pl_PositionVector;
    } else if (TRUE) {
      vLaserTarget = m_vPlayerSpotted;
    }

    // cast 1st ray
    CCastRay crRay1( this, m_vFirePosLeftLaserAbs, vLaserTarget);
    crRay1.cr_fTestR = 0.10f;
    crRay1.cr_bHitTranslucentPortals = FALSE;
    crRay1.cr_bPhysical = FALSE;
    crRay1.cr_ttHitModels = CCastRay::TT_COLLISIONBOX;
    GetWorld()->CastRay(crRay1);
    
    // if entity is hit
    if( crRay1.cr_penHit != NULL) {
      m_bRenderLeftLaser = TRUE;
      m_vLeftLaserTarget = crRay1.cr_vHit;

      // apply damage
      InflictDirectDamage( crRay1.cr_penHit, this, DMT_BURNING, 25.0f,
          FLOAT3D(0, 0, 0), (m_vFirePosLeftLaserAbs-m_vLeftLaserTarget).Normalize());
      
      if (crRay1.cr_penHit->GetRenderType()!=RT_BRUSH) {
        crRay1.cr_ttHitModels = CCastRay::TT_NONE;
        GetWorld()->ContinueCast(crRay1);
        if (crRay1.cr_penHit != NULL) {
          m_vLeftLaserTarget = crRay1.cr_vHit;
        }
      }
    } else if (TRUE) {
      m_bRenderLeftLaser = FALSE;
    }
    
    // cast 2nd ray
    CCastRay crRay2( this, m_vFirePosRightLaserAbs, vLaserTarget);
    crRay2.cr_fTestR = 0.10f;
    crRay2.cr_bHitTranslucentPortals = FALSE;
    crRay2.cr_bPhysical = FALSE;
    crRay2.cr_ttHitModels = CCastRay::TT_COLLISIONBOX;
    GetWorld()->CastRay(crRay2);
    
    // if entity is hit
    if( crRay2.cr_penHit != NULL) {
      m_bRenderRightLaser = TRUE;
      m_vRightLaserTarget = crRay2.cr_vHit;

      // apply damage
      InflictDirectDamage( crRay2.cr_penHit, this, DMT_BURNING, 25.0f,
          FLOAT3D(0, 0, 0), (m_vFirePosRightLaserAbs-m_vRightLaserTarget).Normalize());

      if (crRay2.cr_penHit->GetRenderType()!=RT_BRUSH) {
        crRay2.cr_ttHitModels = CCastRay::TT_NONE;
        GetWorld()->ContinueCast(crRay2);
        if (crRay2.cr_penHit != NULL) {
          m_vRightLaserTarget = crRay2.cr_vHit;
        }
      }
    } else if (TRUE) {
      m_bRenderRightLaser = FALSE;
    }
  }
  
  void ExplodeLaser(void)
  {
    if (m_bRenderLeftLaser) {
      ESpawnEffect eSpawnEffect;
      eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
      eSpawnEffect.betType = BET_CANNON;
      eSpawnEffect.vStretch = FLOAT3D(m_fStretch*0.5f, m_fStretch*0.5f, m_fStretch*0.5f);
      CEntityPointer penExplosion = CreateEntity(CPlacement3D(m_vLeftLaserTarget,
        ANGLE3D(0.0f, 0.0f, 0.0f)), CLASS_BASIC_EFFECT);
      penExplosion->Initialize(eSpawnEffect);
      
        // explosion debris
      eSpawnEffect.betType = BET_EXPLOSION_DEBRIS;
      penExplosion = CreateEntity(CPlacement3D(m_vLeftLaserTarget, 
        ANGLE3D(0.0f, 0.0f, 0.0f)), CLASS_BASIC_EFFECT);
      penExplosion->Initialize(eSpawnEffect);

      // explosion smoke
      eSpawnEffect.betType = BET_EXPLOSION_SMOKE;
      penExplosion = CreateEntity(CPlacement3D(m_vLeftLaserTarget, 
        ANGLE3D(0.0f, 0.0f, 0.0f)), CLASS_BASIC_EFFECT);
      penExplosion->Initialize(eSpawnEffect);

      InflictRangeDamage( this, DMT_EXPLOSION, 25.0f,
        m_vLeftLaserTarget, 5.0f, 25.0f);
    }
    
    if (m_bRenderRightLaser) {
      ESpawnEffect eSpawnEffect;
      eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
      eSpawnEffect.betType = BET_CANNON;
      eSpawnEffect.vStretch = FLOAT3D(m_fStretch*0.5f, m_fStretch*0.5f, m_fStretch*0.5f);
      CEntityPointer penExplosion = CreateEntity(CPlacement3D(m_vLeftLaserTarget,
        ANGLE3D(0.0f, 0.0f, 0.0f)), CLASS_BASIC_EFFECT);
      penExplosion->Initialize(eSpawnEffect);
      
      // explosion debris
      eSpawnEffect.betType = BET_EXPLOSION_DEBRIS;
      penExplosion = CreateEntity(CPlacement3D(m_vLeftLaserTarget, 
        ANGLE3D(0.0f, 0.0f, 0.0f)), CLASS_BASIC_EFFECT);
      penExplosion->Initialize(eSpawnEffect);

      // explosion smoke
      eSpawnEffect.betType = BET_EXPLOSION_SMOKE;
      penExplosion = CreateEntity(CPlacement3D(m_vLeftLaserTarget, 
        ANGLE3D(0.0f, 0.0f, 0.0f)), CLASS_BASIC_EFFECT);
      penExplosion->Initialize(eSpawnEffect);

      InflictRangeDamage( this, DMT_EXPLOSION, 25.0f,
        m_vLeftLaserTarget, 5.0f, 25.0f);
    }    
  }
  

/****************************************/
/*         P R O C E D U R E S          */
/****************************************/

procedures:
  
  // override wounding so that Larva doesn't stutter
  BeWounded(EDamage eDamage) : CEnemyBase::BeWounded { 
    return EReturn();
  };

  ArmExplosion()
  {
    FLOATmatrix3D mRot;
    FLOAT3D       vPos;
    
    m_bActive = FALSE;

    // right arm
    if (m_iExplodingArm==ARM_RIGHT) {
      MakeRotationMatrixFast(mRot, ANGLE3D(0.0f, 0.0f, 0.0f));
      vPos = FLOAT3D(0.0f, 0.0f, 0.0f);
      GetModelForRendering()->GetAttachmentTransformations(BODY_ATTACHMENT_ARM_RIGHT, mRot, vPos, FALSE);
      m_plExpArmPos.pl_PositionVector = vPos*GetRotationMatrix() + GetPlacement().pl_PositionVector;
      m_plExpArmPos.pl_OrientationAngle = GetPlacement().pl_OrientationAngle;
      CAttachmentModelObject &amo0 = *GetModelObject()->GetAttachmentModel(BODY_ATTACHMENT_ARM_RIGHT);
      amo0.amo_moModelObject.GetAttachmentTransformations(ARM_ATTACHMENT_PLASMAGUN, mRot, vPos, FALSE);
      m_plExpGunPos.pl_PositionVector = vPos*GetRotationMatrix() + GetPlacement().pl_PositionVector;
      m_plExpGunPos.pl_OrientationAngle = GetPlacement().pl_OrientationAngle;
      m_vExpDamage = FLOAT3D( +12.0f, 15.0f, 0.0f);

      if (m_penLeftArmDestroyTarget) {
        SendToTarget(m_penLeftArmDestroyTarget, EET_TRIGGER, FixupCausedToPlayer(this, m_penEnemy));
      }
    }
    
    // left arm
    if (m_iExplodingArm==ARM_LEFT) {
      MakeRotationMatrixFast(mRot, ANGLE3D(0.0f, 0.0f, 0.0f));
      vPos = FLOAT3D(0.0f, 0.0f, 0.0f);
      GetModelForRendering()->GetAttachmentTransformations(BODY_ATTACHMENT_ARM_LEFT, mRot, vPos, FALSE);
      m_plExpArmPos.pl_PositionVector = vPos*GetRotationMatrix() + GetPlacement().pl_PositionVector;
      m_plExpArmPos.pl_OrientationAngle = GetPlacement().pl_OrientationAngle;
      m_plExpArmPos.pl_OrientationAngle(1)+=180.0f;
      CAttachmentModelObject &amo0 = *GetModelObject()->GetAttachmentModel(BODY_ATTACHMENT_ARM_LEFT);
      amo0.amo_moModelObject.GetAttachmentTransformations(ARM_ATTACHMENT_PLASMAGUN, mRot, vPos, FALSE);
      m_plExpGunPos.pl_PositionVector = vPos*GetRotationMatrix() + GetPlacement().pl_PositionVector;
      m_plExpGunPos.pl_OrientationAngle = GetPlacement().pl_OrientationAngle;
      m_vExpDamage = FLOAT3D( -12.0f, 15.0f, 0.0f);

      if (m_penRightArmDestroyTarget) {        
        SendToTarget(m_penRightArmDestroyTarget, EET_TRIGGER, FixupCausedToPlayer(this, m_penEnemy));
      }
    }
    m_aExpArmRot = ANGLE3D(FRnd()*360.0f-180.0f, FRnd()*360.0f-180.0f, FRnd()*360.0f-180.0f);
    m_aExpGunRot = ANGLE3D(FRnd()*360.0f-180.0f, FRnd()*360.0f-180.0f, FRnd()*360.0f-180.0f);
    m_vExpDamage = m_vExpDamage*GetRotationMatrix();
    
    if (m_iExplodingArm==ARM_RIGHT) { m_bRightArmActive = FALSE; }
    if (m_iExplodingArm==ARM_LEFT)  { m_bLeftArmActive = FALSE; }

    PlaySound(m_soVoice, SOUND_ARMDESTROY, SOF_3D);
    // spawn explosion #1
    CPlacement3D pl = GetPlacement();
    pl.pl_PositionVector += FLOAT3D(0.0f, LARVA_HANDLE_TRANSLATE, 0.0f);
    ShakeItBaby(_pTimer->CurrentTick(), 0.5f, FALSE);
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_CANNON;
    eSpawnEffect.vStretch = FLOAT3D(m_fStretch*0.5f, m_fStretch*0.5f, m_fStretch*0.5f);
    CEntityPointer penExplosion = CreateEntity(pl, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);
    autowait(FRnd()*0.25f+0.15f);
    // spawn explosion #2
    ShakeItBaby(_pTimer->CurrentTick(), 0.5f, FALSE);
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_CANNON;
    eSpawnEffect.vStretch = FLOAT3D(m_fStretch, m_fStretch, m_fStretch);
    CPlacement3D plMiddle;
    plMiddle.pl_PositionVector = (m_plExpArmPos.pl_PositionVector +  m_plExpGunPos.pl_PositionVector)/2.0f;
    plMiddle.pl_OrientationAngle = m_plExpArmPos.pl_OrientationAngle;
    CEntityPointer penExplosion = CreateEntity(plMiddle, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);
    autowait(FRnd()*0.15f+0.15f);
    // spawn explosion #3 & #4
    CPlacement3D pl = GetPlacement();
    pl.pl_PositionVector += FLOAT3D(0.0f, LARVA_HANDLE_TRANSLATE, 0.0f);
    ShakeItBaby(_pTimer->CurrentTick(), 1.0f, FALSE);
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_CANNON;
    eSpawnEffect.vStretch = FLOAT3D(m_fStretch*1.5f,m_fStretch*1.5f,m_fStretch*1.5f);
    CEntityPointer penExplosion = CreateEntity(pl, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);
    eSpawnEffect.betType = BET_ROCKET;
    penExplosion = CreateEntity(m_plExpGunPos, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);  

    SpawnWingDebris();
    RemoveWing(m_iExplodingArm);
   
    // wait a little bit 'to recover'
    autowait(1.5f);

    m_bExploding = FALSE;
    m_bActive = TRUE;

    // restart behaveour
    SendEvent(EBegin());
    
    return EReturn();
  }

  Die(EDeath eDeath) : CEnemyBase::Die { 
    
    m_penDeathInflictor = eDeath.eLastDamage.penInflictor;

    m_bActive = FALSE;
    m_iExplosions = 8;
    
    PlaySound(m_soChirp, SOUND_DEATH, SOF_3D);
    m_soLaser.Stop();

    // spawn explosions
    while ((m_iExplosions--)>0)
    {
      ShakeItBaby(_pTimer->CurrentTick(), 0.5f, FALSE);

      // randomize explosion position and size
      CPlacement3D plExplosion;
      plExplosion.pl_OrientationAngle = ANGLE3D(0.0f, 0.0f, 0.0f);
      plExplosion.pl_PositionVector   = FLOAT3D(FRnd()*2.0f-1.0f, FRnd()*3.0f-1.5f+LARVA_HANDLE_TRANSLATE, FRnd()*2.0f-1.0f)*m_fStretch + GetPlacement().pl_PositionVector;
      FLOAT vExpSize = (FRnd()*0.7f+0.7f)*m_fStretch;

      ESpawnEffect eSpawnEffect;
      eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
      eSpawnEffect.betType = BET_CANNON;
      eSpawnEffect.vStretch = FLOAT3D(vExpSize, vExpSize, vExpSize);
      CEntityPointer penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
      penExplosion->Initialize(eSpawnEffect);
      autowait(FRnd()*0.05f+0.35f);
    }
    
    ShakeItBaby(_pTimer->CurrentTick(), 2.0f, FALSE);

    // final explosions
    CPlacement3D plExplosion;
    plExplosion.pl_OrientationAngle = ANGLE3D(0.0f, 0.0f, 0.0f);
    plExplosion.pl_PositionVector   = FLOAT3D(0.0f,-1.5f+LARVA_HANDLE_TRANSLATE, 1.5f)*m_fStretch + GetPlacement().pl_PositionVector;
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_CANNON;
    eSpawnEffect.vStretch = FLOAT3D(m_fStretch, m_fStretch, m_fStretch)*2.0f;
    CEntityPointer penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);
    plExplosion.pl_PositionVector   = FLOAT3D(-1.0f,-0.2f+LARVA_HANDLE_TRANSLATE,-1.5f)*m_fStretch + GetPlacement().pl_PositionVector;
    penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);
    plExplosion.pl_PositionVector   = FLOAT3D(1.0f, 1.7f+LARVA_HANDLE_TRANSLATE, 0.1f)*m_fStretch + GetPlacement().pl_PositionVector;
    penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);
    plExplosion.pl_PositionVector   = GetPlacement().pl_PositionVector;
    eSpawnEffect.betType = BET_ROCKET;
    penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);
    // end debris
    m_vExpDamage = FLOAT3D( 0.0f, 15.0f, 0.0f);
    FLOAT3D vTranslation = m_vExpDamage;
    CPlacement3D plDebris = GetPlacement();
    plDebris.pl_PositionVector += FLOAT3D(0.0f, LARVA_HANDLE_TRANSLATE, 0.0f);
    Debris_Begin(EIBT_FLESH, DPT_BLOODTRAIL, BET_BLOODSTAIN, 1.0f, m_vExpDamage, en_vCurrentTranslationAbsolute, 5.0f, 2.0f);
    Debris_Spawn_Independent(this, this, MODEL_DEBRIS_BODY, TEXTURE_BODY, 0, 0, 0, 0, m_fStretch,
      plDebris, vTranslation, ANGLE3D(45.0f, 230.0f, 0.0f));
    vTranslation += FLOAT3D(FRnd()*4.0f-2.0f, FRnd()*4.0f-2.0f, FRnd()*4.0f-2.0f);
    Debris_Spawn_Independent(this, this, MODEL_DEBRIS_TAIL01, TEXTURE_BODY, 0, 0, 0, 0, m_fStretch,
      plDebris, vTranslation, ANGLE3D(15.0f, 130.0f, 0.0f));
    vTranslation += FLOAT3D(FRnd()*4.0f-2.0f, FRnd()*4.0f-2.0f, FRnd()*4.0f-2.0f);
    Debris_Spawn_Independent(this, this, MODEL_DEBRIS_TAIL02, TEXTURE_BODY, 0, 0, 0, 0, m_fStretch,
      plDebris, vTranslation, ANGLE3D(145.0f, 30.0f, 0.0f));
    for (INDEX i=0; i<8; i++) {
      Debris_Spawn(this, this, MODEL_DEBRIS_FLESH, TEXTURE_DEBRIS_FLESH , 0, 0, 0, 0, m_fStretch, 
        FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f+LARVA_HANDLE_TRANSLATE, FRnd()*0.6f+0.2f));        
    }
    
    // explosion
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_EXPLOSION_DEBRIS;
    eSpawnEffect.vStretch = FLOAT3D(1,1,1);
    penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);
    // explosion smoke
    eSpawnEffect.betType = BET_EXPLOSION_SMOKE;
    penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    penExplosion->Initialize(eSpawnEffect);

    EDeath eDeath;
    eDeath.eLastDamage.penInflictor = m_penDeathInflictor;
    
    EActivateBeam eab;
    eab.bTurnOn = FALSE;
    m_penRecharger->SendEvent(eab);
    
    if (m_penDeathTarget) {
      SendToTarget(m_penDeathTarget, EET_TRIGGER, FixupCausedToPlayer(this, m_penEnemy));
    }

    jump CEnemyBase::Die(eDeath);
  }


  Fire(EVoid) : CEnemyBase::Fire { 
    
    UpdateFiringPos();
    // if larva has at least one wing
    if (m_bLeftArmActive || m_bRightArmActive) {
      m_iRnd = IRnd()%9;
      if (m_iRnd>6 && !m_bRechargePose && GetHealth()>0.1f*m_fMaxHealth) {
        PlaySound(m_soFire3, SOUND_FIRE_TAIL, SOF_3D);
        ShootTailProjectile();
      } 
      if (m_iRnd>6 && m_bRechargePose) {
        m_iRnd = 3;
      }

      if (m_iRnd>3) {
        return EReturn();
      }
      
      while(m_iRnd>0) {
        if (m_bLeftArmActive) {
          PlaySound(m_soFire1, SOUND_FIRE_PLASMA, SOF_3D);
          ShootProjectile(PRT_LARVA_PLASMA, m_vFirePosLeftPlasmaRel, ANGLE3D(0, 0, 0));
          RemoveAttachmentFromModel(*PlasmaLeftModel(), PLASMAGUN_ATTACHMENT_PROJECTILE);
          autowait(0.25f);
          PlasmaLeftModel()->PlayAnim(PLASMAGUN_ANIM_SPAWNING, 0);  
          autowait(0.25f);
          AddAttachmentToModel(this, *PlasmaLeftModel(), PLASMAGUN_ATTACHMENT_PROJECTILE, MODEL_PLASMA, TEXTURE_PLASMA, 0, 0, 0);
          CAttachmentModelObject *amo = PlasmaLeftModel()->GetAttachmentModel(BODY_ATTACHMENT_ARM_LEFT);
          amo->amo_moModelObject.StretchModel(FLOAT3D(m_fStretch, m_fStretch, m_fStretch));
        }
        if (m_bRightArmActive) {
          PlaySound(m_soFire2, SOUND_FIRE_PLASMA, SOF_3D);
          ShootProjectile(PRT_LARVA_PLASMA, m_vFirePosRightPlasmaRel, ANGLE3D(0, 0, 0));
          RemoveAttachmentFromModel(*PlasmaRightModel(), PLASMAGUN_ATTACHMENT_PROJECTILE);
          autowait(0.25f);
          PlasmaRightModel()->PlayAnim(PLASMAGUN_ANIM_SPAWNING, 0);  
          autowait(0.25f);
          AddAttachmentToModel(this, *PlasmaRightModel(), PLASMAGUN_ATTACHMENT_PROJECTILE, MODEL_PLASMA, TEXTURE_PLASMA, 0, 0, 0);
          CAttachmentModelObject *amo = PlasmaRightModel()->GetAttachmentModel(BODY_ATTACHMENT_ARM_LEFT);
          amo->amo_moModelObject.StretchModel(FLOAT3D(m_fStretch, m_fStretch, m_fStretch));
        }
        m_iRnd--;
      }    
    // if no wings left fire laser
    } else if (TRUE) {
      m_iRnd = IRnd()%10;
      if (m_iRnd>6 && !m_bRechargePose && GetHealth()>0.1f*m_fMaxHealth) {
        PlaySound(m_soFire3, SOUND_FIRE_TAIL, SOF_3D);
        ShootTailProjectile();
      }
      if (m_iRnd<4 && _pTimer->CurrentTick()>m_tmDontFireLaserBefore) {
        PlaySound(m_soLaser, SOUND_LASER_CHARGE, SOF_3D);
        SpawnReminder(this, 3.0f, 129);
        m_tmDontFireLaserBefore = _pTimer->CurrentTick()+m_fMinimumLaserWait;
      }
    }
    
    PerhapsChangeTarget();
    
    return EReturn();
  }

  Hit(EVoid) : CEnemyBase::Hit {
    return EReturn();
  }

  BeIdle(EVoid) : CEnemyBase::BeIdle {
    
    PerhapsChangeTarget();
    
    autowait(0.5f);

    while (TRUE) {
      FindNewTarget();  
      SendEvent(EReconsiderBehavior());
      autowait(0.5f);
    }
  };


  LarvaLoop() {
    
    FindNewTarget();  
    SendEvent(EReconsiderBehavior());
    
    StartModelAnim(BODY_ANIM_IDLE, AOF_SMOOTHCHANGE|AOF_LOOPING);

    SpawnReminder(this, 0.5f, 128);
    SpawnReminder(this, 0.5f, 145); // fire guided reminder
    wait () {
      on (EBegin) :
      {
        if (!m_bLeftArmActive && !m_bRightArmActive) {
          CModelObject &amo = GetModelObject()->GetAttachmentModel(BODY_ATTACHMENT_BACKARMS)->amo_moModelObject;
          amo.PlayAnim(BACKARMS_ANIM_ACTIVATING, AOF_SMOOTHCHANGE|AOF_NORESTART);
          PlaySound(m_soFire1, SOUND_DEPLOYLASER, SOF_3D);
          SpawnReminder(this, amo.GetAnimLength(BACKARMS_ANIM_ACTIVATING), 160);           
        }
        call CEnemyBase::MainLoop();
      }
      
      on (ELarvaArmDestroyed ead) :
      {
        m_iExplodingArm = ead.iArm;
        call ArmExplosion();
      }

      on (ELarvaRechargePose elrp) :
      {
        if (elrp.bStart==TRUE && m_bRechargePose!=TRUE) {
          StartModelAnim(BODY_ANIM_TORECHARGING, AOF_SMOOTHCHANGE|AOF_NORESTART);
          SpawnReminder(this, GetModelObject()->GetAnimLength(BODY_ANIM_TORECHARGING), 156);
        }
        if (elrp.bStart==FALSE && m_bRechargePose!=FALSE) {
          StartModelAnim(BODY_ANIM_FROMRECHARGING, AOF_SMOOTHCHANGE|AOF_NORESTART);
          SpawnReminder(this, GetModelObject()->GetAnimLength(BODY_ANIM_FROMRECHARGING), 157);
        }
        resume;
      }
      
      on (EReminder er) :
      {
        // if this is not our reminder, pass (although there are no states below this?)
        if (er.iValue==128) {
          // while recharger is active keep respawning the reminder to return here
          if (RechargerActive()) { 
            SpawnReminder(this, 1.0f, 128);
          } else {
            m_bRecharging = FALSE;
            m_ltTarget = LT_ENEMY;
            // return to idle pose
            ELarvaRechargePose elrp;
            elrp.bStart = FALSE;
            SendEvent(elrp);
          }
          // if larva is in recharge mode and close enough to recharger and beam is up
          if (m_bActive && m_bRecharging && DistanceXZ(this, m_penRecharger)<5.0f) {
            if (m_bRechargePose) {
              if (((CExotechLarvaCharger *) m_penRecharger.ep_pen)->m_bBeamActive)
              {
                if (!m_bRechargedAtLeastOnce) {
                  if (m_penFirstRechargeTarget) {
                    SendToTarget(m_penFirstRechargeTarget , EET_TRIGGER, FixupCausedToPlayer(this, m_penEnemy));
                  }
                  m_bRechargedAtLeastOnce = TRUE;
                }
                SetHealth(ClampUp(GetHealth()+m_fRechargePerSecond, m_fMaxHealth*m_fMaxRechargedHealth));
                if (GetHealth()>m_fMaxHealth*0.95f) {
                  m_ltTarget = LT_ENEMY;
                  m_bRecharging = FALSE;
                  // deactivate beam
                  EActivateBeam eab;
                  eab.bTurnOn = FALSE;
                  m_penRecharger->SendEvent(eab);
                  // return to idle pose
                  ELarvaRechargePose elrp;
                  elrp.bStart = FALSE;
                  SendEvent(elrp);
                }
              } else if (TRUE) {
                EActivateBeam eab;
                eab.bTurnOn = TRUE;
                m_penRecharger->SendEvent(eab);
              }
            } else {
              ELarvaRechargePose elrp;
              elrp.bStart = TRUE;
              SendEvent(elrp);
            }
          }
          // if larva is in normal mode
          else if (TRUE) {
            if (GetHealth()<(m_fLarvaHealth*0.7f)) {            
              if (!RechargerActive()) {
                m_ltTarget = LT_ENEMY;
              } else {
                m_bRecharging = TRUE;
                m_ltTarget = LT_RECHARGER;
              }
            }
          }
          resume;
        // check to see if guided missile firing is needed
        } else if (er.iValue==145) {
          FindNewTarget();        
          if (AnyPlayerCloserThen(9.0f) && GetHealth()>0.1f*m_fMaxHealth) {
            UpdateFiringPos();
            PlaySound(m_soFire3, SOUND_FIRE_TAIL, SOF_3D);
				    ShootTailProjectile();
          }
          else if (m_penEnemy && GetHealth()>0.1f*m_fMaxHealth) {
            if (!IsVisible(m_penEnemy)) {
              INDEX iRnd = IRnd()%6;
              if (iRnd>4) {
                UpdateFiringPos();
                PlaySound(m_soFire3, SOUND_FIRE_TAIL, SOF_3D);
				        ShootTailProjectile();
              }
            }
          }
          SpawnReminder(this, 0.5f, 145);
          resume;
        // begin rendering laser
        } else if (er.iValue==129) {
          if (m_bActive && m_bLaserActive) { FireLaser(); }
          SpawnReminder(this, 0.35f, 130);
          resume;
        // explode the laser without turning it off
        } else if (er.iValue==130) {
          if (m_bActive) { ExplodeLaser(); }
          SpawnReminder(this, 0.75f, 131);
          resume;
        // finally stop rendering the laser
        } else if (er.iValue==131) {
          m_bRenderLeftLaser  = FALSE;
          m_bRenderRightLaser = FALSE;
          resume;
        // start charging anim
        } else if (er.iValue==156) {
          m_bRechargePose = TRUE;
          StartModelAnim(BODY_ANIM_RECHARGING, AOF_SMOOTHCHANGE|AOF_LOOPING);
          resume;
        // return to idle anim
        } else if (er.iValue==157) {
          m_bRechargePose = FALSE;
          StartModelAnim(BODY_ANIM_IDLE, AOF_SMOOTHCHANGE|AOF_LOOPING);
        // stop charging anim
        } else if (er.iValue==160) {
          CModelObject &amo = GetModelObject()->GetAttachmentModel(BODY_ATTACHMENT_BACKARMS)->amo_moModelObject;
          amo.PlayAnim(BACKARMS_ANIM_ACTIVE, AOF_SMOOTHCHANGE|AOF_LOOPING);  
          m_bLaserActive = TRUE;
        }
        resume;
      }
    }
  }

  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_FLYING|EPF_HASLUNGS|EPF_ABSOLUTETRANSLATE);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    en_fDensity = 2000.0f;

    // set your appearance
    SetModel(MODEL_BODY);
    SetModelMainTexture(TEXTURE_BODY);

    // add left side attachments
    AddAttachmentToModel(this, *GetModelObject(), BODY_ATTACHMENT_ARM_LEFT, MODEL_WING, TEXTURE_WING, 0, 0, 0);
    CModelObject &amo0 = GetModelObject()->GetAttachmentModel(BODY_ATTACHMENT_ARM_LEFT)->amo_moModelObject;
    AddAttachmentToModel(this, amo0, ARM_ATTACHMENT_PLASMAGUN, MODEL_PLASMAGUN, TEXTURE_PLASMAGUN, 0, 0, 0);
    
    // add right side attachments
    AddAttachmentToModel(this, *GetModelObject(), BODY_ATTACHMENT_ARM_RIGHT, MODEL_WING, TEXTURE_WING, 0, 0, 0);
    CModelObject &amo1 = GetModelObject()->GetAttachmentModel(BODY_ATTACHMENT_ARM_RIGHT)->amo_moModelObject;
    amo1.StretchModel(FLOAT3D(-1.0f, 1.0f, 1.0f));
    AddAttachmentToModel(this, amo1, ARM_ATTACHMENT_PLASMAGUN, MODEL_PLASMAGUN, TEXTURE_PLASMAGUN, 0, 0, 0);
    CModelObject &amo2 = amo1.GetAttachmentModel(ARM_ATTACHMENT_PLASMAGUN)->amo_moModelObject;
    amo2.StretchModel(FLOAT3D(-1.0f, 1.0f, 1.0f));

    // add blades
    AddAttachmentToModel(this, *GetModelObject(), BODY_ATTACHMENT_BACKARMS, MODEL_BLADES, TEXTURE_BODY, 0, 0, 0);
    
    // add holder
    AddAttachmentToModel(this, *GetModelObject(), BODY_ATTACHMENT_EXOTECHLARVA, MODEL_EXOTECHLARVA, TEXTURE_EXOTECHLARVA, 0, 0, 0);
    CModelObject &amo3 = GetModelObject()->GetAttachmentModel(BODY_ATTACHMENT_EXOTECHLARVA)->amo_moModelObject;
    AddAttachmentToModel(this, amo3, EXOTECHLARVA_ATTACHMENT_BEAM, MODEL_BEAM, TEXTURE_BEAM, 0, 0, 0);
    AddAttachmentToModel(this, amo3, EXOTECHLARVA_ATTACHMENT_ENERGYBEAMS, MODEL_ENERGYBEAMS, TEXTURE_ENERGYBEAMS, 0, 0, 0);
    AddAttachmentToModel(this, amo3, EXOTECHLARVA_ATTACHMENT_FLARE, MODEL_FLARE, TEXTURE_FLARE, 0, 0, 0);

    AddAttachmentToModel(this, *PlasmaLeftModel(), PLASMAGUN_ATTACHMENT_PROJECTILE, MODEL_PLASMA, TEXTURE_PLASMA, 0, 0, 0);
    AddAttachmentToModel(this, *PlasmaRightModel(), PLASMAGUN_ATTACHMENT_PROJECTILE, MODEL_PLASMA, TEXTURE_PLASMA, 0, 0, 0);

    // set the size of this model
    GetModelObject()->StretchModelRelative(FLOAT3D(m_fStretch, m_fStretch, m_fStretch));
   
    m_vFirePosLeftPlasmaRel  = FIREPOS_PLASMA_LEFT*m_fStretch;
    m_vFirePosRightPlasmaRel = FIREPOS_PLASMA_RIGHT*m_fStretch;
    //m_vFirePosMouthRel       = FIREPOS_MOUTH*m_fStretch;
    m_vFirePosTailRel        = FIREPOS_TAIL*m_fStretch;

    // this is a boss
    m_bBoss = TRUE;

    // setup moving speed
    m_fWalkSpeed = 0.0f;
    m_aWalkRotateSpeed = 100.0f;
    m_fAttackRunSpeed = 7.5f;
    m_aAttackRotateSpeed = 100.0f;
    // setup attack distances
    m_fStopDistance = m_fStopRadius;
    m_fBlowUpAmount = 100.0f;
    m_fBodyParts = 0;
    m_fDamageWounded = 0.0f;
    m_iScore = 750000;
    m_sptType = SPT_BLOOD;
    m_fAttackDistance = 100.0f;
    m_fCloseDistance = 0.0f;

    m_fAttackFireTime = 0.5f;
    m_fCloseFireTime = 0.5f;

    // set acceleration/deceleration
    en_fAcceleration = UpperLimit(1.0f);
    en_fDeceleration = UpperLimit(1.0f);
    
    // set health
    SetHealth(m_fLarvaHealth);
    m_fMaxHealth = m_fLarvaHealth;

    m_bActive = TRUE;
    m_bExploding = FALSE;
    m_bLaserActive = FALSE;

    // set stretch factors for height and width
    //GetModelObject()->StretchModel(FLOAT3D(2.0f, 2.0f, 2.0f));

    ModelChangeNotify();
    StandingAnim();

    autowait(0.05f);
    
    // make larva invulnerable 'till start marker
    m_bInvulnerable = TRUE;

    if (!DoSafetyChecks()) {
      Destroy();
      return;
    }

    // wait to be triggered
    wait() {
      on (EBegin) : { resume; }
      on (ETrigger) : { stop; }
      otherwise (): { resume; }
    }

    PlaySound(m_soChirp, SOUND_CHIRP, SOF_3D|SOF_LOOP);

    // move to first marker
    while (DistanceTo(this, m_penMarkerNew)>5.0f) {
      wait(0.05f) {
        on (EBegin) : { resume; }
        on (ETimer) : { 
          FLOAT3D vToMarker = m_penMarkerNew->GetPlacement().pl_PositionVector - GetPlacement().pl_PositionVector;
          vToMarker.Normalize();
          SetDesiredTranslation(vToMarker*m_fAttackRunSpeed);
          stop;
        }
      }
    }

    m_bInvulnerable = FALSE;

    // one state under base class to intercept some events
    jump LarvaLoop();
    //jump CEnemyBase::MainLoop();
  };
};
