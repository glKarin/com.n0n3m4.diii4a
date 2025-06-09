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

#ifdef SE1_D3D


// helper for keep user clip plane of D3D happy (i.e. updated as view matrix changes)
static void UpdateClipPlane_D3D(void)
{
  FLOAT afObjectClipPlane[4];

  // if view matrix is active
  if( GFX_bViewMatrix) {
    // need to back-transform user clip plane from view to object space
    FLOAT *pcp = &D3D_afClipPlane[0]; // (pl-v) * !m;
    FLOAT *pvm = &D3D_afViewMatrix[0];
    afObjectClipPlane[0] = pcp[0]*pvm[0] + pcp[1]*pvm[1] + pcp[2]*pvm[2];
    afObjectClipPlane[1] = pcp[0]*pvm[4] + pcp[1]*pvm[5] + pcp[2]*pvm[6];
    afObjectClipPlane[2] = pcp[0]*pvm[8] + pcp[1]*pvm[9] + pcp[2]*pvm[10];
    afObjectClipPlane[3] = pcp[3] + (pcp[0]*pvm[12] + pcp[1]*pvm[13] + pcp[2]*pvm[14]);
  } else {
    // just copy clip plane
    (ULONG&)afObjectClipPlane[0] = (ULONG&)D3D_afClipPlane[0];
    (ULONG&)afObjectClipPlane[1] = (ULONG&)D3D_afClipPlane[1];
    (ULONG&)afObjectClipPlane[2] = (ULONG&)D3D_afClipPlane[2];
    (ULONG&)afObjectClipPlane[3] = (ULONG&)D3D_afClipPlane[3];
  }
  // skip if the same as last time
  ULONG *pulThis = (ULONG*) afObjectClipPlane;
  ULONG *pulLast = (ULONG*)_afActiveClipPlane;
  if( pulLast[0]==pulThis[0] && pulLast[1]==pulThis[1]
   && pulLast[2]==pulThis[2] && pulLast[3]==pulThis[3]) return;
  // update (if supported!)
  if( _pGfx->gl_ulFlags&GLF_D3D_CLIPPLANE) {
    HRESULT hr = _pGfx->gl_pd3dDevice->SetClipPlane( 0, &afObjectClipPlane[0]);
    D3D_CHECKERROR(hr);
  }
  // keep it as current
  pulLast[0] = pulThis[0];
  pulLast[1] = pulThis[1];
  pulLast[2] = pulThis[2];
  pulLast[3] = pulThis[3];
}



// ENABLE/DISABLE FUNCTIONS


static void d3d_EnableTexture(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetTextureStageState( GFX_iActiveTexUnit, D3DTSS_COLOROP, (DWORD*)&bRes);
  if( bRes==D3DTOP_DISABLE) bRes = FALSE;
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_abTexture[GFX_iActiveTexUnit]);
#endif

  // cached?
  if( GFX_abTexture[GFX_iActiveTexUnit] && gap_bOptimizeStateChanges) return;
  GFX_abTexture[GFX_iActiveTexUnit] = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  D3DTEXTUREOP d3dTexOp = (GFX_iTexModulation[GFX_iActiveTexUnit]==2) ? D3DTOP_MODULATE2X : D3DTOP_MODULATE;
  hr = _pGfx->gl_pd3dDevice->SetTextureStageState( GFX_iActiveTexUnit, D3DTSS_COLOROP, d3dTexOp);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_DisableTexture(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetTextureStageState( GFX_iActiveTexUnit, D3DTSS_COLOROP, (DWORD*)&bRes);
  if( bRes==D3DTOP_DISABLE) bRes = FALSE;
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_abTexture[GFX_iActiveTexUnit]);
#endif

  // cached?
  if( !GFX_abTexture[GFX_iActiveTexUnit] && gap_bOptimizeStateChanges) return;
  GFX_abTexture[GFX_iActiveTexUnit] = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetTextureStageState( GFX_iActiveTexUnit, D3DTSS_COLOROP, D3DTOP_DISABLE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_EnableDepthTest(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_ZENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bDepthTest);
#endif
  // cached?
  if( GFX_bDepthTest && gap_bOptimizeStateChanges) return;
  GFX_bDepthTest = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_DisableDepthTest(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_ZENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bDepthTest);
#endif

  // cached?
  if( !GFX_bDepthTest && gap_bOptimizeStateChanges) return;
  GFX_bDepthTest = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_EnableDepthBias(void)
{
  // only if supported
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  if( !(_pGfx->gl_ulFlags&GLF_D3D_ZBIAS)) return;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  HRESULT hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_ZBIAS, 2);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_DisableDepthBias(void)
{
  // only if supported
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  if( !(_pGfx->gl_ulFlags&GLF_D3D_ZBIAS)) return;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  HRESULT hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_ZBIAS, 0);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_EnableDepthWrite(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_ZWRITEENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bDepthWrite);
#endif

  // cached?
  if( GFX_bDepthWrite && gap_bOptimizeStateChanges) return;
  GFX_bDepthWrite = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_DisableDepthWrite(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_ZWRITEENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bDepthWrite);
#endif

  // cached?
  if( !GFX_bDepthWrite && gap_bOptimizeStateChanges) return;
  GFX_bDepthWrite = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_EnableDither(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_DITHERENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bDithering);
#endif

  // cached?
  if( GFX_bDithering && gap_bOptimizeStateChanges) return;
  GFX_bDithering = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE, TRUE);
  D3D_CHECKERROR(hr);
  
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_DisableDither(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_DITHERENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bDithering);
#endif

  // cached?
  if( !GFX_bDithering && gap_bOptimizeStateChanges) return;
  GFX_bDithering = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE, FALSE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_EnableAlphaTest(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_ALPHATESTENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bAlphaTest);
