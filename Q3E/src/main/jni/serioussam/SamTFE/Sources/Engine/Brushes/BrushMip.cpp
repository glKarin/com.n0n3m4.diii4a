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
#include <Engine/Math/Float.h>
#include <Engine/Entities/Entity.h>

#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/Selection.cpp>

template class CDynamicArray<CBrushSector>;

// tolerance value for csg selection
#define CSG_RANGE_EPSILON (0.25f)

/*
 * Select all sectors within a range.
 */
void CBrushMip::SelectSectorsInRange(
    CBrushSectorSelectionForCSG &selbscInRange,
    FLOATaabbox3D boxRange
    )
{
  // for all sectors in the brush
  {FOREACHINDYNAMICARRAY(bm_abscSectors, CBrushSector, itbsc) {
    // if the sector is in the range
    if ( itbsc->bsc_boxBoundingBox.HasContactWith(boxRange, CSG_RANGE_EPSILON) ) {
      // select it
      selbscInRange.Select(itbsc.Current());
    }
  }}
}
void CBrushMip::SelectSectorsInRange(
    CBrushSectorSelection &selbscInRange,
    FLOATaabbox3D boxRange
    )
{
  // for all sectors in the brush
  {FOREACHINDYNAMICARRAY(bm_abscSectors, CBrushSector, itbsc) {
    // if the sector is in the range
    if ( itbsc->bsc_boxBoundingBox.HasContactWith(boxRange, CSG_RANGE_EPSILON) ) {
      // select it
      selbscInRange.Select(itbsc.Current());
    }
  }}
}

/*
 * Select open sector in brush.
 */
void CBrushMip::SelectOpenSector(CBrushSectorSelectionForCSG &selbscOpen)
{
  // for all sectors in the brush
  {FOREACHINDYNAMICARRAY(bm_abscSectors, CBrushSector, itbsc) {
    // if the sector is open
    if (itbsc->bsc_ulFlags & BSCF_OPENSECTOR) {
      // select it
      selbscOpen.Select(itbsc.Current());
    }
  }}
  // there must be at most one open sector in a brush mip
  ASSERT(selbscOpen.Count()<=1);
}

/*
 * Select closed sectors in brush.
 */
void CBrushMip::SelectClosedSectors(CBrushSectorSelectionForCSG &selbscClosed)
{
  // for all sectors in the brush
  {FOREACHINDYNAMICARRAY(bm_abscSectors, CBrushSector, itbsc) {
    // if the sector is closed
    if (!(itbsc->bsc_ulFlags & BSCF_OPENSECTOR)) {
      // select it
      selbscClosed.Select(itbsc.Current());
    }
  }}
  // there must be at most one open sector in a brush mip
  ASSERT(bm_abscSectors.Count()-selbscClosed.Count()<=1);
}

/*
 * Select all sectors in brush.
 */
void CBrushMip::SelectAllSectors(CBrushSectorSelectionForCSG &selbscAll)
{
  // for all sectors in the brush
  {FOREACHINDYNAMICARRAY(bm_abscSectors, CBrushSector, itbsc) {
    // select it
    selbscAll.Select(itbsc.Current());
  }}
}
void CBrushMip::SelectAllSectors(CBrushSectorSelection &selbscAll)
{
  // for all sectors in the brush
  {FOREACHINDYNAMICARRAY(bm_abscSectors, CBrushSector, itbsc) {
    // select it
    selbscAll.Select(itbsc.Current());
  }}
}

/*
 * Delete all sectors in a selection.
 */
void CBrushMip::DeleteSelectedSectors(CBrushSectorSelectionForCSG &selbscToDelete)
{
  // for each sector in the selection
  {FOREACHINDYNAMICCONTAINER(selbscToDelete, CBrushSector, itbsc) {
    // delete it from the brush mip
    bm_abscSectors.Delete(itbsc);
  }}

  /* NOTE: we must not clear the selection directly, since the sectors
     contained there are already freed and deselecting them would make an access
     violation.
   */
  // clear the selection on the container level
  selbscToDelete.CDynamicContainer<CBrushSector>::Clear();
}

/* Constructor. */
CBrushMip::CBrushMip(void) : bm_fMaxDistance(1E6f)
{
}

/*
 * Copy brush mip from another brush mip.
 */
