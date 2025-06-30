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

#ifndef SE_INCL_VECTOR_H
#define SE_INCL_VECTOR_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Assert.h>
#include <Engine/Base/Types.h>
#include <Engine/Base/Stream.h>
#include <Engine/Math/Matrix.h>
#include <Engine/Math/Functions.h>
           
/*
 * Template class for vector of arbitrary dimensions and arbitrary type of members
 */
template<class Type, int iDimensions>
class Vector {
public:
  Type vector[iDimensions];     // array that holds the members
public:
  /* Default constructor. */
  __forceinline Vector(void);
  /* Constructor from coordinates. */
  __forceinline Vector(Type x1);
  __forceinline Vector(Type x1, Type x2);
  __forceinline Vector(Type x1, Type x2, Type x3);
  __forceinline Vector(Type x1, Type x2, Type x3, Type x4);
  /* Clear function */
  __forceinline void Clear(void) {};


  /* Conversion into scalar -- length of vector (Euclidian norm). */
  __forceinline Type Length(void) const;
  /* Conversion into scalar -- Manhattan norm of vector. */
  __forceinline Type ManhattanNorm(void) const;
  /* Conversion into scalar -- Max norm of vector. */
  __forceinline Type MaxNorm(void) const;

  /* Reference vector member by it's index (1-based indices!). */
  __forceinline Type &operator()(int i);
  __forceinline const Type &operator()(int i) const;

  /* Normalize vector, i.e. make it a unit vector. */
  __forceinline Vector<Type, iDimensions> &Normalize(void);
  __forceinline Vector<Type, iDimensions> &SafeNormalize(void); // gives vector with (0,0,0) orientation if input is too small

  /* Mathematical operators. */
  // unary minus
  __forceinline Vector<Type, iDimensions> &Flip(void);
  __forceinline Vector<Type, iDimensions> operator-(void) const;
  // between two vectors
  __forceinline Vector<Type, iDimensions> operator+(const Vector<Type, iDimensions> &vector2) const;
  __forceinline Vector<Type, iDimensions> &operator+=(const Vector<Type, iDimensions> &vector2);
  __forceinline Vector<Type, iDimensions> operator-(const Vector<Type, iDimensions> &vector2) const;
  __forceinline Vector<Type, iDimensions> &operator-=(const Vector<Type, iDimensions> &vector2);
  // multiplication with scalar
  __forceinline Vector<Type, iDimensions> &operator*=(const Type scalar);
  __forceinline Vector<Type, iDimensions> operator*(const Type scalar) const;
  // division with scalar
  __forceinline Vector<Type, iDimensions> &operator/=(const Type scalar);
  __forceinline Vector<Type, iDimensions> operator/(const Type scalar) const;
  // multiplication by a square matrix (sides swapped -- see implementation for notes)
  __forceinline Vector<Type, iDimensions> &operator*=(const Matrix<Type, iDimensions, iDimensions> &matrix2);
  __forceinline Vector<Type, iDimensions> operator*(const Matrix<Type, iDimensions, iDimensions> &matrix2) const;
  // scalar product - dot product, inner product
  __forceinline Type operator%(const Vector<Type, iDimensions> &vector2) const;
  // vector product - cross product, outer product
  __forceinline Vector<Type, iDimensions> &operator*=(const Vector<Type, iDimensions> &vector2);
  __forceinline Vector<Type, iDimensions> operator*(const Vector<Type, iDimensions> &vector2) const;
  // comparing vectors
  __forceinline BOOL operator==(const Vector<Type, iDimensions> &vector2) const;
  __forceinline BOOL operator!=(const Vector<Type, iDimensions> &vector2) const;

  /* Stream operations */
  friend __forceinline CTStream &operator>>(CTStream &strm, Vector<Type, iDimensions> &vector) {
    for (SLONG i = 0; i < iDimensions; i++)
        strm>>vector.vector[i];
    return strm;
  }
  friend __forceinline CTStream &operator<<(CTStream &strm, const Vector<Type, iDimensions> &vector) {
    for (SLONG i = 0; i < iDimensions; i++)
        strm<<vector.vector[i];
    return strm;
  }
};


