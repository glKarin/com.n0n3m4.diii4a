
#ifndef __MATH_SIMD_BLENDO_H__
#define __MATH_SIMD_BLENDO_H__

/*
===========================================================================
Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans
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

#include "idlib/math/Simd.h"
#include "idlib/math/Math.h"

#include <immintrin.h>

#ifdef _WIN32
#define SIMD_VPCALL __fastcall
#else
#define SIMD_VPCALL
#endif

#define SIMD_INLINE						inline
#define SIMD_FORCE_INLINE				__forceinline
#define SIMD_INLINE_EXTERN				static inline //extern inline
#define SIMD_FORCE_INLINE_EXTERN		static __forceinline //	extern __forceinline

#define SIMD_PI	3.14159265358979323846f

#define SIMD_VERT_SIZE				60
#define SIMD_VERT_XYZ_OFFSET		(0*4)
#define SIMD_VERT_ST_OFFSET			(3*4)
#define SIMD_VERT_NORMAL_OFFSET		(5*4)
#define SIMD_VERT_TANGENT0_OFFSET	(8*4)
#define SIMD_VERT_TANGENT1_OFFSET	(11*4)
#define SIMD_VERT_COLOR_OFFSET		(14*4)

#define SHUFFLEPD( x, y )			(( (x) & 1 ) << 1 | ( (y) & 1 ))
#define R_SHUFFLEPD( x, y )			(( (y) & 1 ) << 1 | ( (x) & 1 ))

#define SIMD_SWAP( a, b ) {auto tempswapvar = a; a = b; b = tempswapvar;}
#define SIMD_SHUFFLEPS( x, y, z, w ) (( (x) & 3 ) << 6 | ( (y) & 3 ) << 4 | ( (z) & 3 ) << 2 | ( (w) & 3 ))
#define SIMD_R_SHUFFLEPS( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))
#define SIMD_UNROLL1(Y) { int _IX; for (_IX=0;_IX<count;_IX++) {Y(_IX);} }
#define SIMD_UNROLL2(Y) { int _IX, _NM = count&0xfffffffe; for (_IX=0;_IX<_NM;_IX+=2){Y(_IX+0);Y(_IX+1);} if (_IX < count) {Y(_IX);}}
#define SIMD_UNROLL4(Y) { int _IX, _NM = count&0xfffffffc; for (_IX=0;_IX<_NM;_IX+=4){Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);}for(;_IX<count;_IX++){Y(_IX);}}
#define SIMD_UNROLL8(Y) { int _IX, _NM = count&0xfffffff8; for (_IX=0;_IX<_NM;_IX+=8){Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);Y(_IX+4);Y(_IX+5);Y(_IX+6);Y(_IX+7);} _NM = count&0xfffffffe; for(;_IX<_NM;_IX+=2){Y(_IX); Y(_IX+1);} if (_IX < count) {Y(_IX);} }
#define SIMD_R_SHUFFLE_D( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))

#ifdef _DEBUG
#define SIMD_NODEFAULT	default: assert( 0 )
#elif _MSC_VER
#define SIMD_NODEFAULT	default: __assume( 0 )
#else
#define SIMD_NODEFAULT
#endif

#define assert_2_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) &  1 ) == 0 )
#define assert_4_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) &  3 ) == 0 )
#define assert_8_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) &  7 ) == 0 )
#define assert_16_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) & 15 ) == 0 )
#define assert_32_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) & 31 ) == 0 )
#define assert_64_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) & 63 ) == 0 )
#define assert_128_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) & 127 ) == 0 )
#define assert_aligned_to_type_size( ptr )	assert( ( ((UINT_PTR)(ptr)) & ( sizeof( (ptr)[0] ) - 1 ) ) == 0 )

// doom bfg sys intrinsics
#ifndef __SYS_INTRIINSICS_H__
#define __SYS_INTRIINSICS_H__

/*
================================================================================================
Scalar single precision floating-point intrinsics
================================================================================================
*/
#define ID_WIN_X86_SSE2_INTRIN

