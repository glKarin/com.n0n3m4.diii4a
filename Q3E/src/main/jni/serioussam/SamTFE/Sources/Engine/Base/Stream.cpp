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

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

// !!! FIXME : rcg10162001 Need this anymore, since _findfirst() is abstracted?
#ifdef PLATFORM_WIN32
#include <io.h>
#include <direct.h>
#include <DbgHelp.h>
#endif

#include <Engine/Base/Protection.h>
#include <Engine/Base/Stream.h>

#include <Engine/Base/Memory.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Base/Unzip.h>
#include <Engine/Base/CRC.h>
#include <Engine/Base/Shell.h>
#include <Engine/Base/FileSystem.h>
#include <Engine/Templates/NameTable_CTFileName.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicStackArray.cpp>

#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CModelData.h>

#include "iconvlite.h"

// default size of page used for stream IO operations (4Kb)
ULONG _ulPageSize = 0;
// maximum lenght of file that can be saved (default: 128Mb)
ULONG _ulMaxLenghtOfSavingFile = (1UL<<20)*128;
#ifdef PLATFORM_UNIX
INDEX fil_bPreferZips = FALSE;

// set if current thread has currently enabled stream handling
THREADLOCAL(BOOL, _bThreadCanHandleStreams, FALSE);
// list of currently opened streams
ULONG _ulVirtuallyAllocatedSpace = 0;
ULONG _ulVirtuallyAllocatedSpaceTotal = 0;
THREADLOCAL(CListHead *, _plhOpenedStreams, NULL);
// portable version (all user files stored in game dir)
INDEX _bPortableVersion = FALSE;
#else
extern INDEX fil_bPreferZips = FALSE;

// set if current thread has currently enabled stream handling
static _declspec(thread) BOOL _bThreadCanHandleStreams = FALSE;
// list of currently opened streams
static _declspec(thread) CListHead *_plhOpenedStreams = NULL;

ULONG _ulVirtuallyAllocatedSpace = 0;
ULONG _ulVirtuallyAllocatedSpaceTotal = 0;
#endif

// global string with application path (utf-8)
CTFileName _fnmApplicationPath;
CTFileName _fnmApplicationPathTMP; // home dir or application path

// global string with filename of the started application
CTFileName _fnmApplicationExe;
// global string with user-specific writable directory.
CTFileName _fnmUserDir;
// global string with current MOD path
CTFileName _fnmMod;
// global string with current name (the parameter that is passed on cmdline)
CTString _strModName;
// global string with url to be shown to users that don't have the mod installed
// (should be set by game.dll)
CTString _strModURL;
// global string with current MOD extension (for adding to dlls)
CTString _strModExt;
// global string with CD path (for minimal installations)
CTFileName _fnmCDPath;

// include/exclude lists for base dir writing/browsing
CDynamicStackArray<CTFileName> _afnmBaseWriteInc;
CDynamicStackArray<CTFileName> _afnmBaseWriteExc;
CDynamicStackArray<CTFileName> _afnmBaseBrowseInc;
CDynamicStackArray<CTFileName> _afnmBaseBrowseExc;
// list of paths or patterns that are not included when making CRCs for network connection
// this is used to enable connection between different localized versions
CDynamicStackArray<CTFileName> _afnmNoCRC;
// used to convert filenames from ansi format (written inside text files) into utf-8 (
CTFileName convertWindow1251ToUtf8(const CTFileName &from) {
  static char outBuffer[10000];
  cp2utf1(outBuffer, from.str_String, sizeof(outBuffer) - 10);
  return (CTString) outBuffer;
}

// load a filelist
static BOOL LoadFileList(CDynamicStackArray<CTFileName> &afnm, const CTFileName &fnmList)
{
  afnm.PopAll();
  try {
    CTFileStream strm;
    strm.Open_t(fnmList);
    while(!strm.AtEOF()) {
      CTString strLine;
      strm.GetLine_t(strLine);
      strLine.TrimSpacesLeft();
      strLine.TrimSpacesRight();
      if (strLine!="") {
        afnm.Push() = strLine;
      }
    }
    return TRUE;
  } catch (const char *strError) {
    CPrintF("%s\n", strError);
    return FALSE;
  }
}

extern BOOL FileMatchesList(CDynamicStackArray<CTFileName> &afnm, const CTFileName &fnm)
{
  for(INDEX i=0; i<afnm.Count(); i++) {
    if (fnm.Matches(afnm[i]) || fnm.HasPrefix(afnm[i])) {
      return TRUE;
    }
  }
  return FALSE;
}

static CTFileName _fnmApp;

void InitStreams(void)
{
  // obtain information about system
// !!! FIXME: Move this into an abstraction of some sort...
#ifdef PLATFORM_WIN32
  SYSTEM_INFO siSystemInfo;
  GetSystemInfo( &siSystemInfo);
  // and remember page size
  _ulPageSize = siSystemInfo.dwPageSize*16;   // cca. 64kB on WinNT/Win95
#else
  _ulPageSize = PAGESIZE;
#endif

  // keep a copy of path for setting purposes
  _fnmApp = _fnmApplicationPath;

  // if no mod defined yet
  if (_fnmMod=="") {
    // check for 'default mod' file
    LoadStringVar(CTString("DefaultMod.txt"), _fnmMod);
  }

  CPrintF(TRANSV("Current mod: %s\n"),
            (_fnmMod=="") ? TRANS("<none>") :
                            (const char *) (CTString&)_fnmMod);
  // if there is a mod active
  if (_fnmMod!="") {
    // load mod's include/exclude lists
    CPrintF(TRANSV("Loading mod include/exclude lists...\n"));
    BOOL bOK = FALSE;
    bOK |= LoadFileList(_afnmBaseWriteInc , CTString("BaseWriteInclude.lst"));
    bOK |= LoadFileList(_afnmBaseWriteExc , CTString("BaseWriteExclude.lst"));
    bOK |= LoadFileList(_afnmBaseBrowseInc, CTString("BaseBrowseInclude.lst"));
    bOK |= LoadFileList(_afnmBaseBrowseExc, CTString("BaseBrowseExclude.lst"));

    // if none found
    if (!bOK) {
      // the mod is not valid
      _fnmMod = CTString("");
      CPrintF(TRANSV("Error: MOD not found!\n"));
    // if mod is ok
    } else {
      // remember mod name (the parameter that is passed on cmdline)
      _strModName = _fnmMod;
      _strModName.DeleteChar(_strModName.Length()-1);
      _strModName = CTFileName(_strModName).FileName();
    }
  }
  // find eventual extension for the mod's dlls
  _strModExt = "";
  // DG: apparently both ModEXT.txt and ModExt.txt exist in the wild.
  CTFileName tmp;
  if(ExpandFilePath(EFP_READ, CTString("ModEXT.txt"), tmp) != EFP_NONE) {
    LoadStringVar(CTString("ModEXT.txt"), _strModExt);
  } else {
    LoadStringVar(CTString("ModExt.txt"), _strModExt);
  }


  CPrintF(TRANSV("Loading group files...\n"));

  CDynamicArray<CTString> *files = NULL;

  // for each group file in base directory
  files = _pFileSystem->FindFiles(_fnmApplicationPath, "*.gro");
  int max = files->Count();
  int i;

  // for each .gro file in the directory
  for (i = 0; i < max; i++) {
    // add it to active set
    UNZIPAddArchive( _fnmApplicationPath+((*files)[i]) );
  }
  delete files;

  // if there is a mod active
  if (_fnmMod!="") {
    // for each group file in mod directory
     files = _pFileSystem->FindFiles(_fnmApplicationPath+_fnmMod, "*.gro");
     max = files->Count();
     for (i = 0; i < max; i++) {
       UNZIPAddArchive( _fnmApplicationPath + _fnmMod + ((*files)[i]) );
     }
     delete files;
  }

  // if there is a CD path
  if (_fnmCDPath!="") {
    // for each group file on the CD
    files = _pFileSystem->FindFiles(_fnmCDPath, "*.gro");
    max = files->Count();
    for (i = 0; i < max; i++) {
      UNZIPAddArchive( _fnmCDPath + ((*files)[i]) );
    }
    delete files;

    // if there is a mod active
    if (_fnmMod!="") {
      // for each group file in mod directory
      files = _pFileSystem->FindFiles(_fnmCDPath+_fnmMod, "*.gro");
      max = files->Count();
      for (i = 0; i < max; i++) {
        UNZIPAddArchive( _fnmCDPath + _fnmMod + ((*files)[i]) );
      }
      delete files;
    }
  }

  // try to
  try {
    // read the zip directories
    UNZIPReadDirectoriesReverse_t();
  // if failed
  } catch (const char *strError) {
    // report warning
    CPrintF( TRANS("There were group file errors:\n%s"), strError);
  }
  CPrintF("\n");

  const char *dirsep = CFileSystem::GetDirSeparator();
  LoadFileList(_afnmNoCRC, CTFILENAME("Data") + CTString(dirsep) + CTString("NoCRC.lst"));

  _pShell->SetINDEX(CTString("sys")+"_iCPU"+"Misc", 1);
}

