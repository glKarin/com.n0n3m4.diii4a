/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma  once

#ifdef _DEBUG
#define STL_NODEBUG
#endif

#ifdef STL_NODEBUG
#pragma optimize( "", on )
#endif

#ifndef FLT_EPSILON
#define FLT_EPSILON 1.19209290E-07F
#endif

#include <vector>

#ifdef STL_NODEBUG
#pragma optimize( "", off )
#endif


/*
===============================================================================

	List template
	Does not allocate memory until the first item is added.

===============================================================================
*/

// jmarshall
// idList does a lot of unnnessary memory reallocations that cause huge performance problems,
// to fix that I've modified rvmListSTL to use std::vector as a backend which should help with performance.
// jmarshall end


/*
================
rvmListSTLSortCompare<type>
================
*/
#ifdef __INTEL_COMPILER
// the intel compiler doesn't do the right thing here
template< class type >
ID_INLINE int rvmListSTLSortCompare(const type *a, const type *b) {
	assert(0);
	return 0;
}
#else
template< class type >
ID_INLINE int rvmListSTLSortCompare(const type *a, const type *b) {
	return *a - *b;
}
#endif

/*
================
rvmListSTLNewElement<type>
================
*/
template< class type >
ID_INLINE type *rvmListSTLNewElement(void) {
	return new type;
}

template< class type >
class rvmListSTL {
public:

	typedef int		cmp_t(const type *, const type *);
	typedef type	new_t(void);

	rvmListSTL(int newgranularity = 16);
	rvmListSTL(const rvmListSTL<type> &other);
	~rvmListSTL<type>(void);

	void			Clear(bool purge = false);										// clear the list
	int				Num(void) const;									// returns number of elements in list
	int				NumAllocated(void) const;							// returns number of elements allocated for
	void			SetGranularity(int newgranularity);				// set new granularity
	int				GetGranularity(void) const;						// get the current granularity

	size_t			Allocated(void) const;							// returns total size of allocated memory
	size_t			Size(void) const;									// returns total size of allocated memory including size of list type
	size_t			MemoryUsed(void) const;							// returns size of the used elements in the list

	rvmListSTL<type> &	operator=(const rvmListSTL<type> &other);
	const type &	operator[](int index) const;
	type &			operator[](int index);

	void			Condense(void);									// resizes list to exactly the number of elements it contains
	void			Resize(int newsize);								// resizes list to the given number of elements
	void			Resize(int newsize, int newgranularity);			// resizes list and sets new granularity
	void			SetNum(int newnum, bool resize = true);			// set number of elements in list and resize to exactly this number if necessary
	void			AssureSize(int newSize);							// assure list has given number of elements, but leave them uninitialized
	void			AssureSize(int newSize, const type &initValue);	// assure list has given number of elements and initialize any new elements
	void			AssureSizeAlloc(int newSize, new_t *allocator);	// assure the pointer list has the given number of elements and allocate any new elements

	type *			Ptr(void);										// returns a pointer to the list
	const type *	Ptr(void) const;									// returns a pointer to the list
	type &			Alloc(void);										// returns reference to a new data element at the end of the list
	int				Append(const type & obj);							// append element
	int				Append(const rvmListSTL<type> &other);				// append list
	int				AddUnique(const type & obj);						// add unique element
	int				Insert(const type & obj, int index = 0);			// insert the element at the given index
	int				FindIndex(const type & obj) const;				// find the index for the given element
	type *			Find(type const & obj) const;						// find pointer to the given element
	int				FindNull(void) const;								// find the index for the first NULL pointer in the list
	int				IndexOf(const type *obj) const;					// returns the index for the pointer to an element in the list
	bool			RemoveIndex(int index);							// remove the element at the given index
	bool			Remove(const type & obj);							// remove the element
	void			Sort(cmp_t *compare = (cmp_t *)&rvmListSTLSortCompare<type>);
	void			SortSubSection(int startIndex, int endIndex, cmp_t *compare = (cmp_t *)&rvmListSTLSortCompare<type>);
	void			Swap(rvmListSTL<type> &other);						// swap the contents of the lists
	void			DeleteContents(bool clear);						// delete the contents of the list

private:
	std::vector<type>			list;
};

