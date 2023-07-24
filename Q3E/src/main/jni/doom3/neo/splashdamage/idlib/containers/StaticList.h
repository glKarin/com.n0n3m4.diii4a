// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __STATICLIST_H__
#define __STATICLIST_H__

/*
===============================================================================

	Static list template
	A non-growing, memset-able list using no memory allocation.

===============================================================================
*/

template<class type,int size>
class idStaticList {
public:
	typedef int		cmp_t( const type *, const type * );

	typedef const type	ConstType;
	typedef type		Type;
	typedef Type*		Iterator;
	typedef ConstType*	ConstIterator;

						idStaticList();
						idStaticList( const idStaticList<type,size> &other );
						~idStaticList( void );

	void				Clear( void );										// marks the list as empty.  does not deallocate or intialize data.
	int					Num( void ) const;									// returns number of elements in list
	int					Max( void ) const;									// returns the maximum number of elements in the list
	void				SetNum( int newnum );								// set number of elements in list
	bool				Empty( void ) const;								// returns true if number elements in list is 0

	size_t				Allocated( void ) const;							// returns total size of allocated memory
	size_t				Size( void ) const;									// returns total size of allocated memory including size of list type
	size_t				MemoryUsed( void ) const;							// returns size of the used elements in the list

	const type &		operator[]( int index ) const;
	type &				operator[]( int index );

	type *				Alloc( void );										// returns reference to a new data element at the end of the list.  returns NULL when full.
	int					Append( const type & obj );							// append element
	int					Append( const idStaticList<type,size> &other );		// append list
	int					AddUnique( const type & obj );						// add unique element
	int					Insert( const type & obj, int index );				// insert the element at the given index
	int					FindIndex( const type & obj ) const;				// find the index for the given element
	const type *		FindElement( type const & obj ) const;				// find pointer to the given element
	int					FindNull( void ) const;								// find the index for the first NULL pointer in the list

	bool				RemoveIndex( int index );							// remove the element at the given index
	bool				RemoveIndexFast( int index );						// remove the element at the given index and put the last element into its spot
	bool				Remove( const type & obj );							// remove the element
	bool				RemoveFast( const type & obj );						// remove the element, move the last element into its spot

	void				Swap( idStaticList<type,size> &other );				// swap the contents of the lists
	void				DeleteContents( bool clear );						// delete the contents of the list
	void				Sort( cmp_t *compare );

	// members for compatibility with STL algorithms
	Iterator			Begin( void );										// return list[ 0 ]
	Iterator			End( void );										// return list[ num ]	(one past end)

	ConstIterator		Begin( void ) const;								// return list[ 0 ]
	ConstIterator		End( void ) const;									// return list[ num ]	(one past end)

	type&				Front();											// return *list[ 0 ]
	const type&			Front() const;										// return *list[ 0 ]

	type&				Back();												// return *list[ num - 1 ]
	const type&			Back() const;										// return *list[ num - 1 ]

	bool				Remove( Iterator iter );

	// Comparison functions
	// find an object that passes the predicate
	// IMPORTANT: unlike Find, this returns End() if the element is not found, NOT NULL	
	template< class Cmp >
	Iterator		FindIteratorIf( Cmp predicate ) {
						Iterator begin = Begin();
						Iterator end = End();
						while( begin != end ) {
							if( predicate( *begin ) ) {
								break;
							}
							++begin;
						}
						return begin;
					}

	// find an object that passes the predicate
	// IMPORTANT: unlike Find, this returns End() if the element is not found, NOT NULL
	template< class Cmp >
	ConstIterator	FindIteratorIf( Cmp predicate ) const {
						ConstIterator begin = Begin();
						ConstIterator end = End();
						while( begin != end ) {
							if( predicate( *begin ) ) {
								break;
							}
							++begin;
						}
						return begin;
					}

	// Comparison functions
	// find an object that passes the predicate
	// IMPORTANT: unlike Find, this returns End() if the element is not found, NOT NULL	
	template< class searchType, class Cmp >
	Iterator		FindIteratorIf( const searchType& element, const Cmp predicate ) {
						Iterator begin = Begin();
						Iterator end = End();
						while( begin != end ) {
							if( predicate( element, *begin ) ) {
								break;
							}
							++begin;
						}
						return begin;
					}

	// find an object that passes the predicate
	// IMPORTANT: unlike Find, this returns End() if the element is not found, NOT NULL
	template< class searchType, class Cmp >
	ConstIterator	FindIteratorIf( const searchType& element, Cmp predicate ) const {
						ConstIterator begin = Begin();
						ConstIterator end = End();
						while( begin != end ) {
							if( predicate( element, *begin ) ) {
								break;
							}
							++begin;
						}
						return begin;
					}

