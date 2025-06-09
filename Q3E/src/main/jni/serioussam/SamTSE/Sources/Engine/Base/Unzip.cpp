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

// unzip.cpp : Defines the entry point for the console application.
//

#include "Engine/StdH.h"
#include <Engine/Base/Stream.h>
#include <Engine/Base/FileName.h>
#include <Engine/Base/Translation.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/Synchronization.h>
#include <Engine/Math/Functions.h>

#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>

#ifdef PLATFORM_MACOSX
typedef unsigned char Byte;  // !!! FIXME:  not sure why I suddenly needed this typedef here.
#endif

#if defined(PLATFORM_UNIX) && !SE1_ZLIB
#include <zlib.h>
#else
#include <Engine/zlib/zlib.h>
#endif
extern CTCriticalSection zip_csLock; // critical section for access to zlib functions

#pragma pack(1)

// before each file in the zip
#define SIGNATURE_LFH 0x04034b50
struct LocalFileHeader {
  SWORD lfh_swVersionToExtract;
  SWORD lfh_swGPBFlag;
  SWORD lfh_swCompressionMethod;
  SWORD lfh_swModFileTime;
  SWORD lfh_swModFileDate;
  SLONG lfh_slCRC32;
  SLONG lfh_slCompressedSize;
  SLONG lfh_slUncompressedSize;
  SWORD lfh_swFileNameLen;
  SWORD lfh_swExtraFieldLen;

// follows:
//	filename (variable size)
//  extra field (variable size)
};
  
// after file data, only if compressed from a non-seekable stream
// this exists only if bit 3 in GPB flag is set
#define SIGNATURE_DD 0x08074b50
struct DataDescriptor {
	SLONG dd_slCRC32;
	SLONG dd_slCompressedSize;
	SLONG dd_slUncompressedSize;
};

// one file in central dir
#define SIGNATURE_FH 0x02014b50
struct FileHeader {
  SWORD fh_swVersionMadeBy;
  SWORD fh_swVersionToExtract;
  SWORD fh_swGPBFlag;
  SWORD fh_swCompressionMethod;
  SWORD fh_swModFileTime;
  SWORD fh_swModFileDate;
  SLONG fh_slCRC32;
  SLONG fh_slCompressedSize;
  SLONG fh_slUncompressedSize;
  SWORD fh_swFileNameLen;
  SWORD fh_swExtraFieldLen;
  SWORD fh_swFileCommentLen;
  SWORD fh_swDiskNoStart;
  SWORD fh_swInternalFileAttributes;
  SLONG fh_swExternalFileAttributes;
  SLONG fh_slLocalHeaderOffset;

// follows:
//  filename (variable size)
//  extra field (variable size)
//  file comment (variable size)
};

// at the end of entire zip file
#define SIGNATURE_EOD 0x06054b50
struct EndOfDir {
  SWORD eod_swDiskNo;
  SWORD eod_swDirStartDiskNo;
  SWORD eod_swEntriesInDirOnThisDisk;
  SWORD eod_swEntriesInDir;
  SLONG eod_slSizeOfDir;
  SLONG eod_slDirOffsetInFile;
  SWORD eod_swCommentLength;
// follows: 
//  zipfile comment (variable size)
};

#pragma pack()

// one entry (a zipped file) in a zip archive
class CZipEntry {
public:
  CTFileName *ze_pfnmArchive;   // path of the archive
  CTFileName ze_fnm;            // file name with path inside archive
  SLONG ze_slCompressedSize;    // size of file in the archive
  SLONG ze_slUncompressedSize;  // size when uncompressed
  SLONG ze_slDataOffset;        // position of compressed data inside archive
  ULONG ze_ulCRC;               // checksum of the file
  BOOL ze_bStored;              // set if file is not compressed, but stored
  BOOL ze_bMod;                 // set if from a mod's archive

  void Clear(void)
  {
    ze_pfnmArchive = NULL;
    ze_fnm.Clear();
  }
};

