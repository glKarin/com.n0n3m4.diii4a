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

#if (!defined SINGLE_THREADED)
#error you probably want to define SINGLE_THREADED if you compile this.
#endif

CTCriticalSection::CTCriticalSection(void) {}
CTCriticalSection::~CTCriticalSection(void) {}
INDEX CTCriticalSection::Lock(void) { return(1); }
INDEX CTCriticalSection::TryToLock(void) { return(1); }
INDEX CTCriticalSection::Unlock(void) { return(0); }
CTSingleLock::CTSingleLock(CTCriticalSection *pcs, BOOL bLock) : sl_cs(*pcs) {}
CTSingleLock::~CTSingleLock(void) {}
void CTSingleLock::Lock(void) {}
BOOL CTSingleLock::TryToLock(void) { return(TRUE); }
BOOL CTSingleLock::IsLocked(void) { return(TRUE); }
void CTSingleLock::Unlock(void) {}

// end of NullSynchronization.cpp ...

