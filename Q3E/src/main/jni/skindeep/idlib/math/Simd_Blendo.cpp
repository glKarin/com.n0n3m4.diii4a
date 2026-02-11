/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2020 Stephen Pridham (Mikkelsen tangent space support)
Copyright (C) 2021-2024 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/




#if defined(__BLENDO_SIMD_INLINE__)

#include "sys/platform.h"
#include "idlib/geometry/DrawVert.h"
#include "idlib/geometry/JointTransform.h"
#include "idlib/math/Plane.h"
#include "idlib/bv/Bounds.h"
#include "idlib/Lib.h"
#include "framework/Common.h"
#include "renderer/Model.h"

#include "idlib/math/Simd.h"


#include "idlib/math/Vector.h"
#include "idlib/math/Angles.h"
#include "idlib/math/Quat.h"
#include "idlib/math/Rotation.h"
#include "idlib/math/Plane.h"
#include "idlib/Lib.h"
#include "idlib/math/Math.h"
#include "idlib/geometry/DrawVert.h"
#include "idlib/geometry/JointTransform.h"
#include "framework/Common.h"
#include "renderer/Model.h"
#include "sys/platform.h"

#include "idlib/math/Simd_Blendo.h"

#define DRAWVERT_SIZE				60
#define DRAWVERT_XYZ_OFFSET			(0*4)
#define DRAWVERT_ST_OFFSET			(3*4)
#define DRAWVERT_NORMAL_OFFSET		(5*4)
#define DRAWVERT_TANGENT0_OFFSET	(8*4)
#define DRAWVERT_TANGENT1_OFFSET	(11*4)
#define DRAWVERT_COLOR_OFFSET		(14*4)


#if 1 // ACTUAL SIMD

#if 1 // idSIMD_sse 1
/*
============
Dot

dst[i] = constant.Normal() * src[i].xyz + constant[3];
============
*/
void SIMD_VPCALL idSIMDProcessor::Dot(float *dst, const idPlane &constant, const idDrawVert *src, const int count) {
	// 0,  1,  2
	// 3,  4,  5
	// 6,  7,  8
	// 9, 10, 11

	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;	// Declare 8 xmm registers.
	int count_l4 = count;                                   // count_l4 = eax
	int count_l1 = count;                                   // count_l1 = edx
	char *constant_p = (char *)&constant;                   // constant_p = edi
	char *src_p = (char *)src;                             // src_p = esi
	char *dst_p = (char *)dst;                             // dst_p = ecx

														   //assert(sizeof(idDrawVert) == SIMD_VERT_SIZE);
														   //assert(ptrdiff_t(&src->xyz) - ptrdiff_t(src) == SIMD_VERT_XYZ_OFFSET);

	count_l4 = count_l4 & ~3;
	xmm4 = _mm_load_ss((float *)(constant_p));
	xmm4 = _mm_shuffle_ps(xmm4, xmm4, SIMD_R_SHUFFLEPS(0, 0, 0, 0));
	xmm5 = _mm_load_ss((float *)(constant_p + 4));
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, SIMD_R_SHUFFLEPS(0, 0, 0, 0));
	xmm6 = _mm_load_ss((float *)(constant_p + 8));
	xmm6 = _mm_shuffle_ps(xmm6, xmm6, SIMD_R_SHUFFLEPS(0, 0, 0, 0));
	xmm7 = _mm_load_ss((float *)(constant_p + 12));
	xmm7 = _mm_shuffle_ps(xmm7, xmm7, SIMD_R_SHUFFLEPS(0, 0, 0, 0));


	if (count_l4 != 0) {

		count_l4 = count_l4 * SIMD_VERT_SIZE;
		src_p = src_p + count_l4;
		count_l4 = -count_l4;

		do {
			xmm0 = _mm_load_ss((float *)(src_p + count_l4 + 1 * SIMD_VERT_SIZE + SIMD_VERT_XYZ_OFFSET + 0));        // 3,  X,  X,  X
			xmm2 = _mm_load_ss((float *)(src_p + count_l4 + 0 * SIMD_VERT_SIZE + SIMD_VERT_XYZ_OFFSET + 8));        // 2,  X,  X,  X
			xmm0 = _mm_loadh_pi(xmm0, (__m64 *) (src_p + count_l4 + 0 * SIMD_VERT_SIZE + SIMD_VERT_XYZ_OFFSET + 0)); // 3,  X,  0,  1
			xmm1 = xmm0;							                                                    // 3,  X,  0,  1


			xmm1 = _mm_loadl_pi(xmm1, (__m64 *) (src_p + count_l4 + 1 * SIMD_VERT_SIZE + SIMD_VERT_XYZ_OFFSET + 4)); // 4,  5,  0,  1
			xmm2 = _mm_shuffle_ps(xmm2, xmm1, SIMD_R_SHUFFLEPS(0, 1, 0, 1));                               // 2,  X,  4,  5

			xmm3 = _mm_load_ss((float *)(src_p + count_l4 + 3 * SIMD_VERT_SIZE + SIMD_VERT_XYZ_OFFSET + 0));        // 9,  X,  X,  X
			xmm3 = _mm_loadh_pi(xmm3, (__m64 *) (src_p + count_l4 + 2 * SIMD_VERT_SIZE + SIMD_VERT_XYZ_OFFSET + 0)); // 9,  X,  6,  7
			xmm0 = _mm_shuffle_ps(xmm0, xmm3, SIMD_R_SHUFFLEPS(2, 0, 2, 0));                               // 0,  3,  6,  9

			xmm3 = _mm_loadl_pi(xmm3, (__m64 *)(src_p + count_l4 + 3 * SIMD_VERT_SIZE + SIMD_VERT_XYZ_OFFSET + 4));  // 10, 11, 6,  7
			xmm1 = _mm_shuffle_ps(xmm1, xmm3, SIMD_R_SHUFFLEPS(3, 0, 3, 0));                               // 1,  4,  7,  10

			xmm3 = _mm_loadh_pi(xmm3, (__m64 *)(src_p + count_l4 + 2 * SIMD_VERT_SIZE + SIMD_VERT_XYZ_OFFSET + 8));  // 10, 11, 8,  X
			xmm2 = _mm_shuffle_ps(xmm2, xmm3, SIMD_R_SHUFFLEPS(0, 3, 2, 1));                               // 2,  5,  8,  11

			dst_p = dst_p + 16;
			count_l4 = count_l4 + 4 * SIMD_VERT_SIZE;


			xmm0 = _mm_mul_ps(xmm0, xmm4);
			xmm1 = _mm_mul_ps(xmm1, xmm5);
			xmm2 = _mm_mul_ps(xmm2, xmm6);
			xmm0 = _mm_add_ps(xmm0, xmm7);
			xmm0 = _mm_add_ps(xmm0, xmm1);
			xmm0 = _mm_add_ps(xmm0, xmm2);

			_mm_storel_pi((__m64 *) (dst_p - 16 + 0), xmm0);
			_mm_storeh_pi((__m64 *) (dst_p - 16 + 8), xmm0);
		} while (count_l4 < 0);
	}


	count_l1 = count_l1 & 3;
	if (count_l1 != 0) {

		do {
			xmm0 = _mm_load_ss((float *)(src_p + count_l4 + SIMD_VERT_XYZ_OFFSET + 0));
			xmm1 = _mm_load_ss((float *)(src_p + count_l4 + SIMD_VERT_XYZ_OFFSET + 4));
			xmm2 = _mm_load_ss((float *)(src_p + count_l4 + SIMD_VERT_XYZ_OFFSET + 8));
			xmm0 = _mm_mul_ss(xmm0, xmm4);
			xmm1 = _mm_mul_ss(xmm1, xmm5);
			xmm2 = _mm_mul_ss(xmm2, xmm6);
			xmm0 = _mm_add_ss(xmm0, xmm7);
			dst_p = dst_p + 4;
			xmm0 = _mm_add_ss(xmm0, xmm1);
			count_l4 = count_l4 + SIMD_VERT_SIZE;
			xmm0 = _mm_add_ss(xmm0, xmm2);
			count_l1 = count_l1 - 1;
			_mm_store_ss((float *)(dst_p - 4), xmm0);
		} while (count_l1 != 0);
	}
}

/*
============
MinMax
============
*/
void SIMD_VPCALL idSIMDProcessor::MinMax(idVec3 &min, idVec3 &max, const idDrawVert *src, const int *indexes, const int count) {

	assert(sizeof(idDrawVert) == SIMD_VERT_SIZE);
	assert(ptrdiff_t(&src->xyz) - ptrdiff_t(src) == SIMD_VERT_XYZ_OFFSET);

	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	char *indexes_p;
	char *src_p;
	int count_l;
	int edx;
	char *min_p;
	char *max_p;

	xmm0 = _mm_load_ss(&idMath::INFINITY);
	// To satisfy the compiler use xmm0 instead.
	xmm1 = _mm_xor_ps(xmm0, xmm0);
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, SIMD_R_SHUFFLEPS(0, 0, 0, 0));
	xmm1 = _mm_sub_ps(xmm1, xmm0);
	xmm2 = xmm0;
	xmm3 = xmm1;

	indexes_p = (char *)indexes;
	src_p = (char *)src;
	count_l = count;
	count_l = count_l & ~3;
	if (count_l != 0) {

		count_l = count_l << 2;
		indexes_p = indexes_p + count_l;
		count_l = -count_l;

		do {

			edx = *((int*)(indexes_p + count_l + 0));
			edx = edx * SIMD_VERT_SIZE;
			xmm4 = _mm_load_ss((float *)(src_p + edx + SIMD_VERT_XYZ_OFFSET + 8));
			xmm4 = _mm_loadh_pi(xmm4, (__m64 *) (src_p + edx + SIMD_VERT_XYZ_OFFSET + 0));
			xmm0 = _mm_min_ps(xmm0, xmm4);
			xmm1 = _mm_max_ps(xmm1, xmm4);

			edx = *((int*)(indexes_p + count_l + 4));
			edx = edx * SIMD_VERT_SIZE;
			xmm5 = _mm_load_ss((float *)(src_p + edx + SIMD_VERT_XYZ_OFFSET + 0));
			xmm5 = _mm_loadh_pi(xmm5, (__m64 *) (src_p + edx + SIMD_VERT_XYZ_OFFSET + 4));
			xmm2 = _mm_min_ps(xmm2, xmm5);
			xmm3 = _mm_max_ps(xmm3, xmm5);

			edx = *((int*)(indexes_p + count_l + 8));
			edx = edx * SIMD_VERT_SIZE;
			xmm6 = _mm_load_ss((float *)(src_p + edx + SIMD_VERT_XYZ_OFFSET + 8));
			xmm6 = _mm_loadh_pi(xmm6, (__m64 *) (src_p + edx + SIMD_VERT_XYZ_OFFSET + 0));
			xmm0 = _mm_min_ps(xmm0, xmm6);
			xmm1 = _mm_max_ps(xmm1, xmm6);

			edx = *((int*)(indexes_p + count_l + 12));
			edx = edx * SIMD_VERT_SIZE;
			xmm7 = _mm_load_ss((float *)(src_p + edx + SIMD_VERT_XYZ_OFFSET + 0));
			xmm7 = _mm_loadh_pi(xmm7, (__m64 *) (src_p + edx + SIMD_VERT_XYZ_OFFSET + 4));
			xmm2 = _mm_min_ps(xmm2, xmm7);
			xmm3 = _mm_max_ps(xmm3, xmm7);

			count_l = count_l + 4 * 4;
		} while (count_l < 0);
	}

	count_l = count;
	count_l = count_l & 3;
	if (count_l != 0) {
		count_l = count_l << 2;
		indexes_p = indexes_p + count_l;
		count_l = -count_l;

		do {

			edx = *((int*)(indexes_p + count_l + 0));
			edx = edx * SIMD_VERT_SIZE;
			xmm4 = _mm_load_ss((float *)(src_p + edx + SIMD_VERT_XYZ_OFFSET + 8));
			xmm4 = _mm_loadh_pi(xmm4, (__m64 *) (src_p + edx + SIMD_VERT_XYZ_OFFSET + 0));
			xmm0 = _mm_min_ps(xmm0, xmm4);
			xmm1 = _mm_max_ps(xmm1, xmm4);
			count_l = count_l + 4;
		} while (count_l < 0);

	}

	xmm2 = _mm_shuffle_ps(xmm2, xmm2, SIMD_R_SHUFFLEPS(3, 1, 0, 2));
	xmm3 = _mm_shuffle_ps(xmm3, xmm3, SIMD_R_SHUFFLEPS(3, 1, 0, 2));
	xmm0 = _mm_min_ps(xmm0, xmm2);
	xmm1 = _mm_max_ps(xmm1, xmm3);
	min_p = (char *)&min;
	_mm_storeh_pi((__m64 *)(min_p), xmm0);
	_mm_store_ss((float *)(min_p + 8), xmm0);
	max_p = (char *)&max;
	_mm_storeh_pi((__m64 *)(max_p), xmm1);
	_mm_store_ss((float *)(max_p + 8), xmm1);
}


/*
============
MinMax
============
*/
void SIMD_VPCALL idSIMDProcessor::MinMax(idVec3 &min, idVec3 &max, const idDrawVert *src, const int count) {
#if 0
	min[0] = min[1] = min[2] = idMath::INFINITY; max[0] = max[1] = max[2] = -idMath::INFINITY;
#define OPER(X) const idVec3 &v = src[(X)].xyz; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	SIMD_UNROLL1(OPER)
#undef OPER
#else
	// adopted from the indexed version
	//#define OPER(X) const idVec3 &v = src[(X)].xyz;				if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	//#define OPER(X) const idVec3 &v = src[indexes[(X)]].xyz;		if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }

	assert(sizeof(idDrawVert) == SIMD_VERT_SIZE);
	assert(ptrdiff_t(&src->xyz) - ptrdiff_t(src) == SIMD_VERT_XYZ_OFFSET);

	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	//char *indexes_p;
	char *src_p;
	int count_l;
	int edx;
	char *min_p;
	char *max_p;

	xmm0 = _mm_load_ss(&idMath::INFINITY);
	// To satisfy the compiler use xmm0 instead.
	xmm1 = _mm_xor_ps(xmm0, xmm0);
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, SIMD_R_SHUFFLEPS(0, 0, 0, 0));
	xmm1 = _mm_sub_ps(xmm1, xmm0);
	xmm2 = xmm0;
	xmm3 = xmm1;

	//indexes_p = (char *)indexes;
	src_p = (char *)src;
	count_l = count;
	count_l = count_l & ~3;
	if (count_l != 0) {
		//eric note: the iterator is negative and ticks upwards until it hits the end (src+count*size)
		count_l = count_l * SIMD_VERT_SIZE;
		src_p = src_p + count_l; // indexes_p = indexes_p + count_l;
		count_l = -count_l;

		do {

			/*
			mov			edx, [edi+eax+0]
			imul		edx, DRAWVERT_SIZE;
			movss		xmm4, [esi+edx+DRAWVERT_XYZ_OFFSET+8]
			movhps		xmm4, [esi+edx+DRAWVERT_XYZ_OFFSET+0]
			minps		xmm0, xmm4
			maxps		xmm1, xmm4
			*/
			edx = count_l + 0* SIMD_VERT_SIZE; //*((int*)(indexes_p + count_l + 0));
			xmm4 = _mm_load_ss((float *)(src_p + edx + SIMD_VERT_XYZ_OFFSET + 8));
			xmm4 = _mm_loadh_pi(xmm4, (__m64 *) (src_p + edx + SIMD_VERT_XYZ_OFFSET + 0));
			xmm0 = _mm_min_ps(xmm0, xmm4);
			xmm1 = _mm_max_ps(xmm1, xmm4);

			edx = count_l + 1 * SIMD_VERT_SIZE;
			xmm5 = _mm_load_ss((float *)(src_p + edx + SIMD_VERT_XYZ_OFFSET + 0));
			xmm5 = _mm_loadh_pi(xmm5, (__m64 *) (src_p + edx + SIMD_VERT_XYZ_OFFSET + 4));
			xmm2 = _mm_min_ps(xmm2, xmm5);
			xmm3 = _mm_max_ps(xmm3, xmm5);

			edx = count_l + 2 * SIMD_VERT_SIZE;
			xmm6 = _mm_load_ss((float *)(src_p + edx + SIMD_VERT_XYZ_OFFSET + 8));
			xmm6 = _mm_loadh_pi(xmm6, (__m64 *) (src_p + edx + SIMD_VERT_XYZ_OFFSET + 0));
			xmm0 = _mm_min_ps(xmm0, xmm6);
			xmm1 = _mm_max_ps(xmm1, xmm6);

			edx = count_l + 3 * SIMD_VERT_SIZE;
			xmm7 = _mm_load_ss((float *)(src_p + edx + SIMD_VERT_XYZ_OFFSET + 0));
			xmm7 = _mm_loadh_pi(xmm7, (__m64 *) (src_p + edx + SIMD_VERT_XYZ_OFFSET + 4));
			xmm2 = _mm_min_ps(xmm2, xmm7);
			xmm3 = _mm_max_ps(xmm3, xmm7);

			// eric note: increment 4 verts at a time
			count_l = count_l + 4*SIMD_VERT_SIZE;
		} while (count_l < 0);
	}

	count_l = count;
	count_l = count_l & 3;
	if (count_l != 0) {
		count_l = count_l * SIMD_VERT_SIZE;
		src_p = src_p + count_l;
		count_l = -count_l;

		do {

			edx =  count_l + 0;
			xmm4 = _mm_load_ss((float *)(src_p + edx + SIMD_VERT_XYZ_OFFSET + 8));
			xmm4 = _mm_loadh_pi(xmm4, (__m64 *) (src_p + edx + SIMD_VERT_XYZ_OFFSET + 0));
			xmm0 = _mm_min_ps(xmm0, xmm4);
			xmm1 = _mm_max_ps(xmm1, xmm4);
			count_l = count_l + SIMD_VERT_SIZE;
		} while (count_l < 0);

	}

	xmm2 = _mm_shuffle_ps(xmm2, xmm2, SIMD_R_SHUFFLEPS(3, 1, 0, 2));
	xmm3 = _mm_shuffle_ps(xmm3, xmm3, SIMD_R_SHUFFLEPS(3, 1, 0, 2));
	xmm0 = _mm_min_ps(xmm0, xmm2);
	xmm1 = _mm_max_ps(xmm1, xmm3);
	min_p = (char *)&min;
	_mm_storeh_pi((__m64 *)(min_p), xmm0);
	_mm_store_ss((float *)(min_p + 8), xmm0);
	max_p = (char *)&max;
	_mm_storeh_pi((__m64 *)(max_p), xmm1);
	_mm_store_ss((float *)(max_p + 8), xmm1);
#endif
}


/*
============
Dot

dst[i] = constant * src[i].Normal() + src[i][3];
============
*/
#pragma warning( push )
#pragma warning( disable : 4700 )
 void SIMD_VPCALL idSIMDProcessor::Dot(float *dst, const idVec3 &constant, const idPlane *src, const int count) {
	int count_l4;
	int count_l1;
	char *constant_p;
	char *src_p;
	char *dst_p;

	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;	// Declare 8 xmm registers.

	count_l4 = count;
	constant_p = (char *)&constant;
	count_l1 = count_l4;
	src_p = (char *)src;
	dst_p = (char *)dst;
	count_l4 = count_l4 & ~3;

	xmm5 = _mm_load_ss((float *)(constant_p + 0));
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, SIMD_R_SHUFFLEPS(0, 0, 0, 0));
	xmm6 = _mm_load_ss((float *)(constant_p + 4));
	xmm6 = _mm_shuffle_ps(xmm6, xmm6, SIMD_R_SHUFFLEPS(0, 0, 0, 0));
	xmm7 = _mm_load_ss((float *)(constant_p + 8));
	xmm7 = _mm_shuffle_ps(xmm7, xmm7, SIMD_R_SHUFFLEPS(0, 0, 0, 0));

	if (count_l4 != 0) {

		count_l4 = count_l4 * 16;
		src_p = src_p + count_l4;
		count_l4 = -count_l4;

		do {
			xmm1 = _mm_loadl_pi(xmm1, (__m64 *)(src_p + count_l4 + 0));
			xmm3 = _mm_loadl_pi(xmm3, (__m64 *)(src_p + count_l4 + 8));
			xmm1 = _mm_loadh_pi(xmm1, (__m64 *)(src_p + count_l4 + 16));
			xmm3 = _mm_loadh_pi(xmm3, (__m64 *)(src_p + count_l4 + 24));
			xmm2 = _mm_loadl_pi(xmm2, (__m64 *)(src_p + count_l4 + 32));
			xmm4 = _mm_loadl_pi(xmm4, (__m64 *)(src_p + count_l4 + 40));
			xmm2 = _mm_loadh_pi(xmm2, (__m64 *)(src_p + count_l4 + 48));
			xmm4 = _mm_loadh_pi(xmm4, (__m64 *)(src_p + count_l4 + 56));

			xmm0 = xmm1;
			xmm0 = _mm_shuffle_ps(xmm0, xmm2, SIMD_R_SHUFFLEPS(0, 2, 0, 2));
			xmm1 = _mm_shuffle_ps(xmm1, xmm2, SIMD_R_SHUFFLEPS(1, 3, 1, 3));
			xmm2 = xmm3;
			xmm2 = _mm_shuffle_ps(xmm2, xmm4, SIMD_R_SHUFFLEPS(0, 2, 0, 2));
			xmm3 = _mm_shuffle_ps(xmm3, xmm4, SIMD_R_SHUFFLEPS(1, 3, 1, 3));

			dst_p = dst_p + 16;
			count_l4 = count_l4 + 4 * 16;

			xmm0 = _mm_mul_ps(xmm0, xmm5);
			xmm1 = _mm_mul_ps(xmm1, xmm6);
			xmm2 = _mm_mul_ps(xmm2, xmm7);
			xmm0 = _mm_add_ps(xmm0, xmm3);
			xmm0 = _mm_add_ps(xmm0, xmm1);
			xmm0 = _mm_add_ps(xmm0, xmm2);

			_mm_storel_pi((__m64 *) (dst_p - 16 + 0), xmm0);
			_mm_storeh_pi((__m64 *) (dst_p - 16 + 8), xmm0);
		} while (count_l4 < 0);
	}

	count_l1 = count_l1 & 3;

	if (count_l1 != 0) {
		do {

			xmm0 = _mm_load_ss((float *)(src_p + count_l4 + 0));
			xmm1 = _mm_load_ss((float *)(src_p + count_l4 + 4));
			xmm2 = _mm_load_ss((float *)(src_p + count_l4 + 8));
			xmm3 = _mm_load_ss((float *)(src_p + count_l4 + 12));

			xmm0 = _mm_mul_ss(xmm0, xmm5);
			xmm1 = _mm_mul_ss(xmm1, xmm6);
			xmm2 = _mm_mul_ss(xmm2, xmm7);

			xmm0 = _mm_add_ss(xmm0, xmm3);
			dst_p = dst_p + 4;
			xmm0 = _mm_add_ss(xmm0, xmm1);
			count_l4 = count_l4 + 16;
			xmm0 = _mm_add_ss(xmm0, xmm2);
			count_l1 = count_l1 - 1;
			_mm_store_ss((float *)(dst_p - 4), xmm0);
		} while (count_l1 != 0);
	}
}
#pragma warning(pop)

#endif


