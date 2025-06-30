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

#include <Engine/StdH.h>

#include <Engine/Graphics/DrawPort.h>

#include <Engine/Base/Memory.h>
#include <Engine/Base/CRC.h>
#include <Engine/Math/Functions.h>
#include <Engine/Math/Projection.h>
#include <Engine/Math/AABBox.h>
#include <Engine/Graphics/Raster.h>
#include <Engine/Graphics/GfxProfile.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/ImageInfo.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/ViewPort.h>
#include <Engine/Graphics/Font.h>

#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>

extern INDEX gfx_bDecoratedText;
extern INDEX ogl_iFinish;
extern INDEX d3d_iFinish;


// RECT HANDLING ROUTINES


static BOOL ClipToDrawPort( const CDrawPort *pdp, PIX &pixI, PIX &pixJ, PIX &pixW, PIX &pixH)
{
  if( pixI < 0) { pixW += pixI;  pixI = 0; }
  else if( pixI >= pdp->dp_Width) return FALSE;

  if( pixJ < 0) { pixH += pixJ;  pixJ = 0; }
  else if( pixJ >= pdp->dp_Height) return FALSE;

  if( pixW<1 || pixH<1) return FALSE;

  if( (pixI+pixW) > pdp->dp_Width)  pixW = pdp->dp_Width  - pixI;
  if( (pixJ+pixH) > pdp->dp_Height) pixH = pdp->dp_Height - pixJ;

  ASSERT( pixI>=0 && pixI<pdp->dp_Width);
  ASSERT( pixJ>=0 && pixJ<pdp->dp_Height);
  ASSERT( pixW>0  && pixH>0);
  return TRUE;
}


// set scissor (clipping) to window inside drawport
static void SetScissor( const CDrawPort *pdp, PIX pixI, PIX pixJ, PIX pixW, PIX pixH)
{
  ASSERT( pixI>=0 && pixI<pdp->dp_Width);
  ASSERT( pixJ>=0 && pixJ<pdp->dp_Height);
  ASSERT( pixW>0  && pixH>0);

  const PIX pixInvMinJ = pdp->dp_Raster->ra_Height - (pdp->dp_MinJ+pixJ+pixH);
  pglScissor( pdp->dp_MinI+pixI, pixInvMinJ, pixW, pixH);
  ASSERT( pglIsEnabled(GL_SCISSOR_TEST));
  OGL_CHECKERROR;
}


// reset scissor (clipping) to whole drawport
static void ResetScissor( const CDrawPort *pdp)
{
  const PIX pixMinSI = pdp->dp_ScissorMinI;
  const PIX pixMaxSI = pdp->dp_ScissorMaxI;
  const PIX pixMinSJ = pdp->dp_Raster->ra_Height-1 - pdp->dp_ScissorMaxJ;
  const PIX pixMaxSJ = pdp->dp_Raster->ra_Height-1 - pdp->dp_ScissorMinJ;

  pglScissor( pixMinSI, pixMinSJ, pixMaxSI-pixMinSI+1, pixMaxSJ-pixMinSJ+1);
  ASSERT( pglIsEnabled(GL_SCISSOR_TEST));
  OGL_CHECKERROR;
}


// DRAWPORT ROUTINES

// set cloned drawport dimensions
void CDrawPort::InitCloned( CDrawPort *pdpBase, DOUBLE rMinI,DOUBLE rMinJ, DOUBLE rSizeI,DOUBLE rSizeJ)
{
  // remember the raster structures
  dp_Raster = pdpBase->dp_Raster;
  // set relative dimensions to make it contain the whole raster
  dp_MinIOverRasterSizeI  = rMinI * pdpBase->dp_SizeIOverRasterSizeI
    +pdpBase->dp_MinIOverRasterSizeI;
  dp_MinJOverRasterSizeJ  = rMinJ * pdpBase->dp_SizeJOverRasterSizeJ
    +pdpBase->dp_MinJOverRasterSizeJ;
  dp_SizeIOverRasterSizeI = rSizeI * pdpBase->dp_SizeIOverRasterSizeI;
  dp_SizeJOverRasterSizeJ = rSizeJ * pdpBase->dp_SizeJOverRasterSizeJ;
	// calculate pixel dimensions from relative dimensions
  RecalculateDimensions();
  // clip scissor to origin drawport
  dp_ScissorMinI = Max( dp_MinI, pdpBase->dp_MinI);
	dp_ScissorMinJ = Max( dp_MinJ, pdpBase->dp_MinJ);
  dp_ScissorMaxI = Min( dp_MaxI, pdpBase->dp_MaxI);
	dp_ScissorMaxJ = Min( dp_MaxJ, pdpBase->dp_MaxJ);
  if( dp_ScissorMinI>dp_ScissorMaxI) dp_ScissorMinI = dp_ScissorMaxI = 0;
  if( dp_ScissorMinJ>dp_ScissorMaxJ) dp_ScissorMinJ = dp_ScissorMaxJ = 0;
	// clone some vars
  dp_FontData = pdpBase->dp_FontData;
  dp_pixTextCharSpacing = pdpBase->dp_pixTextCharSpacing;
  dp_pixTextLineSpacing = pdpBase->dp_pixTextLineSpacing;
  dp_fTextScaling = pdpBase->dp_fTextScaling;
  dp_fTextAspect  = pdpBase->dp_fTextAspect;
  dp_iTextMode    = pdpBase->dp_iTextMode;
  dp_fWideAdjustment   = pdpBase->dp_fWideAdjustment;
  dp_bRenderingOverlay = pdpBase->dp_bRenderingOverlay;
  // reset rest of vars
  dp_ulBlendingRA = 0;
  dp_ulBlendingGA = 0;
  dp_ulBlendingBA = 0;
  dp_ulBlendingA  = 0;
}

/* Create a drawport for full raster. */
CDrawPort::CDrawPort( CRaster *praBase)
{
  // remember the raster structures
  dp_Raster = praBase;
  dp_fWideAdjustment = 1.0f;
  dp_bRenderingOverlay = FALSE;
  // set relative dimensions to make it contain the whole raster
  dp_MinIOverRasterSizeI  = 0.0;
  dp_MinJOverRasterSizeJ  = 0.0;
  dp_SizeIOverRasterSizeI = 1.0;
  dp_SizeJOverRasterSizeJ = 1.0;
	// clear unknown values
  dp_FontData = NULL;
  dp_pixTextCharSpacing = 1;
  dp_pixTextLineSpacing = 0;
  dp_fTextScaling = 1.0f;
  dp_fTextAspect  = 1.0f;
  dp_iTextMode    = 1;
  dp_ulBlendingRA = 0;
  dp_ulBlendingGA = 0;
  dp_ulBlendingBA = 0;
  dp_ulBlendingA  = 0;
}

/* Clone a drawport */
CDrawPort::CDrawPort( CDrawPort *pdpBase,
                      DOUBLE rMinI,DOUBLE rMinJ, DOUBLE rSizeI,DOUBLE rSizeJ)
{
  InitCloned( pdpBase, rMinI,rMinJ, rSizeI,rSizeJ);
}

CDrawPort::CDrawPort( CDrawPort *pdpBase, const PIXaabbox2D &box)
{
  // force dimensions
  dp_MinI   = box.Min()(1) +pdpBase->dp_MinI;
	dp_MinJ   = box.Min()(2) +pdpBase->dp_MinJ;
  dp_Width  = box.Size()(1);
	dp_Height = box.Size()(2);
  dp_MaxI   = dp_MinI+dp_Width  -1;
	dp_MaxJ   = dp_MinJ+dp_Height -1;
  // clip scissor to origin drawport
  dp_ScissorMinI = Max( dp_MinI, pdpBase->dp_MinI);
	dp_ScissorMinJ = Max( dp_MinJ, pdpBase->dp_MinJ);
  dp_ScissorMaxI = Min( dp_MaxI, pdpBase->dp_MaxI);
	dp_ScissorMaxJ = Min( dp_MaxJ, pdpBase->dp_MaxJ);
  if( dp_ScissorMinI>dp_ScissorMaxI) dp_ScissorMinI = dp_ScissorMaxI = 0;
  if( dp_ScissorMinJ>dp_ScissorMaxJ) dp_ScissorMinJ = dp_ScissorMaxJ = 0;
  // remember the raster structures
  dp_Raster = pdpBase->dp_Raster;
  // set relative dimensions to make it contain the whole raster
  dp_MinIOverRasterSizeI  = (DOUBLE)dp_MinI   / dp_Raster->ra_Width;
  dp_MinJOverRasterSizeJ  = (DOUBLE)dp_MinJ   / dp_Raster->ra_Height;
  dp_SizeIOverRasterSizeI = (DOUBLE)dp_Width  / dp_Raster->ra_Width;
  dp_SizeJOverRasterSizeJ = (DOUBLE)dp_Height / dp_Raster->ra_Height;
	// clone some vars
  dp_FontData = pdpBase->dp_FontData;
  dp_pixTextCharSpacing = pdpBase->dp_pixTextCharSpacing;
  dp_pixTextLineSpacing = pdpBase->dp_pixTextLineSpacing;
  dp_fTextScaling = pdpBase->dp_fTextScaling;
  dp_fTextAspect  = pdpBase->dp_fTextAspect;
  dp_iTextMode    = pdpBase->dp_iTextMode;
  dp_fWideAdjustment   = pdpBase->dp_fWideAdjustment;
  dp_bRenderingOverlay = pdpBase->dp_bRenderingOverlay;
  // reset rest of vars
  dp_ulBlendingRA = 0;
  dp_ulBlendingGA = 0;
  dp_ulBlendingBA = 0;
  dp_ulBlendingA  = 0;
}



// check if a drawport is dualhead
BOOL CDrawPort::IsDualHead(void)
{
  return GetWidth()*3 == GetHeight()*8;
}


// check if a drawport is already wide screen
BOOL CDrawPort::IsWideScreen(void)
{
  return GetWidth()*9 == GetHeight()*16;
}


// returns unique drawports number
ULONG CDrawPort::GetID(void)
{
  ULONG ulCRC;
  CRC_Start(   ulCRC);
#ifdef PLATFORM_64BIT
  CRC_AddLONGLONG( ulCRC, (__uint64)(size_t)dp_Raster);
#else
  CRC_AddLONG( ulCRC, (ULONG)(size_t)dp_Raster);
#endif
  CRC_AddLONG( ulCRC, (ULONG)dp_MinI);
  CRC_AddLONG( ulCRC, (ULONG)dp_MinJ);
  CRC_AddLONG( ulCRC, (ULONG)dp_MaxI);
  CRC_AddLONG( ulCRC, (ULONG)dp_MaxJ);
  CRC_Finish(  ulCRC);
  return ulCRC;
}


