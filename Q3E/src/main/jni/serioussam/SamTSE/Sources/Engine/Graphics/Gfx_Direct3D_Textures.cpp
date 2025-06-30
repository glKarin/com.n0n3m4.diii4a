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

#ifdef SE1_D3D

#include <d3dx8tex.h>
#pragma comment(lib, "d3dx8.lib")

#include <Engine/Graphics/GfxLibrary.h>

#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Math/Functions.h>
#include <Engine/Graphics/GfxProfile.h>

#include <Engine/Base/ListIterator.inl>


// asm shortcuts
#define O offset
#define Q qword ptr
#define D dword ptr
#define W  word ptr
#define B  byte ptr


// we need array for Direct3D mipmaps that are lower than N*1 or 1*N
static ULONG _aulLastMipmaps[(INDEX)(1024*1.334)];
static CTexParams *_tpCurrent;
static _D3DTEXTUREFILTERTYPE _eLastMipFilter;

extern INDEX GFX_iActiveTexUnit;


// conversion from OpenGL's RGBA color format to one of D3D color formats
extern void SetInternalFormat_D3D( D3DFORMAT d3dFormat);
extern void UploadMipmap_D3D( ULONG *pulSrc, LPDIRECT3DTEXTURE8 ptexDst, PIX pixWidth, PIX pixHeight, INDEX iMip);


// unpacks texture filtering from one INDEX to two GLenums (and eventually re-adjust input INDEX)
extern void UnpackFilter_D3D( INDEX iFilter, _D3DTEXTUREFILTERTYPE &eMagFilter,
                             _D3DTEXTUREFILTERTYPE &eMinFilter, _D3DTEXTUREFILTERTYPE &eMipFilter)
{
  switch( iFilter) {
  case 110:  case 10:  eMagFilter=D3DTEXF_POINT;  eMinFilter=D3DTEXF_POINT;  eMipFilter=D3DTEXF_NONE;   break;
  case 111:  case 11:  eMagFilter=D3DTEXF_POINT;  eMinFilter=D3DTEXF_POINT;  eMipFilter=D3DTEXF_POINT;  break;
  case 112:  case 12:  eMagFilter=D3DTEXF_POINT;  eMinFilter=D3DTEXF_POINT;  eMipFilter=D3DTEXF_LINEAR; break;
  case 220:  case 20:  eMagFilter=D3DTEXF_LINEAR; eMinFilter=D3DTEXF_LINEAR; eMipFilter=D3DTEXF_NONE;   break;
  case 221:  case 21:  eMagFilter=D3DTEXF_LINEAR; eMinFilter=D3DTEXF_LINEAR; eMipFilter=D3DTEXF_POINT;  break;
  case 222:  case 22:  eMagFilter=D3DTEXF_LINEAR; eMinFilter=D3DTEXF_LINEAR; eMipFilter=D3DTEXF_LINEAR; break;
  case 120:            eMagFilter=D3DTEXF_POINT;  eMinFilter=D3DTEXF_LINEAR; eMipFilter=D3DTEXF_NONE;   break;
  case 121:            eMagFilter=D3DTEXF_POINT;  eMinFilter=D3DTEXF_LINEAR; eMipFilter=D3DTEXF_POINT;  break;
  case 122:            eMagFilter=D3DTEXF_POINT;  eMinFilter=D3DTEXF_LINEAR; eMipFilter=D3DTEXF_LINEAR; break;
  case 210:            eMagFilter=D3DTEXF_LINEAR; eMinFilter=D3DTEXF_POINT;  eMipFilter=D3DTEXF_NONE;   break;
  case 211:            eMagFilter=D3DTEXF_LINEAR; eMinFilter=D3DTEXF_POINT;  eMipFilter=D3DTEXF_POINT;  break;
  case 212:            eMagFilter=D3DTEXF_LINEAR; eMinFilter=D3DTEXF_POINT;  eMipFilter=D3DTEXF_LINEAR; break;
  default: ASSERTALWAYS( "Illegal Direct3D texture filtering mode."); break;
  }
}


