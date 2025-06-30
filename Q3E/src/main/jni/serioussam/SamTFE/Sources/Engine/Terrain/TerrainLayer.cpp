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
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Translation.h>
#include <Engine/Terrain/TerrainLayer.h>
#include <Engine/Terrain/TerrainMisc.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Graphics/ImageInfo.h>

CTerrainLayer::CTerrainLayer()
{
  tl_ptdTexture  = NULL;
  tl_aubColors   = NULL;
  tl_iMaskWidth  = 0;
  tl_iMaskHeight = 0;
  ResetLayerParams();
}

CTerrainLayer::~CTerrainLayer()
{
  Clear();
}

CTextureData *CTerrainLayer::GetThumbnail(INDEX iWidth, INDEX iHeight)
{
  tl_tdThumbNail.Clear();
  tl_tdThumbNail.DefaultAnimation();

  INDEX iMaskWidth = tl_iMaskWidth-1;
  INDEX iMaskHeight = tl_iMaskHeight-1;

  if(iWidth>iMaskWidth) {
    // ASSERT(FALSE);
    iWidth = iMaskWidth;
  }
  if(iHeight>iMaskHeight) {
    // ASSERT(FALSE);
    iHeight = iMaskHeight;
  }

  CreateTexture(tl_tdThumbNail,iWidth,iHeight,TEX_STATIC);

  INDEX iStepX = iMaskWidth/iWidth;
  INDEX iStepY = iMaskHeight/iHeight - 1;
  
  UBYTE *paubMask = tl_aubColors;
  GFXColor *pcolTexture = (GFXColor*)tl_tdThumbNail.td_pulFrames;

  for(INDEX iy=0;iy<iHeight;iy++) {
    for(INDEX ix=0;ix<iWidth;ix++) {
      pcolTexture->ub.r = *paubMask;
      pcolTexture->ub.g = *paubMask;
      pcolTexture->ub.b = *paubMask;
      pcolTexture->ub.a = 0xFF;
      pcolTexture++;
      paubMask+=iStepX;
    }
    paubMask+=(tl_iMaskWidth*iStepY)+1;
  }

  // make mipmaps
  INDEX ctMips = GetNoOfMipmaps(iWidth,iHeight);
  MakeMipmaps(ctMips, tl_tdThumbNail.td_pulFrames, iWidth, iHeight);

  tl_tdThumbNail.SetAsCurrent(0,TRUE);
  return &tl_tdThumbNail;
}

// Set layer size
void CTerrainLayer::SetLayerSize(INDEX iTerrainWidth, INDEX iTerrainHeight)
{
  // if array of vertex colors was initialized
  if(tl_aubColors!=NULL) {
    // free array
    FreeMemory(tl_aubColors);
    tl_aubColors = NULL;
  }
  // if size of mask is greater than 0
  INDEX iSize = iTerrainWidth*iTerrainHeight;
  if(iSize>0) {
    // Allocate new memory for vertex colors
    tl_aubColors = (UBYTE*)AllocMemory(iSize);
    // Reset color values
    memset(&tl_aubColors[0],0,sizeof(UBYTE)*iSize);
  }

  tl_iMaskWidth = iTerrainWidth;
  tl_iMaskHeight = iTerrainHeight;
}

// Set base texture this layer will be using
void CTerrainLayer::SetLayerTexture_t(CTFileName fnTexture)
{
  // if layer has valid texture 
  if(tl_ptdTexture!=NULL) {
    // release texture from stock
    _pTextureStock->Release(tl_ptdTexture);
    tl_ptdTexture = NULL;
  }
  // get reguested texture
  tl_ptdTexture = _pTextureStock->Obtain_t(fnTexture);
  tl_ptdTexture->Force(TEX_STATIC);
  
  // if this is tile layer
  if(tl_ltType == LT_TILE) {
    // Update tile widht and height
    SetTilesPerRow(GetTilesPerRow());
  }
}

// Set num of tiles in one row
void CTerrainLayer::SetTilesPerRow(INDEX ctTilesInRow)
{
  ASSERT(tl_ltType == LT_TILE);
  tl_ctTilesInRow = ctTilesInRow;
  // Calc one tile widht 
  tl_pixTileWidth = tl_ptdTexture->GetPixWidth() / ctTilesInRow;
  // Calc num of tiles in texture col
  tl_ctTilesInCol = tl_ptdTexture->GetPixHeight() / tl_pixTileWidth;

  tl_fTileU = 1.0f / tl_ctTilesInRow;
  tl_fTileV = 1.0f / tl_ctTilesInCol;
}

