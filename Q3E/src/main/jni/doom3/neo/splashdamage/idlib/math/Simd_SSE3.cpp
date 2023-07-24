// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "Simd_Generic.h"
#include "Simd_MMX.h"
#include "Simd_SSE.h"
#include "Simd_SSE2.h"
#include "Simd_SSE3.h"


//===============================================================
//
//	SSE3 implementation of idSIMDProcessor
//
//===============================================================

#ifdef ID_WIN_X86_ASM

#include "Simd_InstructionMacros.h"

ALIGN4_INIT4( unsigned long SIMD_SP_clearLast, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 );
ALIGN4_INIT4( float SIMD_SP_lastOne, 0.0f, 0.0f, 0.0f, 1.0f );

/*
============
SSE3_Dot
============
*/
void SSE3_Dot( const idVec4 &v1, const idVec4 &v2, float &result ) {
	__asm {
		mov			esi, v1
		mov			edi, v2
		mov			ecx, result
		movaps		xmm0, [esi]
		mulps		xmm0, [edi]
		_haddps(	_xmm0, _xmm0 )
		_haddps(	_xmm0, _xmm0 )
		movss		[ecx], xmm0
	}
}

/*
============
SSE3_Dot4
============
*/
void SSE3_Dot4( const idVec4 v1[4], const idVec4 v2[4], float result[4] ) {
	__asm {
		mov			esi, v1
		mov			edi, v2
		mov			ecx, result
		movaps		xmm0, [esi+0*16]
		mulps		xmm0, [edi+0*16]
		movaps		xmm1, [esi+1*16]
		mulps		xmm1, [edi+1*16]
		movaps		xmm2, [esi+2*16]
		mulps		xmm2, [edi+2*16]
		movaps		xmm3, [esi+3*16]
		mulps		xmm3, [edi+3*16]
		_haddps(	_xmm0, _xmm1 )
		_haddps(	_xmm2, _xmm3 )
		_haddps(	_xmm0, _xmm2 )
		movaps		[ecx], xmm0
	}
}

/*
============
idSIMD_SSE3::GetName
============
*/
const char * idSIMD_SSE3::GetName( void ) const {
	return "MMX & SSE & SSE2 & SSE3";
}

/*
============
idSIMD_SSE3::TransformVerts
============
*/
void VPCALL idSIMD_SSE3::TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) {

	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( base );

	__asm
	{
		mov			eax, numVerts
		test		eax, eax
		jz			done
		imul		eax, DRAWVERT_SIZE

		mov			ecx, verts
		mov			edx, weights
		mov			esi, base
		mov			edi, joints

		add			ecx, eax
		neg			eax

	loopVert:
		mov			ebx, dword ptr [edx+JOINTWEIGHT_JOINTMATOFFSET_OFFSET]
		movaps		xmm2, [esi]
		add			edx, JOINTWEIGHT_SIZE
		movaps		xmm0, xmm2
		add			esi, BASEVECTOR_SIZE
		movaps		xmm1, xmm2

		mulps		xmm0, [edi+ebx+ 0]						// xmm0 = m0, m1, m2, t0
		mulps		xmm1, [edi+ebx+16]						// xmm1 = m3, m4, m5, t1
		mulps		xmm2, [edi+ebx+32]						// xmm2 = m6, m7, m8, t2

		cmp			dword ptr [edx-JOINTWEIGHT_SIZE+JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET], JOINTWEIGHT_SIZE

		je			doneWeight

	loopWeight:
		mov			ebx, dword ptr [edx+JOINTWEIGHT_JOINTMATOFFSET_OFFSET]
		movaps		xmm5, [esi]
		add			edx, JOINTWEIGHT_SIZE
		movaps		xmm3, xmm5
		add			esi, BASEVECTOR_SIZE
		movaps		xmm4, xmm5

		mulps		xmm3, [edi+ebx+ 0]						// xmm3 = m0, m1, m2, t0
		mulps		xmm4, [edi+ebx+16]						// xmm4 = m3, m4, m5, t1
		mulps		xmm5, [edi+ebx+32]						// xmm5 = m6, m7, m8, t2

		cmp			dword ptr [edx-JOINTWEIGHT_SIZE+JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET], JOINTWEIGHT_SIZE

		addps		xmm0, xmm3
		addps		xmm1, xmm4
		addps		xmm2, xmm5

		jne			loopWeight

	doneWeight:
		add			eax, DRAWVERT_SIZE

		_haddps(	_xmm0, _xmm1 )
		_haddps(	_xmm2, _xmm0 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0], xmm2

		pshufd		xmm3, xmm2, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm3, xmm2

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8], xmm3

		jl			loopVert
	done:
	}
}

