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

#include <Engine/Math/Object3D.h>
#include <Engine/Templates/BSP_internal.h>

#include <Engine/Templates/DynamicArray.cpp>

/*
 * CSG operation table action.
 */
enum CSGAction {
  CSGA_WallA1,           // wall of sector A1
  CSGA_WallA1_2,         // wall of sector A1 second part
  CSGA_PortalA1A2,       // portal between A1 and A2
  CSGA_PortalA1A2_2,     // portal between A1 and A2 second part
  CSGA_PortalA1B1B1A1,   // two portals: A1-B1 and B1-A1
  CSGA_WallB1,           // wall of sector B1
  CSGA_PortalB1B2,       // portal between B1 and B2
  CSGA_Remove,           // this polygon is removed
  CSGA_Proceed,          // this polygon proceeds with testing
};

/*
 * CSG operation table.
 */
struct CSGOperationTable {
  enum CSGAction cot_WallA1InsideB1,     cot_WallA1OutsideB,     cot_WallA1OnB1BorderInside,      cot_WallA1OnB1BorderOutside;
  enum CSGAction cot_PortalA1A2InsideB1, cot_PortalA1A2OutsideB, cot_PortalA1A2OnB1BorderInside,  cot_PortalA1A2OnB1BorderOutside;
};

/*
  NOTE: In following tables, the operations for object B have A and B sectors reversed.
  That is because tables are always interpreted as being for object A, therefore
  when using it for B, sector B means 'sector of other object' that is in fact sector A
  and vice versa.
  For readable tables see "CSG.doc".
*/

/*
 * CSG operation tables for 'add rooms'.
 */
static struct CSGOperationTable csgotAddRoomsA = {
  CSGA_PortalA1B1B1A1,  CSGA_WallA1,        CSGA_WallA1,          CSGA_PortalA1B1B1A1,
  CSGA_PortalA1A2,      CSGA_PortalA1A2,    CSGA_PortalA1A2,      CSGA_PortalA1A2,
};
static struct CSGOperationTable csgotAddRoomsB = {
  CSGA_Remove,          CSGA_WallA1,        CSGA_Remove,          CSGA_Remove,
  CSGA_Remove,          CSGA_PortalA1A2,    CSGA_Remove,          CSGA_Remove,
};

/*
 * CSG operation tables for 'add material'.
 */
static struct CSGOperationTable csgotAddMaterialA = {
  CSGA_WallA1,          CSGA_Remove,    CSGA_Remove,          CSGA_Remove,
  CSGA_PortalA1A2,      CSGA_Remove,    CSGA_Remove,          CSGA_Remove,
};
static struct CSGOperationTable csgotAddMaterialB = {
  CSGA_WallB1,          CSGA_Remove,    CSGA_WallB1,          CSGA_Proceed,
  CSGA_Remove,          CSGA_Remove,    CSGA_Remove,          CSGA_Remove,
};
static struct CSGOperationTable csgotAddMaterialReverseA = {
  CSGA_WallA1,          CSGA_Remove,    CSGA_WallA1,          CSGA_Remove,
  CSGA_PortalA1A2,      CSGA_Remove,    CSGA_Remove,          CSGA_Remove,
};
static struct CSGOperationTable csgotAddMaterialReverseB = {
  CSGA_WallB1,          CSGA_Remove,    CSGA_Remove,          CSGA_Proceed,
  CSGA_Remove,          CSGA_Remove,    CSGA_Remove,          CSGA_Remove,
};

/*
 * CSG operation tables for 'split sectors'.
 */
static struct CSGOperationTable csgotSplitSectorsA = {
  CSGA_WallB1,          CSGA_WallA1,        CSGA_WallB1,          CSGA_WallA1,
  CSGA_PortalB1B2,      CSGA_PortalA1A2,    CSGA_PortalB1B2,      CSGA_PortalA1A2,
};
static struct CSGOperationTable csgotSplitSectorsB = {
  CSGA_PortalA1B1B1A1,  CSGA_Remove,        CSGA_Remove,          CSGA_Remove,
  CSGA_Remove,          CSGA_Remove,        CSGA_Remove,          CSGA_Remove,
};

/*
 * CSG operation tables for 'join sectors'.
 */
