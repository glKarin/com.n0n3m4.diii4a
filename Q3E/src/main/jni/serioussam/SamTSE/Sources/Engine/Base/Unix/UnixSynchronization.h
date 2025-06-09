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

#include <Engine/Base/Synchronization.h>

#define WAIT_OBJECT_0 0
#define INFINITE 0xFFFFFFFF
#define VOID void
typedef void *PVOID;
typedef PVOID HANDLE;
typedef const char *LPCSTR;


LONG InterlockedIncrement(LONG *Addend);
LONG InterlockedDecrement(LONG volatile *Addend);
unsigned long long GetCurrentThreadId();
HANDLE CreateEvent(void *attr, BOOL bManualReset, BOOL initial, LPCSTR lpName);
BOOL CloseHandle(HANDLE hObject);
BOOL ResetEvent(HANDLE hEvent);
BOOL SetEvent(HANDLE hEvent);
DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
