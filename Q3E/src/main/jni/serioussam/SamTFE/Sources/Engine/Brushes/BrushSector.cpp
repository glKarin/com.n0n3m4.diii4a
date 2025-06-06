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
#include <Engine/Math/Geometry.inl>
#include <Engine/Base/Console.h>
#include <Engine/World/World.h>
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/Math/Projection_DOUBLE.h>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Math/Float.h>
#include <Engine/Math/OBBox.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Templates/BSP.h>
#include <Engine/Templates/BSP_internal.h>

//template CDynamicArray<CBrushVertex>;

CBrushSector::CBrushSector(const CBrushSector &c) 
: bsc_bspBSPTree(*new DOUBLEbsptree3D)
{ 
  ASSERT(FALSE);
};

CBrushSector &CBrushSector::operator=(const CBrushSector &c)
{
  ASSERT(FALSE);
  return *this;
};

extern void AssureFPT_53(void);

/* Default constructor. */
CBrushSector::CBrushSector(void) 
: bsc_ulFlags(0)
, bsc_ulFlags2(0)
, bsc_ulTempFlags(0)
, bsc_ulVisFlags(0)
, bsc_strName("")
, bsc_bspBSPTree(*new DOUBLEbsptree3D)
{

};
CBrushSector::~CBrushSector(void)
{
  delete &bsc_bspBSPTree;
}

/*
 * Calculate bounding boxs of polygons in this sector.
 */
void CBrushSector::CalculateBoundingBoxes(CSimpleProjection3D_DOUBLE &prRelativeToAbsolute)
{
  // assure that floating point precision is 53 bits
  AssureFPT_53();

  // discard portal-sector links to this sector
  extern BOOL _bDontDiscardLinks;
  if (!(bsc_ulTempFlags&BSCTF_PRELOADEDLINKS) && !_bDontDiscardLinks) {
    bsc_rdOtherSidePortals.Clear();
  }

  // create an array of precise vertices in absolute space
  CStaticArray<DOUBLE3D> avdAbsoluteVertices;
  avdAbsoluteVertices.New(bsc_abvxVertices.Count());
  bsc_boxRelative = FLOATaabbox3D();

  // for each vertex in sector
  for(INDEX ivx=0; ivx<bsc_abvxVertices.Count(); ivx++) {
    // make the original vertex point to absolute vertex
    bsc_abvxVertices[ivx].bvx_pvdPreciseAbsolute = &avdAbsoluteVertices[ivx];
    bsc_abvxVertices[ivx].bvx_pwvxWorking = &bsc_awvxVertices[ivx];
    // transform original there
    prRelativeToAbsolute.ProjectCoordinate(bsc_abvxVertices[ivx].bvx_vdPreciseRelative, avdAbsoluteVertices[ivx]);
    // remember the absolute and relative coordinates in lower precision
    bsc_abvxVertices[ivx].bvx_vAbsolute = DOUBLEtoFLOAT(avdAbsoluteVertices[ivx]);
    bsc_awvxVertices[ivx].wvx_vRelative =
    bsc_abvxVertices[ivx].bvx_vRelative = DOUBLEtoFLOAT(bsc_abvxVertices[ivx].bvx_vdPreciseRelative);
    // add vertex to relative box
    bsc_boxRelative |= bsc_abvxVertices[ivx].bvx_vRelative;
  }

  // create an array of precise planes in absolute space
  CStaticArray<DOUBLEplane3D> apldAbsolutePlanes;
  apldAbsolutePlanes.New(bsc_abplPlanes.Count());

  // for each plane in sector
  for(INDEX ipl=0; ipl<bsc_abplPlanes.Count(); ipl++){
    // make the original plane point to absolute plane
    bsc_abplPlanes[ipl].bpl_ppldPreciseAbsolute = &apldAbsolutePlanes[ipl];
    bsc_abplPlanes[ipl].bpl_pwplWorking = &bsc_awplPlanes[ipl];
    // transform original there
    prRelativeToAbsolute.Project(bsc_abplPlanes[ipl].bpl_pldPreciseRelative, apldAbsolutePlanes[ipl]);
    // remember the absolute and relative coordinates in lower precision
    bsc_abplPlanes[ipl].bpl_plAbsolute = DOUBLEtoFLOAT(apldAbsolutePlanes[ipl]);
    bsc_awplPlanes[ipl].wpl_plRelative =
    bsc_abplPlanes[ipl].bpl_plRelative = DOUBLEtoFLOAT(bsc_abplPlanes[ipl].bpl_pldPreciseRelative);
    // make default mapping coordinates for the plane
    bsc_awplPlanes[ipl].wpl_mvRelative.FromPlane(bsc_awplPlanes[ipl].wpl_plRelative);
    // remember major axes of the plane in apsolute space
    GetMajorAxesForPlane(bsc_abplPlanes[ipl].bpl_plAbsolute,
      bsc_abplPlanes[ipl].bpl_iPlaneMajorAxis1,
      bsc_abplPlanes[ipl].bpl_iPlaneMajorAxis2);
  }

  // clear the bounding box of the sector
  bsc_boxBoundingBox = FLOATaabbox3D();
  // for all polygons in this sector
  {FOREACHINSTATICARRAY(bsc_abpoPolygons, CBrushPolygon, itbpo) {
    // calculate bounding box for that polygon
    itbpo->CalculateBoundingBox();
    // add the polygon's bounding box to sector's bounding box
    bsc_boxBoundingBox |= itbpo->bpo_boxBoundingBox;
  }}

  // if the bsp tree is not preloaded
  if (!(bsc_ulTempFlags&BSCTF_PRELOADEDBSP)) {
    // clear BSP tree of the sector
    bsc_bspBSPTree.Destroy();
    // if the brush is zoning or field
    CEntity *pen = bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
    if (pen!=NULL && 
      ((pen->en_ulFlags&ENF_ZONING) || pen->en_RenderType==CEntity::RT_FIELDBRUSH) ) {
      // create an array of bsp polygons for sector polygons
      INDEX ctPolygons = bsc_abpoPolygons.Count();
      CDynamicArray< BSPPolygon<DOUBLE, 3> > arbpoPolygons;
      arbpoPolygons.New(ctPolygons);

      // for all polygons in this sector
      arbpoPolygons.Lock();
      {for(INDEX iPolygon=0; iPolygon<ctPolygons; iPolygon++){
        // create a BSP polygon from the brush polygon
        CBrushPolygon         &brpo = bsc_abpoPolygons[iPolygon];
        BSPPolygon<DOUBLE, 3> &bspo = arbpoPolygons[iPolygon];
        brpo.CreateBSPPolygon(bspo);
      }}
      arbpoPolygons.Unlock();

      // create the bsp tree from the bsp polygons
      bsc_bspBSPTree.Create(arbpoPolygons);
    }
  }
  // clear preloading flags
  bsc_ulTempFlags&=~BSCTF_PRELOADEDBSP;
  bsc_ulTempFlags&=~BSCTF_PRELOADEDLINKS;

// if in debug version
#ifndef NDEBUG
  // for each vertex in sector
  {for(INDEX ivx=0; ivx<bsc_abvxVertices.Count(); ivx++){
    // discard absolute vertex pointer
    bsc_abvxVertices[ivx].bvx_pvdPreciseAbsolute = NULL;
  }}
  // for each plane in sector
  {for(INDEX ipl=0; ipl<bsc_abplPlanes.Count(); ipl++){
    // discard absolute plane pointer
    bsc_abplPlanes[ipl].bpl_ppldPreciseAbsolute = NULL;
  }}
#endif

  // !!!! remove this after loading all old levels
  // but should save size of polygon in mex (not shadow map)
  // NOTE: but this is also called in FromObject3D (?!)
  // for each polygon
  {for(INDEX iPolygon=0; iPolygon<bsc_abpoPolygons.Count(); iPolygon++) {
    CBrushPolygon  &bpo =    bsc_abpoPolygons[iPolygon];  // brush polygon alias
    // if the shadow map of the polygon is not initialized
    if (bpo.bpo_smShadowMap.sm_mexWidth==0 || bpo.bpo_smShadowMap.sm_pixPolygonSizeU<0) {
      // initialize the shadow map of the polygon
      bpo.InitializeShadowMap();
    }
  }}
}


