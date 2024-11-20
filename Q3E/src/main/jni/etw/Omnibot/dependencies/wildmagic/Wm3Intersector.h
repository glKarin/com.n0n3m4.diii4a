// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3INTERSECTOR_H
#define WM3INTERSECTOR_H

#include "Wm3LinComp.h"
#include "Wm3Vector2.h"
#include "Wm3Vector3.h"

namespace Wm3
{

template <class Real, class TVector>
class Intersector
{
public:
    // abstract base class
    virtual ~Intersector ();

    // Static intersection queries.  The default implementations return
    // 'false'.  The Find query produces a set of intersection.  The derived
    // class is responsible for providing access to that set, since the nature
    // of the set is dependent on the object types.
    virtual bool Test ();
    virtual bool Find ();

    // Dynamic intersection queries.  The default implementations return
    // 'false'.  The Find query produces a set of first contact.  The derived
    // class is responsible for providing access to that set, since the nature
    // of the set is dependent on the object types.
    virtual bool Test (Real fTMax, const TVector& rkVelocity0,
        const TVector& rkVelocity1);
    virtual bool Find (Real fTMax, const TVector& rkVelocity0,
        const TVector& rkVelocity1);

    // The time at which two objects are in first contact for the dynamic
    // intersection queries.
    Real GetContactTime () const;

    // information about the intersection set
    enum
    {
        IT_EMPTY = LinComp<Real>::CT_EMPTY,
        IT_POINT = LinComp<Real>::CT_POINT,
        IT_SEGMENT = LinComp<Real>::CT_SEGMENT,
        IT_RAY = LinComp<Real>::CT_RAY,
        IT_LINE = LinComp<Real>::CT_LINE,
        IT_POLYGON,
        IT_PLANE,
        IT_POLYHEDRON,
        IT_OTHER
    };
    int GetIntersectionType () const;

protected:
    Intersector ();

    Real m_fContactTime;
    int m_iIntersectionType;
};

typedef Intersector<float, Vector2<float> > Intersector2f;
typedef Intersector<float, Vector3<float> > Intersector3f;
typedef Intersector<double, Vector2<double> > Intersector2d;
typedef Intersector<double, Vector3<double> > Intersector3d;

}

#endif

