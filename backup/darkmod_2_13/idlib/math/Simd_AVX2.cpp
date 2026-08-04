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

#include "Simd_AVX2.h"


idSIMD_AVX2::idSIMD_AVX2() {
	name = "AVX2";
}

#ifdef ENABLE_SSE_PROCESSORS

#include <immintrin.h>

//===============================================================
//
//	AVX2 implementation of idSIMDProcessor
//
//===============================================================

//apply optimizations to this file in Debug with Inlines configuration
DEBUG_OPTIMIZE_ON

/*
============
idSIMD_AVX2::CullByFrustum
============
*/
void idSIMD_AVX2::CullByFrustum( idDrawVert *verts, const int numVerts, const idPlane frustum[6], byte *pointCull, float epsilon ) {
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
		__m256 d = _mm256_fmadd_ps( fA, vX,
			_mm256_fmadd_ps( fB, vY,
				_mm256_fmadd_ps( fC, vZ, fD )
			) );
		int mask_lo = _mm256_movemask_ps( _mm256_cmp_ps( d, eps, _CMP_LT_OQ ) );
		pointCull[j] = (byte)mask_lo & mask6;
	}
	_mm256_zeroupper();
}

/*
============
idSIMD_AVX2::CullByFrustum2
============
*/
void idSIMD_AVX2::CullByFrustum2( idDrawVert *verts, const int numVerts, const idPlane frustum[6], unsigned short *pointCull, float epsilon ) {
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
		__m256 d = _mm256_fmadd_ps( fA, vX,
			_mm256_fmadd_ps( fB, vY,
				_mm256_fmadd_ps( fC, vZ, fD )
			) );
		int mask_lo = _mm256_movemask_ps( _mm256_cmp_ps( d, eps , _CMP_LT_OQ ) );
		int mask_hi = _mm256_movemask_ps( _mm256_cmp_ps( d, epsM, _CMP_GT_OQ ) );
		pointCull[j] = (unsigned short)(mask_lo & mask6 | (mask_hi & mask6) << 6);
	}
	_mm256_zeroupper();
}

#define SHUF(i0, i1, i2, i3) _MM_SHUFFLE(i3, i2, i1, i0)

#define DECL3_P8(Res) __m256 Res##_x, Res##_y, Res##_z;

#define CROSS3_P8(Res, A, B) \
	Res##_x = _mm256_fmsub_ps(A##_y, B##_z, _mm256_mul_ps(A##_z, B##_y)); \
	Res##_y = _mm256_fmsub_ps(A##_z, B##_x, _mm256_mul_ps(A##_x, B##_z)); \
	Res##_z = _mm256_fmsub_ps(A##_x, B##_y, _mm256_mul_ps(A##_y, B##_x)); \

