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

#include "StdAfx.h"
#include "WorldEditor.h"
 
/////////////////////////////////////////////////////////////////////
// CSlaveViewer

#define MIN_TARGET_DISTANCE 0.1f
#define MAX_TARGET_DISTANCE 640000.0f

extern FLOAT wed_fFrontClipDistance;


/*
 * Default constructor
 */ 
CMasterViewer::CMasterViewer(void)
{
  // set initial viewer variables, viewing tovards origin from ten meters distance
  mv_plViewer.pl_PositionVector = FLOAT3D(3.0f, 4.0f, 10.0f);
  mv_plViewer.pl_OrientationAngle = ANGLE3D(AngleDeg( 20.0f), AngleDeg( -20.0f), 0);
  mv_fTargetDistance = 10.0f;  // this must never be very small!
}

// get placement of the virtual target
CPlacement3D CMasterViewer::GetTargetPlacement(void) const
{
  CPlacement3D plTarget;
  // copy viewer to target
  plTarget = mv_plViewer;
  // get the direction vector of viewer's sight
  FLOAT3D vDirection;
  AnglesToDirectionVector(mv_plViewer.pl_OrientationAngle, vDirection);
  // offset the target placement by target distance along the vector
  plTarget.pl_PositionVector += vDirection*mv_fTargetDistance;

  // return the target placement
  return plTarget;
}

// set placement of the virtual target
void CMasterViewer::SetTargetPlacement(FLOAT3D f3dTarget)
{
  CPlacement3D plViewer;
  // copy the viewer's orientation, it will stay the same
  plViewer.pl_OrientationAngle = mv_plViewer.pl_OrientationAngle;
  // set position vector of new target
  plViewer.pl_PositionVector = f3dTarget;
  // translate viewer back from target for their distance
  plViewer.Translate_OwnSystem( FLOAT3D( 0.0f, 0.0f, mv_fTargetDistance) );
  // set new viewer's pacement
  mv_plViewer = plViewer;
}

/*
 * Convert from slave.
 */
void CMasterViewer::operator=(const CSlaveViewer &svSlave)
{
  // copy target distance
  mv_fTargetDistance = svSlave.sv_fTargetDistance;

  // if is perspective
  if( svSlave.IsPerspective())
  {
    // copy viewer placement
    mv_plViewer = svSlave.sv_plViewer;
  }
  // if is isometric
  else
  {
    // copy slave viewer position to master viewer position
    mv_plViewer.pl_PositionVector = svSlave.sv_plViewer.pl_PositionVector;

    // get the ray of the slave viewing direction
    FLOAT3D vSlaveDirection;
    AnglesToDirectionVector(svSlave.sv_plViewer.pl_OrientationAngle, vSlaveDirection);
    // offset the viewer forward by target distance along the slave direction vector
    mv_plViewer.pl_PositionVector += vSlaveDirection*mv_fTargetDistance;

    // get the ray of the master viewing direction
    FLOAT3D vMasterDirection;
    AnglesToDirectionVector(mv_plViewer.pl_OrientationAngle, vMasterDirection);
    // offset the viewer backward by target distance along the slave direction vector
    mv_plViewer.pl_PositionVector -= vMasterDirection*mv_fTargetDistance;
  }
}

/////////////////////////////////////////////////////////////////////
// CSlaveViewer

// get orientation angles for type of isometric viewer
ANGLE3D CSlaveViewer::GetAngleForIsometricType(void) const
{
  // depending on the projection type, return angles
  switch (sv_ProjectionType) {
  case PT_ISOMETRIC_FRONT:
    return ANGLE3D((ANGLE)ANGLE_0  ,(ANGLE)ANGLE_0  ,(ANGLE)ANGLE_0  );

  case PT_ISOMETRIC_RIGHT:
    return ANGLE3D((ANGLE)ANGLE_90 ,(ANGLE)ANGLE_0  ,(ANGLE)ANGLE_0  );

  case PT_ISOMETRIC_TOP:
    return ANGLE3D((ANGLE)ANGLE_0  ,(ANGLE)ANGLE_270,(ANGLE)ANGLE_0  );

  case PT_ISOMETRIC_BACK:
    return ANGLE3D((ANGLE)ANGLE_180,(ANGLE)ANGLE_0  ,(ANGLE)ANGLE_0  );

  case PT_ISOMETRIC_LEFT:
    return ANGLE3D((ANGLE)ANGLE_270,(ANGLE)ANGLE_0  ,(ANGLE)ANGLE_0  );

  case PT_ISOMETRIC_BOTTOM:
    return ANGLE3D((ANGLE)ANGLE_0  ,(ANGLE)ANGLE_90 ,(ANGLE)ANGLE_0  );

  default:
    ASSERT(FALSE);
    return ANGLE3D((ANGLE)ANGLE_0  ,(ANGLE)ANGLE_0  ,(ANGLE)ANGLE_0  );
  }
}
 
