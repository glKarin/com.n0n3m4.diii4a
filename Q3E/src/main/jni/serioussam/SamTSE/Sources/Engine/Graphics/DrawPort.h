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

#ifndef SE_INCL_DRAWPORT_H
#define SE_INCL_DRAWPORT_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>


class ENGINE_API CDrawPort {
// implementation:
public:
  CListNode	dp_NodeInRaster;	   // for linking in list of clones
public:
  class CRaster *dp_Raster;	     // pointer to the Raster this refers to
  class CFontData *dp_FontData;  // pointer to the current text font
  PIX   dp_Width, dp_Height;		 // width, height in pixels
  PIX   dp_MinI, dp_MinJ;		     // I,J coord of the upper left edge in the Raster
  PIX   dp_MaxI, dp_MaxJ;		     // I,J coord of the lower right edge in the Raster
  PIX   dp_ScissorMinI, dp_ScissorMinJ;	// I,J coord of the upper left edge in the Raster
  PIX   dp_ScissorMaxI, dp_ScissorMaxJ;	// I,J coord of the lower right edge in the Raster
  PIX	  dp_pixTextCharSpacing;	 // space between chars in text
  PIX	  dp_pixTextLineSpacing;	 // space between lines in text
  FLOAT dp_fTextScaling;				 // scaling factor for font size
  FLOAT dp_fTextAspect;				   // aspect ratio for font (x/y)
  INDEX dp_iTextMode;            // text output mode (-1 = print special codes, 0 = ignore special codes, 1 = parse special codes)
  FLOAT dp_fWideAdjustment;      // for wide (16:9 or 16:10) screen support (needed in some calculations) = 1.0f or >1 (10/12 perhaps)
  BOOL  dp_bRenderingOverlay;    // set when scene renderer requires overlay mode (don't clear z-buffer)

  // dimensions and position relative to the raster size
  double dp_SizeIOverRasterSizeI, dp_SizeJOverRasterSizeJ; 
  double dp_MinIOverRasterSizeI,  dp_MinJOverRasterSizeJ;

  // adjust this during frame to be used for screen blending
  ULONG dp_ulBlendingRA, dp_ulBlendingGA, dp_ulBlendingBA; // r*a, g*a, b*a
  ULONG dp_ulBlendingA;

  // set cloned drawport dimensions
  void InitCloned(CDrawPort *pdpBase, double rMinI,double rMinJ, double rSizeI,double rSizeJ);

  // Recalculate pixel dimensions from relative dimensions and raster size
  void RecalculateDimensions(void);

  // set orthogonal projection
  void SetOrtho(void) const;
  // set given projection
  void SetProjection(CAnyProjection3D &apr) const;

//interface:
  // Create a drawport for full raster
  CDrawPort( CRaster *praBase=NULL);
  // Clone a drawport
  CDrawPort( CDrawPort *pdpBase, double rMinI,double rMinJ, double rSizeI,double rSizeJ);
  CDrawPort( CDrawPort *pdpBase, const PIXaabbox2D &box);
  // dualhead cloning
  CDrawPort( CDrawPort *pdpBase, BOOL bLeft);

  //  wide-screen cloning
  void MakeWideScreen(CDrawPort *pdp);

  // check if a drawport is dualhead
  BOOL IsDualHead(void);
  // check if a drawport is wide screen
  BOOL IsWideScreen(void);

  // set/get rendering in overlay mode
  inline void SetOverlappedRendering(BOOL bOverlay) { dp_bRenderingOverlay = bOverlay; };
  inline BOOL IsOverlappedRendering(void) const     { return dp_bRenderingOverlay; }; 

  // returns unique drawport number
  ULONG GetID(void);

  // Get dimensions and location of drawport
  inline PIX GetWidth( void) const { return dp_Width;  };
  inline PIX GetHeight(void) const { return dp_Height; };

