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
#include <Engine/Terrain/TerrainMisc.h>

// !!! FIXME: This is messing up name mangling. Need to look at this. --ryan.
#if ((defined PLATFORM_UNIX) && (defined __forceinline))
#  undef __forceinline
#  define __forceinline
#endif

#include <Engine/Terrain/TerrainTile.h>

extern CTerrain *_ptrTerrain;
extern FLOAT3D  _vViewerAbs;

#define BORDERTEST 0

extern CStaticStackArray<GFXVertex4> _avLerpedVerices;

CTerrainTile::CTerrainTile()
{
  tt_iIndex = -1;
  tt_iArrayIndex = -1;
  tt_iLod = -1;
  tt_iRequestedLod = 0;
  tt_ulTileFlags   = 0;
}

// Render tile
void CTerrainTile::Render(void)
{
  ASSERT(FALSE);
}

CTerrainTile::~CTerrainTile()
{
  Clear();
}

// Release tile
void CTerrainTile::Clear()
{
}

// TEMP!!!!!
__forceinline void CTerrainTile::LerpVertexPos(GFXVertex4 &vtx, INDEX iVxTarget, INDEX iVxFirst,INDEX iVxLast)
{
  GFXVertex4 &vxFirst  = GetVertices()[iVxFirst];
  GFXVertex4 &vxLast   = GetVertices()[iVxLast];
  GFXVertex4 &vxTarget = GetVertices()[iVxTarget];
  
  FLOAT fLerpMaxPosY = Lerp(vxFirst.y,vxLast.y,0.5f);
  FLOAT fLerpResultY = Lerp(vxTarget.y,fLerpMaxPosY,tt_fLodLerpFactor);
  vtx.x = vxTarget.x;
  vtx.y = fLerpResultY;
  vtx.z = vxTarget.z;
}

/*
 * Tile memory alloc
 */ 

INDEX CTerrainTile::ChangeTileArrays(INDEX iRequestedArrayLod)
{
  // if requested lod is same as current lod
  if(iRequestedArrayLod==tt_iLod) {
    // Just pop all arrays
    GetVertices().PopAll();
    GetTexCoords().PopAll();
    GetShadowMapTC().PopAll();
    GetIndices().PopAll();
    // if tile is in highest lod
    if(tt_iLod==0) {
      // pop detail uvmap
      GetDetailTC().PopAll();
      // for each tile layer
      INDEX cttl=GetTileLayers().Count();
      for(INDEX itl=0;itl<cttl;itl++) {
        // pop all arrays for this layer
        TileLayer &tl = GetTileLayers()[itl];
        tl.tl_acColors.PopAll();
        tl.tl_atcTexCoords.PopAll();
        tl.tl_auiIndices.PopAll();
        tl.tl_avVertices.PopAll();
      }
    }
    return tt_iLod;
  }

  // release current tile arrays
  ReleaseTileArrays();
  // Allocate new arrays for new lod
  CArrayHolder &ah = _ptrTerrain->tr_aArrayHolders[iRequestedArrayLod];

  ASSERT(tt_iArrayIndex==-1);
  tt_iArrayIndex = ah.GetNewArrays();

  // if this is first lod
  if(iRequestedArrayLod==0) {
    // Add tile layers
    INDEX ctLayers = _ptrTerrain->tr_atlLayers.Count();
    if(ctLayers>0) {
      GetTileLayers().Push(ctLayers);
    }
  }
  return iRequestedArrayLod;
}

void CTerrainTile::ReleaseTileArrays()
{
  // if tile had some arrays
  if(tt_iArrayIndex != -1) {
    // Free them
    CArrayHolder &ahOld = _ptrTerrain->tr_aArrayHolders[tt_iLod];
    ahOld.FreeArrays(tt_iArrayIndex);
    tt_iArrayIndex = -1;
  }
}

void CTerrainTile::EmptyTileArrays()
{
  ASSERT(tt_iArrayIndex != -1);

  CArrayHolder &ahCurrent = _ptrTerrain->tr_aArrayHolders[tt_iLod];
  ahCurrent.EmptyArrays(tt_iArrayIndex);
}

/*
 *  Tile generation (TEMP)
 */ 

inline void CTerrainTile::AddTriangle(INDEX iind1,INDEX iind2,INDEX iind3)
{
  // is this tile in highest lod
  if(tt_iLod==0) {

    // Is this triangle visible
    GFXVertex *pvx[3];
    pvx[0] = &GetVertices()[iind1];
    pvx[1] = &GetVertices()[iind2];
    pvx[2] = &GetVertices()[iind3];

    // check if all vertices all visible
    SLONG slTriangleMask = pvx[0]->shade + pvx[1]->shade + pvx[2]->shade;
    if(slTriangleMask!=255*3) {
      return;
    }

    // Add one triangle
    INDEX_T *pIndices = GetIndices().Push(3);
    pIndices[0] = iind1;
    pIndices[1] = iind2;
    pIndices[2] = iind3;

    // for each layer
    INDEX cttl = GetTileLayers().Count();
    for(INDEX itl=0;itl<cttl;itl++) {
      TileLayer &ttl = GetTileLayers()[itl];
      CTerrainLayer &tl = _ptrTerrain->tr_atlLayers[itl];
      // if this is tile layer
      if(tl.tl_ltType==LT_TILE) {
        continue; // skip it
      }
	  COLOR ul = ttl.tl_acColors[iind1].ub.a + ttl.tl_acColors[iind2].ub.a + ttl.tl_acColors[iind3].ub.a;
      if(ul>0) {
        INDEX_T *pIndices = ttl.tl_auiIndices.Push(3);
        pIndices[0] = iind1;
        pIndices[1] = iind2;
        pIndices[2] = iind3;
      }
    }
  } else {
    INDEX_T *pIndices = GetIndices().Push(3);
    pIndices[0] = iind1;
    pIndices[1] = iind2;
    pIndices[2] = iind3;
  }
}

