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

#include <Engine/Math/Clipping.inl>

/////////////////////////////////////////////////////////////////////
//  CSimpleProjection3D

/*
 * Prepare for projecting.
 */
void CSimpleProjection3D::Prepare(void)
{
  FLOATmatrix3D t3dObjectStretch;   // matrix for object stretch
  FLOATmatrix3D t3dObjectRotation;  // matrix for object angles

  // calc. matrices for viewer and object angles and stretch
  MakeRotationMatrix(t3dObjectRotation, pr_ObjectPlacement.pl_OrientationAngle);  // object normally
  MakeInverseRotationMatrix(pr_ViewerRotationMatrix, pr_ViewerPlacement.pl_OrientationAngle);  // viewer inverse
  t3dObjectStretch.Diagonal(pr_ObjectStretch);

  // if the object is face-forward
  if (pr_bFaceForward) {
    // apply object stretch only
    pr_RotationMatrix = t3dObjectStretch;
  } else {
    // first apply object stretch then object rotation and then viewer rotation
    pr_RotationMatrix = pr_ViewerRotationMatrix*t3dObjectRotation*t3dObjectStretch;
  }

  // calc. offset of object from viewer
  pr_TranslationVector = pr_ObjectPlacement.pl_PositionVector - pr_ViewerPlacement.pl_PositionVector;
  // rotate offset only by viewer angles
  pr_TranslationVector = pr_TranslationVector*pr_ViewerRotationMatrix;
  // transform handle from object space to viewer space and add it to the offset
  pr_TranslationVector -= pr_vObjectHandle*pr_RotationMatrix;

  // mark as prepared
  pr_Prepared = TRUE;
}

/*
 * Project 3D object point into 3D view space, before clipping.
 */
void CSimpleProjection3D::PreClip(const FLOAT3D &v3dObjectPoint,
                                       FLOAT3D &v3dTransformedPoint) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // rotate and translate the point
  v3dTransformedPoint = v3dObjectPoint*pr_RotationMatrix + pr_TranslationVector;
}

/*
 * Project 3D object point into 3D view space, after clipping.
 */
void CSimpleProjection3D::PostClip( const FLOAT3D &v3dTransformedPoint,
                                    FLOAT3D &v3dViewPoint) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);
  v3dViewPoint = v3dTransformedPoint;
}


void CSimpleProjection3D::PostClip( const FLOAT3D &v3dTransformedPoint, FLOAT fTransformedR,
                                    FLOAT3D &v3dViewPoint, FLOAT &fViewR) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);
  v3dViewPoint = v3dTransformedPoint;
  fViewR = fTransformedR;
}


/* Test if a sphere in view space is inside view frustum. */
INDEX CSimpleProjection3D::TestSphereToFrustum(const FLOAT3D &vViewPoint, FLOAT fRadius) const
{
  ASSERT(pr_Prepared);
  ASSERTALWAYS("Function not supported");
  return 1;
}

/* Test if an oriented box in view space is inside view frustum. */
INDEX CSimpleProjection3D::TestBoxToFrustum(const FLOATobbox3D &boxView) const
{
  ASSERT(pr_Prepared);
  ASSERTALWAYS("Function not supported");
  return 1;
}

/*
 * Project 3D object point into 3D view space.
 */
void CSimpleProjection3D::ProjectCoordinate(const FLOAT3D &v3dObjectPoint,
                                  FLOAT3D &v3dViewPoint) const
{
  // rotate and translate the point
  v3dViewPoint = v3dObjectPoint*pr_RotationMatrix + pr_TranslationVector;
}

/*
 * Get a distance of object point from the viewer.
 */
FLOAT CSimpleProjection3D::GetDistance(const FLOAT3D &v3dObjectPoint) const
{
  // get just the z coordinate of the point in viewer space
  return
    v3dObjectPoint(1)*pr_RotationMatrix(3,1)+
    v3dObjectPoint(2)*pr_RotationMatrix(3,2)+
    v3dObjectPoint(3)*pr_RotationMatrix(3,3)+
    pr_TranslationVector(3);
}

/*
 * Project 3D object direction vector into 3D view space.
 */
void CSimpleProjection3D::ProjectDirection(const FLOAT3D &v3dObjectPoint,
                                  FLOAT3D &v3dViewPoint) const
{
  // rotate the direction
  v3dViewPoint = v3dObjectPoint*pr_RotationMatrix;
}

