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

#include "Simd_Generic.h"
#include "Simd_MMX.h"
#include "Simd_SSE.h"
#include "Simd_SSE2.h"


//===============================================================
//
//	SSE2 implementation of idSIMDProcessor
//
//===============================================================
#if defined(__GNUC__) && defined(__SSE2__) || ( ( defined(_M_X64) || defined(__x86_64__) ) && defined(_USE_SSE) ) || ( ( defined(__arm__) || defined(__aarch64__) ) && defined(_ARM_SIMD_SSE2NEON) )

#if ( defined(__arm__) || defined(__aarch64__) ) && defined(_ARM_SIMD_SSE2NEON)
#include "sse2neon/sse2neon.h"
#else
#include <emmintrin.h>
#endif

#if defined(_MSC_VER)
//#define __m128_to_m128i(x) _mm_cvtps_epi32(x)
//#define __m128i_to_m128(x) _mm_cvtepi32_ps(x)
#define __m128_to_m128i(x) _mm_castps_si128(x)
#define __m128i_to_m128(x) _mm_castsi128_ps(x)
#else
#define __m128_to_m128i(x) (__m128i) (x)
#define __m128i_to_m128(x) (__m128) (x)
#endif

#define SHUFFLEPS( x, y, z, w )		(( (x) & 3 ) << 6 | ( (y) & 3 ) << 4 | ( (z) & 3 ) << 2 | ( (w) & 3 ))
#define R_SHUFFLEPS( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))

/*
============
idSIMD_SSE2::GetName
============
*/
const char *idSIMD_SSE2::GetName(void) const
{
	return "MMX & SSE & SSE2";
}

/*
============
idSIMD_SSE::CmpLT

  dst[i] |= ( src0[i] < constant ) << bitNum;
============
*/
void VPCALL idSIMD_SSE2::CmpLT(byte *dst, const byte bitNum, const float *src0, const float constant, const int count)
{
	int i, cnt, pre, post;
	float *aligned;
	__m128 xmm0, xmm1;
	__m128i xmm0i;
	int cnt_l;
	char *src0_p;
	char *constant_p;
	char *dst_p;
	int mask_l;
	int dst_l;

	/* if the float array is not aligned on a 4 byte boundary */
    if ( ptrdiff_t(src0) & 3 ) {
		/* unaligned memory access */
		pre = 0;
		cnt = count >> 2;
		post = count - (cnt<<2);

		/*
			__asm	mov			edx, cnt
			__asm	test		edx, edx
			__asm	je			doneCmp
		*/
		cnt_l = cnt;

		if (cnt_l != 0) {
			/*
				__asm	push		ebx
				__asm	neg			edx
				__asm	mov			esi, src0
				__asm	prefetchnta	[esi+64]
				__asm	movss		xmm1, constant
				__asm	shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
				__asm	mov			edi, dst
				__asm	mov			cl, bitNum
			*/
			cnt_l = -cnt_l;
			src0_p = (char *) src0;
			_mm_prefetch(src0_p+64, _MM_HINT_NTA);
			constant_p = (char *) &constant;
			xmm1 = _mm_load_ss((float *)constant_p);
			xmm1 = _mm_shuffle_ps(xmm1, xmm1, R_SHUFFLEPS(0, 0, 0, 0));
			dst_p = (char *)dst;

			/*
					__asm loopNA:
			*/
			do {
				/*
					__asm	movups		xmm0, [esi]
					__asm	prefetchnta	[esi+128]
					__asm	cmpltps		xmm0, xmm1
					__asm	movmskps	eax, xmm0																												\
					__asm	mov			ah, al
					__asm	shr			ah, 1
					__asm	mov			bx, ax
					__asm	shl			ebx, 14
					__asm	mov			bx, ax
					__asm	and			ebx, 0x01010101
					__asm	shl			ebx, cl
					__asm	or			ebx, dword ptr [edi]
					__asm	mov			dword ptr [edi], ebx
					__asm	add			esi, 16
					__asm	add			edi, 4
					__asm	inc			edx
					__asm	jl			loopNA
					__asm	pop			ebx
				*/
				xmm0 = _mm_loadu_ps((float *) src0_p);
				_mm_prefetch(src0_p+128, _MM_HINT_NTA);
				xmm0 = _mm_cmplt_ps(xmm0, xmm1);
				// Simplify using SSE2
				// xmm0i = (__m128i) xmm0;
                xmm0i = __m128_to_m128i(xmm0);
				xmm0i = _mm_packs_epi32(xmm0i, xmm0i);
				xmm0i = _mm_packs_epi16(xmm0i, xmm0i);
				mask_l = _mm_cvtsi128_si32(xmm0i);
				// End
				mask_l = mask_l &  0x01010101;
				mask_l = mask_l << bitNum;
				dst_l  = *((int *) dst_p);
				mask_l = mask_l | dst_l;
				*((int *) dst_p) = mask_l;
				src0_p = src0_p + 16;
				dst_p = dst_p + 4;
				cnt_l = cnt_l + 1;
			} while (cnt_l < 0);
		}
	} else {
		/* aligned memory access */
		aligned = (float *)((((ptrdiff_t) src0) + 15) & ~15);

		if ((ptrdiff_t)aligned > ((ptrdiff_t)src0) + count) {
			pre = count;
			post = 0;
		} else {
			pre = aligned - src0;
			cnt = (count - pre) >> 2;
			post = count - pre - (cnt<<2);
			/*
					__asm	mov			edx, cnt
					__asm	test		edx, edx
					__asm	je			doneCmp
			*/
			cnt_l = cnt;

			if (cnt_l != 0) {
				/*
						__asm	push		ebx
						__asm	neg			edx
						__asm	mov			esi, aligned
						__asm	prefetchnta	[esi+64]
						__asm	movss		xmm1, constant
						__asm	shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
						__asm	mov			edi, dst
						__asm	add			edi, pre
						__asm	mov			cl, bitNum
				*/
				cnt_l = -cnt_l;
				src0_p = (char *) src0;
				_mm_prefetch(src0_p+64, _MM_HINT_NTA);
				constant_p = (char *) &constant;
				xmm1 = _mm_load_ss((float *)constant_p);
				xmm1 = _mm_shuffle_ps(xmm1, xmm1, R_SHUFFLEPS(0, 0, 0, 0));
				dst_p = (char *)dst;
				dst_p = dst_p + pre;

				/*
						__asm loopA:
				*/
				do {
					/*
							__asm	movaps		xmm0, [esi]
							__asm	prefetchnta	[esi+128]
							__asm	cmpltps		xmm0, xmm1
							__asm	movmskps	eax, xmm0																											\
							__asm	mov			ah, al
							__asm	shr			ah, 1
							__asm	mov			bx, ax
							__asm	shl			ebx, 14
							__asm	mov			bx, ax
							__asm	and			ebx, 0x01010101
							__asm	shl			ebx, cl
							__asm	or			ebx, dword ptr [edi]
							__asm	mov			dword ptr [edi], ebx
							__asm	add			esi, 16
							__asm	add			edi, 4
							__asm	inc			edx
							__asm	jl			loopA
							__asm	pop			ebx
					*/
					xmm0 = _mm_load_ps((float *) src0_p);
					_mm_prefetch(src0_p+128, _MM_HINT_NTA);
					xmm0 = _mm_cmplt_ps(xmm0, xmm1);
					// Simplify using SSE2
					// xmm0i = (__m128i) xmm0;
                    xmm0i = __m128_to_m128i(xmm0);
					xmm0i = _mm_packs_epi32(xmm0i, xmm0i);
					xmm0i = _mm_packs_epi16(xmm0i, xmm0i);
					mask_l = _mm_cvtsi128_si32(xmm0i);
					// End
					mask_l = mask_l &  0x01010101;
					mask_l = mask_l << bitNum;
					dst_l  = *((int *) dst_p);
					mask_l = mask_l | dst_l;
					*((int *) dst_p) = mask_l;
					src0_p = src0_p + 16;
					dst_p = dst_p + 4;
					cnt_l = cnt_l + 1;
				} while (cnt_l < 0);
			}
		}
	}

	/*
	doneCmp:
	*/
	float c = constant;

	for (i = 0; i < pre; i++) {
		dst[i] |= (src0[i] < c) << bitNum;
	}

	for (i = count - post; i < count; i++) {
		dst[i] |= (src0[i] < c) << bitNum;
	}
}

// DOOM3-BFG
#ifndef M_PI // DG: this is already defined in math.h
#define M_PI	3.14159265358979323846f
#endif

#define JOINTQUAT_SIZE				(7*4)
#define JOINTMAT_SIZE				(4*3*4)

template< typename _type_ >
ID_INLINE void SwapValues( _type_ & a, _type_ & b )
{
    _type_ c = a;
    a = b;
    b = c;
}

/*
================================================================================================

	Scalar single precision floating-point intrinsics

================================================================================================
*/

ID_INLINE float __fmuls( float a, float b )
{
    return ( a * b );
}
ID_INLINE float __fmadds( float a, float b, float c )
{
    return ( a * b + c );
}
ID_INLINE float __fnmsubs( float a, float b, float c )
{
    return ( c - a * b );
}
ID_INLINE float __fsels( float a, float b, float c )
{
    return ( a >= 0.0f ) ? b : c;
}
ID_INLINE float __frcps( float x )
{
    return ( 1.0f / x );
}
ID_INLINE float __fdivs( float x, float y )
{
    return ( x / y );
}
ID_INLINE float __frsqrts( float x )
{
    return ( 1.0f / sqrtf( x ) );
}
ID_INLINE float __frcps16( float x )
{
    return ( 1.0f / x );
}
ID_INLINE float __fdivs16( float x, float y )
{
    return ( x / y );
}
ID_INLINE float __frsqrts16( float x )
{
    return ( 1.0f / sqrtf( x ) );
}
ID_INLINE float __frndz( float x )
{
    return ( float )( ( int )( x ) );
}

/*
================================================================================================

	Zero cache line and prefetch intrinsics

================================================================================================
*/

// The code below assumes that a cache line is 64 bytes.
// We specify the cache line size as 128 here to make the code consistent with the consoles.
#define CACHE_LINE_SIZE						128

ID_INLINE void Prefetch( const void* ptr, int offset )
{
//	const char * bytePtr = ( (const char *) ptr ) + offset;
//	_mm_prefetch( bytePtr +  0, _MM_HINT_NTA );
//	_mm_prefetch( bytePtr + 64, _MM_HINT_NTA );
}
ID_INLINE void ZeroCacheLine( void* ptr, int offset )
{
    //assert_128_byte_aligned( ptr );
    char* bytePtr = ( ( char* ) ptr ) + offset;
    __m128i zero = _mm_setzero_si128();
    _mm_store_si128( ( __m128i* )( bytePtr + 0 * 16 ), zero );
    _mm_store_si128( ( __m128i* )( bytePtr + 1 * 16 ), zero );
    _mm_store_si128( ( __m128i* )( bytePtr + 2 * 16 ), zero );
    _mm_store_si128( ( __m128i* )( bytePtr + 3 * 16 ), zero );
    _mm_store_si128( ( __m128i* )( bytePtr + 4 * 16 ), zero );
    _mm_store_si128( ( __m128i* )( bytePtr + 5 * 16 ), zero );
    _mm_store_si128( ( __m128i* )( bytePtr + 6 * 16 ), zero );
    _mm_store_si128( ( __m128i* )( bytePtr + 7 * 16 ), zero );
}
ID_INLINE void FlushCacheLine( const void* ptr, int offset )
{
    const char* bytePtr = ( ( const char* ) ptr ) + offset;
    _mm_clflush( bytePtr +  0 );
    _mm_clflush( bytePtr + 64 );
}

/*
================================================
#endif
	Other
================================================
*/

/*
================================================
	Block Clear Macros
================================================
*/

// number of additional elements that are potentially cleared when clearing whole cache lines at a time
ID_INLINE int CACHE_LINE_CLEAR_OVERFLOW_COUNT( int size )
{
    if( ( size & ( CACHE_LINE_SIZE - 1 ) ) == 0 )
    {
        return 0;
    }
    if( size > CACHE_LINE_SIZE )
    {
        return 1;
    }
    return ( CACHE_LINE_SIZE / ( size & ( CACHE_LINE_SIZE - 1 ) ) );
}

