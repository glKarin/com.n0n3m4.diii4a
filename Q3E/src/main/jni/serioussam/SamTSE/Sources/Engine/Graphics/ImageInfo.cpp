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

#include <Engine/Graphics/ImageInfo.h>
#include <Engine/Graphics/Color.h>

#include <Engine/Base/Stream.h>
#include <Engine/Base/Memory.h>
#include <Engine/Math/Functions.h>

extern void FlipBitmap( UBYTE *pubSrc, UBYTE *pubDst, PIX pixWidth, PIX pixHeight, INDEX iFlipType, BOOL bAlphaChannel);


// Order of CroTeam true color pixel components
#define COMPONENT_1 red
#define COMPONENT_2 green
#define COMPONENT_3 blue
#define COMPONENT_4 alpha

// and vice versa
#define RED_COMPONENT   0
#define GREEN_COMPONENT 1
#define BLUE_COMPONENT  2
#define ALPHA_COMPONENT 3


// PCX header structure
struct PCXHeader
{
  SBYTE MagicID;
  SBYTE Version;
  SBYTE Encoding;
  SBYTE PixelBits;
  SWORD Xmin, Ymin, Xmax, Ymax;
  SWORD Hres, Vres;
  UBYTE Palette[16*3];
  SBYTE Reserved;
  SBYTE Planes;
  UWORD BytesPerLine;
  SWORD PaletteInfo;
  SWORD	HscreenSize, VscreenSize;
  SBYTE Filler[54];
};

static __forceinline CTStream &operator>>(CTStream &strm, PCXHeader &t) {
  strm>>t.MagicID;
  strm>>t.Version;
  strm>>t.Encoding;
  strm>>t.PixelBits;
  strm>>t.Xmin;
  strm>>t.Ymin;
  strm>>t.Xmax;
  strm>>t.Ymax;
  strm>>t.Hres;
  strm>>t.Vres;
  strm.Read_t(t.Palette, sizeof (t.Palette));
  strm>>t.Reserved;
  strm>>t.Planes;
  strm>>t.BytesPerLine;
  strm>>t.PaletteInfo;
  strm>>t.HscreenSize;
  strm>>t.VscreenSize;
  strm.Read_t(t.Filler, sizeof (t.Filler));
  return strm;
}

#if 0 // DG: unused.
static __forceinline CTStream &operator<<(CTStream &strm, const PCXHeader &t) {
  strm<<t.MagicID;
  strm<<t.Version;
  strm<<t.Encoding;
  strm<<t.PixelBits;
  strm<<t.Xmin;
  strm<<t.Ymin;
  strm<<t.Xmax;
  strm<<t.Ymax;
  strm<<t.Hres;
  strm<<t.Vres;
  strm.Write_t(t.Palette, sizeof (t.Palette));
  strm<<t.Reserved;
  strm<<t.Planes;
  strm<<t.BytesPerLine;
  strm<<t.PaletteInfo;
  strm<<t.HscreenSize;
  strm<<t.VscreenSize;
  strm.Write_t(t.Filler, sizeof (t.Filler));
  return strm;
}
#endif // 0

// TARGA header structure
struct TGAHeader
{
  UBYTE IdLength;
  UBYTE ColorMapType;
  UBYTE ImageType;
  UBYTE ColorMapSpec[5]; // #### 5 ... strm>>t.ColorMapSpec[5]; ????
  UWORD Xorigin;
  UWORD Yorigin;
  UWORD	Width;
  UWORD Height;
  UBYTE BitsPerPixel;
  UBYTE Descriptor;
};

static __forceinline CTStream &operator>>(CTStream &strm, TGAHeader &t) {
  strm>>t.IdLength;
  strm>>t.ColorMapType;
  strm>>t.ImageType;
  //strm>>t.ColorMapSpec[5];  // #### not correct 
  strm.Read_t(t.ColorMapSpec, sizeof (t.ColorMapSpec));
  strm>>t.Xorigin;
  strm>>t.Yorigin;
  strm>>t.Width;
  strm>>t.Height;
  strm>>t.BitsPerPixel;
  strm>>t.Descriptor;
  return(strm);
}

