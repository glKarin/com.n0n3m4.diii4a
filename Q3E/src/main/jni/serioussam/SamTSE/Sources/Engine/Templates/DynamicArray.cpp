/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef SE_INCL_DYNAMICARRAY_CPP
#define SE_INCL_DYNAMICARRAY_CPP
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <stddef.h>

#include <Engine/Base/Memory.h>
#include <Engine/Base/ListIterator.inl>

#include <Engine/Templates/DynamicArray.h>

// iterate whole dynamic array
/* NOTE: The iterator defined by this macro must be destroyed before adding/removing
 * elements in the array. To do so, embed the for loop in additional curly braces.
 */
#define FOREACHINDYNAMICARRAY(array, type, iter) \
  for(CDynamicArrayIterator<type> iter(array); !iter.IsPastEnd(); iter.MoveToNext() )

class CDABlockInfo {
public:
  CListNode bi_ListNode;
  void *bi_Memory;
};

/*
 * Default constructor.
 */
template<class Type>
CDynamicArray<Type>::CDynamicArray(void) {
#if CHECKARRAYLOCKING
  // not locked
  da_LockCt = 0;
#endif
  // set to empty array of pointers
  da_Pointers = NULL;
  da_Count = 0;
}

/*
 * Copy constructor.
 */
template<class Type>
CDynamicArray<Type>::CDynamicArray(CDynamicArray<Type> &daOriginal)
{
#if CHECKARRAYLOCKING
  // not locked
  da_LockCt = 0;
#endif
  // set to empty array of pointers
  da_Pointers = NULL;
  da_Count = 0;

  // call assignment operator
  (*this) = daOriginal;
}
/*
 * Destructor -- frees all memory.
 */
template<class Type>
CDynamicArray<Type>::~CDynamicArray(void) {
  Clear();
}

/*
 * Destroy all objects, and reset the array to initial (empty) state.
 */
template<class Type>
void CDynamicArray<Type>::Clear(void) {
  ASSERT(this!=NULL);
  // if any pointers are allocated
  if (da_Count!=0) {
    /* NOTE: We must explicitly clear objects here, because array deleting
     * does not call object destructors!
     */
    // for all pointers
    for (INDEX iPointer=0; iPointer<da_Count; iPointer++) {
      // destroy the object that it points to
      ::Clear(*da_Pointers[iPointer]);
    }

    // free the pointers
    FreeMemory(da_Pointers);
    // mark as freed
    da_Pointers = NULL;
    da_Count = 0;
  // otherwise
  } else {
    // check that the pointers are really not allocated
    ASSERT(da_Pointers==NULL);
    // nothing to free
  }
  // for all memory blocks
  FORDELETELIST(CDABlockInfo, bi_ListNode, da_BlocksList, itBlock) {
    // free memory used by block (this doesn't call destructors - see note above!)
    delete[] (Type *)itBlock->bi_Memory;
    // free memory used by block info
    delete &itBlock.Current();
  }
}

/*
 * Grow pointer array by a given number of members.
 */
template<class Type>
void CDynamicArray<Type>::GrowPointers(INDEX iCount) {
  ASSERT(this!=NULL && iCount>0);
  // if not yet allocated
  if (da_Count==0) {
    // check that the pointers are really not allocated
    ASSERT(da_Pointers==NULL);
    // allocate
    da_Count=iCount;
    da_Pointers = (Type **)AllocMemory(da_Count*sizeof(Type*));
  // if allocated
  } else {
    // grow to new size
    da_Count+=iCount;
    GrowMemory((void **)&da_Pointers, da_Count*sizeof(Type*));
  }
}

/*
 * Shrink pointer array by a given number of members.
 */
template<class Type>
void CDynamicArray<Type>::ShrinkPointers(INDEX iCount) {
  ASSERT(this!=NULL && iCount>0);
  // check that the pointers are allocated
  ASSERT(da_Pointers!=NULL);

  // decrement count
  da_Count-=iCount;
  // checked that it has not dropped below zero
  ASSERT(da_Count>=0);
  // if all pointers are freed by this
  if (da_Count==0) {
    // free the array
    FreeMemory(da_Pointers);
    da_Pointers = NULL;
  // if some remain
  } else {
    // shrink to new size
    ShrinkMemory((void **)&da_Pointers, da_Count*sizeof(Type*));
  }
}