void EndStreams(void)
{
}


void UseApplicationPath(void) 
{
  _fnmApplicationPath = _fnmApp;
}

void IgnoreApplicationPath(void)
{
  _fnmApplicationPath = CTString("");
}


/////////////////////////////////////////////////////////////////////////////
// Helper functions

/* Static function enable stream handling. */
void CTStream::EnableStreamHandling(void)
{
  ASSERT(!_bThreadCanHandleStreams && _plhOpenedStreams == NULL);

  _bThreadCanHandleStreams = TRUE;
  _plhOpenedStreams = new CListHead;
}

/* Static function disable stream handling. */
void CTStream::DisableStreamHandling(void)
{
  ASSERT(_bThreadCanHandleStreams && _plhOpenedStreams != NULL);

  _bThreadCanHandleStreams = FALSE;
  delete _plhOpenedStreams;
  _plhOpenedStreams = NULL;
}

#ifdef PLATFORM_WIN32
int CTStream::ExceptionFilter(DWORD dwCode, _EXCEPTION_POINTERS *pExceptionInfoPtrs)
{
  // If the exception is not a page fault, exit.
  if( dwCode != EXCEPTION_ACCESS_VIOLATION)
  {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  // obtain access violation virtual address
  UBYTE *pIllegalAdress = (UBYTE *)pExceptionInfoPtrs->ExceptionRecord->ExceptionInformation[1];

  CTStream *pstrmAccessed = NULL;

  // search for stream that was accessed
  FOREACHINLIST( CTStream, strm_lnListNode, (*_plhOpenedStreams), itStream)
  {
    // if access violation happened inside curently testing stream
    if(itStream.Current().PointerInStream(pIllegalAdress))
    {
      // remember accesed stream ptr
      pstrmAccessed = &itStream.Current();
      // stream found, stop searching
      break;
    }
  }

  // if none of our streams was accessed, real access violation occured
  if( pstrmAccessed == NULL)
  {
    // so continue default exception handling
    return EXCEPTION_CONTINUE_SEARCH;
  }

  // Continue execution where the page fault occurred
  return EXCEPTION_CONTINUE_EXECUTION;
}

/*
 * Static function to report fatal exception error.
 */
void CTStream::ExceptionFatalError(void)
{
  FatalError( GetWindowsError( GetLastError()) );
}
#endif
/*
 * Throw an exception of formatted string.
 */
void CTStream::Throw_t(const char *strFormat, ...)  // throws char *
{
  const SLONG slBufferSize = 256;
  char strFormatBuffer[slBufferSize];
  static char *strBuffer = NULL;

  // ...and yes, you are screwed if you call this in a catch block and
  //  try to access the previous text again.
  delete[] strBuffer;
  strBuffer = new char[slBufferSize];

  // add the stream description to the format string
  _snprintf(strFormatBuffer, slBufferSize, "%s (%s)", strFormat, (const char *) strm_strStreamDescription);
  // format the message in buffer
  va_list arg;
  va_start(arg, strFormat); // variable arguments start after this argument
  _vsnprintf(strBuffer, slBufferSize, strFormatBuffer, arg);
  va_end(arg);
  throw strBuffer;
}

/////////////////////////////////////////////////////////////////////////////
// Binary access methods

/* Get CRC32 of stream */
ULONG CTStream::GetStreamCRC32_t(void)
{
  // remember where stream is now
  SLONG slOldPos = GetPos_t();
  // go to start of file
  SetPos_t(0);
  // get size of file
  SLONG slFileSize = GetStreamSize();

  ULONG ulCRC;
  CRC_Start(ulCRC);

  // for each block in file
  const SLONG slBlockSize = 4096;
  for(SLONG slPos=0; slPos<slFileSize; slPos+=slBlockSize) {
    // read the block
    UBYTE aubBlock[slBlockSize];
    SLONG slThisBlockSize = Min(slFileSize-slPos, slBlockSize);
    Read_t(aubBlock, slThisBlockSize);
    // checksum it
    CRC_AddBlock(ulCRC, aubBlock, slThisBlockSize);
  }

  // restore position
  SetPos_t(slOldPos);

  CRC_Finish(ulCRC);
  return ulCRC;
}

/////////////////////////////////////////////////////////////////////////////
// Text access methods

/* Get a line of text from file. */
// throws char *
void CTStream::GetLine_t(char *strBuffer, SLONG slBufferSize, char cDelimiter /*='\n'*/ )
{
  // check parameters
  ASSERT(strBuffer!=NULL && slBufferSize>0);
  // check that the stream can be read
  ASSERT(IsReadable());
  // letters slider
  INDEX iLetters = 0;
  // test if EOF reached
  if(AtEOF()) {
    ThrowF_t(TRANS("EOF reached, file %s"), (const char *) strm_strStreamDescription);
  }
  // get line from istream
  FOREVER
  {
    char c;
    Read_t(&c, 1);

    if(AtEOF()) {
      // cut off
      strBuffer[ iLetters] = 0;
      break;
    }

    // don't read "\r" characters but rather act like they don't exist
    if( c != '\r') {
      strBuffer[ iLetters] = c;
      // stop reading when delimiter loaded
      if( strBuffer[ iLetters] == cDelimiter) {
        // convert delimiter to zero
        strBuffer[ iLetters] = 0;
        // jump over delimiter
        //Seek_t(1, SD_CUR);
        break;
      }
      // jump to next destination letter
      iLetters++;
    }
    // test if maximum buffer lenght reached
    if( iLetters==slBufferSize) {
      return;
    }
  }
}

void CTStream::GetLine_t(CTString &strLine, char cDelimiter/*='\n'*/) // throw char *
{
  char strBuffer[1024];
  GetLine_t(strBuffer, sizeof(strBuffer)-1, cDelimiter);
  strLine = strBuffer;
}


/* Put a line of text into file. */
void CTStream::PutLine_t(const char *strBuffer) // throws char *
{
  // check parameters
  ASSERT(strBuffer!=NULL);
  // check that the stream is writteable
  ASSERT(IsWriteable());
  // get string length
  INDEX iStringLength = strlen(strBuffer);
  // put line into stream
  Write_t(strBuffer, iStringLength);
  // write "\r\n" into stream
  Write_t("\r\n", 2);
}

void CTStream::PutString_t(const char *strString) // throw char *
{
  // check parameters
  ASSERT(strString!=NULL);
  // check that the stream is writteable
  ASSERT(IsWriteable());
  // get string length
  INDEX iStringLength = strlen(strString);
  // put line into stream
  for( INDEX iLetter=0; iLetter<iStringLength; iLetter++)
  {
    if (*strString=='\n') {
      // write "\r\n" into stream
      Write_t("\r\n", 2);
      strString++;
    } else {
      Write_t(strString++, 1);
    }
  }
}

void CTStream::FPrintF_t(const char *strFormat, ...) // throw char *
{
  const SLONG slBufferSize = 2048;
  char strBuffer[slBufferSize];
  // format the message in buffer
  va_list arg;
  va_start(arg, strFormat); // variable arguments start after this argument
  _vsnprintf(strBuffer, slBufferSize, strFormat, arg);
  va_end(arg);
  // print the buffer
  PutString_t(strBuffer);
}

/////////////////////////////////////////////////////////////////////////////
// Chunk reading/writing methods

CChunkID CTStream::GetID_t(void) // throws char *
{
	CChunkID cidToReturn;
	Read_t( &cidToReturn.cid_ID[0], CID_LENGTH);
	return( cidToReturn);
}

CChunkID CTStream::PeekID_t(void) // throw char *
{
  // read the chunk id
	CChunkID cidToReturn;
	Read_t( &cidToReturn.cid_ID[0], CID_LENGTH);
  // return the stream back
  Seek_t(-CID_LENGTH, SD_CUR);
	return( cidToReturn);
}

void CTStream::ExpectID_t(const CChunkID &cidExpected) // throws char *
{
	CChunkID cidToCompare;

	Read_t( &cidToCompare.cid_ID[0], CID_LENGTH);
	if( !(cidToCompare == cidExpected))
	{
		ThrowF_t(TRANS("Chunk ID validation failed.\nExpected ID \"%s\" but found \"%s\"\n"),
      cidExpected.cid_ID, cidToCompare.cid_ID);
	}
}
void CTStream::ExpectKeyword_t(const CTString &strKeyword) // throw char *
{
  // check that the keyword is present
  const INDEX total = (INDEX)strlen(strKeyword);
  for(INDEX iKeywordChar=0; iKeywordChar<total; iKeywordChar++) {
    SBYTE chKeywordChar;
    (*this)>>chKeywordChar;
    if (chKeywordChar!=strKeyword[iKeywordChar]) {
      ThrowF_t(TRANS("Expected keyword %s not found"), (const char *) strKeyword);
    }
  }
}


SLONG CTStream::GetSize_t(void) // throws char *
{
	SLONG chunkSize;

	Read_t( (char *) &chunkSize, sizeof( SLONG));
	return( chunkSize);
}

void CTStream::ReadRawChunk_t(void *pvBuffer, SLONG slSize)  // throws char *
{
	Read_t((char *)pvBuffer, slSize);
}

void CTStream::ReadChunk_t(void *pvBuffer, SLONG slExpectedSize) // throws char *
{
	if( slExpectedSize != GetSize_t())
		throw TRANS("Chunk size not equal as expected size");
	Read_t((char *)pvBuffer, slExpectedSize);
}

void CTStream::ReadFullChunk_t(const CChunkID &cidExpected, void *pvBuffer,
                             SLONG slExpectedSize) // throws char *
{
	ExpectID_t( cidExpected);
	ReadChunk_t( pvBuffer, slExpectedSize);
};

void* CTStream::ReadChunkAlloc_t(SLONG slSize) // throws char *
{
	UBYTE *buffer;
	SLONG chunkSize;

	if( slSize != 0)
		chunkSize = slSize;
	else
		chunkSize = GetSize_t(); // throws char *
	buffer = (UBYTE *) AllocMemory( chunkSize);
	if( buffer == NULL)
		throw TRANS("ReadChunkAlloc: Unable to allocate needed amount of memory.");
	Read_t((char *)buffer, chunkSize); // throws char *
	return buffer;
}
void CTStream::ReadStream_t(CTStream &strmOther) // throw char *
{
  // implement this !!!! @@@@
}

void CTStream::WriteID_t(const CChunkID &cidSave) // throws char *
{
	Write_t( &cidSave.cid_ID[0], CID_LENGTH);
}

void CTStream::WriteSize_t(SLONG slSize) // throws char *
{
	Write_t( (char *)&slSize, sizeof( SLONG));
}

void CTStream::WriteRawChunk_t(void *pvBuffer, SLONG slSize) // throws char *
{
	Write_t( (char *)pvBuffer, slSize);
}

void CTStream::WriteChunk_t(void *pvBuffer, SLONG slSize) // throws char *
{
	WriteSize_t( slSize);
	WriteRawChunk_t( pvBuffer, slSize);
}

void CTStream::WriteFullChunk_t(const CChunkID &cidSave, void *pvBuffer,
                              SLONG slSize) // throws char *
{
	WriteID_t( cidSave); // throws char *
	WriteChunk_t( pvBuffer, slSize); // throws char *
}
void CTStream::WriteStream_t(CTStream &strmOther) // throw char *
{
  // implement this !!!! @@@@
}

// whether or not the given pointer is coming from this stream (mainly used for exception handling)
BOOL CTStream::PointerInStream(void* pPointer)
{
  // safe to return FALSE, we're using virtual functions anyway
  return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// filename dictionary operations

// enable dictionary in writable file from this point
void CTStream::DictionaryWriteBegin_t(const CTFileName &fnmImportFrom, SLONG slImportOffset)
{
  ASSERT(strm_slDictionaryPos==0);
  ASSERT(strm_dmDictionaryMode == DM_NONE);
  strm_ntDictionary.SetAllocationParameters(100, 5, 5);
  strm_ctDictionaryImported = 0;

  // if importing an existing dictionary to start with
  if (fnmImportFrom!="") {
    // open that file
    CTFileStream strmOther;
    strmOther.Open_t(fnmImportFrom);
    // read the dictionary in that stream
    strmOther.ReadDictionary_intenal_t(slImportOffset);
    // copy the dictionary here
    CopyDictionary(strmOther);
    // write dictionary importing data
    WriteID_t("DIMP");  // dictionary import
    *this<<fnmImportFrom<<slImportOffset;
    // remember how many filenames were imported
    strm_ctDictionaryImported = strm_afnmDictionary.Count();
  }

  // write dictionary position chunk id
  WriteID_t("DPOS");  // dictionary position
  // remember where position will be placed
  strm_slDictionaryPos = GetPos_t();
  // leave space for position
  *this<<SLONG(0);

  // start dictionary
  strm_dmDictionaryMode = DM_ENABLED;
}

// write the dictionary (usually at the end of file)
void CTStream::DictionaryWriteEnd_t(void)
{
  ASSERT(strm_dmDictionaryMode == DM_ENABLED);
  ASSERT(strm_slDictionaryPos>0);
  // remember the dictionary position chunk position
  SLONG slDictPos = strm_slDictionaryPos;
  // mark that now saving dictionary
  strm_slDictionaryPos = -1;
  // remember where dictionary begins
  SLONG slDictBegin = GetPos_t();
  // start dictionary processing
  strm_dmDictionaryMode = DM_PROCESSING;

  WriteID_t("DICT");  // dictionary
  // write number of used filenames
  INDEX ctFileNames = strm_afnmDictionary.Count();
  INDEX ctFileNamesNew = ctFileNames-strm_ctDictionaryImported;
  *this<<ctFileNamesNew;
  // for each filename
  for(INDEX iFileName=strm_ctDictionaryImported; iFileName<ctFileNames; iFileName++) {
    // write it to disk
    *this<<strm_afnmDictionary[iFileName];
  }
  WriteID_t("DEND");  // dictionary end

  // remember where is end of dictionary
  SLONG slContinue = GetPos_t();

  // write the position back to dictionary position chunk
  SetPos_t(slDictPos);
  *this<<slDictBegin;

  // stop dictionary processing
  strm_dmDictionaryMode = DM_NONE;
  strm_ntDictionary.Clear();
  strm_afnmDictionary.Clear();

  // return to end of dictionary
  SetPos_t(slContinue);
  strm_slDictionaryPos=0;
}

// read the dictionary from given offset in file (internal function)
void CTStream::ReadDictionary_intenal_t(SLONG slOffset)
{
  // remember where to continue loading
  SLONG slContinue = GetPos_t();

  // go to dictionary beginning
  SetPos_t(slOffset);

  // start dictionary processing
  strm_dmDictionaryMode = DM_PROCESSING;

  ExpectID_t("DICT");  // dictionary
  // read number of new filenames
  INDEX ctFileNamesOld = strm_afnmDictionary.Count();
  INDEX ctFileNamesNew;
  *this>>ctFileNamesNew;
  // if there are any new filenames
  if (ctFileNamesNew>0) {
    // create that much space
    strm_afnmDictionary.Push(ctFileNamesNew);
    // for each filename
    for(INDEX iFileName=ctFileNamesOld; iFileName<ctFileNamesOld+ctFileNamesNew; iFileName++) {
      // read it
      *this>>strm_afnmDictionary[iFileName];
    }
  }
  ExpectID_t("DEND");  // dictionary end

  // remember where end of dictionary is
  strm_slDictionaryPos = GetPos_t();

  // return to continuing position
  SetPos_t(slContinue);
}

// copy filename dictionary from another stream
void CTStream::CopyDictionary(CTStream &strmOther)
{
   strm_afnmDictionary = strmOther.strm_afnmDictionary;
   for (INDEX i=0; i<strm_afnmDictionary.Count(); i++) {
     strm_ntDictionary.Add(&strm_afnmDictionary[i]);
   }
}

SLONG CTStream::DictionaryReadBegin_t(void)
{
  ASSERT(strm_dmDictionaryMode == DM_NONE);
  ASSERT(strm_slDictionaryPos==0);
  strm_ntDictionary.SetAllocationParameters(100, 5, 5);

  SLONG slImportOffset = 0;
  // if there is imported dictionary
  if (PeekID_t()==CChunkID("DIMP")) {  // dictionary import
    // read dictionary importing data
    ExpectID_t("DIMP");  // dictionary import
    CTFileName fnmImportFrom;
    *this>>fnmImportFrom>>slImportOffset;

    // open that file
    CTFileStream strmOther;
    strmOther.Open_t(fnmImportFrom);
    // read the dictionary in that stream
    strmOther.ReadDictionary_intenal_t(slImportOffset);
    // copy the dictionary here
    CopyDictionary(strmOther);
  }

  // if the dictionary is not here
  if (PeekID_t()!=CChunkID("DPOS")) {  // dictionary position
    // do nothing
    return 0;
  }

  // read dictionary position
  ExpectID_t("DPOS"); // dictionary position
  SLONG slDictBeg;
  *this>>slDictBeg;

  // read the dictionary from that offset in file
  ReadDictionary_intenal_t(slDictBeg);

  // stop dictionary processing - go to dictionary using
  strm_dmDictionaryMode = DM_ENABLED;

  // return offset of dictionary for later cross-file importing
  if (slImportOffset!=0) {
    return slImportOffset;
  } else {
    return slDictBeg;
  }
}

void CTStream::DictionaryReadEnd_t(void)
{
  if (strm_dmDictionaryMode == DM_ENABLED) {
    ASSERT(strm_slDictionaryPos>0);
    // just skip the dictionary (it was already read)
    SetPos_t(strm_slDictionaryPos);
    strm_slDictionaryPos=0;
    strm_dmDictionaryMode = DM_NONE;
    strm_ntDictionary.Clear();

    // for each filename
    INDEX ctFileNames = strm_afnmDictionary.Count();
    for(INDEX iFileName=0; iFileName<ctFileNames; iFileName++) {
      CTFileName &fnm = strm_afnmDictionary[iFileName];
      // if not preloaded
      if (fnm.fnm_pserPreloaded==NULL) {
        // skip
        continue;
      }
      // free preloaded instance
      CTString strExt = fnm.FileExt();
      if (strExt==".tex") {
        _pTextureStock->Release((CTextureData*)fnm.fnm_pserPreloaded);
      } else if (strExt==".mdl") {
        _pModelStock->Release((CModelData*)fnm.fnm_pserPreloaded);
      }
    }

    strm_afnmDictionary.Clear();
  }
}
void CTStream::DictionaryPreload_t(void)
{
  INDEX ctFileNames = strm_afnmDictionary.Count();
  // for each filename
  for(INDEX iFileName=0; iFileName<ctFileNames; iFileName++) {
    // preload it
    CTFileName &fnm = strm_afnmDictionary[iFileName];
    CTString strExt = fnm.FileExt();
    CallProgressHook_t(FLOAT(iFileName)/ctFileNames);
    try {
      if (strExt==".tex") {
        fnm.fnm_pserPreloaded = _pTextureStock->Obtain_t(fnm);
      } else if (strExt==".mdl") {
        fnm.fnm_pserPreloaded = _pModelStock->Obtain_t(fnm);
      }
    } catch (const char *strError) {
      CPrintF( TRANS("Cannot preload %s: %s\n"), (const char *) (CTString&)fnm, strError);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// General construction/destruction

/* Default constructor. */
CTStream::CTStream(void) : strm_ntDictionary(*new CNameTable_CTFileName)
{
  strm_strStreamDescription = "";
  strm_slDictionaryPos = 0;
  strm_dmDictionaryMode = DM_NONE;
}

/* Destructor. */
CTStream::~CTStream(void)
{
  strm_ntDictionary.Clear();
  strm_afnmDictionary.Clear();

  delete &strm_ntDictionary;
}

/////////////////////////////////////////////////////////////////////////////
// File stream opening/closing methods

/*
 * Default constructor.
 */
CTFileStream::CTFileStream(void)
{
  fstrm_pFile = NULL;
  // mark that file is created for writing
  fstrm_bReadOnly = TRUE;
  fstrm_iZipHandle = -1;
  fstrm_iZipLocation = 0;
  fstrm_pubZipBuffer = NULL;
}

/*
 * Destructor.
 */
CTFileStream::~CTFileStream(void)
{
  // close stream
  if (fstrm_pFile != NULL || fstrm_iZipHandle!=-1) {
    Close();
  }
}

/*
 * Open an existing file.
 */
// throws char *
void CTFileStream::Open_t(const CTFileName &fnFileName, CTStream::OpenMode om/*=OM_READ*/)
{
  // if current thread has not enabled stream handling
  if (!_bThreadCanHandleStreams) {
    // error
    ::ThrowF_t(TRANS("Cannot open file `%s', stream handling is not enabled for this thread"),
      (const char *) (CTString&)fnFileName);
  }


  // check parameters
  ASSERT(strlen(fnFileName)>0);
  // check that the file is not open
  ASSERT(fstrm_pFile==NULL && fstrm_iZipHandle==-1);

  // expand the filename to full path
  CTFileName fnmFullFileName;
  INDEX iFile = ExpandFilePath((om == OM_READ)?EFP_READ:EFP_WRITE, fnFileName, fnmFullFileName);
  
  // CPrintF("Open_t: `%s\n  %s\n",(const char *) (CTString&)fnFileName ,(const char *) (CTString)fnmFullFileName);

  // if read only mode requested
  if( om == OM_READ) {
    // initially, no physical file
    fstrm_pFile = NULL;
    // if zip file
    if( iFile==EFP_MODZIP || iFile==EFP_BASEZIP) {
      // open from zip
      fstrm_iZipHandle = UNZIPOpen_t(fnmFullFileName);
      fstrm_slZipSize = UNZIPGetSize(fstrm_iZipHandle);
      // load the file from the zip in the buffer
      fstrm_pubZipBuffer = new UBYTE[fstrm_slZipSize];
      UNZIPReadBlock_t(fstrm_iZipHandle, fstrm_pubZipBuffer, 0, fstrm_slZipSize);
    // if it is a physical file
    } else if (iFile==EFP_FILE) {
      // open file in read only mode
      fstrm_pFile = fopen(fnmFullFileName, "rb");
    }
    fstrm_bReadOnly = TRUE;
  
  // if write mode requested
  } else if( om == OM_WRITE) {
    // open file for reading and writing
    fstrm_pFile = fopen(fnmFullFileName, "rb+");
    fstrm_bReadOnly = FALSE;
  // if unknown mode
  } else {
    FatalError(TRANS("File stream opening requested with unknown open mode: %d\n"), om);
  }

  // if openning operation was not successfull
  if(fstrm_pFile == NULL && fstrm_iZipHandle==-1) {
    // throw exception
    Throw_t(TRANS("Cannot open file `%s' (%s)"), (const char *) (CTString&)fnmFullFileName,
      strerror(errno));
  }

  // if file opening was successfull, set stream description to file name
  strm_strStreamDescription = fnmFullFileName;
  // add this newly opened file into opened stream list
  _plhOpenedStreams->AddTail( strm_lnListNode);
}

static void MakeSureDirectoryPathExists(const CTFileName &fnmFullFileName)
{
    STUBBED("!!! FIXME: get the code back in from Ryan's original port.");
}

/*
 * Create a new file or overwrite existing.
 */
void CTFileStream::Create_t(const CTFileName &fnFileName,
                            enum CTStream::CreateMode cm) // throws char *
{
  (void)cm; // OBSOLETE!
  
  CTFileName fnFileNameAbsolute = fnFileName;
  fnFileNameAbsolute.SetAbsolutePath();

  // if current thread has not enabled stream handling
  if (!_bThreadCanHandleStreams) {
    // error
    ::ThrowF_t(TRANS("Cannot create file `%s', stream handling is not enabled for this thread"),
      (const char *) (CTString&)fnFileNameAbsolute);
  }

  CTFileName fnmFullFileName;
  /* INDEX iFile = */ ExpandFilePath(EFP_WRITE, fnFileNameAbsolute, fnmFullFileName);

  // check parameters
  ASSERT(strlen(fnFileNameAbsolute)>0);
  // check that the file is not open
  ASSERT(fstrm_pFile == NULL);

  // create the directory for the new file if it doesn't exist yet
  MakeSureDirectoryPathExists(fnmFullFileName);

  // open file stream for writing (destroy file context if file existed before)
  fstrm_pFile = fopen(fnmFullFileName, "wb+");
  // if not successfull
  if(fstrm_pFile == NULL)
  {
    // throw exception
    Throw_t(TRANS("Cannot create file `%s' (%s)"), (const char *) (CTString&)fnmFullFileName,
      strerror(errno));
  }
  // if file creation was successfull, set stream description to file name
  strm_strStreamDescription = fnFileNameAbsolute;
  // mark that file is created for writing
  fstrm_bReadOnly = FALSE;
  // add this newly created file into opened stream list
  _plhOpenedStreams->AddTail( strm_lnListNode);
}

/*
 * Close an open file.
 */
void CTFileStream::Close(void)
{
  // if file is not open
  if (fstrm_pFile==NULL && fstrm_iZipHandle==-1) {
    ASSERT(FALSE);
    return;
  }

  // clear stream description
  strm_strStreamDescription = "";
  // remove file from list of curently opened streams
  strm_lnListNode.Remove();

  // if file on disk
  if (fstrm_pFile != NULL) {
    // close file
    fclose( fstrm_pFile);
    fstrm_pFile = NULL;
  // if file in zip
  } else if (fstrm_iZipHandle>=0) {
    // close zip entry
    UNZIPClose(fstrm_iZipHandle);
    fstrm_iZipHandle = -1;
    delete[] fstrm_pubZipBuffer;
    //VirtualFree(fstrm_pubZipBuffer, 0, MEM_RELEASE);

    _ulVirtuallyAllocatedSpace -= fstrm_slZipSize;
    //CPrintF("Freed virtual memory with size ^c00ff00%d KB^C (now %d KB)\n", (fstrm_slZipSize / 1000), (_ulVirtuallyAllocatedSpace / 1000));
  }

  // clear dictionary vars
  strm_dmDictionaryMode = DM_NONE;
  strm_ntDictionary.Clear();
  strm_afnmDictionary.Clear();
  strm_slDictionaryPos=0;
}

/* Get CRC32 of stream */
ULONG CTFileStream::GetStreamCRC32_t(void)
{
  // if file on disk
  if (fstrm_pFile != NULL) {
    // use base class implementation (really calculates the CRC)
    return CTStream::GetStreamCRC32_t();
  // if file in zip
  } else if (fstrm_iZipHandle >=0) {
    return UNZIPGetCRC(fstrm_iZipHandle);
  } else {
    ASSERT(FALSE);
    return 0;
  }
}

/* Read a block of data from stream. */
void CTFileStream::Read_t(void *pvBuffer, SLONG slSize)
{
  if(fstrm_iZipHandle != -1) {
    memcpy(pvBuffer, fstrm_pubZipBuffer + fstrm_iZipLocation, slSize);
    fstrm_iZipLocation += slSize;
    return;
  }

  size_t x = fread(pvBuffer, slSize, 1, fstrm_pFile);
}

/* Write a block of data to stream. */
void CTFileStream::Write_t(const void *pvBuffer, SLONG slSize)
{
  if(fstrm_bReadOnly || fstrm_iZipHandle != -1) {
    throw "Stream is read-only!";
  }

  fwrite(pvBuffer, slSize, 1, fstrm_pFile);
}

/* Seek in stream. */
void CTFileStream::Seek_t(SLONG slOffset, enum SeekDir sd)
{
  if(fstrm_iZipHandle != -1) {
    switch(sd) {
    case SD_BEG: fstrm_iZipLocation = slOffset; break;
    case SD_CUR: fstrm_iZipLocation += slOffset; break;
    case SD_END: fstrm_iZipLocation = GetSize_t() + slOffset; break;
    }
  } else {
    fseek(fstrm_pFile, slOffset, sd);
  }
}

/* Set absolute position in stream. */
void CTFileStream::SetPos_t(SLONG slPosition)
{
  Seek_t(slPosition, SD_BEG);
}

/* Get absolute position in stream. */
SLONG CTFileStream::GetPos_t(void)
{
  if(fstrm_iZipHandle != -1) {
    return fstrm_iZipLocation;
  } else {
    return ftell(fstrm_pFile);
  }
}

/* Get size of stream */
SLONG CTFileStream::GetStreamSize(void)
{
  if(fstrm_iZipHandle != -1) {
    return UNZIPGetSize(fstrm_iZipHandle);
  } else {
    long lCurrentPos = ftell(fstrm_pFile);
    fseek(fstrm_pFile, 0, SD_END);
    long lRet = ftell(fstrm_pFile);
    fseek(fstrm_pFile, lCurrentPos, SD_BEG);
    return lRet;
  }
}

/* Check if file position points to the EOF */
BOOL CTFileStream::AtEOF(void)
{
  if(fstrm_iZipHandle != -1) {
    return fstrm_iZipLocation >= fstrm_slZipSize;
  } else {
    int eof = feof(fstrm_pFile);
    return eof != 0;
  }
}

// whether or not the given pointer is coming from this stream (mainly used for exception handling)
BOOL CTFileStream::PointerInStream(void* pPointer)
{
  // we're not using virtual allocation buffers so it's fine to return FALSE here.
  return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Memory stream construction/destruction

/*
 * Create dynamically resizing stream for reading/writing.
 */
CTMemoryStream::CTMemoryStream(void)
{
  // if current thread has not enabled stream handling
  if (!_bThreadCanHandleStreams) {
    // error
    ::FatalError(TRANS("Can create memory stream, stream handling is not enabled for this thread"));
  }

  mstrm_ctLocked = 0;
  mstrm_bReadable = TRUE;
  mstrm_bWriteable = TRUE;
  mstrm_slLocation = 0;
  // set stream description
  strm_strStreamDescription = "dynamic memory stream";
  // add this newly created memory stream into opened stream list
  _plhOpenedStreams->AddTail( strm_lnListNode);
  // allocate amount of memory needed to hold maximum allowed file length (when saving)
  mstrm_pubBuffer = new UBYTE[_ulMaxLenghtOfSavingFile];
  mstrm_pubBufferEnd = mstrm_pubBuffer + _ulMaxLenghtOfSavingFile;
  mstrm_pubBufferMax = mstrm_pubBuffer;
}

/*
 * Create static stream from given buffer.
 */
CTMemoryStream::CTMemoryStream(void *pvBuffer, SLONG slSize,
                               CTStream::OpenMode om /*= CTStream::OM_READ*/)
{
  // if current thread has not enabled stream handling
  if (!_bThreadCanHandleStreams) {
    // error
    ::FatalError(TRANS("Can create memory stream, stream handling is not enabled for this thread"));
  }

  // allocate amount of memory needed to hold maximum allowed file length (when saving)
  mstrm_pubBuffer = new UBYTE[_ulMaxLenghtOfSavingFile];
  mstrm_pubBufferEnd = mstrm_pubBuffer + _ulMaxLenghtOfSavingFile;
  mstrm_pubBufferMax = mstrm_pubBuffer + slSize;
  // copy given block of memory into memory file
  memcpy( mstrm_pubBuffer, pvBuffer, slSize);

  mstrm_ctLocked = 0;
  mstrm_bReadable = TRUE;
  mstrm_slLocation = 0;
  // if stram is opened in read only mode
  if( om == OM_READ)
  {
    mstrm_bWriteable = FALSE;
  }
  // otherwise, write is enabled
  else
  {
    mstrm_bWriteable = TRUE;
  }
  // set stream description
  strm_strStreamDescription = "dynamic memory stream";
  // add this newly created memory stream into opened stream list
  _plhOpenedStreams->AddTail( strm_lnListNode);
}

/* Destructor. */
CTMemoryStream::~CTMemoryStream(void)
{
  ASSERT(mstrm_ctLocked==0);
  delete[] mstrm_pubBuffer;
  // remove memory stream from list of curently opened streams
  strm_lnListNode.Remove();
}

/////////////////////////////////////////////////////////////////////////////
// Memory stream buffer operations

/*
 * Lock the buffer contents and it's size.
 */
void CTMemoryStream::LockBuffer(void **ppvBuffer, SLONG *pslSize)
{
  mstrm_ctLocked++;
  ASSERT(mstrm_ctLocked>0);

  *ppvBuffer = mstrm_pubBuffer;
  *pslSize = GetSize_t();
}

/*
 * Unlock buffer.
 */
void CTMemoryStream::UnlockBuffer()
{
  mstrm_ctLocked--;
  ASSERT(mstrm_ctLocked>=0);
}

/////////////////////////////////////////////////////////////////////////////
// Memory stream overrides from CTStream

BOOL CTMemoryStream::IsReadable(void)
{
  return mstrm_bReadable && (mstrm_ctLocked==0);
}
BOOL CTMemoryStream::IsWriteable(void)
{
  return mstrm_bWriteable && (mstrm_ctLocked==0);
}
BOOL CTMemoryStream::IsSeekable(void)
{
  return TRUE;
}

/* Read a block of data from stream. */
void CTMemoryStream::Read_t(void *pvBuffer, SLONG slSize)
{
  memcpy(pvBuffer, mstrm_pubBuffer + mstrm_slLocation, slSize);
  mstrm_slLocation += slSize;
}

/* Write a block of data to stream. */
void CTMemoryStream::Write_t(const void *pvBuffer, SLONG slSize)
{
  memcpy(mstrm_pubBuffer + mstrm_slLocation, pvBuffer, slSize);
  mstrm_slLocation += slSize;

  if(mstrm_pubBuffer + mstrm_slLocation > mstrm_pubBufferMax) {
    mstrm_pubBufferMax = mstrm_pubBuffer + mstrm_slLocation;
  }
}

/* Seek in stream. */
void CTMemoryStream::Seek_t(SLONG slOffset, enum SeekDir sd)
{
  switch(sd) {
  case SD_BEG: mstrm_slLocation = slOffset; break;
  case SD_CUR: mstrm_slLocation += slOffset; break;
  case SD_END: mstrm_slLocation = GetStreamSize() + slOffset; break;
  }
}

/* Set absolute position in stream. */
void CTMemoryStream::SetPos_t(SLONG slPosition)
{
  mstrm_slLocation = slPosition;
}

/* Get absolute position in stream. */
SLONG CTMemoryStream::GetPos_t(void)
{
  return mstrm_slLocation;
}

/* Get size of stream. */
SLONG CTMemoryStream::GetSize_t(void)
{
  return GetStreamSize();
}

/* Get size of stream */
SLONG CTMemoryStream::GetStreamSize(void)
{
  return mstrm_pubBufferMax - mstrm_pubBuffer;
}

/* Get CRC32 of stream */
ULONG CTMemoryStream::GetStreamCRC32_t(void)
{
  return CTStream::GetStreamCRC32_t();
}

/* Check if file position points to the EOF */
BOOL CTMemoryStream::AtEOF(void)
{
  return mstrm_slLocation >= GetStreamSize();
}

// whether or not the given pointer is coming from this stream (mainly used for exception handling)
BOOL CTMemoryStream::PointerInStream(void* pPointer)
{
  return pPointer >= mstrm_pubBuffer && pPointer < mstrm_pubBufferEnd;
}

// Test if a file exists.
BOOL FileExists(const CTFileName &fnmFile)
{
  // if no file
  if (fnmFile=="") {
    // it doesn't exist
    return FALSE;
  }
  // try to
  try {
    // open the file for reading
    CTFileStream strmFile;
    strmFile.Open_t(fnmFile);
    // if successful, it means that it exists,
    return TRUE;
  // if failed, it means that it doesn't exist
  } catch (const char *strError) {
    (void) strError;
    return FALSE;
  }
}

// Test if a file exists for writing. 
// (this is can be diferent than normal FileExists() if a mod uses basewriteexclude.lst
BOOL FileExistsForWriting(const CTFileName &fnmFile)
{
  // if no file
  if (fnmFile=="") {
    // it doesn't exist
    return FALSE;
  }
  // expand the filename to full path for writing
  CTFileName fnmFullFileName;
  /* INDEX iFile = */ ExpandFilePath(EFP_WRITE, fnmFile, fnmFullFileName);

  // check if it exists
  FILE *f = fopen(fnmFullFileName, "rb");
  if (f!=NULL) { 
    fclose(f);
    return TRUE;
  } else {
    return FALSE;
  }
}

// Get file timestamp
SLONG GetFileTimeStamp_t(const CTFileName &fnm)
{
  // expand the filename to full path
  CTFileName fnmExpanded;
  INDEX iFile = ExpandFilePath(EFP_READ, fnm, fnmExpanded);
  if (iFile!=EFP_FILE) {
    return FALSE;
  }

  int file_handle;
  // try to open file for reading
  file_handle = _open( fnmExpanded, _O_RDONLY | _O_BINARY);
  if(file_handle==-1) {
    ThrowF_t(TRANS("Cannot open file '%s' for reading"), (const char *) CTString(fnm));
    return -1;
  }
  struct stat statFileStatus;
  // get file status
  fstat( file_handle, &statFileStatus);
  _close( file_handle);
  ASSERT(statFileStatus.st_mtime<=time(NULL));
  return statFileStatus.st_mtime;
}

// Get CRC32 of a file
ULONG GetFileCRC32_t(const CTFileName &fnmFile) // throw char *
{
  // open the file
  CTFileStream fstrm;
  fstrm.Open_t(fnmFile);
  // return the checksum
  return fstrm.GetStreamCRC32_t();
}

// Test if a file is read only (also returns FALSE if file does not exist)
BOOL IsFileReadOnly(const CTFileName &fnm)
{
  // expand the filename to full path
  CTFileName fnmExpanded;
  INDEX iFile = ExpandFilePath(EFP_READ, fnm, fnmExpanded);
  if (iFile!=EFP_FILE) {
    return FALSE;
  }

  int file_handle;
  // try to open file for reading
  file_handle = _open( fnmExpanded, _O_RDONLY | _O_BINARY);
  if(file_handle==-1) {
    return FALSE;
  }
  struct stat statFileStatus;
  // get file status
  fstat( file_handle, &statFileStatus);
  _close( file_handle);
  ASSERT(statFileStatus.st_mtime<=time(NULL));
  return !(statFileStatus.st_mode&_S_IWRITE);
}

// Delete a file
BOOL RemoveFile(const CTFileName &fnmFile)
{
  // expand the filename to full path
  CTFileName fnmExpanded;
  INDEX iFile = ExpandFilePath(EFP_WRITE, fnmFile, fnmExpanded);
  if (iFile==EFP_FILE) {
    int ires = remove(fnmExpanded);
    return ires==0;
  } else {
    return FALSE;
  }
}


static BOOL IsFileReadable_internal(CTFileName &fnmFullFileName)
{
  FILE *pFile = fopen(fnmFullFileName, "rb");
  if (pFile!=NULL) {
    fclose(pFile);
    return TRUE;
  } else {
    return FALSE;
  }
}

// check for some file extensions that can be substituted
static BOOL SubstExt_internal(CTFileName &fnmFullFileName)
{
  if (fnmFullFileName.FileExt()==".mp3") {
    fnmFullFileName = fnmFullFileName.NoExt()+".ogg";
    return TRUE;
  } else if (fnmFullFileName.FileExt()==".ogg") {
    fnmFullFileName = fnmFullFileName.NoExt()+".mp3";
    return TRUE;
  } else {
    return TRUE;
  }
}


static INDEX ExpandFilePath_read(ULONG ulType, const CTFileName &fnmFile, CTFileName &fnmExpanded)
{
  // search for the file in zips
  INDEX iFileInZip = UNZIPGetFileIndex(fnmFile);
  //const BOOL userdir_not_basedir = (_fnmUserDir != _fnmApplicationPath);

  // if a mod is active
  if (_fnmMod!="") {

    // first try in the mod's dir
    if (!fil_bPreferZips) {
      fnmExpanded = _fnmApplicationPathTMP + _fnmMod + convertWindow1251ToUtf8(fnmFile);
      if (IsFileReadable_internal(fnmExpanded)) {
        return EFP_FILE;
      }
    }

    // if not disallowing zips
    if (!(ulType&EFP_NOZIPS)) {
      // if exists in mod's zip
      if (iFileInZip>=0 && UNZIPIsFileAtIndexMod(iFileInZip)) {
        // use that one
        fnmExpanded = fnmFile;
        return EFP_MODZIP;
      }
    }

    // try in the mod's dir after
    if (fil_bPreferZips) {
      fnmExpanded = _fnmApplicationPathTMP + _fnmMod + convertWindow1251ToUtf8(fnmFile);
      if (IsFileReadable_internal(fnmExpanded)) {
        return EFP_FILE;
      }
    }
  }

  // try in the app's base dir
  if (!fil_bPreferZips) {
    CTFileName fnmAppPath = _fnmApplicationPathTMP;
    fnmAppPath.SetAbsolutePath();

    if(fnmFile.HasPrefix(fnmAppPath)) {
      fnmExpanded = convertWindow1251ToUtf8(fnmFile);
    } else {
      fnmExpanded = _fnmApplicationPathTMP + convertWindow1251ToUtf8(fnmFile);
    }

    if (IsFileReadable_internal(fnmExpanded)) {
      return EFP_FILE;
    }
  }

  // if not disallowing zips
  if (!(ulType&EFP_NOZIPS)) {
    // if exists in any zip
    if (iFileInZip>=0) {
      // use that one
      fnmExpanded = fnmFile;
      return EFP_BASEZIP;
    }
  }

  // try in the app's base dir
  if (fil_bPreferZips) {
    fnmExpanded = _fnmApplicationPathTMP + convertWindow1251ToUtf8(fnmFile);
    if (IsFileReadable_internal(fnmExpanded)) {
      return EFP_FILE;
    }
  }

  // finally, try in the CD path
  if (_fnmCDPath!="") {

    // if a mod is active
    if (_fnmMod!="") {
      // first try in the mod's dir
      fnmExpanded = _fnmCDPath + _fnmMod + convertWindow1251ToUtf8(fnmFile);
      if (IsFileReadable_internal(fnmExpanded)) {
        return EFP_FILE;
      }
    }

    fnmExpanded = _fnmCDPath + convertWindow1251ToUtf8(fnmFile);
    if (IsFileReadable_internal(fnmExpanded)) {
      return EFP_FILE;
    }
  }
  return EFP_NONE;
}


// rcg01042002 User dir and children may need to be created on the fly...
static void VerifyDirsExist(const char *_path)
{
  char *path = (char *) AllocMemory(strlen(_path) + 1);
  strcpy(path, _path);
  const char *dirsep = CFileSystem::GetDirSeparator();

    // skip first dirsep. This assumes an absolute path and some other
    //  fundamentals of how a filepath is specified.
  char *ptr = strstr(path, dirsep);
  ASSERT(ptr != NULL);
  if (ptr == NULL)
    return;

  for (ptr = strstr(ptr+1, dirsep); ptr != NULL; ptr = strstr(ptr+1, dirsep)) {
    char ch = *ptr;
    *ptr = '\0'; // terminate the path.
    if (!_pFileSystem->IsDirectory(path)) {
      if (_pFileSystem->Exists(path)) {
        CPrintF("Expected %s to be a directory, but it's a file!\n", path);
        break;
      } else {
        CPrintF("Creating directory %s ...\n", path);
        _mkdir(path);
        if (!_pFileSystem->IsDirectory(path)) {
          CPrintF("Creation of directory %s FAILED!\n", path);
          break;
        }
      }
    }

    *ptr = ch;  // put path char back...
  }

  FreeMemory(path);
}

// Expand a file's filename to full path
INDEX ExpandFilePath(ULONG ulType, const CTFileName &fnmFile, CTFileName &fnmExpanded)
{
  CTFileName fnmFileAbsolute = fnmFile;
  fnmFileAbsolute.SetAbsolutePath();

#ifdef PLATFORM_WIN32
  _fnmApplicationPathTMP = _fnmApplicationPath;
#else
//#######################################################################################################################
//###############                             users files in home dir                ####################################
//#######################################################################################################################
  //CTFileName _fnmApplicationPathTMP; ... move to begin file

  int _savegame       = strncmp((const char *)fnmFile, (const char *) "SaveGame", (size_t) 8 );
  int _usercontrols   = strncmp((const char *)fnmFile, (const char *) "Controls", (size_t) 8 );
  int _persistentsym  = strncmp((const char *)fnmFile, (const char *) "Scripts/PersistentSymbols.ini", (size_t) 29 );
  int _gamesgms       = strncmp((const char *)fnmFile, (const char *) "Data/SeriousSam.gms", (size_t) 19 );
  int _comsolehistory = strncmp((const char *)fnmFile, (const char *) "Temp/ConsoleHistory.txt", (size_t) 23 );
  int _userdemos      = strncmp((const char *)fnmFile, (const char *) "Demos/Demo", (size_t) 10 );
  int _playersplr     = strncmp((const char *)fnmFile, (const char *) "Players", (size_t) 7 );
  int _screenshots    = strncmp((const char *)fnmFile, (const char *) "ScreenShots", (size_t) 11 );
  int _levelsvis      = strncmp((const char *)fnmFile, (const char *) "Levels", (size_t) 6 );

  //CPrintF("ExpandFilePath: %s\n",(const char *) fnmFile);

  if(( _savegame == 0 || _persistentsym == 0 || _gamesgms == 0 ||
    _comsolehistory == 0 || _userdemos == 0 || _playersplr == 0 || _screenshots == 0) && ( _bPortableVersion == FALSE)) {
       _fnmApplicationPathTMP = _fnmUserDir;
  } else {
       _fnmApplicationPathTMP = _fnmApplicationPath;
  }

  if( _levelsvis == 0 && _bPortableVersion == FALSE) {
    if (fnmFileAbsolute.FileExt()==".vis") {
       _fnmApplicationPathTMP = _fnmUserDir;
    }
  }

  if( _usercontrols == 0 && _bPortableVersion == FALSE) {
    CTFileName _fnSControls = fnmFileAbsolute.FileName();
    int _controls   = strncmp((const char *)_fnSControls, (const char *) "Controls", (size_t) 8 );
    if ( _controls == 0 ) {
       _fnmApplicationPathTMP = _fnmUserDir;
    }
  }

  if( _pShell->GetINDEX("sys_iSysPath") == 1 && fnmFileAbsolute.FileExt()==".so" ) {
    _fnmApplicationPathTMP = _fnmModLibPath;
    fnmFileAbsolute = fnmFileAbsolute.FileName() + fnmFileAbsolute.FileExt();
  }

//#######################################################################################################################
#endif
  // if writing
  if (ulType&EFP_WRITE) {
    // if should write to mod dir
    if (_fnmMod!="" && (!FileMatchesList(_afnmBaseWriteInc, fnmFileAbsolute) || FileMatchesList(_afnmBaseWriteExc, fnmFileAbsolute))) {
      // do that
      fnmExpanded = _fnmApplicationPathTMP + _fnmMod + convertWindow1251ToUtf8(fnmFileAbsolute);
      fnmExpanded.SetAbsolutePath();
#ifdef PLATFORM_UNIX
      VerifyDirsExist(fnmExpanded.FileDir());
#endif
      return EFP_FILE;
    // if should not write to mod dir
    } else {
      // write to base dir
      fnmExpanded = _fnmApplicationPathTMP + convertWindow1251ToUtf8(fnmFileAbsolute);
      fnmExpanded.SetAbsolutePath();
#ifdef PLATFORM_UNIX
      VerifyDirsExist(fnmExpanded.FileDir());
#endif
      return EFP_FILE;
    }

  // if reading
  } else if (ulType&EFP_READ) {

    // check for expansions for reading
    INDEX iRes = ExpandFilePath_read(ulType, fnmFileAbsolute, fnmExpanded);
    // if not found
    if (iRes==EFP_NONE) {
      //check for some file extensions that can be substituted
      CTFileName fnmSubst = fnmFileAbsolute;
      if (SubstExt_internal(fnmSubst)) {
        iRes = ExpandFilePath_read(ulType, fnmSubst, fnmExpanded);
      }
    }

    fnmExpanded.SetAbsolutePath();

    if (iRes!=EFP_NONE) {
      return iRes;
    }

  // in other cases
  } else  {
    ASSERT(FALSE);
    fnmExpanded = _fnmApplicationPathTMP + convertWindow1251ToUtf8(fnmFileAbsolute);
    fnmExpanded.SetAbsolutePath();
    return EFP_FILE;
  }

  fnmExpanded = _fnmApplicationPathTMP + convertWindow1251ToUtf8(fnmFileAbsolute);
  fnmExpanded.SetAbsolutePath();
  return EFP_NONE;
}

