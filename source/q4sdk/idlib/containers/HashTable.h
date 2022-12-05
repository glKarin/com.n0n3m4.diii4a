
#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

/*
===============================================================================

	General hash table. Slower than idHashIndex but it can also be used for
	linked lists and other data structures than just indexes or arrays.

===============================================================================
*/

template< class Type >
class idHashTable {
public:
					idHashTable( int newtablesize = 256 );
					idHashTable( const idHashTable<Type> &map );
					~idHashTable( void );

					// returns total size of allocated memory
	size_t			Allocated( void ) const;
					// returns total size of allocated memory including size of hash table type
	size_t			Size( void ) const;

	void			Set( const char *key, Type &value );
	bool			Get( const char *key, Type **value = NULL ) const;
	bool			Remove( const char *key );

	void			Clear( void );
	void			DeleteContents( void );

					// the entire contents can be itterated over, but note that the
					// exact index for a given element may change when new elements are added
	int				Num( void ) const;
	Type *			GetIndex( int index ) const;

	int				GetSpread( void ) const;

private:
	struct hashnode_s {
		idStr		key;
		Type		value;
		hashnode_s *next;

		hashnode_s( const idStr &k, Type v, hashnode_s *n ) : key( k ), value( v ), next( n ) {};
		hashnode_s( const char *k, Type v, hashnode_s *n ) : key( k ), value( v ), next( n ) {};
	};

	hashnode_s **	heads;

	int				tablesize;
	int				numentries;
	int				tablesizemask;

	int				GetHash( const char *key ) const;
};

/*
================
idHashTable<Type>::idHashTable
================
*/
template< class Type >
ID_INLINE idHashTable<Type>::idHashTable( int newtablesize ) {

	assert( idMath::IsPowerOfTwo( newtablesize ) );

	tablesize = newtablesize;
	assert( tablesize > 0 );

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT_CONTAINERS)
	bool ok=rvPushHeapContainingMemory(this);
#endif
// RAVEN END

	heads = new hashnode_s *[ tablesize ];

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT_CONTAINERS)
	if(ok)
	{
		RV_POP_HEAP();
	}
#endif
// RAVEN END

	memset( heads, 0, sizeof( *heads ) * tablesize );

	numentries		= 0;

	tablesizemask = tablesize - 1;
}

/*
================
idHashTable<Type>::idHashTable
================
*/
template< class Type >
ID_INLINE idHashTable<Type>::idHashTable( const idHashTable<Type> &map ) {
	int			i;
	hashnode_s	*node;
	hashnode_s	**prev;

	assert( map.tablesize > 0 );

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT_CONTAINERS)
	bool ok=rvPushHeapContainingMemory(this);
#endif
// RAVEN END

	tablesize		= map.tablesize;
	heads			= new hashnode_s *[ tablesize ];
	numentries		= map.numentries;
	tablesizemask	= map.tablesizemask;

	for( i = 0; i < tablesize; i++ ) {
		if ( !map.heads[ i ] ) {
			heads[ i ] = NULL;
			continue;
		}

		prev = &heads[ i ];
		for( node = map.heads[ i ]; node != NULL; node = node->next ) {
			*prev = new hashnode_s( node->key, node->value, NULL );
			prev = &( *prev )->next;
		}
	}

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT_CONTAINERS)
	if(ok)
	{
		RV_POP_HEAP();
	}
#endif
// RAVEN END
}

/*
================
idHashTable<Type>::~idHashTable<Type>
================
*/
template< class Type >
ID_INLINE idHashTable<Type>::~idHashTable( void ) {
	Clear();
	delete[] heads;
}

/*
================
idHashTable<Type>::Allocated
================
*/
template< class Type >
ID_INLINE size_t idHashTable<Type>::Allocated( void ) const {
	return sizeof( heads ) * tablesize + sizeof( *heads ) * numentries;
}

/*
================
idHashTable<Type>::Size
================
*/
template< class Type >
ID_INLINE size_t idHashTable<Type>::Size( void ) const {
	return sizeof( idHashTable<Type> ) + sizeof( heads ) * tablesize + sizeof( *heads ) * numentries;
}

/*
================
idHashTable<Type>::GetHash
================
*/
template< class Type >
ID_INLINE int idHashTable<Type>::GetHash( const char *key ) const {
	return ( idStr::Hash( key ) & tablesizemask );
}

