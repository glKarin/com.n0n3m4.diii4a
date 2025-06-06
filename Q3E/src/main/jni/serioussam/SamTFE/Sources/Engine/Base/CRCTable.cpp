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

#include <Engine/Base/CRCTable.h>
#include <Engine/Base/FileName.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/CRC.h>
#include <Engine/Templates/DynamicStackArray.cpp>

extern INDEX net_bReportCRC;

class CCRCEntry {
public:
  CTFileName ce_fnmFile;    // the file that CRC is for
  ULONG ce_ulCRC;           // CRC of the file
  BOOL ce_bActive;          // set if the file is now active for CRC checking

  // filename is its name (used for storing in nametable)
  inline const CTFileName &GetName(void) { return ce_fnmFile; };
  void Clear(void) 
  {
    ce_fnmFile.Clear(); 
    ce_ulCRC = 0;
    ce_bActive = FALSE;
  }
};

extern CDynamicStackArray<CTFileName> _afnmNoCRC;
extern BOOL FileMatchesList(CDynamicStackArray<CTFileName> &afnm, const CTFileName &fnm);

#define TYPE CCRCEntry
#define CNameTable_TYPE CNameTable_CCRCEntry
#define CNameTableSlot_TYPE CNameTableSlot_CCRCEntry
#include <Engine/Templates/NameTable.h>
#include <Engine/Templates/NameTable.cpp>
#undef CNameTableSlot_TYPE
#undef CNameTable_TYPE
#undef TYPE

static CDynamicStackArray<CCRCEntry> _aceEntries;
static CNameTable_CCRCEntry _ntEntries;

__extern BOOL CRCT_bGatherCRCs = FALSE;  // set while gathering CRCs of all loaded files

// init CRC table
void CRCT_Init(void)
{
  _ntEntries.SetAllocationParameters(50, 10, 10);
}

// check if a file is added
BOOL CRCT_IsFileAdded(const CTFileName &fnm)
{
  return _ntEntries.Find(fnm)!=NULL;
}

// add one file to active list and get its crc
void CRCT_AddFile_t(const CTFileName &fnm, ULONG ulCRC/*=0*/) // throw char *
{
  // if not gathering CRCs now
  if (!CRCT_bGatherCRCs) {
    // do nothing
    return;
  }

  // try to find it in table
  CCRCEntry *pce = _ntEntries.Find(fnm);

  BOOL bNew = FALSE;
  // if found
  if (pce!=NULL) {
    // just activate it
    bNew = !pce->ce_bActive;
    pce->ce_bActive = TRUE;
    // if crc is given
    if (ulCRC!=0) {
      // force it
      pce->ce_ulCRC = ulCRC;
    }
  // if not found
  } else {
    // calculate checksum
    if (ulCRC==0) {
      
      if (FileMatchesList(_afnmNoCRC, fnm)) {
        ulCRC = 0x12345678;
      } else {
        ulCRC = GetFileCRC32_t(fnm);
      }
    }
    // add to the table
    pce = &_aceEntries.Push();
    pce->ce_fnmFile = fnm;
    pce->ce_ulCRC = ulCRC;
    pce->ce_bActive = TRUE;
    _ntEntries.Add(pce);
    bNew = TRUE;
  }
  if (bNew && net_bReportCRC) {
    CPrintF("CRC %08x: '%s'\n", pce->ce_ulCRC, (const char*)pce->ce_fnmFile);
  }
}

// free all memory used by the crc cache
void CRCT_Clear(void)
{
  _ntEntries.Clear();
  _aceEntries.Clear();
}

// reset all files to not active
void CRCT_ResetActiveList(void)
{
  for(INDEX ice=0; ice<_aceEntries.Count(); ice++) {
    _aceEntries[ice].ce_bActive = FALSE;
  }
}


static INDEX GetNumberOfActiveEntries(void)
{
  INDEX ctActive = 0;
  for(INDEX ice=0; ice<_aceEntries.Count(); ice++) {
    if (_aceEntries[ice].ce_bActive) {
      ctActive++;
    }
  }
  return ctActive;
}

// dump list of all active files to the stream
void CRCT_MakeFileList_t(CTStream &strmFiles)  // throw char *
{
  // save number of active entries
  INDEX ctActive = GetNumberOfActiveEntries();
  strmFiles<<ctActive;
  // for each active entry
  for(INDEX ice=0; ice<_aceEntries.Count(); ice++) {
    CCRCEntry &ce = _aceEntries[ice];
    if (!ce.ce_bActive) {
      continue;
    }
    // save name to stream
    strmFiles<<(CTString&)ce.ce_fnmFile;
  }
}

// dump checksums for all files from the list
ULONG CRCT_MakeCRCForFiles_t(CTStream &strmFiles)  // throw char *
{
  BOOL bOld = CRCT_bGatherCRCs;
  CRCT_bGatherCRCs = TRUE;

  ULONG ulCRC;
  CRC_Start(ulCRC);
  // read number of active files
  INDEX ctFiles;
  strmFiles>>ctFiles;
  // for each one
  for(INDEX i=0; i<ctFiles; i++) {
    // read the name
    CTString strName;
    strmFiles>>strName;
    CTFileName fname = strName;
    // try to find it in table
    CCRCEntry *pce = _ntEntries.Find(fname);
    // if not there
    if (pce==NULL) {
      CRCT_AddFile_t(fname);
      // add it now
      pce = _ntEntries.Find(fname);
    }
    // add the crc
    CRC_AddLONG(ulCRC, pce->ce_ulCRC);
  }
  CRCT_bGatherCRCs = bOld;
  CRC_Finish(ulCRC);
  return ulCRC;
}