static struct CSGOperationTable csgotJoinSectorsA = {
  CSGA_Remove,      CSGA_WallA1,        CSGA_WallA1,          CSGA_Remove,
  CSGA_Remove,      CSGA_PortalA1A2,    CSGA_PortalA1A2,      CSGA_Remove,
};
static struct CSGOperationTable csgotJoinSectorsB = {
  CSGA_Remove,      CSGA_WallB1,        CSGA_Remove,          CSGA_Remove,
  CSGA_Remove,      CSGA_PortalB1B2,    CSGA_Remove,          CSGA_Remove,
};

/*
 * CSG operation tables for 'split polygons'.
 */
static struct CSGOperationTable csgotSplitPolygonsA = {
  CSGA_WallA1_2,          CSGA_WallA1,        CSGA_WallA1_2,          CSGA_WallA1,
  CSGA_PortalA1A2_2,      CSGA_PortalA1A2,    CSGA_PortalA1A2_2,      CSGA_PortalA1A2,
};
static struct CSGOperationTable csgotSplitPolygonsB = {
  CSGA_Remove,      CSGA_Remove,        CSGA_Remove,          CSGA_Remove,
  CSGA_Remove,      CSGA_Remove,        CSGA_Remove,          CSGA_Remove,
};

/*
 * Create a new polygon that will be a piece of some split polygon.
 */
static inline CObjectPolygon *CreatePieceOfPolygon(
      CObjectSector *poscSector,
      const CObjectPolygon &opoOriginal,
      BOOL bReversePlane)
{
  // create new polygon
  CObjectPolygon *popoPiece = poscSector->osc_aopoPolygons.New(1);

  // if the plane should be reversed
  if (bReversePlane) {
    // create a new reversed plane
    CObjectPlane *poplNew = poscSector->osc_aoplPlanes.New(1);
    *poplNew = -*opoOriginal.opo_Plane;
    popoPiece->opo_Plane = poplNew;
  // otherwise
  } else {
    // create a new plane (not reversed)
    CObjectPlane *poplNew = poscSector->osc_aoplPlanes.New(1);
    *poplNew = *opoOriginal.opo_Plane;
    popoPiece->opo_Plane = poplNew;
  }

  // create a new material
  CObjectMaterial *pomtNew = poscSector->osc_aomtMaterials.New(1);
  *pomtNew = *opoOriginal.opo_Material;
  popoPiece->opo_Material = pomtNew;

  // copy other attributes
  popoPiece->opo_colorColor = opoOriginal.opo_colorColor;
  memcpy(popoPiece->opo_amdMappings, opoOriginal.opo_amdMappings,
    sizeof(opoOriginal.opo_amdMappings));
  popoPiece->opo_ulFlags = opoOriginal.opo_ulFlags;
  memcpy(popoPiece->opo_ubUserData, opoOriginal.opo_ubUserData, OPO_MAXUSERDATA);
  return popoPiece;
}

/*
 * A machine for doing CSG operations on an CObject3D object.
 */
class CObjectCSG {
public:
  // pointers to sectors and polygons that is currently operating on
  CObjectSector *oc_poscSectorA;
  CObjectSector *oc_poscSectorB;

  CObjectPolygon *oc_popoA;

  CObjectPolygon *oc_popoWallA1;
  CObjectPolygon *oc_popoWallA1_2;
  CObjectPolygon *oc_popoPortalA1A2;
  CObjectPolygon *oc_popoPortalA1A2_2;
  CObjectPolygon *oc_popoPortalA1B1;

  CObjectPolygon *oc_popoWallB1;
  CObjectPolygon *oc_popoPortalB1A1;
  CObjectPolygon *oc_popoPortalB1B2;

  inline CObjectPolygon *GetWallA1(void);
  inline CObjectPolygon *GetWallA1_2(void);
  inline CObjectPolygon *GetPortalA1A2(void);
  inline CObjectPolygon *GetPortalA1A2_2(void);
  inline CObjectPolygon *GetPortalA1B1(void);

  inline CObjectPolygon *GetWallB1(void);
  inline CObjectPolygon *GetPortalB1A1(void);
  inline CObjectPolygon *GetPortalB1B2(void);

