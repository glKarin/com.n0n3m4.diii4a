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

#include "renderer/tr_local.h"
#include <math.h>

//====================================================================

static const unsigned int NUM_FRAME_DATA = 2;
static const unsigned int FRAME_ALLOC_ALIGNMENT = 128;
static const unsigned int MAX_FRAME_MEMORY = 64 * 1024 * 1024;	// larger so that we can noclip on PC for dev purposes

frameData_t		smpFrameData[NUM_FRAME_DATA];
frameData_t 	*frameData;
frameData_t		*backendFrameData;
unsigned int	smpFrame;

/*
======================
idScreenRect::Clear
======================
*/
void idScreenRect::Clear() {
	x1 = y1 = 32000;
	x2 = y2 = -32000;
	zmin = 0.0f;
	zmax = 1.0f;
}

/*
======================
idScreenRect::ClearWithZ
======================
*/
void idScreenRect::ClearWithZ() {
	Clear();
	zmin = 1.0f;
	zmax = 0.0f;
}

/*
======================
idScreenRect::AddPoint
======================
*/
void idScreenRect::AddPoint( float x, float y ) {
	int	ix = idMath::FtoiRound( x );
	int iy = idMath::FtoiRound( y );

	if ( ix < x1 ) {
		x1 = ix;
	}
	if ( ix > x2 ) {
		x2 = ix;
	}
	if ( iy < y1 ) {
		y1 = iy;
	}
	if ( iy > y2 ) {
		y2 = iy;
	}
}

/*
======================
idScreenRect::Expand
======================
*/
void idScreenRect::Expand( int pixels ) {
	x1 -= pixels;
	y1 -= pixels;
	x2 += pixels;
	y2 += pixels;
}

/*
======================
idScreenRect::Intersect
======================
*/
void idScreenRect::Intersect( const idScreenRect &rect ) {
	if ( rect.x1 > x1 ) {
		x1 = rect.x1;
	}
	if ( rect.x2 < x2 ) {
		x2 = rect.x2;
	}
	if ( rect.y1 > y1 ) {
		y1 = rect.y1;
	}
	if ( rect.y2 < y2 ) {
		y2 = rect.y2;
	}
}

/*
======================
idScreenRect::IntersectWithZ
======================
*/
void idScreenRect::IntersectWithZ( const idScreenRect &rect ) {
	Intersect( rect );
	zmin = idMath::Fmax( zmin, rect.zmin );
	zmax = idMath::Fmin( zmax, rect.zmax );
}

/*
======================
idScreenRect::Union
======================
*/
void idScreenRect::Union( const idScreenRect &rect ) {
	if ( rect.x1 < x1 ) {
		x1 = rect.x1;
	}
	if ( rect.x2 > x2 ) {
		x2 = rect.x2;
	}
	if ( rect.y1 < y1 ) {
		y1 = rect.y1;
	}
	if ( rect.y2 > y2 ) {
		y2 = rect.y2;
	}
}

/*
======================
idScreenRect::UnionWithZ
======================
*/
void idScreenRect::UnionWithZ( const idScreenRect &rect ) {
	Union( rect );
	zmin = idMath::Fmin( zmin, rect.zmin );
	zmax = idMath::Fmax( zmax, rect.zmax );
}

/*
======================
idScreenRect::IsEmpty
======================
*/
bool idScreenRect::IsEmpty() const {
	return ( x1 > x2 || y1 > y2 );
}

/*
======================
idScreenRect::IsEmptyWithZ
======================
*/
bool idScreenRect::IsEmptyWithZ() const {
	return IsEmpty() || zmin > zmax;
}

/*
======================
R_ScreenRectFromViewFrustumBounds
======================
*/
idScreenRect R_ScreenRectFromViewFrustumBounds( const idBounds &bounds ) {
	idScreenRect screenRect;

	screenRect.x1 = idMath::FtoiRound( 0.5f * ( 1.0f - bounds[1].y ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 ) );
	screenRect.x2 = idMath::FtoiRound( 0.5f * ( 1.0f - bounds[0].y ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 ) );
	screenRect.y1 = idMath::FtoiRound( 0.5f * ( 1.0f + bounds[0].z ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 ) );
	screenRect.y2 = idMath::FtoiRound( 0.5f * ( 1.0f + bounds[1].z ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 ) );

	assert( bounds[0].x <= bounds[1].x );
	float zmin, zmax;
	R_TransformEyeZToDepth( -bounds[0].x, tr.viewDef->projectionMatrix, zmin );
	R_TransformEyeZToDepth( -bounds[1].x, tr.viewDef->projectionMatrix, zmax );
	// stgatilov: slightly expand the bounds to cover floating-point errors
	// this is similar to what idScreenRect::Expand does to scissor
	zmin = idMath::ClampFloat( 0.0f, 1.0f, zmin - 1e-6f );
	zmax = idMath::ClampFloat( 0.0f, 1.0f, zmax + 1e-6f );
	// this should never fail... but even if it does, ensure bounds are not inverted/empty
	assert( zmin <= zmax );
	screenRect.zmin = idMath::Fmin( zmin, zmax );
	screenRect.zmax = idMath::Fmax( zmin, zmax );

	return screenRect;
}

/*
======================
R_ShowColoredScreenRect
======================
*/
void R_ShowColoredScreenRect( const idScreenRect &rect, int colorIndex ) {
	if ( !rect.IsEmpty() ) {
		static idVec4 colors[] = { colorRed, colorGreen, colorBlue, colorYellow, colorMagenta, colorCyan, colorWhite, colorPurple };
		tr.viewDef->renderWorld->DebugScreenRect( colors[colorIndex & 7], rect, tr.viewDef );
	}
}