// an open instance of a file inside a zip
class CZipHandle {
public:
  BOOL zh_bOpen;          // set if the handle is used
  CZipEntry zh_zeEntry;   // the entry itself
  z_stream zh_zstream;    // zlib filestream for decompression
  FILE *zh_fFile;         // open handle of the archive
#define BUF_SIZE  1024
  UBYTE *zh_pubBufIn;     // input buffer

  CZipHandle(void);
  void Clear(void);
  void ThrowZLIBError_t(int ierr, const CTString &strDescription);
};

// get error string for a zlib error
CTString GetZlibError(int ierr)
{
  switch(ierr) {
  case Z_OK           : return TRANS("Z_OK           "); break;
  case Z_STREAM_END   : return TRANS("Z_STREAM_END   "); break;   
  case Z_NEED_DICT    : return TRANS("Z_NEED_DICT    "); break;
  case Z_STREAM_ERROR : return TRANS("Z_STREAM_ERROR "); break;
  case Z_DATA_ERROR   : return TRANS("Z_DATA_ERROR   "); break;
  case Z_MEM_ERROR    : return TRANS("Z_MEM_ERROR    "); break; 
  case Z_BUF_ERROR    : return TRANS("Z_BUF_ERROR    "); break;
  case Z_VERSION_ERROR: return TRANS("Z_VERSION_ERROR"); break;
  case Z_ERRNO        : {
    CTString strError;
    strError.PrintF(TRANSV("Z_ERRNO: %s"), strerror(errno));
    return strError; 
                        } break;
  default: {
    CTString strError;
    strError.PrintF(TRANSV("Unknown ZLIB error: %d"), ierr);
    return strError; 
           } break;
  }
}

CZipHandle::CZipHandle(void) 
{
  zh_bOpen = FALSE;
  zh_fFile = NULL;
  zh_pubBufIn = NULL;
  memset(&zh_zstream, 0, sizeof(zh_zstream));
}
void CZipHandle::Clear(void) 
{
  zh_bOpen = FALSE;
  zh_zeEntry.Clear();

  // clear the zlib stream
  CTSingleLock slZip(&zip_csLock, TRUE);
  inflateEnd(&zh_zstream);
  memset(&zh_zstream, 0, sizeof(zh_zstream));

  // free buffers
  if (zh_pubBufIn!=NULL) {
    FreeMemory(zh_pubBufIn);
    zh_pubBufIn = NULL;
  }
  // close the zip archive file
  if (zh_fFile!=NULL) {
    fclose(zh_fFile);
    zh_fFile = NULL;
  }
}
void CZipHandle::ThrowZLIBError_t(int ierr, const CTString &strDescription)
{
  ThrowF_t(TRANS("(%s/%s) %s - ZLIB error: %s - %s"), 
    (const char *) (const CTString&)*zh_zeEntry.ze_pfnmArchive,
    (const char *) (const CTString&)zh_zeEntry.ze_fnm,
    (const char *) strDescription,
    (const char *) GetZlibError(ierr),
    (const char *) zh_zstream.msg);
}

// all files in all active zip archives
static CStaticStackArray<CZipEntry>  _azeFiles;
// handles for currently open files
static CStaticStackArray<CZipHandle> _azhHandles;
// filenames of all archives
static CStaticStackArray<CTFileName> _afnmArchives;

// convert slashes to backslashes in a file path
void ConvertSlashes(char *p)
{
  while (*p!=0) {
    if (*p=='/') {
      *p = '\\';
    }
    p++;
  }
}

#define READ_ZIPFIELD(f, x) { size_t _size = fread(&x, sizeof(x), 1, f); BYTESWAP(x); }

