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

#include <Engine/Base/Translation.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Memory.h>
#include <Engine/Base/Console.h>

#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/ViewPort.h>

#include <Engine/Templates/StaticStackArray.cpp>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/Stock_CTextureData.h>

#ifdef SE1_D3D


// asm shortcuts
#define O offset
#define Q qword ptr
#define D dword ptr
#define W  word ptr
#define B  byte ptr

#define ASMOPT 1


extern INDEX gap_iTruformLevel;

extern ULONG _fog_ulTexture;
extern ULONG _haze_ulTexture;


// state variables
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

extern INDEX GFX_ctVertices;
extern BOOL  D3D_bUseColorArray = FALSE;

// internal vars
static INDEX _iVtxOffset = 0;
static INDEX _iIdxOffset = 0;
static INDEX _iVtxPos = 0;
static INDEX _iTexPass = 0;
static INDEX _iColPass = 0;
static DWORD _dwCurrentVS = NONE;
static ULONG _ulStreamsMask = NONE;
static ULONG _ulLastStreamsMask = NONE;
static BOOL  _bProjectiveMapping = FALSE;
static BOOL  _bLastProjectiveMapping = FALSE;
static DWORD _dwVtxLockFlags;                 // for vertex and normal
static DWORD _dwColLockFlags[GFX_MAXLAYERS];  // for colors
static DWORD _dwTexLockFlags[GFX_MAXLAYERS];  // for texture coords

// shaders created so far
struct VertexShader {
  ULONG vs_ulMask;
  DWORD vs_dwHandle;
};
static CStaticStackArray<VertexShader> _avsShaders;


// device type (HAL is default, REF is only for debuging)
extern const D3DDEVTYPE d3dDevType = D3DDEVTYPE_HAL;

// identity matrix
extern const D3DMATRIX GFX_d3dIdentityMatrix = {
  1, 0, 0, 0,
  0, 1, 0, 0,
  0, 0, 1, 0,
  0, 0, 0, 1 };


// sizes of vertex components
#define POSSIZE (4*sizeof(FLOAT))
#define NORSIZE (4*sizeof(FLOAT))
#define TEXSIZE (2*sizeof(FLOAT))
#define TX4SIZE (4*sizeof(FLOAT))
#define COLSIZE (4*sizeof(UBYTE))
#define IDXSIZE (1*sizeof(UWORD))

#define POSIDX (0)
#define COLIDX (1)
#define TEXIDX (2)
#define NORIDX (6)


// SHADER SETUP PARAMS

#define DECLTEXOFS (2*TEXIDX)
#define MAXSTREAMS (7L)

// template shader
DWORD _adwDeclTemplate[] = {
  D3DVSD_STREAM(0),
  D3DVSD_REG( D3DVSDE_POSITION,  D3DVSDT_FLOAT3),
  D3DVSD_STREAM(1),
  D3DVSD_REG( D3DVSDE_DIFFUSE,   D3DVSDT_D3DCOLOR),
  D3DVSD_STREAM(2),
  D3DVSD_REG( D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2),
  D3DVSD_STREAM(3),
  D3DVSD_REG( D3DVSDE_TEXCOORD1, D3DVSDT_FLOAT2),
  D3DVSD_STREAM(4),
  D3DVSD_REG( D3DVSDE_TEXCOORD2, D3DVSDT_FLOAT2),
  D3DVSD_STREAM(5),
  D3DVSD_REG( D3DVSDE_TEXCOORD3, D3DVSDT_FLOAT2),
  D3DVSD_STREAM(6),
  D3DVSD_REG( D3DVSDE_NORMAL,    D3DVSDT_FLOAT3),
  D3DVSD_END()
};

// current shader
DWORD _adwCurrentDecl[2*MAXSTREAMS+1];



// check whether texture format is supported in D3D
static BOOL HasTextureFormat_D3D( D3DFORMAT d3dTextureFormat)
{
  // quickie?
  const D3DFORMAT d3dScreenFormat = _pGfx->gl_d3dColorFormat;
  if( d3dTextureFormat==D3DFMT_UNKNOWN || d3dScreenFormat==NONE) return TRUE;
  // check
  extern const D3DDEVTYPE d3dDevType;
  HRESULT hr = _pGfx->gl_pD3D->CheckDeviceFormat( _pGfx->gl_iCurrentAdapter, d3dDevType, d3dScreenFormat,
                                                  0, D3DRTYPE_TEXTURE, d3dTextureFormat);
  return( hr==D3D_OK);
}


#define VTXSIZE (POSSIZE + (_pGfx->gl_iMaxTessellationLevel>0 ? NORSIZE : 0) + TX4SIZE \
               + COLSIZE *  _pGfx->gl_ctColBuffers \
               + TEXSIZE * (_pGfx->gl_ctTexBuffers-1))


// returns number of vertices based on vertex size and required size in memory (KB)
extern INDEX VerticesFromSize_D3D( SLONG &slSize)
{
  slSize = Clamp( slSize, 64L, 4096L);
  return (slSize*1024 / VTXSIZE);
}

// returns size in memory based on number of vertices
extern SLONG SizeFromVertices_D3D( INDEX ctVertices)
{
  return (ctVertices * VTXSIZE);
}
   

// construct vertex shader out of streams' bit-mask
extern DWORD SetupShader_D3D( ULONG ulStreamsMask)
{
  HRESULT hr;
  const LPDIRECT3DDEVICE8 pd3dDev = _pGfx->gl_pd3dDevice;
  const INDEX ctShaders = _avsShaders.Count();
  INDEX iVS;

  // delete all shaders?
  if( ulStreamsMask==NONE) {
    for( iVS=0; iVS<ctShaders; iVS++) {
      hr = pd3dDev->DeleteVertexShader(_avsShaders[iVS].vs_dwHandle);
      D3D_CHECKERROR(hr);
    } // free array
    _avsShaders.PopAll();
    _dwCurrentVS = NONE;
    return NONE;
  }

  // see if required shader has already been created
  for( iVS=0; iVS<ctShaders; iVS++) {
    if( _avsShaders[iVS].vs_ulMask==ulStreamsMask) return _avsShaders[iVS].vs_dwHandle;
  }

  // darn, need to create shader :(
  VertexShader &vs = _avsShaders.Push();
  vs.vs_ulMask = ulStreamsMask;

  // pre-adjustment for eventual projective mapping
  _adwDeclTemplate[DECLTEXOFS+1] = D3DVSD_REG( D3DVSDE_TEXCOORD0, (ulStreamsMask&0x1000) ? D3DVSDT_FLOAT4 : D3DVSDT_FLOAT2);
  ulStreamsMask &= ~0x1000;

  // process mask, bit by bit
  INDEX iSrcDecl=0, iDstDecl=0;
  while( iSrcDecl<MAXSTREAMS)
  { // add declarator if used
    if( ulStreamsMask&1) {
      _adwCurrentDecl[iDstDecl*2+0] = _adwDeclTemplate[iSrcDecl*2+0];
      _adwCurrentDecl[iDstDecl*2+1] = _adwDeclTemplate[iSrcDecl*2+1];
      iDstDecl++;
    } // move to next declarator
    iSrcDecl++;
    ulStreamsMask >>= 1;
  }
  // mark end
  _adwCurrentDecl[iDstDecl*2] = D3DVSD_END();
  ASSERT( iDstDecl < MAXSTREAMS);
  ASSERT( _iTexPass>=0 && _iColPass>=0);
  ASSERT( _iVtxPos >=0 && _iVtxPos<65536);

  // create new vertex shader
  const DWORD dwFlags = (_pGfx->gl_ulFlags&GLF_D3D_USINGHWTNL) ? NONE : D3DUSAGE_SOFTWAREPROCESSING;
  hr = pd3dDev->CreateVertexShader( &_adwCurrentDecl[0], NULL, &vs.vs_dwHandle, dwFlags);
  D3D_CHECKERROR(hr);

  // store and reset shader
  _pGfx->gl_dwVertexShader = 0;
  return vs.vs_dwHandle;
}



