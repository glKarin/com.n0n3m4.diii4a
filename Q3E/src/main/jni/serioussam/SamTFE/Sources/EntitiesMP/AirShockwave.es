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

// WARNING: probable sync bads, doesn't work right

349
%{
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/Enemies/AirElemental/ShockWaveBase.h"
#include "ModelsMP/Enemies/AirElemental/ShockWave.h"
%}

uses "EntitiesMP/BasicEffects";

// input parameter for launching the projectile
event EAirShockwave {
  CEntityPointer penLauncher,     // who launched it
  FLOAT fHeight,                  // height in the beginning
  FLOAT fEndWidth,                // width along the X,Z axes in the end
  FLOAT fDuration,                // how long to live
};

%{
// shockwave
#define ECF_SHOCKAWAVE ( \
  ((ECBI_BRUSH|ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_TEST) |\
  ((ECBI_BRUSH|ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_PASS) |\
  ((ECBI_MODEL)<<ECB_IS))
#define EPF_SHOCKAWAVE ( \
  EPF_ONBLOCK_STOP|EPF_ORIENTEDBYGRAVITY|EPF_ABSOLUTETRANSLATE)

#define SHOCKWAVE_HEIGHT  5.0f
#define SHOCKWAVE_WIDTH   0.1f
  
%}

class CAirShockwave : CMovableModelEntity {
name      "AirShockwave";
thumbnail "";

properties:
  1 CEntityPointer m_penLauncher,     // who lanuched it
  2 FLOAT m_fHeight = 0.0f,           // beginning stretch
  3 FLOAT m_fEndWidth = 0.0f,         // ending stretch along the X,Z axes   
  4 FLOAT m_tmBegin = 0.0f,           // time when animation began
  5 FLOAT m_tmEnd = 0.0f,             // time to destroy
  6 FLOAT m_fDuration = 0.0f,         // how long?
  7 BOOL  m_bGrowing = FALSE,         // are we growing?
  8 FLOAT m_tmLastGrow = 0.0f,        // last grow time
  9 FLOAT m_fFadeStartTime = 0.0f,    // when the fading started
 10 FLOAT m_fFadeStartPercent = 0.6f, // when to start the fading
  
 // internal variables
 20 FLOAT m_fStretchY = 0.0f,
 21 FLOAT m_fBeginStretchXZ = 0.0f,
 22 FLOAT m_fEndStretchXZ = 0.0f,
 
 25 FLOATaabbox3D m_boxMaxSize = FLOATaabbox3D(FLOAT3D(0,0,0), FLOAT3D(1,1,1)),

 30 BOOL m_bFadeOut = FALSE,         // set this when fading out

components:
  1 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",

  10 model   MODEL_INVISIBLE    "ModelsMP\\Enemies\\AirElemental\\ShockwaveBase.mdl",
  11 model   MODEL_SHOCKWAVE    "ModelsMP\\Enemies\\AirElemental\\Shockwave.mdl",
  12 texture TEXTURE_SHOCKWAVE  "ModelsMP\\Enemies\\AirElemental\\Twister.tex",

functions:
  
  // get the attachment that IS the AirShockwave
  CModelObject *ShockwaveModel(void) {
    CAttachmentModelObject &amo0 = *GetModelObject()->GetAttachmentModel(SHOCKWAVEBASE_ATTACHMENT_SHOCKWAVE);
    return &(amo0.amo_moModelObject);
  };