// read directory of a zip archive and add all files in it to active set
void ReadZIPDirectory_t(CTFileName *pfnmZip)
{

  FILE *f = fopen(*pfnmZip, "rb");
  if (f==NULL) {
    ThrowF_t(TRANS("%s: Cannot open file (%s)"), (const char *) (CTString&)*pfnmZip, strerror(errno));
  }
  // start at the end of file, minus expected minimum overhead
  fseek(f, 0, SEEK_END);
  int iPos = ftell(f)-sizeof(SLONG)-sizeof(EndOfDir)+2;
  // do not search more than 128k (should be around 65k at most)
  int iMinPos = iPos-128*1024;
  if (iMinPos<0) {
    iMinPos = 0;
  }

  EndOfDir eod;
  BOOL bEODFound = FALSE;
  // while not at beginning
  for(; iPos>iMinPos; iPos--) {
    // read signature
    fseek(f, iPos, SEEK_SET);
    SLONG slSig;
    size_t _size = fread(&slSig, sizeof(slSig), 1, f);
    BYTESWAP(slSig);
    // if this is the sig
    if (slSig==SIGNATURE_EOD) {
      // read directory end
      READ_ZIPFIELD(f, eod.eod_swDiskNo);
      READ_ZIPFIELD(f, eod.eod_swDirStartDiskNo);
      READ_ZIPFIELD(f, eod.eod_swEntriesInDirOnThisDisk);
      READ_ZIPFIELD(f, eod.eod_swEntriesInDir);
      READ_ZIPFIELD(f, eod.eod_slSizeOfDir);
      READ_ZIPFIELD(f, eod.eod_slDirOffsetInFile);
      READ_ZIPFIELD(f, eod.eod_swCommentLength);

      // if multi-volume zip
      if (eod.eod_swDiskNo!=0||eod.eod_swDirStartDiskNo!=0
        ||eod.eod_swEntriesInDirOnThisDisk!=eod.eod_swEntriesInDir) {
        // fail
        ThrowF_t(TRANS("%s: Multi-volume zips are not supported"), (const char *) (CTString&)*pfnmZip);
      }                                                     
      // check against empty zips
      if (eod.eod_swEntriesInDir<=0) {
        // fail
        ThrowF_t(TRANS("%s: Empty zip"), (const char *) (CTString&)*pfnmZip);
      }                                                     
      // all ok
      bEODFound = TRUE;
      break;
    }
  }
  // if eod not found
  if (!bEODFound) {
    // fail
    ThrowF_t(TRANS("%s: Cannot find 'end of central directory'"), (const char *) (CTString&)*pfnmZip);
  }

  // check if the zip is from a mod
  BOOL bMod = 
    pfnmZip->HasPrefix(_fnmApplicationPath+"Mods/") || 
    pfnmZip->HasPrefix(_fnmCDPath+"Mods/");

  // go to the beginning of the central dir
  fseek(f, eod.eod_slDirOffsetInFile, SEEK_SET);
  INDEX ctFiles = 0;
  // for each file
  for (INDEX iFile=0; iFile<eod.eod_swEntriesInDir; iFile++) {
    // read the sig
    SLONG slSig;
    size_t _size = fread(&slSig, sizeof(slSig), 1, f);
    BYTESWAP(slSig);

    // if this is not the expected sig
    if (slSig!=SIGNATURE_FH) {
      // fail
      ThrowF_t(TRANS("%s: Wrong signature for 'file header' number %d'"), 
        (const char *) (CTString&)*pfnmZip, iFile);
    }
    // read its header
    FileHeader fh;
    READ_ZIPFIELD(f, fh.fh_swVersionMadeBy);
    READ_ZIPFIELD(f, fh.fh_swVersionToExtract);
    READ_ZIPFIELD(f, fh.fh_swGPBFlag);
    READ_ZIPFIELD(f, fh.fh_swCompressionMethod);
    READ_ZIPFIELD(f, fh.fh_swModFileTime);
    READ_ZIPFIELD(f, fh.fh_swModFileDate);
    READ_ZIPFIELD(f, fh.fh_slCRC32);
    READ_ZIPFIELD(f, fh.fh_slCompressedSize);
    READ_ZIPFIELD(f, fh.fh_slUncompressedSize);
    READ_ZIPFIELD(f, fh.fh_swFileNameLen);
    READ_ZIPFIELD(f, fh.fh_swExtraFieldLen);
    READ_ZIPFIELD(f, fh.fh_swFileCommentLen);
    READ_ZIPFIELD(f, fh.fh_swDiskNoStart);
    READ_ZIPFIELD(f, fh.fh_swInternalFileAttributes);
    READ_ZIPFIELD(f, fh.fh_swExternalFileAttributes);
    READ_ZIPFIELD(f, fh.fh_slLocalHeaderOffset);

    // read the filename
    const SLONG slMaxFileName = 512;
    char strBuffer[slMaxFileName+1];
    memset(strBuffer, 0, sizeof(strBuffer));
    if (fh.fh_swFileNameLen>slMaxFileName) {
      ThrowF_t(TRANS("%s: Too long filepath in zip"), (const char *) (CTString&)*pfnmZip);
    }
    if (fh.fh_swFileNameLen<=0) {
      ThrowF_t(TRANS("%s: Invalid filepath length in zip"), (const char *) (CTString&)*pfnmZip);
    }
    _size = fread(strBuffer, fh.fh_swFileNameLen, 1, f);

    // skip eventual comment and extra fields
    if (fh.fh_swFileCommentLen+fh.fh_swExtraFieldLen>0) {
      fseek(f, fh.fh_swFileCommentLen+fh.fh_swExtraFieldLen, SEEK_CUR);
    }

    // if the file is directory
    if (strBuffer[strlen(strBuffer)-1]=='/') {
      // check size
      if (fh.fh_slUncompressedSize!=0
        ||fh.fh_slCompressedSize!=0) {
        ThrowF_t(TRANS("%s/%s: Invalid directory"), 
          (const char *) (CTString&)*pfnmZip, strBuffer);
      }

    // if the file is real file
    } else {
      ctFiles++;
      // convert filename
      ConvertSlashes(strBuffer);
      // create a new entry
      CZipEntry &ze = _azeFiles.Push();
      // remember the file's data
      ze.ze_fnm = CTString(strBuffer);
      ze.ze_pfnmArchive = pfnmZip;
      ze.ze_slCompressedSize = fh.fh_slCompressedSize;
      ze.ze_slUncompressedSize = fh.fh_slUncompressedSize;
      ze.ze_slDataOffset = fh.fh_slLocalHeaderOffset;
      ze.ze_ulCRC = fh.fh_slCRC32;
      ze.ze_bMod = bMod;
      // check for compressopn
      if (fh.fh_swCompressionMethod==0) {
        ze.ze_bStored = TRUE;
      } else if (fh.fh_swCompressionMethod==8) {
        ze.ze_bStored = FALSE;
      } else {
        ThrowF_t(TRANS("%s/%s: Only 'deflate' compression is supported"),
          (const char *) (CTString&)*ze.ze_pfnmArchive,
          (const char *) ze.ze_fnm);
      }
    }
  }

  // if error reading
  if (ferror(f)) {
    // fail
    ThrowF_t(TRANS("%s: Error reading central directory"), (const char *) (CTString&)*pfnmZip);
  }

  // report that file was read
  CPrintF(TRANSV("  %s: %d files\n"), (const char *) (CTString&)*pfnmZip, ctFiles++);
}

