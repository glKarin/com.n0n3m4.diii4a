// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __VECTORSET_H__
#define __VECTORSET_H__


template< class type >
struct sdVectorCompare_XYZ {
	sdVectorCompare_XYZ( float epsilon_ ) : epsilon( epsilon_ ) {}

	bool operator()( const type& lhs, const type& rhs ) const {
		for ( int i = 0; i < lhs.GetDimension(); i++ ) {
			if ( idMath::Fabs( lhs[i] - rhs[i] ) > epsilon ) {
				return false;
			}
		}
		return true;
	}

	float epsilon;
};

/*
===============================================================================

	Vector Set

	Creates a set of vectors without duplicates.

===============================================================================
*/

template< class type, class boundsType, int dimension >
class idVectorSet : public idList< type > {
public:
							idVectorSet( void );
							idVectorSet( const boundsType &mins, const boundsType &maxs, const int boxHashSize, const int initialSize );

							// returns total size of allocated memory
	size_t					Allocated( void ) const { return idList<type>::Allocated() + hash.Allocated(); }
							// returns total size of allocated memory including size of type
	size_t					Size( void ) const { return sizeof( *this ) + Allocated(); }

	void					Init( const boundsType &mins, const boundsType &maxs, const int boxHashSize, const int initialSize );
	int						HashSizeForBounds( const boundsType &mins, const boundsType &maxs, const float epsilon ) const;
	void					ResizeIndex( const int newSize );
	void					SetGranularity( int newGranularity );
	void					Clear( void );

	template< class Cmp >
	int						FindVector( const type &v, Cmp cmp ) {	
								int i, j, hashKey, partialHashKey[dimension];

								for ( i = 0; i < dimension; i++ ) {
									//assert( epsilon <= boxHalfSize[i] );
									partialHashKey[i] = (int) ( ( v[i] - mins[i] - boxHalfSize[i] ) * boxInvSize[i] );
								}

								for ( i = 0; i < ( 1 << dimension ); i++ ) {

									hashKey = 0;
									for ( j = 0; j < dimension; j++ ) {
										hashKey *= boxHashSize;
										hashKey += partialHashKey[j] + ( ( i >> j ) & 1 );
									}

									for ( j = hash.GetFirst( hashKey ); j != idHashIndexInt::NULL_INDEX; j = hash.GetNext( j ) ) {
										const type &lv = (*this)[j];
										if( cmp( lv, v ) ) {
											return j;
										}
									}
								}

								hashKey = 0;
								for ( i = 0; i < dimension; i++ ) {
									hashKey *= boxHashSize;
									hashKey += (int) ( ( v[i] - mins[i] ) * boxInvSize[i] );
								}

								hash.Add( hashKey, Num() );
								Append( v );
								return Num()-1;
							}

private:
	idHashIndexInt				hash;
	boundsType				mins;
	boundsType				maxs;
	int						boxHashSize;
	float					boxInvSize[dimension];
	float					boxHalfSize[dimension];
};

template< class type, class boundsType, int dimension >
ID_INLINE idVectorSet<type,boundsType,dimension>::idVectorSet( void ) {
	hash.Clear( idMath::IPow( boxHashSize, dimension ), 128 );
	boxHashSize = 16;
	memset( boxInvSize, 0, dimension * sizeof( boxInvSize[0] ) );
	memset( boxHalfSize, 0, dimension * sizeof( boxHalfSize[0] ) );
}

template< class type, class boundsType, int dimension >
ID_INLINE idVectorSet<type,boundsType,dimension>::idVectorSet( const boundsType &mins, const boundsType &maxs, const int boxHashSize, const int initialSize ) {
	Init( mins, maxs, boxHashSize, initialSize );
}

template< class type, class boundsType, int dimension >
ID_INLINE void idVectorSet<type,boundsType,dimension>::Init( const boundsType &mins, const boundsType &maxs, const int boxHashSize, const int initialSize ) {
	int i;
	float boxSize;

	idList<type>::AssureSize( initialSize );
	idList<type>::SetNum( 0, false );

	hash.Clear( idMath::IPow( boxHashSize, dimension ), initialSize );

	this->mins = mins;
	this->maxs = maxs;
	this->boxHashSize = boxHashSize;

	for ( i = 0; i < dimension; i++ ) {
		boxSize = ( maxs[i] - mins[i] ) / (float) boxHashSize;
		boxInvSize[i] = 1.0f / boxSize;
		boxHalfSize[i] = boxSize * 0.5f;
	}
}

template< class type, class boundsType, int dimension >
int idVectorSet<type,boundsType,dimension>::HashSizeForBounds( const boundsType &mins, const boundsType &maxs, const float epsilon ) const {
	int i;
	float min = idMath::INFINITY;
	for ( i = 0; i < dimension; i++ ) {
		if ( maxs[i] - mins[i] < min ) {
			min = maxs[i] - mins[i];
		}
	}
	min /= epsilon * 4;
	for ( i = 0; i < 7; i++ ) {
		if ( ( 1 << i ) > min ) {
			break;
		}
	}
	return 1 << i;
}

template< class type, class boundsType, int dimension >
ID_INLINE void idVectorSet<type,boundsType,dimension>::ResizeIndex( const int newSize ) {
	idList<type>::Resize( newSize );
	hash.ResizeIndex( newSize );
}

template< class type, class boundsType, int dimension >
ID_INLINE void idVectorSet<type,boundsType,dimension>::SetGranularity( int newGranularity ) {
	idList<type>::SetGranularity( newGranularity );
	hash.SetGranularity( newGranularity );
}

template< class type, class boundsType, int dimension >
ID_INLINE void idVectorSet<type,boundsType,dimension>::Clear( void ) {
	idList<type>::Clear();
	hash.Clear();
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

							// returns either vectorNum or an index to a previously found vector
	int						FindVector( const type *vectorList, const int vectorNum, const float epsilon );

private:
	idHashIndexInt				hash;
	type					mins;
	type					maxs;
	int						boxHashSize;
	float					boxInvSize[dimension];
	float					boxHalfSize[dimension];
};

template< class type, int dimension >
ID_INLINE idVectorSubset<type,dimension>::idVectorSubset( void ) {
	hash.Clear( idMath::IPow( boxHashSize, dimension ), 128 );
	boxHashSize = 16;
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

	hash.Clear( idMath::IPow( boxHashSize, dimension ), initialSize );

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

		for ( j = hash.GetFirst( hashKey ); j != idHashIndexInt::NULL_INDEX; j = hash.GetNext( j ) ) {
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
