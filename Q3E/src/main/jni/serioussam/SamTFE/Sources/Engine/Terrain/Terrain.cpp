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
#include <Engine/Base/Stream.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Math/Projection.h>
#include <Engine/Math/FixInt.h>
#include <Engine/Graphics/DrawPort.h>
#include <Engine/Graphics/ImageInfo.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Terrain/Terrain.h>
#include <Engine/Terrain/TerrainRender.h>
#include <Engine/Terrain/TerrainEditing.h>
#include <Engine/Terrain/TerrainMisc.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Entities/ShadingInfo.h>
#include <Engine/Graphics/Font.h>
#include <Engine/Base/Console.h>
#include <Engine/Rendering/Render.h>

extern CTerrain *_ptrTerrain;

extern BOOL _bWorldEditorApp; // is this world edtior app


static INDEX _iTerrainVersion = 9;   // Current terrain version
static INDEX ctGeneratedTopMaps = 0; // TEMP
static INDEX ctGlobalTopMaps    = 0; // TEMP
INDEX  _ctShadowMapUpdates = 0;      // TEMP
// TEMP

INDEX _ctNodesVis = 0;
INDEX _ctTris = 0;
INDEX _ctDelayedNodes = 0;
static void ShowTerrainInfo(CAnyProjection3D &apr, CDrawPort *pdp, CTerrain *ptrTerrain); // TEMP

/*
 * Terrain initialization
 */
CTerrain::CTerrain()
{
  tr_vStretch = FLOAT3D(1,0.05f,1);
  tr_fDistFactor = 32;
  tr_iMaxTileLod = 0;
  tr_iSelectedLayer = 0;
  tr_ctDriverChanges = -1;

  tr_pixHeightMapWidth       = 256;
  tr_pixHeightMapHeight      = 256;
  tr_pixTopMapWidth          = 256;
  tr_pixTopMapHeight         = 256;
  tr_pixFirstMipTopMapWidth  = 256;
  tr_pixFirstMipTopMapHeight = 256;
  tr_ptdDetailMap  = NULL;
  tr_auwHeightMap  = NULL;
  tr_aubEdgeMap    = NULL;
  tr_auwShadingMap = NULL;

  // TEMP
  try {
    tr_ptdDetailMap = _pTextureStock->Obtain_t((CTString)"Textures\\Detail\\Crumples04.tex");
  } catch (const char *) {
  }

  // Set size of shadow and shading maps
  SetShadowMapsSize(0,0);

  // Set terrain size
  SetTerrainSize(FLOAT3D(256,80,256));
  // Set num of quads in one tile
  SetQuadsPerTileRow(32);

  SetFlags(TR_REGENERATE);
}

// Render visible terrain tiles
void CTerrain::Render(CAnyProjection3D &apr, CDrawPort *pdp)
{
  // prepare gfx stuff
  PrepareScene(apr, pdp, this);

  if(tr_ctDriverChanges!=_pGfx->gl_ctDriverChanges) {
    tr_ctDriverChanges = _pGfx->gl_ctDriverChanges;
    // RefreshTerrain();
  }

  // if terrain is not frozen
  extern INDEX ter_bNoRegeneration;
  if(GetFlags()&TR_REGENERATE && !ter_bNoRegeneration) {
    // Regenerate tiles
    ReGenerate();
  }
  // if shadow map must be regenerated
  if(GetFlags()&TR_UPDATE_SHADOWMAP) {
    UpdateShadowMap();
  }


  // if top map regen is allowed
  if(GetFlags()&TR_ALLOW_TOP_MAP_REGEN) {
    // if top map regen is requested
    if(GetFlags()&TR_REGENERATE_TOP_MAP) {
      // update terrain top map
      UpdateTopMap(-1);
      // remove request for top map regen
      RemoveFlag(TR_REGENERATE_TOP_MAP);
    }
    // remove flag that allows terrain to regenerate top map
    RemoveFlag(TR_ALLOW_TOP_MAP_REGEN);
  }

  // show
  RenderTerrain();

  // if flag show brush selection has been set
  if(GetFlags()&TR_SHOW_SELECTION) {
    ShowSelectionInternal(this);
    // remove show selection terrain flag
    RemoveFlag(TR_SHOW_SELECTION);
  }

  // if flag for quadtree rebuilding is set
  if(GetFlags()&TR_REBUILD_QUADTREE) {
    // Resize quadtree
    UpdateQuadTree();
    // remove flag for quadtree rebuilding
    RemoveFlag(TR_REBUILD_QUADTREE);
  }

  RemoveFlag(TR_HAS_FOG);
  RemoveFlag(TR_HAS_HAZE);

  extern INDEX ter_bShowWireframe;
  // if wireframe mode forced
  if(ter_bShowWireframe) {
    COLOR colWire = 0xFFFFFFFF;
    RenderTerrainWire(colWire);
  }

  extern INDEX ter_bShowQuadTree;
  // if showing of quad tree is required
  if(ter_bShowQuadTree) {
    DrawQuadTree();
  }

  extern INDEX ter_bShowInfo;
  if(ter_bShowInfo) {
    ShowTerrainInfo(apr, pdp, this);
  }

  _ptrTerrain = NULL;
}

void CTerrain::RenderWireFrame(CAnyProjection3D &apr, CDrawPort *pdp, COLOR &colEdges)
{
  // prepare gfx stuff
  PrepareScene(apr, pdp, this);
  // Regenerate tiles 
  if(tr_ctTiles>=0) {
    CTerrainTile &tt = tr_attTiles[0];
    if(tt.tt_iLod == -1) {
      ReGenerate();
    }
  }
  // show
  RenderTerrainWire(colEdges);
}

// Create empty terrain with given size
void CTerrain::CreateEmptyTerrain_t(PIX pixWidth,PIX pixHeight)
{
  _ptrTerrain = this;
  // Clear old terrain data if exists
  Clear();

  ASSERT(tr_auwHeightMap==NULL);
  ASSERT(tr_aubEdgeMap==NULL);
  
  AllocateHeightMap(pixWidth,pixHeight);

  AddDefaultLayer_t();
  // Rebuild terrain
  ReBuildTerrain();
  _ptrTerrain = NULL;
}

// Import height map from targa file
void CTerrain::ImportHeightMap_t(CTFileName fnHeightMap, BOOL bUse16b/*=TRUE*/)
{
  _ptrTerrain = this;
  //BOOL bResizeTerrain = FALSE;

  // Load targa file 
  CImageInfo iiHeightMap;
  iiHeightMap.LoadAnyGfxFormat_t(fnHeightMap);

  // if new width and height are same
  /* unused
  if(tr_pixHeightMapWidth==iiHeightMap.ii_Width && tr_pixHeightMapHeight==iiHeightMap.ii_Height) {
    // Clear terrain data without removing layers
    bResizeTerrain = FALSE;
  } else {
    // Clear all terrain data
    bResizeTerrain = TRUE;
  }
  bResizeTerrain = TRUE;
  */

  FLOAT fLogWidht  = Log2(iiHeightMap.ii_Width-1);
  FLOAT fLogHeight = Log2(iiHeightMap.ii_Height-1);
  if(fLogWidht!=INDEX(fLogWidht) || fLogHeight!=INDEX(fLogHeight)) {
    ThrowF_t("Invalid terrain width or height");
  }
  if(iiHeightMap.ii_Width!= iiHeightMap.ii_Height) {
    ThrowF_t("Only terrains with same width and height are supported in this version");
  }

  // Reallocate memory for terrain with size
  ReAllocateHeightMap(iiHeightMap.ii_Width, iiHeightMap.ii_Height);

  INDEX iHeightMapSize = iiHeightMap.ii_Width * iiHeightMap.ii_Height;

  UBYTE *puwSrc = &iiHeightMap.ii_Picture[0];
  UWORD *puwDst = &tr_auwHeightMap[0];
  INDEX iBpp = iiHeightMap.ii_BitsPerPixel/8;

  // for each word in loaded image
  for(INDEX iw=0;iw<iHeightMapSize;iw++) {
    // use 16 bits for importing
    if(bUse16b) {
      *puwDst = *(UWORD*)puwSrc;
    // use 8 bits for importing
    } else {
      *puwDst = *(UBYTE*)puwSrc<<8;
    }
    puwDst++;
    puwSrc+=iBpp;
  }

  // Rebuild terrain
  ReBuildTerrain();

  _ptrTerrain = NULL;
}  

// Export height map to targa file
void CTerrain::ExportHeightMap_t(CTFileName fnHeightMap, BOOL bUse16b/*=TRUE*/)
{
  ASSERT(tr_auwHeightMap!=NULL);
  INDEX iSize = tr_pixHeightMapWidth*tr_pixHeightMapHeight;

  CImageInfo iiHeightMap;
  iiHeightMap.ii_Width  = tr_pixHeightMapWidth;
  iiHeightMap.ii_Height = tr_pixHeightMapHeight;
  iiHeightMap.ii_BitsPerPixel = 32;
  iiHeightMap.ii_Picture = (UBYTE*)AllocMemory(iSize*iiHeightMap.ii_BitsPerPixel/8);

  GFXColor *pacolImage = (GFXColor*)&iiHeightMap.ii_Picture[0];
  UWORD    *puwHeight  = tr_auwHeightMap;
  for(INDEX ipix=0;ipix<iSize;ipix++) {
    *pacolImage = 0x00000000;
    if(bUse16b) {
      UWORD *puwData = (UWORD*)&pacolImage[0];
      *puwData = *puwHeight;
    } else {
      UBYTE *pubData = (UBYTE*)&pacolImage[0];
      UWORD *puwHData = puwHeight;
      *pubData = (UBYTE)(*puwHData>>8);
    }
    pacolImage++;
    puwHeight++;
  }

  iiHeightMap.SaveTGA_t(fnHeightMap);
  iiHeightMap.Clear();
}

// Rebuild all terrain
void CTerrain::ReBuildTerrain(BOOL bDelayTileRegen/*=FALSE*/)
{
  _ptrTerrain = this;

  ClearTopMaps();
  ClearTiles();
  ClearArrays();
  ClearQuadTree();

  // Make sure terrain is same size (in metars)
  SetTerrainSize(tr_vTerrainSize);
  // Build terrain data
  BuildTerrainData();
  // Build terrain quadtree
  BuildQuadTree();
  // Generate global top map
  GenerateTerrainTopMap();
  // Clear current regen list
  ClearRegenList();
  // Add all tiles to reqen queue
  AddAllTilesToRegenQueue();
  
  // if not delaying tile regen
  if(!bDelayTileRegen) {
    // Regenerate tiles now
    ReGenerate();
    // Update shadow map
    UpdateShadowMap();
  }
}

// Refresh terrain
void CTerrain::RefreshTerrain(void)
{
  ReBuildTerrain();
}

// Set terrain size
void CTerrain::SetTerrainSize(FLOAT3D vSize)
{
  tr_vStretch(1) = vSize(1) / (tr_pixHeightMapWidth-1);
  tr_vStretch(2) = vSize(2) / 65535.0f;
  tr_vStretch(3) = vSize(3) / (tr_pixHeightMapHeight-1);
  // remember new size
  tr_vTerrainSize = vSize;
}

template <class Type>
static void CropMap(INDEX iNewWidth, INDEX iNewHeight, INDEX iOldWidth, INDEX iOldHeight, Type *pNewData, Type *pOldData)
{
  INDEX iWidth  = Min(iOldWidth,iNewWidth);
  INDEX iHeight = Min(iOldHeight,iNewHeight);
  INDEX iNewStepX = ClampDn(iNewWidth-iOldWidth, (INDEX)0);
  INDEX iOldStepX = ClampDn(iOldWidth-iNewWidth, (INDEX)0);

  INDEX iNew = 0;
  INDEX iOld = 0;
  for(INDEX iy=0;iy<iHeight;iy++) {
    for(INDEX ix=0;ix<iWidth;ix++) {
      pNewData[iNew] = pOldData[iOld];
      iNew++;
      iOld++;
    }
    iNew += iNewStepX;
    iOld += iOldStepX;
  }
}

