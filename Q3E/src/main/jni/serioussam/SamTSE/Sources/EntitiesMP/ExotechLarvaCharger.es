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

351
%{
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/Enemies/ExotechLarva/Charger/FloorCharger.h"
%}

uses "EntitiesMP/BloodSpray";
uses "EntitiesMP/Projectile";
uses "EntitiesMP/ExotechLarvaBattery";

event EActivateBeam {
  BOOL bTurnOn,
};

class CExotechLarvaCharger : CRationalEntity {
name      "ExotechLarvaCharger";
thumbnail "Thumbnails\\ExotechLarvaCharger.tbn";
features  "HasName", "IsTargetable";

properties:
  
  1 BOOL  m_bActive  = TRUE,
  2 BOOL  m_bBeamActive = FALSE,
  3 FLOAT m_fStretch "Stretch" 'S' = 1.0f,
  7 CTString m_strName  "Name" 'N' = "ExotechLarva Floor Charger",
  8 RANGE m_rSound "Sound Range" = 100.0f,

 10 CEntityPointer m_penBattery01 "Wall Battery 01", 
 11 CEntityPointer m_penBattery02 "Wall Battery 02", 
 12 CEntityPointer m_penBattery03 "Wall Battery 03", 
 13 CEntityPointer m_penBattery04 "Wall Battery 04", 
 14 CEntityPointer m_penBattery05 "Wall Battery 05", 
 15 CEntityPointer m_penBattery06 "Wall Battery 06", 

 20 BOOL m_bCustomShading "Custom Shading" = FALSE,
 21 ANGLE3D m_aShadingDirection "Light direction" 'D' = ANGLE3D( AngleDeg(45.0f),AngleDeg(45.0f),AngleDeg(45.0f)),
 22 COLOR m_colLight "Light Color" = C_WHITE,
 23 COLOR m_colAmbient "Ambient Light Color" = C_BLACK,
 
 50 CSoundObject m_soSound,

components:

  1 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",
  2 class   CLASS_BLOOD_SPRAY   "Classes\\BloodSpray.ecl",
  
  5 model   MODEL_CHARGER       "ModelsMP\\Enemies\\ExotechLarva\\Charger\\FloorCharger.mdl",
  6 texture TEXTURE_CHARGER     "ModelsMP\\Enemies\\ExotechLarva\\Charger\\FloorCharger.tex",

  7 model   MODEL_BEAM          "ModelsMP\\Enemies\\ExotechLarva\\Charger\\Beam.mdl",
  8 texture TEXTURE_BEAM        "ModelsMP\\Enemies\\ExotechLarva\\Charger\\Beam.tex",

  9 model   MODEL_ELECTRICITY   "ModelsMP\\Enemies\\ExotechLarva\\Charger\\ElectricityBeams.mdl",
 10 texture TEXTURE_ELECTRICITY "ModelsMP\\Effects\\Laser\\Laser_Red.tex",

