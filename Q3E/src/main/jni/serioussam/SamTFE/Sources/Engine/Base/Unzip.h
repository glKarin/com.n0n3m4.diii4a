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

#ifndef SE_INCL_UNZIP_H
#define SE_INCL_UNZIP_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// add one zip archive to current active set
void UNZIPAddArchive(const CTFileName &fnm);
// read directories of all currently added archives, in reverse alphabetical order
void UNZIPReadDirectoriesReverse_t(void);
// check if a zip file entry exists
BOOL UNZIPFileExists(const CTFileName &fnm);
// open a zip file entry for reading
INDEX UNZIPOpen_t(const CTFileName &fnm);
// get uncompressed size of a file
SLONG UNZIPGetSize(INDEX iHandle);
// get CRC of a file
ULONG UNZIPGetCRC(INDEX iHandle);
// read a block from zip file
void UNZIPReadBlock_t(INDEX iHandle, UBYTE *pub, SLONG slStart, SLONG slLen);
// close a zip file entry
void UNZIPClose(INDEX iHandle);
// get info on a zip file entry
void UNZIPGetFileInfo(INDEX iHandle, CTFileName &fnmZip,
  SLONG &slOffset, SLONG &slSizeCompressed, SLONG &slSizeUncompressed, 
  BOOL &bCompressed);
// enumeration for all files in all zips
INDEX UNZIPGetFileCount(void);
const CTFileName &UNZIPGetFileAtIndex(INDEX i);

// get index of a file (-1 for no file)
INDEX UNZIPGetFileIndex(const CTFileName &fnm);
// check if a file is from a mod's zip
BOOL UNZIPIsFileAtIndexMod(INDEX i);


#endif  /* include-once check. */

