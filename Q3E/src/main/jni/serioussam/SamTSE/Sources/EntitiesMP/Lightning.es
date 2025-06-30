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

607
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/BackgroundViewer.h"
#include "EntitiesMP/WorldSettingsController.h"
#include "EntitiesMP/Light.h"
%}

%{
  struct ThunderInfo
  {
    INDEX ti_iSound;
    FLOAT ti_fThunderStrikeDelay;
  };

  struct ThunderInfo _atiThunderSounds[3] =
  {
    { SOUND_THUNDER1, 0.6f},
    { SOUND_THUNDER2, 0.0f},
    { SOUND_THUNDER3, 0.0f},
  };
%}

class CLightning: CMovableModelEntity {
name      "Lightning";
thumbnail "Thumbnails\\Lightning.tbn";
features  "IsTargetable", "HasName";

properties:
  1 CEntityPointer m_penTarget  "Target" 'T' COLOR(C_BLUE|0xFF),   // ptr to lightninig target
  2 CEntityPointer m_penwsc,      // ptr to world settings controller
  3 CTString m_strName              "Name" 'N' = "Lightning",         // class name
  4 FLOAT m_tmLightningStart = -1.0f,             // lightning start time
  5 CSoundObject m_soThunder,     // sound channel
  6 BOOL m_bBackground "Background" 'B' =FALSE,
  7 CEntityPointer m_penLight   "Light" 'L' COLOR(C_CYAN|0xFF),   // ptr to light
  8 ANIMATION m_iLightAnim      "Light Animation" 'A' = 0,
  9 INDEX m_iSoundPlaying = 0,
  10 FLOAT m_fLightningPower "Lightning power" 'P' = 1.0f,  // lightning's ligting power
  11 FLOAT m_fSoundDelay "Sound delay" 'D' = 0.0f,         // thunder's delay

components:
  1 model   MODEL_TELEPORT     "Models\\Editor\\Teleport.mdl",
  2 texture TEXTURE_TELEPORT   "Models\\Editor\\BoundingBox.tex",
  3 sound   SOUND_THUNDER1       "Sounds\\Environment\\Thunders\\Thunder1.wav",
  4 sound   SOUND_THUNDER2       "Sounds\\Environment\\Thunders\\Thunder2.wav",
  5 sound   SOUND_THUNDER3       "Sounds\\Environment\\Thunders\\Thunder3.wav",

functions:
  void Precache(void) 
  {
    CMovableModelEntity::Precache();
    PrecacheSound(SOUND_THUNDER1);
    PrecacheSound(SOUND_THUNDER2);
    PrecacheSound(SOUND_THUNDER3);
  }

  /* Get anim data for given animation property - return NULL for none. */
  CAnimData *GetAnimData(SLONG slPropertyOffset) 
  {
    if (m_penLight==NULL) {
      return NULL;
    }

    // if light entity
    if (IsOfClass(m_penLight, "Light"))
    {
      CLight *penLight = (CLight*) m_penLight.ep_pen;

      if (slPropertyOffset==_offsetof(CLightning, m_iLightAnim))
      {
        return penLight->m_aoLightAnimation.GetData();
      }
    }
    else
    {
      WarningMessage("Target '%s' is not of light class!", (const char *) m_penLight->GetName());
    }
    return NULL;
  };

