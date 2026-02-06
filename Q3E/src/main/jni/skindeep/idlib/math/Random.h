/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __MATH_RANDOM_H__
#define __MATH_RANDOM_H__

#include "idlib/math/Vector.h"

/*
===============================================================================

	Random number generator

===============================================================================
*/

class idRandom {
public:
						idRandom( int seed = 0 );

	void				SetSeed( int seed );
	int					GetSeed( void ) const;

	int					RandomInt( void );			// random integer in the range [0, MAX_RAND]
	int					RandomInt( int max );		// random integer in the range [0, max[
	float				RandomFloat( void );		// random number in the range [0.0f, 1.0f]
	float				CRandomFloat( void );		// random number in the range [-1.0f, 1.0f]

	int					RandomInt(int min, int max); //BC helper function

	// SM
	idVec3				RandomPointInSphere( const idVec3& origin, float radius );

	static const int	MAX_RAND = 0x7fff;

private:
	int					seed;
};

ID_INLINE idRandom::idRandom( int seed ) {
	this->seed = seed;
}

ID_INLINE void idRandom::SetSeed( int seed ) {
	this->seed = seed;
}

ID_INLINE int idRandom::GetSeed( void ) const {
	return seed;
}

ID_INLINE int idRandom::RandomInt( void ) {
	//seed = 69069 * seed + 1;
	//return ( seed & idRandom::MAX_RAND );

	//Dark mod.
	seed = seed * 1103515245 + 12345;
	return (unsigned(seed) >> 16) & idRandom::MAX_RAND;
}

//Get a random number, non-inclusive of max number.
ID_INLINE int idRandom::RandomInt( int max ) {
	if ( max == 0 ) {
		return 0;			// avoid divide by zero error
	}
	return RandomInt() % max;
}

//BC Get a random number, inclusive of min/max number.
ID_INLINE int idRandom::RandomInt(int min, int max) {
	if (max == 0)
	{
		return 0;			// avoid divide by zero error
	}

	max += 1;

	int randomRange;
	if (min <= 0)
		randomRange = abs(min) + max;
	else
		randomRange = max - min;

	return min + (RandomInt() % randomRange);
}

//RANDOM FLOAT 0.0 TO 1.0
ID_INLINE float idRandom::RandomFloat( void ) {
	return ( RandomInt() / ( float )( idRandom::MAX_RAND + 1 ) );
}

//RANDOM FLOAT -1.0 TO 1.0
ID_INLINE float idRandom::CRandomFloat( void ) {
	return ( 2.0f * ( RandomFloat() - 0.5f ) );
}

ID_INLINE idVec3 idRandom::RandomPointInSphere( const idVec3& origin, float radius ) {
	idVec3 point;

	float u = RandomFloat();
	float v = RandomFloat();
	float theta = u * 2.0f * idMath::PI;
	float phi = idMath::ACos( 2.0f * v - 1.0f );
	float r = cbrtf( RandomFloat() );
	float sinTheta = idMath::Sin( theta );
	float cosTheta = idMath::Cos( theta );
	float sinPhi = idMath::Sin( phi );
	float cosPhi = idMath::Cos( phi );

	point.x = r * sinPhi * cosTheta;
	point.y = r * sinPhi * sinTheta;
	point.z = r * cosPhi;

	return origin + point * radius;
}

/*
===============================================================================

	Random number generator

===============================================================================
*/

class idRandom2 {
public:
							idRandom2( unsigned int seed = 0 );

	void					SetSeed( unsigned int seed );
	unsigned int			GetSeed( void ) const;

	int						RandomInt( void );			// random integer in the range [0, MAX_RAND]
	int						RandomInt( int max );		// random integer in the range [0, max]
	float					RandomFloat( void );		// random number in the range [0.0f, 1.0f]
	float					CRandomFloat( void );		// random number in the range [-1.0f, 1.0f]

	static const int		MAX_RAND = 0x7fff;

private:
	unsigned int			seed;

	static const unsigned int	IEEE_ONE = 0x3f800000;
	static const unsigned int	IEEE_MASK = 0x007fffff;
};

ID_INLINE idRandom2::idRandom2( unsigned int seed ) {
	this->seed = seed;
}

ID_INLINE void idRandom2::SetSeed( unsigned int seed ) {
	this->seed = seed;
}

ID_INLINE unsigned int idRandom2::GetSeed( void ) const {
	return seed;
}

ID_INLINE int idRandom2::RandomInt( void ) {
	seed = 1664525 * seed + 1013904223;
	return ( (int) seed & idRandom2::MAX_RAND );
}

ID_INLINE int idRandom2::RandomInt( int max ) {
	if ( max == 0 ) {
		return 0;		// avoid divide by zero error
	}
	return ( RandomInt() >> ( 16 - idMath::BitsForInteger( max ) ) ) % max;
}

ID_INLINE float idRandom2::RandomFloat( void ) {
	unsigned int i;
	seed = 1664525 * seed + 1013904223;
	i = idRandom2::IEEE_ONE | ( seed & idRandom2::IEEE_MASK );
	return ( ( *(float *)&i ) - 1.0f );
}

ID_INLINE float idRandom2::CRandomFloat( void ) {
	unsigned int i;
	seed = 1664525 * seed + 1013904223;
	i = idRandom2::IEEE_ONE | ( seed & idRandom2::IEEE_MASK );
	return ( 2.0f * ( *(float *)&i ) - 3.0f );
}

#endif /* !__MATH_RANDOM_H__ */
