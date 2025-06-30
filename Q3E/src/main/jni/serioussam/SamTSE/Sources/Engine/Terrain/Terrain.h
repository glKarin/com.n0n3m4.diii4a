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

#ifndef SE_INCL_TERRAIN_H
#define SE_INCL_TERRAIN_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Base/FileName.h>
#include <Engine/Math/AABBox.h>
#include <Engine/Brushes/BrushBase.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Terrain/TerrainLayer.h>
#include <Engine/Terrain/TerrainTile.h>
#include <Engine/Terrain/ArrayHolder.h>
#include <Engine/Templates/StaticArray.cpp>

#define TR_REGENERATE              (1UL<<0) // terrain needs to be regenerated
#define TR_REGENERATE_TOP_MAP      (1UL<<1) // regenerate terrain top map
#define TR_ALLOW_TOP_MAP_REGEN     (1UL<<2) // allow top map regen (top map is not regenerate if this flag is not set)
#define TR_REBUILD_QUADTREE        (1UL<<3) // rebuild higher levels of quad tree nodes (last levels are rebuild by tile flags)
#define TR_UPDATE_SHADOWMAP        (1UL<<4) // update terrain shadow map       (NOT USED!)
#define TR_ALLOW_SHADOW_MAP_UPDATE (1UL<<5) // allow terrain shadow map update (NOT USED!)
#define TR_SHOW_SELECTION          (1UL<<6) // show selection
#define TR_HAS_FOG                 (1UL<<7) // terrain has fog
#define TR_HAS_HAZE                (1UL<<8) // terrain has haze

struct QuadTreeNode
{
  FLOATaabbox3D qtn_aabbox;    // Bounding box for this quadtree node
  INDEX         qtn_iTileIndex;// Index of tile that this node represents
  INDEX         qtn_iChild[4]; // Indices of children for this quadtree node
};

struct QuadTreeLevel
{
  INDEX qtl_iFirstNode; // Index of first quadtree node in this level
  INDEX qtl_ctNodes;    // Count of nodes in this level
  INDEX qtl_ctNodesCol; // Count of nodes in col
  INDEX qtl_ctNodesRow; // Count of nodes in row
};

struct Point {
  Point() {}
  ~Point() {}
  Point(const Point &ptOther) {
    *this = ptOther;
  }
  INDEX pt_iX;
  INDEX pt_iY;
};

struct Rect {
  Rect() {}
  ~Rect() {}
  Rect(const Rect &rcOther) {
    *this = rcOther;
  }
  Rect(INDEX iLeft, INDEX iTop, INDEX iRight, INDEX iBottom) {
    rc_iLeft   = iLeft;
    rc_iTop    = iTop;
    rc_iRight  = iRight;
    rc_iBottom = iBottom;
  }
  INDEX Width() {return rc_iRight-rc_iLeft;}
  INDEX Height() {return rc_iBottom-rc_iTop;}
  INDEX rc_iLeft;
  INDEX rc_iRight;
  INDEX rc_iTop;
  INDEX rc_iBottom;
};

class ENGINE_API CTerrain : public CBrushBase
{
public:
  CTerrain();
  ~CTerrain();

  // Render terrain tiles
  void Render(CAnyProjection3D &apr, CDrawPort *pdp);
  // Render terrain tiles in wireframe
  void RenderWireFrame(CAnyProjection3D &apr, CDrawPort *pdp, COLOR &colEdges);

  // Create empty terrain with given size
  void CreateEmptyTerrain_t(PIX pixWidth,PIX pixHeight);
  // Import height map from targa file
  void ImportHeightMap_t(CTFileName fnHeightMap, BOOL bUse16b = TRUE);
  // Export height map to targa file
  void ExportHeightMap_t(CTFileName fnHeightMap, BOOL bUse16b = TRUE);
  // Rebuild all terrain
  void ReBuildTerrain(BOOL bDelayTileRegen=FALSE);
  // Refresh terrain
  void RefreshTerrain(void);

