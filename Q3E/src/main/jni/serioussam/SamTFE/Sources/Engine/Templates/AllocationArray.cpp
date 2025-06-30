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


#ifndef SE_INCL_ALLOCATIONARRAY_CPP
#define SE_INCL_ALLOCATIONARRAY_CPP
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/StaticStackArray.h>
#include <Engine/Templates/StaticStackArray.cpp>
#include <Engine/Templates/StaticArray.cpp>

extern BOOL _bAllocationArrayParanoiaCheck;

/*
 * Default constructor.
 */
template<class Type>
inline CAllocationArray<Type>::CAllocationArray(void) : 
  CStaticArray<Type>(),
  aa_aiFreeElements()
{
  aa_ctAllocationStep = 256;
}

/*
 * Destructor.
 */
template<class Type>
inline CAllocationArray<Type>::~CAllocationArray(void) {
};

/* Destroy all objects, and reset the array to initial (empty) state. */
template<class Type>
inline void CAllocationArray<Type>::Clear(void) {
  // delete the objects themselves
  CStaticArray<Type>::Clear();
  // clear array of free indices
  aa_aiFreeElements.Clear();
}

  /*
 * Set how many elements to allocate when stack overflows.
 */
template<class Type>
inline void CAllocationArray<Type>::SetAllocationStep(INDEX ctStep) 
{
  ASSERT(ctStep>0);
  aa_ctAllocationStep = ctStep;
};

/*
 * Create a given number of objects.
 */
template<class Type>
inline void CAllocationArray<Type>::New(INDEX iCount) {
  // never call this!
  ASSERT(FALSE);
};

/*
 * Destroy all objects.
 */
template<class Type>
inline void CAllocationArray<Type>::Delete(void) {
  // never call this!
  ASSERT(FALSE);
}

/* Alocate a new object. */
template<class Type>
inline INDEX CAllocationArray<Type>::Allocate(void)
{
  // if there are no more free indices
  if (aa_aiFreeElements.Count()==0) {
    // remember old size
    INDEX ctOldSize = CStaticArray<Type>::Count();
    // expand the array by the allocation step
    this->Expand(ctOldSize+aa_ctAllocationStep);
    // create new free indices
    INDEX *piNewFree = aa_aiFreeElements.Push(aa_ctAllocationStep);
    // fill them up
    for(INDEX iNew=0; iNew<aa_ctAllocationStep; iNew++) {
      piNewFree[iNew] = ctOldSize+iNew;
    }
  }
  // pop one free index from the top of stack, and use that one
  return aa_aiFreeElements.Pop();
}
/* Free object with given index. */
template<class Type>
inline void CAllocationArray<Type>::Free(INDEX iToFree)
{
#ifndef NDEBUG
  // must be within pool limits
  ASSERT(iToFree>=0 && iToFree<CStaticArray<Type>::Count());
  // must not be free
  if (_bAllocationArrayParanoiaCheck) {
    ASSERT(IsAllocated(iToFree));
  }
#endif
  // push its index on top of the free stack
  aa_aiFreeElements.Push() = iToFree;
}

/* Free all objects, but keep pool space. */
template<class Type>
inline void CAllocationArray<Type>::FreeAll(void)
{
  // clear the free array
  aa_aiFreeElements.PopAll();
  // push as much free elements as there is pool space
  INDEX ctSize = CStaticArray<Type>::Count();
  INDEX *piNewFree = aa_aiFreeElements.Push(ctSize);
  // fill them up
  for(INDEX iNew=0; iNew<ctSize; iNew++) {
    piNewFree[iNew] = iNew;
  }
}

// check if an index is allocated (slow!)
template<class Type>
inline BOOL CAllocationArray<Type>::IsAllocated(INDEX i)
{
  // must be within pool limits
  ASSERT(i>=0 && i<CStaticArray<Type>::Count());
  // for each free index
  INDEX ctFree = aa_aiFreeElements.Count();
  for(INDEX iFree=0; iFree<ctFree; iFree++) {
    // if it is that one
    if (aa_aiFreeElements[iFree]==i) {
      // it is not allocated
      return FALSE;
    }
  }
  // if not found as free, it is allocated
  return TRUE;
}

/* Random access operator. */
// rcg10162001 wtf...I had to move this into the class definition itself.
//  I think it's an optimization bug; I didn't have this problem when I
//  didn't give GCC the "-O2" option.
#if 0
template<class Type>
inline Type &CAllocationArray<Type>::operator[](INDEX iObject)
{
#ifndef NDEBUG
  ASSERT(this!=NULL);
  // must be within pool limits
  ASSERT(iObject>=0 && iObject<CStaticArray<Type>::Count());
  // must not be free
  if (_bAllocationArrayParanoiaCheck) {
    ASSERT(IsAllocated(iObject));
  }
#endif
  return CStaticArray<Type>::operator[](iObject);
}
template<class Type>
inline const Type &CAllocationArray<Type>::operator[](INDEX iObject) const
{
#ifndef NDEBUG
  ASSERT(this!=NULL);
  // must be within pool limits
  ASSERT(iObject>=0 && iObject<CStaticArray<Type>::Count());
  // must not be free
  if (_bAllocationArrayParanoiaCheck) {
    ASSERT(IsAllocated(iObject));
  }
#endif
  return CStaticArray<Type>::operator[](iObject);
}
#endif  // 0

/* Get number of allocated objects in array. */
template<class Type>
INDEX CAllocationArray<Type>::Count(void) const
{
  ASSERT(this!=NULL);
  // it is pool size without the count of free elements
  return CStaticArray<Type>::Count()-aa_aiFreeElements.Count();
}

/* Get index of a object from it's pointer. */
template<class Type>
INDEX CAllocationArray<Type>::Index(Type *ptObject)
{
  ASSERT(this!=NULL);
  INDEX i = CStaticArray<Type>::Index(this->ptMember);
  ASSERT(IsAllocated(i));
  return i;
}

/* Assignment operator. */
template<class Type>
CAllocationArray<Type> &CAllocationArray<Type>::operator=(
  const CAllocationArray<Type> &aaOriginal)
{
  ASSERT(this!=NULL);
  (CStaticArray<Type>&)(*this) = (CStaticArray<Type>&)aaOriginal;
  aa_aiFreeElements = aaOriginal.aa_aiFreeElements;
  aa_ctAllocationStep = aaOriginal.aa_ctAllocationStep;
}


#endif  /* include-once check. */

