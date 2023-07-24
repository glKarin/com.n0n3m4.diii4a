// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

// Gordon: FIXME: This isn't initialised, so wont actually be zero?
idBoundsShort bounds_short_zero;

/*
================
idBoundsShort::PlaneDistance
================
*/
float idBoundsShort::PlaneDistance( const idPlane &plane ) const {
	// Gordon: FIXME: Don't convert to float?
	const idVec3 center( ( b[ 0 ][ 0 ] + b[ 1 ][ 0 ] ) * 0.5f,
						( b[ 0 ][ 1 ] + b[ 1 ][ 1 ] ) * 0.5f,
						( b[ 0 ][ 2 ] + b[ 1 ][ 2 ] ) * 0.5f );

	const idVec3 planeNormal = plane.Normal();
	const float planeD = plane[ 3 ];

	const float d2 = idMath::Fabs( ( b[1][0] - center[0] ) * planeNormal[0] ) +
					idMath::Fabs( ( b[1][1] - center[1] ) * planeNormal[1] ) +
					idMath::Fabs( ( b[1][2] - center[2] ) * planeNormal[2] );
	const float d1 = center*planeNormal + planeD;

	if ( d1 - d2 > 0.0f ) {
		return d1 - d2;
	}
	if ( d1 + d2 < 0.0f ) {
		return d1 + d2;
	}
	return 0.0f;
}

/*
================
idBoundsShort::PlaneSide
================
*/
#if defined( ID_WIN_X86_SSE )
#include "../math/Simd_InstructionMacros.h"
ALIGN4_INIT1( float SIMD_SP_half, 0.5f );
ALIGN4_INIT1( unsigned int SIMD_SP_absMask, ~(1<<31) );
#endif

int idBoundsShort::PlaneSide( const idPlane &plane, const float epsilon ) const {
#if defined( ID_WIN_X86_SSE )
	__asm {
		mov			edi, this
		mov			esi, plane

		movss		xmm1, [esi+ 0]
		movhps		xmm1, [esi+ 4]
		movss		xmm0, [esi+ 12]

		movsx		eax, word ptr [edi + 0]
		movsx		ebx, word ptr [edi + 2]
		movsx		ecx, word ptr [edi + 4]
		cvtsi2ss	xmm2, eax
		cvtsi2ss	xmm3, ebx
		cvtsi2ss	xmm4, ecx

		movsx		eax, word ptr [edi + 6]
		movsx		ebx, word ptr [edi + 8]
		movsx		ecx, word ptr [edi + 10]
		cvtsi2ss	xmm5, eax
		cvtsi2ss	xmm6, ebx
		cvtsi2ss	xmm7, ecx

		unpcklps	xmm3, xmm4
		movlhps		xmm2, xmm3

		unpcklps	xmm6, xmm7
		movlhps		xmm5, xmm6

		movaps		xmm7, xmm2
		addps		xmm7, xmm5
		mulps		xmm7, SIMD_SP_half

		//float d1 = plane.Distance( center );
		movaps		xmm2, xmm7
		mulps		xmm2, xmm1
		movhlps		xmm3, xmm2
		movaps		xmm4, xmm3
		shufps		xmm4, xmm4, SHUFFLE_PS( 1, 1, 1, 1 )
		
		addss		xmm2, xmm3
		addss		xmm2, xmm4
		addss		xmm2, xmm0
	
		// idMath::Fabs( ( b[1] - center ) * plane.Normal() )
		subps		xmm5, xmm7
		mulps		xmm5, xmm1
		andps		xmm5, SIMD_SP_absMask

		// add
		movhlps		xmm4, xmm5
		movaps		xmm3, xmm4
		shufps		xmm3, xmm3, SHUFFLE_PS( 1, 1, 1, 1 )

		addss		xmm5, xmm4
		addss		xmm5, xmm3

		// xmm5 contains final idMath::Fabs( ( b[1] - center ) * plane.Normal() )

		movss		xmm0, xmm2
		subss		xmm0, xmm5

		movss		xmm1, xmm2
		addss		xmm1, xmm5

		movss		xmm2, epsilon
		xorps		xmm3, xmm3
		subps		xmm3, xmm2

		cmpltps		xmm2, xmm0
		cmpltps		xmm1, xmm3

		movmskps 	eax, xmm2
		movmskps 	ecx, xmm1

		and eax, 1
		jz nexttest1
		mov eax, PLANESIDE_FRONT
		jmp exity

nexttest1:
		and ecx, 1
		jz nexttest2
		mov eax, PLANESIDE_BACK
		jmp exity

nexttest2:
		mov eax, PLANESIDE_CROSS

exity:
	}
	//float d1 = plane.Distance( center );
#else
	// Gordon: FIXME: Don't convert to float?
	idVec3 center;
	center[ 0 ] = ( b[ 0 ][ 0 ] + b[ 1 ][ 0 ] ) * 0.5f;
	center[ 1 ] = ( b[ 0 ][ 1 ] + b[ 1 ][ 1 ] ) * 0.5f;
	center[ 2 ] = ( b[ 0 ][ 2 ] + b[ 1 ][ 2 ] ) * 0.5f;

	float d1 = plane.Distance( center );
	float d2 = idMath::Fabs( ( b[1][0] - center[0] ) * plane.Normal()[0] ) +
			idMath::Fabs( ( b[1][1] - center[1] ) * plane.Normal()[1] ) +
				idMath::Fabs( ( b[1][2] - center[2] ) * plane.Normal()[2] );

	if ( d1 - d2 > epsilon ) {
		return PLANESIDE_FRONT;
	}
	if ( d1 + d2 < -epsilon ) {
		return PLANESIDE_BACK;
	}
	return PLANESIDE_CROSS;
#endif
}
