// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3INTRLINE3PLANE3_H
#define WM3INTRLINE3PLANE3_H

#include "Wm3Intersector.h"
#include "Wm3Line3.h"
#include "Wm3Plane3.h"

namespace Wm3
{

template <class Real>
class IntrLine3Plane3 : public Intersector<Real,Vector3<Real> >
{
public:
    IntrLine3Plane3 (const Line3<Real>& rkLine, const Plane3<Real>& rkPlane);

    // object access
    const Line3<Real>& GetLine () const;
    const Plane3<Real>& GetPlane () const;

    // test-intersection query
    virtual bool Test ();

    // Find-intersection query.  The point of intersection is
    // P = origin + t*direction.
    virtual bool Find ();
    Real GetLineT () const;

private:
    using Intersector<Real,Vector3<Real> >::IT_EMPTY;
    using Intersector<Real,Vector3<Real> >::IT_POINT;
    using Intersector<Real,Vector3<Real> >::IT_LINE;
    using Intersector<Real,Vector3<Real> >::m_iIntersectionType;

    // the objects to intersect
    const Line3<Real>& m_rkLine;
    const Plane3<Real>& m_rkPlane;

    // information about the intersection set
    Real m_fLineT;
};

typedef IntrLine3Plane3<float> IntrLine3Plane3f;
typedef IntrLine3Plane3<double> IntrLine3Plane3d;

}

#endif