// Returns a height in heightmap
inline FLOAT GetHeight(INDEX ic,INDEX ir,INDEX iTileIndex)
{
  CTerrainTile &tt = _ptrTerrain->tr_attTiles[iTileIndex];
  // Get Y position of vertex from heightmap
  INDEX icHMap = ic + tt.tt_iOffsetX*_ptrTerrain->GetQuadsPerTileRow();
  INDEX irHMap = ir + tt.tt_iOffsetZ*_ptrTerrain->GetQuadsPerTileRow();
  INDEX ivx = icHMap+irHMap*_ptrTerrain->tr_pixHeightMapWidth;
  return (FLOAT)_ptrTerrain->tr_auwHeightMap[ivx];
}

BYTE GetVertexAlpha(INDEX ic,INDEX ir,INDEX iTileIndex,INDEX iLayer)
{
  CTerrainTile &tt = _ptrTerrain->tr_attTiles[iTileIndex];
  INDEX icHMap = ic + tt.tt_iOffsetX*_ptrTerrain->GetQuadsPerTileRow();
  INDEX irHMap = ir + tt.tt_iOffsetZ*_ptrTerrain->GetQuadsPerTileRow();
  INDEX ivx = icHMap+irHMap*_ptrTerrain->tr_pixHeightMapWidth;
  CTerrainLayer &tl = _ptrTerrain->tr_atlLayers[iLayer];
  return tl.tl_aubColors[ivx];
}

// Returns vertex at specified position inside one tile
GFXVertex4 GetVertex(INDEX ic,INDEX ir,INDEX iTileIndex)
{
  CTerrainTile &tt = _ptrTerrain->tr_attTiles[iTileIndex];
  FLOAT fPosY = GetHeight(ic,ir,iTileIndex);

  GFXVertex4 vx;
  INDEX ix = ic + tt.tt_iOffsetX*_ptrTerrain->GetQuadsPerTileRow();
  INDEX iz = ir + tt.tt_iOffsetZ*_ptrTerrain->GetQuadsPerTileRow();
  vx.x = (FLOAT)(ix);
  vx.z = (FLOAT)(iz);
  vx.y = fPosY;
  // Fill 'shade' with edge map value
  INDEX iMask = ix + iz*_ptrTerrain->tr_pixHeightMapWidth;
  vx.shade = _ptrTerrain->tr_aubEdgeMap[iMask];

  return vx;
}

// Add vertex to array of vertices
void CTerrainTile::AddVertex(INDEX ic, INDEX ir)
{
  GFXVertex4 &vxFinal = GetVertices().Push();
  GFXTexCoord &tcShadow = GetShadowMapTC().Push();

  const GFXVertex4 &vx = GetVertex(ic,ir,tt_iIndex);
  vxFinal.x = vx.x * _ptrTerrain->tr_vStretch(1);
  vxFinal.y = vx.y * _ptrTerrain->tr_vStretch(2);
  vxFinal.z = vx.z * _ptrTerrain->tr_vStretch(3);
  vxFinal.shade = vx.shade;

  // if this tile is in highest lod
  if(tt_iLod==0) {
    // for each layer
    INDEX cttl = GetTileLayers().Count();
    for(INDEX itl=0;itl<cttl;itl++) {
      TileLayer &ttl = GetTileLayers()[itl];
      CTerrainLayer &tl = _ptrTerrain->tr_atlLayers[itl];
      // Set vertex color
      GFXColor &col = ttl.tl_acColors.Push();
      BYTE bAlpha = GetVertexAlpha(ic,ir,tt_iIndex,itl);
	  col.ul.abgr = 0x00FFFFFF;
	  col.ub.a = bAlpha;
      // if this is normal layer
      if(tl.tl_ltType == LT_NORMAL) {
        // Set its texcoords
        GFXTexCoord &tc = ttl.tl_atcTexCoords.Push();
		tc.uv.u = (FLOAT)ic;
		tc.uv.v = (FLOAT)ir;
      }
    }

    GFXTexCoord &tcDetail = GetDetailTC().Push();
	tcDetail.uv.u = ic * 2;
	tcDetail.uv.v = ir * 2;
  // if tile is in lowest lod
  } else if(tt_iLod==_ptrTerrain->tr_iMaxTileLod) {
    GFXTexCoord &tc = GetTexCoords().Push();
    FLOAT fWidth = (_ptrTerrain->tr_pixHeightMapWidth-1);
    FLOAT fHeight = (_ptrTerrain->tr_pixHeightMapHeight-1);
	tc.uv.u = vx.x / fWidth;
	tc.uv.v = vx.z / fHeight;
  // tile is not in highest lod nor in lowest lod
  } else {
    GFXTexCoord &tc = GetTexCoords().Push();
	tc.uv.u = ((vx.x - tt_iOffsetX * _ptrTerrain->GetQuadsPerTileRow()) / (_ptrTerrain->GetQuadsPerTileRow()));
	tc.uv.v = ((vx.z - tt_iOffsetZ * _ptrTerrain->GetQuadsPerTileRow()) / (_ptrTerrain->GetQuadsPerTileRow()));
  }
  tcShadow.uv.u = vx.x / (_ptrTerrain->tr_pixHeightMapWidth - 1);
  tcShadow.uv.v = vx.z / (_ptrTerrain->tr_pixHeightMapHeight - 1);
}

