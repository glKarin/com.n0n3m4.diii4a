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

#ifndef SE_INCL_DYNAMICCONTAINER_CPP
#define SE_INCL_DYNAMICCONTAINER_CPP
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/DynamicContainer.h>
#include <Engine/Base/Memory.h>
#include <Engine/Templates/StaticStackArray.cpp>

/*
 * Default constructor.
 */
template<class Type>
CDynamicContainer<Type>::CDynamicContainer(void) {
#if CHECKARRAYLOCKING
  // not locked
  dc_LockCt = 0;
#endif
}

/*
 * Copy constructor.
 */
template<class Type>
CDynamicContainer<Type>::CDynamicContainer(CDynamicContainer<Type> &dcOriginal)
{
#if CHECKARRAYLOCKING
  // not locked
  dc_LockCt = 0;
#endif

  // call assignment operator
  (*this) = dcOriginal;
}
/*
 * Destructor -- removes all objects.
 */
template<class Type>
CDynamicContainer<Type>::~CDynamicContainer(void) {
  Clear();
}

/*
 * Remove all objects, and reset the array to initial (empty) state.
 */
template<class Type>
void CDynamicContainer<Type>::Clear(void) {
  ASSERT(this!=NULL);
  CStaticStackArray<Type *>::Clear();
}

/*
 * Add a given object to container.
 */
template<class Type>
void CDynamicContainer<Type>::Add(Type *ptNewObject)
{
  // set the new pointer
  this->Push() = ptNewObject;
}

/*
 * Insert a given object to container at specified index.
*/
 template<class Type>
void CDynamicContainer<Type>::Insert(Type * ptNewObject, const INDEX iPos)
{
	// get number of member that need moving and add new one
	const INDEX ctMovees = CStaticStackArray<Type*>::Count() - iPos;
	CStaticStackArray<Type*>::Push();
	// move all members after insert position one place up
	Type **pptInsertAt = this->sa_Array + iPos;
	Type **pptMoveTo = pptInsertAt + 1;
	memmove(pptMoveTo, pptInsertAt, sizeof(Type*)*ctMovees);
	// store pointer to newly inserted member at specified position
	*pptInsertAt = ptNewObject;
}

/*
 * Remove a given object from container.
 */
template<class Type>
void CDynamicContainer<Type>::Remove(Type *ptOldObject)
{
  ASSERT(this!=NULL);
#if CHECKARRAYLOCKING
  // check that not locked for indices
  ASSERT(dc_LockCt == 0);
#endif

  // find its index
  INDEX iMember=GetIndex(ptOldObject);
  // move last pointer here
  this->sa_Array[iMember]=this->sa_Array[this->Count()-1];
  this->Pop();
}

/* Test if a given object is in the container. */
template<class Type>
BOOL CDynamicContainer<Type>::IsMember(Type *ptOldObject)
{
  ASSERT(this!=NULL);
  // slow !!!!
  // check all members
  for (INDEX iMember=0; iMember<this->Count(); iMember++) {
    if(this->sa_Array[iMember]==ptOldObject) {
      return TRUE;
    }
  }
  return FALSE;
}

/*
 * Get pointer to a member from it's index.
 */
template<class Type>
Type *CDynamicContainer<Type>::Pointer(INDEX iMember) {
  ASSERT(this!=NULL);
  // check that index is currently valid
  ASSERT(iMember>=0 && iMember<this->Count());
#if CHECKARRAYLOCKING
  // check that locked for indices
  ASSERT(dc_LockCt>0);
#endif
  return this->sa_Array[iMember];
}
template<class Type>
const Type *CDynamicContainer<Type>::Pointer(INDEX iMember) const {
  ASSERT(this!=NULL);
  // check that index is currently valid
  ASSERT(iMember>=0 && iMember<this->Count());
#if CHECKARRAYLOCKING
  // check that locked for indices
  ASSERT(dc_LockCt>0);
#endif
  return this->sa_Array[iMember];
}

/*
 * Lock for getting indices.
 */
template<class Type>
void CDynamicContainer<Type>::Lock(void) {
  ASSERT(this!=NULL);
#if CHECKARRAYLOCKING
  ASSERT(dc_LockCt>=0);
  // increment lock counter
  dc_LockCt++;
#endif
}

/*
 * Unlock after getting indices.
 */