/*
 * Project 3D object placement into 3D view space.
 */
void CSimpleProjection3D::ProjectPlacement(const CPlacement3D &plObject,
                                           CPlacement3D &plView) const
{
  // rotate and translate the position vector
  plView.pl_PositionVector = plObject.pl_PositionVector*pr_RotationMatrix
    + pr_TranslationVector;
  // rotate the orientation
  FLOATmatrix3D mOrientation;
  MakeRotationMatrix(mOrientation, plObject.pl_OrientationAngle);
  DecomposeRotationMatrix(plView.pl_OrientationAngle, pr_RotationMatrix*mOrientation);
}

/*
 * Project 3D object placement into 3D view space.
 */
void CSimpleProjection3D::ProjectPlacementSmooth(const CPlacement3D &plObject,
                                           CPlacement3D &plView) const
{
  // rotate and translate the position vector
  plView.pl_PositionVector = plObject.pl_PositionVector*pr_RotationMatrix
    + pr_TranslationVector;
  // rotate the orientation
  FLOATmatrix3D mOrientation;
  MakeRotationMatrixFast(mOrientation, plObject.pl_OrientationAngle);
  DecomposeRotationMatrixNoSnap(plView.pl_OrientationAngle, pr_RotationMatrix*mOrientation);
}

/*
 * Project 3D object axis aligned bounding box into 3D view space.
 */
void CSimpleProjection3D::ProjectAABBox(const FLOATaabbox3D &boxObject,
                                             FLOATaabbox3D &boxView) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // get center and max axial extent of object box
  const FLOAT3D vObjectBoxCenter = boxObject.Center();
  const FLOAT fObjectBoxMaxExtent = boxObject.Size().MaxNorm();
  // project the center to view space
  FLOAT3D vViewBoxCenter;
  ProjectCoordinate(vObjectBoxCenter, vViewBoxCenter);

  // let the view box have center there and radius of twice max axial extent of object box
  boxView = FLOATaabbox3D(vViewBoxCenter, 2*fObjectBoxMaxExtent);

/*  // clear view box
  boxView = FLOATaabbox3D();
  // create all vertex points of object box
  FLOAT3D avObjectBoxVertices[8];
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
void CSimpleProjection3D::Project(const FLOATplane3D &p3dObjectPlane,
                                FLOATplane3D &p3dTransformedPlane) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // rotate and translate the plane
  p3dTransformedPlane = p3dObjectPlane*pr_RotationMatrix + pr_TranslationVector;
}

/* Calculate plane gradient for a plane in 3D view space. */
void CSimpleProjection3D::MakeOoKGradient(const FLOATplane3D &plViewerPlane, CPlanarGradients &pgOoK) const
{
  ASSERTALWAYS("Function not supported");
}
/*
 * Clip a line.
 */
ULONG CSimpleProjection3D::ClipLine(FLOAT3D &v3dPoint0, FLOAT3D &v3dPoint1) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // line remains unclipped
  return LCFVERTEX0(LCF_UNCLIPPED) | LCFVERTEX1(LCF_UNCLIPPED);
}

/*
 * Get placement for a ray through a projected point.
 */
void CSimpleProjection3D::RayThroughPoint(const FLOAT3D &v3dViewPoint,
                                               CPlacement3D &plRay) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  ASSERTALWAYS("Not implemented!");
}

/*
 * Check if an object-space plane is visible.
 */
BOOL CSimpleProjection3D::IsObjectPlaneVisible(const FLOATplane3D &p3dObjectPlane) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  ASSERTALWAYS("Not implemented!");
  return TRUE;
}

/*
 * Check if a viewer-space plane is visible.
 */
BOOL CSimpleProjection3D::IsViewerPlaneVisible(const FLOATplane3D &p3dViewerPlane) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);
  // the object plane is visible is it is not heading away from the view plane
  return p3dViewerPlane(3)>0.0f;
}

/*
 * Calculate a mip-factor for a given object.
 */
// by its distance from viewer
FLOAT CSimpleProjection3D::MipFactor(FLOAT fDistance) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  ASSERTALWAYS("Not implemented!");
  return 0.0f;
}
// general mip-factor for target object
FLOAT CSimpleProjection3D::MipFactor(void) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  ASSERTALWAYS("Not implemented!");
  return 0.0f;
}

