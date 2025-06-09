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

#ifndef SE_INCL_PROJECTION_H
#define SE_INCL_PROJECTION_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Math/Vector.h>
#include <Engine/Math/Matrix.h>
#include <Engine/Math/Plane.h>
#include <Engine/Math/AABBox.h>
#include <Engine/Math/Placement.h>

/*
 * Geometric projection of one 3D space onto another 3D space
 */
class ENGINE_API CProjection3D {
public:
// implementation:
  // factors set by user
  CPlacement3D pr_ObjectPlacement;  // placement of the projected object in absolute space
  FLOAT3D pr_vObjectHandle;         // handle the projected object in its own space
  CPlacement3D pr_ViewerPlacement;  // placement of the viewer in absolute space
  FLOAT3D pr_vViewerPosition;       // viewer position (possibly mirrored)
  FLOAT pr_NearClipDistance;        // distance of near clipping plane from viewer
  FLOAT pr_FarClipDistance;         // distance of far clipping plane from viewer
  FLOATaabbox2D pr_ScreenBBox;      // bounding box of viewing screen
  FLOAT pr_AspectRatio;             // aspect ratio of viewing screen
  FLOAT3D pr_ObjectStretch;         // stretching coeficients for target object space
  BOOL pr_bFaceForward;             // set if object is face-forward
  BOOL pr_bHalfFaceForward;         // set if object is face-forward, but only on heading
  BOOL pr_bMirror;                  // enable mirror projection
  BOOL pr_bWarp;                    // enable warp clip
  FLOATplane3D pr_plMirror;         // plane to mirror(warp) about
  FLOATplane3D pr_plMirrorView;     // mirror(warp) clip plane in view space
  FLOAT pr_fViewStretch;            // stretch of entire view

  // internal variables
  BOOL pr_Prepared;                 // set if all precalculated variables are prepared
  BOOL pr_bInverted;                // set if projection is inverted
  FLOATmatrix3D pr_RotationMatrix;  // matrix for rotating when projecting
  FLOATmatrix3D pr_mDirectionRotation;  // matrix for rotating direction vectors
  FLOATmatrix3D pr_ViewerRotationMatrix;  // viewer part of rotation matrix
  FLOAT3D pr_TranslationVector;     // vector for translating when projecting
  FLOAT2D pr_ScreenCenter;          // center of viewing screen

  FLOAT pr_fDepthBufferFactor;  // correction to 0..1
  FLOAT pr_fDepthBufferMul;     // correction to needed range
  FLOAT pr_fDepthBufferAdd;
  FLOAT pr_fDepthBufferNear;  // depth buffer range used
  FLOAT pr_fDepthBufferFar;

  // clip planes (in view space)
  FLOATplane3D pr_plClipL;
  FLOATplane3D pr_plClipR;
  FLOATplane3D pr_plClipU;
  FLOATplane3D pr_plClipD;

public:
// interface:
  // construction/destruction
  /* Default constructor. */
  CProjection3D(void);

  // member referencing
  /* Reference object placement. */
  inline CPlacement3D &ObjectPlacementL(void);
  inline const CPlacement3D &ObjectPlacementR(void) const;
  /* Reference object handle. */
  inline FLOAT3D &ObjectHandleL(void);
  inline const FLOAT3D &ObjectHandleR(void) const;
  /* Reference viewer placement. */
  inline CPlacement3D &ViewerPlacementL(void);
  inline const CPlacement3D &ViewerPlacementR(void) const;
  /* Reference clipping distances. */
  inline FLOAT &FrontClipDistanceL(void);             // obsolete
  inline const FLOAT &FrontClipDistanceR(void) const; // obsolete
  inline FLOAT &NearClipDistanceL(void);
  inline const FLOAT &NearClipDistanceR(void) const;
  inline FLOAT &FarClipDistanceL(void);
  inline const FLOAT &FarClipDistanceR(void) const;
  /* Reference screen bounding box. */
  inline FLOATaabbox2D &ScreenBBoxL(void);
  inline const FLOATaabbox2D &ScreenBBoxR(void) const;
  /* Reference screen aspect ratio. */
  inline FLOAT &AspectRatioL(void);
  inline const FLOAT &AspectRatioR(void) const;
  /* Reference target object stretching. */
  inline FLOAT3D &ObjectStretchL(void);
  inline const FLOAT3D &ObjectStretchR(void) const;
  /* Reference miror plane. */
  inline FLOATplane3D &MirrorPlaneL(void);
  inline const FLOATplane3D &MirrorPlaneR(void) const;
  inline void TurnOffMirrorPlane(void);
  /* Reference warp plane. */
  inline FLOATplane3D &WarpPlaneL(void);
  inline const FLOATplane3D &WarpPlaneR(void) const;
  inline void TurnOffWarpPlane(void);
  /* Reference target object face-forward flag. */
  inline BOOL &ObjectFaceForwardL(void);
  inline const BOOL &ObjectFaceForwardR(void) const;
  inline BOOL &ObjectHalfFaceForwardL(void);
  inline const BOOL &ObjectHalfFaceForwardR(void) const;
  /* Reference corrections for depth buffer factor. */
  inline FLOAT &DepthBufferNearL(void);
  inline const FLOAT &DepthBufferNearR(void) const;
  inline FLOAT &DepthBufferFarL(void);
  inline const FLOAT &DepthBufferFarR(void) const;
  /* Reference view stretching. */
  inline FLOAT &ViewStretchL(void);
  inline const FLOAT &ViewStretchR(void) const;

