// Copyright (C) 2007 Id Software, Inc.
//

/*
================================================================================================================================
================================================================================================================================
WARNING: This is included by the radiant project as well... don't try to use any gamecode stuff in here
================================================================================================================================
================================================================================================================================
*/


#include "../precompiled.h"
#pragma hdrstop

#include "UserInterfaceManager.h"
#include "GuiSurfaceRenderable.h"
#include "../gamesys/Pvs.h"
#include "../../renderer/DeviceContext.h"

/*
===============================================================================

sdGuiSurfaceRenderable

===============================================================================
*/

/*
============
sdGuiSurfaceRenderable::sdGuiSurfaceRenderable
============
*/
sdGuiSurfaceRenderable::sdGuiSurfaceRenderable() {
}

/*
============
sdGuiSurfaceRenderable::~sdGuiSurfaceRenderable
============
*/
sdGuiSurfaceRenderable::~sdGuiSurfaceRenderable() {
	Clear();
}

/*
============
sdGuiSurfaceRenderable::Clear
============
*/
void sdGuiSurfaceRenderable::Clear() {
	if ( handle.IsValid() ) {
		uiManager->FreeUserInterface( handle );
		handle.Release();
	}
}

/*
============
sdGuiSurfaceRenderable::Init
============
*/
void sdGuiSurfaceRenderable::Init( const guiSurface_t& surface, const guiHandle_t& handle, const int allowInViewID, const bool weaponDepthHack ) {
	this->surface = surface;
	this->handle = handle;
	this->allowInViewID = allowInViewID;
	this->weaponDepthHack = weaponDepthHack;

	axisLenSquared[0] = surface.axis[0].LengthSqr();
	axisLenSquared[1] = surface.axis[1].LengthSqr();
}

#if !defined( MONOLITHIC )
// TEMP TEMP
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

#else

void R_AxisToModelMatrix( const idMat3 &axis, const idVec3 &origin, float modelMatrix[16] );

#endif

void R_ModelMatrixToAxis( const float modelMatrix[16], idMat3 &axis, idVec3 &origin ) {
	axis[0][0] = modelMatrix[0];
	axis[0][1] = modelMatrix[1];
	axis[0][2] = modelMatrix[2];

	axis[1][0] = modelMatrix[4];
	axis[1][1] = modelMatrix[5];
	axis[1][2] = modelMatrix[6];

	axis[2][0] = modelMatrix[8];
	axis[2][1] = modelMatrix[9];
	axis[2][2] = modelMatrix[10];

	origin[0]  = modelMatrix[12];			   
	origin[1]  = modelMatrix[13];			   
	origin[2]  = modelMatrix[14];
}