	Iterator		FindIterator( type const & obj ) {
						Iterator begin = Begin();
						Iterator end = End();
						while( begin != end ) {
							if( *begin == obj ) {
								break;
							}
							++begin;
						}
						return begin;
					}

	ConstIterator	FindIterator( type const & obj ) const {
						ConstIterator begin = Begin();
						ConstIterator end = End();
						while( begin != end ) {
							if( *begin == obj ) {
								break;
							}
							++begin;
						}
						return begin;
					}

	// Sorting functions
	template< class Cmp >
	void			Sort( Cmp compare ) {
						sdQuickSort( Begin(), End(), compare );
					}

	// Searching functions
	// IMPORTANT: unlike FindElement, this returns End() if the element is not found, NOT NULL
	template< class Cmp >
	Iterator		FindIteratorBinary( const type& searchElement, Cmp compare ) {
						return sdBinarySearch( searchElement, Begin(), End(), compare );
					}
	
	// IMPORTANT: unlike FindElement, this returns End() if the element is not found, NOT NULL
	template< class Cmp >
	ConstIterator	FindIteratorBinary( const type& searchElement, Cmp compare ) const {
						return sdBinarySearch( searchElement, Begin(), End(), compare );
					}

	template< class Iter >
	void			CopyFromRange( Iter begin, Iter end ) {
						SetNum( end - begin );
						for( int i = 0; i < num; i++, ++begin ) {
							list[ i ] = *begin;
						}
					}

protected:
	int					num;
	type 				list[ size ];
};

/*
================
idStaticList<type,size>::idStaticList()
================
*/
template<class type,int size>
ID_INLINE idStaticList<type,size>::idStaticList() {
	num = 0;
}

/*
================
idStaticList<type,size>::idStaticList( const idStaticList<type,size> &other )
================
*/
template<class type,int size>
ID_INLINE idStaticList<type,size>::idStaticList( const idStaticList<type,size> &other ) {
	*this = other;
}

/*
================
idStaticList<type,size>::~idStaticList<type,size>
================
*/
template<class type,int size>
ID_INLINE idStaticList<type,size>::~idStaticList( void ) {
}

/*
================
idStaticList<type, size>::Empty
================
*/
template< class type, int size >
ID_INLINE bool idStaticList<type, size>::Empty() const {
	return num == 0;
}

/*
================
idStaticList<type,size>::Clear

Sets the number of elements in the list to 0.  Assumes that type automatically handles freeing up memory.
================
*/
template<class type,int size>
ID_INLINE void idStaticList<type,size>::Clear( void ) {
	num	= 0;
}

/*
================
idStaticList<type,size>::DeleteContents

Calls the destructor of all elements in the list.  Conditionally frees up memory used by the list.
Note that this only works on lists containing pointers to objects and will cause a compiler error
if called with non-pointers.  Since the list was not responsible for allocating the object, it has
no information on whether the object still exists or not, so care must be taken to ensure that
the pointers are still valid when this function is called.  Function will set all pointers in the
list to NULL.
================
*/
template<class type,int size>
ID_INLINE void idStaticList<type,size>::DeleteContents( bool clear ) {
	int i;

	for( i = 0; i < num; i++ ) {
		delete list[ i ];
		list[ i ] = NULL;
	}

	if ( clear ) {
		Clear();
	} else {
		memset( list, 0, sizeof( list ) );
	}
}

/*
================
idStaticList<type,size>::Num

Returns the number of elements currently contained in the list.
================
*/
template<class type,int size>
ID_INLINE int idStaticList<type,size>::Num( void ) const {
	return num;
}

/*
================
idStaticList<type,size>::Num

Returns the maximum number of elements in the list.
================
*/
template<class type,int size>
ID_INLINE int idStaticList<type,size>::Max( void ) const {
	return size;
}

/*
================
idStaticList<type>::Allocated
================
*/
template<class type,int size>
ID_INLINE size_t idStaticList<type,size>::Allocated( void ) const {
	return size * sizeof( type );
}

/*
================
idStaticList<type>::Size
================
*/
template<class type,int size>
ID_INLINE size_t idStaticList<type,size>::Size( void ) const {
	return sizeof( idStaticList<type,size> ) + Allocated();
}

/*
================
idStaticList<type,size>::Num
================
*/
template<class type,int size>
ID_INLINE size_t idStaticList<type,size>::MemoryUsed( void ) const {
	return num * sizeof( list[ 0 ] );
}

