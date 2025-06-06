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


#if !defined(TYPE) || !defined(CStock_TYPE) || !defined(CNameTable_TYPE)
#error
#endif

#include <Engine/Templates/DynamicContainer.h>

/*
 * Template for stock of some kind of objects that can be saved and loaded.
 */
class CStock_TYPE {
public:
  CDynamicContainer<TYPE> st_ctObjects;   // objects on stock
  CNameTable_TYPE st_ntObjects;  // name table for fast lookup

public:
  /* Default constructor. */
  CStock_TYPE(void);
  /* Destructor. */
  ~CStock_TYPE(void);

  /* Obtain an object from stock - loads if not loaded. */
  ENGINE_API TYPE *Obtain_t(const CTFileName &fnmFileName); // throw char *
  /* Release an object when not needed any more. */
  ENGINE_API void Release(TYPE *ptObject);
  // free all unused elements of the stock
  ENGINE_API void FreeUnused(void);
  // calculate amount of memory used by all objects in the stock
  SLONG CalculateUsedMemory(void);
  // dump memory usage report to a file
  void DumpMemoryUsage_t(CTStream &strm); // throw char *
  // get number of total elements in stock
  INDEX GetTotalCount(void);
  // get number of used elements in stock
  INDEX GetUsedCount(void);
};