// add one zip archive to current active set
void UNZIPAddArchive(const CTFileName &fnm)
{
  // remember its filename
  CTFileName &fnmNew = _afnmArchives.Push();
  fnmNew = fnm;
} 

// read directory of an archive
void ReadOneArchiveDir_t(CTFileName &fnm)
{
  // remember current number of files
  INDEX ctOrgFiles = _azeFiles.Count();

  // try to
  try {
    // read the directory and add all files
    ReadZIPDirectory_t(&fnm);
  // if failed
  } catch (const char *) {
    // if some files were added
    if (ctOrgFiles<_azeFiles.Count()) {
      // remove them
      if (ctOrgFiles==0) {
        _azeFiles.PopAll();
      } else {
        _azeFiles.PopUntil(ctOrgFiles-1);
      }
    }
    // cascade the error
    throw;
  }
}

int qsort_ArchiveCTFileName_reverse(const void *elem1, const void *elem2 )
{                   
  // get the filenames
  const CTFileName &fnm1 = *(CTFileName *)elem1;
  const CTFileName &fnm2 = *(CTFileName *)elem2;

#ifdef PLATFORM_WIN32
  // find if any is in a mod or on CD
  BOOL bMod1 = fnm1.HasPrefix(_fnmApplicationPath+"Mods\\");
  BOOL bCD1 = fnm1.HasPrefix(_fnmCDPath);
  BOOL bModCD1 = fnm1.HasPrefix(_fnmCDPath+"Mods\\");
  BOOL bMod2 = fnm2.HasPrefix(_fnmApplicationPath+"Mods\\");
  BOOL bCD2 = fnm2.HasPrefix(_fnmCDPath);
  BOOL bModCD2 = fnm2.HasPrefix(_fnmCDPath+"Mods\\");
#else
  // find if any is in a mod or on CD
  BOOL bMod1 = fnm1.HasPrefix(_fnmApplicationPath+"Mods/");
  BOOL bCD1 = fnm1.HasPrefix(_fnmCDPath);
  BOOL bModCD1 = fnm1.HasPrefix(_fnmCDPath+"Mods/");
  BOOL bMod2 = fnm2.HasPrefix(_fnmApplicationPath+"Mods/");
  BOOL bCD2 = fnm2.HasPrefix(_fnmCDPath);
  BOOL bModCD2 = fnm2.HasPrefix(_fnmCDPath+"Mods/");
#endif

  // calculate priorities based on location of gro file
  INDEX iPriority1 = 0;
  if (bMod1) {
    iPriority1 = 3;
  } else if (bModCD1) {
    iPriority1 = 2;
  } else if (bCD1) {
    iPriority1 = 0;
  } else {
    iPriority1 = 1;
  }

  INDEX iPriority2 = 0;
  if (bMod2) {
    iPriority2 = 3;
  } else if (bModCD2) {
    iPriority2 = 2;
  } else if (bCD2) {
    iPriority2 = 0;
  } else {
    iPriority2 = 1;
  }

  // find sorting order
  if (iPriority1<iPriority2) {
    return +1;
  } else if (iPriority1>iPriority2) {
    return -1;
  } else {
    return -stricmp(fnm1, fnm2);
  }
}
// read directories of all currently added archives, in reverse alphabetical order
void UNZIPReadDirectoriesReverse_t(void)
{
  // if no archives
  if (_afnmArchives.Count()==0) {
    // do nothing
    return;
  }

  // sort the archive filenames reversely
  qsort(&_afnmArchives[0], _afnmArchives.Count(), sizeof(CTFileName), 
    qsort_ArchiveCTFileName_reverse);

  CTString strAllErrors = "";
  // for each archive
  for (INDEX iArchive=0; iArchive<_afnmArchives.Count(); iArchive++) {
    //try to
    try {
      // read its directory
      ReadOneArchiveDir_t(_afnmArchives[iArchive]);
    // if failed
    } catch (const char *strError) {
      // remember the error
      strAllErrors += strError;
      strAllErrors += "\n";
    }
  }

  // if there were errors
  if (strAllErrors!="") {
    // report them
    ThrowF_t("%s", (const char *) strAllErrors);
  }
}

