// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __JOINTTRANSFORM_H__
#define __JOINTTRANSFORM_H__

/*
===============================================================================

  Joint Quaternion

===============================================================================
*/

class idJointQuat {
public:

	idQuat			q;
	idVec3			t;
};


/*
===============================================================================

  Joint Matrix

  idMat3 m;
  idVec3 t;

  m[0][0], m[1][0], m[2][0], t[0]
  m[0][1], m[1][1], m[2][1], t[1]
  m[0][2], m[1][2], m[2][2], t[2]

===============================================================================
*/

class idJointMat {
public:

	void			SetRotation( const idMat3 &m );
	void			SetTranslation( const idVec3 &t );

	idVec3			operator*( const idVec3 &v ) const;							// only rotate
	idVec3			operator*( const idVec4 &v ) const;							// rotate and translate

	idJointMat &	operator*=( const idJointMat &a );							// transform
	idJointMat &	operator/=( const idJointMat &a );							// untransform

	bool			Compare( const idJointMat &a ) const;						// exact compare, no epsilon
	bool			Compare( const idJointMat &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idJointMat &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const idJointMat &a ) const;					// exact compare, no epsilon

	idMat3			ToMat3( void ) const;
	idVec3			ToVec3( void ) const;
	idJointQuat		ToJointQuat( void ) const;
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );

	//HUMANHEAD bjk
	ID_INLINE static void		Mul( idJointMat &result, const idJointMat &mat, const float s );
	ID_INLINE static void		Mad( idJointMat &result, const idJointMat &mat, const float s );
	ID_INLINE static void		Multiply( idJointMat &result, const idJointMat &m1, const idJointMat &m2 );

	ID_INLINE void	Invert();
	//HUMANHEAD END

private:
	float			mat[3*4];
};

