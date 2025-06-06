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
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/Math/Float.h>
#include <Engine/Math/Object3D.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/DynamicArray.cpp>

/*
 * Fill a 3d object from a selection in a brush mip.
 */
void CBrushMip::ToObject3D(
    CObject3D &ob,
    CBrushSectorSelection &selbscToCopy)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  // get number of sectors in the selection
  INDEX ctSectors = selbscToCopy.Count();
  // create that much sectors in the object
  CObjectSector *poscSectors = ob.ob_aoscSectors.New(ctSectors);
  // for each sector in the selection mip
  FOREACHINDYNAMICCONTAINER(selbscToCopy, CBrushSector, itbsc) {
    // fill corresponding sector in object from it
    itbsc->ToObjectSector(*poscSectors++);
  }

  // optimize the object, to remove unused elements
//  CBrush3D::OptimizeObject3D(ob);
}

/*
 * Fill a 3d object from a selection in a brush mip.
 */
void CBrushMip::ToObject3D(
    CObject3D &ob,
    CBrushSectorSelectionForCSG &selbscToCopy)
{
  CSetFPUPrecision sfp(FPT_53BIT);
  // get number of sectors in the selection
  INDEX ctSectors = selbscToCopy.Count();
  // create that much sectors in the object
  CObjectSector *poscSectors = ob.ob_aoscSectors.New(ctSectors);
  // for each sector in the selection mip
  FOREACHINDYNAMICCONTAINER(selbscToCopy, CBrushSector, itbsc) {
    // fill corresponding sector in object from it
    itbsc->ToObjectSector(*poscSectors++);
  }

  // optimize the object, to remove unused elements
//  CBrush3D::OptimizeObject3D(ob);
}

/*
 * Fill an object sector from a sector in brush.
 */
