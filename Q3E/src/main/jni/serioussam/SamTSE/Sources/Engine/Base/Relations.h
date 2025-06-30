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

#ifndef SE_INCL_RELATIONS_H
#define SE_INCL_RELATIONS_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>

// Object representing a link at the member of relation domain.
class CRelationSrc : public CListHead {
public:
// implementation:
// interface:
  // Construction/destruction.
  ENGINE_API CRelationSrc(void);
  ENGINE_API ~CRelationSrc(void);
  void Clear(void);
};

// Object representing a link at the member of relation codomain.
class CRelationDst : public CListHead {
public:
// implementation:
// interface:
  // Construction/destruction.
  ENGINE_API CRelationDst(void);
  ENGINE_API ~CRelationDst(void);
  void Clear(void);
};

// Object representing a link between a domain member and a codomain member.
class CRelationLnk {
public:
// implementation:
  CRelationSrc *rl_prsSrc;       // domain member
  CRelationDst *rl_prdDst;       // codomain member
  CListNode rl_lnSrc;   // node in list of links in domain member
  CListNode rl_lnDst;   // node in list of links in codomain member
// interface:
  // Construction/destruction.
  CRelationLnk(void);
  ~CRelationLnk(void);
  // Get the domain member of this pair.
  ENGINE_API CRelationSrc &GetSrc(void);
  // Get the codomain member of this pair.
  ENGINE_API CRelationDst &GetDst(void);
};

// Global functions for creating relations.
void ENGINE_API AddRelationPair(CRelationSrc &rsSrc, CRelationDst &rdDst);
void ENGINE_API AddRelationPairTailTail(CRelationSrc &rsSrc, CRelationDst &rdDst);
void ENGINE_API AddRelationPairHeadHead(CRelationSrc &rsSrc, CRelationDst &rdDst);

// make 'for' construct for walking a list in domain member
#define FOREACHSRCLINK(head, iter) \
  for ( LISTITER(CRelationLnk, rl_lnSrc) iter(head); !iter.IsPastEnd(); iter.MoveToNext() )
// make 'for' construct for walking a list in codomain member
#define FOREACHDSTLINK(head, iter) \
  for ( LISTITER(CRelationLnk, rl_lnDst) iter(head); !iter.IsPastEnd(); iter.MoveToNext() )

// get a domain member related to a codomain member through a link
#define DST(plink, dstclass, dstmember) \
  ( (dstclass *) ( ((UBYTE *)(&(plink->GetDst()))) - _offsetof(dstclass, dstmember) ) )
// get a codomain member that a domain member is related to through a link
#define SRC(plink, srcclass, srcmember) \
  ( (srcclass *) ( ((UBYTE *)(&(plink->GetSrc()))) - _offsetof(srcclass, srcmember) ) )

// make 'for' construct for walking all codomain members related to a domain member
#define FOREACHDSTOFSRC(srchead, dstclass, dstmember, pdst) \
  FOREACHSRCLINK(srchead, pdst##_iter) { \
    dstclass *pdst = DST(pdst##_iter, dstclass, dstmember);
// make 'for' construct for walking all domain members related to a codomain member
#define FOREACHSRCOFDST(dsthead, srcclass, srcmember, psrc) \
  FOREACHDSTLINK(dsthead, psrc##_iter) { \
    srcclass *psrc = SRC(psrc##_iter, srcclass, srcmember);
#define ENDFOR }


#endif  /* include-once check. */