// if the pointer is not on a cache line boundary this assumes the cache line the pointer starts in was already cleared
// RB: changed UINT_PTR to uintptr_t
#define CACHE_LINE_CLEAR_BLOCK( ptr, size )																		\
	byte * startPtr = (byte *)( ( ( (uintptr_t) ( ptr ) ) + CACHE_LINE_SIZE - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );	\
	byte * endPtr = (byte *)( ( (uintptr_t) ( ptr ) + ( size ) - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );				\
	for ( ; startPtr <= endPtr; startPtr += CACHE_LINE_SIZE ) {													\
		ZeroCacheLine( startPtr, 0 );																			\
	}

#define CACHE_LINE_CLEAR_BLOCK_AND_FLUSH( ptr, size )															\
	byte * startPtr = (byte *)( ( ( (uintptr_t) ( ptr ) ) + CACHE_LINE_SIZE - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );	\
	byte * endPtr = (byte *)( ( (uintptr_t) ( ptr ) + ( size ) - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );				\
	for ( ; startPtr <= endPtr; startPtr += CACHE_LINE_SIZE ) {													\
		ZeroCacheLine( startPtr, 0 );																			\
		FlushCacheLine( startPtr, 0 );																			\
	}
// RB end

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
    __m128c() {}
    __m128c( __m128 f )
    {
        m128 = f;
    }
    __m128c( __m128i i )
    {
        m128i = i;
    }
    operator	__m128()
    {
        return m128;
    }
    operator	__m128i()
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

ID_INLINE __m128 _mm_msum3_ps( __m128 a, __m128 b )
{
    __m128 c = _mm_mul_ps( a, b );
    return _mm_add_ps( _mm_splat_ps( c, 0 ), _mm_add_ps( _mm_splat_ps( c, 1 ), _mm_splat_ps( c, 2 ) ) );
}

ID_INLINE __m128 _mm_msum4_ps( __m128 a, __m128 b )
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
ID_INLINE __m128 _mm_rcp32_ps( __m128 x )
{
    __m128 r = _mm_rcp_ps( x );		// _mm_rcp_ps() has 12 bits of precision
    r = _mm_sub_ps( _mm_add_ps( r, r ), _mm_mul_ps( _mm_mul_ps( x, r ), r ) );
    r = _mm_sub_ps( _mm_add_ps( r, r ), _mm_mul_ps( _mm_mul_ps( x, r ), r ) );
    return r;
}
// floating-point reciprocal with at least 16 bits precision
ID_INLINE __m128 _mm_rcp16_ps( __m128 x )
{
    __m128 r = _mm_rcp_ps( x );		// _mm_rcp_ps() has 12 bits of precision
    r = _mm_sub_ps( _mm_add_ps( r, r ), _mm_mul_ps( _mm_mul_ps( x, r ), r ) );
    return r;
}
// floating-point divide with close to full precision
ID_INLINE __m128 _mm_div32_ps( __m128 x, __m128 y )
{
    return _mm_mul_ps( x, _mm_rcp32_ps( y ) );
}
// floating-point divide with at least 16 bits precision
ID_INLINE __m128 _mm_div16_ps( __m128 x, __m128 y )
{
    return _mm_mul_ps( x, _mm_rcp16_ps( y ) );
}
// load idBounds::GetMins()
#define _mm_loadu_bounds_0( bounds )		_mm_perm_ps( _mm_loadh_pi( _mm_load_ss( & bounds[0].x ), (__m64 *) & bounds[0].y ), _MM_SHUFFLE( 1, 3, 2, 0 ) )
// load idBounds::GetMaxs()
#define _mm_loadu_bounds_1( bounds )		_mm_perm_ps( _mm_loadh_pi( _mm_load_ss( & bounds[1].x ), (__m64 *) & bounds[1].y ), _MM_SHUFFLE( 1, 3, 2, 0 ) )

#define JOINTQUAT_STRIDE 7 //karin: DOOM3BFG is 8(align 32), it will crash if memory not in stack by _alloca
#if JOINTQUAT_SIZE == (8 * 4)
#define JOINTQUAT_T_32 1
#else
#define JOINTQUAT_T_32 0

ID_INLINE __m128 _mm_load_ps_3( const float *ptr )
{
    _CRT_ALIGN(16) float tmp[] = { ptr[0], ptr[1], ptr[2], 0.0f };
    return _mm_load_ps( tmp );
}

ID_INLINE void _mm_store_ps_3( float *ptr, __m128 a)
{
    _CRT_ALIGN(16) float tmp[4];
    _mm_store_ps( tmp, a );
    ptr[0] = tmp[0];
    ptr[1] = tmp[1];
    ptr[2] = tmp[2];
}
#endif

/*
============
idSIMD_SSE::BlendJoints
============
*/
void VPCALL idSIMD_SSE2::BlendJoints( idJointQuat* joints, const idJointQuat* blendJoints, const float lerp, const int* index, const int numJoints )
{

    if( lerp <= 0.0f )
    {
        return;
    }
    else if( lerp >= 1.0f )
    {
        for( int i = 0; i < numJoints; i++ )
        {
            int j = index[i];
            joints[j] = blendJoints[j];
        }
        return;
    }

    const __m128 vlerp = { lerp, lerp, lerp, lerp };

    const __m128 vector_float_one		= { 1.0f, 1.0f, 1.0f, 1.0f };
    const __m128 vector_float_sign_bit	= __m128c( _mm_set_epi32( 0x80000000, 0x80000000, 0x80000000, 0x80000000 ) );
    const __m128 vector_float_rsqrt_c0	= {  -3.0f,  -3.0f,  -3.0f,  -3.0f };
    const __m128 vector_float_rsqrt_c1	= {  -0.5f,  -0.5f,  -0.5f,  -0.5f };
    const __m128 vector_float_tiny		= {    1e-10f,    1e-10f,    1e-10f,    1e-10f };
    const __m128 vector_float_half_pi	= { M_PI * 0.5f, M_PI * 0.5f, M_PI * 0.5f, M_PI * 0.5f };

    const __m128 vector_float_sin_c0	= { -2.39e-08f, -2.39e-08f, -2.39e-08f, -2.39e-08f };
    const __m128 vector_float_sin_c1	= {  2.7526e-06f, 2.7526e-06f, 2.7526e-06f, 2.7526e-06f };
    const __m128 vector_float_sin_c2	= { -1.98409e-04f, -1.98409e-04f, -1.98409e-04f, -1.98409e-04f };
    const __m128 vector_float_sin_c3	= {  8.3333315e-03f, 8.3333315e-03f, 8.3333315e-03f, 8.3333315e-03f };
    const __m128 vector_float_sin_c4	= { -1.666666664e-01f, -1.666666664e-01f, -1.666666664e-01f, -1.666666664e-01f };

    const __m128 vector_float_atan_c0	= {  0.0028662257f,  0.0028662257f,  0.0028662257f,  0.0028662257f };
    const __m128 vector_float_atan_c1	= { -0.0161657367f, -0.0161657367f, -0.0161657367f, -0.0161657367f };
    const __m128 vector_float_atan_c2	= {  0.0429096138f,  0.0429096138f,  0.0429096138f,  0.0429096138f };
    const __m128 vector_float_atan_c3	= { -0.0752896400f, -0.0752896400f, -0.0752896400f, -0.0752896400f };
    const __m128 vector_float_atan_c4	= {  0.1065626393f,  0.1065626393f,  0.1065626393f,  0.1065626393f };
    const __m128 vector_float_atan_c5	= { -0.1420889944f, -0.1420889944f, -0.1420889944f, -0.1420889944f };
    const __m128 vector_float_atan_c6	= {  0.1999355085f,  0.1999355085f,  0.1999355085f,  0.1999355085f };
    const __m128 vector_float_atan_c7	= { -0.3333314528f, -0.3333314528f, -0.3333314528f, -0.3333314528f };

    int i = 0;
    for( ; i < numJoints - 3; i += 4 )
    {
        const int n0 = index[i + 0];
        const int n1 = index[i + 1];
        const int n2 = index[i + 2];
        const int n3 = index[i + 3];

        __m128 jqa_0 = _mm_load_ps( joints[n0].q.ToFloatPtr() );
        __m128 jqb_0 = _mm_load_ps( joints[n1].q.ToFloatPtr() );
        __m128 jqc_0 = _mm_load_ps( joints[n2].q.ToFloatPtr() );
        __m128 jqd_0 = _mm_load_ps( joints[n3].q.ToFloatPtr() );

#if JOINTQUAT_T_32
        __m128 jta_0 = _mm_load_ps( joints[n0].t.ToFloatPtr() );
		__m128 jtb_0 = _mm_load_ps( joints[n1].t.ToFloatPtr() );
		__m128 jtc_0 = _mm_load_ps( joints[n2].t.ToFloatPtr() );
		__m128 jtd_0 = _mm_load_ps( joints[n3].t.ToFloatPtr() );
#else
        __m128 jta_0 = _mm_load_ps_3( joints[n0].t.ToFloatPtr() );
        __m128 jtb_0 = _mm_load_ps_3( joints[n1].t.ToFloatPtr() );
        __m128 jtc_0 = _mm_load_ps_3( joints[n2].t.ToFloatPtr() );
        __m128 jtd_0 = _mm_load_ps_3( joints[n3].t.ToFloatPtr() );
#endif

        __m128 bqa_0 = _mm_load_ps( blendJoints[n0].q.ToFloatPtr() );
        __m128 bqb_0 = _mm_load_ps( blendJoints[n1].q.ToFloatPtr() );
        __m128 bqc_0 = _mm_load_ps( blendJoints[n2].q.ToFloatPtr() );
        __m128 bqd_0 = _mm_load_ps( blendJoints[n3].q.ToFloatPtr() );

#if JOINTQUAT_T_32
        __m128 bta_0 = _mm_load_ps( blendJoints[n0].t.ToFloatPtr() );
		__m128 btb_0 = _mm_load_ps( blendJoints[n1].t.ToFloatPtr() );
		__m128 btc_0 = _mm_load_ps( blendJoints[n2].t.ToFloatPtr() );
		__m128 btd_0 = _mm_load_ps( blendJoints[n3].t.ToFloatPtr() );
#else
        __m128 bta_0 = _mm_load_ps_3( blendJoints[n0].t.ToFloatPtr() );
        __m128 btb_0 = _mm_load_ps_3( blendJoints[n1].t.ToFloatPtr() );
        __m128 btc_0 = _mm_load_ps_3( blendJoints[n2].t.ToFloatPtr() );
        __m128 btd_0 = _mm_load_ps_3( blendJoints[n3].t.ToFloatPtr() );
#endif

        bta_0 = _mm_sub_ps( bta_0, jta_0 );
        btb_0 = _mm_sub_ps( btb_0, jtb_0 );
        btc_0 = _mm_sub_ps( btc_0, jtc_0 );
        btd_0 = _mm_sub_ps( btd_0, jtd_0 );

        jta_0 = _mm_madd_ps( vlerp, bta_0, jta_0 );
        jtb_0 = _mm_madd_ps( vlerp, btb_0, jtb_0 );
        jtc_0 = _mm_madd_ps( vlerp, btc_0, jtc_0 );
        jtd_0 = _mm_madd_ps( vlerp, btd_0, jtd_0 );

#if JOINTQUAT_T_32
        _mm_store_ps( joints[n0].t.ToFloatPtr(), jta_0 );
		_mm_store_ps( joints[n1].t.ToFloatPtr(), jtb_0 );
		_mm_store_ps( joints[n2].t.ToFloatPtr(), jtc_0 );
		_mm_store_ps( joints[n3].t.ToFloatPtr(), jtd_0 );
#else
        _mm_store_ps_3( joints[n0].t.ToFloatPtr(), jta_0 );
        _mm_store_ps_3( joints[n1].t.ToFloatPtr(), jtb_0 );
        _mm_store_ps_3( joints[n2].t.ToFloatPtr(), jtc_0 );
        _mm_store_ps_3( joints[n3].t.ToFloatPtr(), jtd_0 );
#endif

        __m128 jqr_0 = _mm_unpacklo_ps( jqa_0, jqc_0 );
        __m128 jqs_0 = _mm_unpackhi_ps( jqa_0, jqc_0 );
        __m128 jqt_0 = _mm_unpacklo_ps( jqb_0, jqd_0 );
        __m128 jqu_0 = _mm_unpackhi_ps( jqb_0, jqd_0 );

        __m128 bqr_0 = _mm_unpacklo_ps( bqa_0, bqc_0 );
        __m128 bqs_0 = _mm_unpackhi_ps( bqa_0, bqc_0 );
        __m128 bqt_0 = _mm_unpacklo_ps( bqb_0, bqd_0 );
        __m128 bqu_0 = _mm_unpackhi_ps( bqb_0, bqd_0 );

        __m128 jqx_0 = _mm_unpacklo_ps( jqr_0, jqt_0 );
        __m128 jqy_0 = _mm_unpackhi_ps( jqr_0, jqt_0 );
        __m128 jqz_0 = _mm_unpacklo_ps( jqs_0, jqu_0 );
        __m128 jqw_0 = _mm_unpackhi_ps( jqs_0, jqu_0 );

        __m128 bqx_0 = _mm_unpacklo_ps( bqr_0, bqt_0 );
        __m128 bqy_0 = _mm_unpackhi_ps( bqr_0, bqt_0 );
        __m128 bqz_0 = _mm_unpacklo_ps( bqs_0, bqu_0 );
        __m128 bqw_0 = _mm_unpackhi_ps( bqs_0, bqu_0 );

        __m128 cosoma_0 = _mm_mul_ps( jqx_0, bqx_0 );
        __m128 cosomb_0 = _mm_mul_ps( jqy_0, bqy_0 );
        __m128 cosomc_0 = _mm_mul_ps( jqz_0, bqz_0 );
        __m128 cosomd_0 = _mm_mul_ps( jqw_0, bqw_0 );

        __m128 cosome_0 = _mm_add_ps( cosoma_0, cosomb_0 );
        __m128 cosomf_0 = _mm_add_ps( cosomc_0, cosomd_0 );
        __m128 cosomg_0 = _mm_add_ps( cosome_0, cosomf_0 );

        __m128 sign_0 = _mm_and_ps( cosomg_0, vector_float_sign_bit );
        __m128 cosom_0 = _mm_xor_ps( cosomg_0, sign_0 );
        __m128 ss_0 = _mm_nmsub_ps( cosom_0, cosom_0, vector_float_one );

        ss_0 = _mm_max_ps( ss_0, vector_float_tiny );

        __m128 rs_0 = _mm_rsqrt_ps( ss_0 );
        __m128 sq_0 = _mm_mul_ps( rs_0, rs_0 );
        __m128 sh_0 = _mm_mul_ps( rs_0, vector_float_rsqrt_c1 );
        __m128 sx_0 = _mm_madd_ps( ss_0, sq_0, vector_float_rsqrt_c0 );
        __m128 sinom_0 = _mm_mul_ps( sh_0, sx_0 );						// sinom = sqrt( ss );

        ss_0 = _mm_mul_ps( ss_0, sinom_0 );

        __m128 min_0 = _mm_min_ps( ss_0, cosom_0 );
        __m128 max_0 = _mm_max_ps( ss_0, cosom_0 );
        __m128 mask_0 = _mm_cmpeq_ps( min_0, cosom_0 );
        __m128 masksign_0 = _mm_and_ps( mask_0, vector_float_sign_bit );
        __m128 maskPI_0 = _mm_and_ps( mask_0, vector_float_half_pi );

        __m128 rcpa_0 = _mm_rcp_ps( max_0 );
        __m128 rcpb_0 = _mm_mul_ps( max_0, rcpa_0 );
        __m128 rcpd_0 = _mm_add_ps( rcpa_0, rcpa_0 );
        __m128 rcp_0 = _mm_nmsub_ps( rcpb_0, rcpa_0, rcpd_0 );			// 1 / y or 1 / x
        __m128 ata_0 = _mm_mul_ps( min_0, rcp_0 );						// x / y or y / x

        __m128 atb_0 = _mm_xor_ps( ata_0, masksign_0 );					// -x / y or y / x
        __m128 atc_0 = _mm_mul_ps( atb_0, atb_0 );
        __m128 atd_0 = _mm_madd_ps( atc_0, vector_float_atan_c0, vector_float_atan_c1 );

        atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c2 );
        atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c3 );
        atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c4 );
        atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c5 );
        atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c6 );
        atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_atan_c7 );
        atd_0 = _mm_madd_ps( atd_0, atc_0, vector_float_one );

        __m128 omega_a_0 = _mm_madd_ps( atd_0, atb_0, maskPI_0 );
        __m128 omega_b_0 = _mm_mul_ps( vlerp, omega_a_0 );
        omega_a_0 = _mm_sub_ps( omega_a_0, omega_b_0 );

        __m128 sinsa_0 = _mm_mul_ps( omega_a_0, omega_a_0 );
        __m128 sinsb_0 = _mm_mul_ps( omega_b_0, omega_b_0 );
        __m128 sina_0 = _mm_madd_ps( sinsa_0, vector_float_sin_c0, vector_float_sin_c1 );
        __m128 sinb_0 = _mm_madd_ps( sinsb_0, vector_float_sin_c0, vector_float_sin_c1 );
        sina_0 = _mm_madd_ps( sina_0, sinsa_0, vector_float_sin_c2 );
        sinb_0 = _mm_madd_ps( sinb_0, sinsb_0, vector_float_sin_c2 );
        sina_0 = _mm_madd_ps( sina_0, sinsa_0, vector_float_sin_c3 );
        sinb_0 = _mm_madd_ps( sinb_0, sinsb_0, vector_float_sin_c3 );
        sina_0 = _mm_madd_ps( sina_0, sinsa_0, vector_float_sin_c4 );
        sinb_0 = _mm_madd_ps( sinb_0, sinsb_0, vector_float_sin_c4 );
        sina_0 = _mm_madd_ps( sina_0, sinsa_0, vector_float_one );
        sinb_0 = _mm_madd_ps( sinb_0, sinsb_0, vector_float_one );
        sina_0 = _mm_mul_ps( sina_0, omega_a_0 );
        sinb_0 = _mm_mul_ps( sinb_0, omega_b_0 );
        __m128 scalea_0 = _mm_mul_ps( sina_0, sinom_0 );
        __m128 scaleb_0 = _mm_mul_ps( sinb_0, sinom_0 );

        scaleb_0 = _mm_xor_ps( scaleb_0, sign_0 );

        jqx_0 = _mm_mul_ps( jqx_0, scalea_0 );
        jqy_0 = _mm_mul_ps( jqy_0, scalea_0 );
        jqz_0 = _mm_mul_ps( jqz_0, scalea_0 );
        jqw_0 = _mm_mul_ps( jqw_0, scalea_0 );

        jqx_0 = _mm_madd_ps( bqx_0, scaleb_0, jqx_0 );
        jqy_0 = _mm_madd_ps( bqy_0, scaleb_0, jqy_0 );
        jqz_0 = _mm_madd_ps( bqz_0, scaleb_0, jqz_0 );
        jqw_0 = _mm_madd_ps( bqw_0, scaleb_0, jqw_0 );

        __m128 tp0_0 = _mm_unpacklo_ps( jqx_0, jqz_0 );
        __m128 tp1_0 = _mm_unpackhi_ps( jqx_0, jqz_0 );
        __m128 tp2_0 = _mm_unpacklo_ps( jqy_0, jqw_0 );
        __m128 tp3_0 = _mm_unpackhi_ps( jqy_0, jqw_0 );

        __m128 p0_0 = _mm_unpacklo_ps( tp0_0, tp2_0 );
        __m128 p1_0 = _mm_unpackhi_ps( tp0_0, tp2_0 );
        __m128 p2_0 = _mm_unpacklo_ps( tp1_0, tp3_0 );
        __m128 p3_0 = _mm_unpackhi_ps( tp1_0, tp3_0 );

        _mm_store_ps( joints[n0].q.ToFloatPtr(), p0_0 );
        _mm_store_ps( joints[n1].q.ToFloatPtr(), p1_0 );
        _mm_store_ps( joints[n2].q.ToFloatPtr(), p2_0 );
        _mm_store_ps( joints[n3].q.ToFloatPtr(), p3_0 );
    }

    for( ; i < numJoints; i++ )
    {
        int n = index[i];

        idVec3& jointVert = joints[n].t;
        const idVec3& blendVert = blendJoints[n].t;

        jointVert[0] += lerp * ( blendVert[0] - jointVert[0] );
        jointVert[1] += lerp * ( blendVert[1] - jointVert[1] );
        jointVert[2] += lerp * ( blendVert[2] - jointVert[2] );
        //joints[n].w = 0.0f;

        idQuat& jointQuat = joints[n].q;
        const idQuat& blendQuat = blendJoints[n].q;

        float cosom;
        float sinom;
        float omega;
        float scale0;
        float scale1;
        // DG: use int instead of long for 64bit compatibility
        unsigned int signBit;
        // DG end

        cosom = jointQuat.x * blendQuat.x + jointQuat.y * blendQuat.y + jointQuat.z * blendQuat.z + jointQuat.w * blendQuat.w;

        // DG: use int instead of long for 64bit compatibility
        signBit = ( *( unsigned int* )&cosom ) & ( 1 << 31 );

        ( *( unsigned int* )&cosom ) ^= signBit;
        // DG end

        scale0 = 1.0f - cosom * cosom;
        scale0 = ( scale0 <= 0.0f ) ? 1e-10f : scale0;
        sinom = idMath::InvSqrt( scale0 );
        omega = idMath::ATan16( scale0 * sinom, cosom );
        scale0 = idMath::Sin16( ( 1.0f - lerp ) * omega ) * sinom;
        scale1 = idMath::Sin16( lerp * omega ) * sinom;

        ( *( unsigned int* )&scale1 ) ^= signBit; // DG: use int instead of long for 64bit compatibility

        jointQuat.x = scale0 * jointQuat.x + scale1 * blendQuat.x;
        jointQuat.y = scale0 * jointQuat.y + scale1 * blendQuat.y;
        jointQuat.z = scale0 * jointQuat.z + scale1 * blendQuat.z;
        jointQuat.w = scale0 * jointQuat.w + scale1 * blendQuat.w;
    }
}

