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

#include <Engine/Base/Console.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/Raster.h>
#include <Engine/Graphics/ViewPort.h>

#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>


extern INDEX gap_iOptimizeDepthReads;
#ifdef SE1_D3D
extern COLOR UnpackColor_D3D( UBYTE *pd3dColor, D3DFORMAT d3dFormat, SLONG &slColorSize);
#endif // SE1_D3D

static INDEX _iCheckIteration = 0;
static CTimerValue _tvLast[8];  // 8 is max mirror recursion

#define KEEP_BEHIND 8

// info of one point for delayed depth buffer lookup
struct DepthInfo {
  INDEX di_iID;               // unique identifier
  PIX   di_pixI, di_pixJ;     // last requested coordinates
  FLOAT di_fOoK;              // last requested depth
  INDEX di_iSwapLastRequest;  // index of swap when last requested
  INDEX di_iMirrorLevel;      // level of mirror recursion in which flare is
  BOOL  di_bVisible;          // whether the point was visible
};
CStaticStackArray<DepthInfo> _adiDelayed;  // active delayed points
// don't ask, these are  for D3D
CStaticStackArray<CTVERTEX> _avtxDelayed;  
CStaticStackArray<COLOR>    _acolDelayed;  

// read depth buffer and update visibility flag of depth points
static void UpdateDepthPointsVisibility( const CDrawPort *pdp, const INDEX iMirrorLevel,
                                         DepthInfo *pdi, const INDEX ctCount)
{
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT(GfxValidApi(eAPI));
  ASSERT( pdp!=NULL && ctCount>0);
  const CRaster *pra = pdp->dp_Raster;

  // OpenGL
  if( eAPI==GAT_OGL)
  { 
#if !defined(_GLES) //karin: not support read depth buffer
    _sfStats.StartTimer(CStatForm::STI_GFXAPI);
    FLOAT fPointOoK;
    // for each stored point
    for( INDEX idi=0; idi<ctCount; idi++) {
      DepthInfo &di = pdi[idi];
      // skip if not in required mirror level or was already checked in this iteration
      if( iMirrorLevel!=di.di_iMirrorLevel || _iCheckIteration!=di.di_iSwapLastRequest) continue;
      const PIX pixJ = pra->ra_Height-1 - di.di_pixJ; // OpenGL has Y-inversed buffer!
      pglReadPixels( di.di_pixI, pixJ, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &fPointOoK);
      OGL_CHECKERROR;
      // it is visible if there is nothing nearer in z-buffer already
      di.di_bVisible = (di.di_fOoK<fPointOoK);
    }
    // done
    _sfStats.StopTimer(CStatForm::STI_GFXAPI);
#endif
    return;
  }

  // Direct3D
#ifdef SE1_D3D
  if( eAPI==GAT_D3D)
  {
    _sfStats.StartTimer(CStatForm::STI_GFXAPI);
    // ok, this will get really complicated ...
    // We'll have to do it thru back buffer because darn DX8 won't let us have values from z-buffer;
    // Anyway, we'll lock backbuffer, read color from the lens location and try to write little triangle there
    // with slightly modified color. Then we'll readout that color and see if triangle passes z-test. Voila! :)
    // P.S. To avoid lock-modify-lock, we need to batch all the locks in one. Uhhhh ... :(
    COLOR col;
    INDEX idi;
    SLONG slColSize;
    HRESULT hr;
    D3DLOCKED_RECT rectLocked;
    D3DSURFACE_DESC surfDesc;
    LPDIRECT3DSURFACE8 pBackBuffer;
    // fetch back buffer (different for full screen and windowed mode)
    const BOOL bFullScreen = _pGfx->gl_ulFlags & GLF_FULLSCREEN;
    if( bFullScreen) {
      hr = _pGfx->gl_pd3dDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    } else {
      hr = pra->ra_pvpViewPort->vp_pSwapChain->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    }
    // what, cannot get a back buffer?
    if( hr!=D3D_OK) { 
      // to hell with it all
      _sfStats.StopTimer(CStatForm::STI_GFXAPI);
      return;
    }
    // keep format of back-buffer
    pBackBuffer->GetDesc(&surfDesc);
    const D3DFORMAT d3dfBack = surfDesc.Format;
    
    // prepare array that'll back-buffer colors from depth point locations
    _acolDelayed.Push(ctCount);
    // store all colors
    for( idi=0; idi<ctCount; idi++) {
      DepthInfo &di = pdi[idi];
      // skip if not in required mirror level or was already checked in this iteration
      if( iMirrorLevel!=di.di_iMirrorLevel || _iCheckIteration!=di.di_iSwapLastRequest) continue;
      // fetch pixel
      _acolDelayed[idi] = 0;
      const RECT rectToLock = { di.di_pixI, di.di_pixJ, di.di_pixI+1, di.di_pixJ+1 };
      hr = pBackBuffer->LockRect( &rectLocked, &rectToLock, D3DLOCK_READONLY);
      if( hr!=D3D_OK) continue; // skip if lock didn't make it
      // read, convert and store original color
      _acolDelayed[idi] = UnpackColor_D3D( (UBYTE*)rectLocked.pBits, d3dfBack, slColSize) | CT_OPAQUE;
      pBackBuffer->UnlockRect();
    }

    // prepare to draw little triangles there with slightly adjusted colors
    _sfStats.StopTimer(CStatForm::STI_GFXAPI);
    gfxEnableDepthTest();
    gfxDisableDepthWrite();
    gfxDisableBlend();
    gfxDisableAlphaTest();
    gfxDisableTexture();
    _sfStats.StartTimer(CStatForm::STI_GFXAPI);
    // prepare array and shader
    _avtxDelayed.Push(ctCount*3);
    d3dSetVertexShader(D3DFVF_CTVERTEX);

    // draw one trianle around each depth point
    INDEX ctVertex = 0;
    for( idi=0; idi<ctCount; idi++) {
      DepthInfo &di = pdi[idi];
      col = _acolDelayed[idi];
      // skip if not in required mirror level or was already checked in this iteration, or wasn't fetched at all
      if( iMirrorLevel!=di.di_iMirrorLevel || _iCheckIteration!=di.di_iSwapLastRequest || col==0) continue;
      const ULONG d3dCol = rgba2argb(col^0x20103000);
      const PIX pixI = di.di_pixI - pdp->dp_MinI; // convert raster loc to drawport loc
      const PIX pixJ = di.di_pixJ - pdp->dp_MinJ;
      // batch it and advance to next triangle
      CTVERTEX &vtx0 = _avtxDelayed[ctVertex++];
      CTVERTEX &vtx1 = _avtxDelayed[ctVertex++];
      CTVERTEX &vtx2 = _avtxDelayed[ctVertex++];
      vtx0.fX=pixI;   vtx0.fY=pixJ-2; vtx0.fZ=di.di_fOoK; vtx0.ulColor=d3dCol; vtx0.fU=vtx0.fV=0;
      vtx1.fX=pixI-2; vtx1.fY=pixJ+2; vtx1.fZ=di.di_fOoK; vtx1.ulColor=d3dCol; vtx1.fU=vtx0.fV=0;
      vtx2.fX=pixI+2; vtx2.fY=pixJ;   vtx2.fZ=di.di_fOoK; vtx2.ulColor=d3dCol; vtx2.fU=vtx0.fV=0;
    }
    // draw a bunch
    hr = _pGfx->gl_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLELIST, ctVertex/3, &_avtxDelayed[0], sizeof(CTVERTEX));
    D3D_CHECKERROR(hr);

    // readout colors again and compare to old ones
    for( idi=0; idi<ctCount; idi++) {
      DepthInfo &di = pdi[idi];
      col = _acolDelayed[idi];
      // skip if not in required mirror level or was already checked in this iteration, or wasn't fetched at all
      if( iMirrorLevel!=di.di_iMirrorLevel || _iCheckIteration!=di.di_iSwapLastRequest || col==0) continue;
      // fetch pixel
      const RECT rectToLock = { di.di_pixI, di.di_pixJ, di.di_pixI+1, di.di_pixJ+1 };
      hr = pBackBuffer->LockRect( &rectLocked, &rectToLock, D3DLOCK_READONLY);
      if( hr!=D3D_OK) continue; // skip if lock didn't make it
      // read new color
      const COLOR colNew = UnpackColor_D3D( (UBYTE*)rectLocked.pBits, d3dfBack, slColSize) | CT_OPAQUE;
      pBackBuffer->UnlockRect();
      // if we managed to write adjusted color, point is visible!
      di.di_bVisible = (col!=colNew);
    }
    // phew, done! :)
    D3DRELEASE( pBackBuffer, TRUE);
    _acolDelayed.PopAll();
    _avtxDelayed.PopAll();
    _sfStats.StopTimer(CStatForm::STI_GFXAPI);
    return;
  }
