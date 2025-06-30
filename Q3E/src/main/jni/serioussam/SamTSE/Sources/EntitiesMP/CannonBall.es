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

506
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Weapons/Cannon/Projectile/CannonBall.h"
#include "EntitiesMP/MovingBrush.h"
#include "EntitiesMP/DestroyableArchitecture.h"
%}

uses "EntitiesMP/BasicEffects";
uses "EntitiesMP/Light";
uses "EntitiesMP/PlayerWeapons";
uses "EntitiesMP/EnemyBase";

enum CannonBallType {
  0 CBT_IRON    "",
  1 CBT_NUKE    "",
};

// input parameter for launching the projectile
event ELaunchCannonBall {
  CEntityPointer penLauncher,     // who launched it
  enum CannonBallType cbtType,    // type of cannon ball
  FLOAT fLaunchPower,             // how fast will cannon be launched
  FLOAT fSize,                    // the size of the cannonball
};
event EForceExplode {
};

%{

// projectile solid
#define ECF_CANNON_BALL ( \
  ((ECBI_MODEL|ECBI_BRUSH|ECBI_PROJECTILE_SOLID|ECBI_CORPSE|ECBI_MODEL_HOLDER|ECBI_MODEL_HOLDER)<<ECB_TEST) |\
  ((ECBI_PROJECTILE_SOLID)<<ECB_IS) |\
  ((ECBI_MODEL|ECBI_MODEL_HOLDER|ECBI_CORPSE)<<ECB_PASS) )

#define IRON_LIFE_TIME  10.0f
#define NUKE_LIFE_TIME  5.0f

//#define CANNONBALL_STRETCH 3.0f

// damages
#define IRON_DAMAGE_MIN 400.0f
#define IRON_DAMAGE_MAX 750.0f
#define IRON_RANGE_DAMAGE (25.0f/4)   // because we have 4 explosions //50
#define IRON_RANGE_HOTSPOT 2.0f  //2
#define IRON_RANGE_FALLOFF 16.0f //8

#define NUKE_DAMAGE_MIN 600.0f
#define NUKE_DAMAGE_MAX 800.0f
#define NUKE_RANGE_DAMAGE (1000.0f/13)   // because we have 13 explosions
#define NUKE_RANGE_HOTSPOT 15.0f
#define NUKE_RANGE_FALLOFF 50.0f
#define SOUND_RANGE 250.0f

#define STRETCH_0  FLOAT3D(0.0f,0.0f,0.0f)
#define STRETCH_1  FLOAT3D(1.0f,1.0f,1.0f)
#define STRETCH_2  FLOAT3D(2.0f,2.0f,2.0f)
#define STRETCH_3  FLOAT3D(3.0f,3.0f,3.0f)
#define STRETCH_4  FLOAT3D(4.0f,4.0f,4.0f)
#define STRETCH_6  FLOAT3D(6.0f,6.0f,6.0f)
#define STRETCH_8  FLOAT3D(8.0f,8.0f,8.0f)
#define STRETCH_10 FLOAT3D(10.0f,10.0f,10.0f)

void CCannonBall_OnPrecache(CDLLEntityClass *pdec, INDEX iUser) 
{
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_CANNON);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_CANNONEXPLOSIONSTAIN);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_CANNONSHOCKWAVE);

  pdec->PrecacheModel(MODEL_BALL);
  pdec->PrecacheTexture(TEXTURE_IRON_BALL);
  pdec->PrecacheTexture(TEXTURE_NUKE_BALL);
  pdec->PrecacheTexture(TEX_REFL_BWRIPLES01);
  pdec->PrecacheTexture(TEX_SPEC_MEDIUM);
  pdec->PrecacheSound(SOUND_BALL_BOUNCE);
}

%}


class export CCannonBall : CMovableModelEntity {
name      "Cannon ball";
thumbnail "";
features "ImplementsOnPrecache";

properties:
  1 CEntityPointer m_penLauncher,     // who lanuched it
  2 FLOAT m_fLaunchPower = 0.0f,      // how fast will cannon be launched
  3 FLOAT m_fCannonBallSize = 0.0f,   // the size of the cannonball
 10 FLOAT m_fIgnoreTime = 0.0f,       // time when laucher will be ignored
 11 FLOAT m_fStartTime = 0.0f,        // start time when launched

 12 INDEX m_iNextChannel = 0,         // next channel to play sound on