// dualhead cloning
CDrawPort::CDrawPort( CDrawPort *pdpBase, BOOL bLeft)
{
  // if it is not a dualhead drawport
  if (!pdpBase->IsDualHead()) {
    // always use entire drawport
    InitCloned(pdpBase, 0,0, 1,1);
  // if dualhead is on
  } else {
    // use left or right
    if (bLeft) {
      InitCloned(pdpBase, 0,0, 0.5,1);
    } else {
      InitCloned(pdpBase, 0.5,0, 0.5,1);
    }
  }
}


//  wide-screen cloning
void CDrawPort::MakeWideScreen(CDrawPort *pdp)
{
  // already wide?
  if( IsWideScreen()) {
    pdp->InitCloned( this, 0,0, 1,1);
    return;
  }
  // make wide!
  else {
    // get size
    const PIX pixSizeI = GetWidth();
    const PIX pixSizeJ = GetHeight();
    // make horiz width
    PIX pixSizeJW = pixSizeI*9/16;
    // if already too wide
    if (pixSizeJW>pixSizeJ-10) {
      // no wide screen
      pdp->InitCloned( this, 0,0, 1,1);
      return;
    }
    // clear upper and lower blanks
    const PIX pixJ0 = (pixSizeJ-pixSizeJW)/2;
    if( Lock()) {
      Fill(0, 0, pixSizeI, pixJ0, C_BLACK|CT_OPAQUE);
      Fill(0, pixJ0+pixSizeJW, pixSizeI, pixJ0, C_BLACK|CT_OPAQUE);
      Unlock();
    }
    // init
    pdp->InitCloned( this, 0, FLOAT(pixJ0)/pixSizeJ, 1, FLOAT(pixSizeJW)/pixSizeJ);
    pdp->dp_fWideAdjustment = 9.0f / 12.0f;
  }
}


/*****
 * Recalculate pixel dimensions from relative dimensions and raster size.
 */

void CDrawPort::RecalculateDimensions(void)
{
	const PIX pixRasterSizeI = dp_Raster->ra_Width;
	const PIX pixRasterSizeJ = dp_Raster->ra_Height;
  dp_Width  = (PIX)(dp_SizeIOverRasterSizeI*pixRasterSizeI);
	dp_Height = (PIX)(dp_SizeJOverRasterSizeJ*pixRasterSizeJ);
  dp_ScissorMinI = dp_MinI = (PIX)(dp_MinIOverRasterSizeI*pixRasterSizeI);
	dp_ScissorMinJ = dp_MinJ = (PIX)(dp_MinJOverRasterSizeJ*pixRasterSizeJ);
  dp_ScissorMaxI = dp_MaxI = dp_MinI+dp_Width  -1;
	dp_ScissorMaxJ = dp_MaxJ = dp_MinJ+dp_Height -1;
}


// set orthogonal projection
void CDrawPort::SetOrtho(void) const
{
  // finish all pending render-operations (if required)
  ogl_iFinish = Clamp( ogl_iFinish, (INDEX)0, (INDEX)3);
  d3d_iFinish = Clamp( d3d_iFinish, (INDEX)0, (INDEX)3);
  if( (ogl_iFinish==3 && _pGfx->gl_eCurrentAPI==GAT_OGL) 
#ifdef SE1_D3D
   || (d3d_iFinish==3 && _pGfx->gl_eCurrentAPI==GAT_D3D)
#endif // SE1_D3D
   ) gfxFinish();

  // prepare ortho dimensions
  const PIX pixMinI  = dp_MinI;
  const PIX pixMinSI = dp_ScissorMinI;
  const PIX pixMaxSI = dp_ScissorMaxI;
  const PIX pixMaxJ  = dp_Raster->ra_Height -1 - dp_MinJ;
  const PIX pixMinSJ = dp_Raster->ra_Height -1 - dp_ScissorMaxJ;
  const PIX pixMaxSJ = dp_Raster->ra_Height -1 - dp_ScissorMinJ;

  // init matrices (D3D needs sub-pixel adjustment)
  gfxSetOrtho( pixMinSI-pixMinI, pixMaxSI-pixMinI+1, pixMaxJ-pixMaxSJ, pixMaxJ-pixMinSJ+1, 0.0f, -1.0f, TRUE);
  gfxDepthRange(0,1);
  gfxSetViewMatrix(NULL);
  // disable face culling, custom clip plane and truform
  gfxCullFace(GFX_NONE);
  gfxDisableClipPlane();
  gfxDisableTruform();
}


// set given projection
void CDrawPort::SetProjection(CAnyProjection3D &apr) const
{
  // finish all pending render-operations (if required)
  ogl_iFinish = Clamp( ogl_iFinish, (INDEX)0, (INDEX)3);
  d3d_iFinish = Clamp( d3d_iFinish, (INDEX)0, (INDEX)3);
  if( (ogl_iFinish==3 && _pGfx->gl_eCurrentAPI==GAT_OGL) 
#ifdef SE1_D3D
   || (d3d_iFinish==3 && _pGfx->gl_eCurrentAPI==GAT_D3D)
#endif // SE1_D3D
   ) gfxFinish();

  // if isometric projection
  if( apr.IsIsometric()) {
    CIsometricProjection3D &ipr = (CIsometricProjection3D&)*apr;
    const FLOAT2D vMin  = ipr.pr_ScreenBBox.Min()-ipr.pr_ScreenCenter;
    const FLOAT2D vMax  = ipr.pr_ScreenBBox.Max()-ipr.pr_ScreenCenter;
    const FLOAT fFactor = 1.0f / (ipr.ipr_ZoomFactor*ipr.pr_fViewStretch);
    const FLOAT fNear   = ipr.pr_NearClipDistance;
    const FLOAT fLeft   = +vMin(1) *fFactor;
    const FLOAT fRight  = +vMax(1) *fFactor;
    const FLOAT fTop    = -vMin(2) *fFactor;
    const FLOAT fBottom = -vMax(2) *fFactor;
    // if far clip plane is not specified use maximum expected dimension of the world
    FLOAT fFar = ipr.pr_FarClipDistance;
    if( fFar<0) fFar = 1E5f;  // max size 32768, 3D (sqrt(3)), rounded up
    gfxSetOrtho( fLeft, fRight, fTop, fBottom, fNear, fFar, FALSE);
  }
  // if perspective projection
  else {
    ASSERT( apr.IsPerspective());
    CPerspectiveProjection3D &ppr = (CPerspectiveProjection3D&)*apr;
    const FLOAT fNear   = ppr.pr_NearClipDistance;
    const FLOAT fLeft   = ppr.pr_plClipL(3) / ppr.pr_plClipL(1) *fNear;
    const FLOAT fRight  = ppr.pr_plClipR(3) / ppr.pr_plClipR(1) *fNear;
    const FLOAT fTop    = ppr.pr_plClipU(3) / ppr.pr_plClipU(2) *fNear;
    const FLOAT fBottom = ppr.pr_plClipD(3) / ppr.pr_plClipD(2) *fNear;
    // if far clip plane is not specified use maximum expected dimension of the world
    FLOAT fFar = ppr.pr_FarClipDistance;
    if( fFar<0) fFar = 1E5f;  // max size 32768, 3D (sqrt(3)), rounded up
    gfxSetFrustum( fLeft, fRight, fTop, fBottom, fNear, fFar);
  }
  
  // set some rendering params
  gfxDepthRange( apr->pr_fDepthBufferNear, apr->pr_fDepthBufferFar);
  gfxCullFace(GFX_BACK);
  gfxSetViewMatrix(NULL);
  gfxDisableTruform();
  
  // if projection is mirrored/warped and mirroring is allowed
  if( apr->pr_bMirror || apr->pr_bWarp) {
    // set custom clip plane 0 to mirror plane
    gfxEnableClipPlane();
    DOUBLE adViewPlane[4];
    adViewPlane[0] = +apr->pr_plMirrorView(1); 
    adViewPlane[1] = +apr->pr_plMirrorView(2); 
    adViewPlane[2] = +apr->pr_plMirrorView(3); 
    adViewPlane[3] = -apr->pr_plMirrorView.Distance(); 
    gfxClipPlane(adViewPlane); // NOTE: view clip plane is multiplied by inverse modelview matrix at time when specified
  }
  // if projection is not mirrored
  else {
    // just disable custom clip plane 0
    gfxDisableClipPlane();
  }
}



// implementation for some drawport routines that uses raster class

void CDrawPort::Unlock(void)
{
  dp_Raster->Unlock();
  _pGfx->UnlockDrawPort(this);
}


BOOL CDrawPort::Lock(void)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_LOCKDRAWPORT);
  BOOL bRasterLocked = dp_Raster->Lock();
  if( bRasterLocked) {
    // try to lock drawport with driver
    BOOL bDrawportLocked = _pGfx->LockDrawPort(this);
    if( !bDrawportLocked) {
      dp_Raster->Unlock();
      bRasterLocked = FALSE;
    }
  } // done
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_LOCKDRAWPORT);
  return bRasterLocked;
}



// DRAWING ROUTINES -------------------------------------



// draw one point
void CDrawPort::DrawPoint( PIX pixI, PIX pixJ, COLOR col, PIX pixRadius/*=1*/) const
{
  // check API and radius
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );
  ASSERT( pixRadius>=0);
  if( pixRadius==0) return; // do nothing if radius is 0

  // setup rendering mode
  gfxDisableTexture(); 
  gfxDisableDepthTest();
  gfxDisableDepthWrite();
  gfxDisableAlphaTest();
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  // set point color/alpha and radius
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);
  const FLOAT fR = pixRadius;

  // OpenGL
  if( eAPI==GAT_OGL) {
    const FLOAT fI = pixI+0.5f;
    const FLOAT fJ = pixJ+0.5f;
    glCOLOR(col);
    pglPointSize(fR);
#ifdef _GLES //karin: no glBegin/glEnd on GLES
	GLboolean ve = pglIsEnabled(GL_VERTEX_ARRAY);
	GLboolean te = pglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean ne = pglIsEnabled(GL_NORMAL_ARRAY);
	GLboolean ce = pglIsEnabled(GL_COLOR_ARRAY);
	if(!ve) pglEnableClientState(GL_VERTEX_ARRAY);
	if(te) pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglDisableClientState(GL_NORMAL_ARRAY);
	if(ce) pglDisableClientState(GL_COLOR_ARRAY);
	const GLfloat vs[] = {fI,fJ};
	pglVertexPointer(2, GL_FLOAT, 0, vs);
	pglDrawArrays(GL_POINTS, 0, 1);
	if(!ve) pglDisableClientState(GL_VERTEX_ARRAY);
	if(te) pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglEnableClientState(GL_NORMAL_ARRAY);
	if(ce) pglEnableClientState(GL_COLOR_ARRAY);
#else
    pglBegin(GL_POINTS);
      pglVertex2f(fI,fJ);
    pglEnd();
#endif
    OGL_CHECKERROR;
  } // Direct3D
