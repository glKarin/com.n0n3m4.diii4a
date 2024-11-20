// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2005.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#include "Wm3ContBox3.h"
#include "Wm3Quaternion.h"
#include <set>

namespace Wm3
{
//----------------------------------------------------------------------------
template <class Real>
Box3<Real> ContAlignedBox (int iQuantity, const Vector3<Real>* akPoint,
    const bool* abValid)
{
    Box3<Real> kBox(Vector3<Real>::ZERO,Vector3<Real>::UNIT_X,
        Vector3<Real>::UNIT_Y,Vector3<Real>::UNIT_Z,(Real)1.0,(Real)1.0,
        (Real)1.0);

    int i;

    Vector3<Real> kMin, kMax;
    if ( !abValid )
    {
        kMin = akPoint[0];
        kMax = kMin;

        for (i = 1; i < iQuantity; i++)
        {
            if ( akPoint[i].x < kMin.x )
                kMin.x = akPoint[i].x;
            else if ( akPoint[i].x > kMax.x )
                kMax.x = akPoint[i].x;

            if ( akPoint[i].y < kMin.y )
                kMin.y = akPoint[i].y;
            else if ( akPoint[i].y > kMax.y )
                kMax.y = akPoint[i].y;

            if ( akPoint[i].z < kMin.z )
                kMin.z = akPoint[i].z;
            else if ( akPoint[i].z > kMax.z )
                kMax.z = akPoint[i].z;
        }
    }
    else
    {
        for (i = 0; i < iQuantity; i++)
        {
            if ( abValid[i] )
            {
                kMin = akPoint[i];
                kMax = kMin;
                break;
            }
        }
        if ( i == iQuantity )
        {
            kBox.Extent[0] = (Real)-1.0;
            kBox.Extent[1] = (Real)-1.0;
            kBox.Extent[2] = (Real)-1.0;
            return kBox;
        }

        for (++i; i < iQuantity; i++)
        {
            if ( abValid[i] )
            {
                if ( akPoint[i].x < kMin.x )
                    kMin.x = akPoint[i].x;
                else if ( akPoint[i].x > kMax.x )
                    kMax.x = akPoint[i].x;

                if ( akPoint[i].y < kMin.y )
                    kMin.y = akPoint[i].y;
                else if ( akPoint[i].y > kMax.y )
                    kMax.y = akPoint[i].y;

                if ( akPoint[i].z < kMin.z )
                    kMin.z = akPoint[i].z;
                else if ( akPoint[i].z > kMax.z )
                    kMax.z = akPoint[i].z;
            }
        }
    }

    kBox.Center = ((Real)0.5)*(kMin + kMax);
    Vector3<Real> kHalfDiagonal = ((Real)0.5)*(kMax - kMin);
    for (i = 0; i < 3; i++)
        kBox.Extent[i] = kHalfDiagonal[i];

    return kBox;
}
//----------------------------------------------------------------------------
template <class Real>
bool InBox (const Vector3<Real>& rkPoint, const Box3<Real>& rkBox)
{
    Vector3<Real> kDiff = rkPoint - rkBox.Center;
    for (int i = 0; i < 3; i++)
    {
        Real fCoeff = kDiff.Dot(rkBox.Axis[i]);
        if ( Math<Real>::FAbs(fCoeff) > rkBox.Extent[i] )
            return false;
    }
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
Box3<Real> MergeBoxes (const Box3<Real>& rkBox0, const Box3<Real>& rkBox1)
{
    // construct a box that contains the input boxes
    Box3<Real> kBox;

    // The first guess at the box center.  This value will be updated later
    // after the input box vertices are projected onto axes determined by an
    // average of box axes.
    kBox.Center = ((Real)0.5)*(rkBox0.Center + rkBox1.Center);

    // A box's axes, when viewed as the columns of a matrix, form a rotation
    // matrix.  The input box axes are converted to quaternions.  The average
    // quaternion is computed, then normalized to unit length.  The result is
    // the slerp of the two input quaternions with t-value of 1/2.  The result
    // is converted back to a rotation matrix and its columns are selected as
    // the merged box axes.
    Quaternion<Real> kQ0, kQ1;
    kQ0.FromRotationMatrix(rkBox0.Axis);
    kQ1.FromRotationMatrix(rkBox1.Axis);
    if ( kQ0.Dot(kQ1) < (Real)0.0 )
        kQ1 = -kQ1;

    Quaternion<Real> kQ = kQ0 + kQ1;
    Real fInvLength = Math<Real>::InvSqrt(kQ.Dot(kQ));
    kQ = fInvLength*kQ;
    kQ.ToRotationMatrix(kBox.Axis);

    // Project the input box vertices onto the merged-box axes.  Each axis
    // D[i] containing the current center C has a minimum projected value
    // pmin[i] and a maximum projected value pmax[i].  The corresponding end
    // points on the axes are C+pmin[i]*D[i] and C+pmax[i]*D[i].  The point C
    // is not necessarily the midpoint for any of the intervals.  The actual
    // box center will be adjusted from C to a point C' that is the midpoint
    // of each interval,
    //   C' = C + sum_{i=0}^2 0.5*(pmin[i]+pmax[i])*D[i]
    // The box extents are
    //   e[i] = 0.5*(pmax[i]-pmin[i])

    int i, j;
    Real fDot;
    Vector3<Real> akVertex[8], kDiff;
    Vector3<Real> kMin = Vector3<Real>::ZERO;
    Vector3<Real> kMax = Vector3<Real>::ZERO;

    rkBox0.ComputeVertices(akVertex);
    for (i = 0; i < 8; i++)
    {
        kDiff = akVertex[i] - kBox.Center;
        for (j = 0; j < 3; j++)
        {
            fDot = kDiff.Dot(kBox.Axis[j]);
            if ( fDot > kMax[j] )
                kMax[j] = fDot;
            else if ( fDot < kMin[j] )
                kMin[j] = fDot;
        }
    }

    rkBox1.ComputeVertices(akVertex);
    for (i = 0; i < 8; i++)
    {
        kDiff = akVertex[i] - kBox.Center;
        for (j = 0; j < 3; j++)
        {
            fDot = kDiff.Dot(kBox.Axis[j]);
            if ( fDot > kMax[j] )
                kMax[j] = fDot;
            else if ( fDot < kMin[j] )
                kMin[j] = fDot;
        }
    }

    // [kMin,kMax] is the axis-aligned box in the coordinate system of the
    // merged box axes.  Update the current box center to be the center of
    // the new box.  Compute the extens based on the new center.
    for (j = 0; j < 3; j++)
    {
        kBox.Center += (((Real)0.5)*(kMax[j]+kMin[j]))*kBox.Axis[j];
        kBox.Extent[j] = ((Real)0.5)*(kMax[j]-kMin[j]);
    }

    return kBox;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
// float
template 
Box3<float> ContAlignedBox<float> (int, const Vector3<float>*,
    const bool*);

template 
bool InBox<float> (const Vector3<float>&, const Box3<float>&);

template 
Box3<float> MergeBoxes<float> (const Box3<float>&, const Box3<float>&);

// double
template 
Box3<double> ContAlignedBox<double> (int, const Vector3<double>*,
    const bool*);

template 
bool InBox<double> (const Vector3<double>&, const Box3<double>&);

template 
Box3<double> MergeBoxes<double> (const Box3<double>&, const Box3<double>&);
//----------------------------------------------------------------------------
}

