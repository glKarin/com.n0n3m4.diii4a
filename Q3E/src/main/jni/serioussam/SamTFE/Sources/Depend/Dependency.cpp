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
#include <Engine/Base/TranslationPair.h>

// adjust file path automatically for application path prefix
void AdjustFilePath_t(CTFileName &fnm)
{
  // if filename contains a colon or double backslash
  if (strchr(fnm, ':')!=NULL
    ||strstr(fnm, "\\\\")!=NULL) {
    // it must be prefixed with application path
    fnm.RemoveApplicationPath_t();
  }
}

// class constructor
CDependInfo::CDependInfo(CTFileName fnFileName, CTFileName fnParent)
{
  // copy file name
  di_fnFileName = fnFileName;
  di_fnParent = fnParent;
}

BOOL CDependInfo::IsFileOnDiskUpdated(void)
{
  int file_handle;
  // try to open file for reading
  file_handle = _open( _fnmApplicationPath + di_fnFileName, _O_RDONLY | _O_BINARY);
  // mark as it is not updated
  BOOL bUpdated = FALSE;
  // if opened succesefully
  if( file_handle != -1)
  {
    struct stat statFileStatus;
    // get file status
    fstat( file_handle, &statFileStatus);
    ASSERT(statFileStatus.st_mtime<=time(NULL));

    // if last modification time is same as remembered in dependency info object
    if( difftime( statFileStatus.st_mtime, di_tTime) >= 0)
    {
      // mark file as updated
      bUpdated = TRUE;
    }
  }
  if(file_handle!=-1) {
    _close(file_handle);
  }

  return bUpdated;
}

// if given file allready has its own DependInfo object linked in list
BOOL CDependencyList::ExistsInList(CListHead &lh, CTFileName fnTestName) const
{
  // for all members in depend list
  FOREACHINLIST( CDependInfo, di_Node, lh, itDependInfo)
  {
    // if this DependInfo is describing searching file
    if( itDependInfo->di_fnFileName == fnTestName)
    {
      // return true
      return TRUE;
    }
  }
  // if none of linked DepenInfo objects represent our testing file
  return FALSE;
}

