
#include "../precompiled.h"
#pragma hdrstop

#include "Simd_generic.h"
#include "Simd_MMX.h"
#include "Simd_SSE.h"
#include "Simd_SSE2.h"
#include "Simd_InstructionMacros.h"


//===============================================================
//
//	SSE2 implementation of idSIMDProcessor
//
//===============================================================

#ifdef _WINDOWS

#include "Simd_InstructionMacros.h"

ALIGN8_INIT1( unsigned short SIMD_W_zero, 0 );
ALIGN8_INIT1( unsigned short SIMD_W_maxShort, 1<<15 );

ALIGN4_INIT4( unsigned long SIMD_SP_firstSignBit, IEEE_SP_SIGN, IEEE_SP_ZERO, IEEE_SP_ZERO, IEEE_SP_ZERO );
ALIGN4_INIT1( unsigned long SIMD_SP_signBit, IEEE_SP_SIGN );
ALIGN4_INIT1( unsigned long SIMD_SP_absMask, ~IEEE_SP_SIGN );
ALIGN4_INIT1( unsigned long SIMD_SP_infinityMask, ~IEEE_SP_INF );
ALIGN4_INIT1( unsigned long SIMD_SP_not, 0xFFFFFFFF );
ALIGN4_INIT4( unsigned long SIMD_SP_clearLast, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 );
ALIGN4_INIT4( unsigned long SIMD_SP_clearFirstThree, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF );

ALIGN4_INIT4( unsigned long SIMD_SP_quat2mat_x0, IEEE_SP_ZERO, IEEE_SP_SIGN, IEEE_SP_SIGN, IEEE_SP_SIGN );
ALIGN4_INIT4( unsigned long SIMD_SP_quat2mat_x1, IEEE_SP_SIGN, IEEE_SP_ZERO, IEEE_SP_SIGN, IEEE_SP_SIGN );
ALIGN4_INIT4( unsigned long SIMD_SP_quat2mat_x2, IEEE_SP_ZERO, IEEE_SP_SIGN, IEEE_SP_SIGN, IEEE_SP_SIGN );

ALIGN4_INIT4( unsigned long SIMD_DW_zero, 0, 0, 0, 0 );
ALIGN16_INIT1( unsigned char SIMD_B_one, 1 );

ALIGN4_INIT4( unsigned long SIMD_DW_capTris_c0, 0, 0, 0, 1 );
ALIGN4_INIT4( unsigned long SIMD_DW_capTris_c1, 1, 1, 0, 0 );
ALIGN4_INIT4( unsigned long SIMD_DW_capTris_c2, 0, 1, 0, 0 );

ALIGN4_INIT1( float SIMD_SP_zero, 0.0f );
ALIGN4_INIT1( float SIMD_SP_one, 1.0f );
ALIGN4_INIT1( float SIMD_SP_two, 2.0f );
ALIGN4_INIT1( float SIMD_SP_three, 3.0f );
ALIGN4_INIT1( float SIMD_SP_four, 4.0f );
ALIGN4_INIT1( float SIMD_SP_maxShort, (1<<15) );
ALIGN4_INIT1( float SIMD_SP_tiny, 1e-10f );
ALIGN4_INIT1( float SIMD_SP_PI, idMath::PI );
ALIGN4_INIT1( float SIMD_SP_halfPI, idMath::HALF_PI );
ALIGN4_INIT1( float SIMD_SP_twoPI, idMath::TWO_PI );
ALIGN4_INIT1( float SIMD_SP_oneOverTwoPI, 1.0f / idMath::TWO_PI );
ALIGN4_INIT1( float SIMD_SP_infinity, idMath::INFINITY );
ALIGN4_INIT1( float SIMD_SP_negInfinity, -idMath::INFINITY );