#if 0 // DG: unused.
static __forceinline CTStream &operator<<(CTStream &strm, const TGAHeader &t) {
  strm<<t.IdLength;
  strm<<t.ColorMapType;
  strm<<t.ImageType;
  strm<<t.ColorMapSpec[5];
  strm<<t.Xorigin;
  strm<<t.Yorigin;
  strm<<t.Width;
  strm<<t.Height;
  strm<<t.BitsPerPixel;
  strm<<t.Descriptor;
  return(strm);
}
#endif // 0 (unused)


/******************************************************
 * Routines for manipulating CroTeam picture raw format
 */

CImageInfo::CImageInfo() {
  Detach();
}
CImageInfo::~CImageInfo() {
  Clear();
}

// reads image info raw format from file
void CImageInfo::Read_t( CTStream *inFile)   // throw char *
{
  Clear();

  // read image info header
  inFile->ExpectID_t( CChunkID("CTII"));
  if( inFile->GetSize_t() != 5*4) throw( "Invalid image info file.");

  SLONG tmp;
  *inFile >> tmp;
  ii_Width = (PIX) tmp;
  *inFile >> tmp;
  ii_Height = (PIX) tmp;
  *inFile >> tmp;
  ii_BitsPerPixel = (SLONG) tmp;

  // read image contents (all channels)
  ULONG pic_size = ii_Width*ii_Height * ii_BitsPerPixel/8;
  ii_Picture = (UBYTE*)AllocMemory( pic_size);
  inFile->ReadFullChunk_t( CChunkID("IPIC"), ii_Picture, pic_size);

  #if PLATFORM_BIGENDIAN
  STUBBED("Byte order");
  #endif
}

// writes image info raw format to file
void CImageInfo::Write_t( CTStream *outFile) const  // throw char *
{
  // write image info header
  outFile->WriteID_t( CChunkID("CTII"));
  outFile->WriteSize_t( 5*4);

  *outFile << (PIX)ii_Width;
  *outFile << (PIX)ii_Height;
  *outFile << (SLONG)ii_BitsPerPixel;

  // write image contents (all channels)
  ULONG pic_size = ii_Width*ii_Height * ii_BitsPerPixel/8;
  outFile->WriteFullChunk_t( CChunkID("IPIC"), ii_Picture, pic_size);

  #if PLATFORM_BIGENDIAN
  STUBBED("Byte order");
  #endif
}


// initializes structure members and attaches pointer to image
void CImageInfo::Attach( UBYTE *pPicture, PIX pixWidth, PIX pixHeight, SLONG slBitsPerPixel)
{
  // parameters must be meaningful
  ASSERT( (pPicture != NULL) && (pixWidth>0) && (pixHeight>0));
  ASSERT( (slBitsPerPixel == 24) || (slBitsPerPixel == 32));
  // do it ...
  ii_Picture = pPicture;
  ii_Width   = pixWidth;
  ii_Height  = pixHeight;
  ii_BitsPerPixel = slBitsPerPixel;
}

// clears the content of an image info structure but does not free allocated memory
void CImageInfo::Detach(void)
{
  ii_Picture = NULL;
  ii_Width   = 0;
  ii_Height  = 0;
  ii_BitsPerPixel = 0;
}


// clears the content of an image info structure and frees allocated memory (if any)
void CImageInfo::Clear()
{
  // if allocated, release picture memory
  if( ii_Picture != NULL) FreeMemory( ii_Picture);
  Detach();
}


