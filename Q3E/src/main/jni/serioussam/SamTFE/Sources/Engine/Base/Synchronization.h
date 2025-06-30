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

#ifndef SE_INCL_SYNCHRONIZATION_H
#define SE_INCL_SYNCHRONIZATION_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#ifdef PLATFORM_UNIX
#include <pthread.h>
#endif
#include <stdio.h>

// intra-process mutex (only used by thread of same process)
class CTCriticalSection {
public:
    // !!! FIXME : rcg10142001 This would be better with a real subclass,
    // !!! FIXME :  and not void pointers.
  void *cs_pvObject;  // object is internal to implementation
  INDEX cs_iIndex;    // index of mutex used to prevent deadlock with assertions
  // use numbers from 1 and above for deadlock control, or -1 for no deadlock control
  ENGINE_API CTCriticalSection(void);
  ENGINE_API ~CTCriticalSection(void);
  INDEX Lock(void);
  INDEX TryToLock(void);
  INDEX Unlock(void);

private:
  ULONG LockCounter;
  ULONG owner;
};

// lock object for locking a mutex with automatic unlocking
class CTSingleLock {
public:
  CTCriticalSection &sl_cs;   // the mutex this object refers to
  BOOL sl_bLocked;            // set while locked
  INDEX sl_iLastLockedIndex;    // index of mutex that was locked before this lock
  ENGINE_API CTSingleLock(CTCriticalSection *pcs, BOOL bLock);
  ENGINE_API ~CTSingleLock(void);
  ENGINE_API void Lock(void);
  ENGINE_API BOOL TryToLock(void);
  ENGINE_API BOOL IsLocked(void);
  ENGINE_API void Unlock(void);
};

#ifdef PLATFORM_UNIX
template <typename T>
class CThreadLocal {
  pthread_key_t key;

public:
  CThreadLocal() { pthread_key_create(&key, nullptr); }

  T &get() {
    T *ptr = (T *)pthread_getspecific(key);
    if (!ptr) {
      ptr = new T;
      memset(ptr, 0, sizeof(T));
      pthread_setspecific(key, ptr);
    }
    return *ptr;
  }

  T &operator*() { return get(); }
};
#endif // PLATFORM_UNIX
#endif  /* include-once check. */