#endif

  // cached?
  if( GFX_bAlphaTest && gap_bOptimizeStateChanges) return;
  GFX_bAlphaTest = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_DisableAlphaTest(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_ALPHATESTENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bAlphaTest);
#endif

  // cached?
  if( !GFX_bAlphaTest && gap_bOptimizeStateChanges) return;
  GFX_bAlphaTest = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE);
  D3D_CHECKERROR(hr);
 
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_EnableBlend(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_ALPHABLENDENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bBlending);
#endif
  // cached?
  if( GFX_bBlending && gap_bOptimizeStateChanges) return;
  GFX_bBlending = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);

  // adjust dithering
  if( gap_iDithering==2) d3d_EnableDither();
  else d3d_DisableDither();
}



static void d3d_DisableBlend(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_ALPHABLENDENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bBlending);
#endif

  // cached?
  if( !GFX_bBlending && gap_bOptimizeStateChanges) return;
  GFX_bBlending = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);

  // adjust dithering
  if( gap_iDithering==0) d3d_DisableDither();
  else d3d_EnableDither();
}



static void d3d_EnableClipping(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes = GFX_bClipping;
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_CLIPPING, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bClipping);
#endif

  // cached?
  if( GFX_bClipping && gap_bOptimizeStateChanges) return;
  GFX_bClipping = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  _bWantsClipping = TRUE; // need to signal for custom clip plane
  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_CLIPPING, TRUE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_DisableClipping(void)
{
  // only if allowed
  if( gap_iOptimizeClipping<2) return;

  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes;
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_CLIPPING, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bClipping);
#endif

  // cached?
  if( !GFX_bClipping && gap_bOptimizeStateChanges) return;
  GFX_bClipping = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // cannot disable clipping if clip-plane is enabled!
  if( GFX_bClipPlane) {
    GFX_bClipping  = TRUE;
   _bWantsClipping = FALSE;
  } else {
    hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_CLIPPING, FALSE);
    D3D_CHECKERROR(hr);
  }
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_EnableClipPlane(void)
{
  // only if supported
  if( !(_pGfx->gl_ulFlags&GLF_D3D_CLIPPLANE)) return;

  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_CLIPPLANEENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bClipPlane);
