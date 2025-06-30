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

#ifndef SE_INCL_MATRIX_H
#define SE_INCL_MATRIX_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Stream.h>

/*
 * Template class for matrix of arbitrary dimensions and arbitrary type of members
 */
template<class Type, int iRows, int iColumns>
class Matrix {
public:
  Type matrix[iRows][iColumns];     // array that holds the members
public:
  /* Default constructor. */
  __forceinline Matrix(void);
  /* Constructor that sets the whole matrix to same number. */
  __forceinline Matrix(const Type x);

  /* Reference matrix member by it's row and column indices (1-based indices!). */
  __forceinline Type &operator()(int iRow, int iColumn);
  __forceinline const Type &operator()(int iRow, int iColumn) const;

  /* Make a transposed matrix. */
  __forceinline Matrix<Type, iRows, iColumns> operator!(void) const;
  __forceinline Matrix<Type, iRows, iColumns> &operator!=(const Matrix<Type, iRows, iColumns> &matrix2);

  /* Mathematical operators. */
  // between matrices
  __forceinline Matrix<Type, iRows, iColumns> operator+(const Matrix<Type, iRows, iColumns> &matrix2) const;
  __forceinline Matrix<Type, iRows, iColumns> &operator+=(const Matrix<Type, iRows, iColumns> &matrix2);
  __forceinline Matrix<Type, iRows, iColumns> operator-(const Matrix<Type, iRows, iColumns> &matrix2) const;
  __forceinline Matrix<Type, iRows, iColumns> &operator-=(const Matrix<Type, iRows, iColumns> &matrix2);
  __forceinline Matrix<Type, iRows, iColumns> operator*(const Matrix<Type, iRows, iColumns> &matrix2) const;
  __forceinline Matrix<Type, iRows, iColumns> &operator*=(const Matrix<Type, iRows, iColumns> &matrix2);
  // matrices and scalars
  __forceinline Matrix<Type, iRows, iColumns> operator*(const Type tMul) const;
  __forceinline Matrix<Type, iRows, iColumns> &operator*=(const Type tMul);
  __forceinline Matrix<Type, iRows, iColumns> operator/(const Type tMul) const;
  __forceinline Matrix<Type, iRows, iColumns> &operator/=(const Type tMul);

  /* Set matrix main diagonal. */
  void Diagonal(Type x);
  void Diagonal(const Vector<Type, iRows> &v);

  // get main vectors of matrix
  Vector<Type, iColumns> GetRow(Type iRow) const;
  Vector<Type, iRows> GetColumn(Type iColumn) const;

  /* Stream operations */
  friend __forceinline CTStream &operator>>(CTStream &strm, Matrix<Type, iRows, iColumns> &matrix)
  {
    strm.Read_t(&matrix, sizeof(matrix));
    return strm;
  }
  friend __forceinline CTStream &operator<<(CTStream &strm, Matrix<Type, iRows, iColumns> &matrix)
  {
    strm.Write_t(&matrix, sizeof(matrix));
    return strm;
  }
};


// inline functions implementation

/*
 * Default constructor.
 */
template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns>::Matrix(void)
{
#ifndef NDEBUG
  // set whole matrix to trash
  ULONG ulTrash = 0xCDCDCDCDul;
  for(int iRow=1; iRow<=iRows; iRow++) {
    for(int iColumn=1; iColumn<=iColumns; iColumn++) {
      (*this)(iRow, iColumn) = *reinterpret_cast<Type *>(&ulTrash);
    }
  }
#endif
}


/*
 * Constructor that sets the whole matrix to same number.
 */

// set FLOAT 3x3
template<> __forceinline Matrix<FLOAT,3,3>::Matrix(const FLOAT x /*= Type(0)*/)
{
  // set whole matrix to constant
  (*this)(1,1)=x;  (*this)(1,2)=x;  (*this)(1,3)=x;
  (*this)(2,1)=x;  (*this)(2,2)=x;  (*this)(2,3)=x;
  (*this)(3,1)=x;  (*this)(3,2)=x;  (*this)(3,3)=x;
}

