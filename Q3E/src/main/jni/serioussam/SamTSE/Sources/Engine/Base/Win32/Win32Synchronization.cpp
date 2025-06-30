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

#include "Engine/StdH.h"

#include <Engine/Base/Synchronization.h>

 
/*
This is implementation of OPTEX (optimized mutex), 
originally from MSDN Periodicals 1996, by Jeffrey Richter.

It is updated for clearer comments, shielded with tons of asserts,
and modified to support TryToEnter() function. The original version
had timeout parameter, bu it didn't work.

NOTES: 
- TryToEnter() was not tested with more than one thread, and perhaps
  there might be some problems with the final decrementing and eventual event resetting
  when lock fails. Dunno.

- take care to center the lock tests around 0 (-1 means not locked). that is
  neccessary because win95 returns only <0, ==0 and >0 results from interlocked 
  functions, so testing against any other number than 0 doesn't work.
*/

// The opaque OPTEX data structure
typedef struct {
   LONG   lLockCount; // note: must center all tests around 0 for win95 compatibility!
   DWORD  dwThreadId;
   LONG   lRecurseCount;
   HANDLE hEvent;
} OPTEX, *POPTEX;

_declspec(thread) INDEX _iLastLockedMutex = 0;

BOOL OPTEX_Initialize (POPTEX poptex) {
  
  poptex->lLockCount = -1;   // No threads have enterred the OPTEX
  poptex->dwThreadId = 0;    // The OPTEX is unowned
  poptex->lRecurseCount = 0; // The OPTEX is unowned
  poptex->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  return(poptex->hEvent != NULL);  // TRUE if the event is created
}

VOID OPTEX_Delete (POPTEX poptex) {

   // No in-use check
   CloseHandle(poptex->hEvent);  // Close the event
}

INDEX OPTEX_Enter (POPTEX poptex) 
{
  
  DWORD dwThreadId = GetCurrentThreadId();  // The calling thread's ID
  
  // increment lock counter
  INDEX ctLocked = InterlockedIncrement(&poptex->lLockCount);
  ASSERT(poptex->lLockCount>=0);

  // if this is first thread that entered
  if (ctLocked == 0) {
    
    // mark that we own it, exactly once
    ASSERT(poptex->dwThreadId==0);
    ASSERT(poptex->lRecurseCount==0);
    poptex->dwThreadId = dwThreadId;
    poptex->lRecurseCount = 1;
    
  // if already owned
  } else {
    
    // if owned by this thread
    if (poptex->dwThreadId == dwThreadId) {
      // just mark that we own it once more
      poptex->lRecurseCount++;
      ASSERT(poptex->lRecurseCount>1);
      
    // if owned by some other thread
    } else {
      
      // wait for the owning thread to release the OPTEX
      DWORD dwRet = WaitForSingleObject(poptex->hEvent, INFINITE);
      ASSERT(dwRet == WAIT_OBJECT_0);
  
      // mark that we own it, exactly once
      ASSERT(poptex->dwThreadId==0);
      ASSERT(poptex->lRecurseCount==0);
      poptex->dwThreadId = dwThreadId;
      poptex->lRecurseCount = 1;
    }
  }
  ASSERT(poptex->lRecurseCount>=1);
  ASSERT(poptex->lLockCount>=0);
  return poptex->lRecurseCount;
}

INDEX OPTEX_TryToEnter (POPTEX poptex) 
{
  ASSERT(poptex->lLockCount>=-1);
  DWORD dwThreadId = GetCurrentThreadId();  // The calling thread's ID
  
  // increment lock counter
  INDEX ctLocked = InterlockedIncrement(&poptex->lLockCount);
  ASSERT(poptex->lLockCount>=0);

  // if this is first thread that entered
  if (ctLocked == 0) {
    
    // mark that we own it, exactly once
    ASSERT(poptex->dwThreadId==0);
    ASSERT(poptex->lRecurseCount==0);
    poptex->dwThreadId = dwThreadId;
    poptex->lRecurseCount = 1;
    // lock succeeded
    return poptex->lRecurseCount;
    
  // if already owned
  } else {
    
    // if owned by this thread
    if (poptex->dwThreadId == dwThreadId) {
      
      // just mark that we own it once more
      poptex->lRecurseCount++;
      ASSERT(poptex->lRecurseCount>=1);

      // lock succeeded
      return poptex->lRecurseCount;
      
    // if owned by some other thread
    } else {

      // give up taking it
      INDEX ctLocked = InterlockedDecrement(&poptex->lLockCount);
      ASSERT(poptex->lLockCount>=-1);

      // if unlocked in the mean time
      if (ctLocked<0) {
        // NOTE: this has not been tested!
        // ignore sent the signal
        ResetEvent(poptex->hEvent);
      }

      // lock failed
      return 0;
    }
  }
}