INDEX CTerrainLayer::GetTilesPerRow()
{
  ASSERT(tl_ltType == LT_TILE);
  return tl_ctTilesInRow;
}

// Import layer mask from targa file
void CTerrainLayer::ImportLayerMask_t(CTFileName fnLayerMask)
{
  // Load targa file 
  CImageInfo iiLayerMask;
  iiLayerMask.LoadAnyGfxFormat_t(fnLayerMask);
  if(iiLayerMask.ii_Width != tl_iMaskWidth) {
    ThrowF_t(TRANS("Layer mask width is %d, but it must be same size as terrain width %d"),iiLayerMask.ii_Width,tl_iMaskWidth);
  }
  if(iiLayerMask.ii_Height != tl_iMaskHeight) {
    ThrowF_t(TRANS("Layer mask height is %d, but it must be same size as terrain height %d"),iiLayerMask.ii_Height,tl_iMaskHeight);
  }

  UBYTE *pubSrc = &iiLayerMask.ii_Picture[0];
  UBYTE *pubDst = &tl_aubColors[0];
  INDEX iBpp = iiLayerMask.ii_BitsPerPixel/8;

  // for each byte in loaded image
  INDEX iMaskSize = tl_iMaskWidth * tl_iMaskHeight;
  for(INDEX ib=0;ib<iMaskSize;ib++) {
    // copy red value from image
    *pubDst = *(UBYTE*)pubSrc;
    pubDst++;
    pubSrc+=iBpp;
  }
}

// Export layer mask to targa file
void CTerrainLayer::ExportLayerMask_t(CTFileName fnLayerMask)
{
  ASSERT(tl_aubColors!=NULL);
  INDEX iSize = tl_iMaskWidth*tl_iMaskHeight;

  CImageInfo iiHeightMap;
  iiHeightMap.ii_Width  = tl_iMaskWidth;
  iiHeightMap.ii_Height = tl_iMaskHeight;
  iiHeightMap.ii_BitsPerPixel = 32;
  iiHeightMap.ii_Picture = (UBYTE*)AllocMemory(iSize*iiHeightMap.ii_BitsPerPixel/8);

  GFXColor *pacolImage = (GFXColor*)&iiHeightMap.ii_Picture[0];
  UBYTE    *pubMask    = &tl_aubColors[0];
  for(INDEX ipix=0;ipix<iSize;ipix++) {
	pacolImage->ul.abgr = 0x00000000;
	pacolImage->ub.r = *pubMask;
    pacolImage++;
    pubMask++;
  }
  iiHeightMap.SaveTGA_t(fnLayerMask);
  iiHeightMap.Clear();
}

// Reset layer mask
void CTerrainLayer::ResetLayerMask(UBYTE ubMaskFill)
{
  memset(&tl_aubColors[0],ubMaskFill,sizeof(UBYTE) * tl_iMaskWidth * tl_iMaskHeight);
}

// Reset layer params
void CTerrainLayer::ResetLayerParams()
{
  tl_iMaskWidth  = 0;
  tl_iMaskHeight = 0;
  tl_strName   = "NoName";
  tl_bVisible  = TRUE;

  tl_colMultiply = C_GRAY|CT_OPAQUE;
  tl_fSmoothness = 1.0f;
  tl_ltType      = LT_NORMAL;
  
  tl_fRotateX=0.0f;
  tl_fRotateY=0.0f;
  tl_fStretchX=1.0f;
  tl_fStretchY=1.0f;
  tl_fOffsetX=0;
  tl_fOffsetY=0;

  tl_bAutoRegenerated=FALSE;
  tl_fCoverage=1.0f;
  tl_fCoverageNoise=0.5f;
  tl_fCoverageRandom=0.0f;
  
  tl_bApplyMinAltitude=TRUE;
  tl_fMinAltitude=0.0f;
  tl_fMinAltitudeFade=0.25f;
  tl_fMinAltitudeNoise=0.25f;
  tl_fMinAltitudeRandom=0;

  tl_bApplyMaxAltitude=TRUE;
  tl_fMaxAltitude=1.0f;
  tl_fMaxAltitudeFade=0.25f;
  tl_fMaxAltitudeNoise=0.25f;
  tl_fMaxAltitudeRandom=0;

  tl_bApplyMinSlope=TRUE;
  tl_fMinSlope=0.0f;
  tl_fMinSlopeFade=0.25f;
  tl_fMinSlopeNoise=0.25f;
  tl_fMinSlopeRandom=0;

  tl_bApplyMaxSlope=TRUE;
  tl_fMaxSlope=1.0f;
  tl_fMaxSlopeFade=0.25f;
  tl_fMaxSlopeNoise=0.25f;
  tl_fMaxSlopeRandom=0;

  // Tile layer properties
  tl_ctTilesInRow  = 1;
  tl_ctTilesInCol  = 1;
  tl_iSelectedTile = 0;
  tl_pixTileWidth  = 128;
  tl_pixTileHeight = 128;
  tl_fTileU        = 1.0f;
  tl_fTileV        = 1.0f;
}