#ifdef SE1_D3D
  else if( eAPI==GAT_D3D) {
    HRESULT hr;
    const FLOAT fI = pixI+0.75f;
    const FLOAT fJ = pixJ+0.75f;
    const ULONG d3dColor = rgba2argb(col);
    CTVERTEX avtx = {fI,fJ,0, d3dColor, 0,0};
    hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_POINTSIZE, *((DWORD*)&fR));
    D3D_CHECKERROR(hr);
    // set vertex shader and draw
    d3dSetVertexShader(D3DFVF_CTVERTEX);
    hr = _pGfx->gl_pd3dDevice->DrawPrimitiveUP( D3DPT_POINTLIST, 1, &avtx, sizeof(CTVERTEX));
    D3D_CHECKERROR(hr);
  }
#endif // SE1_D3D
}


// draw one point in 3D
void CDrawPort::DrawPoint3D( FLOAT3D v, COLOR col, FLOAT fRadius/*=1.0f*/) const
{
  // check API and radius
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );
  ASSERT( fRadius>=0);
  if( fRadius==0) return; // do nothing if radius is 0

  // setup rendering mode
  gfxDisableTexture(); 
  gfxDisableDepthWrite();
  gfxDisableAlphaTest();
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  // set point color/alpha
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);

  // OpenGL
  if( eAPI==GAT_OGL) {
    glCOLOR(col);
    pglPointSize(fRadius);
#ifdef _GLES //karin: no glBegin/glEnd on GLES
	GLboolean ve = pglIsEnabled(GL_VERTEX_ARRAY);
	GLboolean te = pglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean ne = pglIsEnabled(GL_NORMAL_ARRAY);
	GLboolean ce = pglIsEnabled(GL_COLOR_ARRAY);
	if(!ve) pglEnableClientState(GL_VERTEX_ARRAY);
	if(te) pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglDisableClientState(GL_NORMAL_ARRAY);
	if(ce) pglDisableClientState(GL_COLOR_ARRAY);
	const GLfloat vs[] = {v(1),v(2),v(3)};
	pglVertexPointer(3, GL_FLOAT, 0, vs);
	pglDrawArrays(GL_POINTS, 0, 1);
	if(!ve) pglDisableClientState(GL_VERTEX_ARRAY);
	if(te) pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglEnableClientState(GL_NORMAL_ARRAY);
	if(ce) pglEnableClientState(GL_COLOR_ARRAY);
#else
    pglBegin(GL_POINTS);
      pglVertex3f( v(1),v(2),v(3));
    pglEnd();
#endif
    OGL_CHECKERROR;
  } // Direct3D
#ifdef SE1_D3D
  else if( eAPI==GAT_D3D) {
    HRESULT hr;
    const ULONG d3dColor = rgba2argb(col);
    CTVERTEX avtx = {v(1),v(2),v(3), d3dColor, 0,0};
    hr = _pGfx->gl_pd3dDevice->SetRenderState( D3DRS_POINTSIZE, *((DWORD*)&fRadius));
    D3D_CHECKERROR(hr);
    // set vertex shader and draw
    d3dSetVertexShader(D3DFVF_CTVERTEX);
    hr = _pGfx->gl_pd3dDevice->DrawPrimitiveUP( D3DPT_POINTLIST, 1, &avtx, sizeof(CTVERTEX));
    D3D_CHECKERROR(hr);
  }
#endif // SE1_D3D
}



// draw one line
void CDrawPort::DrawLine( PIX pixI0, PIX pixJ0, PIX pixI1, PIX pixJ1, COLOR col, ULONG typ/*=_FULL*/) const
{
  // check API
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );

  // setup rendering mode
  gfxDisableDepthTest();
  gfxDisableDepthWrite();
  gfxDisableAlphaTest();
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  FLOAT fD;
  INDEX iTexFilter, iTexAnisotropy;
  if( typ==_FULL_) {
    // no pattern - just disable texturing
    gfxDisableTexture(); 
    fD = 0;
  } else {
    // revert to simple point-sample filtering without mipmaps
    INDEX iNewFilter=10, iNewAnisotropy=1;
    gfxGetTextureFiltering( iTexFilter, iTexAnisotropy);
    gfxSetTextureFiltering( iNewFilter, iNewAnisotropy);
    // prepare line pattern and mapping
    extern void gfxSetPattern( ULONG ulPattern); 
    gfxSetPattern(typ);
    fD = Max( Abs(pixI0-pixI1), Abs(pixJ0-pixJ1)) /32.0f;
  } 

  // set line color/alpha and go go go 
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);
  // OpenGL
  if( eAPI==GAT_OGL) {
    const FLOAT fI0 = pixI0+0.5f;  const FLOAT fJ0 = pixJ0+0.5f;
    const FLOAT fI1 = pixI1+0.5f;  const FLOAT fJ1 = pixJ1+0.5f;
    glCOLOR(col);
#ifdef _GLES //karin: no glBegin/glEnd on GLES
	GLboolean ve = pglIsEnabled(GL_VERTEX_ARRAY);
	GLboolean te = pglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean ne = pglIsEnabled(GL_NORMAL_ARRAY);
	GLboolean ce = pglIsEnabled(GL_COLOR_ARRAY);
	if(!ve) pglEnableClientState(GL_VERTEX_ARRAY);
	if(!te) pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglDisableClientState(GL_NORMAL_ARRAY);
	if(ce) pglDisableClientState(GL_COLOR_ARRAY);
	const GLfloat vs[] = { 
		0,0, fI0,fJ0,
		fD,0, fI1,fJ1
	};
	pglTexCoordPointer(2, GL_FLOAT, 0, vs);
	pglVertexPointer(2, GL_FLOAT, sizeof(GLfloat) * 4, vs + 2);
	pglDrawArrays(GL_LINES, 0, 2);
	if(!ve) pglDisableClientState(GL_VERTEX_ARRAY);
	if(!te) pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglEnableClientState(GL_NORMAL_ARRAY);
	if(ce) pglEnableClientState(GL_COLOR_ARRAY);
#else
    pglBegin( GL_LINES);
      pglTexCoord2f( 0,0); pglVertex2f(fI0,fJ0);
      pglTexCoord2f(fD,0); pglVertex2f(fI1,fJ1);
    pglEnd();
#endif
    OGL_CHECKERROR;
  } // Direct3D
#ifdef SE1_D3D
  else if( eAPI==GAT_D3D) {
    HRESULT hr;
    const FLOAT fI0 = pixI0+0.75f;  const FLOAT fJ0 = pixJ0+0.75f;
    const FLOAT fI1 = pixI1+0.75f;  const FLOAT fJ1 = pixJ1+0.75f;
    const ULONG d3dColor = rgba2argb(col);
    CTVERTEX avtxLine[2] = {
      {fI0,fJ0,0, d3dColor,  0,0},
      {fI1,fJ1,0, d3dColor, fD,0} };
    // set vertex shader and draw
    d3dSetVertexShader(D3DFVF_CTVERTEX);
    hr = _pGfx->gl_pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, avtxLine, sizeof(CTVERTEX));
    D3D_CHECKERROR(hr);
  }
#endif // SE1_D3D
  // revert to old filtering
  if( typ!=_FULL_) gfxSetTextureFiltering( iTexFilter, iTexAnisotropy);
}



// draw one line in 3D
void CDrawPort::DrawLine3D( FLOAT3D v0, FLOAT3D v1, COLOR col) const
{
  // check API
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );

  // setup rendering mode
  gfxDisableTexture(); 
  gfxDisableDepthWrite();
  gfxDisableAlphaTest();
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  // set line color/alpha and go go go 
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);
  // OpenGL
  if( eAPI==GAT_OGL) {
    glCOLOR(col);
#ifdef _GLES //karin: no glBegin/glEnd on GLES
	GLboolean ve = pglIsEnabled(GL_VERTEX_ARRAY);
	GLboolean te = pglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean ne = pglIsEnabled(GL_NORMAL_ARRAY);
	GLboolean ce = pglIsEnabled(GL_COLOR_ARRAY);
	if(!ve) pglEnableClientState(GL_VERTEX_ARRAY);
	if(te) pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglDisableClientState(GL_NORMAL_ARRAY);
	if(ce) pglDisableClientState(GL_COLOR_ARRAY);
	const GLfloat vs[] = {
      v0(1),v0(2),v0(3),
      v1(1),v1(2),v1(3),
	};
	pglVertexPointer(3, GL_FLOAT, 0, vs);
	pglDrawArrays(GL_LINES, 0, 2);
	if(!ve) pglDisableClientState(GL_VERTEX_ARRAY);
	if(te) pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglEnableClientState(GL_NORMAL_ARRAY);
	if(ce) pglEnableClientState(GL_COLOR_ARRAY);
#else
    pglBegin( GL_LINES);
      pglVertex3f( v0(1),v0(2),v0(3));
      pglVertex3f( v1(1),v1(2),v1(3));
    pglEnd();
#endif
    OGL_CHECKERROR;
  } // Direct3D
#ifdef SE1_D3D
  else if( eAPI==GAT_D3D) {
    HRESULT hr;
    const ULONG d3dColor = rgba2argb(col);
    CTVERTEX avtxLine[2] = {
      {v0(1),v0(2),v0(3), d3dColor, 0,0},
      {v1(1),v1(2),v1(3), d3dColor, 0,0} };
    // set vertex shader and draw
    d3dSetVertexShader(D3DFVF_CTVERTEX);
    hr = _pGfx->gl_pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, avtxLine, sizeof(CTVERTEX));
    D3D_CHECKERROR(hr);
  }
#endif // SE1_D3D
}



