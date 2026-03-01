/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Robert Beckebans

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

#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "../tr_local.h"

/*
==========================================================================================

OLD MATRIX MATH

==========================================================================================
*/

/*
==========================
R_MatrixMultiply
==========================
*/
void R_MatrixMultiply( const float a[16], const float b[16], float out[16] )
{
#if defined(USE_INTRINSICS)
	__m128 a0 = _mm_loadu_ps( a + 0 * 4 );
	__m128 a1 = _mm_loadu_ps( a + 1 * 4 );
	__m128 a2 = _mm_loadu_ps( a + 2 * 4 );
	__m128 a3 = _mm_loadu_ps( a + 3 * 4 );
	
	__m128 b0 = _mm_loadu_ps( b + 0 * 4 );
	__m128 b1 = _mm_loadu_ps( b + 1 * 4 );
	__m128 b2 = _mm_loadu_ps( b + 2 * 4 );
	__m128 b3 = _mm_loadu_ps( b + 3 * 4 );
	
	__m128 t0 = _mm_mul_ps( _mm_splat_ps( a0, 0 ), b0 );
	__m128 t1 = _mm_mul_ps( _mm_splat_ps( a1, 0 ), b0 );
	__m128 t2 = _mm_mul_ps( _mm_splat_ps( a2, 0 ), b0 );
	__m128 t3 = _mm_mul_ps( _mm_splat_ps( a3, 0 ), b0 );
	
	t0 = _mm_add_ps( t0, _mm_mul_ps( _mm_splat_ps( a0, 1 ), b1 ) );
	t1 = _mm_add_ps( t1, _mm_mul_ps( _mm_splat_ps( a1, 1 ), b1 ) );
	t2 = _mm_add_ps( t2, _mm_mul_ps( _mm_splat_ps( a2, 1 ), b1 ) );
	t3 = _mm_add_ps( t3, _mm_mul_ps( _mm_splat_ps( a3, 1 ), b1 ) );
	
	t0 = _mm_add_ps( t0, _mm_mul_ps( _mm_splat_ps( a0, 2 ), b2 ) );
	t1 = _mm_add_ps( t1, _mm_mul_ps( _mm_splat_ps( a1, 2 ), b2 ) );
	t2 = _mm_add_ps( t2, _mm_mul_ps( _mm_splat_ps( a2, 2 ), b2 ) );
	t3 = _mm_add_ps( t3, _mm_mul_ps( _mm_splat_ps( a3, 2 ), b2 ) );
	
	t0 = _mm_add_ps( t0, _mm_mul_ps( _mm_splat_ps( a0, 3 ), b3 ) );
	t1 = _mm_add_ps( t1, _mm_mul_ps( _mm_splat_ps( a1, 3 ), b3 ) );
	t2 = _mm_add_ps( t2, _mm_mul_ps( _mm_splat_ps( a2, 3 ), b3 ) );
	t3 = _mm_add_ps( t3, _mm_mul_ps( _mm_splat_ps( a3, 3 ), b3 ) );
	
	_mm_storeu_ps( out + 0 * 4, t0 );
	_mm_storeu_ps( out + 1 * 4, t1 );
	_mm_storeu_ps( out + 2 * 4, t2 );
	_mm_storeu_ps( out + 3 * 4, t3 );
	
#else
	
	/*
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			out[ i * 4 + j ] =
				a[ i * 4 + 0 ] * b[ 0 * 4 + j ] +
				a[ i * 4 + 1 ] * b[ 1 * 4 + j ] +
				a[ i * 4 + 2 ] * b[ 2 * 4 + j ] +
				a[ i * 4 + 3 ] * b[ 3 * 4 + j ];
		}
	}
	*/
	
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
======================
R_MatrixTranspose
======================
*/
void R_MatrixTranspose( const float in[16], float out[16] )
{
	for( int i = 0; i < 4; i++ )
	{
		for( int j = 0; j < 4; j++ )
		{
			out[i * 4 + j] = in[j * 4 + i];
		}
	}
}

/*
==========================
R_TransformClipToDevice

Clip to normalized device coordinates
==========================
*/
void R_TransformClipToDevice( const idPlane& clip, idVec3& ndc )
{
	const float invW = 1.0f / clip[3];
	ndc[0] = clip[0] * invW;
	ndc[1] = clip[1] * invW;
	ndc[2] = clip[2] * invW;		// NOTE: in D3D this is in the range [0,1]
}

/*
==========================================================================================

WORLD/VIEW/PROJECTION MATRIX SETUP

==========================================================================================
*/

/*
======================
R_SetupViewMatrix

Sets up the world to view matrix for a given viewParm
======================
*/
void R_SetupViewMatrix( viewDef_t* viewDef )
{
	static float s_flipMatrix[16] =
	{
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		0, 0, -1, 0,
		-1, 0,  0, 0,
		0, 1,  0, 0,
		0, 0,  0, 1
	};
	
	viewEntity_t* world = &viewDef->worldSpace;
	memset( world, 0, sizeof( *world ) );
	
	// the model matrix is an identity
	world->modelMatrix[0 * 4 + 0] = 1.0f;
	world->modelMatrix[1 * 4 + 1] = 1.0f;
	world->modelMatrix[2 * 4 + 2] = 1.0f;
	
	// transform by the camera placement
	const idVec3& origin = viewDef->renderView.vieworg;
	const idMat3& axis = viewDef->renderView.viewaxis;
	
	float viewerMatrix[16];
	viewerMatrix[0 * 4 + 0] = axis[0][0];
	viewerMatrix[1 * 4 + 0] = axis[0][1];
	viewerMatrix[2 * 4 + 0] = axis[0][2];
	viewerMatrix[3 * 4 + 0] = - origin[0] * axis[0][0] - origin[1] * axis[0][1] - origin[2] * axis[0][2];
	
	viewerMatrix[0 * 4 + 1] = axis[1][0];
	viewerMatrix[1 * 4 + 1] = axis[1][1];
	viewerMatrix[2 * 4 + 1] = axis[1][2];
	viewerMatrix[3 * 4 + 1] = - origin[0] * axis[1][0] - origin[1] * axis[1][1] - origin[2] * axis[1][2];
	
	viewerMatrix[0 * 4 + 2] = axis[2][0];
	viewerMatrix[1 * 4 + 2] = axis[2][1];
	viewerMatrix[2 * 4 + 2] = axis[2][2];
	viewerMatrix[3 * 4 + 2] = - origin[0] * axis[2][0] - origin[1] * axis[2][1] - origin[2] * axis[2][2];
	
	viewerMatrix[0 * 4 + 3] = 0.0f;
	viewerMatrix[1 * 4 + 3] = 0.0f;
	viewerMatrix[2 * 4 + 3] = 0.0f;
	viewerMatrix[3 * 4 + 3] = 1.0f;
	
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	R_MatrixMultiply( viewerMatrix, s_flipMatrix, world->modelViewMatrix );
}

/*
======================
R_SetupProjectionMatrix

This uses the "infinite far z" trick
======================
*/
idCVar r_centerX( "r_centerX", "0", CVAR_FLOAT, "projection matrix center adjust" );
idCVar r_centerY( "r_centerY", "0", CVAR_FLOAT, "projection matrix center adjust" );

void R_SetupProjectionMatrix( viewDef_t* viewDef )
{
	// random jittering is usefull when multiple
	// frames are going to be blended together
	// for motion blurred anti-aliasing
	float jitterx, jittery;
	if( r_jitter.GetBool() )
	{
		static idRandom random;
		jitterx = random.RandomFloat();
		jittery = random.RandomFloat();
	}
	else
	{
		jitterx = 0.0f;
		jittery = 0.0f;
	}
	
	//
	// set up projection matrix
	//
	const float zNear = ( viewDef->renderView.cramZNear ) ? ( r_znear.GetFloat() * 0.25f ) : r_znear.GetFloat();
	
	float ymax = zNear * tan( viewDef->renderView.fov_y * idMath::PI / 360.0f );
	float ymin = -ymax;
	
	float xmax = zNear * tan( viewDef->renderView.fov_x * idMath::PI / 360.0f );
	float xmin = -xmax;
	
	const float width = xmax - xmin;
	const float height = ymax - ymin;
	
	const int viewWidth = viewDef->viewport.x2 - viewDef->viewport.x1 + 1;
	const int viewHeight = viewDef->viewport.y2 - viewDef->viewport.y1 + 1;
	
	jitterx = jitterx * width / viewWidth;
	jitterx += r_centerX.GetFloat();
	//k jitterx += viewDef->renderView.stereoScreenSeparation;
	xmin += jitterx * width;
	xmax += jitterx * width;
	
	jittery = jittery * height / viewHeight;
	jittery += r_centerY.GetFloat();
	ymin += jittery * height;
	ymax += jittery * height;
	
	viewDef->projectionMatrix[0 * 4 + 0] = 2.0f * zNear / width;
	viewDef->projectionMatrix[1 * 4 + 0] = 0.0f;
	viewDef->projectionMatrix[2 * 4 + 0] = ( xmax + xmin ) / width;	// normally 0
	viewDef->projectionMatrix[3 * 4 + 0] = 0.0f;
	
	viewDef->projectionMatrix[0 * 4 + 1] = 0.0f;
	viewDef->projectionMatrix[1 * 4 + 1] = 2.0f * zNear / height;
	viewDef->projectionMatrix[2 * 4 + 1] = ( ymax + ymin ) / height;	// normally 0
	viewDef->projectionMatrix[3 * 4 + 1] = 0.0f;
	
	// this is the far-plane-at-infinity formulation, and
	// crunches the Z range slightly so w=0 vertexes do not
	// rasterize right at the wraparound point
	viewDef->projectionMatrix[0 * 4 + 2] = 0.0f;
	viewDef->projectionMatrix[1 * 4 + 2] = 0.0f;
	viewDef->projectionMatrix[2 * 4 + 2] = -0.999f; // adjust value to prevent imprecision issues
	viewDef->projectionMatrix[3 * 4 + 2] = -2.0f * zNear;
	
	viewDef->projectionMatrix[0 * 4 + 3] = 0.0f;
	viewDef->projectionMatrix[1 * 4 + 3] = 0.0f;
	viewDef->projectionMatrix[2 * 4 + 3] = -1.0f;
	viewDef->projectionMatrix[3 * 4 + 3] = 0.0f;

#if 0 //k
	if( viewDef->renderView.flipProjection )
	{
		viewDef->projectionMatrix[1 * 4 + 1] = -viewDef->projectionMatrix[1 * 4 + 1];
		viewDef->projectionMatrix[1 * 4 + 3] = -viewDef->projectionMatrix[1 * 4 + 3];
	}
#endif
}


// RB: standard OpenGL projection matrix
void R_SetupProjectionMatrix2( const viewDef_t* viewDef, const float zNear, const float zFar, float projectionMatrix[16] )
{
	float ymax = zNear * tan( viewDef->renderView.fov_y * idMath::PI / 360.0f );
	float ymin = -ymax;
	
	float xmax = zNear * tan( viewDef->renderView.fov_x * idMath::PI / 360.0f );
	float xmin = -xmax;
	
	const float width = xmax - xmin;
	const float height = ymax - ymin;
	
	const int viewWidth = viewDef->viewport.x2 - viewDef->viewport.x1 + 1;
	const int viewHeight = viewDef->viewport.y2 - viewDef->viewport.y1 + 1;
	
	float jitterx, jittery;
	jitterx = 0.0f;
	jittery = 0.0f;
	jitterx = jitterx * width / viewWidth;
	jitterx += r_centerX.GetFloat();
	//k jitterx += viewDef->renderView.stereoScreenSeparation;
	xmin += jitterx * width;
	xmax += jitterx * width;
	
	jittery = jittery * height / viewHeight;
	jittery += r_centerY.GetFloat();
	ymin += jittery * height;
	ymax += jittery * height;
	
	float depth = zFar - zNear;
	
	projectionMatrix[0 * 4 + 0] = 2.0f * zNear / width;
	projectionMatrix[1 * 4 + 0] = 0.0f;
	projectionMatrix[2 * 4 + 0] = ( xmax + xmin ) / width;	// normally 0
	projectionMatrix[3 * 4 + 0] = 0.0f;
	
	projectionMatrix[0 * 4 + 1] = 0.0f;
	projectionMatrix[1 * 4 + 1] = 2.0f * zNear / height;
	projectionMatrix[2 * 4 + 1] = ( ymax + ymin ) / height;	// normally 0
	projectionMatrix[3 * 4 + 1] = 0.0f;
	
	projectionMatrix[0 * 4 + 2] = 0.0f;
	projectionMatrix[1 * 4 + 2] = 0.0f;
	projectionMatrix[2 * 4 + 2] =  -( zFar + zNear ) / depth;		// -0.999f; // adjust value to prevent imprecision issues
	projectionMatrix[3 * 4 + 2] = -2 * zFar * zNear / depth;	// -2.0f * zNear;
	
	projectionMatrix[0 * 4 + 3] = 0.0f;
	projectionMatrix[1 * 4 + 3] = 0.0f;
	projectionMatrix[2 * 4 + 3] = -1.0f;
	projectionMatrix[3 * 4 + 3] = 0.0f;

#if 0 //k
	if( viewDef->renderView.flipProjection )
	{
		projectionMatrix[1 * 4 + 1] = -viewDef->projectionMatrix[1 * 4 + 1];
		projectionMatrix[1 * 4 + 3] = -viewDef->projectionMatrix[1 * 4 + 3];
	}
#endif
}


void R_MatrixFullInverse( const float a[16], float r[16] )
{
	idMat4	am;
	
	for( int i = 0 ; i < 4 ; i++ )
	{
		for( int j = 0 ; j < 4 ; j++ )
		{
			am[i][j] = a[j * 4 + i];
		}
	}
	
//	idVec4 test( 100, 100, 100, 1 );
//	idVec4	transformed, inverted;
//	transformed = test * am;

	if( !am.InverseSelf() )
	{
		common->Error( "Invert failed" );
	}
//	inverted = transformed * am;

	for( int i = 0 ; i < 4 ; i++ )
	{
		for( int j = 0 ; j < 4 ; j++ )
		{
			r[j * 4 + i] = am[i][j];
		}
	}
}
// RB end