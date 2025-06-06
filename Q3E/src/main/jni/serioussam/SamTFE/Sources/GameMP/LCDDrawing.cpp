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

#include "StdAfx.h"
#include "LCDDrawing.h"

static CTextureObject _toPointer;
static CTextureObject _toBcgClouds;
static CTextureObject _toBcgGrid;
static PIX _pixSizeI;
static PIX _pixSizeJ;
static PIXaabbox2D _boxScreen;
static FLOAT _tmNow;
static ULONG _ulA;

// rcg11162001 Made static to fix duplicate symbol resolution issue.
static CDrawPort *_pdp = NULL;

extern void _LCDInit(void)
{
  try {
    _toBcgClouds.SetData_t(CTFILENAME("Textures\\General\\Background6.tex"));
    _toBcgGrid.SetData_t(CTFILENAME("Textures\\General\\Grid16x16-dot.tex"));
    _toPointer.SetData_t(CTFILENAME("Textures\\General\\Pointer.tex"));
  } catch (const char *strError) {
    FatalError("%s\n", strError);
  }
}

extern void _LCDEnd(void)
{
  _toBcgClouds.SetData(NULL);
  _toBcgGrid.SetData(NULL);
  _toPointer.SetData(NULL);
}

extern void _LCDPrepare(FLOAT fFade)
{
  // get current time and alpha value
  _tmNow = (FLOAT)_pTimer->GetHighPrecisionTimer().GetSeconds();
  _ulA   = NormFloatToByte(fFade);
}

extern void _LCDSetDrawport(CDrawPort *pdp)
{
  _pdp = pdp;
  _pixSizeI = _pdp->GetWidth();
  _pixSizeJ = _pdp->GetHeight();
  _boxScreen = PIXaabbox2D ( PIX2D(0,0), PIX2D(_pixSizeI, _pixSizeJ));
}

void TiledTexture( PIXaabbox2D &_boxScreen, FLOAT fStretch, const MEX2D &vScreen, MEXaabbox2D &boxTexture)
{
  PIX pixW = _boxScreen.Size()(1);
  PIX pixH = _boxScreen.Size()(2);
  boxTexture = MEXaabbox2D(MEX2D(0, 0), MEX2D(pixW/fStretch, pixH/fStretch));
  boxTexture+=vScreen;
}

extern void _LCDDrawBox(PIX pixUL, PIX pixDR, const PIXaabbox2D &box, COLOR col)
{
  // up
  _pdp->DrawLine(
    box.Min()(1)-pixUL, box.Min()(2)-pixUL, 
    box.Max()(1)+pixDR, box.Min()(2)-pixUL, col);
  // down
  _pdp->DrawLine(
    box.Min()(1)-pixUL, box.Max()(2)+pixDR, 
    box.Max()(1)+pixDR, box.Max()(2)+pixDR, col);
  // left
  _pdp->DrawLine(
    box.Min()(1)-pixUL, box.Min()(2)-pixUL, 
    box.Min()(1)-pixUL, box.Max()(2)+pixDR, col);
  // right
  _pdp->DrawLine(
    box.Max()(1)+pixDR, box.Min()(2)-pixUL, 
    box.Max()(1)+pixDR, box.Max()(2)+pixDR+1, col);
}

extern void _LCDScreenBoxOpenLeft(COLOR col)
{
  // up
  _pdp->DrawLine(
    _boxScreen.Min()(1)-1, _boxScreen.Min()(2), 
    _boxScreen.Max()(1)-1, _boxScreen.Min()(2), col);
  // down
  _pdp->DrawLine(
    _boxScreen.Min()(1)-1, _boxScreen.Max()(2)-1, 
    _boxScreen.Max()(1)-1, _boxScreen.Max()(2)-1, col);
  // right
  _pdp->DrawLine(
    _boxScreen.Max()(1)-1, _boxScreen.Min()(2), 
    _boxScreen.Max()(1)-1, _boxScreen.Max()(2)-1+1, col);
}

extern void _LCDScreenBoxOpenRight(COLOR col)
{
  // up
  _pdp->DrawLine(
    _boxScreen.Min()(1)-1, _boxScreen.Min()(2), 
    _boxScreen.Max()(1)-1, _boxScreen.Min()(2), col);
  // down
  _pdp->DrawLine(
    _boxScreen.Min()(1)-1, _boxScreen.Max()(2)-1, 
    _boxScreen.Max()(1)-1, _boxScreen.Max()(2)-1, col);
  // left
  _pdp->DrawLine(
    _boxScreen.Min()(1), _boxScreen.Min()(2), 
    _boxScreen.Min()(1), _boxScreen.Max()(2)-1+1, col);
}

extern void _LCDScreenBox(COLOR col)
{
  _LCDDrawBox(0,-1, _boxScreen, col);
}