// set DOUBLE 3x3
template<> __forceinline Matrix<DOUBLE,3,3>::Matrix(const DOUBLE x /*= Type(0)*/)
{
  // set whole matrix to constant
  (*this)(1,1)=x;  (*this)(1,2)=x;  (*this)(1,3)=x;
  (*this)(2,1)=x;  (*this)(2,2)=x;  (*this)(2,3)=x;
  (*this)(3,1)=x;  (*this)(3,2)=x;  (*this)(3,3)=x;
}

template<class Type, int iRows, int iColumns>
inline Matrix<Type, iRows, iColumns>::Matrix(const Type x /*= Type(0)*/)
{
  ASSERT( iRows!=3 && iColumns!=3);  // 3 is optimized special case 
  // set whole matrix to constant
  for(int iRow=1; iRow<=iRows; iRow++) {
    for(int iColumn=1; iColumn<=iColumns; iColumn++) {
      (*this)(iRow, iColumn) = x;
    }
  }
}


/*
 * Reference matrix member by it's row and column indices.
 */
template<class Type, int iRows, int iColumns>
__forceinline Type &Matrix<Type, iRows, iColumns>::operator()(int iRow, int iColumn)
{
  // check boundaries (indices start at 1, not at 0)
  ASSERT(iRow>=1 && iRow<=iRows && iColumn>=1 && iColumn<=iColumns);
  // return member reference
  return matrix[iRow-1][iColumn-1];
}

template<class Type, int iRows, int iColumns>
__forceinline const Type &Matrix<Type, iRows, iColumns>::operator()(int iRow, int iColumn) const
{
  // check boundaries (indices start at 1, not at 0)
  ASSERT(iRow>=1 && iRow<=iRows && iColumn>=1 && iColumn<=iColumns);
  // return member reference
  return matrix[iRow-1][iColumn-1];
}


/* Mathematical operators. */


// transposed FLOAT 3x3
template<> __forceinline Matrix<FLOAT,3,3> &Matrix<FLOAT,3,3>::operator!=(const Matrix<FLOAT,3,3> &matrix2)
{
  (*this)(1,1)=matrix2(1,1);  (*this)(1,2)=matrix2(2,1);  (*this)(1,3)=matrix2(3,1);
  (*this)(2,1)=matrix2(1,2);  (*this)(2,2)=matrix2(2,2);  (*this)(2,3)=matrix2(3,2);
  (*this)(3,1)=matrix2(1,3);  (*this)(3,2)=matrix2(2,3);  (*this)(3,3)=matrix2(3,3);
  return *this;
}

// transposed DOUBLE 3x3
template<> __forceinline Matrix<DOUBLE,3,3> &Matrix<DOUBLE,3,3>::operator!=(const Matrix<DOUBLE,3,3> &matrix2)
{
  (*this)(1,1)=matrix2(1,1);  (*this)(1,2)=matrix2(2,1);  (*this)(1,3)=matrix2(3,1);
  (*this)(2,1)=matrix2(1,2);  (*this)(2,2)=matrix2(2,2);  (*this)(2,3)=matrix2(3,2);
  (*this)(3,1)=matrix2(1,3);  (*this)(3,2)=matrix2(2,3);  (*this)(3,3)=matrix2(3,3);
  return *this;
}


// transposed matrix
template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> Matrix<Type, iRows, iColumns>::operator!(void) const
{
  return Matrix<Type, iRows, iColumns>() != *this;
}

template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> &Matrix<Type, iRows, iColumns>::operator!=(const Matrix<Type, iRows, iColumns> &matrix2)
{
  // transpose member by member
  ASSERT( iRows!=3 && iColumns!=3);  // 3 is optimized special case 
  for(int iRow=1; iRow<=iRows; iRow++) {
    for(int iColumn=1; iColumn<=iColumns; iColumn++) {
      (*this)(iColumn, iRow) = matrix2(iRow, iColumn);
    }
  }
  return *this;
}