/*
============
idSIMD_SSE3::TransformShadowVerts
============
*/
void VPCALL idSIMD_SSE3::TransformShadowVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const jointWeight_t *weights, const int numWeights ) {
	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( base );

	__asm
	{
		mov			eax, numVerts
		test		eax, eax
		jz			done
		imul		eax, DRAWVERT_SIZE

		mov			ecx, verts
		mov			edx, weights
		mov			esi, base
		mov			edi, joints

		add			ecx, eax
		neg			eax

		movaps		xmm0, SIMD_SP_clearLast
		movaps		xmm1, SIMD_SP_lastOne

	loopVert:
		add			esi, DRAWVERT_SIZE
		mov			ebx, dword ptr [edx+JOINTWEIGHT_JOINTMATOFFSET_OFFSET]

		movaps		xmm3, [esi-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0]
		andps		xmm3, xmm0
		orps		xmm3, xmm1

		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, [edi+ebx+ 0]						// xmm0 = m0, m1, m2, t0
		mulps		xmm4, [edi+ebx+16]						// xmm1 = m3, m4, m5, t1
		mulps		xmm5, [edi+ebx+32]						// xmm2 = m6, m7, m8, t2

		add			edx, dword ptr [edx+JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET]
		add			eax, DRAWVERT_SIZE

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0], xmm5

		pshufd		xmm7, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm7, xmm5

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8], xmm7

		jl			loopVert
	done:
	}
}

/*
============
idSIMD_SSE3::TransformShadowVerts
============
*/
void VPCALL idSIMD_SSE3::TransformShadowVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const short *weights, const int numWeights ) {
	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( base );

	__asm
	{
		mov			eax, numVerts
		test		eax, eax
		jz			done
		imul		eax, DRAWVERT_SIZE

		mov			ecx, verts
		mov			edx, weights
		mov			esi, base
		mov			edi, joints

		add			ecx, eax
		neg			eax

		movaps		xmm0, SIMD_SP_clearLast
		movaps		xmm1, SIMD_SP_lastOne

	loopVert:
		add			esi, DRAWVERT_SIZE
		movzx		ebx, word ptr [edx]

		movaps		xmm3, [esi-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0]
		andps		xmm3, xmm0
		orps		xmm3, xmm1

		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, [edi+ebx+ 0]						// xmm0 = m0, m1, m2, t0
		mulps		xmm4, [edi+ebx+16]						// xmm1 = m3, m4, m5, t1
		mulps		xmm5, [edi+ebx+32]						// xmm2 = m6, m7, m8, t2

		add			edx, 2
		add			eax, DRAWVERT_SIZE

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0], xmm5

		pshufd		xmm7, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm7, xmm5

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8], xmm7

		jl			loopVert
	done:
	}
}

/*
============
idSIMD_SSE3::TransformShadowVerts
============
*/
void VPCALL idSIMD_SSE3::TransformShadowVerts( shadowCache_t *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const short *weights, const int numWeights ) {
	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( base );

	__asm
	{
		mov			eax, numVerts
		test		eax, eax
		jz			done
		imul		eax, SHADOWVERT_SIZE

		mov			ecx, verts
		mov			edx, weights
		mov			esi, base
		mov			edi, joints

		add			ecx, eax
		neg			eax

		movaps		xmm0, SIMD_SP_clearLast
		movaps		xmm1, SIMD_SP_lastOne

	loopVert:
		add			esi, DRAWVERT_SIZE
		movzx		ebx, word ptr [edx]

		movaps		xmm3, [esi-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0]
		andps		xmm3, xmm0
		orps		xmm3, xmm1

		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, [edi+ebx+ 0]						// xmm0 = m0, m1, m2, t0
		mulps		xmm4, [edi+ebx+16]						// xmm1 = m3, m4, m5, t1
		mulps		xmm5, [edi+ebx+32]						// xmm2 = m6, m7, m8, t2

		add			edx, 2
		add			eax, SHADOWVERT_SIZE

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-SHADOWVERT_SIZE+0], xmm5

		pshufd		xmm7, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm7, xmm5

		movss		[ecx+eax-SHADOWVERT_SIZE+8], xmm7

		jl			loopVert
	done:
	}
}


