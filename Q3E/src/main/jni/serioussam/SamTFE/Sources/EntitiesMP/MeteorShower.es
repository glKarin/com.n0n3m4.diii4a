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

615
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Projectile";
//uses "Entities/EnemyMarker";

class CMeteorShower : CRationalEntity {
name      "MeteorShower";
thumbnail "Thumbnails\\Eruptor.tbn";
features  "IsTargetable", "HasName";

properties:
 
  1 CTString m_strName   "Name" 'N' = "Meteor Shower",         // class name
 
 10 ANGLE3D m_aAngle     "Shoot Angle" 'S' = ANGLE3D( AngleDeg(0.0f),AngleDeg(0.0f),AngleDeg(0.0f)),
 11 INDEX   m_iPerTickLaunchChance "Density (1-100)" 'D' = 10,  // 0-100

 15 FLOAT   m_fMinStretch  "Min. Stretch" = 1.0f,
 16 FLOAT   m_fMaxStretch  "Max. Stretch" = 1.1f,

 19 RANGE   m_rSafeArea    "Safe Area" = 10.0f,
 20 RANGE   m_rArea        "Area" = 50.0f,

 30 FLOAT m_fSpeed         "Speed" 'P' = 300.0f,
 40 FLOAT m_fLaunchDistance "Launch distance" 'L' = 500.0f,
 

components:

 1 model   MODEL_MARKER      "ModelsMP\\Editor\\EffectMarker.mdl",
 2 texture TEXTURE_MARKER    "ModelsMP\\Editor\\EffectMarker.tex",

 5 class   CLASS_PROJECTILE  "Classes\\Projectile.ecl",

functions:
  void Precache(void) {
    PrecacheClass(CLASS_PROJECTILE, PRT_METEOR);
  }

  // fire projectile in given direction with given speed
  void SpawnProjectile(const CPlacement3D &pl)
  {
    CEntityPointer pen = CreateEntity(pl, CLASS_PROJECTILE);

    // launch
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = PRT_METEOR;
    eLaunch.fStretch = Lerp(m_fMinStretch, m_fMaxStretch, FRnd());
    eLaunch.fSpeed = m_fSpeed;
    pen->Initialize(eLaunch);
  }

  void MaybeShootMeteor(void)
  {
    INDEX iShoot = IRnd()%100;
  
    if (iShoot<=m_iPerTickLaunchChance)
    {
      CPlacement3D plFall;
    
      plFall.pl_PositionVector = GetPlacement().pl_PositionVector;
      FLOAT fR = Lerp(m_rSafeArea, m_rArea, FRnd());
      FLOAT fA = FRnd()*360.0f;
      plFall.pl_PositionVector += FLOAT3D(CosFast(fA)*fR, 0.05f, SinFast(fA)*fR);
      FLOAT3D vDir;
      AnglesToDirectionVector(m_aAngle, vDir);
      vDir.Normalize();
      CPlacement3D plLaunch=plFall;
      plLaunch.pl_PositionVector=plFall.pl_PositionVector-vDir*m_fLaunchDistance;
      plLaunch.pl_OrientationAngle = m_aAngle;
      SpawnProjectile(plLaunch);
    }
  }

procedures:
/************************************************************
 *                    A C T I O N S                         *
 ************************************************************/
  // active state
  Active(EVoid)
  {
    while (TRUE) {
      
      wait(_pTimer->TickQuantum) {
        on (EBegin) : { 
          resume;
        }
        on (EEnvironmentStop) : {
          jump Inactive();
        }
        on (ETimer) : {
          MaybeShootMeteor();
          stop;
        }
      } // wait

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


  Main(EVoid) {

    if (m_fMinStretch>m_fMaxStretch) { m_fMinStretch = m_fMaxStretch; }
    if (m_rSafeArea>m_rArea) { m_rSafeArea = m_rArea; }

    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    autowait(0.05f);

    jump Inactive();
  }

};
