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

#include <Engine/Base/Lists.h>

#include <Engine/Base/ListIterator.inl>

/////////////////////////////////////////////////////////////////////
// CListHead implementation

/*
 * Initialize a list head.
 */
void CListHead::Clear(void)
{
  ASSERT(this!=NULL);
  lh_Head = (CListNode *) &(lh_NULL);
  lh_NULL = (CListNode *) NULL;
  lh_Tail = (CListNode *) &(lh_Head);
}

/*
 * Check if list head is valid.
 */
BOOL CListHead::IsValid(void) const
{
  ASSERT(this!=NULL);
  ASSERT(lh_NULL == NULL);
  ASSERT(((lh_Head == (CListNode *) &lh_NULL) && (lh_Tail == (CListNode *) &lh_Head))
      ||  (lh_Tail->IsValid() && lh_Head->IsValid()) );
  return TRUE;
}

/*
 * Check if list is empty.
 */
BOOL CListHead::IsEmpty(void) const
{
  ASSERT(IsValid());
  return( lh_Head == (CListNode *) &lh_NULL );
}

/*
 * Add a node to head of list.
 */
void CListHead::AddHead(CListNode &element)
{
  ASSERT(IsValid()&& !element.IsLinked());

  CListNode &first = *lh_Head;

  lh_Head = &element;
  element.ln_Succ = &first;
  element.ln_Pred = first.ln_Pred;
  first.ln_Pred = &element;
}

/*
 * Add a node to tail of list.
 */
void CListHead::AddTail(CListNode &element)
{
  ASSERT(IsValid()&& !element.IsLinked());
  CListNode &last = *lh_Tail;

  lh_Tail = &element;
  element.ln_Succ = last.ln_Succ;
  element.ln_Pred = &last;
  last.ln_Succ = &element;
}

/*
 * Remove a node from head of list.
 */
void CListHead::RemHead(void)
{
  ASSERT(!IsEmpty());
  lh_Head->Remove();
}

/*
 * Remove a node from tail of list.
 */
void CListHead::RemTail(void)
{
  ASSERT(!IsEmpty());
  lh_Tail->Remove();
}

/* Remove all elements from list. */
void CListHead::RemAll(void)
{
  // for each element
  for ( CListIter<CListNode, 0> iter(*this), iternext;
    iternext=iter, iternext.IsPastEnd() || (iternext.MoveToNext(),1), !iter.IsPastEnd();
    iter = iternext) {
    // remove it
    iter->Remove();
  }
}

/*
 * Move all elements of another list into this one.
 */
void CListHead::MoveList(CListHead &lhOther)
{
  ASSERT(IsValid() && lhOther.IsValid());

  // if the second list is empty
  if (lhOther.IsEmpty()) {
    // no moving
    return;
  }

  // get first element in other list
  CListNode &lnOtherFirst = *lhOther.lh_Head;
  // get last element in other list
  CListNode &lnOtherLast = *lhOther.lh_Tail;

  // get last element in this list
  CListNode &lnThisLast = *lh_Tail;

  // relink elements
  lnOtherLast.ln_Succ = lnThisLast.ln_Succ;
  lnThisLast.ln_Succ = &lnOtherFirst;
  lnOtherFirst.ln_Pred = &lnThisLast;
  lh_Tail = &lnOtherLast;

  // clear the other list
  lhOther.Clear();
}

/*
 * Return the number of elements in list.
 */
INDEX CListHead::Count(void) const
{
  INDEX slCount = 0;
  // walk the list -- modification of FOREACHINLIST that works with base CListNode class
  for ( CListIter<CListNode, 0> iter(*this); !iter.IsPastEnd(); iter.MoveToNext() ) {
    slCount++;
  }
  return slCount;
}

  /* Sort the list. */
void CListHead::Sort(int (*pCompare)(const void *p0, const void *p1), int iNodeOffset)
{
  // get number of elements
  INDEX ctCount = Count();
  // if none
  if (ctCount==0) {
    // do not sort
  }

  // create array of that much integers (the array will hold pointers to the list)
  size_t *aulPointers = new size_t[ctCount];
  // fill it
  INDEX i=0;
  for ( CListIter<int, 0> iter(*this); !iter.IsPastEnd(); iter.MoveToNext() ) {
    aulPointers[i] = ((size_t)&*iter)-iNodeOffset;
    i++;
  }

  // sort it
  qsort(aulPointers, ctCount, sizeof(aulPointers[0]), pCompare);

  // make temporary list
  CListHead lhTmp;
  // for each pointer
  {for(INDEX i=0; i<ctCount; i++) {
    const size_t ul = aulPointers[i];
    // get the node
    CListNode *pln = (CListNode*)(ul+iNodeOffset);
    // remove it from original list
    pln->Remove();
    // add it to the end of new list
    lhTmp.AddTail(*pln);
  }}

  // free the pointer array
  delete[] aulPointers;

  // move the sorted list here
  MoveList(lhTmp);
}

/////////////////////////////////////////////////////////////////////
// CListNode implementation

/*
 * Check if list node is valid.
 */
BOOL CListNode::IsValid(void) const
{
  ASSERT(this!=NULL);
  ASSERT((ln_Pred==NULL && ln_Succ==NULL) || (ln_Pred!=NULL && ln_Succ!=NULL));
  // it is valid if it is cleared or if it is linked
  return (ln_Pred==NULL && ln_Succ==NULL)
      || ((ln_Pred->ln_Succ == this) && (ln_Succ->ln_Pred == this));
}

/*
 * Check is linked in some list.
 */
BOOL CListNode::IsLinked(void) const
{
  ASSERT(IsValid());
  return ln_Pred != NULL;
}

/*
 * Remove a node from list.
 */
void CListNode::Remove(void)
{
  ASSERT(IsLinked());
  CListNode &next = *ln_Succ;
  CListNode &prev = *ln_Pred;
  ASSERT(next.IsTailMarker() || next.IsLinked());
  ASSERT(prev.IsHeadMarker() || prev.IsLinked());

  next.ln_Pred = &prev;
  prev.ln_Succ = &next;
  // make a non-linked node
  ln_Succ = NULL;
  ln_Pred = NULL;
}

/*
 * Add a node after this node.
 */
void CListNode::AddAfter(CListNode &lnToAdd)
{
  ASSERT(IsLinked() && !lnToAdd.IsLinked());

  CListNode &succ = IterationSucc();
  CListNode &pred = *this;

  succ.ln_Pred = &lnToAdd;
  pred.ln_Succ = &lnToAdd;
  lnToAdd.ln_Succ = &succ;
  lnToAdd.ln_Pred = &pred;
}

/*
 * Add a node before this node.
 */
void CListNode::AddBefore(CListNode &lnToAdd)
{
  ASSERT(IsLinked() && !lnToAdd.IsLinked());

  CListNode &succ = *this;
  CListNode &pred = IterationPred();

  succ.ln_Pred = &lnToAdd;
  pred.ln_Succ = &lnToAdd;
  lnToAdd.ln_Succ = &succ;
  lnToAdd.ln_Pred = &pred;
}

/*
 * Find the head of the list that this node is in.
 */
CListHead &CListNode::GetHead(void)
{
  // start at this node
  CListNode *pln = this;
  // while current node is not pointer to list.lh_Head
  while(pln->ln_Pred != NULL) {
    // go backwards
    pln = pln->ln_Pred;
  }
  // return the head pointer
  return *(CListHead*)pln;
}
