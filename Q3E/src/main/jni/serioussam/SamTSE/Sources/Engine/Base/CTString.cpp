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

#include <Engine/Base/CTString.h>
#include <Engine/Base/Memory.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>

#ifndef _MSC_VER
#include <stdio.h>   // for vsscanf()
#include <stdarg.h>
#endif

/*
 * Equality comparison.
 */
BOOL CTString::operator==(const CTString &strOther) const
{
  ASSERT(IsValid() && strOther.IsValid());

  return stricmp( str_String, strOther.str_String) == 0;
}
BOOL CTString::operator==(const char *strOther) const
{
  ASSERT(IsValid() && strOther!=NULL);

  return stricmp( str_String, strOther) == 0;
}
 BOOL operator==(const char *strThis, const CTString &strOther)
{
  ASSERT(strOther.IsValid() && strThis!=NULL);

  return strOther == strThis;
}

/*
 * Inequality comparison.
 */
BOOL CTString::operator!=(const CTString &strOther) const
{
  ASSERT(IsValid() && strOther.IsValid());

  return !( *this == strOther );
}
BOOL CTString::operator!=(const char *strOther) const
{
  ASSERT(IsValid() && strOther!=NULL);

  return !( *this == strOther );
}
 BOOL operator!=(const char *strThis, const CTString &strOther)
{
  ASSERT(strOther.IsValid() && strThis!=NULL);

  return !( strOther == strThis);
}


/*
 * String concatenation.
 */
CTString CTString::operator+(const CTString &strSecond) const
{
  ASSERT(IsValid() && strSecond.IsValid());

  return(CTString(*this)+=strSecond);
}
CTString operator+(const char *strFirst, const CTString &strSecond)
{
  ASSERT(strFirst!=NULL && strSecond.IsValid());

  return(CTString(strFirst)+=strSecond);
}
CTString &CTString::operator+=(const CTString &strSecond)
{
  ASSERT(IsValid() && strSecond.IsValid());

  GrowMemory( (void **)&str_String, strlen( str_String) + strlen( strSecond) + 1 );
  strcat(str_String, strSecond.str_String);
  return *this;
}

/*
 * Remove given prefix string from this string
 */
BOOL CTString::RemovePrefix( const CTString &strPrefix)
{
  INDEX lenPrefix = strlen( strPrefix);
  INDEX lenDest = strlen( str_String) - lenPrefix;

  if( strnicmp( str_String, strPrefix, lenPrefix) != 0)
    return FALSE;
  CTString strTemp = CTString( &str_String[ lenPrefix]);
  ShrinkMemory( (void **)&str_String, lenDest+1);
  strcpy( str_String, strTemp.str_String);
  return TRUE;
}
/* Check if has given prefix */
BOOL CTString::HasPrefix( const CTString &strPrefix) const
{
  INDEX lenPrefix = strlen( strPrefix);
  if( strnicmp( str_String, strPrefix, lenPrefix) != 0)
    return FALSE;
  return TRUE;
}

/* Find index of a substring in a string (returns -1 if not found). */
INDEX CTString::FindSubstr(const CTString &strSub)
{
  INDEX ct = Length();
  INDEX ctSub = strSub.Length();
  for (INDEX i=0; i<ct-ctSub+1; i++) {
    for (INDEX iSub=0; iSub<ctSub; iSub++) {
      if ((*this)[i+iSub]!=strSub[iSub]) {
        goto wrong;
      }
    }
    return i;
wrong:;
  }
  return -1;
}

/* Replace a substring in a string. */
BOOL CTString::ReplaceSubstr(const CTString &strSub, const CTString &strNewSub)
{
  INDEX iPos = FindSubstr(strSub);
  if (iPos<0) {
    return FALSE;
  }

  CTString strPart1, strPart2;
  Split(iPos, strPart1, strPart2);
  strPart2.RemovePrefix(strSub);

  *this = strPart1+strNewSub+strPart2;

  return TRUE;
}