SIMD_INLINE_EXTERN float SIMD_fmuls(float a, float b) { return (a * b); }
SIMD_INLINE_EXTERN float SIMD_fmadds(float a, float b, float c) { return (a * b + c); }
SIMD_INLINE_EXTERN float SIMD_fnmsubs(float a, float b, float c) { return (c - a * b); }
SIMD_INLINE_EXTERN float SIMD_fsels(float a, float b, float c) { return (a >= 0.0f) ? b : c; }
SIMD_INLINE_EXTERN float SIMD_frcps(float x) { return (1.0f / x); }
SIMD_INLINE_EXTERN float SIMD_fdivs(float x, float y) { return (x / y); }
SIMD_INLINE_EXTERN float SIMD_frsqrts(float x) { return (1.0f / sqrtf(x)); }
SIMD_INLINE_EXTERN float SIMD_frcps16(float x) { return (1.0f / x); }
SIMD_INLINE_EXTERN float SIMD_fdivs16(float x, float y) { return (x / y); }
SIMD_INLINE_EXTERN float SIMD_frsqrts16(float x) { return (1.0f / sqrtf(x)); }
SIMD_INLINE_EXTERN float SIMD_frndz(float x) { return (float)((int)(x)); }

/*
================================================================================================
Zero cache line and prefetch intrinsics
================================================================================================
*/

#ifdef ID_WIN_X86_SSE2_INTRIN

// The code below assumes that a cache line is 64 bytes.
// We specify the cache line size as 128 here to make the code consistent with the consoles.
#define CACHE_LINE_SIZE						128

SIMD_FORCE_INLINE void Prefetch(const void * ptr, int offset) {
		const char * bytePtr = ( (const char *) ptr ) + offset;
		_mm_prefetch( bytePtr +  0, _MM_HINT_NTA );
		//_mm_prefetch( bytePtr + 64, _MM_HINT_NTA );
}
SIMD_FORCE_INLINE void ZeroCacheLine(void * ptr, int offset) {
	assert_128_byte_aligned(ptr);
	char * bytePtr = ((char *)ptr) + offset;
	__m128i zero = _mm_setzero_si128();
	_mm_store_si128((__m128i *) (bytePtr + 0 * 16), zero);
	_mm_store_si128((__m128i *) (bytePtr + 1 * 16), zero);
	_mm_store_si128((__m128i *) (bytePtr + 2 * 16), zero);
	_mm_store_si128((__m128i *) (bytePtr + 3 * 16), zero);
	_mm_store_si128((__m128i *) (bytePtr + 4 * 16), zero);
	_mm_store_si128((__m128i *) (bytePtr + 5 * 16), zero);
	_mm_store_si128((__m128i *) (bytePtr + 6 * 16), zero);
	_mm_store_si128((__m128i *) (bytePtr + 7 * 16), zero);
}
SIMD_FORCE_INLINE void FlushCacheLine(const void * ptr, int offset) {
	const char * bytePtr = ((const char *)ptr) + offset;
	_mm_clflush(bytePtr + 0);
	_mm_clflush(bytePtr + 64);
}

/*
================================================
Other
================================================
*/
#else

#define CACHE_LINE_SIZE						128

SIMD_INLINE void Prefetch(const void * ptr, int offset) {}
SIMD_INLINE void ZeroCacheLine(void * ptr, int offset) {
	byte * bytePtr = (byte *)((((UINT_PTR)(ptr)) + (offset)) & ~(CACHE_LINE_SIZE - 1));
	memset(bytePtr, 0, CACHE_LINE_SIZE);
}
SIMD_INLINE void FlushCacheLine(const void * ptr, int offset) {}

#endif

/*
================================================
Block Clear Macros
================================================
*/

// number of additional elements that are potentially cleared when clearing whole cache lines at a time
SIMD_INLINE_EXTERN int CACHE_LINE_CLEAR_OVERFLOW_COUNT(int size) {
	if ((size & (CACHE_LINE_SIZE - 1)) == 0) {
		return 0;
	}
	if (size > CACHE_LINE_SIZE) {
		return 1;
	}
	return (CACHE_LINE_SIZE / (size & (CACHE_LINE_SIZE - 1)));
}