// sum of two FLOATs 3x3
template<> __forceinline Matrix<FLOAT,3,3> &Matrix<FLOAT,3,3>::operator+=(const Matrix<FLOAT,3,3> &matrix2)
{
  (*this)(1,1)+=matrix2(1,1);  (*this)(1,2)+=matrix2(1,2);  (*this)(1,3)+=matrix2(1,3);
  (*this)(2,1)+=matrix2(2,1);  (*this)(2,2)+=matrix2(2,2);  (*this)(2,3)+=matrix2(2,3);
  (*this)(3,1)+=matrix2(3,1);  (*this)(3,2)+=matrix2(3,2);  (*this)(3,3)+=matrix2(3,3);
  return *this;
}

// sum of two DOUBLEs 3x3
template<> __forceinline Matrix<DOUBLE,3,3> &Matrix<DOUBLE,3,3>::operator+=(const Matrix<DOUBLE,3,3> &matrix2)
{
  (*this)(1,1)+=matrix2(1,1);  (*this)(1,2)+=matrix2(1,2);  (*this)(1,3)+=matrix2(1,3);
  (*this)(2,1)+=matrix2(2,1);  (*this)(2,2)+=matrix2(2,2);  (*this)(2,3)+=matrix2(2,3);
  (*this)(3,1)+=matrix2(3,1);  (*this)(3,2)+=matrix2(3,2);  (*this)(3,3)+=matrix2(3,3);
  return *this;
}

// sum of two matrices
template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> &Matrix<Type, iRows, iColumns>::operator+=(const Matrix<Type, iRows, iColumns> &matrix2)
{
  // add member by member
  ASSERT( iRows!=3 && iColumns!=3);  // 3 is optimized special case 
  for(int iRow=1; iRow<=iRows; iRow++) {
    for(int iColumn=1; iColumn<=iColumns; iColumn++) {
      (*this)(iRow, iColumn) += matrix2(iRow, iColumn);
    }
  }
  return *this;
}

template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> Matrix<Type, iRows, iColumns>::operator+(const Matrix<Type, iRows, iColumns> &matrix2) const
{
  return Matrix<Type, iRows, iColumns>(*this)+=matrix2;
}


// difference of two FLOATs 3x3
template<> __forceinline Matrix<FLOAT,3,3> &Matrix<FLOAT,3,3>::operator-=(const Matrix<FLOAT,3,3> &matrix2)
{
  (*this)(1,1)-=matrix2(1,1);  (*this)(1,2)-=matrix2(1,2);  (*this)(1,3)-=matrix2(1,3);
  (*this)(2,1)-=matrix2(2,1);  (*this)(2,2)-=matrix2(2,2);  (*this)(2,3)-=matrix2(2,3);
  (*this)(3,1)-=matrix2(3,1);  (*this)(3,2)-=matrix2(3,2);  (*this)(3,3)-=matrix2(3,3);
  return *this;
}

// difference of two DOUBLEs 3x3
template<> __forceinline Matrix<DOUBLE,3,3> &Matrix<DOUBLE,3,3>::operator-=(const Matrix<DOUBLE,3,3> &matrix2)
{
  (*this)(1,1)-=matrix2(1,1);  (*this)(1,2)-=matrix2(1,2);  (*this)(1,3)-=matrix2(1,3);
  (*this)(2,1)-=matrix2(2,1);  (*this)(2,2)-=matrix2(2,2);  (*this)(2,3)-=matrix2(2,3);
  (*this)(3,1)-=matrix2(3,1);  (*this)(3,2)-=matrix2(3,2);  (*this)(3,3)-=matrix2(3,3);
  return *this;
}