/* Trim the string from left to contain at most given number of characters. */
INDEX CTString::TrimLeft( INDEX ctCharacters)
{
  // clamp negative values
  if( ctCharacters<0) ctCharacters = 0;
  // find how much characters to remove
  INDEX lenOriginal = strlen(str_String);
  INDEX lenPrefix = lenOriginal-ctCharacters;
  // if nothing needs to be removed
  if( lenPrefix<=0) return 0;
  // crop
  memmove( str_String, &str_String[ lenPrefix], ctCharacters+1);
  ShrinkMemory( (void **)&str_String, ctCharacters+1);
  return lenPrefix;
}

/* Trim the string from right to contain at most given number of characters. */
INDEX CTString::TrimRight( INDEX ctCharacters)
{
  // clamp negative values
  if( ctCharacters<0) ctCharacters = 0;
  // find how much characters to remove
  INDEX lenOriginal = strlen(str_String);
  INDEX lenPrefix = lenOriginal-ctCharacters;
  // if nothing needs to be removed
  if( lenPrefix<=0) return 0;
  // crop
  str_String[ctCharacters] = '\0';
  ShrinkMemory( (void**)&str_String, ctCharacters+1);
  return lenPrefix;
}


// return naked length of the string (ignoring all decorate codes)
INDEX CTString::LengthNaked(void) const
{
  return Undecorated().Length();
}

// strip decorations from the string
CTString CTString::Undecorated(void) const
{
  // make a copy of the string to hold the result - we will rewrite it without the codes
  CTString strResult = *this;

  // start at the beginning of both strings
  const char *pchSrc = str_String;
  char *pchDst = strResult.str_String;

  // while the source is not finished
  while(pchSrc[0]!=0) {
    // if the source char is not escape char
    if (pchSrc[0]!='^') {
      // copy it over
      *pchDst++ = *pchSrc++;
      // go to next char
      continue;
    }
    // check the next char
    switch(pchSrc[1]) {
      // if one of the control codes, skip corresponding number of characters
      case 'c':  pchSrc += 2+FindZero((UBYTE*)pchSrc+2,6);  break;
      case 'a':  pchSrc += 2+FindZero((UBYTE*)pchSrc+2,2);  break;
      case 'f':  pchSrc += 2+FindZero((UBYTE*)pchSrc+2,2);  break;
      case 'b':  case 'i':  case 'r':  case 'o':
      case 'C':  case 'A':  case 'F':  case 'B':  case 'I':  pchSrc+=2;  break;
      // if it is the escape char again, skip the first escape and copy the char
      case '^':  pchSrc++; *pchDst++ = *pchSrc++; break;
      // if it is something else
      default:
        // just copy over the control char
        *pchDst++ = *pchSrc++;
        break;
    }
  }
  *pchDst++ = 0;
  ASSERT(strResult.Length()<=Length());
  return strResult;
}

BOOL IsSpace(char c)
{
  return c==' ' || c=='\t' || c=='\n' || c=='\r';
}

/* Trim the string from from spaces from left. */
INDEX CTString::TrimSpacesLeft(void)
{
  // for each character in string
  const char *chr;
  for(chr = str_String; *chr!=0; chr++) {
    // if the character is not space 
    if (!IsSpace(*chr)) {
      // stop searching
      break;
    }
  }
  // trim to that character
  return TrimLeft(str_String+strlen(str_String) - chr);
}

/* Trim the string from from spaces from right. */
INDEX CTString::TrimSpacesRight(void)
{
  // for each character in string reversed
  const char *chr;
  for(chr = str_String+strlen(str_String)-1; chr>str_String; chr--) {
    // if the character is not space 
    if (!IsSpace(*chr)) {
      // stop searching
      break;
    }
  }
  // trim to that character
  return TrimRight(chr-str_String+1);
}


// retain only first line of the string
void CTString::OnlyFirstLine(void)
{
  // get position of first line end
  const char *pchNL = strchr(str_String, '\n');
  // if none
  if (pchNL==NULL) {
    // do nothing
    return;
  }
  // trim everything after that char
  TrimRight(pchNL-str_String);
}


/* Calculate hashing value for the string. */
ULONG CTString::GetHash(void) const
{
  ULONG ulKey = 0;
  INDEX len = strlen(str_String);
#ifdef PLATFORM_WIN32
#define  rotl _rotl
#endif
  for(INDEX i=0; i<len; i++) {
    ulKey = rotl(ulKey,4)+toupper(str_String[i]);
  }
  return ulKey;
}

/*
 * Throw exception
 */
