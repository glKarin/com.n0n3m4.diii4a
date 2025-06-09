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

#ifndef SE_INCL_GAMEAGENT_H
#define SE_INCL_GAMEAGENT_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

extern CTString ga_strServer;
extern CTString ga_strMSLegacy;
extern BOOL ga_bMSLegacy;

/// Initialize GameAgent.
extern void GameAgent_ServerInit(void);
/// Let GameAgent know that the server has stopped.
extern void GameAgent_ServerEnd(void);
/// GameAgent server update call which responds to enumeration pings and sends pings to masterserver.
extern void GameAgent_ServerUpdate(void);
/// Notify GameAgent that the server state has changed.
extern void GameAgent_ServerStateChanged(void);

/// Request serverlist enumeration.
extern void GameAgent_EnumTrigger(BOOL bInternet);
/// GameAgent client update for enumeration.
extern void GameAgent_EnumUpdate(void);
/// Cancel the GameAgent serverlist enumeration.
extern void GameAgent_EnumCancel(void);

#ifdef PLATFORM_WIN32
DWORD WINAPI _MS_Thread(LPVOID lpParam);
///
DWORD WINAPI _LocalNet_Thread(LPVOID lpParam);
#else
void* _MS_Thread(void *arg);
///
void* _LocalNet_Thread(void *arg);
#endif

/// Server request structure. Primarily used for getting server pings.
class CServerRequest {
public:
  ULONG sr_ulAddress;
  USHORT sr_iPort;
  long long sr_tmRequestTime;

public:
  CServerRequest(void);
  ~CServerRequest(void);

  /* Destroy all objects, and reset the array to initial (empty) state. */
  void Clear(void);
};

#endif // include once check