/*
 * Allocate a new memory block.
 */
template<class Type>
Type *CDynamicArray<Type>::AllocBlock(INDEX iCount) {
  ASSERT(this!=NULL && iCount>0);
  Type *ptBlock;
  CDABlockInfo *pbi;

  // allocate the memory and call constructors for all members (+1 for cache-prefetch opt)
  ptBlock = new Type[iCount+1];     // call vector constructor, for better performance
  // allocate the block info
  pbi = new CDABlockInfo;
  // add the block to list
  da_BlocksList.AddTail(pbi->bi_ListNode);
  // remember block memory
  pbi->bi_Memory = ptBlock;
  return ptBlock;
}

/*
 * Create a given number of new members.
 */
template<class Type>
Type *CDynamicArray<Type>::New(INDEX iCount /*= 1*/) {
  ASSERT(this!=NULL && iCount>=0);
  // if no new members are needed in fact
  if (iCount==0) {
    // do nothing
    return NULL;
  }
  Type *ptBlock;
  INDEX iOldCount = da_Count;

  // grow the pointer table
  GrowPointers(iCount);
  // allocate the memory block
  ptBlock = AllocBlock(iCount);
  // set pointers
  for(INDEX iNewMember=0; iNewMember<iCount; iNewMember++) {
    da_Pointers[iOldCount+iNewMember] = ptBlock+iNewMember;
  }
  return ptBlock;
}

/*
 * Delete a given member.
 */
template<class Type>
void CDynamicArray<Type>::Delete(Type *ptMember) {
  ASSERT(this!=NULL);
#if CHECKARRAYLOCKING
  // check that not locked for indices
  ASSERT(da_LockCt == 0);
#endif

  // clear the object
  ::Clear(*ptMember);

  INDEX iMember=GetIndex(ptMember);
  // move last pointer here
  da_Pointers[iMember]=da_Pointers[da_Count-1];
  // shrink pointers by one
  ShrinkPointers(1);
  // do nothing to free memory
  //!!!!
}

/*
 * Get pointer to a member from it's index.
 */
template<class Type>
Type *CDynamicArray<Type>::Pointer(INDEX iMember) {
  ASSERT(this!=NULL);
  // check that index is currently valid
  ASSERT(iMember>=0 && iMember<da_Count);
#if CHECKARRAYLOCKING
  // check that locked for indices
  ASSERT(da_LockCt>0);
#endif
  return da_Pointers[iMember];
}
template<class Type>
const Type *CDynamicArray<Type>::Pointer(INDEX iMember) const {
  ASSERT(this!=NULL);
  // check that index is currently valid
  ASSERT(iMember>=0 && iMember<da_Count);
#if CHECKARRAYLOCKING
  // check that locked for indices
  ASSERT(da_LockCt>0);
#endif
  return da_Pointers[iMember];
}

/*
 * Lock for getting indices.
 */
template<class Type>
void CDynamicArray<Type>::Lock(void) {
  ASSERT(this!=NULL);
#if CHECKARRAYLOCKING
  ASSERT(da_LockCt>=0);
  // increment lock counter
  da_LockCt++;
#endif
}

/*
 * Unlock after getting indices.
 */
template<class Type>
void CDynamicArray<Type>::Unlock(void) {
  ASSERT(this!=NULL);
#if CHECKARRAYLOCKING
  da_LockCt--;
  ASSERT(da_LockCt>=0);
#endif
}

/*
 * Get index of a member from it's pointer.
 */
template<class Type>
INDEX CDynamicArray<Type>::Index(Type *ptMember) {
  ASSERT(this!=NULL);
#if CHECKARRAYLOCKING
  // check that locked for indices
  ASSERT(da_LockCt>0);
#endif
  return GetIndex(ptMember);
}

/*
 * Get index of a member from it's pointer without locking.
 */
template<class Type>
INDEX CDynamicArray<Type>::GetIndex(Type *ptMember) {
  ASSERT(this!=NULL);
  // slow !!!!
  // check all members
  for (INDEX iMember=0; iMember<da_Count; iMember++) {
    if(da_Pointers[iMember]==ptMember) {
      return iMember;
    }
  }
  ASSERTALWAYS("CDynamicArray<>::Index(): Not a member of this array!");
  return 0;
}