void CBrushSector::ToObjectSector(CObjectSector &osc)
{
  // copy sector color and ambient
  osc.osc_colColor   = bsc_colColor;
  osc.osc_colAmbient = bsc_colAmbient;
  osc.osc_ulFlags[0]    = bsc_ulFlags;
  osc.osc_ulFlags[1]    = bsc_ulFlags2;
  osc.osc_ulFlags[2]    = bsc_ulVisFlags;
  osc.osc_strName    = bsc_strName;
  // lock the object elements
  osc.LockAll();
  // lock the brush elements
  LockAll();

  /* Copy vertices. */

  // get the number of vertices in brush
  INDEX ctVertices = bsc_abvxVertices.Count();
  // create that much vertices in object
  osc.osc_aovxVertices.New(ctVertices);
  // copy all vertices
  for(INDEX iVertex=0; iVertex<ctVertices; iVertex++) {
    osc.osc_aovxVertices[iVertex] = bsc_abvxVertices[iVertex].bvx_vdPreciseRelative;
  }

  /* Copy planes. */

  // get the number of planes in brush
  INDEX ctPlanes = bsc_abplPlanes.Count();
  // create that much planes in object
  osc.osc_aoplPlanes.New(ctPlanes);
  // copy all planes
  for(INDEX iPlane=0; iPlane<ctPlanes; iPlane++) {
    osc.osc_aoplPlanes[iPlane] = bsc_abplPlanes[iPlane].bpl_pldPreciseRelative;
  }

  /* Copy edges. */

  // get the number of edges in brush
  INDEX ctEdges = bsc_abedEdges.Count();
  // create that much edges in object
  osc.osc_aoedEdges.New(ctEdges);
  // for all edges in brush
  for(INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
    CObjectEdge &oed = osc.osc_aoedEdges[iEdge];  // object edge alias
    CBrushEdge &bed  =     bsc_abedEdges[iEdge];  // brush edge alias
    // set the object edge
    oed.oed_Vertex0 = &osc.osc_aovxVertices[bsc_abvxVertices.Index(bed.bed_pbvxVertex0)];
    oed.oed_Vertex1 = &osc.osc_aovxVertices[bsc_abvxVertices.Index(bed.bed_pbvxVertex1)];
  }

  /* Copy polygons. */

  // get the number of polygons in brush
  INDEX ctPolygons = bsc_abpoPolygons.Count();
  // create that much polygons and materials in object
  osc.osc_aopoPolygons.New(ctPolygons);
  osc.osc_aomtMaterials.New(ctPolygons);

  // for all polygons in brush
  for(INDEX iPolygon=0; iPolygon<ctPolygons; iPolygon++) {
    CBrushPolygon  &bpo =    bsc_abpoPolygons[iPolygon];  // brush polygon alias
    CObjectPolygon &opo =osc.osc_aopoPolygons[iPolygon];  // object polygon alias
    CObjectMaterial &omt =osc.osc_aomtMaterials[iPolygon];  // object material alias

    // get plane
    opo.opo_Plane = &osc.osc_aoplPlanes[bsc_abplPlanes.Index(bpo.bpo_pbplPlane)];
    // set object material
    opo.opo_Material = &omt;
    // set object material name from texture
    omt.omt_Name = bpo.bpo_abptTextures[0].bpt_toTexture.GetName();
    omt.omt_strName2 = bpo.bpo_abptTextures[1].bpt_toTexture.GetName();
    omt.omt_strName3 = bpo.bpo_abptTextures[2].bpt_toTexture.GetName();

    // set polygon color
    opo.opo_colorColor = bpo.bpo_colColor;
    // set polygon mapping
    opo.opo_amdMappings[0] = bpo.bpo_abptTextures[0].bpt_mdMapping;
    opo.opo_amdMappings[1] = bpo.bpo_abptTextures[1].bpt_mdMapping;
    opo.opo_amdMappings[2] = bpo.bpo_abptTextures[2].bpt_mdMapping;
    opo.opo_amdMappings[3] = bpo.bpo_mdShadow;

    // set polygon flags
    opo.opo_ulFlags = bpo.bpo_ulFlags;
    // set portal backtracking flag
    if (bpo.bpo_ulFlags&OPOF_PORTAL) {
      opo.opo_ulFlags |= BPOF_WASPORTAL;
    } else {
      opo.opo_ulFlags &= ~BPOF_WASPORTAL;
    }
    // if polygon is not selected
    if (!(bpo.bpo_ulFlags&BPOF_SELECTED)) {
      // mark as ignored for csg (if csg ignoring is enabled - when doing split polygons)
      opo.opo_ulFlags|=OPOF_IGNOREDBYCSG;
    }
    // copy polygon properties
    const int sizeTextureProperties = sizeof(bpo.bpo_abptTextures[0].bpt_auProperties);
    const int sizePolygonProperties = sizeof(CBrushPolygonProperties);
    ASSERT(sizeof(opo.opo_ubUserData)>=sizePolygonProperties+3*sizeTextureProperties);
    UBYTE *pubUserData = (UBYTE*)&opo.opo_ubUserData;
    memcpy(pubUserData, &bpo.bpo_bppProperties, sizePolygonProperties);
    memcpy(pubUserData+sizePolygonProperties+0*sizeTextureProperties,
      &bpo.bpo_abptTextures[0].bpt_auProperties,
      sizeTextureProperties);
    memcpy(pubUserData+sizePolygonProperties+1*sizeTextureProperties,
      &bpo.bpo_abptTextures[1].bpt_auProperties,
      sizeTextureProperties);
    memcpy(pubUserData+sizePolygonProperties+2*sizeTextureProperties,
      &bpo.bpo_abptTextures[2].bpt_auProperties,
      sizeTextureProperties);
    *(ULONG*)(pubUserData+sizePolygonProperties+3*sizeTextureProperties) = bpo.bpo_colShadow;

    opo.opo_PolygonEdges.Lock();
    // get the number of edges in brush polygon
    INDEX ctPolygonEdges = bpo.bpo_abpePolygonEdges.Count();
    // create that much edges in object polygon
    opo.opo_PolygonEdges.New(ctPolygonEdges);

    // for all edges in brush polygon
    INDEX iPolygonEdge=0;
    FOREACHINSTATICARRAY(bpo.bpo_abpePolygonEdges, CBrushPolygonEdge, itbpe) {
      // get corresponding polygon edge in object polygon
      CObjectPolygonEdge &ope = opo.opo_PolygonEdges[iPolygonEdge];
      // set edge reference
      ope.ope_Edge = &osc.osc_aoedEdges[bsc_abedEdges.Index(itbpe->bpe_pbedEdge)];
      // set backward flag
      ope.ope_Backward = itbpe->bpe_bReverse;

      iPolygonEdge++;
    }
    opo.opo_PolygonEdges.Unlock();
  }

  // unlock the object elements
  osc.UnlockAll();
  // unlock the brush elements
  UnlockAll();
}
