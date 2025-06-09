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

#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/RenderPoly.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/GfxProfile.h>

#if USE_MMX_INTRINSICS
#include <mmintrin.h>
#endif

// asm shortcuts
#define O offset
#define Q qword ptr
#define D dword ptr
#define W  word ptr
#define B  byte ptr

extern INDEX tex_bProgressiveFilter; // filter mipmaps in creation time (not afterwards)


// returns number of mip-maps to skip from original texture
INDEX ClampTextureSize( PIX pixClampSize, PIX pixClampDimension, PIX pixSizeU, PIX pixSizeV)
{
  __int64 pixMaxSize  = (__int64)pixSizeU * (__int64)pixSizeV;
  PIX pixMaxDimension = Max( pixSizeU, pixSizeV);
  INDEX ctSkipMips    = 0;
  while( (pixMaxSize>pixClampSize || pixMaxDimension>pixClampDimension) && pixMaxDimension>1) {
    ctSkipMips++;
    pixMaxDimension >>=1;
    pixMaxSize >>=2;
  }
  return ctSkipMips;
}


// retrives memory offset of a specified mip-map or a size of all mip-maps (IN PIXELS!)
// (zero offset means first, i.e. largest mip-map)
PIX GetMipmapOffset( INDEX iMipLevel, PIX pixWidth, PIX pixHeight)
{
  PIX pixTexSize = 0;
  PIX pixMipSize = pixWidth*pixHeight;
  INDEX iMips = GetNoOfMipmaps( pixWidth, pixHeight);
  iMips = Min( iMips, iMipLevel);
  while( iMips>0) {
    pixTexSize +=pixMipSize;
    pixMipSize>>=2;
    iMips--;
  }
  return pixTexSize;
}


// return offset, pointer and dimensions of mipmap of specified size inside texture or shadowmap mipmaps
INDEX GetMipmapOfSize( PIX pixWantedSize, ULONG *&pulFrame, PIX &pixWidth, PIX &pixHeight)
{
  INDEX iMipOffset = 0;
  while( pixWidth>1 && pixHeight>1) {
    const PIX pixCurrentSize = pixWidth*pixHeight;
    if( pixCurrentSize <= pixWantedSize) break; // found
    pulFrame += pixCurrentSize;
    pixWidth >>=1;
    pixHeight>>=1;
    iMipOffset++;
  } // done
  return iMipOffset;
}


// adds 8-bit opaque alpha channel to 24-bit bitmap (in place supported)
void AddAlphaChannel( UBYTE *pubSrcBitmap, ULONG *pulDstBitmap, PIX pixSize, UBYTE *pubAlphaBitmap)
{
  UBYTE ubR,ubG,ubB, ubA=255;
  // loop backwards thru all bitmap pixels
  for( INDEX iPix=(pixSize-1); iPix>=0; iPix--) {
    ubR = pubSrcBitmap[iPix*3 +0];
    ubG = pubSrcBitmap[iPix*3 +1];
    ubB = pubSrcBitmap[iPix*3 +2];
    if( pubAlphaBitmap!=NULL) ubA = pubAlphaBitmap[iPix];
    else ubA = 255; // for the sake of forced RGBA internal formats!
    pulDstBitmap[iPix] = ByteSwap( RGBAToColor( ubR,ubG,ubB, ubA));
  }
}

// removes 8-bit alpha channel from 32-bit bitmap (in place supported)
void RemoveAlphaChannel( ULONG *pulSrcBitmap, UBYTE *pubDstBitmap, PIX pixSize)
{
  UBYTE ubR,ubG,ubB;
  // loop thru all bitmap pixels
  for( INDEX iPix=0; iPix<pixSize; iPix++) {
    ColorToRGB( ByteSwap( pulSrcBitmap[iPix]), ubR,ubG,ubB);
    pubDstBitmap[iPix*3 +0] = ubR;
    pubDstBitmap[iPix*3 +1] = ubG;
    pubDstBitmap[iPix*3 +2] = ubB;
  }
}



// flips 24 or 32-bit bitmap (iType: 1-horizontal, 2-vertical, 3-diagonal) - in place supported
void FlipBitmap( UBYTE *pubSrc, UBYTE *pubDst, PIX pixWidth, PIX pixHeight, INDEX iFlipType, BOOL bAlphaChannel)
{
  // safety
  ASSERT( iFlipType>=0 && iFlipType<4);
  // no flipping ?
  PIX pixSize = pixWidth*pixHeight;
  if( iFlipType==0) {
    // copy bitmap only if needed
    INDEX ctBPP = (bAlphaChannel ? 4 : 3);
    if( pubSrc!=pubDst) memcpy( pubDst, pubSrc, pixSize*ctBPP);
    return;
  }

  // prepare images without alpha channels
  ULONG *pulNew = NULL;
  ULONG *pulNewSrc = (ULONG*)pubSrc;
  ULONG *pulNewDst = (ULONG*)pubDst;
  if( !bAlphaChannel) {
    pulNew = (ULONG*)AllocMemory( pixSize *BYTES_PER_TEXEL);
    AddAlphaChannel( pubSrc, pulNew, pixSize);
    pulNewSrc = pulNew;
    pulNewDst = pulNew;
  }

  // prepare half-width and half-height rounded
  const PIX pixHalfWidth  = (pixWidth+1) /2;
  const PIX pixHalfHeight = (pixHeight+1)/2;

  // flip horizontal
  if( iFlipType==2 || iFlipType==3)
  { // for each row
    for( INDEX iRow=0; iRow<pixHeight; iRow++)
    { // find row pointer
      PIX pixRowOffset = iRow*pixWidth;
      // for each pixel in row
      for( INDEX iPix=0; iPix<pixHalfWidth; iPix++)
      { // transfer pixels
        PIX pixBeg = pulNewSrc[pixRowOffset+iPix];
        PIX pixEnd = pulNewSrc[pixRowOffset+(pixWidth-1-iPix)];
        pulNewDst[pixRowOffset+iPix]              = pixEnd;
        pulNewDst[pixRowOffset+(pixWidth-1-iPix)] = pixBeg;
      }
    }
  }

  // prepare new pointers
  if( iFlipType==3) pulNewSrc = pulNewDst;

  // flip vertical/diagonal
  if( iFlipType==1 || iFlipType==3)
  { // for each row
    for( INDEX iRow=0; iRow<pixHalfHeight; iRow++)
    { // find row pointers
      PIX pixBegOffset = iRow*pixWidth;
      PIX pixEndOffset = (pixHeight-1-iRow)*pixWidth;
      // for each pixel in row
      for( INDEX iPix=0; iPix<pixWidth; iPix++)
      { // transfer pixels
        PIX pixBeg = pulNewSrc[pixBegOffset+iPix];
        PIX pixEnd = pulNewSrc[pixEndOffset+iPix];
        pulNewDst[pixBegOffset+iPix] = pixEnd;
        pulNewDst[pixEndOffset+iPix] = pixBeg;
      }
    }
  }

  // postpare images without alpha channels
  if( !bAlphaChannel) {
    RemoveAlphaChannel( pulNewDst, pubDst, pixSize);
    if( pulNew!=NULL) FreeMemory(pulNew);
  }
}



// makes one level lower mipmap (bilinear or nearest-neighbour with border preservance)
#if (defined __GNUC__)
__int64 mmRounder = 0x0002000200020002ll;
#else
__int64 mmRounder = 0x0002000200020002;
#endif

static void MakeOneMipmap( ULONG *pulSrcMipmap, ULONG *pulDstMipmap, PIX pixWidth, PIX pixHeight, BOOL bBilinear)
{
  // some safety checks
  ASSERT( pixWidth>1 && pixHeight>1);
  ASSERT( pixWidth  == 1L<<FastLog2(pixWidth));
  ASSERT( pixHeight == 1L<<FastLog2(pixHeight));
  pixWidth >>=1;
  pixHeight>>=1;

  if( bBilinear) // type of filtering?
  { // BILINEAR

   #if (defined __MSVC_INLINE__)
    __asm {
      pxor    mm0,mm0
      mov     ebx,D [pixWidth]
      mov     esi,D [pulSrcMipmap]
      mov     edi,D [pulDstMipmap]
      mov     edx,D [pixHeight]
rowLoop:
      mov     ecx,D [pixWidth]
pixLoopN:           
      movd    mm1,D [esi+ 0]        // up-left
      movd    mm2,D [esi+ 4]        // up-right
      movd    mm3,D [esi+ ebx*8 +0] // down-left
      movd    mm4,D [esi+ ebx*8 +4] // down-right
      punpcklbw mm1,mm0
      punpcklbw mm2,mm0
      punpcklbw mm3,mm0
      punpcklbw mm4,mm0
      paddw   mm1,mm2
      paddw   mm1,mm3
      paddw   mm1,mm4
      paddw   mm1,Q [mmRounder]
      psrlw   mm1,2
      packuswb mm1,mm0
      movd    D [edi],mm1
      // advance to next pixel
      add     esi,4*2
      add     edi,4
      dec     ecx
      jnz     pixLoopN
      // advance to next row
      lea     esi,[esi+ ebx*8] // skip one row in source mip-map
      dec     edx
      jnz     rowLoop
      emms
    }

   #elif (defined __GNU_INLINE_X86_32__)
    __asm__ __volatile__ (
      "pxor    %%mm0, %%mm0                 \n\t"
      "movl    %[pulSrcMipmap], %%esi       \n\t"
      "movl    %[pulDstMipmap], %%edi       \n\t"
      "movl    %[pixHeight], %%edx          \n\t"

      "0:                                   \n\t"  // rowLoop
      "movl    %[pixWidth], %%ecx           \n\t"

      "1:                                   \n\t"  // pixLoopN
      "movd      0(%%esi), %%mm1            \n\t"  // up-left
      "movd      4(%%esi), %%mm2            \n\t"  // up-right
      "movd      0(%%esi, %[pixWidth], 8), %%mm3 \n\t" // down-left
      "movd      4(%%esi, %[pixWidth], 8), %%mm4 \n\t" // down-right
      "punpcklbw %%mm0, %%mm1               \n\t"
      "punpcklbw %%mm0, %%mm2               \n\t"
      "punpcklbw %%mm0, %%mm3               \n\t"
      "punpcklbw %%mm0, %%mm4               \n\t"
      "paddw     %%mm2, %%mm1               \n\t"
      "paddw     %%mm3, %%mm1               \n\t"
      "paddw     %%mm4, %%mm1               \n\t"
      "paddw     (" ASMSYM(mmRounder) "), %%mm1 \n\t"
      "psrlw     $2, %%mm1                  \n\t"
      "packuswb  %%mm0, %%mm1               \n\t"
      "movd      %%mm1, (%%edi)             \n\t"

      // advance to next pixel
      "addl     $8, %%esi                   \n\t"
      "addl     $4, %%edi                   \n\t"
      "decl     %%ecx                       \n\t"
      "jnz      1b                          \n\t"  // pixLoopN

      // advance to next row
      // skip one row in source mip-map
      "leal     0(%%esi, %[pixWidth], 8), %%esi \n\t"
      "decl     %%edx                       \n\t"
      "jnz      0b                          \n\t"  // rowLoop
      "emms                                 \n\t"
          : // no outputs.
          : [pixWidth] "r" (pixWidth),
            [pulSrcMipmap] "g" (pulSrcMipmap),
            [pulDstMipmap] "g" (pulDstMipmap),
            [pixHeight] "g" (pixHeight)
          : FPU_REGS, MMX_REGS, "ecx", "edx", "esi", "edi",
            "cc", "memory"
    );

   #else
	UBYTE *src = (UBYTE *) pulSrcMipmap;
	UBYTE *dest = (UBYTE *) pulDstMipmap;
	for (int i = 0 ; i < pixHeight; i++)
	{
		for (int j = 0; j < pixWidth; j++)
		{
			// Grab pixels from image
			UWORD upleft[4];
			UWORD upright[4];
			UWORD downleft[4];
			UWORD downright[4];
			upleft[0] = *(src + 0);
			upleft[1] = *(src + 1);
			upleft[2] = *(src + 2);
			upleft[3] = *(src + 3);
			upright[0] = *(src + 4);
			upright[1] = *(src + 5);
			upright[2] = *(src + 6);
			upright[3] = *(src + 7);

			downleft[0] = *(src + pixWidth*8 + 0);
			downleft[1] = *(src + pixWidth*8 + 1);
			downleft[2] = *(src + pixWidth*8 + 2);
			downleft[3] = *(src + pixWidth*8 + 3);
			downright[0] = *(src + pixWidth*8 + 4);
			downright[1] = *(src + pixWidth*8 + 5);
			downright[2] = *(src + pixWidth*8 + 6);
			downright[3] = *(src + pixWidth*8 + 7);

			UWORD answer[4];
			answer[0] = upleft[0] + upright[0] + downleft[0] + downright[0] + 2;
			answer[1] = upleft[1] + upright[1] + downleft[1] + downright[1] + 2;
			answer[2] = upleft[2] + upright[2] + downleft[2] + downright[2] + 2;
			answer[3] = upleft[3] + upright[3] + downleft[3] + downright[3] + 2;
			answer[0] /= 4;
			answer[1] /= 4;
			answer[2] /= 4;
			answer[3] /= 4;

			*(dest + 0) = answer[0];
			*(dest + 1) = answer[1];
			*(dest + 2) = answer[2];
			*(dest + 3) = answer[3];

			src += 8;
			dest += 4;
		}
		src += 8*pixWidth;
    }

   #endif
    }
    else
    { // NEAREST-NEIGHBOUR but with border preserving
       ULONG ulRowModulo = pixWidth*2 *BYTES_PER_TEXEL;

   #if (defined __MSVC_INLINE__)
    __asm {
      xor     ebx,ebx
      mov     esi,D [pulSrcMipmap]
      mov     edi,D [pulDstMipmap]
      // setup upper half
      mov     edx,D [pixHeight]
      shr     edx,1
halfLoop:
      mov     ecx,D [pixWidth]
      shr     ecx,1
leftLoop:
      mov     eax,D [esi+ ebx*8+ 0] // upper-left (or lower-left)
      mov     D [edi],eax
      // advance to next pixel
      add     esi,4*2
      add     edi,4
      sub     ecx,1
      jg      leftLoop
      // do right row half
      mov     ecx,D [pixWidth]
      shr     ecx,1
      jz      halfEnd
rightLoop:
      mov     eax,D [esi+ ebx*8+ 4] // upper-right (or lower-right)
      mov     D [edi],eax
      // advance to next pixel
      add     esi,4*2
      add     edi,4
      sub     ecx,1
      jg      rightLoop
halfEnd:
      // advance to next row
      add     esi,D [ulRowModulo]  // skip one row in source mip-map
      sub     edx,1
      jg      halfLoop
      // do eventual lower half loop (if not yet done)
      mov     edx,D [pixHeight]
      shr     edx,1
      jz      fullEnd
      cmp     ebx,D [pixWidth]
      mov     ebx,D [pixWidth]
      jne     halfLoop
fullEnd:
    }

   #elif (defined __GNU_INLINE_X86_32__)
    ULONG tmp, tmp2;
    __asm__ __volatile__ (
      "xorl     %[xbx], %[xbx]             \n\t"
      "movl     %[pulSrcMipmap], %%esi     \n\t"
      "movl     %[pulDstMipmap], %%edi     \n\t"
      // setup upper half
      "movl     %[pixHeight], %%eax        \n\t"
      "movl     %%eax, %[xdx]              \n\t"
      "shrl     $1, %[xdx]                 \n\t"

      "0:                                  \n\t" // halfLoop
      "movl     %[pixWidth], %%ecx         \n\t"
      "shrl     $1, %%ecx                  \n\t"

      "1:                                  \n\t" // leftLoop
      "movl     0(%%esi, %[xbx], 8), %%eax \n\t" // upper-left (or lower-left)
      "movl     %%eax, (%%edi)             \n\t"

      // advance to next pixel
      "addl     $8, %%esi                  \n\t"
      "addl     $4, %%edi                  \n\t"
      "subl     $1, %%ecx                  \n\t"
      "jg       1b                         \n\t" // leftLoop

      // do right row half
      "movl     %[pixWidth], %%ecx         \n\t"
      "shrl     $1, %%ecx                  \n\t"
      "jz       3f                         \n\t" // halfEnd

      "2:                                  \n\t" // rightLoop
      "movl     4(%%esi, %[xbx], 8), %%eax \n\t" // upper-right (or lower-right)
      "movl     %%eax, (%%edi)             \n\t"

      // advance to next pixel
      "addl     $8, %%esi                  \n\t"
      "addl     $4, %%edi                  \n\t"
      "subl     $1, %%ecx                  \n\t"
      "jg       2b                         \n\t" // rightLoop

      "3:                                  \n\t" // halfEnd
      // advance to next row
      "addl     %[ulRowModulo], %%esi      \n\t" // skip one row in source mip-map
      "subl     $1, %[xdx]                 \n\t"
      "jg       0b                         \n\t" // halfLoop

      // do eventual lower half loop (if not yet done)
      "movl     %[pixHeight], %%eax        \n\t"
      "movl     %%eax, %[xdx]              \n\t"
      "shrl     $1, %[xdx]                 \n\t"
      "jz       4f                         \n\t" // fullEnd
      "cmpl     %[pixWidth], %[xbx]        \n\t"
      "movl     %[pixWidth], %[xbx]        \n\t"
      "jne      0b                         \n\t" // halfLoop

      "4:                                  \n\t" // fullEnd
          : [xbx] "=&r" (tmp), [xdx] "=&g" (tmp2)
          : [pulSrcMipmap] "g" (pulSrcMipmap),
            [pulDstMipmap] "g" (pulDstMipmap),
            [pixHeight] "g" (pixHeight), [pixWidth] "g" (pixWidth),
            [ulRowModulo] "g" (ulRowModulo)
          : "eax", "ecx", "esi", "edi", "cc", "memory"
    );

   #else
     PIX offset = 0;
     ulRowModulo /= 4;

     for (int q = 0; q < 2; q++)
     {
         for (PIX i = pixHeight / 2; i > 0; i--)
         {
             for (PIX j = pixWidth / 2; j > 0; j--)
             {
                 *pulDstMipmap = *(pulSrcMipmap + offset);
                 pulSrcMipmap += 2;
                 pulDstMipmap++;
             }

             for (PIX j = pixWidth / 2; j > 0; j--)
             {
                 *pulDstMipmap = *(pulSrcMipmap + offset + 1);
                 pulSrcMipmap += 2;
                 pulDstMipmap++;
             }

             pulSrcMipmap += ulRowModulo;
        }

        offset = pixWidth * 2;
     }

   #endif
  }
}