#if 1 //  rb doom blendjoints
/*
============
BlendJoints
============
*/
void SIMD_VPCALL idSIMDProcessor::BlendJoints(idJointQuat* joints, const idJointQuat* blendJoints, const float lerp, const int* index, const int numJoints)
{

	if (lerp <= 0.0f)
	{
		return;
	}
	else if (lerp >= 1.0f)
	{
		for (int i = 0; i < numJoints; i++)
		{
			int j = index[i];
			joints[j] = blendJoints[j];
		}
		return;
	}

	const __m128 vlerp = { lerp, lerp, lerp, lerp };

	const __m128 vector_float_one = { 1.0f, 1.0f, 1.0f, 1.0f };
	const __m128 vector_float_sign_bit = __m128c(_mm_set_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000));
	const __m128 vector_float_rsqrt_c0 = { -3.0f,  -3.0f,  -3.0f,  -3.0f };
	const __m128 vector_float_rsqrt_c1 = { -0.5f,  -0.5f,  -0.5f,  -0.5f };
	const __m128 vector_float_tiny = { 1e-10f,    1e-10f,    1e-10f,    1e-10f };
	const __m128 vector_float_half_pi = { SIMD_PI * 0.5f, SIMD_PI * 0.5f, SIMD_PI * 0.5f, SIMD_PI * 0.5f };

	const __m128 vector_float_sin_c0 = { -2.39e-08f, -2.39e-08f, -2.39e-08f, -2.39e-08f };
	const __m128 vector_float_sin_c1 = { 2.7526e-06f, 2.7526e-06f, 2.7526e-06f, 2.7526e-06f };
	const __m128 vector_float_sin_c2 = { -1.98409e-04f, -1.98409e-04f, -1.98409e-04f, -1.98409e-04f };
	const __m128 vector_float_sin_c3 = { 8.3333315e-03f, 8.3333315e-03f, 8.3333315e-03f, 8.3333315e-03f };
	const __m128 vector_float_sin_c4 = { -1.666666664e-01f, -1.666666664e-01f, -1.666666664e-01f, -1.666666664e-01f };

	const __m128 vector_float_atan_c0 = { 0.0028662257f,  0.0028662257f,  0.0028662257f,  0.0028662257f };
	const __m128 vector_float_atan_c1 = { -0.0161657367f, -0.0161657367f, -0.0161657367f, -0.0161657367f };
	const __m128 vector_float_atan_c2 = { 0.0429096138f,  0.0429096138f,  0.0429096138f,  0.0429096138f };
	const __m128 vector_float_atan_c3 = { -0.0752896400f, -0.0752896400f, -0.0752896400f, -0.0752896400f };
	const __m128 vector_float_atan_c4 = { 0.1065626393f,  0.1065626393f,  0.1065626393f,  0.1065626393f };
	const __m128 vector_float_atan_c5 = { -0.1420889944f, -0.1420889944f, -0.1420889944f, -0.1420889944f };
	const __m128 vector_float_atan_c6 = { 0.1999355085f,  0.1999355085f,  0.1999355085f,  0.1999355085f };
	const __m128 vector_float_atan_c7 = { -0.3333314528f, -0.3333314528f, -0.3333314528f, -0.3333314528f };

	int i = 0;
	for (; i < numJoints - 3; i += 4)
	{
		const int n0 = index[i + 0];
		const int n1 = index[i + 1];
		const int n2 = index[i + 2];
		const int n3 = index[i + 3];

		__m128 jqa_0 = _mm_load_ps(joints[n0].q.ToFloatPtr());
		__m128 jqb_0 = _mm_load_ps(joints[n1].q.ToFloatPtr());
		__m128 jqc_0 = _mm_load_ps(joints[n2].q.ToFloatPtr());
		__m128 jqd_0 = _mm_load_ps(joints[n3].q.ToFloatPtr());

		__m128 jta_0 = _mm_load_ps(joints[n0].t.ToFloatPtr());
		__m128 jtb_0 = _mm_load_ps(joints[n1].t.ToFloatPtr());
		__m128 jtc_0 = _mm_load_ps(joints[n2].t.ToFloatPtr());
		__m128 jtd_0 = _mm_load_ps(joints[n3].t.ToFloatPtr());

		__m128 bqa_0 = _mm_load_ps(blendJoints[n0].q.ToFloatPtr());
		__m128 bqb_0 = _mm_load_ps(blendJoints[n1].q.ToFloatPtr());
		__m128 bqc_0 = _mm_load_ps(blendJoints[n2].q.ToFloatPtr());
		__m128 bqd_0 = _mm_load_ps(blendJoints[n3].q.ToFloatPtr());

		__m128 bta_0 = _mm_load_ps(blendJoints[n0].t.ToFloatPtr());
		__m128 btb_0 = _mm_load_ps(blendJoints[n1].t.ToFloatPtr());
		__m128 btc_0 = _mm_load_ps(blendJoints[n2].t.ToFloatPtr());
		__m128 btd_0 = _mm_load_ps(blendJoints[n3].t.ToFloatPtr());

		bta_0 = _mm_sub_ps(bta_0, jta_0);
		btb_0 = _mm_sub_ps(btb_0, jtb_0);
		btc_0 = _mm_sub_ps(btc_0, jtc_0);
		btd_0 = _mm_sub_ps(btd_0, jtd_0);

		jta_0 = _mm_madd_ps(vlerp, bta_0, jta_0);
		jtb_0 = _mm_madd_ps(vlerp, btb_0, jtb_0);
		jtc_0 = _mm_madd_ps(vlerp, btc_0, jtc_0);
		jtd_0 = _mm_madd_ps(vlerp, btd_0, jtd_0);

		_mm_store_ps(joints[n0].t.ToFloatPtr(), jta_0);
		_mm_store_ps(joints[n1].t.ToFloatPtr(), jtb_0);
		_mm_store_ps(joints[n2].t.ToFloatPtr(), jtc_0);
		_mm_store_ps(joints[n3].t.ToFloatPtr(), jtd_0);

		__m128 jqr_0 = _mm_unpacklo_ps(jqa_0, jqc_0);
		__m128 jqs_0 = _mm_unpackhi_ps(jqa_0, jqc_0);
		__m128 jqt_0 = _mm_unpacklo_ps(jqb_0, jqd_0);
		__m128 jqu_0 = _mm_unpackhi_ps(jqb_0, jqd_0);

		__m128 bqr_0 = _mm_unpacklo_ps(bqa_0, bqc_0);
		__m128 bqs_0 = _mm_unpackhi_ps(bqa_0, bqc_0);
		__m128 bqt_0 = _mm_unpacklo_ps(bqb_0, bqd_0);
		__m128 bqu_0 = _mm_unpackhi_ps(bqb_0, bqd_0);

		__m128 jqx_0 = _mm_unpacklo_ps(jqr_0, jqt_0);
		__m128 jqy_0 = _mm_unpackhi_ps(jqr_0, jqt_0);
		__m128 jqz_0 = _mm_unpacklo_ps(jqs_0, jqu_0);
		__m128 jqw_0 = _mm_unpackhi_ps(jqs_0, jqu_0);

		__m128 bqx_0 = _mm_unpacklo_ps(bqr_0, bqt_0);
		__m128 bqy_0 = _mm_unpackhi_ps(bqr_0, bqt_0);
		__m128 bqz_0 = _mm_unpacklo_ps(bqs_0, bqu_0);
		__m128 bqw_0 = _mm_unpackhi_ps(bqs_0, bqu_0);

		__m128 cosoma_0 = _mm_mul_ps(jqx_0, bqx_0);
		__m128 cosomb_0 = _mm_mul_ps(jqy_0, bqy_0);
		__m128 cosomc_0 = _mm_mul_ps(jqz_0, bqz_0);
		__m128 cosomd_0 = _mm_mul_ps(jqw_0, bqw_0);

		__m128 cosome_0 = _mm_add_ps(cosoma_0, cosomb_0);
		__m128 cosomf_0 = _mm_add_ps(cosomc_0, cosomd_0);
		__m128 cosomg_0 = _mm_add_ps(cosome_0, cosomf_0);

		__m128 sign_0 = _mm_and_ps(cosomg_0, vector_float_sign_bit);
		__m128 cosom_0 = _mm_xor_ps(cosomg_0, sign_0);
		__m128 ss_0 = SIMD_mm_nmsub_ps(cosom_0, cosom_0, vector_float_one);

		ss_0 = _mm_max_ps(ss_0, vector_float_tiny);

		__m128 rs_0 = _mm_rsqrt_ps(ss_0);
		__m128 sq_0 = _mm_mul_ps(rs_0, rs_0);
		__m128 sh_0 = _mm_mul_ps(rs_0, vector_float_rsqrt_c1);
		__m128 sx_0 = _mm_madd_ps(ss_0, sq_0, vector_float_rsqrt_c0);
		__m128 sinom_0 = _mm_mul_ps(sh_0, sx_0);						// sinom = sqrt( ss );

		ss_0 = _mm_mul_ps(ss_0, sinom_0);

		__m128 min_0 = _mm_min_ps(ss_0, cosom_0);
		__m128 max_0 = _mm_max_ps(ss_0, cosom_0);
		__m128 mask_0 = _mm_cmpeq_ps(min_0, cosom_0);
		__m128 masksign_0 = _mm_and_ps(mask_0, vector_float_sign_bit);
		__m128 maskPI_0 = _mm_and_ps(mask_0, vector_float_half_pi);

		__m128 rcpa_0 = _mm_rcp_ps(max_0);
		__m128 rcpb_0 = _mm_mul_ps(max_0, rcpa_0);
		__m128 rcpd_0 = _mm_add_ps(rcpa_0, rcpa_0);
		__m128 rcp_0 = SIMD_mm_nmsub_ps(rcpb_0, rcpa_0, rcpd_0);			// 1 / y or 1 / x
		__m128 ata_0 = _mm_mul_ps(min_0, rcp_0);						// x / y or y / x

		__m128 atb_0 = _mm_xor_ps(ata_0, masksign_0);					// -x / y or y / x
		__m128 atc_0 = _mm_mul_ps(atb_0, atb_0);
		__m128 atd_0 = _mm_madd_ps(atc_0, vector_float_atan_c0, vector_float_atan_c1);

		atd_0 = _mm_madd_ps(atd_0, atc_0, vector_float_atan_c2);
		atd_0 = _mm_madd_ps(atd_0, atc_0, vector_float_atan_c3);
		atd_0 = _mm_madd_ps(atd_0, atc_0, vector_float_atan_c4);
		atd_0 = _mm_madd_ps(atd_0, atc_0, vector_float_atan_c5);
		atd_0 = _mm_madd_ps(atd_0, atc_0, vector_float_atan_c6);
		atd_0 = _mm_madd_ps(atd_0, atc_0, vector_float_atan_c7);
		atd_0 = _mm_madd_ps(atd_0, atc_0, vector_float_one);

		__m128 omega_a_0 = _mm_madd_ps(atd_0, atb_0, maskPI_0);
		__m128 omega_b_0 = _mm_mul_ps(vlerp, omega_a_0);
		omega_a_0 = _mm_sub_ps(omega_a_0, omega_b_0);

		__m128 sinsa_0 = _mm_mul_ps(omega_a_0, omega_a_0);
		__m128 sinsb_0 = _mm_mul_ps(omega_b_0, omega_b_0);
		__m128 sina_0 = _mm_madd_ps(sinsa_0, vector_float_sin_c0, vector_float_sin_c1);
		__m128 sinb_0 = _mm_madd_ps(sinsb_0, vector_float_sin_c0, vector_float_sin_c1);
		sina_0 = _mm_madd_ps(sina_0, sinsa_0, vector_float_sin_c2);
		sinb_0 = _mm_madd_ps(sinb_0, sinsb_0, vector_float_sin_c2);
		sina_0 = _mm_madd_ps(sina_0, sinsa_0, vector_float_sin_c3);
		sinb_0 = _mm_madd_ps(sinb_0, sinsb_0, vector_float_sin_c3);
		sina_0 = _mm_madd_ps(sina_0, sinsa_0, vector_float_sin_c4);
		sinb_0 = _mm_madd_ps(sinb_0, sinsb_0, vector_float_sin_c4);
		sina_0 = _mm_madd_ps(sina_0, sinsa_0, vector_float_one);
		sinb_0 = _mm_madd_ps(sinb_0, sinsb_0, vector_float_one);
		sina_0 = _mm_mul_ps(sina_0, omega_a_0);
		sinb_0 = _mm_mul_ps(sinb_0, omega_b_0);
		__m128 scalea_0 = _mm_mul_ps(sina_0, sinom_0);
		__m128 scaleb_0 = _mm_mul_ps(sinb_0, sinom_0);

		scaleb_0 = _mm_xor_ps(scaleb_0, sign_0);

		jqx_0 = _mm_mul_ps(jqx_0, scalea_0);
		jqy_0 = _mm_mul_ps(jqy_0, scalea_0);
		jqz_0 = _mm_mul_ps(jqz_0, scalea_0);
		jqw_0 = _mm_mul_ps(jqw_0, scalea_0);

		jqx_0 = _mm_madd_ps(bqx_0, scaleb_0, jqx_0);
		jqy_0 = _mm_madd_ps(bqy_0, scaleb_0, jqy_0);
		jqz_0 = _mm_madd_ps(bqz_0, scaleb_0, jqz_0);
		jqw_0 = _mm_madd_ps(bqw_0, scaleb_0, jqw_0);

		__m128 tp0_0 = _mm_unpacklo_ps(jqx_0, jqz_0);
		__m128 tp1_0 = _mm_unpackhi_ps(jqx_0, jqz_0);
		__m128 tp2_0 = _mm_unpacklo_ps(jqy_0, jqw_0);
		__m128 tp3_0 = _mm_unpackhi_ps(jqy_0, jqw_0);

		__m128 p0_0 = _mm_unpacklo_ps(tp0_0, tp2_0);
		__m128 p1_0 = _mm_unpackhi_ps(tp0_0, tp2_0);
		__m128 p2_0 = _mm_unpacklo_ps(tp1_0, tp3_0);
		__m128 p3_0 = _mm_unpackhi_ps(tp1_0, tp3_0);

		_mm_store_ps(joints[n0].q.ToFloatPtr(), p0_0);
		_mm_store_ps(joints[n1].q.ToFloatPtr(), p1_0);
		_mm_store_ps(joints[n2].q.ToFloatPtr(), p2_0);
		_mm_store_ps(joints[n3].q.ToFloatPtr(), p3_0);
	}

	for (; i < numJoints; i++)
	{
		int n = index[i];

		idVec3& jointVert = joints[n].t;
		const idVec3& blendVert = blendJoints[n].t;

		jointVert[0] += lerp * (blendVert[0] - jointVert[0]);
		jointVert[1] += lerp * (blendVert[1] - jointVert[1]);
		jointVert[2] += lerp * (blendVert[2] - jointVert[2]);
		joints[n].w = 0.0f;

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
		signBit = (*(unsigned int*)&cosom) & (1 << 31);

		(*(unsigned int*)&cosom) ^= signBit;
		// DG end

		scale0 = 1.0f - cosom * cosom;
		scale0 = (scale0 <= 0.0f) ? 1e-10f : scale0;
		sinom = idMath::InvSqrt(scale0);
		omega = idMath::ATan16(scale0 * sinom, cosom);
		scale0 = idMath::Sin16((1.0f - lerp) * omega) * sinom;
		scale1 = idMath::Sin16(lerp * omega) * sinom;

		(*(unsigned int*)&scale1) ^= signBit; // DG: use int instead of long for 64bit compatibility

		jointQuat.x = scale0 * jointQuat.x + scale1 * blendQuat.x;
		jointQuat.y = scale0 * jointQuat.y + scale1 * blendQuat.y;
		jointQuat.z = scale0 * jointQuat.z + scale1 * blendQuat.z;
		jointQuat.w = scale0 * jointQuat.w + scale1 * blendQuat.w;
	}
}

/*
============
BlendJointsFast
============
*/
void SIMD_VPCALL idSIMDProcessor::BlendJointsFast(idJointQuat* joints, const idJointQuat* blendJoints, const float lerp, const int* index, const int numJoints)
{
	//assert_16_byte_aligned(joints);
	//assert_16_byte_aligned(blendJoints);
	//assert_16_byte_aligned(JOINTQUAT_Q_OFFSET);
	//assert_16_byte_aligned(JOINTQUAT_T_OFFSET);
	//assert_sizeof_16_byte_multiple(idJointQuat);

	if (lerp <= 0.0f)
	{
		return;
	}
	else if (lerp >= 1.0f)
	{
		for (int i = 0; i < numJoints; i++)
		{
			int j = index[i];
			joints[j] = blendJoints[j];
		}
		return;
	}

	const __m128 vector_float_sign_bit = __m128c(_mm_set_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000));
	const __m128 vector_float_rsqrt_c0 = { -3.0f,  -3.0f,  -3.0f,  -3.0f };
	const __m128 vector_float_rsqrt_c1 = { -0.5f,  -0.5f,  -0.5f,  -0.5f };

	const float scaledLerp = lerp / (1.0f - lerp);
	const __m128 vlerp = { lerp, lerp, lerp, lerp };
	const __m128 vscaledLerp = { scaledLerp, scaledLerp, scaledLerp, scaledLerp };

	int i = 0;
	for (; i < numJoints - 3; i += 4)
	{
		const int n0 = index[i + 0];
		const int n1 = index[i + 1];
		const int n2 = index[i + 2];
		const int n3 = index[i + 3];

		__m128 jqa_0 = _mm_load_ps(joints[n0].q.ToFloatPtr());
		__m128 jqb_0 = _mm_load_ps(joints[n1].q.ToFloatPtr());
		__m128 jqc_0 = _mm_load_ps(joints[n2].q.ToFloatPtr());
		__m128 jqd_0 = _mm_load_ps(joints[n3].q.ToFloatPtr());

		__m128 jta_0 = _mm_load_ps(joints[n0].t.ToFloatPtr());
		__m128 jtb_0 = _mm_load_ps(joints[n1].t.ToFloatPtr());
		__m128 jtc_0 = _mm_load_ps(joints[n2].t.ToFloatPtr());
		__m128 jtd_0 = _mm_load_ps(joints[n3].t.ToFloatPtr());

		__m128 bqa_0 = _mm_load_ps(blendJoints[n0].q.ToFloatPtr());
		__m128 bqb_0 = _mm_load_ps(blendJoints[n1].q.ToFloatPtr());
		__m128 bqc_0 = _mm_load_ps(blendJoints[n2].q.ToFloatPtr());
		__m128 bqd_0 = _mm_load_ps(blendJoints[n3].q.ToFloatPtr());

		__m128 bta_0 = _mm_load_ps(blendJoints[n0].t.ToFloatPtr());
		__m128 btb_0 = _mm_load_ps(blendJoints[n1].t.ToFloatPtr());
		__m128 btc_0 = _mm_load_ps(blendJoints[n2].t.ToFloatPtr());
		__m128 btd_0 = _mm_load_ps(blendJoints[n3].t.ToFloatPtr());

		bta_0 = _mm_sub_ps(bta_0, jta_0);
		btb_0 = _mm_sub_ps(btb_0, jtb_0);
		btc_0 = _mm_sub_ps(btc_0, jtc_0);
		btd_0 = _mm_sub_ps(btd_0, jtd_0);

		jta_0 = _mm_madd_ps(vlerp, bta_0, jta_0);
		jtb_0 = _mm_madd_ps(vlerp, btb_0, jtb_0);
		jtc_0 = _mm_madd_ps(vlerp, btc_0, jtc_0);
		jtd_0 = _mm_madd_ps(vlerp, btd_0, jtd_0);

		_mm_store_ps(joints[n0].t.ToFloatPtr(), jta_0);
		_mm_store_ps(joints[n1].t.ToFloatPtr(), jtb_0);
		_mm_store_ps(joints[n2].t.ToFloatPtr(), jtc_0);
		_mm_store_ps(joints[n3].t.ToFloatPtr(), jtd_0);

		__m128 jqr_0 = _mm_unpacklo_ps(jqa_0, jqc_0);
		__m128 jqs_0 = _mm_unpackhi_ps(jqa_0, jqc_0);
		__m128 jqt_0 = _mm_unpacklo_ps(jqb_0, jqd_0);
		__m128 jqu_0 = _mm_unpackhi_ps(jqb_0, jqd_0);

		__m128 bqr_0 = _mm_unpacklo_ps(bqa_0, bqc_0);
		__m128 bqs_0 = _mm_unpackhi_ps(bqa_0, bqc_0);
		__m128 bqt_0 = _mm_unpacklo_ps(bqb_0, bqd_0);
		__m128 bqu_0 = _mm_unpackhi_ps(bqb_0, bqd_0);

		__m128 jqx_0 = _mm_unpacklo_ps(jqr_0, jqt_0);
		__m128 jqy_0 = _mm_unpackhi_ps(jqr_0, jqt_0);
		__m128 jqz_0 = _mm_unpacklo_ps(jqs_0, jqu_0);
		__m128 jqw_0 = _mm_unpackhi_ps(jqs_0, jqu_0);

		__m128 bqx_0 = _mm_unpacklo_ps(bqr_0, bqt_0);
		__m128 bqy_0 = _mm_unpackhi_ps(bqr_0, bqt_0);
		__m128 bqz_0 = _mm_unpacklo_ps(bqs_0, bqu_0);
		__m128 bqw_0 = _mm_unpackhi_ps(bqs_0, bqu_0);

		__m128 cosoma_0 = _mm_mul_ps(jqx_0, bqx_0);
		__m128 cosomb_0 = _mm_mul_ps(jqy_0, bqy_0);
		__m128 cosomc_0 = _mm_mul_ps(jqz_0, bqz_0);
		__m128 cosomd_0 = _mm_mul_ps(jqw_0, bqw_0);

		__m128 cosome_0 = _mm_add_ps(cosoma_0, cosomb_0);
		__m128 cosomf_0 = _mm_add_ps(cosomc_0, cosomd_0);
		__m128 cosom_0 = _mm_add_ps(cosome_0, cosomf_0);

		__m128 sign_0 = _mm_and_ps(cosom_0, vector_float_sign_bit);

		__m128 scale_0 = _mm_xor_ps(vscaledLerp, sign_0);

		jqx_0 = _mm_madd_ps(scale_0, bqx_0, jqx_0);
		jqy_0 = _mm_madd_ps(scale_0, bqy_0, jqy_0);
		jqz_0 = _mm_madd_ps(scale_0, bqz_0, jqz_0);
		jqw_0 = _mm_madd_ps(scale_0, bqw_0, jqw_0);

		__m128 da_0 = _mm_mul_ps(jqx_0, jqx_0);
		__m128 db_0 = _mm_mul_ps(jqy_0, jqy_0);
		__m128 dc_0 = _mm_mul_ps(jqz_0, jqz_0);
		__m128 dd_0 = _mm_mul_ps(jqw_0, jqw_0);

		__m128 de_0 = _mm_add_ps(da_0, db_0);
		__m128 df_0 = _mm_add_ps(dc_0, dd_0);
		__m128 d_0 = _mm_add_ps(de_0, df_0);

		__m128 rs_0 = _mm_rsqrt_ps(d_0);
		__m128 sq_0 = _mm_mul_ps(rs_0, rs_0);
		__m128 sh_0 = _mm_mul_ps(rs_0, vector_float_rsqrt_c1);
		__m128 sx_0 = _mm_madd_ps(d_0, sq_0, vector_float_rsqrt_c0);
		__m128 s_0 = _mm_mul_ps(sh_0, sx_0);

		jqx_0 = _mm_mul_ps(jqx_0, s_0);
		jqy_0 = _mm_mul_ps(jqy_0, s_0);
		jqz_0 = _mm_mul_ps(jqz_0, s_0);
		jqw_0 = _mm_mul_ps(jqw_0, s_0);

		__m128 tp0_0 = _mm_unpacklo_ps(jqx_0, jqz_0);
		__m128 tp1_0 = _mm_unpackhi_ps(jqx_0, jqz_0);
		__m128 tp2_0 = _mm_unpacklo_ps(jqy_0, jqw_0);
		__m128 tp3_0 = _mm_unpackhi_ps(jqy_0, jqw_0);

		__m128 p0_0 = _mm_unpacklo_ps(tp0_0, tp2_0);
		__m128 p1_0 = _mm_unpackhi_ps(tp0_0, tp2_0);
		__m128 p2_0 = _mm_unpacklo_ps(tp1_0, tp3_0);
		__m128 p3_0 = _mm_unpackhi_ps(tp1_0, tp3_0);

		_mm_store_ps(joints[n0].q.ToFloatPtr(), p0_0);
		_mm_store_ps(joints[n1].q.ToFloatPtr(), p1_0);
		_mm_store_ps(joints[n2].q.ToFloatPtr(), p2_0);
		_mm_store_ps(joints[n3].q.ToFloatPtr(), p3_0);
	}

	for (; i < numJoints; i++)
	{
		const int n = index[i];

		idVec3& jointVert = joints[n].t;
		const idVec3& blendVert = blendJoints[n].t;

		jointVert[0] += lerp * (blendVert[0] - jointVert[0]);
		jointVert[1] += lerp * (blendVert[1] - jointVert[1]);
		jointVert[2] += lerp * (blendVert[2] - jointVert[2]);

		idQuat& jointQuat = joints[n].q;
		const idQuat& blendQuat = blendJoints[n].q;

		float cosom;
		float scale;
		float s;

		cosom = jointQuat.x * blendQuat.x + jointQuat.y * blendQuat.y + jointQuat.z * blendQuat.z + jointQuat.w * blendQuat.w;

		scale = SIMD_fsels(cosom, scaledLerp, -scaledLerp);

		jointQuat.x += scale * blendQuat.x;
		jointQuat.y += scale * blendQuat.y;
		jointQuat.z += scale * blendQuat.z;
		jointQuat.w += scale * blendQuat.w;

		s = jointQuat.x * jointQuat.x + jointQuat.y * jointQuat.y + jointQuat.z * jointQuat.z + jointQuat.w * jointQuat.w;
		s = SIMD_frsqrts(s);

		jointQuat.x *= s;
		jointQuat.y *= s;
		jointQuat.z *= s;
		jointQuat.w *= s;
	}
}

