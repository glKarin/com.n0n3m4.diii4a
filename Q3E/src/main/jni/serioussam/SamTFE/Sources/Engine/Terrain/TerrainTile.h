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

#ifndef SE_INCL_TERRAIN_TILE_H
#define SE_INCL_TERRAIN_TILE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Graphics/Vertex.h>
#include <Engine/Terrain/ArrayHolder.h>

#define NB_TOP    0
#define NB_LEFT   1
#define NB_BOTTOM 2
#define NB_RIGHT  3

#define TT_REGENERATE         (1UL<<0) // tile needs to be regenerated
#define TT_NO_TOPMAP_REGEN    (1UL<<1) // when regenerating tile do not regenerate top map
#define TT_NO_GEOMETRY_REGEN  (1UL<<2) // when regenerating tile do not regenerate geometry
#define TT_QUADTREENODE_REGEN (1UL<<3) // when regenerating tile also regenerate it quad tree node box
#define TT_NO_LODING          (1UL<<4) // when regenerating tile do not use lod
#define TT_FORCE_TOPMAP_REGEN (1UL<<5) // force top map regen
#define TT_IN_LOWEST_LOD      (1UL<<6) // tile in lowest lod and has no additional vertices inserted

class ENGINE_API CTerrainTile
{
public:
  CTerrainTile();
  ~CTerrainTile();
  // Render tile
  void Render(void);
  // Regenerate tile
  void ReGenerate(void);
  // Regenerate tile layer 
  void ReGenerateTileLayer(INDEX iTileLayer);
  // Release tile
  void Clear(void);
  // Terrain tile flags handling
  inline ULONG &GetFlags()             { return tt_ulTileFlags; }
  inline void SetFlags(ULONG ulFlags)  { tt_ulTileFlags  = ulFlags; }
  inline void AddFlag(ULONG ulFlag)    { tt_ulTileFlags |= ulFlag; }
  inline void RemoveFlag(ULONG ulFlag) { tt_ulTileFlags &= ~ulFlag; }

  CStaticStackArray<GFXVertex4>   &GetVertices();
  CStaticStackArray<GFXTexCoord>  &GetTexCoords();
  CStaticStackArray<GFXTexCoord>  &GetShadowMapTC();
  CStaticStackArray<GFXTexCoord>  &GetDetailTC();
  CStaticStackArray<INDEX_T>      &GetIndices();
  CStaticStackArray<TileLayer>    &GetTileLayers();
  CTextureData                    *GetTopMap();

  INDEX ChangeTileArrays(INDEX iRequestedArrayLod);
  void ReleaseTileArrays();
  void EmptyTileArrays();

  // Calculate lod of tile 
  INDEX CalculateLOD(void);
  // Update quad tree node
  void UpdateQuadTreeNode();
  // Count used memory
  SLONG GetUsedMemory(void);

//temp:
void AddTriangle(INDEX iind1,INDEX iind2,INDEX iind3);
void AddVertex(INDEX ic, INDEX ir);
void LerpVertexPos(GFXVertex4 &vtx, INDEX iVxTarget, INDEX iVxFirst,INDEX iVxLast);
void PrepareSmothVertices();


private:
  // Regenerate left border
  void ReGenerateLeftBorder();
  // Regenerate top border
  void ReGenerateTopBorder();
  // Regenerate right border
  void ReGenerateRightBorder();
  // Regenerate bottom border
  void ReGenerateBottomBorder();


public:
  INDEX tt_ctVtxX;    // Number of vertices in row 
  INDEX tt_ctVtxY;    // Number of vertices in col
  INDEX tt_ctLodVtxX; // Number of vertices in row for current lod
  INDEX tt_ctLodVtxY; // Number of vertices in col for current lod

  INDEX tt_iIndex;    // Index of this tile 
  INDEX tt_iLod;      // Current lod of tile
  INDEX tt_iRequestedLod;   // Requested lod for tile
  INDEX tt_iArrayIndex;     // Index of array holder this tile uses
  INDEX tt_aiNeighbours[4]; // Array of tile neighbours

  INDEX tt_iFirstBorderVertex[4];// Index of first border vertex inserted
  INDEX tt_ctBorderVertices[4];  // Number of vertices inserted for each border

  FLOAT tt_fLodLerpFactor;  // Lod lerp factor
  ULONG tt_ulTileFlags;
  BOOL  tt_bUseOnlyGlobalTopMap; // Allways use global top map

  INDEX tt_iOffsetX;  // Offset of this tile in world
  INDEX tt_iOffsetZ;  // Offset of this tile in world
private:
  
};

#endif