template <class Type>
static void StretchMap(INDEX iNewWidth, INDEX iNewHeight, INDEX iOldWidth, INDEX iOldHeight, Type *pNewData, Type *pOldData)
{
  //int a=0;
  CropMap(iNewWidth,iNewHeight,iOldWidth,iOldHeight,pNewData,pOldData);
}

template <class Type>
static void ShrinkMap(INDEX iNewWidth, INDEX iNewHeight, INDEX iOldWidth, INDEX iOldHeight, Type *pNewData, Type *pOldData)
{
  //FLOAT fWidth  = iNewWidth;
  //FLOAT fHeight = iNewHeight;
  FLOAT fDiffX = (FLOAT)iNewWidth  / iOldWidth;
  FLOAT fDiffY = (FLOAT)iNewHeight / iOldHeight;

  ULONG *pulNewData = (ULONG*)AllocMemory(iNewWidth * iNewHeight * sizeof(ULONG));
  memset(pulNewData,0,iNewWidth * iNewHeight * sizeof(ULONG));

  INDEX iOldPix = 0;
  for(FLOAT fy=0;fy<iNewHeight;fy+=fDiffY) {
    for(FLOAT fx=0;fx<iNewWidth;fx+=fDiffX) {
      INDEX iNewPix = floor(fx) + floor(fy) * iNewWidth;
      pulNewData[iNewPix] += pOldData[iOldPix];
      iOldPix++;
    }
  }

  ULONG ulDiv = ceil(1.0f/fDiffX) * ceil(1.0f/fDiffY);
  for(INDEX ii=0;ii<iNewWidth*iNewHeight;ii++) {
    pNewData[ii] = pulNewData[ii] / ulDiv;
  }

  FreeMemory(pulNewData);
  ASSERT(_CrtCheckMemory());
}

template <class Type>
static void ResizeMap(INDEX iNewWidth, INDEX iNewHeight, INDEX iOldWidth, INDEX iOldHeight, Type *pNewData, Type *pOldData)
{
  CropMap(iNewWidth,iNewHeight,iOldWidth,iOldHeight,pNewData,pOldData);
  /*
  if(iNewWidth>=iOldWidth && iNewHeight>=iOldHeight) {
    StretchMap(iNewWidth,iNewHeight,iOldWidth,iOldHeight,pNewData,pOldData);
  } else if(iNewWidth<=iOldWidth && iNewHeight<=iOldHeight) {
    ShrinkMap(iNewWidth,iNewHeight,iOldWidth,iOldHeight,pNewData,pOldData);
  } else {
    INDEX iTempWidth  = Max(iNewWidth ,iOldWidth);
    INDEX iTempHeight = Max(iNewHeight,iOldHeight);
    INDEX iTempSize   = iTempWidth*iTempHeight*sizeof(Type);
    Type *pTempData   = (Type*)AllocMemory(iTempSize);
    memset(pTempData,0,iTempSize);
    StretchMap(iTempWidth,iTempHeight,iOldWidth,iOldHeight,pTempData,pOldData);
    ShrinkMap(iNewWidth,iNewHeight,iTempWidth,iTempHeight,pNewData, pTempData);
    FreeMemory(pTempData);
  }
  */
}

void CTerrain::AllocateHeightMap(PIX pixWidth, PIX pixHeight)
{
  ASSERT(tr_auwHeightMap==NULL);
  ASSERT(tr_aubEdgeMap==NULL);

  FLOAT fLogWidht  = Log2(pixWidth-1);
  FLOAT fLogHeight = Log2(pixHeight-1);
  if(fLogWidht!=INDEX(fLogWidht) || fLogHeight!=INDEX(fLogHeight)) {
    ASSERTALWAYS("Invalid terrain width or height");
    return;
  }
  if(pixWidth != pixHeight) {
    ASSERTALWAYS("Only terrains with same width and height are supported in this version");
    return;
  }

  INDEX iSize = pixWidth * pixHeight * sizeof(UBYTE);

  // Allocate memory for maps
  tr_auwHeightMap  = (UWORD*)AllocMemory(iSize*2);
  tr_aubEdgeMap    = (UBYTE*)AllocMemory(iSize);
  memset(tr_auwHeightMap,0,iSize*2);
  memset(tr_aubEdgeMap,255,iSize);

  tr_pixHeightMapWidth  = pixWidth;
  tr_pixHeightMapHeight = pixHeight;

  // Update shadow map size cos it depends on size of height map
  SetShadowMapsSize(tr_iShadowMapSizeAspect,tr_iShadingMapSizeAspect);
}

void CTerrain::ReAllocateHeightMap(PIX pixWidth, PIX pixHeight)
{
  ASSERT(tr_auwHeightMap!=NULL);
  ASSERT(tr_aubEdgeMap!=NULL);

  FLOAT fLogWidht  = Log2(pixWidth-1);
  FLOAT fLogHeight = Log2(pixHeight-1);
  if(fLogWidht!=INDEX(fLogWidht) || fLogHeight!=INDEX(fLogHeight)) {
    ASSERTALWAYS("Invalid terrain width or height");
    return;
  }
  if(pixWidth != pixHeight) {
    ASSERTALWAYS("Only terrains with same width and height are supported in this version");
    return;
  }

  INDEX iSize = pixWidth * pixHeight * sizeof(UBYTE);

  // Allocate memory for maps
  UWORD *auwHeightMap = (UWORD*)AllocMemory(iSize*2);
  UBYTE *aubEdgeMap   = (UBYTE*)AllocMemory(iSize);

  // Resize height map
  memset(auwHeightMap,0,iSize*2);
  ResizeMap(pixWidth,pixHeight,tr_pixHeightMapWidth,tr_pixHeightMapHeight,auwHeightMap,tr_auwHeightMap);

  // Resize edge map
  memset(aubEdgeMap,255,iSize);
  ResizeMap(pixWidth,pixHeight,tr_pixHeightMapWidth,tr_pixHeightMapHeight,aubEdgeMap,tr_aubEdgeMap);

  // for each layer
  INDEX cttl = tr_atlLayers.Count();
  for(INDEX itl=0;itl<cttl;itl++) {
    CTerrainLayer &tl = tr_atlLayers[itl];
    // Allocate memory for layer mask
    UBYTE *aubLayerMask = (UBYTE*)AllocMemory(iSize);
    memset(aubLayerMask,0,iSize);
    ASSERT(tl.tl_iMaskWidth  == tr_pixHeightMapWidth);
    ASSERT(tl.tl_iMaskHeight == tr_pixHeightMapHeight);
    // resize layer
    ResizeMap(pixWidth,pixHeight,tl.tl_iMaskWidth,tl.tl_iMaskHeight,aubLayerMask,tl.tl_aubColors);
    // Free old mask
    FreeMemory(tl.tl_aubColors);
    // Apply changes
    tl.tl_aubColors = aubLayerMask;
    tl.tl_iMaskWidth  = pixWidth;
    tl.tl_iMaskHeight = pixHeight;
    // if this is first layer 
    if(itl==0) {
      // fill it
      tl.ResetLayerMask(255);
    }
  }

  // Free old maps
  FreeMemory(tr_auwHeightMap);
  FreeMemory(tr_aubEdgeMap);

  // Apply changes
  tr_auwHeightMap = auwHeightMap;
  tr_aubEdgeMap   = aubEdgeMap;
  tr_pixHeightMapWidth  = pixWidth;
  tr_pixHeightMapHeight = pixHeight;

  ASSERT(_CrtCheckMemory());

  // Update shadow map size cos it depends on size of height map
  SetShadowMapsSize(tr_iShadowMapSizeAspect,tr_iShadingMapSizeAspect);
  /*
  // Clear current maps if they exists
  ClearHeightMap();
  ClearEdgeMap();
  ClearLayers();
  */
/*
  ASSERT(tr_auwHeightMap==NULL);
  ASSERT(tr_aubEdgeMap==NULL);

  INDEX iSize = sizeof(UBYTE)*pixWidth*pixHeight;
  // Allocate memory for heightmap
  tr_auwHeightMap = (UWORD*)AllocMemory(iSize*2);
  // Allocate memory for edge map
  tr_aubEdgeMap = (UBYTE*)AllocMemory(iSize);

  // Reset height map to 0
  memset(tr_auwHeightMap,0,iSize*2);
  // Reset edge map to 255
  memset(tr_aubEdgeMap,255,iSize);

  tr_pixHeightMapWidth  = pixWidth;
  tr_pixHeightMapHeight = pixHeight;
*/
}

// Set shadow map size aspect (relative to height map size) and shading map aspect (relative to shadow map size)
void CTerrain::SetShadowMapsSize(INDEX iShadowMapAspect, INDEX iShadingMapAspect)
{
  // TEMP
  //#pragma message(">> Clamp dn SetShadowMapsSize")

  if(iShadingMapAspect<0) {
    iShadingMapAspect = 0;
  }
  ASSERT(iShadingMapAspect>=0);


  tr_iShadowMapSizeAspect  = iShadowMapAspect;
  tr_iShadingMapSizeAspect = iShadingMapAspect;

  if(GetShadowMapWidth()<32 || GetShadingMapHeight()<32) {
    tr_iShadowMapSizeAspect = -(FastLog2(tr_pixHeightMapWidth-1)-5);
  }

  if(GetShadingMapWidth()<32 || GetShadingMapHeight()<32) {
    tr_iShadingMapSizeAspect = 0;
  }

  PIX pixShadowMapWidth   = GetShadowMapWidth();
  PIX pixShadowMapHeight  = GetShadowMapHeight();

  PIX pixShadingMapWidth  = GetShadingMapWidth();
  PIX pixShadingMapHeight = GetShadingMapHeight();


  // Clear current shadow map
  ClearShadowMap();
 
  ULONG ulShadowMapFlags = 0;
  // if current app is world editor app
  if(_bWorldEditorApp) {
    // force texture to be static
    ulShadowMapFlags = TEX_STATIC;
  }

  // Create new shadow map texture
  ASSERT(tr_tdShadowMap.td_pulFrames==NULL);
  CreateTexture(tr_tdShadowMap,pixShadowMapWidth,pixShadowMapHeight,ulShadowMapFlags);
  // Reset shadow map texture
  memset(&tr_tdShadowMap.td_pulFrames[0],0,sizeof(COLOR)*pixShadowMapWidth*pixShadowMapHeight);

  // Create new shading map
  ASSERT(tr_auwShadingMap==NULL);
  tr_auwShadingMap = (UWORD*)AllocMemory(pixShadingMapWidth*pixShadingMapHeight*sizeof(UWORD));
  // Reset shading map
  memset(&tr_auwShadingMap[0],0,pixShadingMapWidth*pixShadingMapHeight*sizeof(UWORD));
}

// Set size of terrain top map texture
void CTerrain::SetGlobalTopMapSize(PIX pixTopMapSize)
{
  FLOAT fLogSize = Log2(pixTopMapSize);
  if(fLogSize!=INDEX(fLogSize)) {
    ASSERTALWAYS("Invalid top map size");
    return;
  }

  tr_pixTopMapWidth  = pixTopMapSize;
  tr_pixTopMapHeight = pixTopMapSize;
}

// Set size of top map texture for tiles in lower lods
void CTerrain::SetTileTopMapSize(PIX pixLodTopMapSize)
{
  FLOAT fLogSize = Log2(pixLodTopMapSize);
  if(fLogSize!=INDEX(fLogSize)) {
    ASSERTALWAYS("Invalid top map size");
    return;
  }

  tr_pixFirstMipTopMapWidth  = pixLodTopMapSize;
  tr_pixFirstMipTopMapHeight = pixLodTopMapSize;
}

// Set lod distance factor
void CTerrain::SetLodDistanceFactor(FLOAT fLodDistance)
{
  tr_fDistFactor = ClampDn(fLodDistance,0.1f);
}