// prepare vertex arrays for drawing
extern void SetupVertexArrays_D3D( INDEX ctVertices)
{
  INDEX i;
  HRESULT hr;
  ASSERT( ctVertices>=0);

  // do nothing if buffer is sufficient
  ctVertices = ClampUp( ctVertices, 65535L); // need to clamp max vertices first
  if( ctVertices!=0 && ctVertices<=_pGfx->gl_ctVertices) return;

  // determine SW or HW VP and NPatches usage (Truform) 
  DWORD dwFlags = D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY;
  const BOOL bNPatches = (_pGfx->gl_iMaxTessellationLevel>0 && gap_iTruformLevel>0);
  if( !(_pGfx->gl_ulFlags&GLF_D3D_USINGHWTNL)) dwFlags |= D3DUSAGE_SOFTWAREPROCESSING;
  if( bNPatches) dwFlags |= D3DUSAGE_NPATCHES;
  const LPDIRECT3DDEVICE8 pd3dDev = _pGfx->gl_pd3dDevice;

  // deallocate if needed
  if( _pGfx->gl_pd3dVtx!=NULL)
  {
    // vertex and eventual normal array
    D3DRELEASE( _pGfx->gl_pd3dVtx, TRUE);
    if( _pGfx->gl_pd3dNor!=NULL) D3DRELEASE( _pGfx->gl_pd3dNor, TRUE);
    // color arrays
    for( i=0; i<_pGfx->gl_ctColBuffers; i++) {
      ASSERT( _pGfx->gl_pd3dCol[i]!=NULL);
      D3DRELEASE( _pGfx->gl_pd3dCol[i], TRUE);
    }
    // texcoord arrays
    for( i=0; i<_pGfx->gl_ctTexBuffers; i++) {
      ASSERT( _pGfx->gl_pd3dTex[i]!=NULL);
      D3DRELEASE( _pGfx->gl_pd3dTex[i], TRUE);
    }
    // reset all streams, too
    for( i=0; i<_pGfx->gl_ctMaxStreams; i++) {
      hr = pd3dDev->SetStreamSource( i, NULL,0);
      D3D_CHECKERROR(hr);
    }
  }

  // allocate if needed
  if( ctVertices>0)
  {
    // update max vertex count
    if( _pGfx->gl_ctVertices < ctVertices) _pGfx->gl_ctVertices = ctVertices;
    else ctVertices = _pGfx->gl_ctVertices;
    // create buffers
    hr = pd3dDev->CreateVertexBuffer( ctVertices*POSSIZE, dwFlags, 0, D3DPOOL_DEFAULT, &_pGfx->gl_pd3dVtx);
    D3D_CHECKERROR(hr);
    // normals buffer only if required
    if( bNPatches) { 
      hr = pd3dDev->CreateVertexBuffer( ctVertices*NORSIZE, dwFlags, 0, D3DPOOL_DEFAULT, &_pGfx->gl_pd3dNor);
      D3D_CHECKERROR(hr);
    }
    // all color buffers
    for( i=0; i<_pGfx->gl_ctColBuffers; i++) {
      hr = pd3dDev->CreateVertexBuffer( ctVertices*COLSIZE, dwFlags, 0, D3DPOOL_DEFAULT, &_pGfx->gl_pd3dCol[i]);
      D3D_CHECKERROR(hr);
    }
    // 1st texture buffer might have projective mapping
    hr = pd3dDev->CreateVertexBuffer( ctVertices*TX4SIZE, dwFlags, 0, D3DPOOL_DEFAULT, &_pGfx->gl_pd3dTex[0]);
    D3D_CHECKERROR(hr);
    for( i=1; i<_pGfx->gl_ctTexBuffers; i++) {
      hr = pd3dDev->CreateVertexBuffer( ctVertices*TEXSIZE, dwFlags, 0, D3DPOOL_DEFAULT, &_pGfx->gl_pd3dTex[i]);
      D3D_CHECKERROR(hr);
    }
  }
  // just switch it off if not needed any more (i.e. D3D is shutting down)
  else _pGfx->gl_ctVertices = 0;

  // reset and check
  _iVtxOffset = 0;
  _pGfx->gl_dwVertexShader = NONE;
  _ulStreamsMask = NONE;
  _ulLastStreamsMask = NONE;
  _bProjectiveMapping = FALSE;
  _bLastProjectiveMapping = FALSE;
  // reset locking flags
  _dwVtxLockFlags = D3DLOCK_DISCARD;
  for( i=0; i<GFX_MAXLAYERS; i++) _dwColLockFlags[i] = _dwTexLockFlags[i] = D3DLOCK_DISCARD;
  ASSERT(_pGfx->gl_ctVertices<65536);
}



// prepare index arrays for drawing
extern void SetupIndexArray_D3D( INDEX ctIndices)
{
  HRESULT hr;
  ASSERT( ctIndices>=0);
  // clamp max indices
  ctIndices = ClampUp( ctIndices, 65535L);

  // do nothing if buffer is sufficient
  if( ctIndices!=0 && ctIndices<=_pGfx->gl_ctIndices) return;

  // determine SW or HW VP
  DWORD dwFlags = D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY;
  if( !(_pGfx->gl_ulFlags&GLF_D3D_USINGHWTNL)) dwFlags |= D3DUSAGE_SOFTWAREPROCESSING;
  if( _pGfx->gl_iMaxTessellationLevel>0 && gap_iTruformLevel>0) dwFlags |= D3DUSAGE_NPATCHES;
  const LPDIRECT3DDEVICE8 pd3dDev = _pGfx->gl_pd3dDevice;
  
  // dealocate if needed
  if( _pGfx->gl_ctIndices>0) D3DRELEASE( _pGfx->gl_pd3dIdx, TRUE);

  // allocate if needed
  if( ctIndices>0)
  {
    // eventually update max index count
    if( _pGfx->gl_ctIndices < ctIndices) _pGfx->gl_ctIndices = ctIndices;
    else ctIndices = _pGfx->gl_ctIndices;
    // create buffer
    hr = pd3dDev->CreateIndexBuffer( ctIndices*IDXSIZE, dwFlags, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &_pGfx->gl_pd3dIdx);
    D3D_CHECKERROR(hr);
    // set it
    hr = _pGfx->gl_pd3dDevice->SetIndices( _pGfx->gl_pd3dIdx, 0);
    D3D_CHECKERROR(hr);
  }
  // just switch it off if not needed any more (i.e. D3D is shutting down)
  else _pGfx->gl_ctIndices = 0;
  
  // reset and check
  _iIdxOffset = 0;
  ASSERT(_pGfx->gl_ctIndices<65536);
}



// initialize Direct3D driver
BOOL CGfxLibrary::InitDriver_D3D(void)
{
  // check for presence of DirectX 8
  gl_hiDriver = (CDynamicLoader *)LoadLibraryA( "D3D8.DLL");
  if( gl_hiDriver==NONE) {
    // not present - BUAHHAHAHAHAR :)
    CPrintF( "DX8 error: API not installed.\n");
    gl_gaAPI[GAT_D3D].ga_ctAdapters = 0;
    return FALSE; 
  }

  // query DX8 interface
  IDirect3D8* (WINAPI *pDirect3DCreate8)(UINT SDKVersion);
  pDirect3DCreate8 = (IDirect3D8* (WINAPI *)(UINT SDKVersion))GetProcAddress((HMODULE)gl_hiDriver, "Direct3DCreate8");
  if( pDirect3DCreate8==NULL) {
    // cannot init
    CPrintF( "DX8 error: Cannot get entry procedure address.\n");
    FreeLibrary((HMODULE)gl_hiDriver);
    gl_hiDriver = NONE;
    return FALSE; 
  }

  // init DX8
  gl_pD3D = pDirect3DCreate8(D3D_SDK_VERSION);
  if( gl_pD3D==NULL) {
    // cannot start
    CPrintF( "DX8 error: Cannot be initialized.\n");
    FreeLibrary((HMODULE)gl_hiDriver);
    gl_hiDriver = NONE;
    return FALSE;
  }
  // made it!
  return TRUE;
}


