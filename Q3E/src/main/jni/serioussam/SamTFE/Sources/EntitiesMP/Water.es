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

508
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Light";

enum WaterSize {
  0 WTS_SMALL     "",     // small water
  1 WTS_BIG       "",     // big water
  2 WTS_LARGE     "",     // large water
};


// input parameter for water
event EWater {
  CEntityPointer penLauncher,     // entity which launch it
  enum WaterSize EwsSize,         // water size
};


%{
#define FLY_TIME 5.0f
%}

class CWater : CMovableModelEntity {
name      "Water";
thumbnail "";

properties:
  1 CEntityPointer m_penLauncher,           // entity which launch it
  2 enum WaterSize m_EwsSize = WTS_SMALL,   // water size (type)

 // internal -> do not use
 10 FLOAT m_fDamageAmount = 0.0f,        // water hit damage
 11 FLOAT m_fIgnoreTime = 0.0f,          // time when laucher will be ignored
 12 FLOAT m_fPushAwayFactor = 0.0f,      // push away on hit

{
  CLightSource m_lsLightSource;
}


components:
  1 class   CLASS_LIGHT         "Classes\\Light.ecl",

// ********* WATER *********
 10 model   MODEL_WATER         "Models\\Enemies\\Elementals\\Projectile\\WaterDrop.mdl",
 11 texture TEXTURE_WATER       "Models\\Enemies\\Elementals\\WaterManFX.tex",

// ************** SPECULAR **************
210 texture TEX_SPEC_WEAK           "Models\\SpecularTextures\\Weak.tex",
211 texture TEX_SPEC_MEDIUM         "Models\\SpecularTextures\\Medium.tex",
212 texture TEX_SPEC_STRONG         "Models\\SpecularTextures\\Strong.tex",



functions:
  /* Read from stream. */
  void Read_t( CTStream *istr) // throw char *
  {
    CRationalEntity::Read_t(istr);
    SetupLightSource();
  };

  /* Get static light source information. */
  CLightSource *GetLightSource(void)
  {
    if (!IsPredictor()) {
      return &m_lsLightSource;
    } else {
      return NULL;
    }
  };

  // Setup light source
  void SetupLightSource(void)
  {
    // setup light source
    CLightSource lsNew;
    lsNew.ls_ulFlags = LSF_NONPERSISTENT|LSF_DYNAMIC;
    lsNew.ls_colColor = C_lBLUE;
    lsNew.ls_rFallOff = 1.0f;
    lsNew.ls_rHotSpot = 0.2f;
    lsNew.ls_plftLensFlare = NULL;
    lsNew.ls_ubPolygonalMask = 0;
    lsNew.ls_paoLightAnimation = NULL;

    m_lsLightSource.ls_penEntity = this;
    m_lsLightSource.SetLightSource(lsNew);
  };

  // render particles
  void RenderParticles(void) {
  };


  // water touch his valid target
  void WaterTouch(CEntityPointer penHit) {
    // direct damage
    FLOAT3D vDirection;
    AnglesToDirectionVector(GetPlacement().pl_OrientationAngle, vDirection);
    InflictDirectDamage(penHit, m_penLauncher, DMT_PROJECTILE, m_fDamageAmount,
               GetPlacement().pl_PositionVector, vDirection);
    // push target away
    FLOAT3D vSpeed;
    GetHeadingDirection(0.0f, vSpeed);
    vSpeed = vSpeed * m_fPushAwayFactor;
    KickEntity(penHit, vSpeed);
  };



/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
  // --->>> PROJECTILE SLIDE ON BRUSH
  WaterFly(EVoid) {
    // fly loop
    wait(FLY_TIME) {
      on (EBegin) : { resume; }
      on (EPass epass) : {
        BOOL bHit;
        // ignore launcher within 1 second
        bHit = epass.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
        // ignore water
        bHit &= !(IsOfClass(epass.penOther, "Water"));
        if (bHit) {
          WaterTouch(epass.penOther);
          stop;
        }
        resume;
      }
      on (ETouch etouch) : {
        // clear time limit for launcher
        m_fIgnoreTime = 0.0f;
        // ignore brushes
        BOOL bHit;
        bHit = !(etouch.penOther->GetRenderType() & RT_BRUSH);
        // ignore another projectile of same type
        bHit &= !(IsOfClass(etouch.penOther, "Water"));
        if (bHit) {
          WaterTouch(etouch.penOther);
          stop;
        }
        // water is moving to slow (stuck somewhere) -> kill it
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
  Main(EWater ew) {
    // remember the initial parameters
    ASSERT(ew.penLauncher!=NULL);
    m_penLauncher = ew.penLauncher;
    m_EwsSize = ew.EwsSize;

    // initialization
    InitAsModel();
    SetPhysicsFlags(EPF_ONBLOCK_SLIDE|EPF_PUSHABLE|EPF_MOVABLE);
    SetCollisionFlags(ECF_PROJECTILE_MAGIC);
    SetComponents(this, *GetModelObject(), MODEL_WATER, TEXTURE_WATER, 0, TEX_SPEC_STRONG, 0);

    // setup
    switch(m_EwsSize) {
      case WTS_SMALL:
        m_fDamageAmount = 10.0f;
        m_fPushAwayFactor = 10.0f;
        LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -30.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
        break;
      case WTS_BIG:
        m_fDamageAmount = 20.0f;
        m_fPushAwayFactor = 20.0f;
        GetModelObject()->StretchModel(FLOAT3D(4.0f, 4.0f, 4.0f));
        LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -50.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
        break;
      case WTS_LARGE:
        m_fDamageAmount = 40.0f;
        m_fPushAwayFactor = 40.0f;
        GetModelObject()->StretchModel(FLOAT3D(16.0f, 16.0f, 16.0f));
        LaunchAsPropelledProjectile(FLOAT3D(0.0f, 0.0f, -80.0f), (CMovableEntity*)(CEntity*)m_penLauncher);
        break;
    }
    ModelChangeNotify();

    // setup light source
    SetupLightSource();

    // remember lauching time
    m_fIgnoreTime = _pTimer->CurrentTick() + 1.0f;

    // fly
    autocall WaterFly() EEnd;

    // cease to exist
    Destroy();

    return;
  }
};