/*
============
idSIMD_SSE2::GetName
============
*/
const char * idSIMD_SSE2::GetName( void ) const {
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
void VPCALL idSIMD_SSE2::MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip ) {
	int nc;
	const float *lptr;

	if ( skip >= n ) {
		return;
	}

	lptr = L[skip];
	nc = L.GetNumColumns();

	// unrolled cases for n < 8
	if ( n < 8 ) {
		#define NSKIP( n, s )	((n<<3)|(s&7))
		switch( NSKIP( n, skip ) ) {
			case NSKIP( 1, 0 ): x[0] = b[0];
				return;
			case NSKIP( 2, 0 ): x[0] = b[0];
			case NSKIP( 2, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
				return;
			case NSKIP( 3, 0 ): x[0] = b[0];
			case NSKIP( 3, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 3, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
				return;
			case NSKIP( 4, 0 ): x[0] = b[0];
			case NSKIP( 4, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 4, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 4, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
				return;
			case NSKIP( 5, 0 ): x[0] = b[0];
			case NSKIP( 5, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 5, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 5, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP( 5, 4 ): x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
				return;
			case NSKIP( 6, 0 ): x[0] = b[0];
			case NSKIP( 6, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 6, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 6, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP( 6, 4 ): x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
			case NSKIP( 6, 5 ): x[5] = b[5] - lptr[5*nc+0] * x[0] - lptr[5*nc+1] * x[1] - lptr[5*nc+2] * x[2] - lptr[5*nc+3] * x[3] - lptr[5*nc+4] * x[4];
				return;
			case NSKIP( 7, 0 ): x[0] = b[0];
			case NSKIP( 7, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 7, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 7, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP( 7, 4 ): x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
			case NSKIP( 7, 5 ): x[5] = b[5] - lptr[5*nc+0] * x[0] - lptr[5*nc+1] * x[1] - lptr[5*nc+2] * x[2] - lptr[5*nc+3] * x[3] - lptr[5*nc+4] * x[4];
			case NSKIP( 7, 6 ): x[6] = b[6] - lptr[6*nc+0] * x[0] - lptr[6*nc+1] * x[1] - lptr[6*nc+2] * x[2] - lptr[6*nc+3] * x[3] - lptr[6*nc+4] * x[4] - lptr[6*nc+5] * x[5];
				return;
		}
		return;
	}

	// process first 4 rows
	switch( skip ) {
		case 0: x[0] = b[0];
		case 1: x[1] = b[1] - lptr[1*nc+0] * x[0];
		case 2: x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
		case 3: x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
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
		shufpd		xmm1, xmm1, R_SHUFFLE_PD( 1, 0 )
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
void VPCALL idSIMD_SSE2::MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n ) {
	int nc;
	const float *lptr;

	lptr = L.ToFloatPtr();
	nc = L.GetNumColumns();

	// unrolled cases for n < 8
	if ( n < 8 ) {
		switch( n ) {
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
	if ( m & 1 ) {
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
			shufpd		xmm5, xmm5, R_SHUFFLE_PD( 0, 0 )
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+4]
			shufpd		xmm7, xmm7, R_SHUFFLE_PD( 0, 0 )
			mulpd		xmm1, xmm7
			cvtps2pd	xmm3, [edi]
			mulpd		xmm6, xmm7
			cvtps2pd	xmm4, [edi+8]
			subpd		xmm0, xmm1
			subpd		xmm2, xmm6
			add			edi, edx
			cvtss2sd	xmm5, [esi+4*ecx+8]
			shufpd		xmm5, xmm5, R_SHUFFLE_PD( 0, 0 )
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+12]
			shufpd		xmm7, xmm7, R_SHUFFLE_PD( 0, 0 )
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
			shufpd		xmm5, xmm5, R_SHUFFLE_PD( 0, 0 )
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
			shufpd		xmm1, xmm1, R_SHUFFLE_PD( 1, 1 )
			movapd		xmm3, xmm2
			shufpd		xmm3, xmm3, R_SHUFFLE_PD( 1, 1 )
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
	}
	else {
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
			shufpd		xmm5, xmm5, R_SHUFFLE_PD( 0, 0 )
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+4]
			shufpd		xmm7, xmm7, R_SHUFFLE_PD( 0, 0 )
			mulpd		xmm1, xmm7
			cvtps2pd	xmm3, [edi]
			mulpd		xmm6, xmm7
			cvtps2pd	xmm4, [edi+8]
			subpd		xmm0, xmm1
			subpd		xmm2, xmm6
			add			edi, edx
			cvtss2sd	xmm5, [esi+4*ecx+8]
			shufpd		xmm5, xmm5, R_SHUFFLE_PD( 0, 0 )
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+12]
			shufpd		xmm7, xmm7, R_SHUFFLE_PD( 0, 0 )
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
			shufpd		xmm1, xmm1, R_SHUFFLE_PD( 1, 1 )
			movapd		xmm3, xmm2
			shufpd		xmm3, xmm3, R_SHUFFLE_PD( 1, 1 )
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
	for ( i = (m&3)-1; i >= 0; i-- ) {
		s0 = b[i];
		lptr = L[i+1] + i;
		for ( j = i + 1; j < m; j++ ) {
			s0 -= lptr[0] * x[j];
			lptr += nc;
		}
		x[i] = s0;
	}
}

#endif

/*
============
idSIMD_SSE2::ConvertJointQuatsToJointMats
============
*/
void VPCALL idSIMD_SSE2::ConvertJointQuatsToJointMats( idJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints ) {
#if 1

	assert_16_byte_aligned( jointMats );
	assert_16_byte_aligned( jointQuats );

	__asm {
		mov			eax, numJoints
		shl			eax, JOINTQUAT_SIZE_SHIFT
		mov			esi, jointQuats
		mov			edi, jointMats

		add			esi, eax
		neg			eax
		jz			done

	loopQuat:
		movaps		xmm0, [esi+eax+JOINTQUAT_Q_OFFSET]		// xmm0 =  q.x,  q.y,  q.z,  q.w
		movaps		xmm6, [esi+eax+JOINTQUAT_T_OFFSET]		// xmm6 =  t.x,  t.y,  t.z,    w

		add			edi, JOINTMAT_SIZE

		movaps		xmm1, xmm0								// xmm1 =    x,    y,    z,    w
		addps		xmm1, xmm1								// xmm1 =   x2,   y2,   z2,   w2

		add			eax, JOINTQUAT_SIZE

		// calculate the 9 products
		pshufd		xmm2, xmm0, R_SHUFFLE_D( 1, 0, 0, 1 )	// xmm2 =    y,    x,    x,    y
		pshufd		xmm3, xmm1, R_SHUFFLE_D( 1, 1, 2, 2 )	// xmm3 =   y2,   y2,   z2,   z2
		mulps		xmm2, xmm3								// xmm2 =  yy2,  xy2,  xz2,  yz2

		pshufd		xmm4, xmm0, R_SHUFFLE_D( 2, 3, 3, 3 )	// xmm4 =    z,    w,    w,    w
		pshufd		xmm5, xmm1, R_SHUFFLE_D( 2, 2, 1, 0 )	// xmm5 =   z2,   z2,   y2,   x2
		mulps		xmm4, xmm5								// xmm4 =  zz2,  wz2,  wy2,  wx2

		mulss		xmm0, xmm1								// xmm0 =  xx2,   y2,   z2,   w2

		// calculate the last two elements of the third row
		movss		xmm7, SIMD_SP_one						// xmm7 =          1,          0,          0,       0
		subss		xmm7, xmm0								// xmm7 =     -xx2+1,          0,          0,       0
		subss		xmm7, xmm2								// xmm7 = -xx2-yy2+1,          0,          0,       0
		shufps		xmm7, xmm6, R_SHUFFLE_PS( 0, 1, 2, 3 )	// xmm7 = -xx2-yy2+1,          0,        t.z,       w

		// calcluate first row
		xorps		xmm2, SIMD_SP_quat2mat_x0				// xmm2 =        yy2,       -xy2,       -xz2,    -yz2
		xorps		xmm4, SIMD_SP_quat2mat_x1				// xmm4 =       -zz2,        wz2,       -wy2,    -wx2
		addss		xmm4, SIMD_SP_one						// xmm4 =     -zz2+1,        wz2,       -wy2,    -wx2
		movaps		xmm3, xmm4								// xmm3 =     -zz2+1,        wz2,       -wy2,    -wx2
		subps		xmm3, xmm2								// xmm3 = -yy2-zz2+1,    xy2+wz2,    xz2-wy2, yz2-wx2
		movaps		[edi-JOINTMAT_SIZE+0*16+0*4], xmm3		// row0 = -yy2-zz2+1,    xy2+wz2,    xz2-wy2, yz2-wx2
		movss		[edi-JOINTMAT_SIZE+0*16+3*4], xmm6		// row0 = -yy2-zz2+1,    xy2+wz2,    xz2-wy2,     t.x

		// calculate second row
		movss		xmm2, xmm0								// xmm2 =        xx2,       -xy2,       -xz2,    -yz2
		xorps		xmm4, SIMD_SP_quat2mat_x2				// xmm4 =     -zz2+1,       -wz2,        wy2,     wx2
		subps		xmm4, xmm2								// xmm4 = -xx2-zz2+1,    xy2-wz2,    xz2+wy2, yz2+wx2
		shufps		xmm6, xmm6, R_SHUFFLE_PS( 1, 2, 3, 0 )	// xmm6 =        t.y,        t.z,          w,     t.x
		shufps		xmm4, xmm4, R_SHUFFLE_PS( 1, 0, 3, 2 )	// xmm4 =    xy2-wz2, -xx2-zz2+1,    yz2+wx2, xz2+wy2
		movaps		[edi-JOINTMAT_SIZE+1*16+0*4], xmm4		// row1 =    xy2-wz2, -xx2-zz2+1,    yz2+wx2, xz2+wy2
		movss		[edi-JOINTMAT_SIZE+1*16+3*4], xmm6		// row1 =    xy2-wz2, -xx2-zz2+1,    yz2+wx2,     t.y

		// calculate third row
		movhlps		xmm3, xmm4								// xmm3 =    yz2+wx2,    xz2+wy2,    xz2-wy2, yz2-wx2
		shufps		xmm3, xmm7, R_SHUFFLE_PS( 1, 3, 0, 2 )	// xmm3 =    xz2+wy2,    yz2-wx2, -xx2-yy2+1,     t.z
		movaps		[edi-JOINTMAT_SIZE+2*16+0*4], xmm3		// row2 =    xz2+wy2,    yz2-wx2, -xx2-yy2+1,     t.z

		jl			loopQuat

	done:
	}

#else

	for ( int i = 0; i < numJoints; i++ ) {
        const float *q = &jointQuats[i].q;
        float *m = jointMats[i].mat;

        float x2 = q[0] + q[0];
        float y2 = q[1] + q[1];
        float z2 = q[2] + q[2];
        float w2 = q[3] + q[3];

        float yy2 = q[1] * y2;
        float xy2 = q[0] * y2;
        float xz2 = q[0] * z2;
        float yz2 = q[1] * z2;

        float zz2 = q[2] * z2;
        float wz2 = q[3] * z2;
        float wy2 = q[3] * y2;
        float wx2 = q[3] * x2;

        float xx2 = q[0] * x2;

        m[0*4+0] = - yy2 - zz2 + 1.0f;
        m[0*4+1] =   xy2 + wz2;
        m[0*4+2] =   xz2 - wy2;
        m[0*4+3] = q[4];

        m[1*4+0] =   xy2 - wz2;
        m[1*4+1] = - xx2 - zz2 + 1.0f;
        m[1*4+2] =   yz2 + wx2;
        m[1*4+3] = q[5];

        m[2*4+0] =   xz2 + wy2;
        m[2*4+1] =   yz2 - wx2;
        m[2*4+2] = - xx2 - yy2 + 1.0f;
        m[2*4+3] = q[6];
	}

#endif
}

/*
============
idSIMD_SSE2::MultiplyJoints
============
*/
/*
ALIGN4_INIT4( unsigned long SIMD_SP_quatmul_xor0, IEEE_SP_ZERO, IEEE_SP_SIGN, IEEE_SP_ZERO, IEEE_SP_SIGN );
ALIGN4_INIT4( unsigned long SIMD_SP_quatmul_xor1, IEEE_SP_ZERO, IEEE_SP_ZERO, IEEE_SP_SIGN, IEEE_SP_SIGN );
ALIGN4_INIT4( unsigned long SIMD_SP_quatmul_xor2, IEEE_SP_SIGN, IEEE_SP_ZERO, IEEE_SP_ZERO, IEEE_SP_SIGN );

void VPCALL idSIMD_SSE2::MultiplyJoints( idJointQuat *result, const idJointQuat *joints1, const idJointQuat *joints2, const int numJoints ) {
#if 1

	assert_16_byte_aligned( result );
	assert_16_byte_aligned( joints1 );
	assert_16_byte_aligned( joints2 );

	__asm {

		mov			eax, numJoints
		test		eax, eax
		jz			done
		mov			ecx, joints1
		mov			edx, joints2
		mov			edi, result
		imul		eax, JOINTMAT_SIZE
		add			ecx, eax
		add			edx, eax
		add			edi, eax
		neg			eax

	loopQuat:
		movaps		xmm0, [ecx+eax]				// Ax, Ay, Az, Aw										6/1
		movaps		xmm1, [edx+eax]				// Bx, By, Bz, Bw										6/1
		pshufd		xmm2, xmm0, 0x00			// Ax, Ax, Ax, Ax										4/2
		pshufd		xmm3, xmm1, 0x1B			// BW, BZ, BY, BX										4/2
		pshufd		xmm4, xmm0, 0x55			// Ay, Ay, Ay, Ay										4/2
		pshufd		xmm5, xmm1, 0x4E			// Bz, Bw, Bx, By										4/2
		mulps		xmm2, xmm3					// Ax*Bw, Ax*Bz, Ax*By, Ax*Bx							6/2
		pshufd		xmm6, xmm0, 0xAA			// Az, Az, Az, Az										4/2
		mulps		xmm4, xmm5					// Ay*Bz, Ay*Bw, Ay*Bx, Ay*By							6/2
		pshufd		xmm7, xmm1, 0xB1			// By, Bx, Bw, Bz										4/2
		xorps		xmm2, SIMD_SP_quatmul_xor0	// Ax*Bw, -Ax*Bz, Ax*By, -Ax*Bx							4/2
		pshufd		xmm0, xmm0, 0xFF			// Aw, Aw, Aw, Aw										4/2
		mulps		xmm6, xmm7					// Az*By, Az*Bx, Az*Bw, Az*Bz							6/2
		xorps		xmm4, SIMD_SP_quatmul_xor1	// Ay*Bz, Ay*Bw, -Ay*Bx, -Ay*By							4/2
		mulps		xmm0, xmm1					// Aw*Bx, Aw*By, Aw*Bz, Aw*Bw							6/2
		addps		xmm2, xmm4					// Ax*Bw+Ay*Bz, -Ax*Bz+Ay*Bw, Ax*By-Ay*Bx, -Ax*Bx-Ay*By	4/2
		xorps		xmm6, SIMD_SP_quatmul_xor2	// -Az*By, Az*Bx, Az*Bw, -Az*Bz							4/2
		addps		xmm0, xmm2					// add to the result									4/2
		addps		xmm0, xmm6					// add to the result									4/2
		movaps		[edi+eax], xmm0				// save result											6/1
		add			eax, JOINTQUAT_SIZE
		jl			loopQuat
	}

#else

	int i;

	for ( i = 0; i < numJoints; i++ ) {
		result[i].q = joints1[i].q * joints2[i].q;
	}

#endif
}
*/

/*
============
idSIMD_SSE2::TransformJoints
============
*/
void VPCALL idSIMD_SSE2::TransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) {
#if 1

	assert_16_byte_aligned( jointMats );

	__asm {

		mov			ecx, firstJoint
		mov			eax, lastJoint
		sub			eax, ecx
		jl			done
		shl			ecx, 2									// ecx = firstJoint * 4
		mov			edi, parents
		add			edi, ecx								// edx = &parents[firstJoint]
		lea         ecx, [ecx+ecx*2]
		shl         ecx, 2									// ecx = firstJoint * JOINTMAT_SIZE
		mov			esi, jointMats							// esi = jointMats
		shl			eax, 2									// eax = ( lastJoint - firstJoint ) * 4
		add			edi, eax
		neg			eax

	loopJoint:

		mov			edx, [edi+eax]
		movaps		xmm0, [esi+ecx+ 0]						// xmm0 = m0, m1, m2, t0
		lea         edx, [edx+edx*2]
		movaps		xmm1, [esi+ecx+16]						// xmm1 = m2, m3, m4, t1
		shl         edx, 4									// edx = parents[i] * JOINTMAT_SIZE
		movaps		xmm2, [esi+ecx+32]						// xmm2 = m5, m6, m7, t2

		movaps		xmm7, [esi+edx+ 0]
		pshufd		xmm4, xmm7, R_SHUFFLE_D( 0, 0, 0, 0 )
		mulps		xmm4, xmm0
		pshufd		xmm5, xmm7, R_SHUFFLE_D( 1, 1, 1, 1 )
		mulps		xmm5, xmm1
		addps		xmm4, xmm5

		add			ecx, JOINTMAT_SIZE
		add			eax, 4

		pshufd		xmm6, xmm7, R_SHUFFLE_D( 2, 2, 2, 2 )
		mulps		xmm6, xmm2
		addps		xmm4, xmm6
		andps		xmm7, SIMD_SP_clearFirstThree
		addps		xmm4, xmm7

		movaps		[esi+ecx-JOINTMAT_SIZE+ 0], xmm4

		movaps		xmm3, [esi+edx+16]
		pshufd		xmm5, xmm3, R_SHUFFLE_D( 0, 0, 0, 0 )
		mulps		xmm5, xmm0
		pshufd		xmm6, xmm3, R_SHUFFLE_D( 1, 1, 1, 1 )
		mulps		xmm6, xmm1
		addps		xmm5, xmm6
		pshufd		xmm4, xmm3, R_SHUFFLE_D( 2, 2, 2, 2 )
		mulps		xmm4, xmm2
		addps		xmm5, xmm4
		andps		xmm3, SIMD_SP_clearFirstThree
		addps		xmm5, xmm3

		movaps		[esi+ecx-JOINTMAT_SIZE+16], xmm5

		movaps		xmm7, [esi+edx+32]
		pshufd		xmm6, xmm7, R_SHUFFLE_D( 0, 0, 0, 0 )
		mulps		xmm6, xmm0
		pshufd		xmm4, xmm7, R_SHUFFLE_D( 1, 1, 1, 1 )
		mulps		xmm4, xmm1
		addps		xmm6, xmm4
		pshufd		xmm3, xmm7, R_SHUFFLE_D( 2, 2, 2, 2 )
		mulps		xmm3, xmm2
		addps		xmm6, xmm3
		andps		xmm7, SIMD_SP_clearFirstThree
		addps		xmm6, xmm7

		movaps		[esi+ecx-JOINTMAT_SIZE+32], xmm6

		jle			loopJoint
	done:
	}

#else

	int i;

	for( i = firstJoint; i <= lastJoint; i++ ) {
		assert( parents[i] < i );
		jointMats[i] *= jointMats[parents[i]];
	}

#endif
}

/*
============
idSIMD_SSE2::UntransformJoints
============
*/
void VPCALL idSIMD_SSE2::UntransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) {
#if 1

	assert_16_byte_aligned( jointMats );

	__asm {

		mov			edx, firstJoint
		mov			eax, lastJoint
		mov			ecx, eax
		sub			eax, edx
		jl			done
		mov			esi, jointMats							// esi = jointMats
		lea         ecx, [ecx+ecx*2]
		shl         ecx, 4									// ecx = lastJoint * JOINTMAT_SIZE
		shl			edx, 2
		mov			edi, parents
		add			edi, edx								// edi = &parents[firstJoint]
		shl			eax, 2									// eax = ( lastJoint - firstJoint ) * 4

	loopJoint:

		mov			edx, [edi+eax]
		movaps		xmm0, [esi+ecx+ 0]						// xmm0 = m0, m1, m2, t0
		lea         edx, [edx+edx*2]
		movaps		xmm1, [esi+ecx+16]						// xmm1 = m2, m3, m4, t1
		shl         edx, 4									// edx = parents[i] * JOINTMAT_SIZE
		movaps		xmm2, [esi+ecx+32]						// xmm2 = m5, m6, m7, t2

		movss		xmm7, [esi+edx+12]
		shufps		xmm7, xmm7, R_SHUFFLE_PS( 1, 2, 3, 0 )
		subps		xmm0, xmm7
		movss		xmm6, [esi+edx+28]
		shufps		xmm6, xmm6, R_SHUFFLE_PS( 1, 2, 3, 0 )
		subps		xmm1, xmm6
		movss		xmm5, [esi+edx+44]
		shufps		xmm5, xmm5, R_SHUFFLE_PS( 1, 2, 3, 0 )
		subps		xmm2, xmm5

		sub			ecx, JOINTMAT_SIZE
		sub			eax, 4

		movaps		xmm7, [esi+edx+ 0]

		pshufd		xmm3, xmm7, R_SHUFFLE_D( 0, 0, 0, 0 )
		mulps		xmm3, xmm0
		pshufd		xmm4, xmm7, R_SHUFFLE_D( 1, 1, 1, 1 )
		mulps		xmm4, xmm0
		pshufd		xmm5, xmm7, R_SHUFFLE_D( 2, 2, 2, 2 )
		mulps		xmm5, xmm0

		movaps		xmm7, [esi+edx+16]

		pshufd		xmm0, xmm7, R_SHUFFLE_D( 0, 0, 0, 0 )
		mulps		xmm0, xmm1
		addps		xmm3, xmm0
		pshufd		xmm6, xmm7, R_SHUFFLE_D( 1, 1, 1, 1 )
		mulps		xmm6, xmm1
		addps		xmm4, xmm6
		pshufd		xmm0, xmm7, R_SHUFFLE_D( 2, 2, 2, 2 )
		mulps		xmm0, xmm1
		addps		xmm5, xmm0

		movaps		xmm7, [esi+edx+32]

		pshufd		xmm6, xmm7, R_SHUFFLE_D( 0, 0, 0, 0 )
		mulps		xmm6, xmm2
		addps		xmm3, xmm6

		movaps		[esi+ecx+JOINTMAT_SIZE+ 0], xmm3

		pshufd		xmm1, xmm7, R_SHUFFLE_D( 1, 1, 1, 1 )
		mulps		xmm1, xmm2
		addps		xmm4, xmm1

		movaps		[esi+ecx+JOINTMAT_SIZE+16], xmm4

		pshufd		xmm6, xmm7, R_SHUFFLE_D( 2, 2, 2, 2 )
		mulps		xmm6, xmm2
		addps		xmm5, xmm6

		movaps		[esi+ecx+JOINTMAT_SIZE+32], xmm5

		jge			loopJoint
	done:
	}

#else

	int i;

	for( i = lastJoint; i >= firstJoint; i-- ) {
		assert( parents[i] < i );
		jointMats[i] /= jointMats[parents[i]];
	}

#endif
}

/*
============
idSIMD_SSE2::MultiplyJoints
============
*/
void VPCALL idSIMD_SSE2::MultiplyJoints( idJointMat *result, const idJointMat *joints1, const idJointMat *joints2, const int numJoints ) {
#if 1

	assert_16_byte_aligned( result );
	assert_16_byte_aligned( joints1 );
	assert_16_byte_aligned( joints2 );

	__asm {

		mov			eax, numJoints
		test		eax, eax
		jz			done
		mov			ecx, joints1
		mov			edx, joints2
		mov			edi, result
		imul		eax, JOINTMAT_SIZE
		add			ecx, eax
		add			edx, eax
		add			edi, eax
		neg			eax

	loopJoint:

		movaps		xmm0, [edx+eax+ 0]
		movaps		xmm1, [edx+eax+16]
		movaps		xmm2, [edx+eax+32]

		movaps		xmm7, [ecx+eax+ 0]
		pshufd		xmm3, xmm7, R_SHUFFLE_D( 0, 0, 0, 0 )
		mulps		xmm3, xmm0
		pshufd		xmm4, xmm7, R_SHUFFLE_D( 1, 1, 1, 1 )
		mulps		xmm4, xmm1
		addps		xmm3, xmm4

		add			eax, JOINTMAT_SIZE

		pshufd		xmm5, xmm7, R_SHUFFLE_D( 2, 2, 2, 2 )
		mulps		xmm5, xmm2
		addps		xmm3, xmm5
		andps		xmm7, SIMD_SP_clearFirstThree
		addps		xmm3, xmm7

		movaps		[edi+eax-JOINTMAT_SIZE+0], xmm3

		movaps		xmm7, [ecx+eax-JOINTMAT_SIZE+16]
		pshufd		xmm3, xmm7, R_SHUFFLE_D( 0, 0, 0, 0 )
		mulps		xmm3, xmm0
		pshufd		xmm4, xmm7, R_SHUFFLE_D( 1, 1, 1, 1 )
		mulps		xmm4, xmm1
		addps		xmm3, xmm4
		pshufd		xmm5, xmm7, R_SHUFFLE_D( 2, 2, 2, 2 )
		mulps		xmm5, xmm2
		addps		xmm3, xmm5
		andps		xmm7, SIMD_SP_clearFirstThree
		addps		xmm3, xmm7

		movaps		[edi+eax-JOINTMAT_SIZE+16], xmm3

		movaps		xmm7, [ecx+eax-JOINTMAT_SIZE+32]
		pshufd		xmm3, xmm7, R_SHUFFLE_D( 0, 0, 0, 0 )
		mulps		xmm3, xmm0
		pshufd		xmm4, xmm7, R_SHUFFLE_D( 1, 1, 1, 1 )
		mulps		xmm4, xmm1
		addps		xmm3, xmm4
		pshufd		xmm5, xmm7, R_SHUFFLE_D( 2, 2, 2, 2 )
		mulps		xmm5, xmm2
		addps		xmm3, xmm5
		andps		xmm7, SIMD_SP_clearFirstThree
		addps		xmm3, xmm7

		movaps		[edi+eax-JOINTMAT_SIZE+32], xmm3

		jl			loopJoint
	done:
	}

#else

	int i;

	for ( i = 0; i < numJoints; i++ ) {
		idJointMat::Multiply( result[i], joints1[i], joints2[i] );
	}

#endif
}

#pragma warning( disable : 4731 )	// frame pointer register 'ebx' modified by inline assembly code

/*
============
idSIMD_SSE2::TransformVertsNew
============
*/
void VPCALL idSIMD_SSE2::TransformVertsNew( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) {
	ALIGN16( float tmpMin[4] );
	ALIGN16( float tmpMax[4] );

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
		movaps		tmpMin, xmm6
		movaps		tmpMax, xmm7

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

		movaps		xmm6, xmm0								// xmm6 =    m0,    m1,          m2,          t0
		unpcklps	xmm6, xmm1								// xmm6 =    m0,    m3,          m1,          m4
		unpckhps	xmm0, xmm1								// xmm1 =    m2,    m5,          t0,          t1
		addps		xmm6, xmm0								// xmm6 = m0+m2, m3+m5,       m1+t0,       m4+t1

		movaps		xmm7, xmm2								// xmm7 =    m6,    m7,          m8,          t2
		movlhps		xmm2, xmm6								// xmm2 =    m6,    m7,       m0+m2,       m3+m5
		movhlps		xmm6, xmm7								// xmm6 =    m8,    t2,       m1+t0,       m4+t1
		addps		xmm6, xmm2								// xmm6 = m6+m8, m7+t2, m0+m1+m2+t0, m3+m4+m5+t1

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0], xmm6

		pshufd		xmm5, xmm6, R_SHUFFLE_D( 1, 0, 2, 3 )	// xmm7 = m7+t2, m6+m8
		addss		xmm5, xmm6								// xmm5 = m6+m8+m7+t2
		movss		xmm6, xmm5

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8], xmm5

		movaps		xmm7, xmm6
		minps		xmm7, tmpMin
		maxps		xmm6, tmpMax
		movaps		tmpMin, xmm7
		movaps		tmpMax, xmm6

		jl			loopVert
	done:
		pop			ebx
		mov			esi, bounds
		movaps		xmm6, tmpMin
		movaps		xmm7, tmpMax
		movhps		[esi+ 0], xmm6
		movss		[esi+ 8], xmm6
		movhps		[esi+12], xmm7
		movss		[esi+20], xmm7
	}
}

/*
============
idSIMD_SSE2::TransformVertsAndTangents
============
*/
void VPCALL idSIMD_SSE2::TransformVertsAndTangents( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) {
	ALIGN16( float tmpMin[4] );
	ALIGN16( float tmpMax[4] );

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
		movaps		tmpMin, xmm6
		movaps		tmpMax, xmm7

	loopVert:
		movss		xmm0, [edx+JOINTWEIGHT_WEIGHT_OFFSET]
		mov			ebx, dword ptr [edx+JOINTWEIGHT_JOINTMATOFFSET_OFFSET]
		shufps		xmm0, xmm0, R_SHUFFLE_PS( 0, 0, 0, 0 )
		add			edx, JOINTWEIGHT_SIZE
		movaps		xmm1, xmm0
		movaps		xmm2, xmm0

		mulps		xmm0, [edi+ebx+ 0]						// xmm0 = m0, m1, m2, t0
		mulps		xmm1, [edi+ebx+16]						// xmm1 = m3, m4, m5, t1
		mulps		xmm2, [edi+ebx+32]						// xmm2 = m6, m7, m8, t2

		cmp			dword ptr [edx-JOINTWEIGHT_SIZE+JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET], JOINTWEIGHT_SIZE

		je			doneWeight

	loopWeight:
		movss		xmm3, [edx+JOINTWEIGHT_WEIGHT_OFFSET]
		mov			ebx, dword ptr [edx+JOINTWEIGHT_JOINTMATOFFSET_OFFSET]
		shufps		xmm3, xmm3, R_SHUFFLE_PS( 0, 0, 0, 0 )
		add			edx, JOINTWEIGHT_SIZE
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

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

		movaps		xmm6, xmm3								// xmm6 =    m0,    m1,          m2,          t0
		unpcklps	xmm6, xmm4								// xmm6 =    m0,    m3,          m1,          m4
		unpckhps	xmm3, xmm4								// xmm4 =    m2,    m5,          t0,          t1
		addps		xmm6, xmm3								// xmm6 = m0+m2, m3+m5,       m1+t0,       m4+t1

		movaps		xmm7, xmm5								// xmm7 =    m6,    m7,          m8,          t2
		movlhps		xmm5, xmm6								// xmm5 =    m6,    m7,       m0+m2,       m3+m5
		movhlps		xmm6, xmm7								// xmm6 =    m8,    t2,       m1+t0,       m4+t1
		addps		xmm6, xmm5								// xmm6 = m6+m8, m7+t2, m0+m1+m2+t0, m3+m4+m5+t1

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0], xmm6

		pshufd		xmm7, xmm6, R_SHUFFLE_D( 1, 0, 2, 3 )	// xmm7 = m7+t2, m6+m8
		addss		xmm7, xmm6								// xmm7 = m6+m8+m7+t2
		movss		xmm6, xmm7

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8], xmm7

		movaps		xmm5, xmm6
		minps		xmm5, tmpMin
		maxps		xmm6, tmpMax
		movaps		tmpMin, xmm5
		movaps		tmpMax, xmm6

		// transform normal
		movaps		xmm3, [esi-3*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		movaps		xmm6, xmm3								// xmm6 =    m0,    m1,          m2,          t0
		unpcklps	xmm6, xmm4								// xmm6 =    m0,    m3,          m1,          m4
		unpckhps	xmm3, xmm4								// xmm3 =    m2,    m5,          t0,          t1
		addps		xmm6, xmm3								// xmm6 = m0+m2, m3+m5,       m1+t0,       m4+t1

		movaps		xmm7, xmm5								// xmm7 =    m6,    m7,          m8,          t2
		movlhps		xmm5, xmm6								// xmm5 =    m6,    m7,       m0+m2,       m3+m5
		movhlps		xmm6, xmm7								// xmm6 =    m8,    t2,       m1+t0,       m4+t1
		addps		xmm6, xmm5								// xmm6 = m6+m8, m7+t2, m0+m1+m2+t0, m3+m4+m5+t1

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+0], xmm6

		pshufd		xmm7, xmm6, R_SHUFFLE_D( 1, 0, 2, 3 )	// xmm7 = m7+t2, m6+m8
		addss		xmm7, xmm6								// xmm7 = m6+m8+m7+t2

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+8], xmm7

		// transform first tangent
		movaps		xmm3, [esi-2*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		movaps		xmm6, xmm3								// xmm6 =    m0,    m1,          m2,          t0
		unpcklps	xmm6, xmm4								// xmm6 =    m0,    m3,          m1,          m4
		unpckhps	xmm3, xmm4								// xmm3 =    m2,    m5,          t0,          t1
		addps		xmm6, xmm3								// xmm6 = m0+m2, m3+m5,       m1+t0,       m4+t1

		movaps		xmm7, xmm5								// xmm7 =    m6,    m7,          m8,          t2
		movlhps		xmm5, xmm6								// xmm5 =    m6,    m7,       m0+m2,       m3+m5
		movhlps		xmm6, xmm7								// xmm6 =    m8,    t2,       m1+t0,       m4+t1
		addps		xmm6, xmm5								// xmm6 = m6+m8, m7+t2, m0+m1+m2+t0, m3+m4+m5+t1

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT0_OFFSET+0], xmm6

		pshufd		xmm7, xmm6, R_SHUFFLE_D( 1, 0, 2, 3 )	// xmm7 = m7+t2, m6+m8
		addss		xmm7, xmm6								// xmm7 = m6+m8+m7+t2

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT0_OFFSET+8], xmm7

		// transform second tangent
		movaps		xmm3, [esi-1*BASEVECTOR_SIZE]

		mulps		xmm0, xmm3
		mulps		xmm1, xmm3
		mulps		xmm2, xmm3

		movaps		xmm6, xmm0								// xmm6 =    m0,    m1,          m2,          t0
		unpcklps	xmm6, xmm1								// xmm6 =    m0,    m3,          m1,          m4
		unpckhps	xmm0, xmm1								// xmm1 =    m2,    m5,          t0,          t1
		addps		xmm6, xmm0								// xmm6 = m0+m2, m3+m5,       m1+t0,       m4+t1

		movaps		xmm7, xmm2								// xmm7 =    m6,    m7,          m8,          t2
		movlhps		xmm2, xmm6								// xmm2 =    m6,    m7,       m0+m2,       m3+m5
		movhlps		xmm6, xmm7								// xmm6 =    m8,    t2,       m1+t0,       m4+t1
		addps		xmm6, xmm2								// xmm6 = m6+m8, m7+t2, m0+m1+m2+t0, m3+m4+m5+t1

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT1_OFFSET+0], xmm6

		pshufd		xmm7, xmm6, R_SHUFFLE_D( 1, 0, 2, 3 )	// xmm7 = m7+t2, m6+m8
		addss		xmm7, xmm6								// xmm7 = m6+m8+m7+t2

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT1_OFFSET+8], xmm7

		jl			loopVert

	done:
		pop			ebx
		mov			esi, bounds
		movaps		xmm6, tmpMin
		movaps		xmm7, tmpMax
		movhps		[esi+ 0], xmm6
		movss		[esi+ 8], xmm6
		movhps		[esi+12], xmm7
		movss		[esi+20], xmm7
	}
}