// initialize Direct3D driver
void CGfxLibrary::EndDriver_D3D(void)
{
  // unbind textures
  if( _pTextureStock!=NULL) {
    {FOREACHINDYNAMICCONTAINER( _pTextureStock->st_ctObjects, CTextureData, ittd) {
      CTextureData &td = *ittd;
      td.td_tpLocal.Clear();
      td.Unbind();
    }}
  }
  // unbind fog, haze and flat texture
  gfxDeleteTexture( _fog_ulTexture); 
  gfxDeleteTexture( _haze_ulTexture);
  ASSERT( _ptdFlat!=NULL);
  _ptdFlat->td_tpLocal.Clear();
  _ptdFlat->Unbind();

  // reset shader and vertices
  SetupShader_D3D(NONE); 
  SetupVertexArrays_D3D(0); 
  SetupIndexArray_D3D(0); 
  gl_d3dColorFormat = (D3DFORMAT)NONE;
  gl_d3dDepthFormat = (D3DFORMAT)NONE;

  // shutdown device and d3d
  INDEX iRef;
  iRef = gl_pd3dDevice->Release();
  iRef = gl_pD3D->Release();
}


// prepare current viewport for rendering thru Direct3D
BOOL CGfxLibrary::SetCurrentViewport_D3D(CViewPort *pvp)
{
  // determine full screen mode
  CDisplayMode dm;
  RECT rectWindow;
  _pGfx->GetCurrentDisplayMode(dm);
  ASSERT( (dm.dm_pixSizeI==0 && dm.dm_pixSizeJ==0) || (dm.dm_pixSizeI!=0 && dm.dm_pixSizeJ!=0));
	GetClientRect( pvp->vp_hWnd, &rectWindow);
	const PIX pixWinSizeI = rectWindow.right  - rectWindow.left;
	const PIX pixWinSizeJ = rectWindow.bottom - rectWindow.top;

  // full screen allows only one window (main one, which has already been initialized)
  if( dm.dm_pixSizeI==pixWinSizeI && dm.dm_pixSizeJ==pixWinSizeJ) {
    gl_pvpActive = pvp;  // remember as current viewport (must do that BEFORE InitContext)
    if( gl_ulFlags & GLF_INITONNEXTWINDOW) InitContext_D3D();
    gl_ulFlags &= ~GLF_INITONNEXTWINDOW;
    return TRUE; 
  }

  // if must init entire D3D
  if( gl_ulFlags & GLF_INITONNEXTWINDOW) {
    gl_ulFlags &= ~GLF_INITONNEXTWINDOW;
    // reopen window
    pvp->CloseCanvas();
    pvp->OpenCanvas();
    gl_pvpActive = pvp;
    InitContext_D3D();
    pvp->vp_ctDisplayChanges = gl_ctDriverChanges;
    return TRUE;
  }

  // if window was not set for this driver
  if( pvp->vp_ctDisplayChanges<gl_ctDriverChanges) {
    // reopen window
    pvp->CloseCanvas();
    pvp->OpenCanvas();
    pvp->vp_ctDisplayChanges = gl_ctDriverChanges;
    gl_pvpActive = pvp;
    return TRUE;
  }

  // no need to set context if it is the same window as last time
  if( gl_pvpActive!=NULL && gl_pvpActive->vp_hWnd==pvp->vp_hWnd) return TRUE;

  // set rendering target
  HRESULT hr;
  LPDIRECT3DSURFACE8 pColorSurface;
  hr = pvp->vp_pSwapChain->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pColorSurface);
  if( hr!=D3D_OK) return FALSE;
  hr = gl_pd3dDevice->SetRenderTarget( pColorSurface, pvp->vp_pSurfDepth);
  D3DRELEASE( pColorSurface, TRUE);
  if( hr!=D3D_OK) return FALSE;

  // remember as current window
  gl_pvpActive = pvp;
  return TRUE;
}