/*
 * Get zoom factor for the viewer.
 */
FLOAT CSlaveViewer::GetZoomFactor(void)
{
  // calculate the zoom factor that as ratio of size in pixels and size in meters 
  // for target object
  FLOAT fScreenX = (float)sv_pdpDrawPort->GetWidth();
  ANGLE aHalfI = AngleDeg(90.0f)/2;     // assume FOV of 90 degrees for perspective
  FLOAT fPerspectiveRatio = fScreenX/(2.0f*Tan(aHalfI));
  FLOAT fZoomFactor = fPerspectiveRatio/sv_fTargetDistance;
  ASSERT( fZoomFactor > 0);
  return fZoomFactor;
}

/*
 * Get distance for wanted zoom factor for the viewer.
 */
FLOAT CSlaveViewer::GetDistanceForZoom(FLOAT fZoom)
{
  // calculate the zoom factor that as ratio of size in pixels and size in meters 
  // for target object
  FLOAT fScreenX = (float)sv_pdpDrawPort->GetWidth();
  ANGLE aHalfI = AngleDeg(90.0f)/2;     // assume FOV of 90 degrees for perspective
  FLOAT fPerspectiveRatio = fScreenX/(2.0f*Tan(aHalfI));
  FLOAT fDistance = fPerspectiveRatio / fZoom;
  ASSERT( fDistance > 0);
  return fDistance;
}

// get placement of the virtual target
CPlacement3D CSlaveViewer::GetTargetPlacement(void) const
{
  CPlacement3D plTarget;
  // copy viewer to target
  plTarget = sv_plViewer;
  // get the direction vector of viewer's sight
  FLOAT3D vDirection;
  AnglesToDirectionVector(sv_plViewer.pl_OrientationAngle, vDirection);
  // offset the target placement by target distance along the vector
  plTarget.pl_PositionVector += vDirection*sv_fTargetDistance;

  // return the target placement
  return plTarget;
}

/*
 * Default constructor
 */
CSlaveViewer::CSlaveViewer(const CMasterViewer &mvMaster, 
                           enum ProjectionType ptProjectionType,
                           const CPlacement3D &plGrid,
                           CDrawPort *pdpDrawPort)
{
  // remember the drawport
  sv_pdpDrawPort = pdpDrawPort;

  // set projection type
  sv_ProjectionType = ptProjectionType;
  // set grid placement
  sv_plGrid = plGrid;
  // copy target distance
  sv_fTargetDistance = mvMaster.mv_fTargetDistance;

  // if is perspective
  if( IsPerspective())
  {
    // copy viewer placement
    sv_plViewer = mvMaster.mv_plViewer;
  }
  // if is isometric
  else
  {
    // copy target position to slave viewer position
    sv_plViewer.pl_PositionVector = mvMaster.GetTargetPlacement().pl_PositionVector;
    // get the orientation depending on the isometry type
    sv_plViewer.pl_OrientationAngle = GetAngleForIsometricType();

    // transform to grid system
    sv_plViewer.AbsoluteToRelative(sv_plGrid);

    // get the ray of the viewing direction
    FLOAT3D vDirection;
    AnglesToDirectionVector(sv_plViewer.pl_OrientationAngle, vDirection);
    // offset the viewer backward by target distance along the vector
    sv_plViewer.pl_PositionVector -= vDirection*sv_fTargetDistance;

    // transform back from grid system
    sv_plViewer.RelativeToAbsolute(sv_plGrid);
  }
}

/*
 * create a perspective projection for this viewer
 */
void CSlaveViewer::MakePerspectiveProjection(CPerspectiveProjection3D &prPerspectiveProjection)
{
  // if is perspective
  ASSERT( IsPerspective());

  // init projection parameters
  prPerspectiveProjection.FOVL() = AngleDeg(90.0f);
  prPerspectiveProjection.ScreenBBoxL() = FLOATaabbox2D(
    FLOAT2D(0.0f, 0.0f),
    FLOAT2D((float)sv_pdpDrawPort->GetWidth(), (float)sv_pdpDrawPort->GetHeight())
  );
  wed_fFrontClipDistance = Clamp( wed_fFrontClipDistance, 0.02f, 2.00f);
  prPerspectiveProjection.FrontClipDistanceL() = wed_fFrontClipDistance;
  prPerspectiveProjection.AspectRatioL() = 1.0f;
  prPerspectiveProjection.ObjectStretchL() = FLOAT3D(1.0f, 1.0f, 1.0f);

  // set up viewer position and placement
  prPerspectiveProjection.ViewerPlacementL() = sv_plViewer;
  prPerspectiveProjection.ObjectPlacementL().pl_PositionVector = FLOAT3D(0.0f, 0.0f, 0.0f);
  prPerspectiveProjection.ObjectPlacementL().pl_OrientationAngle = ANGLE3D(0, 0, 0);
}

