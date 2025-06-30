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

#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/Common/PathFinding.h"
#include "EntitiesMP/NavigationMarker.h"

#define PRINTOUT(_dummy)
//#define PRINTOUT(something) something

// open and closed lists of nodes
static CListHead _lhOpen;
static CListHead _lhClosed;

FLOAT NodeDistance(CPathNode *ppn0, CPathNode *ppn1)
{
  return (
    ppn0->pn_pnmMarker->GetPlacement().pl_PositionVector - 
    ppn1->pn_pnmMarker->GetPlacement().pl_PositionVector).Length();
}

CPathNode::CPathNode(class CNavigationMarker *penMarker)
{
  pn_pnmMarker = penMarker;
  pn_ppnParent = NULL;
  pn_fG = 0.0f;
  pn_fH = 0.0f;
  pn_fF = 0.0f;
}

CPathNode::~CPathNode(void)
{
  // detach from marker when deleting
  ASSERT(pn_pnmMarker!=NULL);
  pn_pnmMarker->m_ppnNode = NULL;
}

// get name of this node
const CTString &CPathNode::GetName(void)
{
  ASSERT(this!=NULL);
  static CTString strNone="<none>";
  if (pn_pnmMarker==NULL) {
    return strNone;
  } else {
    return pn_pnmMarker->GetName();
  }
}

// get link with given index or null if no more (for iteration along the graph)
CPathNode *CPathNode::GetLink(INDEX i)
{
  ASSERT(this!=NULL && pn_pnmMarker!=NULL);
  CNavigationMarker *pnm = pn_pnmMarker->GetLink(i);
  if (pnm==NULL) {
    return NULL;
  }
  return pnm->GetPathNode();
}

// add given node to open list, sorting best first
static void SortIntoOpenList(CPathNode *ppnLink)
{
  // start at head of the open list
  LISTITER(CPathNode, pn_lnInOpen) itpn(_lhOpen);
  // while the given node is further than the one in list
  while(ppnLink->pn_fF>itpn->pn_fF && !itpn.IsPastEnd()) {
    // move to next node
    itpn.MoveToNext();
  }

  // if past the end of list
  if (itpn.IsPastEnd()) {
    // add to the end of list
    _lhOpen.AddTail(ppnLink->pn_lnInOpen);
  // if not past end of list
  } else {
    // add before current node
    itpn.InsertBeforeCurrent(ppnLink->pn_lnInOpen);
  }
}
  
// find shortest path from one marker to another
static BOOL FindPath(CNavigationMarker *pnmSrc, CNavigationMarker *pnmDst)
{

  ASSERT(pnmSrc!=pnmDst);
  CPathNode *ppnSrc = pnmSrc->GetPathNode();
  CPathNode *ppnDst = pnmDst->GetPathNode();

  PRINTOUT(CPrintF("--------------------\n"));
  PRINTOUT(CPrintF("FindPath(%s, %s)\n", (const char *)ppnSrc->GetName(), (const char *)ppnDst->GetName()));

  // start with empty open and closed lists
  ASSERT(_lhOpen.IsEmpty());
  ASSERT(_lhClosed.IsEmpty());

  // add the start node to open list
  ppnSrc->pn_fG = 0.0f;
  ppnSrc->pn_fH = NodeDistance(ppnSrc, ppnDst);
  ppnSrc->pn_fF = ppnSrc->pn_fG +ppnSrc->pn_fH;
  _lhOpen.AddTail(ppnSrc->pn_lnInOpen);
  PRINTOUT(CPrintF("StartState: %s\n", (const char *)ppnSrc->GetName()));

  // while the open list is not empty
  while (!_lhOpen.IsEmpty()) {
    // get the first node from open list (that is, the one with lowest F)
    CPathNode *ppnNode = LIST_HEAD(_lhOpen, CPathNode, pn_lnInOpen);
    ppnNode->pn_lnInOpen.Remove();
      _lhClosed.AddTail(ppnNode->pn_lnInClosed);
    PRINTOUT(CPrintF("Node: %s - moved from OPEN to CLOSED\n", (const char *)ppnNode->GetName()));

    // if this is the goal
    if (ppnNode==ppnDst) {
      PRINTOUT(CPrintF("PATH FOUND!\n"));
      // the path is found
      return TRUE;
    }

    // for each link of current node
    CPathNode *ppnLink = NULL;
    for(INDEX i=0; (ppnLink=ppnNode->GetLink(i))!=NULL; i++) {
      PRINTOUT(CPrintF(" Link %d: %s\n", i, (const char *)ppnLink->GetName()));
      // get cost to get to this node if coming from current node
      FLOAT fNewG = ppnLink->pn_fG+NodeDistance(ppnNode, ppnLink);
      // if a shorter path already exists
      if ((ppnLink->pn_lnInOpen.IsLinked() || ppnLink->pn_lnInClosed.IsLinked()) && fNewG>=ppnLink->pn_fG) {
        PRINTOUT(CPrintF("  shorter path exists through: %s\n", (const char *)ppnLink->pn_ppnParent->GetName()));
        // skip this link
        continue;
      }
      // remember this path
      ppnLink->pn_ppnParent = ppnNode;
      ppnLink->pn_fG = fNewG;
      ppnLink->pn_fH = NodeDistance(ppnLink, ppnDst);
      ppnLink->pn_fF = ppnLink->pn_fG + ppnLink->pn_fH;
      // remove from closed list, if in it
      if (ppnLink->pn_lnInClosed.IsLinked()) {
        ppnLink->pn_lnInClosed.Remove();
        PRINTOUT(CPrintF("  %s removed from CLOSED\n", (const char *)ppnLink->GetName()));
      }
      // add to open if not in it
      if (!ppnLink->pn_lnInOpen.IsLinked()) {
        SortIntoOpenList(ppnLink);
        PRINTOUT(CPrintF("  %s added to OPEN\n", (const char *)ppnLink->GetName()));
      }
    }
  }

  // if we get here, there is no path
  PRINTOUT(CPrintF("PATH NOT FOUND!\n"));
  return FALSE;
}

