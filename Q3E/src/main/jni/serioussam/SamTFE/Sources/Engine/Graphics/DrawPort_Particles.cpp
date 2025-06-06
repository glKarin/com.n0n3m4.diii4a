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

#include <Engine/Graphics/DrawPort.h>

#include <Engine/Math/Projection.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Graphics/Vertex.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Fog_internal.h>
#include <Engine/Base/Statistics_Internal.h>

#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>


extern const FLOAT *pfSinTable;
extern const FLOAT *pfCosTable;

__extern CEntity *_Particle_penCurrentViewer = NULL;
__extern INDEX _Particle_iCurrentDrawPort = 0;
__extern FLOAT _Particle_fCurrentMip = 0.0f;
__extern BOOL  _Particle_bHasFog  = FALSE;
__extern BOOL  _Particle_bHasHaze = FALSE;

// variables used for rendering particles
static CProjection3D *_pprProjection;
static FLOAT _fPerspectiveFactor;
static MEX _mexTextureWidth, _mexTextureHeight;
static FLOAT _fTextureCorrectionU, _fTextureCorrectionV;
static FLOAT _fNearClipDistance;
static GFXTexCoord _atex[4];
static COLOR _colAttMask;
static BOOL _bTransFogHaze  = FALSE;
static BOOL _bNeedsClipping = FALSE;
static CDrawPort *_pDP;
static CStaticStackArray<GFXTexCoord> _atexFogHaze;
static CTextureData *_ptd = NULL;
static INDEX _iFrame = 0;



// prepare particles for rendering
void Particle_PrepareSystem( CDrawPort *pdpDrawPort, CAnyProjection3D &prProjection)
{
  _pDP = pdpDrawPort;
  _pprProjection = (CProjection3D*)&*prProjection;
  _fNearClipDistance = -prProjection->pr_NearClipDistance;
  _fPerspectiveFactor = 1.0f;
  _Particle_iCurrentDrawPort = pdpDrawPort->GetID();
  
  // prepare projection and scale factor
  pdpDrawPort->SetProjection(prProjection);
  if( prProjection.IsPerspective()) _fPerspectiveFactor = ((CPerspectiveProjection3D*)&*prProjection)->ppr_PerspectiveRatios(1);
  
  // setup rendering mode
  gfxEnableDepthTest();
  gfxCullFace(GFX_NONE);
  gfxEnableTexture();
  // prepare general texture parameters
  gfxSetTextureWrapping( GFX_REPEAT, GFX_REPEAT);
  // prepare arrays to draw from begining
  gfxResetArrays();
}

void Particle_EndSystem( BOOL bRestoreOrtho/*=TRUE*/)
{
  // reset projection and re-enable clipping
  if( bRestoreOrtho) _pDP->SetOrtho();
  gfxEnableClipping();
}


FLOAT Particle_GetMipFactor(void)
{
  return _Particle_fCurrentMip;
}

CEntity *Particle_GetViewer(void)
{
  return _Particle_penCurrentViewer;
}

CProjection3D *Particle_GetProjection(void)
{
  return _pprProjection;
}

INDEX Particle_GetDrawPortID(void)
{
  return _Particle_iCurrentDrawPort;
}