// makes ALL lower mipmaps (to size of 1x1!) of a specified 32-bit bitmap
// and returns pointer to newely created and mipmaped image
// (only first ctFineMips number of mip-maps will be filtered with bilinear subsampling, while
//  all others will be downsampled with nearest-neighbour method)
void MakeMipmaps( INDEX ctFineMips, ULONG *pulMipmaps, PIX pixWidth, PIX pixHeight, INDEX iFilter/*=NONE*/)
{
  ASSERT( pixWidth>0 && pixHeight>0);
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_MAKEMIPMAPS);

  // prepare some variables
  INDEX ctMipmaps = 1;
  PIX pixTexSize  = 0;
  PIX pixCurrWidth  = pixWidth;
  PIX pixCurrHeight = pixHeight;
  ULONG *pulSrcMipmap, *pulDstMipmap;

  // determine filtering mode (-1=prefiltering, 0=none, 1=postfiltering)
  INDEX iFilterMode = 0;
  if( iFilter!=0) {
    iFilterMode = -1;
    if( !tex_bProgressiveFilter) iFilterMode = +1;
  }

  // loop thru mip-map levels
  while( pixCurrWidth>1 && pixCurrHeight>1)
  { // determine mip size
    PIX pixMipSize = pixCurrWidth*pixCurrHeight;
    pulSrcMipmap   = pulMipmaps   + pixTexSize;
    pulDstMipmap   = pulSrcMipmap + pixMipSize;
    // do pre filter is required
    if( iFilterMode<0) FilterBitmap( iFilter, pulSrcMipmap, pulSrcMipmap, pixCurrWidth, pixCurrHeight);
    // create one mipmap
    MakeOneMipmap( pulSrcMipmap, pulDstMipmap, pixCurrWidth, pixCurrHeight, ctMipmaps<ctFineMips);
    // do post filter if required
    if( iFilterMode>0) FilterBitmap( iFilter, pulSrcMipmap, pulSrcMipmap, pixCurrWidth, pixCurrHeight);
    // advance to next mipmap
    pixTexSize += pixMipSize;
    pixCurrWidth  >>=1;
    pixCurrHeight >>=1;
    ctMipmaps++;
  }
  // all done
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_MAKEMIPMAPS);
}


// mipmap colorization table (from 1024 to 1)
static COLOR _acolMips[10] = { C_RED, C_GREEN, C_BLUE, C_CYAN, C_MAGENTA, C_YELLOW, C_RED, C_GREEN, C_BLUE, C_WHITE };

// colorize mipmaps
void ColorizeMipmaps( INDEX i1stMipmapToColorize, ULONG *pulMipmaps, PIX pixWidth, PIX pixHeight)
{
  // prepare ...
  ULONG *pulSrcMipmap = pulMipmaps + GetMipmapOffset( i1stMipmapToColorize, pixWidth, pixHeight);
  //ULONG *pulDstMipmap;
  PIX pixCurrWidth  = pixWidth >>i1stMipmapToColorize;
  PIX pixCurrHeight = pixHeight>>i1stMipmapToColorize;
  PIX pixMipSize;
  // skip too large textures
  const PIX pixMaxDim = Max( pixCurrWidth, pixCurrHeight);
  if( pixMaxDim>1024) return;
  INDEX iTableOfs = 10-FastLog2(pixMaxDim);

  // loop thru mip-map levels
  while( pixCurrWidth>1 && pixCurrHeight>1)
  { // prepare current mip-level
    pixMipSize   = pixCurrWidth*pixCurrHeight;
    //pulDstMipmap = pulSrcMipmap + pixMipSize;
    // mask mipmap
    const ULONG ulColorMask = ByteSwap( _acolMips[iTableOfs] | 0x3F3F3FFF);
    for( INDEX iPix=0; iPix<pixMipSize; iPix++) pulSrcMipmap[iPix] &= ulColorMask;
    // advance to next mipmap
    pulSrcMipmap += pixMipSize;
    pixCurrWidth  >>=1;
    pixCurrHeight >>=1;
    iTableOfs++;
  }
}



// calculates standard deviation of a bitmap
DOUBLE CalcBitmapDeviation( ULONG *pulBitmap, PIX pixSize)
{
  UBYTE ubR,ubG,ubB;
  ULONG ulSumR =0, ulSumG =0, ulSumB =0;
__int64 mmSumR2=0, mmSumG2=0, mmSumB2=0;

  // calculate sum and sum^2
  for( INDEX iPix=0; iPix<pixSize; iPix++) {
    ColorToRGB( ByteSwap(pulBitmap[iPix]), ubR,ubG,ubB);
    ulSumR  += ubR;      ulSumG  += ubG;      ulSumB  += ubB;
    mmSumR2 += ubR*ubR;  mmSumG2 += ubG*ubG;  mmSumB2 += ubB*ubB;
  }

  // calculate deviation of each channel
  DOUBLE d1oSize   = 1.0 / (DOUBLE) pixSize;
  DOUBLE d1oSizeM1 = 1.0 / (DOUBLE)(pixSize-1);
  DOUBLE dAvgR = (DOUBLE)ulSumR *d1oSize;
  DOUBLE dAvgG = (DOUBLE)ulSumG *d1oSize;
  DOUBLE dAvgB = (DOUBLE)ulSumB *d1oSize;
  DOUBLE dDevR = Sqrt( ((DOUBLE)mmSumR2 - 2*ulSumR*dAvgR + pixSize*dAvgR*dAvgR) *d1oSizeM1);
  DOUBLE dDevG = Sqrt( ((DOUBLE)mmSumG2 - 2*ulSumG*dAvgG + pixSize*dAvgG*dAvgG) *d1oSizeM1);
  DOUBLE dDevB = Sqrt( ((DOUBLE)mmSumB2 - 2*ulSumB*dAvgB + pixSize*dAvgB*dAvgB) *d1oSizeM1);

  // return maximum deviation
  return Max( Max( dDevR, dDevG), dDevB);
}





// DITHERING ROUTINES

// dither tables
static ULONG ulDither4[4][4] = {
  { 0x0F0F0F0F, 0x07070707, 0x0D0D0D0D, 0x05050505 }, 
  { 0x03030303, 0x0B0B0B0B, 0x01010101, 0x09090909 },
  { 0x0C0C0C0C, 0x04040404, 0x0E0E0E0E, 0x06060606 },
  { 0x00000000, 0x08080808, 0x02020202, 0x0A0A0A0A }
};
static ULONG ulDither3[4][4] = {
  { 0x06060606, 0x02020202, 0x06060606, 0x02020202 }, 
  { 0x00000000, 0x04040404, 0x00000000, 0x04040404 },
  { 0x06060606, 0x02020202, 0x06060606, 0x02020202 }, 
  { 0x00000000, 0x04040404, 0x00000000, 0x04040404 },
};
static ULONG ulDither2[4][4] = {
  { 0x02020202, 0x06060606, 0x02020202, 0x06060606 },
  { 0x06060606, 0x02020202, 0x06060606, 0x02020202 },
  { 0x02020202, 0x06060606, 0x02020202, 0x06060606 },
  { 0x06060606, 0x02020202, 0x06060606, 0x02020202 },
};


__int64 mmErrDiffMask=0;
#if (defined __GNUC__)
__int64 mmW3 = 0x0003000300030003ll;
__int64 mmW5 = 0x0005000500050005ll;
__int64 mmW7 = 0x0007000700070007ll;
#else
__int64 mmW3 = 0x0003000300030003;
__int64 mmW5 = 0x0005000500050005;
__int64 mmW7 = 0x0007000700070007;
#endif
__int64 mmShifter = 0;
__int64 mmMask  = 0;
ULONG *pulDitherTable;

#if !(defined __MSVC_INLINE__) && !(defined __GNU_INLINE_X86_32__)
extern const UBYTE *pubClipByte;
// increment a byte without overflowing it
static inline void IncrementByteWithClip( UBYTE &ub, SLONG slAdd)
{
  ub = pubClipByte[(SLONG)ub+slAdd];
}
#endif

// performs dithering of a 32-bit bipmap (can be in-place)
void DitherBitmap( INDEX iDitherType, ULONG *pulSrc, ULONG *pulDst, PIX pixWidth, PIX pixHeight,
                   PIX pixCanvasWidth, PIX pixCanvasHeight)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_DITHERBITMAP);

  // determine row modulo
  if( pixCanvasWidth ==0) pixCanvasWidth  = pixWidth;
  if( pixCanvasHeight==0) pixCanvasHeight = pixHeight;
  ASSERT( pixCanvasWidth>=pixWidth && pixCanvasHeight>=pixHeight);
  SLONG slModulo      = (pixCanvasWidth-pixWidth) *BYTES_PER_TEXEL;
  SLONG slWidthModulo = pixWidth*BYTES_PER_TEXEL +slModulo;
  (void)slWidthModulo; // shut up compiler, this is used if inline ASM is used

  // if bitmap is smaller than 4x2 pixels
  if( pixWidth<4 || pixHeight<2)
  { // don't dither it at all, rather copy only (if needed)
    if( pulDst!=pulSrc) memcpy( pulDst, pulSrc, pixCanvasWidth*pixCanvasHeight *BYTES_PER_TEXEL);
    goto theEnd;
  }

  // determine proper dither type
  switch( iDitherType)
  { // low dithers
  case 1:
    pulDitherTable = &ulDither2[0][0];
    mmShifter = 2;
#ifdef __GNUC__
    mmMask  = 0x3F3F3F3F3F3F3F3Fll;
#else
    mmMask  = 0x3F3F3F3F3F3F3F3F;
#endif
    goto ditherOrder;
  case 2:
    pulDitherTable = &ulDither2[0][0];
    mmShifter = 1;
#ifdef __GNUC__
    mmMask  = 0x7F7F7F7F7F7F7F7Fll;
#else
    mmMask  = 0x7F7F7F7F7F7F7F7F;
#endif
    goto ditherOrder;
  case 3:
#ifdef __GNUC__
    mmErrDiffMask = 0x0003000300030003ll;
#else
    mmErrDiffMask = 0x0003000300030003;
#endif
    goto ditherError;
  // medium dithers
  case 4:
    pulDitherTable = &ulDither2[0][0];
    mmShifter = 0;
#ifdef __GNUC__
    mmMask  = 0xFFFFFFFFFFFFFFFFll;
#else
    mmMask  = 0xFFFFFFFFFFFFFFFF;
#endif
    goto ditherOrder;
  case 5:
    pulDitherTable = &ulDither3[0][0];
    mmShifter = 1;
#ifdef __GNUC__
    mmMask  = 0x7F7F7F7F7F7F7F7Fll;
#else
    mmMask  = 0x7F7F7F7F7F7F7F7F;
#endif
    goto ditherOrder;
  case 6:
    pulDitherTable = &ulDither4[0][0];
    mmShifter = 1;
#ifdef __GNUC__
    mmMask  = 0x7F7F7F7F7F7F7F7Fll;
#else
    mmMask  = 0x7F7F7F7F7F7F7F7F;
#endif
    goto ditherOrder;
  case 7:
#ifdef __GNUC__
    mmErrDiffMask = 0x0007000700070007ll;
#else
    mmErrDiffMask = 0x0007000700070007;
#endif
    goto ditherError;
  // high dithers
  case 8:
    pulDitherTable = &ulDither3[0][0];
    mmShifter = 0;
#ifdef __GNUC__
    mmMask  = 0xFFFFFFFFFFFFFFFFll;
#else
    mmMask  = 0xFFFFFFFFFFFFFFFF;
#endif
    goto ditherOrder;
  case 9:
    pulDitherTable = &ulDither4[0][0];
    mmShifter = 0;
#ifdef __GNUC__
    mmMask  = 0xFFFFFFFFFFFFFFFFll;
#else
    mmMask  = 0xFFFFFFFFFFFFFFFF;
#endif
    goto ditherOrder;
  case 10:
#ifdef __GNUC__
    mmErrDiffMask = 0x000F000F000F000Fll;
#else
    mmErrDiffMask = 0x000F000F000F000F;
#endif
    goto ditherError;
  default:
    // improper dither type
    ASSERTALWAYS( "Improper dithering type.");
    // if bitmap copying is needed
    if( pulDst!=pulSrc) memcpy( pulDst, pulSrc, pixCanvasWidth*pixCanvasHeight *BYTES_PER_TEXEL);
    goto theEnd;
  }

// ------------------------------- ordered matrix dithering routine

ditherOrder:
#if (defined __MSVC_INLINE__)
  __asm {
    mov     esi,D [pulSrc]
    mov     edi,D [pulDst]
    mov     ebx,D [pulDitherTable]
    // reset dither line offset
    xor     eax,eax
    mov     edx,D [pixHeight]
rowLoopO:
    // get horizontal dither patterns
    movq    mm4,Q [ebx+ eax*4 +0]
    movq    mm5,Q [ebx+ eax*4 +8]
    psrlw   mm4,Q [mmShifter]
    psrlw   mm5,Q [mmShifter]
    pand    mm4,Q [mmMask]
    pand    mm5,Q [mmMask]
    // process row
    mov     ecx,D [pixWidth]
pixLoopO:
    movq    mm1,Q [esi +0]
    movq    mm2,Q [esi +8]
    paddusb mm1,mm4
    paddusb mm2,mm5
    movq    Q [edi +0],mm1
    movq    Q [edi +8],mm2
    // advance to next pixel
    add     esi,4*4
    add     edi,4*4
    sub     ecx,4
    jg      pixLoopO  // !!!! possible memory leak?
    je      nextRowO
    // backup couple of pixels
    lea     esi,[esi+ ecx*4]
    lea     edi,[edi+ ecx*4]
nextRowO:
    // get next dither line patterns
    add     esi,D [slModulo]
    add     edi,D [slModulo]
    add     eax,1*4
    and     eax,4*4-1
    // advance to next row
    dec     edx
    jnz     rowLoopO
    emms;
  }

#elif (defined __GNU_INLINE_X86_32__)
  ULONG tmp;
  __asm__ __volatile__ (
    "movl     %[pulSrc], %%esi           \n\t"
    "movl     %[pulDst], %%edi           \n\t"
    // reset dither line offset
    "movl     %[pixHeight], %%eax        \n\t"
    "movl     %%eax, %[xdx]              \n\t"
    "xorl     %%eax, %%eax               \n\t"

    "0:                                  \n\t" // rowLoopO
    // get horizontal dither patterns
    "movq    0(%[pulDitherTable], %%eax, 4), %%mm4 \n\t"
    "movq    8(%[pulDitherTable], %%eax, 4), %%mm5 \n\t"
    "psrlw   (" ASMSYM(mmShifter) "), %%mm4            \n\t"
    "psrlw   (" ASMSYM(mmShifter) "), %%mm5            \n\t"
    "pand    (" ASMSYM(mmMask) "), %%mm4             \n\t"
    "pand    (" ASMSYM(mmMask) "), %%mm5             \n\t"

    // process row
    "movl    %[pixWidth], %%ecx          \n\t"
    "1:                                  \n\t" // pixLoopO
    "movq    0(%%esi), %%mm1             \n\t"
    "movq    8(%%esi), %%mm2             \n\t"
    "paddusb %%mm4, %%mm1                \n\t"
    "paddusb %%mm5, %%mm2                \n\t"
    "movq    %%mm1, 0(%%edi)             \n\t"
    "movq    %%mm2, 8(%%edi)             \n\t"

    // advance to next pixel
    "addl     $16, %%esi                 \n\t"
    "addl     $16, %%edi                 \n\t"
    "subl     $4, %%ecx                  \n\t"
    "jg       1b                         \n\t" // !!!! possible memory leak?
    "je       2f                         \n\t" // nextRowO

    // backup couple of pixels
    "leal     0(%%esi, %%ecx, 4), %%esi  \n\t"
    "leal     0(%%edi, %%ecx, 4), %%edi  \n\t"

    "2:                                  \n\t" // nextRowO
    // get next dither line patterns
    "addl     %[slModulo], %%esi         \n\t"
    "addl     %[slModulo], %%edi         \n\t"
    "addl     $4, %%eax                  \n\t"
    "andl     $15, %%eax                 \n\t"

    // advance to next row
    "decl     %[xdx]                     \n\t"
    "jnz      0b                         \n\t" // rowLoopO
    "emms                                \n\t"
        : [xdx] "=&g" (tmp)
        : [pulSrc] "g" (pulSrc), [pulDst] "g" (pulDst),
          [pixHeight] "g" (pixHeight), [pixWidth] "g" (pixWidth),
          [slModulo] "g" (slModulo), [pulDitherTable] "r" (pulDitherTable)
        : FPU_REGS, MMX_REGS, "eax", "ecx", "esi", "edi",
          "cc", "memory"
  );

