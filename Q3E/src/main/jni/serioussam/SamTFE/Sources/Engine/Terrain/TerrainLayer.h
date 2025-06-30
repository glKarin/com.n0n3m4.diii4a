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

#ifndef SE_INCL_TERRAIN_LAYER_H
#define SE_INCL_TERRAIN_LAYER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Graphics/Texture.h>

enum LayerType {
  LT_NORMAL = 0,
  LT_TILE,
};

#define TL_FLIPX_SHIFT   4
#define TL_FLIPY_SHIFT   5
#define TL_SWAPXY_SHIFT  6
#define TL_VISIBLE_SHIFT 7

#define TL_TILE_INDEX 0x0F
#define TL_VISIBLE    (1<<TL_VISIBLE_SHIFT)
#define TL_FLIPX      (1<<TL_FLIPX_SHIFT)
#define TL_FLIPY      (1<<TL_FLIPY_SHIFT)
#define TL_SWAPXY     (1<<TL_SWAPXY_SHIFT)

class ENGINE_API CTerrainLayer {
public:
  CTerrainLayer();
  ~CTerrainLayer();
  // Import layer mask from targa file
  void ImportLayerMask_t(CTFileName fnLayerMask);
  // Export layer mask to targa file
  void ExportLayerMask_t(CTFileName fnLayerMask);
  // Set layer size
  void SetLayerSize(INDEX iTerrainWidth, INDEX iTerrainHeight);
  // Set layer texture
  void SetLayerTexture_t(CTFileName fnTexture);
  // Set num of tiles in one row
  void SetTilesPerRow(INDEX ctTilesInRow);
  // Get num of tiles in one row
  INDEX GetTilesPerRow(void);
  // Get thumbnail
  CTextureData *GetThumbnail(INDEX iWidth, INDEX iHeight);
  // Reset layer mask
  void ResetLayerMask(UBYTE ubMaskFill);
  // Reset layer params
  void ResetLayerParams();
  // Read from stream.
  void Read_t(CTStream *istrFile,INDEX iSavedVersion); // throw char *
  // Write to stream.
  void Write_t( CTStream *ostrFile);  // throw char *
  // Copy terrain data from other terrain
  void Copy(const CTerrainLayer &tlOther);
  void operator=(const CTerrainLayer &tlOther);
  // Clear layer
  void Clear();
  // Count used memory
  SLONG GetUsedMemory(void);

public:
  CTextureData *tl_ptdTexture;  // Texture this layer is using
  UBYTE        *tl_aubColors;   // Array of vertex alpha colors for this layer
  CTextureData  tl_tdThumbNail; // Mask thumbnail (used only in we)
  INDEX         tl_iMaskWidth;  // Width of terrain layer mask
  INDEX         tl_iMaskHeight; // Height of terrain layer mask

  CTString      tl_strName;     // Layer name
  BOOL          tl_bVisible;    // Is layer visible
  COLOR         tl_colMultiply; // Color of layer
  FLOAT         tl_fSmoothness; // Layer smoothness
  LayerType     tl_ltType;      // Layer type

  FLOAT         tl_fRotateX;
  FLOAT         tl_fRotateY;
  FLOAT         tl_fStretchX;
  FLOAT         tl_fStretchY;
  FLOAT         tl_fOffsetX;
  FLOAT         tl_fOffsetY;

  BOOL          tl_bAutoRegenerated;
  FLOAT         tl_fCoverage;
  FLOAT         tl_fCoverageNoise;
  FLOAT         tl_fCoverageRandom;
  
  BOOL          tl_bApplyMinAltitude;
  FLOAT         tl_fMinAltitude;
  FLOAT         tl_fMinAltitudeFade;
  FLOAT         tl_fMinAltitudeNoise;
  FLOAT         tl_fMinAltitudeRandom;

  BOOL          tl_bApplyMaxAltitude;
  FLOAT         tl_fMaxAltitude;
  FLOAT         tl_fMaxAltitudeFade;
  FLOAT         tl_fMaxAltitudeNoise;
  FLOAT         tl_fMaxAltitudeRandom;

  BOOL          tl_bApplyMinSlope;
  FLOAT         tl_fMinSlope;
  FLOAT         tl_fMinSlopeFade;
  FLOAT         tl_fMinSlopeNoise;
  FLOAT         tl_fMinSlopeRandom;

  BOOL          tl_bApplyMaxSlope;
  FLOAT         tl_fMaxSlope;
  FLOAT         tl_fMaxSlopeFade;
  FLOAT         tl_fMaxSlopeNoise;
  FLOAT         tl_fMaxSlopeRandom;

// private:
public:
  INDEX         tl_ctTilesInRow;  // Number of tiles in one row (used if this is tile layer)
  INDEX         tl_ctTilesInCol;  
  INDEX         tl_iSelectedTile; // GUI variable, selected tile for this layer
  PIX           tl_pixTileWidth;
  PIX           tl_pixTileHeight;
  FLOAT         tl_fTileU;
  FLOAT         tl_fTileV;
};

#endif