void CTerrainTile::ReGenerateTileLayer(INDEX iTileLayer)
{
  FLOAT fStrX = _ptrTerrain->tr_vStretch(1);
  FLOAT fStrY = _ptrTerrain->tr_vStretch(2);
  FLOAT fStrZ = _ptrTerrain->tr_vStretch(3);
  PIX pixHMWidth = _ptrTerrain->tr_pixHeightMapWidth;

  PIX iOffsetX = tt_iOffsetX * _ptrTerrain->tr_ctQuadsInTileRow;
  PIX iOffsetZ = tt_iOffsetZ * _ptrTerrain->tr_ctQuadsInTileRow;

  INDEX ctQuadsPerRow    = _ptrTerrain->tr_ctQuadsInTileRow;

  CTerrainLayer &tl = _ptrTerrain->tr_atlLayers[iTileLayer];
  TileLayer &ttl = GetTileLayers()[iTileLayer];
  INDEX ctVertices = ctQuadsPerRow*ctQuadsPerRow*4; // four vertices per one quad
  INDEX ctIndices  = ctQuadsPerRow*ctQuadsPerRow*6; // six  indices per one quad

  ASSERT(ttl.tl_avVertices.Count()==0 && ttl.tl_atcTexCoords.Count()==0 && ttl.tl_auiIndices.Count()==0);
  GFXVertex   *pvtx = ttl.tl_avVertices.Push(ctVertices);
  GFXTexCoord *ptc  = ttl.tl_atcTexCoords.Push(ctVertices);
  INDEX_T     *pind = ttl.tl_auiIndices.Push(ctIndices);
  UBYTE       *pubMask = tl.tl_aubColors;

  INDEX ivx  = 0;
  INDEX iind = 0;
  BOOL  bFacing = FALSE;
  INDEX iTilesInRowLog2 = FastLog2(tl.tl_ctTilesInRow);
  // for each quad in tile
  for(INDEX iz=0;iz<ctQuadsPerRow;iz++) {
    for(INDEX ix=0;ix<ctQuadsPerRow;ix++) {
      PIX pix = ix+iOffsetX + (iz+iOffsetZ)*pixHMWidth;

      // Add four vertices for this quad
      pvtx[ivx  ].x = (FLOAT)(iOffsetX+ix+0)*fStrX;
      pvtx[ivx  ].y = (FLOAT)_ptrTerrain->tr_auwHeightMap[pix] * fStrY;
      pvtx[ivx  ].z = (FLOAT)(iOffsetZ+iz+0)*fStrZ;
      pvtx[ivx+1].x = (FLOAT)(iOffsetX+ix+1)*fStrX;
      pvtx[ivx+1].y = (FLOAT)_ptrTerrain->tr_auwHeightMap[pix+1] * fStrY;
      pvtx[ivx+1].z = (FLOAT)(iOffsetZ+iz+0)*fStrZ;
      pvtx[ivx+2].x = (FLOAT)(iOffsetX+ix+0)*fStrX;
      pvtx[ivx+2].y = (FLOAT)_ptrTerrain->tr_auwHeightMap[pix+pixHMWidth] * fStrY;
      pvtx[ivx+2].z = (FLOAT)(iOffsetZ+iz+1)*fStrZ;
      pvtx[ivx+3].x = (FLOAT)(iOffsetX+ix+1)*fStrX;
      pvtx[ivx+3].y = (FLOAT)_ptrTerrain->tr_auwHeightMap[pix+pixHMWidth+1] * fStrY;
      pvtx[ivx+3].z = (FLOAT)(iOffsetZ+iz+1)*fStrZ;

      UBYTE ubMask   = pubMask[pix];
      INDEX iTile    = ubMask&TL_TILE_INDEX; // First 4 bits
      BOOL  bFlipX   = (ubMask&TL_FLIPX)>>TL_FLIPX_SHIFT;
      BOOL  bFlipY   = (ubMask&TL_FLIPY)>>TL_FLIPY_SHIFT;
      BOOL  bSwapXY  = (ubMask&TL_SWAPXY)>>TL_SWAPXY_SHIFT;
      BOOL  bVisible = (ubMask&TL_VISIBLE)>>TL_VISIBLE_SHIFT;
      INDEX iTileX   = iTile&(tl.tl_ctTilesInRow-1);
      INDEX iTileY   = iTile>>iTilesInRowLog2;

      ASSERT(iTileX<tl.tl_ctTilesInRow);
      ASSERT(iTileY<tl.tl_ctTilesInCol);

	  // Add four texcoords
	  ptc[ivx].uv.u = tl.tl_fTileU * (iTileX + bFlipX);
	  ptc[ivx].uv.v = tl.tl_fTileV * (iTileY + bFlipY);
	  ptc[ivx + 1].uv.u = tl.tl_fTileU * (iTileX + 1 - bFlipX);
	  ptc[ivx + 1].uv.v = tl.tl_fTileV * (iTileY + bFlipY);
	  ptc[ivx + 2].uv.u = tl.tl_fTileU * (iTileX + bFlipX);
	  ptc[ivx + 2].uv.v = tl.tl_fTileV * (iTileY + 1 - bFlipY);
	  ptc[ivx + 3].uv.u = tl.tl_fTileU * (iTileX + 1 - bFlipX);
	  ptc[ivx + 3].uv.v = tl.tl_fTileV * (iTileY + 1 - bFlipY);

	  if (bSwapXY) {
		  Swap(ptc[ivx + 1].uv.u, ptc[ivx + 2].uv.u);
		  Swap(ptc[ivx + 1].uv.v, ptc[ivx + 2].uv.v);
	  }

      // if tile is visible
      if(bVisible) {
        // Add six indices 
        if(bFacing) {
          pind[iind  ] = ivx;
          pind[iind+1] = ivx+2;
          pind[iind+2] = ivx+1;
          pind[iind+3] = ivx+1;
          pind[iind+4] = ivx+2;
          pind[iind+5] = ivx+3;
        } else {
          pind[iind  ] = ivx+2;
          pind[iind+1] = ivx+3;
          pind[iind+2] = ivx;
          pind[iind+3] = ivx;
          pind[iind+4] = ivx+3;
          pind[iind+5] = ivx+1;
        }
        iind+=6;
      } else {
        ctIndices-=6;
      }

      ivx+=4;
      bFacing=!bFacing;
    }
    bFacing=!bFacing;
  }

  // discard triangles that arn't visible
  if(ctIndices<ctQuadsPerRow*ctQuadsPerRow*6) {
    ttl.tl_auiIndices.PopUntil(ctIndices);
  }
  // discart vertices that arn't visible
  if(ctVertices<ctQuadsPerRow*ctQuadsPerRow*4) {
    ttl.tl_avVertices.PopUntil(ctVertices);
    ttl.tl_atcTexCoords.PopUntil(ctVertices);
  }

  ASSERT(ivx==ctVertices);
  ASSERT(iind==ctIndices);
}

