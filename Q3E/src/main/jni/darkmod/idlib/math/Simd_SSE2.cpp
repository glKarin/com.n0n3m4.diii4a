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

#include "Simd_SSE2.h"


idSIMD_SSE2::idSIMD_SSE2() {
	name = "SSE2";
}

#ifdef ENABLE_SSE_PROCESSORS

#include <emmintrin.h>

//apply optimizations to this file in Debug with Inlines configuration
DEBUG_OPTIMIZE_ON

#if defined(_MSVC) && defined(_DEBUG) && !defined(EXPLICIT_OPTIMIZATION)
	//assert only checked in "Debug" build of MSVC
	#define DBG_ASSERT(cond) assert(cond)
#else
	#define DBG_ASSERT(cond) ((void)0)
#endif

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


//suitable for any compiler, OS and bitness  (intrinsics)
//generally used on Windows 64-bit and all Linuxes

//somewhat slower than ID's code (28.6K vs 21.4K)
void idSIMD_SSE2::NormalizeTangents( idDrawVert *verts, const int numVerts ) {
	//in all vector normalizations, W component is can be zero (division by zero)
	//we have to mask any exceptions here
	idIgnoreFpExceptions guardFpExceptions;

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

		static_assert(OFFSETOF(idDrawVert, normal) + sizeof(vertex.normal) == OFFSETOF(idDrawVert, tangents), "Bad members offsets");
		static_assert(sizeof(vertex.tangents) == 24, "Bad members offsets");
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


template<class Lambda> static ID_INLINE void VertexMinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int count, Lambda Index ) {
	//idMD5Mesh::CalcBounds calls this with uninitialized texcoords
	//we have to mask any exceptions here
	idIgnoreFpExceptions guardFpExceptions;

	__m128 rmin = _mm_set1_ps( 1e30f);
	__m128 rmax = _mm_set1_ps(-1e30f);
	int i = 0;
	for (; i < (count & (~3)); i += 4) {
		__m128 pos0 = _mm_loadu_ps(&src[Index(i + 0)].xyz.x);
		__m128 pos1 = _mm_loadu_ps(&src[Index(i + 1)].xyz.x);
		__m128 pos2 = _mm_loadu_ps(&src[Index(i + 2)].xyz.x);
		__m128 pos3 = _mm_loadu_ps(&src[Index(i + 3)].xyz.x);
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
		__m128 pos = _mm_loadu_ps(&src[Index(i + 0)].xyz.x);
		rmin = _mm_min_ps(rmin, pos);
		rmax = _mm_max_ps(rmax, pos);
		if (i + 1 < count) {
			pos = _mm_loadu_ps(&src[Index(i + 1)].xyz.x);
			rmin = _mm_min_ps(rmin, pos);
			rmax = _mm_max_ps(rmax, pos);
			if (i + 2 < count) {
				pos = _mm_loadu_ps(&src[Index(i + 2)].xyz.x);
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
void idSIMD_SSE2::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int count ) {
	VertexMinMax(min, max, src, count, [](int i) { return i; });
}
void idSIMD_SSE2::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int *indexes, const int count ) {
	VertexMinMax(min, max, src, count, [indexes](int i) { return indexes[i]; });
}

//this thing is significantly faster that ID's original code (50K vs 87K)
//one major difference is: this version zeroes all resulting vectors in preprocessing step
//this allows to write completely branchless code
//ID' original version has many flaws, namely:
//  1. branches for singular cases
//  2. storing data is done with scalar C++ code
//  3. branches for detecting: store or add
void idSIMD_SSE2::DeriveTangents( idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) {
	int numTris = numIndexes / 3;
#define NORMAL_EPS 1e-10f

	//note that idDrawVerts must have normal & tangents tightly packed, going on after the other
	static_assert(OFFSETOF(idDrawVert, normal) + sizeof(verts->normal) == OFFSETOF(idDrawVert, tangents), "Bad members offsets");
	static_assert(sizeof(verts->tangents) == 24, "Bad members offsets");
	for (int i = 0; i < numVerts; i++) {
		float *ptr = &verts[i].normal.x;
		_mm_storeu_ps(ptr, _mm_setzero_ps());
		_mm_storeu_ps(ptr + 4, _mm_setzero_ps());
		_mm_store_ss(ptr + 8, _mm_setzero_ps());
	}

	for (int i = 0; i < numTris; i++) {
		int idxA = indexes[3 * i + 0];
		int idxB = indexes[3 * i + 1];
		int idxC = indexes[3 * i + 2];
		idDrawVert &vA = verts[idxA];
		idDrawVert &vB = verts[idxB];
		idDrawVert &vC = verts[idxC];

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
		planeShift = _mm_xor_ps(planeShift, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));
		//save plane equation
		static_assert(sizeof(idPlane) == 16, "Wrong idPlane size");
		_mm_storeu_ps(planes[i].ToFloatPtr(), normal);
		_mm_store_ss(planes[i].ToFloatPtr() + 3, planeShift);

		//check area sign
		__m128 area = _mm_sub_ps(_mm_mul_ps(dpAB, dtAC), _mm_mul_ps(dpAC, dtAB));
		area = _mm_shuffle_ps(area, area, SHUF(3, 3, 3, 3));
		__m128 sign = _mm_and_ps(area, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));

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
	__m128 xyzMask = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));
	__m128 oneW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0);
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

	__m128 radP = _mm_set1_ps( radius);
	__m128 radM = _mm_set1_ps(-radius);

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

void idSIMD_SSE2::CalcTriFacing( const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes, const idVec3 &lightOrigin, byte *facing ) {
	int numTris = numIndexes / 3;
	__m128 orig = _mm_setr_ps(lightOrigin.x, lightOrigin.y, lightOrigin.z, 0.0f);

	for (int i = 0; i < numTris; i++) {
		int idxA = indexes[3 * i + 0];
		int idxB = indexes[3 * i + 1];
		int idxC = indexes[3 * i + 2];
		const idDrawVert &vA = verts[idxA];
		const idDrawVert &vB = verts[idxB];
		const idDrawVert &vC = verts[idxC];

		__m128 posA = _mm_loadu_ps( &vA.xyz.x );		//xyzs A
		__m128 posB = _mm_loadu_ps( &vB.xyz.x );		//xyzs B
		__m128 posC = _mm_loadu_ps( &vC.xyz.x );		//xyzs C
		//compute AB/AC differences
		__m128 dpAB = _mm_sub_ps( posB, posA );			//xyzs AB
		__m128 dpAC = _mm_sub_ps( posC, posA );			//xyzs AC
		//compute normal vector (length can be arbitrary)
		CROSS_PRODUCT( normal, dpAC, dpAB );

		//get orientation
		__m128 vertToOrig = _mm_sub_ps(orig, posA);
		DOT_PRODUCT( signedVolume, vertToOrig, normal );
		__m128 oriPositive = _mm_cmple_ss(_mm_setzero_ps(), signedVolume);
		int num = _mm_cvtsi128_si32(_mm_castps_si128(oriPositive));

		facing[i] = -num;		//-1 when true, 0 when false
	}
}

void CopyBufferSSE2_NT( byte* dst, const byte* src, size_t numBytes ) {
	size_t i = 0;
	for ( ; i < numBytes && (size_t(src + i) & 15); i++ ) {
		dst[i] = src[i];
	}
	for ( ; i + 128 <= numBytes; i += 128 ) {
		__m128i d0 = _mm_load_si128( ( __m128i* ) & src[i + 0 * 16] );
		__m128i d1 = _mm_load_si128( ( __m128i* ) & src[i + 1 * 16] );
		__m128i d2 = _mm_load_si128( ( __m128i* ) & src[i + 2 * 16] );
		__m128i d3 = _mm_load_si128( ( __m128i* ) & src[i + 3 * 16] );
		__m128i d4 = _mm_load_si128( ( __m128i* ) & src[i + 4 * 16] );
		__m128i d5 = _mm_load_si128( ( __m128i* ) & src[i + 5 * 16] );
		__m128i d6 = _mm_load_si128( ( __m128i* ) & src[i + 6 * 16] );
		__m128i d7 = _mm_load_si128( ( __m128i* ) & src[i + 7 * 16] );
		_mm_stream_si128( ( __m128i* ) & dst[i + 0 * 16], d0 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 1 * 16], d1 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 2 * 16], d2 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 3 * 16], d3 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 4 * 16], d4 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 5 * 16], d5 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 6 * 16], d6 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 7 * 16], d7 );
	}
	for ( ; i + 16 <= numBytes; i += 16 ) {
		__m128i d = _mm_load_si128( ( __m128i* ) & src[i] );
		_mm_stream_si128( ( __m128i* ) & dst[i], d );
	}
	for ( ; i < numBytes; i++ ) {
		dst[i] = src[i];
	}
	_mm_sfence();
	assert(i == numBytes);
}