/*
============
idSIMD_SSE::ConvertJointQuatsToJointMats
============
*/
void VPCALL idSIMD_SSE2::ConvertJointQuatsToJointMats( idJointMat* jointMats, const idJointQuat* jointQuats, const int numJoints )
{
    ID_STATIC_ASSERT( sizeof( idJointQuat ) == JOINTQUAT_SIZE );
    ID_STATIC_ASSERT( sizeof( idJointMat ) == JOINTMAT_SIZE );

    // RB: changed int to intptr_t
    ID_STATIC_ASSERT( ( intptr_t )( &( ( idJointQuat* )0 )->t ) == ( intptr_t )( &( ( idJointQuat* )0 )->q ) + ( intptr_t )sizeof( ( ( idJointQuat* )0 )->q ) );
    // RB end

    const float* jointQuatPtr = ( float* )jointQuats;
    float* jointMatPtr = ( float* )jointMats;

    const __m128 vector_float_first_sign_bit		= __m128c( _mm_set_epi32( 0x00000000, 0x00000000, 0x00000000, 0x80000000 ) );
    const __m128 vector_float_last_three_sign_bits	= __m128c( _mm_set_epi32( 0x80000000, 0x80000000, 0x80000000, 0x00000000 ) );
    const __m128 vector_float_first_pos_half		= {   0.5f,   0.0f,   0.0f,   0.0f };	// +.5 0 0 0
    const __m128 vector_float_first_neg_half		= {  -0.5f,   0.0f,   0.0f,   0.0f };	// -.5 0 0 0
    const __m128 vector_float_quat2mat_mad1			= {  -1.0f,  -1.0f,  +1.0f,  -1.0f };	//  - - + -
    const __m128 vector_float_quat2mat_mad2			= {  -1.0f,  +1.0f,  -1.0f,  -1.0f };	//  - + - -
    const __m128 vector_float_quat2mat_mad3			= {  +1.0f,  -1.0f,  -1.0f,  +1.0f };	//  + - - +

    int i = 0;
    for( ; i + 1 < numJoints; i += 2 )
    {

        __m128 q0 = _mm_load_ps( &jointQuatPtr[i * JOINTQUAT_STRIDE + 0 * JOINTQUAT_STRIDE + 0] );
        __m128 q1 = _mm_load_ps( &jointQuatPtr[i * JOINTQUAT_STRIDE + 1 * JOINTQUAT_STRIDE + 0] );

#if JOINTQUAT_T_32
        __m128 t0 = _mm_load_ps( &jointQuatPtr[i * JOINTQUAT_STRIDE + 0 * JOINTQUAT_STRIDE + 4] );
		__m128 t1 = _mm_load_ps( &jointQuatPtr[i * JOINTQUAT_STRIDE + 1 * JOINTQUAT_STRIDE + 4] );
#else
        __m128 t0 = _mm_load_ps_3( &jointQuatPtr[i * JOINTQUAT_STRIDE + 0 * JOINTQUAT_STRIDE + 4] );
        __m128 t1 = _mm_load_ps_3( &jointQuatPtr[i * JOINTQUAT_STRIDE + 1 * JOINTQUAT_STRIDE + 4] );
#endif

        __m128 d0 = _mm_add_ps( q0, q0 );
        __m128 d1 = _mm_add_ps( q1, q1 );

        __m128 sa0 = _mm_perm_ps( q0, _MM_SHUFFLE( 1, 0, 0, 1 ) );							//   y,   x,   x,   y
        __m128 sb0 = _mm_perm_ps( d0, _MM_SHUFFLE( 2, 2, 1, 1 ) );							//  y2,  y2,  z2,  z2
        __m128 sc0 = _mm_perm_ps( q0, _MM_SHUFFLE( 3, 3, 3, 2 ) );							//   z,   w,   w,   w
        __m128 sd0 = _mm_perm_ps( d0, _MM_SHUFFLE( 0, 1, 2, 2 ) );							//  z2,  z2,  y2,  x2
        __m128 sa1 = _mm_perm_ps( q1, _MM_SHUFFLE( 1, 0, 0, 1 ) );							//   y,   x,   x,   y
        __m128 sb1 = _mm_perm_ps( d1, _MM_SHUFFLE( 2, 2, 1, 1 ) );							//  y2,  y2,  z2,  z2
        __m128 sc1 = _mm_perm_ps( q1, _MM_SHUFFLE( 3, 3, 3, 2 ) );							//   z,   w,   w,   w
        __m128 sd1 = _mm_perm_ps( d1, _MM_SHUFFLE( 0, 1, 2, 2 ) );							//  z2,  z2,  y2,  x2

        sa0 = _mm_xor_ps( sa0, vector_float_first_sign_bit );
        sa1 = _mm_xor_ps( sa1, vector_float_first_sign_bit );

        sc0 = _mm_xor_ps( sc0, vector_float_last_three_sign_bits );							// flip stupid inverse quaternions
        sc1 = _mm_xor_ps( sc1, vector_float_last_three_sign_bits );							// flip stupid inverse quaternions

        __m128 ma0 = _mm_add_ps( _mm_mul_ps( sa0, sb0 ), vector_float_first_pos_half );		//  .5 - yy2,  xy2,  xz2,  yz2		//  .5 0 0 0
        __m128 mb0 = _mm_add_ps( _mm_mul_ps( sc0, sd0 ), vector_float_first_neg_half );		// -.5 + zz2,  wz2,  wy2,  wx2		// -.5 0 0 0
        __m128 mc0 = _mm_sub_ps( vector_float_first_pos_half, _mm_mul_ps( q0, d0 ) );		//  .5 - xx2, -yy2, -zz2, -ww2		//  .5 0 0 0
        __m128 ma1 = _mm_add_ps( _mm_mul_ps( sa1, sb1 ), vector_float_first_pos_half );		//  .5 - yy2,  xy2,  xz2,  yz2		//  .5 0 0 0
        __m128 mb1 = _mm_add_ps( _mm_mul_ps( sc1, sd1 ), vector_float_first_neg_half );		// -.5 + zz2,  wz2,  wy2,  wx2		// -.5 0 0 0
        __m128 mc1 = _mm_sub_ps( vector_float_first_pos_half, _mm_mul_ps( q1, d1 ) );		//  .5 - xx2, -yy2, -zz2, -ww2		//  .5 0 0 0

        __m128 mf0 = _mm_shuffle_ps( ma0, mc0, _MM_SHUFFLE( 0, 0, 1, 1 ) );					//       xy2,  xy2, .5 - xx2, .5 - xx2	// 01, 01, 10, 10
        __m128 md0 = _mm_shuffle_ps( mf0, ma0, _MM_SHUFFLE( 3, 2, 0, 2 ) );					//  .5 - xx2,  xy2,  xz2,  yz2			// 10, 01, 02, 03
        __m128 me0 = _mm_shuffle_ps( ma0, mb0, _MM_SHUFFLE( 3, 2, 1, 0 ) );					//  .5 - yy2,  xy2,  wy2,  wx2			// 00, 01, 12, 13
        __m128 mf1 = _mm_shuffle_ps( ma1, mc1, _MM_SHUFFLE( 0, 0, 1, 1 ) );					//       xy2,  xy2, .5 - xx2, .5 - xx2	// 01, 01, 10, 10
        __m128 md1 = _mm_shuffle_ps( mf1, ma1, _MM_SHUFFLE( 3, 2, 0, 2 ) );					//  .5 - xx2,  xy2,  xz2,  yz2			// 10, 01, 02, 03
        __m128 me1 = _mm_shuffle_ps( ma1, mb1, _MM_SHUFFLE( 3, 2, 1, 0 ) );					//  .5 - yy2,  xy2,  wy2,  wx2			// 00, 01, 12, 13

        __m128 ra0 = _mm_add_ps( _mm_mul_ps( mb0, vector_float_quat2mat_mad1 ), ma0 );		// 1 - yy2 - zz2, xy2 - wz2, xz2 + wy2,					// - - + -
        __m128 rb0 = _mm_add_ps( _mm_mul_ps( mb0, vector_float_quat2mat_mad2 ), md0 );		// 1 - xx2 - zz2, xy2 + wz2,          , yz2 - wx2		// - + - -
        __m128 rc0 = _mm_add_ps( _mm_mul_ps( me0, vector_float_quat2mat_mad3 ), md0 );		// 1 - xx2 - yy2,          , xz2 - wy2, yz2 + wx2		// + - - +
        __m128 ra1 = _mm_add_ps( _mm_mul_ps( mb1, vector_float_quat2mat_mad1 ), ma1 );		// 1 - yy2 - zz2, xy2 - wz2, xz2 + wy2,					// - - + -
        __m128 rb1 = _mm_add_ps( _mm_mul_ps( mb1, vector_float_quat2mat_mad2 ), md1 );		// 1 - xx2 - zz2, xy2 + wz2,          , yz2 - wx2		// - + - -
        __m128 rc1 = _mm_add_ps( _mm_mul_ps( me1, vector_float_quat2mat_mad3 ), md1 );		// 1 - xx2 - yy2,          , xz2 - wy2, yz2 + wx2		// + - - +

        __m128 ta0 = _mm_shuffle_ps( ra0, t0, _MM_SHUFFLE( 0, 0, 2, 2 ) );
        __m128 tb0 = _mm_shuffle_ps( rb0, t0, _MM_SHUFFLE( 1, 1, 3, 3 ) );
        __m128 tc0 = _mm_shuffle_ps( rc0, t0, _MM_SHUFFLE( 2, 2, 0, 0 ) );
        __m128 ta1 = _mm_shuffle_ps( ra1, t1, _MM_SHUFFLE( 0, 0, 2, 2 ) );
        __m128 tb1 = _mm_shuffle_ps( rb1, t1, _MM_SHUFFLE( 1, 1, 3, 3 ) );
        __m128 tc1 = _mm_shuffle_ps( rc1, t1, _MM_SHUFFLE( 2, 2, 0, 0 ) );

        ra0 = _mm_shuffle_ps( ra0, ta0, _MM_SHUFFLE( 2, 0, 1, 0 ) );						// 00 01 02 10
        rb0 = _mm_shuffle_ps( rb0, tb0, _MM_SHUFFLE( 2, 0, 0, 1 ) );						// 01 00 03 11
        rc0 = _mm_shuffle_ps( rc0, tc0, _MM_SHUFFLE( 2, 0, 3, 2 ) );						// 02 03 00 12
        ra1 = _mm_shuffle_ps( ra1, ta1, _MM_SHUFFLE( 2, 0, 1, 0 ) );						// 00 01 02 10
        rb1 = _mm_shuffle_ps( rb1, tb1, _MM_SHUFFLE( 2, 0, 0, 1 ) );						// 01 00 03 11
        rc1 = _mm_shuffle_ps( rc1, tc1, _MM_SHUFFLE( 2, 0, 3, 2 ) );						// 02 03 00 12

        _mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 0], ra0 );
        _mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 4], rb0 );
        _mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 8], rc0 );
        _mm_store_ps( &jointMatPtr[i * 12 + 1 * 12 + 0], ra1 );
        _mm_store_ps( &jointMatPtr[i * 12 + 1 * 12 + 4], rb1 );
        _mm_store_ps( &jointMatPtr[i * 12 + 1 * 12 + 8], rc1 );
    }

    for( ; i < numJoints; i++ )
    {

        __m128 q0 = _mm_load_ps( &jointQuatPtr[i * JOINTQUAT_STRIDE + 0 * JOINTQUAT_STRIDE + 0] );
#if JOINTQUAT_T_32
        __m128 t0 = _mm_load_ps( &jointQuatPtr[i * JOINTQUAT_STRIDE + 0 * JOINTQUAT_STRIDE + 4] );
#else
        __m128 t0 = _mm_load_ps_3( &jointQuatPtr[i * JOINTQUAT_STRIDE + 0 * JOINTQUAT_STRIDE + 4] );
#endif

        __m128 d0 = _mm_add_ps( q0, q0 );

        __m128 sa0 = _mm_perm_ps( q0, _MM_SHUFFLE( 1, 0, 0, 1 ) );							//   y,   x,   x,   y
        __m128 sb0 = _mm_perm_ps( d0, _MM_SHUFFLE( 2, 2, 1, 1 ) );							//  y2,  y2,  z2,  z2
        __m128 sc0 = _mm_perm_ps( q0, _MM_SHUFFLE( 3, 3, 3, 2 ) );							//   z,   w,   w,   w
        __m128 sd0 = _mm_perm_ps( d0, _MM_SHUFFLE( 0, 1, 2, 2 ) );							//  z2,  z2,  y2,  x2

        sa0 = _mm_xor_ps( sa0, vector_float_first_sign_bit );
        sc0 = _mm_xor_ps( sc0, vector_float_last_three_sign_bits );							// flip stupid inverse quaternions

        __m128 ma0 = _mm_add_ps( _mm_mul_ps( sa0, sb0 ), vector_float_first_pos_half );		//  .5 - yy2,  xy2,  xz2,  yz2		//  .5 0 0 0
        __m128 mb0 = _mm_add_ps( _mm_mul_ps( sc0, sd0 ), vector_float_first_neg_half );		// -.5 + zz2,  wz2,  wy2,  wx2		// -.5 0 0 0
        __m128 mc0 = _mm_sub_ps( vector_float_first_pos_half, _mm_mul_ps( q0, d0 ) );		//  .5 - xx2, -yy2, -zz2, -ww2		//  .5 0 0 0

        __m128 mf0 = _mm_shuffle_ps( ma0, mc0, _MM_SHUFFLE( 0, 0, 1, 1 ) );					//       xy2,  xy2, .5 - xx2, .5 - xx2	// 01, 01, 10, 10
        __m128 md0 = _mm_shuffle_ps( mf0, ma0, _MM_SHUFFLE( 3, 2, 0, 2 ) );					//  .5 - xx2,  xy2,  xz2,  yz2			// 10, 01, 02, 03
        __m128 me0 = _mm_shuffle_ps( ma0, mb0, _MM_SHUFFLE( 3, 2, 1, 0 ) );					//  .5 - yy2,  xy2,  wy2,  wx2			// 00, 01, 12, 13

        __m128 ra0 = _mm_add_ps( _mm_mul_ps( mb0, vector_float_quat2mat_mad1 ), ma0 );		// 1 - yy2 - zz2, xy2 - wz2, xz2 + wy2,					// - - + -
        __m128 rb0 = _mm_add_ps( _mm_mul_ps( mb0, vector_float_quat2mat_mad2 ), md0 );		// 1 - xx2 - zz2, xy2 + wz2,          , yz2 - wx2		// - + - -
        __m128 rc0 = _mm_add_ps( _mm_mul_ps( me0, vector_float_quat2mat_mad3 ), md0 );		// 1 - xx2 - yy2,          , xz2 - wy2, yz2 + wx2		// + - - +

        __m128 ta0 = _mm_shuffle_ps( ra0, t0, _MM_SHUFFLE( 0, 0, 2, 2 ) );
        __m128 tb0 = _mm_shuffle_ps( rb0, t0, _MM_SHUFFLE( 1, 1, 3, 3 ) );
        __m128 tc0 = _mm_shuffle_ps( rc0, t0, _MM_SHUFFLE( 2, 2, 0, 0 ) );

        ra0 = _mm_shuffle_ps( ra0, ta0, _MM_SHUFFLE( 2, 0, 1, 0 ) );						// 00 01 02 10
        rb0 = _mm_shuffle_ps( rb0, tb0, _MM_SHUFFLE( 2, 0, 0, 1 ) );						// 01 00 03 11
        rc0 = _mm_shuffle_ps( rc0, tc0, _MM_SHUFFLE( 2, 0, 3, 2 ) );						// 02 03 00 12

        _mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 0], ra0 );
        _mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 4], rb0 );
        _mm_store_ps( &jointMatPtr[i * 12 + 0 * 12 + 8], rc0 );
    }
}