void CTerrainLayer::operator=(const CTerrainLayer &tlOther)
{
  Copy(tlOther);
}

// Copy terrain data from other terrain
void CTerrainLayer::Copy(const CTerrainLayer &tlOther)
{
  // clear current layer data
  Clear();

  // if texture exists
  if(tlOther.tl_ptdTexture!=NULL) {
    // Copy texture
    SetLayerTexture_t(tlOther.tl_ptdTexture->GetName());
  }

  // Allocate memory for vertex colors
  SetLayerSize(tlOther.tl_iMaskWidth,tlOther.tl_iMaskHeight);
  // Copy vertex colors
  memcpy(tl_aubColors,tlOther.tl_aubColors,sizeof(UBYTE) * tl_iMaskWidth * tl_iMaskHeight);

  // Copy reast of params
  tl_strName      = tlOther.tl_strName;
  tl_bVisible     = tlOther.tl_bVisible;
  tl_colMultiply  = tlOther.tl_colMultiply;
  tl_fSmoothness  = tlOther.tl_fSmoothness;
  tl_ltType       = tlOther.tl_ltType;

  tl_fRotateX  = tlOther.tl_fRotateX;
  tl_fRotateY  = tlOther.tl_fRotateY;
  tl_fStretchX = tlOther.tl_fStretchX;
  tl_fStretchY = tlOther.tl_fStretchY;
  tl_fOffsetX  = tlOther.tl_fOffsetX;
  tl_fOffsetY  = tlOther.tl_fOffsetY;

  tl_bAutoRegenerated = tlOther.tl_bAutoRegenerated;
  tl_fCoverage        = tlOther.tl_fCoverage;
  tl_fCoverageNoise   = tlOther.tl_fCoverageNoise;
  tl_fCoverageRandom  = tlOther.tl_fCoverageRandom;
  
  tl_bApplyMinAltitude  = tlOther.tl_bApplyMinAltitude;
  tl_fMinAltitude       = tlOther.tl_fMinAltitude;
  tl_fMinAltitudeFade   = tlOther.tl_fMinAltitudeFade;
  tl_fMinAltitudeNoise  = tlOther.tl_fMinAltitudeNoise;
  tl_fMinAltitudeRandom = tlOther.tl_fMinAltitudeRandom;

  tl_bApplyMaxAltitude  = tlOther.tl_bApplyMaxAltitude;
  tl_fMaxAltitude       = tlOther.tl_fMaxAltitude;
  tl_fMaxAltitudeFade   = tlOther.tl_fMaxAltitudeFade;
  tl_fMaxAltitudeNoise  = tlOther.tl_fMaxAltitudeNoise;
  tl_fMaxAltitudeRandom = tlOther.tl_fMaxAltitudeRandom;

  tl_bApplyMinSlope     = tlOther.tl_bApplyMinSlope;
  tl_fMinSlope          = tlOther.tl_fMinSlope;
  tl_fMinSlopeFade      = tlOther.tl_fMinSlopeFade;
  tl_fMinSlopeNoise     = tlOther.tl_fMinSlopeNoise;
  tl_fMinSlopeRandom    = tlOther.tl_fMinSlopeRandom;

  tl_bApplyMaxSlope     = tlOther.tl_bApplyMaxSlope;
  tl_fMaxSlope          = tlOther.tl_fMaxSlope;
  tl_fMaxSlopeFade      = tlOther.tl_fMaxSlopeFade;
  tl_fMaxSlopeNoise     = tlOther.tl_fMaxSlopeNoise;
  tl_fMaxSlopeRandom    = tlOther.tl_fMaxSlopeRandom;

  // Tile layer properties
  tl_ctTilesInRow       = tlOther.tl_ctTilesInRow;
  tl_ctTilesInCol       = tlOther.tl_ctTilesInCol;
  tl_iSelectedTile      = tlOther.tl_iSelectedTile;
  tl_pixTileWidth       = tlOther.tl_pixTileWidth;
  tl_pixTileHeight      = tlOther.tl_pixTileHeight;
  tl_fTileU             = tlOther.tl_fTileU;
  tl_fTileV             = tlOther.tl_fTileV;
}

