
#include "../precompiled.h"
#pragma hdrstop

#include "Simd_generic.h"
#include "Simd_MMX.h"
#include "Simd_SSE.h"
#include "Simd_SSE2.h"
#include "Simd_SSE3.h"
#include "Simd_InstructionMacros.h"
#include "../geometry/JointTransform.h"

//===============================================================
//
//	SSE3 implementation of idSIMDProcessor
//
//===============================================================

#ifdef _WINDOWS

#include "Simd_InstructionMacros.h"

ALIGN4_INIT1( float SIMD_SP_infinity, idMath::INFINITY );
ALIGN4_INIT1( float SIMD_SP_negInfinity, -idMath::INFINITY );

/*
============
SSE3_Dot
============
*/
float SSE3_Dot( const idVec4 &v1, const idVec4 &v2 ) {
	float d;
	__asm {
		mov		esi, v1
		mov		edi, v2
		movaps	xmm0, [esi]
		mulps	xmm0, [edi]
		_haddps(_xmm0, _xmm0 )
		_haddps(_xmm0, _xmm0 )
		movss	d, xmm0
	}
	return d;
}

/*
============
idSIMD_SSE3::GetName
============
*/
const char * idSIMD_SSE3::GetName( void ) const {
	return "MMX & SSE & SSE2 & SSE3";
}

#pragma warning( disable : 4731 )	// frame pointer register 'ebx' modified by inline assembly code

/*
============
idSIMD_SSE3::TransformVertsNew
============
*/
void VPCALL idSIMD_SSE3::TransformVertsNew( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) {
	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( base );

	__asm {
		push		ebx
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

		movaps		xmm6, SIMD_SP_infinity
		movaps		xmm7, SIMD_SP_negInfinity

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
		movss		xmm2, xmm3

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8], xmm3

		minps		xmm6, xmm2
		maxps		xmm7, xmm2

		jl			loopVert

	done:
		pop			ebx
		mov			esi, bounds
		movhps		[esi+ 0], xmm6
		movss		[esi+ 8], xmm6
		movhps		[esi+12], xmm7
		movss		[esi+20], xmm7
	}
}

/*
============
idSIMD_SSE3::TransformVertsAndTangents
============
*/
void VPCALL idSIMD_SSE3::TransformVertsAndTangents( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) {

	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( base );

	__asm {
		push		ebx
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

		movaps		xmm6, SIMD_SP_infinity
		movaps		xmm7, SIMD_SP_negInfinity

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
		add			esi, 4*BASEVECTOR_SIZE
		add			eax, DRAWVERT_SIZE

		// transform vertex
		movaps		xmm3, [esi-4*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0], xmm5

		pshufd		xmm4, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm4, xmm5
		movss		xmm5, xmm4

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8], xmm4

		minps		xmm6, xmm5
		maxps		xmm7, xmm5

		// transform normal
		movaps		xmm3, [esi-3*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+0], xmm5

		pshufd		xmm4, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm4, xmm5

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+8], xmm4

		// transform first tangent
		movaps		xmm3, [esi-2*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT0_OFFSET+0], xmm5

		pshufd		xmm4, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm4, xmm5

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT0_OFFSET+8], xmm4

		// transform second tangent
		movaps		xmm3, [esi-1*BASEVECTOR_SIZE]

		mulps		xmm0, xmm3
		mulps		xmm1, xmm3
		mulps		xmm2, xmm3

		_haddps(	_xmm0, _xmm1 )
		_haddps(	_xmm2, _xmm0 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT1_OFFSET+0], xmm2

		pshufd		xmm4, xmm2, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm4, xmm2

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT1_OFFSET+8], xmm4

		jl			loopVert

	done:
		pop			ebx
		mov			esi, bounds
		movhps		[esi+ 0], xmm6
		movss		[esi+ 8], xmm6
		movhps		[esi+12], xmm7
		movss		[esi+20], xmm7
	}
}

/*
============
idSIMD_SSE3::TransformVertsAndTangentsFast
============
*/
void VPCALL idSIMD_SSE3::TransformVertsAndTangentsFast( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) {

	assert_16_byte_aligned( joints );
	assert_16_byte_aligned( base );

	__asm {
		push		ebx
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

		movaps		xmm6, SIMD_SP_infinity
		movaps		xmm7, SIMD_SP_negInfinity

	loopVert:
		mov			ebx, dword ptr [edx+JOINTWEIGHT_JOINTMATOFFSET_OFFSET]

		add			esi, 4*BASEVECTOR_SIZE

		movaps		xmm0, [edi+ebx+ 0]						// xmm0 = m0, m1, m2, t0
		movaps		xmm1, [edi+ebx+16]						// xmm1 = m3, m4, m5, t1
		movaps		xmm2, [edi+ebx+32]						// xmm2 = m6, m7, m8, t2

		add			edx, dword ptr [edx+JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET]

		add			eax, DRAWVERT_SIZE

		// transform vertex
		movaps		xmm3, [esi-4*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0], xmm5

		pshufd		xmm4, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm4, xmm5
		movss		xmm5, xmm4

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8], xmm4

		minps		xmm6, xmm5
		maxps		xmm7, xmm5

		// transform normal
		movaps		xmm3, [esi-3*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+0], xmm5

		pshufd		xmm4, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm4, xmm5

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+8], xmm4

		// transform first tangent
		movaps		xmm3, [esi-2*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		_haddps(	_xmm3, _xmm4 )
		_haddps(	_xmm5, _xmm3 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT0_OFFSET+0], xmm5

		pshufd		xmm4, xmm5, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm4, xmm5

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT0_OFFSET+8], xmm4

		// transform second tangent
		movaps		xmm3, [esi-1*BASEVECTOR_SIZE]

		mulps		xmm0, xmm3
		mulps		xmm1, xmm3
		mulps		xmm2, xmm3

		_haddps(	_xmm0, _xmm1 )
		_haddps(	_xmm2, _xmm0 )

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT1_OFFSET+0], xmm2

		pshufd		xmm4, xmm2, R_SHUFFLE_D( 1, 0, 2, 3 )
		addss		xmm4, xmm2

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT1_OFFSET+8], xmm4

		jl			loopVert

	done:
		pop			ebx
		mov			esi, bounds
		movhps		[esi+ 0], xmm6
		movss		[esi+ 8], xmm6
		movhps		[esi+12], xmm7
		movss		[esi+20], xmm7
	}
}

#endif /* _WINDOWS */