/*
============
idSIMD_SSE2::TransformVertsAndTangentsFast
============
*/
void VPCALL idSIMD_SSE2::TransformVertsAndTangentsFast( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) {
	ALIGN16( float tmpMin[4] );
	ALIGN16( float tmpMax[4] );

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
		movaps		tmpMin, xmm6
		movaps		tmpMax, xmm7

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

		movaps		xmm6, xmm3								// xmm6 =    m0,    m1,          m2,          t0
		unpcklps	xmm6, xmm4								// xmm6 =    m0,    m3,          m1,          m4
		unpckhps	xmm3, xmm4								// xmm4 =    m2,    m5,          t0,          t1
		addps		xmm6, xmm3								// xmm6 = m0+m2, m3+m5,       m1+t0,       m4+t1

		movaps		xmm7, xmm5								// xmm7 =    m6,    m7,          m8,          t2
		movlhps		xmm5, xmm6								// xmm5 =    m6,    m7,       m0+m2,       m3+m5
		movhlps		xmm6, xmm7								// xmm6 =    m8,    t2,       m1+t0,       m4+t1
		addps		xmm6, xmm5								// xmm6 = m6+m8, m7+t2, m0+m1+m2+t0, m3+m4+m5+t1

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0], xmm6

		pshufd		xmm7, xmm6, R_SHUFFLE_D( 1, 0, 2, 3 )	// xmm7 = m7+t2, m6+m8
		addss		xmm7, xmm6								// xmm7 = m6+m8+m7+t2
		movss		xmm6, xmm7

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8], xmm7

		movaps		xmm5, xmm6
		minps		xmm5, tmpMin
		maxps		xmm6, tmpMax
		movaps		tmpMin, xmm5
		movaps		tmpMax, xmm6

		// transform normal
		movaps		xmm3, [esi-3*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		movaps		xmm6, xmm3								// xmm6 =    m0,    m1,          m2,          t0
		unpcklps	xmm6, xmm4								// xmm6 =    m0,    m3,          m1,          m4
		unpckhps	xmm3, xmm4								// xmm3 =    m2,    m5,          t0,          t1
		addps		xmm6, xmm3								// xmm6 = m0+m2, m3+m5,       m1+t0,       m4+t1

		movaps		xmm7, xmm5								// xmm7 =    m6,    m7,          m8,          t2
		movlhps		xmm5, xmm6								// xmm5 =    m6,    m7,       m0+m2,       m3+m5
		movhlps		xmm6, xmm7								// xmm6 =    m8,    t2,       m1+t0,       m4+t1
		addps		xmm6, xmm5								// xmm6 = m6+m8, m7+t2, m0+m1+m2+t0, m3+m4+m5+t1

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+0], xmm6

		pshufd		xmm7, xmm6, R_SHUFFLE_D( 1, 0, 2, 3 )	// xmm7 = m7+t2, m6+m8
		addss		xmm7, xmm6								// xmm7 = m6+m8+m7+t2

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_NORMAL_OFFSET+8], xmm7

		// transform first tangent
		movaps		xmm3, [esi-2*BASEVECTOR_SIZE]
		movaps		xmm4, xmm3
		movaps		xmm5, xmm3

		mulps		xmm3, xmm0
		mulps		xmm4, xmm1
		mulps		xmm5, xmm2

		movaps		xmm6, xmm3								// xmm6 =    m0,    m1,          m2,          t0
		unpcklps	xmm6, xmm4								// xmm6 =    m0,    m3,          m1,          m4
		unpckhps	xmm3, xmm4								// xmm3 =    m2,    m5,          t0,          t1
		addps		xmm6, xmm3								// xmm6 = m0+m2, m3+m5,       m1+t0,       m4+t1

		movaps		xmm7, xmm5								// xmm7 =    m6,    m7,          m8,          t2
		movlhps		xmm5, xmm6								// xmm5 =    m6,    m7,       m0+m2,       m3+m5
		movhlps		xmm6, xmm7								// xmm6 =    m8,    t2,       m1+t0,       m4+t1
		addps		xmm6, xmm5								// xmm6 = m6+m8, m7+t2, m0+m1+m2+t0, m3+m4+m5+t1

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT0_OFFSET+0], xmm6

		pshufd		xmm7, xmm6, R_SHUFFLE_D( 1, 0, 2, 3 )	// xmm7 = m7+t2, m6+m8
		addss		xmm7, xmm6								// xmm7 = m6+m8+m7+t2

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT0_OFFSET+8], xmm7

		// transform second tangent
		movaps		xmm3, [esi-1*BASEVECTOR_SIZE]

		mulps		xmm0, xmm3
		mulps		xmm1, xmm3
		mulps		xmm2, xmm3

		movaps		xmm6, xmm0								// xmm6 =    m0,    m1,          m2,          t0
		unpcklps	xmm6, xmm1								// xmm6 =    m0,    m3,          m1,          m4
		unpckhps	xmm0, xmm1								// xmm1 =    m2,    m5,          t0,          t1
		addps		xmm6, xmm0								// xmm6 = m0+m2, m3+m5,       m1+t0,       m4+t1

		movaps		xmm7, xmm2								// xmm7 =    m6,    m7,          m8,          t2
		movlhps		xmm2, xmm6								// xmm2 =    m6,    m7,       m0+m2,       m3+m5
		movhlps		xmm6, xmm7								// xmm6 =    m8,    t2,       m1+t0,       m4+t1
		addps		xmm6, xmm2								// xmm6 = m6+m8, m7+t2, m0+m1+m2+t0, m3+m4+m5+t1

		movhps		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT1_OFFSET+0], xmm6

		pshufd		xmm7, xmm6, R_SHUFFLE_D( 1, 0, 2, 3 )	// xmm7 = m7+t2, m6+m8
		addss		xmm7, xmm6								// xmm7 = m6+m8+m7+t2

		movss		[ecx+eax-DRAWVERT_SIZE+DRAWVERT_TANGENT1_OFFSET+8], xmm7

		jl			loopVert

	done:
		pop			ebx
		mov			esi, bounds
		movaps		xmm6, tmpMin
		movaps		xmm7, tmpMax
		movhps		[esi+ 0], xmm6
		movss		[esi+ 8], xmm6
		movhps		[esi+12], xmm7
		movss		[esi+20], xmm7
	}
}

