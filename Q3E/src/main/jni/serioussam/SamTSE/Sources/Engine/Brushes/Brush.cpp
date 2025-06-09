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

#include <Engine/Brushes/Brush.h>
#include <Engine/World/World.h>
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/Math/Object3D.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Math/Projection_DOUBLE.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Math/Float.h>
#include <Engine/Entities/Entity.h>


// constructor
CBrush3D::CBrush3D(void)
{
  br_penEntity = NULL;
  br_pfsFieldSettings = NULL;
  br_ulFlags = 0;
}

// destructor
CBrush3D::~CBrush3D(void)
{
}

/* Delete a brush mip with given factor. */
void CBrush3D::DeleteBrushMip(CBrushMip *pbmToDelete)
{
  ASSERT(pbmToDelete!=NULL);
  ASSERT(pbmToDelete->bm_pbrBrush == this);
  // if there is only one brush mip
  if (br_lhBrushMips.Count()<=1) {
    // do nothing;
    return;
  }
  // remove it from list
  pbmToDelete->bm_lnInBrush.Remove();
  // destroy it
  delete pbmToDelete;
}

/* Create a new brush mip. */
CBrushMip *CBrush3D::NewBrushMipAfter(CBrushMip *pbmOld, BOOL bCopy)
{
  ASSERT(pbmOld!=NULL);
  ASSERT(pbmOld->bm_pbrBrush == this);

  // create one brush mip
  CBrushMip *pbmNew = new CBrushMip;
  pbmNew->bm_pbrBrush = this;
  // add it to the brush
  pbmOld->bm_lnInBrush.AddAfter(pbmNew->bm_lnInBrush);
  // copy the original to it
  if (bCopy) {
    pbmNew->Copy(*pbmOld, 1.0f, FALSE);
  }
  // respread the mips after old one
  pbmOld->SpreadFurtherMips();

  return pbmNew;
}
CBrushMip *CBrush3D::NewBrushMipBefore(CBrushMip *pbmOld, BOOL bCopy)
{
  ASSERT(pbmOld!=NULL);
  ASSERT(pbmOld->bm_pbrBrush == this);

  // create one brush mip
  CBrushMip *pbmNew = new CBrushMip;
  pbmNew->bm_pbrBrush = this;
  // add it to the brush
  pbmOld->bm_lnInBrush.AddBefore(pbmNew->bm_lnInBrush);
  // copy the original to it
  if (bCopy) {
    // copy the original to it
    pbmNew->Copy(*pbmOld, 1.0f, FALSE);
  }

  // get factor of mip before the new one
  FLOAT fFactorBefore = 0.0f;
  CBrushMip *pbmBefore = pbmNew->GetPrev();
  if (pbmBefore!=NULL) {
    fFactorBefore = pbmBefore->bm_fMaxDistance;
  }

  // calculate factor for new one to be between those two
  pbmNew->bm_fMaxDistance = (fFactorBefore+pbmOld->bm_fMaxDistance)/2.0f;

  return pbmNew;
}

// make 'for' construct for walking a list reversely
#define FOREACHINLIST_R(baseclass, member, head, iter) \
  for ( LISTITER(baseclass, member) iter(head.IterationTail()); \
   !iter->member.IsHeadMarker(); iter.MoveToPrev() )

/*
 * Get a brush mip for given mip-factor.
 */
CBrushMip *CBrush3D::GetBrushMipByDistance(FLOAT fMipDistance)
{
  // initially there is no brush mip
  CBrushMip *pbmLastGood=NULL;
  // for all brush mips in brush reversely
  FOREACHINLIST_R(CBrushMip, bm_lnInBrush, br_lhBrushMips, itbm) {
    CBrushMip &bm = *itbm;
    // if this mip cannot be of given factor
    if (bm.bm_fMaxDistance<fMipDistance) {
      // return last mip found
      return pbmLastGood;
    }
    // remember this mip
    pbmLastGood = itbm;
  }
  // return last mip found
  return pbmLastGood;
}
/* Get a brush mip by its given index. */
CBrushMip *CBrush3D::GetBrushMipByIndex(INDEX iMip)
{
  INDEX iCurrentMip = 0;
  // for all brush mips in brush
  FOREACHINLIST(CBrushMip, bm_lnInBrush, br_lhBrushMips, itbm) {
    iCurrentMip++;
    // if this is the mip
    if (iCurrentMip == iMip) {
      // return the mip found
      return &*itbm;
    }
  }
  return NULL;
}

// get first brush mip
CBrushMip *CBrush3D::GetFirstMip(void)
{
  return LIST_HEAD(br_lhBrushMips, CBrushMip, bm_lnInBrush);
}

// get last brush mip
CBrushMip *CBrush3D::GetLastMip(void)
{
  return LIST_TAIL(br_lhBrushMips, CBrushMip, bm_lnInBrush);
}

