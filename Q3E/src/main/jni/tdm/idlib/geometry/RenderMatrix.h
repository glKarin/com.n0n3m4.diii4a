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
#ifndef __RENDERMATRIX_H__
#define __RENDERMATRIX_H__

static const int NUM_FRUSTUM_CORNERS	= 8;

struct frustumCorners_t
{
	float	x[NUM_FRUSTUM_CORNERS];
	float	y[NUM_FRUSTUM_CORNERS];
	float	z[NUM_FRUSTUM_CORNERS];
};

enum frustumCull_t
{
	FRUSTUM_CULL_FRONT		= 1,
	FRUSTUM_CULL_BACK		= 2,
	FRUSTUM_CULL_CROSS		= 3
};

/*
================================================================================================

idRenderMatrix

This is a row-major matrix and transforms are applied with left-multiplication.

stgatilov #6279: This is the main type of matrix from Doom 3 BFG renderer.
It is row-major, i.e. it is indexed like M[row][column], and each row occupies float[4] block of memory.
To transform a vector, put it as column-vector on the right of the matrix * vector product.
Naturally, in order to apply matrix A first and matrix B then, compute Multiply(B, A).

================================================================================================
*/
class idRenderMatrix
{
public:
	idRenderMatrix() {}
	ID_INLINE				idRenderMatrix(	float a0, float a1, float a2, float a3,
											float b0, float b1, float b2, float b3,
											float c0, float c1, float c2, float c3,
											float d0, float d1, float d2, float d3 );
											
	ID_FORCE_INLINE const float* 			operator[]( int index ) const
	{
		assert( index >= 0 && index < 4 );
		return &m[index * 4];
	}
	ID_FORCE_INLINE float* 					operator[]( int index )
	{
		assert( index >= 0 && index < 4 );
		return &m[index * 4];
	}
	
	void					Zero()
	{
		memset( m, 0, sizeof( m ) );
	}
	ID_INLINE void			Identity();
	
	// Matrix classification (only meant to be used for asserts).
	ID_INLINE bool			IsZero( float epsilon ) const;
	ID_INLINE bool			IsIdentity( float epsilon ) const;
	ID_INLINE bool			IsAffineTransform( float epsilon ) const;
	ID_INLINE bool			IsUniformScale( float epsilon ) const;
	
	// Transform a point.
	// NOTE: the idVec3 out variant does not divide by W.
	ID_INLINE void			TransformPoint( const idVec3& in, idVec3& out ) const;
	ID_INLINE void			TransformPoint( const idVec3& in, idVec4& out ) const;
	ID_INLINE void			TransformPoint( const idVec4& in, idVec4& out ) const;
	
	// These assume the matrix has no non-uniform scaling or shearing.
	// NOTE: a direction will only stay normalized if the matrix has no skewing or scaling.
	ID_INLINE void			TransformDir( const idVec3& in, idVec3& out, bool normalize ) const;
	ID_INLINE void			TransformPlane( const idPlane& in, idPlane& out, bool normalize ) const;
	
	// These transforms work with non-uniform scaling and shearing by multiplying
	// with 'transpose(inverse(M))' where this matrix is assumed to be 'inverse(M)'.
	ID_INLINE void			InverseTransformDir( const idVec3& in, idVec3& out, bool normalize ) const;
	ID_INLINE void			InverseTransformPlane( const idPlane& in, idPlane& out, bool normalize ) const;
	
	// Project a point.
	static ID_INLINE void	TransformModelToClip( const idVec3& src, const idRenderMatrix& modelMatrix, const idRenderMatrix& projectionMatrix, idVec4& eye, idVec4& clip );
	static ID_INLINE void	TransformClipToDevice( const idVec4& clip, idVec3& ndc );
	
	// Create a matrix that goes from local space to the space defined by the 'origin' and 'axis'.
	static void				CreateFromOriginAxis( const idVec3& origin, const idMat3& axis, idRenderMatrix& out );
	static void				CreateFromOriginAxisScale( const idVec3& origin, const idMat3& axis, const idVec3& scale, idRenderMatrix& out );
	
