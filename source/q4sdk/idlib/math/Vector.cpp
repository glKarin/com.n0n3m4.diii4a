
#include "../precompiled.h"
#pragma hdrstop

idVec2 vec2_origin( 0.0f, 0.0f );
idVec3 vec3_origin( 0.0f, 0.0f, 0.0f );
idVec4 vec4_origin( 0.0f, 0.0f, 0.0f, 0.0f );
idVec4 vec4_one( 1.0f, 1.0f, 1.0f, 1.0f );
idVec5 vec5_origin( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
idVec6 vec6_origin( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
idVec6 vec6_infinity( idMath::INFINITY, idMath::INFINITY, idMath::INFINITY, idMath::INFINITY, idMath::INFINITY, idMath::INFINITY );


//===============================================================
//
//	idVec2
//
//===============================================================

/*
=============
idVec2::ToString
=============
*/
const char *idVec2::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void idVec2::Lerp( const idVec2 &v1, const idVec2 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}


//===============================================================
//
//	idVec3
//
//===============================================================

/*
=============
idVec3::ToYaw
=============
*/
float idVec3::ToYaw( void ) const {
	float yaw;
	
	if ( ( y == 0.0f ) && ( x == 0.0f ) ) {
		yaw = 0.0f;
	} else {
		yaw = RAD2DEG( idMath::ATan( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}
	}

	return yaw;
}

/*
=============
idVec3::ToPitch
=============
*/
float idVec3::ToPitch( void ) const {
	float	forward;
	float	pitch;
	
	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		forward = ( float )idMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( idMath::ATan( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	return pitch;
}

/*
=============
idVec3::ToAngles
=============
*/
idAngles idVec3::ToAngles( void ) const {
	float forward;
	float yaw;
	float pitch;
	
	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		yaw = 0.0f;
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		yaw = RAD2DEG( idMath::ATan( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}

		forward = ( float )idMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( idMath::ATan( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	return idAngles( -pitch, yaw, 0.0f );
}

// RAVEN BEGIN
/*
=============
idVec3::ToRadians
=============
*/
rvAngles idVec3::ToRadians( void ) const 
{
	float forward;
	float yaw;
	float pitch;
	
	if( !x && !y ) 
	{
		yaw = 0.0f;
		if( z > 0.0f ) 
		{
			pitch = idMath::HALF_PI;
		}
		else 
		{
			pitch = idMath::THREEFOURTHS_PI;
		}
	} 
	else 
	{
		yaw = idMath::ATan( y, x );
		if( yaw < 0.0f ) 
		{
			yaw += idMath::TWO_PI;
		}

		forward = ( float )idMath::Sqrt( x * x + y * y );
		pitch = idMath::ATan( z, forward );
		if( pitch < 0.0f ) 
		{
			pitch += idMath::TWO_PI;
		}
	}

	return( rvAngles( -pitch, yaw, 0.0f ) );
}
// RAVEN END

/*
=============
idVec3::ToPolar
=============
*/
idPolar3 idVec3::ToPolar( void ) const {
	float forward;
	float yaw;
	float pitch;
	
	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		yaw = 0.0f;
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		yaw = RAD2DEG( idMath::ATan( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}

		forward = ( float )idMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( idMath::ATan( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}
	return idPolar3( idMath::Sqrt( x * x + y * y + z * z ), yaw, -pitch );
}

/*
=============
idVec3::ToMat3
=============
*/
idMat3 idVec3::ToMat3( void ) const {
	idMat3	mat;
	float	d;

	mat[0] = *this;
	d = x * x + y * y;
	if ( !d ) {
		mat[1][0] = 1.0f;
		mat[1][1] = 0.0f;
		mat[1][2] = 0.0f;
	} else {
		d = idMath::InvSqrt( d );
		mat[1][0] = -y * d;
		mat[1][1] = x * d;
		mat[1][2] = 0.0f;
	}
	mat[2] = Cross( mat[1] );

	return mat;
}

/*
=============
idVec3::ToMat3
=============
*/
// RAVEN BEGIN
// abahr: added axis so we can create matrix with non-x vector
idMat3 idVec3::ToMat3( int axis ) const {
	idMat3	mat;
	float	d;
	int		index_x = axis % GetDimension();
	int		index_y = (axis + 1) % GetDimension();
	int		index_z = (axis + 2) % GetDimension();
	float	local_x = (*this)[index_x];
	float	local_y = (*this)[index_y];

	mat[axis] = *this;
	d = local_x * local_x + local_y * local_y;
	if ( !d ) {
		mat[index_y][index_x] = 1.0f;
		mat[index_y][index_y] = 0.0f;
		mat[index_y][index_z] = 0.0f;
	} else {
		d = idMath::InvSqrt( d );
		mat[index_y][index_x] = -local_y * d;
		mat[index_y][index_y] = local_x * d;
		mat[index_y][index_z] = 0.0f;
	}
	mat[index_z] = Cross( mat[index_y] );

	return mat;
}
// RAVEN END

// RAVEN BEGIN
// jscott: slightly quicker version without the copy
idMat3 &idVec3::ToMat3( idMat3 &mat ) const 
{
	float	d;

	mat[0] = *this;
	d = x * x + y * y;
	if ( !d ) 
	{
		mat[1][0] = 1.0f;
		mat[1][1] = 0.0f;
		mat[1][2] = 0.0f;
	} 
	else 
	{
		d = idMath::InvSqrt( d );
		mat[1][0] = -y * d;
		mat[1][1] = x * d;
		mat[1][2] = 0.0f;
	}
	mat[2] = Cross( mat[1] );
	return( mat );
}
// RAVEN END

/*
=============
idVec3::ToString
=============
*/
const char *idVec3::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void idVec3::Lerp( const idVec3 &v1, const idVec3 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}

/*
=============
SLerp

Spherical linear interpolation from v1 to v2.
Vectors are expected to be normalized.
=============
*/
#define LERP_DELTA 1e-6

void idVec3::SLerp( const idVec3 &v1, const idVec3 &v2, const float t ) {
	float omega, cosom, sinom, scale0, scale1;

	if ( t <= 0.0f ) {
		(*this) = v1;
		return;
	} else if ( t >= 1.0f ) {
		(*this) = v2;
		return;
	}

	cosom = v1 * v2;
	if ( ( 1.0f - cosom ) > LERP_DELTA ) {
		omega = acos( cosom );
		sinom = idMath::Sin( omega );
		scale0 = idMath::Sin( ( 1.0f - t ) * omega ) / sinom;
		scale1 = idMath::Sin( t * omega ) / sinom;
	} else {
		scale0 = 1.0f - t;
		scale1 = t;
	}

	(*this) = ( v1 * scale0 + v2 * scale1 );
}

/*
=============
ProjectSelfOntoSphere

Projects the z component onto a sphere.
=============
*/
void idVec3::ProjectSelfOntoSphere( const float radius ) {
	float rsqr = radius * radius;
	float len = Length();
	if ( len  < rsqr * 0.5f ) {
		z = idMath::Sqrt( rsqr - len );
	} else {
		z = rsqr / ( 2.0f * idMath::Sqrt( len ) );
	}
}

//===============================================================
//
//	idVec4
//
//===============================================================

/*
=============
idVec4::ToString
=============
*/
const char *idVec4::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
// jsinger: moved to inline in the header file for Xenon

//===============================================================
//
//	idVec5
//
//===============================================================

/*
=============
idVec5::ToString
=============
*/
const char *idVec5::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
idVec5::Lerp
=============
*/
void idVec5::Lerp( const idVec5 &v1, const idVec5 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		x = v1.x + l * ( v2.x - v1.x );
		y = v1.y + l * ( v2.y - v1.y );
		z = v1.z + l * ( v2.z - v1.z );
		s = v1.s + l * ( v2.s - v1.s );
		t = v1.t + l * ( v2.t - v1.t );
	}
}


//===============================================================
//
//	idVec6
//
//===============================================================

/*
=============
idVec6::ToString
=============
*/
const char *idVec6::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

//===============================================================
//
//	idVecX
//
//===============================================================

// RAVEN BEGIN
float	idVecX::tempPtr[VECX_MAX_TEMP];
//float *	idVecX::tempPtr = (float *) ( ( (int) idVecX::temp + 15 ) & ~15 );
// RAVEN END
int		idVecX::tempIndex = 0;

/*
=============
idVecX::ToString
=============
*/
const char *idVecX::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}