// difference of two matrices
template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> &Matrix<Type, iRows, iColumns>::operator-=(const Matrix<Type, iRows, iColumns> &matrix2)
{
  // sub member by member
  ASSERT( iRows!=3 && iColumns!=3);  // 3 is optimized special case 
  for(int iRow=1; iRow<=iRows; iRow++) {
    for(int iColumn=1; iColumn<=iColumns; iColumn++) {
      (*this)(iRow, iColumn) -= matrix2(iRow, iColumn);
    }
  }
  return *this;
}

template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> Matrix<Type, iRows, iColumns>::operator-(const Matrix<Type, iRows, iColumns> &matrix2) const
{
  return Matrix<Type, iRows, iColumns>(*this)-=matrix2;
}


// multiplication of two square matrices of same dimensions
/*
 * Dimensions: A(n,k), B(k,p), C(n,p)
 *
 * Formula: C=AxB --> Cij=Sum(s=1..k)(Ais*Bsj)
 */



// FLOAT 3x3
template<> __forceinline Matrix<FLOAT,3,3> Matrix<FLOAT,3,3>::operator*(const Matrix<FLOAT,3,3> &matrix2) const
{
  Matrix<FLOAT,3,3> result;
  result(1,1) = (*this)(1,1) * matrix2(1,1) + (*this)(1,2) * matrix2(2,1) + (*this)(1,3) * matrix2(3,1);
  result(1,2) = (*this)(1,1) * matrix2(1,2) + (*this)(1,2) * matrix2(2,2) + (*this)(1,3) * matrix2(3,2);
  result(1,3) = (*this)(1,1) * matrix2(1,3) + (*this)(1,2) * matrix2(2,3) + (*this)(1,3) * matrix2(3,3);
  result(2,1) = (*this)(2,1) * matrix2(1,1) + (*this)(2,2) * matrix2(2,1) + (*this)(2,3) * matrix2(3,1);
  result(2,2) = (*this)(2,1) * matrix2(1,2) + (*this)(2,2) * matrix2(2,2) + (*this)(2,3) * matrix2(3,2);
  result(2,3) = (*this)(2,1) * matrix2(1,3) + (*this)(2,2) * matrix2(2,3) + (*this)(2,3) * matrix2(3,3);
  result(3,1) = (*this)(3,1) * matrix2(1,1) + (*this)(3,2) * matrix2(2,1) + (*this)(3,3) * matrix2(3,1);
  result(3,2) = (*this)(3,1) * matrix2(1,2) + (*this)(3,2) * matrix2(2,2) + (*this)(3,3) * matrix2(3,2);
  result(3,3) = (*this)(3,1) * matrix2(1,3) + (*this)(3,2) * matrix2(2,3) + (*this)(3,3) * matrix2(3,3);
  return result;
}

// DOUBLE 3x3
template<> __forceinline Matrix<DOUBLE,3,3> Matrix<DOUBLE,3,3>::operator*(const Matrix<DOUBLE,3,3> &matrix2) const
{
  Matrix<DOUBLE,3,3> result;
  result(1,1) = (*this)(1,1) * matrix2(1,1) + (*this)(1,2) * matrix2(2,1) + (*this)(1,3) * matrix2(3,1);
  result(1,2) = (*this)(1,1) * matrix2(1,2) + (*this)(1,2) * matrix2(2,2) + (*this)(1,3) * matrix2(3,2);
  result(1,3) = (*this)(1,1) * matrix2(1,3) + (*this)(1,2) * matrix2(2,3) + (*this)(1,3) * matrix2(3,3);
  result(2,1) = (*this)(2,1) * matrix2(1,1) + (*this)(2,2) * matrix2(2,1) + (*this)(2,3) * matrix2(3,1);
  result(2,2) = (*this)(2,1) * matrix2(1,2) + (*this)(2,2) * matrix2(2,2) + (*this)(2,3) * matrix2(3,2);
  result(2,3) = (*this)(2,1) * matrix2(1,3) + (*this)(2,2) * matrix2(2,3) + (*this)(2,3) * matrix2(3,3);
  result(3,1) = (*this)(3,1) * matrix2(1,1) + (*this)(3,2) * matrix2(2,1) + (*this)(3,3) * matrix2(3,1);
  result(3,2) = (*this)(3,1) * matrix2(1,2) + (*this)(3,2) * matrix2(2,2) + (*this)(3,3) * matrix2(3,2);
  result(3,3) = (*this)(3,1) * matrix2(1,3) + (*this)(3,2) * matrix2(2,3) + (*this)(3,3) * matrix2(3,3);
  return result;
}