  /* Prepare for projecting. */
  virtual void Prepare(void) = 0;
  virtual BOOL IsPerspective(void) { return FALSE; };

  /* Project 3D object point into 3D view space. */
  virtual void ProjectCoordinate(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dViewPoint) const = 0;
  /* Get a distance of object point from the viewer. */
  virtual FLOAT GetDistance(const FLOAT3D &v3dObjectPoint) const = 0;
  /* Project 3D object direction vector into 3D view space. */
  virtual void ProjectDirection(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dViewPoint) const = 0;
  /* Project 3D object axis aligned bounding box into 3D view space. */
  virtual void ProjectAABBox(const FLOATaabbox3D &boxObject, FLOATaabbox3D &boxView) const = 0;
  /* Project 3D object point into 3D view space, before clipping. */
  virtual void PreClip(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dTransformedPoint) const = 0;
  /* Clip a line. */
  virtual ULONG ClipLine(FLOAT3D &v3dPoint0, FLOAT3D &v3dPoint1) const = 0;
  /* Project 3D object point into 3D view space, after clipping. */
  virtual void PostClip(const FLOAT3D &v3dTransformedPoint, FLOAT3D &v3dViewPoint) const = 0;
  virtual void PostClip(const FLOAT3D &v3dTransformedPoint, FLOAT fTransformedR,
    FLOAT3D &v3dViewPoint, FLOAT &fViewR) const = 0;
  /* Test if a sphere in view space is inside view frustum. */
  virtual INDEX TestSphereToFrustum(const FLOAT3D &vViewPoint, FLOAT fRadius) const = 0;
  /* Test if an oriented box in view space is inside view frustum. */
  virtual INDEX TestBoxToFrustum(const FLOATobbox3D &boxView) const = 0;

  /* Get placement for a ray through a projected point. */
  virtual void RayThroughPoint(const FLOAT3D &v3dViewPoint, CPlacement3D &plRay) const = 0;

  /* Project 3D object plane into 3D view space. */
  virtual void Project(const FLOATplane3D &p3dObjectPlane, FLOATplane3D &v3dTransformedPlane) const = 0;
  /* Check if an object-space plane is visible. */
  virtual BOOL IsObjectPlaneVisible(const FLOATplane3D &p3dObjectPlane) const = 0;
  /* Check if a viewer-space plane is visible. */
  virtual BOOL IsViewerPlaneVisible(const FLOATplane3D &p3dViewerPlane) const = 0;

  /* Calculate a mip-factor for a given object. */
  // by its distance from viewer
  virtual FLOAT MipFactor(FLOAT fDistance) const = 0;
  // general mip-factor for target object
  virtual FLOAT MipFactor(void) const = 0;

  /* Calculate plane gradient for a plane in 3D view space. */
  virtual void MakeOoKGradient(const FLOATplane3D &plViewerPlane, CPlanarGradients &pgOoK) const = 0;
};

/*
 * Perspective projection.
 */
