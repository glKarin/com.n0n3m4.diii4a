
#include "../precompiled.h"
#pragma hdrstop

/*
=====================
idQuat::ToAngles
=====================
*/
idAngles idQuat::ToAngles( void ) const {
	return ToMat3().ToAngles();
}

/*
=====================
idQuat::ToRotation
=====================
*/
idRotation idQuat::ToRotation( void ) const {
	idVec3 vec;
	float angle;

	vec.x = x;
	vec.y = y;
	vec.z = z;
	angle = idMath::ACos( w );
	if ( angle == 0.0f ) {
		vec.Set( 0.0f, 0.0f, 1.0f );
	} else {
		//vec *= (1.0f / sin( angle ));
		vec.Normalize();
		vec.FixDegenerateNormal();
		angle *= 2.0f * idMath::M_RAD2DEG;
	}
	return idRotation( vec3_origin, vec, angle );
}

/*
=====================
idQuat::ToMat3
=====================
*/
idMat3 idQuat::ToMat3( void ) const {
	idMat3	mat;
	float	wx, wy, wz;
	float	xx, yy, yz;
	float	xy, xz, zz;
	float	x2, y2, z2;

	x2 = x + x;
	y2 = y + y;
	z2 = z + z;

	xx = x * x2;
	xy = x * y2;
	xz = x * z2;

	yy = y * y2;
	yz = y * z2;
	zz = z * z2;

	wx = w * x2;
	wy = w * y2;
	wz = w * z2;

	mat[ 0 ][ 0 ] = 1.0f - ( yy + zz );
	mat[ 0 ][ 1 ] = xy - wz;
	mat[ 0 ][ 2 ] = xz + wy;

	mat[ 1 ][ 0 ] = xy + wz;
	mat[ 1 ][ 1 ] = 1.0f - ( xx + zz );
	mat[ 1 ][ 2 ] = yz - wx;

	mat[ 2 ][ 0 ] = xz - wy;
	mat[ 2 ][ 1 ] = yz + wx;
	mat[ 2 ][ 2 ] = 1.0f - ( xx + yy );

	return mat;
}

/*
=====================
idQuat::ToMat4
=====================
*/
idMat4 idQuat::ToMat4( void ) const {
	return ToMat3().ToMat4();
}

/*
=====================
idQuat::ToCQuat
=====================
*/
idCQuat idQuat::ToCQuat( void ) const {
	if ( w < 0.0f ) {
		return idCQuat( -x, -y, -z );
	}
	return idCQuat( x, y, z );
}

/*
============
idQuat::ToAngularVelocity
============
*/
idVec3 idQuat::ToAngularVelocity( void ) const {
	idVec3 vec;

	vec.x = x;
	vec.y = y;
	vec.z = z;
	vec.Normalize();
	return vec * idMath::ACos( w );
}

/*
=============
idQuat::ToString
=============
*/
const char *idQuat::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=====================
idQuat::Slerp

Spherical linear interpolation between two quaternions.
=====================
*/
idQuat &idQuat::Slerp( const idQuat &from, const idQuat &to, float t ) {
#ifdef _XENON
#else
	idQuat	temp;
	float	omega, cosom, sinom, scale0, scale1;

	if ( t <= 0.0f ) {
		*this = from;
		return *this;
	}

	if ( t >= 1.0f ) {
		*this = to;
		return *this;
	}

	if ( from == to ) {
		*this = to;
		return *this;
	}

	cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
	if ( cosom < 0.0f ) {
		temp = -to;
		cosom = -cosom;
	} else {
		temp = to;
	}

	if ( ( 1.0f - cosom ) > 1e-6f ) {
#if 0
		omega = acos( cosom );
		sinom = 1.0f / idMath::Sin( omega );
		scale0 = idMath::Sin( ( 1.0f - t ) * omega ) * sinom;
		scale1 = idMath::Sin( t * omega ) * sinom;
#else
		scale0 = 1.0f - cosom * cosom;
		sinom = idMath::InvSqrt( scale0 );
		omega = idMath::ATan16( scale0 * sinom, cosom );
		scale0 = idMath::Sin16( ( 1.0f - t ) * omega ) * sinom;
		scale1 = idMath::Sin16( t * omega ) * sinom;
#endif
	} else {
		scale0 = 1.0f - t;
		scale1 = t;
	}

	*this = ( scale0 * from ) + ( scale1 * temp );
	return *this;
#endif
}

/*
=============
idCQuat::ToAngles
=============
*/
idAngles idCQuat::ToAngles( void ) const {
	return ToQuat().ToAngles();
}

/*
=============
idCQuat::ToRotation
=============
*/
idRotation idCQuat::ToRotation( void ) const {
	return ToQuat().ToRotation();
}

/*
=============
idCQuat::ToMat3
=============
*/
idMat3 idCQuat::ToMat3( void ) const {
	return ToQuat().ToMat3();
}

/*
=============
idCQuat::ToMat4
=============
*/
idMat4 idCQuat::ToMat4( void ) const {
	return ToQuat().ToMat4();
}

/*
=============
idCQuat::ToString
=============
*/
const char *idCQuat::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}