// if the pointer is not on a cache line boundary this assumes the cache line the pointer starts in was already cleared
#define CACHE_LINE_CLEAR_BLOCK( ptr, size )																		\
	byte * startPtr = (byte *)( ( ( (UINT_PTR) ( ptr ) ) + CACHE_LINE_SIZE - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );	\
	byte * endPtr = (byte *)( ( (UINT_PTR) ( ptr ) + ( size ) - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );				\
	for ( ; startPtr <= endPtr; startPtr += CACHE_LINE_SIZE ) {													\
		ZeroCacheLine( startPtr, 0 );																			\
	}

#define CACHE_LINE_CLEAR_BLOCK_AND_FLUSH( ptr, size )															\
	byte * startPtr = (byte *)( ( ( (UINT_PTR) ( ptr ) ) + CACHE_LINE_SIZE - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );	\
	byte * endPtr = (byte *)( ( (UINT_PTR) ( ptr ) + ( size ) - 1 ) & ~( CACHE_LINE_SIZE - 1 ) );				\
	for ( ; startPtr <= endPtr; startPtr += CACHE_LINE_SIZE ) {													\
		ZeroCacheLine( startPtr, 0 );																			\
		FlushCacheLine( startPtr, 0 );																			\
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

// make the intrinsics "type unsafe"
typedef union __declspec(intrin_type)_CRT_ALIGN(16) __m128c {
	__m128c() {}
	__m128c(__m128 f) { m128 = f; }
	__m128c(__m128i i) { m128i = i; }
	operator	__m128() { return m128; }
	operator	__m128i() { return m128i; }
	__m128		m128;
	__m128i		m128i;
} __m128c;

#ifndef _mm_madd_ps
#define _mm_madd_ps( a, b, c )				_mm_add_ps( _mm_mul_ps( (a), (b) ), (c) )
#endif
#ifndef SIMD_mm_nmsub_ps
#define SIMD_mm_nmsub_ps( a, b, c )				_mm_sub_ps( (c), _mm_mul_ps( (a), (b) ) )
#endif
#ifndef _mm_splat_ps
#define _mm_splat_ps( x, i )				__m128c( _mm_shuffle_epi32( __m128c( x ), _MM_SHUFFLE( i, i, i, i ) ) )
#endif
#ifndef _mm_perm_ps
#define _mm_perm_ps( x, perm )				__m128c( _mm_shuffle_epi32( __m128c( x ), perm ) )
#endif
#ifndef _mm_sel_ps
#define _mm_sel_ps( a, b, c )  				_mm_or_ps( _mm_andnot_ps( __m128c( c ), a ), _mm_and_ps( __m128c( c ), b ) )
#endif
#ifndef _mm_sel_si128
#define _mm_sel_si128( a, b, c )			_mm_or_si128( _mm_andnot_si128( __m128c( c ), a ), _mm_and_si128( __m128c( c ), b ) )
#endif
#ifndef _mm_sld_ps
#define _mm_sld_ps( x, y, imm )				__m128c( _mm_or_si128( _mm_srli_si128( __m128c( x ), imm ), _mm_slli_si128( __m128c( y ), 16 - imm ) ) )
#endif
#ifndef _mm_sld_si128
#define _mm_sld_si128( x, y, imm )			_mm_or_si128( _mm_srli_si128( x, imm ), _mm_slli_si128( y, 16 - imm ) )
#endif

SIMD_FORCE_INLINE_EXTERN __m128 _mm_msum3_ps(__m128 a, __m128 b) {
	__m128 c = _mm_mul_ps(a, b);
	return _mm_add_ps(_mm_splat_ps(c, 0), _mm_add_ps(_mm_splat_ps(c, 1), _mm_splat_ps(c, 2)));
}

SIMD_FORCE_INLINE_EXTERN __m128 _mm_msum4_ps(__m128 a, __m128 b) {
	__m128 c = _mm_mul_ps(a, b);
	c = _mm_add_ps(c, _mm_perm_ps(c, _MM_SHUFFLE(1, 0, 3, 2)));
	c = _mm_add_ps(c, _mm_perm_ps(c, _MM_SHUFFLE(2, 3, 0, 1)));
	return c;
}

#define _mm_shufmix_epi32( x, y, perm )		__m128c( _mm_shuffle_ps( __m128c( x ), __m128c( y ), perm ) )
#define _mm_loadh_epi64( x, address )		__m128c( _mm_loadh_pi( __m128c( x ), (__m64 *)address ) )
#define _mm_storeh_epi64( address, x )		_mm_storeh_pi( (__m64 *)address, __m128c( x ) )

// floating-point reciprocal with close to full precision
SIMD_FORCE_INLINE_EXTERN __m128 _mm_rcp32_ps(__m128 x) {
	__m128 r = _mm_rcp_ps(x);		// _mm_rcp_ps() has 12 bits of precision
	r = _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(x, r), r));
	r = _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(x, r), r));
	return r;
}
// floating-point reciprocal with at least 16 bits precision
SIMD_FORCE_INLINE_EXTERN __m128 _mm_rcp16_ps(__m128 x) {
	__m128 r = _mm_rcp_ps(x);		// _mm_rcp_ps() has 12 bits of precision
	r = _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(x, r), r));
	return r;
}
// floating-point divide with close to full precision
SIMD_FORCE_INLINE_EXTERN __m128 _mm_div32_ps(__m128 x, __m128 y) {
	return _mm_mul_ps(x, _mm_rcp32_ps(y));
}
// floating-point divide with at least 16 bits precision
SIMD_FORCE_INLINE_EXTERN __m128 _mm_div16_ps(__m128 x, __m128 y) {
	return _mm_mul_ps(x, _mm_rcp16_ps(y));
}
// load idBounds::GetMins()
#define _mm_loadu_bounds_0( bounds )		_mm_perm_ps( _mm_loadh_pi( _mm_load_ss( & bounds[0].x ), (__m64 *) & bounds[0].y ), _MM_SHUFFLE( 1, 3, 2, 0 ) )
// load idBounds::GetMaxs()
#define _mm_loadu_bounds_1( bounds )		_mm_perm_ps( _mm_loadh_pi( _mm_load_ss( & bounds[1].x ), (__m64 *) & bounds[1].y ), _MM_SHUFFLE( 1, 3, 2, 0 ) )