void Particle_PrepareTexture( CTextureObject *pto, enum ParticleBlendType pbt)
{
  // determine blend type
  switch( pbt) {
  case PBT_BLEND:
    gfxDisableDepthWrite();
    gfxDisableAlphaTest();
    gfxEnableBlend();
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    _colAttMask = 0xFFFFFF00; // attenuate alpha
    break;
  case PBT_ADD:
    gfxDisableDepthWrite();
    gfxDisableAlphaTest();
    gfxEnableBlend();
    gfxBlendFunc( GFX_ONE, GFX_ONE);
    _colAttMask = 0x000000FF; // attenuate color
    break;
  case PBT_MULTIPLY:
    gfxDisableDepthWrite();
    gfxDisableAlphaTest();
    gfxEnableBlend();
    gfxBlendFunc( GFX_ZERO, GFX_INV_SRC_COLOR);
    _colAttMask = 0x000000FF; // attenuate color
    break;
  case PBT_ADDALPHA:
    gfxDisableDepthWrite();
    gfxDisableAlphaTest();
    gfxEnableBlend();
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_ONE);
    _colAttMask = 0xFFFFFF00; // attenuate alpha
    break;
  case PBT_FLEX:
    gfxDisableDepthWrite();
    gfxDisableAlphaTest();
    gfxEnableBlend();
    gfxBlendFunc( GFX_ONE, GFX_INV_SRC_ALPHA);
    _colAttMask = 0xFFFFFFFF; // attenuate alpha
    break;
  case PBT_TRANSPARENT:
    gfxEnableDepthWrite();
    gfxEnableAlphaTest();
    gfxDisableBlend();
    _colAttMask = 0; // no attenuation - texture instead
    break;
  }
  // get texture parameters for current frame and needed mip factor
  _ptd = (CTextureData*)pto->GetData();
  _iFrame = pto->GetFrame();
  // prepare and upload texture
  _ptd->SetAsCurrent(_iFrame);
  // obtain curently used texture's width and height in mexes
  _mexTextureWidth  = _ptd->GetWidth();
  _mexTextureHeight = _ptd->GetHeight();
  // calculate correction factor (relative to greater texture dimension)
  _fTextureCorrectionU = 1.0f/_mexTextureWidth;
  _fTextureCorrectionV = 1.0f/_mexTextureHeight;
  _atexFogHaze.Push(4); // temporary
  _bTransFogHaze  = _colAttMask==0 && (_Particle_bHasFog || _Particle_bHasHaze);
  _bNeedsClipping = FALSE;
}


void Particle_SetTexturePart( MEX mexWidth, MEX mexHeight, INDEX iCol, INDEX iRow)
{
  // prepare full texture for displaying
  MEXaabbox2D boxTextureClipped( MEX2D( mexWidth*(iCol+0), mexHeight*(iRow+0)),
                                 MEX2D( mexWidth*(iCol+1), mexHeight*(iRow+1)));

  // prepare coordinates of the rectangle
  _atex[0].st.s = boxTextureClipped.Min()(1) *_fTextureCorrectionU;
  _atex[0].st.t = boxTextureClipped.Min()(2) *_fTextureCorrectionV;
  _atex[1].st.s = boxTextureClipped.Min()(1) *_fTextureCorrectionU;
  _atex[1].st.t = boxTextureClipped.Max()(2) *_fTextureCorrectionV;
  _atex[2].st.s = boxTextureClipped.Max()(1) *_fTextureCorrectionU;
  _atex[2].st.t = boxTextureClipped.Max()(2) *_fTextureCorrectionV;
  _atex[3].st.s = boxTextureClipped.Max()(1) *_fTextureCorrectionU;
  _atex[3].st.t = boxTextureClipped.Min()(2) *_fTextureCorrectionV;
}



