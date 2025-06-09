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

510
%{
#include "Entities/StdH/StdH.h"

#define ECF_AIRWAVE ( \
  ((ECBI_BRUSH|ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_TEST) |\
  ((ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_PASS) |\
  ((ECBI_MODEL)<<ECB_IS))
#define EPF_AIRWAVE ( \
  EPF_ONBLOCK_SLIDE|EPF_MOVABLE|EPF_ORIENTEDBYGRAVITY|EPF_TRANSLATEDBYGRAVITY)
%}

// input parameter for air wave
event EAirWave {
  CEntityPointer penLauncher,     // entity which launch it
};


%{
#define SLIDE_TIME 5.0f
%}

class CAirWave : CMovableModelEntity {
name      "Air wave";
thumbnail "";

properties:
  1 CEntityPointer m_penLauncher, // entity which launch it

 // internal -> do not use
 10 FLOAT m_fDamageAmount = 0.0f,   // water hit damage
 11 FLOAT m_fIgnoreTime = 0.0f,     // time when laucher will be ignored
 12 FLOAT m_fStartTime = 0.0f,      // projectile start time

components:
// ********* WATER *********
 10 model   MODEL_AIRWAVE       "Models\\Enemies\\Mamut\\Projectile\\MamutProjectile.mdl",
 11 texture TEXTURE_AIRWAVE     "Models\\Enemies\\Mamut\\Projectile\\MamutProjectile.tex",

functions:
  void PreMoving(void) {
    // stretch model (1-9)
    FLOAT3D vRatio = FLOAT3D(1, 1, 1);
    vRatio *= 8*((_pTimer->CurrentTick()-m_fStartTime)/SLIDE_TIME)+1;
    GetModelObject()->StretchModel(vRatio);
    ModelChangeNotify();

    CMovableModelEntity::PreMoving();
  };



/************************************************************
 *                        FADE OUT                          *
 ************************************************************/
  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient) {
    FLOAT fTimeRemain = m_fStartTime + SLIDE_TIME - _pTimer->CurrentTick();
    if (fTimeRemain < 0.0f) { fTimeRemain = 0.0f; }
    COLOR colAlpha = GetModelObject()->mo_colBlendColor;
    colAlpha = (colAlpha&0xffffff00) + (COLOR(fTimeRemain/SLIDE_TIME*0xff)&0xff);
    GetModelObject()->mo_colBlendColor = colAlpha;

    return CMovableModelEntity::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
  };



/************************************************************
 *                   ATTACK SPECIFIC                        *
 ************************************************************/
  // air wave touch his valid target
  void AirWaveTouch(CEntityPointer penHit) {
    // time passed
    FLOAT fTimeDiff = _pTimer->CurrentTick() - m_fStartTime;
    FLOAT fRatio = (SLIDE_TIME - fTimeDiff) / 5.0f;

    // direct damage
    FLOAT3D vDirection;
    AnglesToDirectionVector(GetPlacement().pl_OrientationAngle, vDirection);
    InflictDirectDamage(penHit, m_penLauncher, DMT_PROJECTILE, 2.0f * fRatio,
               GetPlacement().pl_PositionVector, vDirection);
    // push target away
    FLOAT3D vSpeed;
    GetPitchDirection(90.0f, vSpeed);
    vSpeed = vSpeed * 10.0f * fRatio;
    KickEntity(penHit, vSpeed);
  };



/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
  // --->>> PROJECTILE SLIDE ON BRUSH
  AirWaveSlide(EVoid) {
    m_fStartTime = _pTimer->CurrentTick();
    LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
    // fly loop
    wait(SLIDE_TIME) {
      on (EBegin) : { resume; }
      on (EPass epass) : {
        BOOL bHit;
        // ignore launcher within 1 second
        bHit = epass.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
        if (bHit) {
          AirWaveTouch(epass.penOther);
        }
        resume;
      }
      on (ETouch etouch) : {
        // clear time limit for launcher
        m_fIgnoreTime = 0.0f;
        // air wave is moving to slow (stuck somewhere) -> kill it
        if (en_vCurrentTranslationAbsolute.Length() < 0.25f*en_vDesiredTranslationRelative.Length()) {
          stop;
        }
        resume;
      }
      on (EDeath) : { stop; }
      on (ETimer) : { stop; }
    }
    return EEnd();
  };


  // --->>> MAIN
  Main(EAirWave eaw) {
    // remember the initial parameters
    ASSERT(eaw.penLauncher!=NULL);
    m_penLauncher = eaw.penLauncher;

    // initialization
    InitAsModel();
    SetPhysicsFlags(EPF_AIRWAVE);
    SetCollisionFlags(ECF_AIRWAVE);
    SetModel(MODEL_AIRWAVE);
    SetModelMainTexture(TEXTURE_AIRWAVE);

    // remember lauching time
    m_fIgnoreTime = _pTimer->CurrentTick() + 1.0f;

    // slide
    autocall AirWaveSlide() EEnd;

    // cease to exist
    Destroy();

    return;
  }
};
