// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2005.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3DISTVECTOR3SEGMENT3_H
#define WM3DISTVECTOR3SEGMENT3_H

#include "Wm3Distance.h"
#include "Wm3Segment3.h"

namespace Wm3
{

template <class Real>
class DistVector3Segment3 : public Distance<Real,Vector3<Real> >
{
public:
    DistVector3Segment3 (const Vector3<Real>& rkVector,
        const Segment3<Real>& rkSegment);

    // object access
    const Vector3<Real>& GetVector () const;
    const Segment3<Real>& GetSegment () const;

    // static distance queries
    virtual Real Get ();
    virtual Real GetSquared ();

    // function calculations for dynamic distance queries
    virtual Real Get (Real fT, const Vector3<Real>& rkVelocity0,
        const Vector3<Real>& rkVelocity1);
    virtual Real GetSquared (Real fT, const Vector3<Real>& rkVelocity0,
        const Vector3<Real>& rkVelocity1);

private:
    using Distance<Real,Vector3<Real> >::m_kClosestPoint0;
    using Distance<Real,Vector3<Real> >::m_kClosestPoint1;

    const Vector3<Real>& m_rkVector;
    const Segment3<Real>& m_rkSegment;
};

typedef DistVector3Segment3<float> DistVector3Segment3f;
typedef DistVector3Segment3<double> DistVector3Segment3d;

}

#endif