#else

#ifdef _MSC_VER
#define PACKED
#pragma pack(push,1)
#else
#define PACKED __attribute__ ((__packed__))
#endif

  union uConv
  {
    __int64 val;
    DWORD dwords[2];
    UWORD words[4];
    WORD  iwords[4];
    UBYTE bytes[8];
  } PACKED;	//avoid optimisation and BUSERROR on Pyra build
#ifdef _MSC_VER
#pragma pack(pop)
#endif
#undef PACKED

  for (int i=0; i<pixHeight; i++) {
    int idx = i&3;
    uConv dith;
    dith.dwords[0] = pulDitherTable[idx];
    dith.dwords[1] = pulDitherTable[idx+1];
    for (int j=0; j<4; j++) { dith.words[j] >>= mmShifter; }
    dith.val &= mmMask;
    uConv* src = (uConv*)(pulSrc+i*pixWidth);
    uConv* dst = (uConv*)(pulDst+i*pixWidth);
    for (int j=0; j<pixWidth; j+=2) {
      uConv p=src[0];
      for (int k=0; k<8; k++) {
        IncrementByteWithClip(p.bytes[k], dith.bytes[k]);
      }
      dst[0] = p;
      src++;
      dst++;
    }
  }

#endif

  goto theEnd;

// ------------------------------- error diffusion dithering routine

ditherError:
  // since error diffusion algorithm requires in-place dithering, original bitmap must be copied if needed
  if( pulDst!=pulSrc) memcpy( pulDst, pulSrc, pixCanvasWidth*pixCanvasHeight *BYTES_PER_TEXEL);
  // slModulo+=4;
  // now, dither destination
#if (defined __MSVC_INLINE__)
  __asm {
    pxor    mm0,mm0
    mov     esi,D [pulDst]
    mov     ebx,D [pixCanvasWidth]
    mov     edx,D [pixHeight]
    dec     edx // need not to dither last row
rowLoopE:
    // left to right
    mov     ecx,D [pixWidth]
    dec     ecx
pixLoopEL:
    movd    mm1,D [esi]
    punpcklbw mm1,mm0
    pand    mm1,Q [mmErrDiffMask] 
    // determine errors
    movq    mm3,mm1
    movq    mm5,mm1
    movq    mm7,mm1
    pmullw  mm3,Q [mmW3]
    pmullw  mm5,Q [mmW5]
    pmullw  mm7,Q [mmW7]
    psrlw   mm3,4     // *3/16
    psrlw   mm5,4     // *5/16
    psrlw   mm7,4     // *7/16
    psubw   mm1,mm3
    psubw   mm1,mm5
    psubw   mm1,mm7   // *rest/16
    packuswb mm1,mm0
    packuswb mm3,mm0
    packuswb mm5,mm0
    packuswb mm7,mm0
    // spread errors
    paddusb mm7,Q [esi+       +4]
    paddusb mm3,Q [esi+ ebx*4 -4]
    paddusb mm5,Q [esi+ ebx*4 +0]
    paddusb mm1,Q [esi+ ebx*4 +4]  // !!!! possible memory leak?
    movd    D [esi+       +4],mm7
    movd    D [esi+ ebx*4 -4],mm3
    movd    D [esi+ ebx*4 +0],mm5
    movd    D [esi+ ebx*4 +4],mm1
    // advance to next pixel
    add     esi,4
    dec     ecx
    jnz     pixLoopEL
    // advance to next row
    add     esi,D [slWidthModulo]
    dec     edx
    jz      allDoneE

    // right to left
    mov     ecx,D [pixWidth]
    dec     ecx
pixLoopER:
    movd    mm1,D [esi]
    punpcklbw mm1,mm0
    pand    mm1,Q [mmErrDiffMask] 
    // determine errors
    movq    mm3,mm1
    movq    mm5,mm1
    movq    mm7,mm1
    pmullw  mm3,Q [mmW3]
    pmullw  mm5,Q [mmW5]
    pmullw  mm7,Q [mmW7]
    psrlw   mm3,4     // *3/16
    psrlw   mm5,4     // *5/16
    psrlw   mm7,4     // *7/16
    psubw   mm1,mm3
    psubw   mm1,mm5
    psubw   mm1,mm7   // *rest/16
    packuswb mm1,mm0
    packuswb mm3,mm0
    packuswb mm5,mm0
    packuswb mm7,mm0
    // spread errors
    paddusb mm7,Q [esi+       -4]
    paddusb mm1,Q [esi+ ebx*4 -4]
    paddusb mm5,Q [esi+ ebx*4 +0]
    paddusb mm3,Q [esi+ ebx*4 +4]   // !!!! possible memory leak?
    movd    D [esi+       -4],mm7
    movd    D [esi+ ebx*4 -4],mm1
    movd    D [esi+ ebx*4 +0],mm5
    movd    D [esi+ ebx*4 +4],mm3
    // revert to previous pixel
    sub     esi,4
    dec     ecx
    jnz     pixLoopER
    // advance to next row
    lea     esi,[esi+ ebx*4]
    dec     edx
    jnz     rowLoopE
allDoneE:
    emms;
  }

#elif (defined __GNU_INLINE_X86_32__)
  __asm__ __volatile__ (
    "pxor    %%mm0, %%mm0                 \n\t"
    "movl    %[pulDst], %%esi             \n\t"
    "movl    %[pixHeight], %%edx          \n\t"
    "decl    %%edx                        \n\t" // need not to dither last row

    "0:                                   \n\t" // rowLoopE
    // left to right
    "movl      %[pixWidth], %%ecx         \n\t"
    "decl      %%ecx                      \n\t"

    "1:                                   \n\t" // pixLoopEL
    "movd      (%%esi), %%mm1             \n\t"
    "punpcklbw %%mm0, %%mm1               \n\t"
    "pand      (" ASMSYM(mmErrDiffMask) "), %%mm1     \n\t"

    // determine errors
    "movq      %%mm1, %%mm3               \n\t"
    "movq      %%mm1, %%mm5               \n\t"
    "movq      %%mm1, %%mm7               \n\t"
    "pmullw    (" ASMSYM(mmW3) "), %%mm3              \n\t"
    "pmullw    (" ASMSYM(mmW5) "), %%mm5              \n\t"
    "pmullw    (" ASMSYM(mmW7) "), %%mm7              \n\t"
    "psrlw     $4, %%mm3                  \n\t"  // *3/16
    "psrlw     $4, %%mm5                  \n\t"  // *5/16
    "psrlw     $4, %%mm7                  \n\t"  // *7/16     
    "psubw     %%mm3,%%mm1                \n\t"
    "psubw     %%mm5,%%mm1                \n\t"
    "psubw     %%mm7,%%mm1                \n\t"  // *rest/16
    "packuswb  %%mm0,%%mm1                \n\t"
    "packuswb  %%mm0,%%mm3                \n\t"
    "packuswb  %%mm0,%%mm5                \n\t"
    "packuswb  %%mm0,%%mm7                \n\t"

    // spread errors
    "paddusb    4(%%esi), %%mm7           \n\t"
    "paddusb   -4(%%esi, %[pixCanvasWidth], 4), %%mm3 \n\t"
    "paddusb    0(%%esi, %[pixCanvasWidth], 4), %%mm5 \n\t"
    "paddusb    4(%%esi, %[pixCanvasWidth], 4), %%mm1 \n\t"  // !!!! possible memory leak?
    "movd      %%mm7,  4(%%esi)           \n\t"
    "movd      %%mm3, -4(%%esi, %[pixCanvasWidth], 4) \n\t"
    "movd      %%mm5,  0(%%esi, %[pixCanvasWidth], 4) \n\t"
    "movd      %%mm1,  4(%%esi, %[pixCanvasWidth], 4) \n\t"

    // advance to next pixel
    "addl      $4, %%esi                  \n\t"
    "decl      %%ecx                      \n\t"
    "jnz       1b                         \n\t" // pixLoopEL

    // advance to next row
    "addl      %[slWidthModulo], %%esi    \n\t"
    "decl      %%edx                      \n\t"
    "jz        3f                         \n\t" // allDoneE

    // right to left
    "movl      %[pixWidth], %%ecx         \n\t"
    "decl      %%ecx                      \n\t"

    "2:                                   \n\t" // pixLoopER
    "movd      (%%esi), %%mm1             \n\t"
    "punpcklbw %%mm0, %%mm1               \n\t"
    "pand      (" ASMSYM(mmErrDiffMask) "), %%mm1     \n\t"
 
    // determine errors
    "movq      %%mm1, %%mm3               \n\t"
    "movq      %%mm1, %%mm5               \n\t"
    "movq      %%mm1, %%mm7               \n\t"
    "pmullw    (" ASMSYM(mmW3) "), %%mm3              \n\t"
    "pmullw    (" ASMSYM(mmW5) "), %%mm5              \n\t"
    "pmullw    (" ASMSYM(mmW7) "), %%mm7              \n\t"
    "psrlw     $4, %%mm3                  \n\t" // *3/16
    "psrlw     $4, %%mm5                  \n\t" // *5/16
    "psrlw     $4, %%mm7                  \n\t" // *7/16
    "psubw     %%mm3, %%mm1               \n\t"
    "psubw     %%mm5, %%mm1               \n\t"
    "psubw     %%mm7, %%mm1               \n\t" // *rest/16
    "packuswb  %%mm0, %%mm1               \n\t"
    "packuswb  %%mm0, %%mm3               \n\t"
    "packuswb  %%mm0, %%mm5               \n\t"
    "packuswb  %%mm0, %%mm7               \n\t"

    // spread errors
    "paddusb   -4(%%esi), %%mm7           \n\t"
    "paddusb   -4(%%esi, %[pixCanvasWidth], 4), %%mm1 \n\t"
    "paddusb    0(%%esi, %[pixCanvasWidth], 4), %%mm5 \n\t"
    "paddusb    4(%%esi, %[pixCanvasWidth], 4), %%mm3 \n\t" // !!!! possible memory leak?
    "movd      %%mm7, -4(%%esi)           \n\t"
    "movd      %%mm1, -4(%%esi, %[pixCanvasWidth], 4) \n\t"
    "movd      %%mm5,  0(%%esi, %[pixCanvasWidth], 4) \n\t"
    "movd      %%mm3,  4(%%esi, %[pixCanvasWidth], 4) \n\t"

    // revert to previous pixel
    "subl      $4, %%esi                  \n\t"
    "decl      %%ecx                      \n\t"
    "jnz       2b                         \n\t" // pixLoopER

    // advance to next row
    "leal      0(%%esi, %[pixCanvasWidth], 4), %%esi \n\t"
    "decl      %%edx                      \n\t"
    "jnz       0b                         \n\t" // rowLoopE
    "3:                                   \n\t" // allDoneE
    "emms                                 \n\t"
        : // no outputs.
        : [pulDst] "g" (pulDst), [pixCanvasWidth] "r" (pixCanvasWidth),
          [pixHeight] "g" (pixHeight), [pixWidth] "g" (pixWidth),
          [slWidthModulo] "g" (slWidthModulo)
        : FPU_REGS, MMX_REGS, "ecx", "edx", "esi", "cc", "memory"
  );

#else
  #if 1 //SEB doesn't works....
  for (int i=0; i<pixHeight-1; i++) {
    int step = (i&1)?-4:+4;
    const UBYTE ubMask = (mmErrDiffMask&0xff);
    UBYTE *src = ((UBYTE*)pulDst)+i*pixCanvasWidth*4;
    if(i&1) src+=pixWidth*4;
    // left to right or right to left
    for (int j=0; j<pixWidth-1; j++) {
      uConv p1, p3, p5, p7;
      src+=step;
      for (int k=0; k<4; k++) { p1.words[k] = src[k]&ubMask; }
      //p1.val &= mmErrDiffMask;
      for (int k=0; k<4; k++) { p3.words[k] = (p1.words[k]*3)>>4;
                                p5.words[k] = (p1.words[k]*5)>>4;
                                p7.words[k] = (p1.words[k]*7)>>4; }
      for (int k=0; k<4; k++) { p1.words[k] -= (p3.words[k] + p5.words[k] + p7.words[k]);}
      for (int k=0; k<4; k++) { 
        IncrementByteWithClip( src[k + step]                 , p7.words[k]);
        IncrementByteWithClip( src[pixCanvasWidth*4 -step +k], p5.words[k]);
        IncrementByteWithClip( src[pixCanvasWidth*4 +0    +k], p3.words[k]);
        IncrementByteWithClip( src[pixCanvasWidth*4 +step +k], p1.words[k]);
      }
    }
  }
  #endif

#endif

  goto theEnd;

  // all done
theEnd:
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_DITHERBITMAP);
}



// performs dithering of a 32-bit mipmaps (can be in-place)
void DitherMipmaps( INDEX iDitherType, ULONG *pulSrc, ULONG *pulDst, PIX pixWidth, PIX pixHeight)
{
  // safety check
  ASSERT( pixWidth>0 && pixHeight>0);
  // loop thru mipmaps
  PIX pixMipSize;
  while( pixWidth>0 && pixHeight>0)
  { // dither one mipmap
    DitherBitmap( iDitherType, pulSrc, pulDst, pixWidth, pixHeight);
    // advance to next mipmap
    pixMipSize = pixWidth*pixHeight;
    pulSrc += pixMipSize;
    pulDst += pixMipSize;
    pixWidth >>=1;
    pixHeight>>=1;
  }
}



// blur/sharpen filters
static INDEX aiFilters[6][3] = {
  {  0,  1, 16 },  // minimum
  {  0,  2,  8 },  // low
  {  1,  2,  7 },  // medium
  {  1,  2,  3 },  // high
  {  3,  4,  5 },  // maximum
  {  1,  1,  1 }}; // 

// temp for middle pixels, vertical/horizontal edges, and corners
__int64 mmMc,  mmMe,  mmMm;  // corner, edge, middle
__int64 mmEch, mmEm;  // corner-high, middle
#define mmEcl mmMc  // corner-low
#define mmEe  mmMe  // edge
__int64 mmCm;  // middle
#define mmCc mmMc  // corner
#define mmCe mmEch // edge
__int64 mmInvDiv;

#if (defined __GNUC__)
__int64 mmAdd = 0x0007000700070007ll;
#else
__int64 mmAdd = 0x0007000700070007;
#endif

// temp rows for in-place filtering support
extern "C" { ULONG aulRows[2048]; }

// FilterBitmap() INTERNAL: generates convolution filter matrix if needed
static INDEX iLastFilter;
static void GenerateConvolutionMatrix( INDEX iFilter)
{
  // same as last?
  if( iLastFilter==iFilter) return;
  // update filter
  iLastFilter = iFilter;
  INDEX iFilterAbs = Abs(iFilter) -1;
  // convert convolution values to MMX format
  INDEX iMc = aiFilters[iFilterAbs][0];  // corner
  INDEX iMe = aiFilters[iFilterAbs][1];  // edge
  INDEX iMm = aiFilters[iFilterAbs][2];  // middle
  // negate values for sharpen filter case
  if( iFilter<0) {
    iMm += (iMe+iMc) *8;  // (4*Edge + 4*Corner) *2
    iMe  = -iMe;
    iMc  = -iMc;
  }
  // find values for edge and corner cases
  INDEX iEch = iMc  + iMe;
  INDEX iEm  = iMm  + iMe;
  INDEX iCm  = iEch + iEm;
  // prepare divider
  __int64 mm = ((__int64)ceil(65536.0f/(iMc*4+iMe*4+iMm))) & 0xFFFF;
  mmInvDiv   = (mm<<48) | (mm<<32) | (mm<<16) | mm;
  // prepare filter values
  mm = iMc  & 0xFFFF;  mmMc = (mm<<48) | (mm<<32) | (mm<<16) | mm;
  mm = iMe  & 0xFFFF;  mmMe = (mm<<48) | (mm<<32) | (mm<<16) | mm;
  mm = iMm  & 0xFFFF;  mmMm = (mm<<48) | (mm<<32) | (mm<<16) | mm;
  mm = iEch & 0xFFFF;  mmEch= (mm<<48) | (mm<<32) | (mm<<16) | mm;
  mm = iEm  & 0xFFFF;  mmEm = (mm<<48) | (mm<<32) | (mm<<16) | mm;
  mm = iCm  & 0xFFFF;  mmCm = (mm<<48) | (mm<<32) | (mm<<16) | mm;
}


extern "C" {
    ULONG *FB_pulSrc = NULL;
    ULONG *FB_pulDst = NULL;
    PIX FB_pixWidth = 0;
    PIX FB_pixHeight = 0;
    PIX FB_pixCanvasWidth = 0;
    SLONG FB_slModulo1 = 0;
    SLONG FB_slCanvasWidth = 0;
}


#if !(defined USE_MMX_INTRINSICS) && !(defined __MSVC_INLINE__) && !(defined __GNU_INLINE_X86_32__)
typedef SWORD ExtPix[4];

