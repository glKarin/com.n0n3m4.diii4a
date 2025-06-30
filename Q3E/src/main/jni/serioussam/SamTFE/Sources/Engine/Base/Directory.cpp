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
#include <Engine/Base/Stream.h>
#include <Engine/Base/FileName.h>
#include <Engine/Base/Unzip.h>
#include <Engine/Base/FileSystem.h>
#include <Engine/Templates/DynamicStackArray.cpp>
#ifndef PLATFORM_UNIX
#include <io.h>
#endif

extern CDynamicStackArray<CTFileName> _afnmBaseBrowseInc;
extern CDynamicStackArray<CTFileName> _afnmBaseBrowseExc;

class CDirToRead {
public:
  CListNode dr_lnNode;
  CTString dr_strDir;
};

int qsort_CompareCTFileName(const void *elem1, const void *elem2 )
{
  const CTFileName &fnm1 = **(CTFileName **)elem1;
  const CTFileName &fnm2 = **(CTFileName **)elem2;
  return strcmp(fnm1, fnm2);
}

extern BOOL FileMatchesList(CDynamicStackArray<CTFileName> &afnm, const CTFileName &fnm);

void FillDirList_internal(const CTFileName &fnmBasePath,
  CDynamicStackArray<CTFileName> &afnm, const CTFileName &fnmDir, const CTString &strPattern, BOOL bRecursive,
  CDynamicStackArray<CTFileName> *pafnmInclude, CDynamicStackArray<CTFileName> *pafnmExclude)
{
  // add the directory to list of directories to search
  CListHead lhDirs;
  CDirToRead *pdrFirst = new CDirToRead;
  pdrFirst->dr_strDir = fnmDir;
  lhDirs.AddTail(pdrFirst->dr_lnNode);

  // while the list of directories is not empty
  while (!lhDirs.IsEmpty()) {
    // take the first one
    CDirToRead *pdr = LIST_HEAD(lhDirs, CDirToRead, dr_lnNode);
    CTFileName fnmDir = pdr->dr_strDir;
    delete pdr;

    // if the dir is not allowed
    if (pafnmInclude!=NULL &&
      (!FileMatchesList(*pafnmInclude, fnmDir) || FileMatchesList(*pafnmExclude, fnmDir)) ) {
      // skip it
      continue;
    }
    
    const char *dirsep = CFileSystem::GetDirSeparator();

    #ifdef PLATFORM_UNIX
    // start listing the directory
    CDynamicArray<CTString> *files;
    files = _pFileSystem->FindFiles(fnmBasePath+fnmDir, "*");
    int max = files->Count();

    // for each file in the directory
    for (int i = 0; i < max; i++) {
      const char *fname = (*files)[i];

      // if dummy dir (this dir, parent dir, or any dir starting with '.')
      if (_pFileSystem->IsDummyFile(fname)) {
        // skip it
        continue;
      }

      // get the file's filepath
      CTFileName fnm = fnmDir + fname;

      // if it is a directory
      if (_pFileSystem->IsDirectory(fnmBasePath+fnm)) {
    #else
    // start listing the directory
    struct _finddata_t c_file; intptr_t hFile;
    hFile = _findfirst( (const char *)(fnmBasePath+fnmDir+"*"), &c_file );
    
    // for each file in the directory
    for (
      BOOL bFileExists = hFile!=-1; 
      bFileExists; 
      bFileExists = _findnext( hFile, &c_file )==0) {

      // if dummy dir (this dir, parent dir, or any dir starting with '.')
      if (c_file.name[0]=='.') {
        // skip it
        continue;
      }

      // get the file's filepath
      CTFileName fnm = fnmDir+c_file.name;

      // if it is a directory
      if (c_file.attrib&_A_SUBDIR) {
    #endif
        // if recursive reading
        if (bRecursive) {
          // add it to the list of directories to search
          CDirToRead *pdrNew = new CDirToRead;
          pdrNew->dr_strDir = fnm + dirsep;
          lhDirs.AddTail(pdrNew->dr_lnNode);
        }
      // if it matches the pattern
      } else if (strPattern=="" || fnm.Matches(strPattern)) {
        // add that file
        afnm.Push() = fnm;
      }
    }
    #ifdef PLATFORM_UNIX
    delete files;
    #endif
  }
}


// make a list of all files in a directory
ENGINE_API void MakeDirList(
  CDynamicStackArray<CTFileName> &afnmDir, const CTFileName &fnmDir, const CTFileName &fnmPattern, ULONG ulFlags)
{
  afnmDir.PopAll();
  BOOL bRecursive = ulFlags&DLI_RECURSIVE;
  BOOL bSearchCD  = ulFlags&DLI_SEARCHCD;

  // make one temporary array
  CDynamicStackArray<CTFileName> afnm;

  if (_fnmMod!="") {
    FillDirList_internal(_fnmUserDir, afnm, fnmDir, fnmPattern, bRecursive,
      &_afnmBaseBrowseInc, &_afnmBaseBrowseExc);
    FillDirList_internal(_fnmApplicationPath, afnm, fnmDir, fnmPattern, bRecursive,
      &_afnmBaseBrowseInc, &_afnmBaseBrowseExc);
    if (bSearchCD) {
      FillDirList_internal(_fnmCDPath, afnm, fnmDir, fnmPattern, bRecursive,
      &_afnmBaseBrowseInc, &_afnmBaseBrowseExc);
    }
    FillDirList_internal(_fnmUserDir+_fnmMod, afnm, fnmDir, fnmPattern, bRecursive, NULL, NULL);
    FillDirList_internal(_fnmApplicationPath+_fnmMod, afnm, fnmDir, fnmPattern, bRecursive, NULL, NULL);
  } else {
    FillDirList_internal(_fnmUserDir, afnm, fnmDir, fnmPattern, bRecursive, NULL, NULL);
    FillDirList_internal(_fnmApplicationPath, afnm, fnmDir, fnmPattern, bRecursive, NULL, NULL);
    if (bSearchCD) {
      FillDirList_internal(_fnmCDPath, afnm, fnmDir, fnmPattern, bRecursive, NULL, NULL);
    }
  }

  // for each file in zip archives
  CTString strDirPattern = fnmDir;
  INDEX ctFilesInZips = UNZIPGetFileCount();
  for(INDEX iFileInZip=0; iFileInZip<ctFilesInZips; iFileInZip++) {
    const CTFileName &fnm = UNZIPGetFileAtIndex(iFileInZip);

    // if not in this dir, skip it
    if (bRecursive) {
      if (!fnm.HasPrefix(strDirPattern)) {
        continue;
      }
    } else {
      if (fnm.FileDir()!=fnmDir) {
        continue;
      }
    }

    // if doesn't match pattern
    if (fnmPattern!="" && !fnm.Matches(fnmPattern)) {
      // skip it
      continue;
    }

    // if mod is active, and the file is not in mod
    if (_fnmMod!="" && !UNZIPIsFileAtIndexMod(iFileInZip)) {
      // if it doesn't match base browse path
      if ( !FileMatchesList(_afnmBaseBrowseInc, fnm) || FileMatchesList(_afnmBaseBrowseExc, fnm) ) {
        // skip it
        continue;
      }
    }

    // add that file
    afnm.Push() = fnm;
  }

  // if no files
  if (afnm.Count()==0) {
    // don't check for duplicates
    return;
  }

  // resort the array
  qsort(afnm.da_Pointers, afnm.Count(), sizeof(void*), qsort_CompareCTFileName);

  // for each file
  INDEX ctFiles = afnm.Count();
  for (INDEX iFile=0; iFile<ctFiles; iFile++) {
    // if not same as last one
    if (iFile==0 || afnm[iFile]!=afnm[iFile-1]) {
      // copy over to final array
      afnmDir.Push() = afnm[iFile];
    }
  }
}
