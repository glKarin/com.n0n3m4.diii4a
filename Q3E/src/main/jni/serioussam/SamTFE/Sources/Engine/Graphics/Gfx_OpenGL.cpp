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

#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Base/Translation.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Memory.h>
#include <Engine/Base/Console.h>

#include <Engine/Graphics/ViewPort.h>
#include <Engine/Graphics/MultiMonitor.h>

#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/Stock_CTextureData.h>

#include <Engine/Base/ListIterator.inl>

#ifdef PLATFORM_WIN32
#include <Engine/Graphics/Gfx_OpenGL.h>
#endif

BOOL _TBCapability = FALSE;

extern INDEX ogl_iTBufferEffect;
extern INDEX ogl_iTBufferSamples;


// fog/haze textures
extern ULONG _fog_ulTexture;
extern ULONG _haze_ulTexture;

// change control
extern INDEX GFX_ctVertices;


// attributes for t-buffer
int aiAttribList[] = {
  WGL_DRAW_TO_WINDOW_EXT, TRUE,
  WGL_SUPPORT_OPENGL_EXT, TRUE,
  WGL_DOUBLE_BUFFER_EXT, TRUE,
	WGL_PIXEL_TYPE_EXT, WGL_TYPE_RGBA_EXT,
  WGL_COLOR_BITS_EXT, 16,
  WGL_DEPTH_BITS_EXT, 16,
	WGL_SAMPLE_BUFFERS_3DFX, 1,
  WGL_SAMPLES_3DFX, 4,
	0, 0
};
int *piAttribList = aiAttribList;


// engine's internal opengl state variables
extern BOOL GFX_bDepthTest;
extern BOOL GFX_bDepthWrite;
extern BOOL GFX_bAlphaTest;
extern BOOL GFX_bBlending;
extern BOOL GFX_bDithering;
extern BOOL GFX_bClipping;
extern BOOL GFX_bClipPlane;
extern BOOL GFX_bColorArray;
extern BOOL GFX_bFrontFace;
extern BOOL GFX_bTruform;
extern INDEX GFX_iActiveTexUnit;
extern FLOAT GFX_fMinDepthRange;
extern FLOAT GFX_fMaxDepthRange;
extern GfxBlend GFX_eBlendSrc;
extern GfxBlend GFX_eBlendDst;
extern GfxComp  GFX_eDepthFunc;
extern GfxFace  GFX_eCullFace;
extern INDEX GFX_iTexModulation[GFX_MAXTEXUNITS];

__extern BOOL  glbUsingVARs = FALSE;   // vertex_array_range