 13 BOOL m_bSelfExploded = FALSE,     // if cannonball exploded because of time, not because of impact

 // sound channels for bouncing sound
 20 CSoundObject m_soBounce0,
 21 CSoundObject m_soBounce1,
 22 CSoundObject m_soBounce2,
 23 CSoundObject m_soBounce3,
 24 CSoundObject m_soBounce4,

 30 enum CannonBallType m_cbtType = CBT_IRON,
 40 FLOAT m_tmInvisibility = 0.0f,           // don't render before given time
 41 FLOAT m_tmExpandBox = 0.0f,              // expand collision after a few seconds
 42 FLOAT m_tmForceExplode = 0.0f,                 // force explosion at given moment
/*
{
  CLightSource m_lsLightSource;
}*/

components:
  1 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",
  2 class   CLASS_LIGHT         "Classes\\Light.ecl",

// ********* PLAYER ROCKET *********
 10 model   MODEL_BALL            "Models\\Weapons\\Cannon\\Projectile\\CannonBall.mdl",
 11 texture TEXTURE_NUKE_BALL     "Models\\Weapons\\Cannon\\Projectile\\NukeBall.tex",
 13 texture TEXTURE_IRON_BALL     "Models\\Weapons\\Cannon\\Projectile\\IronBall.tex",
 12 sound   SOUND_BALL_BOUNCE     "Models\\Weapons\\Cannon\\Sounds\\Bounce.wav",
200 texture TEX_REFL_BWRIPLES01   "Models\\ReflectionTextures\\BWRiples01.tex",
211 texture TEX_SPEC_MEDIUM       "Models\\SpecularTextures\\Medium.tex",

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
  
  void PostMoving(void)
  {
    CMovableModelEntity::PostMoving();
    if (en_vCurrentTranslationAbsolute.Length()<1.0f ||         // if very slow, allmost standing
        _pTimer->CurrentTick()>=m_tmForceExplode ||             // if forced explosion
        (GetCollisionBoxIndex()==0 &&                           // if unable to change collision box for some time
        (_pTimer->CurrentTick()>m_fStartTime+m_tmExpandBox+0.5f)))
    {
      SendEvent(EForceExplode());
    }
  }
  /* Read from stream. */
  void Read_t( CTStream *istr) // throw char *
  {
    CMovableModelEntity::Read_t(istr);
    // setup light source
//    SetupLightSource();
  }

  /* Get static light source information. */
/*
  CLightSource *GetLightSource(void)
  {
//    if (!IsPredictor()) {
//      return &m_lsLightSource;
//    } else {
      return NULL;
//    }
  }
*/
  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    // if time now is inside invisibility time, don't render model
    CModelObject *pmo = GetModelObject();
    if ( (pmo != NULL) && (_pTimer->GetLerpedCurrentTick() < (m_fStartTime+m_tmInvisibility) ) )
    {
      // make it invisible
      pmo->mo_colBlendColor = 0;
    }
    else
    {
      // make it visible
      pmo->mo_colBlendColor = C_WHITE|CT_OPAQUE;
    }
    return CEntity::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
  }

/*  // Setup light source
  void SetupLightSource(void)
  {
    // setup light source
    CLightSource lsNew;
    lsNew.ls_ulFlags = LSF_NONPERSISTENT|LSF_DARKLIGHT|LSF_DYNAMIC;
    lsNew.ls_rHotSpot = 0.0f;
    lsNew.ls_colColor = RGBToColor(128, 128, 128);
    lsNew.ls_rFallOff = 5.0f;
    lsNew.ls_plftLensFlare = NULL;
    lsNew.ls_ubPolygonalMask = 0;
    lsNew.ls_paoLightAnimation = NULL;

    m_lsLightSource.ls_penEntity = this;
    m_lsLightSource.SetLightSource(lsNew);
  }
  */

  // render particles
  void RenderParticles(void) {
    // no particles when not existing
    if (GetRenderType()!=CEntity::RT_MODEL) {
      return;
    }

    FLOAT fSpeedRatio = Min( en_vCurrentTranslationAbsolute.Length()/140.0f, 1.0f);
    INDEX ctFireParticles = INDEX( (Max( fSpeedRatio-0.5f, 0.0f)*2.0f)*128);
    //CPrintF("fSpeedRatio=%g, ctFireParticles=%d\n", fSpeedRatio, ctFireParticles);
    if( _pTimer->GetLerpedCurrentTick()-m_fStartTime>0.075f)
    {
      Particles_BeastBigProjectileTrail( this, 2.0f, 1.0f, 0.75f, ctFireParticles);
    }
  }