/*
============
idSIMD_SSE::ConvertJointMatsToJointQuats
============
*/
void VPCALL idSIMD_SSE2::ConvertJointMatsToJointQuats( idJointQuat* jointQuats, const idJointMat* jointMats, const int numJoints )
{

    ID_STATIC_ASSERT( sizeof( idJointQuat ) == JOINTQUAT_SIZE );
    ID_STATIC_ASSERT( sizeof( idJointMat ) == JOINTMAT_SIZE );

    // RB: changed int to intptr_t
    ID_STATIC_ASSERT( ( intptr_t )( &( ( idJointQuat* )0 )->t ) == ( intptr_t )( &( ( idJointQuat* )0 )->q ) + ( intptr_t )sizeof( ( ( idJointQuat* )0 )->q ) );
    // RB end

    const __m128 vector_float_zero		= _mm_setzero_ps();
    const __m128 vector_float_one		= { 1.0f, 1.0f, 1.0f, 1.0f };
    const __m128 vector_float_not		= __m128c( _mm_set_epi32( -1, -1, -1, -1 ) );
    const __m128 vector_float_sign_bit	= __m128c( _mm_set_epi32( 0x80000000, 0x80000000, 0x80000000, 0x80000000 ) );
    const __m128 vector_float_rsqrt_c0	= {  -3.0f,  -3.0f,  -3.0f,  -3.0f };
    const __m128 vector_float_rsqrt_c2	= { -0.25f, -0.25f, -0.25f, -0.25f };

    int i = 0;
    for( ; i < numJoints - 3; i += 4 )
    {
        const float* __restrict m = ( float* )&jointMats[i];
        float* __restrict q = ( float* )&jointQuats[i];

        __m128 ma0 = _mm_load_ps( &m[0 * 12 + 0] );
        __m128 ma1 = _mm_load_ps( &m[0 * 12 + 4] );
        __m128 ma2 = _mm_load_ps( &m[0 * 12 + 8] );

        __m128 mb0 = _mm_load_ps( &m[1 * 12 + 0] );
        __m128 mb1 = _mm_load_ps( &m[1 * 12 + 4] );
        __m128 mb2 = _mm_load_ps( &m[1 * 12 + 8] );

        __m128 mc0 = _mm_load_ps( &m[2 * 12 + 0] );
        __m128 mc1 = _mm_load_ps( &m[2 * 12 + 4] );
        __m128 mc2 = _mm_load_ps( &m[2 * 12 + 8] );

        __m128 md0 = _mm_load_ps( &m[3 * 12 + 0] );
        __m128 md1 = _mm_load_ps( &m[3 * 12 + 4] );
        __m128 md2 = _mm_load_ps( &m[3 * 12 + 8] );

        __m128 ta0 = _mm_unpacklo_ps( ma0, mc0 );	// a0, c0, a1, c1
        __m128 ta1 = _mm_unpackhi_ps( ma0, mc0 );	// a2, c2, a3, c3
        __m128 ta2 = _mm_unpacklo_ps( mb0, md0 );	// b0, d0, b1, b2
        __m128 ta3 = _mm_unpackhi_ps( mb0, md0 );	// b2, d2, b3, d3

        __m128 tb0 = _mm_unpacklo_ps( ma1, mc1 );	// a0, c0, a1, c1
        __m128 tb1 = _mm_unpackhi_ps( ma1, mc1 );	// a2, c2, a3, c3
        __m128 tb2 = _mm_unpacklo_ps( mb1, md1 );	// b0, d0, b1, b2
        __m128 tb3 = _mm_unpackhi_ps( mb1, md1 );	// b2, d2, b3, d3

        __m128 tc0 = _mm_unpacklo_ps( ma2, mc2 );	// a0, c0, a1, c1
        __m128 tc1 = _mm_unpackhi_ps( ma2, mc2 );	// a2, c2, a3, c3
        __m128 tc2 = _mm_unpacklo_ps( mb2, md2 );	// b0, d0, b1, b2
        __m128 tc3 = _mm_unpackhi_ps( mb2, md2 );	// b2, d2, b3, d3

        __m128 m00 = _mm_unpacklo_ps( ta0, ta2 );
        __m128 m01 = _mm_unpackhi_ps( ta0, ta2 );
        __m128 m02 = _mm_unpacklo_ps( ta1, ta3 );
        __m128 m03 = _mm_unpackhi_ps( ta1, ta3 );

        __m128 m10 = _mm_unpacklo_ps( tb0, tb2 );
        __m128 m11 = _mm_unpackhi_ps( tb0, tb2 );
        __m128 m12 = _mm_unpacklo_ps( tb1, tb3 );
        __m128 m13 = _mm_unpackhi_ps( tb1, tb3 );

        __m128 m20 = _mm_unpacklo_ps( tc0, tc2 );
        __m128 m21 = _mm_unpackhi_ps( tc0, tc2 );
        __m128 m22 = _mm_unpacklo_ps( tc1, tc3 );
        __m128 m23 = _mm_unpackhi_ps( tc1, tc3 );

        __m128 b00 = _mm_add_ps( m00, m11 );
        __m128 b11 = _mm_cmpgt_ps( m00, m22 );
        __m128 b01 = _mm_add_ps( b00, m22 );
        __m128 b10 = _mm_cmpgt_ps( m00, m11 );
        __m128 b0  = _mm_cmpgt_ps( b01, vector_float_zero );
        __m128 b1  = _mm_and_ps( b10, b11 );
        __m128 b2  = _mm_cmpgt_ps( m11, m22 );

        __m128 m0  = b0;
        __m128 m1  = _mm_and_ps( _mm_xor_ps( b0, vector_float_not ), b1 );
        __m128 p1  = _mm_or_ps( b0, b1 );
        __m128 p2  = _mm_or_ps( p1, b2 );
        __m128 m2  = _mm_and_ps( _mm_xor_ps( p1, vector_float_not ), b2 );
        __m128 m3  = _mm_xor_ps( p2, vector_float_not );

        __m128 i0  = _mm_or_ps( m2, m3 );
        __m128 i1  = _mm_or_ps( m1, m3 );
        __m128 i2  = _mm_or_ps( m1, m2 );

        __m128 s0  = _mm_and_ps( i0, vector_float_sign_bit );
        __m128 s1  = _mm_and_ps( i1, vector_float_sign_bit );
        __m128 s2  = _mm_and_ps( i2, vector_float_sign_bit );

        m00 = _mm_xor_ps( m00, s0 );
        m11 = _mm_xor_ps( m11, s1 );
        m22 = _mm_xor_ps( m22, s2 );
        m21 = _mm_xor_ps( m21, s0 );
        m02 = _mm_xor_ps( m02, s1 );
        m10 = _mm_xor_ps( m10, s2 );

        __m128 t0  = _mm_add_ps( m00, m11 );
        __m128 t1  = _mm_add_ps( m22, vector_float_one );
        __m128 q0  = _mm_add_ps( t0, t1 );
        __m128 q1  = _mm_sub_ps( m01, m10 );
        __m128 q2  = _mm_sub_ps( m20, m02 );
        __m128 q3  = _mm_sub_ps( m12, m21 );

        __m128 rs = _mm_rsqrt_ps( q0 );
        __m128 sq = _mm_mul_ps( rs, rs );
        __m128 sh = _mm_mul_ps( rs, vector_float_rsqrt_c2 );
        __m128 sx = _mm_madd_ps( q0, sq, vector_float_rsqrt_c0 );
        __m128 s  = _mm_mul_ps( sh, sx );

        q0 = _mm_mul_ps( q0, s );
        q1 = _mm_mul_ps( q1, s );
        q2 = _mm_mul_ps( q2, s );
        q3 = _mm_mul_ps( q3, s );

        m0 = _mm_or_ps( m0, m2 );
        m2 = _mm_or_ps( m2, m3 );

        __m128 fq0 = _mm_sel_ps( q0, q3, m0 );
        __m128 fq1 = _mm_sel_ps( q1, q2, m0 );
        __m128 fq2 = _mm_sel_ps( q2, q1, m0 );
        __m128 fq3 = _mm_sel_ps( q3, q0, m0 );

        __m128 rq0 = _mm_sel_ps( fq0, fq2, m2 );
        __m128 rq1 = _mm_sel_ps( fq1, fq3, m2 );
        __m128 rq2 = _mm_sel_ps( fq2, fq0, m2 );
        __m128 rq3 = _mm_sel_ps( fq3, fq1, m2 );

        __m128 tq0 = _mm_unpacklo_ps( rq0, rq2 );
        __m128 tq1 = _mm_unpackhi_ps( rq0, rq2 );
        __m128 tq2 = _mm_unpacklo_ps( rq1, rq3 );
        __m128 tq3 = _mm_unpackhi_ps( rq1, rq3 );

        __m128 sq0 = _mm_unpacklo_ps( tq0, tq2 );
        __m128 sq1 = _mm_unpackhi_ps( tq0, tq2 );
        __m128 sq2 = _mm_unpacklo_ps( tq1, tq3 );
        __m128 sq3 = _mm_unpackhi_ps( tq1, tq3 );

        __m128 tt0 = _mm_unpacklo_ps( m03, m23 );
        __m128 tt1 = _mm_unpackhi_ps( m03, m23 );
        __m128 tt2 = _mm_unpacklo_ps( m13, vector_float_zero );
        __m128 tt3 = _mm_unpackhi_ps( m13, vector_float_zero );

        __m128 st0 = _mm_unpacklo_ps( tt0, tt2 );
        __m128 st1 = _mm_unpackhi_ps( tt0, tt2 );
        __m128 st2 = _mm_unpacklo_ps( tt1, tt3 );
        __m128 st3 = _mm_unpackhi_ps( tt1, tt3 );

#if JOINTQUAT_T_32
        _mm_store_ps( &q[0 * 4], sq0 );
		_mm_store_ps( &q[1 * 4], st0 );
		_mm_store_ps( &q[2 * 4], sq1 );
		_mm_store_ps( &q[3 * 4], st1 );
		_mm_store_ps( &q[4 * 4], sq2 );
		_mm_store_ps( &q[5 * 4], st2 );
		_mm_store_ps( &q[6 * 4], sq3 );
		_mm_store_ps( &q[7 * 4], st3 );
#else
        _mm_store_ps( &q[0 * JOINTQUAT_STRIDE], sq0 );
        _mm_store_ps_3( &q[0 * JOINTQUAT_STRIDE + 4], st0 );
        _mm_store_ps( &q[1 * JOINTQUAT_STRIDE], sq1 );
        _mm_store_ps_3( &q[1 * JOINTQUAT_STRIDE + 4], st1 );
        _mm_store_ps( &q[2 * JOINTQUAT_STRIDE], sq2 );
        _mm_store_ps_3( &q[2 * JOINTQUAT_STRIDE + 4], st2 );
        _mm_store_ps( &q[3 * JOINTQUAT_STRIDE], sq3 );
        _mm_store_ps_3( &q[3 * JOINTQUAT_STRIDE + 4], st3 );
#endif
    }

    float sign[2] = { 1.0f, -1.0f };

    for( ; i < numJoints; i++ )
    {
        const float* __restrict m = ( float* )&jointMats[i];
        float* __restrict q = ( float* )&jointQuats[i];

        int b0 = m[0 * 4 + 0] + m[1 * 4 + 1] + m[2 * 4 + 2] > 0.0f;
        int b1 = m[0 * 4 + 0] > m[1 * 4 + 1] && m[0 * 4 + 0] > m[2 * 4 + 2];
        int b2 = m[1 * 4 + 1] > m[2 * 4 + 2];

        int m0 = b0;
        int m1 = ( !b0 ) & b1;
        int m2 = ( !( b0 | b1 ) ) & b2;
        int m3 = !( b0 | b1 | b2 );

        int i0 = ( m2 | m3 );
        int i1 = ( m1 | m3 );
        int i2 = ( m1 | m2 );

        float s0 = sign[i0];
        float s1 = sign[i1];
        float s2 = sign[i2];

        float t = s0 * m[0 * 4 + 0] + s1 * m[1 * 4 + 1] + s2 * m[2 * 4 + 2] + 1.0f;
        float s = __frsqrts( t );
        s = ( t * s * s + -3.0f ) * ( s * -0.25f );

        q[0] = t * s;
        q[1] = ( m[0 * 4 + 1] - s2 * m[1 * 4 + 0] ) * s;
        q[2] = ( m[2 * 4 + 0] - s1 * m[0 * 4 + 2] ) * s;
        q[3] = ( m[1 * 4 + 2] - s0 * m[2 * 4 + 1] ) * s;

        if( m0 | m2 )
        {
            // reverse
            SwapValues( q[0], q[3] );
            SwapValues( q[1], q[2] );
        }
        if( m2 | m3 )
        {
            // rotate 2
            SwapValues( q[0], q[2] );
            SwapValues( q[1], q[3] );
        }

        q[4] = m[0 * 4 + 3];
        q[5] = m[1 * 4 + 3];
        q[6] = m[2 * 4 + 3];
        q[7] = 0.0f;
    }
}

