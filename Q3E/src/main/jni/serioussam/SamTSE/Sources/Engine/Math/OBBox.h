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

#ifndef SE_INCL_OBBOX_H
#define SE_INCL_OBBOX_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Math/Vector.h>
#include <Engine/Math/Matrix.h>
#include <Engine/Math/Functions.h>
#include <Engine/Math/Plane.h>

/*
 * Template for oriented bounding box of arbitrary type in 3D
 */
template<class Type>
class OBBox {
// implementation:
public:
  Vector<Type, 3> box_vO;          // center of the box
  Vector<Type, 3> box_avAxis[3];   // axis direction vectors
  Type box_atSize[3];              // size on each of the axis (in both directions)

  /* Clear to normalized empty bounding box. */
  inline void SetToNormalizedEmpty(void);
// interface:
public:
  /* Default constructor. */
  inline OBBox(void);
  /* Constructor from components. */
  inline OBBox(const Vector<Type, 3> &vO,
    const Vector<Type, 3> &vAxis0, const Vector<Type, 3> &vAxis1, const Vector<Type, 3> &vAxis2,
    Type tSize0, Type tSize1, Type tSize2);
  /* Constructor from axis aligned box and placement. */
  inline OBBox(const AABBox<Type, 3> &aabbox, 
    const Vector<Type, 3> &vPos, const Matrix<Type, 3, 3> &mRot);
  /* Constructor from axis aligned box without placement. */
  inline OBBox(const AABBox<Type, 3> &aabbox);

  // classify box with respect to a plane
  inline Type TestAgainstPlane(const Plane<Type, 3> &pl) const;
  // check if two boxes intersect/touch
  inline BOOL HasContactWith(const OBBox<Type> &boxB) const;

  /* Check if empty. */
  inline BOOL IsEmpty(void) const;
};

/*
 * Clear to normalized empty bounding box.
 */
template<class Type>
inline void OBBox<Type>::SetToNormalizedEmpty(void) {
  for ( int i=0; i<3; i++ ) {
    box_atSize[i] = LowerLimit(Type(0));
  }
}

/*
 * Constructor for empty bounding box.
 */
template<class Type>
inline OBBox<Type>::OBBox() {
  SetToNormalizedEmpty();
}

/* Constructor from axis aligned box and placement. */
template<class Type>
inline OBBox<Type>::OBBox(const AABBox<Type, 3> &aabbox, 
  const Vector<Type, 3> &vPos, const Matrix<Type, 3, 3> &mRot)
{
  // translate and rotate the center
  box_vO = aabbox.Center()*mRot+vPos;
  // extracted orientation from the rotation matrix
  box_avAxis[0](1) = mRot(1,1); box_avAxis[0](2) = mRot(2,1); box_avAxis[0](3) = mRot(3,1);
  box_avAxis[1](1) = mRot(1,2); box_avAxis[1](2) = mRot(2,2); box_avAxis[1](3) = mRot(3,2);
  box_avAxis[2](1) = mRot(1,3); box_avAxis[2](2) = mRot(2,3); box_avAxis[2](3) = mRot(3,3);

  // get sizes from obbox sizes
  box_atSize[0] = aabbox.Size()(1)*0.5f;
  box_atSize[1] = aabbox.Size()(2)*0.5f;
  box_atSize[2] = aabbox.Size()(3)*0.5f;
}
/* Constructor from axis aligned box without placement. */
template<class Type>
inline OBBox<Type>::OBBox(const AABBox<Type, 3> &aabbox)
{
  box_vO = aabbox.Center();
  box_avAxis[0] = Vector<Type, 3>(1,0,0);
  box_avAxis[1] = Vector<Type, 3>(0,1,0);
  box_avAxis[2] = Vector<Type, 3>(0,0,1);
  box_atSize[0] = aabbox.Size()(1)*0.5f;
  box_atSize[1] = aabbox.Size()(2)*0.5f;
  box_atSize[2] = aabbox.Size()(3)*0.5f;
}

/* Constructor from components. */
template<class Type>
inline OBBox<Type>::OBBox(const Vector<Type, 3> &vO,
  const Vector<Type, 3> &vAxis0, const Vector<Type, 3> &vAxis1, const Vector<Type, 3> &vAxis2,
  Type tSize0, Type tSize1, Type tSize2) {
  box_vO = vO;
  box_avAxis[0] = vAxis0; box_avAxis[1] = vAxis1; box_avAxis[2] = vAxis2;
  box_atSize[0] = tSize0; box_atSize[1] = tSize1; box_atSize[2] = tSize2;
};