// general
template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> Matrix<Type, iRows, iColumns>::operator*(const Matrix<Type, iRows, iColumns> &matrix2) const
{
  Matrix<Type, iRows, iColumns> result;
  // check that the matrices have square dimensions
  ASSERT(iRows==iColumns);
  ASSERT( iRows!=3 && iColumns!=3);  // 3 is optimized special case 
  // multiply
  for(int iRow=1; iRow<=iRows; iRow++) {
    for(int iColumn=1; iColumn<=iColumns; iColumn++) {
      result(iRow, iColumn) = (Type)0;
      for(int s=1; s<=iRows; s++) {
        result(iRow, iColumn) += (*this)(iRow, s) * matrix2(s, iColumn);
      }
    }
  }
  return result;
}

// general
template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> &Matrix<Type, iRows, iColumns>::operator*=(const Matrix<Type, iRows, iColumns> &matrix2)
{
  *this = *this * matrix2;
  return *this;
}



// multiply FLOAT 3x3 with scalar
template<> __forceinline Matrix<FLOAT,3,3> &Matrix<FLOAT,3,3>::operator*=(const FLOAT tMul)
{
  (*this)(1,1)*=tMul;  (*this)(1,2)*=tMul;  (*this)(1,3)*=tMul;
  (*this)(2,1)*=tMul;  (*this)(2,2)*=tMul;  (*this)(2,3)*=tMul;
  (*this)(3,1)*=tMul;  (*this)(3,2)*=tMul;  (*this)(3,3)*=tMul;
  return *this;
}

// multiply DOUBLE 3x3 with scalar
template<> __forceinline Matrix<DOUBLE,3,3> &Matrix<DOUBLE,3,3>::operator*=(const DOUBLE tMul)
{
  (*this)(1,1)*=tMul;  (*this)(1,2)*=tMul;  (*this)(1,3)*=tMul;
  (*this)(2,1)*=tMul;  (*this)(2,2)*=tMul;  (*this)(2,3)*=tMul;
  (*this)(3,1)*=tMul;  (*this)(3,2)*=tMul;  (*this)(3,3)*=tMul;
  return *this;
}


// multiply matrix with scalar
template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> &Matrix<Type, iRows, iColumns>::operator*=(const Type tMul)
{
  // multiply member by member
  ASSERT( iRows!=3 && iColumns!=3);  // 3 is optimized special case 
  for(int iRow=1; iRow<=iRows; iRow++) {
    for(int iColumn=1; iColumn<=iColumns; iColumn++) {
      (*this)(iRow, iColumn) *= tMul;
    }
  }
  return *this;
}

// multiply matrix with scalar
template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> Matrix<Type, iRows, iColumns>::operator*(const Type tMul) const
{
  return Matrix<Type, iRows, iColumns>(*this)*=tMul;
}


// divide matrix with scalar
template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> &Matrix<Type, iRows, iColumns>::operator/=(const Type tDiv)
{
  // multiply with reciprocal
  (*this)*=(1/tDiv);
  return *this;
}

// divide matrix with scalar
template<class Type, int iRows, int iColumns>
__forceinline Matrix<Type, iRows, iColumns> Matrix<Type, iRows, iColumns>::operator/(const Type tDiv) const
{
  // multiply with reciprocal
  return Matrix<Type, iRows, iColumns>(*this)*=(1/tDiv);
}



