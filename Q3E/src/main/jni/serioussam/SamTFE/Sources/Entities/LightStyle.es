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

201
%{
#include "Entities/StdH/StdH.h"
%}

class CLightStyle : CEntity {
name      "Light";
thumbnail "Models\\Editor\\LightStyle.tbn";
features "HasName", "IsTargetable";

properties:
    2 CTString m_strName               "Name" 'N' = "<light style>",
//    1 LIGHTANIMATION m_iLightAnimation "Light animation" 'A'

components:
    1 model   MODEL_LIGHTSTYLE    "Models\\Editor\\LightSource.mdl",
    2 texture TEXTURE_LIGHTSTYLE  "Models\\Editor\\LightStyle.tex"

functions:
procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_LIGHTSTYLE);
    SetModelMainTexture(TEXTURE_LIGHTSTYLE);

    return;
  }
};