/*
====================
R_ToggleSmpFrame
====================
*/
void R_ToggleSmpFrame( void ) {
	// update the highwater mark
	if ( frameData->frameMemoryAllocated > frameData->memoryHighwater ) {
		frameData->memoryHighwater = frameData->frameMemoryAllocated;
	}

	// switch to the next frame
	smpFrame++;
	if (com_smp.GetBool()) {
		backendFrameData = frameData;
		frameData = &smpFrameData[smpFrame % NUM_FRAME_DATA];
	} else {
		frameData = backendFrameData = &smpFrameData[0];
	}

	// reset the memory allocation
	R_FreeDeferredTriSurfs( frameData );

	// RB: 64 bit fixes, changed unsigned int to uintptr_t
	const uintptr_t bytesNeededForAlignment = FRAME_ALLOC_ALIGNMENT - ( ( uintptr_t )frameData->frameMemory & ( FRAME_ALLOC_ALIGNMENT - 1 ) );
	// RB end

	frameData->frameMemoryAllocated = bytesNeededForAlignment;
	frameData->frameMemoryUsed = 0;

	R_ClearCommandChain( frameData );
}


//=====================================================

#define	MEMORY_BLOCK_SIZE	0x100000

/*
=====================
R_ShutdownFrameData
=====================
*/
void R_ShutdownFrameData( void ) {
	R_FreeDeferredTriSurfs( frameData );
	frameData = NULL;
	for ( int i = 0; i < NUM_FRAME_DATA; i++ ) {
		Mem_Free16( smpFrameData[i].frameMemory );
		smpFrameData[i].frameMemory = NULL;
	}
}

/*
=====================
R_InitFrameData
=====================
*/
void R_InitFrameData( void ) {
	R_ShutdownFrameData();

	for ( int i = 0; i < NUM_FRAME_DATA; i++ ) {
		smpFrameData[i].frameMemory = ( byte * )Mem_Alloc16( MAX_FRAME_MEMORY );
	}

	// must be set before calling R_ToggleSmpFrame()
	frameData = &smpFrameData[0];
	backendFrameData = &smpFrameData[1];

	R_ToggleSmpFrame();
	R_ClearCommandChain( backendFrameData );
}

/*
=================
R_StaticAlloc
=================
*/
void *R_StaticAlloc( int bytes ) {
	void	*buf;

	tr.pc.c_alloc++;

	tr.staticAllocCount += bytes;

	buf = Mem_Alloc( bytes );

	// don't exit on failure on zero length allocations since the old code didn't
	if ( !buf && ( bytes != 0 ) ) {
		common->FatalError( "R_StaticAlloc failed on %i bytes", bytes );
	}
	return buf;
}

/*
=================
R_ClearedStaticAlloc
=================
*/
void *R_ClearedStaticAlloc( int bytes ) {
	void	*buf;

	buf = R_StaticAlloc( bytes );
	SIMDProcessor->Memset( buf, 0, bytes );
	return buf;
}

/*
=================
R_StaticFree
=================
*/
void R_StaticFree( void *data ) {
	tr.pc.c_free++;
	Mem_Free( data );
}

/*
================
R_FrameAlloc

This data will be automatically freed when the
current frame's back end completes.

This should only be called by the front end.  The
back end shouldn't need to allocate memory.

If we passed smpFrame in, the back end could
alloc memory, because it will always be a
different frameData than the front end is using.

All temporary data, like dynamic tesselations
and local spaces are allocated here.

The memory will not move, but it may not be
contiguous with previous allocations even
from this frame.

The memory is NOT zero filled.
Should part of this be inlined in a macro?
================
*/
void *R_FrameAlloc( int bytes ) {
	bytes = ( bytes + FRAME_ALLOC_ALIGNMENT - 1 ) & ~( FRAME_ALLOC_ALIGNMENT - 1 );

	// thread safe add
	int	end = frameData->frameMemoryAllocated += bytes;

	if ( end > MAX_FRAME_MEMORY ) {
		idLib::Error( "R_FrameAlloc ran out of memory. bytes = %d, end = %d, highWaterAllocated = %d\n", bytes, end, frameData->memoryHighwater );
	}
	byte *ptr = frameData->frameMemory + end - bytes;

	return ptr;
}

/*
==================
R_ClearedFrameAlloc
==================
*/
void *R_ClearedFrameAlloc( int bytes ) {
	void *r;
	r = R_FrameAlloc( bytes );
	SIMDProcessor->Memset( r, 0, bytes );
	return r;
}


/*
==================
R_FrameFree

This does nothing at all, as the frame data is reused every frame
and can only be stack allocated.

The only reason for it's existance is so functions that can
use either static or frame memory can set function pointers
to both alloc and free.
==================
*/
void R_FrameFree( void *data ) {
}

//==========================================================================

void R_AxisToModelMatrix( const idMat3 &axis, const idVec3 &origin, float modelMatrix[16] ) {
	modelMatrix[0] = axis[0][0];
	modelMatrix[4] = axis[1][0];
	modelMatrix[8] = axis[2][0];
	modelMatrix[12] = origin[0];

	modelMatrix[1] = axis[0][1];
	modelMatrix[5] = axis[1][1];
	modelMatrix[9] = axis[2][1];
	modelMatrix[13] = origin[1];

	modelMatrix[2] = axis[0][2];
	modelMatrix[6] = axis[1][2];
	modelMatrix[10] = axis[2][2];
	modelMatrix[14] = origin[2];

	modelMatrix[3] = 0;
	modelMatrix[7] = 0;
	modelMatrix[11] = 0;
	modelMatrix[15] = 1;
}