	// Create a matrix that goes from a global coordinate to a view coordinate (OpenGL looking down -Z) based on the given view origin/axis.
	static void				CreateViewMatrix( const idVec3& origin, const idMat3& axis, idRenderMatrix& out );
	
	// Create a projection matrix.
	static void				CreateProjectionMatrix( float xMin, float xMax, float yMin, float yMax, float zNear, float zFar, idRenderMatrix& out );
	static void				CreateProjectionMatrixFov( float xFovDegrees, float yFovDegrees, float zNear, float zFar, float xOffset, float yOffset, idRenderMatrix& out );
	
	// Apply depth hacks to a projection matrix.
	static ID_INLINE void	ApplyDepthHack( idRenderMatrix& src );
	static ID_INLINE void	ApplyModelDepthHack( idRenderMatrix& src, float value );
	
	// Offset and scale the given matrix such that the result matrix transforms the unit-cube to exactly cover the given bounds (and the inverse).
	static void				OffsetScaleForBounds( const idRenderMatrix& src, const idBounds& bounds, idRenderMatrix& out );
	static void				InverseOffsetScaleForBounds( const idRenderMatrix& src, const idBounds& bounds, idRenderMatrix& out );
	
	// Basic matrix operations.
	static void				Transpose( const idRenderMatrix& src, idRenderMatrix& out );
	static void				Multiply( const idRenderMatrix& a, const idRenderMatrix& b, idRenderMatrix& out );
	static bool				Inverse( const idRenderMatrix& src, idRenderMatrix& out );
	static void				InverseByTranspose( const idRenderMatrix& src, idRenderMatrix& out );
	static bool				InverseByDoubles( const idRenderMatrix& src, idRenderMatrix& out );
	
	// Copy or create a matrix that is stored directly into four float4 vectors which is useful for directly setting vertex program uniforms.
	static void				CopyMatrix( const idRenderMatrix& matrix, idVec4& row0, idVec4& row1, idVec4& row2, idVec4& row3 );
	static void				SetMVP( const idRenderMatrix& mvp, idVec4& row0, idVec4& row1, idVec4& row2, idVec4& row3, bool& negativeDeterminant );
	static void				SetMVPForBounds( const idRenderMatrix& mvp, const idBounds& bounds, idVec4& row0, idVec4& row1, idVec4& row2, idVec4& row3, bool& negativeDeterminant );
	static void				SetMVPForInverseProject( const idRenderMatrix& mvp, const idRenderMatrix& inverseProject, idVec4& row0, idVec4& row1, idVec4& row2, idVec4& row3, bool& negativeDeterminant );
	
	// Cull to a Model-View-Projection (MVP) matrix.
	static bool				CullPointToMVP( const idRenderMatrix& mvp, const idVec3& point, bool zeroToOne = false );
	static bool				CullPointToMVPbits( const idRenderMatrix& mvp, const idVec3& point, byte* outBits, bool zeroToOne = false );
	static bool				CullBoundsToMVP( const idRenderMatrix& mvp, const idBounds& bounds, bool zeroToOne = false );
	static bool				CullBoundsToMVPbits( const idRenderMatrix& mvp, const idBounds& bounds, byte* outBits, bool zeroToOne = false );
	static bool				CullExtrudedBoundsToMVP( const idRenderMatrix& mvp, const idBounds& bounds, const idVec3& extrudeDirection, const idPlane& clipPlane, bool zeroToOne = false );
	static bool				CullExtrudedBoundsToMVPbits( const idRenderMatrix& mvp, const idBounds& bounds, const idVec3& extrudeDirection, const idPlane& clipPlane, byte* outBits, bool zeroToOne = false );
	