/*
================
rvmListSTL<type>::rvmListSTL( int )
================
*/
template< class type >
ID_INLINE rvmListSTL<type>::rvmListSTL(int newgranularity) {
	assert(newgranularity > 0);
	Clear();
}

/*
================
rvmListSTL<type>::rvmListSTL( const rvmListSTL<type> &other )
================
*/
template< class type >
ID_INLINE rvmListSTL<type>::rvmListSTL(const rvmListSTL<type> &other) {
	*this = other;
}

/*
================
rvmListSTL<type>::~rvmListSTL<type>
================
*/
template< class type >
ID_INLINE rvmListSTL<type>::~rvmListSTL(void) {
	Clear();
}

/*
================
rvmListSTL<type>::Clear

Frees up the memory allocated by the list.  Assumes that type automatically handles freeing up memory.
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::Clear(bool purge) {
	list.clear();

	if (purge)
	{
		list.shrink_to_fit();
		//k list.resize(list.size());
	}
}

/*
================
rvmListSTL<type>::DeleteContents

Calls the destructor of all elements in the list.  Conditionally frees up memory used by the list.
Note that this only works on lists containing pointers to objects and will cause a compiler error
if called with non-pointers.  Since the list was not responsible for allocating the object, it has
no information on whether the object still exists or not, so care must be taken to ensure that
the pointers are still valid when this function is called.  Function will set all pointers in the
list to NULL.
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::DeleteContents(bool clear) {
	int i;

	for (i = 0; i < list.size(); i++) {
		delete list[i];
		list[i] = NULL;
	}

	Clear(clear);
}

/*
================
rvmListSTL<type>::Allocated

return total memory allocated for the list in bytes, but doesn't take into account additional memory allocated by type
================
*/
template< class type >
ID_INLINE size_t rvmListSTL<type>::Allocated(void) const {
	return list.size() * sizeof(type);
}

/*
================
rvmListSTL<type>::Size

return total size of list in bytes, but doesn't take into account additional memory allocated by type
================
*/
template< class type >
ID_INLINE size_t rvmListSTL<type>::Size(void) const {
	return sizeof(rvmListSTL<type>) + Allocated();
}

/*
================
rvmListSTL<type>::MemoryUsed
================
*/
template< class type >
ID_INLINE size_t rvmListSTL<type>::MemoryUsed(void) const {
	return list.size() * sizeof(type);
}

/*
================
rvmListSTL<type>::Num

Returns the number of elements currently contained in the list.
Note that this is NOT an indication of the memory allocated.
================
*/
template< class type >
ID_INLINE int rvmListSTL<type>::Num(void) const {
	return list.size();
}

/*
================
rvmListSTL<type>::NumAllocated

Returns the number of elements currently allocated for.
================
*/
template< class type >
ID_INLINE int rvmListSTL<type>::NumAllocated(void) const {
	return list.capacity();
}

/*
================
rvmListSTL<type>::SetNum

Resize to the exact size specified irregardless of granularity
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::SetNum(int newnum, bool resize) {
	assert(newnum >= 0);
	if (newnum == 0)
	{
		Clear(false);
		return;
	}

	list.resize(newnum);
}

/*
================
rvmListSTL<type>::SetGranularity

Sets the base size of the array and resizes the array to match.
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::SetGranularity(int newgranularity) {

}

/*
================
rvmListSTL<type>::GetGranularity

Get the current granularity.
================
*/
template< class type >
ID_INLINE int rvmListSTL<type>::GetGranularity(void) const {
	return 1;
}

