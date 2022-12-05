
#ifndef __MATH_PLUECKER_H__
#define __MATH_PLUECKER_H__

/*
===============================================================================

	Pluecker coordinate

===============================================================================
*/

class idPluecker {
public:	
					idPluecker( void );
					explicit idPluecker( const float *a );
					explicit idPluecker( const idVec3 &start, const idVec3 &end );
					explicit idPluecker( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	idPluecker		operator-() const;											// flips the direction
	idPluecker		operator*( const float a ) const;
	idPluecker		operator/( const float a ) const;
	float			operator*( const idPluecker &a ) const;						// permuted inner product
	idPluecker		operator-( const idPluecker &a ) const;
	idPluecker		operator+( const idPluecker &a ) const;
	idPluecker &	operator*=( const float a );
	idPluecker &	operator/=( const float a );
	idPluecker &	operator+=( const idPluecker &a );
	idPluecker &	operator-=( const idPluecker &a );

	bool			Compare( const idPluecker &a ) const;						// exact compare, no epsilon
	bool			Compare( const idPluecker &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idPluecker &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const idPluecker &a ) const;					// exact compare, no epsilon

	void 			Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );
	void			Zero( void );

	idPluecker		Rotate( const idMat3 &rotation ) const;
	idPluecker		Translate( const idVec3 &translation ) const;
	idPluecker		TranslateAndRotate( const idVec3 &translation, const idMat3 &rotation ) const;

	void			FromLine( const idVec3 &start, const idVec3 &end );			// pluecker from line
	void			FromRay( const idVec3 &start, const idVec3 &dir );			// pluecker from ray
	bool			FromPlanes( const idPlane &p1, const idPlane &p2 );			// pluecker from intersection of planes
	bool			ToLine( idVec3 &start, idVec3 &end ) const;					// pluecker to line
	bool			ToRay( idVec3 &start, idVec3 &dir ) const;					// pluecker to ray
	void			ToDir( idVec3 &dir ) const;									// pluecker to direction
	float			PermutedInnerProduct( const idPluecker &a ) const;			// pluecker permuted inner product
	float			Distance3DSqr( const idPluecker &a ) const;					// pluecker line distance

	float			Length( void ) const;										// pluecker length
	float			LengthSqr( void ) const;									// pluecker squared length
	idPluecker		Normalize( void ) const;									// pluecker normalize
	float			NormalizeSelf( void );										// pluecker normalize

