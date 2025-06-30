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

#ifndef SE_INCL_LINEARALLOCATOR_CPP
#define SE_INCL_LINEARALLOCATOR_CPP
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/LinearAllocator.h>

class CLABlockInfo {
public:
  CListNode bi_lnNode;
  INDEX bi_ctObjects;   // number of objects in this block
  void *bi_pvMemory;    // start of block memory
  void *bi_pvEnd;       // end of block memory
};

// default constructor
template <class Type>
CLinearAllocator<Type>::CLinearAllocator(void)
{
  la_ctAllocationStep = 256;
  la_ctObjects = 0;
  la_ctFree = 0;
  la_ptNextFree = NULL;
}

// copy constructor
template <class Type>
CLinearAllocator<Type>::CLinearAllocator(CLinearAllocator<Type> &laOriginal)
{
  ASSERT(FALSE);
}

// destructor -- frees all memory
template <class Type>
CLinearAllocator<Type>::~CLinearAllocator(void)
{
  Clear();
}

// destroy all objects, and reset the allocator to initial (empty) state
template <class Type>
void CLinearAllocator<Type>::Clear(void)
{
  FORDELETELIST(CLABlockInfo, bi_lnNode, la_lhBlocks, itBlock) {
  // for all memory blocks
    // free memory used by block (this doesn't call destructors - see note above!)
    delete[] (Type *)itBlock->bi_pvMemory;
    // free memory used by block info
    delete &itBlock.Current();
  }
  la_ctObjects = 0;
  la_ctFree = 0;
  la_ptNextFree = NULL;
}


/* Set how many elements to allocate when stack overflows. */
template <class Type>
inline void CLinearAllocator<Type>::SetAllocationStep(INDEX ctStep)
{
  la_ctAllocationStep = ctStep;
}


// allocate a new memory block
template <class Type>
void CLinearAllocator<Type>::AllocBlock(INDEX iCount)
{
  ASSERT(this!=NULL && iCount>0);
  //ASSERT(la_ctFree==0);
  Type *ptBlock;
  CLABlockInfo *pbi;

  // allocate the memory and call constructors for all members
  ptBlock = new Type[iCount];     // call vector constructor, for better performance
  // allocate the block info
  pbi = new CLABlockInfo;
  // add the block to list
  la_lhBlocks.AddTail(pbi->bi_lnNode);
  // remember block memory and size
  pbi->bi_pvMemory = ptBlock;
  pbi->bi_pvEnd = ptBlock+iCount;
  pbi->bi_ctObjects = iCount;

  // count total number of allocated objects
  la_ctObjects+=iCount;
  // set up to get new objects from here
  la_ctFree = iCount;
  la_ptNextFree = ptBlock;
}

// allocate a new object
template <class Type>
inline Type &CLinearAllocator<Type>::New(void)
{
  if (la_ctFree == 0) {
    // allocate a new memory block
    AllocBlock(la_ctAllocationStep);
  }
  Type *ptNew = la_ptNextFree;
  la_ctFree--;
  la_ptNextFree++;
  return *ptNew;
}
template <class Type>
inline Type *CLinearAllocator<Type>::New(INDEX ct)
{
  // if not enough space in current block
  if (la_ctFree < ct) {
    // allocate an entirely new memory block of exact that size
    AllocBlock(ct);
    // use it entirely
    Type *ptNew = la_ptNextFree;
    la_ctFree=0;
    la_ptNextFree=NULL;
    return ptNew;
  // if there is enough space in current block
  } else {
    // use the space
    Type *ptNew = la_ptNextFree;
    la_ctFree-=ct;
    la_ptNextFree+=ct;
    return ptNew;
  }
}
// free all objects but keep allocated space and relinearize it
template <class Type>
inline void CLinearAllocator<Type>::Reset(void)
{
  // if there is no block allocated
  if (la_lhBlocks.IsEmpty()) {
    // do nothing
    return;

#ifdef PLATFORM_OPENBSD
#ifdef LIST_HEAD
  #undef LIST_HEAD
#endif
// get the pointer to the first element in the list
#define LIST_HEAD(listhead, baseclass, member) \
  ( (baseclass *) ( ((UBYTE *)(&(listhead).Head())) - _offsetof(baseclass, member) ) )
#endif

  // if there is only one block allocated
  } else if (&la_lhBlocks.Head()==&la_lhBlocks.Tail()) {
    // just restart at the beginning
    la_ctFree = la_ctObjects;
    la_ptNextFree = (Type*) (LIST_HEAD(la_lhBlocks, CLABlockInfo, bi_lnNode)->bi_pvMemory);

  // if there is more than one block allocated
  } else {
    // remember how much objects were used and allocation step
    INDEX ctObjectsOld = la_ctObjects;
    INDEX ctAllocationStepOld = la_ctAllocationStep;
    // free all blocks
    Clear();
    // restore the allocation step
    la_ctAllocationStep = ctAllocationStepOld;
    // allocate only one linear block
    AllocBlock(ctObjectsOld);
  }
}

// make 'for' construct for walking all objects in a linear allocator
#define FOREACHINLINEARALLOCATOR(allocator, type, pt) \
FOREACHINLIST(CLABlockInfo, bi_lnNode, allocator.la_lhBlocks, pt##itBlock) { \
    for(type *pt = (type *)pt##itBlock->bi_pvMemory; pt<(type *)pt##itBlock->bi_pvEnd; pt++)


#endif  /* include-once check. */

