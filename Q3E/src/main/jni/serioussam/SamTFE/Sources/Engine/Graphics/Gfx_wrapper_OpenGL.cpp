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



// ENABLE/DISABLE FUNCTIONS


static void ogl_EnableTexture(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes;
  bRes = pglIsEnabled(GL_TEXTURE_2D);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_abTexture[GFX_iActiveTexUnit]);
#endif

  // cached?
  if( GFX_abTexture[GFX_iActiveTexUnit] && gap_bOptimizeStateChanges) return;
  GFX_abTexture[GFX_iActiveTexUnit] = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglEnable(GL_TEXTURE_2D);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void ogl_DisableTexture(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  bRes = pglIsEnabled(GL_TEXTURE_2D);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_abTexture[GFX_iActiveTexUnit]);
#endif

  // cached?
  if( !GFX_abTexture[GFX_iActiveTexUnit] && gap_bOptimizeStateChanges) return;
  GFX_abTexture[GFX_iActiveTexUnit] = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDisable(GL_TEXTURE_2D);
  pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void ogl_EnableDepthTest(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes;
  bRes = pglIsEnabled(GL_DEPTH_TEST);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bDepthTest);
#endif
  // cached?
  if( GFX_bDepthTest && gap_bOptimizeStateChanges) return;
  GFX_bDepthTest = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglEnable( GL_DEPTH_TEST);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}


static void ogl_DisableDepthTest(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  bRes = pglIsEnabled(GL_DEPTH_TEST);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bDepthTest);
#endif

  // cached?
  if( !GFX_bDepthTest && gap_bOptimizeStateChanges) return;
  GFX_bDepthTest = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDisable(GL_DEPTH_TEST);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}


static void ogl_EnableDepthBias(void)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglEnable( GL_POLYGON_OFFSET_POINT);
  pglEnable( GL_POLYGON_OFFSET_LINE);
  pglEnable( GL_POLYGON_OFFSET_FILL);
  pglPolygonOffset( -1.0f, -2.0f);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}


static void ogl_DisableDepthBias(void)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDisable( GL_POLYGON_OFFSET_POINT);
  pglDisable( GL_POLYGON_OFFSET_LINE);
  pglDisable( GL_POLYGON_OFFSET_FILL);
  pglPolygonOffset( 0.0f, 0.0f);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void ogl_EnableDepthWrite(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  pglGetIntegerv( GL_DEPTH_WRITEMASK, (GLint*)&bRes);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bDepthWrite);
#endif

  // cached?
  if( GFX_bDepthWrite && gap_bOptimizeStateChanges) return;
  GFX_bDepthWrite = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDepthMask(GL_TRUE);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void ogl_DisableDepthWrite(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  pglGetIntegerv( GL_DEPTH_WRITEMASK, (GLint*)&bRes);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bDepthWrite);
#endif

  // cached?
  if( !GFX_bDepthWrite && gap_bOptimizeStateChanges) return;
  GFX_bDepthWrite = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDepthMask(GL_FALSE);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void ogl_EnableDither(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes;
  bRes = pglIsEnabled(GL_DITHER);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bDithering);
#endif

  // cached?
  if( GFX_bDithering && gap_bOptimizeStateChanges) return;
  GFX_bDithering = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglEnable(GL_DITHER);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void ogl_DisableDither(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  bRes = pglIsEnabled(GL_DITHER);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bDithering);
#endif

  // cached?
  if( !GFX_bDithering && gap_bOptimizeStateChanges) return;
  GFX_bDithering = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDisable(GL_DITHER);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void ogl_EnableAlphaTest(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes;
  bRes = pglIsEnabled(GL_ALPHA_TEST);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bAlphaTest);
#endif

  // cached?
  if( GFX_bAlphaTest && gap_bOptimizeStateChanges) return;
  GFX_bAlphaTest = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglEnable(GL_ALPHA_TEST);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void ogl_DisableAlphaTest(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  bRes = pglIsEnabled(GL_ALPHA_TEST);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bAlphaTest);