// add one particle square to rendering queue
void Particle_RenderSquare( const FLOAT3D &vPos, FLOAT fSize, ANGLE aRotation, COLOR col, FLOAT fYRatio/*=1.0f*/)
{
  // trivial rejection
  if( fSize<0.0001f || ((col&CT_AMASK)>>CT_ASHIFT)<2) return;

  // project point to screen
  FLOAT3D vProjected;
  _pprProjection->PreClip( vPos, vProjected);
  // skip if not in screen
  const INDEX iTest = _pprProjection->TestSphereToFrustum( vProjected, fSize);
  if( iTest<0) return;
  const FLOAT fPixSize = fSize * _fPerspectiveFactor / vProjected(3);
  if( fPixSize<0.5f) return;

  // adjust the need for clipping
  if( iTest==0) _bNeedsClipping = TRUE;

  // eventual tex coords for fog or haze
  const INDEX ctTexFG = _atexFogHaze.Count();
  GFXTexCoord *ptexFogHaze = &_atexFogHaze[ctTexFG-4];

  // if haze is active
  if( _Particle_bHasHaze)
  { // get haze strength at particle position
    ptexFogHaze[0].st.s = (-vProjected(3)+_haze_fAdd)*_haze_fMul;
    const ULONG ulH = 255-GetHazeAlpha(ptexFogHaze[0].st.s);
    if( ulH<4) return;
    if( _colAttMask) { // apply haze color (if not transparent)
      const COLOR colH = _colAttMask | RGBAToColor( ulH,ulH,ulH,ulH);
      col = MulColors( col, colH);
    } else ptexFogHaze[0].st.t = 0;
  }
  // if fog is active
  if( _Particle_bHasFog)
  { // get fog strength at particle position
    ptexFogHaze[0].st.s = -vProjected(3)*_fog_fMulZ;
    ptexFogHaze[0].st.t = (vProjected%_fog_vHDirView+_fog_fAddH)*_fog_fMulH;
    const ULONG ulF = 255-GetFogAlpha(ptexFogHaze[0]);
    if( ulF<4) return;
    if( _colAttMask) { // apply fog color (if not transparent)
      const COLOR colF = _colAttMask | RGBAToColor( ulF,ulF,ulF,ulF);
      col = MulColors( col, colF);
    }
  }
  // keep fog/haze tex coords (if needed)
  if( _bTransFogHaze) {
    ptexFogHaze[1] = ptexFogHaze[2] = ptexFogHaze[3] = ptexFogHaze[0];
    _atexFogHaze.Push(4);
  }

  // prepare screen coords
  const FLOAT fI0  = vProjected(1);
  const FLOAT fJ0  = vProjected(2);
  const FLOAT fOoK = vProjected(3);

  // add to vertex arrays
  GFXVertex4  *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);

  // prepare vertices
  const FLOAT fRX = fSize;
  const FLOAT fRY = fSize*fYRatio;
  if( aRotation==0) {
    const FLOAT fIBeg = fI0-fRX;  const FLOAT fIEnd = fI0+fRX;
    const FLOAT fJBeg = fJ0-fRY;  const FLOAT fJEnd = fJ0+fRY;
    pvtx[0].x = fIBeg;  pvtx[0].y = fJBeg;  pvtx[0].z = fOoK;
    pvtx[1].x = fIBeg;  pvtx[1].y = fJEnd;  pvtx[1].z = fOoK;
    pvtx[2].x = fIEnd;  pvtx[2].y = fJEnd;  pvtx[2].z = fOoK;
    pvtx[3].x = fIEnd;  pvtx[3].y = fJBeg;  pvtx[3].z = fOoK;
  } else {
    const INDEX iRot256 = FloatToInt(aRotation*0.7111f) & 255; // *256/360
    const FLOAT fSinA = pfSinTable[iRot256];
    const FLOAT fCosA = pfCosTable[iRot256];
    const FLOAT fSinPCos = fCosA*fRX+fSinA*fRY;
    const FLOAT fSinMCos = fSinA*fRX-fCosA*fRY;
    pvtx[0].x = fI0-fSinPCos;  pvtx[0].y = fJ0-fSinMCos;  pvtx[0].z = fOoK;
    pvtx[1].x = fI0+fSinMCos;  pvtx[1].y = fJ0-fSinPCos;  pvtx[1].z = fOoK;
    pvtx[2].x = fI0+fSinPCos;  pvtx[2].y = fJ0+fSinMCos;  pvtx[2].z = fOoK;
    pvtx[3].x = fI0-fSinMCos;  pvtx[3].y = fJ0+fSinPCos;  pvtx[3].z = fOoK;
  }
  // prepare texture coords 
  ptex[0] = _atex[1];
  ptex[1] = _atex[0];
  ptex[2] = _atex[3];
  ptex[3] = _atex[2];
  // prepare colors
  const GFXColor glcol( AdjustColor( col, _slTexHueShift, _slTexSaturation));
  pcol[0] = glcol;
  pcol[1] = glcol;
  pcol[2] = glcol;
  pcol[3] = glcol;
}



