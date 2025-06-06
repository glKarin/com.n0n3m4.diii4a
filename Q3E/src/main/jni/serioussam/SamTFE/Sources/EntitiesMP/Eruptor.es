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

213
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/ModelHolder2";
uses "EntitiesMP/Projectile";

class CEruptor : CModelHolder2 {
name      "Eruptor";
thumbnail "Thumbnails\\Eruptor.tbn";
features  "HasName", "IsTargetable";

properties:
 // stretch
 10 FLOAT m_fStretchAll     "Er StretchAll" = 1.0f,
 11 FLOAT m_fStretchX       "Er StretchX"   = 1.0f,
 12 FLOAT m_fStretchY       "Er StretchY"   = 1.0f,
 13 FLOAT m_fStretchZ       "Er StretchZ"   = 1.0f,

 // random stretch
 15 BOOL m_bRandomStretch   "Er Stretch Random" = FALSE,       // random stretch
 16 FLOAT m_fStretchHeight  "Er Stretch Height (Y%)" = 0.2f,   // stretch height
 17 FLOAT m_fStretchWidth   "Er Stretch Width (X%)" =  0.2f,   // stretch width
 18 FLOAT m_fStretchDepth   "Er Stretch Depth (Z%)" =  0.2f,   // strecth depth

 // spawn properties
 20 FLOAT m_fAngle          "Er Angle" 'Q' = 45.0f,        // spawn spatial angle
 21 FLOAT m_fMaxSpeed       "Er Speed max" 'V' = 20.0f,    // max speed
 22 FLOAT m_fMinSpeed       "Er Speed min" 'B' = 10.0f,    // min speed
 23 FLOAT m_fTime           "Er Spawn time" 'F' = 1.0f,    // spawn every x seconds
 24 FLOAT m_fRandomWait     "Er Random wait" 'G' = 0.0f,   // wait between two spawns
 25 enum ProjectileType m_ptType  "Er Type" 'T' = PRT_LAVA_COMET,
 26 BOOL m_bShootInArc      "Er Shoot In Arc" 'S' = TRUE,
 
 27 FLOAT m_fProjectileStretch "Er projectile stretch" =  1.0f,   // strecth

components:
  1 class   CLASS_PROJECTILE      "Classes\\Projectile.ecl",

functions:
  void Precache(void) {
    PrecacheClass(CLASS_PROJECTILE, m_ptType);
  }

  /* calculates launch velocity and heading correction for angular launch */
  void CalculateAngularLaunchParams(
    CMovableEntity *penTarget,
    FLOAT3D vShooting,
    FLOAT3D vTarget, FLOAT3D vSpeedDest,
    ANGLE aPitch,
    ANGLE &aHeading,
    FLOAT &fLaunchSpeed)
  {
    FLOAT3D vNewTarget = vTarget;
    FLOAT3D &vGravity = penTarget->en_vGravityDir;
    FLOAT fa = TanFast(AngleDeg(aPitch));
    FLOAT3D vd, vyd0;
    FLOAT fd, fyd0;
    FLOAT fTime = 0.0f;
    FLOAT fLastTime = 0.0f;

    INDEX iIterations = 0;

    do
    {
      iIterations++;
      FLOAT3D vDistance = vNewTarget-vShooting;
      GetParallelAndNormalComponents(vDistance, vGravity, vyd0, vd);
      fd = vd.Length();
      fyd0 = vyd0.Length();
      fLastTime=fTime;
      fTime = Sqrt(2.0f)*Sqrt((fa*fd-fyd0)/penTarget->en_fGravityA);
      vNewTarget = vTarget+vSpeedDest*fTime;
    }
    while( Abs(fTime-fLastTime) > _pTimer->TickQuantum && iIterations<10);

    // calculate launch speed
    fLaunchSpeed = 0.707108f*fd/
      (Cos(AngleDeg(aPitch))*Sqrt((fa*fd-fyd0)/penTarget->en_fGravityA));
    // calculate heading correction
    FLOAT3D vDir = (vNewTarget-vShooting).Normalize();
    ANGLE3D aAngles;
    DirectionVectorToAngles(vDir, aAngles);
    aHeading = aAngles(1);
  }

  // spawn one projectile towards the target
  void SpawnShoot(CEntity *penTarget)
  {
    // if not movable entity
    if (penTarget==NULL || !(penTarget->GetPhysicsFlags()&EPF_MOVABLE)) {
      // do nothing
      return;
    }

    CPlacement3D plLava = GetPlacement();

    FLOAT fSpeed = (m_fMaxSpeed-m_fMinSpeed)*FRnd() + m_fMinSpeed;

    // if shootind with free falling projectile
    if (m_bShootInArc) {
      // calculate speed for angular launch
      FLOAT fPitch = GetPlacement().pl_OrientationAngle(2);
      FLOAT fHeading;
      CalculateAngularLaunchParams((CMovableEntity*)penTarget, 
        GetPlacement().pl_PositionVector,
        penTarget->GetPlacement().pl_PositionVector,
        ((CMovableEntity*)penTarget)->en_vCurrentTranslationAbsolute,
        fPitch, fHeading, fSpeed);

      // if the heading is out of range
      if (Abs(NormalizeAngle(GetPlacement().pl_OrientationAngle(1)-fHeading)) >m_fAngle) {
        // do nothing
        return;
      }
      plLava.pl_OrientationAngle(1) = fHeading;
    // if shootind with propelled projectile
    } else {
      // calculate direction
      FLOAT3D vTargetDir = (penTarget->GetPlacement().pl_PositionVector-
        GetPlacement().pl_PositionVector).Normalize();
      FLOAT3D vShootDir;
      AnglesToDirectionVector(GetPlacement().pl_OrientationAngle, vShootDir);

      // if the heading is out of range
      if (Abs(vTargetDir%vShootDir) < Cos(m_fAngle)) {
        // do nothing
        return;
      }

      DirectionVectorToAngles(vTargetDir, plLava.pl_OrientationAngle);
    }

    // create lava
    SpawnProjectile(plLava, fSpeed);
  }