	// Calculate the projected bounds.
	static void				ProjectedBounds( idBounds& projected, const idRenderMatrix& mvp, const idBounds& bounds, bool windowSpace = true );
	static void				ProjectedNearClippedBounds( idBounds& projected, const idRenderMatrix& mvp, const idBounds& bounds, bool windowSpace = true );
	static void				ProjectedFullyClippedBounds( idBounds& projected, const idRenderMatrix& mvp, const idBounds& bounds, bool windowSpace = true );
	
	// Calculate the projected depth bounds.
	static void				DepthBoundsForBounds( float& min, float& max, const idRenderMatrix& mvp, const idBounds& bounds, bool windowSpace = true );
	static void				DepthBoundsForExtrudedBounds( float& min, float& max, const idRenderMatrix& mvp, const idBounds& bounds, const idVec3& extrudeDirection, const idPlane& clipPlane, bool windowSpace = true );
	static void				DepthBoundsForShadowBounds( float& min, float& max, const idRenderMatrix& mvp, const idBounds& bounds, const idVec3& localLightOrigin, bool windowSpace = true );
	
	// Returns eye Z coordinate with reversed sign (monotonically increasing with depth).
	// In other words, it is eye-fragment distance along view direction.
	// Projection matrix is read from "this" matrix.
	ID_INLINE float			DepthToZ( float depth ) const;

	// Create frustum planes and corners from a matrix.
	static void				GetFrustumPlanes( idPlane planes[6], const idRenderMatrix& frustum, bool zeroToOne, bool normalize );
	static void				GetFrustumCorners( frustumCorners_t& corners, const idRenderMatrix& frustumTransform, const idBounds& frustumBounds );
	static frustumCull_t	CullFrustumCornersToPlane( const frustumCorners_t& corners, const idPlane& plane );
	
	// stgatilov #5886: This code is similar to CullBoundsToMVPbits, but:
	//  1) It accepts six inward-looking planes instead of frustum MVP matrix.
	//  2) It produces two bitmasks: whole box outside plane / some point in box is outside
	//  3) It is faster, although it needs preparation for maximum performance.
	struct CullSixPlanes {
		ALIGNTYPE16 float prep[12][4];
		void Prepare( const idPlane frustumPlanes[6] );
		void CullBounds( const idPlane frustumPlanes[6], const idBounds& bounds, byte* allOutBits, byte* anyOutBits ) const;
	};

	// stgatilov #6296: This code is optimized version of R_CullFrustumSphere / R_RadiusCullLocalBox
	struct CullSixPlanes2 {
		ALIGNTYPE16 float prep[8][4];
		void Prepare( const idPlane frustumPlanes[6] );
		bool CullSphere( const idPlane frustumPlanes[6], const idSphere& sphere ) const;
	};

private:
	float					m[16];
};

extern const idRenderMatrix renderMatrix_identity;
extern const idRenderMatrix renderMatrix_flipToOpenGL;
extern const idRenderMatrix renderMatrix_windowSpaceToClipSpace;
// RB begin
extern const idRenderMatrix renderMatrix_clipSpaceToWindowSpace;
// RB end

/*
========================
idRenderMatrix::idRenderMatrix
========================
*/
ID_INLINE idRenderMatrix::idRenderMatrix(	float a0, float a1, float a2, float a3,
		float b0, float b1, float b2, float b3,
		float c0, float c1, float c2, float c3,
		float d0, float d1, float d2, float d3 )
{
	m[0 * 4 + 0] = a0;
	m[0 * 4 + 1] = a1;
	m[0 * 4 + 2] = a2;
	m[0 * 4 + 3] = a3;
	m[1 * 4 + 0] = b0;
	m[1 * 4 + 1] = b1;
	m[1 * 4 + 2] = b2;
	m[1 * 4 + 3] = b3;
	m[2 * 4 + 0] = c0;
	m[2 * 4 + 1] = c1;
	m[2 * 4 + 2] = c2;
	m[2 * 4 + 3] = c3;
	m[3 * 4 + 0] = d0;
	m[3 * 4 + 1] = d1;
	m[3 * 4 + 2] = d2;
	m[3 * 4 + 3] = d3;
}

