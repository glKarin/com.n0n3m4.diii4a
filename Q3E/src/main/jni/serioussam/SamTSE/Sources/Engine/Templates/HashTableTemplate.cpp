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

#include <Engine/Base/Console.h>
#include <math.h>
#include <Engine/Base/Translation.h>

// default constructor
CHashTable_TYPE::CHashTable_TYPE()
{
  ht_ctCompartments = 0;
  ht_ctSlotsPerComp = 0;
  ht_ctSlotsPerCompStep = 0;
  ht_GetItemKey = NULL;
  ht_GetItemValue = NULL;
}

// destructor -- frees all memory
CHashTable_TYPE::~CHashTable_TYPE(void)
{
}

// remove all slots, and reset the nametable to initial (empty) state, keeps the callback functions
void CHashTable_TYPE::Clear(void)
{
  ht_ctCompartments = 0;
  ht_ctSlotsPerComp = 0;
  ht_ctSlotsPerCompStep = 0;
  ht_ahtsSlots.Clear();
}


// internal finding, returns pointer to the the item
CHashTableSlot_TYPE *CHashTable_TYPE::FindSlot(ULONG ulKey, VALUE_TYPE &Value) 
{
  ASSERT(ht_ctCompartments>0 && ht_ctSlotsPerComp>0);

  // find compartment number
  INDEX iComp = ulKey%ht_ctCompartments;
  
  // for each slot in the compartment
  INDEX iSlot = iComp*ht_ctSlotsPerComp;
  for(INDEX iSlotInComp=0; iSlotInComp<ht_ctSlotsPerComp; iSlotInComp++, iSlot++) {
    CHashTableSlot_TYPE *phts = &ht_ahtsSlots[iSlot];
    // if empty
    if (phts->hts_ptElement==NULL) {
      // skip it
      continue;
    }
    // if it has same key
    if (phts->hts_ulKey==ulKey) {
      // if it is same element
      if (ht_GetItemValue(phts->hts_ptElement) == Value) {
        // return it
        return phts;
      }
    }
  }

  // not found
  return NULL;
}


// internal finding, returns the index of the item in the nametable
INDEX CHashTable_TYPE::FindSlotIndex(ULONG ulKey, VALUE_TYPE &Value)
{
  ASSERT(ht_ctCompartments>0 && ht_ctSlotsPerComp>0);

  // find compartment number
  INDEX iComp = ulKey%ht_ctCompartments;
  
  // for each slot in the compartment
  INDEX iSlot = iComp*ht_ctSlotsPerComp;
  for(INDEX iSlotInComp=0; iSlotInComp<ht_ctSlotsPerComp; iSlotInComp++, iSlot++) {
    CHashTableSlot_TYPE *phts = &ht_ahtsSlots[iSlot];
    // if empty
    if (phts->hts_ptElement==NULL) {
      // skip it
      continue;
    }
    // if it has same key
    if (phts->hts_ulKey==ulKey) {
      // if it is same element
      if (ht_GetItemValue(phts->hts_ptElement) == Value) {
        // return it
        return iSlot;
      }
    }
  }

  // not found
  return -1;
}

TYPE* CHashTable_TYPE::GetItemFromIndex(INDEX iIndex)
{
  ASSERT(ht_ctCompartments>0 && ht_ctSlotsPerComp>0);
  ASSERT(iIndex>=0 && iIndex<ht_ctCompartments*ht_ctSlotsPerComp);

  return ht_ahtsSlots[iIndex].hts_ptElement;
}

VALUE_TYPE CHashTable_TYPE::GetValueFromIndex(INDEX iIndex)
{
  ASSERT(ht_ctCompartments>0 && ht_ctSlotsPerComp>0);
  ASSERT(iIndex>=0 && iIndex<ht_ctCompartments*ht_ctSlotsPerComp);

  return ht_GetItemValue(ht_ahtsSlots[iIndex].hts_ptElement);
}