// expand image edges
void CImageInfo::ExpandEdges( INDEX ctPasses/*=8192*/)
{
  // do nothing if image is too small or doesn't have an alpha channel
  if( ii_Width<3 || ii_Height<3 || ii_BitsPerPixel!=32) return;

  // allocate some memory for spare picture and wipe it clean
  SLONG slSize = ii_Width*ii_Height*ii_BitsPerPixel/8;
  ULONG *pulSrc = (ULONG*)ii_Picture;
  ULONG *pulDst = (ULONG*)AllocMemory(slSize);
  memcpy( pulDst, pulSrc, slSize);

  // loop while there are some more pixels to be processed or for specified number of passes
  for( INDEX iPass=0; iPass<ctPasses; iPass++)
  { 
    BOOL bAllPixelsVisible = TRUE;
    // loop thru rows
    for( PIX pixV=1; pixV<ii_Height-1; pixV++)
    { // loop thru pixels in row
      for( PIX pixU=1; pixU<ii_Width-1; pixU++)
      { // determine pixel location
        const PIX pixOffset = pixV*ii_Width + pixU;
        // do nothing if it is already visible
        COLOR col = ByteSwap(pulSrc[pixOffset]);
        if( ((col&CT_AMASK)>>CT_ASHIFT)>3) continue;
        bAllPixelsVisible = FALSE;
        // average all surrounding pixels that are visible
        ULONG ulRa=0, ulGa=0, ulBa=0;
        INDEX ctVisible=0;
        for( INDEX j=-1; j<=1; j++) {
          for( INDEX i=-1; i<=1; i++) {
            const PIX pixSurrOffset = pixOffset + j*ii_Width + i;
            col = ByteSwap(pulSrc[pixSurrOffset]);
            if( ((col&CT_AMASK)>>CT_ASHIFT)<4) continue; // skip non-visible pixels
            UBYTE ubR, ubG, ubB;
            ColorToRGB( col, ubR,ubG,ubB);
            ulRa+=ubR;  ulGa+=ubG;  ulBa += ubB;
            ctVisible++;
          }
        } // if there were some visible pixels around
        if( ctVisible>0) {
          // calc average
          ulRa/=ctVisible;  ulGa/=ctVisible;  ulBa/=ctVisible;
          col = RGBAToColor( ulRa,ulGa,ulBa,255);
          // put it to center pixel
          pulDst[pixOffset] = ByteSwap(col);
        }
      }
    } // copy resulting picture over source
    memcpy( pulSrc, pulDst, slSize);
    // done if all pixels are visible
    if( bAllPixelsVisible) break;
  }
  // free temp memory
  FreeMemory(pulDst);
}



// sets image info structure members with info form file of any supported graphic format
//  (CT RAW, PCX8, PCX24, TGA32 uncompressed), but does not load picture content nor palette
INDEX CImageInfo::GetGfxFileInfo_t( const CTFileName &strFileName) // throw char *
{
  CTFileStream GfxFile;
  TGAHeader TGAhdr;
  PCXHeader PCXhdr;

  // lets assume it's a TGA file
  GfxFile.Open_t( strFileName, CTStream::OM_READ);
  GfxFile>>TGAhdr;
  GfxFile.Close();

  // check for supported targa format
  if( (TGAhdr.ImageType==2 || TGAhdr.ImageType==10) && TGAhdr.BitsPerPixel>=24) {
    // targa it is, so clear image info and set new values
    Clear();
    ii_Width  = TGAhdr.Width;
    ii_Height = TGAhdr.Height;
    ii_BitsPerPixel = TGAhdr.BitsPerPixel;
    // we done here, no need to check further
    return TGA_FILE;
  }

  // we miss Targa, so lets check for supported PCX format
  GfxFile.Open_t( strFileName, CTStream::OM_READ);
  GfxFile>>PCXhdr;
  GfxFile.Close();

  // check for supported PCX format
  if( (PCXhdr.MagicID == 10) && (PCXhdr.PixelBits == 8)) {
    // PCX it is, so clear image info and set new values
    Clear();
    ii_Width = PCXhdr.Xmax - PCXhdr.Xmin + 1;
    ii_Height = PCXhdr.Ymax - PCXhdr.Ymin + 1;
    ii_BitsPerPixel = PCXhdr.PixelBits * PCXhdr.Planes;
    // we done here, no need to check further
    return PCX_FILE;
  }

  // we didn't found a supported gfx format, sorry ...
  return UNSUPPORTED_FILE;
}