#endif	// !__SYS_INTRIINSICS_H__

class idSIMDProcessor {
public:

	const int cpuid = (CPUID_MMX) & (CPUID_SSE) & (CPUID_SSE2) & (CPUID_SSE3);
	const char* SIMD_VPCALL GetName() { return "Blendo SIMD Inlined"; }

	idSIMDProcessor() {}
	~idSIMDProcessor() {};

	void SIMD_VPCALL Add( float *dst,			const float constant,	const float *src,		const int count );
	void SIMD_VPCALL Add( float *dst,			const float *src0,		const float *src1,		const int count );
	void SIMD_VPCALL Sub( float *dst,			const float constant,	const float *src,		const int count );
	void SIMD_VPCALL Sub( float *dst,			const float *src0,		const float *src1,		const int count );
	void SIMD_VPCALL Mul( float *dst,			const float constant,	const float *src,		const int count );
	void SIMD_VPCALL Mul( float *dst,			const float *src0,		const float *src1,		const int count );
	void SIMD_VPCALL Div( float *dst,			const float constant,	const float *src,		const int count );
	void SIMD_VPCALL Div( float *dst,			const float *src0,		const float *src1,		const int count );
	void SIMD_VPCALL MulAdd( float *dst,			const float constant,	const float *src,		const int count );
	void SIMD_VPCALL MulAdd( float *dst,			const float *src0,		const float *src1,		const int count );
	void SIMD_VPCALL MulSub( float *dst,			const float constant,	const float *src,		const int count );
	void SIMD_VPCALL MulSub( float *dst,			const float *src0,		const float *src1,		const int count );