/*
============
ConvertJointQuatsToJointMats
============
*/
void SIMD_VPCALL idSIMDProcessor::ConvertJointQuatsToJointMats(idJointMat* jointMats, const idJointQuat* jointQuats, const int numJoints)
{
	assert(sizeof(idJointQuat) == JOINTQUAT_SIZE);
	assert(sizeof(idJointMat) == JOINTMAT_SIZE);

	// RB: changed int to intptr_t
	assert((intptr_t)(&((idJointQuat*)0)->t) == (intptr_t)(&((idJointQuat*)0)->q) + (intptr_t)sizeof(((idJointQuat*)0)->q));
	// RB end

	const float* jointQuatPtr = (float*)jointQuats;
	float* jointMatPtr = (float*)jointMats;

	const __m128 vector_float_first_sign_bit = __m128c(_mm_set_epi32(0x00000000, 0x00000000, 0x00000000, 0x80000000));
	const __m128 vector_float_last_three_sign_bits = __m128c(_mm_set_epi32(0x80000000, 0x80000000, 0x80000000, 0x00000000));
	const __m128 vector_float_first_pos_half = { 0.5f,   0.0f,   0.0f,   0.0f };	// +.5 0 0 0
	const __m128 vector_float_first_neg_half = { -0.5f,   0.0f,   0.0f,   0.0f };	// -.5 0 0 0
	const __m128 vector_float_quat2mat_mad1 = { -1.0f,  -1.0f,  +1.0f,  -1.0f };	//  - - + -
	const __m128 vector_float_quat2mat_mad2 = { -1.0f,  +1.0f,  -1.0f,  -1.0f };	//  - + - -
	const __m128 vector_float_quat2mat_mad3 = { +1.0f,  -1.0f,  -1.0f,  +1.0f };	//  + - - +

	int i = 0;
	for (; i + 1 < numJoints; i += 2)
	{

		__m128 q0 = _mm_load_ps(&jointQuatPtr[i * 8 + 0 * 8 + 0]);
		__m128 q1 = _mm_load_ps(&jointQuatPtr[i * 8 + 1 * 8 + 0]);

		__m128 t0 = _mm_load_ps(&jointQuatPtr[i * 8 + 0 * 8 + 4]);
		__m128 t1 = _mm_load_ps(&jointQuatPtr[i * 8 + 1 * 8 + 4]);

		__m128 d0 = _mm_add_ps(q0, q0);
		__m128 d1 = _mm_add_ps(q1, q1);

		__m128 sa0 = _mm_perm_ps(q0, _MM_SHUFFLE(1, 0, 0, 1));							//   y,   x,   x,   y
		__m128 sb0 = _mm_perm_ps(d0, _MM_SHUFFLE(2, 2, 1, 1));							//  y2,  y2,  z2,  z2
		__m128 sc0 = _mm_perm_ps(q0, _MM_SHUFFLE(3, 3, 3, 2));							//   z,   w,   w,   w
		__m128 sd0 = _mm_perm_ps(d0, _MM_SHUFFLE(0, 1, 2, 2));							//  z2,  z2,  y2,  x2
		__m128 sa1 = _mm_perm_ps(q1, _MM_SHUFFLE(1, 0, 0, 1));							//   y,   x,   x,   y
		__m128 sb1 = _mm_perm_ps(d1, _MM_SHUFFLE(2, 2, 1, 1));							//  y2,  y2,  z2,  z2
		__m128 sc1 = _mm_perm_ps(q1, _MM_SHUFFLE(3, 3, 3, 2));							//   z,   w,   w,   w
		__m128 sd1 = _mm_perm_ps(d1, _MM_SHUFFLE(0, 1, 2, 2));							//  z2,  z2,  y2,  x2

		sa0 = _mm_xor_ps(sa0, vector_float_first_sign_bit);
		sa1 = _mm_xor_ps(sa1, vector_float_first_sign_bit);

		sc0 = _mm_xor_ps(sc0, vector_float_last_three_sign_bits);							// flip stupid inverse quaternions
		sc1 = _mm_xor_ps(sc1, vector_float_last_three_sign_bits);							// flip stupid inverse quaternions

		__m128 ma0 = _mm_add_ps(_mm_mul_ps(sa0, sb0), vector_float_first_pos_half);		//  .5 - yy2,  xy2,  xz2,  yz2		//  .5 0 0 0
		__m128 mb0 = _mm_add_ps(_mm_mul_ps(sc0, sd0), vector_float_first_neg_half);		// -.5 + zz2,  wz2,  wy2,  wx2		// -.5 0 0 0
		__m128 mc0 = _mm_sub_ps(vector_float_first_pos_half, _mm_mul_ps(q0, d0));		//  .5 - xx2, -yy2, -zz2, -ww2		//  .5 0 0 0
		__m128 ma1 = _mm_add_ps(_mm_mul_ps(sa1, sb1), vector_float_first_pos_half);		//  .5 - yy2,  xy2,  xz2,  yz2		//  .5 0 0 0
		__m128 mb1 = _mm_add_ps(_mm_mul_ps(sc1, sd1), vector_float_first_neg_half);		// -.5 + zz2,  wz2,  wy2,  wx2		// -.5 0 0 0
		__m128 mc1 = _mm_sub_ps(vector_float_first_pos_half, _mm_mul_ps(q1, d1));		//  .5 - xx2, -yy2, -zz2, -ww2		//  .5 0 0 0

		__m128 mf0 = _mm_shuffle_ps(ma0, mc0, _MM_SHUFFLE(0, 0, 1, 1));					//       xy2,  xy2, .5 - xx2, .5 - xx2	// 01, 01, 10, 10
		__m128 md0 = _mm_shuffle_ps(mf0, ma0, _MM_SHUFFLE(3, 2, 0, 2));					//  .5 - xx2,  xy2,  xz2,  yz2			// 10, 01, 02, 03
		__m128 me0 = _mm_shuffle_ps(ma0, mb0, _MM_SHUFFLE(3, 2, 1, 0));					//  .5 - yy2,  xy2,  wy2,  wx2			// 00, 01, 12, 13
		__m128 mf1 = _mm_shuffle_ps(ma1, mc1, _MM_SHUFFLE(0, 0, 1, 1));					//       xy2,  xy2, .5 - xx2, .5 - xx2	// 01, 01, 10, 10
		__m128 md1 = _mm_shuffle_ps(mf1, ma1, _MM_SHUFFLE(3, 2, 0, 2));					//  .5 - xx2,  xy2,  xz2,  yz2			// 10, 01, 02, 03
		__m128 me1 = _mm_shuffle_ps(ma1, mb1, _MM_SHUFFLE(3, 2, 1, 0));					//  .5 - yy2,  xy2,  wy2,  wx2			// 00, 01, 12, 13

		__m128 ra0 = _mm_add_ps(_mm_mul_ps(mb0, vector_float_quat2mat_mad1), ma0);		// 1 - yy2 - zz2, xy2 - wz2, xz2 + wy2,					// - - + -
		__m128 rb0 = _mm_add_ps(_mm_mul_ps(mb0, vector_float_quat2mat_mad2), md0);		// 1 - xx2 - zz2, xy2 + wz2,          , yz2 - wx2		// - + - -
		__m128 rc0 = _mm_add_ps(_mm_mul_ps(me0, vector_float_quat2mat_mad3), md0);		// 1 - xx2 - yy2,          , xz2 - wy2, yz2 + wx2		// + - - +
		__m128 ra1 = _mm_add_ps(_mm_mul_ps(mb1, vector_float_quat2mat_mad1), ma1);		// 1 - yy2 - zz2, xy2 - wz2, xz2 + wy2,					// - - + -
		__m128 rb1 = _mm_add_ps(_mm_mul_ps(mb1, vector_float_quat2mat_mad2), md1);		// 1 - xx2 - zz2, xy2 + wz2,          , yz2 - wx2		// - + - -
		__m128 rc1 = _mm_add_ps(_mm_mul_ps(me1, vector_float_quat2mat_mad3), md1);		// 1 - xx2 - yy2,          , xz2 - wy2, yz2 + wx2		// + - - +

		__m128 ta0 = _mm_shuffle_ps(ra0, t0, _MM_SHUFFLE(0, 0, 2, 2));
		__m128 tb0 = _mm_shuffle_ps(rb0, t0, _MM_SHUFFLE(1, 1, 3, 3));
		__m128 tc0 = _mm_shuffle_ps(rc0, t0, _MM_SHUFFLE(2, 2, 0, 0));
		__m128 ta1 = _mm_shuffle_ps(ra1, t1, _MM_SHUFFLE(0, 0, 2, 2));
		__m128 tb1 = _mm_shuffle_ps(rb1, t1, _MM_SHUFFLE(1, 1, 3, 3));
		__m128 tc1 = _mm_shuffle_ps(rc1, t1, _MM_SHUFFLE(2, 2, 0, 0));

		ra0 = _mm_shuffle_ps(ra0, ta0, _MM_SHUFFLE(2, 0, 1, 0));						// 00 01 02 10
		rb0 = _mm_shuffle_ps(rb0, tb0, _MM_SHUFFLE(2, 0, 0, 1));						// 01 00 03 11
		rc0 = _mm_shuffle_ps(rc0, tc0, _MM_SHUFFLE(2, 0, 3, 2));						// 02 03 00 12
		ra1 = _mm_shuffle_ps(ra1, ta1, _MM_SHUFFLE(2, 0, 1, 0));						// 00 01 02 10
		rb1 = _mm_shuffle_ps(rb1, tb1, _MM_SHUFFLE(2, 0, 0, 1));						// 01 00 03 11
		rc1 = _mm_shuffle_ps(rc1, tc1, _MM_SHUFFLE(2, 0, 3, 2));						// 02 03 00 12

		_mm_store_ps(&jointMatPtr[i * 12 + 0 * 12 + 0], ra0);
		_mm_store_ps(&jointMatPtr[i * 12 + 0 * 12 + 4], rb0);
		_mm_store_ps(&jointMatPtr[i * 12 + 0 * 12 + 8], rc0);
		_mm_store_ps(&jointMatPtr[i * 12 + 1 * 12 + 0], ra1);
		_mm_store_ps(&jointMatPtr[i * 12 + 1 * 12 + 4], rb1);
		_mm_store_ps(&jointMatPtr[i * 12 + 1 * 12 + 8], rc1);
	}

	for (; i < numJoints; i++)
	{

		__m128 q0 = _mm_load_ps(&jointQuatPtr[i * 8 + 0 * 8 + 0]);
		__m128 t0 = _mm_load_ps(&jointQuatPtr[i * 8 + 0 * 8 + 4]);

		__m128 d0 = _mm_add_ps(q0, q0);

		__m128 sa0 = _mm_perm_ps(q0, _MM_SHUFFLE(1, 0, 0, 1));							//   y,   x,   x,   y
		__m128 sb0 = _mm_perm_ps(d0, _MM_SHUFFLE(2, 2, 1, 1));							//  y2,  y2,  z2,  z2
		__m128 sc0 = _mm_perm_ps(q0, _MM_SHUFFLE(3, 3, 3, 2));							//   z,   w,   w,   w
		__m128 sd0 = _mm_perm_ps(d0, _MM_SHUFFLE(0, 1, 2, 2));							//  z2,  z2,  y2,  x2

		sa0 = _mm_xor_ps(sa0, vector_float_first_sign_bit);
		sc0 = _mm_xor_ps(sc0, vector_float_last_three_sign_bits);							// flip stupid inverse quaternions

		__m128 ma0 = _mm_add_ps(_mm_mul_ps(sa0, sb0), vector_float_first_pos_half);		//  .5 - yy2,  xy2,  xz2,  yz2		//  .5 0 0 0
		__m128 mb0 = _mm_add_ps(_mm_mul_ps(sc0, sd0), vector_float_first_neg_half);		// -.5 + zz2,  wz2,  wy2,  wx2		// -.5 0 0 0
		__m128 mc0 = _mm_sub_ps(vector_float_first_pos_half, _mm_mul_ps(q0, d0));		//  .5 - xx2, -yy2, -zz2, -ww2		//  .5 0 0 0

		__m128 mf0 = _mm_shuffle_ps(ma0, mc0, _MM_SHUFFLE(0, 0, 1, 1));					//       xy2,  xy2, .5 - xx2, .5 - xx2	// 01, 01, 10, 10
		__m128 md0 = _mm_shuffle_ps(mf0, ma0, _MM_SHUFFLE(3, 2, 0, 2));					//  .5 - xx2,  xy2,  xz2,  yz2			// 10, 01, 02, 03
		__m128 me0 = _mm_shuffle_ps(ma0, mb0, _MM_SHUFFLE(3, 2, 1, 0));					//  .5 - yy2,  xy2,  wy2,  wx2			// 00, 01, 12, 13

		__m128 ra0 = _mm_add_ps(_mm_mul_ps(mb0, vector_float_quat2mat_mad1), ma0);		// 1 - yy2 - zz2, xy2 - wz2, xz2 + wy2,					// - - + -
		__m128 rb0 = _mm_add_ps(_mm_mul_ps(mb0, vector_float_quat2mat_mad2), md0);		// 1 - xx2 - zz2, xy2 + wz2,          , yz2 - wx2		// - + - -
		__m128 rc0 = _mm_add_ps(_mm_mul_ps(me0, vector_float_quat2mat_mad3), md0);		// 1 - xx2 - yy2,          , xz2 - wy2, yz2 + wx2		// + - - +

		__m128 ta0 = _mm_shuffle_ps(ra0, t0, _MM_SHUFFLE(0, 0, 2, 2));
		__m128 tb0 = _mm_shuffle_ps(rb0, t0, _MM_SHUFFLE(1, 1, 3, 3));
		__m128 tc0 = _mm_shuffle_ps(rc0, t0, _MM_SHUFFLE(2, 2, 0, 0));

		ra0 = _mm_shuffle_ps(ra0, ta0, _MM_SHUFFLE(2, 0, 1, 0));						// 00 01 02 10
		rb0 = _mm_shuffle_ps(rb0, tb0, _MM_SHUFFLE(2, 0, 0, 1));						// 01 00 03 11
		rc0 = _mm_shuffle_ps(rc0, tc0, _MM_SHUFFLE(2, 0, 3, 2));						// 02 03 00 12

		_mm_store_ps(&jointMatPtr[i * 12 + 0 * 12 + 0], ra0);
		_mm_store_ps(&jointMatPtr[i * 12 + 0 * 12 + 4], rb0);
		_mm_store_ps(&jointMatPtr[i * 12 + 0 * 12 + 8], rc0);
	}
}

/*
============
ConvertJointMatsToJointQuats
============
*/
void SIMD_VPCALL idSIMDProcessor::ConvertJointMatsToJointQuats(idJointQuat* jointQuats, const idJointMat* jointMats, const int numJoints)
{

	assert(sizeof(idJointQuat) == JOINTQUAT_SIZE);
	assert(sizeof(idJointMat) == JOINTMAT_SIZE);

	// RB: changed int to intptr_t
	assert((intptr_t)(&((idJointQuat*)0)->t) == (intptr_t)(&((idJointQuat*)0)->q) + (intptr_t)sizeof(((idJointQuat*)0)->q));
	// RB end

	const __m128 vector_float_zero = _mm_setzero_ps();
	const __m128 vector_float_one = { 1.0f, 1.0f, 1.0f, 1.0f };
	const __m128 vector_float_not = __m128c(_mm_set_epi32(-1, -1, -1, -1));
	const __m128 vector_float_sign_bit = __m128c(_mm_set_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000));
	const __m128 vector_float_rsqrt_c0 = { -3.0f,  -3.0f,  -3.0f,  -3.0f };
	const __m128 vector_float_rsqrt_c2 = { -0.25f, -0.25f, -0.25f, -0.25f };

	int i = 0;
	for (; i < numJoints - 3; i += 4)
	{
		const float* __restrict m = (float*)&jointMats[i];
		float* __restrict q = (float*)&jointQuats[i];

		__m128 ma0 = _mm_load_ps(&m[0 * 12 + 0]);
		__m128 ma1 = _mm_load_ps(&m[0 * 12 + 4]);
		__m128 ma2 = _mm_load_ps(&m[0 * 12 + 8]);

		__m128 mb0 = _mm_load_ps(&m[1 * 12 + 0]);
		__m128 mb1 = _mm_load_ps(&m[1 * 12 + 4]);
		__m128 mb2 = _mm_load_ps(&m[1 * 12 + 8]);

		__m128 mc0 = _mm_load_ps(&m[2 * 12 + 0]);
		__m128 mc1 = _mm_load_ps(&m[2 * 12 + 4]);
		__m128 mc2 = _mm_load_ps(&m[2 * 12 + 8]);

		__m128 md0 = _mm_load_ps(&m[3 * 12 + 0]);
		__m128 md1 = _mm_load_ps(&m[3 * 12 + 4]);
		__m128 md2 = _mm_load_ps(&m[3 * 12 + 8]);

		__m128 ta0 = _mm_unpacklo_ps(ma0, mc0);	// a0, c0, a1, c1
		__m128 ta1 = _mm_unpackhi_ps(ma0, mc0);	// a2, c2, a3, c3
		__m128 ta2 = _mm_unpacklo_ps(mb0, md0);	// b0, d0, b1, b2
		__m128 ta3 = _mm_unpackhi_ps(mb0, md0);	// b2, d2, b3, d3

		__m128 tb0 = _mm_unpacklo_ps(ma1, mc1);	// a0, c0, a1, c1
		__m128 tb1 = _mm_unpackhi_ps(ma1, mc1);	// a2, c2, a3, c3
		__m128 tb2 = _mm_unpacklo_ps(mb1, md1);	// b0, d0, b1, b2
		__m128 tb3 = _mm_unpackhi_ps(mb1, md1);	// b2, d2, b3, d3

		__m128 tc0 = _mm_unpacklo_ps(ma2, mc2);	// a0, c0, a1, c1
		__m128 tc1 = _mm_unpackhi_ps(ma2, mc2);	// a2, c2, a3, c3
		__m128 tc2 = _mm_unpacklo_ps(mb2, md2);	// b0, d0, b1, b2
		__m128 tc3 = _mm_unpackhi_ps(mb2, md2);	// b2, d2, b3, d3

		__m128 m00 = _mm_unpacklo_ps(ta0, ta2);
		__m128 m01 = _mm_unpackhi_ps(ta0, ta2);
		__m128 m02 = _mm_unpacklo_ps(ta1, ta3);
		__m128 m03 = _mm_unpackhi_ps(ta1, ta3);

		__m128 m10 = _mm_unpacklo_ps(tb0, tb2);
		__m128 m11 = _mm_unpackhi_ps(tb0, tb2);
		__m128 m12 = _mm_unpacklo_ps(tb1, tb3);
		__m128 m13 = _mm_unpackhi_ps(tb1, tb3);

		__m128 m20 = _mm_unpacklo_ps(tc0, tc2);
		__m128 m21 = _mm_unpackhi_ps(tc0, tc2);
		__m128 m22 = _mm_unpacklo_ps(tc1, tc3);
		__m128 m23 = _mm_unpackhi_ps(tc1, tc3);

		__m128 b00 = _mm_add_ps(m00, m11);
		__m128 b11 = _mm_cmpgt_ps(m00, m22);
		__m128 b01 = _mm_add_ps(b00, m22);
		__m128 b10 = _mm_cmpgt_ps(m00, m11);
		__m128 b0 = _mm_cmpgt_ps(b01, vector_float_zero);
		__m128 b1 = _mm_and_ps(b10, b11);
		__m128 b2 = _mm_cmpgt_ps(m11, m22);

		__m128 m0 = b0;
		__m128 m1 = _mm_and_ps(_mm_xor_ps(b0, vector_float_not), b1);
		__m128 p1 = _mm_or_ps(b0, b1);
		__m128 p2 = _mm_or_ps(p1, b2);
		__m128 m2 = _mm_and_ps(_mm_xor_ps(p1, vector_float_not), b2);
		__m128 m3 = _mm_xor_ps(p2, vector_float_not);

		__m128 i0 = _mm_or_ps(m2, m3);
		__m128 i1 = _mm_or_ps(m1, m3);
		__m128 i2 = _mm_or_ps(m1, m2);

		__m128 s0 = _mm_and_ps(i0, vector_float_sign_bit);
		__m128 s1 = _mm_and_ps(i1, vector_float_sign_bit);
		__m128 s2 = _mm_and_ps(i2, vector_float_sign_bit);

		m00 = _mm_xor_ps(m00, s0);
		m11 = _mm_xor_ps(m11, s1);
		m22 = _mm_xor_ps(m22, s2);
		m21 = _mm_xor_ps(m21, s0);
		m02 = _mm_xor_ps(m02, s1);
		m10 = _mm_xor_ps(m10, s2);

		__m128 t0 = _mm_add_ps(m00, m11);
		__m128 t1 = _mm_add_ps(m22, vector_float_one);
		__m128 q0 = _mm_add_ps(t0, t1);
		__m128 q1 = _mm_sub_ps(m01, m10);
		__m128 q2 = _mm_sub_ps(m20, m02);
		__m128 q3 = _mm_sub_ps(m12, m21);

		__m128 rs = _mm_rsqrt_ps(q0);
		__m128 sq = _mm_mul_ps(rs, rs);
		__m128 sh = _mm_mul_ps(rs, vector_float_rsqrt_c2);
		__m128 sx = _mm_madd_ps(q0, sq, vector_float_rsqrt_c0);
		__m128 s = _mm_mul_ps(sh, sx);

		q0 = _mm_mul_ps(q0, s);
		q1 = _mm_mul_ps(q1, s);
		q2 = _mm_mul_ps(q2, s);
		q3 = _mm_mul_ps(q3, s);

		m0 = _mm_or_ps(m0, m2);
		m2 = _mm_or_ps(m2, m3);

		__m128 fq0 = _mm_sel_ps(q0, q3, m0);
		__m128 fq1 = _mm_sel_ps(q1, q2, m0);
		__m128 fq2 = _mm_sel_ps(q2, q1, m0);
		__m128 fq3 = _mm_sel_ps(q3, q0, m0);

		__m128 rq0 = _mm_sel_ps(fq0, fq2, m2);
		__m128 rq1 = _mm_sel_ps(fq1, fq3, m2);
		__m128 rq2 = _mm_sel_ps(fq2, fq0, m2);
		__m128 rq3 = _mm_sel_ps(fq3, fq1, m2);

		__m128 tq0 = _mm_unpacklo_ps(rq0, rq2);
		__m128 tq1 = _mm_unpackhi_ps(rq0, rq2);
		__m128 tq2 = _mm_unpacklo_ps(rq1, rq3);
		__m128 tq3 = _mm_unpackhi_ps(rq1, rq3);

		__m128 sq0 = _mm_unpacklo_ps(tq0, tq2);
		__m128 sq1 = _mm_unpackhi_ps(tq0, tq2);
		__m128 sq2 = _mm_unpacklo_ps(tq1, tq3);
		__m128 sq3 = _mm_unpackhi_ps(tq1, tq3);

		__m128 tt0 = _mm_unpacklo_ps(m03, m23);
		__m128 tt1 = _mm_unpackhi_ps(m03, m23);
		__m128 tt2 = _mm_unpacklo_ps(m13, vector_float_zero);
		__m128 tt3 = _mm_unpackhi_ps(m13, vector_float_zero);

		__m128 st0 = _mm_unpacklo_ps(tt0, tt2);
		__m128 st1 = _mm_unpackhi_ps(tt0, tt2);
		__m128 st2 = _mm_unpacklo_ps(tt1, tt3);
		__m128 st3 = _mm_unpackhi_ps(tt1, tt3);

		_mm_store_ps(&q[0 * 4], sq0);
		_mm_store_ps(&q[1 * 4], st0);
		_mm_store_ps(&q[2 * 4], sq1);
		_mm_store_ps(&q[3 * 4], st1);
		_mm_store_ps(&q[4 * 4], sq2);
		_mm_store_ps(&q[5 * 4], st2);
		_mm_store_ps(&q[6 * 4], sq3);
		_mm_store_ps(&q[7 * 4], st3);
	}

	float sign[2] = { 1.0f, -1.0f };

	for (; i < numJoints; i++)
	{
		const float* __restrict m = (float*)&jointMats[i];
		float* __restrict q = (float*)&jointQuats[i];

		int b0 = m[0 * 4 + 0] + m[1 * 4 + 1] + m[2 * 4 + 2] > 0.0f;
		int b1 = m[0 * 4 + 0] > m[1 * 4 + 1] && m[0 * 4 + 0] > m[2 * 4 + 2];
		int b2 = m[1 * 4 + 1] > m[2 * 4 + 2];

		int m0 = b0;
		int m1 = (!b0) & b1;
		int m2 = (!(b0 | b1)) & b2;
		int m3 = !(b0 | b1 | b2);

		int i0 = (m2 | m3);
		int i1 = (m1 | m3);
		int i2 = (m1 | m2);

		float s0 = sign[i0];
		float s1 = sign[i1];
		float s2 = sign[i2];

		float t = s0 * m[0 * 4 + 0] + s1 * m[1 * 4 + 1] + s2 * m[2 * 4 + 2] + 1.0f;
		float s = SIMD_frsqrts(t);
		s = (t * s * s + -3.0f) * (s * -0.25f);

		q[0] = t * s;
		q[1] = (m[0 * 4 + 1] - s2 * m[1 * 4 + 0]) * s;
		q[2] = (m[2 * 4 + 0] - s1 * m[0 * 4 + 2]) * s;
		q[3] = (m[1 * 4 + 2] - s0 * m[2 * 4 + 1]) * s;

		if (m0 | m2)
		{
			// reverse
			SIMD_SWAP(q[0], q[3]);
			SIMD_SWAP(q[1], q[2]);
		}
		if (m2 | m3)
		{
			// rotate 2
			SIMD_SWAP(q[0], q[2]);
			SIMD_SWAP(q[1], q[3]);
		}

		q[4] = m[0 * 4 + 3];
		q[5] = m[1 * 4 + 3];
		q[6] = m[2 * 4 + 3];
		q[7] = 0.0f;
	}
}