// add one particle line to rendering queue
void Particle_RenderLine( const FLOAT3D &vPos0, const FLOAT3D &vPos1, FLOAT fWidth, COLOR col)
{
  // trivial rejection
  if( fWidth<0 || ((col&CT_AMASK)>>CT_ASHIFT)<2) return;

  // project point to screen
  FLOAT3D vProjected0, vProjected1;
  _pprProjection->PreClip( vPos0, vProjected0);
  _pprProjection->PreClip( vPos1, vProjected1);
  // skip if not in screen
  if (vProjected0(3)>_fNearClipDistance || vProjected1(3)>_fNearClipDistance) return;

  const FLOAT fK0 = 1.0f / vProjected0(3);
  const FLOAT fK1 = 1.0f / vProjected1(3);
  const FLOAT fR0 = fWidth * _fPerspectiveFactor *fK0;
  const FLOAT fR1 = fWidth * _fPerspectiveFactor *fK1;
  if( fR0<0.5f && fR1<0.5f) return;

  // line might need clipping
  _bNeedsClipping = TRUE;

  COLOR col0, col1;
  col0 = col1 = col;
  // eventual tex coords for fog or haze
  const INDEX ctTexFG = _atexFogHaze.Count();
  GFXTexCoord *ptexFogHaze = &_atexFogHaze[ctTexFG-4];

  // if haze is active
  if( _Particle_bHasHaze)
  { // get haze strength at particle positions
    ptexFogHaze[0].st.s = (-vProjected0(3)+_haze_fAdd)*_haze_fMul;
    ptexFogHaze[1].st.s = (-vProjected1(3)+_haze_fAdd)*_haze_fMul;
    const ULONG ulH0 = 255-GetHazeAlpha(ptexFogHaze[0].st.s);
    const ULONG ulH1 = 255-GetHazeAlpha(ptexFogHaze[1].st.s);
    if( (ulH0|ulH1)<4) return;
    if( _colAttMask) { // apply haze color (if not transparent)
      COLOR colH;
      colH = _colAttMask | RGBAToColor( ulH0,ulH0,ulH0,ulH0);  col0 = MulColors( col0, colH);
      colH = _colAttMask | RGBAToColor( ulH1,ulH1,ulH1,ulH1);  col1 = MulColors( col1, colH);
    } else ptexFogHaze[0].st.t = ptexFogHaze[1].st.t = 0;
  }
  // if fog is active
  if( _Particle_bHasFog)
  { // get fog strength at particle position
    ptexFogHaze[0].st.s = -vProjected0(3)*_fog_fMulZ;
    ptexFogHaze[0].st.t = (vProjected0%_fog_vHDirView+_fog_fAddH)*_fog_fMulH;
    ptexFogHaze[1].st.s = -vProjected1(3)*_fog_fMulZ;
    ptexFogHaze[1].st.t = (vProjected1%_fog_vHDirView+_fog_fAddH)*_fog_fMulH;
    const ULONG ulF0 = 255-GetFogAlpha(ptexFogHaze[0]);
    const ULONG ulF1 = 255-GetFogAlpha(ptexFogHaze[1]);
    if( (ulF0|ulF1)<4) return;
    if( _colAttMask) { // apply fog color (if not transparent)
      COLOR colF; // apply fog color
      colF = _colAttMask | RGBAToColor( ulF0,ulF0,ulF0,ulF0);  col0 = MulColors( col0, colF);
      colF = _colAttMask | RGBAToColor( ulF1,ulF1,ulF1,ulF1);  col1 = MulColors( col1, colF);
    }
  }
  // keep fog/haze tex coords (if needed)
  if( _bTransFogHaze) {
    ptexFogHaze[2] = ptexFogHaze[1];
    ptexFogHaze[3] = ptexFogHaze[0];
    _atexFogHaze.Push(4);
  }

  // lets draw
  const FLOAT fI0   = vProjected0(1);  const FLOAT fI1   = vProjected1(1);
  const FLOAT fJ0   = vProjected0(2);  const FLOAT fJ1   = vProjected1(2);
  const FLOAT fOoK0 = vProjected0(3);  const FLOAT fOoK1 = vProjected1(3);
  FLOAT fDI = fI1*fK1 - fI0*fK0;
  FLOAT fDJ = fJ1*fK1 - fJ0*fK0;
  const FLOAT fD = fWidth / Sqrt( fDI*fDI + fDJ*fDJ);
  fDI *= fD;  // multiplied by width!
  fDJ *= fD;

  // add to vertex arrays
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);

  // prepare vertices
  pvtx[0].x = fI0+fDJ;  pvtx[0].y = fJ0-fDI;  pvtx[0].z = fOoK0;
  pvtx[1].x = fI1+fDJ;  pvtx[1].y = fJ1-fDI;  pvtx[1].z = fOoK1;
  pvtx[2].x = fI1-fDJ;  pvtx[2].y = fJ1+fDI;  pvtx[2].z = fOoK1;
  pvtx[3].x = fI0-fDJ;  pvtx[3].y = fJ0+fDI;  pvtx[3].z = fOoK0;
  // prepare texture coords 
  ptex[0] = _atex[0];
  ptex[1] = _atex[1];
  ptex[2] = _atex[2];
  ptex[3] = _atex[3];
  // prepare colors
  const GFXColor glcol0( AdjustColor( col0, _slTexHueShift, _slTexSaturation));
  const GFXColor glcol1( AdjustColor( col1, _slTexHueShift, _slTexSaturation));
  pcol[0] = glcol0;
  pcol[1] = glcol1;
  pcol[2] = glcol1;
  pcol[3] = glcol0;
}



