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

#include <Engine/Math/TextureMapping.h>
#include <Engine/Math/OBBox.h>

#include <Engine/Math/Geometry.inl>
#include <Engine/Math/Clipping.inl>

/////////////////////////////////////////////////////////////////////
//  CParallelProjection3D

/*
 * Prepare for projecting.
 */
void CParallelProjection3D::Prepare(void)
{
  FLOATmatrix3D t3dObjectStretch;   // matrix for object stretch
  FLOATmatrix3D t3dObjectRotation;  // matrix for object angles

  // calc. matrices for viewer and object angles and stretch
  MakeRotationMatrix(t3dObjectRotation, pr_ObjectPlacement.pl_OrientationAngle);  // object normally
  MakeInverseRotationMatrix(pr_ViewerRotationMatrix, pr_ViewerPlacement.pl_OrientationAngle);  // viewer inverse
  t3dObjectStretch.Diagonal(pr_ObjectStretch);
  pr_vViewerPosition = pr_ViewerPlacement.pl_PositionVector;
  BOOL bXInverted = pr_ObjectStretch(1)<0;
  BOOL bYInverted = pr_ObjectStretch(2)<0;
  BOOL bZInverted = pr_ObjectStretch(3)<0;

  pr_bInverted = (bXInverted != bYInverted) != bZInverted;

  // if the projection is mirrored
  if (pr_bMirror) {
    // reflect viewer
    ReflectPositionVectorByPlane(pr_plMirror, pr_vViewerPosition);
    ReflectRotationMatrixByPlane_rows(pr_plMirror, pr_ViewerRotationMatrix);
    // invert inversion
    pr_bInverted = !pr_bInverted;
  }

  // calculate screen center
  pr_ScreenCenter = pr_ScreenBBox.Center();

  // if the object is face-forward
  if (pr_bFaceForward) {
    // apply object stretch only
    pr_RotationMatrix = t3dObjectStretch;
  } else {
    // first apply object stretch then object rotation and then viewer rotation
    pr_mDirectionRotation = pr_ViewerRotationMatrix*t3dObjectRotation;
    pr_RotationMatrix = pr_mDirectionRotation*t3dObjectStretch;
  }

  // calc. offset of object from viewer
  pr_TranslationVector = pr_ObjectPlacement.pl_PositionVector - pr_vViewerPosition;
  // rotate offset only by viewer angles
  pr_TranslationVector = pr_TranslationVector*pr_ViewerRotationMatrix;
  // transform handle from object space to viewer space and add it to the offset
  pr_TranslationVector -= pr_vObjectHandle*pr_RotationMatrix;

  // calculate constant value used for calculating z-buffer k-value from vertex's z coordinate
  pr_fDepthBufferFactor = -pr_NearClipDistance;
  pr_fDepthBufferMul = (pr_fDepthBufferFar-pr_fDepthBufferNear);
  pr_fDepthBufferAdd = pr_fDepthBufferNear;

  // make clip planes
  MakeClipPlane(FLOAT3D(+pr_vZoomFactors(1),0,+pr_vStepFactors(1)), pr_ScreenBBox.Min()(1)-pr_ScreenCenter(1), pr_plClipL);
  MakeClipPlane(FLOAT3D(-pr_vZoomFactors(1),0,-pr_vStepFactors(1)), pr_ScreenCenter(1)-pr_ScreenBBox.Max()(1), pr_plClipR);
  MakeClipPlane(FLOAT3D(0,-pr_vZoomFactors(2),-pr_vStepFactors(2)), pr_ScreenBBox.Min()(2)-pr_ScreenCenter(2), pr_plClipU);
  MakeClipPlane(FLOAT3D(0,+pr_vZoomFactors(2),+pr_vStepFactors(2)), pr_ScreenCenter(2)-pr_ScreenBBox.Max()(2), pr_plClipD);

  // find vector in direction of viewing
  pr_vViewDirection = FLOAT3D(
    pr_vStepFactors(1)/pr_vZoomFactors(1),
    pr_vStepFactors(2)/pr_vZoomFactors(2),
    -1.0f);

  // mark as prepared
  pr_Prepared = TRUE;
}

/*
 * Project 3D object point into 3D view space, before clipping.
 */
void CParallelProjection3D::PreClip(const FLOAT3D &v3dObjectPoint,
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
void CParallelProjection3D::PostClip( const FLOAT3D &v3dTransformedPoint,
                                      FLOAT3D &v3dViewPoint) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);
  // multiply X and Y coordinates with zoom factor and add the center of screen
  v3dViewPoint(1) = pr_ScreenCenter(1)
                  + v3dTransformedPoint(1) *pr_vZoomFactors(1)
                  + v3dTransformedPoint(3) *pr_vStepFactors(1);
  v3dViewPoint(2) = pr_ScreenCenter(2)
                  - v3dTransformedPoint(2) *pr_vZoomFactors(2)
                  - v3dTransformedPoint(3) *pr_vStepFactors(2);
}