void idSIMD_SSE2::MemcpyNT( void* dst, const void* src, const int count ) {
	assert(count >= 0);
	if ( ( (size_t)src ^ (size_t)dst ) & 15 ) // FIXME allow SSE2 on differently aligned addresses
		memcpy( dst, src, count );
	else
		CopyBufferSSE2_NT( (byte*)dst, (byte*)src, count );
}

/*
============
idSIMD_SSE2::GenerateMipMap2x2
============
*/
void idSIMD_SSE2::GenerateMipMap2x2( const byte *srcPtr, int srcStride, int halfWidth, int halfHeight, byte *dstPtr, int dstStride ) {
	for (int i = 0; i < halfHeight; i++) {
		const byte *inRow0 = &srcPtr[(2*i+0) * srcStride];
		const byte *inRow1 = &srcPtr[(2*i+1) * srcStride];
		byte *outRow = &dstPtr[i * dstStride];

		int j;
		for (j = 0; j + 4 <= halfWidth; j += 4) {
			__m128i A0 = _mm_loadu_si128((__m128i*)(inRow0 + 8*j + 0));
			__m128i A1 = _mm_loadu_si128((__m128i*)(inRow0 + 8*j + 16));
			__m128i B0 = _mm_loadu_si128((__m128i*)(inRow1 + 8*j + 0));
			__m128i B1 = _mm_loadu_si128((__m128i*)(inRow1 + 8*j + 16));

			__m128i A0shuf = _mm_shuffle_epi32(A0, SHUF(0, 2, 1, 3));
			__m128i A1shuf = _mm_shuffle_epi32(A1, SHUF(0, 2, 1, 3));
			__m128i B0shuf = _mm_shuffle_epi32(B0, SHUF(0, 2, 1, 3));
			__m128i B1shuf = _mm_shuffle_epi32(B1, SHUF(0, 2, 1, 3));
			__m128i A0l = _mm_unpacklo_epi8(A0shuf, _mm_setzero_si128());
			__m128i A0r = _mm_unpackhi_epi8(A0shuf, _mm_setzero_si128());
			__m128i A1l = _mm_unpacklo_epi8(A1shuf, _mm_setzero_si128());
			__m128i A1r = _mm_unpackhi_epi8(A1shuf, _mm_setzero_si128());
			__m128i B0l = _mm_unpacklo_epi8(B0shuf, _mm_setzero_si128());
			__m128i B0r = _mm_unpackhi_epi8(B0shuf, _mm_setzero_si128());
			__m128i B1l = _mm_unpacklo_epi8(B1shuf, _mm_setzero_si128());
			__m128i B1r = _mm_unpackhi_epi8(B1shuf, _mm_setzero_si128());

			__m128i sum0 = _mm_add_epi16(_mm_add_epi16(A0l, A0r), _mm_add_epi16(B0l, B0r));
			__m128i sum1 = _mm_add_epi16(_mm_add_epi16(A1l, A1r), _mm_add_epi16(B1l, B1r));
			__m128i avg0 = _mm_srli_epi16(_mm_add_epi16(sum0, _mm_set1_epi16(2)), 2);
			__m128i avg1 = _mm_srli_epi16(_mm_add_epi16(sum1, _mm_set1_epi16(2)), 2);

			__m128i res = _mm_packus_epi16(avg0, avg1);
			_mm_storeu_si128((__m128i*)(outRow + 4*j), res);
		}

		for (; j < halfWidth; j++) {
			unsigned sum0 = (unsigned)inRow0[8*j+0] + inRow0[8*j+4+0] + inRow1[8*j+0] + inRow1[8*j+4+0];
			unsigned sum1 = (unsigned)inRow0[8*j+1] + inRow0[8*j+4+1] + inRow1[8*j+1] + inRow1[8*j+4+1];
			unsigned sum2 = (unsigned)inRow0[8*j+2] + inRow0[8*j+4+2] + inRow1[8*j+2] + inRow1[8*j+4+2];
			unsigned sum3 = (unsigned)inRow0[8*j+3] + inRow0[8*j+4+3] + inRow1[8*j+3] + inRow1[8*j+4+3];
			outRow[4*j+0] = (sum0 + 2) >> 2;
			outRow[4*j+1] = (sum1 + 2) >> 2;
			outRow[4*j+2] = (sum2 + 2) >> 2;
			outRow[4*j+3] = (sum3 + 2) >> 2;
		}
	}
}

namespace DxtCompress {

static void LoadBoundaryBlocks( const byte *srcPtr, int stride, int width, int height, int baseR, int baseC, int numBlocks, byte *expandedBlocks ) {
	// Copy blocks with clamp-style padding
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4 * numBlocks; c++) {
			int a = baseR + r;
			int b = baseC + c;
			if (a > height - 1)
				a = height - 1;
			if (b > width - 1)
				b = width - 1;
			((dword*)expandedBlocks)[r * (4 * numBlocks) + c] = *(dword*)&srcPtr[a * stride + 4 * b];
		}
	}
}

static int StoreBoundaryOutput( byte *dstPtr, int width, int height, int baseR, int baseC, int numBlocks, int numWordsPerBlock, const byte *expandedOutput) {
	// Copy starting subsegment of blocks to output (not always all of them)
	int countBlocks = idMath::Imin((width - baseC + 3) >> 2, numBlocks);
	int countWords = numWordsPerBlock * countBlocks;
	memcpy(dstPtr, expandedOutput, 8 * countWords);
	return 8 * countWords;
}

template<int NumBlocks> ID_FORCE_INLINE static void LoadBlocks( const byte *srcPtr, int stride, __m128i rowsRgba[NumBlocks][4] ) {
	for (int r = 0; r < 4; r++) {
		for (int b = 0; b < NumBlocks; b++)
			rowsRgba[b][r] = _mm_loadu_si128( (__m128i*)(srcPtr + b * 16) );
		srcPtr += stride;
	}
}

template<int NumRegs> ID_FORCE_INLINE static void StoreOutput( byte *dstPtr, const __m128i output[NumRegs] ) {
	for (int i = 0; i < NumRegs; i++)
		_mm_storeu_si128( (__m128i*)(dstPtr + i * 16), output[i] );
}

ID_FORCE_INLINE static void ExtractRedGreen_x2( const __m128i inputRowsRgba[2][4], __m128i outputRowsRgrg[4] ) {
	// inputRowsRgba[b][r] is r-th row of b-th block
	// each register is full 8-bit RGBA data, as read from image memory
	#define REPACKROW(r) { \
		__m128i rg0 = _mm_and_si128(inputRowsRgba[0][r], _mm_set1_epi32(0xFFFF)); \
		__m128i rg1 = _mm_slli_epi32(inputRowsRgba[1][r], 16); \
		outputRowsRgrg[r] = _mm_xor_si128(rg1, rg0); \
	}
	REPACKROW(0)
	REPACKROW(1)
	REPACKROW(2)
	REPACKROW(3)
	#undef REPACKROW
	// outputRowsRgrg[r] contains red and green channels of r-th row in both blocks
	// each "pixel" (4-byte world) of it has layout: red0, green0, red1, green1
}

ID_FORCE_INLINE static void ExtractAlpha_x4( const __m128i inputRowsRgba[4][4], __m128i outputRowsAaaa[4] ) {
	// same input data as in ExtractRedGreen_x2 but for 4 blocks
	#define REPACKROW(r) { \
		__m128i alpha0 = _mm_srli_epi32(inputRowsRgba[0][r], 24); \
		__m128i alpha1 = _mm_srli_epi32(_mm_and_si128(inputRowsRgba[1][r], _mm_set1_epi32(0xFF000000)), 16); \
		__m128i alpha2 = _mm_srli_epi32(_mm_and_si128(inputRowsRgba[2][r], _mm_set1_epi32(0xFF000000)), 8); \
		__m128i alpha3 = _mm_and_si128(inputRowsRgba[3][r], _mm_set1_epi32(0xFF000000)); \
		outputRowsAaaa[r] = _mm_xor_si128(_mm_xor_si128(alpha0, alpha1), _mm_xor_si128(alpha2, alpha3)); \
	}
	REPACKROW(0)
	REPACKROW(1)
	REPACKROW(2)
	REPACKROW(3)
	#undef REPACKROW
	// outputRowsAaaa[r] contains alpha channels of r-th row in all blocks
	// each "pixel" (4-byte world) of it has layout: alpha0, alpha1, alpha2, alpha3
}

