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

#ifndef __LIST_H__
#define __LIST_H__

#include <initializer_list>

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
#ifdef __INTEL_COMPILER
// the intel compiler doesn't do the right thing here
template< class type >
ID_INLINE int idListSortCompare( const type *a, const type *b ) {
	assert( 0 );
	return 0;
}
#else
template< class type >
ID_INLINE int idListSortCompare( const type *a, const type *b ) {
	return *a - *b;
}
#endif

/*
================
idListNewElement<type>
================
*/
template< class type >
ID_INLINE type *idListNewElement( void ) {
	return new type;
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

template< class type >
class idList {
public:

	typedef int		cmp_t( const type *, const type * );
	typedef type	new_t( void );

					idList( int newgranularity = 16 );
					idList( const idList<type> &other );
					idList( const std::initializer_list<type> &other );
					~idList( void );

	void			Clear( void );										// clear the list
	void			ClearFree( void );									// clear the list and delete buffer
	int				Num( void ) const;									// returns number of elements in list
	int				NumAllocated( void ) const;							// returns number of elements allocated for
	void			SetGranularity( int newgranularity );				// set new granularity
	int				GetGranularity( void ) const;						// get the current granularity

	size_t			Allocated( void ) const;							// returns total size of allocated memory
	size_t			Size( void ) const;									// returns total size of allocated memory including size of list type
	size_t			MemoryUsed( void ) const;							// returns size of the used elements in the list
	void			FillZero( void );									// memset elements with 0

	idList<type> &	operator=( const idList<type> &other );
	const type &	operator[]( int index ) const;
	type &			operator[]( int index );
	const type &	Last() const;
	type &			Last();

	void			Condense( void );									// resizes list to exactly the number of elements it contains
	void			Resize( int newsize );								// resizes list to the given number of elements
	void			Resize( int newsize, int newgranularity	 );			// resizes list and sets new granularity
	void			SetNum( int newnum, bool resize = true );			// set number of elements in list and resize to exactly this number if necessary
	void			AssureSize( int newSize);							// assure list has given number of elements, but leave them uninitialized
	void			AssureSize( int newSize, const type &initValue );	// assure list has given number of elements and initialize any new elements
	void			AssureSizeAlloc( int newSize, new_t *allocator );	// assure the pointer list has the given number of elements and allocate any new elements
	void			Reserve( int newSize );								// resize list to newSize if it is smaller, don't change Num

	type *			Ptr( void );										// returns a pointer to the list
	const type *	Ptr( void ) const;									// returns a pointer to the list
	type &			Alloc( void );										// returns reference to a new data element at the end of the list
	int				Append( const type & obj );							// append element
	int				Append( const idList<type> &other );				// append list
	int				AddGrow( type obj );								// append with exponential growth (like std::vector::push_back)
	int				AddUnique( const type & obj );						// add unique element
	int				Insert( const type & obj, int index = 0 );			// insert the element at the given index
	int				FindIndex( const type & obj ) const;				// find the index for the given element
	type *			Find( type const & obj ) const;						// find pointer to the given element
	int				FindNull( void ) const;								// find the index for the first NULL pointer in the list
	int				IndexOf( const type *obj ) const;					// returns the index for the pointer to an element in the list
	bool			RemoveIndex( const int index );						// remove the element at the given index and keep items sorted
	bool			RemoveIndex( const int index, bool keepSorted );	// Tels: remove the element at the given index, keep sorted only if wanted
	bool			Remove( const type & obj );							// remove the element
	type			Pop();												// stgatilov: remove and return last element
	void			Sort( cmp_t *compare = ( cmp_t * )&idListSortCompare<type> );
	void			SortSubSection( int startIndex, int endIndex, cmp_t *compare = ( cmp_t * )&idListSortCompare<type> );
	void			Reverse();											// stgatilov: reverse order of elements
	void			Swap( idList<type> &other );						// swap the contents of the lists
	void			DeleteContents( bool clear = true );				// delete the contents of the list

	//stgatilov: for "range-based for" from C++11
	type *			begin();
	const type *	begin() const;
	type *			end();
	const type *	end() const;

private:
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
template< class type >
ID_INLINE idList<type>::idList( int newgranularity ) {
	assert( newgranularity > 0 );

	list		= NULL;
	num			= 0;
	size		= 0;
	granularity	= newgranularity;
}

/*
================
idList<type>::idList( const idList<type> &other )
================
*/
template< class type >
ID_INLINE idList<type>::idList( const idList<type> &other ) {
	list = NULL;
	*this = other;
}

/*
================
idList<type>::idList( const std::initializer_list<type> &other )
================
*/
template< class type >
ID_INLINE idList<type>::idList( const std::initializer_list<type> &other ) : idList() {
	Resize( other.size() );
	auto x = other.begin();
	while ( x != other.end() )
		Append( *x++ );
}

/*
================
idList<type>::~idList<type>
================
*/
template< class type >
ID_INLINE idList<type>::~idList( void ) {
	ClearFree();
}

/*
================
idList<type>::Clear

//stgatilov #5593: Removes all elements from the list without freeing memory.
================
*/
template< class type >
ID_INLINE void idList<type>::Clear( void ) {
	num		= 0;
}

/*
================
idList<type>::ClearFree

Frees up the memory allocated by the list.  Assumes that type automatically handles freeing up memory.
================
*/
template< class type >
ID_INLINE void idList<type>::ClearFree( void ) {
	if ( list ) {
		delete[] list;
	}

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
	int i;

	for( i = 0; i < num; i++ ) {
		delete list[ i ];
		list[ i ] = NULL;
	}

	if ( clear ) {
		ClearFree();
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
idList<type>::FillZero
================
*/
template< class type >
ID_INLINE void idList<type>::FillZero( void ) {
	memset( list, 0, num * sizeof(type) );
}

/*
================
idList<type>::Num

Returns the number of elements currently contained in the list.
Note that this is NOT an indication of the memory allocated.
================
*/
template< class type >
ID_FORCE_INLINE int idList<type>::Num( void ) const {
	return num;
}

/*
================
idList<type>::NumAllocated

Returns the number of elements currently allocated for.
================
*/
template< class type >
ID_FORCE_INLINE int idList<type>::NumAllocated( void ) const {
	return size;
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
ID_FORCE_INLINE int idList<type>::GetGranularity( void ) const {
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
			ClearFree();
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
	type	*temp;
	int		i;

	assert( newsize >= 0 );

	// free up the list if no data is being reserved
	if ( newsize <= 0 ) {
		ClearFree();
		return;
	}

	if ( newsize == size ) {
		// not changing the size, so just exit
		return;
	}

	temp	= list;
	size	= newsize;
	if ( size < num ) {
		num = size;
	}

	// copy the old list into our new one
	list = new type[ size ];
	for( i = 0; i < num; i++ ) {
		list[ i ] = temp[ i ];
	}

	// delete the old list if it exists
	if ( temp ) {
		delete[] temp;
	}
}

/*
================
idList<type>::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correnctly instantiated.
================
*/
template< class type >
ID_INLINE void idList<type>::Resize( int newsize, int newgranularity ) {
	type	*temp;
	int		i;

	assert( newsize >= 0 );

	assert( newgranularity > 0 );
	granularity = newgranularity;

	// free up the list if no data is being reserved
	if ( newsize <= 0 ) {
		ClearFree();
		return;
	}

	temp	= list;
	size	= newsize;
	if ( size < num ) {
		num = size;
	}

	// copy the old list into our new one
	list = new type[ size ];
	for( i = 0; i < num; i++ ) {
		list[ i ] = temp[ i ];
	}

	// delete the old list if it exists
	if ( temp ) {
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
idList<type>::AssureSizeAlloc

Makes sure the list has at least the given number of elements and allocates any elements using the allocator.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template< class type >
ID_INLINE void idList<type>::AssureSizeAlloc( int newSize, new_t *allocator ) {
	int newNum = newSize;

	if ( newSize > size ) {

		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		num = size;
		Resize( newSize );

		for ( int i = num; i < newSize; i++ ) {
			list[i] = (*allocator)();
		}
	}

	num = newNum;
}

/*
================
idList<type>::Reserve

Makes sure the list capacity can hold at least the given number of elements.
================
*/
template< class type >
ID_INLINE void idList<type>::Reserve( int newSize ) {
	if (newSize > size) {
		int tnum = num;
		AssureSize( newSize );
		num = tnum;
	}
}

/*
================
idList<type>::operator=

Copies the contents and size attributes of another list.
================
*/
template< class type >
ID_INLINE idList<type> &idList<type>::operator=( const idList<type> &other ) {
	if ( this == &other)
		return *this;

	ClearFree();

	num			= other.num;
	size		= other.size;
	granularity	= other.granularity;

	if ( size ) {
		list = new type[ size ];
		for( int i = 0; i < num; i++ ) {
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
ID_FORCE_INLINE const type &idList<type>::operator[]( int index ) const {
	assert( unsigned(index) < unsigned(num) );

	return list[ index ];
}

/*
================
idList<type>::operator[]

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template< class type >
ID_FORCE_INLINE type &idList<type>::operator[]( int index ) {
	assert( unsigned(index) < unsigned(num) );

	return list[ index ];
}

/*
================
idList<type>::Last
================
*/
template< class type >
ID_FORCE_INLINE const type &idList<type>::Last() const {
	assert( num > 0 );
	return list[num - 1];
}

/*
================
idList<type>::Last
================
*/
template< class type >
ID_FORCE_INLINE type &idList<type>::Last() {
	assert( num > 0 );
	return list[num - 1];
}

/*
================
idList<type>::Ptr

Returns a pointer to the beginning of the array.  Useful for iterating through the list in loops.

Note: may return NULL if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template< class type >
ID_FORCE_INLINE type *idList<type>::Ptr( void ) {
	return list;
}

/*
================
idList<type>::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return NULL if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template< class type >
const ID_FORCE_INLINE type *idList<type>::Ptr( void ) const {
	return list;
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

greebo: Important: Don't do this: spotList.Append( spotList[randomLocation] )!

Don't call idList::Append() or idList::Alloc() with a reference to a current 
list element. The operator[] will return a reference to the memory BEFORE a
possible Resize() call, which will relocate the entire list. The reference
to the "old" memory location will be invalid and crashes are ahead.
================
*/
template< class type >
ID_INLINE int idList<type>::Append( type const & obj ) {
	if ( !list ) {
		Resize( granularity );
	}

	if ( num == size ) {
		int newsize;

		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
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
idList<type>::AddGrow

Increases the size of the list by one element and copies the supplied data into it.
Returns the index of the new element.

stgatilov: this method is different from Append, because it grows exponentially like std::vector.
This allows to grow to size N in O(N) time.
================
*/
template< class type >
ID_INLINE int idList<type>::AddGrow( type obj ) {
	if ( num == size ) {
		int newsize;

		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			granularity = 16;
		}
		newsize = (size * 3) >> 1;			// + 50% size
		newsize += granularity;				// round up to granularity
		newsize -= newsize % granularity;	//
		Resize( newsize );
	}

	list[ num ] = obj;
	num++;

	return num - 1;
}

/*
================
idList<type>::Insert

Increases the size of the list by at leat one element if necessary 
and inserts the supplied data into it.

Returns the index of the new element.
================
*/
template< class type >
ID_INLINE int idList<type>::Insert( type const & obj, int index ) {
	if ( !list ) {
		Resize( granularity );
	}

	if ( num == size ) {
		int newsize;

		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			granularity = 16;
		}
		newsize = size + granularity;
		Resize( newsize - newsize % granularity );
	}

	if ( index < 0 ) {
		index = 0;
	}
	else if ( index > num ) {
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

Returns the size of the new combined list
================
*/
template< class type >
ID_INLINE int idList<type>::Append( const idList<type> &other ) {

	// Tels: Old code, with quadratic (O(N*N) performance, it would call Resize
	// 	 every so often, which is a O(N) copy operation.
/*	if ( !list ) {
		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			granularity = 16;
		}
		Resize( granularity );
	}

	int n = other.Num();
	for (int i = 0; i < n; i++) {
		Append(other[i]);
	}

	return Num();
*/

	// Tels 2010-07-21: new code, resize only once, then copy data over O(N)
	if ( granularity == 0 ) {	// this is a hack to fix our memset classes
		granularity = 16;
	}
	int n = other.Num();
	int newsize = size + granularity + n;
	Resize( newsize - newsize % granularity );

	for (int i = 0; i < n; i++) {
		// simply copy data over
		list[num + i] = other[i];
	}
	num += n;
	return num;
}

/*
================
idList<type>::AddUnique

Adds the data to the list if it doesn't already exist.  Returns the index of the data in the list.
================
*/
template< class type >
ID_INLINE int idList<type>::AddUnique( type const & obj ) {
	int index;

	index = FindIndex( obj );
	if ( index < 0 ) {
		index = Append( obj );
	}

	return index;
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

Searches for the specified data in the list and returns it's address. Returns NULL if the data is not found.
================
*/
template< class type >
ID_INLINE type *idList<type>::Find( type const & obj ) const {
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
================
idList<type>::RemoveIndex

Removes the element at the specified index and if keepSorted is true, moves all data following the element down to
fill in the gap. If keepSorted is false, just fills the gap with the last element in the list (if any).
The number of elements in the list is reduced by one.  Returns false if the index is outside the bounds of the list.
Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template< class type >
ID_INLINE bool idList<type>::RemoveIndex( const int index, const bool keepSorted ) {

	assert( list != NULL );
	assert( index >= 0 );
	assert( index < num );

	if ( ( index < 0 ) || ( index >= num ) ) {
		return false;
	}

	num--;
	if (keepSorted)
	{
		//gameLocal.Printf("Moving %i list entries (index = %i)\n", num - index, index );
		for( int i = index; i < num; i++ ) {
			list[ i ] = list[ i + 1 ];
		}
	}
	else
	{
		// [1,2,3,4] (remove 2, num was 4, is now 3, index = 1) => (num = 2), [1,4,3]
		// if index == num, we removed the last element, so nothing to do
		if ( index < num )
		{
			list[ index ] = list[ num ];
		}
	}

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
idList<type>::Pop

Returns the last element of the list (by value), removing it at the same time.
================
*/
template< class type >
ID_INLINE type idList<type>::Pop( ) {
	assert(num >= 0);
	return list[--num];
}

/*
================
idList<type>::Sort

Performs a qsort on the list using the supplied comparison function.  Note that the data is merely moved around the
list, so any pointers to data within the list may no longer be valid.
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
idList<type>::SortSubSection

Sorts a subsection of the list.
================
*/
template< class type >
ID_INLINE void idList<type>::SortSubSection( int startIndex, int endIndex, cmp_t *compare ) {
	if ( !list ) {
		return;
	}
	if ( startIndex < 0 ) {
		startIndex = 0;
	}
	if ( endIndex >= num ) {
		endIndex = num - 1;
	}
	if ( startIndex >= endIndex ) {
		return;
	}
	typedef int cmp_c(const void *, const void *);

	cmp_c *vCompare = (cmp_c *)compare;
	qsort( ( void * )( &list[startIndex] ), ( size_t )( endIndex - startIndex + 1 ), sizeof( type ), vCompare );
}

template< class type >
ID_INLINE void idList<type>::Reverse() {
	int k = (num >> 1);
	for (int i = 0; i < k; i++)
		idSwap(list[i], list[num-1-i]);
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


template< class type >
ID_FORCE_INLINE type * idList<type>::begin() {
	return list;
}
template< class type >
ID_FORCE_INLINE const type * idList<type>::begin() const {
	return list;
}
template< class type >
ID_FORCE_INLINE type * idList<type>::end() {
	return list + num;
}
template< class type >
ID_FORCE_INLINE const type * idList<type>::end() const {
	return list + num;
}


typedef idList<idStr> idStringList;

#endif /* !__LIST_H__ */