// inline functions implementation

/*
 * Default constructor.
 */
template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions>::Vector(void) {}


/*
 * Constructor from coordinates.
 */
template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions>::Vector(Type x1)
{
  ASSERT(iDimensions==1);
  (*this)(1)=x1;
}

template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions>::Vector(Type x1, Type x2)
{
  ASSERT(iDimensions==2);
  (*this)(1)=x1;
  (*this)(2)=x2;
}

template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions>::Vector(Type x1, Type x2, Type x3)
{
  ASSERT(iDimensions==3);
  (*this)(1)=x1;
  (*this)(2)=x2;
  (*this)(3)=x3;
}

template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions>::Vector(Type x1, Type x2, Type x3, Type x4)
{
  ASSERT(iDimensions==4);
  (*this)(1)=x1;
  (*this)(2)=x2;
  (*this)(3)=x3;
  (*this)(4)=x4;
}


/*
 * Conversion into scalar -- length of vector.
 */
template<> __forceinline FLOAT Vector<FLOAT,3>::Length(void) const
{
  return (FLOAT)sqrt( (DOUBLE)((*this)(1)*(*this)(1) + (*this)(2)*(*this)(2) + (*this)(3)*(*this)(3)));
}

template<> __forceinline DOUBLE Vector<DOUBLE,3>::Length(void) const
{
  return (DOUBLE)sqrt( (DOUBLE)((*this)(1)*(*this)(1) + (*this)(2)*(*this)(2) + (*this)(3)*(*this)(3)));
}

template<class Type, int iDimensions>
__forceinline Type Vector<Type, iDimensions>::Length(void) const
{
  Type result=(Type)0;
  for(int i=1; i<=iDimensions; i++) {
    result += (*this)(i) * (*this)(i);
  }
  return (Type)sqrt((DOUBLE)result);
}

/*
 * Conversion into scalar -- Manhattan norm of vector.
 */

template<class Type, int iDimensions>
__forceinline Type Vector<Type, iDimensions>::ManhattanNorm(void) const
{
  Type result=(Type)0;
  for(int i=1; i<=iDimensions; i++) {
    result += Abs((*this)(i));
  }
  return result;
}

/*
 * Conversion into scalar -- Max norm of vector.
 */
template<class Type, int iDimensions>
__forceinline Type Vector<Type, iDimensions>::MaxNorm(void) const
{
  Type result=(Type)0;
  for(int i=1; i<=iDimensions; i++) {
    result = Max(result, Abs((*this)(i)));
  }
  return result;
}

/*
 * Reference vector member by it's index (1-based indices!).
 */
template<class Type, int iDimensions>
__forceinline Type &Vector<Type, iDimensions>::operator()(int i)
{
  // check boundaries (indices start at 1, not at 0)
  ASSERT(i>=1 && i<=iDimensions);
  // return vector member reference
  return vector[i-1];
}

template<class Type, int iDimensions>
__forceinline const Type &Vector<Type, iDimensions>::operator()(int i) const
{
  // check boundaries (indices start at 1, not at 0)
  ASSERT(i>=1 && i<=iDimensions);
  // return vector member reference
  return vector[i-1];
}

/*
 * Normalize vector, i.e. make it a unit vector.
 */
template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> &Vector<Type, iDimensions>::Normalize(void)
{
  // Normalizing a vector of a very small length can be very unprecise!
  // ASSERT(((Type)*this) > 0.001);
  *this/=Length();
  return *this;
}
// gives vector with (0,0,0) orientation if input is too small
template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> &Vector<Type, iDimensions>::SafeNormalize(void)
{
  Type tLen = Length();
  if (tLen<1E-6f) {
    if (iDimensions==2) {
      *this = Vector(1,0);
    } else {
      *this = Vector(0,0,-1);
    }
  } else {
    *this/=tLen;
  }
  return *this;
}