  BOOL oc_bCSGIngoringEnabled;
  BOOL oc_bSkipObjectB;

  // array for holding edges that proceed with testing
  CDynamicArray<DOUBLEbspedge3D> oc_abedProceeding;

  CObjectCSG(void) {
    oc_bCSGIngoringEnabled = FALSE;
    oc_bSkipObjectB = FALSE;
  }

  /* Add an entire array of BSP edges to some polygon according to action code. */
  inline void AddEdgeArrayAccordingToAction(
      CDynamicArray<DOUBLEbspedge3D> &abed,
      enum CSGAction csga);

  /* Fill array of bsp edges from array of polygon edges. */
  void PolygonEdgesToBSPEdges(
      CDynamicArray<CObjectPolygonEdge> &aope,
      CDynamicArray<DOUBLEbspedge3D> &abed
    );

public:
  /* Perform CSG splitting of sectors in one operand using other operand. */
  void DoCSGSplitting(
      CObject3D &obResult,
      CObject3D &obA,
      INDEX iSectorOffsetA,
      struct CSGOperationTable *pcsgotA,
      CObject3D &obB,
      INDEX iSectorOffsetB);
  /* Perform CSG operation -- destroys both operands! */
  void DoCSGOperation(
      CObject3D &obResult,
      CObject3D &obA,
      CObject3D &obB,
      struct CSGOperationTable *pcsgotA,
      struct CSGOperationTable *pcsgotB);
};

CObjectPolygon *CObjectCSG::GetWallA1(void)
{
  if (oc_popoWallA1==NULL) {
    oc_popoWallA1 = CreatePieceOfPolygon(oc_poscSectorA, *oc_popoA, FALSE);
    oc_popoWallA1->opo_ulFlags &= ~OPOF_PORTAL;
  }
  return oc_popoWallA1;
}

CObjectPolygon *CObjectCSG::GetWallA1_2(void)
{
  if (oc_popoWallA1_2==NULL) {
    oc_popoWallA1_2 = CreatePieceOfPolygon(oc_poscSectorA, *oc_popoA, FALSE);
    oc_popoWallA1_2->opo_ulFlags &= ~OPOF_PORTAL;
  }
  return oc_popoWallA1_2;
}

CObjectPolygon *CObjectCSG::GetPortalA1A2(void)
{
  if (oc_popoPortalA1A2==NULL) {
    oc_popoPortalA1A2 = CreatePieceOfPolygon(oc_poscSectorA, *oc_popoA, FALSE);
    oc_popoPortalA1A2->opo_ulFlags |= OPOF_PORTAL;
  }
  return oc_popoPortalA1A2;
}

CObjectPolygon *CObjectCSG::GetPortalA1A2_2(void)
{
  if (oc_popoPortalA1A2_2==NULL) {
    oc_popoPortalA1A2_2 = CreatePieceOfPolygon(oc_poscSectorA, *oc_popoA, FALSE);
    oc_popoPortalA1A2_2->opo_ulFlags |= OPOF_PORTAL;
  }
  return oc_popoPortalA1A2_2;
}

CObjectPolygon *CObjectCSG::GetPortalA1B1(void)
{
  if (oc_popoPortalA1B1==NULL) {
    oc_popoPortalA1B1 = CreatePieceOfPolygon(oc_poscSectorA, *oc_popoA, FALSE);
    oc_popoPortalA1B1->opo_ulFlags |= OPOF_PORTAL;
  }
  return oc_popoPortalA1B1;
}

CObjectPolygon *CObjectCSG::GetWallB1(void)
{
  if (oc_popoWallB1==NULL) {
    oc_popoWallB1 = CreatePieceOfPolygon(oc_poscSectorB, *oc_popoA, FALSE);
    oc_popoWallB1->opo_ulFlags &= ~OPOF_PORTAL;
  }
  return oc_popoWallB1;
}

CObjectPolygon *CObjectCSG::GetPortalB1A1(void)
{
  if (oc_popoPortalB1A1==NULL) {
    oc_popoPortalB1A1 = CreatePieceOfPolygon(oc_poscSectorB, *oc_popoA, TRUE); // this one is reversed !
    oc_popoPortalB1A1->opo_ulFlags |= OPOF_PORTAL;
  }
  return oc_popoPortalB1A1;
}

