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

#ifndef SE_INCL_RENDERPOLY_H
#define SE_INCL_RENDERPOLY_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// general structure
struct PolyVertex2D
{
  FLOAT pv2_fI, pv2_fJ;     // DrawPort 2D coords
  FLOAT pv2_f1oK;           // Viewpoint (camera) distance (z-buffer value)
  FLOAT pv2_fUoK, pv2_fVoK; // texture coordinates (for perspective mapping)
};

// functions for rendering triangles
extern void SetTriangleTexture( ULONG *pulCurrentMipmap, PIX pixMipWidth, PIX pixMipHeight);

extern void DrawTriangle_Mask( UBYTE *pubMaskPlane, SLONG slMaskWidth, SLONG slMaskHeight,
                               struct PolyVertex2D *ppv2Vtx1, struct PolyVertex2D *ppv2Vtx2,
                               struct PolyVertex2D *ppv2Vtx3, BOOL bTransparency);

// has model cast some cluster shadow (shadow renderer needs this)
extern BOOL _bSomeDarkExists;



#endif  /* include-once check. */