/*
 * create a projection for this viewer
 */
void CSlaveViewer::MakeProjection(CAnyProjection3D &prProjection)
{
  // if is perspective
  if( IsPerspective()) {
    // use perspective projection
    CPerspectiveProjection3D prPerspectiveProjection;
    // make perspective projection
    MakePerspectiveProjection( prPerspectiveProjection);
    // return the result
    prProjection = prPerspectiveProjection;
  // if is isometric
  } else {
    // use isometric projection
    CIsometricProjection3D prIsometricProjection;
    
    // init projection parameters
    prIsometricProjection.ZoomFactorL() = GetZoomFactor();
    prIsometricProjection.ScreenBBoxL() = FLOATaabbox2D(
      FLOAT2D(0.0f, 0.0f),
      FLOAT2D((float)sv_pdpDrawPort->GetWidth(), (float)sv_pdpDrawPort->GetHeight())
    );
    wed_fFrontClipDistance = Clamp( wed_fFrontClipDistance, 0.02f, 2.00f);
    prIsometricProjection.AspectRatioL() = 1.0f;
    prIsometricProjection.FrontClipDistanceL() = wed_fFrontClipDistance;
    prIsometricProjection.ObjectStretchL() = FLOAT3D(1.0f, 1.0f, 1.0f);

    // set up viewer position and placement
    prIsometricProjection.ViewerPlacementL() = sv_plViewer;
    prIsometricProjection.ObjectPlacementL().pl_PositionVector = FLOAT3D(0.0f, 0.0f, 0.0f);
    prIsometricProjection.ObjectPlacementL().pl_OrientationAngle = ANGLE3D(0, 0, 0);

    // return the result
    prProjection = prIsometricProjection;
  }
}


/*
 * Translate slave viewer in his own system
 */
void CSlaveViewer::Translate_OwnSystem( PIX pixDI, PIX pixDJ, PIX pixDK)
{
  FLOAT fZoom = GetZoomFactor();
  sv_fTargetDistance += pixDK/fZoom;

  if( sv_fTargetDistance < MIN_TARGET_DISTANCE)
  {
    sv_fTargetDistance = MIN_TARGET_DISTANCE;
    return;
  }
  if( sv_fTargetDistance > MAX_TARGET_DISTANCE)
  {
    sv_fTargetDistance = MAX_TARGET_DISTANCE;
    return;
  }
  ASSERT( sv_fTargetDistance > 0);

  sv_plViewer.Translate_OwnSystem( FLOAT3D(-pixDI/fZoom, pixDJ/fZoom, pixDK/fZoom));
}

/*
 * Translate slave viewer without stretching zoom factor
 */
void CSlaveViewer::Translate_Local_OwnSystem(FLOAT fdX, FLOAT fdY, FLOAT fdZ)
{
  sv_plViewer.Translate_OwnSystem( FLOAT3D(fdX, fdY, fdZ));
}

/*
 * Scales target distance using given factor
 */
void CSlaveViewer::ScaleTargetDistance( FLOAT fFactor)
{
  // if very close to the target, and trying to go even closer or
  // if very far away and trying to move further away
  if( (sv_fTargetDistance <MIN_TARGET_DISTANCE && fFactor<1.0f) ||
      (sv_fTargetDistance >MAX_TARGET_DISTANCE && fFactor>1.0f) )
  {
    // don't do anything
    return;
  }
  // change along z axis
  FLOAT dz = sv_fTargetDistance * fFactor;
  sv_plViewer.Translate_OwnSystem( FLOAT3D( 0.0f, 0.0f, dz));
  sv_fTargetDistance += dz;
}

/*
 * Rotate slave viewer arround target using HPB method
 */