void CTString::Throw_t(void)
{
  throw(str_String);
}


/*
 * Read from stream.
 */
CTStream &operator>>(CTStream &strmStream, CTString &strString)
{
  ASSERT(strString.IsValid());

  // read length
  INDEX iLength;
  strmStream>>iLength;
  ASSERT(iLength>=0);
  // allocate that much memory
  FreeMemory(strString.str_String);
  strString.str_String = (char *) AllocMemory(iLength+1);  // take end-marker in account
  // if the string is not empty
  if (iLength>0) {
    // read string
    strmStream.Read_t( strString.str_String, iLength);  // without end-marker
  }
  // set end-marker
  strString.str_String[iLength] = 0;

  return strmStream;
}


void CTString::ReadFromText_t(CTStream &strmStream,
                              const CTString &strKeyword="") // throw char *
{
  ASSERT(IsValid());

  // keyword must be present
  strmStream.ExpectKeyword_t(strKeyword);

  // read the string from the file
  char str[1024];
  strmStream.GetLine_t(str, sizeof(str));

  // copy it here
  (*this) = str;
}

/*
 * Write to stream.
 */
CTStream &operator<<(CTStream &strmStream, const CTString &strString)
{
  ASSERT(strString.IsValid());

  // calculate size
  INDEX iStringLen = strlen( strString);
  // write size
  strmStream<<iStringLen;
  // if the string is not empty
  if (iStringLen>0) {
    // write string
    strmStream.Write_t(strString.str_String, iStringLen); // without end-marker
  }

  return strmStream;
}


#ifndef NDEBUG
/*
 * Check if string data is valid.
 */
BOOL CTString::IsValid(void) const
{
  ASSERT(this!=NULL && str_String!=NULL);
  return TRUE;
}
#endif // NDEBUG


/* Load an entire text file into a string. */
void CTString::ReadUntilEOF_t(CTStream &strmFile)  // throw char *
{
  // get the file size
  SLONG slFileSize = strmFile.GetStreamSize()-strmFile.GetPos_t();

  // allocate that much memory
  FreeMemory(str_String);
  str_String = (char *) AllocMemory(slFileSize+1);  // take end-marker in account
  // read the entire file there
  if (slFileSize>0) {
    strmFile.Read_t( str_String, slFileSize);
  }
  // add end marker
  str_String[slFileSize] = 0;

  // rewrite entire string
  char *pchRead=str_String;
  char *pchWrite=str_String;
  while(*pchRead!=0) {
    // skip the '\r' characters
    if (*pchRead!='\r') {
      *pchWrite++ = *pchRead++;
    } else {
      pchRead++;
    }
  }
  *pchWrite = 0;
}


void CTString::Load_t(const class CTFileName &fnmFile)  // throw char *
{
  ASSERT(IsValid());

  // open the file for reading
  CTFileStream strmFile;
  strmFile.Open_t(fnmFile);

  // read string until end of file
  ReadUntilEOF_t(strmFile);
}


void CTString::LoadKeepCRLF_t(const class CTFileName &fnmFile)  // throw char *
{
  ASSERT(IsValid());

  // open the file for reading
  CTFileStream strmFile;
  strmFile.Open_t(fnmFile);
  // get the file size
  SLONG slFileSize = strmFile.GetStreamSize();

  // allocate that much memory
  FreeMemory(str_String);
  str_String = (char *) AllocMemory(slFileSize+1);  // take end-marker in account
  // read the entire file there
  if (slFileSize>0) {
    strmFile.Read_t( str_String, slFileSize);
  }
  // add end marker
  str_String[slFileSize] = 0;
}

/* Save an entire string into a text file. */
void CTString::Save_t(const class CTFileName &fnmFile)  // throw char *
{
  // open the file for writing
  CTFileStream strmFile;
  strmFile.Create_t(fnmFile);
  // save the string to the file
  strmFile.PutString_t(*this);
}
void CTString::SaveKeepCRLF_t(const class CTFileName &fnmFile)  // throw char *
{
  // open the file for writing
  CTFileStream strmFile;
  strmFile.Create_t(fnmFile);
  // save the string to the file
  if (strlen(str_String)>0) {
    strmFile.Write_t(str_String, strlen(str_String));
  }
}