ID_FORCE_INLINE static void ProcessAlphaBlock_x4( const __m128i inputRows[4], __m128i outputs[2] ) {
	// 4 input blocks are provided in SoA fashion:
	// inputRows[r] contains r-th row of all blocks
	// (4 * c + d)-th byte of it is pixel in c-th column of d-th block

	// Note: originally the function was written for RGTC compression (of two blocks)
	__m128i rgrg0 = inputRows[0];
	__m128i rgrg1 = inputRows[1];
	__m128i rgrg2 = inputRows[2];
	__m128i rgrg3 = inputRows[3];

	// Compute min/max values
	__m128i minBytes = _mm_min_epu8(_mm_min_epu8(rgrg0, rgrg1), _mm_min_epu8(rgrg2, rgrg3));
	__m128i maxBytes = _mm_max_epu8(_mm_max_epu8(rgrg0, rgrg1), _mm_max_epu8(rgrg2, rgrg3));
	minBytes = _mm_min_epu8(minBytes, _mm_shuffle_epi32(minBytes, SHUF(2, 3, 0, 1)));
	maxBytes = _mm_max_epu8(maxBytes, _mm_shuffle_epi32(maxBytes, SHUF(2, 3, 0, 1)));
	minBytes = _mm_min_epu8(minBytes, _mm_shuffle_epi32(minBytes, SHUF(1, 0, 1, 0)));
	maxBytes = _mm_max_epu8(maxBytes, _mm_shuffle_epi32(maxBytes, SHUF(1, 0, 1, 0)));
	// (each 32-bit element contains min/max in RGRG format)
	for (int i = 0; i < 4; i++)
		DBG_ASSERT(maxBytes.m128i_u8[i] >= minBytes.m128i_u8[i]);

	// Make sure min != max
	__m128i deltaBytes = _mm_sub_epi8(maxBytes, minBytes);
	__m128i maskDeltaZero = _mm_cmpeq_epi8(deltaBytes, _mm_setzero_si128());
	maxBytes = _mm_sub_epi8(maxBytes, maskDeltaZero);
	deltaBytes = _mm_sub_epi8(deltaBytes, maskDeltaZero);
	__m128i maskMaxOverflown = _mm_and_si128(maskDeltaZero, _mm_cmpeq_epi8(maxBytes, _mm_setzero_si128()));
	minBytes = _mm_add_epi8(minBytes, maskMaxOverflown);
	maxBytes = _mm_add_epi8(maxBytes, maskMaxOverflown);
	for (int i = 0; i < 4; i++) {
		DBG_ASSERT(maxBytes.m128i_u8[i] > minBytes.m128i_u8[i]);
		DBG_ASSERT(maxBytes.m128i_u8[i] - minBytes.m128i_u8[i] == deltaBytes.m128i_u8[i]);
	}

	// Prepare multiplier
	__m128i deltaDwords = _mm_unpacklo_epi8(deltaBytes, _mm_setzero_si128());
	deltaDwords = _mm_unpacklo_epi16(deltaDwords, _mm_setzero_si128());
	__m128 deltaFloat = _mm_cvtepi32_ps(deltaDwords);
	__m128 multFloat = _mm_div_ps(_mm_set1_ps(7 << 12), deltaFloat);
	__m128i multWords = _mm_cvttps_epi32(_mm_add_ps(multFloat, _mm_set1_ps(0.999f)));
	multWords = _mm_packs_epi32(multWords, multWords);

	__m128i chunksRow0, chunksRow1, chunksRow2, chunksRow3;
	#define PROCESS_ROW(r) { \
		/* Compute ratio, find closest ramp point */ \
		__m128i numerBytes = _mm_sub_epi8(rgrg##r, minBytes); \
		__m128i numerWords0 = _mm_unpacklo_epi8(numerBytes, _mm_setzero_si128()); \
		__m128i numerWords1 = _mm_unpackhi_epi8(numerBytes, _mm_setzero_si128()); \
		__m128i fixedQuot0 = _mm_mullo_epi16(numerWords0, multWords); \
		__m128i fixedQuot1 = _mm_mullo_epi16(numerWords1, multWords); \
		__m128i idsWords0 = _mm_srli_epi16(_mm_add_epi16(fixedQuot0, _mm_set1_epi16(1 << 11)), 12); \
		__m128i idsWords1 = _mm_srli_epi16(_mm_add_epi16(fixedQuot1, _mm_set1_epi16(1 << 11)), 12); \
		for (int i = 0; i < 8; i++) \
			DBG_ASSERT(idsWords0.m128i_u16[i] <= 7 && idsWords1.m128i_u16[i] <= 7); \
		__m128i idsBytes = _mm_packs_epi16(idsWords0, idsWords1); \
		/* Convert ramp point index to DXT index */ \
		__m128i dxtIdsBytes = _mm_sub_epi8(_mm_set1_epi8(8), idsBytes); \
		dxtIdsBytes = _mm_add_epi8(dxtIdsBytes, _mm_cmpeq_epi8(idsBytes, _mm_set1_epi8(7))); \
		dxtIdsBytes = _mm_sub_epi8(dxtIdsBytes, _mm_and_si128(_mm_cmpeq_epi8(idsBytes, _mm_setzero_si128()), _mm_set1_epi8(7))); \
		for (int i = 0; i < 8; i++) \
			DBG_ASSERT(dxtIdsBytes.m128i_u8[i] <= 7); \
		/* Compress 3-bit indices into row 12-bit chunks */ \
		__m128i temp = dxtIdsBytes; \
		temp = _mm_xor_si128(temp, _mm_srli_epi64(temp, 32 - 3)); \
		__m128i tempLo = _mm_unpacklo_epi8(temp, _mm_setzero_si128()); \
		__m128i tempHi = _mm_unpackhi_epi8(temp, _mm_setzero_si128()); \
		temp = _mm_xor_si128(tempLo, _mm_slli_epi16(tempHi, 6)); \
		temp = _mm_unpacklo_epi16(temp, _mm_setzero_si128()); \
		temp = _mm_and_si128(temp, _mm_set1_epi32((1 << 12) - 1)); \
		for (int i = 0; i < 8; i++) \
			DBG_ASSERT(temp.m128i_u32[i] < (1 << 12)); \
		chunksRow##r = temp; \
	}
	PROCESS_ROW(0)
	PROCESS_ROW(1)
	PROCESS_ROW(2)
	PROCESS_ROW(3)
	#undef PROCESS_ROW

	// Compress 12-bit row chunks into (16+32)-bit block chunks
	__m128i blockLow32 = _mm_xor_si128(_mm_slli_epi32(chunksRow0, 16), _mm_slli_epi32(chunksRow1, 28));
	__m128i blockHigh32 = _mm_xor_si128(_mm_slli_epi32(chunksRow2, 8), _mm_slli_epi32(chunksRow3, 20));
	blockHigh32 = _mm_xor_si128(blockHigh32, _mm_srli_epi32(chunksRow1, 4));
	for (int i = 0; i < 4; i++)
		DBG_ASSERT(blockLow32.m128i_u16[2*i] == 0);
	// Add max/min bytes to first 16 bits
	__m128i maxMinDwords = _mm_unpacklo_epi8(maxBytes, minBytes);
	maxMinDwords = _mm_unpacklo_epi16(maxMinDwords, _mm_setzero_si128());
	blockLow32 = _mm_xor_si128(blockLow32, maxMinDwords);

	// Output two blocks
	outputs[0] = _mm_unpacklo_epi32(blockLow32, blockHigh32);
	outputs[1] = _mm_unpackhi_epi32(blockLow32, blockHigh32);
}

