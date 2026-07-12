#ifndef __PODLIST_H__
#define __PODLIST_H__

/*
===============================================================================

	List template
	Does not allocate memory until the first item is added.

===============================================================================
*/

template< class type >
class idPODList
{
	public:

		typedef int		cmp_t(const type *, const type *);
		typedef type	new_t(void);


		idPODList(int newgranularity = 16);
		idPODList(const idPODList<type> &other);
		~idPODList<type>(void);

		void			Clear(void);										// clear the list
		int				Num(void) const;									// returns number of elements in list
		int				NumAllocated(void) const;							// returns number of elements allocated for
		void			SetGranularity(int newgranularity);				// set new granularity
		int				GetGranularity(void) const;						// get the current granularity

		size_t			Allocated(void) const;							// returns total size of allocated memory
		size_t			Size(void) const;									// returns total size of allocated memory including size of list type
		size_t			MemoryUsed(void) const;							// returns size of the used elements in the list

		idPODList<type> &	operator=(const idPODList<type> &other);
		const type 	&operator[](int index) const;
		type 			&operator[](int index);

		void			Condense(void);									// resizes list to exactly the number of elements it contains
		void			Resize(int newsize);								// resizes list to the given number of elements
		void			Resize(int newsize, int newgranularity);			// resizes list and sets new granularity
		void			SetNum(int newnum, bool resize = true);			// set number of elements in list and resize to exactly this number if necessary
		void			AssureSize(int newSize);							// assure list has given number of elements, but leave them uninitialized
		void			AssureSize(int newSize, const type &initValue);	// assure list has given number of elements and initialize any new elements
		void			AssureSizeAlloc(int newSize, new_t *allocator);	// assure the pointer list has the given number of elements and allocate any new elements

		type 			*Ptr(void);										// returns a pointer to the list
		const type 	*Ptr(void) const;									// returns a pointer to the list
		type 			&Alloc(void);										// returns reference to a new data element at the end of the list
		int				Append(const type &obj);							// append element
		int				Append(const idPODList<type> &other);				// append list
		int				AddUnique(const type &obj);						// add unique element
		int				Insert(const type &obj, int index = 0);			// insert the element at the given index
		int				FindIndex(const type &obj) const;				// find the index for the given element
		type 			*Find(type const &obj) const;						// find pointer to the given element
		int				FindNull(void) const;								// find the index for the first NULL pointer in the list
		int				IndexOf(const type *obj) const;					// returns the index for the pointer to an element in the list
		bool			RemoveIndex(int index);							// remove the element at the given index
		bool			Remove(const type &obj);							// remove the element
		void			Sort(cmp_t *compare = (cmp_t *)&idListSortCompare<type>);
		void			SortSubSection(int startIndex, int endIndex, cmp_t *compare = (cmp_t *)&idListSortCompare<type>);
		void			Swap(idPODList<type> &other);						// swap the contents of the lists
		void			DeleteContents(bool clear);						// delete the contents of the list
	protected:
		static type *	MemAlloc(int size);
		static void		MemFree(type *ptr);
		static void		MemCopy(type *dst, const type *src, int num);
		static void		MemMove(type *list, int dst, int src);

	private:
		int				num;
		int				size;
		int				granularity;
		type 			*list;
};

/*
================
idPODList<type>::idPODList( int )
================
*/
template< class type >
ID_INLINE idPODList<type>::idPODList(int newgranularity)
{
	assert(newgranularity > 0);

	list		= NULL;
	granularity	= newgranularity;
	Clear();
}

/*
================
idPODList<type>::idPODList( const idPODList<type> &other )
================
*/
template< class type >
ID_INLINE idPODList<type>::idPODList(const idPODList<type> &other)
{
	list = NULL;
	*this = other;
}

/*
================
idPODList<type>::~idPODList<type>
================
*/
template< class type >
ID_INLINE idPODList<type>::~idPODList(void)
{
	Clear();
}

/*
================
idPODList<type>::Clear

Frees up the memory allocated by the list.  Assumes that type automatically handles freeing up memory.
================
*/
template< class type >
ID_INLINE void idPODList<type>::Clear(void)
{
	if (list) {
		MemFree(list);
	}

	list	= NULL;
	num		= 0;
	size	= 0;
}