/*
============
idSIMD_SSE2::ShadowVolume_CountFacing
============
*/
int VPCALL idSIMD_SSE2::ShadowVolume_CountFacing( const byte *facing, const int numFaces ) {
#if 1
	ALIGN16( int n[4]; )

	__asm {

		mov			eax, numFaces
		mov			edi, facing
		test		eax, eax
		jz			done

		pxor		xmm6, xmm6
		pxor		xmm7, xmm7

		sub			eax, 256
		jl			run16

	loop256:
		movdqa		xmm0, [edi+ 0*16]
		movdqa		xmm1, [edi+ 1*16]
		movdqa		xmm2, [edi+ 2*16]
		movdqa		xmm3, [edi+ 3*16]
		paddusb		xmm0, [edi+ 4*16]
		paddusb		xmm1, [edi+ 5*16]
		paddusb		xmm2, [edi+ 6*16]
		paddusb		xmm3, [edi+ 7*16]
		paddusb		xmm0, [edi+ 8*16]
		paddusb		xmm1, [edi+ 9*16]
		paddusb		xmm2, [edi+10*16]
		paddusb		xmm3, [edi+11*16]
		paddusb		xmm0, [edi+12*16]
		paddusb		xmm1, [edi+13*16]
		paddusb		xmm2, [edi+14*16]
		paddusb		xmm3, [edi+15*16]
		paddusb		xmm0, xmm1
		paddusb		xmm2, xmm3
		paddusb		xmm0, xmm2

		add			edi, 256
		sub			eax, 256

		movdqa		xmm1, xmm0
		punpcklbw	xmm0, xmm7
		punpckhbw	xmm1, xmm7
		paddusw		xmm0, xmm1
		movdqa		xmm1, xmm0
		punpcklwd	xmm0, xmm7
		punpckhwd	xmm1, xmm7
		paddd		xmm0, xmm1
		paddd		xmm6, xmm0

		jge			loop256

	run16:
		pxor		xmm0, xmm0
		add			eax, 256 - 16
		jl			run4

	loop16:
		paddusb		xmm0, [edi]
		add			edi, 16
		sub			eax, 16
		jge			loop16

	run4:
		add			eax, 16 - 4
		jl			run1

		pxor		xmm1, xmm1

	loop4:
		movd		xmm1, [edi]
		paddusb		xmm0, xmm1
		add			edi, 4
		sub			eax, 4
		jge			loop4

	run1:
		movdqa		xmm1, xmm0
		punpcklbw	xmm0, xmm7
		punpckhbw	xmm1, xmm7
		paddusw		xmm0, xmm1
		movdqa		xmm1, xmm0
		punpcklwd	xmm0, xmm7
		punpckhwd	xmm1, xmm7
		paddd		xmm0, xmm1
		paddd		xmm6, xmm0

		movdqa		n, xmm6
		add			eax, 4-1
		jl			done

		mov			edx, dword ptr n

	loop1:
		movzx		ecx, [edi]
		add			edx, ecx
		add			edi, 1
		sub			eax, 1
		jge			loop1

		mov			dword ptr n, edx

	done:
	}

	return n[0] + n[1] + n[2] + n[3];

#else

	int i, n0, n1, n2, n3;

	n0 = n1 = n2 = n3 = 0;
	for ( i = 0; i < numFaces - 3; i += 4 ) {
		n0 += facing[i+0];
		n1 += facing[i+1];
		n2 += facing[i+2];
		n3 += facing[i+3];
	}
	for ( ; i < numFaces; i++ ) {
		n0 += facing[i];
	}
	return n0 + n1 + n2 + n3;

#endif
}