/*
============
TransformJoints
============
*/
void SIMD_VPCALL idSIMDProcessor::TransformJoints(idJointMat* jointMats, const int* parents, const int firstJoint, const int lastJoint)
{
	const __m128 vector_float_mask_keep_last = __m128c(_mm_set_epi32(0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000));

	const float* __restrict firstMatrix = jointMats->ToFloatPtr() + (firstJoint + firstJoint + firstJoint - 3) * 4;

	__m128 pma = _mm_load_ps(firstMatrix + 0);
	__m128 pmb = _mm_load_ps(firstMatrix + 4);
	__m128 pmc = _mm_load_ps(firstMatrix + 8);

	for (int joint = firstJoint; joint <= lastJoint; joint++)
	{
		const int parent = parents[joint];
		const float* __restrict parentMatrix = jointMats->ToFloatPtr() + (parent + parent + parent) * 4;
		float* __restrict childMatrix = jointMats->ToFloatPtr() + (joint + joint + joint) * 4;

		if (parent != joint - 1)
		{
			pma = _mm_load_ps(parentMatrix + 0);
			pmb = _mm_load_ps(parentMatrix + 4);
			pmc = _mm_load_ps(parentMatrix + 8);
		}

		__m128 cma = _mm_load_ps(childMatrix + 0);
		__m128 cmb = _mm_load_ps(childMatrix + 4);
		__m128 cmc = _mm_load_ps(childMatrix + 8);

		__m128 ta = _mm_splat_ps(pma, 0);
		__m128 tb = _mm_splat_ps(pmb, 0);
		__m128 tc = _mm_splat_ps(pmc, 0);

		__m128 td = _mm_splat_ps(pma, 1);
		__m128 te = _mm_splat_ps(pmb, 1);
		__m128 tf = _mm_splat_ps(pmc, 1);

		__m128 tg = _mm_splat_ps(pma, 2);
		__m128 th = _mm_splat_ps(pmb, 2);
		__m128 ti = _mm_splat_ps(pmc, 2);

		pma = _mm_madd_ps(ta, cma, _mm_and_ps(pma, vector_float_mask_keep_last));
		pmb = _mm_madd_ps(tb, cma, _mm_and_ps(pmb, vector_float_mask_keep_last));
		pmc = _mm_madd_ps(tc, cma, _mm_and_ps(pmc, vector_float_mask_keep_last));

		pma = _mm_madd_ps(td, cmb, pma);
		pmb = _mm_madd_ps(te, cmb, pmb);
		pmc = _mm_madd_ps(tf, cmb, pmc);

		pma = _mm_madd_ps(tg, cmc, pma);
		pmb = _mm_madd_ps(th, cmc, pmb);
		pmc = _mm_madd_ps(ti, cmc, pmc);

		_mm_store_ps(childMatrix + 0, pma);
		_mm_store_ps(childMatrix + 4, pmb);
		_mm_store_ps(childMatrix + 8, pmc);
	}
}

/*
============
UntransformJoints
============
*/
void SIMD_VPCALL idSIMDProcessor::UntransformJoints(idJointMat* jointMats, const int* parents, const int firstJoint, const int lastJoint)
{
	const __m128 vector_float_mask_keep_last = __m128c(_mm_set_epi32(0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000));

	for (int joint = lastJoint; joint >= firstJoint; joint--)
	{
		assert(parents[joint] < joint);
		const int parent = parents[joint];
		const float* __restrict parentMatrix = jointMats->ToFloatPtr() + (parent + parent + parent) * 4;
		float* __restrict childMatrix = jointMats->ToFloatPtr() + (joint + joint + joint) * 4;

		__m128 pma = _mm_load_ps(parentMatrix + 0);
		__m128 pmb = _mm_load_ps(parentMatrix + 4);
		__m128 pmc = _mm_load_ps(parentMatrix + 8);

		__m128 cma = _mm_load_ps(childMatrix + 0);
		__m128 cmb = _mm_load_ps(childMatrix + 4);
		__m128 cmc = _mm_load_ps(childMatrix + 8);

		__m128 ta = _mm_splat_ps(pma, 0);
		__m128 tb = _mm_splat_ps(pma, 1);
		__m128 tc = _mm_splat_ps(pma, 2);

		__m128 td = _mm_splat_ps(pmb, 0);
		__m128 te = _mm_splat_ps(pmb, 1);
		__m128 tf = _mm_splat_ps(pmb, 2);

		__m128 tg = _mm_splat_ps(pmc, 0);
		__m128 th = _mm_splat_ps(pmc, 1);
		__m128 ti = _mm_splat_ps(pmc, 2);

		cma = _mm_sub_ps(cma, _mm_and_ps(pma, vector_float_mask_keep_last));
		cmb = _mm_sub_ps(cmb, _mm_and_ps(pmb, vector_float_mask_keep_last));
		cmc = _mm_sub_ps(cmc, _mm_and_ps(pmc, vector_float_mask_keep_last));

		pma = _mm_mul_ps(ta, cma);
		pmb = _mm_mul_ps(tb, cma);
		pmc = _mm_mul_ps(tc, cma);

		pma = _mm_madd_ps(td, cmb, pma);
		pmb = _mm_madd_ps(te, cmb, pmb);
		pmc = _mm_madd_ps(tf, cmb, pmc);

		pma = _mm_madd_ps(tg, cmc, pma);
		pmb = _mm_madd_ps(th, cmc, pmb);
		pmc = _mm_madd_ps(ti, cmc, pmc);

		_mm_store_ps(childMatrix + 0, pma);
		_mm_store_ps(childMatrix + 4, pmb);
		_mm_store_ps(childMatrix + 8, pmc);
	}
}
#endif

#endif // end ACTUAL SIMD


/*
============
Add

dst[i] = constant + src[i];
============
*/
inline void SIMD_VPCALL idSIMDProcessor::Add(float *dst, const float constant, const float *src, const int count) {
#define OPER(X) dst[(X)] = src[(X)] + constant;
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
Add

dst[i] = src0[i] + src1[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::Add(float *dst, const float *src0, const float *src1, const int count) {
#define OPER(X) dst[(X)] = src0[(X)] + src1[(X)];
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
Sub

dst[i] = constant - src[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::Sub(float *dst, const float constant, const float *src, const int count) {
	double c = constant;
#define OPER(X) dst[(X)] = c - src[(X)];
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
Sub

dst[i] = src0[i] - src1[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::Sub(float *dst, const float *src0, const float *src1, const int count) {
#define OPER(X) dst[(X)] = src0[(X)] - src1[(X)];
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
Mul

dst[i] = constant * src[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::Mul(float *dst, const float constant, const float *src0, const int count) {
	double c = constant;
#define OPER(X) (dst[(X)] = (c * src0[(X)]))
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
Mul

dst[i] = src0[i] * src1[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::Mul(float *dst, const float *src0, const float *src1, const int count) {
#define OPER(X) (dst[(X)] = src0[(X)] * src1[(X)])
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
Div

dst[i] = constant / divisor[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::Div(float *dst, const float constant, const float *divisor, const int count) {
	double c = constant;
#define OPER(X) (dst[(X)] = (c / divisor[(X)]))
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
Div

dst[i] = src0[i] / src1[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::Div(float *dst, const float *src0, const float *src1, const int count) {
#define OPER(X) (dst[(X)] = src0[(X)] / src1[(X)])
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
MulAdd

dst[i] += constant * src[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::MulAdd(float *dst, const float constant, const float *src, const int count) {
	double c = constant;
#define OPER(X) (dst[(X)] += c * src[(X)])
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
MulAdd

dst[i] += src0[i] * src1[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::MulAdd(float *dst, const float *src0, const float *src1, const int count) {
#define OPER(X) (dst[(X)] += src0[(X)] * src1[(X)])
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
MulSub

dst[i] -= constant * src[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::MulSub(float *dst, const float constant, const float *src, const int count) {
	double c = constant;
#define OPER(X) (dst[(X)] -= c * src[(X)])
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
MulSub

dst[i] -= src0[i] * src1[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::MulSub(float *dst, const float *src0, const float *src1, const int count) {
#define OPER(X) (dst[(X)] -= src0[(X)] * src1[(X)])
	SIMD_UNROLL4(OPER)
#undef OPER
}



/*
============
Dot

dot = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2] + ...
============
*/
void SIMD_VPCALL idSIMDProcessor::Dot(float &dot, const float *src1, const float *src2, const int count) {
#if 1

	switch (count) {
	case 0: {
		dot = 0.0f;
		return;
	}
	case 1: {
		dot = src1[0] * src2[0];
		return;
	}
	case 2: {
		dot = src1[0] * src2[0] + src1[1] * src2[1];
		return;
	}
	case 3: {
		dot = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2];
		return;
	}
	default: {
		int i;
		double s0, s1, s2, s3;
		s0 = src1[0] * src2[0];
		s1 = src1[1] * src2[1];
		s2 = src1[2] * src2[2];
		s3 = src1[3] * src2[3];
		for (i = 4; i < count - 7; i += 8) {
			s0 += src1[i + 0] * src2[i + 0];
			s1 += src1[i + 1] * src2[i + 1];
			s2 += src1[i + 2] * src2[i + 2];
			s3 += src1[i + 3] * src2[i + 3];
			s0 += src1[i + 4] * src2[i + 4];
			s1 += src1[i + 5] * src2[i + 5];
			s2 += src1[i + 6] * src2[i + 6];
			s3 += src1[i + 7] * src2[i + 7];
		}
		switch (count - i) {
			SIMD_NODEFAULT;
		case 7: s0 += src1[i + 6] * src2[i + 6];
		case 6: s1 += src1[i + 5] * src2[i + 5];
		case 5: s2 += src1[i + 4] * src2[i + 4];
		case 4: s3 += src1[i + 3] * src2[i + 3];
		case 3: s0 += src1[i + 2] * src2[i + 2];
		case 2: s1 += src1[i + 1] * src2[i + 1];
		case 1: s2 += src1[i + 0] * src2[i + 0];
		case 0: break;
		}
		double sum;
		sum = s3;
		sum += s2;
		sum += s1;
		sum += s0;
		dot = sum;
	}
	}

#else

	dot = 0.0f;
	for (i = 0; i < count; i++) {
		dot += src1[i] * src2[i];
	}

#endif
}

/*
============
CmpGT

dst[i] = src0[i] > constant;
============
*/
void SIMD_VPCALL idSIMDProcessor::CmpGT(byte *dst, const float *src0, const float constant, const int count) {
#define OPER(X) dst[(X)] = src0[(X)] > constant;
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
CmpGT

dst[i] |= ( src0[i] > constant ) << bitNum;
============
*/
void SIMD_VPCALL idSIMDProcessor::CmpGT(byte *dst, const byte bitNum, const float *src0, const float constant, const int count) {
#define OPER(X) dst[(X)] |= ( src0[(X)] > constant ) << bitNum;
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
CmpGE

dst[i] = src0[i] >= constant;
============
*/
void SIMD_VPCALL idSIMDProcessor::CmpGE(byte *dst, const float *src0, const float constant, const int count) {
#define OPER(X) dst[(X)] = src0[(X)] >= constant;
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
CmpGE

dst[i] |= ( src0[i] >= constant ) << bitNum;
============
*/
void SIMD_VPCALL idSIMDProcessor::CmpGE(byte *dst, const byte bitNum, const float *src0, const float constant, const int count) {
#define OPER(X) dst[(X)] |= ( src0[(X)] >= constant ) << bitNum;
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
CmpLT

dst[i] = src0[i] < constant;
============
*/
void SIMD_VPCALL idSIMDProcessor::CmpLT(byte *dst, const float *src0, const float constant, const int count) {
#define OPER(X) dst[(X)] = src0[(X)] < constant;
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
CmpLE

dst[i] = src0[i] <= constant;
============
*/
void SIMD_VPCALL idSIMDProcessor::CmpLE(byte *dst, const float *src0, const float constant, const int count) {
#define OPER(X) dst[(X)] = src0[(X)] <= constant;
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
CmpLE

dst[i] |= ( src0[i] <= constant ) << bitNum;
============
*/
void SIMD_VPCALL idSIMDProcessor::CmpLE(byte *dst, const byte bitNum, const float *src0, const float constant, const int count) {
#define OPER(X) dst[(X)] |= ( src0[(X)] <= constant ) << bitNum;
	SIMD_UNROLL4(OPER)
#undef OPER
}

/*
============
MinMax
============
*/
void SIMD_VPCALL idSIMDProcessor::MinMax(float &min, float &max, const float *src, const int count) {
	min = idMath::INFINITY; max = -idMath::INFINITY;
#define OPER(X) if ( src[(X)] < min ) {min = src[(X)];} if ( src[(X)] > max ) {max = src[(X)];}
	SIMD_UNROLL1(OPER)
#undef OPER
}


/*
============
Clamp
============
*/
void SIMD_VPCALL idSIMDProcessor::Clamp(float *dst, const float *src, const float min, const float max, const int count) {
#define OPER(X) dst[(X)] = src[(X)] < min ? min : src[(X)] > max ? max : src[(X)];
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
ClampMin
============
*/
void SIMD_VPCALL idSIMDProcessor::ClampMin(float *dst, const float *src, const float min, const int count) {
#define OPER(X) dst[(X)] = src[(X)] < min ? min : src[(X)];
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
ClampMax
============
*/
void SIMD_VPCALL idSIMDProcessor::ClampMax(float *dst, const float *src, const float max, const int count) {
#define OPER(X) dst[(X)] = src[(X)] > max ? max : src[(X)];
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
Negate16
============
*/
void SIMD_VPCALL idSIMDProcessor::Negate16(float *dst, const int count) {
	unsigned int *ptr = reinterpret_cast<unsigned int *>(dst);
#define OPER(X) ptr[(X)] ^= ( 1 << 31 )		// IEEE 32 bits float sign bit
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
Copy16
============
*/
void SIMD_VPCALL idSIMDProcessor::Copy16(float *dst, const float *src, const int count) {
#define OPER(X) dst[(X)] = src[(X)]
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
Add16
============
*/
void SIMD_VPCALL idSIMDProcessor::Add16(float *dst, const float *src1, const float *src2, const int count) {
#define OPER(X) dst[(X)] = src1[(X)] + src2[(X)]
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
Sub16
============
*/
void SIMD_VPCALL idSIMDProcessor::Sub16(float *dst, const float *src1, const float *src2, const int count) {
#define OPER(X) dst[(X)] = src1[(X)] - src2[(X)]
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
Mul16
============
*/
void SIMD_VPCALL idSIMDProcessor::Mul16(float *dst, const float *src1, const float constant, const int count) {
#define OPER(X) dst[(X)] = src1[(X)] * constant
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
AddAssign16
============
*/
void SIMD_VPCALL idSIMDProcessor::AddAssign16(float *dst, const float *src, const int count) {
#define OPER(X) dst[(X)] += src[(X)]
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
SubAssign16
============
*/
void SIMD_VPCALL idSIMDProcessor::SubAssign16(float *dst, const float *src, const int count) {
#define OPER(X) dst[(X)] -= src[(X)]
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
MulAssign16
============
*/
void SIMD_VPCALL idSIMDProcessor::MulAssign16(float *dst, const float constant, const int count) {
#define OPER(X) dst[(X)] *= constant
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
UpSamplePCMTo44kHz

Duplicate samples for 44kHz output.
============
*/
void SIMD_VPCALL idSIMDProcessor::UpSamplePCMTo44kHz(float *dest, const short *src, const int numSamples, const int kHz, const int numChannels) {
	if (kHz == 11025) {
		if (numChannels == 1) {
			for (int i = 0; i < numSamples; i++) {
				dest[i * 4 + 0] = dest[i * 4 + 1] = dest[i * 4 + 2] = dest[i * 4 + 3] = (float)src[i + 0];
			}
		}
		else {
			for (int i = 0; i < numSamples; i += 2) {
				dest[i * 4 + 0] = dest[i * 4 + 2] = dest[i * 4 + 4] = dest[i * 4 + 6] = (float)src[i + 0];
				dest[i * 4 + 1] = dest[i * 4 + 3] = dest[i * 4 + 5] = dest[i * 4 + 7] = (float)src[i + 1];
			}
		}
	}
	else if (kHz == 22050) {
		if (numChannels == 1) {
			for (int i = 0; i < numSamples; i++) {
				dest[i * 2 + 0] = dest[i * 2 + 1] = (float)src[i + 0];
			}
		}
		else {
			for (int i = 0; i < numSamples; i += 2) {
				dest[i * 2 + 0] = dest[i * 2 + 2] = (float)src[i + 0];
				dest[i * 2 + 1] = dest[i * 2 + 3] = (float)src[i + 1];
			}
		}
	}
	else if (kHz == 44100) {
		for (int i = 0; i < numSamples; i++) {
			dest[i] = (float)src[i];
		}
	}
	else {
		assert(0);
	}
}

/*
============
UpSampleOGGTo44kHz

Duplicate samples for 44kHz output.
============
*/
void SIMD_VPCALL idSIMDProcessor::UpSampleOGGTo44kHz(float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels) {
	if (kHz == 11025) {
		if (numChannels == 1) {
			for (int i = 0; i < numSamples; i++) {
				dest[i * 4 + 0] = dest[i * 4 + 1] = dest[i * 4 + 2] = dest[i * 4 + 3] = ogg[0][i] * 32768.0f;
			}
		}
		else {
			for (int i = 0; i < numSamples >> 1; i++) {
				dest[i * 8 + 0] = dest[i * 8 + 2] = dest[i * 8 + 4] = dest[i * 8 + 6] = ogg[0][i] * 32768.0f;
				dest[i * 8 + 1] = dest[i * 8 + 3] = dest[i * 8 + 5] = dest[i * 8 + 7] = ogg[1][i] * 32768.0f;
			}
		}
	}
	else if (kHz == 22050) {
		if (numChannels == 1) {
			for (int i = 0; i < numSamples; i++) {
				dest[i * 2 + 0] = dest[i * 2 + 1] = ogg[0][i] * 32768.0f;
			}
		}
		else {
			for (int i = 0; i < numSamples >> 1; i++) {
				dest[i * 4 + 0] = dest[i * 4 + 2] = ogg[0][i] * 32768.0f;
				dest[i * 4 + 1] = dest[i * 4 + 3] = ogg[1][i] * 32768.0f;
			}
		}
	}
	else if (kHz == 44100) {
		if (numChannels == 1) {
			for (int i = 0; i < numSamples; i++) {
				dest[i * 1 + 0] = ogg[0][i] * 32768.0f;
			}
		}
		else {
			for (int i = 0; i < numSamples >> 1; i++) {
				dest[i * 2 + 0] = ogg[0][i] * 32768.0f;
				dest[i * 2 + 1] = ogg[1][i] * 32768.0f;
			}
		}
	}
	else {
		assert(0);
	}
}

/*
============
MixSoundTwoSpeakerMono
============
*/
void SIMD_VPCALL idSIMDProcessor::MixSoundTwoSpeakerMono(float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2]) {
	float sL = lastV[0];
	float sR = lastV[1];
	float incL = (currentV[0] - lastV[0]) / MIXBUFFER_SAMPLES;
	float incR = (currentV[1] - lastV[1]) / MIXBUFFER_SAMPLES;

	assert(numSamples == MIXBUFFER_SAMPLES);

	for (int j = 0; j < MIXBUFFER_SAMPLES; j++) {
		mixBuffer[j * 2 + 0] += samples[j] * sL;
		mixBuffer[j * 2 + 1] += samples[j] * sR;
		sL += incL;
		sR += incR;
	}
}

/*
============
MixSoundTwoSpeakerStereo
============
*/
void SIMD_VPCALL idSIMDProcessor::MixSoundTwoSpeakerStereo(float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2]) {
	float sL = lastV[0];
	float sR = lastV[1];
	float incL = (currentV[0] - lastV[0]) / MIXBUFFER_SAMPLES;
	float incR = (currentV[1] - lastV[1]) / MIXBUFFER_SAMPLES;

	assert(numSamples == MIXBUFFER_SAMPLES);

	for (int j = 0; j < MIXBUFFER_SAMPLES; j++) {
		mixBuffer[j * 2 + 0] += samples[j * 2 + 0] * sL;
		mixBuffer[j * 2 + 1] += samples[j * 2 + 1] * sR;
		sL += incL;
		sR += incR;
	}
}

/*
============
MixSoundSixSpeakerMono
============
*/
void SIMD_VPCALL idSIMDProcessor::MixSoundSixSpeakerMono(float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6]) {
	float sL0 = lastV[0];
	float sL1 = lastV[1];
	float sL2 = lastV[2];
	float sL3 = lastV[3];
	float sL4 = lastV[4];
	float sL5 = lastV[5];

	float incL0 = (currentV[0] - lastV[0]) / MIXBUFFER_SAMPLES;
	float incL1 = (currentV[1] - lastV[1]) / MIXBUFFER_SAMPLES;
	float incL2 = (currentV[2] - lastV[2]) / MIXBUFFER_SAMPLES;
	float incL3 = (currentV[3] - lastV[3]) / MIXBUFFER_SAMPLES;
	float incL4 = (currentV[4] - lastV[4]) / MIXBUFFER_SAMPLES;
	float incL5 = (currentV[5] - lastV[5]) / MIXBUFFER_SAMPLES;

	assert(numSamples == MIXBUFFER_SAMPLES);

	for (int i = 0; i < MIXBUFFER_SAMPLES; i++) {
		mixBuffer[i * 6 + 0] += samples[i] * sL0;
		mixBuffer[i * 6 + 1] += samples[i] * sL1;
		mixBuffer[i * 6 + 2] += samples[i] * sL2;
		mixBuffer[i * 6 + 3] += samples[i] * sL3;
		mixBuffer[i * 6 + 4] += samples[i] * sL4;
		mixBuffer[i * 6 + 5] += samples[i] * sL5;
		sL0 += incL0;
		sL1 += incL1;
		sL2 += incL2;
		sL3 += incL3;
		sL4 += incL4;
		sL5 += incL5;
	}
}

/*
============
MixSoundSixSpeakerStereo
============
*/
void SIMD_VPCALL idSIMDProcessor::MixSoundSixSpeakerStereo(float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6]) {
	float sL0 = lastV[0];
	float sL1 = lastV[1];
	float sL2 = lastV[2];
	float sL3 = lastV[3];
	float sL4 = lastV[4];
	float sL5 = lastV[5];

	float incL0 = (currentV[0] - lastV[0]) / MIXBUFFER_SAMPLES;
	float incL1 = (currentV[1] - lastV[1]) / MIXBUFFER_SAMPLES;
	float incL2 = (currentV[2] - lastV[2]) / MIXBUFFER_SAMPLES;
	float incL3 = (currentV[3] - lastV[3]) / MIXBUFFER_SAMPLES;
	float incL4 = (currentV[4] - lastV[4]) / MIXBUFFER_SAMPLES;
	float incL5 = (currentV[5] - lastV[5]) / MIXBUFFER_SAMPLES;

	assert(numSamples == MIXBUFFER_SAMPLES);

	for (int i = 0; i < MIXBUFFER_SAMPLES; i++) {
		mixBuffer[i * 6 + 0] += samples[i * 2 + 0] * sL0;
		mixBuffer[i * 6 + 1] += samples[i * 2 + 1] * sL1;
		mixBuffer[i * 6 + 2] += samples[i * 2 + 0] * sL2;
		mixBuffer[i * 6 + 3] += samples[i * 2 + 0] * sL3;
		mixBuffer[i * 6 + 4] += samples[i * 2 + 0] * sL4;
		mixBuffer[i * 6 + 5] += samples[i * 2 + 1] * sL5;
		sL0 += incL0;
		sL1 += incL1;
		sL2 += incL2;
		sL3 += incL3;
		sL4 += incL4;
		sL5 += incL5;
	}
}

/*
============
MixedSoundToSamples
============
*/
void SIMD_VPCALL idSIMDProcessor::MixedSoundToSamples(short *samples, const float *mixBuffer, const int numSamples) {

#if 1
	for (int i = 0; i < numSamples; i++) {
		if (mixBuffer[i] <= -32768.0f) {
			samples[i] = -32768;
		}
		else if (mixBuffer[i] >= 32767.0f) {
			samples[i] = 32767;
		}
		else {
			samples[i] = (short)mixBuffer[i];
		}
	}

#else

	assert((numSamples % MIXBUFFER_SAMPLES) == 0);

	__asm {

		mov			eax, numSamples
		mov			edi, mixBuffer
		mov			esi, samples
		shl			eax, 2
		add			edi, eax
		neg			eax

		loop16 :

		movaps		xmm0, [edi + eax + 0 * 16]
			movaps		xmm1, [edi + eax + 1 * 16]
			movaps		xmm2, [edi + eax + 2 * 16]
			movaps		xmm3, [edi + eax + 3 * 16]

			add			esi, 4 * 4 * 2

			cvtps2dq	xmm4, xmm0
			cvtps2dq	xmm5, xmm1
			cvtps2dq	xmm6, xmm2
			cvtps2dq	xmm7, xmm3

			prefetchnta[edi + eax + 128]

			packssdw	xmm4, xmm5
			packssdw	xmm6, xmm7

			add			eax, 4 * 16

			movlps[esi - 4 * 4 * 2], xmm4		// FIXME: should not use movlps/movhps to move integer data
			movhps[esi - 3 * 4 * 2], xmm4
			movlps[esi - 2 * 4 * 2], xmm6
			movhps[esi - 1 * 4 * 2], xmm6

			jl			loop16
	}
#endif
}



/*
============
Dot

dst[i] = constant * src[i];
============
*/
void SIMD_VPCALL idSIMDProcessor::Dot(float *dst, const idVec3 &constant, const idVec3 *src, const int count) {
#define OPER(X) dst[(X)] = constant * src[(X)];
	SIMD_UNROLL1(OPER)
#undef OPER
}


/*
============
Dot

dst[i] = constant * src[i].xyz;
============
*/
void SIMD_VPCALL idSIMDProcessor::Dot(float *dst, const idVec3 &constant, const idDrawVert *src, const int count) {
#define OPER(X) dst[(X)] = constant * src[(X)].xyz;
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
Dot

dst[i] = constant.Normal() * src[i] + constant[3];
============
*/
void SIMD_VPCALL idSIMDProcessor::Dot(float *dst, const idPlane &constant, const idVec3 *src, const int count) {
#define OPER(X) dst[(X)] = constant.Normal() * src[(X)] + constant[3];
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
Dot

dst[i] = constant.Normal() * src[i].Normal() + constant[3] * src[i][3];
============
*/
void SIMD_VPCALL idSIMDProcessor::Dot(float *dst, const idPlane &constant, const idPlane *src, const int count) {
#define OPER(X) dst[(X)] = constant.Normal() * src[(X)].Normal() + constant[3] * src[(X)][3];
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
MinMax
============
*/
void SIMD_VPCALL idSIMDProcessor::MinMax(idVec2 &min, idVec2 &max, const idVec2 *src, const int count) {
	min[0] = min[1] = idMath::INFINITY; max[0] = max[1] = -idMath::INFINITY;
#define OPER(X) const idVec2 &v = src[(X)]; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; }
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
MinMax
============
*/
void SIMD_VPCALL idSIMDProcessor::MinMax(idVec3 &min, idVec3 &max, const idVec3 *src, const int count) {
	min[0] = min[1] = min[2] = idMath::INFINITY; max[0] = max[1] = max[2] = -idMath::INFINITY;
#define OPER(X) const idVec3 &v = src[(X)]; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	SIMD_UNROLL1(OPER)
#undef OPER
}

/*
============
MatX_MultiplyVecX
============
*/
void SIMD_VPCALL idSIMDProcessor::MatX_MultiplyVecX(idVecX &dst, const idMatX &mat, const idVecX &vec) {
	int i, j, numRows;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert(vec.GetSize() >= mat.GetNumColumns());
	assert(dst.GetSize() >= mat.GetNumRows());

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numRows = mat.GetNumRows();
	switch (mat.GetNumColumns()) {
	case 1:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] = mPtr[0] * vPtr[0];
			mPtr++;
		}
		break;
	case 2:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] = mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1];
			mPtr += 2;
		}
		break;
	case 3:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] = mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2];
			mPtr += 3;
		}
		break;
	case 4:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] = mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
				mPtr[3] * vPtr[3];
			mPtr += 4;
		}
		break;
	case 5:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] = mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
				mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4];
			mPtr += 5;
		}
		break;
	case 6:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] = mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
				mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4] + mPtr[5] * vPtr[5];
			mPtr += 6;
		}
		break;
	default:
		int numColumns = mat.GetNumColumns();
		for (i = 0; i < numRows; i++) {
			float sum = mPtr[0] * vPtr[0];
			for (j = 1; j < numColumns; j++) {
				sum += mPtr[j] * vPtr[j];
			}
			dstPtr[i] = sum;
			mPtr += numColumns;
		}
		break;
	}
}

/*
============
MatX_MultiplyAddVecX
============
*/
void SIMD_VPCALL idSIMDProcessor::MatX_MultiplyAddVecX(idVecX &dst, const idMatX &mat, const idVecX &vec) {
	int i, j, numRows;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert(vec.GetSize() >= mat.GetNumColumns());
	assert(dst.GetSize() >= mat.GetNumRows());

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numRows = mat.GetNumRows();
	switch (mat.GetNumColumns()) {
	case 1:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] += mPtr[0] * vPtr[0];
			mPtr++;
		}
		break;
	case 2:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] += mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1];
			mPtr += 2;
		}
		break;
	case 3:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] += mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2];
			mPtr += 3;
		}
		break;
	case 4:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] += mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
				mPtr[3] * vPtr[3];
			mPtr += 4;
		}
		break;
	case 5:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] += mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
				mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4];
			mPtr += 5;
		}
		break;
	case 6:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] += mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
				mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4] + mPtr[5] * vPtr[5];
			mPtr += 6;
		}
		break;
	default:
		int numColumns = mat.GetNumColumns();
		for (i = 0; i < numRows; i++) {
			float sum = mPtr[0] * vPtr[0];
			for (j = 1; j < numColumns; j++) {
				sum += mPtr[j] * vPtr[j];
			}
			dstPtr[i] += sum;
			mPtr += numColumns;
		}
		break;
	}
}

/*
============
MatX_MultiplySubVecX
============
*/
void SIMD_VPCALL idSIMDProcessor::MatX_MultiplySubVecX(idVecX &dst, const idMatX &mat, const idVecX &vec) {
	int i, j, numRows;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert(vec.GetSize() >= mat.GetNumColumns());
	assert(dst.GetSize() >= mat.GetNumRows());

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numRows = mat.GetNumRows();
	switch (mat.GetNumColumns()) {
	case 1:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] -= mPtr[0] * vPtr[0];
			mPtr++;
		}
		break;
	case 2:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] -= mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1];
			mPtr += 2;
		}
		break;
	case 3:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] -= mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2];
			mPtr += 3;
		}
		break;
	case 4:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] -= mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
				mPtr[3] * vPtr[3];
			mPtr += 4;
		}
		break;
	case 5:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] -= mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
				mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4];
			mPtr += 5;
		}
		break;
	case 6:
		for (i = 0; i < numRows; i++) {
			dstPtr[i] -= mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
				mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4] + mPtr[5] * vPtr[5];
			mPtr += 6;
		}
		break;
	default:
		int numColumns = mat.GetNumColumns();
		for (i = 0; i < numRows; i++) {
			float sum = mPtr[0] * vPtr[0];
			for (j = 1; j < numColumns; j++) {
				sum += mPtr[j] * vPtr[j];
			}
			dstPtr[i] -= sum;
			mPtr += numColumns;
		}
		break;
	}
}

