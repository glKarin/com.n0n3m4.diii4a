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

#ifndef SE_INCL_WORLDCOLLISION_H
#define SE_INCL_WORLDCOLLISION_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Matrix.h>
#include <Engine/Math/Placement.h>
#include <Engine/Entities/EntityCollision.h>

/*
 * A class used for clipping a movement.
 */
class CClipMove {
public:
  BOOL cm_bMovingBrush;  // set if moving a brush (some things are reversed then)
  CMovableEntity *cm_penMoving;  // entity that is moving
  CEntity *cm_penA;      // entity A - can be only model
  CEntity *cm_penB;      // entity B - can be either model or brush

  // masks for collision flags of other entities
  ULONG cm_ulTestMask1;
  ULONG cm_ulTestMask2;
  ULONG cm_ulPassMaskA;   // pass - send event to A
  ULONG cm_ulPassMaskB;   // pass - send event to B

  // test if should test with some entity
  inline BOOL MustTest(CEntity *pen) {
    return
      (pen->en_ulCollisionFlags&cm_ulTestMask1)&&
      (pen->en_ulCollisionFlags&cm_ulTestMask2);
  };
  // send pass if needed
  inline BOOL SendPassEvent(CEntity *pen);

  CListHead cm_lhActiveSectors; // brush sectors that are queued for testing

  // placement of entity A
  FLOAT3D cm_vA0; FLOATmatrix3D cm_mA0; // at the start of movement
  FLOAT3D cm_vA1; FLOATmatrix3D cm_mA1; // at the end of movement
  // placement of entity B
  FLOAT3D cm_vB0; FLOATmatrix3D cm_mB0; // at the start of movement
  FLOAT3D cm_vB1; FLOATmatrix3D cm_mB1; // at the end of movement

  CStaticArray<CMovingSphere> *cm_pamsA;  // bounding spheres used by entity A
  CStaticArray<CMovingSphere> *cm_pamsB;  // bounding spheres used by entity B (if not brush)

// helper variables
  FLOATaabbox3D cm_boxMovementPath; // aabbox around entire movement path
  CEntity *cm_penTested;            // entity to be remembered if hit (A or B)
  CBrushPolygon *cm_pbpoTested;     // brush polygon to be remembered if hit
  class CWorld *cm_pwoWorld;        // world that movement is taking place in

  // projections for converting from space of entity A to space of entity B
  FLOAT3D cm_vAToB0; FLOATmatrix3D cm_mAToB0;   // at the start of movement
  FLOAT3D cm_vAToB1; FLOATmatrix3D cm_mAToB1;   // at the end of movement
  // for converting from space of entity B to absolute space
  FLOAT3D cm_vBToAbsolute; FLOATmatrix3D cm_mBToAbsolute;

  FLOATaabbox3D cm_boxMovementPathAbsoluteA; // movement box in absolute space for A entity

  // get start and end positions of an entity in this tick
  inline void GetPositionsOfEntity(
    CEntity *pen, FLOAT3D &v0, FLOATmatrix3D &m0, FLOAT3D &v1, FLOATmatrix3D &m1);

  /* Project spheres of entity A to space of entity B. */
  void ProjectASpheresToB(void);
  /* Find movement box in absolute space for A entity. */
  void FindAbsoluteMovementBoxForA(void);

  /* Clip a moving point to a sphere, update collision data. */
  inline void ClipMovingPointToSphere(const FLOAT3D &vStart, const FLOAT3D &vEnd,
    const FLOAT3D &vSphereCenter, const FLOAT fSphereRadius);
  /* Clip a moving point to a cylinder, update collision data. */
  inline void ClipMovingPointToCylinder(const FLOAT3D &vStart, const FLOAT3D &vEnd,
    const FLOAT3D &vCylinderBottomCenter, const FLOAT3D &vCylinderTopCenter,
    const FLOAT fCylinderRadius);

  /* Clip a moving sphere to a standing sphere, update collision data. */
  void ClipMovingSphereToSphere(const CMovingSphere &msMoving,
    const CMovingSphere &msStanding);
  /* Clip a moving sphere to a brush polygon, update collision data. */
  void ClipMovingSphereToBrushPolygon(
    const CMovingSphere &msMoving, CBrushPolygon *pbpoPolygon);
  /* Clip a moving sphere to a terrain polygon, update collision data. */
  void ClipMovingSphereToTerrainPolygon(
    const CMovingSphere &msMoving, const FLOAT3D &v0, const FLOAT3D &v1, const FLOAT3D &v2);
  /* Clip movement to a brush polygon. */
  void ClipMoveToBrushPolygon(CBrushPolygon *pbpoPolygon);
  /* Clip movement to a terrain polygon. */
  void ClipMoveToTerrainPolygon(const FLOAT3D &v0, const FLOAT3D &v1, const FLOAT3D &v2);

  /* Prepare projections and spheres for movement clipping. */
  void PrepareProjectionsAndSpheres(void);
  /* Clip movement if B is a model. */
  void ClipModelMoveToModel(void);
  /* Clip movement if B is a brush. */
  void ClipBrushMoveToModel(void);

  /* Clip movement to a model entity. */
  void ClipMoveToModel(CEntity *penModel);

  void ClipToNonZoningSector(CBrushSector *pbsc);
  void ClipToZoningSector(CBrushSector *pbsc);
  void ClipToTerrain(CEntity *pen);

  /* Cache near polygons of movable entity. */
  void CacheNearPolygons(void);
  /* Clip movement to brush sectors near the entity. */
  void ClipMoveToBrushes(void);
  /* Clip movement to models near the entity. */
  void ClipMoveToModels(void);

  /* Clip movement to the world. */
  void ClipMoveToWorld(class CWorld *pwoWorld);
public:

// these are filled by clipping algorithm:
  CEntity *cm_penHit;             // entity hit when moving. NULL if nothing was hit
  CBrushPolygon *cm_pbpoHit;      // brush polygon that was hit (NULL if did not hit a brush)
  FLOAT cm_fMovementFraction;     // fraction of movement done before hitting
  FLOATplane3D cm_plClippedPlane; // the plane that was hit (in absolute space)
  FLOAT3D cm_vClippedLine;        // vector describing part of test line that was clipped

  /* Constructor. */
  CClipMove(CMovableEntity *penEntity);
};


#endif  /* include-once check. */

