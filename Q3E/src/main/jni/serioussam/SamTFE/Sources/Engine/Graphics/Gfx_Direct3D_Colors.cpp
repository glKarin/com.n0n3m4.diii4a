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

// asm shortcuts
#define O offset
#define Q qword ptr
#define D dword ptr
#define W  word ptr
#define B  byte ptr


static ULONG _ulAlphaMask = 0;
static void (*pConvertMipmap)( ULONG *pulSrc, void *pulDst, PIX pixWidth, PIX pixHeight, SLONG slPitch);



// convert to any D3DFormat (thru D3DX functions - slow!)
static void ConvertAny( ULONG *pulSrc, LPDIRECT3DTEXTURE8 ptexDst, PIX pixWidth, PIX pixHeight, INDEX iMip)
{
  // alloc temporary memory and flip colors there
  const PIX pixSize = pixWidth*pixHeight;
  ULONG *pulFlipped = (ULONG*)AllocMemory( pixSize*4);
  abgr2argb( pulSrc, pulFlipped, pixSize);

  // get mipmap surface
  HRESULT hr;
  LPDIRECT3DSURFACE8 pd3dSurf;
  hr = ptexDst->GetSurfaceLevel( iMip, &pd3dSurf);
  D3D_CHECKERROR(hr);

  // prepare and upload surface
  const RECT rect = { 0,0, pixWidth, pixHeight };
  const SLONG slPitch = pixWidth*4;
  hr = D3DXLoadSurfaceFromMemory( pd3dSurf, NULL, NULL, pulFlipped, D3DFMT_A8R8G8B8, slPitch, NULL, &rect, D3DX_FILTER_NONE, 0);
  D3D_CHECKERROR(hr);

  // done
  pd3dSurf->Release();  // must not use D3DRELEASE, because freeing all istances will free texture also when using DXTC!? (Bravo MS!)
  FreeMemory( pulFlipped);
}



// fast conversion from RGBA memory format to one of D3D color formats


static void ConvARGB8( ULONG *pulSrc, void *pulDst, PIX pixWidth, PIX pixHeight, SLONG slPitch)
{
  const ULONG slRowModulo = slPitch - (pixWidth<<2);
  __asm {
    mov     esi,D [pulSrc]
    mov     edi,D [pulDst]
    mov     ebx,D [pixHeight]
rowLoop:
    mov     ecx,D [pixWidth]
pixLoop:
    mov     eax,D [esi]
    or      eax,D [_ulAlphaMask]
    bswap   eax
    ror     eax,8
    mov     D [edi],eax
    add     esi,4
    add     edi,4
    dec     ecx
    jnz     pixLoop
    add     edi,D [slRowModulo]
    dec     ebx
    jnz     rowLoop
  }
}


static void ConvARGB5( ULONG *pulSrc, void *pulDst, PIX pixWidth, PIX pixHeight, SLONG slPitch)
{
  const ULONG slRowModulo = slPitch - (pixWidth<<1);
  __asm {
    mov     esi,D [pulSrc]
    mov     edi,D [pulDst]
    mov     ebx,D [pixHeight]
rowLoop:
    movd    mm0,ebx
    mov     ecx,D [pixWidth]
pixLoop:
    movd    mm1,ecx
    mov     eax,D [esi]
    or      eax,D [_ulAlphaMask]
    mov     ebx,eax
    mov     ecx,eax
    mov     edx,eax
    and     eax,0x000000F8 // R
    and     ebx,0x0000F800 // G
    and     ecx,0x00F80000 // B
    and     edx,0x80000000 // A
    shl     eax,7
    shr     ebx,6
    shr     ecx,16+3
    shr     edx,16
    or      eax,ebx
    or      ecx,edx
    or      eax,ecx
    mov     W [edi],ax
    add     esi,4
    add     edi,2
    movd    ecx,mm1
    dec     ecx
    jnz     pixLoop
    add     edi,D [slRowModulo]
    movd    ebx,mm0
    dec     ebx
    jnz     rowLoop
    emms
  }
}


