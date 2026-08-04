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

#ifndef __FLEXLIST_H__
#define __FLEXLIST_H__

/**
 * Array with hybrid storage.
 * Contains automatic storage, which is used as long as count <= N.
 * Allocates dynamic memory from heap when count exceeds N.
 *
 * As in all other idlib containers, all elements are always in constructed state.
 * This includes the N elements in automatic storage.
 * BEWARE: do not use it for elements with nontrivial constructor,
 *         or you will get serious performance problem!
 *
 * Originally implemented to substitute numerous local arrays of size MAX_GENTITIES.
 * That's why its interface is a bit limited compared to idList.
 *
 * Contains self-reference, hence is not trivially relocatable.
 */

template< class type, int N >
class idFlexList {
public:
	~idFlexList() {
		if (list != autoStore)
			delete[] list;
	}
	idFlexList() {
		list = autoStore;
		num = 0;
		size = N;
	}
	void Clear() {
		num = 0;
	}
	void ClearFree() {
		if (list != autoStore)
			delete[] list;
		list = autoStore;
		num = 0;
		size = N;
	}

	void SetNum( int newNum ) {
		if (newNum > size) {
			int newSize = newNum;
			if (newSize < 2 * size)
				newSize = 2 * size;	//ensure exponential growth
			Grow(newSize);
		}
		num = newNum;
	}

	ID_FORCE_INLINE int Num( void ) const {
		return num;
	}
	int	NumAllocated( void ) const {
		return size;
	}
	ID_FORCE_INLINE type * Ptr( void ) {
		return list;
	}
	ID_FORCE_INLINE const type * Ptr( void ) const {
		return list;
	}

	ID_FORCE_INLINE const type &operator[]( int index ) const {
		assert(unsigned(index) < unsigned(num));
		return list[index];
	}
	ID_FORCE_INLINE type &operator[]( int index ) {
		assert(unsigned(index) < unsigned(num));
		return list[index];
	}
	ID_FORCE_INLINE const type &Last() const {
		assert(num > 0);
		return list[num - 1];
	}
	ID_FORCE_INLINE type &Last() {
		assert(num > 0);
		return list[num - 1];
	}

	int	AddGrow( type obj ) {
		if (num == size)
			Grow(2 * size);
		int idx = num++;
		list[idx] = obj;
		return idx;
	}

	ID_FORCE_INLINE type Pop( void ) {
		return list[--num];
	}

	type *Find( const type &value ) const {
		for (int i = 0; i < num; i++)
			if (list[i] == value)
				return &list[i];
		return nullptr;
	}

	void Append( int k, const type *arr ) {
		int base = num;
		SetNum(base + k);
		for (int i = 0; i < k; i++)
			list[base + i] = arr[i];
	}

private:
	void Grow(int newSize) {
		type *newList = new type[newSize];
		for (int i = 0; i < num; i++)
			newList[i] = list[i];
		if (list != autoStore)
			delete[] list;
		list = newList;
		size = newSize;
	}

	//noncopyable!
	//moving automatic storage around is not worth it
	idFlexList(const idFlexList &) = delete;
	idFlexList& operator= (const idFlexList &) = delete;

	int num;
	int size;
	type *list;
	type autoStore[N];
};

#endif
