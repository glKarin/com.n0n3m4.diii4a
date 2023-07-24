// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#include "Simd_Generic.h"
#include "Simd_Xenon.h"

//===============================================================
//
//	Xenon implementation of idSIMDProcessor
//
//===============================================================

#ifdef _XENON

#if !defined( SHUFFLE_D )
#define SHUFFLE_D( x, y, z, w )	(( (x) & 3 ) << 6 | ( (y) & 3 ) << 4 | ( (z) & 3 ) << 2 | ( (w) & 3 ))
#endif

static __vector4i vmxi_byte_zero				= { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static __vector4i vmxi_dword_not				= { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
static __vector4i vmxi_dword_perm_replacelast	= { 0x00010203, 0x04050607, 0x08090A0B, 0x10111213 };	// 00 01 02 10
static __vector4i vmxi_dword_perm_plane_x		= { 0x00010203, 0x04050607, 0x10111213, 0x14151617 };	// 00 01 10 11
static __vector4i vmxi_dword_perm_plane_y		= { 0x08090A0B, 0x0C0D0E0F, 0x18191A1B, 0x1C1D1E1F };	// 02 03 12 13
static __vector4i vmxi_dword_perm_matrix		= { 0x10111213, 0x10111213, 0x10111213, 0x0C0D0E0F };	// 10 10 10 03
static __vector4i vmxi_dword_perm_quat2mat1		= { 0x10111213, 0x04050607, 0x08090A0B, 0x0C0D0E0F };	// 10 01 02 03
static __vector4i vmxi_dword_perm_quat2mat2		= { 0x00010203, 0x04050607, 0x18191A1B, 0x1C1D1E1F };	// 00 01 12 13
static __vector4i vmxi_dword_perm_quat2mat3		= { 0x00010203, 0x04050607, 0x08090A0B, 0x10111213 };	// 00 01 02 10
static __vector4i vmxi_dword_perm_quat2mat4		= { 0x04050607, 0x00010203, 0x0C0D0E0F, 0x14151617 };	// 01 00 03 11
static __vector4i vmxi_dword_perm_quat2mat5		= { 0x08090A0B, 0x0C0D0E0F, 0x00010203, 0x18191A1B };	// 02 03 00 12
static __vector4i vmxi_dword_mask_clear_last	= { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
static __vector4i vmxi_dword_quat2mat_swizzle0	= { 0x0C080400, 0x0C080400, 0x0C080400, 0x0C080400 };	// C 8 4 0
static __vector4i vmxi_dword_quat2mat_swizzle1	= { 0x0004080C, 0x0004080C, 0x0004080C, 0x0004080C };	// 0 4 8 C
static __vector4i vmxi_dword_quat2mat_swizzle2	= { 0x04000C08, 0x04000C08, 0x04000C08, 0x04000C08 };	// 4 0 C 8
static __vector4i vmxi_dword_quat2mat_swizzle3	= { 0x080C0004, 0x080C0004, 0x080C0004, 0x080C0004 };	// 8 C 0 4
static __vector4i vmxi_dword_quat2mat_splat0	= { 0x00000000, 0x01010101, 0x02020202, 0x03030303 };	// 0 1 2 3
static __vector4i vmxi_dword_quat2mat_splat1	= { 0x04040404, 0x05050505, 0x06060606, 0x07070707 };	// 4 5 6 7
static __vector4i vmxi_dword_quat2mat_splat2	= { 0x08080808, 0x09090909, 0x0A0A0A0A, 0x0B0B0B0B };	// 8 9 A B
static __vector4i vmxi_dword_quat2mat_splat3	= { 0x0C0C0C0C, 0x0D0D0D0D, 0x0E0E0E0E, 0x0F0F0F0F };	// C D E F
static __vector4i vmxi_dword_quat2mat_or		= { 0x00010203, 0x00010203, 0x00010203, 0x00010203 };	// 0 1 2 3
static __vector4i vmxi_float_quat2mat_xor		= { 0x80000000, 0x00000000, 0x00000000, 0x00000000 };	// - + + +
static __vector4i vmxi_dword_overlay_mask0		= {    1 <<  0,    1 <<  1,    1 <<  8,    1 <<  9 };
static __vector4i vmxi_dword_overlay_mask1		= {    1 << 16,    1 << 17,    1 << 24,    1 << 25 };
static __vector4i vmxi_dword_overlay_mask2		= {    1 <<  2,    1 <<  3,    1 << 10,    1 << 11 };
static __vector4i vmxi_dword_overlay_mask3		= {    1 << 18,    1 << 19,    1 << 26,    1 << 27 };
static __vector4i vmxi_dword_overlay_xor		= { 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F };
static __vector4i vmxi_dword_overlay_perm		= { 0x03020100, 0x03020100, 0x03020100, 0x03020100 };
static __vector4i vmxi_dword_decal_mask0		= {    1 <<  0,    1 <<  1,    1 <<  2,    1 <<  3 };	//  0  1  2  3
static __vector4i vmxi_dword_decal_mask1		= {    1 <<  8,    1 <<  9,    1 << 10,    1 << 11 };	//  8  9 10 11
static __vector4i vmxi_dword_decal_mask2		= {    1 << 16,    1 << 17,    1 << 18,    1 << 19 };	// 16 17 18 19
static __vector4i vmxi_dword_decal_mask3		= {    1 << 24,    1 << 25,    1 << 26,    1 << 27 };	// 24 25 26 27
static __vector4i vmxi_dword_decal_mask4		= {    1 <<  4,    1 <<  5,    1 << 12,    1 << 13 };	//  4  5 12 13
static __vector4i vmxi_dword_decal_mask5		= {    1 << 20,    1 << 21,    1 << 28,    1 << 29 };	// 20 21 28 29
static __vector4i vmxi_dword_trace_mask0		= {    1 <<  0,    1 <<  1,    1 <<  2,    1 <<  3 };	//  0  1  2  3
static __vector4i vmxi_dword_trace_mask1		= {    1 <<  8,    1 <<  9,    1 << 10,    1 << 11 };	//  8  9 10 11
static __vector4i vmxi_dword_trace_mask2		= {    1 << 16,    1 << 17,    1 << 18,    1 << 19 };	// 16 17 18 19
static __vector4i vmxi_dword_trace_mask3		= {    1 << 24,    1 << 25,    1 << 26,    1 << 27 };	// 24 25 26 27
static __vector4i vmxi_dword_trace_mask4		= {    1 <<  4,    1 <<  5,    1 <<  6,    1 <<  7 };	//  4  5  6  7
static __vector4i vmxi_dword_trace_mask5		= {    1 << 12,    1 << 13,    1 << 14,    1 << 15 };	// 12 13 14 15
static __vector4i vmxi_dword_trace_mask6		= {    1 << 20,    1 << 21,    1 << 22,    1 << 23 };	// 20 21 22 23
static __vector4i vmxi_dword_trace_mask7		= {    1 << 28,    1 << 29,    1 << 30,    1 << 31 };	// 28 29 30 31
static __vector4i vmxi_dword_trace_xor			= { 0xF0F0F0F0, 0xF0F0F0F0, 0xF0F0F0F0, 0xF0F0F0F0 };
static __vector4i vmxi_dword_facing_mask0		= {    1 << 24,    1 << 24,    1 << 24,    1 << 24 };
static __vector4i vmxi_dword_facing_mask1		= {    1 << 16,    1 << 16,    1 << 16,    1 << 16 };
static __vector4i vmxi_dword_facing_mask2		= {    1 <<  8,    1 <<  8,    1 <<  8,    1 <<  8 };
static __vector4i vmxi_dword_facing_mask3		= {    1 <<  0,    1 <<  0,    1 <<  0,    1 <<  0 };
static __vector4i vmxi_float_sign_bit			= { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
static __vector4i vmxi_float_pos_infinity		= { 0x7149F2CA, 0x7149F2CA, 0x7149F2CA, 0x7149F2CA };
static __vector4i vmxi_float_neg_infinity		= { 0xF149F2CA, 0xF149F2CA, 0xF149F2CA, 0xF149F2CA };

static __vector4 vmx_float_zero					= {      0.0f,      0.0f,      0.0f,      0.0f };
static __vector4 vmx_float_one					= {      1.0f,      1.0f,      1.0f,      1.0f };
static __vector4 vmx_float_neg_one				= {     -1.0f,     -1.0f,     -1.0f,     -1.0f };
static __vector4 vmx_float_last_one				= {      0.0f,      0.0f,      0.0f,      1.0f };
static __vector4 vmx_float_tiny					= {    1e-10f,    1e-10f,    1e-10f,    1e-10f };
static __vector4 vmx_float_PI					= { M_PI*0.5f, M_PI*0.5f, M_PI*0.5f, M_PI*0.5f };

static __vector4 vmx_float_quat2mat_mad1		= { -1.0f,  1.0f, -1.0f, -1.0f };	// - + - -
static __vector4 vmx_float_quat2mat_mad2		= { -1.0f, -1.0f, -1.0f,  1.0f };	// - - - +
static __vector4 vmx_float_quat2mat_mad3		= { -1.0f, -1.0f,  1.0f, -1.0f };	// - - + -
static __vector4 vmx_float_quat2mat_add			= {  1.0f,  0.0f,  0.0f,  0.0f };	// 1 0 0 0

static __vector4 vmx_float_rsqrt_c0				= {  -3.0f,  -3.0f,  -3.0f,  -3.0f };
static __vector4 vmx_float_rsqrt_c1				= {  -0.5f,  -0.5f,  -0.5f,  -0.5f };
static __vector4 vmx_float_rsqrt_c2				= { -0.25f, -0.25f, -0.25f, -0.25f };

static __vector4 vmx_float_sin_c0				= { -2.39e-08f, -2.39e-08f, -2.39e-08f, -2.39e-08f };
static __vector4 vmx_float_sin_c1				= {  2.7526e-06f, 2.7526e-06f, 2.7526e-06f, 2.7526e-06f };
static __vector4 vmx_float_sin_c2				= { -1.98409e-04f, -1.98409e-04f, -1.98409e-04f, -1.98409e-04f };
static __vector4 vmx_float_sin_c3				= {  8.3333315e-03f, 8.3333315e-03f, 8.3333315e-03f, 8.3333315e-03f };
static __vector4 vmx_float_sin_c4				= { -1.666666664e-01f, -1.666666664e-01f, -1.666666664e-01f, -1.666666664e-01f };

static __vector4 vmx_float_atan_c0				= {  0.0028662257f,  0.0028662257f,  0.0028662257f,  0.0028662257f };
static __vector4 vmx_float_atan_c1				= { -0.0161657367f, -0.0161657367f, -0.0161657367f, -0.0161657367f };
static __vector4 vmx_float_atan_c2				= {  0.0429096138f,  0.0429096138f,  0.0429096138f,  0.0429096138f };
static __vector4 vmx_float_atan_c3				= { -0.0752896400f, -0.0752896400f, -0.0752896400f, -0.0752896400f };
static __vector4 vmx_float_atan_c4				= {  0.1065626393f,  0.1065626393f,  0.1065626393f,  0.1065626393f };
static __vector4 vmx_float_atan_c5				= { -0.1420889944f, -0.1420889944f, -0.1420889944f, -0.1420889944f };
static __vector4 vmx_float_atan_c6				= {  0.1999355085f,  0.1999355085f,  0.1999355085f,  0.1999355085f };
static __vector4 vmx_float_atan_c7				= { -0.3333314528f, -0.3333314528f, -0.3333314528f, -0.3333314528f };

#define vmx_byte_zero							__vspltisw( 0 )		//*(__vector4 *)&vmxi_byte_zero
#define vmx_dword_not							__vspltisw( -1 )	// *(__vector4 *)&vmxi_dword_not
#define vmx_dword_perm_replacelast				*(__vector4 *)&vmxi_dword_perm_replacelast
#define vmx_dword_perm_plane_x					*(__vector4 *)&vmxi_dword_perm_plane_x
#define vmx_dword_perm_plane_y					*(__vector4 *)&vmxi_dword_perm_plane_y
#define vmx_dword_perm_matrix					*(__vector4 *)&vmxi_dword_perm_matrix
#define vmx_dword_perm_quat2mat1				*(__vector4 *)&vmxi_dword_perm_quat2mat1
#define vmx_dword_perm_quat2mat2				*(__vector4 *)&vmxi_dword_perm_quat2mat2
#define vmx_dword_perm_quat2mat3				*(__vector4 *)&vmxi_dword_perm_quat2mat3
#define vmx_dword_perm_quat2mat4				*(__vector4 *)&vmxi_dword_perm_quat2mat4
#define vmx_dword_perm_quat2mat5				*(__vector4 *)&vmxi_dword_perm_quat2mat5
#define vmx_dword_mask_clear_last				*(__vector4 *)&vmxi_dword_mask_clear_last
#define vmx_dword_quat2mat_swizzle0				*(__vector4 *)&vmxi_dword_quat2mat_swizzle0
#define vmx_dword_quat2mat_swizzle1				*(__vector4 *)&vmxi_dword_quat2mat_swizzle1
#define vmx_dword_quat2mat_swizzle2				*(__vector4 *)&vmxi_dword_quat2mat_swizzle2
#define vmx_dword_quat2mat_swizzle3				*(__vector4 *)&vmxi_dword_quat2mat_swizzle3
#define vmx_dword_quat2mat_splat0				*(__vector4 *)&vmxi_dword_quat2mat_splat0
#define vmx_dword_quat2mat_splat1				*(__vector4 *)&vmxi_dword_quat2mat_splat1
#define vmx_dword_quat2mat_splat2				*(__vector4 *)&vmxi_dword_quat2mat_splat2
#define vmx_dword_quat2mat_splat3				*(__vector4 *)&vmxi_dword_quat2mat_splat3
#define vmx_dword_quat2mat_or					*(__vector4 *)&vmxi_dword_quat2mat_or
#define vmx_float_quat2mat_xor					*(__vector4 *)&vmxi_float_quat2mat_xor
#define vmx_dword_overlay_mask0					*(__vector4 *)&vmxi_dword_overlay_mask0
#define vmx_dword_overlay_mask1					*(__vector4 *)&vmxi_dword_overlay_mask1
#define vmx_dword_overlay_mask2					*(__vector4 *)&vmxi_dword_overlay_mask2
#define vmx_dword_overlay_mask3					*(__vector4 *)&vmxi_dword_overlay_mask3
#define vmx_dword_overlay_xor					*(__vector4 *)&vmxi_dword_overlay_xor
#define vmx_dword_overlay_perm					*(__vector4 *)&vmxi_dword_overlay_perm
#define vmx_dword_decal_mask0					*(__vector4 *)&vmxi_dword_decal_mask0
#define vmx_dword_decal_mask1					*(__vector4 *)&vmxi_dword_decal_mask1
#define vmx_dword_decal_mask2					*(__vector4 *)&vmxi_dword_decal_mask2
#define vmx_dword_decal_mask3					*(__vector4 *)&vmxi_dword_decal_mask3
#define vmx_dword_decal_mask4					*(__vector4 *)&vmxi_dword_decal_mask4
#define vmx_dword_decal_mask5					*(__vector4 *)&vmxi_dword_decal_mask5
#define vmx_dword_trace_mask0					*(__vector4 *)&vmxi_dword_trace_mask0
#define vmx_dword_trace_mask1					*(__vector4 *)&vmxi_dword_trace_mask1
#define vmx_dword_trace_mask2					*(__vector4 *)&vmxi_dword_trace_mask2
#define vmx_dword_trace_mask3					*(__vector4 *)&vmxi_dword_trace_mask3
#define vmx_dword_trace_mask4					*(__vector4 *)&vmxi_dword_trace_mask4
#define vmx_dword_trace_mask5					*(__vector4 *)&vmxi_dword_trace_mask5
#define vmx_dword_trace_mask6					*(__vector4 *)&vmxi_dword_trace_mask6
#define vmx_dword_trace_mask7					*(__vector4 *)&vmxi_dword_trace_mask7
#define vmx_dword_trace_xor						*(__vector4 *)&vmxi_dword_trace_xor
#define vmx_dword_facing_mask0					*(__vector4 *)&vmxi_dword_facing_mask0
#define vmx_dword_facing_mask1					*(__vector4 *)&vmxi_dword_facing_mask1
#define vmx_dword_facing_mask2					*(__vector4 *)&vmxi_dword_facing_mask2
#define vmx_dword_facing_mask3					*(__vector4 *)&vmxi_dword_facing_mask3
#define vmx_float_sign_bit						*(__vector4 *)&vmxi_float_sign_bit
#define vmx_float_pos_infinity					*(__vector4 *)&vmxi_float_pos_infinity
#define vmx_float_neg_infinity					*(__vector4 *)&vmxi_float_neg_infinity

/*
============
idSIMD_Xenon::GetName
============
*/
const char *idSIMD_Xenon::GetName( void ) const {
	return "Xenon";
}

/*
============
Xenon_ReciprocalSqrt
============
*/
ID_INLINE float Xenon_ReciprocalSqrt( float x ) {
	float r = __frsqrte( x );
	return ( x * r * r + -3.0f ) * ( r * -0.5f );
}

/*
============
Xenon_SinZeroHalfPI

  The angle must be between zero and half PI.
============
*/
ID_INLINE float Xenon_SinZeroHalfPI( float a ) {
	float s, t;

	assert( a >= 0.0f && a <= idMath::HALF_PI );

	s = a * a;
	t = -2.39e-08f;
	t *= s;
	t += 2.7526e-06f;
	t *= s;
	t += -1.98409e-04f;
	t *= s;
	t += 8.3333315e-03f;
	t *= s;
	t += -1.666666664e-01f;
	t *= s;
	t += 1.0f;
	t *= a;

	return t;
}

/*
============
Xenon_ATanPositive

  Both 'x' and 'y' must be positive.
============
*/
ID_INLINE float Xenon_ATanPositive( float y, float x ) {
	float a, b, c, d, s, t;

	assert( y >= 0.0f && x >= 0.0f );

	a = y - x;
	b = __fsel( a, -x, y );
	c = __fsel( a, y, x );
	d = __fsel( a, idMath::HALF_PI, 0.0f );

	a = b / c;
	s = a * a;
	t = 0.0028662257f;
	t *= s;
	t += -0.0161657367f;
	t *= s;
	t += 0.0429096138f;
	t *= s;
	t += -0.0752896400f;
	t *= s;
	t += 0.1065626393f;
	t *= s;
	t += -0.1420889944f;
	t *= s;
	t += 0.1999355085f;
	t *= s;
	t += -0.3333314528f;
	t *= s;
	t += 1.0f;
	t *= a;
	t += d;

	return t;
}

/*
============
idSIMD_Xenon::MinMax
============
*/
void VPCALL idSIMD_Xenon::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *__restrict src, const int count ) {
	const idDrawVert *__restrict end = src + count;

	assert_16_byte_aligned( src );
	assert_sizeof_16_byte_multiple( idDrawVert );

	__vector4 mask = vmx_dword_mask_clear_last;

	__vector4 min0 = vmx_float_pos_infinity;
	__vector4 min1 = vmx_float_pos_infinity;
	__vector4 min2 = vmx_float_pos_infinity;
	__vector4 min3 = vmx_float_pos_infinity;
	__vector4 min4 = vmx_float_pos_infinity;
	__vector4 min5 = vmx_float_pos_infinity;
	__vector4 min6 = vmx_float_pos_infinity;
	__vector4 min7 = vmx_float_pos_infinity;

	__vector4 max0 = vmx_float_neg_infinity;
	__vector4 max1 = vmx_float_neg_infinity;
	__vector4 max2 = vmx_float_neg_infinity;
	__vector4 max3 = vmx_float_neg_infinity;
	__vector4 max4 = vmx_float_neg_infinity;
	__vector4 max5 = vmx_float_neg_infinity;
	__vector4 max6 = vmx_float_neg_infinity;
	__vector4 max7 = vmx_float_neg_infinity;

	for ( ; src + 7 < end; src += 8 ) {
		__vector4 v0 = *(__vector4 *)&src[0].xyz;
		__vector4 v1 = *(__vector4 *)&src[1].xyz;
		__vector4 v2 = *(__vector4 *)&src[2].xyz;
		__vector4 v3 = *(__vector4 *)&src[3].xyz;
		__vector4 v4 = *(__vector4 *)&src[4].xyz;
		__vector4 v5 = *(__vector4 *)&src[5].xyz;
		__vector4 v6 = *(__vector4 *)&src[6].xyz;
		__vector4 v7 = *(__vector4 *)&src[7].xyz;

		__vector4 m0 = __vand( v0, mask );
		__vector4 m1 = __vand( v1, mask );
		__vector4 m2 = __vand( v2, mask );
		__vector4 m3 = __vand( v3, mask );
		__vector4 m4 = __vand( v4, mask );
		__vector4 m5 = __vand( v5, mask );
		__vector4 m6 = __vand( v6, mask );
		__vector4 m7 = __vand( v7, mask );

		min0 = __vminfp( min0, m0 );
		min1 = __vminfp( min1, m1 );
		min2 = __vminfp( min2, m2 );
		min3 = __vminfp( min3, m3 );
		min4 = __vminfp( min4, m4 );
		min5 = __vminfp( min5, m5 );
		min6 = __vminfp( min6, m6 );
		min7 = __vminfp( min7, m7 );

		max0 = __vmaxfp( max0, m0 );
		max1 = __vmaxfp( max1, m1 );
		max2 = __vmaxfp( max2, m2 );
		max3 = __vmaxfp( max3, m3 );
		max4 = __vmaxfp( max4, m4 );
		max5 = __vmaxfp( max5, m5 );
		max6 = __vmaxfp( max6, m6 );
		max7 = __vmaxfp( max7, m7 );
	}

	for ( ; src < end; src++ ) {
		__vector4 v0 = *(__vector4 *)&src[0].xyz;
		__vector4 m0 = __vand( v0, mask );

		min0 = __vminfp( min0, m0 );
		max0 = __vmaxfp( max0, m0 );
	}

	min0 = __vminfp( min0, min1 );
	max0 = __vmaxfp( max0, max1 );
	min2 = __vminfp( min2, min3 );
	max2 = __vmaxfp( max2, max3 );
	min4 = __vminfp( min4, min5 );
	max4 = __vmaxfp( max4, max5 );
	min6 = __vminfp( min6, min7 );
	max6 = __vmaxfp( max6, max7 );

	min0 = __vminfp( min0, min2 );
	max0 = __vmaxfp( max0, max2 );
	min4 = __vminfp( min4, min6 );
	max4 = __vmaxfp( max4, max6 );

	min0 = __vminfp( min0, min4 );
	max0 = __vmaxfp( max0, max4 );

	min[0] = min0.x;
	min[1] = min0.y;
	min[2] = min0.z;
	max[0] = max0.x;
	max[1] = max0.y;
	max[2] = max0.z;
}

/*
============
idSIMD_Xenon::MinMax
============
*/
void VPCALL idSIMD_Xenon::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *__restrict src, const int *indexes, const int count ) {
	const int *__restrict end = indexes + count;

	assert_16_byte_aligned( src );
	assert_sizeof_16_byte_multiple( idDrawVert );

	__vector4 mask = vmx_dword_mask_clear_last;

	__vector4 min0 = vmx_float_pos_infinity;
	__vector4 min1 = vmx_float_pos_infinity;
	__vector4 min2 = vmx_float_pos_infinity;
	__vector4 min3 = vmx_float_pos_infinity;
	__vector4 min4 = vmx_float_pos_infinity;
	__vector4 min5 = vmx_float_pos_infinity;
	__vector4 min6 = vmx_float_pos_infinity;
	__vector4 min7 = vmx_float_pos_infinity;

	__vector4 max0 = vmx_float_neg_infinity;
	__vector4 max1 = vmx_float_neg_infinity;
	__vector4 max2 = vmx_float_neg_infinity;
	__vector4 max3 = vmx_float_neg_infinity;
	__vector4 max4 = vmx_float_neg_infinity;
	__vector4 max5 = vmx_float_neg_infinity;
	__vector4 max6 = vmx_float_neg_infinity;
	__vector4 max7 = vmx_float_neg_infinity;

	for ( ; indexes + 7 < end; indexes += 8 ) {
		__vector4 v0 = *(__vector4 *)&src[indexes[0]].xyz;
		__vector4 v1 = *(__vector4 *)&src[indexes[1]].xyz;
		__vector4 v2 = *(__vector4 *)&src[indexes[2]].xyz;
		__vector4 v3 = *(__vector4 *)&src[indexes[3]].xyz;
		__vector4 v4 = *(__vector4 *)&src[indexes[4]].xyz;
		__vector4 v5 = *(__vector4 *)&src[indexes[5]].xyz;
		__vector4 v6 = *(__vector4 *)&src[indexes[6]].xyz;
		__vector4 v7 = *(__vector4 *)&src[indexes[7]].xyz;

		__vector4 m0 = __vand( v0, mask );
		__vector4 m1 = __vand( v1, mask );
		__vector4 m2 = __vand( v2, mask );
		__vector4 m3 = __vand( v3, mask );
		__vector4 m4 = __vand( v4, mask );
		__vector4 m5 = __vand( v5, mask );
		__vector4 m6 = __vand( v6, mask );
		__vector4 m7 = __vand( v7, mask );

		min0 = __vminfp( min0, m0 );
		min1 = __vminfp( min1, m1 );
		min2 = __vminfp( min2, m2 );
		min3 = __vminfp( min3, m3 );
		min4 = __vminfp( min4, m4 );
		min5 = __vminfp( min5, m5 );
		min6 = __vminfp( min6, m6 );
		min7 = __vminfp( min7, m7 );

		max0 = __vmaxfp( max0, m0 );
		max1 = __vmaxfp( max1, m1 );
		max2 = __vmaxfp( max2, m2 );
		max3 = __vmaxfp( max3, m3 );
		max4 = __vmaxfp( max4, m4 );
		max5 = __vmaxfp( max5, m5 );
		max6 = __vmaxfp( max6, m6 );
		max7 = __vmaxfp( max7, m7 );
	}

	for ( ; indexes < end; indexes++ ) {
		__vector4 v0 = *(__vector4 *)&src[indexes[0]].xyz;
		__vector4 m0 = __vand( v0, mask );

		min0 = __vminfp( min0, m0 );
		max0 = __vmaxfp( max0, m0 );
	}

	min0 = __vminfp( min0, min1 );
	max0 = __vmaxfp( max0, max1 );
	min2 = __vminfp( min2, min3 );
	max2 = __vmaxfp( max2, max3 );
	min4 = __vminfp( min4, min5 );
	max4 = __vmaxfp( max4, max5 );
	min6 = __vminfp( min6, min7 );
	max6 = __vmaxfp( max6, max7 );

	min0 = __vminfp( min0, min2 );
	max0 = __vmaxfp( max0, max2 );
	min4 = __vminfp( min4, min6 );
	max4 = __vmaxfp( max4, max6 );

	min0 = __vminfp( min0, min4 );
	max0 = __vmaxfp( max0, max4 );

	min[0] = min0.x;
	min[1] = min0.y;
	min[2] = min0.z;
	max[0] = max0.x;
	max[1] = max0.y;
	max[2] = max0.z;
}

/*
============
idSIMD_Xenon::BlendJoints
============
*/
void VPCALL idSIMD_Xenon::BlendJoints( idJointQuat *__restrict joints, const idJointQuat *__restrict blendJoints, const float lerp, const int *__restrict index, const int numJoints ) {
	int i;

	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( blendJoints );
	assert_16_byte_aligned( JOINTQUAT_Q_OFFSET );
	assert_16_byte_aligned( JOINTQUAT_T_OFFSET );
	assert_sizeof_16_byte_multiple( idJointQuat );

	if ( lerp <= 0.0f ) {
		return;
	} else if ( lerp >= 1.0f ) {
		for ( i = 0; i < numJoints; i++ ) {
			int j = index[i];
			joints[j] = blendJoints[j];
		}
		return;
	}

	__vector4 vlerp = { lerp, lerp, lerp, lerp };

	for ( i = 0; i < numJoints - 7; i += 8 ) {
		int n0 = index[i+0];
		int n1 = index[i+1];
		int n2 = index[i+2];
		int n3 = index[i+3];
		int n4 = index[i+4];
		int n5 = index[i+5];
		int n6 = index[i+6];
		int n7 = index[i+7];

		__vector4 jqa_0 = *(__vector4 *)&joints[n0].q;
		__vector4 jqb_0 = *(__vector4 *)&joints[n1].q;
		__vector4 jqc_0 = *(__vector4 *)&joints[n2].q;
		__vector4 jqd_0 = *(__vector4 *)&joints[n3].q;

		__vector4 jta_0 = *(__vector4 *)&joints[n0].t;
		__vector4 jtb_0 = *(__vector4 *)&joints[n1].t;
		__vector4 jtc_0 = *(__vector4 *)&joints[n2].t;
		__vector4 jtd_0 = *(__vector4 *)&joints[n3].t;

		__vector4 bqa_0 = *(__vector4 *)&blendJoints[n0].q;
		__vector4 bqb_0 = *(__vector4 *)&blendJoints[n1].q;
		__vector4 bqc_0 = *(__vector4 *)&blendJoints[n2].q;
		__vector4 bqd_0 = *(__vector4 *)&blendJoints[n3].q;

		__vector4 bta_0 = *(__vector4 *)&blendJoints[n0].t;
		__vector4 btb_0 = *(__vector4 *)&blendJoints[n1].t;
		__vector4 btc_0 = *(__vector4 *)&blendJoints[n2].t;
		__vector4 btd_0 = *(__vector4 *)&blendJoints[n3].t;

		__vector4 jqa_1 = *(__vector4 *)&joints[n4].q;
		__vector4 jqb_1 = *(__vector4 *)&joints[n5].q;
		__vector4 jqc_1 = *(__vector4 *)&joints[n6].q;
		__vector4 jqd_1 = *(__vector4 *)&joints[n7].q;

		__vector4 jta_1 = *(__vector4 *)&joints[n4].t;
		__vector4 jtb_1 = *(__vector4 *)&joints[n5].t;
		__vector4 jtc_1 = *(__vector4 *)&joints[n6].t;
		__vector4 jtd_1 = *(__vector4 *)&joints[n7].t;

		__vector4 bqa_1 = *(__vector4 *)&blendJoints[n4].q;
		__vector4 bqb_1 = *(__vector4 *)&blendJoints[n5].q;
		__vector4 bqc_1 = *(__vector4 *)&blendJoints[n6].q;
		__vector4 bqd_1 = *(__vector4 *)&blendJoints[n7].q;

		__vector4 bta_1 = *(__vector4 *)&blendJoints[n4].t;
		__vector4 btb_1 = *(__vector4 *)&blendJoints[n5].t;
		__vector4 btc_1 = *(__vector4 *)&blendJoints[n6].t;
		__vector4 btd_1 = *(__vector4 *)&blendJoints[n7].t;

		bta_0 = __vsubfp( bta_0, jta_0 );
		btb_0 = __vsubfp( btb_0, jtb_0 );
		btc_0 = __vsubfp( btc_0, jtc_0 );
		btd_0 = __vsubfp( btd_0, jtd_0 );

		bta_1 = __vsubfp( bta_1, jta_1 );
		btb_1 = __vsubfp( btb_1, jtb_1 );
		btc_1 = __vsubfp( btc_1, jtc_1 );
		btd_1 = __vsubfp( btd_1, jtd_1 );

		jta_0 = __vmaddfp( vlerp, bta_0, jta_0 );
		jtb_0 = __vmaddfp( vlerp, btb_0, jtb_0 );
		jtc_0 = __vmaddfp( vlerp, btc_0, jtc_0 );
		jtd_0 = __vmaddfp( vlerp, btd_0, jtd_0 );

		jta_1 = __vmaddfp( vlerp, bta_1, jta_1 );
		jtb_1 = __vmaddfp( vlerp, btb_1, jtb_1 );
		jtc_1 = __vmaddfp( vlerp, btc_1, jtc_1 );
		jtd_1 = __vmaddfp( vlerp, btd_1, jtd_1 );

		__stvx( jta_0, &joints[n0].t, 0 );
		__stvx( jtb_0, &joints[n1].t, 0 );
		__stvx( jtc_0, &joints[n2].t, 0 );
		__stvx( jtd_0, &joints[n3].t, 0 );

		__stvx( jta_1, &joints[n4].t, 0 );
		__stvx( jtb_1, &joints[n5].t, 0 );
		__stvx( jtc_1, &joints[n6].t, 0 );
		__stvx( jtd_1, &joints[n7].t, 0 );

		__vector4 jqr_0 = __vmrghw( jqa_0, jqb_0 );
		__vector4 jqs_0 = __vmrghw( jqc_0, jqd_0 );
		__vector4 jqt_0 = __vmrglw( jqa_0, jqb_0 );
		__vector4 jqu_0 = __vmrglw( jqc_0, jqd_0 );

		__vector4 jqr_1 = __vmrghw( jqa_1, jqb_1 );
		__vector4 jqs_1 = __vmrghw( jqc_1, jqd_1 );
		__vector4 jqt_1 = __vmrglw( jqa_1, jqb_1 );
		__vector4 jqu_1 = __vmrglw( jqc_1, jqd_1 );

		__vector4 bqr_0 = __vmrghw( bqa_0, bqb_0 );
		__vector4 bqs_0 = __vmrghw( bqc_0, bqd_0 );
		__vector4 bqt_0 = __vmrglw( bqa_0, bqb_0 );
		__vector4 bqu_0 = __vmrglw( bqc_0, bqd_0 );

		__vector4 bqr_1 = __vmrghw( bqa_1, bqb_1 );
		__vector4 bqs_1 = __vmrghw( bqc_1, bqd_1 );
		__vector4 bqt_1 = __vmrglw( bqa_1, bqb_1 );
		__vector4 bqu_1 = __vmrglw( bqc_1, bqd_1 );

		__vector4 jqx_0 = __vperm( jqr_0, jqs_0, vmx_dword_perm_plane_x );
		__vector4 jqy_0 = __vperm( jqr_0, jqs_0, vmx_dword_perm_plane_y );
		__vector4 jqz_0 = __vperm( jqt_0, jqu_0, vmx_dword_perm_plane_x );
		__vector4 jqw_0 = __vperm( jqt_0, jqu_0, vmx_dword_perm_plane_y );

		__vector4 jqx_1 = __vperm( jqr_1, jqs_1, vmx_dword_perm_plane_x );
		__vector4 jqy_1 = __vperm( jqr_1, jqs_1, vmx_dword_perm_plane_y );
		__vector4 jqz_1 = __vperm( jqt_1, jqu_1, vmx_dword_perm_plane_x );
		__vector4 jqw_1 = __vperm( jqt_1, jqu_1, vmx_dword_perm_plane_y );

		__vector4 bqx_0 = __vperm( bqr_0, bqs_0, vmx_dword_perm_plane_x );
		__vector4 bqy_0 = __vperm( bqr_0, bqs_0, vmx_dword_perm_plane_y );
		__vector4 bqz_0 = __vperm( bqt_0, bqu_0, vmx_dword_perm_plane_x );
		__vector4 bqw_0 = __vperm( bqt_0, bqu_0, vmx_dword_perm_plane_y );

		__vector4 bqx_1 = __vperm( bqr_1, bqs_1, vmx_dword_perm_plane_x );
		__vector4 bqy_1 = __vperm( bqr_1, bqs_1, vmx_dword_perm_plane_y );
		__vector4 bqz_1 = __vperm( bqt_1, bqu_1, vmx_dword_perm_plane_x );
		__vector4 bqw_1 = __vperm( bqt_1, bqu_1, vmx_dword_perm_plane_y );

		__vector4 cosoma_0 = __vmulfp( jqx_0, bqx_0 );
		__vector4 cosomb_0 = __vmulfp( jqy_0, bqy_0 );
		__vector4 cosomc_0 = __vmulfp( jqz_0, bqz_0 );
		__vector4 cosomd_0 = __vmulfp( jqw_0, bqw_0 );

		__vector4 cosoma_1 = __vmulfp( jqx_1, bqx_1 );
		__vector4 cosomb_1 = __vmulfp( jqy_1, bqy_1 );
		__vector4 cosomc_1 = __vmulfp( jqz_1, bqz_1 );
		__vector4 cosomd_1 = __vmulfp( jqw_1, bqw_1 );

		__vector4 cosome_0 = __vaddfp( cosoma_0, cosomb_0 );
		__vector4 cosomf_0 = __vaddfp( cosomc_0, cosomd_0 );
		__vector4 cosomg_0 = __vaddfp( cosome_0, cosomf_0 );

		__vector4 cosome_1 = __vaddfp( cosoma_1, cosomb_1 );
		__vector4 cosomf_1 = __vaddfp( cosomc_1, cosomd_1 );
		__vector4 cosomg_1 = __vaddfp( cosome_1, cosomf_1 );

		__vector4 sign_0 = __vand( cosomg_0, vmx_float_sign_bit );
		__vector4 cosom_0 = __vandc( cosomg_0, vmx_float_sign_bit );
		__vector4 ss_0 = __vnmsubfp( cosom_0, cosom_0, vmx_float_one );

		ss_0 = __vmaxfp( ss_0, vmx_float_tiny );

		__vector4 sign_1 = __vand( cosomg_1, vmx_float_sign_bit );
		__vector4 cosom_1 = __vandc( cosomg_1, vmx_float_sign_bit );
		__vector4 ss_1 = __vnmsubfp( cosom_1, cosom_1, vmx_float_one );

		ss_1 = __vmaxfp( ss_1, vmx_float_tiny );

		__vector4 rs_0 = __vrsqrtefp( ss_0 );
		__vector4 sq_0 = __vmulfp( rs_0, rs_0 );
		__vector4 sh_0 = __vmulfp( rs_0, vmx_float_rsqrt_c1 );
		__vector4 sx_0 = __vmaddfp( ss_0, sq_0, vmx_float_rsqrt_c0 );
		__vector4 sinom_0 = __vmulfp( sh_0, sx_0 );						// sinom = sqrt( ss );

		ss_0 = __vmulfp( ss_0, sinom_0 );

		__vector4 rs_1 = __vrsqrtefp( ss_1 );
		__vector4 sq_1 = __vmulfp( rs_1, rs_1 );
		__vector4 sh_1 = __vmulfp( rs_1, vmx_float_rsqrt_c1 );
		__vector4 sx_1 = __vmaddfp( ss_1, sq_1, vmx_float_rsqrt_c0 );
		__vector4 sinom_1 = __vmulfp( sh_1, sx_1 );						// sinom = sqrt( ss );

		ss_1 = __vmulfp( ss_1, sinom_1 );

		__vector4 min_0 = __vminfp( ss_0, cosom_0 );
		__vector4 max_0 = __vmaxfp( ss_0, cosom_0 );
		__vector4 mask_0 = __vcmpeqfp( min_0, cosom_0 );
		__vector4 masksign_0 = __vand( mask_0, vmx_float_sign_bit );
		__vector4 maskPI_0 = __vand( mask_0, vmx_float_PI );

		__vector4 min_1 = __vminfp( ss_1, cosom_1 );
		__vector4 max_1 = __vmaxfp( ss_1, cosom_1 );
		__vector4 mask_1 = __vcmpeqfp( min_1, cosom_1 );
		__vector4 masksign_1 = __vand( mask_1, vmx_float_sign_bit );
		__vector4 maskPI_1 = __vand( mask_1, vmx_float_PI );

		__vector4 rcpa_0 = __vrefp( max_0 );
		__vector4 rcpb_0 = __vmulfp( max_0, rcpa_0 );
		__vector4 rcpd_0 = __vaddfp( rcpa_0, rcpa_0 );
		__vector4 rcp_0 = __vnmsubfp( rcpb_0, rcpa_0, rcpd_0 );			// 1 / y or 1 / x
		__vector4 ata_0 = __vmulfp( min_0, rcp_0 );						// x / y or y / x

		__vector4 rcpa_1 = __vrefp( max_1 );
		__vector4 rcpb_1 = __vmulfp( max_1, rcpa_1 );
		__vector4 rcpd_1 = __vaddfp( rcpa_1, rcpa_1 );
		__vector4 rcp_1 = __vnmsubfp( rcpb_1, rcpa_1, rcpd_1 );			// 1 / y or 1 / x
		__vector4 ata_1 = __vmulfp( min_1, rcp_1 );						// x / y or y / x

		__vector4 atb_0 = __vxor( ata_0, masksign_0 );					// -x / y or y / x
		__vector4 atc_0 = __vmulfp( atb_0, atb_0 );
		__vector4 atd_0 = __vmaddfp( atc_0, vmx_float_atan_c0, vmx_float_atan_c1 );

		__vector4 atb_1 = __vxor( ata_1, masksign_1 );					// -x / y or y / x
		__vector4 atc_1 = __vmulfp( atb_1, atb_1 );
		__vector4 atd_1 = __vmaddfp( atc_1, vmx_float_atan_c0, vmx_float_atan_c1 );

		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c2 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c3 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c4 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c5 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c6 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c7 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_one );

		atd_1 = __vmaddfp( atd_1, atc_1, vmx_float_atan_c2 );
		atd_1 = __vmaddfp( atd_1, atc_1, vmx_float_atan_c3 );
		atd_1 = __vmaddfp( atd_1, atc_1, vmx_float_atan_c4 );
		atd_1 = __vmaddfp( atd_1, atc_1, vmx_float_atan_c5 );
		atd_1 = __vmaddfp( atd_1, atc_1, vmx_float_atan_c6 );
		atd_1 = __vmaddfp( atd_1, atc_1, vmx_float_atan_c7 );
		atd_1 = __vmaddfp( atd_1, atc_1, vmx_float_one );

		__vector4 omega_a_0 = __vmaddfp( atd_0, atb_0, maskPI_0 );
		__vector4 omega_b_0 = __vmulfp( vlerp, omega_a_0 );
		omega_a_0 = __vsubfp( omega_a_0, omega_b_0 );

		__vector4 omega_a_1 = __vmaddfp( atd_1, atb_1, maskPI_1 );
		__vector4 omega_b_1 = __vmulfp( vlerp, omega_a_1 );
		omega_a_1 = __vsubfp( omega_a_1, omega_b_1 );

		__vector4 sinsa_0 = __vmulfp( omega_a_0, omega_a_0 );
		__vector4 sinsb_0 = __vmulfp( omega_b_0, omega_b_0 );
		__vector4 sina_0 = __vmaddfp( sinsa_0, vmx_float_sin_c0, vmx_float_sin_c1 );
		__vector4 sinb_0 = __vmaddfp( sinsb_0, vmx_float_sin_c0, vmx_float_sin_c1 );
		sina_0 = __vmaddfp( sina_0, sinsa_0, vmx_float_sin_c2 );
		sinb_0 = __vmaddfp( sinb_0, sinsb_0, vmx_float_sin_c2 );
		sina_0 = __vmaddfp( sina_0, sinsa_0, vmx_float_sin_c3 );
		sinb_0 = __vmaddfp( sinb_0, sinsb_0, vmx_float_sin_c3 );
		sina_0 = __vmaddfp( sina_0, sinsa_0, vmx_float_sin_c4 );
		sinb_0 = __vmaddfp( sinb_0, sinsb_0, vmx_float_sin_c4 );
		sina_0 = __vmaddfp( sina_0, sinsa_0, vmx_float_one );
		sinb_0 = __vmaddfp( sinb_0, sinsb_0, vmx_float_one );
		sina_0 = __vmulfp( sina_0, omega_a_0 );
		sinb_0 = __vmulfp( sinb_0, omega_b_0 );
		__vector4 scalea_0 = __vmulfp( sina_0, sinom_0 );
		__vector4 scaleb_0 = __vmulfp( sinb_0, sinom_0 );

		__vector4 sinsa_1 = __vmulfp( omega_a_1, omega_a_1 );
		__vector4 sinsb_1 = __vmulfp( omega_b_1, omega_b_1 );
		__vector4 sina_1 = __vmaddfp( sinsa_1, vmx_float_sin_c0, vmx_float_sin_c1 );
		__vector4 sinb_1 = __vmaddfp( sinsb_1, vmx_float_sin_c0, vmx_float_sin_c1 );
		sina_1 = __vmaddfp( sina_1, sinsa_1, vmx_float_sin_c2 );
		sinb_1 = __vmaddfp( sinb_1, sinsb_1, vmx_float_sin_c2 );
		sina_1 = __vmaddfp( sina_1, sinsa_1, vmx_float_sin_c3 );
		sinb_1 = __vmaddfp( sinb_1, sinsb_1, vmx_float_sin_c3 );
		sina_1 = __vmaddfp( sina_1, sinsa_1, vmx_float_sin_c4 );
		sinb_1 = __vmaddfp( sinb_1, sinsb_1, vmx_float_sin_c4 );
		sina_1 = __vmaddfp( sina_1, sinsa_1, vmx_float_one );
		sinb_1 = __vmaddfp( sinb_1, sinsb_1, vmx_float_one );
		sina_1 = __vmulfp( sina_1, omega_a_1 );
		sinb_1 = __vmulfp( sinb_1, omega_b_1 );
		__vector4 scalea_1 = __vmulfp( sina_1, sinom_1 );
		__vector4 scaleb_1 = __vmulfp( sinb_1, sinom_1 );

		scaleb_0 = __vxor( scaleb_0, sign_0 );

		scaleb_1 = __vxor( scaleb_1, sign_1 );

		jqx_0 = __vmulfp( jqx_0, scalea_0 );
		jqy_0 = __vmulfp( jqy_0, scalea_0 );
		jqz_0 = __vmulfp( jqz_0, scalea_0 );
		jqw_0 = __vmulfp( jqw_0, scalea_0 );

		jqx_1 = __vmulfp( jqx_1, scalea_1 );
		jqy_1 = __vmulfp( jqy_1, scalea_1 );
		jqz_1 = __vmulfp( jqz_1, scalea_1 );
		jqw_1 = __vmulfp( jqw_1, scalea_1 );

		jqx_0 = __vmaddfp( bqx_0, scaleb_0, jqx_0 );
		jqy_0 = __vmaddfp( bqy_0, scaleb_0, jqy_0 );
		jqz_0 = __vmaddfp( bqz_0, scaleb_0, jqz_0 );
		jqw_0 = __vmaddfp( bqw_0, scaleb_0, jqw_0 );

		jqx_1 = __vmaddfp( bqx_1, scaleb_1, jqx_1 );
		jqy_1 = __vmaddfp( bqy_1, scaleb_1, jqy_1 );
		jqz_1 = __vmaddfp( bqz_1, scaleb_1, jqz_1 );
		jqw_1 = __vmaddfp( bqw_1, scaleb_1, jqw_1 );

		__vector4 tp0_0 = __vmrghw( jqx_0, jqy_0 );
		__vector4 tp1_0 = __vmrghw( jqz_0, jqw_0 );
		__vector4 tp2_0 = __vmrglw( jqx_0, jqy_0 );
		__vector4 tp3_0 = __vmrglw( jqz_0, jqw_0 );

		__vector4 tp0_1 = __vmrghw( jqx_1, jqy_1 );
		__vector4 tp1_1 = __vmrghw( jqz_1, jqw_1 );
		__vector4 tp2_1 = __vmrglw( jqx_1, jqy_1 );
		__vector4 tp3_1 = __vmrglw( jqz_1, jqw_1 );

		__vector4 p0_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_x );
		__vector4 p1_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_y );
		__vector4 p2_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_x );
		__vector4 p3_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_y );

		__vector4 p0_1 = __vperm( tp0_1, tp1_1, vmx_dword_perm_plane_x );
		__vector4 p1_1 = __vperm( tp0_1, tp1_1, vmx_dword_perm_plane_y );
		__vector4 p2_1 = __vperm( tp2_1, tp3_1, vmx_dword_perm_plane_x );
		__vector4 p3_1 = __vperm( tp2_1, tp3_1, vmx_dword_perm_plane_y );

		__stvx( p0_0, &joints[n0].q, 0 );
		__stvx( p1_0, &joints[n1].q, 0 );
		__stvx( p2_0, &joints[n2].q, 0 );
		__stvx( p3_0, &joints[n3].q, 0 );

		__stvx( p0_1, &joints[n4].q, 0 );
		__stvx( p1_1, &joints[n5].q, 0 );
		__stvx( p2_1, &joints[n6].q, 0 );
		__stvx( p3_1, &joints[n7].q, 0 );
	}

	for ( ; i < numJoints - 3; i += 4 ) {
		int n0 = index[i+0];
		int n1 = index[i+1];
		int n2 = index[i+2];
		int n3 = index[i+3];

		__vector4 jqa_0 = *(__vector4 *)&joints[n0].q;
		__vector4 jqb_0 = *(__vector4 *)&joints[n1].q;
		__vector4 jqc_0 = *(__vector4 *)&joints[n2].q;
		__vector4 jqd_0 = *(__vector4 *)&joints[n3].q;

		__vector4 jta_0 = *(__vector4 *)&joints[n0].t;
		__vector4 jtb_0 = *(__vector4 *)&joints[n1].t;
		__vector4 jtc_0 = *(__vector4 *)&joints[n2].t;
		__vector4 jtd_0 = *(__vector4 *)&joints[n3].t;

		__vector4 bqa_0 = *(__vector4 *)&blendJoints[n0].q;
		__vector4 bqb_0 = *(__vector4 *)&blendJoints[n1].q;
		__vector4 bqc_0 = *(__vector4 *)&blendJoints[n2].q;
		__vector4 bqd_0 = *(__vector4 *)&blendJoints[n3].q;

		__vector4 bta_0 = *(__vector4 *)&blendJoints[n0].t;
		__vector4 btb_0 = *(__vector4 *)&blendJoints[n1].t;
		__vector4 btc_0 = *(__vector4 *)&blendJoints[n2].t;
		__vector4 btd_0 = *(__vector4 *)&blendJoints[n3].t;

		bta_0 = __vsubfp( bta_0, jta_0 );
		btb_0 = __vsubfp( btb_0, jtb_0 );
		btc_0 = __vsubfp( btc_0, jtc_0 );
		btd_0 = __vsubfp( btd_0, jtd_0 );

		jta_0 = __vmaddfp( vlerp, bta_0, jta_0 );
		jtb_0 = __vmaddfp( vlerp, btb_0, jtb_0 );
		jtc_0 = __vmaddfp( vlerp, btc_0, jtc_0 );
		jtd_0 = __vmaddfp( vlerp, btd_0, jtd_0 );

		__stvx( jta_0, &joints[n0].t, 0 );
		__stvx( jtb_0, &joints[n1].t, 0 );
		__stvx( jtc_0, &joints[n2].t, 0 );
		__stvx( jtd_0, &joints[n3].t, 0 );

		__vector4 jqr_0 = __vmrghw( jqa_0, jqb_0 );
		__vector4 jqs_0 = __vmrghw( jqc_0, jqd_0 );
		__vector4 jqt_0 = __vmrglw( jqa_0, jqb_0 );
		__vector4 jqu_0 = __vmrglw( jqc_0, jqd_0 );

		__vector4 bqr_0 = __vmrghw( bqa_0, bqb_0 );
		__vector4 bqs_0 = __vmrghw( bqc_0, bqd_0 );
		__vector4 bqt_0 = __vmrglw( bqa_0, bqb_0 );
		__vector4 bqu_0 = __vmrglw( bqc_0, bqd_0 );

		__vector4 jqx_0 = __vperm( jqr_0, jqs_0, vmx_dword_perm_plane_x );
		__vector4 jqy_0 = __vperm( jqr_0, jqs_0, vmx_dword_perm_plane_y );
		__vector4 jqz_0 = __vperm( jqt_0, jqu_0, vmx_dword_perm_plane_x );
		__vector4 jqw_0 = __vperm( jqt_0, jqu_0, vmx_dword_perm_plane_y );

		__vector4 bqx_0 = __vperm( bqr_0, bqs_0, vmx_dword_perm_plane_x );
		__vector4 bqy_0 = __vperm( bqr_0, bqs_0, vmx_dword_perm_plane_y );
		__vector4 bqz_0 = __vperm( bqt_0, bqu_0, vmx_dword_perm_plane_x );
		__vector4 bqw_0 = __vperm( bqt_0, bqu_0, vmx_dword_perm_plane_y );

		__vector4 cosoma_0 = __vmulfp( jqx_0, bqx_0 );
		__vector4 cosomb_0 = __vmulfp( jqy_0, bqy_0 );
		__vector4 cosomc_0 = __vmulfp( jqz_0, bqz_0 );
		__vector4 cosomd_0 = __vmulfp( jqw_0, bqw_0 );

		__vector4 cosome_0 = __vaddfp( cosoma_0, cosomb_0 );
		__vector4 cosomf_0 = __vaddfp( cosomc_0, cosomd_0 );
		__vector4 cosomg_0 = __vaddfp( cosome_0, cosomf_0 );

		__vector4 sign_0 = __vand( cosomg_0, vmx_float_sign_bit );
		__vector4 cosom_0 = __vandc( cosomg_0, vmx_float_sign_bit );
		__vector4 ss_0 = __vnmsubfp( cosom_0, cosom_0, vmx_float_one );

		ss_0 = __vmaxfp( ss_0, vmx_float_tiny );

		__vector4 rs_0 = __vrsqrtefp( ss_0 );
		__vector4 sq_0 = __vmulfp( rs_0, rs_0 );
		__vector4 sh_0 = __vmulfp( rs_0, vmx_float_rsqrt_c1 );
		__vector4 sx_0 = __vmaddfp( ss_0, sq_0, vmx_float_rsqrt_c0 );
		__vector4 sinom_0 = __vmulfp( sh_0, sx_0 );						// sinom = sqrt( ss );

		ss_0 = __vmulfp( ss_0, sinom_0 );

		__vector4 min_0 = __vminfp( ss_0, cosom_0 );
		__vector4 max_0 = __vmaxfp( ss_0, cosom_0 );
		__vector4 mask_0 = __vcmpeqfp( min_0, cosom_0 );
		__vector4 masksign_0 = __vand( mask_0, vmx_float_sign_bit );
		__vector4 maskPI_0 = __vand( mask_0, vmx_float_PI );

		__vector4 rcpa_0 = __vrefp( max_0 );
		__vector4 rcpb_0 = __vmulfp( max_0, rcpa_0 );
		__vector4 rcpd_0 = __vaddfp( rcpa_0, rcpa_0 );
		__vector4 rcp_0 = __vnmsubfp( rcpb_0, rcpa_0, rcpd_0 );			// 1 / y or 1 / x
		__vector4 ata_0 = __vmulfp( min_0, rcp_0 );						// x / y or y / x

		__vector4 atb_0 = __vxor( ata_0, masksign_0 );					// -x / y or y / x
		__vector4 atc_0 = __vmulfp( atb_0, atb_0 );
		__vector4 atd_0 = __vmaddfp( atc_0, vmx_float_atan_c0, vmx_float_atan_c1 );

		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c2 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c3 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c4 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c5 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c6 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_atan_c7 );
		atd_0 = __vmaddfp( atd_0, atc_0, vmx_float_one );

		__vector4 omega_a_0 = __vmaddfp( atd_0, atb_0, maskPI_0 );
		__vector4 omega_b_0 = __vmulfp( vlerp, omega_a_0 );
		omega_a_0 = __vsubfp( omega_a_0, omega_b_0 );

		__vector4 sinsa_0 = __vmulfp( omega_a_0, omega_a_0 );
		__vector4 sinsb_0 = __vmulfp( omega_b_0, omega_b_0 );
		__vector4 sina_0 = __vmaddfp( sinsa_0, vmx_float_sin_c0, vmx_float_sin_c1 );
		__vector4 sinb_0 = __vmaddfp( sinsb_0, vmx_float_sin_c0, vmx_float_sin_c1 );
		sina_0 = __vmaddfp( sina_0, sinsa_0, vmx_float_sin_c2 );
		sinb_0 = __vmaddfp( sinb_0, sinsb_0, vmx_float_sin_c2 );
		sina_0 = __vmaddfp( sina_0, sinsa_0, vmx_float_sin_c3 );
		sinb_0 = __vmaddfp( sinb_0, sinsb_0, vmx_float_sin_c3 );
		sina_0 = __vmaddfp( sina_0, sinsa_0, vmx_float_sin_c4 );
		sinb_0 = __vmaddfp( sinb_0, sinsb_0, vmx_float_sin_c4 );
		sina_0 = __vmaddfp( sina_0, sinsa_0, vmx_float_one );
		sinb_0 = __vmaddfp( sinb_0, sinsb_0, vmx_float_one );
		sina_0 = __vmulfp( sina_0, omega_a_0 );
		sinb_0 = __vmulfp( sinb_0, omega_b_0 );
		__vector4 scalea_0 = __vmulfp( sina_0, sinom_0 );
		__vector4 scaleb_0 = __vmulfp( sinb_0, sinom_0 );

		scaleb_0 = __vxor( scaleb_0, sign_0 );

		jqx_0 = __vmulfp( jqx_0, scalea_0 );
		jqy_0 = __vmulfp( jqy_0, scalea_0 );
		jqz_0 = __vmulfp( jqz_0, scalea_0 );
		jqw_0 = __vmulfp( jqw_0, scalea_0 );

		jqx_0 = __vmaddfp( bqx_0, scaleb_0, jqx_0 );
		jqy_0 = __vmaddfp( bqy_0, scaleb_0, jqy_0 );
		jqz_0 = __vmaddfp( bqz_0, scaleb_0, jqz_0 );
		jqw_0 = __vmaddfp( bqw_0, scaleb_0, jqw_0 );

		__vector4 tp0_0 = __vmrghw( jqx_0, jqy_0 );
		__vector4 tp1_0 = __vmrghw( jqz_0, jqw_0 );
		__vector4 tp2_0 = __vmrglw( jqx_0, jqy_0 );
		__vector4 tp3_0 = __vmrglw( jqz_0, jqw_0 );

		__vector4 p0_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_x );
		__vector4 p1_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_y );
		__vector4 p2_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_x );
		__vector4 p3_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_y );

		__stvx( p0_0, &joints[n0].q, 0 );
		__stvx( p1_0, &joints[n1].q, 0 );
		__stvx( p2_0, &joints[n2].q, 0 );
		__stvx( p3_0, &joints[n3].q, 0 );
	}

	for ( ; i < numJoints; i++ ) {
		int n = index[i];

		idVec3 &jointVert = joints[n].t;
		const idVec3 &blendVert = blendJoints[n].t;

		jointVert[0] += lerp * ( blendVert[0] - jointVert[0] );
		jointVert[1] += lerp * ( blendVert[1] - jointVert[1] );
		jointVert[2] += lerp * ( blendVert[2] - jointVert[2] );

		idQuat &jointQuat = joints[n].q;
		const idQuat &blendQuat = blendJoints[n].q;

		float cosom;
		float sinom;
		float omega;
		float scale0;
		float scale1;
		float signBit;

		cosom = jointQuat.x * blendQuat.x + jointQuat.y * blendQuat.y + jointQuat.z * blendQuat.z + jointQuat.w * blendQuat.w;

		signBit = __fsel( cosom, 1.0f, -1.0f );

		cosom = __fabs( cosom );

		scale0 = 1.0f - cosom * cosom;
		scale0 = __fsel( -scale0, 1e-10f, scale0 );
		sinom = Xenon_ReciprocalSqrt( scale0 );
		omega = Xenon_ATanPositive( scale0 * sinom, cosom );
		scale0 = Xenon_SinZeroHalfPI( ( 1.0f - lerp ) * omega ) * sinom;
		scale1 = Xenon_SinZeroHalfPI( lerp * omega ) * sinom;

		scale1 *= signBit;

		jointQuat.x = scale0 * jointQuat.x + scale1 * blendQuat.x;
		jointQuat.y = scale0 * jointQuat.y + scale1 * blendQuat.y;
		jointQuat.z = scale0 * jointQuat.z + scale1 * blendQuat.z;
		jointQuat.w = scale0 * jointQuat.w + scale1 * blendQuat.w;
	}
}

