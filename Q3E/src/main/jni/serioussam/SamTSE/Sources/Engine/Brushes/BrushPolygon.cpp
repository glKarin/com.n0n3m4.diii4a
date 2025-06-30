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
#include <Engine/Brushes/BrushTransformed.h>
#include <Engine/Entities/ShadingInfo.h>
#include <Engine/Templates/BSP_internal.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Math/Float.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Templates/Selection.cpp>

template class CStaticArray<CBrushPolygonEdge>;
template class CStaticArray<CBrushPolygon>;
template class CStaticArray<long>;

// set new absolute position for the vertex
void CBrushVertex::SetAbsolutePosition(const DOUBLE3D &vAbsolute)
{
  // get its brush entity
  CEntity *pen = bvx_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
  if (pen==NULL) {
    ASSERT(FALSE);
    return;
  }
  // back-transform to relative coordinates
  DOUBLE3D vRelative = vAbsolute-FLOATtoDOUBLE(pen->en_plPlacement.pl_PositionVector);
  vRelative *= FLOATtoDOUBLE(!pen->en_mRotation);

  // remember new coordinates
  bvx_vdPreciseRelative = vRelative;
  bvx_vAbsolute = DOUBLEtoFLOAT(vAbsolute);
  bvx_vRelative = DOUBLEtoFLOAT(vRelative);
  if(bvx_pwvxWorking!=NULL)
  {
    bvx_pwvxWorking->wvx_vRelative = bvx_vRelative;
  }
}

/*
 * Calculate bounding box of this polygon.
 */
void CBrushPolygon::CalculateBoundingBox(void)
{
  // NOTE: vertices are already transformed to absolute space
  // discard portal-sector links to this polygon
  extern BOOL _bDontDiscardLinks;
  if (!(bpo_pbscSector->bsc_ulTempFlags&BSCTF_PRELOADEDLINKS)&&!_bDontDiscardLinks) {
    bpo_rsOtherSideSectors.Clear();
  }

  // clear the bounding box
  bpo_boxBoundingBox = FLOATaabbox3D();
  // for all edges in polygon
  {FOREACHINSTATICARRAY(bpo_abpePolygonEdges, CBrushPolygonEdge, itbpe) {
    // add the edges vertices to the bounding box
    bpo_boxBoundingBox |= itbpe->bpe_pbedEdge->bed_pbvxVertex0->bvx_vAbsolute;
    bpo_boxBoundingBox |= itbpe->bpe_pbedEdge->bed_pbvxVertex1->bvx_vAbsolute;
  }}
}


/* Create a BSP polygon from this polygon. */
void CBrushPolygon::CreateBSPPolygon(BSPPolygon<DOUBLE, 3> &bspo)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  CBrushPolygon &brpo = *this;

  // set the plane of the bsp polygon
  ((DOUBLEplane3D &)bspo) = *brpo.bpo_pbplPlane->bpl_ppldPreciseAbsolute;
  bspo.bpo_ulPlaneTag = (size_t)brpo.bpo_pbscSector->bsc_abplPlanes.Index(brpo.bpo_pbplPlane);

  // create the array of edges in the bsp polygon
  INDEX ctEdges = brpo.bpo_abpePolygonEdges.Count();
  bspo.bpo_abedPolygonEdges.New(ctEdges);

  // for all edges in the polygon
  bspo.bpo_abedPolygonEdges.Lock();
  {for(INDEX iEdge=0; iEdge<ctEdges; iEdge++){
    CBrushPolygonEdge &brped = brpo.bpo_abpePolygonEdges[iEdge];
    BSPEdge<DOUBLE, 3>  &bed = bspo.bpo_abedPolygonEdges[iEdge];
    // create the bsp edge in the bsp polygon
    brped.GetVertexCoordinatesPreciseAbsolute(bed.bed_vVertex0, bed.bed_vVertex1);
  }}
  bspo.bpo_abedPolygonEdges.Unlock();
}

