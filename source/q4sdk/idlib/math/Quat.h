
#ifndef __MATH_QUAT_H__
#define __MATH_QUAT_H__

/*
===============================================================================

	Quaternion

===============================================================================
*/


class idVec3;
class idAngles;
class idRotation;
class idMat3;
class idMat4;
class idCQuat;

class idQuat {
public:
	float			x;
	float			y;
	float			z;
	float			w;

					idQuat( void );
					idQuat( float x, float y, float z, float w );

	void 			Set( float x, float y, float z, float w );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	idQuat			operator-() const;
	idQuat &		operator=( const idQuat &a );
	idQuat			operator+( const idQuat &a ) const;
	idQuat &		operator+=( const idQuat &a );
	idQuat			operator-( const idQuat &a ) const;
	idQuat &		operator-=( const idQuat &a );
	idQuat			operator*( const idQuat &a ) const;
	idVec3			operator*( const idVec3 &a ) const;
	idQuat			operator*( float a ) const;
	idQuat &		operator*=( const idQuat &a );
	idQuat &		operator*=( float a );

	friend idQuat	operator*( const float a, const idQuat &b );
	friend idVec3	operator*( const idVec3 &a, const idQuat &b );

	bool			Compare( const idQuat &a ) const;						// exact compare, no epsilon
	bool			Compare( const idQuat &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idQuat &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const idQuat &a ) const;					// exact compare, no epsilon

	idQuat			Inverse( void ) const;
	float			Length( void ) const;
	idQuat &		Normalize( void );

	float			CalcW( void ) const;
	int				GetDimension( void ) const;

	idAngles		ToAngles( void ) const;
	idRotation		ToRotation( void ) const;
	idMat3			ToMat3( void ) const;
	idMat4			ToMat4( void ) const;
	idCQuat			ToCQuat( void ) const;
	idVec3			ToAngularVelocity( void ) const;
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	idQuat &		Slerp( const idQuat &from, const idQuat &to, float t );
};

ID_INLINE idQuat::idQuat( void ) {
}

ID_INLINE idQuat::idQuat( float x, float y, float z, float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ID_INLINE float idQuat::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[ index ];
}

ID_INLINE float& idQuat::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[ index ];
}

ID_INLINE idQuat idQuat::operator-() const {
	return idQuat( -x, -y, -z, -w );
}

ID_INLINE idQuat &idQuat::operator=( const idQuat &a ) {
	x = a.x;
	y = a.y;
	z = a.z;
	w = a.w;

	return *this;
}

ID_INLINE idQuat idQuat::operator+( const idQuat &a ) const {
	return idQuat( x + a.x, y + a.y, z + a.z, w + a.w );
}