#endif

  // cached?
  if( GFX_bClipPlane && gap_bOptimizeStateChanges) return;
  GFX_bClipPlane = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0);
  D3D_CHECKERROR(hr);
  // (eventually) update clip plane, too
  UpdateClipPlane_D3D();
  // D3D needs to have clipping enabled for that matter
  if( !GFX_bClipping) {
    hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_CLIPPING, TRUE);
    D3D_CHECKERROR(hr);
    GFX_bClipping  = TRUE;
   _bWantsClipping = FALSE;
  }
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_DisableClipPlane(void)
{
  // only if supported
  if( !(_pGfx->gl_ulFlags&GLF_D3D_CLIPPLANE)) return;

  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes = GFX_bClipPlane; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_CLIPPLANEENABLE, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  ASSERT( !bRes == !GFX_bClipPlane);
#endif
  // cached?
  if( !GFX_bClipPlane && gap_bOptimizeStateChanges) return;
  GFX_bClipPlane = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_CLIPPLANEENABLE, FALSE);
  D3D_CHECKERROR(hr);
  // maybe we can disable clipping in general (if was kept enabled just beacuse of clip plane)
  if( !_bWantsClipping && GFX_bClipping) {
    GFX_bClipping = FALSE;
    hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_CLIPPING, FALSE);
    D3D_CHECKERROR(hr);
  }
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_EnableColorArray(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_LIGHTING, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  bRes = !bRes;
  ASSERT( !bRes == !GFX_bColorArray);
#endif

  // cached?
  if( GFX_bColorArray && gap_bOptimizeStateChanges) return;
  GFX_bColorArray = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// enable usage of constant color for subsequent rendering
static void d3d_DisableColorArray(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_LIGHTING, (DWORD*)&bRes);
  D3D_CHECKERROR(hr);
  bRes = !bRes;
  ASSERT( !bRes == !GFX_bColorArray);
#endif

  // cached?
  if( !GFX_bColorArray && gap_bOptimizeStateChanges) return;
  GFX_bColorArray = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_EnableTruform(void)
{
  // skip if Truform isn't set
  if( truform_iLevel<1) return;

  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
  FLOAT fSegments;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_PATCHSEGMENTS, (DWORD*)&fSegments);
  D3D_CHECKERROR(hr);
  bRes = (fSegments>1) ? TRUE : FALSE;
  ASSERT( !bRes == !GFX_bTruform);
#endif

  if( GFX_bTruform && gap_bOptimizeStateChanges) return;
  GFX_bTruform = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  fSegments = truform_iLevel+1;
  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_PATCHSEGMENTS, *((DWORD*)&fSegments));
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_DisableTruform(void)
{
  // skip if Truform isn't set
  if( truform_iLevel<1) return;

  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
  FLOAT fSegments;
#ifndef NDEBUG
  BOOL bRes; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_PATCHSEGMENTS, (DWORD*)&fSegments);
  D3D_CHECKERROR(hr);
  bRes = (fSegments>1) ? TRUE : FALSE;
  ASSERT( !bRes == !GFX_bTruform);
#endif

  if( !GFX_bTruform && gap_bOptimizeStateChanges) return;
  GFX_bTruform = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  fSegments = 1.0f;
  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_PATCHSEGMENTS, *((DWORD*)&fSegments));
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}




__forceinline _D3DBLEND BlendToD3D( GfxBlend eFunc) {
  switch( eFunc) {
  case GFX_ZERO:          return D3DBLEND_ZERO;
  case GFX_ONE:           return D3DBLEND_ONE;
  case GFX_SRC_COLOR:     return D3DBLEND_SRCCOLOR;
  case GFX_INV_SRC_COLOR: return D3DBLEND_INVSRCCOLOR;
  case GFX_DST_COLOR:     return D3DBLEND_DESTCOLOR;
  case GFX_INV_DST_COLOR: return D3DBLEND_INVDESTCOLOR;
  case GFX_SRC_ALPHA:     return D3DBLEND_SRCALPHA;
  case GFX_INV_SRC_ALPHA: return D3DBLEND_INVSRCALPHA;
  default: ASSERTALWAYS("Invalid GFX blending function!");
  } return D3DBLEND_ONE;
}

__forceinline GfxBlend BlendFromD3D( _D3DBLEND d3dbFunc) {
  switch( d3dbFunc) {
  case D3DBLEND_ZERO:         return GFX_ZERO;
  case D3DBLEND_ONE:          return GFX_ONE;          
  case D3DBLEND_SRCCOLOR:     return GFX_SRC_COLOR;    
  case D3DBLEND_INVSRCCOLOR:  return GFX_INV_SRC_COLOR;
  case D3DBLEND_DESTCOLOR:    return GFX_DST_COLOR;    
  case D3DBLEND_INVDESTCOLOR: return GFX_INV_DST_COLOR;
  case D3DBLEND_SRCALPHA:     return GFX_SRC_ALPHA;    
  case D3DBLEND_INVSRCALPHA:  return GFX_INV_SRC_ALPHA;
  default: ASSERTALWAYS("Unsupported D3D blending function!");
  } return GFX_ONE;
}



