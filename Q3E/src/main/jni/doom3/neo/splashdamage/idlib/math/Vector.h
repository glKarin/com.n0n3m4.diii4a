// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __MATH_VECTOR_H__
#define __MATH_VECTOR_H__

/*
===============================================================================

  Vector classes

===============================================================================
*/

#define VECTOR_EPSILON		0.001f

class idAngles;
class idPolar3;
class idMat3;

// RAVEN BEGIN
class rvAngles;
// RAVEN END

//===============================================================
//
//	idVec2 - 2D vector
//
//===============================================================

class idVec2 {
public:
	float			x;
	float			y;

					idVec2( void );
					explicit idVec2( const float x, const float y );

	void 			Set( const float x, const float y );
	void			Zero( void );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	idVec2			operator-() const;
	float			operator*( const idVec2 &a ) const;
	idVec2			operator*( const float a ) const;
	idVec2			operator/( const float a ) const;
	idVec2			operator+( const idVec2 &a ) const;
	idVec2			operator-( const idVec2 &a ) const;
	idVec2 &		operator+=( const idVec2 &a );
	idVec2 &		operator-=( const idVec2 &a );
	idVec2 &		operator/=( const idVec2 &a );
	idVec2 &		operator/=( const float a );
	idVec2 &		operator*=( const float a );

	friend idVec2	operator*( const float a, const idVec2 b );

	bool			Compare( const idVec2 &a ) const;							// exact compare, no epsilon
	bool			Compare( const idVec2 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const idVec2 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idVec2 &a ) const;						// exact compare, no epsilon

	bool			IsZero( void ) const;

	float			Length( void ) const;
	float			LengthFast( void ) const;
	float			LengthSqr( void ) const;
	float			Normalize( void );			// returns length
	float			NormalizeFast( void );		// returns length
	idVec2 &		Truncate( float length );	// cap length
	void			Clamp( const idVec2 &min, const idVec2 &max );
	void			Snap( void );				// snap to closest integer value
	void			SnapInt( void );			// snap towards integer (floor)

	int				GetDimension( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const idVec2 &v1, const idVec2 &v2, const float l );

// RAVEN BEGIN
// jscott: Ensures second element greater than first
	void			EnsureIncremental( void );
// RAVEN END
};

extern idVec2 vec2_origin;
extern idVec2 vec2_one;
#define vec2_zero vec2_origin

ID_INLINE idVec2::idVec2( void ) {
}

ID_INLINE idVec2::idVec2( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

ID_INLINE void idVec2::Set( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

ID_INLINE void idVec2::Zero( void ) {
	x = y = 0.0f;
}

ID_INLINE bool idVec2::Compare( const idVec2 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) );
}