void CBrushPolygon::CreateBSPPolygon(BSPPolygon<FLOAT, 3> &bspo)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  CBrushPolygon &brpo = *this;

  // set the plane of the bsp polygon
  ((FLOATplane3D &)bspo) = DOUBLEtoFLOAT(*brpo.bpo_pbplPlane->bpl_ppldPreciseAbsolute);
  bspo.bpo_ulPlaneTag = (size_t)brpo.bpo_pbscSector->bsc_abplPlanes.Index(brpo.bpo_pbplPlane);

  // create the array of edges in the bsp polygon
  INDEX ctEdges = brpo.bpo_abpePolygonEdges.Count();
  bspo.bpo_abedPolygonEdges.New(ctEdges);

  // for all edges in the polygon
  bspo.bpo_abedPolygonEdges.Lock();
  {for(INDEX iEdge=0; iEdge<ctEdges; iEdge++){
    CBrushPolygonEdge &brped = brpo.bpo_abpePolygonEdges[iEdge];
    BSPEdge<FLOAT, 3>  &bed = bspo.bpo_abedPolygonEdges[iEdge];
    // create the bsp edge in the bsp polygon
    Vector<DOUBLE, 3> v0, v1;
    brped.GetVertexCoordinatesPreciseAbsolute(v0, v1);
    bed.bed_vVertex0 = DOUBLEtoFLOAT(v0);
    bed.bed_vVertex1 = DOUBLEtoFLOAT(v1);
  }}
  bspo.bpo_abedPolygonEdges.Unlock();
}

void CBrushPolygon::CreateBSPPolygonNonPrecise(BSPPolygon<DOUBLE, 3> &bspo)
{
  CBrushPolygon &brpo = *this;

  // offset for epsilon testing
  const DOUBLE fOffset = -0.01;

  // set the plane of the bsp polygon
  ((DOUBLEplane3D &)bspo) = FLOATtoDOUBLE(brpo.bpo_pbplPlane->bpl_plAbsolute);
  bspo.bpo_ulPlaneTag = (size_t)brpo.bpo_pbscSector->bsc_abplPlanes.Index(brpo.bpo_pbplPlane);
  // calculate offset for points
  DOUBLE3D vOffset = FLOATtoDOUBLE(((FLOAT3D&)brpo.bpo_pbplPlane->bpl_plAbsolute))*-fOffset;
  // offset the plane
  bspo.Offset(fOffset);

  // create the array of edges in the bsp polygon
  INDEX ctEdges = brpo.bpo_abpePolygonEdges.Count();
  bspo.bpo_abedPolygonEdges.New(ctEdges);

  // for all edges in the polygon
  bspo.bpo_abedPolygonEdges.Lock();
  {for(INDEX iEdge=0; iEdge<ctEdges; iEdge++){
    CBrushPolygonEdge &brped = brpo.bpo_abpePolygonEdges[iEdge];
    BSPEdge<DOUBLE, 3>  &bed = bspo.bpo_abedPolygonEdges[iEdge];
    // create the offseted bsp edge in the bsp polygon
    FLOAT3D v0, v1;
    brped.GetVertexCoordinatesAbsolute(v0, v1);
    bed.bed_vVertex0 = FLOATtoDOUBLE(v0)+vOffset;
    bed.bed_vVertex1 = FLOATtoDOUBLE(v1)+vOffset;
  }}
  bspo.bpo_abedPolygonEdges.Unlock();
}

void CBrushPolygon::CreateBSPPolygonNonPrecise(BSPPolygon<FLOAT, 3> &bspo)
{
  CBrushPolygon &brpo = *this;

  // offset for epsilon testing
  const FLOAT fOffset = -0.01f;

  // set the plane of the bsp polygon
  ((FLOATplane3D &)bspo) = brpo.bpo_pbplPlane->bpl_plAbsolute;
  bspo.bpo_ulPlaneTag = (size_t)brpo.bpo_pbscSector->bsc_abplPlanes.Index(brpo.bpo_pbplPlane);
  // calculate offset for points
  FLOAT3D vOffset = ((FLOAT3D&)brpo.bpo_pbplPlane->bpl_plAbsolute)*-fOffset;
  // offset the plane
  bspo.Offset(fOffset);

  // create the array of edges in the bsp polygon
  INDEX ctEdges = brpo.bpo_abpePolygonEdges.Count();
  bspo.bpo_abedPolygonEdges.New(ctEdges);

  // for all edges in the polygon
  bspo.bpo_abedPolygonEdges.Lock();
  {for(INDEX iEdge=0; iEdge<ctEdges; iEdge++){
    CBrushPolygonEdge &brped = brpo.bpo_abpePolygonEdges[iEdge];
    BSPEdge<FLOAT, 3>  &bed = bspo.bpo_abedPolygonEdges[iEdge];
    // create the offseted bsp edge in the bsp polygon
    FLOAT3D v0, v1;
    brped.GetVertexCoordinatesAbsolute(v0, v1);
    bed.bed_vVertex0 = v0+vOffset;
    bed.bed_vVertex1 = v1+vOffset;
  }}
  bspo.bpo_abedPolygonEdges.Unlock();
}