/*
================
rvmListSTL<type>::Condense

Resizes the array to exactly the number of elements it contains or frees up memory if empty.
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::Condense(void) {
	//if (list.size() > 0) {
	//	if (num) {
	//		Resize(num);
	//	}
	//	else {
	//		Clear();
	//	}
	//}
}

/*
================
rvmListSTL<type>::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correnctly instantiated.
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::Resize(int newsize) {
	list.reserve(newsize);
}

/*
================
rvmListSTL<type>::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correnctly instantiated.
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::Resize(int newsize, int newgranularity) {
	list.reserve(newsize);
}

/*
================
rvmListSTL<type>::AssureSize

Makes sure the list has at least the given number of elements.
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::AssureSize(int newSize) {
	if (newSize == 0)
	{
		Clear(false);
		return;
	}

	list.resize(newSize);
}

/*
================
rvmListSTL<type>::AssureSize

Makes sure the list has at least the given number of elements and initialize any elements not yet initialized.
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::AssureSize(int newSize, const type &initValue) {
	if (newSize == 0)
	{
		Clear(false);
		return;
	}

	list.resize(newSize);
}

/*
================
rvmListSTL<type>::AssureSizeAlloc

Makes sure the list has at least the given number of elements and allocates any elements using the allocator.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::AssureSizeAlloc(int newSize, new_t *allocator) {
	int currentSize = list.size();

	if (newSize > list.size()) {
		list.resize(newSize);
		for (int i = currentSize; i < newSize; i++) {
			list[i] = (*allocator)();
		}
	}
}

/*
================
rvmListSTL<type>::operator=

Copies the contents and size attributes of another list.
================
*/
template< class type >
ID_INLINE rvmListSTL<type> &rvmListSTL<type>::operator=(const rvmListSTL<type> &other) {
	int	i;

	Clear();

	//	num			= other.num;
	//	size		= other.size;
		//granularity	= other.granularity;
	list.resize(other.list.size());
	std::copy(other.list.begin(), other.list.end(), list.begin());

	return *this;
}

/*
================
rvmListSTL<type>::operator[] const

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template< class type >
ID_INLINE const type &rvmListSTL<type>::operator[](int index) const {
	assert(index >= 0);

	return list[index];
}

/*
================
rvmListSTL<type>::operator[]

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template< class type >
ID_INLINE type &rvmListSTL<type>::operator[](int index) {
	assert(index >= 0);

	return (type &)list[index];
}

/*
================
rvmListSTL<type>::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return NULL if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template< class type >
ID_INLINE type *rvmListSTL<type>::Ptr(void) {
	if (list.size() == 0)
		return nullptr;

	return (type *)&list[0];
}

/*
================
rvmListSTL<type>::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return NULL if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template< class type >
const ID_INLINE type *rvmListSTL<type>::Ptr(void) const {
	if (list.size() == 0)
		return nullptr;

	return (type *)&list[0];
}

/*
================
rvmListSTL<type>::Alloc

Returns a reference to a new data element at the end of the list.
================
*/
template< class type >
ID_INLINE type &rvmListSTL<type>::Alloc(void) {
	type new_obj;
	Append(new_obj);
	return list[list.size() - 1];
}

/*
================
rvmListSTL<type>::Append

Increases the size of the list by one element and copies the supplied data into it.

Returns the index of the new element.
================
*/
template< class type >
ID_INLINE int rvmListSTL<type>::Append(type const & obj) {
	list.push_back(obj);
	return list.size() - 1;
}


/*
================
rvmListSTL<type>::Insert

Increases the size of the list by at leat one element if necessary
and inserts the supplied data into it.

Returns the index of the new element.
================
*/
template< class type >
ID_INLINE int rvmListSTL<type>::Insert(type const & obj, int index) {
	list.insert(list.begin() + index, obj);
	return index;
}

/*
================
rvmListSTL<type>::Append

adds the other list to this one

Returns the size of the new combined list
================
*/
template< class type >
ID_INLINE int rvmListSTL<type>::Append(const rvmListSTL<type> &other) {
	int startIdx = list.size();
	Resize(list.size() + other.Size());

	int n = other.Num();
	for (int i = startIdx; i < startIdx + n; i++) {
		Append(other[i]);
	}

	return Num();
}