  // per-frame adjustments here
  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient) {
    // growing
    if (m_bGrowing) {
      FLOAT3D vSize;
      FLOAT fLifeTime = _pTimer->GetLerpedCurrentTick() - m_tmBegin;
      vSize(1) = (fLifeTime/m_fDuration)*(m_fEndStretchXZ-m_fBeginStretchXZ)+m_fBeginStretchXZ;
      vSize(2) = m_fStretchY;
      vSize(3) = vSize(1);
      
      ShockwaveModel()->StretchModel(vSize);

      // start fadeout if more than 70% of lifetime passed
      if ((fLifeTime/m_fDuration)>m_fFadeStartPercent && !m_bFadeOut) {
        m_bFadeOut = TRUE;
        m_fFadeStartTime = _pTimer->GetLerpedCurrentTick();
      }
      
      // remember last grow time
      m_tmLastGrow = _pTimer->GetLerpedCurrentTick();
    }
    
    // fading out
    if (m_bFadeOut) {
      FLOAT fTimeRemain = m_tmEnd - _pTimer->GetLerpedCurrentTick();
      FLOAT fFadeTime = (1 - m_fFadeStartPercent)*m_fDuration;
      if (fTimeRemain < 0.0f) { fTimeRemain = 0.0f; }
      COLOR colAlpha = ShockwaveModel()->mo_colBlendColor;
      colAlpha = (colAlpha&0xffffff00) + (COLOR(fTimeRemain/fFadeTime*0xff)&0xff);
      ShockwaveModel()->mo_colBlendColor = colAlpha;
    }
    return CMovableModelEntity::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
  };


  // throw away an entity when touched
  void LaunchEntity(CEntity *pen) {
    // don't launch air elemental and twisters and shockwaves and any items
    if (IsOfClass(pen, "AirElemental") || IsOfClass(pen, "Twister")
      || IsDerivedFromClass(pen, "Item") || IsOfClass(pen, "AirShockwave")) {
      return;
    }
    if (pen->GetPhysicsFlags()&EPF_MOVABLE) {
      FLOAT3D vSpeed;
      vSpeed = pen->GetPlacement().pl_PositionVector - GetPlacement().pl_PositionVector;
      if (vSpeed(2)<vSpeed.Length()*0.5f) { vSpeed(2)=vSpeed.Length()*0.5f; }
      vSpeed.Normalize();
      vSpeed = vSpeed*50.0f;
      ((CMovableEntity &)*pen).GiveImpulseTranslationAbsolute(vSpeed);
    }
  };
  
  void TestForCollisionAndLaunchEntity() {
    // test the bounding box for all colliding entities
    static CStaticStackArray<CEntity *> apenNearEntities;
    FLOAT fLifeTime = _pTimer->CurrentTick() - m_tmBegin;
    FLOAT fCurrentRadius = Lerp(SHOCKWAVE_WIDTH, m_fEndWidth, fLifeTime/m_fDuration)/2.0f;

    FLOATaabbox3D m_boxCurrent = m_boxMaxSize;
    m_boxCurrent += GetPlacement().pl_PositionVector;
    
    // width of 'launch belt'
    FLOAT fBeltWidth = m_fEndWidth*_pTimer->TickQuantum*2.0f/m_fDuration;
      
    GetWorld()->FindEntitiesNearBox(m_boxCurrent, apenNearEntities);
    for (INDEX i=0; i<apenNearEntities.Count(); i++)
    {
      FLOAT fDistance = DistanceTo(this, apenNearEntities[i]);
      FLOATaabbox3D m_boxEntity;
      apenNearEntities[i]->GetBoundingBox(m_boxEntity);
      if (fDistance<(fCurrentRadius+fBeltWidth/2.0f) && fDistance>(fCurrentRadius-fBeltWidth/2.0f) &&
          m_boxEntity.HasContactWith(m_boxCurrent)) {
        LaunchEntity(apenNearEntities[i]);
      }
    }

    CMovableModelEntity::PreMoving();
  };
  
procedures:

  // --->>> MAIN
  Main(EAirShockwave eas) {
    // remember the initial parameters
    ASSERT(eas.penLauncher!=NULL);
    ASSERT(eas.fHeight>0.0f);
    ASSERT(eas.fEndWidth>0.1f);
    ASSERT(eas.fDuration>0.0f);
    m_penLauncher = eas.penLauncher;
    m_fHeight = eas.fHeight;
    m_fEndWidth = eas.fEndWidth;
    m_fDuration = eas.fDuration;

    // calculate stretches from parameters
    m_fStretchY = m_fHeight/SHOCKWAVE_HEIGHT;
    m_fBeginStretchXZ = 1.0f;
    m_fEndStretchXZ = m_fEndWidth/SHOCKWAVE_WIDTH;
    
    FLOAT3D v1 = FLOAT3D(-m_fEndWidth/2.0f, 0.0f, -m_fEndWidth/2.0f);
    FLOAT3D v2 = FLOAT3D(+m_fEndWidth/2.0f, m_fHeight, +m_fEndWidth/2.0f);
    m_boxMaxSize = FLOATaabbox3D(v1, v2);
    
    // initialization
    InitAsModel();
    SetPhysicsFlags(EPF_SHOCKAWAVE);
    SetCollisionFlags(ECF_SHOCKAWAVE);
    SetFlags(GetFlags() | ENF_SEETHROUGH);

    // set appearance
    SetModel(MODEL_INVISIBLE);
    AddAttachmentToModel(this, *GetModelObject(), SHOCKWAVEBASE_ATTACHMENT_SHOCKWAVE, MODEL_SHOCKWAVE, TEXTURE_SHOCKWAVE, 0, 0, 0);
    GetModelObject()->StretchModel(FLOAT3D(1.0f, 1.0f, 1.0f));
    ModelChangeNotify();
    ShockwaveModel()->StretchModel(FLOAT3D(m_fBeginStretchXZ, m_fStretchY, m_fBeginStretchXZ));

    autowait(_pTimer->TickQuantum);

    m_tmBegin = _pTimer->CurrentTick();
    m_tmEnd = m_tmBegin + m_fDuration;
    m_tmLastGrow = _pTimer->CurrentTick();
    m_bGrowing = TRUE;

    while(_pTimer->CurrentTick()<m_tmEnd)
    {
      autowait(_pTimer->TickQuantum);
      TestForCollisionAndLaunchEntity();        
    }
    // cease to exist
    Destroy();
    return;
  }
};
