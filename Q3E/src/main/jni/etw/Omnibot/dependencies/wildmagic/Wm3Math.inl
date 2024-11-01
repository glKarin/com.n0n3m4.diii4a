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
Real Math<Real>::ACos (Real fValue)
{
    if ( -(Real)1.0 < fValue )
    {
        if ( fValue < (Real)1.0 )
            return (Real)acos((double)fValue);
        else
            return (Real)0.0;
    }
    else
    {
        return PI;
    }
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ASin (Real fValue)
{
    if ( -(Real)1.0 < fValue )
    {
        if ( fValue < (Real)1.0 )
            return (Real)asin((double)fValue);
        else
            return HALF_PI;
    }
    else
    {
        return -HALF_PI;
    }
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ATan (Real fValue)
{
    return (Real)atan((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ATan2 (Real fY, Real fX)
{
    return (Real)atan2((double)fY,(double)fX);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Ceil (Real fValue)
{
    return (Real)ceil((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
int Math<Real>::Round (Real fValue)
{
	return (int)floor(fValue + (Real)0.5);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Cos (Real fValue)
{
    return (Real)cos((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Exp (Real fValue)
{
    return (Real)exp((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FAbs (Real fValue)
{
    return (Real)fabs((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Floor (Real fValue)
{
    return (Real)floor((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FMod (Real fX, Real fY)
{
    return (Real)fmod((double)fX,(double)fY);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::InvSqrt (Real fValue)
{
    return (Real)(1.0/sqrt((double)fValue));
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Log (Real fValue)
{
    return (Real)log((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Pow (Real fBase, Real fExponent)
{
    return (Real)pow((double)fBase,(double)fExponent);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sin (Real fValue)
{
    return (Real)sin((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sqr (Real fValue)
{
    return fValue*fValue;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sqrt (Real fValue)
{
    return (Real)sqrt((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Tan (Real fValue)
{
    return (Real)tan((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
void Math<Real>::SinCos(Real a, Real &s, Real &c)
{

#if	defined(_WIN32) && !(defined(_M_X64))
	_asm 
	{
		fld		a
		fsincos
		mov		ecx, c
		mov		edx, s
		fstp	dword ptr [ecx]
		fstp	dword ptr [edx]
	}
#else
	s = Sin( a );
	c = Cos( a );
#endif
}
//----------------------------------------------------------------------------
template <class Real>
int Math<Real>::Sign (int iValue)
{
    if ( iValue > 0 )
        return 1;

    if ( iValue < 0 )
        return -1;

    return 0;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sign (Real fValue)
{
    if ( fValue > (Real)0.0 )
        return (Real)1.0;

    if ( fValue < (Real)0.0 )
        return -(Real)1.0;

    return (Real)0.0;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::UnitRandom (unsigned int uiSeed )
{
    if ( uiSeed > 0 )
        srand(uiSeed);

    double dRatio = ((double)rand())/((double)(RAND_MAX));
    return (Real)dRatio;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::SymmetricRandom (unsigned int uiSeed)
{
    if ( uiSeed > 0.0 )
        srand(uiSeed);

    double dRatio = ((double)rand())/((double)(RAND_MAX));
    return (Real)(2.0*dRatio - 1.0);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::IntervalRandom (Real fMin, Real fMax, unsigned int uiSeed)
{
    if ( uiSeed > 0 )
        srand(uiSeed);

    double dRatio = ((double)rand())/((double)(RAND_MAX));
    return fMin+(fMax-fMin)*((Real)dRatio);
}
//----------------------------------------------------------------------------
template <class Real>
int Math<Real>::IntervalRandomInt (int iMin, int iMax, unsigned int uiSeed)
{
	if ( uiSeed > 0 )
		srand(uiSeed);

	int iDiff = (iMax-iMin);
	if(iDiff==0)
		return iMin;
	int iRand = rand() % iDiff;
	return iMin+iRand;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastSin0 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)7.61e-03;
    fResult *= fASqr;
    fResult -= (Real)1.6605e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastSin1 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = -(Real)2.39e-08;
    fResult *= fASqr;
    fResult += (Real)2.7526e-06;
    fResult *= fASqr;
    fResult -= (Real)1.98409e-04;
    fResult *= fASqr;
    fResult += (Real)8.3333315e-03;
    fResult *= fASqr;
    fResult -= (Real)1.666666664e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastCos0 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)3.705e-02;
    fResult *= fASqr;
    fResult -= (Real)4.967e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastCos1 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = -(Real)2.605e-07;
    fResult *= fASqr;
    fResult += (Real)2.47609e-05;
    fResult *= fASqr;
    fResult -= (Real)1.3888397e-03;
    fResult *= fASqr;
    fResult += (Real)4.16666418e-02;
    fResult *= fASqr;
    fResult -= (Real)4.999999963e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastTan0 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)2.033e-01;
    fResult *= fASqr;
    fResult += (Real)3.1755e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastTan1 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)9.5168091e-03;
    fResult *= fASqr;
    fResult += (Real)2.900525e-03;
    fResult *= fASqr;
    fResult += (Real)2.45650893e-02;
    fResult *= fASqr;
    fResult += (Real)5.33740603e-02;
    fResult *= fASqr;
    fResult += (Real)1.333923995e-01;
    fResult *= fASqr;
    fResult += (Real)3.333314036e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvSin0 (Real fValue)
{
    Real fRoot = Math<Real>::Sqrt(((Real)1.0)-fValue);
    Real fResult = -(Real)0.0187293;
    fResult *= fValue;
    fResult += (Real)0.0742610;
    fResult *= fValue;
    fResult -= (Real)0.2121144;
    fResult *= fValue;
    fResult += (Real)1.5707288;
    fResult = HALF_PI - fRoot*fResult;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvSin1 (Real fValue)
{
    Real fRoot = Math<Real>::Sqrt(FAbs(((Real)1.0)-fValue));
    Real fResult = -(Real)0.0012624911;
    fResult *= fValue;
    fResult += (Real)0.0066700901;
    fResult *= fValue;
    fResult -= (Real)0.0170881256;
    fResult *= fValue;
    fResult += (Real)0.0308918810;
    fResult *= fValue;
    fResult -= (Real)0.0501743046;
    fResult *= fValue;
    fResult += (Real)0.0889789874;
    fResult *= fValue;
    fResult -= (Real)0.2145988016;
    fResult *= fValue;
    fResult += (Real)1.5707963050;
    fResult = HALF_PI - fRoot*fResult;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvCos0 (Real fValue)
{
    Real fRoot = Math<Real>::Sqrt(((Real)1.0)-fValue);
    Real fResult = -(Real)0.0187293;
    fResult *= fValue;
    fResult += (Real)0.0742610;
    fResult *= fValue;
    fResult -= (Real)0.2121144;
    fResult *= fValue;
    fResult += (Real)1.5707288;
    fResult *= fRoot;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvCos1 (Real fValue)
{
    Real fRoot = Math<Real>::Sqrt(FAbs(((Real)1.0)-fValue));
    Real fResult = -(Real)0.0012624911;
    fResult *= fValue;
    fResult += (Real)0.0066700901;
    fResult *= fValue;
    fResult -= (Real)0.0170881256;
    fResult *= fValue;
    fResult += (Real)0.0308918810;
    fResult *= fValue;
    fResult -= (Real)0.0501743046;
    fResult *= fValue;
    fResult += (Real)0.0889789874;
    fResult *= fValue;
    fResult -= (Real)0.2145988016;
    fResult *= fValue;
    fResult += (Real)1.5707963050;
    fResult *= fRoot;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvTan0 (Real fValue)
{
    Real fVSqr = fValue*fValue;
    Real fResult = (Real)0.0208351;
    fResult *= fVSqr;
    fResult -= (Real)0.085133;
    fResult *= fVSqr;
    fResult += (Real)0.180141;
    fResult *= fVSqr;
    fResult -= (Real)0.3302995;
    fResult *= fVSqr;
    fResult += (Real)0.999866;
    fResult *= fValue;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvTan1 (Real fValue)
{
    Real fVSqr = fValue*fValue;
    Real fResult = (Real)0.0028662257;
    fResult *= fVSqr;
    fResult -= (Real)0.0161657367;
    fResult *= fVSqr;
    fResult += (Real)0.0429096138;
    fResult *= fVSqr;
    fResult -= (Real)0.0752896400;
    fResult *= fVSqr;
    fResult += (Real)0.1065626393;
    fResult *= fVSqr;
    fResult -= (Real)0.1420889944;
    fResult *= fVSqr;
    fResult += (Real)0.1999355085;
    fResult *= fVSqr;
    fResult -= (Real)0.3333314528;
    fResult *= fVSqr;
    fResult += (Real)1.0;
    fResult *= fValue;
    return fResult;
}

template <class Real>
Real Math<Real>::Max(Real _a, Real _b) 
{
	return (((_a) > (_b)) ? (_a) : (_b));
}

template <class Real>
Real Math<Real>::Min(Real _a, Real _b) 
{
	return (((_a) < (_b)) ? (_a) : (_b));
}

template <class Real>
Real Math<Real>::DegToRad(Real _a) 
{
	return _a * Math<Real>::PI/(Real)180.0;
}

template <class Real>
Real Math<Real>::RadToDeg(Real _a) 
{
	return _a * (Real)180.0/Math<Real>::PI;
}

template <class Real>
Real Length(Real v1, Real v2)
{
	return  Math<Real>::FAbs(v1-v2);
}

template <class Real>
Real Math<Real>::UnitCircleNormalize( Real fRadians )
{
	/*
	ASSERTF( fRadians < 500.0f  && fRadians > -500.0f, "The Radians we are attempting to normalize are too large... %f\n", fRadians );
	while( fRadians > PblMath::fPi )
	fRadians -= PblMath::fTwoPi;
	while( fRadians < -PblMath::fPi )
	fRadians += PblMath::fTwoPi;
	return fRadians;
	*/
	return fRadians - Math<Real>::TWO_PI * Math<Real>::Floor( ( fRadians + Math<Real>::PI ) * Math<Real>::INV_TWO_PI );
}