/*
 * Select adjacent polygons with same color as this one.
 */
void CBrushPolygon::SelectSimilarByColor(CBrushPolygonSelection &selbpoSimilar)
{
  // if this polygon is not selected
  if (!IsSelected(BPOF_SELECTED)) {
    // select this polygon
    selbpoSimilar.Select(*this);
  }

  // for all other unselected walls in brush mip that have same color
  FOREACHINDYNAMICARRAY(bpo_pbscSector->bsc_pbmBrushMip->bm_abscSectors, CBrushSector, itbsc) {
    FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpoOther) {
      if ((!(itbpoOther->bpo_ulFlags&BPOF_PORTAL)||(itbpoOther->bpo_ulFlags&(BPOF_TRANSLUCENT|BPOF_TRANSPARENT))) &&
          !itbpoOther->IsSelected(BPOF_SELECTED) &&
          itbpoOther->bpo_colColor == bpo_colColor) {
        // if the other polygon touches this one
        if (TouchesInAnySector(*itbpoOther)) {
          // recursively select the other polygon
          itbpoOther->SelectSimilarByColor(selbpoSimilar);
        }
      }
    }
  }
}

/*
 * Select adjacent polygons with same texture as this one.
 */
void CBrushPolygon::SelectSimilarByTexture(
  CBrushPolygonSelection &selbpoSimilar, INDEX iTexture)
{
  // if this polygon is not selected
  if (!IsSelected(BPOF_SELECTED)) {
    // select this polygon
    selbpoSimilar.Select(*this);
  }

  // for all other unselected walls in brush mip that have same texture
  FOREACHINDYNAMICARRAY(bpo_pbscSector->bsc_pbmBrushMip->bm_abscSectors, CBrushSector, itbsc) {
    FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpoOther) {
      if ((!(itbpoOther->bpo_ulFlags&BPOF_PORTAL)||(itbpoOther->bpo_ulFlags&(BPOF_TRANSLUCENT|BPOF_TRANSPARENT))) &&
          !itbpoOther->IsSelected(BPOF_SELECTED) &&
          itbpoOther->bpo_abptTextures[iTexture].bpt_toTexture.GetData()
          == bpo_abptTextures[iTexture].bpt_toTexture.GetData()) {
        // if the other polygon touches this one
        if (TouchesInAnySector(*itbpoOther)) {
          // recursively select the other polygon
          itbpoOther->SelectSimilarByTexture(selbpoSimilar, iTexture);
        }
      }
    }
  }
}

/*
 * Select all polygons in sector with same texture as this one.
 */
void CBrushPolygon::SelectByTextureInSector(CBrushPolygonSelection &selbpoSimilar, INDEX iTexture)
{
  // for all other unselected walls in sector that have same texture
  FOREACHINSTATICARRAY(bpo_pbscSector->bsc_abpoPolygons, CBrushPolygon, itbpo) {
    if ((!(itbpo->bpo_ulFlags&BPOF_PORTAL)||(itbpo->bpo_ulFlags&(BPOF_TRANSLUCENT|BPOF_TRANSPARENT)))&&
        !itbpo->IsSelected(BPOF_SELECTED) &&
        itbpo->bpo_abptTextures[iTexture].bpt_toTexture.GetData()
        == bpo_abptTextures[iTexture].bpt_toTexture.GetData())
    {
      // select this polygon
      selbpoSimilar.Select(*itbpo);
    }
  }
}

/*
 * Select all polygons in sector with same color as this one.
 */
void CBrushPolygon::SelectByColorInSector(CBrushPolygonSelection &selbpoSimilar)
{
  // for all other unselected walls in sector that have same color
  FOREACHINSTATICARRAY(bpo_pbscSector->bsc_abpoPolygons, CBrushPolygon, itbpo) {
    if ((!(itbpo->bpo_ulFlags&BPOF_PORTAL)||(itbpo->bpo_ulFlags&(BPOF_TRANSLUCENT|BPOF_TRANSPARENT))) &&
        !itbpo->IsSelected(BPOF_SELECTED) &&
        itbpo->bpo_colColor == bpo_colColor)
    {
      // select this polygon
      selbpoSimilar.Select(*itbpo);
    }
  }
}

