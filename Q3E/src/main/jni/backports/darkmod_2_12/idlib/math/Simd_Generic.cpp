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

#include "Simd_Generic.h"


//===============================================================
//
//	Generic implementation of idSIMDProcessor
//
//===============================================================

#define UNROLL1(Y) { int _IX; for (_IX=0;_IX<count;_IX++) {Y(_IX);} }
#define UNROLL2(Y) { int _IX, _NM = count&0xfffffffe; for (_IX=0;_IX<_NM;_IX+=2){Y(_IX+0);Y(_IX+1);} if (_IX < count) {Y(_IX);}}
#define UNROLL4(Y) { int _IX, _NM = count&0xfffffffc; for (_IX=0;_IX<_NM;_IX+=4){Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);}for(;_IX<count;_IX++){Y(_IX);}}
#define UNROLL8(Y) { int _IX, _NM = count&0xfffffff8; for (_IX=0;_IX<_NM;_IX+=8){Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);Y(_IX+4);Y(_IX+5);Y(_IX+6);Y(_IX+7);} _NM = count&0xfffffffe; for(;_IX<_NM;_IX+=2){Y(_IX); Y(_IX+1);} if (_IX < count) {Y(_IX);} }

#ifdef _DEBUG
#define NODEFAULT	default: assert( 0 )
#elif _WIN32
#define NODEFAULT	default: __assume( 0 )
#else
#define NODEFAULT
#endif


idSIMD_Generic::idSIMD_Generic() {
	name = "Generic";
}
const char * idSIMD_Generic::GetName( void ) const {
	return name.c_str();
}

