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

#ifndef __VECTORSET_H__
#define __VECTORSET_H__

/*
===============================================================================

	Vector Set

	Creates a set of vectors without duplicates.

===============================================================================
*/

template< class type, int dimension >
class idVectorSet : public idList<type> {
public:
							idVectorSet( void );
							idVectorSet( const type &mins, const type &maxs, const int boxHashSize, const int initialSize );

							// returns total size of allocated memory
	size_t					Allocated( void ) const { return idList<type>::Allocated() + hash.Allocated(); }
							// returns total size of allocated memory including size of type
	size_t					Size( void ) const { return sizeof( *this ) + Allocated(); }

	void					Init( const type &mins, const type &maxs, const int boxHashSize, const int initialSize );
	void					ResizeIndex( const int newSize );
	void					Clear( void );
	void					ClearFree( void );

							// finds arbitrary match for the specified vector
							// if there is no match, then the specified vector is appended
	int						FindVector( const type &v, const float epsilon );

							// stgatilov: calls given callback for all matches for the specified vector
	template<class Lambda> void ForeachMatch( const type &v, const float epsilon, const Lambda &callback ) const;
							// stgatilov: appends specified vector (without search for matches)
	int						AddVector( const type &v );

private:
	idHashIndex				hash;
	type					mins;
	type					maxs;
	int						boxHashSize;
	float					boxInvSize[dimension];
	float					boxHalfSize[dimension];
};

template< class type, int dimension >
ID_INLINE idVectorSet<type,dimension>::idVectorSet( void ) {
	boxHashSize = 16;
	hash.ClearFree( idMath::IPow( boxHashSize, dimension ), 128 );
	memset( boxInvSize, 0, dimension * sizeof( boxInvSize[0] ) );
	memset( boxHalfSize, 0, dimension * sizeof( boxHalfSize[0] ) );
}

template< class type, int dimension >
ID_INLINE idVectorSet<type,dimension>::idVectorSet( const type &mins, const type &maxs, const int boxHashSize, const int initialSize ) {
	Init( mins, maxs, boxHashSize, initialSize );
}

template< class type, int dimension >
ID_INLINE void idVectorSet<type,dimension>::Init( const type &mins, const type &maxs, const int boxHashSize, const int initialSize ) {
	int i;
	float boxSize;

	idList<type>::AssureSize( initialSize );
	idList<type>::SetNum( 0, false );

	int hashSize = idMath::CeilPowerOfTwo( idMath::IPow( boxHashSize, dimension ) );
	if (hashSize != hash.GetHashSize())
		hash.ClearFree( hashSize, initialSize );
	else {
		hash.Clear();
		hash.ResizeIndex(initialSize);
	}

	this->mins = mins;
	this->maxs = maxs;
	this->boxHashSize = boxHashSize;

	for ( i = 0; i < dimension; i++ ) {
		boxSize = ( maxs[i] - mins[i] ) / (float) boxHashSize;
		boxInvSize[i] = 1.0f / boxSize;
		boxHalfSize[i] = boxSize * 0.5f;
	}
}

template< class type, int dimension >
ID_INLINE void idVectorSet<type,dimension>::ResizeIndex( const int newSize ) {
	idList<type>::Resize( newSize );
	hash.ResizeIndex( newSize );
}

template< class type, int dimension >
ID_INLINE void idVectorSet<type,dimension>::Clear( void ) {
	idList<type>::Clear();
	hash.Clear();
}

template< class type, int dimension >
ID_INLINE void idVectorSet<type,dimension>::ClearFree( void ) {
	idList<type>::ClearFree();
	hash.ClearFree();
}

template< class type, int dimension > template< class Lambda >
ID_INLINE void idVectorSet<type,dimension>::ForeachMatch( const type &v, const float epsilon, const Lambda &callback ) const {
	int i, j, k, hashKey, partialHashKey[dimension];

	for ( i = 0; i < dimension; i++ ) {
		assert( epsilon <= boxHalfSize[i] );
		partialHashKey[i] = (int) ( ( v[i] - mins[i] - boxHalfSize[i] ) * boxInvSize[i] );
	}

	for ( i = 0; i < ( 1 << dimension ); i++ ) {

		hashKey = 0;
		for ( j = 0; j < dimension; j++ ) {
			hashKey *= boxHashSize;
			hashKey += partialHashKey[j] + ( ( i >> j ) & 1 );
		}

		for ( j = hash.First( hashKey ); j >= 0; j = hash.Next( j ) ) {
			const type &lv = (*this)[j];
			for ( k = 0; k < dimension; k++ ) {
				if ( idMath::Fabs( lv[k] - v[k] ) > epsilon ) {
					break;
				}
			}
			if ( k >= dimension ) {
				if (callback(j, lv))
					return;
			}
		}
	}
}

