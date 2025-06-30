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

#include <Engine/Base/Console.h>
#include <Engine/Network/PlayerTarget.h>
#include <Engine/Base/ListIterator.inl>

class CActionEntry {
public:
  CListNode ae_ln;
  CPlayerAction ae_pa;
};

CActionBuffer::CActionBuffer(void)
{
}
CActionBuffer::~CActionBuffer(void)
{
  Clear();
}
void CActionBuffer::Clear(void)
{
  // for each buffered action
  FORDELETELIST(CActionEntry, ae_ln, ab_lhActions, itae) {
    // delete it
    delete &*itae;
  }
}

int qsort_CompareActions(const void *elem1, const void *elem2 )
{
  const CActionEntry &ae1 = **(CActionEntry **)elem1;
  const CActionEntry &ae2 = **(CActionEntry **)elem2;
  return ae1.ae_pa.pa_llCreated-ae2.ae_pa.pa_llCreated;
}

// add a new action to the buffer
void CActionBuffer::AddAction(const CPlayerAction &pa)
{
  // search all buffered actions
  FOREACHINLIST(CActionEntry, ae_ln, ab_lhActions, itae) {
    CActionEntry &ae = *itae;
    // if this is the one
    if (ae.ae_pa.pa_llCreated==pa.pa_llCreated) {
      // skip adding it again
      return;
    }
  }

  // add to the tail
  CActionEntry *pae = new CActionEntry;
  pae->ae_pa = pa;
  ab_lhActions.AddTail(pae->ae_ln);

  // sort the list
  ab_lhActions.Sort(&qsort_CompareActions, _offsetof(CActionEntry, ae_ln));

  //CPrintF("Buffered: %d (after add)\n", ab_lhActions.Count());
}

// flush all actions up to given time tag
void CActionBuffer::FlushUntilTime(__int64 llNewest)
{
  // for each buffered action
  FORDELETELIST(CActionEntry, ae_ln, ab_lhActions, itae) {
    CActionEntry &ae = *itae;

    // if up to that time
    if (ae.ae_pa.pa_llCreated<=llNewest) {
      // delete it
      delete &*itae;
    }
  }
}

// remove oldest buffered action
void CActionBuffer::RemoveOldest(void)
{
  // for each buffered action
  FORDELETELIST(CActionEntry, ae_ln, ab_lhActions, itae) {
    //CActionEntry &ae = *itae;
    // delete only first one
    delete &*itae;
    break;
  }
  //CPrintF("Buffered: %d (after remove)\n", ab_lhActions.Count());
}

// get number of actions buffered
INDEX CActionBuffer::GetCount(void)
{
  return ab_lhActions.Count();
}

// get an action by its index (0=oldest)
void CActionBuffer::GetActionByIndex(INDEX i, CPlayerAction &pa)
{
  // for each buffered action
  INDEX iInList=0;
  FOREACHINLIST(CActionEntry, ae_ln, ab_lhActions, itae) {
    if (iInList==i) {
      pa = itae->ae_pa;
      return;
    }
    iInList++;
  }
  // if not found, use empty
  pa.Clear();
}

// get last action older than given timetag
CPlayerAction *CActionBuffer::GetLastOlderThan(__int64 llTime)
{
  CPlayerAction *ppa = NULL;
  FOREACHINLIST(CActionEntry, ae_ln, ab_lhActions, itae) {
    CActionEntry &ae = *itae;
    if (ae.ae_pa.pa_llCreated>=llTime) {
      return ppa;
    }
    ppa = &ae.ae_pa;
  }
  return ppa;
}