INDEX OPTEX_Leave (POPTEX poptex) 
{

  ASSERT(poptex->dwThreadId==GetCurrentThreadId());
  
  // we own in one time less
  poptex->lRecurseCount--;
  ASSERT(poptex->lRecurseCount>=0);
  INDEX ctResult = poptex->lRecurseCount;

  // if more multiple locks from this thread
  if (poptex->lRecurseCount > 0) {
    
    // just decrement the lock count
    InterlockedDecrement(&poptex->lLockCount);
    ASSERT(poptex->lLockCount>=-1);
    
  // if no more multiple locks from this thread
  } else {
    
    // mark that this thread doesn't own it
    poptex->dwThreadId = 0;
    // decrement the lock count
    INDEX ctLocked = InterlockedDecrement(&poptex->lLockCount);
    ASSERT(poptex->lLockCount>=-1);
    // if some threads are waiting for it
    if ( ctLocked >= 0) {
      // wake one of them
      SetEvent(poptex->hEvent);
    }
  }
  
  ASSERT(poptex->lRecurseCount>=0);
  ASSERT(poptex->lLockCount>=-1);
  return ctResult;
}

// these are just wrapper classes for locking/unlocking

CTCriticalSection::CTCriticalSection(void)
{
  // index must be set before using the mutex
  cs_iIndex = -2;
  cs_pvObject = new OPTEX;
  OPTEX_Initialize((OPTEX*)cs_pvObject);
}
CTCriticalSection::~CTCriticalSection(void)
{
  OPTEX_Delete((OPTEX*)cs_pvObject);
  delete (OPTEX*)cs_pvObject;
}
INDEX CTCriticalSection::Lock(void)
{
  return OPTEX_Enter((OPTEX*)cs_pvObject);
}
INDEX CTCriticalSection::TryToLock(void)
{
  return OPTEX_TryToEnter((OPTEX*)cs_pvObject);
}
INDEX CTCriticalSection::Unlock(void)
{
  return OPTEX_Leave((OPTEX*)cs_pvObject);
}

CTSingleLock::CTSingleLock(CTCriticalSection *pcs, BOOL bLock) : sl_cs(*pcs)
{
  // initially not locked
  sl_bLocked = FALSE;
  sl_iLastLockedIndex = -2;
  // critical section must have index assigned
  ASSERT(sl_cs.cs_iIndex>=1||sl_cs.cs_iIndex==-1);
  // if should lock immediately
  if (bLock) {
    Lock();
  }
}
CTSingleLock::~CTSingleLock(void)
{
  // if locked
  if (sl_bLocked) {
    // unlock
    Unlock();
  }
}
void CTSingleLock::Lock(void)
{
  // must not be locked
  ASSERT(!sl_bLocked);
  ASSERT(sl_iLastLockedIndex==-2);

  // if not locked
  if (!sl_bLocked) {
    // lock
    INDEX ctLocks = sl_cs.Lock();
    // if this mutex was not locked already
    if (ctLocks==1) {
      // check that locking in given order
      if (sl_cs.cs_iIndex!=-1) {
        ASSERT(_iLastLockedMutex<sl_cs.cs_iIndex);
        sl_iLastLockedIndex = _iLastLockedMutex;
        _iLastLockedMutex = sl_cs.cs_iIndex;
      }
    }
  }
  sl_bLocked = TRUE;
}

BOOL CTSingleLock::TryToLock(void)
{
  // must not be locked
  ASSERT(!sl_bLocked);
  // if not locked
  if (!sl_bLocked) {
    // if can lock
    INDEX ctLocks = sl_cs.TryToLock();
    if (ctLocks>=1) {
      sl_bLocked = TRUE;

      // if this mutex was not locked already
      if (ctLocks==1) {
        // check that locking in given order
        if (sl_cs.cs_iIndex!=-1) {
          ASSERT(_iLastLockedMutex<sl_cs.cs_iIndex);
          sl_iLastLockedIndex = _iLastLockedMutex;
          _iLastLockedMutex = sl_cs.cs_iIndex;
        }
      }
    }
  }
  return sl_bLocked;
}
BOOL CTSingleLock::IsLocked(void)
{
  return sl_bLocked;
}

void CTSingleLock::Unlock(void)
{
  // must be locked
  ASSERT(sl_bLocked);
  // if locked
  if (sl_bLocked) {
    // unlock
    INDEX ctLocks = sl_cs.Unlock();
    // if unlocked completely
    if (ctLocks==0) {
      // check that unlocking in exact reverse order
      if (sl_cs.cs_iIndex!=-1) {
        ASSERT(_iLastLockedMutex==sl_cs.cs_iIndex);
        _iLastLockedMutex = sl_iLastLockedIndex;
        sl_iLastLockedIndex = -2;
      }
    }
  }
  sl_bLocked = FALSE;
}

