// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2005.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

//----------------------------------------------------------------------------

template <class Real>
Vector3<Real>::Vector3 ()
{
    // uninitialized for performance in array construction
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>::Vector3 (Real fX, Real fY, Real fZ)
{
    x = fX;
    y = fY;
    z = fZ;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>::Vector3 (const Vector3& rkV)
{
    x = rkV.x;
    y = rkV.y;
    z = rkV.z;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>::Vector3 (const Real* afTuple)
{
	x = afTuple[0];
	y = afTuple[1];
	z = afTuple[2];
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>::operator const Real* (void) const
{
	return (Real*)this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>::operator Real* (void)
{
	return (Real*)this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>& Vector3<Real>::operator= (const Vector3& rkV)
{
    x = rkV.x;
    y = rkV.y;
    z = rkV.z;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector3<Real>::operator== (const Vector3& rkV) const
{
    return (x == rkV.x) && (y == rkV.y) && (z == rkV.z);
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector3<Real>::operator!= (const Vector3& rkV) const
{
     return (x != rkV.x) || (y != rkV.y) || (z != rkV.z);
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector3<Real>::operator< (const Vector3& rkV) const
{
     return (x < rkV.x) && (y < rkV.y) && (z < rkV.z);
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector3<Real>::operator<= (const Vector3& rkV) const
{
     return (x <= rkV.x) && (y <= rkV.y) && (z <= rkV.z);
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector3<Real>::operator> (const Vector3& rkV) const
{
    return (x > rkV.x) && (y > rkV.y) && (z > rkV.z);
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector3<Real>::operator>= (const Vector3& rkV) const
{
     return (x >= rkV.x) && (y >= rkV.y) && (z >= rkV.z);
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::operator+ (const Vector3& rkV) const
{
    return Vector3(
        x+rkV.x,
        y+rkV.y,
        z+rkV.z);
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::operator- (const Vector3& rkV) const
{
    return Vector3(
        x-rkV.x,
        y-rkV.y,
        z-rkV.z);
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::operator* (Real fScalar) const
{
    return Vector3(
        fScalar*x,
        fScalar*y,
        fScalar*z);
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::operator/ (Real fScalar) const
{
    Vector3 kQuot;

    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        kQuot.x = fInvScalar*x;
        kQuot.y = fInvScalar*y;
        kQuot.z = fInvScalar*z;
    }
    else
    {
        kQuot.x = Math<Real>::MAX_REAL;
        kQuot.y = Math<Real>::MAX_REAL;
        kQuot.z = Math<Real>::MAX_REAL;
    }

    return kQuot;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::operator- () const
{
    return Vector3(
        -x,
        -y,
        -z);
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> operator* (Real fScalar, const Vector3<Real>& rkV)
{
    return Vector3<Real>(
        fScalar*rkV[0],
        fScalar*rkV[1],
        fScalar*rkV[2]);
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>& Vector3<Real>::operator+= (const Vector3& rkV)
{
    x += rkV.x;
    y += rkV.y;
    z += rkV.z;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>& Vector3<Real>::operator-= (const Vector3& rkV)
{
    x -= rkV.x;
    y -= rkV.y;
    z -= rkV.z;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>& Vector3<Real>::operator*= (Real fScalar)
{
    x *= fScalar;
    y *= fScalar;
    z *= fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real>& Vector3<Real>::operator/= (Real fScalar)
{
    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        x *= fInvScalar;
        y *= fInvScalar;
        z *= fInvScalar;
    }
    else
    {
        x = Math<Real>::MAX_REAL;
        y = Math<Real>::MAX_REAL;
        z = Math<Real>::MAX_REAL;
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector3<Real>::Length () const
{
    return Math<Real>::Sqrt(
        x*x +
        y*y +
        z*z);
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector3<Real>::SquaredLength () const
{
    return
        x*x +
        y*y +
        z*z;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector3<Real>::Dot (const Vector3& rkV) const
{
    return
        x*rkV.x +
        y*rkV.y +
        z*rkV.z;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector3<Real>::Normalize ()
{
    Real fLength = Length();

    if ( fLength > Math<Real>::ZERO_TOLERANCE )
    {
        Real fInvLength = ((Real)1.0)/fLength;
        x *= fInvLength;
        y *= fInvLength;
        z *= fInvLength;
    }
    else
    {
        fLength = (Real)0.0;
        x = (Real)0.0;
        y = (Real)0.0;
        z = (Real)0.0;
    }

    return fLength;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::Reflect(const Vector3& normal) const
{
	return Vector3( *this - ( 2 * this->Dot(normal) * normal ) );
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::MidPoint(const Vector3& vec) const
{
	return Vector3( 
		( x + vec.x ) * (Real)0.5, 
		( y + vec.y ) * (Real)0.5, 
		( z + vec.z ) * (Real)0.5 );
}
//----------------------------------------------------------------------------
template <class Real>
void Vector3<Real>::Truncate(Real length)
{
	if(length == 0.0f)
		x = y = z = 0.0f;
	else
	{
		Real length2 = SquaredLength();
		if ( length2 > length * length ) 
		{
			Real ilength = length * Math<Real>::InvSqrt( length2 );
			x *= ilength;
			y *= ilength;
			z *= ilength;
		}
	}
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector3<Real>::ProjectOntoVector(const Vector3& rU)
{
	Real Scale = this->Dot(rU) / rU.SquaredLength();	// Find the scale of this vector on U
	(*this)=rU;											// Copy U onto this vector
	(*this)*=Scale;										// Use the previously calculated scale to get the right length.
	return Scale;
}
//----------------------------------------------------------------------------
template <class Real>
void Vector3<Real>::RandomVector(Real magnitude)
{
	x = Math<Real>::SymmetricRandom();
	y = Math<Real>::SymmetricRandom();
	z = Math<Real>::SymmetricRandom();

	Normalize();

	*this *= magnitude;
}
//----------------------------------------------------------------------------
template <class Real>
void Vector3<Real>::FromSpherical( Real heading, Real pitch, Real radius ) 
{ 
	float fST, fCT, fSP, fCP;
	Math<Real>::SinCos(heading, fST, fCT);
	Math<Real>::SinCos(pitch, fSP, fCP);
	*this = radius * Vector3<Real>(fCP*fST, fCP*fCT, fSP);
}
//----------------------------------------------------------------------------
template <class Real>
void Vector3<Real>::ToSpherical( Real &heading, Real &pitch, Real &radius )
{
	// reference vector is 0,0,1
	radius = Length();
	pitch = radius > 0.0f ? Math<Real>::ASin(z/radius) : 0.0f;
	heading = Math<Real>::ATan2(x, y);
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector3<Real>::GetPitch() const
{
	Vector3<Real> vTmp = *this;
	vTmp.Normalize();
	return Math<Real>::ASin(vTmp.z);
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector3<Real>::XYHeading() const
{
	return -Math<Real>::ATan2(-x, y);
}
//----------------------------------------------------------------------------
template <class Real>
void Vector3<Real>::FromHeading( Real fHeading )
{
	Math<Real>::SinCos(fHeading, x, y);
	z = (Real)0.0;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::Cross (const Vector3& rkV) const
{
    return Vector3(
        y*rkV.z - z*rkV.y,
        z*rkV.x - x*rkV.z,
        x*rkV.y - y*rkV.x);
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::UnitCross (const Vector3& rkV) const
{
    Vector3 kCross(
        y*rkV.z - z*rkV.y,
        z*rkV.x - x*rkV.z,
        x*rkV.y - y*rkV.x);
    kCross.Normalize();
    return kCross;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::Perpendicular() const
{
	const Real fSquareZero = (Real)1e-06 * (Real)1e-06;

	Vector3 perp = UnitCross(Vector3::UNIT_X);

	// Check length
	if(perp.SquaredLength() < fSquareZero)
	{
		/* This vector is the Y axis multiplied by a scalar, so we have
		to use another axis. */
		perp = UnitCross( Vector3::UNIT_Y );
	}
	return perp;
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector3<Real>::IsZero () const
{
	return *this == Vector3<Real>::ZERO;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector3<Real>::As2d () const
{
	return Vector2<Real>(x,y);
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Vector3<Real>::Flatten(Real _z) const
{
	return Vector3<Real>(x,y,_z);
}
//----------------------------------------------------------------------------
template <class Real>
std::ostream& operator<< (std::ostream& rkOStr, const Vector3<Real>& rkV)
{
	return rkOStr << rkV.x << ' ' << rkV.y << ' ' << rkV.z;
}
//----------------------------------------------------------------------------
template <class Real>
std::istream& operator>> (std::istream& rkOStr, Vector3<Real>& rkV)
{
	return rkOStr >> rkV.x >> rkV.y >> rkV.z;
}
//----------------------------------------------------------------------------
template <class Real>
Real Length(const Vector3<Real>& v1, const Vector3<Real>& v2)
{
	return (v1-v2).Length();
}
//----------------------------------------------------------------------------
template <class Real>
Real Length2d(const Vector3<Real>& v1, const Vector3<Real>& v2)
{
	return (v1-v2).As2d().Length();
}
//----------------------------------------------------------------------------
template <class Real>
Real SquaredLength(const Vector3<Real>& v1, const Vector3<Real>& v2)
{
	return (v1-v2).SquaredLength();
}
//----------------------------------------------------------------------------
template <class Real>
Real SquaredLength2d(const Vector3<Real>& v1, const Vector3<Real>& v2)
{
	return (v1-v2).As2d().SquaredLength();
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Normalize(const Vector3<Real>& v1)
{
	Vector3<Real> v = v1;
	v.Normalize();
	return v;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Interpolate(const Vector3<Real> &_v1, const Vector3<Real> &_v2, Real _t)
{
	return _v1 + (_v2 - _v1) * _t;
}
//----------------------------------------------------------------------------
template <class Real>
Real PointToSegmentDistance(const Vector3<Real> &aPt, const Vector3<Real> &aSeg1, 
							 const Vector3<Real> &aSeg2, Vector3<Real> &aOutClosest, Real &aOutTime)
{
	Vector3<Real> vSegmentNormal = aSeg2 - aSeg1;
	vSegmentNormal.Normalize();
	Vector3<Real> vDiff = aPt - aSeg1;
	Real fSegmentLength = Length(aSeg2, aSeg1);

	// Edge cases if the point isn't on the segment, distance must be
	// to the nearest end point.
	float fProjection = vSegmentNormal.Dot(vDiff);
	if(fProjection < 0)
	{
		aOutClosest = aSeg1;
		aOutTime = 0.0;
	}
	else if(fProjection > fSegmentLength)
	{
		aOutClosest = aSeg2;
		aOutTime = 1.0;
	}
	else
	{
		aOutClosest = aSeg1 + vSegmentNormal * fProjection;
		aOutTime = fProjection / fSegmentLength;
	}
	return Length(aPt, aOutClosest);
}
//----------------------------------------------------------------------------
template<class Real>
Vector3<Real> Vector3<Real>::AddZ(Real _z) const
{
	return Vector3(x, y, z + _z);
}
