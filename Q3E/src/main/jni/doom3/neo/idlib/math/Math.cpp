/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../precompiled.h"
#pragma hdrstop


const float	idMath::PI				= 3.14159265358979323846f;
const float	idMath::TWO_PI			= 2.0f * PI;
const float	idMath::HALF_PI			= 0.5f * PI;
const float	idMath::ONEFOURTH_PI	= 0.25f * PI;
const float idMath::E				= 2.71828182845904523536f;
const float idMath::SQRT_TWO		= 1.41421356237309504880f;
const float idMath::SQRT_THREE		= 1.73205080756887729352f;
const float	idMath::SQRT_1OVER2		= 0.70710678118654752440f;
const float	idMath::SQRT_1OVER3		= 0.57735026918962576450f;
const float	idMath::M_DEG2RAD		= PI / 180.0f;
const float	idMath::M_RAD2DEG		= 180.0f / PI;
const float	idMath::M_SEC2MS		= 1000.0f;
const float	idMath::M_MS2SEC		= 0.001f;
const float	idMath::INFINITY		= 1e30f;
const float idMath::FLT_EPSILON		= 1.192092896e-07f;

bool		idMath::initialized		= false;
dword		idMath::iSqrt[SQRT_TABLE_SIZE];		// inverse square root lookup table

/*
===============
idMath::Init
===============
*/
void idMath::Init(void)
{
	union _flint fi, fo;

	for (int i = 0; i < SQRT_TABLE_SIZE; i++) {
		fi.i	 = ((EXP_BIAS-1) << EXP_POS) | (i << LOOKUP_POS);
		fo.f	 = (float)(1.0 / sqrt(fi.f));
		iSqrt[i] = ((dword)(((fo.i + (1<<(SEED_POS-2))) >> SEED_POS) & 0xFF))<<SEED_POS;
	}

	iSqrt[SQRT_TABLE_SIZE / 2] = ((dword)(0xFF))<<(SEED_POS);

	initialized = true;
}

/*
================
idMath::FloatToBits
================
*/
int idMath::FloatToBits(float f, int exponentBits, int mantissaBits)
{
	int i, sign, exponent, mantissa, value;

	assert(exponentBits >= 2 && exponentBits <= 8);
	assert(mantissaBits >= 2 && mantissaBits <= 23);

	int maxBits = (((1 << (exponentBits - 1)) - 1) << mantissaBits) | ((1 << mantissaBits) - 1);
	int minBits = (((1 <<   exponentBits) - 2) << mantissaBits) | 1;

	float max = BitsToFloat(maxBits, exponentBits, mantissaBits);
	float min = BitsToFloat(minBits, exponentBits, mantissaBits);

	if (f >= 0.0f) {
		if (f >= max) {
			return maxBits;
		} else if (f <= min) {
			return minBits;
		}
	} else {
		if (f <= -max) {
			return (maxBits | (1 << (exponentBits + mantissaBits)));
		} else if (f >= -min) {
			return (minBits | (1 << (exponentBits + mantissaBits)));
		}
	}

	exponentBits--;
	i = *reinterpret_cast<int *>(&f);
	sign = (i >> IEEE_FLT_SIGN_BIT) & 1;
	exponent = ((i >> IEEE_FLT_MANTISSA_BITS) & ((1 << IEEE_FLT_EXPONENT_BITS) - 1)) - IEEE_FLT_EXPONENT_BIAS;
	mantissa = i & ((1 << IEEE_FLT_MANTISSA_BITS) - 1);
	value = sign << (1 + exponentBits + mantissaBits);
	value |= ((INTSIGNBITSET(exponent) << exponentBits) | (abs(exponent) & ((1 << exponentBits) - 1))) << mantissaBits;
	value |= mantissa >> (IEEE_FLT_MANTISSA_BITS - mantissaBits);
	return value;
}

