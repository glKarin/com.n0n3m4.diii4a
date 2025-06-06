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

104
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Marker";

class CShipMarker: CMarker {
name      "Ship Marker";
thumbnail "Thumbnails\\ShipMarker.tbn";

properties:
  1 BOOL m_bHarbor                    "Harbor" 'H' = FALSE,
  2 FLOAT m_fSpeed                    "Speed [m/s]" 'S' = -1.0f,
  3 FLOAT m_fRotation                 "Rotation [deg/s]" 'R' = -1.0f,
  4 FLOAT m_fAcceleration             "Acceleration" 'C' = 10.0f,
  5 FLOAT m_fRockingV                 "Rocking V" 'V' = -1.0f,
  6 FLOAT m_fRockingA                 "Rocking A" 'A' = -1.0f,
  7 FLOAT m_tmRockingChange           "Rocking Change Time" = 3.0f,

components:
  1 model   MODEL_MARKER     "Models\\Editor\\ShipMarker.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\ShipMarker.tex"

functions:
  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker(CTFileName &fnmMarkerClass, CTString &strTargetProperty) const {
    fnmMarkerClass = CTFILENAME("Classes\\ShipMarker.ecl");
    strTargetProperty = "Target";
    return TRUE;
  }
procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    return;
  }
};

