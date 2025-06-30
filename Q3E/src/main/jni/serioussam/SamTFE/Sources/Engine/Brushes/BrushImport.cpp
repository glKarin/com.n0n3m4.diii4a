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
#include <Engine/Math/Float.h>
#include <Engine/Math/Object3D.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/Brushes/BrushTransformed.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Math/Projection_DOUBLE.h>

/*
 * Add 3d object as new mip brush.
 */
void CBrush3D::AddMipBrushFromObject3D_t(CObject3D &ob, FLOAT fSwitchDistance) // throw char *
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  // create one brush mip
  CBrushMip *pbmBrushMip = new CBrushMip;
  // add it to the brush
  br_lhBrushMips.AddTail(pbmBrushMip->bm_lnInBrush);
  pbmBrushMip->bm_pbrBrush = this;
  pbmBrushMip->bm_fMaxDistance = fSwitchDistance;

  // add the object to the brush mip
  pbmBrushMip->AddFromObject3D_t(ob);
}

/*
 * Fill a brush from 3d object.
 */
void CBrush3D::FromObject3D_t(CObject3D &ob) // throw char *
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  // clear this brush in case there is something in it
  Clear();

  // create one brush mip
  CBrushMip *pbmBrushMip = new CBrushMip;
  // add it to the brush
  br_lhBrushMips.AddTail(pbmBrushMip->bm_lnInBrush);
  pbmBrushMip->bm_pbrBrush = this;

  // add the object to the brush mip
  pbmBrushMip->AddFromObject3D_t(ob);
}

/*
 * Add an object3d to brush. (returns pointer to the first created sector)
 */
CBrushSector *CBrushMip::AddFromObject3D_t(CObject3D &ob) // throw char *
{
  CSetFPUPrecision sfp(FPT_53BIT);
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_ADDFROMOBJECT3D);
  // optimize the object, to remove replicated and unused elements and find edge inverses
  CBrush3D::OptimizeObject3D(ob);

// turn this on to dump result of all CSG operations
#ifndef NDEBUG
//  ob.DebugDump();
#endif //NDEBUG

  // create as much new sectors in brush mip as there are sectors in object
  CBrushSector *pbscSectors = bm_abscSectors.New(ob.ob_aoscSectors.Count());
  CBrushSector *pbscFirstSector = pbscSectors;
  // for each sector in the object
  FOREACHINDYNAMICARRAY(ob.ob_aoscSectors, CObjectSector, itosc) {
    // set brush sector's pointer to the brush
    pbscSectors->bsc_pbmBrushMip = this;
    // fill one brush sector from it
    pbscSectors->FromObjectSector_t(*itosc);

    pbscSectors++;
  }

  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_ADDFROMOBJECT3D);
  // return pointer to first created sector
  return pbscFirstSector;
}

/*
 * Fill a brush sector from a sector in object3d.
 */