// set blending operations
static void d3d_BlendFunc( GfxBlend eSrc, GfxBlend eDst)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  GfxBlend  gfxSrc, gfxDst; 
  _D3DBLEND d3dSrc, d3dDst;
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_SRCBLEND,  (DWORD*)&d3dSrc);  D3D_CHECKERROR(hr);
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_DESTBLEND, (DWORD*)&d3dDst);  D3D_CHECKERROR(hr);
  gfxSrc = BlendFromD3D(d3dSrc);
  gfxDst = BlendFromD3D(d3dDst);
  ASSERT( gfxSrc==GFX_eBlendSrc && gfxDst==GFX_eBlendDst);
#endif
  // cached?
  if( eSrc==GFX_eBlendSrc && eDst==GFX_eBlendDst && gap_bOptimizeStateChanges) return;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  if( eSrc!=GFX_eBlendSrc) {
   _D3DBLEND d3dSrc = BlendToD3D(eSrc);
    hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, d3dSrc);
    D3D_CHECKERROR(hr);
    GFX_eBlendSrc = eSrc;
  }
  if( eDst!=GFX_eBlendDst) {
   _D3DBLEND d3dDst = BlendToD3D(eDst);
    hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, d3dDst);
    D3D_CHECKERROR(hr);
    GFX_eBlendDst = eDst;
  }
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_SetColorMask( ULONG ulColorMask)
{
  // only if supported
  _ulCurrentColorMask = ulColorMask; // keep for Get...()
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  if( !(_pGfx->gl_ulFlags&GLF_D3D_COLORWRITES))
  { // emulate disabling of all channels
    if( ulColorMask==0) {
      d3d_EnableBlend();
      d3d_BlendFunc( GFX_ZERO, GFX_ONE);
    } // done
    return;
  }

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // no emulation
  ULONG ulBitMask = NONE;
  if( (ulColorMask&CT_RMASK) == CT_RMASK) ulBitMask |= D3DCOLORWRITEENABLE_RED;
  if( (ulColorMask&CT_GMASK) == CT_GMASK) ulBitMask |= D3DCOLORWRITEENABLE_GREEN;
  if( (ulColorMask&CT_BMASK) == CT_BMASK) ulBitMask |= D3DCOLORWRITEENABLE_BLUE;
  if( (ulColorMask&CT_AMASK) == CT_AMASK) ulBitMask |= D3DCOLORWRITEENABLE_ALPHA;
  HRESULT hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_COLORWRITEENABLE, ulBitMask);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



__forceinline _D3DCMPFUNC CompToD3D( GfxComp eFunc) {
  switch( eFunc) {
  case GFX_NEVER:         return D3DCMP_NEVER;
  case GFX_LESS:          return D3DCMP_LESS;
  case GFX_LESS_EQUAL:    return D3DCMP_LESSEQUAL;
  case GFX_EQUAL:         return D3DCMP_EQUAL;
  case GFX_NOT_EQUAL:     return D3DCMP_NOTEQUAL;
  case GFX_GREATER_EQUAL: return D3DCMP_GREATEREQUAL;
  case GFX_GREATER:       return D3DCMP_GREATER;
  case GFX_ALWAYS:        return D3DCMP_ALWAYS;
  default: ASSERTALWAYS("Invalid GFX compare function!");
  } return D3DCMP_ALWAYS;
}

__forceinline GfxComp CompFromD3D( _D3DCMPFUNC d3dcFunc) {
  switch( d3dcFunc) {
  case D3DCMP_NEVER:        return GFX_NEVER;
  case D3DCMP_LESS:         return GFX_LESS;
  case D3DCMP_LESSEQUAL:    return GFX_LESS_EQUAL;
  case D3DCMP_EQUAL:        return GFX_EQUAL;
  case D3DCMP_NOTEQUAL:     return GFX_NOT_EQUAL;
  case D3DCMP_GREATEREQUAL: return GFX_GREATER_EQUAL;
  case D3DCMP_GREATER:      return GFX_GREATER;
  case D3DCMP_ALWAYS:       return GFX_ALWAYS;
  default: ASSERTALWAYS("Invalid D3D compare function!");
  } return GFX_ALWAYS;
}




// set depth buffer compare mode
static void d3d_DepthFunc( GfxComp eFunc)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
 _D3DCMPFUNC d3dcFunc;
#ifndef NDEBUG
  GfxComp gfxFunc; 
  hr = _pGfx->gl_pd3dDevice->GetRenderState( D3DRS_ZFUNC, (DWORD*)&d3dcFunc);
  D3D_CHECKERROR(hr);
  gfxFunc = CompFromD3D( d3dcFunc);
  ASSERT( gfxFunc==GFX_eDepthFunc);