// prepares Direct3D drawing context
void CGfxLibrary::InitContext_D3D()
{
  // must have context
  ASSERT( gl_pvpActive!=NULL);

  // report header
  CPrintF( TRANS("\n* Direct3D context created: *----------------------------------\n"));
  CDisplayAdapter &da = gl_gaAPI[GAT_D3D].ga_adaAdapter[gl_iCurrentAdapter];
  CPrintF( "  (%s, %s, %s)\n\n", da.da_strVendor, da.da_strRenderer, da.da_strVersion);
  HRESULT hr;

  // reset engine's internal Direct3D state variables
  GFX_bTruform   = FALSE;
  GFX_bClipping  = TRUE;
  GFX_bFrontFace = TRUE;
  hr = gl_pd3dDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE);     D3D_CHECKERROR(hr);  GFX_bDepthTest  = FALSE;
  hr = gl_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE);      D3D_CHECKERROR(hr);  GFX_bDepthWrite = FALSE;
  hr = gl_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE);   D3D_CHECKERROR(hr);  GFX_bAlphaTest  = FALSE;
  hr = gl_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE);  D3D_CHECKERROR(hr);  GFX_bBlending   = FALSE;
  hr = gl_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE, TRUE);       D3D_CHECKERROR(hr);  GFX_bDithering  = TRUE;
  hr = gl_pd3dDevice->SetRenderState( D3DRS_CLIPPLANEENABLE, FALSE);   D3D_CHECKERROR(hr);  GFX_bClipPlane  = FALSE;
  hr = gl_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE);   D3D_CHECKERROR(hr);  GFX_eCullFace   = GFX_NONE;
  hr = gl_pd3dDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL);  D3D_CHECKERROR(hr);  GFX_eDepthFunc  = GFX_LESS_EQUAL;
  hr = gl_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE);  D3D_CHECKERROR(hr);  GFX_eBlendSrc   = GFX_ONE; 
  hr = gl_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE);  D3D_CHECKERROR(hr);  GFX_eBlendDst   = GFX_ONE;
    
  // (re)set some D3D defaults
  hr = gl_pd3dDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL); D3D_CHECKERROR(hr); 
  hr = gl_pd3dDevice->SetRenderState( D3DRS_ALPHAREF,  128);                 D3D_CHECKERROR(hr); 

  // constant color default setup
  hr = gl_pd3dDevice->SetRenderState( D3DRS_COLORVERTEX, FALSE);      D3D_CHECKERROR(hr); 
  hr = gl_pd3dDevice->SetRenderState( D3DRS_LIGHTING,    TRUE);       D3D_CHECKERROR(hr); 
  hr = gl_pd3dDevice->SetRenderState( D3DRS_AMBIENT,     0xFFFFFFFF); D3D_CHECKERROR(hr); 
  D3DMATERIAL8 d3dMaterial;
  memset( &d3dMaterial, 0, sizeof(d3dMaterial));
  d3dMaterial.Diffuse.r = d3dMaterial.Ambient.r = 1.0f;
  d3dMaterial.Diffuse.g = d3dMaterial.Ambient.g = 1.0f;
  d3dMaterial.Diffuse.b = d3dMaterial.Ambient.b = 1.0f;
  d3dMaterial.Diffuse.a = d3dMaterial.Ambient.a = 1.0f;
  hr = gl_pd3dDevice->SetMaterial(&d3dMaterial);
  D3D_CHECKERROR(hr); 
  GFX_bColorArray = FALSE;

  // disable texturing
  extern BOOL GFX_abTexture[GFX_MAXTEXUNITS];
  for( INDEX iUnit=0; iUnit<GFX_MAXTEXUNITS; iUnit++) {
    GFX_abTexture[iUnit] = FALSE;
    GFX_iTexModulation[iUnit] = 1;
    hr = gl_pd3dDevice->SetTexture( iUnit, NULL);                                      D3D_CHECKERROR(hr);
    hr = gl_pd3dDevice->SetTextureStageState( iUnit, D3DTSS_COLOROP, D3DTOP_DISABLE);  D3D_CHECKERROR(hr); 
    hr = gl_pd3dDevice->SetTextureStageState( iUnit, D3DTSS_ALPHAOP, D3DTOP_MODULATE); D3D_CHECKERROR(hr); 
  }
  // set default texture unit and modulation mode
  GFX_iActiveTexUnit = 0;
  // reset frustum/ortho matrix
  extern BOOL  GFX_bViewMatrix;
  extern FLOAT GFX_fLastL, GFX_fLastR, GFX_fLastT, GFX_fLastB, GFX_fLastN, GFX_fLastF;
  GFX_fLastL = GFX_fLastR = GFX_fLastT = GFX_fLastB = GFX_fLastN = GFX_fLastF = 0;
  GFX_bViewMatrix = TRUE;

  // reset depth range
  D3DVIEWPORT8 d3dViewPort = { 0,0, 8,8, 0,1 };
  hr = gl_pd3dDevice->SetViewport( &d3dViewPort);  D3D_CHECKERROR(hr);
  hr = gl_pd3dDevice->GetViewport( &d3dViewPort);  D3D_CHECKERROR(hr);
  ASSERT( d3dViewPort.MinZ==0 && d3dViewPort.MaxZ==1);
  GFX_fMinDepthRange = 0.0f;
  GFX_fMaxDepthRange = 1.0f;

  // get capabilites
  D3DCAPS8 d3dCaps;
  hr = gl_pd3dDevice->GetDeviceCaps( &d3dCaps);
  D3D_CHECKERROR(hr);

  // determine rasterizer acceleration
  gl_ulFlags &= ~GLF_HASACCELERATION;
  if( (d3dCaps.DevCaps & D3DDEVCAPS_HWRASTERIZATION)
    || d3dDevType==D3DDEVTYPE_REF) gl_ulFlags |= GLF_HASACCELERATION;

  // determine support for 32-bit textures
  gl_ulFlags &= ~GLF_32BITTEXTURES;
  if( HasTextureFormat_D3D(D3DFMT_X8R8G8B8)
   || HasTextureFormat_D3D(D3DFMT_A8R8G8B8)) gl_ulFlags |= GLF_32BITTEXTURES;

  // determine support for compressed textures
  gl_ulFlags &= ~GLF_TEXTURECOMPRESSION;
  if( HasTextureFormat_D3D(D3DFMT_DXT1)) gl_ulFlags |= GLF_TEXTURECOMPRESSION;

  // determine max supported dimension of texture
  gl_pixMaxTextureDimension = d3dCaps.MaxTextureWidth;
  ASSERT( gl_pixMaxTextureDimension == d3dCaps.MaxTextureHeight); // perhaps not ?

  // determine support for disabling of color buffer writes
  gl_ulFlags &= ~GLF_D3D_COLORWRITES;
  if( d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_COLORWRITEENABLE) gl_ulFlags |= GLF_D3D_COLORWRITES;

  // determine support for custom clip planes
  gl_ulFlags &= ~GLF_D3D_CLIPPLANE;
  if( d3dCaps.MaxUserClipPlanes>0) gl_ulFlags |= GLF_D3D_CLIPPLANE;
  else CPrintF( TRANS("User clip plane not supported - mirrors will not work well.\n"));

  // determine support for texture LOD biasing
  gl_fMaxTextureLODBias = 0.0f;
  if( d3dCaps.RasterCaps & D3DPRASTERCAPS_MIPMAPLODBIAS) {
    gl_fMaxTextureLODBias = 4.0f;
  }

  // determine support for anisotropic filtering
  gl_iMaxTextureAnisotropy = 1;
  if( d3dCaps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY) {
    gl_iMaxTextureAnisotropy = d3dCaps.MaxAnisotropy;
    ASSERT( gl_iMaxTextureAnisotropy>1); 
  }

  // determine support for z-biasing
  gl_ulFlags &= ~GLF_D3D_ZBIAS;
  if( d3dCaps.RasterCaps & D3DPRASTERCAPS_ZBIAS) gl_ulFlags |= GLF_D3D_ZBIAS;

  // check support for vsync swapping
  gl_ulFlags &= ~GLF_VSYNC;
  if( d3dCaps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE) {
    if( d3dCaps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE) gl_ulFlags |= GLF_VSYNC;  
  } else CPrintF( TRANS("  Vertical syncronization cannot be disabled.\n"));

  // determine support for N-Patches
  extern INDEX truform_iLevel;
  extern BOOL  truform_bLinear;
  truform_iLevel  = -1;
  truform_bLinear = FALSE;
  gl_iTessellationLevel    = 0;
  gl_iMaxTessellationLevel = 0;
  INDEX ctMinStreams = GFX_MINSTREAMS; // set minimum number of required streams
  if( d3dCaps.DevCaps & D3DDEVCAPS_NPATCHES) {
    if( gl_ctMaxStreams>GFX_MINSTREAMS) {
      gl_iMaxTessellationLevel = 7;
      hr = gl_pd3dDevice->SetRenderState( D3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE);
      D3D_CHECKERROR(hr);
      ctMinStreams++; // need an extra stream for normals now
    } else CPrintF( TRANS("Not enough streams - N-Patches cannot be used.\n"));
  }

  // determine support for multi-texturing (only if Modulate2X mode is supported!)
  gl_ctTextureUnits = 1;
  gl_ctRealTextureUnits = d3dCaps.MaxSimultaneousTextures;
  if( gl_ctRealTextureUnits>1) {
    // check everything that is required for multi-texturing
    if( !(d3dCaps.TextureOpCaps&D3DTOP_MODULATE2X)) CPrintF( TRANS("Texture operation MODULATE2X missing - multi-texturing cannot be used.\n"));
    else if( gl_ctMaxStreams<=ctMinStreams)         CPrintF( TRANS("Not enough streams - multi-texturing cannot be used.\n"));
    else gl_ctTextureUnits = Min( (INDEX)GFX_MAXTEXUNITS, Min( gl_ctRealTextureUnits, (INDEX)1+gl_ctMaxStreams-ctMinStreams));
  }

  // setup fog and haze textures
  extern PIX _fog_pixSizeH;
  extern PIX _fog_pixSizeL;
  extern PIX _haze_pixSize;
  _fog_ulTexture  = NONE;
  _haze_ulTexture = NONE;
  _fog_pixSizeH = 0;
  _fog_pixSizeL = 0;
  _haze_pixSize = 0;

  // prepare pattern texture
  extern CTexParams _tpPattern;
  extern ULONG _ulPatternTexture;
  extern ULONG _ulLastUploadedPattern;
  _ulPatternTexture = NONE;
  _ulLastUploadedPattern = 0;
  _tpPattern.Clear();

  // determine number of color/texcoord buffers
  gl_ctTexBuffers = gl_ctTextureUnits;
  gl_ctColBuffers = 1;
  INDEX ctStreamsRemain = gl_ctMaxStreams - (ctMinStreams-1+gl_ctTextureUnits); // -1 because of 1 texture unit inside MinStreams
  FOREVER {
    // done if no more or enough streams
    if( ctStreamsRemain==0 || (gl_ctTexBuffers==GFX_MAXLAYERS && gl_ctColBuffers==GFX_MAXLAYERS)) break;
    // increase number of tex or color buffers
    if( gl_ctColBuffers<gl_ctTexBuffers) gl_ctColBuffers++;
    else gl_ctTexBuffers++;
    // next stream (if available)
    ctStreamsRemain--;
  }

  // prepare vertex arrays
  gl_pd3dIdx = NULL;
  gl_pd3dVtx = NULL;
  gl_pd3dNor = NULL;
  for( INDEX i=0; i<GFX_MAXLAYERS; i++) gl_pd3dCol[i] = gl_pd3dTex[i] = NULL;
  ASSERT( gl_ctTexBuffers>0 && gl_ctTexBuffers<=GFX_MAXLAYERS);
  ASSERT( gl_ctColBuffers>0 && gl_ctColBuffers<=GFX_MAXLAYERS);
  gl_ctVertices = 0;
  gl_ctIndices  = 0;
  extern INDEX d3d_iVertexBuffersSize;
  extern INDEX _iLastVertexBufferSize;
  const  INDEX ctVertices = VerticesFromSize_D3D(d3d_iVertexBuffersSize);
  _iLastVertexBufferSize  = d3d_iVertexBuffersSize;
  SetupVertexArrays_D3D(ctVertices); 
  SetupIndexArray_D3D(2*ctVertices);

  // reset texture filtering and some static vars
  _tpGlobal[0].Clear();
  _tpGlobal[1].Clear();
  _tpGlobal[2].Clear();
  _tpGlobal[3].Clear();
  _avsShaders.Clear();
  _iVtxOffset = 0;
  _iIdxOffset = 0;
  _dwCurrentVS = NONE;
  _ulStreamsMask = NONE;
  _ulLastStreamsMask = NONE;
  _bProjectiveMapping = FALSE;
  _bLastProjectiveMapping = FALSE;
  gl_dwVertexShader = NONE;
  GFX_ctVertices = 0;
  // reset locking flags
  _dwVtxLockFlags = D3DLOCK_DISCARD;
  INDEX i;
  for( i=0; i<GFX_MAXLAYERS; i++) _dwColLockFlags[i] = _dwTexLockFlags[i] = D3DLOCK_DISCARD;

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

  // reload all loaded textures and eventually shadowmaps
  extern INDEX shd_bCacheAll;
  extern void ReloadTextures(void);
  extern void CacheShadows(void);
  ReloadTextures();
  if( shd_bCacheAll) CacheShadows();
}



