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
Plane3<Real>::Plane3 ()
{
    // uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Plane3<Real>::Plane3 (const Plane3& rkPlane)
    :
    Normal(rkPlane.Normal)
{
    Constant = rkPlane.Constant;
}
//----------------------------------------------------------------------------
template <class Real>
Plane3<Real>::Plane3 (const Vector3<Real>& rkNormal, Real fConstant)
    :
    Normal(rkNormal)
{
    Constant = fConstant;
}
//----------------------------------------------------------------------------
template <class Real>
Plane3<Real>::Plane3 (const Vector3<Real>& rkNormal, const Vector3<Real>& rkP)
    :
    Normal(rkNormal)
{
    Constant = rkNormal.Dot(rkP);
}
//----------------------------------------------------------------------------
template <class Real>
Plane3<Real>::Plane3 (const Vector3<Real>& rkP0, const Vector3<Real>& rkP1,
    const Vector3<Real>& rkP2)
{
    Vector3<Real> kEdge1 = rkP1 - rkP0;
    Vector3<Real> kEdge2 = rkP2 - rkP0;
    Normal = kEdge1.UnitCross(kEdge2);
    Constant = Normal.Dot(rkP0);
}
//----------------------------------------------------------------------------
template <class Real>
Plane3<Real>& Plane3<Real>::operator= (const Plane3& rkPlane)
{
    Normal = rkPlane.Normal;
    Constant = rkPlane.Constant;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
bool Plane3<Real>::operator==(const Plane3& rkPlane) const
{
	return Normal == rkPlane.Normal && Constant == rkPlane.Constant;
}
//----------------------------------------------------------------------------
template <class Real>
Real Plane3<Real>::DistanceTo (const Vector3<Real>& rkP) const
{
    return Normal.Dot(rkP) + Constant;
}
//----------------------------------------------------------------------------
template <class Real>
int Plane3<Real>::WhichSide (const Vector3<Real>& rkQ) const
{
    Real fDistance = DistanceTo(rkQ);

    if ( fDistance < (Real)0.0 )
        return -1;

    if ( fDistance > (Real)0.0 )
        return +1;

    return 0;
}
//----------------------------------------------------------------------------
template <class Real>
std::ostream& operator<< (std::ostream& rkOStr, const Plane3<Real>& rkV)
{
	return rkOStr << rkV.Normal << ' ' << rkV.Constant;
}
//----------------------------------------------------------------------------
template <class Real>
std::istream& operator>> (std::istream& rkOStr, Plane3<Real>& rkV)
{
	return rkOStr >> rkV.Normal >> rkV.Constant;
}
//----------------------------------------------------------------------------