// define gl function pointers
#define DLLFUNCTION(dll, output, name, inputs, params, required) \
  output (__stdcall *p##name) inputs = NULL;
#include "gl_functions.h"
#undef DLLFUNCTION

// extensions
void (__stdcall *pglLockArraysEXT)(GLint first, GLsizei count) = NULL;
void (__stdcall *pglUnlockArraysEXT)(void) = NULL;

#if PLATFORM_WIN32  // SDL handles this elsewhere.
GLboolean (__stdcall *pwglSwapIntervalEXT)(GLint interval) = NULL;
GLint     (__stdcall *pwglGetSwapIntervalEXT)(void) = NULL;
#endif

void (__stdcall *pglActiveTextureARB)(GLenum texunit) = NULL;
void (__stdcall *pglClientActiveTextureARB)(GLenum texunit) = NULL;

#ifdef PLATFORM_WIN32
char *(__stdcall *pwglGetExtensionsStringARB)(HDC hdc);
BOOL  (__stdcall *pwglChoosePixelFormatARB)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
BOOL  (__stdcall *pwglGetPixelFormatAttribivARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *piValues);
#endif

// t-buffer support
void  (__stdcall *pglTBufferMask3DFX)(GLuint mask);

// NV occlusion query
void (__stdcall *pglGenOcclusionQueriesNV)( GLsizei n, GLuint *ids);
void (__stdcall *pglDeleteOcclusionQueriesNV)( GLsizei n, const GLuint *ids);
void (__stdcall *pglBeginOcclusionQueryNV)( GLuint id);
void (__stdcall *pglEndOcclusionQueryNV)(void);
void (__stdcall *pglGetOcclusionQueryivNV)( GLuint id, GLenum pname, GLint *params);
void (__stdcall *pglGetOcclusionQueryuivNV)( GLuint id, GLenum pname, GLuint *params);
GLboolean (__stdcall *pglIsOcclusionQueryNV)( GLuint id);

// ATI GL_ATI_pn_triangles
void (__stdcall *pglPNTrianglesiATI)( GLenum pname, GLint param);
void (__stdcall *pglPNTrianglesfATI)( GLenum pname, GLfloat param);


// test if an extension exists
static BOOL HasExtension( const char *strAllExtensions, const char *strExtension)
{
  // find substring
  const char *strFound = strstr( strAllExtensions, strExtension);
  //  no extension if not found
  if( strFound==NULL) return FALSE;
  INDEX iExtensionLen = strlen(strExtension);
  // if found substring is substring of some other extension
  if( strFound[iExtensionLen]!=' ' && strFound[iExtensionLen]!=0) {
    // continue searching after that char
    return HasExtension( strFound+iExtensionLen, strExtension);
  }
  // extension found
  return TRUE; 
}


// add OpenGL extensions to engine
void CGfxLibrary::AddExtension_OGL( ULONG ulFlag, const char *strName)
{
  gl_ulFlags = (gl_ulFlags & ~ulFlag) | ulFlag;
  go_strSupportedExtensions += strName;
  go_strSupportedExtensions += " ";
}


// determine OpenGL extensions that engine supports
void CGfxLibrary::TestExtension_OGL( ULONG ulFlag, const char *strName)
{
  if( HasExtension( go_strExtensions, strName)) AddExtension_OGL( ulFlag, strName);
}


// prepares OpenGL drawing context
void CGfxLibrary::InitContext_OGL(void)
{
  // must have context
  ASSERT( gl_pvpActive!=NULL);

  // reset engine's internal OpenGL state variables
  extern BOOL GFX_abTexture[GFX_MAXTEXUNITS];
  for( INDEX iUnit=0; iUnit<GFX_MAXTEXUNITS; iUnit++) {
    GFX_abTexture[iUnit] = FALSE;
    GFX_iTexModulation[iUnit] = 1;
  }
  // set default texture unit and modulation mode
  GFX_iActiveTexUnit = 0;
  gl_ctMaxStreams = 16; // GL always has enough "streams" for multi-texturing
  // reset frustum/ortho stuff
  extern BOOL  GFX_bViewMatrix;
  extern FLOAT GFX_fLastL, GFX_fLastR, GFX_fLastT, GFX_fLastB, GFX_fLastN, GFX_fLastF;
  GFX_fLastL = GFX_fLastR = GFX_fLastT = GFX_fLastB = GFX_fLastN = GFX_fLastF = 0;
  GFX_bViewMatrix = TRUE;

  glbUsingVARs  = FALSE;
  GFX_bTruform  = FALSE;
  GFX_bClipping = TRUE;
  pglEnable(  GL_TEXTURE_2D);     GFX_abTexture[0] = TRUE;
  pglEnable(  GL_DITHER);         GFX_bDithering   = TRUE;
  pglDisable( GL_BLEND);          GFX_bBlending    = FALSE;
  pglDisable( GL_DEPTH_TEST);     GFX_bDepthTest   = FALSE;
  pglDisable( GL_ALPHA_TEST);     GFX_bAlphaTest   = FALSE;
  pglDisable( GL_CLIP_PLANE0);    GFX_bClipPlane   = FALSE;
  pglDisable( GL_CULL_FACE);      GFX_eCullFace    = GFX_NONE;
  pglFrontFace( GL_CCW);          GFX_bFrontFace   = TRUE;
  pglDepthMask( GL_FALSE);        GFX_bDepthWrite  = FALSE;
  pglDepthFunc( GL_LEQUAL);       GFX_eDepthFunc   = GFX_LESS_EQUAL;
  pglBlendFunc( GL_ONE, GL_ONE);  GFX_eBlendSrc = GFX_eBlendDst = GFX_ONE;
  pglDepthRange( 0.0f, 1.0f);     GFX_fMinDepthRange = 0.0f;
                                  GFX_fMaxDepthRange = 1.0f;
  // (re)set some OpenGL defaults
  gfxPolygonMode( GFX_FILL);
  pglFrontFace( GL_CCW);
  pglShadeModel( GL_SMOOTH);
  pglEnable( GL_SCISSOR_TEST);
  pglDrawBuffer( GL_BACK);
  pglAlphaFunc( GL_GEQUAL, 0.5f);
  pglColor4f( 1.0f, 1.0f, 1.0f, 1.0f);
  pglMatrixMode( GL_MODELVIEW);
  pglLoadIdentity();
  pglMatrixMode( GL_TEXTURE);
  pglLoadIdentity();
  // enable rendering only thru vertex arrays by default
  pglEnableClientState(  GL_VERTEX_ARRAY);
  pglDisableClientState( GL_NORMAL_ARRAY);
  pglDisableClientState( GL_TEXTURE_COORD_ARRAY);
  pglDisableClientState( GL_COLOR_ARRAY);
  GFX_bColorArray = FALSE;

  // set single byte pixel alignment
  pglPixelStorei( GL_PACK_ALIGNMENT,   1);
  pglPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  // TEST EXTENSIONS
  CDisplayAdapter &da = gl_gaAPI[GAT_OGL].ga_adaAdapter[gl_iCurrentAdapter];
  da.da_strVendor   = (const char*)pglGetString(GL_VENDOR);
  da.da_strRenderer = (const char*)pglGetString(GL_RENDERER);
  da.da_strVersion  = (const char*)pglGetString(GL_VERSION);
  go_strExtensions  = (const char*)pglGetString(GL_EXTENSIONS);

  // report
  CPrintF( TRANS("\n* OpenGL context created: *----------------------------------\n"));
  CPrintF( "  (%s, %s, %s)\n\n", (const char *) da.da_strVendor, (const char *) da.da_strRenderer, (const char *) da.da_strVersion);

  // test for used extensions
  GLint   gliRet;
  GLfloat glfRet;
  go_strSupportedExtensions = "";

  // check for WGL extensions, too
  go_strWinExtensions = "";
#ifdef PLATFORM_WIN32
  pwglGetExtensionsStringARB = (char* (__stdcall*)(HDC))pwglGetProcAddress("wglGetExtensionsStringARB");
  if( pwglGetExtensionsStringARB != NULL) {
    AddExtension_OGL( NONE, "WGL_ARB_extensions_string"); // register
    CTempDC tdc(gl_pvpActive->vp_hWnd);
    go_strWinExtensions = (char*)pwglGetExtensionsStringARB(tdc.hdc);
  }
#endif

  // multitexture is supported only thru GL_EXT_texture_env_combine extension
  gl_ctTextureUnits = 1;
  gl_ctRealTextureUnits = 1;
  pglActiveTextureARB       = NULL;
  pglClientActiveTextureARB = NULL;

#ifdef PLATFORM_WIN32
#define  OGL_GetProcAddress pwglGetProcAddress
#endif 
// This renders badly on the current Intel Macs...my bug, probably.  !!! FIXME
#if PLATFORM_MACOSX
    CPrintF("Forcibly disabled multitexturing for now on Mac OS X.");
#else
  if( HasExtension( go_strExtensions, "GL_ARB_multitexture")) {
    pglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, (int*)&gl_ctRealTextureUnits); // get number of texture units
    if( gl_ctRealTextureUnits>1 && (HasExtension( go_strExtensions, "GL_EXT_texture_env_combine") || HasExtension( go_strExtensions, "GL_ARB_texture_env_combine")) ) {
      AddExtension_OGL( NONE, "GL_ARB_multitexture");
      AddExtension_OGL( NONE, "GL_EXT_texture_env_combine");
      pglActiveTextureARB       = (void (__stdcall*)(GLenum))OGL_GetProcAddress( "glActiveTextureARB");
      pglClientActiveTextureARB = (void (__stdcall*)(GLenum))OGL_GetProcAddress( "glClientActiveTextureARB");
      ASSERT( pglActiveTextureARB!=NULL && pglClientActiveTextureARB!=NULL);
      gl_ctTextureUnits = Min((INDEX)GFX_MAXTEXUNITS, gl_ctRealTextureUnits);
    } else {
      CPrintF( TRANS("  GL_TEXTURE_ENV_COMBINE extension missing - multi-texturing cannot be used.\n"));
    }
  }
#endif

  // find all supported texture compression extensions
  TestExtension_OGL( GLF_EXTC_ARB,    "GL_ARB_texture_compression");
  TestExtension_OGL( GLF_EXTC_S3TC,   "GL_EXT_texture_compression_s3tc");
  TestExtension_OGL( GLF_EXTC_FXT1,   "GL_3DFX_texture_compression_FXT1");
  TestExtension_OGL( GLF_EXTC_LEGACY, "GL_S3_s3tc");
  // mark if there is at least one extension present
  gl_ulFlags &= ~GLF_TEXTURECOMPRESSION;
  if( (gl_ulFlags&GLF_EXTC_ARB)  || (gl_ulFlags&GLF_EXTC_FXT1)
   || (gl_ulFlags&GLF_EXTC_S3TC) || (gl_ulFlags&GLF_EXTC_LEGACY)) {
    gl_ulFlags |= GLF_TEXTURECOMPRESSION;
  }

  // determine max supported dimension of texture
  pglGetIntegerv( GL_MAX_TEXTURE_SIZE, (GLint*)&gl_pixMaxTextureDimension);
  OGL_CHECKERROR;

  // determine support for texture LOD biasing
  gl_fMaxTextureLODBias = 0.0f;
  if( HasExtension( go_strExtensions, "GL_EXT_texture_lod_bias")) {
    AddExtension_OGL( NONE, "GL_EXT_texture_lod_bias"); // register
    // check max possible lod bias (absolute)
    pglGetFloatv( GL_MAX_TEXTURE_LOD_BIAS_EXT, &glfRet);
    GLenum gleError = pglGetError();  // just because of invalid extension implementations (S3)
    if( gleError || glfRet<0.1f || glfRet>4.0f) glfRet = 4.0f;
    gl_fMaxTextureLODBias = glfRet;
    OGL_CHECKERROR;
  }

  // determine support for anisotropic filtering
  gl_iMaxTextureAnisotropy = 1;
  if( HasExtension( go_strExtensions, "GL_EXT_texture_filter_anisotropic")) {
    AddExtension_OGL( NONE, "GL_EXT_texture_filter_anisotropic"); // register
    // keep max allowed anisotropy degree
    pglGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gliRet);
    gl_iMaxTextureAnisotropy = gliRet;
    OGL_CHECKERROR;
  }

  // check support for compiled vertex arrays
  pglLockArraysEXT   = NULL;
  pglUnlockArraysEXT = NULL;
  if( HasExtension( go_strExtensions, "GL_EXT_compiled_vertex_array")) {
    AddExtension_OGL( GLF_EXT_COMPILEDVERTEXARRAY, "GL_EXT_compiled_vertex_array");
    pglLockArraysEXT   = (void (__stdcall*)(GLint,GLsizei))OGL_GetProcAddress( "glLockArraysEXT");
    pglUnlockArraysEXT = (void (__stdcall*)(void)         )OGL_GetProcAddress( "glUnlockArraysEXT");
    ASSERT( pglLockArraysEXT!=NULL && pglUnlockArraysEXT!=NULL);
  }

  // check support for swap interval
  #ifdef PLATFORM_WIN32  // SDL handles this elsewhere.
  pwglSwapIntervalEXT    = NULL;
  pwglGetSwapIntervalEXT = NULL;
  if( HasExtension( go_strExtensions, "WGL_EXT_swap_control")) {
    AddExtension_OGL( GLF_VSYNC, "WGL_EXT_swap_control");
    pwglSwapIntervalEXT    = (GLboolean (__stdcall*)(GLint))OGL_GetProcAddress( "wglSwapIntervalEXT");
    pwglGetSwapIntervalEXT = (GLint     (__stdcall*)(void) )OGL_GetProcAddress( "wglGetSwapIntervalEXT");
    ASSERT( pwglSwapIntervalEXT!=NULL && pwglGetSwapIntervalEXT!=NULL);
  }
  #endif

  // determine support for ATI Truform technology
  extern INDEX truform_iLevel;
  extern BOOL  truform_bLinear;
  truform_iLevel  = -1;
  truform_bLinear = FALSE;
  pglPNTrianglesiATI = NULL;
  pglPNTrianglesfATI = NULL;
  gl_iTessellationLevel    = 0;
  gl_iMaxTessellationLevel = 0;
  if( HasExtension( go_strExtensions, "GL_ATI_pn_triangles")) {
    AddExtension_OGL( NONE, "GL_ATI_pn_triangles");
    pglPNTrianglesiATI = (void (__stdcall*)(GLenum,GLint  ))OGL_GetProcAddress( "glPNTrianglesiATI");
    pglPNTrianglesfATI = (void (__stdcall*)(GLenum,GLfloat))OGL_GetProcAddress( "glPNTrianglesfATI");
    ASSERT( pglPNTrianglesiATI!=NULL && pglPNTrianglesfATI!=NULL);
    // check max possible tessellation
    pglGetIntegerv( GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI, &gliRet);
    gl_iMaxTessellationLevel = gliRet;
    OGL_CHECKERROR;
  } 

  // if T-buffer is supported
  if( _TBCapability) {
    // add extension and disable t-buffer usage by default
    AddExtension_OGL( GLF_EXT_TBUFFER, "GL_3DFX_multisample");
    pglDisable( GL_MULTISAMPLE_3DFX);
    OGL_CHECKERROR;
  }

  // test for clamp to edge