 50 sound   SOUND_HUM           "ModelsMP\\Enemies\\ExotechLarva\\Charger\\Sounds\\FloorChargerHum.wav",
 51 sound   SOUND_SHUTDOWN      "ModelsMP\\Enemies\\ExotechLarva\\Charger\\Sounds\\FloorChargerShutdown.wav",

functions:
 
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if( slPropertyOffset == _offsetof(CExotechLarvaCharger, m_penBattery01) ||
        slPropertyOffset == _offsetof(CExotechLarvaCharger, m_penBattery02) ||
        slPropertyOffset == _offsetof(CExotechLarvaCharger, m_penBattery03) ||
        slPropertyOffset == _offsetof(CExotechLarvaCharger, m_penBattery04) ||
        slPropertyOffset == _offsetof(CExotechLarvaCharger, m_penBattery05) ||
        slPropertyOffset == _offsetof(CExotechLarvaCharger, m_penBattery06))
    {
      if (IsOfClass(penTarget, "ExotechLarvaBattery")) { return TRUE; }
      else { return FALSE; }
    }   
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }
  
  void Precache(void) {
    CRationalEntity::Precache();
    
    PrecacheModel   (MODEL_ELECTRICITY    );
    PrecacheTexture (TEXTURE_ELECTRICITY  );
    PrecacheModel   (MODEL_BEAM           );
    PrecacheTexture (TEXTURE_BEAM         );

    PrecacheSound   (SOUND_HUM            );
    PrecacheSound   (SOUND_SHUTDOWN       );
  }
  

  /* Adjust model shading parameters if needed. */
  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    if (m_bCustomShading)
    {
      colLight   = m_colLight;
      colAmbient = m_colAmbient;
         
      AnglesToDirectionVector(m_aShadingDirection, vLightDirection);
      vLightDirection = -vLightDirection;
    }
    
    return TRUE;
  };


  void UpdateOperationalState(void) {
    
    CEntityPointer *penFirst = &m_penBattery01;
    
    for (INDEX i=0; i<6; i++) {
      CExotechLarvaBattery *penBattery = (CExotechLarvaBattery *) (penFirst[i].ep_pen);
      // if model pointer is valid
      if (penBattery) {
        if (penBattery->m_bActive) { 
          // at least one exists, so we are still active
          m_bActive = TRUE;
          return;        
        }
      }
    }
    // if no batteries found, change the state to inactive
    m_bActive = FALSE;
    EActivateBeam eab;
    eab.bTurnOn = FALSE;
    SendEvent(eab);
    
    PlaySound(m_soSound, SOUND_SHUTDOWN, SOF_3D);
    
    RemoveAttachmentFromModel(*GetModelObject(), FLOORCHARGER_ATTACHMENT_ELECTRICITY);
  }

  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    NOTHING;
  };

  void RenderParticles(void)
  {
    if (m_bBeamActive)
    {
      CEntityPointer *penFirst = &m_penBattery01;
      
      for (INDEX i=0; i<6; i++) {
        CExotechLarvaBattery *penBattery = (CExotechLarvaBattery *) (penFirst[i].ep_pen);
        // if model pointer is valid
        if (penBattery) {
          if (penBattery->m_bActive) { 
            // render electricity
            FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
            Particles_Ghostbuster(GetPlacement().pl_PositionVector + FLOAT3D(0.0f, 0.2f, 0.0f),
                                  penBattery->GetPlacement().pl_PositionVector + FLOAT3D(0.0f, 0.2f, 0.0f),
                                  32, 1.0f);
            Particles_ModelGlow(penBattery, 1e6, PT_STAR05, 1.0f+0.5f*sin(4.0f*tmNow), 4, 0.0f, C_WHITE);
          }
        }
      }
      
    }
  };

procedures:
  
  ActivateBeam()
  {
    AddAttachmentToModel(this, *GetModelObject(), FLOORCHARGER_ATTACHMENT_BEAM, MODEL_BEAM, TEXTURE_BEAM, 0, 0, 0);
    CModelObject &amo = GetModelObject()->GetAttachmentModel(FLOORCHARGER_ATTACHMENT_BEAM)->amo_moModelObject;
    amo.StretchModelRelative(FLOAT3D(m_fStretch, m_fStretch, m_fStretch));
    m_bBeamActive = TRUE;
    return;
  }

  DeactivateBeam()
  {
    RemoveAttachmentFromModel(*GetModelObject(), FLOORCHARGER_ATTACHMENT_BEAM);
    m_bBeamActive = FALSE;
    return;
  }

  Main()
  {
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_FIXED);
    SetCollisionFlags(ECF_IMMATERIAL);
    SetFlags(GetFlags()|ENF_ALIVE);
    
    // set your appearance
    SetModel(MODEL_CHARGER);
    SetModelMainTexture(TEXTURE_CHARGER);
    
    // set stretch factors for height and width
    GetModelObject()->StretchModel(FLOAT3D(m_fStretch, m_fStretch, m_fStretch));
    ModelChangeNotify();
    
    autowait(0.05f);

    m_soSound.Set3DParameters(m_rSound, m_rSound/2.0f, 2.0f, 1.0f);
    m_bActive = FALSE;
    m_bBeamActive = FALSE;

    // wait to be triggered
    wait() {
      on (EBegin) : { resume; }
      on (ETrigger) : { stop; }
      otherwise (): { resume; }
    }

    UpdateOperationalState();
    if (m_bActive) {
      AddAttachmentToModel(this, *GetModelObject(), FLOORCHARGER_ATTACHMENT_ELECTRICITY, MODEL_ELECTRICITY, TEXTURE_ELECTRICITY, 0, 0, 0);
      CAttachmentModelObject *amo = GetModelObject()->GetAttachmentModel(FLOORCHARGER_ATTACHMENT_ELECTRICITY);
      amo->amo_moModelObject.StretchModel(FLOAT3D(m_fStretch, m_fStretch, m_fStretch));
      PlaySound(m_soSound, SOUND_HUM, SOF_3D|SOF_LOOP);
    }

    while (TRUE) {
      wait(0.5f)
      {
        on (EBegin) : {
          resume;
        }
        on (ETimer) : {
          if (m_bActive) {
            UpdateOperationalState();
          }
          stop;
        }
        on (EActivateBeam eab) : {
          if (eab.bTurnOn==TRUE && m_bBeamActive!=TRUE) {
            call ActivateBeam();
          } else if (eab.bTurnOn==FALSE && m_bBeamActive!=FALSE) {
            call DeactivateBeam();
          }
          resume;
        }
      }
    }
    return;
  }

};