/* Uncache lightmaps on all shadows on the sector. */
void CBrushSector::UncacheLightMaps(void)
{
  // for all polygons in this sector
  {FOREACHINSTATICARRAY(bsc_abpoPolygons, CBrushPolygon, itbpo) {
    // uncache shadow map on the polygon
    itbpo->bpo_smShadowMap.Uncache();
  }}
}
/* Find and remember all entities in this sector. */
void CBrushSector::FindEntitiesInSector(void)
{
  // assure that floating point precision is 53 bits
  CSetFPUPrecision sfp(FPT_53BIT);

  // get the entity of this sector's brush
  CEntity *penEntity = bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
  // if the brush entity is not zoning
  if (penEntity==NULL || !(penEntity->en_ulFlags&ENF_ZONING)) {
    // do nothing
    return;
  }

  // unset spatial clasification
  bsc_rsEntities.Clear();

  // remember sectors obbox
  FLOATobbox3D boxSector(bsc_boxBoundingBox);

  // for each entity in the world
  {FOREACHINDYNAMICCONTAINER(penEntity->en_pwoWorld->wo_cenEntities, CEntity, iten) {
    // if not in spatial clasification
    if (iten->en_fSpatialClassificationRadius<0) {
      // skip it
      continue;
    }
    // get bounding sphere
    FLOAT fSphereRadius = iten->en_fSpatialClassificationRadius;
    const FLOAT3D &vSphereCenter = iten->en_plPlacement.pl_PositionVector;
    // if the sector's bounding box has contact with the sphere
    if(bsc_boxBoundingBox.TouchesSphere(vSphereCenter, fSphereRadius)) {
      
      // if the sphere is inside the sector
      if (bsc_bspBSPTree.TestSphere(
          FLOATtoDOUBLE(vSphereCenter), FLOATtoDOUBLE(fSphereRadius))>=0) {
        // make oriented bounding box of the entity
        FLOATobbox3D boxEntity(iten->en_boxSpatialClassification, 
          iten->en_plPlacement.pl_PositionVector, iten->en_mRotation);

        // if the box is inside the sector
        if (boxSector.HasContactWith(boxEntity) &&
          bsc_bspBSPTree.TestBox(FLOATtoDOUBLE(boxEntity))>=0) {
          // relate the entity to the sector
          if (iten->en_RenderType==CEntity::RT_BRUSH
            ||iten->en_RenderType==CEntity::RT_FIELDBRUSH
            ||iten->en_RenderType==CEntity::RT_TERRAIN) {  // brushes first
            AddRelationPairHeadHead(bsc_rsEntities, iten->en_rdSectors);
          } else {
            AddRelationPairTailTail(bsc_rsEntities, iten->en_rdSectors);
          }
        }
      }
    }
  }}
}

/*
 * Clear the object.
 */
void CBrushSector::Clear(void)
{
  bsc_abvxVertices.Clear();
  bsc_awvxVertices.Clear();
  bsc_abedEdges.Clear();
  bsc_awedEdges.Clear();
  bsc_abplPlanes.Clear();
  bsc_awplPlanes.Clear();
  bsc_abpoPolygons.Clear();
  bsc_rdOtherSidePortals.Clear();
  bsc_rsEntities.Clear();
  bsc_strName.Clear();
//  bsc_bspBSPTree.Destroy();
}

/*
 * Lock all arrays.
 */
void CBrushSector::LockAll(void)
{
  /* this function does nothing, because in current implementation all
     arrays in brush sector are static arrays
   */
}

/*
 * Unlock all arrays.
 */
void CBrushSector::UnlockAll(void)
{
  /* this function does nothing, because in current implementation all
     arrays in brush sector are static arrays
   */
}

/*
 * Calculate volume of the sector (all polygons must be triangularized).
 */
DOUBLE CBrushSector::CalculateVolume(void)
{
  // assure that floating point precision is 53 bits
  AssureFPT_53();

  DOUBLE fSectorVolume = 0.0;
  // for each polygon
  {FOREACHINSTATICARRAY(bsc_abpoPolygons, CBrushPolygon, itbpo) {
    // calculate the area of the polygon
    DOUBLE fPolygonArea = itbpo->CalculateArea();
    // add the volume of the pyramid that the polygon closes with origin
    fSectorVolume += fPolygonArea * itbpo->bpo_pbplPlane->bpl_pldPreciseRelative.Distance() / 3.0;
  }}

  // if the volume is positive
  if (fSectorVolume>=0.0) {
    // remember that the sector is open
    bsc_ulFlags |= BSCF_OPENSECTOR;
    // if the sector belongs to a field brush
    CBrush3D *pbr = bsc_pbmBrushMip->bm_pbrBrush;
    ASSERT(pbr!=NULL);
    if (pbr->br_pfsFieldSettings!=NULL) {
      // report a warning
      CPrintF("Warning: Open sector in a field brush!\n");
    }
  // if the volume is negative
  } else {
    // remember that the sector is closed
    bsc_ulFlags &= ~BSCF_OPENSECTOR;
  }
  Triangulate();
  return fSectorVolume;
}
void CBrushSector::Triangulate(void)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_TRIANGULATE);
  // for each polygon
  {FOREACHINSTATICARRAY(bsc_abpoPolygons, CBrushPolygon, itbpo) {
    itbpo->Triangulate();
  }}

  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_TRIANGULATE);
}
// update changed sector's data after dragging vertices or importing
void CBrushSector::UpdateSector(void)
{
  // calculate the bounding box of the brush sector
  CSimpleProjection3D_DOUBLE prBrushToAbsolute;
  bsc_pbmBrushMip->bm_pbrBrush->PrepareRelativeToAbsoluteProjection(prBrushToAbsolute);
  CalculateBoundingBoxes(prBrushToAbsolute);
  // calculate the volume of the sector
  CalculateVolume();

  // for each polygon
  INDEX ctPolygons = bsc_abpoPolygons.Count();
  {for(INDEX iPolygon=0; iPolygon<ctPolygons; iPolygon++) {
    CBrushPolygon  &bpo =    bsc_abpoPolygons[iPolygon];  // brush polygon alias
    // initialize the shadow map of the polygon
    bpo.InitializeShadowMap();
  }}

  // find and remember all entities in this sector
  FindEntitiesInSector();
}

// recalculate planes for polygons from their vertices
// NOTE: Planes are not optimal here. You have to reoptimize the brush 
// after done with dragging
//
void CBrushSector::MakePlanesFromVertices(void)
{
  // clear old planes
  bsc_abplPlanes.Clear();
  bsc_awplPlanes.Clear();
  // get the number of planes to create
  INDEX ctPlanes = bsc_abpoPolygons.Count();
  // create that much planes
  bsc_abplPlanes.New(ctPlanes);
  bsc_awplPlanes.New(ctPlanes);

  // for all polygons
  INDEX ctPolygons = bsc_abpoPolygons.Count();
  for(INDEX iPolygon=0; iPolygon<ctPolygons; iPolygon++) {
    CBrushPolygon &bpo = bsc_abpoPolygons[iPolygon];  // brush polygon alias
    CBrushPlane   &bpl = bsc_abplPlanes[iPolygon];
    CWorkingPlane &wpl = bsc_awplPlanes[iPolygon];
    // link to plane
    bpo.bpo_pbplPlane = &bpl;
    bpl.bpl_pwplWorking = &wpl;

    // clear plane normal
    DOUBLE3D vNormal = DOUBLE3D(0.0f,0.0f,0.0f);
    DOUBLE3D vAnyVertex;

    // for all edges in polygon
    INDEX ctVertices = bpo.bpo_abpePolygonEdges.Count();
    {for(INDEX iVertex=0; iVertex<ctVertices; iVertex++) {
		  // get its vertices in counterclockwise order
      CBrushPolygonEdge &bpe = bpo.bpo_abpePolygonEdges[iVertex];
      DOUBLE3D v0, v1;
      bpe.GetVertexCoordinatesPreciseRelative(v0, v1);
      DOUBLE3D vSum = v0+v1;
      DOUBLE3D vDif = v0-v1;
		  // add the edge contribution to the normal vector
      vNormal(1) += vDif(2)*vSum(3);
      vNormal(2) += vDif(3)*vSum(1);
      vNormal(3) += vDif(1)*vSum(2);
      vAnyVertex = v0;
    }}

    // if the polygon area is too small
    if (vNormal.Length()<1E-8) {
      // make dummy normal
      vNormal = DOUBLE3D(0,1,0);
    }
    // construct this plane from normal vector and one point
	  bpl.bpl_pldPreciseRelative = DOUBLEplane3D(vNormal, vAnyVertex);
    wpl.wpl_plRelative = DOUBLEtoFLOAT(bpl.bpl_pldPreciseRelative);
    // make default mapping coordinates for the plane
    wpl.wpl_mvRelative.FromPlane(wpl.wpl_plRelative);
  }
}