#ifdef _GLES //karin: GLES standard
  AddExtension_OGL( GLF_EXT_EDGECLAMP, "GL_EXT_texture_edge_clamp");
#else
  TestExtension_OGL( GLF_EXT_EDGECLAMP, "GL_EXT_texture_edge_clamp");
#endif

  // test for clip volume hint
  TestExtension_OGL( GLF_EXT_CLIPHINT, "GL_EXT_clip_volume_hint");
  /*
  // test for occlusion culling
  TestExtension_OGL( GLF_EXT_OCCLUSIONTEST, "GL_HP_occlusion_test");

  pglGenOcclusionQueriesNV = NULL;  pglDeleteOcclusionQueriesNV = NULL;
  pglBeginOcclusionQueryNV = NULL;  pglEndOcclusionQueryNV      = NULL;
  pglGetOcclusionQueryivNV = NULL;  pglGetOcclusionQueryuivNV   = NULL;
  pglIsOcclusionQueryNV    = NULL;

  if( HasExtension( go_strExtensions, "GL_NV_occlusion_query"))
  { // prepare extension's functions
    AddExtension_OGL( GLF_EXT_OCCLUSIONQUERY, "GL_NV_occlusion_query");
    pglGenOcclusionQueriesNV    = (void (__stdcall*)(GLsizei, GLuint*))pwglGetProcAddress( "glGenOcclusionQueriesNV");
    pglDeleteOcclusionQueriesNV = (void (__stdcall*)(GLsizei, const GLuint*))pwglGetProcAddress( "glDeleteOcclusionQueriesNV");
    pglBeginOcclusionQueryNV    = (void (__stdcall*)(GLuint))pwglGetProcAddress( "glBeginOcclusionQueryNV");
    pglEndOcclusionQueryNV      = (void (__stdcall*)(void))pwglGetProcAddress( "glEndOcclusionQueryNV");
    pglGetOcclusionQueryivNV    = (void (__stdcall*)(GLuint, GLenum, GLint*))pwglGetProcAddress( "glGetOcclusionQueryivNV");
    pglGetOcclusionQueryuivNV   = (void (__stdcall*)(GLuint, GLenum, GLuint*))pwglGetProcAddress( "glGetOcclusionQueryuivNV");
    pglIsOcclusionQueryNV  = (GLboolean (__stdcall*)(GLuint))pwglGetProcAddress( "glIsOcclusionQueryNV");
    ASSERT( pglGenOcclusionQueriesNV!=NULL && pglDeleteOcclusionQueriesNV!=NULL
         && pglBeginOcclusionQueryNV!=NULL && pglEndOcclusionQueryNV!=NULL
         && pglGetOcclusionQueryivNV!=NULL && pglGetOcclusionQueryuivNV!=NULL
         && pglIsOcclusionQueryNV!=NULL);
  }
  */
  // done with seeking for supported extensions
  if( go_strSupportedExtensions=="") go_strSupportedExtensions = "none";

  // allocate vertex buffers
  // eglAdjustVertexBuffers(ogl_iVertexBufferSize*1024);
  // OGL_CHECKERROR;

  // check if 32-bit textures are supported
  GLuint uiTmpTex;
  const ULONG ulTmpTex = 0xFFFFFFFF;
  pglGenTextures( 1, &uiTmpTex);
  pglBindTexture( GL_TEXTURE_2D, uiTmpTex);
