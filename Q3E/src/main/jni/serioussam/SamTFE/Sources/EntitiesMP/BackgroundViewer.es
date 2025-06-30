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

211
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Marker";

class CBackgroundViewer: CMarker {
name      "Background Viewer";
thumbnail "Thumbnails\\BackgroundViewer.tbn";
features "IsImportant";

properties:
  1 BOOL m_bActive  "Active" 'A' =TRUE,   // set if active at beginning of game
  2 CEntityPointer m_penWorldSettingsController  "World settings controller" 'W',

components:
  1 model   MODEL_MARKER     "Models\\Editor\\Axis.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\Vector.tex"

functions:
  // Validate offered target for one property
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if(penTarget==NULL)
    {
      return FALSE;
    }
    // if gradient marker
    if( slPropertyOffset==_offsetof(CBackgroundViewer, m_penWorldSettingsController))
    {
      return IsOfClass(penTarget, "WorldSettingsController");
    }
    return TRUE;
  }

  BOOL HandleEvent(const CEntityEvent &ee) {
    if (ee.ee_slEvent == EVENTCODE_EStart) {
      GetWorld()->SetBackgroundViewer(this);
      return TRUE;
    }

    return FALSE;
  };

procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    // set name
    if (m_strName=="Marker") {
      m_strName = "Background Viewer";
    }

    if (m_bActive) {
      GetWorld()->SetBackgroundViewer(this);
    }
    return;
  }
};