DEBUG_OPTIMIZE_ON
// FIXME: these assume no skewing or scaling transforms
void R_LocalPointToGlobal( const float modelMatrix[16], const idVec3 &in, idVec3 &out ) {
#if defined(__SSE__)
	__m128 row0 = _mm_loadu_ps( &modelMatrix[0] );
	__m128 row1 = _mm_loadu_ps( &modelMatrix[4] );
	__m128 row2 = _mm_loadu_ps( &modelMatrix[8] );
	__m128 row3 = _mm_loadu_ps( &modelMatrix[12] );

	__m128 xxxx = _mm_set1_ps( in.x );
	__m128 yyyy = _mm_set1_ps( in.y );
	__m128 zzzz = _mm_set1_ps( in.z );

	__m128 res = _mm_add_ps(
	                 _mm_add_ps(
	                     _mm_mul_ps( row0, xxxx ),
	                     _mm_mul_ps( row1, yyyy )
	                 ),
	                 _mm_add_ps(
	                     _mm_mul_ps( row2, zzzz ),
	                     row3
	                 )
	             );

	//unaligned float x 3 store
	_mm_storel_pi( ( __m64 * )&out[0], res );
	_mm_store_ss( &out[2], _mm_movehl_ps( res, res ) );
#else
	out[0] = in[0] * modelMatrix[0] + in[1] * modelMatrix[4]
	         + in[2] * modelMatrix[8] + modelMatrix[12];
	out[1] = in[0] * modelMatrix[1] + in[1] * modelMatrix[5]
	         + in[2] * modelMatrix[9] + modelMatrix[13];
	out[2] = in[0] * modelMatrix[2] + in[1] * modelMatrix[6]
	         + in[2] * modelMatrix[10] + modelMatrix[14];
#endif
}
DEBUG_OPTIMIZE_OFF

void R_PointTimesMatrix( const float modelMatrix[16], const idVec4 &in, idVec4 &out ) {
	out[0] = in[0] * modelMatrix[0] + in[1] * modelMatrix[4]
	         + in[2] * modelMatrix[8] + modelMatrix[12];
	out[1] = in[0] * modelMatrix[1] + in[1] * modelMatrix[5]
	         + in[2] * modelMatrix[9] + modelMatrix[13];
	out[2] = in[0] * modelMatrix[2] + in[1] * modelMatrix[6]
	         + in[2] * modelMatrix[10] + modelMatrix[14];
	out[3] = in[0] * modelMatrix[3] + in[1] * modelMatrix[7]
	         + in[2] * modelMatrix[11] + modelMatrix[15];
}

DEBUG_OPTIMIZE_ON
void R_GlobalPointToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out ) {
	idVec3	temp;

	VectorSubtract( in, &modelMatrix[12], temp );

	out[0] = DotProduct( temp, &modelMatrix[0] );
	out[1] = DotProduct( temp, &modelMatrix[4] );
	out[2] = DotProduct( temp, &modelMatrix[8] );
}
DEBUG_OPTIMIZE_OFF

DEBUG_OPTIMIZE_ON
void R_LocalVectorToGlobal( const float modelMatrix[16], const idVec3 &in, idVec3 &out ) {
	out[0] = in[0] * modelMatrix[0] + in[1] * modelMatrix[4]
	         + in[2] * modelMatrix[8];
	out[1] = in[0] * modelMatrix[1] + in[1] * modelMatrix[5]
	         + in[2] * modelMatrix[9];
	out[2] = in[0] * modelMatrix[2] + in[1] * modelMatrix[6]
	         + in[2] * modelMatrix[10];
}
DEBUG_OPTIMIZE_OFF

DEBUG_OPTIMIZE_ON
void R_GlobalVectorToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out ) {
	out[0] = DotProduct( in, &modelMatrix[0] );
	out[1] = DotProduct( in, &modelMatrix[4] );
	out[2] = DotProduct( in, &modelMatrix[8] );
}
DEBUG_OPTIMIZE_OFF

DEBUG_OPTIMIZE_ON
void R_GlobalPlaneToLocal( const float modelMatrix[16], const idPlane &in, idPlane &out ) {
	out[0] = DotProduct( in, &modelMatrix[0] );
	out[1] = DotProduct( in, &modelMatrix[4] );
	out[2] = DotProduct( in, &modelMatrix[8] );
	out[3] = in[3] + modelMatrix[12] * in[0] + modelMatrix[13] * in[1] + modelMatrix[14] * in[2];
}
DEBUG_OPTIMIZE_OFF

DEBUG_OPTIMIZE_ON
void R_LocalPlaneToGlobal( const float modelMatrix[16], const idPlane &in, idPlane &out ) {
	float	offset;

	R_LocalVectorToGlobal( modelMatrix, in.Normal(), out.Normal() );

	offset = modelMatrix[12] * out[0] + modelMatrix[13] * out[1] + modelMatrix[14] * out[2];
	out[3] = in[3] - offset;
}
DEBUG_OPTIMIZE_OFF

// transform Z in eye coordinates to window coordinates (depth)
void R_TransformEyeZToDepth( float src_z, const float *projectionMatrix, float &dst_depth ) {
	float clip_z, clip_w;

	// projection
	clip_z = src_z * projectionMatrix[ 2 + 2 * 4 ] + projectionMatrix[ 2 + 3 * 4 ];
	clip_w = src_z * projectionMatrix[ 3 + 2 * 4 ] + projectionMatrix[ 3 + 3 * 4 ];

	if ( clip_w <= 0.0f ) {
		dst_depth = 0.0f;					// clamp to near plane
	} else {
		dst_depth = clip_z / clip_w;
		dst_depth = dst_depth * 0.5f + 0.5f;	// convert to window coords
	}
}

// transform [0..1] depth to Z in eye coordinates
void R_TransformDepthToEyeZ( float src_depth, const float *projectionMatrix, float &dst_z ) {
	// convert to NDC coords
	float ndcZ = 2.0f * src_depth - 1.0f;

	// this is exactly the equation from R_TransformEyeZToWin solved for Z
	float numer = projectionMatrix[ 2 + 3 * 4 ] - ndcZ * projectionMatrix[ 3 + 3 * 4 ];
	float denom = projectionMatrix[ 2 + 2 * 4 ] - ndcZ * projectionMatrix[ 3 + 2 * 4 ];

	// stgatilov: not sure this is correct!
	// also, we should take care to restore +infty for depth > 0.999
	// (see R_SetupProjection for far-at-infinity details)
	assert(0);

	dst_z = - numer / denom;
}