/* Clear the object. */
void CBrushPolygon::Clear(void)
{
  bpo_abpePolygonEdges.Clear();
  bpo_smShadowMap.Clear();
  bpo_abptTextures[0].Clear();
  bpo_abptTextures[1].Clear();
  bpo_abptTextures[2].Clear();
  DiscardShadingInfos();
};
// discard all cached shading info for models
void CBrushPolygon::DiscardShadingInfos(void)
{
  FORDELETELIST( CShadingInfo, si_lnInPolygon, bpo_lhShadingInfos, itsi) {
    itsi->si_penEntity->en_ulFlags &= ~ENF_VALIDSHADINGINFO;
    itsi->si_lnInPolygon.Remove();
    itsi->si_pbpoPolygon = NULL;
  }
}

/*
 * Copy polygon within same sector.
 */
void CBrushPolygon::CopyFromSameSector(CBrushPolygon &bpoOriginal)
{
  // copy simple data
  bpo_pbplPlane    = bpoOriginal.bpo_pbplPlane;
  bpo_colColor     = bpoOriginal.bpo_colColor;
  bpo_ulFlags      = bpoOriginal.bpo_ulFlags;
  BOOL bCopyMapping = TRUE;
  bpo_abptTextures[0].CopyTextureProperties( bpoOriginal.bpo_abptTextures[0], bCopyMapping);
  bpo_abptTextures[1].CopyTextureProperties( bpoOriginal.bpo_abptTextures[1], bCopyMapping);
  bpo_abptTextures[2].CopyTextureProperties( bpoOriginal.bpo_abptTextures[2], bCopyMapping);
  bpo_mdShadow     = bpoOriginal.bpo_mdShadow;
  bpo_pbscSector   = bpoOriginal.bpo_pbscSector;

  // copy all edges
  bpo_abpePolygonEdges = bpoOriginal.bpo_abpePolygonEdges;
}

/* Copy polygon properties */
CBrushPolygon &CBrushPolygon::CopyProperties(CBrushPolygon &bpoOther, BOOL bCopyMapping) {
  bpo_ulFlags &= ~BPOF_MASK_FOR_COPYING;
  bpo_ulFlags |= (bpoOther.bpo_ulFlags&BPOF_MASK_FOR_COPYING);
  bpo_bppProperties = bpoOther.bpo_bppProperties;
  bpo_colShadow = bpoOther.bpo_colShadow;
  bpo_abptTextures[0].CopyTextureProperties( bpoOther.bpo_abptTextures[0], bCopyMapping);
  bpo_abptTextures[1].CopyTextureProperties( bpoOther.bpo_abptTextures[1], bCopyMapping);
  bpo_abptTextures[2].CopyTextureProperties( bpoOther.bpo_abptTextures[2], bCopyMapping);
  return *this;
};

/* Copy polygon properties without texture */
CBrushPolygon &CBrushPolygon::CopyPropertiesWithoutTexture(CBrushPolygon &bpoOther) {
  bpo_ulFlags &= ~BPOF_MASK_FOR_COPYING;
  bpo_ulFlags |= (bpoOther.bpo_ulFlags&BPOF_MASK_FOR_COPYING);
  bpo_bppProperties = bpoOther.bpo_bppProperties;
  bpo_colShadow = bpoOther.bpo_colShadow;
  return *this;
};

/* Copy polygon's textures */
CBrushPolygon &CBrushPolygon::CopyTextures(CBrushPolygon &bpoOther) {
  bpo_abptTextures[0].CopyTextureProperties( bpoOther.bpo_abptTextures[0], TRUE);
  bpo_abptTextures[1].CopyTextureProperties( bpoOther.bpo_abptTextures[1], TRUE);
  bpo_abptTextures[2].CopyTextureProperties( bpoOther.bpo_abptTextures[2], TRUE);
  return *this;
};

/*
 * Calculate area of the polygon.
 */
DOUBLE CBrushPolygon::CalculateArea(void)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  DOUBLE3D vArea = DOUBLE3D(0.0, 0.0, 0.0);
  // for each polygon edge
  {FOREACHINSTATICARRAY(bpo_abpePolygonEdges, CBrushPolygonEdge, itbpe) {
    DOUBLE3D v0, v1;
    itbpe->GetVertexCoordinatesPreciseRelative(v0, v1);
    // add the area of triangle that the edge closes with the origin
    vArea += v0*v1;
  }}
  return ( ((DOUBLE3D&)bpo_pbplPlane->bpl_pldPreciseRelative)%vArea ) / 2.0;
}

