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

#ifndef SE_INCL_TERRAIN_EDITING_H
#define SE_INCL_TERRAIN_EDITING_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Terrain/Terrain.h>

enum EEditingTool {
  ET_NONE         = 0,
  ET_HEIGHT_TOOL  = 1,
  ET_SMOTH_TOOL   = 2,
  ET_EQUALIZE     = 3,
  ET_ERASE        = 4,
  ET_FLATERN      = 5,
  ET_NOISE        = 6,
};

enum BufferType {
  BT_INVALID   = 0,
  BT_HEIGHT_MAP,
  BT_LAYER_MASK,
  BT_EDGE_MAP,
};

enum SelectionFill {
  SF_POLYGON = 0,
  SF_WIREFRAME,
  SF_POINTS
};
void ShowBrushSelection(void);

ENGINE_API void EditTerrain(CTerrain *ptrTerrain, CTextureData *ptdBrush, FLOAT3D &vHitPoint, 
                 COLOR colColor, FLOAT fStrength, EEditingTool etTool, INDEX iLayerIndex = -1);

void ShowSelectionInternal(CTerrain *ptrTerrain);

ENGINE_API UWORD *GetBufferForEditing(CTerrain *ptrTerrain, Rect &rcExtract, BufferType btBufferType, INDEX iBufferData=-1);
ENGINE_API void   SetBufferForEditing(CTerrain *ptrTerrain, UWORD *puwEditedBuffer, Rect &rcExtract, BufferType btBufferType, INDEX iBufferData=-1);
ENGINE_API void   ShowSelection(CTerrain *ptrTerrain, Rect &rcExtract, CTextureData *ptdBrush, COLOR colSelection, FLOAT fStrenght, SelectionFill sfFill=SF_POLYGON);

#endif
