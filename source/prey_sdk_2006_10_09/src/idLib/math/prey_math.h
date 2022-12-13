
#ifndef __PREY_GAME_MATH_H__
#define __PREY_GAME_MATH_H__

class hhMath : public idMath {

public:
	static float		logBase(float base, float x);
	static float		dB2Scale( float dB );
	static float		Scale2dB( float scale );
	static float		Frac( float a );

	static float		Pow( const float num, const float exponent );

	static float		MidPointLerp( const float startVal, const float midVal, const float endVal, const float alpha );

	static float		Lerp( const float startVal, const float endVal, const float alpha );
	static float		Lerp( const idVec2& valRange, const float alpha );

	template< class Type >
	static Type			hhMin( Type Val1, Type Val2 );

	template< class Type >
	static Type			hhMax( Type Val1, Type Val2 );

	static idVec3		GetClosestPtOnBoundary(const idVec3 &pt, const idBounds &bnds );

	static idVec3		ProjectPointOntoLine( const idVec3& point, const idVec3& line, const idVec3& lineStartPoint );
	static float		DistFromPointToLine( const idVec3& point, const idVec3& line, const idVec3& lineStartPoint );

	static void			BuildRotationMatrix(float phi, int axis, idMat3 &mat); //rww

	static const float	EXPONENTIAL;
};

/*
===============
hhMath::hhMin
===============
*/
template< class Type >
Type hhMath::hhMin( Type Val1, Type Val2 ) {
	return Min( Val1, Val2 );
}

/*
===============
hhMath::hhMax
===============
*/
template< class Type >
Type hhMath::hhMax( Type Val1, Type Val2 ) {
	return Max( Val1, Val2 );
}

#endif /* __PREY_GAME_MATH_H__ */