static inline void extpix_fromi64(ExtPix &pix, const __int64 i64)
{
    //memcpy(pix, i64, sizeof (ExtPix));
    pix[0] = ((i64 >>  0) & 0xFFFF);
    pix[1] = ((i64 >> 16) & 0xFFFF);
    pix[2] = ((i64 >> 32) & 0xFFFF);
    pix[3] = ((i64 >> 48) & 0xFFFF);
}

static inline void extend_pixel(const ULONG ul, ExtPix &pix)
{
    pix[0] = ((ul >>  0) & 0xFF);
    pix[1] = ((ul >>  8) & 0xFF);
    pix[2] = ((ul >> 16) & 0xFF);
    pix[3] = ((ul >> 24) & 0xFF);
}

static inline ULONG unextend_pixel(const ExtPix &pix)
{
    return
    (
        (((ULONG) ((pix[0] >= 255) ? 255 : ((pix[0] <= 0) ? 0 : pix[0]))) <<  0) |
        (((ULONG) ((pix[1] >= 255) ? 255 : ((pix[1] <= 0) ? 0 : pix[1]))) <<  8) |
        (((ULONG) ((pix[2] >= 255) ? 255 : ((pix[2] <= 0) ? 0 : pix[2]))) << 16) |
        (((ULONG) ((pix[3] >= 255) ? 255 : ((pix[3] <= 0) ? 0 : pix[3]))) << 24)
    );
}

static inline void extpix_add(ExtPix &p1, const ExtPix &p2)
{
    p1[0] = (SWORD) (((SLONG) p1[0]) + ((SLONG) p2[0]));
    p1[1] = (SWORD) (((SLONG) p1[1]) + ((SLONG) p2[1]));
    p1[2] = (SWORD) (((SLONG) p1[2]) + ((SLONG) p2[2]));
    p1[3] = (SWORD) (((SLONG) p1[3]) + ((SLONG) p2[3]));
}

static inline void extpix_mul(ExtPix &p1, const ExtPix &p2)
{
    p1[0] = (SWORD) (((SLONG) p1[0]) * ((SLONG) p2[0]));
    p1[1] = (SWORD) (((SLONG) p1[1]) * ((SLONG) p2[1]));
    p1[2] = (SWORD) (((SLONG) p1[2]) * ((SLONG) p2[2]));
    p1[3] = (SWORD) (((SLONG) p1[3]) * ((SLONG) p2[3]));
}

static inline void extpix_adds(ExtPix &p1, const ExtPix &p2)
{
    SLONG x0 = (((SLONG) ((SWORD) p1[0])) + ((SLONG) ((SWORD) p2[0])));
    SLONG x1 = (((SLONG) ((SWORD) p1[1])) + ((SLONG) ((SWORD) p2[1])));
    SLONG x2 = (((SLONG) ((SWORD) p1[2])) + ((SLONG) ((SWORD) p2[2])));
    SLONG x3 = (((SLONG) ((SWORD) p1[3])) + ((SLONG) ((SWORD) p2[3])));

    p1[0] = (SWORD) ((x0 <= -32768) ? -32768 : ((x0 >= 32767) ? 32767 : x0));
    p1[1] = (SWORD) ((x1 <= -32768) ? -32768 : ((x1 >= 32767) ? 32767 : x1));
    p1[2] = (SWORD) ((x2 <= -32768) ? -32768 : ((x2 >= 32767) ? 32767 : x2));
    p1[3] = (SWORD) ((x3 <= -32768) ? -32768 : ((x3 >= 32767) ? 32767 : x3));
}

static inline void extpix_mulhi(ExtPix &p1, const ExtPix &p2)
{
    p1[0] = (SWORD) (((((SLONG) p1[0]) * ((SLONG) p2[0])) >> 16) & 0xFFFF);
    p1[1] = (SWORD) (((((SLONG) p1[1]) * ((SLONG) p2[1])) >> 16) & 0xFFFF);
    p1[2] = (SWORD) (((((SLONG) p1[2]) * ((SLONG) p2[2])) >> 16) & 0xFFFF);
    p1[3] = (SWORD) (((((SLONG) p1[3]) * ((SLONG) p2[3])) >> 16) & 0xFFFF);
}
#endif


// applies filter to bitmap
void FilterBitmap( INDEX iFilter, ULONG *pulSrc, ULONG *pulDst, PIX pixWidth, PIX pixHeight,
                   PIX pixCanvasWidth, PIX pixCanvasHeight)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_FILTERBITMAP);
  ASSERT( iFilter>=-6 && iFilter<=+6);

  // adjust canvas size
  if( pixCanvasWidth ==0) pixCanvasWidth  = pixWidth;
  if( pixCanvasHeight==0) pixCanvasHeight = pixHeight;
  ASSERT( pixCanvasWidth>=pixWidth && pixCanvasHeight>=pixHeight);

  // if bitmap is smaller than 4x4
  if( pixWidth<4 || pixHeight<4)
  { // don't blur it at all, but eventually only copy
    if( pulDst!=pulSrc) memcpy( pulDst, pulSrc, pixCanvasWidth*pixCanvasHeight *BYTES_PER_TEXEL);
    _pfGfxProfile.StopTimer( CGfxProfile::PTI_FILTERBITMAP);
    return;
  }

  // prepare convolution matrix and row modulo
  iFilter = Clamp( iFilter, (INDEX)-6, (INDEX)6);
  GenerateConvolutionMatrix( iFilter);
  SLONG slModulo1 = (pixCanvasWidth-pixWidth+1) *BYTES_PER_TEXEL;
  SLONG slCanvasWidth = pixCanvasWidth *BYTES_PER_TEXEL;

  // lets roll ...
#if (defined USE_MMX_INTRINSICS)
    slModulo1 /= BYTES_PER_TEXEL;  // C++ handles incrementing by sizeof type
    slCanvasWidth /= BYTES_PER_TEXEL;  // C++ handles incrementing by sizeof type

    ULONG *src = pulSrc;
    ULONG *dst = pulDst;
    ULONG *rowptr = aulRows;

    __m64 rmm0 = _mm_setzero_si64();
    __m64 rmmCm = _mm_set_pi32(((int *)((char*)&mmCm))[0],((int *)((char*)&mmCm))[1]);
    __m64 rmmCe = _mm_set_pi32(((int *)((char*)&mmCe))[0],((int *)((char*)&mmCe))[1]);
    __m64 rmmCc = _mm_set_pi32(((int *)((char*)&mmCc))[0],((int *)((char*)&mmCc))[1]);
    __m64 rmmEch = _mm_set_pi32(((int *)((char*)&mmEch))[0],((int *)((char*)&mmEch))[1]);
    __m64 rmmEcl = _mm_set_pi32(((int *)((char*)&mmEcl))[0],((int *)((char*)&mmEcl))[1]);
    __m64 rmmEe = _mm_set_pi32(((int *)((char*)&mmEe))[0],((int *)((char*)&mmEe))[1]);
    __m64 rmmEm = _mm_set_pi32(((int *)((char*)&mmEm))[0],((int *)((char*)&mmEm))[1]);
    __m64 rmmMm = _mm_set_pi32(((int *)((char*)&mmMm))[0],((int *)((char*)&mmMm))[1]);
    __m64 rmmMe = _mm_set_pi32(((int *)((char*)&mmMe))[0],((int *)((char*)&mmMe))[1]);
    __m64 rmmMc = _mm_set_pi32(((int *)((char*)&mmMc))[0],((int *)((char*)&mmMc))[1]);
    __m64 rmmAdd = _mm_set_pi32(((int *)((char*)&mmAdd))[0],((int *)((char*)&mmAdd))[1]);
    __m64 rmmInvDiv = _mm_set_pi32(((int *)((char*)&mmInvDiv))[0],((int *)((char*)&mmInvDiv))[1]);

    // ----------------------- process upper left corner
    __m64 rmm1 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[0]), rmm0);
    __m64 rmm2 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[1]), rmm0);
    __m64 rmm3 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth]), rmm0);
    __m64 rmm4 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth+1]), rmm0);
    __m64 rmm5 = _mm_setzero_si64();
    __m64 rmm6 = _mm_setzero_si64();
    __m64 rmm7 = _mm_setzero_si64();

    rmm2 = _mm_add_pi16(rmm2, rmm3);
    rmm1 = _mm_mullo_pi16(rmm1, rmmCm);
    rmm2 = _mm_mullo_pi16(rmm2, rmmCe);
    rmm4 = _mm_mullo_pi16(rmm4, rmmCc);
    rmm1 = _mm_add_pi16(rmm1, rmm2);
    rmm1 = _mm_add_pi16(rmm1, rmm4);
    rmm1 = _mm_adds_pi16(rmm1, rmmAdd);
    rmm1 = _mm_mulhi_pi16(rmm1, rmmInvDiv);
    rmm1 = _mm_packs_pu16(rmm1, rmm0);
    *(rowptr++) = _mm_cvtsi64_si32(rmm1);
    src++;

    // ----------------------- process upper edge pixels
    for (PIX i = pixWidth - 2; i != 0; i--)
    {
        rmm1 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-1]), rmm0);
        rmm2 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[0]), rmm0);
        rmm3 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[1]), rmm0);
        rmm4 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth-1]), rmm0);
        rmm5 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth]), rmm0);
        rmm6 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth+1]), rmm0);

        rmm1 = _mm_add_pi16(rmm1, rmm3);
        rmm4 = _mm_add_pi16(rmm4, rmm6);
        rmm1 = _mm_mullo_pi16(rmm1, rmmEch);
        rmm2 = _mm_mullo_pi16(rmm2, rmmEm);
        rmm4 = _mm_mullo_pi16(rmm4, rmmEcl);
        rmm5 = _mm_mullo_pi16(rmm5, rmmEe);
        rmm1 = _mm_add_pi16(rmm1, rmm2);
        rmm1 = _mm_add_pi16(rmm1, rmm4);
        rmm1 = _mm_add_pi16(rmm1, rmm5);
        rmm1 = _mm_adds_pi16(rmm1, rmmAdd);
        rmm1 = _mm_mulhi_pi16(rmm1, rmmInvDiv);
        rmm1 = _mm_packs_pu16(rmm1, rmm0);
        *(rowptr++) = _mm_cvtsi64_si32(rmm1);
        src++;
    }

    // ----------------------- process upper right corner

    rmm1 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-1]), rmm0);
    rmm2 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[0]), rmm0);
    rmm3 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth-1]), rmm0);
    rmm4 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth]), rmm0);

    rmm1 = _mm_add_pi16(rmm1, rmm4);
    rmm1 = _mm_mullo_pi16(rmm1, rmmCe);
    rmm2 = _mm_mullo_pi16(rmm2, rmmCm);
    rmm3 = _mm_mullo_pi16(rmm3, rmmCc);
    rmm1 = _mm_add_pi16(rmm1, rmm2);
    rmm1 = _mm_add_pi16(rmm1, rmm3);
    rmm1 = _mm_adds_pi16(rmm1, rmmAdd);
    rmm1 = _mm_mulhi_pi16(rmm1, rmmInvDiv);
    rmm1 = _mm_packs_pu16(rmm1, rmm0);
    *rowptr = _mm_cvtsi64_si32(rmm1);

// ----------------------- process bitmap middle pixels

    dst += slCanvasWidth;
    src += slModulo1;

    // for each row
    for (size_t i = pixHeight-2; i != 0; i--)  // rowLoop
    {
        rowptr = aulRows;

        // process left edge pixel
        rmm1 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-pixCanvasWidth]), rmm0);
        rmm2 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[(-pixCanvasWidth)+1]), rmm0);
        rmm3 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[0]), rmm0);
        rmm4 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[1]), rmm0);
        rmm5 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth]), rmm0);
        rmm6 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth+1]), rmm0);
        rmm1 = _mm_add_pi16(rmm1, rmm5);
        rmm2 = _mm_add_pi16(rmm2, rmm6);
        rmm1 = _mm_mullo_pi16(rmm1, rmmEch);
        rmm2 = _mm_mullo_pi16(rmm2, rmmEcl);
        rmm3 = _mm_mullo_pi16(rmm3, rmmEm);
        rmm4 = _mm_mullo_pi16(rmm4, rmmEe);
        rmm1 = _mm_add_pi16(rmm1, rmm2);
        rmm1 = _mm_add_pi16(rmm1, rmm3);
        rmm1 = _mm_add_pi16(rmm1, rmm4);
        rmm1 = _mm_adds_pi16(rmm1, rmmAdd);
        rmm1 = _mm_mulhi_pi16(rmm1, rmmInvDiv);
        rmm1 = _mm_packs_pu16(rmm1, rmm0);
        dst[-pixCanvasWidth] = *rowptr;
        *(rowptr++) = _mm_cvtsi64_si32(rmm1);
        src++;
        dst++;

        // for each pixel in current row
        for (size_t j = pixWidth-2; j != 0; j--)  // pixLoop
        {
            // prepare upper convolution row
            rmm1 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[(-pixCanvasWidth)-1]), rmm0);
            rmm2 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-pixCanvasWidth]), rmm0);
            rmm3 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[(-pixCanvasWidth)+1]), rmm0);

            // prepare middle convolution row
            rmm4 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-1]), rmm0);
            rmm5 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[0]), rmm0);
            rmm6 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[1]), rmm0);

            // free some registers
            rmm1 = _mm_add_pi16(rmm1, rmm3);
            rmm2 = _mm_add_pi16(rmm2, rmm4);
            rmm5 = _mm_mullo_pi16(rmm5, rmmMm);

            // prepare lower convolution row
            rmm3 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth-1]), rmm0);
            rmm4 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth]), rmm0);
            rmm7 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth+1]), rmm0);

            // calc weightened value
            rmm2 = _mm_add_pi16(rmm2, rmm6);
            rmm1 = _mm_add_pi16(rmm1, rmm3);
            rmm2 = _mm_add_pi16(rmm2, rmm4);
            rmm1 = _mm_add_pi16(rmm1, rmm7);
            rmm2 = _mm_mullo_pi16(rmm2, rmmMe);
            rmm1 = _mm_mullo_pi16(rmm1, rmmMc);
            rmm2 = _mm_add_pi16(rmm2, rmm5);
            rmm1 = _mm_add_pi16(rmm1, rmm2);

            // calc and store wightened value
            rmm1 = _mm_adds_pi16(rmm1, rmmAdd);
            rmm1 = _mm_mulhi_pi16(rmm1, rmmInvDiv);
            rmm1 = _mm_packs_pu16(rmm1, rmm0);
            dst[-pixCanvasWidth] = *rowptr;
            *(rowptr++) = _mm_cvtsi64_si32(rmm1);

            // advance to next pixel
            src++;
            dst++;
        }

        // process right edge pixel
        rmm1 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[(-pixCanvasWidth)-1]), rmm0);
        rmm2 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-pixCanvasWidth]), rmm0);
        rmm3 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-1]), rmm0);
        rmm4 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[0]), rmm0);
        rmm5 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth-1]), rmm0);
        rmm6 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[pixCanvasWidth]), rmm0);

        rmm1 = _mm_add_pi16(rmm1, rmm5);
        rmm2 = _mm_add_pi16(rmm2, rmm6);
        rmm1 = _mm_mullo_pi16(rmm1, rmmEcl);
        rmm2 = _mm_mullo_pi16(rmm2, rmmEch);
        rmm3 = _mm_mullo_pi16(rmm3, rmmEe);
        rmm4 = _mm_mullo_pi16(rmm4, rmmEm);
        rmm1 = _mm_add_pi16(rmm1, rmm2);
        rmm1 = _mm_add_pi16(rmm1, rmm3);
        rmm1 = _mm_add_pi16(rmm1, rmm4);
        rmm1 = _mm_adds_pi16(rmm1, rmmAdd);
        rmm1 = _mm_mulhi_pi16(rmm1, rmmInvDiv);
        rmm1 = _mm_packs_pu16(rmm1, rmm0);
        dst[-pixCanvasWidth] = *rowptr;
        *rowptr = _mm_cvtsi64_si32(rmm1);

        // advance to next row
        src += slModulo1;
        dst += slModulo1;
    }

    // ----------------------- process lower left corner
    rowptr = aulRows;
    rmm1 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-pixCanvasWidth]), rmm0);
    rmm2 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[(-pixCanvasWidth)+1]), rmm0);
    rmm3 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[0]), rmm0);
    rmm4 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[1]), rmm0);

    rmm1 = _mm_add_pi16(rmm1, rmm4);
    rmm1 = _mm_mullo_pi16(rmm1, rmmCe);
    rmm2 = _mm_mullo_pi16(rmm2, rmmCc);
    rmm3 = _mm_mullo_pi16(rmm3, rmmCm);
    rmm1 = _mm_add_pi16(rmm1, rmm2);
    rmm1 = _mm_add_pi16(rmm1, rmm3);
    rmm1 = _mm_adds_pi16(rmm1, rmmAdd);
    rmm1 = _mm_mulhi_pi16(rmm1, rmmInvDiv);
    rmm1 = _mm_packs_pu16(rmm1, rmm0);
    dst[-pixCanvasWidth] = *rowptr;
    dst[0] = _mm_cvtsi64_si32(rmm1);

    src++;
    dst++;
    rowptr++;

    // ----------------------- process lower edge pixels
    for (size_t i = pixWidth-2; i != 0; i--)  // lowerLoop
    {
        // for each pixel
        rmm1 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[(-pixCanvasWidth)-1]), rmm0);
        rmm2 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-pixCanvasWidth]), rmm0);
        rmm3 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[(-pixCanvasWidth)+1]), rmm0);
        rmm4 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-1]), rmm0);
        rmm5 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[0]), rmm0);
        rmm6 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[1]), rmm0);

        rmm1 = _mm_add_pi16(rmm1, rmm3);
        rmm4 = _mm_add_pi16(rmm4, rmm6);
        rmm1 = _mm_mullo_pi16(rmm1, rmmEcl);
        rmm2 = _mm_mullo_pi16(rmm2, rmmEe);
        rmm4 = _mm_mullo_pi16(rmm4, rmmEch);
        rmm5 = _mm_mullo_pi16(rmm5, rmmEm);
        rmm1 = _mm_add_pi16(rmm1, rmm2);
        rmm1 = _mm_add_pi16(rmm1, rmm4);
        rmm1 = _mm_add_pi16(rmm1, rmm5);
        rmm1 = _mm_adds_pi16(rmm1, rmmAdd);
        rmm1 = _mm_mulhi_pi16(rmm1, rmmInvDiv);
        rmm1 = _mm_packs_pu16(rmm1, rmm0);
        dst[-pixCanvasWidth] = *rowptr;
        dst[0] = _mm_cvtsi64_si32(rmm1);

        // advance to next pixel
        src++;
        dst++;
        rowptr++;
    }

    // ----------------------- lower right corners
    rmm1 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[(-pixCanvasWidth)-1]), rmm0);
    rmm2 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-pixCanvasWidth]), rmm0);
    rmm3 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[-1]), rmm0);
    rmm4 = _mm_unpacklo_pi8(_mm_cvtsi32_si64(src[0]), rmm0);

    rmm2 = _mm_add_pi16(rmm2, rmm3);
    rmm1 = _mm_mullo_pi16(rmm1, rmmCc);
    rmm2 = _mm_mullo_pi16(rmm2, rmmCe);
    rmm4 = _mm_mullo_pi16(rmm4, rmmCm);
    rmm1 = _mm_add_pi16(rmm1, rmm2);
    rmm1 = _mm_add_pi16(rmm1, rmm4);
    rmm1 = _mm_adds_pi16(rmm1, rmmAdd);
    rmm1 = _mm_mulhi_pi16(rmm1, rmmInvDiv);
    rmm1 = _mm_packs_pu16(rmm1, rmm0);
    dst[-pixCanvasWidth] = *rowptr;
    dst[0] = _mm_cvtsi64_si32(rmm1);

    _mm_empty();  // we're done, clear out the MMX registers!