/*
================
rvmListSTL<type>::AddUnique

Adds the data to the list if it doesn't already exist.  Returns the index of the data in the list.
================
*/
template< class type >
ID_INLINE int rvmListSTL<type>::AddUnique(type const & obj) {
	int index;

	index = FindIndex(obj);
	if (index < 0) {
		index = Append(obj);
	}

	return index;
}

/*
================
rvmListSTL<type>::FindIndex

Searches for the specified data in the list and returns it's index.  Returns -1 if the data is not found.
================
*/
template< class type >
ID_INLINE int rvmListSTL<type>::FindIndex(type const & obj) const {
	int i;

	for (i = 0; i < list.size(); i++) {
		if (list[i] == obj) {
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
rvmListSTL<type>::Find

Searches for the specified data in the list and returns it's address. Returns NULL if the data is not found.
================
*/
template< class type >
ID_INLINE type *rvmListSTL<type>::Find(type const & obj) const {
	int i;

	i = FindIndex(obj);
	if (i >= 0) {
		return (type *)&Ptr()[i];
	}

	return NULL;
}

/*
================
rvmListSTL<type>::FindNull

Searches for a NULL pointer in the list.  Returns -1 if NULL is not found.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template< class type >
ID_INLINE int rvmListSTL<type>::FindNull(void) const {
	int i;

	for (i = 0; i < list.size(); i++) {
		if (list[i] == NULL) {
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
rvmListSTL<type>::IndexOf

Takes a pointer to an element in the list and returns the index of the element.
This is NOT a guarantee that the object is really in the list.
Function will assert in debug builds if pointer is outside the bounds of the list,
but remains silent in release builds.
================
*/
template< class type >
ID_INLINE int rvmListSTL<type>::IndexOf(type const *objptr) const {
	int index;

	index = objptr - list;

	assert(index >= 0);
//	assert(index < num);

	return index;
}

/*
================
rvmListSTL<type>::RemoveIndex

Removes the element at the specified index and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the index is outside the bounds of the list.
Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template< class type >
ID_INLINE bool rvmListSTL<type>::RemoveIndex(int index) {
	int i;

	//	assert( list != NULL );
	assert(index >= 0);

	list.erase(list.begin() + index);

	return true;
}

/*
================
rvmListSTL<type>::Remove

Removes the element if it is found within the list and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the data is not found in the list.  Note that
the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template< class type >
ID_INLINE bool rvmListSTL<type>::Remove(type const & obj) {
	int index;

	index = FindIndex(obj);
	if (index >= 0) {
		return RemoveIndex(index);
	}

	return false;
}

/*
================
rvmListSTL<type>::Sort

Performs a qsort on the list using the supplied comparison function.  Note that the data is merely moved around the
list, so any pointers to data within the list may no longer be valid.
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::Sort(cmp_t *compare) {
	if (list.size() <= 0) {
		return;
	}
	typedef int cmp_c(const void *, const void *);

	cmp_c *vCompare = (cmp_c *)compare;
	qsort((void *)Ptr(), (size_t)list.size(), sizeof(type), vCompare);
}

/*
================
rvmListSTL<type>::SortSubSection

Sorts a subsection of the list.
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::SortSubSection(int startIndex, int endIndex, cmp_t *compare) {
	if (list.size() <= 0) {
		return;
	}
	if (startIndex < 0) {
		startIndex = 0;
	}
	if (endIndex >= list.size()) {
		endIndex = list.size() - 1;
	}
	if (startIndex >= endIndex) {
		return;
	}
	typedef int cmp_c(const void *, const void *);

	cmp_c *vCompare = (cmp_c *)compare;
	qsort((void *)(Ptr()[startIndex]), (size_t)(endIndex - startIndex + 1), sizeof(type), vCompare);
}

/*
================
rvmListSTL<type>::Swap

Swaps the contents of two lists
================
*/
template< class type >
ID_INLINE void rvmListSTL<type>::Swap(rvmListSTL<type> &other) {
	//	idSwap( num, other.num );
	//	idSwap( size, other.size );
	//	idSwap( granularity, other.granularity );
	idSwap(list, other.list);
}

#undef FLT_EPSILON