  // Set height map size
  void AllocateHeightMap(PIX pixWidth, PIX pixHeight);
  // Change height map size
  void ReAllocateHeightMap(PIX pixWidth, PIX pixHeight);
  // Set terrain size
  void SetTerrainSize(FLOAT3D vSize);
  // Set shadow map size aspect (relative to height map size) and shading map aspect (relative to shadow map size)
  void SetShadowMapsSize(INDEX iShadowMapAspect, INDEX iShadingMapAspect);

  // Set size of top map texture
  void SetGlobalTopMapSize(PIX pixTopMapSize);
  // Set size of top map texture for tiles in lower lods
  void SetTileTopMapSize(PIX pixLodTopMapSize);
  // Set lod distance factor
  void SetLodDistanceFactor(FLOAT fLodDistance);

  // Get shadow map size
  PIX GetShadowMapWidth(void);
  PIX GetShadowMapHeight(void);
  // Get shading map size
  PIX GetShadingMapWidth(void);
  PIX GetShadingMapHeight(void);

  // Get reference to layer
  CTerrainLayer &GetLayer(INDEX iLayer);
  // Add new layer
  CTerrainLayer &AddLayer_t(CTFileName fnTexture, LayerType ltType = LT_NORMAL, BOOL bUpdateTerrain=TRUE);
  // Remove one layer
  void RemoveLayer(INDEX iLayer, BOOL bUpdateTerrain=TRUE);
  // Move layer to new position
  INDEX SetLayerIndex(INDEX iLayer,INDEX iNewIndex, BOOL bUpdateTerrain=TRUE);

  // Add tile to reqen queue
  void AddTileToRegenQueue(INDEX iTileIndex);
  // Add all tiles to regen queue
  void AddAllTilesToRegenQueue(void);
  // Clear current regen list
  void ClearRegenList(void);

  // Update shadow map
  void UpdateShadowMap(FLOATaabbox3D *pbboxUpdate=NULL, BOOL bAbsoluteSpace=FALSE);
  // Update top map
  void UpdateTopMap(INDEX iTileIndex, Rect *prcDest = NULL);

  // Terrain flags handling
  inline ULONG &GetFlags(void)         { return tr_ulTerrainFlags; }
  inline void SetFlags(ULONG ulFlags)  { tr_ulTerrainFlags  = ulFlags; }
  inline void AddFlag(ULONG ulFlag)    { tr_ulTerrainFlags |= ulFlag; }
  inline void RemoveFlag(ULONG ulFlag) { tr_ulTerrainFlags &= ~ulFlag; }

  // get first quad tree node bounding box (all tiles box)
  void GetAllTerrainBBox(FLOATaabbox3D &bbox);

  INDEX GetBrushType(void) { return CBrushBase::BT_TERRAIN; } // this is terrain not brush
  
  // Get shading color from tex coords in shading map
  COLOR GetShadeColor(CShadingInfo *psi);
  // Get plane from given point
  FLOATplane3D GetPlaneFromPoint(FLOAT3D &vAbsPoint);


  // Sets number of quads in row of one tile
  void SetQuadsPerTileRow(INDEX ctQuadsPerTileRow);
  inline INDEX GetQuadsPerTileRow(void)    {return tr_ctQuadsInTileRow;}
  inline INDEX GetVerticesPerTileRow(void) {return tr_ctVerticesInTileRow;}
 
  // Read from stream.
  void ReadVersion_t( CTStream *istrFile, INDEX iSavedVersion);
  // Read from stream.
  void Read_t( CTStream *istrFile); // throw char *
  // Write to stream.
  void Write_t( CTStream *ostrFile);  // throw char *
  // Copy terrain data from other terrain
  void Copy(CTerrain &trOther);
  // Clean terrain data (does not remove layers)
  void Clean(BOOL bCleanLayers=TRUE);
  // Clean terrain data
  void Clear(void);