#endif

  // cached?
  if( !GFX_bAlphaTest && gap_bOptimizeStateChanges) return;
  GFX_bAlphaTest = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDisable(GL_ALPHA_TEST);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void ogl_EnableBlend(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes;
  bRes = pglIsEnabled(GL_BLEND);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bBlending);
#endif
  // cached?
  if( GFX_bBlending && gap_bOptimizeStateChanges) return;
  GFX_bBlending = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglEnable( GL_BLEND);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);

  // adjust dithering
  if( gap_iDithering==2) ogl_EnableDither();
  else ogl_DisableDither();
}



static void ogl_DisableBlend(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  bRes = pglIsEnabled(GL_BLEND);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bBlending);
#endif

  // cached?
  if( !GFX_bBlending && gap_bOptimizeStateChanges) return;
  GFX_bBlending = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDisable(GL_BLEND);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);

  // adjust dithering
  if( gap_iDithering==0) ogl_DisableDither();
  else ogl_EnableDither();
}



static void ogl_EnableClipping(void)
{
  // only if supported
  if( !(_pGfx->gl_ulFlags&GLF_EXT_CLIPHINT)) return;

  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes;
  pglGetIntegerv( GL_CLIP_VOLUME_CLIPPING_HINT_EXT, (GLint*)&bRes);
  bRes = (bRes==GL_FASTEST) ? FALSE : TRUE;
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bClipping);
#endif

  // cached?
  if( GFX_bClipping && gap_bOptimizeStateChanges) return;
  GFX_bClipping = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglHint( GL_CLIP_VOLUME_CLIPPING_HINT_EXT, GL_DONT_CARE);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void ogl_DisableClipping(void)
{
  // only if allowed and supported
  if( gap_iOptimizeClipping<2 || !(_pGfx->gl_ulFlags&GLF_EXT_CLIPHINT)) return;

  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes;
  pglGetIntegerv( GL_CLIP_VOLUME_CLIPPING_HINT_EXT, (GLint*)&bRes);
  bRes = (bRes==GL_FASTEST) ? FALSE : TRUE;
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bClipping);
#endif

  // cached?
  if( !GFX_bClipping && gap_bOptimizeStateChanges) return;
  GFX_bClipping = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglHint( GL_CLIP_VOLUME_CLIPPING_HINT_EXT, GL_FASTEST);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}




static void ogl_EnableClipPlane(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  bRes = pglIsEnabled(GL_CLIP_PLANE0);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bClipPlane);
#endif
  // cached?
  if( GFX_bClipPlane && gap_bOptimizeStateChanges) return;
  GFX_bClipPlane = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglEnable( GL_CLIP_PLANE0);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



static void ogl_DisableClipPlane(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  bRes = pglIsEnabled(GL_CLIP_PLANE0);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bClipPlane);
#endif
  // cached?
  if( !GFX_bClipPlane && gap_bOptimizeStateChanges) return;
  GFX_bClipPlane = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDisable(GL_CLIP_PLANE0);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// enable usage of color array for subsequent rendering
static void ogl_EnableColorArray(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  bRes = pglIsEnabled(GL_COLOR_ARRAY);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bColorArray);
#endif

  // cached?
  if( GFX_bColorArray && gap_bOptimizeStateChanges) return;
  GFX_bColorArray = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglEnableClientState(GL_COLOR_ARRAY);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// enable usage of constant color for subsequent rendering
static void ogl_DisableColorArray(void)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  bRes = pglIsEnabled(GL_COLOR_ARRAY);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bColorArray);
#endif

  // cached?
  if( !GFX_bColorArray && gap_bOptimizeStateChanges) return;
  GFX_bColorArray = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDisableClientState(GL_COLOR_ARRAY);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// enable truform rendering
static void ogl_EnableTruform(void)
{
  // skip if Truform isn't set
  if( truform_iLevel<1) return;

  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  bRes = pglIsEnabled(GL_PN_TRIANGLES_ATI);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bTruform);