#endif
  // cached?
  if( eFunc==GFX_eDepthFunc && gap_bOptimizeStateChanges) return;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  d3dcFunc = CompToD3D(eFunc);
  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_ZFUNC, d3dcFunc);
  D3D_CHECKERROR(hr);
  GFX_eDepthFunc = eFunc;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}


    

// set depth buffer range
static void d3d_DepthRange( FLOAT fMin, FLOAT fMax)
{
  // D3D doesn't allow 0 for max value (no comment!)
  if( fMax<0.001f) fMax = 0.001f; 

  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
  D3DVIEWPORT8 d3dViewport;
#ifndef NDEBUG
  hr = _pGfx->gl_pd3dDevice->GetViewport( &d3dViewport);
  ASSERT( d3dViewport.MinZ==GFX_fMinDepthRange && d3dViewport.MaxZ==GFX_fMaxDepthRange);
#endif

  // cached?
  if( GFX_fMinDepthRange==fMin && GFX_fMaxDepthRange==fMax && gap_bOptimizeStateChanges) return;
  GFX_fMinDepthRange = fMin;
  GFX_fMaxDepthRange = fMax;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // get viewport
  hr = _pGfx->gl_pd3dDevice->GetViewport( &d3dViewport);
  D3D_CHECKERROR(hr);
  // update depth range and set the viewport back
  d3dViewport.MinZ = fMin;
  d3dViewport.MaxZ = fMax;
  hr = _pGfx->gl_pd3dDevice->SetViewport( &d3dViewport);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}




static void d3d_CullFace( GfxFace eFace)
{
  // check consistency and face
  ASSERT( eFace==GFX_FRONT || eFace==GFX_BACK || eFace==GFX_NONE);
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;

  // must (re)assign faces for front-face emulation purposes
  GfxFace eFrontFace, eBackFace;
  if( GFX_bFrontFace) { eFrontFace = GFX_FRONT; eBackFace  = GFX_BACK;  }
  else                { eFrontFace = GFX_BACK;  eBackFace  = GFX_FRONT; }
  // cached?
  if( gap_bOptimizeStateChanges) {
    if( GFX_eCullFace==D3DCULL_NONE && eFace==GFX_NONE)   return;
    if( GFX_eCullFace==D3DCULL_CCW  && eFace==eFrontFace) return;
    if( GFX_eCullFace==D3DCULL_CW   && eFace==eBackFace)  return;
  }

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

       if( eFace==eFrontFace) hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW);   
  else if( eFace==eBackFace)  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW);
  else                        hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE); 
  D3D_CHECKERROR(hr);
  GFX_eCullFace = eFace;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_FrontFace( GfxFace eFace)
{
  // check consistency and face
  ASSERT( eFace==GFX_CW || eFace==GFX_CCW);
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  // cached?
  BOOL bFrontFace = (eFace==GFX_CCW);
  if( !bFrontFace==!GFX_bFrontFace && gap_bOptimizeStateChanges) return;

  // must emulate this by toggling cull face
  GFX_bFrontFace = bFrontFace; 
  if( GFX_eCullFace!=GFX_NONE) d3d_CullFace(GFX_eCullFace);
}




static void d3d_ClipPlane( const DOUBLE *pdViewPlane)
{
  // check API and plane
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D && pdViewPlane!=NULL);

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // convert to floats, keep and update
  D3D_afClipPlane[0] = pdViewPlane[0];
  D3D_afClipPlane[1] = pdViewPlane[1];
  D3D_afClipPlane[2] = pdViewPlane[2];
  D3D_afClipPlane[3] = pdViewPlane[3];
  UpdateClipPlane_D3D();

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_SetTextureMatrix( const FLOAT *pfMatrix/*=NULL*/)
{
  // check API
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  HRESULT hr;
  D3DTRANSFORMSTATETYPE tsMatrixNo = (D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + GFX_iActiveTexUnit);

  if( pfMatrix!=NULL) {
    hr = _pGfx->gl_pd3dDevice->SetTransform( tsMatrixNo, (_D3DMATRIX*)pfMatrix);
  } else {
    // load identity matrix
    extern const D3DMATRIX GFX_d3dIdentityMatrix;
    hr = _pGfx->gl_pd3dDevice->SetTransform( tsMatrixNo, &GFX_d3dIdentityMatrix);
  } // check
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}




