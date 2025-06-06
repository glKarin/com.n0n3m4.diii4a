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

503
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/BasicEffects";
uses "EntitiesMP/Light";
uses "EntitiesMP/AmmoItem";

// input parameter for launching the projectile
event EDropPipebomb {
  CEntityPointer penLauncher,     // who launched it
  FLOAT fSpeed,                   // launch speed
};

%{
#define ECF_PIPEBOMB ( \
  ((ECBI_MODEL|ECBI_BRUSH|ECBI_PROJECTILE_SOLID|ECBI_MODEL_HOLDER)<<ECB_TEST) |\
  ((ECBI_PROJECTILE_SOLID)<<ECB_IS) )

void CPipebomb_OnPrecache(CDLLEntityClass *pdec, INDEX iUser) 
{
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_GRENADE);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_EXPLOSIONSTAIN);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_SHOCKWAVE);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_GRENADE_PLANE);
  pdec->PrecacheModel(MODEL_PIPEBOMB);
  pdec->PrecacheTexture(TEXTURE_PIPEBOMB);
  pdec->PrecacheSound(SOUND_LAUNCH);
}
%}

class CPipebomb : CMovableModelEntity {
name      "Pipebomb";
thumbnail "";
features "ImplementsOnPrecache", "CanBePredictable";

properties:
  1 CEntityPointer m_penLauncher,     // who lanuched it
  2 FLOAT m_fIgnoreTime = 0.0f,       // time when laucher will be ignored
  3 FLOAT m_fSpeed = 0.0f,            // launch speed
  4 BOOL m_bCollected = FALSE,        // collect -> do not explode

{
  CLightSource m_lsLightSource;
}

components:
  1 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",
  2 class   CLASS_LIGHT         "Classes\\Light.ecl",

// ********* PIPEBOMB (GRENADE) *********
 10 model   MODEL_PIPEBOMB      "Models\\Weapons\\Pipebomb\\Grenade\\Grenade.mdl",
 11 texture TEXTURE_PIPEBOMB    "Models\\Weapons\\Pipebomb\\Grenade\\Grenade.tex",
 12 sound   SOUND_LAUNCH        "Sounds\\Weapons\\RocketFired.wav",

functions:
  /* Read from stream. */
  void Read_t( CTStream *istr) // throw char *
  {
    CMovableModelEntity::Read_t(istr);
    SetupLightSource();
  }

  /* Get static light source information. */
  CLightSource *GetLightSource(void)
  {
    if (!IsPredictor()) {
      return &m_lsLightSource;
    } else {
      return NULL;
    }
  }

  // Setup light source
  void SetupLightSource(void)
  {
    // setup light source
    CLightSource lsNew;
    lsNew.ls_ulFlags = LSF_NONPERSISTENT|LSF_DYNAMIC;
    lsNew.ls_colColor = C_vdRED;
    lsNew.ls_rFallOff = 1.0f;
    lsNew.ls_rHotSpot = 0.1f;
    lsNew.ls_plftLensFlare = &_lftYellowStarRedRingFar;
    lsNew.ls_ubPolygonalMask = 0;
    lsNew.ls_paoLightAnimation = NULL;

    m_lsLightSource.ls_penEntity = this;
    m_lsLightSource.SetLightSource(lsNew);
  }

