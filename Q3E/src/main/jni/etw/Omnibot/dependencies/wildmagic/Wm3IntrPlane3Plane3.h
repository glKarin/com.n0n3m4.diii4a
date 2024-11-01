// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3INTRPLANE3PLANE3_H
#define WM3INTRPLANE3PLANE3_H

#include "Wm3Intersector.h"
#include "Wm3Plane3.h"
#include "Wm3Line3.h"

namespace Wm3
{

template <class Real>
class IntrPlane3Plane3 : public Intersector<Real,Vector3<Real> >
{
public:
    IntrPlane3Plane3 (const Plane3<Real>& rkPlane0,
        const Plane3<Real>& rkPlane1);

    // object access
    const Plane3<Real>& GetPlane0 () const;
    const Plane3<Real>& GetPlane1 () const;

    // static intersection queries
    virtual bool Test ();
    virtual bool Find ();

    // dynamic intersection queries
    virtual bool Test (Real fTMax, const Vector3<Real>& rkVelocity0,
        const Vector3<Real>& rkVelocity1);
    virtual bool Find (Real fTMax, const Vector3<Real>& rkVelocity0,
        const Vector3<Real>& rkVelocity1);

    // Information about the intersection set.  Only get the specific object
    // of intersection corresponding to the intersection type (IT_LINE or
    // IT_PLANE).
    const Line3<Real>& GetIntersectionLine () const;
    const Plane3<Real>& GetIntersectionPlane () const;

protected:
    using Intersector<Real,Vector3<Real> >::IT_EMPTY;
    using Intersector<Real,Vector3<Real> >::IT_LINE;
    using Intersector<Real,Vector3<Real> >::IT_PLANE;
    using Intersector<Real,Vector3<Real> >::m_iIntersectionType;
    using Intersector<Real,Vector3<Real> >::m_fContactTime;

    // the objects to intersect
    const Plane3<Real>& m_rkPlane0;
    const Plane3<Real>& m_rkPlane1;

    // information about the intersection set
    Line3<Real> m_kIntrLine;
    Plane3<Real> m_kIntrPlane;
};

typedef IntrPlane3Plane3<float> IntrPlane3Plane3f;
typedef IntrPlane3Plane3<double> IntrPlane3Plane3d;

}

#endif