void CBrushSector::FromObjectSector_t(CObjectSector &osc) // throw char *
{
  // copy sector color
  bsc_colColor   = osc.osc_colColor;
  bsc_colAmbient = osc.osc_colAmbient;
  bsc_ulFlags    = osc.osc_ulFlags[0] & ~(BSCF_SELECTED|BSCF_SELECTEDFORCSG);
  bsc_ulFlags2   = osc.osc_ulFlags[1];
  bsc_ulVisFlags = osc.osc_ulFlags[2];
  bsc_strName    = osc.osc_strName;

  // lock the object elements
  osc.LockAll();
  // lock the brush elements
  LockAll();

  /* Copy vertices. */

  // get the number of vertices in object
  INDEX ctVertices = osc.osc_aovxVertices.Count();
  // create that much vertices in brush
  bsc_abvxVertices.New(ctVertices);
  bsc_awvxVertices.New(ctVertices);
  // copy all vertices and set their indices
  for(INDEX iVertex=0; iVertex<ctVertices; iVertex++) {
    bsc_abvxVertices[iVertex].bvx_vdPreciseRelative = osc.osc_aovxVertices[iVertex];
    bsc_abvxVertices[iVertex].bvx_pbscSector = this;
    osc.osc_aovxVertices[iVertex].ovx_Index = iVertex;
  }

  /* Copy planes. */

  // get the number of planes in object
  INDEX ctPlanes = osc.osc_aoplPlanes.Count();
  // create that much planes in brush
  bsc_abplPlanes.New(ctPlanes);
  bsc_awplPlanes.New(ctPlanes);
  // copy all planes and set their indices
  for(INDEX iPlane=0; iPlane<ctPlanes; iPlane++) {
    bsc_abplPlanes[iPlane].bpl_pldPreciseRelative = osc.osc_aoplPlanes[iPlane];
    osc.osc_aoplPlanes[iPlane].opl_Index = iPlane;
  }

  /* Copy edges. */

  // get the number of edges in object
  INDEX ctEdges = osc.osc_aoedEdges.Count();
  // create that much edges in brush
  bsc_abedEdges.New(ctEdges);
  bsc_awedEdges.New(ctEdges);
  // for all edges in object
  for(INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
    CObjectEdge &oed = osc.osc_aoedEdges[iEdge];  // object edge alias
    CBrushEdge &bed = bsc_abedEdges[iEdge];      // brush edge alias
    CWorkingEdge &wed = bsc_awedEdges[iEdge];
    // set the brush edge
    bed.bed_pbvxVertex0 = &bsc_abvxVertices[oed.oed_Vertex0->ovx_Index];
    bed.bed_pbvxVertex1 = &bsc_abvxVertices[oed.oed_Vertex1->ovx_Index];
    // set the working edge
    bed.bed_pwedWorking = &wed;
    wed.wed_iwvx0 = oed.oed_Vertex0->ovx_Index;
    wed.wed_iwvx1 = oed.oed_Vertex1->ovx_Index;
    // set object edge index
    oed.oed_Index = iEdge;
  }

  /* Copy polygons. */

  // get the number of polygons in object
  INDEX ctPolygons = osc.osc_aopoPolygons.Count();
  // create that much polygons in brush
  bsc_abpoPolygons.New(ctPolygons);

  // copy all polygons and set their indices
  for(INDEX iPolygon=0; iPolygon<ctPolygons; iPolygon++) {
    CBrushPolygon  &bpo =    bsc_abpoPolygons[iPolygon];  // brush polygon alias
    CObjectPolygon &opo = osc.osc_aopoPolygons[iPolygon];  // object polygon alias

    // get plane
    bpo.bpo_pbplPlane = &bsc_abplPlanes[opo.opo_Plane->opl_Index];
    // get texture from object material
    bpo.bpo_abptTextures[0].bpt_toTexture.SetData_t(opo.opo_Material->omt_Name);
    bpo.bpo_abptTextures[1].bpt_toTexture.SetData_t(opo.opo_Material->omt_strName2);
    bpo.bpo_abptTextures[2].bpt_toTexture.SetData_t(opo.opo_Material->omt_strName3);
    // set polygon index
    opo.opo_Index = iPolygon;
    // set polygon color
    bpo.bpo_colColor = opo.opo_colorColor;
    // set polygon mapping
    bpo.bpo_abptTextures[0].bpt_mdMapping = opo.opo_amdMappings[0];
    bpo.bpo_abptTextures[1].bpt_mdMapping = opo.opo_amdMappings[1];
    bpo.bpo_abptTextures[2].bpt_mdMapping = opo.opo_amdMappings[2];
    bpo.bpo_mdShadow                      = opo.opo_amdMappings[3];
    // set sector pointer
    bpo.bpo_pbscSector = this;

    // copy polygon properties
    const int sizeTextureProperties = sizeof(bpo.bpo_abptTextures[0].bpt_auProperties);
    const int sizePolygonProperties = sizeof(CBrushPolygonProperties);
    ASSERT(sizeof(opo.opo_ubUserData)>=sizePolygonProperties+3*sizeTextureProperties);
    UBYTE *pubUserData = (UBYTE*)&opo.opo_ubUserData;
    memcpy(&bpo.bpo_bppProperties, pubUserData, sizePolygonProperties);
    memcpy(&bpo.bpo_abptTextures[0].bpt_auProperties,
      pubUserData+sizePolygonProperties+0*sizeTextureProperties,
      sizeTextureProperties);
    memcpy(&bpo.bpo_abptTextures[1].bpt_auProperties,
      pubUserData+sizePolygonProperties+1*sizeTextureProperties,
      sizeTextureProperties);
    memcpy(&bpo.bpo_abptTextures[2].bpt_auProperties,
      pubUserData+sizePolygonProperties+2*sizeTextureProperties,
      sizeTextureProperties);
    bpo.bpo_colShadow = *(ULONG*)(pubUserData+sizePolygonProperties+3*sizeTextureProperties),

    // set polygon flags
    bpo.bpo_ulFlags = opo.opo_ulFlags & ~(OPOF_IGNOREDBYCSG|BPOF_SELECTED);

    // if the polygon was just created
    if(!(bpo.bpo_ulFlags&BPOF_WASBRUSHPOLYGON)) {
      // initialize its textures and properties properly
      bpo.bpo_bppProperties.bpp_ubShadowBlend = 1;
      bpo.bpo_colShadow = C_WHITE|CT_OPAQUE;

      bpo.bpo_abptTextures[0].s.bpt_colColor = C_WHITE|CT_OPAQUE;
      bpo.bpo_abptTextures[0].s.bpt_ubFlags = BPTF_DISCARDABLE;
      bpo.bpo_abptTextures[0].s.bpt_ubScroll = 0;
      bpo.bpo_abptTextures[0].s.bpt_ubBlend = BPT_BLEND_OPAQUE;

      bpo.bpo_abptTextures[1].s.bpt_colColor = C_WHITE|CT_OPAQUE;
      bpo.bpo_abptTextures[1].s.bpt_ubFlags = BPTF_DISCARDABLE;
      bpo.bpo_abptTextures[1].s.bpt_ubScroll = 0;
      bpo.bpo_abptTextures[1].s.bpt_ubBlend = BPT_BLEND_SHADE;

      bpo.bpo_abptTextures[2].s.bpt_colColor = C_WHITE|CT_OPAQUE;
      bpo.bpo_abptTextures[2].s.bpt_ubFlags = BPTF_DISCARDABLE;
      bpo.bpo_abptTextures[2].s.bpt_ubScroll = 0;
      bpo.bpo_abptTextures[2].s.bpt_ubBlend = BPT_BLEND_SHADE;

      bpo.bpo_ulFlags|=BPOF_WASBRUSHPOLYGON;
    }

    // if it was a wall, but it became a portal now
    if (!(bpo.bpo_ulFlags&BPOF_WASPORTAL) && (bpo.bpo_ulFlags&OPOF_PORTAL)) {
      // turn on usual portal flags
      bpo.bpo_ulFlags |= (BPOF_PASSABLE|BPOF_PORTAL);
      // make its first texture translucent
      bpo.bpo_abptTextures[0].s.bpt_ubBlend = BPT_BLEND_BLEND;
      // make its shadow additive
      bpo.bpo_bppProperties.bpp_ubShadowBlend = BPT_BLEND_ADD;

    // if it was a portal, but it became a wall now
    } else if ((bpo.bpo_ulFlags&BPOF_WASPORTAL) && !(bpo.bpo_ulFlags&OPOF_PORTAL)) {
      // turn off usual portal flags
      bpo.bpo_ulFlags &= ~(BPOF_PASSABLE|BPOF_PORTAL);
      // make its first texture opaque
      bpo.bpo_abptTextures[0].s.bpt_ubBlend = BPT_BLEND_OPAQUE;
      // make its shadow shading
      bpo.bpo_bppProperties.bpp_ubShadowBlend = BPT_BLEND_SHADE;
    }

    // get the brush of this sector
    CBrush3D *pbr = bsc_pbmBrushMip->bm_pbrBrush;
    ASSERT(pbr!=NULL);
    // if the brush is field
    if (pbr->br_pfsFieldSettings!=NULL) {
      // set polygon flags for fields
      bpo.bpo_ulFlags|=BPOF_PORTAL|BPOF_PASSABLE;
    }

    // get the number of edges in object polygon
    INDEX ctPolygonEdges = opo.opo_PolygonEdges.Count();
    // create that much edges in brush polygon
    bpo.bpo_abpePolygonEdges.New(ctPolygonEdges);

    // for all edges in object polygon
    INDEX iPolygonEdge=0;
    FOREACHINDYNAMICARRAY(opo.opo_PolygonEdges, CObjectPolygonEdge, itope) {
      // get corresponding polygon edge in brush polygon
      CBrushPolygonEdge &bpe = bpo.bpo_abpePolygonEdges[iPolygonEdge];
      // set edge reference
      bpe.bpe_pbedEdge = &bsc_abedEdges[itope->ope_Edge->oed_Index];
      // set backward flag
      bpe.bpe_bReverse = itope->ope_Backward;

      iPolygonEdge++;
    }
  }

  // unlock the object elements
  osc.UnlockAll();
  // unlock the brush elements
  UnlockAll();

  // update changed sector's data after dragging vertices or importing
  UpdateSector();
}
