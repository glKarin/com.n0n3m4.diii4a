// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2005.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3VECTOR2_H
#define WM3VECTOR2_H

#include "Wm3Math.h"

namespace Wm3
{

template <class Real>
class Vector2
{
public:
    // construction
    Vector2 ();  // uninitialized
    Vector2 (Real fX, Real fY);
    Vector2 (const Vector2& rkV);
	Vector2 (const Real* afTuple);

    // assignment
    Vector2& operator= (const Vector2& rkV);

    // comparison
    bool operator== (const Vector2& rkV) const;
    bool operator!= (const Vector2& rkV) const;
    bool operator<  (const Vector2& rkV) const;
    bool operator<= (const Vector2& rkV) const;
    bool operator>  (const Vector2& rkV) const;
    bool operator>= (const Vector2& rkV) const;

    // arithmetic operations
    Vector2 operator+ (const Vector2& rkV) const;
    Vector2 operator- (const Vector2& rkV) const;
    Vector2 operator* (Real fScalar) const;
    Vector2 operator/ (Real fScalar) const;
    Vector2 operator- () const;

    // arithmetic updates
    Vector2& operator+= (const Vector2& rkV);
    Vector2& operator-= (const Vector2& rkV);
    Vector2& operator*= (Real fScalar);
    Vector2& operator/= (Real fScalar);

    // vector operations
    Real Length () const;
    Real SquaredLength () const;
    Real Dot (const Vector2& rkV) const;
    Real Normalize ();
	Vector2 Reflect(const Vector2& normal) const;
	Vector2 MidPoint(const Vector2& vec) const;
	Vector2 &Truncate(Real length);
	Real ProjectOntoVector(const Vector2& rU);
	Real Get2dHeading() const;

    // returns (y,-x)
    Vector2 Perp () const;

    // returns (y,-x)/sqrt(x*x+y*y)
    Vector2 UnitPerp () const;

    // returns DotPerp((x,y),(V.x,V.y)) = x*V.y - y*V.x
    Real DotPerp (const Vector2& rkV) const;

	Real x, y;

	// special vectors
	static const Vector2 ZERO;
	static const Vector2 UNIT_X;
	static const Vector2 UNIT_Y;
};

// arithmetic operations
template <class Real>
Vector2<Real> operator* (Real fScalar, const Vector2<Real>& rkV);

// debugging output
template <class Real>
std::ostream& operator<< (std::ostream& rkOStr, const Vector2<Real>& rkV);

template <class Real>
std::istream& operator>> (std::istream& rkOStr, Vector2<Real>& rkV);

template <class Real>
Real Length(const Vector2<Real>& v1, const Vector2<Real>& v2);

template <class Real>
Real SquaredLength(const Vector2<Real>& v1, const Vector2<Real>& v2);

template <class Real>
Vector2<Real> Normalize(const Vector2<Real>& v1);

template <class Real>
Vector2<Real> Interpolate(const Vector2<Real> &_1, const Vector2<Real> &_2, Real _t);

template <class Real>
Real PointToSegmentDistance(const Vector2<Real> &aPt, const Vector2<Real> &aSeg1, 
							 const Vector2<Real> &aSeg2, Vector2<Real> &aOutClosest, Real &aOutTime);

#include "Wm3Vector2.inl"

typedef Vector2<float> Vector2f;
typedef Vector2<double> Vector2d;

}

#endif



