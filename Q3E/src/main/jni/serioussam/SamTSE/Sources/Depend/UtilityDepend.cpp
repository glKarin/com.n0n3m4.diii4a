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

#include "StdH.h"
#include "Dependency.h"

// Depend - extract dependencies and create group file utility

#define ACHR_OPTION        argv[1]
#define ACHR_APP_DIR       argv[2]

void PrintUsage()
{
  printf( 
  "\nUSAGE:\n"
  "Depend d <application dir> <file 1> <file 2> <output file>\n"
  "  make difference beetween two dependency files (result = file 1 - file 2)\n"
  "Depend i <application dir> <lst file> <dep file>\n"
  "  create dependencies for files listed in given list file\n"
  "Depend u <application dir> <in dep file> <out dep file>\n"
  "  remove updated files from dependency file\n"
  "Depend e <application dir> <dep file> <lst file>\n"
  "  export dependency file to ascii list file\n"
  "Depend t <application dir> <lst file> <translation file>\n"
  "  export strings for translation\n"
  "\n");
  exit( EXIT_FAILURE);
}

void SubMain( int argc, char *argv[]);

int main( int argc, char *argv[])
{
  CTSTREAM_BEGIN {
    SubMain(argc, argv);
  } CTSTREAM_END;
  return 0;
}

void SubMain( int argc, char *argv[])
{
  // there must be 4 or 6 parameters (first is depend.exe file name)
  if( (argc < 4) || (argc > 6) )
  {
    PrintUsage();
  }
  
  // initialize engine
  SE_InitEngine("");
  // get application path from cmd line
  _fnmApplicationPath = CTString(ACHR_APP_DIR);
  // if not ending with backslash
  if (_fnmApplicationPath[strlen(_fnmApplicationPath)-1]!='\\') {
    _fnmApplicationPath += "\\";
  }

  // get all filenames from command line
  CTFileName afnFiles[3];
  INDEX ctFiles = argc-3;
  if (ctFiles>ARRAYCOUNT(afnFiles)) {
    PrintUsage();
  }
  for (INDEX iFile=0; iFile<ctFiles; iFile++) {
    afnFiles[iFile] = CTString(argv[iFile+3]);
  }

  // lenght of options string must be 1
  if( strlen( ACHR_OPTION) != 1) {
    printf( "First argument must be letter representing wanted option.\n\n");
    PrintUsage();
  }

  // try to
  try {
    // remove application paths
    for (INDEX iFile=0; iFile<ctFiles; iFile++) {
      AdjustFilePath_t(afnFiles[iFile]);
    }

    // see what option was requested
    switch( tolower(ACHR_OPTION[0]) ) {
    case 'i': {
      if( ctFiles != 2) PrintUsage();
      CDependencyList dl;
      CTFileStream strmDep;

      // import files into dependency list from given ascii file
      dl.ImportASCII( afnFiles[0]);
      // extract dependencies
      dl.ExtractDependencies();
      // write dependency list
      strmDep.Create_t( afnFiles[1], CTStream::CM_BINARY);
      dl.Write_t( &strmDep);
              } break;
    case 'e': {
      if( ctFiles != 2) PrintUsage();
      CDependencyList dl;
      CTFileStream strmDepIn;

      // read dependency list
  	  strmDepIn.Open_t( afnFiles[0], CTStream::OM_READ);
      dl.Read_t( &strmDepIn);
      strmDepIn.Close();
      // export file suitable for archivers
      dl.ExportASCII_t( afnFiles[1]);
              } break;
    case 'u': {
      if( ctFiles != 2) PrintUsage();
      CDependencyList dl;
      CTFileStream strmDepIn, strmDepOut;

      // read dependency list
  	  strmDepIn.Open_t( afnFiles[0], CTStream::OM_READ);
      dl.Read_t( &strmDepIn);
      strmDepIn.Close();
      // remove updated files from list
      dl.RemoveUpdatedFiles();
      // write dependency list
      strmDepOut.Create_t( afnFiles[1], CTStream::CM_BINARY);
      dl.Write_t( &strmDepOut);
              } break;
    case 'd': {   // UNTESTED!!!!
      if( ctFiles != 3) PrintUsage();
      // load dependency lists
      CDependencyList dl1, dl2;
      CTFileStream strmDep1, strmDep2, strmDepDiff;
  	  strmDep1.Open_t( afnFiles[0], CTStream::OM_READ);
  	  strmDep2.Open_t( afnFiles[1], CTStream::OM_READ);
      dl1.Read_t( &strmDep1);
      dl2.Read_t( &strmDep2);
      strmDep1.Close();
      strmDep2.Close();
      // calculate difference dependency 1 and 2 (res = 1-2)
      dl1.Substract( dl2);
      // save the difference dependency list
      strmDepDiff.Create_t( afnFiles[2], CTStream::CM_BINARY);
      dl1.Write_t( &strmDepDiff);
              } break;
    case 't': {
      if( ctFiles != 2) PrintUsage();
      CDependencyList dl;
      // read file list
      dl.ImportASCII( afnFiles[0]);
      // extract translations
      dl.ExtractTranslations_t( afnFiles[1]);
              } break;
    default: {
      printf( "Unrecognizable option requested.\n\n");
      PrintUsage();
             }
    }
  }
  catch( char *pError)
  {
    printf( "Error occured.\n%s\n", pError);
  }
}