void Initialize(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_BOUNCING);
  SetCollisionFlags(ECF_CANNON_BALL);
  SetModel(MODEL_BALL);
  if( m_cbtType == CBT_IRON)
  {
    SetModelMainTexture(TEXTURE_IRON_BALL);
  }
  else
  {
    SetModelMainTexture(TEXTURE_NUKE_BALL);
  }
  // stretch it
  GetModelObject()->StretchModel(FLOAT3D(m_fCannonBallSize, m_fCannonBallSize, m_fCannonBallSize));
  ModelChangeNotify();

  // reflection texture data
  GetModelObject()->mo_toReflection.SetData(GetTextureDataForComponent(TEX_REFL_BWRIPLES01));
  // specular texture data
  GetModelObject()->mo_toSpecular.SetData(GetTextureDataForComponent(TEX_SPEC_MEDIUM));
  // start moving
  LaunchAsFreeProjectile(FLOAT3D(0.0f, 0.0f, -m_fLaunchPower), (CMovableEntity*)(CEntity*)m_penLauncher);
  en_fBounceDampNormal   = 0.5f;
  en_fBounceDampParallel = 0.75f;
  en_fAcceleration = 0.0f;
  en_fDeceleration = 5.0f;
  en_fCollisionSpeedLimit = 40.0f;
  en_fCollisionDamageFactor = 10.0f;
  SetHealth(50000.0f);
  GetModelObject()->PlayAnim(CANNONBALL_ANIM_FIRESLOW, 0);
};

FLOAT CalculateDamageToInflict(void)
{
  FLOAT fMaxDamage = IRON_DAMAGE_MAX;
  if(m_cbtType == CBT_NUKE)
  {
    fMaxDamage = IRON_DAMAGE_MAX;
  }

  // speed can range from
  FLOAT fSpeedRatio = en_vCurrentTranslationAbsolute.Length()/140.0f;
  // apply damage to range from 0 to damage max
  FLOAT fApplyDamage = Clamp( fSpeedRatio*fMaxDamage, 0.0f, fMaxDamage);
  return fApplyDamage;
}

void Explosion(FLOAT3D vCenter,
               const FLOAT3D &vStretchExplosion,
               const FLOAT3D &vStretchShockwave,
               const FLOAT3D &vStretchStain,
               BOOL bHasExplosion,
               BOOL bHasShockWave,
               BOOL bHasStain,
               BOOL bHasLight)
{
  ESpawnEffect ese;
  FLOAT3D vOnPlane;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;
  
  // explosion
  if( bHasExplosion)
  {
    ese.colMuliplier = C_WHITE|CT_OPAQUE;
    if( bHasLight)
    {
      ese.betType = BET_CANNON;
    }
    else
    {
      ese.betType = BET_CANNON_NOLIGHT;
    }
    ese.vStretch = vStretchExplosion;
    CPlacement3D plHandle = GetPlacement();
    plHandle.pl_PositionVector+=vCenter;
    SpawnEffect(plHandle, ese);
    // spawn sound event in range
    if( IsDerivedFromClass( m_penLauncher, "Player")) {
      SpawnRangeSound( m_penLauncher, this, SNDT_PLAYER, 100.0f);
    }
  }
  // on plane
  if (GetNearestPolygon(vOnPlane, vPlaneNormal, fDistanceToEdge)) {
    if ((vOnPlane-GetPlacement().pl_PositionVector).Length() < 3.5f) {
      if( bHasStain)
      {
        // wall stain
        ese.colMuliplier = C_WHITE|CT_OPAQUE;
        ese.betType = BET_CANNONEXPLOSIONSTAIN;
        ese.vNormal = FLOAT3D(vPlaneNormal);
        ese.vStretch = vStretchShockwave;
        SpawnEffect(CPlacement3D(vOnPlane, ANGLE3D(0, 0, 0)), ese);
      }
      if( bHasShockWave)
      {
        // shock wave horizontal
        ese.colMuliplier = C_WHITE|CT_OPAQUE;
        ese.betType = BET_CANNONSHOCKWAVE;
        ese.vNormal = FLOAT3D(vPlaneNormal);
        ese.vStretch = vStretchShockwave;
        SpawnEffect(CPlacement3D(vOnPlane, ANGLE3D(0, 0, 0)), ese);
      }
      // shock wave vertical
      /*
      ese.colMuliplier = C_WHITE|CT_OPAQUE;
      ese.betType = BET_CANNONSHOCKWAVE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      ese.vStretch = vStretchShockwave;
      SpawnEffect(CPlacement3D(vOnPlane, ANGLE3D(0, 0.0f, 0)), ese);
      */
      // second explosion on plane
      /*
      ese.colMuliplier = C_WHITE|CT_OPAQUE;
      ese.betType = BET_CANNON_PLANE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      ese.vStretch = vStretchExplosion;
      SpawnEffect(CPlacement3D(vOnPlane+ese.vNormal/50.0f, ANGLE3D(0, 0, 0)), ese);
      */
    }
  }

  RangeDamage();
};