// draw border
void CDrawPort::DrawBorder( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight, COLOR col, ULONG typ/*=_FULL_*/) const
{
  // check API
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );

  // setup rendering mode
  gfxDisableDepthTest();
  gfxDisableDepthWrite();
  gfxDisableAlphaTest();
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  // for non-full lines, must have 
  FLOAT fD;
  INDEX iTexFilter, iTexAnisotropy;
  if( typ==_FULL_) {
    // no pattern - just disable texturing
    gfxDisableTexture(); 
    fD = 0;
  } else {
    // revert to simple point-sample filtering without mipmaps
    INDEX iNewFilter=10, iNewAnisotropy=1;
    gfxGetTextureFiltering( iTexFilter, iTexAnisotropy);
    gfxSetTextureFiltering( iNewFilter, iNewAnisotropy);
    // prepare line pattern
    extern void gfxSetPattern( ULONG ulPattern); 
    gfxSetPattern(typ);
    fD = Max( pixWidth, pixHeight) /32.0f;
  }

  // set line color/alpha and go go go 
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);
  const FLOAT fI0 = pixI+0.5f;
  const FLOAT fJ0 = pixJ+0.5f;
  const FLOAT fI1 = pixI-0.5f +pixWidth;
  const FLOAT fJ1 = pixJ-0.5f +pixHeight;

  // OpenGL
  if( eAPI==GAT_OGL) {
    glCOLOR(col);
#ifdef _GLES //karin: no glBegin/glEnd on GLES
	GLboolean ve = pglIsEnabled(GL_VERTEX_ARRAY);
	GLboolean te = pglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean ne = pglIsEnabled(GL_NORMAL_ARRAY);
	GLboolean ce = pglIsEnabled(GL_COLOR_ARRAY);
	if(!ve) pglEnableClientState(GL_VERTEX_ARRAY);
	if(te) pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglDisableClientState(GL_NORMAL_ARRAY);
	if(ce) pglDisableClientState(GL_COLOR_ARRAY);
	const GLfloat vs[] = {
      0,0, 0,fJ0,   fD,0,  1,  fJ0,  // up
      0,0, 1,fJ0,   fD,0,  1,  fJ1,  // right
      0,0, 0,fJ1,   fD,0,  1+1,fJ1,  // down
      0,0, 0,fJ0+1, fD,0,  0,  fJ1,  // left
	};
	pglVertexPointer(2, GL_FLOAT, 0, vs);
	pglDrawArrays(GL_LINES, 0, 16);
	if(!ve) pglDisableClientState(GL_VERTEX_ARRAY);
	if(te) pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglEnableClientState(GL_NORMAL_ARRAY);
	if(ce) pglEnableClientState(GL_COLOR_ARRAY);
#else
    pglBegin( GL_LINES);
      pglTexCoord2f(0,0); pglVertex2f(fI0,fJ0);   pglTexCoord2f(fD,0);  pglVertex2f(fI1,  fJ0);  // up
      pglTexCoord2f(0,0); pglVertex2f(fI1,fJ0);   pglTexCoord2f(fD,0);  pglVertex2f(fI1,  fJ1);  // right
      pglTexCoord2f(0,0); pglVertex2f(fI0,fJ1);   pglTexCoord2f(fD,0);  pglVertex2f(fI1+1,fJ1);  // down
      pglTexCoord2f(0,0); pglVertex2f(fI0,fJ0+1); pglTexCoord2f(fD,0);  pglVertex2f(fI0,  fJ1);  // left
    pglEnd();
#endif
    OGL_CHECKERROR;
  }
  // Direct3D
#ifdef SE1_D3D
  else if( eAPI==GAT_D3D) {
    HRESULT hr;
    const ULONG d3dColor = rgba2argb(col);
    CTVERTEX avtxLines[8] = { // setup lines
      {fI0,fJ0,  0, d3dColor, 0,0}, {fI1,  fJ0,0, d3dColor, fD,0},   // up
      {fI1,fJ0,  0, d3dColor, 0,0}, {fI1,  fJ1,0, d3dColor, fD,0},   // right
      {fI0,fJ1,  0, d3dColor, 0,0}, {fI1+1,fJ1,0, d3dColor, fD,0},   // down
      {fI0,fJ0+1,0, d3dColor, 0,0}, {fI0,  fJ1,0, d3dColor, fD,0} }; // left
    // set vertex shader and draw
    d3dSetVertexShader(D3DFVF_CTVERTEX);
    hr = _pGfx->gl_pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 4, avtxLines, sizeof(CTVERTEX));
    D3D_CHECKERROR(hr);
  }
#endif // SE1_D3D
  // revert to old filtering
  if( typ!=_FULL_) gfxSetTextureFiltering( iTexFilter, iTexAnisotropy);
}
 


// fill part of a drawport with a given color
void CDrawPort::Fill( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight, COLOR col) const
{
  // if color is tranlucent
  if( ((col&CT_AMASK)>>CT_ASHIFT) != CT_OPAQUE)
  { // draw thru polygon
    Fill( pixI,pixJ, pixWidth,pixHeight, col,col,col,col);
    return;
  }

  // clip and eventually reject
  const BOOL bInside = ClipToDrawPort( this, pixI, pixJ, pixWidth, pixHeight);
  if( !bInside) return;

  // draw thru fast clear for opaque colors
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);

  // check API
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );

  // OpenGL
  if( eAPI==GAT_OGL)
  { 
    // do fast filling
    SetScissor( this, pixI, pixJ, pixWidth, pixHeight);
    UBYTE ubR, ubG, ubB;
    ColorToRGB( col, ubR,ubG,ubB);
    pglClearColor( ubR/255.0f, ubG/255.0f, ubB/255.0f, 1.0f);
    pglClear( GL_COLOR_BUFFER_BIT);
    ResetScissor(this);
  }
  // Direct3D
#ifdef SE1_D3D
  else if( eAPI==GAT_D3D)
  {
    HRESULT hr;
    // must convert coordinates to raster (i.e. surface)
    pixI += dp_MinI;
    pixJ += dp_MinJ;
    const PIX pixRasterW = dp_Raster->ra_Width;
    const PIX pixRasterH = dp_Raster->ra_Height;
    const ULONG d3dColor = rgba2argb(col);
    // do fast filling
    if( pixI==0 && pixJ==0 && pixWidth==pixRasterW && pixHeight==pixRasterH) {
      hr = _pGfx->gl_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, d3dColor,0,0);
    } else {
      D3DRECT d3dRect = { pixI, pixJ, pixI+pixWidth, pixJ+pixHeight };
      hr = _pGfx->gl_pd3dDevice->Clear( 1, &d3dRect, D3DCLEAR_TARGET, d3dColor,0,0);
    } // done
    D3D_CHECKERROR(hr);
  }
#endif // SE1_D3D
}


// fill part of a drawport with a four corner colors
void CDrawPort::Fill( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight, 
                      COLOR colUL, COLOR colUR, COLOR colDL, COLOR colDR) const
{
  // clip and eventually reject
  const BOOL bInside = ClipToDrawPort( this, pixI, pixJ, pixWidth, pixHeight);
  if( !bInside) return;

  // check API
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );

  // setup rendering mode
  gfxDisableDepthTest();
  gfxDisableDepthWrite();
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  gfxDisableAlphaTest();
  gfxDisableTexture();
  // prepare colors and coords
  colUL = AdjustColor( colUL, _slTexHueShift, _slTexSaturation);
  colUR = AdjustColor( colUR, _slTexHueShift, _slTexSaturation);
  colDL = AdjustColor( colDL, _slTexHueShift, _slTexSaturation);
  colDR = AdjustColor( colDR, _slTexHueShift, _slTexSaturation);
  const FLOAT fI0 = pixI;  const FLOAT fI1 = pixI +pixWidth; 
  const FLOAT fJ0 = pixJ;  const FLOAT fJ1 = pixJ +pixHeight;

  // render rectangle
  if( eAPI==GAT_OGL) {
    // thru OpenGL
    gfxResetArrays();
    GFXVertex   *pvtx = _avtxCommon.Push(4);
    /* GFXTexCoord *ptex = */ _atexCommon.Push(4);
    GFXColor    *pcol = _acolCommon.Push(4);
    const GFXColor glcolUL(colUL);  const GFXColor glcolUR(colUR);
    const GFXColor glcolDL(colDL);  const GFXColor glcolDR(colDR);
    // add to element list and flush (the toilet!:)
    pvtx[0].x = fI0;  pvtx[0].y = fJ0;  pvtx[0].z = 0;  pcol[0] = glcolUL;
    pvtx[1].x = fI0;  pvtx[1].y = fJ1;  pvtx[1].z = 0;  pcol[1] = glcolDL;
    pvtx[2].x = fI1;  pvtx[2].y = fJ1;  pvtx[2].z = 0;  pcol[2] = glcolDR;
    pvtx[3].x = fI1;  pvtx[3].y = fJ0;  pvtx[3].z = 0;  pcol[3] = glcolUR;
    gfxFlushQuads();
  }
#ifdef SE1_D3D
  else if( eAPI==GAT_D3D) { 
    // thru Direct3D
    HRESULT hr;
    const ULONG d3dColUL = rgba2argb(colUL);  const ULONG d3dColUR = rgba2argb(colUR);
    const ULONG d3dColDL = rgba2argb(colDL);  const ULONG d3dColDR = rgba2argb(colDR);
    CTVERTEX avtxTris[6] = {
      {fI0,fJ0,0, d3dColUL, 0,0}, {fI0,fJ1,0, d3dColDL, 0,1}, {fI1,fJ1,0, d3dColDR, 1,1},
      {fI0,fJ0,0, d3dColUL, 0,0}, {fI1,fJ1,0, d3dColDR, 1,1}, {fI1,fJ0,0, d3dColUR, 1,0} };
    // set vertex shader and draw
    d3dSetVertexShader(D3DFVF_CTVERTEX);
    hr = _pGfx->gl_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLELIST, 2, avtxTris, sizeof(CTVERTEX));
    D3D_CHECKERROR(hr);
  }
#endif // SE1_D3D
}


// fill an entire drawport with a given color
void CDrawPort::Fill( COLOR col) const
{
  // if color is tranlucent
  if( ((col&CT_AMASK)>>CT_ASHIFT) != CT_OPAQUE)
  { // draw thru polygon
    Fill( 0,0, dp_Width,dp_Height, col,col,col,col);
    return;
  }

  // draw thru fast clear for opaque colors
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);

  // check API
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );

  // OpenGL
  if( eAPI==GAT_OGL)
  { 
    // do fast filling
    UBYTE ubR, ubG, ubB;
    ColorToRGB( col, ubR,ubG,ubB);
    pglClearColor( ubR/255.0f, ubG/255.0f, ubB/255.0f, 1.0f);
    pglClear( GL_COLOR_BUFFER_BIT);
  }

