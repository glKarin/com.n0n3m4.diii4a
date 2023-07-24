// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __LIST_H__
#define __LIST_H__

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#endif

/*
===============================================================================

	List template
	Does not allocate memory until the first item is added.

===============================================================================
*/


/*
================
idListSortCompare<type>
================
*/
template< class type >
ID_INLINE int idListSortCompare( const type *a, const type *b ) {
	return *a - *b;
}

/*
================
idSwap<type>
================
*/
template< class type >
ID_INLINE void idSwap( type &a, type &b ) {
	type c = a;
	a = b;
	b = c;
}

/*
============
idList
============
*/
template< class type>
class idList {
public:
	typedef const type		ConstType;
	typedef type			Type;
	typedef Type*			Iterator;
	typedef ConstType*		ConstIterator;
	typedef int				cmp_t( const type *, const type * );

	static const int		DEFAULT_GRANULARITY = 16;
	

	explicit		idList( int newgranularity = DEFAULT_GRANULARITY );
					idList( const idList &other );
	
	template< class Iter >	
	explicit		idList( Iter begin, Iter end ) :
						list( NULL ),
						granularity( DEFAULT_GRANULARITY ) {
						Clear();
						CopyFromRange( begin, end );
					}

					~idList( void );

	void			Clear( void );										// clear the list
	int				Num( void ) const;									// returns number of elements in list
	bool			Empty( void ) const;								// returns true if number elements in list is 0
	void			SetGranularity( int newgranularity );				// set new granularity
	int				GetGranularity( void ) const;						// get the current granularity

	size_t			Allocated( void ) const;							// returns total size of allocated memory
	size_t			Size( void ) const;									// returns total size of allocated memory including size of list type
	size_t			MemoryUsed( void ) const;							// returns size of the used elements in the list

	idList&			operator=( const idList &other );
	type &			operator[]( int index );
	const type &	operator[]( int index ) const;	

	void			Condense( void );									// resizes list to exactly the number of elements it contains
	void			Resize( int newsize );								// resizes list to the given number of elements
	void			Resize( int newsize, int newgranularity );			// resizes list and sets new granularity
	void			SetNum( int newnum, bool resize = true );			// set number of elements in list and resize to exactly this number if necessary
	void			AssureSize( int newSize );							// assure list has given number of elements, but leave them uninitialized
	void			AssureSize( int newSize, const type &initValue );	// assure list has given number of elements and initialize any new elements
	void			PreAllocate( int newSize );

	type &			Alloc( void );										// returns reference to a new data element at the end of the list
	int				Append( const type & obj );							// append element
	int				Append( const idList &other );						// append list
	int				AddUnique( const type & obj );						// add unique element
	int				Insert( const type & obj, int index = 0 );			// insert the element at the given index	
	
	int				FindIndex( const type & obj ) const;				// find the index for the given element	
	int				FindIndexBinary ( const type & key, cmp_t *compare = ( cmp_t * )&idListSortCompare<type> ) const;
	type *			FindElement( const type & obj ) const;				// find pointer to the given element
	int				FindNull( void ) const;								// find the index for the first NULL pointer in the list

	void			Fill( int num, const type & obj );					// resize the list, if neccessary and set all items to obj
	
	int				IndexOf( const type *obj ) const;					// returns the index for the pointer to an element in the list
	bool			RemoveIndex( int index );							// remove the element at the given index
	bool			RemoveIndexFast( int index );						// remove the element at the given index and put the last element into its spot
	bool			Remove( const type & obj );							// remove the element
	bool			RemoveFast( const type & obj );						// remove the element, move the last element into its spot
	void			Sort( cmp_t *compare = idListSortCompare<type> );	// sort the list
	void			Swap( idList &other );								// swap the contents of the lists
	void			DeleteContents( bool clear );						// delete the contents of the list

	// members for compatibility with STL algorithms
	Iterator		Begin( void );										// return list[ 0 ]
	Iterator		End( void );										// return list[ num ]	(one past end)

	ConstIterator	Begin( void ) const;								// return list[ 0 ]
	ConstIterator	End( void ) const;									// return list[ num ]	(one past end)

	type&			Front();											// return *list[ 0 ]
	const type&		Front() const;										// return *list[ 0 ]

	type&			Back();												// return *list[ num - 1 ]
	const type&		Back() const;										// return *list[ num - 1 ]