/* TGA *********************************************************************************
 * Routines that load and save true color (24 or 32 bit per pixel) uncompressed targa file
 */



void CImageInfo::LoadTGA_t( const CTFileName &strFileName) // throw char *
{
  TGAHeader *pTGAHdr;
  UBYTE *pTGABuffer, *pTGAImage;
  SLONG slFileSize;
  CTFileStream TGAFile;

  Clear();

  // determine file size
  TGAFile.Open_t( strFileName, CTStream::OM_READ);
  slFileSize = TGAFile.GetStreamSize();

  // load entire TGA file to memory, as is, and close it (no further usage)
  pTGABuffer = (UBYTE*)AllocMemory( slFileSize);
STUBBED("Byte swapping TGA data");

  TGAFile.Read_t( pTGABuffer, slFileSize);
  TGAFile.Close();

  // TGA header starts at the begining of the TGA file
  pTGAHdr = (struct TGAHeader*)pTGABuffer;
  // TGA image bytes definition follows up
  pTGAImage = pTGABuffer + sizeof(struct TGAHeader) + pTGAHdr->IdLength;

  // detremine picture size dimensions
  ii_Width        = (SLONG)pTGAHdr->Width;
  ii_Height       = (SLONG)pTGAHdr->Height;
  ii_BitsPerPixel = (SLONG)pTGAHdr->BitsPerPixel;
  SLONG slBytesPerPixel = ii_BitsPerPixel/8;
  PIX pixBitmapSize     = ii_Width*ii_Height;
  BOOL bAlphaChannel    = (slBytesPerPixel==4);

  // check for supported file types
  ASSERT( slBytesPerPixel==3 || slBytesPerPixel==4);
  if( slBytesPerPixel!=3 && slBytesPerPixel!=4) throw( TRANS("Unsupported BitsPerPixel in TGA format."));

  // allocate memory for image content
  ii_Picture = (UBYTE*)AllocMemory( ii_Width*ii_Height *slBytesPerPixel);
  UBYTE *pubSrc = pTGAImage;  // need 'walking' pointers
  UBYTE *pubDst = ii_Picture;

  // determine TGA image type
  if( pTGAHdr->ImageType==10) {
    // RLE encoded
    UBYTE ubControl;
    INDEX iBlockSize;
    BOOL  bRepeat;
    PIX pixCurrentSize=0;
    // loop thru blocks
    while( pixCurrentSize<pixBitmapSize)
    { // readout control byte
      ubControl  = *pubSrc++;
      bRepeat    =  ubControl&0x80;
      iBlockSize = (ubControl&0x7F) +1;
      // repeat or copy color values
      for( INDEX i=0; i<iBlockSize; i++) {
        *pubDst++ = pubSrc[0]; 
        *pubDst++ = pubSrc[1]; 
        *pubDst++ = pubSrc[2]; 
        if( bAlphaChannel) *pubDst++ = pubSrc[3];
        if( !bRepeat) pubSrc += slBytesPerPixel;
      }
      // advance for next block if repeated
      if( bRepeat) pubSrc += slBytesPerPixel;
      // update image size
      pixCurrentSize += iBlockSize;
    }
    // mark that image was encoded to ImageInfo buffer
    pTGAImage = ii_Picture; 
  } 
  // not true-colored?
  else if( pTGAHdr->ImageType!=2) {
    // whoops!
    ASSERTALWAYS("Unsupported TGA format.");
    throw( TRANS("Unsupported TGA format."));
  }

  // determine image flipping
  INDEX iFlipType;
  switch( (pTGAHdr->Descriptor&0x30)>>4) {
  case 0:  iFlipType = 1;  break; // vertical flipping
  case 1:  iFlipType = 3;  break; // diagonal flipping
  case 3:  iFlipType = 2;  break; // horizontal flipping
  default: iFlipType = 0;  break; // no flipping (just copying)
  }
  // do flipping
  FlipBitmap( pTGAImage, ii_Picture, ii_Width, ii_Height, iFlipType, bAlphaChannel);

  // convert TGA pixel format to CroTeam
  pubSrc = ii_Picture;  // need 'walking' pointer again
  for( INDEX iPix=0; iPix<pixBitmapSize; iPix++)
  { // flip bytes
    Swap( pubSrc[0], pubSrc[2]);  // R & B channels
    pubSrc += slBytesPerPixel; 
  }

  // free temorary allocated memory for TGA image format
  FreeMemory( pTGABuffer);
}