// Read from stream.
void CTerrainLayer::Read_t(CTStream *istrFile,INDEX iSavedVersion)
{
  CTFileName fn;
  INDEX iMaskWidth;
  INDEX iMaskHeight;

  // Read terrain layer texture
  (*istrFile).ExpectID_t("TLTX");  // 'Terrain layer texture'
  (*istrFile)>>fn;
  // Add texture to layer
  SetLayerTexture_t(fn);  
  // Read terrain layer mask
  (*istrFile).ExpectID_t("TLMA"); // 'Terrain layer mask'
  (*istrFile)>>iMaskWidth;
  (*istrFile)>>iMaskHeight;
  // Set layer size
  SetLayerSize(iMaskWidth,iMaskHeight);
  (*istrFile).Read_t(&tl_aubColors[0],sizeof(UBYTE) * tl_iMaskWidth * tl_iMaskHeight);
  
  if(istrFile->PeekID_t()==CChunkID("TLPA")) { // 'Terrain edge map'
    // Read terrain layer params
    (*istrFile).ExpectID_t("TLPA"); // 'Terrain layer params'

    (*istrFile)>>tl_strName;
    (*istrFile)>>tl_bVisible;
    FLOAT fDummy;
    (*istrFile)>>tl_fRotateX;
    (*istrFile)>>tl_fRotateY;
    (*istrFile)>>tl_fStretchX;
    (*istrFile)>>tl_fStretchY;
    (*istrFile)>>tl_fOffsetX;
    (*istrFile)>>tl_fOffsetY;
    (*istrFile)>>tl_bAutoRegenerated;
    (*istrFile)>>tl_fCoverage;
    (*istrFile)>>tl_fCoverageNoise;
    (*istrFile)>>fDummy;
    (*istrFile)>>fDummy;
    (*istrFile)>>fDummy;
    (*istrFile)>>fDummy;
    (*istrFile)>>tl_fMinSlope;
    (*istrFile)>>tl_fMaxSlope;
    (*istrFile)>>fDummy;
    (*istrFile)>>fDummy;
  } else {
    // Read terrain layer params
    (*istrFile).ExpectID_t("TLPR"); // 'Terrain layer params'

    (*istrFile)>>tl_strName;
    (*istrFile)>>tl_bVisible;

    (*istrFile)>>tl_fRotateX;
    (*istrFile)>>tl_fRotateY;
    (*istrFile)>>tl_fStretchX;
    (*istrFile)>>tl_fStretchY;
    (*istrFile)>>tl_fOffsetX;
    (*istrFile)>>tl_fOffsetY;

    (*istrFile)>>tl_bAutoRegenerated;
    (*istrFile)>>tl_fCoverage;
    (*istrFile)>>tl_fCoverageNoise;
    (*istrFile)>>tl_fCoverageRandom;

    (*istrFile)>>tl_bApplyMinAltitude;
    (*istrFile)>>tl_fMinAltitude;
    (*istrFile)>>tl_fMinAltitudeFade;
    (*istrFile)>>tl_fMinAltitudeNoise;
    (*istrFile)>>tl_fMinAltitudeRandom;

    (*istrFile)>>tl_bApplyMaxAltitude;
    (*istrFile)>>tl_fMaxAltitude;
    (*istrFile)>>tl_fMaxAltitudeFade;
    (*istrFile)>>tl_fMaxAltitudeNoise;
    (*istrFile)>>tl_fMaxAltitudeRandom;

    (*istrFile)>>tl_bApplyMinSlope;
    (*istrFile)>>tl_fMinSlope;
    (*istrFile)>>tl_fMinSlopeFade;
    (*istrFile)>>tl_fMinSlopeNoise;
    (*istrFile)>>tl_fMinSlopeRandom;

    (*istrFile)>>tl_bApplyMaxSlope;
    (*istrFile)>>tl_fMaxSlope;
    (*istrFile)>>tl_fMaxSlopeFade;
    (*istrFile)>>tl_fMaxSlopeNoise;
    (*istrFile)>>tl_fMaxSlopeRandom;
    
    if(iSavedVersion>=9) {
      INDEX iType;
      (*istrFile)>>tl_colMultiply;
      (*istrFile)>>tl_fSmoothness;
      (*istrFile)>>iType;
      tl_ltType = (LayerType)iType;

      // Tile layer properties
      (*istrFile)>>tl_ctTilesInRow;
      (*istrFile)>>tl_ctTilesInCol;
      (*istrFile)>>tl_iSelectedTile;
      (*istrFile)>>tl_pixTileWidth;
      (*istrFile)>>tl_pixTileHeight;
      (*istrFile)>>tl_fTileU;
      (*istrFile)>>tl_fTileV;
    }
  }
}

