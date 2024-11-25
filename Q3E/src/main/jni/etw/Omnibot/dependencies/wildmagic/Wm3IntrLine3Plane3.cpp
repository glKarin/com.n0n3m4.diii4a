// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#include "Wm3IntrLine3Plane3.h"

namespace Wm3
{
//----------------------------------------------------------------------------
template <class Real>
IntrLine3Plane3<Real>::IntrLine3Plane3 (const Line3<Real>& rkLine,
    const Plane3<Real>& rkPlane)
    :
    m_rkLine(rkLine),
    m_rkPlane(rkPlane)
{
}
//----------------------------------------------------------------------------
template <class Real>
const Line3<Real>& IntrLine3Plane3<Real>::GetLine () const
{
    return m_rkLine;
}
//----------------------------------------------------------------------------
template <class Real>
const Plane3<Real>& IntrLine3Plane3<Real>::GetPlane () const
{
    return m_rkPlane;
}
//----------------------------------------------------------------------------
template <class Real>
bool IntrLine3Plane3<Real>::Test ()
{
    Real fDdN = m_rkLine.Direction.Dot(m_rkPlane.Normal);
    if (Math<Real>::FAbs(fDdN) > Math<Real>::ZERO_TOLERANCE)
    {
        // The line is not parallel to the plane, so they must intersect.
        // The line parameter is *not* set, since this is a test-intersection
        // query.
        m_iIntersectionType = IT_POINT;
        return true;
    }

    // The line and plane are parallel.  Determine if they are numerically
    // close enough to be coincident.
    Real fSDistance = m_rkPlane.DistanceTo(m_rkLine.Origin);
    if (Math<Real>::FAbs(fSDistance) <= Math<Real>::ZERO_TOLERANCE)
    {
        m_iIntersectionType = IT_LINE;
        return true;
    }

    m_iIntersectionType = IT_EMPTY;
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool IntrLine3Plane3<Real>::Find ()
{
    Real fDdN = m_rkLine.Direction.Dot(m_rkPlane.Normal);
    Real fSDistance = m_rkPlane.DistanceTo(m_rkLine.Origin);
    if (Math<Real>::FAbs(fDdN) > Math<Real>::ZERO_TOLERANCE)
    {
        // The line is not parallel to the plane, so they must intersect.
        m_fLineT = -fSDistance/fDdN;
        m_iIntersectionType = IT_POINT;
        return true;
    }

    // The Line and plane are parallel.  Determine if they are numerically
    // close enough to be coincident.
    if (Math<Real>::FAbs(fSDistance) <= Math<Real>::ZERO_TOLERANCE)
    {
        // The line is coincident with the plane, so choose t = 0 for the
        // parameter.
        m_fLineT = (Real)0.0;
        m_iIntersectionType = IT_LINE;
        return true;
    }

    m_iIntersectionType = IT_EMPTY;
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
Real IntrLine3Plane3<Real>::GetLineT () const
{
    return m_fLineT;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template
class IntrLine3Plane3<float>;

template
class IntrLine3Plane3<double>;
//----------------------------------------------------------------------------
}