class ENGINE_API CPerspectiveProjection3D : public CProjection3D {
public:
// implementation:
  // factors set by user
  ANGLE ppr_FOVWidth;                // width of field-of-view
  FLOAT ppr_fMipRatio;               // for mip-factor calculation

  // internal variables
  FLOAT2D ppr_PerspectiveRatios;     // ratios for perspective projection

  // factors for shadow casting projections
  FLOAT ppr_fMetersPerPixel;    // meters per pixel on destination plane
  FLOAT ppr_fViewerDistance;    // distance between viewer and destination plane
  FLOATaabbox2D ppr_boxSubScreen;      // box-in-box for rendering on subdrawports
public:
// interface:
  // constructor
  CPerspectiveProjection3D(void);

  // member referencing
  /* Reference field of view. */
  inline ANGLE &FOVL(void);
  inline const ANGLE &FOVR(void) const;

  /* Prepare for projecting. */
  virtual void Prepare(void);
  virtual BOOL IsPerspective(void) { return TRUE; };

  /* Project 3D object point into 3D view space. */
  void ProjectCoordinate(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dViewPoint) const;
  /* Get a distance of object point from the viewer. */
  FLOAT GetDistance(const FLOAT3D &v3dObjectPoint) const;
  /* Project 3D object direction vector into 3D view space. */
  void ProjectDirection(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dViewPoint) const;
  /* Project 3D object axis aligned bounding box into 3D view space. */
  virtual void ProjectAABBox(const FLOATaabbox3D &boxObject, FLOATaabbox3D &boxView) const;
  /* Project 3D object point into 3D view space, before clipping. */
  virtual void PreClip(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dTransformedPoint) const;
  /* Clip a line. */
  virtual ULONG ClipLine(FLOAT3D &v3dPoint0, FLOAT3D &v3dPoint1) const;
  /* Project 3D object point into 3D view space, after clipping. */
  virtual void PostClip(const FLOAT3D &v3dTransformedPoint, FLOAT3D &v3dViewPoint) const;
  virtual void PostClip(const FLOAT3D &v3dTransformedPoint, FLOAT fTransformedR,
    FLOAT3D &v3dViewPoint, FLOAT &fViewR) const;
  /* Test if a sphere in view space is inside view frustum. */
  INDEX TestSphereToFrustum(const FLOAT3D &vViewPoint, FLOAT fRadius) const;
  /* Test if an oriented box in view space is inside view frustum. */
  INDEX TestBoxToFrustum(const FLOATobbox3D &boxView) const;

  /* Calculate plane gradient for a plane in 3D view space. */
  virtual void MakeOoKGradient(const FLOATplane3D &plViewerPlane, CPlanarGradients &pgOoK) const;

  /* Get placement for a ray through a projected point. */
  virtual void RayThroughPoint(const FLOAT3D &v3dViewPoint, CPlacement3D &plRay) const;

  /* Project 3D object plane into 3D view space. */
  virtual void Project(const FLOATplane3D &p3dObjectPlane, FLOATplane3D &v3dTransformedPlane) const;
  /* Check if an object-space plane is visible. */
  virtual BOOL IsObjectPlaneVisible(const FLOATplane3D &p3dObjectPlane) const;
  /* Check if a viewer-space plane is visible. */
  virtual BOOL IsViewerPlaneVisible(const FLOATplane3D &p3dViewerPlane) const;

  /* Calculate a mip-factor for a given object. */
  // by its distance from viewer
  virtual FLOAT MipFactor(FLOAT fDistance) const;
  // general mip-factor for target object
  virtual FLOAT MipFactor(void) const;
};

/*
 * Isometric projection.
 */
class ENGINE_API CIsometricProjection3D : public CProjection3D {
public:
// implementation:
  // factors set by user
  FLOAT ipr_ZoomFactor;              // zoom factor

  // internal variables
public:
// implementation:
  // member referencing
  /* Reference zoom factor. */
  inline FLOAT &ZoomFactorL(void);
  inline const FLOAT &ZoomFactorR(void) const;

  /* Prepare for projecting. */
  virtual void Prepare(void);