/*
============
MatX_TransposeMultiplyVecX
============
*/
void SIMD_VPCALL idSIMDProcessor::MatX_TransposeMultiplyVecX(idVecX &dst, const idMatX &mat, const idVecX &vec) {
	int i, j, numColumns;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert(vec.GetSize() >= mat.GetNumRows());
	assert(dst.GetSize() >= mat.GetNumColumns());

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numColumns = mat.GetNumColumns();
	switch (mat.GetNumRows()) {
	case 1:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] = *(mPtr)* vPtr[0];
			mPtr++;
		}
		break;
	case 2:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] = *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1];
			mPtr++;
		}
		break;
	case 3:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] = *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2];
			mPtr++;
		}
		break;
	case 4:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] = *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2] +
				*(mPtr + 3 * numColumns) * vPtr[3];
			mPtr++;
		}
		break;
	case 5:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] = *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2] +
				*(mPtr + 3 * numColumns) * vPtr[3] + *(mPtr + 4 * numColumns) * vPtr[4];
			mPtr++;
		}
		break;
	case 6:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] = *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2] +
				*(mPtr + 3 * numColumns) * vPtr[3] + *(mPtr + 4 * numColumns) * vPtr[4] + *(mPtr + 5 * numColumns) * vPtr[5];
			mPtr++;
		}
		break;
	default:
		int numRows = mat.GetNumRows();
		for (i = 0; i < numColumns; i++) {
			mPtr = mat.ToFloatPtr() + i;
			float sum = mPtr[0] * vPtr[0];
			for (j = 1; j < numRows; j++) {
				mPtr += numColumns;
				sum += mPtr[0] * vPtr[j];
			}
			dstPtr[i] = sum;
		}
		break;
	}
}

/*
============
MatX_TransposeMultiplyAddVecX
============
*/
void SIMD_VPCALL idSIMDProcessor::MatX_TransposeMultiplyAddVecX(idVecX &dst, const idMatX &mat, const idVecX &vec) {
	int i, j, numColumns;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert(vec.GetSize() >= mat.GetNumRows());
	assert(dst.GetSize() >= mat.GetNumColumns());

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numColumns = mat.GetNumColumns();
	switch (mat.GetNumRows()) {
	case 1:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] += *(mPtr)* vPtr[0];
			mPtr++;
		}
		break;
	case 2:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] += *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1];
			mPtr++;
		}
		break;
	case 3:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] += *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2];
			mPtr++;
		}
		break;
	case 4:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] += *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2] +
				*(mPtr + 3 * numColumns) * vPtr[3];
			mPtr++;
		}
		break;
	case 5:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] += *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2] +
				*(mPtr + 3 * numColumns) * vPtr[3] + *(mPtr + 4 * numColumns) * vPtr[4];
			mPtr++;
		}
		break;
	case 6:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] += *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2] +
				*(mPtr + 3 * numColumns) * vPtr[3] + *(mPtr + 4 * numColumns) * vPtr[4] + *(mPtr + 5 * numColumns) * vPtr[5];
			mPtr++;
		}
		break;
	default:
		int numRows = mat.GetNumRows();
		for (i = 0; i < numColumns; i++) {
			mPtr = mat.ToFloatPtr() + i;
			float sum = mPtr[0] * vPtr[0];
			for (j = 1; j < numRows; j++) {
				mPtr += numColumns;
				sum += mPtr[0] * vPtr[j];
			}
			dstPtr[i] += sum;
		}
		break;
	}
}

/*
============
MatX_TransposeMultiplySubVecX
============
*/
void SIMD_VPCALL idSIMDProcessor::MatX_TransposeMultiplySubVecX(idVecX &dst, const idMatX &mat, const idVecX &vec) {
	int i, numColumns;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert(vec.GetSize() >= mat.GetNumRows());
	assert(dst.GetSize() >= mat.GetNumColumns());

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numColumns = mat.GetNumColumns();
	switch (mat.GetNumRows()) {
	case 1:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] -= *(mPtr)* vPtr[0];
			mPtr++;
		}
		break;
	case 2:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] -= *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1];
			mPtr++;
		}
		break;
	case 3:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] -= *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2];
			mPtr++;
		}
		break;
	case 4:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] -= *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2] +
				*(mPtr + 3 * numColumns) * vPtr[3];
			mPtr++;
		}
		break;
	case 5:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] -= *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2] +
				*(mPtr + 3 * numColumns) * vPtr[3] + *(mPtr + 4 * numColumns) * vPtr[4];
			mPtr++;
		}
		break;
	case 6:
		for (i = 0; i < numColumns; i++) {
			dstPtr[i] -= *(mPtr)* vPtr[0] + *(mPtr + numColumns) * vPtr[1] + *(mPtr + 2 * numColumns) * vPtr[2] +
				*(mPtr + 3 * numColumns) * vPtr[3] + *(mPtr + 4 * numColumns) * vPtr[4] + *(mPtr + 5 * numColumns) * vPtr[5];
			mPtr++;
		}
		break;
	default:
		int numRows = mat.GetNumRows();
		for (i = 0; i < numColumns; i++) {
			mPtr = mat.ToFloatPtr() + i;
			float sum = mPtr[0] * vPtr[0];
			for (int j = 1; j < numRows; j++) {
				mPtr += numColumns;
				sum += mPtr[0] * vPtr[j];
			}
			dstPtr[i] -= sum;
		}
		break;
	}
}

/*
============
MatX_MultiplyMatX

optimizes the following matrix multiplications:

NxN * Nx6
6xN * Nx6
Nx6 * 6xN
6x6 * 6xN

with N in the range [1-6].
============
*/
void SIMD_VPCALL idSIMDProcessor::MatX_MultiplyMatX(idMatX &dst, const idMatX &m1, const idMatX &m2) {
	int i, j, k, l, n;
	float *dstPtr;
	const float *m1Ptr, *m2Ptr;
	double sum;

	assert(m1.GetNumColumns() == m2.GetNumRows());

	dstPtr = dst.ToFloatPtr();
	m1Ptr = m1.ToFloatPtr();
	m2Ptr = m2.ToFloatPtr();
	k = m1.GetNumRows();
	l = m2.GetNumColumns();

	switch (m1.GetNumColumns()) {
	case 1: {
		if (l == 6) {
			for (i = 0; i < k; i++) {		// Nx1 * 1x6
				*dstPtr++ = m1Ptr[i] * m2Ptr[0];
				*dstPtr++ = m1Ptr[i] * m2Ptr[1];
				*dstPtr++ = m1Ptr[i] * m2Ptr[2];
				*dstPtr++ = m1Ptr[i] * m2Ptr[3];
				*dstPtr++ = m1Ptr[i] * m2Ptr[4];
				*dstPtr++ = m1Ptr[i] * m2Ptr[5];
			}
			return;
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0];
				m2Ptr++;
			}
			m1Ptr++;
		}
		break;
	}
	case 2: {
		if (l == 6) {
			for (i = 0; i < k; i++) {		// Nx2 * 2x6
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[6];
				*dstPtr++ = m1Ptr[0] * m2Ptr[1] + m1Ptr[1] * m2Ptr[7];
				*dstPtr++ = m1Ptr[0] * m2Ptr[2] + m1Ptr[1] * m2Ptr[8];
				*dstPtr++ = m1Ptr[0] * m2Ptr[3] + m1Ptr[1] * m2Ptr[9];
				*dstPtr++ = m1Ptr[0] * m2Ptr[4] + m1Ptr[1] * m2Ptr[10];
				*dstPtr++ = m1Ptr[0] * m2Ptr[5] + m1Ptr[1] * m2Ptr[11];
				m1Ptr += 2;
			}
			return;
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l];
				m2Ptr++;
			}
			m1Ptr += 2;
		}
		break;
	}
	case 3: {
		if (l == 6) {
			for (i = 0; i < k; i++) {		// Nx3 * 3x6
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[6] + m1Ptr[2] * m2Ptr[12];
				*dstPtr++ = m1Ptr[0] * m2Ptr[1] + m1Ptr[1] * m2Ptr[7] + m1Ptr[2] * m2Ptr[13];
				*dstPtr++ = m1Ptr[0] * m2Ptr[2] + m1Ptr[1] * m2Ptr[8] + m1Ptr[2] * m2Ptr[14];
				*dstPtr++ = m1Ptr[0] * m2Ptr[3] + m1Ptr[1] * m2Ptr[9] + m1Ptr[2] * m2Ptr[15];
				*dstPtr++ = m1Ptr[0] * m2Ptr[4] + m1Ptr[1] * m2Ptr[10] + m1Ptr[2] * m2Ptr[16];
				*dstPtr++ = m1Ptr[0] * m2Ptr[5] + m1Ptr[1] * m2Ptr[11] + m1Ptr[2] * m2Ptr[17];
				m1Ptr += 3;
			}
			return;
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2 * l];
				m2Ptr++;
			}
			m1Ptr += 3;
		}
		break;
	}
	case 4: {
		if (l == 6) {
			for (i = 0; i < k; i++) {		// Nx4 * 4x6
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[6] + m1Ptr[2] * m2Ptr[12] + m1Ptr[3] * m2Ptr[18];
				*dstPtr++ = m1Ptr[0] * m2Ptr[1] + m1Ptr[1] * m2Ptr[7] + m1Ptr[2] * m2Ptr[13] + m1Ptr[3] * m2Ptr[19];
				*dstPtr++ = m1Ptr[0] * m2Ptr[2] + m1Ptr[1] * m2Ptr[8] + m1Ptr[2] * m2Ptr[14] + m1Ptr[3] * m2Ptr[20];
				*dstPtr++ = m1Ptr[0] * m2Ptr[3] + m1Ptr[1] * m2Ptr[9] + m1Ptr[2] * m2Ptr[15] + m1Ptr[3] * m2Ptr[21];
				*dstPtr++ = m1Ptr[0] * m2Ptr[4] + m1Ptr[1] * m2Ptr[10] + m1Ptr[2] * m2Ptr[16] + m1Ptr[3] * m2Ptr[22];
				*dstPtr++ = m1Ptr[0] * m2Ptr[5] + m1Ptr[1] * m2Ptr[11] + m1Ptr[2] * m2Ptr[17] + m1Ptr[3] * m2Ptr[23];
				m1Ptr += 4;
			}
			return;
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2 * l] +
					m1Ptr[3] * m2Ptr[3 * l];
				m2Ptr++;
			}
			m1Ptr += 4;
		}
		break;
	}
	case 5: {
		if (l == 6) {
			for (i = 0; i < k; i++) {		// Nx5 * 5x6
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[6] + m1Ptr[2] * m2Ptr[12] + m1Ptr[3] * m2Ptr[18] + m1Ptr[4] * m2Ptr[24];
				*dstPtr++ = m1Ptr[0] * m2Ptr[1] + m1Ptr[1] * m2Ptr[7] + m1Ptr[2] * m2Ptr[13] + m1Ptr[3] * m2Ptr[19] + m1Ptr[4] * m2Ptr[25];
				*dstPtr++ = m1Ptr[0] * m2Ptr[2] + m1Ptr[1] * m2Ptr[8] + m1Ptr[2] * m2Ptr[14] + m1Ptr[3] * m2Ptr[20] + m1Ptr[4] * m2Ptr[26];
				*dstPtr++ = m1Ptr[0] * m2Ptr[3] + m1Ptr[1] * m2Ptr[9] + m1Ptr[2] * m2Ptr[15] + m1Ptr[3] * m2Ptr[21] + m1Ptr[4] * m2Ptr[27];
				*dstPtr++ = m1Ptr[0] * m2Ptr[4] + m1Ptr[1] * m2Ptr[10] + m1Ptr[2] * m2Ptr[16] + m1Ptr[3] * m2Ptr[22] + m1Ptr[4] * m2Ptr[28];
				*dstPtr++ = m1Ptr[0] * m2Ptr[5] + m1Ptr[1] * m2Ptr[11] + m1Ptr[2] * m2Ptr[17] + m1Ptr[3] * m2Ptr[23] + m1Ptr[4] * m2Ptr[29];
				m1Ptr += 5;
			}
			return;
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2 * l] +
					m1Ptr[3] * m2Ptr[3 * l] + m1Ptr[4] * m2Ptr[4 * l];
				m2Ptr++;
			}
			m1Ptr += 5;
		}
		break;
	}
	case 6: {
		switch (k) {
		case 1: {
			if (l == 1) {		// 1x6 * 6x1
				dstPtr[0] = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[1] + m1Ptr[2] * m2Ptr[2] +
					m1Ptr[3] * m2Ptr[3] + m1Ptr[4] * m2Ptr[4] + m1Ptr[5] * m2Ptr[5];
				return;
			}
			break;
		}
		case 2: {
			if (l == 2) {		// 2x6 * 6x2
				for (i = 0; i < 2; i++) {
					for (j = 0; j < 2; j++) {
						*dstPtr = m1Ptr[0] * m2Ptr[0 * 2 + j]
							+ m1Ptr[1] * m2Ptr[1 * 2 + j]
							+ m1Ptr[2] * m2Ptr[2 * 2 + j]
							+ m1Ptr[3] * m2Ptr[3 * 2 + j]
							+ m1Ptr[4] * m2Ptr[4 * 2 + j]
							+ m1Ptr[5] * m2Ptr[5 * 2 + j];
						dstPtr++;
					}
					m1Ptr += 6;
				}
				return;
			}
			break;
		}
		case 3: {
			if (l == 3) {		// 3x6 * 6x3
				for (i = 0; i < 3; i++) {
					for (j = 0; j < 3; j++) {
						*dstPtr = m1Ptr[0] * m2Ptr[0 * 3 + j]
							+ m1Ptr[1] * m2Ptr[1 * 3 + j]
							+ m1Ptr[2] * m2Ptr[2 * 3 + j]
							+ m1Ptr[3] * m2Ptr[3 * 3 + j]
							+ m1Ptr[4] * m2Ptr[4 * 3 + j]
							+ m1Ptr[5] * m2Ptr[5 * 3 + j];
						dstPtr++;
					}
					m1Ptr += 6;
				}
				return;
			}
			break;
		}
		case 4: {
			if (l == 4) {		// 4x6 * 6x4
				for (i = 0; i < 4; i++) {
					for (j = 0; j < 4; j++) {
						*dstPtr = m1Ptr[0] * m2Ptr[0 * 4 + j]
							+ m1Ptr[1] * m2Ptr[1 * 4 + j]
							+ m1Ptr[2] * m2Ptr[2 * 4 + j]
							+ m1Ptr[3] * m2Ptr[3 * 4 + j]
							+ m1Ptr[4] * m2Ptr[4 * 4 + j]
							+ m1Ptr[5] * m2Ptr[5 * 4 + j];
						dstPtr++;
					}
					m1Ptr += 6;
				}
				return;
			}
		}
		case 5: {
			if (l == 5) {		// 5x6 * 6x5
				for (i = 0; i < 5; i++) {
					for (j = 0; j < 5; j++) {
						*dstPtr = m1Ptr[0] * m2Ptr[0 * 5 + j]
							+ m1Ptr[1] * m2Ptr[1 * 5 + j]
							+ m1Ptr[2] * m2Ptr[2 * 5 + j]
							+ m1Ptr[3] * m2Ptr[3 * 5 + j]
							+ m1Ptr[4] * m2Ptr[4 * 5 + j]
							+ m1Ptr[5] * m2Ptr[5 * 5 + j];
						dstPtr++;
					}
					m1Ptr += 6;
				}
				return;
			}
		}
		case 6: {
			switch (l) {
			case 1: {		// 6x6 * 6x1
				for (i = 0; i < 6; i++) {
					*dstPtr = m1Ptr[0] * m2Ptr[0 * 1]
						+ m1Ptr[1] * m2Ptr[1 * 1]
						+ m1Ptr[2] * m2Ptr[2 * 1]
						+ m1Ptr[3] * m2Ptr[3 * 1]
						+ m1Ptr[4] * m2Ptr[4 * 1]
						+ m1Ptr[5] * m2Ptr[5 * 1];
					dstPtr++;
					m1Ptr += 6;
				}
				return;
			}
			case 2: {		// 6x6 * 6x2
				for (i = 0; i < 6; i++) {
					for (j = 0; j < 2; j++) {
						*dstPtr = m1Ptr[0] * m2Ptr[0 * 2 + j]
							+ m1Ptr[1] * m2Ptr[1 * 2 + j]
							+ m1Ptr[2] * m2Ptr[2 * 2 + j]
							+ m1Ptr[3] * m2Ptr[3 * 2 + j]
							+ m1Ptr[4] * m2Ptr[4 * 2 + j]
							+ m1Ptr[5] * m2Ptr[5 * 2 + j];
						dstPtr++;
					}
					m1Ptr += 6;
				}
				return;
			}
			case 3: {		// 6x6 * 6x3
				for (i = 0; i < 6; i++) {
					for (j = 0; j < 3; j++) {
						*dstPtr = m1Ptr[0] * m2Ptr[0 * 3 + j]
							+ m1Ptr[1] * m2Ptr[1 * 3 + j]
							+ m1Ptr[2] * m2Ptr[2 * 3 + j]
							+ m1Ptr[3] * m2Ptr[3 * 3 + j]
							+ m1Ptr[4] * m2Ptr[4 * 3 + j]
							+ m1Ptr[5] * m2Ptr[5 * 3 + j];
						dstPtr++;
					}
					m1Ptr += 6;
				}
				return;
			}
			case 4: {		// 6x6 * 6x4
				for (i = 0; i < 6; i++) {
					for (j = 0; j < 4; j++) {
						*dstPtr = m1Ptr[0] * m2Ptr[0 * 4 + j]
							+ m1Ptr[1] * m2Ptr[1 * 4 + j]
							+ m1Ptr[2] * m2Ptr[2 * 4 + j]
							+ m1Ptr[3] * m2Ptr[3 * 4 + j]
							+ m1Ptr[4] * m2Ptr[4 * 4 + j]
							+ m1Ptr[5] * m2Ptr[5 * 4 + j];
						dstPtr++;
					}
					m1Ptr += 6;
				}
				return;
			}
			case 5: {		// 6x6 * 6x5
				for (i = 0; i < 6; i++) {
					for (j = 0; j < 5; j++) {
						*dstPtr = m1Ptr[0] * m2Ptr[0 * 5 + j]
							+ m1Ptr[1] * m2Ptr[1 * 5 + j]
							+ m1Ptr[2] * m2Ptr[2 * 5 + j]
							+ m1Ptr[3] * m2Ptr[3 * 5 + j]
							+ m1Ptr[4] * m2Ptr[4 * 5 + j]
							+ m1Ptr[5] * m2Ptr[5 * 5 + j];
						dstPtr++;
					}
					m1Ptr += 6;
				}
				return;
			}
			case 6: {		// 6x6 * 6x6
				for (i = 0; i < 6; i++) {
					for (j = 0; j < 6; j++) {
						*dstPtr = m1Ptr[0] * m2Ptr[0 * 6 + j]
							+ m1Ptr[1] * m2Ptr[1 * 6 + j]
							+ m1Ptr[2] * m2Ptr[2 * 6 + j]
							+ m1Ptr[3] * m2Ptr[3 * 6 + j]
							+ m1Ptr[4] * m2Ptr[4 * 6 + j]
							+ m1Ptr[5] * m2Ptr[5 * 6 + j];
						dstPtr++;
					}
					m1Ptr += 6;
				}
				return;
			}
			}
		}
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2 * l] +
					m1Ptr[3] * m2Ptr[3 * l] + m1Ptr[4] * m2Ptr[4 * l] + m1Ptr[5] * m2Ptr[5 * l];
				m2Ptr++;
			}
			m1Ptr += 6;
		}
		break;
	}
	default: {
		for (i = 0; i < k; i++) {
			for (j = 0; j < l; j++) {
				m2Ptr = m2.ToFloatPtr() + j;
				sum = m1Ptr[0] * m2Ptr[0];
				for (n = 1; n < m1.GetNumColumns(); n++) {
					m2Ptr += l;
					sum += m1Ptr[n] * m2Ptr[0];
				}
				*dstPtr++ = sum;
			}
			m1Ptr += m1.GetNumColumns();
		}
		break;
	}
	}
}