/*
=================
R_BoundingSphereOfLocalBox

stgatilov: Conservative bounding sphere for a box in local coordinates.
Note that it also supports rotation-hacked entities with non-orthogonal modelMatrix.
=================
*/
idSphere R_BoundingSphereOfLocalBox( const idBounds &bounds, const float modelMatrix[16] ) {
	// transform the surface bounds into world space
	idVec3	localOrigin = ( bounds[0] + bounds[1] ) * 0.5;

	idVec3 worldOrigin;
	R_LocalPointToGlobal( modelMatrix, localOrigin, worldOrigin );

	//stgatilov #4970: should be 1 for orthogonal transformations
	float maxScale = idMath::Sqrt( idMath::Fmax( idMath::Fmax (
		idVec3(modelMatrix[0], modelMatrix[1], modelMatrix[2]).LengthSqr(),
		idVec3(modelMatrix[4], modelMatrix[5], modelMatrix[6]).LengthSqr() ),
		idVec3(modelMatrix[8], modelMatrix[9], modelMatrix[10]).LengthSqr() )
	);
	float worldRadius = ( bounds[0] - localOrigin ).Length() * maxScale;

	return idSphere( worldOrigin, worldRadius );
}

/*
=================
R_RadiusCullLocalBox

A fast, conservative sphere + frustum culling test
Returns true if the sphere is outside the given global frustum (positive sides are out)
=================
*/
bool R_CullFrustumSphere( const idSphere &bounds, int numPlanes, const idPlane *planes )
{
	for ( int i = 0 ; i < numPlanes ; i++ ) {
		const idPlane *frust = planes + i;
		float d = frust->Distance( bounds.GetOrigin() );
		if ( d > bounds.GetRadius() ) {
			return true;	// culled
		}
	}
	return false;		// no culled
}

/*
=================
R_RadiusCullLocalBox

A fast, conservative center-to-corner culling test
Returns true if the box is outside the given global frustum (positive sides are out)
=================
*/
bool R_RadiusCullLocalBox( const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes ) {
	if ( r_useCulling.GetInteger() == 0 ) {
		return false;
	}
	idSphere boundingSphere = R_BoundingSphereOfLocalBox( bounds, modelMatrix );
	return R_CullFrustumSphere( boundingSphere, numPlanes, planes );
}

/*
=================
R_CornerCullLocalBox

Tests all corners against the frustum.
Can still generate a few false positives when the box is outside a corner.
Returns true if the box is outside the given global frustum, (positive sides are out)
=================
*/
bool R_CornerCullLocalBox( const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes ) {
	// we can disable box culling for experimental timing purposes
	if ( r_useCulling.GetInteger() < 2 ) {
		return false;
	}

	//stgatilov: while here is some vectorized version,
	//we cannot enable it unless this place starts eating CPU time
	//otherwise it is not even possible to say if the new approach is actually faster than the trivial one!
#if 0 && defined(__SSE2__)
#define SHUF(a, b, c, d) _MM_SHUFFLE(d, c, b, a)
	/*//DEBUG: interleave two equivalent methods to see that they produce same stats with "r_showCull 1"
	if (tr.frameCount & 1)
	return R_CornerCullLocalBox( bounds, modelMatrix, numPlanes, planes );*/

	//prepare transposed modelview matrix
	__m128 row0 = _mm_loadu_ps( &modelMatrix[0] );
	__m128 row1 = _mm_loadu_ps( &modelMatrix[4] );
	__m128 row2 = _mm_loadu_ps( &modelMatrix[8] );
	__m128 row3 = _mm_loadu_ps( &modelMatrix[12] );
	_MM_TRANSPOSE4_PS( row0, row1, row2, row3 );

	//load bounds to two vectors
	static_assert( sizeof( idBounds ) == 24, "idPlane must be tightly packed" );
	__m128 bmin = _mm_loadu_ps( &bounds[0].x );
	__m128 bmax = _mm_loadu_ps( &bounds[0].z );
	bmax = _mm_shuffle_ps( bmax, bmax, SHUF( 1, 2, 3, 1 ) );
	//calculate center point and half-span
	__m128 bctr = _mm_mul_ps( _mm_add_ps( bmax, bmin ), _mm_set1_ps( 0.5f ) );
	__m128 bspan = _mm_mul_ps( _mm_sub_ps( bmax, bmin ), _mm_set1_ps( 0.5f ) );

	for ( int i = 0; i < numPlanes; i++ ) {
		static_assert( sizeof( idPlane ) == 16, "idPlane must be tightly packed" );
		//load plane XYZD
		__m128 plane = _mm_loadu_ps( planes[i].ToFloatPtr() );
		//compute dot product of normal with each coordinate axis:
		__m128 xxxx = _mm_shuffle_ps( plane, plane, SHUF( 0, 0, 0, 0 ) );
		__m128 yyyy = _mm_shuffle_ps( plane, plane, SHUF( 1, 1, 1, 1 ) );
		__m128 zzzz = _mm_shuffle_ps( plane, plane, SHUF( 2, 2, 2, 2 ) );
		//get dots = [(Ax*N), (Ay*N), (Az*N), (O*N)] :
		__m128 dots = _mm_add_ps( _mm_add_ps( _mm_mul_ps( row0, xxxx ), _mm_mul_ps( row1, yyyy ) ), _mm_mul_ps( row2, zzzz ) );
		//take absolute values of dot products (thus choosing side)
		__m128 absDots = _mm_and_ps( dots, _mm_castsi128_ps( _mm_set1_epi32( 0x7FFFFFFF ) ) );

		//calculate difference of two dot products  (we are taking minimum among distances)
		__m128 work = _mm_sub_ps( _mm_mul_ps( bctr, dots ), _mm_mul_ps( bspan, absDots ) );
		//(horizontal sum of XYZ: ignore W)
		__m128 res = work;
		res = _mm_add_ss( res, _mm_shuffle_ps( work, work, SHUF( 1, 1, 1, 1 ) ) );
		res = _mm_add_ss( res, _mm_shuffle_ps( work, work, SHUF( 2, 2, 2, 2 ) ) );
		//do not forget to take translation and plane offset into account
		__m128 inW = _mm_add_ps( plane, dots );
		res = _mm_add_ss( res, _mm_shuffle_ps( inW, inW, SHUF( 3, 3, 3, 3 ) ) );

		//extract float and do the check
		float dist = _mm_cvtss_f32( res );
		if ( dist >= 0.0f ) {
			//all points were behind one of the planes
			tr.pc.c_box_cull_out++;
			return true;
		}
	}
	tr.pc.c_box_cull_in++;
	return false;
#undef SHUF
#else
	int			i, j;
	idVec3		v;
	const idPlane *frust;

	idVec3		transformed[8];
	// transform into world space
	for ( i = 0; i < 8; i++ ) {
		v[0] = bounds[i & 1][0];
		v[1] = bounds[( i >> 1 ) & 1][1];
		v[2] = bounds[( i >> 2 ) & 1][2];

		R_LocalPointToGlobal( modelMatrix, v, transformed[i] );
	}
	// check against frustum planes
	for ( i = 0; i < numPlanes; i++ ) {
		frust = planes + i;
		for ( j = 0; j < 8; j++ ) {
			float dist = frust->Distance( transformed[j] );
			if ( dist < 0 ) {
				break;
			}
		}
		if ( j == 8 ) {
			// all points were behind one of the planes
			tr.pc.c_box_cull_out++;
			return true;
		}
	}
	tr.pc.c_box_cull_in++;

	return false;		// not culled
#endif
}

