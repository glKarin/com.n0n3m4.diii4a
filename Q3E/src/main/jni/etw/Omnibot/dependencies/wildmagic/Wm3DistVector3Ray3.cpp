// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2005.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#include "Wm3DistVector3Ray3.h"

namespace Wm3
{
//----------------------------------------------------------------------------
template <class Real>
DistVector3Ray3<Real>::DistVector3Ray3 (const Vector3<Real>& rkVector,
    const Ray3<Real>& rkRay)
    :
    m_rkVector(rkVector),
    m_rkRay(rkRay)
{
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& DistVector3Ray3<Real>::GetVector () const
{
    return m_rkVector;
}
//----------------------------------------------------------------------------
template <class Real>
const Ray3<Real>& DistVector3Ray3<Real>::GetRay () const
{
    return m_rkRay;
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Ray3<Real>::Get ()
{
    Real fSqrDist = GetSquared();
    return Math<Real>::Sqrt(fSqrDist);
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Ray3<Real>::GetSquared ()
{
    Vector3<Real> kDiff = m_rkVector - m_rkRay.Origin;
    Real fParam = m_rkRay.Direction.Dot(kDiff);
    if (fParam > (Real)0.0)
    {
        m_kClosestPoint1 = m_rkRay.Origin + fParam*m_rkRay.Direction;
    }
    else
    {
        m_kClosestPoint1 = m_rkRay.Origin;
    }

    m_kClosestPoint0 = m_rkVector;
    kDiff = m_kClosestPoint1 - m_kClosestPoint0;
    return kDiff.SquaredLength();
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Ray3<Real>::Get (Real fT, const Vector3<Real>& rkVelocity0,
    const Vector3<Real>& rkVelocity1)
{
    Vector3<Real> kMVector = m_rkVector + fT*rkVelocity0;
    Vector3<Real> kMOrigin = m_rkRay.Origin + fT*rkVelocity1;
    Ray3<Real> kMRay(kMOrigin,m_rkRay.Direction);
    return DistVector3Ray3<Real>(kMVector,kMRay).Get();
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Ray3<Real>::GetSquared (Real fT,
    const Vector3<Real>& rkVelocity0, const Vector3<Real>& rkVelocity1)
{
    Vector3<Real> kMVector = m_rkVector + fT*rkVelocity0;
    Vector3<Real> kMOrigin = m_rkRay.Origin + fT*rkVelocity1;
    Ray3<Real> kMRay(kMOrigin,m_rkRay.Direction);
    return DistVector3Ray3<Real>(kMVector,kMRay).GetSquared();
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template
class DistVector3Ray3<float>;

template
class DistVector3Ray3<double>;
//----------------------------------------------------------------------------
}
