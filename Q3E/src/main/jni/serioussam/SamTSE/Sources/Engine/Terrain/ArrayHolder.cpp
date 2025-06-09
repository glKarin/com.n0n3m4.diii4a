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
#include <Engine/Terrain/ArrayHolder.h>
#include <Engine/Terrain/Terrain.h>
#include <Engine/Terrain/TerrainMisc.h>

CArrayHolder::CArrayHolder()
{
}

CArrayHolder::~CArrayHolder()
{
  Clear();
}

void CArrayHolder::operator=(const CArrayHolder &ahOther)
{
  ASSERT(FALSE);
}

// Returns pointer for new tile arrays
INDEX CArrayHolder::GetNewArrays()
{
  INDEX ctUnusedArrays = ah_aiFreeArrays.Count();
  // if there are some unused arrays
  if(ctUnusedArrays>0) {
    // get index of last unused arrays
    INDEX iArrays = ah_aiFreeArrays[ctUnusedArrays-1];
    // mark last arrays in stack as used
    ah_aiFreeArrays.Pop();
    // return last arrays in stack
    return iArrays;
  // there arn't any unused arrays 
  } else {
    // allocate new one
    TileArrays &ta = ah_ataTileArrays.Push();

    // if this array holder does not hold tiles in highes nor in lowest lod 
    if(ah_iLod>0 && ah_iLod<ah_ptrTerrain->tr_iMaxTileLod) {
      // create new topmap for tile
      CTextureData *ptdTopMap = new CTextureData;
      ah_ptrTerrain->tr_atdTopMaps.Add(ptdTopMap);
      ta.ta_ptdTopMap = ptdTopMap;
      
      // Setup tile topmap
      INDEX iTopMapWidth  = ah_ptrTerrain->tr_pixFirstMipTopMapWidth>>(ah_iLod-1);
      INDEX iTopMapHeight = ah_ptrTerrain->tr_pixFirstMipTopMapHeight>>(ah_iLod-1);
      CreateTopMap(*ta.ta_ptdTopMap,iTopMapWidth,iTopMapHeight);
      ASSERT(ta.ta_ptdTopMap->td_pulFrames==NULL);
    }
    // return index of new arrays
    return ah_ataTileArrays.Count()-1;
  }
}

// Mark tile arrays as unused
void CArrayHolder::FreeArrays(SINT iOldArraysIndex)
{
  // if arrays are valid
  if(iOldArraysIndex!=-1) {
    // remember this arrays as unused
    INDEX &iFreeIndex = ah_aiFreeArrays.Push();
    iFreeIndex = iOldArraysIndex;
    // Popall all arrays
    EmptyArrays(iOldArraysIndex);
  }
}

void CArrayHolder::EmptyArrays(INDEX iArrayIndex)
{
  TileArrays &ta = ah_ataTileArrays[iArrayIndex];
  // for each layer
  INDEX cttl = ta.ta_atlLayers.Count();
  for(INDEX itl=0;itl<cttl;itl++) {
    // clear arrays of layer
    TileLayer &tl = ta.ta_atlLayers[itl];
    tl.tl_acColors.PopAll();
    tl.tl_auiIndices.PopAll();
    tl.tl_atcTexCoords.PopAll();
    tl.tl_avVertices.PopAll();
  }
  // clear its arrays
  ta.ta_avVertices.PopAll();
  ta.ta_auvTexCoords.PopAll();
  ta.ta_auvShadowMap.PopAll();
  ta.ta_auiIndices.PopAll();
  ta.ta_atlLayers.PopAll();
  ta.ta_auvDetailMap.PopAll();
}

// Release array holder
void CArrayHolder::Clear(void)
{
  // for each tile arrays
  SLONG ctta = ah_ataTileArrays.Count();
  for(SLONG ita=0;ita<ctta;ita++) {
    TileArrays &ta = ah_ataTileArrays[ita];
    // for each tile layer
    SLONG cttl = ta.ta_atlLayers.Count();
    for(SLONG itl=0;itl<cttl;itl++) {
      // Clear its indices and vertex color
      TileLayer &tl = ta.ta_atlLayers[itl];
      tl.tl_auiIndices.Clear();
      tl.tl_acColors.Clear();
      tl.tl_atcTexCoords.Clear();
      tl.tl_avVertices.Clear();
    }
    // Clear arrays
    ta.ta_avVertices.Clear();
    ta.ta_auvTexCoords.Clear();
    ta.ta_auvShadowMap.Clear();
    ta.ta_auvDetailMap.Clear();
    ta.ta_auiIndices.Clear();
    ta.ta_atlLayers.Clear();
    // NOTE: Terrain will clear topmap
  }
  // Clear array of tile arrays
  ah_ataTileArrays.Clear();
  // Clear free arrays
  ah_aiFreeArrays.Clear();
}

// Count used memory
SLONG CArrayHolder::GetUsedMemory(void)
{
  // Show memory usage
  SLONG slUsedMemory=0;
  slUsedMemory+=sizeof(CArrayHolder);
  slUsedMemory+=sizeof(SLONG) * ah_aiFreeArrays.sa_Count;
  slUsedMemory+=sizeof(TileArrays) * ah_ataTileArrays.sa_Count;

  INDEX ctta=ah_ataTileArrays.sa_Count;
  if(ctta>0) {
    TileArrays *ptaArrays = &ah_ataTileArrays[0];
    // for each tile array
    for(INDEX ita=0;ita<ctta;ita++) {
      slUsedMemory+=ptaArrays->ta_avVertices.sa_Count * sizeof(GFXVertex);
      slUsedMemory+=ptaArrays->ta_auvTexCoords.sa_Count * sizeof(GFXTexCoord);
      slUsedMemory+=ptaArrays->ta_auvShadowMap.sa_Count * sizeof(GFXTexCoord);
      slUsedMemory+=ptaArrays->ta_auvDetailMap.sa_Count * sizeof(GFXTexCoord);
      slUsedMemory+=ptaArrays->ta_auiIndices.sa_Count * sizeof(INDEX);
      // for each tile layer
      INDEX cttl = ptaArrays->ta_atlLayers.sa_Count;
      if(cttl>0) {
        TileLayer *ptlTileLayer = &ptaArrays->ta_atlLayers.sa_Array[0];
        for(INDEX itl=0;itl<cttl;itl++) {
          slUsedMemory+=ptlTileLayer->tl_auiIndices.sa_Count * sizeof(INDEX);
          slUsedMemory+=ptlTileLayer->tl_acColors.sa_Count   * sizeof(GFXColor);
          slUsedMemory+=ptlTileLayer->tl_atcTexCoords.sa_Count * sizeof(GFXTexCoord);
          slUsedMemory+=ptlTileLayer->tl_avVertices.sa_Count   * sizeof(GFXVertex);
          ptlTileLayer++;
        }
      }
      ptaArrays++;
    }
  }
  return slUsedMemory;
}