static void d3d_SetViewMatrix( const FLOAT *pfMatrix/*=NULL*/)
{
  // check API
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;

  // cached? (only identity matrix)
  if( pfMatrix==NULL && GFX_bViewMatrix==NONE && gap_bOptimizeStateChanges) return;
  GFX_bViewMatrix = (pfMatrix!=NULL);

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  if( pfMatrix!=NULL) {
    // need to keep it for clip plane
    CopyLongs( (ULONG*)pfMatrix, (ULONG*)D3D_afViewMatrix, 16);
    hr = _pGfx->gl_pd3dDevice->SetTransform( D3DTS_VIEW, (_D3DMATRIX*)D3D_afViewMatrix);
  } else {
    // load identity matrix
    extern const D3DMATRIX GFX_d3dIdentityMatrix;
    hr = _pGfx->gl_pd3dDevice->SetTransform( D3DTS_VIEW, &GFX_d3dIdentityMatrix);
  } // check
  D3D_CHECKERROR(hr);
  // update clip plane if enabled
  if( GFX_bClipPlane) UpdateClipPlane_D3D();

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_SetOrtho( const FLOAT fLeft,   const FLOAT fRight, const FLOAT fTop,
                          const FLOAT fBottom, const FLOAT fNear,  const FLOAT fFar,
                          const BOOL bSubPixelAdjust/*=FALSE*/)
{
  // check API and matrix type
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);

  // cached?
  if( GFX_fLastL==fLeft  && GFX_fLastT==fTop    && GFX_fLastN==fNear
   && GFX_fLastR==fRight && GFX_fLastB==fBottom && GFX_fLastF==fFar && gap_bOptimizeStateChanges) return;
  GFX_fLastL = fLeft;   GFX_fLastT = fTop;     GFX_fLastN = fNear;
  GFX_fLastR = fRight;  GFX_fLastB = fBottom;  GFX_fLastF = fFar;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // adjust coords for sub-pixel precision
  // generate ortho matrix (+1 is because of sub-pixel adjustment!)
  FLOAT fAdj = bSubPixelAdjust ? 1 : 0;
  const FLOAT fRpL=fRight+fLeft+fAdj;  const FLOAT fRmL=fRight-fLeft;  const FLOAT fFpN=fFar+fNear;
  const FLOAT fTpB=fTop+fBottom+fAdj;  const FLOAT fTmB=fTop-fBottom;  const FLOAT fFmN=fFar-fNear;
  const FLOAT afMatrix[16] = { 2/fRmL,          0,           0, 0,
                                    0,     2/fTmB,           0, 0,
                                    0,          0,     -1/fFmN, 0,
                           -fRpL/fRmL, -fTpB/fTmB, -fNear/fFmN, 1 };
  HRESULT hr = _pGfx->gl_pd3dDevice->SetTransform( D3DTS_PROJECTION, (_D3DMATRIX*)afMatrix);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_SetFrustum( const FLOAT fLeft, const FLOAT fRight,
                            const FLOAT fTop,  const FLOAT fBottom,
                            const FLOAT fNear, const FLOAT fFar)
{
  // check API
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);

  // cached?
  if( GFX_fLastL==-fLeft  && GFX_fLastT==-fTop    && GFX_fLastN==-fNear
   && GFX_fLastR==-fRight && GFX_fLastB==-fBottom && GFX_fLastF==-fFar && gap_bOptimizeStateChanges) return;
  GFX_fLastL = -fLeft;   GFX_fLastT = -fTop;     GFX_fLastN = -fNear;
  GFX_fLastR = -fRight;  GFX_fLastB = -fBottom;  GFX_fLastF = -fFar;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // generate frustum matrix
  const FLOAT fRpL=fRight+fLeft;  const FLOAT fTpB=fTop+fBottom;  const FLOAT fFpN=fFar+fNear;
  const FLOAT fRmL=fRight-fLeft;  const FLOAT fTmB=fTop-fBottom;  const FLOAT fFmN=fFar-fNear;
  const FLOAT afMatrix[4][4] = {
    { 2*fNear/fRmL,            0,                0,  0 },
    {            0, 2*fNear/fTmB,                0,  0 },
    {    fRpL/fRmL,    fTpB/fTmB,       -fFar/fFmN, -1 },
    {            0,            0, -fFar*fNear/fFmN,  0 }
  }; // set it
  HRESULT hr = _pGfx->gl_pd3dDevice->SetTransform( D3DTS_PROJECTION, (const _D3DMATRIX*)&afMatrix);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}




static void d3d_PolygonMode( GfxPolyMode ePolyMode)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  HRESULT hr;
  switch(ePolyMode) {
  case GFX_POINT:  hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_POINT);      break;
  case GFX_LINE:   hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME);  break;
  case GFX_FILL:   hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID);      break;
  default:  ASSERTALWAYS("Wrong polygon mode!");
  } // check
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}