/*
========================
idRenderMatrix::Identity
========================
*/
ID_INLINE void idRenderMatrix::Identity()
{
	m[0 * 4 + 0] = 1.0f;
	m[0 * 4 + 1] = 0.0f;
	m[0 * 4 + 2] = 0.0f;
	m[0 * 4 + 3] = 0.0f;
	
	m[1 * 4 + 0] = 0.0f;
	m[1 * 4 + 1] = 1.0f;
	m[1 * 4 + 2] = 0.0f;
	m[1 * 4 + 3] = 0.0f;
	
	m[2 * 4 + 0] = 0.0f;
	m[2 * 4 + 1] = 0.0f;
	m[2 * 4 + 2] = 1.0f;
	m[2 * 4 + 3] = 0.0f;
	
	m[3 * 4 + 0] = 0.0f;
	m[3 * 4 + 1] = 0.0f;
	m[3 * 4 + 2] = 0.0f;
	m[3 * 4 + 3] = 1.0f;
}

/*
========================
idRenderMatrix::IsZero
========================
*/
ID_INLINE bool idRenderMatrix::IsZero( float epsilon ) const
{
	for( int i = 0; i < 16; i++ )
	{
		if( idMath::Fabs( m[i] ) > epsilon )
		{
			return false;
		}
	}
	return true;
}

/*
========================
idRenderMatrix::IsIdentity
========================
*/
ID_INLINE bool idRenderMatrix::IsIdentity( float epsilon ) const
{
	for( int i = 0; i < 4; i++ )
	{
		for( int j = 0; j < 4; j++ )
		{
			if( i == j )
			{
				if( idMath::Fabs( m[i * 4 + j] - 1.0f ) > epsilon )
				{
					return false;
				}
			}
			else
			{
				if( idMath::Fabs( m[i * 4 + j] ) > epsilon )
				{
					return false;
				}
			}
		}
	}
	return true;
}

/*
========================
idRenderMatrix::IsAffineTransform
========================
*/
ID_INLINE bool idRenderMatrix::IsAffineTransform( float epsilon ) const
{
	if( idMath::Fabs( m[3 * 4 + 0] ) > epsilon ||
			idMath::Fabs( m[3 * 4 + 1] ) > epsilon ||
			idMath::Fabs( m[3 * 4 + 2] ) > epsilon ||
			idMath::Fabs( m[3 * 4 + 3] - 1.0f ) > epsilon )
	{
		return false;
	}
	return true;
}

/*
========================
idRenderMatrix::IsUniformScale
========================
*/
ID_INLINE bool idRenderMatrix::IsUniformScale( float epsilon ) const
{
	float d0 = idMath::InvSqrt( m[0 * 4 + 0] * m[0 * 4 + 0] + m[1 * 4 + 0] * m[1 * 4 + 0] + m[2 * 4 + 0] * m[2 * 4 + 0] );
	float d1 = idMath::InvSqrt( m[0 * 4 + 1] * m[0 * 4 + 1] + m[1 * 4 + 1] * m[1 * 4 + 1] + m[2 * 4 + 1] * m[2 * 4 + 1] );
	float d2 = idMath::InvSqrt( m[0 * 4 + 2] * m[0 * 4 + 2] + m[1 * 4 + 2] * m[1 * 4 + 2] + m[2 * 4 + 2] * m[2 * 4 + 2] );
	if( idMath::Fabs( d0 - d1 ) > epsilon )
	{
		return false;
	}
	if( idMath::Fabs( d1 - d2 ) > epsilon )
	{
		return false;
	}
	if( idMath::Fabs( d0 - d2 ) > epsilon )
	{
		return false;
	}
	return true;
}