CObjectPolygon *CObjectCSG::GetPortalB1B2(void)
{
  if (oc_popoPortalB1B2==NULL) {
    oc_popoPortalB1B2 = CreatePieceOfPolygon(oc_poscSectorB, *oc_popoA, FALSE);
    oc_popoPortalB1B2->opo_ulFlags |= OPOF_PORTAL;
  }
  return oc_popoPortalB1B2;
}

/*
 * Fill array of bsp edges from array of polygon edges.
 */
void CObjectCSG::PolygonEdgesToBSPEdges(
    CDynamicArray<CObjectPolygonEdge> &aope,
    CDynamicArray<DOUBLEbspedge3D> &abed
  )
{
  aope.Lock();
  abed.Lock();

  // get number of edges in the polygon
  INDEX ctEdges = aope.Count();
  // create that much edges in array of bsp edges
  abed.New(ctEdges);

  // for each edge in polygon
  for(INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
    const CObjectPolygonEdge &ope = aope[iEdge];
    // if it is reversed
    if (ope.ope_Backward) {
      // add bsp edge with reverse vertices
      abed[iEdge] = DOUBLEbspedge3D(*ope.ope_Edge->oed_Vertex1,
        *ope.ope_Edge->oed_Vertex0, (size_t)ope.ope_Edge);

    // if it is not reversed
    } else{
      // add bsp edge with normal vertices
      abed[iEdge] = DOUBLEbspedge3D(*ope.ope_Edge->oed_Vertex0,
        *ope.ope_Edge->oed_Vertex1, (size_t)ope.ope_Edge);
    }
  }

  aope.Unlock();
  abed.Unlock();
}

/*
 * Add an entire array of BSP edges to some polygon according to action code.
 */
inline void CObjectCSG::AddEdgeArrayAccordingToAction(
    CDynamicArray<DOUBLEbspedge3D> &abed,
    enum CSGAction csga)
{
  // if there are no edges to process
  INDEX ctEdges = abed.Count();
  if (ctEdges==0) {
    // do nothing
    return;
  }

  // if the action is Remove
  if (csga==CSGA_Remove) {
    // do nothing
    return;
  }

  // if the action is Proceed
  if (csga==CSGA_Proceed) {
    // add entire array to array of proceeding edges
    oc_abedProceeding.MoveArray(abed);
    return;
  }

  // check the action code and find sector(s) and polygon(s) to add edges to
  CObjectPolygon *popoNormal = NULL;
  CObjectSector  *poscNormal = NULL;
  CObjectPolygon *popoReverse = NULL;
  CObjectSector  *poscReverse = NULL;
  switch (csga) {
  case CSGA_WallA1:
    poscNormal = oc_poscSectorA;
    popoNormal = GetWallA1();
    break;
  case CSGA_WallA1_2:
    poscNormal = oc_poscSectorA;
    popoNormal = GetWallA1_2();
    break;
  case CSGA_PortalA1A2_2:
    poscNormal = oc_poscSectorA;
    popoNormal = GetPortalA1A2_2();
    break;
  case CSGA_PortalA1A2:
    poscNormal = oc_poscSectorA;
    popoNormal = GetPortalA1A2();
    break;
  case CSGA_PortalA1B1B1A1:
    poscNormal = oc_poscSectorA;
    popoNormal = GetPortalA1B1();
    poscReverse = oc_poscSectorB;
    popoReverse = GetPortalB1A1();
    break;
  case CSGA_WallB1:
    poscNormal = oc_poscSectorB;
    popoNormal = GetWallB1();
    break;
  case CSGA_PortalB1B2:
    poscNormal = oc_poscSectorB;
    popoNormal = GetPortalB1B2();
    break;
  default:
    ASSERTALWAYS("Unknown CSG action code");
  }

  // create needed number of vertices, edges and polygon edges
  CObjectVertex       *aovxNormal = NULL;
  CObjectEdge         *aoedNormal = NULL;
  CObjectPolygonEdge  *aopeNormal = NULL;
  CObjectVertex       *aovxReverse = NULL;
  CObjectEdge         *aoedReverse = NULL;
  CObjectPolygonEdge  *aopeReverse = NULL;