// Get shadow map size
PIX CTerrain::GetShadowMapWidth(void)
{
  if(tr_iShadowMapSizeAspect<0) {
    return (tr_pixHeightMapWidth-1)>>-tr_iShadowMapSizeAspect;
  } else {
    return (tr_pixHeightMapWidth-1)<<tr_iShadowMapSizeAspect;
  }
}
PIX CTerrain::GetShadowMapHeight(void)
{
  if(tr_iShadowMapSizeAspect<0) {
    return (tr_pixHeightMapHeight-1)>>-tr_iShadowMapSizeAspect;
  } else {
    return (tr_pixHeightMapHeight-1)<<tr_iShadowMapSizeAspect;
  }
}

// Get shading map size
PIX CTerrain::GetShadingMapWidth(void)
{
  ASSERT(tr_iShadingMapSizeAspect>=0);
  return GetShadowMapWidth()>>tr_iShadingMapSizeAspect;
}
PIX CTerrain::GetShadingMapHeight(void)
{
  ASSERT(tr_iShadingMapSizeAspect>=0);
  return GetShadowMapHeight()>>tr_iShadingMapSizeAspect;
}

// Get reference to layer
CTerrainLayer &CTerrain::GetLayer(INDEX iLayer)
{
  INDEX cttl = tr_atlLayers.Count();

  ASSERT(iLayer<cttl);
  ASSERT(iLayer>=0);
  return tr_atlLayers[iLayer];
}

// Add new layer
CTerrainLayer &CTerrain::AddLayer_t(CTFileName fnTexture, LayerType ltType/*=LT_NORMAL*/, BOOL bUpdateTerrain/*=TRUE*/)
{
  CTerrainLayer &tl = tr_atlLayers.Push();

  // Set layer properties
  tl.tl_ltType = ltType;
  tl.SetLayerSize(tr_pixHeightMapWidth, tr_pixHeightMapHeight);
  tl.SetLayerTexture_t(fnTexture);

  // if update terrain flag has been set
  if(bUpdateTerrain) {
    // Refresh whole terrain
    RefreshTerrain();
  }
  return tl;
}

// Remove one layer
void CTerrain::RemoveLayer(INDEX iLayer, BOOL bUpdateTerrain/*=TRUE*/)
{
  CStaticStackArray<class CTerrainLayer> atlLayers;
  INDEX cttl = tr_atlLayers.Count();

  if(iLayer<0 || iLayer>=cttl) {
    ASSERTALWAYS("Invalid layer index");
    return;
  }

  if(iLayer==0 && cttl==1) {
    ASSERTALWAYS("Can't remove last layer");
    return;
  }

  // for each exisiting layer
  for(INDEX itl=0;itl<cttl;itl++) {
    // if this layer index is not same as index of layer that need to be removed
    if(itl!=iLayer) {
      // Add new layer
      CTerrainLayer &tl = atlLayers.Push();
      // Copy this layer into new array
      tl = tr_atlLayers[itl];
    }
  }

  ASSERT(atlLayers.Count() == cttl-1);
  // Clear old layers
  tr_atlLayers.Clear();
  // Copy new layers insted of old one
  tr_atlLayers = atlLayers;

  // if update terrain flag has been set
  if(bUpdateTerrain) {
    // Refresh whole terrain
    RefreshTerrain();
  }
}

// Move layer to new position
INDEX CTerrain::SetLayerIndex(INDEX iLayer, INDEX iNewIndex, BOOL bUpdateTerrain/*=TRUE*/)
{
  CStaticStackArray<class CTerrainLayer> atlLayers;
  INDEX cttl = tr_atlLayers.Count();

  if(iLayer<0 || iLayer>=cttl) {
    ASSERTALWAYS("Invalid layer index");
    return iLayer;
  }

  if(iLayer==0 && cttl==1) {
    ASSERTALWAYS("Can't move only layer");
    return iLayer;
  }

  if(iLayer==iNewIndex) {
    ASSERTALWAYS("Old layer index is same as new one");
    return iLayer;
  }


  CStaticStackArray<class CTerrainLayer> &atlFrom = tr_atlLayers;
  CStaticStackArray<class CTerrainLayer> &atlTo = atlLayers;

  atlTo.Push(cttl);

  INDEX iOld = iLayer;
  INDEX iNew = iNewIndex;

  for(INDEX iFrom=0; iFrom<cttl; iFrom++) {
    INDEX iTo=-1;
    if (iNew==iOld) {
      iTo = iFrom;
    } if ((iFrom<iOld && iFrom<iNew) || (iFrom>iOld && iFrom>iNew)) {
      iTo = iFrom;
    } else if (iFrom==iOld) {
      iTo = iNew;
    } else {
      if (iNew>iOld) {
        iTo = iFrom-1;
      } else {
        iTo = iFrom+1;
      }
    }
    atlTo[iTo] = atlFrom[iFrom];
  }

  ASSERT(atlLayers.Count() == cttl);
  // Clear old layers
  tr_atlLayers.Clear();
  // Copy new layers insted of old one
  tr_atlLayers = atlLayers;

  // if update terrain flag has been set
  if(bUpdateTerrain) {
    // Refresh whole terrain
    RefreshTerrain();
  }
  return iNewIndex;
}

// Add tile to reqen queue
void CTerrain::AddTileToRegenQueue(INDEX iTileIndex)
{
  INDEX &iRegenIndex = tr_auiRegenList.Push();
  CTerrainTile &tt = tr_attTiles[iTileIndex];

  iRegenIndex = iTileIndex;
  tt.AddFlag(TT_REGENERATE);
}

// Add all tiles to regen queue
void CTerrain::AddAllTilesToRegenQueue()
{
  // for each terrain tile
  for(INDEX itt=0;itt<tr_ctTiles;itt++) {
    // Add tile to reqen queue
    //CTerrainTile &tt = tr_attTiles[itt];
    AddTileToRegenQueue(itt);
  }
}

// Clear current regen list
void CTerrain::ClearRegenList(void)
{
  tr_auiRegenList.PopAll();
}

void CTerrain::UpdateShadowMap(FLOATaabbox3D *pbboxUpdate/*=NULL*/, BOOL bAbsoluteSpace/*=FALSE*/)
{
  // if this is world editor app
  if(!_bWorldEditorApp) {
    // Shadow map can only be updated from world editor
    return;
  }
  // if shadow update is allowed
  if(_wrpWorldRenderPrefs.GetShadowsType()==CWorldRenderPrefs::SHT_FULL) {
    // update terrain shadow map
    UpdateTerrainShadowMap(this,pbboxUpdate,bAbsoluteSpace);
    // don't update shadow map next frame
    RemoveFlag(TR_UPDATE_SHADOWMAP);
  // else 
  } else {
    // dont update shadow map but remeber that it's not up to date
    AddFlag(TR_UPDATE_SHADOWMAP);
  }
}

// Temp:
__forceinline void CopyPixel(COLOR *pubSrc,COLOR *pubDst,FLOAT fMaskStrength)
{
  GFXColor *pcolSrc = (GFXColor*)pubSrc;
  GFXColor *pcolDst = (GFXColor*)pubDst;
  pcolSrc->ub.r = Lerp(pcolSrc->ub.r, pcolDst->ub.r, fMaskStrength);
  pcolSrc->ub.g = Lerp(pcolSrc->ub.g, pcolDst->ub.g, fMaskStrength);
  pcolSrc->ub.b = Lerp(pcolSrc->ub.b, pcolDst->ub.b, fMaskStrength);
  pcolSrc->ub.a = 255;
}

#if 0 // DG: unused.
static INDEX _ctSavedTopMaps=0;
static void SaveAsTga(CTextureData *ptdTex)
{
  INDEX iSize = ptdTex->td_mexWidth * ptdTex->td_mexHeight * 4;
  CImageInfo iiHeightMap;
  iiHeightMap.ii_Width  = ptdTex->td_mexWidth;
  iiHeightMap.ii_Height = ptdTex->td_mexHeight;
  iiHeightMap.ii_BitsPerPixel = 32;
  iiHeightMap.ii_Picture = (UBYTE*)AllocMemory(iSize);

  memcpy(&iiHeightMap.ii_Picture[0],&ptdTex->td_pulFrames[0],iSize);

  
  CTString strTopMap = CTString(0,"Temp\\Topmap%d.tga",++_ctSavedTopMaps);
  iiHeightMap.SaveTGA_t(strTopMap);
  iiHeightMap.Clear();


  /*
  GFXColor *pacolImage = (GFXColor*)&iiHeightMap.ii_Picture[0];
  UWORD    *puwHeight  = tr_auwHeightMap;
  for(INDEX ipix=0;ipix<iSize;ipix++) {
    *pacolImage = 0x00000000;
    if(bUse16b) {
      UWORD *puwData = (UWORD*)&pacolImage[0];
      *puwData = *puwHeight;
    } else {
      UBYTE *pubData = (UBYTE*)&pacolImage[0];
      UWORD *puwHData = puwHeight;
      *pubData = (UBYTE)(*puwHData>>8);
    }
    pacolImage++;
    puwHeight++;
  }
  */

}
#endif // 0

static void AddTileLayerToTopMap(CTerrain *ptrTerrain, INDEX iTileIndex, INDEX iLayer)
{
  CTerrainLayer &tl = ptrTerrain->tr_atlLayers[iLayer];
  CTextureData *ptdSrc = tl.tl_ptdTexture;
  CTextureData *ptdDst;

  INDEX ctQuadsPerTile = ptrTerrain->tr_ctQuadsInTileRow;
  INDEX iOffsetX = 0;
  INDEX iOffsetZ = 0;

  if(iTileIndex==(-1)) {
    ptdDst = &ptrTerrain->tr_tdTopMap;
    ctQuadsPerTile = ptrTerrain->tr_ctTilesX * ptrTerrain->tr_ctQuadsInTileRow;
  } else {
    CTerrainTile  &tt = ptrTerrain->tr_attTiles[iTileIndex];
    ptdDst = tt.GetTopMap();
    iOffsetX = tt.tt_iOffsetX*ctQuadsPerTile;
    iOffsetZ = tt.tt_iOffsetZ*ctQuadsPerTile;
  }

  ULONG *pulFirstInTopMap = ptdDst->td_pulFrames;
  UBYTE *pubFirstInLayerMask = tl.tl_aubColors;

  // Calculate width and height of quad that will be draw in top map
  PIX pixDstQuadWidth = ptdDst->GetPixWidth() / ctQuadsPerTile;
  PIX pixSrcQuadWidth = tl.tl_pixTileWidth;

  // if dst quad is smaller then one pixel
  if(pixDstQuadWidth==0) {
    return; 
  }

  ASSERT(tl.tl_ctTilesInRow==tl.tl_ctTilesInCol);

  INDEX iSrcMipmap = FastLog2((ptdSrc->GetPixWidth() / tl.tl_ctTilesInRow) / pixDstQuadWidth);
  INDEX iSrcMipMapOffset = GetMipmapOffset(iSrcMipmap,ptdSrc->GetPixWidth(),ptdSrc->GetPixHeight());
  INDEX iSrcMipWidth = ptdSrc->GetPixWidth() >> iSrcMipmap;
  INDEX iSrcMipQuadWidth = pixSrcQuadWidth >> iSrcMipmap;
  INDEX iSrcMipQuadHeight = iSrcMipQuadWidth;

  ASSERT(pixDstQuadWidth==iSrcMipQuadWidth);
   
  ULONG *pulSrcMip = &ptdSrc->td_pulFrames[iSrcMipMapOffset];

  INDEX iMaskIndex = iOffsetX + iOffsetZ*ptrTerrain->tr_pixHeightMapWidth;
  INDEX iMaskStepX = ptrTerrain->tr_pixHeightMapWidth - ctQuadsPerTile;

  // for each quad in tile
  for(INDEX iQuadY=0;iQuadY<ctQuadsPerTile;iQuadY++) {
    for(INDEX iQuadX=0;iQuadX<ctQuadsPerTile;iQuadX++) {

      UBYTE ubMask = pubFirstInLayerMask[iMaskIndex];
      BOOL  bFlipX   = (ubMask&TL_FLIPX)>>TL_FLIPX_SHIFT;
      BOOL  bFlipY   = (ubMask&TL_FLIPY)>>TL_FLIPY_SHIFT;
      BOOL  bSwapXY  = (ubMask&TL_SWAPXY)>>TL_SWAPXY_SHIFT;
      BOOL  bVisible = (ubMask&TL_VISIBLE)>>TL_VISIBLE_SHIFT;
      INDEX iTile = ubMask&TL_TILE_INDEX;
      INDEX iTileX = iTile%tl.tl_ctTilesInRow;
      INDEX iTileY = iTile/tl.tl_ctTilesInRow;

      // if not visible
      if(!bVisible) {
        iMaskIndex++;
        continue; // skip it
      }

      ASSERT(iTileX<tl.tl_ctTilesInRow);
      ASSERT(iTileY<tl.tl_ctTilesInCol);

      INDEX iFirstDstQuadPixel = (iQuadX*pixDstQuadWidth) + iQuadY*pixDstQuadWidth*ptdDst->GetPixWidth();
      ULONG *pulDstPixel = &pulFirstInTopMap[iFirstDstQuadPixel];
      PIX pixSrc = iTileX*iSrcMipQuadWidth + iTileY*iSrcMipQuadWidth*iSrcMipWidth;
      PIX pixDst = 0;
      PIX pixDstModulo = ptdDst->GetPixWidth() - pixDstQuadWidth;
      PIX pixSrcStepY = iSrcMipWidth;
      PIX pixSrcStepX = 1;

      if(bFlipY) {
        pixSrcStepY = -pixSrcStepY; 
        pixSrc+=iSrcMipQuadHeight*iSrcMipWidth;
      }
      if(bFlipX) {
        pixSrcStepX = -pixSrcStepX;
        pixSrc+=iSrcMipQuadWidth;
      }

      if(bSwapXY) {
        Swap(pixSrcStepX, pixSrcStepY);
      }

      pixSrcStepY -= pixDstQuadWidth*pixSrcStepX;

      // for each pixel in this quad
      for(PIX pixY=0;pixY<pixDstQuadWidth;pixY++) {
        for(PIX pixX=0;pixX<pixDstQuadWidth;pixX++) {
          if((pulSrcMip[pixSrc]&0xFF000000) > 0x80000000) {
            pulDstPixel[pixDst] = pulSrcMip[pixSrc];
          }
          pixSrc+=pixSrcStepX;
          pixDst++;
        }
        pixDst+=pixDstModulo;
        pixSrc+=pixSrcStepY;
      }
      iMaskIndex++;
    }
    iMaskIndex+=iMaskStepX;
  }
}

