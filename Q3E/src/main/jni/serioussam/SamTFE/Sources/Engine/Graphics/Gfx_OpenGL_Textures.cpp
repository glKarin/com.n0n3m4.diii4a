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

#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Math/Functions.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Graphics/OpenGL.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/GfxProfile.h>

#include <Engine/Base/ListIterator.inl>

// asm shortcuts
#define O offset
#define Q qword ptr
#define D dword ptr
#define W  word ptr
#define B  byte ptr


// we need array for OpenGL mipmaps that are lower than N*1 or 1*N
static ULONG _aulLastMipmaps[(INDEX)(1024*1.334)];
static CTexParams *_tpCurrent;
extern INDEX GFX_iActiveTexUnit;

// unpacks texture filering from one INDEX to two GLenums (and eventually re-adjust input INDEX)
static void UnpackFilter_OGL( INDEX iFilter, GLenum &eMagFilter, GLenum &eMinFilter)
{
  switch( iFilter) {
  case 110:  case 10:  eMagFilter=GL_NEAREST;  eMinFilter=GL_NEAREST;                 break;
  case 111:  case 11:  eMagFilter=GL_NEAREST;  eMinFilter=GL_NEAREST_MIPMAP_NEAREST;  break;
  case 112:  case 12:  eMagFilter=GL_NEAREST;  eMinFilter=GL_NEAREST_MIPMAP_LINEAR;   break;
  case 220:  case 20:  eMagFilter=GL_LINEAR;   eMinFilter=GL_LINEAR;                  break;
  case 221:  case 21:  eMagFilter=GL_LINEAR;   eMinFilter=GL_LINEAR_MIPMAP_NEAREST;   break;
  case 222:  case 22:  eMagFilter=GL_LINEAR;   eMinFilter=GL_LINEAR_MIPMAP_LINEAR;    break;
  case 120:            eMagFilter=GL_NEAREST;  eMinFilter=GL_LINEAR;                  break;
  case 121:            eMagFilter=GL_NEAREST;  eMinFilter=GL_LINEAR_MIPMAP_NEAREST;   break;
  case 122:            eMagFilter=GL_NEAREST;  eMinFilter=GL_LINEAR_MIPMAP_LINEAR;    break;
  case 210:            eMagFilter=GL_LINEAR;   eMinFilter=GL_NEAREST;                 break;
  case 211:            eMagFilter=GL_LINEAR;   eMinFilter=GL_NEAREST_MIPMAP_NEAREST;  break;
  case 212:            eMagFilter=GL_LINEAR;   eMinFilter=GL_NEAREST_MIPMAP_LINEAR;   break;
  default: ASSERTALWAYS( "Illegal OpenGL texture filtering mode."); break;
  }
}