/*
================
idMath::BitsToFloat
================
*/
float idMath::BitsToFloat(int i, int exponentBits, int mantissaBits)
{
	static int exponentSign[2] = { 1, -1 };
	int sign, exponent, mantissa, value;

	assert(exponentBits >= 2 && exponentBits <= 8);
	assert(mantissaBits >= 2 && mantissaBits <= 23);

	exponentBits--;
	sign = i >> (1 + exponentBits + mantissaBits);
	exponent = ((i >> mantissaBits) & ((1 << exponentBits) - 1)) * exponentSign[(i >> (exponentBits + mantissaBits)) & 1];
	mantissa = (i & ((1 << mantissaBits) - 1)) << (IEEE_FLT_MANTISSA_BITS - mantissaBits);
	value = sign << IEEE_FLT_SIGN_BIT | (exponent + IEEE_FLT_EXPONENT_BIAS) << IEEE_FLT_MANTISSA_BITS | mantissa;
	return *reinterpret_cast<float *>(&value);
}

#ifdef _RAVEN
// RAVEN BEGIN
// jscott: renamed to prevent name clash
const float idMath::FLOAT_EPSILON	= 1.192092896e-07f;
// RAVEN END
// RAVEN BEGIN
const float	idMath::THREEFOURTHS_PI	= 0.75f * PI;
// RAVEN END

// abahr:
float idMath::Lerp( const idVec2& range, float frac ) {
	return Lerp( range[0], range[1], frac );
}

// abahr:
float idMath::Lerp( float start, float end, float frac ) {
	if( frac >= 1.0f ) {
		return end;
	}

	if( frac <= 0.0f ) {
		return start;
	}

	return start + (end - start) * frac;
}

// abahr:
float idMath::MidPointLerp( float start, float mid, float end, float frac ) {
	if( frac < 0.5f ) {
		return Lerp( start, mid, 2.0f * frac );
	}

	return Lerp( mid, end, 2.0f * (frac - 0.5f) );
}

float idMath::dBToScale( float db ) {

	if( db < -60.0f ) {

		return( 0.0f );

	} else {

		return( powf( 2.0f, db * ( 1.0f / 6.0f ) ) );
	}
}

float idMath::ScaleToDb( float scale ) {

	if( scale <= 0.0f ) {

		return( -60.0f );

	} else {

		return( 6.0f * idMath::Log( scale ) / idMath::Log( 2 ) );
	}
}



// ================================================================================================
// jscott: fast and reliable random routines
// ================================================================================================

unsigned/* 64long */int rvRandom::mSeed;

float rvRandom::flrand( float min, float max )
{
	float	result;

	mSeed = ( mSeed * 214013L ) + 2531011;
	// Note: the shift and divide cannot be combined as this breaks the routine
	result = ( float )( mSeed >> 17 );						// 0 - 32767 range
	result = ( ( result * ( max - min ) ) * ( 1.0f / 32768.0f ) ) + min;
	return( result );
}

float rvRandom::flrand() {
	return flrand( 0.0f, 1.0f );
}

float rvRandom::flrand( const idVec2& v ) {
	return flrand( v[0], v[1] );
}

int rvRandom::irand( int min, int max )
{
	int		result;

	max++;
	mSeed = ( mSeed * 214013L ) + 2531011;
	result = mSeed >> 17;
	result = ( ( result * ( max - min ) ) >> 15 ) + min;
	return( result );
}

// Try to get a seed independent of the random number system

int rvRandom::Init( void )
{
	mSeed *= ( unsigned/* 64long */int )sys->Milliseconds();

	return( mSeed );
}
#endif

#ifdef _RAVEN
/*
===================
idMath::Distance
===================
*/
float idMath::Distance(const idVec3 &p1, const idVec3 &p2)
{
	idVec3 v = p2 - p1;
	return v.Length();
}

/*
========================
idMath::CreateVector
========================
*/
idVec3 idMath::CreateVector(float x, float y, float z)
{
	return idVec3(x, y, z);
}
#endif
