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

// DecodeReport.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

void SubMain( int argc, char *argv[]);

int main( int argc, char *argv[])
{
  CTSTREAM_BEGIN {
    SubMain(argc, argv);
  } CTSTREAM_END;
  return 0;
}

void FindInMapFile(const CTFileName &fnSymbols, const CTString &strImage, ULONG ulSeg, ULONG ulOff, CTString &strFunction, SLONG &slDelta)
{
  CTFileName fnmImage = strImage;
  CTFileName fnmMap = fnSymbols+fnmImage.FileName()+".map";
  strFunction = CTString("<not found in '")+fnmMap+"'>";

  try {
    CTFileStream strmMap;
    strmMap.Open_t(fnmMap, CTStream::OM_READ);
    // find beginning of functions in map file
    for(;;) {
      if (strmMap.AtEOF()) {
        return;
      }
      // read the line
      CTString strLine;
      strmMap.GetLine_t(strLine);
      if (strncmp(strLine, "  Address", 9)==0) {
        break;
      }
    }

    CTString strEmpty;
    strmMap.GetLine_t(strEmpty);

    while (!strmMap.AtEOF()) {
      // read the line
      CTString strLine;
      strmMap.GetLine_t(strLine);
      char strFunctionLine[1024];
      strFunctionLine[0]=0;
      ULONG ulSegLine=-1;
      ULONG ulOfsLine=-1;
      strLine.ScanF("%x:%x %s", &ulSegLine, &ulOfsLine, strFunctionLine);
      if (ulSegLine!=ulSeg) {
        continue;
      }
      if (ulOfsLine>ulOff) {
        return;
      }
      strFunction = strFunctionLine;
      slDelta = ulOff-ulOfsLine;
    }

  } catch (char *strError) {
    (void)strError;
    return;
  }
}

void SubMain( int argc, char *argv[])
{
  printf("\nDecodeReport - '.RPT' file decoder V1.0\n");
  printf(  "           (C)1999 CROTEAM Ltd\n\n");

  if( argc!=3+1)
  {
    printf( "USAGE:\nDecodeReport <infilename> <outfilename> <symbolsdir>\n");
    exit( EXIT_FAILURE);
  }
  
  // initialize engine
  SE_InitEngine("");
  _fnmApplicationPath = CTString("");


  CTFileName fnSrc = CTString(argv[1]);
  CTFileName fnDst = CTString(argv[2]);
  CTFileName fnSymbols = CTString(argv[3]);

  try
  {
    if (fnSrc==fnDst) {
      throw "Use different files!";
    }

    CTFileStream strmSrc;
    strmSrc.Open_t(fnSrc, CTStream::OM_READ);
    CTFileStream strmDst;
    strmDst.Create_t(fnDst);
    
    // while there is some line in src
    while(!strmSrc.AtEOF()) {
      // read the line
      CTString strLine;
      strmSrc.GetLine_t(strLine);

      // try to find address marker in it
      const char *strAdr = strstr(strLine, "$adr:");
      // if there is no marker
      if (strAdr==NULL) {
        // just copy the line
        strmDst.PutLine_t(strLine);

      // if there is marker
      } else {
        // parse the line
        char strImage[1024];
        strImage[0]=0;
        ULONG ulSegment=-1;
        ULONG ulOffset=-1;
        sscanf(strAdr, "$adr: %s %x:%x", strImage, &ulSegment, &ulOffset);
        // find the function
        CTString strFunction;
        SLONG slDelta;
        FindInMapFile(fnSymbols, CTString(strImage), ulSegment, ulOffset, strFunction, slDelta);
        // out put the result
        CTString strResult;
        strResult.PrintF("%s (%s+0X%X)", strLine, strFunction, slDelta);
        strmDst.PutLine_t(strResult);
      }
    }
  }
  catch(char *strError)
  {
    printf("\nError: %s\n", strError);
    exit(EXIT_FAILURE);
  }
}