#endif

  if( GFX_bTruform && gap_bOptimizeStateChanges) return;
  GFX_bTruform = TRUE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglEnable( GL_PN_TRIANGLES_ATI); 
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// disable truform rendering
static void ogl_DisableTruform(void)
{
  // skip if Truform isn't set
  if( truform_iLevel<1) return;

  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  BOOL bRes; 
  bRes = pglIsEnabled(GL_PN_TRIANGLES_ATI);
  OGL_CHECKERROR;
  ASSERT( !bRes == !GFX_bTruform);
#endif

  if( !GFX_bTruform && gap_bOptimizeStateChanges) return;
  GFX_bTruform = FALSE;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDisable( GL_PN_TRIANGLES_ATI); 
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// helper for blending operation function
__forceinline GLenum BlendToOGL( GfxBlend eFunc) {
  switch( eFunc) {
  case GFX_ZERO:          return GL_ZERO;
  case GFX_ONE:           return GL_ONE;
  case GFX_SRC_COLOR:     return GL_SRC_COLOR;
  case GFX_INV_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
  case GFX_DST_COLOR:     return GL_DST_COLOR;          
  case GFX_INV_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
  case GFX_SRC_ALPHA:     return GL_SRC_ALPHA;          
  case GFX_INV_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
  default: ASSERTALWAYS("Invalid GFX blending function!");
  } return GL_ONE;
}

__forceinline GfxBlend BlendFromOGL( GLenum gFunc) {
  switch( gFunc) {
  case GL_ZERO:                return GFX_ZERO;
  case GL_ONE:                 return GFX_ONE;
  case GL_SRC_COLOR:           return GFX_SRC_COLOR;
  case GL_ONE_MINUS_SRC_COLOR: return GFX_INV_SRC_COLOR;
  case GL_DST_COLOR:           return GFX_DST_COLOR;
  case GL_ONE_MINUS_DST_COLOR: return GFX_INV_DST_COLOR;
  case GL_SRC_ALPHA:           return GFX_SRC_ALPHA;
  case GL_ONE_MINUS_SRC_ALPHA: return GFX_INV_SRC_ALPHA;
  default: ASSERTALWAYS("Unsupported OGL blending function!");
  } return GFX_ONE;
}


// set blending operations
static void ogl_BlendFunc( GfxBlend eSrc, GfxBlend eDst)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  GLenum gleSrc, gleDst;
#ifndef NDEBUG
  GfxBlend gfxSrc, gfxDst; 
  pglGetIntegerv( GL_BLEND_SRC, (GLint*)&gleSrc);
  pglGetIntegerv( GL_BLEND_DST, (GLint*)&gleDst);
  OGL_CHECKERROR;
  gfxSrc = BlendFromOGL(gleSrc);
  gfxDst = BlendFromOGL(gleDst);
  ASSERT( gfxSrc==GFX_eBlendSrc && gfxDst==GFX_eBlendDst);
#endif
  // cached?
  if( eSrc==GFX_eBlendSrc && eDst==GFX_eBlendDst && gap_bOptimizeStateChanges) return;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  gleSrc = BlendToOGL(eSrc);
  gleDst = BlendToOGL(eDst);
  pglBlendFunc( gleSrc, gleDst);
  OGL_CHECKERROR;
  // done
  GFX_eBlendSrc = eSrc;
  GFX_eBlendDst = eDst;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// color buffer writing enable
static void ogl_SetColorMask( ULONG ulColorMask)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  _ulCurrentColorMask = ulColorMask; // keep for Get...()
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  const BOOL bR = (ulColorMask&CT_RMASK) == CT_RMASK;
  const BOOL bG = (ulColorMask&CT_GMASK) == CT_GMASK;
  const BOOL bB = (ulColorMask&CT_BMASK) == CT_BMASK;
  const BOOL bA = (ulColorMask&CT_AMASK) == CT_AMASK;
  pglColorMask( bR,bG,bB,bA);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// helper for depth compare function
__forceinline GLenum CompToOGL( GfxComp eFunc) {
  switch( eFunc) {
  case GFX_NEVER:         return GL_NEVER;
  case GFX_LESS:          return GL_LESS;
  case GFX_LESS_EQUAL:    return GL_LEQUAL;
  case GFX_EQUAL:         return GL_EQUAL;
  case GFX_NOT_EQUAL:     return GL_NOTEQUAL;
  case GFX_GREATER_EQUAL: return GL_GEQUAL;
  case GFX_GREATER:       return GL_GREATER;
  case GFX_ALWAYS:        return GL_ALWAYS;
  default: ASSERTALWAYS("Invalid GFX compare function!");
  } return GL_ALWAYS;
}
  
__forceinline GfxComp CompFromOGL( GLenum gFunc) {
  switch( gFunc) {
  case GL_NEVER:    return GFX_NEVER;
  case GL_LESS:     return GFX_LESS;
  case GL_LEQUAL:   return GFX_LESS_EQUAL;
  case GL_EQUAL:    return GFX_EQUAL;
  case GL_NOTEQUAL: return GFX_NOT_EQUAL;
  case GL_GEQUAL:   return GFX_GREATER_EQUAL;
  case GL_GREATER:  return GFX_GREATER;
  case GL_ALWAYS:   return GFX_ALWAYS;
  default: ASSERTALWAYS("Invalid OGL compare function!");
  } return GFX_ALWAYS;
}



// set depth buffer compare mode
static void ogl_DepthFunc( GfxComp eFunc)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  GLenum gleFunc;
#ifndef NDEBUG
  GfxComp gfxFunc; 
  pglGetIntegerv( GL_DEPTH_FUNC, (GLint*)&gleFunc);
  OGL_CHECKERROR;
  gfxFunc = CompFromOGL( gleFunc);
  ASSERT( gfxFunc==GFX_eDepthFunc);
#endif
  // cached?
  if( eFunc==GFX_eDepthFunc && gap_bOptimizeStateChanges) return;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  gleFunc = CompToOGL(eFunc);
  pglDepthFunc(gleFunc);
  OGL_CHECKERROR;
  GFX_eDepthFunc = eFunc;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}


    
