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

#ifndef SE_INCL_LISTITERATOR_INL
#define SE_INCL_LISTITERATOR_INL
#ifdef PRAGMA_ONCE
  #pragma once
#endif

/* simple list iterator: 4 bytes structure, all functions are inline */
template<class Cbase, int iOffset>
class CListIter {
private:
  CListNode *li_CurrentNode;
public:
  /* default constructor - no list attached */
  CListIter(void) {
    li_CurrentNode = NULL;
  };
  /* constructor with list attaching */
  CListIter(const CListHead &lhHead) {
    li_CurrentNode = &lhHead.IterationHead();
  };
  /* constructor to start from given node */
  CListIter(CListNode &lnNode) {
    ASSERT(lnNode.IsLinked());
    li_CurrentNode = &lnNode;
  };
  /* start iterating */
  void Reset(const CListHead &lhHead) {
    li_CurrentNode = &lhHead.IterationHead();
  };
  /* move to next node */
  void MoveToNext(void) {
    li_CurrentNode = &li_CurrentNode->IterationSucc();
  };
  /* move to previous node */
  void MoveToPrev(void) {
    li_CurrentNode = &li_CurrentNode->IterationPred();
  };
  /* check if finished */
  BOOL IsPastEnd(void) {
    return li_CurrentNode->IsTailMarker();
  };
  /* Insert a node after current one. */
  inline void InsertAfterCurrent(CListNode &lnNew) {
    li_CurrentNode->IterationInsertAfter(lnNew);
  };
  /* Insert a node before current one. */
  inline void InsertBeforeCurrent(CListNode &lnNew) {
    li_CurrentNode->IterationInsertBefore(lnNew);
  };

	/* Get current element. */
  Cbase &Current(void) { return *((Cbase*)((UBYTE *)li_CurrentNode - iOffset)); }
  Cbase &operator*(void) { return *((Cbase*)((UBYTE *)li_CurrentNode - iOffset)); }
  operator Cbase *(void) { return ((Cbase*)((UBYTE *)li_CurrentNode - iOffset)); }
  Cbase *operator->(void) { return ((Cbase*)((UBYTE *)li_CurrentNode - iOffset)); }
};

// taken from stddef.h
//#ifndef offsetof
//#define offsetof(s,m)	(size_t)&(((s *)0)->m)
//#endif

// declare a list iterator for a class with a CListNode member
#define LISTITER(baseclass, member) CListIter<baseclass, offsetof(baseclass, member)>

// make 'for' construct for walking a list
#define FOREACHINLIST(baseclass, member, head, iter) \
  for ( LISTITER(baseclass, member) iter(head); !iter.IsPastEnd(); iter.MoveToNext() )

// make 'for' construct for walking a list, keeping the iterator for later use
#define FOREACHINLISTKEEP(baseclass, member, head, iter) \
  LISTITER(baseclass, member) iter(head); \
  for (; !iter.IsPastEnd(); iter.MoveToNext() )

// make 'for' construct for deleting a list
#define FORDELETELIST(baseclass, member, head, iter)		  \
   for ( LISTITER(baseclass, member) iter(head), iter##next;	  \
   iter##next=iter, iter##next.IsPastEnd() || (iter##next.MoveToNext(),1), !iter.IsPastEnd(); \
     iter = iter##next)

#ifdef PLATFORM_OPENBSD
#ifdef LIST_HEAD
  #undef LIST_HEAD
#endif
#endif

// get the pointer to the first element in the list
#define LIST_HEAD(listhead, baseclass, member) \
  ( (baseclass *) ( ((UBYTE *)(&(listhead).Head())) - _offsetof(baseclass, member) ) )
// get the pointer to the last element in the list
#define LIST_TAIL(listhead, baseclass, member) \
  ( (baseclass *) ( ((UBYTE *)(&(listhead).Tail())) - _offsetof(baseclass, member) ) )

// get the pointer to the predecessor of the element
#define LIST_PRED(element, baseclass, member) \
  ( (baseclass *) ( ((UBYTE *)(&(element).member.Pred())) - _offsetof(baseclass, member) ) )
// get the pointer to the successor of the element
#define LIST_SUCC(element, baseclass, member) \
  ( (baseclass *) ( ((UBYTE *)(&(element).member.Succ())) - _offsetof(baseclass, member) ) )



#endif  /* include-once check. */