// change texture filtering mode if needed
extern void MimicTexParams_OGL( CTexParams &tpLocal)
{
  ASSERT( &tpLocal!=NULL);
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_TEXTUREPARAMS);

  // set texture filtering mode if required
  if( tpLocal.tp_iFilter != _tpGlobal[0].tp_iFilter)
  { // update OpenGL texture filters
    GLenum eMagFilter, eMinFilter;
    UnpackFilter_OGL( _tpGlobal[0].tp_iFilter, eMagFilter, eMinFilter);
    // adjust minimize filter in case of a single mipmap
    if( tpLocal.tp_bSingleMipmap) {
           if( eMinFilter==GL_NEAREST_MIPMAP_NEAREST || eMinFilter==GL_NEAREST_MIPMAP_LINEAR) eMinFilter = GL_NEAREST;
      else if( eMinFilter==GL_LINEAR_MIPMAP_NEAREST  || eMinFilter==GL_LINEAR_MIPMAP_LINEAR)  eMinFilter = GL_LINEAR;
    }
    // update texture filter
    pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, eMagFilter);
    pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, eMinFilter);
    tpLocal.tp_iFilter = _tpGlobal[0].tp_iFilter;
    OGL_CHECKERROR;
  }

  // set texture anisotropy degree if required and supported
  if( tpLocal.tp_iAnisotropy != _tpGlobal[0].tp_iAnisotropy) { 
    tpLocal.tp_iAnisotropy = _tpGlobal[0].tp_iAnisotropy;
    if( _pGfx->gl_iMaxTextureAnisotropy>=2) { // only if allowed
      pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, tpLocal.tp_iAnisotropy);
    }
  }

  // set texture clamping modes if changed
  if( tpLocal.tp_eWrapU!=_tpGlobal[GFX_iActiveTexUnit].tp_eWrapU
   || tpLocal.tp_eWrapV!=_tpGlobal[GFX_iActiveTexUnit].tp_eWrapV)
  { // prepare temp vars
    GLuint eWrapU = _tpGlobal[GFX_iActiveTexUnit].tp_eWrapU==GFX_REPEAT ? GL_REPEAT : GL_CLAMP;
    GLuint eWrapV = _tpGlobal[GFX_iActiveTexUnit].tp_eWrapV==GFX_REPEAT ? GL_REPEAT : GL_CLAMP;
    // eventually re-adjust clamping params in case of clamp_to_edge extension
    if( _pGfx->gl_ulFlags&GLF_EXT_EDGECLAMP) {
      if( eWrapU == GL_CLAMP) eWrapU = GL_CLAMP_TO_EDGE;
      if( eWrapV == GL_CLAMP) eWrapV = GL_CLAMP_TO_EDGE;
    } 
    // set clamping params and update local texture clamping modes
    pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, eWrapU);
    pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, eWrapV);
    tpLocal.tp_eWrapU = _tpGlobal[GFX_iActiveTexUnit].tp_eWrapU;
    tpLocal.tp_eWrapV = _tpGlobal[GFX_iActiveTexUnit].tp_eWrapV;
    OGL_CHECKERROR;
  }

  // keep last texture params (for tex upload and stuff)
  _tpCurrent = &tpLocal;
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_TEXTUREPARAMS);
}



