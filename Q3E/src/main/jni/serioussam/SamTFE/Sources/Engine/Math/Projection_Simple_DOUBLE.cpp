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

#include <Engine/Math/Projection_DOUBLE.h>

#include <Engine/Math/AABBox.h>
#include <Engine/Math/Geometry.h>

#include <Engine/Math/Clipping.inl>

/*
 * Default constructor.
 */
CSimpleProjection3D_DOUBLE::CSimpleProjection3D_DOUBLE(void) {
  pr_Prepared = FALSE;
  pr_ObjectStretch = FLOAT3D(1.0f, 1.0f, 1.0f);
}
/*
 * Prepare for projecting.
 */
void CSimpleProjection3D_DOUBLE::Prepare(void)
{
  DOUBLEmatrix3D t3dObjectStretch;   // matrix for object stretch
  DOUBLEmatrix3D t3dObjectRotation;  // matrix for object angles

  // calc. matrices for viewer and object angles and stretch
  t3dObjectRotation ^= pr_ObjectPlacement.pl_OrientationAngle;  // object normally
  pr_ViewerRotationMatrix != pr_ViewerPlacement.pl_OrientationAngle;  // viewer inverse
  t3dObjectStretch.Diagonal(FLOATtoDOUBLE(pr_ObjectStretch));

  // first apply object stretch then object rotation and then viewer rotation
  pr_RotationMatrix = pr_ViewerRotationMatrix*t3dObjectRotation*t3dObjectStretch;

  // calc. offset of object from viewer
  pr_TranslationVector = FLOATtoDOUBLE(pr_ObjectPlacement.pl_PositionVector - pr_ViewerPlacement.pl_PositionVector);
  // rotate offset only by viewer angles
  pr_TranslationVector = pr_TranslationVector*pr_ViewerRotationMatrix;

  // mark as prepared
  pr_Prepared = TRUE;
}

/*
 * Project 3D object point into 3D view space.
 */
void CSimpleProjection3D_DOUBLE::ProjectCoordinate(const DOUBLE3D &v3dObjectPoint,
                                  DOUBLE3D &v3dViewPoint) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // rotate and translate the point
  v3dViewPoint = v3dObjectPoint*pr_RotationMatrix + pr_TranslationVector;
}

/*
 * Project 3D object direction vector into 3D view space.
 */
void CSimpleProjection3D_DOUBLE::ProjectDirection(const DOUBLE3D &v3dObjectPoint,
                                  DOUBLE3D &v3dViewPoint) const
{
  // rotate the direction
  v3dViewPoint = v3dObjectPoint*pr_RotationMatrix;
}

/*
 * Project 3D object placement into 3D view space.
 */
void CSimpleProjection3D_DOUBLE::ProjectPlacement(const CPlacement3D &plObject,
                                           CPlacement3D &plView) const
{
  // rotate and translate the position vector
  plView.pl_PositionVector = DOUBLEtoFLOAT(
    FLOATtoDOUBLE(plObject.pl_PositionVector)*pr_RotationMatrix + pr_TranslationVector);
  // rotate the orientation
  DOUBLEmatrix3D mOrientation;
  mOrientation ^= plObject.pl_OrientationAngle;
  plView.pl_OrientationAngle ^= pr_RotationMatrix*mOrientation;
}

/*
 * Project 3D object mapping into 3D view space.
 */
void CSimpleProjection3D_DOUBLE::ProjectMapping(const CMappingDefinition &mdObject,
                                         const DOUBLEplane3D &plObject,
                                         CMappingDefinition &mdView) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // make mapping vectors for given plane
  CMappingVectors mvObjectDefault;
  mvObjectDefault.FromPlane_DOUBLE(plObject);
  CMappingVectors mvObject;
  mdObject.ToMappingVectors(mvObjectDefault, mvObject);

  // make projected plane and vectors
  DOUBLEplane3D plView;
  Project(plObject, plView);
  CMappingVectors mvViewDefault;
  mvViewDefault.FromPlane_DOUBLE(plView);
  DOUBLE3D vO, vU, vV;
  ProjectCoordinate(FLOATtoDOUBLE(mvObject.mv_vO), vO);
  ProjectDirection(FLOATtoDOUBLE(mvObject.mv_vU), vU);
  ProjectDirection(FLOATtoDOUBLE(mvObject.mv_vV), vV);
  CMappingVectors mvView;
  mvView.mv_vO = DOUBLEtoFLOAT(vO);
  mvView.mv_vU = DOUBLEtoFLOAT(vU);
  mvView.mv_vV = DOUBLEtoFLOAT(vV);

  // create definition back from the vectors
  mdView.FromMappingVectors(mvViewDefault, mvView);
}

/*
 * Project 3D object axis aligned bounding box into 3D view space.
 */
void CSimpleProjection3D_DOUBLE::ProjectAABBox(const DOUBLEaabbox3D &boxObject,
                                             DOUBLEaabbox3D &boxView) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // get center and max axial extent of object box
  const DOUBLE3D vObjectBoxCenter = boxObject.Center();
  const DOUBLE fObjectBoxMaxExtent = boxObject.Size().MaxNorm();
  // project the center to view space
  DOUBLE3D vViewBoxCenter;
  ProjectCoordinate(vObjectBoxCenter, vViewBoxCenter);

  // let the view box have center there and radius of twice max axial extent of object box
  boxView = DOUBLEaabbox3D(vViewBoxCenter, 2*fObjectBoxMaxExtent);

/*  // clear view box
  boxView = DOUBLEaabbox3D();
  // create all vertex points of object box
  DOUBLE3D avObjectBoxVertices[8];
  avObjectBoxVertices[0](1) = boxObject.Min()(1);
  avObjectBoxVertices[0](2) = boxObject.Min()(2);
  avObjectBoxVertices[0](3) = boxObject.Min()(3);

  avObjectBoxVertices[0](1) = boxObject.Min()(1);
  avObjectBoxVertices[0](2) = boxObject.Min()(2);
  avObjectBoxVertices[0](3) = boxObject.Min()(3);

  avObjectBoxVertices[0](1) = boxObject.Max()(1);
  // project all vertex points of object box
  // add all vertex points of object box to view box
*/
}

/*
 * Project 3D object plane into 3D view space.
 */
void CSimpleProjection3D_DOUBLE::Project(const DOUBLEplane3D &p3dObjectPlane,
                                  DOUBLEplane3D &p3dTransformedPlane) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // rotate the plane
  p3dTransformedPlane = p3dObjectPlane*pr_RotationMatrix;
  // renormalize it in case of stretch
  if (pr_ObjectStretch(1)!=1.0f || pr_ObjectStretch(2)!=1.0f || pr_ObjectStretch(3)!=1.0f) {
    FLOAT fLen = ((DOUBLE3D&)p3dTransformedPlane).Length();
    p3dTransformedPlane = DOUBLEplane3D(
      ((DOUBLE3D&)p3dTransformedPlane)/fLen,
      p3dTransformedPlane.Distance()*fLen);
  }
  // translate the plane
  p3dTransformedPlane += pr_TranslationVector;
}