/*
==========================
R_TransformModelToClip
==========================
*/
void R_TransformModelToClip( const idVec3 &src, const float *modelMatrix, const float *projectionMatrix, idPlane &eye, idPlane &dst ) {
	int i;

	for ( i = 0 ; i < 4 ; i++ ) {
		eye[i] =
		    src[0] * modelMatrix[ i + 0 * 4 ] +
		    src[1] * modelMatrix[ i + 1 * 4 ] +
		    src[2] * modelMatrix[ i + 2 * 4 ] +
		    1 * modelMatrix[ i + 3 * 4 ];
	}

	for ( i = 0 ; i < 4 ; i++ ) {
		dst[i] =
		    eye[0] * projectionMatrix[ i + 0 * 4 ] +
		    eye[1] * projectionMatrix[ i + 1 * 4 ] +
		    eye[2] * projectionMatrix[ i + 2 * 4 ] +
		    eye[3] * projectionMatrix[ i + 3 * 4 ];
	}
}

/*
==========================
R_GlobalToNormalizedDeviceCoordinates

-1 to 1 range in x, y, and z
==========================
*/
void R_GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc ) {
	int		i;
	idPlane	view;
	idPlane	clip;

	// _D3XP added work on primaryView when no viewDef
	if ( !tr.viewDef ) {

		for ( i = 0 ; i < 4 ; i ++ ) {
			view[i] =
			    global[0] * tr.primaryView->worldSpace.modelViewMatrix[ i + 0 * 4 ] +
			    global[1] * tr.primaryView->worldSpace.modelViewMatrix[ i + 1 * 4 ] +
			    global[2] * tr.primaryView->worldSpace.modelViewMatrix[ i + 2 * 4 ] +
			    tr.primaryView->worldSpace.modelViewMatrix[ i + 3 * 4 ];
		}

		for ( i = 0 ; i < 4 ; i ++ ) {
			clip[i] =
			    view[0] * tr.primaryView->projectionMatrix[ i + 0 * 4 ] +
			    view[1] * tr.primaryView->projectionMatrix[ i + 1 * 4 ] +
			    view[2] * tr.primaryView->projectionMatrix[ i + 2 * 4 ] +
			    view[3] * tr.primaryView->projectionMatrix[ i + 3 * 4 ];
		}

	} else {

		for ( i = 0 ; i < 4 ; i ++ ) {
			view[i] =
			    global[0] * tr.viewDef->worldSpace.modelViewMatrix[ i + 0 * 4 ] +
			    global[1] * tr.viewDef->worldSpace.modelViewMatrix[ i + 1 * 4 ] +
			    global[2] * tr.viewDef->worldSpace.modelViewMatrix[ i + 2 * 4 ] +
			    tr.viewDef->worldSpace.modelViewMatrix[ i + 3 * 4 ];
		}


		for ( i = 0 ; i < 4 ; i ++ ) {
			clip[i] =
			    view[0] * tr.viewDef->projectionMatrix[ i + 0 * 4 ] +
			    view[1] * tr.viewDef->projectionMatrix[ i + 1 * 4 ] +
			    view[2] * tr.viewDef->projectionMatrix[ i + 2 * 4 ] +
			    view[3] * tr.viewDef->projectionMatrix[ i + 3 * 4 ];
		}
	}
	ndc[0] = clip[0] / clip[3];
	ndc[1] = clip[1] / clip[3];
	ndc[2] = ( clip[2] + clip[3] ) / ( 2 * clip[3] );
}

/*
==========================
R_TransformClipToDevice

Clip to normalized device coordinates
==========================
*/
void R_TransformClipToDevice( const idPlane &clip, const viewDef_t *view, idVec3 &normalized ) {
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = clip[2] / clip[3];
}


/*
==========================
myGlMultMatrix
==========================
*/
void myGlMultMatrix( const float a[16], const float b[16], float out[16] ) {
#ifdef __SSE__
	__m128 B0x = _mm_loadu_ps( &b[0] );
	__m128 B1x = _mm_loadu_ps( &b[4] );
	__m128 B2x = _mm_loadu_ps( &b[8] );
	__m128 B3x = _mm_loadu_ps( &b[12] );
	for ( int i = 0; i < 4; i++ ) {
		__m128 Ai0 = _mm_set1_ps( a[4 * i + 0] );
		__m128 Ai1 = _mm_set1_ps( a[4 * i + 1] );
		__m128 Ai2 = _mm_set1_ps( a[4 * i + 2] );
		__m128 Ai3 = _mm_set1_ps( a[4 * i + 3] );
		__m128 Rix = 
		_mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps( Ai0, B0x ),
				_mm_mul_ps( Ai1, B1x )
			),
			_mm_add_ps(
				_mm_mul_ps( Ai2, B2x ),
				_mm_mul_ps( Ai3, B3x )
			)
		);
		_mm_storeu_ps( &out[4 * i], Rix );
	}
