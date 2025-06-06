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

class CMarker: CEntity {
name      "Marker";
thumbnail "Thumbnails\\Marker.tbn";
features  "HasName", "HasTarget", "IsTargetable";

properties:

  1 CTString m_strName          "Name" 'N' = "Marker",
  3 CTString m_strDescription = "",
  2 CEntityPointer m_penTarget  "Target" 'T' COLOR(C_dGREEN|0xFF),


components:

  1 model   MODEL_MARKER     "Models\\Editor\\Axis.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\Vector.tex"


functions:

  const CTString &GetDescription(void) const
  {
    ((CTString&)m_strDescription).PrintF("-><none>");
    if (m_penTarget!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s", (const char *) m_penTarget->GetName());
    }
    return m_strDescription;
  }

  /* Check if entity is moved on a route set up by its targets. */
  BOOL MovesByTargetedRoute(CTString &strTargetProperty) const {
    strTargetProperty = "Target";
    return TRUE;
  };
  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker(CTFileName &fnmMarkerClass, CTString &strTargetProperty) const {
    fnmMarkerClass = CTFILENAME("Classes\\Marker.ecl");
    strTargetProperty = "Target";
    return TRUE;
  }


  // this is MARKER !!!!
  virtual BOOL IsMarker(void) const
  {
    return TRUE;
  }


  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    // initial
    SLONG slUsedMemory = sizeof(CMarker) - sizeof(CEntity) + CEntity::GetUsedMemory();
    // add some more
    slUsedMemory += m_strName.Length();
    slUsedMemory += m_strDescription.Length();
    return slUsedMemory;
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