// change texture filtering mode if needed
extern void MimicTexParams_D3D( CTexParams &tpLocal)
{
  ASSERT( &tpLocal!=NULL);
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_TEXTUREPARAMS);

  // update texture filtering mode if required
  if( tpLocal.tp_iFilter != _tpGlobal[0].tp_iFilter) tpLocal.tp_iFilter = _tpGlobal[0].tp_iFilter;

  // eventually adjust filtering for textures w/o mipmaps
  const INDEX iMipFilter = _tpGlobal[0].tp_iFilter % 10;
  if( (!tpLocal.tp_bSingleMipmap != !_tpGlobal[GFX_iActiveTexUnit].tp_bSingleMipmap) && iMipFilter!=0)
  { 
    HRESULT hr;
    _D3DTEXTUREFILTERTYPE eMipFilter;
    extern INDEX GFX_iActiveTexUnit;

    // no mipmaps?
    if( tpLocal.tp_bSingleMipmap) {
#ifndef NDEBUG
      // paranoid!
      hr = _pGfx->gl_pd3dDevice->GetTextureStageState( GFX_iActiveTexUnit, D3DTSS_MIPFILTER, (ULONG*)&eMipFilter);
      D3D_CHECKERROR(hr);
      ASSERT( eMipFilter==D3DTEXF_POINT || eMipFilter==D3DTEXF_LINEAR);
#endif // set it
      hr = _pGfx->gl_pd3dDevice->SetTextureStageState( GFX_iActiveTexUnit, D3DTSS_MIPFILTER, D3DTEXF_NONE);
    }
    // yes mipmaps?
    else {
      switch( iMipFilter) {
      case 0: eMipFilter = D3DTEXF_NONE;   break; 
      case 1: eMipFilter = D3DTEXF_POINT;  break; 
      case 2: eMipFilter = D3DTEXF_LINEAR; break; 
      default: ASSERTALWAYS( "Invalid mipmap filtering mode.");
      } // set it
      hr = _pGfx->gl_pd3dDevice->SetTextureStageState( GFX_iActiveTexUnit, D3DTSS_MIPFILTER, eMipFilter);
    } 
    // check and update mipmap state
    D3D_CHECKERROR(hr);
    _tpGlobal[GFX_iActiveTexUnit].tp_bSingleMipmap = tpLocal.tp_bSingleMipmap;
  }

  // update texture anisotropy degree
  if( tpLocal.tp_iAnisotropy != _tpGlobal[0].tp_iAnisotropy) tpLocal.tp_iAnisotropy = _tpGlobal[0].tp_iAnisotropy;

  // update texture clamping modes if changed
  if( tpLocal.tp_eWrapU!=_tpGlobal[GFX_iActiveTexUnit].tp_eWrapU || tpLocal.tp_eWrapV!=_tpGlobal[GFX_iActiveTexUnit].tp_eWrapV) { 
    tpLocal.tp_eWrapU = _tpGlobal[GFX_iActiveTexUnit].tp_eWrapU;
    tpLocal.tp_eWrapV = _tpGlobal[GFX_iActiveTexUnit].tp_eWrapV;
  }

  // keep last texture params (for tex upload and stuff)
  _tpCurrent = &tpLocal;
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_TEXTUREPARAMS);
}