/*
 * Get number of elements in array.
 */
template<class Type>
INDEX CDynamicArray<Type>::Count(void) const {
  ASSERT(this!=NULL);
  return da_Count;
}

/*
 * Assignment operator.
 */
template<class Type>
CDynamicArray<Type> &CDynamicArray<Type>::operator=(CDynamicArray<Type> &arOriginal)
{
  ASSERT(this!=NULL);
  ASSERT(&arOriginal!=NULL);
  ASSERT(this!=&arOriginal);
  // clear previous contents
  Clear();
  // get count of elements in original array
  INDEX ctOriginal = arOriginal.Count();
  // if the other array has no elements
  if (ctOriginal ==0) {
    // no assignment
    return*this;
  }
  // create that much elements
  Type *atNew = New(ctOriginal);

  // copy them all
  arOriginal.Lock();
  for (INDEX iNew=0; iNew<ctOriginal; iNew++) {
    atNew[iNew] = arOriginal[iNew];
  }
  arOriginal.Unlock();

  return *this;
}

/*
 * Move all elements of another array into this one.
 */
template<class Type>
void CDynamicArray<Type>::MoveArray(CDynamicArray<Type> &arOther)
{
  ASSERT(this!=NULL && &arOther!=NULL);
#if CHECKARRAYLOCKING
  // check that not locked for indices
  ASSERT(da_LockCt==0 && arOther.da_LockCt==0);
#endif

  // if the other array has no elements
  if (arOther.da_Count==0) {
    // no moving
    return;
  }

  // remember number of elements
  INDEX iOldCount = da_Count;
  // grow pointer array to add the pointers to elements of other array
  GrowPointers(arOther.da_Count);
  // for each pointer in other array
  for (INDEX iOtherPointer=0; iOtherPointer<arOther.da_Count; iOtherPointer++) {
    // copy it at the end of this array
    da_Pointers[iOldCount+iOtherPointer] = arOther.da_Pointers[iOtherPointer];
  }
  // remove array of pointers in other array
  arOther.ShrinkPointers(arOther.da_Count);
  // move list of allocated blocks from the other array to the end of this one
  da_BlocksList.MoveList(arOther.da_BlocksList);
}

/////////////////////////////////////////////////////////////////////
// CDynamicArrayIterator

/*
 * Template class for iterating dynamic array.
 */
template<class Type>
class CDynamicArrayIterator {
private:
  INDEX dai_Index;           // index of current element
  CDynamicArray<Type> &dai_Array;   // reference to array
public:
  /* Constructor for given array. */
  inline CDynamicArrayIterator(CDynamicArray<Type> &da);
  /* Destructor. */
  inline ~CDynamicArrayIterator(void);

  /* Move to next object. */
  inline void MoveToNext(void);
  /* Check if finished. */
  inline BOOL IsPastEnd(void);
  /* Get current element. */
  Type &Current(void) { return *dai_Array.Pointer(dai_Index); }
  Type &operator*(void) { return *dai_Array.Pointer(dai_Index); }
  operator Type *(void) { return dai_Array.Pointer(dai_Index); }
  Type *operator->(void) { return dai_Array.Pointer(dai_Index); }
};


/*
 * Constructor for given array.
 */
template<class Type>
inline CDynamicArrayIterator<Type>::CDynamicArrayIterator(CDynamicArray<Type> &da) : dai_Array(da) {
  // lock indices
  dai_Array.Lock();
  dai_Index = 0;
}

/*
 * Destructor.
 */
template<class Type>
inline CDynamicArrayIterator<Type>::~CDynamicArrayIterator(void) {
  // unlock indices
  dai_Array.Unlock();
  dai_Index = -1;
}

/*
 * Move to next object.
 */
template<class Type>
inline void CDynamicArrayIterator<Type>::MoveToNext(void) {
  ASSERT(this!=NULL);
  dai_Index++;
}

/*
 * Check if finished.
 */
template<class Type>
inline BOOL CDynamicArrayIterator<Type>::IsPastEnd(void) {
  ASSERT(this!=NULL);
  return dai_Index>=dai_Array.Count();
}



#endif  /* include-once check. */

