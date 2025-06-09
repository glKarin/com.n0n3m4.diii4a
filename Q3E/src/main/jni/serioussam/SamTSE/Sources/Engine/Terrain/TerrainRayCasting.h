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

#ifndef SE_INCL_TERRAIN_RAY_CASTING_H
#define SE_INCL_TERRAIN_RAY_CASTING_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

FLOAT FASTMATH TestRayCastHit(CTerrain *ptrTerrain, const FLOATmatrix3D &mRotation, const FLOAT3D &vPosition, 
                     const FLOAT3D &vOrigin, const FLOAT3D &vTarget,const FLOAT fOldDistance, 
                     const BOOL bHitInvisibleTris);

FLOAT FASTMATH TestRayCastHit(CTerrain *ptrTerrain, const FLOATmatrix3D &mRotation, const FLOAT3D &vPosition, 
                     const FLOAT3D &vOrigin, const FLOAT3D &vTarget,const FLOAT fOldDistance, 
                     const BOOL bHitInvisibleTris, FLOATplane3D &plHitPlane, FLOAT3D &vHitPoint);
#endif
