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

#ifndef SE_INCL_ENTITYCOLLISION_H
#define SE_INCL_ENTITYCOLLISION_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Math/Vector.h>
#include <Engine/Math/Matrix.h>
#include <Engine/Math/AABBox.h>
#include <Engine/Templates/StaticArray.h>

/*
 * Bounding sphere used for movement clipping.
 */
class ENGINE_API CMovingSphere {
public:
// implementation:
  FLOAT3D ms_vCenter;    // sphere center in space of moving entity
  FLOAT ms_fR;           // sphere radius

  FLOAT3D ms_vRelativeCenter0; // start point of sphere center in space of standing entity
  FLOAT3D ms_vRelativeCenter1; // end point of sphere center in space of standing entity
  FLOATaabbox3D ms_boxMovement; // the movement path in space of standing entity
public:
// interface:
};

// Used for caching collision info for models
#define CIF_IGNOREHEADING     (1L<<0) // heading rotation can be ignored
#define CIF_IGNOREROTATION    (1L<<1) // any rotation can be ignored
#define CIF_CANSTANDONHANDLE  (1L<<2) // entity can stand on its handle sphere
#define CIF_BRUSH             (1L<<3) // entity is a brush
class ENGINE_API CCollisionInfo {
public:
  CStaticArray<CMovingSphere> ci_absSpheres;  // collision spheres of the model
  FLOAT ci_fMinHeight, ci_fMaxHeight;         // height span of the model
  FLOAT ci_fHandleY;  // y coordinate of handle sphere to stand on
  FLOAT ci_fHandleR;  // radius of handle sphere to stand on
  FLOATaabbox3D ci_boxCurrent;  // current bounding box
  ULONG ci_ulFlags;

  CCollisionInfo(void) {};
  CCollisionInfo(const CCollisionInfo &ciOrg);
  /* Create collision info for a model. */
  void FromModel(CEntity *penModel, INDEX iBox);
  /* Create collision info for a ska model */
  void FromSkaModel(CEntity *penModel, INDEX iBox);
  /* Create collision info for a brush. */
  void FromBrush(CBrush3D *pbrBrush);
  /* Calculate bounding box in absolute space from position. */
  void MakeBoxAtPlacement(const FLOAT3D &vPosition, const FLOATmatrix3D &mRotation,
    FLOATaabbox3D &box);
  // get maximum radius of entity in xz plane (relative to entity handle)
  FLOAT GetMaxFloorRadius(void);

  inline void Clear(void) { ci_absSpheres.Clear(); };
};



#endif  /* include-once check. */

