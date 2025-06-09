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

#ifndef SE_INCL_DYNAMICSTACKARRAY_CPP
#define SE_INCL_DYNAMICSTACKARRAY_CPP
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/DynamicStackArray.h>
#include <Engine/Templates/DynamicArray.cpp>

/*
 * Default constructor.
 */
template<class Type>
inline CDynamicStackArray<Type>::CDynamicStackArray(void) : CDynamicArray<Type>() {
  da_ctUsed=0;
  da_ctAllocationStep = 256;
  // lock the array on construction
  CDynamicArray<Type>::Lock();
}

/*
 * Destructor.
 */
template<class Type>
inline CDynamicStackArray<Type>::~CDynamicStackArray(void) {
  // lock the array on destruction
  CDynamicArray<Type>::Unlock();
};

/* Destroy all objects, and reset the array to initial (empty) state. */
template<class Type>
inline void CDynamicStackArray<Type>::Clear(void) {
  CDynamicArray<Type>::Clear(); da_ctUsed = 0; 
}

/*
 * Set how many elements to allocate when stack overflows.
 */
template<class Type>
inline void CDynamicStackArray<Type>::SetAllocationStep(INDEX ctStep) 
{
  ASSERT(ctStep>0);
  da_ctAllocationStep = ctStep;
};

/*
 * Add new object(s) on top of stack.
 */
template<class Type>
inline Type &CDynamicStackArray<Type>::Push(void) {
  // if there are no free elements in the array
  if (CDynamicArray<Type>::Count()-da_ctUsed<1) {
    // alocate a new block
    CDynamicArray<Type>::New(da_ctAllocationStep);
  }
  // get the new element
  da_ctUsed++;
  ASSERT(da_ctUsed <= CDynamicArray<Type>::Count());
  return CDynamicArray<Type>::operator[](da_ctUsed-1);
}
template<class Type>
inline Type *CDynamicStackArray<Type>::Push(INDEX ct) {
  // if there are no free elements in the array
  while(CDynamicArray<Type>::Count()-da_ctUsed<ct) {
    // alocate a new block
    CDynamicArray<Type>::New(da_ctAllocationStep);
  }
  // get new elements
  da_ctUsed+=ct;
  ASSERT(da_ctUsed <= CDynamicArray<Type>::Count());
  return &CDynamicArray<Type>::operator[](da_ctUsed-ct);
}

/*
 * Remove all objects from stack, but keep stack space.
 */
template<class Type>
inline void CDynamicStackArray<Type>::PopAll(void) {
  // if there is only one block allocated
  if ( this->da_BlocksList.IsEmpty()
    || &this->da_BlocksList.Head()==&this->da_BlocksList.Tail()) {
    // just clear the counter
    da_ctUsed = 0;

  // if there is more than one block allocated
  } else {
    // remember how much was allocated, rounded up to allocation step
    INDEX ctUsedBefore = CDynamicArray<Type>::Count();
    // free all memory
    CDynamicArray<Type>::Clear();
    // allocate one big block
    CDynamicArray<Type>::New(ctUsedBefore);
    da_ctUsed = 0;
  }
}

/*
 * Random access operator.
 */
template<class Type>
inline Type &CDynamicStackArray<Type>::operator[](INDEX i) {
  ASSERT(this!=NULL);
  ASSERT(i<da_ctUsed);     // check bounds
  return CDynamicArray<Type>::operator[](i);
}
template<class Type>
inline const Type &CDynamicStackArray<Type>::operator[](INDEX i) const {
  ASSERT(this!=NULL);
  ASSERT(i<da_ctUsed);     // check bounds
  return CDynamicArray<Type>::operator[](i);
}

/*
 * Get number of elements in array.
 */
template<class Type>
INDEX CDynamicStackArray<Type>::Count(void) const {
  ASSERT(this!=NULL);
  return da_ctUsed;
}

/*
 * Get index of a member from it's pointer
 */
template<class Type>
INDEX CDynamicStackArray<Type>::Index(Type *ptMember) {
  ASSERT(this!=NULL);
  INDEX i = CDynamicArray<Type>::Index(ptMember);
  ASSERTMSG(i<da_ctUsed, "CDynamicStackArray<>::Index(): Not a member of this array!");
  return i;
}

/*
 * Get array of pointers to elements (used for sorting elements by sorting pointers).
 */
template<class Type>
Type **CDynamicStackArray<Type>::GetArrayOfPointers(void)
{
  return this->da_Pointers;
}

/*
 * Assignment operator.
 */
template<class Type>
CDynamicStackArray<Type> &CDynamicStackArray<Type>::operator=(CDynamicStackArray<Type> &arOriginal)
{
  ASSERT(this!=NULL);
  ASSERT(&arOriginal!=NULL);
  ASSERT(this!=&arOriginal);

  // copy stack arrays
  CDynamicArray<Type>::operator=(arOriginal);
  // copy used count
  da_ctUsed = arOriginal.da_ctUsed;

  return *this;
}


#endif  /* include-once check. */

