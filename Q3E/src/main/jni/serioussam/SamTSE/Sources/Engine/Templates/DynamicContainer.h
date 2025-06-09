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

#ifndef SE_INCL_DYNAMICCONTAINER_H
#define SE_INCL_DYNAMICCONTAINER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/StaticStackArray.h>


/*
 * Template class for a container that holds references to objects of some class.
 */
template<class Type>
class CDynamicContainer : public CStaticStackArray<Type *> {
public:
#if CHECKARRAYLOCKING
  INDEX da_LockCt;          // lock counter for getting indices
#endif

  /* Get index of an object from it's pointer without locking. */
  INDEX GetIndex(Type *ptMember);
public:
  /* Default constructor. */
  CDynamicContainer(void);
  /* Copy constructor. */
  CDynamicContainer(CDynamicContainer<Type> &coOriginal);
  /* Destructor -- frees all memory. */
  ~CDynamicContainer(void);

  /* Add a given object to container. */
  void Add(Type *ptNewObject);
  /* Insert a given object to container at specified index. */
  void Insert(Type *ptNewObject, const INDEX iPos=0);
  /* Remove a given object from container. */
  void Remove(Type *ptOldObject);
  /* Remove all objects, and reset the container to initial (empty) state. */
  void Clear(void);
  /* Test if a given object is in the container. */
  BOOL IsMember(Type *ptOldObject);

  /* Get pointer to a object from it's index. */
  Type *Pointer(INDEX iObject);
  const Type *Pointer(INDEX iObject) const;
  /* Random access operator. */
  inline Type &operator[](INDEX iObject) { return *Pointer(iObject); };
  inline const Type &operator[](INDEX iObject) const { return *Pointer(iObject); };
  /* Assignment operator. */
  CDynamicContainer<Type> &operator=(CDynamicContainer<Type> &coOriginal);
  /* Move all elements of another container into this one. */
  void MoveContainer(CDynamicContainer<Type> &coOther);

  /* Lock for getting indices. */
  void Lock(void);
  /* Unlock after getting indices. */
  void Unlock(void);
  /* Get index of a object from it's pointer. */
  INDEX Index(Type *ptObject);
  /* Get first object in container (there must be at least one when calling this). */
  Type &GetFirst(void);
};



#endif  /* include-once check. */