  /* Project 3D object point into 3D view space. */
  void ProjectCoordinate(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dViewPoint) const;
  /* Get a distance of object point from the viewer. */
  FLOAT GetDistance(const FLOAT3D &v3dObjectPoint) const;
  /* Project 3D object direction vector into 3D view space. */
  void ProjectDirection(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dViewPoint) const;
  /* Project 3D object axis aligned bounding box into 3D view space. */
  virtual void ProjectAABBox(const FLOATaabbox3D &boxObject, FLOATaabbox3D &boxView) const;
  /* Project 3D object point into 3D view space, before clipping. */
  virtual void PreClip(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dTransformedPoint) const;
  /* Clip a line. */
  virtual ULONG ClipLine(FLOAT3D &v3dPoint0, FLOAT3D &v3dPoint1) const;
  /* Project 3D object point into 3D view space, after clipping. */
  virtual void PostClip(const FLOAT3D &v3dTransformedPoint, FLOAT3D &v3dViewPoint) const;
  virtual void PostClip(const FLOAT3D &v3dTransformedPoint, FLOAT fTransformedR,
    FLOAT3D &v3dViewPoint, FLOAT &fViewR) const;
  /* Test if a sphere in view space is inside view frustum. */
  INDEX TestSphereToFrustum(const FLOAT3D &vViewPoint, FLOAT fRadius) const;
  /* Test if an oriented box in view space is inside view frustum. */
  INDEX TestBoxToFrustum(const FLOATobbox3D &boxView) const;

  /* Get placement for a ray through a projected point. */
  virtual void RayThroughPoint(const FLOAT3D &v3dViewPoint, CPlacement3D &plRay) const;

  /* Calculate plane gradient for a plane in 3D view space. */
  virtual void MakeOoKGradient(const FLOATplane3D &plViewerPlane, CPlanarGradients &pgOoK) const;

  /* Project 3D object plane into 3D view space. */
  virtual void Project(const FLOATplane3D &p3dObjectPlane, FLOATplane3D &v3dTransformedPlane) const;
  /* Check if an object-space plane is visible. */
  virtual BOOL IsObjectPlaneVisible(const FLOATplane3D &p3dObjectPlane) const;
  /* Check if a viewer-space plane is visible. */
  virtual BOOL IsViewerPlaneVisible(const FLOATplane3D &p3dViewerPlane) const;

  /* Calculate a mip-factor for a given object. */
  // by its distance from viewer
  virtual FLOAT MipFactor(FLOAT fDistance) const;
  // general mip-factor for target object
  virtual FLOAT MipFactor(void) const;
};

/*
 * Isometric projection.
 */
class ENGINE_API CParallelProjection3D : public CProjection3D {
public:
// implementation:
  // factors set by user
  FLOAT2D pr_vStepFactors;    // gradient of x and y along z (angle of parallel projection)
  FLOAT2D pr_vZoomFactors;    // zoom of x and y

  // internal variables
  FLOAT3D pr_vViewDirection;  // heads in the direction of viewing
public:
// implementation:
  /* Prepare for projecting. */
  virtual void Prepare(void);

  /* Project 3D object point into 3D view space. */
  virtual void ProjectCoordinate(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dViewPoint) const;
  /* Get a distance of object point from the viewer. */
  virtual FLOAT GetDistance(const FLOAT3D &v3dObjectPoint) const;
  /* Project 3D object direction vector into 3D view space. */
  virtual void ProjectDirection(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dViewPoint) const;
  /* Project 3D object axis aligned bounding box into 3D view space. */
  virtual void ProjectAABBox(const FLOATaabbox3D &boxObject, FLOATaabbox3D &boxView) const;
  /* Project 3D object point into 3D view space, before clipping. */
  virtual void PreClip(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dTransformedPoint) const;
  /* Clip a line. */
  virtual ULONG ClipLine(FLOAT3D &v3dPoint0, FLOAT3D &v3dPoint1) const;
  /* Project 3D object point into 3D view space, after clipping. */
  virtual void PostClip(const FLOAT3D &v3dTransformedPoint, FLOAT3D &v3dViewPoint) const;
  virtual void PostClip(const FLOAT3D &v3dTransformedPoint, FLOAT fTransformedR,
    FLOAT3D &v3dViewPoint, FLOAT &fViewR) const;
  /* Test if a sphere in view space is inside view frustum. */
  INDEX TestSphereToFrustum(const FLOAT3D &vViewPoint, FLOAT fRadius) const;
  /* Test if an oriented box in view space is inside view frustum. */
  INDEX TestBoxToFrustum(const FLOATobbox3D &boxView) const;

