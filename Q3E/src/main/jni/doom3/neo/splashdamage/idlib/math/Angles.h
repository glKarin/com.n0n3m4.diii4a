// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __MATH_ANGLES_H__
#define __MATH_ANGLES_H__

/*
===============================================================================

	Euler angles

===============================================================================
*/

// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

const int A_YAW				= 2;
const int A_PITCH			= 1;
const int A_ROLL			= 0;

class idVec3;
class idQuat;
class idRotation;
class idMat3;
class idMat4;

class idAngles {
public:
	float			pitch;
	float			yaw;
	float			roll;

					idAngles( void );
					idAngles( float pitch, float yaw, float roll );
					explicit idAngles( const idVec3 &v );

	void 			Set( float pitch, float yaw, float roll );
	idAngles &		Zero( void );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	idAngles		operator-() const;			// negate angles, in general not the inverse rotation
	idAngles &		operator=( const idAngles &a );
	idAngles		operator+( const idAngles &a ) const;
	idAngles &		operator+=( const idAngles &a );
	idAngles		operator-( const idAngles &a ) const;
	idAngles &		operator-=( const idAngles &a );
	idAngles		operator*( const float a ) const;
	idAngles &		operator*=( const float a );
	idAngles		operator/( const float a ) const;
	idAngles &		operator/=( const float a );

	friend idAngles	operator*( const float a, const idAngles &b );

	bool			Compare( const idAngles &a ) const;							// exact compare, no epsilon
	bool			Compare( const idAngles &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idAngles &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idAngles &a ) const;						// exact compare, no epsilon

	idAngles &		Normalize360( void );	// normalizes 'this'
	idAngles &		Normalize180( void );	// normalizes 'this'

	void			Clamp( const idAngles &min, const idAngles &max );

	int				GetDimension( void ) const;

	void			ToVectors( idVec3 *forward, idVec3 *right = NULL, idVec3 *up = NULL ) const;
	idVec3			ToForward( void ) const;
	idQuat			ToQuat( void ) const;
	idRotation		ToRotation( void ) const;
	idMat3			ToMat3( void ) const;
	void			ToMat3NoRoll( idMat3& mat ) const;
	static idMat3&	YawToMat3( float yaw, idMat3& mat );
	static idMat3&	PitchToMat3( float pitch, idMat3& mat );
	static idMat3&	RollToMat3( float roll, idMat3& mat );
	idMat4			ToMat4( void ) const;
	idVec3			ToAngularVelocity( void ) const;
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	idMat3			ToMat3Maya( void ) const;

	bool			FixDenormals( float epsilon = idMath::FLT_EPSILON );		// change tiny numbers to zero

};

extern idAngles ang_zero;

ID_INLINE idAngles::idAngles( void ) {
}

ID_INLINE idAngles::idAngles( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ID_INLINE idAngles::idAngles( const idVec3 &v ) {
	this->pitch = v[0];
	this->yaw	= v[1];
	this->roll	= v[2];
}

ID_INLINE void idAngles::Set( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ID_INLINE idAngles &idAngles::Zero( void ) {
	pitch = yaw = roll = 0.0f;
	return *this;
}

ID_INLINE float idAngles::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[ index ];
}

ID_INLINE float &idAngles::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[ index ];
}

ID_INLINE idAngles idAngles::operator-() const {
	return idAngles( -pitch, -yaw, -roll );
}

ID_INLINE idAngles &idAngles::operator=( const idAngles &a ) {
	pitch	= a.pitch;
	yaw		= a.yaw;
	roll	= a.roll;
	return *this;
}

ID_INLINE idAngles idAngles::operator+( const idAngles &a ) const {
	return idAngles( pitch + a.pitch, yaw + a.yaw, roll + a.roll );
}

ID_INLINE idAngles& idAngles::operator+=( const idAngles &a ) {
	pitch	+= a.pitch;
	yaw		+= a.yaw;
	roll	+= a.roll;

	return *this;
}

ID_INLINE idAngles idAngles::operator-( const idAngles &a ) const {
	return idAngles( pitch - a.pitch, yaw - a.yaw, roll - a.roll );
}