  aovxNormal = poscNormal->osc_aovxVertices.New(2*ctEdges);
  aoedNormal = poscNormal->osc_aoedEdges.New(ctEdges);
  aopeNormal = popoNormal->opo_PolygonEdges.New(ctEdges);
  if (poscReverse!=NULL) {
    aovxReverse = poscReverse->osc_aovxVertices.New(2*ctEdges);
    aoedReverse = poscReverse->osc_aoedEdges.New(ctEdges);
    aopeReverse = popoReverse->opo_PolygonEdges.New(ctEdges);
  }

  abed.Lock();
  // add all edges to normal polygon
  {for(INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
    DOUBLEbspedge3D &bed = abed[iEdge];
    aovxNormal[iEdge*2+0] = bed.bed_vVertex0;
    aovxNormal[iEdge*2+1] = bed.bed_vVertex1;
    aoedNormal[iEdge].oed_Vertex0 = &aovxNormal[iEdge*2+0];
    aoedNormal[iEdge].oed_Vertex1 = &aovxNormal[iEdge*2+1];
    aopeNormal[iEdge].ope_Backward = FALSE;
    aopeNormal[iEdge].ope_Edge = &aoedNormal[iEdge];
  }}

  // if there is reverse polygon
  if (poscReverse!=NULL) {
  // add all edges to reverse polygon
    {for(INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
      DOUBLEbspedge3D &bed = abed[iEdge];
      aovxReverse[iEdge*2+0] = bed.bed_vVertex1;
      aovxReverse[iEdge*2+1] = bed.bed_vVertex0;
      aoedReverse[iEdge].oed_Vertex0 = &aovxReverse[iEdge*2+0];
      aoedReverse[iEdge].oed_Vertex1 = &aovxReverse[iEdge*2+1];
      aopeReverse[iEdge].ope_Backward = FALSE;
      aopeReverse[iEdge].ope_Edge = &aoedReverse[iEdge];
    }}
  }
  abed.Unlock();
}

/*
 * Perform CSG splitting of sectors in one operand using other operand.
 */
