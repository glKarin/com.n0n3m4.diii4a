// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2005.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3RAY3_H
#define WM3RAY3_H

#include "Wm3Vector3.h"

namespace Wm3
{

template <class Real>
class Ray3
{
public:
    // The ray is represented as P+t*D, where P is the ray origin, D is a
    // unit-length direction vector, and t >= 0.  The user must ensure that
    // the direction vector satisfies this condition.

    // construction
    Ray3 ();  // uninitialized
    Ray3 (const Vector3<Real>& rkOrigin, const Vector3<Real>& rkDirection);

    Vector3<Real> Origin, Direction;
};

#include "Wm3Ray3.inl"

typedef Ray3<float> Ray3f;
typedef Ray3<double> Ray3d;

}

#endif
