#ifndef _MATH_RADIANS_H_INC_
#define _MATH_RADIANS_H_INC_

/*
===============================================================================
	Euler angles

	This is basically a duplicate of idAngles, but used radians rather than 
	degrees (to avoid the conversion before trig calls)

	All trig calls use float precision

	ToMat3 passes in workspace to avoid a memcpy

===============================================================================
*/

#ifndef M_PI
#  define M_PI (3.1415926536f)
#endif

class idVec3;
class idMat3;

class rvAngles 
{
public:
	float			pitch;
	float			yaw;
	float			roll;

					rvAngles( void ) {}
					rvAngles( float pitch, float yaw, float roll );
	explicit		rvAngles( const idVec3 &v );

	void 			Set( float pitch, float yaw, float roll );
	rvAngles &		Zero( void );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	rvAngles		operator-() const;
	rvAngles &		operator=( const rvAngles &a );
	rvAngles &		operator=( const idVec3 &a );
	rvAngles		operator+( const rvAngles &a ) const;
	rvAngles		operator+( const idVec3 &a ) const;
	rvAngles &		operator+=( const rvAngles &a );
	rvAngles &		operator+=( const idVec3 &a );
	rvAngles		operator-( const rvAngles &a ) const;
	rvAngles		operator-( const idVec3 &a ) const;
	rvAngles &		operator-=( const rvAngles &a );
	rvAngles &		operator-=( const idVec3 &a );
	rvAngles		operator*( const float a ) const;
	rvAngles &		operator*=( const float a );

	friend rvAngles	operator+( const idVec3 &a, const rvAngles &b );
	friend rvAngles	operator-( const idVec3 &a, const rvAngles &b );
	friend rvAngles	operator*( const float a, const rvAngles &b );

	bool			Compare( const rvAngles &a ) const;							// exact compare, no epsilon
	bool			Compare( const rvAngles &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const rvAngles &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const rvAngles &a ) const;						// exact compare, no epsilon

	rvAngles &		NormalizeFull( void );	// normalizes 'this'
	rvAngles &		NormalizeHalf( void );	// normalizes 'this'

	void			ToVectors( idVec3 *forward, idVec3 *right = NULL, idVec3 *up = NULL ) const;
	idVec3			ToForward( void ) const;
	idMat3			&ToMat3( idMat3 &mat ) const;

	const float *	ToFloatPtr( void ) const { return( &pitch ); }
	float *			ToFloatPtr( void ) { return( &pitch ); }
};

ID_INLINE rvAngles::rvAngles( float pitch, float yaw, float roll ) 
{
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ID_INLINE rvAngles::rvAngles( const idVec3 &v ) 
{
	this->pitch = v[0];
	this->yaw	= v[1];
	this->roll	= v[2];
}

ID_INLINE void rvAngles::Set( float pitch, float yaw, float roll ) 
{
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ID_INLINE rvAngles &rvAngles::Zero( void ) 
{
	pitch = 0.0f;
	yaw = 0.0f;
	roll = 0.0f;
	return( *this );
}

ID_INLINE float rvAngles::operator[]( int index ) const 
{
	assert( ( index >= 0 ) && ( index < 3 ) );
	return( ( &pitch )[ index ] );
}

ID_INLINE float &rvAngles::operator[]( int index ) 
{
	assert( ( index >= 0 ) && ( index < 3 ) );
	return( ( &pitch )[ index ] );
}

ID_INLINE rvAngles rvAngles::operator-( void ) const 
{
	return( rvAngles( -pitch, -yaw, -roll ) );
}

ID_INLINE rvAngles &rvAngles::operator=( const rvAngles &a ) 
{
	pitch = a.pitch;
	yaw = a.yaw;
	roll = a.roll;
	return( *this );
}

ID_INLINE rvAngles &rvAngles::operator=( const idVec3 &a ) 
{
	pitch = a.x;
	yaw = a.y;
	roll = a.z;
	return( *this );
}

ID_INLINE rvAngles rvAngles::operator+( const rvAngles &a ) const 
{
	return( rvAngles( pitch + a.pitch, yaw + a.yaw, roll + a.roll ) );
}

ID_INLINE rvAngles rvAngles::operator+( const idVec3 &a ) const 
{
	return( rvAngles( pitch + a.x, yaw + a.y, roll + a.z ) );
}

ID_INLINE rvAngles& rvAngles::operator+=( const rvAngles &a ) 
{
	pitch += a.pitch;
	yaw += a.yaw;
	roll += a.roll;
	return( *this );
}

ID_INLINE rvAngles& rvAngles::operator+=( const idVec3 &a ) 
{
	pitch += a.x;
	yaw += a.y;
	roll += a.z;
	return( *this );
}

ID_INLINE rvAngles rvAngles::operator-( const rvAngles &a ) const 
{
	return( rvAngles( pitch - a.pitch, yaw - a.yaw, roll - a.roll ) );
}

ID_INLINE rvAngles rvAngles::operator-( const idVec3 &a ) const 
{
	return( rvAngles( pitch - a.x, yaw - a.y, roll - a.z ) );
}

ID_INLINE rvAngles& rvAngles::operator-=( const rvAngles &a ) 
{
	pitch -= a.pitch;
	yaw -= a.yaw;
	roll -= a.roll;
	return( *this );
}

ID_INLINE rvAngles& rvAngles::operator-=( const idVec3 &a ) 
{
	pitch -= a.x;
	yaw -= a.y;
	roll -= a.z;
	return( *this );
}

ID_INLINE rvAngles rvAngles::operator*( const float a ) const 
{
	return( rvAngles( pitch * a, yaw * a, roll * a ) );
}

ID_INLINE rvAngles& rvAngles::operator*=( float a ) 
{
	pitch *= a;
	yaw *= a;
	roll *= a;
	return( *this );
}

ID_INLINE rvAngles operator+( const idVec3 &a, const rvAngles &b ) 
{
	return( rvAngles( a.x + b.pitch, a.y + b.yaw, a.z + b.roll ) );
}

ID_INLINE rvAngles operator-( const idVec3 &a, const rvAngles &b ) 
{
	return( rvAngles( a.x - b.pitch, a.y - b.yaw, a.z - b.roll ) ); 
}

ID_INLINE rvAngles operator*( const float a, const rvAngles &b ) 
{
	return( rvAngles( a * b.pitch, a * b.yaw, a * b.roll ) );
}

ID_INLINE bool rvAngles::Compare( const rvAngles &a ) const 
{
	return( ( a.pitch == pitch ) && ( a.yaw == yaw ) && ( a.roll == roll ) );
}

ID_INLINE bool rvAngles::Compare( const rvAngles &a, const float epsilon ) const 
{
	if( idMath::Fabs( pitch - a.pitch ) > epsilon ) 
	{
		return( false );
	}
			
	if( idMath::Fabs( yaw - a.yaw ) > epsilon ) 
	{
		return( false );
	}

	if( idMath::Fabs( roll - a.roll ) > epsilon ) 
	{
		return( false );
	}

	return( true );
}

ID_INLINE bool rvAngles::operator==( const rvAngles &a ) const 
{
	return( Compare( a ) );
}

ID_INLINE bool rvAngles::operator!=( const rvAngles &a ) const 
{
	return( !Compare( a ) );
}

#endif // _MATH_RADIANS_H_INC_