#ifdef PLATFORM_WIN32 // Direct3D
#ifdef SE1_D3D
  else if( eAPI==GAT_D3D)
  {
    const ULONG d3dColor = rgba2argb(col);
    HRESULT hr = _pGfx->gl_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, d3dColor,0,0);
    D3D_CHECKERROR(hr);
  }
#endif
#endif

}


// fill an part of Z-Buffer with a given value
void CDrawPort::FillZBuffer( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight, FLOAT zval) const
{ 
  // check API
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );

  // clip and eventually reject
  const BOOL bInside = ClipToDrawPort( this, pixI, pixJ, pixWidth, pixHeight);
  if( !bInside) return;

  gfxEnableDepthWrite();

  // OpenGL
  if( eAPI==GAT_OGL)
  {
    // fast clearing thru scissor
    SetScissor( this, pixI, pixJ, pixWidth, pixHeight);
    pglClearDepth(zval);
    pglClearStencil(0);
    // must clear stencil buffer too in case it exist (we don't need it) for the performance sake
    pglClear( GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    ResetScissor(this);
    OGL_CHECKERROR;
  }
  // Direct3D
#ifdef SE1_D3D
  else if( eAPI==GAT_D3D)
  {
    D3DRECT d3dRect = { pixI, pixJ, pixI+pixWidth, pixJ+pixHeight };
    HRESULT hr = _pGfx->gl_pd3dDevice->Clear( 1, &d3dRect, D3DCLEAR_ZBUFFER, 0,zval,0);
    D3D_CHECKERROR(hr);
  }
#endif //SE1_D3D
}


// fill an entire Z-Buffer with a given value
void CDrawPort::FillZBuffer( FLOAT zval) const
{ 
  // check API
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );
  gfxEnableDepthWrite();

  // OpenGL
  if( eAPI==GAT_OGL)
  {
    // fill whole z-buffer
    pglClearDepth(zval);
    pglClearStencil(0);
    pglClear( GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
  }
  // Direct3D
#ifdef SE1_D3D
  else if( eAPI==GAT_D3D)
  {
    HRESULT hr = _pGfx->gl_pd3dDevice->Clear( 0, NULL, D3DCLEAR_ZBUFFER, 0,zval,0);
    D3D_CHECKERROR(hr);
  }
#endif //SE1_D3D
}


// grab screen
void CDrawPort::GrabScreen( class CImageInfo &iiGrabbedImage, INDEX iGrabZBuffer/*=0*/) const
{
  // check API
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );
  extern INDEX ogl_bGrabDepthBuffer;
  const BOOL bGrabDepth = eAPI==GAT_OGL && ((iGrabZBuffer==1 && ogl_bGrabDepthBuffer) || iGrabZBuffer==2);

  // prepare image info's dimensions
  iiGrabbedImage.Clear();
  iiGrabbedImage.ii_Width  = dp_Width;
  iiGrabbedImage.ii_Height = dp_Height;
  iiGrabbedImage.ii_BitsPerPixel = bGrabDepth ? 32 : 24;

  // allocate memory for 24-bit raw picture and copy buffer context
  const PIX pixPicSize = iiGrabbedImage.ii_Width * iiGrabbedImage.ii_Height;
  const SLONG slBytes  = pixPicSize * iiGrabbedImage.ii_BitsPerPixel/8;
  iiGrabbedImage.ii_Picture = (UBYTE*)AllocMemory( slBytes);
  memset( iiGrabbedImage.ii_Picture, 128, slBytes);
  
  // OpenGL
  if( eAPI==GAT_OGL)
  {
    // determine drawport starting location inside raster
    const PIX pixStartI = dp_MinI;
    const PIX pixStartJ = dp_Raster->ra_Height-(dp_MinJ+dp_Height);
#ifdef _GLES //karin: only GL_RGBA
	if(iiGrabbedImage.ii_BitsPerPixel == 32)
	{
		pglReadPixels( pixStartI, pixStartJ, dp_Width, dp_Height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)iiGrabbedImage.ii_Picture);
	}
	else
	{
		UBYTE *pixelData = (UBYTE*)AllocMemory( pixPicSize * 4 );
		pglReadPixels( pixStartI, pixStartJ, dp_Width, dp_Height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pixelData);
		for(int h = 0; h < dp_Height; h++)
		{
			for(int w = 0; w < dp_Width; w++)
			{
				auto offset = h * dp_Width + w;
				memcpy(&iiGrabbedImage.ii_Picture[offset * 3], &pixelData[offset * 4], 3);
			}
		}
		FreeMemory(pixelData);
	}
#else
    pglReadPixels( pixStartI, pixStartJ, dp_Width, dp_Height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)iiGrabbedImage.ii_Picture);
#endif
    OGL_CHECKERROR;
    // grab z-buffer to alpha channel, if needed
    if( bGrabDepth) {
      // grab
      FLOAT *pfZBuffer = (FLOAT*)AllocMemory( pixPicSize*sizeof(FLOAT));
#if !defined(_GLES) //karin: not support read depth buffer
      pglReadPixels( pixStartI, pixStartJ, dp_Width, dp_Height, GL_DEPTH_COMPONENT, GL_FLOAT, (GLvoid*)pfZBuffer);
      OGL_CHECKERROR;
#else
	  memset(pfZBuffer, 0, sizeof(pixPicSize * sizeof(FLOAT)));
#endif
      // convert
      UBYTE *pubZBuffer = (UBYTE*)pfZBuffer;
      for( INDEX i=0; i<pixPicSize; i++) pubZBuffer[i] = 255-NormFloatToByte(pfZBuffer[i]);
      // add as alpha channel
      AddAlphaChannel( iiGrabbedImage.ii_Picture, (ULONG*)iiGrabbedImage.ii_Picture,
                       iiGrabbedImage.ii_Width * iiGrabbedImage.ii_Height, pubZBuffer);
      FreeMemory(pfZBuffer);
    }
    // flip image vertically  
    FlipBitmap( iiGrabbedImage.ii_Picture, iiGrabbedImage.ii_Picture,
                iiGrabbedImage.ii_Width, iiGrabbedImage.ii_Height, 1, iiGrabbedImage.ii_BitsPerPixel==32);
  }

  // Direct3D
#ifdef SE1_D3D
  else if( eAPI==GAT_D3D)
  {
    // get back buffer
    HRESULT hr;
    D3DLOCKED_RECT rectLocked;
    D3DSURFACE_DESC surfDesc;
    LPDIRECT3DSURFACE8 pBackBuffer;
    const BOOL bFullScreen = _pGfx->gl_ulFlags & GLF_FULLSCREEN;
    if( bFullScreen) hr = _pGfx->gl_pd3dDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    else hr = dp_Raster->ra_pvpViewPort->vp_pSwapChain->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    D3D_CHECKERROR(hr);
    pBackBuffer->GetDesc(&surfDesc);
    ASSERT( surfDesc.Width==dp_Raster->ra_Width && surfDesc.Height==dp_Raster->ra_Height);
    const RECT rectToLock = { dp_MinI, dp_MinJ, dp_MaxI+1, dp_MaxJ+1 };
    hr = pBackBuffer->LockRect( &rectLocked, &rectToLock, D3DLOCK_READONLY);
    D3D_CHECKERROR(hr);

    // prepare to copy'n'convert
    SLONG slColSize;    
    UBYTE *pubSrc = (UBYTE*)rectLocked.pBits;
    UBYTE *pubDst = iiGrabbedImage.ii_Picture;
    // loop thru rows
    for( INDEX j=0; j<dp_Height; j++) {
      // loop thru pixles in row
      for( INDEX i=0; i<dp_Width; i++) {
        UBYTE ubR,ubG,ubB;
        extern COLOR UnpackColor_D3D( UBYTE *pd3dColor, D3DFORMAT d3dFormat, SLONG &slColorSize);
        COLOR col = UnpackColor_D3D( pubSrc, surfDesc.Format, slColSize);
        ColorToRGB( col, ubR,ubG,ubB);
        *pubDst++ = ubR;
        *pubDst++ = ubG;
        *pubDst++ = ubB;
        pubSrc += slColSize;
      } // advance modulo
      pubSrc += rectLocked.Pitch - (dp_Width*slColSize);
    } // all done
    pBackBuffer->UnlockRect();
    D3DRELEASE( pBackBuffer, TRUE);
  }
#endif // SE1_D3D
}


BOOL CDrawPort::IsPointVisible( PIX pixI, PIX pixJ, FLOAT fOoK, INDEX iID, INDEX iMirrorLevel/*=0*/) const
{
  // must have raster!
  if( dp_Raster==NULL) { ASSERT(FALSE);  return FALSE; }

  // if the point is out or at the edge of drawport, it is not visible by default
  if( pixI<1 || pixI>dp_Width-2 || pixJ<1 || pixJ>dp_Height-2) return FALSE;

  #if defined(__arm__) || PLATFORM_RISCV64
  #warning PLATFORM_NOT_X86 use GLES based GPU (Lens Flare not work)
  // Assuming here that all ARM machine use GLES based GPU, were DEPTH reading is probably not available (or super slow)
  return FALSE;
  #endif

  // check API
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );

  // use delayed mechanism for checking
  extern BOOL CheckDepthPoint( const CDrawPort *pdp, PIX pixI, PIX pixJ, FLOAT fOoK, INDEX iID, INDEX iMirrorLevel=0);
  return CheckDepthPoint( this, pixI, pixJ, fOoK, iID, iMirrorLevel);
}