/*
========================
idRenderMatrix::TransformPoint
========================
*/
ID_INLINE void idRenderMatrix::TransformPoint( const idVec3& in, idVec3& out ) const
{
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const idRenderMatrix& matrix = *this;
	out[0] = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2] + matrix[0][3];
	out[1] = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2] + matrix[1][3];
	out[2] = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2] + matrix[2][3];
	assert( idMath::Fabs( in[0] * matrix[3][0] + in[1] * matrix[3][1] + in[2] * matrix[3][2] + matrix[3][3] - 1.0f ) < 0.01f );
}

/*
========================
idRenderMatrix::TransformPoint
========================
*/
ID_INLINE void idRenderMatrix::TransformPoint( const idVec3& in, idVec4& out ) const
{
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const idRenderMatrix& matrix = *this;
	out[0] = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2] + matrix[0][3];
	out[1] = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2] + matrix[1][3];
	out[2] = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2] + matrix[2][3];
	out[3] = in[0] * matrix[3][0] + in[1] * matrix[3][1] + in[2] * matrix[3][2] + matrix[3][3];
}

/*
========================
idRenderMatrix::TransformPoint
========================
*/
ID_INLINE void idRenderMatrix::TransformPoint( const idVec4& in, idVec4& out ) const
{
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const idRenderMatrix& matrix = *this;
	out[0] = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2] + in[3] * matrix[0][3];
	out[1] = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2] + in[3] * matrix[1][3];
	out[2] = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2] + in[3] * matrix[2][3];
	out[3] = in[0] * matrix[3][0] + in[1] * matrix[3][1] + in[2] * matrix[3][2] + in[3] * matrix[3][3];
}

/*
========================
idRenderMatrix::TransformDir
========================
*/
ID_INLINE void idRenderMatrix::TransformDir( const idVec3& in, idVec3& out, bool normalize ) const
{
	const idRenderMatrix& matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2];
	float p1 = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2];
	float p2 = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2];
	if( normalize )
	{
		float r = idMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
}

/*
========================
idRenderMatrix::TransformPlane
========================
*/
ID_INLINE void idRenderMatrix::TransformPlane( const idPlane& in, idPlane& out, bool normalize ) const
{
	assert( IsUniformScale( 0.01f ) );
	const idRenderMatrix& matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2];
	float p1 = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2];
	float p2 = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2];
	float d0 = matrix[0][3] - p0 * in[3];
	float d1 = matrix[1][3] - p1 * in[3];
	float d2 = matrix[2][3] - p2 * in[3];
	if( normalize )
	{
		float r = idMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
	out[3] = - p0 * d0 - p1 * d1 - p2 * d2;
}

