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

#ifndef SE_INCL_QUATERNION_H
#define SE_INCL_QUATERNION_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Assert.h>
#include <Engine/Math/Functions.h>

/*
 * Template class for quaternion of arbitrary precision.
 */
template<class Type>
class Quaternion {
public:
  Type q_w, q_x, q_y, q_z;
public:
  // default constructor
  inline Quaternion(void);
  // constructor from four scalar values
  inline Quaternion(Type w, Type x, Type y, Type z);

  // conversion from euler angles
  void FromEuler(const Vector<Type, 3> &a);

  inline Type EPS(Type orig) const;

  // conversion to matrix
  void ToMatrix(Matrix<Type, 3, 3> &m) const;
  // conversion from matrix
  void FromMatrix(Matrix<Type, 3, 3> &m);

  // conversion to/from axis-angle
  void FromAxisAngle(const Vector<Type, 3> &n, Type a);
  void ToAxisAngle(Vector<Type, 3> &n, Type &a);

  // unary minus (fliping of the quaternion)
  inline Quaternion<Type> operator-(void) const;
  // conjugation
  inline Quaternion<Type> operator~(void) const;
  // inversion
  inline Quaternion<Type> Inv(void) const;

  // multiplication/division by a scalar
  inline Quaternion<Type> operator*(Type t) const;
  inline Quaternion<Type> &operator*=(Type t);

  friend Quaternion<Type> operator*(Type t, Quaternion<Type> q)
  {
    return Quaternion<Type>(q.q_w*t, q.q_x*t, q.q_y*t, q.q_z*t);
  }

  inline Quaternion<Type> operator/(Type t) const;
  inline Quaternion<Type> &operator/=(Type t);
  
  // addition/substraction
  inline Quaternion<Type> operator+(const Quaternion<Type> &q2) const;
  inline Quaternion<Type> &operator+=(const Quaternion<Type> &q2);
  inline Quaternion<Type> operator-(const Quaternion<Type> &q2) const;
  inline Quaternion<Type> &operator-=(const Quaternion<Type> &q2);
  // multiplication
  inline Quaternion<Type> operator*(const Quaternion<Type> &q2) const;
  inline Quaternion<Type> &operator*=(const Quaternion<Type> &q2);
  // dot product
  inline Type operator%(const Quaternion<Type> &q2) const;

  // quaternion norm (euclidian length of a 4d vector)
  inline Type Norm(void) const;

  friend __forceinline CTStream &operator>>(CTStream &strm, Quaternion<Type> &q) {
    strm>>q.q_w;
    strm>>q.q_x;
    strm>>q.q_y;
    strm>>q.q_z;
    return strm;
  }

  friend __forceinline CTStream &operator<<(CTStream &strm, const Quaternion<Type> &q) {
    strm<<q.q_w;
    strm<<q.q_x;
    strm<<q.q_y;
    strm<<q.q_z;
    return strm;
  }

  // transcendental functions
  friend Quaternion<Type> Exp(const Quaternion<Type> &q)
  {
    Type tAngle = (Type)sqrt(q.q_x*q.q_x + q.q_y*q.q_y + q.q_z*q.q_z);
    Type tSin = sin(tAngle);
    Type tCos = cos(tAngle);

    if (fabs(tSin)<0.001) {
      return Quaternion<Type>(tCos, q.q_x, q.q_y, q.q_z);
    } else {
      Type tRatio = tSin/tAngle;
      return Quaternion<Type>(tCos, q.q_x*tRatio, q.q_y*tRatio, q.q_z*tRatio);
    }
  }

  friend Quaternion<Type> Log(const Quaternion<Type> &q)
  {
    if (fabs(q.q_w)<1.0) {
      Type tAngle = acos(q.q_w);
      Type tSin   = sin(tAngle);
      if (fabs(tSin)>=0.001) {
        Type tRatio = tAngle/tSin;
        return Quaternion<Type>(Type(0), q.q_x*tRatio, q.q_y*tRatio, q.q_z*tRatio);
      }
    }

    return Quaternion<Type>(Type(0), q.q_x, q.q_y, q.q_z);
  }

