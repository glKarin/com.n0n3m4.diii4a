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
#include <Engine/Terrain/Terrain.h>
#include <Engine/Terrain/TerrainRender.h>
#include <Engine/Terrain/TerrainEditing.h>
#include <Engine/Terrain/TerrainMisc.h>
#include <Engine/Entities/Entity.h>

extern FLOAT3D   _vViewerAbs; // Viewer pos 
extern CStaticStackArray<GFXColor>   _aiExtColors;


// Selection preview
static CTerrain *_ptrSelectionTerrain;   // Terrain that needs to show vertex selection
static CTextureData *_ptdSelectionBrush; // Brush that will be used for vertex selection preview
static GFXColor _colSelection;           // Selection color
static Rect  _rcSelectionExtract;        // Rect of selection vertices that will be shown
static FLOAT _fSelectionStrenght;        // Selection preview strenght
static SelectionFill _sfSelectionFill;   // Type of fill for selection preview


static FLOATaabbox3D CalculateAABBoxFromRect(CTerrain *ptrTerrain, Rect rcExtract)
{
  ASSERT(ptrTerrain!=NULL);
  ASSERT(ptrTerrain->tr_penEntity!=NULL);

  // Get entity that holds this terrain
  //CEntity *penEntity = ptrTerrain->tr_penEntity;

  FLOATaabbox3D bboxExtract;
  FLOATaabbox3D bboxAllTerrain;
  ptrTerrain->GetAllTerrainBBox(bboxAllTerrain);

  FLOAT fMinY = bboxAllTerrain.minvect(2);
  FLOAT fMaxY = bboxAllTerrain.maxvect(2);

  bboxExtract.minvect = FLOAT3D(rcExtract.rc_iLeft  * ptrTerrain->tr_vStretch(1),fMinY,rcExtract.rc_iTop    * ptrTerrain->tr_vStretch(3));
  bboxExtract.maxvect = FLOAT3D(rcExtract.rc_iRight * ptrTerrain->tr_vStretch(1),fMaxY,rcExtract.rc_iBottom * ptrTerrain->tr_vStretch(3));
  return bboxExtract;
}

// Find if there are any tiles in given rect that are not in lowest nor in highest lod without using TT_NO_LODING flag
static INDEX GetFirstTileInMidLod(CTerrain *ptrTerrain, Rect &rcExtract)
{
  FLOATaabbox3D bboxExtract = CalculateAABBoxFromRect(ptrTerrain,rcExtract);
  // for each terrain tile
  for(INDEX itt=0;itt<ptrTerrain->tr_ctTiles;itt++) {
    QuadTreeNode &qtn = ptrTerrain->tr_aqtnQuadTreeNodes[itt];
    //CTerrainTile &tt =  ptrTerrain->tr_attTiles[itt];
    // if it is coliding with given box
    if(qtn.qtn_aabbox.HasContactWith(bboxExtract)) {
      // calculate its real distance factor
      FLOAT fDistance = (qtn.qtn_aabbox.Center() - _vViewerAbs).Length();
      INDEX iRealLod = Clamp((INDEX)(fDistance/ptrTerrain->tr_fDistFactor),(INDEX)0,ptrTerrain->tr_iMaxTileLod);
      if(iRealLod>0 && iRealLod<ptrTerrain->tr_iMaxTileLod) {
        // found one
        return itt;
      }
    }
  }
  return -1;
}

// Add given flags to all tiles in rect
static void AddFlagsToTilesInRect(CTerrain *ptrTerrain, Rect &rcExtract, ULONG ulFlags, BOOL bRegenerateTiles=FALSE)
{
  ASSERT(ptrTerrain!=NULL);
  FLOATaabbox3D bboxExtract = CalculateAABBoxFromRect(ptrTerrain, rcExtract);
  
  // for each terrain tile
  for(INDEX itt=0;itt<ptrTerrain->tr_ctTiles;itt++) {
    QuadTreeNode &qtn = ptrTerrain->tr_aqtnQuadTreeNodes[itt];
    CTerrainTile &tt =  ptrTerrain->tr_attTiles[itt];
    // if it is coliding with given box
    if(qtn.qtn_aabbox.HasContactWith(bboxExtract)) {
      // if tile must regenerate
      if(bRegenerateTiles) {
        // add tile to regen queue
        ptrTerrain->AddTileToRegenQueue(itt);
      }
      // add given flags to tile
      tt.AddFlag(ulFlags);
    }
  }
}

