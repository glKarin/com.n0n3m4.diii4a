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

#ifndef SE_INCL_GROUPFILE_H
#define SE_INCL_GROUPFILE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/FileName.h>

/*
 * Class containing info about files inside group file
 */
class ENGINE_API CGroupFileFileInfo {
public:
  CTFileName gffi_fnFileName; // full name of the file (relative to application path)
  ULONG gffi_ulFileDataOffset; // file data block offset from beginning of the file
  ULONG gffi_ulFileSize; // size of sub group file
  /* Read one file info from stream. */
  void Read_t(CTStream *istr); // throw char *
  /* Write one file info into stream. */
  void Write_t(CTStream *ostr); // throw char *
};


#endif  /* include-once check. */

