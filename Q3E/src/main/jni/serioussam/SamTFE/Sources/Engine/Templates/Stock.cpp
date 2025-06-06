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

#include <Engine/Base/Stream.h>

#include <Engine/Templates/DynamicContainer.cpp>

/*
 * Default constructor.
 */

CStock_TYPE::CStock_TYPE(void)
{
  st_ntObjects.SetAllocationParameters(50, 2, 2);
}

/*
 * Destructor.
 */

CStock_TYPE::~CStock_TYPE(void)
{
  // free all unused elements of the stock
  FreeUnused();
}

/*
 * Obtain an object from stock - loads if not loaded.
 */

TYPE *CStock_TYPE::Obtain_t(const CTFileName &fnmFileName)
{
  // find stocked object with same name
  TYPE *pExisting = st_ntObjects.Find(fnmFileName);
  
  // if found
  if (pExisting!=NULL) {
    // mark that it is used once again
    pExisting->MarkUsed();
    // return its pointer
    return pExisting;
  }

  /* if not found, */

  // create new stock object
  TYPE *ptNew = new TYPE;
  ptNew->ser_FileName = fnmFileName;
  st_ctObjects.Add(ptNew);
  st_ntObjects.Add(ptNew);

  // load it
  try {
    ptNew->Load_t(fnmFileName);
  } catch (const char *) {
    st_ctObjects.Remove(ptNew);
    st_ntObjects.Remove(ptNew);
    delete ptNew;
    throw;
  }

  // mark that it is used for the first time
  //ASSERT(!ptNew->IsUsed());
  ptNew->MarkUsed();

  // return the pointer to the new one
  return ptNew;
}

/*
 * Release an object when not needed any more.
 */

void CStock_TYPE::Release(TYPE *ptObject)
{
  // mark that it is used one less time
  ptObject->MarkUnused();
  // if it is not used at all any more and should be freed automatically
  if (!ptObject->IsUsed() && ptObject->IsAutoFreed()) {
    // remove it from stock
    st_ctObjects.Remove(ptObject);
    st_ntObjects.Remove(ptObject);
    delete ptObject;
  }
}

// free all unused elements of the stock

void CStock_TYPE::FreeUnused(void)
{
  BOOL bAnyRemoved;
  // repeat
  do {
    // create container of objects that should be freed
    CDynamicContainer<TYPE> ctToFree;
    {FOREACHINDYNAMICCONTAINER(st_ctObjects, TYPE, itt) {
      if (!itt->IsUsed()) {
        ctToFree.Add(itt);
      }
    }}
    bAnyRemoved = ctToFree.Count()>0;
    // for each object that should be freed
    {FOREACHINDYNAMICCONTAINER(ctToFree, TYPE, itt) {
      st_ctObjects.Remove(itt);
      st_ntObjects.Remove(itt);
      delete (&*itt);
    }}

  // as long as there is something to remove
  } while (bAnyRemoved);

}
// calculate amount of memory used by all objects in the stock

SLONG CStock_TYPE::CalculateUsedMemory(void)
{
  SLONG slUsedTotal = 0;
  {FOREACHINDYNAMICCONTAINER(st_ctObjects, TYPE, itt) {
    SLONG slUsedByObject = itt->GetUsedMemory();
    if (slUsedByObject<0) {
      return -1;
    }
    slUsedTotal+=slUsedByObject;
  }}

  return slUsedTotal;
}

// dump memory usage report to a file
void CStock_TYPE::DumpMemoryUsage_t(CTStream &strm) // throw char *
{
  CTString strLine;
  //SLONG slUsedTotal = 0;
  {FOREACHINDYNAMICCONTAINER(st_ctObjects, TYPE, itt) {
    SLONG slUsedByObject = itt->GetUsedMemory();
    if (slUsedByObject<0) {
      strm.PutLine_t("Error!");
      return;
    }
    strLine.PrintF("%7.1fk %s(%d) %s", 
      slUsedByObject/1024.0f, (const char*)(itt->GetName()), itt->GetUsedCount(), (const char *) itt->GetDescription());
    strm.PutLine_t(strLine);
  }}
}

// get number of total elements in stock
INDEX CStock_TYPE::GetTotalCount(void)
{
  return st_ctObjects.Count();
}

// get number of used elements in stock

INDEX CStock_TYPE::GetUsedCount(void)
{
  INDEX ctUsed = 0;
  {FOREACHINDYNAMICCONTAINER(st_ctObjects, TYPE, itt) {
    if (itt->IsUsed()) {
      ctUsed++;
    }
  }}
  return ctUsed;
}