// Update sector after moving vertices
void CBrushSector::UpdateVertexChanges(void)
{
  // recalculate planes for polygons from their vertices
  MakePlanesFromVertices();

  // update changed sector's data after dragging vertices or importing
  bsc_ulTempFlags|=BSCTF_PRELOADEDBSP;
  UpdateSector();
}

void CBrushSector::TriangularizePolygon( CBrushPolygon *pbpo)
{
  // clear marked for use flag on all polygons in world
  CWorld *pwo=bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->en_pwoWorld;
  pwo->ClearMarkedForUseFlag();

  pbpo->bpo_ulFlags |= BPOF_MARKED_FOR_USE;
  TriangularizeMarkedPolygons();
  UpdateVertexChanges();
}

// Triangularize polygons that continin vertices from selection
void CBrushSector::TriangularizeForVertices( CBrushVertexSelection &selVertex)
{
  // clear marked for use flag on all polygons in world
  CWorld *pwo=bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->en_pwoWorld;
  pwo->ClearMarkedForUseFlag();

  // ---------- Mark polygons in this sector that contain any of the selected vertices
  // for all polygons in this sector
  {FOREACHINSTATICARRAY(bsc_abpoPolygons, CBrushPolygon, itbpo)
  {
    // if polygon is already marked for triangularization
    if( itbpo->bpo_ulFlags & BPOF_MARKED_FOR_USE)
    {
      // no need to test it again
      continue;
    }
    // if this polygon is triangle
    if( itbpo->bpo_aiTriangleElements.Count() == 3)
    {
      // skip it
      continue;
    }
    // for all vertices in this polygon
    {FOREACHINSTATICARRAY(itbpo->bpo_apbvxTriangleVertices, CBrushVertex *, itpbvx)
    {
      // if any of polygon's vertices is selected
      if( (*itpbvx)->bvx_ulFlags&BVXF_SELECTED)
      {
        // mark for triangularized
        itbpo->bpo_ulFlags |= BPOF_MARKED_FOR_USE;
        // no need to test other vertices in this polygon
        break;
      }
    }}
  }}
  
  // triangularize marked polygons
  TriangularizeMarkedPolygons();
}
  
void CBrushSector::TriangularizeMarkedPolygons( void)
{
  // for marked polygons: count how many there are and how many new triangles will be created
  INDEX ctPolygonsToRemove = 0;
  INDEX ctNewTriangles = 0;
  
  // for all polygons in this sector
  {FOREACHINSTATICARRAY(bsc_abpoPolygons, CBrushPolygon, itbpo)
  {
    // if this polygon is triangle
    if( itbpo->bpo_aiTriangleElements.Count() == 3)
    {
      // skip it
      continue;
    }
    // if polygon is already marked for triangularization
    if( itbpo->bpo_ulFlags & BPOF_MARKED_FOR_USE)
    {
      // count polygon
      ctPolygonsToRemove++;
      // and its triangles
      ctNewTriangles+=itbpo->bpo_aiTriangleElements.Count()/3;
    }
  }}

  // if all marked polygons are already triangularized
  if( ctPolygonsToRemove == 0)
  {
    // don't do anything
    return;
  }

  // create new edge and polygon arrays
  CStaticArray<CBrushEdge> abedEdgesNew;
  CStaticArray<CBrushPolygon> abpoPolygonsNew;

  INDEX ctOldEdges = bsc_abedEdges.Count();
  INDEX ctOldPolygons = bsc_abpoPolygons.Count();

  abedEdgesNew.New( ctOldEdges+ctNewTriangles*3);
  abpoPolygonsNew.New( ctOldPolygons-ctPolygonsToRemove+ctNewTriangles);

  // copy old edges to new edge array
  INDEX iEdge;
  for( iEdge=0; iEdge<ctOldEdges; iEdge++)
  {
    abedEdgesNew[iEdge] = bsc_abedEdges[iEdge];
  }
  // note that iEdge points to first new edge

  
  // ----------- Copy old polygons, create new ones along with their edges
  // for all polygons in this sector
  INDEX iNewPolygons=0;
  {FOREACHINSTATICARRAY(bsc_abpoPolygons, CBrushPolygon, itbpo)
  {
    CBrushPolygon &bpoOld = *itbpo;

    // polygon shouldn't be triangularized
    if( !(itbpo->bpo_ulFlags & BPOF_MARKED_FOR_USE))
    {
      CBrushPolygon &bpoNew = abpoPolygonsNew[ iNewPolygons];
      // copy the old polygon
      bpoNew.bpo_pbplPlane =      itbpo->bpo_pbplPlane;

      bpoNew.bpo_abptTextures[0].CopyTextureProperties( itbpo->bpo_abptTextures[0], TRUE);
      bpoNew.bpo_abptTextures[1].CopyTextureProperties( itbpo->bpo_abptTextures[1], TRUE);
      bpoNew.bpo_abptTextures[2].CopyTextureProperties( itbpo->bpo_abptTextures[2], TRUE);

      bpoNew.bpo_colColor         = itbpo->bpo_colColor;
      bpoNew.bpo_ulFlags          = itbpo->bpo_ulFlags & ~(BPOF_MARKED_FOR_USE|BPOF_SELECTED);
      bpoNew.bpo_colShadow        = itbpo->bpo_colShadow;
      bpoNew.bpo_bppProperties    = itbpo->bpo_bppProperties;
      bpoNew.bpo_pbscSector       = itbpo->bpo_pbscSector;

      // remap brush polygon edges to point to edges of new array
      INDEX ctEdgesToRemap = itbpo->bpo_abpePolygonEdges.Count();
      // allocate new polygon edges
      bpoNew.bpo_abpePolygonEdges.New(ctEdgesToRemap);
      // for each edge in polygon
      for( INDEX iRemapEdge=0; iRemapEdge<ctEdgesToRemap; iRemapEdge++)
      {
        CBrushPolygonEdge &bpeOld = itbpo->bpo_abpePolygonEdges[iRemapEdge];
        CBrushPolygonEdge &bpeNew = bpoNew.bpo_abpePolygonEdges[iRemapEdge];
        
        // get index of the edge for old edge array
        INDEX iOldIndex = bsc_abedEdges.Index( bpeOld.bpe_pbedEdge);

        // use same index, but point to edge in new edge array
        bpeNew.bpe_pbedEdge = &abedEdgesNew[iOldIndex];
        // set edge direction
        bpeNew.bpe_bReverse = bpeOld.bpe_bReverse;
      }

      bpoNew.bpo_apbvxTriangleVertices = bpoOld.bpo_apbvxTriangleVertices;
      bpoNew.bpo_aiTriangleElements    = bpoOld.bpo_aiTriangleElements;

      // initialize shadow map
      bpoNew.InitializeShadowMap();
      
      iNewPolygons++;
    }
    // if polygon is marked for triangularization
    else
    {
      INDEX ctTriangles = itbpo->bpo_aiTriangleElements.Count()/3;
      // for each triangle in old polygon
      for( INDEX iTriangle=0; iTriangle<ctTriangles; iTriangle++)
      {
        CBrushPolygon &bpoNew = abpoPolygonsNew[ iNewPolygons];
        INDEX iVtx0 = itbpo->bpo_aiTriangleElements[iTriangle*3+0];
        INDEX iVtx1 = itbpo->bpo_aiTriangleElements[iTriangle*3+1];
        INDEX iVtx2 = itbpo->bpo_aiTriangleElements[iTriangle*3+2];
        
        CBrushVertex *pbvtx0 = itbpo->bpo_apbvxTriangleVertices[ iVtx0];
        CBrushVertex *pbvtx1 = itbpo->bpo_apbvxTriangleVertices[ iVtx1];
        CBrushVertex *pbvtx2 = itbpo->bpo_apbvxTriangleVertices[ iVtx2];

        // setup edge 0
        abedEdgesNew[iEdge+0].bed_pbvxVertex0 = pbvtx0;
        abedEdgesNew[iEdge+0].bed_pbvxVertex1 = pbvtx1;

        // setup edge 1
        abedEdgesNew[iEdge+1].bed_pbvxVertex0 = pbvtx1;
        abedEdgesNew[iEdge+1].bed_pbvxVertex1 = pbvtx2;

        // setup edge 2
        abedEdgesNew[iEdge+2].bed_pbvxVertex0 = pbvtx2;
        abedEdgesNew[iEdge+2].bed_pbvxVertex1 = pbvtx0;
        
        // allocate and set polygon edges for new triangle
        bpoNew.bpo_abpePolygonEdges.New(3);
        bpoNew.bpo_abpePolygonEdges[0].bpe_pbedEdge = &abedEdgesNew[iEdge+0];
        bpoNew.bpo_abpePolygonEdges[0].bpe_bReverse = FALSE;
        bpoNew.bpo_abpePolygonEdges[1].bpe_pbedEdge = &abedEdgesNew[iEdge+1];
        bpoNew.bpo_abpePolygonEdges[1].bpe_bReverse = FALSE;
        bpoNew.bpo_abpePolygonEdges[2].bpe_pbedEdge = &abedEdgesNew[iEdge+2];
        bpoNew.bpo_abpePolygonEdges[2].bpe_bReverse = FALSE;

        //CBrushEdge &edg0 = *bpoNew.bpo_abpePolygonEdges[0].bpe_pbedEdge;
        //CBrushEdge &edg1 = *bpoNew.bpo_abpePolygonEdges[1].bpe_pbedEdge;
        //CBrushEdge &edg2 = *bpoNew.bpo_abpePolygonEdges[2].bpe_pbedEdge;

        // set brush vertex ptrs
        bpoNew.bpo_apbvxTriangleVertices.New(3);
        bpoNew.bpo_apbvxTriangleVertices[0] = pbvtx0;
        bpoNew.bpo_apbvxTriangleVertices[1] = pbvtx1;
        bpoNew.bpo_apbvxTriangleVertices[2] = pbvtx2;

        // setup fixed trinagle element indices
        bpoNew.bpo_aiTriangleElements.New(3);
        bpoNew.bpo_aiTriangleElements[0] = 0;
        bpoNew.bpo_aiTriangleElements[1] = 1;
        bpoNew.bpo_aiTriangleElements[2] = 2;

        // copy parameters from old polygon
        bpoNew.bpo_pbplPlane =      itbpo->bpo_pbplPlane;

        bpoNew.bpo_abptTextures[0].CopyTextureProperties( itbpo->bpo_abptTextures[0], TRUE);
        bpoNew.bpo_abptTextures[1].CopyTextureProperties( itbpo->bpo_abptTextures[1], TRUE);
        bpoNew.bpo_abptTextures[2].CopyTextureProperties( itbpo->bpo_abptTextures[2], TRUE);

        bpoNew.bpo_colColor         = itbpo->bpo_colColor;
        bpoNew.bpo_ulFlags          = itbpo->bpo_ulFlags & ~(BPOF_MARKED_FOR_USE|BPOF_SELECTED);
        bpoNew.bpo_colShadow        = itbpo->bpo_colShadow;
        bpoNew.bpo_bppProperties    = itbpo->bpo_bppProperties;
        bpoNew.bpo_pbscSector       = itbpo->bpo_pbscSector;

        // initialize shadow map
        bpoNew.InitializeShadowMap();

        // skip created edges
        iEdge+=3;
        // next triangle
        iNewPolygons++;
      }
    }
  }}

  // copy new arrays over old ones
  bsc_abedEdges.MoveArray( abedEdgesNew);
  bsc_abpoPolygons.MoveArray( abpoPolygonsNew);

  // create array of working edges
  INDEX cted = bsc_abedEdges.Count();
  bsc_awedEdges.Clear();
  bsc_awedEdges.New(cted);
  // for each edge
  for (INDEX ied=0; ied<cted; ied++) {
    CWorkingEdge &wed = bsc_awedEdges[ied];
    CBrushEdge   &bed = bsc_abedEdges[ied];
    // setup its working edge
    bed.bed_pwedWorking = &wed;
    wed.wed_iwvx0 = bsc_abvxVertices.Index(bed.bed_pbvxVertex0);
    wed.wed_iwvx1 = bsc_abvxVertices.Index(bed.bed_pbvxVertex1);
  }

  // recalculate planes for polygons from their vertices
  MakePlanesFromVertices();
}

