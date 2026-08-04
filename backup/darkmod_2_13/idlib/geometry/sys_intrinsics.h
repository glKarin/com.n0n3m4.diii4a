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
#ifndef __SYS_INTRIINSICS_H__
#define __SYS_INTRIINSICS_H__

#include <emmintrin.h>

/*
================================================================================================

	Scalar single precision floating-point intrinsics

================================================================================================
*/

ID_INLINE_EXTERN float __fmuls( float a, float b )
{
	return ( a * b );
}
ID_INLINE_EXTERN float __fmadds( float a, float b, float c )
{
	return ( a * b + c );
}
ID_INLINE_EXTERN float __fnmsubs( float a, float b, float c )
{
	return ( c - a * b );
}
ID_INLINE_EXTERN float __fsels( float a, float b, float c )
{
	return ( a >= 0.0f ) ? b : c;
}
ID_INLINE_EXTERN float __frcps( float x )
{
	return ( 1.0f / x );
}
ID_INLINE_EXTERN float __fdivs( float x, float y )
{
	return ( x / y );
}
ID_INLINE_EXTERN float __frsqrts( float x )
{
	return ( 1.0f / sqrtf( x ) );
}
ID_INLINE_EXTERN float __frcps16( float x )
{
	return ( 1.0f / x );
}
ID_INLINE_EXTERN float __fdivs16( float x, float y )
{
	return ( x / y );
}
ID_INLINE_EXTERN float __frsqrts16( float x )
{
	return ( 1.0f / sqrtf( x ) );
}
ID_INLINE_EXTERN float __frndz( float x )
{
	return ( float )( ( int )( x ) );
}

/*
================================================================================================

	Vector Intrinsics

================================================================================================
*/

/*
================================================
	PC Windows
================================================
*/

#if !defined( R_SHUFFLE_D )
#define R_SHUFFLE_D( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))
#endif

// DG: _CRT_ALIGN seems to be MSVC specific, so provide implementation..
#ifndef _CRT_ALIGN
#if defined(__GNUC__) // also applies for clang
#define _CRT_ALIGN(x) __attribute__ ((__aligned__ (x)))
#elif defined(_MSC_VER) // also for MSVC, just to be sure
#define _CRT_ALIGN(x) __declspec(align(x))
#endif
#endif
// DG: make sure __declspec(intrin_type) is only used on MSVC (it's not available on GCC etc
#ifdef _MSC_VER
#define DECLSPEC_INTRINTYPE __declspec( intrin_type )
#else
#define DECLSPEC_INTRINTYPE
#endif
// DG end

// make the intrinsics "type unsafe"
typedef union DECLSPEC_INTRINTYPE _CRT_ALIGN( 16 ) __m128c
{
	ID_FORCE_INLINE __m128c() {}
	ID_FORCE_INLINE __m128c( __m128 f )
	{
		m128 = f;
	}
	ID_FORCE_INLINE __m128c( __m128i i )
	{
		m128i = i;
	}
	ID_FORCE_INLINE operator	__m128()
	{
		return m128;
	}
	ID_FORCE_INLINE operator	__m128i()
	{
		return m128i;
	}
	__m128		m128;
	__m128i		m128i;
} __m128c;

#define _mm_madd_ps( a, b, c )				_mm_add_ps( _mm_mul_ps( (a), (b) ), (c) )
#define _mm_nmsub_ps( a, b, c )				_mm_sub_ps( (c), _mm_mul_ps( (a), (b) ) )
#define _mm_splat_ps( x, i )				__m128c( _mm_shuffle_epi32( __m128c( x ), _MM_SHUFFLE( i, i, i, i ) ) )
#define _mm_perm_ps( x, perm )				__m128c( _mm_shuffle_epi32( __m128c( x ), perm ) )
#define _mm_sel_ps( a, b, c )  				_mm_or_ps( _mm_andnot_ps( __m128c( c ), a ), _mm_and_ps( __m128c( c ), b ) )
#define _mm_sel_si128( a, b, c )			_mm_or_si128( _mm_andnot_si128( __m128c( c ), a ), _mm_and_si128( __m128c( c ), b ) )
#define _mm_sld_ps( x, y, imm )				__m128c( _mm_or_si128( _mm_srli_si128( __m128c( x ), imm ), _mm_slli_si128( __m128c( y ), 16 - imm ) ) )
#define _mm_sld_si128( x, y, imm )			_mm_or_si128( _mm_srli_si128( x, imm ), _mm_slli_si128( y, 16 - imm ) )

