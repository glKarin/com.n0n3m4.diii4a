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

#ifndef SE_INCL_STATICSTACKARRAY_H
#define SE_INCL_STATICSTACKARRAY_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/StaticArray.h>

/*
 * Template class for stack-like array with static allocation of objects.
 */
template<class Type>
class CStaticStackArray : public CStaticArray<Type> {
public:
  INDEX sa_UsedCount;         // number of used objects in array
  INDEX sa_ctAllocationStep;  // how many elements to allocate when stack overflows
public:
  /* Default constructor. */
  inline CStaticStackArray(void);
  /* Destructor. */
  inline ~CStaticStackArray(void);

  /* Set how many elements to allocate when stack overflows. */
  inline void SetAllocationStep(INDEX ctStep);
  /* Create a given number of objects. */
  inline void New(INDEX iCount);
  /* Destroy all objects. */
  inline void Delete(void);
  /* Destroy all objects, and reset the array to initial (empty) state. */
  inline void Clear(void);

  /* Add new object(s) on top of stack. */
  inline Type &Push(void);
  inline Type *Push(INDEX ct);
  /* Remove one object from top of stack and return it. */
  inline Type &Pop(void);
  /* Remove objects with higher than the given index from stack, but keep stack space. */
  inline void PopUntil(INDEX iNewTop);
  /* Remove all objects from stack, but keep stack space. */
  inline void PopAll(void);

  /* Random access operator. */
  inline Type &operator[](INDEX iObject);
  inline const Type &operator[](INDEX iObject) const;
  /* Get number of objects in array. */
  INDEX Count(void) const;
  /* Get index of a object from it's pointer. */
  INDEX Index(Type *ptObject);
  /* Move all elements of another array into this one. */
  void MoveArray(CStaticStackArray<Type> &arOther);

  /* Assignment operator. */
  CStaticStackArray<Type> &operator=(const CStaticStackArray<Type> &arOriginal);
};


#endif  /* include-once check. */