// Print formatted to a string
INDEX CTString::PrintF(const char *strFormat, ...)
{
  va_list arg;
  va_start(arg, strFormat);
  return VPrintF(strFormat, arg);
  va_end(arg);
}


INDEX CTString::VPrintF(const char *strFormat, va_list arg)
{

  static INDEX _ctBufferSize = 0;
  static char *_pchBuffer = NULL;

  // if buffer was not allocated yet
  if (_ctBufferSize==0) {
    // allocate it
   
    //_ctBufferSize = 256;
    // 256 very small buffer for /status/ server request - change to 5120 ... and repaet not worked! for this
    _ctBufferSize = 5120;
    _pchBuffer = (char*)AllocMemory(_ctBufferSize);
  }

  // repeat
  INDEX iLen;
  FOREVER {
    // print to the buffer
    iLen = _vsnprintf(_pchBuffer, _ctBufferSize, strFormat, arg);
    // if printed ok
    if (iLen!=-1) {
      // stop
      break;
    }
    // increase the buffer size
    _ctBufferSize += 256;
    GrowMemory((void**)&_pchBuffer, _ctBufferSize);
  }

  (*this) = _pchBuffer;
  return iLen;
}

// !!! FIXME: maybe don't do this, and just use the vsscanf() version.  --ryan.
#if (defined __MSVC_INLINE__) && (defined  PLATFORM_32BIT)
static void *psscanf = &sscanf;
// Scan formatted from a string
__declspec(naked) INDEX CTString::ScanF(const char *strFormat, ...)
{
  __asm {
    push    eax
    mov     eax,dword ptr [esp+8]
    mov     eax,dword ptr [eax]
    mov     dword ptr [esp+8], eax
    pop     eax
    jmp     dword ptr [psscanf]
  }
}

#else

#if defined(_WIN32) && (_MSC_VER <= 1700)
int vsscanf(const char *s, const char *fmt, va_list ap)
{
  void *a[20];
  int i;
  for (i=0; i<sizeof(a)/sizeof(a[0]); i++) a[i] = va_arg(ap, void *);
  return sscanf(s, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]);
}
#endif // MSC_VER <= 1700

INDEX CTString::ScanF(const char *strFormat, ...)
{
    INDEX retval;
    va_list ap;
    va_start(ap, strFormat);
    retval = (INDEX) vsscanf((const char *) (*this), strFormat, ap);
    va_end(ap);
    return(retval);
}

#endif


  // split string in two strings at specified position (char AT splitting position goes to str2)
void CTString::Split( INDEX iPos, CTString &str1, CTString &str2)
{
  str1 = str_String;
  str2 = str_String;
  str1.TrimRight(iPos);
  str2.TrimLeft(strlen(str2)-iPos);
}


// insert one character in string at specified pos
void CTString::InsertChar( INDEX iPos, char cChr)
{
  // clamp position
  INDEX ctChars = strlen(str_String);
  if( iPos>ctChars) iPos=ctChars;
  else if( iPos<0)  iPos=0;
  // grow memory used by string
  GrowMemory( (void**)&str_String, ctChars+2);
  // copy part of string to make room for char to insert
  memmove( &str_String[iPos+1], &str_String[iPos], ctChars+1-iPos);
  str_String[iPos] = cChr;
}


// delete one character from string at specified pos
void CTString::DeleteChar( INDEX iPos)
{
  // clamp position
  INDEX ctChars = strlen(str_String);
  if (ctChars==0) {
    return;
  }
  if( iPos>ctChars) iPos=ctChars - 1;
  else if( iPos<0)  iPos=0;
  // copy part of string
  memmove( &str_String[iPos], &str_String[iPos+1], ctChars-iPos);
  // shrink memory used by string over deleted char
  ShrinkMemory( (void**)&str_String, ctChars);
}



// wild card comparison
BOOL CTString::Matches(const CTString &strOther) const
{
  return Matches(strOther.str_String);
}