void CSlaveViewer::Rotate_HPB( PIX pixDI, PIX pixDJ, PIX pixDK)
{
  // if is perspective
  if( IsPerspective())
  {
    // get target placement
    CPlacement3D plTarget = GetTargetPlacement();
    // project viewer placement to the target placement
    sv_plViewer.AbsoluteToRelative(plTarget);
    // rotate the target
    plTarget.Rotate_HPB( ANGLE3D( AngleDeg((FLOAT)-pixDI),
                                  AngleDeg((FLOAT)-pixDJ),
                                  AngleDeg((FLOAT)-pixDK)));
    // project viewer placement back from the target placement
    sv_plViewer.RelativeToAbsolute(plTarget);
  }
  // if is isometric
  else
  {
    // can't rotate isometric projections
  }
}

/*
 * Rotate slave viewer in his own system using HPB method
 */
void CSlaveViewer::Rotate_Local_HPB( FLOAT fdX, FLOAT fdY, FLOAT fdZ)
{
  // if is perspective
  if( IsPerspective())
  {
    // rotate the viewer
    sv_plViewer.Rotate_HPB( ANGLE3D( AngleDeg(-fdX), AngleDeg(-fdY), AngleDeg(-fdZ)));
  }
  // if is isometric
  else
  {
    // can't rotate isometric projections
  }
}


// translate a placement in given system
void CSlaveViewer::TranslatePlacement_OtherSystem(CPlacement3D &plToTranslate,
                                                  CPlacement3D &plOtherSystem,
                                                  PIX pixDI, PIX pixDJ, PIX pixDK)
{
  FLOAT fZoom = GetZoomFactor();

  // project the placement to the viewer's system
  plToTranslate.AbsoluteToRelative(sv_plViewer);
  // project the placement to the given system
  plToTranslate.AbsoluteToRelative(plOtherSystem);
  // translate it
  plToTranslate.Translate_AbsoluteSystem( FLOAT3D(pixDI/fZoom, -pixDJ/fZoom, pixDK/fZoom));
  // project the placement back from given system
  plToTranslate.RelativeToAbsolute(plOtherSystem);
  // project the placement back from viewer's system
  plToTranslate.RelativeToAbsolute(sv_plViewer);
}

// translate a placement in viewer's system
void CSlaveViewer::TranslatePlacement_OwnSystem(CPlacement3D &plToTranslate,
    PIX pixDI, PIX pixDJ, PIX pixDK)
{
  FLOAT fZoom = GetZoomFactor();

  // project the placement to the viewer's system
  plToTranslate.AbsoluteToRelative(sv_plViewer);
  // translate it
  plToTranslate.Translate_AbsoluteSystem( FLOAT3D(pixDI/fZoom, -pixDJ/fZoom, pixDK/fZoom));
  // project the placement back from viewer's system
  plToTranslate.RelativeToAbsolute(sv_plViewer);
}

// convert offset from pixels to meters
FLOAT CSlaveViewer::PixelsToMeters(PIX pix)
{
  FLOAT fZoom = GetZoomFactor();
  return pix/fZoom;
}

// translate a placement in viewer's system
void CSlaveViewer::RotatePlacement_HPB(CPlacement3D &plToRotate,
    PIX pixDI, PIX pixDJ, PIX pixDK)
{
  // project the placement to the viewer's system
  plToRotate.AbsoluteToRelative(sv_plViewer);
  // rotate it
  plToRotate.Rotate_HPB( ANGLE3D( AngleDeg((FLOAT)-pixDI),
                                  AngleDeg((FLOAT)-pixDJ),
                                  AngleDeg((FLOAT)-pixDK)));
  // project the placement back from viewer's system
  plToRotate.RelativeToAbsolute(sv_plViewer);
}

// translate a placement in viewer's system
void CSlaveViewer::RotatePlacement_TrackBall(CPlacement3D &plToRotate,
    PIX pixDI, PIX pixDJ, PIX pixDK)
{
  // if viewer is perspective
  if( IsPerspective()) {
    // project the placement to the viewer's system
    plToRotate.AbsoluteToRelative(sv_plViewer);
    // rotate it
    plToRotate.Rotate_TrackBall( ANGLE3D( AngleDeg((FLOAT)-pixDI),
                                          AngleDeg((FLOAT)-pixDJ),
                                          AngleDeg((FLOAT)-pixDK)));
    // project the placement back from viewer's system
    plToRotate.RelativeToAbsolute(sv_plViewer);

  // if viewer is isometric
  } else {
    // project the placement to the viewer's system
    plToRotate.AbsoluteToRelative(sv_plViewer);
    // rotate it only around banking axis
    plToRotate.Rotate_TrackBall( ANGLE3D( 0,0, AngleDeg((FLOAT)-pixDI)));
    // project the placement back from viewer's system
    plToRotate.RelativeToAbsolute(sv_plViewer);
  }
}