void CDrawPort::RenderLensFlare( CTextureObject *pto, FLOAT fI, FLOAT fJ,
                                 FLOAT fSizeI, FLOAT fSizeJ, ANGLE aRotation, COLOR colLight) const
{
  // check API
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );

  // setup rendering mode
  gfxEnableDepthTest();
  gfxDisableDepthWrite();
  gfxEnableBlend();
  gfxBlendFunc( GFX_ONE, GFX_ONE);
  gfxDisableAlphaTest();
  gfxResetArrays();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);

  // find lens location and dimension
  const FLOAT fRI   = fSizeI*0.5f;
  const FLOAT fRJ   = fSizeJ*0.5f;
  const FLOAT fSinA = SinFast(aRotation);
  const FLOAT fCosA = CosFast(aRotation);
  const FLOAT fRICosA = fRI * +fCosA;
  const FLOAT fRJSinA = fRJ * -fSinA;
  const FLOAT fRISinA = fRI * +fSinA;
  const FLOAT fRJCosA = fRJ * +fCosA;

  // get texture parameters for current frame and needed mip factor and upload texture
  CTextureData *ptd = (CTextureData*)pto->GetData();
  ptd->SetAsCurrent(pto->GetFrame());
  // set lens color
  colLight = AdjustColor( colLight, _slShdHueShift, _slShdSaturation);
  const GFXColor glcol(colLight);

  // prepare coordinates of the rectangle
  pvtx[0].x = fI- fRICosA+fRJSinA;  pvtx[0].y = fJ- fRISinA+fRJCosA;  pvtx[0].z = 0.01f;
  pvtx[1].x = fI- fRICosA-fRJSinA;  pvtx[1].y = fJ- fRISinA-fRJCosA;  pvtx[1].z = 0.01f;
  pvtx[2].x = fI+ fRICosA-fRJSinA;  pvtx[2].y = fJ+ fRISinA-fRJCosA;  pvtx[2].z = 0.01f;
  pvtx[3].x = fI+ fRICosA+fRJSinA;  pvtx[3].y = fJ+ fRISinA+fRJCosA;  pvtx[3].z = 0.01f;
  ptex[0].st.s = 0;  ptex[0].st.t = 0;
  ptex[1].st.s = 0;  ptex[1].st.t = 1;
  ptex[2].st.s = 1;  ptex[2].st.t = 1;
  ptex[3].st.s = 1;  ptex[3].st.t = 0;
  pcol[0] = glcol;
  pcol[1] = glcol;
  pcol[2] = glcol;
  pcol[3] = glcol;
  // render it
  _pGfx->gl_ctWorldTriangles += 2; 
  gfxFlushQuads();
}



/*******************************************************
 * Routines for manipulating drawport's text capabilites
 */


// sets font to be used to printout some text on this drawport 
// WARNING: this resets text spacing, scaling and mode variables
void CDrawPort::SetFont( CFontData *pfd)
{
  // check if we're using font that's not even loaded yet
  ASSERT( pfd!=NULL); 
  dp_FontData = pfd;
  dp_pixTextCharSpacing = pfd->fd_pixCharSpacing; 
  dp_pixTextLineSpacing = pfd->fd_pixLineSpacing;
  dp_fTextScaling = 1.0f;                         
  dp_fTextAspect  = 1.0f;
  dp_iTextMode    = 1;
};


// returns width of the longest line in text string
ULONG CDrawPort::GetTextWidth( const CTString &strText) const
{
  // prepare scaling factors
  PIX   pixCellWidth    = dp_FontData->fd_pixCharWidth;
  SLONG fixTextScalingX = FloatToInt(dp_fTextScaling*dp_fTextAspect*65536.0f);

  // calculate width of entire text line
  PIX pixStringWidth=0, pixOldWidth=0;
  PIX pixCharStart=0, pixCharEnd=pixCellWidth;
  //INDEX ctCharsPrinted=0;
  for( INDEX i=0; i<(INDEX)strlen(strText); i++)
  { // get current letter
    unsigned char chrCurrent = strText[i];
    // next line situation?
    if( chrCurrent == '\n') {
      if( pixOldWidth < pixStringWidth) pixOldWidth = pixStringWidth;
      pixStringWidth=0;
      continue;
    }
    // special char encountered and allowed?
    else if( chrCurrent=='^' && dp_iTextMode!=-1) {
      // get next char
      chrCurrent = strText[++i];
      switch( chrCurrent) {
      // skip corresponding number of characters
      case 'c':  i += FindZero((UBYTE*)&strText[i],6);  continue;
      case 'a':  i += FindZero((UBYTE*)&strText[i],2);  continue;
      case 'f':  i += 1;  continue;
      case 'b':  case 'i':  case 'r':  case 'o':
      case 'C':  case 'A':  case 'F':  case 'B':  case 'I':  i+=0;  continue;
      default:   break; // if we get here this means that ^ or an unrecognized special code was specified
      }
    }
    // ignore tab
    else if( chrCurrent == '\t') continue;

    // add current letter's width to result width
    if( !dp_FontData->fd_bFixedWidth) {
      // proportional font case
      pixCharStart = dp_FontData->fd_fcdFontCharData[chrCurrent].fcd_pixStart;
      pixCharEnd   = dp_FontData->fd_fcdFontCharData[chrCurrent].fcd_pixEnd;
    }
    pixStringWidth += (((pixCharEnd-pixCharStart)*fixTextScalingX)>>16) +dp_pixTextCharSpacing;
    //ctCharsPrinted++;
  }
  // determine largest width
  if( pixStringWidth < pixOldWidth) pixStringWidth = pixOldWidth;
  return pixStringWidth;
}


// writes text string on drawport (left aligned if not forced otherwise)
void CDrawPort::PutText( const CTString &strText, PIX pixX0, PIX pixY0, const COLOR colBlend) const
{
  // check API and adjust position for D3D by half pixel
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );

  // skip drawing if text falls above or below draw port
  if( pixY0>dp_Height || pixX0>dp_Width) return;
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_PUTTEXT);
  char acTmp[7]; // needed for strtoul()
  char *pcDummy; 
  INDEX iRet;

  // cache char and texture dimensions
  FLOAT fTextScalingX   = dp_fTextScaling*dp_fTextAspect;
  SLONG fixTextScalingX = FloatToInt(fTextScalingX  *65536.0f);
  SLONG fixTextScalingY = FloatToInt(dp_fTextScaling*65536.0f);
  PIX pixCellWidth  = dp_FontData->fd_pixCharWidth;
  PIX pixCharHeight = dp_FontData->fd_pixCharHeight-1;
  PIX pixScaledWidth  = (pixCellWidth *fixTextScalingX)>>16;
  PIX pixScaledHeight = (pixCharHeight*fixTextScalingY)>>16;

  // prepare font texture
  gfxSetTextureWrapping( GFX_REPEAT, GFX_REPEAT);
  CTextureData &td = *dp_FontData->fd_ptdTextureData;
  td.SetAsCurrent();
  // setup rendering mode
  gfxDisableDepthTest();
  gfxDisableDepthWrite();
  gfxDisableAlphaTest();
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  // calculate and apply correction factor
  FLOAT fCorrectionU = 1.0f / td.GetPixWidth();
  FLOAT fCorrectionV = 1.0f / td.GetPixHeight();
  INDEX ctMaxChars = (INDEX)strlen(strText);
  // determine text color
  GFXColor glcolDefault( AdjustColor( colBlend, _slTexHueShift, _slTexSaturation));
  GFXColor glcol = glcolDefault;
  ULONG ulAlphaDefault = (colBlend&CT_AMASK)>>CT_ASHIFT;;  // for flasher
  ASSERT( dp_iTextMode==-1 || dp_iTextMode==0 || dp_iTextMode==+1);

  // prepare some text control and output vars
  INDEX ctCharsPrinted=0, ctBoldsPrinted=0;
  BOOL bBold    = FALSE;
  BOOL bItalic  = FALSE;
  INDEX iFlash  = 0;
  ULONG ulAlpha = ulAlphaDefault;
  TIME tmFrame  = _pGfx->gl_tvFrameTime.GetSeconds();
  BOOL bParse = dp_iTextMode==1;

  // prepare arrays
  gfxResetArrays();
  GFXVertex   *pvtx = _avtxCommon.Push( 2*ctMaxChars*4);  // 2* because of bold
  GFXTexCoord *ptex = _atexCommon.Push( 2*ctMaxChars*4);
  GFXColor    *pcol = _acolCommon.Push( 2*ctMaxChars*4);

  // loop thru chars
  PIX pixAdvancer = ((pixCellWidth*fixTextScalingX)>>16) +dp_pixTextCharSpacing;
  PIX pixStartX = pixX0;
  for( INDEX iChar=0; iChar<ctMaxChars; iChar++)
  {
    // get current char
    unsigned char chrCurrent = strText[iChar];
    // if at end of current line
    if( chrCurrent=='\n') {
      // advance to next line
      pixX0  = pixStartX;
      pixY0 += pixScaledHeight+dp_pixTextLineSpacing;
      if( pixY0>dp_Height) break;
      // skip to next char
      continue;
    }
    // special char encountered and allowed?
    else if( chrCurrent=='^' && dp_iTextMode!=-1) {
      // get next char
      chrCurrent = strText[++iChar];
      COLOR col;
      switch( chrCurrent)
      {
      // color change?
      case 'c':
        strncpy( acTmp, &strText[iChar+1], 6);
        iRet = FindZero( (UBYTE*)&strText[iChar+1], 6);
        iChar+=iRet;
        if( !bParse || iRet<6) continue;
        acTmp[6] = '\0'; // terminate string
        col = strtoul( acTmp, &pcDummy, 16) <<8;
        col = AdjustColor( col, _slTexHueShift, _slTexSaturation);
        glcol.Set( col|glcol.ub.a); // do color change but keep original alpha
        continue;
      // alpha change?
      case 'a':
        strncpy( acTmp, &strText[iChar+1], 2);
        iRet = FindZero( (UBYTE*)&strText[iChar+1], 2);
        iChar+=iRet;
        if( !bParse || iRet<2) continue;
        acTmp[2] = '\0'; // terminate string
        ulAlpha = strtoul( acTmp, &pcDummy, 16);
        continue;
      // flash?
      case 'f':
        chrCurrent = strText[++iChar];
        if( bParse) iFlash = 1+ 2* Clamp( (INDEX)(chrCurrent-'0'), (INDEX)0, (INDEX)9);
        continue;
      // reset all?
      case 'r':
        bBold   = FALSE;
        bItalic = FALSE;
        iFlash  = 0;
        glcol   = glcolDefault;
        ulAlpha = ulAlphaDefault;
        continue;
      // simple codes ...
      case 'o':  bParse = bParse && gfx_bDecoratedText;  continue;  // allow console override settings?
      case 'b':  if( bParse) bBold   = TRUE;  continue;  // bold?
      case 'i':  if( bParse) bItalic = TRUE;  continue;  // italic?
      case 'C':  glcol   = glcolDefault;      continue;  // color reset?
      case 'A':  ulAlpha = ulAlphaDefault;    continue;  // alpha reset?
      case 'B':  bBold   = FALSE;             continue;  // no bold?
      case 'I':  bItalic = FALSE;             continue;  // italic?
      case 'F':  iFlash  = 0;                 continue;  // no flash?
      default:   break;
      } // unrecognized special code or just plain ^
      if( chrCurrent!='^') { iChar--; break; }
    }
    // ignore tab
    else if( chrCurrent=='\t') continue;

    // get current location and dimensions
    CFontCharData &fcdCurrent = dp_FontData->fd_fcdFontCharData[chrCurrent];
    PIX pixCharX = fcdCurrent.fcd_pixXOffset;
    PIX pixCharY = fcdCurrent.fcd_pixYOffset;
    PIX pixCharStart = fcdCurrent.fcd_pixStart;
    PIX pixCharEnd   = fcdCurrent.fcd_pixEnd;
    PIX pixXA; // adjusted starting X location of printout

    // determine corresponding char width and position adjustments
    if( dp_FontData->fd_bFixedWidth) {
      // for fixed font
      pixXA = pixX0 - ((pixCharStart*fixTextScalingX)>>16)
            + (((pixScaledWidth<<16) - ((pixCharEnd-pixCharStart)*fixTextScalingX) +0x10000) >>17);
    } else {
      // for proportional font
      pixXA = pixX0 - ((pixCharStart*fixTextScalingX)>>16);
      pixAdvancer = (((pixCharEnd-pixCharStart)*fixTextScalingX)>>16) +dp_pixTextCharSpacing;
    }
    // out of screen (left) ?
    if( pixXA>dp_Width || (pixXA+pixCharEnd)<0) {
      // skip to next char
      pixX0 += pixAdvancer;
      continue; 
    }

    // adjust alpha for flashing
    if( iFlash>0) glcol.ub.a = (UBYTE) (ulAlpha*(sin(iFlash*tmFrame)*0.5f+0.5f));
    else glcol.ub.a = ulAlpha; 

    // prepare coordinates for screen and texture
    const FLOAT fX0 = pixXA;  const FLOAT fX1 = fX0 +pixScaledWidth;
    const FLOAT fY0 = pixY0;  const FLOAT fY1 = fY0 +pixScaledHeight;
    const FLOAT fU0 = pixCharX *fCorrectionU;  const FLOAT fU1 = (pixCharX+pixCellWidth)  *fCorrectionU;
    const FLOAT fV0 = pixCharY *fCorrectionV;  const FLOAT fV1 = (pixCharY+pixCharHeight) *fCorrectionV;
    pvtx[0].x = fX0;  pvtx[0].y = fY0;  pvtx[0].z = 0;
    pvtx[1].x = fX0;  pvtx[1].y = fY1;  pvtx[1].z = 0;
    pvtx[2].x = fX1;  pvtx[2].y = fY1;  pvtx[2].z = 0;
    pvtx[3].x = fX1;  pvtx[3].y = fY0;  pvtx[3].z = 0;
    ptex[0].st.s = fU0;  ptex[0].st.t = fV0;
    ptex[1].st.s = fU0;  ptex[1].st.t = fV1;
    ptex[2].st.s = fU1;  ptex[2].st.t = fV1;
    ptex[3].st.s = fU1;  ptex[3].st.t = fV0;
    pcol[0] = glcol;
    pcol[1] = glcol;
    pcol[2] = glcol;
    pcol[3] = glcol;

    // adjust for italic
    if( bItalic) {
      const FLOAT fAdjustX = fTextScalingX * (fY1-fY0)*0.2f;  // 20% slanted
      pvtx[0].x += fAdjustX;
      pvtx[3].x += fAdjustX;
    }
    // advance to next vetrices group
    pvtx += 4;
    ptex += 4;
    pcol += 4;
    // add bold char
    if( bBold) {
      const FLOAT fAdjustX = fTextScalingX * ((FLOAT)pixCellWidth)*0.1f;  // 10% fat (extra light mayonnaise:)
      pvtx[0].x = pvtx[0-4].x +fAdjustX;  pvtx[0].y = fY0;  pvtx[0].z = 0;
      pvtx[1].x = pvtx[1-4].x +fAdjustX;  pvtx[1].y = fY1;  pvtx[1].z = 0;
      pvtx[2].x = pvtx[2-4].x +fAdjustX;  pvtx[2].y = fY1;  pvtx[2].z = 0;
      pvtx[3].x = pvtx[3-4].x +fAdjustX;  pvtx[3].y = fY0;  pvtx[3].z = 0;
      ptex[0].st.s = fU0;    ptex[0].st.t = fV0;
      ptex[1].st.s = fU0;    ptex[1].st.t = fV1;
      ptex[2].st.s = fU1;    ptex[2].st.t = fV1;
      ptex[3].st.s = fU1;    ptex[3].st.t = fV0;
      pcol[0] = glcol;
      pcol[1] = glcol;
      pcol[2] = glcol;
      pcol[3] = glcol;
      pvtx += 4;
      ptex += 4;
      pcol += 4;
      ctBoldsPrinted++;
    }
    // advance to next char
    pixX0 += pixAdvancer;
    ctCharsPrinted++;
  }

  // adjust vertex arrays size according to chars that really got printed out
  ctCharsPrinted += ctBoldsPrinted;
  _avtxCommon.PopUntil( ctCharsPrinted*4-1);
  _atexCommon.PopUntil( ctCharsPrinted*4-1);
  _acolCommon.PopUntil( ctCharsPrinted*4-1);
  gfxFlushQuads();

  // all done
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_PUTTEXT);
}