  // render particles
  void RenderParticles(void) {
    Particles_GrenadeTrail(this);
  }

/************************************************************
 *                       PIPEBOMB                           *
 ************************************************************/
void Pipebomb(void) {
  // set appearance
  InitAsModel();
  SetPhysicsFlags(EPF_MODEL_BOUNCING);
  SetCollisionFlags(ECF_PIPEBOMB);
  //GetModelObject()->StretchModel(FLOAT3D(2.5f, 2.5f, 2.5f));
  //ModelChangeNotify();
  SetModel(MODEL_PIPEBOMB);
  SetModelMainTexture(TEXTURE_PIPEBOMB);
  // start moving
  LaunchAsFreeProjectile(FLOAT3D(0.0f, 0.0f, -m_fSpeed), (CMovableEntity*) m_penLauncher.ep_pen);
  SetDesiredRotation(ANGLE3D(0, FRnd()*120.0f+120.0f, FRnd()*250.0f-125.0f));
  en_fBounceDampNormal   = 0.7f;
  en_fBounceDampParallel = 0.7f;
  en_fJumpControlMultiplier = 0.0f;
  en_fCollisionSpeedLimit = 45.0f;
  en_fCollisionDamageFactor = 10.0f;
  SetHealth(20.0f);
};

void PipebombExplosion(void) {
  ESpawnEffect ese;
  FLOAT3D vPoint;
  FLOATplane3D vPlaneNormal;
  FLOAT fDistanceToEdge;

  // explosion
  ese.colMuliplier = C_WHITE|CT_OPAQUE;
  ese.betType = BET_GRENADE;
  ese.vStretch = FLOAT3D(1,1,1);
  SpawnEffect(GetPlacement(), ese);
  // spawn sound event in range
  if( IsDerivedFromClass( m_penLauncher, "Player")) {
    SpawnRangeSound( m_penLauncher, this, SNDT_PLAYER, 50.0f);
  }

  // on plane
  if (GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge)) {
    if ((vPoint-GetPlacement().pl_PositionVector).Length() < 3.5f) {
      // wall stain
      ese.betType = BET_EXPLOSIONSTAIN;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // shock wave
      ese.betType = BET_SHOCKWAVE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint, ANGLE3D(0, 0, 0)), ese);
      // second explosion on plane
      ese.betType = BET_GRENADE_PLANE;
      ese.vNormal = FLOAT3D(vPlaneNormal);
      SpawnEffect(CPlacement3D(vPoint+ese.vNormal/50.0f, ANGLE3D(0, 0, 0)), ese);
    }
  }
};




/************************************************************
 *             C O M M O N   F U N C T I O N S              *
 ************************************************************/
// projectile hitted (or time expired or can't move any more)
void ProjectileHit(void) {
  // explode ...
  InflictRangeDamage(m_penLauncher, DMT_EXPLOSION, 100.0f,
      GetPlacement().pl_PositionVector, 4.0f, 8.0f);
  // sound event
  ESound eSound;
  eSound.EsndtSound = SNDT_EXPLOSION;
  eSound.penTarget = m_penLauncher;
  SendEventInRange(eSound, FLOATaabbox3D(GetPlacement().pl_PositionVector, 50.0f));
};


// spawn effect
void SpawnEffect(const CPlacement3D &plEffect, const ESpawnEffect &eSpawnEffect) {
  CEntityPointer penEffect = CreateEntity(plEffect, CLASS_BASIC_EFFECT);
  penEffect->Initialize(eSpawnEffect);
};




/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
  // --->>> PROJECTILE SLIDE ON BRUSH
  ProjectileSlide(EVoid) {
    m_bCollected = FALSE;
    // fly loop
    wait() {
      on (EBegin) : { resume; }
      // collected
      on (ETouch etouch) : {
        // clear time limit for launcher
        if (etouch.penOther->GetRenderType() & RT_BRUSH) {
          m_fIgnoreTime = 0.0f;
          resume;
        }

        BOOL bCollect;
        // ignore launcher within 0.5 second
        bCollect = etouch.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
        // speed must be almost zero
        bCollect &= (en_vCurrentTranslationAbsolute.Length() < 0.25f);
        // only if can be collected
        EAmmoItem eai;
        eai.EaitType = AIT_GRENADES;
        eai.iQuantity = 1;
        if (bCollect && etouch.penOther->ReceiveItem(eai)) {
          m_bCollected = TRUE;
          stop;
        }
        resume;
      }
      // killed
      on (EDeath) : {
        ProjectileHit();
        stop;
      }
      // activated
      on (EStart) : {
        ProjectileHit();
        stop;
      }
    }
    return EEnd();
  };

  // --->>> MAIN
  Main(EDropPipebomb edrop) {
    // remember the initial parameters
    ASSERT(edrop.penLauncher!=NULL);
    m_penLauncher = edrop.penLauncher;
    m_fSpeed = edrop.fSpeed;

    // projectile initialization
    Pipebomb();

    // setup light source
    SetupLightSource();

    // remember lauching time
    m_fIgnoreTime = _pTimer->CurrentTick() + 0.5f;

    // slide
    autocall ProjectileSlide() EEnd;

    // pipebomb explosion if not collected
    if (!m_bCollected) {
      PipebombExplosion();
    }

    Destroy();

    return;
  }
};