ID_FORCE_INLINE static void ProcessAlphaBlock4b_x2( const __m128i inputRows[2][4], __m128i outputs[1] ) {
	__m128i bytes[2];
	for (int b = 0; b < 2; b++) {
		__m128i row0 = _mm_srli_epi32(inputRows[b][0], 24);
		__m128i row1 = _mm_srli_epi32(inputRows[b][1], 24);
		__m128i row2 = _mm_srli_epi32(inputRows[b][2], 24);
		__m128i row3 = _mm_srli_epi32(inputRows[b][3], 24);
		__m128i rows01 = _mm_packs_epi32(row0, row1);
		__m128i rows23 = _mm_packs_epi32(row2, row3);

		// convert to 4-bit: round(X * 15 / 255)
		rows01 = _mm_add_epi16(_mm_mullo_epi16(rows01, _mm_set1_epi16(15)), _mm_set1_epi16(127));
		rows23 = _mm_add_epi16(_mm_mullo_epi16(rows23, _mm_set1_epi16(15)), _mm_set1_epi16(127));
		// note: X/255 = X/256 + X/256^2 + (X/256^3 + ...)
		rows01 = _mm_srli_epi16(_mm_add_epi16(rows01, _mm_srli_epi16(rows01, 8)), 8);
		rows23 = _mm_srli_epi16(_mm_add_epi16(rows23, _mm_srli_epi16(rows23, 8)), 8);

		bytes[b] = _mm_packus_epi16(rows01, rows23);
	}
	__m128i lohalf = _mm_packus_epi16(_mm_and_si128(bytes[0], _mm_set1_epi16(0x00FF)), _mm_and_si128(bytes[1], _mm_set1_epi16(0x00FF)));
	__m128i hihalf = _mm_packus_epi16(
		_mm_srli_epi16(_mm_and_si128(bytes[0], _mm_set1_epi16(0xFF00u)), 4),
		_mm_srli_epi16(_mm_and_si128(bytes[1], _mm_set1_epi16(0xFF00u)), 4)
	);
	outputs[0] = _mm_xor_si128(lohalf, hihalf);
}