static void ConvARGB4( ULONG *pulSrc, void *pulDst, PIX pixWidth, PIX pixHeight, SLONG slPitch)
{
  const ULONG slRowModulo = slPitch - (pixWidth<<1);
  __asm {
    mov     esi,D [pulSrc]
    mov     edi,D [pulDst]
    mov     ebx,D [pixHeight]
rowLoop:
    movd    mm0,ebx
    mov     ecx,D [pixWidth]
pixLoop:
    movd    mm1,ecx
    mov     eax,D [esi]
    or      eax,D [_ulAlphaMask]
    mov     ebx,eax
    mov     ecx,eax
    mov     edx,eax
    and     eax,0x000000F0 // R
    and     ebx,0x0000F000 // G
    and     ecx,0x00F00000 // B
    and     edx,0xF0000000 // A
    shl     eax,4
    shr     ebx,8
    shr     ecx,16+4
    shr     edx,16
    or      eax,ebx
    or      ecx,edx
    or      eax,ecx
    mov     W [edi],ax
    add     esi,4
    add     edi,2
    movd    ecx,mm1
    dec     ecx
    jnz     pixLoop
    add     edi,D [slRowModulo]
    movd    ebx,mm0
    dec     ebx
    jnz     rowLoop
    emms
  }
}


static void ConvRGB5( ULONG *pulSrc, void *pulDst, PIX pixWidth, PIX pixHeight, SLONG slPitch)
{
  const ULONG slRowModulo = slPitch - (pixWidth<<1);
  __asm {
    mov     esi,D [pulSrc]
    mov     edi,D [pulDst]
    mov     ebx,D [pixHeight]
rowLoop:
    movd    mm0,ebx
    mov     ecx,D [pixWidth]
pixLoop:
    mov     eax,D [esi]
    mov     ebx,eax
    mov     edx,eax
    and     eax,0x000000F8 // R
    and     ebx,0x0000FC00 // G
    and     edx,0x00F80000 // B
    shl     eax,8
    shr     ebx,5
    shr     edx,8+5+6
    or      eax,ebx
    or      eax,edx
    mov     W [edi],ax
    add     esi,4
    add     edi,2
    dec     ecx
    jnz     pixLoop
    add     edi,D [slRowModulo]
    movd    ebx,mm0
    dec     ebx
    jnz     rowLoop
    emms
  }
}


static void ConvAL8( ULONG *pulSrc, void *pulDst, PIX pixWidth, PIX pixHeight, SLONG slPitch)
{
  const ULONG slRowModulo = slPitch - (pixWidth<<1);
  __asm {
    mov     esi,D [pulSrc]
    mov     edi,D [pulDst]
    mov     ebx,D [pixHeight]
rowLoop:
    mov     ecx,D [pixWidth]
pixLoop:
    mov     eax,D [esi]
    ror     eax,8
    bswap   eax
    mov     D [edi],eax
    add     esi,4
    add     edi,2
    dec     ecx
    jnz     pixLoop
    add     edi,D [slRowModulo]
    dec     ebx
    jnz     rowLoop
  }
}



static void ConvL8( ULONG *pulSrc, void *pulDst, PIX pixWidth, PIX pixHeight, SLONG slPitch)
{
  const ULONG slRowModulo = slPitch - (pixWidth<<0);
  __asm {
    mov     esi,D [pulSrc]
    mov     edi,D [pulDst]
    mov     ebx,D [pixHeight]
rowLoop:
    mov     ecx,D [pixWidth]
pixLoop:
    mov     eax,D [esi]
    mov     B [edi],al
    add     esi,4
    add     edi,1
    dec     ecx
    jnz     pixLoop
    add     edi,D [slRowModulo]
    dec     ebx
    jnz     rowLoop
  }
}




