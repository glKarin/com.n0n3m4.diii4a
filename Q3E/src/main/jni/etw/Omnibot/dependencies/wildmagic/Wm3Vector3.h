// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2005.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3VECTOR3_H
#define WM3VECTOR3_H

#include "Wm3Math.h"
#include "Wm3Vector2.h"

namespace Wm3
{

template <class Real>
class Vector3
{
public:
    // construction
    Vector3 ();  // uninitialized
    Vector3 (Real fX, Real fY, Real fZ);
    Vector3 (const Vector3& rkV);
	Vector3 (const Real* afTuple);

    // coordinate access
    operator const Real* () const;
    operator Real* ();

    // assignment
    Vector3& operator= (const Vector3& rkV);

    // comparison
    bool operator== (const Vector3& rkV) const;
    bool operator!= (const Vector3& rkV) const;
    bool operator<  (const Vector3& rkV) const;
    bool operator<= (const Vector3& rkV) const;
    bool operator>  (const Vector3& rkV) const;
    bool operator>= (const Vector3& rkV) const;

    // arithmetic operations
    Vector3 operator+ (const Vector3& rkV) const;
    Vector3 operator- (const Vector3& rkV) const;
    Vector3 operator* (Real fScalar) const;
    Vector3 operator/ (Real fScalar) const;
    Vector3 operator- () const;

    // arithmetic updates
    Vector3& operator+= (const Vector3& rkV);
    Vector3& operator-= (const Vector3& rkV);
    Vector3& operator*= (Real fScalar);
    Vector3& operator/= (Real fScalar);

    // vector operations
    Real Length () const;
    Real SquaredLength () const;
    Real Dot (const Vector3& rkV) const;
    Real Normalize ();
	Vector3 Reflect(const Vector3& normal) const;
	Vector3 MidPoint(const Vector3& vec) const;
	void Truncate(Real length);
	Real ProjectOntoVector(const Vector3& rU);

	void RandomVector(Real magnitude);
	void FromSpherical(Real heading, Real pitch, Real radius);
	void ToSpherical(Real &heading, Real &pitch, Real &radius);

	Real GetPitch() const;
	Real XYHeading() const;
	void FromHeading(Real fHeading);

	Vector2<Real> As2d() const;
	Vector3<Real> Flatten(Real _z = 0.f) const;

    // The cross products are computed using the right-handed rule.  Be aware
    // that some graphics APIs use a left-handed rule.  If you have to compute
    // a cross product with these functions and send the result to the API
    // that expects left-handed, you will need to change sign on the vector
    // (replace each component value c by -c).
    Vector3 Cross (const Vector3& rkV) const;
    Vector3 UnitCross (const Vector3& rkV) const;

	Vector3 Perpendicular() const;

	bool IsZero() const;
	Vector3 AddZ(Real z) const;

	Real x, y, z;

	// special vectors
	static const Vector3 ZERO;
	static const Vector3 UNIT_X;
	static const Vector3 UNIT_Y;
	static const Vector3 UNIT_Z;
};

template <class Real>
Vector3<Real> operator* (Real fScalar, const Vector3<Real>& rkV);

template <class Real>
Real Length(const Vector3<Real>& v1, const Vector3<Real>& v2);
template <class Real>
Real Length2d(const Vector3<Real>& v1, const Vector3<Real>& v2);

template <class Real>
Real SquaredLength(const Vector3<Real>& v1, const Vector3<Real>& v2);
template <class Real>
Real SquaredLength2d(const Vector3<Real>& v1, const Vector3<Real>& v2);

template <class Real>
Vector3<Real> Normalize(const Vector3<Real>& v1);

template <class Real>
Vector3<Real> Interpolate(const Vector3<Real> &_1, const Vector3<Real> &_2, Real _t);

template <class Real>
Real PointToSegmentDistance(const Vector3<Real> &aPt, const Vector3<Real> &aSeg1, 
							const Vector3<Real> &aSeg2, Vector3<Real> &aOutClosest, Real &aOutTime);

// debugging output
template <class Real>
std::ostream& operator<< (std::ostream& rkOStr, const Vector3<Real>& rkV);

template <class Real>
std::istream& operator>> (std::istream& rkOStr, Vector3<Real>& rkV);

#include "Wm3Vector3.inl"

typedef Vector3<float> Vector3f;
typedef Vector3<double> Vector3d;

}

#endif



