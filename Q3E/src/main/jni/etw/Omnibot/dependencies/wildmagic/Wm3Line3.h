// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3LINE3_H
#define WM3LINE3_H

#include "Wm3Vector3.h"

namespace Wm3
{

template <class Real>
class Line3
{
public:
    // The line is represented as P+t*D where P is the line origin and D is
    // a unit-length direction vector.  The user must ensure that the
    // direction vector satisfies this condition.

    // construction
    Line3 ();  // uninitialized
    Line3 (const Vector3<Real>& rkOrigin, const Vector3<Real>& rkDirection);

    Vector3<Real> Origin, Direction;
};

#include "Wm3Line3.inl"

typedef Line3<float> Line3f;
typedef Line3<double> Line3d;

}

#endif