/*
========================
idRenderMatrix::InverseTransformDir
========================
*/
ID_INLINE void idRenderMatrix::InverseTransformDir( const idVec3& in, idVec3& out, bool normalize ) const
{
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const idRenderMatrix& matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[1][0] + in[2] * matrix[2][0];
	float p1 = in[0] * matrix[0][1] + in[1] * matrix[1][1] + in[2] * matrix[2][1];
	float p2 = in[0] * matrix[0][2] + in[1] * matrix[1][2] + in[2] * matrix[2][2];
	if( normalize )
	{
		float r = idMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
}

/*
========================
idRenderMatrix::InverseTransformPlane
========================
*/
ID_INLINE void idRenderMatrix::InverseTransformPlane( const idPlane& in, idPlane& out, bool normalize ) const
{
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const idRenderMatrix& matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[1][0] + in[2] * matrix[2][0] + in[3] * matrix[3][0];
	float p1 = in[0] * matrix[0][1] + in[1] * matrix[1][1] + in[2] * matrix[2][1] + in[3] * matrix[3][1];
	float p2 = in[0] * matrix[0][2] + in[1] * matrix[1][2] + in[2] * matrix[2][2] + in[3] * matrix[3][2];
	float p3 = in[0] * matrix[0][3] + in[1] * matrix[1][3] + in[2] * matrix[2][3] + in[3] * matrix[3][3];
	if( normalize )
	{
		float r = idMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
		p3 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
	out[3] = p3;
}

/*
========================
idRenderMatrix::TransformModelToClip
========================
*/
ID_INLINE void idRenderMatrix::TransformModelToClip( const idVec3& src, const idRenderMatrix& modelMatrix, const idRenderMatrix& projectionMatrix, idVec4& eye, idVec4& clip )
{
	for( int i = 0; i < 4; i++ )
	{
		eye[i] =	modelMatrix[i][0] * src[0] +
					modelMatrix[i][1] * src[1] +
					modelMatrix[i][2] * src[2] +
					modelMatrix[i][3];
	}
	for( int i = 0; i < 4; i++ )
	{
		clip[i] =	projectionMatrix[i][0] * eye[0] +
					projectionMatrix[i][1] * eye[1] +
					projectionMatrix[i][2] * eye[2] +
					projectionMatrix[i][3] * eye[3];
	}
}

/*
========================
idRenderMatrix::TransformClipToDevice

Clip to normalized device coordinates.
========================
*/
ID_INLINE void idRenderMatrix::TransformClipToDevice( const idVec4& clip, idVec3& ndc )
{
	assert( idMath::Fabs( clip[3] ) > idMath::FLT_SMALLEST_NON_DENORMAL );
	float r = 1.0f / clip[3];
	ndc[0] = clip[0] * r;
	ndc[1] = clip[1] * r;
	ndc[2] = clip[2] * r;
}

/*
========================
idRenderMatrix::ApplyDepthHack
========================
*/
ID_INLINE void idRenderMatrix::ApplyDepthHack( idRenderMatrix& src )
{
	// scale projected z by 25%
	src.m[2 * 4 + 0] *= 0.25f;
	src.m[2 * 4 + 1] *= 0.25f;
	src.m[2 * 4 + 2] *= 0.25f;
	src.m[2 * 4 + 3] *= 0.25f;
}

/*
========================
idRenderMatrix::ApplyModelDepthHack
========================
*/
ID_INLINE void idRenderMatrix::ApplyModelDepthHack( idRenderMatrix& src, float value )
{
	// offset projected z
	src.m[2 * 4 + 3] -= value;
}

/*
========================
idRenderMatrix::CullPointToMVP
========================
*/
ID_INLINE bool idRenderMatrix::CullPointToMVP( const idRenderMatrix& mvp, const idVec3& point, bool zeroToOne )
{
	byte bits;
	return CullPointToMVPbits( mvp, point, &bits, zeroToOne );
}

/*
========================
idRenderMatrix::CullBoundsToMVP
========================
*/
ID_INLINE bool idRenderMatrix::CullBoundsToMVP( const idRenderMatrix& mvp, const idBounds& bounds, bool zeroToOne )
{
	byte bits;
	return CullBoundsToMVPbits( mvp, bounds, &bits, zeroToOne );
}

/*
========================
idRenderMatrix::CullExtrudedBoundsToMVP
========================
*/
ID_INLINE bool idRenderMatrix::CullExtrudedBoundsToMVP( const idRenderMatrix& mvp, const idBounds& bounds, const idVec3& extrudeDirection, const idPlane& clipPlane, bool zeroToOne )
{
	byte bits;
	return CullExtrudedBoundsToMVPbits( mvp, bounds, extrudeDirection, clipPlane, &bits, zeroToOne );
}

/*
========================
idRenderMatrix::DepthToZ
========================
*/
ID_INLINE float idRenderMatrix::DepthToZ( float depth ) const
{
	// only OpenGL-standard perspective projection matrix is supported!
	assert( m[1] == 0.0f && m[2] == 0.0f && m[3] == 0.0f );
	assert( m[4] == 0.0f && m[6] == 0.0f && m[7] == 0.0f );
	assert( m[8] == 0.0f && m[9] == 0.0f && m[12] == 0.0f && m[13] == 0.0f );
	assert( m[14] == -1.0f && m[15] == 0.0f );

	float clipZ = 2.0 * depth - 1.0;
	float A = m[10];
	float B = m[11];
	return B / (A + clipZ);
}

#endif // !__RENDERMATRIX_H__
