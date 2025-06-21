//======= Copyright (c) 2005-2011, Valve Corporation, All rights reserved. =========
//
// Public domain MurmurHash3 by Austin Appleby is a very solid general-purpose
// hash with a 32-bit output. References:
// http://code.google.com/p/smhasher/ (home of MurmurHash3)
// https://sites.google.com/site/murmurhash/avalanche
// http://www.strchr.com/hash_functions 
//
// Variant Pearson Hash general purpose hashing algorithm described
// by Cargill in C++ Report 1994. Generates a 16-bit result.
// Now relegated to PearsonHash namespace, not recommended for use;
// still around in case someone needs value compatibility with old code.
//
//=============================================================================

#pragma once
#ifndef GENERICHASH_H
#define GENERICHASH_H

#include "miniutl.h"

uint32_t MurmurHash3_32( const void *key, size_t len, uint32_t seed, bool bCaselessStringVariant = false );
void MurmurHash3_128( const void * key, const int len, const uint32_t seed, void * out );

inline uint32_t HashString( const char *pszKey, size_t len )
{
	return MurmurHash3_32( pszKey, len, 1047 /*anything will do for a seed*/, false );
}

inline uint32_t HashStringCaseless( const char *pszKey, size_t len )
{
	return MurmurHash3_32( pszKey, len, 1047 /*anything will do for a seed*/, true );
}

inline uint32_t HashString( const char *pszKey )
{
	return HashString( pszKey, strlen( pszKey ) );
}

inline uint32_t HashStringCaseless( const char *pszKey )
{
	return HashStringCaseless( pszKey, strlen( pszKey ) );
}

inline uint32_t HashInt64( uint64_t h )
{
	// roughly equivalent to MurmurHash3_32( &lower32, sizeof(uint32), upper32_as_seed )...
	// theory being that most of the entropy is in the lower 32 bits and we still mix
	// everything together at the end, so not fully shuffling upper32 is not a big deal
	uint32_t h1 = static_cast<uint32_t>( h>>32 );
	uint32_t k1 = (uint32_t)h;

	k1 *= 0xcc9e2d51;
	k1 = (k1 << 15) | (k1 >> 17);
	k1 *= 0x1b873593;

	h1 ^= k1;
	h1 = (h1 << 13) | (h1 >> 19);
	h1 = h1*5+0xe6546b64;

	h1 ^= h1 >> 16;
	h1 *= 0x85ebca6b;
	h1 ^= h1 >> 13;
	h1 *= 0xc2b2ae35;
	h1 ^= h1 >> 16;

	return h1;
}

inline uint32_t HashInt( uint32_t h )
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

template <typename T>
inline uint32_t HashItemAsBytes( const T&item )
{
	if ( sizeof( item ) == sizeof( uint32_t ))
		return HashInt( *(uint32_t*)&item );

	if ( sizeof( item ) == sizeof( uint64_t ))
		return HashInt64( *(uint64_t*)&item );

	return MurmurHash3_32( &item, sizeof(item), 1047 );
}

template <typename T>
inline uint32_t HashItem( const T &item )
{
	return HashItemAsBytes( item );
}


template<typename T>
struct HashFunctor
{
	typedef uint32_t TargetType;
	TargetType operator()(const T &key) const
	{
		return HashItem( key );
	}
};

template<>
struct HashFunctor<char *>
{
	typedef uint32_t TargetType;
	TargetType operator()(const char *key) const
	{
		return HashString( key );
	}
};

template<>
struct HashFunctor<char const *>
{
	typedef uint32_t TargetType;
	TargetType operator()(const char *key) const
	{
		return HashString( key );
	}
};

struct HashFunctorStringCaseless
{
	typedef uint32_t TargetType;
	TargetType operator()(const char *key) const
	{
		return HashStringCaseless( key );
	}
};

template<class T>
struct HashFunctorUnpaddedStructure
{
	typedef uint32_t TargetType;
	TargetType operator()(const T &key) const
	{
		return HashItemAsBytes( key );
	}
};

//-----------------------------------------------------------------------------

#endif /* !GENERICHASH_H */