	bool			Remove( Iterator iter );

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
	template< class Cmp >
	Iterator		FindIteratorIf( const type& element, const Cmp predicate ) {
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
	template< class Cmp >
	ConstIterator	FindIteratorIf( const type& element, Cmp predicate ) const {
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
						AssureSize( end - begin );
						for( int i = 0; i < num; i++, ++begin ) {
							list[ i ] = *begin;
						}
					}
protected:
	int				num;
	int				size;
	int				granularity;
	type *			list;
};

/*
================
idList<type>::idList( int )
================
*/
template<class type>
ID_INLINE idList<type>::idList( int newgranularity )
	:	granularity( newgranularity ),
		list( NULL ) {
	assert( granularity > 0 );
	Clear();
}

/*
================
idList<type>::idList( const idList &other )
================
*/
template< class type >
ID_INLINE idList<type>::idList( const idList &other ) 
	:	list( NULL ) {
	*this = other;
}

/*
================
idList<type>::~idList<type>
================
*/
template< class type >
ID_INLINE idList<type>::~idList( void ) {
	Clear();
}

/*
================
idList<type>::Clear

Frees up the memory allocated by the list.  Assumes that type automatically handles freeing up memory.
================
*/
template< class type >
ID_INLINE void idList<type>::Clear( void ) {
    delete[] list;
	list	= NULL;
	num		= 0;
	size	= 0;
}

/*
================
idList<type>::DeleteContents

Calls the destructor of all elements in the list.  Conditionally frees up memory used by the list.
Note that this only works on lists containing pointers to objects and will cause a compiler error
if called with non-pointers.  Since the list was not responsible for allocating the object, it has
no information on whether the object still exists or not, so care must be taken to ensure that
the pointers are still valid when this function is called.  Function will set all pointers in the
list to NULL.
================
*/
template< class type >
ID_INLINE void idList<type>::DeleteContents( bool clear ) {
	for( int i = 0; i < num; i++ ) {
		delete list[ i ];
		list[ i ] = NULL;
	}

	if ( clear ) {
		Clear();
	} else {
		memset( list, 0, size * sizeof( type ) );
	}
}

/*
================
idList<type>::Allocated

return total memory allocated for the list in bytes, but doesn't take into account additional memory allocated by type
================
*/
template< class type >
ID_INLINE size_t idList<type>::Allocated( void ) const {
	return size * sizeof( type );
}

/*
================
idList<type>::Size

return total size of list in bytes, but doesn't take into account additional memory allocated by type
================
*/
template< class type >
ID_INLINE size_t idList<type>::Size( void ) const {
	return sizeof( idList<type> ) + Allocated();
}

/*
================
idList<type>::MemoryUsed
================
*/
template< class type >
ID_INLINE size_t idList<type>::MemoryUsed( void ) const {
	return num * sizeof( *list );
}

/*
================
idList<type>::Num

Returns the number of elements currently contained in the list.
Note that this is NOT an indication of the memory allocated.
================
*/
template< class type >
ID_INLINE int idList<type>::Num( void ) const {
	return num;
}

/*
================
idList<type>::SetNum

Resize to the exact size specified irregardless of granularity
================
*/
template< class type >
ID_INLINE void idList<type>::SetNum( int newnum, bool resize ) {
	assert( newnum >= 0 );
	if ( resize || newnum > size ) {
		Resize( newnum );
	}
	num = newnum;
}

/*
================
idList<type>::SetGranularity

Sets the base size of the array and resizes the array to match.
================
*/
template< class type >
ID_INLINE void idList<type>::SetGranularity( int newgranularity ) {
	int newsize;

	assert( newgranularity > 0 );
	granularity = newgranularity;

	if ( list ) {
		// resize it to the closest level of granularity
		newsize = num + granularity - 1;
		newsize -= newsize % granularity;
		if ( newsize != size ) {
			Resize( newsize );
		}
	}
}

/*
================
idList<type>::GetGranularity

Get the current granularity.
================
*/
template< class type >
ID_INLINE int idList<type>::GetGranularity( void ) const {
	return granularity;
}

/*
================
idList<type>::Condense

Resizes the array to exactly the number of elements it contains or frees up memory if empty.
================
*/
template< class type >
ID_INLINE void idList<type>::Condense( void ) {
	if ( list ) {
		if ( num ) {
			Resize( num );
		} else {
			Clear();
		}
	}
}

/*
================
idList<type>::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correctly instantiated.
================
*/
template< class type >
ID_INLINE void idList<type>::Resize( int newsize ) {
	assert( newsize >= 0 );

	// free up the list if no data is being reserved
	if ( newsize <= 0 ) {
		Clear();
		return;
	}

	if ( newsize == size ) {
		// not changing the size, so just exit
		return;
	}

	type* temp	= list;
	size		= newsize;
	if ( size < num ) {
		num = size;
	}

	list = new type[ size ];

	if ( temp ) {
		// copy the old list into our new one
		for( int i = 0; i < num; i++ ) {
			list[ i ] = temp[ i ];
		}

		// delete the old list if it exists
		delete[] temp;
	}
}

/*
================
idList<type>::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correctly instantiated.
================
*/
template< class type >
ID_INLINE void idList<type>::Resize( int newsize, int newgranularity ) {
	assert( newsize >= 0 );

	assert( newgranularity > 0 );
	granularity = newgranularity;

	// free up the list if no data is being reserved
	if ( newsize <= 0 ) {
		Clear();
		return;
	}

	if ( newsize == size ) {
		// not changing the size, so just exit
		return;
	}

	type* temp	= list;
	size		= newsize;

	if ( size < num ) {
		num = size;
	}

	// copy the old list into our new one
	list = new type[ size ];

	if ( temp ) {
		// copy the old list into our new one
		for( int i = 0; i < num; i++ ) {
			list[ i ] = temp[ i ];
		}

		// delete the old list if it exists
		delete[] temp;
	}
}

/*
================
idList<type>::AssureSize

Makes sure the list has at least the given number of elements.
================
*/
template< class type >
ID_INLINE void idList<type>::AssureSize( int newSize ) {
	int newNum = newSize;

	if ( newSize > size ) {

		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			assert( 0 );
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		Resize( newSize );
	}

	num = newNum;
}

/*
================
idList<type>::AssureSize

Makes sure the list has at least the given number of elements and initialize any elements not yet initialized.
================
*/
template< class type >
ID_INLINE void idList<type>::AssureSize( int newSize, const type &initValue ) {
	int newNum = newSize;

	if ( newSize > size ) {

		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			assert( 0 );
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		num = size;
		Resize( newSize );

		for ( int i = num; i < newSize; i++ ) {
			list[i] = initValue;
		}
	}
	num = newNum;
}

/*
================
idList< class type >::PreAllocate

Makes sure the list has at least the given number of elements
allocated but don't actually change the number of items.
================
*/
template< class type >
ID_INLINE void idList< type >::PreAllocate( int newSize ) {
	int newNum = newSize;

	if ( newSize > size ) {

		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		Resize( newSize );
	}
}

/*
================
idList<type>::operator=

Copies the contents and size attributes of another list.
================
*/
template< class type >
ID_INLINE idList<type> &idList<type>::operator=( const idList &other ) {
	if( &other == this ) {
		return *this;
	}

	int	i;

	Clear();

	num			= other.num;
	size		= other.size;
	granularity	= other.granularity;

	if ( size ) {
		list = new type[ size ];
		for( i = 0; i < num; i++ ) {
			list[ i ] = other.list[ i ];
		}
	}

	return *this;
}

/*
================
idList<type>::operator[] const

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template< class type >
ID_INLINE const type &idList<type>::operator[]( int index ) const {
	assert( index >= 0 );
	assert( index < num );

	return list[ index ];
}

/*
================
idList<type>::operator[]
	
Access operator.	 Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template< class type >
ID_INLINE type &idList<type>::operator[]( int index ) {
	assert( index >= 0 );
	assert( index < num );

	return list[ index ];
}

/*
================
idList<type>::Alloc

Returns a reference to a new data element at the end of the list.
================
*/
template< class type >
ID_INLINE type &idList<type>::Alloc( void ) {
	if ( !list ) {
		Resize( granularity );
	}

	if ( num == size ) {
		Resize( size + granularity );
	}

	return list[ num++ ];
}

/*
================
idList<type>::Append

Increases the size of the list by one element and copies the supplied data into it.

Returns the index of the new element.
================
*/
template< class type >
ID_INLINE int idList<type>::Append( type const & obj ) {

	// appending one of the list items does not work because the list may be reallocated
	assert( &obj < list || &obj >= list + num );

	if ( !list ) {
		Resize( granularity );
	}

	if ( num == size ) {
		int newsize;

		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			assert( 0 );
			granularity = 16;
		}
		newsize = size + granularity;
		Resize( newsize - newsize % granularity );
	}