// check if a zip file entry exists
BOOL UNZIPFileExists(const CTFileName &fnm)
{
  // for each file
  for(INDEX iFile=0; iFile<_azeFiles.Count(); iFile++) {
    // if it is that one
    if (_azeFiles[iFile].ze_fnm == fnm) {
      return TRUE;
    }
  }
  return FALSE;
}

// enumeration for all files in all zips
INDEX UNZIPGetFileCount(void)
{
  return _azeFiles.Count();
}
const CTFileName &UNZIPGetFileAtIndex(INDEX i)
{
  return _azeFiles[i].ze_fnm;
}

BOOL UNZIPIsFileAtIndexMod(INDEX i)
{
  return _azeFiles[i].ze_bMod;
}

// get index of a file (-1 for no file)
INDEX UNZIPGetFileIndex(const CTFileName &fnm)
{
  // for each file
  for(INDEX iFile=0; iFile<_azeFiles.Count(); iFile++) {
    // if it is that one
    if (_azeFiles[iFile].ze_fnm == fnm) {
      return iFile;
    }
  }
  return -1;
}

// get info on a zip file entry
void UNZIPGetFileInfo(INDEX iHandle, CTFileName &fnmZip, 
  SLONG &slOffset, SLONG &slSizeCompressed, SLONG &slSizeUncompressed, 
  BOOL &bCompressed)
{
  // check handle number
  if(iHandle<0 || iHandle>=_azhHandles.Count()) {
    ASSERT(FALSE);
    return;
  }
  // get the handle
  CZipHandle &zh = _azhHandles[iHandle];
  // check the handle
  if (!zh.zh_bOpen) {
    ASSERT(FALSE);
    return;
  }

  // get parameters
  fnmZip = *zh.zh_zeEntry.ze_pfnmArchive;
  bCompressed = !zh.zh_zeEntry.ze_bStored;
  slOffset = zh.zh_zeEntry.ze_slDataOffset;
  slSizeCompressed = zh.zh_zeEntry.ze_slCompressedSize;
  slSizeUncompressed = zh.zh_zeEntry.ze_slUncompressedSize;
}