void CTerrain::UpdateTopMap(INDEX iTileIndex, Rect *prcDest/*=NULL*/)
{
  //ReGenerateTopMap(this, iTileIndex);
  
  if(iTileIndex==(-1)) {
    ctGlobalTopMaps++;
  } else {
    ctGeneratedTopMaps++;
  }

  INDEX _fiMaskDiv = 1;
  FIX16_16 fiMaskDiv;
  fiMaskDiv = (FIX16_16)_fiMaskDiv;

  INDEX iFirstInMask = 0;
  INDEX iMaskWidth = tr_pixHeightMapWidth;
  INDEX iTiling = 1;
  //INDEX iSrcMipWidth = 1;
  

  // destionation texture (must have set allocated memory)
  CTextureData *ptdDest;

  // if global top map
  if(iTileIndex==(-1)) {
    ptdDest = &tr_tdTopMap;
  // else tile top map
  } else {
    CTerrainTile &tt = tr_attTiles[iTileIndex];
    ptdDest = tt.GetTopMap();
    fiMaskDiv = tr_ctTilesX;
    iFirstInMask = iMaskWidth * tt.tt_iOffsetZ * (tr_ctVerticesInTileRow-1) + (tt.tt_iOffsetX * (tr_ctVerticesInTileRow-1));
  }

  ASSERT(ptdDest->td_pulFrames==NULL);
  PrepareSharedTopMapMemory(ptdDest, iTileIndex);

  ASSERT(ptdDest!=NULL);
  ASSERT(ptdDest->td_pulFrames!=NULL);
 
  // ASSERT(ptdDest->GetPixWidth()>0 && ptdDest->GetPixHeight()>0 && ptdDest->GetPixWidth()==ptdDest->GetPixHeight());
  // iTiling = ClampDn(iTiling,(INDEX)1);
  // INDEX iSrcMipWidth = ClampDn(ptdDest->GetWidth()/iTiling,(INDEX)1);


  INDEX ctLayers = tr_atlLayers.Count();
  // for each layer
  for(INDEX itl=0;itl<ctLayers;itl++) {
    CTerrainLayer &tl = tr_atlLayers[itl];
    // if layer isn't visible
    if(!tl.tl_bVisible) {
      // skip it
      continue;
    }
    if(tl.tl_ltType==LT_TILE) {
      AddTileLayerToTopMap(this,iTileIndex,itl);
      continue;
    }

    if(iTileIndex==(-1)) {
      // ptdDest = &tr_tdTopMap;
      iTiling = (INDEX)(tr_ctTilesX*tr_ctQuadsInTileRow*tl.tl_fStretchX);

    // else tile top map
    } else {
      CTerrainTile &tt = tr_attTiles[iTileIndex];
      // ptdDest = tt.GetTopMap();
      fiMaskDiv = tr_ctTilesX;
      iFirstInMask = iMaskWidth * tt.tt_iOffsetZ * (tr_ctVerticesInTileRow-1)
                   + (tt.tt_iOffsetX * (tr_ctVerticesInTileRow-1));
      iTiling = (INDEX)(tr_ctQuadsInTileRow*tl.tl_fStretchX);
    }

    ASSERT(ptdDest->GetPixWidth()>0 && ptdDest->GetPixHeight()>0 && ptdDest->GetPixWidth()==ptdDest->GetPixHeight());
    ASSERT(iTiling>=1);
    iTiling = ClampDn(iTiling,(INDEX)1);

    // get source texture
    CTextureData *ptdSrc = tl.tl_ptdTexture;
    INDEX iSrcMipWidth  = ClampDn( ptdSrc->GetPixWidth() /iTiling, (PIX)1);
    INDEX iSrcMipHeight = ClampDn( ptdSrc->GetPixHeight()/iTiling, (PIX)1);

    // Get mipmap of source texture
    INDEX immW = FastLog2( ptdSrc->GetPixWidth()  / iSrcMipWidth);
    INDEX immH = FastLog2( ptdSrc->GetPixHeight() / iSrcMipHeight);
    // get address of first byte in source mipmap
    INDEX imm = Max( immW, immH);
    INDEX iMipAdr = GetMipmapOffset(imm,ptdSrc->GetPixWidth(),ptdSrc->GetPixHeight());
 
    // Mask thing
    // get first byte in layer mask
    UBYTE *ubFirstInMask = &tl.tl_aubColors[iFirstInMask];
    // get first byte in edge map
    UBYTE *ubFirstInEdgeMap = &tr_aubEdgeMap[iFirstInMask];
    FIX16_16 fiHMaskStep = FIX16_16(iMaskWidth-1) / FIX16_16(ptdDest->GetWidth()-1) / fiMaskDiv;
    FIX16_16 fiVMaskStep = FIX16_16(iMaskWidth-1) / FIX16_16(ptdDest->GetWidth()-1) / fiMaskDiv;

    SLONG xHMaskStep = fiHMaskStep.slHolder;
    SLONG xVMaskStep = fiVMaskStep.slHolder;
    SLONG xMaskVPos=0;
    
    // get first byte in destination texture
    ULONG *pulTexDst = (ULONG*)&ptdDest->td_pulFrames[0];
    // get first byte in source texture
    ULONG *pulFirstInMipSrc = (ULONG*)&ptdSrc->td_pulFrames[iMipAdr];
  
    // for each row
    for(UINT ir = 0; ir < static_cast<UINT>(ptdDest->GetPixHeight()); ir++)
    {
      // get first byte for src mip texture in this row
      ULONG *pulSrcRow = &pulFirstInMipSrc[(ir&(iSrcMipWidth-1))*iSrcMipWidth];//%
      INDEX iMaskVPos = (INDEX)(xMaskVPos>>16) * (iMaskWidth);
      UBYTE *pubMaskRow = &ubFirstInMask[iMaskVPos];
      UBYTE *pubEdgeMaskRow = &ubFirstInEdgeMap[iMaskVPos];
      SLONG xMaskHPos = 0;
      // for each column
      for(UINT ic = 0; ic < static_cast<UINT>(ptdDest->GetPixWidth()); ic++)
      {
        ULONG *ulSrc = &pulSrcRow[ic&(iSrcMipWidth-1)];
        INDEX iMask = (INDEX)(xMaskHPos>>16);

        SLONG x1 = (SLONG)(pubMaskRow[iMask+0]) <<0; //NormByteToFixInt(pubMaskRow[iMask]);
        SLONG x2 = (SLONG)(pubMaskRow[iMask+1]) <<0; //NormByteToFixInt(pubMaskRow[iMask+1]);
        SLONG x3 = (SLONG)(pubMaskRow[iMask+iMaskWidth+0]) <<0;//NormByteToFixInt(pubMaskRow[iMask+iMaskWidth+0]);
        SLONG x4 = (SLONG)(pubMaskRow[iMask+iMaskWidth+1]) <<0;//NormByteToFixInt(pubMaskRow[iMask+iMaskWidth+1]);
        SLONG xFactH = xMaskHPos - (xMaskHPos&0xFFFF0000);
        SLONG xFactV = xMaskVPos - (xMaskVPos&0xFFFF0000);
        
        SLONG xStrengthX1 = (x1<<7) + (SLONG)(((x2-x1)*xFactH)>>9); //Lerp(fi1,fi2,fiFactH);
        SLONG xStrengthX2 = (x3<<7) + (SLONG)(((x4-x3)*xFactH)>>9); //Lerp(fi3,fi4,fiFactH);
        SLONG xStrength   = (xStrengthX1<<1) + (SLONG)((((xStrengthX2>>0)-(xStrengthX1>>0))*xFactV)>>15);   //Lerp(fiStrengthX1,fiStrengthX2,fiFactV);
        
        GFXColor *pcolSrc = (GFXColor*)pulTexDst;
        GFXColor *pcolDst = (GFXColor*)ulSrc;
		pcolSrc->ub.r = (BYTE)((ULONG)pcolSrc->ub.r + ((((ULONG)pcolDst->ub.r - (ULONG)pcolSrc->ub.r) * xStrength) >> 16));
		pcolSrc->ub.g = (BYTE)((ULONG)pcolSrc->ub.g + ((((ULONG)pcolDst->ub.g - (ULONG)pcolSrc->ub.g) * xStrength) >> 16));
		pcolSrc->ub.b = (BYTE)((ULONG)pcolSrc->ub.b + ((((ULONG)pcolDst->ub.b - (ULONG)pcolSrc->ub.b) * xStrength) >> 16));
		pcolSrc->ub.a = pubEdgeMaskRow[iMask];        
        pulTexDst++;
        xMaskHPos += xHMaskStep;
      }
      xMaskVPos += xVMaskStep;
    }
  }
  // make mipmaps
  INDEX ctMipMaps = GetNoOfMipmaps(ptdDest->GetPixWidth(),ptdDest->GetPixHeight());
  MakeMipmaps(ctMipMaps, ptdDest->td_pulFrames, ptdDest->GetPixWidth(), ptdDest->GetPixHeight());

  //#pragma message(">> Fix DitherMipmaps")
  INDEX iDithering = 4;
  DitherMipmaps(iDithering,ptdDest->td_pulFrames,ptdDest->td_pulFrames,ptdDest->GetPixWidth(),ptdDest->GetPixHeight());
  // force topmap upload
  ptdDest->SetAsCurrent(0,TRUE);

  // Free shared memory
  FreeSharedTopMapMemory(ptdDest, iTileIndex);
}

void CTerrain::GetAllTerrainBBox(FLOATaabbox3D &bbox)
{
  // Get last quad tree level
  INDEX ctqtl = tr_aqtlQuadTreeLevels.Count();
  QuadTreeLevel &qtl = tr_aqtlQuadTreeLevels[ctqtl-1];

  ASSERT(qtl.qtl_ctNodes==1);
  // Get quad tree node for last level
  QuadTreeNode  &qtn = tr_aqtnQuadTreeNodes[qtl.qtl_iFirstNode];
  bbox = qtn.qtn_aabbox;
}

