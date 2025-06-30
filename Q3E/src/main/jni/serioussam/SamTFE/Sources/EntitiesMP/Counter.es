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

232
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/ModelHolder2";

class CCounter : CRationalEntity {
name      "Counter";
thumbnail "Thumbnails\\Counter.tbn";
features "HasName", "IsTargetable", "IsImportant";

properties:
  1 FLOAT m_fCountdownSpeed "Countdown speed" 'S' = 12.0f,
  2 CEntityPointer m_penTarget "Zero target" 'T' COLOR(C_WHITE|0x80),
  3 FLOAT m_fNumber = 0.0f,
  4 FLOAT m_tmStart = -1.0f,
  5 CTString m_strName        "Name" 'N' ="",
  6 CSoundObject m_soSound,
  7 INDEX m_iCountFrom "Count start" 'A' = 1023,
 10 CEntityPointer m_pen0  "Bit 0" COLOR(C_RED|0x30),
 11 CEntityPointer m_pen1  "Bit 1" COLOR(C_RED|0x30),
 12 CEntityPointer m_pen2  "Bit 2" COLOR(C_RED|0x30),
 13 CEntityPointer m_pen3  "Bit 3" COLOR(C_RED|0x30),
 14 CEntityPointer m_pen4  "Bit 4" COLOR(C_RED|0x30),
 15 CEntityPointer m_pen5  "Bit 5" COLOR(C_RED|0x30),
 16 CEntityPointer m_pen6  "Bit 6" COLOR(C_RED|0x30),
 17 CEntityPointer m_pen7  "Bit 7" COLOR(C_RED|0x30),
 18 CEntityPointer m_pen8  "Bit 8" COLOR(C_RED|0x30),
 19 CEntityPointer m_pen9  "Bit 9" COLOR(C_RED|0x30),

components:
 0 sound   SOUND_TICK       "Sounds\\Menu\\Select.wav",
 1 model   MODEL_MARKER     "Models\\Editor\\Axis.mdl",
 2 texture TEXTURE_MARKER   "Models\\Editor\\Vector.tex"

functions:
  void Precache(void)
  {
    PrecacheSound( SOUND_TICK);
    CRationalEntity::Precache();
  }
 
  void DisplayNumber(void)
  {
    for( INDEX iDigit=0; iDigit<10; iDigit++)
    {
      CModelHolder2 *pmh = (CModelHolder2 *) (&m_pen0)[iDigit].ep_pen;
      if( pmh!=NULL && pmh->GetModelObject()!=NULL &&
          pmh->GetModelObject()->mo_toTexture.GetData()!=NULL)
      {
        // set texture animation
        INDEX iOldAnim = pmh->GetModelObject()->mo_toTexture.GetAnim();
        INDEX iAnim=(INDEX(m_fNumber)&(1<<iDigit))>>iDigit;
        pmh->GetModelObject()->mo_toTexture.PlayAnim(iAnim, 0);
        
        // play sound
        m_soSound.Set3DParameters(200.0f, 100.0f, 1.0f, 
          Clamp(1.0f+(m_iCountFrom-m_fNumber)/m_iCountFrom*2.0f, 1.0f, 3.0f) );
        if( iDigit==0 && iOldAnim!=iAnim /*iOldAnim==1 && iAnim==0 */&& !m_soSound.IsPlaying())
        {
          PlaySound(m_soSound, SOUND_TICK, SOF_3D|SOF_VOLUMETRIC);
        }
      }
    }
  }

procedures:
  CountDown()
  {
    while( TRUE)
    {
      autowait(_pTimer->TickQuantum);
      FLOAT tmNow = _pTimer->CurrentTick();
      FLOAT tmDelta = tmNow-m_tmStart;
      FLOAT fSub = Clamp( tmDelta/m_fCountdownSpeed, 0.01f, 1.0f);
      m_fNumber = Clamp( m_fNumber-fSub, 0.0f, FLOAT(m_iCountFrom));
      DisplayNumber();
      if( m_fNumber==0)
      {
        return EReturn();
      }
    }
  }

/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    autowait(0.1f);
    m_fNumber = m_iCountFrom;
    DisplayNumber();

    wait() {
      on(EBegin): {
        resume;
      }
      on (ETrigger eTrigger): {
        m_fNumber = m_iCountFrom;
        DisplayNumber();
        m_tmStart = _pTimer->CurrentTick();
        call CountDown();
      }
      on(EReturn): {
        if( m_penTarget!= NULL)
        {
          SendToTarget(m_penTarget, EET_TRIGGER);
        }
        stop;
      }
    }

    return;
  }
};
