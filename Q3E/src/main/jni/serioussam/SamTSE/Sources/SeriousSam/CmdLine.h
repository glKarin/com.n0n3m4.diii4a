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

#ifndef SE_INCL_CMDLINE_H
#define SE_INCL_CMDLINE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

extern CTString cmd_strWorld;       // world to load
extern INDEX cmd_iGoToMarker;       // marker to go to
extern CTString cmd_strScript;      // script to execute
extern CTString cmd_strServer;      // server to connect to
extern INDEX cmd_iPort;             // port to connect to
extern CTString cmd_strPassword;    // network password
extern CTString cmd_strOutput;      // output from parsing command line
extern BOOL cmd_bServer;            // set to run as server
extern BOOL cmd_bQuickJoin;         // do not ask for players and network settings

void ParseCommandLine(CTString strCmd);


#endif  /* include-once check. */