  // spherical linear interpolation
  friend Quaternion<Type> Slerp(Type tT,
    const Quaternion<Type> &q1, const Quaternion<Type> &q2)
  {
    Type tCos = q1%q2;

    Quaternion<Type> qTemp;
  
    if (tCos<Type(0)) {
      tCos = -tCos;
      qTemp = -q2;
    } else  {
      qTemp = q2;
    }

    Type tF1, tF2;
    if ((Type(1)-tCos) > Type(0.001)) {
      // standard case (slerp)
      Type tAngle = acos(tCos);
      Type tSin   = sin(tAngle);
      tF1 = sin((Type(1)-tT)*tAngle)/tSin;
      tF2 = sin(tT*tAngle)/tSin;
    } else {
      // linear interpolation
      tF1 = Type(1)-tT;
      tF2 = tT;
    }
    return q1*tF1 + qTemp*tF2;
  }

  // spherical quadratic interpolation
  friend Quaternion<Type> Squad(Type tT,
    const Quaternion<Type> &q1, const Quaternion<Type> &q2,
    const Quaternion<Type> &qa, const Quaternion<Type> &qb)
  {
    return Slerp(2*tT*(1-tT),Slerp(tT,q1,q2),Slerp(tT,qa,qb));
  }
};

// inline functions implementation
/*
 * Default constructor.
 */
template<class Type>
inline Quaternion<Type>::Quaternion(void) {};

/* Constructor from three values. */
template<class Type>
inline Quaternion<Type>::Quaternion(Type w, Type x, Type y, Type z)
: q_w(w), q_x(x), q_y(y), q_z(z) {};

// unary minus (additive inversion)
template<class Type>
inline Quaternion<Type> Quaternion<Type>::operator-(void) const {
  return Quaternion<Type>(-q_w, -q_x, -q_y, -q_z);
}
// conjugation
template<class Type>
inline Quaternion<Type> Quaternion<Type>::operator~(void) const {
  return Quaternion<Type>(q_w, -q_x, -q_y, -q_z);
}
// multiplicative inversion
template<class Type>
inline Quaternion<Type> Quaternion<Type>::Inv(void) const {
  return (~(*this))/Norm();
}

// multiplication/division by a scalar
template<class Type>
inline Quaternion<Type> Quaternion<Type>::operator*(Type t) const {
  return Quaternion<Type>(q_w*t, q_x*t, q_y*t, q_z*t);
}
template<class Type>
inline Quaternion<Type> &Quaternion<Type>::operator*=(Type t) {
  q_w*=t; q_x*=t; q_y*=t; q_z*=t;
  return *this;
}

#if 0
template<class Type>
inline Quaternion<Type> operator*(Type t, Quaternion<Type> q) {
  return Quaternion<Type>(q.q_w*t, q.q_x*t, q.q_y*t, q.q_z*t);
}
#endif

template<class Type>
inline Quaternion<Type> Quaternion<Type>::operator/(Type t) const {
  return Quaternion<Type>(q_w/t, q_x/t, q_y/t, q_z/t);
}
template<class Type>
inline Quaternion<Type> &Quaternion<Type>::operator/=(Type t) {
  q_w/=t; q_x/=t; q_y/=t; q_z/=t;
  return *this;
}

// addition/substraction
template<class Type>
inline Quaternion<Type> Quaternion<Type>::operator+(const Quaternion<Type> &q2) const {
  return Quaternion<Type>(q_w+q2.q_w, q_x+q2.q_x, q_y+q2.q_y, q_z+q2.q_z);
}
template<class Type>
inline Quaternion<Type> &Quaternion<Type>::operator+=(const Quaternion<Type> &q2) {
  q_w+=q2.q_w; q_x+=q2.q_x; q_y+=q2.q_y; q_z+=q2.q_z;
  return *this;
}
template<class Type>
inline Quaternion<Type> Quaternion<Type>::operator-(const Quaternion<Type> &q2) const {
  return Quaternion<Type>(q_w-q2.q_w, q_x-q2.q_x, q_y-q2.q_y, q_z-q2.q_z);
}
template<class Type>
inline Quaternion<Type> &Quaternion<Type>::operator-=(const Quaternion<Type> &q2) {
  q_w-=q2.q_w; q_x-=q2.q_x; q_y-=q2.q_y; q_z-=q2.q_z;
  return *this;
}