// unary minus FLOAT3D
template<> __forceinline Vector<FLOAT,3> &Vector<FLOAT,3>::Flip(void)
{
  (*this)(1) = -(*this)(1);
  (*this)(2) = -(*this)(2);
  (*this)(3) = -(*this)(3);
  return *this;
}

// unary minus DOUBLE3D
template<> __forceinline Vector<DOUBLE,3> &Vector<DOUBLE,3>::Flip(void)
{
  (*this)(1) = -(*this)(1);
  (*this)(2) = -(*this)(2);
  (*this)(3) = -(*this)(3);
  return *this;
}

// unary minus
template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> &Vector<Type, iDimensions>::Flip(void)
{
  // flip member by member
  ASSERT( iDimensions!=3);  // 3 is optimized special case 
  for(int iDimension=1; iDimension<=iDimensions; iDimension++) {
    (*this)(iDimension) = -(*this)(iDimension);
  }
  return *this;
}

template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> Vector<Type, iDimensions>::operator-(void) const
{
  return Vector<Type, iDimensions>(*this).Flip();
}


// sum of two vectors FLOAT3D
template<> __forceinline Vector<FLOAT,3> &Vector<FLOAT,3>::operator+=(const Vector<FLOAT,3> &vector2)
{
  // add member by member
  (*this)(1) += vector2(1);
  (*this)(2) += vector2(2);
  (*this)(3) += vector2(3);
  return *this;
}

// sum of two vectors DOUBLE3D
template<> __forceinline Vector<DOUBLE,3> &Vector<DOUBLE,3>::operator+=(const Vector<DOUBLE,3> &vector2)
{
  // add member by member
  (*this)(1) += vector2(1);
  (*this)(2) += vector2(2);
  (*this)(3) += vector2(3);
  return *this;
}

// sum of two vectors
template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> &Vector<Type, iDimensions>::operator+=(const Vector<Type, iDimensions> &vector2)
{
  // add member by member
  ASSERT( iDimensions!=3);  // 3 is optimized special case 
  for(int iDimension=1; iDimension<=iDimensions; iDimension++) {
    (*this)(iDimension) += vector2(iDimension);
  }
  return *this;
}

template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> Vector<Type, iDimensions>::operator+(const Vector<Type, iDimensions> &vector2) const
{
  return Vector<Type, iDimensions>(*this)+=vector2;
}


// difference of two vectors FLOAT3D
template<> __forceinline Vector<FLOAT,3> &Vector<FLOAT,3>::operator-=(const Vector<FLOAT,3> &vector2)
{
  // add member by member
  (*this)(1) -= vector2(1);
  (*this)(2) -= vector2(2);
  (*this)(3) -= vector2(3);
  return *this;
}

// difference of two vectors DOUBLE3D
template<> __forceinline Vector<DOUBLE,3> &Vector<DOUBLE,3>::operator-=(const Vector<DOUBLE,3> &vector2)
{
  // add member by member
  (*this)(1) -= vector2(1);
  (*this)(2) -= vector2(2);
  (*this)(3) -= vector2(3);
  return *this;
}

// difference of two vectors
template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> &Vector<Type, iDimensions>::operator-=(const Vector<Type, iDimensions> &vector2)
{
  // sub member by member
  ASSERT( iDimensions!=3);  // 3 is optimized special case 
  for(int iDimension=1; iDimension<=iDimensions; iDimension++) {
    (*this)(iDimension) -= vector2(iDimension);
  }
  return *this;
}

template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> Vector<Type, iDimensions>::operator-(const Vector<Type, iDimensions> &vector2) const
{
  return Vector<Type, iDimensions>(*this)-=vector2;
}



// multiplication with scalar FLOAT3D
template<> __forceinline Vector<FLOAT,3> &Vector<FLOAT,3>::operator*=(const FLOAT scalar)
{
  (*this)(1) *= scalar;
  (*this)(2) *= scalar;
  (*this)(3) *= scalar;
  return *this;
}

