// Copyright (C) 2007 Id Software, Inc.
//


#include "../../renderer/DeviceContext.h"

#ifndef __GAME_MISC_WORLDTOSCREEN_H__
#define __GAME_MISC_WORLDTOSCREEN_H__

/*
============
sdWorldToScreenConverter
============
*/
class sdWorldToScreenConverter {
public:
	sdWorldToScreenConverter( const renderView_t& view );
	sdWorldToScreenConverter( float* projectionMatrix, float* modelViewMatrix );

	void					Transform( const idVec3& src, idVec2& dst ) const;
	void					Transform( const idBounds& src, const idMat3& axes, const idVec3& origin, sdBounds2D& dst ) const;

	void					Setup( const renderView_t& view );
	void					Setup( float* projectionMatrix, float* modelViewMatrix );

	void					SetExtents( const idVec2& _extents ) { extents = _extents; }

	static bool				TransformClipped( const idBounds &bounds, const idMat3& axes, const idVec3& origin, sdBounds2D& dst, const idFrustum& frustum, const idVec2& extents );

protected:
	float					projectionMatrixArray[ 16 ];
	float					modelViewMatrixArray[ 16 ];

	float*					projectionMatrix;
	float*					modelViewMatrix;

	idVec2					extents;
};


/*
======================
sdWorldToScreenConverter::TransformClipped
======================
*/
ID_INLINE bool sdWorldToScreenConverter::TransformClipped( const idBounds &bounds, const idMat3& axes, const idVec3& origin, sdBounds2D& dst, const idFrustum& frustum, const idVec2& extents ) {
	idBounds temp;
	if ( !frustum.ProjectionBounds( idBox( bounds, origin, axes ), temp ) ) {
		return false;
	}

	dst.GetMins().x = 0.5f * ( 1.0f - temp[ 1 ].y ) * extents.x;
	dst.GetMaxs().x = 0.5f * ( 1.0f - temp[ 0 ].y ) * extents.x;

	dst.GetMins().y = 0.5f * ( 1.0f - temp[ 1 ].z ) * extents.y;
	dst.GetMaxs().y = 0.5f * ( 1.0f - temp[ 0 ].z ) * extents.y;

	return true;
}


/*
==========================
sdWorldToScreenConverter::sdWorldToScreenConverter
==========================
*/
ID_INLINE sdWorldToScreenConverter::sdWorldToScreenConverter( const renderView_t& view ) {
	const float width = SCREEN_WIDTH * 1.0f / deviceContext->GetAspectRatioCorrection();
	extents.Set( width, SCREEN_HEIGHT );
	Setup( view );
}

/*
==========================
sdWorldToScreenConverter::sdWorldToScreenConverter
==========================
*/
ID_INLINE sdWorldToScreenConverter::sdWorldToScreenConverter( float* projectionMatrix, float* modelViewMatrix ) {
	const float width = SCREEN_WIDTH * 1.0f / deviceContext->GetAspectRatioCorrection();
	extents.Set( width, SCREEN_HEIGHT );
	Setup( projectionMatrix, modelViewMatrix );
}

/*
==========================
sdWorldToScreenConverter::Transform
==========================
*/
ID_INLINE void sdWorldToScreenConverter::Transform( const idVec3& src, idVec2& dst ) const {
	int i;
	idPlane eye, clip;

	for ( i = 0 ; i < 4 ; i++ ) {
		eye[i] = 
			src[0] * modelViewMatrix[ i + 0 * 4 ] +
			src[1] * modelViewMatrix[ i + 1 * 4 ] +
			src[2] * modelViewMatrix[ i + 2 * 4 ] +
			modelViewMatrix[ i + 3 * 4 ];
	}

	for ( i = 0 ; i < 4 ; i++ ) {
		clip[i] = 
			eye[0] * projectionMatrix[ i + 0 * 4 ] +
			eye[1] * projectionMatrix[ i + 1 * 4 ] +
			eye[2] * projectionMatrix[ i + 2 * 4 ] +
			eye[3] * projectionMatrix[ i + 3 * 4 ];
	}

	if( clip[ 3 ] <= 0.01f ) {
		clip[ 3 ] = 0.01f;
	}

	dst[ 0 ] = clip[ 0 ] / clip[ 3 ];
	dst[ 1 ] = clip[ 1 ] / clip[ 3 ];

	dst[ 0 ] = ( 0.5f * ( 1.0f + dst[ 0 ] ) * extents[ 0 ] );
	dst[ 1 ] = ( 0.5f * ( 1.0f - dst[ 1 ] ) * extents[ 1 ] );
}

/*
==========================
sdWorldToScreenConverter::Transform
==========================
*/
ID_INLINE void sdWorldToScreenConverter::Transform( const idBounds& src, const idMat3& axes, const idVec3& origin, sdBounds2D& dst ) const {
	idVec3 point;
	idVec2 screenPoint;

	dst.Clear();

	int i;
	for ( i = 0; i < 8; i++ ) {
		point[ 0 ] = src[ ( i & 1 ) >> 0 ][ 0 ];
		point[ 1 ] = src[ ( i & 2 ) >> 1 ][ 1 ];
		point[ 2 ] = src[ ( i & 4 ) >> 2 ][ 2 ];

		point *= axes;
		point += origin;

		Transform( point, screenPoint );

		dst.AddPoint( screenPoint );
	}
}

/*
==========================
sdWorldToScreenConverter::Setup
==========================
*/
ID_INLINE void sdWorldToScreenConverter::Setup( const renderView_t& view ) {
	projectionMatrix = projectionMatrixArray;
	modelViewMatrix = modelViewMatrixArray;
	gameRenderWorld->SetupMatrices( &view, projectionMatrix, modelViewMatrix, false );
}

/*
==========================
sdWorldToScreenConverter::Setup
==========================
*/
ID_INLINE void sdWorldToScreenConverter::Setup( float* projectionMatrix, float* modelViewMatrix ) {
	this->projectionMatrix = projectionMatrix;
	this->modelViewMatrix = modelViewMatrix;
}

#endif // __GAME_MISC_WORLDTOSCREEN_H__