/*
================
idPODList<type>::DeleteContents

Calls the destructor of all elements in the list.  Conditionally frees up memory used by the list.
Note that this only works on lists containing pointers to objects and will cause a compiler error
if called with non-pointers.  Since the list was not responsible for allocating the object, it has
no information on whether the object still exists or not, so care must be taken to ensure that
the pointers are still valid when this function is called.  Function will set all pointers in the
list to NULL.
================
*/
template< class type >
ID_INLINE void idPODList<type>::DeleteContents(bool clear)
{
	int i;

	for (i = 0; i < num; i++) {
		delete list[ i ];
		list[ i ] = NULL;
	}

	if (clear) {
		Clear();
	} else {
		memset(list, 0, size * sizeof(type));
	}
}

/*
================
idPODList<type>::Allocated

return total memory allocated for the list in bytes, but doesn't take into account additional memory allocated by type
================
*/
template< class type >
ID_INLINE size_t idPODList<type>::Allocated(void) const
{
	return size * sizeof(type);
}

/*
================
idPODList<type>::Size

return total size of list in bytes, but doesn't take into account additional memory allocated by type
================
*/
template< class type >
ID_INLINE size_t idPODList<type>::Size(void) const
{
	return sizeof(idPODList<type>) + Allocated();
}

/*
================
idPODList<type>::MemoryUsed
================
*/
template< class type >
ID_INLINE size_t idPODList<type>::MemoryUsed(void) const
{
	return num * sizeof(*list);
}

/*
================
idPODList<type>::Num

Returns the number of elements currently contained in the list.
Note that this is NOT an indication of the memory allocated.
================
*/
template< class type >
ID_INLINE int idPODList<type>::Num(void) const
{
	return num;
}

/*
================
idPODList<type>::NumAllocated

Returns the number of elements currently allocated for.
================
*/
template< class type >
ID_INLINE int idPODList<type>::NumAllocated(void) const
{
	return size;
}

/*
================
idPODList<type>::SetNum

Resize to the exact size specified irregardless of granularity
================
*/
template< class type >
ID_INLINE void idPODList<type>::SetNum(int newnum, bool resize)
{
	assert(newnum >= 0);

	if (resize || newnum > size) {
		Resize(newnum);
	}

	num = newnum;
}

/*
================
idPODList<type>::SetGranularity

Sets the base size of the array and resizes the array to match.
================
*/
template< class type >
ID_INLINE void idPODList<type>::SetGranularity(int newgranularity)
{
	int newsize;

	assert(newgranularity > 0);
	granularity = newgranularity;

	if (list) {
		// resize it to the closest level of granularity
		newsize = num + granularity - 1;
		newsize -= newsize % granularity;

		if (newsize != size) {
			Resize(newsize);
		}
	}
}

/*
================
idPODList<type>::GetGranularity

Get the current granularity.
================
*/
template< class type >
ID_INLINE int idPODList<type>::GetGranularity(void) const
{
	return granularity;
}

/*
================
idPODList<type>::Condense

Resizes the array to exactly the number of elements it contains or frees up memory if empty.
================
*/
template< class type >
ID_INLINE void idPODList<type>::Condense(void)
{
	if (list) {
		if (num) {
			Resize(num);
		} else {
			Clear();
		}
	}
}

/*
================
idPODList<type>::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correnctly instantiated.
================
*/
template< class type >
ID_INLINE void idPODList<type>::Resize(int newsize)
{
	type	*temp;

	assert(newsize >= 0);

	// free up the list if no data is being reserved
	if (newsize <= 0) {
		Clear();
		return;
	}

	if (newsize == size) {
		// not changing the size, so just exit
		return;
	}

	temp	= list;
	size	= newsize;

	if (size < num) {
		num = size;
	}

	// copy the old list into our new one
	list = MemAlloc(sizeof(type) * size);

	if(temp) {
		MemCopy(list, temp, num);

	// delete the old list if it exists
		MemFree(temp);
	}
}

/*
================
idPODList<type>::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correnctly instantiated.
================
*/
template< class type >
ID_INLINE void idPODList<type>::Resize(int newsize, int newgranularity)
{
	type	*temp;
	int		i;

	assert(newsize >= 0);

	assert(newgranularity > 0);
	granularity = newgranularity;

	// free up the list if no data is being reserved
	if (newsize <= 0) {
		Clear();
		return;
	}
	
    if ( newsize == size ) {
        // not changing the size, so just exit
        return;
    }

	temp	= list;
	size	= newsize;

	if (size < num) {
		num = size;
	}

	// copy the old list into our new one
	list = MemAlloc(size);

	if (temp) {
		MemCopy(list, temp, num);

	// delete the old list if it exists
		MemFree(temp);
	}
}

/*
================
idPODList<type>::AssureSize

Makes sure the list has at least the given number of elements.
================
*/
template< class type >
ID_INLINE void idPODList<type>::AssureSize(int newSize)
{
	int newNum = newSize;

	if (newSize > size) {

		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		Resize(newSize);
	}

	num = newNum;
}