// Regenerate tile
void CTerrainTile::ReGenerate()
{
  // remember lod before regen
  INDEX iOldLod = tt_iLod;
  // Allocate arrays for requested lod
  tt_iLod = ChangeTileArrays(tt_iRequestedLod);

  // for each vertex in row
  INDEX iStep = 1<<tt_iLod;
  INDEX ir;
  for(ir=0;ir<tt_ctVtxY;ir+=iStep) {
    // for each vertex in col
    for(INDEX ic=0;ic<tt_ctVtxX;ic+=iStep) {
      // add vertex in this row and col
      AddVertex(ic,ir);
    }
  }

  // Calculate number of quads for this lod
  INDEX ctQuads = _ptrTerrain->GetQuadsPerTileRow()>>tt_iLod;

  // Fill middle of tile with triangles
  for(ir=1;ir<ctQuads-1;ir++) {
    for(INDEX ic=1;ic<ctQuads-1;ic++) {
      INDEX ivx = ic+ir*(ctQuads+1);
      if(ivx%2) {
        AddTriangle(ivx,ivx+ctQuads+1,ivx+1);
        AddTriangle(ivx+1,ivx+ctQuads+1,ivx+ctQuads+2);
      } else {
        AddTriangle(ivx+ctQuads+1,ivx+ctQuads+2,ivx);
        AddTriangle(ivx,ivx+ctQuads+2,ivx+1);
      }
    }
  }

  //INDEX ctVtxBefore = GetVertices().Count();
  //INDEX ctTrisBefore = GetIndices().Count()/3;

  // tt_ctNormalVertices = GetVertexCount();
  // Generate borders for tile
  ReGenerateTopBorder();
  ReGenerateLeftBorder();
  ReGenerateBottomBorder();
  ReGenerateRightBorder();

  // if this tile is in first lod
  if(tt_iLod==0) {
    // for each layer
    INDEX cttl = _ptrTerrain->tr_atlLayers.Count();
    for(INDEX itl=0;itl<cttl;itl++) {
      CTerrainLayer &tl = _ptrTerrain->tr_atlLayers[itl];
      // if this is tile layer 
      if(tl.tl_ltType == LT_TILE) {
        // Regenerate it
        ReGenerateTileLayer(itl);
      }
    }
  }

  BOOL bAllowTopMapRegen = !(GetFlags()&TT_NO_TOPMAP_REGEN);
  // if top map is allowed to be regenerated
  if(bAllowTopMapRegen) {
    // if tile is not in highest nor in lowest lod
    if(tt_iLod>0 && tt_iLod<_ptrTerrain->tr_iMaxTileLod) {
      // if top map regen is forced or tile has changed lod
      BOOL bForceTopMapRegen = (GetFlags()&TT_FORCE_TOPMAP_REGEN);
      if(bForceTopMapRegen || iOldLod!=tt_iLod) {
        // Update tile top map
        _ptrTerrain->UpdateTopMap(tt_iIndex);
        // remove flag that forced top map regen
        RemoveFlag(TT_FORCE_TOPMAP_REGEN);
        // allow terrain to regenerete top map
        _ptrTerrain->AddFlag(TR_ALLOW_TOP_MAP_REGEN);
      }
    }
  // if not 
  } else {
    // regenerate it next time
    RemoveFlag(TT_NO_TOPMAP_REGEN);
  }

  // if flag to resize quad tree node has been set
  if(GetFlags()&TT_QUADTREENODE_REGEN) {
    // update quad tree node
    UpdateQuadTreeNode();
    // node has been updated
    RemoveFlag(TT_QUADTREENODE_REGEN);
  }

  INDEX ctBorderVertices = tt_ctBorderVertices[0] + tt_ctBorderVertices[1] + 
                           tt_ctBorderVertices[2] + tt_ctBorderVertices[3];
  // if tile is in lowest lod, has not lerp factor and no border vertices
  if(tt_iLod==_ptrTerrain->tr_iMaxTileLod && tt_fLodLerpFactor==0.0f && ctBorderVertices == 0) {
    // mark it as available for batch rendering
    AddFlag(TT_IN_LOWEST_LOD);
  } else {
    RemoveFlag(TT_IN_LOWEST_LOD);
  }
    
}

INDEX CTerrainTile::CalculateLOD(void)
{
  QuadTreeNode &qtn = _ptrTerrain->tr_aqtnQuadTreeNodes[tt_iIndex];
  FLOAT fDistance = (qtn.qtn_aabbox.Center() - _vViewerAbs).Length() - qtn.qtn_aabbox.Size().Length() / 2;

  // if flag has been set for tile to regenerate without lod
  if(GetFlags()&TT_NO_LODING) {
    // set new lod at 0
    fDistance = 0;
    // if tile is in highest lod, no need to regenerate texture
    // AddFlag(TT_NO_TOPMAP_REGEN);

    // remove flag for no loding
    RemoveFlag(TT_NO_LODING);
  }

  // Calculate new lod
  INDEX iNewLod = Clamp((INDEX)(fDistance/_ptrTerrain->tr_fDistFactor),(INDEX)0,_ptrTerrain->tr_iMaxTileLod);

  // if lod has changed
  if(iNewLod!=tt_iLod) {
    // add to regeneration queue
    _ptrTerrain->AddTileToRegenQueue(tt_iIndex);
    // for each neighbour
    for(INDEX in=0;in<4;in++) {
      INDEX ini = tt_aiNeighbours[in];
      // if neighbour is valid
      if(ini>=0) {
        //CTerrainTile &ttNeigbour = _ptrTerrain->tr_attTiles[ini];
        // if neighbour is in higher lod
        if(TRUE) { /*ttNeigbour.tt_iLod > tt.tt_iNewLod*/
          // add neighbour to regen queue
          _ptrTerrain->AddTileToRegenQueue(ini);
        }
      }
    }
    // Calculate num of vertices for row and col in current lod
    tt_ctLodVtxX = (_ptrTerrain->GetQuadsPerTileRow() >> iNewLod) + 1;
    tt_ctLodVtxY = (_ptrTerrain->GetQuadsPerTileRow() >> iNewLod) + 1;
  }

  // Calculate lerp factor
  tt_fLodLerpFactor = Clamp(fDistance/_ptrTerrain->tr_fDistFactor - iNewLod,0.0f,1.0f);
  // if tile is in lowest lod
  if(iNewLod == _ptrTerrain->tr_iMaxTileLod) {
    // no lerping for this tile
    tt_fLodLerpFactor = 0.0f;
  }
  // return new lod
  return iNewLod;
}