/*
============
MatX_TransposeMultiplyMatX

optimizes the following tranpose matrix multiplications:

Nx6 * NxN
6xN * 6x6

with N in the range [1-6].
============
*/
void SIMD_VPCALL idSIMDProcessor::MatX_TransposeMultiplyMatX(idMatX &dst, const idMatX &m1, const idMatX &m2) {
	int i, j, k, l, n;
	float *dstPtr;
	const float *m1Ptr, *m2Ptr;
	double sum;

	assert(m1.GetNumRows() == m2.GetNumRows());

	m1Ptr = m1.ToFloatPtr();
	m2Ptr = m2.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	k = m1.GetNumColumns();
	l = m2.GetNumColumns();

	switch (m1.GetNumRows()) {
	case 1:
		if (k == 6 && l == 1) {			// 1x6 * 1x1
			for (i = 0; i < 6; i++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0];
				m1Ptr++;
			}
			return;
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0];
				m2Ptr++;
			}
			m1Ptr++;
		}
		break;
	case 2:
		if (k == 6 && l == 2) {			// 2x6 * 2x2
			for (i = 0; i < 6; i++) {
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 2 + 0] + m1Ptr[1 * 6] * m2Ptr[1 * 2 + 0];
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 2 + 1] + m1Ptr[1 * 6] * m2Ptr[1 * 2 + 1];
				m1Ptr++;
			}
			return;
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l];
				m2Ptr++;
			}
			m1Ptr++;
		}
		break;
	case 3:
		if (k == 6 && l == 3) {			// 3x6 * 3x3
			for (i = 0; i < 6; i++) {
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 3 + 0] + m1Ptr[1 * 6] * m2Ptr[1 * 3 + 0] + m1Ptr[2 * 6] * m2Ptr[2 * 3 + 0];
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 3 + 1] + m1Ptr[1 * 6] * m2Ptr[1 * 3 + 1] + m1Ptr[2 * 6] * m2Ptr[2 * 3 + 1];
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 3 + 2] + m1Ptr[1 * 6] * m2Ptr[1 * 3 + 2] + m1Ptr[2 * 6] * m2Ptr[2 * 3 + 2];
				m1Ptr++;
			}
			return;
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2 * k] * m2Ptr[2 * l];
				m2Ptr++;
			}
			m1Ptr++;
		}
		break;
	case 4:
		if (k == 6 && l == 4) {			// 4x6 * 4x4
			for (i = 0; i < 6; i++) {
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 4 + 0] + m1Ptr[1 * 6] * m2Ptr[1 * 4 + 0] + m1Ptr[2 * 6] * m2Ptr[2 * 4 + 0] + m1Ptr[3 * 6] * m2Ptr[3 * 4 + 0];
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 4 + 1] + m1Ptr[1 * 6] * m2Ptr[1 * 4 + 1] + m1Ptr[2 * 6] * m2Ptr[2 * 4 + 1] + m1Ptr[3 * 6] * m2Ptr[3 * 4 + 1];
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 4 + 2] + m1Ptr[1 * 6] * m2Ptr[1 * 4 + 2] + m1Ptr[2 * 6] * m2Ptr[2 * 4 + 2] + m1Ptr[3 * 6] * m2Ptr[3 * 4 + 2];
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 4 + 3] + m1Ptr[1 * 6] * m2Ptr[1 * 4 + 3] + m1Ptr[2 * 6] * m2Ptr[2 * 4 + 3] + m1Ptr[3 * 6] * m2Ptr[3 * 4 + 3];
				m1Ptr++;
			}
			return;
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2 * k] * m2Ptr[2 * l] +
					m1Ptr[3 * k] * m2Ptr[3 * l];
				m2Ptr++;
			}
			m1Ptr++;
		}
		break;
	case 5:
		if (k == 6 && l == 5) {			// 5x6 * 5x5
			for (i = 0; i < 6; i++) {
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 5 + 0] + m1Ptr[1 * 6] * m2Ptr[1 * 5 + 0] + m1Ptr[2 * 6] * m2Ptr[2 * 5 + 0] + m1Ptr[3 * 6] * m2Ptr[3 * 5 + 0] + m1Ptr[4 * 6] * m2Ptr[4 * 5 + 0];
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 5 + 1] + m1Ptr[1 * 6] * m2Ptr[1 * 5 + 1] + m1Ptr[2 * 6] * m2Ptr[2 * 5 + 1] + m1Ptr[3 * 6] * m2Ptr[3 * 5 + 1] + m1Ptr[4 * 6] * m2Ptr[4 * 5 + 1];
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 5 + 2] + m1Ptr[1 * 6] * m2Ptr[1 * 5 + 2] + m1Ptr[2 * 6] * m2Ptr[2 * 5 + 2] + m1Ptr[3 * 6] * m2Ptr[3 * 5 + 2] + m1Ptr[4 * 6] * m2Ptr[4 * 5 + 2];
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 5 + 3] + m1Ptr[1 * 6] * m2Ptr[1 * 5 + 3] + m1Ptr[2 * 6] * m2Ptr[2 * 5 + 3] + m1Ptr[3 * 6] * m2Ptr[3 * 5 + 3] + m1Ptr[4 * 6] * m2Ptr[4 * 5 + 3];
				*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 5 + 4] + m1Ptr[1 * 6] * m2Ptr[1 * 5 + 4] + m1Ptr[2 * 6] * m2Ptr[2 * 5 + 4] + m1Ptr[3 * 6] * m2Ptr[3 * 5 + 4] + m1Ptr[4 * 6] * m2Ptr[4 * 5 + 4];
				m1Ptr++;
			}
			return;
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2 * k] * m2Ptr[2 * l] +
					m1Ptr[3 * k] * m2Ptr[3 * l] + m1Ptr[4 * k] * m2Ptr[4 * l];
				m2Ptr++;
			}
			m1Ptr++;
		}
		break;
	case 6:
		if (l == 6) {
			switch (k) {
			case 1:						// 6x1 * 6x6
				m2Ptr = m2.ToFloatPtr();
				for (j = 0; j < 6; j++) {
					*dstPtr++ = m1Ptr[0 * 1] * m2Ptr[0 * 6] +
						m1Ptr[1 * 1] * m2Ptr[1 * 6] +
						m1Ptr[2 * 1] * m2Ptr[2 * 6] +
						m1Ptr[3 * 1] * m2Ptr[3 * 6] +
						m1Ptr[4 * 1] * m2Ptr[4 * 6] +
						m1Ptr[5 * 1] * m2Ptr[5 * 6];
					m2Ptr++;
				}
				return;
			case 2:						// 6x2 * 6x6
				for (i = 0; i < 2; i++) {
					m2Ptr = m2.ToFloatPtr();
					for (j = 0; j < 6; j++) {
						*dstPtr++ = m1Ptr[0 * 2] * m2Ptr[0 * 6] +
							m1Ptr[1 * 2] * m2Ptr[1 * 6] +
							m1Ptr[2 * 2] * m2Ptr[2 * 6] +
							m1Ptr[3 * 2] * m2Ptr[3 * 6] +
							m1Ptr[4 * 2] * m2Ptr[4 * 6] +
							m1Ptr[5 * 2] * m2Ptr[5 * 6];
						m2Ptr++;
					}
					m1Ptr++;
				}
				return;
			case 3:						// 6x3 * 6x6
				for (i = 0; i < 3; i++) {
					m2Ptr = m2.ToFloatPtr();
					for (j = 0; j < 6; j++) {
						*dstPtr++ = m1Ptr[0 * 3] * m2Ptr[0 * 6] +
							m1Ptr[1 * 3] * m2Ptr[1 * 6] +
							m1Ptr[2 * 3] * m2Ptr[2 * 6] +
							m1Ptr[3 * 3] * m2Ptr[3 * 6] +
							m1Ptr[4 * 3] * m2Ptr[4 * 6] +
							m1Ptr[5 * 3] * m2Ptr[5 * 6];
						m2Ptr++;
					}
					m1Ptr++;
				}
				return;
			case 4:						// 6x4 * 6x6
				for (i = 0; i < 4; i++) {
					m2Ptr = m2.ToFloatPtr();
					for (j = 0; j < 6; j++) {
						*dstPtr++ = m1Ptr[0 * 4] * m2Ptr[0 * 6] +
							m1Ptr[1 * 4] * m2Ptr[1 * 6] +
							m1Ptr[2 * 4] * m2Ptr[2 * 6] +
							m1Ptr[3 * 4] * m2Ptr[3 * 6] +
							m1Ptr[4 * 4] * m2Ptr[4 * 6] +
							m1Ptr[5 * 4] * m2Ptr[5 * 6];
						m2Ptr++;
					}
					m1Ptr++;
				}
				return;
			case 5:						// 6x5 * 6x6
				for (i = 0; i < 5; i++) {
					m2Ptr = m2.ToFloatPtr();
					for (j = 0; j < 6; j++) {
						*dstPtr++ = m1Ptr[0 * 5] * m2Ptr[0 * 6] +
							m1Ptr[1 * 5] * m2Ptr[1 * 6] +
							m1Ptr[2 * 5] * m2Ptr[2 * 6] +
							m1Ptr[3 * 5] * m2Ptr[3 * 6] +
							m1Ptr[4 * 5] * m2Ptr[4 * 6] +
							m1Ptr[5 * 5] * m2Ptr[5 * 6];
						m2Ptr++;
					}
					m1Ptr++;
				}
				return;
			case 6:						// 6x6 * 6x6
				for (i = 0; i < 6; i++) {
					m2Ptr = m2.ToFloatPtr();
					for (j = 0; j < 6; j++) {
						*dstPtr++ = m1Ptr[0 * 6] * m2Ptr[0 * 6] +
							m1Ptr[1 * 6] * m2Ptr[1 * 6] +
							m1Ptr[2 * 6] * m2Ptr[2 * 6] +
							m1Ptr[3 * 6] * m2Ptr[3 * 6] +
							m1Ptr[4 * 6] * m2Ptr[4 * 6] +
							m1Ptr[5 * 6] * m2Ptr[5 * 6];
						m2Ptr++;
					}
					m1Ptr++;
				}
				return;
			}
		}
		for (i = 0; i < k; i++) {
			m2Ptr = m2.ToFloatPtr();
			for (j = 0; j < l; j++) {
				*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2 * k] * m2Ptr[2 * l] +
					m1Ptr[3 * k] * m2Ptr[3 * l] + m1Ptr[4 * k] * m2Ptr[4 * l] + m1Ptr[5 * k] * m2Ptr[5 * l];
				m2Ptr++;
			}
			m1Ptr++;
		}
		break;
	default:
		for (i = 0; i < k; i++) {
			for (j = 0; j < l; j++) {
				m1Ptr = m1.ToFloatPtr() + i;
				m2Ptr = m2.ToFloatPtr() + j;
				sum = m1Ptr[0] * m2Ptr[0];
				for (n = 1; n < m1.GetNumRows(); n++) {
					m1Ptr += k;
					m2Ptr += l;
					sum += m1Ptr[0] * m2Ptr[0];
				}
				*dstPtr++ = sum;
			}
		}
		break;
	}
}

/*
============
MatX_LowerTriangularSolve

solves x in Lx = b for the n * n sub-matrix of L
if skip > 0 the first skip elements of x are assumed to be valid already
L has to be a lower triangular matrix with (implicit) ones on the diagonal
x == b is allowed
============
*/
void SIMD_VPCALL idSIMDProcessor::MatX_LowerTriangularSolve(const idMatX &L, float *x, const float *b, const int n, int skip) {
#if 1

	int nc;
	const float *lptr;

	if (skip >= n) {
		return;
	}

	lptr = L.ToFloatPtr();
	nc = L.GetNumColumns();

	// unrolled cases for n < 8
	if (n < 8) {
#define NSKIP( n, s )	((n<<3)|(s&7))
		switch (NSKIP(n, skip)) {
		case NSKIP(1, 0): x[0] = b[0];
			return;
		case NSKIP(2, 0): x[0] = b[0];
		case NSKIP(2, 1): x[1] = b[1] - lptr[1 * nc + 0] * x[0];
			return;
		case NSKIP(3, 0): x[0] = b[0];
		case NSKIP(3, 1): x[1] = b[1] - lptr[1 * nc + 0] * x[0];
		case NSKIP(3, 2): x[2] = b[2] - lptr[2 * nc + 0] * x[0] - lptr[2 * nc + 1] * x[1];
			return;
		case NSKIP(4, 0): x[0] = b[0];
		case NSKIP(4, 1): x[1] = b[1] - lptr[1 * nc + 0] * x[0];
		case NSKIP(4, 2): x[2] = b[2] - lptr[2 * nc + 0] * x[0] - lptr[2 * nc + 1] * x[1];
		case NSKIP(4, 3): x[3] = b[3] - lptr[3 * nc + 0] * x[0] - lptr[3 * nc + 1] * x[1] - lptr[3 * nc + 2] * x[2];
			return;
		case NSKIP(5, 0): x[0] = b[0];
		case NSKIP(5, 1): x[1] = b[1] - lptr[1 * nc + 0] * x[0];
		case NSKIP(5, 2): x[2] = b[2] - lptr[2 * nc + 0] * x[0] - lptr[2 * nc + 1] * x[1];
		case NSKIP(5, 3): x[3] = b[3] - lptr[3 * nc + 0] * x[0] - lptr[3 * nc + 1] * x[1] - lptr[3 * nc + 2] * x[2];
		case NSKIP(5, 4): x[4] = b[4] - lptr[4 * nc + 0] * x[0] - lptr[4 * nc + 1] * x[1] - lptr[4 * nc + 2] * x[2] - lptr[4 * nc + 3] * x[3];
			return;
		case NSKIP(6, 0): x[0] = b[0];
		case NSKIP(6, 1): x[1] = b[1] - lptr[1 * nc + 0] * x[0];
		case NSKIP(6, 2): x[2] = b[2] - lptr[2 * nc + 0] * x[0] - lptr[2 * nc + 1] * x[1];
		case NSKIP(6, 3): x[3] = b[3] - lptr[3 * nc + 0] * x[0] - lptr[3 * nc + 1] * x[1] - lptr[3 * nc + 2] * x[2];
		case NSKIP(6, 4): x[4] = b[4] - lptr[4 * nc + 0] * x[0] - lptr[4 * nc + 1] * x[1] - lptr[4 * nc + 2] * x[2] - lptr[4 * nc + 3] * x[3];
		case NSKIP(6, 5): x[5] = b[5] - lptr[5 * nc + 0] * x[0] - lptr[5 * nc + 1] * x[1] - lptr[5 * nc + 2] * x[2] - lptr[5 * nc + 3] * x[3] - lptr[5 * nc + 4] * x[4];
			return;
		case NSKIP(7, 0): x[0] = b[0];
		case NSKIP(7, 1): x[1] = b[1] - lptr[1 * nc + 0] * x[0];
		case NSKIP(7, 2): x[2] = b[2] - lptr[2 * nc + 0] * x[0] - lptr[2 * nc + 1] * x[1];
		case NSKIP(7, 3): x[3] = b[3] - lptr[3 * nc + 0] * x[0] - lptr[3 * nc + 1] * x[1] - lptr[3 * nc + 2] * x[2];
		case NSKIP(7, 4): x[4] = b[4] - lptr[4 * nc + 0] * x[0] - lptr[4 * nc + 1] * x[1] - lptr[4 * nc + 2] * x[2] - lptr[4 * nc + 3] * x[3];
		case NSKIP(7, 5): x[5] = b[5] - lptr[5 * nc + 0] * x[0] - lptr[5 * nc + 1] * x[1] - lptr[5 * nc + 2] * x[2] - lptr[5 * nc + 3] * x[3] - lptr[5 * nc + 4] * x[4];
		case NSKIP(7, 6): x[6] = b[6] - lptr[6 * nc + 0] * x[0] - lptr[6 * nc + 1] * x[1] - lptr[6 * nc + 2] * x[2] - lptr[6 * nc + 3] * x[3] - lptr[6 * nc + 4] * x[4] - lptr[6 * nc + 5] * x[5];
			return;
		}
		return;
	}

	// process first 4 rows
	switch (skip) {
	case 0: x[0] = b[0];
	case 1: x[1] = b[1] - lptr[1 * nc + 0] * x[0];
	case 2: x[2] = b[2] - lptr[2 * nc + 0] * x[0] - lptr[2 * nc + 1] * x[1];
	case 3: x[3] = b[3] - lptr[3 * nc + 0] * x[0] - lptr[3 * nc + 1] * x[1] - lptr[3 * nc + 2] * x[2];
		skip = 4;
	}

	lptr = L[skip];

	int i, j;
	double s0, s1, s2, s3; // blendo eric: removed register keyword for new c++

	for (i = skip; i < n; i++) {
		s0 = lptr[0] * x[0];
		s1 = lptr[1] * x[1];
		s2 = lptr[2] * x[2];
		s3 = lptr[3] * x[3];
		for (j = 4; j < i - 7; j += 8) {
			s0 += lptr[j + 0] * x[j + 0];
			s1 += lptr[j + 1] * x[j + 1];
			s2 += lptr[j + 2] * x[j + 2];
			s3 += lptr[j + 3] * x[j + 3];
			s0 += lptr[j + 4] * x[j + 4];
			s1 += lptr[j + 5] * x[j + 5];
			s2 += lptr[j + 6] * x[j + 6];
			s3 += lptr[j + 7] * x[j + 7];
		}
		switch (i - j) {
			SIMD_NODEFAULT;
		case 7: s0 += lptr[j + 6] * x[j + 6];
		case 6: s1 += lptr[j + 5] * x[j + 5];
		case 5: s2 += lptr[j + 4] * x[j + 4];
		case 4: s3 += lptr[j + 3] * x[j + 3];
		case 3: s0 += lptr[j + 2] * x[j + 2];
		case 2: s1 += lptr[j + 1] * x[j + 1];
		case 1: s2 += lptr[j + 0] * x[j + 0];
		case 0: break;
		}
		double sum;
		sum = s3;
		sum += s2;
		sum += s1;
		sum += s0;
		sum -= b[i];
		x[i] = -sum;
		lptr += nc;
	}

#else

	int i, j;
	const float *lptr;
	double sum;

	for (i = skip; i < n; i++) {
		sum = b[i];
		lptr = L[i];
		for (j = 0; j < i; j++) {
			sum -= lptr[j] * x[j];
		}
		x[i] = sum;
	}

#endif
}

/*
============
MatX_LowerTriangularSolveTranspose

solves x in L'x = b for the n * n sub-matrix of L
L has to be a lower triangular matrix with (implicit) ones on the diagonal
x == b is allowed
============
*/
void SIMD_VPCALL idSIMDProcessor::MatX_LowerTriangularSolveTranspose(const idMatX &L, float *x, const float *b, const int n) {
#if 1

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
			x[0] = b[0] - lptr[1 * nc + 0] * x[1];
			return;
		case 3:
			x[2] = b[2];
			x[1] = b[1] - lptr[2 * nc + 1] * x[2];
			x[0] = b[0] - lptr[2 * nc + 0] * x[2] - lptr[1 * nc + 0] * x[1];
			return;
		case 4:
			x[3] = b[3];
			x[2] = b[2] - lptr[3 * nc + 2] * x[3];
			x[1] = b[1] - lptr[3 * nc + 1] * x[3] - lptr[2 * nc + 1] * x[2];
			x[0] = b[0] - lptr[3 * nc + 0] * x[3] - lptr[2 * nc + 0] * x[2] - lptr[1 * nc + 0] * x[1];
			return;
		case 5:
			x[4] = b[4];
			x[3] = b[3] - lptr[4 * nc + 3] * x[4];
			x[2] = b[2] - lptr[4 * nc + 2] * x[4] - lptr[3 * nc + 2] * x[3];
			x[1] = b[1] - lptr[4 * nc + 1] * x[4] - lptr[3 * nc + 1] * x[3] - lptr[2 * nc + 1] * x[2];
			x[0] = b[0] - lptr[4 * nc + 0] * x[4] - lptr[3 * nc + 0] * x[3] - lptr[2 * nc + 0] * x[2] - lptr[1 * nc + 0] * x[1];
			return;
		case 6:
			x[5] = b[5];
			x[4] = b[4] - lptr[5 * nc + 4] * x[5];
			x[3] = b[3] - lptr[5 * nc + 3] * x[5] - lptr[4 * nc + 3] * x[4];
			x[2] = b[2] - lptr[5 * nc + 2] * x[5] - lptr[4 * nc + 2] * x[4] - lptr[3 * nc + 2] * x[3];
			x[1] = b[1] - lptr[5 * nc + 1] * x[5] - lptr[4 * nc + 1] * x[4] - lptr[3 * nc + 1] * x[3] - lptr[2 * nc + 1] * x[2];
			x[0] = b[0] - lptr[5 * nc + 0] * x[5] - lptr[4 * nc + 0] * x[4] - lptr[3 * nc + 0] * x[3] - lptr[2 * nc + 0] * x[2] - lptr[1 * nc + 0] * x[1];
			return;
		case 7:
			x[6] = b[6];
			x[5] = b[5] - lptr[6 * nc + 5] * x[6];
			x[4] = b[4] - lptr[6 * nc + 4] * x[6] - lptr[5 * nc + 4] * x[5];
			x[3] = b[3] - lptr[6 * nc + 3] * x[6] - lptr[5 * nc + 3] * x[5] - lptr[4 * nc + 3] * x[4];
			x[2] = b[2] - lptr[6 * nc + 2] * x[6] - lptr[5 * nc + 2] * x[5] - lptr[4 * nc + 2] * x[4] - lptr[3 * nc + 2] * x[3];
			x[1] = b[1] - lptr[6 * nc + 1] * x[6] - lptr[5 * nc + 1] * x[5] - lptr[4 * nc + 1] * x[4] - lptr[3 * nc + 1] * x[3] - lptr[2 * nc + 1] * x[2];
			x[0] = b[0] - lptr[6 * nc + 0] * x[6] - lptr[5 * nc + 0] * x[5] - lptr[4 * nc + 0] * x[4] - lptr[3 * nc + 0] * x[3] - lptr[2 * nc + 0] * x[2] - lptr[1 * nc + 0] * x[1];
			return;
		}
		return;
	}

	int i, j;
	double s0, s1, s2, s3; // blendo eric: removed register keyword for new c++
	float *xptr;

	lptr = L.ToFloatPtr() + n * nc + n - 4;
	xptr = x + n;

	// process 4 rows at a time
	for (i = n; i >= 4; i -= 4) {
		s0 = b[i - 4];
		s1 = b[i - 3];
		s2 = b[i - 2];
		s3 = b[i - 1];
		// process 4x4 blocks
		for (j = 0; j < n - i; j += 4) {
			s0 -= lptr[(j + 0)*nc + 0] * xptr[j + 0];
			s1 -= lptr[(j + 0)*nc + 1] * xptr[j + 0];
			s2 -= lptr[(j + 0)*nc + 2] * xptr[j + 0];
			s3 -= lptr[(j + 0)*nc + 3] * xptr[j + 0];
			s0 -= lptr[(j + 1)*nc + 0] * xptr[j + 1];
			s1 -= lptr[(j + 1)*nc + 1] * xptr[j + 1];
			s2 -= lptr[(j + 1)*nc + 2] * xptr[j + 1];
			s3 -= lptr[(j + 1)*nc + 3] * xptr[j + 1];
			s0 -= lptr[(j + 2)*nc + 0] * xptr[j + 2];
			s1 -= lptr[(j + 2)*nc + 1] * xptr[j + 2];
			s2 -= lptr[(j + 2)*nc + 2] * xptr[j + 2];
			s3 -= lptr[(j + 2)*nc + 3] * xptr[j + 2];
			s0 -= lptr[(j + 3)*nc + 0] * xptr[j + 3];
			s1 -= lptr[(j + 3)*nc + 1] * xptr[j + 3];
			s2 -= lptr[(j + 3)*nc + 2] * xptr[j + 3];
			s3 -= lptr[(j + 3)*nc + 3] * xptr[j + 3];
		}
		// process left over of the 4 rows
		s0 -= lptr[0 - 1 * nc] * s3;
		s1 -= lptr[1 - 1 * nc] * s3;
		s2 -= lptr[2 - 1 * nc] * s3;
		s0 -= lptr[0 - 2 * nc] * s2;
		s1 -= lptr[1 - 2 * nc] * s2;
		s0 -= lptr[0 - 3 * nc] * s1;
		// store result
		xptr[-4] = s0;
		xptr[-3] = s1;
		xptr[-2] = s2;
		xptr[-1] = s3;
		// update pointers for next four rows
		lptr -= 4 + 4 * nc;
		xptr -= 4;
	}
	// process left over rows
	for (i--; i >= 0; i--) {
		s0 = b[i];
		lptr = L[0] + i;
		for (j = i + 1; j < n; j++) {
			s0 -= lptr[j*nc] * x[j];
		}
		x[i] = s0;
	}