  /* Get placement for a ray through a projected point. */
  virtual void RayThroughPoint(const FLOAT3D &v3dViewPoint, CPlacement3D &plRay) const;

  /* Calculate plane gradient for a plane in 3D view space. */
  virtual void MakeOoKGradient(const FLOATplane3D &plViewerPlane, CPlanarGradients &pgOoK) const;

  /* Project 3D object plane into 3D view space. */
  virtual void Project(const FLOATplane3D &p3dObjectPlane, FLOATplane3D &v3dTransformedPlane) const;
  /* Check if an object-space plane is visible. */
  virtual BOOL IsObjectPlaneVisible(const FLOATplane3D &p3dObjectPlane) const;
  /* Check if a viewer-space plane is visible. */
  virtual BOOL IsViewerPlaneVisible(const FLOATplane3D &p3dViewerPlane) const;

  /* Calculate a mip-factor for a given object. */
  // by its distance from viewer
  virtual FLOAT MipFactor(FLOAT fDistance) const;
  // general mip-factor for target object
  virtual FLOAT MipFactor(void) const;
};

/*
 * Simple projection.
 */
class ENGINE_API CSimpleProjection3D : public CProjection3D {
public:
// implementation:
  // factors set by user
  // internal variables
public:
// implementation:
  // member referencing

  /* Prepare for projecting. */
  virtual void Prepare(void);

  /* Project 3D object point into 3D view space. */
  void ProjectCoordinate(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dViewPoint) const;
  /* Get a distance of object point from the viewer. */
  FLOAT GetDistance(const FLOAT3D &v3dObjectPoint) const;
  /* Project 3D object direction vector into 3D view space. */
  void ProjectDirection(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dViewPoint) const;
  /* Project 3D object placement into 3D view space. */
  void ProjectPlacement(const CPlacement3D &plObject, CPlacement3D &plView) const;
  void ProjectPlacementSmooth(const CPlacement3D &plObject, CPlacement3D &plView) const;
  /* Project 3D object axis aligned bounding box into 3D view space. */
  virtual void ProjectAABBox(const FLOATaabbox3D &boxObject, FLOATaabbox3D &boxView) const;
  /* Project 3D object point into 3D view space, before clipping. */
  virtual void PreClip(const FLOAT3D &v3dObjectPoint, FLOAT3D &v3dTransformedPoint) const;
  /* Clip a line. */
  virtual ULONG ClipLine(FLOAT3D &v3dPoint0, FLOAT3D &v3dPoint1) const;
  /* Project 3D object point into 3D view space, after clipping. */
  virtual void PostClip(const FLOAT3D &v3dTransformedPoint, FLOAT3D &v3dViewPoint) const;
  virtual void PostClip(const FLOAT3D &v3dTransformedPoint, FLOAT fTransformedR,
    FLOAT3D &v3dViewPoint, FLOAT &fViewR) const;
  /* Test if a sphere in view space is inside view frustum. */
  INDEX TestSphereToFrustum(const FLOAT3D &vViewPoint, FLOAT fRadius) const;
  /* Test if an oriented box in view space is inside view frustum. */
  INDEX TestBoxToFrustum(const FLOATobbox3D &boxView) const;

  /* Get placement for a ray through a projected point. */
  virtual void RayThroughPoint(const FLOAT3D &v3dViewPoint, CPlacement3D &plRay) const;

  /* Calculate plane gradient for a plane in 3D view space. */
  virtual void MakeOoKGradient(const FLOATplane3D &plViewerPlane, CPlanarGradients &pgOoK) const;

  /* Project 3D object plane into 3D view space. */
  virtual void Project(const FLOATplane3D &p3dObjectPlane, FLOATplane3D &v3dTransformedPlane) const;
  /* Check if an object-space plane is visible. */
  virtual BOOL IsObjectPlaneVisible(const FLOATplane3D &p3dObjectPlane) const;
  /* Check if a viewer-space plane is visible. */
  virtual BOOL IsViewerPlaneVisible(const FLOATplane3D &p3dViewerPlane) const;