void CObjectCSG::DoCSGSplitting(
    CObject3D &obResult,
    CObject3D &obA,
    INDEX iSectorOffsetA,
    struct CSGOperationTable *pcsgotA,
    CObject3D &obB,
    INDEX iSectorOffsetB)
{
  obResult.ob_aoscSectors.Lock();
  // for each sector in A
  {FOREACHINDYNAMICARRAY(obA.ob_aoscSectors, CObjectSector, itoscA) {
    // make sector reference in result
    oc_poscSectorA = &obResult.ob_aoscSectors[iSectorOffsetA+itoscA->osc_Index];
    // copy sector properties from operand A to result
    oc_poscSectorA->osc_colColor   = itoscA->osc_colColor;
    oc_poscSectorA->osc_colAmbient = itoscA->osc_colAmbient;
    oc_poscSectorA->osc_ulFlags[0]    = itoscA->osc_ulFlags[0];
    oc_poscSectorA->osc_ulFlags[1]    = itoscA->osc_ulFlags[1];
    oc_poscSectorA->osc_ulFlags[2]    = itoscA->osc_ulFlags[2];
    oc_poscSectorA->osc_strName    = itoscA->osc_strName;

    // for each of polygons in that sector
    {FOREACHINDYNAMICARRAY(itoscA->osc_aopoPolygons, CObjectPolygon, itopoA) {
      oc_popoA = itopoA;
      // prepare appropriate actions for it
      enum CSGAction csgaInside, csgaOutside, csgaBorderInside, csgaBorderOutside, csgaSkip;
      if (itopoA->opo_ulFlags & OPOF_PORTAL) {
        csgaInside = pcsgotA->cot_PortalA1A2InsideB1;
        csgaOutside = pcsgotA->cot_PortalA1A2OutsideB;
        csgaBorderInside = pcsgotA->cot_PortalA1A2OnB1BorderInside;
        csgaBorderOutside = pcsgotA->cot_PortalA1A2OnB1BorderOutside;
        csgaSkip = CSGA_PortalA1A2;
      } else {
        csgaInside = pcsgotA->cot_WallA1InsideB1;
        csgaOutside = pcsgotA->cot_WallA1OutsideB;
        csgaBorderInside = pcsgotA->cot_WallA1OnB1BorderInside;
        csgaBorderOutside = pcsgotA->cot_WallA1OnB1BorderOutside;
        csgaSkip = CSGA_WallA1;
      }

      // create temporary array for holding remaining edges
      CDynamicArray<DOUBLEbspedge3D> abedRemaining;
      // fill the array with edges from the polygon
      PolygonEdgesToBSPEdges(itopoA->opo_PolygonEdges, abedRemaining);

      // create wall A1 polygon
      oc_popoWallA1 = NULL;
      oc_popoWallA1_2 = NULL;

      // create portal A1-A2 polygon
      oc_popoPortalA1A2 = NULL;
      oc_popoPortalA1A2_2 = NULL;

      // if the polygon should be skipped
      if (oc_bCSGIngoringEnabled && (itopoA->opo_ulFlags&OPOF_IGNOREDBYCSG)) {
        // add entire polygon according to _skip_ action
        AddEdgeArrayAccordingToAction(abedRemaining, csgaSkip);
        // skip splitting
        continue;
      }

      // for each sector in B
      {FOREACHINDYNAMICARRAY(obB.ob_aoscSectors, CObjectSector, itoscB) {
        // clear array of proceeding edges
        oc_abedProceeding.Clear();

        // make sector references in result
        oc_poscSectorB = &obResult.ob_aoscSectors[iSectorOffsetB+itoscB->osc_Index];
        // copy sector properties from operand B to result
        oc_poscSectorB->osc_colColor   = itoscB->osc_colColor;
        oc_poscSectorB->osc_colAmbient = itoscB->osc_colAmbient;
        oc_poscSectorB->osc_ulFlags[0] = itoscB->osc_ulFlags[0];
        oc_poscSectorB->osc_ulFlags[1] = itoscB->osc_ulFlags[1];
        oc_poscSectorB->osc_ulFlags[2] = itoscB->osc_ulFlags[2];
        oc_poscSectorB->osc_strName    = itoscB->osc_strName;

        // create portal A1-B1 polygon
        oc_popoPortalA1B1 = NULL;

        // create portal B1-A1 polygon
        oc_popoPortalB1A1 = NULL;

        // create wall B1 polygon
        oc_popoWallB1 = NULL;

        // create portal B1-B2 polygon
        oc_popoPortalB1B2 = NULL;

        // create a bsp polygon from first temporary array
        DOUBLEbsppolygon3D bpoA(*itopoA->opo_Plane, abedRemaining, (size_t)itopoA->opo_Plane);

        // create a BSP cutter for B's sector BSP and A's polygon
        DOUBLEbspcutter3D bcCutter(bpoA, *itoscB->osc_BSPTree.bt_pbnRoot);
        // optimize all parts of the polygon
        DOUBLEbspedge3D::OptimizeBSPEdges(bcCutter.bc_abedInside);
        DOUBLEbspedge3D::OptimizeBSPEdges(bcCutter.bc_abedBorderInside);
        DOUBLEbspedge3D::OptimizeBSPEdges(bcCutter.bc_abedBorderOutside);
        DOUBLEbspedge3D::OptimizeBSPEdges(bcCutter.bc_abedOutside);

        // add all parts that are inside according to _inside_ action
        AddEdgeArrayAccordingToAction(bcCutter.bc_abedInside, csgaInside);

        // add all parts that are on border inside according to _on_border_inside_ action
        AddEdgeArrayAccordingToAction(bcCutter.bc_abedBorderInside, csgaBorderInside);

        // add all parts that are on border outside according to _on_border_outside_ action
        AddEdgeArrayAccordingToAction(bcCutter.bc_abedBorderOutside, csgaBorderOutside);

        // clear the temporary array
        abedRemaining.Clear();

        // move all parts that are outside or proceeding to the temporary array
        abedRemaining.MoveArray(bcCutter.bc_abedOutside);
        abedRemaining.MoveArray(oc_abedProceeding);
      }}

      // add all parts that are still remaining according to _outside_ action
      AddEdgeArrayAccordingToAction(abedRemaining, csgaOutside);
    }}

  }}

  obResult.ob_aoscSectors.Unlock();
}