// clear all temporary structures used for path finding
static void ClearPath(CEntity *penThis)
{
  {FORDELETELIST(CPathNode, pn_lnInOpen, _lhOpen, itpn) {
    delete &itpn.Current();
  }}
  {FORDELETELIST(CPathNode, pn_lnInClosed, _lhClosed, itpn) {
    delete &itpn.Current();          
  }}

#ifndef NDEBUG
  // for each navigation marker in the world
  {FOREACHINDYNAMICCONTAINER(penThis->en_pwoWorld->wo_cenEntities, CEntity, iten) {
    if (!IsOfClass(iten, "NavigationMarker")) {
      continue;
    }
    CNavigationMarker &nm = (CNavigationMarker&)*iten;
    ASSERT(nm.m_ppnNode==NULL);
  }}
#endif
}

// find marker closest to a given position
static void FindClosestMarker(
    CEntity *penThis, const FLOAT3D &vSrc, CEntity *&penMarker, FLOAT3D &vPath)
{
  CNavigationMarker *pnmMin = NULL;
  FLOAT fMinDist = UpperLimit(0.0f);
  // for each sector this entity is in
  {FOREACHSRCOFDST(penThis->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
    // for each navigation marker in that sector
    {FOREACHDSTOFSRC(pbsc->bsc_rsEntities, CEntity, en_rdSectors, pen)
      if (!IsOfClass(pen, "NavigationMarker")) {
        continue;
      }
      CNavigationMarker &nm = (CNavigationMarker&)*pen;

      // get distance from source
      FLOAT fDist = (vSrc-nm.GetPlacement().pl_PositionVector).Length();
      // if closer than best found
      if(fDist<fMinDist) {
        // remember it
        fMinDist = fDist;
        pnmMin = &nm;
      }
    ENDFOR}
  ENDFOR}

  // if none found
  if (pnmMin==NULL) {
    // fail
    vPath = vSrc;
    penMarker = NULL;
    return;
  }

  // return position
  vPath = pnmMin->GetPlacement().pl_PositionVector;
  penMarker = pnmMin;
}

// find first marker for path navigation
void PATH_FindFirstMarker(CEntity *penThis, const FLOAT3D &vSrc, const FLOAT3D &vDst, CEntity *&penMarker, FLOAT3D &vPath)
{
  // find closest markers to source and destination positions
  CNavigationMarker *pnmSrc;
  FLOAT3D vSrcPath;
  FindClosestMarker(penThis, vSrc, (CEntity*&)pnmSrc, vSrcPath);
  CNavigationMarker *pnmDst;
  FLOAT3D vDstPath;
  FindClosestMarker(penThis, vDst, (CEntity*&)pnmDst, vDstPath);

  // if at least one is not found, or if they are same
  if (pnmSrc==NULL || pnmDst==NULL || pnmSrc==pnmDst) {
    // fail
    penMarker = NULL;
    vPath = vSrc;
    return;
  }

  // go to the source marker position
  vPath = vSrcPath;
  penMarker = pnmSrc;
}

// find next marker for path navigation
void PATH_FindNextMarker(CEntity *penThis, const FLOAT3D &vSrc, const FLOAT3D &vDst, CEntity *&penMarker, FLOAT3D &vPath)
{
  // find closest marker to destination position
  CNavigationMarker *pnmDst;
  FLOAT3D vDstPath;
  FindClosestMarker(penThis, vDst, (CEntity*&)pnmDst, vDstPath);

  // if at not found, or if same as current
  if (pnmDst==NULL || penMarker==pnmDst) {
    // fail
    penMarker = NULL;
    vPath = vSrc;
    return;
  }

  // try to find shortest path to the destination
  BOOL bFound = FindPath((CNavigationMarker*)penMarker, pnmDst);

  // if not found
  if (!bFound) {
    // just clean up and fail
    delete pnmDst->GetPathNode();
    ClearPath(penThis);
    penMarker = NULL;
    vPath = vSrc;
    return;
  }

  // find the first marker position after current
  CPathNode *ppn = pnmDst->GetPathNode();
  while (ppn->pn_ppnParent!=NULL && ppn->pn_ppnParent->pn_pnmMarker!=penMarker) {
    ppn = ppn->pn_ppnParent;
  }
  penMarker = ppn->pn_pnmMarker;

  // go there
  vPath = penMarker->GetPlacement().pl_PositionVector;

  // clean up
  ClearPath(penThis);
}
