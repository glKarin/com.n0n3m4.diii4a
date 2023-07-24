// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop


const float	idMath::PI				= 3.14159265358979323846f;
const float	idMath::TWO_PI			= 2.0f * PI;
const float	idMath::HALF_PI			= 0.5f * PI;
const float	idMath::ONEFOURTH_PI	= 0.25f * PI;
const float	idMath::THREEFOURTHS_PI	= 0.75f * PI;
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

#ifdef ID_WIN_X86_SSE
const float idMath::SSE_FLOAT_ZERO	= 0.0f;
const float idMath::SSE_FLOAT_255	= 255.0f;
#endif

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

	int maxBits = ( 1 << ( exponentBits + mantissaBits ) ) - 1;
	int minBits = 1;

	float max = BitsToFloat( maxBits, exponentBits, mantissaBits );
	float min = BitsToFloat( minBits, exponentBits, mantissaBits );

	if ( f >= 0.0f ) {
		if ( f >= max ) {
			return maxBits;
		} else if ( f == min ) {
			return minBits;
		} else if ( f < min ) {
			return 0;
		}
	} else {
		if ( f <= -max ) {
			return ( maxBits | ( 1 << ( exponentBits + mantissaBits ) ) );
		} else if ( f == -min ) {
			return ( minBits | ( 1 << ( exponentBits + mantissaBits ) ) );
		} else if ( f > -min ) {
			return 0;
		}
	}

	i = *reinterpret_cast<int *>(&f);
	sign = ( i >> IEEE_FLT_SIGN_BIT ) & 1;
	exponent = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	mantissa = i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 );
	value = sign << ( exponentBits + mantissaBits );
	value |= ( exponent + ( ( 1 << ( exponentBits - 1 ) ) - 1 ) ) << mantissaBits;
	value |= mantissa >> ( IEEE_FLT_MANTISSA_BITS - mantissaBits );
	return value;
}

/*
================
idMath::BitsToFloat
================
*/
float idMath::BitsToFloat( int i, int exponentBits, int mantissaBits ) {
	int sign, exponent, mantissa, value;

	assert( exponentBits >= 2 && exponentBits <= 8 );
	assert( mantissaBits >= 2 && mantissaBits <= 23 );

	if ( ( i & ( ( 1 << ( exponentBits + mantissaBits ) ) - 1 ) ) == 0 ) {
		// if exponent & mantissa are zero, value is plus or minus zero
		return 0.0f;
	}

	sign = i >> ( exponentBits + mantissaBits );
	exponent = ( ( i >> mantissaBits ) & ( ( 1 << exponentBits ) - 1 ) ) - ( ( 1 << ( exponentBits - 1 ) ) - 1 );
	mantissa = ( i & ( ( 1 << mantissaBits ) - 1 ) ) << ( IEEE_FLT_MANTISSA_BITS - mantissaBits );
	value = sign << IEEE_FLT_SIGN_BIT | ( exponent + IEEE_FLT_EXPONENT_BIAS ) << IEEE_FLT_MANTISSA_BITS | mantissa;
	return *reinterpret_cast<float *>(&value);
}

