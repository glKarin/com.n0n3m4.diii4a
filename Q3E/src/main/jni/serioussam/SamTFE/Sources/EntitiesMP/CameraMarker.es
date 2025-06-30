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

224
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Marker";

class CCameraMarker: CMarker
{
name      "Camera Marker";
thumbnail "Thumbnails\\CameraMarker.tbn";


properties:

  1 FLOAT m_fDeltaTime   "Delta time"  'D' = 5.0f,
  2 FLOAT m_fBias        "Bias"        'B' = 0.0f,
  3 FLOAT m_fTension     "Tension"     'E' = 0.0f,
  4 FLOAT m_fContinuity  "Continuity"  'C' = 0.0f,
  5 BOOL  m_bStopMoving  "Stop moving" 'O' = FALSE,
  6 FLOAT m_fFOV         "FOV"         'F' = 90.0f,
  7 BOOL  m_bSkipToNext  "Skip to next" 'S' = FALSE,
  8 COLOR m_colFade      "Fade Color" 'C' = 0,     // camera fading color
  9 CEntityPointer m_penTrigger "Trigger" 'G', // camera triggers when at this marker
 10 CEntityPointer m_penViewTarget "View Target" 'V', // camera will targeting this entity
 11 FLOAT3D m_vPosRatio = FLOAT3D(0,0,0),
 12 FLOAT m_fRatioX "View pos ratio X" 'X' = 0.5f,
 13 FLOAT m_fRatioY "View pos ratio Y" 'Y' = 0.5f,
 14 FLOAT m_fRatioZ "View pos ratio Z" 'Z' = 0.5f,


components:

  1 model   MODEL_MARKER     "Models\\Editor\\CameraMarker.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\CameraMarker.tex"


functions:

  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker( CTFileName &fnmMarkerClass, CTString &strTargetProperty) const
  {
    fnmMarkerClass = CTFILENAME("Classes\\CameraMarker.ecl");
    strTargetProperty = "Target";
    return TRUE;
  }


  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    return( sizeof(CCameraMarker) - sizeof(CMarker) + CMarker::GetUsedMemory());
  }



procedures:

  Main()
  {
    // clamp parameters
    m_fDeltaTime  = ClampDn( m_fDeltaTime, 0.001f);
    m_fBias       = Clamp( m_fBias,       -1.0f, +1.0f);
    m_fTension    = Clamp( m_fTension,    -1.0f, +1.0f);
    m_fContinuity = Clamp( m_fContinuity, -1.0f, +1.0f);
    
    m_vPosRatio=FLOAT3D(m_fRatioX, m_fRatioY, m_fRatioZ);

    // init model
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    if( m_penTarget!=NULL && !IsOfClass( m_penTarget, "Camera Marker")) {
      WarningMessage( "Entity '%s' is not of Camera Marker class!", (const char *) (m_penTarget->GetName()));
      m_penTarget = NULL;
    }

    return;
  }

};

