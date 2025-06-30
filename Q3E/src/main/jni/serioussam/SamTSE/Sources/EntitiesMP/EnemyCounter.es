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

339
%{
#include "EntitiesMP/StdH/StdH.h"
#include <EntitiesMP/MusicHolder.h>
%}

class CEnemyCounter : CRationalEntity {
name      "EnemyCounter";
thumbnail "Thumbnails\\Counter.tbn";
features "HasName", "IsTargetable";

properties:
  1 CEntityPointer m_penMainMusicHolder,
  2 CTString m_strName        "Name" 'N' ="",
  4 INDEX m_iCountFrom "Count start" 'A' = 100,
  5 INDEX m_iCount = -1,

components:
 0 sound   SOUND_TICK       "Sounds\\Menu\\Select.wav",
 1 model   MODEL_MARKER     "Models\\Editor\\Axis.mdl",
 2 texture TEXTURE_MARKER   "Models\\Editor\\Vector.tex"

functions:
  class CMusicHolder *GetMusicHolder()
  {
    if (m_penMainMusicHolder==NULL) {
      m_penMainMusicHolder = _pNetwork->GetEntityWithName("MusicHolder", 0);
    }
    return (CMusicHolder *) m_penMainMusicHolder.ep_pen;
  }
  void StartCounting(void)
  {
    CMusicHolder *pmh = GetMusicHolder();
    if (pmh==NULL) {
      return;
    }
    pmh->m_penCounter = this;
    m_iCount = m_iCountFrom;
  }
  void CountOne(void)
  {
    if (m_iCount>0) {
      m_iCount-=1;
    }
  }
  void StopCounting(void)
  {
    CMusicHolder *pmh = GetMusicHolder();
    if (pmh==NULL) {
      return;
    }
    m_iCount = 0;
    pmh->m_penCounter = NULL;
  }
procedures:

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

    wait() {
      on(EBegin): {
        resume;
      }
      // when started
      on (EStart): {
        StartCounting();
        resume;
      }
      // when stopped
      on (EStop): {
        StopCounting();
        resume;
      }
      // when triggered 
      on (ETrigger): {
        // if not started yet
        if (m_iCount==-1) {
          // start
          StartCounting();
        }
        CountOne();
        // if finished
        if (m_iCount==0) {
          // stop
          StopCounting();
        }
        resume;
      }
    }

    return;
  }
};
