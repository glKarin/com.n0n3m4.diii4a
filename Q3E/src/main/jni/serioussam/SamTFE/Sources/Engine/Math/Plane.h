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

#ifndef SE_INCL_PLANE_H
#define SE_INCL_PLANE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Assert.h>

/*
 * Template class for plane in space of arbitrary dimensions and arbitrary type of coordinates
 */
template<class Type, int iDimensions>
class Plane : public Vector<Type, iDimensions> { // normal vector
public:
  Type pl_distance;      // distance from point 0 along the normal vector
public:
  /* Default constructor. */
  inline Plane(void);
  /* Constructor from normal vector and distance. */
  inline Plane(const Vector<Type, iDimensions> &normal, const Type distance);
  /* Constructor from normal vector and a point on plane. */
  inline Plane(const Vector<Type, iDimensions> &normal, const Vector<Type, iDimensions> &point);
  /* Constructor from 3 points on plane, counter clockwise order. */
  inline Plane(const Vector<Type, iDimensions> &point0, const Vector<Type, iDimensions> &point1, const Vector<Type, iDimensions> &point2);

  /* Reference distance. */
  inline Type &Distance(void);
  inline const Type &Distance(void) const;
  /* Get a reference point on the plane. */
  inline Vector<Type, iDimensions> ReferencePoint(void) const;
  /* Get a reference point on the plane, if origin is at given vector. */
  inline Vector<Type, iDimensions> ReferencePoint(const Vector<Type, iDimensions> &vOrigin) const;

  /* Get distance of point from plane. */
  inline const Type PointDistance(const Vector<Type, iDimensions> &point) const;
  /* Get missing coordinate value */
  inline void GetCoordinate(const int iIndex, Vector<Type, iDimensions> &point) const;
  /* Get distance of plane from plane. */
  inline const Type PlaneDistance(const Plane<Type, iDimensions> &plOther) const;
  /* Project a point to the plane. */
  inline Vector<Type, iDimensions> ProjectPoint(const Vector<Type, iDimensions> &point) const;
  /* Project a direction vector to the plane. */
  inline Vector<Type, iDimensions> ProjectDirection(const Vector<Type, iDimensions> &direction) const;
  /* Get index of the greatest coordinate of normal. */
  inline INDEX GetMaxNormal(void) const;
  /* Deproject a point to the plane. */
  inline Vector<Type, iDimensions> DeprojectPoint(const Plane<Type, iDimensions> &plOther, const Vector<Type, iDimensions> &point) const;
  /* Deproject a direction vector to the plane. */
  inline Vector<Type, iDimensions> DeprojectDirection(const Plane<Type, iDimensions> &plOther, const Vector<Type, iDimensions> &point) const;

  /* Offset the plane forward for a given distance. */
  inline void Offset(const Type offset);

  /* Mathematical operators. */
  // unary minus (fliping of the plane)
  inline Plane<Type, iDimensions> operator-(void) const;
  // addition of vector (translation of the plane by the vector)
  inline Plane<Type, iDimensions> operator+(const Vector<Type, iDimensions> &vector2) const;
  inline Plane<Type, iDimensions> &operator+=(const Vector<Type, iDimensions> &vector2);
  inline Plane<Type, iDimensions> operator-(const Vector<Type, iDimensions> &vector2) const;
  inline Plane<Type, iDimensions> &operator-=(const Vector<Type, iDimensions> &vector2);
  // multiplication by a square matrix (sides swapped -- see implementation for notes)
  inline Plane<Type, iDimensions> &operator*=(const Matrix<Type, iDimensions, iDimensions> &matrix2);
  Plane<Type, iDimensions> operator*(const Matrix<Type, iDimensions, iDimensions> &matrix2) const;

  friend __forceinline CTStream &operator>>(CTStream &strm, Plane<Type, iDimensions> &p) {
    strm>>(Vector<Type, iDimensions>&)p;
    strm>>p.pl_distance;
    return strm;
  }
  friend __forceinline CTStream &operator<<(CTStream &strm, const Plane<Type, iDimensions> &p) {
    strm<<(const Vector<Type, iDimensions>&)p;
    strm<<p.pl_distance;
    return strm;
  }
};

// inline functions implementation
/*
 * Default constructor.
 */
template<class Type, int iDimensions>
inline Plane<Type, iDimensions>::Plane(void) {}

/* 
 * Constructor from normal vector and distance.
 */
