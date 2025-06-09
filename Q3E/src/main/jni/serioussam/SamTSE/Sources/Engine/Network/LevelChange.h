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

#ifndef SE_INCL_LEVELCHANGE_H
#define SE_INCL_LEVELCHANGE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

enum LevelChangePhase {
  // initial state
  LCP_NOCHANGE = 0,
  // ->initiation->
  LCP_INITIATED,
  // ->pre-change signalled->
  LCP_SIGNALLED,
  // ->change done->
  LCP_CHANGED,
  // ->post-change signalled->
  //LCP_NOCHANGE
};

extern LevelChangePhase _lphCurrent;

class CRememberedLevel {
public:
  CListNode rl_lnInSessionState;      // for linking in list of all remembered levels
  CTString rl_strFileName;            // file name of the level
  CTMemoryStream rl_strmSessionState; // saved session state
};


#endif  /* include-once check. */