  /* Calculate a mip-factor for a given object. */
  // by its distance from viewer
  virtual FLOAT MipFactor(FLOAT fDistance) const;
  // general mip-factor for target object
  virtual FLOAT MipFactor(void) const;
};

/*
 * Holder for any kind of 3D projection.
 */
class ENGINE_API CAnyProjection3D {
private:
  CSimpleProjection3D ap_Simple;
  CIsometricProjection3D ap_Isometric;
  CPerspectiveProjection3D ap_Perspective;
  CParallelProjection3D ap_Parallel;
  CProjection3D *ap_CurrentProjection;
public:
  /* Default constructor. */
  inline CAnyProjection3D(void) : ap_CurrentProjection(NULL) {};
  /* Copy constructor. */
  inline CAnyProjection3D(const CAnyProjection3D &apOriginal) { operator=(apOriginal);}
  /* Start being CSimpleProjection3D. */
  inline void BeSimple(void);
  /* Test if CSimpleProjection3D. */
  inline BOOL IsSimple(void);
  /* Start being CIsometricProjection3D. */
  inline void BeIsometric(void);
  /* Test if CIsometricProjection3D. */
  inline BOOL IsIsometric(void);
  /* Start being CPerspectiveProjection3D. */
  inline void BePerspective(void);
  /* Test if CPerspectiveProjection3D. */
  inline BOOL IsPerspective(void);
  /* Start being CParallelProjection3D. */
  inline void BeParallel(void);
  /* Test if CParallelProjection3D. */
  inline BOOL IsParallel(void);
  /* Reference currently active projection. */
  inline CProjection3D *operator->(void);
  /* Get the pointer to currently active projection. */
  inline operator CProjection3D *(void);

  /* Initialize from another any-projection. */
  inline void operator=(const CAnyProjection3D &prAny);
  /* Initialize from a simple projection. */
  inline void operator=(const CSimpleProjection3D &prSimple);
  /* Initialize from an isometric projection. */
  inline void operator=(const CIsometricProjection3D &prIsometric);
  /* Initialize from a perspective projection. */
  inline void operator=(const CPerspectiveProjection3D &prPerspective);
  /* Initialize from a parallel projection. */
  inline void operator=(const CParallelProjection3D &prParallel);
};


/////////////////////////////////////////////////////////////////////
//  CProjection3D
/////////////////////////////////////////////////////////////////////
// Member referencing functions

/*
 * Reference viewer placement.
 */
ENGINE_API inline CPlacement3D &CProjection3D::ViewerPlacementL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_ViewerPlacement;
}
ENGINE_API inline const CPlacement3D &CProjection3D::ViewerPlacementR(void) const {
  return pr_ViewerPlacement;
}

/*
 * Reference object placement.
 */
ENGINE_API inline CPlacement3D &CProjection3D::ObjectPlacementL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_ObjectPlacement;
}
ENGINE_API inline const CPlacement3D &CProjection3D::ObjectPlacementR(void) const {
  return pr_ObjectPlacement;
}

/* Reference object handle. */
ENGINE_API inline FLOAT3D &CProjection3D::ObjectHandleL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_vObjectHandle;
}
ENGINE_API inline const FLOAT3D &CProjection3D::ObjectHandleR(void) const {
  return pr_vObjectHandle;
}

/*
 * Reference front clipping distance.
 */
ENGINE_API inline FLOAT &CProjection3D::FrontClipDistanceL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_NearClipDistance;
}
ENGINE_API inline const FLOAT &CProjection3D::FrontClipDistanceR(void) const {
  return pr_NearClipDistance;
}
ENGINE_API inline FLOAT &CProjection3D::NearClipDistanceL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_NearClipDistance;
}
ENGINE_API inline const FLOAT &CProjection3D::NearClipDistanceR(void) const {
  return pr_NearClipDistance;
}
ENGINE_API inline FLOAT &CProjection3D::FarClipDistanceL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_FarClipDistance;
}
ENGINE_API inline const FLOAT &CProjection3D::FarClipDistanceR(void) const {
  return pr_FarClipDistance;
}

/*
 * Reference screen bounding box.
 */
ENGINE_API inline FLOATaabbox2D &CProjection3D::ScreenBBoxL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_ScreenBBox;
}
ENGINE_API inline const FLOATaabbox2D &CProjection3D::ScreenBBoxR(void) const {
  return pr_ScreenBBox;
}