	void SIMD_VPCALL Dot(float *dst, const idVec3 &constant, const idVec3 *src, const int count);
	void SIMD_VPCALL Dot(float *dst, const idVec3 &constant, const idPlane *src, const int count);
	void SIMD_VPCALL Dot(float *dst, const idVec3 &constant, const idDrawVert *src, const int count);
	void SIMD_VPCALL Dot(float *dst, const idPlane &constant, const idVec3 *src, const int count);
	void SIMD_VPCALL Dot(float *dst, const idPlane &constant, const idPlane *src, const int count);
	void SIMD_VPCALL Dot(float *dst, const idPlane &constant, const idDrawVert *src, const int count);
	void SIMD_VPCALL Dot(float *dst, const idVec3 *src0, const idVec3 *src1, const int count);
	void SIMD_VPCALL Dot(float &dot, const float *src1, const float *src2, const int count);

	void SIMD_VPCALL CmpGT(byte *dst, const float *src0, const float constant, const int count);
	void SIMD_VPCALL CmpGT(byte *dst, const byte bitNum, const float *src0, const float constant, const int count);
	void SIMD_VPCALL CmpGE(byte *dst, const float *src0, const float constant, const int count);
	void SIMD_VPCALL CmpGE(byte *dst, const byte bitNum, const float *src0, const float constant, const int count);
	void SIMD_VPCALL CmpLT(byte *dst, const float *src0, const float constant, const int count);
	void SIMD_VPCALL CmpLT(byte *dst, const byte bitNum, const float *src0, const float constant, const int count);
	void SIMD_VPCALL CmpLE(byte *dst, const float *src0, const float constant, const int count);
	void SIMD_VPCALL CmpLE(byte *dst, const byte bitNum, const float *src0, const float constant, const int count);


	void SIMD_VPCALL MinMax(float &min, float &max, const float *src, const int count);
	void SIMD_VPCALL MinMax(idVec2 &min, idVec2 &max, const idVec2 *src, const int count);
	void SIMD_VPCALL MinMax(idVec3 &min, idVec3 &max, const idVec3 *src, const int count);
	void SIMD_VPCALL MinMax(idVec3 &min, idVec3 &max, const idDrawVert *src, const int count);
	void SIMD_VPCALL MinMax(idVec3 &min, idVec3 &max, const idDrawVert *src, const int *indexes, const int count);

	void SIMD_VPCALL Clamp(float *dst, const float *src, const float min, const float max, const int count);
	void SIMD_VPCALL ClampMin(float *dst, const float *src, const float min, const int count);
	void SIMD_VPCALL ClampMax(float *dst, const float *src, const float max, const int count);

	void SIMD_VPCALL Memcpy(void *dst, const void *src, const int count);
	void SIMD_VPCALL Memset(void *dst, const int val, const int count);

	// these assume 16 byte aligned and 16 byte padded memory
	void SIMD_VPCALL Zero16(float *dst, const int count);
	void SIMD_VPCALL Negate16(float *dst, const int count);
	void SIMD_VPCALL Copy16(float *dst, const float *src, const int count);
	void SIMD_VPCALL Add16(float *dst, const float *src1, const float *src2, const int count);
	void SIMD_VPCALL Sub16(float *dst, const float *src1, const float *src2, const int count);
	void SIMD_VPCALL Mul16(float *dst, const float *src1, const float constant, const int count);
	void SIMD_VPCALL AddAssign16(float *dst, const float *src, const int count);
	void SIMD_VPCALL SubAssign16(float *dst, const float *src, const int count);
	void SIMD_VPCALL MulAssign16(float *dst, const float constant, const int count);

	// idMatX operations
	void SIMD_VPCALL MatX_MultiplyVecX(idVecX &dst, const idMatX &mat, const idVecX &vec);
	void SIMD_VPCALL MatX_MultiplyAddVecX(idVecX &dst, const idMatX &mat, const idVecX &vec);
	void SIMD_VPCALL MatX_MultiplySubVecX(idVecX &dst, const idMatX &mat, const idVecX &vec);
	void SIMD_VPCALL MatX_TransposeMultiplyVecX(idVecX &dst, const idMatX &mat, const idVecX &vec);
	void SIMD_VPCALL MatX_TransposeMultiplyAddVecX(idVecX &dst, const idMatX &mat, const idVecX &vec);
	void SIMD_VPCALL MatX_TransposeMultiplySubVecX(idVecX &dst, const idMatX &mat, const idVecX &vec);
	void SIMD_VPCALL MatX_MultiplyMatX(idMatX &dst, const idMatX &m1, const idMatX &m2);
	void SIMD_VPCALL MatX_TransposeMultiplyMatX(idMatX &dst, const idMatX &m1, const idMatX &m2);
	void SIMD_VPCALL MatX_LowerTriangularSolve(const idMatX &L, float *x, const float *b, const int n, int skip = 0);
	void SIMD_VPCALL MatX_LowerTriangularSolveTranspose(const idMatX &L, float *x, const float *b, const int n);
	bool SIMD_VPCALL MatX_LDLTFactor(idMatX &mat, idVecX &invDiag, const int n);


