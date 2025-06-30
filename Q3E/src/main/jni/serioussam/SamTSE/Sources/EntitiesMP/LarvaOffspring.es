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

353
%{
#include "EntitiesMP/StdH/StdH.h"

#include "EntitiesMP/EnemyBase.h"
#include "ModelsMP/Enemies/ExotechLarva/Projectile/TailProjectile.h"

%}

uses "EntitiesMP/BasicEffects";
uses "EntitiesMP/Light";
uses "EntitiesMP/Flame";


// input parameter for launching the LarvaOffspring
event ELaunchLarvaOffspring {
  CEntityPointer penLauncher,     // who launched it
};


%{
#define ECF_OFFSPRING ( \
  ((ECBI_MODEL|ECBI_BRUSH|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID|ECBI_ITEM|ECBI_MODEL_HOLDER|ECBI_CORPSE_SOLID)<<ECB_TEST) |\
  ((ECBI_MODEL)<<ECB_IS) |\
  ((ECBI_MODEL)<<ECB_PASS) )
%}

class export CLarvaOffspring : CMovableModelEntity {
name      "LarvaOffspring";
thumbnail "";
features  "CanBePredictable";

properties:
  1 CEntityPointer m_penLauncher,     // who lanuched it
  5 CEntityPointer m_penTarget,       // guided LarvaOffspring's target

 11 FLOAT m_fIgnoreTime = 0.0f,              // time when laucher will be ignored
 12 FLOAT m_fFlyTime = 0.0f,                 // fly time before explode/disappear
 13 FLOAT m_fStartTime = 0.0f,               // start time when launched
 14 FLOAT m_fDamageAmount = 0.0f,            // damage amount when hit something
 15 FLOAT m_fRangeDamageAmount = 0.0f,       // range damage amount
 16 FLOAT m_fDamageHotSpotRange = 0.0f,      // hot spot range damage for exploding LarvaOffspring
 17 FLOAT m_fDamageFallOffRange = 0.0f,      // fall off range damage for exploding LarvaOffspring
 18 FLOAT m_fSoundRange = 0.0f,              // sound range where explosion can be heard
 19 BOOL m_bExplode = FALSE,                 // explode -> range damage
 24 FLOAT m_aRotateSpeed = 0.0f,             // speed of rotation for guided LarvaOffsprings*/
 25 FLOAT m_tmExpandBox = 0.0f,              // expand collision after a few seconds
 30 CSoundObject m_soEffect,          // sound channel

 50 BOOL bLockedOn = TRUE,
 

components:
  1 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",

10 model   MODEL_LARVA_TAIL          "ModelsMP\\Enemies\\ExotechLarva\\Projectile\\TailProjectile.mdl",
11 texture TEXTURE_LARVA_TAIL        "ModelsMP\\Enemies\\ExotechLarva\\Projectile\\TailProjectile.tex",
12 sound   SOUND_LARVETTE            "ModelsMP\\Enemies\\ExotechLarva\\Sounds\\Squeak.wav",


functions:
  // premoving
  void PreMoving(void) {
    if (m_tmExpandBox>0) {
      if (_pTimer->CurrentTick()>m_fStartTime+m_tmExpandBox) {
        ChangeCollisionBoxIndexWhenPossible(1);
        m_tmExpandBox = 0;
      }
    }
    CMovableModelEntity::PreMoving();
  }

  void Precache() 
  {
	PrecacheSound(SOUND_LARVETTE);
    PrecacheModel(MODEL_LARVA_TAIL);
    PrecacheTexture(TEXTURE_LARVA_TAIL);
    PrecacheClass(CLASS_BASIC_EFFECT, BET_ROCKET);    
	PrecacheClass(CLASS_BASIC_EFFECT, BET_SHOCKWAVE);    
  }

void InitializeProjectile(void) {
  
  // we need target for guided misile
  if (IsDerivedFromClass(m_penLauncher, "Enemy Base")) {
    m_penTarget = ((CEnemyBase *) m_penLauncher.ep_pen)->m_penEnemy;
  }
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_SLIDING);
  SetCollisionFlags(ECF_OFFSPRING);
  
  SetModel(MODEL_LARVA_TAIL);
  SetModelMainTexture(TEXTURE_LARVA_TAIL);
  GetModelObject()->StretchModel(FLOAT3D(4.0f, 4.0f, 4.0f));