// Get shading color from tex coords in shading map
COLOR CTerrain::GetShadeColor(CShadingInfo *psi)
{
  ASSERT(psi!=NULL);
  ASSERT(tr_auwShadingMap!=NULL);

  PIX pixShadowU = Clamp(psi->si_pixShadowU, (PIX)0,GetShadingMapWidth()-2);
  PIX pixShadowV = Clamp(psi->si_pixShadowV, (PIX)0,GetShadingMapHeight()-2);
  FLOAT fUDRatio = psi->si_fUDRatio;
  FLOAT fLRRatio = psi->si_fLRRatio;

  PIX pixWidth  = GetShadingMapWidth();
  PIX pixShadow = pixShadowU + pixShadowV*pixWidth;
  UWORD auwShade[4];
  SLONG aslr[4], aslg[4], aslb[4];

  auwShade[0] = tr_auwShadingMap[pixShadow];
  auwShade[1] = tr_auwShadingMap[pixShadow+1];
  auwShade[2] = tr_auwShadingMap[pixShadow+pixWidth+0];
  auwShade[3] = tr_auwShadingMap[pixShadow+pixWidth+1];

  for(INDEX ish=0;ish<4;ish++) {
    aslr[ish] = (auwShade[ish]&0x7C00)>>10;
    aslg[ish] = (auwShade[ish]&0x03E0)>> 5;
    aslb[ish] = (auwShade[ish]&0x001F)>> 0;

    aslr[ish] = (aslr[ish]<<3) | (aslr[ish]>>2);
    aslg[ish] = (aslg[ish]<<3) | (aslg[ish]>>2);
    aslb[ish] = (aslb[ish]<<3) | (aslb[ish]>>2);
  }

  SLONG slRed   = Lerp( Lerp(aslr[0], aslr[1], fLRRatio), Lerp(aslr[2], aslr[3], fLRRatio), fUDRatio);
  SLONG slGreen = Lerp( Lerp(aslg[0], aslg[1], fLRRatio), Lerp(aslg[2], aslg[3], fLRRatio), fUDRatio);
  SLONG slBlue  = Lerp( Lerp(aslb[0], aslb[1], fLRRatio), Lerp(aslb[2], aslb[3], fLRRatio), fUDRatio);

  ULONG ulPixel = ((slRed  <<24)&0xFF000000) |
                  ((slGreen<<16)&0x00FF0000) | 
                  ((slBlue << 8)&0x0000FF00) | 0xFF;

  return ulPixel;
}

// Get plane from given point
FLOATplane3D CTerrain::GetPlaneFromPoint(FLOAT3D &vAbsPoint)
{
  ASSERT(tr_penEntity!=NULL);
  FLOAT3D vRelPoint = (vAbsPoint-tr_penEntity->en_plPlacement.pl_PositionVector) * !tr_penEntity->en_mRotation;
  vRelPoint(1) /= tr_vStretch(1);
  vRelPoint(3) /= tr_vStretch(3);
  PIX pixX = (PIX) floor(vRelPoint(1));
  PIX pixZ = (PIX) floor(vRelPoint(3));
  PIX pixWidth = tr_pixHeightMapWidth;
  FLOAT fXRatio = vRelPoint(1) - pixX;
  FLOAT fZRatio = vRelPoint(3) - pixZ;

  INDEX iPix = pixX + pixZ*pixWidth;
  BOOL bFacing = (iPix)&1;

  FLOAT3D vx0 = FLOAT3D((pixX+0)*tr_vStretch(1),tr_auwHeightMap[iPix]           * tr_vStretch(2) ,(pixZ+0)*tr_vStretch(3));
  FLOAT3D vx1 = FLOAT3D((pixX+1)*tr_vStretch(1),tr_auwHeightMap[iPix+1]         * tr_vStretch(2) ,(pixZ+0)*tr_vStretch(3));
  FLOAT3D vx2 = FLOAT3D((pixX+0)*tr_vStretch(1),tr_auwHeightMap[iPix+pixWidth]  * tr_vStretch(2) ,(pixZ+1)*tr_vStretch(3));
  FLOAT3D vx3 = FLOAT3D((pixX+1)*tr_vStretch(1),tr_auwHeightMap[iPix+pixWidth+1]* tr_vStretch(2) ,(pixZ+1)*tr_vStretch(3));

  vx0 = vx0 * tr_penEntity->en_mRotation + tr_penEntity->en_plPlacement.pl_PositionVector;
  vx1 = vx1 * tr_penEntity->en_mRotation + tr_penEntity->en_plPlacement.pl_PositionVector;
  vx2 = vx2 * tr_penEntity->en_mRotation + tr_penEntity->en_plPlacement.pl_PositionVector;
  vx3 = vx3 * tr_penEntity->en_mRotation + tr_penEntity->en_plPlacement.pl_PositionVector;

  if(bFacing) {
    if(fXRatio>=fZRatio) {
      return FLOATplane3D(vx0,vx2,vx1);
    } else {
      return FLOATplane3D(vx1,vx2,vx3);
    }
  } else {
    if(fXRatio>=fZRatio) {
      return FLOATplane3D(vx2,vx3,vx0);
    } else {
      return FLOATplane3D(vx0,vx3,vx1);
    }
  }
}

// Sets number of quads in row of one tile
void CTerrain::SetQuadsPerTileRow(INDEX ctQuadsPerTileRow)
{
  tr_ctQuadsInTileRow = Clamp(ctQuadsPerTileRow,(INDEX)4,(INDEX)(tr_pixHeightMapWidth-1));
  if(tr_ctQuadsInTileRow!=ctQuadsPerTileRow) {
    CPrintF("Warning: Quads per tile has been changed from requested %d to %d\n",ctQuadsPerTileRow,tr_ctQuadsInTileRow);
  }
  // TODO: Assert that it is 2^n
  tr_ctVerticesInTileRow = tr_ctQuadsInTileRow+1;
}

// Set Terrain stretch
void CTerrain::SetTerrainStretch(FLOAT3D vStretch)
{
  tr_vStretch = vStretch;
}

// Build terrain data
void CTerrain::BuildTerrainData()
{
  // Allocate space for terrain tiles
  tr_ctTilesX = (tr_pixHeightMapWidth-1)  / tr_ctQuadsInTileRow;
  tr_ctTilesY = (tr_pixHeightMapHeight-1) / tr_ctQuadsInTileRow;
  tr_ctTiles  = tr_ctTilesX*tr_ctTilesY;
  tr_attTiles.New(tr_ctTiles);

  // Calculate max posible lod
  INDEX ctVtxInLod = tr_ctQuadsInTileRow;
  tr_iMaxTileLod = 0;
  while(ctVtxInLod>2) {
    tr_iMaxTileLod++;
    ctVtxInLod = ctVtxInLod>>1;
  }

  // Allocate memory for terrain tile arrays
  tr_aArrayHolders.New(tr_iMaxTileLod+1);
  INDEX ctah  = tr_aArrayHolders.Count();
  // for each array handler
  for(INDEX iah=0;iah<ctah;iah++) {
    CArrayHolder &ah = tr_aArrayHolders[iah];
    // set its lod index
    ah.ah_iLod = iah;
    ah.ah_ptrTerrain = this;
  }

  // for each tile row
  for(INDEX iy=0;iy<tr_ctTilesY;iy++) {
    // for each tile col
    for(INDEX ix=0;ix<tr_ctTilesX;ix++) {
      // Initialize terrain tile
      UINT iTileIndex = ix+iy*tr_ctTilesX;
      CTerrainTile &tt = tr_attTiles[iTileIndex];
      tt.tt_iIndex = iTileIndex;
      tt.tt_iOffsetX = ix;
      tt.tt_iOffsetZ = iy;
      tt.tt_ctVtxX   = tr_ctVerticesInTileRow;
      tt.tt_ctVtxY   = tr_ctVerticesInTileRow;
      tt.tt_ctLodVtxX = tt.tt_ctVtxX;
      tt.tt_ctLodVtxY = tt.tt_ctVtxY;
      tt.tt_iLod = 0;
      tt.tt_fLodLerpFactor = 0;
      // Reset tile neighbours
      tt.tt_aiNeighbours[NB_TOP]   =-1; tt.tt_aiNeighbours[NB_LEFT]  =-1;
      tt.tt_aiNeighbours[NB_BOTTOM]=-1; tt.tt_aiNeighbours[NB_RIGHT] =-1;
      // Set tile neighbours
      if(iy>0) tt.tt_aiNeighbours[NB_TOP]  = iTileIndex-tr_ctTilesX;
      if(ix>0) tt.tt_aiNeighbours[NB_LEFT] = iTileIndex-1;
      if(iy<tr_ctTilesX-1) tt.tt_aiNeighbours[NB_BOTTOM] = iTileIndex+tr_ctTilesX;
      if(ix<tr_ctTilesY-1) tt.tt_aiNeighbours[NB_RIGHT]  = iTileIndex+1;
    }
  }
}