/*
============
idSIMD_Xenon::BlendJointsFast
============
*/
void VPCALL idSIMD_Xenon::BlendJointsFast( idJointQuat *__restrict joints, const idJointQuat *__restrict blendJoints, const float lerp, const int *index, const int numJoints ) {
	int i;

	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( blendJoints );
	assert_16_byte_aligned( JOINTQUAT_Q_OFFSET );
	assert_16_byte_aligned( JOINTQUAT_T_OFFSET );
	assert_sizeof_16_byte_multiple( idJointQuat );

	if ( lerp <= 0.0f ) {
		return;
	} else if ( lerp >= 1.0f ) {
		for ( i = 0; i < numJoints; i++ ) {
			int j = index[i];
			joints[j] = blendJoints[j];
		}
		return;
	}

	float scaledLerp = lerp / ( 1.0f - lerp );

	__vector4 vlerp = { lerp, lerp, lerp, lerp };
	__vector4 vscaledLerp = { scaledLerp, scaledLerp, scaledLerp, scaledLerp };

	for ( i = 0; i < numJoints - 7; i += 8 ) {
		int n0 = index[i+0];
		int n1 = index[i+1];
		int n2 = index[i+2];
		int n3 = index[i+3];
		int n4 = index[i+4];
		int n5 = index[i+5];
		int n6 = index[i+6];
		int n7 = index[i+7];

		__vector4 jqa_0 = *(__vector4 *)&joints[n0].q;
		__vector4 jqb_0 = *(__vector4 *)&joints[n1].q;
		__vector4 jqc_0 = *(__vector4 *)&joints[n2].q;
		__vector4 jqd_0 = *(__vector4 *)&joints[n3].q;

		__vector4 jta_0 = *(__vector4 *)&joints[n0].t;
		__vector4 jtb_0 = *(__vector4 *)&joints[n1].t;
		__vector4 jtc_0 = *(__vector4 *)&joints[n2].t;
		__vector4 jtd_0 = *(__vector4 *)&joints[n3].t;

		__vector4 bqa_0 = *(__vector4 *)&blendJoints[n0].q;
		__vector4 bqb_0 = *(__vector4 *)&blendJoints[n1].q;
		__vector4 bqc_0 = *(__vector4 *)&blendJoints[n2].q;
		__vector4 bqd_0 = *(__vector4 *)&blendJoints[n3].q;

		__vector4 bta_0 = *(__vector4 *)&blendJoints[n0].t;
		__vector4 btb_0 = *(__vector4 *)&blendJoints[n1].t;
		__vector4 btc_0 = *(__vector4 *)&blendJoints[n2].t;
		__vector4 btd_0 = *(__vector4 *)&blendJoints[n3].t;

		__vector4 jqa_1 = *(__vector4 *)&joints[n4].q;
		__vector4 jqb_1 = *(__vector4 *)&joints[n5].q;
		__vector4 jqc_1 = *(__vector4 *)&joints[n6].q;
		__vector4 jqd_1 = *(__vector4 *)&joints[n7].q;

		__vector4 jta_1 = *(__vector4 *)&joints[n4].t;
		__vector4 jtb_1 = *(__vector4 *)&joints[n5].t;
		__vector4 jtc_1 = *(__vector4 *)&joints[n6].t;
		__vector4 jtd_1 = *(__vector4 *)&joints[n7].t;

		__vector4 bqa_1 = *(__vector4 *)&blendJoints[n4].q;
		__vector4 bqb_1 = *(__vector4 *)&blendJoints[n5].q;
		__vector4 bqc_1 = *(__vector4 *)&blendJoints[n6].q;
		__vector4 bqd_1 = *(__vector4 *)&blendJoints[n7].q;

		__vector4 bta_1 = *(__vector4 *)&blendJoints[n4].t;
		__vector4 btb_1 = *(__vector4 *)&blendJoints[n5].t;
		__vector4 btc_1 = *(__vector4 *)&blendJoints[n6].t;
		__vector4 btd_1 = *(__vector4 *)&blendJoints[n7].t;

		bta_0 = __vsubfp( bta_0, jta_0 );
		btb_0 = __vsubfp( btb_0, jtb_0 );
		btc_0 = __vsubfp( btc_0, jtc_0 );
		btd_0 = __vsubfp( btd_0, jtd_0 );

		bta_1 = __vsubfp( bta_1, jta_1 );
		btb_1 = __vsubfp( btb_1, jtb_1 );
		btc_1 = __vsubfp( btc_1, jtc_1 );
		btd_1 = __vsubfp( btd_1, jtd_1 );

		jta_0 = __vmaddfp( vlerp, bta_0, jta_0 );
		jtb_0 = __vmaddfp( vlerp, btb_0, jtb_0 );
		jtc_0 = __vmaddfp( vlerp, btc_0, jtc_0 );
		jtd_0 = __vmaddfp( vlerp, btd_0, jtd_0 );

		jta_1 = __vmaddfp( vlerp, bta_1, jta_1 );
		jtb_1 = __vmaddfp( vlerp, btb_1, jtb_1 );
		jtc_1 = __vmaddfp( vlerp, btc_1, jtc_1 );
		jtd_1 = __vmaddfp( vlerp, btd_1, jtd_1 );

		__stvx( jta_0, &joints[n0].t, 0 );
		__stvx( jtb_0, &joints[n1].t, 0 );
		__stvx( jtc_0, &joints[n2].t, 0 );
		__stvx( jtd_0, &joints[n3].t, 0 );

		__stvx( jta_1, &joints[n4].t, 0 );
		__stvx( jtb_1, &joints[n5].t, 0 );
		__stvx( jtc_1, &joints[n6].t, 0 );
		__stvx( jtd_1, &joints[n7].t, 0 );

		__vector4 jqr_0 = __vmrghw( jqa_0, jqb_0 );
		__vector4 jqs_0 = __vmrghw( jqc_0, jqd_0 );
		__vector4 jqt_0 = __vmrglw( jqa_0, jqb_0 );
		__vector4 jqu_0 = __vmrglw( jqc_0, jqd_0 );

		__vector4 jqr_1 = __vmrghw( jqa_1, jqb_1 );
		__vector4 jqs_1 = __vmrghw( jqc_1, jqd_1 );
		__vector4 jqt_1 = __vmrglw( jqa_1, jqb_1 );
		__vector4 jqu_1 = __vmrglw( jqc_1, jqd_1 );

		__vector4 bqr_0 = __vmrghw( bqa_0, bqb_0 );
		__vector4 bqs_0 = __vmrghw( bqc_0, bqd_0 );
		__vector4 bqt_0 = __vmrglw( bqa_0, bqb_0 );
		__vector4 bqu_0 = __vmrglw( bqc_0, bqd_0 );

		__vector4 bqr_1 = __vmrghw( bqa_1, bqb_1 );
		__vector4 bqs_1 = __vmrghw( bqc_1, bqd_1 );
		__vector4 bqt_1 = __vmrglw( bqa_1, bqb_1 );
		__vector4 bqu_1 = __vmrglw( bqc_1, bqd_1 );

		__vector4 jqx_0 = __vperm( jqr_0, jqs_0, vmx_dword_perm_plane_x );
		__vector4 jqy_0 = __vperm( jqr_0, jqs_0, vmx_dword_perm_plane_y );
		__vector4 jqz_0 = __vperm( jqt_0, jqu_0, vmx_dword_perm_plane_x );
		__vector4 jqw_0 = __vperm( jqt_0, jqu_0, vmx_dword_perm_plane_y );

		__vector4 jqx_1 = __vperm( jqr_1, jqs_1, vmx_dword_perm_plane_x );
		__vector4 jqy_1 = __vperm( jqr_1, jqs_1, vmx_dword_perm_plane_y );
		__vector4 jqz_1 = __vperm( jqt_1, jqu_1, vmx_dword_perm_plane_x );
		__vector4 jqw_1 = __vperm( jqt_1, jqu_1, vmx_dword_perm_plane_y );

		__vector4 bqx_0 = __vperm( bqr_0, bqs_0, vmx_dword_perm_plane_x );
		__vector4 bqy_0 = __vperm( bqr_0, bqs_0, vmx_dword_perm_plane_y );
		__vector4 bqz_0 = __vperm( bqt_0, bqu_0, vmx_dword_perm_plane_x );
		__vector4 bqw_0 = __vperm( bqt_0, bqu_0, vmx_dword_perm_plane_y );

		__vector4 bqx_1 = __vperm( bqr_1, bqs_1, vmx_dword_perm_plane_x );
		__vector4 bqy_1 = __vperm( bqr_1, bqs_1, vmx_dword_perm_plane_y );
		__vector4 bqz_1 = __vperm( bqt_1, bqu_1, vmx_dword_perm_plane_x );
		__vector4 bqw_1 = __vperm( bqt_1, bqu_1, vmx_dword_perm_plane_y );

		__vector4 cosoma_0 = __vmulfp( jqx_0, bqx_0 );
		__vector4 cosomb_0 = __vmulfp( jqy_0, bqy_0 );
		__vector4 cosomc_0 = __vmulfp( jqz_0, bqz_0 );
		__vector4 cosomd_0 = __vmulfp( jqw_0, bqw_0 );

		__vector4 cosoma_1 = __vmulfp( jqx_1, bqx_1 );
		__vector4 cosomb_1 = __vmulfp( jqy_1, bqy_1 );
		__vector4 cosomc_1 = __vmulfp( jqz_1, bqz_1 );
		__vector4 cosomd_1 = __vmulfp( jqw_1, bqw_1 );

		__vector4 cosome_0 = __vaddfp( cosoma_0, cosomb_0 );
		__vector4 cosomf_0 = __vaddfp( cosomc_0, cosomd_0 );
		__vector4 cosom_0 = __vaddfp( cosome_0, cosomf_0 );

		__vector4 cosome_1 = __vaddfp( cosoma_1, cosomb_1 );
		__vector4 cosomf_1 = __vaddfp( cosomc_1, cosomd_1 );
		__vector4 cosom_1 = __vaddfp( cosome_1, cosomf_1 );

		__vector4 sign_0 = __vand( cosom_0, vmx_float_sign_bit );
		__vector4 scale_0 = __vxor( vscaledLerp, sign_0 );

		__vector4 sign_1 = __vand( cosom_1, vmx_float_sign_bit );
		__vector4 scale_1 = __vxor( vscaledLerp, sign_1 );

		jqx_0 = __vmaddfp( scale_0, bqx_0, jqx_0 );
		jqy_0 = __vmaddfp( scale_0, bqy_0, jqy_0 );
		jqz_0 = __vmaddfp( scale_0, bqz_0, jqz_0 );
		jqw_0 = __vmaddfp( scale_0, bqw_0, jqw_0 );

		jqx_1 = __vmaddfp( scale_1, bqx_1, jqx_1 );
		jqy_1 = __vmaddfp( scale_1, bqy_1, jqy_1 );
		jqz_1 = __vmaddfp( scale_1, bqz_1, jqz_1 );
		jqw_1 = __vmaddfp( scale_1, bqw_1, jqw_1 );

		__vector4 da_0 = __vmulfp( jqx_0, jqx_0 );
		__vector4 db_0 = __vmulfp( jqy_0, jqy_0 );
		__vector4 dc_0 = __vmulfp( jqz_0, jqz_0 );
		__vector4 dd_0 = __vmulfp( jqw_0, jqw_0 );

		__vector4 da_1 = __vmulfp( jqx_1, jqx_1 );
		__vector4 db_1 = __vmulfp( jqy_1, jqy_1 );
		__vector4 dc_1 = __vmulfp( jqz_1, jqz_1 );
		__vector4 dd_1 = __vmulfp( jqw_1, jqw_1 );

		__vector4 de_0 = __vaddfp( da_0, db_0 );
		__vector4 df_0 = __vaddfp( dc_0, dd_0 );
		__vector4 d_0 = __vaddfp( de_0, df_0 );

		__vector4 de_1 = __vaddfp( da_1, db_1 );
		__vector4 df_1 = __vaddfp( dc_1, dd_1 );
		__vector4 d_1 = __vaddfp( de_1, df_1 );

		__vector4 rs_0 = __vrsqrtefp( d_0 );
		__vector4 sq_0 = __vmulfp( rs_0, rs_0 );
		__vector4 sh_0 = __vmulfp( rs_0, vmx_float_rsqrt_c1 );
		__vector4 sx_0 = __vmaddfp( d_0, sq_0, vmx_float_rsqrt_c0 );
		__vector4 s_0 = __vmulfp( sh_0, sx_0 );

		__vector4 rs_1 = __vrsqrtefp( d_1 );
		__vector4 sq_1 = __vmulfp( rs_1, rs_1 );
		__vector4 sh_1 = __vmulfp( rs_1, vmx_float_rsqrt_c1 );
		__vector4 sx_1 = __vmaddfp( d_1, sq_1, vmx_float_rsqrt_c0 );
		__vector4 s_1 = __vmulfp( sh_1, sx_1 );

		jqx_0 = __vmulfp( jqx_0, s_0 );
		jqy_0 = __vmulfp( jqy_0, s_0 );
		jqz_0 = __vmulfp( jqz_0, s_0 );
		jqw_0 = __vmulfp( jqw_0, s_0 );

		jqx_1 = __vmulfp( jqx_1, s_1 );
		jqy_1 = __vmulfp( jqy_1, s_1 );
		jqz_1 = __vmulfp( jqz_1, s_1 );
		jqw_1 = __vmulfp( jqw_1, s_1 );

		__vector4 tp0_0 = __vmrghw( jqx_0, jqy_0 );
		__vector4 tp1_0 = __vmrghw( jqz_0, jqw_0 );
		__vector4 tp2_0 = __vmrglw( jqx_0, jqy_0 );
		__vector4 tp3_0 = __vmrglw( jqz_0, jqw_0 );

		__vector4 tp0_1 = __vmrghw( jqx_1, jqy_1 );
		__vector4 tp1_1 = __vmrghw( jqz_1, jqw_1 );
		__vector4 tp2_1 = __vmrglw( jqx_1, jqy_1 );
		__vector4 tp3_1 = __vmrglw( jqz_1, jqw_1 );

		__vector4 p0_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_x );
		__vector4 p1_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_y );
		__vector4 p2_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_x );
		__vector4 p3_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_y );

		__vector4 p0_1 = __vperm( tp0_1, tp1_1, vmx_dword_perm_plane_x );
		__vector4 p1_1 = __vperm( tp0_1, tp1_1, vmx_dword_perm_plane_y );
		__vector4 p2_1 = __vperm( tp2_1, tp3_1, vmx_dword_perm_plane_x );
		__vector4 p3_1 = __vperm( tp2_1, tp3_1, vmx_dword_perm_plane_y );

		__stvx( p0_0, &joints[n0].q, 0 );
		__stvx( p1_0, &joints[n1].q, 0 );
		__stvx( p2_0, &joints[n2].q, 0 );
		__stvx( p3_0, &joints[n3].q, 0 );

		__stvx( p0_1, &joints[n4].q, 0 );
		__stvx( p1_1, &joints[n5].q, 0 );
		__stvx( p2_1, &joints[n6].q, 0 );
		__stvx( p3_1, &joints[n7].q, 0 );
	}

	for ( ; i < numJoints - 3; i += 4 ) {
		int n0 = index[i+0];
		int n1 = index[i+1];
		int n2 = index[i+2];
		int n3 = index[i+3];

		__vector4 jqa_0 = *(__vector4 *)&joints[n0].q;
		__vector4 jqb_0 = *(__vector4 *)&joints[n1].q;
		__vector4 jqc_0 = *(__vector4 *)&joints[n2].q;
		__vector4 jqd_0 = *(__vector4 *)&joints[n3].q;

		__vector4 jta_0 = *(__vector4 *)&joints[n0].t;
		__vector4 jtb_0 = *(__vector4 *)&joints[n1].t;
		__vector4 jtc_0 = *(__vector4 *)&joints[n2].t;
		__vector4 jtd_0 = *(__vector4 *)&joints[n3].t;

		__vector4 bqa_0 = *(__vector4 *)&blendJoints[n0].q;
		__vector4 bqb_0 = *(__vector4 *)&blendJoints[n1].q;
		__vector4 bqc_0 = *(__vector4 *)&blendJoints[n2].q;
		__vector4 bqd_0 = *(__vector4 *)&blendJoints[n3].q;

		__vector4 bta_0 = *(__vector4 *)&blendJoints[n0].t;
		__vector4 btb_0 = *(__vector4 *)&blendJoints[n1].t;
		__vector4 btc_0 = *(__vector4 *)&blendJoints[n2].t;
		__vector4 btd_0 = *(__vector4 *)&blendJoints[n3].t;

		bta_0 = __vsubfp( bta_0, jta_0 );
		btb_0 = __vsubfp( btb_0, jtb_0 );
		btc_0 = __vsubfp( btc_0, jtc_0 );
		btd_0 = __vsubfp( btd_0, jtd_0 );

		jta_0 = __vmaddfp( vlerp, bta_0, jta_0 );
		jtb_0 = __vmaddfp( vlerp, btb_0, jtb_0 );
		jtc_0 = __vmaddfp( vlerp, btc_0, jtc_0 );
		jtd_0 = __vmaddfp( vlerp, btd_0, jtd_0 );

		__stvx( jta_0, &joints[n0].t, 0 );
		__stvx( jtb_0, &joints[n1].t, 0 );
		__stvx( jtc_0, &joints[n2].t, 0 );
		__stvx( jtd_0, &joints[n3].t, 0 );

		__vector4 jqr_0 = __vmrghw( jqa_0, jqb_0 );
		__vector4 jqs_0 = __vmrghw( jqc_0, jqd_0 );
		__vector4 jqt_0 = __vmrglw( jqa_0, jqb_0 );
		__vector4 jqu_0 = __vmrglw( jqc_0, jqd_0 );

		__vector4 bqr_0 = __vmrghw( bqa_0, bqb_0 );
		__vector4 bqs_0 = __vmrghw( bqc_0, bqd_0 );
		__vector4 bqt_0 = __vmrglw( bqa_0, bqb_0 );
		__vector4 bqu_0 = __vmrglw( bqc_0, bqd_0 );

		__vector4 jqx_0 = __vperm( jqr_0, jqs_0, vmx_dword_perm_plane_x );
		__vector4 jqy_0 = __vperm( jqr_0, jqs_0, vmx_dword_perm_plane_y );
		__vector4 jqz_0 = __vperm( jqt_0, jqu_0, vmx_dword_perm_plane_x );
		__vector4 jqw_0 = __vperm( jqt_0, jqu_0, vmx_dword_perm_plane_y );

		__vector4 bqx_0 = __vperm( bqr_0, bqs_0, vmx_dword_perm_plane_x );
		__vector4 bqy_0 = __vperm( bqr_0, bqs_0, vmx_dword_perm_plane_y );
		__vector4 bqz_0 = __vperm( bqt_0, bqu_0, vmx_dword_perm_plane_x );
		__vector4 bqw_0 = __vperm( bqt_0, bqu_0, vmx_dword_perm_plane_y );

		__vector4 cosoma_0 = __vmulfp( jqx_0, bqx_0 );
		__vector4 cosomb_0 = __vmulfp( jqy_0, bqy_0 );
		__vector4 cosomc_0 = __vmulfp( jqz_0, bqz_0 );
		__vector4 cosomd_0 = __vmulfp( jqw_0, bqw_0 );

		__vector4 cosome_0 = __vaddfp( cosoma_0, cosomb_0 );
		__vector4 cosomf_0 = __vaddfp( cosomc_0, cosomd_0 );
		__vector4 cosom_0 = __vaddfp( cosome_0, cosomf_0 );

		__vector4 sign_0 = __vand( cosom_0, vmx_float_sign_bit );

		__vector4 scale_0 = __vxor( vscaledLerp, sign_0 );

		jqx_0 = __vmaddfp( scale_0, bqx_0, jqx_0 );
		jqy_0 = __vmaddfp( scale_0, bqy_0, jqy_0 );
		jqz_0 = __vmaddfp( scale_0, bqz_0, jqz_0 );
		jqw_0 = __vmaddfp( scale_0, bqw_0, jqw_0 );

		__vector4 da_0 = __vmulfp( jqx_0, jqx_0 );
		__vector4 db_0 = __vmulfp( jqy_0, jqy_0 );
		__vector4 dc_0 = __vmulfp( jqz_0, jqz_0 );
		__vector4 dd_0 = __vmulfp( jqw_0, jqw_0 );

		__vector4 de_0 = __vaddfp( da_0, db_0 );
		__vector4 df_0 = __vaddfp( dc_0, dd_0 );
		__vector4 d_0 = __vaddfp( de_0, df_0 );

		__vector4 rs_0 = __vrsqrtefp( d_0 );
		__vector4 sq_0 = __vmulfp( rs_0, rs_0 );
		__vector4 sh_0 = __vmulfp( rs_0, vmx_float_rsqrt_c1 );
		__vector4 sx_0 = __vmaddfp( d_0, sq_0, vmx_float_rsqrt_c0 );
		__vector4 s_0 = __vmulfp( sh_0, sx_0 );

		jqx_0 = __vmulfp( jqx_0, s_0 );
		jqy_0 = __vmulfp( jqy_0, s_0 );
		jqz_0 = __vmulfp( jqz_0, s_0 );
		jqw_0 = __vmulfp( jqw_0, s_0 );

		__vector4 tp0_0 = __vmrghw( jqx_0, jqy_0 );
		__vector4 tp1_0 = __vmrghw( jqz_0, jqw_0 );
		__vector4 tp2_0 = __vmrglw( jqx_0, jqy_0 );
		__vector4 tp3_0 = __vmrglw( jqz_0, jqw_0 );

		__vector4 p0_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_x );
		__vector4 p1_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_y );
		__vector4 p2_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_x );
		__vector4 p3_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_y );

		__stvx( p0_0, &joints[n0].q, 0 );
		__stvx( p1_0, &joints[n1].q, 0 );
		__stvx( p2_0, &joints[n2].q, 0 );
		__stvx( p3_0, &joints[n3].q, 0 );
	}

	for ( ; i < numJoints; i++ ) {
		int n = index[i];

		idVec3 &jointVert = joints[n].t;
		const idVec3 &blendVert = blendJoints[n].t;

		jointVert[0] += lerp * ( blendVert[0] - jointVert[0] );
		jointVert[1] += lerp * ( blendVert[1] - jointVert[1] );
		jointVert[2] += lerp * ( blendVert[2] - jointVert[2] );

		idQuat &jointQuat = joints[n].q;
		const idQuat &blendQuat = blendJoints[n].q;

		float cosom, scale, s;

		cosom = jointQuat.x * blendQuat.x + jointQuat.y * blendQuat.y + jointQuat.z * blendQuat.z + jointQuat.w * blendQuat.w;

		scale = __fsel( cosom, scaledLerp, -scaledLerp );

		jointQuat.x += scale * blendQuat.x;
		jointQuat.y += scale * blendQuat.y;
		jointQuat.z += scale * blendQuat.z;
		jointQuat.w += scale * blendQuat.w;

		s = jointQuat.x * jointQuat.x + jointQuat.y * jointQuat.y + jointQuat.z * jointQuat.z + jointQuat.w * jointQuat.w;
		s = Xenon_ReciprocalSqrt( s );

		jointQuat.x *= s;
		jointQuat.y *= s;
		jointQuat.z *= s;
		jointQuat.w *= s;
	}
}