// writes text string on drawport (centered arround X)
void CDrawPort::PutTextC( const CTString &strText, PIX pixX0, PIX pixY0,
                          const COLOR colBlend/*=0xFFFFFFFF*/) const
{
  PutText( strText, pixX0-GetTextWidth(strText)/2, pixY0, colBlend);
}

// writes text string on drawport (centered arround X and Y)
void CDrawPort::PutTextCXY( const CTString &strText, PIX pixX0, PIX pixY0,
                            const COLOR colBlend/*=0xFFFFFFFF*/) const
{
  PIX pixTextWidth  = GetTextWidth(strText);
  PIX pixTextHeight = (PIX) (dp_FontData->fd_pixCharHeight * dp_fTextScaling);
  PutText( strText, pixX0-pixTextWidth/2, pixY0-pixTextHeight/2, colBlend);
}

// writes text string on drawport (right-aligned)
void CDrawPort::PutTextR( const CTString &strText, PIX pixX0, PIX pixY0,
                          const COLOR colBlend/*=0xFFFFFFFF*/) const
{
  PutText( strText, pixX0-GetTextWidth(strText), pixY0, colBlend);
}


/**********************************************************
 * Routines for putting and getting textures strictly in 2D
 */

void CDrawPort::PutTexture( class CTextureObject *pTO, const PIXaabbox2D &boxScreen,
                            const COLOR colBlend/*=0xFFFFFFFF*/) const
{
  PutTexture( pTO, boxScreen, colBlend, colBlend, colBlend, colBlend);
}

void CDrawPort::PutTexture( class CTextureObject *pTO, const PIXaabbox2D &boxScreen,
                            const MEXaabbox2D &boxTexture, const COLOR colBlend/*=0xFFFFFFFF*/) const
{
  PutTexture( pTO, boxScreen, boxTexture, colBlend, colBlend, colBlend, colBlend);
}

void CDrawPort::PutTexture( class CTextureObject *pTO, const PIXaabbox2D &boxScreen,
                            const COLOR colUL, const COLOR colUR, const COLOR colDL, const COLOR colDR) const
{
  MEXaabbox2D boxTexture( MEX2D(0,0), MEX2D(pTO->GetWidth(), pTO->GetHeight()));
  PutTexture( pTO, boxScreen, boxTexture, colUL, colUR, colDL, colDR);
}

// complete put texture routine
void CDrawPort::PutTexture( class CTextureObject *pTO,
                            const PIXaabbox2D &boxScreen, const MEXaabbox2D &boxTexture,
                            const COLOR colUL, const COLOR colUR, const COLOR colDL, const COLOR colDR) const
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_PUTTEXTURE);

  // extract screen and texture coordinates
  const PIX pixI0 = boxScreen.Min()(1);  const PIX pixI1 = boxScreen.Max()(1);
  const PIX pixJ0 = boxScreen.Min()(2);  const PIX pixJ1 = boxScreen.Max()(2);

  // if whole texture is out of drawport
  if( pixI0>dp_Width || pixJ0>dp_Height || pixI1<0 || pixJ1<0) {
    // skip it (just to reduce OpenGL call overhead)
    _pfGfxProfile.StopTimer( CGfxProfile::PTI_PUTTEXTURE);
    return;
  }

  // check API and adjust position for D3D by half pixel
  const GfxAPIType eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );
  FLOAT fI0 = pixI0;  FLOAT fI1 = pixI1;
  FLOAT fJ0 = pixJ0;  FLOAT fJ1 = pixJ1;

  // prepare texture
  gfxSetTextureWrapping( GFX_REPEAT, GFX_REPEAT);
  CTextureData *ptd = (CTextureData*)pTO->GetData();
  ptd->SetAsCurrent(pTO->GetFrame());

  // setup rendering mode
  gfxDisableDepthTest();
  gfxDisableDepthWrite();
  gfxDisableAlphaTest();
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  gfxResetArrays();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);

  // extract texture coordinates and apply correction factor
  const PIX pixWidth  = ptd->GetPixWidth();
  const PIX pixHeight = ptd->GetPixHeight();
  FLOAT fCorrectionU, fCorrectionV;
  (SLONG&)fCorrectionU = (127-FastLog2(pixWidth))  <<23; // fCorrectionU = 1.0f / ptd->GetPixWidth() 
  (SLONG&)fCorrectionV = (127-FastLog2(pixHeight)) <<23; // fCorrectionV = 1.0f / ptd->GetPixHeight() 
  FLOAT fU0 = (boxTexture.Min()(1)>>ptd->td_iFirstMipLevel) *fCorrectionU;
  FLOAT fU1 = (boxTexture.Max()(1)>>ptd->td_iFirstMipLevel) *fCorrectionU;
  FLOAT fV0 = (boxTexture.Min()(2)>>ptd->td_iFirstMipLevel) *fCorrectionV;
  FLOAT fV1 = (boxTexture.Max()(2)>>ptd->td_iFirstMipLevel) *fCorrectionV;

  // if not tiled
  const BOOL bTiled = Abs(fU0-fU1)>1 || Abs(fV0-fV1)>1;
  if( !bTiled) {
    // slight adjust for sub-pixel precision
    fU0 += +0.25f *fCorrectionU;
    fU1 += -0.25f *fCorrectionU;
    fV0 += +0.25f *fCorrectionV;
    fV1 += -0.25f *fCorrectionV;
  }
  // prepare colors
  const GFXColor glcolUL( AdjustColor( colUL, _slTexHueShift, _slTexSaturation));
  const GFXColor glcolUR( AdjustColor( colUR, _slTexHueShift, _slTexSaturation));
  const GFXColor glcolDL( AdjustColor( colDL, _slTexHueShift, _slTexSaturation));
  const GFXColor glcolDR( AdjustColor( colDR, _slTexHueShift, _slTexSaturation));

  // prepare coordinates of the rectangle
  pvtx[0].x = fI0;  pvtx[0].y = fJ0;  pvtx[0].z = 0;
  pvtx[1].x = fI0;  pvtx[1].y = fJ1;  pvtx[1].z = 0;
  pvtx[2].x = fI1;  pvtx[2].y = fJ1;  pvtx[2].z = 0;
  pvtx[3].x = fI1;  pvtx[3].y = fJ0;  pvtx[3].z = 0;
  ptex[0].st.s = fU0;  ptex[0].st.t = fV0;
  ptex[1].st.s = fU0;  ptex[1].st.t = fV1;
  ptex[2].st.s = fU1;  ptex[2].st.t = fV1;
  ptex[3].st.s = fU1;  ptex[3].st.t = fV0;
  pcol[0] = glcolUL;
  pcol[1] = glcolDL;
  pcol[2] = glcolDR;
  pcol[3] = glcolUR;
  gfxFlushQuads();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_PUTTEXTURE);
}