void CBrushMip::Copy(CBrushMip &bmOther, FLOAT fStretch, BOOL bMirrorX)
{
  // clear this brush mip
  Clear();
  // copy the mip factor
  bm_fMaxDistance = bmOther.bm_fMaxDistance;
  // create an object 3d from the source brush mip
  CObject3D obOther;
  CBrushSectorSelectionForCSG selbscAll;
  bmOther.SelectAllSectors(selbscAll);
  bmOther.ToObject3D(obOther, selbscAll);

  // if there is some mirror or stretch
  if (fStretch!=1.0f || bMirrorX) {
    CSimpleProjection3D_DOUBLE prMirrorAndStretch;
    prMirrorAndStretch.ObjectPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
    prMirrorAndStretch.ViewerPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
    if (bMirrorX) {
      prMirrorAndStretch.ObjectStretchL() = FLOAT3D(-fStretch, fStretch, fStretch);
    } else {
      prMirrorAndStretch.ObjectStretchL() = FLOAT3D(fStretch, fStretch, fStretch);
    }
    prMirrorAndStretch.Prepare();
    obOther.Project(prMirrorAndStretch);
  }

  // try to
  try {
    // fill this brush mip from the object 3d
    AddFromObject3D_t(obOther);

  // if failed
  } catch (const char *strError) {
    // ignore the error
    (void) strError;
    ASSERT(FALSE);    // this should not happen
    return;
  }

  bm_pbrBrush->CalculateBoundingBoxesForOneMip(this);
}

/*
 * Free all memory and leave empty brush mip.
 */
void CBrushMip::Clear(void)
{
  // clear the sectors
  bm_abscSectors.Clear();
}

/* Update bounding box from bounding boxes of all sectors. */
void CBrushMip::UpdateBoundingBox(void)
{
  // clear the bounding box of the mip
  bm_boxBoundingBox = FLOATaabbox3D();
  bm_boxRelative = FLOATaabbox3D();
  // for all sectors in the brush mip
  {FOREACHINDYNAMICARRAY(bm_abscSectors, CBrushSector, itbsc) {
    // discard portal-sector links to this sector
    itbsc->bsc_rdOtherSidePortals.Clear();
    {FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
      itbpo->bpo_rsOtherSideSectors.Clear();
    }}
    // add the box of the sector to the box of mip
    bm_boxBoundingBox|=itbsc->bsc_boxBoundingBox;
    bm_boxRelative|=itbsc->bsc_boxRelative;
  }}

  // if this brush is zoning
  if (bm_pbrBrush->br_penEntity!=NULL && (bm_pbrBrush->br_penEntity->en_ulFlags&ENF_ZONING)) {
    // portal links must be updated also
    bm_pbrBrush->br_penEntity->en_pwoWorld->wo_bPortalLinksUpToDate = FALSE;
  }
}

/*
 * Calculate bounding boxes in all sectors.
 */
void CBrushMip::CalculateBoundingBoxes(CSimpleProjection3D_DOUBLE &prBrushToAbsolute)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  // clear the bounding box of the mip
  bm_boxBoundingBox = FLOATaabbox3D();
  bm_boxRelative = FLOATaabbox3D();
  // if there are no sectors
  if (bm_abscSectors.Count()==0) {
    // just make a small bounding box around brush center
    bm_boxBoundingBox = FLOATaabbox3D(
      prBrushToAbsolute.ObjectPlacementR().pl_PositionVector,
      0.01f);
    bm_boxRelative = FLOATaabbox3D(FLOAT3D(0,0,0), 0.01f);
    return;
  }
  // for all sectors in the brush mip
  {FOREACHINDYNAMICARRAY(bm_abscSectors, CBrushSector, itbsc) {
    // calculate bounding boxes in that sector
    itbsc->CalculateBoundingBoxes(prBrushToAbsolute);
    // add the box of the sector to the box of mip
    bm_boxBoundingBox|=itbsc->bsc_boxBoundingBox;
    bm_boxRelative|=itbsc->bsc_boxRelative;
  }}
}

/* Reoptimize all sectors in the brush mip. */
void CBrushMip::Reoptimize(void)
{
  // create an object 3d from the source brush mip
  CObject3D ob;
  { // NOTE: This is in a block to destroy the selection before brush mip is cleared!
    CBrushSectorSelectionForCSG selbscAll;
    SelectAllSectors(selbscAll);
    ToObject3D(ob, selbscAll);
  }
  // clear this brush mip
  Clear();

  // try to
  try {
    // fill this brush mip from the object 3d
    AddFromObject3D_t(ob);  // this will optimize the object3d first

  // if failed
  } catch (const char *strError) {
    // ignore the error
    (void) strError;
    ASSERT(FALSE);    // this should not happen
    return;
  }
}

