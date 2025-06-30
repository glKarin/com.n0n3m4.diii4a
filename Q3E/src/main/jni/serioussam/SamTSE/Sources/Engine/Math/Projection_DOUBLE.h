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

#ifndef SE_INCL_PROJECTION_DOUBLE_H
#define SE_INCL_PROJECTION_DOUBLE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Math/Vector.h>
#include <Engine/Math/Matrix.h>
#include <Engine/Math/Placement.h>
#include <Engine/Math/TextureMapping.h>

/*
 * Geometric projection of one 3D space onto another 3D space
 */
class ENGINE_API CSimpleProjection3D_DOUBLE {
public:
  // factors set by user
  CPlacement3D pr_ObjectPlacement;  // placement of the projected object
  CPlacement3D pr_ViewerPlacement;  // placement of the viewer
  FLOAT3D pr_ObjectStretch;         // stretching coeficients for target object space

  // internal variables
  BOOL pr_Prepared;                  // set if all precalculated variables are prepared
  DOUBLEmatrix3D pr_RotationMatrix;  // matrix for rotating when projecting
  DOUBLEmatrix3D pr_ViewerRotationMatrix;  // viewer part of rotation matrix
  DOUBLE3D pr_TranslationVector;     // vector for translating when projecting
public:
  // construction/destruction
  /* Default constructor. */
  CSimpleProjection3D_DOUBLE(void);

  // member referencing
  /* Reference viewer placement. */
  inline CPlacement3D &ViewerPlacementL(void) {
    IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
    return pr_ViewerPlacement;
  }
  inline const CPlacement3D &ViewerPlacementR(void) const {
    return pr_ViewerPlacement;
  }

  /* Reference object placement. */
  inline CPlacement3D &ObjectPlacementL(void) {
    IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
    return pr_ObjectPlacement;
  }
  inline const CPlacement3D &ObjectPlacementR(void) const {
    return pr_ObjectPlacement;
  }
  /* Reference target object stretching. */
  inline FLOAT3D &ObjectStretchL(void) {
    IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
    return pr_ObjectStretch;
  }
  inline const FLOAT3D &ObjectStretchR(void) const {
    return pr_ObjectStretch;
  }

  /* Prepare for projecting. */
  void Prepare(void);

  /* Project 3D object point into 3D view space. */
  void ProjectCoordinate(const DOUBLE3D &v3dObjectPoint, DOUBLE3D &v3dViewPoint) const;
  /* Project 3D object direction vector into 3D view space. */
  void ProjectDirection(const DOUBLE3D &v3dObjectPoint, DOUBLE3D &v3dViewPoint) const;
  /* Project 3D object placement into 3D view space. */
  void ProjectPlacement(const CPlacement3D &plObject, CPlacement3D &plView) const;
  /* Project 3D object mapping into 3D view space. */
  void ProjectMapping(const CMappingDefinition &mdObject, const DOUBLEplane3D &plObject,
    CMappingDefinition &mdView) const;
  /* Project 3D object axis aligned bounding box into 3D view space. */
  void ProjectAABBox(const DOUBLEaabbox3D &boxObject, DOUBLEaabbox3D &boxView) const;
  /* Project 3D object plane into 3D view space. */
  void Project(const DOUBLEplane3D &p3dObjectPlane, DOUBLEplane3D &v3dTransformedPlane) const;
};


#endif  /* include-once check. */

