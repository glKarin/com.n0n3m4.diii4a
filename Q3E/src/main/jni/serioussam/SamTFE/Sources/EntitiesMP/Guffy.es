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

344
%{
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/Enemies/Guffy/Guffy.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/Projectile";


%{
// info structure
static EntityInfo eiGuffy = {
  EIBT_FLESH, 800.0f,
  0.0f, 1.9f, 0.0f,     // source (eyes)
  0.0f, 1.0f, 0.0f,     // target (body)
};

#define FIRE_LEFT_ARM   FLOAT3D(-0.56f, +1.125f, -1.32f)
#define FIRE_RIGHT_ARM  FLOAT3D(+0.50f, +1.060f, -0.82f)

//#define FIRE_DEATH_LEFT   FLOAT3D( 0.0f, 7.0f, -2.0f)
//#define FIRE_DEATH_RIGHT  FLOAT3D(3.75f, 4.2f, -2.5f)

%}


class CGuffy : CEnemyBase {
name      "Guffy";
thumbnail "Thumbnails\\Guffy.tbn";

properties:
  
  2 INDEX m_iLoopCounter = 0,
  3 FLOAT m_fSize = 1.0f,
  4 BOOL  m_bWalkSoundPlaying = FALSE,
  5 FLOAT m_fThreatDistance = 5.0f,
  6 BOOL  m_bEnemyToTheLeft = FALSE,

  //10 CSoundObject m_soFeet,
  10 CSoundObject m_soFire1,
  11 CSoundObject m_soFire2,
  
components:
  0 class   CLASS_BASE          "Classes\\EnemyBase.ecl",
  1 class   CLASS_PROJECTILE    "Classes\\Projectile.ecl",

 10 model   MODEL_GUFFY         "ModelsMP\\Enemies\\Guffy\\Guffy.mdl",
 11 texture TEXTURE_GUFFY       "ModelsMP\\Enemies\\Guffy\\Guffy.tex",
 12 model   MODEL_GUN           "ModelsMP\\Enemies\\Guffy\\Gun.mdl",
 13 texture TEXTURE_GUN         "ModelsMP\\Enemies\\Guffy\\Gun.tex",

// ************** SOUNDS **************
 40 sound   SOUND_IDLE          "ModelsMP\\Enemies\\Guffy\\Sounds\\Idle.wav",
 41 sound   SOUND_SIGHT         "ModelsMP\\Enemies\\Guffy\\Sounds\\Sight.wav",
 43 sound   SOUND_FIRE          "ModelsMP\\Enemies\\Guffy\\Sounds\\Fire.wav",
 44 sound   SOUND_WOUND         "ModelsMP\\Enemies\\Guffy\\Sounds\\Wound.wav",
 45 sound   SOUND_DEATH         "ModelsMP\\Enemies\\Guffy\\Sounds\\Death.wav",

functions:

// describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANSV("Guffy gunned %s down"), (const char *) strPlayerName);
    return str;
  }

  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnmSoldier,  "DataMP\\Messages\\Enemies\\Guffy.txt");
    return fnmSoldier;
  }
  /*// overridable function to get range for switching to another player
  FLOAT GetThreatDistance(void)
  {
    return m_fThreatDistance;
  }*/

  /*BOOL ForcesCannonballToExplode(void)
  {
    if (m_EwcChar==WLC_SERGEANT) {
      return TRUE;
    }
    return CEnemyBase::ForcesCannonballToExplode();
  }*/

  void Precache(void) {
    CEnemyBase::Precache();
    
    // guffy
    PrecacheModel(MODEL_GUFFY);
    PrecacheTexture(TEXTURE_GUFFY);

    // weapon
    PrecacheModel(MODEL_GUN);
    PrecacheTexture(TEXTURE_GUN);

    // sounds
    PrecacheSound(SOUND_IDLE );
    PrecacheSound(SOUND_SIGHT);
    PrecacheSound(SOUND_DEATH);
    PrecacheSound(SOUND_FIRE);
    PrecacheSound(SOUND_WOUND);
    
    // projectile
    PrecacheClass(CLASS_PROJECTILE, PRT_GUFFY_PROJECTILE);
  };

  // Entity info
  void *GetEntityInfo(void) {
    return &eiGuffy;
  };

  /*FLOAT GetCrushHealth(void)
  {
    if (m_EwcChar==WLC_SERGEANT) {
      return 100.0f;
    }
    return 0.0f;
  }*/

  // Receive damage
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // guffy can't harm guffy
    if (!IsOfClass(penInflictor, "Guffy")) {
      CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  };


  // virtual anim functions
  void StandingAnim(void) {
    StartModelAnim(GUFFY_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);
  };
  /*void StandingAnimFight(void)
  {
    StartModelAnim(GUFFY_ANIM_FIRE, AOF_LOOPING|AOF_NORESTART);
  }*/
  void RunningAnim(void) {
    StartModelAnim(GUFFY_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
  };
  void WalkingAnim(void) {
    RunningAnim();
  };
  void RotatingAnim(void) {
    StartModelAnim(GUFFY_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
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

  // fire rocket
  void FireRocket(FLOAT3D &vPos) {
    CPlacement3D plRocket;
    plRocket.pl_PositionVector = vPos;
    plRocket.pl_OrientationAngle = ANGLE3D(0, -5.0f-FRnd()*10.0f, 0);
    plRocket.RelativeToAbsolute(GetPlacement());
    CEntityPointer penProjectile = CreateEntity(plRocket, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = PRT_GUFFY_PROJECTILE;
    penProjectile->Initialize(eLaunch);
  };
  
  // adjust sound and watcher parameters here if needed
  void EnemyPostInit(void) 
  {
    // set sound default parameters
    m_soSound.Set3DParameters(160.0f, 50.0f, 1.0f, 1.0f);
    m_soFire1.Set3DParameters(160.0f, 50.0f, 1.0f, 1.0f);
    m_soFire2.Set3DParameters(160.0f, 50.0f, 1.0f, 1.0f);
  };

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    iAnim = GUFFY_ANIM_WOUND;
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
      iAnim = GUFFY_ANIM_DEATHBACKWARD;
    } else {
      iAnim = GUFFY_ANIM_DEATHFORWARD;
    }

    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // death
  FLOAT WaitForDust(FLOAT3D &vStretch) {
    vStretch=FLOAT3D(1,1,2)*1.5f;
    if(GetModelObject()->GetAnim()==GUFFY_ANIM_DEATHBACKWARD)
    {
      return 0.48f;
    }
    else if(GetModelObject()->GetAnim()==GUFFY_ANIM_DEATHFORWARD)
    {
      return 1.0f;
   }
    return -1.0f;
  };

procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  Fire(EVoid) : CEnemyBase::Fire {
    
    StartModelAnim(GUFFY_ANIM_FIRE, AOF_LOOPING);
    
    // wait for animation to bring the left hand into firing position
    autowait(0.1f);

    FLOATmatrix3D m;
    FLOAT3D fLookRight = FLOAT3D(1.0f, 0.0f, 0.0f);
    MakeRotationMatrixFast(m, GetPlacement().pl_OrientationAngle);
    fLookRight = fLookRight * m;
    BOOL bEnemyRight = (BOOL) (fLookRight % (m_penEnemy->GetPlacement().pl_PositionVector - GetPlacement().pl_PositionVector));

    if (bEnemyRight>=0) {  // enemy is to the right of guffy
      ShootProjectile(PRT_GUFFY_PROJECTILE, FIRE_LEFT_ARM*m_fSize, ANGLE3D(0, 0, 0));
      PlaySound(m_soFire1, SOUND_FIRE, SOF_3D);
      
      ShootProjectile(PRT_GUFFY_PROJECTILE, FIRE_RIGHT_ARM*m_fSize, ANGLE3D(-9, 0, 0));
      PlaySound(m_soFire2, SOUND_FIRE, SOF_3D);
    } else { // enemy is to the left of guffy
      ShootProjectile(PRT_GUFFY_PROJECTILE, FIRE_LEFT_ARM*m_fSize, ANGLE3D(9, 0, 0));
      PlaySound(m_soFire1, SOUND_FIRE, SOF_3D);
      
      ShootProjectile(PRT_GUFFY_PROJECTILE, FIRE_RIGHT_ARM*m_fSize, ANGLE3D(0, 0, 0));
      PlaySound(m_soFire2, SOUND_FIRE, SOF_3D);
    }
    
    autowait(1.0f);
    
    StopMoving();
    
    MaybeSwitchToAnotherPlayer();

    // wait for a while
    StandingAnimFight();
    autowait(FRnd()*0.25f+0.25f);

    return EReturn();
  };


/************************************************************
 *                    D  E  A  T  H                         *
 ************************************************************/
  /*Death(EVoid) : CEnemyBase::Death {
    // stop moving
    StopMoving();
    DeathSound();     // death sound
    
    // set physic flags
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags() | ENF_SEETHROUGH);

    // death notify (change collision box)
    ChangeCollisionBoxIndexWhenPossible(GUFFY_COLLISION_BOX_DEATH);

    // start death anim
    StartModelAnim(GUFFY_ANIM_DEATHFORWARD, 0);
    autowait(GetModelObject()->GetAnimLength(GUFFY_ANIM_TOFIRE));
    
    return EEnd();
  };*/

/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    SetHealth(210.0f);
    m_fMaxHealth = 210.0f;
    en_fDensity = 2000.0f;

    // set your appearance
    SetModel(MODEL_GUFFY);
    m_fSize = 1.5f;
    SetModelMainTexture(TEXTURE_GUFFY);
    AddAttachment(GUFFY_ATTACHMENT_GUNRIGHT, MODEL_GUN, TEXTURE_GUN);
    AddAttachment(GUFFY_ATTACHMENT_GUNLEFT, MODEL_GUN, TEXTURE_GUN);
    GetModelObject()->StretchModel(FLOAT3D(m_fSize, m_fSize, m_fSize));
    ModelChangeNotify();
    CModelObject *pmoRight = &GetModelObject()->GetAttachmentModel(GUFFY_ATTACHMENT_GUNRIGHT)->amo_moModelObject;
    pmoRight->StretchModel(FLOAT3D(-1,1,1));
    m_fBlowUpAmount = 10000.0f;
    m_iScore = 3000;
    //m_fThreatDistance = 15;
    
    if (m_fStepHeight==-1) {
      m_fStepHeight = 4.0f;
    }

    StandingAnim();
    // setup moving speed
    m_fWalkSpeed = FRnd() + 2.5f;
    m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 500.0f);
    m_fAttackRunSpeed = FRnd() + 5.0f;
    m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
    m_fCloseRunSpeed = FRnd() + 5.0f;
    m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
    // setup attack distances
    m_fAttackDistance = 150.0f;
    m_fCloseDistance = 0.0f;
    m_fStopDistance = 25.0f;
    m_fAttackFireTime = 5.0f;
    m_fCloseFireTime = 5.0f;
    m_fIgnoreRange = 250.0f;
    // damage/explode properties
    m_fBodyParts = 5;
    m_fDamageWounded = 100.0f;

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
