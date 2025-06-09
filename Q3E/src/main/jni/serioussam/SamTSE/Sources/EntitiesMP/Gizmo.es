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

335
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Enemies/Gizmo/Gizmo.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/BasicEffects";

%{
// info structure
static EntityInfo eiGizmo = {
  EIBT_FLESH, 100.0f,
  0.0f, 1.3f, 0.0f,     // source (eyes)
  0.0f, 1.0f, 0.0f,     // target (body)
};

#define EXPLODE_GIZMO   2.5f
%}

class CGizmo: CEnemyBase {
name      "Gizmo";
thumbnail "Thumbnails\\Gizmo.tbn";

properties:
  // class internal
  1 BOOL m_bExploded = FALSE,
  
components:
  1 class   CLASS_BASE            "Classes\\EnemyBase.ecl",
  2 class   CLASS_BLOOD_SPRAY     "Classes\\BloodSpray.ecl",
  3 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",

// ************** DATA **************
 10 model   MODEL_GIZMO           "Models\\Enemies\\Gizmo\\Gizmo.mdl",
 20 texture TEXTURE_GIZMO         "Models\\Enemies\\Gizmo\\Gizmo.tex",
 
 50 sound   SOUND_IDLE            "Models\\Enemies\\Gizmo\\Sounds\\Idle.wav",
 51 sound   SOUND_JUMP            "Models\\Enemies\\Gizmo\\Sounds\\Jump.wav",
 52 sound   SOUND_DEATH_JUMP      "Models\\Enemies\\Gizmo\\Sounds\\JumpDeath.wav",
 53 sound   SOUND_SIGHT           "Models\\Enemies\\Gizmo\\Sounds\\Sight.wav",

functions:
  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANSV("%s ate a marsh hopper"), (const char *) strPlayerName);
    return str;
  }
  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnm, "Data\\Messages\\Enemies\\Gizmo.txt");
    return fnm;
  };
  /* Entity info */
  void *GetEntityInfo(void)
  {
    return &eiGizmo;
  };

  void Precache(void)
  {
    CEnemyBase::Precache();
    PrecacheSound(SOUND_SIGHT);
    PrecacheSound(SOUND_IDLE);
    PrecacheSound(SOUND_JUMP);
    PrecacheSound(SOUND_DEATH_JUMP);
    PrecacheClass(CLASS_BASIC_EFFECT, BET_GIZMO_SPLASH_FX);
    PrecacheClass(CLASS_BLOOD_SPRAY);
  };

  void SightSound(void) {
    PlaySound(m_soSound, SOUND_SIGHT, SOF_3D);
  };

  void RunningAnim(void)
  {
    StartModelAnim(GIZMO_ANIM_RUN, 0);
  };

  void MortalJumpAnim(void)
  {
    StartModelAnim(GIZMO_ANIM_RUN, 0);
  };
  
  void StandAnim(void)
  {
    StartModelAnim(GIZMO_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);
  };

  // virtual sound functions
  void IdleSound(void) {
    PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
  };

