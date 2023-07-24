// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "Simd_Generic.h"

#pragma warning( disable : 4244 )


//===============================================================
//
//	Generic implementation of idSIMDProcessor
//
//===============================================================

#ifdef _DEBUG
#define NODEFAULT	default: assert( 0 )
#elif _WIN32
#define NODEFAULT	default: __assume( 0 )
#else
#define NODEFAULT
#endif

#define UNROLL1(Y) { int _IX; for (_IX=0;_IX<count;_IX++) {Y(_IX);} }
#define UNROLL2(Y) { int _IX, _NM = count&0xfffffffe; for (_IX=0;_IX<_NM;_IX+=2){Y(_IX+0);Y(_IX+1);} if (_IX < count) {Y(_IX);}}
#define UNROLL4(Y) { int _IX, _NM = count&0xfffffffc; for (_IX=0;_IX<_NM;_IX+=4){Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);}for(;_IX<count;_IX++){Y(_IX);}}
#define UNROLL8(Y) { int _IX, _NM = count&0xfffffff8; for (_IX=0;_IX<_NM;_IX+=8){Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);Y(_IX+4);Y(_IX+5);Y(_IX+6);Y(_IX+7);} _NM = count&0xfffffffe; for(;_IX<_NM;_IX+=2){Y(_IX); Y(_IX+1);} if (_IX < count) {Y(_IX);} }

#define OPTIMIZED_SHADOW_VOLUME_CODE
#define OPTIMIZED_TRI_PLANE_CODE

/*
============
idSIMD_Generic::GetName
============
*/
const char * idSIMD_Generic::GetName( void ) const {
	return "generic code";
}