// save TGA routine
void CImageInfo::SaveTGA_t( const CTFileName &strFileName) const // throw char *
{
  TGAHeader *pTGAHdr;
  UBYTE *pTGABuffer, *pTGAImage;
  SLONG slFileSize;
  PIX pixBitmapSize = ii_Width*ii_Height;
  CTFileStream TGAFile;

  // determine and check image info format
  SLONG slBytesPerPixel = ii_BitsPerPixel/8;
  ASSERT( slBytesPerPixel==3 || slBytesPerPixel==4);
  if( slBytesPerPixel!=3 && slBytesPerPixel!=4) throw( TRANS( "Unsupported BitsPerPixel in ImageInfo header."));

  // determine TGA file size and allocate memory
  slFileSize = sizeof(struct TGAHeader) + pixBitmapSize *slBytesPerPixel;
  pTGABuffer = (UBYTE*)AllocMemory( slFileSize);
  pTGAHdr    = (struct TGAHeader*)pTGABuffer;
  pTGAImage  = pTGABuffer + sizeof(struct TGAHeader);

  // set TGA picture size dimensions
  memset( pTGABuffer, 0x0, sizeof(struct TGAHeader));
  pTGAHdr->Width        = (UWORD)ii_Width;
  pTGAHdr->Height       = (UWORD)ii_Height;
  pTGAHdr->BitsPerPixel = (UBYTE)ii_BitsPerPixel;
  pTGAHdr->ImageType    = 2;

  // flip image vertically
  BOOL bAlphaChannel = (slBytesPerPixel==4);
  FlipBitmap( ii_Picture, pTGAImage, ii_Width, ii_Height, 1, bAlphaChannel);

  // convert CroTeam's pixel format to TGA format
  UBYTE *pubTmp = pTGAImage;  // need 'walking' pointer
  for( INDEX iPix=0; iPix<pixBitmapSize; iPix++)
  { // flip bytes
    Swap( pubTmp[0], pubTmp[2]);  // R & B channels
    pubTmp += slBytesPerPixel; 
  }

  // save entire TGA memory to file and close it
  TGAFile.Create_t( strFileName);
  TGAFile.Write_t( pTGABuffer, slFileSize);
  TGAFile.Close();

  // free temorary allocated memory for TGA image format
  FreeMemory( pTGABuffer);
}


/* PCX ***********************************************************************
 * This routine reads file with given file name and if it is valid PCX file it
 * loads it into given ImageInfo structure in CroTeam true-color format.
 * (and, if the one exists, loads the palette)
 */