template<class Type, int iDimensions>
inline Plane<Type, iDimensions>::Plane(const Vector<Type, iDimensions> &normal, const Type distance)
  : Vector<Type, iDimensions>(normal),
    pl_distance(distance)
{
  // normalize normal vector
  ((Vector<Type, iDimensions>)*this).Normalize();
}

/*
 * Constructor from normal vector and a point on plane.
 */
template<class Type, int iDimensions>
inline Plane<Type, iDimensions>::Plane(const Vector<Type, iDimensions> &normal, const Vector<Type, iDimensions> &point)
  : Vector<Type, iDimensions>(normal)
{
  // normalize normal vector
  this->Normalize();
  pl_distance = (*this)%point;   // distance = normalized_normal * point (dot product)
}

/*
 * Constructor from 3 points on plane, counter clockwise order.
 */
template<class Type, int iDimensions>
inline Plane<Type, iDimensions>::Plane(const Vector<Type, iDimensions> &point0, const Vector<Type, iDimensions> &point1, const Vector<Type, iDimensions> &point2)
{
  // create normal vector of plane
  Vector<Type, iDimensions> normal = (point2-point1)*(point0-point1);  // cross product
  // construct plane with normal and one point
  *this = Plane<Type, iDimensions>(normal, point0);
}

/*
 * Reference distance.
 */
template<class Type, int iDimensions>
inline Type &Plane<Type, iDimensions>::Distance(void) {
  return pl_distance;
}
template<class Type, int iDimensions>
inline const Type &Plane<Type, iDimensions>::Distance(void) const {
  return pl_distance;
}

/*
 * Get distance of point from plane.
 */
template<class Type, int iDimensions>
inline const Type Plane<Type, iDimensions>::PointDistance(const Vector<Type, iDimensions> &point) const
{
  /* Distance of the point from (0,0,0) along the normal vector of the plane
   * minus distance of the plane from (0,0,0) along the normal vector of the plane.
   */
  return (*this)%point-pl_distance;
}

/*
 * Get missing coordinate
 */
template<class Type, int iDimensions>
inline void Plane<Type, iDimensions>::GetCoordinate(const int iIndex, Vector<Type, iDimensions> &point) const
{
  Type sum = pl_distance;
  for( INDEX i=1; i<=iDimensions; i++)
  {
    if( i != iIndex)
      sum -= (*this)(i) * point(i);
  }
  point( iIndex) = sum/(*this)( iIndex);
}
/*
 * Get distance of plane from plane.
 */
template<class Type, int iDimensions>
inline const Type Plane<Type, iDimensions>::PlaneDistance(const Plane<Type, iDimensions> &plOther) const
{
  /* Distance of the reference point of ther other plane
   */
  return PointDistance(plOther.ReferencePoint());
}

/*
 * Get a reference point on the plane.
 */
template<class Type, int iDimensions>
inline Vector<Type, iDimensions> Plane<Type, iDimensions>::ReferencePoint(void) const
{
  // let the reference point be from (0,0,0) along the normal vector
  return ((Vector<Type, iDimensions>)*this)*pl_distance;
}
/*
 * Get a reference point on the plane, if origin is at given vector.
 */
template<class Type, int iDimensions>
inline Vector<Type, iDimensions> Plane<Type, iDimensions>::ReferencePoint(const Vector<Type, iDimensions> &vOrigin) const
{
  // let the reference point be from the origin along the normal vector,
  // as far as the origin is away from the plane
  return vOrigin-((Vector<Type, iDimensions>)*this)*(PointDistance(vOrigin));
}

/*
 * Project a point to the plane.
 */
template<class Type, int iDimensions>
inline Vector<Type, iDimensions> Plane<Type, iDimensions>::ProjectPoint(const Vector<Type, iDimensions> &point) const
{
  return ReferencePoint(point);
}

/*
 * Project a direction vector on the plane.
 */