/*
================
idStaticList<type,size>::SetNum

Set number of elements in list.
================
*/
template<class type,int size>
ID_INLINE void idStaticList<type,size>::SetNum( int newnum ) {
	assert( newnum >= 0 );
	assert( newnum <= size );
	num = newnum;
}

/*
================
idStaticList<type,size>::operator[] const

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template<class type,int size>
ID_INLINE const type &idStaticList<type,size>::operator[]( int index ) const {
	assert( index >= 0 );
	assert( index < num );

	return list[ index ];
}

/*
================
idStaticList<type,size>::operator[]

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template<class type,int size>
ID_INLINE type &idStaticList<type,size>::operator[]( int index ) {
	assert( index >= 0 );
	assert( index < num );

	return list[ index ];
}

/*
================
idStaticList<type,size>::Alloc

Returns a pointer to a new data element at the end of the list.
================
*/
template<class type,int size>
ID_INLINE type *idStaticList<type,size>::Alloc( void ) {
	if ( num >= size ) {
		return NULL;
	}

	return &list[ num++ ];
}

/*
================
idStaticList<type,size>::Append

Increases the size of the list by one element and copies the supplied data into it.

Returns the index of the new element, or -1 when list is full.
================
*/
template<class type,int size>
ID_INLINE int idStaticList<type,size>::Append( type const & obj ) {
	assert( num < size );
	if ( num < size ) {
		list[ num ] = obj;
		num++;
		return num - 1;
	}

	return -1;
}


/*
================
idStaticList<type,size>::Insert

Increases the size of the list by at leat one element if necessary 
and inserts the supplied data into it.

Returns the index of the new element, or -1 when list is full.
================
*/
template<class type,int size>
ID_INLINE int idStaticList<type,size>::Insert( type const & obj, int index ) {
	int i;

	assert( num < size );
	if ( num >= size ) {
		return -1;
	}

	assert( index >= 0 );
	if ( index < 0 ) {
		index = 0;
	} else if ( index > num ) {
		index = num;
	}

	for( i = num; i > index; --i ) {
		list[i] = list[i-1];
	}

	num++;
	list[index] = obj;
	return index;
}

/*
================
idStaticList<type,size>::Append

adds the other list to this one

Returns the size of the new combined list
================
*/
template<class type,int size>
ID_INLINE int idStaticList<type,size>::Append( const idStaticList<type,size> &other ) {
	int i;
	int n = other.Num();

	if ( num + n > size ) {
		n = size - num;
	}
	for( i = 0; i < n; i++ ) {
		list[i + num] = other.list[i];
	}
	num += n;
	return Num();
}

/*
================
idStaticList<type,size>::AddUnique

Adds the data to the list if it doesn't already exist.  Returns the index of the data in the list.
================
*/
template<class type,int size>
ID_INLINE int idStaticList<type,size>::AddUnique( type const & obj ) {
	int index;

	index = FindIndex( obj );
	if ( index < 0 ) {
		index = Append( obj );
	}

	return index;
}