ID_FORCE_INLINE static void ProcessColorBlock_x8( const __m128i inputRows[8][4], __m128i outputs[4] ) {
	// shuffle/transpose data to get a pack of 8 16-bit values of every scalar
	__m128i inputx2Row[8][4];
	for (int b = 0; b < 8; b += 2)
		for (int r = 0; r < 4; r++) {
			inputx2Row[b + 0][r] = _mm_unpacklo_epi8(inputRows[b + 0][r], inputRows[b + 1][r]);
			inputx2Row[b + 1][r] = _mm_unpackhi_epi8(inputRows[b + 0][r], inputRows[b + 1][r]);
		}
	__m128i inputx4Row[8][4];
	for (int b = 0; b < 8; b += 4)
		for (int r = 0; r < 4; r++) {
			inputx4Row[b + 0][r] = _mm_unpacklo_epi16(inputx2Row[b + 0][r], inputx2Row[b + 2][r]);
			inputx4Row[b + 1][r] = _mm_unpackhi_epi16(inputx2Row[b + 0][r], inputx2Row[b + 2][r]);
			inputx4Row[b + 2][r] = _mm_unpacklo_epi16(inputx2Row[b + 1][r], inputx2Row[b + 3][r]);
			inputx4Row[b + 3][r] = _mm_unpackhi_epi16(inputx2Row[b + 1][r], inputx2Row[b + 3][r]);
		}
	__m128i pixels[16][4];
	for (int r = 0; r < 4; r++)
		for (int c = 0; c < 4; c++) {
			__m128i rrgg = _mm_unpacklo_epi32(inputx4Row[c + 0][r], inputx4Row[c + 4][r]);
			pixels[4 * r + c][0] = _mm_unpacklo_epi8(rrgg, _mm_setzero_si128());
			pixels[4 * r + c][1] = _mm_unpackhi_epi8(rrgg, _mm_setzero_si128());
			__m128i bbaa = _mm_unpackhi_epi32(inputx4Row[c + 0][r], inputx4Row[c + 4][r]);
			pixels[4 * r + c][2] = _mm_unpacklo_epi8(bbaa, _mm_setzero_si128());
			pixels[4 * r + c][3] = _mm_unpackhi_epi8(bbaa, _mm_setzero_si128());
		}

	// compute sum and average color
	__m128i sumColor[3] = {_mm_setzero_si128(), _mm_setzero_si128(), _mm_setzero_si128()};
	for (int i = 0; i < 16; i++) {
		sumColor[0] = _mm_add_epi16(sumColor[0], pixels[i][0]);
		sumColor[1] = _mm_add_epi16(sumColor[1], pixels[i][1]);
		sumColor[2] = _mm_add_epi16(sumColor[2], pixels[i][2]);
	}
	__m128i avgColor[3] = {_mm_setzero_si128(), _mm_setzero_si128(), _mm_setzero_si128()};
	avgColor[0] = _mm_srli_epi16(_mm_add_epi16(sumColor[0], _mm_set1_epi16(7)), 4);
	avgColor[1] = _mm_srli_epi16(_mm_add_epi16(sumColor[1], _mm_set1_epi16(7)), 4);
	avgColor[2] = _mm_srli_epi16(_mm_add_epi16(sumColor[2], _mm_set1_epi16(7)), 4);

	// compute covariance matrix (float)
	__m128 covMatrRRlo = _mm_setzero_ps(), covMatrRRhi = _mm_setzero_ps();
	__m128 covMatrRGlo = _mm_setzero_ps(), covMatrRGhi = _mm_setzero_ps();
	__m128 covMatrRBlo = _mm_setzero_ps(), covMatrRBhi = _mm_setzero_ps();
	__m128 covMatrGGlo = _mm_setzero_ps(), covMatrGGhi = _mm_setzero_ps();
	__m128 covMatrGBlo = _mm_setzero_ps(), covMatrGBhi = _mm_setzero_ps();
	__m128 covMatrBBlo = _mm_setzero_ps(), covMatrBBhi = _mm_setzero_ps();
	for (int i = 0; i < 16; i++) {
		__m128i Rdiff = _mm_sub_epi16(pixels[i][0], avgColor[0]);
		__m128i Gdiff = _mm_sub_epi16(pixels[i][1], avgColor[1]);
		__m128i Bdiff = _mm_sub_epi16(pixels[i][2], avgColor[2]);
		__m128 Rlo = _mm_cvtepi32_ps(_mm_srai_epi32(_mm_unpacklo_epi16(Rdiff, Rdiff), 16));
		__m128 Rhi = _mm_cvtepi32_ps(_mm_srai_epi32(_mm_unpackhi_epi16(Rdiff, Rdiff), 16));
		__m128 Glo = _mm_cvtepi32_ps(_mm_srai_epi32(_mm_unpacklo_epi16(Gdiff, Gdiff), 16));
		__m128 Ghi = _mm_cvtepi32_ps(_mm_srai_epi32(_mm_unpackhi_epi16(Gdiff, Gdiff), 16));
		__m128 Blo = _mm_cvtepi32_ps(_mm_srai_epi32(_mm_unpacklo_epi16(Bdiff, Bdiff), 16));
		__m128 Bhi = _mm_cvtepi32_ps(_mm_srai_epi32(_mm_unpackhi_epi16(Bdiff, Bdiff), 16));
		covMatrRRlo = _mm_add_ps(covMatrRRlo, _mm_mul_ps(Rlo, Rlo));
		covMatrRGlo = _mm_add_ps(covMatrRGlo, _mm_mul_ps(Rlo, Glo));
		covMatrRBlo = _mm_add_ps(covMatrRBlo, _mm_mul_ps(Rlo, Blo));
		covMatrGGlo = _mm_add_ps(covMatrGGlo, _mm_mul_ps(Glo, Glo));
		covMatrGBlo = _mm_add_ps(covMatrGBlo, _mm_mul_ps(Glo, Blo));
		covMatrBBlo = _mm_add_ps(covMatrBBlo, _mm_mul_ps(Blo, Blo));
		covMatrRRhi = _mm_add_ps(covMatrRRhi, _mm_mul_ps(Rhi, Rhi));
		covMatrRGhi = _mm_add_ps(covMatrRGhi, _mm_mul_ps(Rhi, Ghi));
		covMatrRBhi = _mm_add_ps(covMatrRBhi, _mm_mul_ps(Rhi, Bhi));
		covMatrGGhi = _mm_add_ps(covMatrGGhi, _mm_mul_ps(Ghi, Ghi));
		covMatrGBhi = _mm_add_ps(covMatrGBhi, _mm_mul_ps(Ghi, Bhi));
		covMatrBBhi = _mm_add_ps(covMatrBBhi, _mm_mul_ps(Bhi, Bhi));
	}
	// find max eigenvector with power iteration
	__m128 veclo[3] = {_mm_set1_ps(1.0f), _mm_set1_ps(1.0f), _mm_set1_ps(1.0f)};
	__m128 vechi[3] = {_mm_set1_ps(1.0f), _mm_set1_ps(1.0f), _mm_set1_ps(1.0f)};
	for (int pwr = 0; pwr < 3; pwr++) {
		__m128 nveclo[3], nvechi[3];
		nveclo[0] = _mm_add_ps(_mm_mul_ps(covMatrRRlo, veclo[0]), _mm_add_ps(_mm_mul_ps(covMatrRGlo, veclo[1]), _mm_mul_ps(covMatrRBlo, veclo[2])));
		nveclo[1] = _mm_add_ps(_mm_mul_ps(covMatrRGlo, veclo[0]), _mm_add_ps(_mm_mul_ps(covMatrGGlo, veclo[1]), _mm_mul_ps(covMatrGBlo, veclo[2])));
		nveclo[2] = _mm_add_ps(_mm_mul_ps(covMatrRBlo, veclo[0]), _mm_add_ps(_mm_mul_ps(covMatrGBlo, veclo[1]), _mm_mul_ps(covMatrBBlo, veclo[2])));
		veclo[0] = nveclo[0];
		veclo[1] = nveclo[1];
		veclo[2] = nveclo[2];
		nvechi[0] = _mm_add_ps(_mm_mul_ps(covMatrRRhi, vechi[0]), _mm_add_ps(_mm_mul_ps(covMatrRGhi, vechi[1]), _mm_mul_ps(covMatrRBhi, vechi[2])));
		nvechi[1] = _mm_add_ps(_mm_mul_ps(covMatrRGhi, vechi[0]), _mm_add_ps(_mm_mul_ps(covMatrGGhi, vechi[1]), _mm_mul_ps(covMatrGBhi, vechi[2])));
		nvechi[2] = _mm_add_ps(_mm_mul_ps(covMatrRBhi, vechi[0]), _mm_add_ps(_mm_mul_ps(covMatrGBhi, vechi[1]), _mm_mul_ps(covMatrBBhi, vechi[2])));
		vechi[0] = nvechi[0];
		vechi[1] = nvechi[1];
		vechi[2] = nvechi[2];
	}
	// compute length of found axis
	__m128 normlo = _mm_sqrt_ps(_mm_add_ps(_mm_mul_ps(veclo[0], veclo[0]), _mm_add_ps(_mm_mul_ps(veclo[1], veclo[1]), _mm_mul_ps(veclo[2], veclo[2]))));
	__m128 normhi = _mm_sqrt_ps(_mm_add_ps(_mm_mul_ps(vechi[0], vechi[0]), _mm_add_ps(_mm_mul_ps(vechi[1], vechi[1]), _mm_mul_ps(vechi[2], vechi[2]))));
	__m128 isNormZerolo = _mm_cmpeq_ps(normlo, _mm_setzero_ps());
	__m128 isNormZerohi = _mm_cmpeq_ps(normhi, _mm_setzero_ps());
	for (int c = 0; c < 3; c++) {
		static const float SQRT3 = sqrtf(3.0f);
		veclo[c] = _mm_xor_ps(veclo[c], _mm_and_ps(isNormZerolo, _mm_set1_ps(1.0f)));
		vechi[c] = _mm_xor_ps(vechi[c], _mm_and_ps(isNormZerohi, _mm_set1_ps(1.0f)));
		normlo = _mm_xor_ps(normlo, _mm_and_ps(isNormZerolo, _mm_set1_ps(SQRT3)));
		normhi = _mm_xor_ps(normhi, _mm_and_ps(isNormZerohi, _mm_set1_ps(SQRT3)));
	}
	// normalize, scale and convert to 16-bit integers
	__m128 normMultiplierlo = _mm_div_ps(_mm_set1_ps(64.0f), normlo);
	__m128 normMultiplierhi = _mm_div_ps(_mm_set1_ps(64.0f), normhi);
	__m128i axis[3];
	for (int c = 0; c < 3; c++) {
		__m128i lo = _mm_cvtps_epi32(_mm_mul_ps(veclo[c], normMultiplierlo));
		__m128i hi = _mm_cvtps_epi32(_mm_mul_ps(vechi[c], normMultiplierhi));
		axis[c] = _mm_packs_epi32(lo, hi);
	}

	// find bounding interval along axis
	__m128i minDot = _mm_set1_epi16(INT16_MAX), maxDot = _mm_set1_epi16(INT16_MIN);
	for (int i = 0; i < 16; i++) {
		__m128i Rdiff = _mm_sub_epi16(pixels[i][0], avgColor[0]);
		__m128i Gdiff = _mm_sub_epi16(pixels[i][1], avgColor[1]);
		__m128i Bdiff = _mm_sub_epi16(pixels[i][2], avgColor[2]);
		__m128i dot = _mm_add_epi16(_mm_mullo_epi16(axis[0], Rdiff), _mm_add_epi16(_mm_mullo_epi16(axis[1], Gdiff), _mm_mullo_epi16(axis[2], Bdiff)));
		minDot = _mm_min_epi16(minDot, dot);
		maxDot = _mm_max_epi16(maxDot, dot);
	}
	// find endpoints of this interval
	__m128i bmin[3], bmax[3];
	for (int c = 0; c < 3; c++) {
		__m128i minlo = _mm_mullo_epi16(axis[c], minDot);
		__m128i minhi = _mm_mulhi_epi16(axis[c], minDot);
		__m128i minscaled = _mm_packs_epi32(_mm_srai_epi32(_mm_unpacklo_epi16(minlo, minhi), 12), _mm_srai_epi32(_mm_unpackhi_epi16(minlo, minhi), 12));
		__m128i maxlo = _mm_mullo_epi16(axis[c], maxDot);
		__m128i maxhi = _mm_mulhi_epi16(axis[c], maxDot);
		__m128i maxscaled = _mm_packs_epi32(_mm_srai_epi32(_mm_unpacklo_epi16(maxlo, maxhi), 12), _mm_srai_epi32(_mm_unpackhi_epi16(maxlo, maxhi), 12));
		bmin[c] = _mm_add_epi16(avgColor[c], minscaled);
		bmax[c] = _mm_add_epi16(avgColor[c], maxscaled);
	}
	// specify initial key colors as the interval symmetrically reduced by 25%
	__m128i keyColorA[3], keyColorB[3];
	for (int c = 0; c < 3; c++) {
		__m128i diag = _mm_sub_epi16(bmax[c], bmin[c]);
		diag = _mm_srai_epi16(diag, 3);
		keyColorA[c] = _mm_add_epi16(bmin[c], diag);
		keyColorB[c] = _mm_sub_epi16(bmax[c], diag);
	}

	__m128i key565A[3], key565B[3];
	__m128i ids[16];
	for (int iter = 0; ; iter++) {
		// round outwards (almost) in case of low-variance blocks
		__m128i diff0 = _mm_sub_epi16(keyColorA[0], keyColorB[0]);
		__m128i diff1 = _mm_sub_epi16(keyColorA[1], keyColorB[1]);
		__m128i diff2 = _mm_sub_epi16(keyColorA[2], keyColorB[2]);
		__m128i lowvar0 = _mm_and_si128(_mm_cmplt_epi16(diff0, _mm_set1_epi16(8)), _mm_cmpgt_epi16(diff0, _mm_set1_epi16(-8)));
		__m128i lowvar1 = _mm_and_si128(_mm_cmplt_epi16(diff1, _mm_set1_epi16(4)), _mm_cmpgt_epi16(diff1, _mm_set1_epi16(-4)));
		__m128i lowvar2 = _mm_and_si128(_mm_cmplt_epi16(diff2, _mm_set1_epi16(8)), _mm_cmpgt_epi16(diff2, _mm_set1_epi16(-8)));
		__m128i delta0 = _mm_cmplt_epi16(keyColorA[0], keyColorB[0]);
		__m128i delta1 = _mm_cmplt_epi16(keyColorA[1], keyColorB[1]);
		__m128i delta2 = _mm_cmplt_epi16(keyColorA[2], keyColorB[2]);
		delta0 = _mm_and_si128(lowvar0, _mm_xor_si128(_mm_and_si128(delta0, _mm_set1_epi16(-3)), _mm_andnot_si128(delta0, _mm_set1_epi16(3))));
		delta1 = _mm_and_si128(lowvar1, _mm_xor_si128(_mm_and_si128(delta1, _mm_set1_epi16(-1)), _mm_andnot_si128(delta1, _mm_set1_epi16(1))));
		delta2 = _mm_and_si128(lowvar2, _mm_xor_si128(_mm_and_si128(delta2, _mm_set1_epi16(-3)), _mm_andnot_si128(delta2, _mm_set1_epi16(3))));
		keyColorA[0] = _mm_add_epi16(keyColorA[0], delta0);
		keyColorB[0] = _mm_sub_epi16(keyColorB[0], delta0);
		keyColorA[1] = _mm_add_epi16(keyColorA[1], delta1);
		keyColorB[1] = _mm_sub_epi16(keyColorB[1], delta1);
		keyColorA[2] = _mm_add_epi16(keyColorA[2], delta2);
		keyColorB[2] = _mm_sub_epi16(keyColorB[2], delta2);
		// clamp to [0..255]
		keyColorA[0] = _mm_unpacklo_epi8(_mm_packus_epi16(keyColorA[0], _mm_setzero_si128()), _mm_setzero_si128());
		keyColorA[1] = _mm_unpacklo_epi8(_mm_packus_epi16(keyColorA[1], _mm_setzero_si128()), _mm_setzero_si128());
		keyColorA[2] = _mm_unpacklo_epi8(_mm_packus_epi16(keyColorA[2], _mm_setzero_si128()), _mm_setzero_si128());
		keyColorB[0] = _mm_unpacklo_epi8(_mm_packus_epi16(keyColorB[0], _mm_setzero_si128()), _mm_setzero_si128());
		keyColorB[1] = _mm_unpacklo_epi8(_mm_packus_epi16(keyColorB[1], _mm_setzero_si128()), _mm_setzero_si128());
		keyColorB[2] = _mm_unpacklo_epi8(_mm_packus_epi16(keyColorB[2], _mm_setzero_si128()), _mm_setzero_si128());
		// convert to 565 format: round(X * 31 / 255)
		key565A[0] = _mm_add_epi16(_mm_mullo_epi16(keyColorA[0], _mm_set1_epi16(31)), _mm_set1_epi16(128));
		key565A[1] = _mm_add_epi16(_mm_mullo_epi16(keyColorA[1], _mm_set1_epi16(63)), _mm_set1_epi16(128));
		key565A[2] = _mm_add_epi16(_mm_mullo_epi16(keyColorA[2], _mm_set1_epi16(31)), _mm_set1_epi16(128));
		key565B[0] = _mm_add_epi16(_mm_mullo_epi16(keyColorB[0], _mm_set1_epi16(31)), _mm_set1_epi16(128));
		key565B[1] = _mm_add_epi16(_mm_mullo_epi16(keyColorB[1], _mm_set1_epi16(63)), _mm_set1_epi16(128));
		key565B[2] = _mm_add_epi16(_mm_mullo_epi16(keyColorB[2], _mm_set1_epi16(31)), _mm_set1_epi16(128));
		// note: X/255 = X/256 + X/256^2 + (X/256^3 + ...)
		key565A[0] = _mm_srli_epi16(_mm_add_epi16(key565A[0], _mm_srli_epi16(key565A[0], 8)), 8);
		key565A[1] = _mm_srli_epi16(_mm_add_epi16(key565A[1], _mm_srli_epi16(key565A[1], 8)), 8);
		key565A[2] = _mm_srli_epi16(_mm_add_epi16(key565A[2], _mm_srli_epi16(key565A[2], 8)), 8);
		key565B[0] = _mm_srli_epi16(_mm_add_epi16(key565B[0], _mm_srli_epi16(key565B[0], 8)), 8);
		key565B[1] = _mm_srli_epi16(_mm_add_epi16(key565B[1], _mm_srli_epi16(key565B[1], 8)), 8);
		key565B[2] = _mm_srli_epi16(_mm_add_epi16(key565B[2], _mm_srli_epi16(key565B[2], 8)), 8);
		// if keycolors are equal, bump B-green a bit
		__m128i sameKeys = _mm_and_si128(_mm_and_si128(
			_mm_cmpeq_epi16(key565A[0], key565B[0]),
			_mm_cmpeq_epi16(key565A[1], key565B[1])),
			_mm_cmpeq_epi16(key565A[2], key565B[2])
		);
		__m128i bump1 = _mm_sub_epi16(_mm_cmplt_epi16(key565B[1], _mm_set1_epi16(32)), _mm_cmpgt_epi16(key565B[1], _mm_set1_epi16(31)));
		key565B[1] = _mm_sub_epi16(key565B[1], _mm_and_si128(sameKeys, bump1));
		// convert key colors back into 8-bit
		keyColorA[0] = _mm_add_epi16(_mm_mullo_epi16(key565A[0], _mm_set1_epi16(255)), _mm_set1_epi16(16));
		keyColorA[1] = _mm_add_epi16(_mm_mullo_epi16(key565A[1], _mm_set1_epi16(255)), _mm_set1_epi16(32));
		keyColorA[2] = _mm_add_epi16(_mm_mullo_epi16(key565A[2], _mm_set1_epi16(255)), _mm_set1_epi16(16));
		keyColorB[0] = _mm_add_epi16(_mm_mullo_epi16(key565B[0], _mm_set1_epi16(255)), _mm_set1_epi16(16));
		keyColorB[1] = _mm_add_epi16(_mm_mullo_epi16(key565B[1], _mm_set1_epi16(255)), _mm_set1_epi16(32));
		keyColorB[2] = _mm_add_epi16(_mm_mullo_epi16(key565B[2], _mm_set1_epi16(255)), _mm_set1_epi16(16));
		// note: X/31 = X/32 + X/32^2 + X/32^3 + (X/32^4 + ...)
		//       X/63 = X/64 + X/64^2 + X/64^3 + (X/64^4 + ...)
		keyColorA[0] = _mm_srli_epi16(_mm_add_epi16(keyColorA[0], _mm_srli_epi16(_mm_add_epi16(keyColorA[0], _mm_srli_epi16(keyColorA[0], 5)), 5)), 5);
		keyColorA[1] = _mm_srli_epi16(_mm_add_epi16(keyColorA[1], _mm_srli_epi16(_mm_add_epi16(keyColorA[1], _mm_srli_epi16(keyColorA[1], 6)), 6)), 6);
		keyColorA[2] = _mm_srli_epi16(_mm_add_epi16(keyColorA[2], _mm_srli_epi16(_mm_add_epi16(keyColorA[2], _mm_srli_epi16(keyColorA[2], 5)), 5)), 5);
		keyColorB[0] = _mm_srli_epi16(_mm_add_epi16(keyColorB[0], _mm_srli_epi16(_mm_add_epi16(keyColorB[0], _mm_srli_epi16(keyColorB[0], 5)), 5)), 5);
		keyColorB[1] = _mm_srli_epi16(_mm_add_epi16(keyColorB[1], _mm_srli_epi16(_mm_add_epi16(keyColorB[1], _mm_srli_epi16(keyColorB[1], 6)), 6)), 6);
		keyColorB[2] = _mm_srli_epi16(_mm_add_epi16(keyColorB[2], _mm_srli_epi16(_mm_add_epi16(keyColorB[2], _mm_srli_epi16(keyColorB[2], 5)), 5)), 5);

		// compute squared length of A-B key vector
		__m128i denomlo = _mm_setzero_si128();
		__m128i denomhi = _mm_setzero_si128();
		for (int c = 0; c < 3; c++) {
			__m128i diff = _mm_sub_epi16(keyColorB[c], keyColorA[c]);
			__m128i mullo = _mm_mullo_epi16(diff, diff);
			__m128i mulhi = _mm_mulhi_epi16(diff, diff);
			denomlo = _mm_add_epi32(denomlo, _mm_unpacklo_epi16(mullo, mulhi));
			denomhi = _mm_add_epi32(denomhi, _mm_unpackhi_epi16(mullo, mulhi));
		}
		assert(_mm_movemask_epi8(_mm_cmpgt_epi32(denomlo, _mm_setzero_si128())) == 0xFFFF);
		assert(_mm_movemask_epi8(_mm_cmpgt_epi32(denomhi, _mm_setzero_si128())) == 0xFFFF);
		// compute length-based multiplier to convert dot product into index
		__m128 invDenom3lo = _mm_add_ps(_mm_div_ps(_mm_set1_ps(3.0f), _mm_cvtepi32_ps(denomlo)), _mm_set1_ps(3.0f * FLT_EPSILON));
		__m128 invDenom3hi = _mm_add_ps(_mm_div_ps(_mm_set1_ps(3.0f), _mm_cvtepi32_ps(denomhi)), _mm_set1_ps(3.0f * FLT_EPSILON));

		// compute pixel indices
		for (int i = 0; i < 16; i++) {
			__m128i numerlo = _mm_setzero_si128();
			__m128i numerhi = _mm_setzero_si128();
			for (int c = 0; c < 3; c++) {
				__m128i ABdiff = _mm_sub_epi16(keyColorB[c], keyColorA[c]);
				__m128i APdiff = _mm_sub_epi16(pixels[i][c], keyColorA[c]);
				__m128i mullo = _mm_mullo_epi16(APdiff, ABdiff);
				__m128i mulhi = _mm_mulhi_epi16(APdiff, ABdiff);
				numerlo = _mm_add_epi32(numerlo, _mm_unpacklo_epi16(mullo, mulhi));
				numerhi = _mm_add_epi32(numerhi, _mm_unpackhi_epi16(mullo, mulhi));
			}
			__m128i klo = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(numerlo), invDenom3lo));
			__m128i khi = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(numerhi), invDenom3hi));
			__m128i k = _mm_packs_epi32(klo, khi);
			ids[i] = _mm_min_epi16(_mm_max_epi16(k, _mm_setzero_si128()), _mm_set1_epi16(3));
		}

		if (iter == 1)
			break;

		// compute equation system for least squares problem
		__m128i alpha = _mm_setzero_si128();
		__m128i beta = _mm_setzero_si128();
		__m128i gamma = _mm_setzero_si128();
		__m128i rightB[3] = {_mm_setzero_si128(), _mm_setzero_si128(), _mm_setzero_si128()};
		for (int i = 0; i < 16; i++) {
			__m128i idx = ids[i];
			__m128i sidx = _mm_sub_epi16(_mm_set1_epi16(3), ids[i]);
			alpha = _mm_add_epi16(alpha, _mm_mullo_epi16(sidx, sidx));
			beta = _mm_add_epi16(beta, _mm_mullo_epi16(idx, idx));
			gamma = _mm_add_epi16(gamma, _mm_mullo_epi16(sidx, idx));
			rightB[0] = _mm_add_epi16(rightB[0], _mm_mullo_epi16(idx, pixels[i][0]));
			rightB[1] = _mm_add_epi16(rightB[1], _mm_mullo_epi16(idx, pixels[i][1]));
			rightB[2] = _mm_add_epi16(rightB[2], _mm_mullo_epi16(idx, pixels[i][2]));
		}
		__m128i rightA[3];
		for (int c = 0; c < 3; c++) {
			rightA[c] = _mm_sub_epi16(_mm_mullo_epi16(sumColor[c], _mm_set1_epi16(3)), rightB[c]);
			// note: rightX[*] can be up to 9*16*255 > 32K, so these values are actually unsigned
			rightA[c] = _mm_mullo_epi16(rightA[c], _mm_set1_epi16(3));
			rightB[c] = _mm_mullo_epi16(rightB[c], _mm_set1_epi16(3));
		}

		// solve equation system
		__m128i ablo = _mm_mullo_epi16(alpha, beta);
		__m128i abhi = _mm_mulhi_epu16(alpha, beta);
		__m128i gglo = _mm_mullo_epi16(gamma, gamma);
		__m128i gghi = _mm_mulhi_epu16(gamma, gamma);
		__m128i detlo = _mm_sub_epi32(_mm_unpacklo_epi16(ablo, abhi), _mm_unpacklo_epi16(gglo, gghi));
		__m128i dethi = _mm_sub_epi32(_mm_unpackhi_epi16(ablo, abhi), _mm_unpackhi_epi16(gglo, gghi));
		assert(_mm_movemask_epi8(_mm_cmplt_epi32(detlo, _mm_setzero_si128())) == 0);
		assert(_mm_movemask_epi8(_mm_cmplt_epi32(dethi, _mm_setzero_si128())) == 0);
		__m128i detZero = _mm_packs_epi32(_mm_cmpeq_epi32(detlo, _mm_setzero_si128()), _mm_cmpeq_epi32(dethi, _mm_setzero_si128()));
		__m128 invDetlo = _mm_div_ps(_mm_set1_ps(1.0f), _mm_max_ps(_mm_cvtepi32_ps(detlo), _mm_set1_ps(1.0f)));
		__m128 invDethi = _mm_div_ps(_mm_set1_ps(1.0f), _mm_max_ps(_mm_cvtepi32_ps(dethi), _mm_set1_ps(1.0f)));
		for (int c = 0; c < 3; c++) {
			__m128i bralo = _mm_mullo_epi16(beta, rightA[c]);
			__m128i brahi = _mm_mulhi_epu16(beta, rightA[c]);
			__m128i grblo = _mm_mullo_epi16(gamma, rightB[c]);
			__m128i grbhi = _mm_mulhi_epu16(gamma, rightB[c]);
			__m128i detAlo = _mm_sub_epi32(_mm_unpacklo_epi16(bralo, brahi), _mm_unpacklo_epi16(grblo, grbhi));
			__m128i detAhi = _mm_sub_epi32(_mm_unpackhi_epi16(bralo, brahi), _mm_unpackhi_epi16(grblo, grbhi));
			__m128i keyAlo = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(detAlo), invDetlo));
			__m128i keyAhi = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(detAhi), invDethi));
			__m128i keyA = _mm_packs_epi32(keyAlo, keyAhi);
			keyColorA[c] = _mm_xor_si128(_mm_andnot_si128(detZero, keyA), _mm_and_si128(detZero, keyColorA[c]));
			__m128i arblo = _mm_mullo_epi16(alpha, rightB[c]);
			__m128i arbhi = _mm_mulhi_epu16(alpha, rightB[c]);
			__m128i gralo = _mm_mullo_epi16(gamma, rightA[c]);
			__m128i grahi = _mm_mulhi_epu16(gamma, rightA[c]);
			__m128i detBlo = _mm_sub_epi32(_mm_unpacklo_epi16(arblo, arbhi), _mm_unpacklo_epi16(gralo, grahi));
			__m128i detBhi = _mm_sub_epi32(_mm_unpackhi_epi16(arblo, arbhi), _mm_unpackhi_epi16(gralo, grahi));
			__m128i keyBlo = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(detBlo), invDetlo));
			__m128i keyBhi = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(detBhi), invDethi));
			__m128i keyB = _mm_packs_epi32(keyBlo, keyBhi);
			keyColorB[c] = _mm_xor_si128(_mm_andnot_si128(detZero, keyB), _mm_and_si128(detZero, keyColorB[c]));
			// key colors will be clamped at the beginning of next iteration
		}
	}

	// pack 565 key colors into 16-bit words
	__m128i baseA = _mm_xor_si128(key565A[2], _mm_xor_si128(_mm_slli_epi16(key565A[1], 5), _mm_slli_epi16(key565A[0], 11)));
	__m128i baseB = _mm_xor_si128(key565B[2], _mm_xor_si128(_mm_slli_epi16(key565B[1], 5), _mm_slli_epi16(key565B[0], 11)));
	assert(_mm_movemask_epi8(_mm_cmpeq_epi16(baseA, baseB)) == 0);
	// compute pixel mask (in 2 halves)
	__m128i masklo = _mm_setzero_si128();
	__m128i maskhi = _mm_setzero_si128();
	for (int i = 7; i >= 0; i--) {
		// reorder for DXT
		__m128i idslo = ids[i + 0];
		__m128i idshi = ids[i + 8];
		idslo = _mm_sub_epi16(idslo, _mm_cmpgt_epi16(idslo, _mm_setzero_si128()));
		idshi = _mm_sub_epi16(idshi, _mm_cmpgt_epi16(idshi, _mm_setzero_si128()));
		idslo = _mm_sub_epi16(idslo, _mm_and_si128(_mm_cmpeq_epi16(idslo, _mm_set1_epi16(4)), _mm_set1_epi16(3)));
		idshi = _mm_sub_epi16(idshi, _mm_and_si128(_mm_cmpeq_epi16(idshi, _mm_set1_epi16(4)), _mm_set1_epi16(3)));
		masklo = _mm_xor_si128(_mm_slli_epi16(masklo, 2), idslo);
		maskhi = _mm_xor_si128(_mm_slli_epi16(maskhi, 2), idshi);
	}
	// make sure 565 colors have A > B
	baseA = _mm_add_epi16(baseA, _mm_set1_epi16(0x8000U));  // unsigned -> signed comparison
	baseB = _mm_add_epi16(baseB, _mm_set1_epi16(0x8000U));
	__m128i revMask = _mm_cmplt_epi16(baseA, baseB);
	revMask = _mm_and_si128(revMask, _mm_set1_epi8(0x55));
	masklo = _mm_xor_si128(masklo, revMask);
	maskhi = _mm_xor_si128(maskhi, revMask);
	__m128i nbaseA = _mm_max_epi16(baseA, baseB);
	__m128i nbaseB = _mm_min_epi16(baseA, baseB);
	baseA = _mm_sub_epi16(nbaseA, _mm_set1_epi16(0x8000U));
	baseB = _mm_sub_epi16(nbaseB, _mm_set1_epi16(0x8000U));

	// shuffle/transpose 4 x 16-bit masks into 8 x 64-bit blocks
	__m128i outA32 = _mm_unpacklo_epi16(baseA, baseB);
	__m128i outB32 = _mm_unpackhi_epi16(baseA, baseB);
	__m128i outC32 = _mm_unpacklo_epi16(masklo, maskhi);
	__m128i outD32 = _mm_unpackhi_epi16(masklo, maskhi);
	outputs[0] = _mm_unpacklo_epi32(outA32, outC32);
	outputs[1] = _mm_unpackhi_epi32(outA32, outC32);
	outputs[2] = _mm_unpacklo_epi32(outB32, outD32);
	outputs[3] = _mm_unpackhi_epi32(outB32, outD32);
}