// TEXTURE MANAGEMENT


static void d3d_SetTextureWrapping( enum GfxWrap eWrapU, enum GfxWrap eWrapV)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG
  // check wrapping consistency
  D3DTEXTUREADDRESS d3dtaU, d3dtaV;
  hr = _pGfx->gl_pd3dDevice->GetTextureStageState( GFX_iActiveTexUnit, D3DTSS_ADDRESSU, (ULONG*)&d3dtaU);  D3D_CHECKERROR(hr);
  hr = _pGfx->gl_pd3dDevice->GetTextureStageState( GFX_iActiveTexUnit, D3DTSS_ADDRESSV, (ULONG*)&d3dtaV);  D3D_CHECKERROR(hr);
  ASSERT( (d3dtaU==D3DTADDRESS_WRAP  && _tpGlobal[GFX_iActiveTexUnit].tp_eWrapU==GFX_REPEAT)
       || (d3dtaU==D3DTADDRESS_CLAMP && _tpGlobal[GFX_iActiveTexUnit].tp_eWrapU==GFX_CLAMP)
       || (_tpGlobal[GFX_iActiveTexUnit].tp_eWrapU==0));
  ASSERT( (d3dtaV==D3DTADDRESS_WRAP  && _tpGlobal[GFX_iActiveTexUnit].tp_eWrapV==GFX_REPEAT)
       || (d3dtaV==D3DTADDRESS_CLAMP && _tpGlobal[GFX_iActiveTexUnit].tp_eWrapV==GFX_CLAMP)
       || (_tpGlobal[GFX_iActiveTexUnit].tp_eWrapV==0));
#endif

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // adjust only those that differ
  if( _tpGlobal[GFX_iActiveTexUnit].tp_eWrapU!=eWrapU) {
    D3DTEXTUREADDRESS d3dta = (eWrapU==GFX_REPEAT) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
    hr = _pGfx->gl_pd3dDevice->SetTextureStageState( GFX_iActiveTexUnit, D3DTSS_ADDRESSU, d3dta);
    D3D_CHECKERROR(hr);
   _tpGlobal[GFX_iActiveTexUnit].tp_eWrapU = eWrapU;
  }
  if( _tpGlobal[GFX_iActiveTexUnit].tp_eWrapV!=eWrapV) {
    D3DTEXTUREADDRESS d3dta = (eWrapV==GFX_REPEAT) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
    hr = _pGfx->gl_pd3dDevice->SetTextureStageState( GFX_iActiveTexUnit, D3DTSS_ADDRESSV, d3dta);
    D3D_CHECKERROR(hr);
   _tpGlobal[GFX_iActiveTexUnit].tp_eWrapV = eWrapV;
  }

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_SetTextureModulation( INDEX iScale)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  HRESULT hr;
#ifndef NDEBUG                 
  INDEX iRet;
  hr = _pGfx->gl_pd3dDevice->GetTextureStageState( GFX_iActiveTexUnit, D3DTSS_COLOROP, (DWORD*)&iRet);
  D3D_CHECKERROR(hr);
       if( iRet==D3DTOP_MODULATE2X) ASSERT( GFX_iTexModulation[GFX_iActiveTexUnit]==2);
  else if( iRet==D3DTOP_MODULATE)   ASSERT( GFX_iTexModulation[GFX_iActiveTexUnit]==1);
  else                              ASSERT( iRet==D3DTOP_DISABLE);
#endif
  
  // cached?
  ASSERT( iScale==1 || iScale==2);
  if( GFX_iTexModulation[GFX_iActiveTexUnit]==iScale) return;
  GFX_iTexModulation[GFX_iActiveTexUnit] = iScale;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // set only if texturing is enabled - will be auto-set at gfxEnableTexture
  if( GFX_abTexture[GFX_iActiveTexUnit]) {
    D3DTEXTUREOP d3dTexOp = (iScale==2) ? D3DTOP_MODULATE2X : D3DTOP_MODULATE;
    hr = _pGfx->gl_pd3dDevice->SetTextureStageState( GFX_iActiveTexUnit, D3DTSS_COLOROP, d3dTexOp);
    D3D_CHECKERROR(hr);
  }
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_GenerateTexture( ULONG &ulTexObject)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  _sfStats.StartTimer(CStatForm::STI_BINDTEXTURE);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // generate one dummy texture that'll be entirely replaced upon 1st upload
  HRESULT hr = _pGfx->gl_pd3dDevice->CreateTexture( 1, 1, 0, 0, (D3DFORMAT)TS.ts_tfRGBA8, D3DPOOL_MANAGED, (LPDIRECT3DTEXTURE8*)&ulTexObject);
  D3D_CHECKERROR(hr);

  _sfStats.StopTimer(CStatForm::STI_BINDTEXTURE);
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}