template<class Type>
void CDynamicContainer<Type>::Unlock(void) {
  ASSERT(this!=NULL);
#if CHECKARRAYLOCKING
  dc_LockCt--;
  ASSERT(dc_LockCt>=0);
#endif
}

/*
 * Get index of a member from it's pointer.
 */
template<class Type>
INDEX CDynamicContainer<Type>::Index(Type *ptMember) {
  ASSERT(this!=NULL);
  // check that locked for indices
#if CHECKARRAYLOCKING
  ASSERT(dc_LockCt>0);
#endif
  return GetIndex(ptMember);
}

/*
 * Get index of a member from it's pointer without locking.
 */
template<class Type>
INDEX CDynamicContainer<Type>::GetIndex(Type *ptMember) {
  ASSERT(this!=NULL);
  // slow !!!!
  // check all members
  for (INDEX iMember=0; iMember<this->Count(); iMember++) {
    if(this->sa_Array[iMember]==ptMember) {
      return iMember;
    }
  }
  ASSERTALWAYS("CDynamicContainer<Type><>::Index(): Not a member of this container!");
  return 0;
}

/* Get first object in container (there must be at least one when calling this). */
template<class Type>
Type &CDynamicContainer<Type>::GetFirst(void)
{
  ASSERT(this->Count()>=1);
  return *this->sa_Array[0];
}

/*
 * Assignment operator.
 */
template<class Type>
CDynamicContainer<Type> &CDynamicContainer<Type>::operator=(CDynamicContainer<Type> &coOriginal)
{
  CStaticStackArray<Type *>::operator=(coOriginal);
  return *this;
}

/*
 * Move all elements of another array into this one.
 */
template<class Type>
void CDynamicContainer<Type>::MoveContainer(CDynamicContainer<Type> &coOther)
{
  ASSERT(this!=NULL && &coOther!=NULL);
  // check that not locked for indices
#if CHECKARRAYLOCKING
  ASSERT(dc_LockCt==0 && coOther.dc_LockCt==0);
#endif
  CStaticStackArray<Type*>::MoveArray(coOther);
}

/////////////////////////////////////////////////////////////////////
// CDynamicContainerIterator<Type>

/*
 * Template class for iterating dynamic array.
 */
template<class Type>
class CDynamicContainerIterator {
private:
  INDEX dci_Index;               // index of current element
  CDynamicContainer<Type> &dci_Array;   // reference to array
public:
  /* Constructor for given array. */
  inline CDynamicContainerIterator(CDynamicContainer<Type> &da);
  /* Destructor. */
  inline ~CDynamicContainerIterator(void);

  /* Move to next object. */
  inline void MoveToNext(void);
  /* Check if finished. */
  inline BOOL IsPastEnd(void);
	
  /* Get current element. */
  Type &Current(void) { return *dci_Array.Pointer(dci_Index); }
  Type &operator*(void) { return *dci_Array.Pointer(dci_Index); }
  operator Type *(void) { return dci_Array.Pointer(dci_Index); }
  Type *operator->(void) { return dci_Array.Pointer(dci_Index); }
};


/*
 * Constructor for given array.
 */
template<class Type>
inline CDynamicContainerIterator<Type>::CDynamicContainerIterator(CDynamicContainer<Type> &da) : dci_Array(da) {
  // lock indices
  dci_Array.Lock();
  dci_Index = 0;
}

/*
 * Destructor.
 */
template<class Type>
inline CDynamicContainerIterator<Type>::~CDynamicContainerIterator(void) {
  // unlock indices
  dci_Array.Unlock();
  dci_Index = -1;
}

/*
 * Move to next object.
 */
template<class Type>
inline void CDynamicContainerIterator<Type>::MoveToNext(void) {
  ASSERT(this!=NULL);
  dci_Index++;
}

/*
 * Check if finished.
 */
template<class Type>
inline BOOL CDynamicContainerIterator<Type>::IsPastEnd(void) {
  ASSERT(this!=NULL);
  return dci_Index>=dci_Array.Count();
}

// iterate whole dynamic container
/* NOTE: The iterator defined by this macro must be destroyed before adding/removing
 * elements in the container. To do so, embed the for loop in additional curly braces.
 */
#define FOREACHINDYNAMICCONTAINER(container, type, iter) \
  for(CDynamicContainerIterator<type> iter(container); !iter.IsPastEnd(); iter.MoveToNext() )


#endif  /* include-once check. */

