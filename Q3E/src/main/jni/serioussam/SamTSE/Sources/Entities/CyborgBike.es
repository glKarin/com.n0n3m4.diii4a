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

331
%{
#include "Entities/StdH/StdH.h"
#include "Models/Enemies/Cyborg/Bike.h"
%}


uses "Entities/BasicEffects";
uses "Entities/Debris";

event ECyborgBike {
  FLOAT fSpeed,
};


%{
static EntityInfo eiCyborgBike = {
  EIBT_FLESH, 1300.0f,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
};
%}


class CCyborgBike : CMovableModelEntity {
name      "Cyborg Bike";
thumbnail "";

properties:
  1 FLOAT m_fSpeed = 0.0f,
  2 INDEX m_iIndex = 0,

components:
  1 class   CLASS_DEBRIS          "Classes\\Debris.ecl",
  2 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",

// ************** BIKE **************
 10 model   MODEL_BIKE            "Models\\Enemies\\Cyborg\\Bike.mdl",
 11 texture TEXTURE_BIKE          "Models\\Enemies\\Cyborg\\Bike.tex",

// ************** REFLECTIONS **************
202 texture TEX_REFL_LIGHTMETAL01       "Models\\ReflectionTextures\\LightMetal01.tex",

// ************** SPECULAR **************
211 texture TEX_SPEC_MEDIUM             "Models\\SpecularTextures\\Medium.tex",


functions:

procedures:
/************************************************************
 *                M  A  I  N    L  O  O  P                  *
 ************************************************************/
  // main loop
  MainLoop(EVoid) {
    SetDesiredTranslation(FLOAT3D(0, -(2.0f+FRnd()*2.0f), -m_fSpeed));
    SetDesiredRotation(ANGLE3D(0, 0, 0));

    // wait to touch brush or time limit or death
    wait (10.0f) {
      on (EBegin) : { resume; }
      // brush touched
      on (ETouch et) : {
        if (et.penOther->GetRenderType()&RT_BRUSH) {
          SetDesiredTranslation(FLOAT3D(0, 0, 0));
          SetDesiredRotation(ANGLE3D(0, 0, 0));
          stop;
        }
        resume;
      }
      on (EDamage) : { resume; }
      on (EDeath) : { stop; }
      on (ETimer) : { stop; }
    }

    // hide yourself
    SwitchToEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // explode
    m_iIndex=0;
    while (m_iIndex<4) {
      // spawn effect
      CPlacement3D plExplosion;
      plExplosion.pl_PositionVector = FLOAT3D(FRnd()*4.0f-2.0f, FRnd()*4.0f-2.0f, FRnd()*2.0f);
      plExplosion.RelativeToAbsolute(GetPlacement());
      ESpawnEffect eSpawnEffect;
      eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
      eSpawnEffect.betType = BET_GRENADE;
      eSpawnEffect.vStretch = FLOAT3D(1,1,1);
      CEntityPointer penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
      penExplosion->Initialize(eSpawnEffect);

      // damage
      FLOAT3D vSource;
      GetEntityInfoPosition(this, eiCyborgBike.vTargetCenter, vSource);
      InflictRangeDamage(this, DMT_EXPLOSION, 15.0f, vSource, 4.0f, 8.0f);

      // next explosion
      autowait(0.1f + FRnd()/5);
      m_iIndex++;
    }

    // cease to exist
    Destroy();

    return;
  };


  // dummy main
  Main(ECyborgBike ecb) {
    m_fSpeed = ecb.fSpeed;

    // declare yourself as a model
    InitAsModel();
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    SetPhysicsFlags(EPF_MODEL_FLYING);
    en_fDensity = 5000.0f;
    SetHealth(50.0f);

    // set your appearance
    SetComponents(this, *GetModelObject(), MODEL_BIKE, TEXTURE_BIKE, TEX_REFL_LIGHTMETAL01, TEX_SPEC_MEDIUM, 0);
    ModelChangeNotify();

    jump MainLoop();

    return;
  };
};