#elif (defined __MSVC_INLINE__)
  __asm {
    cld
    mov     eax,D [pixCanvasWidth] // EAX = positive row offset
    mov     edx,eax
    neg     edx                    // EDX = negative row offset
    pxor    mm0,mm0
    mov     esi,D [pulSrc]
    mov     edi,D [pulDst]
    xor     ebx,ebx

// ----------------------- process upper left corner

    movd    mm1,D [esi+       +0]
    movd    mm2,D [esi+       +4]
    movd    mm3,D [esi+ eax*4 +0]
    movd    mm4,D [esi+ eax*4 +4]
    punpcklbw mm1,mm0
    punpcklbw mm2,mm0
    punpcklbw mm3,mm0
    punpcklbw mm4,mm0
    paddw   mm2,mm3
    pmullw  mm1,Q [mmCm]
    pmullw  mm2,Q [mmCe]
    pmullw  mm4,Q [mmCc]
    paddw   mm1,mm2
    paddw   mm1,mm4
    paddsw  mm1,Q [mmAdd]
    pmulhw  mm1,Q [mmInvDiv]
    packuswb mm1,mm0
    movd    D [ebx+ aulRows],mm1
    add     esi,4
    add     ebx,4

// ----------------------- process upper edge pixels

    mov     ecx,D [pixWidth]
    sub     ecx,2
    // for each pixel
upperLoop:      
    movd    mm1,D [esi+       -4]
    movd    mm2,D [esi+       +0]
    movd    mm3,D [esi+       +4]
    movd    mm4,D [esi+ eax*4 -4]
    movd    mm5,D [esi+ eax*4 +0]
    movd    mm6,D [esi+ eax*4 +4]
    punpcklbw mm1,mm0
    punpcklbw mm2,mm0
    punpcklbw mm3,mm0
    punpcklbw mm4,mm0
    punpcklbw mm5,mm0
    punpcklbw mm6,mm0
    paddw   mm1,mm3
    paddw   mm4,mm6
    pmullw  mm1,Q [mmEch]
    pmullw  mm2,Q [mmEm]
    pmullw  mm4,Q [mmEcl]
    pmullw  mm5,Q [mmEe]
    paddw   mm1,mm2
    paddw   mm1,mm4
    paddw   mm1,mm5
    paddsw  mm1,Q [mmAdd]
    pmulhw  mm1,Q [mmInvDiv]
    packuswb mm1,mm0
    movd    D [ebx+ aulRows],mm1
    // advance to next pixel
    add     esi,4
    add     ebx,4
    dec     ecx
    jnz     upperLoop

// ----------------------- process upper right corner

    movd    mm1,D [esi+       -4]
    movd    mm2,D [esi+       +0]
    movd    mm3,D [esi+ eax*4 -4]
    movd    mm4,D [esi+ eax*4 +0]
    punpcklbw mm1,mm0
    punpcklbw mm2,mm0
    punpcklbw mm3,mm0
    punpcklbw mm4,mm0
    paddw   mm1,mm4
    pmullw  mm1,Q [mmCe]
    pmullw  mm2,Q [mmCm]
    pmullw  mm3,Q [mmCc]
    paddw   mm1,mm2
    paddw   mm1,mm3
    paddsw  mm1,Q [mmAdd]
    pmulhw  mm1,Q [mmInvDiv]
    packuswb mm1,mm0
    movd    D [ebx+ aulRows],mm1

// ----------------------- process bitmap middle pixels

    add     esi,D [slModulo1]
    add     edi,D [slCanvasWidth]
    mov     ebx,D [pixHeight]
    sub     ebx,2
    // for each row
rowLoop:      
    push    ebx
    xor     ebx,ebx
    // process left edge pixel
    movd    mm1,D [esi+ edx*4 +0]
    movd    mm2,D [esi+ edx*4 +4]
    movd    mm3,D [esi+       +0]
    movd    mm4,D [esi+       +4]
    movd    mm5,D [esi+ eax*4 +0]
    movd    mm6,D [esi+ eax*4 +4]
    punpcklbw mm1,mm0
    punpcklbw mm2,mm0
    punpcklbw mm3,mm0
    punpcklbw mm4,mm0
    punpcklbw mm5,mm0
    punpcklbw mm6,mm0
    paddw   mm1,mm5
    paddw   mm2,mm6
    pmullw  mm1,Q [mmEch]
    pmullw  mm2,Q [mmEcl]
    pmullw  mm3,Q [mmEm]
    pmullw  mm4,Q [mmEe]
    paddw   mm1,mm2
    paddw   mm1,mm3
    paddw   mm1,mm4
    paddsw  mm1,Q [mmAdd]
    pmulhw  mm1,Q [mmInvDiv]
    packuswb mm1,mm0
    movd    mm2,D [ebx+ aulRows]
    movd    D [ebx+ aulRows],mm1
    movd    D [edi+ edx*4],mm2
    add     esi,4
    add     edi,4
    add     ebx,4

    // for each pixel in current row
    mov     ecx,D [pixWidth]
    sub     ecx,2
pixLoop:
    // prepare upper convolution row
    movd    mm1,D [esi+ edx*4 -4]
    movd    mm2,D [esi+ edx*4 +0]
    movd    mm3,D [esi+ edx*4 +4]
    punpcklbw mm1,mm0
    punpcklbw mm2,mm0
    punpcklbw mm3,mm0
    // prepare middle convolution row
    movd    mm4,D [esi+ -4]
    movd    mm5,D [esi+ +0]
    movd    mm6,D [esi+ +4]
    punpcklbw mm4,mm0
    punpcklbw mm5,mm0
    punpcklbw mm6,mm0
    // free some registers
    paddw   mm1,mm3
    paddw   mm2,mm4
    pmullw  mm5,Q [mmMm]
    // prepare lower convolution row
    movd    mm3,D [esi+ eax*4 -4]
    movd    mm4,D [esi+ eax*4 +0]
    movd    mm7,D [esi+ eax*4 +4]
    punpcklbw mm3,mm0
    punpcklbw mm4,mm0
    punpcklbw mm7,mm0
    // calc weightened value
    paddw   mm2,mm6
    paddw   mm1,mm3
    paddw   mm2,mm4
    paddw   mm1,mm7
    pmullw  mm2,Q [mmMe]
    pmullw  mm1,Q [mmMc]
    paddw   mm2,mm5
    paddw   mm1,mm2
    // calc and store wightened value
    paddsw  mm1,Q [mmAdd]
    pmulhw  mm1,Q [mmInvDiv]
    packuswb mm1,mm0
    movd    mm2,D [ebx+ aulRows]
    movd    D [ebx+ aulRows],mm1
    movd    D [edi+ edx*4],mm2
    // advance to next pixel
    add     esi,4
    add     edi,4
    add     ebx,4
    dec     ecx
    jnz     pixLoop

    // process right edge pixel
    movd    mm1,D [esi+ edx*4 -4]
    movd    mm2,D [esi+ edx*4 +0]
    movd    mm3,D [esi+       -4]
    movd    mm4,D [esi+       +0]
    movd    mm5,D [esi+ eax*4 -4]
    movd    mm6,D [esi+ eax*4 +0]
    punpcklbw mm1,mm0
    punpcklbw mm2,mm0
    punpcklbw mm3,mm0
    punpcklbw mm4,mm0
    punpcklbw mm5,mm0
    punpcklbw mm6,mm0
    paddw   mm1,mm5
    paddw   mm2,mm6
    pmullw  mm1,Q [mmEcl]
    pmullw  mm2,Q [mmEch]
    pmullw  mm3,Q [mmEe]
    pmullw  mm4,Q [mmEm]
    paddw   mm1,mm2
    paddw   mm1,mm3
    paddw   mm1,mm4
    paddsw  mm1,Q [mmAdd]
    pmulhw  mm1,Q [mmInvDiv]
    packuswb mm1,mm0
    movd    mm2,D [ebx+ aulRows]
    movd    D [ebx+ aulRows],mm1
    movd    D [edi+ edx*4],mm2
    // advance to next row
    add     esi,D [slModulo1]
    add     edi,D [slModulo1]
    pop     ebx
    dec     ebx
    jnz     rowLoop

// ----------------------- process lower left corner

    xor     ebx,ebx
    movd    mm1,D [esi+ edx*4 +0]
    movd    mm2,D [esi+ edx*4 +4]
    movd    mm3,D [esi+       +0]
    movd    mm4,D [esi+       +4]
    punpcklbw mm1,mm0
    punpcklbw mm2,mm0
    punpcklbw mm3,mm0
    punpcklbw mm4,mm0
    paddw   mm1,mm4
    pmullw  mm1,Q [mmCe]
    pmullw  mm2,Q [mmCc]
    pmullw  mm3,Q [mmCm]
    paddw   mm1,mm2
    paddw   mm1,mm3
    paddsw  mm1,Q [mmAdd]
    pmulhw  mm1,Q [mmInvDiv]
    packuswb mm1,mm0
    movd    mm2,D [ebx+ aulRows]
    movd    D [edi],mm1
    movd    D [edi+ edx*4],mm2
    add     esi,4
    add     edi,4
    add     ebx,4

// ----------------------- process lower edge pixels

    mov     ecx,D [pixWidth]
    sub     ecx,2
    // for each pixel
lowerLoop:      
    movd    mm1,D [esi+ edx*4 -4]
    movd    mm2,D [esi+ edx*4 +0]
    movd    mm3,D [esi+ edx*4 +4]
    movd    mm4,D [esi+       -4]
    movd    mm5,D [esi+       +0]
    movd    mm6,D [esi+       +4]
    punpcklbw mm1,mm0
    punpcklbw mm2,mm0
    punpcklbw mm3,mm0
    punpcklbw mm4,mm0
    punpcklbw mm5,mm0
    punpcklbw mm6,mm0
    paddw   mm1,mm3
    paddw   mm4,mm6
    pmullw  mm1,Q [mmEcl]
    pmullw  mm2,Q [mmEe]
    pmullw  mm4,Q [mmEch]
    pmullw  mm5,Q [mmEm]
    paddw   mm1,mm2
    paddw   mm1,mm4
    paddw   mm1,mm5
    paddsw  mm1,Q [mmAdd]
    pmulhw  mm1,Q [mmInvDiv]
    packuswb mm1,mm0
    movd    mm2,D [ebx+ aulRows]
    movd    D [edi],mm1
    movd    D [edi+ edx*4],mm2
    // advance to next pixel
    add     esi,4
    add     edi,4
    add     ebx,4
    dec     ecx
    jnz     lowerLoop

// ----------------------- lower right corners

    movd    mm1,D [esi+ edx*4 -4]
    movd    mm2,D [esi+ edx*4 +0]
    movd    mm3,D [esi+       -4]
    movd    mm4,D [esi+       +0]
    punpcklbw mm1,mm0
    punpcklbw mm2,mm0
    punpcklbw mm3,mm0
    punpcklbw mm4,mm0
    paddw   mm2,mm3
    pmullw  mm1,Q [mmCc]
    pmullw  mm2,Q [mmCe]
    pmullw  mm4,Q [mmCm]
    paddw   mm1,mm2
    paddw   mm1,mm4
    paddsw  mm1,Q [mmAdd]
    pmulhw  mm1,Q [mmInvDiv]
    packuswb mm1,mm0
    movd    mm2,D [ebx+ aulRows]
    movd    D [edi],mm1
    movd    D [edi+ edx*4],mm2
    emms
  }