/*
================
idHashTable<Type>::Set
================
*/
template< class Type >
ID_INLINE void idHashTable<Type>::Set( const char *key, Type &value ) {
	hashnode_s *node, **nextPtr;
	int hash, s;

	hash = GetHash( key );
	for( nextPtr = &(heads[hash]), node = *nextPtr; node != NULL; nextPtr = &(node->next), node = *nextPtr ) {
		s = node->key.Cmp( key );
		if ( s == 0 ) {
			node->value = value;
			return;
		}
		if ( s > 0 ) {
			break;
		}
	}

	numentries++;

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT_CONTAINERS)
	bool ok=rvPushHeapContainingMemory(this);
#endif
// RAVEN END

	*nextPtr = new hashnode_s( key, value, heads[ hash ] );
	(*nextPtr)->next = node;

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT_CONTAINERS)
	if(ok)
	{
		RV_POP_HEAP();
	}
#endif
// RAVEN END
}

/*
================
idHashTable<Type>::Get
================
*/
template< class Type >
ID_INLINE bool idHashTable<Type>::Get( const char *key, Type **value ) const {
	hashnode_s *node;
	int hash, s;

	hash = GetHash( key );
	for( node = heads[ hash ]; node != NULL; node = node->next ) {
		s = node->key.Cmp( key );
		if ( s == 0 ) {
			if ( value ) {
				*value = &node->value;
			}
			return true;
		}
		if ( s > 0 ) {
			break;
		}
	}

	if ( value ) {
		*value = NULL;
	}

	return false;
}

/*
================
idHashTable<Type>::GetIndex

the entire contents can be itterated over, but note that the
exact index for a given element may change when new elements are added
================
*/
template< class Type >
ID_INLINE Type *idHashTable<Type>::GetIndex( int index ) const {
	hashnode_s	*node;
	int			count;
	int			i;

	if ( ( index < 0 ) || ( index > numentries ) ) {
		assert( 0 );
		return NULL;
	}

	count = 0;
	for( i = 0; i < tablesize; i++ ) {
		for( node = heads[ i ]; node != NULL; node = node->next ) {
			if ( count == index ) {
				return &node->value;
			}
			count++;
		}
	}

	return NULL;
}

/*
================
idHashTable<Type>::Remove
================
*/
template< class Type >
ID_INLINE bool idHashTable<Type>::Remove( const char *key ) {
	hashnode_s	**head;
	hashnode_s	*node;
	hashnode_s	*prev;
	int			hash;

	hash = GetHash( key );
	head = &heads[ hash ];
	if ( *head ) {
		for( prev = NULL, node = *head; node != NULL; prev = node, node = node->next ) {
			if ( node->key == key ) {
				if ( prev ) {
					prev->next = node->next;
				} else {
					*head = node->next;
				}

				delete node;
				numentries--;
				return true;
			}
		}
	}

	return false;
}

/*
================
idHashTable<Type>::Clear
================
*/
template< class Type >
ID_INLINE void idHashTable<Type>::Clear( void ) {
	int			i;
	hashnode_s	*node;
	hashnode_s	*next;

	for( i = 0; i < tablesize; i++ ) {
		next = heads[ i ];
		while( next != NULL ) {
			node = next;
			next = next->next;
			delete node;
		}

		heads[ i ] = NULL;
	}

	numentries = 0;
}

/*
================
idHashTable<Type>::DeleteContents
================
*/
template< class Type >
ID_INLINE void idHashTable<Type>::DeleteContents( void ) {
	int			i;
	hashnode_s	*node;
	hashnode_s	*next;

	for( i = 0; i < tablesize; i++ ) {
		next = heads[ i ];
		while( next != NULL ) {
			node = next;
			next = next->next;
			delete node->value;
			delete node;
		}

		heads[ i ] = NULL;
	}

	numentries = 0;
}

/*
================
idHashTable<Type>::Num
================
*/
template< class Type >
ID_INLINE int idHashTable<Type>::Num( void ) const {
	return numentries;
}

#if !defined(__GNUC__) || __GNUC__ < 4
/*
================
idHashTable<Type>::GetSpread
================
*/
template< class Type >
int idHashTable<Type>::GetSpread( void ) const {
	int i, average, error, e;
	hashnode_s	*node;

	// if no items in hash
	if ( !numentries ) {
		return 100;
	}
	average = numentries / tablesize;
	error = 0;
	for ( i = 0; i < tablesize; i++ ) {
		numItems = 0;
		for( node = heads[ i ]; node != NULL; node = node->next ) {
			numItems++;
		}
		e = abs( numItems - average );
		if ( e > 1 ) {
			error += e - 1;
		}
	}
	return 100 - (error * 100 / numentries);
}
#endif

#endif /* !__HASHTABLE_H__ */