/*
#if 1
void CTerrain::GenerateTopMap(INDEX iTileIndex)
{
  FIX16_16 fiMaskDiv = 1;
  INDEX iFirstInMask = 0;
  INDEX iMaskWidth = tr_pixHeightMapWidth;
  INDEX iTiling = (INDEX)(tr_ctTilesX*tr_ctQuadsInTileRow*tr_fTexStretch);

  // destionation texture (must have set allocated memory)
  CTextureData *ptdDest;
  // if global top map
  if(iTileIndex==(-1)) {
    ptdDest = &tr_tdTopMap;
  // else tile top map
  } else {
    CTerrainTile &tt = tr_attTiles[iTileIndex];
    ptdDest = tt.GetTopMap();
    fiMaskDiv = tr_ctTilesX;
    iFirstInMask = iMaskWidth * tt.tt_iOffsetZ * (tr_ctVerticesInTileRow-1) + (tt.tt_iOffsetX * (tr_ctVerticesInTileRow-1));
    iTiling = (INDEX)(tr_ctQuadsInTileRow*tr_fTexStretch);
  }
 
  ASSERT(ptdDest->GetPixWidth()>0 && ptdDest->GetPixHeight()>0 && ptdDest->GetPixWidth()==ptdDest->GetPixHeight());
  INDEX iSrcMipWidth = ClampDn(ptdDest->GetWidth()/iTiling,(INDEX)1);


  INDEX ctLayers = tr_atlLayers.Count();
  // for each layer
  for(INDEX itl=0;itl<ctLayers;itl++) {
    CTerrainLayer &tl = tr_atlLayers[itl];
    // get source texture
    CTextureData *ptdSrc = tl.tl_ptdTexture;
    // Get mipmap of source texture
    INDEX imm = FastLog2(ptdSrc->GetPixWidth()/iSrcMipWidth);
    // get address of first byte in source mipmap
    INDEX iMipAdr = GetMipmapOffset(imm,ptdSrc->GetPixWidth(),ptdSrc->GetPixHeight());
 
    // Mask thing
    // get first byte in layer mask
    UBYTE *ubFirstInMask = &tl.tl_aubColors[iFirstInMask];
    FIX16_16 fiHMaskStep = FIX16_16(iMaskWidth-1)/FIX16_16(ptdDest->GetWidth()-1)/fiMaskDiv;
    FIX16_16 fiVMaskStep = FIX16_16(iMaskWidth-1)/FIX16_16(ptdDest->GetWidth()-1)/fiMaskDiv;

    SLONG xHMaskStep = fiHMaskStep.slHolder;
    SLONG xVMaskStep = fiVMaskStep.slHolder;
    SLONG xMaskVPos=0;
    
    // get first byte in destination texture
    ULONG *pulTexDst = (ULONG*)&ptdDest->td_pulFrames[0];
    // get first byte in source texture
    ULONG *pulFirstInMipSrc = (ULONG*)&ptdSrc->td_pulFrames[iMipAdr];
  
    // for each row
    for(UINT ir=0;ir<ptdDest->GetHeight();ir++) {
      // get first byte for src mip texture in this row
      ULONG *pulSrcRow = &pulFirstInMipSrc[(ir&(iSrcMipWidth-1))*iSrcMipWidth];//%
      INDEX iMaskVPos = (INDEX)(xMaskVPos>>16) * (iMaskWidth);
      UBYTE *pubMaskRow = &ubFirstInMask[iMaskVPos];
      SLONG xMaskHPos = 0;
      // for each column
      for(UINT ic=0;ic<ptdDest->GetWidth();ic++) {

        ULONG *ulSrc = &pulSrcRow[ic&(iSrcMipWidth-1)];
        INDEX iMask = (INDEX)(xMaskHPos>>16);

        SLONG x1 = (SLONG)(pubMaskRow[iMask+0]) <<0; //NormByteToFixInt(pubMaskRow[iMask]);
        SLONG x2 = (SLONG)(pubMaskRow[iMask+1]) <<0; //NormByteToFixInt(pubMaskRow[iMask+1]);
        SLONG x3 = (SLONG)(pubMaskRow[iMask+iMaskWidth+0]) <<0;//NormByteToFixInt(pubMaskRow[iMask+iMaskWidth+0]);
        SLONG x4 = (SLONG)(pubMaskRow[iMask+iMaskWidth+1]) <<0;//NormByteToFixInt(pubMaskRow[iMask+iMaskWidth+1]);
        SLONG xFactH = xMaskHPos - (xMaskHPos&0xFFFF0000);
        SLONG xFactV = xMaskVPos - (xMaskVPos&0xFFFF0000);
        
        SLONG xStrengthX1 = (x1<<7) + (SLONG)(((x2-x1)*xFactH)>>9); //Lerp(fi1,fi2,fiFactH);
        SLONG xStrengthX2 = (x3<<7) + (SLONG)(((x4-x3)*xFactH)>>9); //Lerp(fi3,fi4,fiFactH);
        SLONG xStrength   = (xStrengthX1<<1) + (SLONG)((((xStrengthX2>>0)-(xStrengthX1>>0))*xFactV)>>15);   //Lerp(fiStrengthX1,fiStrengthX2,fiFactV);
        
        GFXColor *pcolSrc = (GFXColor*)pulTexDst;
        GFXColor *pcolDst = (GFXColor*)ulSrc;
        pcolSrc->r = (BYTE)( (ULONG)pcolSrc->r + ((((ULONG)pcolDst->r - (ULONG)pcolSrc->r) * xStrength)>>16));
        pcolSrc->g = (BYTE)( (ULONG)pcolSrc->g + ((((ULONG)pcolDst->g - (ULONG)pcolSrc->g) * xStrength)>>16));
        pcolSrc->b = (BYTE)( (ULONG)pcolSrc->b + ((((ULONG)pcolDst->b - (ULONG)pcolSrc->b) * xStrength)>>16));
        pcolSrc->a = 255;
        
        pulTexDst++;
        xMaskHPos += xHMaskStep;
      }
      xMaskVPos += xVMaskStep;
    }
  }
  // make mipmaps
  MakeMipmaps(32, ptdDest->td_pulFrames, ptdDest->GetWidth(), ptdDest->GetHeight());
  // force topmap upload
  ptdDest->SetAsCurrent(0,TRUE);
}
#else
void CTerrain::GenerateTopMap(INDEX iTileIndex)
{
  INDEX iMaskDiv = 1;
  INDEX iFirstInMask = 0;
  INDEX iMaskWidth = tr_pixHeightMapWidth;
  INDEX iTiling = (INDEX)(tr_ctTilesX*tr_ctQuadsInTileRow*tr_fTexStretch);

  // destionation texture (must have set required width and height)
  CTextureData *ptdDest;
  // if global top map
  if(iTileIndex==(-1)) {
    ptdDest = &tr_tdTopMap;
  // else tile top map
  } else {
    CTerrainTile &tt = tr_attTiles[iTileIndex];
    ptdDest = &tt.GetTopMap();
    iMaskDiv = tr_ctTilesX;
    iFirstInMask = iMaskWidth * tt.tt_iOffsetZ * (tr_ctVerticesInTileRow-1) + (tt.tt_iOffsetX * (tr_ctVerticesInTileRow-1));
    iTiling = (INDEX)(tr_ctQuadsInTileRow*tr_fTexStretch);
  }
 
  ASSERT(ptdDest->GetPixWidth()>0 && ptdDest->GetPixHeight()>0 && ptdDest->GetPixWidth()==ptdDest->GetPixHeight());

  INDEX iSrcMipWidth = ptdDest->GetWidth()/iTiling;

  INDEX ctLayers = tr_atlLayers.Count();
  // for each layer
  for(INDEX ilr=0;ilr<ctLayers;ilr++) {
    CTerrainLayer &tl = tr_atlLayers[ilr];
    // get source texture
    CTextureData *ptdSrc = tl.tl_ptdTexture;
    // Get mipmap of source texture
    INDEX imm = FastLog2(ptdSrc->GetPixWidth()/iSrcMipWidth);
    // get address of first byte in source mipmap
    INDEX iMipAdr = GetMipmapOffset(imm,ptdSrc->GetPixWidth(),ptdSrc->GetPixHeight());
 
    // Mask thing
    // get first byte in layer mask
    UBYTE *ubFirstInMask = &tl.tl_aubColors[iFirstInMask];
    FLOAT fHMaskStep = FLOAT(iMaskWidth-1)/(ptdDest->GetWidth()-1)/iMaskDiv;
    FLOAT fVMaskStep = FLOAT(iMaskWidth-1)/(ptdDest->GetWidth()-1)/iMaskDiv;
    FLOAT fMaskVPos=0;
    
    // get first byte in destination texture
    ULONG *pulTexDst = (ULONG*)&ptdDest->td_pulFrames[0];
    // get first byte in source texture
    ULONG *pulFirstInMipSrc = &ptdSrc->td_pulFrames[iMipAdr];
  
    // for each row
    for(UINT ir=0;ir<ptdDest->GetWidth();ir++) {
      // get first byte for src mip texture in this row
      ULONG *pulSrcRow = &pulFirstInMipSrc[(ir&(iSrcMipWidth-1))*iSrcMipWidth];//%
      INDEX iMaskVPos = (INDEX)fMaskVPos * (iMaskWidth);
      UBYTE *pubMaskRow = &ubFirstInMask[iMaskVPos];
      FLOAT fMaskHPos = 0;
      // for each column
      for(UINT ic=0;ic<ptdDest->GetWidth();ic++) {

        ULONG *ulSrc = &pulSrcRow[ic&(iSrcMipWidth-1)];
        INDEX iMask = (INDEX)fMaskHPos;
        FLOAT f1 = NormByteToFloat(pubMaskRow[iMask]);
        FLOAT f2 = NormByteToFloat(pubMaskRow[iMask+1]);
        FLOAT f3 = NormByteToFloat(pubMaskRow[iMask+iMaskWidth+0]);
        FLOAT f4 = NormByteToFloat(pubMaskRow[iMask+iMaskWidth+1]);
        FLOAT fStrengthX1 = Lerp(f1,f2,fMaskHPos-(INDEX)fMaskHPos);
        FLOAT fStrengthX2 = Lerp(f3,f4,fMaskHPos-(INDEX)fMaskHPos);
        FLOAT fStrength   = Lerp(fStrengthX1,fStrengthX2,fMaskVPos-(INDEX)fMaskVPos);

        CopyPixel(pulTexDst,ulSrc,fStrength);
        pulTexDst++;
        fMaskHPos+=fHMaskStep;
      }
      fMaskVPos+=fVMaskStep;
    }
  }
  // make mipmaps
  MakeMipmaps( 32, ptdDest->td_pulFrames, ptdDest->GetWidth(), ptdDest->GetHeight());
  // force topmap upload
  ptdDest->SetAsCurrent(0,TRUE);
}
#endif
*/

void CTerrain::GenerateTerrainTopMap()
{
  CreateTopMap(tr_tdTopMap,tr_pixTopMapWidth,tr_pixTopMapHeight);
  UpdateTopMap(-1);
}

// Add default layer
void CTerrain::AddDefaultLayer_t(void)
{
  // Add one layer using default texture, but do not refresh terrain 
  CTerrainLayer &tl = AddLayer_t((CTString)"Textures\\Editor\\Default.TEX", LT_NORMAL, FALSE);
  // fill this layer
  tl.ResetLayerMask(255);
}

// Build quadtree for terrain
void CTerrain::BuildQuadTree(void)
{
  INDEX ctQuadNodeRows = tr_ctTilesX;
  INDEX ctQuadNodeCols = tr_ctTilesY;
  INDEX ctQuadNodes = 0;

  // Create quad tree levels
  while(TRUE) {
    QuadTreeLevel &qtl = tr_aqtlQuadTreeLevels.Push();
    // Remember first node
    qtl.qtl_iFirstNode = ctQuadNodes;
    // Add nodes in this level to total node count
    ctQuadNodes += ClampDn(ctQuadNodeRows,(INDEX)1) * ClampDn(ctQuadNodeCols,(INDEX)1);

    // Count nodes in this level
    qtl.qtl_ctNodes    = ctQuadNodes - qtl.qtl_iFirstNode;
    qtl.qtl_ctNodesCol = ctQuadNodeCols;
    qtl.qtl_ctNodesRow = ctQuadNodeRows;
    
    // if only one node is in this level
    if(qtl.qtl_ctNodes == 1) {
      // this is last level so exit loop
      break;
    }
    if(ctQuadNodeCols%2 == 1 && ctQuadNodeCols != 1) {
      ctQuadNodeCols = (ctQuadNodeCols+1)>>1;
    } else {
      ctQuadNodeCols = ctQuadNodeCols>>1;
    }

    if(ctQuadNodeRows%2 == 1 && ctQuadNodeRows != 1) {
      ctQuadNodeRows = (ctQuadNodeRows+1)>>1;
    } else {
      ctQuadNodeRows = ctQuadNodeRows>>1;
    }
  }
  
  QuadTreeLevel &qtlFirst = tr_aqtlQuadTreeLevels[0];
  // Add quadtree nodes for first level
  tr_aqtnQuadTreeNodes.Push(qtlFirst.qtl_ctNodes);
  // for each quad tree node in first level
  for(INDEX iqn=0;iqn<qtlFirst.qtl_ctNodes;iqn++) {
    // Generate vertices for tile in first lod with no vertex lerping
    CTerrainTile &tt = tr_attTiles[iqn];
    QuadTreeNode &qtn = tr_aqtnQuadTreeNodes[iqn];
    tt.tt_iLod = -1;
    tt.tt_iRequestedLod  = 0;
    tt.tt_fLodLerpFactor = 0;
    tt.AddFlag(TT_REGENERATE|TT_NO_TOPMAP_REGEN);
    ReGenerateTile(iqn);
    tt.RemoveFlag(TT_REGENERATE|TT_NO_TOPMAP_REGEN);

    // Set quad tree bbox as first vertex in tile
    GFXVertex4 &vtxFirst = tt.GetVertices()[0];
    qtn.qtn_aabbox = FLOATaabbox3D(FLOAT3D(vtxFirst.x,vtxFirst.y,vtxFirst.z));
    // for each vertex after first
    INDEX ctVtx = tt.GetVertices().Count();
    for(INDEX ivx=1;ivx<ctVtx;ivx++) {
      // Add vertex in quad tree node bbox
      GFXVertex4 &vtx = tt.GetVertices()[ivx];
      qtn.qtn_aabbox |= FLOATaabbox3D(FLOAT3D(vtx.x,vtx.y,vtx.z));
    }
    // release arrays of tile
    tt.ReleaseTileArrays();
    tt.tt_iLod = -1;
    tt.tt_iRequestedLod = 0;
    qtn.qtn_iTileIndex = iqn;
    // nodes in first level does not have children
    qtn.qtn_iChild[0] = -1;
    qtn.qtn_iChild[1] = -1;
    qtn.qtn_iChild[2] = -1;
    qtn.qtn_iChild[3] = -1;
  }

  // Create all other levels of quad tree
  INDEX ctQuadLevels = tr_aqtlQuadTreeLevels.Count();
  // for each quadtree level after first
  for(INDEX iql=1;iql<ctQuadLevels;iql++) {
    //QuadTreeLevel &qtl = tr_aqtlQuadTreeLevels[iql];
    QuadTreeLevel &qtlPrev = tr_aqtlQuadTreeLevels[iql-1];
    // for each quadtree node row
    for(INDEX ir=0;ir<qtlPrev.qtl_ctNodesRow;ir+=2) {
      // for each quadtree node col
      for(INDEX ic=0;ic<qtlPrev.qtl_ctNodesCol;ic+=2) {
        // Set quadtree node children 
        INDEX iqt = qtlPrev.qtl_iFirstNode + ic+ir*qtlPrev.qtl_ctNodesCol;
        QuadTreeNode &qtn = tr_aqtnQuadTreeNodes.Push();
        QuadTreeNode *pqtnFirst = &tr_aqtnQuadTreeNodes[iqt];
        // Set first child node
        qtn.qtn_aabbox = pqtnFirst->qtn_aabbox;
        qtn.qtn_iChild[0] = iqt;
        qtn.qtn_iChild[1] = -1;
        qtn.qtn_iChild[2] = -1;
        qtn.qtn_iChild[3] = -1;
        qtn.qtn_iTileIndex = -1; 
        // If second child node exists
        if(ic+1<qtlPrev.qtl_ctNodesCol) {
          // Set second child
          qtn.qtn_iChild[1] = iqt+1;
          qtn.qtn_aabbox |= pqtnFirst[1].qtn_aabbox;
        }
        // if fourth child exist
        if(ir+1<qtlPrev.qtl_ctNodesRow) {
          // Set third child
          qtn.qtn_iChild[2] = iqt+qtlPrev.qtl_ctNodesCol;
          qtn.qtn_aabbox |= pqtnFirst[qtlPrev.qtl_ctNodesCol].qtn_aabbox;
          // Set fourth child
          if(ic+1<qtlPrev.qtl_ctNodesCol) {
            qtn.qtn_iChild[3] = iqt+qtlPrev.qtl_ctNodesCol+1;
            qtn.qtn_aabbox |= pqtnFirst[qtlPrev.qtl_ctNodesCol+1].qtn_aabbox;
          }
        }
      }
    }
  }
}


