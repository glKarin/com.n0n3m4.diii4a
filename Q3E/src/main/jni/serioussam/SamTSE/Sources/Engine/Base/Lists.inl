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

#ifndef SE_INCL_LISTS_INL
#define SE_INCL_LISTS_INL
#ifdef PRAGMA_ONCE
  #pragma once
#endif

/* Default constructor */
inline CListNode::CListNode(void)
{
  // make a non-linked node
  ln_Succ = NULL;
  ln_Pred = NULL;
}
/* Copy constructor */
inline CListNode::CListNode(const CListNode &lnOriginal)
{
  (void) lnOriginal;
  // make a non-linked node
  ln_Succ = NULL;
  ln_Pred = NULL;
}
/* Destructor */
inline CListNode::~CListNode(void)
{
  // if node is linked
  if (IsLinked()) {
    // remove it from list
    Remove();
  }
}
/* Assignment. */
inline CListNode &CListNode::operator=(const CListNode &lnOriginal)
{
  (void) lnOriginal;
  // make a non-linked node
  ln_Succ = NULL;
  ln_Pred = NULL;
  return *this;
}
/* Get successor of this node. */
inline CListNode &CListNode::Succ(void) const
{
  ASSERT(IsLinked() && ln_Succ->IsLinked());
  return *ln_Succ;
}
/* Get predeccessor of this node. */
inline CListNode &CListNode::Pred(void) const
{
  ASSERT(IsLinked() && ln_Pred->IsLinked());
  return *ln_Pred;
}
/* Get successor of this node for iteration. */
inline CListNode &CListNode::IterationSucc(void) const
{
  ASSERT(ln_Succ->IsTailMarker() || ln_Succ->IsLinked());
  return *ln_Succ;
}
/* Get predecessor of this node for iteration. */
inline CListNode &CListNode::IterationPred(void) const
{
  ASSERT(ln_Pred->IsHeadMarker() || ln_Pred->IsLinked());
  return *ln_Pred;
}
/* Insert a node after current one during iteration. */
inline void CListNode::IterationInsertAfter(CListNode &lnNew)
{
  ASSERT(!lnNew.IsLinked());
  ASSERT(ln_Succ->IsTailMarker() || ln_Succ->IsLinked());
  ln_Succ->ln_Pred = &lnNew;
  lnNew.ln_Succ = ln_Succ;
  lnNew.ln_Pred = this;
  ln_Succ = &lnNew;
}
/* Insert a node before current one during iteration. */
inline void CListNode::IterationInsertBefore(CListNode &lnNew)
{
  ASSERT(!lnNew.IsLinked());
  ASSERT(ln_Pred->IsHeadMarker() || ln_Pred->IsLinked());
  ln_Pred->ln_Succ = &lnNew;
  lnNew.ln_Pred = ln_Pred;
  lnNew.ln_Succ = this;
  ln_Pred = &lnNew;
}

/* Check that this list node is head marker of list. */
inline BOOL CListNode::IsHeadMarker(void) const
{
  // if this is in fact pointer to list.lh_Head
  if (ln_Pred == NULL ) {
    // it is end marker
    return TRUE;
  // otherwise
  } else {
    // it must be somewhere inside the list
    ASSERT(IsLinked());
    return FALSE;
  }
}

/* Check that this list node is tail marker of list. */
inline BOOL CListNode::IsTailMarker(void) const
{
  // if this is in fact pointer to list.lh_NULL
  if (ln_Succ == NULL ) {
    // it is end marker
    return TRUE;
  // otherwise
  } else {
    // it must be somewhere inside the list
    ASSERT(IsLinked());
    return FALSE;
  }
}

/* Check if this list node is head of list. */
inline BOOL CListNode::IsHead(void) const
{
  // it must be somewhere inside the list
  ASSERT(IsLinked());

  // if previous is list.lh_Head
  return ln_Pred->ln_Pred == NULL;
}

/* Check that this list node is tail of list. */
inline BOOL CListNode::IsTail(void) const
{
  // it must be somewhere inside the list
  ASSERT(IsLinked());

  // if next is list.lh_NULL
  return ln_Succ->ln_Succ == NULL;
}
/////////////////////////////////////////////////////////////////////
// CListHead implementations
/* Get list head. */
inline CListNode &CListHead::Head(void) const
{
  ASSERT(IsValid() && lh_Head->IsLinked());
  return *lh_Head;
}
/* Get list tail. */
inline CListNode &CListHead::Tail(void) const
{
  ASSERT(IsValid() && lh_Tail->IsLinked());
  return *lh_Tail;
}
/* Get list head for iteration. */
inline CListNode &CListHead::IterationHead(void) const
{
  ASSERT(IsValid() && (lh_Head->IsTailMarker() || lh_Head->IsLinked()));
  return *lh_Head;
}
/* Get list tail for iteration. */
inline CListNode &CListHead::IterationTail(void) const
{
  ASSERT(IsValid() && (lh_Tail->IsHeadMarker() || lh_Tail->IsLinked()));
  return *lh_Tail;
}


#endif  /* include-once check. */

