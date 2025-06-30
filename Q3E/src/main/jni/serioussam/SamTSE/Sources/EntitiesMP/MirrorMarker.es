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

218
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Marker";

enum WarpRotation {
  0 WR_NONE       "none",
  1 WR_BANKING    "banking",
  2 WR_TWIRLING   "twirling",
};

class CMirrorMarker: CMarker {
name      "Mirror Marker";
thumbnail "Thumbnails\\WarpMarker.tbn";
features "IsImportant";


properties:
  1 enum WarpRotation m_wrRotation "Rotation Type" 'R' = WR_NONE,
  2 FLOAT m_fRotationSpeed "Rotation Speed" 'S' = 90.0f,
components:
  1 model   MODEL_IN     "Models\\Editor\\WarpEntrance.mdl",
  2 texture TEXTURE_IN   "Models\\Editor\\Warp.tex",
  3 model   MODEL_OUT    "Models\\Editor\\WarpExit.mdl",
  4 texture TEXTURE_OUT  "Models\\Editor\\Warp.tex"

functions:

  /* Get mirror type name, return empty string if not used. */
  const CTString &GetMirrorName(void)
  {
    return m_strName;
  }
  /* Get mirror. */
  void GetMirror(class CMirrorParameters &mpMirror)
  {
    mpMirror.mp_ulFlags = MPF_WARP;
    mpMirror.mp_plWarpIn = GetLerpedPlacement();
    if (m_penTarget!=NULL) {
      mpMirror.mp_penWarpViewer = m_penTarget;
      mpMirror.mp_plWarpOut = m_penTarget->GetLerpedPlacement();
    } else {
      mpMirror.mp_penWarpViewer = this;
      mpMirror.mp_plWarpOut = GetLerpedPlacement();
    }
    FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
    mpMirror.mp_fWarpFOV = -1.0f;
    if (m_wrRotation==WR_BANKING) {
      mpMirror.mp_plWarpOut.Rotate_Airplane(ANGLE3D(0,0,m_fRotationSpeed*tmNow));
    } else if (m_wrRotation==WR_TWIRLING) {
      ANGLE3D a;
      a(1) = sin(tmNow*3.9)*5.0f;
      a(2) = sin(tmNow*2.7)*5.0f;
      a(3) = sin(tmNow*4.5)*5.0f;
      mpMirror.mp_plWarpOut.Rotate_Airplane(a);
      mpMirror.mp_fWarpFOV = 90.0f+sin(tmNow*7.79f)*5.0f;
    }
  }
procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    if (m_penTarget!=NULL) {
      SetModel(MODEL_IN);
      SetModelMainTexture(TEXTURE_IN);
    } else {
      SetModel(MODEL_OUT);
      SetModelMainTexture(TEXTURE_OUT);
    }

    // set name
    if (m_strName=="Marker") {
      m_strName = "Mirror marker";
    }
    return;
  }
};