void CParallelProjection3D::PostClip( const FLOAT3D &v3dTransformedPoint, FLOAT fTransformedR,
                                      FLOAT3D &v3dViewPoint, FLOAT &fViewR) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);
  // multiply X and Y coordinates with zoom factor and add the center of screen
  v3dViewPoint(1) = pr_ScreenCenter(1)
                  + v3dTransformedPoint(1) *pr_vZoomFactors(1)
                  + v3dTransformedPoint(3) *pr_vStepFactors(1);
  v3dViewPoint(2) = pr_ScreenCenter(2)
                  - v3dTransformedPoint(2) *pr_vZoomFactors(2)
                  - v3dTransformedPoint(3) *pr_vStepFactors(2);
  fViewR = fTransformedR *pr_vZoomFactors(1);
}

/* Test if a sphere in view space is inside view frustum. */
INDEX CParallelProjection3D::TestSphereToFrustum(const FLOAT3D &vViewPoint, FLOAT fRadius) const
{
  ASSERT(pr_Prepared);
  const FLOAT fX = vViewPoint(1);
  const FLOAT fY = vViewPoint(2);
  const FLOAT fZ = vViewPoint(3);
  INDEX iPass = 1;

  // check to near
  if( fZ-fRadius>-pr_NearClipDistance) {
    return -1;
  } else if( fZ+fRadius>-pr_NearClipDistance) {
    iPass = 0;
  }
  // check to far
  if( pr_FarClipDistance>0) {
    if( fZ+fRadius<-pr_FarClipDistance) {
      return -1;
    } else if( fZ-fRadius<-pr_FarClipDistance) {
      iPass = 0;
    }
  }
  // check to left
  FLOAT fL = fX*pr_plClipL(1) + fZ*pr_plClipL(3) - pr_plClipL.Distance();
  if( fL<-fRadius) {
    return -1;
  } else if( fL<fRadius) {
    iPass = 0;
  }
  // check to right
  FLOAT fR = fX*pr_plClipR(1) + fZ*pr_plClipR(3) - pr_plClipR.Distance();
  if( fR<-fRadius) {
    return -1;
  } else if( fR<fRadius) {
    iPass = 0;
  }
  // check to up
  FLOAT fU = fY*pr_plClipU(2) + fZ*pr_plClipU(3) - pr_plClipU.Distance();
  if( fU<-fRadius) {
    return -1;
  } else if( fU<fRadius) {
    iPass = 0;
  }
  // check to down
  FLOAT fD = fY*pr_plClipD(2) + fZ*pr_plClipD(3) - pr_plClipD.Distance();
  if( fD<-fRadius) {
    return -1;
  } else if( fD<fRadius) {
    iPass = 0;
  }
  // all done
  return iPass;
}

/* Test if an oriented box in view space is inside view frustum. */
INDEX CParallelProjection3D::TestBoxToFrustum(const FLOATobbox3D &box) const
{
  ASSERT(pr_Prepared);
  INDEX iPass = 1;
  INDEX iTest;

  // check to near
  iTest = (INDEX) box.TestAgainstPlane(FLOATplane3D(FLOAT3D(0,0,-1), pr_NearClipDistance));
  if( iTest<0) {
    return -1;
  } else if( iTest==0) {
    iPass = 0;
  }
  // check to far
  if( pr_FarClipDistance>0) {
    iTest = (INDEX) box.TestAgainstPlane(FLOATplane3D(FLOAT3D(0,0,1), -pr_FarClipDistance));
    if( iTest<0) {
      return -1;
    } else if( iTest==0) {
      iPass = 0;
    }
  }
  // check to left
  iTest = (INDEX) box.TestAgainstPlane(pr_plClipL);
  if( iTest<0) {
    return -1;
  } else if( iTest==0) {
    iPass = 0;
  }
  // check to right
  iTest = (INDEX) box.TestAgainstPlane(pr_plClipR);
  if( iTest<0) {
    return -1;
  } else if( iTest==0) {
    iPass = 0;
  }
  // check to up
  iTest = (INDEX) box.TestAgainstPlane(pr_plClipU);
  if( iTest<0) {
    return -1;
  } else if( iTest==0) {
    iPass = 0;
  }
  // check to down
  iTest = (INDEX) box.TestAgainstPlane(pr_plClipD);
  if( iTest<0) {
    return -1;
  } else if( iTest==0) {
    iPass = 0;
  }
  // all done
  return iPass;
}


/*
 * Project 3D object point into 3D view space.
 */
void CParallelProjection3D::ProjectCoordinate(const FLOAT3D &v3dObjectPoint,
                                  FLOAT3D &v3dViewPoint) const
{
  // rotate and translate the point
  v3dViewPoint = v3dObjectPoint*pr_RotationMatrix + pr_TranslationVector;
  // multiply X and Y coordinates with zoom factor and add the center of screen
  v3dViewPoint(1) =
    pr_ScreenCenter(1)
    +v3dViewPoint(1)*pr_vZoomFactors(1)
    +v3dViewPoint(3)*pr_vStepFactors(1);
  v3dViewPoint(2) =
    pr_ScreenCenter(2)
    +v3dViewPoint(2)*pr_vZoomFactors(2)
    +v3dViewPoint(3)*pr_vStepFactors(2);
}

