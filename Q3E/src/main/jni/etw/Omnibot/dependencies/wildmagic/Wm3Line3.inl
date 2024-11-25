// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

//----------------------------------------------------------------------------
template <class Real>
Line3<Real>::Line3 ()
{
    // uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Line3<Real>::Line3 (const Vector3<Real>& rkOrigin,
    const Vector3<Real>& rkDirection)
    :
    Origin(rkOrigin),
    Direction(rkDirection)
{
}
//----------------------------------------------------------------------------

