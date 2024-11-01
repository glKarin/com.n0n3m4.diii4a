// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3INTRSEGMENT3PLANE3_H
#define WM3INTRSEGMENT3PLANE3_H

#include "Wm3Intersector.h"
#include "Wm3Segment3.h"
#include "Wm3Plane3.h"

namespace Wm3
{

template <class Real>
class IntrSegment3Plane3 : public Intersector<Real,Vector3<Real> >
{
public:
    IntrSegment3Plane3 (const Segment3<Real>& rkSegment,
        const Plane3<Real>& rkPlane);

    // object access
    const Segment3<Real>& GetSegment () const;
    const Plane3<Real>& GetPlane () const;

    // test-intersection query
    virtual bool Test ();

    // Find-intersection query.  The point of intersection is
    // P = origin + t*direction, with |t| <= e, where e is the segment
    // extent.
    virtual bool Find ();
    Real GetSegmentT () const;

private:
    using Intersector<Real,Vector3<Real> >::IT_EMPTY;
    using Intersector<Real,Vector3<Real> >::IT_POINT;
    using Intersector<Real,Vector3<Real> >::IT_SEGMENT;
    using Intersector<Real,Vector3<Real> >::m_iIntersectionType;

    // the objects to intersect
    const Segment3<Real>& m_rkSegment;
    const Plane3<Real>& m_rkPlane;

    // information about the intersection set
    Real m_fSegmentT;
};

typedef IntrSegment3Plane3<float> IntrSegment3Plane3f;
typedef IntrSegment3Plane3<double> IntrSegment3Plane3d;

}

#endif