static void RgtcKernel8x4( const byte *srcPtr, int stride, byte *dstPtr ) {
	__m128i rgbaRows[2][4];
	LoadBlocks<2>(srcPtr, stride, rgbaRows);

	__m128i rgrgRows[4];
	ExtractRedGreen_x2(rgbaRows, rgrgRows);
	__m128i output[2];
	ProcessAlphaBlock_x4(rgrgRows, output);

	StoreOutput<2>(dstPtr, output);
}

static void Dxt1Kernel32x4( const byte *srcPtr, int stride, byte *dstPtr ) {
	__m128i rgbaRows[8][4];
	LoadBlocks<8>(srcPtr, stride, rgbaRows);

	__m128i output[4];
	ProcessColorBlock_x8(rgbaRows, output);

	StoreOutput<4>(dstPtr, output);
}

static void Dxt3Kernel32x4( const byte *srcPtr, int stride, byte *dstPtr ) {
	__m128i rgbaRows[8][4];
	LoadBlocks<8>(srcPtr, stride, rgbaRows);

	__m128i colorOutput[4];
	ProcessColorBlock_x8(rgbaRows, colorOutput);

	__m128i alphaOutput[4];
	ProcessAlphaBlock4b_x2(rgbaRows + 0, alphaOutput + 0);
	ProcessAlphaBlock4b_x2(rgbaRows + 2, alphaOutput + 1);
	ProcessAlphaBlock4b_x2(rgbaRows + 4, alphaOutput + 2);
	ProcessAlphaBlock4b_x2(rgbaRows + 6, alphaOutput + 3);

	__m128i output[8];
	output[0] = _mm_unpacklo_epi64(alphaOutput[0], colorOutput[0]);
	output[1] = _mm_unpackhi_epi64(alphaOutput[0], colorOutput[0]);
	output[2] = _mm_unpacklo_epi64(alphaOutput[1], colorOutput[1]);
	output[3] = _mm_unpackhi_epi64(alphaOutput[1], colorOutput[1]);
	output[4] = _mm_unpacklo_epi64(alphaOutput[2], colorOutput[2]);
	output[5] = _mm_unpackhi_epi64(alphaOutput[2], colorOutput[2]);
	output[6] = _mm_unpacklo_epi64(alphaOutput[3], colorOutput[3]);
	output[7] = _mm_unpackhi_epi64(alphaOutput[3], colorOutput[3]);
	StoreOutput<8>(dstPtr, output);
}