// set depth buffer range
static void ogl_DepthRange( FLOAT fMin, FLOAT fMax)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  GLfloat fDepths[2];
  pglGetFloatv( GL_DEPTH_RANGE, fDepths);
  OGL_CHECKERROR;
  ASSERT( fDepths[0]==GFX_fMinDepthRange && fDepths[1]==GFX_fMaxDepthRange);
#endif

  // cached?
  if( GFX_fMinDepthRange==fMin && GFX_fMaxDepthRange==fMax && gap_bOptimizeStateChanges) return;
  GFX_fMinDepthRange = fMin;
  GFX_fMaxDepthRange = fMax;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDepthRange( fMin, fMax); 
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// set face culling
static void ogl_CullFace( GfxFace eFace)
{
  // check consistency and face
  ASSERT( eFace==GFX_FRONT || eFace==GFX_BACK || eFace==GFX_NONE);
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  GLenum gleCull;
  BOOL bRes = pglIsEnabled(GL_CULL_FACE);
  pglGetIntegerv( GL_CULL_FACE_MODE, (GLint*)&gleCull);
  OGL_CHECKERROR;
  ASSERT( (bRes==GL_FALSE && GFX_eCullFace==GFX_NONE)
       || (bRes==GL_TRUE  && gleCull==GL_FRONT && GFX_eCullFace==GFX_FRONT)
       || (bRes==GL_TRUE  && gleCull==GL_BACK  && GFX_eCullFace==GFX_BACK));
#endif
  // cached?
  if( GFX_eCullFace==eFace && gap_bOptimizeStateChanges) return;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  if( eFace==GFX_FRONT) {
    if( GFX_eCullFace==GFX_NONE) pglEnable(GL_CULL_FACE);
    pglCullFace(GL_FRONT);
  } else if( eFace==GFX_BACK) {
    if( GFX_eCullFace==GFX_NONE) pglEnable(GL_CULL_FACE);
    pglCullFace(GL_BACK);
  } else {
    pglDisable(GL_CULL_FACE);
  }
  OGL_CHECKERROR;
  GFX_eCullFace = eFace;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// set front face
static void ogl_FrontFace( GfxFace eFace)
{
  // check consistency and face
  ASSERT( eFace==GFX_CW || eFace==GFX_CCW);
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  GLenum gleFace;
  pglGetIntegerv( GL_FRONT_FACE, (GLint*)&gleFace);
  OGL_CHECKERROR;
  ASSERT( (gleFace==GL_CCW &&  GFX_bFrontFace)
       || (gleFace==GL_CW  && !GFX_bFrontFace));
#endif
  // cached?
  BOOL bFrontFace = (eFace==GFX_CCW);
  if( !bFrontFace==!GFX_bFrontFace && gap_bOptimizeStateChanges) return;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  if( eFace==GFX_CCW) {
    pglFrontFace( GL_CCW);
  } else {
    pglFrontFace( GL_CW);
  }
  OGL_CHECKERROR;
  GFX_bFrontFace = bFrontFace; 

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// set custom clip plane 
static void ogl_ClipPlane( const DOUBLE *pdViewPlane)
{
  // check API and plane
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL && pdViewPlane!=NULL);

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

#ifdef _GLES //karin: glClipPlanef                              const GLfloat cps[] = { pdViewPlane[0], pdViewPlane[1], pdViewPlane[2], pdViewPlane[3] };                                   pglClipPlane( GL_CLIP_PLANE0, cps);                         #else
  pglClipPlane( GL_CLIP_PLANE0, pdViewPlane);
#endif
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// set texture matrix
static void ogl_SetTextureMatrix( const FLOAT *pfMatrix/*=NULL*/)
{
  // check API
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // set matrix
  pglMatrixMode(GL_TEXTURE);
  if( pfMatrix!=NULL) pglLoadMatrixf(pfMatrix);
  else pglLoadIdentity();
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}




// set view matrix 
static void ogl_SetViewMatrix( const FLOAT *pfMatrix/*=NULL*/)
{
  // check API
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);

  // cached? (only identity matrix)
  if( pfMatrix==NULL && GFX_bViewMatrix==NONE && gap_bOptimizeStateChanges) return;
  GFX_bViewMatrix = (pfMatrix!=NULL);

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // set matrix
  pglMatrixMode( GL_MODELVIEW);
  if( pfMatrix!=NULL) pglLoadMatrixf(pfMatrix);
  else pglLoadIdentity();
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// set orthographic matrix
static void ogl_SetOrtho( const FLOAT fLeft,   const FLOAT fRight, const FLOAT fTop,
                          const FLOAT fBottom, const FLOAT fNear,  const FLOAT fFar,
                          const BOOL bSubPixelAdjust/*=FALSE*/)
{
  // check API and matrix type
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);

  // cached?
  if( GFX_fLastL==fLeft  && GFX_fLastT==fTop    && GFX_fLastN==fNear
   && GFX_fLastR==fRight && GFX_fLastB==fBottom && GFX_fLastF==fFar && gap_bOptimizeStateChanges) return;
  GFX_fLastL = fLeft;   GFX_fLastT = fTop;     GFX_fLastN = fNear;
  GFX_fLastR = fRight;  GFX_fLastB = fBottom;  GFX_fLastF = fFar;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // set matrix
  pglMatrixMode( GL_PROJECTION);
  pglLoadIdentity();
  pglOrtho( fLeft, fRight, fBottom, fTop, fNear, fFar);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// set frustrum matrix
static void ogl_SetFrustum( const FLOAT fLeft, const FLOAT fRight,
                            const FLOAT fTop,  const FLOAT fBottom,
                            const FLOAT fNear, const FLOAT fFar)
{
  // check API
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);

  // cached?
  if( GFX_fLastL==-fLeft  && GFX_fLastT==-fTop    && GFX_fLastN==-fNear
   && GFX_fLastR==-fRight && GFX_fLastB==-fBottom && GFX_fLastF==-fFar && gap_bOptimizeStateChanges) return;
  GFX_fLastL = -fLeft;   GFX_fLastT = -fTop;     GFX_fLastN = -fNear;
  GFX_fLastR = -fRight;  GFX_fLastB = -fBottom;  GFX_fLastF = -fFar;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  // set matrix
  pglMatrixMode( GL_PROJECTION);
  pglLoadIdentity();
  pglFrustum( fLeft, fRight, fBottom, fTop, fNear, fFar);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// set polygon mode (point, line or fill)
static void ogl_PolygonMode( GfxPolyMode ePolyMode)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  switch(ePolyMode) {
  case GFX_POINT:  pglPolygonMode( GL_FRONT_AND_BACK, GL_POINT);  break;
  case GFX_LINE:   pglPolygonMode( GL_FRONT_AND_BACK, GL_LINE);   break;
  case GFX_FILL:   pglPolygonMode( GL_FRONT_AND_BACK, GL_FILL);   break;
  default:  ASSERTALWAYS("Wrong polygon mode!");
  } // check
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}




// TEXTURE MANAGEMENT


// set texture wrapping mode
static void ogl_SetTextureWrapping( enum GfxWrap eWrapU, enum GfxWrap eWrapV)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  // check texture unit consistency
  GLint gliRet;
  pglGetIntegerv( GL_ACTIVE_TEXTURE_ARB, &gliRet);
  ASSERT( GFX_iActiveTexUnit==(gliRet-GL_TEXTURE0_ARB));
  pglGetIntegerv( GL_CLIENT_ACTIVE_TEXTURE_ARB, &gliRet);
  ASSERT( GFX_iActiveTexUnit==(gliRet-GL_TEXTURE0_ARB));
#endif

  _tpGlobal[GFX_iActiveTexUnit].tp_eWrapU = eWrapU;
  _tpGlobal[GFX_iActiveTexUnit].tp_eWrapV = eWrapV;
}



