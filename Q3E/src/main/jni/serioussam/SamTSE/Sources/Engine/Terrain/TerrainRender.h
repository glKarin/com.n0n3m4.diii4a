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

#ifndef SE_INCL_TERRAIN_RENDER_H
#define SE_INCL_TERRAIN_RENDER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// Prepare scene for terrain render
void PrepareScene(CAnyProjection3D &apr, CDrawPort *pdp, CTerrain *ptrTerrain);
// Render one terrain
void RenderTerrain(void);
// Render one terrain in wireframe mode
void RenderTerrainWire(COLOR &colEdges);
// Regenerate terrain tile
void ReGenerateTile(INDEX itt);
// Draw terrain quad tree
void DrawQuadTree(void);
// Draw box in wireframe
void gfxDrawWireBox(FLOATaabbox3D &bbox, COLOR col);
// Draw selected vertices
void DrawSelectedVertices(GFXVertex *pavVertices, GFXColor *pacolColors, INDEX ctVertices);

SLONG GetUsedMemoryForTileBatching(void);

#endif
