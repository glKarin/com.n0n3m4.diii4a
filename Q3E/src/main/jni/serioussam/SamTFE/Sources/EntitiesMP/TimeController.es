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

613
%{
#include "EntitiesMP/StdH/StdH.h"
%}

class CTimeController: CRationalEntity {
name      "TimeController";
thumbnail "Thumbnails\\TimeController.tbn";
features  "IsTargetable", "HasName", "IsImportant";

properties:
  1 FLOAT m_fTimeStretch "Time speed" = 1.0f,
  2 FLOAT m_tmFadeIn     "Fade in time" = 0.25f,
  3 FLOAT m_tmInterval   "Auto clear stretch after..." = -1.0f,
  4 BOOL  m_bAbsolute    "Absolute" = TRUE,
  5 FLOAT m_fMyTimer=0.0f,
  6 FLOAT m_tmStretchChangeStart=0.0f,
  7 CTString m_strName   "Name" 'N' = "Time controller",
  8 FLOAT m_fOldTimeStretch=0.0f,
  9 FLOAT m_fNewTimeStretch=0.0f,

components:
  1 model   MODEL_TIME_CONTROLLER     "ModelsMP\\Editor\\TimeControler.mdl",
  2 texture TEXTURE_TIME_CONTROLLER   "ModelsMP\\Editor\\TimeController.tex"

functions:

procedures:
  
  ChangeTimeStretch(EVoid)
  {
    m_fMyTimer=0.0f;
    while( m_fMyTimer<m_tmFadeIn-_pTimer->TickQuantum/2.0f)
    {
      autowait(_pTimer->TickQuantum);
      m_fMyTimer+=_pTimer->TickQuantum/_pNetwork->GetRealTimeFactor();
      FLOAT fNewStretch=Lerp(m_fOldTimeStretch, m_fNewTimeStretch, Clamp(m_fMyTimer/m_tmFadeIn, 0.0f, 1.0f));
      _pNetwork->SetRealTimeFactor(fNewStretch);
    }
    _pNetwork->SetRealTimeFactor(m_fNewTimeStretch);
    return EReturn();
  }

  ApplyTimeStretch(EVoid)
  {
    autocall ChangeTimeStretch() EReturn;
    if( m_tmInterval>0)
    {
      autowait(m_tmInterval);
      autocall ResetTimeStretch() EReturn;
    }
    return EReturn();
  }

  ResetTimeStretch(EVoid)
  {
    if(_pNetwork->GetRealTimeFactor()==1) {return EReturn(); };
    m_fOldTimeStretch=_pNetwork->GetRealTimeFactor();
    m_fNewTimeStretch=1.0f;
    autocall ChangeTimeStretch(EVoid()) EReturn;
    return EReturn();
  }

  Main(EVoid)
  {
    // set appearance
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_TIME_CONTROLLER);
    SetModelMainTexture(TEXTURE_TIME_CONTROLLER);
    
    // spawn in world editor
    autowait(0.1f);

    wait()
    {
      on (EBegin) :
      {
        resume;
      }
      // immediate
      on (EStart eStart) :
      {
        m_fOldTimeStretch=_pNetwork->GetRealTimeFactor();
        m_fNewTimeStretch=m_fTimeStretch;
        call ApplyTimeStretch();
        resume;
      }
      on (EStop) :
      {
        _pNetwork->SetRealTimeFactor(1.0f);
        resume;
      }
      on (EReturn) :
      {
        resume;
      }
    }
  }
};