static void Dxt5Kernel32x4( const byte *srcPtr, int stride, byte *dstPtr ) {
	__m128i rgbaRows[8][4];
	LoadBlocks<8>(srcPtr, stride, rgbaRows);

	__m128i colorOutput[4];
	ProcessColorBlock_x8(rgbaRows, colorOutput);

	__m128i alphaRows[4];
	__m128i alphaOutput[4];
	ExtractAlpha_x4(rgbaRows + 0, alphaRows);
	ProcessAlphaBlock_x4(alphaRows, alphaOutput + 0);
	ExtractAlpha_x4(rgbaRows + 4, alphaRows);
	ProcessAlphaBlock_x4(alphaRows, alphaOutput + 2);

	__m128i output[8];
	output[0] = _mm_unpacklo_epi64(alphaOutput[0], colorOutput[0]);
	output[1] = _mm_unpackhi_epi64(alphaOutput[0], colorOutput[0]);
	output[2] = _mm_unpacklo_epi64(alphaOutput[1], colorOutput[1]);
	output[3] = _mm_unpackhi_epi64(alphaOutput[1], colorOutput[1]);
	output[4] = _mm_unpacklo_epi64(alphaOutput[2], colorOutput[2]);
	output[5] = _mm_unpackhi_epi64(alphaOutput[2], colorOutput[2]);
	output[6] = _mm_unpacklo_epi64(alphaOutput[3], colorOutput[3]);
	output[7] = _mm_unpackhi_epi64(alphaOutput[3], colorOutput[3]);
	StoreOutput<8>(dstPtr, output);
}