/*
============
idSIMD_SSE::TransformJoints
============
*/
void VPCALL idSIMD_SSE2::TransformJoints( idJointMat* jointMats, const int* parents, const int firstJoint, const int lastJoint )
{
    const __m128 vector_float_mask_keep_last	= __m128c( _mm_set_epi32( 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 ) );

    const float* __restrict firstMatrix = jointMats->ToFloatPtr() + ( firstJoint + firstJoint + firstJoint - 3 ) * 4;

    __m128 pma = _mm_load_ps( firstMatrix + 0 );
    __m128 pmb = _mm_load_ps( firstMatrix + 4 );
    __m128 pmc = _mm_load_ps( firstMatrix + 8 );

    for( int joint = firstJoint; joint <= lastJoint; joint++ )
    {
        const int parent = parents[joint];
        const float* __restrict parentMatrix = jointMats->ToFloatPtr() + ( parent + parent + parent ) * 4;
        float* __restrict childMatrix = jointMats->ToFloatPtr() + ( joint + joint + joint ) * 4;

        if( parent != joint - 1 )
        {
            pma = _mm_load_ps( parentMatrix + 0 );
            pmb = _mm_load_ps( parentMatrix + 4 );
            pmc = _mm_load_ps( parentMatrix + 8 );
        }

        __m128 cma = _mm_load_ps( childMatrix + 0 );
        __m128 cmb = _mm_load_ps( childMatrix + 4 );
        __m128 cmc = _mm_load_ps( childMatrix + 8 );

        __m128 ta = _mm_splat_ps( pma, 0 );
        __m128 tb = _mm_splat_ps( pmb, 0 );
        __m128 tc = _mm_splat_ps( pmc, 0 );

        __m128 td = _mm_splat_ps( pma, 1 );
        __m128 te = _mm_splat_ps( pmb, 1 );
        __m128 tf = _mm_splat_ps( pmc, 1 );

        __m128 tg = _mm_splat_ps( pma, 2 );
        __m128 th = _mm_splat_ps( pmb, 2 );
        __m128 ti = _mm_splat_ps( pmc, 2 );

        pma = _mm_madd_ps( ta, cma, _mm_and_ps( pma, vector_float_mask_keep_last ) );
        pmb = _mm_madd_ps( tb, cma, _mm_and_ps( pmb, vector_float_mask_keep_last ) );
        pmc = _mm_madd_ps( tc, cma, _mm_and_ps( pmc, vector_float_mask_keep_last ) );

        pma = _mm_madd_ps( td, cmb, pma );
        pmb = _mm_madd_ps( te, cmb, pmb );
        pmc = _mm_madd_ps( tf, cmb, pmc );

        pma = _mm_madd_ps( tg, cmc, pma );
        pmb = _mm_madd_ps( th, cmc, pmb );
        pmc = _mm_madd_ps( ti, cmc, pmc );

        _mm_store_ps( childMatrix + 0, pma );
        _mm_store_ps( childMatrix + 4, pmb );
        _mm_store_ps( childMatrix + 8, pmc );
    }
}

/*
============
idSIMD_SSE::UntransformJoints
============
*/
void VPCALL idSIMD_SSE2::UntransformJoints( idJointMat* jointMats, const int* parents, const int firstJoint, const int lastJoint )
{
    const __m128 vector_float_mask_keep_last	= __m128c( _mm_set_epi32( 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 ) );

    for( int joint = lastJoint; joint >= firstJoint; joint-- )
    {
        ID_STATIC_ASSERT( parents[joint] < joint );
        const int parent = parents[joint];
        const float* __restrict parentMatrix = jointMats->ToFloatPtr() + ( parent + parent + parent ) * 4;
        float* __restrict childMatrix = jointMats->ToFloatPtr() + ( joint + joint + joint ) * 4;

        __m128 pma = _mm_load_ps( parentMatrix + 0 );
        __m128 pmb = _mm_load_ps( parentMatrix + 4 );
        __m128 pmc = _mm_load_ps( parentMatrix + 8 );

        __m128 cma = _mm_load_ps( childMatrix + 0 );
        __m128 cmb = _mm_load_ps( childMatrix + 4 );
        __m128 cmc = _mm_load_ps( childMatrix + 8 );

        __m128 ta = _mm_splat_ps( pma, 0 );
        __m128 tb = _mm_splat_ps( pma, 1 );
        __m128 tc = _mm_splat_ps( pma, 2 );

        __m128 td = _mm_splat_ps( pmb, 0 );
        __m128 te = _mm_splat_ps( pmb, 1 );
        __m128 tf = _mm_splat_ps( pmb, 2 );

        __m128 tg = _mm_splat_ps( pmc, 0 );
        __m128 th = _mm_splat_ps( pmc, 1 );
        __m128 ti = _mm_splat_ps( pmc, 2 );

        cma = _mm_sub_ps( cma, _mm_and_ps( pma, vector_float_mask_keep_last ) );
        cmb = _mm_sub_ps( cmb, _mm_and_ps( pmb, vector_float_mask_keep_last ) );
        cmc = _mm_sub_ps( cmc, _mm_and_ps( pmc, vector_float_mask_keep_last ) );

        pma = _mm_mul_ps( ta, cma );
        pmb = _mm_mul_ps( tb, cma );
        pmc = _mm_mul_ps( tc, cma );

        pma = _mm_madd_ps( td, cmb, pma );
        pmb = _mm_madd_ps( te, cmb, pmb );
        pmc = _mm_madd_ps( tf, cmb, pmc );

        pma = _mm_madd_ps( tg, cmc, pma );
        pmb = _mm_madd_ps( th, cmc, pmb );
        pmc = _mm_madd_ps( ti, cmc, pmc );

        _mm_store_ps( childMatrix + 0, pma );
        _mm_store_ps( childMatrix + 4, pmb );
        _mm_store_ps( childMatrix + 8, pmc );
    }
}