// Update higher quadtree levels from first one
void CTerrain::UpdateQuadTree()
{
  // for all quad tree levels after first one
  INDEX ctqtl=tr_aqtlQuadTreeLevels.Count();
  for(INDEX iqtl=1;iqtl<ctqtl;iqtl++) {
    QuadTreeLevel &qtl = tr_aqtlQuadTreeLevels[iqtl];

    // for each quad tree node in this level
    INDEX ctqtn=qtl.qtl_iFirstNode+qtl.qtl_ctNodes;
    for(INDEX iqtn=qtl.qtl_iFirstNode;iqtn<ctqtn;iqtn++) {
      QuadTreeNode &qtn = tr_aqtnQuadTreeNodes[iqtn];
      // trash qtn box
      qtn.qtn_aabbox.maxvect = FLOAT3D(-1,-1,-1);
      qtn.qtn_aabbox.minvect = FLOAT3D( 1, 1, 1);

      // for each child of qtn
      for(INDEX ichild=0;ichild<4;ichild++) {
        INDEX iqtnChild = qtn.qtn_iChild[ichild];
        // if child exists
        if(iqtnChild!=(-1)) {
          QuadTreeNode &qtnChild = tr_aqtnQuadTreeNodes[iqtnChild];
          // if qtn box is empty 
          if(qtn.qtn_aabbox.IsEmpty()) {
            // set qtn box as box of this child 
            qtn.qtn_aabbox = qtnChild.qtn_aabbox;
          // if it had some data
          } else {
            // just add child box to qtn box
            qtn.qtn_aabbox |= qtnChild.qtn_aabbox;
          }
        }
      }
    }
  }
}


/*
 * Generation
 */ 

void CTerrain::ReGenerate(void)
{
  // for each tile in terrain
  for(INDEX it=0;it<tr_ctTiles;it++) {
    CTerrainTile &tt = tr_attTiles[it];
    // calculate new lod
    tt.tt_iRequestedLod = tt.CalculateLOD();
  }

  // for each tile that is waiting in regen queue
  INDEX ctrt = tr_auiRegenList.Count();
  INDEX irt;
  for(irt=0;irt<ctrt;irt++) {
    // mark tile as ready for regeneration
    INDEX iTileIndex = tr_auiRegenList[irt];
    CTerrainTile &tt = tr_attTiles[iTileIndex];
    tt.AddFlag(TT_REGENERATE);
  }

  // for each tile that is waiting in regen queue
  for(irt=0;irt<ctrt;irt++) {
    INDEX iTileIndex = tr_auiRegenList[irt];
    CTerrainTile &tt = tr_attTiles[iTileIndex];
    // if tile needs to be regenerated
    if(tt.GetFlags() & TT_REGENERATE) {
      // Regenerate it now
      ReGenerateTile(tt.tt_iIndex);
      // remove flag for regeneration
      tt.RemoveFlag(TT_REGENERATE);
    }
  }

  // clear regenration list
  ClearRegenList();
}

extern CStaticStackArray<GFXVertex4> _avLerpedVerices;
static void ShowTerrainInfo(CAnyProjection3D &apr, CDrawPort *pdp, CTerrain *ptrTerrain)
{
  pdp->SetFont( _pfdConsoleFont);
  pdp->SetTextAspect( 1.0f);
  pdp->SetOrtho();
  CTString strInfo;
  INDEX ctTopMaps = ptrTerrain->tr_atdTopMaps.Count() + 1;
  strInfo.PrintF("Tris = %d\nNodes = %d\nDelayed nodes = %d\nTop maps = %d\nTexgens = %d, %d\nShadowmap updates = %d\n",
                 _ctTris,_ctNodesVis,_ctDelayedNodes,ctTopMaps,ctGeneratedTopMaps,ctGlobalTopMaps,_ctShadowMapUpdates);

  CStaticStackArray<INDEX> iaLodInfo;
  iaLodInfo.Push(ptrTerrain->tr_iMaxTileLod+1);
  memset(&iaLodInfo[0],0,sizeof(INDEX)*iaLodInfo.sa_Count);
  // build lod info
  for(INDEX it=0;it<ptrTerrain->tr_ctTiles;it++) {
    CTerrainTile &tt = ptrTerrain->tr_attTiles[it];
    INDEX &ili = iaLodInfo[tt.tt_iLod];
    ili++;
  }
  // Show how many tiles are in witch lod
  CTString strTemp = "LodInfo:\n";
  for(INDEX itti=0;itti<ptrTerrain->tr_iMaxTileLod+1;itti++) {
    CTString str;
    CArrayHolder &ah = ptrTerrain->tr_aArrayHolders[itti];
    str.PrintF("L%d = mem = %d KB, nodes = %d\n",itti, ah.GetUsedMemory()/1024, iaLodInfo[itti]);
    strTemp += str;
  }
  strTemp += "\n";
  strInfo +=strTemp;

  // Show memory usage
  //SLONG slUsedMemory=0;
  // Height map usage
  SLONG slHeightMap = ptrTerrain->tr_pixHeightMapWidth*ptrTerrain->tr_pixHeightMapHeight*sizeof(UWORD);
  // Edge map usage
  SLONG slEdgeMap = ptrTerrain->tr_pixHeightMapWidth*ptrTerrain->tr_pixHeightMapHeight*sizeof(UBYTE);
  // Shadow map usage
  SLONG slShadowMap = ptrTerrain->tr_tdShadowMap.GetUsedMemory();
  // Quad tree usage
  SLONG slQTNodes  = sizeof(QuadTreeNode)*ptrTerrain->tr_aqtnQuadTreeNodes.Count();
  SLONG slQTLevels = sizeof(QuadTreeLevel)*ptrTerrain->tr_aqtlQuadTreeLevels.Count();
  // Tiles usage 
  SLONG slTiles = 0;
  INDEX cttt = ptrTerrain->tr_ctTiles;
  for(INDEX itt=0;itt<cttt;itt++) {
    CTerrainTile &tt = ptrTerrain->tr_attTiles[itt];
    slTiles+=tt.GetUsedMemory();
  }
  // Arrays holders usage
  SLONG slArrayHoldes=0;
  INDEX ctah=ptrTerrain->tr_aArrayHolders.Count();
  for(INDEX iah=0;iah<ctah;iah++) {
    CArrayHolder &ah = ptrTerrain->tr_aArrayHolders[iah];
    slArrayHoldes+=ah.GetUsedMemory();
  }
  SLONG slLayers=0;
  // Terrain layers usage
  INDEX cttl = ptrTerrain->tr_atlLayers.Count();
  for(INDEX itl=0;itl<cttl;itl++) {
    CTerrainLayer &tl = ptrTerrain->tr_atlLayers[itl];
    slLayers+=tl.GetUsedMemory();
  }
  SLONG slTopMaps=0;
  // Top maps usage
  INDEX cttm=ptrTerrain->tr_atdTopMaps.Count();
  for(INDEX itm=0;itm<cttm;itm++) {
    CTextureData *ptdTopMap = &ptrTerrain->tr_atdTopMaps[itm];
    slTopMaps+=ptdTopMap->GetUsedMemory();
  }
  SLONG slGlobalTopMap = ptrTerrain->tr_tdTopMap.GetUsedMemory();
  SLONG slTileBatchingSize = GetUsedMemoryForTileBatching();
  SLONG slVertexSmoothing  = _avLerpedVerices.sa_Count * sizeof(GFXVertex4);
  extern SLONG  _slSharedTopMapSize; // Shared top map size
  // Global top map usage
  SLONG slTotal = slHeightMap+slEdgeMap+slShadowMap+slQTNodes+slQTLevels+slTiles+slArrayHoldes+slLayers+
                  slTopMaps+slGlobalTopMap+slTileBatchingSize+slVertexSmoothing;
  CTString strMemoryUsed;
  strMemoryUsed.PrintF("Heightmap = %d KB\nEdgemap   = %d KB\nShadowMap = %d KB\nQuadTree  = %d KB\nTiles     = %d KB\nArrays    = %d KB\nLayers    = %d KB\nTopMaps   = %d KB\nGlobal TM = %d KB\nShared TM = %d KB\nVtx lerp  = %d KB\nBatching  = %d KB\nTotal     = %d KB\n",
                        slHeightMap/1024,slEdgeMap/1024,slShadowMap/1024,(slQTNodes+slQTLevels)/1024,slTiles/1024,slArrayHoldes/1024,slLayers/1024,slTopMaps/1024,slGlobalTopMap/1024,_slSharedTopMapSize/1024,slVertexSmoothing/1024,slTileBatchingSize/1024,slTotal/1024);


  strInfo += strMemoryUsed;

  extern FLOAT3D _vDirection;
  strInfo += CTString(0,"Shadow map size = %d,%d [%d]\nShading map size= %d,%d [%d]\n",
                      ptrTerrain->GetShadowMapWidth(), ptrTerrain->GetShadowMapHeight(),
                      ptrTerrain->tr_iShadowMapSizeAspect,ptrTerrain->GetShadingMapWidth(),
                      ptrTerrain->GetShadingMapHeight(),ptrTerrain->tr_iShadingMapSizeAspect);
  pdp->PutText(strInfo,0,40);
}







static void ReadOldShadowMap(CTerrain *ptrTerrain, CTStream *istrFile)
{
  // Read terrain shadow map
  // Read shadow map size
  INDEX pixShadowMapWidth;
  INDEX pixShadowMapHeight;
  (*istrFile)>>pixShadowMapWidth;
  (*istrFile)>>pixShadowMapHeight;
  BOOL bHaveShadowMap;
  (*istrFile)>>bHaveShadowMap;
  // is shadow map saved
  if(bHaveShadowMap) {
    // skip reading of first mip of shadow map
    istrFile->Seek_t(pixShadowMapWidth*pixShadowMapHeight*sizeof(GFXColor),CTStream::SD_CUR);
    //istrFile->Read_t(&tr_tdShadowMap.td_pulFrames[0],tr_pixShadowMapWidth*tr_pixShadowMapHeight*sizeof(GFXColor));
  }
  // Set default shadow map size
  ptrTerrain->SetShadowMapsSize(0,0);
}

