// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#include "Wm3IntrSegment3Plane3.h"
#include "Wm3IntrLine3Plane3.h"

namespace Wm3
{
//----------------------------------------------------------------------------
template <class Real>
IntrSegment3Plane3<Real>::IntrSegment3Plane3 (const Segment3<Real>& rkSegment,
    const Plane3<Real>& rkPlane)
    :
    m_rkSegment(rkSegment),
    m_rkPlane(rkPlane)
{
}
//----------------------------------------------------------------------------
template <class Real>
const Segment3<Real>& IntrSegment3Plane3<Real>::GetSegment () const
{
    return m_rkSegment;
}
//----------------------------------------------------------------------------
template <class Real>
const Plane3<Real>& IntrSegment3Plane3<Real>::GetPlane () const
{
    return m_rkPlane;
}
//----------------------------------------------------------------------------
template <class Real>
bool IntrSegment3Plane3<Real>::Test ()
{
    Vector3<Real> kP0 = m_rkSegment.GetNegEnd();
    Real fSDistance0 = m_rkPlane.DistanceTo(kP0);
    if (Math<Real>::FAbs(fSDistance0) <= Math<Real>::ZERO_TOLERANCE)
    {
        fSDistance0 = (Real)0.0;
    }

    Vector3<Real> kP1 = m_rkSegment.GetPosEnd();
    Real fSDistance1 = m_rkPlane.DistanceTo(kP1);
    if (Math<Real>::FAbs(fSDistance1) <= Math<Real>::ZERO_TOLERANCE)
    {
        fSDistance1 = (Real)0.0;
    }

    Real fProd = fSDistance0*fSDistance1;
    if (fProd < (Real)0.0)
    {
        // The segment passes through the plane.
        m_iIntersectionType = IT_POINT;
        return true;
    }

    if (fProd > (Real)0.0)
    {
        // The segment is on one side of the plane.
        m_iIntersectionType = IT_EMPTY;
        return false;
    }

    if (fSDistance0 != (Real)0.0 || fSDistance1 != (Real)0.0)
    {
        // A segment end point touches the plane.
        m_iIntersectionType = IT_POINT;
        return true;
    }

    // The segment is coincident with the plane.
    m_iIntersectionType = IT_SEGMENT;
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool IntrSegment3Plane3<Real>::Find ()
{
    Line3<Real> kLine(m_rkSegment.Origin,m_rkSegment.Direction);
    IntrLine3Plane3<Real> kIntr(kLine,m_rkPlane);
    if (kIntr.Find())
    {
        // The line intersects the plane, but possibly at a point that is
        // not on the segment.
        m_iIntersectionType = kIntr.GetIntersectionType();
        m_fSegmentT = kIntr.GetLineT();
        return Math<Real>::FAbs(m_fSegmentT) <= m_rkSegment.Extent;
    }

    m_iIntersectionType = IT_EMPTY;
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
Real IntrSegment3Plane3<Real>::GetSegmentT () const
{
    return m_fSegmentT;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template
class IntrSegment3Plane3<float>;

template
class IntrSegment3Plane3<double>;
//----------------------------------------------------------------------------
}