#define DOT3_P8(Res, A, B) \
	Res = _mm256_fmadd_ps(A##_x, B##_x, _mm256_fmadd_ps(A##_y, B##_y, _mm256_mul_ps(A##_z, B##_z))); \

#define MUL3S_P8(Res, A, S) \
	Res##_x = _mm256_mul_ps(A##_x, S); \
	Res##_y = _mm256_mul_ps(A##_y, S); \
	Res##_z = _mm256_mul_ps(A##_z, S); \

void idSIMD_AVX2::DeriveTangents( idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) {
	for (int i = 0; i < numVerts; i++) {
		float *ptr = &verts[i].normal.x;
		_mm256_storeu_ps(ptr, _mm256_setzero_ps());
		_mm_store_ss(ptr + 8, _mm_setzero_ps());
	}

	#define NORMAL_EPS 1e-10f
	size_t i, n = numIndexes / 3;
	unsigned *unsIndexed = (unsigned*)indexes;
	for (i = 0; i + 8 <= n; i += 8) {
		//load all 3 vertices (A, B, C) of 8 triangles
		//get side vectors immediately: AB and AC
		#define LOAD(k) \
			idDrawVert &dvA##k = verts[unsIndexed[3 * i + (3 * k + 0)]]; \
			idDrawVert &dvB##k = verts[unsIndexed[3 * i + (3 * k + 1)]]; \
			idDrawVert &dvC##k = verts[unsIndexed[3 * i + (3 * k + 2)]]; \
			__m256 xyzstA##k = _mm256_loadu_ps(&dvA##k.xyz.x); \
			__m256 xyzstAB##k = _mm256_sub_ps(_mm256_loadu_ps(&dvB##k.xyz.x), xyzstA##k); \
			__m256 xyzstAC##k = _mm256_sub_ps(_mm256_loadu_ps(&dvC##k.xyz.x), xyzstA##k);
		LOAD(0);
		LOAD(1);
		LOAD(2);
		LOAD(3);
		LOAD(4);
		LOAD(5);
		LOAD(6);
		LOAD(7);
		#undef LOAD

		//convert from AoS to SoA layout
		//for every side (AB, AC) and every component (x, y, z, s, t), we would have a pack with eight values
		#define TRANSPOSE8x5(v) \
		__m256 v##_x, v##_y, v##_z, v##_s, v##_t; \
		{ \
			__m256 xyzs04 = _mm256_permute2f128_ps(xyzst##v##0, xyzst##v##4, SHUF(0,0,2,0)); \
			__m256 xyzs15 = _mm256_permute2f128_ps(xyzst##v##1, xyzst##v##5, SHUF(0,0,2,0)); \
			__m256 xyzs26 = _mm256_permute2f128_ps(xyzst##v##2, xyzst##v##6, SHUF(0,0,2,0)); \
			__m256 xyzs37 = _mm256_permute2f128_ps(xyzst##v##3, xyzst##v##7, SHUF(0,0,2,0)); \
			__m256 xy0145 = _mm256_unpacklo_ps(xyzs04, xyzs15); \
			__m256 zs0145 = _mm256_unpackhi_ps(xyzs04, xyzs15); \
			__m256 xy2367 = _mm256_unpacklo_ps(xyzs26, xyzs37); \
			__m256 zs2367 = _mm256_unpackhi_ps(xyzs26, xyzs37); \
			v##_x = _mm256_shuffle_ps(xy0145, xy2367, SHUF(0,1,0,1)); \
			v##_y = _mm256_shuffle_ps(xy0145, xy2367, SHUF(2,3,2,3)); \
			v##_z = _mm256_shuffle_ps(zs0145, zs2367, SHUF(0,1,0,1)); \
			v##_s = _mm256_shuffle_ps(zs0145, zs2367, SHUF(2,3,2,3)); \
			__m256 t___04 = _mm256_permute2f128_ps(xyzst##v##0, xyzst##v##4, SHUF(1, 0, 3, 0)); \
			__m256 t___15 = _mm256_permute2f128_ps(xyzst##v##1, xyzst##v##5, SHUF(1, 0, 3, 0)); \
			__m256 t___26 = _mm256_permute2f128_ps(xyzst##v##2, xyzst##v##6, SHUF(1, 0, 3, 0)); \
			__m256 t___37 = _mm256_permute2f128_ps(xyzst##v##3, xyzst##v##7, SHUF(1, 0, 3, 0)); \
			__m256 t_0145 = _mm256_unpacklo_ps(t___04, t___15); \
			__m256 t_2367 = _mm256_unpacklo_ps(t___26, t___37); \
			v##_t = _mm256_shuffle_ps(t_0145, t_2367, SHUF(0,1,0,1)); \
		}
		TRANSPOSE8x5(AB);
		TRANSPOSE8x5(AC);
		#undef TRANSPOSE8x5
		__m256 A_x, A_y, A_z;
		{
			__m256 xyzs04 = _mm256_permute2f128_ps(xyzstA0, xyzstA4, SHUF(0, 0, 2, 0));
			__m256 xyzs15 = _mm256_permute2f128_ps(xyzstA1, xyzstA5, SHUF(0, 0, 2, 0));
			__m256 xyzs26 = _mm256_permute2f128_ps(xyzstA2, xyzstA6, SHUF(0, 0, 2, 0));
			__m256 xyzs37 = _mm256_permute2f128_ps(xyzstA3, xyzstA7, SHUF(0, 0, 2, 0));
			__m256 xy0145 = _mm256_unpacklo_ps(xyzs04, xyzs15);
			__m256 zs0145 = _mm256_unpackhi_ps(xyzs04, xyzs15);
			__m256 xy2367 = _mm256_unpacklo_ps(xyzs26, xyzs37);
			__m256 zs2367 = _mm256_unpackhi_ps(xyzs26, xyzs37);
			A_x = _mm256_shuffle_ps(xy0145, xy2367, SHUF(0,1,0,1));
			A_y = _mm256_shuffle_ps(xy0145, xy2367, SHUF(2,3,2,3));
			A_z = _mm256_shuffle_ps(zs0145, zs2367, SHUF(0,1,0,1));
		}

		//check area sign
		__m256 area = _mm256_fmsub_ps(AB_s, AC_t, _mm256_mul_ps(AC_s, AB_t));
		__m256 sign = _mm256_and_ps(area, _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000)));

		//compute normal unit vector
		DECL3_P8(normal);
		CROSS3_P8(normal, AC, AB);
		__m256 normalSqr;
		DOT3_P8(normalSqr, normal, normal);
		normalSqr = _mm256_max_ps(normalSqr, _mm256_set1_ps(NORMAL_EPS));
		normalSqr = _mm256_rsqrt_ps(normalSqr);
		//fit plane though point A
		__m256 planeShift;
		DOT3_P8(planeShift, A, normal);
		planeShift = _mm256_xor_ps(planeShift, _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000)));
		planeShift = _mm256_mul_ps(planeShift, normalSqr);
		//end: normalize normal vector
		MUL3S_P8(normal, normal, normalSqr);

		//compute first tangent
		__m256 tangU_x = _mm256_fmsub_ps(AB_x, AC_t, _mm256_mul_ps(AC_x, AB_t));
		__m256 tangU_y = _mm256_fmsub_ps(AB_y, AC_t, _mm256_mul_ps(AC_y, AB_t));
		__m256 tangU_z = _mm256_fmsub_ps(AB_z, AC_t, _mm256_mul_ps(AC_z, AB_t));
		__m256 tangUlen;
		DOT3_P8(tangUlen, tangU, tangU);
		tangUlen = _mm256_max_ps(tangUlen, _mm256_set1_ps(NORMAL_EPS));
		tangUlen = _mm256_rsqrt_ps(tangUlen);
		tangUlen = _mm256_xor_ps(tangUlen, sign);
		MUL3S_P8(tangU, tangU, tangUlen);

		//compute second tangent
		__m256 tangV_x = _mm256_fmsub_ps(AC_x, AB_s, _mm256_mul_ps(AB_x, AC_s));
		__m256 tangV_y = _mm256_fmsub_ps(AC_y, AB_s, _mm256_mul_ps(AB_y, AC_s));
		__m256 tangV_z = _mm256_fmsub_ps(AC_z, AB_s, _mm256_mul_ps(AB_z, AC_s));
		__m256 tangVlen;
		DOT3_P8(tangVlen, tangV, tangV);
		tangVlen = _mm256_max_ps(tangVlen, _mm256_set1_ps(NORMAL_EPS));
		tangVlen = _mm256_rsqrt_ps(tangVlen);
		tangVlen = _mm256_xor_ps(tangVlen, sign);
		MUL3S_P8(tangV, tangV, tangVlen);

		__m256 xy0145, xy2367;
		{ //save plane equation
			       xy0145 = _mm256_unpacklo_ps(normal_x, normal_y);
			       xy2367 = _mm256_unpackhi_ps(normal_x, normal_y);
			__m256 zd0145 = _mm256_unpacklo_ps(normal_z, planeShift);
			__m256 zd2367 = _mm256_unpackhi_ps(normal_z, planeShift);
			__m256 xyzd04 = _mm256_shuffle_ps(xy0145, zd0145, SHUF(0,1,0,1));
			__m256 xyzd15 = _mm256_shuffle_ps(xy0145, zd0145, SHUF(2,3,2,3));
			__m256 xyzd26 = _mm256_shuffle_ps(xy2367, zd2367, SHUF(0,1,0,1));
			__m256 xyzd37 = _mm256_shuffle_ps(xy2367, zd2367, SHUF(2,3,2,3));
			_mm_storeu_ps(planes[i+0].ToFloatPtr(), _mm256_castps256_ps128(xyzd04));
			_mm_storeu_ps(planes[i+1].ToFloatPtr(), _mm256_castps256_ps128(xyzd15));
			_mm_storeu_ps(planes[i+2].ToFloatPtr(), _mm256_castps256_ps128(xyzd26));
			_mm_storeu_ps(planes[i+3].ToFloatPtr(), _mm256_castps256_ps128(xyzd37));
			_mm_storeu_ps(planes[i+4].ToFloatPtr(),  _mm256_extractf128_ps(xyzd04, 1));
			_mm_storeu_ps(planes[i+5].ToFloatPtr(),  _mm256_extractf128_ps(xyzd15, 1));
			_mm_storeu_ps(planes[i+6].ToFloatPtr(),  _mm256_extractf128_ps(xyzd26, 1));
			_mm_storeu_ps(planes[i+7].ToFloatPtr(),  _mm256_extractf128_ps(xyzd37, 1));
		}

		{ //add normals and tangents to stored results
			__m256 ab0145 = xy0145; //_mm256_unpacklo_ps(normal_x, normal_y);
			__m256 ab2367 = xy2367; //_mm256_unpackhi_ps(normal_x, normal_y);
			__m256 cd0145 = _mm256_unpacklo_ps(normal_z, tangU_x);
			__m256 cd2367 = _mm256_unpackhi_ps(normal_z, tangU_x);
			__m256 ef0145 = _mm256_unpacklo_ps(tangU_y, tangU_z);
			__m256 ef2367 = _mm256_unpackhi_ps(tangU_y, tangU_z);
			__m256 gh0145 = _mm256_unpacklo_ps(tangV_x, tangV_y);
			__m256 gh2367 = _mm256_unpackhi_ps(tangV_x, tangV_y);
			__m256 abcd04 = _mm256_shuffle_ps(ab0145, cd0145, SHUF(0,1,0,1));
			__m256 abcd15 = _mm256_shuffle_ps(ab0145, cd0145, SHUF(2,3,2,3));
			__m256 efgh04 = _mm256_shuffle_ps(ef0145, gh0145, SHUF(0,1,0,1));
			__m256 efgh15 = _mm256_shuffle_ps(ef0145, gh0145, SHUF(2,3,2,3));
			__m256 abcd26 = _mm256_shuffle_ps(ab2367, cd2367, SHUF(0,1,0,1));
			__m256 abcd37 = _mm256_shuffle_ps(ab2367, cd2367, SHUF(2,3,2,3));
			__m256 efgh26 = _mm256_shuffle_ps(ef2367, gh2367, SHUF(0,1,0,1));
			__m256 efgh37 = _mm256_shuffle_ps(ef2367, gh2367, SHUF(2,3,2,3));
			__m256 all0 = _mm256_permute2f128_ps(abcd04, efgh04, SHUF(0,0,2,0));
			__m256 all1 = _mm256_permute2f128_ps(abcd15, efgh15, SHUF(0,0,2,0));
			__m256 all2 = _mm256_permute2f128_ps(abcd26, efgh26, SHUF(0,0,2,0));
			__m256 all3 = _mm256_permute2f128_ps(abcd37, efgh37, SHUF(0,0,2,0));
			__m256 all4 = _mm256_permute2f128_ps(abcd04, efgh04, SHUF(1,0,3,0));
			__m256 all5 = _mm256_permute2f128_ps(abcd15, efgh15, SHUF(1,0,3,0));
			__m256 all6 = _mm256_permute2f128_ps(abcd26, efgh26, SHUF(1,0,3,0));
			__m256 all7 = _mm256_permute2f128_ps(abcd37, efgh37, SHUF(1,0,3,0));
			__m128 i0123 = _mm256_castps256_ps128(tangV_z);
			__m128 i4567 = _mm256_extractf128_ps(tangV_z, 1);
			#define STOREADD(k, v) {\
				float *ptr = &dv##v##k.normal.x; \
				_mm256_storeu_ps(ptr, _mm256_add_ps(_mm256_loadu_ps(ptr), all##k)); \
				_mm_store_ss(ptr + 8, _mm_add_ss(_mm_load_ss(ptr + 8), i##k)); \
			}
			__m128 i0 = i0123;
			STOREADD(0, A);
			STOREADD(0, B);
			STOREADD(0, C);
			__m128 i1 = _mm_shuffle_ps(i0123, i0123, SHUF(1,1,1,1));
			STOREADD(1, A);
			STOREADD(1, B);
			STOREADD(1, C);
			__m128 i2 = _mm_shuffle_ps(i0123, i0123, SHUF(2,2,2,2));
			STOREADD(2, A);
			STOREADD(2, B);
			STOREADD(2, C);
			__m128 i3 = _mm_shuffle_ps(i0123, i0123, SHUF(3,3,3,3));
			STOREADD(3, A);
			STOREADD(3, B);
			STOREADD(3, C);
			__m128 i4 = i4567;
			STOREADD(4, A);
			STOREADD(4, B);
			STOREADD(4, C);
			__m128 i5 = _mm_shuffle_ps(i4567, i4567, SHUF(1,1,1,1));
			STOREADD(5, A);
			STOREADD(5, B);
			STOREADD(5, C);
			__m128 i6 = _mm_shuffle_ps(i4567, i4567, SHUF(2,2,2,2));
			STOREADD(6, A);
			STOREADD(6, B);
			STOREADD(6, C);
			__m128 i7 = _mm_shuffle_ps(i4567, i4567, SHUF(3,3,3,3));
			STOREADD(7, A);
			STOREADD(7, B);
			STOREADD(7, C);
			#undef STOREADD
		}
	}
	_mm256_zeroupper();

	//process tail with scalar code
	for (; i < n; i++) {
		idDrawVert &A = verts[indexes[3 * i + 0]];
		idDrawVert &B = verts[indexes[3 * i + 1]];
		idDrawVert &C = verts[indexes[3 * i + 2]];
		idVec3 ABxyz = B.xyz - A.xyz;
		idVec3 ACxyz = C.xyz - A.xyz;
		idVec2 ABst = B.st - A.st;
		idVec2 ACst = C.st - A.st;

		idVec3 normal = ACxyz.Cross(ABxyz);
		normal.NormalizeFast();
		planes[i].SetNormal(normal);
		planes[i].FitThroughPoint(A.xyz);

		// area sign bit
		float area = ABst.Cross(ACst);
		float sign = (area < 0.0 ? -1.0 : 1.0);
		// first tangent
		idVec3 tangU = ABxyz * ACst.y - ACxyz * ABst.y;
		tangU *= sign * idMath::RSqrt( tangU.LengthSqr() );
		// second tangent
		idVec3 tangV = ACxyz * ABst.x - ABxyz * ACst.x;
		tangV *= sign * idMath::RSqrt( tangV.LengthSqr() );

		A.normal += normal;
		A.tangents[0] += tangU;
		A.tangents[1] += tangV;
		B.normal += normal;
		B.tangents[0] += tangU;
		B.tangents[1] += tangV;
		C.normal += normal;
		C.tangents[0] += tangU;
		C.tangents[1] += tangV;
	}
}

void idSIMD_AVX2::NormalizeTangents( idDrawVert *verts, const int numVerts ) {
	//in all vector normalizations, W component is can be zero (division by zero)
	//we have to mask any exceptions here
	idIgnoreFpExceptions guardFpExceptions;

	size_t i, n = numVerts;
	for (i = 0; i + 8 <= n; i += 8) {
		#define LOAD(k) \
			idDrawVert &vertex##k = verts[i + k]; \
			__m256 pack##k = _mm256_loadu_ps(&vertex##k.normal.x); \
			__m128 last##k = _mm_load_ss(&vertex##k.tangents[1].z)
		LOAD(0);
		LOAD(1);
		LOAD(2);
		LOAD(3);
		LOAD(4);
		LOAD(5);
		LOAD(6);
		LOAD(7);
		#undef LOAD

		//convert from AoS to SoA representation
		__m256 normal_x, normal_y, normal_z;
		__m256 tangU_x, tangU_y, tangU_z;
		__m256 tangV_x, tangV_y, tangV_z;
		{
			__m256 abcd04 = _mm256_permute2f128_ps(pack0, pack4, SHUF(0,0,2,0));
			__m256 abcd15 = _mm256_permute2f128_ps(pack1, pack5, SHUF(0,0,2,0));
			__m256 abcd26 = _mm256_permute2f128_ps(pack2, pack6, SHUF(0,0,2,0));
			__m256 abcd37 = _mm256_permute2f128_ps(pack3, pack7, SHUF(0,0,2,0));
			__m256 efgh04 = _mm256_permute2f128_ps(pack0, pack4, SHUF(1,0,3,0));
			__m256 efgh15 = _mm256_permute2f128_ps(pack1, pack5, SHUF(1,0,3,0));
			__m256 efgh26 = _mm256_permute2f128_ps(pack2, pack6, SHUF(1,0,3,0));
			__m256 efgh37 = _mm256_permute2f128_ps(pack3, pack7, SHUF(1,0,3,0));
			__m256 ab0145 = _mm256_unpacklo_ps(abcd04, abcd15);
			__m256 cd0145 = _mm256_unpackhi_ps(abcd04, abcd15);
			__m256 ab2367 = _mm256_unpacklo_ps(abcd26, abcd37);
			__m256 cd2367 = _mm256_unpackhi_ps(abcd26, abcd37);
			__m256 ef0145 = _mm256_unpacklo_ps(efgh04, efgh15);
			__m256 gh0145 = _mm256_unpackhi_ps(efgh04, efgh15);
			__m256 ef2367 = _mm256_unpacklo_ps(efgh26, efgh37);
			__m256 gh2367 = _mm256_unpackhi_ps(efgh26, efgh37);
			normal_x = _mm256_shuffle_ps(ab0145, ab2367, SHUF(0,1,0,1));
			normal_y = _mm256_shuffle_ps(ab0145, ab2367, SHUF(2,3,2,3));
			normal_z = _mm256_shuffle_ps(cd0145, cd2367, SHUF(0,1,0,1));
			tangU_x  = _mm256_shuffle_ps(cd0145, cd2367, SHUF(2,3,2,3));
			tangU_y  = _mm256_shuffle_ps(ef0145, ef2367, SHUF(0,1,0,1));
			tangU_z  = _mm256_shuffle_ps(ef0145, ef2367, SHUF(2,3,2,3));
			tangV_x  = _mm256_shuffle_ps(gh0145, gh2367, SHUF(0,1,0,1));
			tangV_y  = _mm256_shuffle_ps(gh0145, gh2367, SHUF(2,3,2,3));
			__m128 last0123 = _mm_movelh_ps(_mm_unpacklo_ps(last0, last1), _mm_unpacklo_ps(last2, last3));
			__m128 last4567 = _mm_movelh_ps(_mm_unpacklo_ps(last4, last5), _mm_unpacklo_ps(last6, last7));
			tangV_z = _mm256_insertf128_ps(_mm256_castps128_ps256(last0123), last4567, 1);
		}

		__m256 dotNormal, dotTangU, dotTangV;
		DOT3_P8(dotNormal, normal, normal);
		DOT3_P8(dotTangU, tangU, normal);
		DOT3_P8(dotTangV, tangV, normal);
		dotNormal = _mm256_rsqrt_ps(dotNormal);
		MUL3S_P8(normal, normal, dotNormal);
		dotNormal = _mm256_xor_ps(dotNormal, _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000)));
		dotTangU = _mm256_mul_ps(dotTangU, dotNormal);
		dotTangV = _mm256_mul_ps(dotTangV, dotNormal);
		tangU_x = _mm256_fmadd_ps(normal_x, dotTangU, tangU_x);
		tangU_y = _mm256_fmadd_ps(normal_y, dotTangU, tangU_y);
		tangU_z = _mm256_fmadd_ps(normal_z, dotTangU, tangU_z);
		tangV_x = _mm256_fmadd_ps(normal_x, dotTangV, tangV_x);
		tangV_y = _mm256_fmadd_ps(normal_y, dotTangV, tangV_y);
		tangV_z = _mm256_fmadd_ps(normal_z, dotTangV, tangV_z);

		__m256 sqlenTangU;
		DOT3_P8(sqlenTangU, tangU, tangU);
		sqlenTangU = _mm256_rsqrt_ps(sqlenTangU);
		MUL3S_P8(tangU, tangU, sqlenTangU);
		__m256 sqlenTangV;
		DOT3_P8(sqlenTangV, tangV, tangV);
		sqlenTangV = _mm256_rsqrt_ps(sqlenTangV);
		MUL3S_P8(tangV, tangV, sqlenTangV);

		//convert back to AoS layout
		{
			__m256 ab0145 = _mm256_unpacklo_ps(normal_x, normal_y);
			__m256 ab2367 = _mm256_unpackhi_ps(normal_x, normal_y);
			__m256 cd0145 = _mm256_unpacklo_ps(normal_z, tangU_x);
			__m256 cd2367 = _mm256_unpackhi_ps(normal_z, tangU_x);
			__m256 ef0145 = _mm256_unpacklo_ps(tangU_y, tangU_z);
			__m256 ef2367 = _mm256_unpackhi_ps(tangU_y, tangU_z);
			__m256 gh0145 = _mm256_unpacklo_ps(tangV_x, tangV_y);
			__m256 gh2367 = _mm256_unpackhi_ps(tangV_x, tangV_y);
			__m256 abcd04 = _mm256_shuffle_ps(ab0145, cd0145, SHUF(0,1,0,1));
			__m256 abcd15 = _mm256_shuffle_ps(ab0145, cd0145, SHUF(2,3,2,3));
			__m256 efgh04 = _mm256_shuffle_ps(ef0145, gh0145, SHUF(0,1,0,1));
			__m256 efgh15 = _mm256_shuffle_ps(ef0145, gh0145, SHUF(2,3,2,3));
			__m256 abcd26 = _mm256_shuffle_ps(ab2367, cd2367, SHUF(0,1,0,1));
			__m256 abcd37 = _mm256_shuffle_ps(ab2367, cd2367, SHUF(2,3,2,3));
			__m256 efgh26 = _mm256_shuffle_ps(ef2367, gh2367, SHUF(0,1,0,1));
			__m256 efgh37 = _mm256_shuffle_ps(ef2367, gh2367, SHUF(2,3,2,3));
			__m256 all0 = _mm256_permute2f128_ps(abcd04, efgh04, SHUF(0,0,2,0));
			__m256 all1 = _mm256_permute2f128_ps(abcd15, efgh15, SHUF(0,0,2,0));
			__m256 all2 = _mm256_permute2f128_ps(abcd26, efgh26, SHUF(0,0,2,0));
			__m256 all3 = _mm256_permute2f128_ps(abcd37, efgh37, SHUF(0,0,2,0));
			__m256 all4 = _mm256_permute2f128_ps(abcd04, efgh04, SHUF(1,0,3,0));
			__m256 all5 = _mm256_permute2f128_ps(abcd15, efgh15, SHUF(1,0,3,0));
			__m256 all6 = _mm256_permute2f128_ps(abcd26, efgh26, SHUF(1,0,3,0));
			__m256 all7 = _mm256_permute2f128_ps(abcd37, efgh37, SHUF(1,0,3,0));
			__m128 i0123 = _mm256_castps256_ps128(tangV_z);
			__m128 i4567 = _mm256_extractf128_ps(tangV_z, 1);
			#define STORE(k) {\
				float *ptr = &vertex##k.normal.x; \
				_mm256_storeu_ps(ptr, all##k); \
				_mm_store_ss(ptr + 8, i##k); \
			}
			__m128 i0 = i0123;
			STORE(0);
			__m128 i1 = _mm_shuffle_ps(i0123, i0123, SHUF(1,1,1,1));
			STORE(1);
			__m128 i2 = _mm_shuffle_ps(i0123, i0123, SHUF(2,2,2,2));
			STORE(2);
			__m128 i3 = _mm_shuffle_ps(i0123, i0123, SHUF(3,3,3,3));
			STORE(3);
			__m128 i4 = i4567;
			STORE(4);
			__m128 i5 = _mm_shuffle_ps(i4567, i4567, SHUF(1,1,1,1));
			STORE(5);
			__m128 i6 = _mm_shuffle_ps(i4567, i4567, SHUF(2,2,2,2));
			STORE(6);
			__m128 i7 = _mm_shuffle_ps(i4567, i4567, SHUF(3,3,3,3));
			STORE(7);
			#undef STORE
		}
	}
	_mm256_zeroupper();

	for (; i < n; i++) {
		idVec3 &normal = verts[i].normal;
		normal.NormalizeFast();
		for ( int j = 0; j < 2; j++ ) {
			idVec3 &tang = verts[i].tangents[j];
			tang -= ( tang * normal ) * normal;
			tang.NormalizeFast();
		}
	}
}

#endif
