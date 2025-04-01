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

#include "precompiled.h"
#pragma hdrstop



int idHashIndex::INVALID_INDEX[1] = { -1 };

/*
================
idHashIndex::Init
================
*/
void idHashIndex::Init( const int initialHashSize, const int initialIndexSize ) {
	assert( idMath::IsPowerOfTwo( initialHashSize ) );

	hashSize = initialHashSize;
	hash = INVALID_INDEX;
	indexSize = initialIndexSize;
	indexChain = INVALID_INDEX;
	granularity = DEFAULT_HASH_GRANULARITY;
	hashMask = hashSize - 1;
	lookupMask = 0;
}

/*
================
idHashIndex::Allocate
================
*/
void idHashIndex::Allocate( const int newHashSize, const int newIndexSize ) {
	assert( idMath::IsPowerOfTwo( newHashSize ) );

	ClearFree();
	hashSize = newHashSize;
	hash = new int[hashSize];
	memset( hash, 0xff, hashSize * sizeof( hash[0] ) );
	indexSize = newIndexSize;
	indexChain = new int[indexSize];
	memset( indexChain, 0xff, indexSize * sizeof( indexChain[0] ) );
	hashMask = hashSize - 1;
	lookupMask = -1;
}

/*
================
idHashIndex::ClearFree
================
*/
void idHashIndex::ClearFree( void ) {
	if ( hash != INVALID_INDEX ) {
		delete[] hash;
		hash = INVALID_INDEX;
	}
	if ( indexChain != INVALID_INDEX ) {
		delete[] indexChain;
		indexChain = INVALID_INDEX;
	}
	lookupMask = 0;
}

/*
================
idHashIndex::ResizeIndex
================
*/
void idHashIndex::ResizeIndex( const int newIndexSize ) {
	int *oldIndexChain, mod, newSize;

	if ( newIndexSize <= indexSize ) {
		return;
	}

	mod = newIndexSize % granularity;
	if ( !mod ) {
		newSize = newIndexSize;
	} else {
		newSize = newIndexSize + granularity - mod;
	}

	if ( indexChain == INVALID_INDEX ) {
		indexSize = newSize;
		return;
	}

	oldIndexChain = indexChain;
	indexChain = new int[newSize];
	memcpy( indexChain, oldIndexChain, indexSize * sizeof(int) );
	memset( indexChain + indexSize, 0xff, (newSize - indexSize) * sizeof(int) );
	delete[] oldIndexChain;
	indexSize = newSize;
}

/*
================
idHashIndex::GetSpread
================
*/
int idHashIndex::GetSpread( void ) const {
	int i, index, totalItems, *numHashItems, average, error, e;

	if ( hash == INVALID_INDEX ) {
		return 100;
	}

	totalItems = 0;
	numHashItems = new int[hashSize];
	for ( i = 0; i < hashSize; i++ ) {
		numHashItems[i] = 0;
		for ( index = hash[i]; index >= 0; index = indexChain[index] ) {
			numHashItems[i]++;
		}
		totalItems += numHashItems[i];
	}
	// if no items in hash
	if ( totalItems <= 1 ) {
		delete[] numHashItems;
		return 100;
	}
	average = totalItems / hashSize;
	error = 0;
	for ( i = 0; i < hashSize; i++ ) {
		e = abs( numHashItems[i] - average );
		if ( e > 1 ) {
			error += e - 1;
		}
	}
	delete[] numHashItems;
	return 100 - (error * 100 / totalItems);
}