// The Dark Mod
#define OFFSETOF(s, m) offsetof(s, m)
#define SHUF(i0, i1, i2, i3) _MM_SHUFFLE(i3, i2, i1, i0)

#define DOT_PRODUCT(xyz, a, b) \
	__m128 xyz = _mm_mul_ps(a, b); \
	{ \
		__m128 yzx = _mm_shuffle_ps(xyz, xyz, SHUF(1, 2, 0, 3)); \
		__m128 zxy = _mm_shuffle_ps(xyz, xyz, SHUF(2, 0, 1, 3)); \
		xyz = _mm_add_ps(_mm_add_ps(xyz, yzx), zxy); \
	}
#define CROSS_PRODUCT(dst, a, b) \
	__m128 dst = _mm_mul_ps(a, _mm_shuffle_ps(b, b, SHUF(1, 2, 0, 3))); \
	dst = _mm_sub_ps(dst, _mm_mul_ps(b, _mm_shuffle_ps(a, a, SHUF(1, 2, 0, 3)))); \
	dst = _mm_shuffle_ps(dst, dst, SHUF(1, 2, 0, 3));


void idSIMD_SSE2::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int count ) {
	//idMD5Mesh::CalcBounds calls this with uninitialized texcoords
	//we have to mask any exceptions here
	//idIgnoreFpExceptions guardFpExceptions;

	__m128 rmin = _mm_set1_ps( 1e30f);
	__m128 rmax = _mm_set1_ps(-1e30f);
	int i = 0;
	for (; i < (count & (~3)); i += 4) {
		__m128 pos0 = _mm_loadu_ps(&src[i    ].xyz.x);
		__m128 pos1 = _mm_loadu_ps(&src[i + 1].xyz.x);
		__m128 pos2 = _mm_loadu_ps(&src[i + 2].xyz.x);
		__m128 pos3 = _mm_loadu_ps(&src[i + 3].xyz.x);
		__m128 min01 = _mm_min_ps(pos0, pos1);
		__m128 max01 = _mm_max_ps(pos0, pos1);
		__m128 min23 = _mm_min_ps(pos2, pos3);
		__m128 max23 = _mm_max_ps(pos2, pos3);
		__m128 minA = _mm_min_ps(min01, min23);
		__m128 maxA = _mm_max_ps(max01, max23);
		rmin = _mm_min_ps(rmin, minA);
		rmax = _mm_max_ps(rmax, maxA);
	}
	if (i + 0 < count) {
		__m128 pos = _mm_loadu_ps(&src[i    ].xyz.x);
		rmin = _mm_min_ps(rmin, pos);
		rmax = _mm_max_ps(rmax, pos);
		if (i + 1 < count) {
			pos = _mm_loadu_ps(&src[i + 1].xyz.x);
			rmin = _mm_min_ps(rmin, pos);
			rmax = _mm_max_ps(rmax, pos);
			if (i + 2 < count) {
				pos = _mm_loadu_ps(&src[i + 2].xyz.x);
				rmin = _mm_min_ps(rmin, pos);
				rmax = _mm_max_ps(rmax, pos);
			}
		}
	}
	_mm_store_sd((double*)&min.x, _mm_castps_pd(rmin));
	_mm_store_ss(&min.z, _mm_movehl_ps(rmin, rmin));
	_mm_store_sd((double*)&max.x, _mm_castps_pd(rmax));
	_mm_store_ss(&max.z, _mm_movehl_ps(rmax, rmax));
}

#if 0
//somewhat slower than ID's code (28.6K vs 21.4K)
void idSIMD_SSE2::NormalizeTangents( idDrawVert *verts, const int numVerts ) {
	//in all vector normalizations, W component is can be zero (division by zero)
	//we have to mask any exceptions here
	//idIgnoreFpExceptions guardFpExceptions;

	for (int i = 0; i < numVerts; i++) {
		idDrawVert &vertex = verts[i];
		__m128 normal = _mm_loadu_ps(&vertex.normal.x);
		__m128 tangU  = _mm_loadu_ps(&vertex.tangents[0].x);
		__m128 tangV  = _mm_loadu_ps(&vertex.tangents[1].x);

		DOT_PRODUCT(dotNormal, normal, normal)
		normal = _mm_mul_ps(normal, _mm_rsqrt_ps(dotNormal));

		DOT_PRODUCT(dotTangU, tangU, normal)
		tangU = _mm_sub_ps(tangU, _mm_mul_ps(normal, dotTangU));
		DOT_PRODUCT(dotTangV, tangV, normal)
		tangV = _mm_sub_ps(tangV, _mm_mul_ps(normal, dotTangV));

		DOT_PRODUCT(sqlenTangU, tangU, tangU);
		tangU = _mm_mul_ps(tangU, _mm_rsqrt_ps(sqlenTangU));
		DOT_PRODUCT(sqlenTangV, tangV, tangV);
		tangV = _mm_mul_ps(tangV, _mm_rsqrt_ps(sqlenTangV));

		ID_STATIC_ASSERT2(OFFSETOF(idDrawVert, normal) + sizeof(vertex.normal) == OFFSETOF(idDrawVert, tangents), "Bad members offsets");
		ID_STATIC_ASSERT2(sizeof(vertex.tangents) == 24, "Bad members offsets");
		//note: we do overlapping stores here (protected by static asserts)
		_mm_storeu_ps(&verts[i].normal.x, normal);
		_mm_storeu_ps(&verts[i].tangents[0].x, tangU);
		//last store is tricky (must not overwrite)
		_mm_store_sd((double*)&verts[i].tangents[1].x, _mm_castps_pd(tangV));
		_mm_store_ss(&verts[i].tangents[1].z, _mm_movehl_ps(tangV, tangV));
	}
}


//note: this version is slower than ID's original code in testSIMD benchmark (10K vs 5K)
//however, the ID's code contains branches, which are highly predictable in the benchmark (and not so predictable in real workload)
//this version is branchless: it does not suffer from branch mispredictions, so it won't slow down even on long random inputs
void idSIMD_SSE2::TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *weights, const int *index, int numWeights ) {
	const byte *jointsPtr = (byte *)joints;

	int i = 0;
	__m128 sum = _mm_setzero_ps();

	for (int j = 0; j < numWeights; j++) {
		int offset = index[j*2], isLast = index[j*2+1];
		const idJointMat &matrix = *(idJointMat *) (jointsPtr + offset);
		const idVec4 &weight = weights[j];

		__m128 wgt = _mm_load_ps(&weight.x);
		__m128 mulX = _mm_mul_ps(_mm_load_ps(matrix.ToFloatPtr() + 0), wgt);		//x0as
		__m128 mulY = _mm_mul_ps(_mm_load_ps(matrix.ToFloatPtr() + 4), wgt);		//y1bt
		__m128 mulZ = _mm_mul_ps(_mm_load_ps(matrix.ToFloatPtr() + 8), wgt);		//z2cr

		//transpose 3 x 4 matrix
		__m128 xy01 = _mm_unpacklo_ps(mulX, mulY);
		__m128 abst = _mm_unpackhi_ps(mulX, mulY);
		__m128 Vxyz = _mm_shuffle_ps(xy01, mulZ, _MM_SHUFFLE(0, 0, 1, 0));
		__m128 V012 = _mm_shuffle_ps(xy01, mulZ, _MM_SHUFFLE(1, 1, 3, 2));
		__m128 Vabc = _mm_shuffle_ps(abst, mulZ, _MM_SHUFFLE(2, 2, 1, 0));
		__m128 Vstr = _mm_shuffle_ps(abst, mulZ, _MM_SHUFFLE(3, 3, 3, 2));

		__m128 res = _mm_add_ps(_mm_add_ps(Vxyz, V012), _mm_add_ps(Vabc, Vstr));
		sum = _mm_add_ps(sum, res);

		//note: branchless version here
		//current sum is always stored to memory, but pointer does not always move
		_mm_store_sd((double*)&verts[i].xyz.x, _mm_castps_pd(sum));
		_mm_store_ss(&verts[i].xyz.z, _mm_movehl_ps(sum, sum));
		i += isLast;
		//zero current sum if last
		__m128 mask = _mm_castsi128_ps(_mm_cvtsi32_si128(isLast - 1));	//empty mask <=> last
		mask = _mm_shuffle_ps(mask, mask, _MM_SHUFFLE(0, 0, 0, 0));
		sum = _mm_and_ps(sum, mask);
	}
}
#endif

//this thing is significantly faster that ID's original code (50K vs 87K)
//one major difference is: this version zeroes all resulting vectors in preprocessing step
//this allows to write completely branchless code
//ID' original version has many flaws, namely:
//  1. branches for singular cases
//  2. storing data is done with scalar C++ code
//  3. branches for detecting: store or add
void idSIMD_SSE2::DeriveTangents( idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) {
	int numTris = numIndexes / 3;
	const __m128 vector_float_sign_bit = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
#define NORMAL_EPS 1e-10f

	//note that idDrawVerts must have normal & tangents tightly packed, going on after the other
	ID_STATIC_ASSERT2(OFFSETOF(idDrawVert, normal) + sizeof(verts->normal) == OFFSETOF(idDrawVert, tangents), "Bad members offsets");
	ID_STATIC_ASSERT2(sizeof(verts->tangents) == 24, "Bad members offsets");
	for (int i = 0; i < numVerts; i++) {
		float *ptr = &verts[i].normal.x;
		_mm_storeu_ps(ptr, _mm_setzero_ps());
		_mm_storeu_ps(ptr + 4, _mm_setzero_ps());
		_mm_store_ss(ptr + 8, _mm_setzero_ps());
	}

	for (int i = 0; i < numTris; i++) {
		idDrawVert &vA = verts[indexes[3 * i + 0]];
		idDrawVert &vB = verts[indexes[3 * i + 1]];
		idDrawVert &vC = verts[indexes[3 * i + 2]];

		__m128 posA = _mm_loadu_ps(&vA.xyz.x);		//xyzs A
		__m128 posB = _mm_loadu_ps(&vB.xyz.x);		//xyzs B
		__m128 posC = _mm_loadu_ps(&vC.xyz.x);		//xyzs C
		__m128 tA = _mm_load_ss(&vA.st.y);				//t    A
		__m128 tB = _mm_load_ss(&vB.st.y);				//t    B
		__m128 tC = _mm_load_ss(&vC.st.y);				//t    C

		//compute AB/AC differences
		__m128 dpAB = _mm_sub_ps(posB, posA);			//xyzs AB
		__m128 dpAC = _mm_sub_ps(posC, posA);			//xyzs AC
		__m128 dtAB = _mm_sub_ps(tB, tA);					//tttt AB
		__m128 dtAC = _mm_sub_ps(tC, tA);					//tttt AC
		dtAB = _mm_shuffle_ps(dtAB, dtAB, SHUF(0, 0, 0, 0));
		dtAC = _mm_shuffle_ps(dtAC, dtAC, SHUF(0, 0, 0, 0));

		//compute normal unit vector
		CROSS_PRODUCT(normal, dpAC, dpAB)
		DOT_PRODUCT(normalSqr, normal, normal);
		normalSqr = _mm_max_ps(normalSqr, _mm_set1_ps(NORMAL_EPS));
		normalSqr = _mm_rsqrt_ps(normalSqr);
		normal = _mm_mul_ps(normal, normalSqr);

		//fit plane though point A
		DOT_PRODUCT(planeShift, posA, normal);
		planeShift = _mm_xor_ps(planeShift, vector_float_sign_bit);
		//save plane equation
		ID_STATIC_ASSERT2(sizeof(idPlane) == 16, "Wrong idPlane size");
		_mm_storeu_ps(planes[i].ToFloatPtr(), normal);
		_mm_store_ss(planes[i].ToFloatPtr() + 3, planeShift);

		//check area sign
		__m128 area = _mm_sub_ps(_mm_mul_ps(dpAB, dtAC), _mm_mul_ps(dpAC, dtAB));
		area = _mm_shuffle_ps(area, area, SHUF(3, 3, 3, 3));
		__m128 sign = _mm_and_ps(area, vector_float_sign_bit);

		//compute first tangent
		__m128 tangentU = _mm_sub_ps(
			_mm_mul_ps(dpAB, dtAC),
			_mm_mul_ps(dpAC, dtAB)
		);
		DOT_PRODUCT(tangUlen, tangentU, tangentU)
		tangUlen = _mm_max_ps(tangUlen, _mm_set1_ps(NORMAL_EPS));
		tangUlen = _mm_rsqrt_ps(tangUlen);
		tangUlen = _mm_xor_ps(tangUlen, sign);
		tangentU = _mm_mul_ps(tangentU, tangUlen);

		//compute second tangent
		__m128 tangentV = _mm_sub_ps(
			_mm_mul_ps(dpAC, _mm_shuffle_ps(dpAB, dpAB, SHUF(3, 3, 3, 3))),
			_mm_mul_ps(dpAB, _mm_shuffle_ps(dpAC, dpAC, SHUF(3, 3, 3, 3)))
		);
		DOT_PRODUCT(tangVlen, tangentV, tangentV)
		tangVlen = _mm_max_ps(tangVlen, _mm_set1_ps(NORMAL_EPS));
		tangVlen = _mm_rsqrt_ps(tangVlen);
		tangVlen = _mm_xor_ps(tangVlen, sign);
		tangentV = _mm_mul_ps(tangentV, tangVlen);

		//pack normal and tangents into 9 values
		__m128 pack0 = _mm_xor_ps(normal, _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(tangentU), 12)));
		__m128 pack1 = _mm_shuffle_ps(tangentU, tangentV, SHUF(1, 2, 0, 1));
		__m128 pack2 = _mm_movehl_ps(tangentV, tangentV);

		//add computed normal and tangents to endpoints' data (for averaging)
		#define ADDV(dst, src) _mm_storeu_ps(dst, _mm_add_ps(_mm_loadu_ps(dst), src));
		#define ADDS(dst, src) _mm_store_ss (dst, _mm_add_ss(_mm_load_ss (dst), src));
		ADDV(&vA.normal.x, pack0);
		ADDV(&vA.tangents[0].y, pack1);
		ADDS(&vA.tangents[1].z, pack2);
		ADDV(&vB.normal.x, pack0);
		ADDV(&vB.tangents[0].y, pack1);
		ADDS(&vB.tangents[1].z, pack2);
		ADDV(&vC.normal.x, pack0);
		ADDV(&vC.tangents[0].y, pack1);
		ADDS(&vC.tangents[1].z, pack2);
		#undef ADDV
		#undef ADDS
	}
}

