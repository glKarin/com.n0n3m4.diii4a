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

616
%{
#include "EntitiesMP/StdH/StdH.h"
#define RAND_05 (FLOAT(rand())/(float)(RAND_MAX)-0.5f)
#define LAUNCH_SPEED 32.0f
%}

class CFireworks : CRationalEntity {
name      "Fireworks";
thumbnail "Thumbnails\\Eruptor.tbn";
features  "IsTargetable", "HasName";

properties:
 
 1 RANGE   m_rRndRadius        "Random radius" = 50.0f,
 10 CSoundObject m_soFly,       
 11 CSoundObject m_soExplosion, 
 12 FLOAT m_tmActivated=0.0f,

 20 CTString m_strName        "Name" 'N' ="",

 50 FLOAT m_tmLastAnimation=0.0f,

{
  CEmiter m_emEmiter;
}

components:

 1 model   MODEL_MARKER     "Models\\Editor\\Axis.mdl",
 2 texture TEXTURE_MARKER   "Models\\Editor\\Vector.tex",
 3 sound   SOUND_FLY         "SoundsMP\\Misc\\Whizz.wav",
 4 sound   SOUND_EXPLODE     "SoundsMP\\Misc\\Firecrackers.wav",
 
functions:
  void Read_t( CTStream *istr) // throw char *
  { 
    CRationalEntity::Read_t(istr);
    m_emEmiter.Read_t(*istr);
  }
  
  void Write_t( CTStream *istr) // throw char *
  { 
    CRationalEntity::Write_t(istr);
    m_emEmiter.Write_t(*istr);
  }
 
  void RenderParticles(void)
  {
    FLOAT tmNow = _pTimer->CurrentTick();
    if( tmNow>m_tmLastAnimation)
    {
      FLOAT fRatio=CalculateRatio(m_tmActivated-tmNow,0.0f,6.0f,1,0);
      FLOAT fGPower=(Min(fRatio,0.5f)-0.5f)*2.0f*10.0f;
      m_emEmiter.em_vG=FLOAT3D(0, fGPower, 0);
      m_emEmiter.AnimateParticles();
      m_tmLastAnimation=tmNow;
      
      for(INDEX i=0; i<m_emEmiter.em_aepParticles.Count(); i++)
      {
        CEmittedParticle &ep=m_emEmiter.em_aepParticles[i];
        if(ep.ep_tmEmitted<0) {continue;};
        FLOAT fLiving=tmNow-ep.ep_tmEmitted;
        FLOAT fSpeed=0.0f;

        if( fLiving>=6.0f)
        {
          fSpeed=0.0f;
        }
        else
        {
          //fSpeed=(0.996f+0.387f*fLiving-0.158f*fLiving*fLiving)*LAUNCH_SPEED;

          fSpeed=(1.77f*pow(0.421f,fLiving))*LAUNCH_SPEED;
          /*
          FLOAT fSpeedRatio=1.0f-(Clamp(fLiving,2.0f,4.0f)-2.0f)/2.0f;
          fSpeed=fSpeedRatio*LAUNCH_SPEED;
          */
        }
        FLOAT3D vNormalized=ep.ep_vSpeed;
        vNormalized.Normalize();
        ep.ep_vSpeed=vNormalized*(4.0f+fSpeed);
      }
    }
    m_emEmiter.RenderParticles();
  }

procedures:
  SpawnFireworks()
  {
    PlaySound(m_soFly, SOUND_FLY, 0);
    autowait(GetSoundLength(SOUND_FLY));
    PlaySound(m_soExplosion, SOUND_EXPLODE, 0);

    // add emited firework sparks
    FLOAT3D vRndPos=FLOAT3D( RAND_05, RAND_05, RAND_05)*m_rRndRadius;
    FLOAT3D vPos=GetPlacement().pl_PositionVector+vRndPos;
    
    m_emEmiter.em_vG=FLOAT3D(0,0,0);
    m_emEmiter.em_iGlobal=(INDEX) (FRnd()*16);
    
    UBYTE ubRndH = UBYTE( FRnd()*255);
    UBYTE ubRndS = UBYTE( 255);
    UBYTE ubRndV = UBYTE( 255);
    m_emEmiter.em_colGlobal=C_WHITE|CT_OPAQUE;//HSVToColor(ubRndH, ubRndS, ubRndV)|CT_OPAQUE;

    FLOAT tmNow = _pTimer->CurrentTick();
    m_tmActivated=tmNow;
    INDEX ctSparks=128;
    for( INDEX iSpark=0; iSpark<ctSparks; iSpark++)
    {
      FLOAT tmBirth=tmNow+(iSpark+RAND_05)*_pTimer->TickQuantum/ctSparks*2.0f;
      FLOAT fLife=4.0f+RAND_05*2.0f;
      FLOAT fStretch=(1.0f+RAND_05*0.25f)*2.5f;
      FLOAT3D vSpeed=FLOAT3D( RAND_05, RAND_05, RAND_05);
      vSpeed=vSpeed.Normalize()*LAUNCH_SPEED;
      FLOAT fRotSpeed=RAND_05*360.0f;
      
      /*
      UBYTE ubRndH = UBYTE( FRnd()*16);
      UBYTE ubRndS = UBYTE( 255);
      UBYTE ubRndV = UBYTE( 255);
      COLOR col=HSVToColor(ubRndH, ubRndS, ubRndV)|CT_OPAQUE;
      */
      COLOR col=C_WHITE|CT_OPAQUE;
      m_emEmiter.AddParticle(vPos, vSpeed, RAND_05*360.0f, fRotSpeed, tmBirth, fLife, fStretch, col);
    }
    return EReturn();
  }

  Main()
  {
    // init model
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);
    GetModelObject()->StretchModel(FLOAT3D(4.0f, 4.0f, 4.0f));

    autowait(_pTimer->TickQuantum);

    m_emEmiter.Initialize(this);
    m_emEmiter.em_etType=ET_FIREWORKS01;

    // wait to be triggered
    wait() {
      on (EBegin) : { resume; }
      on (ETrigger) : 
      {
        call SpawnFireworks();
      }
      otherwise (): { resume; }
    }

    return;
  }
};

