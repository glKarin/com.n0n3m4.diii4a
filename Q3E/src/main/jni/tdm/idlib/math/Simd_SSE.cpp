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
#pragma warning(disable: 4740)

#include "Simd_SSE.h"


idSIMD_SSE::idSIMD_SSE() {
	name = "SSE";
}

#ifdef ENABLE_SSE_PROCESSORS

#include <xmmintrin.h>

//apply optimizations to this file in Debug with Inlines configuration
DEBUG_OPTIMIZE_ON

#define SHUF(i0, i1, i2, i3) _MM_SHUFFLE(i3, i2, i1, i0)

/*
============
idSIMD_SSE::CullByFrustum
============
*/
void idSIMD_SSE::CullByFrustum( idDrawVert *verts, const int numVerts, const idPlane frustum[6], byte *pointCull, float epsilon ) {
	__m128 fA14 = _mm_set_ps( frustum[3][0], frustum[2][0], frustum[1][0], frustum[0][0] );
	__m128 fA56 = _mm_set_ps( 0, 0, frustum[5][0], frustum[4][0] );
	__m128 fB14 = _mm_set_ps( frustum[3][1], frustum[2][1], frustum[1][1], frustum[0][1] );
	__m128 fB56 = _mm_set_ps( 0, 0, frustum[5][1], frustum[4][1] );
	__m128 fC14 = _mm_set_ps( frustum[3][2], frustum[2][2], frustum[1][2], frustum[0][2] );
	__m128 fC56 = _mm_set_ps( 0, 0, frustum[5][2], frustum[4][2] );
	__m128 fD14 = _mm_set_ps( frustum[3][3], frustum[2][3], frustum[1][3], frustum[0][3] );
	__m128 fD56 = _mm_set_ps( 0, 0, frustum[5][3], frustum[4][3] );
	for ( int j = 0; j < numVerts; j++ ) {
		auto &vec = verts[j].xyz;
		__m128 vX = _mm_set1_ps( vec.x );
		__m128 vY = _mm_set1_ps( vec.y );
		__m128 vZ = _mm_set1_ps( vec.z );
		__m128 d14 = _mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps( fA14, vX ),
				_mm_mul_ps( fB14, vY )
			),
			_mm_add_ps(
				_mm_mul_ps( fC14, vZ ),
				fD14
			)
		);
		__m128 d56 = _mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps( fA56, vX ),
				_mm_mul_ps( fB56, vY )
			),
			_mm_add_ps(
				_mm_mul_ps( fC56, vZ ),
				fD56
			)
		);
		const short mask6 = (1 << 6) - 1;
		__m128 eps = _mm_set1_ps( epsilon );
		int mask_lo14 = _mm_movemask_ps( _mm_cmplt_ps( d14, eps ) );
		int mask_lo56 = _mm_movemask_ps( _mm_cmplt_ps( d56, eps ) );
		int mask_lo = mask_lo14 | mask_lo56 << 4;
		pointCull[j] = mask_lo & mask6;
	}
}

/*
============
idSIMD_SSE::CullByFrustum2
============
*/
void idSIMD_SSE::CullByFrustum2( idDrawVert *verts, const int numVerts, const idPlane frustum[6], unsigned short *pointCull, float epsilon ) {
	__m128 fA14 = _mm_set_ps( frustum[3][0], frustum[2][0], frustum[1][0], frustum[0][0] );
	__m128 fA56 = _mm_set_ps( 0, 0, frustum[5][0], frustum[4][0] );
	__m128 fB14 = _mm_set_ps( frustum[3][1], frustum[2][1], frustum[1][1], frustum[0][1] );
	__m128 fB56 = _mm_set_ps( 0, 0, frustum[5][1], frustum[4][1] );
	__m128 fC14 = _mm_set_ps( frustum[3][2], frustum[2][2], frustum[1][2], frustum[0][2] );
	__m128 fC56 = _mm_set_ps( 0, 0, frustum[5][2], frustum[4][2] );
	__m128 fD14 = _mm_set_ps( frustum[3][3], frustum[2][3], frustum[1][3], frustum[0][3] );
	__m128 fD56 = _mm_set_ps( 0, 0, frustum[5][3], frustum[4][3] );
	const __m128 eps  = _mm_set1_ps(  epsilon );
	const __m128 epsM = _mm_set1_ps( -epsilon );
	for ( int j = 0; j < numVerts; j++ ) {
		auto &vec = verts[j].xyz;
		__m128 vX = _mm_set1_ps( vec.x );
		__m128 vY = _mm_set1_ps( vec.y );
		__m128 vZ = _mm_set1_ps( vec.z );
		__m128 d14 = _mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps( fA14, vX ),
				_mm_mul_ps( fB14, vY )
			),
			_mm_add_ps(
				_mm_mul_ps( fC14, vZ ),
				fD14
			)
		);
		__m128 d56 = _mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps( fA56, vX ),
				_mm_mul_ps( fB56, vY )
			),
			_mm_add_ps(
				_mm_mul_ps( fC56, vZ ),
				fD56
			)
		);
		const short mask6 = (1 << 6) - 1;
		int mask_lo14 = _mm_movemask_ps( _mm_cmplt_ps( d14, eps  ) );
		int mask_lo56 = _mm_movemask_ps( _mm_cmplt_ps( d56, eps  ) );
		int mask_hi14 = _mm_movemask_ps( _mm_cmpgt_ps( d14, epsM ) );
		int mask_hi56 = _mm_movemask_ps( _mm_cmpgt_ps( d56, epsM ) );
		int mask_lo = mask_lo14 | mask_lo56 << 4;
		int mask_hi = mask_hi14 | mask_hi56 << 4;
		pointCull[j] = mask_lo & mask6 | (mask_hi & mask6) << 6;
	}
}

