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

#include <Engine/Graphics/Font.h>
#include <Engine/Base/Stream.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Templates/Stock_CTextureData.h>


// some default fonts
CFontData *_pfdDisplayFont;
CFontData *_pfdConsoleFont;


// constructor deletes letter data
CFontCharData::CFontCharData(void)
{
  fcd_pixXOffset = 0;
  fcd_pixYOffset = 0;
  fcd_pixStart   = 0;
  fcd_pixEnd   = 0;
}

// simple stream functions
void CFontCharData::Read_t(  CTStream *inFile)
{
  *inFile>>fcd_pixXOffset;
  *inFile>>fcd_pixYOffset;
  *inFile>>fcd_pixStart;
  *inFile>>fcd_pixEnd;
}

void CFontCharData::Write_t( CTStream *outFile)
{
  *outFile<<fcd_pixXOffset;
  *outFile<<fcd_pixYOffset;
  *outFile<<fcd_pixStart;
  *outFile<<fcd_pixEnd;
}

CFontData::CFontData()
{
  fd_ptdTextureData = NULL;
  fd_fnTexture = CTString("");
}

CFontData::~CFontData()
{
  Clear();
}


void CFontData::Clear()
{
  if( fd_ptdTextureData != NULL) {
    fd_fnTexture = CTString("");
    _pTextureStock->Release(fd_ptdTextureData);
    fd_ptdTextureData = NULL;
  }
}

void CFontData::Read_t( CTStream *inFile) // throw char *
{
  // clear current font data (if needed)
  Clear();

  // read the filename of the corresponding texture file.
  inFile->ExpectID_t( CChunkID("FTTF"));
  *inFile >> fd_fnTexture;
  // read maximum width and height of all letters
  *inFile >> fd_pixCharWidth;
  *inFile >> fd_pixCharHeight;
  // read entire letter data table
  for( INDEX iLetterData=0; iLetterData<256; iLetterData ++) {
    fd_fcdFontCharData[ iLetterData].Read_t( inFile);
  }
  // load corresponding texture file
  fd_ptdTextureData = _pTextureStock->Obtain_t(fd_fnTexture);
  // reload corresponding texture if not loaded in largest mip-map
  fd_ptdTextureData->Force( TEX_CONSTANT);

  // initialize default font variables
  SetVariableWidth();
  SetCharSpacing(+1);
  SetLineSpacing(+1);
  SetSpaceWidth(0.5f);
  fd_fcdFontCharData[(int)' '].fcd_pixStart = 0;
}

void CFontData::Write_t( CTStream *outFile) // throw char *
{
  ASSERT( fd_ptdTextureData != NULL);
  // write the filename of the corresponding texture file
  outFile->WriteID_t( CChunkID("FTTF"));
  *outFile << fd_fnTexture;
  // write max letter width and height of all letters
  *outFile << fd_pixCharWidth;
  *outFile << fd_pixCharHeight;

  // write entire letter data table
  for( INDEX iLetterData=0; iLetterData<256; iLetterData ++) {
    fd_fcdFontCharData[ iLetterData].Write_t( outFile);
  }
}

/*
 * Function used for creating font data object
 */