// add one 3D particle quad to rendering queue
void Particle_RenderQuad3D( const FLOAT3D &vPos0, const FLOAT3D &vPos1, const FLOAT3D &vPos2,
                            const FLOAT3D &vPos3, COLOR col)
{
  // trivial rejection
  if( ((col&CT_AMASK)>>CT_ASHIFT)<2) return;

  // project point to screen
  FLOAT3D vProjected0, vProjected1, vProjected2, vProjected3;
  _pprProjection->PreClip( vPos0, vProjected0);
  _pprProjection->PreClip( vPos1, vProjected1);
  _pprProjection->PreClip( vPos2, vProjected2);
  _pprProjection->PreClip( vPos3, vProjected3);

  // test for trivial rejection (sphere method)
  FLOAT3D vNearest = vProjected0; // find nearest-Z vertex
  if( vNearest(3)>vProjected1(3)) vNearest = vProjected1;
  if( vNearest(3)>vProjected2(3)) vNearest = vProjected2;
  if( vNearest(3)>vProjected3(3)) vNearest = vProjected3;
  // find center
  const FLOAT fX = (vProjected0(1)+vProjected1(1)+vProjected2(1)+vProjected3(1)) * 0.25f;
  const FLOAT fY = (vProjected0(2)+vProjected1(2)+vProjected2(2)+vProjected3(2)) * 0.25f;
  // find radius (approx. distance to nearest-Z vertex)
  // we won't do sqrt but rather larger distance * 0.7f (1/sqrt(2))
  const FLOAT fDX = Abs(fX-vNearest(1));
  const FLOAT fDY = Abs(fY-vNearest(2));
  const FLOAT fR  = 0.7f * Max(fDX,fDY);
  // set center vertex location and test it
  vNearest(1) = fX;
  vNearest(2) = fY;
  const INDEX iTest = _pprProjection->TestSphereToFrustum( vNearest, fR);
  if( iTest<0) return;

  // adjust the need for clipping
  if( iTest==0) _bNeedsClipping = TRUE;

  // separate colors (for the sake of fog/haze)
  COLOR col0,col1,col2,col3;
  col0 = col1 = col2 = col3 = col;
  // eventual tex coords for fog or haze
  const INDEX ctTexFG = _atexFogHaze.Count();
  GFXTexCoord *ptexFogHaze = &_atexFogHaze[ctTexFG-4];

  // if haze is active
  if( _Particle_bHasHaze)
  { // get haze strength at particle position
    ptexFogHaze[0].st.s = (-vProjected0(3)+_haze_fAdd)*_haze_fMul;
    ptexFogHaze[1].st.s = (-vProjected1(3)+_haze_fAdd)*_haze_fMul;
    ptexFogHaze[2].st.s = (-vProjected2(3)+_haze_fAdd)*_haze_fMul;
    ptexFogHaze[3].st.s = (-vProjected3(3)+_haze_fAdd)*_haze_fMul;
    const ULONG ulH0 = 255-GetHazeAlpha(ptexFogHaze[0].st.s);
    const ULONG ulH1 = 255-GetHazeAlpha(ptexFogHaze[1].st.s);
    const ULONG ulH2 = 255-GetHazeAlpha(ptexFogHaze[2].st.s);
    const ULONG ulH3 = 255-GetHazeAlpha(ptexFogHaze[3].st.s);
    if( (ulH0|ulH1|ulH2|ulH3)<4) return;
    if( _colAttMask) { // apply haze color (if not transparent)
      COLOR colH;
      colH = _colAttMask | RGBAToColor( ulH0,ulH0,ulH0,ulH0);  col0 = MulColors( col0, colH);
      colH = _colAttMask | RGBAToColor( ulH1,ulH1,ulH1,ulH1);  col1 = MulColors( col1, colH);
      colH = _colAttMask | RGBAToColor( ulH2,ulH2,ulH2,ulH2);  col2 = MulColors( col2, colH);
      colH = _colAttMask | RGBAToColor( ulH3,ulH3,ulH3,ulH3);  col3 = MulColors( col3, colH);
    } else ptexFogHaze[0].st.t = ptexFogHaze[1].st.t = ptexFogHaze[2].st.t = ptexFogHaze[3].st.t = 0;
  }
  // if fog is active
  if( _Particle_bHasFog)
  { // get fog strength at particle position
    ptexFogHaze[0].st.s = -vProjected0(3)*_fog_fMulZ;
    ptexFogHaze[0].st.t = (vProjected0%_fog_vHDirView+_fog_fAddH)*_fog_fMulH;
    ptexFogHaze[1].st.s = -vProjected1(3)*_fog_fMulZ;
    ptexFogHaze[1].st.t = (vProjected1%_fog_vHDirView+_fog_fAddH)*_fog_fMulH;
    ptexFogHaze[2].st.s = -vProjected2(3)*_fog_fMulZ;
    ptexFogHaze[2].st.t = (vProjected2%_fog_vHDirView+_fog_fAddH)*_fog_fMulH;
    ptexFogHaze[3].st.s = -vProjected3(3)*_fog_fMulZ;
    ptexFogHaze[3].st.t = (vProjected3%_fog_vHDirView+_fog_fAddH)*_fog_fMulH;
    const ULONG ulF0 = 255-GetFogAlpha(ptexFogHaze[0]);
    const ULONG ulF1 = 255-GetFogAlpha(ptexFogHaze[1]);
    const ULONG ulF2 = 255-GetFogAlpha(ptexFogHaze[2]);
    const ULONG ulF3 = 255-GetFogAlpha(ptexFogHaze[3]);
    if( (ulF0|ulF1|ulF2|ulF3)<4) return;
    if( _colAttMask) { // apply fog color (if not transparent)
      COLOR colF; 
      colF = _colAttMask | RGBAToColor( ulF0,ulF0,ulF0,ulF0);  col0 = MulColors( col0, colF);
      colF = _colAttMask | RGBAToColor( ulF1,ulF1,ulF1,ulF1);  col1 = MulColors( col1, colF);
      colF = _colAttMask | RGBAToColor( ulF2,ulF2,ulF2,ulF2);  col2 = MulColors( col2, colF);
      colF = _colAttMask | RGBAToColor( ulF3,ulF3,ulF3,ulF3);  col3 = MulColors( col3, colF);
    }
  }
  // keep fog/haze tex coords (if needed)
  if( _bTransFogHaze) _atexFogHaze.Push(4);

  // add to vertex arrays
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);

  // prepare vertices
  pvtx[0].x = vProjected0(1);  pvtx[0].y = vProjected0(2);  pvtx[0].z = vProjected0(3);
  pvtx[1].x = vProjected1(1);  pvtx[1].y = vProjected1(2);  pvtx[1].z = vProjected1(3);
  pvtx[2].x = vProjected2(1);  pvtx[2].y = vProjected2(2);  pvtx[2].z = vProjected2(3);
  pvtx[3].x = vProjected3(1);  pvtx[3].y = vProjected3(2);  pvtx[3].z = vProjected3(3);
  // prepare texture coords 
  ptex[0] = _atex[0];
  ptex[1] = _atex[1];
  ptex[2] = _atex[2];
  ptex[3] = _atex[3];
  // prepare colors
  const GFXColor glcol0( AdjustColor( col0, _slTexHueShift, _slTexSaturation));
  const GFXColor glcol1( AdjustColor( col1, _slTexHueShift, _slTexSaturation));
  const GFXColor glcol2( AdjustColor( col2, _slTexHueShift, _slTexSaturation));
  const GFXColor glcol3( AdjustColor( col3, _slTexHueShift, _slTexSaturation));
  pcol[0] = glcol0;
  pcol[1] = glcol1;
  pcol[2] = glcol2;
  pcol[3] = glcol3;
}