/*
================
idStaticList<type,size>::FindIndex

Searches for the specified data in the list and returns it's index.  Returns -1 if the data is not found.
================
*/
template<class type,int size>
ID_INLINE int idStaticList<type,size>::FindIndex( type const & obj ) const {
	int i;

	for( i = 0; i < num; i++ ) {
		if ( list[ i ] == obj ) {
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
idStaticList<type,size>::Find

Searches for the specified data in the list and returns it's address. Returns NULL if the data is not found.
================
*/
template<class type,int size>
ID_INLINE const type *idStaticList<type,size>::FindElement( type const & obj ) const {
	int i;

	i = FindIndex( obj );
	if ( i >= 0 ) {
		return &list[ i ];
	}

	return NULL;
}

/*
================
idStaticList<type,size>::FindNull

Searches for a NULL pointer in the list.  Returns -1 if NULL is not found.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template<class type,int size>
ID_INLINE int idStaticList<type,size>::FindNull( void ) const {
	int i;

	for( i = 0; i < num; i++ ) {
		if ( list[ i ] == NULL ) {
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
idStaticList<type,size>::RemoveIndex

Removes the element at the specified index and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the index is outside the bounds of the list.
Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template<class type,int size>
ID_INLINE bool idStaticList<type,size>::RemoveIndex( int index ) {
	int i;

	assert( index >= 0 );
	assert( index < num );

	if ( ( index < 0 ) || ( index >= num ) ) {
		return false;
	}

	num--;
	for( i = index; i < num; i++ ) {
		list[ i ] = list[ i + 1 ];
	}

	return true;
}

/*
===============
idStaticList<type, size>::RemoveIndexFast

Removes the element at the specified index and moves the last element into
it's spot, rather than moving the whole array down by one.  Of course, this 
doesn't maintain the order of elements!
The number of elements in the list is reduced by one.  Returns false if the 
index is outside the bounds of the list. Note that the element is not destroyed, 
so any memory used by it may not be freed until the destruction of the list.
===============
*/
template< class type, int size >
ID_INLINE bool idStaticList< type, size >::RemoveIndexFast( int index ) {
	assert( index >= 0 );
	assert( index < num );

	if ( ( index < 0 ) || ( index >= num ) ) {
		return false;
	}	

	num--;

	// nothing to do
	if( index == num )  {
		return true;
	}
	list[ index ] = list[ num ];

	return true;
}

/*
================
idStaticList<type,size>::Remove

Removes the element if it is found within the list and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the data is not found in the list.  Note that
the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template<class type,int size>
ID_INLINE bool idStaticList<type,size>::Remove( type const & obj ) {
	int index;

	index = FindIndex( obj );
	if ( index >= 0 ) {
		return RemoveIndex( index );
	}
	
	return false;
}

/*
================
idStaticList<type, size>::RemoveFast

Removes the element if it is found within the list and moves the last element into the gap.
The number of elements in the list is reduced by one.  Returns false if the data is not found in the list.  Note that
the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template< class type, int size >
ID_INLINE bool idStaticList<type, size>::RemoveFast( type const & obj ) {
	int index;

	index = FindIndex( obj );
	if ( index >= 0 ) {
		return RemoveIndexFast( index );
	}
	
	return false;
}

/*
================
idStaticList<type,size>::Swap

Swaps the contents of two lists
================
*/
template<class type,int size>
ID_INLINE void idStaticList<type,size>::Swap( idStaticList<type,size> &other ) {
	idStaticList<type,size> temp = *this;
	*this = other;
	other = temp;
}

/*
================
idStaticList<type, size>::Begin

Returns the first element of the list
================
*/
template< class type, int size >
ID_INLINE typename idStaticList<type, size>::Iterator idStaticList<type, size>::Begin( void ) {
	if( num == 0 ) {
		return End();
	}
	return list;
}

/*
================
idStaticList<type, size>::End

Returns one past the end of the list
================
*/
template< class type, int size >
ID_INLINE typename idStaticList<type, size>::Iterator idStaticList<type, size>::End( void ) {
	return list + num;
}

/*
================
idStaticList<type, size>::Begin

Returns the first element of the list
================
*/
template< class type, int size >
ID_INLINE typename idStaticList<type, size>::ConstIterator idStaticList<type, size>::Begin( void ) const {
	if( num == 0 ) {
		return End();
	}
	return list;
}

/*
================
idStaticList<type, size>::End

Returns one past the end of the list
================
*/
template< class type, int size >
ID_INLINE typename idStaticList<type, size>::ConstIterator idStaticList<type, size>::End( void ) const {
	return list + num;
}

/*
================
idStaticList<type, size>::Back

returns the last element
================
*/
template< class type, int size >
ID_INLINE type& idStaticList<type, size>::Back() {
	assert( num != 0 );
	return list[ num - 1 ];
}

/*
================
idStaticList<type, size>::Back

returns the last element
================
*/
template< class type, int size >
ID_INLINE const type& idStaticList<type, size>::Back() const {
	assert( num != 0 );
	return list[ num - 1 ];
}

/*
================
idStaticList<type, size>::Front

returns the first element
================
*/
template< class type, int size >
ID_INLINE type& idStaticList<type, size>::Front() {
	assert( num != 0 );
	return list[ 0 ];
}

/*
================
idStaticList<type, size>::Front

returns the first element
================
*/
template< class type, int size >
ID_INLINE const type& idStaticList<type, size>::Front() const {
	assert( num != 0 );
	return list[ 0 ];
}

/*
================
idStaticList<type, size>::Remove
================
*/
template< class type, int size >
ID_INLINE bool idStaticList<type, size>::Remove( Iterator iter ) {
	int index = iter - list;
	return RemoveIndex( index );
}

/*
================
idStaticList<type>::Sort

Performs a qsort on the list using the supplied comparison function.  Note that the data is merely moved around the
list, so any pointers to data within the list may no longer be valid.
================
*/
template<class type,int size>
ID_INLINE void idStaticList<type,size>::Sort( cmp_t *compare ) {
	if ( !list ) {
		return;
	}
	typedef int cmp_c(const void *, const void *);

	cmp_c *vCompare = (cmp_c *)compare;
	qsort( ( void * )list, ( size_t )num, sizeof( type ), vCompare );
}

#endif /* !__STATICLIST_H__ */