ID_INLINE void idJointMat::SetRotation( const idMat3 &m ) {
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

ID_INLINE void idJointMat::SetTranslation( const idVec3 &t ) {
	mat[0 * 4 + 3] = t[0];
	mat[1 * 4 + 3] = t[1];
	mat[2 * 4 + 3] = t[2];
}

ID_INLINE idVec3 idJointMat::operator*( const idVec3 &v ) const {
	return idVec3(	mat[0 * 4 + 0] * v[0] + mat[0 * 4 + 1] * v[1] + mat[0 * 4 + 2] * v[2],
					mat[1 * 4 + 0] * v[0] + mat[1 * 4 + 1] * v[1] + mat[1 * 4 + 2] * v[2],
					mat[2 * 4 + 0] * v[0] + mat[2 * 4 + 1] * v[1] + mat[2 * 4 + 2] * v[2] );
}

ID_INLINE idVec3 idJointMat::operator*( const idVec4 &v ) const {
	return idVec3(	mat[0 * 4 + 0] * v[0] + mat[0 * 4 + 1] * v[1] + mat[0 * 4 + 2] * v[2] + mat[0 * 4 + 3] * v[3],
					mat[1 * 4 + 0] * v[0] + mat[1 * 4 + 1] * v[1] + mat[1 * 4 + 2] * v[2] + mat[1 * 4 + 3] * v[3],
					mat[2 * 4 + 0] * v[0] + mat[2 * 4 + 1] * v[1] + mat[2 * 4 + 2] * v[2] + mat[2 * 4 + 3] * v[3] );
}

ID_INLINE idJointMat &idJointMat::operator*=( const idJointMat &a ) {
	float dst[3];

	dst[0] = mat[0 * 4 + 0] * a.mat[0 * 4 + 0] + mat[1 * 4 + 0] * a.mat[0 * 4 + 1] + mat[2 * 4 + 0] * a.mat[0 * 4 + 2];
	dst[1] = mat[0 * 4 + 0] * a.mat[1 * 4 + 0] + mat[1 * 4 + 0] * a.mat[1 * 4 + 1] + mat[2 * 4 + 0] * a.mat[1 * 4 + 2];
	dst[2] = mat[0 * 4 + 0] * a.mat[2 * 4 + 0] + mat[1 * 4 + 0] * a.mat[2 * 4 + 1] + mat[2 * 4 + 0] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 0] = dst[0];
	mat[1 * 4 + 0] = dst[1];
	mat[2 * 4 + 0] = dst[2];

	dst[0] = mat[0 * 4 + 1] * a.mat[0 * 4 + 0] + mat[1 * 4 + 1] * a.mat[0 * 4 + 1] + mat[2 * 4 + 1] * a.mat[0 * 4 + 2];
	dst[1] = mat[0 * 4 + 1] * a.mat[1 * 4 + 0] + mat[1 * 4 + 1] * a.mat[1 * 4 + 1] + mat[2 * 4 + 1] * a.mat[1 * 4 + 2];
	dst[2] = mat[0 * 4 + 1] * a.mat[2 * 4 + 0] + mat[1 * 4 + 1] * a.mat[2 * 4 + 1] + mat[2 * 4 + 1] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 1] = dst[0];
	mat[1 * 4 + 1] = dst[1];
	mat[2 * 4 + 1] = dst[2];

	dst[0] = mat[0 * 4 + 2] * a.mat[0 * 4 + 0] + mat[1 * 4 + 2] * a.mat[0 * 4 + 1] + mat[2 * 4 + 2] * a.mat[0 * 4 + 2];
	dst[1] = mat[0 * 4 + 2] * a.mat[1 * 4 + 0] + mat[1 * 4 + 2] * a.mat[1 * 4 + 1] + mat[2 * 4 + 2] * a.mat[1 * 4 + 2];
	dst[2] = mat[0 * 4 + 2] * a.mat[2 * 4 + 0] + mat[1 * 4 + 2] * a.mat[2 * 4 + 1] + mat[2 * 4 + 2] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 2] = dst[0];
	mat[1 * 4 + 2] = dst[1];
	mat[2 * 4 + 2] = dst[2];

	dst[0] = mat[0 * 4 + 3] * a.mat[0 * 4 + 0] + mat[1 * 4 + 3] * a.mat[0 * 4 + 1] + mat[2 * 4 + 3] * a.mat[0 * 4 + 2];
	dst[1] = mat[0 * 4 + 3] * a.mat[1 * 4 + 0] + mat[1 * 4 + 3] * a.mat[1 * 4 + 1] + mat[2 * 4 + 3] * a.mat[1 * 4 + 2];
	dst[2] = mat[0 * 4 + 3] * a.mat[2 * 4 + 0] + mat[1 * 4 + 3] * a.mat[2 * 4 + 1] + mat[2 * 4 + 3] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 3] = dst[0];
	mat[1 * 4 + 3] = dst[1];
	mat[2 * 4 + 3] = dst[2];

	mat[0 * 4 + 3] += a.mat[0 * 4 + 3];
	mat[1 * 4 + 3] += a.mat[1 * 4 + 3];
	mat[2 * 4 + 3] += a.mat[2 * 4 + 3];

	return *this;
}

