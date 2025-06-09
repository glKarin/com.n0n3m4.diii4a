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

#include <Engine/Math/Placement.h>

#include <Engine/Base/Stream.h>
#include <Engine/Math/Geometry.h>
#include <Engine/Math/Projection.h>

// Stream operations

CTStream &operator>>(CTStream &strm, CPlacement3D &p3d)
{
  strm>>p3d.pl_PositionVector;
  strm>>p3d.pl_OrientationAngle;
  return strm;
}
CTStream &operator<<(CTStream &strm, const CPlacement3D &p3d)
{
  strm<<p3d.pl_PositionVector;
  strm<<p3d.pl_OrientationAngle;
  return strm;
}

/////////////////////////////////////////////////////////////////////
// transformations from between coordinate systems

/*
 * Project this placement from absolute system to coordinate system of another placement.
 */
void CPlacement3D::AbsoluteToRelative(const CPlacement3D &plSystem)
{
  // create a simple projection
  CSimpleProjection3D prSimple;
  // set the relative system as the viewer
  prSimple.ViewerPlacementL() = plSystem;
  // set the absolute system as object
  prSimple.ObjectPlacementL().pl_PositionVector = FLOAT3D(0.0f, 0.0f, 0.0f);
  prSimple.ObjectPlacementL().pl_OrientationAngle = ANGLE3D(0, 0, 0);
  // prepare the projection
  prSimple.Prepare();
  // project this placement using the projection
  prSimple.ProjectPlacement(*this, *this);
}
void CPlacement3D::AbsoluteToRelativeSmooth(const CPlacement3D &plSystem)
{
  // create a simple projection
  CSimpleProjection3D prSimple;
  // set the relative system as the viewer
  prSimple.ViewerPlacementL() = plSystem;
  // set the absolute system as object
  prSimple.ObjectPlacementL().pl_PositionVector = FLOAT3D(0.0f, 0.0f, 0.0f);
  prSimple.ObjectPlacementL().pl_OrientationAngle = ANGLE3D(0, 0, 0);
  // prepare the projection
  prSimple.Prepare();
  // project this placement using the projection
  prSimple.ProjectPlacementSmooth(*this, *this);
}

/*
 * Project this placement from coordinate system of another placement to absolute system.
 */
void CPlacement3D::RelativeToAbsolute(const CPlacement3D &plSystem)
{
  // create a simple projection
  CSimpleProjection3D prSimple;
  // set the absolute system as the viewer
  prSimple.ViewerPlacementL().pl_PositionVector = FLOAT3D(0.0f, 0.0f, 0.0f);
  prSimple.ViewerPlacementL().pl_OrientationAngle = ANGLE3D(0, 0, 0);
  // set the relative system as object
  prSimple.ObjectPlacementL() = plSystem;
  // prepare the projection
  prSimple.Prepare();
  // project this placement using the projection
  prSimple.ProjectPlacement(*this, *this);
}
void CPlacement3D::RelativeToAbsoluteSmooth(const CPlacement3D &plSystem)
{
  // create a simple projection
  CSimpleProjection3D prSimple;
  // set the absolute system as the viewer
  prSimple.ViewerPlacementL().pl_PositionVector = FLOAT3D(0.0f, 0.0f, 0.0f);
  prSimple.ViewerPlacementL().pl_OrientationAngle = ANGLE3D(0, 0, 0);
  // set the relative system as object
  prSimple.ObjectPlacementL() = plSystem;
  // prepare the projection
  prSimple.Prepare();
  // project this placement using the projection
  prSimple.ProjectPlacementSmooth(*this, *this);
}

/*
 * Project this placement from system of one placement to system of another placement.
 */
void CPlacement3D::RelativeToRelative(const CPlacement3D &plSource,
                                      const CPlacement3D &plTarget)
{
  // create a simple projection
  CSimpleProjection3D prSimple;
  // set the target system as the viewer
  prSimple.ViewerPlacementL() = plTarget;
  // set the source system as object
  prSimple.ObjectPlacementL() = plSource;
  // prepare the projection
  prSimple.Prepare();
  // project this placement using the projection
  prSimple.ProjectPlacement(*this, *this);
}
void CPlacement3D::RelativeToRelativeSmooth(const CPlacement3D &plSource,
                                      const CPlacement3D &plTarget)
{
  // create a simple projection
  CSimpleProjection3D prSimple;
  // set the target system as the viewer
  prSimple.ViewerPlacementL() = plTarget;
  // set the source system as object
  prSimple.ObjectPlacementL() = plSource;
  // prepare the projection
  prSimple.Prepare();
  // project this placement using the projection
  prSimple.ProjectPlacementSmooth(*this, *this);
}