// move edges from another polygon into this one
void CBrushPolygon::MovePolygonEdges(CBrushPolygon &bpoSource)
{
  ASSERT(bpo_pbplPlane==bpoSource.bpo_pbplPlane);
  INDEX ctEdgesThis = bpo_abpePolygonEdges.Count();
  INDEX ctEdgesSource = bpoSource.bpo_abpePolygonEdges.Count();
  // create an array to hold all edges
  CStaticArray<CBrushPolygonEdge> abpeNew;
  abpeNew.New(ctEdgesThis+ctEdgesSource);
  INDEX ibpeNew = 0;
  // copy edges of this polygon
  {for(INDEX ibpeThis=0; ibpeThis<ctEdgesThis; ibpeThis++) {
    abpeNew[ibpeNew] = bpo_abpePolygonEdges[ibpeThis];
    ibpeNew++;
  }}
  // copy edges of source polygon
  {for(INDEX ibpeSource=0; ibpeSource<ctEdgesSource; ibpeSource++) {
    abpeNew[ibpeNew] = bpoSource.bpo_abpePolygonEdges[ibpeSource];
    ibpeNew++;
  }}
  // use the new array of edges instead old one
  bpo_abpePolygonEdges.MoveArray(abpeNew);
}

/* Test if this edge touches another one. */
#define TOUCHEPSILON 0.001f
extern INDEX wed_bIgnoreTJunctions;