ID_INLINE idJointMat &idJointMat::operator/=( const idJointMat &a ) {
	float dst[3];

	mat[0 * 4 + 3] -= a.mat[0 * 4 + 3];
	mat[1 * 4 + 3] -= a.mat[1 * 4 + 3];
	mat[2 * 4 + 3] -= a.mat[2 * 4 + 3];

	dst[0] = mat[0 * 4 + 0] * a.mat[0 * 4 + 0] + mat[1 * 4 + 0] * a.mat[1 * 4 + 0] + mat[2 * 4 + 0] * a.mat[2 * 4 + 0];
	dst[1] = mat[0 * 4 + 0] * a.mat[0 * 4 + 1] + mat[1 * 4 + 0] * a.mat[1 * 4 + 1] + mat[2 * 4 + 0] * a.mat[2 * 4 + 1];
	dst[2] = mat[0 * 4 + 0] * a.mat[0 * 4 + 2] + mat[1 * 4 + 0] * a.mat[1 * 4 + 2] + mat[2 * 4 + 0] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 0] = dst[0];
	mat[1 * 4 + 0] = dst[1];
	mat[2 * 4 + 0] = dst[2];

	dst[0] = mat[0 * 4 + 1] * a.mat[0 * 4 + 0] + mat[1 * 4 + 1] * a.mat[1 * 4 + 0] + mat[2 * 4 + 1] * a.mat[2 * 4 + 0];
	dst[1] = mat[0 * 4 + 1] * a.mat[0 * 4 + 1] + mat[1 * 4 + 1] * a.mat[1 * 4 + 1] + mat[2 * 4 + 1] * a.mat[2 * 4 + 1];
	dst[2] = mat[0 * 4 + 1] * a.mat[0 * 4 + 2] + mat[1 * 4 + 1] * a.mat[1 * 4 + 2] + mat[2 * 4 + 1] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 1] = dst[0];
	mat[1 * 4 + 1] = dst[1];
	mat[2 * 4 + 1] = dst[2];

	dst[0] = mat[0 * 4 + 2] * a.mat[0 * 4 + 0] + mat[1 * 4 + 2] * a.mat[1 * 4 + 0] + mat[2 * 4 + 2] * a.mat[2 * 4 + 0];
	dst[1] = mat[0 * 4 + 2] * a.mat[0 * 4 + 1] + mat[1 * 4 + 2] * a.mat[1 * 4 + 1] + mat[2 * 4 + 2] * a.mat[2 * 4 + 1];
	dst[2] = mat[0 * 4 + 2] * a.mat[0 * 4 + 2] + mat[1 * 4 + 2] * a.mat[1 * 4 + 2] + mat[2 * 4 + 2] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 2] = dst[0];
	mat[1 * 4 + 2] = dst[1];
	mat[2 * 4 + 2] = dst[2];

	dst[0] = mat[0 * 4 + 3] * a.mat[0 * 4 + 0] + mat[1 * 4 + 3] * a.mat[1 * 4 + 0] + mat[2 * 4 + 3] * a.mat[2 * 4 + 0];
	dst[1] = mat[0 * 4 + 3] * a.mat[0 * 4 + 1] + mat[1 * 4 + 3] * a.mat[1 * 4 + 1] + mat[2 * 4 + 3] * a.mat[2 * 4 + 1];
	dst[2] = mat[0 * 4 + 3] * a.mat[0 * 4 + 2] + mat[1 * 4 + 3] * a.mat[1 * 4 + 2] + mat[2 * 4 + 3] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 3] = dst[0];
	mat[1 * 4 + 3] = dst[1];
	mat[2 * 4 + 3] = dst[2];

	return *this;
}

ID_INLINE bool idJointMat::Compare( const idJointMat &a ) const {
	int i;

	for ( i = 0; i < 12; i++ ) {
		if ( mat[i] != a.mat[i] ) {
			return false;
		}
	}
	return true;
}