  ModelChangeNotify();
  // play the flying sound
  m_soEffect.Set3DParameters(50.0f, 10.0f, 1.0f, 1.0f);
  PlaySound(m_soEffect, SOUND_LARVETTE, SOF_3D|SOF_LOOP);
  // start moving
  LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
  SetDesiredRotation(ANGLE3D(0, 0, 0));
  m_fFlyTime = 12.0f;
  m_fDamageAmount = 10.0f;
  m_aRotateSpeed = 275.0f;
  SetHealth(10.0f);
}

void LarvaTailExplosion(void) {
  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;

  // explosion
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_ROCKET;
  ese.vStretch = FLOAT3D(1,1,1);
  SpawnEffect(GetPlacement(), ese);
  // spawn sound event in range
  if( IsDerivedFromClass( m_penLauncher, "Player")) {
    SpawnRangeSound( m_penLauncher, this, SNDT_PLAYER, m_fSoundRange);
  }

  // explosion debris
  ese.betType = BET_EXPLOSION_DEBRIS;
  SpawnEffect(GetPlacement(), ese);

  // explosion smoke
  ese.betType = BET_EXPLOSION_SMOKE;
  SpawnEffect(GetPlacement(), ese);

  // on plane
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge)) {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f) {
      // stain
      ese.betType = BET_EXPLOSIONSTAIN;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // shock wave
      ese.betType = BET_SHOCKWAVE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // second explosion on plane
      ese.betType = BET_ROCKET_PLANE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint+ese.vNormal/50.0f, ANGLE3D(0, 0, 0)), ese);
    }
  }
}


/************************************************************
 *             C O M M O N   F U N C T I O N S              *
 ************************************************************/

void ProjectileTouch(CEntityPointer penHit)
{
  // explode if needed
  ProjectileHit();

  // direct damage
  FLOAT3D vDirection;
  FLOAT fTransLen = en_vIntendedTranslation.Length();
  if( fTransLen>0.5f)
  {
    vDirection = en_vIntendedTranslation/fTransLen;
  }
  else
  {
    vDirection = -en_vGravityDir;
  }

  const FLOAT fDamageMul = GetSeriousDamageMultiplier(m_penLauncher);
  
  InflictDirectDamage(penHit, m_penLauncher, DMT_PROJECTILE, m_fDamageAmount*fDamageMul,
               GetPlacement().pl_PositionVector, vDirection);
  
};


void ProjectileHit(void)
{
  // explode ...
  if (m_bExplode) {
    const FLOAT fDamageMul = GetSeriousDamageMultiplier(m_penLauncher);
    InflictRangeDamage(m_penLauncher, DMT_EXPLOSION, m_fRangeDamageAmount*fDamageMul,
        GetPlacement().pl_PositionVector, m_fDamageHotSpotRange, m_fDamageFallOffRange);
  }
  // sound event
  if (m_fSoundRange > 0.0f) {
    ESound eSound;
    eSound.EsndtSound = SNDT_EXPLOSION;
    eSound.penTarget = m_penLauncher;
    SendEventInRange(eSound, FLOATaabbox3D(GetPlacement().pl_PositionVector, m_fSoundRange));
  }
};


// spawn effect
void SpawnEffect(const CPlacement3D &plEffect, const ESpawnEffect &eSpawnEffect) {
  CEntityPointer penEffect = CreateEntity(plEffect, CLASS_BASIC_EFFECT);
  penEffect->Initialize(eSpawnEffect);
};


// Calculate current rotation speed to rich given orientation in future
ANGLE GetRotationSpeed(ANGLE aWantedAngle, ANGLE aRotateSpeed, FLOAT fWaitFrequency)
{
  ANGLE aResult;
  // if desired position is smaller
  if ( aWantedAngle<-aRotateSpeed*fWaitFrequency)
  {
    // start decreasing
    aResult = -aRotateSpeed;
  }
  // if desired position is bigger
  else if (aWantedAngle>aRotateSpeed*fWaitFrequency)
  {
    // start increasing
    aResult = +aRotateSpeed;
  }
  // if desired position is more-less ahead
  else
  {
    aResult = aWantedAngle/fWaitFrequency;
  }
  return aResult;
}