  void RenderParticles(void)
  {
    if( m_penTarget==NULL || m_tmLightningStart == -1) {return;};

    TIME tmNow = _pTimer->GetLerpedCurrentTick();
    // if lightning is traveling
    if(
      ((tmNow-m_tmLightningStart) > 0.0f) &&
      ((tmNow-m_tmLightningStart) < 1.5f) )
    {
      // render lightning particles
      FLOAT3D vSrc = GetPlacement().pl_PositionVector;
      FLOAT3D vDst = m_penTarget->GetPlacement().pl_PositionVector;
	  
      // [SSE] Lightning - Potential Crash Fix
      if (vSrc != vDst) {
        Particles_Lightning( vSrc, vDst, m_tmLightningStart);
      }
    }
  }

procedures:
  LightningStike()
  {
    // choose random sound
    m_iSoundPlaying = 1+IRnd()%( ARRAYCOUNT(_atiThunderSounds)-1);
    if( m_fSoundDelay != 0)
    {
      m_iSoundPlaying=0;
    }
    m_soThunder.SetVolume(1.5f*m_fLightningPower, 1.5f*m_fLightningPower);
    m_soThunder.SetPitch(Lerp(0.9f, 1.2f, FRnd()));
    
    if( m_fSoundDelay == 0.0f)
    {
      // play thunder !
      PlaySound(m_soThunder, _atiThunderSounds[ m_iSoundPlaying].ti_iSound, 0);
    }

    // wait for sound to progress to lightning strike
    if (_atiThunderSounds[ m_iSoundPlaying].ti_fThunderStrikeDelay>0.0f) {
    autowait( _atiThunderSounds[ m_iSoundPlaying].ti_fThunderStrikeDelay);
    }

    // remember current time as lightning start time
    TIME tmNow = _pTimer->CurrentTick();
    m_tmLightningStart = tmNow;
    // also in world settings controller
    ((CWorldSettingsController *) m_penwsc.ep_pen)->m_tmLightningStart = tmNow;
    // set power of lightning
    ((CWorldSettingsController *) m_penwsc.ep_pen)->m_fLightningPower = m_fLightningPower;

    // trigger light animation
    if( m_penLight != NULL)
    {
      EChangeAnim eChange;
      eChange.iLightAnim  = m_iLightAnim;
      eChange.bLightLoop  = FALSE;
      m_penLight->SendEvent(eChange);
    }

    if( m_fSoundDelay != 0.0f)
    {
      // wait given delay time
      autowait( m_fSoundDelay);
      // play thunder !
      PlaySound(m_soThunder, _atiThunderSounds[ m_iSoundPlaying].ti_iSound, 0);
    }
    
    // wait until end of sound
    wait( GetSoundLength(_atiThunderSounds[ m_iSoundPlaying].ti_iSound)-
      _atiThunderSounds[ m_iSoundPlaying].ti_fThunderStrikeDelay)
    {
      on (ETimer) :
      {
        stop;
      }
      otherwise() :
      {
        resume;
      };
    }

    return EBegin();
  }

  Main(EVoid)
  {
    // set appearance
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_TELEPORT);
    SetModelMainTexture(TEXTURE_TELEPORT);

    // see if it is lightning on background
    if (m_bBackground)
    {
      SetFlags(GetFlags()|ENF_BACKGROUND);
    }
    else
    {
      SetFlags(GetFlags()&~ENF_BACKGROUND);
    }

    // obtain bcg viewer entity
    CBackgroundViewer *penBcgViewer = (CBackgroundViewer *) GetWorld()->GetBackgroundViewer();
    if( penBcgViewer == NULL)
    {
      // don't do anything
      return;
    }

    // obtain world settings controller 
    m_penwsc = penBcgViewer->m_penWorldSettingsController;
    if( m_penwsc == NULL)
    {
      // don't do anything
      return;
    }
    
    // must be world settings controller entity
    if (!IsOfClass(m_penwsc, "WorldSettingsController"))
    {
      // don't do anything
      return;
    }

    // lightning target must be marker
    if( (m_penTarget == NULL) || (!IsOfClass(m_penTarget, "Marker")) )
    {
      if( m_penTarget != NULL)
      {
        WarningMessage("Target '%s' is not of Marker class!", (const char *) m_penTarget->GetName());
      }
      // don't do anything
      return;
    }

    // stretch model
    FLOAT3D vDirection =   
      (m_penTarget->GetPlacement().pl_PositionVector-
      GetPlacement().pl_PositionVector);
    
    FLOAT3D vStretch = vDirection;
    vStretch(1) = 1.0f;
    vStretch(2) = 1.0f;
    vStretch(3) = -vDirection.Length();

    // set entity orientation
    CPlacement3D pl = GetPlacement();
    DirectionVectorToAngles(vDirection.Normalize(), pl.pl_OrientationAngle);
    SetPlacement(pl);
    
    GetModelObject()->StretchModel(vStretch);
    ModelChangeNotify();

    // correct power factor to fall under 0-1 boundaries
    m_fLightningPower = Clamp( m_fLightningPower, 0.0f, 1.0f);

    // spawn in world editor
    autowait(0.1f);

    while (TRUE)
    {
      wait()
      {
        on (EBegin) : { resume; }
        on (ETrigger eTrigger) :
        {
          call LightningStike();
          resume;
        }
        otherwise() :
        {
          resume;
        };
      };
    }
  }
};