/*
 * Check if empty.
 */
template<class Type>
inline BOOL OBBox<Type>::IsEmpty(void) const {
  // if any dimension is empty, it is empty
  for ( int i=0; i<3; i++ ) {
    if (box_atSize[i] < Type(0)) {
      return TRUE;
    }
  }
  // otherwise, it is not empty
  return FALSE;
}

// classify a box with respect to a plane
template<class Type>
inline Type OBBox<Type>::TestAgainstPlane(const Plane<Type, 3> &pl) const
{
  // project each axis to the plane normal
  Type tNX = ((const Vector<Type,3> &)pl)%box_avAxis[0];
  Type tNY = ((const Vector<Type,3> &)pl)%box_avAxis[1];
  Type tNZ = ((const Vector<Type,3> &)pl)%box_avAxis[2];
  // calculate overall size of the box along the plane normal
  Type tSize = Abs(tNX*box_atSize[0]) + Abs(tNY*box_atSize[1]) + Abs(tNZ*box_atSize[2]);
  // get distance of the center from the plane
  Type tCenterD = pl.PointDistance(box_vO);

  // if the center is further front than box's size
  if (tCenterD>tSize) {
    // completely in front                     `
    return Type(1);
  // if the center is further back than box's size
  } else if (tCenterD<-tSize) {
    // completely back
    return Type(-1);
  // otherwise, it touches the plane
  } else {
    return Type(0);
  }
}