void CBrushSector::InsertVertexIntoTriangle( CBrushPolygonSelection &selPolygon, FLOAT3D vVertex)
{
  if( selPolygon.Count() != 1)
  {
    return;
  }

  SubdivideTriangles(selPolygon);
 
  // get last vertex
  INDEX ctVertices = bsc_abvxVertices.Count();  
  CBrushVertex *pbvtxLast = &bsc_abvxVertices[ctVertices-1];
  pbvtxLast->SetAbsolutePosition( FLOATtoDOUBLE(vVertex));
}

void CBrushSector::SubdivideTriangles( CBrushPolygonSelection &selPolygon)
{
  INDEX ctPolygonsToRemove = selPolygon.Count();
  INDEX ctNewTriangles = ctPolygonsToRemove*3;
  
  // clear marked for use flag on all polygons in world
  CWorld *pwo=bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->en_pwoWorld;
  pwo->ClearMarkedForUseFlag();

  // for all polygons in selection
  {FOREACHINDYNAMICCONTAINER(selPolygon, CBrushPolygon, itbpo)
  {
    // mark them for use
    itbpo->bpo_ulFlags |= BPOF_MARKED_FOR_USE;
    if( itbpo->bpo_aiTriangleElements.Count() != 3)
    {
      return;
    }
  }}

  // clear the selection
  selPolygon.Clear();

  // create new arrays
  CStaticArray<CWorkingVertex> awvxVerticesNew;
  CStaticArray<CBrushVertex> abvxVerticesNew;
  CStaticArray<CBrushEdge> abedEdgesNew;
  CStaticArray<CBrushPolygon> abpoPolygonsNew;

  INDEX ctOldVertices = bsc_abvxVertices.Count();
  INDEX ctOldEdges = bsc_abedEdges.Count();
  INDEX ctOldPolygons = bsc_abpoPolygons.Count();

  // allocate arrays
  abvxVerticesNew.New( ctOldVertices+ctPolygonsToRemove);
  awvxVerticesNew.New( ctOldVertices+ctPolygonsToRemove);
  abedEdgesNew.New( ctOldEdges+ctPolygonsToRemove*6);
  abpoPolygonsNew.New( ctOldPolygons-ctPolygonsToRemove+ctNewTriangles);

  // copy old vertices to new vertex array
  INDEX iVtx;
  for( iVtx=0; iVtx<ctOldVertices; iVtx++)
  {
    abvxVerticesNew[iVtx] = bsc_abvxVertices[iVtx];
    awvxVerticesNew[iVtx] = bsc_awvxVertices[iVtx];
  }
  // note that iVtx points to first new vertex

  // initialize working vertex ptrs
  for( INDEX iVtxTemp=0; iVtxTemp<abvxVerticesNew.Count(); iVtxTemp++)
  {
    abvxVerticesNew[iVtxTemp].bvx_pwvxWorking = &awvxVerticesNew[iVtxTemp];
  }
  
  // copy old edges to new edge array
  INDEX iEdge;
  for( iEdge=0; iEdge<ctOldEdges; iEdge++)
  {
    // remap old vertices into new vertex array using their indices
    INDEX iOldVtx0 = bsc_abvxVertices.Index( bsc_abedEdges[iEdge].bed_pbvxVertex0);
    INDEX iOldVtx1 = bsc_abvxVertices.Index( bsc_abedEdges[iEdge].bed_pbvxVertex1);
    abedEdgesNew[iEdge].bed_pbvxVertex0 = &abvxVerticesNew[iOldVtx0];
    abedEdgesNew[iEdge].bed_pbvxVertex1 = &abvxVerticesNew[iOldVtx1];
  }
  // note that iEdge points to first new edge

  
  // ----------- Copy old polygons, create new ones along with their edges and vertices
  // for all polygons in this sector
  INDEX iNewPolygons=0;
  {FOREACHINSTATICARRAY(bsc_abpoPolygons, CBrushPolygon, itbpo)
  {
    CBrushPolygon &bpoOld = *itbpo;

    // polygon shouldn't be subdivided
    if( !(itbpo->bpo_ulFlags & BPOF_MARKED_FOR_USE))
    {
      // copy the old polygon
      CBrushPolygon &bpoNew = abpoPolygonsNew[ iNewPolygons];
      bpoNew.bpo_pbplPlane =      bpoOld.bpo_pbplPlane;

      bpoNew.bpo_abptTextures[0].CopyTextureProperties( bpoOld.bpo_abptTextures[0], TRUE);
      bpoNew.bpo_abptTextures[1].CopyTextureProperties( bpoOld.bpo_abptTextures[1], TRUE);
      bpoNew.bpo_abptTextures[2].CopyTextureProperties( bpoOld.bpo_abptTextures[2], TRUE);

      bpoNew.bpo_colColor         = bpoOld.bpo_colColor;
      bpoNew.bpo_ulFlags          = bpoOld.bpo_ulFlags & ~(BPOF_MARKED_FOR_USE|BPOF_SELECTED);
      bpoNew.bpo_colShadow        = bpoOld.bpo_colShadow;
      bpoNew.bpo_bppProperties    = bpoOld.bpo_bppProperties;
      bpoNew.bpo_pbscSector       = bpoOld.bpo_pbscSector;

      // remap brush polygon edges to point to edges of new array
      INDEX ctEdgesToRemap = bpoOld.bpo_abpePolygonEdges.Count();
      // allocate new polygon edges
      bpoNew.bpo_abpePolygonEdges.New(ctEdgesToRemap);
      // for each edge in polygon
      for( INDEX iRemapEdge=0; iRemapEdge<ctEdgesToRemap; iRemapEdge++)
      {
        CBrushPolygonEdge &bpeOld = bpoOld.bpo_abpePolygonEdges[iRemapEdge];
        CBrushPolygonEdge &bpeNew = bpoNew.bpo_abpePolygonEdges[iRemapEdge];
        
        // get index of the edge for old edge array
        INDEX iOldIndex = bsc_abedEdges.Index( bpeOld.bpe_pbedEdge);

        // use same index, but point to edge in new edge array
        bpeNew.bpe_pbedEdge = &abedEdgesNew[iOldIndex];
        // set edge direction
        bpeNew.bpe_bReverse = bpeOld.bpe_bReverse;
      }

      // allocate and set vertex pointers
      bpoNew.bpo_apbvxTriangleVertices.New(3);
      // remap brush vertex pointers
      for( INDEX iTVtx=0; iTVtx<3; iTVtx++)
      {
        INDEX iOldVtx = bsc_abvxVertices.Index( bpoOld.bpo_apbvxTriangleVertices[iTVtx]);
        bpoNew.bpo_apbvxTriangleVertices[iTVtx] = &abvxVerticesNew[iOldVtx];
      }

      // copy triangle and triangle elements arrays
      bpoNew.bpo_aiTriangleElements    = bpoOld.bpo_aiTriangleElements;

      // initialize shadow map
      bpoNew.InitializeShadowMap();
      
      iNewPolygons++;
    }
    // if polygon is marked for subdivision
    else
    {
      INDEX iVtx0 = bpoOld.bpo_aiTriangleElements[0];
      INDEX iVtx1 = bpoOld.bpo_aiTriangleElements[1];
      INDEX iVtx2 = bpoOld.bpo_aiTriangleElements[2];

      INDEX iOldVtx0 = bsc_abvxVertices.Index( bpoOld.bpo_apbvxTriangleVertices[ iVtx0]);
      INDEX iOldVtx1 = bsc_abvxVertices.Index( bpoOld.bpo_apbvxTriangleVertices[ iVtx1]);
      INDEX iOldVtx2 = bsc_abvxVertices.Index( bpoOld.bpo_apbvxTriangleVertices[ iVtx2]);
      CBrushVertex *pbvtx0 = &abvxVerticesNew[iOldVtx0];
      CBrushVertex *pbvtx1 = &abvxVerticesNew[iOldVtx1];
      CBrushVertex *pbvtx2 = &abvxVerticesNew[iOldVtx2];
      CBrushVertex *pbvtx3 = &abvxVerticesNew[ iVtx];
      pbvtx3->bvx_pbscSector = this;

      // calculate and set middle point of the triangle
      DOUBLE3D vCenter = FLOATtoDOUBLE(
        pbvtx0->bvx_vAbsolute +
        pbvtx1->bvx_vAbsolute +
        pbvtx2->bvx_vAbsolute)/3.0;
      pbvtx3->SetAbsolutePosition( vCenter);
      
      // add new edges
      // setup edge 0 (copy of old edge)
      abedEdgesNew[iEdge+0].bed_pbvxVertex0 = pbvtx0;
      abedEdgesNew[iEdge+0].bed_pbvxVertex1 = pbvtx1;
      // setup edge 1 (copy of old edge)
      abedEdgesNew[iEdge+1].bed_pbvxVertex0 = pbvtx1;
      abedEdgesNew[iEdge+1].bed_pbvxVertex1 = pbvtx2;
      // setup edge 2 (copy of old edge)
      abedEdgesNew[iEdge+2].bed_pbvxVertex0 = pbvtx2;
      abedEdgesNew[iEdge+2].bed_pbvxVertex1 = pbvtx0;
      // setup edge 3 
      abedEdgesNew[iEdge+3].bed_pbvxVertex0 = pbvtx0;
      abedEdgesNew[iEdge+3].bed_pbvxVertex1 = pbvtx3;
      // setup edge 4
      abedEdgesNew[iEdge+4].bed_pbvxVertex0 = pbvtx1;
      abedEdgesNew[iEdge+4].bed_pbvxVertex1 = pbvtx3;
      // setup edge 5
      abedEdgesNew[iEdge+5].bed_pbvxVertex0 = pbvtx2;
      abedEdgesNew[iEdge+5].bed_pbvxVertex1 = pbvtx3;

      // ---------------- Create first sub-triangle
      CBrushPolygon &bpoNew1 = abpoPolygonsNew[ iNewPolygons+0];
      // allocate and set polygon edges
      bpoNew1.bpo_abpePolygonEdges.New(3);
      bpoNew1.bpo_abpePolygonEdges[0].bpe_pbedEdge = &abedEdgesNew[iEdge+0];
      bpoNew1.bpo_abpePolygonEdges[0].bpe_bReverse = FALSE;
      bpoNew1.bpo_abpePolygonEdges[1].bpe_pbedEdge = &abedEdgesNew[iEdge+4];
      bpoNew1.bpo_abpePolygonEdges[1].bpe_bReverse = FALSE;
      bpoNew1.bpo_abpePolygonEdges[2].bpe_pbedEdge = &abedEdgesNew[iEdge+3];
      bpoNew1.bpo_abpePolygonEdges[2].bpe_bReverse = TRUE;

      // set brush vertex ptrs
      bpoNew1.bpo_apbvxTriangleVertices.New(3);
      bpoNew1.bpo_apbvxTriangleVertices[0] = pbvtx0;
      bpoNew1.bpo_apbvxTriangleVertices[1] = pbvtx1;
      bpoNew1.bpo_apbvxTriangleVertices[2] = pbvtx3;

      // setup fixed trinagle element indices
      bpoNew1.bpo_aiTriangleElements.New(3);
      bpoNew1.bpo_aiTriangleElements[0] = 0;
      bpoNew1.bpo_aiTriangleElements[1] = 1;
      bpoNew1.bpo_aiTriangleElements[2] = 2;

      // copy parameters from old polygon
      bpoNew1.bpo_pbplPlane = bpoOld.bpo_pbplPlane;
      bpoNew1.bpo_abptTextures[0].CopyTextureProperties( bpoOld.bpo_abptTextures[0], TRUE);
      bpoNew1.bpo_abptTextures[1].CopyTextureProperties( bpoOld.bpo_abptTextures[1], TRUE);
      bpoNew1.bpo_abptTextures[2].CopyTextureProperties( bpoOld.bpo_abptTextures[2], TRUE);
      bpoNew1.bpo_colColor         = bpoOld.bpo_colColor;
      bpoNew1.bpo_ulFlags          = bpoOld.bpo_ulFlags & ~(BPOF_MARKED_FOR_USE|BPOF_SELECTED);
      bpoNew1.bpo_colShadow        = bpoOld.bpo_colShadow;
      bpoNew1.bpo_bppProperties    = bpoOld.bpo_bppProperties;
      bpoNew1.bpo_pbscSector       = bpoOld.bpo_pbscSector;

      // initialize shadow map
      bpoNew1.InitializeShadowMap();

      // ---------------- Create second sub-triangle
      CBrushPolygon &bpoNew2 = abpoPolygonsNew[ iNewPolygons+1];
      // allocate and set polygon edges
      bpoNew2.bpo_abpePolygonEdges.New(3);
      bpoNew2.bpo_abpePolygonEdges[0].bpe_pbedEdge = &abedEdgesNew[iEdge+1];
      bpoNew2.bpo_abpePolygonEdges[0].bpe_bReverse = FALSE;
      bpoNew2.bpo_abpePolygonEdges[1].bpe_pbedEdge = &abedEdgesNew[iEdge+5];
      bpoNew2.bpo_abpePolygonEdges[1].bpe_bReverse = FALSE;
      bpoNew2.bpo_abpePolygonEdges[2].bpe_pbedEdge = &abedEdgesNew[iEdge+4];
      bpoNew2.bpo_abpePolygonEdges[2].bpe_bReverse = TRUE;

      // set brush vertex ptrs
      bpoNew2.bpo_apbvxTriangleVertices.New(3);
      bpoNew2.bpo_apbvxTriangleVertices[0] = pbvtx1;
      bpoNew2.bpo_apbvxTriangleVertices[1] = pbvtx2;
      bpoNew2.bpo_apbvxTriangleVertices[2] = pbvtx3;

      // setup fixed trinagle element indices
      bpoNew2.bpo_aiTriangleElements.New(3);
      bpoNew2.bpo_aiTriangleElements[0] = 0;
      bpoNew2.bpo_aiTriangleElements[1] = 1;
      bpoNew2.bpo_aiTriangleElements[2] = 2;

      // copy parameters from old polygon
      bpoNew2.bpo_pbplPlane = bpoOld.bpo_pbplPlane;
      bpoNew2.bpo_abptTextures[0].CopyTextureProperties( bpoOld.bpo_abptTextures[0], TRUE);
      bpoNew2.bpo_abptTextures[1].CopyTextureProperties( bpoOld.bpo_abptTextures[1], TRUE);
      bpoNew2.bpo_abptTextures[2].CopyTextureProperties( bpoOld.bpo_abptTextures[2], TRUE);
      bpoNew2.bpo_colColor         = bpoOld.bpo_colColor;
      bpoNew2.bpo_ulFlags          = bpoOld.bpo_ulFlags & ~(BPOF_MARKED_FOR_USE|BPOF_SELECTED);
      bpoNew2.bpo_colShadow        = bpoOld.bpo_colShadow;
      bpoNew2.bpo_bppProperties    = bpoOld.bpo_bppProperties;
      bpoNew2.bpo_pbscSector       = bpoOld.bpo_pbscSector;

      // initialize shadow map
      bpoNew2.InitializeShadowMap();

      // ---------------- Create third sub-triangle
      CBrushPolygon &bpoNew3 = abpoPolygonsNew[ iNewPolygons+2];
      // allocate and set polygon edges
      bpoNew3.bpo_abpePolygonEdges.New(3);
      bpoNew3.bpo_abpePolygonEdges[0].bpe_pbedEdge = &abedEdgesNew[iEdge+2];
      bpoNew3.bpo_abpePolygonEdges[0].bpe_bReverse = FALSE;
      bpoNew3.bpo_abpePolygonEdges[1].bpe_pbedEdge = &abedEdgesNew[iEdge+3];
      bpoNew3.bpo_abpePolygonEdges[1].bpe_bReverse = FALSE;
      bpoNew3.bpo_abpePolygonEdges[2].bpe_pbedEdge = &abedEdgesNew[iEdge+5];
      bpoNew3.bpo_abpePolygonEdges[2].bpe_bReverse = TRUE;

      // set brush vertex ptrs
      bpoNew3.bpo_apbvxTriangleVertices.New(3);
      bpoNew3.bpo_apbvxTriangleVertices[0] = pbvtx2;
      bpoNew3.bpo_apbvxTriangleVertices[1] = pbvtx0;
      bpoNew3.bpo_apbvxTriangleVertices[2] = pbvtx3;

      // setup fixed trinagle element indices
      bpoNew3.bpo_aiTriangleElements.New(3);
      bpoNew3.bpo_aiTriangleElements[0] = 0;
      bpoNew3.bpo_aiTriangleElements[1] = 1;
      bpoNew3.bpo_aiTriangleElements[2] = 2;

      // copy parameters from old polygon
      bpoNew3.bpo_pbplPlane = bpoOld.bpo_pbplPlane;
      bpoNew3.bpo_abptTextures[0].CopyTextureProperties( bpoOld.bpo_abptTextures[0], TRUE);
      bpoNew3.bpo_abptTextures[1].CopyTextureProperties( bpoOld.bpo_abptTextures[1], TRUE);
      bpoNew3.bpo_abptTextures[2].CopyTextureProperties( bpoOld.bpo_abptTextures[2], TRUE);
      bpoNew3.bpo_colColor         = bpoOld.bpo_colColor;
      bpoNew3.bpo_ulFlags          = bpoOld.bpo_ulFlags & ~(BPOF_MARKED_FOR_USE|BPOF_SELECTED);
      bpoNew3.bpo_colShadow        = bpoOld.bpo_colShadow;
      bpoNew3.bpo_bppProperties    = bpoOld.bpo_bppProperties;
      bpoNew3.bpo_pbscSector       = bpoOld.bpo_pbscSector;

      // initialize shadow map
      bpoNew3.InitializeShadowMap();

      // skip newly created vertex
      iVtx++;
      // skip created edges
      iEdge+=6;
      // next triangle
      iNewPolygons += 3;
    }
  }}

  // copy new arrays over old ones
  bsc_awvxVertices.MoveArray( awvxVerticesNew);
  bsc_abvxVertices.MoveArray( abvxVerticesNew);
  bsc_abedEdges.MoveArray( abedEdgesNew);
  bsc_abpoPolygons.MoveArray( abpoPolygonsNew);

  // create array of working edges
  INDEX cted = bsc_abedEdges.Count();
  bsc_awedEdges.Clear();
  bsc_awedEdges.New(cted);
  // for each edge
  for (INDEX ied=0; ied<cted; ied++)
  {
    CWorkingEdge &wed = bsc_awedEdges[ied];
    CBrushEdge   &bed = bsc_abedEdges[ied];
    // setup its working edge
    bed.bed_pwedWorking = &wed;
    wed.wed_iwvx0 = bsc_abvxVertices.Index(bed.bed_pbvxVertex0);
    wed.wed_iwvx1 = bsc_abvxVertices.Index(bed.bed_pbvxVertex1);
  }

  // recalculate planes for polygons from their vertices
  MakePlanesFromVertices();
  UpdateSector();
}