BOOL CBrushEdge::TouchesInSameSector(CBrushEdge &bedOther)
{
  // if they have some common vertices
  if (
    bed_pbvxVertex0==bedOther.bed_pbvxVertex0 ||
    bed_pbvxVertex0==bedOther.bed_pbvxVertex1 ||
    bed_pbvxVertex1==bedOther.bed_pbvxVertex0 ||
    bed_pbvxVertex1==bedOther.bed_pbvxVertex1) {
    // they touch
    return TRUE;
  // if they have no common vertices
  } else if( !wed_bIgnoreTJunctions) {
    // test if some vertex is on the other edge
    FLOAT fA0A1 = (bed_pbvxVertex0->bvx_vRelative-bed_pbvxVertex1->bvx_vRelative).Length();
    FLOAT fB0B1 = (bedOther.bed_pbvxVertex0->bvx_vRelative-bedOther.bed_pbvxVertex1->bvx_vRelative).Length();
    FLOAT fA0B0 = (bed_pbvxVertex0->bvx_vRelative-bedOther.bed_pbvxVertex0->bvx_vRelative).Length();
    FLOAT fA0B1 = (bed_pbvxVertex0->bvx_vRelative-bedOther.bed_pbvxVertex1->bvx_vRelative).Length();
    FLOAT fA1B0 = (bed_pbvxVertex1->bvx_vRelative-bedOther.bed_pbvxVertex0->bvx_vRelative).Length();
    FLOAT fA1B1 = (bed_pbvxVertex1->bvx_vRelative-bedOther.bed_pbvxVertex1->bvx_vRelative).Length();
    FLOAT fB0A0 = (bedOther.bed_pbvxVertex0->bvx_vRelative-bed_pbvxVertex0->bvx_vRelative).Length();
    FLOAT fB0A1 = (bedOther.bed_pbvxVertex0->bvx_vRelative-bed_pbvxVertex1->bvx_vRelative).Length();
    FLOAT fB1A0 = (bedOther.bed_pbvxVertex1->bvx_vRelative-bed_pbvxVertex0->bvx_vRelative).Length();
    FLOAT fB1A1 = (bedOther.bed_pbvxVertex1->bvx_vRelative-bed_pbvxVertex1->bvx_vRelative).Length();

    return
      Abs(fB0B1-fA0B0-fA0B1)<TOUCHEPSILON||
      Abs(fB0B1-fA1B0-fA1B1)<TOUCHEPSILON||
      Abs(fA0A1-fB0A0-fB0A1)<TOUCHEPSILON||
      Abs(fA0A1-fB1A0-fB1A1)<TOUCHEPSILON;
  }
  return FALSE;
}
BOOL CBrushEdge::TouchesInAnySector(CBrushEdge &bedOther)
{
  // if they have some common vertices
  if (
    // they touch
    bed_pbvxVertex0->bvx_vRelative==bedOther.bed_pbvxVertex0->bvx_vRelative ||
    bed_pbvxVertex0->bvx_vRelative==bedOther.bed_pbvxVertex1->bvx_vRelative ||
    bed_pbvxVertex1->bvx_vRelative==bedOther.bed_pbvxVertex0->bvx_vRelative ||
    bed_pbvxVertex1->bvx_vRelative==bedOther.bed_pbvxVertex1->bvx_vRelative) {
    return TRUE;
  // if they have no common vertices
  } else if( !wed_bIgnoreTJunctions) {
    // test if some vertex is on the other edge
    FLOAT fA0A1 = (bed_pbvxVertex0->bvx_vRelative-bed_pbvxVertex1->bvx_vRelative).Length();
    FLOAT fB0B1 = (bedOther.bed_pbvxVertex0->bvx_vRelative-bedOther.bed_pbvxVertex1->bvx_vRelative).Length();
    FLOAT fA0B0 = (bed_pbvxVertex0->bvx_vRelative-bedOther.bed_pbvxVertex0->bvx_vRelative).Length();
    FLOAT fA0B1 = (bed_pbvxVertex0->bvx_vRelative-bedOther.bed_pbvxVertex1->bvx_vRelative).Length();
    FLOAT fA1B0 = (bed_pbvxVertex1->bvx_vRelative-bedOther.bed_pbvxVertex0->bvx_vRelative).Length();
    FLOAT fA1B1 = (bed_pbvxVertex1->bvx_vRelative-bedOther.bed_pbvxVertex1->bvx_vRelative).Length();
    FLOAT fB0A0 = (bedOther.bed_pbvxVertex0->bvx_vRelative-bed_pbvxVertex0->bvx_vRelative).Length();
    FLOAT fB0A1 = (bedOther.bed_pbvxVertex0->bvx_vRelative-bed_pbvxVertex1->bvx_vRelative).Length();
    FLOAT fB1A0 = (bedOther.bed_pbvxVertex1->bvx_vRelative-bed_pbvxVertex0->bvx_vRelative).Length();
    FLOAT fB1A1 = (bedOther.bed_pbvxVertex1->bvx_vRelative-bed_pbvxVertex1->bvx_vRelative).Length();

    return
      Abs(fB0B1-fA0B0-fA0B1)<TOUCHEPSILON||
      Abs(fB0B1-fA1B0-fA1B1)<TOUCHEPSILON||
      Abs(fA0A1-fB0A0-fB0A1)<TOUCHEPSILON||
      Abs(fA0A1-fB1A0-fB1A1)<TOUCHEPSILON;
  }
  return FALSE;
}

/* Test if this polygon touches another one. */
BOOL CBrushPolygon::TouchesInAnySector(CBrushPolygon &bpoOther)
{
  INDEX ctEdgesThis  = bpo_abpePolygonEdges.Count();
  INDEX ctEdgesOther = bpoOther.bpo_abpePolygonEdges.Count();
  // for each edge in this polygon
  {for(INDEX ibpeThis=0; ibpeThis<ctEdgesThis; ibpeThis++) {
    CBrushEdge &bedThis = *bpo_abpePolygonEdges[ibpeThis].bpe_pbedEdge;
    // for each edge in other polygon
    {for(INDEX ibpeOther=0; ibpeOther<ctEdgesOther; ibpeOther++) {
      CBrushEdge &bedOther = *bpoOther.bpo_abpePolygonEdges[ibpeOther].bpe_pbedEdge;
      // if they touch
      if (bedThis.TouchesInAnySector(bedOther)) {
        // polygons touch
        return TRUE;
      }
    }}
  }}
  // if no two edges touch, the polygons don't touch
  return FALSE;
}
BOOL CBrushPolygon::TouchesInSameSector(CBrushPolygon &bpoOther)
{
  INDEX ctEdgesThis  = bpo_abpePolygonEdges.Count();
  INDEX ctEdgesOther = bpoOther.bpo_abpePolygonEdges.Count();
  // for each edge in this polygon
  {for(INDEX ibpeThis=0; ibpeThis<ctEdgesThis; ibpeThis++) {
    CBrushEdge &bedThis = *bpo_abpePolygonEdges[ibpeThis].bpe_pbedEdge;
    // for each edge in other polygon
    {for(INDEX ibpeOther=0; ibpeOther<ctEdgesOther; ibpeOther++) {
      CBrushEdge &bedOther = *bpoOther.bpo_abpePolygonEdges[ibpeOther].bpe_pbedEdge;
      // if they touch
      if (bedThis.TouchesInSameSector(bedOther)) {
        // polygons touch
        return TRUE;
      }
    }}
  }}
  // if no two edges touch, the polygons don't touch
  return FALSE;
}