#elif (defined __GNU_INLINE_X86_32__)

  FB_pulSrc = pulSrc;
  FB_pulDst = pulDst;
  FB_pixWidth = pixWidth;
  FB_pixHeight = pixHeight;
  FB_pixCanvasWidth = pixCanvasWidth;
  FB_slModulo1 = slModulo1;
  FB_slCanvasWidth = slCanvasWidth;

  __asm__ __volatile__ (
    "pushl     %%ebx                      \n\t"
    "cld                                  \n\t"
    "movl      (" ASMSYM(FB_pixCanvasWidth) "), %%eax \n\t" // EAX = positive row offset
    "movl      %%eax, %%edx               \n\t"
    "negl      %%edx                      \n\t" // EDX = negative row offset
    "pxor      %%mm0, %%mm0               \n\t"
    "movl      (" ASMSYM(FB_pulSrc) "), %%esi         \n\t"
    "movl      (" ASMSYM(FB_pulDst) "), %%edi         \n\t"
    "xorl      %%ebx, %%ebx               \n\t"

// ----------------------- process upper left corner

    "movd      0(%%esi), %%mm1            \n\t"
    "movd      4(%%esi), %%mm2            \n\t"
    "movd      0(%%esi, %%eax, 4), %%mm3  \n\t"
    "movd      4(%%esi, %%eax, 4), %%mm4  \n\t"
    "punpcklbw %%mm0, %%mm1               \n\t"
    "punpcklbw %%mm0, %%mm2               \n\t"
    "punpcklbw %%mm0, %%mm3               \n\t"
    "punpcklbw %%mm0, %%mm4               \n\t"
    "paddw     %%mm3, %%mm2               \n\t"
    "pmullw    (" ASMSYM(mmCm) "), %%mm1              \n\t"
    "pmullw    (" ASMSYM(mmEch) "), %%mm2             \n\t"
    "pmullw    (" ASMSYM(mmMc) "), %%mm4              \n\t"
    "paddw     %%mm2, %%mm1               \n\t"
    "paddw     %%mm4, %%mm1               \n\t"
    "paddsw    (" ASMSYM(mmAdd) "), %%mm1             \n\t"
    "pmulhw    (" ASMSYM(mmInvDiv) "), %%mm1          \n\t"
    "packuswb  %%mm0, %%mm1               \n\t"
    "movd      %%mm1, " ASMSYM(aulRows) "(%%ebx)      \n\t"
    "add       $4, %%esi                  \n\t"
    "add       $4, %%ebx                  \n\t"

// ----------------------- process upper edge pixels

    "movl      (" ASMSYM(FB_pixWidth) "), %%ecx       \n\t"
    "subl      $2, %%ecx                  \n\t"

    // for each pixel
    "0:                                   \n\t"  // upperLoop
    "movd      -4(%%esi), %%mm1           \n\t"
    "movd       0(%%esi), %%mm2           \n\t"
    "movd       4(%%esi), %%mm3           \n\t"
    "movd      -4(%%esi, %%eax, 4), %%mm4 \n\t"
    "movd       0(%%esi, %%eax, 4), %%mm5 \n\t"
    "movd       4(%%esi, %%eax, 4), %%mm6 \n\t"
    "punpcklbw %%mm0, %%mm1               \n\t"
    "punpcklbw %%mm0, %%mm2               \n\t"
    "punpcklbw %%mm0, %%mm3               \n\t"
    "punpcklbw %%mm0, %%mm4               \n\t"
    "punpcklbw %%mm0, %%mm5               \n\t"
    "punpcklbw %%mm0, %%mm6               \n\t"
    "paddw     %%mm3, %%mm1               \n\t"
    "paddw     %%mm6, %%mm4               \n\t"
    "pmullw    (" ASMSYM(mmEch) "), %%mm1             \n\t"
    "pmullw    (" ASMSYM(mmEm) "), %%mm2              \n\t"
    "pmullw    (" ASMSYM(mmMc) "), %%mm4              \n\t"
    "pmullw    (" ASMSYM(mmMe) "), %%mm5              \n\t"
    "paddw     %%mm2, %%mm1               \n\t"
    "paddw     %%mm4, %%mm1               \n\t"
    "paddw     %%mm5, %%mm1               \n\t"
    "paddsw    (" ASMSYM(mmAdd) "), %%mm1             \n\t"
    "pmulhw    (" ASMSYM(mmInvDiv) "), %%mm1          \n\t"
    "packuswb  %%mm0, %%mm1               \n\t"
    "movd      %%mm1, " ASMSYM(aulRows) "(%%ebx)      \n\t"

    // advance to next pixel
    "addl      $4, %%esi                  \n\t"
    "addl      $4, %%ebx                  \n\t"
    "decl      %%ecx                      \n\t"
    "jnz       0b                         \n\t"  // upperLoop

// ----------------------- process upper right corner

    "movd      -4(%%esi), %%mm1           \n\t"
    "movd       0(%%esi), %%mm2           \n\t"
    "movd      -4(%%esi, %%eax, 4), %%mm3 \n\t"
    "movd       0(%%esi, %%eax, 4), %%mm4 \n\t"
    "punpcklbw %%mm0, %%mm1               \n\t"
    "punpcklbw %%mm0, %%mm2               \n\t"
    "punpcklbw %%mm0, %%mm3               \n\t"
    "punpcklbw %%mm0, %%mm4               \n\t"
    "paddw     %%mm4, %%mm1               \n\t"
    "pmullw    (" ASMSYM(mmEch) "), %%mm1              \n\t"
    "pmullw    (" ASMSYM(mmCm) "), %%mm2              \n\t"
    "pmullw    (" ASMSYM(mmMc) "), %%mm3              \n\t"
    "paddw     %%mm2, %%mm1               \n\t"
    "paddw     %%mm3, %%mm1               \n\t"
    "paddsw    (" ASMSYM(mmAdd) "), %%mm1             \n\t"
    "pmulhw    (" ASMSYM(mmInvDiv) "), %%mm1          \n\t"
    "packuswb  %%mm0, %%mm1               \n\t"
    "movd      %%mm1, " ASMSYM(aulRows) "(%%ebx)      \n\t"

// ----------------------- process bitmap middle pixels

    "addl      (" ASMSYM(FB_slModulo1) "), %%esi      \n\t"
    "addl      (" ASMSYM(FB_slCanvasWidth) "), %%edi  \n\t"
    "movl      (" ASMSYM(FB_pixHeight) "), %%ebx      \n\t"
    "subl      $2, %%ebx                  \n\t"

    // for each row
    "1:                                   \n\t"  // rowLoop
    "pushl     %%ebx                      \n\t"
    "xorl      %%ebx, %%ebx               \n\t"
    // process left edge pixel
    "movd      0(%%esi, %%edx, 4), %%mm1  \n\t"
    "movd      4(%%esi, %%edx, 4), %%mm2  \n\t"
    "movd      0(%%esi), %%mm3            \n\t"
    "movd      4(%%esi), %%mm4            \n\t"
    "movd      0(%%esi, %%eax, 4), %%mm5  \n\t"
    "movd      4(%%esi, %%eax, 4), %%mm6  \n\t"
    "punpcklbw %%mm0, %%mm1               \n\t"
    "punpcklbw %%mm0, %%mm2               \n\t"
    "punpcklbw %%mm0, %%mm3               \n\t"
    "punpcklbw %%mm0, %%mm4               \n\t"
    "punpcklbw %%mm0, %%mm5               \n\t"
    "punpcklbw %%mm0, %%mm6               \n\t"
    "paddw     %%mm5, %%mm1               \n\t"
    "paddw     %%mm6, %%mm2               \n\t"
    "pmullw    (" ASMSYM(mmEch) "), %%mm1             \n\t"
    "pmullw    (" ASMSYM(mmMc) "), %%mm2              \n\t"
    "pmullw    (" ASMSYM(mmEm) "), %%mm3              \n\t"
    "pmullw    (" ASMSYM(mmMe) "), %%mm4              \n\t"
    "paddw     %%mm2, %%mm1               \n\t"
    "paddw     %%mm3, %%mm1               \n\t"
    "paddw     %%mm4, %%mm1               \n\t"
    "paddsw    (" ASMSYM(mmAdd) "), %%mm1             \n\t"
    "pmulhw    (" ASMSYM(mmInvDiv) "), %%mm1          \n\t"
    "packuswb  %%mm0, %%mm1               \n\t"
    "movd      " ASMSYM(aulRows) "(%%ebx), %%mm2      \n\t"
    "movd      %%mm1, " ASMSYM(aulRows) "(%%ebx)      \n\t"
    "movd      %%mm2, 0(%%edi, %%edx, 4)  \n\t"
    "add       $4, %%esi                  \n\t"
    "add       $4, %%edi                  \n\t"
    "add       $4, %%ebx                  \n\t"

    // for each pixel in current row
    "mov       (" ASMSYM(FB_pixWidth) "), %%ecx       \n\t"
    "sub       $2, %%ecx                  \n\t"
    "2:                                   \n\t" // pixLoop
    // prepare upper convolution row
    "movd      -4(%%esi, %%edx, 4), %%mm1 \n\t"
    "movd       0(%%esi, %%edx, 4), %%mm2 \n\t"
    "movd       4(%%esi, %%edx, 4), %%mm3 \n\t"
    "punpcklbw %%mm0, %%mm1               \n\t"
    "punpcklbw %%mm0, %%mm2               \n\t"
    "punpcklbw %%mm0, %%mm3               \n\t"
    // prepare middle convolution row
    "movd      -4(%%esi), %%mm4           \n\t"
    "movd       0(%%esi), %%mm5           \n\t"
    "movd       4(%%esi), %%mm6           \n\t"
    "punpcklbw %%mm0, %%mm4               \n\t"
    "punpcklbw %%mm0, %%mm5               \n\t"
    "punpcklbw %%mm0, %%mm6               \n\t"
    // free some registers
    "paddw     %%mm3, %%mm1               \n\t"
    "paddw     %%mm4, %%mm2               \n\t"
    "pmullw    (" ASMSYM(mmMm) "), %%mm5              \n\t"
    // prepare lower convolution row
    "movd      -4(%%esi, %%eax, 4), %%mm3 \n\t"
    "movd       0(%%esi, %%eax, 4), %%mm4 \n\t"
    "movd       4(%%esi, %%eax, 4), %%mm7 \n\t"
    "punpcklbw %%mm0, %%mm3               \n\t"
    "punpcklbw %%mm0, %%mm4               \n\t"
    "punpcklbw %%mm0, %%mm7               \n\t"
    // calc weightened value
    "paddw     %%mm6, %%mm2               \n\t"
    "paddw     %%mm3, %%mm1               \n\t"
    "paddw     %%mm4, %%mm2               \n\t"
    "paddw     %%mm7, %%mm1               \n\t"
    "pmullw    (" ASMSYM(mmMe) "), %%mm2              \n\t"
    "pmullw    (" ASMSYM(mmMc) "), %%mm1              \n\t"
    "paddw     %%mm5, %%mm2               \n\t"
    "paddw     %%mm2, %%mm1               \n\t"
    // calc and store wightened value
    "paddsw    (" ASMSYM(mmAdd) "), %%mm1             \n\t"
    "pmulhw    (" ASMSYM(mmInvDiv) "), %%mm1          \n\t"
    "packuswb  %%mm0, %%mm1               \n\t"
    "movd      " ASMSYM(aulRows) "(%%ebx), %%mm2      \n\t"
    "movd      %%mm1, " ASMSYM(aulRows) "(%%ebx)      \n\t"
    "movd      %%mm2, (%%edi, %%edx, 4)   \n\t"
    // advance to next pixel
    "addl      $4, %%esi                  \n\t"
    "addl      $4, %%edi                  \n\t"
    "addl      $4, %%ebx                  \n\t"
    "decl      %%ecx                      \n\t"
    "jnz       2b                         \n\t"  // pixLoop

    // process right edge pixel
    "movd      -4(%%esi, %%edx, 4), %%mm1 \n\t"
    "movd       0(%%esi, %%edx, 4), %%mm2 \n\t"
    "movd      -4(%%esi), %%mm3           \n\t"
    "movd       0(%%esi), %%mm4           \n\t"
    "movd      -4(%%esi, %%eax, 4), %%mm5 \n\t"
    "movd       0(%%esi, %%eax, 4), %%mm6 \n\t"
    "punpcklbw %%mm0, %%mm1               \n\t"
    "punpcklbw %%mm0, %%mm2               \n\t"
    "punpcklbw %%mm0, %%mm3               \n\t"
    "punpcklbw %%mm0, %%mm4               \n\t"
    "punpcklbw %%mm0, %%mm5               \n\t"
    "punpcklbw %%mm0, %%mm6               \n\t"
    "paddw     %%mm5, %%mm1               \n\t"
    "paddw     %%mm6, %%mm2               \n\t"
    "pmullw    (" ASMSYM(mmMc) "), %%mm1              \n\t"
    "pmullw    (" ASMSYM(mmEch) "), %%mm2             \n\t"
    "pmullw    (" ASMSYM(mmMe) "), %%mm3              \n\t"
    "pmullw    (" ASMSYM(mmEm) "), %%mm4              \n\t"
    "paddw     %%mm2, %%mm1               \n\t"
    "paddw     %%mm3, %%mm1               \n\t"
    "paddw     %%mm4, %%mm1               \n\t"
    "paddsw    (" ASMSYM(mmAdd) "), %%mm1             \n\t"
    "pmulhw    (" ASMSYM(mmInvDiv) "), %%mm1          \n\t"
    "packuswb  %%mm0, %%mm1               \n\t"
    "movd      " ASMSYM(aulRows) "(%%ebx), %%mm2      \n\t"
    "movd      %%mm1, " ASMSYM(aulRows) "(%%ebx)      \n\t"
    "movd      %%mm2, 0(%%edi, %%edx, 4)  \n\t"

    // advance to next row
    "addl     (" ASMSYM(FB_slModulo1) "), %%esi       \n\t"  // slModulo1
    "addl     (" ASMSYM(FB_slModulo1) "), %%edi       \n\t"  // slModulo1
    "popl     %%ebx                       \n\t"
    "decl     %%ebx                       \n\t"
    "jnz      1b                          \n\t"  // rowLoop

// ----------------------- process lower left corner

    "xorl      %%ebx, %%ebx               \n\t"
    "movd      0(%%esi, %%edx, 4), %%mm1  \n\t"
    "movd      4(%%esi, %%edx, 4), %%mm2  \n\t"
    "movd      0(%%esi), %%mm3            \n\t"
    "movd      4(%%esi), %%mm4            \n\t"
    "punpcklbw %%mm0, %%mm1               \n\t"
    "punpcklbw %%mm0, %%mm2               \n\t"
    "punpcklbw %%mm0, %%mm3               \n\t"
    "punpcklbw %%mm0, %%mm4               \n\t"
    "paddw     %%mm4, %%mm1               \n\t"
    "pmullw    (" ASMSYM(mmEch) "), %%mm1             \n\t"
    "pmullw    (" ASMSYM(mmMc) "), %%mm2              \n\t"
    "pmullw    (" ASMSYM(mmCm) "), %%mm3              \n\t"
    "paddw     %%mm2, %%mm1               \n\t"
    "paddw     %%mm3, %%mm1               \n\t"
    "paddsw    (" ASMSYM(mmAdd) "), %%mm1             \n\t"
    "pmulhw    (" ASMSYM(mmInvDiv) "), %%mm1          \n\t"
    "packuswb  %%mm0, %%mm1               \n\t"
    "movd      " ASMSYM(aulRows) "(%%ebx), %%mm2      \n\t"
    "movd      %%mm1, (%%edi)             \n\t"
    "movd      %%mm2, 0(%%edi, %%edx, 4)  \n\t"
    "add       $4, %%esi                  \n\t"
    "add       $4, %%edi                  \n\t"
    "add       $4, %%ebx                  \n\t"

// ----------------------- process lower edge pixels

    "movl      (" ASMSYM(FB_pixWidth) "), %%ecx       \n\t" // pixWidth
    "subl      $2, %%ecx                  \n\t"
    // for each pixel
    "3:                                   \n\t" // lowerLoop
    "movd      -4(%%esi, %%edx, 4), %%mm1 \n\t"
    "movd       0(%%esi, %%edx, 4), %%mm2 \n\t"
    "movd       4(%%esi, %%edx, 4), %%mm3 \n\t"
    "movd      -4(%%esi), %%mm4           \n\t"
    "movd       0(%%esi), %%mm5           \n\t"
    "movd       4(%%esi), %%mm6           \n\t"
    "punpcklbw %%mm0, %%mm1               \n\t"
    "punpcklbw %%mm0, %%mm2               \n\t"
    "punpcklbw %%mm0, %%mm3               \n\t"
    "punpcklbw %%mm0, %%mm4               \n\t"
    "punpcklbw %%mm0, %%mm5               \n\t"
    "punpcklbw %%mm0, %%mm6               \n\t"
    "paddw     %%mm3, %%mm1               \n\t"
    "paddw     %%mm6, %%mm4               \n\t"
    "pmullw    (" ASMSYM(mmMc) "), %%mm1              \n\t"
    "pmullw    (" ASMSYM(mmMe) "), %%mm2              \n\t"
    "pmullw    (" ASMSYM(mmEch) "), %%mm4             \n\t"
    "pmullw    (" ASMSYM(mmEm) "), %%mm5              \n\t"
    "paddw     %%mm2, %%mm1               \n\t"
    "paddw     %%mm4, %%mm1               \n\t"
    "paddw     %%mm5, %%mm1               \n\t"
    "paddsw    (" ASMSYM(mmAdd) "), %%mm1             \n\t"
    "pmulhw    (" ASMSYM(mmInvDiv) "), %%mm1          \n\t"
    "packuswb  %%mm0, %%mm1               \n\t"
    "movd      " ASMSYM(aulRows) "(%%ebx), %%mm2      \n\t"
    "movd      %%mm1, (%%edi)             \n\t"
    "movd      %%mm2, 0(%%edi, %%edx, 4)  \n\t"
    // advance to next pixel
    "addl     $4, %%esi                   \n\t"
    "addl     $4, %%edi                   \n\t"
    "addl     $4, %%ebx                   \n\t"
    "decl     %%ecx                       \n\t"
    "jnz      3b                          \n\t" // lowerLoop

// ----------------------- lower right corners

    "movd      -4(%%esi, %%edx, 4), %%mm1 \n\t"
    "movd       0(%%esi, %%edx, 4), %%mm2 \n\t"
    "movd      -4(%%esi), %%mm3           \n\t"
    "movd       0(%%esi), %%mm4           \n\t"
    "punpcklbw %%mm0, %%mm1               \n\t"
    "punpcklbw %%mm0, %%mm2               \n\t"
    "punpcklbw %%mm0, %%mm3               \n\t"
    "punpcklbw %%mm0, %%mm4               \n\t"
    "paddw     %%mm3, %%mm2               \n\t"
    "pmullw    (" ASMSYM(mmMc) "), %%mm1              \n\t"
    "pmullw    (" ASMSYM(mmEch) "), %%mm2              \n\t"
    "pmullw    (" ASMSYM(mmCm) "), %%mm4              \n\t"
    "paddw     %%mm2, %%mm1               \n\t"
    "paddw     %%mm4, %%mm1               \n\t"
    "paddsw    (" ASMSYM(mmAdd) "), %%mm1             \n\t"
    "pmulhw    (" ASMSYM(mmInvDiv) "), %%mm1          \n\t"
    "packuswb  %%mm0, %%mm1               \n\t"
    "movd      " ASMSYM(aulRows) "(%%ebx), %%mm2      \n\t"
    "movd      %%mm1, (%%edi)             \n\t"
    "movd      %%mm2, 0(%%edi, %%edx, 4)  \n\t"
    "emms                                 \n\t"
    "popl      %%ebx                      \n\t"
        : // no outputs.
        : // inputs are all globals.
        : FPU_REGS, MMX_REGS, "eax", "ecx", "edx", "esi", "edi",
          "cc", "memory"
  );