/*
 * Set matrix main diagonal.
 */
template<class Type, int iRows, int iColumns>
void Matrix<Type, iRows, iColumns>::Diagonal(Type x)
{
  // check that the matrix is symetric
  ASSERT(iRows==iColumns);

  // clear whole matrix to zeroes
  for(int iRow=1; iRow<=iRows; iRow++) {
    for(int iColumn=1; iColumn<=iColumns; iColumn++) {
      (*this)(iRow, iColumn) = Type(0);
    }
  }
  // set the main diagonal
  {for(int iRow=0; iRow<iRows; iRow++) {
    matrix[iRow][iRow] = x;
  }}
}

template<class Type, int iRows, int iColumns>
void Matrix<Type, iRows, iColumns>::Diagonal(const Vector<Type, iRows> &v)
{
  // check that the matrix is symetric
  ASSERT(iRows==iColumns);

  // clear whole matrix to zeroes
  for(int iRow=1; iRow<=iRows; iRow++) {
    for(int iColumn=1; iColumn<=iColumns; iColumn++) {
      (*this)(iRow, iColumn) = Type(0);
    }
  }
  // set the main diagonal
  {for(int iRow=1; iRow<=iRows; iRow++) {
    operator()(iRow, iRow) = v(iRow);
  }}
}


// get main vectors of matrix
template<class Type, int iRows, int iColumns>
Vector<Type, iColumns> Matrix<Type, iRows, iColumns>::GetRow(Type iRow) const
{
  Vector<Type, iColumns> v;
  for(int iColumn=1; iColumn<=iColumns; iColumn++) {
    v(iColumn) = (*this)(iRow, iColumn);
  }
  return v;
}

template<class Type, int iRows, int iColumns>
Vector<Type, iRows> Matrix<Type, iRows, iColumns>::GetColumn(Type iColumn) const
{
  Vector<Type, iRows> v;
  for(int iRow=1; iRow<=iRows; iRow++) {
    v(iRow) = (*this)(iRow, iColumn);
  }
  return v;
}


// helper functions for converting between FLOAT and DOUBLE matrices
static __forceinline DOUBLEmatrix3D FLOATtoDOUBLE(const FLOATmatrix3D &mf)
{
  DOUBLEmatrix3D m;
  m(1,1) = FLOATtoDOUBLE(mf(1,1)); m(1,2) = FLOATtoDOUBLE(mf(1,2)); m(1,3) = FLOATtoDOUBLE(mf(1,3));
  m(2,1) = FLOATtoDOUBLE(mf(2,1)); m(2,2) = FLOATtoDOUBLE(mf(2,2)); m(2,3) = FLOATtoDOUBLE(mf(2,3));
  m(3,1) = FLOATtoDOUBLE(mf(3,1)); m(3,2) = FLOATtoDOUBLE(mf(3,2)); m(3,3) = FLOATtoDOUBLE(mf(3,3));
  return m;
}
static __forceinline FLOATmatrix3D DOUBLEtoFLOAT(const DOUBLEmatrix3D &md) {
  FLOATmatrix3D m;
  m(1,1) = DOUBLEtoFLOAT(md(1,1)); m(1,2) = DOUBLEtoFLOAT(md(1,2)); m(1,3) = DOUBLEtoFLOAT(md(1,3));
  m(2,1) = DOUBLEtoFLOAT(md(2,1)); m(2,2) = DOUBLEtoFLOAT(md(2,2)); m(2,3) = DOUBLEtoFLOAT(md(2,3));
  m(3,1) = DOUBLEtoFLOAT(md(3,1)); m(3,2) = DOUBLEtoFLOAT(md(3,2)); m(3,3) = DOUBLEtoFLOAT(md(3,3));
  return m;
}


#endif  /* include-once check. */

