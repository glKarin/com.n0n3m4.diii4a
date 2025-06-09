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
#include <Engine/Base/Translation.h>
#include <Engine/Base/TranslationPair.h>
#include <Engine/Base/Input.h>
#include <Engine/Templates/NameTable_CTranslationPair.h>

#include <Engine/Base/Memory.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>

#include <Engine/Templates/DynamicArray.cpp>

// table of translations
static CNameTable_CTranslationPair _nttpPairs;
static CDynamicArray<CTranslationPair> _atpPairs;
static int _iLine;

#define CHAR_EOF -1
#define CHAR_SRC -2
#define CHAR_DST -3

static int ReadOneChar_t(CTStream &strm)
{
  // skip line breaks and count them
  unsigned char c;
  do {
    strm>>c;
    if (c=='\n') {
      _iLine++;
    }
  } while (c=='\n' || c=='\r');

  // if start of control token
  if (c=='\\') {
    // read next char
    strm>>c;
    // decode token
    switch (c) {
    case '\\': return '\\';
    // eol
    case 'n': return '\n';
    // string end
    case '0': return 0;
    // source string
    case '<': return CHAR_SRC;
    // destination string
    case '>': return CHAR_DST;
    // file end
    case '$': return CHAR_EOF;
    // other
    default:
      if (isprint(c)) {
        ThrowF_t(TRANS("%d: unknown token '%c'"), _iLine, c);
      } else {
        ThrowF_t(TRANS("%d: unknown token ascii: 0x%02x"), _iLine, c);
      }
    }
  }

  return c;
}
// read one translation string from file
static CTString ReadOneString_t(CTStream &strm)
{
  // start with empty string
  CTString str;

  // read characters
  FOREVER{
    int i = ReadOneChar_t(strm);
    if (i==0) {
      return str;
    } else {
      char c[2];
      c[1] = 0;
      c[0] = i;
      str+=c;
    }
  }
}

// init translation routines with given translation table
ENGINE_API void InitTranslation(void)
{
  // init tables
  _atpPairs.Clear();
  _nttpPairs.Clear();
  _nttpPairs.SetAllocationParameters(100, 5, 5);
}

ENGINE_API void ReadTranslationTable_t(
  CDynamicArray<CTranslationPair> &atpPairs, const CTFileName &fnmTable) // throw char *
{
  _iLine = 0;

  CTFileStream strm;
  strm.Open_t(fnmTable);

  // read number of pairs
  INDEX ctPairs;
  CTString strPairs = ReadOneString_t(strm);
  strPairs.ScanF("%d", &ctPairs);

  // instance that much
  CTranslationPair *atp = atpPairs.New(ctPairs);

  // for each pair
  for(INDEX iPair=0; iPair<ctPairs; iPair++) {
    CTranslationPair &tp = atp[iPair];
    // read one token
    int iToken = ReadOneChar_t(strm);
    // otherwise it must be source
    if (iToken!=CHAR_SRC) {
      if (iToken==CHAR_EOF) {
        ThrowF_t(TRANS("error in file <%s>, premature EOF in line #%d!"),
          (const char *)fnmTable, _iLine);
      } else {
        ThrowF_t(TRANS("error in file <%s>, line #%d (pair #%d): expected '<' but found '%c'"),
          (const char *)fnmTable, _iLine, iPair, iToken);
      }
    }
    // read source
    tp.tp_strSrc = ReadOneString_t(strm);
    // next token must be destination
    if (ReadOneChar_t(strm)!=CHAR_DST) {
      if (iToken==CHAR_EOF) {
        ThrowF_t(TRANS("error in file <%s>, premature EOF in line #%d!"),
          (const char *)fnmTable, _iLine);
      } else {
        ThrowF_t(TRANS("error in file <%s>, line #%d (pair #%d): expected '>' but found '%c'"),
          (const char *)fnmTable, _iLine, iPair, iToken);
      }
    }
    // read destination
    tp.tp_strDst = ReadOneString_t(strm);
  };
  // last token must be eof
  if (ReadOneChar_t(strm)!=CHAR_EOF) {
    ThrowF_t(TRANS("error in file <%s>: end of file marker not found in line #%d!"), (const char *)fnmTable, _iLine);
  }
}

// finish translation table building
ENGINE_API void FinishTranslationTable(void)
{
  INDEX ctPairs = _atpPairs.Count();
  // for each pair
  _atpPairs.Lock();
  for(INDEX iPair=0; iPair<ctPairs; iPair++) {
    // add pair to name table
    _nttpPairs.Add(&_atpPairs[iPair]);
  };
  _atpPairs.Unlock();

  _pInput->SetKeyNames();
}

// add given translation table
ENGINE_API void AddTranslationTable_t(const CTFileName &fnmTable) // throw char *
{
  // just read it to global array
  ReadTranslationTable_t(_atpPairs, fnmTable);
}

// add several tables from a directory using wildcards
ENGINE_API void AddTranslationTablesDir_t(const CTFileName &fnmDir, const CTFileName &fnmPattern) // throw char *
{
  // list the translation tables matching given pattern
  CDynamicStackArray<CTFileName> afnmTables;
  MakeDirList(afnmTables, fnmDir, fnmPattern, 0);
  for(INDEX i=0; i<afnmTables.Count(); i++) {
    ReadTranslationTable_t(_atpPairs, afnmTables[i]);
  }
}

// !!! FIXME: clean these out.
// translate a string
ENGINE_API char *Translate(const char *str, INDEX iOffset)
{
  return (char*)TranslateConst(str, iOffset);
}

ENGINE_API const char *TranslateConst(const char *str, INDEX iOffset)
{
  // skip first bytes
  if (static_cast<INDEX>(strlen(str))>=iOffset) {
    str+=iOffset;
  } else {
    ASSERT(FALSE);
  }
  // find translation pair
  CTranslationPair *ptp = NULL;
  if (_atpPairs.Count()>0) {
    ptp = _nttpPairs.Find(str);
  }
  // if not found
  if (ptp==NULL) {
    // return original string
    return str;
  // if found
  } else {
    // return translation
    return ptp->tp_strDst;
  }
}
