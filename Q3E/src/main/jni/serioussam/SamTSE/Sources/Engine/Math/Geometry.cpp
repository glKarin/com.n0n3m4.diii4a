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

#include <Engine/Math/Geometry.h>

#include <Engine/Math/Functions.h>

/////////////////////////////////////////////////////////////////////
//
//      General functions
//
/////////////////////////////////////////////////////////////////////

/*
 * Calculate rotation matrix from angles in 3D.
 */
/*void operator^=(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles)
{
  MakeRotationMatrix(t3dRotation, a3dAngles);
}
*/

void MakeRotationMatrix(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles)
{
  FLOAT fSinH = Sin(a3dAngles(1));  // heading
  FLOAT fCosH = Cos(a3dAngles(1));
  FLOAT fSinP = Sin(a3dAngles(2));  // pitch
  FLOAT fCosP = Cos(a3dAngles(2));
  FLOAT fSinB = Sin(a3dAngles(3));  // banking
  FLOAT fCosB = Cos(a3dAngles(3));

  t3dRotation(1,1) = fCosH*fCosB+fSinP*fSinH*fSinB;
  t3dRotation(1,2) = fSinP*fSinH*fCosB-fCosH*fSinB;
  t3dRotation(1,3) = fCosP*fSinH;
  t3dRotation(2,1) = fCosP*fSinB;
  t3dRotation(2,2) = fCosP*fCosB;
  t3dRotation(2,3) = -fSinP;
  t3dRotation(3,1) = fSinP*fCosH*fSinB-fSinH*fCosB;
  t3dRotation(3,2) = fSinP*fCosH*fCosB+fSinH*fSinB;
  t3dRotation(3,3) = fCosP*fCosH;
}
void MakeRotationMatrixFast(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles)
{
  FLOAT fSinH = SinFast(a3dAngles(1));  // heading
  FLOAT fCosH = CosFast(a3dAngles(1));
  FLOAT fSinP = SinFast(a3dAngles(2));  // pitch
  FLOAT fCosP = CosFast(a3dAngles(2));
  FLOAT fSinB = SinFast(a3dAngles(3));  // banking
  FLOAT fCosB = CosFast(a3dAngles(3));

  t3dRotation(1,1) = fCosH*fCosB+fSinP*fSinH*fSinB;
  t3dRotation(1,2) = fSinP*fSinH*fCosB-fCosH*fSinB;
  t3dRotation(1,3) = fCosP*fSinH;
  t3dRotation(2,1) = fCosP*fSinB;
  t3dRotation(2,2) = fCosP*fCosB;
  t3dRotation(2,3) = -fSinP;
  t3dRotation(3,1) = fSinP*fCosH*fSinB-fSinH*fCosB;
  t3dRotation(3,2) = fSinP*fCosH*fCosB+fSinH*fSinB;
  t3dRotation(3,3) = fCosP*fCosH;
}

/*
 * Calculate inverse rotation matrix from angles in 3D.
 */
/*void operator!=(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles)
{
  MakeInverseRotationMatrix(t3dRotation, a3dAngles);
}
*/
void MakeInverseRotationMatrix(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles)
{
  FLOAT fSinH = Sin(a3dAngles(1));  // heading
  FLOAT fCosH = Cos(a3dAngles(1));
  FLOAT fSinP = Sin(a3dAngles(2));  // pitch
  FLOAT fCosP = Cos(a3dAngles(2));
  FLOAT fSinB = Sin(a3dAngles(3));  // banking
  FLOAT fCosB = Cos(a3dAngles(3));

  // to make inverse of rotation matrix, we only need to transpose it
  t3dRotation(1,1) = fCosH*fCosB+fSinP*fSinH*fSinB;
  t3dRotation(2,1) = fSinP*fSinH*fCosB-fCosH*fSinB;
  t3dRotation(3,1) = fCosP*fSinH;
  t3dRotation(1,2) = fCosP*fSinB;
  t3dRotation(2,2) = fCosP*fCosB;
  t3dRotation(3,2) = -fSinP;
  t3dRotation(1,3) = fSinP*fCosH*fSinB-fSinH*fCosB;
  t3dRotation(2,3) = fSinP*fCosH*fCosB+fSinH*fSinB;
  t3dRotation(3,3) = fCosP*fCosH;
}
void MakeInverseRotationMatrixFast(FLOATmatrix3D &t3dRotation, const ANGLE3D &a3dAngles)
{
  FLOAT fSinH = SinFast(a3dAngles(1));  // heading
  FLOAT fCosH = CosFast(a3dAngles(1));
  FLOAT fSinP = SinFast(a3dAngles(2));  // pitch
  FLOAT fCosP = CosFast(a3dAngles(2));
  FLOAT fSinB = SinFast(a3dAngles(3));  // banking
  FLOAT fCosB = CosFast(a3dAngles(3));

  // to make inverse of rotation matrix, we only need to transpose it
  t3dRotation(1,1) = fCosH*fCosB+fSinP*fSinH*fSinB;
  t3dRotation(2,1) = fSinP*fSinH*fCosB-fCosH*fSinB;
  t3dRotation(3,1) = fCosP*fSinH;
  t3dRotation(1,2) = fCosP*fSinB;
  t3dRotation(2,2) = fCosP*fCosB;
  t3dRotation(3,2) = -fSinP;
  t3dRotation(1,3) = fSinP*fCosH*fSinB-fSinH*fCosB;
  t3dRotation(2,3) = fSinP*fCosH*fCosB+fSinH*fSinB;
  t3dRotation(3,3) = fCosP*fCosH;
}