#else

	int i, j, nc;
	const float *ptr;
	double sum;

	nc = L.GetNumColumns();
	for (i = n - 1; i >= 0; i--) {
		sum = b[i];
		ptr = L[0] + i;
		for (j = i + 1; j < n; j++) {
			sum -= ptr[j*nc] * x[j];
		}
		x[i] = sum;
	}

#endif
}

/*
============
MatX_LDLTFactor

in-place factorization LDL' of the n * n sub-matrix of mat
the reciprocal of the diagonal elements are stored in invDiag
============
*/
bool SIMD_VPCALL idSIMDProcessor::MatX_LDLTFactor(idMatX &mat, idVecX &invDiag, const int n) {
#if 1

	int i, j, k, nc;
	float *v, *diag, *mptr;
	double s0, s1, s2, s3, sum, d;

	v = (float *)_alloca16(n * sizeof(float));
	diag = (float *)_alloca16(n * sizeof(float));

	nc = mat.GetNumColumns();

	if (n <= 0) {
		return true;
	}

	mptr = mat[0];

	sum = mptr[0];

	if (sum == 0.0f) {
		return false;
	}

	diag[0] = sum;
	invDiag[0] = d = 1.0f / sum;

	if (n <= 1) {
		return true;
	}

	mptr = mat[0];
	for (j = 1; j < n; j++) {
		mptr[j*nc + 0] = (mptr[j*nc + 0]) * d;
	}

	mptr = mat[1];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	sum = mptr[1] - s0;

	if (sum == 0.0f) {
		return false;
	}

	mat[1][1] = sum;
	diag[1] = sum;
	invDiag[1] = d = 1.0f / sum;

	if (n <= 2) {
		return true;
	}

	mptr = mat[0];
	for (j = 2; j < n; j++) {
		mptr[j*nc + 1] = (mptr[j*nc + 1] - v[0] * mptr[j*nc + 0]) * d;
	}

	mptr = mat[2];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	v[1] = diag[1] * mptr[1]; s1 = v[1] * mptr[1];
	sum = mptr[2] - s0 - s1;

	if (sum == 0.0f) {
		return false;
	}

	mat[2][2] = sum;
	diag[2] = sum;
	invDiag[2] = d = 1.0f / sum;

	if (n <= 3) {
		return true;
	}

	mptr = mat[0];
	for (j = 3; j < n; j++) {
		mptr[j*nc + 2] = (mptr[j*nc + 2] - v[0] * mptr[j*nc + 0] - v[1] * mptr[j*nc + 1]) * d;
	}

	mptr = mat[3];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	v[1] = diag[1] * mptr[1]; s1 = v[1] * mptr[1];
	v[2] = diag[2] * mptr[2]; s2 = v[2] * mptr[2];
	sum = mptr[3] - s0 - s1 - s2;

	if (sum == 0.0f) {
		return false;
	}

	mat[3][3] = sum;
	diag[3] = sum;
	invDiag[3] = d = 1.0f / sum;

	if (n <= 4) {
		return true;
	}

	mptr = mat[0];
	for (j = 4; j < n; j++) {
		mptr[j*nc + 3] = (mptr[j*nc + 3] - v[0] * mptr[j*nc + 0] - v[1] * mptr[j*nc + 1] - v[2] * mptr[j*nc + 2]) * d;
	}

	for (i = 4; i < n; i++) {

		mptr = mat[i];

		v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
		v[1] = diag[1] * mptr[1]; s1 = v[1] * mptr[1];
		v[2] = diag[2] * mptr[2]; s2 = v[2] * mptr[2];
		v[3] = diag[3] * mptr[3]; s3 = v[3] * mptr[3];
		for (k = 4; k < i - 3; k += 4) {
			v[k + 0] = diag[k + 0] * mptr[k + 0]; s0 += v[k + 0] * mptr[k + 0];
			v[k + 1] = diag[k + 1] * mptr[k + 1]; s1 += v[k + 1] * mptr[k + 1];
			v[k + 2] = diag[k + 2] * mptr[k + 2]; s2 += v[k + 2] * mptr[k + 2];
			v[k + 3] = diag[k + 3] * mptr[k + 3]; s3 += v[k + 3] * mptr[k + 3];
		}
		switch (i - k) {
			SIMD_NODEFAULT;
		case 3: v[k + 2] = diag[k + 2] * mptr[k + 2]; s0 += v[k + 2] * mptr[k + 2];
		case 2: v[k + 1] = diag[k + 1] * mptr[k + 1]; s1 += v[k + 1] * mptr[k + 1];
		case 1: v[k + 0] = diag[k + 0] * mptr[k + 0]; s2 += v[k + 0] * mptr[k + 0];
		case 0: break;
		}
		sum = s3;
		sum += s2;
		sum += s1;
		sum += s0;
		sum = mptr[i] - sum;

		if (sum == 0.0f) {
			return false;
		}

		mat[i][i] = sum;
		diag[i] = sum;
		invDiag[i] = d = 1.0f / sum;

		if (i + 1 >= n) {
			return true;
		}

		mptr = mat[i + 1];
		for (j = i + 1; j < n; j++) {
			s0 = mptr[0] * v[0];
			s1 = mptr[1] * v[1];
			s2 = mptr[2] * v[2];
			s3 = mptr[3] * v[3];
			for (k = 4; k < i - 7; k += 8) {
				s0 += mptr[k + 0] * v[k + 0];
				s1 += mptr[k + 1] * v[k + 1];
				s2 += mptr[k + 2] * v[k + 2];
				s3 += mptr[k + 3] * v[k + 3];
				s0 += mptr[k + 4] * v[k + 4];
				s1 += mptr[k + 5] * v[k + 5];
				s2 += mptr[k + 6] * v[k + 6];
				s3 += mptr[k + 7] * v[k + 7];
			}
			switch (i - k) {
				SIMD_NODEFAULT;
			case 7: s0 += mptr[k + 6] * v[k + 6];
			case 6: s1 += mptr[k + 5] * v[k + 5];
			case 5: s2 += mptr[k + 4] * v[k + 4];
			case 4: s3 += mptr[k + 3] * v[k + 3];
			case 3: s0 += mptr[k + 2] * v[k + 2];
			case 2: s1 += mptr[k + 1] * v[k + 1];
			case 1: s2 += mptr[k + 0] * v[k + 0];
			case 0: break;
			}
			sum = s3;
			sum += s2;
			sum += s1;
			sum += s0;
			mptr[i] = (mptr[i] - sum) * d;
			mptr += nc;
		}
	}

	return true;

#else

	int i, j, k, nc;
	float *v, *ptr, *diagPtr;
	double d, sum;

	v = (float *)_alloca16(n * sizeof(float));
	nc = mat.GetNumColumns();

	for (i = 0; i < n; i++) {

		ptr = mat[i];
		diagPtr = mat[0];
		sum = ptr[i];
		for (j = 0; j < i; j++) {
			d = ptr[j];
			v[j] = diagPtr[0] * d;
			sum -= v[j] * d;
			diagPtr += nc + 1;
		}

		if (sum == 0.0f) {
			return false;
		}

		diagPtr[0] = sum;
		invDiag[i] = d = 1.0f / sum;

		if (i + 1 >= n) {
			continue;
		}

		ptr = mat[i + 1];
		for (j = i + 1; j < n; j++) {
			sum = ptr[i];
			for (k = 0; k < i; k++) {
				sum -= ptr[k] * v[k];
			}
			ptr[i] = sum * d;
			ptr += nc;
		}
	}

	return true;

#endif
}


/*
============
TransformVerts
============
*/
void SIMD_VPCALL idSIMDProcessor::TransformVerts(idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *weights, const int *index, int numWeights) {
	int i, j;
	const byte *jointsPtr = (byte *)joints;

	for (j = i = 0; i < numVerts; i++) {
		idVec3 v;

		v = (*(idJointMat *)(jointsPtr + index[j * 2 + 0])) * weights[j];
		while (index[j * 2 + 1] == 0) {
			j++;
			v += (*(idJointMat *)(jointsPtr + index[j * 2 + 0])) * weights[j];
		}
		j++;

		verts[i].xyz = v;
	}
}

/*
============
TracePointCull
============
*/
void SIMD_VPCALL idSIMDProcessor::TracePointCull(byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts) {
	int i;
	byte tOr;

	tOr = 0;

	for (i = 0; i < numVerts; i++) {
		byte bits;
		float d0, d1, d2, d3, t;
		const idVec3 &v = verts[i].xyz;

		d0 = planes[0].Distance(v);
		d1 = planes[1].Distance(v);
		d2 = planes[2].Distance(v);
		d3 = planes[3].Distance(v);

		t = d0 + radius;
		bits = FLOATSIGNBITSET(t) << 0;
		t = d1 + radius;
		bits |= FLOATSIGNBITSET(t) << 1;
		t = d2 + radius;
		bits |= FLOATSIGNBITSET(t) << 2;
		t = d3 + radius;
		bits |= FLOATSIGNBITSET(t) << 3;

		t = d0 - radius;
		bits |= FLOATSIGNBITSET(t) << 4;
		t = d1 - radius;
		bits |= FLOATSIGNBITSET(t) << 5;
		t = d2 - radius;
		bits |= FLOATSIGNBITSET(t) << 6;
		t = d3 - radius;
		bits |= FLOATSIGNBITSET(t) << 7;

		bits ^= 0x0F;		// flip lower four bits

		tOr |= bits;
		cullBits[i] = bits;
	}

	totalOr = tOr;
}

/*
============
DecalPointCull
============
*/
void SIMD_VPCALL idSIMDProcessor::DecalPointCull(byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts) {
	int i;

	for (i = 0; i < numVerts; i++) {
		byte bits;
		float d0, d1, d2, d3, d4, d5;
		const idVec3 &v = verts[i].xyz;

		d0 = planes[0].Distance(v);
		d1 = planes[1].Distance(v);
		d2 = planes[2].Distance(v);
		d3 = planes[3].Distance(v);
		d4 = planes[4].Distance(v);
		d5 = planes[5].Distance(v);

		bits = FLOATSIGNBITSET(d0) << 0;
		bits |= FLOATSIGNBITSET(d1) << 1;
		bits |= FLOATSIGNBITSET(d2) << 2;
		bits |= FLOATSIGNBITSET(d3) << 3;
		bits |= FLOATSIGNBITSET(d4) << 4;
		bits |= FLOATSIGNBITSET(d5) << 5;

		cullBits[i] = bits ^ 0x3F;		// flip lower 6 bits
	}
}

/*
============
OverlayPointCull
============
*/
void SIMD_VPCALL idSIMDProcessor::OverlayPointCull(byte *cullBits, idVec2 *texCoords, const idPlane *planes, const idDrawVert *verts, const int numVerts) {
	int i;

	for (i = 0; i < numVerts; i++) {
		byte bits;
		float d0, d1;
		const idVec3 &v = verts[i].xyz;

		texCoords[i][0] = d0 = planes[0].Distance(v);
		texCoords[i][1] = d1 = planes[1].Distance(v);

		bits = FLOATSIGNBITSET(d0) << 0;
		d0 = 1.0f - d0;
		bits |= FLOATSIGNBITSET(d1) << 1;
		d1 = 1.0f - d1;
		bits |= FLOATSIGNBITSET(d0) << 2;
		bits |= FLOATSIGNBITSET(d1) << 3;

		cullBits[i] = bits;
	}
}

/*
============
DeriveTriPlanes

Derives a plane equation for each triangle.
============
*/
void SIMD_VPCALL idSIMDProcessor::DeriveTriPlanes(idPlane *planes, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes) {
	int i;

	for (i = 0; i < numIndexes; i += 3) {
		const idDrawVert *a, *b, *c;
		float d0[3], d1[3], f;
		idVec3 n;

		a = verts + indexes[i + 0];
		b = verts + indexes[i + 1];
		c = verts + indexes[i + 2];

		d0[0] = b->xyz[0] - a->xyz[0];
		d0[1] = b->xyz[1] - a->xyz[1];
		d0[2] = b->xyz[2] - a->xyz[2];

		d1[0] = c->xyz[0] - a->xyz[0];
		d1[1] = c->xyz[1] - a->xyz[1];
		d1[2] = c->xyz[2] - a->xyz[2];

		n[0] = d1[1] * d0[2] - d1[2] * d0[1];
		n[1] = d1[2] * d0[0] - d1[0] * d0[2];
		n[2] = d1[0] * d0[1] - d1[1] * d0[0];

		f = idMath::RSqrt(n.x * n.x + n.y * n.y + n.z * n.z);

		n.x *= f;
		n.y *= f;
		n.z *= f;

		planes->SetNormal(n);
		planes->FitThroughPoint(a->xyz);
		planes++;
	}
}

/*
============
DeriveTangents

Derives the normal and orthogonal tangent vectors for the triangle vertices.
For each vertex the normal and tangent vectors are derived from all triangles
using the vertex which results in smooth tangents across the mesh.
In the process the triangle planes are calculated as well.
============
*/
#define USE_GENERIC_DERIVETANGENT 1
#define FIX_DEGENERATE_TANGENT

#if USE_GENERIC_DERIVETANGENT
void SIMD_VPCALL idSIMDProcessor::DeriveTangents(idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes)
#else
void SIMD_VPCALL DeriveTangentsTest(idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes)
#endif
{
	int i;

	bool *used = (bool *)_alloca16(numVerts * sizeof(used[0]));
	memset(used, 0, numVerts * sizeof(used[0]));

	idPlane *planesPtr = planes;
	for (i = 0; i < numIndexes; i += 3) {
		idDrawVert *a, *b, *c;
		unsigned int signBit;
		float d0[5], d1[5], f, area;
		idVec3 n, t0, t1;

		int v0 = indexes[i + 0];
		int v1 = indexes[i + 1];
		int v2 = indexes[i + 2];

		a = verts + v0;
		b = verts + v1;
		c = verts + v2;

		d0[0] = b->xyz[0] - a->xyz[0];
		d0[1] = b->xyz[1] - a->xyz[1];
		d0[2] = b->xyz[2] - a->xyz[2];
		d0[3] = b->st[0] - a->st[0];
		d0[4] = b->st[1] - a->st[1];

		d1[0] = c->xyz[0] - a->xyz[0];
		d1[1] = c->xyz[1] - a->xyz[1];
		d1[2] = c->xyz[2] - a->xyz[2];
		d1[3] = c->st[0] - a->st[0];
		d1[4] = c->st[1] - a->st[1];

		// normal
		n[0] = d1[1] * d0[2] - d1[2] * d0[1];
		n[1] = d1[2] * d0[0] - d1[0] * d0[2];
		n[2] = d1[0] * d0[1] - d1[1] * d0[0];

		f = idMath::RSqrt(n.x * n.x + n.y * n.y + n.z * n.z);

		n.x *= f;
		n.y *= f;
		n.z *= f;

		planesPtr->SetNormal(n);
		planesPtr->FitThroughPoint(a->xyz);
		planesPtr++;

		// area sign bit
		area = d0[3] * d1[4] - d0[4] * d1[3];
		signBit = (*(unsigned int *)&area) & (1 << 31);

		// first tangent
		t0[0] = d0[0] * d1[4] - d0[4] * d1[0];
		t0[1] = d0[1] * d1[4] - d0[4] * d1[1];
		t0[2] = d0[2] * d1[4] - d0[4] * d1[2];

		f = idMath::RSqrt(t0.x * t0.x + t0.y * t0.y + t0.z * t0.z);
		*(unsigned int *)&f ^= signBit;

		t0.x *= f;
		t0.y *= f;
		t0.z *= f;

		// second tangent
		t1[0] = d0[3] * d1[0] - d0[0] * d1[3];
		t1[1] = d0[3] * d1[1] - d0[1] * d1[3];
		t1[2] = d0[3] * d1[2] - d0[2] * d1[3];

		f = idMath::RSqrt(t1.x * t1.x + t1.y * t1.y + t1.z * t1.z);
		*(unsigned int *)&f ^= signBit;

		t1.x *= f;
		t1.y *= f;
		t1.z *= f;

		if (used[v0]) {
			a->normal += n;
			a->tangents[0] += t0;
			a->tangents[1] += t1;
		}
		else {
			a->normal = n;
			a->tangents[0] = t0;
			a->tangents[1] = t1;
			used[v0] = true;
		}

		if (used[v1]) {
			b->normal += n;
			b->tangents[0] += t0;
			b->tangents[1] += t1;
		}
		else {
			b->normal = n;
			b->tangents[0] = t0;
			b->tangents[1] = t1;
			used[v1] = true;
		}

		if (used[v2]) {
			c->normal += n;
			c->tangents[0] += t0;
			c->tangents[1] += t1;
		}
		else {
			c->normal = n;
			c->tangents[0] = t0;
			c->tangents[1] = t1;
			used[v2] = true;
		}
	}
}