// open a zip file entry for reading
INDEX UNZIPOpen_t(const CTFileName &fnm)
{
  CZipEntry *pze = NULL;
  // for each file
  for(INDEX iFile=0; iFile<_azeFiles.Count(); iFile++) {
    // if it is that one
    if (_azeFiles[iFile].ze_fnm == fnm) {
      // stop searching
      pze = &_azeFiles[iFile];
      break;
    }
  }

  // if not found
  if (pze==NULL) {
    // fail
    ThrowF_t(TRANS("File not found: %s"), (const char *) (const CTString&)fnm);
  }

  // for each existing handle
  BOOL bHandleFound = FALSE;
  INDEX iHandle;
  for (iHandle=1; iHandle<_azhHandles.Count(); iHandle++) {
    // if unused
    if (!_azhHandles[iHandle].zh_bOpen) {
      // use that one
      bHandleFound = TRUE;
      break;
    }
  }
  // if no free handle found
  if (!bHandleFound) {
    // create a new one
    iHandle = _azhHandles.Count();
    _azhHandles.Push(1);
  }
  
  // get the handle
  CZipHandle &zh = _azhHandles[iHandle];
  ASSERT(!zh.zh_bOpen);
  zh.zh_zeEntry = *pze;

  // open zip archive for reading
  zh.zh_fFile = fopen(*pze->ze_pfnmArchive, "rb");
  // if failed to open it
  if (zh.zh_fFile==NULL) {
    // clear the handle
    zh.Clear();
    // fail
    ThrowF_t(TRANS("Cannot open '%s': %s"), (const char *) (const CTString&)*pze->ze_pfnmArchive,
      strerror(errno));
  }
  // seek to the local header of the entry
  fseek(zh.zh_fFile, zh.zh_zeEntry.ze_slDataOffset, SEEK_SET);
  // read the sig
  SLONG slSig;
  size_t _size = fread(&slSig, sizeof(slSig), 1, zh.zh_fFile);
  BYTESWAP(slSig);
  // if this is not the expected sig
  if (slSig!=SIGNATURE_LFH) {
    // fail
    ThrowF_t(TRANS("%s/%s: Wrong signature for 'local file header'"), 
      (const char *) (CTString&)*zh.zh_zeEntry.ze_pfnmArchive, (const char *) zh.zh_zeEntry.ze_fnm);
  }
  // read the header
  LocalFileHeader lfh;
  READ_ZIPFIELD(zh.zh_fFile, lfh.lfh_swVersionToExtract);
  READ_ZIPFIELD(zh.zh_fFile, lfh.lfh_swGPBFlag);
  READ_ZIPFIELD(zh.zh_fFile, lfh.lfh_swCompressionMethod);
  READ_ZIPFIELD(zh.zh_fFile, lfh.lfh_swModFileTime);
  READ_ZIPFIELD(zh.zh_fFile, lfh.lfh_swModFileDate);
  READ_ZIPFIELD(zh.zh_fFile, lfh.lfh_slCRC32);
  READ_ZIPFIELD(zh.zh_fFile, lfh.lfh_slCompressedSize);
  READ_ZIPFIELD(zh.zh_fFile, lfh.lfh_slUncompressedSize);
  READ_ZIPFIELD(zh.zh_fFile, lfh.lfh_swFileNameLen);
  READ_ZIPFIELD(zh.zh_fFile, lfh.lfh_swExtraFieldLen);

  // determine exact compressed data position
  zh.zh_zeEntry.ze_slDataOffset = 
    ftell(zh.zh_fFile)+lfh.lfh_swFileNameLen+lfh.lfh_swExtraFieldLen;
  // seek there
  fseek(zh.zh_fFile, zh.zh_zeEntry.ze_slDataOffset, SEEK_SET);

  // allocate buffers
  zh.zh_pubBufIn  = (UBYTE*)AllocMemory(BUF_SIZE);

  // initialize zlib stream
  CTSingleLock slZip(&zip_csLock, TRUE);
  zh.zh_zstream.next_out  = NULL;
  zh.zh_zstream.avail_out = 0;
  zh.zh_zstream.next_in   = NULL;
  zh.zh_zstream.avail_in  = 0;
  zh.zh_zstream.zalloc = (alloc_func)Z_NULL;
  zh.zh_zstream.zfree = (free_func)Z_NULL;
  int err = inflateInit2(&zh.zh_zstream, -15);  // 32k windows
  // if failed
  if (err!=Z_OK) {
    // clean up what is possible
    FreeMemory(zh.zh_pubBufIn );
    zh.zh_pubBufIn  = NULL;
    fclose(zh.zh_fFile);
    zh.zh_fFile = NULL;
    // throw error
    zh.ThrowZLIBError_t(err, TRANS("Cannot init inflation"));
  }

  // return the handle successfully
  zh.zh_bOpen = TRUE;
  return iHandle;
}

