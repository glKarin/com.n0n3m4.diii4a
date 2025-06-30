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

#include <Engine/Base/FileName.h>

#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/FileSystem.h>
#include <Engine/Templates/NameTable_CTFileName.h>
#include <Engine/Templates/DynamicStackArray.cpp>

template class CDynamicArray<CTFileName>;
template class CDynamicStackArray<CTFileName>;
#include <Engine/Templates/StaticStackArray.cpp>
template class CStaticStackArray<long>;


const char *CTFileName::convertFromWin32(const char *src)
{
#if (defined PLATFORM_WIN32)
    return(src);
#else
    static const char *dirsep = NULL;
    static size_t seplen = 0;
    static char buf[MAX_PATH];  // This is NOT thread safe, fyi.
    char *dest = buf;

    if (src == NULL)
    {
        buf[0] = '\0';
        return(buf);
    }

    if (dirsep == NULL)
    {
        dirsep = CFileSystem::GetDirSeparator();
        seplen = strlen(dirsep);
    }

    for (dest = buf; *src != '\0'; src++)
    {
        if (*src == '\\')
        {
            strcpy(dest, dirsep);
            dest += seplen;
        }
        else
        {
            *(dest++) = *src;
        }
    }

    *dest = '\0';
    return(buf);
#endif
}


const char *CTFileName::convertToWin32(const char *src)
{
#if (defined PLATFORM_WIN32)
    return(src);
#else
    static const char *dirsep = NULL;
    static size_t seplen = 0;
    static char buf[MAX_PATH];  // This is NOT thread safe, fyi.
    char *dest = buf;

    if (src == NULL)
    {
        buf[0] = '\0';
        return(buf);
    }

    if (dirsep == NULL)
    {
        dirsep = CFileSystem::GetDirSeparator();
        seplen = strlen(dirsep);
    }

    for (dest = buf; *src != '\0'; src++)
    {
        if ((*src == *dirsep) && (strncmp(src, dirsep, seplen) == 0))
        {
            *(dest++) = '\\';
            src += (seplen - 1);
        }
        else
        {
            *(dest++) = *src;
        }
    }

    *dest = '\0';
    return(buf);
#endif
}


#define USE_ABSTRACT_CTFILENAME 1


/*
 * Get directory part of a filename.
 */
CTFileName CTFileName::FileDir() const
{
  ASSERT(IsValid());

  // make a temporary copy of string
  CTFileName strPath(*this);
  // find last backlash in it

#ifdef USE_ABSTRACT_CTFILENAME
  const char *dirsep = CFileSystem::GetDirSeparator();
  char *pPathBackSlash = strstr( strPath.str_String, dirsep);
  // if there is no backslash
  if( pPathBackSlash == NULL) {
    // return emptystring as directory
    return( CTFileName(""));
  }

  for (char *p = pPathBackSlash;
        (p = strstr(p + 1, dirsep)) != NULL;
        pPathBackSlash = p)
  {
      // (*yawn*).
  }

  // set end of string after where the backslash was
  pPathBackSlash[strlen(dirsep)] = 0;

#else

  char *pPathBackSlash = strrchr( strPath.str_String, '\\');
  pPathBackSlash[1] = 0;
#endif

  // return a copy of temporary string
  return( CTFileName( strPath));
}

CTFileName &CTFileName::operator=(const char *strCharString)
{
  ASSERTALWAYS( "Use CTFILENAME for conversion from char *!");
  return *this;
}

/*
 * Get name part of a filename.
 */
CTFileName CTFileName::FileName() const
{
  ASSERT(IsValid());

  // make a temporary copy of string
  CTFileName strPath(*this);

  // find last backlash in what's left
#ifdef USE_ABSTRACT_CTFILENAME
  const char *dirsep = CFileSystem::GetDirSeparator();
  char *pBackSlash = strstr( strPath.str_String, dirsep);
  // if there is no backslash
  if( pBackSlash == NULL) {
    // return it all as filename
    pBackSlash = strPath.str_String;
  } else {
    for (char *p = pBackSlash;
          (p = strstr(p + 1, dirsep)) != NULL;
          pBackSlash = p)
    {
        // (*yawn*).
    }

    pBackSlash += strlen(dirsep);
  }

  // find last dot in it
  char *pDot = strrchr(pBackSlash, '.');
  // if there is a dot
  if( pDot != NULL) {
    // set end of string there
    *pDot = '\0';
  }

  // return a copy of temporary string, starting after the backslash
  return( CTFileName( pBackSlash ));

#else

  char *pBackSlash = strrchr( strPath.str_String, '\\');

  // if there is no backslash
  if( pBackSlash == NULL) {
    // return it all as filename
    return( CTFileName(strPath));
  }

  // return a copy of temporary string, starting after the backslash
  return( CTFileName( pBackSlash+1));
#endif
}