ID_INLINE idQuat& idQuat::operator+=( const idQuat &a ) {
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

ID_INLINE idQuat idQuat::operator-( const idQuat &a ) const {
	return idQuat( x - a.x, y - a.y, z - a.z, w - a.w );
}

ID_INLINE idQuat& idQuat::operator-=( const idQuat &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

ID_INLINE idQuat idQuat::operator*( const idQuat &a ) const {
	return idQuat(	w*a.x + x*a.w + y*a.z - z*a.y,
					w*a.y + y*a.w + z*a.x - x*a.z,
					w*a.z + z*a.w + x*a.y - y*a.x,
					w*a.w - x*a.x - y*a.y - z*a.z );
}

ID_INLINE idVec3 idQuat::operator*( const idVec3 &a ) const {
#if 0
	// it's faster to do the conversion to a 3x3 matrix and multiply the vector by this 3x3 matrix
	return ( ToMat3() * a );
#else
	// result = this->Inverse() * idQuat( a.x, a.y, a.z, 0.0f ) * (*this)
	float xx = x * x;
	float zz = z * z;
	float ww = w * w;
	float yy = y * y;

	float xw2 = x*w*2.0f;
	float xy2 = x*y*2.0f;
	float xz2 = x*z*2.0f;
	float yw2 = y*w*2.0f;
	float yz2 = y*z*2.0f;
	float zw2 = z*w*2.0f;

	return idVec3(
		( xx - yy - zz + ww )	* a.x	+ ( xy2 + zw2 )			* a.y	+ ( xz2 - yw2 )			* a.z,
		( xy2 - zw2 )			* a.x	+ ( -xx + yy - zz + ww )* a.y	+ ( yz2 + xw2 )			* a.z,
		( xz2 + yw2 )			* a.x	+ ( yz2 - xw2 )			* a.y	+ ( -xx - yy + zz + ww )* a.z );
#endif
}

ID_INLINE idQuat idQuat::operator*( float a ) const {
	return idQuat( x * a, y * a, z * a, w * a );
}

ID_INLINE idQuat operator*( const float a, const idQuat &b ) {
	return b * a;
}

ID_INLINE idVec3 operator*( const idVec3 &a, const idQuat &b ) {
	return b * a;
}

ID_INLINE idQuat& idQuat::operator*=( const idQuat &a ) {
	*this = *this * a;

	return *this;
}

ID_INLINE idQuat& idQuat::operator*=( float a ) {
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

ID_INLINE bool idQuat::Compare( const idQuat &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) && ( w == a.w ) );
}

ID_INLINE bool idQuat::Compare( const idQuat &a, const float epsilon ) const {
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

ID_INLINE bool idQuat::operator==( const idQuat &a ) const {
	return Compare( a );
}

ID_INLINE bool idQuat::operator!=( const idQuat &a ) const {
	return !Compare( a );
}

ID_INLINE void idQuat::Set( float x, float y, float z, float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ID_INLINE idQuat idQuat::Inverse( void ) const {
	return idQuat( -x, -y, -z, w );
}

ID_INLINE float idQuat::Length( void ) const {
	float len;

	len = x * x + y * y + z * z + w * w;
	return idMath::Sqrt( len );
}

ID_INLINE idQuat& idQuat::Normalize( void ) {
	float len;
	float ilength;

	len = this->Length();
	if ( len ) {
		ilength = 1 / len;
		x *= ilength;
		y *= ilength;
		z *= ilength;
		w *= ilength;
	}
	return *this;
}

ID_INLINE float idQuat::CalcW( void ) const {
	// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
	return idMath::Sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) );
}

ID_INLINE int idQuat::GetDimension( void ) const {
	return 4;
}

ID_INLINE const float *idQuat::ToFloatPtr( void ) const {
	return &x;
}

ID_INLINE float *idQuat::ToFloatPtr( void ) {
	return &x;
}


/*
===============================================================================

	Compressed quaternion

===============================================================================
*/

class idCQuat {
public:
	float			x;
	float			y;
	float			z;

					idCQuat( void );
					idCQuat( float x, float y, float z );

	void 			Set( float x, float y, float z );

	float			operator[]( int index ) const;
	float &			operator[]( int index );

	bool			Compare( const idCQuat &a ) const;						// exact compare, no epsilon
	bool			Compare( const idCQuat &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idCQuat &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const idCQuat &a ) const;					// exact compare, no epsilon

	int				GetDimension( void ) const;

	idAngles		ToAngles( void ) const;
	idRotation		ToRotation( void ) const;
	idMat3			ToMat3( void ) const;
	idMat4			ToMat4( void ) const;
	idQuat			ToQuat( void ) const;
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;
};

ID_INLINE idCQuat::idCQuat( void ) {
}

ID_INLINE idCQuat::idCQuat( float x, float y, float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

ID_INLINE void idCQuat::Set( float x, float y, float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

ID_INLINE float idCQuat::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &x )[ index ];
}

ID_INLINE float& idCQuat::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &x )[ index ];
}

ID_INLINE bool idCQuat::Compare( const idCQuat &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) );
}

ID_INLINE bool idCQuat::Compare( const idCQuat &a, const float epsilon ) const {
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

ID_INLINE bool idCQuat::operator==( const idCQuat &a ) const {
	return Compare( a );
}

ID_INLINE bool idCQuat::operator!=( const idCQuat &a ) const {
	return !Compare( a );
}

ID_INLINE int idCQuat::GetDimension( void ) const {
	return 3;
}

ID_INLINE idQuat idCQuat::ToQuat( void ) const {
	// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
	return idQuat( x, y, z, idMath::Sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) ) );
}

ID_INLINE const float *idCQuat::ToFloatPtr( void ) const {
	return &x;
}

ID_INLINE float *idCQuat::ToFloatPtr( void ) {
	return &x;
}

#endif /* !__MATH_QUAT_H__ */