// Write to stream.
void CTerrainLayer::Write_t( CTStream *ostrFile)
{
  (*ostrFile).WriteID_t("TLTX"); // 'Terrain layer texture'
  const CTFileName &fn = tl_ptdTexture->GetName();
  (*ostrFile)<<fn;
  // write terrain layer mask
  (*ostrFile).WriteID_t("TLMA"); // 'Terrain layer mask'
  (*ostrFile)<<tl_iMaskWidth;
  (*ostrFile)<<tl_iMaskHeight;
  (*ostrFile).Write_t(&tl_aubColors[0],sizeof(UBYTE) * tl_iMaskWidth * tl_iMaskHeight);

  // Write terrain layer params
  (*ostrFile).WriteID_t("TLPR"); // 'Terrain layer params'
  (*ostrFile)<<tl_strName;
  (*ostrFile)<<tl_bVisible;
  
  (*ostrFile)<<tl_fRotateX;
  (*ostrFile)<<tl_fRotateY;
  (*ostrFile)<<tl_fStretchX;
  (*ostrFile)<<tl_fStretchY;
  (*ostrFile)<<tl_fOffsetX;
  (*ostrFile)<<tl_fOffsetY;

  (*ostrFile)<<tl_bAutoRegenerated;
  (*ostrFile)<<tl_fCoverage;
  (*ostrFile)<<tl_fCoverageNoise;
  (*ostrFile)<<tl_fCoverageRandom;

  (*ostrFile)<<tl_bApplyMinAltitude;
  (*ostrFile)<<tl_fMinAltitude;
  (*ostrFile)<<tl_fMinAltitudeFade;
  (*ostrFile)<<tl_fMinAltitudeNoise;
  (*ostrFile)<<tl_fMinAltitudeRandom;

  (*ostrFile)<<tl_bApplyMaxAltitude;
  (*ostrFile)<<tl_fMaxAltitude;
  (*ostrFile)<<tl_fMaxAltitudeFade;
  (*ostrFile)<<tl_fMaxAltitudeNoise;
  (*ostrFile)<<tl_fMaxAltitudeRandom;

  (*ostrFile)<<tl_bApplyMinSlope;
  (*ostrFile)<<tl_fMinSlope;
  (*ostrFile)<<tl_fMinSlopeFade;
  (*ostrFile)<<tl_fMinSlopeNoise;
  (*ostrFile)<<tl_fMinSlopeRandom;

  (*ostrFile)<<tl_bApplyMaxSlope;
  (*ostrFile)<<tl_fMaxSlope;
  (*ostrFile)<<tl_fMaxSlopeFade;
  (*ostrFile)<<tl_fMaxSlopeNoise;
  (*ostrFile)<<tl_fMaxSlopeRandom;

  (*ostrFile)<<tl_colMultiply;
  (*ostrFile)<<tl_fSmoothness;
  (*ostrFile)<<(INDEX)tl_ltType;

  // Tile layer properties
  (*ostrFile)<<tl_ctTilesInRow;
  (*ostrFile)<<tl_ctTilesInCol;
  (*ostrFile)<<tl_iSelectedTile;
  (*ostrFile)<<tl_pixTileWidth;
  (*ostrFile)<<tl_pixTileHeight;
  (*ostrFile)<<tl_fTileU;
  (*ostrFile)<<tl_fTileV;
}

// Clear terrain layer
void CTerrainLayer::Clear()
{
  // if array of vertex colors was initialized
  if(tl_aubColors!=NULL) {
    // free array
    FreeMemory(tl_aubColors);
    tl_aubColors = NULL;
  }
  // if layer has valid texture 
  if(tl_ptdTexture!=NULL) {
    // release texture from stock
    _pTextureStock->Release(tl_ptdTexture);
    tl_ptdTexture = NULL;
  }

  // Clear tumbnail texture
  tl_tdThumbNail.Clear();
}

// Count used memory
SLONG CTerrainLayer::GetUsedMemory(void)
{
  SLONG slUsedMemory=0;
  slUsedMemory += sizeof(CTerrainLayer);
  slUsedMemory += sizeof(UBYTE)*tl_iMaskWidth*tl_iMaskHeight;
  return slUsedMemory;
}