ID_INLINE bool idVec2::Compare( const idVec2 &a, const float epsilon ) const {
	if ( idMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}
			
	if ( idMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool idVec2::operator==( const idVec2 &a ) const {
	return Compare( a );
}

ID_INLINE bool idVec2::operator!=( const idVec2 &a ) const {
	return !Compare( a );
}

ID_INLINE bool idVec2::IsZero( void ) const {
   return ( ( (*(const unsigned int *)&(x)) | (*(const unsigned int *)&(y)) ) & ~(1<<31) ) == 0;
}

ID_INLINE float idVec2::operator[]( int index ) const {
	return ( &x )[ index ];
}

ID_INLINE float& idVec2::operator[]( int index ) {
	return ( &x )[ index ];
}

ID_INLINE float idVec2::Length( void ) const {
	return ( float )idMath::Sqrt( x * x + y * y );
}

ID_INLINE float idVec2::LengthFast( void ) const {
	float sqrLength;

	sqrLength = x * x + y * y;
	return sqrLength * idMath::RSqrt( sqrLength );
}

ID_INLINE float idVec2::LengthSqr( void ) const {
	return ( x * x + y * y );
}

ID_INLINE float idVec2::Normalize( void ) {
	float invLength, length;

	length = x * x + y * y;
	length = idMath::Sqrt( length );
	if ( length < idMath::FLT_EPSILON ) {
		return 0.f;
	}
	invLength = 1.f / length;
	x *= invLength;
	y *= invLength;
	return length;
}

ID_INLINE float idVec2::NormalizeFast( void ) {
	float lengthSqr, invLength;

	lengthSqr = x * x + y * y;
	invLength = idMath::RSqrt( lengthSqr );
	x *= invLength;
	y *= invLength;
	return invLength * lengthSqr;
}

ID_INLINE idVec2 &idVec2::Truncate( float length ) {
	float length2;
	float ilength;

	if ( !length ) {
		Zero();
	}
	else {
		length2 = LengthSqr();
		if ( length2 > length * length ) {
			ilength = length * idMath::InvSqrt( length2 );
			x *= ilength;
			y *= ilength;
		}
	}

	return *this;
}

ID_INLINE void idVec2::Clamp( const idVec2 &min, const idVec2 &max ) {
	if ( x < min.x ) {
		x = min.x;
	} else if ( x > max.x ) {
		x = max.x;
	}
	if ( y < min.y ) {
		y = min.y;
	} else if ( y > max.y ) {
		y = max.y;
	}
}

ID_INLINE void idVec2::Snap( void ) {
	x = idMath::Floor( x + 0.5f );
	y = idMath::Floor( y + 0.5f );
}

ID_INLINE void idVec2::SnapInt( void ) {
	x = idMath::Floor( x );
	y = idMath::Floor( y );
}

ID_INLINE idVec2 idVec2::operator-() const {
	return idVec2( -x, -y );
}
	
ID_INLINE idVec2 idVec2::operator-( const idVec2 &a ) const {
	return idVec2( x - a.x, y - a.y );
}

ID_INLINE float idVec2::operator*( const idVec2 &a ) const {
	return x * a.x + y * a.y;
}

ID_INLINE idVec2 idVec2::operator*( const float a ) const {
	return idVec2( x * a, y * a );
}

ID_INLINE idVec2 idVec2::operator/( const float a ) const {
	float inva = 1.0f / a;
	return idVec2( x * inva, y * inva );
}

ID_INLINE idVec2 operator*( const float a, const idVec2 b ) {
	return idVec2( b.x * a, b.y * a );
}

ID_INLINE idVec2 idVec2::operator+( const idVec2 &a ) const {
	return idVec2( x + a.x, y + a.y );
}

ID_INLINE idVec2 &idVec2::operator+=( const idVec2 &a ) {
	x += a.x;
	y += a.y;

	return *this;
}

ID_INLINE idVec2 &idVec2::operator/=( const idVec2 &a ) {
	x /= a.x;
	y /= a.y;

	return *this;
}

ID_INLINE idVec2 &idVec2::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;

	return *this;
}

ID_INLINE idVec2 &idVec2::operator-=( const idVec2 &a ) {
	x -= a.x;
	y -= a.y;

	return *this;
}

ID_INLINE idVec2 &idVec2::operator*=( const float a ) {
	x *= a;
	y *= a;

	return *this;
}

ID_INLINE int idVec2::GetDimension( void ) const {
	return 2;
}

ID_INLINE const float *idVec2::ToFloatPtr( void ) const {
	return &x;
}

ID_INLINE float *idVec2::ToFloatPtr( void ) {
	return &x;
}

// RAVEN BEGIN
// jscott: ensures the second element is greater than the first
ID_INLINE void idVec2::EnsureIncremental( void )
{
	float	temp;

	if( x < y )
	{
		return;
	}
	temp = x;
	x = y;
	y = temp;
}
// RAVEN END

//===============================================================
//
//	idVec3 - 3D vector
//
//===============================================================

class idVec3 {
public:	
	float			x;
	float			y;
	float			z;

					idVec3( void );
					explicit idVec3( const float x, const float y, const float z );

	void 			Set( const float x, const float y, const float z );
	void			Zero( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	idVec3			operator-() const;
	idVec3 &		operator=( const idVec3 &a );		// required because of a msvc 6 & 7 bug
// RAVEN BEGIN
// bdube: added vec2 equal
	idVec3 &		operator=( const idVec2 &a );
	idVec3 &		operator*=( const idVec3 &a );
// RAVEN END
	float			operator*( const idVec3 &a ) const;
	idVec3			operator*( const float a ) const;
	idVec3			operator/( const float a ) const;
	idVec3			operator+( const idVec3 &a ) const;
	idVec3			operator-( const idVec3 &a ) const;
	idVec3 &		operator+=( const idVec3 &a );
	idVec3 &		operator-=( const idVec3 &a );
	idVec3 &		operator/=( const idVec3 &a );
	idVec3 &		operator/=( const float a );
	idVec3 &		operator*=( const float a );

	friend idVec3	operator*( const float a, const idVec3 &b );

	bool			Compare( const idVec3 &a ) const;							// exact compare, no epsilon
	bool			Compare( const idVec3 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const idVec3 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idVec3 &a ) const;						// exact compare, no epsilon

	bool			IsZero( void ) const;

	bool			FixDegenerateNormal( void );								// fix degenerate axial cases
	bool			FixDenormals( float epsilon = idMath::FLT_EPSILON );		// change tiny numbers to zero

	idVec3			Cross( const idVec3 &a ) const;
	idVec3 &		Cross( const idVec3 &a, const idVec3 &b );
	float			Length( void ) const;
	float			LengthSqr( void ) const;
	float			LengthFast( void ) const;
	float			Normalize( void );				// returns length
	float			NormalizeFast( void );			// returns length
	idVec3 &		Truncate( float length );		// cap length
	void			Clamp( const idVec3 &min, const idVec3 &max );
	void			ClampMin( const float &minx, const float &miny, const float &minz );
	void			Snap( void );					// snap to closest integer value
	void			SnapInt( void );				// snap towards integer (floor)

	int				GetDimension( void ) const;

	float			ToYaw( void ) const;
	float			ToPitch( void ) const;
	idAngles		ToAngles( void ) const;
	idPolar3		ToPolar( void ) const;
	idMat3			ToMat3( void ) const;		// vector should be normalized
	const idVec2 &	ToVec2( void ) const;
	idVec2 &		ToVec2( void );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			NormalVectors( idVec3 &left, idVec3 &down ) const;	// vector should be normalized
	void			OrthogonalBasis( idVec3 &left, idVec3 &up ) const;

	void			ProjectOntoPlane( const idVec3 &normal, const float overBounce = 1.0f );
	bool			ProjectAlongPlane( const idVec3 &normal, const float epsilon, const float overBounce = 1.0f );
	void			ProjectSelfOntoSphere( const float radius );

	void			Lerp( const idVec3 &v1, const idVec3 &v2, const float l );
	void			SLerp( const idVec3 &v1, const idVec3 &v2, const float l );
	
// RAVEN BEGIN
// jscott: Ensures second element greater than first
	void			EnsureIncremental( void );
// jscott: for BSE
	int				GetLargestAxis( void ) const;
// jscott: for rvAngles
	rvAngles		ToRadians( void ) const;
	idMat3			&ToMat3( idMat3 &mat ) const;		// vector should be normalized
	float			Dist(const idVec3 &Pt) const 
	{
		idVec3 delta(x,y,z);
		delta = delta - Pt;
		return delta.LengthFast();	
	}

// RAVEN END

	idVec3			ToMaya( void ) const;
	idVec3&			ToMayaSelf( void );
	idVec3			FromMaya( void ) const;
	idVec3&			FromMayaSelf( void );

	static float	BiTangentSign( const idVec3& n, const idVec3& t0, const idVec3& t1 );
};

extern idVec3 vec3_origin;
extern idVec3 vec3_one;
#define vec3_zero vec3_origin

ID_INLINE idVec3::idVec3( void ) {
}

ID_INLINE idVec3::idVec3( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
	VEC_CHECK_BAD( *this );
}

ID_INLINE float idVec3::operator[]( const int index ) const {
	assert( index >= 0 && index < 3 );
	return ( &x )[ index ];
}

ID_INLINE float &idVec3::operator[]( const int index ) {
	assert( index >= 0 && index < 3 );
	return ( &x )[ index ];
}

ID_INLINE void idVec3::Set( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
	VEC_CHECK_BAD( *this );
}

ID_INLINE void idVec3::Zero( void ) {
	x = y = z = 0.0f;
}

ID_INLINE idVec3 idVec3::operator-() const {
	return idVec3( -x, -y, -z );
}

ID_INLINE idVec3 &idVec3::operator=( const idVec3 &a ) {
	x = a.x;
	y = a.y;
	z = a.z;
	VEC_CHECK_BAD( *this );
	return *this;
}

// RAVEN BEGIN
// bdube: added vec3 from vec2 assignment
ID_INLINE idVec3 &idVec3::operator=( const idVec2 &a ) {
	x = a.x;
	y = a.y;
	VEC_CHECK_BAD( *this );
	return *this;
}
ID_INLINE idVec3 &idVec3::operator*=( const idVec3 &a )
{
	x *= a.x;
	y *= a.y;
	z *= a.z;
	VEC_CHECK_BAD( *this );
	return *this;
}
// RAVEN END

ID_INLINE idVec3 idVec3::operator-( const idVec3 &a ) const {
	const idVec3 temp( x - a.x, y - a.y, z - a.z );
	VEC_CHECK_BAD( temp );
	return temp;
}

ID_INLINE float idVec3::operator*( const idVec3 &a ) const {
	const float temp = x * a.x + y * a.y + z * a.z;
	FLOAT_CHECK_BAD( temp );
	return temp;
}

ID_INLINE idVec3 idVec3::operator*( const float a ) const {
	const idVec3 temp( x * a, y * a, z * a );
	VEC_CHECK_BAD( temp );
	return temp;
}

ID_INLINE idVec3 idVec3::operator/( const float a ) const {
	float inva = 1.0f / a;
	const idVec3 temp( x * inva, y * inva, z * inva );
	VEC_CHECK_BAD( temp );
	return temp;
}

ID_INLINE idVec3 operator*( const float a, const idVec3 &b ) {
	const idVec3 temp( b.x * a, b.y * a, b.z * a );
	VEC_CHECK_BAD( temp );
	return temp;
}

ID_INLINE idVec3 idVec3::operator+( const idVec3 &a ) const {
	const idVec3 temp( x + a.x, y + a.y, z + a.z );
	VEC_CHECK_BAD( temp );
	return temp;
}

ID_INLINE idVec3 &idVec3::operator+=( const idVec3 &a ) {
	x += a.x;
	y += a.y;
	z += a.z;

	VEC_CHECK_BAD( *this );
	return *this;
}

ID_INLINE idVec3 &idVec3::operator/=( const idVec3 &a ) {
	x /= a.x;
	y /= a.y;
	z /= a.z;

	VEC_CHECK_BAD( *this );
	return *this;
}

ID_INLINE idVec3 &idVec3::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	z *= inva;

	VEC_CHECK_BAD( *this );
	return *this;
}

ID_INLINE idVec3 &idVec3::operator-=( const idVec3 &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;

	VEC_CHECK_BAD( *this );
	return *this;
}

ID_INLINE idVec3 &idVec3::operator*=( const float a ) {
	x *= a;
	y *= a;
	z *= a;

	VEC_CHECK_BAD( *this );
	return *this;
}

ID_INLINE bool idVec3::Compare( const idVec3 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) );
}