/*
================
idPODList<type>::AssureSize

Makes sure the list has at least the given number of elements and initialize any elements not yet initialized.
================
*/
template< class type >
ID_INLINE void idPODList<type>::AssureSize(int newSize, const type &initValue)
{
	int newNum = newSize;

	if (newSize > size) {

		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		num = size;
		Resize(newSize);

		for (int i = num; i < newSize; i++) {
			list[i] = initValue;
		}
	}

	num = newNum;
}

/*
================
idPODList<type>::AssureSizeAlloc

Makes sure the list has at least the given number of elements and allocates any elements using the allocator.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template< class type >
ID_INLINE void idPODList<type>::AssureSizeAlloc(int newSize, new_t *allocator)
{
	int newNum = newSize;

	if (newSize > size) {

		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		num = size;
		Resize(newSize);

		for (int i = num; i < newSize; i++) {
			list[i] = (*allocator)();
		}
	}

	num = newNum;
}

/*
================
idPODList<type>::operator=

Copies the contents and size attributes of another list.
================
*/
template< class type >
ID_INLINE idPODList<type> &idPODList<type>::operator=(const idPODList<type> &other)
{
    if( &other == this ) {
        return *this;
    }
	int	i;

	Clear();

	num			= other.num;
	size		= other.size;
	granularity	= other.granularity;

	if (size) {
		list = MemAlloc(size);

		MemCopy(list, other.list, num);
	}

	return *this;
}

/*
================
idPODList<type>::operator[] const

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template< class type >
ID_INLINE const type &idPODList<type>::operator[](int index) const
{
	assert(index >= 0);
	assert(index < num);

	return list[ index ];
}

/*
================
idPODList<type>::operator[]

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template< class type >
ID_INLINE type &idPODList<type>::operator[](int index)
{
	assert(index >= 0);
	assert(index < num);

	return list[ index ];
}

/*
================
idPODList<type>::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return NULL if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template< class type >
ID_INLINE type *idPODList<type>::Ptr(void)
{
	return list;
}

/*
================
idPODList<type>::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return NULL if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template< class type >
const ID_INLINE type *idPODList<type>::Ptr(void) const
{
	return list;
}

/*
================
idPODList<type>::Alloc

Returns a reference to a new data element at the end of the list.
================
*/
template< class type >
ID_INLINE type &idPODList<type>::Alloc(void)
{
	if (!list) {
		Resize(granularity);
	}

	if (num == size) {
		Resize(size + granularity);
	}

	return list[ num++ ];
}

/*
================
idPODList<type>::Append

Increases the size of the list by one element and copies the supplied data into it.

Returns the index of the new element.
================
*/
template< class type >
ID_INLINE int idPODList<type>::Append(type const &obj)
{
	if (!list) {
		Resize(granularity);
	}

	if (num == size) {
		int newsize;

		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newsize = size + granularity;
		Resize(newsize - newsize % granularity);
	}

	list[ num ] = obj;
	num++;

	return num - 1;
}


/*
================
idPODList<type>::Insert

Increases the size of the list by at leat one element if necessary
and inserts the supplied data into it.

Returns the index of the new element.
================
*/
template< class type >
ID_INLINE int idPODList<type>::Insert(type const &obj, int index)
{
	if (!list) {
		Resize(granularity);
	}

	if (num == size) {
		int newsize;

		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newsize = size + granularity;
		Resize(newsize - newsize % granularity);
	}

	if (index < 0) {
		index = 0;
	} else if (index > num) {
		index = num;
	}

	for (int i = num; i > index; --i) {
		MemMove(list, i, i - 1);
	}

	num++;
	list[index] = obj;
	return index;
}

/*
================
idPODList<type>::Append

adds the other list to this one

Returns the size of the new combined list
================
*/
template< class type >
ID_INLINE int idPODList<type>::Append(const idPODList<type> &other)
{
	if (!list) {
		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		Resize(granularity);
	}
 	else {
        Resize( num + other.Num() );
    }

	int n = other.Num();

	for (int i = 0; i < n; i++) {
		Append(other[i]);
	}

	return Num();
}

/*
================
idPODList<type>::AddUnique

Adds the data to the list if it doesn't already exist.  Returns the index of the data in the list.
================
*/
template< class type >
ID_INLINE int idPODList<type>::AddUnique(type const &obj)
{
	int index;

	index = FindIndex(obj);

	if (index < 0) {
		index = Append(obj);
	}

	return index;
}