int idSIMD_SSE2::CreateVertexProgramShadowCache( idVec4 *shadowVerts, const idDrawVert *verts, const int numVerts ) {
	const __m128 xyzMask = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));
	const __m128 oneW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0);
	for ( int i = 0; i < numVerts; i++ ) {
		const float *v = verts[i].xyz.ToFloatPtr();
		__m128 vec = _mm_loadu_ps(v);
		vec = _mm_and_ps(vec, xyzMask);
		_mm_storeu_ps(&shadowVerts[i*2+0].x, _mm_xor_ps(vec, oneW));
		_mm_storeu_ps(&shadowVerts[i*2+1].x, vec);
	}
	return numVerts * 2;
}

void idSIMD_SSE2::TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts ) {
	__m128 pA = _mm_loadu_ps(planes[0].ToFloatPtr());
	__m128 pB = _mm_loadu_ps(planes[1].ToFloatPtr());
	__m128 pC = _mm_loadu_ps(planes[2].ToFloatPtr());
	__m128 pD = _mm_loadu_ps(planes[3].ToFloatPtr());
	_MM_TRANSPOSE4_PS(pA, pB, pC, pD);

	const __m128 radP = _mm_set1_ps( radius);
	const __m128 radM = _mm_set1_ps(-radius);

	size_t orAll = 0;
	for ( int i = 0; i < numVerts; i++ ) {
		__m128 xyzs = _mm_loadu_ps(&verts[i].xyz.x);
		__m128 vX = _mm_shuffle_ps(xyzs, xyzs, SHUF(0, 0, 0, 0));
		__m128 vY = _mm_shuffle_ps(xyzs, xyzs, SHUF(1, 1, 1, 1));
		__m128 vZ = _mm_shuffle_ps(xyzs, xyzs, SHUF(2, 2, 2, 2));
		__m128 dist = _mm_add_ps(
			_mm_add_ps(_mm_mul_ps(pA, vX), _mm_mul_ps(pB, vY)), 
			_mm_add_ps(_mm_mul_ps(pC, vZ), pD)
		);
		size_t lower = _mm_movemask_ps(_mm_cmpgt_ps(dist, radM));
		size_t upper = _mm_movemask_ps(_mm_cmplt_ps(dist, radP));
		size_t mask = lower + (upper << 4);
		cullBits[i] = mask;
		orAll |= mask;
	}

	totalOr = orAll;
}

#elif defined(_MSC_VER) && defined(_M_IX86)

#include <xmmintrin.h>

#define SHUFFLEPS( x, y, z, w )		(( (x) & 3 ) << 6 | ( (y) & 3 ) << 4 | ( (z) & 3 ) << 2 | ( (w) & 3 ))
#define R_SHUFFLEPS( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))
#define SHUFFLEPD( x, y )			(( (x) & 1 ) << 1 | ( (y) & 1 ))
#define R_SHUFFLEPD( x, y )			(( (y) & 1 ) << 1 | ( (x) & 1 ))


#define ALIGN4_INIT1( X, INIT )				ALIGN16( static X[4] ) = { INIT, INIT, INIT, INIT }
#define ALIGN4_INIT4( X, I0, I1, I2, I3 )	ALIGN16( static X[4] ) = { I0, I1, I2, I3 }
#define ALIGN8_INIT1( X, INIT )				ALIGN16( static X[8] ) = { INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT }

ALIGN8_INIT1(unsigned short SIMD_W_zero, 0);
ALIGN8_INIT1(unsigned short SIMD_W_maxShort, 1<<15);

ALIGN4_INIT4(unsigned int SIMD_SP_singleSignBitMask, (unsigned int)(1 << 31), 0, 0, 0);
ALIGN4_INIT1(unsigned int SIMD_SP_signBitMask, (unsigned int)(1 << 31));
ALIGN4_INIT1(unsigned int SIMD_SP_absMask, (unsigned int) ~(1 << 31));
ALIGN4_INIT1(unsigned int SIMD_SP_infinityMask, (unsigned int) ~(1 << 23));

ALIGN4_INIT1(float SIMD_SP_zero, 0.0f);
ALIGN4_INIT1(float SIMD_SP_one, 1.0f);
ALIGN4_INIT1(float SIMD_SP_two, 2.0f);
ALIGN4_INIT1(float SIMD_SP_three, 3.0f);
ALIGN4_INIT1(float SIMD_SP_four, 4.0f);
ALIGN4_INIT1(float SIMD_SP_maxShort, (1<<15));
ALIGN4_INIT1(float SIMD_SP_tiny, 1e-10f);
ALIGN4_INIT1(float SIMD_SP_PI, idMath::PI);
ALIGN4_INIT1(float SIMD_SP_halfPI, idMath::HALF_PI);
ALIGN4_INIT1(float SIMD_SP_twoPI, idMath::TWO_PI);
ALIGN4_INIT1(float SIMD_SP_oneOverTwoPI, 1.0f / idMath::TWO_PI);
ALIGN4_INIT1(float SIMD_SP_infinity, idMath::INFINITY);


/*
============
idSIMD_SSE2::GetName
============
*/
const char *idSIMD_SSE2::GetName(void) const
{
	return "MMX & SSE & SSE2";
}

#if 0		// the SSE2 code is ungodly slow