  // Renders visible terrain tiles in wireframe 
  void RenderWire(void);
  // Render vertices of all terrain tiles
  void RenderPoints(void);
  // Generate terrain tiles 
  void ReGenerate(void);
  // Build terrain data
  void BuildTerrainData(void);
  // Build quadtree for terrain
  void BuildQuadTree(void);
  // Update quadtree for terrain
  void UpdateQuadTree(void);
  // Generate terrain top map
  void GenerateTerrainTopMap(void);
  // Draws one quad node and its children
  void DrawQuadNode(INDEX iqn);
  // Set Terrain stretch
  void SetTerrainStretch(FLOAT3D vStretch);
  // Add default layer
  void AddDefaultLayer_t(void);

  // Discard all cached shading info for models
  void DiscardShadingInfos(void);

  // Clear height map
  void ClearHeightMap(void);
  // Clear shadow map
  void ClearShadowMap(void);
  // Clear edge map
  void ClearEdgeMap(void);
  // Clear all topmaps
  void ClearTopMaps(void);
  // Clear tiles
  void ClearTiles(void);
  // Clear arrays
  void ClearArrays(void);
  // Clear quadtree
  void ClearQuadTree(void);
  // Clear layers
  void ClearLayers(void);

public:
  CListNode tr_lnInActiveTerrains; // for linking in list of active terrains in renderer
  CListHead tr_lhShadingInfos;     // for linking shading infos of entities
  CEntity *tr_penEntity;           // pointer to entity that holds this terrain

  INDEX   tr_ctTiles;     // Terrain tiles count
  INDEX   tr_ctTilesX;    // Terrain tiles count in row
  INDEX   tr_ctTilesY;    // Terrain tiles count in col
  INDEX   tr_iMaxTileLod; // Maximum lod in witch one tile can be

  FLOAT3D tr_vStretch;    // Terrain stretch
  FLOAT   tr_fDistFactor; // Distance for lod switching

  CStaticStackArray<QuadTreeNode>        tr_aqtnQuadTreeNodes;  // Array of quadtree nodes
  CStaticStackArray<QuadTreeLevel>       tr_aqtlQuadTreeLevels; // Array of quadtree levels
  CStaticArray<class CTerrainTile>       tr_attTiles;           // Array of terrain tiles for terrain
  CStaticArray<class CArrayHolder>       tr_aArrayHolders;      // Array of memory holders for each lod
  CStaticStackArray<class CTerrainLayer> tr_atlLayers;          // Array of terrain layers
  CDynamicContainer<class CTextureData>  tr_atdTopMaps;         // Array of top maps for each tile array (used by ArrayHolder)
  CStaticStackArray<INDEX>               tr_auiRegenList;       // List of tiles that need to be regenerated

  /* Do not change any of this params directly */
  UWORD  *tr_auwHeightMap;        // Terrain height map
  UWORD  *tr_auwShadingMap;       // Terrain shading map
  UBYTE  *tr_aubEdgeMap;          // Terrain edge map
  CTextureData tr_tdTopMap;       // Terrain top map
  CTextureData tr_tdShadowMap;    // Terrain shadow map
  CTextureData *tr_ptdDetailMap;  // Terrain detail map

  PIX     tr_pixHeightMapWidth;   // Terrain height map widht
  PIX     tr_pixHeightMapHeight;  // Terrain height map height

  PIX     tr_pixTopMapWidth;         // Width  of terrain top map
  PIX     tr_pixTopMapHeight;        // Height of terrain top map
  PIX     tr_pixFirstMipTopMapWidth; // Width  of tile first mip top map
  PIX     tr_pixFirstMipTopMapHeight;// Height of tile first mip top map

  INDEX   tr_iShadowMapSizeAspect;   // Size of shadow map (relative to height map size)
  INDEX   tr_iShadingMapSizeAspect;  // Size of shading map (relative to shadow map)

  INDEX   tr_ctQuadsInTileRow;    // Count of quads in one row in tile
  INDEX   tr_ctVerticesInTileRow; // Count of vertices in one row in tile
  ULONG   tr_ulTerrainFlags;      // Terrain flags
  FLOAT3D tr_vTerrainSize;        // Terrain size in metars

  INDEX   tr_iSelectedLayer;      // Selected layer in we
private:
  INDEX   tr_ctDriverChanges;
};

#endif

