
#ifndef __MAT3X4_H__
#define __MAT3X4_H__

/*
===============================================================================

  row-major 3x4 matrix

  idMat3 m;
  idVec3 t;

  m[0][0], m[1][0], m[2][0], t[0]
  m[0][1], m[1][1], m[2][1], t[1]
  m[0][2], m[1][2], m[2][2], t[2]

===============================================================================
*/

class idMat3x4 {
public:

	void			SetRotation( const idMat3 &m );
	void			SetTranslation( const idVec3 &t );

	bool			Compare( const idMat3x4 &a ) const;							// exact compare, no epsilon
	bool			Compare( const idMat3x4 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idMat3x4 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idMat3x4 &a ) const;						// exact compare, no epsilon

	void			Identity( void );
	void			Invert( void );

	void			LeftMultiply( const idMat3x4 &m );
	void			LeftMultiply( const idMat3 &m );
	void			RightMultiply( const idMat3x4 &m );
	void			RightMultiply( const idMat3 &m );

	void			Transform( idVec3 &result, const idVec3 &v ) const;
	void			Rotate( idVec3 &result, const idVec3 &v ) const;

	idMat3			ToMat3( void ) const;
	idVec3			ToVec3( void ) const;
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	float			mat[3*4];
};


ID_INLINE void idMat3x4::SetRotation( const idMat3 &m ) {
	// NOTE: idMat3 is transposed because it is column-major
	mat[0 * 4 + 0] = m[0][0];
	mat[0 * 4 + 1] = m[1][0];
	mat[0 * 4 + 2] = m[2][0];
	mat[1 * 4 + 0] = m[0][1];
	mat[1 * 4 + 1] = m[1][1];
	mat[1 * 4 + 2] = m[2][1];
	mat[2 * 4 + 0] = m[0][2];
	mat[2 * 4 + 1] = m[1][2];
	mat[2 * 4 + 2] = m[2][2];
}

ID_INLINE void idMat3x4::SetTranslation( const idVec3 &t ) {
	mat[0 * 4 + 3] = t[0];
	mat[1 * 4 + 3] = t[1];
	mat[2 * 4 + 3] = t[2];
}

ID_INLINE bool idMat3x4::Compare( const idMat3x4 &a ) const {
	int i;

	for ( i = 0; i < 12; i++ ) {
		if ( mat[i] != a.mat[i] ) {
			return false;
		}
	}
	return true;
}