#else
	out[0 * 4 + 0] = a[0 * 4 + 0] * b[0 * 4 + 0] + a[0 * 4 + 1] * b[1 * 4 + 0] + a[0 * 4 + 2] * b[2 * 4 + 0] + a[0 * 4 + 3] * b[3 * 4 + 0];
	out[0 * 4 + 1] = a[0 * 4 + 0] * b[0 * 4 + 1] + a[0 * 4 + 1] * b[1 * 4 + 1] + a[0 * 4 + 2] * b[2 * 4 + 1] + a[0 * 4 + 3] * b[3 * 4 + 1];
	out[0 * 4 + 2] = a[0 * 4 + 0] * b[0 * 4 + 2] + a[0 * 4 + 1] * b[1 * 4 + 2] + a[0 * 4 + 2] * b[2 * 4 + 2] + a[0 * 4 + 3] * b[3 * 4 + 2];
	out[0 * 4 + 3] = a[0 * 4 + 0] * b[0 * 4 + 3] + a[0 * 4 + 1] * b[1 * 4 + 3] + a[0 * 4 + 2] * b[2 * 4 + 3] + a[0 * 4 + 3] * b[3 * 4 + 3];
	out[1 * 4 + 0] = a[1 * 4 + 0] * b[0 * 4 + 0] + a[1 * 4 + 1] * b[1 * 4 + 0] + a[1 * 4 + 2] * b[2 * 4 + 0] + a[1 * 4 + 3] * b[3 * 4 + 0];
	out[1 * 4 + 1] = a[1 * 4 + 0] * b[0 * 4 + 1] + a[1 * 4 + 1] * b[1 * 4 + 1] + a[1 * 4 + 2] * b[2 * 4 + 1] + a[1 * 4 + 3] * b[3 * 4 + 1];
	out[1 * 4 + 2] = a[1 * 4 + 0] * b[0 * 4 + 2] + a[1 * 4 + 1] * b[1 * 4 + 2] + a[1 * 4 + 2] * b[2 * 4 + 2] + a[1 * 4 + 3] * b[3 * 4 + 2];
	out[1 * 4 + 3] = a[1 * 4 + 0] * b[0 * 4 + 3] + a[1 * 4 + 1] * b[1 * 4 + 3] + a[1 * 4 + 2] * b[2 * 4 + 3] + a[1 * 4 + 3] * b[3 * 4 + 3];
	out[2 * 4 + 0] = a[2 * 4 + 0] * b[0 * 4 + 0] + a[2 * 4 + 1] * b[1 * 4 + 0] + a[2 * 4 + 2] * b[2 * 4 + 0] + a[2 * 4 + 3] * b[3 * 4 + 0];
	out[2 * 4 + 1] = a[2 * 4 + 0] * b[0 * 4 + 1] + a[2 * 4 + 1] * b[1 * 4 + 1] + a[2 * 4 + 2] * b[2 * 4 + 1] + a[2 * 4 + 3] * b[3 * 4 + 1];
	out[2 * 4 + 2] = a[2 * 4 + 0] * b[0 * 4 + 2] + a[2 * 4 + 1] * b[1 * 4 + 2] + a[2 * 4 + 2] * b[2 * 4 + 2] + a[2 * 4 + 3] * b[3 * 4 + 2];
	out[2 * 4 + 3] = a[2 * 4 + 0] * b[0 * 4 + 3] + a[2 * 4 + 1] * b[1 * 4 + 3] + a[2 * 4 + 2] * b[2 * 4 + 3] + a[2 * 4 + 3] * b[3 * 4 + 3];
	out[3 * 4 + 0] = a[3 * 4 + 0] * b[0 * 4 + 0] + a[3 * 4 + 1] * b[1 * 4 + 0] + a[3 * 4 + 2] * b[2 * 4 + 0] + a[3 * 4 + 3] * b[3 * 4 + 0];
	out[3 * 4 + 1] = a[3 * 4 + 0] * b[0 * 4 + 1] + a[3 * 4 + 1] * b[1 * 4 + 1] + a[3 * 4 + 2] * b[2 * 4 + 1] + a[3 * 4 + 3] * b[3 * 4 + 1];
	out[3 * 4 + 2] = a[3 * 4 + 0] * b[0 * 4 + 2] + a[3 * 4 + 1] * b[1 * 4 + 2] + a[3 * 4 + 2] * b[2 * 4 + 2] + a[3 * 4 + 3] * b[3 * 4 + 2];
	out[3 * 4 + 3] = a[3 * 4 + 0] * b[0 * 4 + 3] + a[3 * 4 + 1] * b[1 * 4 + 3] + a[3 * 4 + 2] * b[2 * 4 + 3] + a[3 * 4 + 3] * b[3 * 4 + 3];
#endif
}

/*
================
R_TransposeGLMatrix
================
*/
void R_TransposeGLMatrix( const float in[16], float out[16] ) {
	int		i, j;

	for ( i = 0 ; i < 4 ; i++ ) {
		for ( j = 0 ; j < 4 ; j++ ) {
			out[i * 4 + j] = in[j * 4 + i];
		}
	}
}

/*
================
R_IdentityGLMatrix
================
*/
void R_IdentityGLMatrix( float out[16] ) {
	for ( int i = 0; i < 4; i++ )
		for ( int j = 0; j < 4; j++ )
			out[i * 4 + j] = float( i == j );
}