/*
============
idSIMD_Generic::ShadowVolume_CountFacingCull
============
*/
#pragma warning( disable : 4731 )	// frame pointer register 'ebx' modified by inline assembly code

int VPCALL idSIMD_SSE2::ShadowVolume_CountFacingCull( byte *facing, const int numFaces, const int *indexes, const byte *cull ) {
#if 1

	ALIGN16( int n[4]; )

	__asm {
		push		ebx
		mov			eax, numFaces
		mov			esi, indexes
		mov			edi, cull
		mov			ebx, facing
		test		eax, eax
		jz			done
		add			ebx, eax
		neg			eax

		pxor		xmm5, xmm5
		pxor		xmm6, xmm6
		movdqa		xmm7, SIMD_B_one

		add			eax, 4
		jg			run1

	loop4:

		mov			ecx, dword ptr [esi+0*4]
		movzx		edx, byte ptr [edi+ecx]
		mov			ecx, dword ptr [esi+1*4]
		and			dl, byte ptr [edi+ecx]
		mov			ecx, dword ptr [esi+2*4]
		and			dl, byte ptr [edi+ecx]

		mov			ecx, dword ptr [esi+3*4]
		mov			dh, byte ptr [edi+ecx]
		mov			ecx, dword ptr [esi+4*4]
		and			dh, byte ptr [edi+ecx]
		mov			ecx, dword ptr [esi+5*4]
		and			dh, byte ptr [edi+ecx]
		movd		xmm0, edx

		mov			ecx, dword ptr [esi+6*4]
		movzx		edx, byte ptr [edi+ecx]
		mov			ecx, dword ptr [esi+7*4]
		and			dl, byte ptr [edi+ecx]
		mov			ecx, dword ptr [esi+8*4]
		and			dl, byte ptr [edi+ecx]

		mov			ecx, dword ptr [esi+9*4]
		mov			dh, byte ptr [edi+ecx]
		mov			ecx, dword ptr [esi+10*4]
		and			dh, byte ptr [edi+ecx]
		mov			ecx, dword ptr [esi+11*4]
		and			dh, byte ptr [edi+ecx]
		pinsrw		xmm0, edx, 1

		add			esi, 12*4

		movd		xmm1, [ebx+eax-4]
		pcmpgtb		xmm0, xmm6
		pand		xmm0, xmm7
		por			xmm1, xmm0
		movd		[ebx+eax-4], xmm1

		add			eax, 4

		punpcklbw	xmm1, xmm6
		punpcklwd	xmm1, xmm6
		paddd		xmm5, xmm1

		jle			loop4

	run1:
		sub			eax, 4
		jge			done

	loop1:
		mov			ecx, dword ptr [esi+0*4]
		movzx		edx, byte ptr [edi+ecx]
		mov			ecx, dword ptr [esi+1*4]
		and			dl, byte ptr [edi+ecx]
		mov			ecx, dword ptr [esi+2*4]
		and			dl, byte ptr [edi+ecx]

		neg			edx
		shr			edx, 31
		movzx		ecx, byte ptr [ebx+eax]
		or			ecx, edx
		mov			byte ptr [ebx+eax], cl
		movd		xmm0, ecx
		paddd		xmm5, xmm0

		add			esi, 3*4
		add			eax, 1
		jl			loop1

	done:
		pop			ebx
		movdqa		dword ptr n, xmm5
	}

	return n[0] + n[1] + n[2] + n[3];

#else

	int i, n;

	n = 0;
	for ( i = 0; i < numFaces - 3; i += 4 ) {
		int c0 = cull[indexes[0]] & cull[indexes[ 1]] & cull[indexes[ 2]];
		int c1 = cull[indexes[3]] & cull[indexes[ 4]] & cull[indexes[ 5]];
		int c2 = cull[indexes[6]] & cull[indexes[ 7]] & cull[indexes[ 8]];
		int c3 = cull[indexes[9]] & cull[indexes[10]] & cull[indexes[11]];
		facing[i+0] |= ( (-c0) >> 31 ) & 1;
		facing[i+1] |= ( (-c1) >> 31 ) & 1;
		facing[i+2] |= ( (-c2) >> 31 ) & 1;
		facing[i+3] |= ( (-c3) >> 31 ) & 1;
		n += facing[i+0];
		n += facing[i+1];
		n += facing[i+2];
		n += facing[i+3];
		indexes += 12;
	}
	for ( ; i < numFaces; i++ ) {
		int c = cull[indexes[0]] & cull[indexes[1]] & cull[indexes[2]];
		c = ( (-c) >> 31 ) & 1;
		facing[i] |= c;
		n += facing[i];
		indexes += 3;
	}
	return n;

#endif
}

