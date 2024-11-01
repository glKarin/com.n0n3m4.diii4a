// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2005.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#include "Wm3Quaternion.h"
using namespace Wm3;

template<> const Quaternion<float>
    Quaternion<float>::IDENTITY(1.0f,0.0f,0.0f,0.0f);
template<> const Quaternion<float>
    Quaternion<float>::ZERO(0.0f,0.0f,0.0f,0.0f);
template<> int Quaternion<float>::ms_iNext[3] = { 1, 2, 0 };

template<> const Quaternion<double>
    Quaternion<double>::IDENTITY(1.0,0.0,0.0,0.0);
template<> const Quaternion<double>
    Quaternion<double>::ZERO(0.0,0.0,0.0,0.0);
template<> int Quaternion<double>::ms_iNext[3] = { 1, 2, 0 };