// Search for shared edge in two given triangles
void GetSharedEdge( CBrushPolygon *pbpo0, CBrushPolygon *pbpo1, CBrushEdge *&pse)
{
  for( INDEX iFirst=0; iFirst<3; iFirst++)
  {
    for( INDEX iSecond=0; iSecond<3; iSecond++)
    {
      if( pbpo0->bpo_abpePolygonEdges[iFirst].bpe_pbedEdge == 
          pbpo1->bpo_abpePolygonEdges[iSecond].bpe_pbedEdge )
      {
        pse = pbpo0->bpo_abpePolygonEdges[iFirst].bpe_pbedEdge;
        return;
      }
    }
  }
  pse = NULL;
}

BOOL CBrushSector::IsReTripleAvailable( CBrushPolygonSelection &selPolygon)
{
  // we must have two polygons in selection
  if( selPolygon.Count() != 2) return FALSE;
  
  // obtain polygons
  CBrushPolygon *pbpoOld0 = selPolygon.sa_Array[0];
  CBrushPolygon *pbpoOld1 = selPolygon.sa_Array[1];
  
  // must be in the same sector
  if( pbpoOld0->bpo_pbscSector != pbpoOld1->bpo_pbscSector) return FALSE;

  // both of them must be triangles
  if( pbpoOld0->bpo_aiTriangleElements.Count() != 3) return FALSE;
  if( pbpoOld1->bpo_aiTriangleElements.Count() != 3) return FALSE;

  // triangles must share an edge
  CBrushEdge *pse = NULL;
  GetSharedEdge( pbpoOld0, pbpoOld1, pse);
  return(pse != NULL);
}