  // text manipulation
  void SetFont( CFontData *pfd);   // WARNING: this resets text variables
  inline void SetTextCharSpacing( PIX pixSpacing) { dp_pixTextCharSpacing = pixSpacing; };
  inline void SetTextLineSpacing( PIX pixSpacing) { dp_pixTextLineSpacing = pixSpacing; };
  inline void SetTextScaling( FLOAT fScalingFactor) { dp_fTextScaling = fScalingFactor; };
  inline void SetTextAspect(  FLOAT fAspectRatio)   { dp_fTextAspect  = fAspectRatio; };
  inline void SetTextMode(    INDEX iMode)          { dp_iTextMode    = iMode; };
  // returns width of entire text string (with scale included)
  ULONG GetTextWidth( const CTString &strText) const; 
   
  // writes text string on drawport (left-aligned)
  void PutText(    const CTString &strText, PIX pixX0, PIX pixY0, const COLOR colBlend=0xFFFFFFFF) const;
  // writes text string on drawport (centered arround X)
  void PutTextC(   const CTString &strText, PIX pixX0, PIX pixY0, const COLOR colBlend=0xFFFFFFFF) const;
  // writes text string on drawport (centered arround X and Y)
  void PutTextCXY( const CTString &strText, PIX pixX0, PIX pixY0, const COLOR colBlend=0xFFFFFFFF) const;
  // writes text string on drawport (right-aligned)
  void PutTextR(   const CTString &strText, PIX pixX0, PIX pixY0, const COLOR colBlend=0xFFFFFFFF) const;

  // plain texture display
  void PutTexture( class CTextureObject *pTO, const PIXaabbox2D &boxScreen,
                   const COLOR colBlend=0xFFFFFFFF) const;
  void PutTexture( class CTextureObject *pTO, const PIXaabbox2D &boxScreen,
                   const COLOR colUL, const COLOR colUR, const COLOR colDL, const COLOR colDR) const;
  void PutTexture( class CTextureObject *pTO, const PIXaabbox2D &boxScreen,
                   const MEXaabbox2D &boxTexture, const COLOR colBlend=0xFFFFFFFF) const;
  void PutTexture( class CTextureObject *pTO, const PIXaabbox2D &boxScreen, const MEXaabbox2D &boxTexture,
                   const COLOR colUL, const COLOR colUR, const COLOR colDL, const COLOR colDR) const;

  // advanced texture display
  void InitTexture( class CTextureObject *pTO, const BOOL bClamp=FALSE) const; // prepares texture and rendering arrays
  // adds one full texture to rendering queue
  void AddTexture( const FLOAT fI0, const FLOAT fJ0, const FLOAT fI1, const FLOAT fJ1, const COLOR col) const; 
  // adds one part of texture to rendering queue
  void AddTexture( const FLOAT fI0, const FLOAT fJ0, const FLOAT fI1, const FLOAT fJ1, 
                   const FLOAT fU0, const FLOAT fV0, const FLOAT fU1, const FLOAT fV1, const COLOR col) const;
  // adds one textured quad to rendering queue (up-left start, counter-clockwise)
  void AddTexture( const FLOAT fI0, const FLOAT fJ0, const FLOAT fU0, const FLOAT fV0, const COLOR col0,
                   const FLOAT fI1, const FLOAT fJ1, const FLOAT fU1, const FLOAT fV1, const COLOR col1,
                   const FLOAT fI2, const FLOAT fJ2, const FLOAT fU2, const FLOAT fV2, const COLOR col2,
                   const FLOAT fI3, const FLOAT fJ3, const FLOAT fU3, const FLOAT fV3, const COLOR col3) const;
  // adds one flat triangle rendering queue (up-left start, counter-clockwise)
  void AddTriangle( const FLOAT fI0, const FLOAT fJ0, const FLOAT fI1, const FLOAT fJ1,
                    const FLOAT fI2, const FLOAT fJ2, const COLOR col) const; 

  // renders all textures from rendering queue and flushed rendering arrays
  void FlushRenderingQueue(void) const; 

