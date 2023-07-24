// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __HASHMAP_H__
#define __HASHMAP_H__

/*
===============================================================================

	General hash table. Slower than idHashIndex but it can also be used for
	linked lists and other data structures than just indexes or arrays.

===============================================================================
*/

template< class Type >
class idHashMap {
public:
					idHashMap();
					idHashMap( const idHashMap<Type> &map );
					~idHashMap( void );

					// returns total size of allocated memory
	size_t			Allocated( void ) const;
					// returns total size of allocated memory including size of hash table type
	size_t			Size( void ) const;

	Type &			Set( const char *key, const Type &value );
	bool			Get( const char *key, Type **value = NULL );
	bool			Get( const char *key, const Type **value = NULL ) const;

					// the entire contents can be iterated over, but note that the
					// exact index for a given element may change when new elements are added
	int				Num( void ) const;
	const idStr	&	GetKey( int index ) const;
	Type *			GetIndex( int index );
	const Type *	GetIndex( int index ) const;

	bool			Remove( const char *key );

	const idList<idStr> &GetKeyList( void ) const { return keyList; }
	const idList<Type>  &GetValList( void ) const { return list; }

	void			Clear( void );
	void			DeleteContents( bool clear = true );

					// returns number in the range [0-100] representing the spread over the hash table
	int				GetSpread( void ) const;

	void			Swap( idHashMap& rhs );

private:
	idList<Type>	list;
	idHashIndex		hashIndex;
	idList<idStr>	keyList;
};

/*
================
idHashMap<Type>::idHashMap
================
*/
template< class Type >
ID_INLINE idHashMap<Type>::idHashMap() {
}

/*
================
idHashMap<Type>::idHashMap
================
*/
template< class Type >
ID_INLINE idHashMap<Type>::idHashMap( const idHashMap<Type> &map ) {
	list = map.list;
	index = map.index;
	keyList = map.keyList;
}

/*
================
idHashMap<Type>::~idHashMap<Type>
================
*/
template< class Type >
ID_INLINE idHashMap<Type>::~idHashMap( void ) {
	Clear();
}

/*
================
idHashMap<Type>::Allocated
================
*/
template< class Type >
ID_INLINE size_t idHashMap<Type>::Allocated( void ) const {
	return list.Allocated();
}

/*
================
idHashMap<Type>::Size
================
*/
template< class Type >
ID_INLINE size_t idHashMap<Type>::Size( void ) const {
	return list.Size() + hashIndex.Size() + keyList.Size();
}

/*
================
idHashMap<Type>::Set
Sets a key in the table to be a certain value, adds it if it's not already there
Returns a reference to the value in the table
================
*/
template< class Type >
ID_INLINE Type &idHashMap<Type>::Set( const char *key, const Type& value ) {
	Type *existingValue;
	if ( Get( key, &existingValue ) ) {
		*existingValue = value;
		return *existingValue;
	} else {
		int index = list.Append( value );
		keyList.Append( key );
		hashIndex.Add( hashIndex.GenerateKey( key ), index );
		return list[index];
	}
}

/*
================
idHashMap<Type>::Remove
================
*/
template< class Type >
ID_INLINE bool idHashMap<Type>::Remove( const char *key ) {
	int hashKey = hashIndex.GenerateKey( key );	
	int i;
	for ( i = hashIndex.GetFirst( hashKey ); i != idHashIndex::NULL_INDEX; i = hashIndex.GetNext( i ) ) {
		if ( keyList[ i ].Cmp( key ) == 0 ) {
			hashIndex.RemoveIndex( hashKey, i );
			keyList.RemoveIndex( i );
			list.RemoveIndex( i );
			return true;
		}
	}
	return false;
}



/*
===============
idHashMap<Type>::Get
===============
*/
template< class Type >
ID_INLINE bool idHashMap<Type>::Get( const char *key, Type **value ) {
	int hashKey = hashIndex.GenerateKey( key );
	int i;
	for ( i = hashIndex.GetFirst( hashKey ); i != idHashIndex::NULL_INDEX; i = hashIndex.GetNext( i ) ) {
		if ( keyList[i].Cmp( key ) == 0 ) {
			if ( value ) {
				*value = &list[ i ];
			}
			return true;
		}
	}
	if ( value ) {
		*value = NULL;
	}
	return false;
}

/*
===============
idHashMap<Type>::Get
===============
*/
template< class Type >
ID_INLINE bool idHashMap<Type>::Get( const char *key, const Type **value ) const {
	return const_cast<idHashMap<Type> *>(this)->Get( key, const_cast<Type **>( value ) );
}

/*
================
idHashMap<Type>::GetIndex

the entire contents can be itterated over, but note that the
exact index for a given element may change when new elements are added
================
*/
template< class Type >
ID_INLINE Type *idHashMap<Type>::GetIndex( int index ) {
	if ( index >= 0 && index < list.Num() ) {
		return &list[index];
	}
	return NULL;
}

/*
================
idHashMap<Type>::GetIndex

the entire contents can be itterated over, but note that the
exact index for a given element may change when new elements are added
================
*/
template< class Type >
ID_INLINE const Type *idHashMap<Type>::GetIndex( int index ) const {
	if ( index >= 0 && index < list.Num() ) {
		return &list[index];
	}
	return NULL;
}

/*
================
idHashMap<Type>::GetKey

the entire contents can be itterated over, but note that the
exact index for a given element may change when new elements are added
================
*/
template< class Type >
ID_INLINE const idStr &idHashMap<Type>::GetKey( int index ) const {
	if ( index >= 0 && index < list.Num() ) {
		return keyList[index];
	}
	static idStr blank;
	return blank;
}

/*
================
idHashMap<Type>::Clear
================
*/
template< class Type >
ID_INLINE void idHashMap<Type>::Clear( void ) {
	list.Clear();
	hashIndex.Clear();
	keyList.Clear();
}

/*
================
idHashMap<Type>::DeleteContents
================
*/
template< class Type >
ID_INLINE void idHashMap<Type>::DeleteContents( bool clear ) {
	list.DeleteContents( clear );
	if ( clear ) {
		// list already cleared
		hashIndex.Clear();
		keyList.Clear();
	}
}

/*
================
idHashMap<Type>::Num
================
*/
template< class Type >
ID_INLINE int idHashMap<Type>::Num( void ) const {
	return list.Num();
}

/*
================
idHashMap<Type>::GetSpread
================
*/
template< class Type >
ID_INLINE int idHashMap<Type>::GetSpread( void ) const {
	return hashIndex.GetSpread();
}

/*
============
idHashMap<Type>::Swap
============
*/
template< class Type >
ID_INLINE void idHashMap<Type>::Swap( idHashMap& rhs ) {
	hashIndex.Swap( rhs.hashIndex );
	list.Swap( rhs.list );
	keyList.Swap( rhs.keyList );
}

#endif /* !__HASHMAP_H__ */