/*
============
idSIMD_Xenon::ConvertJointQuatsToJointMats
============
*/
void VPCALL idSIMD_Xenon::ConvertJointQuatsToJointMats( idJointMat * __restrict jointMats, const idJointQuat * __restrict jointQuats, const int numJoints ) {
	idJointMat * __restrict end = jointMats + numJoints;

	assert_16_byte_aligned( jointMats );
	assert_16_byte_aligned( jointQuats );

	for ( ; jointMats + 3 < end; jointMats += 4, jointQuats += 4 ) {

		__vector4 q0 = __lvx( jointQuats, 0*32+0 );
		__vector4 r0 = __lvx( jointQuats, 0*32+16 );
		__vector4 q1 = __lvx( jointQuats, 1*32+0 );
		__vector4 r1 = __lvx( jointQuats, 1*32+16 );
		__vector4 q2 = __lvx( jointQuats, 2*32+0 );
		__vector4 r2 = __lvx( jointQuats, 2*32+16 );
		__vector4 q3 = __lvx( jointQuats, 3*32+0 );
		__vector4 r3 = __lvx( jointQuats, 3*32+16 );

		__vector4 d0 = __vaddfp( q0, q0 );
		__vector4 d1 = __vaddfp( q1, q1 );
		__vector4 d2 = __vaddfp( q2, q2 );
		__vector4 d3 = __vaddfp( q3, q3 );

		__vector4 sa0 = __vpermwi( q0, SHUFFLE_D( 1, 0, 0, 1 ) );
		__vector4 sb0 = __vpermwi( d0, SHUFFLE_D( 1, 1, 2, 2 ) );
		__vector4 sc0 = __vpermwi( q0, SHUFFLE_D( 2, 3, 3, 3 ) );
		__vector4 sd0 = __vpermwi( d0, SHUFFLE_D( 2, 2, 1, 0 ) );
		__vector4 sa1 = __vpermwi( q1, SHUFFLE_D( 1, 0, 0, 1 ) );
		__vector4 sb1 = __vpermwi( d1, SHUFFLE_D( 1, 1, 2, 2 ) );
		__vector4 sc1 = __vpermwi( q1, SHUFFLE_D( 2, 3, 3, 3 ) );
		__vector4 sd1 = __vpermwi( d1, SHUFFLE_D( 2, 2, 1, 0 ) );
		__vector4 sa2 = __vpermwi( q2, SHUFFLE_D( 1, 0, 0, 1 ) );
		__vector4 sb2 = __vpermwi( d2, SHUFFLE_D( 1, 1, 2, 2 ) );
		__vector4 sc2 = __vpermwi( q2, SHUFFLE_D( 2, 3, 3, 3 ) );
		__vector4 sd2 = __vpermwi( d2, SHUFFLE_D( 2, 2, 1, 0 ) );
		__vector4 sa3 = __vpermwi( q3, SHUFFLE_D( 1, 0, 0, 1 ) );
		__vector4 sb3 = __vpermwi( d3, SHUFFLE_D( 1, 1, 2, 2 ) );
		__vector4 sc3 = __vpermwi( q3, SHUFFLE_D( 2, 3, 3, 3 ) );
		__vector4 sd3 = __vpermwi( d3, SHUFFLE_D( 2, 2, 1, 0 ) );

		__vector4 ma0 = __vmulfp( sa0, sb0 );								//  yy2,  xy2,  xz2,  yz2
		__vector4 mb0 = __vmulfp( sc0, sd0 );								//  zz2,  wz2,  wy2,  wx2
		__vector4 mc0 = __vmulfp( q0, d0 );									//  xx2,  yy2,  zz2,  ww2
		__vector4 ma1 = __vmulfp( sa1, sb1 );								//  yy2,  xy2,  xz2,  yz2
		__vector4 mb1 = __vmulfp( sc1, sd1 );								//  zz2,  wz2,  wy2,  wx2
		__vector4 mc1 = __vmulfp( q1, d1 );									//  xx2,  yy2,  zz2,  ww2
		__vector4 ma2 = __vmulfp( sa2, sb2 );								//  yy2,  xy2,  xz2,  yz2
		__vector4 mb2 = __vmulfp( sc2, sd2 );								//  zz2,  wz2,  wy2,  wx2
		__vector4 mc2 = __vmulfp( q2, d2 );									//  xx2,  yy2,  zz2,  ww2
		__vector4 ma3 = __vmulfp( sa3, sb3 );								//  yy2,  xy2,  xz2,  yz2
		__vector4 mb3 = __vmulfp( sc3, sd3 );								//  zz2,  wz2,  wy2,  wx2
		__vector4 mc3 = __vmulfp( q3, d3 );									//  xx2,  yy2,  zz2,  ww2

		__vector4 md0 = __vperm( ma0, mc0, vmx_dword_perm_quat2mat1 );		//  xx2,  xy2,  xz2,  yz2							// 10, 01, 02, 03
		__vector4 me0 = __vperm( ma0, mb0, vmx_dword_perm_quat2mat2 );		//  yy2,  xy2,  wy2,  wx2							// 00, 01, 12, 13
		__vector4 md1 = __vperm( ma1, mc1, vmx_dword_perm_quat2mat1 );		//  xx2,  xy2,  xz2,  yz2							// 10, 01, 02, 03
		__vector4 me1 = __vperm( ma1, mb1, vmx_dword_perm_quat2mat2 );		//  yy2,  xy2,  wy2,  wx2							// 00, 01, 12, 13
		__vector4 md2 = __vperm( ma2, mc2, vmx_dword_perm_quat2mat1 );		//  xx2,  xy2,  xz2,  yz2							// 10, 01, 02, 03
		__vector4 me2 = __vperm( ma2, mb2, vmx_dword_perm_quat2mat2 );		//  yy2,  xy2,  wy2,  wx2							// 00, 01, 12, 13
		__vector4 md3 = __vperm( ma3, mc3, vmx_dword_perm_quat2mat1 );		//  xx2,  xy2,  xz2,  yz2							// 10, 01, 02, 03
		__vector4 me3 = __vperm( ma3, mb3, vmx_dword_perm_quat2mat2 );		//  yy2,  xy2,  wy2,  wx2							// 00, 01, 12, 13

		__vector4 mf0 = __vxor( ma0, vmx_float_quat2mat_xor );				// -yy2,  xy2,  xz2,  yz2							// - + + +
		__vector4 mg0 = __vxor( md0, vmx_float_quat2mat_xor );				// -xx2,  xy2,  xz2,  yz2							// - + + +
		__vector4 mf1 = __vxor( ma1, vmx_float_quat2mat_xor );				// -yy2,  xy2,  xz2,  yz2							// - + + +
		__vector4 mg1 = __vxor( md1, vmx_float_quat2mat_xor );				// -xx2,  xy2,  xz2,  yz2							// - + + +
		__vector4 mf2 = __vxor( ma2, vmx_float_quat2mat_xor );				// -yy2,  xy2,  xz2,  yz2							// - + + +
		__vector4 mg2 = __vxor( md2, vmx_float_quat2mat_xor );				// -xx2,  xy2,  xz2,  yz2							// - + + +
		__vector4 mf3 = __vxor( ma3, vmx_float_quat2mat_xor );				// -yy2,  xy2,  xz2,  yz2							// - + + +
		__vector4 mg3 = __vxor( md3, vmx_float_quat2mat_xor );				// -xx2,  xy2,  xz2,  yz2							// - + + +

		__vector4 ra0 = __vmaddfp( mb0, vmx_float_quat2mat_mad1, mf0 );		// -yy2 - zz2, xy2 + wz2, xz2 + wy2,				// - + - -
		__vector4 rb0 = __vmaddfp( mb0, vmx_float_quat2mat_mad2, mg0 );		// -xx2 - zz2, xy2 - wz2,          , yz2 + wx2		// - - - +
		__vector4 rc0 = __vmaddfp( me0, vmx_float_quat2mat_mad3, mg0 );		// -xx2 - yy2,          , xz2 + wy2, yz2 + wx2		// - - + -
		__vector4 ra1 = __vmaddfp( mb1, vmx_float_quat2mat_mad1, mf1 );		// -yy2 - zz2, xy2 + wz2, xz2 + wy2,				// - + - -
		__vector4 rb1 = __vmaddfp( mb1, vmx_float_quat2mat_mad2, mg1 );		// -xx2 - zz2, xy2 - wz2,          , yz2 + wx2		// - - - +
		__vector4 rc1 = __vmaddfp( me1, vmx_float_quat2mat_mad3, mg1 );		// -xx2 - yy2,          , xz2 + wy2, yz2 + wx2		// - - + -
		__vector4 ra2 = __vmaddfp( mb2, vmx_float_quat2mat_mad1, mf2 );		// -yy2 - zz2, xy2 + wz2, xz2 + wy2,				// - + - -
		__vector4 rb2 = __vmaddfp( mb2, vmx_float_quat2mat_mad2, mg2 );		// -xx2 - zz2, xy2 - wz2,          , yz2 + wx2		// - - - +
		__vector4 rc2 = __vmaddfp( me2, vmx_float_quat2mat_mad3, mg2 );		// -xx2 - yy2,          , xz2 + wy2, yz2 + wx2		// - - + -
		__vector4 ra3 = __vmaddfp( mb3, vmx_float_quat2mat_mad1, mf3 );		// -yy2 - zz2, xy2 + wz2, xz2 + wy2,				// - + - -
		__vector4 rb3 = __vmaddfp( mb3, vmx_float_quat2mat_mad2, mg3 );		// -xx2 - zz2, xy2 - wz2,          , yz2 + wx2		// - - - +
		__vector4 rc3 = __vmaddfp( me3, vmx_float_quat2mat_mad3, mg3 );		// -xx2 - yy2,          , xz2 + wy2, yz2 + wx2		// - - + -

		__vector4 re0 = __vaddfp( ra0, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 rf0 = __vaddfp( rb0, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 rg0 = __vaddfp( rc0, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 re1 = __vaddfp( ra1, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 rf1 = __vaddfp( rb1, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 rg1 = __vaddfp( rc1, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 re2 = __vaddfp( ra2, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 rf2 = __vaddfp( rb2, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 rg2 = __vaddfp( rc2, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 re3 = __vaddfp( ra3, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 rf3 = __vaddfp( rb3, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 rg3 = __vaddfp( rc3, vmx_float_quat2mat_add );			// 1 0 0 0

		__vector4 rh0 = __vperm( re0, r0, vmx_dword_perm_quat2mat3 );		// 00 01 02 10
		__vector4 ri0 = __vperm( rf0, r0, vmx_dword_perm_quat2mat4 );		// 01 00 03 11
		__vector4 rj0 = __vperm( rg0, r0, vmx_dword_perm_quat2mat5 );		// 02 03 00 12
		__vector4 rh1 = __vperm( re1, r1, vmx_dword_perm_quat2mat3 );		// 00 01 02 10
		__vector4 ri1 = __vperm( rf1, r1, vmx_dword_perm_quat2mat4 );		// 01 00 03 11
		__vector4 rj1 = __vperm( rg1, r1, vmx_dword_perm_quat2mat5 );		// 02 03 00 12
		__vector4 rh2 = __vperm( re2, r2, vmx_dword_perm_quat2mat3 );		// 00 01 02 10
		__vector4 ri2 = __vperm( rf2, r2, vmx_dword_perm_quat2mat4 );		// 01 00 03 11
		__vector4 rj2 = __vperm( rg2, r2, vmx_dword_perm_quat2mat5 );		// 02 03 00 12
		__vector4 rh3 = __vperm( re3, r3, vmx_dword_perm_quat2mat3 );		// 00 01 02 10
		__vector4 ri3 = __vperm( rf3, r3, vmx_dword_perm_quat2mat4 );		// 01 00 03 11
		__vector4 rj3 = __vperm( rg3, r3, vmx_dword_perm_quat2mat5 );		// 02 03 00 12

		__stvx( rh0, jointMats, 0*48+0 );
		__stvx( ri0, jointMats, 0*48+16 );
		__stvx( rj0, jointMats, 0*48+32 );
		__stvx( rh1, jointMats, 1*48+0 );
		__stvx( ri1, jointMats, 1*48+16 );
		__stvx( rj1, jointMats, 1*48+32 );
		__stvx( rh2, jointMats, 2*48+0 );
		__stvx( ri2, jointMats, 2*48+16 );
		__stvx( rj2, jointMats, 2*48+32 );
		__stvx( rh3, jointMats, 3*48+0 );
		__stvx( ri3, jointMats, 3*48+16 );
		__stvx( rj3, jointMats, 3*48+32 );
	}

	for ( ; jointMats < end; jointMats++, jointQuats++ ) {

		__vector4 q0 = __lvx( jointQuats, 0 );
		__vector4 r0 = __lvx( jointQuats, 16 );
		__vector4 d0 = __vaddfp( q0, q0 );

		__vector4 sa0 = __vpermwi( q0, SHUFFLE_D( 1, 0, 0, 1 ) );
		__vector4 sb0 = __vpermwi( d0, SHUFFLE_D( 1, 1, 2, 2 ) );

		__vector4 sc0 = __vpermwi( q0, SHUFFLE_D( 2, 3, 3, 3 ) );
		__vector4 sd0 = __vpermwi( d0, SHUFFLE_D( 2, 2, 1, 0 ) );

		__vector4 ma0 = __vmulfp( sa0, sb0 );								//  yy2,  xy2,  xz2,  yz2
		__vector4 mb0 = __vmulfp( sc0, sd0 );								//  zz2,  wz2,  wy2,  wx2
		__vector4 mc0 = __vmulfp( q0, d0 );									//  xx2,  yy2,  zz2,  ww2

		__vector4 md0 = __vperm( ma0, mc0, vmx_dword_perm_quat2mat1 );		//  xx2,  xy2,  xz2,  yz2							// 10, 01, 02, 03
		__vector4 me0 = __vperm( ma0, mb0, vmx_dword_perm_quat2mat2 );		//  yy2,  xy2,  wy2,  wx2							// 00, 01, 12, 13

		__vector4 mf0 = __vxor( ma0, vmx_float_quat2mat_xor );				// -yy2,  xy2,  xz2,  yz2							// - + + +
		__vector4 mg0 = __vxor( md0, vmx_float_quat2mat_xor );				// -xx2,  xy2,  xz2,  yz2							// - + + +

		__vector4 ra0 = __vmaddfp( mb0, vmx_float_quat2mat_mad1, mf0 );		// -yy2 - zz2, xy2 + wz2, xz2 + wy2,				// - + - -
		__vector4 rb0 = __vmaddfp( mb0, vmx_float_quat2mat_mad2, mg0 );		// -xx2 - zz2, xy2 - wz2,          , yz2 + wx2		// - - - +
		__vector4 rc0 = __vmaddfp( me0, vmx_float_quat2mat_mad3, mg0 );		// -xx2 - yy2,          , xz2 + wy2, yz2 + wx2		// - - + -

		__vector4 re0 = __vaddfp( ra0, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 rf0 = __vaddfp( rb0, vmx_float_quat2mat_add );			// 1 0 0 0
		__vector4 rg0 = __vaddfp( rc0, vmx_float_quat2mat_add );			// 1 0 0 0

		__vector4 rh0 = __vperm( re0, r0, vmx_dword_perm_quat2mat3 );		// 00 01 02 10
		__vector4 ri0 = __vperm( rf0, r0, vmx_dword_perm_quat2mat4 );		// 01 00 03 11
		__vector4 rj0 = __vperm( rg0, r0, vmx_dword_perm_quat2mat5 );		// 02 03 00 12

		__stvx( rh0, jointMats, 0 );
		__stvx( ri0, jointMats, 16 );
		__stvx( rj0, jointMats, 32 );
	}
}

/*
============
idSIMD_Xenon::ConvertJointMatsToJointQuats
============
*/
void VPCALL idSIMD_Xenon::ConvertJointMatsToJointQuats( idJointQuat * __restrict jointQuats, const idJointMat * __restrict jointMats, const int numJoints ) {
	int i;

	assert_16_byte_aligned( jointMats );
	assert_16_byte_aligned( jointQuats );
	compile_time_assert( (UINT_PTR)(&((idJointQuat *)0)->t) == (UINT_PTR)(&((idJointQuat *)0)->q) + (UINT_PTR)sizeof( ((idJointQuat *)0)->q ) );

	__vector4 zero = vmx_float_zero;

	for ( i = 0; i < numJoints - 3; i += 4 ) {
		const float *__restrict m = (float *)&jointMats[i];
		float *__restrict q = (float *)&jointQuats[i];

		__vector4 ma0 = __lvx( m, 0*48+0 );
		__vector4 ma1 = __lvx( m, 0*48+16 );
		__vector4 ma2 = __lvx( m, 0*48+32 );

		__vector4 mb0 = __lvx( m, 1*48+0 );
		__vector4 mb1 = __lvx( m, 1*48+16 );
		__vector4 mb2 = __lvx( m, 1*48+32 );

		__vector4 mc0 = __lvx( m, 2*48+0 );
		__vector4 mc1 = __lvx( m, 2*48+16 );
		__vector4 mc2 = __lvx( m, 2*48+32 );

		__vector4 md0 = __lvx( m, 3*48+0 );
		__vector4 md1 = __lvx( m, 3*48+16 );
		__vector4 md2 = __lvx( m, 3*48+32 );

		__vector4 ta0 = __vmrghw( ma0, mb0 );
		__vector4 ta1 = __vmrghw( mc0, md0 );
		__vector4 ta2 = __vmrglw( ma0, mb0 );
		__vector4 ta3 = __vmrglw( mc0, md0 );

		__vector4 tb0 = __vmrghw( ma1, mb1 );
		__vector4 tb1 = __vmrghw( mc1, md1 );
		__vector4 tb2 = __vmrglw( ma1, mb1 );
		__vector4 tb3 = __vmrglw( mc1, md1 );

		__vector4 tc0 = __vmrghw( ma2, mb2 );
		__vector4 tc1 = __vmrghw( mc2, md2 );
		__vector4 tc2 = __vmrglw( ma2, mb2 );
		__vector4 tc3 = __vmrglw( mc2, md2 );

		__vector4 m00 = __vperm( ta0, ta1, vmx_dword_perm_plane_x );
		__vector4 m01 = __vperm( ta0, ta1, vmx_dword_perm_plane_y );
		__vector4 m02 = __vperm( ta2, ta3, vmx_dword_perm_plane_x );
		__vector4 m03 = __vperm( ta2, ta3, vmx_dword_perm_plane_y );

		__vector4 m10 = __vperm( tb0, tb1, vmx_dword_perm_plane_x );
		__vector4 m11 = __vperm( tb0, tb1, vmx_dword_perm_plane_y );
		__vector4 m12 = __vperm( tb2, tb3, vmx_dword_perm_plane_x );
		__vector4 m13 = __vperm( tb2, tb3, vmx_dword_perm_plane_y );

		__vector4 m20 = __vperm( tc0, tc1, vmx_dword_perm_plane_x );
		__vector4 m21 = __vperm( tc0, tc1, vmx_dword_perm_plane_y );
		__vector4 m22 = __vperm( tc2, tc3, vmx_dword_perm_plane_x );
		__vector4 m23 = __vperm( tc2, tc3, vmx_dword_perm_plane_y );

		__vector4 b00 = __vaddfp( m00, m11 );
		__vector4 b11 = __vcmpgtfp( m00, m22 );
		__vector4 b01 = __vaddfp( b00, m22 );
		__vector4 b10 = __vcmpgtfp( m00, m11 );
		__vector4 b0  = __vcmpgtfp( b01, zero );
		__vector4 b1  = __vand( b10, b11 );
		__vector4 b2  = __vcmpgtfp( m11, m22 );

		__vector4 m0  = b0;
		__vector4 m1  = __vandc( b1, b0 );
		__vector4 p1  = __vor( b0, b1 );
		__vector4 p2  = __vor( p1, b2 );
		__vector4 m2  = __vandc( b2, p1 );
		__vector4 m3  = __vxor( p2, vmx_dword_not );

		__vector4 i0  = __vor( m2, m3 );
		__vector4 i1  = __vor( m1, m3 );
		__vector4 i2  = __vor( m1, m2 );

		__vector4 s0  = __vand( i0, vmx_float_sign_bit );
		__vector4 s1  = __vand( i1, vmx_float_sign_bit );
		__vector4 s2  = __vand( i2, vmx_float_sign_bit );

		__vector4 n0  = __vand( m0, vmx_dword_quat2mat_swizzle0 );
		__vector4 n1  = __vand( m1, vmx_dword_quat2mat_swizzle1 );
		__vector4 n2  = __vand( m2, vmx_dword_quat2mat_swizzle2 );
		__vector4 n3  = __vand( m3, vmx_dword_quat2mat_swizzle3 );

		__vector4 n4  = __vor( n0, n1 );
		__vector4 n5  = __vor( n2, n3 );
		__vector4 n6  = __vor( n4, n5 );

		m00 = __vxor( m00, s0 );
		m11 = __vxor( m11, s1 );
		m22 = __vxor( m22, s2 );
		m21 = __vxor( m21, s0 );
		m02 = __vxor( m02, s1 );
		m10 = __vxor( m10, s2 );

		__vector4 t0  = __vaddfp( m00, m11 );
		__vector4 t1  = __vaddfp( m22, vmx_float_one );
		__vector4 q0  = __vaddfp( t0, t1 );
		__vector4 q1  = __vsubfp( m01, m10 );
		__vector4 q2  = __vsubfp( m20, m02 );
		__vector4 q3  = __vsubfp( m12, m21 );

		__vector4 rs = __vrsqrtefp( q0 );
		__vector4 sq = __vmulfp( rs, rs );
		__vector4 sh = __vmulfp( rs, vmx_float_rsqrt_c2 );
		__vector4 sx = __vmaddfp( q0, sq, vmx_float_rsqrt_c0 );
		__vector4 s  = __vmulfp( sh, sx );

		q0 = __vmulfp( q0, s );
		q1 = __vmulfp( q1, s );
		q2 = __vmulfp( q2, s );
		q3 = __vmulfp( q3, s );

		__vector4 tq0 = __vmrghw( q0, q1 );
		__vector4 tq1 = __vmrghw( q2, q3 );
		__vector4 tq2 = __vmrglw( q0, q1 );
		__vector4 tq3 = __vmrglw( q2, q3 );

		__vector4 tt0 = __vmrghw( m03, m13 );
		__vector4 tt1 = __vmrghw( m23, zero );
		__vector4 tt2 = __vmrglw( m03, m13 );
		__vector4 tt3 = __vmrglw( m23, zero );

		__vector4 sw0 = __vperm( n6, zero, vmx_dword_quat2mat_splat0 );
		__vector4 sw1 = __vperm( n6, zero, vmx_dword_quat2mat_splat1 );
		__vector4 sw2 = __vperm( n6, zero, vmx_dword_quat2mat_splat2 );
		__vector4 sw3 = __vperm( n6, zero, vmx_dword_quat2mat_splat3 );

		sw0 = __vor( sw0, vmx_dword_quat2mat_or );
		sw1 = __vor( sw1, vmx_dword_quat2mat_or );
		sw2 = __vor( sw2, vmx_dword_quat2mat_or );
		sw3 = __vor( sw3, vmx_dword_quat2mat_or );

		__vector4 r0 = __vperm( tq0, tq1, vmx_dword_perm_plane_x );
		__vector4 r1 = __vperm( tq0, tq1, vmx_dword_perm_plane_y );
		__vector4 r2 = __vperm( tq2, tq3, vmx_dword_perm_plane_x );
		__vector4 r3 = __vperm( tq2, tq3, vmx_dword_perm_plane_y );

		__vector4 r4 = __vperm( tt0, tt1, vmx_dword_perm_plane_x );
		__vector4 r5 = __vperm( tt0, tt1, vmx_dword_perm_plane_y );
		__vector4 r6 = __vperm( tt2, tt3, vmx_dword_perm_plane_x );
		__vector4 r7 = __vperm( tt2, tt3, vmx_dword_perm_plane_y );

		r0 = __vperm( r0, r0, sw0 );
		r1 = __vperm( r1, r1, sw1 );
		r2 = __vperm( r2, r2, sw2 );
		r3 = __vperm( r3, r3, sw3 );

		__stvx( r0, q, 0*16 );
		__stvx( r4, q, 1*16 );
		__stvx( r1, q, 2*16 );
		__stvx( r5, q, 3*16 );
		__stvx( r2, q, 4*16 );
		__stvx( r6, q, 5*16 );
		__stvx( r3, q, 6*16 );
		__stvx( r7, q, 7*16 );
	}

	int k0 = (3<<0)|(2<<2)|(1<<4)|(0<<6);
	int k1 = (0<<0)|(1<<2)|(2<<4)|(3<<6);
	int k2 = (1<<0)|(0<<2)|(3<<4)|(2<<6);
	int k3 = (2<<0)|(3<<2)|(0<<4)|(1<<6);
	float sign[2] = { 1.0f, -1.0f };

	for ( ; i < numJoints; i++, jointMats++, jointQuats++ ) {
		const float *m = (float *)jointMats;
		float *q = (float *)jointQuats;

		int b0 = m[0 * 4 + 0] + m[1 * 4 + 1] + m[2 * 4 + 2] > 0.0f;
		int b1 = m[0 * 4 + 0] > m[1 * 4 + 1] && m[0 * 4 + 0] > m[2 * 4 + 2];
		int b2 = m[1 * 4 + 1] > m[2 * 4 + 2];

		int m0 = b0;
		int m1 = !b0 & b1;
		int m2 = !( b0 | b1 ) & b2;
		int m3 = !( b0 | b1 | b2 );

		int i0 = ( m2 | m3 );
		int i1 = ( m1 | m3 );
		int i2 = ( m1 | m2 );

		float s0 = sign[i0];
		float s1 = sign[i1];
		float s2 = sign[i2];

		int index = ( -m0 & k0 ) | ( -m1 & k1 ) | ( -m2 & k2 ) | ( -m3 & k3 );

		float t = s0 * m[0 * 4 + 0] + s1 * m[1 * 4 + 1] + s2 * m[2 * 4 + 2] + 1.0f;
		float s = __frsqrte( t );
		s = ( t * s * s + -3.0f ) * ( s * -0.25f );

		q[(index>>0)&3] = t * s;
		q[(index>>2)&3] = ( m[0 * 4 + 1] - s2 * m[1 * 4 + 0] ) * s;
		q[(index>>4)&3] = ( m[2 * 4 + 0] - s1 * m[0 * 4 + 2] ) * s;
		q[(index>>6)&3] = ( m[1 * 4 + 2] - s0 * m[2 * 4 + 1] ) * s;

		q[4] = m[0 * 4 + 3];
		q[5] = m[1 * 4 + 3];
		q[6] = m[2 * 4 + 3];
		q[7] = 0.0f;
	}
}

/*
============
idSIMD_Xenon::TransformJoints
============
*/
void VPCALL idSIMD_Xenon::TransformJoints( idJointMat *__restrict jointMats, const int *__restrict parents, const int firstJoint, const int lastJoint ) {
	for( int i = firstJoint; i <= lastJoint; i++ ) {
		assert( parents[i] < i );

		float *__restrict m1 = jointMats[parents[i]].ToFloatPtr();
		float *__restrict m2 = jointMats[i].ToFloatPtr();

		__vector4 m1a0 = __lvx( m1, 0 );
		__vector4 m1b0 = __lvx( m1, 16 );
		__vector4 m1c0 = __lvx( m1, 32 );

		__vector4 m2a0 = __lvx( m2, 0 );
		__vector4 m2b0 = __lvx( m2, 16 );
		__vector4 m2c0 = __lvx( m2, 32 );

		__vector4 ta0 = __vspltw( m1a0, 0 );
		__vector4 tb0 = __vspltw( m1a0, 1 );
		__vector4 tc0 = __vspltw( m1a0, 2 );

		__vector4 td0 = __vspltw( m1b0, 0 );
		__vector4 te0 = __vspltw( m1b0, 1 );
		__vector4 tf0 = __vspltw( m1b0, 2 );

		__vector4 tg0 = __vspltw( m1c0, 0 );
		__vector4 th0 = __vspltw( m1c0, 1 );
		__vector4 ti0 = __vspltw( m1c0, 2 );

		__vector4 tj0 = __vperm( m1a0, vmx_float_zero, vmx_dword_perm_matrix );
		__vector4 tk0 = __vperm( m1b0, vmx_float_zero, vmx_dword_perm_matrix );
		__vector4 tl0 = __vperm( m1c0, vmx_float_zero, vmx_dword_perm_matrix );

		__vector4 ra0 = __vmulfp( ta0, m2a0 );
		__vector4 rb0 = __vmulfp( tb0, m2b0 );
		__vector4 rc0 = __vmulfp( tc0, m2c0 );

		__vector4 rd0 = __vmulfp( td0, m2a0 );
		__vector4 re0 = __vmulfp( te0, m2b0 );
		__vector4 rf0 = __vmulfp( tf0, m2c0 );

		__vector4 rg0 = __vmulfp( tg0, m2a0 );
		__vector4 rh0 = __vmulfp( th0, m2b0 );
		__vector4 ri0 = __vmulfp( ti0, m2c0 );

		__vector4 sa0 = __vaddfp( ra0, rb0 );
		__vector4 sb0 = __vaddfp( rc0, tj0 );
		__vector4 sc0 = __vaddfp( sa0, sb0 );

		__vector4 sd0 = __vaddfp( rd0, re0 );
		__vector4 se0 = __vaddfp( rf0, tk0 );
		__vector4 sf0 = __vaddfp( sd0, se0 );

		__vector4 sg0 = __vaddfp( rg0, rh0 );
		__vector4 sh0 = __vaddfp( ri0, tl0 );
		__vector4 si0 = __vaddfp( sg0, sh0 );

		__stvx( sc0, m2, 0 );
		__stvx( sf0, m2, 16 );
		__stvx( si0, m2, 32 );
	}
}

/*
============
idSIMD_Xenon::UntransformJoints
============
*/
void VPCALL idSIMD_Xenon::UntransformJoints( idJointMat *__restrict jointMats, const int *__restrict parents, const int firstJoint, const int lastJoint ) {
	for( int i = lastJoint; i >= firstJoint; i-- ) {
		assert( parents[i] < i );

		float *__restrict m1 = jointMats[parents[i]].ToFloatPtr();
		float *__restrict m2 = jointMats[i].ToFloatPtr();

		__vector4 m1a0 = __lvx( m1, 0 );
		__vector4 m1b0 = __lvx( m1, 16 );
		__vector4 m1c0 = __lvx( m1, 32 );

		__vector4 m2a0 = __lvx( m2, 0 );
		__vector4 m2b0 = __lvx( m2, 16 );
		__vector4 m2c0 = __lvx( m2, 32 );

		__vector4 ta0 = __vspltw( m1a0, 0 );
		__vector4 tb0 = __vspltw( m1a0, 1 );
		__vector4 tc0 = __vspltw( m1a0, 2 );

		__vector4 td0 = __vspltw( m1b0, 0 );
		__vector4 te0 = __vspltw( m1b0, 1 );
		__vector4 tf0 = __vspltw( m1b0, 2 );

		__vector4 tg0 = __vspltw( m1c0, 0 );
		__vector4 th0 = __vspltw( m1c0, 1 );
		__vector4 ti0 = __vspltw( m1c0, 2 );

		__vector4 tj0 = __vperm( m1a0, vmx_float_zero, vmx_dword_perm_matrix );
		__vector4 tk0 = __vperm( m1b0, vmx_float_zero, vmx_dword_perm_matrix );
		__vector4 tl0 = __vperm( m1c0, vmx_float_zero, vmx_dword_perm_matrix );

		m2a0 = __vsubfp( m2a0, tj0 );
		m2b0 = __vsubfp( m2b0, tk0 );
		m2c0 = __vsubfp( m2c0, tl0 );

		__vector4 ra0 = __vmulfp( ta0, m2a0 );
		__vector4 rb0 = __vmulfp( td0, m2b0 );
		__vector4 rc0 = __vmulfp( tg0, m2c0 );

		__vector4 rd0 = __vmulfp( tb0, m2a0 );
		__vector4 re0 = __vmulfp( te0, m2b0 );
		__vector4 rf0 = __vmulfp( th0, m2c0 );

		__vector4 rg0 = __vmulfp( tc0, m2a0 );
		__vector4 rh0 = __vmulfp( tf0, m2b0 );
		__vector4 ri0 = __vmulfp( ti0, m2c0 );

		__vector4 sa0 = __vaddfp( ra0, rb0 );
		__vector4 sb0 = __vaddfp( rc0, sa0 );

		__vector4 sd0 = __vaddfp( rd0, re0 );
		__vector4 se0 = __vaddfp( rf0, sd0 );

		__vector4 sg0 = __vaddfp( rg0, rh0 );
		__vector4 sh0 = __vaddfp( ri0, sg0 );

		__stvx( sb0, m2, 0 );
		__stvx( se0, m2, 16 );
		__stvx( sh0, m2, 32 );
	}
}

/*
============
idSIMD_Xenon::MultiplyJoints
============
*/
void VPCALL idSIMD_Xenon::MultiplyJoints( idJointMat *__restrict result, const idJointMat *__restrict joints1, const idJointMat *__restrict joints2, const int numJoints ) {
	idJointMat *__restrict end = result + numJoints;

	assert_16_byte_aligned( result );
	assert_16_byte_aligned( joints1 );
	assert_16_byte_aligned( joints2 );
	assert_sizeof_16_byte_multiple( idJointMat );

	for ( ; result + 1 < end; result += 2, joints1 += 2, joints2 += 2 ) {
		__vector4 m1a0 = __lvx( joints1, 0 );
		__vector4 m1b0 = __lvx( joints1, 16 );
		__vector4 m1c0 = __lvx( joints1, 32 );

		__vector4 m1a1 = __lvx( joints1, 48+0 );
		__vector4 m1b1 = __lvx( joints1, 48+16 );
		__vector4 m1c1 = __lvx( joints1, 48+32 );

		__vector4 m2a0 = __lvx( joints2, 0 );
		__vector4 m2b0 = __lvx( joints2, 16 );
		__vector4 m2c0 = __lvx( joints2, 32 );

		__vector4 m2a1 = __lvx( joints2, 48+0 );
		__vector4 m2b1 = __lvx( joints2, 48+16 );
		__vector4 m2c1 = __lvx( joints2, 48+32 );

		__vector4 ta0 = __vspltw( m1a0, 0 );
		__vector4 tb0 = __vspltw( m1a0, 1 );
		__vector4 tc0 = __vspltw( m1a0, 2 );

		__vector4 td0 = __vspltw( m1b0, 0 );
		__vector4 te0 = __vspltw( m1b0, 1 );
		__vector4 tf0 = __vspltw( m1b0, 2 );

		__vector4 tg0 = __vspltw( m1c0, 0 );
		__vector4 th0 = __vspltw( m1c0, 1 );
		__vector4 ti0 = __vspltw( m1c0, 2 );

		__vector4 ta1 = __vspltw( m1a1, 0 );
		__vector4 tb1 = __vspltw( m1a1, 1 );
		__vector4 tc1 = __vspltw( m1a1, 2 );

		__vector4 td1 = __vspltw( m1b1, 0 );
		__vector4 te1 = __vspltw( m1b1, 1 );
		__vector4 tf1 = __vspltw( m1b1, 2 );

		__vector4 tg1 = __vspltw( m1c1, 0 );
		__vector4 th1 = __vspltw( m1c1, 1 );
		__vector4 ti1 = __vspltw( m1c1, 2 );

		__vector4 tj0 = __vperm( m1a0, vmx_float_zero, vmx_dword_perm_matrix );
		__vector4 tk0 = __vperm( m1b0, vmx_float_zero, vmx_dword_perm_matrix );
		__vector4 tl0 = __vperm( m1c0, vmx_float_zero, vmx_dword_perm_matrix );

		__vector4 tj1 = __vperm( m1a1, vmx_float_zero, vmx_dword_perm_matrix );
		__vector4 tk1 = __vperm( m1b1, vmx_float_zero, vmx_dword_perm_matrix );
		__vector4 tl1 = __vperm( m1c1, vmx_float_zero, vmx_dword_perm_matrix );

		__vector4 ra0 = __vmulfp( ta0, m2a0 );
		__vector4 rb0 = __vmulfp( tb0, m2b0 );
		__vector4 rc0 = __vmulfp( tc0, m2c0 );

		__vector4 rd0 = __vmulfp( td0, m2a0 );
		__vector4 re0 = __vmulfp( te0, m2b0 );
		__vector4 rf0 = __vmulfp( tf0, m2c0 );

		__vector4 rg0 = __vmulfp( tg0, m2a0 );
		__vector4 rh0 = __vmulfp( th0, m2b0 );
		__vector4 ri0 = __vmulfp( ti0, m2c0 );

		__vector4 sa0 = __vaddfp( ra0, rb0 );
		__vector4 sb0 = __vaddfp( rc0, tj0 );
		__vector4 sd0 = __vaddfp( rd0, re0 );
		__vector4 se0 = __vaddfp( rf0, tk0 );
		__vector4 sg0 = __vaddfp( rg0, rh0 );
		__vector4 sh0 = __vaddfp( ri0, tl0 );

		__vector4 ra1 = __vmulfp( ta1, m2a1 );
		__vector4 rb1 = __vmulfp( tb1, m2b1 );
		__vector4 rc1 = __vmulfp( tc1, m2c1 );

		__vector4 rd1 = __vmulfp( td1, m2a1 );
		__vector4 re1 = __vmulfp( te1, m2b1 );
		__vector4 rf1 = __vmulfp( tf1, m2c1 );

		__vector4 rg1 = __vmulfp( tg1, m2a1 );
		__vector4 rh1 = __vmulfp( th1, m2b1 );
		__vector4 ri1 = __vmulfp( ti1, m2c1 );

		__vector4 sa1 = __vaddfp( ra1, rb1 );
		__vector4 sb1 = __vaddfp( rc1, tj1 );
		__vector4 sd1 = __vaddfp( rd1, re1 );
		__vector4 se1 = __vaddfp( rf1, tk1 );
		__vector4 sg1 = __vaddfp( rg1, rh1 );
		__vector4 sh1 = __vaddfp( ri1, tl1 );

		__vector4 sc0 = __vaddfp( sa0, sb0 );
		__vector4 sf0 = __vaddfp( sd0, se0 );
		__vector4 si0 = __vaddfp( sg0, sh0 );

		__vector4 sc1 = __vaddfp( sa1, sb1 );
		__vector4 sf1 = __vaddfp( sd1, se1 );
		__vector4 si1 = __vaddfp( sg1, sh1 );

		__stvx( sc0, result, 0 );
		__stvx( sf0, result, 16 );
		__stvx( si0, result, 32 );

		__stvx( sc1, result, 48+0 );
		__stvx( sf1, result, 48+16 );
		__stvx( si1, result, 48+32 );
	}

	for ( ; result < end; result++, joints1++, joints2++ ) {
		__vector4 m1a0 = __lvx( joints1, 0 );
		__vector4 m1b0 = __lvx( joints1, 16 );
		__vector4 m1c0 = __lvx( joints1, 32 );

		__vector4 m2a0 = __lvx( joints2, 0 );
		__vector4 m2b0 = __lvx( joints2, 16 );
		__vector4 m2c0 = __lvx( joints2, 32 );

		__vector4 ta0 = __vspltw( m1a0, 0 );
		__vector4 tb0 = __vspltw( m1a0, 1 );
		__vector4 tc0 = __vspltw( m1a0, 2 );

		__vector4 td0 = __vspltw( m1b0, 0 );
		__vector4 te0 = __vspltw( m1b0, 1 );
		__vector4 tf0 = __vspltw( m1b0, 2 );

		__vector4 tg0 = __vspltw( m1c0, 0 );
		__vector4 th0 = __vspltw( m1c0, 1 );
		__vector4 ti0 = __vspltw( m1c0, 2 );

		__vector4 tj0 = __vperm( m1a0, vmx_float_zero, vmx_dword_perm_matrix );
		__vector4 tk0 = __vperm( m1b0, vmx_float_zero, vmx_dword_perm_matrix );
		__vector4 tl0 = __vperm( m1c0, vmx_float_zero, vmx_dword_perm_matrix );

		__vector4 ra0 = __vmulfp( ta0, m2a0 );
		__vector4 rb0 = __vmulfp( tb0, m2b0 );
		__vector4 rc0 = __vmulfp( tc0, m2c0 );

		__vector4 rd0 = __vmulfp( td0, m2a0 );
		__vector4 re0 = __vmulfp( te0, m2b0 );
		__vector4 rf0 = __vmulfp( tf0, m2c0 );

		__vector4 rg0 = __vmulfp( tg0, m2a0 );
		__vector4 rh0 = __vmulfp( th0, m2b0 );
		__vector4 ri0 = __vmulfp( ti0, m2c0 );

		__vector4 sa0 = __vaddfp( ra0, rb0 );
		__vector4 sb0 = __vaddfp( rc0, tj0 );
		__vector4 sc0 = __vaddfp( sa0, sb0 );

		__vector4 sd0 = __vaddfp( rd0, re0 );
		__vector4 se0 = __vaddfp( rf0, tk0 );
		__vector4 sf0 = __vaddfp( sd0, se0 );

		__vector4 sg0 = __vaddfp( rg0, rh0 );
		__vector4 sh0 = __vaddfp( ri0, tl0 );
		__vector4 si0 = __vaddfp( sg0, sh0 );

		__stvx( sc0, result, 0 );
		__stvx( sf0, result, 16 );
		__stvx( si0, result, 32 );
	}
}

/*
============
idSIMD_Xenon::TransformVerts
============
*/
void VPCALL idSIMD_Xenon::TransformVerts( idDrawVert *__restrict verts, const int numVerts, const idJointMat *__restrict joints, const idVec4 *__restrict base, const jointWeight_t *__restrict weights, const int numWeights ) {
	const byte *__restrict jointsPtr = (byte *)joints;
	const idDrawVert *__restrict end = verts + numVerts;

	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( base );
	assert_sizeof_16_byte_multiple( idJointMat );
	assert_sizeof_16_byte_multiple( idVec4 );

	for( ; verts < end; verts++, weights++, base++ ) {

		__vector4 *matrix = (__vector4 *) ( jointsPtr + weights->jointMatOffset );
		__vector4 baseVector = *(__vector4 *) base;

		__vector4 m0 = matrix[0];
		__vector4 m1 = matrix[1];
		__vector4 m2 = matrix[2];

		__vector4 vx = __vmulfp( m0, baseVector );
		__vector4 vy = __vmulfp( m1, baseVector );
		__vector4 vz = __vmulfp( m2, baseVector );

		while( weights->nextVertexOffset != JOINTWEIGHT_SIZE ) {
			weights++;
			base++;

			__vector4 *matrix = (__vector4 *) ( jointsPtr + weights->jointMatOffset );
			__vector4 baseVector = *(__vector4 *) base;

			__vector4 m0 = matrix[0];
			__vector4 m1 = matrix[1];
			__vector4 m2 = matrix[2];

			vx = __vmaddfp( m0, baseVector, vx );
			vy = __vmaddfp( m1, baseVector, vy );
			vz = __vmaddfp( m2, baseVector, vz );
		}

		float *result = verts->xyz.ToFloatPtr();

		vx = __vmsum4fp( vx, vmx_float_one );
		vy = __vmsum4fp( vy, vmx_float_one );
		vz = __vmsum4fp( vz, vmx_float_one );

		__stvewx( vx, result, 0 );
		__stvewx( vy, result, 4 );
		__stvewx( vz, result, 8 );
	}
}

/*
============
idSIMD_Xenon::TransformShadowVerts
============
*/
void VPCALL idSIMD_Xenon::TransformShadowVerts( idDrawVert *__restrict verts, const int numVerts, const idJointMat *__restrict joints, const idDrawVert *__restrict base, const jointWeight_t *__restrict weights, const int numWeights ) {
	const byte *__restrict jointsPtr = (byte *)joints;
	const byte *__restrict weightsPtr = (byte *)weights;
	const idDrawVert *__restrict end = verts + numVerts;

	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( base );
	assert_sizeof_16_byte_multiple( idJointMat );
	assert_sizeof_16_byte_multiple( idVec4 );

	for( ; verts + 7 < end; verts += 8, base += 3*8 ) {

		__vector4 *matrix0 = (__vector4 *) ( jointsPtr + ((jointWeight_t *)weightsPtr)->jointMatOffset );
		weightsPtr += ((jointWeight_t *)weightsPtr)->nextVertexOffset;
		__vector4 *matrix1 = (__vector4 *) ( jointsPtr + ((jointWeight_t *)weightsPtr)->jointMatOffset );
		weightsPtr += ((jointWeight_t *)weightsPtr)->nextVertexOffset;
		__vector4 *matrix2 = (__vector4 *) ( jointsPtr + ((jointWeight_t *)weightsPtr)->jointMatOffset );
		weightsPtr += ((jointWeight_t *)weightsPtr)->nextVertexOffset;
		__vector4 *matrix3 = (__vector4 *) ( jointsPtr + ((jointWeight_t *)weightsPtr)->jointMatOffset );
		weightsPtr += ((jointWeight_t *)weightsPtr)->nextVertexOffset;
		__vector4 *matrix4 = (__vector4 *) ( jointsPtr + ((jointWeight_t *)weightsPtr)->jointMatOffset );
		weightsPtr += ((jointWeight_t *)weightsPtr)->nextVertexOffset;
		__vector4 *matrix5 = (__vector4 *) ( jointsPtr + ((jointWeight_t *)weightsPtr)->jointMatOffset );
		weightsPtr += ((jointWeight_t *)weightsPtr)->nextVertexOffset;
		__vector4 *matrix6 = (__vector4 *) ( jointsPtr + ((jointWeight_t *)weightsPtr)->jointMatOffset );
		weightsPtr += ((jointWeight_t *)weightsPtr)->nextVertexOffset;
		__vector4 *matrix7 = (__vector4 *) ( jointsPtr + ((jointWeight_t *)weightsPtr)->jointMatOffset );
		weightsPtr += ((jointWeight_t *)weightsPtr)->nextVertexOffset;

		float *result = verts->xyz.ToFloatPtr();

		__vector4 mx0 = matrix0[0];
		__vector4 my0 = matrix0[1];
		__vector4 mz0 = matrix0[2];
		__vector4 mx1 = matrix1[0];
		__vector4 my1 = matrix1[1];
		__vector4 mz1 = matrix1[2];
		__vector4 mx2 = matrix2[0];
		__vector4 my2 = matrix2[1];
		__vector4 mz2 = matrix2[2];
		__vector4 mx3 = matrix3[0];
		__vector4 my3 = matrix3[1];
		__vector4 mz3 = matrix3[2];

		__vector4 mx4 = matrix4[0];
		__vector4 my4 = matrix4[1];
		__vector4 mz4 = matrix4[2];
		__vector4 mx5 = matrix5[0];
		__vector4 my5 = matrix5[1];
		__vector4 mz5 = matrix5[2];
		__vector4 mx6 = matrix6[0];
		__vector4 my6 = matrix6[1];
		__vector4 mz6 = matrix6[2];
		__vector4 mx7 = matrix7[0];
		__vector4 my7 = matrix7[1];
		__vector4 mz7 = matrix7[2];

		__vector4 b0 = (__vector4 &) base[0*3].xyz;
		__vector4 b1 = (__vector4 &) base[1*3].xyz;
		__vector4 b2 = (__vector4 &) base[2*3].xyz;
		__vector4 b3 = (__vector4 &) base[3*3].xyz;
		__vector4 b4 = (__vector4 &) base[4*3].xyz;
		__vector4 b5 = (__vector4 &) base[5*3].xyz;
		__vector4 b6 = (__vector4 &) base[6*3].xyz;
		__vector4 b7 = (__vector4 &) base[7*3].xyz;

		b0 = __vand( b0, vmx_dword_mask_clear_last );
		b1 = __vand( b1, vmx_dword_mask_clear_last );
		b2 = __vand( b2, vmx_dword_mask_clear_last );
		b3 = __vand( b3, vmx_dword_mask_clear_last );
		b4 = __vand( b4, vmx_dword_mask_clear_last );
		b5 = __vand( b5, vmx_dword_mask_clear_last );
		b6 = __vand( b6, vmx_dword_mask_clear_last );
		b7 = __vand( b7, vmx_dword_mask_clear_last );

		b0 = __vor( b0, vmx_float_last_one );
		b1 = __vor( b1, vmx_float_last_one );
		b2 = __vor( b2, vmx_float_last_one );
		b3 = __vor( b3, vmx_float_last_one );
		b4 = __vor( b4, vmx_float_last_one );
		b5 = __vor( b5, vmx_float_last_one );
		b6 = __vor( b6, vmx_float_last_one );
		b7 = __vor( b7, vmx_float_last_one );

		__vector4 vx0 = __vmsum4fp( mx0, b0 );
		__vector4 vy0 = __vmsum4fp( my0, b0 );
		__vector4 vz0 = __vmsum4fp( mz0, b0 );
		__vector4 vx1 = __vmsum4fp( mx1, b1 );
		__vector4 vy1 = __vmsum4fp( my1, b1 );
		__vector4 vz1 = __vmsum4fp( mz1, b1 );
		__vector4 vx2 = __vmsum4fp( mx2, b2 );
		__vector4 vy2 = __vmsum4fp( my2, b2 );
		__vector4 vz2 = __vmsum4fp( mz2, b2 );
		__vector4 vx3 = __vmsum4fp( mx3, b3 );
		__vector4 vy3 = __vmsum4fp( my3, b3 );
		__vector4 vz3 = __vmsum4fp( mz3, b3 );

		__stvewx( vx0, result, 0*DRAWVERT_SIZE+0 );
		__stvewx( vy0, result, 0*DRAWVERT_SIZE+4 );
		__stvewx( vz0, result, 0*DRAWVERT_SIZE+8 );
		__stvewx( vx1, result, 1*DRAWVERT_SIZE+0 );
		__stvewx( vy1, result, 1*DRAWVERT_SIZE+4 );
		__stvewx( vz1, result, 1*DRAWVERT_SIZE+8 );
		__stvewx( vx2, result, 2*DRAWVERT_SIZE+0 );
		__stvewx( vy2, result, 2*DRAWVERT_SIZE+4 );
		__stvewx( vz2, result, 2*DRAWVERT_SIZE+8 );
		__stvewx( vx3, result, 3*DRAWVERT_SIZE+0 );
		__stvewx( vy3, result, 3*DRAWVERT_SIZE+4 );
		__stvewx( vz3, result, 3*DRAWVERT_SIZE+8 );

		__vector4 vx4 = __vmsum4fp( mx4, b4 );
		__vector4 vy4 = __vmsum4fp( my4, b4 );
		__vector4 vz4 = __vmsum4fp( mz4, b4 );
		__vector4 vx5 = __vmsum4fp( mx5, b5 );
		__vector4 vy5 = __vmsum4fp( my5, b5 );
		__vector4 vz5 = __vmsum4fp( mz5, b5 );
		__vector4 vx6 = __vmsum4fp( mx6, b6 );
		__vector4 vy6 = __vmsum4fp( my6, b6 );
		__vector4 vz6 = __vmsum4fp( mz6, b6 );
		__vector4 vx7 = __vmsum4fp( mx7, b7 );
		__vector4 vy7 = __vmsum4fp( my7, b7 );
		__vector4 vz7 = __vmsum4fp( mz7, b7 );

		__stvewx( vx4, result, 4*DRAWVERT_SIZE+0 );
		__stvewx( vy4, result, 4*DRAWVERT_SIZE+4 );
		__stvewx( vz4, result, 4*DRAWVERT_SIZE+8 );
		__stvewx( vx5, result, 5*DRAWVERT_SIZE+0 );
		__stvewx( vy5, result, 5*DRAWVERT_SIZE+4 );
		__stvewx( vz5, result, 5*DRAWVERT_SIZE+8 );
		__stvewx( vx6, result, 6*DRAWVERT_SIZE+0 );
		__stvewx( vy6, result, 6*DRAWVERT_SIZE+4 );
		__stvewx( vz6, result, 6*DRAWVERT_SIZE+8 );
		__stvewx( vx7, result, 7*DRAWVERT_SIZE+0 );
		__stvewx( vy7, result, 7*DRAWVERT_SIZE+4 );
		__stvewx( vz7, result, 7*DRAWVERT_SIZE+8 );
	}

	for( ; verts < end; verts++, base += 3 ) {
		__vector4 *matrix = (__vector4 *) ( jointsPtr + ((jointWeight_t *)weightsPtr)->jointMatOffset );
		__vector4 baseVector = *(__vector4 *) base;

		weightsPtr += ((jointWeight_t *)weightsPtr)->nextVertexOffset;

		float *result = verts->xyz.ToFloatPtr();

		__vector4 mx = matrix[0];
		__vector4 my = matrix[1];
		__vector4 mz = matrix[2];

		__vector4 vx = __vmsum4fp( mx, baseVector );
		__vector4 vy = __vmsum4fp( my, baseVector );
		__vector4 vz = __vmsum4fp( mz, baseVector );

		__stvewx( vx, result, 0 );
		__stvewx( vy, result, 4 );
		__stvewx( vz, result, 8 );
	}
}

/*
============
idSIMD_Xenon::TracePointCull
============
*/
void VPCALL idSIMD_Xenon::TracePointCull( byte *__restrict cullBits, byte &totalOr, const float radius, const idPlane *__restrict planes, const idDrawVert *__restrict verts, const int numVerts ) {
	int i;
	byte tOr;

	tOr = 0;

	__vector4 px = { planes[0][0], planes[1][0], planes[2][0], planes[3][0] };
	__vector4 py = { planes[0][1], planes[1][1], planes[2][1], planes[3][1] };
	__vector4 pz = { planes[0][2], planes[1][2], planes[2][2], planes[3][2] };
	__vector4 pw = { planes[0][3], planes[1][3], planes[2][3], planes[3][3] };
	__vector4 vradius = { radius, radius, radius, radius };
	__vector4 vOr = vmx_byte_zero;

	for ( i = 0; i < numVerts - 7; i += 8 ) {

		__vector4 va_0 = *(__vector4 *)( &verts[i+0].xyz );
		__vector4 vb_0 = *(__vector4 *)( &verts[i+1].xyz );
		__vector4 vc_0 = *(__vector4 *)( &verts[i+2].xyz );
		__vector4 vd_0 = *(__vector4 *)( &verts[i+3].xyz );
		__vector4 va_1 = *(__vector4 *)( &verts[i+4].xyz );
		__vector4 vb_1 = *(__vector4 *)( &verts[i+5].xyz );
		__vector4 vc_1 = *(__vector4 *)( &verts[i+6].xyz );
		__vector4 vd_1 = *(__vector4 *)( &verts[i+7].xyz );

		__vector4 va0_0 = __vpermwi( va_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 va1_0 = __vpermwi( va_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 va2_0 = __vpermwi( va_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 va0_1 = __vpermwi( va_1, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 va1_1 = __vpermwi( va_1, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 va2_1 = __vpermwi( va_1, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vb0_0 = __vpermwi( vb_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vb1_0 = __vpermwi( vb_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vb2_0 = __vpermwi( vb_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 vb0_1 = __vpermwi( vb_1, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vb1_1 = __vpermwi( vb_1, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vb2_1 = __vpermwi( vb_1, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vc0_0 = __vpermwi( vc_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vc1_0 = __vpermwi( vc_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vc2_0 = __vpermwi( vc_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 vc0_1 = __vpermwi( vc_1, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vc1_1 = __vpermwi( vc_1, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vc2_1 = __vpermwi( vc_1, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vd0_0 = __vpermwi( vd_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vd1_0 = __vpermwi( vd_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vd2_0 = __vpermwi( vd_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 vd0_1 = __vpermwi( vd_1, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vd1_1 = __vpermwi( vd_1, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vd2_1 = __vpermwi( vd_1, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 da_0 = __vmaddfp( va0_0, px, pw );
		__vector4 db_0 = __vmaddfp( vb0_0, px, pw );
		__vector4 dc_0 = __vmaddfp( vc0_0, px, pw );
		__vector4 dd_0 = __vmaddfp( vd0_0, px, pw );
		__vector4 da_1 = __vmaddfp( va0_1, px, pw );
		__vector4 db_1 = __vmaddfp( vb0_1, px, pw );
		__vector4 dc_1 = __vmaddfp( vc0_1, px, pw );
		__vector4 dd_1 = __vmaddfp( vd0_1, px, pw );

		da_0 = __vmaddfp( va1_0, py, da_0 );
		db_0 = __vmaddfp( vb1_0, py, db_0 );
		dc_0 = __vmaddfp( vc1_0, py, dc_0 );
		dd_0 = __vmaddfp( vd1_0, py, dd_0 );
		da_1 = __vmaddfp( va1_1, py, da_1 );
		db_1 = __vmaddfp( vb1_1, py, db_1 );
		dc_1 = __vmaddfp( vc1_1, py, dc_1 );
		dd_1 = __vmaddfp( vd1_1, py, dd_1 );

		da_0 = __vmaddfp( va2_0, pz, da_0 );
		db_0 = __vmaddfp( vb2_0, pz, db_0 );
		dc_0 = __vmaddfp( vc2_0, pz, dc_0 );
		dd_0 = __vmaddfp( vd2_0, pz, dd_0 );
		da_1 = __vmaddfp( va2_1, pz, da_1 );
		db_1 = __vmaddfp( vb2_1, pz, db_1 );
		dc_1 = __vmaddfp( vc2_1, pz, dc_1 );
		dd_1 = __vmaddfp( vd2_1, pz, dd_1 );

		__vector4 ta_0 = __vaddfp( da_0, vradius );
		__vector4 tb_0 = __vaddfp( db_0, vradius );
		__vector4 tc_0 = __vaddfp( dc_0, vradius );
		__vector4 td_0 = __vaddfp( dd_0, vradius );
		__vector4 ta_1 = __vaddfp( da_1, vradius );
		__vector4 tb_1 = __vaddfp( db_1, vradius );
		__vector4 tc_1 = __vaddfp( dc_1, vradius );
		__vector4 td_1 = __vaddfp( dd_1, vradius );

		__vector4 sa_0 = __vsubfp( da_0, vradius );
		__vector4 sb_0 = __vsubfp( db_0, vradius );
		__vector4 sc_0 = __vsubfp( dc_0, vradius );
		__vector4 sd_0 = __vsubfp( dd_0, vradius );
		__vector4 sa_1 = __vsubfp( da_1, vradius );
		__vector4 sb_1 = __vsubfp( db_1, vradius );
		__vector4 sc_1 = __vsubfp( dc_1, vradius );
		__vector4 sd_1 = __vsubfp( dd_1, vradius );

		ta_0 = __vcmpgtfp( ta_0, vmx_float_zero );
		tb_0 = __vcmpgtfp( tb_0, vmx_float_zero );
		tc_0 = __vcmpgtfp( tc_0, vmx_float_zero );
		td_0 = __vcmpgtfp( td_0, vmx_float_zero );
		ta_1 = __vcmpgtfp( ta_1, vmx_float_zero );
		tb_1 = __vcmpgtfp( tb_1, vmx_float_zero );
		tc_1 = __vcmpgtfp( tc_1, vmx_float_zero );
		td_1 = __vcmpgtfp( td_1, vmx_float_zero );

		sa_0 = __vcmpgtfp( sa_0, vmx_float_zero );
		sb_0 = __vcmpgtfp( sb_0, vmx_float_zero );
		sc_0 = __vcmpgtfp( sc_0, vmx_float_zero );
		sd_0 = __vcmpgtfp( sd_0, vmx_float_zero );
		sa_1 = __vcmpgtfp( sa_1, vmx_float_zero );
		sb_1 = __vcmpgtfp( sb_1, vmx_float_zero );
		sc_1 = __vcmpgtfp( sc_1, vmx_float_zero );
		sd_1 = __vcmpgtfp( sd_1, vmx_float_zero );

		ta_0 = __vand( ta_0, vmx_dword_trace_mask0 );
		tb_0 = __vand( tb_0, vmx_dword_trace_mask1 );
		tc_0 = __vand( tc_0, vmx_dword_trace_mask2 );
		td_0 = __vand( td_0, vmx_dword_trace_mask3 );
		ta_1 = __vand( ta_1, vmx_dword_trace_mask0 );
		tb_1 = __vand( tb_1, vmx_dword_trace_mask1 );
		tc_1 = __vand( tc_1, vmx_dword_trace_mask2 );
		td_1 = __vand( td_1, vmx_dword_trace_mask3 );

		sa_0 = __vand( sa_0, vmx_dword_trace_mask4 );
		sb_0 = __vand( sb_0, vmx_dword_trace_mask5 );
		sc_0 = __vand( sc_0, vmx_dword_trace_mask6 );
		sd_0 = __vand( sd_0, vmx_dword_trace_mask7 );
		sa_1 = __vand( sa_1, vmx_dword_trace_mask4 );
		sb_1 = __vand( sb_1, vmx_dword_trace_mask5 );
		sc_1 = __vand( sc_1, vmx_dword_trace_mask6 );
		sd_1 = __vand( sd_1, vmx_dword_trace_mask7 );

		ta_0 = __vor( ta_0, sa_0 );
		tb_0 = __vor( tb_0, sb_0 );
		tc_0 = __vor( tc_0, sc_0 );
		td_0 = __vor( td_0, sd_0 );
		ta_1 = __vor( ta_1, sa_1 );
		tb_1 = __vor( tb_1, sb_1 );
		tc_1 = __vor( tc_1, sc_1 );
		td_1 = __vor( td_1, sd_1 );

		ta_0 = __vor( ta_0, tb_0 );
		tc_0 = __vor( tc_0, td_0 );
		ta_1 = __vor( ta_1, tb_1 );
		tc_1 = __vor( tc_1, td_1 );

		ta_0 = __vor( ta_0, tc_0 );
		ta_1 = __vor( ta_1, tc_1 );

		__vector4 bits0_0 = __vpermwi( ta_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 bits1_0 = __vpermwi( ta_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 bits2_0 = __vpermwi( ta_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 bits3_0 = __vpermwi( ta_0, SHUFFLE_D( 3, 3, 3, 3 ) );
		__vector4 bits0_1 = __vpermwi( ta_1, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 bits1_1 = __vpermwi( ta_1, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 bits2_1 = __vpermwi( ta_1, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 bits3_1 = __vpermwi( ta_1, SHUFFLE_D( 3, 3, 3, 3 ) );

		bits0_0 = __vor( bits0_0, bits1_0 );
		bits2_0 = __vor( bits2_0, bits3_0 );
		bits0_0 = __vor( bits0_0, bits2_0 );
		bits0_1 = __vor( bits0_1, bits1_1 );
		bits2_1 = __vor( bits2_1, bits3_1 );
		bits0_1 = __vor( bits0_1, bits2_1 );

		bits0_0 = __vxor( bits0_0, vmx_dword_trace_xor );
		bits0_1 = __vxor( bits0_1, vmx_dword_trace_xor );

		vOr = __vor( vOr, bits0_0 );
		vOr = __vor( vOr, bits0_1 );

		bits0_0 = __vperm( bits0_0, bits0_0, vmx_dword_overlay_perm );
		bits0_1 = __vperm( bits0_1, bits0_1, vmx_dword_overlay_perm );

		__stvewx( bits0_0, cullBits, i );
		__stvewx( bits0_1, cullBits, i+4 );
	}

	for ( ; i < numVerts - 3; i += 4 ) {

		__vector4 va_0 = *(__vector4 *)( &verts[i+0].xyz );
		__vector4 vb_0 = *(__vector4 *)( &verts[i+1].xyz );
		__vector4 vc_0 = *(__vector4 *)( &verts[i+2].xyz );
		__vector4 vd_0 = *(__vector4 *)( &verts[i+3].xyz );

		__vector4 va0_0 = __vpermwi( va_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 va1_0 = __vpermwi( va_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 va2_0 = __vpermwi( va_0, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vb0_0 = __vpermwi( vb_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vb1_0 = __vpermwi( vb_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vb2_0 = __vpermwi( vb_0, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vc0_0 = __vpermwi( vc_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vc1_0 = __vpermwi( vc_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vc2_0 = __vpermwi( vc_0, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vd0_0 = __vpermwi( vd_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vd1_0 = __vpermwi( vd_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vd2_0 = __vpermwi( vd_0, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 da_0 = __vmaddfp( va0_0, px, pw );
		__vector4 db_0 = __vmaddfp( vb0_0, px, pw );
		__vector4 dc_0 = __vmaddfp( vc0_0, px, pw );
		__vector4 dd_0 = __vmaddfp( vd0_0, px, pw );

		da_0 = __vmaddfp( va1_0, py, da_0 );
		db_0 = __vmaddfp( vb1_0, py, db_0 );
		dc_0 = __vmaddfp( vc1_0, py, dc_0 );
		dd_0 = __vmaddfp( vd1_0, py, dd_0 );

		da_0 = __vmaddfp( va2_0, pz, da_0 );
		db_0 = __vmaddfp( vb2_0, pz, db_0 );
		dc_0 = __vmaddfp( vc2_0, pz, dc_0 );
		dd_0 = __vmaddfp( vd2_0, pz, dd_0 );

		__vector4 ta_0 = __vaddfp( da_0, vradius );
		__vector4 tb_0 = __vaddfp( db_0, vradius );
		__vector4 tc_0 = __vaddfp( dc_0, vradius );
		__vector4 td_0 = __vaddfp( dd_0, vradius );

		__vector4 sa_0 = __vsubfp( da_0, vradius );
		__vector4 sb_0 = __vsubfp( db_0, vradius );
		__vector4 sc_0 = __vsubfp( dc_0, vradius );
		__vector4 sd_0 = __vsubfp( dd_0, vradius );

		ta_0 = __vcmpgtfp( ta_0, vmx_float_zero );
		tb_0 = __vcmpgtfp( tb_0, vmx_float_zero );
		tc_0 = __vcmpgtfp( tc_0, vmx_float_zero );
		td_0 = __vcmpgtfp( td_0, vmx_float_zero );

		sa_0 = __vcmpgtfp( sa_0, vmx_float_zero );
		sb_0 = __vcmpgtfp( sb_0, vmx_float_zero );
		sc_0 = __vcmpgtfp( sc_0, vmx_float_zero );
		sd_0 = __vcmpgtfp( sd_0, vmx_float_zero );

		ta_0 = __vand( ta_0, vmx_dword_trace_mask0 );
		tb_0 = __vand( tb_0, vmx_dword_trace_mask1 );
		tc_0 = __vand( tc_0, vmx_dword_trace_mask2 );
		td_0 = __vand( td_0, vmx_dword_trace_mask3 );

		sa_0 = __vand( sa_0, vmx_dword_trace_mask4 );
		sb_0 = __vand( sb_0, vmx_dword_trace_mask5 );
		sc_0 = __vand( sc_0, vmx_dword_trace_mask6 );
		sd_0 = __vand( sd_0, vmx_dword_trace_mask7 );

		ta_0 = __vor( ta_0, sa_0 );
		tb_0 = __vor( tb_0, sb_0 );
		tc_0 = __vor( tc_0, sc_0 );
		td_0 = __vor( td_0, sd_0 );

		ta_0 = __vor( ta_0, tb_0 );
		tc_0 = __vor( tc_0, td_0 );

		ta_0 = __vor( ta_0, tc_0 );

		__vector4 bits0_0 = __vpermwi( ta_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 bits1_0 = __vpermwi( ta_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 bits2_0 = __vpermwi( ta_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 bits3_0 = __vpermwi( ta_0, SHUFFLE_D( 3, 3, 3, 3 ) );

		bits0_0 = __vor( bits0_0, bits1_0 );
		bits2_0 = __vor( bits2_0, bits3_0 );
		bits0_0 = __vor( bits0_0, bits2_0 );

		bits0_0 = __vxor( bits0_0, vmx_dword_trace_xor );

		vOr = __vor( vOr, bits0_0 );

		bits0_0 = __vperm( bits0_0, bits0_0, vmx_dword_overlay_perm );

		__stvewx( bits0_0, cullBits, i );
	}

	for ( ; i < numVerts - 1; i += 2 ) {
		byte bits0, bits1;

		const idVec3 &v0 = verts[i+0].xyz;
		const idVec3 &v1 = verts[i+1].xyz;

		float d0  = planes[0][0] * v0[0] + planes[0][1] * v0[1] + planes[0][2] * v0[2] + planes[0][3];
		float d1  = planes[1][0] * v0[0] + planes[1][1] * v0[1] + planes[1][2] * v0[2] + planes[1][3];
		float d2  = planes[2][0] * v0[0] + planes[2][1] * v0[1] + planes[2][2] * v0[2] + planes[2][3];
		float d3  = planes[3][0] * v0[0] + planes[3][1] * v0[1] + planes[3][2] * v0[2] + planes[3][3];

		float d4  = planes[0][0] * v1[0] + planes[0][1] * v1[1] + planes[0][2] * v1[2] + planes[0][3];
		float d5  = planes[1][0] * v1[0] + planes[1][1] * v1[1] + planes[1][2] * v1[2] + planes[1][3];
		float d6  = planes[2][0] * v1[0] + planes[2][1] * v1[1] + planes[2][2] * v1[2] + planes[2][3];
		float d7  = planes[3][0] * v1[0] + planes[3][1] * v1[1] + planes[3][2] * v1[2] + planes[3][3];

		float t0  = d0  + radius;
		float t1  = d1  + radius;
		float t2  = d2  + radius;
		float t3  = d3  + radius;

		float s0  = d0  - radius;
		float s1  = d1  - radius;
		float s2  = d2  - radius;
		float s3  = d3  - radius;

		float t4  = d4  + radius;
		float t5  = d5  + radius;
		float t6  = d6  + radius;
		float t7  = d7  + radius;

		float s4  = d4  - radius;
		float s5  = d5  - radius;
		float s6  = d6  - radius;
		float s7  = d7  - radius;

		bits0  = FLOATSIGNBITSET( t0 ) << 0;
		bits0 |= FLOATSIGNBITSET( t1 ) << 1;
		bits0 |= FLOATSIGNBITSET( t2 ) << 2;
		bits0 |= FLOATSIGNBITSET( t3 ) << 3;
		bits0 |= FLOATSIGNBITSET( s0 ) << 4;
		bits0 |= FLOATSIGNBITSET( t1 ) << 5;
		bits0 |= FLOATSIGNBITSET( t2 ) << 6;
		bits0 |= FLOATSIGNBITSET( t3 ) << 7;

		bits1  = FLOATSIGNBITSET( t4 ) << 0;
		bits1 |= FLOATSIGNBITSET( t5 ) << 1;
		bits1 |= FLOATSIGNBITSET( t6 ) << 2;
		bits1 |= FLOATSIGNBITSET( t7 ) << 3;
		bits1 |= FLOATSIGNBITSET( s4 ) << 4;
		bits1 |= FLOATSIGNBITSET( t5 ) << 5;
		bits1 |= FLOATSIGNBITSET( t6 ) << 6;
		bits1 |= FLOATSIGNBITSET( t7 ) << 7;

		bits0 ^= 0x0F;		// flip lower four bits
		bits1 ^= 0x0F;		// flip lower four bits

		tOr |= bits0;
		tOr |= bits1;

		cullBits[i+0] = bits0;
		cullBits[i+1] = bits1;
	}

	for ( ; i < numVerts; i++ ) {
		byte bits;

		const idVec3 &v = verts[i].xyz;

		float d0 = planes[0][0] * v[0] + planes[0][1] * v[1] + planes[0][2] * v[2] + planes[0][3];
		float d1 = planes[1][0] * v[0] + planes[1][1] * v[1] + planes[1][2] * v[2] + planes[1][3];
		float d2 = planes[2][0] * v[0] + planes[2][1] * v[1] + planes[2][2] * v[2] + planes[2][3];
		float d3 = planes[3][0] * v[0] + planes[3][1] * v[1] + planes[3][2] * v[2] + planes[3][3];

		float t0 = d0 + radius;
		float t1 = d1 + radius;
		float t2 = d2 + radius;
		float t3 = d3 + radius;
		float t4 = d0 - radius;
		float t5 = d1 - radius;
		float t6 = d2 - radius;
		float t7 = d3 - radius;

		bits  = FLOATSIGNBITSET( t0 ) << 0;
		bits |= FLOATSIGNBITSET( t1 ) << 1;
		bits |= FLOATSIGNBITSET( t2 ) << 2;
		bits |= FLOATSIGNBITSET( t3 ) << 3;
		bits |= FLOATSIGNBITSET( t4 ) << 4;
		bits |= FLOATSIGNBITSET( t5 ) << 5;
		bits |= FLOATSIGNBITSET( t6 ) << 6;
		bits |= FLOATSIGNBITSET( t7 ) << 7;

		bits ^= 0x0F;		// flip lower four bits

		tOr |= bits;
		cullBits[i] = bits;
	}

	byte *ptr = (byte *)&vOr;

	totalOr = tOr | ptr[0] | ptr[1] | ptr[2] | ptr[3];
}

/*
============
idSIMD_Xenon::DecalPointCull
============
*/
void VPCALL idSIMD_Xenon::DecalPointCull( byte *__restrict cullBits, const idPlane *__restrict planes, const idDrawVert *__restrict verts, const int numVerts ) {
	int i;

	__vector4 p0x = { planes[0][0], planes[1][0], planes[2][0], planes[3][0] };
	__vector4 p0y = { planes[0][1], planes[1][1], planes[2][1], planes[3][1] };
	__vector4 p0z = { planes[0][2], planes[1][2], planes[2][2], planes[3][2] };
	__vector4 p0w = { planes[0][3], planes[1][3], planes[2][3], planes[3][3] };

	__vector4 p1x = { planes[4][0], planes[5][0], planes[4][0], planes[5][0] };
	__vector4 p1y = { planes[4][1], planes[5][1], planes[4][1], planes[5][1] };
	__vector4 p1z = { planes[4][2], planes[5][2], planes[4][2], planes[5][2] };
	__vector4 p1w = { planes[4][3], planes[5][3], planes[4][3], planes[5][3] };

	for ( i = 0; i < numVerts - 7; i += 8 ) {

		__vector4 va_0 = *(__vector4 *)( &verts[i+0].xyz );
		__vector4 vb_0 = *(__vector4 *)( &verts[i+1].xyz );
		__vector4 vc_0 = *(__vector4 *)( &verts[i+2].xyz );
		__vector4 vd_0 = *(__vector4 *)( &verts[i+3].xyz );
		__vector4 va_1 = *(__vector4 *)( &verts[i+4].xyz );
		__vector4 vb_1 = *(__vector4 *)( &verts[i+5].xyz );
		__vector4 vc_1 = *(__vector4 *)( &verts[i+6].xyz );
		__vector4 vd_1 = *(__vector4 *)( &verts[i+7].xyz );

		__vector4 ta_0 = __vmrghw( va_0, vb_0 );
		__vector4 tb_0 = __vmrghw( vc_0, vd_0 );
		__vector4 tc_0 = __vmrglw( va_0, vb_0 );
		__vector4 td_0 = __vmrglw( vc_0, vd_0 );
		__vector4 ta_1 = __vmrghw( va_1, vb_1 );
		__vector4 tb_1 = __vmrghw( vc_1, vd_1 );
		__vector4 tc_1 = __vmrglw( va_1, vb_1 );
		__vector4 td_1 = __vmrglw( vc_1, vd_1 );

		__vector4 va0_0 = __vpermwi( va_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 va1_0 = __vpermwi( va_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 va2_0 = __vpermwi( va_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 va0_1 = __vpermwi( va_1, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 va1_1 = __vpermwi( va_1, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 va2_1 = __vpermwi( va_1, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vb0_0 = __vpermwi( vb_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vb1_0 = __vpermwi( vb_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vb2_0 = __vpermwi( vb_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 vb0_1 = __vpermwi( vb_1, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vb1_1 = __vpermwi( vb_1, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vb2_1 = __vpermwi( vb_1, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vc0_0 = __vpermwi( vc_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vc1_0 = __vpermwi( vc_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vc2_0 = __vpermwi( vc_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 vc0_1 = __vpermwi( vc_1, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vc1_1 = __vpermwi( vc_1, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vc2_1 = __vpermwi( vc_1, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vd0_0 = __vpermwi( vd_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vd1_0 = __vpermwi( vd_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vd2_0 = __vpermwi( vd_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 vd0_1 = __vpermwi( vd_1, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vd1_1 = __vpermwi( vd_1, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vd2_1 = __vpermwi( vd_1, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 ve0_0 = __vpermwi( ta_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 ve1_0 = __vpermwi( ta_0, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 ve2_0 = __vpermwi( tc_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 ve0_1 = __vpermwi( ta_1, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 ve1_1 = __vpermwi( ta_1, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 ve2_1 = __vpermwi( tc_1, SHUFFLE_D( 0, 0, 1, 1 ) );

		__vector4 vf0_0 = __vpermwi( tb_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 vf1_0 = __vpermwi( tb_0, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 vf2_0 = __vpermwi( td_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 vf0_1 = __vpermwi( tb_1, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 vf1_1 = __vpermwi( tb_1, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 vf2_1 = __vpermwi( td_1, SHUFFLE_D( 0, 0, 1, 1 ) );

		__vector4 da_0 = __vmaddfp( va0_0, p0x, p0w );
		__vector4 db_0 = __vmaddfp( vb0_0, p0x, p0w );
		__vector4 dc_0 = __vmaddfp( vc0_0, p0x, p0w );
		__vector4 dd_0 = __vmaddfp( vd0_0, p0x, p0w );
		__vector4 de_0 = __vmaddfp( ve0_0, p1x, p1w );
		__vector4 df_0 = __vmaddfp( vf0_0, p1x, p1w );
		__vector4 da_1 = __vmaddfp( va0_1, p0x, p0w );
		__vector4 db_1 = __vmaddfp( vb0_1, p0x, p0w );
		__vector4 dc_1 = __vmaddfp( vc0_1, p0x, p0w );
		__vector4 dd_1 = __vmaddfp( vd0_1, p0x, p0w );
		__vector4 de_1 = __vmaddfp( ve0_1, p1x, p1w );
		__vector4 df_1 = __vmaddfp( vf0_1, p1x, p1w );

		da_0 = __vmaddfp( va1_0, p0y, da_0 );
		db_0 = __vmaddfp( vb1_0, p0y, db_0 );
		dc_0 = __vmaddfp( vc1_0, p0y, dc_0 );
		dd_0 = __vmaddfp( vd1_0, p0y, dd_0 );
		de_0 = __vmaddfp( ve1_0, p1y, de_0 );
		df_0 = __vmaddfp( vf1_0, p1y, df_0 );
		da_1 = __vmaddfp( va1_1, p0y, da_1 );
		db_1 = __vmaddfp( vb1_1, p0y, db_1 );
		dc_1 = __vmaddfp( vc1_1, p0y, dc_1 );
		dd_1 = __vmaddfp( vd1_1, p0y, dd_1 );
		de_1 = __vmaddfp( ve1_1, p1y, de_1 );
		df_1 = __vmaddfp( vf1_1, p1y, df_1 );

		da_0 = __vmaddfp( va2_0, p0z, da_0 );
		db_0 = __vmaddfp( vb2_0, p0z, db_0 );
		dc_0 = __vmaddfp( vc2_0, p0z, dc_0 );
		dd_0 = __vmaddfp( vd2_0, p0z, dd_0 );
		de_0 = __vmaddfp( ve2_0, p1z, de_0 );
		df_0 = __vmaddfp( vf2_0, p1z, df_0 );
		da_1 = __vmaddfp( va2_1, p0z, da_1 );
		db_1 = __vmaddfp( vb2_1, p0z, db_1 );
		dc_1 = __vmaddfp( vc2_1, p0z, dc_1 );
		dd_1 = __vmaddfp( vd2_1, p0z, dd_1 );
		de_1 = __vmaddfp( ve2_1, p1z, de_1 );
		df_1 = __vmaddfp( vf2_1, p1z, df_1 );

		da_0 = __vcmpgtfp( da_0, vmx_float_zero );
		db_0 = __vcmpgtfp( db_0, vmx_float_zero );
		dc_0 = __vcmpgtfp( dc_0, vmx_float_zero );
		dd_0 = __vcmpgtfp( dd_0, vmx_float_zero );
		de_0 = __vcmpgtfp( de_0, vmx_float_zero );
		df_0 = __vcmpgtfp( df_0, vmx_float_zero );
		da_1 = __vcmpgtfp( da_1, vmx_float_zero );
		db_1 = __vcmpgtfp( db_1, vmx_float_zero );
		dc_1 = __vcmpgtfp( dc_1, vmx_float_zero );
		dd_1 = __vcmpgtfp( dd_1, vmx_float_zero );
		de_1 = __vcmpgtfp( de_1, vmx_float_zero );
		df_1 = __vcmpgtfp( df_1, vmx_float_zero );

		da_0 = __vand( da_0, vmx_dword_decal_mask0 );		//  0  1  2  3
		db_0 = __vand( db_0, vmx_dword_decal_mask1 );		//  8  9 10 11
		dc_0 = __vand( dc_0, vmx_dword_decal_mask2 );		// 16 17 18 19
		dd_0 = __vand( dd_0, vmx_dword_decal_mask3 );		// 24 25 26 27
		de_0 = __vand( de_0, vmx_dword_decal_mask4 );		//  4  5 12 13
		df_0 = __vand( df_0, vmx_dword_decal_mask5 );		// 20 21 28 29
		da_1 = __vand( da_1, vmx_dword_decal_mask0 );		//  0  1  2  3
		db_1 = __vand( db_1, vmx_dword_decal_mask1 );		//  8  9 10 11
		dc_1 = __vand( dc_1, vmx_dword_decal_mask2 );		// 16 17 18 19
		dd_1 = __vand( dd_1, vmx_dword_decal_mask3 );		// 24 25 26 27
		de_1 = __vand( de_1, vmx_dword_decal_mask4 );		//  4  5 12 13
		df_1 = __vand( df_1, vmx_dword_decal_mask5 );		// 20 21 28 29

		da_0 = __vor( da_0, db_0 );
		dc_0 = __vor( dc_0, dd_0 );
		de_0 = __vor( de_0, df_0 );
		da_1 = __vor( da_1, db_1 );
		dc_1 = __vor( dc_1, dd_1 );
		de_1 = __vor( de_1, df_1 );

		da_0 = __vor( da_0, dc_0 );
		da_0 = __vor( da_0, de_0 );
		da_1 = __vor( da_1, dc_1 );
		da_1 = __vor( da_1, de_1 );

		__vector4 bits0_0 = __vpermwi( da_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 bits1_0 = __vpermwi( da_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 bits2_0 = __vpermwi( da_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 bits3_0 = __vpermwi( da_0, SHUFFLE_D( 3, 3, 3, 3 ) );
		__vector4 bits0_1 = __vpermwi( da_1, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 bits1_1 = __vpermwi( da_1, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 bits2_1 = __vpermwi( da_1, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 bits3_1 = __vpermwi( da_1, SHUFFLE_D( 3, 3, 3, 3 ) );

		bits0_0 = __vor( bits0_0, bits1_0 );
		bits2_0 = __vor( bits2_0, bits3_0 );
		bits0_0 = __vor( bits0_0, bits2_0 );
		bits0_1 = __vor( bits0_1, bits1_1 );
		bits2_1 = __vor( bits2_1, bits3_1 );
		bits0_1 = __vor( bits0_1, bits2_1 );

		bits0_0 = __vperm( bits0_0, bits0_0, vmx_dword_overlay_perm );
		bits0_1 = __vperm( bits0_1, bits0_1, vmx_dword_overlay_perm );

		__stvewx( bits0_0, cullBits, i );
		__stvewx( bits0_1, cullBits, i+4 );
	}

	for ( ; i < numVerts - 3; i += 4 ) {

		__vector4 va_0 = *(__vector4 *)( &verts[i+0].xyz );
		__vector4 vb_0 = *(__vector4 *)( &verts[i+1].xyz );
		__vector4 vc_0 = *(__vector4 *)( &verts[i+2].xyz );
		__vector4 vd_0 = *(__vector4 *)( &verts[i+3].xyz );

		__vector4 ta_0 = __vmrghw( va_0, vb_0 );
		__vector4 tb_0 = __vmrghw( vc_0, vd_0 );
		__vector4 tc_0 = __vmrglw( va_0, vb_0 );
		__vector4 td_0 = __vmrglw( vc_0, vd_0 );

		__vector4 va0_0 = __vpermwi( va_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 va1_0 = __vpermwi( va_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 va2_0 = __vpermwi( va_0, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vb0_0 = __vpermwi( vb_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vb1_0 = __vpermwi( vb_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vb2_0 = __vpermwi( vb_0, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vc0_0 = __vpermwi( vc_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vc1_0 = __vpermwi( vc_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vc2_0 = __vpermwi( vc_0, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 vd0_0 = __vpermwi( vd_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 vd1_0 = __vpermwi( vd_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 vd2_0 = __vpermwi( vd_0, SHUFFLE_D( 2, 2, 2, 2 ) );

		__vector4 ve0_0 = __vpermwi( ta_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 ve1_0 = __vpermwi( ta_0, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 ve2_0 = __vpermwi( tc_0, SHUFFLE_D( 0, 0, 1, 1 ) );

		__vector4 vf0_0 = __vpermwi( tb_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 vf1_0 = __vpermwi( tb_0, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 vf2_0 = __vpermwi( td_0, SHUFFLE_D( 0, 0, 1, 1 ) );

		__vector4 da_0 = __vmaddfp( va0_0, p0x, p0w );
		__vector4 db_0 = __vmaddfp( vb0_0, p0x, p0w );
		__vector4 dc_0 = __vmaddfp( vc0_0, p0x, p0w );
		__vector4 dd_0 = __vmaddfp( vd0_0, p0x, p0w );
		__vector4 de_0 = __vmaddfp( ve0_0, p1x, p1w );
		__vector4 df_0 = __vmaddfp( vf0_0, p1x, p1w );

		da_0 = __vmaddfp( va1_0, p0y, da_0 );
		db_0 = __vmaddfp( vb1_0, p0y, db_0 );
		dc_0 = __vmaddfp( vc1_0, p0y, dc_0 );
		dd_0 = __vmaddfp( vd1_0, p0y, dd_0 );
		de_0 = __vmaddfp( ve1_0, p1y, de_0 );
		df_0 = __vmaddfp( vf1_0, p1y, df_0 );

		da_0 = __vmaddfp( va2_0, p0z, da_0 );
		db_0 = __vmaddfp( vb2_0, p0z, db_0 );
		dc_0 = __vmaddfp( vc2_0, p0z, dc_0 );
		dd_0 = __vmaddfp( vd2_0, p0z, dd_0 );
		de_0 = __vmaddfp( ve2_0, p1z, de_0 );
		df_0 = __vmaddfp( vf2_0, p1z, df_0 );

		da_0 = __vcmpgtfp( da_0, vmx_float_zero );
		db_0 = __vcmpgtfp( db_0, vmx_float_zero );
		dc_0 = __vcmpgtfp( dc_0, vmx_float_zero );
		dd_0 = __vcmpgtfp( dd_0, vmx_float_zero );
		de_0 = __vcmpgtfp( de_0, vmx_float_zero );
		df_0 = __vcmpgtfp( df_0, vmx_float_zero );

		da_0 = __vand( da_0, vmx_dword_decal_mask0 );		//  0  1  2  3
		db_0 = __vand( db_0, vmx_dword_decal_mask1 );		//  8  9 10 11
		dc_0 = __vand( dc_0, vmx_dword_decal_mask2 );		// 16 17 18 19
		dd_0 = __vand( dd_0, vmx_dword_decal_mask3 );		// 24 25 26 27
		de_0 = __vand( de_0, vmx_dword_decal_mask4 );		//  4  5 12 13
		df_0 = __vand( df_0, vmx_dword_decal_mask5 );		// 20 21 28 29

		da_0 = __vor( da_0, db_0 );
		dc_0 = __vor( dc_0, dd_0 );
		de_0 = __vor( de_0, df_0 );

		da_0 = __vor( da_0, dc_0 );
		da_0 = __vor( da_0, de_0 );

		__vector4 bits0_0 = __vpermwi( da_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 bits1_0 = __vpermwi( da_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 bits2_0 = __vpermwi( da_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 bits3_0 = __vpermwi( da_0, SHUFFLE_D( 3, 3, 3, 3 ) );

		bits0_0 = __vor( bits0_0, bits1_0 );
		bits2_0 = __vor( bits2_0, bits3_0 );
		bits0_0 = __vor( bits0_0, bits2_0 );

		bits0_0 = __vperm( bits0_0, bits0_0, vmx_dword_overlay_perm );

		__stvewx( bits0_0, cullBits, i );
	}

	for ( ; i < numVerts - 1; i += 2 ) {
		int bits0, bits1;

		const idVec3 &v0 = verts[i+0].xyz;
		const idVec3 &v1 = verts[i+1].xyz;

		float d0  = planes[0][0] * v0[0] + planes[0][1] * v0[1] + planes[0][2] * v0[2] + planes[0][3];
		float d1  = planes[1][0] * v0[0] + planes[1][1] * v0[1] + planes[1][2] * v0[2] + planes[1][3];
		float d2  = planes[2][0] * v0[0] + planes[2][1] * v0[1] + planes[2][2] * v0[2] + planes[2][3];
		float d3  = planes[3][0] * v0[0] + planes[3][1] * v0[1] + planes[3][2] * v0[2] + planes[3][3];

		float d4  = planes[4][0] * v0[0] + planes[4][1] * v0[1] + planes[4][2] * v0[2] + planes[4][3];
		float d5  = planes[5][0] * v0[0] + planes[5][1] * v0[1] + planes[5][2] * v0[2] + planes[5][3];
		float d10 = planes[4][0] * v1[0] + planes[4][1] * v1[1] + planes[4][2] * v1[2] + planes[4][3];
		float d11 = planes[5][0] * v1[0] + planes[5][1] * v1[1] + planes[5][2] * v1[2] + planes[5][3];

		float d6  = planes[0][0] * v1[0] + planes[0][1] * v1[1] + planes[0][2] * v1[2] + planes[0][3];
		float d7  = planes[1][0] * v1[0] + planes[1][1] * v1[1] + planes[1][2] * v1[2] + planes[1][3];
		float d8  = planes[2][0] * v1[0] + planes[2][1] * v1[1] + planes[2][2] * v1[2] + planes[2][3];
		float d9  = planes[3][0] * v1[0] + planes[3][1] * v1[1] + planes[3][2] * v1[2] + planes[3][3];

		bits0  = FLOATSIGNBITSET( d0  ) << 0;
		bits0 |= FLOATSIGNBITSET( d1  ) << 1;
		bits0 |= FLOATSIGNBITSET( d2  ) << 2;
		bits0 |= FLOATSIGNBITSET( d3  ) << 3;
		bits0 |= FLOATSIGNBITSET( d4  ) << 4;
		bits0 |= FLOATSIGNBITSET( d5  ) << 5;

		bits1  = FLOATSIGNBITSET( d6  ) << 0;
		bits1 |= FLOATSIGNBITSET( d7  ) << 1;
		bits1 |= FLOATSIGNBITSET( d8  ) << 2;
		bits1 |= FLOATSIGNBITSET( d9  ) << 3;
		bits1 |= FLOATSIGNBITSET( d10 ) << 4;
		bits1 |= FLOATSIGNBITSET( d11 ) << 5;

		cullBits[i+0] = bits0 ^ 0x3F;				// flip lower 6 bits
		cullBits[i+1] = bits1 ^ 0x3F;				// flip lower 6 bits
	}

	if ( numVerts & 1 ) {
		byte bits;
		float d0, d1, d2, d3, d4, d5;
		const idVec3 &v = verts[numVerts - 1].xyz;

		d0 = planes[0][0] * v[0] + planes[0][1] * v[1] + planes[0][2] * v[2] + planes[0][3];
		d1 = planes[1][0] * v[0] + planes[1][1] * v[1] + planes[1][2] * v[2] + planes[1][3];
		d2 = planes[2][0] * v[0] + planes[2][1] * v[1] + planes[2][2] * v[2] + planes[2][3];
		d3 = planes[3][0] * v[0] + planes[3][1] * v[1] + planes[3][2] * v[2] + planes[3][3];

		d4 = planes[4][0] * v[0] + planes[4][1] * v[1] + planes[4][2] * v[2] + planes[4][3];
		d5 = planes[5][0] * v[0] + planes[5][1] * v[1] + planes[5][2] * v[2] + planes[5][3];

		bits  = FLOATSIGNBITSET( d0 ) << 0;
		bits |= FLOATSIGNBITSET( d1 ) << 1;
		bits |= FLOATSIGNBITSET( d2 ) << 2;
		bits |= FLOATSIGNBITSET( d3 ) << 3;
		bits |= FLOATSIGNBITSET( d4 ) << 4;
		bits |= FLOATSIGNBITSET( d5 ) << 5;

		cullBits[numVerts - 1] = bits ^ 0x3F;		// flip lower 6 bits
	}
}

/*
============
idSIMD_Xenon::OverlayPointCull
============
*/
void VPCALL idSIMD_Xenon::OverlayPointCull( byte *__restrict cullBits, idVec2 *__restrict texCoords, const idPlane *__restrict planes, const idDrawVert *__restrict verts, const int numVerts ) {
	int i;

	const idPlane &p0 = planes[0];
	const idPlane &p1 = planes[1];

	__vector4 px = { p0[0], p1[0], p0[0], p1[0] };
	__vector4 py = { p0[1], p1[1], p0[1], p1[1] };
	__vector4 pz = { p0[2], p1[2], p0[2], p1[2] };
	__vector4 pw = { p0[3], p1[3], p0[3], p1[3] };

	for ( i = 0; i < numVerts - 7; i += 8 ) {

		__vector4 va_0 = *(__vector4 *)( &verts[i+0].xyz );
		__vector4 vb_0 = *(__vector4 *)( &verts[i+1].xyz );
		__vector4 vc_0 = *(__vector4 *)( &verts[i+2].xyz );
		__vector4 vd_0 = *(__vector4 *)( &verts[i+3].xyz );
		__vector4 va_1 = *(__vector4 *)( &verts[i+4].xyz );
		__vector4 vb_1 = *(__vector4 *)( &verts[i+5].xyz );
		__vector4 vc_1 = *(__vector4 *)( &verts[i+6].xyz );
		__vector4 vd_1 = *(__vector4 *)( &verts[i+7].xyz );

		__vector4 ta_0 = __vmrghw( va_0, vb_0 );		// 00, 10, 01, 11
		__vector4 tb_0 = __vmrghw( vc_0, vd_0 );		// 00, 10, 01, 11
		__vector4 tc_0 = __vmrglw( va_0, vb_0 );		// 02, 12, 03, 13
		__vector4 td_0 = __vmrglw( vc_0, vd_0 );		// 02, 12, 03, 13
		__vector4 ta_1 = __vmrghw( va_1, vb_1 );		// 00, 10, 01, 11
		__vector4 tb_1 = __vmrghw( vc_1, vd_1 );		// 00, 10, 01, 11
		__vector4 tc_1 = __vmrglw( va_1, vb_1 );		// 02, 12, 03, 13
		__vector4 td_1 = __vmrglw( vc_1, vd_1 );		// 02, 12, 03, 13

		__vector4 sa_0 = __vpermwi( ta_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 sb_0 = __vpermwi( ta_0, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 sc_0 = __vpermwi( tc_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 sa_1 = __vpermwi( ta_1, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 sb_1 = __vpermwi( ta_1, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 sc_1 = __vpermwi( tc_1, SHUFFLE_D( 0, 0, 1, 1 ) );

		__vector4 sd_0 = __vpermwi( tb_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 se_0 = __vpermwi( tb_0, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 sf_0 = __vpermwi( td_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 sd_1 = __vpermwi( tb_1, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 se_1 = __vpermwi( tb_1, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 sf_1 = __vpermwi( td_1, SHUFFLE_D( 0, 0, 1, 1 ) );

		__vector4 da_0 = __vmaddfp( sa_0, px, pw );
		__vector4 db_0 = __vmaddfp( sd_0, px, pw );
		__vector4 da_1 = __vmaddfp( sa_1, px, pw );
		__vector4 db_1 = __vmaddfp( sd_1, px, pw );

		da_0 = __vmaddfp( sb_0, py, da_0 );
		db_0 = __vmaddfp( se_0, py, db_0 );
		da_1 = __vmaddfp( sb_1, py, da_1 );
		db_1 = __vmaddfp( se_1, py, db_1 );

		da_0 = __vmaddfp( sc_0, pz, da_0 );
		db_0 = __vmaddfp( sf_0, pz, db_0 );
		da_1 = __vmaddfp( sc_1, pz, da_1 );
		db_1 = __vmaddfp( sf_1, pz, db_1 );

		__stvx( da_0, texCoords, i*8+0 );
		__stvx( db_0, texCoords, i*8+16 );
		__stvx( da_1, texCoords, i*8+32 );
		__stvx( db_1, texCoords, i*8+48 );

		__vector4 ba_0 = __vcmpgtfp( da_0, vmx_float_zero );
		__vector4 bb_0 = __vcmpgtfp( db_0, vmx_float_zero );
		__vector4 ba_1 = __vcmpgtfp( da_1, vmx_float_zero );
		__vector4 bb_1 = __vcmpgtfp( db_1, vmx_float_zero );

		ba_0 = __vand( ba_0, vmx_dword_overlay_mask0 );
		bb_0 = __vand( bb_0, vmx_dword_overlay_mask1 );
		ba_1 = __vand( ba_1, vmx_dword_overlay_mask0 );
		bb_1 = __vand( bb_1, vmx_dword_overlay_mask1 );

		da_0 = __vsubfp( vmx_float_one, da_0 );
		db_0 = __vsubfp( vmx_float_one, db_0 );
		da_1 = __vsubfp( vmx_float_one, da_1 );
		db_1 = __vsubfp( vmx_float_one, db_1 );

		__vector4 bc_0 = __vcmpgtfp( da_0, vmx_float_zero );
		__vector4 bd_0 = __vcmpgtfp( db_0, vmx_float_zero );
		__vector4 bc_1 = __vcmpgtfp( da_1, vmx_float_zero );
		__vector4 bd_1 = __vcmpgtfp( db_1, vmx_float_zero );

		bc_0 = __vand( bc_0, vmx_dword_overlay_mask2 );
		bd_0 = __vand( bd_0, vmx_dword_overlay_mask3 );
		bc_1 = __vand( bc_1, vmx_dword_overlay_mask2 );
		bd_1 = __vand( bd_1, vmx_dword_overlay_mask3 );

		ba_0 = __vor( ba_0, bb_0 );
		bc_0 = __vor( bc_0, bd_0 );
		ba_0 = __vor( ba_0, bc_0 );
		ba_1 = __vor( ba_1, bb_1 );
		bc_1 = __vor( bc_1, bd_1 );
		ba_1 = __vor( ba_1, bc_1 );

		__vector4 bits0_0 = __vpermwi( ba_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 bits1_0 = __vpermwi( ba_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 bits2_0 = __vpermwi( ba_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 bits3_0 = __vpermwi( ba_0, SHUFFLE_D( 3, 3, 3, 3 ) );
		__vector4 bits0_1 = __vpermwi( ba_1, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 bits1_1 = __vpermwi( ba_1, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 bits2_1 = __vpermwi( ba_1, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 bits3_1 = __vpermwi( ba_1, SHUFFLE_D( 3, 3, 3, 3 ) );

		bits0_0 = __vor( bits0_0, bits1_0 );
		bits2_0 = __vor( bits2_0, bits3_0 );
		bits0_0 = __vor( bits0_0, bits2_0 );
		bits0_1 = __vor( bits0_1, bits1_1 );
		bits2_1 = __vor( bits2_1, bits3_1 );
		bits0_1 = __vor( bits0_1, bits2_1 );

		bits0_0 = __vxor( bits0_0, vmx_dword_overlay_xor );
		bits0_1 = __vxor( bits0_1, vmx_dword_overlay_xor );

		bits0_0 = __vperm( bits0_0, bits0_0, vmx_dword_overlay_perm );
		bits0_1 = __vperm( bits0_1, bits0_1, vmx_dword_overlay_perm );

		__stvewx( bits0_0, cullBits, i );
		__stvewx( bits0_1, cullBits, i+4 );
	}

	for ( ; i < numVerts - 3; i += 4 ) {

		__vector4 va_0 = *(__vector4 *)( &verts[i+0].xyz );
		__vector4 vb_0 = *(__vector4 *)( &verts[i+1].xyz );
		__vector4 vc_0 = *(__vector4 *)( &verts[i+2].xyz );
		__vector4 vd_0 = *(__vector4 *)( &verts[i+3].xyz );

		__vector4 ta_0 = __vmrghw( va_0, vb_0 );		// 00, 10, 01, 11
		__vector4 tb_0 = __vmrghw( vc_0, vd_0 );		// 00, 10, 01, 11
		__vector4 tc_0 = __vmrglw( va_0, vb_0 );		// 02, 12, 03, 13
		__vector4 td_0 = __vmrglw( vc_0, vd_0 );		// 02, 12, 03, 13

		__vector4 sa_0 = __vpermwi( ta_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 sb_0 = __vpermwi( ta_0, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 sc_0 = __vpermwi( tc_0, SHUFFLE_D( 0, 0, 1, 1 ) );

		__vector4 sd_0 = __vpermwi( tb_0, SHUFFLE_D( 0, 0, 1, 1 ) );
		__vector4 se_0 = __vpermwi( tb_0, SHUFFLE_D( 2, 2, 3, 3 ) );
		__vector4 sf_0 = __vpermwi( td_0, SHUFFLE_D( 0, 0, 1, 1 ) );

		__vector4 da_0 = __vmaddfp( sa_0, px, pw );
		__vector4 db_0 = __vmaddfp( sd_0, px, pw );

		da_0 = __vmaddfp( sb_0, py, da_0 );
		db_0 = __vmaddfp( se_0, py, db_0 );

		da_0 = __vmaddfp( sc_0, pz, da_0 );
		db_0 = __vmaddfp( sf_0, pz, db_0 );

		__stvx( da_0, texCoords, i*8+0 );
		__stvx( db_0, texCoords, i*8+16 );

		__vector4 ba_0 = __vcmpgtfp( da_0, vmx_float_zero );
		__vector4 bb_0 = __vcmpgtfp( db_0, vmx_float_zero );

		ba_0 = __vand( ba_0, vmx_dword_overlay_mask0 );
		bb_0 = __vand( bb_0, vmx_dword_overlay_mask1 );

		da_0 = __vsubfp( vmx_float_one, da_0 );
		db_0 = __vsubfp( vmx_float_one, db_0 );

		__vector4 bc_0 = __vcmpgtfp( da_0, vmx_float_zero );
		__vector4 bd_0 = __vcmpgtfp( db_0, vmx_float_zero );

		bc_0 = __vand( bc_0, vmx_dword_overlay_mask2 );
		bd_0 = __vand( bd_0, vmx_dword_overlay_mask3 );

		ba_0 = __vor( ba_0, bb_0 );
		bc_0 = __vor( bc_0, bd_0 );
		ba_0 = __vor( ba_0, bc_0 );

		__vector4 bits0_0 = __vpermwi( ba_0, SHUFFLE_D( 0, 0, 0, 0 ) );
		__vector4 bits1_0 = __vpermwi( ba_0, SHUFFLE_D( 1, 1, 1, 1 ) );
		__vector4 bits2_0 = __vpermwi( ba_0, SHUFFLE_D( 2, 2, 2, 2 ) );
		__vector4 bits3_0 = __vpermwi( ba_0, SHUFFLE_D( 3, 3, 3, 3 ) );

		bits0_0 = __vor( bits0_0, bits1_0 );
		bits2_0 = __vor( bits2_0, bits3_0 );
		bits0_0 = __vor( bits0_0, bits2_0 );

		bits0_0 = __vxor( bits0_0, vmx_dword_overlay_xor );

		bits0_0 = __vperm( bits0_0, bits0_0, vmx_dword_overlay_perm );

		__stvewx( bits0_0, cullBits, i );
	}

	for ( ; i < numVerts - 1; i += 2 ) {
		unsigned int bits;

		const idVec3 &v0 = verts[i+0].xyz;
		const idVec3 &v1 = verts[i+1].xyz;

		float d0 = p0[0] * v0[0] + p0[1] * v0[1] + p0[2] * v0[2] + p0[3];
		float d1 = p1[0] * v0[0] + p1[1] * v0[1] + p1[2] * v0[2] + p1[3];
		float d2 = p0[0] * v1[0] + p0[1] * v1[1] + p0[2] * v1[2] + p0[3];
		float d3 = p1[0] * v1[0] + p1[1] * v1[1] + p1[2] * v1[2] + p1[3];

		texCoords[i+0][0] = d0;
		texCoords[i+0][1] = d1;
		texCoords[i+1][0] = d2;
		texCoords[i+1][1] = d3;

		bits  = FLOATSIGNBITSET( d0 ) << 0;
		bits |= FLOATSIGNBITSET( d1 ) << 1;
		bits |= FLOATSIGNBITSET( d2 ) << 8;
		bits |= FLOATSIGNBITSET( d3 ) << 9;

		d0 = 1.0f - d0;
		d1 = 1.0f - d1;
		d2 = 1.0f - d2;
		d3 = 1.0f - d3;

		bits |= FLOATSIGNBITSET( d0 ) << 2;
		bits |= FLOATSIGNBITSET( d1 ) << 3;
		bits |= FLOATSIGNBITSET( d2 ) << 10;
		bits |= FLOATSIGNBITSET( d3 ) << 11;

		cullBits[i+0] = ( bits >> 0 ) & 0xFF;
		cullBits[i+1] = ( bits >> 8 ) & 0xFF;
	}

	if ( numVerts & 1 ) {
		byte bits;
		float d0, d1;

		const idPlane &p0 = planes[0];
		const idPlane &p1 = planes[1];
		const idVec3 &v0 = verts[numVerts - 1].xyz;

		d0 = p0[0] * v0[0] + p0[1] * v0[1] + p0[2] * v0[2] + p0[3];
		d1 = p1[0] * v0[0] + p1[1] * v0[1] + p1[2] * v0[2] + p1[3];

		texCoords[numVerts - 1][0] = d0;
		texCoords[numVerts - 1][1] = d1;

		bits  = FLOATSIGNBITSET( d0 ) << 0;
		bits |= FLOATSIGNBITSET( d1 ) << 1;

		d0 = 1.0f - d0;
		d1 = 1.0f - d1;

		bits |= FLOATSIGNBITSET( d0 ) << 2;
		bits |= FLOATSIGNBITSET( d1 ) << 3;

		cullBits[numVerts - 1] = bits;
	}
}

/*
============
idSIMD_Xenon::DeriveTriPlanes

	Derives a plane equation for each triangle.
============
*/
void VPCALL idSIMD_Xenon::DeriveTriPlanes( idPlane *__restrict planes, const idDrawVert *__restrict verts, const int numVerts, const vertIndex_t *__restrict indexes, const int numIndexes ) {
	const vertIndex_t *end = indexes + numIndexes;

	assert_16_byte_aligned( planes );
	assert_16_byte_aligned( verts );
	assert_sizeof_16_byte_multiple( idPlane );
	assert_sizeof_16_byte_multiple( idDrawVert );

	for ( ; indexes + 7*3 < end; indexes += 8*3, planes += 8 ) {

		__vector4 a0_0 = *(__vector4 *)( verts + indexes[0 * 3 + 0] );
		__vector4 b0_0 = *(__vector4 *)( verts + indexes[0 * 3 + 1] );
		__vector4 c0_0 = *(__vector4 *)( verts + indexes[0 * 3 + 2] );
		__vector4 a1_0 = *(__vector4 *)( verts + indexes[1 * 3 + 0] );
		__vector4 b1_0 = *(__vector4 *)( verts + indexes[1 * 3 + 1] );
		__vector4 c1_0 = *(__vector4 *)( verts + indexes[1 * 3 + 2] );
		__vector4 a2_0 = *(__vector4 *)( verts + indexes[2 * 3 + 0] );
		__vector4 b2_0 = *(__vector4 *)( verts + indexes[2 * 3 + 1] );
		__vector4 c2_0 = *(__vector4 *)( verts + indexes[2 * 3 + 2] );
		__vector4 a3_0 = *(__vector4 *)( verts + indexes[3 * 3 + 0] );
		__vector4 b3_0 = *(__vector4 *)( verts + indexes[3 * 3 + 1] );
		__vector4 c3_0 = *(__vector4 *)( verts + indexes[3 * 3 + 2] );

		__vector4 a0_1 = *(__vector4 *)( verts + indexes[4 * 3 + 0] );
		__vector4 b0_1 = *(__vector4 *)( verts + indexes[4 * 3 + 1] );
		__vector4 c0_1 = *(__vector4 *)( verts + indexes[4 * 3 + 2] );
		__vector4 a1_1 = *(__vector4 *)( verts + indexes[5 * 3 + 0] );
		__vector4 b1_1 = *(__vector4 *)( verts + indexes[5 * 3 + 1] );
		__vector4 c1_1 = *(__vector4 *)( verts + indexes[5 * 3 + 2] );
		__vector4 a2_1 = *(__vector4 *)( verts + indexes[6 * 3 + 0] );
		__vector4 b2_1 = *(__vector4 *)( verts + indexes[6 * 3 + 1] );
		__vector4 c2_1 = *(__vector4 *)( verts + indexes[6 * 3 + 2] );
		__vector4 a3_1 = *(__vector4 *)( verts + indexes[7 * 3 + 0] );
		__vector4 b3_1 = *(__vector4 *)( verts + indexes[7 * 3 + 1] );
		__vector4 c3_1 = *(__vector4 *)( verts + indexes[7 * 3 + 2] );

		__vector4 ta0_0 = __vmrghw( a0_0, a1_0 );
		__vector4 ta1_0 = __vmrghw( a2_0, a3_0 );
		__vector4 ta2_0 = __vmrglw( a0_0, a1_0 );
		__vector4 ta3_0 = __vmrglw( a2_0, a3_0 );
		__vector4 ta0_1 = __vmrghw( a0_1, a1_1 );
		__vector4 ta1_1 = __vmrghw( a2_1, a3_1 );
		__vector4 ta2_1 = __vmrglw( a0_1, a1_1 );
		__vector4 ta3_1 = __vmrglw( a2_1, a3_1 );

		__vector4 tb0_0 = __vmrghw( b0_0, b1_0 );
		__vector4 tb1_0 = __vmrghw( b2_0, b3_0 );
		__vector4 tb2_0 = __vmrglw( b0_0, b1_0 );
		__vector4 tb3_0 = __vmrglw( b2_0, b3_0 );
		__vector4 tb0_1 = __vmrghw( b0_1, b1_1 );
		__vector4 tb1_1 = __vmrghw( b2_1, b3_1 );
		__vector4 tb2_1 = __vmrglw( b0_1, b1_1 );
		__vector4 tb3_1 = __vmrglw( b2_1, b3_1 );

		__vector4 tc0_0 = __vmrghw( c0_0, c1_0 );
		__vector4 tc1_0 = __vmrghw( c2_0, c3_0 );
		__vector4 tc2_0 = __vmrglw( c0_0, c1_0 );
		__vector4 tc3_0 = __vmrglw( c2_0, c3_0 );
		__vector4 tc0_1 = __vmrghw( c0_1, c1_1 );
		__vector4 tc1_1 = __vmrghw( c2_1, c3_1 );
		__vector4 tc2_1 = __vmrglw( c0_1, c1_1 );
		__vector4 tc3_1 = __vmrglw( c2_1, c3_1 );

		__vector4 ax_0 = __vperm( ta0_0, ta1_0, vmx_dword_perm_plane_x );
		__vector4 ay_0 = __vperm( ta0_0, ta1_0, vmx_dword_perm_plane_y );
		__vector4 az_0 = __vperm( ta2_0, ta3_0, vmx_dword_perm_plane_x );
		__vector4 ax_1 = __vperm( ta0_1, ta1_1, vmx_dword_perm_plane_x );
		__vector4 ay_1 = __vperm( ta0_1, ta1_1, vmx_dword_perm_plane_y );
		__vector4 az_1 = __vperm( ta2_1, ta3_1, vmx_dword_perm_plane_x );

		__vector4 bx_0 = __vperm( tb0_0, tb1_0, vmx_dword_perm_plane_x );
		__vector4 by_0 = __vperm( tb0_0, tb1_0, vmx_dword_perm_plane_y );
		__vector4 bz_0 = __vperm( tb2_0, tb3_0, vmx_dword_perm_plane_x );
		__vector4 bx_1 = __vperm( tb0_1, tb1_1, vmx_dword_perm_plane_x );
		__vector4 by_1 = __vperm( tb0_1, tb1_1, vmx_dword_perm_plane_y );
		__vector4 bz_1 = __vperm( tb2_1, tb3_1, vmx_dword_perm_plane_x );

		__vector4 cx_0 = __vperm( tc0_0, tc1_0, vmx_dword_perm_plane_x );
		__vector4 cy_0 = __vperm( tc0_0, tc1_0, vmx_dword_perm_plane_y );
		__vector4 cz_0 = __vperm( tc2_0, tc3_0, vmx_dword_perm_plane_x );
		__vector4 cx_1 = __vperm( tc0_1, tc1_1, vmx_dword_perm_plane_x );
		__vector4 cy_1 = __vperm( tc0_1, tc1_1, vmx_dword_perm_plane_y );
		__vector4 cz_1 = __vperm( tc2_1, tc3_1, vmx_dword_perm_plane_x );

		__vector4 d0_0 = __vsubfp( bx_0, ax_0 );
		__vector4 d1_0 = __vsubfp( by_0, ay_0 );
		__vector4 d2_0 = __vsubfp( bz_0, az_0 );
		__vector4 d0_1 = __vsubfp( bx_1, ax_1 );
		__vector4 d1_1 = __vsubfp( by_1, ay_1 );
		__vector4 d2_1 = __vsubfp( bz_1, az_1 );

		__vector4 d3_0 = __vsubfp( cx_0, ax_0 );
		__vector4 d4_0 = __vsubfp( cy_0, ay_0 );
		__vector4 d5_0 = __vsubfp( cz_0, az_0 );
		__vector4 d3_1 = __vsubfp( cx_1, ax_1 );
		__vector4 d4_1 = __vsubfp( cy_1, ay_1 );
		__vector4 d5_1 = __vsubfp( cz_1, az_1 );

		__vector4 m0_0 = __vmulfp( d4_0, d2_0 );
		__vector4 m1_0 = __vmulfp( d5_0, d0_0 );
		__vector4 m2_0 = __vmulfp( d3_0, d1_0 );
		__vector4 m0_1 = __vmulfp( d4_1, d2_1 );
		__vector4 m1_1 = __vmulfp( d5_1, d0_1 );
		__vector4 m2_1 = __vmulfp( d3_1, d1_1 );

		__vector4 m3_0 = __vmulfp( d5_0, d1_0 );
		__vector4 m4_0 = __vmulfp( d3_0, d2_0 );
		__vector4 m5_0 = __vmulfp( d4_0, d0_0 );
		__vector4 m3_1 = __vmulfp( d5_1, d1_1 );
		__vector4 m4_1 = __vmulfp( d3_1, d2_1 );
		__vector4 m5_1 = __vmulfp( d4_1, d0_1 );

		__vector4 vx_0 = __vsubfp( m0_0, m3_0 );
		__vector4 vy_0 = __vsubfp( m1_0, m4_0 );
		__vector4 vz_0 = __vsubfp( m2_0, m5_0 );
		__vector4 vx_1 = __vsubfp( m0_1, m3_1 );
		__vector4 vy_1 = __vsubfp( m1_1, m4_1 );
		__vector4 vz_1 = __vsubfp( m2_1, m5_1 );

		__vector4 s0_0 = __vmulfp( vx_0, vx_0 );
		__vector4 s1_0 = __vmulfp( vy_0, vy_0 );
		__vector4 s2_0 = __vmulfp( vz_0, vz_0 );
		__vector4 s0_1 = __vmulfp( vx_1, vx_1 );
		__vector4 s1_1 = __vmulfp( vy_1, vy_1 );
		__vector4 s2_1 = __vmulfp( vz_1, vz_1 );

		__vector4 e0_0 = __vaddfp( s0_0, s1_0 );
		__vector4 e1_0 = __vaddfp( e0_0, s2_0 );
		__vector4 e0_1 = __vaddfp( s0_1, s1_1 );
		__vector4 e1_1 = __vaddfp( e0_1, s2_1 );

		__vector4 rp_0 = __vrsqrtefp( e1_0 );
		__vector4 rp_1 = __vrsqrtefp( e1_1 );

		__vector4 nx_0 = __vmulfp( vx_0, rp_0 );
		__vector4 ny_0 = __vmulfp( vy_0, rp_0 );
		__vector4 nz_0 = __vmulfp( vz_0, rp_0 );
		__vector4 nx_1 = __vmulfp( vx_1, rp_1 );
		__vector4 ny_1 = __vmulfp( vy_1, rp_1 );
		__vector4 nz_1 = __vmulfp( vz_1, rp_1 );

		__vector4 s3_0 = __vmulfp( nx_0, ax_0 );
		__vector4 s4_0 = __vmaddfp( ny_0, ay_0, s3_0 );
		__vector4 vw_0 = __vmaddfp( nz_0, az_0, s4_0 );
		__vector4 s3_1 = __vmulfp( nx_1, ax_1 );
		__vector4 s4_1 = __vmaddfp( ny_1, ay_1, s3_1 );
		__vector4 vw_1 = __vmaddfp( nz_1, az_1, s4_1 );

		__vector4 nw_0 = __vxor( vw_0, vmx_float_sign_bit );
		__vector4 nw_1 = __vxor( vw_1, vmx_float_sign_bit );

		__vector4 tp0_0 = __vmrghw( nx_0, ny_0 );
		__vector4 tp1_0 = __vmrghw( nz_0, nw_0 );
		__vector4 tp2_0 = __vmrglw( nx_0, ny_0 );
		__vector4 tp3_0 = __vmrglw( nz_0, nw_0 );
		__vector4 tp0_1 = __vmrghw( nx_1, ny_1 );
		__vector4 tp1_1 = __vmrghw( nz_1, nw_1 );
		__vector4 tp2_1 = __vmrglw( nx_1, ny_1 );
		__vector4 tp3_1 = __vmrglw( nz_1, nw_1 );

		__vector4 p0_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_x );
		__vector4 p1_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_y );
		__vector4 p2_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_x );
		__vector4 p3_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_y );
		__vector4 p0_1 = __vperm( tp0_1, tp1_1, vmx_dword_perm_plane_x );
		__vector4 p1_1 = __vperm( tp0_1, tp1_1, vmx_dword_perm_plane_y );
		__vector4 p2_1 = __vperm( tp2_1, tp3_1, vmx_dword_perm_plane_x );
		__vector4 p3_1 = __vperm( tp2_1, tp3_1, vmx_dword_perm_plane_y );

		__stvx( p0_0, planes, 0*16 );
		__stvx( p1_0, planes, 1*16 );
		__stvx( p2_0, planes, 2*16 );
		__stvx( p3_0, planes, 3*16 );
		__stvx( p0_1, planes, 4*16 );
		__stvx( p1_1, planes, 5*16 );
		__stvx( p2_1, planes, 6*16 );
		__stvx( p3_1, planes, 7*16 );
	}

	for ( ; indexes + 3*3 < end; indexes += 4*3, planes += 4 ) {

		__vector4 a0_0 = *(__vector4 *)( verts + indexes[0 * 3 + 0] );
		__vector4 b0_0 = *(__vector4 *)( verts + indexes[0 * 3 + 1] );
		__vector4 c0_0 = *(__vector4 *)( verts + indexes[0 * 3 + 2] );

		__vector4 a1_0 = *(__vector4 *)( verts + indexes[1 * 3 + 0] );
		__vector4 b1_0 = *(__vector4 *)( verts + indexes[1 * 3 + 1] );
		__vector4 c1_0 = *(__vector4 *)( verts + indexes[1 * 3 + 2] );

		__vector4 a2_0 = *(__vector4 *)( verts + indexes[2 * 3 + 0] );
		__vector4 b2_0 = *(__vector4 *)( verts + indexes[2 * 3 + 1] );
		__vector4 c2_0 = *(__vector4 *)( verts + indexes[2 * 3 + 2] );

		__vector4 a3_0 = *(__vector4 *)( verts + indexes[3 * 3 + 0] );
		__vector4 b3_0 = *(__vector4 *)( verts + indexes[3 * 3 + 1] );
		__vector4 c3_0 = *(__vector4 *)( verts + indexes[3 * 3 + 2] );

		__vector4 ta0_0 = __vmrghw( a0_0, a1_0 );
		__vector4 ta1_0 = __vmrghw( a2_0, a3_0 );
		__vector4 ta2_0 = __vmrglw( a0_0, a1_0 );
		__vector4 ta3_0 = __vmrglw( a2_0, a3_0 );

		__vector4 tb0_0 = __vmrghw( b0_0, b1_0 );
		__vector4 tb1_0 = __vmrghw( b2_0, b3_0 );
		__vector4 tb2_0 = __vmrglw( b0_0, b1_0 );
		__vector4 tb3_0 = __vmrglw( b2_0, b3_0 );

		__vector4 tc0_0 = __vmrghw( c0_0, c1_0 );
		__vector4 tc1_0 = __vmrghw( c2_0, c3_0 );
		__vector4 tc2_0 = __vmrglw( c0_0, c1_0 );
		__vector4 tc3_0 = __vmrglw( c2_0, c3_0 );

		__vector4 ax_0 = __vperm( ta0_0, ta1_0, vmx_dword_perm_plane_x );
		__vector4 ay_0 = __vperm( ta0_0, ta1_0, vmx_dword_perm_plane_y );
		__vector4 az_0 = __vperm( ta2_0, ta3_0, vmx_dword_perm_plane_x );

		__vector4 bx_0 = __vperm( tb0_0, tb1_0, vmx_dword_perm_plane_x );
		__vector4 by_0 = __vperm( tb0_0, tb1_0, vmx_dword_perm_plane_y );
		__vector4 bz_0 = __vperm( tb2_0, tb3_0, vmx_dword_perm_plane_x );

		__vector4 cx_0 = __vperm( tc0_0, tc1_0, vmx_dword_perm_plane_x );
		__vector4 cy_0 = __vperm( tc0_0, tc1_0, vmx_dword_perm_plane_y );
		__vector4 cz_0 = __vperm( tc2_0, tc3_0, vmx_dword_perm_plane_x );

		__vector4 d0_0 = __vsubfp( bx_0, ax_0 );
		__vector4 d1_0 = __vsubfp( by_0, ay_0 );
		__vector4 d2_0 = __vsubfp( bz_0, az_0 );

		__vector4 d3_0 = __vsubfp( cx_0, ax_0 );
		__vector4 d4_0 = __vsubfp( cy_0, ay_0 );
		__vector4 d5_0 = __vsubfp( cz_0, az_0 );

		__vector4 m0_0 = __vmulfp( d4_0, d2_0 );
		__vector4 m1_0 = __vmulfp( d5_0, d0_0 );
		__vector4 m2_0 = __vmulfp( d3_0, d1_0 );

		__vector4 m3_0 = __vmulfp( d5_0, d1_0 );
		__vector4 m4_0 = __vmulfp( d3_0, d2_0 );
		__vector4 m5_0 = __vmulfp( d4_0, d0_0 );

		__vector4 vx_0 = __vsubfp( m0_0, m3_0 );
		__vector4 vy_0 = __vsubfp( m1_0, m4_0 );
		__vector4 vz_0 = __vsubfp( m2_0, m5_0 );

		__vector4 s0_0 = __vmulfp( vx_0, vx_0 );
		__vector4 s1_0 = __vmulfp( vy_0, vy_0 );
		__vector4 s2_0 = __vmulfp( vz_0, vz_0 );

		__vector4 e0_0 = __vaddfp( s0_0, s1_0 );
		__vector4 e1_0 = __vaddfp( e0_0, s2_0 );

		__vector4 rp_0 = __vrsqrtefp( e1_0 );

		__vector4 nx_0 = __vmulfp( vx_0, rp_0 );
		__vector4 ny_0 = __vmulfp( vy_0, rp_0 );
		__vector4 nz_0 = __vmulfp( vz_0, rp_0 );

		__vector4 s3_0 = __vmulfp( nx_0, ax_0 );
		__vector4 s4_0 = __vmaddfp( ny_0, ay_0, s3_0 );
		__vector4 vw_0 = __vmaddfp( nz_0, az_0, s4_0 );

		__vector4 nw_0 = __vxor( vw_0, vmx_float_sign_bit );

		__vector4 tp0_0 = __vmrghw( nx_0, ny_0 );
		__vector4 tp1_0 = __vmrghw( nz_0, nw_0 );
		__vector4 tp2_0 = __vmrglw( nx_0, ny_0 );
		__vector4 tp3_0 = __vmrglw( nz_0, nw_0 );

		__vector4 p0_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_x );
		__vector4 p1_0 = __vperm( tp0_0, tp1_0, vmx_dword_perm_plane_y );
		__vector4 p2_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_x );
		__vector4 p3_0 = __vperm( tp2_0, tp3_0, vmx_dword_perm_plane_y );

		__stvx( p0_0, planes, 0 );
		__stvx( p1_0, planes, 16 );
		__stvx( p2_0, planes, 32 );
		__stvx( p3_0, planes, 48 );
	}

	for ( ; indexes < end; indexes += 3, planes++ ) {

		__vector4 a = *(__vector4 *)( verts + indexes[0] );
		__vector4 b = *(__vector4 *)( verts + indexes[1] );
		__vector4 c = *(__vector4 *)( verts + indexes[2] );

		__vector4 da = __vsubfp( b, a );
		__vector4 db = __vsubfp( c, a );

		__vector4 dc = __vpermwi( da, SHUFFLE_D( 1, 2, 0, 3 ) );
		__vector4 dd = __vpermwi( db, SHUFFLE_D( 1, 2, 0, 3 ) );

		__vector4 de = __vmulfp( db, dc );
		__vector4 df = __vmulfp( da, dd );

		__vector4 dg = __vsubfp( de, df );

		__vector4 dh = __vmsum3fp( dg, dg );
		__vector4 di = __vrsqrtefp( dh );
		__vector4 np = __vmulfp( dg, di );

		__vector4 normal = __vpermwi( np, SHUFFLE_D( 1, 2, 0, 3 ) );

		__vector4 dj = __vmsum3fp( normal, a );
		__vector4 dist = __vxor( dj, vmx_float_sign_bit );

		__vector4 plane = __vperm( normal, dist, vmx_dword_perm_replacelast );

		__stvx( plane, planes, 0 );
	}
}

/*
============
idSIMD_Xenon::CalculateFacing
============
*/
void VPCALL idSIMD_Xenon::CalculateFacing( byte *__restrict facing, const idPlane *__restrict planes, const int numTriangles, const idVec4 &light ) {
	int	i;

	__vector4 vlight = (__vector4 &) light;

	for	( i	= 0; i < numTriangles - 15; i += 16 ) {
		__vector4 p0_0 = __lvx( planes, i*16+ 0*16 );
		__vector4 p1_0 = __lvx( planes, i*16+ 1*16 );
		__vector4 p2_0 = __lvx( planes, i*16+ 2*16 );
		__vector4 p3_0 = __lvx( planes, i*16+ 3*16 );
		__vector4 p0_1 = __lvx( planes, i*16+ 4*16 );
		__vector4 p1_1 = __lvx( planes, i*16+ 5*16 );
		__vector4 p2_1 = __lvx( planes, i*16+ 6*16 );
		__vector4 p3_1 = __lvx( planes, i*16+ 7*16 );
		__vector4 p0_2 = __lvx( planes, i*16+ 8*16 );
		__vector4 p1_2 = __lvx( planes, i*16+ 9*16 );
		__vector4 p2_2 = __lvx( planes, i*16+10*16 );
		__vector4 p3_2 = __lvx( planes, i*16+11*16 );
		__vector4 p0_3 = __lvx( planes, i*16+12*16 );
		__vector4 p1_3 = __lvx( planes, i*16+13*16 );
		__vector4 p2_3 = __lvx( planes, i*16+14*16 );
		__vector4 p3_3 = __lvx( planes, i*16+15*16 );

		__vector4 d0_0 = __vmsum4fp( p0_0, vlight );
		__vector4 d1_0 = __vmsum4fp( p1_0, vlight );
		__vector4 d2_0 = __vmsum4fp( p2_0, vlight );
		__vector4 d3_0 = __vmsum4fp( p3_0, vlight );
		__vector4 d0_1 = __vmsum4fp( p0_1, vlight );
		__vector4 d1_1 = __vmsum4fp( p1_1, vlight );
		__vector4 d2_1 = __vmsum4fp( p2_1, vlight );
		__vector4 d3_1 = __vmsum4fp( p3_1, vlight );
		__vector4 d0_2 = __vmsum4fp( p0_2, vlight );
		__vector4 d1_2 = __vmsum4fp( p1_2, vlight );
		__vector4 d2_2 = __vmsum4fp( p2_2, vlight );
		__vector4 d3_2 = __vmsum4fp( p3_2, vlight );
		__vector4 d0_3 = __vmsum4fp( p0_3, vlight );
		__vector4 d1_3 = __vmsum4fp( p1_3, vlight );
		__vector4 d2_3 = __vmsum4fp( p2_3, vlight );
		__vector4 d3_3 = __vmsum4fp( p3_3, vlight );

		__vector4 b0_0 = __vcmpgtfp( d0_0, vmx_float_zero );
		__vector4 b1_0 = __vcmpgtfp( d1_0, vmx_float_zero );
		__vector4 b2_0 = __vcmpgtfp( d2_0, vmx_float_zero );
		__vector4 b3_0 = __vcmpgtfp( d3_0, vmx_float_zero );
		__vector4 b0_1 = __vcmpgtfp( d0_1, vmx_float_zero );
		__vector4 b1_1 = __vcmpgtfp( d1_1, vmx_float_zero );
		__vector4 b2_1 = __vcmpgtfp( d2_1, vmx_float_zero );
		__vector4 b3_1 = __vcmpgtfp( d3_1, vmx_float_zero );
		__vector4 b0_2 = __vcmpgtfp( d0_2, vmx_float_zero );
		__vector4 b1_2 = __vcmpgtfp( d1_2, vmx_float_zero );
		__vector4 b2_2 = __vcmpgtfp( d2_2, vmx_float_zero );
		__vector4 b3_2 = __vcmpgtfp( d3_2, vmx_float_zero );
		__vector4 b0_3 = __vcmpgtfp( d0_3, vmx_float_zero );
		__vector4 b1_3 = __vcmpgtfp( d1_3, vmx_float_zero );
		__vector4 b2_3 = __vcmpgtfp( d2_3, vmx_float_zero );
		__vector4 b3_3 = __vcmpgtfp( d3_3, vmx_float_zero );

		b0_0 = __vand( b0_0, vmx_dword_facing_mask0 );
		b1_0 = __vand( b1_0, vmx_dword_facing_mask1 );
		b2_0 = __vand( b2_0, vmx_dword_facing_mask2 );
		b3_0 = __vand( b3_0, vmx_dword_facing_mask3 );
		b0_1 = __vand( b0_1, vmx_dword_facing_mask0 );
		b1_1 = __vand( b1_1, vmx_dword_facing_mask1 );
		b2_1 = __vand( b2_1, vmx_dword_facing_mask2 );
		b3_1 = __vand( b3_1, vmx_dword_facing_mask3 );
		b0_2 = __vand( b0_2, vmx_dword_facing_mask0 );
		b1_2 = __vand( b1_2, vmx_dword_facing_mask1 );
		b2_2 = __vand( b2_2, vmx_dword_facing_mask2 );
		b3_2 = __vand( b3_2, vmx_dword_facing_mask3 );
		b0_3 = __vand( b0_3, vmx_dword_facing_mask0 );
		b1_3 = __vand( b1_3, vmx_dword_facing_mask1 );
		b2_3 = __vand( b2_3, vmx_dword_facing_mask2 );
		b3_3 = __vand( b3_3, vmx_dword_facing_mask3 );

		b0_0 = __vor( b0_0, b1_0 );
		b2_0 = __vor( b2_0, b3_0 );
		b0_0 = __vor( b0_0, b2_0 );
		b0_1 = __vor( b0_1, b1_1 );
		b2_1 = __vor( b2_1, b3_1 );
		b0_1 = __vor( b0_1, b2_1 );
		b0_2 = __vor( b0_2, b1_2 );
		b2_2 = __vor( b2_2, b3_2 );
		b0_2 = __vor( b0_2, b2_2 );
		b0_3 = __vor( b0_3, b1_3 );
		b2_3 = __vor( b2_3, b3_3 );
		b0_3 = __vor( b0_3, b2_3 );

	 	__stvewx( b0_0, facing, i+ 0 );
	 	__stvewx( b0_1, facing, i+ 4 );
	 	__stvewx( b0_2, facing, i+ 8 );
	 	__stvewx( b0_3, facing, i+12 );
	}

	for	( ; i < numTriangles - 3; i += 4 ) {
		__vector4 p0_0 = __lvx( planes, i*16+ 0 );
		__vector4 p1_0 = __lvx( planes, i*16+16 );
		__vector4 p2_0 = __lvx( planes, i*16+32 );
		__vector4 p3_0 = __lvx( planes, i*16+48 );

		__vector4 d0_0 = __vmsum4fp( p0_0, vlight );
		__vector4 d1_0 = __vmsum4fp( p1_0, vlight );
		__vector4 d2_0 = __vmsum4fp( p2_0, vlight );
		__vector4 d3_0 = __vmsum4fp( p3_0, vlight );

		__vector4 b0_0 = __vcmpgtfp( d0_0, vmx_float_zero );
		__vector4 b1_0 = __vcmpgtfp( d1_0, vmx_float_zero );
		__vector4 b2_0 = __vcmpgtfp( d2_0, vmx_float_zero );
		__vector4 b3_0 = __vcmpgtfp( d3_0, vmx_float_zero );

		b0_0 = __vand( b0_0, vmx_dword_facing_mask0 );
		b1_0 = __vand( b1_0, vmx_dword_facing_mask1 );
		b2_0 = __vand( b2_0, vmx_dword_facing_mask2 );
		b3_0 = __vand( b3_0, vmx_dword_facing_mask3 );

		b0_0 = __vor( b0_0, b1_0 );
		b2_0 = __vor( b2_0, b3_0 );
		b0_0 = __vor( b0_0, b2_0 );

	 	__stvewx( b0_0, facing, i );
	}

	for	( ; i < numTriangles; i++ ) {
		facing[i] =	planes[i][0] * light.x + planes[i][1] * light.y + planes[i][2] * light.z + planes[i][3] * light.w > 0.0f;
	}
	facing[numTriangles] = 1;	// for dangling	edges to reference
}

/*
============
idSIMD_Xenon::CalculateCullBits
============
*/
void VPCALL idSIMD_Xenon::CalculateCullBits( byte * __restrict cullBits, const idDrawVert * __restrict verts, const int numVerts, const int frontBits, const idPlane lightPlanes[NUM_LIGHT_PLANES] ) {
	int i, j;

	assert( NUM_LIGHT_PLANES <= sizeof( cullBits[0] ) * 8 );

	XMemSet( cullBits, 0, numVerts * sizeof( cullBits[0] ) );

	for ( i = 0; i < NUM_LIGHT_PLANES; i++ ) {
		// if completely infront of this clipping plane
		if ( frontBits & ( 1 << i ) ) {
			continue;
		}
		__vector4 plane = (__vector4 &) lightPlanes[i];
		__vector4i mask0 = { (1<<24) << i, (1<<24) << i, (1<<24) << i, (1<<24) << i };
		__vector4i mask1 = { (1<<16) << i, (1<<16) << i, (1<<16) << i, (1<<16) << i };
		__vector4i mask2 = { (1<< 8) << i, (1<< 8) << i, (1<< 8) << i, (1<< 8) << i };
		__vector4i mask3 = { (1<< 0) << i, (1<< 0) << i, (1<< 0) << i, (1<< 0) << i };

		plane = __vxor( plane, vmx_float_sign_bit );

		__vector4 perm = vmx_dword_perm_replacelast;
		__vector4 zero = vmx_float_zero;

		for ( j = 0; j < numVerts - 15; j += 16 ) {
	 		__vector4 bits = __lvx( cullBits, j );

			__vector4 v0_0 = __lvx( verts, j*DRAWVERT_SIZE+ 0*DRAWVERT_SIZE );
			__vector4 v1_0 = __lvx( verts, j*DRAWVERT_SIZE+ 1*DRAWVERT_SIZE );
			__vector4 v2_0 = __lvx( verts, j*DRAWVERT_SIZE+ 2*DRAWVERT_SIZE );
			__vector4 v3_0 = __lvx( verts, j*DRAWVERT_SIZE+ 3*DRAWVERT_SIZE );
			__vector4 v0_1 = __lvx( verts, j*DRAWVERT_SIZE+ 4*DRAWVERT_SIZE );
			__vector4 v1_1 = __lvx( verts, j*DRAWVERT_SIZE+ 5*DRAWVERT_SIZE );
			__vector4 v2_1 = __lvx( verts, j*DRAWVERT_SIZE+ 6*DRAWVERT_SIZE );
			__vector4 v3_1 = __lvx( verts, j*DRAWVERT_SIZE+ 7*DRAWVERT_SIZE );
			__vector4 v0_2 = __lvx( verts, j*DRAWVERT_SIZE+ 8*DRAWVERT_SIZE );
			__vector4 v1_2 = __lvx( verts, j*DRAWVERT_SIZE+ 9*DRAWVERT_SIZE );
			__vector4 v2_2 = __lvx( verts, j*DRAWVERT_SIZE+10*DRAWVERT_SIZE );
			__vector4 v3_2 = __lvx( verts, j*DRAWVERT_SIZE+11*DRAWVERT_SIZE );
			__vector4 v0_3 = __lvx( verts, j*DRAWVERT_SIZE+12*DRAWVERT_SIZE );
			__vector4 v1_3 = __lvx( verts, j*DRAWVERT_SIZE+13*DRAWVERT_SIZE );
			__vector4 v2_3 = __lvx( verts, j*DRAWVERT_SIZE+14*DRAWVERT_SIZE );
			__vector4 v3_3 = __lvx( verts, j*DRAWVERT_SIZE+15*DRAWVERT_SIZE );

			v0_0 = __vand( v0_0, vmx_dword_mask_clear_last );
			v1_0 = __vand( v1_0, vmx_dword_mask_clear_last );
			v2_0 = __vand( v2_0, vmx_dword_mask_clear_last );
			v3_0 = __vand( v3_0, vmx_dword_mask_clear_last );
			v0_1 = __vand( v0_1, vmx_dword_mask_clear_last );
			v1_1 = __vand( v1_1, vmx_dword_mask_clear_last );
			v2_1 = __vand( v2_1, vmx_dword_mask_clear_last );
			v3_1 = __vand( v3_1, vmx_dword_mask_clear_last );
			v0_2 = __vand( v0_2, vmx_dword_mask_clear_last );
			v1_2 = __vand( v1_2, vmx_dword_mask_clear_last );
			v2_2 = __vand( v2_2, vmx_dword_mask_clear_last );
			v3_2 = __vand( v3_2, vmx_dword_mask_clear_last );
			v0_3 = __vand( v0_3, vmx_dword_mask_clear_last );
			v1_3 = __vand( v1_3, vmx_dword_mask_clear_last );
			v2_3 = __vand( v2_3, vmx_dword_mask_clear_last );
			v3_3 = __vand( v3_3, vmx_dword_mask_clear_last );

			v0_0 = __vor( v0_0, vmx_float_last_one );
			v1_0 = __vor( v1_0, vmx_float_last_one );
			v2_0 = __vor( v2_0, vmx_float_last_one );
			v3_0 = __vor( v3_0, vmx_float_last_one );
			v0_1 = __vor( v0_1, vmx_float_last_one );
			v1_1 = __vor( v1_1, vmx_float_last_one );
			v2_1 = __vor( v2_1, vmx_float_last_one );
			v3_1 = __vor( v3_1, vmx_float_last_one );
			v0_2 = __vor( v0_2, vmx_float_last_one );
			v1_2 = __vor( v1_2, vmx_float_last_one );
			v2_2 = __vor( v2_2, vmx_float_last_one );
			v3_2 = __vor( v3_2, vmx_float_last_one );
			v0_3 = __vor( v0_3, vmx_float_last_one );
			v1_3 = __vor( v1_3, vmx_float_last_one );
			v2_3 = __vor( v2_3, vmx_float_last_one );
			v3_3 = __vor( v3_3, vmx_float_last_one );

			__vector4 d0_0 = __vmsum4fp( plane, v0_0 );
			__vector4 d1_0 = __vmsum4fp( plane, v1_0 );
			__vector4 d2_0 = __vmsum4fp( plane, v2_0 );
			__vector4 d3_0 = __vmsum4fp( plane, v3_0 );
			__vector4 d0_1 = __vmsum4fp( plane, v0_1 );
			__vector4 d1_1 = __vmsum4fp( plane, v1_1 );
			__vector4 d2_1 = __vmsum4fp( plane, v2_1 );
			__vector4 d3_1 = __vmsum4fp( plane, v3_1 );
			__vector4 d0_2 = __vmsum4fp( plane, v0_2 );
			__vector4 d1_2 = __vmsum4fp( plane, v1_2 );
			__vector4 d2_2 = __vmsum4fp( plane, v2_2 );
			__vector4 d3_2 = __vmsum4fp( plane, v3_2 );
			__vector4 d0_3 = __vmsum4fp( plane, v0_3 );
			__vector4 d1_3 = __vmsum4fp( plane, v1_3 );
			__vector4 d2_3 = __vmsum4fp( plane, v2_3 );
			__vector4 d3_3 = __vmsum4fp( plane, v3_3 );

			__vector4 b0_0 = __vcmpgtfp( d0_0, zero );
			__vector4 b1_0 = __vcmpgtfp( d1_0, zero );
			__vector4 b2_0 = __vcmpgtfp( d2_0, zero );
			__vector4 b3_0 = __vcmpgtfp( d3_0, zero );
			__vector4 b0_1 = __vcmpgtfp( d0_1, zero );
			__vector4 b1_1 = __vcmpgtfp( d1_1, zero );
			__vector4 b2_1 = __vcmpgtfp( d2_1, zero );
			__vector4 b3_1 = __vcmpgtfp( d3_1, zero );
			__vector4 b0_2 = __vcmpgtfp( d0_2, zero );
			__vector4 b1_2 = __vcmpgtfp( d1_2, zero );
			__vector4 b2_2 = __vcmpgtfp( d2_2, zero );
			__vector4 b3_2 = __vcmpgtfp( d3_2, zero );
			__vector4 b0_3 = __vcmpgtfp( d0_3, zero );
			__vector4 b1_3 = __vcmpgtfp( d1_3, zero );
			__vector4 b2_3 = __vcmpgtfp( d2_3, zero );
			__vector4 b3_3 = __vcmpgtfp( d3_3, zero );

			b0_0 = __vand( b0_0, (__vector4 &)mask0 );
			b1_0 = __vand( b1_0, (__vector4 &)mask1 );
			b2_0 = __vand( b2_0, (__vector4 &)mask2 );
			b3_0 = __vand( b3_0, (__vector4 &)mask3 );
			b0_1 = __vand( b0_1, (__vector4 &)mask0 );
			b1_1 = __vand( b1_1, (__vector4 &)mask1 );
			b2_1 = __vand( b2_1, (__vector4 &)mask2 );
			b3_1 = __vand( b3_1, (__vector4 &)mask3 );
			b0_2 = __vand( b0_2, (__vector4 &)mask0 );
			b1_2 = __vand( b1_2, (__vector4 &)mask1 );
			b2_2 = __vand( b2_2, (__vector4 &)mask2 );
			b3_2 = __vand( b3_2, (__vector4 &)mask3 );
			b0_3 = __vand( b0_3, (__vector4 &)mask0 );
			b1_3 = __vand( b1_3, (__vector4 &)mask1 );
			b2_3 = __vand( b2_3, (__vector4 &)mask2 );
			b3_3 = __vand( b3_3, (__vector4 &)mask3 );

			b0_0 = __vor( b0_0, b1_0 );
			b2_0 = __vor( b2_0, b3_0 );
			b0_0 = __vor( b0_0, b2_0 );
			b0_1 = __vor( b0_1, b1_1 );
			b2_1 = __vor( b2_1, b3_1 );
			b0_1 = __vor( b0_1, b2_1 );
			b0_2 = __vor( b0_2, b1_2 );
			b2_2 = __vor( b2_2, b3_2 );
			b0_2 = __vor( b0_2, b2_2 );
			b0_3 = __vor( b0_3, b1_3 );
			b2_3 = __vor( b2_3, b3_3 );
			b0_3 = __vor( b0_3, b2_3 );

			b0_0 = __vor( b0_0, bits );
			b0_1 = __vor( b0_1, bits );
			b0_2 = __vor( b0_2, bits );
			b0_3 = __vor( b0_3, bits );

	 		__stvewx( b0_0, cullBits, j+ 0 );
	 		__stvewx( b0_1, cullBits, j+ 4 );
	 		__stvewx( b0_2, cullBits, j+ 8 );
	 		__stvewx( b0_3, cullBits, j+12 );
		}

		for ( ; j < numVerts - 3; j += 4 ) {
	 		__vector4 bits = __lvewx( cullBits, j );

			__vector4 v0_0 = __lvx( verts, j*DRAWVERT_SIZE+ 0*DRAWVERT_SIZE );
			__vector4 v1_0 = __lvx( verts, j*DRAWVERT_SIZE+ 1*DRAWVERT_SIZE );
			__vector4 v2_0 = __lvx( verts, j*DRAWVERT_SIZE+ 2*DRAWVERT_SIZE );
			__vector4 v3_0 = __lvx( verts, j*DRAWVERT_SIZE+ 3*DRAWVERT_SIZE );

			v0_0 = __vand( v0_0, vmx_dword_mask_clear_last );
			v1_0 = __vand( v1_0, vmx_dword_mask_clear_last );
			v2_0 = __vand( v2_0, vmx_dword_mask_clear_last );
			v3_0 = __vand( v3_0, vmx_dword_mask_clear_last );

			v0_0 = __vor( v0_0, vmx_float_last_one );
			v1_0 = __vor( v1_0, vmx_float_last_one );
			v2_0 = __vor( v2_0, vmx_float_last_one );
			v3_0 = __vor( v3_0, vmx_float_last_one );

			__vector4 d0_0 = __vmsum4fp( plane, v0_0 );
			__vector4 d1_0 = __vmsum4fp( plane, v1_0 );
			__vector4 d2_0 = __vmsum4fp( plane, v2_0 );
			__vector4 d3_0 = __vmsum4fp( plane, v3_0 );

			__vector4 b0_0 = __vcmpgtfp( d0_0, vmx_float_zero );
			__vector4 b1_0 = __vcmpgtfp( d1_0, vmx_float_zero );
			__vector4 b2_0 = __vcmpgtfp( d2_0, vmx_float_zero );
			__vector4 b3_0 = __vcmpgtfp( d3_0, vmx_float_zero );

			b0_0 = __vand( b0_0, (__vector4 &)mask0 );
			b1_0 = __vand( b1_0, (__vector4 &)mask1 );
			b2_0 = __vand( b2_0, (__vector4 &)mask2 );
			b3_0 = __vand( b3_0, (__vector4 &)mask3 );

			b0_0 = __vor( b0_0, b1_0 );
			b2_0 = __vor( b2_0, b3_0 );
			b0_0 = __vor( b0_0, b2_0 );

			b0_0 = __vor( b0_0, bits );

	 		__stvewx( b0_0, cullBits, j );
		}

		for ( ; j < numVerts; j++ ) {
			int bit = lightPlanes[i][0] * verts[j].xyz.x + lightPlanes[i][1] * verts[j].xyz.y + lightPlanes[i][2] * verts[j].xyz.z + lightPlanes[i][3] < 0.0f;
			cullBits[j] |= bit << i;
		}
	}
}

/*
============
idSIMD_Xenon::CreateShadowCache
============
*/
int VPCALL idSIMD_Xenon::CreateShadowCache( idVec4 *__restrict vertexCache, const idDrawVert *__restrict verts, const int numVerts ) {
	const idDrawVert *end = verts + numVerts;

	assert_16_byte_aligned( vertexCache );
	assert_16_byte_aligned( verts );
	assert_sizeof_16_byte_multiple( idVec4 );
	assert_sizeof_16_byte_multiple( idDrawVert );

	__vector4 clear_last = vmx_dword_mask_clear_last;
	__vector4 last_one = vmx_float_last_one;

	for( ; verts + 7 < end; verts += 8, vertexCache += 2*8 ) {
		__vector4 v0 = (__vector4 &) verts[0].xyz;
		__vector4 v1 = (__vector4 &) verts[1].xyz;
		__vector4 v2 = (__vector4 &) verts[2].xyz;
		__vector4 v3 = (__vector4 &) verts[3].xyz;
		__vector4 v4 = (__vector4 &) verts[4].xyz;
		__vector4 v5 = (__vector4 &) verts[5].xyz;
		__vector4 v6 = (__vector4 &) verts[6].xyz;
		__vector4 v7 = (__vector4 &) verts[7].xyz;

		__vector4 b0 = __vand( v0, clear_last );
		__vector4 b1 = __vand( v1, clear_last );
		__vector4 b2 = __vand( v2, clear_last );
		__vector4 b3 = __vand( v3, clear_last );
		__vector4 b4 = __vand( v4, clear_last );
		__vector4 b5 = __vand( v5, clear_last );
		__vector4 b6 = __vand( v6, clear_last );
		__vector4 b7 = __vand( v7, clear_last );

		__vector4 a0 = __vor( b0, last_one );
		__vector4 a1 = __vor( b1, last_one );
		__vector4 a2 = __vor( b2, last_one );
		__vector4 a3 = __vor( b3, last_one );
		__vector4 a4 = __vor( b4, last_one );
		__vector4 a5 = __vor( b5, last_one );
		__vector4 a6 = __vor( b6, last_one );
		__vector4 a7 = __vor( b7, last_one );

		__stvx( b0, vertexCache, 0*32+16 );
		__stvx( b1, vertexCache, 1*32+16 );
		__stvx( b2, vertexCache, 2*32+16 );
		__stvx( b3, vertexCache, 3*32+16 );
		__stvx( b4, vertexCache, 4*32+16 );
		__stvx( b5, vertexCache, 5*32+16 );
		__stvx( b6, vertexCache, 6*32+16 );
		__stvx( b7, vertexCache, 7*32+16 );

		__stvx( a0, vertexCache, 0*32+0 );
		__stvx( a1, vertexCache, 1*32+0 );
		__stvx( a2, vertexCache, 2*32+0 );
		__stvx( a3, vertexCache, 3*32+0 );
		__stvx( a4, vertexCache, 4*32+0 );
		__stvx( a5, vertexCache, 5*32+0 );
		__stvx( a6, vertexCache, 6*32+0 );
		__stvx( a7, vertexCache, 7*32+0 );
	}
	for( ; verts < end; verts++, vertexCache += 2 ) {
		__vector4 v0 = (__vector4 &) verts->xyz;

		__vector4 b0 = __vand( v0, clear_last );
		__vector4 a0 = __vor( b0, last_one );
		__stvx( a0, vertexCache, 0 );
		__stvx( b0, vertexCache, 16 );
	}
	return numVerts * 2;
}

/*
============
idSIMD_Xenon::ShadowVolume_CountFacing
============
*/
int VPCALL idSIMD_Xenon::ShadowVolume_CountFacing( const byte *__restrict facing, const int numFaces ) {
	int i, n0, n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12, n13, n14, n15;

	n0 = n1 = n2 = n3 = n4 = n5 = n6 = n7 = n8 = n9 = n10 = n11 = n12 = n13 = n14 = n15 = 0;
	for ( i = 0; i < numFaces-15; i += 16 ) {
		n0 += facing[i+0];
		n1 += facing[i+1];
		n2 += facing[i+2];
		n3 += facing[i+3];
		n4 += facing[i+4];
		n5 += facing[i+5];
		n6 += facing[i+6];
		n7 += facing[i+7];
		n8 += facing[i+8];
		n9 += facing[i+9];
		n10 += facing[i+10];
		n11 += facing[i+11];
		n12 += facing[i+12];
		n13 += facing[i+13];
		n14 += facing[i+14];
		n15 += facing[i+15];
	}
	for ( ; i < numFaces; i++ ) {
		n0 += facing[i];
	}
	return n0 + n1 + n2 + n3 + n4 + n5 + n6 + n7 + n8 + n9 + n10 + n11 + n12 + n13 + n14 + n15;
}

/*
============
idSIMD_Xenon::ShadowVolume_CountFacingCull
============
*/
int VPCALL idSIMD_Xenon::ShadowVolume_CountFacingCull( byte *__restrict facing, const int numFaces, const vertIndex_t *__restrict indexes, const byte *cull ) {
	int i, n0, n1, n2, n3;

	n0 = n1 = n2 = n3 = 0;
	for ( i = 0; i < numFaces - 3; i += 4 ) {

		int c0 = cull[indexes[0*3+0]] & cull[indexes[0*3+1]] & cull[indexes[0*3+2]];
		int c1 = cull[indexes[1*3+0]] & cull[indexes[1*3+1]] & cull[indexes[1*3+2]];
		int c2 = cull[indexes[2*3+0]] & cull[indexes[2*3+1]] & cull[indexes[2*3+2]];
		int c3 = cull[indexes[3*3+0]] & cull[indexes[3*3+1]] & cull[indexes[3*3+2]];

		int f0 = facing[i+0] | ( (-c0) >> 31 ) & 1;
		int f1 = facing[i+1] | ( (-c1) >> 31 ) & 1;
		int f2 = facing[i+2] | ( (-c2) >> 31 ) & 1;
		int f3 = facing[i+3] | ( (-c3) >> 31 ) & 1;

		n0 += f0;
		n1 += f1;
		n2 += f2;
		n3 += f3;

		facing[i+0] = f0;
		facing[i+1] = f1;
		facing[i+2] = f2;
		facing[i+3] = f3;

		indexes += 4*3;
	}
	for ( ; i < numFaces; i++ ) {
		int c = cull[indexes[0]] & cull[indexes[1]] & cull[indexes[2]];
		facing[i] |= ( (-c) >> 31 ) & 1;
		n0 += facing[i];
		indexes += 3;
	}
	return n0 + n1 + n2 + n3;
}

/*
============
idSIMD_Xenon::ShadowVolume_CreateSilTriangles
============
*/
int VPCALL idSIMD_Xenon::ShadowVolume_CreateSilTriangles( vertIndex_t *__restrict shadowIndexes, const byte *__restrict facing, const silEdge_t *__restrict silEdges, const int numSilEdges ) {
	const silEdge_t *__restrict sil, *__restrict end;
	vertIndex_t *__restrict si;
	byte inc[2] = { 0, 6 };

	si = shadowIndexes;
	end = silEdges + numSilEdges;
	for ( sil = silEdges; sil + 3 < end; sil += 4 ) {

		byte f1a = facing[sil[0].p1];
		byte f2a = facing[sil[0].p2];
		byte f1b = facing[sil[1].p1];
		byte f2b = facing[sil[1].p2];
		byte f1c = facing[sil[2].p1];
		byte f2c = facing[sil[2].p2];
		byte f1d = facing[sil[3].p1];
		byte f2d = facing[sil[3].p2];

		byte t0 = inc[ f1a ^ f2a ];
		int v1a = sil[0].v1;
		int v2a = sil[0].v2;

		si[0] = v1a;
		si[1] = v2a ^ f1a;
		si[2] = v2a ^ f2a;
		si[3] = v1a ^ f2a;
		si[4] = v1a ^ f1a;
		si[5] = v2a ^ 1;

		si += t0;

		byte t1 = inc[ f1b ^ f2b ];
		int v1b = sil[1].v1;
		int v2b = sil[1].v2;

		si[0] = v1b;
		si[1] = v2b ^ f1b;
		si[2] = v2b ^ f2b;
		si[3] = v1b ^ f2b;
		si[4] = v1b ^ f1b;
		si[5] = v2b ^ 1;

		si += t1;

		byte t2 = inc[ f1c ^ f2c ];
		int v1c = sil[2].v1;
		int v2c = sil[2].v2;

		si[0] = v1c;
		si[1] = v2c ^ f1c;
		si[2] = v2c ^ f2c;
		si[3] = v1c ^ f2c;
		si[4] = v1c ^ f1c;
		si[5] = v2c ^ 1;

		si += t2;

		byte t3 = inc[ f1d ^ f2d ];
		int v1d = sil[3].v1;
		int v2d = sil[3].v2;

		si[0] = v1d;
		si[1] = v2d ^ f1d;
		si[2] = v2d ^ f2d;
		si[3] = v1d ^ f2d;
		si[4] = v1d ^ f1d;
		si[5] = v2d ^ 1;

		si += t3;
	}
	for ( ; sil < end; sil++ ) {

		byte f1 = facing[sil->p1];
		byte f2 = facing[sil->p2];

		byte t = inc[ f1 ^ f2 ];
		int v1 = sil->v1;
		int v2 = sil->v2;

		si[0] = v1;
		si[1] = v2 ^ f1;
		si[2] = v2 ^ f2;
		si[3] = v1 ^ f2;
		si[4] = v1 ^ f1;
		si[5] = v2 ^ 1;

		si += t;
	}
	return si - shadowIndexes;
}

/*
============
idSIMD_Xenon::ShadowVolume_CreateSilTrianglesParallel
============
*/
int VPCALL idSIMD_Xenon::ShadowVolume_CreateSilTrianglesParallel( vertIndex_t *__restrict shadowIndexes, const byte *__restrict facing, const silEdge_t *__restrict silEdges, const int numSilEdges ) {
	const silEdge_t *__restrict sil, *__restrict end;
	vertIndex_t *__restrict si;
	byte inc[2] = { 0, 3 };

	si = shadowIndexes;
	end = silEdges + numSilEdges;
	for ( sil = silEdges; sil + 3 < end; sil += 4 ) {

		byte f1a = facing[sil[0].p1];
		byte f2a = facing[sil[0].p2];

		byte t0 = inc[ f1a ^ f2a ];
		int v1a = sil[0].v1;
		int v2a = sil[0].v2;
		int m0 = f1a - f2a;

		si[0] = v1a;
		si[1] = ( m0 & v2a ) | f1a;
		si[2] = ( ~m0 & v2a ) | f2a;

		si += t0;

		byte f1b = facing[sil[1].p1];
		byte f2b = facing[sil[1].p2];

		byte t1 = inc[ f1b ^ f2b ];
		int v1b = sil[1].v1;
		int v2b = sil[1].v2;
		int m1 = f1b - f2b;

		si[0] = v1b;
		si[1] = ( m1 & v2b ) | f1b;
		si[2] = ( ~m1 & v2b ) | f2b;

		si += t1;

		byte f1c = facing[sil[2].p1];
		byte f2c = facing[sil[2].p2];

		byte t2 = inc[ f1c ^ f2c ];
		int v1c = sil[2].v1;
		int v2c = sil[2].v2;
		int m2 = f1c - f2c;

		si[0] = v1c;
		si[1] = ( m2 & v2c ) | f1c;
		si[2] = ( ~m2 & v2c ) | f2c;

		si += t2;

		byte f1d = facing[sil[3].p1];
		byte f2d = facing[sil[3].p2];

		byte t3 = inc[ f1d ^ f2d ];
		int v1d = sil[3].v1;
		int v2d = sil[3].v2;
		int m3 = f1d - f2d;

		si[0] = v1d;
		si[1] = ( m3 & v2d ) | f1d;
		si[2] = ( ~m3 & v2d ) | f2d;

		si += t3;
	}

	for ( ; sil < end; sil++ ) {

		byte f1 = facing[sil->p1];
		byte f2 = facing[sil->p2];

		byte t = inc[ f1 ^ f2 ];
		int v1 = sil->v1;
		int v2 = sil->v2;
		int m = f1 - f2;

		si[0] = v1;
		si[1] = ( m & v2 ) | f1;
		si[2] = ( ~m & v2 ) | f2;

		si += t;
	}
	return si - shadowIndexes;
}

/*
============
idSIMD_Xenon::ShadowVolume_CreateCapTriangles
============
*/
int VPCALL idSIMD_Xenon::ShadowVolume_CreateCapTriangles( vertIndex_t *__restrict shadowIndexes, const byte *__restrict facing, const vertIndex_t *__restrict indexes, const int numIndexes ) {
	int i, j;
	vertIndex_t *__restrict si;
	byte inc[2] = { 6, 0 };

	si = shadowIndexes;
	for ( i = 0, j = 0; i < numIndexes - 3*4; i += 4*3, j += 4 ) {
		byte t0 = inc[facing[j+0]];
		byte t1 = inc[facing[j+1]];
		byte t2 = inc[facing[j+2]];
		byte t3 = inc[facing[j+3]];

		int i0 = indexes[i+0*3+0] << 1;
		int i1 = indexes[i+0*3+1] << 1;
		int i2 = indexes[i+0*3+2] << 1;

		si[0] = i2;
		si[1] = i1;
		si[2] = i0;
		si[3] = i0 + 1;
		si[4] = i1 + 1;
		si[5] = i2 + 1;

		si += t0;

		int i3 = indexes[i+1*3+0] << 1;
		int i4 = indexes[i+1*3+1] << 1;
		int i5 = indexes[i+1*3+2] << 1;

		si[0] = i5;
		si[1] = i4;
		si[2] = i3;
		si[3] = i3 + 1;
		si[4] = i4 + 1;
		si[5] = i5 + 1;

		si += t1;

		int i6 = indexes[i+2*3+0] << 1;
		int i7 = indexes[i+2*3+1] << 1;
		int i8 = indexes[i+2*3+2] << 1;

		si[0] = i8;
		si[1] = i7;
		si[2] = i6;
		si[3] = i6 + 1;
		si[4] = i7 + 1;
		si[5] = i8 + 1;

		si += t2;

		int i9  = indexes[i+3*3+0] << 1;
		int i10 = indexes[i+3*3+1] << 1;
		int i11 = indexes[i+3*3+2] << 1;

		si[0] = i11;
		si[1] = i10;
		si[2] = i9;
		si[3] = i9 + 1;
		si[4] = i10 + 1;
		si[5] = i11 + 1;

		si += t3;
	}

	for ( ; i < numIndexes; i += 3, j++ ) {
		byte t = inc[facing[j]];

		int i0 = indexes[i+0] << 1;
		int i1 = indexes[i+1] << 1;
		int i2 = indexes[i+2] << 1;

		si[0] = i2;
		si[1] = i1;
		si[2] = i0;
		si[3] = i0 + 1;
		si[4] = i1 + 1;
		si[5] = i2 + 1;

		si += t;
	}

	return si - shadowIndexes;
}

/*
============
idSIMD_Xenon::ShadowVolume_CreateCapTrianglesParallel
============
*/
int VPCALL idSIMD_Xenon::ShadowVolume_CreateCapTrianglesParallel( vertIndex_t *__restrict shadowIndexes, const byte *__restrict facing, const vertIndex_t *__restrict indexes, const int numIndexes ) {
	int i, j;
	vertIndex_t *__restrict si;
	byte inc[2] = { 3, 0 };

	si = shadowIndexes;
	for ( i = 0, j = 0; i < numIndexes - 3*4; i += 3*4, j += 4 ) {
		byte t0 = inc[facing[j+0]];
		byte t1 = inc[facing[j+1]];
		byte t2 = inc[facing[j+2]];
		byte t3 = inc[facing[j+3]];

		si[0] = indexes[i+0*3+2] << 1;
		si[1] = indexes[i+0*3+1] << 1;
		si[2] = indexes[i+0*3+0] << 1;

		si += t0;

		si[0] = indexes[i+1*3+2] << 1;
		si[1] = indexes[i+1*3+1] << 1;
		si[2] = indexes[i+1*3+0] << 1;

		si += t1;

		si[0] = indexes[i+2*3+2] << 1;
		si[1] = indexes[i+2*3+1] << 1;
		si[2] = indexes[i+2*3+0] << 1;

		si += t2;

		si[0] = indexes[i+3*3+2] << 1;
		si[1] = indexes[i+3*3+1] << 1;
		si[2] = indexes[i+3*3+0] << 1;

		si += t3;
	}
	for ( ; i < numIndexes; i += 3, j++ ) {
		byte t = inc[facing[j]];

		si[0] = indexes[i+2] << 1;
		si[1] = indexes[i+1] << 1;
		si[2] = indexes[i+0] << 1;

		si += t;
	}
	return si - shadowIndexes;
}

#endif
