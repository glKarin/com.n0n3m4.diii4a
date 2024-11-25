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
Vector2<Real>::Vector2 ()
{
    // uninitialized for performance in array construction
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::Vector2 (Real fX, Real fY)
{
    x = fX;
    y = fY;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::Vector2 (const Vector2& rkV)
{
    x = rkV.x;
    y = rkV.y;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::Vector2 (const Real* afTuple)
{
	x = afTuple[0];
	y = afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator= (const Vector2& rkV)
{
    x = rkV.x;
    y = rkV.y;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator== (const Vector2& rkV) const
{
    return (x == rkV.x) && (y == rkV.y);
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator!= (const Vector2& rkV) const
{
    return (x != rkV.x) && (y != rkV.y);
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator< (const Vector2& rkV) const
{
    return (x < rkV.x) && (y < rkV.y);
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator<= (const Vector2& rkV) const
{
    return (x <= rkV.x) && (y <= rkV.y);
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator> (const Vector2& rkV) const
{
    return (x > rkV.x) && (y > rkV.y);
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator>= (const Vector2& rkV) const
{
    return (x >= rkV.x) && (y >= rkV.y);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::operator+ (const Vector2& rkV) const
{
    return Vector2(
        x+rkV.x,
        y+rkV.y);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::operator- (const Vector2& rkV) const
{
    return Vector2(
        x-rkV.x,
        y-rkV.y);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::operator* (Real fScalar) const
{
    return Vector2(
        fScalar * x,
        fScalar * y);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::operator/ (Real fScalar) const
{
    Vector2 kQuot;

    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        kQuot.x = fInvScalar * x;
        kQuot.y = fInvScalar * y;
    }
    else
    {
        kQuot.x = Math<Real>::MAX_REAL;
        kQuot.y = Math<Real>::MAX_REAL;
    }

    return kQuot;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::operator- () const
{
    return Vector2(
        -x,
        -y);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> operator* (Real fScalar, const Vector2<Real>& rkV)
{
    return Vector2<Real>(
        fScalar*rkV.x,
        fScalar*rkV.y);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator+= (const Vector2& rkV)
{
    x += rkV.x;
    y += rkV.y;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator-= (const Vector2& rkV)
{
    x -= rkV.x;
    y -= rkV.y;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator*= (Real fScalar)
{
    x *= fScalar;
    y *= fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator/= (Real fScalar)
{
    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        x *= fInvScalar;
        y *= fInvScalar;
    }
    else
    {
        x = Math<Real>::MAX_REAL;
        y = Math<Real>::MAX_REAL;
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::Length () const
{
    return Math<Real>::Sqrt(
        x * x +
        y * y);
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::SquaredLength () const
{
    return
        x * x +
        y * y;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::Dot (const Vector2& rkV) const
{
    return
        x * rkV.x +
        y * rkV.y;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::Normalize ()
{
    Real fLength = Length();

    if ( fLength > Math<Real>::ZERO_TOLERANCE )
    {
        Real fInvLength = ((Real)1.0)/fLength;
        x *= fInvLength;
        y *= fInvLength;
    }
    else
    {
        fLength = (Real)0.0;
        x = (Real)0.0;
        y = (Real)0.0;
    }

    return fLength;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::Perp () const
{
    return Vector2(y, -x);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::UnitPerp () const
{
    Vector2 kPerp(y, -x);
    kPerp.Normalize();
    return kPerp;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::DotPerp (const Vector2& rkV) const
{
    return x*rkV.y - y*rkV.x;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::Reflect(const Vector2& normal) const
{
	return Vector2( *this - ( 2 * this->Dot(normal) * normal ) );
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::MidPoint(const Vector2& vec) const
{
	return Vector2<Real>( 
		( x + vec.x ) * (Real)0.5, 
		( y + vec.y ) * (Real)0.5 );
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> &Vector2<Real>::Truncate(Real length)
{
	if(length == 0.0f)
		x = y = 0.0f;
	else
	{
		Real length2 = SquaredLength();
		if ( length2 > length * length ) 
		{
			Real ilength = length * Math<Real>::InvSqrt( length2 );
			x *= ilength;
			y *= ilength;
		}
	}
	return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::ProjectOntoVector(const Vector2& rU)
{
	Real Scale = this->Dot(rU) / rU.SquaredLength();	// Find the scale of this vector on U
	(*this)=rU;											// Copy U onto this vector
	(*this)*=Scale;										// Use the previously calculated scale to get the right length.
	return Scale;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::Get2dHeading() const 
{
	return -Math<Real>::Atan2( -x, y ); 
}
//----------------------------------------------------------------------------
template <class Real>
std::ostream& operator<< (std::ostream& rkOStr, const Vector2<Real>& rkV)
{
     return rkOStr << rkV.y << ' ' << rkV.y;
}
//----------------------------------------------------------------------------
template <class Real>
std::istream& operator>> (std::istream& rkOStr, Vector2<Real>& rkV)
{
	return rkOStr >> rkV.x >> rkV.y;
}
//----------------------------------------------------------------------------
template <class Real>
Real Length(const Vector2<Real>& v1, const Vector2<Real>& v2)
{
	return (v1-v2).Length();
}
//----------------------------------------------------------------------------
template <class Real>
Real SquaredLength(const Vector2<Real>& v1, const Vector2<Real>& v2)
{
	return (v1-v2).SquaredLength();
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Normalize(const Vector2<Real>& v1)
{
	Vector2<Real> v = v1;
	v.Normalize();
	return v;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Interpolate(const Vector2<Real> &_1, const Vector2<Real> &_2, Real _t)
{
	return _1 + (_2 - _1) * _t;
}
//----------------------------------------------------------------------------
template <class Real>
Real PointToSegmentDistance(const Vector2<Real> &aPt, const Vector2<Real> &aSeg1, 
							 const Vector2<Real> &aSeg2, Vector2<Real> &aOutClosest, Real &aOutTime)
{
	Vector2<Real> vSegmentNormal = aSeg2 - aSeg1;
	vSegmentNormal.Normalize();
	Vector2<Real> vDiff = aPt - aSeg1;
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