// extract all dependencies from list
void CDependencyList::ExtractDependencies()
{
  int file_handle;
  volatile ULONG ulDFNM = 'NFD';
  volatile ULONG ulTFNM = 'NFT';
  volatile ULONG ulEFNM = 'NFE';
  char strDFNM[]="DFNM";
  char strTFNM[]="TFNM";
  char strEFNM[]="EFNM";

  ulDFNM |= ULONG('M')<<24;
  ulTFNM |= ULONG('M')<<24;
  ulEFNM |= ULONG('M')<<24;

  // for all list members
  CListHead lhToProcess;
  lhToProcess.MoveList(dl_ListHead);
  while(!lhToProcess.IsEmpty()) {
    CDependInfo *pdi = LIST_HEAD(lhToProcess, CDependInfo, di_Node);

    pdi->di_Node.Remove();
    dl_ListHead.AddTail(pdi->di_Node);

    CTFileName fnFileName = pdi->di_fnFileName;
    // try to open file for reading
    file_handle = _open( _fnmApplicationPath + fnFileName, _O_RDONLY | _O_BINARY);
    // if an error occured
    if( file_handle == -1)
    {
      // if file is not available remove it from list
      //FatalError( "File %s can't be opened!", (CTString&)(_fnmApplicationPath + fnFileName));
      printf( "warning, cannot open: %s (referenced from %s)\n", (CTString&)(fnFileName), (CTString&)(pdi->di_fnParent));
      delete pdi;
    }
    // if file is opened properly
    else
    {
      struct stat statFileStatus;
      // get file status
      fstat( file_handle, &statFileStatus);
      pdi->di_tTime = statFileStatus.st_mtime;
      ASSERT(pdi->di_tTime<=time(NULL));
      // get size
      SLONG ulSize = statFileStatus.st_size;
      // read file into memory
      char *pFileInMemory = (char *) AllocMemory( ulSize);
      // must be allocated properly
      ASSERT( pFileInMemory != NULL);
      // read file
      if( _read( file_handle, pFileInMemory, ulSize) != ulSize)
      {
        FatalError( "Fatal error ocured while reading file: %s.", (CTString&)pdi->di_fnFileName);
      }
      if(file_handle!=-1) {
        _close(file_handle);
      }

      // find all file name indentifiers in memory file ("EFNM" or "DFNM")
      for( INDEX charCt=0; charCt<ulSize-4; charCt++)
      {
        ULONG ulMemValue = *((ULONG *)(pFileInMemory + charCt));
        char *pchrDependentFile;
        // if we found file name inside exe file (notice little-big indian convetion)
        if( ulMemValue == ulEFNM)
        {
          // after describing long we will find file name
          pchrDependentFile = pFileInMemory + charCt + 4;
          // create full file name
          CTFileName fnTestName = CTString(pchrDependentFile);
          if( strlen(fnTestName) > 254) {
            continue;
          }
          // if found file name does not yet exists in dependacy list
          if( (fnTestName!="") && 
            (!ExistsInList(dl_ListHead, fnTestName)) &&
            (!ExistsInList(lhToProcess, fnTestName)))
          {
            // create new depend info object
            CDependInfo *pDI = new CDependInfo( fnTestName, fnFileName);
            // add it at tail of dependency list
            lhToProcess.AddTail( pDI->di_Node);
          }
        }
        // if we found file name inside data file (notice little-big indian convetion)
        else if( ulMemValue == ulDFNM)
        {
          char chrFileName[ 256];
          INDEX iStringLenght = *(INDEX *)(pFileInMemory + charCt + 4);
          if( iStringLenght > 254 || iStringLenght<0) {
            continue;
          }
          memcpy( chrFileName, pFileInMemory + charCt + 8, iStringLenght);
          chrFileName[ iStringLenght] = 0;
          // create full file name
          CTFileName fnTestName = CTString(chrFileName);
          if( fnTestName!="" &&
            (!ExistsInList(dl_ListHead, fnTestName)) &&
            (!ExistsInList(lhToProcess, fnTestName)))
          {
            // create new depend info object
            // NOTICE: string containing file name starts after ULONG determing its lenght (+8!)
            CDependInfo *pDI = new CDependInfo( fnTestName, fnFileName);
            // add it at tail of dependency list
            lhToProcess.AddTail( pDI->di_Node);
          }
        }
        // if we found file name inside text file (notice little-big indian convetion)
        else if( ulMemValue == ulTFNM)
        {
          // after describing long and one space we will find file name
          pchrDependentFile = pFileInMemory + charCt + 4 + 1;
          // copy file name from the file, until newline
          char chrFileName[ 256];
          char *chrSrc = pchrDependentFile;
          char *chrDst = chrFileName;
          while(*chrSrc!='\n' && *chrSrc!='\r' && chrSrc-pchrDependentFile<254) {
            *chrDst++ = *chrSrc++;
          }
          *chrDst = 0;
          CTFileName fnTestName = CTString(chrFileName);
          if( strlen(fnTestName) > 254) {
            continue;
          }
          // if found file name does not yet exists in dependacy list
          if( (fnTestName!="") && 
            (!ExistsInList(dl_ListHead, fnTestName)) &&
            (!ExistsInList(lhToProcess, fnTestName)))
          {
            // create new depend info object
            CDependInfo *pDI = new CDependInfo( fnTestName, fnFileName);
            // add it at tail of dependency list
            lhToProcess.AddTail( pDI->di_Node);
          }
        }
      }
      // free file from memory
      FreeMemory(pFileInMemory);
    }
  }
}

// substracts given dependecy list from this
void CDependencyList::Substract( CDependencyList &dlToSubstract)
{
  // for all files in this dependency
  FORDELETELIST( CDependInfo, di_Node, dl_ListHead, itThis)
  {
    // see if file exists in other dependency
    FOREACHINLIST( CDependInfo, di_Node, dlToSubstract.dl_ListHead, itOther)
    {
      // if we found same file in other dependency list
      if( itThis->di_fnFileName == itOther->di_fnFileName)
      {
        // if file is updated
        if( itOther->IsUpdated( *itThis))
        {
          // remove it from list
          delete &itThis.Current();
          break;
        }
        // if file to substract is newer than this one
        else if( itThis->IsOlder( *itOther))
        {
          FatalError( "File \"%s\" is newer in substracting dependency file.",
                      (CTString&)itThis->di_fnFileName);
        }
      }
    }
  }
}