// multiplication with scalar DOUBLE3D
template<> __forceinline Vector<DOUBLE,3> &Vector<DOUBLE,3>::operator*=(const DOUBLE scalar)
{
  (*this)(1) *= scalar;
  (*this)(2) *= scalar;
  (*this)(3) *= scalar;
  return *this;
}

// multiplication with scalar
template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> &Vector<Type, iDimensions>::operator*=(const Type scalar)
{
  ASSERT( iDimensions!=3);  // 3 is optimized special case 
  for( int i=1; i<=iDimensions; i++) (*this)(i) *= scalar;
  return *this;
}

template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> Vector<Type, iDimensions>::operator*(const Type scalar) const
{
  return Vector<Type, iDimensions>(*this) *= scalar;
}


// division with scalar FLOAT3D
template<> __forceinline Vector<FLOAT,3> &Vector<FLOAT,3>::operator/=(const FLOAT scalar)
{
  const FLOAT rcp = 1.0f/scalar;
  (*this)(1) *= rcp;
  (*this)(2) *= rcp;
  (*this)(3) *= rcp;
  return *this;
}

// division with scalar DOUBLE3D
template<> __forceinline Vector<DOUBLE,3> &Vector<DOUBLE,3>::operator/=(const DOUBLE scalar)
{
  const DOUBLE rcp = 1.0/scalar;
  (*this)(1) *= rcp;
  (*this)(2) *= rcp;
  (*this)(3) *= rcp;
  return *this;
}


// division with scalar
template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> &Vector<Type, iDimensions>::operator/=(const Type scalar)
{
  ASSERT( iDimensions!=3);  // 3 is optimized special case 
  for( int i=1; i<=iDimensions; i++) (*this)(i) /= scalar;
  return *this;
}

template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> Vector<Type, iDimensions>::operator/(const Type scalar) const
{
  return Vector<Type, iDimensions>(*this) /= scalar;
}


/*
 * Multiplication of a vector by a square matrix.
 */
// NOTE: The matrix should have been on the left side of the vector, but the template syntax wouldn't allow that.

template<> __forceinline Vector<DOUBLE,3> Vector<DOUBLE,3>::operator*(const Matrix<DOUBLE,3,3> &matrix2) const
{
  Vector<DOUBLE,3> result;
  result(1) = matrix2(1,1) * (*this)(1) + matrix2(1,2) * (*this)(2) + matrix2(1,3) * (*this)(3);
  result(2) = matrix2(2,1) * (*this)(1) + matrix2(2,2) * (*this)(2) + matrix2(2,3) * (*this)(3);
  result(3) = matrix2(3,1) * (*this)(1) + matrix2(3,2) * (*this)(2) + matrix2(3,3) * (*this)(3);
  return result;
}

template<> __forceinline Vector<FLOAT,3> Vector<FLOAT,3>::operator*(const Matrix<FLOAT,3,3> &matrix2) const
{
  Vector<FLOAT,3> result;
  result(1) = matrix2(1,1) * (*this)(1) + matrix2(1,2) * (*this)(2) + matrix2(1,3) * (*this)(3);
  result(2) = matrix2(2,1) * (*this)(1) + matrix2(2,2) * (*this)(2) + matrix2(2,3) * (*this)(3);
  result(3) = matrix2(3,1) * (*this)(1) + matrix2(3,2) * (*this)(2) + matrix2(3,3) * (*this)(3);
  return result;
}

template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> &Vector<Type, iDimensions>::operator*=(const Matrix<Type, iDimensions, iDimensions> &matrix2)
{
  (*this) = (*this) * matrix2;
  return *this;
}

template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> Vector<Type, iDimensions>::operator*(const Matrix<Type, iDimensions, iDimensions> &matrix2) const
{
  ASSERT( iDimensions!=3);  // 3 is optimized special case 
  Vector<Type, iDimensions> result;
  for(int iRow=1; iRow<=iDimensions; iRow++) {
    result(iRow) = (Type)0;
    for(int s=1; s<=iDimensions; s++) {
      result(iRow) += matrix2(iRow, s) * (*this)(s);
    }
  }
  return result;
}