/*
============
idSIMD_SSE::CullTrisByFrustum
============
*/
void idSIMD_SSE::CullTrisByFrustum( idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes, const idPlane frustum[6], byte *triCull, float epsilon ) {
	assert(numIndexes % 3 == 0);
	if (numIndexes <= 0)
		return;

	//note: initially I tried to detect same vertices being usede in adjacent triangles, so that I can process less vertices
	//however, it turned out that:
	//  touching vertex memory is most expensive part of vertex processing
	//  doing all the math for a vertex is actually cheap
	//  searching for vertex reuses needs branches and thus is expensive
	//so it's better to simply process all vertices, and let CPU cache optimize out excessive memory loading

	//prepare plane data
	__m128 planesA0123 = _mm_loadu_ps(frustum[0].ToFloatPtr());
	__m128 planesB0123 = _mm_loadu_ps(frustum[1].ToFloatPtr());
	__m128 planesC0123 = _mm_loadu_ps(frustum[2].ToFloatPtr());
	__m128 planesD0123 = _mm_loadu_ps(frustum[3].ToFloatPtr());
	_MM_TRANSPOSE4_PS(planesA0123, planesB0123, planesC0123, planesD0123);
	__m128 planesA45;
	__m128 planesB45;
	__m128 planesC45;
	__m128 planesD45;
	{
		__m128 planes4 = _mm_loadu_ps(frustum[0].ToFloatPtr());
		__m128 planes5 = _mm_loadu_ps(frustum[1].ToFloatPtr());
		//partial transpose (2x4 + zeros)
		__m128 P4L = _mm_unpacklo_ps(planes4, _mm_setzero_ps());
		__m128 P4H = _mm_unpackhi_ps(planes4, _mm_setzero_ps());
		__m128 P5L = _mm_unpacklo_ps(planes5, _mm_setzero_ps());
		__m128 P5H = _mm_unpackhi_ps(planes5, _mm_setzero_ps());
		planesA45 = _mm_unpacklo_ps(P4L, P5L);
		planesB45 = _mm_unpackhi_ps(P4L, P5L);
		planesC45 = _mm_unpacklo_ps(P4H, P5H);
		planesD45 = _mm_unpackhi_ps(P4H, P5H);
	}
	__m128 eps = _mm_set1_ps(epsilon);

	//process triangles
	int numTris = numIndexes / 3;
	for (int i = 0; i < numTris; i++) {
		const idDrawVert &vert0 = verts[indexes[3 * i + 0]];
		const idDrawVert &vert1 = verts[indexes[3 * i + 1]];
		const idDrawVert &vert2 = verts[indexes[3 * i + 2]];
		int cull0, cull1, cull2;

		#define PROCESS(k) { \
			__m128 pos = _mm_loadu_ps( &vert##k.xyz.x ); \
			\
			__m128 X = _mm_shuffle_ps(pos, pos, SHUF(0, 0, 0, 0)); \
			__m128 Y = _mm_shuffle_ps(pos, pos, SHUF(1, 1, 1, 1)); \
			__m128 Z = _mm_shuffle_ps(pos, pos, SHUF(2, 2, 2, 2)); \
			\
			__m128 dist0123 = _mm_add_ps( \
				_mm_add_ps( \
					_mm_mul_ps(planesA0123, X), \
					_mm_mul_ps(planesB0123, Y) \
				), \
				_mm_add_ps( \
					_mm_mul_ps(planesC0123, Z), \
					planesD0123 \
				) \
			); \
			__m128 dist45 = _mm_add_ps( \
				_mm_add_ps( \
					_mm_mul_ps(planesA45, X), \
					_mm_mul_ps(planesB45, Y) \
				), \
				_mm_add_ps( \
					_mm_mul_ps(planesC45, Z), \
					planesD45 \
				) \
			); \
			\
			int mask0123 = _mm_movemask_ps(_mm_cmplt_ps(dist0123, eps)); \
			int mask45 = _mm_movemask_ps(_mm_cmplt_ps(dist45, eps)); \
			cull##k = mask0123 + (mask45 << 4); \
		}

		PROCESS(0);
		PROCESS(1);
		PROCESS(2);

		triCull[i] = cull0 & cull1 & cull2 & ((1 << 6) - 1);
	}
}

#endif