/*
==========================
R_MultMatrixAligned
==========================
*/
void R_MultMatrixAligned( const float a[16], const float b[16], float output[16] ) {
#if 0
	int		i, j;

	for ( i = 0 ; i < 4 ; i++ ) {
		for ( j = 0 ; j < 4 ; j++ ) {
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
		}
	}
#elif defined( ID_WIN_X86_SSE )
	__asm {
		mov edx, a
		mov eax, output
		mov ecx, b
		movss xmm0, dword ptr [edx]
		movaps xmm1, xmmword ptr [ecx]
		shufps xmm0, xmm0, 0
		movss xmm2, dword ptr [edx+4]
		mulps xmm0, xmm1
		shufps xmm2, xmm2, 0
		movaps xmm3, xmmword ptr [ecx+10h]
		movss xmm7, dword ptr [edx+8]
		mulps xmm2, xmm3
		shufps xmm7, xmm7, 0
		addps xmm0, xmm2
		movaps xmm4, xmmword ptr [ecx+20h]
		movss xmm2, dword ptr [edx+0Ch]
		mulps xmm7, xmm4
		shufps xmm2, xmm2, 0
		addps xmm0, xmm7
		movaps xmm5, xmmword ptr [ecx+30h]
		movss xmm6, dword ptr [edx+10h]
		mulps xmm2, xmm5
		movss xmm7, dword ptr [edx+14h]
		shufps xmm6, xmm6, 0
		addps xmm0, xmm2
		shufps xmm7, xmm7, 0
		movlps qword ptr [eax], xmm0
		movhps qword ptr [eax+8], xmm0
		mulps xmm7, xmm3
		movss xmm0, dword ptr [edx+18h]
		mulps xmm6, xmm1
		shufps xmm0, xmm0, 0
		addps xmm6, xmm7
		mulps xmm0, xmm4
		movss xmm2, dword ptr [edx+24h]
		addps xmm6, xmm0
		movss xmm0, dword ptr [edx+1Ch]
		movss xmm7, dword ptr [edx+20h]
		shufps xmm0, xmm0, 0
		shufps xmm7, xmm7, 0
		mulps xmm0, xmm5
		mulps xmm7, xmm1
		addps xmm6, xmm0
		shufps xmm2, xmm2, 0
		movlps qword ptr [eax+10h], xmm6
		movhps qword ptr [eax+18h], xmm6
		mulps xmm2, xmm3
		movss xmm6, dword ptr [edx+28h]
		addps xmm7, xmm2
		shufps xmm6, xmm6, 0
		movss xmm2, dword ptr [edx+2Ch]
		mulps xmm6, xmm4
		shufps xmm2, xmm2, 0
		addps xmm7, xmm6
		mulps xmm2, xmm5
		movss xmm0, dword ptr [edx+34h]
		addps xmm7, xmm2
		shufps xmm0, xmm0, 0
		movlps qword ptr [eax+20h], xmm7
		movss xmm2, dword ptr [edx+30h]
		movhps qword ptr [eax+28h], xmm7
		mulps xmm0, xmm3
		shufps xmm2, xmm2, 0
		movss xmm6, dword ptr [edx+38h]
		mulps xmm2, xmm1
		shufps xmm6, xmm6, 0
		addps xmm2, xmm0
		mulps xmm6, xmm4
		movss xmm7, dword ptr [edx+3Ch]
		shufps xmm7, xmm7, 0
		addps xmm2, xmm6
		mulps xmm7, xmm5
		addps xmm2, xmm7
		movaps xmmword ptr [eax+30h], xmm2
	}
#else
	output[0*4+0] = a[0*4+0]*b[0*4+0] + a[0*4+1]*b[1*4+0] + a[0*4+2]*b[2*4+0] + a[0*4+3]*b[3*4+0];
	output[0*4+1] = a[0*4+0]*b[0*4+1] + a[0*4+1]*b[1*4+1] + a[0*4+2]*b[2*4+1] + a[0*4+3]*b[3*4+1];
	output[0*4+2] = a[0*4+0]*b[0*4+2] + a[0*4+1]*b[1*4+2] + a[0*4+2]*b[2*4+2] + a[0*4+3]*b[3*4+2];
	output[0*4+3] = a[0*4+0]*b[0*4+3] + a[0*4+1]*b[1*4+3] + a[0*4+2]*b[2*4+3] + a[0*4+3]*b[3*4+3];
	output[1*4+0] = a[1*4+0]*b[0*4+0] + a[1*4+1]*b[1*4+0] + a[1*4+2]*b[2*4+0] + a[1*4+3]*b[3*4+0];
	output[1*4+1] = a[1*4+0]*b[0*4+1] + a[1*4+1]*b[1*4+1] + a[1*4+2]*b[2*4+1] + a[1*4+3]*b[3*4+1];
	output[1*4+2] = a[1*4+0]*b[0*4+2] + a[1*4+1]*b[1*4+2] + a[1*4+2]*b[2*4+2] + a[1*4+3]*b[3*4+2];
	output[1*4+3] = a[1*4+0]*b[0*4+3] + a[1*4+1]*b[1*4+3] + a[1*4+2]*b[2*4+3] + a[1*4+3]*b[3*4+3];
	output[2*4+0] = a[2*4+0]*b[0*4+0] + a[2*4+1]*b[1*4+0] + a[2*4+2]*b[2*4+0] + a[2*4+3]*b[3*4+0];
	output[2*4+1] = a[2*4+0]*b[0*4+1] + a[2*4+1]*b[1*4+1] + a[2*4+2]*b[2*4+1] + a[2*4+3]*b[3*4+1];
	output[2*4+2] = a[2*4+0]*b[0*4+2] + a[2*4+1]*b[1*4+2] + a[2*4+2]*b[2*4+2] + a[2*4+3]*b[3*4+2];
	output[2*4+3] = a[2*4+0]*b[0*4+3] + a[2*4+1]*b[1*4+3] + a[2*4+2]*b[2*4+3] + a[2*4+3]*b[3*4+3];
	output[3*4+0] = a[3*4+0]*b[0*4+0] + a[3*4+1]*b[1*4+0] + a[3*4+2]*b[2*4+0] + a[3*4+3]*b[3*4+0];
	output[3*4+1] = a[3*4+0]*b[0*4+1] + a[3*4+1]*b[1*4+1] + a[3*4+2]*b[2*4+1] + a[3*4+3]*b[3*4+1];
	output[3*4+2] = a[3*4+0]*b[0*4+2] + a[3*4+1]*b[1*4+2] + a[3*4+2]*b[2*4+2] + a[3*4+3]*b[3*4+2];
	output[3*4+3] = a[3*4+0]*b[0*4+3] + a[3*4+1]*b[1*4+3] + a[3*4+2]*b[2*4+3] + a[3*4+3]*b[3*4+3];
#endif
}

// TEMP TEMP

/*
============
sdGuiSurfaceRenderable::DrawCulled
============
*/
void sdGuiSurfaceRenderable::DrawCulled( const idFrustum& viewFrustum ) const {
	// cull if view is behind the gui plane
	idVec3 localViewOrigin = axis.TransposeMultiply( viewFrustum.GetOrigin() - origin );

	if ( surface.plane.Side( localViewOrigin, 0.1f ) == SIDE_BACK ) {
		return;
	}

	// calculate worldspace bounds
	idBounds worldBounds;

	worldBounds.FromTransformedBounds( surface.bounds, origin, axis );

	// frustum cull
	if ( viewFrustum.CullBounds( worldBounds ) ) {
		return;
	}

	Draw();
}

