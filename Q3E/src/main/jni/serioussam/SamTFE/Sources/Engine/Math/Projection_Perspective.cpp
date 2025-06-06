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
//  CPerspectiveProjection3D

// constructor
CPerspectiveProjection3D::CPerspectiveProjection3D(void)
{
  ppr_fMetersPerPixel = -1.0f;  // never used by default
  ppr_fViewerDistance = -1.0f;
}

/*
 * Prepare for projecting.
 */
void CPerspectiveProjection3D::Prepare(void)
{
  FLOATmatrix3D t3dObjectStretch;   // matrix for object stretch
  FLOATmatrix3D t3dObjectRotation;  // matrix for object angles

  // calc. matrices for viewer and object angles and stretch
  MakeRotationMatrixFast(
    t3dObjectRotation, pr_ObjectPlacement.pl_OrientationAngle);
  MakeInverseRotationMatrixFast(
    pr_ViewerRotationMatrix, pr_ViewerPlacement.pl_OrientationAngle);
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
    // get mirror plane in view space
    pr_plMirrorView = pr_plMirror;
    pr_plMirrorView -= pr_vViewerPosition;
    pr_plMirrorView *= pr_ViewerRotationMatrix;
    // invert inversion
    pr_bInverted = !pr_bInverted;
  } else if (pr_bWarp) {
    // get mirror plane in view space
    pr_plMirrorView = pr_plMirror;
  }

  // if the object is face-forward
  if (pr_bFaceForward) {
    // if it turns only heading
    if (pr_bHalfFaceForward) {
      // get the y-axis vector of object rotation
      FLOAT3D vY(t3dObjectRotation(1,2), t3dObjectRotation(2,2), t3dObjectRotation(3,2));
      // find z axis of viewer
      FLOAT3D vViewerZ(
        pr_ViewerRotationMatrix(3,1),
        pr_ViewerRotationMatrix(3,2),
        pr_ViewerRotationMatrix(3,3));
      // calculate x and z axis vectors to make object head towards viewer
      FLOAT3D vX = (-vViewerZ)*vY;
      vX.Normalize();
      FLOAT3D vZ = vY*vX;
      // compose the rotation matrix back from those angles
      t3dObjectRotation(1,1) = vX(1); t3dObjectRotation(1,2) = vY(1); t3dObjectRotation(1,3) = vZ(1);
      t3dObjectRotation(2,1) = vX(2); t3dObjectRotation(2,2) = vY(2); t3dObjectRotation(2,3) = vZ(2);
      t3dObjectRotation(3,1) = vX(3); t3dObjectRotation(3,2) = vY(3); t3dObjectRotation(3,3) = vZ(3);

      // first apply object stretch then object rotation and then viewer rotation
      pr_mDirectionRotation = pr_ViewerRotationMatrix*t3dObjectRotation;
      pr_RotationMatrix = pr_mDirectionRotation*t3dObjectStretch;
    // if it is fully face forward
    } else {
      // apply object stretch and banking only
      FLOATmatrix3D mBanking;
      MakeRotationMatrixFast(
        mBanking, ANGLE3D(0,0, pr_ObjectPlacement.pl_OrientationAngle(3)));
      pr_mDirectionRotation = mBanking;
      pr_RotationMatrix = mBanking*t3dObjectStretch;
    }
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

  FLOAT2D vMin, vMax;
  // if using a shadow projection
  if (ppr_fMetersPerPixel>0) {
    // caclulate factors
    FLOAT fFactor = ppr_fViewerDistance/ppr_fMetersPerPixel;
    ppr_PerspectiveRatios(1) = -fFactor;
    ppr_PerspectiveRatios(2) = -fFactor;
    pr_ScreenCenter = -pr_ScreenBBox.Min();

    vMin = pr_ScreenBBox.Min();
    vMax = pr_ScreenBBox.Max();
  // if using normal projection
  } else if (ppr_boxSubScreen.IsEmpty()) {
    // calculate perspective constants
    FLOAT2D v2dScreenSize = pr_ScreenBBox.Size();
    pr_ScreenCenter = pr_ScreenBBox.Center();
    /* calculate FOVHeight from FOVWidth by formula:
       halfanglej = atan( tan(halfanglei)*jsize*aspect/isize ) */
    ANGLE aHalfI = ppr_FOVWidth/2;
    ANGLE aHalfJ = ATan(TanFast(aHalfI)*v2dScreenSize(2)*pr_AspectRatio/v2dScreenSize(1));

    /* calc. perspective ratios by formulae:
       xratio = isize/(2*tan(anglei/2))
       yratio = jsize/(2*tan(anglej/2))
      sign is negative since viewer is looking down the -z axis
    */
    ppr_PerspectiveRatios(1) = -v2dScreenSize(1)/(2.0f*TanFast(aHalfI))*pr_fViewStretch;
    ppr_PerspectiveRatios(2) = -v2dScreenSize(2)/(2.0f*TanFast(aHalfJ))*pr_fViewStretch;

    vMin = pr_ScreenBBox.Min()-pr_ScreenCenter;
    vMax = pr_ScreenBBox.Max()-pr_ScreenCenter;
  // if using sub-drawport projection
  } else {
    // calculate perspective constants
    FLOAT2D v2dScreenSize = pr_ScreenBBox.Size();
    pr_ScreenCenter = pr_ScreenBBox.Center();
    /* calculate FOVHeight from FOVWidth by formula:
       halfanglej = atan( tan(halfanglei)*jsize*aspect/isize ) */
    ANGLE aHalfI = ppr_FOVWidth/2;
    ANGLE aHalfJ = ATan(TanFast(aHalfI)*v2dScreenSize(2)*pr_AspectRatio/v2dScreenSize(1));

    /* calc. perspective ratios by formulae:
       xratio = isize/(2*tan(anglei/2))
       yratio = jsize/(2*tan(anglej/2))
      sign is negative since viewer is looking down the -z axis
    */
    ppr_PerspectiveRatios(1) = -v2dScreenSize(1)/(2.0f*TanFast(aHalfI))*pr_fViewStretch;
    ppr_PerspectiveRatios(2) = -v2dScreenSize(2)/(2.0f*TanFast(aHalfJ))*pr_fViewStretch;

    vMin = ppr_boxSubScreen.Min()-pr_ScreenCenter;
    vMax = ppr_boxSubScreen.Max()-pr_ScreenCenter;

    pr_ScreenCenter -= ppr_boxSubScreen.Min();
  }
  // find factors for left, right, up and down clipping

  FLOAT fMinI = vMin(1); FLOAT fMinJ = vMin(2);
  FLOAT fMaxI = vMax(1); FLOAT fMaxJ = vMax(2);
  FLOAT fRatioX = ppr_PerspectiveRatios(1);
  FLOAT fRatioY = ppr_PerspectiveRatios(2);

#define MySgn(x) ((x)>=0?1:-1)

  FLOAT fDZ = -1.0f;
  FLOAT fDXL = fDZ*fMinI/fRatioX;
  FLOAT fDXR = fDZ*fMaxI/fRatioX;
  FLOAT fDYU = -fDZ*fMinJ/fRatioY;
  FLOAT fDYD = -fDZ*fMaxJ/fRatioY;

  FLOAT fNLX = -fDZ;
  FLOAT fNLZ = +fDXL;
  FLOAT fOoNL = 1.0f/(FLOAT)sqrt(fNLX*fNLX+fNLZ*fNLZ);
  fNLX*=fOoNL; fNLZ*=fOoNL;

  FLOAT fNRX = +fDZ;
  FLOAT fNRZ = -fDXR;
  FLOAT fOoNR = 1.0f/(FLOAT)sqrt(fNRX*fNRX+fNRZ*fNRZ);
  fNRX*=fOoNR; fNRZ*=fOoNR;

  FLOAT fNDY = -fDZ;
  FLOAT fNDZ = +fDYD;
  FLOAT fOoND = 1.0f/(FLOAT)sqrt(fNDY*fNDY+fNDZ*fNDZ);
  fNDY*=fOoND; fNDZ*=fOoND;

  FLOAT fNUY = +fDZ;
  FLOAT fNUZ = -fDYU;
  FLOAT fOoNU = 1.0f/(FLOAT)sqrt(fNUY*fNUY+fNUZ*fNUZ);
  fNUY*=fOoNU; fNUZ*=fOoNU;

  // make clip planes
  pr_plClipU = FLOATplane3D(FLOAT3D(   0,fNUY,fNUZ), 0.0f);
  pr_plClipD = FLOATplane3D(FLOAT3D(   0,fNDY,fNDZ), 0.0f);
  pr_plClipL = FLOATplane3D(FLOAT3D(fNLX,   0,fNLZ), 0.0f);
  pr_plClipR = FLOATplane3D(FLOAT3D(fNRX,   0,fNRZ), 0.0f);

  // mark as prepared
  pr_Prepared = TRUE;

  // calculate constant value used for calculating z-buffer k-value from vertex's z coordinate
  pr_fDepthBufferFactor = -pr_NearClipDistance;
  pr_fDepthBufferMul = pr_fDepthBufferFar-pr_fDepthBufferNear;
  pr_fDepthBufferAdd = pr_fDepthBufferNear;

  // calculate ratio for mip factor calculation
  ppr_fMipRatio = pr_ScreenBBox.Size()(1)/(ppr_PerspectiveRatios(1)*640.0f);
}