// Update given rect of topmap
static void UpdateShadowMapRect(CTerrain *ptrTerrain, Rect &rcExtract)
{
  FLOATaabbox3D bboxExtract = CalculateAABBoxFromRect(ptrTerrain,rcExtract);
  ptrTerrain->UpdateShadowMap(&bboxExtract);
}

// Update terrain top map
static void UpdateTerrainGlobalTopMap(CTerrain *ptrTerrain, Rect &rcExtract)
{
  // if there aren't any tiles in 
  if(GetFirstTileInMidLod(ptrTerrain,rcExtract)==(-1)) {
    // update gloabal terrain top map now
    ptrTerrain->UpdateTopMap(-1);
  // else 
  } else {
    // gloabal terrain top map will be updated when first tile chage its lod
    ptrTerrain->AddFlag(TR_REGENERATE_TOP_MAP);
  }
}

// 
void ShowSelectionInternal(CTerrain *ptrTerrain, Rect &rcExtract, CTextureData *ptdBrush, GFXColor colSelection, FLOAT fStrenght, SelectionFill sfFill)
{
  ASSERT(ptrTerrain!=NULL);
  ASSERT(ptdBrush!=NULL);

  Rect rcSelection;
  FLOATaabbox3D bboxSelection;
  // Clamp rect used for extraction
  rcSelection.rc_iLeft   = Clamp(rcExtract.rc_iLeft   , (INDEX)0, ptrTerrain->tr_pixHeightMapWidth);
  rcSelection.rc_iTop    = Clamp(rcExtract.rc_iTop    , (INDEX)0, ptrTerrain->tr_pixHeightMapHeight);
  rcSelection.rc_iRight  = Clamp(rcExtract.rc_iRight  , (INDEX)0, ptrTerrain->tr_pixHeightMapWidth);
  rcSelection.rc_iBottom = Clamp(rcExtract.rc_iBottom , (INDEX)0, ptrTerrain->tr_pixHeightMapHeight);

  // Prepare box for vertex selection
  bboxSelection    = FLOAT3D(rcSelection.rc_iLeft,  0, rcSelection.rc_iTop);
  bboxSelection   |= FLOAT3D(rcSelection.rc_iRight, 0, rcSelection.rc_iBottom);

  // Stretch selection box
  bboxSelection.minvect(1) *= ptrTerrain->tr_vStretch(1);
  bboxSelection.minvect(3) *= ptrTerrain->tr_vStretch(3);
  bboxSelection.maxvect(1) *= ptrTerrain->tr_vStretch(1);
  bboxSelection.maxvect(3) *= ptrTerrain->tr_vStretch(3);

  // Set selection box height
  FLOATaabbox3D bboxAllTerrain;
  ptrTerrain->GetAllTerrainBBox(bboxAllTerrain);
  bboxSelection.minvect(2) = bboxAllTerrain.minvect(2);
  bboxSelection.maxvect(2) = bboxAllTerrain.maxvect(2);

  GFXVertex *pavVertices;
  INDEX_T   *paiIndices;
  INDEX      ctVertices;
  INDEX      ctIndices;
  
  // Extract vertices in selection rect
  ExtractVerticesInRect(ptrTerrain, rcSelection, &pavVertices, &paiIndices, ctVertices, ctIndices);

  if(ctVertices!=rcSelection.Width()*rcSelection.Height()) {
    ASSERT(FALSE);
    return;
  }

  // if no vertices
  if(ctVertices==0) {
    return;
  }

  // Prepare vertex colors for selection preview
  PIX pixWidth  = rcSelection.Width();
  PIX pixHeight = rcSelection.Height();
  INDEX iStepX  = ptdBrush->GetWidth() - pixWidth;
  INDEX iFirst  = 0;
  if(rcExtract.rc_iTop<0) {
    iFirst += -rcExtract.rc_iTop*ptdBrush->GetWidth();
  }
  if(rcExtract.rc_iLeft<0) {
    iFirst += -rcExtract.rc_iLeft;
  }

  _aiExtColors.Push(ctVertices);
  GFXColor *pacolColor = (GFXColor*)&_aiExtColors[0];
  GFXColor *pacolBrush = (GFXColor*)&ptdBrush->td_pulFrames[iFirst];

  // Fill vertex colors for selection preview
  SLONG slStrength = (SLONG) (Clamp(Abs(fStrenght),0.0f,1.0f) * 256.0f);
  // for each row
  for(INDEX iy=0;iy<pixHeight;iy++) {
    // for each col
    for(INDEX ix=0;ix<pixWidth;ix++) {
	  pacolColor->ul.abgr = colSelection.ul.abgr;
	  pacolColor->ub.a = (pacolBrush->ub.r*slStrength) >> 8;
      pacolColor++;
      pacolBrush++;
    }
    pacolBrush+=iStepX;
  }

  // Render selected polygons for selection preview
  if(sfFill == SF_WIREFRAME) {
    gfxPolygonMode(GFX_LINE);
    gfxEnableDepthBias();
  }

  if(sfFill != SF_POINTS) {
    // Draw selection
    gfxDisableTexture();
    gfxDisableAlphaTest();
    gfxEnableBlend();
    gfxBlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    gfxSetVertexArray(pavVertices,ctVertices);
    gfxSetColorArray(&_aiExtColors[0]);
    gfxLockArrays();
    gfxDrawElements(ctIndices,paiIndices);
    gfxUnlockArrays();
    gfxDisableBlend();
  }

  if(sfFill == SF_WIREFRAME) {
    gfxDisableDepthBias();
    gfxPolygonMode(GFX_FILL);
  }

  if(sfFill == SF_POINTS) {
    DrawSelectedVertices(pavVertices,&_aiExtColors[0],ctVertices);
  }
}

