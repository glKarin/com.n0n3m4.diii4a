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

#ifndef SE_INCL_PRIORITY_INL
#define SE_INCL_PRIORITY_INL
#ifdef PRAGMA_ONCE
  #pragma once
#endif

class CSetPriority {
public:
#ifdef PLATFORM_WIN32
  DWORD sp_dwProcessOld;
  int sp_iThreadOld;
  HANDLE sp_hThread;
  HANDLE sp_hProcess;
  CSetPriority(DWORD dwProcess, int iThread)
  {
    sp_hProcess = GetCurrentProcess();
    sp_hThread = GetCurrentThread();

    sp_dwProcessOld = GetPriorityClass(sp_hProcess);
    sp_iThreadOld = GetThreadPriority(sp_hThread);
    BOOL bSuccessProcess = SetPriorityClass(sp_hProcess, dwProcess);
    BOOL bSuccessThread = SetThreadPriority(sp_hThread, iThread);
    ASSERT(bSuccessProcess && bSuccessThread);
  }
  ~CSetPriority(void)
  {
    BOOL bSuccessProcess = SetPriorityClass(sp_hProcess, sp_dwProcessOld);
    BOOL bSuccessThread = SetThreadPriority(sp_hThread, sp_iThreadOld);
    ASSERT(bSuccessProcess && bSuccessThread);
  }

#else

  CSetPriority(DWORD dwProcess, int iThread) { STUBBED(""); }
  ~CSetPriority(void) { STUBBED(""); }

#endif
};

#endif /* include-once blocker. */

