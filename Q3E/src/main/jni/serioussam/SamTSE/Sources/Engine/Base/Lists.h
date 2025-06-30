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


#ifndef SE_INCL_LISTS_H
#define SE_INCL_LISTS_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Types.h>
#include <Engine/Base/Assert.h>

/* structures for doubly linked lists */
/* list node */
class ENGINE_API CListNode {
//implementation:
public:
  CListNode *ln_Succ;	/* pointer to previous node (successor) */
  CListNode *ln_Pred;	/* pointer to next node (predecessor) */

  /* Check if list node is valid. */
  BOOL IsValid(void) const;
//interface:
public:
  /* Default constructor */
  inline CListNode(void);
  /* Copy constructor */
  inline CListNode(const CListNode &lnOriginal);
  /* Destructor */
  inline ~CListNode(void);
  /* Assignment. */
  inline CListNode &operator=(const CListNode &lnOriginal);
  /* Check that this list node is linked in some list. */
  BOOL IsLinked(void) const;
  /* Add a node after this node. */
  void AddAfter(CListNode &node);
  /* Add a node before this node. */
  void AddBefore(CListNode &node);
  /* Remove this node from list. */
  void Remove(void);
  /* Check if this list node is head marker of list. */
  inline BOOL IsHeadMarker(void) const;
  /* Check if this list node is tail marker of list. */
  inline BOOL IsTailMarker(void) const;
  /* Check if this list node is at the head of list. */
  inline BOOL IsHead(void) const;
  /* Check if this list node is at the tail of list. */
  inline BOOL IsTail(void) const;
  /* Get successor of this node. */
  inline CListNode &Succ(void) const;
  /* Get predeccessor of this node. */
  inline CListNode &Pred(void) const;

  /* Get successor of this node for iteration. */
  inline CListNode &IterationSucc(void) const;
  /* Get predecessor of this node for iteration. */
  inline CListNode &IterationPred(void) const;
  /* Insert a node before current one during iteration. */
  inline void IterationInsertBefore(CListNode &lnNew);
  /* Insert a node after current one during iteration. */
  inline void IterationInsertAfter(CListNode &lnNew);

  /* Find the head of the list that this node is in. */
  CListHead &GetHead(void);
};
/* list head */
class ENGINE_API CListHead {
//implementation:
public:
  CListNode *lh_Head; /* pointer to first node (head) */
  CListNode *lh_NULL; /* lh_HeadPred and lh_TailSucc at the same time */
  CListNode *lh_Tail; /* pointer to last node (tail) */

  /* Check if this list head is valid. */
  BOOL IsValid(void) const;
  /* Clear the list head. */
  void Clear();
//interface:
public:
  /* Default constructor. */
  inline CListHead() { Clear(); };
  /* Copy constructor. */
  inline CListHead(const CListHead &lh) {ASSERTALWAYS("Don't copy list heads!");};
  /* Assignment. */
  inline void operator=(const CListHead &lh) {ASSERTALWAYS("Don't copy list heads!");};
  /* Get list head. */
  inline CListNode &Head(void) const;
  /* Get list tail. */
  inline CListNode &Tail(void) const;
  /* Get list head for iteration. */
  inline CListNode &IterationHead(void) const;
  /* Get list tail for iteration. */
  inline CListNode &IterationTail(void) const;
  /* Add a new element to head of list. */
  void AddHead(CListNode &node);
  /* Add a new element to tail of list. */
  void AddTail(CListNode &node);
  /* Remove first element from list. */
  void RemHead(void);
  /* Remove last element from list. */
  void RemTail(void);
  /* Remove all elements from list. */
  void RemAll(void);
  /* Test if list is empty. */
  BOOL IsEmpty(void) const;
  /* Move all elements of another list into this one. */
  void MoveList(CListHead &lhOther);
  /* Return the number of elements in list. */
  INDEX Count(void) const;
  /* Sort the list. */
  void Sort(int (*pCompare)(const void *p0, const void *p1), int iNodeOffset);
};

#include <Engine/Base/Lists.inl>


#endif  /* include-once check. */