void ShowSelectionInternal(CTerrain *ptrTerrain)
{
  // just in case
  if(ptrTerrain!=_ptrSelectionTerrain) {
    return;
  }

  // Show selection
  ShowSelectionInternal(ptrTerrain,_rcSelectionExtract,_ptdSelectionBrush,_colSelection,_fSelectionStrenght,_sfSelectionFill);
}

void ShowSelection(CTerrain *ptrTerrain, Rect &rcExtract, CTextureData *ptdBrush, COLOR colSelection, FLOAT fStrenght, SelectionFill sfFill/*=SF_POLYGON*/)
{
  _ptrSelectionTerrain = ptrTerrain;
  _ptdSelectionBrush   = ptdBrush;
  _colSelection        = colSelection;
  _rcSelectionExtract  = rcExtract;
  _fSelectionStrenght  = fStrenght;
  _sfSelectionFill     = sfFill,

  // all tiles in rect must be in zero lod
  AddFlagsToTilesInRect(ptrTerrain,rcExtract,TT_NO_LODING);
  // Make sure selection is visible on next render
  ptrTerrain->AddFlag(TR_SHOW_SELECTION);
}

static void UpdateEditedTerrainTiles(CTerrain *ptrTerrain, Rect &rcExtract, BufferType btBufferType)
{
  // Update terrain tiles
  if(btBufferType == BT_HEIGHT_MAP) {
    AddFlagsToTilesInRect(ptrTerrain, rcExtract, TT_NO_LODING|TT_QUADTREENODE_REGEN, TRUE);
    UpdateShadowMapRect(ptrTerrain, rcExtract);

  } else if(btBufferType == BT_LAYER_MASK) {
    AddFlagsToTilesInRect(ptrTerrain, rcExtract, TT_NO_LODING|TT_FORCE_TOPMAP_REGEN, TRUE);
    UpdateTerrainGlobalTopMap(ptrTerrain,rcExtract);

  } else if(btBufferType == BT_EDGE_MAP) {
    AddFlagsToTilesInRect(ptrTerrain, rcExtract, TT_NO_LODING|TT_FORCE_TOPMAP_REGEN, TRUE);
    UpdateTerrainGlobalTopMap(ptrTerrain,rcExtract);

  } else {
    ASSERTALWAYS("Ilegal buffer type");
    return;
  }
}

