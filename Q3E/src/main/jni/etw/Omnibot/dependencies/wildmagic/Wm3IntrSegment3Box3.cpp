// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#include "Wm3IntrSegment3Box3.h"
#include "Wm3IntrLine3Box3.h"

namespace Wm3
{
//----------------------------------------------------------------------------
template <class Real>
IntrSegment3Box3<Real>::IntrSegment3Box3 (const Segment3<Real>& rkSegment,
    const Box3<Real>& rkBox, bool bSolid)
    :
    m_rkSegment(rkSegment),
    m_rkBox(rkBox)
{
    m_bSolid = bSolid;
}
//----------------------------------------------------------------------------
template <class Real>
const Segment3<Real>& IntrSegment3Box3<Real>::GetSegment () const
{
    return m_rkSegment;
}
//----------------------------------------------------------------------------
template <class Real>
const Box3<Real>& IntrSegment3Box3<Real>::GetBox () const
{
    return m_rkBox;
}
//----------------------------------------------------------------------------
template <class Real>
bool IntrSegment3Box3<Real>::Test ()
{
    Real afAWdU[3], afADdU[3], afAWxDdU[3], fRhs;

    Vector3<Real> kDiff = m_rkSegment.Origin - m_rkBox.Center;

    afAWdU[0] = Math<Real>::FAbs(m_rkSegment.Direction.Dot(m_rkBox.Axis[0]));
    afADdU[0] = Math<Real>::FAbs(kDiff.Dot(m_rkBox.Axis[0]));
    fRhs = m_rkBox.Extent[0] + m_rkSegment.Extent*afAWdU[0];
    if (afADdU[0] > fRhs)
    {
        return false;
    }

    afAWdU[1] = Math<Real>::FAbs(m_rkSegment.Direction.Dot(m_rkBox.Axis[1]));
    afADdU[1] = Math<Real>::FAbs(kDiff.Dot(m_rkBox.Axis[1]));
    fRhs = m_rkBox.Extent[1] + m_rkSegment.Extent*afAWdU[1];
    if (afADdU[1] > fRhs)
    {
        return false;
    }

    afAWdU[2] = Math<Real>::FAbs(m_rkSegment.Direction.Dot(m_rkBox.Axis[2]));
    afADdU[2] = Math<Real>::FAbs(kDiff.Dot(m_rkBox.Axis[2]));
    fRhs = m_rkBox.Extent[2] + m_rkSegment.Extent*afAWdU[2];
    if (afADdU[2] > fRhs)
    {
        return false;
    }

    Vector3<Real> kWxD = m_rkSegment.Direction.Cross(kDiff);

    afAWxDdU[0] = Math<Real>::FAbs(kWxD.Dot(m_rkBox.Axis[0]));
    fRhs = m_rkBox.Extent[1]*afAWdU[2] + m_rkBox.Extent[2]*afAWdU[1];
    if (afAWxDdU[0] > fRhs)
    {
        return false;
    }

    afAWxDdU[1] = Math<Real>::FAbs(kWxD.Dot(m_rkBox.Axis[1]));
    fRhs = m_rkBox.Extent[0]*afAWdU[2] + m_rkBox.Extent[2]*afAWdU[0];
    if (afAWxDdU[1] > fRhs)
    {
        return false;
    }

    afAWxDdU[2] = Math<Real>::FAbs(kWxD.Dot(m_rkBox.Axis[2]));
    fRhs = m_rkBox.Extent[0]*afAWdU[1] + m_rkBox.Extent[1]*afAWdU[0];
    if (afAWxDdU[2] > fRhs)
    {
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool IntrSegment3Box3<Real>::Find ()
{
    Real fT0 = -m_rkSegment.Extent, fT1 = m_rkSegment.Extent;
    return IntrLine3Box3<Real>::DoClipping(fT0,fT1,m_rkSegment.Origin,
        m_rkSegment.Direction,m_rkBox,m_bSolid,m_iQuantity,m_akPoint,
        m_iIntersectionType);
}
//----------------------------------------------------------------------------
template <class Real>
int IntrSegment3Box3<Real>::GetQuantity () const
{
    return m_iQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& IntrSegment3Box3<Real>::GetPoint (int i) const
{
    assert(0 <= i && i < m_iQuantity);
    return m_akPoint[i];
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template
class IntrSegment3Box3<float>;

template
class IntrSegment3Box3<double>;
//----------------------------------------------------------------------------
}

