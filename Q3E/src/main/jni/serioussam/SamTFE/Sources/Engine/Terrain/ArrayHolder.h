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

#ifndef SE_INCL_ARRAY_HOLDER_H
#define SE_INCL_ARRAY_HOLDER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Graphics/Vertex.h>
#include <Engine/Templates/StaticStackArray.cpp>
#include <Engine/Templates/DynamicContainer.h>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Graphics/Texture.h>

struct TileLayer
{
  CStaticStackArray<INDEX_T>     tl_auiIndices;   // Array of indices for one layer
  CStaticStackArray<GFXColor>    tl_acColors;     // Array of colors for one layer
  CStaticStackArray<GFXTexCoord> tl_atcTexCoords; // Array of texcoords for one layer
  CStaticStackArray<GFXVertex>   tl_avVertices;   // Array of vertices  for one layer (used only if tile layer)
};

struct TileArrays
{
  void operator=(const TileArrays &taOther) {
    this->ta_avVertices = taOther.ta_avVertices;
    this->ta_auvTexCoords = taOther.ta_auvTexCoords;
    this->ta_auvShadowMap = taOther.ta_auvShadowMap;
    this->ta_auvDetailMap = taOther.ta_auvDetailMap;
    this->ta_auiIndices   = taOther.ta_auiIndices;
    this->ta_atlLayers    = taOther.ta_atlLayers;
    this->ta_ptdTopMap    = taOther.ta_ptdTopMap;
  }
  CStaticStackArray<GFXVertex4>  ta_avVertices;   // Array of vertices for one tile
  CStaticStackArray<GFXTexCoord> ta_auvTexCoords; // Array of texcoords for one tile (not used in highest lod)
  CStaticStackArray<GFXTexCoord> ta_auvShadowMap; // Array of texcoords for shadow map
  CStaticStackArray<GFXTexCoord> ta_auvDetailMap; // Array of texcoords for detail map
  CStaticStackArray<INDEX_T>     ta_auiIndices;   // Array of indices for one tile
  CStaticStackArray<TileLayer>   ta_atlLayers;    // Array if layers per tile (used only in highest lod)
  CTextureData                  *ta_ptdTopMap;    // Pointer to tile top map
};

class ENGINE_API CArrayHolder
{
public:
  CArrayHolder();
  ~CArrayHolder();
  void operator=(const CArrayHolder &ahOther);

  // Returns index of new tile arrays
  INDEX GetNewArrays();
  // Mark tile arrays as unused
  void FreeArrays(SINT iOldArraysIndex);
  // Just do popall on all arrays
  void EmptyArrays(INDEX iArrayIndex);
  // Release array holder
  void Clear(void);
  // Count used memory
  SLONG GetUsedMemory(void);

public:
  CTerrain *ah_ptrTerrain; // Terrain that owns this array holder
  CStaticStackArray<TileArrays> ah_ataTileArrays; // array of tile arrays
  CStaticStackArray<INDEX>      ah_aiFreeArrays;  // array of indices of free arrays
  INDEX ah_iLod; // this array holder works in this lod
};

#endif