	list[ num ] = obj;
	num++;

	return num - 1;
}


/*
================
idList<type>::Insert

Increases the size of the list by at least one element if necessary 
and inserts the supplied data into it.

Returns the index of the new element.
================
*/
template< class type >
ID_INLINE int idList<type>::Insert( type const & obj, int index ) {

	// inserting one of the list items does not work because the list may be reallocated
	assert( &obj < list || &obj >= list + num );

	if ( !list ) {
		Resize( granularity );
	}

	if ( num == size ) {
		int newsize;

		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			assert( 0 );
			granularity = 16;
		}
		newsize = size + granularity;
		Resize( newsize - newsize % granularity );
	}

	if ( index < 0 ) {
		index = 0;
	} else if ( index > num ) {
		index = num;
	}
	for ( int i = num; i > index; --i ) {
		list[i] = list[i-1];
	}
	num++;
	list[index] = obj;
	return index;
}

/*
================
idList<type>::Append

adds the other list to this one
correctly handles this->Append( *this );

Returns the size of the new combined list
================
*/
template< class type >
ID_INLINE int idList<type>::Append( const idList &other ) {

	// appending the list itself does not work because the list may be reallocated
	assert( &other != this );

	int n = other.Num();
	if ( !list ) {
		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			assert( 0 );
			granularity = 16;
		}
		Resize( granularity + n );
	} else {
		Resize( num + n );
	}	
	
	for ( int i = 0; i < n; i++ ) {
		Append( other[i] );
	}

	return Num();
}