/*
 * Perform CSG operation -- destroys both operands!
 */
void CObjectCSG::DoCSGOperation(
    CObject3D &obResult,
    CObject3D &obA,
    CObject3D &obB,
    struct CSGOperationTable *pcsgotA,
    struct CSGOperationTable *pcsgotB)
{
  // remove current contents
  obResult.Clear();
  // create indices in operand sectors
  obA.CreateSectorIndices();
  obB.CreateSectorIndices();
  // create sector BSP trees in both operands
  obA.CreateSectorBSPs();
  obB.CreateSectorBSPs();

  // get number of sectors in operands
  INDEX ctSectorsA = obA.ob_aoscSectors.Count();
  INDEX ctSectorsB = obB.ob_aoscSectors.Count();
  // create as much sectors in result as there is in both operands
  obResult.ob_aoscSectors.New(ctSectorsA+ctSectorsB);

  // do splitting of first operand using the second operand BSPs and first table
  DoCSGSplitting(obResult, obA, 0, pcsgotA, obB, ctSectorsA);
  // do splitting of second operand using the first operand BSPs and second table
  if (!oc_bSkipObjectB) {
    DoCSGSplitting(obResult, obB, ctSectorsA, pcsgotB, obA, 0);
  }
}

/*
 *** CSG operations -- they all destroy both operands! ***
 */

/*
 * Add rooms.
 */
void CObject3D::CSGAddRooms(CObject3D &obA, CObject3D &obB)
{
  // do CSG with 'add rooms' tables
  CObjectCSG oc;
  oc.DoCSGOperation(*this, obA, obB, &csgotAddRoomsA, &csgotAddRoomsB);
}

/*
 * Add material from object B to object A. (B should have only one
 *   open sector and no closed sectors)
 */
void CObject3D::CSGAddMaterial(CObject3D &obA, CObject3D &obB)
{
  // do CSG with 'add material' tables
  CObjectCSG oc;
  oc.DoCSGOperation(*this, obA, obB, &csgotAddMaterialA, &csgotAddMaterialB);
}
void CObject3D::CSGAddMaterialReverse(CObject3D &obA, CObject3D &obB)
{
  // do CSG with 'add material' tables, but with reverse priorities
  CObjectCSG oc;
  oc.DoCSGOperation(*this, obA, obB, &csgotAddMaterialReverseA, &csgotAddMaterialReverseB);
}

/*
 * Remove material of object B from object A. (B should have only one
 *   open sector and no closed sectors)
 */
void CObject3D::CSGRemoveMaterial(CObject3D &obA, CObject3D &obB)
{
  // reverse the object B
  obB.Inverse();

  // do CSG with 'add rooms' tables, but with reversed priorities
  CObjectCSG oc;
  oc.DoCSGOperation(*this, obA, obB, &csgotAddRoomsB, &csgotAddRoomsA);
}

/*
 * Split sectors of object A using object B. (B should have only one
 *   closed sector and no open sectors)
 */
void CObject3D::CSGSplitSectors(CObject3D &obA, CObject3D &obB)
{
  // do CSG with 'split sectors' tables
  CObjectCSG oc;
  oc.DoCSGOperation(*this, obA, obB, &csgotSplitSectorsA, &csgotSplitSectorsB);
}

/* Join sectors of object A with sectors of object B. (both A and B should
  have only one sector) */
void CObject3D::CSGJoinSectors(CObject3D &obA, CObject3D &obB)
{
  // do CSG with 'join sectors' tables
  CObjectCSG oc;
  oc.DoCSGOperation(*this, obA, obB, &csgotJoinSectorsA, &csgotJoinSectorsB);
}
/* Split polygons of object A with sectors of object B. (both A and B should
  have only one sector) */
void CObject3D::CSGSplitPolygons(CObject3D &obA, CObject3D &obB)
{
  // do CSG with 'split polygons' tables
  CObjectCSG oc;
  oc.oc_bCSGIngoringEnabled = TRUE;
  oc.oc_bSkipObjectB = TRUE;
  oc.DoCSGOperation(*this, obA, obB, &csgotSplitPolygonsA, &csgotSplitPolygonsB);
}