// Update quad tree node
void CTerrainTile::UpdateQuadTreeNode()
{
  // resize aabox for this node
  FLOATaabbox3D bboxNewBox;
  GFXVertex4 *pavVertices;
  INDEX_T    *paiIndices;
  INDEX       ctVertices;
  INDEX       ctIndices;
  QuadTreeNode &qtn = _ptrTerrain->tr_aqtnQuadTreeNodes[tt_iIndex];

  // prepare box that will extract vertices (x and z of old box are allready valid)
  bboxNewBox = qtn.qtn_aabbox;
  bboxNewBox.minvect(2) = 0;
  bboxNewBox.maxvect(2) = 65536 * _ptrTerrain->tr_vStretch(2);

  // extract vertices in box
  ExtractPolygonsInBox(_ptrTerrain,bboxNewBox,&pavVertices,&paiIndices,ctVertices,ctIndices);

  // if some vertices exists
  if(ctVertices>0) {
    qtn.qtn_aabbox = FLOAT3D(pavVertices->x,pavVertices->y,pavVertices->z);
    pavVertices++;
  } else {
    ASSERTALWAYS("Some vertices must exisits for tile bbox");
  }

  // for each vertex in box after first
  for(INDEX ivx=1;ivx<ctVertices;ivx++) {
    // add vertex to box
    qtn.qtn_aabbox |= FLOAT3D(pavVertices->x,pavVertices->y,pavVertices->z);
    pavVertices++;
  }

  // notify terrain that it needs to update higher levels of quad tree
  _ptrTerrain->AddFlag(TR_REBUILD_QUADTREE);
}