/* Set allocation parameters. */
void CHashTable_TYPE::SetAllocationParameters(INDEX ctCompartments, INDEX ctSlotsPerComp, INDEX ctSlotsPerCompStep)
{
  ASSERT(ht_ctCompartments==0 && ht_ctSlotsPerComp==0 && ht_ctSlotsPerCompStep==0);
  ASSERT(ctCompartments>0     && ctSlotsPerComp>0     && ctSlotsPerCompStep>0    );

  ht_ctCompartments = ctCompartments;
  ht_ctSlotsPerComp = ctSlotsPerComp;
  ht_ctSlotsPerCompStep = ctSlotsPerCompStep;

  ht_ahtsSlots.New(ht_ctCompartments*ht_ctSlotsPerComp);
}


void CHashTable_TYPE::SetCallbacks(ULONG (*GetItemKey)(VALUE_TYPE &Item), ULONG (*GetItemValue)(TYPE* Item))
{
  ASSERT(GetItemKey!=NULL);
  ASSERT(GetItemValue!=NULL);

  ht_GetItemKey = GetItemKey;
  ht_GetItemValue = GetItemValue;
}
// find an object by name
TYPE *CHashTable_TYPE::Find(VALUE_TYPE &Value)
{
  ASSERT(ht_ctCompartments>0 && ht_ctSlotsPerComp>0);

  CHashTableSlot_TYPE *phts = FindSlot(ht_GetItemKey(Value), Value);
  if (phts==NULL) return NULL;
  return phts->hts_ptElement;
}


// find an object by name, return it's index
INDEX CHashTable_TYPE::FindIndex(VALUE_TYPE &Value)
{
  ASSERT(ht_ctCompartments>0 && ht_ctSlotsPerComp>0);

  return FindSlotIndex(ht_GetItemKey(Value), Value);
}


// expand the name table to next step
void CHashTable_TYPE::Expand(void)
{
  ASSERT(ht_ctCompartments>0 && ht_ctSlotsPerComp>0);

  // if we are here -> the compartment has overflowed
  ASSERT(ht_ctSlotsPerCompStep>0);

  // move the array of slots
  CStaticArray<CHashTableSlot_TYPE > ahtsSlotsOld;
  ahtsSlotsOld.MoveArray(ht_ahtsSlots);

  // allocate new bigger array
  INDEX ctOldSlotsPerComp = ht_ctSlotsPerComp;
  ht_ctSlotsPerComp+=ht_ctSlotsPerCompStep;
  ht_ahtsSlots.New(ht_ctSlotsPerComp*ht_ctCompartments);

  // for each compartment
  for(INDEX iComp =0; iComp<ht_ctCompartments; iComp++) {
    // for each old slot in compartment
    for(INDEX iSlotInComp=0; iSlotInComp<ctOldSlotsPerComp; iSlotInComp++) {
      CHashTableSlot_TYPE &htsOld = ahtsSlotsOld[iSlotInComp+iComp*ctOldSlotsPerComp];
      CHashTableSlot_TYPE &htsNew = ht_ahtsSlots[iSlotInComp+iComp*ht_ctSlotsPerComp];
      // if it is used
      if (htsOld.hts_ptElement!=NULL) {
        // copy it to new array
        htsNew.hts_ptElement = htsOld.hts_ptElement;
        htsNew.hts_ulKey     = htsOld.hts_ulKey;
      }
    }
  }
}

static BOOL _bExpanding = FALSE;  // check to prevent recursive expanding