// flushes particle rendering queue (i.e. renders particle on screen)
void Particle_Flush(void)
{
  // update stats
  const INDEX ctParticles = _avtxCommon.Count()/4;
  _sfStats.IncrementCounter( CStatForm::SCI_PARTICLES, ctParticles);
  _pGfx->gl_ctParticleTriangles += ctParticles*2;

  // determine need for clipping
  if( _bNeedsClipping) gfxEnableClipping();
  else gfxDisableClipping();

  // flush 1st layer
  gfxFlushQuads();
  // maybe we need to render fog/haze layer
  if( _bTransFogHaze)
  { // setup haze/fog color and texture
    GFXColor glcolFH;        
    gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
    if( _Particle_bHasHaze) {
      gfxSetTexture( _haze_ulTexture, _haze_tpLocal);
      glcolFH.ul.abgr = ByteSwap( AdjustColor( _haze_hp.hp_colColor, _slTexHueShift, _slTexSaturation));
    } else {
      gfxSetTexture( _fog_ulTexture, _fog_tpLocal);
      glcolFH.ul.abgr = ByteSwap( AdjustColor( _fog_fp.fp_colColor, _slTexHueShift, _slTexSaturation));
    }
    // prepare haze rendering parameters
    gfxDisableAlphaTest();
    gfxEnableBlend();
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    gfxDisableDepthWrite();
    gfxDepthFunc( GFX_EQUAL); // adjust z-buffer compare
    // copy fog/haze texture array to main texture array and set color to fog/haze
    const INDEX ctVertices = _atexCommon.Count();
    ASSERT( _atexFogHaze.Count()==ctVertices+4);
    memcpy( &_atexCommon[0], &_atexFogHaze[0], ctVertices*sizeof(GFXTexCoord)); 
    for( INDEX i=0; i<ctVertices; i++) _acolCommon[i] = glcolFH;
    // render it
    gfxFlushQuads();
    // revert to old settings
    gfxEnableAlphaTest();
    gfxDisableBlend();
    gfxDepthFunc( GFX_LESS_EQUAL);
   _ptd->SetAsCurrent(_iFrame);
   _pGfx->gl_ctParticleTriangles += ctParticles*2;
  }

  // all done
  gfxResetArrays();
  _atexFogHaze.PopAll();
  _bNeedsClipping = FALSE;
}



