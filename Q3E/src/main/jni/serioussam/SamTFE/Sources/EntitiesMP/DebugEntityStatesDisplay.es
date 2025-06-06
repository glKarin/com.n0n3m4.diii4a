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

233
%{
#include "EntitiesMP/StdH/StdH.h"
void HUD_SetEntityForStackDisplay(CRationalEntity *pren);
%}


class CEntityStateDisplay : CRationalEntity {
name      "EntityStateDisplay";
thumbnail "Thumbnails\\EntityStateDisplay.tbn";
features  "HasTarget", "HasName";

properties:
  1 CTString m_strName         "Name" 'N' = "EntityStateDisplay",  
  2 CEntityPointer m_penTarget "Target" 'T' COLOR(C_dGREEN|0xFF), // entity which it points to
  

components:
  1 model   MODEL_MARKER     "ModelsMP\\Editor\\Debug_EntityStack.mdl",
  2 texture TEXTURE_MARKER   "ModelsMP\\Editor\\Debug_EntityStack.tex"

functions:
  void ~CEntityStateDisplay()
  {
    HUD_SetEntityForStackDisplay(NULL);
  }
  
procedures:
  Main() {
    
     // init as nothing
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    GetModelObject()->StretchModel(FLOAT3D(0.4f, 0.4f, 0.4f));
    
    // setup target for stack display every 1/10th of the second so
    // that after a reload or restart everything will work allright
    while(TRUE) {
      wait(0.1f)
      {
        on (EBegin) : {
          if (m_penTarget!=NULL) {
            HUD_SetEntityForStackDisplay((CRationalEntity *) m_penTarget.ep_pen);
          } else {
            HUD_SetEntityForStackDisplay(NULL);
          }
          resume;
        }
        on (ETimer) : {
          stop;  
        }
      }
    }
    return;
  };
};