ID_INLINE bool idJointMat::Compare( const idJointMat &a, const float epsilon ) const {
	int i;

	for ( i = 0; i < 12; i++ ) {
		if ( idMath::Fabs( mat[i] - a.mat[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ID_INLINE bool idJointMat::operator==( const idJointMat &a ) const {
	return Compare( a );
}

ID_INLINE bool idJointMat::operator!=( const idJointMat &a ) const {
	return !Compare( a );
}

ID_INLINE idMat3 idJointMat::ToMat3( void ) const {
	return idMat3(	mat[0 * 4 + 0], mat[1 * 4 + 0], mat[2 * 4 + 0],
					mat[0 * 4 + 1], mat[1 * 4 + 1], mat[2 * 4 + 1],
					mat[0 * 4 + 2], mat[1 * 4 + 2], mat[2 * 4 + 2] );
}

ID_INLINE idVec3 idJointMat::ToVec3( void ) const {
	return idVec3( mat[0 * 4 + 3], mat[1 * 4 + 3], mat[2 * 4 + 3] );
}

ID_INLINE const float *idJointMat::ToFloatPtr( void ) const {
	return mat;
}

ID_INLINE float *idJointMat::ToFloatPtr( void ) {
	return mat;
}

ID_INLINE void idJointMat::Mul( idJointMat &result, const idJointMat &mat, const float s ) {
	result.mat[0 * 4 + 0] = s * mat.mat[0 * 4 + 0];
	result.mat[0 * 4 + 1] = s * mat.mat[0 * 4 + 1];
	result.mat[0 * 4 + 2] = s * mat.mat[0 * 4 + 2];
	result.mat[0 * 4 + 3] = s * mat.mat[0 * 4 + 3];
	result.mat[1 * 4 + 0] = s * mat.mat[1 * 4 + 0];
	result.mat[1 * 4 + 1] = s * mat.mat[1 * 4 + 1];
	result.mat[1 * 4 + 2] = s * mat.mat[1 * 4 + 2];
	result.mat[1 * 4 + 3] = s * mat.mat[1 * 4 + 3];
	result.mat[2 * 4 + 0] = s * mat.mat[2 * 4 + 0];
	result.mat[2 * 4 + 1] = s * mat.mat[2 * 4 + 1];
	result.mat[2 * 4 + 2] = s * mat.mat[2 * 4 + 2];
	result.mat[2 * 4 + 3] = s * mat.mat[2 * 4 + 3];
}

ID_INLINE void idJointMat::Mad( idJointMat &result, const idJointMat &mat, const float s ) {
	result.mat[0 * 4 + 0] += s * mat.mat[0 * 4 + 0];
	result.mat[0 * 4 + 1] += s * mat.mat[0 * 4 + 1];
	result.mat[0 * 4 + 2] += s * mat.mat[0 * 4 + 2];
	result.mat[0 * 4 + 3] += s * mat.mat[0 * 4 + 3];
	result.mat[1 * 4 + 0] += s * mat.mat[1 * 4 + 0];
	result.mat[1 * 4 + 1] += s * mat.mat[1 * 4 + 1];
	result.mat[1 * 4 + 2] += s * mat.mat[1 * 4 + 2];
	result.mat[1 * 4 + 3] += s * mat.mat[1 * 4 + 3];
	result.mat[2 * 4 + 0] += s * mat.mat[2 * 4 + 0];
	result.mat[2 * 4 + 1] += s * mat.mat[2 * 4 + 1];
	result.mat[2 * 4 + 2] += s * mat.mat[2 * 4 + 2];
	result.mat[2 * 4 + 3] += s * mat.mat[2 * 4 + 3];
}

ID_INLINE void idJointMat::Multiply( idJointMat &result, const idJointMat &m1, const idJointMat &m2 ) {
	result.mat[0 * 4 + 0] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 0] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 0];
	result.mat[0 * 4 + 1] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 1] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 1];
	result.mat[0 * 4 + 2] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 2] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 2] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 2];
	result.mat[0 * 4 + 3] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 3] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 3] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 3] + m1.mat[0 * 4 + 3];

	result.mat[1 * 4 + 0] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 0] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 0];
	result.mat[1 * 4 + 1] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 1] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 1];
	result.mat[1 * 4 + 2] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 2] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 2] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 2];
	result.mat[1 * 4 + 3] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 3] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 3] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 3] + m1.mat[1 * 4 + 3];

	result.mat[2 * 4 + 0] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 0] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 0];
	result.mat[2 * 4 + 1] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 1] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 1];
	result.mat[2 * 4 + 2] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 2] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 2] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 2];
	result.mat[2 * 4 + 3] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 3] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 3] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 3] + m1.mat[2 * 4 + 3];
}

ID_INLINE void idJointMat::Invert( ) {
	float m01, m02, m12, tx, ty;

	tx = mat[0 * 4 + 3];
	ty = mat[1 * 4 + 3];
	mat[0 * 4 + 3] = -(tx*mat[0 * 4 + 0] + ty*mat[1 * 4 + 0] + mat[2 * 4 + 3]*mat[2 * 4 + 0]);
	mat[1 * 4 + 3] = -(tx*mat[0 * 4 + 1] + ty*mat[1 * 4 + 1] + mat[2 * 4 + 3]*mat[2 * 4 + 1]);
	mat[2 * 4 + 3] = -(tx*mat[0 * 4 + 2] + ty*mat[1 * 4 + 2] + mat[2 * 4 + 3]*mat[2 * 4 + 2]);

	m01 = mat[0 * 4 + 1];
	m02 = mat[0 * 4 + 2];
	m12 = mat[1 * 4 + 2];
	mat[0 * 4 + 1] = mat[1 * 4 + 0];
	mat[0 * 4 + 2] = mat[2 * 4 + 0];
	mat[1 * 4 + 0] = m01;
	mat[1 * 4 + 2] = mat[2 * 4 + 1];
	mat[2 * 4 + 0] = m02;
	mat[2 * 4 + 1] = m12;
}

#endif /* !__JOINTTRANSFORM_H__ */
