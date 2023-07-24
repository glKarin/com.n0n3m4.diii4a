// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __HASHINDEX_H__
#define __HASHINDEX_H__

/*
===============================================================================

	Fast hash table for indexes and arrays.
	Does not allocate memory until the first key/index pair is added.

===============================================================================
*/

template< class type, int Max, int NullIndex, int DEFAULT_HASH_SIZE, int DEFAULT_HASH_GRANULARITY >
class idHashIndexBase {
public:
	static const int NULL_INDEX = NullIndex;
	typedef type Type;

					idHashIndexBase( void );
					idHashIndexBase( const int initialHashSize, const int initialIndexSize );
					~idHashIndexBase( void );

					// returns total size of allocated memory
	size_t			Allocated( void ) const;
	size_t			ReducedAllocated( void ) const;
					// returns total size of allocated memory including size of hash index type
	size_t			Size( void ) const;

	idHashIndexBase &	operator=( const idHashIndexBase &other );
					// add an index to the hash, assumes the index has not yet been added to the hash
	void			Add( const int key, const int index );
					// remove an index from the hash
	void			Remove( const int key, const int index );
					// get the first index from the hash, returns NULL_INDEX if empty hash entry
	int				GetFirst( const int key ) const;
					// get the next index from the hash, returns NULL_INDEX if at the end of the hash chain
	int				GetNext( const int index ) const;
					// insert an entry into the index and add it to the hash, increasing all indexes >= index
	void			InsertIndex( const int key, const int index );
					// remove an entry from the index and remove it from the hash, decreasing all indexes >= index
	void			RemoveIndex( const int key, const int index );
					// clear the hash
	void			Clear( void );
					// clear and resize
	void			Clear( const int newHashSize, const int newIndexSize );
					// free allocated memory
	void			Free( void );
					// allocate memory, only used internally
	void			Allocate( const int newHashSize, const int newIndexSize );
					// get size of hash table
	int				GetHashSize( void ) const;
					// get size of the index
	int				GetIndexSize( void ) const;
					// set granularity
	void			SetGranularity( const int newGranularity );
					// force resizing the index, current hash table stays intact
	void			ResizeIndex( const int newIndexSize );
					// returns number in the range [0-100] representing the spread over the hash table
	int				GetSpread( void ) const;
					// returns a key for a string
	int				GenerateKey( const char *string, bool caseSensitive = true ) const;
					// returns a key for a vector
	int				GenerateKey( const idVec3 &v ) const;
					// returns a key for two integers
	int				GenerateKey( const unsigned int n1, const unsigned int n2 ) const;
					// returns a key for a single integer
	int				GenerateKey( const int n ) const;
					// returns a key a pointer type					
	int				GenerateKey( const void* n ) const;

					// returns a key for a file name
	int				GenerateKeyForFileName( const char *string ) const;

	void			Swap( idHashIndexBase &rhs );

	bool			Verify( void );

	void			Write( idFile* file ) const;
	void			Read( idFile* file );

public:
	void			Init( const int initialHashSize, const int initialIndexSize );

private:
	int				hashSize;
	Type*			hash;
	int				indexSize;
	Type*			indexChain;
	int				granularity;
	int				hashMask;

	void CheckBounds() {
		assert( hashSize < Max && indexSize < Max && "size too big" );
	}
};

#define HASHINDEX_TEMPLATE_HEADER template< class type, int Max, int NullIndex, int DEFAULT_HASH_SIZE, int DEFAULT_HASH_GRANULARITY >
#define HASHINDEX_TEMPLATE_TAG idHashIndexBase< type, Max, NullIndex, DEFAULT_HASH_SIZE, DEFAULT_HASH_GRANULARITY >

