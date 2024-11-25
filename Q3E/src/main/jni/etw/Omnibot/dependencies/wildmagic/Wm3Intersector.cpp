// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#include "Wm3Intersector.h"

namespace Wm3
{
//----------------------------------------------------------------------------
template <class Real, class TVector>
Intersector<Real,TVector>::Intersector ()
{
    m_fContactTime = (Real)0.0;
    m_iIntersectionType = IT_EMPTY;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
Intersector<Real,TVector>::~Intersector ()
{
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
Real Intersector<Real,TVector>::GetContactTime () const
{
    return m_fContactTime;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
int Intersector<Real,TVector>::GetIntersectionType () const
{
    return m_iIntersectionType;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
bool Intersector<Real,TVector>::Test ()
{
    // stub for derived class
    assert(false);
    return false;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
bool Intersector<Real,TVector>::Find ()
{
    // stub for derived class
    assert(false);
    return false;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
bool Intersector<Real,TVector>::Test (Real, const TVector&, const TVector&)
{
    // stub for derived class
    assert(false);
    return false;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
bool Intersector<Real,TVector>::Find (Real, const TVector&, const TVector&)
{
    // stub for derived class
    assert(false);
    return false;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template
class Intersector<float,Vector2f>;

template
class Intersector<float,Vector3f>;

template
class Intersector<double,Vector2d>;

template
class Intersector<double,Vector3d>;
//----------------------------------------------------------------------------
}

