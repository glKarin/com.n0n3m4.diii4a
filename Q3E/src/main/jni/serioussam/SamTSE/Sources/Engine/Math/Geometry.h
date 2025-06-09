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

#ifndef SE_INCL_GEOMETRY_H
#define SE_INCL_GEOMETRY_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Math/Vector.h>
#include <Engine/Math/Plane.h>
#include <Engine/Math/Matrix.h>

/*
 * General geometry functions
 */

/* Calculate rotation matrix from angles in 3D. */
//ENGINE_API extern void operator^=(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles);
ENGINE_API extern void MakeRotationMatrix(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles);
ENGINE_API extern void MakeRotationMatrixFast(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles);
/* Calculate inverse rotation matrix from angles in 3D. */
//ENGINE_API extern void operator!=(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles);
ENGINE_API extern void MakeInverseRotationMatrix(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles);
ENGINE_API extern void MakeInverseRotationMatrixFast(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles);
/* Decompose rotation matrix into angles in 3D. */
//ENGINE_API extern void operator^=(ANGLE3D &a3dAngles, const FLOATmatrix3D &t3dRotation);
ENGINE_API extern void DecomposeRotationMatrix(ANGLE3D &a3dAngles, const FLOATmatrix3D &t3dRotation);
ENGINE_API extern void DecomposeRotationMatrixNoSnap(ANGLE3D &a3dAngles, const FLOATmatrix3D &t3dRotation);

/* Create direction vector from angles in 3D (ignoring banking). */
ENGINE_API extern void AnglesToDirectionVector(const ANGLE3D &a3dAngles,
                                             FLOAT3D &vDirection);
/* Create angles in 3D from direction vector(ignoring banking).
   (direction must be normalized!)*/
ENGINE_API extern void DirectionVectorToAngles(const FLOAT3D &vDirection,
                                             ANGLE3D &a3dAngles);
ENGINE_API extern void DirectionVectorToAnglesNoSnap(const FLOAT3D &vDirection,
                                             ANGLE3D &a3dAngles);
/* Create angles in 3D from up vector (ignoring objects relative heading).
   (up vector must be normalized!)*/
ENGINE_API extern void UpVectorToAngles(const FLOAT3D &vUp,
                                             ANGLE3D &a3dAngles);


/* Calculate rotation matrix from angles in 3D. */
ENGINE_API extern void operator^=(DOUBLEmatrix3D &t3dRotation, const ANGLE3D &a3dAngles);
/* Calculate inverse rotation matrix from angles in 3D. */
ENGINE_API extern void operator!=(DOUBLEmatrix3D &t3dRotation, const ANGLE3D &a3dAngles);
/* Decompose rotation matrix into angles in 3D. */
ENGINE_API extern void operator^=(ANGLE3D &a3dAngles, const DOUBLEmatrix3D &t3dRotation);




#endif  /* include-once check. */

