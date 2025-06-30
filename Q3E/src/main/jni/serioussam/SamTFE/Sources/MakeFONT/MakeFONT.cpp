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


// MakeFONT - Font table File Creator

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../Engine/Engine.h"

// Not used; dummy declaration only needed by
// Engine/Base/ErrorReporting.o
HWND _hwndMain = NULL;

void SubMain( int argc, char *argv[])
{
  printf("\nMakeFONT - Font Tables Maker (2.51)\n");
  printf(  "           (C)1999 CROTEAM Ltd\n\n");
  // 5 to 7 parameters are allowed as input
  if( (argc<5) || (argc>6))
  {
    printf( "USAGE: MakeFont <texture_file> <char_width> <char_height> ");
    printf( "<char_order_file> [-A]\n");
    printf( "\n");
    printf( "texture_file: FULL PATH to texture file that represents font\n");
    printf( "char_width: maximum width (in pixels) of single character\n");
    printf( "char_height: maximum height (in pixels) of single character\n");
    printf( "char_order_file: full path to ASCII file that shows\n");
    printf( "                 graphical order of character in font texture\n");
    printf( "-A: do not include alpha channel when determining character width \n");
    printf( "\n");
    printf( "NOTES: - out file will have the name as texture file, but \".fnt\" extension\n");
    printf( "       - texture file must begin with character that will be a replacement for\n");
    printf( "         each character that hasn't got definition in this texture file\n");
    exit( EXIT_FAILURE);
  }

  // initialize engine
#ifdef PLATFORM_UNIX
  SE_InitEngine("","");
#else
  SE_InitEngine("");
#endif
  // first input parameter is texture name
  CTFileName fnTextureFileName = CTString(argv[1]);
  // parameters 2 and 3 give us character dimensions
  ULONG ulCharWidth = strtoul( argv[2], NULL, 10);
  ULONG ulCharHeight= strtoul( argv[3], NULL, 10);
  // parameter 4 specifies text file for character arrangements
  CTFileName fnOrderFile = CTString(argv[4]);

  // alpha channel ignore check
  BOOL bUseAlpha = TRUE;
  if( argc==6 && (argv[5][1]=='a' || argv[5][1]=='A')) bUseAlpha = FALSE;

  // font generation starts
  printf( "- Generating font table.\n");
  // try to create font
  CFontData fdFontData;
  try
  { 
    // remove application path from font texture file
    fnTextureFileName.RemoveApplicationPath_t();
    // create font
    fdFontData.Make_t( fnTextureFileName, ulCharWidth, ulCharHeight, fnOrderFile, bUseAlpha);
  }
  // catch and report errors
  catch (const char *strError)
  {
    printf( "! Cannot create font.\n  %s\n", (const char *) strError);
    exit(EXIT_FAILURE);
  }
  
  // save processed data
  printf( "- Saving font table file.\n");
  // create font name
  CTFileName strFontFileName;
  strFontFileName = fnTextureFileName.FileDir()+fnTextureFileName.FileName() + ".fnt";
  // try to
  try
  {
    fdFontData.Save_t( strFontFileName);
  }
  catch (const char *strError)
  {
    printf("! Cannot save font.\n  %s\n", (const char *) strError);
    exit(EXIT_FAILURE);
  }
  printf( "- '%s' created successfuly.\n", (const char *) strFontFileName);
  
  exit( EXIT_SUCCESS);
}


// ---------------- Main
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