// prepares texture and rendering arrays
void CDrawPort::InitTexture( class CTextureObject *pTO, const BOOL bClamp/*=FALSE*/) const
{
  // prepare
  if( pTO!=NULL) {
    // has texture
    CTextureData *ptd = (CTextureData*)pTO->GetData();
    GfxWrap eWrap = GFX_REPEAT;
    if( bClamp) eWrap = GFX_CLAMP;
    gfxSetTextureWrapping( eWrap, eWrap);
    ptd->SetAsCurrent(pTO->GetFrame());
  } else {
    // no texture
    gfxDisableTexture();
  }
  // setup rendering mode
  gfxDisableDepthTest();
  gfxDisableDepthWrite();
  gfxDisableAlphaTest();
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  // prepare arrays
  gfxResetArrays();
}



// adds one full texture to rendering queue
void CDrawPort::AddTexture( const FLOAT fI0, const FLOAT fJ0, const FLOAT fI1, const FLOAT fJ1, const COLOR col) const
{
  const GFXColor glCol( AdjustColor( col, _slTexHueShift, _slTexSaturation));
  const INDEX iStart = _avtxCommon.Count();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);
  INDEX_T     *pelm = _aiCommonElements.Push(6);
  pvtx[0].x = fI0;  pvtx[0].y = fJ0;  pvtx[0].z = 0;
  pvtx[1].x = fI0;  pvtx[1].y = fJ1;  pvtx[1].z = 0;
  pvtx[2].x = fI1;  pvtx[2].y = fJ1;  pvtx[2].z = 0;
  pvtx[3].x = fI1;  pvtx[3].y = fJ0;  pvtx[3].z = 0;
  ptex[0].st.s = 0;    ptex[0].st.t = 0;
  ptex[1].st.s = 0;    ptex[1].st.t = 1;
  ptex[2].st.s = 1;    ptex[2].st.t = 1;
  ptex[3].st.s = 1;    ptex[3].st.t = 0;
  pcol[0] = glCol;  pcol[1] = glCol;  pcol[2] = glCol;  pcol[3] = glCol;
  pelm[0] = iStart+0;  pelm[1] = iStart+1;  pelm[2] = iStart+2;
  pelm[3] = iStart+2;  pelm[4] = iStart+3;  pelm[5] = iStart+0;
}


// adds one part of texture to rendering queue
void CDrawPort::AddTexture( const FLOAT fI0, const FLOAT fJ0, const FLOAT fI1, const FLOAT fJ1, 
                            const FLOAT fU0, const FLOAT fV0, const FLOAT fU1, const FLOAT fV1, const COLOR col) const
{
  const GFXColor glCol( AdjustColor( col, _slTexHueShift, _slTexSaturation));
  const INDEX iStart = _avtxCommon.Count();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);
  INDEX_T     *pelm = _aiCommonElements.Push(6);
  pvtx[0].x = fI0;  pvtx[0].y = fJ0;  pvtx[0].z = 0;
  pvtx[1].x = fI0;  pvtx[1].y = fJ1;  pvtx[1].z = 0;
  pvtx[2].x = fI1;  pvtx[2].y = fJ1;  pvtx[2].z = 0;
  pvtx[3].x = fI1;  pvtx[3].y = fJ0;  pvtx[3].z = 0;
  ptex[0].st.s = fU0;  ptex[0].st.t = fV0;
  ptex[1].st.s = fU0;  ptex[1].st.t = fV1;
  ptex[2].st.s = fU1;  ptex[2].st.t = fV1;
  ptex[3].st.s = fU1;  ptex[3].st.t = fV0;
  pcol[0] = glCol;
  pcol[1] = glCol;
  pcol[2] = glCol;
  pcol[3] = glCol;
  pelm[0] = iStart+0;  pelm[1] = iStart+1;  pelm[2] = iStart+2;
  pelm[3] = iStart+2;  pelm[4] = iStart+3;  pelm[5] = iStart+0;
}


// adds one triangle to rendering queue
void CDrawPort::AddTriangle( const FLOAT fI0, const FLOAT fJ0,
                             const FLOAT fI1, const FLOAT fJ1,
                             const FLOAT fI2, const FLOAT fJ2, const COLOR col) const
{
  const GFXColor glCol( AdjustColor( col, _slTexHueShift, _slTexSaturation));
  const INDEX iStart = _avtxCommon.Count();
  GFXVertex   *pvtx = _avtxCommon.Push(3);
  /* GFXTexCoord *ptex = */ _atexCommon.Push(3);
  GFXColor    *pcol = _acolCommon.Push(3);
  INDEX_T     *pelm = _aiCommonElements.Push(3);
  pvtx[0].x = fI0;  pvtx[0].y = fJ0;  pvtx[0].z = 0;
  pvtx[1].x = fI1;  pvtx[1].y = fJ1;  pvtx[1].z = 0;
  pvtx[2].x = fI2;  pvtx[2].y = fJ2;  pvtx[2].z = 0;
  pcol[0] = glCol;
  pcol[1] = glCol;
  pcol[2] = glCol;
  pelm[0] = iStart+0;
  pelm[1] = iStart+1;
  pelm[2] = iStart+2;
}


// adds one textured quad (up-left start, counter-clockwise)
void CDrawPort::AddTexture( const FLOAT fI0, const FLOAT fJ0, const FLOAT fU0, const FLOAT fV0, const COLOR col0,
                            const FLOAT fI1, const FLOAT fJ1, const FLOAT fU1, const FLOAT fV1, const COLOR col1,
                            const FLOAT fI2, const FLOAT fJ2, const FLOAT fU2, const FLOAT fV2, const COLOR col2,
                            const FLOAT fI3, const FLOAT fJ3, const FLOAT fU3, const FLOAT fV3, const COLOR col3) const
{
  const GFXColor glCol0( AdjustColor( col0, _slTexHueShift, _slTexSaturation));
  const GFXColor glCol1( AdjustColor( col1, _slTexHueShift, _slTexSaturation));
  const GFXColor glCol2( AdjustColor( col2, _slTexHueShift, _slTexSaturation));
  const GFXColor glCol3( AdjustColor( col3, _slTexHueShift, _slTexSaturation));
  const INDEX iStart = _avtxCommon.Count();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);
  INDEX_T     *pelm = _aiCommonElements.Push(6);
  pvtx[0].x = fI0;  pvtx[0].y = fJ0;  pvtx[0].z = 0;
  pvtx[1].x = fI1;  pvtx[1].y = fJ1;  pvtx[1].z = 0;
  pvtx[2].x = fI2;  pvtx[2].y = fJ2;  pvtx[2].z = 0;
  pvtx[3].x = fI3;  pvtx[3].y = fJ3;  pvtx[3].z = 0;
  ptex[0].st.s = fU0;  ptex[0].st.t = fV0;
  ptex[1].st.s = fU1;  ptex[1].st.t = fV1;
  ptex[2].st.s = fU2;  ptex[2].st.t = fV2;
  ptex[3].st.s = fU3;  ptex[3].st.t = fV3;
  pcol[0] = glCol0;
  pcol[1] = glCol1;
  pcol[2] = glCol2;
  pcol[3] = glCol3;
  pelm[0] = iStart+0;  pelm[1] = iStart+1;  pelm[2] = iStart+2;
  pelm[3] = iStart+2;  pelm[4] = iStart+3;  pelm[5] = iStart+0;
}


// renders all textures from rendering queue and flushed rendering arrays
void CDrawPort::FlushRenderingQueue(void) const
{ 
  gfxFlushElements(); 
  gfxResetArrays(); 
}



// blends screen with accumulation color
void CDrawPort::BlendScreen(void)
{
  if( dp_ulBlendingA==0) return;

  ULONG fix1oA = 65536 / dp_ulBlendingA;
  ULONG ulRA = (dp_ulBlendingRA*fix1oA)>>16;
  ULONG ulGA = (dp_ulBlendingGA*fix1oA)>>16;
  ULONG ulBA = (dp_ulBlendingBA*fix1oA)>>16;
  ULONG ulA  = ClampUp( dp_ulBlendingA, (ULONG) 255);
  COLOR colBlending = RGBAToColor( ulRA, ulGA, ulBA, ulA);
                                    
  // blend drawport (thru z-buffer because of elimination of pixel artefacts)
  gfxEnableDepthTest();
  gfxDisableDepthWrite();
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  gfxDisableAlphaTest();
  gfxDisableTexture();
  // prepare color
  colBlending = AdjustColor( colBlending, _slTexHueShift, _slTexSaturation);
  GFXColor glcol(colBlending);

  // set arrays
  gfxResetArrays();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  /* GFXTexCoord *ptex = */ _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);
  const INDEX iW = dp_Width;
  const INDEX iH = dp_Height;
  pvtx[0].x =  0;  pvtx[0].y =  0;  pvtx[0].z = 0.01f;
  pvtx[1].x =  0;  pvtx[1].y = iH;  pvtx[1].z = 0.01f;
  pvtx[2].x = iW;  pvtx[2].y = iH;  pvtx[2].z = 0.01f;
  pvtx[3].x = iW;  pvtx[3].y =  0;  pvtx[3].z = 0.01f;
  pcol[0] = glcol;
  pcol[1] = glcol;
  pcol[2] = glcol;
  pcol[3] = glcol;
  gfxFlushQuads();
  // reset accumulation color
  dp_ulBlendingRA = 0;
  dp_ulBlendingGA = 0;
  dp_ulBlendingBA = 0;
  dp_ulBlendingA  = 0;
}

