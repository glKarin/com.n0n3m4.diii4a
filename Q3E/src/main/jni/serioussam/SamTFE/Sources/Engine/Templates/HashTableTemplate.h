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

#if !defined(TYPE) || !defined(VALUE_TYPE) || !defined(CHashTableSlot_TYPE) || !defined(CHashTable_TYPE)
#error
#endif

#include <Engine/Templates/StaticArray.h>

class CHashTableSlot_TYPE {
public:
  ULONG hts_ulKey;      // hashing key
  TYPE *hts_ptElement;  // the element inhere
  CHashTableSlot_TYPE(void)  { hts_ptElement = NULL; };
  void Clear(void) { hts_ptElement = NULL; };
};

/*
 * Template class for storing pointers to objects for fast access by name.
 */
class CHashTable_TYPE {
// implementation:
public:
  INDEX ht_ctCompartments;    // number of compartments in table
  INDEX ht_ctSlotsPerComp;    // number of slots in one compartment
  INDEX ht_ctSlotsPerCompStep;    // allocation step for number of slots in one compartment
  CStaticArray<CHashTableSlot_TYPE> ht_ahtsSlots;  // all slots are here

  ULONG (*ht_GetItemKey)(VALUE_TYPE &Value);
  VALUE_TYPE (*ht_GetItemValue)(TYPE* Item);

  // internal finding, returns pointer to the the slot 
  CHashTableSlot_TYPE *FindSlot(ULONG ulKey, VALUE_TYPE &Value);
  // internal finding, returns the index of the item in the nametable
  INDEX FindSlotIndex(ULONG ulKey, VALUE_TYPE &Value);
  // get the item stored in the hashtable by it's index
  TYPE* GetItemFromIndex(INDEX iIndex);
  // get the value of the item stored in the hashtable by it's index
  VALUE_TYPE GetValueFromIndex(INDEX iIndex);
  // expand the hash table to next step
  void Expand(void);

// interface:
public:
  // default constructor
  CHashTable_TYPE(void);
  // destructor -- frees all memory
  ~CHashTable_TYPE(void);
  // remove all slots, and reset the nametable to initial (empty) state
  void Clear(void);

  /* Set allocation parameters. */
  void SetAllocationParameters(INDEX ctCompartments, INDEX ctSlotsPerComp, INDEX ctSlotsPerCompStep);
  // set callbacks
  void SetCallbacks(ULONG (*GetItemKey)(VALUE_TYPE &Item), VALUE_TYPE (*GetItemValue)(TYPE* Item));

  // find an object by value, return a pointer to it
  TYPE* Find(VALUE_TYPE &Value);
  // find an object by value, return it's index
  INDEX FindIndex(VALUE_TYPE &Value);
  // add a new object
  void Add(TYPE *ptNew);
  // remove an object
  void Remove(TYPE *ptOld);
  // remove an object
  void RemoveAll();

  // remove all objects but keep slots
  void Reset(void);

  // get estimated efficiency of the hashtable
  void ReportEfficiency(void);
};
