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

#ifndef SE_INCL_IMAGEINFO_H
#define SE_INCL_IMAGEINFO_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

/*
 * class of CroTeam RAW image description (bitmap) and several image manipulation routines
 */

#define UNSUPPORTED_FILE 0L
#define PCX_FILE         1L
#define TGA_FILE         2L

class ENGINE_API CImageInfo {
public:
  PIX    ii_Width;	           // width of image in pixels
  PIX    ii_Height;	           // height of image in pixels
  SLONG  ii_BitsPerPixel;      // depth of image (color bits; 24 or 32=24+8 alpha)
  UBYTE *ii_Picture; 	         // pointer to picture contents (raw bytes)
private:
  void LoadTGA_t( const CTFileName &strFileName); // throw char *
  void LoadPCX_t( const CTFileName &strFileName); // throw char *
public:
  // constructor and destructor are must have
   CImageInfo();
  ~CImageInfo();
  // reads image info raw format from file
  void Read_t( CTStream *inFile); // throw char *
  // writes image info raw format to file
  void Write_t( CTStream *outFile) const; // throw char *

  // initializes structure members and attaches pointer to image
  void Attach( UBYTE *pPicture, PIX pixWidth, PIX pixHeight, SLONG slBitsPerPixel);
  // clears the content of an image info structure but does not free allocated memory
  void Detach(void);
  // clears the content of an image info structure and frees allocated memory
  void Clear(void);
  // expand image edges
  void ExpandEdges( INDEX ctPasses=8192);

  // sets image info structure members with info form file of any supported graphic format,
  //  but does not load picture content nor palette; returns format type (see #defines)
  //  (supported formats: CroTeam's RAW, PCX24, TGA32 uncompressed)
  INDEX GetGfxFileInfo_t( const CTFileName &strFileName); // throw char *
  // sets image info structure members with info form file of any supported graphic format
  //  and loads picture content and, eventually, palette
  void LoadAnyGfxFormat_t( const CTFileName &strFileName); // throw char *

  // converts image info structure and content to PCX or TGA format and saves it to file
  void SaveTGA_t( const CTFileName &strFileName) const; // throw char *
};


#endif  /* include-once check. */

