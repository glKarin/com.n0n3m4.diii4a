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

610
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/PyramidSpaceShip.h"
%}

uses "EntitiesMP/Marker";

class CPyramidSpaceShipMarker: CMarker
{
name      "Pyramid Space Ship Marker";
thumbnail "Thumbnails\\PyramidSpaceShipMarker.tbn";

properties:

  1 FLOAT m_fDeltaTime   "Delta time"  'D' = 5.0f,
  2 FLOAT m_fBias        "Bias"        'B' = 0.0f,
  3 FLOAT m_fTension     "Tension"     'E' = 0.0f,
  4 FLOAT m_fContinuity  "Continuity"  'C' = 0.0f,
  5 BOOL  m_bStopMoving  "Stop moving" 'O' = FALSE,
  6 CEntityPointer m_penTrigger "Trigger" 'G',        // PyramidSpaceShip triggers when at this marker
  7 FLOAT m_fRotSpeed "Rotation speed"  'R' = 0.0f,   // current speed of rotation
  8 CEntityPointer m_penSpaceShip "Space ship" 'S',   // pointer to PyramidSpaceShip, for forcing next path marker

components:

  1 model   MODEL_MARKER     "Models\\Editor\\Axis.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\Vector.tex"


functions:
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if(penTarget==NULL)
    {
      return FALSE;
    }
    if(slPropertyOffset == _offsetof(CPyramidSpaceShipMarker, m_penTarget))
    {
      return( IsDerivedFromClass(penTarget, "Pyramid Space Ship Marker") ||
              IsDerivedFromClass(penTarget, "PyramidSpaceShip") );
    }
    return TRUE;
  }

  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker( CTFileName &fnmMarkerClass, CTString &strTargetProperty) const
  {
    fnmMarkerClass = CTFILENAME("Classes\\PyramidSpaceShipMarker.ecl");
    strTargetProperty = "Target";
    return TRUE;
  }

  /* Handle an event, return false if the event is not handled. */
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    if (ee.ee_slEvent==EVENTCODE_ETrigger)
    {
      if(m_penSpaceShip!=NULL && m_penTarget!=NULL)
      {
        EForcePathMarker eForcePathMarker;
        eForcePathMarker.penForcedPathMarker = m_penTarget;
        m_penSpaceShip->SendEvent(eForcePathMarker);
        return TRUE;
      }
    }
    return FALSE;
  }

procedures:

  Main()
  {
    // clamp parameters
    m_fDeltaTime  = ClampDn( m_fDeltaTime, 0.001f);
    m_fBias       = Clamp( m_fBias,       -1.0f, +1.0f);
    m_fTension    = Clamp( m_fTension,    -1.0f, +1.0f);
    m_fContinuity = Clamp( m_fContinuity, -1.0f, +1.0f);

    // init model
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);
    GetModelObject()->StretchModel(FLOAT3D(4,4,4));
    ModelChangeNotify();

    if( m_penTarget!=NULL && !IsOfClass( m_penTarget, "Pyramid Space Ship Marker")) {
      WarningMessage( "Entity '%s' is not of Pyramid Space Ship Marker class!", (const char *) m_penTarget->GetName());
      m_penTarget = NULL;
    }

    return;
  }

};