template<int KernelBlocks, int OutputWordsPerBlock>
static void ProcessWithKernel( void (*kernelFunction)(const byte*, int, byte*), const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) {
	// kernel processes KernelBlocks blocks of size 4 x 4 pixels each
	// for each block, OutputWordsPerBlock words are produced, each of size = 8 bytes
	static const int KernelPixelsInRow = KernelBlocks * 4;
	static const int KernelBytesInRow = KernelPixelsInRow * 4;
	static const int KernelOutputWords = OutputWordsPerBlock * KernelBlocks;
	static const int KernelOutputBytes = KernelOutputWords * 8;
	static const int KernelOutputRegs = KernelOutputWords / 16;

	for (int sr = 0; sr < height; sr += 4) {
		int fitsNum = (sr + 4 <= height ? width / KernelPixelsInRow : 0);

		int iters;
		for (iters = 0; iters < fitsNum; iters++) {
			// load blocks directly from image memory
			kernelFunction(&srcPtr[sr * stride + KernelBytesInRow * iters], stride, dstPtr);
			dstPtr += KernelOutputBytes;
		}

		for (int sc = KernelPixelsInRow * iters; sc < width; sc += KernelPixelsInRow) {
			ALIGNTYPE16 byte inputBlocks[4][KernelBytesInRow];
			ALIGNTYPE16 byte outputData[KernelOutputBytes];
			DxtCompress::LoadBoundaryBlocks(srcPtr, stride, width, height, sr, sc, KernelBlocks, &inputBlocks[0][0]);

			kernelFunction(&inputBlocks[0][0], sizeof(inputBlocks[0]), outputData);
			dstPtr += DxtCompress::StoreBoundaryOutput(dstPtr, width, height, sr, sc, KernelBlocks, OutputWordsPerBlock, outputData);
		}
	}
}

}

void idSIMD_SSE2::CompressRGTCFromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) {
	using namespace DxtCompress;
	ProcessWithKernel<2, 2>( RgtcKernel8x4, srcPtr, width, height, stride, dstPtr );
}

void idSIMD_SSE2::CompressDXT1FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) {
	using namespace DxtCompress;
	ProcessWithKernel<8, 1>( Dxt1Kernel32x4, srcPtr, width, height, stride, dstPtr );
}

void idSIMD_SSE2::CompressDXT3FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) {
	using namespace DxtCompress;
	ProcessWithKernel<8, 2>( Dxt3Kernel32x4, srcPtr, width, height, stride, dstPtr );
}

void idSIMD_SSE2::CompressDXT5FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) {
	using namespace DxtCompress;
	ProcessWithKernel<8, 2>( Dxt5Kernel32x4, srcPtr, width, height, stride, dstPtr );
}

#endif
