//======= Copyright (C) 2005-2011, Valve Corporation, All rights reserved. =========
//
// Public domain MurmurHash3 by Austin Appleby is a very solid general-purpose
// hash with a 32-bit output. References:
// http://code.google.com/p/smhasher/ (home of MurmurHash3)
// https://sites.google.com/site/murmurhash/avalanche
// http://www.strchr.com/hash_functions 
//
// Variant Pearson Hash general purpose hashing algorithm described
// by Cargill in C++ Report 1994. Generates a 16-bit result.
// Now relegated to PearsonHash namespace, not recommended for use
//
//=============================================================================

#include <stdlib.h>
#include <generichash.h>
#include <strtools.h>
#include "minbase_endian.h"

#if defined(_MSC_VER) && _MSC_VER > 1200
#define ROTL32(x,y)	_rotl(x,y)
#define ROTL64(x,y)	_rotl64(x,y)
#else // defined(_MSC_VER) && _MSC_VER > 1200
static inline uint32_t rotl32( uint32_t x, int8_t r )
{
	return ( x << r ) | ( x >> ( 32 - r ) );
}
static inline uint64_t rotl64( uint64_t x, int8_t r )
{
	return ( x << r ) | ( x >> ( 64 - r ) );
}
#define	ROTL32(x,y)	rotl32(x,y)
#define ROTL64(x,y)	rotl64(x,y)
#endif // !defined(_MSC_VER)

//-----------------------------------------------------------------------------

uint32_t MurmurHash3_32( const void * key, size_t len, uint32_t seed, bool bCaselessStringVariant )
{
	const uint8_t* data = (const uint8_t*)key;
	const ptrdiff_t nblocks = len / 4;
	uint32_t uSourceBitwiseAndMask = 0xDFDFDFDF | ((uint32_t)bCaselessStringVariant - 1);

	uint32_t h1 = seed;

	//----------
	// body

	const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);

	for(ptrdiff_t i = -nblocks; i; i++)
	{
		uint32_t k1 = LittleDWord(blocks[i]);
		k1 &= uSourceBitwiseAndMask;

		k1 *= 0xcc9e2d51;
		k1 = (k1 << 15) | (k1 >> 17);
		k1 *= 0x1b873593;

		h1 ^= k1;
		h1 = (h1 << 13) | (h1 >> 19);
		h1 = h1*5+0xe6546b64;
	}

	//----------
	// tail

	const uint8_t * tail = (const uint8_t*)(data + nblocks*4);

	uint32_t k1 = 0;

	switch(len & 3)
	{
	case 3: k1 ^= tail[2] << 16; // fallthrough
	case 2: k1 ^= tail[1] << 8; // fallthrough
	case 1: k1 ^= tail[0];
			k1 &= uSourceBitwiseAndMask;
			k1 *= 0xcc9e2d51;
			k1 = (k1 << 15) | (k1 >> 17);
			k1 *= 0x1b873593;
			h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= len;

	h1 ^= h1 >> 16;
	h1 *= 0x85ebca6b;
	h1 ^= h1 >> 13;
	h1 *= 0xc2b2ae35;
	h1 ^= h1 >> 16;

	return h1;
}

static inline uint64_t fmix64( uint64_t k )
{
	k ^= k >> 33;
	k *= 0xff51afd7ed558ccdLLU;
	k ^= k >> 33;
	k *= 0xc4ceb9fe1a85ec53LLU;
	k ^= k >> 33;

	return k;
}

void MurmurHash3_128( const void * key, const int len, const uint32_t seed, void * out )
{
	const uint8_t * data = ( const uint8_t* )key;
	const int nblocks = len / 16;

	uint64_t h1 = seed;
	uint64_t h2 = seed;

	const uint64_t c1 = 0x87c37b91114253d5LLU;
	const uint64_t c2 = 0x4cf5ad432745937fLLU;

	//----------
	// body

	const uint64_t * blocks = ( const uint64_t * )( data );

	for ( int i = 0; i < nblocks; i++ )
	{
		uint64_t k1 = blocks[i * 2 + 0];
		uint64_t k2 = blocks[i * 2 + 1];

		k1 *= c1; k1 = ROTL64( k1, 31 ); k1 *= c2; h1 ^= k1;

		h1 = ROTL64( h1, 27 ); h1 += h2; h1 = h1 * 5 + 0x52dce729;

		k2 *= c2; k2 = ROTL64( k2, 33 ); k2 *= c1; h2 ^= k2;

		h2 = ROTL64( h2, 31 ); h2 += h1; h2 = h2 * 5 + 0x38495ab5;
	}

	//----------
	// tail

	const uint8_t * tail = ( const uint8_t* )( data + nblocks * 16 );

	uint64_t k1 = 0;
	uint64_t k2 = 0;

	switch ( len & 15 )
	{
	case 15: k2 ^= ( ( uint64_t )tail[14] ) << 48; // fallthrough
	case 14: k2 ^= ( ( uint64_t )tail[13] ) << 40; // fallthrough
	case 13: k2 ^= ( ( uint64_t )tail[12] ) << 32; // fallthrough
	case 12: k2 ^= ( ( uint64_t )tail[11] ) << 24; // fallthrough
	case 11: k2 ^= ( ( uint64_t )tail[10] ) << 16; // fallthrough
	case 10: k2 ^= ( ( uint64_t )tail[9] ) << 8; // fallthrough
	case  9: k2 ^= ( ( uint64_t )tail[8] ) << 0; // fallthrough
		k2 *= c2; k2 = ROTL64( k2, 33 ); k2 *= c1; h2 ^= k2;
		// fallthrough
	case  8: k1 ^= ( ( uint64_t )tail[7] ) << 56; // fallthrough
	case  7: k1 ^= ( ( uint64_t )tail[6] ) << 48; // fallthrough
	case  6: k1 ^= ( ( uint64_t )tail[5] ) << 40; // fallthrough
	case  5: k1 ^= ( ( uint64_t )tail[4] ) << 32; // fallthrough
	case  4: k1 ^= ( ( uint64_t )tail[3] ) << 24; // fallthrough
	case  3: k1 ^= ( ( uint64_t )tail[2] ) << 16; // fallthrough
	case  2: k1 ^= ( ( uint64_t )tail[1] ) << 8; // fallthrough
	case  1: k1 ^= ( ( uint64_t )tail[0] ) << 0; // fallthrough
		k1 *= c1; k1 = ROTL64( k1, 31 ); k1 *= c2; h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= len; h2 ^= len;

	h1 += h2;
	h2 += h1;

	h1 = fmix64( h1 );
	h2 = fmix64( h2 );

	h1 += h2;
	h2 += h1;

	( ( uint64_t* )out )[0] = h1;
	( ( uint64_t* )out )[1] = h2;
}