/* Make this placement be a linear interpolation between given two placements. */
void CPlacement3D::Lerp(const CPlacement3D &pl0, const CPlacement3D &pl1, FLOAT fFactor)
{
  // lerp the position
  pl_PositionVector(1) =
    LerpFLOAT(pl0.pl_PositionVector(1), pl1.pl_PositionVector(1), fFactor);
  pl_PositionVector(2) =
    LerpFLOAT(pl0.pl_PositionVector(2), pl1.pl_PositionVector(2), fFactor);
  pl_PositionVector(3) =
    LerpFLOAT(pl0.pl_PositionVector(3), pl1.pl_PositionVector(3), fFactor);

  // lerp the orientation
  pl_OrientationAngle(1) =
    LerpANGLE(pl0.pl_OrientationAngle(1), pl1.pl_OrientationAngle(1), fFactor);
  pl_OrientationAngle(2) =
    LerpANGLE(pl0.pl_OrientationAngle(2), pl1.pl_OrientationAngle(2), fFactor);
  pl_OrientationAngle(3) =
    LerpANGLE(pl0.pl_OrientationAngle(3), pl1.pl_OrientationAngle(3), fFactor);
}

/////////////////////////////////////////////////////////////////////
// translations and rotations

/*
 * Rotate using trackball method.
 */
void CPlacement3D::Rotate_TrackBall(const ANGLE3D &a3dRotation)
{
  FLOATmatrix3D t3dRotation;    // matrix for the rotation angles
  FLOATmatrix3D t3dOriginal;      // matrix for the original angles

  // create matrices from angles
  MakeRotationMatrix(t3dRotation, a3dRotation);
  MakeRotationMatrix(t3dOriginal, pl_OrientationAngle);

  // make composed matrix
  t3dOriginal = t3dRotation*t3dOriginal;  // rotate first by original, then by rotation angles

  // recreate angles from composed matrix
  DecomposeRotationMatrix(pl_OrientationAngle, t3dOriginal);
}

/*
 * Rotate using airplane method.
 */
void CPlacement3D::Rotate_Airplane(const ANGLE3D &a3dRotation)
{
  FLOATmatrix3D t3dRotation;    // matrix for the rotation angles
  FLOATmatrix3D t3dOriginal;    // matrix for the original angles

  // create matrices from angles
  MakeRotationMatrixFast(t3dRotation, a3dRotation);
  MakeRotationMatrixFast(t3dOriginal, pl_OrientationAngle);

  // make composed matrix
  t3dOriginal = t3dOriginal*t3dRotation;  // rotate first by rotation, then by original angles

  // recreate angles from composed matrix
  DecomposeRotationMatrixNoSnap(pl_OrientationAngle, t3dOriginal);
}

/*
 * Rotate using HPB method.
 */
void CPlacement3D::Rotate_HPB(const ANGLE3D &a3dRotation)
{
  // just add the rotation angles to original angles
  pl_OrientationAngle += a3dRotation;
}

/*
 * Translate in own coordinate system.
 */
void CPlacement3D::Translate_OwnSystem(const FLOAT3D &f3dRelativeTranslation)
{
  FLOATmatrix3D t3dOwnAngles;             // matrix for own angles

  // make matrix from own angles
  MakeRotationMatrix(t3dOwnAngles, pl_OrientationAngle);
  // make absolute translation from relative by rotating for your own angles
  pl_PositionVector += f3dRelativeTranslation*t3dOwnAngles;
}

/*
 * Translate in absolute coordinate system.
 */
void CPlacement3D::Translate_AbsoluteSystem(const FLOAT3D &f3dAbsoluteTranslation)
{
  // just add the translation to the position vector
  pl_PositionVector += f3dAbsoluteTranslation;
}