// multiplication
template<class Type>
inline Quaternion<Type> Quaternion<Type>::operator*(const Quaternion<Type> &q2) const {
  return Quaternion<Type>(
  q_w*q2.q_w - q_x*q2.q_x - q_y*q2.q_y - q_z*q2.q_z,
  q_w*q2.q_x + q_x*q2.q_w + q_y*q2.q_z - q_z*q2.q_y,
  q_w*q2.q_y - q_x*q2.q_z + q_y*q2.q_w + q_z*q2.q_x,
  q_w*q2.q_z + q_x*q2.q_y - q_y*q2.q_x + q_z*q2.q_w);
}
template<class Type>
inline Quaternion<Type> &Quaternion<Type>::operator*=(const Quaternion<Type> &q2) {
  *this = (*this)*q2;
  return *this;
}
// dot product
template<class Type>
inline Type Quaternion<Type>::operator%(const Quaternion<Type> &q2) const {
  return q_w*q2.q_w + q_x*q2.q_x + q_y*q2.q_y + q_z*q2.q_z;
}

// quaternion norm (euclidian length of a 4d vector)
template<class Type>
inline Type Quaternion<Type>::Norm(void) const {
  return (Type)sqrt(q_w*q_w + q_x*q_x + q_y*q_y + q_z*q_z);
}

#if 0
// transcendental functions
template<class Type>
inline Quaternion<Type> Exp(const Quaternion<Type> &q)
{
  Type tAngle = (Type)sqrt(q.q_x*q.q_x + q.q_y*q.q_y + q.q_z*q.q_z);
  Type tSin = sin(tAngle);
  Type tCos = cos(tAngle);

  if (fabs(tSin)<0.001) {
    return Quaternion<Type>(tCos, q.q_x, q.q_y, q.q_z);
  } else {
    Type tRatio = tSin/tAngle;
    return Quaternion<Type>(tCos, q.q_x*tRatio, q.q_y*tRatio, q.q_z*tRatio);
  }
}
// transcendental functions
template<class Type>
inline Quaternion<Type> Log(const Quaternion<Type> &q)
{
  if (fabs(q.q_w)<1.0) {
    Type tAngle = acos(q.q_w);
    Type tSin   = sin(tAngle);
    if (fabs(tSin)>=0.001) {
      Type tRatio = tAngle/tSin;
      return Quaternion<Type>(Type(0), q.q_x*tRatio, q.q_y*tRatio, q.q_z*tRatio);
    }
  }

  return Quaternion<Type>(Type(0), q.q_x, q.q_y, q.q_z);
}

// spherical linear interpolation
template<class Type>
inline Quaternion<Type> Slerp(Type tT, 
  const Quaternion<Type> &q1, const Quaternion<Type> &q2)
{
  Type tCos = q1%q2;

  Quaternion<Type> qTemp;
  
  if (tCos<Type(0)) {
    tCos = -tCos;
    qTemp = -q2;
  } else  {
    qTemp = q2;
  }

  Type tF1, tF2;
  if ((Type(1)-tCos) > Type(0.001)) {
    // standard case (slerp)
    Type tAngle = acos(tCos);
    Type tSin   = sin(tAngle);
    tF1 = sin((Type(1)-tT)*tAngle)/tSin;
    tF2 = sin(tT*tAngle)/tSin;
  } else {        
    // linear interpolation
    tF1 = Type(1)-tT;
    tF2 = tT;
  }
  return q1*tF1 + qTemp*tF2;
}
// spherical quadratic interpolation
template<class Type>
inline Quaternion<Type> Squad(Type tT, 
  const Quaternion<Type> &q1, const Quaternion<Type> &q2,
  const Quaternion<Type> &qa, const Quaternion<Type> &qb)
{
  return Slerp(2*tT*(1-tT),Slerp(tT,q1,q2),Slerp(tT,qa,qb));
}
#endif

// conversion from euler angles
template<class Type>
void Quaternion<Type>::FromEuler(const Vector<Type, 3> &a)
{
  Quaternion<Type> qH;
  Quaternion<Type> qP;
  Quaternion<Type> qB;
  qH.q_w = Cos(a(1)/2);
  qH.q_x = 0;
  qH.q_y = Sin(a(1)/2);
  qH.q_z = 0;

  qP.q_w = Cos(a(2)/2);
  qP.q_x = Sin(a(2)/2);
  qP.q_y = 0;
  qP.q_z = 0;

  qB.q_w = Cos(a(3)/2);
  qB.q_x = 0;
  qB.q_y = 0;
  qB.q_z = Sin(a(3)/2);

  (*this) = qH*qP*qB;
}