// get uncompressed size of a file
SLONG UNZIPGetSize(INDEX iHandle)
{
  // check handle number
  if(iHandle<0 || iHandle>=_azhHandles.Count()) {
    ASSERT(FALSE);
    return 0;
  }
  // get the handle
  CZipHandle &zh = _azhHandles[iHandle];
  // check the handle
  if (!zh.zh_bOpen) {
    ASSERT(FALSE);
    return 0;
  }

  return zh.zh_zeEntry.ze_slUncompressedSize;
}

// get CRC of a file
ULONG UNZIPGetCRC(INDEX iHandle)
{
  // check handle number
  if(iHandle<0 || iHandle>=_azhHandles.Count()) {
    ASSERT(FALSE);
    return 0;
  }
  // get the handle
  CZipHandle &zh = _azhHandles[iHandle];
  // check the handle
  if (!zh.zh_bOpen) {
    ASSERT(FALSE);
    return 0;
  }

  return zh.zh_zeEntry.ze_ulCRC;
}

// read a block from zip file
void UNZIPReadBlock_t(INDEX iHandle, UBYTE *pub, SLONG slStart, SLONG slLen)
{
  // check handle number
  if(iHandle<0 || iHandle>=_azhHandles.Count()) {
    ASSERT(FALSE);
    return;
  }
  // get the handle
  CZipHandle &zh = _azhHandles[iHandle];
  // check the handle
  if (!zh.zh_bOpen) {
    ASSERT(FALSE);
    return;
  }

  // if behind the end of file
  if (slStart>=zh.zh_zeEntry.ze_slUncompressedSize) {
    // do nothing
    return;
  }

  // clamp length to end of the entry data
  slLen = Min(slLen, zh.zh_zeEntry.ze_slUncompressedSize-slStart);

  // if not compressed
  if (zh.zh_zeEntry.ze_bStored) {
    // just read from file
    fseek(zh.zh_fFile, zh.zh_zeEntry.ze_slDataOffset+slStart, SEEK_SET);
    size_t _size = fread(pub, 1, slLen, zh.zh_fFile);
    return;
  }

  CTSingleLock slZip(&zip_csLock, TRUE);

  // if behind the current pointer
  if (static_cast<ULONG>(slStart) < zh.zh_zstream.total_out) {
    // reset the zlib stream to beginning
    inflateReset(&zh.zh_zstream);
    zh.zh_zstream.avail_in = 0;
    zh.zh_zstream.next_in = NULL;
    // seek to start of zip entry data inside archive
    fseek(zh.zh_fFile, zh.zh_zeEntry.ze_slDataOffset, SEEK_SET);
  }

  // while ahead of the current pointer
  while (static_cast<ULONG>(slStart) > zh.zh_zstream.total_out) {
    // if zlib has no more input
    while(zh.zh_zstream.avail_in==0) {
      // read more to it
      SLONG slRead = fread(zh.zh_pubBufIn, 1, BUF_SIZE, zh.zh_fFile);
      if (slRead<=0) {
        return; // !!!!
      }
      // tell zlib that there is more to read
      zh.zh_zstream.next_in = zh.zh_pubBufIn;
      zh.zh_zstream.avail_in  = slRead;
    }
    // read dummy data from the output
    #define DUMMY_SIZE 256
    UBYTE aubDummy[DUMMY_SIZE];
    // decode to output
    zh.zh_zstream.avail_out = Min(SLONG(slStart-zh.zh_zstream.total_out), SLONG(DUMMY_SIZE));
    zh.zh_zstream.next_out = aubDummy;
    int ierr = inflate(&zh.zh_zstream, Z_SYNC_FLUSH);
    if (ierr!=Z_OK && ierr!=Z_STREAM_END) {
      zh.ThrowZLIBError_t(ierr, TRANS("Error seeking in zip"));
    }
  }

  // if not streaming continuously
  if (static_cast<ULONG>(slStart) != zh.zh_zstream.total_out) {
    // this should not happen
    ASSERT(FALSE);
    // read empty
    memset(pub, 0, slLen);
    return;
  }

  // set zlib for writing to the block
  zh.zh_zstream.avail_out = slLen;
  zh.zh_zstream.next_out = pub;

  // while there is something to write to given block
  while (zh.zh_zstream.avail_out>0) {
    // if zlib has no more input
    while(zh.zh_zstream.avail_in==0) {
      // read more to it
      SLONG slRead = fread(zh.zh_pubBufIn, 1, BUF_SIZE, zh.zh_fFile);
      if (slRead<=0) {
        return; // !!!!
      }
      // tell zlib that there is more to read
      zh.zh_zstream.next_in = zh.zh_pubBufIn;
      zh.zh_zstream.avail_in  = slRead;
    }
    // decode to output
    int ierr = inflate(&zh.zh_zstream, Z_SYNC_FLUSH);
    if (ierr!=Z_OK && ierr!=Z_STREAM_END) {
      zh.ThrowZLIBError_t(ierr, TRANS("Error reading from zip"));
    }
  }
}

// close a zip file entry
void UNZIPClose(INDEX iHandle)
{
  // check handle number
  if(iHandle<0 || iHandle>=_azhHandles.Count()) {
    ASSERT(FALSE);
    return;
  }
  // get the handle
  CZipHandle &zh = _azhHandles[iHandle];
  // check the handle
  if (!zh.zh_bOpen) {
    ASSERT(FALSE);
    return;
  }
  // clear it
  zh.Clear();
}