	int				GetDimension( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	float			p[6];
};

extern idPluecker pluecker_origin;
#define pluecker_zero pluecker_origin

ID_INLINE idPluecker::idPluecker( void ) {
}

ID_INLINE idPluecker::idPluecker( const float *a ) {
	memcpy( p, a, 6 * sizeof( float ) );
}

ID_INLINE idPluecker::idPluecker( const idVec3 &start, const idVec3 &end ) {
	FromLine( start, end );
}

ID_INLINE idPluecker::idPluecker( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ID_INLINE idPluecker idPluecker::operator-() const {
	return idPluecker( -p[0], -p[1], -p[2], -p[3], -p[4], -p[5] );
}

ID_INLINE float idPluecker::operator[]( const int index ) const {
	return p[index];
}

ID_INLINE float &idPluecker::operator[]( const int index ) {
	return p[index];
}

ID_INLINE idPluecker idPluecker::operator*( const float a ) const {
	return idPluecker( p[0]*a, p[1]*a, p[2]*a, p[3]*a, p[4]*a, p[5]*a );
}

ID_INLINE float idPluecker::operator*( const idPluecker &a ) const {
	return p[0] * a.p[4] + p[1] * a.p[5] + p[2] * a.p[3] + p[4] * a.p[0] + p[5] * a.p[1] + p[3] * a.p[2];
}

ID_INLINE idPluecker idPluecker::operator/( const float a ) const {
	float inva;

	assert( a != 0.0f );
	inva = 1.0f / a;
	return idPluecker( p[0]*inva, p[1]*inva, p[2]*inva, p[3]*inva, p[4]*inva, p[5]*inva );
}

ID_INLINE idPluecker idPluecker::operator+( const idPluecker &a ) const {
	return idPluecker( p[0] + a[0], p[1] + a[1], p[2] + a[2], p[3] + a[3], p[4] + a[4], p[5] + a[5] );
}

ID_INLINE idPluecker idPluecker::operator-( const idPluecker &a ) const {
	return idPluecker( p[0] - a[0], p[1] - a[1], p[2] - a[2], p[3] - a[3], p[4] - a[4], p[5] - a[5] );
}

ID_INLINE idPluecker &idPluecker::operator*=( const float a ) {
	p[0] *= a;
	p[1] *= a;
	p[2] *= a;
	p[3] *= a;
	p[4] *= a;
	p[5] *= a;
	return *this;
}

ID_INLINE idPluecker &idPluecker::operator/=( const float a ) {
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

ID_INLINE idPluecker &idPluecker::operator+=( const idPluecker &a ) {
	p[0] += a[0];
	p[1] += a[1];
	p[2] += a[2];
	p[3] += a[3];
	p[4] += a[4];
	p[5] += a[5];
	return *this;
}

ID_INLINE idPluecker &idPluecker::operator-=( const idPluecker &a ) {
	p[0] -= a[0];
	p[1] -= a[1];
	p[2] -= a[2];
	p[3] -= a[3];
	p[4] -= a[4];
	p[5] -= a[5];
	return *this;
}

ID_INLINE bool idPluecker::Compare( const idPluecker &a ) const {
	return ( ( p[0] == a[0] ) && ( p[1] == a[1] ) && ( p[2] == a[2] ) &&
			( p[3] == a[3] ) && ( p[4] == a[4] ) && ( p[5] == a[5] ) );
}

ID_INLINE bool idPluecker::Compare( const idPluecker &a, const float epsilon ) const {
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

ID_INLINE bool idPluecker::operator==( const idPluecker &a ) const {
	return Compare( a );
}

ID_INLINE bool idPluecker::operator!=( const idPluecker &a ) const {
	return !Compare( a );
}

ID_INLINE void idPluecker::Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ID_INLINE void idPluecker::Zero( void ) {
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0.0f;
}

ID_INLINE idPluecker idPluecker::Rotate( const idMat3 &rotation ) const {
	idPluecker result;
	result[0] =   rotation[ 0 ].z * p[3] - rotation[ 1 ].z * p[1] + rotation[ 2 ].z * p[0];
	result[1] = - rotation[ 0 ].y * p[3] + rotation[ 1 ].y * p[1] - rotation[ 2 ].y * p[0];
	result[2] =   rotation[ 0 ].x * p[2] - rotation[ 1 ].x * p[5] + rotation[ 2 ].x * p[4];
	result[3] =   rotation[ 0 ].x * p[3] - rotation[ 1 ].x * p[1] + rotation[ 2 ].x * p[0];
	result[4] =   rotation[ 0 ].z * p[2] - rotation[ 1 ].z * p[5] + rotation[ 2 ].z * p[4];
	result[5] = - rotation[ 0 ].y * p[2] + rotation[ 1 ].y * p[5] - rotation[ 2 ].y * p[4];
	return result;
}

ID_INLINE idPluecker idPluecker::Translate( const idVec3 &translation ) const {
	idPluecker result;
	result[0] = p[0] + translation[0] * p[5] + p[2] * translation[1];
	result[1] = p[1] - translation[0] * p[4] + p[2] * translation[2];
	result[2] = p[2];
	result[3] = p[3] - translation[1] * p[4] - p[5] * translation[2];
	result[4] = p[4];
	result[5] = p[5];
	return result;
}

ID_INLINE idPluecker idPluecker::TranslateAndRotate( const idVec3 &translation, const idMat3 &rotation ) const {
	idVec3 t;
	idPluecker result;
	t[0] = p[0] + translation[0] * p[5] + p[2] * translation[1];
	t[1] = p[1] - translation[0] * p[4] + p[2] * translation[2];
	t[2] = p[3] - translation[1] * p[4] - p[5] * translation[2];
	result[0] =   rotation[ 0 ].z * t[2] - rotation[ 1 ].z * t[1] + rotation[ 2 ].z * t[0];
	result[1] = - rotation[ 0 ].y * t[2] + rotation[ 1 ].y * t[1] - rotation[ 2 ].y * t[0];
	result[2] =   rotation[ 0 ].x * p[2] - rotation[ 1 ].x * p[5] + rotation[ 2 ].x * p[4];
	result[3] =   rotation[ 0 ].x * t[2] - rotation[ 1 ].x * t[1] + rotation[ 2 ].x * t[0];
	result[4] =   rotation[ 0 ].z * p[2] - rotation[ 1 ].z * p[5] + rotation[ 2 ].z * p[4];
	result[5] = - rotation[ 0 ].y * p[2] + rotation[ 1 ].y * p[5] - rotation[ 2 ].y * p[4];
	return result;
}

ID_INLINE void idPluecker::FromLine( const idVec3 &start, const idVec3 &end ) {
	p[0] = start[0] * end[1] - end[0] * start[1];
	p[1] = start[0] * end[2] - end[0] * start[2];
	p[2] = start[0] - end[0];
	p[3] = start[1] * end[2] - end[1] * start[2];
	p[4] = start[2] - end[2];
	p[5] = end[1] - start[1];
}

ID_INLINE void idPluecker::FromRay( const idVec3 &start, const idVec3 &dir ) {
	p[0] = start[0] * dir[1] - dir[0] * start[1];
	p[1] = start[0] * dir[2] - dir[0] * start[2];
	p[2] = -dir[0];
	p[3] = start[1] * dir[2] - dir[1] * start[2];
	p[4] = -dir[2];
	p[5] = dir[1];
}

ID_INLINE bool idPluecker::ToLine( idVec3 &start, idVec3 &end ) const {
	idVec3 dir1, dir2;
	float d;

	dir1[0] = p[3];
	dir1[1] = -p[1];
	dir1[2] = p[0];

	dir2[0] = -p[2];
	dir2[1] = p[5];
	dir2[2] = -p[4];

	d = dir2 * dir2;
	if ( d == 0.0f ) {
		return false; // pluecker coordinate does not represent a line
	}

	start = dir2.Cross(dir1) * (1.0f / d);
	end = start + dir2;
	return true;
}

ID_INLINE bool idPluecker::ToRay( idVec3 &start, idVec3 &dir ) const {
	idVec3 dir1;
	float d;

	dir1[0] = p[3];
	dir1[1] = -p[1];
	dir1[2] = p[0];

	dir[0] = -p[2];
	dir[1] = p[5];
	dir[2] = -p[4];

	d = dir * dir;
	if ( d == 0.0f ) {
		return false; // pluecker coordinate does not represent a line
	}

	start = dir.Cross(dir1) * (1.0f / d);
	return true;
}

ID_INLINE void idPluecker::ToDir( idVec3 &dir ) const {
	dir[0] = -p[2];
	dir[1] = p[5];
	dir[2] = -p[4];
}

ID_INLINE float idPluecker::PermutedInnerProduct( const idPluecker &a ) const {
	return p[0] * a.p[4] + p[1] * a.p[5] + p[2] * a.p[3] + p[4] * a.p[0] + p[5] * a.p[1] + p[3] * a.p[2];
}

ID_INLINE float idPluecker::Length( void ) const {
	return ( float )idMath::Sqrt( p[5] * p[5] + p[4] * p[4] + p[2] * p[2] );
}

ID_INLINE float idPluecker::LengthSqr( void ) const {
	return ( p[5] * p[5] + p[4] * p[4] + p[2] * p[2] );
}

ID_INLINE float idPluecker::NormalizeSelf( void ) {
	float l, d;

	l = LengthSqr();
	if ( l == 0.0f ) {
		return l; // pluecker coordinate does not represent a line
	}
	d = idMath::InvSqrt( l );
	p[0] *= d;
	p[1] *= d;
	p[2] *= d;
	p[3] *= d;
	p[4] *= d;
	p[5] *= d;
	return d * l;
}

ID_INLINE idPluecker idPluecker::Normalize( void ) const {
	float d;

	d = LengthSqr();
	if ( d == 0.0f ) {
		return *this; // pluecker coordinate does not represent a line
	}
	d = idMath::InvSqrt( d );
	return idPluecker( p[0]*d, p[1]*d, p[2]*d, p[3]*d, p[4]*d, p[5]*d );
}

ID_INLINE int idPluecker::GetDimension( void ) const {
	return 6;
}

ID_INLINE const float *idPluecker::ToFloatPtr( void ) const {
	return p;
}

ID_INLINE float *idPluecker::ToFloatPtr( void ) {
	return p;
}

#endif /* !__MATH_PLUECKER_H__ */