/*
=================
R_SetViewMatrix

Sets up the world to view matrix for a given viewParm
=================
*/
void R_SetViewMatrix( viewDef_t &viewDef ) {
	idVec3	origin;
	viewEntity_t *world;
	float	viewerMatrix[16];
	static float	s_flipMatrix[16] = {
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		0, 0, -1, 0,
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	};

	world = &viewDef.worldSpace;

	memset( world, 0, sizeof( *world ) );

	// the model matrix is an identity
	world->modelMatrix[0 * 4 + 0] = 1;
	world->modelMatrix[1 * 4 + 1] = 1;
	world->modelMatrix[2 * 4 + 2] = 1;

	// transform by the camera placement
	origin = viewDef.renderView.vieworg;

	viewerMatrix[0] = viewDef.renderView.viewaxis[0][0];
	viewerMatrix[4] = viewDef.renderView.viewaxis[0][1];
	viewerMatrix[8] = viewDef.renderView.viewaxis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];

	viewerMatrix[1] = viewDef.renderView.viewaxis[1][0];
	viewerMatrix[5] = viewDef.renderView.viewaxis[1][1];
	viewerMatrix[9] = viewDef.renderView.viewaxis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];

	viewerMatrix[2] = viewDef.renderView.viewaxis[2][0];
	viewerMatrix[6] = viewDef.renderView.viewaxis[2][1];
	viewerMatrix[10] = viewDef.renderView.viewaxis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix( viewerMatrix, s_flipMatrix, world->modelViewMatrix );
}

/*
===============
R_SetupProjection

This uses the "infinite far z" trick
===============
*/
void R_SetupProjection( void ) {
	float	xmin, xmax, ymin, ymax;
	float	width, height;
	float	zNear;
	float	jitterx, jittery;
	static	idRandom random;

	// random jittering is usefull when multiple
	// frames are going to be blended together
	// for motion blurred anti-aliasing
	if ( r_jitter.GetBool() ) {
		jitterx = random.RandomFloat();
		jittery = random.RandomFloat();
	} else {
		jitterx = jittery = 0;
	}

	//
	// set up projection matrix
	//
	zNear	= r_znear.GetFloat();
	if ( tr.viewDef->renderView.cramZNear ) {
		zNear *= 0.25;
	}

	ymax = zNear * tan( tr.viewDef->renderView.fov_y * idMath::PI / 360.0f );
	ymin = -ymax;

	xmax = zNear * tan( tr.viewDef->renderView.fov_x * idMath::PI / 360.0f );
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;

	jitterx = jitterx * width / ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 + 1 );
	xmin += jitterx;
	xmax += jitterx;
	jittery = jittery * height / ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 + 1 );
	ymin += jittery;
	ymax += jittery;

	tr.viewDef->projectionMatrix[0] = 2 * zNear / width;
	tr.viewDef->projectionMatrix[4] = 0;
	tr.viewDef->projectionMatrix[8] = ( xmax + xmin ) / width;	// normally 0
	tr.viewDef->projectionMatrix[12] = 0;

	tr.viewDef->projectionMatrix[1] = 0;
	tr.viewDef->projectionMatrix[5] = 2 * zNear / height;
	tr.viewDef->projectionMatrix[9] = ( ymax + ymin ) / height;	// normally 0
	tr.viewDef->projectionMatrix[13] = 0;

	// this is the far-plane-at-infinity formulation, and
	// crunches the Z range slightly so w=0 vertexes do not
	// rasterize right at the wraparound point
	tr.viewDef->projectionMatrix[2] = 0;
	tr.viewDef->projectionMatrix[6] = 0;
	tr.viewDef->projectionMatrix[10] = -0.999f;
	tr.viewDef->projectionMatrix[14] = -2.0f * zNear;

	tr.viewDef->projectionMatrix[3] = 0;
	tr.viewDef->projectionMatrix[7] = 0;
	tr.viewDef->projectionMatrix[11] = -1;
	tr.viewDef->projectionMatrix[15] = 0;

	// setup render matrices for faster culling
	idRenderMatrix::Transpose( *(idRenderMatrix*)tr.viewDef->projectionMatrix, tr.viewDef->projectionRenderMatrix );
	idRenderMatrix viewRenderMatrix;
	idRenderMatrix::Transpose( *(idRenderMatrix*)tr.viewDef->worldSpace.modelViewMatrix, viewRenderMatrix );
	idRenderMatrix::Multiply( tr.viewDef->projectionRenderMatrix, viewRenderMatrix, tr.viewDef->worldSpace.mvp );
}

/*
=================
R_SetupViewFrustum

Setup that culling frustum planes for the current view
FIXME: derive from modelview matrix times projection matrix
=================
*/
static void R_SetupViewFrustum( void ) {
	int		i;
	float	xs, xc;
	float	ang;

	ang = DEG2RAD( tr.viewDef->renderView.fov_x ) * 0.5f;
	idMath::SinCos( ang, xs, xc );

	tr.viewDef->frustum[0] = xs * tr.viewDef->renderView.viewaxis[0] + xc * tr.viewDef->renderView.viewaxis[1];
	tr.viewDef->frustum[1] = xs * tr.viewDef->renderView.viewaxis[0] - xc * tr.viewDef->renderView.viewaxis[1];

	ang = DEG2RAD( tr.viewDef->renderView.fov_y ) * 0.5f;
	idMath::SinCos( ang, xs, xc );

	tr.viewDef->frustum[2] = xs * tr.viewDef->renderView.viewaxis[0] + xc * tr.viewDef->renderView.viewaxis[2];
	tr.viewDef->frustum[3] = xs * tr.viewDef->renderView.viewaxis[0] - xc * tr.viewDef->renderView.viewaxis[2];

	// plane four is the front clipping plane
	tr.viewDef->frustum[4] = /* vec3_origin - */ tr.viewDef->renderView.viewaxis[0];

	for ( i = 0; i < 5; i++ ) {
		// flip direction so positive side faces out (FIXME: globally unify this)
		tr.viewDef->frustum[i] = -tr.viewDef->frustum[i].Normal();
		tr.viewDef->frustum[i][3] = -( tr.viewDef->renderView.vieworg * tr.viewDef->frustum[i].Normal() );
	}

	// eventually, plane five will be the rear clipping plane for fog
	float dNear, dFar, dLeft, dUp;

	dNear = r_znear.GetFloat();

	if ( tr.viewDef->renderView.cramZNear ) {
		dNear *= 0.25f;
	}
	dFar = MAX_WORLD_SIZE;
	dLeft = dFar * tan( DEG2RAD( tr.viewDef->renderView.fov_x * 0.5f ) );
	dUp = dFar * tan( DEG2RAD( tr.viewDef->renderView.fov_y * 0.5f ) );
	tr.viewDef->viewFrustum.SetOrigin( tr.viewDef->renderView.vieworg );
	tr.viewDef->viewFrustum.SetAxis( tr.viewDef->renderView.viewaxis );
	tr.viewDef->viewFrustum.SetSize( dNear, dFar, dLeft, dUp );
}