BOOL CTString::Matches(const char *strOther) const
{
// pattern matching code from sourceforge.net codesnippet archive
// adjusted a bit to match in/out parameters
#define        MAX_CALLS        200
  int        calls=0, wild=0, q=0;
  const char *mask=strOther, *name=str_String;
  const char *m=mask, *n=name, *ma=mask, *na=name;
  
  for(;;) {
    if (++calls > MAX_CALLS) {
      return FALSE;
    }
    if (*m == '*') {
      while (*m == '*') ++m;
      wild = 1;
      ma = m;
      na = n;
    }
    
    if (!*m) {
      if (!*n) {
        return TRUE;
      }
      
      for (--m; (m > mask) && (*m == '?'); --m) ;
      
      if ((*m == '*') && (m > mask) &&
        (m[-1] != '\\')) {
        return TRUE;
      }
      if (!wild) {
        return FALSE;
      }
      m = ma;
    } else if (!*n) {
      while(*m == '*') ++m;
      if (*m != 0) {
        return FALSE;
      } else {
        return TRUE;
      }
    }
    if ((*m == '\\') && ((m[1] == '*') || (m[1] == '?'))) {
      ++m;
      q = 1;
    } else {
      q = 0;
    }

    // also, '\\' in mask should match '/' in name, for unix compatibility
    if (((tolower(*m) != tolower(*n)) && ((*m!='\\') && (*n!='/')))
        && ((*m != '?') || q)) {
      if (!wild) {
        return FALSE;
      }
      m = ma;
      n = ++na;
    } else {
      if (*m) ++m;
        if (*n) ++n;
    }
  }
}


// variable management functions
void CTString::LoadVar(const class CTFileName &fnmFile)
{
  try {
    CTString str;
    str.Load_t(fnmFile);
    *this = str;
  } catch (const char *strError) {
    CPrintF(TRANSV("Cannot load variable from '%s':\n%s\n"), (const char *) (CTString&)fnmFile, strError);
  }
}

void CTString::SaveVar(const class CTFileName &fnmFile)
{
  try {
    Save_t(fnmFile);
  } catch (const char *strError) {
    CPrintF(TRANSV("Cannot save variable to '%s':\n%s\n"), (const char *) (CTString&)fnmFile, strError);
  }
}


// general variable functions
void LoadStringVar(const CTFileName &fnmVar, CTString &strVar)
{
  strVar.LoadVar(fnmVar);
}

void SaveStringVar(const CTFileName &fnmVar, CTString &strVar)
{
  strVar.SaveVar(fnmVar);
}


void LoadIntVar(const CTFileName &fnmVar, INDEX &iVar)
{
  CTString strVar;
  strVar.LoadVar(fnmVar);
  if (strVar!="") {
    CTString strHex = strVar;
    if (strHex.RemovePrefix("0x")) {
      strHex.ScanF("%x", &iVar);
    } else {
      strVar.ScanF("%d", &iVar);
    }
  }
}

void SaveIntVar(const CTFileName &fnmVar, INDEX &iVar)
{
  CTString strVar;
  strVar.PrintF("%d", iVar);
  strVar.SaveVar(fnmVar);
}

// used to convert string from ansi format into utf-8 (
CTString strWindow1251ToUtf8(CTString from)
{
  static char outBuffer[10000];
  cp2utf1(outBuffer, from.str_String, sizeof(outBuffer) - 10);
  return (CTString) outBuffer;
}

// remove special codes from string
CTString RemoveSpecialCodes( const CTString &str)
{
  CTString strRet=str;
  char *pcSrc = (char*)(const char*)strRet;
  char *pcDst = (char*)(const char*)strRet;
  // copy char inside string skipping special codes
  while( *pcSrc != 0)
  {
    if( *pcSrc != '^')
    { // advance to next char
      *pcDst = *pcSrc;
      pcSrc++;
      pcDst++;
    }
    else
    { // skip some characters 
      pcSrc++;
      switch( *pcSrc) {
      case 'c':  pcSrc+=FindZero((UBYTE*)pcSrc,7);  continue;
      case 'a':  pcSrc+=FindZero((UBYTE*)pcSrc,3);  continue;
      case 'f':  pcSrc+=FindZero((UBYTE*)pcSrc,2);  continue;
      case 'b':  case 'i':  case 'r':  case 'o':
      case 'C':  case 'A':  case 'F':  case 'B':  case 'I':  pcSrc+=1;  continue;
      // if we get here this means that ^ or an unrecognized special code was specified
      default: continue;
      }
    }
  }
  // terminate string
  *pcDst = 0;
  return strRet;
}