// find depth buffer format (for specified color format) that closest matches required bit depth
static D3DFORMAT FindDepthFormat_D3D( INDEX iAdapter, D3DFORMAT d3dfColor, INDEX &iDepthBits)
{
  // safeties
  ASSERT( iDepthBits==0 || iDepthBits==16 || iDepthBits==24 || iDepthBits==32);
  ASSERT( d3dfColor==D3DFMT_R5G6B5 || d3dfColor==D3DFMT_X8R8G8B8);

  // adjust required Z-depth from color depth if needed
       if( iDepthBits==0 && d3dfColor==D3DFMT_R5G6B5)   iDepthBits = 16;
  else if( iDepthBits==0 && d3dfColor==D3DFMT_X8R8G8B8) iDepthBits = 32;

  // determine closest z-depth
  D3DFORMAT ad3dFormats[] = { D3DFMT_D32,   // 32-bits
                              D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4, // 24-bits
                              D3DFMT_D16,   D3DFMT_D15S1, D3DFMT_D16_LOCKABLE }; // 16-bits
  const INDEX ctFormats = sizeof(ad3dFormats) / sizeof(ad3dFormats[0]);
  
  // find starting point from which format to search for support
  INDEX iStart;
       if( iDepthBits==32) iStart = 0;
  else if( iDepthBits==24) iStart = 1;
  else                     iStart = 4;
  // search
  INDEX i;
  HRESULT hr;
  for( i=iStart; i<ctFormats; i++) {
    hr = _pGfx->gl_pD3D->CheckDepthStencilMatch( iAdapter, d3dDevType, d3dfColor, d3dfColor, ad3dFormats[i]);
    if( hr==D3D_OK) break;
  } 

  // not found?
  if( i==ctFormats) {
    // do additional check for whole format list
    for( i=0; i<iStart; i++) {
      hr = _pGfx->gl_pD3D->CheckDepthStencilMatch( iAdapter, d3dDevType, d3dfColor, d3dfColor, ad3dFormats[i]); 
      if( hr==D3D_OK) break;
    }
    // what, z-buffer still not supported?
    if( i==iStart) {
      ASSERT( "FindDepthFormat_D3D: Z-buffer format not found?!" );
      iDepthBits = 0;
      return D3DFMT_UNKNOWN; 
    }
  } 
  // aaaah, found! :)
  ASSERT( i>=0 && i<ctFormats);
       if( i>3) iDepthBits = 16;
  else if( i>0) iDepthBits = 24;
  else          iDepthBits = 32;
  return ad3dFormats[i];
}


// prepare display mode
BOOL CGfxLibrary::InitDisplay_D3D( INDEX iAdapter, PIX pixSizeI, PIX pixSizeJ,
                                   enum DisplayDepth eColorDepth)
{
  // reset
  HRESULT hr;
  D3DDISPLAYMODE d3dDisplayMode;
  D3DPRESENT_PARAMETERS d3dPresentParams;
  gl_pD3D->GetAdapterDisplayMode( iAdapter, &d3dDisplayMode);
  memset( &d3dPresentParams, 0, sizeof(d3dPresentParams));

  // readout device capabilites
  D3DCAPS8 d3dCaps;
  hr = gl_pD3D->GetDeviceCaps( iAdapter, d3dDevType, &d3dCaps);
  D3D_CHECKERROR(hr);

  // clamp depth/stencil values
  extern INDEX gap_iDepthBits;
  extern INDEX gap_iStencilBits;
       if( gap_iDepthBits <12) gap_iDepthBits   = 0;
  else if( gap_iDepthBits <22) gap_iDepthBits   = 16;
  else if( gap_iDepthBits <28) gap_iDepthBits   = 24;
  else                         gap_iDepthBits   = 32;
       if( gap_iStencilBits<3) gap_iStencilBits = 0;
  else if( gap_iStencilBits<7) gap_iStencilBits = 4;
  else                         gap_iStencilBits = 8;

  // prepare  
  INDEX iZDepth = gap_iDepthBits;
  D3DFORMAT d3dDepthFormat = D3DFMT_UNKNOWN;
  D3DFORMAT d3dColorFormat = d3dDisplayMode.Format;
  d3dPresentParams.BackBufferCount = 1;
  d3dPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE; // !!!! TODO
  d3dPresentParams.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
  d3dPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
  const BOOL bFullScreen = (pixSizeI>0 && pixSizeJ>0); 