void GetNonSharedEdgesContainingVtx(CBrushPolygon *pbpo0, CBrushPolygon *pbpo1,
                                    CBrushEdge *pse, CBrushVertex *pvtx, 
                                    CBrushPolygonEdge *&pbpe0, CBrushPolygonEdge *&pbpe1)
{
  // set invalid ptrs
  pbpe0 = NULL;
  pbpe1 = NULL;

  // for first triangle
  for( INDEX itri0=0; itri0<3; itri0++)
  {
    CBrushPolygonEdge *pbpeTest = &pbpo0->bpo_abpePolygonEdges[itri0];
    if( ( pbpeTest->bpe_pbedEdge != pse) && // if it is not shared edge
        ((pbpeTest->bpe_pbedEdge->bed_pbvxVertex0 == pvtx) || // and contains given vertex
         (pbpeTest->bpe_pbedEdge->bed_pbvxVertex1 == pvtx)) )
    {
      pbpe0 = pbpeTest;
    }
  }
  // for second triangle
  for( INDEX itri1=0; itri1<3; itri1++)
  {
    CBrushPolygonEdge *pbpeTest = &pbpo1->bpo_abpePolygonEdges[itri1];
    if( ( pbpeTest->bpe_pbedEdge != pse) && // if it is not shared edge
        ((pbpeTest->bpe_pbedEdge->bed_pbvxVertex0 == pvtx) || // and contains given vertex
         (pbpeTest->bpe_pbedEdge->bed_pbvxVertex1 == pvtx)) )
    {
      pbpe1 = pbpeTest;
    }
  }

  // both must be found
  ASSERT( pbpe0 != NULL);
  ASSERT( pbpe1 != NULL);
  return;
}

