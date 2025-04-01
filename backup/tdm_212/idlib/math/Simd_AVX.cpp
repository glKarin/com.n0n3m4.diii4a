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

#include "Simd_AVX.h"


idSIMD_AVX::idSIMD_AVX() {
	name = "AVX";
}

#ifdef ENABLE_SSE_PROCESSORS

#include <immintrin.h>

//===============================================================
//
//	AVX implementation of idSIMDProcessor
//
//===============================================================

//apply optimizations to this file in Debug with Inlines configuration
DEBUG_OPTIMIZE_ON

/*
============
idSIMD_AVX::CullByFrustum
============
*/
void idSIMD_AVX::CullByFrustum( idDrawVert *verts, const int numVerts, const idPlane frustum[6], byte *pointCull, float epsilon ) {
	const __m256 fA = _mm256_set_ps( 0, 0, frustum[5][0], frustum[4][0], frustum[3][0], frustum[2][0], frustum[1][0], frustum[0][0] );
	const __m256 fB = _mm256_set_ps( 0, 0, frustum[5][1], frustum[4][1], frustum[3][1], frustum[2][1], frustum[1][1], frustum[0][1] );
	const __m256 fC = _mm256_set_ps( 0, 0, frustum[5][2], frustum[4][2], frustum[3][2], frustum[2][2], frustum[1][2], frustum[0][2] );
	const __m256 fD = _mm256_set_ps( 0, 0, frustum[5][3], frustum[4][3], frustum[3][3], frustum[2][3], frustum[1][3], frustum[0][3] );
	const __m256 eps = _mm256_set1_ps( epsilon );
	const byte mask6 = (1 << 6) - 1;
	for ( int j = 0; j < numVerts; j++ ) {
		auto &vec = verts[j].xyz;
		__m256 vX = _mm256_set1_ps( vec.x );
		__m256 vY = _mm256_set1_ps( vec.y );
		__m256 vZ = _mm256_set1_ps( vec.z );
		__m256 d = _mm256_add_ps(
			_mm256_add_ps(
				_mm256_mul_ps( fA, vX ),
				_mm256_mul_ps( fB, vY )
			),
			_mm256_add_ps(
				_mm256_mul_ps( fC, vZ ),
				fD
			)
		);
		int mask_lo = _mm256_movemask_ps( _mm256_cmp_ps( d, eps, _CMP_LT_OQ ) );
		pointCull[j] = (byte)mask_lo & mask6;
	}
	_mm256_zeroupper();
}

/*
============
idSIMD_AVX::CullByFrustum2
============
*/
void idSIMD_AVX::CullByFrustum2( idDrawVert *verts, const int numVerts, const idPlane frustum[6], unsigned short *pointCull, float epsilon ) {
	const __m256 fA = _mm256_set_ps( 0, 0, frustum[5][0], frustum[4][0], frustum[3][0], frustum[2][0], frustum[1][0], frustum[0][0] );
	const __m256 fB = _mm256_set_ps( 0, 0, frustum[5][1], frustum[4][1], frustum[3][1], frustum[2][1], frustum[1][1], frustum[0][1] );
	const __m256 fC = _mm256_set_ps( 0, 0, frustum[5][2], frustum[4][2], frustum[3][2], frustum[2][2], frustum[1][2], frustum[0][2] );
	const __m256 fD = _mm256_set_ps( 0, 0, frustum[5][3], frustum[4][3], frustum[3][3], frustum[2][3], frustum[1][3], frustum[0][3] );
	const __m256 eps  = _mm256_set1_ps(  epsilon );
	const __m256 epsM = _mm256_set1_ps( -epsilon );
	const short mask6 = (1 << 6) - 1;
	for ( int j = 0; j < numVerts; j++ ) {
		auto &vec = verts[j].xyz;
		__m256 vX = _mm256_set1_ps( vec.x );
		__m256 vY = _mm256_set1_ps( vec.y );
		__m256 vZ = _mm256_set1_ps( vec.z );
		__m256 d = _mm256_add_ps(
			_mm256_add_ps(
				_mm256_mul_ps( fA, vX ),
				_mm256_mul_ps( fB, vY )
			),
			_mm256_add_ps(
				_mm256_mul_ps( fC, vZ ),
				fD
			)
		);
		int mask_lo = _mm256_movemask_ps( _mm256_cmp_ps( d, eps , _CMP_LT_OQ ) );
		int mask_hi = _mm256_movemask_ps( _mm256_cmp_ps( d, epsM, _CMP_GT_OQ ) );
		pointCull[j] = (unsigned short)(mask_lo & mask6 | (mask_hi & mask6) << 6);
	}
	_mm256_zeroupper();
}

#endif