template<class Type, int iDimensions>
inline Vector<Type, iDimensions> Plane<Type, iDimensions>::ProjectDirection(const Vector<Type, iDimensions> &direction) const
{
  // projected direction is vector between projected endpoint and projected origin
  return ProjectPoint(direction) - ReferencePoint();
}
template<class Type, int iDimensions>
/* Deproject a point to the plane. */
inline Vector<Type, iDimensions> Plane<Type, iDimensions>::DeprojectPoint(const Plane<Type, iDimensions> &plOther, const Vector<Type, iDimensions> &point) const
{
  Vector<Type, iDimensions> &vNormal      = (Vector<Type, iDimensions> &) *this;
  Vector<Type, iDimensions> &vNormalOther = (Vector<Type, iDimensions> &) plOther;
  return point - vNormalOther*( PointDistance(point)/(vNormal%vNormalOther) );
}
template<class Type, int iDimensions>
/* Deproject a direction vector to the plane. */
inline Vector<Type, iDimensions> Plane<Type, iDimensions>::DeprojectDirection(const Plane<Type, iDimensions> &plOther, const Vector<Type, iDimensions> &point) const
{
  Vector<Type, iDimensions> &vNormal      = (Vector<Type, iDimensions> &) *this;
  Vector<Type, iDimensions> &vNormalOther = (Vector<Type, iDimensions> &) plOther;
  return point - vNormalOther*( (point%vNormal)/(vNormal%vNormalOther) );
}
/*
 * Get index of the greatest coordinate of normal.
 */
template<class Type, int iDimensions>
inline INDEX Plane<Type, iDimensions>::GetMaxNormal(void) const
{
  INDEX iMax = 1;
  Type tMax = Abs((*this)(1));
  for(INDEX i=2; i<=iDimensions; i++) {
    if (Abs((*this)(i)) > tMax) {
      tMax = Abs((*this)(i));
      iMax = i;
    }
  }
  return iMax;
}

/* Offset the plane forward for a given distance. */
template<class Type, int iDimensions>
inline void Plane<Type, iDimensions>::Offset(const Type offset)
{
  pl_distance+=offset;
}

// unary minus (fliping of the plane)
template<class Type, int iDimensions>
inline Plane<Type, iDimensions> Plane<Type, iDimensions>::operator-(void) const {
  return Plane<Type, iDimensions>(-(Vector<Type, iDimensions>)*this, -pl_distance);
}

// addition of vector (translation along the vector)
template<class Type, int iDimensions>
inline Plane<Type, iDimensions> &Plane<Type, iDimensions>::operator+=(const Vector<Type, iDimensions> &vector2) {
  /* Calculate the length of the projection of the translation vector on the
    normal vector of the plane and add it to the plane distance.
  */
  pl_distance += (*static_cast<Vector<Type, iDimensions> *>(this))%vector2;
  return *this;
}
template<class Type, int iDimensions>
inline Plane<Type, iDimensions> Plane<Type, iDimensions>::operator+(const Vector<Type, iDimensions> &vector2) const {
  return Plane<Type, iDimensions>(*this)+=vector2;
}
template<class Type, int iDimensions>
inline Plane<Type, iDimensions> &Plane<Type, iDimensions>::operator-=(const Vector<Type, iDimensions> &vector2) {
  /* Calculate the length of the projection of the translation vector on the
    normal vector of the plane and add it to the plane distance.
  */
  pl_distance -= (*static_cast<Vector<Type, iDimensions> *>(this))%vector2;
  return *this;
}
template<class Type, int iDimensions>
inline Plane<Type, iDimensions> Plane<Type, iDimensions>::operator-(const Vector<Type, iDimensions> &vector2) const {
  return Plane<Type, iDimensions>(*this)-=vector2;
}

/*
 * Multiplication of a plane by a square matrix.
 */
// NOTE: The matrix should have been on the left side of the vector, but the template syntax 
// wouldn't allow that.
template<class Type, int iDimensions>
inline Plane<Type, iDimensions> &Plane<Type, iDimensions>::operator*=(const Matrix<Type, iDimensions, iDimensions> &matrix2) {
  (*static_cast<Vector<Type, iDimensions> *>(this))*=matrix2;
  return *this;
}
template<class Type, int iDimensions>
inline Plane<Type, iDimensions> Plane<Type, iDimensions>::operator*(const Matrix<Type, iDimensions, iDimensions> &matrix2) const {
  return Plane<Type, iDimensions>(*this)*=matrix2;
}

// helper functions for converting between FLOAT and DOUBLE planes
inline DOUBLEplane3D FLOATtoDOUBLE(const FLOATplane3D &plf) {
  return DOUBLEplane3D(
    FLOATtoDOUBLE((FLOAT3D&)plf),
    FLOATtoDOUBLE(plf.Distance())
    );
}
inline FLOATplane3D DOUBLEtoFLOAT(const DOUBLEplane3D &pld) {
  return FLOATplane3D(
    DOUBLEtoFLOAT((DOUBLE3D&)pld),
    DOUBLEtoFLOAT(pld.Distance())
    );
}


#endif  /* include-once check. */