ID_INLINE bool idMat3x4::Compare( const idMat3x4 &a, const float epsilon ) const {
	int i;

	for ( i = 0; i < 12; i++ ) {
		if ( idMath::Fabs( mat[i] - a.mat[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ID_INLINE bool idMat3x4::operator==( const idMat3x4 &a ) const {
	return Compare( a );
}

ID_INLINE bool idMat3x4::operator!=( const idMat3x4 &a ) const {
	return !Compare( a );
}

ID_INLINE void idMat3x4::Identity( void ) {
	mat[0 * 4 + 0] = 1.0f; mat[0 * 4 + 1] = 0.0f; mat[0 * 4 + 2] = 0.0f; mat[0 * 4 + 3] = 0.0f;
	mat[0 * 4 + 0] = 0.0f; mat[0 * 4 + 1] = 1.0f; mat[0 * 4 + 2] = 0.0f; mat[0 * 4 + 3] = 0.0f;
	mat[0 * 4 + 0] = 0.0f; mat[0 * 4 + 1] = 0.0f; mat[0 * 4 + 2] = 1.0f; mat[0 * 4 + 3] = 0.0f;
}

ID_INLINE void idMat3x4::Invert( void ) {
	float tmp[3];

	// negate inverse rotated translation part
	tmp[0] = mat[0 * 4 + 0] * mat[0 * 4 + 3] + mat[1 * 4 + 0] * mat[1 * 4 + 3] + mat[2 * 4 + 0] * mat[2 * 4 + 3];
	tmp[1] = mat[0 * 4 + 1] * mat[0 * 4 + 3] + mat[1 * 4 + 1] * mat[1 * 4 + 3] + mat[2 * 4 + 1] * mat[2 * 4 + 3];
	tmp[2] = mat[0 * 4 + 2] * mat[0 * 4 + 3] + mat[1 * 4 + 2] * mat[1 * 4 + 3] + mat[2 * 4 + 2] * mat[2 * 4 + 3];
	mat[0 * 4 + 3] = -tmp[0];
	mat[1 * 4 + 3] = -tmp[1];
	mat[2 * 4 + 3] = -tmp[2];

	// transpose rotation part
	tmp[0] = mat[0 * 4 + 1];
	mat[0 * 4 + 1] = mat[1 * 4 + 0];
	mat[1 * 4 + 0] = tmp[0];
	tmp[1] = mat[0 * 4 + 2];
	mat[0 * 4 + 2] = mat[2 * 4 + 0];
	mat[2 * 4 + 0] = tmp[1];
	tmp[2] = mat[1 * 4 + 2];
	mat[1 * 4 + 2] = mat[2 * 4 + 1];
	mat[2 * 4 + 1] = tmp[2];
}

ID_INLINE void idMat3x4::LeftMultiply( const idMat3x4 &m ) {
	float t0, t1;

	t0				= m.mat[0 * 4 + 0] * mat[0 * 4 + 0] + m.mat[0 * 4 + 1] * mat[1 * 4 + 0] + m.mat[0 * 4 + 2] * mat[2 * 4 + 0];
	t1				= m.mat[1 * 4 + 0] * mat[0 * 4 + 0] + m.mat[1 * 4 + 1] * mat[1 * 4 + 0] + m.mat[1 * 4 + 2] * mat[2 * 4 + 0];
	mat[2 * 4 + 0]	= m.mat[2 * 4 + 0] * mat[0 * 4 + 0] + m.mat[2 * 4 + 1] * mat[1 * 4 + 0] + m.mat[2 * 4 + 2] * mat[2 * 4 + 0];

	mat[1 * 4 + 0] = t1;
	mat[0 * 4 + 0] = t0;

	t0				= m.mat[0 * 4 + 0] * mat[0 * 4 + 1] + m.mat[0 * 4 + 1] * mat[1 * 4 + 1] + m.mat[0 * 4 + 2] * mat[2 * 4 + 1];
	t1				= m.mat[1 * 4 + 0] * mat[0 * 4 + 1] + m.mat[1 * 4 + 1] * mat[1 * 4 + 1] + m.mat[1 * 4 + 2] * mat[2 * 4 + 1];
	mat[2 * 4 + 1]	= m.mat[2 * 4 + 0] * mat[0 * 4 + 1] + m.mat[2 * 4 + 1] * mat[1 * 4 + 1] + m.mat[2 * 4 + 2] * mat[2 * 4 + 1];

	mat[1 * 4 + 1] = t1;
	mat[0 * 4 + 1] = t0;

	t0				= m.mat[0 * 4 + 0] * mat[0 * 4 + 2] + m.mat[0 * 4 + 1] * mat[1 * 4 + 2] + m.mat[0 * 4 + 2] * mat[2 * 4 + 2];
	t1				= m.mat[1 * 4 + 0] * mat[0 * 4 + 2] + m.mat[1 * 4 + 1] * mat[1 * 4 + 2] + m.mat[1 * 4 + 2] * mat[2 * 4 + 2];
	mat[2 * 4 + 2]	= m.mat[2 * 4 + 0] * mat[0 * 4 + 2] + m.mat[2 * 4 + 1] * mat[1 * 4 + 2] + m.mat[2 * 4 + 2] * mat[2 * 4 + 2];

	mat[1 * 4 + 2] = t1;
	mat[0 * 4 + 2] = t0;

	t0				= m.mat[0 * 4 + 0] * mat[0 * 4 + 3] + m.mat[0 * 4 + 1] * mat[1 * 4 + 3] + m.mat[0 * 4 + 2] * mat[2 * 4 + 3] + m.mat[0 * 4 + 3];
	t1				= m.mat[1 * 4 + 0] * mat[0 * 4 + 3] + m.mat[1 * 4 + 1] * mat[1 * 4 + 3] + m.mat[1 * 4 + 2] * mat[2 * 4 + 3] + m.mat[1 * 4 + 3];
	mat[2 * 4 + 3]	= m.mat[2 * 4 + 0] * mat[0 * 4 + 3] + m.mat[2 * 4 + 1] * mat[1 * 4 + 3] + m.mat[2 * 4 + 2] * mat[2 * 4 + 3] + m.mat[2 * 4 + 3];

	mat[1 * 4 + 3] = t1;
	mat[0 * 4 + 3] = t0;
}

ID_INLINE void idMat3x4::LeftMultiply( const idMat3 &m ) {
	float t0, t1;

	// NOTE: idMat3 is column-major
	t0				= m[0][0] * mat[0 * 4 + 0] + m[1][0] * mat[1 * 4 + 0] + m[2][0] * mat[2 * 4 + 0];
	t1				= m[0][1] * mat[0 * 4 + 0] + m[1][1] * mat[1 * 4 + 0] + m[2][1] * mat[2 * 4 + 0];
	mat[2 * 4 + 0]	= m[0][2] * mat[0 * 4 + 0] + m[1][2] * mat[1 * 4 + 0] + m[2][2] * mat[2 * 4 + 0];

	mat[1 * 4 + 0] = t1;
	mat[0 * 4 + 0] = t0;

	t0				= m[0][0] * mat[0 * 4 + 1] + m[1][0] * mat[1 * 4 + 1] + m[2][0] * mat[2 * 4 + 1];
	t1				= m[0][1] * mat[0 * 4 + 1] + m[1][1] * mat[1 * 4 + 1] + m[2][1] * mat[2 * 4 + 1];
	mat[2 * 4 + 1]	= m[0][2] * mat[0 * 4 + 1] + m[1][2] * mat[1 * 4 + 1] + m[2][2] * mat[2 * 4 + 1];

	mat[1 * 4 + 1] = t1;
	mat[0 * 4 + 1] = t0;

	t0				= m[0][0] * mat[0 * 4 + 2] + m[1][0] * mat[1 * 4 + 2] + m[2][0] * mat[2 * 4 + 2];
	t1				= m[0][1] * mat[0 * 4 + 2] + m[1][1] * mat[1 * 4 + 2] + m[2][1] * mat[2 * 4 + 2];
	mat[2 * 4 + 2]	= m[0][2] * mat[0 * 4 + 2] + m[1][2] * mat[1 * 4 + 2] + m[2][2] * mat[2 * 4 + 2];

	mat[1 * 4 + 2] = t1;
	mat[0 * 4 + 2] = t0;

	t0				= m[0][0] * mat[0 * 4 + 3] + m[1][0] * mat[1 * 4 + 3] + m[2][0] * mat[2 * 4 + 3];
	t1				= m[0][1] * mat[0 * 4 + 3] + m[1][1] * mat[1 * 4 + 3] + m[2][1] * mat[2 * 4 + 3];
	mat[2 * 4 + 3]	= m[0][2] * mat[0 * 4 + 3] + m[1][2] * mat[1 * 4 + 3] + m[2][2] * mat[2 * 4 + 3];

	mat[1 * 4 + 3] = t1;
	mat[0 * 4 + 3] = t0;
}

ID_INLINE void idMat3x4::RightMultiply( const idMat3x4 &m ) {
	float t0, t1, t2;

	t0				= mat[0 * 4 + 0] * m.mat[0 * 4 + 0] + mat[0 * 4 + 1] * m.mat[1 * 4 + 0] + mat[0 * 4 + 2] * m.mat[2 * 4 + 0];
	t1				= mat[0 * 4 + 0] * m.mat[0 * 4 + 1] + mat[0 * 4 + 1] * m.mat[1 * 4 + 1] + mat[0 * 4 + 2] * m.mat[2 * 4 + 1];
	t2				= mat[0 * 4 + 0] * m.mat[0 * 4 + 2] + mat[0 * 4 + 1] * m.mat[1 * 4 + 2] + mat[0 * 4 + 2] * m.mat[2 * 4 + 2];
	mat[0 * 4 + 3]	= mat[0 * 4 + 0] * m.mat[0 * 4 + 3] + mat[0 * 4 + 1] * m.mat[1 * 4 + 3] + mat[0 * 4 + 2] * m.mat[2 * 4 + 3] + mat[0 * 4 + 3];

	mat[0 * 4 + 0] = t0;
	mat[0 * 4 + 1] = t1;
	mat[0 * 4 + 2] = t2;

	t0				= mat[1 * 4 + 0] * m.mat[0 * 4 + 0] + mat[1 * 4 + 1] * m.mat[1 * 4 + 0] + mat[1 * 4 + 2] * m.mat[2 * 4 + 0];
	t1				= mat[1 * 4 + 0] * m.mat[0 * 4 + 1] + mat[1 * 4 + 1] * m.mat[1 * 4 + 1] + mat[1 * 4 + 2] * m.mat[2 * 4 + 1];
	t2				= mat[1 * 4 + 0] * m.mat[0 * 4 + 2] + mat[1 * 4 + 1] * m.mat[1 * 4 + 2] + mat[1 * 4 + 2] * m.mat[2 * 4 + 2];
	mat[1 * 4 + 3]	= mat[1 * 4 + 0] * m.mat[0 * 4 + 3] + mat[1 * 4 + 1] * m.mat[1 * 4 + 3] + mat[1 * 4 + 2] * m.mat[2 * 4 + 3] + mat[1 * 4 + 3];

	mat[1 * 4 + 0] = t0;
	mat[1 * 4 + 1] = t1;
	mat[1 * 4 + 2] = t2;

	t0				= mat[2 * 4 + 0] * m.mat[0 * 4 + 0] + mat[2 * 4 + 1] * m.mat[1 * 4 + 0] + mat[2 * 4 + 2] * m.mat[2 * 4 + 0];
	t1				= mat[2 * 4 + 0] * m.mat[0 * 4 + 1] + mat[2 * 4 + 1] * m.mat[1 * 4 + 1] + mat[2 * 4 + 2] * m.mat[2 * 4 + 1];
	t2				= mat[2 * 4 + 0] * m.mat[0 * 4 + 2] + mat[2 * 4 + 1] * m.mat[1 * 4 + 2] + mat[2 * 4 + 2] * m.mat[2 * 4 + 2];
	mat[2 * 4 + 3]	= mat[2 * 4 + 0] * m.mat[0 * 4 + 3] + mat[2 * 4 + 1] * m.mat[1 * 4 + 3] + mat[2 * 4 + 2] * m.mat[2 * 4 + 3] + mat[2 * 4 + 3];

	mat[2 * 4 + 0] = t0;
	mat[2 * 4 + 1] = t1;
	mat[2 * 4 + 2] = t2;
}

ID_INLINE void idMat3x4::RightMultiply( const idMat3 &m ) {
	float t0, t1, t2;

	// NOTE: idMat3 is column-major
	t0				= mat[0 * 4 + 0] * m[0][0] + mat[0 * 4 + 1] * m[0][1] + mat[0 * 4 + 2] * m[0][2];
	t1				= mat[0 * 4 + 0] * m[1][0] + mat[0 * 4 + 1] * m[1][1] + mat[0 * 4 + 2] * m[1][2];
	t2				= mat[0 * 4 + 0] * m[2][0] + mat[0 * 4 + 1] * m[2][1] + mat[0 * 4 + 2] * m[2][2];

	mat[0 * 4 + 0] = t0;
	mat[0 * 4 + 1] = t1;
	mat[0 * 4 + 2] = t2;

	t0				= mat[1 * 4 + 0] * m[0][0] + mat[1 * 4 + 1] * m[0][1] + mat[1 * 4 + 2] * m[0][2];
	t1				= mat[1 * 4 + 0] * m[1][0] + mat[1 * 4 + 1] * m[1][1] + mat[1 * 4 + 2] * m[1][2];
	t2				= mat[1 * 4 + 0] * m[2][0] + mat[1 * 4 + 1] * m[2][1] + mat[1 * 4 + 2] * m[2][2];

	mat[1 * 4 + 0] = t0;
	mat[1 * 4 + 1] = t1;
	mat[1 * 4 + 2] = t2;

	t0				= mat[2 * 4 + 0] * m[0][0] + mat[2 * 4 + 1] * m[0][1] + mat[2 * 4 + 2] * m[0][2];
	t1				= mat[2 * 4 + 0] * m[1][0] + mat[2 * 4 + 1] * m[1][1] + mat[2 * 4 + 2] * m[1][2];
	t2				= mat[2 * 4 + 0] * m[2][0] + mat[2 * 4 + 1] * m[2][1] + mat[2 * 4 + 2] * m[2][2];

	mat[2 * 4 + 0] = t0;
	mat[2 * 4 + 1] = t1;
	mat[2 * 4 + 2] = t2;
}

ID_INLINE void idMat3x4::Transform( idVec3 &result, const idVec3 &v ) const {
	result.x = mat[0 * 4 + 0] * v.x + mat[0 * 4 + 1] * v.y + mat[0 * 4 + 2] * v.z + mat[0 * 4 + 3];
	result.y = mat[1 * 4 + 0] * v.x + mat[1 * 4 + 1] * v.y + mat[1 * 4 + 2] * v.z + mat[1 * 4 + 3];
	result.z = mat[2 * 4 + 0] * v.x + mat[2 * 4 + 1] * v.y + mat[2 * 4 + 2] * v.z + mat[2 * 4 + 3];
}

ID_INLINE void idMat3x4::Rotate( idVec3 &result, const idVec3 &v ) const {
	result.x = mat[0 * 4 + 0] * v.x + mat[0 * 4 + 1] * v.y + mat[0 * 4 + 2] * v.z;
	result.y = mat[1 * 4 + 0] * v.x + mat[1 * 4 + 1] * v.y + mat[1 * 4 + 2] * v.z;
	result.z = mat[2 * 4 + 0] * v.x + mat[2 * 4 + 1] * v.y + mat[2 * 4 + 2] * v.z;
}

ID_INLINE idMat3 idMat3x4::ToMat3( void ) const {
	return idMat3(	mat[0 * 4 + 0], mat[1 * 4 + 0], mat[2 * 4 + 0],
					mat[0 * 4 + 1], mat[1 * 4 + 1], mat[2 * 4 + 1],
					mat[0 * 4 + 2], mat[1 * 4 + 2], mat[2 * 4 + 2] );
}

ID_INLINE idVec3 idMat3x4::ToVec3( void ) const {
	return idVec3( mat[0 * 4 + 3], mat[1 * 4 + 3], mat[2 * 4 + 3] );
}

ID_INLINE const float *idMat3x4::ToFloatPtr( void ) const {
	return mat;
}

ID_INLINE float *idMat3x4::ToFloatPtr( void ) {
	return mat;
}

#endif /* !__MAT3X4_H__ */
