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

#ifndef SE_INCL_PLACEMENT_H
#define SE_INCL_PLACEMENT_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Math/Vector.h>
#include <Engine/Math/Geometry.h>


#ifdef NETSTRUCTS_PACKED
#pragma pack(1)
#endif

/*
 * Placement of an object in 3D space
 */
class ENGINE_API CPlacement3D {
public:
  FLOAT3D pl_PositionVector;    // position in space
  ANGLE3D pl_OrientationAngle;  // angles of orientation
public:
  /* Default constructor. */
  inline CPlacement3D(void);
  /* Constructor from coordinates and angle. */
  inline CPlacement3D(const FLOAT3D &vPosition, const ANGLE3D &aOrientation);
  /* Comparison operator. */
  inline BOOL operator==(const CPlacement3D &plOther) const;

  /* Rotate using trackball method. */
  void Rotate_TrackBall(const ANGLE3D &a3dRotation);
  /* Rotate using airplane method. */
  void Rotate_Airplane(const ANGLE3D &a3dRotation);
  /* Rotate using HPB method. */
  void Rotate_HPB(const ANGLE3D &a3dRotation);
  /* Translate in own coordinate system. */
  void Translate_OwnSystem(const FLOAT3D &f3dRelativeTranslation);
  /* Translate in absolute coordinate system. */
  void Translate_AbsoluteSystem(const FLOAT3D &f3dAbsoluteTranslation);

  /* Get the direction vector in the direction of this placement. */
  inline void GetDirectionVector(FLOAT3D &vDirection);

  /* Project this placement from absolute system to coordinate system of another placement. */
  void AbsoluteToRelative(const CPlacement3D &plSystem);
  void AbsoluteToRelativeSmooth(const CPlacement3D &plSystem);
  /* Project this placement from coordinate system of another placement to absolute system. */
  void RelativeToAbsolute(const CPlacement3D &plSystem);
  void RelativeToAbsoluteSmooth(const CPlacement3D &plSystem);
  /* Project this placement from system of one placement to system of another placement. */
  void RelativeToRelative(const CPlacement3D &plSource, const CPlacement3D &plTarget);
  void RelativeToRelativeSmooth(const CPlacement3D &plSource, const CPlacement3D &plTarget);

  /* Make this placement be a linear interpolation between given two placements. */
  void Lerp(const CPlacement3D &pl0, const CPlacement3D &pl1, FLOAT fFactor);
};

#ifdef NETSTRUCTS_PACKED
#pragma pack()
#endif

/* Stream operations */
ENGINE_API CTStream &operator>>(CTStream &strm, CPlacement3D &p3d);
ENGINE_API CTStream &operator<<(CTStream &strm, const CPlacement3D &p3d);
extern ENGINE_API CPlacement3D _plOrigin;

/* Default constructor. */
inline CPlacement3D::CPlacement3D(void) { }
/* Constructor from coordinates and angle. */
inline CPlacement3D::CPlacement3D(const FLOAT3D &vPosition, const ANGLE3D &aOrientation)
  : pl_PositionVector(vPosition)
  , pl_OrientationAngle(aOrientation)
{}
/* Comparison operator. */
inline BOOL CPlacement3D::operator==(const CPlacement3D &plOther) const {
  return pl_PositionVector==plOther.pl_PositionVector
    && pl_OrientationAngle==plOther.pl_OrientationAngle;
}
/* Get the direction vector in the direction of this placement. */
inline void CPlacement3D::GetDirectionVector(FLOAT3D &vDirection)
{
  AnglesToDirectionVector(pl_OrientationAngle, vDirection);
}


#endif  /* include-once check. */