#if !USE_GENERIC_DERIVETANGENT
#define SIMD_DERIVE_TANGENTS 1
void SIMD_VPCALL idSIMDProcessor::DeriveTangents(idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes)
#else
void SIMD_VPCALL DeriveTangentsTest(idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes)
#endif
{

#if defined(_DEBUG)
#pragma warning( push )
#pragma warning( disable : 4311 )
#pragma warning( push )
#pragma warning( disable : 4302 )
	idDrawVert testVert;
	assert(sizeof(idDrawVert) == DRAWVERT_SIZE);
	ulong ptr0 = (ulong)(&testVert);
	ulong ptr20 = (ulong)(&testVert.normal) - ptr0;
	ulong ptr32 = (ulong)(&testVert.tangents[0])- ptr0;
	ulong ptr44 = (ulong)(&testVert.tangents[1]) - ptr0;
	assert(ptr20 == DRAWVERT_NORMAL_OFFSET);
	assert(ptr32 == DRAWVERT_TANGENT0_OFFSET);
	assert(ptr44 == DRAWVERT_TANGENT1_OFFSET);

	assert(planes != NULL);
	assert(verts != NULL);
	assert(numVerts >= 0);
#pragma warning( pop )
#pragma warning( pop )
#endif

	bool* used = (bool*)_alloca16(numVerts * sizeof(used[0]));
	memset(used, 0, numVerts * sizeof(used[0]));

	int i;
	for ( i = 0; i <= numIndexes - 12; i += 12) {
		idDrawVert* a, * b, * c;
		ALIGN16(unsigned int signBit[4]);
		ALIGN16(float d0[4]);
		ALIGN16(float d1[4]);
		ALIGN16(float d2[4]);
		ALIGN16(float d3[4]);
		ALIGN16(float d4[4]);
		ALIGN16(float d5[4]);
		ALIGN16(float d6[4]);
		ALIGN16(float d7[4]);
		ALIGN16(float d8[4]);
		ALIGN16(float d9[4]);
		ALIGN16(float n0[4]);
		ALIGN16(float n1[4]);
		ALIGN16(float n2[4]);
		ALIGN16(float t0[4]);
		ALIGN16(float t1[4]);
		ALIGN16(float t2[4]);
		ALIGN16(float t3[4]);
		ALIGN16(float t4[4]);
		ALIGN16(float t5[4]);

#if SIMD_DERIVE_TANGENTS

		ALIGN16(float v0[4]);
		ALIGN16(float v1[4]);
		//ALIGN16(float v2[4]);
		//ALIGN16(float v3[4]);
		for (int j = 0; j < 4; j++) {

			a = verts + indexes[i + j * 3 + 0];
			b = verts + indexes[i + j * 3 + 1];
			c = verts + indexes[i + j * 3 + 2];

			__m128 b_xyz = _mm_load_ps((float *)&b->xyz);
			__m128 a_xyz = _mm_load_ps((float *)&a->xyz);

			__m128 v_xyz = _mm_sub_ps(b_xyz, a_xyz);

			_mm_store_ps(v0, v_xyz);

			__m128 b_st = _mm_load_ps((float *)&b->st);
			__m128 a_st = _mm_load_ps((float *)&a->st);

			__m128 v_st = _mm_sub_ps(b_st,a_st);
			_mm_store_ps(v1, v_st);

			d0[j] = v0[0];
			d1[j] = v0[1];
			d2[j] = v0[2];
			d3[j] = v1[0];
			d4[j] = v1[1];

			__m128 c_xyz = _mm_load_ps((float *)&c->xyz);
			__m128 c_st = _mm_load_ps((float *)&c->st);

			v_xyz = _mm_sub_ps(c_xyz, a_xyz);
			_mm_store_ps(v0, v_xyz);

			v_st = _mm_sub_ps(c_st, a_st);
			_mm_store_ps(v1, v_st);

			d5[j] = v0[0];
			d6[j] = v0[1];
			d7[j] = v0[2];
			d8[j] = v1[0];
			d9[j] = v1[1];
		}
#else
		for (int j = 0; j < 4; j++) {

			a = verts + indexes[i + j * 3 + 0];
			b = verts + indexes[i + j * 3 + 1];
			c = verts + indexes[i + j * 3 + 2];

			d0[j] = b->xyz[0] - a->xyz[0];
			d1[j] = b->xyz[1] - a->xyz[1];
			d2[j] = b->xyz[2] - a->xyz[2];
			d3[j] = b->st[0] - a->st[0];
			d4[j] = b->st[1] - a->st[1];

			d5[j] = c->xyz[0] - a->xyz[0];
			d6[j] = c->xyz[1] - a->xyz[1];
			d7[j] = c->xyz[2] - a->xyz[2];
			d8[j] = c->st[0] - a->st[0];
			d9[j] = c->st[1] - a->st[1];
		}
#endif


		ALIGN16(float tmp[4]);


		// normal
		n0[0] = d6[0] * d2[0];
		n0[1] = d6[1] * d2[1];
		n0[2] = d6[2] * d2[2];
		n0[3] = d6[3] * d2[3];

		n0[0] -= d7[0] * d1[0];
		n0[1] -= d7[1] * d1[1];
		n0[2] -= d7[2] * d1[2];
		n0[3] -= d7[3] * d1[3];

		n1[0] = d7[0] * d0[0];
		n1[1] = d7[1] * d0[1];
		n1[2] = d7[2] * d0[2];
		n1[3] = d7[3] * d0[3];

		n1[0] -= d5[0] * d2[0];
		n1[1] -= d5[1] * d2[1];
		n1[2] -= d5[2] * d2[2];
		n1[3] -= d5[3] * d2[3];

		n2[0] = d5[0] * d1[0];
		n2[1] = d5[1] * d1[1];
		n2[2] = d5[2] * d1[2];
		n2[3] = d5[3] * d1[3];

		n2[0] -= d6[0] * d0[0];
		n2[1] -= d6[1] * d0[1];
		n2[2] -= d6[2] * d0[2];
		n2[3] -= d6[3] * d0[3];

		tmp[0] = n0[0] * n0[0];
		tmp[1] = n0[1] * n0[1];
		tmp[2] = n0[2] * n0[2];
		tmp[3] = n0[3] * n0[3];

		tmp[0] += n1[0] * n1[0];
		tmp[1] += n1[1] * n1[1];
		tmp[2] += n1[2] * n1[2];
		tmp[3] += n1[3] * n1[3];

		tmp[0] += n2[0] * n2[0];
		tmp[1] += n2[1] * n2[1];
		tmp[2] += n2[2] * n2[2];
		tmp[3] += n2[3] * n2[3];

		tmp[0] = idMath::RSqrt(tmp[0]);
		tmp[1] = idMath::RSqrt(tmp[1]);
		tmp[2] = idMath::RSqrt(tmp[2]);
		tmp[3] = idMath::RSqrt(tmp[3]);

		n0[0] *= tmp[0];
		n0[1] *= tmp[1];
		n0[2] *= tmp[2];
		n0[3] *= tmp[3];

		n1[0] *= tmp[0];
		n1[1] *= tmp[1];
		n1[2] *= tmp[2];
		n1[3] *= tmp[3];

		n2[0] *= tmp[0];
		n2[1] *= tmp[1];
		n2[2] *= tmp[2];
		n2[3] *= tmp[3];

		// area sign bit
		tmp[0] = d3[0] * d9[0];
		tmp[1] = d3[1] * d9[1];
		tmp[2] = d3[2] * d9[2];
		tmp[3] = d3[3] * d9[3];

		tmp[0] -= d4[0] * d8[0];
		tmp[1] -= d4[1] * d8[1];
		tmp[2] -= d4[2] * d8[2];
		tmp[3] -= d4[3] * d8[3];

		signBit[0] = (*(unsigned int*)&tmp[0]) & (1 << 31);
		signBit[1] = (*(unsigned int*)&tmp[1]) & (1 << 31);
		signBit[2] = (*(unsigned int*)&tmp[2]) & (1 << 31);
		signBit[3] = (*(unsigned int*)&tmp[3]) & (1 << 31);

		// first tangent
		t0[0] = d0[0] * d9[0];
		t0[1] = d0[1] * d9[1];
		t0[2] = d0[2] * d9[2];
		t0[3] = d0[3] * d9[3];

		t0[0] -= d4[0] * d5[0];
		t0[1] -= d4[1] * d5[1];
		t0[2] -= d4[2] * d5[2];
		t0[3] -= d4[3] * d5[3];

		t1[0] = d1[0] * d9[0];
		t1[1] = d1[1] * d9[1];
		t1[2] = d1[2] * d9[2];
		t1[3] = d1[3] * d9[3];

		t1[0] -= d4[0] * d6[0];
		t1[1] -= d4[1] * d6[1];
		t1[2] -= d4[2] * d6[2];
		t1[3] -= d4[3] * d6[3];

		t2[0] = d2[0] * d9[0];
		t2[1] = d2[1] * d9[1];
		t2[2] = d2[2] * d9[2];
		t2[3] = d2[3] * d9[3];

		t2[0] -= d4[0] * d7[0];
		t2[1] -= d4[1] * d7[1];
		t2[2] -= d4[2] * d7[2];
		t2[3] -= d4[3] * d7[3];

		tmp[0] = t0[0] * t0[0];
		tmp[1] = t0[1] * t0[1];
		tmp[2] = t0[2] * t0[2];
		tmp[3] = t0[3] * t0[3];

		tmp[0] += t1[0] * t1[0];
		tmp[1] += t1[1] * t1[1];
		tmp[2] += t1[2] * t1[2];
		tmp[3] += t1[3] * t1[3];

		tmp[0] += t2[0] * t2[0];
		tmp[1] += t2[1] * t2[1];
		tmp[2] += t2[2] * t2[2];
		tmp[3] += t2[3] * t2[3];

		tmp[0] = idMath::RSqrt(tmp[0]);
		tmp[1] = idMath::RSqrt(tmp[1]);
		tmp[2] = idMath::RSqrt(tmp[2]);
		tmp[3] = idMath::RSqrt(tmp[3]);

		*(unsigned int*)&tmp[0] ^= signBit[0];
		*(unsigned int*)&tmp[1] ^= signBit[1];
		*(unsigned int*)&tmp[2] ^= signBit[2];
		*(unsigned int*)&tmp[3] ^= signBit[3];

		t0[0] *= tmp[0];
		t0[1] *= tmp[1];
		t0[2] *= tmp[2];
		t0[3] *= tmp[3];

		t1[0] *= tmp[0];
		t1[1] *= tmp[1];
		t1[2] *= tmp[2];
		t1[3] *= tmp[3];

		t2[0] *= tmp[0];
		t2[1] *= tmp[1];
		t2[2] *= tmp[2];
		t2[3] *= tmp[3];

		// second tangent
		t3[0] = d3[0] * d5[0];
		t3[1] = d3[1] * d5[1];
		t3[2] = d3[2] * d5[2];
		t3[3] = d3[3] * d5[3];

		t3[0] -= d0[0] * d8[0];
		t3[1] -= d0[1] * d8[1];
		t3[2] -= d0[2] * d8[2];
		t3[3] -= d0[3] * d8[3];

		t4[0] = d3[0] * d6[0];
		t4[1] = d3[1] * d6[1];
		t4[2] = d3[2] * d6[2];
		t4[3] = d3[3] * d6[3];

		t4[0] -= d1[0] * d8[0];
		t4[1] -= d1[1] * d8[1];
		t4[2] -= d1[2] * d8[2];
		t4[3] -= d1[3] * d8[3];

		t5[0] = d3[0] * d7[0];
		t5[1] = d3[1] * d7[1];
		t5[2] = d3[2] * d7[2];
		t5[3] = d3[3] * d7[3];

		t5[0] -= d2[0] * d8[0];
		t5[1] -= d2[1] * d8[1];
		t5[2] -= d2[2] * d8[2];
		t5[3] -= d2[3] * d8[3];

		tmp[0] = t3[0] * t3[0];
		tmp[1] = t3[1] * t3[1];
		tmp[2] = t3[2] * t3[2];
		tmp[3] = t3[3] * t3[3];

		tmp[0] += t4[0] * t4[0];
		tmp[1] += t4[1] * t4[1];
		tmp[2] += t4[2] * t4[2];
		tmp[3] += t4[3] * t4[3];

		tmp[0] += t5[0] * t5[0];
		tmp[1] += t5[1] * t5[1];
		tmp[2] += t5[2] * t5[2];
		tmp[3] += t5[3] * t5[3];

		tmp[0] = idMath::RSqrt(tmp[0]);
		tmp[1] = idMath::RSqrt(tmp[1]);
		tmp[2] = idMath::RSqrt(tmp[2]);
		tmp[3] = idMath::RSqrt(tmp[3]);

		*(unsigned int*)&tmp[0] ^= signBit[0];
		*(unsigned int*)&tmp[1] ^= signBit[1];
		*(unsigned int*)&tmp[2] ^= signBit[2];
		*(unsigned int*)&tmp[3] ^= signBit[3];

		t3[0] *= tmp[0];
		t3[1] *= tmp[1];
		t3[2] *= tmp[2];
		t3[3] *= tmp[3];

		t4[0] *= tmp[0];
		t4[1] *= tmp[1];
		t4[2] *= tmp[2];
		t4[3] *= tmp[3];

		t5[0] *= tmp[0];
		t5[1] *= tmp[1];
		t5[2] *= tmp[2];
		t5[3] *= tmp[3];

		for (int j = 0; j < 4; j++) {

			const int v0 = indexes[i + j * 3 + 0];
			const int v1 = indexes[i + j * 3 + 1];
			const int v2 = indexes[i + j * 3 + 2];

			a = verts + v0;
			b = verts + v1;
			c = verts + v2;

			planes->Normal()[0] = n0[j];
			planes->Normal()[1] = n1[j];
			planes->Normal()[2] = n2[j];
			planes->FitThroughPoint(a->xyz);
			planes++;

			if (used[v0]) {
				a->normal[0] += n0[j];
				a->normal[1] += n1[j];
				a->normal[2] += n2[j];

				a->tangents[0][0] += t0[j];
				a->tangents[0][1] += t1[j];
				a->tangents[0][2] += t2[j];

				a->tangents[1][0] += t3[j];
				a->tangents[1][1] += t4[j];
				a->tangents[1][2] += t5[j];
			}
			else {
				a->normal[0] = n0[j];
				a->normal[1] = n1[j];
				a->normal[2] = n2[j];

				a->tangents[0][0] = t0[j];
				a->tangents[0][1] = t1[j];
				a->tangents[0][2] = t2[j];

				a->tangents[1][0] = t3[j];
				a->tangents[1][1] = t4[j];
				a->tangents[1][2] = t5[j];

				used[v0] = true;
			}

			if (used[v1]) {
				b->normal[0] += n0[j];
				b->normal[1] += n1[j];
				b->normal[2] += n2[j];

				b->tangents[0][0] += t0[j];
				b->tangents[0][1] += t1[j];
				b->tangents[0][2] += t2[j];

				b->tangents[1][0] += t3[j];
				b->tangents[1][1] += t4[j];
				b->tangents[1][2] += t5[j];
			}
			else {
				b->normal[0] = n0[j];
				b->normal[1] = n1[j];
				b->normal[2] = n2[j];

				b->tangents[0][0] = t0[j];
				b->tangents[0][1] = t1[j];
				b->tangents[0][2] = t2[j];

				b->tangents[1][0] = t3[j];
				b->tangents[1][1] = t4[j];
				b->tangents[1][2] = t5[j];

				used[v1] = true;
			}

			if (used[v2]) {
				c->normal[0] += n0[j];
				c->normal[1] += n1[j];
				c->normal[2] += n2[j];

				c->tangents[0][0] += t0[j];
				c->tangents[0][1] += t1[j];
				c->tangents[0][2] += t2[j];

				c->tangents[1][0] += t3[j];
				c->tangents[1][1] += t4[j];
				c->tangents[1][2] += t5[j];
			}
			else {
				c->normal[0] = n0[j];
				c->normal[1] = n1[j];
				c->normal[2] = n2[j];

				c->tangents[0][0] = t0[j];
				c->tangents[0][1] = t1[j];
				c->tangents[0][2] = t2[j];

				c->tangents[1][0] = t3[j];
				c->tangents[1][1] = t4[j];
				c->tangents[1][2] = t5[j];

				used[v2] = true;
			}
		}
	}

	for (; i < numIndexes; i += 3) {
		idDrawVert* a, * b, * c;
		ALIGN16(unsigned int signBit[4]);
		float d0, d1, d2, d3, d4;
		float d5, d6, d7, d8, d9;
		float n0, n1, n2;
		float t0, t1, t2;
		float t3, t4, t5;

		const int v0 = indexes[i + 0];
		const int v1 = indexes[i + 1];
		const int v2 = indexes[i + 2];

		a = verts + v0;
		b = verts + v1;
		c = verts + v2;

		d0 = b->xyz[0] - a->xyz[0];
		d1 = b->xyz[1] - a->xyz[1];
		d2 = b->xyz[2] - a->xyz[2];
		d3 = b->st[0] - a->st[0];
		d4 = b->st[1] - a->st[1];

		d5 = c->xyz[0] - a->xyz[0];
		d6 = c->xyz[1] - a->xyz[1];
		d7 = c->xyz[2] - a->xyz[2];
		d8 = c->st[0] - a->st[0];
		d9 = c->st[1] - a->st[1];


		float tmp;

		// normal
		n0 = d6 * d2 - d7 * d1;
		n1 = d7 * d0 - d5 * d2;
		n2 = d5 * d1 - d6 * d0;

		tmp = idMath::RSqrt(n0 * n0 + n1 * n1 + n2 * n2);

		n0 *= tmp;
		n1 *= tmp;
		n2 *= tmp;

		// area sign bit
		tmp = d3 * d9 - d4 * d8;
		signBit[0] = (*(unsigned int*)&tmp) & (1 << 31);

		// first tangent
		t0 = d0 * d9 - d4 * d5;
		t1 = d1 * d9 - d4 * d6;
		t2 = d2 * d9 - d4 * d7;

		tmp = idMath::RSqrt(t0 * t0 + t1 * t1 + t2 * t2);
		*(unsigned int*)&tmp ^= signBit[0];

		t0 *= tmp;
		t1 *= tmp;
		t2 *= tmp;

		// second tangent
		t3 = d3 * d5 - d0 * d8;
		t4 = d3 * d6 - d1 * d8;
		t5 = d3 * d7 - d2 * d8;

		tmp = idMath::RSqrt(t3 * t3 + t4 * t4 + t5 * t5);
		*(unsigned int*)&tmp ^= signBit[0];

		t3 *= tmp;
		t4 *= tmp;
		t5 *= tmp;


		planes->Normal()[0] = n0;
		planes->Normal()[1] = n1;
		planes->Normal()[2] = n2;
		planes->FitThroughPoint(a->xyz);
		planes++;

		if (used[v0]) {
			a->normal[0] += n0;
			a->normal[1] += n1;
			a->normal[2] += n2;

			a->tangents[0][0] += t0;
			a->tangents[0][1] += t1;
			a->tangents[0][2] += t2;

			a->tangents[1][0] += t3;
			a->tangents[1][1] += t4;
			a->tangents[1][2] += t5;
		}
		else {
			a->normal[0] = n0;
			a->normal[1] = n1;
			a->normal[2] = n2;

			a->tangents[0][0] = t0;
			a->tangents[0][1] = t1;
			a->tangents[0][2] = t2;

			a->tangents[1][0] = t3;
			a->tangents[1][1] = t4;
			a->tangents[1][2] = t5;

			used[v0] = true;
		}

		if (used[v1]) {
			b->normal[0] += n0;
			b->normal[1] += n1;
			b->normal[2] += n2;

			b->tangents[0][0] += t0;
			b->tangents[0][1] += t1;
			b->tangents[0][2] += t2;

			b->tangents[1][0] += t3;
			b->tangents[1][1] += t4;
			b->tangents[1][2] += t5;
		}
		else {
			b->normal[0] = n0;
			b->normal[1] = n1;
			b->normal[2] = n2;

			b->tangents[0][0] = t0;
			b->tangents[0][1] = t1;
			b->tangents[0][2] = t2;

			b->tangents[1][0] = t3;
			b->tangents[1][1] = t4;
			b->tangents[1][2] = t5;

			used[v1] = true;
		}

		if (used[v2]) {
			c->normal[0] += n0;
			c->normal[1] += n1;
			c->normal[2] += n2;

			c->tangents[0][0] += t0;
			c->tangents[0][1] += t1;
			c->tangents[0][2] += t2;

			c->tangents[1][0] += t3;
			c->tangents[1][1] += t4;
			c->tangents[1][2] += t5;
		}
		else {
			c->normal[0] = n0;
			c->normal[1] = n1;
			c->normal[2] = n2;

			c->tangents[0][0] = t0;
			c->tangents[0][1] = t1;
			c->tangents[0][2] = t2;

			c->tangents[1][0] = t3;
			c->tangents[1][1] = t4;
			c->tangents[1][2] = t5;

			used[v2] = true;
		}
	}
}

/*
============
DeriveUnsmoothedTangents

Derives the normal and orthogonal tangent vectors for the triangle vertices.
For each vertex the normal and tangent vectors are derived from a single dominant triangle.
============
*/
#define DERIVE_UNSMOOTHED_BITANGENT

void SIMD_VPCALL idSIMDProcessor::DeriveUnsmoothedTangents(idDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts) {
	int i;

	for (i = 0; i < numVerts; i++) {
		idDrawVert *a, *b, *c;
#ifndef DERIVE_UNSMOOTHED_BITANGENT
		float d3, d8;
#endif
		float d0, d1, d2, d4;
		float d5, d6, d7, d9;
		float s0, s1, s2;
		float n0, n1, n2;
		float t0, t1, t2;
		float t3, t4, t5;

		const dominantTri_s &dt = dominantTris[i];

		a = verts + i;
		b = verts + dt.v2;
		c = verts + dt.v3;

		d0 = b->xyz[0] - a->xyz[0];
		d1 = b->xyz[1] - a->xyz[1];
		d2 = b->xyz[2] - a->xyz[2];
#ifndef DERIVE_UNSMOOTHED_BITANGENT
		d3 = b->st[0] - a->st[0];
#endif
		d4 = b->st[1] - a->st[1];

		d5 = c->xyz[0] - a->xyz[0];
		d6 = c->xyz[1] - a->xyz[1];
		d7 = c->xyz[2] - a->xyz[2];
#ifndef DERIVE_UNSMOOTHED_BITANGENT
		d8 = c->st[0] - a->st[0];
#endif
		d9 = c->st[1] - a->st[1];

		s0 = dt.normalizationScale[0];
		s1 = dt.normalizationScale[1];
		s2 = dt.normalizationScale[2];

		n0 = s2 * (d6 * d2 - d7 * d1);
		n1 = s2 * (d7 * d0 - d5 * d2);
		n2 = s2 * (d5 * d1 - d6 * d0);

		t0 = s0 * (d0 * d9 - d4 * d5);
		t1 = s0 * (d1 * d9 - d4 * d6);
		t2 = s0 * (d2 * d9 - d4 * d7);

#ifndef DERIVE_UNSMOOTHED_BITANGENT
		t3 = s1 * (d3 * d5 - d0 * d8);
		t4 = s1 * (d3 * d6 - d1 * d8);
		t5 = s1 * (d3 * d7 - d2 * d8);
#else
		t3 = s1 * (n2 * t1 - n1 * t2);
		t4 = s1 * (n0 * t2 - n2 * t0);
		t5 = s1 * (n1 * t0 - n0 * t1);
#endif

		a->normal[0] = n0;
		a->normal[1] = n1;
		a->normal[2] = n2;

		a->tangents[0][0] = t0;
		a->tangents[0][1] = t1;
		a->tangents[0][2] = t2;

		a->tangents[1][0] = t3;
		a->tangents[1][1] = t4;
		a->tangents[1][2] = t5;
	}
}

/*
============
NormalizeTangents

Normalizes each vertex normal and projects and normalizes the
tangent vectors onto the plane orthogonal to the vertex normal.
============
*/
void SIMD_VPCALL idSIMDProcessor::NormalizeTangents(idDrawVert *verts, const int numVerts) {

	for (int i = 0; i < numVerts; i++) {
		idVec3 &v = verts[i].normal;
		float f;

		f = idMath::RSqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		v.x *= f; v.y *= f; v.z *= f;

		for (int j = 0; j < 2; j++) {
			idVec3 &t = verts[i].tangents[j];

			t -= (t * v) * v;
			f = idMath::RSqrt(t.x * t.x + t.y * t.y + t.z * t.z);
			t.x *= f; t.y *= f; t.z *= f;
		}
	}
}

/*
============
CreateTextureSpaceLightVectors

Calculates light vectors in texture space for the given triangle vertices.
For each vertex the direction towards the light origin is projected onto texture space.
The light vectors are only calculated for the vertices referenced by the indexes.
============
*/
void SIMD_VPCALL idSIMDProcessor::CreateTextureSpaceLightVectors(idVec3 *lightVectors, const idVec3 &lightOrigin, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes) {

	bool *used = (bool *)_alloca16(numVerts * sizeof(used[0]));
	memset(used, 0, numVerts * sizeof(used[0]));

	for (int i = numIndexes - 1; i >= 0; i--) {
		used[indexes[i]] = true;
	}

	for (int i = 0; i < numVerts; i++) {
		if (!used[i]) {
			continue;
		}

		const idDrawVert *v = &verts[i];

		idVec3 lightDir = lightOrigin - v->xyz;

		lightVectors[i][0] = lightDir * v->tangents[0];
		lightVectors[i][1] = lightDir * v->tangents[1];
		lightVectors[i][2] = lightDir * v->normal;
	}
}

/*
============
CreateSpecularTextureCoords

Calculates specular texture coordinates for the given triangle vertices.
For each vertex the normalized direction towards the light origin is added to the
normalized direction towards the view origin and the result is projected onto texture space.
The texture coordinates are only calculated for the vertices referenced by the indexes.
============
*/
void SIMD_VPCALL idSIMDProcessor::CreateSpecularTextureCoords(idVec4 *texCoords, const idVec3 &lightOrigin, const idVec3 &viewOrigin, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes) {

	bool *used = (bool *)_alloca16(numVerts * sizeof(used[0]));
	memset(used, 0, numVerts * sizeof(used[0]));

	for (int i = numIndexes - 1; i >= 0; i--) {
		used[indexes[i]] = true;
	}

	for (int i = 0; i < numVerts; i++) {
		if (!used[i]) {
			continue;
		}

		const idDrawVert *v = &verts[i];

		idVec3 lightDir = lightOrigin - v->xyz;
		idVec3 viewDir = viewOrigin - v->xyz;

		float ilength;

		ilength = idMath::RSqrt(lightDir * lightDir);
		lightDir[0] *= ilength;
		lightDir[1] *= ilength;
		lightDir[2] *= ilength;

		ilength = idMath::RSqrt(viewDir * viewDir);
		viewDir[0] *= ilength;
		viewDir[1] *= ilength;
		viewDir[2] *= ilength;

		lightDir += viewDir;

		texCoords[i][0] = lightDir * v->tangents[0];
		texCoords[i][1] = lightDir * v->tangents[1];
		texCoords[i][2] = lightDir * v->normal;
		texCoords[i][3] = 1.0f;
	}
}

/*
============
CreateShadowCache
============
*/
int SIMD_VPCALL idSIMDProcessor::CreateShadowCache(idVec4 *vertexCache, int *vertRemap, const idVec3 &lightOrigin, const idDrawVert *verts, const int numVerts) {
	int outVerts = 0;

	for (int i = 0; i < numVerts; i++) {
		if (vertRemap[i]) {
			continue;
		}
		const float *v = verts[i].xyz.ToFloatPtr();
		vertexCache[outVerts + 0][0] = v[0];
		vertexCache[outVerts + 0][1] = v[1];
		vertexCache[outVerts + 0][2] = v[2];
		vertexCache[outVerts + 0][3] = 1.0f;

		// R_SetupProjection() builds the projection matrix with a slight crunch
		// for depth, which keeps this w=0 division from rasterizing right at the
		// wrap around point and causing depth fighting with the rear caps
		vertexCache[outVerts + 1][0] = v[0] - lightOrigin[0];
		vertexCache[outVerts + 1][1] = v[1] - lightOrigin[1];
		vertexCache[outVerts + 1][2] = v[2] - lightOrigin[2];
		vertexCache[outVerts + 1][3] = 0.0f;
		vertRemap[i] = outVerts;
		outVerts += 2;
	}
	return outVerts;
}

/*
============
CreateVertexProgramShadowCache
============
*/
int SIMD_VPCALL idSIMDProcessor::CreateVertexProgramShadowCache(idVec4 *vertexCache, const idDrawVert *verts, const int numVerts) {
	for (int i = 0; i < numVerts; i++) {
		const float *v = verts[i].xyz.ToFloatPtr();
		vertexCache[i * 2 + 0][0] = v[0];
		vertexCache[i * 2 + 1][0] = v[0];
		vertexCache[i * 2 + 0][1] = v[1];
		vertexCache[i * 2 + 1][1] = v[1];
		vertexCache[i * 2 + 0][2] = v[2];
		vertexCache[i * 2 + 1][2] = v[2];
		vertexCache[i * 2 + 0][3] = 1.0f;
		vertexCache[i * 2 + 1][3] = 0.0f;
	}
	return numVerts * 2;
}

/*
============
idSIMDProcessor::CmpLT

dst[i] |= ( src0[i] < constant ) << bitNum;
============
*/
void SIMD_VPCALL idSIMDProcessor::CmpLT(byte *dst, const byte bitNum, const float *src0, const float constant, const int count)
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
	if (ptrdiff_t(src0) & 3) {
		/* unaligned memory access */
		pre = 0;
		cnt = count >> 2;
		post = count - (cnt << 2);

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
			src0_p = (char *)src0;
			_mm_prefetch(src0_p + 64, _MM_HINT_NTA);
			constant_p = (char *)&constant;
			xmm1 = _mm_load_ss((float *)constant_p);
			xmm1 = _mm_shuffle_ps(xmm1, xmm1, SIMD_R_SHUFFLEPS(0, 0, 0, 0));
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
				xmm0 = _mm_loadu_ps((float *)src0_p);
				_mm_prefetch(src0_p + 128, _MM_HINT_NTA);
				xmm0 = _mm_cmplt_ps(xmm0, xmm1);
				// Simplify using SSE2
				xmm0i = _mm_castps_si128(xmm0);
				xmm0i = _mm_packs_epi32(xmm0i, xmm0i);
				xmm0i = _mm_packs_epi16(xmm0i, xmm0i);
				mask_l = _mm_cvtsi128_si32(xmm0i);
				// End
				mask_l = mask_l & 0x01010101;
				mask_l = mask_l << bitNum;
				dst_l = *((int *)dst_p);
				mask_l = mask_l | dst_l;
				*((int *)dst_p) = mask_l;
				src0_p = src0_p + 16;
				dst_p = dst_p + 4;
				cnt_l = cnt_l + 1;
			} while (cnt_l < 0);
		}
	}
	else {
		/* aligned memory access */
		aligned = (float *)((ptrdiff_t(src0) + 15) & ~15);
		if (ptrdiff_t(aligned) > ptrdiff_t(src0) + count) {
			pre = count;
			post = 0;
		}
		else {
			pre = aligned - src0;
			cnt = (count - pre) >> 2;
			post = count - pre - (cnt << 2);
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
				src0_p = (char *)src0;
				_mm_prefetch(src0_p + 64, _MM_HINT_NTA);
				constant_p = (char *)&constant;
				xmm1 = _mm_load_ss((float *)constant_p);
				xmm1 = _mm_shuffle_ps(xmm1, xmm1, SIMD_R_SHUFFLEPS(0, 0, 0, 0));
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
					xmm0 = _mm_load_ps((float *)src0_p);
					_mm_prefetch(src0_p + 128, _MM_HINT_NTA);
					xmm0 = _mm_cmplt_ps(xmm0, xmm1);
					// Simplify using SSE2
					xmm0i = _mm_castps_si128(xmm0);
					xmm0i = _mm_packs_epi32(xmm0i, xmm0i);
					xmm0i = _mm_packs_epi16(xmm0i, xmm0i);
					mask_l = _mm_cvtsi128_si32(xmm0i);
					// End
					mask_l = mask_l & 0x01010101;
					mask_l = mask_l << bitNum;
					dst_l = *((int *)dst_p);
					mask_l = mask_l | dst_l;
					*((int *)dst_p) = mask_l;
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


#endif // __BLENDO_SIMD_INLINE__