/*
 * Get extension part of a filename.
 */
CTFileName CTFileName::FileExt() const
{
  ASSERT(IsValid());

  // find last dot in the string
  char *pExtension = strrchr( str_String, '.');
  // if there is no dot
  if( pExtension == NULL) {
    // return no extension
    return( CTFileName(""));
  }
  // return a copy of the extension part, together with the dot
  return( CTFileName( pExtension));
}

CTFileName CTFileName::NoExt() const
{
  return FileDir()+FileName();
}

/*
 * Remove application path from a file name.
 */
void CTFileName::RemoveApplicationPath_t(void) // throws char *
{
  // remove the path string from beginning of the string
  BOOL bHadRightPath = RemovePrefix(_fnmApplicationPath);
  if (_fnmMod!="") {
    RemovePrefix(_fnmApplicationPath+_fnmMod);
  }
  // if it had wrong path
  if (!bHadRightPath) {
    // throw error
    ThrowF_t(TRANS("File '%s' has got wrong path!\nAll files must reside in directory '%s'."),
      str_String, (const char *) (CTString&)_fnmApplicationPath);
  }
}

/*
 * Read from stream.
 */
 CTStream &operator>>(CTStream &strmStream, CTFileName &fnmFileName)
{
  // if dictionary is enabled
  if (strmStream.strm_dmDictionaryMode == CTStream::DM_ENABLED) {
    // read the index in dictionary
    INDEX iFileName;
    strmStream>>iFileName;
    // get that file from the dictionary
    fnmFileName = strmStream.strm_afnmDictionary[iFileName];

  // if dictionary is processing or not active
  } else {
    char strTag[] = "_FNM"; strTag[0] = 'D';  // must create tag at run-time!
    // skip dependency catcher header
    strmStream.ExpectID_t(strTag);    // data filename

    // read the string
#ifdef PLATFORM_WIN32
    strmStream>>(CTString &)fnmFileName;
#else
    CTString ctstr;
    strmStream>>ctstr;
    fnmFileName = CTString(CTFileName::convertFromWin32(ctstr));  // converts from win32 paths.
#endif

    fnmFileName.fnm_pserPreloaded = NULL;
  }

  return strmStream;
}

/*
 * Write to stream.
 */
 CTStream &operator<<(CTStream &strmStream, const CTFileName &fnmFileName)
{
  // if dictionary is enabled
  if (strmStream.strm_dmDictionaryMode == CTStream::DM_ENABLED) {
    // try to find the filename in dictionary
    CTFileName *pfnmExisting = strmStream.strm_ntDictionary.Find(fnmFileName);
    // if not existing
    if (pfnmExisting==NULL) {
      // add it
      pfnmExisting = &strmStream.strm_afnmDictionary.Push();
      *pfnmExisting = fnmFileName;
      strmStream.strm_ntDictionary.Add(pfnmExisting);
    }
    // write its index
    strmStream<<strmStream.strm_afnmDictionary.Index(pfnmExisting);

  // if dictionary is processing or not active
  } else {
    char strTag[] = "_FNM"; strTag[0] = 'D';  // must create tag at run-time!
    // write dependency catcher header
    strmStream.WriteID_t(strTag);     // data filename
    // write the string
#ifdef PLATFORM_WIN32
    strmStream<<(CTString &)fnmFileName;
#else
    strmStream<<CTString(CTFileName::convertToWin32(fnmFileName));
#endif
  }

  return strmStream;
}


// rcg01062002
CTString CTFileName::Win32FmtString(void) const
{
    return(CTString(convertToWin32(*this)));
}


void CTFileName::ReadFromText_t(CTStream &strmStream,
                                const CTString &strKeyword) // throw char *
{
  ASSERT(IsValid());

  char strTag[] = "_FNM "; strTag[0] = 'T';  // must create tag at run-time!
  // keyword must be present
  strmStream.ExpectKeyword_t(strKeyword);
  // after the user keyword, dependency keyword must be present
  strmStream.ExpectKeyword_t(strTag);

  // read the string from the file
  char str[1024];
  strmStream.GetLine_t(str, sizeof(str));
  fnm_pserPreloaded = NULL;

  // copy it here
  (*this) = CTString( (const char *)str);
}
