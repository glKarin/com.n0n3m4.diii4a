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

#ifndef SE_INCL_ACTIONBUFFER_H
#define SE_INCL_ACTIONBUFFER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>

// buffer of player actions, sorted by time of arrival
class CActionBuffer {
public:
  CListHead ab_lhActions;
public:
  CActionBuffer(void);
  ~CActionBuffer(void);
  void Clear(void);

  // add a new action to the buffer
  void AddAction(const CPlayerAction &pa);
  // remove oldest buffered action
  void RemoveOldest(void);
  // flush all actions up to given time tag
  void FlushUntilTime(__int64 llNewest);
  // get number of actions buffered
  INDEX GetCount(void);
  // get an action by its index (0=oldest)
  void GetActionByIndex(INDEX i, CPlayerAction &pa);
  // get last action older than given timetag
  CPlayerAction *GetLastOlderThan(__int64 llTime);
};


#endif  /* include-once check. */

