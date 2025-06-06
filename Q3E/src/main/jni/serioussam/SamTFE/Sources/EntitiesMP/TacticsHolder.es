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

235
%{
#include "EntitiesMP/StdH/StdH.h"
%}


enum TacticType {
  0 TCT_NONE                     "None",
  1 TCT_DAMP_ANGLE_STRIFE        "Damp angle strafe",
  2 TCT_PARALLEL_RANDOM_DISTANCE "Parallel random distance",
  3 TCT_STATIC_RANDOM_V_DISTANCE "Static random V distance",
};

class CTacticsHolder : CRationalEntity {
name      "TacticsHolder";
thumbnail "Thumbnails\\TacticsHolder.tbn";
features  "HasName", "IsTargetable";

properties:
  1 CTString m_strName          "Name" 'N' = "TacticsHolder",
  2 CTString m_strDescription = "",

  10 enum   TacticType m_tctType "Type" 'Y' = TCT_NONE, // tactic type
  11 FLOAT  m_fParam1  "Parameter 1" 'D' = 0.0f,   // parameter 1
  12 FLOAT  m_fParam2  "Parameter 2" 'F' = 0.0f,   // parameter 2
  13 FLOAT  m_fParam3  "Parameter 3" 'G' = 0.0f,   // parameter 3
  14 FLOAT  m_fParam4  "Parameter 4" 'H' = 0.0f,   // parameter 4
  15 FLOAT  m_fParam5  "Parameter 5" 'J' = 0.0f,   // parameter 5
  /*16 BOOL   m_bRetryOnBlock "Retry tactic when blocked" = FALSE,
  17 INDEX  m_bRetryCount "Number of retries" = 1,*/

  20 FLOAT  m_tmLastActivation = 0.0f, // internal - the last time when manager was triggered

components:
  1 model   MODEL_MANAGER     "ModelsMP\\Editor\\TacticsHolder.mdl",
  2 texture TEXTURE_MANAGER   "ModelsMP\\Editor\\TacticsHolder.tex"

functions:
  const CTString &GetDescription(void) const {
    return m_strDescription;
  }

procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MANAGER);
    SetModelMainTexture(TEXTURE_MANAGER);
 
    while(TRUE) {
      wait()
      {
        on (ETrigger) : {
          m_tmLastActivation = _pTimer->CurrentTick();
          stop;
        }
        otherwise() : {
          resume;
        }
      };  
     
      // wait a bit to recover
      autowait(0.1f);
    }
  }
};