// scalar product - dot product, inner product for FLOAT3D
template<> __forceinline FLOAT Vector<FLOAT,3>::operator%(const Vector<FLOAT,3> &vector2) const
{
  return (FLOAT)((*this)(1)*vector2(1) + (*this)(2)*vector2(2) + (*this)(3)*vector2(3));
}

// scalar product - dot product, inner product for DOUBLE3D
template<> __forceinline DOUBLE Vector<DOUBLE,3>::operator%(const Vector<DOUBLE,3> &vector2) const
{
  return (DOUBLE)((*this)(1)*vector2(1) + (*this)(2)*vector2(2) + (*this)(3)*vector2(3));
}

// scalar product - dot product, inner product
template<class Type, int iDimensions>
__forceinline Type Vector<Type, iDimensions>::operator%(const Vector<Type, iDimensions> &vector2) const
{
  ASSERT( iDimensions!=3);  // 3 is optimized special case 
  Type result=(Type)0;
  for(int i=1; i<=iDimensions; i++) {
    result += (*this)(i) * vector2(i);
  }
  return result;
}



// vector product - cross product, outer product
/* Formula:   C=A*B
   Cx = Ay*Bz - Az*By
   Cy = Az*Bx - Ax*Bz
   Cz = Ax*By - Ay*Bx
*/
template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> &Vector<Type, iDimensions>::operator*=(const Vector<Type, iDimensions> &vector2)
{
  (*this) = (*this) * vector2;
  return *this;
}

template<class Type, int iDimensions>
__forceinline Vector<Type, iDimensions> Vector<Type, iDimensions>::operator*(const Vector<Type, iDimensions> &vector2) const
{
  Vector<Type, iDimensions> result;
  ASSERT(iDimensions==3);    // cross product is defined only for 3D vectors
  result(1) = (*this)(2)*vector2(3) - (*this)(3)*vector2(2);
  result(2) = (*this)(3)*vector2(1) - (*this)(1)*vector2(3);
  result(3) = (*this)(1)*vector2(2) - (*this)(2)*vector2(1);
  return result;
}


// comparation FLOAT3D
template<> __forceinline BOOL Vector<FLOAT,3>::operator==(const Vector<FLOAT,3> &vector2) const
{
  return( (*this)(1)==vector2(1) && (*this)(2)==vector2(2) && (*this)(3)==vector2(3));
}

// comparation DOUBLE3D
template<> __forceinline BOOL Vector<DOUBLE,3>::operator==(const Vector<DOUBLE,3> &vector2) const
{
  return( (*this)(1)==vector2(1) && (*this)(2)==vector2(2) && (*this)(3)==vector2(3));
}

// comparation
template<class Type, int iDimensions>
__forceinline BOOL Vector<Type, iDimensions>::operator==(const Vector<Type, iDimensions> &vector2) const
{
  ASSERT( iDimensions!=3);  // 3 is optimized special case 
  for(int i=1; i<=iDimensions; i++) {
    if( (*this)(i) != vector2(i))
      return FALSE;
  }
  return TRUE;
}


template<class Type, int iDimensions>
__forceinline BOOL Vector<Type, iDimensions>::operator!=(const Vector<Type, iDimensions> &vector2) const
{
  return !(*this == vector2);
}


// helper functions for converting between FLOAT and DOUBLE vectors
__forceinline DOUBLE3D FLOATtoDOUBLE(const FLOAT3D &vf)
{
  return DOUBLE3D(FLOATtoDOUBLE(vf(1)), FLOATtoDOUBLE(vf(2)), FLOATtoDOUBLE(vf(3)));
}

__forceinline FLOAT3D DOUBLEtoFLOAT(const DOUBLE3D &vd)
{
  return FLOAT3D(DOUBLEtoFLOAT(vd(1)), DOUBLEtoFLOAT(vd(2)), DOUBLEtoFLOAT(vd(3)));
}



#endif  /* include-once check. */