/*
============
idSIMD_SSE2::ShadowVolume_CreateSilTriangles
============
*/
int VPCALL idSIMD_SSE2::ShadowVolume_CreateSilTriangles( int *shadowIndexes, const byte *facing, const silEdge_s *silEdges, const int numSilEdges ) {
#if 1

	int num;

	__asm {
		push		ebx
		mov			eax, numSilEdges
		mov			ebx, shadowIndexes
		mov			esi, facing
		mov			edi, silEdges
		shl			eax, 4
		jz			done
		add			edi, eax
		neg			eax
		shr			ebx, 3

		add			eax, 4*16
		jg			run1

	loop4:
		mov			ecx, dword ptr [edi+eax-4*16+0]
		movzx		ecx, byte ptr [esi+ecx]
		movd		xmm2, ecx
		mov			edx, dword ptr [edi+eax-4*16+4]
		movzx		edx, byte ptr [esi+edx]
		pinsrw		xmm2, edx, 2
		movq		xmm0, qword ptr [edi+eax-4*16+8]
		paddd		xmm0, xmm0								//
		pshufd		xmm1, xmm2, R_SHUFFLE_PS( 2, 0, 1, 1 )
		xor			ecx, edx
		pshufd		xmm0, xmm0, R_SHUFFLE_PS( 0, 1, 1, 0 )
		lea			edx, [ecx*2+ecx]
		pxor		xmm0, xmm1
		add			edx, ebx
		movlps		qword ptr [ebx*8+0*4], xmm0
		pxor		xmm2, xmm0
		movhps		qword ptr [ebx*8+2*4], xmm0
		movlps		qword ptr [ebx*8+4*4], xmm2

		mov			ecx, dword ptr [edi+eax-3*16+0]
		movzx		ecx, byte ptr [esi+ecx]
		movd		xmm3, ecx
		mov			ebx, dword ptr [edi+eax-3*16+4]
		movzx		ebx, byte ptr [esi+ebx]
		pinsrw		xmm3, ebx, 2
		movq		xmm0, qword ptr [edi+eax-3*16+8]
		paddd		xmm0, xmm0								//
		pshufd		xmm1, xmm3, R_SHUFFLE_PS( 2, 0, 1, 1 )
		xor			ecx, ebx
		pshufd		xmm0, xmm0, R_SHUFFLE_PS( 0, 1, 1, 0 )
		lea			ebx, [ecx*2+ecx]
		pxor		xmm0, xmm1
		add			ebx, edx
		movlps		qword ptr [edx*8+0*4], xmm0
		pxor		xmm3, xmm0
		movhps		qword ptr [edx*8+2*4], xmm0
		movlps		qword ptr [edx*8+4*4], xmm3

		mov			ecx, dword ptr [edi+eax-2*16+0]
		movzx		ecx, byte ptr [esi+ecx]
		movd		xmm2, ecx
		mov			edx, dword ptr [edi+eax-2*16+4]
		movzx		edx, byte ptr [esi+edx]
		pinsrw		xmm2, edx, 2
		movq		xmm0, qword ptr [edi+eax-2*16+8]
		paddd		xmm0, xmm0								//
		pshufd		xmm1, xmm2, R_SHUFFLE_PS( 2, 0, 1, 1 )
		xor			ecx, edx
		pshufd		xmm0, xmm0, R_SHUFFLE_PS( 0, 1, 1, 0 )
		lea			edx, [ecx*2+ecx]
		pxor		xmm0, xmm1
		add			edx, ebx
		movlps		qword ptr [ebx*8+0*4], xmm0
		pxor		xmm2, xmm0
		movhps		qword ptr [ebx*8+2*4], xmm0
		movlps		qword ptr [ebx*8+4*4], xmm2

		mov			ecx, dword ptr [edi+eax-1*16+0]
		movzx		ecx, byte ptr [esi+ecx]
		movd		xmm3, ecx
		mov			ebx, dword ptr [edi+eax-1*16+4]
		movzx		ebx, byte ptr [esi+ebx]
		pinsrw		xmm3, ebx, 2
		movq		xmm0, qword ptr [edi+eax-1*16+8]
		paddd		xmm0, xmm0								//
		pshufd		xmm1, xmm3, R_SHUFFLE_PS( 2, 0, 1, 1 )
		xor			ecx, ebx
		pshufd		xmm0, xmm0, R_SHUFFLE_PS( 0, 1, 1, 0 )
		lea			ebx, [ecx*2+ecx]
		pxor		xmm0, xmm1
		add			ebx, edx
		movlps		qword ptr [edx*8+0*4], xmm0
		pxor		xmm3, xmm0
		movhps		qword ptr [edx*8+2*4], xmm0
		add			eax, 4*16
		movlps		qword ptr [edx*8+4*4], xmm3

		jle			loop4

	run1:
		sub			eax, 4*16
		jge			done

	loop1:
		mov			ecx, dword ptr [edi+eax+0]
		movzx		ecx, byte ptr [esi+ecx]
		movd		xmm2, ecx
		mov			edx, dword ptr [edi+eax+4]
		movzx		edx, byte ptr [esi+edx]
		pinsrw		xmm2, edx, 2
		movq		xmm0, qword ptr [edi+eax+8]
		paddd		xmm0, xmm0								//
		pshufd		xmm1, xmm2, R_SHUFFLE_PS( 2, 0, 1, 1 )
		pshufd		xmm0, xmm0, R_SHUFFLE_PS( 0, 1, 1, 0 )
		pxor		xmm0, xmm1
		movlps		qword ptr [ebx*8+0*4], xmm0
		movhps		qword ptr [ebx*8+2*4], xmm0
		pxor		xmm2, xmm0
		movlps		qword ptr [ebx*8+4*4], xmm2
		xor			ecx, edx
		lea			edx, [ecx*2+ecx]
		add			ebx, edx

		add			eax, 16
		jl			loop1

	done:
		shl			ebx, 3
		mov			num, ebx
		pop			ebx
	}

	return ( num - (int)shadowIndexes ) >> 2;

#else

	int i;
	const silEdge_t *sil;
	int *si;

	si = shadowIndexes;
	for ( sil = silEdges, i = numSilEdges; i > 0; i--, sil++ ) {

		int f1 = facing[sil->p1];
		int f2 = facing[sil->p2];

		if ( !( f1 ^ f2 ) ) {
			continue;
		}

		int v1 = sil->v1 << 1;
		int v2 = sil->v2 << 1;

		// set the two triangle winding orders based on facing
		// without using a poorly-predictable branch

		si[0] = v1;
		si[1] = v2 ^ f1;
		si[2] = v2 ^ f2;
		si[3] = v1 ^ f2;
		si[4] = v1 ^ f1;
		si[5] = v2 ^ 1;

		si += 6;
	}
	return si - shadowIndexes;

#endif
}