/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
   
  LarvaOffspringGuidedSlide(EVoid) {
    // if already inside some entity
    CEntity *penObstacle;
    if (CheckForCollisionNow(0, &penObstacle)) {
      // explode now
      ProjectileTouch(penObstacle);
      return EEnd();
    }
    // fly loop
    while( _pTimer->CurrentTick()<(m_fStartTime+m_fFlyTime))
    {
      FLOAT fWaitFrequency = 0.1f;
      if (m_penTarget!=NULL) {
        // calculate desired position and angle
        EntityInfo *pei= (EntityInfo*) (m_penTarget->GetEntityInfo());
        FLOAT3D vDesiredPosition;
        GetEntityInfoPosition( m_penTarget, pei->vSourceCenter, vDesiredPosition);
        FLOAT3D vDesiredDirection = (vDesiredPosition-GetPlacement().pl_PositionVector).Normalize();
        // for heading
        ANGLE aWantedHeading = GetRelativeHeading( vDesiredDirection);
        ANGLE aHeading = GetRotationSpeed( aWantedHeading, m_aRotateSpeed, fWaitFrequency);

        // factor used to decrease speed of LarvaOffsprings oriented opposite of its target
        FLOAT fSpeedDecreasingFactor = ((180-Abs(aWantedHeading))/180.0f);
        // factor used to increase speed when far away from target
        FLOAT fSpeedIncreasingFactor = (vDesiredPosition-GetPlacement().pl_PositionVector).Length()/100;
        fSpeedIncreasingFactor = ClampDn(fSpeedIncreasingFactor, 1.0f);
        // decrease speed acodring to target's direction
        FLOAT fMaxSpeed = 30.0f*fSpeedIncreasingFactor;
        FLOAT fMinSpeedRatio = 0.5f;
        FLOAT fWantedSpeed = fMaxSpeed*( fMinSpeedRatio+(1-fMinSpeedRatio)*fSpeedDecreasingFactor);
        // adjust translation velocity
        SetDesiredTranslation( FLOAT3D(0, 0, -fWantedSpeed));
      
        // adjust rotation speed
        m_aRotateSpeed = 75.0f*(1+0.5f*fSpeedDecreasingFactor);
      
        // calculate distance factor
        FLOAT fDistanceFactor = (vDesiredPosition-GetPlacement().pl_PositionVector).Length()/50.0;
        fDistanceFactor = ClampUp(fDistanceFactor, 4.0f);
        FLOAT fRNDHeading = (FRnd()-0.5f)*180*fDistanceFactor;
        
        // if we are looking near direction of target
        if( Abs(aWantedHeading) < 30.0f)
        {
          // adjust heading and pich
          SetDesiredRotation(ANGLE3D(aHeading+fRNDHeading,0,0));
        }
        // just adjust heading
        else
        {
          SetDesiredRotation(ANGLE3D(aHeading,0,0));
        }
      }

      wait( fWaitFrequency)
      {
        on (EBegin) : { resume; }
        on (EPass epass) : {
          BOOL bHit;
          // ignore launcher within 1 second
          bHit = epass.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
          // ignore another LarvaOffspring
          bHit &= !IsOfClass(epass.penOther, "LarvaOffspring");
          // ignore twister
          bHit &= !IsOfClass(epass.penOther, "Twister");
          if (bHit) {
            ProjectileTouch(epass.penOther);
            return EEnd();
          }
          resume;
        }
        on (EDeath) :
        {
          ProjectileHit();
          return EEnd();
        }
        on (ETimer) :
        {
          stop;
        }
      }
    }
    return EEnd();
  };

 
  Main(ELaunchLarvaOffspring eLaunch) {
    // remember the initial parameters
    ASSERT(eLaunch.penLauncher!=NULL);
    m_penLauncher = eLaunch.penLauncher;
    SetPredictable(TRUE);
    // remember lauching time
    m_fIgnoreTime = _pTimer->CurrentTick() + 1.0f;

    InitializeProjectile();
  
    // fly
    m_fStartTime = _pTimer->CurrentTick();
    autocall LarvaOffspringGuidedSlide() EEnd;
  
	LarvaTailExplosion();
        
    Destroy();

    return;
  }
};