/*
 * Project 3D object point into 3D view space, before clipping.
 */
void CPerspectiveProjection3D::PreClip(const FLOAT3D &v3dObjectPoint,
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
void CPerspectiveProjection3D::PostClip( const FLOAT3D &v3dTransformedPoint,
                                         FLOAT3D &v3dViewPoint) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);
  // divide X and Y with Z and add the center of screen
  const FLOAT f1oTransZ = 1.0f / v3dTransformedPoint(3);
  v3dViewPoint(1) = pr_ScreenCenter(1) + v3dTransformedPoint(1) * ppr_PerspectiveRatios(1) *f1oTransZ;
  v3dViewPoint(2) = pr_ScreenCenter(2) - v3dTransformedPoint(2) * ppr_PerspectiveRatios(2) *f1oTransZ;
}


void CPerspectiveProjection3D::PostClip( const FLOAT3D &v3dTransformedPoint, FLOAT fTransformedR,
                                         FLOAT3D &v3dViewPoint, FLOAT &fViewR) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);
  // multiply X and Y coordinates with zoom factor and add the center of screen
  v3dViewPoint(3) = 1.0f / v3dTransformedPoint(3);
  v3dViewPoint(1) = pr_ScreenCenter(1) + v3dTransformedPoint(1) * ppr_PerspectiveRatios(1) * v3dViewPoint(3);
  v3dViewPoint(2) = pr_ScreenCenter(2) - v3dTransformedPoint(2) * ppr_PerspectiveRatios(2) * v3dViewPoint(3);
  fViewR = fTransformedR * ppr_PerspectiveRatios(1) * v3dViewPoint(3);
}