// unbind texture from API
static void d3d_DeleteTexture( ULONG &ulTexObject)
{
  // skip if already unbound
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  if( ulTexObject==NONE) return;

  _sfStats.StartTimer(CStatForm::STI_BINDTEXTURE);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  D3DRELEASE( (LPDIRECT3DTEXTURE8&)ulTexObject, TRUE);
  ulTexObject = NONE;

  _sfStats.StopTimer(CStatForm::STI_BINDTEXTURE);
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}




// VERTEX ARRAYS


static void d3d_SetVertexArray( GFXVertex4 *pvtx, INDEX ctVtx)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  ASSERT( ctVtx>0 && pvtx!=NULL && GFX_iActiveTexUnit==0);
  GFX_ctVertices = ctVtx;
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  SetVertexArray_D3D( 0, (ULONG*)pvtx); // type 0 = vertices

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_SetNormalArray( GFXNormal *pnor)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  ASSERT( pnor!=NULL);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  SetVertexArray_D3D( 1, (ULONG*)pnor); // type 1 = normals

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_SetColorArray( GFXColor *pcol)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  ASSERT( pcol!=NULL);
  d3d_EnableColorArray();
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);
  
  SetVertexArray_D3D( 2, (ULONG*)pcol); // type 2 = colors

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_SetTexCoordArray( GFXTexCoord *ptex, BOOL b4/*=FALSE*/)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  ASSERT( ptex!=NULL);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // type 3 = regular texture coordinates; type 4 = projective texture coordinates
  SetVertexArray_D3D( b4?4:3, (ULONG*)ptex);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_SetConstantColor( COLOR col)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  d3d_DisableColorArray();
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  const ULONG d3dColor = rgba2argb(col);
  HRESULT hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_AMBIENT, d3dColor);
  D3D_CHECKERROR(hr); 

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void d3d_DrawElements( INDEX ctElem, INDEX *pidx)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
#ifndef NDEBUG
  // check if all indices are inside lock count (or smaller than 65536)
  if( pidx!=NULL) for( INDEX i=0; i<ctElem; i++) ASSERT( pidx[i] < GFX_ctVertices);
#endif

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);
  _pGfx->gl_ctTotalTriangles += ctElem/3;  // for profiling

  ASSERT( pidx!=NULL);
  extern void DrawElements_D3D( INDEX ctElem, INDEX *pidx);
  DrawElements_D3D( ctElem, pidx);

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}




// force finish of API rendering queue
static void d3d_Finish(void)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // must "emulate" this by reading from back buffer :(
  HRESULT hr;
  D3DLOCKED_RECT rectLocked;
  LPDIRECT3DSURFACE8 pBackBuffer;
  // fetch back buffer (different for full screen and windowed mode)
  const BOOL bFullScreen = _pGfx->gl_ulFlags & GLF_FULLSCREEN;
  const RECT rectToLock  = { 0,0, 1,1 };
  if( bFullScreen)   hr = _pGfx->gl_pd3dDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
  else hr = _pGfx->gl_pvpActive->vp_pSwapChain->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
  if( hr==D3D_OK) {
    hr = pBackBuffer->LockRect( &rectLocked, &rectToLock, D3DLOCK_READONLY);
    if( hr==D3D_OK) pBackBuffer->UnlockRect();
    D3DRELEASE( pBackBuffer, TRUE);
  }
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}





static void d3d_LockArrays(void)
{
  // only for OpenGL
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  ASSERT( GFX_ctVertices>0 && !_bCVAReallyLocked);
}


// SPECIAL FUNCTIONS FOR Direct3D ONLY !


// set D3D vertex shader only if different than last time
extern void d3dSetVertexShader( DWORD dwHandle)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_D3D);
  if( _pGfx->gl_dwVertexShader==dwHandle) return;
  HRESULT hr = _pGfx->gl_pd3dDevice->SetVertexShader(dwHandle);
  D3D_CHECKERROR(hr);
  _pGfx->gl_dwVertexShader = dwHandle;
}

#endif // SE1_D3D