// add a new object
void CHashTable_TYPE::Add(TYPE *ptNew)
{
  ASSERT(ht_ctCompartments>0 && ht_ctSlotsPerComp>0);

  VALUE_TYPE Value = ht_GetItemValue(ptNew);
  ULONG ulKey = ht_GetItemKey(Value);

  // find compartment number
  INDEX iComp = ulKey%ht_ctCompartments;
  
  // for each slot in the compartment
  INDEX iSlot = iComp*ht_ctSlotsPerComp;
  for(INDEX iSlotInComp=0; iSlotInComp<ht_ctSlotsPerComp; iSlotInComp++, iSlot++) {
    CHashTableSlot_TYPE *phts = &ht_ahtsSlots[iSlot];
    // if it is empty
    if (phts->hts_ptElement==NULL) {
      // put it here
      phts->hts_ulKey = ulKey;
      phts->hts_ptElement = ptNew;
      return;
    }
    // must not already exist
    //ASSERT(phts->hts_ptElement->GetName()!=ptNew->GetName());
  }

  // if we are here -> the compartment has overflowed

  // expand the name table to next step
  ASSERT(!_bExpanding);
  _bExpanding = TRUE;
  Expand();
  // add the new element
  Add(ptNew);
  _bExpanding = FALSE;
}


// remove an object
void CHashTable_TYPE::Remove(TYPE *ptOld)
{
  ASSERT(ht_ctCompartments>0 && ht_ctSlotsPerComp>0);
  // find its slot
  VALUE_TYPE Value = ht_GetItemValue(ptOld);
  CHashTableSlot_TYPE *phts = FindSlot(ht_GetItemKey(Value), Value);
  if( phts!=NULL) {
    // mark slot as unused
    ASSERT( phts->hts_ptElement==ptOld);
    phts->hts_ptElement = NULL;
  }
}


// remove an object
void CHashTable_TYPE::RemoveAll()
{
  ASSERT(ht_ctCompartments>0 && ht_ctSlotsPerComp>0);

  for (INDEX iComp=0;iComp<ht_ctCompartments;iComp++) {
    // for each slot in the compartment
    INDEX iSlot = iComp*ht_ctSlotsPerComp;

    for(INDEX iSlotInComp=0; iSlotInComp<ht_ctSlotsPerComp; iSlotInComp++, iSlot++) {
      // if it is not empty
      CHashTableSlot_TYPE &hts = ht_ahtsSlots[iSlot];
      if (ht_ahtsSlots[iSlot].hts_ptElement!=NULL) {
        ht_ahtsSlots[iSlot].hts_ptElement = NULL;
      }
    }
  }
  return;
}

// remove all objects but keep slots
void CHashTable_TYPE::Reset(void)
{
  for(INDEX iSlot=0; iSlot<ht_ahtsSlots.Count(); iSlot++) {
    ht_ahtsSlots[iSlot].Clear();
  }
}



void CHashTable_TYPE::ReportEfficiency()
{
  DOUBLE dSum  = 0;
  DOUBLE dSum2 = 0;
  ULONG  ulCount = 0;

  for (INDEX iComp=0;iComp<ht_ctCompartments;iComp++) {
    INDEX iCount = 0;
    for (INDEX iSlot=iComp*ht_ctSlotsPerComp;iSlot<(iComp+1)*ht_ctSlotsPerComp;iSlot++) {
      if(ht_ahtsSlots[iSlot].hts_ptElement != NULL) {
        iCount++;
        ulCount++;
      }
    }
    dSum+=iCount;
    dSum2+=iCount*iCount;
  }

  DOUBLE dFullPercent,dAvg,dStDev;
  dFullPercent = (double) ulCount/(ht_ctCompartments*ht_ctSlotsPerComp); // percentage of full slots in the hash table
  dAvg = dSum/ht_ctCompartments; // average number of full slots per compartement
  dStDev = sqrt((dSum2-2*dSum*dAvg+ulCount*dAvg*dAvg)/(ulCount-1));

  CPrintF(TRANSV("Hash table efficiency report:\n"));
  CPrintF(TRANSV("  Compartements: %ld,  Slots per compartement: %ld,  Full slots: %ld\n"),ht_ctCompartments,ht_ctSlotsPerComp,ulCount);
  CPrintF(TRANSV("  Percentage of full slots: %5.2f%%,  Average full slots per compartement: %5.2f \n"),dFullPercent*100,dAvg);
  CPrintF(TRANSV("  Standard deviation is: %5.2f\n"),dStDev);
}

