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

202
%{
#include "EntitiesMP/StdH/StdH.h"
%}

class CAreaMarker: CEntity {
name      "AreaMarker";
thumbnail "Thumbnails\\AreaMarker.tbn";
features  "HasName", "IsTargetable";

properties:
  1 CTString m_strName          "Name" 'N' = "AreaMarker",
  2 CTString m_strDescription = "",
  3 FLOATaabbox3D m_boxArea "Area box" 'B' = FLOATaabbox3D(FLOAT3D(0,0,0), FLOAT3D(10,10,10)),

components:
  1 model   MODEL_AREAMARKER     "Models\\Editor\\Axis.mdl",
  2 texture TEXTURE_AREAMARKER   "Models\\Editor\\Vector.tex"

functions:
  
  void GetAreaBox(FLOATaabbox3D &box) {
    box = m_boxArea;
    box +=GetPlacement().pl_PositionVector;
    return;
  }

procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_AREAMARKER);
    SetModelMainTexture(TEXTURE_AREAMARKER);

    return;
  }
};

