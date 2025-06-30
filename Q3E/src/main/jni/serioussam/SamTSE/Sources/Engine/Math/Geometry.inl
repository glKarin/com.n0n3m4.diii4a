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

#ifndef SE_INCL_GEOMETRY_INL
#define SE_INCL_GEOMETRY_INL
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// mirror a position vector by a given plane
inline void ReflectPositionVectorByPlane(const FLOATplane3D &plPlane, FLOAT3D &vPoint)
{
  vPoint-=((FLOAT3D &)plPlane)*(2*plPlane.PointDistance(vPoint));
}
// mirror a direction vector by a given plane
inline void ReflectDirectionVectorByPlane(const FLOATplane3D &plPlane, FLOAT3D &vDirection)
{
  vDirection-=((FLOAT3D &)plPlane)*(2*(((FLOAT3D &)plPlane)%vDirection));
}
// mirror a rotation matrix by a given plane
inline void ReflectRotationMatrixByPlane_cols(const FLOATplane3D &plPlane, FLOATmatrix3D &m)
{ // reflect columns vectors
    FLOAT3D vX(m(1,1),m(2,1),m(3,1));
    FLOAT3D vY(m(1,2),m(2,2),m(3,2));
    FLOAT3D vZ(m(1,3),m(2,3),m(3,3));

    ReflectDirectionVectorByPlane(plPlane, vX);
    ReflectDirectionVectorByPlane(plPlane, vY);
    ReflectDirectionVectorByPlane(plPlane, vZ);

    m(1,1) = vX(1); m(1,2) = vY(1); m(1,3) = vZ(1);
    m(2,1) = vX(2); m(2,2) = vY(2); m(2,3) = vZ(2);
    m(3,1) = vX(3); m(3,2) = vY(3); m(3,3) = vZ(3);
}
inline void ReflectRotationMatrixByPlane_rows(const FLOATplane3D &plPlane, FLOATmatrix3D &m)
{ // reflect row vectors
    FLOAT3D vX(m(1,1),m(1,2),m(1,3));
    FLOAT3D vY(m(2,1),m(2,2),m(2,3));
    FLOAT3D vZ(m(3,1),m(3,2),m(3,3));

    ReflectDirectionVectorByPlane(plPlane, vX);
    ReflectDirectionVectorByPlane(plPlane, vY);
    ReflectDirectionVectorByPlane(plPlane, vZ);

    m(1,1) = vX(1); m(2,1) = vY(1); m(3,1) = vZ(1);
    m(1,2) = vX(2); m(2,2) = vY(2); m(3,2) = vZ(2);
    m(1,3) = vX(3); m(2,3) = vY(3); m(3,3) = vZ(3);
}

// get component of a vector parallel to given reference vector
static inline void GetParallelComponent(
  const FLOAT3D &vFull, const FLOAT3D &vReference, FLOAT3D &vParallel)
{
  vParallel = vReference*(vFull%vReference);
}

// get component of a vector normal to given reference vector
static inline void GetNormalComponent(
  const FLOAT3D &vFull, const FLOAT3D &vReference, FLOAT3D &vNormal)
{
  vNormal = vFull-vReference*(vFull%vReference);
}

// get components of a vector parallel and normal to given reference vector
static inline void GetParallelAndNormalComponents(
  const FLOAT3D &vFull, const FLOAT3D &vReference, FLOAT3D &vParallel, FLOAT3D &vNormal)
{
  vParallel = vReference*(vFull%vReference);
  vNormal = vFull-vParallel;
}

// get measure of validity of a rotation matrix (should be around zero)
static inline FLOAT RotationMatrixValidity(const FLOATmatrix3D &m)
{
  FLOATmatrix3D mSqr;
  mSqr(1,1) = m(1,1)*m(1,1);
  mSqr(1,2) = m(1,2)*m(1,2);
  mSqr(1,3) = m(1,3)*m(1,3);
  mSqr(2,1) = m(2,1)*m(2,1);
  mSqr(2,2) = m(2,2)*m(2,2);
  mSqr(2,3) = m(2,3)*m(2,3);
  mSqr(3,1) = m(3,1)*m(3,1);
  mSqr(3,2) = m(3,2)*m(3,2);
  mSqr(3,3) = m(3,3)*m(3,3);

  FLOAT3D vH;
  vH(1) = Sqrt(mSqr(1,1)+mSqr(1,2)+mSqr(1,3))-1;
  vH(2) = Sqrt(mSqr(2,1)+mSqr(2,2)+mSqr(2,3))-1;
  vH(3) = Sqrt(mSqr(3,1)+mSqr(3,2)+mSqr(3,3))-1;
  FLOAT3D vV;
  vV(1) = Sqrt(mSqr(1,1)+mSqr(2,1)+mSqr(3,1))-1;
  vV(2) = Sqrt(mSqr(1,2)+mSqr(2,2)+mSqr(3,2))-1;
  vV(3) = Sqrt(mSqr(1,3)+mSqr(2,3)+mSqr(3,3))-1;

  return Sqrt(
    vH(1)*vH(1)+vH(2)*vH(2)+vH(3)*vH(3)+
    vV(1)*vV(1)+vV(2)*vV(2)+vV(3)*vV(3));
}

// normalize rotation matrix to be special orthogonal
static inline void OrthonormalizeRotationMatrix( FLOATmatrix3D &m)
{
  FLOAT3D vX(m(1,1),m(2,1),m(3,1));
  FLOAT3D vY(m(1,2),m(2,2),m(3,2));
  FLOAT3D vZ;

  vX.Normalize();
  vZ = vX*vY;
  vZ.Normalize();
  vY = vZ*vX;
  vY.Normalize();

  m(1,1) = vX(1); m(1,2) = vY(1); m(1,3) = vZ(1);
  m(2,1) = vX(2); m(2,2) = vY(2); m(2,3) = vZ(2);
  m(3,1) = vX(3); m(3,2) = vY(3); m(3,3) = vZ(3);
}

inline void GetMajorAxesForPlane(
  const FLOATplane3D &plPlane, INDEX &iMajorAxis1, INDEX &iMajorAxis2)
{
  // get maximum normal axis
  INDEX iMaxNormalAxis = plPlane.GetMaxNormal();
  // the major axes are the other two axes
  switch (iMaxNormalAxis) {
  case 1: iMajorAxis1 = 2; iMajorAxis2 = 3;
    break;
  case 2: iMajorAxis1 = 3; iMajorAxis2 = 1;
    break;
  case 3: iMajorAxis1 = 1; iMajorAxis2 = 2;
    break;
  default:
    ASSERT(FALSE);
    iMajorAxis1 = 2;
    iMajorAxis2 = 3;
  }
  ASSERT(Abs(plPlane(iMaxNormalAxis))>=Abs(plPlane(iMajorAxis1))
       &&Abs(plPlane(iMaxNormalAxis))>=Abs(plPlane(iMajorAxis2)));
}

#endif /* include-once checker ... */

