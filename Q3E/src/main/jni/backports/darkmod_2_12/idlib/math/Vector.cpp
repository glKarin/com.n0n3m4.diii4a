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

idVec2 vec2_origin( 0.0f, 0.0f );
idVec3 vec3_origin( 0.0f, 0.0f, 0.0f );
idVec4 vec4_origin( 0.0f, 0.0f, 0.0f, 0.0f );
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


/*
=============
MulCW

Multiply on vector component-wise
=============
*/
void idVec2::MulCW( const idVec2& a ) {
	x *= a.x;
	y *= a.y;
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
		yaw = RAD2DEG( atan2( y, x ) );
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
		pitch = RAD2DEG( atan2( z, forward ) );
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
		yaw = RAD2DEG( atan2( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}

		forward = ( float )idMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	return idAngles( -pitch, yaw, 0.0f );
}

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
		yaw = RAD2DEG( atan2( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}

		forward = ( float )idMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
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
		sinom = sin( omega );
		scale0 = sin( ( 1.0f - t ) * omega ) / sinom;
		scale1 = sin( t * omega ) / sinom;
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
	//stgatilov: WTF is it?
	//even measurement units don't match...
	float rsqr = radius * radius;
	float len = Length();
	if ( len  < rsqr * 0.5f ) {
		z = sqrt( rsqr - len );
	} else {
		z = rsqr / ( 2.0f * sqrt( len ) );
	}
}

/*
=============
ProjectToConvexCone

stgatilov #5599: Consider a point at which "n" planar obstacles met, each having unit outer normal "normals[i]".
Find projection of given velocity "*this" into the admissible cone.
It may be (in order from more desirable to less desirable):
  a) just this velocity
  b) projection onto some obstacle plane
  c) projection onto an intersection edge of two planes
  d) zero
This function can be used for sliding one object along collision contacts with other objects.
=============
*/
idVec3 idVec3::ProjectToConvexCone( const idVec3 *normals, int n, float epsilon ) const {
	const idVec3 &vec = *this;

	// detect set of obstacles we are moving into
	idFlexList<int, 16> badIds;
	for (int i = 0; i < n; i++) {
		assert(idMath::Fabs(normals[i].Length() - 1.0f) <= 1e-3f);
		if (normals[i] * vec < -epsilon)
			badIds.AddGrow(i);
	}

	if (badIds.Num() == 0)
		return vec;	// already within cone 

	idVec3 bestVec(0.0f);
	float minDiffSqr = FLT_MAX;
	// checks if given vector is admissible and updates best known projection
	auto CheckVec = [&](idVec3 checkVec) -> void {
		for (int u = 0; u < n; u++)
			if (normals[u] * checkVec < -epsilon)
				return;	// goes into u-th obstacle

		float diffSqr = (checkVec - vec).LengthSqr();
		if (minDiffSqr > diffSqr) {
			minDiffSqr = diffSqr;
			bestVec = checkVec;
		}
	};

	// try projections onto individual planes
	for (int i = 0; i < badIds.Num(); i++) {
		idVec3 tmpVec = vec;
		tmpVec.ProjectOntoPlane(normals[badIds[i]]);
		CheckVec(tmpVec);
	}
	// try projections onto intersection edges
	for (int i = 0; i < badIds.Num(); i++)
		for (int j = i+1; j < badIds.Num(); j++) {
			idVec3 cross = normals[i].Cross(normals[j]);
			if (cross.Normalize() <= 1e-3f)
				continue;
			idVec3 tmpVec = (vec * cross) * cross;
			CheckVec(tmpVec);
		}

	return bestVec;
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
void idVec4::Lerp( const idVec4 &v1, const idVec4 &v2, const float l ) {
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

float	idVecX::temp[VECX_MAX_TEMP+4];
float *	idVecX::tempPtr = (float *)(((intptr_t)idVecX::temp + 15) & ~15);
int		idVecX::tempIndex = 0;

/*
=============
idVecX::ToString
=============
*/
const char *idVecX::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}