#else
    slModulo1 /= BYTES_PER_TEXEL;  // C++ handles incrementing by sizeof type
    slCanvasWidth /= BYTES_PER_TEXEL;  // C++ handles incrementing by sizeof type

    ULONG *src = pulSrc;
    ULONG *dst = pulDst;
    ULONG *rowptr = aulRows;

    ExtPix rmm1={0}, rmm2={0}, rmm3={0}, rmm4={0}, rmm5={0}, rmm6={0}, rmm7={0};
    #define EXTPIXFROMINT64(x) ExtPix r##x; extpix_fromi64(r##x, x);
    EXTPIXFROMINT64(mmCm);
    EXTPIXFROMINT64(mmCe);
    EXTPIXFROMINT64(mmCc);
    EXTPIXFROMINT64(mmEch);
    EXTPIXFROMINT64(mmEcl);
    EXTPIXFROMINT64(mmEe);
    EXTPIXFROMINT64(mmEm);
    EXTPIXFROMINT64(mmMm);
    EXTPIXFROMINT64(mmMe);
    EXTPIXFROMINT64(mmMc);
    EXTPIXFROMINT64(mmAdd);
    EXTPIXFROMINT64(mmInvDiv);
    #undef EXTPIXFROMINT64

    // ----------------------- process upper left corner
    extend_pixel(src[0], rmm1);
    extend_pixel(src[1], rmm2);
    extend_pixel(src[pixCanvasWidth], rmm3);
    extend_pixel(src[pixCanvasWidth+1], rmm4);

    extpix_add(rmm2, rmm3);
    extpix_mul(rmm1, rmmCm);
    extpix_mul(rmm2, rmmCe);
    extpix_mul(rmm4, rmmCc);
    extpix_add(rmm1, rmm2);
    extpix_add(rmm1, rmm4);
    extpix_adds(rmm1, rmmAdd);
    extpix_mulhi(rmm1, rmmInvDiv);
    *(rowptr++) = unextend_pixel(rmm1);
    
    src++;

    // ----------------------- process upper edge pixels
    for (PIX i = pixWidth - 2; i != 0; i--)
    {
        extend_pixel(src[-1], rmm1);
        extend_pixel(src[0], rmm2);
        extend_pixel(src[1], rmm3);
        extend_pixel(src[pixCanvasWidth-1], rmm4);
        extend_pixel(src[pixCanvasWidth], rmm5);
        extend_pixel(src[pixCanvasWidth+1], rmm6);

        extpix_add(rmm1, rmm3);
        extpix_add(rmm4, rmm6);
        extpix_mul(rmm1, rmmEch);
        extpix_mul(rmm2, rmmEm);
        extpix_mul(rmm4, rmmEcl);
        extpix_mul(rmm5, rmmEe);
        extpix_add(rmm1, rmm2);
        extpix_add(rmm1, rmm4);
        extpix_add(rmm1, rmm5);
        extpix_adds(rmm1, rmmAdd);
        extpix_mulhi(rmm1, rmmInvDiv);
        *(rowptr++) = unextend_pixel(rmm1);
        src++;
    }

    // ----------------------- process upper right corner

    extend_pixel(src[-1], rmm1);
    extend_pixel(src[0], rmm2);
    extend_pixel(src[pixCanvasWidth-1], rmm3);
    extend_pixel(src[pixCanvasWidth], rmm4);

    extpix_add(rmm1, rmm4);
    extpix_mul(rmm1, rmmCe);
    extpix_mul(rmm2, rmmCm);
    extpix_mul(rmm3, rmmCc);
    extpix_add(rmm1, rmm2);
    extpix_add(rmm1, rmm3);
    extpix_adds(rmm1, rmmAdd);
    extpix_mulhi(rmm1, rmmInvDiv);
    *rowptr = unextend_pixel(rmm1);

// ----------------------- process bitmap middle pixels

    dst += slCanvasWidth;
    src += slModulo1;

    // for each row
    for (size_t i = pixHeight-2; i != 0; i--)  // rowLoop
    {
        rowptr = aulRows;

        // process left edge pixel
        extend_pixel(src[-pixCanvasWidth], rmm1);
        extend_pixel(src[(-pixCanvasWidth)+1], rmm2);
        extend_pixel(src[0], rmm3);
        extend_pixel(src[1], rmm4);
        extend_pixel(src[pixCanvasWidth], rmm5);
        extend_pixel(src[pixCanvasWidth+1], rmm6);

        extpix_add(rmm1, rmm5);
        extpix_add(rmm2, rmm6);
        extpix_mul(rmm1, rmmEch);
        extpix_mul(rmm2, rmmEcl);
        extpix_mul(rmm3, rmmEm);
        extpix_mul(rmm4, rmmEe);
        extpix_add(rmm1, rmm2);
        extpix_add(rmm1, rmm3);
        extpix_add(rmm1, rmm4);
        extpix_adds(rmm1, rmmAdd);
        extpix_mulhi(rmm1, rmmInvDiv);
        dst[-pixCanvasWidth] = *rowptr;
        *(rowptr++) = unextend_pixel(rmm1);
        src++;
        dst++;

        // for each pixel in current row
        for (size_t j = pixWidth-2; j != 0; j--)  // pixLoop
        {
            // prepare upper convolution row
            extend_pixel(src[(-pixCanvasWidth)-1], rmm1);
            extend_pixel(src[-pixCanvasWidth], rmm2);
            extend_pixel(src[(-pixCanvasWidth)+1], rmm3);

            // prepare middle convolution row
            extend_pixel(src[-1], rmm4);
            extend_pixel(src[0], rmm5);
            extend_pixel(src[1], rmm6);

            // free some registers
            extpix_add(rmm1, rmm3);
            extpix_add(rmm2, rmm4);
            extpix_mul(rmm5, rmmMm);

            // prepare lower convolution row
            extend_pixel(src[pixCanvasWidth-1], rmm3);
            extend_pixel(src[pixCanvasWidth], rmm4);
            extend_pixel(src[pixCanvasWidth+1], rmm7);

            // calc weightened value
            extpix_add(rmm2, rmm6);
            extpix_add(rmm1, rmm3);
            extpix_add(rmm2, rmm4);
            extpix_add(rmm1, rmm7);
            extpix_mul(rmm2, rmmMe);
            extpix_mul(rmm1, rmmMc);
            extpix_add(rmm2, rmm5);
            extpix_add(rmm1, rmm2);

            // calc and store wightened value
            extpix_adds(rmm1, rmmAdd);
            extpix_mulhi(rmm1, rmmInvDiv);
            dst[-pixCanvasWidth] = *rowptr;
            *(rowptr++) = unextend_pixel(rmm1);

            // advance to next pixel
            src++;
            dst++;
        }

        // process right edge pixel
        extend_pixel(src[(-pixCanvasWidth)-1], rmm1);
        extend_pixel(src[-pixCanvasWidth], rmm2);
        extend_pixel(src[-1], rmm3);
        extend_pixel(src[0], rmm4);
        extend_pixel(src[pixCanvasWidth-1], rmm5);
        extend_pixel(src[pixCanvasWidth], rmm6);

        extpix_add(rmm1, rmm5);
        extpix_add(rmm2, rmm6);
        extpix_mul(rmm1, rmmEcl);
        extpix_mul(rmm2, rmmEch);
        extpix_mul(rmm3, rmmEe);
        extpix_mul(rmm4, rmmEm);
        extpix_add(rmm1, rmm2);
        extpix_add(rmm1, rmm3);
        extpix_add(rmm1, rmm4);
        extpix_adds(rmm1, rmmAdd);
        extpix_mulhi(rmm1, rmmInvDiv);
        dst[-pixCanvasWidth] = *rowptr;
        *rowptr = unextend_pixel(rmm1);

        // advance to next row
        src += slModulo1;
        dst += slModulo1;
    }

    // ----------------------- process lower left corner
    rowptr = aulRows;
    extend_pixel(src[-pixCanvasWidth], rmm1);
    extend_pixel(src[(-pixCanvasWidth)+1], rmm2);
    extend_pixel(src[0], rmm3);
    extend_pixel(src[1], rmm4);

    extpix_add(rmm1, rmm4);
    extpix_mul(rmm1, rmmCe);
    extpix_mul(rmm2, rmmCc);
    extpix_mul(rmm3, rmmCm);
    extpix_add(rmm1, rmm2);
    extpix_add(rmm1, rmm3);
    extpix_adds(rmm1, rmmAdd);
    extpix_mulhi(rmm1, rmmInvDiv);
    dst[-pixCanvasWidth] = *rowptr;
    dst[0] = unextend_pixel(rmm1);

    src++;
    dst++;
    rowptr++;

    // ----------------------- process lower edge pixels
    for (size_t i = pixWidth-2; i != 0; i--)  // lowerLoop
    {
        // for each pixel
        extend_pixel(src[(-pixCanvasWidth)-1], rmm1);
        extend_pixel(src[-pixCanvasWidth], rmm2);
        extend_pixel(src[(-pixCanvasWidth)+1], rmm3);
        extend_pixel(src[-1], rmm4);
        extend_pixel(src[0], rmm5);
        extend_pixel(src[1], rmm6);

        extpix_add(rmm1, rmm3);
        extpix_add(rmm4, rmm6);
        extpix_mul(rmm1, rmmEcl);
        extpix_mul(rmm2, rmmEe);
        extpix_mul(rmm4, rmmEch);
        extpix_mul(rmm5, rmmEm);
        extpix_add(rmm1, rmm2);
        extpix_add(rmm1, rmm4);
        extpix_add(rmm1, rmm5);
        extpix_adds(rmm1, rmmAdd);
        extpix_mulhi(rmm1, rmmInvDiv);
        dst[-pixCanvasWidth] = *rowptr;
        dst[0] = unextend_pixel(rmm1);

        // advance to next pixel
        src++;
        dst++;
        rowptr++;
    }

    // ----------------------- lower right corners
    extend_pixel(src[(-pixCanvasWidth)-1], rmm1);
    extend_pixel(src[-pixCanvasWidth], rmm2);
    extend_pixel(src[-1], rmm3);
    extend_pixel(src[0], rmm4);

    extpix_add(rmm2, rmm3);
    extpix_mul(rmm1, rmmCc);
    extpix_mul(rmm2, rmmCe);
    extpix_mul(rmm4, rmmCm);
    extpix_add(rmm1, rmm2);
    extpix_add(rmm1, rmm4);
    extpix_adds(rmm1, rmmAdd);
    extpix_mulhi(rmm1, rmmInvDiv);
    dst[-pixCanvasWidth] = *rowptr;
    dst[0] = unextend_pixel(rmm1);

#endif

  // all done (finally)
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_FILTERBITMAP);
}
 


// saturate color of bitmap
void AdjustBitmapColor( ULONG *pulSrc, ULONG *pulDst, PIX pixWidth, PIX pixHeight, 
                        SLONG const slHueShift, SLONG const slSaturation)
{
  for( INDEX i=0; i<(pixWidth*pixHeight); i++) {
    pulDst[i] = ByteSwap( AdjustColor( ByteSwap(pulSrc[i]), slHueShift, slSaturation));
  }
}


// create mip-map table for texture or shadow of given dimensions
void MakeMipmapTable( PIX pixU, PIX pixV, MipmapTable &mmt)
{
  mmt.mmt_pixU = pixU;
  mmt.mmt_pixV = pixV;
  // start at first mip map
  PIX pixCurrentU = mmt.mmt_pixU;
  PIX pixCurrentV = mmt.mmt_pixV;
  INDEX iMipmapCurrent = 0;
  SLONG slOffsetCurrent = 0;
  // while the mip-map is not zero-sized
  while (pixCurrentU>0 && pixCurrentV>0) {
    // remember its offset
    mmt.mmt_aslOffsets[iMipmapCurrent] = slOffsetCurrent;
    // go to next mip map
    slOffsetCurrent+=pixCurrentU*pixCurrentV;
    iMipmapCurrent++;
    pixCurrentU>>=1;
    pixCurrentV>>=1;
  }
  // remember number of mip maps and total size
  mmt.mmt_ctMipmaps   = iMipmapCurrent;
  mmt.mmt_slTotalSize = slOffsetCurrent;
}



// TRIANGLE MASK RENDERING (FOR MODEL CLUSTER SHADOWS) ROUTINES

static ULONG *_pulTexture;
static PIX    _pixTexWidth, _pixTexHeight;
__extern BOOL   _bSomeDarkExists = FALSE;


// set texture that will be used for all subsequent triangles
void SetTriangleTexture( ULONG *pulCurrentMipmap, PIX pixMipWidth, PIX pixMipHeight)
{
  _pulTexture   = pulCurrentMipmap;
  _pixTexWidth  = pixMipWidth;
  _pixTexHeight = pixMipHeight;
}

// render one triangle to mask plane for shadow casting purposes
void DrawTriangle_Mask( UBYTE *pubMaskPlane, SLONG slMaskWidth, SLONG slMaskHeight,
                        struct PolyVertex2D *ppv2Vtx1, struct PolyVertex2D *ppv2Vtx2,
                        struct PolyVertex2D *ppv2Vtx3, BOOL bTransparency)
{
  struct PolyVertex2D *pUpper  = ppv2Vtx1;
  struct PolyVertex2D *pMiddle = ppv2Vtx2;
  struct PolyVertex2D *pLower  = ppv2Vtx3;
  struct PolyVertex2D *pTmp;

  // sort vertices by J position
  if( pUpper->pv2_fJ > pMiddle->pv2_fJ) {
    pTmp = pUpper; pUpper = pMiddle; pMiddle = pTmp;
  }
  if( pUpper->pv2_fJ > pLower->pv2_fJ) {
    pTmp = pUpper; pUpper = pLower; pLower = pTmp;
  }
  if( pMiddle->pv2_fJ > pLower->pv2_fJ) {
    pTmp = pMiddle; pMiddle = pLower; pLower = pTmp;
  }

  // determine vertical deltas
  FLOAT fDJShort1 = pMiddle->pv2_fJ - pUpper->pv2_fJ;
  FLOAT fDJShort2 = pLower->pv2_fJ  - pMiddle->pv2_fJ;
  FLOAT fDJLong   = pLower->pv2_fJ  - pUpper->pv2_fJ;
  if( fDJLong == 0) return;

  // determine horizontal deltas
  FLOAT fDIShort1 = pMiddle->pv2_fI - pUpper->pv2_fI;
  FLOAT fDIShort2 = pLower->pv2_fI  - pMiddle->pv2_fI;
  FLOAT fDILong   = pLower->pv2_fI  - pUpper->pv2_fI;

  // determine U/K, V/K and 1/K deltas
  FLOAT fD1oKShort1 = pMiddle->pv2_f1oK - pUpper->pv2_f1oK;
  FLOAT fD1oKShort2 = pLower->pv2_f1oK  - pMiddle->pv2_f1oK;
  FLOAT fD1oKLong   = pLower->pv2_f1oK  - pUpper->pv2_f1oK;
  FLOAT fDUoKShort1 = pMiddle->pv2_fUoK - pUpper->pv2_fUoK;
  FLOAT fDUoKShort2 = pLower->pv2_fUoK  - pMiddle->pv2_fUoK;
  FLOAT fDUoKLong   = pLower->pv2_fUoK  - pUpper->pv2_fUoK;
  FLOAT fDVoKShort1 = pMiddle->pv2_fVoK - pUpper->pv2_fVoK;
  FLOAT fDVoKShort2 = pLower->pv2_fVoK  - pMiddle->pv2_fVoK;
  FLOAT fDVoKLong   = pLower->pv2_fVoK  - pUpper->pv2_fVoK;

  // determine stepping factors;
  FLOAT f1oDJShort1, f1oDJShort2, f1oDJLong;
  if( fDJShort1 != 0) f1oDJShort1 = 1 / fDJShort1;  else f1oDJShort1 = 0;
  if( fDJShort2 != 0) f1oDJShort2 = 1 / fDJShort2;  else f1oDJShort2 = 0;
  if( fDJLong   != 0) f1oDJLong   = 1 / fDJLong;    else f1oDJLong   = 0;

  FLOAT fDIoDJShort1 = fDIShort1 * f1oDJShort1;
  FLOAT fDIoDJShort2 = fDIShort2 * f1oDJShort2;
  FLOAT fDIoDJLong   = fDILong   * f1oDJLong;
  FLOAT fMaxWidth    = fDIoDJLong*fDJShort1 + pUpper->pv2_fI - pMiddle->pv2_fI;

  // determine drawing direction and factors by direction
  SLONG direction = +1;
  if( fMaxWidth > 0) direction = -1;

  // find start and end values for J
  PIX pixUpJ = FloatToInt(pUpper->pv2_fJ  +0.5f);
  PIX pixMdJ = FloatToInt(pMiddle->pv2_fJ +0.5f);
  PIX pixDnJ = FloatToInt(pLower->pv2_fJ  +0.5f);

  // clip vertically
  if( pixDnJ<0 || pixUpJ>=slMaskHeight) return;
  if( pixUpJ<0) pixUpJ=0;
  if( pixDnJ>slMaskHeight) pixDnJ=slMaskHeight;
  if( pixMdJ<0) pixMdJ=0;
  if( pixMdJ>slMaskHeight) pixMdJ=slMaskHeight;
  SLONG fixWidth = slMaskWidth<<11;

  // find prestepped I
  FLOAT fPrestepUp = (FLOAT)pixUpJ - pUpper->pv2_fJ;
  FLOAT fPrestepMd = (FLOAT)pixMdJ - pMiddle->pv2_fJ;
  SLONG fixILong   = FloatToInt((pUpper->pv2_fI  + fPrestepUp * fDIoDJLong  )*2048.0f) +fixWidth*pixUpJ;
  SLONG fixIShort1 = FloatToInt((pUpper->pv2_fI  + fPrestepUp * fDIoDJShort1)*2048.0f) +fixWidth*pixUpJ;
  SLONG fixIShort2 = FloatToInt((pMiddle->pv2_fI + fPrestepMd * fDIoDJShort2)*2048.0f) +fixWidth*pixMdJ;

  // convert steps from floats to fixints (21:11)
  SLONG fixDIoDJLong   = FloatToInt(fDIoDJLong  *2048.0f) +fixWidth;
  SLONG fixDIoDJShort1 = FloatToInt(fDIoDJShort1*2048.0f) +fixWidth;
  SLONG fixDIoDJShort2 = FloatToInt(fDIoDJShort2*2048.0f) +fixWidth;

