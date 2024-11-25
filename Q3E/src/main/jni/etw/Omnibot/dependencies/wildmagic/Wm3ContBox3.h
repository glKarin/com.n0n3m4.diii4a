// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2005.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3CONTBOX3_H
#define WM3CONTBOX3_H

#include "Wm3Box3.h"

namespace Wm3
{

// Compute the smallest axis-aligned bounding box of the points.  If you want
// only a subset of the input points, provide an array of Boolean values,
// where abValid[i] is set to 'true' if you want akPoint[i] to be included in
// the calculations.  If you provide a Boolean array with all 'false' values,
// then the returned box has center (0,0,0), axes (1,0,0), (0,1,0), and
// (0,0,1), and extents all -1.
template <class Real> 
Box3<Real> ContAlignedBox (int iQuantity, const Vector3<Real>* akPoint,
    const bool* abValid = 0);

// Test for containment.  Let X = C + y0*U0 + y1*U1 + y2*U2 where C is the
// box center and U0, U1, U2 are the orthonormal axes of the box.  X is in
// the box if |y_i| <= E_i for all i where E_i are the extents of the box.
template <class Real> 
bool InBox (const Vector3<Real>& rkPoint, const Box3<Real>& rkBox);

// Construct an oriented box that contains two other oriented boxes.  The
// result is not guaranteed to be the minimum volume box containing the
// input boxes.
template <class Real> 
Box3<Real> MergeBoxes (const Box3<Real>& rkBox0, const Box3<Real>& rkBox1);

}

#endif