  // setup for full screen
  if( bFullScreen) {
    // determine color and depth format
    if( eColorDepth==DD_16BIT) d3dColorFormat = D3DFMT_R5G6B5;
    if( eColorDepth==DD_32BIT) d3dColorFormat = D3DFMT_X8R8G8B8;
    d3dDepthFormat = FindDepthFormat_D3D( iAdapter, d3dColorFormat, iZDepth);
    // determine refresh rate and presentation interval
    extern INDEX gap_iRefreshRate;
    const UINT uiRefresh = gap_iRefreshRate>0 ? gap_iRefreshRate : D3DPRESENT_RATE_DEFAULT;
    const SLONG slIntervals = d3dCaps.PresentationIntervals;
    UINT uiInterval = (slIntervals&D3DPRESENT_INTERVAL_IMMEDIATE) ? D3DPRESENT_INTERVAL_IMMEDIATE : D3DPRESENT_INTERVAL_ONE;
    extern INDEX gap_iSwapInterval;
    switch(gap_iSwapInterval) {
    case 1:  if( slIntervals&D3DPRESENT_INTERVAL_ONE)   uiInterval=D3DPRESENT_INTERVAL_ONE;    break; 
    case 2:  if( slIntervals&D3DPRESENT_INTERVAL_TWO)   uiInterval=D3DPRESENT_INTERVAL_TWO;    break; 
    case 3:  if( slIntervals&D3DPRESENT_INTERVAL_THREE) uiInterval=D3DPRESENT_INTERVAL_THREE;  break; 
    case 4:  if( slIntervals&D3DPRESENT_INTERVAL_FOUR)  uiInterval=D3DPRESENT_INTERVAL_FOUR;   break; 
    default: break;
    } // construct back cvar
    switch(uiInterval) {
    case 1:  gap_iSwapInterval=1;  break; 
    case 2:  gap_iSwapInterval=2;  break; 
    case 3:  gap_iSwapInterval=3;  break; 
    case 4:  gap_iSwapInterval=4;  break; 
    default: gap_iSwapInterval=0;  break;
    } gl_iSwapInterval = gap_iSwapInterval; // copy to gfx lib
    // set context directly to main window
    d3dPresentParams.Windowed = FALSE;
    d3dPresentParams.BackBufferWidth  = pixSizeI;
    d3dPresentParams.BackBufferHeight = pixSizeJ;
    d3dPresentParams.BackBufferFormat = d3dColorFormat;
    d3dPresentParams.EnableAutoDepthStencil = TRUE;
    d3dPresentParams.AutoDepthStencilFormat = d3dDepthFormat;
    d3dPresentParams.FullScreen_RefreshRateInHz = uiRefresh;
    d3dPresentParams.FullScreen_PresentationInterval = uiInterval;
  }
  // setup for windowed mode
  else {
    // create dummy Direct3D context
    d3dPresentParams.Windowed = TRUE;
    d3dPresentParams.BackBufferWidth  = 8;
    d3dPresentParams.BackBufferHeight = 8;
    d3dPresentParams.BackBufferFormat = d3dColorFormat;
    d3dPresentParams.EnableAutoDepthStencil = FALSE;
    d3dDepthFormat = FindDepthFormat_D3D( iAdapter, d3dColorFormat, iZDepth);
    gl_iSwapInterval = -1;
  }
  // determine HW or SW vertex processing
  DWORD dwVP = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
  gl_ulFlags &= ~(GLF_D3D_HASHWTNL|GLF_D3D_USINGHWTNL); 
  gl_ctMaxStreams = 16; // software T&L has enough streams
  extern INDEX d3d_bUseHardwareTnL;

  // cannot have HW VP if not supported by HW, right?
  if( d3dCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
    gl_ulFlags |= GLF_D3D_HASHWTNL;
    gl_ctMaxStreams = d3dCaps.MaxStreams;
    if( gl_ctMaxStreams<GFX_MINSTREAMS) d3d_bUseHardwareTnL = 0; // cannot use HW T&L if not enough streams
    if( d3d_bUseHardwareTnL) {
      d3d_bUseHardwareTnL = 1; // clamp just in case
      dwVP = D3DCREATE_HARDWARE_VERTEXPROCESSING;
      gl_ulFlags |= GLF_D3D_USINGHWTNL;
    } // no HW T&L 
  } else d3d_bUseHardwareTnL = 0;

  // go for it ...
  extern HWND _hwndMain;
  extern const D3DDEVTYPE d3dDevType;
  hr = gl_pD3D->CreateDevice( iAdapter, d3dDevType, _hwndMain, dwVP, &d3dPresentParams, &gl_pd3dDevice);
  if( hr!=D3D_OK) return FALSE;
  gl_d3dColorFormat = d3dColorFormat;
  gl_d3dDepthFormat = d3dDepthFormat;
  gl_iCurrentDepth  = iZDepth;
  return TRUE;
}
    



// fallback D3D internal format
// (reverts to next format that closely matches requied one)
static D3DFORMAT FallbackFormat_D3D( D3DFORMAT eFormat, BOOL b2ndTry)
{
  switch( eFormat) {
  case D3DFMT_X8R8G8B8: return !b2ndTry ? D3DFMT_A8R8G8B8 : D3DFMT_R5G6B5;
  case D3DFMT_X1R5G5B5: return !b2ndTry ? D3DFMT_R5G6B5   : D3DFMT_A1R5G5B5;
  case D3DFMT_X4R4G4B4: return !b2ndTry ? D3DFMT_R5G6B5   : D3DFMT_A1R5G5B5;
  case D3DFMT_R5G6B5:   return !b2ndTry ? D3DFMT_X1R5G5B5 : D3DFMT_A1R5G5B5;
  case D3DFMT_L8:       return !b2ndTry ? D3DFMT_A8L8     : D3DFMT_X8R8G8B8;
  case D3DFMT_A8L8:     return D3DFMT_A8R8G8B8;
  case D3DFMT_A1R5G5B5: return D3DFMT_A4R4G4B4;
  case D3DFMT_A8R8G8B8: return D3DFMT_A4R4G4B4;
  case D3DFMT_DXT1:     return D3DFMT_A1R5G5B5;
  case D3DFMT_DXT3:     return D3DFMT_A4R4G4B4;
  case D3DFMT_DXT5:     return D3DFMT_A4R4G4B4;
  case D3DFMT_A4R4G4B4: // must have this one!
  default: ASSERTALWAYS( "Can't fallback texture format.");
  } // missed!
  return D3DFMT_UNKNOWN;
}


// find closest 
extern D3DFORMAT FindClosestFormat_D3D( D3DFORMAT d3df)
{
  FOREVER {
    if( HasTextureFormat_D3D(d3df)) return d3df;
    D3DFORMAT d3df2 = FallbackFormat_D3D( d3df, FALSE);
    if( HasTextureFormat_D3D(d3df2)) return d3df2;
    d3df = FallbackFormat_D3D( d3df, TRUE);
  }
}



// VERTEX/INDEX BUFFERS SUPPORT THRU STREAMS


// DEBUG helper
static void CheckStreams(void)
{
  UINT uiRet, ui;
  INDEX iRef, iPass;
  DWORD dwVS;
  HRESULT hr;
  LPDIRECT3DVERTEXBUFFER8 pVBRet, pVB;
  LPDIRECT3DINDEXBUFFER8 pIBRet;
  const LPDIRECT3DDEVICE8 pd3dDev = _pGfx->gl_pd3dDevice;

  // check passes and buffer position
  ASSERT( _iTexPass>=0 && _iColPass>=0);
  ASSERT( _iVtxPos >=0 && _iVtxPos<65536);

  // check vertex positions
  ASSERT( _ulStreamsMask & (1<<POSIDX)); // must be in shader!
  hr = pd3dDev->GetStreamSource( POSIDX, &pVBRet, &uiRet);
  D3D_CHECKERROR(hr);
  ASSERT( pVBRet!=NULL);
  iRef = pVBRet->Release();
  ASSERT( iRef==1 && pVBRet==_pGfx->gl_pd3dVtx && uiRet==POSSIZE);

  // check normals
  pVB = NULL;
  ui  = NORSIZE;
  hr  = pd3dDev->GetStreamSource( NORIDX, &pVBRet, &uiRet);
  D3D_CHECKERROR(hr);
  if( pVBRet!=NULL) iRef = pVBRet->Release();
  if( _ulStreamsMask & (1<<NORIDX)) pVB = _pGfx->gl_pd3dNor;
  ASSERT( iRef==1 && pVBRet==pVB && (uiRet==ui || uiRet==0));

  // check colors
  pVB = NULL;
  ui  = COLSIZE;
  hr  = pd3dDev->GetStreamSource( COLIDX, &pVBRet, &uiRet);
  D3D_CHECKERROR(hr);
  if( pVBRet!=NULL) iRef = pVBRet->Release();
  if( _ulStreamsMask & (1<<COLIDX)) {
    iPass = (_iColPass-1) % _pGfx->gl_ctColBuffers;
    pVB = _pGfx->gl_pd3dCol[iPass];
  }
  ASSERT( iRef==1 && pVBRet==pVB && (uiRet==ui || uiRet==0));

  // check 1st texture coords
  pVB = NULL;
  ui  = _bProjectiveMapping ? TX4SIZE : TEXSIZE;
  hr  = pd3dDev->GetStreamSource( TEXIDX, &pVBRet, &uiRet);
  D3D_CHECKERROR(hr);
  if( pVBRet!=NULL) iRef = pVBRet->Release();
  if( _ulStreamsMask & (1<<(TEXIDX))) {
    iPass = (_iTexPass-1) % _pGfx->gl_ctTexBuffers;
    pVB = _pGfx->gl_pd3dTex[iPass];
  }
  ASSERT( iRef==1 && pVBRet==pVB && (uiRet==ui || uiRet==0));
  
  // check indices
  hr = pd3dDev->GetIndices( &pIBRet, &uiRet);
  D3D_CHECKERROR(hr);
  ASSERT( pIBRet!=NULL);
  iRef = pIBRet->Release();
  ASSERT( iRef==1 && pIBRet==_pGfx->gl_pd3dIdx && uiRet==0);

  // check shader
  hr = pd3dDev->GetVertexShader( &dwVS);
  D3D_CHECKERROR(hr);
  ASSERT( dwVS!=NONE && dwVS==_pGfx->gl_dwVertexShader);

  /* check shader declaration (SEEMS LIKE THIS SHIT DOESN'T WORK!)
  const INDEX ctMaxDecls = 2*MAXSTREAMS+1;
  INDEX ctDecls = ctMaxDecls;
  DWORD adwDeclRet[ctMaxDecls];
  hr = pd3dDev->GetVertexShaderDeclaration( _pGfx->gl_dwVertexShader, (void*)&adwDeclRet[0], (DWORD*)&ctDecls);
  D3D_CHECKERROR(hr);
  ASSERT( ctDecls>0 && ctDecls<ctMaxDecls);
  INDEX iRet = memcmp( &adwDeclRet[0], &_adwCurrentDecl[0], ctDecls*sizeof(DWORD));
  ASSERT( iRet==0); */
}