// set texture modulation mode
static void ogl_SetTextureModulation( INDEX iScale)
{
  // check consistency
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG                 
  // check current modulation
  GLint iRet;
  pglGetTexEnviv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &iRet);
  ASSERT( (GFX_iTexModulation[GFX_iActiveTexUnit]==1 && iRet==GL_MODULATE)
       || (GFX_iTexModulation[GFX_iActiveTexUnit]==2 && iRet==GL_COMBINE_EXT));
  OGL_CHECKERROR;
#endif
  
  // cached?
  ASSERT( iScale==1 || iScale==2);
  if( GFX_iTexModulation[GFX_iActiveTexUnit]==iScale) return;
  GFX_iTexModulation[GFX_iActiveTexUnit] = iScale;

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  if( iScale==2) {
    pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    pglTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT,  GL_MODULATE);
    pglTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_EXT,    2.0f);
  } else {
    pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  } OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// generate texture for API
static void ogl_GenerateTexture( ULONG &ulTexObject)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  _sfStats.StartTimer(CStatForm::STI_BINDTEXTURE);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglGenTextures( 1, (GLuint*)&ulTexObject);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_BINDTEXTURE);
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}


 
// unbind texture from API
static void ogl_DeleteTexture( ULONG &ulTexObject)
{
  // skip if already unbound
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  if( ulTexObject==NONE) return;

  _sfStats.StartTimer(CStatForm::STI_BINDTEXTURE);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDeleteTextures( 1, (GLuint*)&ulTexObject);
  ulTexObject = NONE;

  _sfStats.StopTimer(CStatForm::STI_BINDTEXTURE);
  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// VERTEX ARRAYS


// prepare vertex array for API
static void ogl_SetVertexArray( GFXVertex4 *pvtx, INDEX ctVtx)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  ASSERT( ctVtx>0 && pvtx!=NULL && GFX_iActiveTexUnit==0);
  GFX_ctVertices = ctVtx;
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglDisableClientState( GL_TEXTURE_COORD_ARRAY);
  pglDisableClientState( GL_COLOR_ARRAY); 
  pglDisableClientState( GL_NORMAL_ARRAY);
  ASSERT( !pglIsEnabled( GL_TEXTURE_COORD_ARRAY));
  ASSERT( !pglIsEnabled( GL_COLOR_ARRAY));
  ASSERT( !pglIsEnabled( GL_NORMAL_ARRAY));
  ASSERT(  pglIsEnabled( GL_VERTEX_ARRAY));
  pglVertexPointer( 3, GL_FLOAT, sizeof (*pvtx), pvtx);
  OGL_CHECKERROR;
  GFX_bColorArray = FALSE; // mark that color array has been disabled (because of potential LockArrays)

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// prepare normal array for API
static void ogl_SetNormalArray( GFXNormal *pnor)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  ASSERT( pnor!=NULL);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglEnableClientState(GL_NORMAL_ARRAY);
  ASSERT( pglIsEnabled(GL_NORMAL_ARRAY));
  pglNormalPointer( GL_FLOAT, sizeof (*pnor), pnor);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// prepare color array for API (and force rendering with color array!)
static void ogl_SetColorArray( GFXColor *pcol)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  ASSERT( pcol!=NULL);
  ogl_EnableColorArray();
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);
  
  pglColorPointer( 4, GL_UNSIGNED_BYTE, sizeof (*pcol), pcol);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// prepare texture coordinates array for API