UWORD *GetBufferForEditing(CTerrain *ptrTerrain, Rect &rcExtract, BufferType btBufferType, INDEX iBufferData/*=-1*/)
{
  ASSERT(ptrTerrain!=NULL);
  ASSERT(rcExtract.Width()>0);
  ASSERT(rcExtract.Height()>0);

  PIX pixLeft   = rcExtract.rc_iLeft;
  PIX pixRight  = rcExtract.rc_iRight;
  PIX pixTop    = rcExtract.rc_iTop;
  PIX pixBottom = rcExtract.rc_iBottom;

  PIX pixWidht  = pixRight-pixLeft;
  PIX pixHeight = pixBottom-pixTop;
  PIX pixMaxWidth  = ptrTerrain->tr_pixHeightMapWidth;
  PIX pixMaxHeight = ptrTerrain->tr_pixHeightMapHeight;

  // allocate memory for editing buffer
  UWORD *pauwEditingBuffer = (UWORD*)AllocMemory(pixWidht*pixHeight*sizeof(UWORD));

  // Get pointer to first member in editing pointer
  UWORD *puwBufferData = &pauwEditingBuffer[0];

  // if buffer type is height map
  if(btBufferType==BT_HEIGHT_MAP) {
    // Extract data from terrain height map
    UWORD *puwFirstInHeightMap = &ptrTerrain->tr_auwHeightMap[0];
    // for each row
    for(PIX pixY=pixTop;pixY<pixBottom;pixY++) {
      PIX pixRealY = Clamp(pixY, (PIX)0,pixMaxHeight-1);
      // for each col
      for(PIX pixX=pixLeft;pixX<pixRight;pixX++) {
        PIX pixRealX = Clamp(pixX, (PIX)0,pixMaxWidth-1);
        // Copy current pixel from height map to dest buffer
        UWORD *puwHeight = &puwFirstInHeightMap[pixRealX + pixRealY*pixMaxWidth];
        *puwBufferData = *puwHeight;
        puwBufferData++;
      }
    }
  // if buffer type is layer mask
  } else if(btBufferType==BT_LAYER_MASK) {
    // Extract data from layer mask
    CTerrainLayer &tl = ptrTerrain->GetLayer(iBufferData);
    UBYTE *pubFirstInLayer  = &tl.tl_aubColors[0];
    // for each row
    for(PIX pixY=pixTop;pixY<pixBottom;pixY++) {
      PIX pixRealY = Clamp(pixY, (PIX)0,pixMaxHeight-1);
      // for each col
      for(PIX pixX=pixLeft;pixX<pixRight;pixX++) {
        PIX pixRealX = Clamp(pixX, (PIX)0,pixMaxWidth-1);
        // Copy current pixel from layer mask to dest buffer
        UBYTE *pubMaskValue = &pubFirstInLayer[pixRealX + pixRealY*pixMaxWidth];
        *puwBufferData = (*pubMaskValue)<<8|(*pubMaskValue);
        puwBufferData++;
      }
    }
  // if buffer type is edge map
  } else if(btBufferType==BT_EDGE_MAP) {
    // Extract data from edge map
    UBYTE *pubFirstInEdgeMap = &ptrTerrain->tr_aubEdgeMap[0];
    // for each row
    for(PIX pixY=pixTop;pixY<pixBottom;pixY++) {
      PIX pixRealY = Clamp(pixY, (PIX)0,pixMaxHeight-1);
      // for each col
      for(PIX pixX=pixLeft;pixX<pixRight;pixX++) {
        PIX pixRealX = Clamp(pixX, (PIX)0,pixMaxWidth-1);
        // Copy current pixel from layer mask to dest buffer
        UBYTE *pubEdgeValue = &pubFirstInEdgeMap[pixRealX + pixRealY*pixMaxWidth];
        if((*pubEdgeValue)==255) {
          *puwBufferData = 1;
        } else {
          *puwBufferData = 0;
        }
        puwBufferData++;
      }
    }
  } else {
    ASSERTALWAYS("Ilegal buffer type");
  }

  return &pauwEditingBuffer[0];
}