// prepare vertex array for D3D (type 0=vtx, 1=nor, 2=col, 3=tex, 4=projtex)
extern void SetVertexArray_D3D( INDEX iType, ULONG *pulVtx)
{
  HRESULT hr;
  SLONG slStride;
  DWORD dwLockFlag;
  INDEX iStream, iThisPass;
  INDEX ctLockSize, iLockOffset;
  LPDIRECT3DVERTEXBUFFER8 pd3dVB;
  ASSERT( _iTexPass>=0 && _iColPass>=0);
  ASSERT( _iVtxPos >=0 && _iVtxPos<65536);
  const BOOL bHWTnL = _pGfx->gl_ulFlags & GLF_D3D_USINGHWTNL;

  // determine which buffer we work on
  switch(iType)
  {

  // VERTICES
  case 0: 
    // make sure that we have enough space in vertex buffers
    ASSERT(GFX_ctVertices>0);
    SetupVertexArrays_D3D( GFX_ctVertices * (bHWTnL?2:1));
    // determine lock type
    pd3dVB = _pGfx->gl_pd3dVtx;
    if( !bHWTnL || (_iVtxOffset+GFX_ctVertices)>=_pGfx->gl_ctVertices) {
       // reset pos and flags
      _iVtxOffset = 0;
      _dwVtxLockFlags = D3DLOCK_DISCARD;
      for( INDEX i=0; i<GFX_MAXLAYERS; i++) _dwColLockFlags[i] = _dwTexLockFlags[i] = _dwVtxLockFlags;
    } else _dwVtxLockFlags = D3DLOCK_NOOVERWRITE;
    // keep and advance current lock position
    _iVtxPos = _iVtxOffset;
    _iVtxOffset += GFX_ctVertices;
    // determine lock type, pos and size
    dwLockFlag  = _dwVtxLockFlags;
    ctLockSize  = GFX_ctVertices*4; 
    iLockOffset = _iVtxPos*4;
    // set stream params
    iStream  = POSIDX;
    slStride = POSSIZE;
    // reset array states
    _ulStreamsMask = NONE;
    _bProjectiveMapping = FALSE;
    _iTexPass = _iColPass = 0;
    break;

  // NORMALS
  case 1: 
    ASSERT( _pGfx->gl_iMaxTessellationLevel>0 && gap_iTruformLevel>0); // only if enabled
    pd3dVB = _pGfx->gl_pd3dNor;
    ASSERT( _iTexPass<2 && _iColPass<2);  // normals must be set in 1st pass (completed or not)
    // determine lock type, pos and size
    dwLockFlag  = _dwVtxLockFlags;
    ctLockSize  = GFX_ctVertices*4; 
    iLockOffset = _iVtxPos*4; 
    // set stream params
    iStream  = NORIDX;
    slStride = NORSIZE;
    break;

  // COLORS
  case 2: 
    iThisPass = _iColPass;
    // restart in case of too many passes
    if( iThisPass>=_pGfx->gl_ctColBuffers) {
      dwLockFlag = D3DLOCK_DISCARD;
      iThisPass %= _pGfx->gl_ctColBuffers;
    } else { // continue in case of enough buffers
      dwLockFlag = _dwColLockFlags[iThisPass];
    } // mark
    _dwColLockFlags[iThisPass] = D3DLOCK_NOOVERWRITE;
    ASSERT( iThisPass>=0 && iThisPass<_pGfx->gl_ctColBuffers);
    pd3dVB = _pGfx->gl_pd3dCol[iThisPass];
    // determine lock pos and size
    ctLockSize  = GFX_ctVertices*1; 
    iLockOffset = _iVtxPos*1; 
    // set stream params
    iStream  = COLIDX;
    slStride = COLSIZE;
    _iColPass++; // advance to next color pass
    break;

  // PROJECTIVE TEXTURE COORDINATES
  case 4:
    _bProjectiveMapping = TRUE;
    // fall thru ...

  // TEXTURE COORDINATES
  case 3:
    iThisPass = _iTexPass;
    // restart in case of too many passes
    if( iThisPass>=_pGfx->gl_ctTexBuffers) {
      dwLockFlag = D3DLOCK_DISCARD;
      iThisPass %= _pGfx->gl_ctTexBuffers;
    } else { // continue in case of enough buffers
      dwLockFlag = _dwTexLockFlags[iThisPass];
    } // mark
    _dwTexLockFlags[iThisPass] = D3DLOCK_NOOVERWRITE;
    ASSERT( iThisPass>=0 && iThisPass<_pGfx->gl_ctTexBuffers);
    pd3dVB = _pGfx->gl_pd3dTex[iThisPass];
    // set stream number (must take into account tex-unit, because of multi-texturing!)
    iStream = TEXIDX +GFX_iActiveTexUnit;
    // determine stride, lock pos and size
    if( _bProjectiveMapping) {
      ctLockSize  = GFX_ctVertices*4; 
      iLockOffset = _iVtxPos*4; 
      slStride = TX4SIZE;
    } else {
      ctLockSize  = GFX_ctVertices*2; 
      iLockOffset = _iVtxPos*2; 
      slStride = TEXSIZE;
    } // advance to next texture pass
    _iTexPass++; 
    break;

  // BUF! WRONG.
  default: ASSERTALWAYS( "SetVertexArray_D3D: wrong stream number!");
  }

  ASSERT( _iTexPass>=0 && _iColPass>=0);
  ASSERT( _iVtxPos >=0 && _iVtxPos<65536);

  // fetch D3D buffer
  ULONG *pulLockedBuffer;
  hr = pd3dVB->Lock( iLockOffset*sizeof(ULONG), ctLockSize*sizeof(ULONG), (UBYTE**)&pulLockedBuffer, dwLockFlag);
  D3D_CHECKERROR(hr);

  // copy (or convert) vertices there and unlock
  ASSERT(pulVtx!=NULL); 
  if( iType!=2) CopyLongs( pulVtx, pulLockedBuffer, ctLockSize); // vertex array
  else          abgr2argb( pulVtx, pulLockedBuffer, ctLockSize); // color array (needs conversion)

  // done
  hr = pd3dVB->Unlock();
  D3D_CHECKERROR(hr);

  // update streams mask and assign 
  _ulStreamsMask |= 1<<iStream;
  hr = _pGfx->gl_pd3dDevice->SetStreamSource( iStream, pd3dVB, slStride);
  D3D_CHECKERROR(hr);
}



