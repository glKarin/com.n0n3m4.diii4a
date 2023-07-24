// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include <float.h>

idAngles ang_zero( 0.0f, 0.0f, 0.0f );


/*
=================
idAngles::Normalize360

returns angles normalized to the range [0 <= angle < 360]
=================
*/
idAngles& idAngles::Normalize360( void ) {
	int i;

	for ( i = 0; i < 3; i++ ) {
		if ( ( (*this)[i] >= 360.0f ) || ( (*this)[i] < 0.0f ) ) {
			(*this)[i] -= floor( (*this)[i] / 360.0f ) * 360.0f;

			if ( (*this)[i] >= 360.0f ) {
				(*this)[i] -= 360.0f;
			}
			if ( (*this)[i] < 0.0f ) {
				(*this)[i] += 360.0f;
			}
		}
	}

	return *this;
}

/*
=================
idAngles::Normalize180

returns angles normalized to the range [-180 < angle <= 180]
=================
*/
idAngles& idAngles::Normalize180( void ) {
	Normalize360();

	if ( pitch > 180.0f ) {
		pitch -= 360.0f;
	}
	
	if ( yaw > 180.0f ) {
		yaw -= 360.0f;
	}

	if ( roll > 180.0f ) {
		roll -= 360.0f;
	}
	return *this;
}

/*
=================
idAngles::ToVectors
=================
*/
void idAngles::ToVectors( idVec3 *forward, idVec3 *right, idVec3 *up ) const {
	float sr, sp, sy, cr, cp, cy;
	
	idMath::SinCos( DEG2RAD( yaw ), sy, cy );
	idMath::SinCos( DEG2RAD( pitch ), sp, cp );
	idMath::SinCos( DEG2RAD( roll ), sr, cr );

	if ( forward ) {
		forward->Set( cp * cy, cp * sy, -sp );
	}

	if ( right ) {
		right->Set( -sr * sp * cy + cr * sy, -sr * sp * sy + -cr * cy, -sr * cp );
	}

	if ( up ) {
		up->Set( cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp );
	}
}

/*
=================
idAngles::ToForward
=================
*/
idVec3 idAngles::ToForward( void ) const {
	float sp, sy, cp, cy;
	
	idMath::SinCos( DEG2RAD( yaw ), sy, cy );
	idMath::SinCos( DEG2RAD( pitch ), sp, cp );

	return idVec3( cp * cy, cp * sy, -sp );
}

/*
=================
idAngles::ToQuat
=================
*/
idQuat idAngles::ToQuat( void ) const {
	float sx, cx, sy, cy, sz, cz;
	float sxcy, cxcy, sxsy, cxsy;

	idMath::SinCos( DEG2RAD( yaw ) * 0.5f, sz, cz );
	idMath::SinCos( DEG2RAD( pitch ) * 0.5f, sy, cy );
	idMath::SinCos( DEG2RAD( roll ) * 0.5f, sx, cx );

	sxcy = sx * cy;
	cxcy = cx * cy;
	sxsy = sx * sy;
	cxsy = cx * sy;

	return idQuat( cxsy*sz - sxcy*cz, -cxsy*cz - sxcy*sz, sxsy*cz - cxcy*sz, cxcy*cz + sxsy*sz );
}