// upload context for current texture to accelerator's memory
// (returns format in which texture was really uploaded)
extern void UploadTexture_D3D( LPDIRECT3DTEXTURE8 *ppd3dTexture, ULONG *pulTexture,
                               PIX pixSizeU, PIX pixSizeV, D3DFORMAT eInternalFormat, BOOL bDiscard)
{
  // safeties
  ASSERT( pulTexture!=NULL);
  ASSERT( pixSizeU>0 && pixSizeV>0);
  _sfStats.StartTimer( CStatForm::STI_BINDTEXTURE);
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_TEXTUREUPLOADING);

  // recreate texture if needed
  HRESULT hr;
  if( bDiscard) {
    if( (*ppd3dTexture)!=NULL) D3DRELEASE( (*ppd3dTexture), TRUE);
    hr = _pGfx->gl_pd3dDevice->CreateTexture( pixSizeU, pixSizeV, 0, 0, eInternalFormat, D3DPOOL_MANAGED, ppd3dTexture);
    D3D_CHECKERROR(hr);
  }
  // D3D texture must be valid now
  LPDIRECT3DTEXTURE8 pd3dTex = (*ppd3dTexture);
  ASSERT( pd3dTex!=NULL);

  // prepare routine for conversion
  SetInternalFormat_D3D(eInternalFormat);
  
  // upload each mipmap
  INDEX iMip=0;
  PIX pixOffset=0;
  while( pixSizeU>0 && pixSizeV>0)
  { 
    // check that memory is readable and upload one mipmap
    ASSERT( pulTexture[pixOffset +pixSizeU*pixSizeV -1] != 0xDEADBEEF);
    UploadMipmap_D3D( pulTexture+pixOffset, pd3dTex, pixSizeU, pixSizeV, iMip);
    // advance to next mip-map
    pixOffset += pixSizeU*pixSizeV;
    pixSizeU >>=1;
    pixSizeV >>=1;
    iMip++;
    // end here if there is only one mip-map to upload
    if( _tpCurrent->tp_bSingleMipmap) break;
  }

  // see if we need to generate and upload additional mipmaps (those under 1*N or N*1)
  if( !_tpCurrent->tp_bSingleMipmap && pixSizeU!=pixSizeV)
  { // prepare variables
    PIX pixSize = Max(pixSizeU,pixSizeV);
    ASSERT( pixSize<=2048);
    ULONG *pulSrc = pulTexture+pixOffset-pixSize*2;
    ULONG *pulDst = _aulLastMipmaps;
    // loop thru mipmaps
    while( pixSizeU>0 || pixSizeV>0)
    { // make next mipmap
      if( pixSizeU==0) pixSizeU=1;
      if( pixSizeV==0) pixSizeV=1;
      pixSize = pixSizeU*pixSizeV;
      __asm {   
        pxor    mm0,mm0
        mov     esi,D [pulSrc]
        mov     edi,D [pulDst]
        mov     ecx,D [pixSize]
  pixLoop:
        movd    mm1,D [esi+0]
        movd    mm2,D [esi+4]
        punpcklbw mm1,mm0
        punpcklbw mm2,mm0
        paddw   mm1,mm2
        psrlw   mm1,1
        packuswb mm1,mm0
        movd    D [edi],mm1
        add     esi,4*2
        add     edi,4
        dec     ecx
        jnz     pixLoop
        emms
      }
      // upload mipmap and advance
      UploadMipmap_D3D( pulDst, pd3dTex, pixSizeU, pixSizeV, iMip);
      pulSrc     = pulDst;
      pulDst    += pixSize;
      pixOffset += pixSize;
      pixSizeU >>=1;
      pixSizeV >>=1;
      iMip++;
    }
  }

  // all done
  _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_TEXTUREUPLOADS, 1);
  _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_TEXTUREUPLOADBYTES, pixOffset*4);
  _sfStats.IncrementCounter( CStatForm::SCI_TEXTUREUPLOADS, 1);
  _sfStats.IncrementCounter( CStatForm::SCI_TEXTUREUPLOADBYTES, pixOffset*4);
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_TEXTUREUPLOADING);
  _sfStats.StopTimer( CStatForm::STI_BINDTEXTURE);
}



// returns bytes/pixels ratio for texture format
extern INDEX GetFormatPixRatio_D3D( D3DFORMAT d3dFormat)
{
  switch( d3dFormat) {
  case D3DFMT_A8R8G8B8:
  case D3DFMT_X8R8G8B8:
    return 4;
  case D3DFMT_R8G8B8:
    return 3;
  case D3DFMT_R5G6B5:
  case D3DFMT_X1R5G5B5:
  case D3DFMT_A1R5G5B5:
  case D3DFMT_A4R4G4B4:
  case D3DFMT_X4R4G4B4:
  case D3DFMT_A8L8:
    return 2;
  // compressed formats and single-channel formats
  default:
    return 1;
  }
}


// returns bytes/pixels ratio for uploaded texture
extern INDEX GetTexturePixRatio_D3D( LPDIRECT3DTEXTURE8 pd3dTexture)
{
  D3DSURFACE_DESC d3dSurfDesc;
  HRESULT hr = pd3dTexture->GetLevelDesc( 0, &d3dSurfDesc);
  D3D_CHECKERROR(hr);
  return GetFormatPixRatio_D3D( d3dSurfDesc.Format);
}


// return allowed dithering method
extern INDEX AdjustDitheringType_D3D( D3DFORMAT eFormat, INDEX iDitheringType)
{
  switch( eFormat) {
  // these formats don't need dithering
  case D3DFMT_A8R8G8B8:
  case D3DFMT_X8R8G8B8:
  case D3DFMT_L8:
  case D3DFMT_A8L8:
    return NONE;
  // these formats need reduced dithering
  case D3DFMT_R5G6B5:
  case D3DFMT_X1R5G5B5:
  case D3DFMT_A1R5G5B5:
    if( iDitheringType>7) return iDitheringType-3;
  // other formats need dithering as it is
  default:
    return iDitheringType;
  }
}

#endif // SE1_D3D