static void ogl_SetTexCoordArray( GFXTexCoord *ptex, BOOL b4/*=FALSE*/)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  ASSERT( ptex!=NULL);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
  ASSERT( pglIsEnabled(GL_TEXTURE_COORD_ARRAY));
  pglTexCoordPointer( b4?4:2, GL_FLOAT, sizeof (*ptex), ptex);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// set constant color (and force rendering w/o color array!)
static void ogl_SetConstantColor( COLOR col)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  ogl_DisableColorArray();
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  glCOLOR(col);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// draw prepared arrays
static void ogl_DrawElements( INDEX ctElem, INDEX_T *pidx)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
#ifndef NDEBUG
  // check if all indices are inside lock count (or smaller than 65536)
  if( pidx!=NULL) for( INDEX i=0; i<ctElem; i++) ASSERT( pidx[i] < GFX_ctVertices);
#endif

  _sfStats.StartTimer(CStatForm::STI_GFXAPI);
  _pGfx->gl_ctTotalTriangles += ctElem/3;  // for profiling

  // arrays or elements
#ifdef _GLES //karin: GL_TRIANGLE_FAN instead of GL_QUADS
  if( pidx==NULL)
  {
	  for(INDEX i = 0; i < ctElem; i += 4)
		pglDrawArrays( GL_TRIANGLE_FAN, i, 4);
  }
#else
  if( pidx==NULL) pglDrawArrays( GL_QUADS, 0, ctElem);
#endif
  else pglDrawElements( GL_TRIANGLES, ctElem, INDEX_GL, pidx);
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}



// force finish of API rendering queue
static void ogl_Finish(void)
{
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  _sfStats.StartTimer(CStatForm::STI_GFXAPI);

  pglFinish();
  OGL_CHECKERROR;

  _sfStats.StopTimer(CStatForm::STI_GFXAPI);
}




// routines for locking and unlocking compiled vertex arrays
static void ogl_LockArrays(void)
{
  // only if supported
  ASSERT( _pGfx->gl_eCurrentAPI==GAT_OGL);
  ASSERT( GFX_ctVertices>0 && !_bCVAReallyLocked);
  if( !(_pGfx->gl_ulFlags&GLF_EXT_COMPILEDVERTEXARRAY)) return;
  pglLockArraysEXT( 0, GFX_ctVertices);
  OGL_CHECKERROR;
 _bCVAReallyLocked = TRUE;
}