/*
============
idSIMD_Generic::Add

  dst[i] = constant + src[i];
============
*/
void VPCALL idSIMD_Generic::Add( float *dst, const float constant, const float *src, const int count ) {
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
void VPCALL idSIMD_Generic::Add( float *dst, const float *src0, const float *src1, const int count ) {
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
void VPCALL idSIMD_Generic::Sub( float *dst, const float constant, const float *src, const int count ) {
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
void VPCALL idSIMD_Generic::Sub( float *dst, const float *src0, const float *src1, const int count ) {
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
void VPCALL idSIMD_Generic::Mul( float *dst, const float constant, const float *src0, const int count) {
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
void VPCALL idSIMD_Generic::Mul( float *dst, const float *src0, const float *src1, const int count ) {
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
void VPCALL idSIMD_Generic::Div( float *dst, const float constant, const float *divisor, const int count ) {
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
void VPCALL idSIMD_Generic::Div( float *dst, const float *src0, const float *src1, const int count ) {
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
void VPCALL idSIMD_Generic::MulAdd( float *dst, const float constant, const float *src, const int count ) {
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
void VPCALL idSIMD_Generic::MulAdd( float *dst, const float *src0, const float *src1, const int count ) {
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
void VPCALL idSIMD_Generic::MulSub( float *dst, const float constant, const float *src, const int count ) {
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
void VPCALL idSIMD_Generic::MulSub( float *dst, const float *src0, const float *src1, const int count ) {
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
void VPCALL idSIMD_Generic::Dot( float *dst, const idVec3 &constant, const idVec3 *src, const int count ) {
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
void VPCALL idSIMD_Generic::Dot( float *dst, const idVec3 &constant, const idPlane *src, const int count ) {
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
void VPCALL idSIMD_Generic::Dot( float *dst, const idVec3 &constant, const idDrawVert *src, const int count ) {
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
void VPCALL idSIMD_Generic::Dot( float *dst, const idPlane &constant, const idVec3 *src, const int count ) {
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
void VPCALL idSIMD_Generic::Dot( float *dst, const idPlane &constant, const idPlane *src, const int count ) {
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
void VPCALL idSIMD_Generic::Dot( float *dst, const idPlane &constant, const idDrawVert *src, const int count ) {
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
void VPCALL idSIMD_Generic::Dot( float *dst, const idVec3 *src0, const idVec3 *src1, const int count ) {
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
void VPCALL idSIMD_Generic::Dot( float &dot, const float *src1, const float *src2, const int count ) {
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
void VPCALL idSIMD_Generic::CmpGT( byte *dst, const float *src0, const float constant, const int count ) {
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
void VPCALL idSIMD_Generic::CmpGT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
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
void VPCALL idSIMD_Generic::CmpGE( byte *dst, const float *src0, const float constant, const int count ) {
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
void VPCALL idSIMD_Generic::CmpGE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
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
void VPCALL idSIMD_Generic::CmpLT( byte *dst, const float *src0, const float constant, const int count ) {
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
void VPCALL idSIMD_Generic::SetCmpLT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
#define OPER(X) dst[(X)] = ( src0[(X)] < constant ) << bitNum;
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::CmpLT

  dst[i] |= ( src0[i] < constant ) << bitNum;
============
*/
void VPCALL idSIMD_Generic::CmpLT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
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
void VPCALL idSIMD_Generic::CmpLE( byte *dst, const float *src0, const float constant, const int count ) {
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
void VPCALL idSIMD_Generic::CmpLE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
#define OPER(X) dst[(X)] |= ( src0[(X)] <= constant ) << bitNum;
	UNROLL4(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MinMax
============
*/
void VPCALL idSIMD_Generic::MinMax( float &min, float &max, const float *src, const int count ) {
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
void VPCALL idSIMD_Generic::MinMax( idVec2 &min, idVec2 &max, const idVec2 *src, const int count ) {
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
void VPCALL idSIMD_Generic::MinMax( idVec3 &min, idVec3 &max, const idVec3 *src, const int count ) {
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
void VPCALL idSIMD_Generic::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int count ) {
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
void VPCALL idSIMD_Generic::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const vertIndex_t *indexes, const int count ) {
	min[0] = min[1] = min[2] = idMath::INFINITY; max[0] = max[1] = max[2] = -idMath::INFINITY;
#define OPER(X) const idVec3 &v = src[indexes[(X)]].xyz; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MinMax
============
*/
void VPCALL idSIMD_Generic::MinMax( idVec3 &min,		idVec3 &max,			const struct shadowCache_s *src,	const int count ) {
	min[0] = min[1] = min[2] = idMath::INFINITY; max[0] = max[1] = max[2] = -idMath::INFINITY;
#define OPER(X) const idVec3 &v = src[(X)].xyz.ToVec3(); if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	UNROLL1(OPER)
#undef OPER
}


/*
============
idSIMD_Generic::Clamp
============
*/
void VPCALL idSIMD_Generic::Clamp( float *dst, const float *src, const float min, const float max, const int count ) {
#define OPER(X) dst[(X)] = src[(X)] < min ? min : src[(X)] > max ? max : src[(X)];
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::ClampMin
============
*/
void VPCALL idSIMD_Generic::ClampMin( float *dst, const float *src, const float min, const int count ) {
#define OPER(X) dst[(X)] = src[(X)] < min ? min : src[(X)];
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::ClampMax
============
*/
void VPCALL idSIMD_Generic::ClampMax( float *dst, const float *src, const float max, const int count ) {
#define OPER(X) dst[(X)] = src[(X)] > max ? max : src[(X)];
	UNROLL1(OPER)
#undef OPER
}

/*
================
idSIMD_Generic::Memcpy
================
*/
void VPCALL idSIMD_Generic::Memcpy( void *dst, const void *src, const int count ) {
	memcpy( dst, src, count );
}

/*
================
idSIMD_Generic::Memset
================
*/
void VPCALL idSIMD_Generic::Memset( void *dst, const int val, const int count ) {
	memset( dst, val, count );
}

/*
============
idSIMD_Generic::Zero16
============
*/
void VPCALL idSIMD_Generic::Zero16( float *dst, const int count ) {
	memset( dst, 0, count * sizeof( float ) );
}

/*
============
idSIMD_Generic::Negate16
============
*/
void VPCALL idSIMD_Generic::Negate16( float *dst, const int count ) {
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
void VPCALL idSIMD_Generic::Copy16( float *dst, const float *src, const int count ) {
#define OPER(X) dst[(X)] = src[(X)]
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Add16
============
*/
void VPCALL idSIMD_Generic::Add16( float *dst, const float *src1, const float *src2, const int count ) {
#define OPER(X) dst[(X)] = src1[(X)] + src2[(X)]
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Sub16
============
*/
void VPCALL idSIMD_Generic::Sub16( float *dst, const float *src1, const float *src2, const int count ) {
#define OPER(X) dst[(X)] = src1[(X)] - src2[(X)]
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::Mul16
============
*/
void VPCALL idSIMD_Generic::Mul16( float *dst, const float *src1, const float constant, const int count ) {
#define OPER(X) dst[(X)] = src1[(X)] * constant
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::AddAssign16
============
*/
void VPCALL idSIMD_Generic::AddAssign16( float *dst, const float *src, const int count ) {
#define OPER(X) dst[(X)] += src[(X)]
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::SubAssign16
============
*/
void VPCALL idSIMD_Generic::SubAssign16( float *dst, const float *src, const int count ) {
#define OPER(X) dst[(X)] -= src[(X)]
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MulAssign16
============
*/
void VPCALL idSIMD_Generic::MulAssign16( float *dst, const float constant, const int count ) {
#define OPER(X) dst[(X)] *= constant
	UNROLL1(OPER)
#undef OPER
}

/*
============
idSIMD_Generic::MatX_MultiplyVecX
============
*/
void VPCALL idSIMD_Generic::MatX_MultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
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
void VPCALL idSIMD_Generic::MatX_MultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
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
void VPCALL idSIMD_Generic::MatX_MultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
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
void VPCALL idSIMD_Generic::MatX_TransposeMultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
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
void VPCALL idSIMD_Generic::MatX_TransposeMultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
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
void VPCALL idSIMD_Generic::MatX_TransposeMultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
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
void VPCALL idSIMD_Generic::MatX_MultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) {
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
void VPCALL idSIMD_Generic::MatX_TransposeMultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) {
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
void VPCALL idSIMD_Generic::MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip ) {
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
void VPCALL idSIMD_Generic::MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n ) {
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
idSIMD_Generic::MatX_UpperTriangularSolve

  solves x in Ux = b for the n * n sub-matrix of U
  U has to be an upper triangular matrix with (implicit) ones on the diagonal
  x == b is allowed
============
*/
void VPCALL idSIMD_Generic::MatX_UpperTriangularSolve( const idMatX &U, float *x, const float *b, const int n ) {
#if 1
	int nc;
	const float *uptr;

	uptr = U.ToFloatPtr();
	nc = U.GetNumColumns();

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
				x[0] = b[0] - uptr[0*nc+1] * x[1];
				return;
			case 3:
				x[2] = b[2];
				x[1] = b[1] - uptr[1*nc+2] * x[2];
				x[0] = b[0] - uptr[0*nc+2] * x[2] - uptr[0*nc+1] * x[1];
				return;
			case 4:
				x[3] = b[3];
				x[2] = b[2] - uptr[2*nc+3] * x[3];
				x[1] = b[1] - uptr[1*nc+3] * x[3] - uptr[1*nc+2] * x[2];
				x[0] = b[0] - uptr[0*nc+3] * x[3] - uptr[0*nc+2] * x[2] - uptr[0*nc+1] * x[1];
				return;
			case 5:
				x[4] = b[4];
				x[3] = b[3] - uptr[3*nc+4] * x[4];
				x[2] = b[2] - uptr[2*nc+4] * x[4] - uptr[2*nc+3] * x[3];
				x[1] = b[1] - uptr[1*nc+4] * x[4] - uptr[1*nc+3] * x[3] - uptr[1*nc+2] * x[2];
				x[0] = b[0] - uptr[0*nc+4] * x[4] - uptr[0*nc+3] * x[3] - uptr[0*nc+2] * x[2] - uptr[0*nc+1] * x[1];
				return;
			case 6:
				x[5] = b[5];
				x[4] = b[4] - uptr[4*nc+5] * x[5];
				x[3] = b[3] - uptr[3*nc+5] * x[5] - uptr[3*nc+4] * x[4];
				x[2] = b[2] - uptr[2*nc+5] * x[5] - uptr[2*nc+4] * x[4] - uptr[2*nc+3] * x[3];
				x[1] = b[1] - uptr[1*nc+5] * x[5] - uptr[1*nc+4] * x[4] - uptr[1*nc+3] * x[3] - uptr[1*nc+2] * x[2];
				x[0] = b[0] - uptr[0*nc+5] * x[5] - uptr[0*nc+4] * x[4] - uptr[0*nc+3] * x[3] - uptr[0*nc+2] * x[2] - uptr[0*nc+1] * x[1];
				return;
			case 7:
				x[6] = b[6];
				x[5] = b[5] - uptr[5*nc+6] * x[6];
				x[4] = b[4] - uptr[4*nc+6] * x[6] - uptr[4*nc+5] * x[5];
				x[3] = b[3] - uptr[3*nc+6] * x[6] - uptr[3*nc+5] * x[5] - uptr[3*nc+4] * x[4];
				x[2] = b[2] - uptr[2*nc+6] * x[6] - uptr[2*nc+5] * x[5] - uptr[2*nc+4] * x[4] - uptr[2*nc+3] * x[3];
				x[1] = b[1] - uptr[1*nc+6] * x[6] - uptr[1*nc+5] * x[5] - uptr[1*nc+4] * x[4] - uptr[1*nc+3] * x[3] - uptr[1*nc+2] * x[2];
				x[0] = b[0] - uptr[0*nc+6] * x[6] - uptr[0*nc+5] * x[5] - uptr[0*nc+4] * x[4] - uptr[0*nc+3] * x[3] - uptr[0*nc+2] * x[2] - uptr[0*nc+1] * x[1];
				return;
		}
		return;
	}

	int i, j;
	register double s0, s1, s2, s3;

	// process the last four rows
	x[n-1] = b[n-1];
	x[n-2] = b[n-2] - uptr[(n-2)*nc+(n-1)] * x[n-1];
	x[n-3] = b[n-3] - uptr[(n-3)*nc+(n-1)] * x[n-1] - uptr[(n-3)*nc+(n-2)] * x[n-2];
	x[n-4] = b[n-4] - uptr[(n-4)*nc+(n-1)] * x[n-1] - uptr[(n-4)*nc+(n-2)] * x[n-2] - uptr[(n-4)*nc+(n-3)] * x[n-3];

	uptr = U[n - 5];

	for ( i = n - 5; i >= 0; i-- ) {
		s0 = uptr[i+1] * x[i+1];
		s1 = uptr[i+2] * x[i+2];
		s2 = uptr[i+3] * x[i+3];
		s3 = uptr[i+4] * x[i+4];
		for ( j = i + 5; j < n-7; j += 8 ) {
			s0 += uptr[j+0] * x[j+0];
			s1 += uptr[j+1] * x[j+1];
			s2 += uptr[j+2] * x[j+2];
			s3 += uptr[j+3] * x[j+3];
			s0 += uptr[j+4] * x[j+4];
			s1 += uptr[j+5] * x[j+5];
			s2 += uptr[j+6] * x[j+6];
			s3 += uptr[j+7] * x[j+7];
		}
		switch( n - j ) {
			NODEFAULT;
			case 7: s0 += uptr[j+6] * x[j+6];
			case 6: s1 += uptr[j+5] * x[j+5];
			case 5: s2 += uptr[j+4] * x[j+4];
			case 4: s3 += uptr[j+3] * x[j+3];
			case 3: s0 += uptr[j+2] * x[j+2];
			case 2: s1 += uptr[j+1] * x[j+1];
			case 1: s2 += uptr[j+0] * x[j+0];
			case 0: break;
		}
		double sum;
		sum = s3;
		sum += s2;
		sum += s1;
		sum += s0;
		sum -= b[i];
		x[i] = -sum;
		uptr -= nc;
	}

#else

	int i, j;
	const float *ptr;
	double sum;

	for ( i = n - 1; i >= 0; i-- ) {
		sum = b[i];
		ptr = U[i];
		for ( j = i + 1; j < n; j++ ) {
			sum -= ptr[j] * x[j];
		}
		x[i] = sum;
	}

#endif
}

/*
============
idSIMD_Generic::MatX_UpperTriangularSolveTranspose

  solves x in U'x = b for the n * n sub-matrix of U
  U has to be an upper triangular matrix with (implicit) ones on the diagonal
  x == b is allowed
============
*/
void VPCALL idSIMD_Generic::MatX_UpperTriangularSolveTranspose( const idMatX &U, float *x, const float *b, const int n ) {
#if 1

	int nc;
	const float *uptr;

	uptr = U.ToFloatPtr();
	nc = U.GetNumColumns();

	// unrolled cases for n < 8
	if ( n < 8 ) {
		switch( n ) {
			case 0:
				return;
			case 1:
				x[0] = b[0];
				return;
			case 2:
				x[0] = b[0];
				x[1] = b[1] - uptr[0*nc+1] * x[0];
				return;
			case 3:
				x[0] = b[0];
				x[1] = b[1] - uptr[0*nc+1] * x[0];
				x[2] = b[2] - uptr[0*nc+2] * x[0] - uptr[1*nc+2] * x[1];
				return;
			case 4:
				x[0] = b[0];
				x[1] = b[1] - uptr[0*nc+1] * x[0];
				x[2] = b[2] - uptr[0*nc+2] * x[0] - uptr[1*nc+2] * x[1];
				x[3] = b[3] - uptr[0*nc+3] * x[0] - uptr[1*nc+3] * x[1] - uptr[2*nc+3] * x[2];
				return;
			case 5:
				x[0] = b[0];
				x[1] = b[1] - uptr[0*nc+1] * x[0];
				x[2] = b[2] - uptr[0*nc+2] * x[0] - uptr[1*nc+2] * x[1];
				x[3] = b[3] - uptr[0*nc+3] * x[0] - uptr[1*nc+3] * x[1] - uptr[2*nc+3] * x[2];
				x[4] = b[4] - uptr[0*nc+4] * x[0] - uptr[1*nc+4] * x[1] - uptr[2*nc+4] * x[2] - uptr[3*nc+4] * x[3];
				return;
			case 6:
				x[0] = b[0];
				x[1] = b[1] - uptr[0*nc+1] * x[0];
				x[2] = b[2] - uptr[0*nc+2] * x[0] - uptr[1*nc+2] * x[1];
				x[3] = b[3] - uptr[0*nc+3] * x[0] - uptr[1*nc+3] * x[1] - uptr[2*nc+3] * x[2];
				x[4] = b[4] - uptr[0*nc+4] * x[0] - uptr[1*nc+4] * x[1] - uptr[2*nc+4] * x[2] - uptr[3*nc+4] * x[3];
				x[5] = b[5] - uptr[0*nc+5] * x[0] - uptr[1*nc+5] * x[1] - uptr[2*nc+5] * x[2] - uptr[3*nc+5] * x[3] - uptr[4*nc+5] * x[4];
				return;
			case 7:
				x[0] = b[0];
				x[1] = b[1] - uptr[0*nc+1] * x[0];
				x[2] = b[2] - uptr[0*nc+2] * x[0] - uptr[1*nc+2] * x[1];
				x[3] = b[3] - uptr[0*nc+3] * x[0] - uptr[1*nc+3] * x[1] - uptr[2*nc+3] * x[2];
				x[4] = b[4] - uptr[0*nc+4] * x[0] - uptr[1*nc+4] * x[1] - uptr[2*nc+4] * x[2] - uptr[3*nc+4] * x[3];
				x[5] = b[5] - uptr[0*nc+5] * x[0] - uptr[1*nc+5] * x[1] - uptr[2*nc+5] * x[2] - uptr[3*nc+5] * x[3] - uptr[4*nc+5] * x[4];
				x[6] = b[6] - uptr[0*nc+6] * x[0] - uptr[1*nc+6] * x[1] - uptr[2*nc+6] * x[2] - uptr[3*nc+6] * x[3] - uptr[4*nc+6] * x[4] - uptr[5*nc+6] * x[5];
				return;
		}
		return;
	}

	int i, j;
	register double s0, s1, s2, s3;

	uptr = U.ToFloatPtr();

	// process 4 columns at a time
	for ( i = 0; i < n - 3; i += 4 ) {
		s0 = b[i+0];
		s1 = b[i+1];
		s2 = b[i+2];
		s3 = b[i+3];
		// process 4x4 blocks
		for ( j = 0; j < i-3; j += 4 ) {
			s0 -= uptr[(j+0)*nc+0] * x[j+0];
			s1 -= uptr[(j+0)*nc+1] * x[j+0];
			s2 -= uptr[(j+0)*nc+2] * x[j+0];
			s3 -= uptr[(j+0)*nc+3] * x[j+0];
			s0 -= uptr[(j+1)*nc+0] * x[j+1];
			s1 -= uptr[(j+1)*nc+1] * x[j+1];
			s2 -= uptr[(j+1)*nc+2] * x[j+1];
			s3 -= uptr[(j+1)*nc+3] * x[j+1];
			s0 -= uptr[(j+2)*nc+0] * x[j+2];
			s1 -= uptr[(j+2)*nc+1] * x[j+2];
			s2 -= uptr[(j+2)*nc+2] * x[j+2];
			s3 -= uptr[(j+2)*nc+3] * x[j+2];
			s0 -= uptr[(j+3)*nc+0] * x[j+3];
			s1 -= uptr[(j+3)*nc+1] * x[j+3];
			s2 -= uptr[(j+3)*nc+2] * x[j+3];
			s3 -= uptr[(j+3)*nc+3] * x[j+3];
		}
		// process left over of the 4 columns
		s1 -= uptr[(j+0)*nc+1] * s0;
		s2 -= uptr[(j+0)*nc+2] * s0;
		s2 -= uptr[(j+1)*nc+2] * s1;
		s3 -= uptr[(j+0)*nc+3] * s0;
		s3 -= uptr[(j+1)*nc+3] * s1;
		s3 -= uptr[(j+2)*nc+3] * s2;
		// store result
		x[i+0] = s0;
		x[i+1] = s1;
		x[i+2] = s2;
		x[i+3] = s3;
		// update pointer for next four columns
		uptr += 4;
	}

	// process left over columns
	for ( ; i < n; i++ ) {
		s0 = b[i];
		uptr = U[0] + i;
		for ( j = 0; j < i; j++ ) {
			s0 -= uptr[j*nc] * x[j];
		}
		x[i] = s0;
	}

#else

	int i, j, nc;
	const float *uptr;
	double sum;

	nc = U.GetNumColumns();
	for ( i = 0; i < n; i++ ) {
		sum = b[i];
		uptr = U.ToFloatPtr() + i;
		for ( j = 0; j < i; j++ ) {
			sum -= uptr[j*nc] * x[j];
		}
		x[i] = sum;
	}

#endif
}

/*
============
idSIMD_Generic::MatX_LU_Factor

  in-place factorization LU of the n * n sub-matrix of mat
  the reciprocal of the diagonal elements of U are stored in invDiag
  no pivoting is used
============
*/
bool VPCALL idSIMD_Generic::MatX_LU_Factor( idMatX &mat, idVecX &invDiag, const int n ) {
#if 1

	int i, j, k;
	float d1, d2, *ptr1, *ptr2;

	for ( i = 0; i < n; i++ ) {

		d1 = mat[i][i];

		if ( d1 == 0.0f ) {
			return false;
		}

		invDiag[i] = d1 = 1.0f / d1;

		ptr1 = mat[i];

		for ( j = i + 1; j < n; j++ ) {

			ptr2 = mat[j];
			ptr2[i] = d2 = ptr2[i] * d1;

			for ( k = i + 1; k < n - 15; k += 16 ) {
				ptr2[k+0] -= d2 * ptr1[k+0];
				ptr2[k+1] -= d2 * ptr1[k+1];
				ptr2[k+2] -= d2 * ptr1[k+2];
				ptr2[k+3] -= d2 * ptr1[k+3];
				ptr2[k+4] -= d2 * ptr1[k+4];
				ptr2[k+5] -= d2 * ptr1[k+5];
				ptr2[k+6] -= d2 * ptr1[k+6];
				ptr2[k+7] -= d2 * ptr1[k+7];
				ptr2[k+8] -= d2 * ptr1[k+8];
				ptr2[k+9] -= d2 * ptr1[k+9];
				ptr2[k+10] -= d2 * ptr1[k+10];
				ptr2[k+11] -= d2 * ptr1[k+11];
				ptr2[k+12] -= d2 * ptr1[k+12];
				ptr2[k+13] -= d2 * ptr1[k+13];
				ptr2[k+14] -= d2 * ptr1[k+14];
				ptr2[k+15] -= d2 * ptr1[k+15];
			}
			switch( n - k ) {
				NODEFAULT;
				case 15: ptr2[k+14] -= d2 * ptr1[k+14];
				case 14: ptr2[k+13] -= d2 * ptr1[k+13];
				case 13: ptr2[k+12] -= d2 * ptr1[k+12];
				case 12: ptr2[k+11] -= d2 * ptr1[k+11];
				case 11: ptr2[k+10] -= d2 * ptr1[k+10];
				case 10: ptr2[k+9] -= d2 * ptr1[k+9];
				case 9: ptr2[k+8] -= d2 * ptr1[k+8];
				case 8: ptr2[k+7] -= d2 * ptr1[k+7];
				case 7: ptr2[k+6] -= d2 * ptr1[k+6];
				case 6: ptr2[k+5] -= d2 * ptr1[k+5];
				case 5: ptr2[k+4] -= d2 * ptr1[k+4];
				case 4: ptr2[k+3] -= d2 * ptr1[k+3];
				case 3: ptr2[k+2] -= d2 * ptr1[k+2];
				case 2: ptr2[k+1] -= d2 * ptr1[k+1];
				case 1: ptr2[k+0] -= d2 * ptr1[k+0];
				case 0: break;
			}
		}
	}

	return true;

#else

	int i, j, k;
	float d;

	for ( i = 0; i < n; i++ ) {

		if ( mat[i][i] == 0.0f ) {
			return false;
		}

		invDiag[i] = d = 1.0f / mat[i][i];

		for ( j = i + 1; j < n; j++ ) {
			mat[j][i] *= d;
		}

		for ( j = i + 1; j < n; j++ ) {
			d = mat[j][i];
			for ( k = i + 1; k < n; k++ ) {
				mat[j][k] -= d * mat[i][k];
			}
		}
	}

	return true;

#endif
}

/*
============
idSIMD_Generic::MatX_LDLT_Factor

  in-place factorization LDL' of the n * n sub-matrix of mat
  the reciprocal of the diagonal elements are stored in invDiag
============
*/
bool VPCALL idSIMD_Generic::MatX_LDLT_Factor( idMatX &mat, idVecX &invDiag, const int n ) {
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
idSIMD_Generic::DecompressJoints
============
*/
void VPCALL idSIMD_Generic::DecompressJoints( idJointQuat *joints, const idCompressedJointQuat *compressedJoints, const int *index, const int numJoints ) {
	for ( int i = 0; i < numJoints; i++ ) {
		int j = index[i];

		joints[j].q = compressedJoints[j].ToQuat();
		joints[j].t = compressedJoints[j].ToOffset();
		joints[j].w = 0.0f;
	}
}

/*
============
SlerpUnoptimized
============
*/
void SlerpUnoptimized( const idQuat &from, const idQuat &to, float t, idQuat &result ) {
	float cosom, absCosom, sinom, omega, scale0, scale1;

	cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
	absCosom = fabs( cosom );
	if ( ( 1.0f - absCosom ) > 1e-6f ) {
		omega = acos( absCosom );
		sinom = 1.0f / sin( omega );
		scale0 = sin( ( 1.0f - t ) * omega ) * sinom;
		scale1 = sin( t * omega ) * sinom;
	} else {
		scale0 = 1.0f - t;
		scale1 = t;
	}
	scale1 = ( cosom >= 0.0f ) ? scale1 : -scale1;
	result.x = scale0 * from.x + scale1 * to.x;
	result.y = scale0 * from.y + scale1 * to.y;
	result.z = scale0 * from.z + scale1 * to.z;
	result.w = scale0 * from.w + scale1 * to.w;
}

/*
============
SlerpOptimized
============
*/
void SlerpOptimized( const idQuat &from, const idQuat &to, float t, idQuat &result ) {
	float cosom, absCosom, sinom, sinSqr, omega, scale0, scale1;

	cosom =	from.x * to.x +	from.y * to.y +	from.z * to.z +	from.w * to.w;
	absCosom = fabs( cosom );
	if ( ( 1.0f - absCosom ) > 1e-6f ) {
		sinSqr = 1.0f -	cosom * cosom;

		//sinom = 1.0f / sqrt( sinSqr );
		{
			long i;
			float y, r;

			y = sinSqr * 0.5f;
			i = *reinterpret_cast<long *>( &sinSqr );
			i = 0x5f3759df - ( i >> 1 );
			r = *reinterpret_cast<float *>( &i );
			sinom = r * ( 1.5f - r * r * y );
		}

		//omega = atan2( sinSqr * sinom, absCosom );
		{
			float y, a, d, s;
			y = sinSqr * sinom;
			if ( y > absCosom ) {
				a = -absCosom / y;
				d = idMath::HALF_PI;
			} else {
				a = y / absCosom;
				d = 0.0f;
			}
			s = a * a;
			omega = ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
						* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a + d;
		}

		// scale0 = sin( ( 1.0f - t ) * omega ) * sinom;
		{
			float a = ( 1.0f - t ) * omega;
			float s = a * a;
			scale0 = sinom * a * ( ( ( ( ( -2.39e-08f * s + 2.7526e-06f ) * s - 1.98409e-04f ) * s + 8.3333315e-03f ) * s - 1.666666664e-01f ) * s + 1.0f );
		}

		// scale1 = sin( t * omega ) * sinom;
		{
			float a = t * omega;
			float s = a * a;
			scale1 = sinom * a * ( ( ( ( ( -2.39e-08f * s + 2.7526e-06f ) * s - 1.98409e-04f ) * s + 8.3333315e-03f ) * s - 1.666666664e-01f ) * s + 1.0f );
		}
	} else {
		scale0 = 1.0f -	t;
		scale1 = t;
	}
	scale1 = ( cosom >=	0.0f ) ? scale1	: -scale1;
	result.x = scale0 *	from.x + scale1	* to.x;
	result.y = scale0 *	from.y + scale1	* to.y;
	result.z = scale0 *	from.z + scale1	* to.z;
	result.w = scale0 *	from.w + scale1	* to.w;
}

/*
============
LerpUnoptimized
============
*/
void LerpUnoptimized( const idQuat &from, const idQuat &to, float t, idQuat &result ) {
	float cosom, scale0, scale1, s;

	cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;

	scale0 = 1.0f - t;
	scale1 = ( cosom >= 0.0f ) ? t : -t;

	result.x = scale0 * from.x + scale1 * to.x;
	result.y = scale0 * from.y + scale1 * to.y;
	result.z = scale0 * from.z + scale1 * to.z;
	result.w = scale0 * from.w + scale1 * to.w;

	s = 1.0f / sqrt( result.x * result.x + result.y * result.y + result.z * result.z + result.w * result.w );

	result.x *= s;
	result.y *= s;
	result.z *= s;
	result.w *= s;
}

/*
============
idSIMD_Generic::BlendJoints
============
*/
void VPCALL idSIMD_Generic::BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) {
	int i;

	for ( i = 0; i < numJoints; i++ ) {
		int j = index[i];
		joints[j].q.Slerp( joints[j].q, blendJoints[j].q, lerp );
		joints[j].t.Lerp( joints[j].t, blendJoints[j].t, lerp );
	}
}

/*
============
idSIMD_Generic::BlendJointsFast
============
*/
void VPCALL idSIMD_Generic::BlendJointsFast( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) {
	int i;

	for ( i = 0; i < numJoints; i++ ) {
		int j = index[i];
		joints[j].q.SlerpFast( joints[j].q, blendJoints[j].q, lerp );
		joints[j].t.Lerp( joints[j].t, blendJoints[j].t, lerp );
	}
}

/*
============
idSIMD_Generic::ConvertJointQuatsToJointMats
============
*/
void VPCALL idSIMD_Generic::ConvertJointQuatsToJointMats( idJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints ) {
#if 1

	compile_time_assert( (UINT_PTR)(&((idJointQuat *)0)->t) == (UINT_PTR)(&((idJointQuat *)0)->q) + (UINT_PTR)sizeof( ((idJointQuat *)0)->q ) );

	for ( int i = 0; i < numJoints; i++ ) {

		const float *q = jointQuats[i].q.ToFloatPtr();
		float *m = jointMats[i].ToFloatPtr();

		m[0*4+3] = q[4];
		m[1*4+3] = q[5];
		m[2*4+3] = q[6];

		float x2 = q[0] + q[0];
		float y2 = q[1] + q[1];
		float z2 = q[2] + q[2];

		{
			float xx2 = q[0] * x2;
			float yy2 = q[1] * y2;
			float zz2 = q[2] * z2;

			m[0*4+0] = 1.0f - yy2 - zz2;
			m[1*4+1] = 1.0f - xx2 - zz2;
			m[2*4+2] = 1.0f - xx2 - yy2;
		}

		{
			float yz2 = q[1] * z2;
			float wx2 = q[3] * x2;

			m[2*4+1] = yz2 - wx2;
			m[1*4+2] = yz2 + wx2;
		}

		{
			float xy2 = q[0] * y2;
			float wz2 = q[3] * z2;

			m[1*4+0] = xy2 - wz2;
			m[0*4+1] = xy2 + wz2;
		}

		{
			float xz2 = q[0] * z2;
			float wy2 = q[3] * y2;

			m[0*4+2] = xz2 - wy2;
			m[2*4+0] = xz2 + wy2;
		}
	}

#else

	int i;

	for ( i = 0; i < numJoints; i++ ) {
		jointMats[i].SetRotation( jointQuats[i].q.ToMat3() );
		jointMats[i].SetTranslation( jointQuats[i].t );
	}

#endif
}

/*
============
idSIMD_Generic::ConvertJointMatsToJointQuats
============
*/
void VPCALL idSIMD_Generic::ConvertJointMatsToJointQuats( idJointQuat *jointQuats, const idJointMat *jointMats, const int numJoints ) {
#if 1

	compile_time_assert( (UINT_PTR)(&((idJointQuat *)0)->t) == (UINT_PTR)(&((idJointQuat *)0)->q) + (UINT_PTR)sizeof( ((idJointQuat *)0)->q ) );

	for ( int i = 0; i < numJoints; i++ ) {

		float *q = jointQuats[i].q.ToFloatPtr();
		const float *m = jointMats[i].ToFloatPtr();

		if ( m[0 * 4 + 0] + m[1 * 4 + 1] + m[2 * 4 + 2] > 0.0f ) {

			float t = + m[0 * 4 + 0] + m[1 * 4 + 1] + m[2 * 4 + 2] + 1.0f;
			float s = idMath::InvSqrt( t ) * 0.5f;

			q[3] = s * t;
			q[2] = ( m[0 * 4 + 1] - m[1 * 4 + 0] ) * s;
			q[1] = ( m[2 * 4 + 0] - m[0 * 4 + 2] ) * s;
			q[0] = ( m[1 * 4 + 2] - m[2 * 4 + 1] ) * s;

		} else if ( m[0 * 4 + 0] > m[1 * 4 + 1] && m[0 * 4 + 0] > m[2 * 4 + 2] ) {

			float t = + m[0 * 4 + 0] - m[1 * 4 + 1] - m[2 * 4 + 2] + 1.0f;
			float s = idMath::InvSqrt( t ) * 0.5f;

			q[0] = s * t;
			q[1] = ( m[0 * 4 + 1] + m[1 * 4 + 0] ) * s;
			q[2] = ( m[2 * 4 + 0] + m[0 * 4 + 2] ) * s;
			q[3] = ( m[1 * 4 + 2] - m[2 * 4 + 1] ) * s;

		} else if ( m[1 * 4 + 1] > m[2 * 4 + 2] ) {

			float t = - m[0 * 4 + 0] + m[1 * 4 + 1] - m[2 * 4 + 2] + 1.0f;
			float s = idMath::InvSqrt( t ) * 0.5f;

			q[1] = s * t;
			q[0] = ( m[0 * 4 + 1] + m[1 * 4 + 0] ) * s;
			q[3] = ( m[2 * 4 + 0] - m[0 * 4 + 2] ) * s;
			q[2] = ( m[1 * 4 + 2] + m[2 * 4 + 1] ) * s;

		} else {

			float t = - m[0 * 4 + 0] - m[1 * 4 + 1] + m[2 * 4 + 2] + 1.0f;
			float s = idMath::InvSqrt( t ) * 0.5f;

			q[2] = s * t;
			q[3] = ( m[0 * 4 + 1] - m[1 * 4 + 0] ) * s;
			q[0] = ( m[2 * 4 + 0] + m[0 * 4 + 2] ) * s;
			q[1] = ( m[1 * 4 + 2] + m[2 * 4 + 1] ) * s;

		}

		q[4] = m[0 * 4 + 3];
		q[5] = m[1 * 4 + 3];
		q[6] = m[2 * 4 + 3];
		q[7] = 0.0f;
	}

#else

	int i;

	for ( i = 0; i < numJoints; i++ ) {
		jointQuats[i] = jointMats[i].ToJointQuat();
	}

#endif
}

/*
============
idSIMD_Generic::TransformJoints
============
*/
void VPCALL idSIMD_Generic::TransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) {
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
void VPCALL idSIMD_Generic::UntransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) {
	int i;

	for( i = lastJoint; i >= firstJoint; i-- ) {
		assert( parents[i] < i );
		jointMats[i] /= jointMats[parents[i]];
	}
}

/*
============
idSIMD_Generic::MultiplyJoints
============
*/
void VPCALL idSIMD_Generic::MultiplyJoints( idJointMat *result, const idJointMat *joints1, const idJointMat *joints2, const int numJoints ) {
	int i;

	for ( i = 0; i < numJoints; i++ ) {
		idJointMat::Multiply( result[i], joints1[i], joints2[i] );
	}
}

/*
============
idSIMD_Generic::TransformVerts
============
*/
void VPCALL idSIMD_Generic::TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, int numWeights ) {
	int i, j;
	const byte *jointsPtr = (byte *)joints;

	for( j = 0, i = 0; i < numVerts; i++, j++ ) {
		idVec3 v;

		v = ( *(idJointMat *) ( jointsPtr + weights[j].jointMatOffset ) ) * base[j];
		while( weights[j].nextVertexOffset != JOINTWEIGHT_SIZE ) {
			j++;
			v += ( *(idJointMat *) ( jointsPtr + weights[j].jointMatOffset ) ) * base[j];
		}

		verts[i].xyz = v;
	}
}

/*
============
idSIMD_Generic::TransformShadowVerts
============
*/
void VPCALL idSIMD_Generic::TransformShadowVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const jointWeight_t *weights, const int numWeights ) {
	int i;
	const byte *jointsPtr = (byte *)joints;
	const byte *weightsPtr = (byte *)weights;

	for( i = 0; i < numVerts; i++ ) {
		const idJointMat &mat = *(idJointMat *) ( jointsPtr + ((jointWeight_t *)weightsPtr)->jointMatOffset );

		weightsPtr += ((jointWeight_t *)weightsPtr)->nextVertexOffset;

		mat.Mul( verts[i].xyz, base[i].xyz );
	}
}

/*
============
idSIMD_Generic::TransformShadowVerts
============
*/
void VPCALL idSIMD_Generic::TransformShadowVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const short *weights, const int numWeights ) {
	int i;
	const byte *jointsPtr = (byte *)joints;
	const short *weightsPtr = (short *)weights;

	for( i = 0; i < numVerts; i++ ) {
		const idJointMat &mat = *(idJointMat *) ( jointsPtr + (*weightsPtr));

		weightsPtr++;

		mat.Mul( verts[i].xyz, base[i].xyz );
	}
}
/*
============
idSIMD_Generic::TransformShadowVerts
============
*/
void VPCALL idSIMD_Generic::TransformShadowVerts( shadowCache_t *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const short *weights, const int numWeights ) {
	int i;
	const byte *jointsPtr = (byte *)joints;
	const short *weightsPtr = (short *)weights;

	for( i = 0; i < numVerts; i++ ) {
		const idJointMat &mat = *(idJointMat *) ( jointsPtr + (*weightsPtr));

		weightsPtr++;

		mat.Mul( verts[i].xyz.ToVec3(), base[i].xyz );
	}
}

/*
============
idSIMD_Generic::TransformVertsAndTangents
============
*/
void VPCALL idSIMD_Generic::TransformVertsAndTangents( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) {
	int i, j;
	const byte *jointsPtr = (byte *)joints;

	for( j = i = 0; i < numVerts; i++, j++ ) {
		idJointMat mat;

		idJointMat::Mul( mat, *(idJointMat *) ( jointsPtr + weights[j].jointMatOffset ), weights[j].weight );

		while( weights[j].nextVertexOffset != JOINTWEIGHT_SIZE ) {
			j++;
			idJointMat::Mad( mat, *(idJointMat *) ( jointsPtr + weights[j].jointMatOffset ), weights[j].weight );
		}

		verts[i].xyz = mat * base[i*3+0];
		verts[i].SetNormal( mat * base[i*3+1] );
		verts[i].SetTangent( mat * base[i*3+2] );
	}
}

/*
============
idSIMD_Generic::TransformVertsAndTangentsFast
============
*/
void VPCALL idSIMD_Generic::TransformVertsAndTangentsFast( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) {
	int i;
	const byte *jointsPtr = (byte *)joints;
	const byte *weightsPtr = (byte *)weights;

	for( i = 0; i < numVerts; i++ ) {
		const idJointMat &mat = *(idJointMat *) ( jointsPtr + ((jointWeight_t *)weightsPtr)->jointMatOffset );

		weightsPtr += ((jointWeight_t *)weightsPtr)->nextVertexOffset;

		verts[i].xyz = mat * base[i*3+0];
		verts[i].SetNormal( mat * base[i*3+1] );
		verts[i].SetTangent( mat * base[i*3+2] );
	}
}

#if SD_SUPPORT_UNSMOOTHEDTANGENTS
/*
============
idSIMD_Generic::DeriveUnsmoothedTangents

	Derives the normal and orthogonal tangent vectors for the triangle vertices.
	For each vertex the normal and tangent vectors are derived from a single dominant triangle.
============
*/
#define DERIVE_UNSMOOTHED_BITANGENT

void VPCALL idSIMD_Generic::DeriveUnsmoothedTangents( idDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts ) {
	int i;

	for ( i = 0; i < numVerts; i++ ) {
		idDrawVert *a, *b, *c;
#if defined( SD_USE_DRAWVERT_SIZE_32 )
		idVec2 aST, bST, cST;
#endif
		float d0, d1, d2, d3, d4;
		float d5, d6, d7, d8, d9;
		float s0, s1, s2;
		float n0, n1, n2;
		float t0, t1, t2;
		float t3, t4, t5;

		const dominantTri_s &dt = dominantTris[i];

		a = verts + i;
		b = verts + dt.v2;
		c = verts + dt.v3;

#if defined( SD_USE_DRAWVERT_SIZE_32 )
		aST = a->GetST();
		bST = b->GetST();
		cST = c->GetST();
#endif

		d0 = b->xyz[0] - a->xyz[0];
		d1 = b->xyz[1] - a->xyz[1];
		d2 = b->xyz[2] - a->xyz[2];
#if defined( SD_USE_DRAWVERT_SIZE_32 )
		d3 = bST[0] - aST[0];
		d4 = bST[1] - aST[1];
#else
		d3 = b->_st[0] - a->_st[0];
		d4 = b->_st[1] - a->_st[1];
#endif

		d5 = c->xyz[0] - a->xyz[0];
		d6 = c->xyz[1] - a->xyz[1];
		d7 = c->xyz[2] - a->xyz[2];
#if defined( SD_USE_DRAWVERT_SIZE_32 )
		d8 = cST[0] - aST[0];
		d9 = cST[1] - aST[1];
#else
		d8 = c->_st[0] - a->_st[0];
		d9 = c->_st[1] - a->_st[1];
#endif

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

		a->SetNormal( n0, n1, n2 );

		a->SetTangent( t0, t1, t2 );

		a->SetBiTangent( t3, t4, t5 );
	}
}
#endif // SD_SUPPORT_UNSMOOTHEDTANGENTS

/*
============
idSIMD_Generic::TracePointCull
============
*/
void VPCALL idSIMD_Generic::TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts ) {
	int i;
	byte tOr;

	tOr = 0;

	for ( i = 0; i < numVerts; i++ ) {
		byte bits;
		float d0, d1, d2, d3, t;
		const idVec3 &v = verts[i].xyz;

		d0 = planes[0].Distance( v );
		d1 = planes[1].Distance( v );
		d2 = planes[2].Distance( v );
		d3 = planes[3].Distance( v );

		t = d0 + radius;
		bits  = FLOATSIGNBITSET( t ) << 0;
		t = d1 + radius;
		bits |= FLOATSIGNBITSET( t ) << 1;
		t = d2 + radius;
		bits |= FLOATSIGNBITSET( t ) << 2;
		t = d3 + radius;
		bits |= FLOATSIGNBITSET( t ) << 3;

		t = d0 - radius;
		bits |= FLOATSIGNBITSET( t ) << 4;
		t = d1 - radius;
		bits |= FLOATSIGNBITSET( t ) << 5;
		t = d2 - radius;
		bits |= FLOATSIGNBITSET( t ) << 6;
		t = d3 - radius;
		bits |= FLOATSIGNBITSET( t ) << 7;

		bits ^= 0x0F;		// flip lower four bits

		tOr |= bits;
		cullBits[i] = bits;
	}

	totalOr = tOr;
}

/*
============
idSIMD_Generic::TracePointCull
============
*/
void VPCALL idSIMD_Generic::TracePointCullShadowVerts( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const shadowCache_t *verts, const int numVerts ) {
	int i;
	byte tOr;

	tOr = 0;

	for ( i = 0; i < numVerts; i++ ) {
		byte bits;
		float d0, d1, d2, d3, t;
		const idVec3 &v = verts[i].xyz.ToVec3();

		d0 = planes[0].Distance( v );
		d1 = planes[1].Distance( v );
		d2 = planes[2].Distance( v );
		d3 = planes[3].Distance( v );

		t = d0 + radius;
		bits  = FLOATSIGNBITSET( t ) << 0;
		t = d1 + radius;
		bits |= FLOATSIGNBITSET( t ) << 1;
		t = d2 + radius;
		bits |= FLOATSIGNBITSET( t ) << 2;
		t = d3 + radius;
		bits |= FLOATSIGNBITSET( t ) << 3;

		t = d0 - radius;
		bits |= FLOATSIGNBITSET( t ) << 4;
		t = d1 - radius;
		bits |= FLOATSIGNBITSET( t ) << 5;
		t = d2 - radius;
		bits |= FLOATSIGNBITSET( t ) << 6;
		t = d3 - radius;
		bits |= FLOATSIGNBITSET( t ) << 7;

		bits ^= 0x0F;		// flip lower four bits

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
void VPCALL idSIMD_Generic::DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts ) {
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

void VPCALL idSIMD_Generic::DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts, int *indexes, int numIndexes ) {
	int i;

	for ( i = 0; i < numIndexes; i++ ) {
		byte bits;
		float d0, d1, d2, d3, d4, d5;
		int idx = indexes[i];
		const idVec3 &v = verts[idx].xyz;

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

		cullBits[idx] = bits ^ 0x3F;		// flip lower 6 bits
	}
}


void VPCALL idSIMD_Generic::DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts, unsigned short *indexes, int numIndexes ) {
	int i;

	for ( i = 0; i < numIndexes; i++ ) {
		byte bits;
		float d0, d1, d2, d3, d4, d5;
		int idx = indexes[i];
		const idVec3 &v = verts[idx].xyz;

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

		cullBits[idx] = bits ^ 0x3F;		// flip lower 6 bits
	}
}

/*
============
idSIMD_Generic::OverlayPointCull
============
*/
void VPCALL idSIMD_Generic::OverlayPointCull( byte *cullBits, idVec2 *texCoords, const idPlane *planes, const idDrawVert *verts, const int numVerts ) {
	int i;

	for ( i = 0; i < numVerts; i++ ) {
		byte bits;
		float d0, d1;
		const idVec3 &v = verts[i].xyz;

		texCoords[i][0] = d0 = planes[0].Distance( v );
		texCoords[i][1] = d1 = planes[1].Distance( v );

		bits  = FLOATSIGNBITSET( d0 ) << 0;
		bits |= FLOATSIGNBITSET( d1 ) << 1;

		d0 = 1.0f - d0;
		d1 = 1.0f - d1;

		bits |= FLOATSIGNBITSET( d0 ) << 2;
		bits |= FLOATSIGNBITSET( d1 ) << 3;

		cullBits[i] = bits;
	}
}

/*
============
idSIMD_Generic::OverlayPointCull
============
*/
void VPCALL idSIMD_Generic::OverlayPointCull( byte *cullBits, idVec2 *texCoords, const idPlane *planes, const shadowCache_t *verts, const int numVerts ) {
	int i;

	for ( i = 0; i < numVerts; i++ ) {
		byte bits;
		float d0, d1;
		const idVec3 &v = verts[i].xyz.ToVec3();

		texCoords[i][0] = d0 = planes[0].Distance( v );
		texCoords[i][1] = d1 = planes[1].Distance( v );

		bits  = FLOATSIGNBITSET( d0 ) << 0;
		bits |= FLOATSIGNBITSET( d1 ) << 1;

		d0 = 1.0f - d0;
		d1 = 1.0f - d1;

		bits |= FLOATSIGNBITSET( d0 ) << 2;
		bits |= FLOATSIGNBITSET( d1 ) << 3;

		cullBits[i] = bits;
	}
}

/*
============
idSIMD_Generic::DeriveTriPlanes

	Derives a plane equation for each triangle.
============
*/
void VPCALL idSIMD_Generic::DeriveTriPlanes( idPlane *planes, const idDrawVert *verts, const int numVerts, const vertIndex_t *indexes, const int numIndexes ) {
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

#if defined( OPTIMIZED_TRI_PLANE_CODE )
		f = idMath::RSqrt( n.x * n.x + n.y * n.y + n.z * n.z );
#else
		f = 1.0f / sqrt( n.x * n.x + n.y * n.y + n.z * n.z );
#endif

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
idSIMD_Generic::DeriveTriPlanes

	Derives a plane equation for each triangle.
============
*/
void VPCALL idSIMD_Generic::DeriveTriPlanes( idPlane *planes, const shadowCache_t *verts, const int numVerts, const vertIndex_t *indexes, const int numIndexes ) {
	int i;

	for ( i = 0; i < numIndexes; i += 3 ) {
		const shadowCache_t *a, *b, *c;
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

#if defined( OPTIMIZED_TRI_PLANE_CODE )
		f = idMath::RSqrt( n.x * n.x + n.y * n.y + n.z * n.z );
#else
		f = 1.0f / sqrt( n.x * n.x + n.y * n.y + n.z * n.z );
#endif

		n.x *= f;
		n.y *= f;
		n.z *= f;

		planes->SetNormal( n );
		planes->FitThroughPoint( a->xyz.ToVec3() );
		planes++;
	}
}

/*
============
idSIMD_Generic::CalculateFacing
============
*/
void VPCALL idSIMD_Generic::CalculateFacing( byte *facing, const idPlane *planes, const int numTriangles, const idVec4 &light ) {
	int	i;

	for	( i	= 0; i < numTriangles; i++ ) {
		facing[i] =	planes[i][0] * light.x +
					planes[i][1] * light.y +
					planes[i][2] * light.z +
					planes[i][3] * light.w > 0.0f;
	}
	facing[numTriangles] = 1;	// for dangling	edges to reference
}

/*
============
idSIMD_Generic::CalculateCullBits
============
*/
void VPCALL idSIMD_Generic::CalculateCullBits( byte *cullBits, const idDrawVert *verts, const int numVerts, const int frontBits, const idPlane lightPlanes[NUM_LIGHT_PLANES] ) {
	int i, j;

	assert( NUM_LIGHT_PLANES <= sizeof( cullBits[0] ) * 8 );

	memset( cullBits, 0, numVerts * sizeof( cullBits[0] ) );

	for ( i = 0; i < NUM_LIGHT_PLANES; i++ ) {
		// if completely infront of this clipping plane
		if ( frontBits & ( 1 << i ) ) {
			continue;
		}
		const idPlane &plane = lightPlanes[i];
		for ( j = 0; j < numVerts; j++ ) {
			int bit = plane[0] * verts[j].xyz.x +
						plane[1] * verts[j].xyz.y +
							plane[2] * verts[j].xyz.z +
								plane[3] < 0.0f;
			cullBits[j] |= bit << i;
		}
	}
}


/*
============
idSIMD_Generic::CreateShadowCache
============
*/
int VPCALL idSIMD_Generic::CreateShadowCache( idVec4 *vertexCache, const idDrawVert *verts, const int numVerts ) {
	for ( int i = 0; i < numVerts; i++ ) {
		const float *v = verts[i].xyz.ToFloatPtr();
		vertexCache[i*2+0][0] = v[0];
		vertexCache[i*2+1][0] = v[0];
		vertexCache[i*2+0][1] = v[1];
		vertexCache[i*2+1][1] = v[1];
		vertexCache[i*2+0][2] = v[2];
		vertexCache[i*2+1][2] = v[2];
		vertexCache[i*2+0][3] = 1.0f;
		vertexCache[i*2+1][3] = 0.0f;
	}
	return numVerts * 2;
}

/*
============
idSIMD_Generic::CreateShadowCache
============
*/
int  VPCALL idSIMD_Generic::CreateShadowCache( idVec4 *vertexCache, const struct shadowCache_s *verts, const int numVerts ) {
	for ( int i = 0; i < numVerts; i++ ) {
		const float *v = verts[i].xyz.ToFloatPtr();
		vertexCache[i*2+0][0] = v[0];
		vertexCache[i*2+1][0] = v[0];
		vertexCache[i*2+0][1] = v[1];
		vertexCache[i*2+1][1] = v[1];
		vertexCache[i*2+0][2] = v[2];
		vertexCache[i*2+1][2] = v[2];
		vertexCache[i*2+0][3] = 1.0f;
		vertexCache[i*2+1][3] = 0.0f;
	}
	return numVerts * 2;
}


/*
============
idSIMD_Generic::ShadowVolume_CountFacing
============
*/
int VPCALL idSIMD_Generic::ShadowVolume_CountFacing( const byte *facing, const int numFaces ) {
#if defined( OPTIMIZED_SHADOW_VOLUME_CODE )

	int i, n;

	n = 0;
	for ( i = 0; i < numFaces; i++ ) {
		n += facing[i];
	}
	return n;

#else

	int i, n;

	n = 0;
	for ( i = 0; i < numFaces; i++ ) {
		if ( facing[i] ) {
			n++;
		}
	}
	return n;

#endif
}

/*
============
idSIMD_Generic::ShadowVolume_CountFacingCull
============
*/
int VPCALL idSIMD_Generic::ShadowVolume_CountFacingCull( byte *facing, const int numFaces, const vertIndex_t *indexes, const byte *cull ) {
#if defined( OPTIMIZED_SHADOW_VOLUME_CODE )

	int i, n;

	n = 0;
	for ( i = 0; i < numFaces; i++ ) {
		int c = cull[indexes[0]] & cull[indexes[1]] & cull[indexes[2]];
		facing[i] |= ( (-c) >> 31 ) & 1;
		n += facing[i];
		indexes += 3;
	}
	return n;

#else

	int i, n;

	n = 0;
	for ( i = 0; i < numFaces; i++ ) {
		if ( !facing[i] ) {
			int	i1 = indexes[0];
			int	i2 = indexes[1];
			int	i3 = indexes[2];
			if ( cull[i1] & cull[i2] & cull[i3] ) {
				facing[i] = 1;
				n++;
			}
		} else {
			n++;
		}
		indexes += 3;
	}
	return n;

#endif
}

/*
============
idSIMD_Generic::ShadowVolume_CreateSilTriangles
============
*/
int VPCALL idSIMD_Generic::ShadowVolume_CreateSilTriangles( vertIndex_t *shadowIndexes, const byte *facing, const silEdge_t *silEdges, const int numSilEdges ) {
#if defined( OPTIMIZED_SHADOW_VOLUME_CODE )

	int i;
	const silEdge_t *sil;
	vertIndex_t *si;

	si = shadowIndexes;
	for ( sil = silEdges, i = numSilEdges; i > 0; i--, sil++ ) {

		int f1 = facing[sil->p1];
		int f2 = facing[sil->p2];

		if ( !( f1 ^ f2 ) ) {
			continue;
		}

		int v1 = sil->v1;
		int v2 = sil->v2;

		// set the two triangle winding orders based on facing
		// without using a poorly-predictable branch

		si[0] = v1;
		si[1] = v2 ^ f2;
		si[2] = v2 ^ f1;
		si[3] = v1 ^ f2;
		si[4] = v2 ^ 1;
		si[5] = v1 ^ f1;

		si += 6;
	}
	return si - shadowIndexes;

#else

	int i;
	const silEdge_t *sil;
	vertIndex_t *si;

	si = shadowIndexes;
	for ( sil = silEdges, i = numSilEdges; i > 0; i--, sil++ ) {

		byte f1 = facing[sil->p1];
		byte f2 = facing[sil->p2];

		if ( f1 != f2 )	{

			int	v1 = sil->v1;
			int	v2 = sil->v2;

			if ( f1	) {
				si[0] = v1;
				si[1] = v2 + 1;
				si[2] = v2;
				si[3] = v1;
				si[4] = v1 + 1;
				si[5] = v2 + 1;
			} else {
				si[0] = v1;
				si[1] = v2;
				si[2] = v2 + 1;
				si[3] = v1 + 1;
				si[4] = v1;
				si[5] = v2 + 1;
			}
			si += 6;
		}
	}
	return si - shadowIndexes;

#endif
}

/*
============
idSIMD_Generic::ShadowVolume_CreateSilTrianglesParallel
============
*/
int VPCALL idSIMD_Generic::ShadowVolume_CreateSilTrianglesParallel( vertIndex_t *shadowIndexes, const byte *facing, const silEdge_t *silEdges, const int numSilEdges ) {
#if defined( OPTIMIZED_SHADOW_VOLUME_CODE )

	int i;
	const silEdge_t *sil;
	vertIndex_t *si;

	si = shadowIndexes;
	for ( sil = silEdges, i = numSilEdges; i > 0; i--, sil++ ) {

		int f1 = facing[sil->p1];
		int f2 = facing[sil->p2];

		if ( !( f1 ^ f2 ) ) {
			continue;
		}

		int v1 = sil->v1;
		int v2 = sil->v2;

		// set the triangle winding order based on facing
		// without using a poorly-predictable branch

		si[0] = v1;
		si[1] = ( v2 & -f1 ) + ( f1 ^ 1 );
		si[2] = ( v2 & -f2 ) + ( f2 ^ 1 );

		si += 3;
	}
	return si - shadowIndexes;

#else

	int i;
	const silEdge_t *sil;
	vertIndex_t *si;

	si = shadowIndexes;
	for ( sil = silEdges, i = numSilEdges; i > 0; i--, sil++ ) {

		byte f1 = facing[sil->p1];
		byte f2 = facing[sil->p2];

		if ( f1 != f2 )	{

			int	v1 = sil->v1;
			int	v2 = sil->v2;

			if ( f1 ) {
				si[0] = v1;
				si[1] = 1;
				si[2] = v2;
			} else {
				si[0] = v1;
				si[1] = v2;
				si[2] = 1;
			}
			si += 3;
		}
	}
	return si - shadowIndexes;

#endif
}

/*
============
idSIMD_Generic::ShadowVolume_CreateCapTriangles
============
*/
int VPCALL idSIMD_Generic::ShadowVolume_CreateCapTriangles( vertIndex_t *shadowIndexes, const byte *facing, const vertIndex_t *indexes, const int numIndexes ) {
	int i, j;
	vertIndex_t *si;

	si = shadowIndexes;
	for ( i = 0, j = 0; i < numIndexes; i += 3, j++ ) {
		if ( facing[j] ) {
			continue;
		}

		int i0 = indexes[i+0] * 2;
		int i1 = indexes[i+1] * 2;
		int i2 = indexes[i+2] * 2;

		si[0] = i0;
		si[1] = i1;
		si[2] = i2;

		si[3] = i2 + 1;
		si[4] = i1 + 1;
		si[5] = i0 + 1;

		si += 6;
	}
	return si - shadowIndexes;
}

/*
============
idSIMD_Generic::ShadowVolume_CreateCapTrianglesParallel
============
*/
int VPCALL idSIMD_Generic::ShadowVolume_CreateCapTrianglesParallel( vertIndex_t *shadowIndexes, const byte *facing, const vertIndex_t *indexes, const int numIndexes ) {
	int i, j;
	vertIndex_t *si;

	si = shadowIndexes;
	for ( i = 0, j = 0; i < numIndexes; i += 3, j++ ) {
		if ( facing[j] ) {
			continue;
		}

		si[0] = indexes[i+0] * 2;
		si[1] = indexes[i+1] * 2;
		si[2] = indexes[i+2] * 2;

		si += 3;
	}
	return si - shadowIndexes;
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
void VPCALL idSIMD_Generic::MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) {
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
void VPCALL idSIMD_Generic::MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) {
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
idSIMD_Generic::MixSoundFourSpeakerMono
============
*/
void VPCALL idSIMD_Generic::MixSoundFourSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) {
	float sL0 = lastV[0];
	float sL1 = lastV[1];
	float sL2 = lastV[4];
	float sL3 = lastV[5];

	float incL0 = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	float incL1 = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;
	float incL2 = ( currentV[4] - lastV[4] ) / MIXBUFFER_SAMPLES;
	float incL3 = ( currentV[5] - lastV[5] ) / MIXBUFFER_SAMPLES;

	assert( numSamples == MIXBUFFER_SAMPLES );

	for( int i = 0; i < MIXBUFFER_SAMPLES; i++ ) {
		mixBuffer[i*4+0] += samples[i] * sL0;
		mixBuffer[i*4+1] += samples[i] * sL1;
		mixBuffer[i*4+2] += samples[i] * sL2;
		mixBuffer[i*4+3] += samples[i] * sL3;
		sL0 += incL0;
		sL1 += incL1;
		sL2 += incL2;
		sL3 += incL3;
	}
}

/*
============
idSIMD_Generic::MixSoundFourSpeakerStereo
============
*/
void VPCALL idSIMD_Generic::MixSoundFourSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) {
	float sL0 = lastV[0];
	float sL1 = lastV[1];
	float sL2 = lastV[4];
	float sL3 = lastV[5];

	float incL0 = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	float incL1 = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;
	float incL2 = ( currentV[4] - lastV[4] ) / MIXBUFFER_SAMPLES;
	float incL3 = ( currentV[5] - lastV[5] ) / MIXBUFFER_SAMPLES;

	assert( numSamples == MIXBUFFER_SAMPLES );

	for( int i = 0; i < MIXBUFFER_SAMPLES; i++ ) {
		mixBuffer[i*4+0] += samples[i*2+0] * sL0;
		mixBuffer[i*4+1] += samples[i*2+1] * sL1;
		mixBuffer[i*4+2] += samples[i*2+0] * sL2;
		mixBuffer[i*4+3] += samples[i*2+1] * sL3;
		sL0 += incL0;
		sL1 += incL1;
		sL2 += incL2;
		sL3 += incL3;
	}
}

/*
============
idSIMD_Generic::MixSoundSixSpeakerMono
============
*/
void VPCALL idSIMD_Generic::MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) {
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
void VPCALL idSIMD_Generic::MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) {
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
idSIMD_Generic::MixSoundEightSpeakerMono
============
*/
void VPCALL idSIMD_Generic::MixSoundEightSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[8], const float currentV[8] ) {
	float sL0 = lastV[0];
	float sL1 = lastV[1];
	float sL2 = lastV[2];
	float sL3 = lastV[3];
	float sL4 = lastV[4];
	float sL5 = lastV[5];
	float sL6 = lastV[6];
	float sL7 = lastV[7];

	float incL0 = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	float incL1 = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;
	float incL2 = ( currentV[2] - lastV[2] ) / MIXBUFFER_SAMPLES;
	float incL3 = ( currentV[3] - lastV[3] ) / MIXBUFFER_SAMPLES;
	float incL4 = ( currentV[4] - lastV[4] ) / MIXBUFFER_SAMPLES;
	float incL5 = ( currentV[5] - lastV[5] ) / MIXBUFFER_SAMPLES;
	float incL6 = ( currentV[6] - lastV[6] ) / MIXBUFFER_SAMPLES;
	float incL7 = ( currentV[7] - lastV[7] ) / MIXBUFFER_SAMPLES;

	assert( numSamples == MIXBUFFER_SAMPLES );

	for( int i = 0; i < MIXBUFFER_SAMPLES; i++ ) {
		mixBuffer[i*8+0] += samples[i] * sL0;
		mixBuffer[i*8+1] += samples[i] * sL1;
		mixBuffer[i*8+2] += samples[i] * sL2;
		mixBuffer[i*8+3] += samples[i] * sL3;
		mixBuffer[i*8+4] += samples[i] * sL4;
		mixBuffer[i*8+5] += samples[i] * sL5;
		mixBuffer[i*8+6] += samples[i] * sL6;
		mixBuffer[i*8+7] += samples[i] * sL7;
		sL0 += incL0;
		sL1 += incL1;
		sL2 += incL2;
		sL3 += incL3;
		sL4 += incL4;
		sL5 += incL5;
		sL6 += incL6;
		sL7 += incL7;
	}
}

/*
============
idSIMD_Generic::MixSoundEightSpeakerStereo
============
*/
void VPCALL idSIMD_Generic::MixSoundEightSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[8], const float currentV[8] ) {
	float sL0 = lastV[0];
	float sL1 = lastV[1];
	float sL2 = lastV[2];
	float sL3 = lastV[3];
	float sL4 = lastV[4];
	float sL5 = lastV[5];
	float sL6 = lastV[6];
	float sL7 = lastV[7];

	float incL0 = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	float incL1 = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;
	float incL2 = ( currentV[2] - lastV[2] ) / MIXBUFFER_SAMPLES;
	float incL3 = ( currentV[3] - lastV[3] ) / MIXBUFFER_SAMPLES;
	float incL4 = ( currentV[4] - lastV[4] ) / MIXBUFFER_SAMPLES;
	float incL5 = ( currentV[5] - lastV[5] ) / MIXBUFFER_SAMPLES;
	float incL6 = ( currentV[6] - lastV[6] ) / MIXBUFFER_SAMPLES;
	float incL7 = ( currentV[7] - lastV[7] ) / MIXBUFFER_SAMPLES;

	assert( numSamples == MIXBUFFER_SAMPLES );

	for( int i = 0; i < MIXBUFFER_SAMPLES; i++ ) {
		mixBuffer[i*8+0] += samples[i*2+0] * sL0;
		mixBuffer[i*8+1] += samples[i*2+1] * sL1;
		mixBuffer[i*8+2] += samples[i*2+0] * sL2;
		mixBuffer[i*8+3] += samples[i*2+0] * sL3;
		mixBuffer[i*8+4] += samples[i*2+0] * sL4;
		mixBuffer[i*8+5] += samples[i*2+1] * sL5;
		mixBuffer[i*8+6] += samples[i*2+0] * sL6;
		mixBuffer[i*8+7] += samples[i*2+1] * sL7;
		sL0 += incL0;
		sL1 += incL1;
		sL2 += incL2;
		sL3 += incL3;
		sL4 += incL4;
		sL5 += incL5;
		sL6 += incL6;
		sL7 += incL7;
	}
}

/*
============
idSIMD_Generic::MixedSoundToSamples
============
*/
void VPCALL idSIMD_Generic::MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples ) {

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

#pragma warning( default : 4244 )