// SORTING ROUTINES

static int qsort_CompareZ( const void *pI0, const void *pI1) {
  const INDEX i0 = (*(INDEX*)pI0) *4;
  const INDEX i1 = (*(INDEX*)pI1) *4;
  const FLOAT fZ0 = _avtxCommon[i0].z;
  const FLOAT fZ1 = _avtxCommon[i1].z;
       if( fZ0<fZ1) return +1;
  else if( fZ0>fZ1) return -1;
  else              return  0;
}

static int qsort_CompareZ3D( const void *pI0, const void *pI1) {
  const INDEX i0 = (*(INDEX*)pI0) *4;
  const INDEX i1 = (*(INDEX*)pI1) *4;
  const FLOAT fZ0 = (_avtxCommon[i0].z + _avtxCommon[i0+1].z + _avtxCommon[i0+2].z + _avtxCommon[i0+3].z) / 4.0f;
  const FLOAT fZ1 = (_avtxCommon[i1].z + _avtxCommon[i1+1].z + _avtxCommon[i1+2].z + _avtxCommon[i1+3].z) / 4.0f;
       if( fZ0<fZ1) return +1;
  else if( fZ0>fZ1) return -1;
  else              return  0;
}


// sorts particles by distance
void Particle_Sort( BOOL b3D/*=FALSE*/)
{
  INDEX i;
  const INDEX ctParticles = _avtxCommon.Count()/4; 
  if( ctParticles<=0) return; // nothing to do!

  // generate sort array
  CStaticArray<INDEX> aiIndices;
  aiIndices.New(ctParticles);
  for( i=0; i<ctParticles; i++) aiIndices[i] = i;

  // bubble sort indices by vertex Z coord
  if(b3D) qsort( &aiIndices[0], ctParticles, sizeof(INDEX), qsort_CompareZ3D);
  else    qsort( &aiIndices[0], ctParticles, sizeof(INDEX), qsort_CompareZ);

  // generate inverse table
  CStaticArray<INDEX> aiInverse;
  aiInverse.New(ctParticles);
  for( i=0; i<ctParticles; i++) {
    const INDEX iOrig = aiIndices[i];
    aiInverse[iOrig] = i;
  }

  // sort vertices by indices
  for( i=0; i<ctParticles;) // i is incremented in loop
  { // fetch destination
    INDEX &iWhere = aiInverse[i];
    ASSERT( iWhere<ctParticles);
    // if current is already in place, advance to next index
    if( iWhere==i) { i++; continue; }
    // swap vertices
    Swap( _avtxCommon[iWhere*4+0], _avtxCommon[i*4+0]);
    Swap( _avtxCommon[iWhere*4+1], _avtxCommon[i*4+1]);
    Swap( _avtxCommon[iWhere*4+2], _avtxCommon[i*4+2]);
    Swap( _avtxCommon[iWhere*4+3], _avtxCommon[i*4+3]);
    // swap texture coords
    Swap( _atexCommon[iWhere*4+0], _atexCommon[i*4+0]);
    Swap( _atexCommon[iWhere*4+1], _atexCommon[i*4+1]);
    Swap( _atexCommon[iWhere*4+2], _atexCommon[i*4+2]);
    Swap( _atexCommon[iWhere*4+3], _atexCommon[i*4+3]);
    // swap colors
    Swap( _acolCommon[iWhere*4+0], _acolCommon[i*4+0]);
    Swap( _acolCommon[iWhere*4+1], _acolCommon[i*4+1]);
    Swap( _acolCommon[iWhere*4+2], _acolCommon[i*4+2]);
    Swap( _acolCommon[iWhere*4+3], _acolCommon[i*4+3]);
    // swap indices
    Swap( aiInverse[iWhere], aiInverse[i]);
  }

#ifndef NDEBUG
  // test to see whether the array is sorted
  INDEX      *pidx = &aiInverse[0];
  GFXVertex4 *pvtx = &_avtxCommon[0];
  for( i=0; i<ctParticles-1; i++) {
    ASSERT( pidx[i] < pidx[i+1]);
    ASSERT( pvtx[i*4].z >= pvtx[(i+1)*4].z);
  }
#endif
}