void CTerrain::ReadVersion_t( CTStream *istrFile, INDEX iSavedVersion)
{
  // set current terrain
  _ptrTerrain = this;

  ASSERT(_CrtCheckMemory());
  PIX pixWidth;
  PIX pixHeight;
  // read height map width and height
  (*istrFile)>>pixWidth;
  (*istrFile)>>pixHeight;

  // Reallocate memory for terrain with size
  AllocateHeightMap(pixWidth,pixHeight);

  // read terrain stretch
  (*istrFile)>>tr_vStretch;
  // read texture stretch
  if(iSavedVersion<6) {
    FLOAT fTemp; // Read temp stretch
    (*istrFile)>>fTemp;
  }
  // read lod distance factor
  (*istrFile)>>tr_fDistFactor;

  if(iSavedVersion>6) {
    // Read terrain size
    (*istrFile)>>tr_vTerrainSize;

    istrFile->ExpectID_t("TRSM"); // 'Terrain shadowmap'

    // if version is smaller than 8
    if(iSavedVersion<8) {
      // read old shadow map format
      ReadOldShadowMap(this,istrFile);
    } else {
      INDEX iShadowMapAspect;
      INDEX iShadingMapAspect;
      (*istrFile)>>iShadowMapAspect;
      (*istrFile)>>iShadingMapAspect;
      SetShadowMapsSize(iShadowMapAspect,iShadingMapAspect);
      INDEX iShadowMapSize = GetShadowMapWidth() * GetShadowMapHeight();
      INDEX iShadingMapSize = GetShadingMapWidth() * GetShadingMapHeight();
      // Read shadow map
      ASSERT(tr_tdShadowMap.td_pulFrames!=NULL);
      for (INDEX i = 0; i < iShadowMapSize; i++)
        (*istrFile)>>tr_tdShadowMap.td_pulFrames[i];
      // Read shading map
      ASSERT(tr_auwShadingMap!=NULL);
      for (INDEX i = 0; i < iShadingMapSize; i++)
        (*istrFile)>>tr_auwShadingMap[i];
    }

    // Create shadow map mipmaps
    INDEX ctMipMaps = GetNoOfMipmaps(GetShadowMapWidth(),GetShadowMapHeight());
    MakeMipmaps(ctMipMaps, tr_tdShadowMap.td_pulFrames, GetShadowMapWidth(), GetShadowMapHeight());
    // Upload shadow map
    tr_tdShadowMap.SetAsCurrent(0,TRUE);

    istrFile->ExpectID_t("TSEN"); // 'Terrain shadowmap end'


    // if there is edge map saved
    if(istrFile->PeekID_t()==CChunkID("TREM")) { // 'Terrain edge map'
      // Read terrain edge map
      istrFile->ExpectID_t("TREM"); // 'Terrain edge map'
      // read edge map
      istrFile->Read_t(&tr_aubEdgeMap[0],tr_pixHeightMapWidth*tr_pixHeightMapHeight);
      istrFile->ExpectID_t("TEEN"); // 'Terrain edge map end'
    }
  }
  
  (*istrFile).ExpectID_t("TRHM");  // 'Terrain heightmap'

  // read height map
  for (ULONG i = 0; i < static_cast<ULONG>(tr_pixHeightMapWidth*tr_pixHeightMapHeight); i++)
    (*istrFile)>>tr_auwHeightMap[i];
  (*istrFile).ExpectID_t("THEN");  // 'Terrain heightmap end'

  // Terrain will be rebuild in entity.cpp
  _ptrTerrain = NULL;
}

// Read from stream.
void CTerrain::Read_t( CTStream *istrFile)
{
  (*istrFile).ExpectID_t("TERR");  // 'Terrain'
  // read the version number
  INDEX iSavedVersion;
  (*istrFile)>>iSavedVersion;
  // is this version 6
  if(iSavedVersion>4) {
    ReadVersion_t(istrFile,iSavedVersion);
  // else unknown version
  } else {
    // report error
    ThrowF_t( TRANS("The terrain version on disk is %d.\n"
      "Current supported version is %d."), iSavedVersion, _iTerrainVersion);
  }

  // Read Terrain layers
  (*istrFile).ExpectID_t("TRLR");  // 'Terrain layers'
  // Read terrain layers
  INDEX cttl;
  (*istrFile)>>cttl;
  // Create layers
  tr_atlLayers.Push(cttl);
  // for each terrain layer
  for(INDEX itl=0;itl<cttl;itl++) {
    // Read layer texture and mask
    CTerrainLayer &tl = tr_atlLayers[itl];
    tl.Read_t(istrFile,iSavedVersion);
  }
  (*istrFile).ExpectID_t("TLEN");  // 'Terrain layers end'

  // End of reading
  (*istrFile).ExpectID_t("TREN");  // 'terrain end'
}

// Write to stream.
void CTerrain::Write_t( CTStream *ostrFile)
{
  (*ostrFile).WriteID_t("TERR");  // 'Terrain'

  // write the terrain version
  (*ostrFile)<<_iTerrainVersion;
  // write height map width and height
  (*ostrFile)<<tr_pixHeightMapWidth;
  (*ostrFile)<<tr_pixHeightMapHeight;
  // write terrain stretch
  (*ostrFile)<<tr_vStretch;
  // write lod distance factor
  (*ostrFile)<<tr_fDistFactor;
  // Write terrain size
  (*ostrFile)<<tr_vTerrainSize;

  // Write terrain shadow map
  ostrFile->WriteID_t("TRSM");    // 'Terrain shadowmap'

  (*ostrFile)<<tr_iShadowMapSizeAspect;
  (*ostrFile)<<tr_iShadingMapSizeAspect;
  INDEX iShadowMapSize = GetShadowMapWidth() * GetShadowMapHeight() * sizeof(ULONG);
  INDEX iShadingMapSize = GetShadingMapWidth() * GetShadingMapHeight() * sizeof(UWORD);
  // Write shadow map
  ASSERT(tr_tdShadowMap.td_pulFrames!=NULL);
  ostrFile->Write_t(&tr_tdShadowMap.td_pulFrames[0],iShadowMapSize);
  // Write shading map
  ASSERT(tr_auwShadingMap!=NULL);
  ostrFile->Write_t(&tr_auwShadingMap[0],iShadingMapSize);

  ostrFile->WriteID_t("TSEN");    // 'Terrain shadowmap end'

  // if edge map exists
  if(tr_aubEdgeMap!=NULL) {
    ostrFile->WriteID_t("TREM");    // 'Terrain edge map'
    // Write edge map
    ostrFile->Write_t(&tr_aubEdgeMap[0],sizeof(UBYTE)*tr_pixHeightMapWidth*tr_pixHeightMapHeight);
    ostrFile->WriteID_t("TEEN");    // 'Terrain edge map end'
  }

  (*ostrFile).WriteID_t("TRHM");  // 'Terrain heightmap'
  // write height map
  (*ostrFile).Write_t(&tr_auwHeightMap[0],sizeof(UWORD)*tr_pixHeightMapWidth*tr_pixHeightMapHeight);
  (*ostrFile).WriteID_t("THEN");  // 'Terrain heightmap end'

  (*ostrFile).WriteID_t("TRLR");  // 'Terrain layers'
  // write terrain layers
  INDEX cttl = tr_atlLayers.Count();
  (*ostrFile)<<cttl;
  for(INDEX itl=0;itl<cttl;itl++) {
    CTerrainLayer &tl = tr_atlLayers[itl];
    tl.Write_t(ostrFile);
  }

  (*ostrFile).WriteID_t("TLEN");  // 'Terrain layers end'

  (*ostrFile).WriteID_t("TREN");  // 'terrain end'
}

// Copy terrain data from other terrain
void CTerrain::Copy(CTerrain &trOther)
{
  ASSERT(FALSE);
}

CTerrain::~CTerrain()
{
  Clear();
}

// Discard all cached shading info for models
void CTerrain::DiscardShadingInfos(void)
{
  FORDELETELIST( CShadingInfo, si_lnInPolygon, tr_lhShadingInfos, itsi) {
    itsi->si_penEntity->en_ulFlags &= ~ENF_VALIDSHADINGINFO;
    itsi->si_lnInPolygon.Remove();
    itsi->si_pbpoPolygon = NULL;
  }
}

// Clear height map
void CTerrain::ClearHeightMap(void)
{
  // if height map space was allocated
  if(tr_auwHeightMap!=NULL) {
    // release it
    FreeMemory(tr_auwHeightMap);
    tr_auwHeightMap = NULL;
  }
}

// Clear shadow map
void CTerrain::ClearShadowMap(void)
{
  // Clear current terrain shadow map
  tr_tdShadowMap.Clear();

  // Also clear shading map
  if(tr_auwShadingMap!=NULL) {
    FreeMemory(tr_auwShadingMap);
    tr_auwShadingMap = NULL;
  }
}

void CTerrain::ClearEdgeMap(void)
{
  // if space for edge map was allocated
  if(tr_aubEdgeMap!=NULL) {
    // release it
    FreeMemory(tr_aubEdgeMap);
    tr_aubEdgeMap = NULL;
  }
}

// Clear all topmaps
void CTerrain::ClearTopMaps(void)
{
  // for each topmap in terrain
  INDEX cttm = tr_atdTopMaps.Count();
  for(INDEX itm=0;itm<cttm;itm++) {
    CTextureData *ptdTopMap = &tr_atdTopMaps[0];
    // Remove memory pointer cos it is shared memory
    ptdTopMap->td_pulFrames = NULL;
    // Clear tile topmap
    ptdTopMap->Clear();
    // remove topmap from container
    tr_atdTopMaps.Remove(ptdTopMap);
    delete ptdTopMap;
    ptdTopMap = NULL;
  }
  tr_atdTopMaps.Clear();
  ASSERT(_CrtCheckMemory());

  // Remove memory pointer from global top map cos it is shared memory
  tr_tdTopMap.td_pulFrames = NULL;
  // Clear global topmap
  tr_tdTopMap.Clear();
}

// Clear tiles
void CTerrain::ClearTiles(void)
{
  // for each tile
  for(INDEX itt=0;itt<tr_ctTiles;itt++) {
    CTerrainTile &tt = tr_attTiles[itt];
    // Clear tile
    tt.Clear();
  }
  tr_attTiles.Clear();
  tr_ctTiles  = 0;
  tr_ctTilesX = 0;
  tr_ctTilesY = 0;
}

// Clear arrays
void CTerrain::ClearArrays(void)
{
  // Clear Array holders
  tr_aArrayHolders.Clear();
}

// Clear quadtree
void CTerrain::ClearQuadTree(void)
{
  // Destroy quad tree nodes
  tr_aqtnQuadTreeNodes.Clear();
  // Clear quad tree levels
  tr_aqtlQuadTreeLevels.Clear();
}

// Clear layers
void CTerrain::ClearLayers(void)
{
  // Clear layers
  tr_atlLayers.Clear();
}

// Clean terrain data (does not remove layers)
void CTerrain::Clean(BOOL bCleanLayers/*=TRUE*/)
{
  ASSERT(FALSE);
  ClearHeightMap();
  ClearShadowMap();
  ClearEdgeMap();
  ClearTopMaps();
  ClearTiles();
  ClearArrays();
  ClearQuadTree();

  // if layers clear is required
  if(bCleanLayers) {
    ClearLayers();
  }
}

// Clear terrain data
void CTerrain::Clear(void)
{
  DiscardShadingInfos();
  ClearHeightMap();
  ClearShadowMap();
  ClearEdgeMap();
  ClearTopMaps();
  ClearTiles();
  ClearArrays();
  ClearQuadTree();
  ClearLayers();

  if(tr_ptdDetailMap!=NULL) {
    _pTextureStock->Release(tr_ptdDetailMap);
    tr_ptdDetailMap = NULL;
  }
}