/*
============
idSIMD_Generic::Add

  dst[i] = constant + src[i];
============
*/
void idSIMD_Generic::Add( float *dst, const float constant, const float *src, const int count ) {
#define OPER(X) dst[(X)] = src[(X)] + constant;
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Add

  dst[i] = src0[i] + src1[i];
============
*/
void idSIMD_Generic::Add( float *dst, const float *src0, const float *src1, const int count ) {
#define OPER(X) dst[(X)] = src0[(X)] + src1[(X)];
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Sub

  dst[i] = constant - src[i];
============
*/
void idSIMD_Generic::Sub( float *dst, const float constant, const float *src, const int count ) {
	double c = constant;
#define OPER(X) dst[(X)] = c - src[(X)];
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Sub

  dst[i] = src0[i] - src1[i];
============
*/
void idSIMD_Generic::Sub( float *dst, const float *src0, const float *src1, const int count ) {
#define OPER(X) dst[(X)] = src0[(X)] - src1[(X)];
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Mul

  dst[i] = constant * src[i];
============
*/
void idSIMD_Generic::Mul( float *dst, const float constant, const float *src0, const int count) {
	double c = constant;
#define OPER(X) (dst[(X)] = (c * src0[(X)]))
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Mul

  dst[i] = src0[i] * src1[i];
============
*/
void idSIMD_Generic::Mul( float *dst, const float *src0, const float *src1, const int count ) {
#define OPER(X) (dst[(X)] = src0[(X)] * src1[(X)])
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Div

  dst[i] = constant / divisor[i];
============
*/
void idSIMD_Generic::Div( float *dst, const float constant, const float *divisor, const int count ) {
	double c = constant;
#define OPER(X) (dst[(X)] = (c / divisor[(X)]))
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Div

  dst[i] = src0[i] / src1[i];
============
*/
void idSIMD_Generic::Div( float *dst, const float *src0, const float *src1, const int count ) {
#define OPER(X) (dst[(X)] = src0[(X)] / src1[(X)])
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MulAdd

  dst[i] += constant * src[i];
============
*/
void idSIMD_Generic::MulAdd( float *dst, const float constant, const float *src, const int count ) {
	double c = constant;
#define OPER(X) (dst[(X)] += c * src[(X)])
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MulAdd

  dst[i] += src0[i] * src1[i];
============
*/
void idSIMD_Generic::MulAdd( float *dst, const float *src0, const float *src1, const int count ) {
#define OPER(X) (dst[(X)] += src0[(X)] * src1[(X)])
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MulSub

  dst[i] -= constant * src[i];
============
*/
void idSIMD_Generic::MulSub( float *dst, const float constant, const float *src, const int count ) {
	double c = constant;
#define OPER(X) (dst[(X)] -= c * src[(X)])
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MulSub

  dst[i] -= src0[i] * src1[i];
============
*/
void idSIMD_Generic::MulSub( float *dst, const float *src0, const float *src1, const int count ) {
#define OPER(X) (dst[(X)] -= src0[(X)] * src1[(X)])
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Dot

  dst[i] = constant * src[i];
============
*/
void idSIMD_Generic::Dot( float *dst, const idVec3 &constant, const idVec3 *src, const int count ) {
#define OPER(X) dst[(X)] = constant * src[(X)];
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Dot

  dst[i] = constant * src[i].Normal() + src[i][3];
============
*/
void idSIMD_Generic::Dot( float *dst, const idVec3 &constant, const idPlane *src, const int count ) {
#define OPER(X) dst[(X)] = constant * src[(X)].Normal() + src[(X)][3];
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Dot

  dst[i] = constant * src[i].xyz;
============
*/
void idSIMD_Generic::Dot( float *dst, const idVec3 &constant, const idDrawVert *src, const int count ) {
#define OPER(X) dst[(X)] = constant * src[(X)].xyz;
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Dot

  dst[i] = constant.Normal() * src[i] + constant[3];
============
*/
void idSIMD_Generic::Dot( float *dst, const idPlane &constant, const idVec3 *src, const int count ) {
#define OPER(X) dst[(X)] = constant.Normal() * src[(X)] + constant[3];
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Dot

  dst[i] = constant.Normal() * src[i].Normal() + constant[3] * src[i][3];
============
*/
void idSIMD_Generic::Dot( float *dst, const idPlane &constant, const idPlane *src, const int count ) {
#define OPER(X) dst[(X)] = constant.Normal() * src[(X)].Normal() + constant[3] * src[(X)][3];
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Dot

  dst[i] = constant.Normal() * src[i].xyz + constant[3];
============
*/
void idSIMD_Generic::Dot( float *dst, const idPlane &constant, const idDrawVert *src, const int count ) {
#define OPER(X) dst[(X)] = constant.Normal() * src[(X)].xyz + constant[3];
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Dot

  dst[i] = src0[i] * src1[i];
============
*/
void idSIMD_Generic::Dot( float *dst, const idVec3 *src0, const idVec3 *src1, const int count ) {
#define OPER(X) dst[(X)] = src0[(X)] * src1[(X)];
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Dot

  dot = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2] + ...
============
*/
void idSIMD_Generic::Dot( float &dot, const float *src1, const float *src2, const int count ) {
#if 1

	switch( count ) {
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
			for ( i = 4; i < count-7; i += 8 ) {
				s0 += src1[i+0] * src2[i+0];
				s1 += src1[i+1] * src2[i+1];
				s2 += src1[i+2] * src2[i+2];
				s3 += src1[i+3] * src2[i+3];
				s0 += src1[i+4] * src2[i+4];
				s1 += src1[i+5] * src2[i+5];
				s2 += src1[i+6] * src2[i+6];
				s3 += src1[i+7] * src2[i+7];
			}
			switch( count - i ) {
				NODEFAULT;
				case 7: s0 += src1[i+6] * src2[i+6];
				case 6: s1 += src1[i+5] * src2[i+5];
				case 5: s2 += src1[i+4] * src2[i+4];
				case 4: s3 += src1[i+3] * src2[i+3];
				case 3: s0 += src1[i+2] * src2[i+2];
				case 2: s1 += src1[i+1] * src2[i+1];
				case 1: s2 += src1[i+0] * src2[i+0];
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
	for ( i = 0; i < count; i++ ) {
		dot += src1[i] * src2[i];
	}

#endif
}

/*
============
idSIMD_Generic::CmpGT

  dst[i] = src0[i] > constant;
============
*/
void idSIMD_Generic::CmpGT( byte *dst, const float *src0, const float constant, const int count ) {
#define OPER(X) dst[(X)] = src0[(X)] > constant;
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::CmpGT

  dst[i] |= ( src0[i] > constant ) << bitNum;
============
*/
void idSIMD_Generic::CmpGT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
#define OPER(X) dst[(X)] |= ( src0[(X)] > constant ) << bitNum;
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::CmpGE

  dst[i] = src0[i] >= constant;
============
*/
void idSIMD_Generic::CmpGE( byte *dst, const float *src0, const float constant, const int count ) {
#define OPER(X) dst[(X)] = src0[(X)] >= constant;
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::CmpGE

  dst[i] |= ( src0[i] >= constant ) << bitNum;
============
*/
void idSIMD_Generic::CmpGE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
#define OPER(X) dst[(X)] |= ( src0[(X)] >= constant ) << bitNum;
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::CmpLT

  dst[i] = src0[i] < constant;
============
*/
void idSIMD_Generic::CmpLT( byte *dst, const float *src0, const float constant, const int count ) {
#define OPER(X) dst[(X)] = src0[(X)] < constant;
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::CmpLT

  dst[i] |= ( src0[i] < constant ) << bitNum;
============
*/
void idSIMD_Generic::CmpLT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
#define OPER(X) dst[(X)] |= ( src0[(X)] < constant ) << bitNum;
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::CmpLE

  dst[i] = src0[i] <= constant;
============
*/
void idSIMD_Generic::CmpLE( byte *dst, const float *src0, const float constant, const int count ) {
#define OPER(X) dst[(X)] = src0[(X)] <= constant;
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::CmpLE

  dst[i] |= ( src0[i] <= constant ) << bitNum;
============
*/
void idSIMD_Generic::CmpLE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
#define OPER(X) dst[(X)] |= ( src0[(X)] <= constant ) << bitNum;
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MinMax
============
*/
void idSIMD_Generic::MinMax( float &min, float &max, const float *src, const int count ) {
	min = idMath::INFINITY; max = -idMath::INFINITY;
#define OPER(X) if ( src[(X)] < min ) {min = src[(X)];} if ( src[(X)] > max ) {max = src[(X)];}
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MinMax
============
*/
void idSIMD_Generic::MinMax( idVec2 &min, idVec2 &max, const idVec2 *src, const int count ) {
	min[0] = min[1] = idMath::INFINITY; max[0] = max[1] = -idMath::INFINITY;
#define OPER(X) const idVec2 &v = src[(X)]; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; }
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MinMax
============
*/
void idSIMD_Generic::MinMax( idVec3 &min, idVec3 &max, const idVec3 *src, const int count ) {
	min[0] = min[1] = min[2] = idMath::INFINITY; max[0] = max[1] = max[2] = -idMath::INFINITY;
#define OPER(X) const idVec3 &v = src[(X)]; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MinMax
============
*/
void idSIMD_Generic::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int count ) {
	min[0] = min[1] = min[2] = idMath::INFINITY; max[0] = max[1] = max[2] = -idMath::INFINITY;
#define OPER(X) const idVec3 &v = src[(X)].xyz; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MinMax
============
*/
void idSIMD_Generic::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int *indexes, const int count ) {
	min[0] = min[1] = min[2] = idMath::INFINITY; max[0] = max[1] = max[2] = -idMath::INFINITY;
#define OPER(X) const idVec3 &v = src[indexes[(X)]].xyz; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Clamp
============
*/
void idSIMD_Generic::Clamp( float *dst, const float *src, const float min, const float max, const int count ) {
#define OPER(X) dst[(X)] = src[(X)] < min ? min : src[(X)] > max ? max : src[(X)];
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::ClampMin
============
*/
void idSIMD_Generic::ClampMin( float *dst, const float *src, const float min, const int count ) {
#define OPER(X) dst[(X)] = src[(X)] < min ? min : src[(X)];
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::ClampMax
============
*/
void idSIMD_Generic::ClampMax( float *dst, const float *src, const float max, const int count ) {
#define OPER(X) dst[(X)] = src[(X)] > max ? max : src[(X)];
	UNROLL1(OPER)
#undef OPER
}

/*
================
idSIMD_Generic::Memcpy
================
*/
void idSIMD_Generic::Memcpy( void *dst, const void *src, const int count ) {
	memcpy( dst, src, count );
}

/*
================
idSIMD_Generic::Memset
================
*/
void idSIMD_Generic::Memset( void *dst, const int val, const int count ) {
	memset( dst, val, count );
}

/*
================
idSIMD_Generic::MemcpyNT
================
*/
void idSIMD_Generic::MemcpyNT( void *dst, const void *src, const int count ) {
	memcpy( dst, src, count );
}

/*
============
idSIMD_Generic::Zero16
============
*/
void idSIMD_Generic::Zero16( float *dst, const int count ) {
	memset( dst, 0, count * sizeof( float ) );
}

/*
============
idSIMD_Generic::Negate16
============
*/
void idSIMD_Generic::Negate16( float *dst, const int count ) {
	unsigned int *ptr = reinterpret_cast<unsigned int *>(dst);
#define OPER(X) ptr[(X)] ^= ( 1 << 31 )		// IEEE 32 bits float sign bit
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Copy16
============
*/
void idSIMD_Generic::Copy16( float *dst, const float *src, const int count ) {
#define OPER(X) dst[(X)] = src[(X)]
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Add16
============
*/
void idSIMD_Generic::Add16( float *dst, const float *src1, const float *src2, const int count ) {
#define OPER(X) dst[(X)] = src1[(X)] + src2[(X)]
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Sub16
============
*/
void idSIMD_Generic::Sub16( float *dst, const float *src1, const float *src2, const int count ) {
#define OPER(X) dst[(X)] = src1[(X)] - src2[(X)]
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Mul16
============
*/
void idSIMD_Generic::Mul16( float *dst, const float *src1, const float constant, const int count ) {
#define OPER(X) dst[(X)] = src1[(X)] * constant
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::AddAssign16
============
*/
void idSIMD_Generic::AddAssign16( float *dst, const float *src, const int count ) {
#define OPER(X) dst[(X)] += src[(X)]
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::SubAssign16
============
*/
void idSIMD_Generic::SubAssign16( float *dst, const float *src, const int count ) {
#define OPER(X) dst[(X)] -= src[(X)]
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MulAssign16
============
*/
void idSIMD_Generic::MulAssign16( float *dst, const float constant, const int count ) {
#define OPER(X) dst[(X)] *= constant
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MatX_MultiplyVecX
============
*/
void idSIMD_Generic::MatX_MultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
	int i, j, numRows;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumColumns() );
	assert( dst.GetSize() >= mat.GetNumRows() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numRows = mat.GetNumRows();
	switch( mat.GetNumColumns() ) {
		case 1:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] = mPtr[0] * vPtr[0];
				mPtr++;
			}
			break;
		case 2:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] = mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1];
				mPtr += 2;
			}
			break;
		case 3:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] = mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2];
				mPtr += 3;
			}
			break;
		case 4:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] = mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
							mPtr[3] * vPtr[3];
				mPtr += 4;
			}
			break;
		case 5:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] = mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
							mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4];
				mPtr += 5;
			}
			break;
		case 6:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] = mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
							mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4] + mPtr[5] * vPtr[5];
				mPtr += 6;
			}
			break;
		default:
			int numColumns = mat.GetNumColumns();
			for ( i = 0; i < numRows; i++ ) {
				float sum = mPtr[0] * vPtr[0];
				for ( j = 1; j < numColumns; j++ ) {
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
idSIMD_Generic::MatX_MultiplyAddVecX
============
*/
void idSIMD_Generic::MatX_MultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
	int i, j, numRows;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumColumns() );
	assert( dst.GetSize() >= mat.GetNumRows() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numRows = mat.GetNumRows();
	switch( mat.GetNumColumns() ) {
		case 1:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] += mPtr[0] * vPtr[0];
				mPtr++;
			}
			break;
		case 2:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] += mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1];
				mPtr += 2;
			}
			break;
		case 3:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] += mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2];
				mPtr += 3;
			}
			break;
		case 4:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] += mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
							mPtr[3] * vPtr[3];
				mPtr += 4;
			}
			break;
		case 5:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] += mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
							mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4];
				mPtr += 5;
			}
			break;
		case 6:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] += mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
							mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4] + mPtr[5] * vPtr[5];
				mPtr += 6;
			}
			break;
		default:
			int numColumns = mat.GetNumColumns();
			for ( i = 0; i < numRows; i++ ) {
				float sum = mPtr[0] * vPtr[0];
				for ( j = 1; j < numColumns; j++ ) {
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
idSIMD_Generic::MatX_MultiplySubVecX
============
*/
void idSIMD_Generic::MatX_MultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
	int i, j, numRows;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumColumns() );
	assert( dst.GetSize() >= mat.GetNumRows() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numRows = mat.GetNumRows();
	switch( mat.GetNumColumns() ) {
		case 1:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] -= mPtr[0] * vPtr[0];
				mPtr++;
			}
			break;
		case 2:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] -= mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1];
				mPtr += 2;
			}
			break;
		case 3:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] -= mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2];
				mPtr += 3;
			}
			break;
		case 4:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] -= mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
							mPtr[3] * vPtr[3];
				mPtr += 4;
			}
			break;
		case 5:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] -= mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
							mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4];
				mPtr += 5;
			}
			break;
		case 6:
			for ( i = 0; i < numRows; i++ ) {
				dstPtr[i] -= mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
							mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4] + mPtr[5] * vPtr[5];
				mPtr += 6;
			}
			break;
		default:
			int numColumns = mat.GetNumColumns();
			for ( i = 0; i < numRows; i++ ) {
				float sum = mPtr[0] * vPtr[0];
				for ( j = 1; j < numColumns; j++ ) {
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
idSIMD_Generic::MatX_TransposeMultiplyVecX
============
*/
void idSIMD_Generic::MatX_TransposeMultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
	int i, j, numColumns;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumRows() );
	assert( dst.GetSize() >= mat.GetNumColumns() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numColumns = mat.GetNumColumns();
	switch( mat.GetNumRows() ) {
		case 1:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] = *(mPtr) * vPtr[0];
				mPtr++;
			}
			break;
		case 2:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] = *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1];
				mPtr++;
			}
			break;
		case 3:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] = *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2];
				mPtr++;
			}
			break;
		case 4:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] = *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
						*(mPtr+3*numColumns) * vPtr[3];
				mPtr++;
			}
			break;
		case 5:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] = *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
						*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4];
				mPtr++;
			}
			break;
		case 6:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] = *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
						*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4] + *(mPtr+5*numColumns) * vPtr[5];
				mPtr++;
			}
			break;
		default:
			int numRows = mat.GetNumRows();
			for ( i = 0; i < numColumns; i++ ) {
				mPtr = mat.ToFloatPtr() + i;
				float sum = mPtr[0] * vPtr[0];
				for ( j = 1; j < numRows; j++ ) {
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
idSIMD_Generic::MatX_TransposeMultiplyAddVecX
============
*/
void idSIMD_Generic::MatX_TransposeMultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
	int i, j, numColumns;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumRows() );
	assert( dst.GetSize() >= mat.GetNumColumns() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numColumns = mat.GetNumColumns();
	switch( mat.GetNumRows() ) {
		case 1:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] += *(mPtr) * vPtr[0];
				mPtr++;
			}
			break;
		case 2:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] += *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1];
				mPtr++;
			}
			break;
		case 3:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] += *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2];
				mPtr++;
			}
			break;
		case 4:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] += *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
						*(mPtr+3*numColumns) * vPtr[3];
				mPtr++;
			}
			break;
		case 5:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] += *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
						*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4];
				mPtr++;
			}
			break;
		case 6:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] += *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
						*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4] + *(mPtr+5*numColumns) * vPtr[5];
				mPtr++;
			}
			break;
		default:
			int numRows = mat.GetNumRows();
			for ( i = 0; i < numColumns; i++ ) {
				mPtr = mat.ToFloatPtr() + i;
				float sum = mPtr[0] * vPtr[0];
				for ( j = 1; j < numRows; j++ ) {
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
idSIMD_Generic::MatX_TransposeMultiplySubVecX
============
*/
void idSIMD_Generic::MatX_TransposeMultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
	int i, numColumns;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumRows() );
	assert( dst.GetSize() >= mat.GetNumColumns() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numColumns = mat.GetNumColumns();
	switch( mat.GetNumRows() ) {
		case 1:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] -= *(mPtr) * vPtr[0];
				mPtr++;
			}
			break;
		case 2:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] -= *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1];
				mPtr++;
			}
			break;
		case 3:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] -= *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2];
				mPtr++;
			}
			break;
		case 4:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] -= *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
						*(mPtr+3*numColumns) * vPtr[3];
				mPtr++;
			}
			break;
		case 5:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] -= *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
						*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4];
				mPtr++;
			}
			break;
		case 6:
			for ( i = 0; i < numColumns; i++ ) {
				dstPtr[i] -= *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
						*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4] + *(mPtr+5*numColumns) * vPtr[5];
				mPtr++;
			}
			break;
		default:
			int numRows = mat.GetNumRows();
			for ( i = 0; i < numColumns; i++ ) {
				mPtr = mat.ToFloatPtr() + i;
				float sum = mPtr[0] * vPtr[0];
				for ( int j = 1; j < numRows; j++ ) {
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
idSIMD_Generic::MatX_MultiplyMatX

	optimizes the following matrix multiplications:

	NxN * Nx6
	6xN * Nx6
	Nx6 * 6xN
	6x6 * 6xN

	with N in the range [1-6].
============
*/
void idSIMD_Generic::MatX_MultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) {
	int i, j, k, l, n;
	float *dstPtr;
	const float *m1Ptr, *m2Ptr;
	double sum;

	assert( m1.GetNumColumns() == m2.GetNumRows() );

	dstPtr = dst.ToFloatPtr();
	m1Ptr = m1.ToFloatPtr();
	m2Ptr = m2.ToFloatPtr();
	k = m1.GetNumRows();
	l = m2.GetNumColumns();

	switch( m1.GetNumColumns() ) {
		case 1: {
			if ( l == 6 ) {
				for ( i = 0; i < k; i++ ) {		// Nx1 * 1x6
					*dstPtr++ = m1Ptr[i] * m2Ptr[0];
					*dstPtr++ = m1Ptr[i] * m2Ptr[1];
					*dstPtr++ = m1Ptr[i] * m2Ptr[2];
					*dstPtr++ = m1Ptr[i] * m2Ptr[3];
					*dstPtr++ = m1Ptr[i] * m2Ptr[4];
					*dstPtr++ = m1Ptr[i] * m2Ptr[5];
				}
				return;
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		}
		case 2: {
			if ( l == 6 ) {
				for ( i = 0; i < k; i++ ) {		// Nx2 * 2x6
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
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l];
					m2Ptr++;
				}
				m1Ptr += 2;
			}
			break;
		}
		case 3: {
			if ( l == 6 ) {
				for ( i = 0; i < k; i++ ) {		// Nx3 * 3x6
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
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2*l];
					m2Ptr++;
				}
				m1Ptr += 3;
			}
			break;
		}
		case 4: {
			if ( l == 6 ) {
				for ( i = 0; i < k; i++ ) {		// Nx4 * 4x6
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
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2*l] +
									 m1Ptr[3] * m2Ptr[3*l];
					m2Ptr++;
				}
				m1Ptr += 4;
			}
			break;
		}
		case 5: {
			if ( l == 6 ) {
				for ( i = 0; i < k; i++ ) {		// Nx5 * 5x6
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
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2*l] +
									 m1Ptr[3] * m2Ptr[3*l] + m1Ptr[4] * m2Ptr[4*l];
					m2Ptr++;
				}
				m1Ptr += 5;
			}
			break;
		}
		case 6: {
			switch( k ) {
				case 1: {
					if ( l == 1 ) {		// 1x6 * 6x1
						dstPtr[0] = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[1] + m1Ptr[2] * m2Ptr[2] +
									 m1Ptr[3] * m2Ptr[3] + m1Ptr[4] * m2Ptr[4] + m1Ptr[5] * m2Ptr[5];
						return;
					}
					break;
				}
				case 2: {
					if ( l == 2 ) {		// 2x6 * 6x2
						for ( i = 0; i < 2; i++ ) {
							for ( j = 0; j < 2; j++ ) {
								*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 2 + j ]
										+ m1Ptr[1] * m2Ptr[ 1 * 2 + j ]
										+ m1Ptr[2] * m2Ptr[ 2 * 2 + j ]
										+ m1Ptr[3] * m2Ptr[ 3 * 2 + j ]
										+ m1Ptr[4] * m2Ptr[ 4 * 2 + j ]
										+ m1Ptr[5] * m2Ptr[ 5 * 2 + j ];
								dstPtr++;
							}
							m1Ptr += 6;
						}
						return;
					}
					break;
				}
				case 3: {
					if ( l == 3 ) {		// 3x6 * 6x3
						for ( i = 0; i < 3; i++ ) {
							for ( j = 0; j < 3; j++ ) {
								*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 3 + j ]
										+ m1Ptr[1] * m2Ptr[ 1 * 3 + j ]
										+ m1Ptr[2] * m2Ptr[ 2 * 3 + j ]
										+ m1Ptr[3] * m2Ptr[ 3 * 3 + j ]
										+ m1Ptr[4] * m2Ptr[ 4 * 3 + j ]
										+ m1Ptr[5] * m2Ptr[ 5 * 3 + j ];
								dstPtr++;
							}
							m1Ptr += 6;
						}
						return;
					}
					break;
				}
				case 4: {
					if ( l == 4 ) {		// 4x6 * 6x4
						for ( i = 0; i < 4; i++ ) {
							for ( j = 0; j < 4; j++ ) {
								*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 4 + j ]
										+ m1Ptr[1] * m2Ptr[ 1 * 4 + j ]
										+ m1Ptr[2] * m2Ptr[ 2 * 4 + j ]
										+ m1Ptr[3] * m2Ptr[ 3 * 4 + j ]
										+ m1Ptr[4] * m2Ptr[ 4 * 4 + j ]
										+ m1Ptr[5] * m2Ptr[ 5 * 4 + j ];
								dstPtr++;
							}
							m1Ptr += 6;
						}
						return;
					}
				}
				case 5: {
					if ( l == 5 ) {		// 5x6 * 6x5
						for ( i = 0; i < 5; i++ ) {
							for ( j = 0; j < 5; j++ ) {
								*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 5 + j ]
										+ m1Ptr[1] * m2Ptr[ 1 * 5 + j ]
										+ m1Ptr[2] * m2Ptr[ 2 * 5 + j ]
										+ m1Ptr[3] * m2Ptr[ 3 * 5 + j ]
										+ m1Ptr[4] * m2Ptr[ 4 * 5 + j ]
										+ m1Ptr[5] * m2Ptr[ 5 * 5 + j ];
								dstPtr++;
							}
							m1Ptr += 6;
						}
						return;
					}
				}
				case 6: {
					switch( l ) {
						case 1: {		// 6x6 * 6x1
							for ( i = 0; i < 6; i++ ) {
								*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 1 ]
										+ m1Ptr[1] * m2Ptr[ 1 * 1 ]
										+ m1Ptr[2] * m2Ptr[ 2 * 1 ]
										+ m1Ptr[3] * m2Ptr[ 3 * 1 ]
										+ m1Ptr[4] * m2Ptr[ 4 * 1 ]
										+ m1Ptr[5] * m2Ptr[ 5 * 1 ];
								dstPtr++;
								m1Ptr += 6;
							}
							return;
						}
						case 2: {		// 6x6 * 6x2
							for ( i = 0; i < 6; i++ ) {
								for ( j = 0; j < 2; j++ ) {
									*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 2 + j ]
											+ m1Ptr[1] * m2Ptr[ 1 * 2 + j ]
											+ m1Ptr[2] * m2Ptr[ 2 * 2 + j ]
											+ m1Ptr[3] * m2Ptr[ 3 * 2 + j ]
											+ m1Ptr[4] * m2Ptr[ 4 * 2 + j ]
											+ m1Ptr[5] * m2Ptr[ 5 * 2 + j ];
									dstPtr++;
								}
								m1Ptr += 6;
							}
							return;
						}
						case 3: {		// 6x6 * 6x3
							for ( i = 0; i < 6; i++ ) {
								for ( j = 0; j < 3; j++ ) {
									*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 3 + j ]
											+ m1Ptr[1] * m2Ptr[ 1 * 3 + j ]
											+ m1Ptr[2] * m2Ptr[ 2 * 3 + j ]
											+ m1Ptr[3] * m2Ptr[ 3 * 3 + j ]
											+ m1Ptr[4] * m2Ptr[ 4 * 3 + j ]
											+ m1Ptr[5] * m2Ptr[ 5 * 3 + j ];
									dstPtr++;
								}
								m1Ptr += 6;
							}
							return;
						}
						case 4: {		// 6x6 * 6x4
							for ( i = 0; i < 6; i++ ) {
								for ( j = 0; j < 4; j++ ) {
									*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 4 + j ]
											+ m1Ptr[1] * m2Ptr[ 1 * 4 + j ]
											+ m1Ptr[2] * m2Ptr[ 2 * 4 + j ]
											+ m1Ptr[3] * m2Ptr[ 3 * 4 + j ]
											+ m1Ptr[4] * m2Ptr[ 4 * 4 + j ]
											+ m1Ptr[5] * m2Ptr[ 5 * 4 + j ];
									dstPtr++;
								}
								m1Ptr += 6;
							}
							return;
						}
						case 5: {		// 6x6 * 6x5
							for ( i = 0; i < 6; i++ ) {
								for ( j = 0; j < 5; j++ ) {
									*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 5 + j ]
											+ m1Ptr[1] * m2Ptr[ 1 * 5 + j ]
											+ m1Ptr[2] * m2Ptr[ 2 * 5 + j ]
											+ m1Ptr[3] * m2Ptr[ 3 * 5 + j ]
											+ m1Ptr[4] * m2Ptr[ 4 * 5 + j ]
											+ m1Ptr[5] * m2Ptr[ 5 * 5 + j ];
									dstPtr++;
								}
								m1Ptr += 6;
							}
							return;
						}
						case 6: {		// 6x6 * 6x6
							for ( i = 0; i < 6; i++ ) {
								for ( j = 0; j < 6; j++ ) {
									*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 6 + j ]
											+ m1Ptr[1] * m2Ptr[ 1 * 6 + j ]
											+ m1Ptr[2] * m2Ptr[ 2 * 6 + j ]
											+ m1Ptr[3] * m2Ptr[ 3 * 6 + j ]
											+ m1Ptr[4] * m2Ptr[ 4 * 6 + j ]
											+ m1Ptr[5] * m2Ptr[ 5 * 6 + j ];
									dstPtr++;
								}
								m1Ptr += 6;
							}
							return;
						}
					}
				}
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2*l] +
									 m1Ptr[3] * m2Ptr[3*l] + m1Ptr[4] * m2Ptr[4*l] + m1Ptr[5] * m2Ptr[5*l];
					m2Ptr++;
				}
				m1Ptr += 6;
			}
			break;
		}
		default: {
			for ( i = 0; i < k; i++ ) {
				for ( j = 0; j < l; j++ ) {
					m2Ptr = m2.ToFloatPtr() + j;
					sum = m1Ptr[0] * m2Ptr[0];
					for ( n = 1; n < m1.GetNumColumns(); n++ ) {
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
idSIMD_Generic::MatX_TransposeMultiplyMatX

	optimizes the following tranpose matrix multiplications:

	Nx6 * NxN
	6xN * 6x6

	with N in the range [1-6].
============
*/
void idSIMD_Generic::MatX_TransposeMultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) {
	int i, j, k, l, n;
	float *dstPtr;
	const float *m1Ptr, *m2Ptr;
	double sum;

	assert( m1.GetNumRows() == m2.GetNumRows() );

	m1Ptr = m1.ToFloatPtr();
	m2Ptr = m2.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	k = m1.GetNumColumns();
	l = m2.GetNumColumns();

	switch( m1.GetNumRows() ) {
		case 1:
			if ( k == 6 && l == 1 ) {			// 1x6 * 1x1
				for ( i = 0; i < 6; i++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0];
					m1Ptr++;
				}
				return;
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		case 2:
			if ( k == 6 && l == 2 ) {			// 2x6 * 2x2
				for ( i = 0; i < 6; i++ ) {
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*2+0] + m1Ptr[1*6] * m2Ptr[1*2+0];
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*2+1] + m1Ptr[1*6] * m2Ptr[1*2+1];
					m1Ptr++;
				}
				return;
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		case 3:
			if ( k == 6 && l == 3 ) {			// 3x6 * 3x3
				for ( i = 0; i < 6; i++ ) {
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*3+0] + m1Ptr[1*6] * m2Ptr[1*3+0] + m1Ptr[2*6] * m2Ptr[2*3+0];
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*3+1] + m1Ptr[1*6] * m2Ptr[1*3+1] + m1Ptr[2*6] * m2Ptr[2*3+1];
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*3+2] + m1Ptr[1*6] * m2Ptr[1*3+2] + m1Ptr[2*6] * m2Ptr[2*3+2];
					m1Ptr++;
				}
				return;
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2*k] * m2Ptr[2*l];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		case 4:
			if ( k == 6 && l == 4 ) {			// 4x6 * 4x4
				for ( i = 0; i < 6; i++ ) {
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*4+0] + m1Ptr[1*6] * m2Ptr[1*4+0] + m1Ptr[2*6] * m2Ptr[2*4+0] + m1Ptr[3*6] * m2Ptr[3*4+0];
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*4+1] + m1Ptr[1*6] * m2Ptr[1*4+1] + m1Ptr[2*6] * m2Ptr[2*4+1] + m1Ptr[3*6] * m2Ptr[3*4+1];
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*4+2] + m1Ptr[1*6] * m2Ptr[1*4+2] + m1Ptr[2*6] * m2Ptr[2*4+2] + m1Ptr[3*6] * m2Ptr[3*4+2];
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*4+3] + m1Ptr[1*6] * m2Ptr[1*4+3] + m1Ptr[2*6] * m2Ptr[2*4+3] + m1Ptr[3*6] * m2Ptr[3*4+3];
					m1Ptr++;
				}
				return;
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2*k] * m2Ptr[2*l] +
									m1Ptr[3*k] * m2Ptr[3*l];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		case 5:
			if ( k == 6 && l == 5 ) {			// 5x6 * 5x5
				for ( i = 0; i < 6; i++ ) {
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*5+0] + m1Ptr[1*6] * m2Ptr[1*5+0] + m1Ptr[2*6] * m2Ptr[2*5+0] + m1Ptr[3*6] * m2Ptr[3*5+0] + m1Ptr[4*6] * m2Ptr[4*5+0];
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*5+1] + m1Ptr[1*6] * m2Ptr[1*5+1] + m1Ptr[2*6] * m2Ptr[2*5+1] + m1Ptr[3*6] * m2Ptr[3*5+1] + m1Ptr[4*6] * m2Ptr[4*5+1];
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*5+2] + m1Ptr[1*6] * m2Ptr[1*5+2] + m1Ptr[2*6] * m2Ptr[2*5+2] + m1Ptr[3*6] * m2Ptr[3*5+2] + m1Ptr[4*6] * m2Ptr[4*5+2];
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*5+3] + m1Ptr[1*6] * m2Ptr[1*5+3] + m1Ptr[2*6] * m2Ptr[2*5+3] + m1Ptr[3*6] * m2Ptr[3*5+3] + m1Ptr[4*6] * m2Ptr[4*5+3];
					*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*5+4] + m1Ptr[1*6] * m2Ptr[1*5+4] + m1Ptr[2*6] * m2Ptr[2*5+4] + m1Ptr[3*6] * m2Ptr[3*5+4] + m1Ptr[4*6] * m2Ptr[4*5+4];
					m1Ptr++;
				}
				return;
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2*k] * m2Ptr[2*l] +
									m1Ptr[3*k] * m2Ptr[3*l] + m1Ptr[4*k] * m2Ptr[4*l];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		case 6:
			if ( l == 6 ) {
				switch( k ) {
					case 1:						// 6x1 * 6x6
						m2Ptr = m2.ToFloatPtr();
						for ( j = 0; j < 6; j++ ) {
							*dstPtr++ = m1Ptr[0*1] * m2Ptr[0*6] +
										m1Ptr[1*1] * m2Ptr[1*6] +
										m1Ptr[2*1] * m2Ptr[2*6] +
										m1Ptr[3*1] * m2Ptr[3*6] +
										m1Ptr[4*1] * m2Ptr[4*6] +
										m1Ptr[5*1] * m2Ptr[5*6];
							m2Ptr++;
						}
						return;
					case 2:						// 6x2 * 6x6
						for ( i = 0; i < 2; i++ ) {
							m2Ptr = m2.ToFloatPtr();
							for ( j = 0; j < 6; j++ ) {
								*dstPtr++ = m1Ptr[0*2] * m2Ptr[0*6] +
											m1Ptr[1*2] * m2Ptr[1*6] +
											m1Ptr[2*2] * m2Ptr[2*6] +
											m1Ptr[3*2] * m2Ptr[3*6] +
											m1Ptr[4*2] * m2Ptr[4*6] +
											m1Ptr[5*2] * m2Ptr[5*6];
								m2Ptr++;
							}
							m1Ptr++;
						}
						return;
					case 3:						// 6x3 * 6x6
						for ( i = 0; i < 3; i++ ) {
							m2Ptr = m2.ToFloatPtr();
							for ( j = 0; j < 6; j++ ) {
								*dstPtr++ = m1Ptr[0*3] * m2Ptr[0*6] +
											m1Ptr[1*3] * m2Ptr[1*6] +
											m1Ptr[2*3] * m2Ptr[2*6] +
											m1Ptr[3*3] * m2Ptr[3*6] +
											m1Ptr[4*3] * m2Ptr[4*6] +
											m1Ptr[5*3] * m2Ptr[5*6];
								m2Ptr++;
							}
							m1Ptr++;
						}
						return;
					case 4:						// 6x4 * 6x6
						for ( i = 0; i < 4; i++ ) {
							m2Ptr = m2.ToFloatPtr();
							for ( j = 0; j < 6; j++ ) {
								*dstPtr++ = m1Ptr[0*4] * m2Ptr[0*6] +
											m1Ptr[1*4] * m2Ptr[1*6] +
											m1Ptr[2*4] * m2Ptr[2*6] +
											m1Ptr[3*4] * m2Ptr[3*6] +
											m1Ptr[4*4] * m2Ptr[4*6] +
											m1Ptr[5*4] * m2Ptr[5*6];
								m2Ptr++;
							}
							m1Ptr++;
						}
						return;
					case 5:						// 6x5 * 6x6
						for ( i = 0; i < 5; i++ ) {
							m2Ptr = m2.ToFloatPtr();
							for ( j = 0; j < 6; j++ ) {
								*dstPtr++ = m1Ptr[0*5] * m2Ptr[0*6] +
											m1Ptr[1*5] * m2Ptr[1*6] +
											m1Ptr[2*5] * m2Ptr[2*6] +
											m1Ptr[3*5] * m2Ptr[3*6] +
											m1Ptr[4*5] * m2Ptr[4*6] +
											m1Ptr[5*5] * m2Ptr[5*6];
								m2Ptr++;
							}
							m1Ptr++;
						}
						return;
					case 6:						// 6x6 * 6x6
						for ( i = 0; i < 6; i++ ) {
							m2Ptr = m2.ToFloatPtr();
							for ( j = 0; j < 6; j++ ) {
								*dstPtr++ = m1Ptr[0*6] * m2Ptr[0*6] +
											m1Ptr[1*6] * m2Ptr[1*6] +
											m1Ptr[2*6] * m2Ptr[2*6] +
											m1Ptr[3*6] * m2Ptr[3*6] +
											m1Ptr[4*6] * m2Ptr[4*6] +
											m1Ptr[5*6] * m2Ptr[5*6];
								m2Ptr++;
							}
							m1Ptr++;
						}
						return;
				}
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2*k] * m2Ptr[2*l] +
									m1Ptr[3*k] * m2Ptr[3*l] + m1Ptr[4*k] * m2Ptr[4*l] + m1Ptr[5*k] * m2Ptr[5*l];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		default:
			for ( i = 0; i < k; i++ ) {
				for ( j = 0; j < l; j++ ) {
					m1Ptr = m1.ToFloatPtr() + i;
					m2Ptr = m2.ToFloatPtr() + j;
					sum = m1Ptr[0] * m2Ptr[0];
					for ( n = 1; n < m1.GetNumRows(); n++ ) {
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
idSIMD_Generic::MatX_LowerTriangularSolve

  solves x in Lx = b for the n * n sub-matrix of L
  if skip > 0 the first skip elements of x are assumed to be valid already
  L has to be a lower triangular matrix with (implicit) ones on the diagonal
  x == b is allowed
============
*/
void idSIMD_Generic::MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip ) {
#if 1

	int nc;
	const float *lptr;

	if ( skip >= n ) {
		return;
	}

	lptr = L.ToFloatPtr();
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

	int i, j;
	register double s0, s1, s2, s3;

	for ( i = skip; i < n; i++ ) {
		s0 = lptr[0] * x[0];
		s1 = lptr[1] * x[1];
		s2 = lptr[2] * x[2];
		s3 = lptr[3] * x[3];
		for ( j = 4; j < i-7; j += 8 ) {
			s0 += lptr[j+0] * x[j+0];
			s1 += lptr[j+1] * x[j+1];
			s2 += lptr[j+2] * x[j+2];
			s3 += lptr[j+3] * x[j+3];
			s0 += lptr[j+4] * x[j+4];
			s1 += lptr[j+5] * x[j+5];
			s2 += lptr[j+6] * x[j+6];
			s3 += lptr[j+7] * x[j+7];
		}
		switch( i - j ) {
			NODEFAULT;
			case 7: s0 += lptr[j+6] * x[j+6];
			case 6: s1 += lptr[j+5] * x[j+5];
			case 5: s2 += lptr[j+4] * x[j+4];
			case 4: s3 += lptr[j+3] * x[j+3];
			case 3: s0 += lptr[j+2] * x[j+2];
			case 2: s1 += lptr[j+1] * x[j+1];
			case 1: s2 += lptr[j+0] * x[j+0];
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

	for ( i = skip; i < n; i++ ) {
		sum = b[i];
		lptr = L[i];
		for ( j = 0; j < i; j++ ) {
			sum -= lptr[j] * x[j];
		}
		x[i] = sum;
	}

#endif
}

/*
============
idSIMD_Generic::MatX_LowerTriangularSolveTranspose

  solves x in L'x = b for the n * n sub-matrix of L
  L has to be a lower triangular matrix with (implicit) ones on the diagonal
  x == b is allowed
============
*/
void idSIMD_Generic::MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n ) {
#if 1

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

	int i, j;
	register double s0, s1, s2, s3;
	float *xptr;

	lptr = L.ToFloatPtr() + n * nc + n - 4;
	xptr = x + n;

	// process 4 rows at a time
	for ( i = n; i >= 4; i -= 4 ) {
		s0 = b[i-4];
		s1 = b[i-3];
		s2 = b[i-2];
		s3 = b[i-1];
		// process 4x4 blocks
		for ( j = 0; j < n-i; j += 4 ) {
			s0 -= lptr[(j+0)*nc+0] * xptr[j+0];
			s1 -= lptr[(j+0)*nc+1] * xptr[j+0];
			s2 -= lptr[(j+0)*nc+2] * xptr[j+0];
			s3 -= lptr[(j+0)*nc+3] * xptr[j+0];
			s0 -= lptr[(j+1)*nc+0] * xptr[j+1];
			s1 -= lptr[(j+1)*nc+1] * xptr[j+1];
			s2 -= lptr[(j+1)*nc+2] * xptr[j+1];
			s3 -= lptr[(j+1)*nc+3] * xptr[j+1];
			s0 -= lptr[(j+2)*nc+0] * xptr[j+2];
			s1 -= lptr[(j+2)*nc+1] * xptr[j+2];
			s2 -= lptr[(j+2)*nc+2] * xptr[j+2];
			s3 -= lptr[(j+2)*nc+3] * xptr[j+2];
			s0 -= lptr[(j+3)*nc+0] * xptr[j+3];
			s1 -= lptr[(j+3)*nc+1] * xptr[j+3];
			s2 -= lptr[(j+3)*nc+2] * xptr[j+3];
			s3 -= lptr[(j+3)*nc+3] * xptr[j+3];
		}
		// process left over of the 4 rows
		s0 -= lptr[0-1*nc] * s3;
		s1 -= lptr[1-1*nc] * s3;
		s2 -= lptr[2-1*nc] * s3;
		s0 -= lptr[0-2*nc] * s2;
		s1 -= lptr[1-2*nc] * s2;
		s0 -= lptr[0-3*nc] * s1;
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
	for ( i--; i >= 0; i-- ) {
		s0 = b[i];
		lptr = L[0] + i;
		for ( j = i + 1; j < n; j++ ) {
			s0 -= lptr[j*nc] * x[j];
		}
		x[i] = s0;
	}

#else

	int i, j, nc;
	const float *ptr;
	double sum;

	nc = L.GetNumColumns();
	for ( i = n - 1; i >= 0; i-- ) {
		sum = b[i];
		ptr = L[0] + i;
		for ( j = i + 1; j < n; j++ ) {
			sum -= ptr[j*nc] * x[j];
		}
		x[i] = sum;
	}

#endif
}

/*
============
idSIMD_Generic::MatX_LDLTFactor

  in-place factorization LDL' of the n * n sub-matrix of mat
  the reciprocal of the diagonal elements are stored in invDiag
============
*/
bool idSIMD_Generic::MatX_LDLTFactor( idMatX &mat, idVecX &invDiag, const int n ) {
#if 1

	int i, j, k, nc;
	float *v, *diag, *mptr;
	double s0, s1, s2, s3, sum, d;

	v = (float *) _alloca16( n * sizeof( float ) );
	diag = (float *) _alloca16( n * sizeof( float ) );

	nc = mat.GetNumColumns();

	if ( n <= 0 ) {
		return true;
	}

	mptr = mat[0];

	sum = mptr[0];

	if ( sum == 0.0f ) {
		return false;
	}

	diag[0] = sum;
	invDiag[0] = d = 1.0f / sum;

	if ( n <= 1 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 1; j < n; j++ ) {
		mptr[j*nc+0] = ( mptr[j*nc+0] ) * d;
	}

	mptr = mat[1];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	sum = mptr[1] - s0;

	if ( sum == 0.0f ) {
		return false;
	}

	mat[1][1] = sum;
	diag[1] = sum;
	invDiag[1] = d = 1.0f / sum;

	if ( n <= 2 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 2; j < n; j++ ) {
		mptr[j*nc+1] = ( mptr[j*nc+1] - v[0] * mptr[j*nc+0] ) * d;
	}

	mptr = mat[2];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	v[1] = diag[1] * mptr[1]; s1 = v[1] * mptr[1];
	sum = mptr[2] - s0 - s1;

	if ( sum == 0.0f ) {
		return false;
	}

	mat[2][2] = sum;
	diag[2] = sum;
	invDiag[2] = d = 1.0f / sum;

	if ( n <= 3 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 3; j < n; j++ ) {
		mptr[j*nc+2] = ( mptr[j*nc+2] - v[0] * mptr[j*nc+0] - v[1] * mptr[j*nc+1] ) * d;
	}

	mptr = mat[3];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	v[1] = diag[1] * mptr[1]; s1 = v[1] * mptr[1];
	v[2] = diag[2] * mptr[2]; s2 = v[2] * mptr[2];
	sum = mptr[3] - s0 - s1 - s2;

	if ( sum == 0.0f ) {
		return false;
	}

	mat[3][3] = sum;
	diag[3] = sum;
	invDiag[3] = d = 1.0f / sum;

	if ( n <= 4 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 4; j < n; j++ ) {
		mptr[j*nc+3] = ( mptr[j*nc+3] - v[0] * mptr[j*nc+0] - v[1] * mptr[j*nc+1] - v[2] * mptr[j*nc+2] ) * d;
	}

	for ( i = 4; i < n; i++ ) {

		mptr = mat[i];

		v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
		v[1] = diag[1] * mptr[1]; s1 = v[1] * mptr[1];
		v[2] = diag[2] * mptr[2]; s2 = v[2] * mptr[2];
		v[3] = diag[3] * mptr[3]; s3 = v[3] * mptr[3];
		for ( k = 4; k < i-3; k += 4 ) {
			v[k+0] = diag[k+0] * mptr[k+0]; s0 += v[k+0] * mptr[k+0];
			v[k+1] = diag[k+1] * mptr[k+1]; s1 += v[k+1] * mptr[k+1];
			v[k+2] = diag[k+2] * mptr[k+2]; s2 += v[k+2] * mptr[k+2];
			v[k+3] = diag[k+3] * mptr[k+3]; s3 += v[k+3] * mptr[k+3];
		}
		switch( i - k ) {
			NODEFAULT;
			case 3: v[k+2] = diag[k+2] * mptr[k+2]; s0 += v[k+2] * mptr[k+2];
			case 2: v[k+1] = diag[k+1] * mptr[k+1]; s1 += v[k+1] * mptr[k+1];
			case 1: v[k+0] = diag[k+0] * mptr[k+0]; s2 += v[k+0] * mptr[k+0];
			case 0: break;
		}
		sum = s3;
		sum += s2;
		sum += s1;
		sum += s0;
		sum = mptr[i] - sum;

		if ( sum == 0.0f ) {
			return false;
		}

		mat[i][i] = sum;
		diag[i] = sum;
		invDiag[i] = d = 1.0f / sum;

		if ( i + 1 >= n ) {
			return true;
		}

		mptr = mat[i+1];
		for ( j = i+1; j < n; j++ ) {
			s0 = mptr[0] * v[0];
			s1 = mptr[1] * v[1];
			s2 = mptr[2] * v[2];
			s3 = mptr[3] * v[3];
			for ( k = 4; k < i-7; k += 8 ) {
				s0 += mptr[k+0] * v[k+0];
				s1 += mptr[k+1] * v[k+1];
				s2 += mptr[k+2] * v[k+2];
				s3 += mptr[k+3] * v[k+3];
				s0 += mptr[k+4] * v[k+4];
				s1 += mptr[k+5] * v[k+5];
				s2 += mptr[k+6] * v[k+6];
				s3 += mptr[k+7] * v[k+7];
			}
			switch( i - k ) {
				NODEFAULT;
				case 7: s0 += mptr[k+6] * v[k+6];
				case 6: s1 += mptr[k+5] * v[k+5];
				case 5: s2 += mptr[k+4] * v[k+4];
				case 4: s3 += mptr[k+3] * v[k+3];
				case 3: s0 += mptr[k+2] * v[k+2];
				case 2: s1 += mptr[k+1] * v[k+1];
				case 1: s2 += mptr[k+0] * v[k+0];
				case 0: break;
			}
			sum = s3;
			sum += s2;
			sum += s1;
			sum += s0;
			mptr[i] = ( mptr[i] - sum ) * d;
			mptr += nc;
		}
	}

	return true;

#else

	int i, j, k, nc;
	float *v, *ptr, *diagPtr;
	double d, sum;

	v = (float *) _alloca16( n * sizeof( float ) );
	nc = mat.GetNumColumns();

	for ( i = 0; i < n; i++ ) {

		ptr = mat[i];
		diagPtr = mat[0];
		sum = ptr[i];
		for ( j = 0; j < i; j++ ) {
			d = ptr[j];
		    v[j] = diagPtr[0] * d;
		    sum -= v[j] * d;
			diagPtr += nc + 1;
		}

		if ( sum == 0.0f ) {
			return false;
		}

		diagPtr[0] = sum;
		invDiag[i] = d = 1.0f / sum;

		if ( i + 1 >= n ) {
			continue;
		}

		ptr = mat[i+1];
		for ( j = i + 1; j < n; j++ ) {
			sum = ptr[i];
			for ( k = 0; k < i; k++ ) {
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
idSIMD_Generic::BlendJoints
============
*/
void idSIMD_Generic::BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) {
	int i;

	for ( i = 0; i < numJoints; i++ ) {
		int j = index[i];
		joints[j].q.Slerp( joints[j].q, blendJoints[j].q, lerp );
		joints[j].t.Lerp( joints[j].t, blendJoints[j].t, lerp );
	}
}

/*
============
idSIMD_Generic::ConvertJointQuatsToJointMats
============
*/
void idSIMD_Generic::ConvertJointQuatsToJointMats( idJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints ) {
	int i;

	for ( i = 0; i < numJoints; i++ ) {
		jointMats[i].SetRotation( jointQuats[i].q.ToMat3() );
		jointMats[i].SetTranslation( jointQuats[i].t );
	}
}

/*
============
idSIMD_Generic::ConvertJointMatsToJointQuats
============
*/
void idSIMD_Generic::ConvertJointMatsToJointQuats( idJointQuat *jointQuats, const idJointMat *jointMats, const int numJoints ) {
	int i;

	for ( i = 0; i < numJoints; i++ ) {
		jointQuats[i] = jointMats[i].ToJointQuat();
	}
}

/*
============
idSIMD_Generic::TransformJoints
============
*/
void idSIMD_Generic::TransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) {
	int i;

	for( i = firstJoint; i <= lastJoint; i++ ) {
		assert( parents[i] < i );
		jointMats[i] *= jointMats[parents[i]];
	}
}

/*
============
idSIMD_Generic::UntransformJoints
============
*/
void idSIMD_Generic::UntransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) {
	int i;

	for( i = lastJoint; i >= firstJoint; i-- ) {
		assert( parents[i] < i );
		jointMats[i] /= jointMats[parents[i]];
	}
}

/*
============
idSIMD_Generic::TransformVerts
============
*/
void idSIMD_Generic::TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *weights, const int *index, int numWeights ) {
	int i, j;
	const byte *jointsPtr = (byte *)joints;

	for( j = i = 0; i < numVerts; i++ ) {
		idVec3 v;

		v = ( *(idJointMat *) ( jointsPtr + index[j*2+0] ) ) * weights[j];
		while( index[j*2+1] == 0 ) {
			j++;
			v += ( *(idJointMat *) ( jointsPtr + index[j*2+0] ) ) * weights[j];
		}
		j++;

		verts[i].xyz = v;
	}
}

/*
============
idSIMD_Generic::ComputeBoundsFromJointBounds
============
*/
void idSIMD_Generic::ComputeBoundsFromJointBounds( idBounds &totalBounds, int numJoints, const idJointMat *joints, const idBounds *jointBounds ) {
	totalBounds.Clear();
	for ( int i = 0; i < numJoints; i++ ) {
		if ( jointBounds[i].IsCleared() )
			continue;
		idBounds jointWorldBounds;
		jointWorldBounds.FromTransformedBounds( jointBounds[i], joints[i].ToVec3(), joints[i].ToMat3() );
		totalBounds.AddBounds( jointWorldBounds );
	}
}

/*
============
idSIMD_Generic::TracePointCull
============
*/
void idSIMD_Generic::TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts ) {
	int i;
	byte tOr;

	tOr = 0;

	for ( i = 0; i < numVerts; i++ ) {
		byte bits, b0, b1, b2, b3;
		float d0, d1, d2, d3;
		const idVec3 &v = verts[i].xyz;

		d0 = planes[0].Distance( v );
		d1 = planes[1].Distance( v );
		d2 = planes[2].Distance( v );
		d3 = planes[3].Distance( v );

		b0 = ( (d0 > -radius) << 0 ) + ( (d0 < radius) << 4 );
		b1 = ( (d1 > -radius) << 1 ) + ( (d1 < radius) << 5 );
		b2 = ( (d2 > -radius) << 2 ) + ( (d2 < radius) << 6 );
		b3 = ( (d3 > -radius) << 3 ) + ( (d3 < radius) << 7 );
		bits = (b0 + b1) + (b2 + b3);

		tOr |= bits;
		cullBits[i] = bits;
	}

	totalOr = tOr;
}

/*
============
idSIMD_Generic::DecalPointCull
============
*/
void idSIMD_Generic::DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts ) {
	int i;

	for ( i = 0; i < numVerts; i++ ) {
		byte bits;
		float d0, d1, d2, d3, d4, d5;
		const idVec3 &v = verts[i].xyz;

		d0 = planes[0].Distance( v );
		d1 = planes[1].Distance( v );
		d2 = planes[2].Distance( v );
		d3 = planes[3].Distance( v );
		d4 = planes[4].Distance( v );
		d5 = planes[5].Distance( v );

		bits  = FLOATSIGNBITSET( d0 ) << 0;
		bits |= FLOATSIGNBITSET( d1 ) << 1;
		bits |= FLOATSIGNBITSET( d2 ) << 2;
		bits |= FLOATSIGNBITSET( d3 ) << 3;
		bits |= FLOATSIGNBITSET( d4 ) << 4;
		bits |= FLOATSIGNBITSET( d5 ) << 5;

		cullBits[i] = bits ^ 0x3F;		// flip lower 6 bits
	}
}

/*
============
idSIMD_Generic::OverlayPointCull
============
*/
void idSIMD_Generic::OverlayPointCull( byte *cullBits, idVec2 *texCoords, const idPlane *planes, const idDrawVert *verts, const int numVerts ) {
	int i;

	for ( i = 0; i < numVerts; i++ ) {
		byte bits;
		float d0, d1;
		const idVec3 &v = verts[i].xyz;

		texCoords[i][0] = d0 = planes[0].Distance( v );
		texCoords[i][1] = d1 = planes[1].Distance( v );

		bits  = FLOATSIGNBITSET( d0 ) << 0;
		d0 = 1.0f - d0;
		bits |= FLOATSIGNBITSET( d1 ) << 1;
		d1 = 1.0f - d1;
		bits |= FLOATSIGNBITSET( d0 ) << 2;
		bits |= FLOATSIGNBITSET( d1 ) << 3;

		cullBits[i] = bits;
	}
}

void idSIMD_Generic::CalcTriFacing( const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes, const idVec3 &lightOrigin, byte *facing ) {
	for ( int i = 0, face = 0; i < numIndexes; i += 3, face++ ) {
		const idDrawVert& v0 = verts[indexes[i + 0]];
		const idDrawVert& v1 = verts[indexes[i + 1]];
		const idDrawVert& v2 = verts[indexes[i + 2]];

		const idPlane plane( v0.xyz, v1.xyz, v2.xyz );
		const float d = plane.Distance( lightOrigin );

		facing[face] = ( d >= 0.0f );
	}
}

/*
============
idSIMD_Generic::DeriveTriPlanes

	Derives a plane equation for each triangle.
============
*/
void idSIMD_Generic::DeriveTriPlanes( idPlane *planes, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) {
	int i;

	for ( i = 0; i < numIndexes; i += 3 ) {
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

		f = idMath::RSqrt( n.x * n.x + n.y * n.y + n.z * n.z );

		n.x *= f;
		n.y *= f;
		n.z *= f;

		planes->SetNormal( n );
		planes->FitThroughPoint( a->xyz );
		planes++;
	}
}

/*
============
idSIMD_Generic::DeriveTangents

	Derives the normal and orthogonal tangent vectors for the triangle vertices.
	For each vertex the normal and tangent vectors are derived from all triangles
	using the vertex which results in smooth tangents across the mesh.
	In the process the triangle planes are calculated as well.
============
*/
void idSIMD_Generic::DeriveTangents( idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) {
	int i;

	bool *used = (bool *)_alloca16( numVerts * sizeof( used[0] ) );
	memset( used, 0, numVerts * sizeof( used[0] ) );

	idPlane *planesPtr = planes;
	for ( i = 0; i < numIndexes; i += 3 ) {
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

		f = idMath::RSqrt( n.x * n.x + n.y * n.y + n.z * n.z );

		n.x *= f;
		n.y *= f;
		n.z *= f;

		planesPtr->SetNormal( n );
		planesPtr->FitThroughPoint( a->xyz );
		planesPtr++;

		// area sign bit
		area = d0[3] * d1[4] - d0[4] * d1[3];
		signBit = ( *(unsigned int *)&area ) & ( 1 << 31 );

		// first tangent
		t0[0] = d0[0] * d1[4] - d0[4] * d1[0];
		t0[1] = d0[1] * d1[4] - d0[4] * d1[1];
		t0[2] = d0[2] * d1[4] - d0[4] * d1[2];

		f = idMath::RSqrt( t0.x * t0.x + t0.y * t0.y + t0.z * t0.z );
		*(unsigned int *)&f ^= signBit;

		t0.x *= f;
		t0.y *= f;
		t0.z *= f;

		// second tangent
		t1[0] = d0[3] * d1[0] - d0[0] * d1[3];
		t1[1] = d0[3] * d1[1] - d0[1] * d1[3];
		t1[2] = d0[3] * d1[2] - d0[2] * d1[3];

		f = idMath::RSqrt( t1.x * t1.x + t1.y * t1.y + t1.z * t1.z );
		*(unsigned int *)&f ^= signBit;

		t1.x *= f;
		t1.y *= f;
		t1.z *= f;

		if ( used[v0] ) {
			a->normal += n;
			a->tangents[0] += t0;
			a->tangents[1] += t1;
		} else {
			a->normal = n;
			a->tangents[0] = t0;
			a->tangents[1] = t1;
			used[v0] = true;
		}

		if ( used[v1] ) {
			b->normal += n;
			b->tangents[0] += t0;
			b->tangents[1] += t1;
		} else {
			b->normal = n;
			b->tangents[0] = t0;
			b->tangents[1] = t1;
			used[v1] = true;
		}

		if ( used[v2] ) {
			c->normal += n;
			c->tangents[0] += t0;
			c->tangents[1] += t1;
		} else {
			c->normal = n;
			c->tangents[0] = t0;
			c->tangents[1] = t1;
			used[v2] = true;
		}
	}
}

/*
============
idSIMD_Generic::DeriveUnsmoothedTangents

	Derives the normal and orthogonal tangent vectors for the triangle vertices.
	For each vertex the normal and tangent vectors are derived from a single dominant triangle.
============
*/
#define DERIVE_UNSMOOTHED_BITANGENT

void idSIMD_Generic::DeriveUnsmoothedTangents( idDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts ) {
	int i;

	for ( i = 0; i < numVerts; i++ ) {
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

		n0 = s2 * ( d6 * d2 - d7 * d1 );
		n1 = s2 * ( d7 * d0 - d5 * d2 );
		n2 = s2 * ( d5 * d1 - d6 * d0 );

		t0 = s0 * ( d0 * d9 - d4 * d5 );
		t1 = s0 * ( d1 * d9 - d4 * d6 );
		t2 = s0 * ( d2 * d9 - d4 * d7 );

#ifndef DERIVE_UNSMOOTHED_BITANGENT
		t3 = s1 * ( d3 * d5 - d0 * d8 );
		t4 = s1 * ( d3 * d6 - d1 * d8 );
		t5 = s1 * ( d3 * d7 - d2 * d8 );
#else
		t3 = s1 * ( n2 * t1 - n1 * t2 );
		t4 = s1 * ( n0 * t2 - n2 * t0 );
		t5 = s1 * ( n1 * t0 - n0 * t1 );
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
idSIMD_Generic::NormalizeTangents

	Normalizes each vertex normal and projects and normalizes the
	tangent vectors onto the plane orthogonal to the vertex normal.
============
*/
void idSIMD_Generic::NormalizeTangents( idDrawVert *verts, const int numVerts ) {

	for ( int i = 0; i < numVerts; i++ ) {
		idVec3 &v = verts[i].normal;
		float f;

		f = idMath::RSqrt( v.x * v.x + v.y * v.y + v.z * v.z );
		v.x *= f; v.y *= f; v.z *= f;

		for ( int j = 0; j < 2; j++ ) {
			idVec3 &t = verts[i].tangents[j];

			t -= ( t * v ) * v;
			f = idMath::RSqrt( t.x * t.x + t.y * t.y + t.z * t.z );
			t.x *= f; t.y *= f; t.z *= f;
		}
	}
}

/*
============
idSIMD_Generic::CreateShadowCache
============
*/
int idSIMD_Generic::CreateShadowCache( idVec4 *shadowVerts, int *vertRemap, const idVec3 &lightOrigin, const idDrawVert *verts, const int numVerts ) {
	int outVerts = 0;

	for ( int i = 0; i < numVerts; i++ ) {
		if ( vertRemap[i] ) {
			continue;
		}
		const float *v = verts[i].xyz.ToFloatPtr();
		shadowVerts[outVerts+0][0] = v[0];
		shadowVerts[outVerts+0][1] = v[1];
		shadowVerts[outVerts+0][2] = v[2];
		shadowVerts[outVerts+0][3] = 1.0f;

		// R_SetupProjection() builds the projection matrix with a slight crunch
		// for depth, which keeps this w=0 division from rasterizing right at the
		// wrap around point and causing depth fighting with the rear caps
		shadowVerts[outVerts+1][0] = v[0] - lightOrigin[0];
		shadowVerts[outVerts+1][1] = v[1] - lightOrigin[1];
		shadowVerts[outVerts+1][2] = v[2] - lightOrigin[2];
		shadowVerts[outVerts+1][3] = 0.0f;
		vertRemap[i] = outVerts;
		outVerts += 2;
	}
	return outVerts;
}

/*
============
idSIMD_Generic::CreateVertexProgramShadowCache
============
*/
int idSIMD_Generic::CreateVertexProgramShadowCache( idVec4 *shadowVerts, const idDrawVert *verts, const int numVerts ) {
	for ( int i = 0; i < numVerts; i++ ) {
		const float *v = verts[i].xyz.ToFloatPtr();
		shadowVerts[i*2+0][0] = v[0];
		shadowVerts[i*2+1][0] = v[0];
		shadowVerts[i*2+0][1] = v[1];
		shadowVerts[i*2+1][1] = v[1];
		shadowVerts[i*2+0][2] = v[2];
		shadowVerts[i*2+1][2] = v[2];
		shadowVerts[i*2+0][3] = 1.0f;
		shadowVerts[i*2+1][3] = 0.0f;
	}
	return numVerts * 2;
}

/*
============
idSIMD_Generic::UpSamplePCMTo44kHz

  Duplicate samples for 44kHz output.
============
*/
void idSIMD_Generic::UpSamplePCMTo44kHz( float *dest, const short *src, const int numSamples, const int kHz, const int numChannels ) {
	if ( kHz == 11025 ) {
		if ( numChannels == 1 ) {
			for ( int i = 0; i < numSamples; i++ ) {
				dest[i*4+0] = dest[i*4+1] = dest[i*4+2] = dest[i*4+3] = (float) src[i+0];
			}
		} else {
			for ( int i = 0; i < numSamples; i += 2 ) {
				dest[i*4+0] = dest[i*4+2] = dest[i*4+4] = dest[i*4+6] = (float) src[i+0];
				dest[i*4+1] = dest[i*4+3] = dest[i*4+5] = dest[i*4+7] = (float) src[i+1];
			}
		}
	} else if ( kHz == 22050 ) {
		if ( numChannels == 1 ) {
			for ( int i = 0; i < numSamples; i++ ) {
				dest[i*2+0] = dest[i*2+1] = (float) src[i+0];
			}
		} else {
			for ( int i = 0; i < numSamples; i += 2 ) {
				dest[i*2+0] = dest[i*2+2] = (float) src[i+0];
				dest[i*2+1] = dest[i*2+3] = (float) src[i+1];
			}
		}
	} else if ( kHz == 44100 ) {
		for ( int i = 0; i < numSamples; i++ ) {
			dest[i] = (float) src[i];
		}
	} else {
		assert( 0 );
	}
}

/*
============
idSIMD_Generic::UpSampleOGGTo44kHz

  Duplicate samples for 44kHz output.
============
*/
void idSIMD_Generic::UpSampleOGGTo44kHz( float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels ) {
	if ( kHz == 11025 ) {
		if ( numChannels == 1 ) {
			for ( int i = 0; i < numSamples; i++ ) {
				dest[i*4+0] = dest[i*4+1] = dest[i*4+2] = dest[i*4+3] = ogg[0][i] * 32768.0f;
			}
		} else {
			for ( int i = 0; i < numSamples >> 1; i++ ) {
				dest[i*8+0] = dest[i*8+2] = dest[i*8+4] = dest[i*8+6] = ogg[0][i] * 32768.0f;
				dest[i*8+1] = dest[i*8+3] = dest[i*8+5] = dest[i*8+7] = ogg[1][i] * 32768.0f;
			}
		}
	} else if ( kHz == 22050 ) {
		if ( numChannels == 1 ) {
			for ( int i = 0; i < numSamples; i++ ) {
				dest[i*2+0] = dest[i*2+1] = ogg[0][i] * 32768.0f;
			}
		} else {
			for ( int i = 0; i < numSamples >> 1; i++ ) {
				dest[i*4+0] = dest[i*4+2] = ogg[0][i] * 32768.0f;
				dest[i*4+1] = dest[i*4+3] = ogg[1][i] * 32768.0f;
			}
		}
	} else if ( kHz == 44100 ) {
		if ( numChannels == 1 ) {
			for ( int i = 0; i < numSamples; i++ ) {
				dest[i*1+0] = ogg[0][i] * 32768.0f;
			}
		} else {
			for ( int i = 0; i < numSamples >> 1; i++ ) {
				dest[i*2+0] = ogg[0][i] * 32768.0f;
				dest[i*2+1] = ogg[1][i] * 32768.0f;
			}
		}
	} else {
		assert( 0 );
	}
}

/*
============
idSIMD_Generic::MixSoundTwoSpeakerMono
============
*/
void idSIMD_Generic::MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) {
	float sL = lastV[0];
	float sR = lastV[1];
	float incL = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	float incR = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;

	assert( numSamples == MIXBUFFER_SAMPLES );

	for( int j = 0; j < MIXBUFFER_SAMPLES; j++ ) {
		mixBuffer[j*2+0] += samples[j] * sL;
		mixBuffer[j*2+1] += samples[j] * sR;
		sL += incL;
		sR += incR;
	}
}

/*
============
idSIMD_Generic::MixSoundTwoSpeakerStereo
============
*/
void idSIMD_Generic::MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) {
	float sL = lastV[0];
	float sR = lastV[1];
	float incL = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	float incR = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;

	assert( numSamples == MIXBUFFER_SAMPLES );

	for( int j = 0; j < MIXBUFFER_SAMPLES; j++ ) {
		mixBuffer[j*2+0] += samples[j*2+0] * sL;
		mixBuffer[j*2+1] += samples[j*2+1] * sR;
		sL += incL;
		sR += incR;
	}
}

/*
============
idSIMD_Generic::MixSoundSixSpeakerMono
============
*/
void idSIMD_Generic::MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) {
	float sL0 = lastV[0];
	float sL1 = lastV[1];
	float sL2 = lastV[2];
	float sL3 = lastV[3];
	float sL4 = lastV[4];
	float sL5 = lastV[5];

	float incL0 = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	float incL1 = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;
	float incL2 = ( currentV[2] - lastV[2] ) / MIXBUFFER_SAMPLES;
	float incL3 = ( currentV[3] - lastV[3] ) / MIXBUFFER_SAMPLES;
	float incL4 = ( currentV[4] - lastV[4] ) / MIXBUFFER_SAMPLES;
	float incL5 = ( currentV[5] - lastV[5] ) / MIXBUFFER_SAMPLES;

	assert( numSamples == MIXBUFFER_SAMPLES );

	for( int i = 0; i < MIXBUFFER_SAMPLES; i++ ) {
		mixBuffer[i*6+0] += samples[i] * sL0;
		mixBuffer[i*6+1] += samples[i] * sL1;
		mixBuffer[i*6+2] += samples[i] * sL2;
		mixBuffer[i*6+3] += samples[i] * sL3;
		mixBuffer[i*6+4] += samples[i] * sL4;
		mixBuffer[i*6+5] += samples[i] * sL5;
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
idSIMD_Generic::MixSoundSixSpeakerStereo
============
*/
void idSIMD_Generic::MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) {
	float sL0 = lastV[0];
	float sL1 = lastV[1];
	float sL2 = lastV[2];
	float sL3 = lastV[3];
	float sL4 = lastV[4];
	float sL5 = lastV[5];

	float incL0 = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	float incL1 = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;
	float incL2 = ( currentV[2] - lastV[2] ) / MIXBUFFER_SAMPLES;
	float incL3 = ( currentV[3] - lastV[3] ) / MIXBUFFER_SAMPLES;
	float incL4 = ( currentV[4] - lastV[4] ) / MIXBUFFER_SAMPLES;
	float incL5 = ( currentV[5] - lastV[5] ) / MIXBUFFER_SAMPLES;

	assert( numSamples == MIXBUFFER_SAMPLES );

	for( int i = 0; i < MIXBUFFER_SAMPLES; i++ ) {
		mixBuffer[i*6+0] += samples[i*2+0] * sL0;
		mixBuffer[i*6+1] += samples[i*2+1] * sL1;
		mixBuffer[i*6+2] += samples[i*2+0] * sL2;
		mixBuffer[i*6+3] += samples[i*2+0] * sL3;
		mixBuffer[i*6+4] += samples[i*2+0] * sL4;
		mixBuffer[i*6+5] += samples[i*2+1] * sL5;
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
idSIMD_Generic::MixedSoundToSamples
============
*/
void idSIMD_Generic::MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples ) {

	for ( int i = 0; i < numSamples; i++ ) {
		if ( mixBuffer[i] <= -32768.0f ) {
			samples[i] = -32768;
		} else if ( mixBuffer[i] >= 32767.0f ) {
			samples[i] = 32767;
		} else {
			samples[i] = (short) mixBuffer[i];
		}
	}
}

/*
============
idSIMD_Generic::CullByFrustum
Moved from R_CalcInteractionCullBits
============
*/
void idSIMD_Generic::CullByFrustum( idDrawVert *verts, const int numVerts, const idPlane frustum[6], byte *pointCull, float epsilon ) {
	for ( int j = 0; j < numVerts; j++ ) {
		const idVec3 &vec = verts[j].xyz;
		byte bits = 0;
		for ( int i = 0; i < 6; i++ ) {
			float d = frustum[i].Distance( vec );
			bits |= (d < epsilon) << i;
		}
		pointCull[j] = bits;
	}
}
/*
============
idSIMD_Generic::CullByFrustum2
Moved from R_CalcPointCull
============
*/
void idSIMD_Generic::CullByFrustum2( idDrawVert *verts, const int numVerts, const idPlane frustum[6], unsigned short *pointCull, float epsilon ) {
	for ( int j = 0; j < numVerts; j++ ) {
		const idVec3 &vec = verts[j].xyz;
		short bits = 0;
		for ( int i = 0; i < 6; i++ ) {
			float d = frustum[i].Distance( vec );
			bits |= (d < epsilon) << i;
			bits |= (d > -epsilon) << (i + 6);
		}
		pointCull[j] = bits;
	}
}

/*
============
idSIMD_Generic::CullTrisByFrustum
============
*/
void idSIMD_Generic::CullTrisByFrustum( idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes, const idPlane frustum[6], byte *triCull, float epsilon ) {
	assert(numIndexes % 3 == 0);
	int numTris = numIndexes / 3;
	for ( int t = 0; t < numTris; t++ ) {
		int i0 = indexes[3 * t + 0];
		int i1 = indexes[3 * t + 1];
		int i2 = indexes[3 * t + 2];
		idVec3 vert0 = verts[i0].xyz;
		idVec3 vert1 = verts[i1].xyz;
		idVec3 vert2 = verts[i2].xyz;
		byte bits0 = 0, bits1 = 0, bits2 = 0;
		for ( int p = 0; p < 6; p++ ) {
			float da = frustum[p].Distance( vert0 );
			float db = frustum[p].Distance( vert1 );
			float dc = frustum[p].Distance( vert2 );
			bits0 |= (da < epsilon) << p;
			bits1 |= (db < epsilon) << p;
			bits2 |= (dc < epsilon) << p;
		}
		triCull[t] = bits0 & bits1 & bits2;
	}
}

/*
============
idSIMD_Generic::GenerateMipMap2x2
============
*/
void idSIMD_Generic::GenerateMipMap2x2( const byte *srcPtr, int srcStride, int halfWidth, int halfHeight, byte *dstPtr, int dstStride ) {
	for (int i = 0; i < halfHeight; i++) {
		const byte *inRow0 = &srcPtr[(2*i+0) * srcStride];
		const byte *inRow1 = &srcPtr[(2*i+1) * srcStride];
		byte *outRow = &dstPtr[i * dstStride];

		for (int j = 0; j < halfWidth; j++) {
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

static void LoadBlockChannel( byte block[16], const byte *srcPtr, int width, int height, int stride, int brow, int bcol, int channel ) {
	for (int r = 0; r < 4; r++)
		for (int c = 0; c < 4; c++) {
			int i = brow * 4 + r;
			int j = bcol * 4 + c;
			// use "clamp" continuation
			if (i > height - 1)
				i = height - 1;
			if (j > width - 1)
				j = width - 1;
			byte &dstPixel = block[4 * r + c];
			const byte *srcPixel = &srcPtr[i * stride + 4 * j];
			dstPixel = srcPixel[channel];
		}
}

static void LoadBlockFull( byte block[16][4], const byte *srcPtr, int width, int height, int stride, int brow, int bcol ) {
	for (int r = 0; r < 4; r++)
		for (int c = 0; c < 4; c++) {
			int i = brow * 4 + r;
			int j = bcol * 4 + c;
			// use "clamp" continuation
			if (i > height - 1)
				i = height - 1;
			if (j > width - 1)
				j = width - 1;
			byte *dstPixel = block[4 * r + c];
			const byte *srcPixel = &srcPtr[i * stride + 4 * j];
			for (int c = 0; c < 4; c++)
				dstPixel[c] = srcPixel[c];
		}
}

static uint64 ProcessAlphaBlock( const byte block[16] ) {
	// compute min/max
	int minv = block[0];
	int maxv = block[0];
	for (int i = 0; i < 16; i++) {
		minv = idMath::Imin(minv, block[i]);
		maxv = idMath::Imax(maxv, block[i]);
	}

	// make sure min < max, so that 7-step case is used
	if (minv == maxv) {
		if (maxv < 255)
			maxv++;
		else
			minv--;
	}

	uint64 blockData = maxv + (minv << 8);
	int bits = 16;
	for (int i = 0; i < 16; i++) {
		// compute ratio
		int numer = block[i] - minv;
		int denom = maxv - minv;
#if 0
		// find closest ramp point
		int idx = (numer * 7 + (denom >> 1)) / denom;
#else
		// this code yields closest ramp point in most cases
		// among all 32K ratios D/N, there are 258 exceptions with N >= 65
		// in exceptional cases, ratio is close to middle, and chosen ramp point is almost as close
		// Note: this code is used here to match CompressRGTCFromRGBA8_Kernel8x4 !
		int mult = ((7 << 12) + denom-1) / denom;
		int idx = (mult * numer + (1 << 11)) >> 12;
#endif
		// convert to DXT5 index
		int val = 8 - idx;
		if (idx == 7)
			val = 0;
		if (idx == 0)
			val = 1;
		//append to bit stream
		blockData += uint64(val) << bits;
		bits += 3;
	}

	assert(bits == 64);
	return blockData;
}

static uint64 ProcessAlphaBlock4b( const byte block[16] ) {
	uint64 result = 0;
	for (int i = 15; i >= 0; i--) {
		int x = (int(block[i]) * 15 + 127) / 255;
		assert(x >= 0 && x < 16);
		result = (result << 4) + x;
	}
	return result;
}

static uint64 ProcessColorBlock( const byte block[16][4] ) {
	// compute average color
	uint16 sumColor[3] = {0};
	for (int i = 0; i < 16; i++)
		for (int c = 0; c < 3; c++)
			sumColor[c] += block[i][c];
	uint8 avgColor[3];
	for (int c = 0; c < 3; c++)
		avgColor[c] = (sumColor[c] + 7) >> 4;

	// compute covariance matrix
	float covMatr[3][3] = {{0.0f}};
	for (int i = 0; i < 16; i++) {
		float r = block[i][0];  r -= avgColor[0];
		float g = block[i][1];  g -= avgColor[1];
		float b = block[i][2];  b -= avgColor[2];
		covMatr[0][0] += r * r;
		covMatr[0][1] += r * g;
		covMatr[0][2] += r * b;
		covMatr[1][1] += g * g;
		covMatr[1][2] += g * b;
		covMatr[2][2] += b * b;
	}
	covMatr[1][0] = covMatr[0][1];
	covMatr[2][0] = covMatr[0][2];
	covMatr[2][1] = covMatr[1][2];

	// compute maximum eigenvector of covariance matrix
	// using a few power iterations from (1,1,1) vector
	float vec[3] = {1.0f, 1.0f, 1.0f};
	for (int pwr = 0; pwr < 3; pwr++) {
		float nvec[3];
		for (int c = 0; c < 3; c++)
			nvec[c] = covMatr[c][0] * vec[0] + covMatr[c][1] * vec[1] + covMatr[c][2] * vec[2];
		for (int c = 0; c < 3; c++)
			vec[c] = nvec[c];
	}
	// singular matrix (e.g. constant color block) can result in zero vector
	// otherwise it is >= 1 since it is integer
	float norm = sqrtf(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
	if (norm == 0.0f) {
		static const float SQRT3 = sqrtf(3.0f);
		vec[0] = vec[1] = vec[2] = 1.0f;
		norm = SQRT3;
	}
	// normalize eigenvector to length = 64 and round to integer
	float normMultiplier = 64.0f / norm;
	int8 axis[3];
	for (int c = 0; c < 3; c++)
		axis[c] = int8(idMath::FtoiRound(vec[c] * normMultiplier));

	// find range of colors projected on the eigenvector axis
	int16 minDot = INT16_MAX, maxDot = INT16_MIN;
	int minIdx = -1, maxIdx = -1;
	for (int i = 0; i < 16; i++) {
		int16 r = block[i][0];  r -= avgColor[0];
		int16 g = block[i][1];  g -= avgColor[1];
		int16 b = block[i][2];  b -= avgColor[2];
		int16 dot = int16(axis[0]) * r + int16(axis[1]) * g + int16(axis[2]) * b;
		minDot = std::min(minDot, dot);
		maxDot = std::max(maxDot, dot);
	}
	// compute endpoints of the range
	int16 bmin[3], bmax[3];
	for (int c = 0; c < 3; c++) {
		// note: divide by 64^2 because length(axis) = 64
		bmin[c] = avgColor[c] + ((int32(axis[c]) * minDot) >> 12);
		bmax[c] = avgColor[c] + ((int32(axis[c]) * maxDot) >> 12);
	}
	// reduce range by 12.5% and use its endpoints as key colors
	int16 keyColorA[3], keyColorB[3];
	for (int c = 0; c < 3; c++) {
		// note: this is quite helpful on photos
		int16 diag = bmax[c] - bmin[c];
		keyColorA[c] = bmin[c] + (diag >> 3);
		keyColorB[c] = bmax[c] - (diag >> 3);
	}

	uint8 key565A[3], key565B[3];	// key colors packed as RGB565
	uint8 ids[16];					// indices of pixels [0..3]

	// compute block and do refinement a few times
	for (int iter = 0; ; iter++) {

		for (int c = 0; c < 3; c++) {
			// slightly push almost equal colors apart from each other (emulated rounding in opposite directions)
			// note: this special rounding is very important for reducing color banding on gradients
			int threshold = (c == 1 ? 4 : 8);
			int shift = (threshold - 1) >> 1;	// 1 or 3
			if (idMath::Abs(keyColorA[c] - keyColorB[c]) < threshold) {
				keyColorA[c] += (keyColorA[c] < keyColorB[c] ? -shift : shift);
				keyColorB[c] += (keyColorA[c] < keyColorB[c] ? shift : -shift);
			}
			// clamp key colors to [0..255] range
			keyColorA[c] = idMath::ClampInt(0, 255, int(keyColorA[c]));
			keyColorB[c] = idMath::ClampInt(0, 255, int(keyColorB[c]));
		}
		// reduce key colors to 565
		for (int c = 0; c < 3; c++) {
			int maxValue = (c == 1 ? 63 : 31);
			key565A[c] = uint8((int16(keyColorA[c]) * maxValue + 127) / 255);
			key565B[c] = uint8((int16(keyColorB[c]) * maxValue + 127) / 255);
		}
		// make sure colors are not exactly equal: avoid encoding with transparency 
		if (key565A[0] == key565B[0] && key565A[1] == key565B[1] && key565A[2] == key565B[2]) {
			key565B[1] += (key565B[1] < 32 ? 1 : -1);
		}
		// recompute 8-bit representation of key colors
		for (int c = 0; c < 3; c++) {
			int maxValue = (c == 1 ? 63 : 31);
			int addedHalf = maxValue >> 1;	// 31 or 15
			keyColorA[c] = ((uint16(key565A[c]) * 255 + addedHalf) / maxValue);
			keyColorB[c] = ((uint16(key565B[c]) * 255 + addedHalf) / maxValue);
		}

		// prepare indices computation
		int32 denom = 0;
		for (int c = 0; c < 3; c++)
			denom += (int32(keyColorB[c]) - keyColorA[c]) * (int32_t(keyColorB[c]) - keyColorA[c]);
		assert(denom > 0);
		// reciprocal value for denominator
		// it is increased slightly to ensure that colors with exact index don't get rounded below it
		float invDenom3 = 3.0f / denom + 3.0f * FLT_EPSILON;
		// compute indices
		for (int i = 0; i < 16; i++) {
			int32 numer = 0;
			for (int c = 0; c < 3; c++)
				numer += (int32(block[i][c]) - keyColorA[c]) * (int32(keyColorB[c]) - keyColorA[c]);
			int k = idMath::FtoiRound(numer * invDenom3);
			k = idMath::ClampInt(0, 3, k);
			ids[i] = k;
		}

		// we do 1 iteration of refinement + recompute block after that
		if (iter == 1)
			break;

		// least squares refinement: find best non-integer key colors for fixed indices
		// compute matrix and right side of 2x2 equation system
		uint16 alpha = 0, beta = 0, gamma = 0;
		uint16 rightA[3] = {0}, rightB[3] = {0};
		for (int i = 0; i < 16; i++) {
			alpha += (3 - ids[i]) * (3 - ids[i]);
			beta += ids[i] * ids[i];
			gamma += ids[i] * (3 - ids[i]);
			for (int c = 0; c < 3; c++)
				rightB[c] += ids[i] * uint16(block[i][c]);
		}
		for (int c = 0; c < 3; c++) {
			rightA[c] = sumColor[c] * 3 - rightB[c];
			rightA[c] *= 3;
			rightB[c] *= 3;
		}

		// don't do anything if dmatrix is singular
		int32_t det = int32(alpha) * beta - int32(gamma) * gamma;
		assert(det >= 0);
		if (det != 0) {
			float invDet = 1.0f / det;
			// solve system
			for (int c = 0; c < 3; c++) {
				int32 detA = int32(rightA[c]) * beta - int32(rightB[c]) * gamma;
				int32 detB = int32(rightB[c]) * alpha - int32(rightA[c]) * gamma;
				keyColorA[c] = idMath::FtoiRound(detA * invDet);
				keyColorB[c] = idMath::FtoiRound(detB * invDet);
			}
		}
	}

	// pack key colors into 32 bits
	uint16 baseA = uint16(key565A[2]) + (uint16(key565A[1]) << 5) + (uint16(key565A[0]) << 11);
	uint16 baseB = uint16(key565B[2]) + (uint16(key565B[1]) << 5) + (uint16(key565B[0]) << 11);
	assert(baseA != baseB);

	// pack indices into 32 bits
	uint32_t mask = 0;
	for (int i = 15; i >= 0; i--) {
		// reorder for DXT: c0 = 0, c1 = 3, c2 = 1, c3 = 2
		int q = ids[i];
		if (q == 3)
			q = 1;
		else if (q > 0)
			q++;

		mask = (mask << 2) + q;
	}

	// swap key colors if necessary, inverting indices
	if (baseA < baseB) {
		std::swap(baseA, baseB);
		mask ^= 0x55555555;
	}

	uint64 result = (((uint64(mask) << 16) + baseB) << 16) + baseA;
	return result;
}

}

/*
============
idSIMD_Generic::CompressRGTCFromRGBA8
============
*/
void idSIMD_Generic::CompressRGTCFromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) {
	int bw = (width + 3) / 4;
	int bh = (height + 3) / 4;
	uint64 *dstBlocks = (uint64*)dstPtr;
	byte block[16];

	using namespace DxtCompress;
	for (int brow = 0; brow < bh; brow++) {
		for (int bcol = 0; bcol < bw; bcol++) {
			for (int comp = 0; comp < 2; comp++) {
				LoadBlockChannel( block, srcPtr, width, height, stride, brow, bcol, comp );
				*dstBlocks++ = ProcessAlphaBlock( block );
			}
		}
	}
}

/*
============
idSIMD_Generic::CompressDXT1FromRGBA8
============
*/
void idSIMD_Generic::CompressDXT1FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) {
	int bw = (width + 3) / 4;
	int bh = (height + 3) / 4;
	uint64 *dstBlocks = (uint64*)dstPtr;
	byte block[16][4];

	using namespace DxtCompress;
	for (int brow = 0; brow < bh; brow++) {
		for (int bcol = 0; bcol < bw; bcol++) {
			LoadBlockFull( block, srcPtr, width, height, stride, brow, bcol );
			*dstBlocks++ = ProcessColorBlock( block );
		}
	}
}

/*
============
idSIMD_Generic::CompressDXT3FromRGBA8
============
*/
void idSIMD_Generic::CompressDXT3FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) {
	int bw = (width + 3) / 4;
	int bh = (height + 3) / 4;
	uint64 *dstBlocks = (uint64*)dstPtr;
	byte block[16][4], alpha[16];

	using namespace DxtCompress;
	for (int brow = 0; brow < bh; brow++) {
		for (int bcol = 0; bcol < bw; bcol++) {
			LoadBlockFull( block, srcPtr, width, height, stride, brow, bcol );
			LoadBlockChannel( alpha, srcPtr, width, height, stride, brow, bcol, 3 );
			*dstBlocks++ = ProcessAlphaBlock4b( alpha );
			*dstBlocks++ = ProcessColorBlock( block );
		}
	}
}

/*
============
idSIMD_Generic::CompressDXT5FromRGBA8
============
*/
void idSIMD_Generic::CompressDXT5FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) {
	int bw = (width + 3) / 4;
	int bh = (height + 3) / 4;
	uint64 *dstBlocks = (uint64*)dstPtr;
	byte block[16][4], alpha[16];

	using namespace DxtCompress;
	for (int brow = 0; brow < bh; brow++) {
		for (int bcol = 0; bcol < bw; bcol++) {
			LoadBlockFull( block, srcPtr, width, height, stride, brow, bcol );
			LoadBlockChannel( alpha, srcPtr, width, height, stride, brow, bcol, 3 );
			*dstBlocks++ = ProcessAlphaBlock( alpha );
			*dstBlocks++ = ProcessColorBlock( block );
		}
	}
}

/*
============
idSIMD_Generic::ConvertRowToRGBA8
============
*/
bool idSIMD_Generic::ConvertRowToRGBA8( const byte *srcPtr, int width, int bitsPerPixel, bool bgr, byte *dstPtr ) {
	if (bitsPerPixel == 8) {
		for (int i = 0; i < width; i++) {
			byte val = srcPtr[i];
			dstPtr[4*i+0] = val;
			dstPtr[4*i+1] = val;
			dstPtr[4*i+2] = val;
			dstPtr[4*i+3] = 255;
		}
		return true;
	}

	if (bitsPerPixel == 32) {
		if (bgr) {
			for (int i = 0; i < width; i++) {
				dstPtr[4*i+0] = srcPtr[4*i+2];
				dstPtr[4*i+1] = srcPtr[4*i+1];
				dstPtr[4*i+2] = srcPtr[4*i+0];
				dstPtr[4*i+3] = srcPtr[4*i+3];
			}
		}
		else {
			for (int i = 0; i < width; i++) {
				dstPtr[4*i+0] = srcPtr[4*i+0];
				dstPtr[4*i+1] = srcPtr[4*i+1];
				dstPtr[4*i+2] = srcPtr[4*i+2];
				dstPtr[4*i+3] = srcPtr[4*i+3];
			}
		}
		return true;
	}

	if (bitsPerPixel == 24) {
		if (bgr) {
			for (int i = 0; i < width; i++) {
				dstPtr[4*i+0] = srcPtr[3*i+2];
				dstPtr[4*i+1] = srcPtr[3*i+1];
				dstPtr[4*i+2] = srcPtr[3*i+0];
				dstPtr[4*i+3] = 255;
			}
		}
		else {
			for (int i = 0; i < width; i++) {
				dstPtr[4*i+0] = srcPtr[3*i+0];
				dstPtr[4*i+1] = srcPtr[3*i+1];
				dstPtr[4*i+2] = srcPtr[3*i+2];
				dstPtr[4*i+3] = 255;
			}
		}
		return true;
	}

	return false;
}


ID_FORCE_INLINE static void Color565to888f(word data, float rgb[3]) {
	//red goes in high bits (like in big-endian)
	uint r = (data >> 11) & 0x1F;
	uint g = (data >> 5 ) & 0x3F;
	uint b = (data >> 0 ) & 0x1F;
	//k-bit unsigned integer X means float color X / (2^k - 1) in [0..1]
	//here we return it already scaled to [0..255] interval (prepared for rounding to 8 bits)
	rgb[0] = r * (255.0f / 31.0f);
	rgb[1] = g * (255.0f / 63.0f);
	rgb[2] = b * (255.0f / 31.0f);
}
ID_FORCE_INLINE static void Color888fto888(const float src[3], byte dst[3]) {
	dst[0] = byte( src[0] + (0.5f + 1e-4f) );
	dst[1] = byte( src[1] + (0.5f + 1e-4f) );
	dst[2] = byte( src[2] + (0.5f + 1e-4f) );
}

ID_FORCE_INLINE static void Dxt1Keys3(const float mainKeys[2][3], byte keys[4][4]) {
	Color888fto888(mainKeys[0], keys[0]);
	Color888fto888(mainKeys[1], keys[1]);
	keys[2][0] = byte( (2.0f * mainKeys[0][0] + 1.0f * mainKeys[1][0]) * (1.0f / 3.0f) + (0.5f + 1e-4f) );
	keys[2][1] = byte( (2.0f * mainKeys[0][1] + 1.0f * mainKeys[1][1]) * (1.0f / 3.0f) + (0.5f + 1e-4f) );
	keys[2][2] = byte( (2.0f * mainKeys[0][2] + 1.0f * mainKeys[1][2]) * (1.0f / 3.0f) + (0.5f + 1e-4f) );
	keys[3][0] = byte( (1.0f * mainKeys[0][0] + 2.0f * mainKeys[1][0]) * (1.0f / 3.0f) + (0.5f + 1e-4f) );
	keys[3][1] = byte( (1.0f * mainKeys[0][1] + 2.0f * mainKeys[1][1]) * (1.0f / 3.0f) + (0.5f + 1e-4f) );
	keys[3][2] = byte( (1.0f * mainKeys[0][2] + 2.0f * mainKeys[1][2]) * (1.0f / 3.0f) + (0.5f + 1e-4f) );
}
ID_FORCE_INLINE static void Dxt1Keys2(const float mainKeys[2][3], byte keys[4][4]) {
	Color888fto888(mainKeys[0], keys[0]);
	Color888fto888(mainKeys[1], keys[1]);
	keys[2][0] = byte( (mainKeys[0][0] + mainKeys[1][0]) * 0.5f + (0.5f + 1e-4f) );
	keys[2][1] = byte( (mainKeys[0][1] + mainKeys[1][1]) * 0.5f + (0.5f + 1e-4f) );
	keys[2][2] = byte( (mainKeys[0][2] + mainKeys[1][2]) * 0.5f + (0.5f + 1e-4f) );
	keys[3][0] = 0;
	keys[3][1] = 0;
	keys[3][2] = 0;
}
ID_FORCE_INLINE static void Dxt5Keys7(byte keys[8]) {
	keys[2] = (keys[0] * 6 + keys[1] * 1 + 3) / 7;
	keys[3] = (keys[0] * 5 + keys[1] * 2 + 3) / 7;
	keys[4] = (keys[0] * 4 + keys[1] * 3 + 3) / 7;
	keys[5] = (keys[0] * 3 + keys[1] * 4 + 3) / 7;
	keys[6] = (keys[0] * 2 + keys[1] * 5 + 3) / 7;
	keys[7] = (keys[0] * 1 + keys[1] * 6 + 3) / 7;
}
ID_FORCE_INLINE static void Dxt5Keys5(byte keys[8]) {
	keys[2] = (keys[0] * 4 + keys[1] * 1 + 2) / 5;
	keys[3] = (keys[0] * 3 + keys[1] * 2 + 2) / 5;
	keys[4] = (keys[0] * 2 + keys[1] * 3 + 2) / 5;
	keys[5] = (keys[0] * 1 + keys[1] * 4 + 2) / 5;
	keys[6] = 0;
	keys[7] = 0xFF;
}

/*
============
idSIMD_Generic::DecompressRGBA8FromDXT1
============
*/
void idSIMD_Generic::DecompressRGBA8FromDXT1( const byte *srcPtr, int width, int height, byte *dstPtr, int stride, bool allowTransparency ) {
	int bw = (width + 3) / 4;
	int bh = (height + 3) / 4;
	const uint64 *srcBlocks = (uint64*)srcPtr;

	for (int brow = 0; brow < bh; brow++) {
		for (int bcol = 0; bcol < bw; bcol++) {
			uint64 blockData = *srcBlocks++;

			word keyColor[2];
			float keyFloat[2][3];
			for (int k = 0; k < 2; k++) {
				keyColor[k] = blockData & 0xFFFF;
				blockData >>= 16;
				Color565to888f(keyColor[k], keyFloat[k]);
			}

			byte keyRgba[4][4];
			for (int k = 0; k < 4; k++)
				keyRgba[k][3] = 0xFF;

			if (keyColor[0] > keyColor[1])
				Dxt1Keys3(keyFloat, keyRgba);
			else {
				Dxt1Keys2(keyFloat, keyRgba);
				if (allowTransparency)
					keyRgba[3][3] = 0;
			}

			for (int r = 0; r < 4; r++)
				for (int c = 0; c < 4; c++) {
					uint idx = (blockData & 0x3);
					blockData >>= 2;

					int i = brow * 4 + r;
					int j = bcol * 4 + c;
					if (i > height - 1 || j > width - 1)
						continue;

					byte *outPixel = &dstPtr[i * stride + 4 * j];
					outPixel[0] = keyRgba[idx][0];
					outPixel[1] = keyRgba[idx][1];
					outPixel[2] = keyRgba[idx][2];
					outPixel[3] = keyRgba[idx][3];
				}
		}
	}
}

/*
============
idSIMD_Generic::DecompressRGBA8FromDXT3
============
*/
void idSIMD_Generic::DecompressRGBA8FromDXT3( const byte *srcPtr, int width, int height, byte *dstPtr, int stride ) {
	int bw = (width + 3) / 4;
	int bh = (height + 3) / 4;
	const uint64 *srcBlocks = (uint64*)srcPtr;

	for (int brow = 0; brow < bh; brow++) {
		for (int bcol = 0; bcol < bw; bcol++) {
			uint64 alphaData = *srcBlocks++;
			uint64 rgbData = *srcBlocks++;

			float keyFloat[2][3];
			for (int k = 0; k < 2; k++) {
				word rgb565 = rgbData & 0xFFFF;
				rgbData >>= 16;
				Color565to888f(rgb565, keyFloat[k]);
			}

			byte keyRgba[4][4];
			Dxt1Keys3(keyFloat, keyRgba);

			for (int r = 0; r < 4; r++)
				for (int c = 0; c < 4; c++) {
					uint rgbIdx = (rgbData & 0x3);
					rgbData >>= 2;
					uint alphaBits = (alphaData & 0xF);
					alphaData >>= 4;

					int i = brow * 4 + r;
					int j = bcol * 4 + c;
					if (i > height - 1 || j > width - 1)
						continue;

					byte *outPixel = &dstPtr[i * stride + 4 * j];
					outPixel[0] = keyRgba[rgbIdx][0];
					outPixel[1] = keyRgba[rgbIdx][1];
					outPixel[2] = keyRgba[rgbIdx][2];
					outPixel[3] = alphaBits * 17;	//17 = 255 / 15
				}
		}
	}
}

/*
============
idSIMD_Generic::DecompressRGBA8FromDXT5
============
*/
void idSIMD_Generic::DecompressRGBA8FromDXT5( const byte *srcPtr, int width, int height, byte *dstPtr, int stride ) {
	int bw = (width + 3) / 4;
	int bh = (height + 3) / 4;
	const uint64 *srcBlocks = (uint64*)srcPtr;

	for (int brow = 0; brow < bh; brow++) {
		for (int bcol = 0; bcol < bw; bcol++) {
			uint64 alphaData = *srcBlocks++;
			uint64 rgbData = *srcBlocks++;

			float keyFloat[2][3];
			for (int k = 0; k < 2; k++) {
				word rgb565 = rgbData & 0xFFFF;
				rgbData >>= 16;
				Color565to888f(rgb565, keyFloat[k]);
			}

			byte keyAlpha[8];
			keyAlpha[0] = (alphaData & 0xFF);
			alphaData >>= 8;
			keyAlpha[1] = (alphaData & 0xFF);
			alphaData >>= 8;

			byte keyRgba[4][4];
			Dxt1Keys3(keyFloat, keyRgba);

			if (keyAlpha[0] > keyAlpha[1])
				Dxt5Keys7(keyAlpha);
			else
				Dxt5Keys5(keyAlpha);

			for (int r = 0; r < 4; r++)
				for (int c = 0; c < 4; c++) {
					uint rgbIdx = (rgbData & 0x3);
					rgbData >>= 2;
					uint alphaIdx = (alphaData & 0x7);
					alphaData >>= 3;

					int i = brow * 4 + r;
					int j = bcol * 4 + c;
					if (i > height - 1 || j > width - 1)
						continue;

					byte *outPixel = &dstPtr[i * stride + 4 * j];
					outPixel[0] = keyRgba[rgbIdx][0];
					outPixel[1] = keyRgba[rgbIdx][1];
					outPixel[2] = keyRgba[rgbIdx][2];
					outPixel[3] = keyAlpha[alphaIdx];
				}
		}
	}
}

/*
============
idSIMD_Generic::DecompressRGBA8FromRGTC
============
*/
void idSIMD_Generic::DecompressRGBA8FromRGTC( const byte *srcPtr, int width, int height, byte *dstPtr, int stride ) {
	int bw = (width + 3) / 4;
	int bh = (height + 3) / 4;
	const uint64 *srcBlocks = (uint64*)srcPtr;

	for (int brow = 0; brow < bh; brow++) {
		for (int bcol = 0; bcol < bw; bcol++) {
			uint64 redData = *srcBlocks++;
			uint64 greenData = *srcBlocks++;

			byte keyRed[8], keyGreen[8];
			keyRed[0] = (redData & 0xFF);
			redData >>= 8;
			keyRed[1] = (redData & 0xFF);
			redData >>= 8;

			keyGreen[0] = (greenData & 0xFF);
			greenData >>= 8;
			keyGreen[1] = (greenData & 0xFF);
			greenData >>= 8;

			if (keyRed[0] > keyRed[1])
				Dxt5Keys7(keyRed);
			else
				Dxt5Keys5(keyRed);

			if (keyGreen[0] > keyGreen[1])
				Dxt5Keys7(keyGreen);
			else
				Dxt5Keys5(keyGreen);

			for (int r = 0; r < 4; r++)
				for (int c = 0; c < 4; c++) {
					uint redIdx = (redData & 0x7);
					redData >>= 3;
					uint greenIdx = (greenData & 0x7);
					greenData >>= 3;

					int i = brow * 4 + r;
					int j = bcol * 4 + c;
					if (i > height - 1 || j > width - 1)
						continue;

					byte *outPixel = &dstPtr[i * stride + 4 * j];
					outPixel[0] = keyRed[redIdx];
					outPixel[1] = keyGreen[greenIdx];
					outPixel[2] = 0;
					outPixel[3] = 255;
				}
		}
	}
}