/*
================
idHashIndexBase::idHashIndexBase
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE HASHINDEX_TEMPLATE_TAG::idHashIndexBase( void ) {
	Init( DEFAULT_HASH_SIZE, 0 );
}

/*
================
idHashIndexBase::idHashIndexBase
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE HASHINDEX_TEMPLATE_TAG::idHashIndexBase( const int initialHashSize, const int initialIndexSize ) {
	Init( initialHashSize, initialIndexSize );
}

/*
================
idHashIndexBase::~idHashIndexBase
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE HASHINDEX_TEMPLATE_TAG::~idHashIndexBase( void ) {
	Free();
}

/*
================
idHashIndexBase::Init
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::Init( const int initialHashSize, const int initialIndexSize ) {
	assert( idMath::IsPowerOfTwo( initialHashSize ) );

	hashSize = initialHashSize;
	hash = NULL;
	indexSize = initialIndexSize;
	indexChain = NULL;
	CheckBounds();
	granularity = DEFAULT_HASH_GRANULARITY;
	hashMask = hashSize - 1;
}

/*
================
idHashIndexBase::Allocate
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::Allocate( const int newHashSize, const int newIndexSize ) {
	assert( idMath::IsPowerOfTwo( newHashSize ) );

	Free();
	hashSize = newHashSize;
	hash = new type[hashSize];
	memset( hash, 0xff, hashSize * sizeof( hash[0] ) );
	indexSize = newIndexSize;
	indexChain = new type[indexSize];
	CheckBounds();
	memset( indexChain, 0xff, indexSize * sizeof( indexChain[0] ) );
	hashMask = hashSize - 1;
}

/*
============
idHashIndexBase::Free
============
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::Free( void ) {
	delete[] hash;
	hash = NULL;
	delete[] indexChain;
	indexChain = NULL;
}

/*
================
idHashIndexBase::ResizeIndex
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::ResizeIndex( const int newIndexSize ) {
	type *oldIndexChain;
	int mod, newSize;

	if ( newIndexSize <= indexSize ) {
		return;
	}

	mod = newIndexSize % granularity;
	if ( !mod ) {
		newSize = newIndexSize;
	} else {
		newSize = newIndexSize + granularity - mod;
	}

	if ( indexChain == NULL ) {
		indexSize = newSize;
		return;
	}

	oldIndexChain = indexChain;
	indexChain = new type[newSize];
	memcpy( indexChain, oldIndexChain, indexSize * sizeof( indexChain[0] ) );
	memset( indexChain + indexSize, 0xff, (newSize - indexSize) * sizeof( indexChain[0] ) );
	delete[] oldIndexChain;
	indexSize = newSize;
	CheckBounds();
}

/*
================
idHashIndexBase::GetSpread
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE int HASHINDEX_TEMPLATE_TAG::GetSpread( void ) const {
	int i, index, totalItems, average, error, e;
	type *numHashItems;

	if ( hash == NULL ) {
		return 100;
	}

	totalItems = 0;
	numHashItems = new type[hashSize];
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
		e = idMath::Abs( numHashItems[i] - average );
		if ( e > 1 ) {
			error += e - 1;
		}
	}
	delete[] numHashItems;
	return 100 - (error * 100 / totalItems);
}

/*
================
idHashIndexBase::Add
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::Add( const int key, const int index ) {
	int h;

	assert( index >= 0 );
	if ( hash == NULL ) {
		int size = ( index >= indexSize ? index + 1 : indexSize ) + granularity - 1;
		Allocate( hashSize, size - size % granularity );
	} else if ( index >= indexSize ) {
		ResizeIndex( index + 1 );
	}

	h = key & hashMask;

#ifdef _DEBUG
	int oldChain	= indexChain[ index ];
	int oldHash		= hash[ h ];
#endif
	assert( oldChain == NULL_INDEX && oldHash != index );

	indexChain[ index ] = hash[h];
	hash[ h ] = index;
}

/*
================
idHashIndexBase::Allocated
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE size_t HASHINDEX_TEMPLATE_TAG::Allocated( void ) const {
	size_t total = 0;
	if( hash ) {
		total += hashSize * sizeof( hash[ 0 ] );
	}
	if(indexChain) {
		total += indexSize * sizeof( indexChain[ 0 ] );
	}
	return total;
}

/*
================
idHashIndexBase::Size
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE size_t HASHINDEX_TEMPLATE_TAG::Size( void ) const {
	return sizeof( *this ) + Allocated();
}

/*
================
idHashIndexBase::operator=
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE HASHINDEX_TEMPLATE_TAG& HASHINDEX_TEMPLATE_TAG::operator=( const idHashIndexBase &other ) {
	if( &other == this ) {
		return *this;
	}

	granularity = other.granularity;
	hashMask = other.hashMask;

	delete[] hash;
	hash = NULL;

	delete[] indexChain;
	indexChain = NULL;

	hashSize = other.hashSize;
	indexSize = other.indexSize;
	CheckBounds();

	if ( other.hash ) {
		hash = new type[hashSize];
		memcpy( hash, other.hash, hashSize * sizeof( hash[0] ) );
	}
	if ( other.indexChain ) {
		indexChain = new type[indexSize];
		memcpy( indexChain, other.indexChain, indexSize * sizeof( indexChain[0] ) );
	}

	return *this;
}

/*
================
idHashIndexBase::Remove
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::Remove( const int key, const int index ) {
	int k = key & hashMask;

	if ( hash == NULL ) {
		return;
	}
	if ( hash[k] == index ) {
		hash[k] = indexChain[index];
	}
	else {
		for ( int i = hash[k]; i != NULL_INDEX; i = indexChain[i] ) {
			if ( indexChain[i] == index ) {
				indexChain[i] = indexChain[index];
				break;
			}
		}
	}
	indexChain[index] = NULL_INDEX;
}

/*
================
idHashIndexBase::GetFirst
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE int HASHINDEX_TEMPLATE_TAG::GetFirst( const int key ) const {
	return hash ? hash[ key & hashMask ] : NULL_INDEX;
}

/*
================
idHashIndexBase::GetNext
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE int HASHINDEX_TEMPLATE_TAG::GetNext( const int index ) const {
	assert( index >= 0 && index < indexSize );
	return indexChain ? indexChain[ index ] : NULL_INDEX;
}

/*
================
idHashIndexBase::InsertIndex
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::InsertIndex( const int key, const int index ) {
	int i, max;

	if ( hash != NULL ) {
		max = index;
		for ( i = 0; i < hashSize; i++ ) {
			if ( hash[i] >= index ) {
				hash[i]++;
				if ( hash[i] > max ) {
					max = hash[i];
				}
			}
		}
		for ( i = 0; i < indexSize; i++ ) {
			if ( indexChain[i] >= index ) {
				indexChain[i]++;
				if ( indexChain[i] > max ) {
					max = indexChain[i];
				}
			}
		}
		if ( max >= indexSize ) {
			ResizeIndex( max + 1 );
		}
		for ( i = max; i > index; i-- ) {
			indexChain[i] = indexChain[i-1];
		}
		indexChain[index] = NULL_INDEX;
	}
	Add( key, index );
}

/*
================
idHashIndexBase::RemoveIndex
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::RemoveIndex( const int key, const int index ) {
	int i, max;

	Remove( key, index );
	if ( hash != NULL ) {
		max = index;
		for ( i = 0; i < hashSize; i++ ) {
			if ( hash[i] >= index ) {
				if ( hash[i] > max ) {
					max = hash[i];
				}
				hash[i]--;
			}
		}
		for ( i = 0; i < indexSize; i++ ) {
			if ( indexChain[i] >= index ) {
				if ( indexChain[i] > max ) {
					max = indexChain[i];
				}
				indexChain[i]--;
			}
		}
		for ( i = index; i < max; i++ ) {
			indexChain[i] = indexChain[i+1];
		}
		indexChain[max] = NULL_INDEX;
	}
}

/*
================
idHashIndexBase::Clear
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::Clear( void ) {
	if ( hash != NULL ) {
		memset( hash, 0xff, hashSize * sizeof( hash[0] ) );
	}
	if ( indexChain != NULL ) {
		memset( indexChain, 0xff, indexSize * sizeof( indexChain[0] ) );
	}
}

/*
================
idHashIndexBase::Clear
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::Clear( const int newHashSize, const int newIndexSize ) {
	Free();
	hashSize = newHashSize;
	indexSize = newIndexSize;
	CheckBounds();
}

/*
================
idHashIndexBase::GetHashSize
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE int HASHINDEX_TEMPLATE_TAG::GetHashSize( void ) const {
	return hashSize;
}

/*
================
idHashIndexBase::GetIndexSize
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE int HASHINDEX_TEMPLATE_TAG::GetIndexSize( void ) const {
	return indexSize;
}

/*
================
idHashIndexBase::SetGranularity
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::SetGranularity( const int newGranularity ) {
	assert( newGranularity > 0 );
	granularity = newGranularity;
}

/*
================
idHashIndexBase::GenerateKey
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE int HASHINDEX_TEMPLATE_TAG::GenerateKey( const char *string, bool caseSensitive ) const {
	if ( caseSensitive ) {
		return ( idStr::Hash( string ) & hashMask );
	} else {
		return ( idStr::IHash( string ) & hashMask );
	}
}

/*
================
idHashIndexBase::GenerateKey
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE int HASHINDEX_TEMPLATE_TAG::GenerateKey( const idVec3 &v ) const {
	return ( ( idMath::Ftoi( v[0] ) + idMath::Ftoi( v[1] ) + idMath::Ftoi( v[2] ) ) & hashMask );
}

/*
================
idHashIndexBase::GenerateKey
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE int HASHINDEX_TEMPLATE_TAG::GenerateKey( const unsigned int n1, const unsigned int n2 ) const {
	return ( ( n1 + n2 ) & hashMask );
}

/*
================
idHashIndexBase::GenerateKey
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE int HASHINDEX_TEMPLATE_TAG::GenerateKey( const int n ) const {
	return n & hashMask;
}

/*
================
idHashIndexBase::GenerateKey
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE int HASHINDEX_TEMPLATE_TAG::GenerateKey( const void* n ) const {
	return ( ( ( UINT_PTR ) n ) & hashMask );
}

/*
================
idHashIndexBase::GenerateKeyForFileName
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE int HASHINDEX_TEMPLATE_TAG::GenerateKeyForFileName( const char *string ) const {
	return idStr::FileNameHash( string, hashSize );
}

/*
================
idHashIndexBase::Verify
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE bool HASHINDEX_TEMPLATE_TAG::Verify( void ) {
	int i, j;
	int c;
	int count = GetHashSize();
	for ( i = 0; i < count; i++ ) {
		for ( c = GetFirst( i ), j = 0; c != NULL_INDEX && j < GetIndexSize(); j++, c = GetNext( c ) ) {
		}
		if ( j == GetIndexSize() ) {
			return false;
		}
	}
	return true;
}

/*
================
idHashIndexBase::Swap
================
*/
HASHINDEX_TEMPLATE_HEADER
ID_INLINE void HASHINDEX_TEMPLATE_TAG::Swap( idHashIndexBase &rhs ) {
	idSwap( hashSize, rhs.hashSize );
	idSwap( hash, rhs.hash );
	idSwap( indexSize, rhs.indexSize );
	idSwap( indexChain, rhs.indexChain );
	idSwap( granularity, rhs.granularity );
	idSwap( hashMask, rhs.hashMask );
}

// Read and Write are in HashIndexImpl.h to avoid dependency issues

#undef HASHINDEX_TEMPLATE_HEADER
#undef HASHINDEX_TEMPLATE_TAG

typedef idHashIndexBase< short, SHRT_MAX, -1, 256, 256 > idHashIndexShort;
typedef idHashIndexBase< unsigned short, USHRT_MAX - 1, USHRT_MAX, 256, 256 > idHashIndexUShort;
typedef idHashIndexBase< int, INT_MAX, -1, 1024, 1024 > idHashIndexInt;

#ifdef SD_USE_HASHINDEX_16
typedef idHashIndexShort idHashIndex;
#else
typedef idHashIndexInt idHashIndex;
#endif

#endif /* !__HASHINDEX_H__ */