/*
 * Reference screen aspect ratio.
 */
ENGINE_API inline FLOAT &CProjection3D::AspectRatioL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_AspectRatio;
}
ENGINE_API inline const FLOAT &CProjection3D::AspectRatioR(void) const {
  return pr_AspectRatio;
}

/* Reference target object stretching. */
ENGINE_API inline FLOAT3D &CProjection3D::ObjectStretchL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_ObjectStretch;
}
ENGINE_API inline const FLOAT3D &CProjection3D::ObjectStretchR(void) const {
  return pr_ObjectStretch;
}

/* Reference view stretching. */
ENGINE_API inline FLOAT &CProjection3D::ViewStretchL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_fViewStretch;
}
ENGINE_API inline const FLOAT &CProjection3D::ViewStretchR(void) const {
  return pr_fViewStretch;
}

/* Reference mirror plane. */
ENGINE_API inline FLOATplane3D &CProjection3D::MirrorPlaneL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  pr_bMirror = TRUE;
  return pr_plMirror;
}
ENGINE_API inline const FLOATplane3D &CProjection3D::MirrorPlaneR(void) const {
  return pr_plMirror;
}
inline void CProjection3D::TurnOffMirrorPlane(void)
{
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  pr_bMirror = FALSE;
}
/* Reference warp plane. */
ENGINE_API inline FLOATplane3D &CProjection3D::WarpPlaneL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  pr_bWarp = TRUE;
  return pr_plMirror;
}
ENGINE_API inline const FLOATplane3D &CProjection3D::WarpPlaneR(void) const {
  return pr_plMirror;
}
inline void CProjection3D::TurnOffWarpPlane(void)
{
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  pr_bWarp = FALSE;
}

/*
 * Reference object face-forward flag.
 */
ENGINE_API inline BOOL &CProjection3D::ObjectFaceForwardL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_bFaceForward;
}
ENGINE_API inline const BOOL &CProjection3D::ObjectFaceForwardR(void) const {
  return pr_bFaceForward;
}
ENGINE_API inline BOOL &CProjection3D::ObjectHalfFaceForwardL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_bHalfFaceForward;
}
ENGINE_API inline const BOOL &CProjection3D::ObjectHalfFaceForwardR(void) const {
  return pr_bHalfFaceForward;
}

/* Reference corrections for depth buffer factor. */
ENGINE_API inline FLOAT &CProjection3D::DepthBufferNearL(void)
{
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_fDepthBufferNear;
}
ENGINE_API inline const FLOAT &CProjection3D::DepthBufferNearR(void) const {
  return pr_fDepthBufferNear;
}
ENGINE_API inline FLOAT &CProjection3D::DepthBufferFarL(void)
{
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return pr_fDepthBufferFar;
}
ENGINE_API inline const FLOAT &CProjection3D::DepthBufferFarR(void) const {
  return pr_fDepthBufferFar;
}

/////////////////////////////////////////////////////////////////////
//  CPerspectiveProjection3D
/////////////////////////////////////////////////////////////////////
// Member referencing functions

/*
 * Reference field of view
 */
ENGINE_API inline ANGLE &CPerspectiveProjection3D::FOVL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return ppr_FOVWidth;
}
ENGINE_API inline const ANGLE &CPerspectiveProjection3D::FOVR(void) const {
  return ppr_FOVWidth;
}

/////////////////////////////////////////////////////////////////////
//  CIsometricProjection3D
/////////////////////////////////////////////////////////////////////
// Member referencing functions

/*
 * Reference zoom factor.
 */
ENGINE_API inline FLOAT &CIsometricProjection3D::ZoomFactorL(void) {
  IFDEBUG(pr_Prepared = FALSE);    // invalidate precalculations on any non-const access
  return ipr_ZoomFactor;
}
ENGINE_API inline const FLOAT &CIsometricProjection3D::ZoomFactorR(void) const {
  return ipr_ZoomFactor;
}

/////////////////////////////////////////////////////////////////////
//  CAnyProjection3D
/////////////////////////////////////////////////////////////////////
/*
 * Start being CSimpleProjection3D.
 */
