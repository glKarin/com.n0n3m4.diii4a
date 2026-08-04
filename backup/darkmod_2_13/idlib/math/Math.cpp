/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

#include "tests/testing.h"
#include <cmath>

const int SMALLEST_NON_DENORMAL = 1 << IEEE_FLT_MANTISSA_BITS; //anon

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
const float idMath::FLT_EPS		= 1.192092896e-07f;

//anon begin
const float idMath::FLT_SMALLEST_NON_DENORMAL = *reinterpret_cast< const float* >(&SMALLEST_NON_DENORMAL);	// 1.1754944e-038f
//anon end

bool		idMath::initialized = false;
dword		idMath::iSqrt[SQRT_TABLE_SIZE];		// inverse square root lookup table

/*
===============
idMath::Init
===============
*/
void idMath::Init( void ) {
    union _flint fi, fo;

    for ( int i = 0; i < SQRT_TABLE_SIZE; i++ ) {
        fi.i	 = ((EXP_BIAS-1) << EXP_POS) | (i << LOOKUP_POS);
        fo.f	 = (float)( 1.0 / sqrt( fi.f ) );
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
int idMath::FloatToBits( float f, int exponentBits, int mantissaBits ) {
	int i, sign, exponent, mantissa, value;

	assert( exponentBits >= 2 && exponentBits <= 8 );
	assert( mantissaBits >= 2 && mantissaBits <= 23 );

	int maxBits = ( ( ( 1 << ( exponentBits - 1 ) ) - 1 ) << mantissaBits ) | ( ( 1 << mantissaBits ) - 1 );
	int minBits = ( ( ( 1 <<   exponentBits       ) - 2 ) << mantissaBits ) | 1;

	float max = BitsToFloat( maxBits, exponentBits, mantissaBits );
	float min = BitsToFloat( minBits, exponentBits, mantissaBits );

	if ( f >= 0.0f ) {
		if ( f >= max ) {
			return maxBits;
		} else if ( f <= min ) {
			return minBits;
		}
	} else {
		if ( f <= -max ) {
			return ( maxBits | ( 1 << ( exponentBits + mantissaBits ) ) );
		} else if ( f >= -min ) {
			return ( minBits | ( 1 << ( exponentBits + mantissaBits ) ) );
		}
	}

	exponentBits--;
	i = *reinterpret_cast<int *>(&f);
	sign = ( i >> IEEE_FLT_SIGN_BIT ) & 1;
	exponent = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	mantissa = i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 );
	value = sign << ( 1 + exponentBits + mantissaBits );
	value |= ( ( INTSIGNBITSET( exponent ) << exponentBits ) | ( abs( exponent ) & ( ( 1 << exponentBits ) - 1 ) ) ) << mantissaBits;
	value |= mantissa >> ( IEEE_FLT_MANTISSA_BITS - mantissaBits );
	return value;
}

/*
================
idMath::BitsToFloat
================
*/
float idMath::BitsToFloat( int i, int exponentBits, int mantissaBits ) {
	static int exponentSign[2] = { 1, -1 };
	int sign, exponent, mantissa, value;

	assert( exponentBits >= 2 && exponentBits <= 8 );
	assert( mantissaBits >= 2 && mantissaBits <= 23 );

	exponentBits--;
	sign = i >> ( 1 + exponentBits + mantissaBits );
	exponent = ( ( i >> mantissaBits ) & ( ( 1 << exponentBits ) - 1 ) ) * exponentSign[( i >> ( exponentBits + mantissaBits ) ) & 1];
	mantissa = ( i & ( ( 1 << mantissaBits ) - 1 ) ) << ( IEEE_FLT_MANTISSA_BITS - mantissaBits );
	value = sign << IEEE_FLT_SIGN_BIT | ( exponent + IEEE_FLT_EXPONENT_BIAS ) << IEEE_FLT_MANTISSA_BITS | mantissa;
	return *reinterpret_cast<float *>(&value);
}


extern idCVar com_fpexceptions;
static thread_local int IgnoreFpExceptionsCount = 0;
idIgnoreFpExceptions::idIgnoreFpExceptions() {
	if (IgnoreFpExceptionsCount == 0 && com_fpexceptions.GetBool())
		sys->FPU_SetExceptions(false);
	IgnoreFpExceptionsCount++;
}
idIgnoreFpExceptions::~idIgnoreFpExceptions() {
	IgnoreFpExceptionsCount--;
	if (IgnoreFpExceptionsCount == 0 && com_fpexceptions.GetBool())
		sys->FPU_SetExceptions(true);
}

TEST_CASE("Math:Rounding") {
	int errors = 0;

	for (int64 i = 0; i < 1 << 22; i++) {
		for (int s = -1; s <= 1; s += 2) {
			int xi = i * s;
			float xf = xi;
			assert(xi == xf);

			// check rounding of exact integers
			errors += (idMath::Floor(xf) != xi);
			errors += (idMath::Ceil(xf) != xi);
			errors += (idMath::Round(xf) != xi);
			errors += (idMath::FtoiTrunc(xf) != xi);
			errors += (idMath::FtoiRound(xf) != xi);

			// check rounding of fractional numbers
			errors += (idMath::Floor(xf + 0.25f) != xi);
			errors += (idMath::Ceil(xf + 0.25f) != xi + 1);
			errors += (idMath::Round(xf + 0.25f) != xi);
			errors += (idMath::FtoiRound(xf + 0.25f) != xi);

			errors += (idMath::Floor(xf + 0.75f) != xi);
			errors += (idMath::Ceil(xf + 0.75f) != xi + 1);
			errors += (idMath::Round(xf + 0.75f) != xi + 1);
			errors += (idMath::FtoiRound(xf + 0.75f) != xi + 1);

			// check rounding of exact halves
			errors += (idMath::Floor(xf + 0.5f) != xi);
			errors += (idMath::Ceil(xf + 0.5f) != xi + 1);
		}

		float xp = i, xm = -i;

		errors += (idMath::FtoiTrunc(xp + 0.25f) != xp);
		errors += (idMath::FtoiTrunc(xm - 0.25f) != xm);

		errors += (idMath::FtoiTrunc(xp + 0.75f) != xp);
		errors += (idMath::FtoiTrunc(xm - 0.75f) != xm);

		errors += (idMath::FtoiTrunc(xp + 0.5f) != xp);
		errors += (idMath::FtoiTrunc(xm - 0.5f) != xm);

		errors += (idMath::FtoiRound(xp + 0.5f) != (i + (i & 1)));	// FE_TONEAREST: choose even
		errors += (idMath::FtoiRound(xm - 0.5f) != -(i + (i & 1)));	// FE_TONEAREST: choose even
	}

	CHECK(errors == 0);
}

TEST_CASE("Math:RoundToNearestAll"
	* doctest::skip()
) {
	int errors = 0;

	for (uint64 i = 0; i <= UINT32_MAX; i++) {
		union {
			uint32 xinteger;
			float xfloat;
		};
		xinteger = i;
		float x = xfloat;

		if (std::isnan(x) || std::isinf(x))
			continue;
		if (double(x) > INT_MAX || double(x) < -INT_MAX)	// note: results might differ for INT_MIN
			continue;

		if (nearbyint(x) != idMath::FtoiRound(x))
			errors++;
	}

	CHECK(errors == 0);
}