#endif // SE1_D3D
}



// check point against depth buffer
extern BOOL CheckDepthPoint( const CDrawPort *pdp, PIX pixI, PIX pixJ, FLOAT fOoK, INDEX iID, INDEX iMirrorLevel/*=0*/)
{
  // no raster?
  const CRaster *pra = pdp->dp_Raster;
  if( pra==NULL) return FALSE;
  // almoust out of raster?
  pixI += pdp->dp_MinI;
  pixJ += pdp->dp_MinJ;
  if( pixI<1 || pixJ<1 || pixI>pra->ra_Width-2 || pixJ>pra->ra_Height-2) return FALSE;

  // if shouldn't delay
  if( gap_iOptimizeDepthReads==0) {
    // just check immediately
    DepthInfo di = { iID, pixI, pixJ, fOoK, _iCheckIteration, iMirrorLevel, FALSE };
    UpdateDepthPointsVisibility( pdp, iMirrorLevel, &di, 1);
    return di.di_bVisible;
  }

  // for each stored point
  for( INDEX idi=0; idi<_adiDelayed.Count(); idi++) {
    DepthInfo &di = _adiDelayed[idi];
    // if same id
    if( di.di_iID == iID) {
      // remember parameters
      di.di_pixI = pixI;
      di.di_pixJ = pixJ;
      di.di_fOoK = fOoK;
      di.di_iSwapLastRequest = _iCheckIteration;
      // return visibility
      return di.di_bVisible;
    }
  }
  // if not found...

  // create new one
  DepthInfo &di = _adiDelayed.Push();
  // remember parameters
  di.di_iID  = iID;
  di.di_pixI = pixI;
  di.di_pixJ = pixJ;
  di.di_fOoK = fOoK;
  di.di_iSwapLastRequest = _iCheckIteration;
  di.di_iMirrorLevel = iMirrorLevel;
  di.di_bVisible = FALSE;
  // not visible by default
  return FALSE;
}


