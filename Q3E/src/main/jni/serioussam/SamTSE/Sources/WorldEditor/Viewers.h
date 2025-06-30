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

#ifndef VIEWERS_H
#define VIEWERS_H

/*
 * Definition of viewer and target placement
 */
class CMasterViewer {
friend class CSlaveViewer;
public:
  CPlacement3D mv_plViewer;           // placement of the viewer
  FLOAT mv_fTargetDistance;           // distance of virtual target from viewer
  // default constructor
  CMasterViewer(void);
  // get placement of the viewer 
  inline CPlacement3D GetViewerPlacement(void) const {return mv_plViewer;};
  // get placement of the virtual target
  CPlacement3D GetTargetPlacement(void) const;
  // set placement of the virtual target
  void SetTargetPlacement(FLOAT3D f3dTarget);
  // convert from slave
  void operator = (const CSlaveViewer &svSlave);
};

/*
 * Definition of viewing projection viewer placement
 */
class CSlaveViewer {
friend class CMasterViewer;
public:
  enum ProjectionType {
    PT_ILLEGAL = 0,
    PT_PERSPECTIVE,
    PT_ISOMETRIC_FRONT,
    PT_ISOMETRIC_RIGHT,
    PT_ISOMETRIC_TOP,
    PT_ISOMETRIC_BACK,
    PT_ISOMETRIC_LEFT,
    PT_ISOMETRIC_BOTTOM,
  };
  CDrawPort *sv_pdpDrawPort;          // drawport that this viewer was created for
  CPlacement3D sv_plViewer;           // placement of the viewer
  CPlacement3D sv_plGrid;             // grid's placement
  FLOAT sv_fTargetDistance;           // distance of virtual target from viewer
  enum ProjectionType sv_ProjectionType; // tyoe of projection
  // get orientation angles for type of isometric viewer
  ANGLE3D GetAngleForIsometricType(void) const;

public:

  // default constructor
  CSlaveViewer( const CMasterViewer &mvMaster, enum ProjectionType ptProjectionType,
                const CPlacement3D &plGrid, CDrawPort *pdpDrawPort);
  // create a projection for this viewer
  void MakeProjection(CAnyProjection3D &prProjection);
  // create a perspective projection for this viewer
  void MakePerspectiveProjection(CPerspectiveProjection3D &prPerspectiveProjection);

  // get placement of the viewer 
  inline CPlacement3D GetViewerPlacement(void) const {return sv_plViewer;};
  // test if this is perspective viewer
  inline BOOL IsPerspective(void) const { ASSERT( sv_ProjectionType != PT_ILLEGAL);
                                   return sv_ProjectionType == PT_PERSPECTIVE; };
  // test if this is isometric viewer
  inline BOOL IsIsometric(void) const { ASSERT( sv_ProjectionType != PT_ILLEGAL);
                                 return sv_ProjectionType != PT_PERSPECTIVE; };
                                 
  /* Get zoom factor for the viewer. */
  FLOAT GetZoomFactor(void);
  /* Get distance for requested zoom factor for the viewer. */
  FLOAT GetDistanceForZoom(FLOAT fZoom);

  // get target distance
  inline FLOAT GetTargetDistance( void) const { return sv_fTargetDistance; };

  // get placement of the virtual target
  CPlacement3D GetTargetPlacement(void) const;

  // translate slave viewer in his own system
  void Translate_OwnSystem( PIX pixDI, PIX pixDJ, PIX pixDK);
  void TranslatePlacement_OtherSystem(CPlacement3D &plToTranslate,CPlacement3D &plOtherSystem,
    PIX pixDI, PIX pixDJ, PIX pixDK);
  // scales target distance using given factor
  void ScaleTargetDistance( FLOAT fFactor);
  // rotate slave viewer in his own system using HPB method
  void Rotate_HPB( PIX pixDI, PIX pixDJ, PIX pixDK);

  // translate a placement in viewer's system
  void TranslatePlacement_OwnSystem(CPlacement3D &plToTranslate,
      PIX pixDI, PIX pixDJ, PIX pixDK);
  // rotate a placement in viewer's system
  void RotatePlacement_HPB(CPlacement3D &plToRotate,
      PIX pixDI, PIX pixDJ, PIX pixDK);
  // rotate a placement in viewer's system
  void RotatePlacement_TrackBall(CPlacement3D &plToRotate,
      PIX pixDI, PIX pixDJ, PIX pixDK);

  // convert offset from pixels to meters
  FLOAT PixelsToMeters(PIX pix);

  void Translate_Local_OwnSystem(FLOAT fdX, FLOAT fdY, FLOAT fdZ);
  void Rotate_Local_HPB( FLOAT fdX, FLOAT fdY, FLOAT fdZ);
};

#endif // VIEWERS_H
