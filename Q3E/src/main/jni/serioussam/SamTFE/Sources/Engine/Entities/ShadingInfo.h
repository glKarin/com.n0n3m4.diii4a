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

#ifndef SE_INCL_SHADINGINFO_H
#define SE_INCL_SHADINGINFO_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Math/Vector.h>

// Used for caching shading info for models if they don't move
class ENGINE_API CShadingInfo {
public:
  CListNode si_lnInPolygon;       // for linking in the relevant polygon
  CBrushPolygon *si_pbpoPolygon;  // the polygon that entity is above
  CTerrain      *si_ptrTerrain;   // terrain that entity is above
  FLOAT3D si_vNearPoint;          // the relevant point in absolute space
  PIX si_pixShadowU, si_pixShadowV; // the relevant point in the polygon shadow map
  FLOAT si_fUDRatio, si_fLRRatio;   // fraction between pixels
  CEntity *si_penEntity;          // the entity which uses this shading info
};


#endif  /* include-once check. */