ID_INLINE bool idVec3::Compare( const idVec3 &a, const float epsilon ) const {
	if ( idMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}
			
	if ( idMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( z - a.z ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool idVec3::operator==( const idVec3 &a ) const {
	return Compare( a );
}

ID_INLINE bool idVec3::operator!=( const idVec3 &a ) const {
	return !Compare( a );
}

ID_INLINE bool idVec3::IsZero( void ) const {
   return ( ( (*(const unsigned int *)&(x)) | (*(const unsigned int *)&(y)) | (*(const unsigned int *)&(z)) ) & ~(1<<31) ) == 0;
}

ID_INLINE float idVec3::NormalizeFast( void ) {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z;
	invLength = idMath::RSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	VEC_CHECK_BAD( *this );
	return invLength * sqrLength;
}

ID_INLINE bool idVec3::FixDegenerateNormal( void ) {
	if ( x == 0.0f ) {
		if ( y == 0.0f ) {
			if ( z > 0.0f ) {
				if ( z != 1.0f ) {
					z = 1.0f;
					return true;
				}
			} else {
				if ( z != -1.0f ) {
					z = -1.0f;
					return true;
				}
			}
			return false;
		} else if ( z == 0.0f ) {
			if ( y > 0.0f ) {
				if ( y != 1.0f ) {
					y = 1.0f;
					return true;
				}
			} else {
				if ( y != -1.0f ) {
					y = -1.0f;
					return true;
				}
			}
			return false;
		}
	} else if ( y == 0.0f ) {
		if ( z == 0.0f ) {
			if ( x > 0.0f ) {
				if ( x != 1.0f ) {
					x = 1.0f;
					return true;
				}
			} else {
				if ( x != -1.0f ) {
					x = -1.0f;
					return true;
				}
			}
			return false;
		}
	}
	if ( idMath::Fabs( x ) == 1.0f ) {
		if ( y != 0.0f || z != 0.0f ) {
			y = z = 0.0f;
			return true;
		}
		return false;
	} else if ( idMath::Fabs( y ) == 1.0f ) {
		if ( x != 0.0f || z != 0.0f ) {
			x = z = 0.0f;
			return true;
		}
		return false;
	} else if ( idMath::Fabs( z ) == 1.0f ) {
		if ( x != 0.0f || y != 0.0f ) {
			x = y = 0.0f;
			return true;
		}
		return false;
	}
	return false;
}

ID_INLINE bool idVec3::FixDenormals( float epsilon ) {
	bool denormal = false;
	if ( fabs( x ) < epsilon ) {
		x = 0.0f;
		denormal = true;
	}
	if ( fabs( y ) < epsilon ) {
		y = 0.0f;
		denormal = true;
	}
	if ( fabs( z ) < epsilon ) {
		z = 0.0f;
		denormal = true;
	}
	return denormal;
}

ID_INLINE idVec3 idVec3::Cross( const idVec3 &a ) const {
	const idVec3 temp( y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x );
	VEC_CHECK_BAD( temp );
	return temp;
}

ID_INLINE idVec3 &idVec3::Cross( const idVec3 &a, const idVec3 &b ) {
	x = a.y * b.z - a.z * b.y;
	y = a.z * b.x - a.x * b.z;
	z = a.x * b.y - a.y * b.x;

	VEC_CHECK_BAD( *this );
	return *this;
}

ID_INLINE float idVec3::Length( void ) const {
	const float temp = idMath::Sqrt( x * x + y * y + z * z );
	FLOAT_CHECK_BAD( temp );
	return temp;
}

ID_INLINE float idVec3::LengthSqr( void ) const {
	const float temp = ( x * x + y * y + z * z );
	FLOAT_CHECK_BAD( temp );
	return temp;
}

ID_INLINE float idVec3::LengthFast( void ) const {
	const float sqrLength = x * x + y * y + z * z;
	const float temp = sqrLength * idMath::RSqrt( sqrLength );
	FLOAT_CHECK_BAD( temp );
	return temp;
}

ID_INLINE float idVec3::Normalize( void ) {
	float invLength, length;

	length = x * x + y * y + z * z;
	length = idMath::Sqrt( length );
	if ( length < idMath::FLT_EPSILON ) {
		return 0.f;
	}
	invLength = 1.0f / length;
	x *= invLength;
	y *= invLength;
	z *= invLength;
	VEC_CHECK_BAD( *this );
	return length;
}

ID_INLINE idVec3 &idVec3::Truncate( float length ) {
	float length2;
	float ilength;

	if ( !length ) {
		Zero();
	}
	else {
		length2 = LengthSqr();
		if ( length2 > length * length ) {
			ilength = length * idMath::InvSqrt( length2 );
			x *= ilength;
			y *= ilength;
			z *= ilength;
		}
	}

	VEC_CHECK_BAD( *this );
	return *this;
}

ID_INLINE void idVec3::Clamp( const idVec3 &min, const idVec3 &max ) {
	if ( x < min.x ) {
		x = min.x;
	} else if ( x > max.x ) {
		x = max.x;
	}
	if ( y < min.y ) {
		y = min.y;
	} else if ( y > max.y ) {
		y = max.y;
	}
	if ( z < min.z ) {
		z = min.z;
	} else if ( z > max.z ) {
		z = max.z;
	}
	VEC_CHECK_BAD( *this );
}

ID_INLINE void idVec3::ClampMin( const float &minx, const float &miny, const float &minz ) {
	if ( x < minx ) {
		x = minx;
	}
	if ( y < miny ) {
		y = miny;
	}
	if ( z < minz ) {
		z = minz;
	}
	VEC_CHECK_BAD( *this );
}

ID_INLINE void idVec3::Snap( void ) {
	x = idMath::Floor( x + 0.5f );
	y = idMath::Floor( y + 0.5f );
	z = idMath::Floor( z + 0.5f );
	VEC_CHECK_BAD( *this );
}

ID_INLINE void idVec3::SnapInt( void ) {
	x = idMath::Floor( x );
	y = idMath::Floor( y );
	z = idMath::Floor( z );
	VEC_CHECK_BAD( *this );
}

ID_INLINE int idVec3::GetDimension( void ) const {
	return 3;
}

ID_INLINE const idVec2 &idVec3::ToVec2( void ) const {
	return *reinterpret_cast<const idVec2 *>(this);
}

ID_INLINE idVec2 &idVec3::ToVec2( void ) {
	return *reinterpret_cast<idVec2 *>(this);
}

ID_INLINE const float *idVec3::ToFloatPtr( void ) const {
	return &x;
}

ID_INLINE float *idVec3::ToFloatPtr( void ) {
	return &x;
}

ID_INLINE void idVec3::NormalVectors( idVec3 &left, idVec3 &down ) const {
	float d;

	d = x * x + y * y;
	if ( !d ) {
		left[0] = 1.0f;
		left[1] = 0.0f;
		left[2] = 0.0f;
	} else {
		d = idMath::InvSqrt( d );
		left[0] = -y * d;
		left[1] = x * d;
		left[2] = 0.0f;
	}
	down = left.Cross( *this );
}

ID_INLINE void idVec3::OrthogonalBasis( idVec3 &left, idVec3 &up ) const {
	float l, s;

	if ( idMath::Fabs( z ) > 0.7f ) {
		l = y * y + z * z;
		s = idMath::InvSqrt( l );
		up[0] = 0.0f;
		up[1] = z * s;
		up[2] = -y * s;
		left[0] = l * s;
		left[1] = -x * up[2];
		left[2] = x * up[1];
	}
	else {
		l = x * x + y * y;
		s = idMath::InvSqrt( l );
		left[0] = -y * s;
		left[1] = x * s;
		left[2] = 0.0f;
		up[0] = -z * left[1];
		up[1] = z * left[0];
		up[2] = l * s;
	}
}

ID_INLINE void idVec3::ProjectOntoPlane( const idVec3 &normal, const float overBounce ) {
	float backoff;
	
	backoff = *this * normal;
	
	if ( overBounce != 1.0f ) {
		if ( backoff < 0.0f ) {
			backoff *= overBounce;
		} else {
			backoff /= overBounce;
		}
	}

	*this -= backoff * normal;
	VEC_CHECK_BAD( *this );
}

ID_INLINE bool idVec3::ProjectAlongPlane( const idVec3 &normal, const float epsilon, const float overBounce ) {
	idVec3 cross;
	float len;

	cross = this->Cross( normal ).Cross( (*this) );
	// normalize so a fixed epsilon can be used
	cross.Normalize();
	len = normal * cross;
	if ( idMath::Fabs( len ) < epsilon ) {
		return false;
	}
	cross *= overBounce * ( normal * (*this) ) / len;
	(*this) -= cross;
	VEC_CHECK_BAD( *this );
	return true;
}

// RAVEN BEGIN
// jscott: ensures x < y < z
ID_INLINE void idVec3::EnsureIncremental( void ) {
	if( y < x ) {
		idSwap( x, y );
	}
	
	if( z < y ) {
		idSwap( x, z );
	}
	
	if( y < x ) {
		idSwap( x, y );
	}
	VEC_CHECK_BAD( *this );
}

ID_INLINE int idVec3::GetLargestAxis( void ) const {
	float a = fabs( x );
	float b = fabs( y );
	float c = fabs( z );

	if( a >= b && a >= c ) {
		return( 0 );
	}
	if( b >= a && b >= c ) {
		return( 1 );
	}
	if( c >= a && c >= b ) {
		return( 2 );
	}
	return( 0 );
}
// RAVEN END

ID_INLINE idVec3 idVec3::ToMaya( void ) const {
	idVec3 vecMaya;

	vecMaya = *this;
	vecMaya.ToMayaSelf();
	return vecMaya;
}

ID_INLINE idVec3& idVec3::ToMayaSelf( void ) {
	idSwap( y, z );
	z = -z;

	VEC_CHECK_BAD( *this );
	return (*this);
}

ID_INLINE idVec3 idVec3::FromMaya( void ) const {
	idVec3 vecId;

	vecId = *this;
	vecId.FromMayaSelf();
	return vecId;
}

ID_INLINE idVec3& idVec3::FromMayaSelf( void ) {
	idSwap( y, z );
	y = -y;

	VEC_CHECK_BAD( *this );
	return (*this);
}

ID_INLINE float idVec3::BiTangentSign( const idVec3& n, const idVec3& t0, const idVec3& t1 ) {
	idVec3 bitangent;

	bitangent.Cross( n, t0 );
	return ( bitangent.x * t1.x + bitangent.y * t1.y + bitangent.z * t1.z ) > 0.0f ? 1.0f : -1.0f;
}

//===============================================================
//
//	idVec4 - 4D vector
//
//===============================================================

class idVec4 {
public:	
	float			x;
	float			y;
	float			z;
	float			w;

					idVec4( void );
					explicit idVec4( const float x, const float y, const float z, const float w );

	void 			Set( const float x, const float y, const float z, const float w );
	void			Zero( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	idVec4			operator-() const;
	float			operator*( const idVec4 &a ) const;
	idVec4			operator*( const float a ) const;
	idVec4			operator/( const float a ) const;
	idVec4			operator+( const idVec4 &a ) const;
	idVec4			operator-( const idVec4 &a ) const;
	idVec4 &		operator+=( const idVec4 &a );
	idVec4 &		operator-=( const idVec4 &a );
	idVec4 &		operator/=( const idVec4 &a );
	idVec4 &		operator/=( const float a );
	idVec4 &		operator*=( const float a );

	friend idVec4	operator*( const float a, const idVec4 b );

	bool			Compare( const idVec4 &a ) const;							// exact compare, no epsilon
	bool			Compare( const idVec4 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const idVec4 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idVec4 &a ) const;						// exact compare, no epsilon

	bool			IsZero( void ) const;

	float			Length( void ) const;
	float			LengthSqr( void ) const;
	float			Normalize( void );			// returns length
	float			NormalizeFast( void );		// returns length

	int				GetDimension( void ) const;

	const idVec2 &	ToVec2( void ) const;
	idVec2 &		ToVec2( void );
	const idVec3 &	ToVec3( void ) const;
	idVec3 &		ToVec3( void );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const idVec4 &v1, const idVec4 &v2, const float l );

	bool			ContainsPoint( const float xTest, const float yTest ) const;
	bool			ContainsPoint( const idVec2& testPoint ) const;
};

extern idVec4 vec4_origin;
extern idVec4 vec4_one;
#define vec4_zero vec4_origin

ID_INLINE idVec4::idVec4( void ) {
}

ID_INLINE idVec4::idVec4( const float x, const float y, const float z, const float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ID_INLINE void idVec4::Set( const float x, const float y, const float z, const float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ID_INLINE void idVec4::Zero( void ) {
	x = y = z = w = 0.0f;
}

ID_INLINE float idVec4::operator[]( int index ) const {
	assert( index >= 0 && index < 4 );
	return ( &x )[ index ];
}

ID_INLINE float& idVec4::operator[]( int index ) {
	assert( index >= 0 && index < 4 );
	return ( &x )[ index ];
}

ID_INLINE idVec4 idVec4::operator-() const {
	return idVec4( -x, -y, -z, -w );
}

ID_INLINE idVec4 idVec4::operator-( const idVec4 &a ) const {
	return idVec4( x - a.x, y - a.y, z - a.z, w - a.w );
}

ID_INLINE float idVec4::operator*( const idVec4 &a ) const {
	return x * a.x + y * a.y + z * a.z + w * a.w;
}

ID_INLINE idVec4 idVec4::operator*( const float a ) const {
	return idVec4( x * a, y * a, z * a, w * a );
}

ID_INLINE idVec4 idVec4::operator/( const float a ) const {
	float inva = 1.0f / a;
	return idVec4( x * inva, y * inva, z * inva, w * inva );
}

ID_INLINE idVec4 operator*( const float a, const idVec4 b ) {
	return idVec4( b.x * a, b.y * a, b.z * a, b.w * a );
}

ID_INLINE idVec4 idVec4::operator+( const idVec4 &a ) const {
	return idVec4( x + a.x, y + a.y, z + a.z, w + a.w );
}

ID_INLINE idVec4 &idVec4::operator+=( const idVec4 &a ) {
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

ID_INLINE idVec4 &idVec4::operator/=( const idVec4 &a ) {
	x /= a.x;
	y /= a.y;
	z /= a.z;
	w /= a.w;

	return *this;
}

ID_INLINE idVec4 &idVec4::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	z *= inva;
	w *= inva;

	return *this;
}

ID_INLINE idVec4 &idVec4::operator-=( const idVec4 &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

ID_INLINE idVec4 &idVec4::operator*=( const float a ) {
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

ID_INLINE bool idVec4::Compare( const idVec4 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) && ( w == a.w ) );
}

ID_INLINE bool idVec4::Compare( const idVec4 &a, const float epsilon ) const {
	if ( idMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}
			
	if ( idMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( z - a.z ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( w - a.w ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool idVec4::operator==( const idVec4 &a ) const {
	return Compare( a );
}

ID_INLINE bool idVec4::operator!=( const idVec4 &a ) const {
	return !Compare( a );
}

ID_INLINE bool idVec4::IsZero( void ) const {
   return ( ( (*(const unsigned int *)&(x)) | (*(const unsigned int *)&(y)) | (*(const unsigned int *)&(z)) | (*(const unsigned int *)&(w)) ) & ~(1<<31) ) == 0;
}

ID_INLINE float idVec4::Length( void ) const {
	return ( float )idMath::Sqrt( x * x + y * y + z * z + w * w );
}

ID_INLINE float idVec4::LengthSqr( void ) const {
	return ( x * x + y * y + z * z + w * w );
}

ID_INLINE float idVec4::Normalize( void ) {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z + w * w;
	invLength = idMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

ID_INLINE float idVec4::NormalizeFast( void ) {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z + w * w;
	invLength = idMath::RSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

ID_INLINE int idVec4::GetDimension( void ) const {
	return 4;
}

ID_INLINE const idVec2 &idVec4::ToVec2( void ) const {
	return *reinterpret_cast<const idVec2 *>(this);
}

ID_INLINE idVec2 &idVec4::ToVec2( void ) {
	return *reinterpret_cast<idVec2 *>(this);
}

ID_INLINE const idVec3 &idVec4::ToVec3( void ) const {
	return *reinterpret_cast<const idVec3 *>(this);
}

ID_INLINE idVec3 &idVec4::ToVec3( void ) {
	return *reinterpret_cast<idVec3 *>(this);
}

ID_INLINE const float *idVec4::ToFloatPtr( void ) const {
	return &x;
}

ID_INLINE float *idVec4::ToFloatPtr( void ) {
	return &x;
}

ID_INLINE bool idVec4::ContainsPoint( const idVec2& testPoint ) const {
	return !((( testPoint.x < x ) || ( testPoint.x > x + z )) || (( testPoint.y < y ) || ( testPoint.y > y + w )));
}

ID_INLINE bool idVec4::ContainsPoint( const float xTest, const float yTest ) const {
	return !((( xTest < x ) || ( xTest > x + z )) || (( yTest < y ) || ( yTest > y + w )));
}

//===============================================================
//
//	idVec5 - 5D vector
//
//===============================================================

class idVec5 {
public:
	float			x;
	float			y;
	float			z;
	float			s;
	float			t;

					idVec5( void );
					explicit idVec5( const idVec3 &xyz, const idVec2 &st );
					explicit idVec5( const float x, const float y, const float z, const float s, const float t );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	idVec5 &		operator=( const idVec3 &a );

	bool			operator==( const idVec5 &rhs ) const;

	int				GetDimension( void ) const;

	const idVec2 &	ToVec2( void ) const;
	idVec2 &		ToVec2( void );

	const idVec3 &	ToVec3( void ) const;
	idVec3 &		ToVec3( void );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const idVec5 &v1, const idVec5 &v2, const float l );
	void			Set( const idVec3& xyz, const idVec2& st );
};

extern idVec5 vec5_origin;
extern idVec5 vec5_one;
#define vec5_zero vec5_origin

ID_INLINE idVec5::idVec5( void ) {
}

ID_INLINE idVec5::idVec5( const idVec3 &xyz, const idVec2 &st ) {
	x = xyz.x;
	y = xyz.y;
	z = xyz.z;
	s = st[0];
	t = st[1];
}

ID_INLINE idVec5::idVec5( const float x, const float y, const float z, const float s, const float t ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->s = s;
	this->t = t;
}

ID_INLINE float idVec5::operator[]( int index ) const {
	return ( &x )[ index ];
}

ID_INLINE float& idVec5::operator[]( int index ) {
	return ( &x )[ index ];
}

ID_INLINE idVec5 &idVec5::operator=( const idVec3 &a ) { 
	x = a.x;
	y = a.y;
	z = a.z;
	s = t = 0;
	return *this;
}

ID_INLINE int idVec5::GetDimension( void ) const {
	return 5;
}

ID_INLINE const idVec3 &idVec5::ToVec3( void ) const {
	return *reinterpret_cast<const idVec3 *>(this);
}

ID_INLINE idVec3 &idVec5::ToVec3( void ) {
	return *reinterpret_cast<idVec3 *>(this);
}

ID_INLINE const idVec2 &idVec5::ToVec2( void ) const {
	return *reinterpret_cast<const idVec2 *>(&s);
}

ID_INLINE idVec2 &idVec5::ToVec2( void ) {
	return *reinterpret_cast<idVec2 *>(&s);
}

ID_INLINE const float *idVec5::ToFloatPtr( void ) const {
	return &x;
}

ID_INLINE float *idVec5::ToFloatPtr( void ) {
	return &x;
}

ID_INLINE void idVec5::Set( const idVec3& xyz, const idVec2& st ) {
	x = xyz.x;
	y = xyz.y;
	z = xyz.z;
	s = st.x;
	t = st.y;
}


//===============================================================
//
//	idVec6 - 6D vector
//
//===============================================================

class idVec6 {
public:	
					idVec6( void );
					explicit idVec6( const float *a );
					explicit idVec6( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );

	void 			Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );
	void			Zero( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	idVec6			operator-() const;
	idVec6			operator*( const float a ) const;
	idVec6			operator/( const float a ) const;
	float			operator*( const idVec6 &a ) const;
	idVec6			operator-( const idVec6 &a ) const;
	idVec6			operator+( const idVec6 &a ) const;
	idVec6 &		operator*=( const float a );
	idVec6 &		operator/=( const float a );
	idVec6 &		operator+=( const idVec6 &a );
	idVec6 &		operator-=( const idVec6 &a );

	friend idVec6	operator*( const float a, const idVec6 b );

	bool			Compare( const idVec6 &a ) const;							// exact compare, no epsilon
	bool			Compare( const idVec6 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const idVec6 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idVec6 &a ) const;						// exact compare, no epsilon

	float			Length( void ) const;
	float			LengthSqr( void ) const;
	float			Normalize( void );			// returns length
	float			NormalizeFast( void );		// returns length

	int				GetDimension( void ) const;

	const idVec3 &	SubVec3( int index ) const;
	idVec3 &		SubVec3( int index );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	float			p[6];
};

extern idVec6 vec6_origin;
extern idVec6 vec6_one;
#define vec6_zero vec6_origin
extern idVec6 vec6_infinity;

ID_INLINE idVec6::idVec6( void ) {
}

ID_INLINE idVec6::idVec6( const float *a ) {
	memcpy( p, a, 6 * sizeof( float ) );
}

ID_INLINE idVec6::idVec6( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ID_INLINE idVec6 idVec6::operator-() const {
	return idVec6( -p[0], -p[1], -p[2], -p[3], -p[4], -p[5] );
}

ID_INLINE float idVec6::operator[]( const int index ) const {
	return p[index];
}

ID_INLINE float &idVec6::operator[]( const int index ) {
	return p[index];
}

ID_INLINE idVec6 idVec6::operator*( const float a ) const {
	return idVec6( p[0]*a, p[1]*a, p[2]*a, p[3]*a, p[4]*a, p[5]*a );
}

ID_INLINE float idVec6::operator*( const idVec6 &a ) const {
	return p[0] * a[0] + p[1] * a[1] + p[2] * a[2] + p[3] * a[3] + p[4] * a[4] + p[5] * a[5];
}

ID_INLINE idVec6 idVec6::operator/( const float a ) const {
	float inva;

	assert( a != 0.0f );
	inva = 1.0f / a;
	return idVec6( p[0]*inva, p[1]*inva, p[2]*inva, p[3]*inva, p[4]*inva, p[5]*inva );
}

ID_INLINE idVec6 idVec6::operator+( const idVec6 &a ) const {
	return idVec6( p[0] + a[0], p[1] + a[1], p[2] + a[2], p[3] + a[3], p[4] + a[4], p[5] + a[5] );
}

ID_INLINE idVec6 idVec6::operator-( const idVec6 &a ) const {
	return idVec6( p[0] - a[0], p[1] - a[1], p[2] - a[2], p[3] - a[3], p[4] - a[4], p[5] - a[5] );
}

ID_INLINE idVec6 &idVec6::operator*=( const float a ) {
	p[0] *= a;
	p[1] *= a;
	p[2] *= a;
	p[3] *= a;
	p[4] *= a;
	p[5] *= a;
	return *this;
}

ID_INLINE idVec6 &idVec6::operator/=( const float a ) {
	float inva;

	assert( a != 0.0f );
	inva = 1.0f / a;
	p[0] *= inva;
	p[1] *= inva;
	p[2] *= inva;
	p[3] *= inva;
	p[4] *= inva;
	p[5] *= inva;
	return *this;
}

ID_INLINE idVec6 &idVec6::operator+=( const idVec6 &a ) {
	p[0] += a[0];
	p[1] += a[1];
	p[2] += a[2];
	p[3] += a[3];
	p[4] += a[4];
	p[5] += a[5];
	return *this;
}

ID_INLINE idVec6 &idVec6::operator-=( const idVec6 &a ) {
	p[0] -= a[0];
	p[1] -= a[1];
	p[2] -= a[2];
	p[3] -= a[3];
	p[4] -= a[4];
	p[5] -= a[5];
	return *this;
}

ID_INLINE idVec6 operator*( const float a, const idVec6 b ) {
	return b * a;
}

ID_INLINE bool idVec6::Compare( const idVec6 &a ) const {
	return ( ( p[0] == a[0] ) && ( p[1] == a[1] ) && ( p[2] == a[2] ) &&
			( p[3] == a[3] ) && ( p[4] == a[4] ) && ( p[5] == a[5] ) );
}

ID_INLINE bool idVec6::Compare( const idVec6 &a, const float epsilon ) const {
	if ( idMath::Fabs( p[0] - a[0] ) > epsilon ) {
		return false;
	}
			
	if ( idMath::Fabs( p[1] - a[1] ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( p[2] - a[2] ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( p[3] - a[3] ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( p[4] - a[4] ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( p[5] - a[5] ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool idVec6::operator==( const idVec6 &a ) const {
	return Compare( a );
}

ID_INLINE bool idVec6::operator!=( const idVec6 &a ) const {
	return !Compare( a );
}

ID_INLINE void idVec6::Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ID_INLINE void idVec6::Zero( void ) {
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0.0f;
}

ID_INLINE float idVec6::Length( void ) const {
	return ( float )idMath::Sqrt( p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5] );
}

ID_INLINE float idVec6::LengthSqr( void ) const {
	return ( p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5] );
}

ID_INLINE float idVec6::Normalize( void ) {
	float sqrLength, invLength;

	sqrLength = p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5];
	invLength = idMath::InvSqrt( sqrLength );
	p[0] *= invLength;
	p[1] *= invLength;
	p[2] *= invLength;
	p[3] *= invLength;
	p[4] *= invLength;
	p[5] *= invLength;
	return invLength * sqrLength;
}

ID_INLINE float idVec6::NormalizeFast( void ) {
	float sqrLength, invLength;

	sqrLength = p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5];
	invLength = idMath::RSqrt( sqrLength );
	p[0] *= invLength;
	p[1] *= invLength;
	p[2] *= invLength;
	p[3] *= invLength;
	p[4] *= invLength;
	p[5] *= invLength;
	return invLength * sqrLength;
}

ID_INLINE int idVec6::GetDimension( void ) const {
	return 6;
}

ID_INLINE const idVec3 &idVec6::SubVec3( int index ) const {
	return *reinterpret_cast<const idVec3 *>(p + index * 3);
}

ID_INLINE idVec3 &idVec6::SubVec3( int index ) {
	return *reinterpret_cast<idVec3 *>(p + index * 3);
}

ID_INLINE const float *idVec6::ToFloatPtr( void ) const {
	return p;
}

ID_INLINE float *idVec6::ToFloatPtr( void ) {
	return p;
}


//===============================================================
//
//	idVecX - arbitrary sized vector
//
//  The vector lives on 16 byte aligned and 16 byte padded memory.
//
//	NOTE: due to the temporary memory pool idVecX cannot be used by multiple threads
//
//===============================================================

#define VECX_MAX_TEMP		1024
#define VECX_QUAD( x )		( ( ( ( x ) + 3 ) & ~3 ) * sizeof( float ) )
#define VECX_CLEAREND()		int s = size; while( s < ( ( s + 3) & ~3 ) ) { p[s++] = 0.0f; }
#define VECX_ALLOCA( n )	( (float *) _alloca16( VECX_QUAD( n ) ) )
#define VECX_SIMD

class idVecX {
	friend class idMatX;

public:	
					idVecX( void );
					explicit idVecX( int length );
					explicit idVecX( int length, float *data );
					~idVecX( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	idVecX			operator-() const;
	idVecX &		operator=( const idVecX &a );
	idVecX			operator*( const float a ) const;
	idVecX			operator/( const float a ) const;
	float			operator*( const idVecX &a ) const;
	idVecX			operator-( const idVecX &a ) const;
	idVecX			operator+( const idVecX &a ) const;
	idVecX &		operator*=( const float a );
	idVecX &		operator/=( const float a );
	idVecX &		operator+=( const idVecX &a );
	idVecX &		operator-=( const idVecX &a );

	friend idVecX	operator*( const float a, const idVecX b );

	bool			Compare( const idVecX &a ) const;							// exact compare, no epsilon
	bool			Compare( const idVecX &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const idVecX &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idVecX &a ) const;						// exact compare, no epsilon

	void			SetSize( int size );
	void			ChangeSize( int size, bool makeZero = false );
	int				GetSize( void ) const { return size; }
	void			SetData( int length, float *data );
	void			Zero( void );
	void			Zero( int length );
	void			Random( int seed, float l = 0.0f, float u = 1.0f );
	void			Random( int length, int seed, float l = 0.0f, float u = 1.0f );
	void			Negate( void );
	void			Clamp( float min, float max );
	idVecX &		SwapElements( int e1, int e2 );

	float			Length( void ) const;
	float			LengthSqr( void ) const;
	idVecX			Normalize( void ) const;
	float			NormalizeSelf( void );

	int				GetDimension( void ) const;

	const idVec3 &	SubVec3( int index ) const;
	idVec3 &		SubVec3( int index );
	const idVec6 &	SubVec6( int index ) const;
	idVec6 &		SubVec6( int index );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	int				size;					// size of the vector
	int				alloced;				// if -1 p points to data set with SetData
	float *			p;						// memory the vector is stored

	static float	temp[VECX_MAX_TEMP+4];	// used to store intermediate results
	static float *	tempPtr;				// pointer to 16 byte aligned temporary memory
	static int		tempIndex;				// index into memory pool, wraps around

private:
	void			SetTempSize( int size );
};


ID_INLINE idVecX::idVecX( void ) {
	size = alloced = 0;
	p = NULL;
}

ID_INLINE idVecX::idVecX( int length ) {
	size = alloced = 0;
	p = NULL;
	SetSize( length );
}

ID_INLINE idVecX::idVecX( int length, float *data ) {
	size = alloced = 0;
	p = NULL;
	SetData( length, data );
}

ID_INLINE idVecX::~idVecX( void ) {
	// if not temp memory
	if ( p && ( p < idVecX::tempPtr || p >= idVecX::tempPtr + VECX_MAX_TEMP ) && alloced != -1 ) {
		Mem_FreeAligned( p );
	}
}

ID_INLINE float idVecX::operator[]( const int index ) const {
	assert( index >= 0 && index < size );
	return p[index];
}

ID_INLINE float &idVecX::operator[]( const int index ) {
	assert( index >= 0 && index < size );
	return p[index];
}

ID_INLINE idVecX idVecX::operator-() const {
	int i;
	idVecX m;

	m.SetTempSize( size );
	for ( i = 0; i < size; i++ ) {
		m.p[i] = -p[i];
	}
	return m;
}

ID_INLINE idVecX &idVecX::operator=( const idVecX &a ) { 
	SetSize( a.size );
#ifdef VECX_SIMD
	SIMDProcessor->Copy16( p, a.p, a.size );
#else
	memcpy( p, a.p, a.size * sizeof( float ) );
#endif
	idVecX::tempIndex = 0;
	return *this;
}

ID_INLINE idVecX idVecX::operator+( const idVecX &a ) const {
	idVecX m;

	assert( size == a.size );
	m.SetTempSize( size );
#ifdef VECX_SIMD
	SIMDProcessor->Add16( m.p, p, a.p, size );
#else
	int i;
	for ( i = 0; i < size; i++ ) {
		m.p[i] = p[i] + a.p[i];
	}
#endif
	return m;
}

ID_INLINE idVecX idVecX::operator-( const idVecX &a ) const {
	idVecX m;

	assert( size == a.size );
	m.SetTempSize( size );
#ifdef VECX_SIMD
	SIMDProcessor->Sub16( m.p, p, a.p, size );
#else
	int i;
	for ( i = 0; i < size; i++ ) {
		m.p[i] = p[i] - a.p[i];
	}
#endif
	return m;
}

ID_INLINE idVecX &idVecX::operator+=( const idVecX &a ) {
	assert( size == a.size );
#ifdef VECX_SIMD
	SIMDProcessor->AddAssign16( p, a.p, size );
#else
	int i;
	for ( i = 0; i < size; i++ ) {
		p[i] += a.p[i];
	}
#endif
	idVecX::tempIndex = 0;
	return *this;
}

ID_INLINE idVecX &idVecX::operator-=( const idVecX &a ) {
	assert( size == a.size );
#ifdef VECX_SIMD
	SIMDProcessor->SubAssign16( p, a.p, size );
#else
	int i;
	for ( i = 0; i < size; i++ ) {
		p[i] -= a.p[i];
	}
#endif
	idVecX::tempIndex = 0;
	return *this;
}

ID_INLINE idVecX idVecX::operator*( const float a ) const {
	idVecX m;

	m.SetTempSize( size );
#ifdef VECX_SIMD
	SIMDProcessor->Mul16( m.p, p, a, size );
#else
	int i;
	for ( i = 0; i < size; i++ ) {
		m.p[i] = p[i] * a;
	}
#endif
	return m;
}

ID_INLINE idVecX &idVecX::operator*=( const float a ) {
#ifdef VECX_SIMD
	SIMDProcessor->MulAssign16( p, a, size );
#else
	int i;
	for ( i = 0; i < size; i++ ) {
		p[i] *= a;
	}
#endif
	return *this;
}

ID_INLINE idVecX idVecX::operator/( const float a ) const {
	assert( a != 0.0f );
	return (*this) * ( 1.0f / a );
}

ID_INLINE idVecX &idVecX::operator/=( const float a ) {
	assert( a != 0.0f );
	(*this) *= ( 1.0f / a );
	return *this;
}

ID_INLINE idVecX operator*( const float a, const idVecX b ) {
	return b * a;
}

ID_INLINE float idVecX::operator*( const idVecX &a ) const {
	int i;
	float sum = 0.0f;

	assert( size == a.size );
	for ( i = 0; i < size; i++ ) {
		sum += p[i] * a.p[i];
	}
	return sum;
}

ID_INLINE bool idVecX::Compare( const idVecX &a ) const {
	int i;

	assert( size == a.size );
	for ( i = 0; i < size; i++ ) {
		if ( p[i] != a.p[i] ) {
			return false;
		}
	}
	return true;
}

ID_INLINE bool idVecX::Compare( const idVecX &a, const float epsilon ) const {
	int i;

	assert( size == a.size );
	for ( i = 0; i < size; i++ ) {
		if ( idMath::Fabs( p[i] - a.p[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ID_INLINE bool idVecX::operator==( const idVecX &a ) const {
	return Compare( a );
}

ID_INLINE bool idVecX::operator!=( const idVecX &a ) const {
	return !Compare( a );
}

ID_INLINE void idVecX::SetSize( int newSize ) {
	//assert( p < idVecX::tempPtr || p > idVecX::tempPtr + VECX_MAX_TEMP );
	int alloc = ( newSize + 3 ) & ~3;
	if ( alloc > alloced && alloced != -1 ) {
		if ( p ) {
			Mem_FreeAligned( p );
		}
		p = (float *) Mem_AllocAligned( alloc * sizeof( float ), ALIGN_16 );
		alloced = alloc;
	}
	size = newSize;
	VECX_CLEAREND();
}

ID_INLINE void idVecX::ChangeSize( int newSize, bool makeZero ) {
	int alloc = ( newSize + 3 ) & ~3;
	if ( alloc > alloced && alloced != -1 ) {
		float *oldVec = p;
		p = (float *) Mem_AllocAligned( alloc * sizeof( float ), ALIGN_16 );
		alloced = alloc;
		if ( oldVec ) {
			for ( int i = 0; i < size; i++ ) {
				p[i] = oldVec[i];
			}
			Mem_FreeAligned( oldVec );
		}
		if ( makeZero ) {
			// zero any new elements
			for ( int i = size; i < newSize; i++ ) {
				p[i] = 0.0f;
			}
		}
	}
	size = newSize;
	VECX_CLEAREND();
}

ID_INLINE void idVecX::SetTempSize( int newSize ) {

	size = newSize;
	alloced = ( newSize + 3 ) & ~3;
	assert( alloced < VECX_MAX_TEMP );
	if ( idVecX::tempIndex + alloced > VECX_MAX_TEMP ) {
		idVecX::tempIndex = 0;
	}
	p = idVecX::tempPtr + idVecX::tempIndex;
	idVecX::tempIndex += alloced;
	VECX_CLEAREND();
}

ID_INLINE void idVecX::SetData( int length, float *data ) {
	if ( p && ( p < idVecX::tempPtr || p >= idVecX::tempPtr + VECX_MAX_TEMP ) && alloced != -1 ) {
		Mem_FreeAligned( p );
	}
	assert( ( ( (UINT_PTR) data ) & 15 ) == 0 ); // data must be 16 byte aligned
	p = data;
	size = length;
	alloced = -1;
	VECX_CLEAREND();
}

ID_INLINE void idVecX::Zero( void ) {
#ifdef VECX_SIMD
	SIMDProcessor->Zero16( p, size );
#else
	memset( p, 0, size * sizeof( float ) );
#endif
}

ID_INLINE void idVecX::Zero( int length ) {
	SetSize( length );
#ifdef VECX_SIMD
	SIMDProcessor->Zero16( p, length );
#else
	memset( p, 0, size * sizeof( float ) );
#endif
}

ID_INLINE void idVecX::Random( int seed, float l, float u ) {
	int i;
	float c;
	idRandom rnd( seed );

	c = u - l;
	for ( i = 0; i < size; i++ ) {
		p[i] = l + rnd.RandomFloat() * c;
	}
}

ID_INLINE void idVecX::Random( int length, int seed, float l, float u ) {
	int i;
	float c;
	idRandom rnd( seed );

	SetSize( length );
	c = u - l;
	for ( i = 0; i < size; i++ ) {
		p[i] = l + rnd.RandomFloat() * c;
	}
}

ID_INLINE void idVecX::Negate( void ) {
#ifdef VECX_SIMD
	SIMDProcessor->Negate16( p, size );
#else
	int i;
	for ( i = 0; i < size; i++ ) {
		p[i] = -p[i];
	}
#endif
}

ID_INLINE void idVecX::Clamp( float min, float max ) {
	int i;
	for ( i = 0; i < size; i++ ) {
		if ( p[i] < min ) {
			p[i] = min;
		} else if ( p[i] > max ) {
			p[i] = max;
		}
	}
}

ID_INLINE idVecX &idVecX::SwapElements( int e1, int e2 ) {
	float tmp;
	tmp = p[e1];
	p[e1] = p[e2];
	p[e2] = tmp;
	return *this;
}

ID_INLINE float idVecX::Length( void ) const {
	int i;
	float sum = 0.0f;

	for ( i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	return idMath::Sqrt( sum );
}

ID_INLINE float idVecX::LengthSqr( void ) const {
	int i;
	float sum = 0.0f;

	for ( i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	return sum;
}

ID_INLINE idVecX idVecX::Normalize( void ) const {
	int i;
	idVecX m;
	float invSqrt, sum = 0.0f;

	m.SetTempSize( size );
	for ( i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	invSqrt = idMath::InvSqrt( sum );
	for ( i = 0; i < size; i++ ) {
		m.p[i] = p[i] * invSqrt;
	}
	return m;
}

ID_INLINE float idVecX::NormalizeSelf( void ) {
	float invSqrt, sum = 0.0f;
	int i;
	for ( i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	invSqrt = idMath::InvSqrt( sum );
	for ( i = 0; i < size; i++ ) {
		p[i] *= invSqrt;
	}
	return invSqrt * sum;
}

ID_INLINE int idVecX::GetDimension( void ) const {
	return size;
}

ID_INLINE idVec3 &idVecX::SubVec3( int index ) {
	assert( index >= 0 && index * 3 + 3 <= size );
	return *reinterpret_cast<idVec3 *>(p + index * 3);
}

ID_INLINE const idVec3 &idVecX::SubVec3( int index ) const {
	assert( index >= 0 && index * 3 + 3 <= size );
	return *reinterpret_cast<const idVec3 *>(p + index * 3);
}

ID_INLINE idVec6 &idVecX::SubVec6( int index ) {
	assert( index >= 0 && index * 6 + 6 <= size );
	return *reinterpret_cast<idVec6 *>(p + index * 6);
}

ID_INLINE const idVec6 &idVecX::SubVec6( int index ) const {
	assert( index >= 0 && index * 6 + 6 <= size );
	return *reinterpret_cast<const idVec6 *>(p + index * 6);
}

ID_INLINE const float *idVecX::ToFloatPtr( void ) const {
	return p;
}

ID_INLINE float *idVecX::ToFloatPtr( void ) {
	return p;
}


//===============================================================
//
//	idPolar3
//
//===============================================================

class idPolar3 {
public:	
	float			radius, theta, phi;

					idPolar3( void );
					explicit idPolar3( const float radius, const float theta, const float phi );

	void 			Set( const float radius, const float theta, const float phi );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	idPolar3		operator-() const;
	idPolar3 &		operator=( const idPolar3 &a );

	idVec3			ToVec3( void ) const;
};

ID_INLINE idPolar3::idPolar3( void ) {
}

ID_INLINE idPolar3::idPolar3( const float radius, const float theta, const float phi ) {
	assert( radius > 0 );
	this->radius = radius;
	this->theta = theta;
	this->phi = phi;
}
	
ID_INLINE void idPolar3::Set( const float radius, const float theta, const float phi ) {
	assert( radius > 0 );
	this->radius = radius;
	this->theta = theta;
	this->phi = phi;
}

ID_INLINE float idPolar3::operator[]( const int index ) const {
	return ( &radius )[ index ];
}

ID_INLINE float &idPolar3::operator[]( const int index ) {
	return ( &radius )[ index ];
}

ID_INLINE idPolar3 idPolar3::operator-() const {
	return idPolar3( radius, -theta, -phi );
}

ID_INLINE idPolar3 &idPolar3::operator=( const idPolar3 &a ) { 
	radius = a.radius;
	theta = a.theta;
	phi = a.phi;
	return *this;
}

ID_INLINE idVec3 idPolar3::ToVec3( void ) const {
	float sp, cp, st, ct;
	idMath::SinCos( phi, sp, cp );
	idMath::SinCos( theta, st, ct );
 	return idVec3( cp * radius * ct, cp * radius * st, radius * sp );
}




#endif /* !__MATH_VECTOR_H__ */
