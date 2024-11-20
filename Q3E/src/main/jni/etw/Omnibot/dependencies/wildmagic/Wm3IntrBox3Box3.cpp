// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#include "Wm3IntrBox3Box3.h"

namespace Wm3
{
//----------------------------------------------------------------------------
template <class Real>
IntrBox3Box3<Real>::IntrBox3Box3 (const Box3<Real>& rkBox0,
    const Box3<Real>& rkBox1)
    :
    m_rkBox0(rkBox0),
    m_rkBox1(rkBox1)
{
}
//----------------------------------------------------------------------------
template <class Real>
const Box3<Real>& IntrBox3Box3<Real>::GetBox0 () const
{
    return m_rkBox0;
}
//----------------------------------------------------------------------------
template <class Real>
const Box3<Real>& IntrBox3Box3<Real>::GetBox1 () const
{
    return m_rkBox1;
}
//----------------------------------------------------------------------------
template <class Real>
bool IntrBox3Box3<Real>::Test ()
{
    // Cutoff for cosine of angles between box axes.  This is used to catch
    // the cases when at least one pair of axes are parallel.  If this happens,
    // there is no need to test for separation along the Cross(A[i],B[j])
    // directions.
    const Real fCutoff = (Real)1.0 - Math<Real>::ZERO_TOLERANCE;
    bool bExistsParallelPair = false;
    int i;

    // convenience variables
    const Vector3<Real>* akA = m_rkBox0.Axis;
    const Vector3<Real>* akB = m_rkBox1.Axis;
    const Real* afEA = m_rkBox0.Extent;
    const Real* afEB = m_rkBox1.Extent;

    // compute difference of box centers, D = C1-C0
    Vector3<Real> kD = m_rkBox1.Center - m_rkBox0.Center;

    Real aafC[3][3];     // matrix C = A^T B, c_{ij} = Dot(A_i,B_j)
    Real aafAbsC[3][3];  // |c_{ij}|
    Real afAD[3];        // Dot(A_i,D)
    Real fR0, fR1, fR;   // interval radii and distance between centers
    Real fR01;           // = R0 + R1

    // axis C0+t*A0
    for (i = 0; i < 3; i++)
    {
        aafC[0][i] = akA[0].Dot(akB[i]);
        aafAbsC[0][i] = Math<Real>::FAbs(aafC[0][i]);
        if (aafAbsC[0][i] > fCutoff)
        {
            bExistsParallelPair = true;
        }
    }
    afAD[0] = akA[0].Dot(kD);
    fR = Math<Real>::FAbs(afAD[0]);
    fR1 = afEB[0]*aafAbsC[0][0]+afEB[1]*aafAbsC[0][1]+afEB[2]*aafAbsC[0][2];
    fR01 = afEA[0] + fR1;
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*A1
    for (i = 0; i < 3; i++)
    {
        aafC[1][i] = akA[1].Dot(akB[i]);
        aafAbsC[1][i] = Math<Real>::FAbs(aafC[1][i]);
        if (aafAbsC[1][i] > fCutoff)
        {
            bExistsParallelPair = true;
        }
    }
    afAD[1] = akA[1].Dot(kD);
    fR = Math<Real>::FAbs(afAD[1]);
    fR1 = afEB[0]*aafAbsC[1][0]+afEB[1]*aafAbsC[1][1]+afEB[2]*aafAbsC[1][2];
    fR01 = afEA[1] + fR1;
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*A2
    for (i = 0; i < 3; i++)
    {
        aafC[2][i] = akA[2].Dot(akB[i]);
        aafAbsC[2][i] = Math<Real>::FAbs(aafC[2][i]);
        if (aafAbsC[2][i] > fCutoff)
        {
            bExistsParallelPair = true;
        }
    }
    afAD[2] = akA[2].Dot(kD);
    fR = Math<Real>::FAbs(afAD[2]);
    fR1 = afEB[0]*aafAbsC[2][0]+afEB[1]*aafAbsC[2][1]+afEB[2]*aafAbsC[2][2];
    fR01 = afEA[2] + fR1;
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*B0
    fR = Math<Real>::FAbs(akB[0].Dot(kD));
    fR0 = afEA[0]*aafAbsC[0][0]+afEA[1]*aafAbsC[1][0]+afEA[2]*aafAbsC[2][0];
    fR01 = fR0 + afEB[0];
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*B1
    fR = Math<Real>::FAbs(akB[1].Dot(kD));
    fR0 = afEA[0]*aafAbsC[0][1]+afEA[1]*aafAbsC[1][1]+afEA[2]*aafAbsC[2][1];
    fR01 = fR0 + afEB[1];
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*B2
    fR = Math<Real>::FAbs(akB[2].Dot(kD));
    fR0 = afEA[0]*aafAbsC[0][2]+afEA[1]*aafAbsC[1][2]+afEA[2]*aafAbsC[2][2];
    fR01 = fR0 + afEB[2];
    if (fR > fR01)
    {
        return false;
    }

    // At least one pair of box axes was parallel, so the separation is
    // effectively in 2D where checking the "edge" normals is sufficient for
    // the separation of the boxes.
    if (bExistsParallelPair)
    {
        return true;
    }

    // axis C0+t*A0xB0
    fR = Math<Real>::FAbs(afAD[2]*aafC[1][0]-afAD[1]*aafC[2][0]);
    fR0 = afEA[1]*aafAbsC[2][0] + afEA[2]*aafAbsC[1][0];
    fR1 = afEB[1]*aafAbsC[0][2] + afEB[2]*aafAbsC[0][1];
    fR01 = fR0 + fR1;
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*A0xB1
    fR = Math<Real>::FAbs(afAD[2]*aafC[1][1]-afAD[1]*aafC[2][1]);
    fR0 = afEA[1]*aafAbsC[2][1] + afEA[2]*aafAbsC[1][1];
    fR1 = afEB[0]*aafAbsC[0][2] + afEB[2]*aafAbsC[0][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*A0xB2
    fR = Math<Real>::FAbs(afAD[2]*aafC[1][2]-afAD[1]*aafC[2][2]);
    fR0 = afEA[1]*aafAbsC[2][2] + afEA[2]*aafAbsC[1][2];
    fR1 = afEB[0]*aafAbsC[0][1] + afEB[1]*aafAbsC[0][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*A1xB0
    fR = Math<Real>::FAbs(afAD[0]*aafC[2][0]-afAD[2]*aafC[0][0]);
    fR0 = afEA[0]*aafAbsC[2][0] + afEA[2]*aafAbsC[0][0];
    fR1 = afEB[1]*aafAbsC[1][2] + afEB[2]*aafAbsC[1][1];
    fR01 = fR0 + fR1;
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*A1xB1
    fR = Math<Real>::FAbs(afAD[0]*aafC[2][1]-afAD[2]*aafC[0][1]);
    fR0 = afEA[0]*aafAbsC[2][1] + afEA[2]*aafAbsC[0][1];
    fR1 = afEB[0]*aafAbsC[1][2] + afEB[2]*aafAbsC[1][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*A1xB2
    fR = Math<Real>::FAbs(afAD[0]*aafC[2][2]-afAD[2]*aafC[0][2]);
    fR0 = afEA[0]*aafAbsC[2][2] + afEA[2]*aafAbsC[0][2];
    fR1 = afEB[0]*aafAbsC[1][1] + afEB[1]*aafAbsC[1][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*A2xB0
    fR = Math<Real>::FAbs(afAD[1]*aafC[0][0]-afAD[0]*aafC[1][0]);
    fR0 = afEA[0]*aafAbsC[1][0] + afEA[1]*aafAbsC[0][0];
    fR1 = afEB[1]*aafAbsC[2][2] + afEB[2]*aafAbsC[2][1];
    fR01 = fR0 + fR1;
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*A2xB1
    fR = Math<Real>::FAbs(afAD[1]*aafC[0][1]-afAD[0]*aafC[1][1]);
    fR0 = afEA[0]*aafAbsC[1][1] + afEA[1]*aafAbsC[0][1];
    fR1 = afEB[0]*aafAbsC[2][2] + afEB[2]*aafAbsC[2][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
    {
        return false;
    }

    // axis C0+t*A2xB2
    fR = Math<Real>::FAbs(afAD[1]*aafC[0][2]-afAD[0]*aafC[1][2]);
    fR0 = afEA[0]*aafAbsC[1][2] + afEA[1]*aafAbsC[0][2];
    fR1 = afEB[0]*aafAbsC[2][1] + afEB[1]*aafAbsC[2][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
    {
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool IntrBox3Box3<Real>::Test (Real fTMax,
    const Vector3<Real>& rkVelocity0, const Vector3<Real>& rkVelocity1)
{
    if (rkVelocity0 == rkVelocity1)
    {
        if (Test())
        {
            m_fContactTime = (Real)0.0;
            return true;
        }
        return false;
    }

    // Cutoff for cosine of angles between box axes.  This is used to catch
    // the cases when at least one pair of axes are parallel.  If this happens,
    // there is no need to include the cross-product axes for separation.
    const Real fCutoff = (Real)1.0 - Math<Real>::ZERO_TOLERANCE;
    bool bExistsParallelPair = false;

    // convenience variables
    const Vector3<Real>* akA = m_rkBox0.Axis;
    const Vector3<Real>* akB = m_rkBox1.Axis;
    const Real* afEA = m_rkBox0.Extent;
    const Real* afEB = m_rkBox1.Extent;
    Vector3<Real> kD = m_rkBox1.Center - m_rkBox0.Center;
    Vector3<Real> kW = rkVelocity1 - rkVelocity0;
    Real aafC[3][3];     // matrix C = A^T B, c_{ij} = Dot(A_i,B_j)
    Real aafAbsC[3][3];  // |c_{ij}|
    Real afAD[3];        // Dot(A_i,D)
    Real afAW[3];        // Dot(A_i,W)
    Real fMin0, fMax0, fMin1, fMax1, fCenter, fRadius, fSpeed;
    int i, j;

    m_fContactTime = (Real)0.0;
    Real fTLast = Math<Real>::MAX_REAL;

    // axes C0+t*A[i]
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            aafC[i][j] = akA[i].Dot(akB[j]);
            aafAbsC[i][j] = Math<Real>::FAbs(aafC[i][j]);
            if (aafAbsC[i][j] > fCutoff)
            {
                bExistsParallelPair = true;
            }
        }
        afAD[i] = akA[i].Dot(kD);
        afAW[i] = akA[i].Dot(kW);
        fMin0 = -afEA[i];
        fMax0 = +afEA[i];
        fRadius = afEB[0]*aafAbsC[i][0] + afEB[1]*aafAbsC[i][1] + 
            afEB[2]*aafAbsC[i][2];
        fMin1 = afAD[i] - fRadius;
        fMax1 = afAD[i] + fRadius;
        fSpeed = afAW[i];
        if (IsSeparated(fMin0,fMax0,fMin1,fMax1,fSpeed,fTMax,
            m_fContactTime,fTLast))
        {
            return false;
        }
    }

    // axes C0+t*B[i]
    for (i = 0; i < 3; i++)
    {
        fRadius = afEA[0]*aafAbsC[0][i] + afEA[1]*aafAbsC[1][i] +
            afEA[2]*aafAbsC[2][i];
        fMin0 = -fRadius;
        fMax0 = +fRadius;
        fCenter = akB[i].Dot(kD);
        fMin1 = fCenter - afEB[i];
        fMax1 = fCenter + afEB[i];
        fSpeed = kW.Dot(akB[i]);
        if (IsSeparated(fMin0,fMax0,fMin1,fMax1,fSpeed,fTMax,
            m_fContactTime,fTLast))
        {
            return false;
        }
    }

    // At least one pair of box axes was parallel, so the separation is
    // effectively in 2D where checking the "edge" normals is sufficient for
    // the separation of the boxes.
    if (bExistsParallelPair)
    {
        return true;
    }

    // axis C0+t*A0xB0
    fRadius = afEA[1]*aafAbsC[2][0] + afEA[2]*aafAbsC[1][0];
    fMin0 = -fRadius;
    fMax0 = +fRadius;
    fCenter = afAD[2]*aafC[1][0] - afAD[1]*aafC[2][0];
    fRadius = afEB[1]*aafAbsC[0][2] + afEB[2]*aafAbsC[0][1];
    fMin1 = fCenter - fRadius;
    fMax1 = fCenter + fRadius;
    fSpeed = afAW[2]*aafC[1][0] - afAW[1]*aafC[2][0];
    if (IsSeparated(fMin0,fMax0,fMin1,fMax1,fSpeed,fTMax,m_fContactTime,
        fTLast))
    {
        return false;
    }

    // axis C0+t*A0xB1
    fRadius = afEA[1]*aafAbsC[2][1] + afEA[2]*aafAbsC[1][1];
    fMin0 = -fRadius;
    fMax0 = +fRadius;
    fCenter = afAD[2]*aafC[1][1] - afAD[1]*aafC[2][1];
    fRadius = afEB[0]*aafAbsC[0][2] + afEB[2]*aafAbsC[0][0];
    fMin1 = fCenter - fRadius;
    fMax1 = fCenter + fRadius;
    fSpeed = afAW[2]*aafC[1][1] - afAW[1]*aafC[2][1];
    if (IsSeparated(fMin0,fMax0,fMin1,fMax1,fSpeed,fTMax,m_fContactTime,
        fTLast))
    {
        return false;
    }

    // axis C0+t*A0xB2
    fRadius = afEA[1]*aafAbsC[2][2] + afEA[2]*aafAbsC[1][2];
    fMin0 = -fRadius;
    fMax0 = +fRadius;
    fCenter = afAD[2]*aafC[1][2] - afAD[1]*aafC[2][2];
    fRadius = afEB[0]*aafAbsC[0][1] + afEB[1]*aafAbsC[0][0];
    fMin1 = fCenter - fRadius;
    fMax1 = fCenter + fRadius;
    fSpeed = afAW[2]*aafC[1][2] - afAW[1]*aafC[2][2];
    if (IsSeparated(fMin0,fMax0,fMin1,fMax1,fSpeed,fTMax,m_fContactTime,
        fTLast))
    {
        return false;
    }

    // axis C0+t*A1xB0
    fRadius = afEA[0]*aafAbsC[2][0] + afEA[2]*aafAbsC[0][0];
    fMin0 = -fRadius;
    fMax0 = +fRadius;
    fCenter = afAD[0]*aafC[2][0] - afAD[2]*aafC[0][0];
    fRadius = afEB[1]*aafAbsC[1][2] + afEB[2]*aafAbsC[1][1];
    fMin1 = fCenter - fRadius;
    fMax1 = fCenter + fRadius;
    fSpeed = afAW[0]*aafC[2][0] - afAW[2]*aafC[0][0];
    if (IsSeparated(fMin0,fMax0,fMin1,fMax1,fSpeed,fTMax,m_fContactTime,
        fTLast))
    {
        return false;
    }

    // axis C0+t*A1xB1
    fRadius = afEA[0]*aafAbsC[2][1] + afEA[2]*aafAbsC[0][1];
    fMin0 = -fRadius;
    fMax0 = +fRadius;
    fCenter = afAD[0]*aafC[2][1] - afAD[2]*aafC[0][1];
    fRadius = afEB[0]*aafAbsC[1][2] + afEB[2]*aafAbsC[1][0];
    fMin1 = fCenter - fRadius;
    fMax1 = fCenter + fRadius;
    fSpeed = afAW[0]*aafC[2][1] - afAW[2]*aafC[0][1];
    if (IsSeparated(fMin0,fMax0,fMin1,fMax1,fSpeed,fTMax,m_fContactTime,
        fTLast))
    {
        return false;
    }

    // axis C0+t*A1xB2
    fRadius = afEA[0]*aafAbsC[2][2] + afEA[2]*aafAbsC[0][2];
    fMin0 = -fRadius;
    fMax0 = +fRadius;
    fCenter = afAD[0]*aafC[2][2] - afAD[2]*aafC[0][2];
    fRadius = afEB[0]*aafAbsC[1][1] + afEB[1]*aafAbsC[1][0];
    fMin1 = fCenter - fRadius;
    fMax1 = fCenter + fRadius;
    fSpeed = afAW[0]*aafC[2][2] - afAW[2]*aafC[0][2];
    if (IsSeparated(fMin0,fMax0,fMin1,fMax1,fSpeed,fTMax,m_fContactTime,
        fTLast))
    {
        return false;
    }

    // axis C0+t*A2xB0
    fRadius = afEA[0]*aafAbsC[1][0] + afEA[1]*aafAbsC[0][0];
    fMin0 = -fRadius;
    fMax0 = +fRadius;
    fCenter = afAD[1]*aafC[0][0] - afAD[0]*aafC[1][0];
    fRadius = afEB[1]*aafAbsC[2][2] + afEB[2]*aafAbsC[2][1];
    fMin1 = fCenter - fRadius;
    fMax1 = fCenter + fRadius;
    fSpeed = afAW[1]*aafC[0][0] - afAW[0]*aafC[1][0];
    if (IsSeparated(fMin0,fMax0,fMin1,fMax1,fSpeed,fTMax,m_fContactTime,
        fTLast))
    {
        return false;
    }

    // axis C0+t*A2xB1
    fRadius = afEA[0]*aafAbsC[1][1] + afEA[1]*aafAbsC[0][1];
    fMin0 = -fRadius;
    fMax0 = +fRadius;
    fCenter = afAD[1]*aafC[0][1] - afAD[0]*aafC[1][1];
    fRadius = afEB[0]*aafAbsC[2][2] + afEB[2]*aafAbsC[2][0];
    fMin1 = fCenter - fRadius;
    fMax1 = fCenter + fRadius;
    fSpeed = afAW[1]*aafC[0][1] - afAW[0]*aafC[1][1];
    if (IsSeparated(fMin0,fMax0,fMin1,fMax1,fSpeed,fTMax,m_fContactTime,
        fTLast))
    {
        return false;
    }

    // axis C0+t*A2xB2
    fRadius = afEA[0]*aafAbsC[1][2] + afEA[1]*aafAbsC[0][2];
    fMin0 = -fRadius;
    fMax0 = +fRadius;
    fCenter = afAD[1]*aafC[0][2] - afAD[0]*aafC[1][2];
    fRadius = afEB[0]*aafAbsC[2][1] + afEB[1]*aafAbsC[2][0];
    fMin1 = fCenter - fRadius;
    fMax1 = fCenter + fRadius;
    fSpeed = afAW[1]*aafC[0][2] - afAW[0]*aafC[1][2];
    if (IsSeparated(fMin0,fMax0,fMin1,fMax1,fSpeed,fTMax,m_fContactTime,
        fTLast))
    {
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool IntrBox3Box3<Real>::IsSeparated (Real fMin0, Real fMax0, Real fMin1,
    Real fMax1, Real fSpeed, Real fTMax, Real& rfTFirst, Real& rfTLast)
{
    Real fInvSpeed, fT;

    if (fMax1 < fMin0) // box1 initially on left of box0
    {
        if (fSpeed <= (Real)0.0)
        {
            // The projection intervals are moving apart.
            return true;
        }
        fInvSpeed = ((Real)1.0)/fSpeed;

        fT = (fMin0 - fMax1)*fInvSpeed;
        if (fT > rfTFirst)
        {
            rfTFirst = fT;
        }

        if (rfTFirst > fTMax)
        {
            // Intervals do not intersect during the specified time.
            return true;
        }

        fT = (fMax0 - fMin1)*fInvSpeed;
        if (fT < rfTLast)
        {
            rfTLast = fT;
        }

        if (rfTFirst > rfTLast)
        {
            // Physically inconsistent times--the objects cannot intersect.
            return true;
        }
    }
    else if (fMax0 < fMin1) // box1 initially on right of box0
    {
        if (fSpeed >= (Real)0.0)
        {
            // The projection intervals are moving apart.
            return true;
        }
        fInvSpeed = ((Real)1.0)/fSpeed;

        fT = (fMax0 - fMin1)*fInvSpeed;
        if (fT > rfTFirst)
        {
            rfTFirst = fT;
        }

        if (rfTFirst > fTMax)
        {
            // Intervals do not intersect during the specified time.
            return true;
        }

        fT = (fMin0 - fMax1)*fInvSpeed;
        if (fT < rfTLast)
        {
            rfTLast = fT;
        }

        if (rfTFirst > rfTLast)
        {
            // Physically inconsistent times--the objects cannot intersect.
            return true;
        }
    }
    else // box0 and box1 initially overlap
    {
        if (fSpeed > (Real)0.0)
        {
            fT = (fMax0 - fMin1)/fSpeed;
            if (fT < rfTLast)
            {
                rfTLast = fT;
            }

            if (rfTFirst > rfTLast)
            {
                // Physically inconsistent times--the objects cannot
                // intersect.
                return true;
            }
        }
        else if (fSpeed < (Real)0.0)
        {
            fT = (fMin0 - fMax1)/fSpeed;
            if (fT < rfTLast)
            {
                rfTLast = fT;
            }

            if (rfTFirst > rfTLast)
            {
                // Physically inconsistent times--the objects cannot
                // intersect.
                return true;
            }
        }
    }

    return false;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template
class IntrBox3Box3<float>;

template
class IntrBox3Box3<double>;
//----------------------------------------------------------------------------
}