// upload context for current texture to accelerator's memory
// (returns format in which texture was really uploaded)
__extern void UploadTexture_OGL( ULONG *pulTexture, PIX pixSizeU, PIX pixSizeV,
                               GLenum eInternalFormat, BOOL bUseSubImage)
{
#ifdef _GLES //karin: always GL_RGBA
	eInternalFormat = GL_RGBA;
#endif
  // safeties
  ASSERT( pulTexture!=NULL);
  ASSERT( pixSizeU>0 && pixSizeV>0);
  _sfStats.StartTimer( CStatForm::STI_BINDTEXTURE);
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_TEXTUREUPLOADING);

  // upload each original mip-map
  INDEX iMip=0;
  PIX pixOffset=0;
  while( pixSizeU>0 && pixSizeV>0)
  { 
    #if 0  // trust me, you'll know if it's not readable.  :)  This assert will read one past the start of the array if U or V is zero, so turning it off.  --ryan.
    // check that memory is readable
    ASSERT( pulTexture[pixOffset +pixSizeU*pixSizeV -1] != 0xDEADBEEF);
    #endif

    // upload mipmap as fast as possible
    if( bUseSubImage) {
      pglTexSubImage2D( GL_TEXTURE_2D, iMip, 0, 0, pixSizeU, pixSizeV,
                        GL_RGBA, GL_UNSIGNED_BYTE, pulTexture+pixOffset);
    } else {
      pglTexImage2D( GL_TEXTURE_2D, iMip, eInternalFormat, pixSizeU, pixSizeV, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pulTexture+pixOffset);
    } OGL_CHECKERROR;
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

      #if (defined __MSVC_INLINE__)
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

      #elif (defined __GNU_INLINE_X86_32__)
      __asm__ __volatile__ (
        "pxor      %%mm0,%%mm0                \n\t"
        "movl      %[pulSrc],%%esi            \n\t"
        "movl      %[pulDst],%%edi            \n\t"
        "movl      %[pixSize],%%ecx           \n\t"
        "0:                                   \n\t" // pixLoop
        "movd      0(%%esi), %%mm1            \n\t"
        "movd      4(%%esi), %%mm2            \n\t"
        "punpcklbw %%mm0,%%mm1                \n\t"
        "punpcklbw %%mm0,%%mm2                \n\t"
        "paddw     %%mm2,%%mm1                \n\t"
        "psrlw     $1,%%mm1                   \n\t"
        "packuswb  %%mm0,%%mm1                \n\t"
        "movd      %%mm1, (%%edi)             \n\t"
        "addl      $8,%%esi                   \n\t"
        "addl      $4,%%edi                   \n\t"
        "decl      %%ecx                      \n\t"
        "jnz       0b                         \n\t" // pixLoop
        "emms                                 \n\t"
            :
            : [pulSrc] "g" (pulSrc), [pulDst] "g" (pulDst),
              [pixSize] "g" (pixSize)
            : FPU_REGS, "mm0", "mm1", "mm2",
              "ecx", "esi", "edi", "memory", "cc"
      );

      #else
      // Basically average every other pixel...
      //UWORD w = 0;
      UBYTE *dptr = (UBYTE *) pulDst;
      UBYTE *sptr = (UBYTE *) pulSrc;
      #if 0
      pixSize *= 4;
      for (PIX i = 0; i < pixSize; i++)
      {
        *dptr = (UBYTE) ( (((UWORD) sptr[0]) + ((UWORD) sptr[1])) >> 1 );
        dptr++;
        sptr += 2;
      }
      #else
      for (PIX i = 0; i < pixSize; i++)
      {
        for (PIX j = 0; j < 4; j++)
        {
          *dptr = (UBYTE) ( (((UWORD) sptr[0]) + ((UWORD) sptr[4])) >> 1 );
          dptr++;
          sptr++;
        }
        sptr += 4;
      }
      #endif
      #endif

      // upload mipmap
      if( bUseSubImage) {
        pglTexSubImage2D( GL_TEXTURE_2D, iMip, 0, 0, pixSizeU, pixSizeV,
                          GL_RGBA, GL_UNSIGNED_BYTE, pulDst);
      } else {
        pglTexImage2D( GL_TEXTURE_2D, iMip, eInternalFormat, pixSizeU, pixSizeV, 0,
                       GL_RGBA, GL_UNSIGNED_BYTE, pulDst);
      } OGL_CHECKERROR;
      // advance to next mip-map
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



// returns bytes/pixels ratio for uploaded texture
extern INDEX GetFormatPixRatio_OGL( GLenum eFormat)
{
  switch( eFormat) {
  case GL_RGBA:
  case GL_RGBA8:
    return 4;
  case GL_RGB:
  case GL_RGB8:
    return 3;
  case GL_RGB5:
  case GL_RGB5_A1:
  case GL_RGB4:
  case GL_RGBA4:
  case GL_LUMINANCE_ALPHA:
  case GL_LUMINANCE8_ALPHA8:
    return 2;
  // compressed formats and single-channel formats
  default:
    return 1;
  }
}


// returns bytes/pixels ratio for uploaded texture
extern INDEX GetTexturePixRatio_OGL( GLuint uiBindNo)
{
#ifdef _GLES //karin: GL_RGBA
	GLenum eInternalFormat = GL_RGBA;
#else
  GLenum eInternalFormat;
  pglBindTexture( GL_TEXTURE_2D, uiBindNo);
  pglGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, (GLint*)&eInternalFormat);
  OGL_CHECKERROR;
#endif
  return GetFormatPixRatio_OGL( eInternalFormat);
}


// return allowed dithering method
extern INDEX AdjustDitheringType_OGL( GLenum eFormat, INDEX iDitheringType)
{
  switch( eFormat) {
  // these formats don't need dithering
  case GL_RGB8:
  case GL_RGBA8:
  case GL_LUMINANCE8:
  case GL_LUMINANCE8_ALPHA8:
    return NONE;
  // these formats need reduced dithering
  case GL_RGB5:
  case GL_RGB5_A1:
    if( iDitheringType>7) return iDitheringType-3;
  // other formats need dithering as it is
  default:
    return iDitheringType;
  }
}
