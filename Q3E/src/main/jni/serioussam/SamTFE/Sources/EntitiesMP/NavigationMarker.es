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

704
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/Common/PathFinding.h"

#define MAX_TARGETS 6
%}

uses "EntitiesMP/Marker";

%{
// info structure
static EntityInfo eiMarker = {
  EIBT_ROCK, 10.0f,
  0.0f, 1.0f, 0.0f,     // source (eyes)
  0.0f, 1.0f, 0.0f,     // target (body)
};

%}

class export CNavigationMarker : CEntity {
name      "NavigationMarker";
thumbnail "Thumbnails\\NavigationMarker.tbn";
features  "HasName", "IsTargetable";

properties:
  1 CTString m_strName          "Name" 'N' = "Marker",
  2 RANGE m_fMarkerRange        "Marker Range" 'M' = 1.0f,  // range around marker (marker doesn't have to be hit directly)

  100 CEntityPointer m_penTarget0  "Target 0" 'T' COLOR(C_dBLUE|0xFF),
  101 CEntityPointer m_penTarget1  "Target 1"     COLOR(C_dBLUE|0xFF),
  102 CEntityPointer m_penTarget2  "Target 2"     COLOR(C_dBLUE|0xFF),
  103 CEntityPointer m_penTarget3  "Target 3"     COLOR(C_dBLUE|0xFF),
  104 CEntityPointer m_penTarget4  "Target 4"     COLOR(C_dBLUE|0xFF),
  105 CEntityPointer m_penTarget5  "Target 5"     COLOR(C_dBLUE|0xFF),

  {
    CPathNode *m_ppnNode;  // for pathfinding algorithm
  }

components:
  1 model   MODEL_MARKER     "Models\\Editor\\NavigationMarker.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\NavigationMarker.tex"

functions:
  void CNavigationMarker(void)
  {
    m_ppnNode = NULL;
  }
  void ~CNavigationMarker(void)
  {
    ASSERT(m_ppnNode == NULL);
  }

  /* Read from stream. */
  void Read_t( CTStream *istr) // throw char *
  {
    CEntity::Read_t(istr);
    m_ppnNode = NULL;
  }
  
  CEntity *GetTarget(void) const { return m_penTarget0; };

  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiMarker;
  };

  // get node used for pathfinding algorithm
  CPathNode *GetPathNode(void)
  {
    if (m_ppnNode==NULL) {
      m_ppnNode = new CPathNode(this);
    }

    return m_ppnNode;
  }

  CEntityPointer &TargetPointer(INDEX i)
  {
    ASSERT(i>=0 && i<MAX_TARGETS);
    return (&m_penTarget0)[i];
  }
  CNavigationMarker &Target(INDEX i)
  {
    return (CNavigationMarker &)*TargetPointer(i);
  }

  // get link with given index or null if no more (for iteration along the graph)
  CNavigationMarker *GetLink(INDEX i)
  {
    for(INDEX iTarget=0; iTarget<MAX_TARGETS; iTarget++) {
      CNavigationMarker *penLink = &Target(iTarget);
      if (penLink==NULL) {
        continue;
      }
      if (iTarget==i) {
        return penLink;
      }
    }
    return NULL;
  }

  const CTString &GetDescription(void) const {
    return m_strName;
  }

  /* Check if entity is moved on a route set up by its targets. */
  BOOL MovesByTargetedRoute(CTString &strTargetProperty) const {
    strTargetProperty = "Target 0";
    return TRUE;
  };
  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker(CTFileName &fnmMarkerClass, CTString &strTargetProperty) const {
    fnmMarkerClass = CTFILENAME("Classes\\NavigationMarker.ecl");
    strTargetProperty = "Target 0";
    return TRUE;
  }
  // this is MARKER
  virtual BOOL IsMarker(void) const {
    return TRUE;
  }

procedures:
  Main() {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    const FLOAT fSize = 0.25f;
    GetModelObject()->StretchModel(FLOAT3D(fSize, fSize, fSize));
    SetModel(MODEL_MARKER);
    ModelChangeNotify();
    SetModelMainTexture(TEXTURE_MARKER);

    // for each non-empty target
    for (INDEX iTarget=0; iTarget<MAX_TARGETS; iTarget++) {
      CEntityPointer &penTarget = TargetPointer(iTarget);
      if (penTarget==NULL) {
        continue;
      }

      // if not valid class
      if (!IsOfClass(penTarget, "NavigationMarker")) {
        // clear it
        penTarget = NULL;
        continue;
      }

      CNavigationMarker &nmOther = (CNavigationMarker &)*penTarget;

      // check if it has back pointer
      BOOL bFound = FALSE;
      for (INDEX iTarget2=0; iTarget2<MAX_TARGETS; iTarget2++) {
        CEntityPointer &penTarget2 = nmOther.TargetPointer(iTarget2);
        if (penTarget2==this) {
          bFound = TRUE;
          break;
        }
      }
      // if none found
      if (!bFound) {
        // auto-set an empty one if possible
        for (INDEX iTarget2=0; iTarget2<MAX_TARGETS; iTarget2++) {
          CEntityPointer &penTarget2 = nmOther.TargetPointer(iTarget2);
          if (penTarget2==NULL) {
            penTarget2 = this;
            break;
          }
        }
      }
    }

    return;
  }
};