// Regenerate top border
void CTerrainTile::ReGenerateTopBorder()
{
  INDEX iTopTileIndex = tt_aiNeighbours[NB_TOP];
  INDEX iTopBorderLod = tt_iLod;
  // If top neighbour exists
  if(iTopTileIndex!=(-1)) {
    CTerrainTile &ttTop = _ptrTerrain->tr_attTiles[iTopTileIndex];
    iTopBorderLod = ttTop.tt_iRequestedLod; // !!!! iLod 2 iRequested
  }
#if BORDERTEST
  iTopBorderLod = 0;
#endif


  INDEX iStep       = 1<<tt_iLod;         // Tile step in vertices for this lod
  INDEX iBorderStep = 1<<iTopBorderLod;   // Border tile step for border lod
  INDEX ctQuads     = tt_ctVtxX-iStep;    // Number of quads in this tile 
  INDEX ctVtxInsert = iStep-iBorderStep;  // Number of vertices to insert in each quad
  INDEX iLastVx     = 0;                  // Last vertex inserted in quads
  INDEX icVtx       = 1;                  // Existing tile vertex index in column, using step one
  INDEX iVtxRow     = 0;                  // Row to add vertices in
  INDEX iBaseFanVtx = (tt_ctVtxX-1) / iStep + 3;  // Index of fan triangle to connect to

  tt_iFirstBorderVertex[NB_TOP] = GetVertices().Count();

  // Add half of topleft corner
  INDEX icb;
  for(icb=0;icb<ctVtxInsert;icb+=iBorderStep) {
    AddVertex(icb+iBorderStep,0);
    INDEX ivxAdded = GetVertices().Count()-1;
    AddTriangle(iBaseFanVtx-1,ivxAdded,iLastVx);
    iLastVx = ivxAdded;
  }
  // Close top left fan
  AddTriangle(iBaseFanVtx-1,icVtx,iLastVx);

  // Insert aditional vertices into each quad in top row after first
  for(INDEX ic=iStep;ic<ctQuads-iStep;ic+=iStep) {
    iLastVx = icVtx;
    if(icVtx%2) {
      // for each vertex nedeed to be inserted in this quad
      for(INDEX icb=0;icb<ctVtxInsert;icb+=iBorderStep) {
        // Insert vertex into quad and add one triangle
        AddVertex(ic+icb+iBorderStep,iVtxRow);
        INDEX ivxAdded = GetVertices().Count()-1;
        AddTriangle(iBaseFanVtx-1,ivxAdded,iLastVx);
        iLastVx = ivxAdded;
      }
      // Close fan
      AddTriangle(iBaseFanVtx-1,icVtx+1,iLastVx);
      // Add last fan triangle
      AddTriangle(iBaseFanVtx,icVtx+1,iBaseFanVtx-1);
    } else {
      // Add first fan triangle
      AddTriangle(iBaseFanVtx,icVtx,iBaseFanVtx-1);
      // for each vertex nedeed to be inserted in this quad
      for(INDEX icb=0;icb<ctVtxInsert;icb+=iBorderStep) {
        // Insert vertex into quad and add one triangle
        AddVertex(ic+icb+iBorderStep,iVtxRow);
        INDEX ivxAdded = GetVertices().Count()-1;
        AddTriangle(iBaseFanVtx,ivxAdded,iLastVx);
        iLastVx = ivxAdded;
      }
      // Close fan
      AddTriangle(iBaseFanVtx,icVtx+1,iLastVx);
    }
    iBaseFanVtx++;
    icVtx++;
  }

  iLastVx = icVtx;
  // Add half of topright corner
  for(icb=0;icb<ctVtxInsert;icb+=iBorderStep) {
    // Insert vertex into quad and add one triangle
    AddVertex(icb+iBorderStep+ctQuads-1,0);
    INDEX ivxAdded = GetVertices().Count()-1;
    AddTriangle(iBaseFanVtx-1,ivxAdded,iLastVx);
    iLastVx = ivxAdded;
  }
  // Close top right fan
  AddTriangle(iBaseFanVtx-1,icVtx+1,iLastVx);

  tt_ctBorderVertices[NB_TOP] = GetVertices().Count() - tt_iFirstBorderVertex[NB_TOP];
}
// Regenerate left border
void CTerrainTile::ReGenerateLeftBorder()
{
  INDEX iLeftTileIndex = tt_aiNeighbours[NB_LEFT];
  INDEX iLeftBorderLod = tt_iLod;
  // If top neighbour exists
  if(iLeftTileIndex!=(-1)) {
    CTerrainTile &ttLeft = _ptrTerrain->tr_attTiles[iLeftTileIndex];
    iLeftBorderLod = ttLeft.tt_iRequestedLod;
  }
#if BORDERTEST
  iLeftBorderLod = 0;
#endif


  INDEX iStep       = 1<<tt_iLod;         // Tile step in vertices for this lod
  INDEX iBorderStep = 1<<iLeftBorderLod;  // Border tile step for border lod
  INDEX ctQuads     = tt_ctVtxX-iStep;// Number of quads in this tile 
  INDEX ctVtxInsert = iStep-iBorderStep;  // Number of vertices to insert in each quad
  INDEX iLastVx     = 0;                  // Last vertex inserted in quads
  INDEX iVtxCol     = 0;                  // Col to add vertices in
  INDEX iBaseStep   = (tt_ctVtxX-1) / iStep + 1;
  INDEX irVtx       = iBaseStep;          // Existing tile vertex index in row, using step one
  INDEX iBaseFanVtx = iBaseStep + 1;      // Index of fan triangle to connect to
  
  tt_iFirstBorderVertex[NB_LEFT] = GetVertices().Count();

  // Add half of topleft corner
  INDEX irb;
  for(irb=0;irb<ctVtxInsert;irb+=iBorderStep) {
    AddVertex(iVtxCol,irb+iBorderStep);
    INDEX ivxAdded = GetVertices().Count()-1;
    AddTriangle(iBaseFanVtx,iLastVx,ivxAdded);
    iLastVx = ivxAdded;
  }
  // Close top left fan
  AddTriangle(iBaseFanVtx,iLastVx,irVtx);

  // Insert aditional vertices into each quad in top row after first
  for(INDEX ir=iStep;ir<ctQuads-iStep;ir+=iStep) {
    iLastVx = irVtx;
    if(irVtx%2) {
      // for each vertex nedeed to be inserted in this quad
      for(INDEX irb=0;irb<ctVtxInsert;irb+=iBorderStep) {
        // Insert vertex into quad and add one triangle
        AddVertex(iVtxCol,ir+irb+iBorderStep);
        INDEX ivxAdded = GetVertices().Count()-1;
        AddTriangle(iBaseFanVtx,iLastVx,ivxAdded);
        iLastVx = ivxAdded;
      }
      // Close fan
      AddTriangle(iBaseFanVtx,iLastVx,irVtx+iBaseStep);
      // Add last fan triangle
      AddTriangle(iBaseFanVtx+iBaseStep,iBaseFanVtx,irVtx+iBaseStep);
    } else {
      // Add first fan triangle
      AddTriangle(iBaseFanVtx,iLastVx,iBaseFanVtx+iBaseStep);
      // for each vertex nedeed to be inserted in this quad
      for(INDEX irb=0;irb<ctVtxInsert;irb+=iBorderStep) {
        // Insert vertex into quad and add one triangle
        AddVertex(iVtxCol,ir+irb+iBorderStep);
        INDEX ivxAdded = GetVertices().Count()-1;
        AddTriangle(iBaseFanVtx+iBaseStep,iLastVx,ivxAdded);
        iLastVx = ivxAdded;
      }
      // Close fan
      AddTriangle(iBaseFanVtx+iBaseStep,iLastVx,irVtx+iBaseStep);
    }
    iBaseFanVtx+=iBaseStep;
    irVtx+=iBaseStep;
  }

  iLastVx = irVtx;
  // Add half of bottomleft corner
  for(irb=0;irb<ctVtxInsert;irb+=iBorderStep) {
    // Insert vertex into quad and add one triangle
    AddVertex(iVtxCol,irb+iBorderStep+ctQuads-1);
    INDEX ivxAdded = GetVertices().Count()-1;
    AddTriangle(iBaseFanVtx,iLastVx,ivxAdded);
    iLastVx = ivxAdded;
  }
  // Close top right fan
  AddTriangle(iBaseFanVtx,iLastVx,irVtx+iBaseStep);

  tt_ctBorderVertices[NB_LEFT] = GetVertices().Count() - tt_iFirstBorderVertex[NB_LEFT];
}
// Regenerate right border
void CTerrainTile::ReGenerateRightBorder()
{
  INDEX iRightTileIndex = tt_aiNeighbours[NB_RIGHT];
  INDEX iRightBorderLod = tt_iLod;
  // If top neighbour exists
  if(iRightTileIndex!=(-1)) {
    CTerrainTile &ttRight = _ptrTerrain->tr_attTiles[iRightTileIndex];
    iRightBorderLod = ttRight.tt_iRequestedLod;
  }
#if BORDERTEST
  iRightBorderLod = 0;
#endif


  INDEX iStep       = 1<<tt_iLod;         // Tile step in vertices for this lod
  INDEX iBorderStep = 1<<iRightBorderLod;  // Border tile step for border lod
  INDEX ctQuads     = tt_ctVtxX-iStep;// Number of quads in this tile 
  INDEX ctVtxInsert = iStep-iBorderStep;  // Number of vertices to insert in each quad
  INDEX iLastVx     = 0;                  // Last vertex inserted in quads
  INDEX iVtxCol     = tt_ctVtxX-1;    // Col to add vertices in
  INDEX iBaseStep   = (tt_ctVtxX-1) / iStep + 1;
  INDEX irVtx       = iBaseStep-1;         // Existing tile vertex index in row, using step one
  INDEX iBaseFanVtx = iBaseStep - 2 +iBaseStep;      // Index of fan triangle to connect to
  
  tt_iFirstBorderVertex[NB_RIGHT] = GetVertices().Count();

  iLastVx = irVtx;
  // Add half of topleft corner
  INDEX irb;
  for(irb=0;irb<ctVtxInsert;irb+=iBorderStep) {
    AddVertex(iVtxCol,irb+iBorderStep);
    INDEX ivxAdded = GetVertices().Count()-1;
    AddTriangle(iBaseFanVtx,ivxAdded,iLastVx);
    iLastVx = ivxAdded;
  }

  // Close top left fan
  AddTriangle(iBaseFanVtx,irVtx+iBaseStep,iLastVx);
  irVtx+=iBaseStep;
  // Insert aditional vertices into each quad in top row after first
  for(INDEX ir=iStep;ir<ctQuads-iStep;ir+=iStep) {
    iLastVx = irVtx;
    if(irVtx%2) {
      // Add first fan triangle
      AddTriangle(iBaseFanVtx,iBaseFanVtx+iBaseStep,iLastVx+iBaseStep);
      // for each vertex nedeed to be inserted in this quad
      for(INDEX irb=0;irb<ctVtxInsert;irb+=iBorderStep) {
        // Insert vertex into quad and add one triangle
        AddVertex(iVtxCol,ir+irb+iBorderStep);
        INDEX ivxAdded = GetVertices().Count()-1;
        AddTriangle(iBaseFanVtx,ivxAdded,iLastVx);
        iLastVx = ivxAdded;
      }
      // Close fan
      AddTriangle(iBaseFanVtx,irVtx+iBaseStep,iLastVx);
    } else {
      // Add first fan triangle
      AddTriangle(iBaseFanVtx,iBaseFanVtx+iBaseStep,iLastVx);
      // for each vertex nedeed to be inserted in this quad
      for(INDEX irb=0;irb<ctVtxInsert;irb+=iBorderStep) {
        // Insert vertex into quad and add one triangle
        AddVertex(iVtxCol,ir+irb+iBorderStep);
        INDEX ivxAdded = GetVertices().Count()-1;
        AddTriangle(iBaseFanVtx+iBaseStep,ivxAdded,iLastVx);
        iLastVx = ivxAdded;
      }
      // Close fan
      AddTriangle(iBaseFanVtx+iBaseStep,irVtx+iBaseStep,iLastVx);

    }
    iBaseFanVtx+=iBaseStep;
    irVtx+=iBaseStep;
  }
  
  iLastVx = irVtx;
  // Add half of bottomleft corner
  for(irb=0;irb<ctVtxInsert;irb+=iBorderStep) {
    // Insert vertex into quad and add one triangle
    AddVertex(iVtxCol,irb+iBorderStep+ctQuads-1);
    INDEX ivxAdded = GetVertices().Count()-1;
    AddTriangle(iBaseFanVtx,ivxAdded,iLastVx);
    iLastVx = ivxAdded;
  }
  // Close top right fan
  AddTriangle(iBaseFanVtx,irVtx+iBaseStep,iLastVx);

  tt_ctBorderVertices[NB_RIGHT] = GetVertices().Count() - tt_iFirstBorderVertex[NB_RIGHT];
}
// Regenerate bottom border
void CTerrainTile::ReGenerateBottomBorder()
{
  INDEX iBottomTileIndex = tt_aiNeighbours[NB_BOTTOM];
  INDEX iBottomBorderLod = tt_iRequestedLod;
  // If bottom neighbour exists
  if(iBottomTileIndex!=(-1)) {
    CTerrainTile &ttBottom = _ptrTerrain->tr_attTiles[iBottomTileIndex];
    iBottomBorderLod = ttBottom.tt_iRequestedLod;
  }
#if BORDERTEST
  iBottomBorderLod = 0;
#endif


  INDEX iStep       = 1<<tt_iLod;         // Tile step in vertices for this lod
  INDEX iBorderStep = 1<<iBottomBorderLod;// Border tile step for border lod
  INDEX ctQuads     = tt_ctVtxX-iStep;// Number of quads in this tile 
  INDEX ctVtxInsert = iStep-iBorderStep;  // Number of vertices to insert in each quad
  INDEX iLastVx     = 0;                  // Last vertex inserted in quads
  INDEX iBaseFanVtx = (((tt_ctVtxX-1) / iStep)*((tt_ctVtxX-1) / iStep)) + 1;  // Index of fan triangle to connect to
  INDEX iVtxRow     = tt_ctVtxX-1;    // Row to add vertices in
  INDEX icVtx       = iBaseFanVtx + (tt_ctVtxX-1)/ iStep; // Existing tile vertex index in column, using step one
  
  tt_iFirstBorderVertex[NB_BOTTOM] = GetVertices().Count();

  iLastVx = icVtx-1;
  // Add half of bottom left corner
  INDEX icb;
  for(icb=0;icb<ctVtxInsert;icb+=iBorderStep) {
    AddVertex(icb+iBorderStep,iVtxRow);
    INDEX ivxAdded = GetVertices().Count()-1;
    AddTriangle(iBaseFanVtx-1,iLastVx,ivxAdded);
    iLastVx = ivxAdded;
  }
  // Close bottom left fan
  AddTriangle(iBaseFanVtx-1,iLastVx,icVtx);

  // Insert aditional vertices into each quad in top row after first
  for(INDEX ic=iStep;ic<ctQuads-iStep;ic+=iStep) {
    iLastVx = icVtx;
    if(icVtx%2) {
      // Add first fan triangle
      AddTriangle(iBaseFanVtx-1,icVtx+1,iBaseFanVtx);
      // for each vertex nedeed to be inserted in this quad
      for(INDEX icb=0;icb<ctVtxInsert;icb+=iBorderStep) {
        // Insert vertex into quad and add one triangle
        AddVertex(ic+icb+iBorderStep,iVtxRow);
        INDEX ivxAdded = GetVertices().Count()-1;
        AddTriangle(iBaseFanVtx-1,iLastVx,ivxAdded);
        iLastVx = ivxAdded;
      }
      // Close fan
      AddTriangle(iBaseFanVtx-1,iLastVx,icVtx+1);
    } else {
      // Add first fan triangle
      AddTriangle(iBaseFanVtx-1,icVtx,iBaseFanVtx);
      // for each vertex nedeed to be inserted in this quad
      for(INDEX icb=0;icb<ctVtxInsert;icb+=iBorderStep) {
        // Insert vertex into quad and add one triangle
        AddVertex(ic+icb+iBorderStep,iVtxRow);
        INDEX ivxAdded = GetVertices().Count()-1;
        AddTriangle(iBaseFanVtx,iLastVx,ivxAdded);
        iLastVx = ivxAdded;
      }
      // Close fan
      AddTriangle(iBaseFanVtx,iLastVx,icVtx+1);
    }
    iBaseFanVtx++;
    icVtx++;
  }

  iLastVx = icVtx;
  // Add half of bottomright corner
  for(icb=0;icb<ctVtxInsert;icb+=iBorderStep) {
    // Insert vertex into quad and add one triangle
    AddVertex(icb+iBorderStep+ctQuads-1,iVtxRow);
    INDEX ivxAdded = GetVertices().Count()-1;
    AddTriangle(iBaseFanVtx-1,iLastVx,ivxAdded);
    iLastVx = ivxAdded;
  }
  // Close bottom right fan
  AddTriangle(iBaseFanVtx-1,iLastVx,icVtx+1);

  tt_ctBorderVertices[NB_BOTTOM] = GetVertices().Count() - tt_iFirstBorderVertex[NB_BOTTOM];
}


