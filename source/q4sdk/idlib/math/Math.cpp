
#include "../precompiled.h"
#pragma hdrstop


const float	idMath::PI				= 3.14159265358979323846f;
const float	idMath::TWO_PI			= 2.0f * PI;
const float	idMath::HALF_PI			= 0.5f * PI;
const float	idMath::ONEFOURTH_PI	= 0.25f * PI;
const float idMath::E				= 2.71828182845904523536f;
const float idMath::SQRT_TWO		= 1.41421356237309504880f;
const float idMath::SQRT_THREE		= 1.73205080756887729352f;
// RAVEN BEGIN
const float	idMath::THREEFOURTHS_PI	= 0.75f * PI;
// RAVEN END
const float	idMath::SQRT_1OVER2		= 0.70710678118654752440f;
const float	idMath::SQRT_1OVER3		= 0.57735026918962576450f;
const float	idMath::M_DEG2RAD		= PI / 180.0f;
const float	idMath::M_RAD2DEG		= 180.0f / PI;
const float	idMath::M_SEC2MS		= 1000.0f;
const float	idMath::M_MS2SEC		= 0.001f;
const float	idMath::INFINITY		= 1e30f;
// RAVEN BEGIN
// jscott: renamed to prevent name clash
const float idMath::FLOAT_EPSILON	= 1.192092896e-07f;
// ddynerman: added, from limits.h
const int idMath::INT_MIN			= (-2147483647 - 1);
const int idMath::INT_MAX			= 2147483647;
// RAVEN END

bool		idMath::initialized		= false;
#ifdef _FAST_MATH
dword		idMath::iSqrt[SQRT_TABLE_SIZE];		// inverse square root lookup table
#endif

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
#ifdef _FAST_MATH
    union _flint fi, fo;

    for ( int i = 0; i < SQRT_TABLE_SIZE; i++ ) {
        fi.i	 = ((EXP_BIAS-1) << EXP_POS) | (i << LOOKUP_POS);
        fo.f	 = (float)( 1.0 / sqrt( fi.f ) );
        iSqrt[i] = ((dword)(((fo.i + (1<<(SEED_POS-2))) >> SEED_POS) & 0xFF))<<SEED_POS;
    }
    
	iSqrt[SQRT_TABLE_SIZE / 2] = ((dword)(0xFF))<<(SEED_POS); 
#endif

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

// RAVEN BEGIN
// bdube: added block

void idMath::ArtesianFromPolar( idVec3 &result, idVec3 view )
{
	float	s1, c1, s2, c2;

	idMath::SinCos( view[1], s1, c1 );
	idMath::SinCos( view[2], s2, c2 );

	result[0] = c1 * s2 * view[0];
	result[1] = s1 * s2 * view[0];
	result[2] = c2 * view[0];
}

void idMath::PolarFromArtesian( idVec3 &view, idVec3 artesian )
{
	float	length;
	view[0] = artesian.Length();
	view[1] = idMath::ATan( artesian[1], artesian[0] );
	length = sqrtf( ( artesian[0] * artesian[0] ) + ( artesian[1] * artesian[1] ) );
	view[2] = idMath::ATan( length, artesian[2] );
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
	area = 0.5f * DotProduct( cross, normal );

	return( area );
}

void idMath::BarycentricEvaluate( idVec2 &result, const idVec3 &point, const idVec3 &normal, const float area, const idVec3 t[3], const idVec2 tc[3] )
{
	float	b1, b2, b3;

	b1 = idMath::BarycentricTriangleArea( normal, point, t[1], t[2] ) / area;
	b2 = idMath::BarycentricTriangleArea( normal, t[0], point, t[2] ) / area;
	b3 = idMath::BarycentricTriangleArea( normal, t[0], t[1], point ) / area;

	result[0] = ( b1 * tc[0][0] ) + ( b2 * tc[1][0] ) + ( b3 * tc[2][0] );
	result[1] = ( b1 * tc[0][1] ) + ( b2 * tc[1][1] ) + ( b3 * tc[2][1] );
}

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

// RAVEN END