/* Test if a sphere in view space is inside view frustum. */
INDEX CPerspectiveProjection3D::TestSphereToFrustum( const FLOAT3D &vViewPoint, FLOAT fRadius) const
{
  ASSERT( pr_Prepared && fRadius>=0);
  const FLOAT fX = vViewPoint(1);
  const FLOAT fY = vViewPoint(2);
  const FLOAT fZ = vViewPoint(3);
  INDEX iPass = +1;

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
INDEX CPerspectiveProjection3D::TestBoxToFrustum(const FLOATobbox3D &box) const
{
  ASSERT( pr_Prepared);
  INDEX iPass = 1;
  INDEX iTest;

  // check to near
  iTest = (INDEX) box.TestAgainstPlane( FLOATplane3D(FLOAT3D(0,0,-1), pr_NearClipDistance));
  if( iTest<0) {
    return -1;
  } else if( iTest==0) {
    iPass = 0;
  }
  // check to far
  if( pr_FarClipDistance>0) {
    iTest = (INDEX) box.TestAgainstPlane( FLOATplane3D(FLOAT3D(0,0,1), -pr_FarClipDistance));
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
void CPerspectiveProjection3D::ProjectCoordinate(const FLOAT3D &v3dObjectPoint,
                                       FLOAT3D &v3dViewPoint) const
{
  // rotate and translate the point
  v3dViewPoint = v3dObjectPoint*pr_RotationMatrix + pr_TranslationVector;

  // divide X and Y with Z and add the center of screen
  v3dViewPoint(1) = pr_ScreenCenter(1) +
      v3dViewPoint(1) * ppr_PerspectiveRatios(1) / v3dViewPoint(3);
  v3dViewPoint(2) = pr_ScreenCenter(2) +
      v3dViewPoint(2) * ppr_PerspectiveRatios(2) / v3dViewPoint(3);
}

/*
 * Get a distance of object point from the viewer.
 */
FLOAT CPerspectiveProjection3D::GetDistance(const FLOAT3D &v3dObjectPoint) const
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
void CPerspectiveProjection3D::ProjectDirection(const FLOAT3D &v3dObjectPoint,
                                       FLOAT3D &v3dViewPoint) const
{
  // rotate the direction
  v3dViewPoint = v3dObjectPoint*pr_mDirectionRotation;
}

/*
 * Project 3D object axis aligned bounding box into 3D view space.
 */
void CPerspectiveProjection3D::ProjectAABBox(const FLOATaabbox3D &boxObject,
                                             FLOATaabbox3D &boxView) const
{
  ASSERTALWAYS( "This is not yet implemented");
}

/*
 * Project 3D object plane into 3D view space.
 */
void CPerspectiveProjection3D::Project(const FLOATplane3D &p3dObjectPlane,
                                FLOATplane3D &p3dTransformedPlane) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // rotate and translate the normal vector
  p3dTransformedPlane = p3dObjectPlane*pr_mDirectionRotation + pr_TranslationVector;
}

/* Calculate plane gradient for a plane in 3D view space. */
void CPerspectiveProjection3D::MakeOoKGradient(const FLOATplane3D &plViewerPlane, CPlanarGradients &pgOoK) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  // get perspective factors
  FLOAT oorx = +1/ppr_PerspectiveRatios(1);
  FLOAT oory = -1/ppr_PerspectiveRatios(2);
  FLOAT ci = pr_ScreenCenter(1);
  FLOAT cj = pr_ScreenCenter(2);
  FLOAT f = pr_fDepthBufferFactor;
  FLOAT fn = f;

  // normalize control vectors
  FLOAT nx = plViewerPlane(1)*fn;
  FLOAT ny = plViewerPlane(2)*fn;
  FLOAT nz = plViewerPlane(3)*fn;
  FLOAT oond = 1/plViewerPlane.Distance();
  // calculate gradients
  FLOAT dookodi = nx*oond*oorx;
  FLOAT dookodj = ny*oond*oory;
  FLOAT ook00  = nz*oond-dookodi*ci-dookodj*cj;

  // remember the gradients
  pgOoK.pg_f00      = ook00;
  pgOoK.pg_fDOverDI = dookodi;
  pgOoK.pg_fDOverDJ = dookodj;
}
/*
 * Clip a line.
 */
ULONG CPerspectiveProjection3D::ClipLine(FLOAT3D &v3dPoint0, FLOAT3D &v3dPoint1) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  ULONG ulCode0 = LCFVERTEX0(LCF_UNCLIPPED);
  ULONG ulCode1 = LCFVERTEX1(LCF_UNCLIPPED);

  // clip the line by each plane at the time, skip if some removes entire line
  if (ClipLineByNearPlane(v3dPoint0, v3dPoint1, pr_NearClipDistance, ulCode0, ulCode1, LCF_NEAR)
    && ((pr_FarClipDistance<0) ||
      ClipLineByFarPlane(v3dPoint0, v3dPoint1, pr_FarClipDistance,  ulCode0, ulCode1, LCF_FAR))
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
void CPerspectiveProjection3D::RayThroughPoint(const FLOAT3D &v3dViewPoint,
                                               CPlacement3D &plRay) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  /* Assume z coordinate of -1 and calculate x and y for that.
   * These are in fact just the perspective formulae, solved for transformed point.
   * The result is a direction in viewer space.
   */
  FLOAT3D v3dDirection;

  v3dDirection(1) = -(v3dViewPoint(1) - pr_ScreenCenter(1))/ppr_PerspectiveRatios(1);
  v3dDirection(2) = -(v3dViewPoint(2) - pr_ScreenCenter(2))/ppr_PerspectiveRatios(2);
  v3dDirection(3) = -1.0f;

  // back-rotate the ray to absolute space
  v3dDirection *= !pr_ViewerRotationMatrix;

  // normalize the ray
  v3dDirection.Normalize();

  // now calculate the angles from the direction
  DirectionVectorToAngles(v3dDirection, plRay.pl_OrientationAngle);

  // position is same as viewer's
  plRay.pl_PositionVector = pr_vViewerPosition;
}

/*
 * Check if an object-space plane is visible.
 */
BOOL CPerspectiveProjection3D::IsObjectPlaneVisible(const FLOATplane3D &p3dObjectPlane) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  /*
    In perspective projection, plane is invisible if viewer is not in front of plane.
    NOTES: 1) Could add a check for plane beeing inside view frustum.
   */
  // if viewer is in front of plane, after plane is transformed into viewer space
  // (viewer is at 0,0,0)
  if ( (p3dObjectPlane*pr_mDirectionRotation + pr_TranslationVector).Distance() < 0.0f ) {
    // plane might be visible (although it still might be out of the view frustum)
    return TRUE;
  // if viewer is on the plane or behind it
  } else {
    // plane is surely not visible
    return FALSE;
  }
}