ID_FORCE_INLINE_EXTERN __m128 _mm_msum3_ps( __m128 a, __m128 b )
{
	__m128 c = _mm_mul_ps( a, b );
	return _mm_add_ps( _mm_splat_ps( c, 0 ), _mm_add_ps( _mm_splat_ps( c, 1 ), _mm_splat_ps( c, 2 ) ) );
}

ID_FORCE_INLINE_EXTERN __m128 _mm_msum4_ps( __m128 a, __m128 b )
{
	__m128 c = _mm_mul_ps( a, b );
	c = _mm_add_ps( c, _mm_perm_ps( c, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	c = _mm_add_ps( c, _mm_perm_ps( c, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	return c;
}

#define _mm_shufmix_epi32( x, y, perm )		__m128c( _mm_shuffle_ps( __m128c( x ), __m128c( y ), perm ) )
#define _mm_loadh_epi64( x, address )		__m128c( _mm_loadh_pi( __m128c( x ), (__m64 *)address ) )
#define _mm_storeh_epi64( address, x )		_mm_storeh_pi( (__m64 *)address, __m128c( x ) )

// floating-point reciprocal with close to full precision
ID_FORCE_INLINE_EXTERN __m128 _mm_rcp32_ps( __m128 x )
{
	__m128 r = _mm_rcp_ps( x );		// _mm_rcp_ps() has 12 bits of precision
	r = _mm_sub_ps( _mm_add_ps( r, r ), _mm_mul_ps( _mm_mul_ps( x, r ), r ) );
	r = _mm_sub_ps( _mm_add_ps( r, r ), _mm_mul_ps( _mm_mul_ps( x, r ), r ) );
	return r;
}
// floating-point reciprocal with at least 16 bits precision
ID_FORCE_INLINE_EXTERN __m128 _mm_rcp16_ps( __m128 x )
{
	__m128 r = _mm_rcp_ps( x );		// _mm_rcp_ps() has 12 bits of precision
	r = _mm_sub_ps( _mm_add_ps( r, r ), _mm_mul_ps( _mm_mul_ps( x, r ), r ) );
	return r;
}
// floating-point divide with close to full precision
ID_FORCE_INLINE_EXTERN __m128 _mm_div32_ps( __m128 x, __m128 y )
{
	return _mm_mul_ps( x, _mm_rcp32_ps( y ) );
}
// floating-point divide with at least 16 bits precision
ID_FORCE_INLINE_EXTERN __m128 _mm_div16_ps( __m128 x, __m128 y )
{
	return _mm_mul_ps( x, _mm_rcp16_ps( y ) );
}
// load idBounds::GetMins()
#define _mm_loadu_bounds_0( bounds )		_mm_perm_ps( _mm_loadh_pi( _mm_load_ss( & bounds[0].x ), (__m64 *) & bounds[0].y ), _MM_SHUFFLE( 1, 3, 2, 0 ) )
// load idBounds::GetMaxs()
#define _mm_loadu_bounds_1( bounds )		_mm_perm_ps( _mm_loadh_pi( _mm_load_ss( & bounds[1].x ), (__m64 *) & bounds[1].y ), _MM_SHUFFLE( 1, 3, 2, 0 ) )

#endif	// !__SYS_INTRIINSICS_H__