/*
===================
R_ConstrainViewFrustum
===================
*/
static void R_ConstrainViewFrustum( void ) {
	idBounds bounds;

	// constrain the view frustum to the total bounds of all visible lights and visible entities
	bounds.Clear();
	for ( viewLight_t *vLight = tr.viewDef->viewLights; vLight; vLight = vLight->next ) {
		bounds.AddBounds( vLight->lightDef->frustumTris->bounds );
	}
	for ( viewEntity_t *vEntity = tr.viewDef->viewEntitys; vEntity; vEntity = vEntity->next ) {
		bounds.AddBounds( vEntity->entityDef->referenceBounds );
	}
	tr.viewDef->viewFrustum.ConstrainToBounds( bounds );

	if ( r_useFrustumFarDistance.GetFloat() > 0.0f ) {
		tr.viewDef->viewFrustum.MoveFarDistance( r_useFrustumFarDistance.GetFloat() );
	}
}

/*
==========================================================================================

DRAWSURF SORTING

==========================================================================================
*/


/*
=======================
R_QsortSurfaces

=======================
*/
static bool R_StdSortSurfaces( const drawSurf_t *ea, const drawSurf_t *eb ) {

	// soft by "sort" value increasing
	if ( ea->sort != eb->sort )
		return ea->sort < eb->sort;

	// sort by material to reduce texture state changes in depth stage
	if (ea->material != eb->material)
		return ea->material < eb->material;

	// sort by entity parameters set
	return ea->space < eb->space;
}

/*
=================
R_SortDrawSurfs
=================
*/
static void R_SortDrawSurfs( void ) {
	TRACE_CPU_SCOPE( "R_SortDrawSurfs" )

	if ( !tr.viewDef->numDrawSurfs ) // otherwise an assert fails in debug builds
		return;
	tr.viewDef->numOffscreenSurfs = 0;
	// sort the drawsurfs by sort type, then orientation, then shader
	std::sort( tr.viewDef->drawSurfs, tr.viewDef->drawSurfs + tr.viewDef->numDrawSurfs, R_StdSortSurfaces );
}

//========================================================================

/*
================
R_RenderView

A view may be either the actual camera view,
a mirror / remote location, or a 3D view on a gui surface.

Parms will typically be allocated with R_FrameAlloc
================
*/
void R_RenderView( viewDef_t &parms ) {
	TRACE_CPU_SCOPE( "R_RenderView" )
	
	viewDef_t		*oldView;

	if ( parms.renderView.width <= 0 || parms.renderView.height <= 0 ) {
		return;
	}
	tr.viewCount++;
	parms.renderWorld->entityDefsInView.SetBitsSameAll(false);

	// save view in case we are a subview
	oldView = tr.viewDef;

	tr.viewDef = &parms;

	tr.sortOffset = 0;

	// set the matrix for world space to eye space
	R_SetViewMatrix( *tr.viewDef );

	// the four sides of the view frustum are needed
	// for culling and portal visibility
	R_SetupViewFrustum();

	// we need to set the projection matrix before doing
	// portal-to-screen scissor box calculations
	R_SetupProjection();

	// identify all the visible portalAreas, and the entityDefs and
	// lightDefs that are in them and pass culling.
	parms.renderWorld->FindViewLightsAndEntities();

	// constrain the view frustum to the view lights and entities
	R_ConstrainViewFrustum();

	// make sure that interactions exist for all light / entity combinations
	// that are visible
	// add any pre-generated light shadows, and calculate the light shader values
	R_AddLightSurfaces();

	// adds ambient surfaces and create any necessary interaction surfaces to add to the light
	// lists
	R_AddModelSurfaces();

	// any viewLight that didn't have visible surfaces can have it's shadows removed
	R_RemoveUnecessaryViewLights();

	// assign pages of shadow map buffer to lights that use shadow maps
	R_AssignShadowMapAtlasPages();

	// sort all the ambient surfaces for translucency ordering
	R_SortDrawSurfs();

	R_Tools();

	// generate any subviews (mirrors, cameras, etc) before adding this view
	if ( R_GenerateSubViews() ) {
		// if we are debugging subviews, allow the skipping of the
		// main view draw
		if ( r_subviewOnly.GetBool() ) {
			return;
		}
	}

	// copy drawsurf geo state for backend use
/*	for ( int i = 0; i < parms.numDrawSurfs; ++i ) {
		drawSurf_t *surf = parms.drawSurfs[i];
		srfTriangles_t *copiedGeo = ( srfTriangles_t * )R_FrameAlloc( sizeof( srfTriangles_t ) );
		memcpy( copiedGeo, surf->frontendGeo, sizeof( srfTriangles_t ) );
		surf->backendGeo = copiedGeo;
	}*/

	// write everything needed to the demo file
	if ( session->writeDemo ) {
		static_cast<idRenderWorldLocal *>( parms.renderWorld )->WriteVisibleDefs( tr.viewDef );
	}

	// add the rendering commands for this viewDef
	R_AddDrawViewCmd( parms );

	// restore view in case we are a subview
	tr.viewDef = oldView;
}
