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

#include "Engine/StdH.h"

#include <Engine/Math/Projection.h>

/////////////////////////////////////////////////////////////////////
//  CProjection3D
/////////////////////////////////////////////////////////////////////
// Construction / destruction

/*
 * Default constructor.
 */
CProjection3D::CProjection3D(void) {
  pr_Prepared = FALSE;
  pr_ObjectStretch = FLOAT3D(1.0f, 1.0f, 1.0f);
  pr_bFaceForward = FALSE;
  pr_bHalfFaceForward = FALSE;
  pr_vObjectHandle = FLOAT3D(0.0f, 0.0f, 0.0f);
  pr_fDepthBufferNear = 0.0f;
  pr_fDepthBufferFar  = 1.0f;
  pr_NearClipDistance = 0.25f;
  pr_FarClipDistance = -9999.0f;  // never used by default
  pr_bMirror = FALSE;
  pr_bWarp = FALSE;
  pr_fViewStretch = 1.0f;
}

CPlacement3D _plOrigin(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