/*
 * Get a distance of object point from the viewer.
 */
FLOAT CParallelProjection3D::GetDistance(const FLOAT3D &v3dObjectPoint) const
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
void CParallelProjection3D::ProjectDirection(const FLOAT3D &v3dObjectPoint,
                                  FLOAT3D &v3dViewPoint) const
{
  // rotate the direction
  v3dViewPoint = v3dObjectPoint*pr_RotationMatrix;
}

/*
 * Project 3D object axis aligned bounding box into 3D view space.
 */
void CParallelProjection3D::ProjectAABBox(const FLOATaabbox3D &boxObject,
                                             FLOATaabbox3D &boxView) const
{
  ASSERTALWAYS( "This is not yet implemented");
}

/*
 * Project 3D object plane into 3D view space.
 */
void CParallelProjection3D::Project(const FLOATplane3D &p3dObjectPlane,
                                FLOATplane3D &p3dTransformedPlane) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // rotate and translate the plane
  p3dTransformedPlane = p3dObjectPlane*pr_RotationMatrix + pr_TranslationVector;
}

/* Calculate plane gradient for a plane in 3D view space. */
void CParallelProjection3D::MakeOoKGradient(const FLOATplane3D &plViewerPlane, CPlanarGradients &pgOoK) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // ####!!!! use viewer plane or object plane?

  // create k gradients from the plane equation
  FLOAT xn = plViewerPlane(1);
  FLOAT yn = plViewerPlane(2);
  FLOAT zn = plViewerPlane(3);
  FLOAT d = plViewerPlane.Distance();
  FLOAT ci = pr_ScreenCenter(1);
  FLOAT cj = pr_ScreenCenter(2);
  FLOAT fx = pr_vZoomFactors(1);
  FLOAT fy = pr_vZoomFactors(2);
  FLOAT sx = pr_vStepFactors(1);
  FLOAT sy = pr_vStepFactors(2);

  FLOAT Div = zn-sx*xn-sy*yn;

  FLOAT dkodi = xn/(fx*Div);
  FLOAT dkodj = yn/(fy*Div);
  FLOAT k00  = d/Div-ci*dkodi-cj*dkodj;

  // NOTE: here, k is really used instead ook
  pgOoK.pg_f00      = k00;
  pgOoK.pg_fDOverDI = dkodi;
  pgOoK.pg_fDOverDJ = dkodj;
}
/*
 * Clip a line.
 */
ULONG CParallelProjection3D::ClipLine(FLOAT3D &v3dPoint0, FLOAT3D &v3dPoint1) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // front clip plane is exactly the viewplane
  //const FLOATplane3D plFrontClip(FLOAT3D(0.0f,0.0f,-1.0f), 0.0f);

  ULONG ulCode0 = LCFVERTEX0(LCF_UNCLIPPED);
  ULONG ulCode1 = LCFVERTEX1(LCF_UNCLIPPED);

  // clip the line by each plane at the time, skip if some removes entire line
  if (ClipLineByNearPlane(v3dPoint0, v3dPoint1, 0.0f,  ulCode0, ulCode1, LCF_NEAR)
  // if something remains
    ) {
    // return the clip code for both vertices
    return ulCode0 | ulCode1;
  // if some of the planes removed entire line
  } else {
    // return the code that tells that entire line is removed
    return LCF_EDGEREMOVED;
  }
}

/*
 * Get placement for a ray through a projected point.
 */
void CParallelProjection3D::RayThroughPoint(const FLOAT3D &v3dViewPoint,
                                               CPlacement3D &plRay) const
{
  ASSERTALWAYS("Function not supported");
}

/*
 * Check if an object-space plane is visible.
 */
BOOL CParallelProjection3D::IsObjectPlaneVisible(const FLOATplane3D &p3dObjectPlane) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // the object plane is visible if it is not heading away from the view direction
  return (p3dObjectPlane*pr_mDirectionRotation)%pr_vViewDirection<0.01f;
}

/*
 * Check if a viewer-space plane is visible.
 */
BOOL CParallelProjection3D::IsViewerPlaneVisible(const FLOATplane3D &p3dViewerPlane) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // the object plane is visible if it is not heading away from the view direction
  return p3dViewerPlane%pr_vViewDirection<0.01f;
}

/*
 * Calculate a mip-factor for a given object.
 */
// by its distance from viewer
FLOAT CParallelProjection3D::MipFactor(FLOAT fDistance) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  /* calculated using following formula:
  k = log2(1024*z/xratio);
  */
  return Log2(1024.0f/pr_vZoomFactors(1));
}
// general mip-factor for target object
FLOAT CParallelProjection3D::MipFactor(void) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  /* calculated using following formula:
  k = log2(1024*z/xratio);
  */
  return Log2(1024.0f/pr_vZoomFactors(1));
}
