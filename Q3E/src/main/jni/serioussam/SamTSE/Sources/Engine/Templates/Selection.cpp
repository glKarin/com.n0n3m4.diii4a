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

#ifndef SE_INCL_SELECTION_CPP
#define SE_INCL_SELECTION_CPP
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/Selection.h>
#include <Engine/Templates/DynamicContainer.cpp>

/*
 * Select one object.
 */
template<class cType, unsigned long ulFlag>
void CSelection<cType, ulFlag>::Select(cType &tToSelect)
{
  // if the object is not yet selected
  if (!tToSelect.IsSelected(ulFlag)) {
    // select it
    tToSelect.Select(ulFlag);
    // add it to this container
    this->Add(&tToSelect);

  // if the object is already selected
  } else {
    ASSERTALWAYS("Object already selected!");
  }
}

/*
 * Deselect one object.
 */
template<class cType, unsigned long ulFlag>
void CSelection<cType, ulFlag>::Deselect(cType &tToSelect)
{
  // if the object is selected
  if (tToSelect.IsSelected(ulFlag)) {
    // deselect it
    tToSelect.Deselect(ulFlag);
    // remove it from this container
    this->Remove(&tToSelect);

  // if the object is not selected
  } else {
    ASSERTALWAYS("Object is not selected!");
  }
}

/*
 * Test if one object is selected.
 */
template<class cType, unsigned long ulFlag>
BOOL CSelection<cType, ulFlag>::IsSelected(cType &tToSelect)
{
  // test if the object is selected
  return tToSelect.IsSelected(ulFlag);
}

/*
 * Deselect all objects.
 */
template<class cType, unsigned long ulFlag>
void CSelection<cType, ulFlag>::Clear(void)
{
  // for all objects in the container
  FOREACHINDYNAMICCONTAINER(*this, cType, itObject) {
    // object must be allocated and valid
  #ifdef _MSC_VER
    ASSERT(_CrtIsValidPointer(&*itObject, sizeof(cType), TRUE));
    /*
    ASSERT(_CrtIsValidHeapPointer(&*itObject));
    ASSERT(_CrtIsMemoryBlock(&*itObject, sizeof(cType), NULL, NULL, NULL ));
    */
  #endif

    // deselect it
    itObject->Deselect(ulFlag);
  }
  // clear the entire container at once
  CDynamicContainer<cType>::Clear();
}

template<class cType, unsigned long ulFlag>
cType *CSelection<cType, ulFlag>::GetFirstInSelection(void)
{
  if( this->Count() == 0)
  {
    return NULL;
  }
  return (cType *) &CDynamicContainer<cType>::GetFirst();
}


#endif  /* include-once check. */