void CBrushSector::ReTriple( CBrushPolygonSelection &selPolygon)
{
  // we must have two polygons in selection
  if( selPolygon.Count() != 2) return;
  
  // obtain polygons
  CBrushPolygon *pbpoOld0 = selPolygon.sa_Array[0];
  CBrushPolygon *pbpoOld1 = selPolygon.sa_Array[1];

  // both of them must be triangles
  if( pbpoOld0->bpo_aiTriangleElements.Count() != 3) return;
  if( pbpoOld1->bpo_aiTriangleElements.Count() != 3) return;

  // clear marked for use flag on all polygons in world
  CWorld *pwo=bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->en_pwoWorld;
  pwo->ClearMarkedForUseFlag();

  // mark them for use
  pbpoOld0->bpo_ulFlags |= BPOF_MARKED_FOR_USE;
  pbpoOld1->bpo_ulFlags |= BPOF_MARKED_FOR_USE;

  // clear the selection (polygons will be erased)
  selPolygon.Clear();

  // create new arrays
  CStaticArray<CBrushEdge> abedEdgesNew;
  CStaticArray<CBrushPolygon> abpoPolygonsNew;

  INDEX ctOldEdges = bsc_abedEdges.Count();
  INDEX ctOldPolygons = bsc_abpoPolygons.Count();

  // allocate arrays
  abedEdgesNew.New( ctOldEdges+1);
  abpoPolygonsNew.New( ctOldPolygons);

  // copy old edges to new edge array
  INDEX iEdge;
  for( iEdge=0; iEdge<ctOldEdges; iEdge++)
  {
    // copy vertex ptrs
    abedEdgesNew[iEdge].bed_pbvxVertex0 = bsc_abedEdges[iEdge].bed_pbvxVertex0;
    abedEdgesNew[iEdge].bed_pbvxVertex1 = bsc_abedEdges[iEdge].bed_pbvxVertex1;
  }
  // note that iEdge points to first new edge

  // ----------- Copy old polygons (except two new ones)
  // for all polygons in this sector
  INDEX iNewPolygons=0;
  {FOREACHINSTATICARRAY(bsc_abpoPolygons, CBrushPolygon, itbpo)
  {
    CBrushPolygon &bpoOld = *itbpo;

    // polygon shouldn't be subdivided
    if( !(itbpo->bpo_ulFlags & BPOF_MARKED_FOR_USE))
    {
      // copy the old polygon
      CBrushPolygon &bpoNew = abpoPolygonsNew[ iNewPolygons];
      bpoNew.bpo_pbplPlane = bpoOld.bpo_pbplPlane;

      bpoNew.bpo_abptTextures[0].CopyTextureProperties( bpoOld.bpo_abptTextures[0], TRUE);
      bpoNew.bpo_abptTextures[1].CopyTextureProperties( bpoOld.bpo_abptTextures[1], TRUE);
      bpoNew.bpo_abptTextures[2].CopyTextureProperties( bpoOld.bpo_abptTextures[2], TRUE);

      bpoNew.bpo_colColor         = bpoOld.bpo_colColor;
      bpoNew.bpo_ulFlags          = bpoOld.bpo_ulFlags & ~(BPOF_MARKED_FOR_USE|BPOF_SELECTED);
      bpoNew.bpo_colShadow        = bpoOld.bpo_colShadow;
      bpoNew.bpo_bppProperties    = bpoOld.bpo_bppProperties;
      bpoNew.bpo_pbscSector       = bpoOld.bpo_pbscSector;

      // remap brush polygon edges to point to edges of new array
      INDEX ctEdgesToRemap = bpoOld.bpo_abpePolygonEdges.Count();
      // allocate new polygon edges
      bpoNew.bpo_abpePolygonEdges.New(ctEdgesToRemap);
      // for each edge in polygon
      for( INDEX iRemapEdge=0; iRemapEdge<ctEdgesToRemap; iRemapEdge++)
      {
        CBrushPolygonEdge &bpeOld = bpoOld.bpo_abpePolygonEdges[iRemapEdge];
        CBrushPolygonEdge &bpeNew = bpoNew.bpo_abpePolygonEdges[iRemapEdge];
        
        // get index of the edge for old edge array
        INDEX iOldIndex = bsc_abedEdges.Index( bpeOld.bpe_pbedEdge);

        // use same index, but point to edge in new edge array
        bpeNew.bpe_pbedEdge = &abedEdgesNew[iOldIndex];
        // set edge direction
        bpeNew.bpe_bReverse = bpeOld.bpe_bReverse;
      }

      // allocate and set vertex pointers
      INDEX ctOldTVtx = bpoOld.bpo_apbvxTriangleVertices.Count();
      bpoNew.bpo_apbvxTriangleVertices.New(ctOldTVtx);
      // copy old brush vertex pointers
      for( INDEX iTVtx=0; iTVtx<ctOldTVtx; iTVtx++)
      {
        bpoNew.bpo_apbvxTriangleVertices[iTVtx] = bpoOld.bpo_apbvxTriangleVertices[iTVtx];
      }

      // copy triangle and triangle elements arrays
      bpoNew.bpo_aiTriangleElements = bpoOld.bpo_aiTriangleElements;

      // initialize shadow map
      bpoNew.InitializeShadowMap();
      
      iNewPolygons++;
    }
  }}

  // get shared edge
  CBrushEdge *pse = NULL;
  GetSharedEdge( pbpoOld0, pbpoOld1, pse);
  ASSERT( pse != NULL);
  // obtain vertices 0 and 1 of shared edge
  CBrushVertex *pv0se = pse->bed_pbvxVertex0;
  CBrushVertex *pv1se = pse->bed_pbvxVertex1;

  // they will form first two edges of retripled polygon (each edge will be from different polygon)
  CBrushPolygonEdge *pbpev0e0;
  CBrushPolygonEdge *pbpev0e1;  
  // from two given polygons, extract edges different from shared edge that contain given vertex of shared edge
  GetNonSharedEdgesContainingVtx( pbpoOld0, pbpoOld1, pse, pv0se, pbpev0e0, pbpev0e1);

#define REMAP_EDGE( pedg) \
  {INDEX iIndex = bsc_abedEdges.Index( pedg->bpe_pbedEdge);\
  pedg->bpe_pbedEdge = &abedEdgesNew[iIndex];}

  REMAP_EDGE( pbpev0e0);
  REMAP_EDGE( pbpev0e1);

  // get two other edges, for second retripled triangle
  CBrushPolygonEdge *pbpev1e0;
  CBrushPolygonEdge *pbpev1e1;  
  GetNonSharedEdgesContainingVtx( pbpoOld0, pbpoOld1, pse, pv1se, pbpev1e0, pbpev1e1);

  REMAP_EDGE( pbpev1e0);
  REMAP_EDGE( pbpev1e1);

  // find edges that exit and enter shared edge's vertex 0
  CBrushPolygonEdge *pbpeExit;
  CBrushPolygonEdge *pbpeEnter;
  if( ((pbpev0e0->bpe_pbedEdge->bed_pbvxVertex0 == pv0se) && !pbpev0e0->bpe_bReverse) ||
      ((pbpev0e0->bpe_pbedEdge->bed_pbvxVertex1 == pv0se) && pbpev0e0->bpe_bReverse) )
  {
    pbpeExit = pbpev0e0;
    pbpeEnter = pbpev0e1;
  }
  else
  {
    pbpeExit = pbpev0e1;
    pbpeEnter = pbpev0e0;
  }

  // find start vertex of new edge
  CBrushVertex *pvNew0;
  if( pbpeExit->bpe_pbedEdge->bed_pbvxVertex0 != pv0se)
  {
    pvNew0 = pbpeExit->bpe_pbedEdge->bed_pbvxVertex0;
  }
  else
  {
    pvNew0 = pbpeExit->bpe_pbedEdge->bed_pbvxVertex1;
  }
  // find end vertex of new edge
  CBrushVertex *pvNew1;
  if( pbpeEnter->bpe_pbedEdge->bed_pbvxVertex0 != pv0se)
  {
    pvNew1 = pbpeEnter->bpe_pbedEdge->bed_pbvxVertex0;
  }
  else
  {
    pvNew1 = pbpeEnter->bpe_pbedEdge->bed_pbvxVertex1;
  }

  // add new edge
  abedEdgesNew[iEdge].bed_pbvxVertex0 = pvNew0;
  abedEdgesNew[iEdge].bed_pbvxVertex1 = pvNew1;  
  
  // --------------- Create retripled triangle 1
  {
    // copy the old polygon
    CBrushPolygon &bpoNew = abpoPolygonsNew[ iNewPolygons+0];
    CBrushPolygon &bpoOld = *pbpoOld0;
    bpoNew.bpo_pbplPlane = bpoOld.bpo_pbplPlane;

    bpoNew.bpo_abptTextures[0].CopyTextureProperties( bpoOld.bpo_abptTextures[0], TRUE);
    bpoNew.bpo_abptTextures[1].CopyTextureProperties( bpoOld.bpo_abptTextures[1], TRUE);
    bpoNew.bpo_abptTextures[2].CopyTextureProperties( bpoOld.bpo_abptTextures[2], TRUE);

    bpoNew.bpo_colColor         = bpoOld.bpo_colColor;
    bpoNew.bpo_ulFlags          = bpoOld.bpo_ulFlags & ~(BPOF_MARKED_FOR_USE|BPOF_SELECTED);
    bpoNew.bpo_colShadow        = bpoOld.bpo_colShadow;
    bpoNew.bpo_bppProperties    = bpoOld.bpo_bppProperties;
    bpoNew.bpo_pbscSector       = bpoOld.bpo_pbscSector;

    // allocate polygon edges
    bpoNew.bpo_abpePolygonEdges.New(3);
    // set edges data
    bpoNew.bpo_abpePolygonEdges[0].bpe_pbedEdge = pbpev0e0->bpe_pbedEdge;
    bpoNew.bpo_abpePolygonEdges[0].bpe_bReverse = pbpev0e0->bpe_bReverse;
    bpoNew.bpo_abpePolygonEdges[1].bpe_pbedEdge = pbpev0e1->bpe_pbedEdge;
    bpoNew.bpo_abpePolygonEdges[1].bpe_bReverse = pbpev0e1->bpe_bReverse;
    bpoNew.bpo_abpePolygonEdges[2].bpe_pbedEdge = &abedEdgesNew[iEdge];
    bpoNew.bpo_abpePolygonEdges[2].bpe_bReverse = FALSE;

    // allocate and set vertex pointers
    bpoNew.bpo_apbvxTriangleVertices.New(3);
    bpoNew.bpo_apbvxTriangleVertices[0] = pv0se;
    bpoNew.bpo_apbvxTriangleVertices[1] = pvNew0;
    bpoNew.bpo_apbvxTriangleVertices[2] = pvNew1;

    // copy triangle and triangle elements arrays
    bpoNew.bpo_aiTriangleElements.New(3);
    bpoNew.bpo_aiTriangleElements[0] = 0;
    bpoNew.bpo_aiTriangleElements[1] = 1;
    bpoNew.bpo_aiTriangleElements[2] = 2;

    // initialize shadow map
    bpoNew.InitializeShadowMap();
  }

  // --------------- Create retripled triangle 2
  {
    // copy the old polygon
    CBrushPolygon &bpoNew = abpoPolygonsNew[ iNewPolygons+1];
    CBrushPolygon &bpoOld = *pbpoOld1;
    bpoNew.bpo_pbplPlane = bpoOld.bpo_pbplPlane;

    bpoNew.bpo_abptTextures[0].CopyTextureProperties( bpoOld.bpo_abptTextures[0], TRUE);
    bpoNew.bpo_abptTextures[1].CopyTextureProperties( bpoOld.bpo_abptTextures[1], TRUE);
    bpoNew.bpo_abptTextures[2].CopyTextureProperties( bpoOld.bpo_abptTextures[2], TRUE);

    bpoNew.bpo_colColor         = bpoOld.bpo_colColor;
    bpoNew.bpo_ulFlags          = bpoOld.bpo_ulFlags & ~(BPOF_MARKED_FOR_USE|BPOF_SELECTED);
    bpoNew.bpo_colShadow        = bpoOld.bpo_colShadow;
    bpoNew.bpo_bppProperties    = bpoOld.bpo_bppProperties;
    bpoNew.bpo_pbscSector       = bpoOld.bpo_pbscSector;

    // allocate polygon edges
    bpoNew.bpo_abpePolygonEdges.New(3);
    // set edges data
    bpoNew.bpo_abpePolygonEdges[0].bpe_pbedEdge = pbpev1e0->bpe_pbedEdge;
    bpoNew.bpo_abpePolygonEdges[0].bpe_bReverse = pbpev1e0->bpe_bReverse;
    bpoNew.bpo_abpePolygonEdges[1].bpe_pbedEdge = pbpev1e1->bpe_pbedEdge;
    bpoNew.bpo_abpePolygonEdges[1].bpe_bReverse = pbpev1e1->bpe_bReverse;
    bpoNew.bpo_abpePolygonEdges[2].bpe_pbedEdge = &abedEdgesNew[iEdge];
    bpoNew.bpo_abpePolygonEdges[2].bpe_bReverse = TRUE;

    // allocate and set vertex pointers
    bpoNew.bpo_apbvxTriangleVertices.New(3);
    bpoNew.bpo_apbvxTriangleVertices[0] = pv1se;
    bpoNew.bpo_apbvxTriangleVertices[1] = pvNew1;
    bpoNew.bpo_apbvxTriangleVertices[2] = pvNew0;

    // copy triangle and triangle elements arrays
    bpoNew.bpo_aiTriangleElements.New(3);
    bpoNew.bpo_aiTriangleElements[0] = 0;
    bpoNew.bpo_aiTriangleElements[1] = 1;
    bpoNew.bpo_aiTriangleElements[2] = 2;

    // initialize shadow map
    bpoNew.InitializeShadowMap();
  }

  // copy new arrays over old ones
  bsc_abedEdges.MoveArray( abedEdgesNew);
  bsc_abpoPolygons.MoveArray( abpoPolygonsNew);

  // create array of working edges
  INDEX cted = bsc_abedEdges.Count();
  bsc_awedEdges.Clear();
  bsc_awedEdges.New(cted);
  // for each edge
  for (INDEX ied=0; ied<cted; ied++)
  {
    CWorkingEdge &wed = bsc_awedEdges[ied];
    CBrushEdge   &bed = bsc_abedEdges[ied];
    // setup its working edge
    bed.bed_pwedWorking = &wed;
    wed.wed_iwvx0 = bsc_abvxVertices.Index(bed.bed_pbvxVertex0);
    wed.wed_iwvx1 = bsc_abvxVertices.Index(bed.bed_pbvxVertex1);
  }

  // recalculate planes for polygons from their vertices
  MakePlanesFromVertices();
  UpdateSector();
}


// get amount of memory used by this object
SLONG CBrushSector::GetUsedMemory(void)
{
  // init
  SLONG slUsedMemory = sizeof(CBrushSector);
  // add some more
  slUsedMemory += bsc_strName.Length();
  slUsedMemory += bsc_rdOtherSidePortals.Count() * sizeof(CRelationLnk);
  slUsedMemory += bsc_rsEntities.Count()         * sizeof(CRelationLnk);
  return slUsedMemory;
}