	// rendering
	void SIMD_VPCALL BlendJoints(idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints);
	void SIMD_VPCALL ConvertJointQuatsToJointMats(idJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints);
	void SIMD_VPCALL ConvertJointMatsToJointQuats(idJointQuat *jointQuats, const idJointMat *jointMats, const int numJoints);
	void SIMD_VPCALL TransformJoints(idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint);
	void SIMD_VPCALL UntransformJoints(idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint);
	void SIMD_VPCALL TransformVerts(idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *weights, const int *index, const int numWeights);
	void SIMD_VPCALL TracePointCull(byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts);
	void SIMD_VPCALL DecalPointCull(byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts);
	void SIMD_VPCALL OverlayPointCull(byte *cullBits, idVec2 *texCoords, const idPlane *planes, const idDrawVert *verts, const int numVerts);
	void SIMD_VPCALL DeriveTriPlanes(idPlane *planes, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes);
	void SIMD_VPCALL DeriveTangents(idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes);
	void SIMD_VPCALL DeriveUnsmoothedTangents(idDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts);
	void SIMD_VPCALL NormalizeTangents(idDrawVert *verts, const int numVerts);
	void SIMD_VPCALL CreateTextureSpaceLightVectors(idVec3 *lightVectors, const idVec3 &lightOrigin, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes);
	void SIMD_VPCALL CreateSpecularTextureCoords(idVec4 *texCoords, const idVec3 &lightOrigin, const idVec3 &viewOrigin, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes);
	int  SIMD_VPCALL CreateShadowCache(idVec4 *vertexCache, int *vertRemap, const idVec3 &lightOrigin, const idDrawVert *verts, const int numVerts);
	int  SIMD_VPCALL CreateVertexProgramShadowCache(idVec4 *vertexCache, const idDrawVert *verts, const int numVerts);


	void SIMD_VPCALL BlendJointsFast(idJointQuat* joints, const idJointQuat* blendJoints, const float lerp, const int* index, const int numJoints);

	// sound mixing
	void SIMD_VPCALL UpSamplePCMTo44kHz(float *dest, const short *pcm, const int numSamples, const int kHz, const int numChannels);
	void SIMD_VPCALL UpSampleOGGTo44kHz(float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels);
	void SIMD_VPCALL MixSoundTwoSpeakerMono(float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2]);
	void SIMD_VPCALL MixSoundTwoSpeakerStereo(float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2]);
	void SIMD_VPCALL MixSoundSixSpeakerMono(float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6]);
	void SIMD_VPCALL MixSoundSixSpeakerStereo(float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6]);
	void SIMD_VPCALL MixedSoundToSamples(short *samples, const float *mixBuffer, const int numSamples);
};



/*
================
Memcpy
================
*/
SIMD_INLINE void SIMD_VPCALL idSIMDProcessor::Memcpy(void *dst, const void *src, const int count) {
	memcpy(dst, src, count);
}

/*
================
Memset
================
*/
SIMD_INLINE void SIMD_VPCALL idSIMDProcessor::Memset(void *dst, const int val, const int count) {
	memset(dst, val, count);
}

/*
============
Zero16
============
*/
SIMD_INLINE void SIMD_VPCALL idSIMDProcessor::Zero16(float *dst, const int count) {
	memset(dst, 0, count * sizeof(float));
}


#endif // __BLENDO_SIMD_INLINE__

#endif // __MATH_SIMD_BLENDO_H__