/*
================
idList<type>::AddUnique

Adds the data to the list if it doesn't already exist.  Returns the index of the data in the list.
================
*/
template< class type >
ID_INLINE int idList<type>::AddUnique( type const & obj ) {

	// inserting one of the list items does not work because the list may be reallocated
	assert( &obj < list || &obj >= list + num );

	int index = FindIndex( obj );
	if ( index < 0 ) {
		index = Append( obj );
	}

	return index;
}

/*
================
idList<type>::FindIndexBinary
Assumes the list is sorted and does a binary search for the given key
================
*/
template< class type >
ID_INLINE int idList<type>::FindIndexBinary ( const type & key, cmp_t *compare ) const {

	typedef int cmp_c(const void *, const void *);
	cmp_c *vCompare = (cmp_c *)compare;

	type* found = (type*) bsearch( ( void * )( &key ), ( void * )list, ( size_t )num, sizeof( type ), vCompare );
	if (found) {
		return IndexOf(found);
	}
	return -1;
}

/*
================
idList<type>::FindIndex

Searches for the specified data in the list and returns it's index.  Returns -1 if the data is not found.
================
*/
template< class type >
ID_INLINE int idList<type>::FindIndex( type const & obj ) const {
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
idList<type>::Find

Searches for the specified data in the list and returns its address. Returns NULL if the data is not found.
================
*/
template< class type >
ID_INLINE type *idList<type>::FindElement( type const & obj ) const {
	int i;

	i = FindIndex( obj );
	if ( i >= 0 ) {
		return &list[ i ];
	}

	return NULL;
}

/*
================
idList<type>::FindNull

Searches for a NULL pointer in the list.  Returns -1 if NULL is not found.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template< class type >
ID_INLINE int idList<type>::FindNull( void ) const {
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
idList<type>::IndexOf

Takes a pointer to an element in the list and returns the index of the element.
This is NOT a guarantee that the object is really in the list. 
Function will assert in debug builds if pointer is outside the bounds of the list,
but remains silent in release builds.
================
*/
template< class type >
ID_INLINE int idList<type>::IndexOf( type const *objptr ) const {
	int index;

	index = objptr - list;

	assert( index >= 0 );
	assert( index < num );

	return index;
}

/*
================
idList<type>::RemoveIndex

Removes the element at the specified index and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the index is outside the bounds of the list.
Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template< class type >
ID_INLINE bool idList<type>::RemoveIndex( int index ) {
	int i;

	assert( list != NULL );
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
idList<type>::RemoveIndexFast

Removes the element at the specified index and moves the last element into
it's spot, rather than moving the whole array down by one.  Of course, this 
doesn't maintain the order of elements!
The number of elements in the list is reduced by one.  Returns false if the 
index is outside the bounds of the list. Note that the element is not destroyed, 
so any memory used by it may not be freed until the destruction of the list.
===============
*/
template< class type >
ID_INLINE bool idList< type >::RemoveIndexFast( int index ) {
	assert( list != NULL );
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
idList<type>::Remove

Removes the element if it is found within the list and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the data is not found in the list.  Note that
the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template< class type >
ID_INLINE bool idList<type>::Remove( type const & obj ) {
	int index;

	index = FindIndex( obj );
	if ( index >= 0 ) {
		return RemoveIndex( index );
	}
	
	return false;
}

/*
================
idList<type>::RemoveFast

Removes the element if it is found within the list and moves the last element into the gap.
The number of elements in the list is reduced by one.  Returns false if the data is not found in the list.  Note that
the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template< class type >
ID_INLINE bool idList<type>::RemoveFast( type const & obj ) {
	int index;

	index = FindIndex( obj );
	if ( index >= 0 ) {
		return RemoveIndexFast( index );
	}
	
	return false;
}

/*
================
idList<type>::Sort

Performs a qsort on the list using the supplied comparison function.  Note that the data is merely moved around the
list, so any pointers to data within the list may no longer be valid.
FIXME: These don't obey the element destruction policy
================
*/
template< class type >
ID_INLINE void idList<type>::Sort( cmp_t *compare ) {
	if ( !list ) {
		return;
	}
	typedef int cmp_c(const void *, const void *);

	cmp_c *vCompare = (cmp_c *)compare;
	qsort( ( void * )list, ( size_t )num, sizeof( type ), vCompare );
}

/*
================
idList<type>::Swap

Swaps the contents of two lists
================
*/
template< class type >
ID_INLINE void idList<type>::Swap( idList<type> &other ) {
	idSwap( num, other.num );
	idSwap( size, other.size );
	idSwap( granularity, other.granularity );
	idSwap( list, other.list );
}

/*
================
idList<type>::Begin

Returns the first element of the list
================
*/
template< class type >
ID_INLINE typename idList<type>::Iterator idList<type>::Begin( void ) {
	if( num == 0 ) {
		return End();
	}
	return list;
}

/*
================
idList<type>::End

Returns one past the end of the list
================
*/
template< class type >
ID_INLINE typename idList<type>::Iterator idList<type>::End( void ) {
	return list + num;
}

/*
================
idList<type>::Begin

Returns the first element of the list
================
*/
template< class type >
ID_INLINE typename idList<type>::ConstIterator idList<type>::Begin( void ) const {
	if( num == 0 ) {
		return End();
	}
	return list;
}

/*
================
idList<type>::End

Returns one past the end of the list
================
*/
template< class type >
ID_INLINE typename idList<type>::ConstIterator idList<type>::End( void ) const {
	return list + num;
}

/*
================
idList<type>::Back

returns the last element
================
*/
template< class type >
ID_INLINE type& idList<type>::Back() {
	assert( num != 0 );
	return list[ num - 1 ];
}

/*
================
idList<type>::Back

returns the last element
================
*/
template< class type >
ID_INLINE const type& idList<type>::Back() const {
	assert( num != 0 );
	return list[ num - 1 ];
}

/*
================
idList<type>::Front

returns the first element
================
*/
template< class type >
ID_INLINE type& idList<type>::Front() {
	assert( num != 0 );
	return list[ 0 ];
}

/*
================
idList<type>::Front

returns the first element
================
*/
template< class type >
ID_INLINE const type& idList<type>::Front() const {
	assert( num != 0 );
	return list[ 0 ];
}

/*
================
idList<type>::Remove
================
*/
template< class type >
ID_INLINE bool idList<type>::Remove( Iterator iter ) {
	int index = iter - list;
	return RemoveIndex( index );
}

/*
================
idList<type>::Empty
================
*/
template< class type >
ID_INLINE bool idList<type>::Empty() const {
	return num == 0 || list == NULL;
}

/*
============
idList<type>::Fill
============
*/
template< class type >
ID_INLINE void idList<type>::Fill( int num, const type & obj ) {

	// inserting one of the list items does not work because the list may be reallocated
	assert( &obj < list || &obj >= list + num );

	AssureSize( num );
	for( int i = 0; i < num; i++ ) {
		list[ i ] = obj;
	}
}

template< class type >
class idListGranularityOne : public idList<type> {
public:
	idListGranularityOne( int gran = 1 ) : idList<type>( gran ) {
	}
};

#endif /* !__LIST_H__ */