void CImageInfo::LoadPCX_t( const CTFileName &strFileName) // throw char *
{
  PCXHeader *pPCXHdr;
  UBYTE *pPCXBuffer, *pPCXImage, *pPCXDecodedImage, *pTmp;
  UBYTE data, counter;
  SLONG pic_size, PCX_size, slFileSize;
  CTFileStream PCXFile;

  Clear();

  // inconvinent way to determine file size
  PCXFile.Open_t( strFileName, CTStream::OM_READ);
  slFileSize = PCXFile.GetStreamSize();

  // load entire PCX file to memory, as is, and close it (no further usage)
  pPCXBuffer = (UBYTE*)AllocMemory( slFileSize);
STUBBED("Byte swapping PCX data");
  PCXFile.Read_t( pPCXBuffer, slFileSize);
  PCXFile.Close();

  // PCX header starts at the begining of the PCX file
  pPCXHdr = (struct PCXHeader*)pPCXBuffer;
  // PCX image bytes definition follows up
  pPCXImage = pPCXBuffer + sizeof( struct PCXHeader);

  // detremine picture size dimensions
  ii_Width  = (SLONG)(pPCXHdr->Xmax - pPCXHdr->Xmin +1);
  ii_Height = (SLONG)(pPCXHdr->Ymax - pPCXHdr->Ymin +1);
  ii_BitsPerPixel = (SLONG)pPCXHdr->Planes*8;
  pic_size = ii_Width * ii_Height * ii_BitsPerPixel/8;

  // allocate memory for image content
  ii_Picture = (UBYTE*)AllocMemory( pic_size);

  // allocate memory for decoded PCX file that hasn't been converted to CT RAW Image format
  PCX_size = (SLONG)(pPCXHdr->BytesPerLine * ii_Height * ii_BitsPerPixel/8);
  pPCXDecodedImage = (UBYTE*)AllocMemory( PCX_size);
  pTmp = pPCXDecodedImage;  // save pointer for latter usage

  // decode PCX file
  for( INDEX i=0; i<PCX_size; )   // i is incremented by counter value  at the and of the loop
  {
    // read one byte from PCX image in memory
    data = *pPCXImage++;
    // check byte-run mark
    if( (data & 0xC0) == 0xC0) {
      counter = data & 0x3F;              // determine repeat value
      data = *pPCXImage++;                // read repeated data
      // put several bytes of PCX image to decoded image area in memory
      for( INDEX j=0; j<counter; j++)
        *pPCXDecodedImage++ = data;
    } else {
      // put just one byte from PCX image to decoded image area in memory
      counter = 1;
      *pPCXDecodedImage++ = data;
    }

    // increment encoded image counter
    i += counter;
  }
  pPCXDecodedImage = pTmp;  // reset pointer

  // convert decoded PCX image to CroTeam RAW Image Info format
  SLONG slBytesPerPixel = ii_BitsPerPixel/8;
  for( INDEX y=0; y<ii_Height; y++)
  {
    SLONG slYSrcOfs = y * ii_Width * slBytesPerPixel;
    SLONG slYDstOfs = y * pPCXHdr->BytesPerLine * slBytesPerPixel;
    // channel looper
    for( INDEX p=0; p<slBytesPerPixel; p++)
    {
      SLONG slPOffset = p * pPCXHdr->BytesPerLine;
      // byte looper
      for( INDEX x=0; x<ii_Width; x++)
        *(ii_Picture + slYSrcOfs + x*slBytesPerPixel + p) =
        *(pPCXDecodedImage + slYDstOfs + slPOffset + x);
    }
  }

  // free temorary allocated memory for PCX encoded and decoded image
  FreeMemory( pPCXBuffer);
  FreeMemory( pPCXDecodedImage);
}


// check for the supported gfx format file and invokes corresponding routine to load it
void CImageInfo::LoadAnyGfxFormat_t( const CTFileName &strFileName) // throw char *
{
  INDEX iFileFormat = GetGfxFileInfo_t( strFileName);
  if( iFileFormat == PCX_FILE) LoadPCX_t( strFileName);
  if( iFileFormat == TGA_FILE) LoadTGA_t( strFileName);
  if( iFileFormat == UNSUPPORTED_FILE) throw( "Gfx format not supported.");
}