/*
 * Check if a viewer-space plane is visible.
 */
BOOL CPerspectiveProjection3D::IsViewerPlaneVisible(const FLOATplane3D &p3dViewerPlane) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);

  /*
    In perspective projection, plane is invisible if viewer is not in front of plane.
    NOTES: 1) Could add a check for plane beeing inside view frustum.
   */
  // if viewer is in front of plane (viewer is at 0,0,0)
  if ( p3dViewerPlane.Distance() < -0.01f ) {
    // plane might be visible (although it still might be out of the view frustum)
    return TRUE;
  // if viewer is on the plane or behind it
  } else {
    // plane is surely not visible
    return FALSE;
  }
}


/*
 * Calculate a mip-factor for a given object.
 */
// by its distance from viewer
FLOAT CPerspectiveProjection3D::MipFactor(FLOAT fDistance) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);
  // calculated using following formula: k = log2(1024*z/xratio);
  return Log2( (FLOAT)Abs(1024.0f*fDistance*ppr_fMipRatio));
}


// general mip-factor for target object
FLOAT CPerspectiveProjection3D::MipFactor(void) const
{
  // check that the projection object is prepared for projecting
  ASSERT(pr_Prepared);
  // calculated using following formula: k = log2(1024*z/xratio);
  // the distance is, in fact, the z coordinate of the translation vector
  return -pr_TranslationVector(3)*TanFast(ppr_FOVWidth/2.0f); // /Tan(90.0f/2.0f)=1;
}