__forceinline CStaticStackArray<GFXVertex4> &CTerrainTile::GetVertices() {
  ASSERT(tt_iArrayIndex!=-1);
  ASSERT(tt_iLod!=-1);
  CArrayHolder &ah = _ptrTerrain->tr_aArrayHolders[tt_iLod];
  TileArrays &ta = ah.ah_ataTileArrays[tt_iArrayIndex];
  return ta.ta_avVertices;
}
__forceinline CStaticStackArray<GFXTexCoord> &CTerrainTile::GetTexCoords() {
  ASSERT(tt_iArrayIndex!=-1);
  ASSERT(tt_iLod!=-1);
  CArrayHolder &ah = _ptrTerrain->tr_aArrayHolders[tt_iLod];
  TileArrays &ta = ah.ah_ataTileArrays[tt_iArrayIndex];
  return ta.ta_auvTexCoords;
}
__forceinline CStaticStackArray<GFXTexCoord> &CTerrainTile::GetShadowMapTC() {
  ASSERT(tt_iArrayIndex!=-1);
  ASSERT(tt_iLod!=-1);
  CArrayHolder &ah = _ptrTerrain->tr_aArrayHolders[tt_iLod];
  TileArrays &ta = ah.ah_ataTileArrays[tt_iArrayIndex];
  return ta.ta_auvShadowMap;
}
__forceinline CStaticStackArray<GFXTexCoord> &CTerrainTile::GetDetailTC() {
  ASSERT(tt_iArrayIndex!=-1);
  ASSERT(tt_iRequestedLod==0 || tt_iLod==0);
  CArrayHolder &ah = _ptrTerrain->tr_aArrayHolders[tt_iRequestedLod];
  TileArrays &ta = ah.ah_ataTileArrays[tt_iArrayIndex];
  return ta.ta_auvDetailMap;
}
__forceinline CStaticStackArray<INDEX_T> &CTerrainTile::GetIndices() {
  ASSERT(tt_iArrayIndex!=-1);
  ASSERT(tt_iLod!=-1);
  CArrayHolder &ah = _ptrTerrain->tr_aArrayHolders[tt_iLod];
  TileArrays &ta = ah.ah_ataTileArrays[tt_iArrayIndex];
  return ta.ta_auiIndices;
}
__forceinline CStaticStackArray<TileLayer> &CTerrainTile::GetTileLayers() {
  ASSERT(tt_iArrayIndex!=-1);
  ASSERT(tt_iRequestedLod==0 || tt_iLod==0);
  CArrayHolder &ah = _ptrTerrain->tr_aArrayHolders[tt_iRequestedLod];
  TileArrays &ta = ah.ah_ataTileArrays[tt_iArrayIndex];
  return ta.ta_atlLayers;
}
__forceinline CTextureData *CTerrainTile::GetTopMap()
{
  ASSERT(tt_iArrayIndex!=-1);
  ASSERT(tt_iLod!=-1);
  ASSERT(tt_iLod!=0);
  ASSERT(tt_iLod!=_ptrTerrain->tr_iMaxTileLod);
  CArrayHolder &ah = _ptrTerrain->tr_aArrayHolders[tt_iRequestedLod];
  TileArrays &ta = ah.ah_ataTileArrays[tt_iArrayIndex];
  return ta.ta_ptdTopMap;
}

// Count used memory
SLONG CTerrainTile::GetUsedMemory(void)
{
  SLONG slUsedMemory=0;
  slUsedMemory += sizeof(CTerrainTile);
  return slUsedMemory;
}