// check if two boxes intersect/touch
// using the separating axes theorem
template<class Type>
inline BOOL OBBox<Type>::HasContactWith(const OBBox<Type> &boxB) const
{
  const OBBox<Type> &boxA = *this;

  // find offset in abs space
  Vector<Type, 3> vOffAbs = boxB.box_vO - boxA.box_vO;
  // rotate offset to A space
  Type vOffA[3] = {
    vOffAbs%boxA.box_avAxis[0], 
    vOffAbs%boxA.box_avAxis[1],
    vOffAbs%boxA.box_avAxis[2]};

  // calculate rotation matrix from B to A
  Type mR[3][3];
  {for(INDEX i=0; i<3; i++) {
    {for(INDEX j=0; j<3; j++) {
      mR[i][j] = boxA.box_avAxis[i]%boxB.box_avAxis[j]; 
    }}
  }}

  Type tRa, tRb, tT;

  // check each axis of A
  {for(INDEX i=0; i<3; i++ ) {
    tRa = boxA.box_atSize[i];
    tRb = boxB.box_atSize[0]*Abs(mR[i][0]) + boxB.box_atSize[1]*Abs(mR[i][1]) + boxB.box_atSize[2]*Abs(mR[i][2]);
    tT = Abs( vOffA[i] );
    if (tT>tRa+tRb) return FALSE;
  }}

  // check each axis of B
  {for(INDEX i=0; i<3; i++ ) {
    tRa = boxA.box_atSize[0]*Abs(mR[0][i]) + boxA.box_atSize[1]*Abs(mR[1][i]) + boxA.box_atSize[2]*Abs(mR[2][i]);
    tRb = boxB.box_atSize[i];
    tT = Abs( vOffA[0]*mR[0][i] + vOffA[1]*mR[1][i] + vOffA[2]*mR[2][i] );
    if (tT>tRa+tRb) return FALSE;
  }}

  // check A0 x B0
  tRa = boxA.box_atSize[1]*Abs(mR[2][0]) + boxA.box_atSize[2]*Abs(mR[1][0]);
  tRb = boxB.box_atSize[1]*Abs(mR[0][2]) + boxB.box_atSize[2]*Abs(mR[0][1]);
  tT =  Abs( vOffA[2]*mR[1][0] - vOffA[1]*mR[2][0] );
  if(tT>tRa+tRb) return FALSE;
  
  // check A0 x B1
  tRa = boxA.box_atSize[1]*Abs(mR[2][1]) + boxA.box_atSize[2]*Abs(mR[1][1]);
  tRb = boxB.box_atSize[0]*Abs(mR[0][2]) + boxB.box_atSize[2]*Abs(mR[0][0]);
  tT = Abs( vOffA[2]*mR[1][1] - vOffA[1]*mR[2][1] );
  if(tT>tRa+tRb) return FALSE;
  
  // check A0 x B2
  tRa = boxA.box_atSize[1]*Abs(mR[2][2]) + boxA.box_atSize[2]*Abs(mR[1][2]);
  tRb = boxB.box_atSize[0]*Abs(mR[0][1]) + boxB.box_atSize[1]*Abs(mR[0][0]);
  tT = Abs( vOffA[2]*mR[1][2] - vOffA[1]*mR[2][2] );
  if(tT>tRa+tRb) return FALSE;
  
  // check A1 x B0
  tRa = boxA.box_atSize[0]*Abs(mR[2][0]) + boxA.box_atSize[2]*Abs(mR[0][0]);
  tRb = boxB.box_atSize[1]*Abs(mR[1][2]) + boxB.box_atSize[2]*Abs(mR[1][1]);
  tT = Abs( vOffA[0]*mR[2][0] - vOffA[2]*mR[0][0] );
  if(tT>tRa+tRb) return FALSE;
  
  // check A1 x B1
  tRa = boxA.box_atSize[0]*Abs(mR[2][1]) + boxA.box_atSize[2]*Abs(mR[0][1]);
  tRb = boxB.box_atSize[0]*Abs(mR[1][2]) + boxB.box_atSize[2]*Abs(mR[1][0]);
  tT = Abs( vOffA[0]*mR[2][1] - vOffA[2]*mR[0][1] );
  if(tT>tRa+tRb) return FALSE;
  
  // check A1 x B2
  tRa = boxA.box_atSize[0]*Abs(mR[2][2]) + boxA.box_atSize[2]*Abs(mR[0][2]);
  tRb = boxB.box_atSize[0]*Abs(mR[1][1]) + boxB.box_atSize[1]*Abs(mR[1][0]);
  tT = Abs( vOffA[0]*mR[2][2] - vOffA[2]*mR[0][2] );
  if(tT>tRa+tRb) return FALSE;
  
  // check A2 x B0
  tRa = boxA.box_atSize[0]*Abs(mR[1][0]) + boxA.box_atSize[1]*Abs(mR[0][0]);
  tRb = boxB.box_atSize[1]*Abs(mR[2][2]) + boxB.box_atSize[2]*Abs(mR[2][1]);
  tT = Abs( vOffA[1]*mR[0][0] - vOffA[0]*mR[1][0] );
  if(tT>tRa+tRb) return FALSE;
  
  // check A2 x B1
  tRa = boxA.box_atSize[0]*Abs(mR[1][1]) + boxA.box_atSize[1]*Abs(mR[0][1]);
  tRb = boxB.box_atSize[0] *Abs(mR[2][2]) + boxB.box_atSize[2]*Abs(mR[2][0]);
  tT = Abs( vOffA[1]*mR[0][1] - vOffA[0]*mR[1][1] );
  if(tT>tRa+tRb) return FALSE;
  
  // check A2 x B2
  tRa = boxA.box_atSize[0]*Abs(mR[1][2]) + boxA.box_atSize[1]*Abs(mR[0][2]);
  tRb = boxB.box_atSize[0]*Abs(mR[2][1]) + boxB.box_atSize[1]*Abs(mR[2][0]);
  tT = Abs( vOffA[1]*mR[0][2] - vOffA[0]*mR[1][2] );
  if(tT>tRa+tRb) return FALSE;

  return TRUE;
}

// helper functions for converting between FLOAT and DOUBLE obboxes
inline DOUBLEobbox3D FLOATtoDOUBLE(const FLOATobbox3D &boxf) {
  return DOUBLEobbox3D( 
    FLOATtoDOUBLE(boxf.box_vO),
    FLOATtoDOUBLE(boxf.box_avAxis[0]),
    FLOATtoDOUBLE(boxf.box_avAxis[1]),
    FLOATtoDOUBLE(boxf.box_avAxis[2]),
    FLOATtoDOUBLE(boxf.box_atSize[0]),
    FLOATtoDOUBLE(boxf.box_atSize[1]),
    FLOATtoDOUBLE(boxf.box_atSize[2]));
}
inline FLOATobbox3D DOUBLEtoFLOAT(const DOUBLEobbox3D &boxd) {
  return FLOATobbox3D(
    DOUBLEtoFLOAT(boxd.box_vO),
    DOUBLEtoFLOAT(boxd.box_avAxis[0]),
    DOUBLEtoFLOAT(boxd.box_avAxis[1]),
    DOUBLEtoFLOAT(boxd.box_avAxis[2]),
    DOUBLEtoFLOAT(boxd.box_atSize[0]),
    DOUBLEtoFLOAT(boxd.box_atSize[1]),
    DOUBLEtoFLOAT(boxd.box_atSize[2]));
}


#endif  /* include-once check. */