extern void _LCDRenderClouds1(void)
{
  MEXaabbox2D boxBcgClouds1;
  TiledTexture(_boxScreen, 1.3f*_pdp->GetWidth()/640.0f, 
    MEX2D(sin(_tmNow*0.75f)*50,sin(_tmNow*0.9f)*40),   boxBcgClouds1);
  _pdp->PutTexture(&_toBcgClouds, _boxScreen, boxBcgClouds1, C_dGREEN|_ulA>>1);
  TiledTexture(_boxScreen, 0.8f*_pdp->GetWidth()/640.0f, 
    MEX2D(sin(_tmNow*0.95f)*50,sin(_tmNow*0.8f)*40),   boxBcgClouds1);
  _pdp->PutTexture(&_toBcgClouds, _boxScreen, boxBcgClouds1, C_dGREEN|_ulA>>1);
}

extern void _LCDRenderClouds2(void)
{
  MEXaabbox2D boxBcgClouds2;
  TiledTexture(_boxScreen, 0.5f*_pdp->GetWidth()/640.0f,
    MEX2D(2,10), boxBcgClouds2);
  _pdp->PutTexture(&_toBcgClouds, _boxScreen, boxBcgClouds2, C_BLACK|(_ulA>>1));
}

extern void _LCDRenderClouds2Light(void)
{
  MEXaabbox2D boxBcgClouds2;
  TiledTexture(_boxScreen, 1.7f*_pdp->GetWidth()/640.0f,
    MEX2D(2,10), boxBcgClouds2);
  _pdp->PutTexture(&_toBcgClouds, _boxScreen, boxBcgClouds2, C_BLACK|(_ulA>>1));
}

extern void _LCDRenderGrid(void)
{
  MEXaabbox2D boxBcgGrid;
  TiledTexture(_boxScreen, 1.0f, MEX2D(0,0),   boxBcgGrid);
  _pdp->PutTexture(&_toBcgGrid,   _boxScreen, boxBcgGrid, C_dGREEN|_ulA);
}

/*
extern void _LCDRenderClouds1(void)
{
  MEXaabbox2D boxBcgClouds1 = MEXaabbox2D(MEX2D(0,0), MEX2D(256,512));
  MEXaabbox2D boxBcgClouds2 = MEXaabbox2D(MEX2D(0,0), MEX2D(512,256));
  boxBcgClouds1 += MEX2D( sin(_tmNow*0.45f)*50, sin(_tmNow*0.65f)*40);
  boxBcgClouds2 += MEX2D( sin(_tmNow*0.55f)*50, sin(_tmNow*0.35f)*40);
  _pdp->PutTexture( &_toBcgClouds, _boxScreen, boxBcgClouds1, C_dGREEN|(_ulA>>1));
  _pdp->PutTexture( &_toBcgClouds, _boxScreen, boxBcgClouds2, C_dGREEN|(_ulA>>1));
}

extern void _LCDRenderClouds2(void)
{
  MEXaabbox2D boxBcgClouds = MEXaabbox2D(MEX2D(0,0), MEX2D(512,512));
  boxBcgClouds += MEX2D(2,10);
  _pdp->PutTexture( &_toBcgClouds, _boxScreen, boxBcgClouds, C_BLACK|(_ulA>>1));
}

extern void _LCDRenderClouds2Light(void)
{
  MEXaabbox2D boxBcgClouds2;
  TiledTexture( _boxScreen, 1.3f, MEX2D(2,10), boxBcgClouds2);
  _pdp->PutTexture( &_toBcgClouds, _boxScreen, boxBcgClouds2, C_BLACK|(_ulA>>1));
}

extern void _LCDRenderGrid(void)
{
  MEXaabbox2D boxBcgGrid;
  TiledTexture( _boxScreen, 1.0f, MEX2D(8,8), boxBcgGrid);
  _pdp->PutTexture( &_toBcgGrid,   _boxScreen, boxBcgGrid, C_dGREEN|_ulA);
}
*/

extern COLOR _LCDGetColor(COLOR colDefault, const char *strName)
{
  return colDefault;//||((colDefault&0xFF0000)<<8);
}

extern COLOR _LCDFadedColor(COLOR col)
{
  return MulColors(C_WHITE|_ulA, col);
}

extern COLOR _LCDBlinkingColor(COLOR col0, COLOR col1)
{
  return LerpColor( col0, col1, sin(_tmNow*10.0f)*0.5f+0.5f);
}

extern void _LCDDrawPointer(PIX pixI, PIX pixJ)
{
  CDisplayMode dmCurrent;
  _pGfx->GetCurrentDisplayMode(dmCurrent);
  if (dmCurrent.IsFullScreen()) {
    while (ShowCursor(FALSE) >= 0);
  } else {
    if (!_pInput->IsInputEnabled()) {
      while (ShowCursor(TRUE) < 0);
    }
    return;
  }
  PIX pixSizeI = _toPointer.GetWidth();
  PIX pixSizeJ = _toPointer.GetHeight();
  pixI-=1;
  pixJ-=1;
  _pdp->PutTexture( &_toPointer, PIXaabbox2D( PIX2D(pixI, pixJ), PIX2D(pixI+pixSizeI, pixJ+pixSizeJ)),
                    _LCDFadedColor(C_WHITE|255));
}