/*
 * Wrapper for CObject3D::Optimize(), updates profiling information.
 */
void CBrush3D::OptimizeObject3D(CObject3D &ob)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_OBJECTOPTIMIZE);
  _pfWorldEditingProfile.IncrementCounter(CWorldEditingProfile::PCI_SECTORSOPTIMIZED,
    ob.ob_aoscSectors.Count());
  ob.Optimize();
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_OBJECTOPTIMIZE);
}

/*
 * Free all memory and leave empty brush.
 */
void CBrush3D::Clear(void) {
  // delete all brush mips
  FORDELETELIST(CBrushMip, bm_lnInBrush, br_lhBrushMips, itbm) {
    delete &*itbm;
  }
}

/* Copy brush from another brush with possible mirror and stretch. */
void CBrush3D::Copy(CBrush3D &brOther, FLOAT fStretch, BOOL bMirrorX)
{
  // clear this brush in case there is something in it
  Clear();

  // for all brush mips in other brush
  FOREACHINLIST(CBrushMip, bm_lnInBrush, brOther.br_lhBrushMips, itbmOther) {
    // create one brush mip
    CBrushMip *pbmBrushMip = new CBrushMip;
    // add it to the brush
    br_lhBrushMips.AddTail(pbmBrushMip->bm_lnInBrush);
    pbmBrushMip->bm_pbrBrush = this;

    // copy the brush mip from original
    pbmBrushMip->Copy(*itbmOther, fStretch, bMirrorX);
  }
}

/*
 * Prepare a projection from brush space to absolute space.
 */
void CBrush3D::PrepareRelativeToAbsoluteProjection(
  CSimpleProjection3D_DOUBLE &prRelativeToAbsolute)
{
  // brush that does not have an entity is initialized at origin
  if(br_penEntity==NULL) {
    prRelativeToAbsolute.ObjectPlacementL().pl_PositionVector = FLOAT3D(0.0f, 0.0f, 0.0f);
    prRelativeToAbsolute.ObjectPlacementL().pl_OrientationAngle = ANGLE3D(0, 0, 0);
  } else {
    prRelativeToAbsolute.ObjectPlacementL() = br_penEntity->en_plPlacement;
  }
  prRelativeToAbsolute.ViewerPlacementL().pl_PositionVector = FLOAT3D(0.0f, 0.0f, 0.0f);
  prRelativeToAbsolute.ViewerPlacementL().pl_OrientationAngle = ANGLE3D(0, 0, 0);
  prRelativeToAbsolute.Prepare();
}

/*
 * Calculate bounding boxes in all brush mips.
 */
void CBrush3D::CalculateBoundingBoxes(void)
{
  CalculateBoundingBoxesForOneMip(NULL);
}
void CBrush3D::CalculateBoundingBoxesForOneMip(CBrushMip *pbmOnly)  // for only one mip
{
  // set FPU to double precision
  CSetFPUPrecision FPUPrecision(FPT_53BIT);

  // prepare a projection from brush space to absolute space
  CSimpleProjection3D_DOUBLE prBrushToAbsolute;
  PrepareRelativeToAbsoluteProjection(prBrushToAbsolute);
  // for all brush mips
  FOREACHINLIST(CBrushMip, bm_lnInBrush, br_lhBrushMips, itbm) {
    CBrushMip *pbm = itbm;
    if (pbmOnly==NULL || pbm==pbmOnly) {
      // calculate its boxes
      pbm->CalculateBoundingBoxes(prBrushToAbsolute);
    }
  }
  // if this brush is zoning
  if (br_penEntity!=NULL && (br_penEntity->en_ulFlags&ENF_ZONING)) {
    // portal links must be updated also
    extern BOOL _bPortalSectorLinksPreLoaded;
    extern BOOL _bDontDiscardLinks;
    br_penEntity->en_pwoWorld->wo_bPortalLinksUpToDate = _bPortalSectorLinksPreLoaded||_bDontDiscardLinks;
  }

  br_penEntity->UpdateSpatialRange();
}

// switch from zoning to non-zoning
void CBrush3D::SwitchToNonZoning(void)
{
  CalculateBoundingBoxes();

  // for all brush mips
  FOREACHINLIST(CBrushMip, bm_lnInBrush, br_lhBrushMips, itbm) {
    // for all sectors in the mip
    {FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
      // unset spatial clasification
      itbsc->bsc_rsEntities.Clear();
    }}
  }
}

// switch from non-zoning to zoning
void CBrush3D::SwitchToZoning(void)
{
  CalculateBoundingBoxes();

  // for all brush mips
  FOREACHINLIST(CBrushMip, bm_lnInBrush, br_lhBrushMips, itbm) {
    // for all sectors in the mip
    {FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
      // find entities in sector
      itbsc->FindEntitiesInSector();
    }}
  }
}