#ifdef _GLES //karin: unsized GL_RGBA
  pglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1,1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &ulTmpTex);
#else
  pglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 1,1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &ulTmpTex);
#endif
  OGL_CHECKERROR;
  gl_ulFlags &= ~GLF_32BITTEXTURES;
  pglGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_SIZE, &gliRet);
  if( gliRet==8) gl_ulFlags |= GLF_32BITTEXTURES;
#ifdef _GLES //karin: 8bits color component
  gl_ulFlags |= GLF_32BITTEXTURES;
#endif
  pglDeleteTextures( 1, &uiTmpTex);
  OGL_CHECKERROR;
  
  // setup fog and haze textures
  extern PIX _fog_pixSizeH;
  extern PIX _fog_pixSizeL;
  extern PIX _haze_pixSize;
  pglGenTextures( 1, (GLuint*)&_fog_ulTexture);
  pglGenTextures( 1, (GLuint*)&_haze_ulTexture);
  _fog_pixSizeH = 0;
  _fog_pixSizeL = 0;
  _haze_pixSize = 0;
  OGL_CHECKERROR;

  // prepare pattern texture
  extern CTexParams _tpPattern;
  extern ULONG _ulPatternTexture;
  extern ULONG _ulLastUploadedPattern;
  pglGenTextures( 1, (GLuint*)&_ulPatternTexture);
  _ulLastUploadedPattern = 0;
  _tpPattern.Clear();

  // reset texture filtering and array locking
  _tpGlobal[0].Clear();
  _tpGlobal[1].Clear();
  _tpGlobal[2].Clear();
  _tpGlobal[3].Clear();
  GFX_ctVertices = 0;
  gl_dwVertexShader = NONE;

  // set default texture filtering/biasing
  extern INDEX gap_iTextureFiltering;
  extern INDEX gap_iTextureAnisotropy;
  extern FLOAT gap_fTextureLODBias;
  gfxSetTextureFiltering( gap_iTextureFiltering, gap_iTextureAnisotropy);
  gfxSetTextureBiasing( gap_fTextureLODBias);

  // mark pretouching and probing
  extern BOOL _bNeedPretouch;
  _bNeedPretouch = TRUE;
  gl_bAllowProbing = FALSE;

  // update console system vars
  extern void UpdateGfxSysCVars(void);
  UpdateGfxSysCVars();

  // reload all loaded textures and eventually shadows
  extern INDEX shd_bCacheAll;
  extern void ReloadTextures(void);
  extern void CacheShadows(void);
  ReloadTextures();
  if( shd_bCacheAll) CacheShadows();
}