// prepare and draw arrays
extern void DrawElements_D3D( INDEX ctIndices, INDEX *pidx)
{
  // paranoid & sunburnt (by Skunk Anansie:)
  ASSERT( _iTexPass>=0 && _iColPass>=0);
  ASSERT( _iVtxPos >=0 && _iVtxPos<65536);
  const LPDIRECT3DDEVICE8 pd3dDev = _pGfx->gl_pd3dDevice;

  // at least one triangle must be sent
  ASSERT( ctIndices>=3 && ((ctIndices/3)*3)==ctIndices);
  if( ctIndices<3) return;
  extern INDEX d3d_iVertexRangeTreshold;
  d3d_iVertexRangeTreshold = Clamp( d3d_iVertexRangeTreshold, 0L, 9999L);

  // eventually adjust size of index buffer
  const BOOL bHWTnL = _pGfx->gl_ulFlags & GLF_D3D_USINGHWTNL;
  SetupIndexArray_D3D( ctIndices * (bHWTnL?2:1));

  // determine lock position and type
  if( (_iIdxOffset+ctIndices) >= _pGfx->gl_ctIndices) _iIdxOffset = 0;
  const DWORD dwLockFlag   = (_iIdxOffset>0) ? D3DLOCK_NOOVERWRITE : D3DLOCK_DISCARD;
  const SLONG slLockSize   =  ctIndices *IDXSIZE; 
  const SLONG slLockOffset = _iIdxOffset*IDXSIZE; 

  // copy indices to index buffer
  HRESULT hr;
  UWORD *puwLockedBuffer;
  hr = _pGfx->gl_pd3dIdx->Lock( slLockOffset, slLockSize, (UBYTE**)&puwLockedBuffer, dwLockFlag);
  D3D_CHECKERROR(hr);

  INDEX iMinIndex = 65536;
  INDEX iMaxIndex = 0;
  const BOOL bSetRange = !(_pGfx->gl_ulFlags&GLF_D3D_USINGHWTNL) && (GFX_ctVertices>d3d_iVertexRangeTreshold);
  ASSERT( _iVtxPos>=0 && _iVtxPos<65536);

#if ASMOPT == 1
  const __int64 mmSignD = 0x0000800000008000;
  const __int64 mmSignW = 0x8000800080008000;
  __asm {
    // adjust 32-bit and copy to 16-bit array
    mov     esi,D [pidx]
    mov     edi,D [puwLockedBuffer]
    mov     ecx,D [ctIndices]
    shr     ecx,2   // 4 by 4
    jz      elemL2
    movd    mm7,D [_iVtxPos]
    punpcklwd mm7,mm7
    punpckldq mm7,mm7   // MM7 = vtxPos | vtxPos || vtxPos | vtxPos
    paddw   mm7,Q [mmSignW]
elemCLoop:
    movq    mm1,Q [esi+0]
    movq    mm2,Q [esi+8]
    psubd   mm1,Q [mmSignD]
    psubd   mm2,Q [mmSignD]
    packssdw mm1,mm2
    paddw   mm1,mm7
    movq    Q [edi],mm1
    add     esi,4*4
    add     edi,2*4
    dec     ecx
    jnz     elemCLoop
    emms
elemL2:
    test    D [ctIndices],2
    jz      elemL1
    mov     eax,D [esi+0]
    mov     edx,D [esi+4]
    add     eax,D [_iVtxPos]
    add     edx,D [_iVtxPos]
    shl     edx,16
    or      eax,edx
    mov     D [edi],eax
    add     esi,4*2
    add     edi,2*2
elemL1:
    test    D [ctIndices],1
    jz      elemRange
    mov     eax,D [esi]
    add     eax,D [_iVtxPos]
    mov     W [edi],ax

elemRange:
    // find min/max index (if needed)
    cmp     D [bSetRange],0
    jz      elemEnd

    mov     edi,D [iMinIndex]
    mov     edx,D [iMaxIndex]
    mov     esi,D [pidx]
    mov     ecx,D [ctIndices]
elemTLoop:
    mov     eax,D [esi]
    add     eax,D [_iVtxPos]
    cmp     eax,edi
    cmovl   edi,eax
    cmp     eax,edx
    cmovg   edx,eax
    add     esi,4
    dec     ecx
    jnz     elemTLoop
    mov     D [iMinIndex],edi 
    mov     D [iMaxIndex],edx
elemEnd:
  }
#else
  for( INDEX idx=0; idx<ctIndices; idx++) {
    const INDEX iAdj = pidx[idx] + _iVtxPos;
         if( iMinIndex>iAdj) iMinIndex = iAdj;
    else if( iMaxIndex<iAdj) iMaxIndex = iAdj;
    puwLockedBuffer[idx] = iAdj;
  }
#endif

  // indices filled
  hr = _pGfx->gl_pd3dIdx->Unlock();
  D3D_CHECKERROR(hr);

  // check whether to use color array or not
  if( GFX_bColorArray) _ulStreamsMask |= (1<<COLIDX);
  else _ulStreamsMask &= ~(1<<COLIDX);

  // must adjust some stuff when projective mapping usage has been toggled
  if( !_bLastProjectiveMapping != !_bProjectiveMapping) {
    _bLastProjectiveMapping = _bProjectiveMapping;
    D3DTEXTURETRANSFORMFLAGS ttf;
    if( _bProjectiveMapping) {
      _ulStreamsMask |= 0x1000;
      ttf = D3DTTFF_PROJECTED;
    } else ttf = D3DTTFF_DISABLE;
    hr = pd3dDev->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, ttf);
    D3D_CHECKERROR(hr);
  }

  // eventually (re)construct vertex shader out of streams' bit-mask
  if( _ulLastStreamsMask != _ulStreamsMask)
  { // reset streams that were used before
    ULONG ulThisMask = _ulStreamsMask;
    ULONG ulLastMask = _ulLastStreamsMask;
    for( INDEX iStream=0; iStream<MAXSTREAMS; iStream++) {
      if( (ulThisMask&1)==0 && (ulLastMask&1)!=0) {
        hr = pd3dDev->SetStreamSource( iStream,NULL,0);
        D3D_CHECKERROR(hr);
      } // next stream
      ulThisMask >>= 1;
      ulLastMask >>= 1;
    }
    // setup new vertex shader
    _dwCurrentVS = SetupShader_D3D(_ulStreamsMask);
    _ulLastStreamsMask = _ulStreamsMask;
  } 

  // (re)set vertex shader
  ASSERT(_dwCurrentVS!=NONE);
  if( _pGfx->gl_dwVertexShader!=_dwCurrentVS) {
    hr = _pGfx->gl_pd3dDevice->SetVertexShader(_dwCurrentVS);
    D3D_CHECKERROR(hr);
   _pGfx->gl_dwVertexShader = _dwCurrentVS;
  }

#ifndef NDEBUG
  // Paranoid Android (by Radiohead:)
  CheckStreams();
#endif

  // determine vertex range  
  INDEX iVtxStart, ctVtxUsed;
  // if not too much vertices in buffer
  if( !bSetRange) {
    // set whole vertex buffer
    iVtxStart = _iVtxPos;
    ctVtxUsed = GFX_ctVertices;
  // if lotta vertices in buffer
  } else {
    // set only part of vertex buffer
    iVtxStart = iMinIndex;            
    ctVtxUsed = iMaxIndex-iMinIndex+1;
    ASSERT( iMinIndex<iMaxIndex);
  }

  // draw indices
  hr = pd3dDev->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, iVtxStart, ctVtxUsed, _iIdxOffset, ctIndices/3);
  D3D_CHECKERROR(hr);
  // move to next available lock position
  _iIdxOffset += ctIndices;
}

#endif // SE1_D3D