/* Find all portals that have no links and kill their portal flag. */
void CBrushMip::RemoveDummyPortals(BOOL bClearPortalFlags)
{
  // for all sectors in the brush mip
  {FOREACHINDYNAMICARRAY(bm_abscSectors, CBrushSector, itbsc) {
    // for each portal polygon in sector
    {FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
      CBrushPolygon &bpo = *itbpo;
      if (!(bpo.bpo_ulFlags&OPOF_PORTAL)) {
        continue;
      }
      // find if it has at least one link in this same mip
      BOOL bHasLink = FALSE;
      // for all entities in the sector
      {FOREACHDSTOFSRC(bpo.bpo_rsOtherSideSectors, CBrushSector, bsc_rdOtherSidePortals, pbsc)
        if (pbsc->bsc_pbmBrushMip==this) {
          bHasLink = TRUE;
          break;
        }
      ENDFOR}
      // if there is none
      if (!bHasLink) {
        // assume that it should not be portal
        bpo.bpo_ulFlags&=~OPOF_PORTAL;

        // also start rendering as a wall so that user can see that 
        if(bClearPortalFlags)
        {
          bpo.bpo_ulFlags &= ~(BPOF_PASSABLE|BPOF_PORTAL);
        }
        bpo.bpo_abptTextures[0].s.bpt_ubBlend = BPT_BLEND_OPAQUE;
        bpo.bpo_bppProperties.bpp_ubShadowBlend = BPT_BLEND_SHADE;

        // remove all of its links
        bpo.bpo_rsOtherSideSectors.Clear();
        // world's links are not up to date anymore
        bm_pbrBrush->br_penEntity->en_pwoWorld->wo_bPortalLinksUpToDate = FALSE;
      }
    }}
  }}
}

/* Spread all brush mips after this one. */
void CBrushMip::SpreadFurtherMips(void)
{
  // get the brush of this mip
  CBrush3D *pbr = bm_pbrBrush;
  // current mip factor is the mip factor of this mip
  FLOAT fMipFactor = bm_fMaxDistance;
  // initially skip
  BOOL bSkip = TRUE;
  // for each mip in the brush
  FOREACHINLIST(CBrushMip, bm_lnInBrush, pbr->br_lhBrushMips, itbm) {
    // if not skipping
    if (!bSkip) {
      // increase the mip factor double as far
      fMipFactor*=2;
      // set the mip factor
      itbm->bm_fMaxDistance = fMipFactor;
    }
    // if it is this mip
    if (this==&*itbm) {
      // stop skipping
      bSkip = FALSE;
    }
  }
}

/* Set mip factor of this mip, spread all that are further. */
void CBrushMip::SetMipDistance(FLOAT fMaxDistance)
{
  // set the factor
  bm_fMaxDistance = fMaxDistance;
  // spread all brush mips after this one
  SpreadFurtherMips();
}

/* Get mip factor of this mip. */
FLOAT CBrushMip::GetMipDistance(void)
{
  return bm_fMaxDistance;
}

/* Get mip index of this mip. */
INDEX CBrushMip::GetMipIndex(void)
{
  // get the brush of this mip
  CBrush3D *pbr = bm_pbrBrush;
  // count each mip in the brush
  INDEX iIndex = 0;
  FOREACHINLIST(CBrushMip, bm_lnInBrush, pbr->br_lhBrushMips, itbm) {
    iIndex++;
    // until this one
    if (this==&*itbm) {
      return iIndex;
    }
  }
  ASSERT(FALSE);
  return 1;
}

// get next brush mip
CBrushMip *CBrushMip::GetNext(void)
{
  // if this is last mip
  if (bm_lnInBrush.IsTail()) {
    // there is no next mip
    return NULL;
  }

  // otherwise, return next one
  return LIST_SUCC(*this, CBrushMip, bm_lnInBrush);
}

// get previous brush mip
CBrushMip *CBrushMip::GetPrev(void)
{
  // if this is first mip
  if (bm_lnInBrush.IsHead()) {
    // there is no previous mip
    return NULL;
  }

  // otherwise, return previous one
  return LIST_PRED(*this, CBrushMip, bm_lnInBrush);
}