/************************************************************
 *             C O M M O N   F U N C T I O N S              *
 ************************************************************/

// ball touch his valid target
BOOL BallTouchExplode(CEntityPointer penHit)
{
  FLOAT fApplyDamage = CalculateDamageToInflict();

  // obtain touched entity health
  FLOAT fHealth = 100;
  BOOL bForceCannonballToExplode = FALSE;

  if (penHit->GetPhysicsFlags()&EPF_MOVABLE) { 
    fHealth = ((CMovableEntity&)*penHit).GetHealth();
    if( IsDerivedFromClass(penHit, "Enemy Base"))
    {
      bForceCannonballToExplode = ((CEnemyBase&)*penHit).ForcesCannonballToExplode();
    }
  } else {
    if (IsOfClass(penHit, "ModelHolder2") || IsOfClass(penHit, "ExotechLarvaBattery")) {
      fHealth = ((CLiveEntity&)*penHit).GetHealth();
    } else {
      return FALSE; 
    }
  }

  if( IsOfClass(penHit, "ModelHolder2"))
  {
    bForceCannonballToExplode=TRUE;
  }

  if (IsOfClass(penHit, "Player")) {
    fHealth += ((CPlayer&)*penHit).m_fArmor * 2.0f;
  }
  // inflict direct damage to kill hitted entity
  FLOAT3D vDirection = en_vCurrentTranslationAbsolute;
  vDirection.Normalize();
//  CPrintF( "Applied damage %g\n", fApplyDamage);
  const FLOAT fDamageMul = GetSeriousDamageMultiplier(m_penLauncher);
  InflictDirectDamage(penHit, m_penLauncher, DMT_CANNONBALL, fApplyDamage*fDamageMul,
             GetPlacement().pl_PositionVector, vDirection);
  return(fApplyDamage <= fHealth || bForceCannonballToExplode);
};


// infilict range damage by cannonball
void RangeDamage(void)
{
  const FLOAT fDamageMul = GetSeriousDamageMultiplier(m_penLauncher);
  if(m_cbtType == CBT_IRON) {
    InflictRangeDamage(m_penLauncher, DMT_CANNONBALL_EXPLOSION, IRON_RANGE_DAMAGE*fDamageMul,
        GetPlacement().pl_PositionVector, IRON_RANGE_HOTSPOT, IRON_RANGE_FALLOFF);
  } else {
    // nuclear explosion ...
    InflictRangeDamage(m_penLauncher, DMT_CANNONBALL_EXPLOSION, NUKE_RANGE_DAMAGE*fDamageMul,
        GetPlacement().pl_PositionVector, NUKE_RANGE_HOTSPOT, NUKE_RANGE_FALLOFF);
  }
};


// spawn effect
void SpawnEffect(const CPlacement3D &plEffect, const ESpawnEffect &eSpawnEffect) {
  CEntityPointer penEffect = CreateEntity(plEffect, CLASS_BASIC_EFFECT);
  penEffect->Initialize(eSpawnEffect);
};



/************************************************************
 *                      S O U N D S                         *
 ************************************************************/
void BounceSound(FLOAT fSpeed) {
  FLOAT fVolume = Clamp(fSpeed/6.0f, 0.0f, 1.0f);
  if (fVolume<0.1f) {
    return;
  }
  CSoundObject &so = (&m_soBounce0)[m_iNextChannel];
  m_iNextChannel = (m_iNextChannel+1)%5;
  so.Set3DParameters(70.0f, 10.0f, fVolume, 1.0f);
  PlaySound(so, SOUND_BALL_BOUNCE, SOF_3D);
};