ID_INLINE idAngles& idAngles::operator-=( const idAngles &a ) {
	pitch	-= a.pitch;
	yaw		-= a.yaw;
	roll	-= a.roll;

	return *this;
}

ID_INLINE idAngles idAngles::operator*( const float a ) const {
	return idAngles( pitch * a, yaw * a, roll * a );
}

ID_INLINE idAngles& idAngles::operator*=( float a ) {
	pitch	*= a;
	yaw		*= a;
	roll	*= a;
	return *this;
}

ID_INLINE idAngles idAngles::operator/( const float a ) const {
	float inva = 1.0f / a;
	return idAngles( pitch * inva, yaw * inva, roll * inva );
}

ID_INLINE idAngles& idAngles::operator/=( float a ) {
	float inva = 1.0f / a;
	pitch	*= inva;
	yaw		*= inva;
	roll	*= inva;
	return *this;
}

ID_INLINE idAngles operator*( const float a, const idAngles &b ) {
	return idAngles( a * b.pitch, a * b.yaw, a * b.roll );
}

ID_INLINE bool idAngles::Compare( const idAngles &a ) const {
	return ( ( a.pitch == pitch ) && ( a.yaw == yaw ) && ( a.roll == roll ) );
}

ID_INLINE bool idAngles::Compare( const idAngles &a, const float epsilon ) const {
	if ( idMath::Fabs( pitch - a.pitch ) > epsilon ) {
		return false;
	}
			
	if ( idMath::Fabs( yaw - a.yaw ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( roll - a.roll ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool idAngles::operator==( const idAngles &a ) const {
	return Compare( a );
}

ID_INLINE bool idAngles::operator!=( const idAngles &a ) const {
	return !Compare( a );
}

ID_INLINE void idAngles::Clamp( const idAngles &min, const idAngles &max ) {
	if ( pitch < min.pitch ) {
		pitch = min.pitch;
	} else if ( pitch > max.pitch ) {
		pitch = max.pitch;
	}
	if ( yaw < min.yaw ) {
		yaw = min.yaw;
	} else if ( yaw > max.yaw ) {
		yaw = max.yaw;
	}
	if ( roll < min.roll ) {
		roll = min.roll;
	} else if ( roll > max.roll ) {
		roll = max.roll;
	}
}

ID_INLINE int idAngles::GetDimension( void ) const {
	return 3;
}

ID_INLINE const float *idAngles::ToFloatPtr( void ) const {
	return &pitch;
}

ID_INLINE float *idAngles::ToFloatPtr( void ) {
	return &pitch;
}

ID_INLINE idMat3& idAngles::YawToMat3( float yaw, idMat3& mat ) {
	float sy, cy;
		
	idMath::SinCos( DEG2RAD( yaw ), sy, cy );

	mat[ 0 ].Set( cy, sy, 0 );
	mat[ 1 ].Set( -sy, cy, 0 );
	mat[ 2 ].Set( 0, 0, 1 );

	return mat;
}

ID_INLINE idMat3& idAngles::PitchToMat3( float pitch, idMat3& mat ) {
	float sp, cp;

	idMath::SinCos( DEG2RAD( pitch ), sp, cp );

	mat[ 0 ].Set( cp, 0, -sp );
	mat[ 1 ].Set( 0, 1, 0 );
	mat[ 2 ].Set( sp, 0, cp );

	return mat;
}

ID_INLINE idMat3& idAngles::RollToMat3( float roll, idMat3& mat ) {
	float sr, cr;

	idMath::SinCos( DEG2RAD( roll ), sr, cr );

	mat[ 0 ].Set( 1, 0, 0 );
	mat[ 1 ].Set( 0, cr, sr );
	mat[ 2 ].Set( 0, -sr, cr );

	return mat;
}

ID_INLINE bool idAngles::FixDenormals( float epsilon ) {
	bool denormal = false;
	if ( fabs( yaw ) < epsilon ) {
		yaw = 0.0f;
		denormal = true;
	}
	if ( fabs( pitch ) < epsilon ) {
		pitch = 0.0f;
		denormal = true;
	}
	if ( fabs( roll ) < epsilon ) {
		roll = 0.0f;
		denormal = true;
	}
	return denormal;
}

#endif /* !__MATH_ANGLES_H__ */