/*
 * Decompose rotation matrix into angles in 3D.
 */
// NOTE: for derivation of the algorithm, see mathlib.doc
void DecomposeRotationMatrixNoSnap(ANGLE3D &a3dAngles, const FLOATmatrix3D &t3dRotation)
{
  ANGLE &h=a3dAngles(1);  // heading
  ANGLE &p=a3dAngles(2);  // pitch
  ANGLE &b=a3dAngles(3);  // banking
  FLOAT a;            // temporary

  // calculate pitch
  FLOAT f23 = t3dRotation(2,3);
  p = ASin(-f23);
  a = Sqrt(1.0f-f23*f23);

  // if pitch makes banking beeing the same as heading
  if (a<0.001) {
    // we choose to have banking of 0
    b = 0;
    // and calculate heading for that
    ASSERT(Abs(t3dRotation(2,3))>0.5); // must be around 1, what is far from 0
    h = ATan2(t3dRotation(1,2)/(-t3dRotation(2,3)), t3dRotation(1,1));  // no division by 0
  // otherwise
  } else {
    // calculate banking and heading normally
    b = ATan2(t3dRotation(2,1), t3dRotation(2,2));
    h = ATan2(t3dRotation(1,3), t3dRotation(3,3));
  }
}

void DecomposeRotationMatrix(ANGLE3D &a3dAngles, const FLOATmatrix3D &t3dRotation)
{
  // decompose the matrix without snapping
  DecomposeRotationMatrixNoSnap(a3dAngles, t3dRotation);
  // snap angles to compensate for errors when converting to and from matrix notation
  Snap(a3dAngles(1), ANGLE_SNAP);
  Snap(a3dAngles(2), ANGLE_SNAP);
  Snap(a3dAngles(3), ANGLE_SNAP);
}

/*void operator^=(ANGLE3D &a3dAngles, const FLOATmatrix3D &t3dRotation) {
  DecomposeRotationMatrix(a3dAngles, t3dRotation);
}
*/

/*
 * Create direction vector from angles in 3D (ignoring banking).
 */
void AnglesToDirectionVector(const ANGLE3D &a3dAngles, FLOAT3D &vDirection)
{
  // find the rotation matrix from the angles
  FLOATmatrix3D mDirection;
  MakeRotationMatrix(mDirection, a3dAngles);
  // rotate a front oriented vector by the matrix
  vDirection = FLOAT3D(0.0f, 0.0f, -1.0f)*mDirection;
}

/*
 * Create angles in 3D from direction vector(ignoring banking).
 */
void DirectionVectorToAnglesNoSnap(const FLOAT3D &vDirection, ANGLE3D &a3dAngles)
{
  // now calculate the angles
  ANGLE &h = a3dAngles(1);
  ANGLE &p = a3dAngles(2);
  ANGLE &b = a3dAngles(3);

  const FLOAT &x = vDirection(1);
  const FLOAT &y = vDirection(2);
  const FLOAT &z = vDirection(3);

  // banking is always irrelevant
  b = 0;
  // calculate pitch
  p = ASin(y);

  // if y is near +1 or -1
  if (y>0.99 || y<-0.99) {
    // heading is irrelevant
    h = 0;
  // otherwise
  } else {
    // calculate heading
    h = ATan2(-x, -z);
  }
}
void DirectionVectorToAngles(const FLOAT3D &vDirection, ANGLE3D &a3dAngles)
{
  DirectionVectorToAnglesNoSnap(vDirection, a3dAngles);

  // snap angles to compensate for errors when converting to and from vector notation
  Snap(a3dAngles(1), ANGLE_SNAP);
  Snap(a3dAngles(2), ANGLE_SNAP);
  Snap(a3dAngles(3), ANGLE_SNAP);
}

/* Create angles in 3D from up vector (ignoring objects relative heading).
   (up vector must be normalized!)*/
