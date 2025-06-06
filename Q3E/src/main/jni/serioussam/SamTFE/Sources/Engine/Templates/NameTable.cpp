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


#include <Engine/Templates/StaticArray.cpp>

#if NAMETABLE_CASESENSITIVE==1
  #define COMPARENAMES(a, b) (strcmp(a, b)==0)
#elif NAMETABLE_CASESENSITIVE==0
  #define COMPARENAMES(a, b) (a==b)
#else
  #error "NAMETABLE_CASESENSITIVE not defined"
#endif

// default constructor
CNameTable_TYPE::CNameTable_TYPE(void)
{
  nt_ctCompartments = 0;
  nt_ctSlotsPerComp = 0;
  nt_ctSlotsPerCompStep = 0;
}

// destructor -- frees all memory
CNameTable_TYPE::~CNameTable_TYPE(void)
{
}

// remove all slots, and reset the nametable to initial (empty) state
void CNameTable_TYPE::Clear(void)
{
  nt_ctCompartments = 0;
  nt_ctSlotsPerComp = 0;
  nt_ctSlotsPerCompStep = 0;
  nt_antsSlots.Clear();
}

// internal finding
CNameTableSlot_TYPE *CNameTable_TYPE::FindSlot(ULONG ulKey, const CTString &strName)
{
  ASSERT(nt_ctCompartments>0 && nt_ctSlotsPerComp>0);

  // find compartment number
  INDEX iComp = ulKey%nt_ctCompartments;
  
  // for each slot in the compartment
  INDEX iSlot = iComp*nt_ctSlotsPerComp;
  for(INDEX iSlotInComp=0; iSlotInComp<nt_ctSlotsPerComp; iSlotInComp++, iSlot++) {
    CNameTableSlot_TYPE *pnts = &nt_antsSlots[iSlot];
    // if empty
    if (pnts->nts_ptElement==NULL) {
      // skip it
      continue;
    }
    // if it has same key
    if (pnts->nts_ulKey==ulKey) {
      // if it is same element
      if (COMPARENAMES(pnts->nts_ptElement->GetName(), strName)) {
        // return it
        return pnts;
      }
    }
  }

  // not found
  return NULL;
}

/* Set allocation parameters. */
void CNameTable_TYPE::SetAllocationParameters(
  INDEX ctCompartments, INDEX ctSlotsPerComp, INDEX ctSlotsPerCompStep)
{
  ASSERT(nt_ctCompartments==0 && nt_ctSlotsPerComp==0 && nt_ctSlotsPerCompStep==0);
  ASSERT(ctCompartments>0     && ctSlotsPerComp>0     && ctSlotsPerCompStep>0    );

  nt_ctCompartments = ctCompartments;
  nt_ctSlotsPerComp = ctSlotsPerComp;
  nt_ctSlotsPerCompStep = ctSlotsPerCompStep;

  nt_antsSlots.New(nt_ctCompartments*nt_ctSlotsPerComp);
}

// find an object by name
TYPE *CNameTable_TYPE::Find(const CTString &strName)
{
  ASSERT(nt_ctCompartments>0 && nt_ctSlotsPerComp>0);

  CNameTableSlot_TYPE *pnts = FindSlot(strName.GetHash(), strName);
  if (pnts==NULL) return NULL;
  return pnts->nts_ptElement;
}

// expand the name table to next step
void CNameTable_TYPE::Expand(void)
{
  ASSERT(nt_ctCompartments>0 && nt_ctSlotsPerComp>0);

  // if we are here -> the compartment has overflowed
  ASSERT(nt_ctSlotsPerCompStep>0);

  // move the array of slots
  CStaticArray<CNameTableSlot_TYPE > antsSlotsOld;
  antsSlotsOld.MoveArray(nt_antsSlots);

  // allocate new bigger array
  INDEX ctOldSlotsPerComp = nt_ctSlotsPerComp;
  nt_ctSlotsPerComp+=nt_ctSlotsPerCompStep;
  nt_antsSlots.New(nt_ctSlotsPerComp*nt_ctCompartments);

  // for each compartment
  for(INDEX iComp =0; iComp<nt_ctCompartments; iComp++) {
    // for each old slot in compartment
    for(INDEX iSlotInComp=0; iSlotInComp<ctOldSlotsPerComp; iSlotInComp++) {
      CNameTableSlot_TYPE &ntsOld = antsSlotsOld[iSlotInComp+iComp*ctOldSlotsPerComp];
      CNameTableSlot_TYPE &ntsNew = nt_antsSlots[iSlotInComp+iComp*nt_ctSlotsPerComp];
      // if it is used
      if (ntsOld.nts_ptElement!=NULL) {
        // copy it to new array
        ntsNew.nts_ptElement = ntsOld.nts_ptElement;
        ntsNew.nts_ulKey     = ntsOld.nts_ulKey;
      }
    }
  }
}

static BOOL _bExpanding = FALSE;  // check to prevend recursive expanding

// add a new object
void CNameTable_TYPE::Add(TYPE *ptNew)
{
  ASSERT(nt_ctCompartments>0 && nt_ctSlotsPerComp>0);

  ULONG ulKey = ptNew->GetName().GetHash();

  // find compartment number
  INDEX iComp = ulKey%nt_ctCompartments;
  
  // for each slot in the compartment
  INDEX iSlot = iComp*nt_ctSlotsPerComp;
  for(INDEX iSlotInComp=0; iSlotInComp<nt_ctSlotsPerComp; iSlotInComp++, iSlot++) {
    CNameTableSlot_TYPE *pnts = &nt_antsSlots[iSlot];
    // if it is empty
    if (pnts->nts_ptElement==NULL) {
      // put it here
      pnts->nts_ulKey = ulKey;
      pnts->nts_ptElement = ptNew;
      return;
    }
    // must not already exist
    //ASSERT(pnts->nts_ptElement->GetName()!=ptNew->GetName());
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
void CNameTable_TYPE::Remove(TYPE *ptOld)
{
  ASSERT(nt_ctCompartments>0 && nt_ctSlotsPerComp>0);
  // find its slot
  const CTString &strName = ptOld->GetName();
  CNameTableSlot_TYPE *pnts = FindSlot(strName.GetHash(), strName);
  if( pnts!=NULL) {
    // mark slot as unused
    ASSERT( pnts->nts_ptElement==ptOld);
    pnts->nts_ptElement = NULL;
  }
}


// remove all objects but keep slots
void CNameTable_TYPE::Reset(void)
{
  for(INDEX iSlot=0; iSlot<nt_antsSlots.Count(); iSlot++) {
    nt_antsSlots[iSlot].Clear();
  }
}

#undef NAMETABLE_CASESENSITIVE
