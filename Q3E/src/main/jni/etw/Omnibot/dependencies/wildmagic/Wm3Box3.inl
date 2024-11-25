// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2005.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

//----------------------------------------------------------------------------
template <class Real>
Box3<Real>::Box3 ()
{
    // uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Box3<Real>::Box3 (const Vector3<Real>& rkCenter, const Vector3<Real>* akAxis,
    const Real* afExtent)
    :
    Center(rkCenter)
{
    for (int i = 0; i < 3; i++)
    {
        Axis[i] = akAxis[i];
        Extent[i] = afExtent[i];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Box3<Real>::Box3 (const Vector3<Real>& rkCenter, const Vector3<Real>& rkAxis0,
    const Vector3<Real>& rkAxis1, const Vector3<Real>& rkAxis2, Real fExtent0,
    Real fExtent1, Real fExtent2)
    :
    Center(rkCenter)
{
    Axis[0] = rkAxis0;
    Axis[1] = rkAxis1;
    Axis[2] = rkAxis2;
    Extent[0] = fExtent0;
    Extent[1] = fExtent1;
    Extent[2] = fExtent2;
}
//----------------------------------------------------------------------------
template <class Real>
void Box3<Real>::ComputeVertices (Vector3<Real> akVertex[8]) const
{
    Vector3<Real> akEAxis[3] =
    {
        Extent[0]*Axis[0],
        Extent[1]*Axis[1],
        Extent[2]*Axis[2]
    };

    akVertex[0] = Center - akEAxis[0] - akEAxis[1] - akEAxis[2];
    akVertex[1] = Center + akEAxis[0] - akEAxis[1] - akEAxis[2];
    akVertex[2] = Center + akEAxis[0] + akEAxis[1] - akEAxis[2];
    akVertex[3] = Center - akEAxis[0] + akEAxis[1] - akEAxis[2];
    akVertex[4] = Center - akEAxis[0] - akEAxis[1] + akEAxis[2];
    akVertex[5] = Center + akEAxis[0] - akEAxis[1] + akEAxis[2];
    akVertex[6] = Center + akEAxis[0] + akEAxis[1] + akEAxis[2];
    akVertex[7] = Center - akEAxis[0] + akEAxis[1] + akEAxis[2];
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real> Box3<Real>::GetCenterBottom() const
{
	return Center - Axis[2] * Extent[2];

}

//----------------------------------------------------------------------------
template <class Real>
void Box3<Real>::Clear()
{
	Center = Vector3<Real>::ZERO;
	Axis[0] = Vector3<Real>::ZERO;
	Axis[1] = Vector3<Real>::ZERO;
	Axis[2] = Vector3<Real>::ZERO;
	Extent[0] = 0.0f;
	Extent[1] = 0.0f;
	Extent[2] = 0.0f;
}

//----------------------------------------------------------------------------
template <class Real>
void Box3<Real>::Identity( float defaultSize )
{
	Center = Vector3<Real>::ZERO;
	Axis[0] = Vector3<Real>( 1.f, 0.f, 0.f );
	Axis[1] = Vector3<Real>( 0.f, 1.f, 0.f );
	Axis[2] = Vector3<Real>( 0.f, 0.f, 1.f );
	Extent[0] = defaultSize;
	Extent[1] = defaultSize;
	Extent[2] = defaultSize;
}

//----------------------------------------------------------------------------