// check all delayed depth points
extern void CheckDelayedDepthPoints( const CDrawPort *pdp, INDEX iMirrorLevel/*=0*/)
{
  // skip if not delayed or mirror level is to high
  gap_iOptimizeDepthReads = Clamp( gap_iOptimizeDepthReads, (INDEX)0, (INDEX)2);
  if( gap_iOptimizeDepthReads==0 || iMirrorLevel>7) return; 
  ASSERT( pdp!=NULL && iMirrorLevel>=0);

  // check only if time lapse allows
  const CTimerValue tvNow = _pTimer->GetLowPrecisionTimer();
  const TIME tmDelta = (tvNow-_tvLast[iMirrorLevel]).GetSeconds();
  ASSERT( tmDelta>=0);
  if( gap_iOptimizeDepthReads==2 && tmDelta<0.1f) return;

  // prepare
  _tvLast[iMirrorLevel] = tvNow;
  INDEX ctPoints = _adiDelayed.Count();
  if( ctPoints==0) return; // done if no points in queue

  // for each point
  INDEX iPoint = 0;
  while( iPoint<ctPoints) {
    DepthInfo &di = _adiDelayed[iPoint];
    // if the point is not active any more
    if( iMirrorLevel==di.di_iMirrorLevel && di.di_iSwapLastRequest<_iCheckIteration-KEEP_BEHIND) {
      // delete it by moving the last one on its place
      di = _adiDelayed[ctPoints-1];
      ctPoints--;
    // if the point is still active
    } else {
      // go to next point
      iPoint++;
    }
  }

  // remove unused points at the end
  if( ctPoints==0) _adiDelayed.PopAll();
  else _adiDelayed.PopUntil(ctPoints-1);

  // ignore stalls
  if( tmDelta>1.0f) return;

  // check and update visibility of what has left
  ASSERT( ctPoints == _adiDelayed.Count());
  if( ctPoints>0) UpdateDepthPointsVisibility( pdp, iMirrorLevel, &_adiDelayed[0], ctPoints);

  // mark checking
  _iCheckIteration++;
}