// set color conversion routine
extern void SetInternalFormat_D3D( D3DFORMAT d3dFormat)
{
  // by default, go thru D3DX :(
  pConvertMipmap = NULL; 
  extern INDEX d3d_bFastUpload;
  if( !d3d_bFastUpload) return; // done here if fast upload is not allowed

  // try to set corresponding fast-upload routine
  switch( d3dFormat) {
  case D3DFMT_A8R8G8B8: _ulAlphaMask=0x00000000;  pConvertMipmap=ConvARGB8;  break;
  case D3DFMT_X8R8G8B8: _ulAlphaMask=0xFF000000;  pConvertMipmap=ConvARGB8;  break;
  case D3DFMT_A1R5G5B5: _ulAlphaMask=0x00000000;  pConvertMipmap=ConvARGB5;  break;
  case D3DFMT_X1R5G5B5: _ulAlphaMask=0xFF000000;  pConvertMipmap=ConvARGB5;  break;
  case D3DFMT_A4R4G4B4: _ulAlphaMask=0x00000000;  pConvertMipmap=ConvARGB4;  break;
  case D3DFMT_X4R4G4B4: _ulAlphaMask=0xFF000000;  pConvertMipmap=ConvARGB4;  break;
  case D3DFMT_R5G6B5: pConvertMipmap=ConvRGB5;  break;
  case D3DFMT_A8L8:   pConvertMipmap=ConvAL8;   break;
  case D3DFMT_L8:     pConvertMipmap=ConvL8;    break;
  default:  break; 
  }
}


// convert one mipmap from memory to surface
extern void UploadMipmap_D3D( ULONG *pulSrc, LPDIRECT3DTEXTURE8 ptexDst, PIX pixWidth, PIX pixHeight, INDEX iMip)
{
  // general case thru D3DX approach
  if( pConvertMipmap==NULL) {
    ConvertAny( pulSrc, ptexDst, pixWidth, pixHeight, iMip);
    return;
  }
  // yeah! - optimized case :)
  HRESULT hr;
  D3DLOCKED_RECT rectLocked;
  hr = ptexDst->LockRect( iMip, &rectLocked, NULL, NONE);
  D3D_CHECKERROR(hr);
  pConvertMipmap( pulSrc, rectLocked.pBits, pixWidth, pixHeight, rectLocked.Pitch);
  hr = ptexDst->UnlockRect(iMip);
  D3D_CHECKERROR(hr);
}


// unpack from some of D3D color formats to COLOR
extern COLOR UnpackColor_D3D( UBYTE *pd3dColor, D3DFORMAT d3dFormat, SLONG &slColorSize)
{
  UWORD uw;
  UBYTE ubR,ubG,ubB;
  switch(d3dFormat) {
  case D3DFMT_R8G8B8:
  case D3DFMT_X8R8G8B8:
  case D3DFMT_A8R8G8B8:
    ubB = pd3dColor[0];
    ubG = pd3dColor[1];
    ubR = pd3dColor[2];
    slColorSize = 4;
    if( d3dFormat==D3DFMT_R8G8B8) slColorSize = 3;
    break; 
  case D3DFMT_R5G6B5:
    uw  = (UWORD&)*pd3dColor;
    ubR = (uw&0xF800)>>8;  ubR |= ubR>>5;
    ubG = (uw&0x07E0)>>3;  ubG |= ubG>>6;
    ubB = (uw&0x001F)<<3;  ubB |= ubB>>5;
    slColorSize = 2;
    break;
  case D3DFMT_X1R5G5B5:
  case D3DFMT_A1R5G5B5:
    uw  = (UWORD&)*pd3dColor;
    ubR = (uw&0x7C00)>>7;  ubR |= ubR>>5;
    ubG = (uw&0x03E0)>>2;  ubG |= ubG>>5;
    ubB = (uw&0x001F)<<3;  ubB |= ubB>>5;
    slColorSize = 2;
    break;
  case D3DFMT_X4R4G4B4:
  case D3DFMT_A4R4G4B4:
    uw  = (UWORD&)*pd3dColor;
    ubR = (uw&0x0F00)>>4;  ubR |= ubR>>4;
    ubG = (uw&0x00F0)>>0;  ubG |= ubG>>4;
    ubB = (uw&0x000F)<<4;  ubB |= ubB>>4;
    slColorSize = 2;
    break;
  default: // unsupported format
    ubR = ubG = ubB = 0;
    slColorSize = 0;
    break;
  }
  // done
  return RGBToColor(ubR,ubG,ubB);
}

#endif // SE1_D3D