void CFontData::Make_t( const CTFileName &fnTexture, PIX pixCharWidth, PIX pixCharHeight,
                        CTFileName &fnOrderFile, BOOL bUseAlpha)
{
  // do it only if font has not been created already
  ASSERT( fd_ptdTextureData == NULL);

  // remember texture name
  fd_fnTexture = fnTexture;
  // load texture and cache width
  fd_ptdTextureData = _pTextureStock->Obtain_t(fd_fnTexture);
  fd_ptdTextureData->Force( TEX_STATIC|TEX_CONSTANT);
  PIX pixTexWidth = fd_ptdTextureData->GetPixWidth();

  // load ascii order file (no application path necessary)
  CTString strLettersOrder;
  IgnoreApplicationPath();
  strLettersOrder.Load_t( fnOrderFile);
  UseApplicationPath();

  // remember letter width and height
  fd_pixCharWidth  = pixCharWidth;
  fd_pixCharHeight = pixCharHeight;
  // determine address in memory where font definition begins in its larger mip-map
  ULONG *pulFont = fd_ptdTextureData->td_pulFrames;
  ASSERT( pulFont!=NULL);

  // find number of letters in line (assuming that the 1st line represents the width of every line)
  INDEX iLettersInLine=0;
  while( (strLettersOrder[iLettersInLine]!='\n') && iLettersInLine < static_cast<INDEX>(strlen(strLettersOrder))) iLettersInLine++;
  if( iLettersInLine<=0) FatalError( "Invalid font definition ASCII file.");

  // determine pixelcheck mast depending of alpha channel usage
  COLOR colPixMask = 0xFFFFFF00;  // FC is because of small tolerance for black 
  if( bUseAlpha) colPixMask = 0xFFFFFFFF;

  // how much we must add to jump to character down
  PIX pixFontCharModulo = pixTexWidth * fd_pixCharHeight;

  // for all letters in font (ranging from space to last letter that user defined)
  INDEX iLetter=0;
  INDEX iCurrentLetterLine = 0;
  while( iLetter < static_cast<INDEX>(strlen(strLettersOrder)))
  { // for letters in one line
    for( INDEX iCurrentLetterColumn=0; iCurrentLetterColumn<iLettersInLine; iCurrentLetterColumn++)
    { // test if we at the end of whole array
      if( iLetter >= static_cast<INDEX>(strlen(strLettersOrder))) break;
      // get char params
      unsigned char chrLetter = strLettersOrder[iLetter++];
      // reset current letter's width
      PIX pixCurrentStart = fd_pixCharWidth;
      PIX pixCurrentEnd   = 0;
      // for all of the letter's lines
      for( INDEX iPixelLine=0; iPixelLine<fd_pixCharHeight; iPixelLine++)
      { // for all of the letter's pixels
        for( INDEX iPixelColumn=0; iPixelColumn<fd_pixCharWidth; iPixelColumn++)
        { // calculate current pixel's adress in font's texture
          ULONG *puwPixel = (ULONG*)( pulFont + pixFontCharModulo * iCurrentLetterLine +  // calc right letter line
                                      fd_pixCharWidth * iCurrentLetterColumn +            // move to right letter column
                                      pixTexWidth * iPixelLine +   // move trough pixel lines of one letter
                                      iPixelColumn);               // move trough pixel columns of one letter
          // if we test alpha channel and alpha value is not 0
          if( ByteSwap(*puwPixel) & colPixMask) {
            // if examined pixel is narrower in letter than last opaque pixel found, remember it as left-most pixel
            if( iPixelColumn < pixCurrentStart) pixCurrentStart = iPixelColumn;
            // if examined pixel is wider in letter than last opaque pixel found, remember it as right-most pixel
            if( iPixelColumn > pixCurrentEnd) pixCurrentEnd = iPixelColumn;
          }
        }
      }
      // letter's data is stored into table on appropriate place
      fd_fcdFontCharData[chrLetter].fcd_pixXOffset = iCurrentLetterColumn * fd_pixCharWidth;
      fd_fcdFontCharData[chrLetter].fcd_pixYOffset = iCurrentLetterLine   * fd_pixCharHeight;
      fd_fcdFontCharData[chrLetter].fcd_pixStart   = pixCurrentStart;
      fd_fcdFontCharData[chrLetter].fcd_pixEnd     = pixCurrentEnd +1;
    }
    // advance to next line in text file
    iCurrentLetterLine++;
    iLetter++;  // skip carriage return
  }
  // set default space width
  fd_fcdFontCharData[(int)' '].fcd_pixStart = 0;
  SetSpaceWidth(0.5f);

  // all done
  SetVariableWidth();
  _pTextureStock->Release( fd_ptdTextureData);
}