/*
============
idSIMD_SSE2::MatX_LowerTriangularSolve

  solves x in Lx = b for the n * n sub-matrix of L
  if skip > 0 the first skip elements of x are assumed to be valid already
  L has to be a lower triangular matrix with (implicit) ones on the diagonal
  x == b is allowed
============
*/
void VPCALL idSIMD_SSE2::MatX_LowerTriangularSolve(const idMatX &L, float *x, const float *b, const int n, int skip)
{
	int nc;
	const float *lptr;

	if (skip >= n) {
		return;
	}

	lptr = L[skip];
	nc = L.GetNumColumns();

	// unrolled cases for n < 8
	if (n < 8) {
#define NSKIP( n, s )	((n<<3)|(s&7))

		switch (NSKIP(n, skip)) {
			case NSKIP(1, 0):
				x[0] = b[0];
				return;
			case NSKIP(2, 0):
				x[0] = b[0];
			case NSKIP(2, 1):
				x[1] = b[1] - lptr[1*nc+0] * x[0];
				return;
			case NSKIP(3, 0):
				x[0] = b[0];
			case NSKIP(3, 1):
				x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP(3, 2):
				x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
				return;
			case NSKIP(4, 0):
				x[0] = b[0];
			case NSKIP(4, 1):
				x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP(4, 2):
				x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP(4, 3):
				x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
				return;
			case NSKIP(5, 0):
				x[0] = b[0];
			case NSKIP(5, 1):
				x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP(5, 2):
				x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP(5, 3):
				x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP(5, 4):
				x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
				return;
			case NSKIP(6, 0):
				x[0] = b[0];
			case NSKIP(6, 1):
				x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP(6, 2):
				x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP(6, 3):
				x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP(6, 4):
				x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
			case NSKIP(6, 5):
				x[5] = b[5] - lptr[5*nc+0] * x[0] - lptr[5*nc+1] * x[1] - lptr[5*nc+2] * x[2] - lptr[5*nc+3] * x[3] - lptr[5*nc+4] * x[4];
				return;
			case NSKIP(7, 0):
				x[0] = b[0];
			case NSKIP(7, 1):
				x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP(7, 2):
				x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP(7, 3):
				x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP(7, 4):
				x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
			case NSKIP(7, 5):
				x[5] = b[5] - lptr[5*nc+0] * x[0] - lptr[5*nc+1] * x[1] - lptr[5*nc+2] * x[2] - lptr[5*nc+3] * x[3] - lptr[5*nc+4] * x[4];
			case NSKIP(7, 6):
				x[6] = b[6] - lptr[6*nc+0] * x[0] - lptr[6*nc+1] * x[1] - lptr[6*nc+2] * x[2] - lptr[6*nc+3] * x[3] - lptr[6*nc+4] * x[4] - lptr[6*nc+5] * x[5];
				return;
		}

		return;
	}

	// process first 4 rows
	switch (skip) {
		case 0:
			x[0] = b[0];
		case 1:
			x[1] = b[1] - lptr[1*nc+0] * x[0];
		case 2:
			x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
		case 3:
			x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			skip = 4;
	}

	lptr = L[skip];

	__asm {
		push		ebx
		mov			eax, skip				// eax = i
		shl			eax, 2					// eax = i*4
		mov			edx, n					// edx = n
		shl			edx, 2					// edx = n*4
		mov			esi, x					// esi = x
		mov			edi, lptr				// edi = lptr
		add			esi, eax
		add			edi, eax
		mov			ebx, b					// ebx = b
		// aligned
		looprow:
		mov			ecx, eax
		neg			ecx
		cvtps2pd	xmm0, [esi+ecx]
		cvtps2pd	xmm2, [edi+ecx]
		mulpd		xmm0, xmm2
		cvtps2pd	xmm1, [esi+ecx+8]
		cvtps2pd	xmm3, [edi+ecx+8]
		mulpd		xmm1, xmm3
		add			ecx, 20*4
		jg			donedot16
		dot16:
		cvtps2pd	xmm2, [esi+ecx-(16*4)]
		cvtps2pd	xmm3, [edi+ecx-(16*4)]
		cvtps2pd	xmm4, [esi+ecx-(14*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(14*4)]
		addpd		xmm0, xmm2
		cvtps2pd	xmm2, [esi+ecx-(12*4)]
		mulpd		xmm4, xmm5
		cvtps2pd	xmm3, [edi+ecx-(12*4)]
		addpd		xmm1, xmm4
		cvtps2pd	xmm4, [esi+ecx-(10*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(10*4)]
		addpd		xmm0, xmm2
		cvtps2pd	xmm2, [esi+ecx-(8*4)]
		mulpd		xmm4, xmm5
		cvtps2pd	xmm3, [edi+ecx-(8*4)]
		addpd		xmm1, xmm4
		cvtps2pd	xmm4, [esi+ecx-(6*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(6*4)]
		addpd		xmm0, xmm2
		cvtps2pd	xmm2, [esi+ecx-(4*4)]
		mulpd		xmm4, xmm5
		cvtps2pd	xmm3, [edi+ecx-(4*4)]
		addpd		xmm1, xmm4
		cvtps2pd	xmm4, [esi+ecx-(2*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(2*4)]
		addpd		xmm0, xmm2
		add			ecx, 16*4
		mulpd		xmm4, xmm5
		addpd		xmm1, xmm4
		jle			dot16
		donedot16:
		sub			ecx, 8*4
		jg			donedot8
		dot8:
		cvtps2pd	xmm2, [esi+ecx-(8*4)]
		cvtps2pd	xmm3, [edi+ecx-(8*4)]
		cvtps2pd	xmm7, [esi+ecx-(6*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(6*4)]
		addpd		xmm0, xmm2
		cvtps2pd	xmm6, [esi+ecx-(4*4)]
		mulpd		xmm7, xmm5
		cvtps2pd	xmm3, [edi+ecx-(4*4)]
		addpd		xmm1, xmm7
		cvtps2pd	xmm4, [esi+ecx-(2*4)]
		mulpd		xmm6, xmm3
		cvtps2pd	xmm7, [edi+ecx-(2*4)]
		addpd		xmm0, xmm6
		add			ecx, 8*4
		mulpd		xmm4, xmm7
		addpd		xmm1, xmm4
		donedot8:
		sub			ecx, 4*4
		jg			donedot4
		dot4:
		cvtps2pd	xmm2, [esi+ecx-(4*4)]
		cvtps2pd	xmm3, [edi+ecx-(4*4)]
		cvtps2pd	xmm4, [esi+ecx-(2*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(2*4)]
		addpd		xmm0, xmm2
		add			ecx, 4*4
		mulpd		xmm4, xmm5
		addpd		xmm1, xmm4
		donedot4:
		addpd		xmm0, xmm1
		movaps		xmm1, xmm0
		shufpd		xmm1, xmm1, R_SHUFFLEPD(1, 0)
		addsd		xmm0, xmm1
		sub			ecx, 4*4
		jz			dot0
		add			ecx, 4
		jz			dot1
		add			ecx, 4
		jz			dot2
		//dot3:
		cvtss2sd	xmm1, [esi-(3*4)]
		cvtss2sd	xmm2, [edi-(3*4)]
		mulsd		xmm1, xmm2
		addsd		xmm0, xmm1
		dot2:
		cvtss2sd	xmm3, [esi-(2*4)]
		cvtss2sd	xmm4, [edi-(2*4)]
		mulsd		xmm3, xmm4
		addsd		xmm0, xmm3
		dot1:
		cvtss2sd	xmm5, [esi-(1*4)]
		cvtss2sd	xmm6, [edi-(1*4)]
		mulsd		xmm5, xmm6
		addsd		xmm0, xmm5
		dot0:
		cvtss2sd	xmm1, [ebx+eax]
		subsd		xmm1, xmm0
		cvtsd2ss	xmm0, xmm1
		movss		[esi], xmm0
		add			eax, 4
		cmp			eax, edx
		jge			done
		add			esi, 4
		mov			ecx, nc
		shl			ecx, 2
		add			edi, ecx
		add			edi, 4
		jmp			looprow
		// done
		done:
		pop			ebx
	}
}

/*
============
idSIMD_SSE2::MatX_LowerTriangularSolveTranspose

  solves x in L'x = b for the n * n sub-matrix of L
  L has to be a lower triangular matrix with (implicit) ones on the diagonal
  x == b is allowed
============
*/
void VPCALL idSIMD_SSE2::MatX_LowerTriangularSolveTranspose(const idMatX &L, float *x, const float *b, const int n)
{
	int nc;
	const float *lptr;

	lptr = L.ToFloatPtr();
	nc = L.GetNumColumns();

	// unrolled cases for n < 8
	if (n < 8) {
		switch (n) {
			case 0:
					return;
			case 1:
					x[0] = b[0];
				return;
			case 2:
					x[1] = b[1];
				x[0] = b[0] - lptr[1*nc+0] * x[1];
				return;
			case 3:
					x[2] = b[2];
				x[1] = b[1] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 4:
					x[3] = b[3];
				x[2] = b[2] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 5:
					x[4] = b[4];
				x[3] = b[3] - lptr[4*nc+3] * x[4];
				x[2] = b[2] - lptr[4*nc+2] * x[4] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[4*nc+1] * x[4] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[4*nc+0] * x[4] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 6:
					x[5] = b[5];
				x[4] = b[4] - lptr[5*nc+4] * x[5];
				x[3] = b[3] - lptr[5*nc+3] * x[5] - lptr[4*nc+3] * x[4];
				x[2] = b[2] - lptr[5*nc+2] * x[5] - lptr[4*nc+2] * x[4] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[5*nc+1] * x[5] - lptr[4*nc+1] * x[4] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[5*nc+0] * x[5] - lptr[4*nc+0] * x[4] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 7:
					x[6] = b[6];
				x[5] = b[5] - lptr[6*nc+5] * x[6];
				x[4] = b[4] - lptr[6*nc+4] * x[6] - lptr[5*nc+4] * x[5];
				x[3] = b[3] - lptr[6*nc+3] * x[6] - lptr[5*nc+3] * x[5] - lptr[4*nc+3] * x[4];
				x[2] = b[2] - lptr[6*nc+2] * x[6] - lptr[5*nc+2] * x[5] - lptr[4*nc+2] * x[4] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[6*nc+1] * x[6] - lptr[5*nc+1] * x[5] - lptr[4*nc+1] * x[4] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[6*nc+0] * x[6] - lptr[5*nc+0] * x[5] - lptr[4*nc+0] * x[4] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
		}

		return;
	}

	int i, j, m;
	float *xptr;
	double s0;

	// if the number of columns is not a multiple of 2 we're screwed for alignment.
	// however, if the number of columns is a multiple of 2 but the number of to be
	// processed rows is not a multiple of 2 we can still run 8 byte aligned
	m = n;

	if (m & 1) {
		m--;
		x[m] = b[m];

		lptr = L[m] + m - 4;
		xptr = x + m;
		__asm {
			push		ebx
			mov			eax, m					// eax = i
			mov			esi, xptr				// esi = xptr
			mov			edi, lptr				// edi = lptr
			mov			ebx, b					// ebx = b
			mov			edx, nc					// edx = nc*sizeof(float)
			shl			edx, 2
			process4rows_1:
			cvtps2pd	xmm0, [ebx+eax*4-16]	// load b[i-2], b[i-1]
			cvtps2pd	xmm2, [ebx+eax*4-8]		// load b[i-4], b[i-3]
			xor			ecx, ecx
			sub			eax, m
			neg			eax
			jz			done4x4_1
			process4x4_1:	// process 4x4 blocks
			cvtps2pd	xmm3, [edi]
			cvtps2pd	xmm4, [edi+8]
			add			edi, edx
			cvtss2sd	xmm5, [esi+4*ecx+0]
			shufpd		xmm5, xmm5, R_SHUFFLEPD(0, 0)
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+4]
			shufpd		xmm7, xmm7, R_SHUFFLEPD(0, 0)
			mulpd		xmm1, xmm7
			cvtps2pd	xmm3, [edi]
			mulpd		xmm6, xmm7
			cvtps2pd	xmm4, [edi+8]
			subpd		xmm0, xmm1
			subpd		xmm2, xmm6
			add			edi, edx
			cvtss2sd	xmm5, [esi+4*ecx+8]
			shufpd		xmm5, xmm5, R_SHUFFLEPD(0, 0)
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+12]
			shufpd		xmm7, xmm7, R_SHUFFLEPD(0, 0)
			mulpd		xmm1, xmm7
			add			ecx, 4
			mulpd		xmm6, xmm7
			cmp			ecx, eax
			subpd		xmm0, xmm1
			subpd		xmm2, xmm6
			jl			process4x4_1
			done4x4_1:		// process left over of the 4 rows
			cvtps2pd	xmm3, [edi]
			cvtps2pd	xmm4, [edi+8]
			cvtss2sd	xmm5, [esi+4*ecx]
			shufpd		xmm5, xmm5, R_SHUFFLEPD(0, 0)
			mulpd		xmm3, xmm5
			mulpd		xmm4, xmm5
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			imul		ecx, edx
			sub			edi, ecx
			neg			eax

			add			eax, m
			sub			eax, 4
			movapd		xmm1, xmm0
			shufpd		xmm1, xmm1, R_SHUFFLEPD(1, 1)
			movapd		xmm3, xmm2
			shufpd		xmm3, xmm3, R_SHUFFLEPD(1, 1)
			sub			edi, edx
			cvtsd2ss	xmm7, xmm3
			movss		[esi-4], xmm7			// xptr[-1] = s3
			movsd		xmm4, xmm3
			movsd		xmm5, xmm3
			cvtss2sd	xmm7, [edi+8]
			mulsd		xmm3, xmm7				// lptr[-1*nc+2] * s3
			cvtss2sd	xmm7, [edi+4]
			mulsd		xmm4, xmm7				// lptr[-1*nc+1] * s3
			cvtss2sd	xmm7, [edi]
			mulsd		xmm5, xmm7				// lptr[-1*nc+0] * s3
			subsd		xmm2, xmm3
			cvtsd2ss	xmm7, xmm2
			movss		[esi-8], xmm7			// xptr[-2] = s2
			movsd		xmm6, xmm2
			sub			edi, edx
			subsd		xmm0, xmm5
			subsd		xmm1, xmm4
			cvtss2sd	xmm7, [edi+4]
			mulsd		xmm2, xmm7				// lptr[-2*nc+1] * s2
			cvtss2sd	xmm7, [edi]
			mulsd		xmm6, xmm7				// lptr[-2*nc+0] * s2
			subsd		xmm1, xmm2
			cvtsd2ss	xmm7, xmm1
			movss		[esi-12], xmm7			// xptr[-3] = s1
			subsd		xmm0, xmm6
			sub			edi, edx
			cmp			eax, 4
			cvtss2sd	xmm7, [edi]
			mulsd		xmm1, xmm7				// lptr[-3*nc+0] * s1
			subsd		xmm0, xmm1
			cvtsd2ss	xmm7, xmm0
			movss		[esi-16], xmm7			// xptr[-4] = s0
			jl			done4rows_1
			sub			edi, edx
			sub			edi, 16
			sub			esi, 16
			jmp			process4rows_1
			done4rows_1:
			pop			ebx
		}
	} else {
		lptr = L.ToFloatPtr() + m * L.GetNumColumns() + m - 4;
		xptr = x + m;
		__asm {
			push		ebx
			mov			eax, m					// eax = i
			mov			esi, xptr				// esi = xptr
			mov			edi, lptr				// edi = lptr
			mov			ebx, b					// ebx = b
			mov			edx, nc					// edx = nc*sizeof(float)
			shl			edx, 2
			process4rows:
			cvtps2pd	xmm0, [ebx+eax*4-16]	// load b[i-2], b[i-1]
			cvtps2pd	xmm2, [ebx+eax*4-8]		// load b[i-4], b[i-3]
			sub			eax, m
			jz			done4x4
			neg			eax
			xor			ecx, ecx
			process4x4:		// process 4x4 blocks
			cvtps2pd	xmm3, [edi]
			cvtps2pd	xmm4, [edi+8]
			add			edi, edx
			cvtss2sd	xmm5, [esi+4*ecx+0]
			shufpd		xmm5, xmm5, R_SHUFFLEPD(0, 0)
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+4]
			shufpd		xmm7, xmm7, R_SHUFFLEPD(0, 0)
			mulpd		xmm1, xmm7
			cvtps2pd	xmm3, [edi]
			mulpd		xmm6, xmm7
			cvtps2pd	xmm4, [edi+8]
			subpd		xmm0, xmm1
			subpd		xmm2, xmm6
			add			edi, edx
			cvtss2sd	xmm5, [esi+4*ecx+8]
			shufpd		xmm5, xmm5, R_SHUFFLEPD(0, 0)
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+12]
			shufpd		xmm7, xmm7, R_SHUFFLEPD(0, 0)
			mulpd		xmm1, xmm7
			add			ecx, 4
			mulpd		xmm6, xmm7
			cmp			ecx, eax
			subpd		xmm0, xmm1
			subpd		xmm2, xmm6
			jl			process4x4
			imul		ecx, edx
			sub			edi, ecx
			neg			eax
			done4x4:		// process left over of the 4 rows
			add			eax, m
			sub			eax, 4
			movapd		xmm1, xmm0
			shufpd		xmm1, xmm1, R_SHUFFLEPD(1, 1)
			movapd		xmm3, xmm2
			shufpd		xmm3, xmm3, R_SHUFFLEPD(1, 1)
			sub			edi, edx
			cvtsd2ss	xmm7, xmm3
			movss		[esi-4], xmm7			// xptr[-1] = s3
			movsd		xmm4, xmm3
			movsd		xmm5, xmm3
			cvtss2sd	xmm7, [edi+8]
			mulsd		xmm3, xmm7				// lptr[-1*nc+2] * s3
			cvtss2sd	xmm7, [edi+4]
			mulsd		xmm4, xmm7				// lptr[-1*nc+1] * s3
			cvtss2sd	xmm7, [edi]
			mulsd		xmm5, xmm7				// lptr[-1*nc+0] * s3
			subsd		xmm2, xmm3
			cvtsd2ss	xmm7, xmm2
			movss		[esi-8], xmm7			// xptr[-2] = s2
			movsd		xmm6, xmm2
			sub			edi, edx
			subsd		xmm0, xmm5
			subsd		xmm1, xmm4
			cvtss2sd	xmm7, [edi+4]
			mulsd		xmm2, xmm7				// lptr[-2*nc+1] * s2
			cvtss2sd	xmm7, [edi]
			mulsd		xmm6, xmm7				// lptr[-2*nc+0] * s2
			subsd		xmm1, xmm2
			cvtsd2ss	xmm7, xmm1
			movss		[esi-12], xmm7			// xptr[-3] = s1
			subsd		xmm0, xmm6
			sub			edi, edx
			cmp			eax, 4
			cvtss2sd	xmm7, [edi]
			mulsd		xmm1, xmm7				// lptr[-3*nc+0] * s1
			subsd		xmm0, xmm1
			cvtsd2ss	xmm7, xmm0
			movss		[esi-16], xmm7			// xptr[-4] = s0
			jl			done4rows
			sub			edi, edx
			sub			edi, 16
			sub			esi, 16
			jmp			process4rows
			done4rows:
			pop			ebx
		}
	}

	// process left over rows
	for (i = (m&3)-1; i >= 0; i--) {
		s0 = b[i];
		lptr = L[i+1] + i;

		for (j = i + 1; j < m; j++) {
			s0 -= lptr[0] * x[j];
			lptr += nc;
		}

		x[i] = s0;
	}
}

#endif

/*
============
idSIMD_SSE2::MixedSoundToSamples
============
*/
void VPCALL idSIMD_SSE2::MixedSoundToSamples(short *samples, const float *mixBuffer, const int numSamples)
{

	assert((numSamples % MIXBUFFER_SAMPLES) == 0);

	__asm {

		mov			eax, numSamples
		mov			edi, mixBuffer
		mov			esi, samples
		shl			eax, 2
		add			edi, eax
		neg			eax

		loop16:

		movaps		xmm0, [edi+eax+0*16]
		movaps		xmm1, [edi+eax+1*16]
		movaps		xmm2, [edi+eax+2*16]
		movaps		xmm3, [edi+eax+3*16]

		add			esi, 4*4*2

		cvtps2dq	xmm4, xmm0
		cvtps2dq	xmm5, xmm1
		cvtps2dq	xmm6, xmm2
		cvtps2dq	xmm7, xmm3

		prefetchnta	[edi+eax+128]

		packssdw	xmm4, xmm5
		packssdw	xmm6, xmm7

		add			eax, 4*16

		movlps		[esi-4*4*2], xmm4		// FIXME: should not use movlps/movhps to move integer data
		movhps		[esi-3*4*2], xmm4
		movlps		[esi-2*4*2], xmm6
		movhps		[esi-1*4*2], xmm6

		jl			loop16
	}
}

#endif /* _WIN32 */