void SetBufferForEditing(CTerrain *ptrTerrain, UWORD *puwEditedBuffer, Rect &rcExtract, BufferType btBufferType, INDEX iBufferData/*=-1*/)
{
  ASSERT(ptrTerrain!=NULL);
  ASSERT(rcExtract.Width()>0);
  ASSERT(rcExtract.Height()>0);

  PIX pixLeft   = rcExtract.rc_iLeft;
  PIX pixRight  = rcExtract.rc_iRight;
  PIX pixTop    = rcExtract.rc_iTop;
  PIX pixBottom = rcExtract.rc_iBottom;

  //PIX pixWidht  = pixRight-pixLeft;
  //PIX pixHeight = pixBottom-pixTop;
  PIX pixMaxWidth  = ptrTerrain->tr_pixHeightMapWidth;
  PIX pixMaxHeight = ptrTerrain->tr_pixHeightMapHeight;

  // Get pointer to first member in editing buffer
  UWORD *puwBufferData = &puwEditedBuffer[0];

  // if buffer type is height map
  if(btBufferType==BT_HEIGHT_MAP) {
    // put data from buffer to terrain height map
    UWORD *puwFirstInHeightMap = &ptrTerrain->tr_auwHeightMap[0];
    // for each row
    for(PIX pixY=pixTop;pixY<pixBottom;pixY++) {
      // if pixY is inside terrain rect
      if(pixY>=0 && pixY<pixMaxHeight) {
        // for each col
        for(PIX pixX=pixLeft;pixX<pixRight;pixX++) {
          // if pixX is inside terrain rect
          if(pixX>=0 && pixX<pixMaxWidth) {
            // Copy current pixel from editing buffer to height map
            UWORD *puwHeight = &puwFirstInHeightMap[pixX + pixY*pixMaxWidth];
            *puwHeight = *puwBufferData;
          }
          puwBufferData++;
        }
      // else pixY is not inside terrain rect
      } else {
        // increment buffer data pointer
        puwBufferData+=pixRight-pixLeft;
      }
    }
  } else if(btBufferType==BT_LAYER_MASK) {
    // Extract data from layer mask
    CTerrainLayer &tl = ptrTerrain->GetLayer(iBufferData);
    UBYTE *pubFirstInLayer  = &tl.tl_aubColors[0];
    // for each row
    for(PIX pixY=pixTop;pixY<pixBottom;pixY++) {
      // if pixY is inside terrain rect
      if(pixY>=0 && pixY<pixMaxHeight) {
        // for each col
        for(PIX pixX=pixLeft;pixX<pixRight;pixX++) {
          // if pixX is inside terrain rect
          if(pixX>=0 && pixX<pixMaxWidth) {
            // Copy current pixel from editing buffer to height map
            UBYTE *pubMask = &pubFirstInLayer[pixX + pixY*pixMaxWidth];
            *pubMask = (*puwBufferData)>>8;
          }
          puwBufferData++;
        }
      // else pixY is not inside terrain rect
      } else {
        // increment buffer data pointer
        puwBufferData+=pixRight-pixLeft;
      }
    }
  } else if(btBufferType==BT_EDGE_MAP) {
    // Extract data from edge map
    UBYTE *pubFirstInEdgeMap  = &ptrTerrain->tr_aubEdgeMap[0];
    // for each row
    for(PIX pixY=pixTop;pixY<pixBottom;pixY++) {
      // if pixY is inside terrain rect
      if(pixY>=0 && pixY<pixMaxHeight) {
        // for each col
        for(PIX pixX=pixLeft;pixX<pixRight;pixX++) {
          // if pixX is inside terrain rect
          if(pixX>=0 && pixX<pixMaxWidth) {
            // Copy current pixel from editing buffer to edge map
            UBYTE *pubMask = &pubFirstInEdgeMap[pixX + pixY*pixMaxWidth];
            if(*puwBufferData>=1) {
              *pubMask = 255;
            } else {
              *pubMask = 0;
            }
          }
          puwBufferData++;
        }
      // else pixY is not inside terrain rect
      } else {
        // increment buffer data pointer
        puwBufferData+=pixRight-pixLeft;
      }
    }
  } else {
    ASSERTALWAYS("Ilegal buffer type");
    return;
  }

  UpdateEditedTerrainTiles(ptrTerrain,rcExtract,btBufferType);
}



























// check whether a polygon is below given point, but not too far away
BOOL IsTerrainBelowPoint(CTerrain *ptrTerrain, const FLOAT3D &vPoint, FLOAT fMaxDist, const FLOAT3D &vGravityDir)
{
  return TRUE;
/*
    // get distance from point to the plane
    FLOAT fD = plPolygon.PointDistance(vPoint);
    // if the point is behind the plane
    if (fD<-0.01f) {
      // it cannot be below
      _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }

    // find distance of point from the polygon along gravity vector
    FLOAT fDistance = -fD/fCos;
    // if too far away
    if (fDistance > fMaxDist) {
      // it cannot be below
      _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }
    // project point to the polygon along gravity vector
    FLOAT3D vProjected = vPoint + en_vGravityDir*fDistance;

    // find major axes of the polygon plane
    INDEX iMajorAxis1, iMajorAxis2;
    GetMajorAxesForPlane(plPolygon, iMajorAxis1, iMajorAxis2);

    // create an intersector
    CIntersector isIntersector(vProjected(iMajorAxis1), vProjected(iMajorAxis2));
    // for all edges in the polygon
    FOREACHINSTATICARRAY(pbpo->bpo_abpePolygonEdges, CBrushPolygonEdge, itbpePolygonEdge) {
      // get edge vertices (edge direction is irrelevant here!)
      const FLOAT3D &vVertex0 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex0->bvx_vAbsolute;
      const FLOAT3D &vVertex1 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex1->bvx_vAbsolute;
      // pass the edge to the intersector
      isIntersector.AddEdge(
        vVertex0(iMajorAxis1), vVertex0(iMajorAxis2),
        vVertex1(iMajorAxis1), vVertex1(iMajorAxis2));
    }

    // if the point is inside polygon
    if (isIntersector.IsIntersecting()) {
      // it is below
      _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return TRUE;
    // if the point is outside polygon
    } else {
      // it is not below
      _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }
    */
}