template< class type, int dimension >
ID_INLINE int idVectorSet<type,dimension>::AddVector( const type &v ) {
	int i, hashKey;

	hashKey = 0;
	for ( i = 0; i < dimension; i++ ) {
		hashKey *= boxHashSize;
		hashKey += (int) ( ( v[i] - mins[i] ) * boxInvSize[i] );
	}

	hash.Add( hashKey, idList<type>::Num() );
	this->AddGrow( v );
	return idList<type>::Num()-1;
}

template< class type, int dimension >
ID_INLINE int idVectorSet<type,dimension>::FindVector( const type &v, const float epsilon ) {
	int match = -1;
	ForeachMatch(v, epsilon, [&](int idx, const type &vec){
		match = idx;
		return true;
	});
	if (match >= 0)
		return match;
	return AddVector(v);
}


/*
===============================================================================

	Vector Subset

	Creates a subset without duplicates from an existing list with vectors.

===============================================================================
*/

template< class type, int dimension >
class idVectorSubset {
public:
							idVectorSubset( void );
							idVectorSubset( const type &mins, const type &maxs, const int boxHashSize, const int initialSize );

							// returns total size of allocated memory
	size_t					Allocated( void ) const { return idList<type>::Allocated() + hash.Allocated(); }
							// returns total size of allocated memory including size of type
	size_t					Size( void ) const { return sizeof( *this ) + Allocated(); }

	void					Init( const type &mins, const type &maxs, const int boxHashSize, const int initialSize );
	void					Clear( void );
	void					ClearFree( void );

							// returns either vectorNum or an index to a previously found vector
	int						FindVector( const type *vectorList, const int vectorNum, const float epsilon );

private:
	idHashIndex				hash;
	type					mins;
	type					maxs;
	int						boxHashSize;
	float					boxInvSize[dimension];
	float					boxHalfSize[dimension];
};

template< class type, int dimension >
ID_INLINE idVectorSubset<type,dimension>::idVectorSubset( void ) {
	boxHashSize = 16;
	hash.ClearFree( idMath::IPow( boxHashSize, dimension ), 128 );
	memset( boxInvSize, 0, dimension * sizeof( boxInvSize[0] ) );
	memset( boxHalfSize, 0, dimension * sizeof( boxHalfSize[0] ) );
}

template< class type, int dimension >
ID_INLINE idVectorSubset<type,dimension>::idVectorSubset( const type &mins, const type &maxs, const int boxHashSize, const int initialSize ) {
	Init( mins, maxs, boxHashSize, initialSize );
}

template< class type, int dimension >
ID_INLINE void idVectorSubset<type,dimension>::Init( const type &mins, const type &maxs, const int boxHashSize, const int initialSize ) {
	int i;
	float boxSize;

	int hashSize = idMath::CeilPowerOfTwo( idMath::IPow( boxHashSize, dimension ) );
	if (hashSize != hash.GetHashSize())
		hash.ClearFree( hashSize, initialSize );
	else {
		hash.Clear();
		hash.ResizeIndex(initialSize);
	}

	this->mins = mins;
	this->maxs = maxs;
	this->boxHashSize = boxHashSize;

	for ( i = 0; i < dimension; i++ ) {
		boxSize = ( maxs[i] - mins[i] ) / (float) boxHashSize;
		boxInvSize[i] = 1.0f / boxSize;
		boxHalfSize[i] = boxSize * 0.5f;
	}
}

template< class type, int dimension >
ID_INLINE void idVectorSubset<type,dimension>::Clear( void ) {
	idList<type>::Clear();
	hash.Clear();
}

template< class type, int dimension >
ID_INLINE void idVectorSubset<type,dimension>::ClearFree( void ) {
	idList<type>::ClearFree();
	hash.ClearFree();
}

template< class type, int dimension >
ID_INLINE int idVectorSubset<type,dimension>::FindVector( const type *vectorList, const int vectorNum, const float epsilon ) {
	int i, j, k, hashKey, partialHashKey[dimension];
	const type &v = vectorList[vectorNum];

	for ( i = 0; i < dimension; i++ ) {
		assert( epsilon <= boxHalfSize[i] );
		partialHashKey[i] = (int) ( ( v[i] - mins[i] - boxHalfSize[i] ) * boxInvSize[i] );
	}

	for ( i = 0; i < ( 1 << dimension ); i++ ) {

		hashKey = 0;
		for ( j = 0; j < dimension; j++ ) {
			hashKey *= boxHashSize;
			hashKey += partialHashKey[j] + ( ( i >> j ) & 1 );
		}

		for ( j = hash.First( hashKey ); j >= 0; j = hash.Next( j ) ) {
			const type &lv = vectorList[j];
			for ( k = 0; k < dimension; k++ ) {
				if ( idMath::Fabs( lv[k] - v[k] ) > epsilon ) {
					break;
				}
			}
			if ( k >= dimension ) {
				return j;
			}
		}
	}

	hashKey = 0;
	for ( i = 0; i < dimension; i++ ) {
		hashKey *= boxHashSize;
		hashKey += (int) ( ( v[i] - mins[i] ) * boxInvSize[i] );
	}

	hash.Add( hashKey, vectorNum );
	return vectorNum;
}

#endif /* !__VECTORSET_H__ */