// create list from ascii file
void CDependencyList::ImportASCII( CTFileName fnAsciiFile)
{
	char chrOneLine[ 256];
  CTFileStream file;
  int file_handle;

  // try to
  try
  {
    // open file list for reading
    file.Open_t( fnAsciiFile);
  }
  catch( char *pError)
  {
    FatalError( "Error opening file %s. Error: %s.", (CTString&)fnAsciiFile, pError);
  }

  // loop loading lines until EOF reached ("catched")
  FOREVER
  {
    try {
      // load one line from file
      file.GetLine_t( chrOneLine, 256);
      // create file name from loaded line
      CTFileName fnFileName =  CTString(chrOneLine);
      AdjustFilePath_t(fnFileName);
	    // try to open file for reading
      file_handle = _open( _fnmApplicationPath+fnFileName, _O_RDONLY | _O_BINARY);

      // if opened succesefully
      if( file_handle != -1) {
        // create new depend info object
        CDependInfo *pDI = new CDependInfo( fnFileName, fnAsciiFile);
        struct stat statFileStatus;
        // get file status
        fstat( file_handle, &statFileStatus);
        // obtain last modification time
        pDI->di_tTime = statFileStatus.st_mtime;
        ASSERT(pDI->di_tTime<=time(NULL));
        // add it at tail of dependency list
        dl_ListHead.AddTail( pDI->di_Node);
        // close file
        _close( file_handle);
      } else {
        CPrintF("cannot open file '%s'\n", chrOneLine);
      }
    }
    // error, EOF catched
    catch( char *pFinished)
    {
      if (!file.AtEOF()) {
        CPrintF("%s\n", pFinished);
      }
      break;
    }
  }
}

// remove updated files from list
void CDependencyList::RemoveUpdatedFiles()
{
  // for all list members
  FORDELETELIST( CDependInfo, di_Node, dl_ListHead, itDependInfo)
  {
    // if file is updated
    if( itDependInfo->IsFileOnDiskUpdated())
    {
      // remove it from list
      delete &itDependInfo.Current();
    }
  }
}

// clear dependency list
void CDependencyList::Clear( void)
{
  // for all list members
  FORDELETELIST( CDependInfo, di_Node, dl_ListHead, itDependInfo)
  {
    // delete member
    delete &itDependInfo.Current();
  }
}

// export list members into ascii file in form sutable for archivers
void CDependencyList::ExportASCII_t( CTFileName fnAsciiFile)
{
  CTFileStream strmFile;
  char line[ 256];

  // try to
  try
  {
    // create exporting text file
    strmFile.Create_t( fnAsciiFile);
  }
  catch( char *pError)
  {
    FatalError( "Error creating file %s. Error: %s.", (CTString&)fnAsciiFile, pError);
  }

  // for all members in depend list
  FOREACHINLIST( CDependInfo, di_Node, dl_ListHead, itDependInfo)
  {
    // prepare line of text
    sprintf( line, "%s\n", (CTString&)itDependInfo->di_fnFileName);
    // write text line into file
    strmFile.Write_t( line, strlen( line));
  }
}


// dynamic array of translation pairs
CDynamicArray<CTranslationPair> _atpPairs;

// add one string to array
static void AddStringForTranslation(const CTString &str)
{
  // for each existing pair
  _atpPairs.Lock();
  INDEX ct = _atpPairs.Count();
  for(INDEX i=0; i<ct; i++) {
    // if it is that one
    if (strcmp(_atpPairs[i].tp_strSrc, str)==0) {
      // just mark it as used
      _atpPairs[i].m_bUsed = TRUE;
      // don't search any more
      _atpPairs.Unlock();
      return;
    }
  }
  _atpPairs.Unlock();

  // if not found, add it
  CTranslationPair &tp = *_atpPairs.New(1);
  tp.tp_strSrc = str;
  tp.tp_strDst = CTString("$TODO$") + str;
  tp.m_bUsed = TRUE;
}

static void WriteTranslationToken_t(CTStream &strm, CTString str)
{
  strm.PutString_t(str);
}
static void WriteTranslationString_t(CTStream &strm, CTString str)
{
  const char *s = str;
  INDEX iLen=strlen(s);
  for (INDEX i=0; i<iLen; i++) {
    char c = s[i];
    switch(c) {
    case '\n':
      strm<<UBYTE('\\')<<UBYTE('n')<<UBYTE('\n');
      break;
    case '\\':
      strm<<UBYTE('\\')<<UBYTE('\\');
      break;
    default:
      strm<<UBYTE(c);
    }
  }
}