ENGINE_API inline void CAnyProjection3D::BeSimple(void)
{
  ap_CurrentProjection = &ap_Simple;
}

/*
 * Start being CIsometricProjection3D.
 */
ENGINE_API inline void CAnyProjection3D::BeIsometric(void)
{
  ap_CurrentProjection = &ap_Isometric;
}

/*
 * Start beeing CPerspectiveProjection3D.
 */
ENGINE_API inline void CAnyProjection3D::BePerspective(void)
{
  ap_CurrentProjection = &ap_Perspective;
}
/*
 * Start beeing CParallelProjection3D.
 */
ENGINE_API inline void CAnyProjection3D::BeParallel(void)
{
  ap_CurrentProjection = &ap_Parallel;
}

/* Test if CSimpleProjection3D. */
ENGINE_API inline BOOL CAnyProjection3D::IsSimple(void) {
  return ap_CurrentProjection == &ap_Simple;
}
/* Test if CIsometricProjection3D. */
ENGINE_API inline BOOL CAnyProjection3D::IsIsometric(void) {
  return ap_CurrentProjection == &ap_Isometric;
}
/* Test if CPerspectiveProjection3D. */
ENGINE_API inline BOOL CAnyProjection3D::IsPerspective(void) {
  return ap_CurrentProjection == &ap_Perspective;
}
/* Test if CParallelProjection3D. */
ENGINE_API inline BOOL CAnyProjection3D::IsParallel(void) {
  return ap_CurrentProjection == &ap_Parallel;
}

/*
 * Reference currently active projection.
 */
ENGINE_API inline CProjection3D *CAnyProjection3D::operator->(void)
{
  return ap_CurrentProjection;
}

/*
 * Get the pointer to currently active projection.
 */
ENGINE_API inline CAnyProjection3D::operator CProjection3D *(void)
{
  return ap_CurrentProjection;
}

/*
 * Initialize from another any-projection.
 */
ENGINE_API inline void CAnyProjection3D::operator=(const CAnyProjection3D &prAny)
{
  // if the other is perspective
  if        ((const CProjection3D *)prAny.ap_CurrentProjection == &prAny.ap_Perspective) {
    // use perspective
    ap_Perspective = prAny.ap_Perspective;
    ap_CurrentProjection = &ap_Perspective;
  // if the other is parallel
  } else if ((const CProjection3D *)prAny.ap_CurrentProjection == &prAny.ap_Parallel) {
    // use parallel
    ap_Parallel = prAny.ap_Parallel;
    ap_CurrentProjection = &ap_Parallel;
  // if the other is simple
  } else if ((const CProjection3D *)prAny.ap_CurrentProjection == &prAny.ap_Simple) {
    // use simple
    ap_Simple = prAny.ap_Simple;
    ap_CurrentProjection = &ap_Simple;
  // if the other is isometric
  } else if ((const CProjection3D *)prAny.ap_CurrentProjection == &prAny.ap_Isometric) {
    // use isometric
    ap_Isometric = prAny.ap_Isometric;
    ap_CurrentProjection = &ap_Isometric;
  // otherwise
  } else {
    // error
    ASSERTALWAYS("CAnyProjection3D::operator=() : Invalid source object");
  }
};

/*
 * Initialize from a simple projection.
 */
ENGINE_API inline void CAnyProjection3D::operator=(const CSimpleProjection3D &prSimple)
{
  ap_Simple = prSimple;
  ap_CurrentProjection = &ap_Simple;
}

/*
 * Initialize from an isometric projection.
 */
ENGINE_API inline void CAnyProjection3D::operator=(const CIsometricProjection3D &prIsometric)
{
  ap_Isometric = prIsometric;
  ap_CurrentProjection = &ap_Isometric;
}

/*
 * Initialize from a perspective projection.
 */
ENGINE_API inline void CAnyProjection3D::operator=(const CPerspectiveProjection3D &prPerspective)
{
  ap_Perspective = prPerspective;
  ap_CurrentProjection = &ap_Perspective;
}

/*
 * Initialize from a parallel projection.
 */
ENGINE_API inline void CAnyProjection3D::operator=(const CParallelProjection3D &prParallel)
{
  ap_Parallel = prParallel;
  ap_CurrentProjection = &ap_Parallel;
}


#endif  /* include-once check. */

