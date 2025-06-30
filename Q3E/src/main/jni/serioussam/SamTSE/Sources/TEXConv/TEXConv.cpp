/* Copyright (c) 2023 Alexander Pavlov <t.x00100x.t@yandex.ru>

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


// TEXConv - Converter normal texture to TGA format.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <Engine/Engine.h>
#include <Engine/Base/Shell.h>
#include <Engine/Base/Stream.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CEntityClass.h>

// Not used; dummy declaration only needed by
// Engine/Base/ErrorReporting.o
HWND _hwndMain = NULL;

#ifdef PLATFORM_UNIX
// retrives memory offset of a specified mip-map or a size of all mip-maps (IN PIXELS!)
// (zero offset means first, i.e. largest mip-map)
extern PIX GetMipmapOffset(INDEX iMipLevel, PIX pixWidth, PIX pixHeight);
// adds 8-bit opaque alpha channel to 24-bit bitmap (in place supported)
extern void AddAlphaChannel(UBYTE *pubSrcBitmap, ULONG *pulDstBitmap, PIX pixSize, UBYTE *pubAlphaBitmap);
// removes 8-bit alpha channel from 32-bit bitmap (in place supported)
extern void RemoveAlphaChannel(ULONG *pulSrcBitmap, UBYTE *pubDstBitmap, PIX pixSize);
#else
// retrives memory offset of a specified mip-map or a size of all mip-maps (IN PIXELS!)
// (zero offset means first, i.e. largest mip-map)
static PIX GetMipmapOffset(INDEX iMipLevel, PIX pixWidth, PIX pixHeight)
{
	PIX pixTexSize = 0;
	PIX pixMipSize = pixWidth*pixHeight;
	INDEX iMips = GetNoOfMipmaps(pixWidth, pixHeight);
	iMips = Min(iMips, iMipLevel);
	while (iMips>0) {
		pixTexSize += pixMipSize;
		pixMipSize >>= 2;
		iMips--;
	}
	return pixTexSize;
}

// adds 8-bit opaque alpha channel to 24-bit bitmap (in place supported)
static void AddAlphaChannel(UBYTE *pubSrcBitmap, ULONG *pulDstBitmap, PIX pixSize, UBYTE *pubAlphaBitmap)
{
	UBYTE ubR, ubG, ubB, ubA = 255;
	// loop backwards thru all bitmap pixels
	for (INDEX iPix = (pixSize - 1); iPix >= 0; iPix--) {
		ubR = pubSrcBitmap[iPix * 3 + 0];
		ubG = pubSrcBitmap[iPix * 3 + 1];
		ubB = pubSrcBitmap[iPix * 3 + 2];
		if (pubAlphaBitmap != NULL) ubA = pubAlphaBitmap[iPix];
		else ubA = 255; // for the sake of forced RGBA internal formats!
		pulDstBitmap[iPix] = ByteSwap(RGBAToColor(ubR, ubG, ubB, ubA));
	}
}

// removes 8-bit alpha channel from 32-bit bitmap (in place supported)
static void RemoveAlphaChannel(ULONG *pulSrcBitmap, UBYTE *pubDstBitmap, PIX pixSize)
{
	UBYTE ubR, ubG, ubB;
	// loop thru all bitmap pixels
	for (INDEX iPix = 0; iPix<pixSize; iPix++) {
		ColorToRGB(ByteSwap(pulSrcBitmap[iPix]), ubR, ubG, ubB);
		pubDstBitmap[iPix * 3 + 0] = ubR;
		pubDstBitmap[iPix * 3 + 1] = ubG;
		pubDstBitmap[iPix * 3 + 2] = ubB;
	}
}
#endif
// Convert old texture format (3) to format (4)
static void Convert( CTextureData *pTD)
{
  // skip effect textures
  if( pTD->td_ptegEffect != NULL) return;

  // determine dimensions 
  PIX pixWidth     = pTD->GetPixWidth();
  PIX pixHeight    = pTD->GetPixHeight();
  PIX pixMipSize   = pixWidth * pixHeight;
  PIX pixFrameSize = GetMipmapOffset( 15, pixWidth, pixHeight);
  // allocate memory for new texture
  ULONG *pulFramesNew = (ULONG*)AllocMemory( pixFrameSize*pTD->td_ctFrames *BYTES_PER_TEXEL);
  UWORD *puwFramesOld = (UWORD*)pTD->td_pulFrames;
  ASSERT( puwFramesOld!=NULL);

  // determine alpha channel presence
  BOOL bHasAlphaChannel = pTD->td_ulFlags & TEX_ALPHACHANNEL;

  // unpack texture from 16-bit RGBA4444 or RGBA5551 format to RGBA8888 32-bit format
  UBYTE r,g,b,a;
  // for each frame
  for( INDEX iFr=0; iFr<pTD->td_ctFrames; iFr++)
  { // get addresses of current frames (new and old)
    PIX pixFrameOffset = iFr * pixFrameSize;
    // for each pixel
    for( INDEX iPix=0; iPix<pixMipSize; iPix++)
    { // read 16-bit pixel
      UWORD uwPix = puwFramesOld[pixFrameOffset+iPix];
      // unpack it
      if( bHasAlphaChannel) {
        // with alpha channel
        r = (uwPix & 0xF000) >>8;
        g = (uwPix & 0x0F00) >>4;
        b = (uwPix & 0x00F0) >>0;
        a = (uwPix & 0x000F) <<4;
        // adjust strength
        r |= r>>4; g |= g>>4; b |= b>>4; a |= a>>4;
      } else {
        // without alpha channel
        r = (uwPix & 0xF800) >>8;
        g = (uwPix & 0x07C0) >>3;
        b = (uwPix & 0x003E) <<2;
        a = 0xFF;
        // adjust strength
        r |= r>>5; g |= g>>5; b |= b>>5;
      }

      // pack it back to 32-bit
      ULONG ulPix = RGBAToColor(r,g,b,a);
      // store 32-bit pixel
      pulFramesNew[pixFrameOffset+iPix] = ByteSwap(ulPix);
    }
  }

  // free old memory
  FreeMemory( pTD->td_pulFrames);
  // remember new texture parameters
  pTD->td_pulFrames   = pulFramesNew;
  pTD->td_slFrameSize = pixFrameSize *BYTES_PER_TEXEL;
}

void SubMain( int argc, char *argv[])
{
  printf("\nTEXConv - Converter normal texture to TGA format (1.00)\n");
  printf(  "           (C)2023  Alexander Pavlov <t.x00100x.t@yandex.ru>\n\n");
  // 2 parameters are allowed as input
  if( (argc<2) || (argc>3))
  {
    printf( "USAGE: TEXConv <texture_file> [<tga_output_file>] ");
    printf( "\n");
    printf( "\n");
    printf( "texture_file: path to texture file\n");
    printf( "tga_output_file: path to create tga file\n");
    printf( "\n");
    printf( "NOTES: - out file will have the name as texture file, but \".tga\" extension\n");
    printf( "The texture file must be in the game directory. In the root or any subdirectory\n");
    exit(EXIT_FAILURE);
  }

  // initialize engine
#ifdef PLATFORM_UNIX
  SE_InitEngine("","");
#else
  SE_InitEngine("");
#endif
  // first input parameter is texture name
  CTFileName fnTexture = CTString(argv[1]);
  if(!FileExists(fnTexture)) {
    printf( "Error: You specified an incorrect file name\n");
    exit(EXIT_FAILURE);
  }

  // output tga file
  CTFileName fnOrderFile = fnTexture.NoExt()+".tga";
  // parameter 2 specifies tga output file
  if( argc>2 ) {
    fnOrderFile = CTString(argv[2]);
  }

  // the creation of the tga image begins
  CTextureData *pTD = new CTextureData;
  CTFileStream TEXFile;
  SLONG slFileSize;

  // determine file size
  TEXFile.Open_t( fnTexture, CTStream::OM_READ);
  slFileSize = TEXFile.GetStreamSize();
  printf("Open texture file: %s (bytes:%d)\n",(const char *) TEXFile.GetDescription(),slFileSize);

  // read version
  INDEX iVersion;
  TEXFile.ExpectID_t( "TVER");
  TEXFile >> iVersion;
  printf("Texture version: %d\n",iVersion);

  // check currently supported versions
  if( iVersion!=4 && iVersion!=3) throw( TRANSV("Error: Invalid texture format version."));
  
  // mark if this texture was loaded form the old format
  if( iVersion==3) pTD->td_ulFlags |= TEX_WASOLD;
  BOOL bAlphaChannel = FALSE;

  // loop trough file and react according to chunk ID
  do
  {
    // obtain chunk id
    CChunkID idChunk = TEXFile.GetID_t();
    if( idChunk == CChunkID("    ")) {
      // we should stop reading when an invalid chunk has been encountered
      break;
    }

    // if this is chunk containing texture data
    if( idChunk == CChunkID("TDAT"))
    {
      printf("CChunkID: TDAT\n");
      // read data describing texture
      ULONG ulFlags=0;
      INDEX ctMipLevels;
      TEXFile >> ulFlags;
      TEXFile >> pTD->td_mexWidth;
      TEXFile >> pTD->td_mexHeight;
      TEXFile >> pTD->td_ctFineMipLevels;
      if( iVersion!=4) TEXFile >> ctMipLevels;
      TEXFile >> pTD->td_iFirstMipLevel;
      if( iVersion!=4) TEXFile >> pTD->td_slFrameSize;
      TEXFile >> pTD->td_ctFrames;
      // combine flags
      pTD->td_ulFlags |= ulFlags;
      bAlphaChannel = pTD->td_ulFlags&TEX_ALPHACHANNEL;
      // determine frame size
      if( iVersion==4) pTD->td_slFrameSize = GetMipmapOffset( 15, pTD->GetPixWidth(), pTD->GetPixHeight())
                                      * BYTES_PER_TEXEL;
    } // TDAT

    // if this is chunk containing raw frames
    else if( idChunk == CChunkID("FRMS")) 
    { 
      printf("CChunkID: FRMS\n");
      // calculate texture size for corresponding texture format and allocate memory
      SLONG slTexSize = pTD->td_slFrameSize * pTD->td_ctFrames;
      pTD->td_pulFrames = (ULONG*)AllocMemory( slTexSize);
      // if older version
      if( iVersion==3) {
        // alloc memory block and read mip-maps
        TEXFile.Read_t( pTD->td_pulFrames, slTexSize);
        #if PLATFORM_BIGENDIAN
        for (SLONG i = 0; i < slTexSize/4; i++)
            BYTESWAP(pTD->td_pulFrames[i]);
        #endif
      } 
      // if current version
      else {
        PIX pixFrameSizeOnDisk = pTD->GetPixWidth()*pTD->GetPixHeight();
        for( INDEX iFr=0; iFr<pTD->td_ctFrames; iFr++)
        { // loop thru frames
          ULONG *pulCurrentFrame = pTD->td_pulFrames + (iFr * pTD->td_slFrameSize/BYTES_PER_TEXEL);
          if( bAlphaChannel) {
            // read texture with alpha channel from file
            TEXFile.Read_t( pulCurrentFrame, pixFrameSizeOnDisk *4);
            #if PLATFORM_BIGENDIAN
            for (SLONG i = 0; i < pixFrameSizeOnDisk; i++)
                BYTESWAP(pulCurrentFrame[i]);
            #endif
          } else {
            // read texture without alpha channel from file
            TEXFile.Read_t( pulCurrentFrame, pixFrameSizeOnDisk *3);
            // add opaque alpha channel
            AddAlphaChannel( (UBYTE*)pulCurrentFrame, pulCurrentFrame, pixFrameSizeOnDisk);
          }
        }
      }
    } // FRMS
  }
  // until we didn't reach end of file
  while( !TEXFile.AtEOF());

  // if texture is in old format, convert it to current format
  if( iVersion==3) Convert(pTD);
  printf("Width: %d  Height: %d\n", pTD->GetPixHeight(), pTD->GetPixWidth());

  // Croteam internal image format
  CImageInfo ii;
  // needs only first frame
  INDEX iFrame = 0;
  // prepare miplevel and mipmap offset
  PIX pixWidth  = pTD->GetPixWidth();
  PIX pixHeight = pTD->GetPixHeight();
  // export header to image info structure
  ii.Clear();
  ii.ii_Width  = pixWidth;
  ii.ii_Height = pixHeight;
  ii.ii_BitsPerPixel = (pTD->td_ulFlags&TEX_ALPHACHANNEL) ? 32 : 24;

  // prepare the texture for exporting (with or without alpha channel)
  ULONG *pulFrame = (ULONG*)pTD->td_pulFrames + pTD->td_slFrameSize*iFrame/BYTES_PER_TEXEL;
  PIX  pixMipSize = pixWidth*pixHeight;
  SLONG slMipSize = pixMipSize * ii.ii_BitsPerPixel/8;
  ii.ii_Picture = (UBYTE*)AllocMemory( slMipSize);

  // export frame
  if( pTD->td_ulFlags&TEX_ALPHACHANNEL) {
    memcpy( ii.ii_Picture, pulFrame, slMipSize);
  } else {
    RemoveAlphaChannel( pulFrame, ii.ii_Picture, pixMipSize);
  }

  // save tga
  ii.SaveTGA_t(fnOrderFile);
  ii.Clear();
  printf("Create %s image success.\n", (const char *) fnOrderFile); 
  exit( EXIT_SUCCESS);
}


// ---------------- Main ----------------
int main( int argc, char *argv[])
{
  CTSTREAM_BEGIN
  {
    SubMain(argc, argv);
  }
  CTSTREAM_END;
  #ifdef PLATFORM_UNIX
  getchar();
  #else
  getch();
  #endif
  return 0;
}