// extract translation strings from all files in list
void CDependencyList::ExtractTranslations_t( const CTFileName &fnTranslations)
{
  _atpPairs.Clear();

  // read original translation table
  ReadTranslationTable_t(_atpPairs, fnTranslations);

  // for each file in list
  FOREACHINLIST( CDependInfo, di_Node, dl_ListHead, itDependInfo) {
    CDependInfo &di = *itDependInfo;
    // open the file
    CTFileStream strm;
    strm.Open_t(di.di_fnFileName);
    // load it in memory
    SLONG slSize = strm.GetStreamSize();
    UBYTE *pubFile = (UBYTE *)AllocMemory(slSize);
    strm.Read_t(pubFile, slSize);

    // for each byte in file
    for(INDEX iByte=0; iByte<slSize-4; iByte++) {
      UBYTE *pub = pubFile+iByte;
      ULONG *pul = (ULONG*)pub;

      // if exe translatable string is here
      if (*pul=='SRTE') {
        // get it
        CTString str = (char*)(pub+4);
        // add it
        AddStringForTranslation(str);
      // if data translatable string is here
      } else if (*pul=='SRTD') {
        char achr[ 256];
        INDEX iStrLen = *(INDEX *)(pub + 4);
        if( iStrLen > 254 || iStrLen<0) {
          continue;
        }
        memcpy( achr, pub + 8, iStrLen);
        achr[ iStrLen] = 0;
        // get it
        CTString str = achr;
        // add it
        AddStringForTranslation(str);
      // if text translatable string is here
      } else if (*pul=='SRTT') {

        // after describing long and one space we will find file name
        char *pchrStart = (char*)pub + 4 + 1;
        // copy file name from the file, until newline
        char chrString[ 256];
        char *chrSrc = pchrStart;
        char *chrDst = chrString;
        while(*chrSrc!='\n' && *chrSrc!='\r' && chrSrc-pchrStart<254) {
          *chrDst++ = *chrSrc++;
        }
        *chrDst = 0;
        CTString str = CTString(chrString);
        if( strlen(str) > 254) {
          continue;
        }
        // add it
        AddStringForTranslation(str);
      }
    }

    // free file from memory
    FreeMemory(pubFile);
  }

  // count used pairs
  _atpPairs.Lock();
  INDEX ctUsedPairs = 0;
  {INDEX ct = _atpPairs.Count();
  for(INDEX i=0; i<ct; i++) {
    if(_atpPairs[i].m_bUsed) {
      ctUsedPairs++;
    }
  }}
  _atpPairs.Unlock();

  // create output file
  CTFileStream strm;
  strm.Create_t(fnTranslations);
  // write number of used pairs
  CTString strNo;
  strNo.PrintF("%d", ctUsedPairs);
  WriteTranslationString_t(strm, strNo);
  WriteTranslationToken_t(strm, "\n");
  // for each used pair
  _atpPairs.Lock();
  {INDEX ct = _atpPairs.Count();
  for(INDEX i=0; i<ct; i++) {
    CTranslationPair &tp = _atpPairs[i];
    if(!tp.m_bUsed) {
      continue;
    }
    // write it, as source and destination, with tags in front
    WriteTranslationToken_t(strm, "\n\\0\\<\n");
    WriteTranslationString_t(strm, tp.tp_strSrc);
    WriteTranslationToken_t(strm, "\n\\0\\>\n");
    WriteTranslationString_t(strm, tp.tp_strDst);
  }}
  _atpPairs.Unlock();
  // write eof token
  WriteTranslationToken_t(strm, "\n\\0\\$\n");
}

// read operation
void CDependencyList::Read_t( CTStream *istrFile)
{
  // count of files
  INDEX ctFiles;
  // expect file ID
  istrFile->ExpectID_t( CChunkID( "DEPD"));
  // read number of entries in list
  *istrFile >> ctFiles;
  // load all list members
  for( INDEX i=0; i<ctFiles; i++)
  {
    // create new depend info object
    CDependInfo *pDI = new CDependInfo( CTString(""), istrFile->GetDescription());
    // read dependency object
    pDI->Read_t( istrFile);
    // add it at tail of dependency list
    dl_ListHead.AddTail( pDI->di_Node);
  }
}

// write opertaion
void CDependencyList::Write_t( CTStream *ostrFile)
{
  // write file ID
  ostrFile->WriteID_t( CChunkID( "DEPD"));
  // write number of entries in list
  *ostrFile << dl_ListHead.Count();
  // for all members in dependency list
  FOREACHINLIST( CDependInfo, di_Node, dl_ListHead, itDependInfo)
  {
    // write one depend info object
    itDependInfo->Write_t( ostrFile);
  }
}

// make a new directory recursively
static void MakeDirectory_t(const CTFileName &fnm)
{
  if (fnm=="") {
    return;
  }
  // remove trailing backslash
  CTFileName fnmDir = fnm;
  ((char *)(const char*)fnmDir)[strlen(fnmDir)-1] = 0;
  // get the path part
  CTFileName fnmDirPath = fnmDir.FileDir();
  // if there is a path part
  if (fnmDirPath!="") {
    // create that first
    MakeDirectory_t(fnmDirPath);
  }
  // try to create the directory
  int iRes = _mkdir(_fnmApplicationPath+fnmDir);
}