// Check for almost, not really, but should be 0.0 values...
template<class Type>
Type Quaternion<Type>::EPS(Type orig) const
{
#if defined(PLATFORM_PANDORA) || defined(PLATFORM_PYRA)
    if ((orig <= 1e-4f) && (orig >= -1e-4f))
#else
    if ((orig <= 10e-6f) && (orig >= -10e-6f))
#endif
        return(0.0f);

    return(orig);
}

// conversion to matrix
template<class Type>
void Quaternion<Type>::ToMatrix(Matrix<Type, 3, 3> &m) const
{
  Type xx = 2*q_x*q_x; Type xy = 2*q_x*q_y; Type xz = 2*q_x*q_z;
  Type yy = 2*q_y*q_y; Type yz = 2*q_y*q_z; Type zz = 2*q_z*q_z;
  Type wx = 2*q_w*q_x; Type wy = 2*q_w*q_y; Type wz = 2*q_w*q_z;

  m(1,1) = 1.0f-EPS(yy+zz);  m(1,2) = EPS(xy-wz);       m(1,3) = EPS(xz+wy);
  m(2,1) = EPS(xy+wz);       m(2,2) = 1.0f-EPS(xx+zz);  m(2,3) = EPS(yz-wx);
  m(3,1) = EPS(xz-wy);       m(3,2) = EPS(yz+wx);       m(3,3) = 1.0f-EPS(xx+yy);
}

// conversion from matrix
template<class Type>
void Quaternion<Type>::FromMatrix(Matrix<Type, 3, 3> &m)
{
    Type trace = m(1,1)+m(2,2)+m(3,3);
    Type root;

    if ( trace > 0.0f )
    {
        // |w| > 1/2, may as well choose w > 1/2
        root = sqrt(trace+1.0f);  // 2w
        q_w = 0.5f*root;
        root = 0.5f/root;  // 1/(4w)
        q_x = (m(3,2)-m(2,3))*root;
        q_y = (m(1,3)-m(3,1))*root;
        q_z = (m(2,1)-m(1,2))*root;
    }
    else
    {
        // |w| <= 1/2
        static int next[3] = { 1, 2, 0 };
        int i = 0;
        if ( m(2,2) > m(1,1) )
            i = 1;
        if ( m(3,3) > m(i+1,i+1) )
            i = 2;
        int j = next[i];
        int k = next[j];

        root = sqrt(m(i+1,i+1)-m(j+1,j+1)-m(k+1,k+1)+1.0f);
        Type* quat[3] = { &q_x, &q_y, &q_z };
        *quat[i] = 0.5f*root;
        root = 0.5f/root;
        q_w = (m(k+1,j+1)-m(j+1,k+1))*root;
        *quat[j] = (m(j+1,i+1)+m(i+1,j+1))*root;
        *quat[k] = (m(k+1,i+1)+m(i+1,k+1))*root;
    }
}

// conversion to/from axis-angle
template<class Type>
void Quaternion<Type>::FromAxisAngle(const Vector<Type, 3> &n, Type a)
{
  Type tSin = sin(a/2);
  Type tCos = cos(a/2);

  q_x = n(1)*tSin;
  q_y = n(2)*tSin;
  q_z = n(3)*tSin;
  q_w = tCos;
}

template<class Type>
void Quaternion<Type>::ToAxisAngle(Vector<Type, 3> &n, Type &a)
{
  Type tCos = q_w;
  Type tSin = sqrt(Type(1)-tCos*tCos);
  a = 2*acos(tCos);

  // if angle is not zero
  if (Abs(tSin)>=0.001) {
    n(1) = q_x / tSin;
    n(2) = q_y / tSin;
    n(3) = q_z / tSin;
  // if angle is zero
  } else {
    n(1) = Type(1);
    n(2) = Type(0);
    n(3) = Type(0);
  }
}


#endif  /* include-once check. */

