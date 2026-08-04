/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __MATH_RANDOM_H__
#define __MATH_RANDOM_H__

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
#if 0
	//stgatilov: OMG this is SOOO bad!
	//the high bits are NEVER used, so this is basically LCG with modulus 2^15
	//read also: https://en.wikipedia.org/wiki/Linear_congruential_generator#Advantages_and_disadvantages
	//"The low-order bits of LCGs when m is a power of 2 should never be relied on for any degree of randomness whatsoever."
	seed = 69069 * seed + 1;
	return ( seed & idRandom::MAX_RAND );
#else
	//stgatilov: recommended by C standard as "rand" implementation
	seed = seed * 1103515245 + 12345;
	return (unsigned(seed) >> 16) & idRandom::MAX_RAND;
#endif
}

ID_INLINE int idRandom::RandomInt( int max ) {
	if ( max == 0 ) {
		return 0;			// avoid divide by zero error
	}
	return RandomInt() % max;
}

ID_INLINE float idRandom::RandomFloat( void ) {
	return ( RandomInt() / ( float )( idRandom::MAX_RAND + 1 ) );
}

ID_INLINE float idRandom::CRandomFloat( void ) {
	return ( 2.0f * ( RandomFloat() - 0.5f ) );
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
	//int						RandomInt( int max );		// random integer in the range [0, max]
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