// find minimum distance of a given point from the polygon edges
// see comp.graphics.algorithms.faq 1.02
FLOAT CBrushPolygon::GetDistanceFromEdges(const FLOAT3D &vC)
{
  INDEX ctEdges = bpo_abpePolygonEdges.Count();
  // start with infinite squared distance (all is calculated in squared distances!)
  FLOAT fMinD2 = +1E20f;
  // for each edge
  {for(INDEX ibpe=0; ibpe<ctEdges; ibpe++) {
    // for all edges in the polygon
    FOREACHINSTATICARRAY(bpo_abpePolygonEdges, CBrushPolygonEdge, itbpePolygonEdge) {
      // get edge vertices (edge direction is irrelevant here!)
      const FLOAT3D &vA = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex0->bvx_vAbsolute;
      const FLOAT3D &vB = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex1->bvx_vAbsolute;
      // compute general vectors needed
      FLOAT3D vAC = vC-vA;
      FLOAT3D vAB = vB-vA;
      // get parameter of the P - orthogonal projection of C onto AB
      FLOAT fR = (vAC%vAB)/(vAB%vAB);
      FLOAT fD2 = 0.0f;
      // if before A
      if (fR<0) {
        // get squared distance AC
        fD2 = vAC%vAC;
      // if after B
      } else if (fR>1) {
        // get squared distance BC
        FLOAT3D vBC = vC-vB;
        fD2 = vBC%vBC;
      // if between
      } else {
        // find PC
        FLOAT3D vBC = vC-vB;
        FLOAT3D vPC = vAC+(vBC-vAC)*fR;
        // get squared distance PC
        fD2 = vPC%vPC;
      }
      // update minimal squared distance
      if (fD2<fMinD2) {
        fMinD2=fD2;
      }
    }
  }}

  // return square root of the minimum squared distance
  return Sqrt(fMinD2);
}

CBrushPolygon &CBrushPolygon::CopyPolygon(CBrushPolygon &bp)
{
  bpo_pbplPlane=bp.bpo_pbplPlane;
  bpo_abpePolygonEdges.MoveArray(bp.bpo_abpePolygonEdges);
  bpo_apbvxTriangleVertices.MoveArray(bp.bpo_apbvxTriangleVertices);
  bpo_aiTriangleElements.MoveArray(bp.bpo_aiTriangleElements);
  CopyTextures(bp);
  bpo_colColor=bp.bpo_colColor;
  bpo_ulFlags=bp.bpo_ulFlags;
  bpo_colShadow=bp.bpo_colShadow;
  bpo_mdShadow=bp.bpo_mdShadow;
  bpo_bppProperties=bp.bpo_bppProperties;
  bpo_pspoScreenPolygon=NULL;
  bpo_boxBoundingBox=bp.bpo_boxBoundingBox;
  bpo_pbscSector=bp.bpo_pbscSector;
  bpo_rsOtherSideSectors.Clear();
  //bpo_lhShadingInfos; // don't copy or anything, it's a CListHead which must not be copied
  bpo_iInWorld=bp.bpo_iInWorld;
  return *this;
}


// get amount of memory used by this object
SLONG CBrushPolygon::GetUsedMemory(void)
{
  // basic size of class
  SLONG slUsedMemory = sizeof(CBrushPolygon);
  // add arrays
  slUsedMemory += bpo_abpePolygonEdges.sa_Count      * sizeof(CBrushPolygonEdge);
  slUsedMemory += bpo_apbvxTriangleVertices.sa_Count * sizeof(CBrushVertex*);
  slUsedMemory += bpo_aiTriangleElements.sa_Count    * sizeof(INDEX);
  slUsedMemory += bpo_rsOtherSideSectors.Count()     * sizeof(CRelationSrc);
  // done
  return slUsedMemory;
}
