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

214
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Marker";

// world link
enum WorldLinkType {
  1 WLT_FIXED     "Fixed",      // fixed link
  2 WLT_RELATIVE  "Relative",   // relative link
};

class CWorldLink : CMarker {
name      "World link";
thumbnail "Thumbnails\\WorldLink.tbn";
features "IsImportant";

properties:
  1 CTString m_strGroup           "Group" 'G' = "",
  2 CTFileNameNoDep m_strWorld    "World" 'W' = "",
  3 BOOL m_bStoreWorld            "Store world" 'S' = FALSE,
  4 enum WorldLinkType m_EwltType "Type" 'Y' = WLT_RELATIVE,

components:
  1 model   MODEL_WORLDLINK     "Models\\Editor\\WorldLink.mdl",
  2 texture TEXTURE_WORLDLINK   "Models\\Editor\\WorldLink.tex"


functions:
/************************************************************
 *                      START EVENT                         *
 ************************************************************/
  BOOL HandleEvent(const CEntityEvent &ee) {
    if (ee.ee_slEvent == EVENTCODE_ETrigger) {
      ETrigger &eTrigger = (ETrigger &)ee;
      _SwcWorldChange.strGroup = m_strGroup;      // group name
      _SwcWorldChange.plLink = GetPlacement();    // link placement
      _SwcWorldChange.iType = (INDEX)m_EwltType;  // type
      _pNetwork->ChangeLevel(m_strWorld, m_bStoreWorld, 0);
      return TRUE;
    }
    return FALSE;
  };

procedures:
/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_WORLDLINK);
    SetModelMainTexture(TEXTURE_WORLDLINK);

    // set name
    m_strName.PrintF("World link - %s", (const char *) m_strGroup);

    return;
  }
};