/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
  Bounce(EVoid) {
    // if already inside some entity
    CEntity *penObstacle;
    if (CheckForCollisionNow(0, &penObstacle)) {
      // explode now
      return EEnd();
    }

    FLOAT fWaitTime = IRON_LIFE_TIME;
    // if this is nuke ball
    if(m_cbtType == CBT_NUKE)
    {
      fWaitTime = NUKE_LIFE_TIME;
    }
    // bounce loop
    wait(fWaitTime) {
      on (EBegin) : { resume; }
      on (EPass epass) : {
        BOOL bHit;
        // ignore launcher within 1 second
        bHit = epass.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
        // ignore twister
        bHit &= !IsOfClass(epass.penOther, "Twister");

        if (bHit)
        {
          if (BallTouchExplode(epass.penOther)) { stop; }
        }
        resume;
      }
      on (ETouch etouch) : {
        // explode if touched another cannon ball
        if( IsOfClass(etouch.penOther, "Cannon ball"))
        {
          stop;
        }
        if( IsOfClass(etouch.penOther, "Moving Brush"))
        {
          CMovingBrush &br = (CMovingBrush &) *etouch.penOther;
          if( br.m_fHealth>0)
          {
            FLOAT3D vDirection = en_vCurrentTranslationAbsolute;
            vDirection.Normalize();
            InflictDirectDamage(etouch.penOther, m_penLauncher, DMT_CANNONBALL, CalculateDamageToInflict(),
                       GetPlacement().pl_PositionVector, vDirection);
            m_bSelfExploded = FALSE;
            stop;
          }
        }
        if( IsOfClass(etouch.penOther, "DestroyableArchitecture"))
        {
          CDestroyableArchitecture &br = (CDestroyableArchitecture &) *etouch.penOther;
          if( br.m_fHealth>0)
          {
            FLOAT3D vDirection = en_vCurrentTranslationAbsolute;
            vDirection.Normalize();
            InflictDirectDamage(etouch.penOther, m_penLauncher, DMT_CANNONBALL, CalculateDamageToInflict(),
                       GetPlacement().pl_PositionVector, vDirection);
            m_bSelfExploded = FALSE;
            stop;
          }
        }
        // clear time limit for launcher
        //m_fIgnoreTime = 0.0f;
        BounceSound(((FLOAT3D&)etouch.plCollision) % en_vCurrentTranslationAbsolute);
        resume;
      }
      on (EForceExplode) : { stop; }
      on (EDeath) : { stop; }
      on (ETimer) : { stop; }
    }
    m_bSelfExploded = TRUE;
    return EEnd();
  };

  // --->>> MAIN
  Main(ELaunchCannonBall eLaunch) {
    // remember the initial parameters
    ASSERT(eLaunch.penLauncher!=NULL);
    m_penLauncher = eLaunch.penLauncher;
    m_fLaunchPower = eLaunch.fLaunchPower;
    m_cbtType = eLaunch.cbtType;
    m_fCannonBallSize = eLaunch.fSize;
    m_tmInvisibility = 0.05f;
    m_bSelfExploded = FALSE;
    m_tmExpandBox = 0.0001f;
    // setup time for forced expolding
    m_tmForceExplode=_pTimer->CurrentTick()+30.0f;

    // initialization
    Initialize();

    SendEvent(EReturn());
    wait() {
      on (EBegin) : { resume;}
      on (EReturn) : { stop;}
    }

    // cast ray to check possible collision
    FLOAT tmCastCoverPath = _pTimer->TickQuantum*1.5f;
    CCastRay crRay(m_penLauncher, GetPlacement(), m_fLaunchPower*tmCastCoverPath);
    crRay.cr_bHitTranslucentPortals = FALSE;
    crRay.cr_fTestR = 0.75f/2.0f*m_fCannonBallSize;
    crRay.cr_ttHitModels = CCastRay::TT_COLLISIONBOX;
    GetWorld()->CastRay(crRay);

    // can't hurt player time
    m_fIgnoreTime = _pTimer->CurrentTick() + 0.1f;

    // bounce
    m_fStartTime = _pTimer->CurrentTick();

    if (crRay.cr_penHit!=NULL && crRay.cr_penHit->GetRenderType()==RT_MODEL)
    {
      if (BallTouchExplode(crRay.cr_penHit))
      {
        m_tmForceExplode = _pTimer->CurrentTick()+tmCastCoverPath;
      }
    }

    autocall Bounce() EEnd;

    // dissapear
    SwitchToEditorModel();
    // stop in place
    ForceFullStop();

    // sound event
    ESound eSound;
    eSound.EsndtSound = SNDT_EXPLOSION;
    eSound.penTarget = m_penLauncher;
    if (IsDerivedFromClass(this, "Player")) {
      SendEventInRange(eSound, FLOATaabbox3D(GetPlacement().pl_PositionVector, SOUND_RANGE));
    }

    if(m_cbtType == CBT_IRON)
    {
   //   Explosion( FLOAT3D(0.0f,0.0f,0.0f),   STRETCH_3, STRETCH_3, STRETCH_3, TRUE, TRUE,  TRUE, TRUE);
   //   autowait(0.15f);
      Explosion( FLOAT3D(0.0f,0.0f,0.0f),   STRETCH_3, STRETCH_3, STRETCH_4, TRUE, TRUE,  TRUE, TRUE);
      Explosion( FLOAT3D(1.0f,1.5f,1.5f),   STRETCH_3, STRETCH_3, STRETCH_4, TRUE, FALSE, FALSE, FALSE);
      Explosion( FLOAT3D(-2.0f,1.0f,-1.5f), STRETCH_3, STRETCH_3, STRETCH_4, TRUE, FALSE, FALSE, FALSE);
      Explosion( FLOAT3D(-1.0f,0.5f,1.0f),  STRETCH_4, STRETCH_4, STRETCH_4, TRUE, FALSE, FALSE, FALSE);
    }
    else if( m_cbtType == CBT_NUKE)
    {
      Explosion( FLOAT3D(0.0f,0.0f,0.0f),   STRETCH_6, STRETCH_6, STRETCH_10, TRUE, TRUE,  TRUE, TRUE);
      autowait(0.15f);
      Explosion( FLOAT3D(4.0f,5.0f,5.0f),   STRETCH_4, STRETCH_6, STRETCH_10, TRUE, FALSE, FALSE, FALSE);
      autowait(0.2f);
      Explosion( FLOAT3D(-5.0f,3.0f,-4.0f), STRETCH_4, STRETCH_6, STRETCH_10, TRUE, FALSE, FALSE, FALSE);
      autowait(0.15f);
      Explosion( FLOAT3D(-3.0f,2.0f,3.0f),  STRETCH_4, STRETCH_6, STRETCH_10, TRUE, FALSE, FALSE, FALSE);
      autowait(0.15f);
      Explosion( FLOAT3D(2.0f,1.0f,4.0f),   STRETCH_4, STRETCH_6, STRETCH_10, TRUE, TRUE,  FALSE, FALSE);
      autowait(0.2f);
      Explosion( FLOAT3D(-2.0f,5.0f,-4.0f), STRETCH_4, STRETCH_6, STRETCH_10, TRUE, FALSE, FALSE, FALSE);
      autowait(0.18f);
      Explosion( FLOAT3D(-3.0f,2.0f,2.0f),  STRETCH_4, STRETCH_6, STRETCH_10, TRUE, FALSE, FALSE, FALSE);
      autowait(0.25f);
      Explosion( FLOAT3D(0.0f,4.0f,-1.0f),  STRETCH_4, STRETCH_6, STRETCH_10, TRUE, FALSE, FALSE, FALSE);
      autowait(0.15f);
      Explosion( FLOAT3D(2.0f,0.0f,-3.0f),  STRETCH_4, STRETCH_6, STRETCH_10, TRUE, TRUE,  FALSE, FALSE);
      autowait(0.25f);
      Explosion( FLOAT3D(-1.0f,2.0f,0.0f),  STRETCH_4, STRETCH_6, STRETCH_10, TRUE, FALSE, FALSE, FALSE);
      autowait(0.125f);
      Explosion( FLOAT3D(3.0f,1.0f,1.0f),   STRETCH_4, STRETCH_6, STRETCH_10, TRUE, FALSE, FALSE, FALSE);
      autowait(0.1f);
      Explosion( FLOAT3D(3.0f,2.0f,2.0f),   STRETCH_4, STRETCH_6, STRETCH_10, TRUE, FALSE, FALSE, FALSE);
      autowait(0.125f);
      Explosion( FLOAT3D(3.0f,2.0f,2.0f),   STRETCH_4, STRETCH_6, STRETCH_10, TRUE, TRUE,  FALSE, FALSE);
    }

    // cease to exist
    Destroy();

    return;
  }
};
