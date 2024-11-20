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
Segment3<Real>::Segment3 ()
{
    // uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Segment3<Real>::Segment3 (const Vector3<Real>& rkOrigin,
    const Vector3<Real>& rkDirection, Real fExtent)
    :
    Origin(rkOrigin),
    Direction(rkDirection),
    Extent(fExtent)
{
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Segment3<Real>::GetPosEnd () const
{
    return Origin + Extent*Direction;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Segment3<Real>::GetNegEnd () const
{
    return Origin - Extent*Direction;
}
//----------------------------------------------------------------------------