/*
=================
idAngles::ToRotation
=================
*/
idRotation idAngles::ToRotation( void ) const {
	idVec3 vec;
	float angle, w;
	float sx, cx, sy, cy, sz, cz;
	float sxcy, cxcy, sxsy, cxsy;

	if ( pitch == 0.0f ) {
		if ( yaw == 0.0f ) {
			return idRotation( vec3_origin, idVec3( -1.0f, 0.0f, 0.0f ), roll );
		}
		if ( roll == 0.0f ) {
			return idRotation( vec3_origin, idVec3( 0.0f, 0.0f, -1.0f ), yaw );
		}
	} else if ( yaw == 0.0f && roll == 0.0f ) {
		return idRotation( vec3_origin, idVec3( 0.0f, -1.0f, 0.0f ), pitch );
	}

	idMath::SinCos( DEG2RAD( yaw ) * 0.5f, sz, cz );
	idMath::SinCos( DEG2RAD( pitch ) * 0.5f, sy, cy );
	idMath::SinCos( DEG2RAD( roll ) * 0.5f, sx, cx );

	sxcy = sx * cy;
	cxcy = cx * cy;
	sxsy = sx * sy;
	cxsy = cx * sy;

	vec.x =  cxsy * sz - sxcy * cz;
	vec.y = -cxsy * cz - sxcy * sz;
	vec.z =  sxsy * cz - cxcy * sz;
	w =		 cxcy * cz + sxsy * sz;
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
=================
idAngles::ToMat3

	Rotation matrices used:

	X =	??

	Y = ??

	Z = ??

	Rotation order: M = ?.?.?

		| cos(y) * cos(z)                               cos(y) * sin(z)                              -sin(y)         |
	M = | sin(x) * sin(y) * cos(z) + cos(x) * -sin(z)   sin(x) * sin(y) * sin(z) + cos(x) * cos(z)   sin(x) * cos(y) |
		| cos(x) * sin(y) * cos(z) + -sin(x) * -sin(z)  cos(x) * sin(y) * sin(z) + -sin(x) * cos(z)  cos(x) * cos(y) |

	pitch = rotation around y axis
	yaw = rotation around z axis
	roll = rotation around x axis

=================
*/
idMat3 idAngles::ToMat3( void ) const {
	idMat3 mat;
	float sr, sp, sy, cr, cp, cy;

	idMath::SinCos( DEG2RAD( yaw ), sy, cy );
	idMath::SinCos( DEG2RAD( pitch ), sp, cp );
	idMath::SinCos( DEG2RAD( roll ), sr, cr );

	mat[ 0 ].Set( cp * cy, cp * sy, -sp );
	mat[ 1 ].Set( sr * sp * cy + cr * -sy, sr * sp * sy + cr * cy, sr * cp );
	mat[ 2 ].Set( cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp );

	return mat;
}

/*
=================
idAngles::ToMat3NoRoll
=================
*/
void idAngles::ToMat3NoRoll( idMat3& mat ) const {
	float sp, sy, cp, cy;
		
	idMath::SinCos( DEG2RAD( yaw ), sy, cy );
	idMath::SinCos( DEG2RAD( pitch ), sp, cp );

	mat[ 0 ].Set( cp * cy, cp * sy, -sp );
	mat[ 1 ].Set( -sy, cy, 0.0f );
	mat[ 2 ].Set( sp * cy, sp * sy, cp );
}

/*
=================
idAngles::ToMat4
=================
*/
idMat4 idAngles::ToMat4( void ) const {
	return ToMat3().ToMat4();
}

/*
=================
idAngles::ToAngularVelocity
=================
*/
idVec3 idAngles::ToAngularVelocity( void ) const {
	return idAngles::ToRotation().ToAngularVelocity();
}

/*
=============
idAngles::ToString
=============
*/
const char *idAngles::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=================
idAngles::ToMat3Maya

	Rotation matrices used:

		| 1    0      0    |
	X =	| 0  cos(x) sin(x) |
		| 0 -sin(x) cos(x) |

		| cos(y) 0 -sin(y) |
	Y =	|   0    1    0    |
		| sin(y) 0  cos(y) |

		| cos(z)  sin(z) 0 |
	Z =	| -sin(z) cos(z) 0 |
		|   0       0    1 |

	Rotation order: M = X.Y.Z

		| cos(y) * cos(z)                               cos(y) * sin(z)                              -sin(y)         |
	M = | sin(x) * sin(y) * cos(z) + cos(x) * -sin(z)   sin(x) * sin(y) * sin(z) + cos(x) * cos(z)   sin(x) * cos(y) |
		| cos(x) * sin(y) * cos(z) + -sin(x) * -sin(z)  cos(x) * sin(y) * sin(z) + -sin(x) * cos(z)  cos(x) * cos(y) |

	pitch = rotation around x axis
	yaw = rotation around y axis
	roll = rotation around z axis

=================
*/
idMat3 idAngles::ToMat3Maya( void ) const {
	idMat3 mat;
	float sr, sp, sy, cr, cp, cy;

	idMath::SinCos( DEG2RAD( yaw ), sy, cy );
	idMath::SinCos( DEG2RAD( pitch ), sp, cp );
	idMath::SinCos( DEG2RAD( roll ), sr, cr );

	mat[ 0 ].Set( cy * cr, cy * sr, -sy );
	mat[ 1 ].Set( sp * sy * cr + cp * -sr, sp * sy * sr + cp * cr, sp * cy );
	mat[ 2 ].Set( cp * sy * cr + -sp * -sr, cp * sy * sr + -sp * cr, cp * cy );

	return mat;
}