#if !defined(SD_USE_DRAWVERT_SIZE_32)
/*
============
idSIMD_SSE3::TransformVertsAndTangents
============
*/
void VPCALL idSIMD_SSE3::TransformVertsAndTangents( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) {

	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( base );

	__asm
	{
		mov			eax, numVerts
		test		eax, eax
		jz			done
		imul		eax, DRAWVERT_SIZE

		mov			ecx, verts
		mov			edx, weights
		mov			esi, base
		mov			edi, joints

		add			ecx, eax
		neg			eax

	loopVert:
		movss		xmm2, [edx+JOINTWEIGHT_WEIGHT_OFFSET]
		mov			ebx, dword ptr [edx+JOINTWEIGHT_JOINTMATOFFSET_OFFSET]
		shufps		xmm2, xmm2, R_SHUFFLE_PS( 0, 0, 0, 0 )
		add			edx, JOINTWEIGHT_SIZE
		movaps		xmm0, xmm2
		movaps		xmm1, xmm2

		mulps		xmm0, [edi+ebx+ 0]						// xmm0 = m0, m1, m2, t0
		mulps		xmm1, [edi+ebx+16]						// xmm1 = m3, m4, m5, t1
		mulps		xmm2, [edi+ebx+32]						// xmm2 = m6, m7, m8, t2

		cmp			dword ptr [edx-JOINTWEIGHT_SIZE+JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET], JOINTWEIGHT_SIZE

		je			doneWeight

	loopWeight:
		movss		xmm5, [edx+JOINTWEIGHT_WEIGHT_OFFSET]
		mov			ebx, dword ptr [edx+JOINTWEIGHT_JOINTMATOFFSET_OFFSET]
		shufps		xmm5, xmm5, R_SHUFFLE_PS( 0, 0, 0, 0 )
		add			edx, JOINTWEIGHT_SIZE
		movaps		xmm3, xmm5
		movaps		xmm4, xmm5

		mulps		xmm3, [edi+ebx+ 0]						// xmm3 = m0, m1, m2, t0
		mulps		xmm4, [edi+ebx+16]						// xmm4 = m3, m4, m5, t1
		mulps		xmm5, [edi+ebx+32]						// xmm5 = m6, m7, m8, t2

		cmp			dword ptr [edx-JOINTWEIGHT_SIZE+JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET], JOINTWEIGHT_SIZE

		addps		xmm0, xmm3
		addps		xmm1, xmm4
		addps		xmm2, xmm5

		jne			loopWeight

	doneWeight:
		add			esi, 3*BASEVECTOR_SIZE
		add			eax, DRAWVERT_SIZE

		// transform vertex
		movaps		xmm3, [esi-3*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0], xmm5

		pshufd		xmm7, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm7, xmm5

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8], xmm7

		// transform normal
		movaps		xmm3, [esi-2*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+0], xmm5

		pshufd		xmm7, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm7, xmm5

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+8], xmm7

		// transform first tangent
		movaps		xmm3, [esi-1*BASEVECTOR_SIZE]

		mulps		xmm0, xmm3
		mulps		xmm1, xmm3
		mulps		xmm2, xmm3

		_haddps(	_xmm0, _xmm1 )
		_haddps(	_xmm2, _xmm0 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT_OFFSET+0], xmm2

		pshufd		xmm7, xmm2, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm7, xmm2

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT_OFFSET+8], xmm7

		jl			loopVert
	done:
	}
}

/*
============
idSIMD_SSE3::TransformVertsAndTangentsFast
============
*/
void VPCALL idSIMD_SSE3::TransformVertsAndTangentsFast( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) {

	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( base );

	__asm
	{
		mov			eax, numVerts
		test		eax, eax
		jz			done
		imul		eax, DRAWVERT_SIZE

		mov			ecx, verts
		mov			edx, weights
		mov			esi, base
		mov			edi, joints

		add			ecx, eax
		neg			eax

	loopVert:
		mov			ebx, dword ptr [edx+JOINTWEIGHT_JOINTMATOFFSET_OFFSET]

		add			esi, 3*BASEVECTOR_SIZE

		movaps		xmm0, [edi+ebx+ 0]						// xmm0 = m0, m1, m2, t0
		movaps		xmm1, [edi+ebx+16]						// xmm1 = m3, m4, m5, t1
		movaps		xmm2, [edi+ebx+32]						// xmm2 = m6, m7, m8, t2

		add			edx, dword ptr [edx+JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET]

		add			eax, DRAWVERT_SIZE

		// transform vertex
		movaps		xmm3, [esi-3*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0], xmm5

		pshufd		xmm7, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm7, xmm5

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8], xmm7

		// transform normal
		movaps		xmm3, [esi-2*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+0], xmm5

		pshufd		xmm7, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm7, xmm5

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+8], xmm7

		// transform first tangent
		movaps		xmm3, [esi-1*BASEVECTOR_SIZE]

		mulps		xmm0, xmm3
		mulps		xmm1, xmm3
		mulps		xmm2, xmm3

		_haddps(	_xmm0, _xmm1 )
		_haddps(	_xmm2, _xmm0 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT_OFFSET+0], xmm2

		pshufd		xmm7, xmm2, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm7, xmm2

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT_OFFSET+8], xmm7

		jl			loopVert
	done:
	}
}
#endif

#endif /* ID_WIN_X86_ASM */