  // lock and unlock raster thru drawport functions
  BOOL Lock(void);
  void Unlock(void);

  // draw point (can be several pixels - depends on radius)
  void DrawPoint( PIX pixI, PIX pixJ, COLOR col, PIX pixRadius=1) const;
  void DrawPoint3D( FLOAT3D v, COLOR col, FLOAT fRadius=1.0f) const; // in 3D
  // draw line
  void DrawLine( PIX pixI0, PIX pixJ0, PIX pixI1, PIX pixJ1, COLOR col, ULONG typ=_FULL_) const;
  void DrawLine3D( FLOAT3D v0, FLOAT3D v1, COLOR col) const; // in 3D
  // draw border
  void DrawBorder( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight, COLOR col, ULONG typ=_FULL_) const;

  // fill with blending part of a drawport with a given color
  void Fill( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight, COLOR col) const;
  // fill with blending part of a drawport with a four corner colors
  void Fill( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight,
             COLOR colUL, COLOR colUR, COLOR colDL, COLOR colDR) const;
  // fill a part of Z-Buffer with a given value
  void FillZBuffer( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight, FLOAT zval) const;

  // fill without blending an entire drawport with a given color
  void Fill( COLOR col) const;
  // fill an entire Z-Buffer with a given value
  void FillZBuffer( FLOAT zval) const;

  // grab screen (iGrabZBuffer: 0=no, 1=if allowed, 2=yes)
  void GrabScreen( class CImageInfo &iiGrabbedImage, INDEX iGrabZBuffer=0) const;
  // functions for getting depth of points on drawport
  BOOL IsPointVisible( PIX pixI, PIX pixJ, FLOAT fOoK, INDEX iID, INDEX iMirrorLevel=0) const;
  // render one lens flare
  void RenderLensFlare( CTextureObject *pto, FLOAT fI, FLOAT fJ,
                        FLOAT fSizeI, FLOAT fSizeJ, ANGLE aRotation, COLOR colLight) const;

  // blend entire drawport with accumulated colors
  void BlendScreen(void);
};


enum ParticleBlendType {
  PBT_BLEND,
  PBT_ADD,
  PBT_MULTIPLY,
  PBT_ADDALPHA,
  PBT_FLEX, // premultiplied alpha (ONE + [1-SRC_ALPHA])
  PBT_TRANSPARENT,
};

void  ENGINE_API Particle_PrepareSystem( CDrawPort *pdpDrawPort, CAnyProjection3D &prProjection);
void  ENGINE_API Particle_PrepareEntity( FLOAT fMipFactor, BOOL bHasFog, BOOL bHasHaze, CEntity *penViewer);
void  ENGINE_API Particle_EndSystem( BOOL bRestoreOrtho=TRUE);
FLOAT ENGINE_API Particle_GetMipFactor(void);
INDEX ENGINE_API Particle_GetDrawPortID(void);
ENGINE_API CEntity *Particle_GetViewer(void);
ENGINE_API CProjection3D *Particle_GetProjection(void);
void ENGINE_API Particle_PrepareTexture( CTextureObject *pto, enum ParticleBlendType pbt);
void ENGINE_API Particle_SetTexturePart( MEX mexWidth, MEX mexHeight, INDEX iCol, INDEX iRow);
void ENGINE_API Particle_RenderSquare( const FLOAT3D &vPos, FLOAT fSize, ANGLE aRotation, COLOR col, FLOAT fYRatio=1.0f);
void ENGINE_API Particle_RenderQuad3D( const FLOAT3D &vPos0, const FLOAT3D &vPos1, const FLOAT3D &vPos2,
                                       const FLOAT3D &vPos3, COLOR col);
void ENGINE_API Particle_RenderLine( const FLOAT3D &vPos0, const FLOAT3D &vPos1, FLOAT fWidth, COLOR col);
void ENGINE_API Particle_Sort( BOOL b3D=FALSE);
void ENGINE_API Particle_Flush(void);



#endif  /* include-once check. */