static void ClearFunctionPointers(void)
{
  // clear gl function pointers
  #define DLLFUNCTION(dll, output, name, inputs, params, required) p##name = NULL;
  #include "gl_functions.h"
  #undef DLLFUNCTION
}


// shutdown OpenGL driver
void CGfxLibrary::EndDriver_OGL(void)
{
  // unbind all textures
  if( _pTextureStock!=NULL) {
    {FOREACHINDYNAMICCONTAINER( _pTextureStock->st_ctObjects, CTextureData, ittd) {
      CTextureData &td = *ittd;
      td.td_tpLocal.Clear();
      td.Unbind();
    }}
  } // unbind fog/haze
  gfxDeleteTexture( _fog_ulTexture); 
  gfxDeleteTexture( _haze_ulTexture);

  ASSERT( _ptdFlat!=NULL);
  _ptdFlat->td_tpLocal.Clear();
  _ptdFlat->Unbind();

  PlatformEndDriver_OGL();
  ClearFunctionPointers();
}



/*
 * 3dfx t-buffer control
 */
extern void SetTBufferEffect( BOOL bEnable)
{
  // adjust console vars
  ogl_iTBufferEffect  = Clamp( ogl_iTBufferEffect, (INDEX)0, (INDEX)2);
  ogl_iTBufferSamples = (1L) << FastLog2(ogl_iTBufferSamples);
  if( ogl_iTBufferSamples<2) ogl_iTBufferSamples = 4;
  // if supported
  if( _pGfx->gl_ulFlags&GLF_EXT_TBUFFER)
  { // disable multisampling if not required
    ASSERT( pglTBufferMask3DFX!=NULL);
    if( ogl_iTBufferEffect==0 || _pGfx->go_ctSampleBuffers<2 || !bEnable) pglDisable( GL_MULTISAMPLE_3DFX);
    else {
      pglEnable( GL_MULTISAMPLE_3DFX);
      //UINT uiMask = 0xFFFFFFFF;
      // set one buffer in case of motion-blur
      //if( ogl_iTBufferEffect==2) uiMask = (1UL) << _pGfx->go_iCurrentWriteBuffer;
      //pglTBufferMask3DFX(uiMask);
    }
  }
}