/*
================
idMath::TestFloatBitConversions
================
*/
void idMath::TestFloatBitConversions() {
	// 
	// test every value that can be represented within the valid ranges of bits
	char testBuf[ 512 ];
	int numFails = 0;
	for ( int exponentBits = 2; exponentBits <= 8; exponentBits++ ) {
		for ( int mantissaBits = 2; mantissaBits <= 23; mantissaBits++ ) {

			sprintf( testBuf, "Float Testing: exp %i mant %i ...... ", exponentBits, mantissaBits );
#ifdef _WIN32
			OutputDebugString( testBuf );
#else
			printf( "%s", testBuf );
#endif

			int maxBits = ( 1 << ( exponentBits + mantissaBits ) ) - 1;
			int minBits = 1;
			float max = idMath::BitsToFloat( maxBits, exponentBits, mantissaBits );
			float min = idMath::BitsToFloat( minBits, exponentBits, mantissaBits );

			bool failed = false;

			// test that all representable values convert to & from a float & maintain value
			unsigned int totalNumbersRepresentable = 1 << ( exponentBits + mantissaBits + 1 );
			for ( unsigned int i = 0; i < totalNumbersRepresentable; i++ ) {
				// skip anything with zero mantissa & exponent
				float testFloat = idMath::BitsToFloat( *( ( int* )&i ), exponentBits, mantissaBits );
				unsigned int testBits = idMath::FloatToBits( testFloat, exponentBits, mantissaBits );

#pragma warning( push )
#pragma warning( disable: 4389 )
				if ( i == 1 << ( exponentBits + mantissaBits ) ) {
#pragma warning( pop )
					// negative zero should be the same as positive zero (not IEEE-754, but simple)
					if ( testBits != 0 ) {
						failed = true;
						break;
					}
				} else if ( testBits != i ) {
					failed = true;
					break;
				}
			}

			for ( float i = -max * 2.0f; i < max * 2.0f; ) {
				unsigned int testBits = idMath::FloatToBits( i, exponentBits, mantissaBits );
				float testFloat = idMath::BitsToFloat( testBits, exponentBits, mantissaBits );

				// it will have already passed the test of bit->float for this value, if
				// FloatToBits has returned anything in the valid range
				unsigned int minForThisExp = testBits - ( testBits & mantissaBits ) + 1;
				float minError = idMath::Fabs( idMath::BitsToFloat( minForThisExp, exponentBits, mantissaBits ) );

				if ( i < -max ) {
					if ( testFloat != -max ) {
						failed = true;
						break;
					}
				} else if ( i > max ) {
					if ( testFloat != max ) {
						failed = true;
						break;
					}
				} else if ( i > -min && i < min ) {
					if ( testFloat != 0.0f ) {
						failed = true;
						break;
					}
				} else {
					float error = idMath::Fabs( i - testFloat );
					if ( error > minError ) {
						failed = true;
						break;
					}
				}

				i += minError;
			}

			if ( failed ) {
				numFails++;
				sprintf( testBuf, "FAILED\n" );
			} else {
				sprintf( testBuf, "PASSED\n" );
			}
#ifdef _WIN32
			OutputDebugString( testBuf );
#else
			printf( "%s", testBuf );
#endif
		}
	}
	sprintf( testBuf, "%i tests failed\n", numFails );
#ifdef _WIN32
	OutputDebugString( testBuf );
#else
	printf( "%s", testBuf );
#endif
}

// ================================================================================================
// jscott: fast and reliable random routines
// ================================================================================================

unsigned long rvRandom::mSeed;

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
	mSeed *= ( unsigned long )sys->Milliseconds();

	return( mSeed );
}

// ================================================================================================
// Barycentric texture coordinate functions
// Get the *SIGNED* area of a triangle required for barycentric
// ================================================================================================
float idMath::BarycentricTriangleArea( const idVec3 &normal, const idVec3 &a, const idVec3 &b, const idVec3 &c ) 
{
	idVec3	v1, v2;
	idVec3	cross;
	float	area;

	v1 = b - a;
	v2 = c - a;
	cross = v1.Cross( v2 );
	area = 0.5f * ( cross * normal );

	return( area );
}

void idMath::BarycentricEvaluate( idVec2 &result, const idVec3 &point, const idVec3 &normal, const float area, const idVec3 t[3], const idVec2 tc[3] )
{
	float	b1, b2, b3;

	float scale = 1.f;

	scale /= area;

	b1 = idMath::BarycentricTriangleArea( normal, point, t[1], t[2] );
	b2 = idMath::BarycentricTriangleArea( normal, t[0], point, t[2] );
	b3 = idMath::BarycentricTriangleArea( normal, t[0], t[1], point );

	result[0] = ( ( b1 * tc[0][0] ) + ( b2 * tc[1][0] ) + ( b3 * tc[2][0] ) ) * scale;
	result[1] = ( ( b1 * tc[0][1] ) + ( b2 * tc[1][1] ) + ( b3 * tc[2][1] ) ) * scale;
}

void idMath::BarycentricEvaluate( idVec2 &result, const idVec3 &point, const idVec3 &normal, const float area, const idVec3 t[3], const short tc[ 3 ][ 2 ], float scale ) {
	float	b1, b2, b3;

	scale /= area;

	b1 = idMath::BarycentricTriangleArea( normal, point, t[1], t[2] );
	b2 = idMath::BarycentricTriangleArea( normal, t[0], point, t[2] );
	b3 = idMath::BarycentricTriangleArea( normal, t[0], t[1], point );

	result[0] = ( ( b1 * tc[0][0] ) + ( b2 * tc[1][0] ) + ( b3 * tc[2][0] ) ) * scale;
	result[1] = ( ( b1 * tc[0][1] ) + ( b2 * tc[1][1] ) + ( b3 * tc[2][1] ) ) * scale;
}
