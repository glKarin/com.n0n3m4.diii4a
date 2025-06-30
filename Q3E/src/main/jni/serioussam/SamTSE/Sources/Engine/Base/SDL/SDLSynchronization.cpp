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

#ifdef _DIII4A //karin: no SDL
#include "../Q3E/Q3ESynchronization.cpp"
#else
#include "SDL.h"
#include "SDL_thread.h"

#include "Engine/StdH.h"
#include <Engine/Base/Synchronization.h>

// !!! FIXME: rcg10142001 Most of CTSingleLock is platform-independent.

CTCriticalSection::CTCriticalSection(void)
{
    LockCounter = 0;
    cs_pvObject = (void *) SDL_CreateMutex();
    ASSERT(cs_pvObject != NULL);
}

CTCriticalSection::~CTCriticalSection(void)
{
    SDL_DestroyMutex((SDL_mutex *) cs_pvObject);
}

INDEX CTCriticalSection::Lock(void)
{
    LockCounter++;
    if (LockCounter == 1)
        SDL_LockMutex((SDL_mutex *) cs_pvObject);
    return(LockCounter);
}

INDEX CTCriticalSection::TryToLock(void)
{
    if (LockCounter > 0)  // !!! race condition. Ironic, eh?
        return(0);
    Lock();
    return(1);
}

INDEX CTCriticalSection::Unlock(void)
{
    if (LockCounter > 0)
    {
        LockCounter--;
        if (LockCounter == 0)
            SDL_UnlockMutex((SDL_mutex *) cs_pvObject);
    }

    return(LockCounter);
}

CTSingleLock::CTSingleLock(CTCriticalSection *pcs, BOOL bLock) : sl_cs(*pcs)
{
  // initially not locked
  sl_bLocked = FALSE;
  sl_iLastLockedIndex = -2;
  // critical section must have index assigned
  //ASSERT(sl_cs.cs_iIndex>=1||sl_cs.cs_iIndex==-1);
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
//    if (ctLocks==1) {
//      // check that locking in given order
//      if (sl_cs.cs_iIndex!=-1) {
//        ASSERT(_iLastLockedMutex<sl_cs.cs_iIndex);
//        sl_iLastLockedIndex = _iLastLockedMutex;
//        _iLastLockedMutex = sl_cs.cs_iIndex;
//      }
//    }
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
//      if (ctLocks==1) {
//        // check that locking in given order
//        if (sl_cs.cs_iIndex!=-1) {
//          ASSERT(_iLastLockedMutex<sl_cs.cs_iIndex);
//          sl_iLastLockedIndex = _iLastLockedMutex;
//          _iLastLockedMutex = sl_cs.cs_iIndex;
//        }
//      }
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
//      if (sl_cs.cs_iIndex!=-1) {
//        ASSERT(_iLastLockedMutex==sl_cs.cs_iIndex);
//        _iLastLockedMutex = sl_iLastLockedIndex;
//        sl_iLastLockedIndex = -2;
//      }
    }
  }
  sl_bLocked = FALSE;
}


#endif