#if !defined( _EDITWORLD )
/*
============
sdGuiSurfaceRenderable::DrawCulled
============
*/
void sdGuiSurfaceRenderable::DrawCulled( const idPVS& pvs, const pvsHandle_t pvsHandle, const idFrustum& viewFrustum ) const {
	// cull if view is behind the gui plane
	idVec3 localViewOrigin = axis.TransposeMultiply( viewFrustum.GetOrigin() - origin );

	if ( surface.plane.Side( localViewOrigin, 0.1f ) == SIDE_BACK ) {
		return;
	}

	// calculate worldspace bounds
	idBounds worldBounds;

	worldBounds.FromTransformedBounds( surface.bounds, origin, axis );

	// cull by pvs
	if ( !pvs.InCurrentPVS( pvsHandle, worldBounds ) ) {
		return;
	}

	// frustum cull
	if ( viewFrustum.CullBounds( worldBounds ) ) {
		return;
	}

	Draw();
}
#endif /* !_EDITWORLD */

/*
============
sdGuiSurfaceRenderable::Draw
============
*/
void sdGuiSurfaceRenderable::Draw() const {
	sdUserInterface* ui = uiManager->GetUserInterface( handle );

	if ( ui != NULL ) {
		ALIGN16(float entityModelMatrix[16]);

		R_AxisToModelMatrix( axis, origin, entityModelMatrix );

		// setup the space
		ALIGN16( float guiModelMatrix[16] );
		ALIGN16( float modelMatrix[16] );

		guiModelMatrix[0] = surface.axis[0][0] / static_cast< float >( SCREEN_WIDTH );
		guiModelMatrix[4] = surface.axis[1][0] / static_cast< float >( SCREEN_HEIGHT );
		guiModelMatrix[8] = surface.axis[2][0];
		guiModelMatrix[12] = surface.origin[0];

		guiModelMatrix[1] = surface.axis[0][1] / static_cast< float >( SCREEN_WIDTH );
		guiModelMatrix[5] = surface.axis[1][1] / static_cast< float >( SCREEN_HEIGHT );
		guiModelMatrix[9] = surface.axis[2][1];
		guiModelMatrix[13] = surface.origin[1];

		guiModelMatrix[2] = surface.axis[0][2] / static_cast< float >( SCREEN_WIDTH );
		guiModelMatrix[6] = surface.axis[1][2] / static_cast< float >( SCREEN_HEIGHT );
		guiModelMatrix[10] = surface.axis[2][2];
		guiModelMatrix[14] = surface.origin[2];

		guiModelMatrix[3] = 0.0f;
		guiModelMatrix[7] = 0.0f;
		guiModelMatrix[11] = 0.0f;
		guiModelMatrix[15] = 1.0f;

		R_MultMatrixAligned( guiModelMatrix, entityModelMatrix, modelMatrix );

		deviceContext->BeginEmitToCurrentView( modelMatrix, allowInViewID, weaponDepthHack );

		// emit the drawing commands
		ui->Draw();

		deviceContext->End();
	}
}

/*
============
sdGuiSurfaceRenderable::Trace
============
*/
guiPoint_t sdGuiSurfaceRenderable::Trace( const idVec3& start, const idVec3& end ) const {
	idVec3		localDir, localStart, localEnd;
    guiPoint_t	pt;

	pt.x = pt.y = -1;

	// compute distance of start to gui plane
	localStart = axis.TransposeMultiply( start - origin );

	float d1 = surface.plane.Distance( localStart );
	if ( d1 < 0.0f ) {
		return pt;
	}

	// compute distance of end to gui plane
	localEnd = axis.TransposeMultiply( end - origin );

	float d2 = surface.plane.Distance( localEnd );
	if ( d2 >= 0.0f ) {
		return pt;
	}

	float denom = d1 - d2;
	float t = d1 / denom;
	
	pt.worldPos = localStart + t * ( localEnd - localStart );

	// compute the barycentric coordinates
 	for ( int i = 0; i < surface.numTris; i++ ) {
		float u = surface.edgePlanes[ i ][ 0 ].Distance( pt.worldPos );
		if ( u < 0.0f || u > 1.0f  ) {
			continue;
		}

		float v = surface.edgePlanes[ i ][ 1 ].Distance( pt.worldPos );
		if ( v < 0.0f ) {
			continue;
		}

		float w = 1.0f - u - v;
		if ( w < 0.0f ) {
			continue;
		}

		// segment intersects tri at distance t in position s
		idVec3 cursor = pt.worldPos - surface.origin;

		pt.x = ( cursor * surface.axis[0] ) / axisLenSquared[0];
		pt.y = ( cursor * surface.axis[1] ) / axisLenSquared[1];

		break;
	}

	return pt;
}

/*
============
sdGuiSurfaceRenderable::GetGuiNum
============
*/
int sdGuiSurfaceRenderable::GetGuiNum() const {
	return surface.guiNum;
}

/*
============
sdGuiSurfaceRenderable::GetGuiHandle
============
*/
guiHandle_t sdGuiSurfaceRenderable::GetGuiHandle() const {
	return handle;
}