void UpVectorToAngles(const FLOAT3D &vY, ANGLE3D &a3dAngles)
{
  // create any front vector
  FLOAT3D vZ;
  if (Abs(vY(2))>0.5f) {
    vZ = FLOAT3D(1,0,0)*vY;
  } else {
    vZ = FLOAT3D(0,1,0)*vY;
  }
  vZ.Normalize();
  // side vector is cross product
  FLOAT3D vX = vY*vZ;
  vX.Normalize();
  // create the rotation matrix
  FLOATmatrix3D m;
  m(1,1) = vX(1); m(1,2) = vY(1); m(1,3) = vZ(1);
  m(2,1) = vX(2); m(2,2) = vY(2); m(2,3) = vZ(2);
  m(3,1) = vX(3); m(3,2) = vY(3); m(3,3) = vZ(3);

  // decompose the matrix without snapping
  DecomposeRotationMatrixNoSnap(a3dAngles, m);
}

/*
 * Calculate rotation matrix from angles in 3D.
 */
void operator^=(DOUBLEmatrix3D &t3dRotation, const ANGLE3D &a3dAngles) {
  const ANGLE &h=a3dAngles(1);  // heading
  const ANGLE &p=a3dAngles(2);  // pitch
  const ANGLE &b=a3dAngles(3);  // banking

  t3dRotation(1,1) = Cos(h)*Cos(b)+Sin(p)*Sin(h)*Sin(b);
  t3dRotation(1,2) = Sin(p)*Sin(h)*Cos(b)-Cos(h)*Sin(b);
  t3dRotation(1,3) = Cos(p)*Sin(h);
  t3dRotation(2,1) = Cos(p)*Sin(b);
  t3dRotation(2,2) = Cos(p)*Cos(b);
  t3dRotation(2,3) = -Sin(p);
  t3dRotation(3,1) = Sin(p)*Cos(h)*Sin(b)-Sin(h)*Cos(b);
  t3dRotation(3,2) = Sin(p)*Cos(h)*Cos(b)+Sin(h)*Sin(b);
  t3dRotation(3,3) = Cos(p)*Cos(h);
}

/*
 * Calculate inverse rotation matrix from angles in 3D.
 */
void operator!=(DOUBLEmatrix3D &t3dRotation, const ANGLE3D &a3dAngles) {
  const ANGLE &h=a3dAngles(1);  // heading
  const ANGLE &p=a3dAngles(2);  // pitch
  const ANGLE &b=a3dAngles(3);  // banking

  // to make inverse of rotation matrix, we only need to transpose it
  t3dRotation(1,1) = Cos(h)*Cos(b)+Sin(p)*Sin(h)*Sin(b);
  t3dRotation(2,1) = Sin(p)*Sin(h)*Cos(b)-Cos(h)*Sin(b);
  t3dRotation(3,1) = Cos(p)*Sin(h);
  t3dRotation(1,2) = Cos(p)*Sin(b);
  t3dRotation(2,2) = Cos(p)*Cos(b);
  t3dRotation(3,2) = -Sin(p);
  t3dRotation(1,3) = Sin(p)*Cos(h)*Sin(b)-Sin(h)*Cos(b);
  t3dRotation(2,3) = Sin(p)*Cos(h)*Cos(b)+Sin(h)*Sin(b);
  t3dRotation(3,3) = Cos(p)*Cos(h);
}

/*
 * Decompose rotation matrix into angles in 3D.
 */
// NOTE: for derivation of the algorithm, see mathlib.doc
void operator^=(ANGLE3D &a3dAngles, const DOUBLEmatrix3D &t3dRotation) {
  ANGLE &h=a3dAngles(1);  // heading
  ANGLE &p=a3dAngles(2);  // pitch
  ANGLE &b=a3dAngles(3);  // banking
  DOUBLE a;            // temporary

  // calculate pitch
  p = ASin(-t3dRotation(2,3));
  a = sqrt(1-t3dRotation(2,3)*t3dRotation(2,3));

  // if pitch makes banking beeing the same as heading
  if (a<0.0001) {
    // we choose to have banking of 0
    b = 0;
    // and calculate heading for that
    ASSERT(Abs(t3dRotation(2,3))>0.5); // must be around 1, what is far from 0
    h = ATan2(t3dRotation(1,2)/(-t3dRotation(2,3)), t3dRotation(1,1));  // no division by 0
  // otherwise
  } else {
    // calculate banking and heading normally
    b = ATan2(t3dRotation(2,1)/a, t3dRotation(2,2)/a);
    h = ATan2(t3dRotation(1,3)/a, t3dRotation(3,3)/a);
  }
  // snap angles to compensate for errors when converting to and from matrix notation
  Snap(h, ANGLE_SNAP);
  Snap(p, ANGLE_SNAP);
  Snap(b, ANGLE_SNAP);
}