  // spawn one projectile in random direction
  void SpawnRandom(void)
  {
    // generate speed
    FLOAT fSpeed = (m_fMaxSpeed-m_fMinSpeed)*FRnd() + m_fMinSpeed;
    ANGLE3D aAngle((FRnd()*2-1)*m_fAngle, (FRnd()*2-1)*m_fAngle, 0);
    // create placement
    CPlacement3D plLava(FLOAT3D(0, 0, 0), aAngle);
    plLava.RelativeToAbsolute(GetPlacement());

    SpawnProjectile(plLava, fSpeed);
  }

  // fire projectile in given direction with given speed
  void SpawnProjectile(const CPlacement3D &pl, FLOAT fSpeed)
  {
    CEntityPointer penLava = CreateEntity(pl, CLASS_PROJECTILE);

    // launch
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = m_ptType;
    eLaunch.fSpeed = fSpeed;
    eLaunch.fStretch=m_fProjectileStretch;
    penLava->Initialize(eLaunch);

    // stretch
    if (!(penLava->GetFlags()&ENF_DELETED)) {
      FLOAT3D fStretchRandom(1, 1, 1);
      if (m_bRandomStretch) {
        fStretchRandom(1) = (FRnd()*m_fStretchWidth *2 - m_fStretchWidth ) + 1;
        fStretchRandom(2) = (FRnd()*m_fStretchHeight*2 - m_fStretchHeight) + 1;
        fStretchRandom(3) = (FRnd()*m_fStretchDepth *2 - m_fStretchDepth ) + 1;
      }
      FLOAT3D vOldStretch = penLava->GetModelObject()->mo_Stretch;
      penLava->GetModelObject()->mo_Stretch = FLOAT3D(
        m_fStretchAll*m_fStretchX*fStretchRandom(1)*vOldStretch(1),
        m_fStretchAll*m_fStretchY*fStretchRandom(2)*vOldStretch(2),
        m_fStretchAll*m_fStretchZ*fStretchRandom(3)*vOldStretch(3));
      penLava->ModelChangeNotify();
    }
  }
procedures:
/************************************************************
 *                    A C T I O N S                         *
 ************************************************************/
  // active state
  Active(EVoid)
  {
    wait() {
      on (EBegin) : { call AutoSpawns(); }
      on (EEnvironmentStop) : { jump Inactive(); }
    }
  };

  // inactive state
  Inactive(EVoid)
  {
    wait() {
      on (EBegin) : { resume; }
      on (EEnvironmentStart) : { jump Active(); }
    }
  };

  // spawn projectiles automatically
  AutoSpawns(EVoid)
  {
    while (TRUE) {
      // wait before spawn next
      autowait(m_fTime);

      // spawn one projectile
      SpawnRandom();

      // random wait
      if (m_fRandomWait > 0.0f) { autowait(m_fRandomWait); }
    }
  };



/************************************************************
 *                M  A  I  N    L  O  O  P                  *
 ************************************************************/
  // main loop
  MainLoop(EVoid) {
    wait() {
      on(EBegin) : {
        call Inactive();
      };
      on(ETrigger eTrigger) : {
        SpawnShoot(eTrigger.penCaused);
        resume;
      }
      otherwise() : {resume;};
    }

    // cease to exist
    Destroy();

    return;
  };

  Main(EVoid) {
    // init as model
    CModelHolder2::InitModelHolder();

    // limit values
    if (m_fTime <= 0.0f) { m_fTime = 0.05f; }
    if (m_fMaxSpeed < m_fMinSpeed) { m_fMaxSpeed = m_fMinSpeed; }
    if (m_fAngle < 0.0f) { m_fAngle = 0.0f; }

    // stretch factors must not have extreme values
    if (Abs(m_fStretchX)  < 0.01f) { m_fStretchX   = 0.01f;  }
    if (Abs(m_fStretchY)  < 0.01f) { m_fStretchY   = 0.01f;  }
    if (Abs(m_fStretchZ)  < 0.01f) { m_fStretchZ   = 0.01f;  }
    if (m_fStretchAll< 0.01f) { m_fStretchAll = 0.01f;  }

    if (Abs(m_fStretchX)  >100.0f) { m_fStretchX   = 100.0f*Sgn(m_fStretchX); }
    if (Abs(m_fStretchY)  >100.0f) { m_fStretchY   = 100.0f*Sgn(m_fStretchY); }
    if (Abs(m_fStretchZ)  >100.0f) { m_fStretchZ   = 100.0f*Sgn(m_fStretchZ); }
    if (m_fStretchAll>100.0f) { m_fStretchAll = 100.0f; }

    if (m_fStretchWidth <0.0f) { m_fStretchWidth  = 0.0f; };
    if (m_fStretchHeight<0.0f) { m_fStretchHeight = 0.0f; };
    if (m_fStretchDepth <0.0f) { m_fStretchDepth  = 0.0f; };
    if (m_fStretchWidth >1.0f) { m_fStretchWidth  = 1.0f; };
    if (m_fStretchHeight>1.0f) { m_fStretchHeight = 1.0f; };
    if (m_fStretchDepth >1.0f) { m_fStretchDepth  = 1.0f; };

    jump MainLoop();
  }
};