/*
============
idSIMD_SSE2::ShadowVolume_CreateCapTriangles
============
*/
int VPCALL idSIMD_SSE2::ShadowVolume_CreateCapTriangles( int *shadowIndexes, const byte *facing, const int *indexes, const int numIndexes ) {
#if 1

	int num = numIndexes / 3;

	__asm {
		push		ebx
		mov			eax, numIndexes
		mov			ebx, shadowIndexes
		mov			esi, facing
		mov			edi, indexes
		shl			eax, 2
		jz			done
		add			edi, eax
		mov			eax, num
		add			esi, eax
		neg			eax
		shr			ebx, 3

		movaps		xmm6, SIMD_DW_capTris_c0
		movaps		xmm7, SIMD_DW_capTris_c1
		movaps		xmm5, SIMD_DW_capTris_c2

		add			eax, 4
		lea			edx, [eax*2+eax]
		jg			run1

	loop4:
		movdqa		xmm0, [edi+edx*4-4*3*4+0]					// xmm0 =  0,  1,  2,  3
		paddd		xmm0, xmm0
		pshufd		xmm1, xmm0, R_SHUFFLE_PS( 2, 1, 0, 0 )		// xmm1 =  2,  1,  0,  0
		movzx		ecx, byte ptr [esi+eax-4]
		pshufd		xmm2, xmm0, R_SHUFFLE_PS( 1, 2, 1, 2 )		// xmm2 =  1,  2,  1,  2
		sub			ecx, 1
		pxor		xmm1, xmm6
		and			ecx, 3
		movlps		qword ptr [ebx*8+0*4], xmm1
		add			ecx, ebx
		movhps		qword ptr [ebx*8+2*4], xmm1
		pxor		xmm2, xmm7
		movlps		qword ptr [ebx*8+4*4], xmm2

		movdqa		xmm3, [edi+edx*4-3*3*4+4]					// xmm3 =  4,  5,  6,  7
		paddd		xmm3, xmm3
		shufps		xmm0, xmm3, R_SHUFFLE_PS( 3, 3, 1, 0 )		// xmm0 =  3   3,  5,  4
		movzx		ebx, byte ptr [esi+eax-3]
		movdqa		xmm2, xmm3									// xmm2 =  4,  5,  6,  7
		sub			ebx, 1
		pxor		xmm0, xmm5
		and			ebx, 3
		movhps		qword ptr [ecx*8+0*4], xmm0
		add			ebx, ecx
		movlps		qword ptr [ecx*8+2*4], xmm0
		pxor		xmm2, xmm7
		movlps		qword ptr [ecx*8+4*4], xmm2

		movdqa		xmm0, [edi+edx*4-1*3*4-4]					// xmm0 =  8,  9, 10, 11
		paddd		xmm0, xmm0
		shufps		xmm3, xmm0, R_SHUFFLE_PS( 2, 3, 0, 1 )		// xmm3 =  6,  7,  8,  9
		pshufd		xmm1, xmm3, R_SHUFFLE_PS( 2, 1, 0, 0 )		// xmm1 =  8,  7,  6,  6
		movzx		ecx, byte ptr [esi+eax-2]
		pshufd		xmm2, xmm3, R_SHUFFLE_PS( 1, 2, 1, 2 )		// xmm2 =  7,  8,  7,  8
		sub			ecx, 1
		pxor		xmm1, xmm6
		and			ecx, 3
		movlps		qword ptr [ebx*8+0*4], xmm1
		add			ecx, ebx
		movhps		qword ptr [ebx*8+2*4], xmm1
		pxor		xmm2, xmm7
		movlps		qword ptr [ebx*8+4*4], xmm2

		pshufd		xmm1, xmm0, R_SHUFFLE_PS( 3, 2, 1, 1 )
		movzx		ebx, byte ptr [esi+eax-1]
		pshufd		xmm2, xmm0, R_SHUFFLE_PS( 2, 3, 2, 3 )
		sub			ebx, 1
		pxor		xmm1, xmm6
		and			ebx, 3
		movlps		qword ptr [ecx*8+0*4], xmm1
		add			ebx, ecx
		movhps		qword ptr [ecx*8+2*4], xmm1
		pxor		xmm2, xmm7
		movlps		qword ptr [ecx*8+4*4], xmm2

		add			edx, 3*4
		add			eax, 4
		jle			loop4

	run1:
		sub			eax, 4
		jge			done

	loop1:
		lea			edx, [eax*2+eax]
		movdqu		xmm0, [edi+edx*4+0]
		paddd		xmm0, xmm0
		pshufd		xmm1, xmm0, R_SHUFFLE_PS( 2, 1, 0, 0 )
		pshufd		xmm2, xmm0, R_SHUFFLE_PS( 1, 2, 1, 2 )
		pxor		xmm1, xmm6
		movlps		qword ptr [ebx*8+0*4], xmm1
		pxor		xmm2, xmm7
		movhps		qword ptr [ebx*8+2*4], xmm1
		movzx		ecx, byte ptr [esi+eax]
		movlps		qword ptr [ebx*8+4*4], xmm2
		sub			ecx, 1
		and			ecx, 3
		add			ebx, ecx

		add			eax, 1
		jl			loop1

	done:
		shl			ebx, 3
		mov			num, ebx
		pop			ebx
	}

	return ( num - (int)shadowIndexes ) >> 2;

#else

	int i, j;
	int *si;

	si = shadowIndexes;
	for ( i = 0, j = 0; i < numIndexes; i += 3, j++ ) {
		if ( facing[j] ) {
			continue;
		}

		int i0 = indexes[i+0];
		int i1 = indexes[i+1];
		int i2 = indexes[i+2];

		i0 += i0;
		i1 += i1;
		i2 += i2;

		si[0] = i2;
		si[1] = i1;
		si[2] = i0;
		si[3] = i0 ^ 1;
		si[4] = i1 ^ 1;
		si[5] = i2 ^ 1;

		si += 6;
	}
	return si - shadowIndexes;

#endif
}

/*
============
idSIMD_SSE2::MixedSoundToSamples
============
*/
void VPCALL idSIMD_SSE2::MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples ) {

	assert( ( numSamples % MIXBUFFER_SAMPLES ) == 0 );

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

#endif /* _WINDOWS */