/*
================
idPODList<type>::FindIndex

Searches for the specified data in the list and returns it's index.  Returns -1 if the data is not found.
================
*/
template< class type >
ID_INLINE int idPODList<type>::FindIndex(type const &obj) const
{
	int i;

	for (i = 0; i < num; i++) {
		if (list[ i ] == obj) {
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
idPODList<type>::Find

Searches for the specified data in the list and returns it's address. Returns NULL if the data is not found.
================
*/
template< class type >
ID_INLINE type *idPODList<type>::Find(type const &obj) const
{
	int i;

	i = FindIndex(obj);

	if (i >= 0) {
		return &list[ i ];
	}

	return NULL;
}

/*
================
idPODList<type>::FindNull

Searches for a NULL pointer in the list.  Returns -1 if NULL is not found.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template< class type >
ID_INLINE int idPODList<type>::FindNull(void) const
{
	int i;

	for (i = 0; i < num; i++) {
		if (list[ i ] == NULL) {
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
idPODList<type>::IndexOf

Takes a pointer to an element in the list and returns the index of the element.
This is NOT a guarantee that the object is really in the list.
Function will assert in debug builds if pointer is outside the bounds of the list,
but remains silent in release builds.
================
*/
template< class type >
ID_INLINE int idPODList<type>::IndexOf(type const *objptr) const
{
	int index;

	index = objptr - list;

	assert(index >= 0);
	assert(index < num);

	return index;
}

/*
================
idPODList<type>::RemoveIndex

Removes the element at the specified index and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the index is outside the bounds of the list.
Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template< class type >
ID_INLINE bool idPODList<type>::RemoveIndex(int index)
{
	int i;

	assert(list != NULL);
	assert(index >= 0);
	assert(index < num);

	if ((index < 0) || (index >= num)) {
		return false;
	}

	num--;

	for (i = index; i < num; i++) {
		MemMove(list, i, i + 1);
	}

	return true;
}

/*
================
idPODList<type>::Remove

Removes the element if it is found within the list and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the data is not found in the list.  Note that
the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template< class type >
ID_INLINE bool idPODList<type>::Remove(type const &obj)
{
	int index;

	index = FindIndex(obj);

	if (index >= 0) {
		return RemoveIndex(index);
	}

	return false;
}

/*
================
idPODList<type>::Sort

Performs a qsort on the list using the supplied comparison function.  Note that the data is merely moved around the
list, so any pointers to data within the list may no longer be valid.
================
*/
template< class type >
ID_INLINE void idPODList<type>::Sort(cmp_t *compare)
{
	if (!list) {
		return;
	}

	typedef int cmp_c(const void *, const void *);

	cmp_c *vCompare = (cmp_c *)compare;
	qsort((void *)list, (size_t)num, sizeof(type), vCompare);
}

/*
================
idPODList<type>::SortSubSection

Sorts a subsection of the list.
================
*/
template< class type >
ID_INLINE void idPODList<type>::SortSubSection(int startIndex, int endIndex, cmp_t *compare)
{
	if (!list) {
		return;
	}

	if (startIndex < 0) {
		startIndex = 0;
	}

	if (endIndex >= num) {
		endIndex = num - 1;
	}

	if (startIndex >= endIndex) {
		return;
	}

	typedef int cmp_c(const void *, const void *);

	cmp_c *vCompare = (cmp_c *)compare;
	qsort((void *)(&list[startIndex]), (size_t)(endIndex - startIndex + 1), sizeof(type), vCompare);
}

/*
================
idPODList<type>::Swap

Swaps the contents of two lists
================
*/
template< class type >
ID_INLINE void idPODList<type>::Swap(idPODList<type> &other)
{
	idSwap(num, other.num);
	idSwap(size, other.size);
	idSwap(granularity, other.granularity);
	idSwap(list, other.list);
}

template< class type >
ID_INLINE type * idPODList<type>::MemAlloc(int size)
{
#if 1
	return (type *)malloc(sizeof(type) * size);
#else
	return (type *)Mem_Alloc(sizeof(type) * size);
#endif
}

template< class type >
ID_INLINE void idPODList<type>::MemFree(type *ptr)
{
#if 1
	free(ptr);
#else
	Mem_Free(ptr);
#endif
}

template< class type >
ID_INLINE void idPODList<type>::MemCopy(type *dst, const type *src, int num)
{
	if(num > 0 && src)
		memcpy(dst, src, num * sizeof(type));
}

template< class type >
ID_INLINE void idPODList<type>::MemMove(type *list, int dst, int src)
{
#if 1
	list[dst] = list[src];
#else
	memcpy(list + dst, list + src, sizeof(type));
#endif
}

#endif /* !__PODLIST_H__ */