  // find row counter and max delta J
  SLONG ctJShort1 = pixMdJ - pixUpJ;
  SLONG ctJShort2 = pixDnJ - pixMdJ;
  //SLONG ctJLong   = pixDnJ - pixUpJ;

  FLOAT currK, curr1oK, currUoK, currVoK;
  //PIX   pixJ = pixUpJ;

  // if model has texture and texture has alpha channel, do complex mapping thru texture's alpha channel
  if( _pulTexture!=NULL && bTransparency) 
  { 
    // calculate some texture variables
    FLOAT fD1oKoDJShort1 = fD1oKShort1 * f1oDJShort1;
    FLOAT fD1oKoDJShort2 = fD1oKShort2 * f1oDJShort2;
    FLOAT fD1oKoDJLong   = fD1oKLong   * f1oDJLong;
    FLOAT fDUoKoDJShort1 = fDUoKShort1 * f1oDJShort1;
    FLOAT fDUoKoDJShort2 = fDUoKShort2 * f1oDJShort2;
    FLOAT fDUoKoDJLong   = fDUoKLong   * f1oDJLong;
    FLOAT fDVoKoDJShort1 = fDVoKShort1 * f1oDJShort1;
    FLOAT fDVoKoDJShort2 = fDVoKShort2 * f1oDJShort2;
    FLOAT fDVoKoDJLong   = fDVoKLong   * f1oDJLong;
    ;// FactOverDI = (DFoDJ * (J2-J1) + fact1 - fact2) * 1/width
    FLOAT f1oMaxWidth = 1 / fMaxWidth;
    FLOAT fD1oKoDI = (fD1oKoDJLong * fDJShort1 + pUpper->pv2_f1oK - pMiddle->pv2_f1oK) * f1oMaxWidth;
    FLOAT fDUoKoDI = (fDUoKoDJLong * fDJShort1 + pUpper->pv2_fUoK - pMiddle->pv2_fUoK) * f1oMaxWidth;
    FLOAT fDVoKoDI = (fDVoKoDJLong * fDJShort1 + pUpper->pv2_fVoK - pMiddle->pv2_fVoK) * f1oMaxWidth;
    if( direction == -1) {
      fD1oKoDI = -fD1oKoDI;
      fDUoKoDI = -fDUoKoDI;
      fDVoKoDI = -fDVoKoDI;
    }
    // ###############################################################
    // comment unised vars - clang warnings: -Wunused-but-set-variable
    // ###############################################################
    FLOAT f1oKLong   = pUpper->pv2_f1oK  + fPrestepUp * fD1oKoDJLong;
    //FLOAT f1oKShort1 = pUpper->pv2_f1oK  + fPrestepUp * fD1oKoDJShort1;
    //FLOAT f1oKShort2 = pMiddle->pv2_f1oK + fPrestepMd * fD1oKoDJShort2;
    FLOAT fUoKLong   = pUpper->pv2_fUoK  + fPrestepUp * fDUoKoDJLong;
    //FLOAT fUoKShort1 = pUpper->pv2_fUoK  + fPrestepUp * fDUoKoDJShort1;
    //FLOAT fUoKShort2 = pMiddle->pv2_fUoK + fPrestepMd * fDUoKoDJShort2;
    FLOAT fVoKLong   = pUpper->pv2_fVoK  + fPrestepUp * fDVoKoDJLong;
    //FLOAT fVoKShort1 = pUpper->pv2_fVoK  + fPrestepUp * fDVoKoDJShort1;
    //FLOAT fVoKShort2 = pMiddle->pv2_fVoK + fPrestepMd * fDVoKoDJShort2;
    
    // render upper triangle part
    PIX pixTexU, pixTexV;
    while( ctJShort1>0) {
      SLONG currI = fixILong>>11;
      SLONG countI = abs( currI - (fixIShort1>>11));
      if( countI==0) goto nextLine1;
      curr1oK = f1oKLong;
      currUoK = fUoKLong;
      currVoK = fVoKLong;
      if( direction == -1) currI--;
      if( countI>0) _bSomeDarkExists = TRUE;
      while( countI>0) {
        currK = 1.0f/curr1oK;
        pixTexU = (FloatToInt(currUoK*currK)) & (_pixTexWidth -1);
        pixTexV = (FloatToInt(currVoK*currK)) & (_pixTexHeight-1);
        if( _pulTexture[pixTexV*_pixTexWidth+pixTexU] & ((CT_rAMASK<<7)&CT_rAMASK)) pubMaskPlane[currI] = 0;
        curr1oK += fD1oKoDI;
        currUoK += fDUoKoDI;
        currVoK += fDVoKoDI;
        currI   += direction;
        countI--;
      }
  nextLine1:
      //pixJ++;
      f1oKLong   += fD1oKoDJLong;
      //f1oKShort1 += fD1oKoDJShort1;
      fUoKLong   += fDUoKoDJLong;
      //fUoKShort1 += fDUoKoDJShort1;
      fVoKLong   += fDVoKoDJLong;
      //fVoKShort1 += fDVoKoDJShort1;
      fixILong   += fixDIoDJLong;
      fixIShort1 += fixDIoDJShort1;
      ctJShort1--;
    }

    // render lower triangle part
    while( ctJShort2>0) {
      SLONG currI = fixILong>>11;
      SLONG countI = abs( currI - (fixIShort2>>11));
      if( countI==0) goto nextLine2;
      curr1oK = f1oKLong;
      currUoK = fUoKLong;
      currVoK = fVoKLong;
      if( direction == -1) currI--;
      if( countI>0) _bSomeDarkExists = TRUE;
      while( countI>0) {
        currK = 1.0f/curr1oK;
        pixTexU = (FloatToInt(currUoK*currK)) & (_pixTexWidth -1);
        pixTexV = (FloatToInt(currVoK*currK)) & (_pixTexHeight-1);
        if( _pulTexture[pixTexV*_pixTexWidth+pixTexU] & CT_rAMASK) pubMaskPlane[currI] = 0;
        curr1oK += fD1oKoDI;
        currUoK += fDUoKoDI;
        currVoK += fDVoKoDI;
        currI   += direction;
        countI--;
      }
  nextLine2:
      //pixJ++;
      f1oKLong   += fD1oKoDJLong;
      //f1oKShort2 += fD1oKoDJShort2;
      fUoKLong   += fDUoKoDJLong;
      //fUoKShort2 += fDUoKoDJShort2;
      fVoKLong   += fDVoKoDJLong;
      //fVoKShort2 += fDVoKoDJShort2;
      fixILong   += fixDIoDJLong;
      fixIShort2 += fixDIoDJShort2;
      ctJShort2--;
    }
  }
  // simple flat mapping (no texture at all)
  else 
  { 
    // render upper triangle part
    while( ctJShort1>0) {
      SLONG currI = fixILong>>11;
      SLONG countI = abs( currI - (fixIShort1>>11));
      if( direction == -1) currI--;
      if( countI>0) _bSomeDarkExists = TRUE;
      while( countI>0) {
        pubMaskPlane[currI] = 0;
        currI += direction;
        countI--;
      }
      //pixJ++;
      fixILong   += fixDIoDJLong;
      fixIShort1 += fixDIoDJShort1;
      ctJShort1--;
    }
    // render lower triangle part
    while( ctJShort2>0) {
      SLONG currI = fixILong>>11;
      SLONG countI = abs( currI - (fixIShort2>>11));
      if( countI>0) _bSomeDarkExists = TRUE;
      if( direction == -1) currI--;
      while( countI>0) {
        pubMaskPlane[currI] = 0;
        currI += direction;
        countI--;
      }
      //pixJ++;
      fixILong   += fixDIoDJLong;
      fixIShort2 += fixDIoDJShort2;
      ctJShort2--;
    }
  }
}








// ---------------------------------------------------------------------------------------------


#if 0

  // bilinear filtering of lower mipmap
  
  // row loop
  UBYTE r,g,b,a;
  for( PIX v=0; v<pixHeight; v++)
  { // column loop
    for( PIX u=0; u<pixWidth; u++)
    { // read four neighbour pixels
      COLOR colUL = pulSrcMipmap[((v*2+0)*pixCurrWidth*2+u*2) +0];
      COLOR colUR = pulSrcMipmap[((v*2+0)*pixCurrWidth*2+u*2) +1];
      COLOR colDL = pulSrcMipmap[((v*2+1)*pixCurrWidth*2+u*2) +0];
      COLOR colDR = pulSrcMipmap[((v*2+1)*pixCurrWidth*2+u*2) +1];
      // separate and add channels
      ULONG rRes=0, gRes=0, bRes=0, aRes=0;
      ColorToRGBA( colUL, r,g,b,a); rRes += r; gRes += g; bRes += b; aRes += a;
      ColorToRGBA( colUR, r,g,b,a); rRes += r; gRes += g; bRes += b; aRes += a;
      ColorToRGBA( colDL, r,g,b,a); rRes += r; gRes += g; bRes += b; aRes += a;
      ColorToRGBA( colDR, r,g,b,a); rRes += r; gRes += g; bRes += b; aRes += a;
      // round, average and store
      rRes += 2; gRes += 2; bRes += 2; aRes += 2;
      rRes >>=2; gRes >>=2; bRes >>=2; aRes >>=2;
      pulDstMipmap[v*pixCurrWidth+u] = RGBAToColor( rRes,gRes,bRes,aRes);
    }
  }



  // nearest-neighbouring of lower mipmap (with border preservance)

  // row loop
  PIX u,v;
  for( v=0; v<pixCurrHeight/2; v++) { 
    for( u=0; u<pixCurrWidth/2; u++) { // mipmap upper left pixel
      pulDstMipmap[v*pixCurrWidth+u] = pulSrcMipmap[((v*2+0)*pixCurrWidth*2+u*2) +0];
    }
    for( u=pixCurrWidth/2; u<pixCurrWidth; u++) { // mipmap upper right pixel
      pulDstMipmap[v*pixCurrWidth+u] = pulSrcMipmap[((v*2+0)*pixCurrWidth*2+u*2) +1];
    }
  }
  for( v=pixCurrHeight/2; v<pixCurrHeight; v++) { 
    for( u=0; u<pixCurrWidth/2; u++) { // mipmap upper left pixel
      pulDstMipmap[v*pixCurrWidth+u] = pulSrcMipmap[((v*2+1)*pixCurrWidth*2+u*2) +0];
    }
    for( u=pixCurrWidth/2; u<pixCurrWidth; u++) { // mipmap upper right pixel
      pulDstMipmap[v*pixCurrWidth+u] = pulSrcMipmap[((v*2+1)*pixCurrWidth*2+u*2) +1];
    }
  }



  // left to right error diffusion dithering

  __asm {
    pxor    mm0,mm0
    mov     esi,D [pulDst]
    mov     ebx,D [pixCanvasWidth]
    mov     edx,D [pixHeight]
    dec     edx // need not to dither last row
rowLoopE:
    mov     ecx,D [pixWidth]
    dec     ecx
pixLoopE:
    movd    mm1,D [esi]
    punpcklbw mm1,mm0
    pand    mm1,Q [mmErrDiffMask] 
    // determine errors
    movq    mm3,mm1
    paddw   mm3,mm3 // *2
    movq    mm5,mm3
    paddw   mm5,mm5 // *4
    movq    mm7,mm5
    paddw   mm7,mm7 // *8
    paddw   mm3,mm1 // *3
    paddw   mm5,mm1 // *5
    psubw   mm7,mm1 // *7
    psrlw   mm1,4
    psrlw   mm3,4
    psrlw   mm5,4
    psrlw   mm7,4
    packuswb mm1,mm0
    packuswb mm3,mm0
    packuswb mm5,mm0
    packuswb mm7,mm0
    // spread errors
    movd    mm2,D [esi+ ebx*4 +4]
    paddusb mm1,mm2
    paddusb mm3,Q [esi+ ebx*4 -4]
    paddusb mm5,Q [esi+ ebx*4 +0]
    paddusb mm7,Q [esi+       +4]
    movd    D [esi+ ebx*4 +4],mm1
    movd    D [esi+ ebx*4 -4],mm3
    movd    D [esi+ ebx*4 +0],mm5
    movd    D [esi+       +4],mm7
    // advance to next pixel
    add     esi,4
    dec     ecx
    jnz     pixLoopE
    // advance to next row
    add     esi,D [slModulo]
    dec     edx
    jnz     rowLoopE
    emms
  }


  // left to right and right to left error diffusion dithering

  __asm {
    pxor    mm0,mm0
    mov     esi,D [pulDst]
    mov     ebx,D [pixCanvasWidth]
    mov     edx,D [pixHeight]
    dec     edx // need not to dither last row
rowLoopE:
    // left to right
    mov     ecx,D [pixWidth]
    dec     ecx
pixLoopEL:
    movd    mm1,D [esi]
    punpcklbw mm1,mm0
    pand    mm1,Q [mmErrDiffMask] 
    // determine errors
    movq    mm3,mm1
    paddw   mm3,mm3 // *2
    movq    mm5,mm3
    paddw   mm5,mm5 // *4
    movq    mm7,mm5
    paddw   mm7,mm7 // *8
    paddw   mm3,mm1 // *3
    paddw   mm5,mm1 // *5
    psubw   mm7,mm1 // *7
    psrlw   mm1,4
    psrlw   mm3,4
    psrlw   mm5,4
    psrlw   mm7,4
    packuswb mm1,mm0
    packuswb mm3,mm0
    packuswb mm5,mm0
    packuswb mm7,mm0
    // spread errors
    movd    mm2,D [esi+ ebx*4 +4]
    paddusb mm1,mm2
    paddusb mm3,Q [esi+ ebx*4 -4]
    paddusb mm5,Q [esi+ ebx*4 +0]
    paddusb mm7,Q [esi+       +4]
    movd    D [esi+ ebx*4 +4],mm1
    movd    D [esi+ ebx*4 -4],mm3
    movd    D [esi+ ebx*4 +0],mm5
    movd    D [esi+       +4],mm7
    // advance to next pixel
    add     esi,4
    dec     ecx
    jnz     pixLoopEL
    // advance to next row
    add     esi,D [slWidthModulo]
    dec     edx
    jz      allDoneE

    // right to left
    mov     ecx,D [pixWidth]
    dec     ecx
pixLoopER:
    movd    mm1,D [esi]
    punpcklbw mm1,mm0
    pand    mm1,Q [mmErrDiffMask] 
    // determine errors
    movq    mm3,mm1
    paddw   mm3,mm3 // *2
    movq    mm5,mm3
    paddw   mm5,mm5 // *4
    movq    mm7,mm5
    paddw   mm7,mm7 // *8
    paddw   mm3,mm1 // *3
    paddw   mm5,mm1 // *5
    psubw   mm7,mm1 // *7
    psrlw   mm1,4
    psrlw   mm3,4
    psrlw   mm5,4
    psrlw   mm7,4
    packuswb mm1,mm0
    packuswb mm3,mm0
    packuswb mm5,mm0
    packuswb mm7,mm0
    // spread errors
    paddusb mm1,Q [esi+ ebx*4 -4]
    paddusb mm3,Q [esi+ ebx*4 +4]
    paddusb mm5,Q [esi+ ebx*4 +0]
    paddusb mm7,Q [esi+       -4]
    movd    D [esi+ ebx*4 -4],mm1
    movd    D [esi+ ebx*4 +4],mm3
    movd    D [esi+ ebx*4 +0],mm5
    movd    D [esi+       -4],mm7
    // revert to previous pixel
    sub     esi,4
    dec     ecx
    jnz     pixLoopER
    // advance to next row
    add     esi,D [slCanvasWidth]
    dec     edx
    jnz     rowLoopE
allDoneE:
    emms
  }



// bicubic

  static INDEX aiWeights[4][4] = {
{ -1,  9,  9, -1, },
{  9, 47, 47,  9, },
{  9, 47, 47,  9, },
{ -1,  9,  9, -1  }
};


      const SLONG slMaskU=pixWidth *2 -1;
    const SLONG slMaskV=pixHeight*2 -1;

    // bicubic?
    if( pixWidth>4 && pixHeight>4 /*&& tex_bBicubicMipmaps*/)
    {
      for( INDEX j=0; j<pixHeight; j++) {
        for( INDEX i=0; i<pixWidth; i++) {
          COLOR col;
          UBYTE ubR, ubG, ubB, ubA;
          SLONG slR=0, slG=0, slB=0, slA=0;
          for( INDEX v=0; v<4; v++) {
            const INDEX iRowSrc = ((v-1)+j*2) & slMaskV;
            for( INDEX u=0; u<4; u++) {
              const INDEX iColSrc = ((u-1)+i*2) & slMaskU;
              const INDEX iWeight = aiWeights[u][v];
              col = ByteSwap( pulSrcMipmap[iRowSrc*(slMaskU+1)+iColSrc]);
              ColorToRGBA( col, ubR,ubG,ubB,ubA);
              slR += ubR*iWeight;
              slG += ubG*iWeight;
              slB += ubB*iWeight;
              slA += ubA*iWeight;
            }
          }
          col = RGBAToColor( slR>>8, slG>>8, slB>>8, slA>>8);
          pulDstMipmap[j*pixWidth+i] = ByteSwap(col);
        }
      }
    }
    // bilinear!
    else
    {


    }

#endif
