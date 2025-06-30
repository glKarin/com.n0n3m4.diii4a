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

#ifndef SE_INCL_TERRAIN_MISC_H
#define SE_INCL_TERRAIN_MISC_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Terrain/Terrain.h>
// Extract polygons in given box and returns clipped rectangle
Rect ExtractPolygonsInBox(CTerrain *ptrTerrain, const FLOATaabbox3D &bbox, GFXVertex4 **pavVtx,
                          INDEX_T **paiInd, INDEX &ctVtx,INDEX &ctInd,BOOL bFixSize=FALSE);

void ExtractVerticesInRect(CTerrain *ptrTerrain, Rect &rc, GFXVertex4 **pavVtx, 
                          INDEX_T **paiInd, INDEX &ctVtx,INDEX &ctInd);


// check whether a polygon is below given point, but not too far away
BOOL IsTerrainBelowPoint(CTerrain *ptrTerrain, const FLOAT3D &vPoint, FLOAT fMaxDist, const FLOAT3D &vGravityDir);

// Extract all tiles that intersect with given box
void FindTilesInBox(CTerrain *ptrTerrain, FLOATaabbox3D &bbox);
// Add these flags to all tiles that have been extracted
void AddFlagsToExtractedTiles(ULONG ulFlags);

// Create new texture
void CreateTexture(CTextureData &td, INDEX iWidth, INDEX iHeight, ULONG ulFlags);

void CreateTopMap(CTextureData &tdTopMap, PIX pixWidth , PIX pixHeight);
void PrepareSharedTopMapMemory(CTextureData *ptdTopMap, INDEX iTileIndex);
void FreeSharedTopMapMemory(CTextureData *ptdTopMap, INDEX iTileIndex);

void ReGenerateTopMap(CTerrain *ptrTerrain, INDEX iTileIndex);

// Update terrain shadow map
void UpdateTerrainShadowMap(CTerrain *ptrTerrain, FLOATaabbox3D *pboxUpdate=NULL, BOOL bAbsoluteSpace=FALSE);

// Get value from layer at given point
ENGINE_API UBYTE GetValueFromMask(CTerrain *ptrTerrain, INDEX iLayer, FLOAT3D vHitPoint);
// Calculate 2d relative point in terrain from absolute 3d point in world
ENGINE_API Point Calculate2dHitPoint(CTerrain *ptrTerrain, FLOAT3D &vHitPoint);
FLOAT2D CalculateShadingTexCoords(CTerrain *ptrTerrain, FLOAT3D &vPoint);





#endif