/************************************************************
 *                 BLOW UP FUNCTIONS                        *
 ************************************************************/
  void BlowUpNotify(void) {
    Explode();
  };

  // explode only once
  void Explode(void)
  {
    if (!m_bExploded)
    {
      m_bExploded = TRUE;
      // spawn blood spray
      CPlacement3D plSpray = GetPlacement();
      CEntity *penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
      penSpray->SetParent( this);
      ESpawnSpray eSpawnSpray;
      eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
      eSpawnSpray.fDamagePower = 2.0f;
      eSpawnSpray.fSizeMultiplier = 1.0f;
      eSpawnSpray.sptType = SPT_SLIME;
      eSpawnSpray.vDirection = en_vCurrentTranslationAbsolute/8.0f;
      eSpawnSpray.penOwner = this;
      penSpray->Initialize( eSpawnSpray);

      // spawn splash fx (sound)
      CPlacement3D plSplash = GetPlacement();
      CEntityPointer penSplash = CreateEntity(plSplash, CLASS_BASIC_EFFECT);
      ESpawnEffect ese;
      ese.colMuliplier = C_WHITE|CT_OPAQUE;
      ese.betType = BET_GIZMO_SPLASH_FX;
      penSplash->Initialize(ese);
    }
  };


  // gizmo should always blow up
  BOOL ShouldBlowUp(void)
  {
    return TRUE;
  }


  // leave stain
  virtual void LeaveStain(BOOL bGrow)
  {
    ESpawnEffect ese;
    FLOAT3D vPoint;
    FLOATplane3D vPlaneNormal;
    FLOAT fDistanceToEdge;
    // get your size
    FLOATaabbox3D box;
    GetBoundingBox(box);
  
    // on plane
    if( GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge))
    {
      // if near to polygon and away from last stain point
      if( (vPoint-GetPlacement().pl_PositionVector).Length()<0.5f )
      {
        FLOAT fStretch = box.Size().Length();
        // stain
        ese.colMuliplier = C_WHITE|CT_OPAQUE;
        ese.betType    = BET_GIZMOSTAIN;
        ese.vStretch   = FLOAT3D( fStretch*0.75f, fStretch*0.75f, 1.0f);
        ese.vNormal    = FLOAT3D( vPlaneNormal);
        ese.vDirection = FLOAT3D( 0, 0, 0);
        FLOAT3D vPos = vPoint+ese.vNormal/50.0f*(FRnd()+0.5f);
        CEntityPointer penEffect = CreateEntity( CPlacement3D(vPos, ANGLE3D(0,0,0)), CLASS_BASIC_EFFECT);
        penEffect->Initialize(ese);
      }
    }
  };

procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  // close range -> move toward enemy and try to jump onto it
  PerformAttack(EVoid) : CEnemyBase::PerformAttack
  {
    while (TRUE)
    {
      // ------------ Exit close attack if out of range or enemy is dead
      // if attacking is futile
      if (ShouldCeaseAttack())
      {
        SetTargetNone();
        return EReturn();
      }
      
      // stop moving
      SetDesiredTranslation(FLOAT3D(0.0f, 0.0f, 0.0f));
      SetDesiredRotation(ANGLE3D(0, 0, 0));

      // ------------ Wait for some time on the ground
      FLOAT fWaitTime = 0.25f+FRnd()*0.4f;
      wait( fWaitTime)
      {
        on (EBegin) : { resume; };
        on (ESound) : { resume; }     // ignore all sounds
        on (EWatch) : { resume; }     // ignore watch
        on (ETimer) : { stop; }       // timer tick expire
      }

      autocall JumpOnce() EReturn;
    }
  }

  JumpOnce(EVoid)
  {
    // ------------ Jump either in slightly randomized direction or mortal, streight and fast toward enemy
    // we are always going for enemy
    m_vDesiredPosition = m_penEnemy->GetPlacement().pl_PositionVector;
    m_fMoveFrequency = 0.1f;
    // if we are close enough for mortal jump
    if( CalcPlaneDist(m_penEnemy) < 10.0f)
    {
      // set mortal jump parameters (no random)
      m_fMoveSpeed = m_fCloseRunSpeed*1.5f;
      m_aRotateSpeed = m_aCloseRotateSpeed*0.5f;
      FLOAT fSpeedX = 0.0f;
      FLOAT fSpeedY = 10.0f;
      FLOAT fSpeedZ = -m_fMoveSpeed;
      // if can't see enemy
      if( !IsInFrustum(m_penEnemy, CosFast(30.0f)))
      {
        // rotate a lot
        m_aRotateSpeed = m_aCloseRotateSpeed*1.5f;
        // but don't jump too much
        fSpeedY /= 2.0f;
        fSpeedZ /= 4.0f;
        PlaySound(m_soSound, SOUND_JUMP, SOF_3D);
      }
      else
      {
        PlaySound(m_soSound, SOUND_DEATH_JUMP, SOF_3D);
      }
      FLOAT3D vTranslation(fSpeedX, fSpeedY, fSpeedZ);
      SetDesiredTranslation(vTranslation);
      MortalJumpAnim();
    }
    // start slightly randomized jump
    else
    {
      m_fMoveSpeed = m_fCloseRunSpeed;
      m_aRotateSpeed = m_aCloseRotateSpeed;
      // set random jump parameters
      FLOAT fSpeedX = (FRnd()-0.5f)*10.0f;
      FLOAT fSpeedY = FRnd()*5.0f+5.0f;
      FLOAT fSpeedZ = -m_fMoveSpeed-FRnd()*2.5f;
      FLOAT3D vTranslation(fSpeedX, fSpeedY, fSpeedZ);
      SetDesiredTranslation(vTranslation);
      RunningAnim();
      PlaySound(m_soSound, SOUND_JUMP, SOF_3D);
    }

    // ------------ While in air, adjust directions, on touch start new jump or explode
    while (TRUE)
    {
      // adjust direction and speed
      m_fMoveSpeed = 0.0f;
      m_aRotateSpeed = m_aCloseRotateSpeed;
      FLOAT3D vTranslation = GetDesiredTranslation();
      SetDesiredMovement(); 
      SetDesiredTranslation(vTranslation);

      wait(m_fMoveFrequency)
      {
        on (EBegin) : { resume; };
        on (ESound) : { resume; }     // ignore all sounds
        on (EWatch) : { resume; }     // ignore watch
        on (ETimer) : { stop; }       // timer tick expire
        on (ETouch etouch) :
        {
          // if we touched ground
          if( etouch.penOther->GetRenderType() & RT_BRUSH)
          {
            return EReturn();
          }
          // we touched player, explode
          else if ( IsDerivedFromClass( etouch.penOther, "Player"))
          {            
            InflictDirectDamage(etouch.penOther, this, DMT_IMPACT, 10.0f,
              GetPlacement().pl_PositionVector, -en_vGravityDir);
            SetHealth(-10000.0f);
            m_vDamage = FLOAT3D(0,10000,0);
            SendEvent(EDeath());
          }
          // we didn't touch ground nor player, ignore
          resume;
        }
      }
    }
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
    SetHealth(9.5f);
    m_fMaxHealth = 9.5f;
    en_tmMaxHoldBreath = 5.0f;
    en_fDensity = 2000.0f;
    m_fBlowUpSize = 2.0f;

    // set your appearance
    SetModel(MODEL_GIZMO);
    SetModelMainTexture(TEXTURE_GIZMO);
    // setup moving speed
    m_fWalkSpeed = FRnd() + 1.5f;
    m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 500.0f);
    m_fAttackRunSpeed = FRnd()*5.0f + 15.0f;
    m_aAttackRotateSpeed = AngleDeg(FRnd()*100 + 600.0f);
    m_fCloseRunSpeed = FRnd()*5.0f + 15.0f;
    m_aCloseRotateSpeed = AngleDeg(360.0f);
    // setup attack distances
    m_fAttackDistance = 400.0f;
    m_fCloseDistance = 250.0f;
    m_fStopDistance = 0.0f;
    m_fAttackFireTime = 2.0f;
    m_fCloseFireTime = 0.5f;
    m_fIgnoreRange = 500.0f;
    // damage/explode properties
    m_fBlowUpAmount = 0.0f;
    m_fBodyParts = 0;
    m_fDamageWounded = 0.0f;
    m_iScore = 500;
    m_sptType = SPT_SLIME;

    en_fDeceleration = 150.0f;

    // set stretch factors for height and width
    GetModelObject()->StretchModel(FLOAT3D(1.25f, 1.25f, 1.25f));
    ModelChangeNotify();
    StandingAnim();

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
