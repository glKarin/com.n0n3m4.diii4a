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

#include <Engine/Base/Shell.h>
#include <Engine/Base/Shell_internal.h>
#include "ParsingSymbols.h"

#include <Engine/Templates/DynamicStackArray.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/Stream.h>

#include <Engine/Templates/AllocationArray.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/DynamicStackArray.cpp>

template class CDynamicArray<CShellSymbol>;

// shell type used for undeclared symbols
__extern INDEX _shell_istUndeclared = -1;

// pointer to global shell object
CShell *_pShell = NULL;
void *_pvNextToDeclare=NULL; // != NULL if declaring external symbol defined in exe code

// define console variable for number of last console lines
__extern INDEX con_iLastLines    = 5;

extern void yy_switch_to_buffer(YY_BUFFER_STATE);

// declarations for recursive shell script parsing
struct BufferStackEntry {
  YY_BUFFER_STATE bse_bs;
  const char *bse_strName;
  const char *bse_strContents;
  int bse_iLineCt;
  BOOL bse_bParserEnd;
};

static BufferStackEntry _abseBufferStack[SHELL_MAX_INCLUDE_LEVEL];
static int _ibsBufferStackTop = -1;

BOOL _bExecNextBlock = 1;

void ShellPushBuffer(const char *strName, const char *strBuffer, BOOL bParserEnd)
{
  _ibsBufferStackTop++;

  _abseBufferStack[_ibsBufferStackTop].bse_strContents = strdup(strBuffer);
  _abseBufferStack[_ibsBufferStackTop].bse_strName = strdup(strName);
  _abseBufferStack[_ibsBufferStackTop].bse_iLineCt = 1;
  _abseBufferStack[_ibsBufferStackTop].bse_bParserEnd = bParserEnd;

  _abseBufferStack[_ibsBufferStackTop].bse_bs = yy_scan_string((char*)(const char*)strBuffer);

  yy_switch_to_buffer(_abseBufferStack[_ibsBufferStackTop].bse_bs);
}
BOOL ShellPopBuffer(void)
{
  yy_delete_buffer( _abseBufferStack[_ibsBufferStackTop].bse_bs);
  free((void*)_abseBufferStack[_ibsBufferStackTop].bse_strName);
  free((void*)_abseBufferStack[_ibsBufferStackTop].bse_strContents);
  BOOL bParserEnd = _abseBufferStack[_ibsBufferStackTop].bse_bParserEnd;

  _ibsBufferStackTop--;

  if (_ibsBufferStackTop>=0) {
    yy_switch_to_buffer(_abseBufferStack[_ibsBufferStackTop].bse_bs);
  }
  return bParserEnd;
}
const char *ShellGetBufferName(void)
{
  return _abseBufferStack[_ibsBufferStackTop].bse_strName;
}
int ShellGetBufferLineNumber(void)
{
  return _abseBufferStack[_ibsBufferStackTop].bse_iLineCt;
}
int ShellGetBufferStackDepth(void)
{
  return _ibsBufferStackTop;
}
const char *ShellGetBufferContents(void)
{
  return _abseBufferStack[_ibsBufferStackTop].bse_strContents;
}
void ShellCountOneLine(void)
{
  _abseBufferStack[_ibsBufferStackTop].bse_iLineCt++;
}


// temporary values for parsing
CDynamicStackArray<CTString> _shell_astrTempStrings;
// values for extern declarations
CDynamicStackArray<CTString> _shell_astrExtStrings;
CDynamicStackArray<FLOAT> _shell_afExtFloats;

//static const char *strCommandLine = "";
#ifdef PLATFORM_WIN32
ENGINE_API extern FLOAT tmp_af[10] = { 0 };
ENGINE_API extern INDEX tmp_ai[10] = { 0 };
ENGINE_API extern INDEX tmp_fAdd   = 0.0f;
ENGINE_API extern INDEX tmp_i      = 0;
#else
FLOAT tmp_af[10] = { 0 };
INDEX tmp_ai[10] = { 0 };
INDEX tmp_fAdd   = 0;
INDEX tmp_i      = 0;
#endif

void CShellSymbol::Clear(void)
{
  ss_istType = -1;
  ss_strName.Clear();
  ss_ulFlags = 0;
};
BOOL CShellSymbol::IsDeclared(void)
{
  return ss_istType>=0 && ss_istType!=_shell_istUndeclared;
}

CTString CShellSymbol::GetCompletionString(void) const
{
  // get its type
  ShellType &st = _shell_ast[ss_istType];

  // get its name
  if (st.st_sttType==STT_FUNCTION) {
    return ss_strName + "()";
  } else if (st.st_sttType==STT_ARRAY) {
    return ss_strName + "[]";
  } else {
    return ss_strName;
  }
}

// Constructor.
CShell::CShell(void)
{
  // allocate undefined symbol
  _shell_istUndeclared = _shell_ast.Allocate();
  pwoCurrentWorld = NULL;
};
CShell::~CShell(void)
{
  _shell_astrExtStrings.Clear();
  _shell_afExtFloats.Clear();
};

static const INDEX _bTRUE  = TRUE;
static const INDEX _bFALSE = FALSE;

CTString ScriptEsc(const CTString &str)
{
  CTString strResult = "";

  const char *pchSrc = (const char *)str;
  char buf[2];
  buf[1] = 0;

  while (*pchSrc!=0) {
    switch(*pchSrc) {
    case  10: strResult+="\\n"; break;
    case  13: strResult+="\\r"; break;
    case '\\': strResult+="\\\\"; break;
    case '"': strResult+="\\\""; break;
    default: buf[0] = *pchSrc; strResult+=buf; break;
    }
    pchSrc++;
  }
  return strResult;
}

#pragma inline_depth(0)
void MakeAccessViolation(void* pArgs)
{
  INDEX bDont = NEXTARGUMENT(INDEX);
  if( bDont) return;
  char *p=NULL;
  *p=1;
}

__extern int _a=123;
void MakeStackOverflow(void* pArgs)
{
  INDEX bDont = NEXTARGUMENT(INDEX);
  if( bDont) return;
  int a[1000];
  a[999] = _a;
  MakeStackOverflow(0);
  _a=a[999];
}

void MakeFatalError(void* pArgs)
{
  INDEX bDont = NEXTARGUMENT(INDEX);
  if( bDont) return;
  FatalError( "MakeFatalError()");
}


extern void ReportGlobalMemoryStatus(void)
{
#ifdef PLATFORM_WIN32
   CPrintF(TRANSV("Global memory status...\n"));

   MEMORYSTATUS ms;
   GlobalMemoryStatus(&ms);

#define MB (1024*1024)
   CPrintF(TRANSV("  Physical memory used: %4d/%4dMB\n"), (ms.dwTotalPhys    -ms.dwAvailPhys    )/MB, ms.dwTotalPhys    /MB);
   CPrintF(TRANSV("  Page file used:       %4d/%4dMB\n"), (ms.dwTotalPageFile-ms.dwAvailPageFile)/MB, ms.dwTotalPageFile/MB);
   CPrintF(TRANSV("  Virtual memory used:  %4d/%4dMB\n"), (ms.dwTotalVirtual -ms.dwAvailVirtual )/MB, ms.dwTotalVirtual /MB);
   CPrintF(TRANSV("  Memory load: %3d%%\n"), ms.dwMemoryLoad);

#if (defined _MSC_VER) && (defined  PLATFORM_64BIT)
   SIZE_T dwMin;
   SIZE_T dwMax;
#else
   DWORD dwMin;
   DWORD dwMax;
#endif  
   GetProcessWorkingSetSize(GetCurrentProcess(), &dwMin, &dwMax);
   CPrintF(TRANSV("  Process working set: %dMB-%dMB\n\n"), dwMin/(1024*1024), dwMax/(1024*1024));
#endif
}

static void MemoryInfo(void)
{
  ReportGlobalMemoryStatus();

#ifdef PLATFORM_WIN32
   _HEAPINFO hinfo;
   int heapstatus;
   hinfo._pentry = NULL;
   SLONG slTotalUsed = 0;
   SLONG slTotalFree = 0;
   INDEX ctUsed = 0;
   INDEX ctFree = 0;

   CPrintF( "Walking heap...\n");
   while( ( heapstatus = _heapwalk( &hinfo ) ) == _HEAPOK )
   {
     if (hinfo._useflag == _USEDENTRY ) {
       slTotalUsed+=hinfo._size;
       ctUsed++;
     } else {
       slTotalFree+=hinfo._size;
       ctFree++;
     }
   }
   switch( heapstatus )   {
     case _HEAPEMPTY:     CPrintF( "Heap empty?!?\n" );                break;
     case _HEAPEND:       CPrintF( "Heap ok.\n" );                     break;
     case _HEAPBADPTR:    CPrintF( "ERROR - bad pointer to heap\n" );  break;
     case _HEAPBADBEGIN:  CPrintF( "ERROR - bad start of heap\n" );    break;
     case _HEAPBADNODE:   CPrintF( "ERROR - bad node in heap\n" );     break;
   }
   CPrintF( "Total used: %d bytes (%.2f MB) in %d blocks\n", slTotalUsed, slTotalUsed/1024.0f/1024.0f, ctUsed);
   CPrintF( "Total free: %d bytes (%.2f MB) in %d blocks\n", slTotalFree, slTotalFree/1024.0f/1024.0f, ctFree);
#endif
}

// get help for a shell symbol
extern CTString GetShellSymbolHelp_t(const CTString &strSymbol)
{
  CTString strPattern = strSymbol+"*";
  // open the symbol help file
  CTFileStream strm;
  strm.Open_t(CTString("Help\\ShellSymbols.txt"));

  // while not at the end of file
  while (!strm.AtEOF()) {
    // read the symbol name and its help
    CTString strSymbolInFile;
    strm.GetLine_t(strSymbolInFile, ':');
    strSymbolInFile.TrimSpacesLeft();
    strSymbolInFile.TrimSpacesRight();
    CTString strHelpInFile;
    strm.GetLine_t(strHelpInFile, '$');
    strHelpInFile.TrimSpacesLeft();
    strHelpInFile.TrimSpacesRight();
    // if that is the one
    if( strSymbolInFile.Matches(strPattern)) {
      // print the help
      return strHelpInFile;
    }
  }
  return "";
}

// check if there is help for a shell symbol
extern BOOL CheckShellSymbolHelp(const CTString &strSymbol)
{
  try {
    return GetShellSymbolHelp_t(strSymbol)!="";
  } catch (const char *strError) {
    (void)strError;
    return FALSE;
  }
}

// print help for a shell symbol
extern void PrintShellSymbolHelp(const CTString &strSymbol)
{
  // try to
  try {
    CTString strHelp = GetShellSymbolHelp_t(strSymbol);
    if (strHelp!="") {
      CPrintF("%s\n", (const char *) strHelp);
    } else {
      CPrintF( TRANS("No help found for '%s'.\n"), (const char *) strSymbol);
    }
  // if failed
  } catch (const char *strError) {
    // just print the error
    CPrintF( TRANS("Cannot print help for '%s': %s\n"), (const char *) strSymbol, strError);
  }
}

extern void ListSymbolsByPattern(CTString strPattern)
{
  // synchronize access to global shell
  CTSingleLock slShell(&_pShell->sh_csShell, TRUE);

  // for each of symbols in the shell
  FOREACHINDYNAMICARRAY(_pShell->sh_assSymbols, CShellSymbol, itss) {
    CShellSymbol &ss = *itss;

    // if it is not visible to user, or not matching
    if (!(ss.ss_ulFlags&SSF_USER) || !ss.ss_strName.Matches(strPattern)) {
      // skip it
      continue;
    }

    // get its type
    ShellType &st = _shell_ast[ss.ss_istType];

    if (ss.ss_ulFlags & SSF_CONSTANT) {
      CPrintF("const ");
    }
    if (ss.ss_ulFlags & SSF_PERSISTENT) {
      CPrintF("persistent ");
    }

    // print its declaration to the console
    if (st.st_sttType == STT_FUNCTION) {
      CPrintF("void %s(void)", (const char *) ss.ss_strName);

    } else if (st.st_sttType == STT_STRING) {
      CPrintF("CTString %s = \"%s\"", (const char *) ss.ss_strName, (const char *) (*(CTString*)ss.ss_pvValue));
    } else if (st.st_sttType == STT_FLOAT) {
      CPrintF("FLOAT %s = %g", (const char *) ss.ss_strName, *(FLOAT*)ss.ss_pvValue);
    } else if (st.st_sttType == STT_INDEX) {
      CPrintF("INDEX %s = %d (0x%08x)", (const char *) ss.ss_strName, *(INDEX*)ss.ss_pvValue, *(INDEX*)ss.ss_pvValue);
    } else if (st.st_sttType == STT_ARRAY) {
      // get base type
      ShellType &stBase = _shell_ast[st.st_istBaseType];
      if (stBase.st_sttType == STT_FLOAT) {
        CPrintF("FLOAT %s[%d]", (const char *) ss.ss_strName, st.st_ctArraySize);
      } else if (stBase.st_sttType == STT_INDEX) {
        CPrintF("INDEX %s[%d]", (const char *) ss.ss_strName, st.st_ctArraySize);
      } else if (stBase.st_sttType == STT_STRING) {
        CPrintF("CTString %s[%d]", (const char *) ss.ss_strName, st.st_ctArraySize);
      } else {
        ASSERT(FALSE);
      }
    } else {
      ASSERT(FALSE);
    }

    if (!CheckShellSymbolHelp(ss.ss_strName)) {
      CPrintF( TRANS(" help N/A"));
    }
    CPrintF("\n");
  }
}

// Print a list of all symbols in global shell to console.
static void ListSymbols(void)
{
  // print header
  CPrintF( TRANS("Useful symbols:\n"));

  // list all symbols
  ListSymbolsByPattern("*");
}


// output any string to console
void Echo(void* pArgs)
{
  CTString str = *NEXTARGUMENT(CTString*);
  CPrintF("%s", (const char *) str);
}



CTString UndecorateString(void* pArgs)
{
  CTString strString = *NEXTARGUMENT(CTString*);
  return strString.Undecorated();
}
BOOL MatchStrings(void* pArgs)
{
  CTString strString = *NEXTARGUMENT(CTString*);
  CTString strPattern = *NEXTARGUMENT(CTString*);
  return strString.Matches(strPattern);
}
CTString MyLoadString(void* pArgs)
{
  CTString strFileName = *NEXTARGUMENT(CTString*);
  try {
    CTString strString;
    strString.Load_t(strFileName);
    return strString;
  } catch (const char *strError) {
    (void)strError;
    return "";
  }
}
void MySaveString(void* pArgs)
{
  CTString strFileName = *NEXTARGUMENT(CTString*);
  CTString strString = *NEXTARGUMENT(CTString*);
  try {
    strString.Save_t(strFileName);
  } catch (const char *strError) {
    (void)strError;
  }
}

// load command batch files
void LoadCommands(void)
{
  // list all command files
  CDynamicStackArray<CTFileName> afnmCmds;
  MakeDirList( afnmCmds, CTString("Scripts\\Commands\\"), CTString("*.ini"), DLI_RECURSIVE);
  // for each file
  for(INDEX i=0; i<afnmCmds.Count(); i++) {
    CTFileName &fnm = afnmCmds[i];
    // load the file
    CTString strCmd;
    try {
      strCmd.Load_t(fnm);
    } catch (const char *strError) {
      CPrintF("%s\n", strError);
      continue;
    }
    CTString strName = fnm.FileName();
    // declare it
    extern void Declaration(
      ULONG ulQualifiers, INDEX istType, CShellSymbol &ssNew,
      INDEX (*pPreFunc)(INDEX), void (*pPostFunc)(INDEX));

    INDEX iType = ShellTypeNewString();
    CShellSymbol &ssNew = *_pShell->GetSymbol(strName, FALSE);
    Declaration(SSF_EXTERNAL|SSF_USER, iType, ssNew, NULL, NULL);
    ShellTypeDelete(iType);

    // get symbol type
    ShellTypeType stt = _shell_ast[ssNew.ss_istType].st_sttType;

    // if the symbol is ok
    if (stt == STT_STRING && !(ssNew.ss_ulFlags&SSF_CONSTANT)) {
      // assign value
      *(CTString*)ssNew.ss_pvValue = "!command "+strCmd;
    } else {
      _pShell->ErrorF("Symbol '%s' is not suitable to be a command", (const char *) ssNew.ss_strName);
    }
  }
}

CTString ToUpper(const CTString &strResult)
{
  char *pch = (char*)(const char *)strResult;
  for(INDEX i=0; i<static_cast<INDEX>(strlen(pch)); i++) {
    pch[i]=toupper(pch[i]);
  }
  return strResult;
}
CTString ToUpperCfunc(void* pArgs)
{
  CTString strResult = *NEXTARGUMENT(CTString*);
  return ToUpper(strResult);
}
CTString ToLower(const CTString &strResult)
{
  char *pch = (char*)(const char *)strResult;
  for(INDEX i=0; i<static_cast<INDEX>(strlen(pch)); i++) {
    pch[i]=tolower(pch[i]);
  }
  return strResult;
}
CTString ToLowerCfunc(void* pArgs)
{
  CTString strResult = *NEXTARGUMENT(CTString*);
  return ToLower(strResult);
}

CTString RemoveSubstring(const CTString &strFull, const CTString &strSub)
{
  CTString strFullL = ToLower(strFull);
  CTString strSubL = ToLower(strSub);

  const char *pchFullL = strFullL;
  const char *pchSubL = strSubL;
  const char *pchFound = strstr(pchFullL, pchSubL);
  if (pchFound==NULL || strlen(strSub)==0) {
    return strFull;
  }
  INDEX iOffset = pchFound-pchFullL;
  INDEX iLenFull = strlen(strFull);
  INDEX iLenSub = strlen(strSub);

  CTString strLeft = strFull;
  strLeft.TrimRight(iOffset);
  CTString strRight = strFull;
  strRight.TrimLeft(iLenFull-iOffset-iLenSub);
  return strLeft+strRight;
}
CTString RemoveSubstringCfunc(void* pArgs)
{
  CTString strFull = *NEXTARGUMENT(CTString*);
  CTString strSub = *NEXTARGUMENT(CTString*);
  return RemoveSubstring(strFull, strSub);
}

// Initialize the shell.
void CShell::Initialize(void)
{
  sh_csShell.cs_iIndex = -1;

  // synchronize access to shell
  CTSingleLock slShell(&sh_csShell, TRUE);
  // add built in commands and constants
  DeclareSymbol("const INDEX TRUE;",  (void*)&_bTRUE);
  DeclareSymbol("const INDEX FALSE;", (void*)&_bFALSE);
  DeclareSymbol("const INDEX ON;",    (void*)&_bTRUE);
  DeclareSymbol("const INDEX OFF;",   (void*)&_bFALSE);
  DeclareSymbol("const INDEX YES;",   (void*)&_bTRUE);
  DeclareSymbol("const INDEX NO;",    (void*)&_bFALSE);

  DeclareSymbol("user void LoadCommands(void);", (void *)&LoadCommands);
  DeclareSymbol("user void ListSymbols(void);", (void *)&ListSymbols);
  DeclareSymbol("user void MemoryInfo(void);",  (void *)&MemoryInfo);
  DeclareSymbol("user void MakeAccessViolation(INDEX);", (void *)&MakeAccessViolation);
  DeclareSymbol("user void MakeStackOverflow(INDEX);",   (void *)&MakeStackOverflow);
  DeclareSymbol("user void MakeFatalError(INDEX);",      (void *)&MakeFatalError);
  DeclareSymbol("persistent user INDEX con_iLastLines;", (void *)&con_iLastLines);
  DeclareSymbol("persistent user FLOAT tmp_af[10];", (void *)&tmp_af);
  DeclareSymbol("persistent user INDEX tmp_ai[10];", (void *)&tmp_ai);
  DeclareSymbol("persistent user INDEX tmp_i;", (void *)&tmp_i);
  DeclareSymbol("persistent user FLOAT tmp_fAdd;", (void *)&tmp_fAdd);

  DeclareSymbol("user void Echo(CTString);", (void *)&Echo);
  DeclareSymbol("user CTString UndecorateString(CTString);", (void *)&UndecorateString);
  DeclareSymbol("user INDEX Matches(CTString, CTString);", (void *)&MatchStrings);
  DeclareSymbol("user CTString LoadString(CTString);", (void *)&MyLoadString);
  DeclareSymbol("user void SaveString(CTString, CTString);", (void *)&MySaveString);
  DeclareSymbol("user CTString RemoveSubstring(CTString, CTString);", (void *)&RemoveSubstringCfunc);
  DeclareSymbol("user CTString ToUpper(CTString);", (void *)&ToUpperCfunc);
  DeclareSymbol("user CTString ToLower(CTString);", (void *)&ToLowerCfunc);
}

static BOOL _iParsing = 0;

// Declare a symbol in the shell.
/* rcg10072001 Added second version of DeclareSymbol()... */
void CShell::DeclareSymbol(const CTString &strDeclaration, void *pvValue)
{
    DeclareSymbol((const char *) strDeclaration, pvValue);
}

void CShell::DeclareSymbol(const char *strDeclaration, void *pvValue)
{
  // synchronize access to shell
  CTSingleLock slShell(&sh_csShell, TRUE);

  _pvNextToDeclare = pvValue;

  _iParsing++;

  // parse the string
  const BOOL old_bExecNextBlock = _bExecNextBlock;
  _bExecNextBlock = 1;

  ShellPushBuffer("<declaration>", strDeclaration, TRUE);
  yyparse();
//  ShellPopBuffer();

  _bExecNextBlock = old_bExecNextBlock;

  _iParsing--;
  if (_iParsing<=0) {
    _shell_astrTempStrings.PopAll();
  }

  // don't use that value for parsing any more
  _pvNextToDeclare = NULL;
}

// Execute command(s).
void CShell::Execute(const CTString &strCommands)
{
  // synchronize access to shell
  CTSingleLock slShell(&sh_csShell, TRUE);

//  ASSERT(_iParsing==0);
  _iParsing++;

  // parse the string
  const BOOL old_bExecNextBlock = _bExecNextBlock;
  _bExecNextBlock = 1;

  ShellPushBuffer("<command>", strCommands, TRUE);
  yyparse();
  //ShellPopBuffer();

  _bExecNextBlock = old_bExecNextBlock;

  _iParsing--;
  if (_iParsing<=0) {
    _shell_astrTempStrings.PopAll();
  }
};

// Get a shell symbol by its name.
CShellSymbol *CShell::GetSymbol(const CTString &strName, BOOL bDeclaredOnly)
{
  // synchronize access to shell
  CTSingleLock slShell(&sh_csShell, TRUE);

  // for each of symbols in the shell
  FOREACHINDYNAMICARRAY(sh_assSymbols, CShellSymbol, itss) {
    // if it is the right one
    if (itss->ss_strName==strName) {
      // return it
      return itss;
    }
  }
  // if none is found...

  // if only declared symbols are allowed
  if (bDeclaredOnly) {
    // return nothing
    return NULL;

  // if undeclared symbols are allowed
  } else {
    // create a new one with that name and undefined type
    CShellSymbol &ssNew = *sh_assSymbols.New(1);
    ssNew.ss_strName = strName;
    ssNew.ss_istType = _shell_istUndeclared;
    ssNew.ss_pvValue = NULL;
    ssNew.ss_ulFlags = 0;
    ssNew.ss_pPreFunc = NULL;
    ssNew.ss_pPostFunc = NULL;
    return &ssNew;
  }
};

FLOAT CShell::GetFLOAT(const CTString &strName)
{
  // get the symbol
  CShellSymbol *pss = GetSymbol(strName, TRUE);

  // if it doesn't exist or is not of given type
  if (pss==NULL || _shell_ast[pss->ss_istType].st_sttType!=STT_FLOAT) {
    // error
    ASSERT(FALSE);
    return -666.0f;
  } 
  // get it
  return *(FLOAT*)pss->ss_pvValue;
}

void CShell::SetFLOAT(const CTString &strName, FLOAT fValue)
{
  CShellSymbol *pss = GetSymbol(strName, TRUE);

  // if it doesn't exist or is not of given type
  if (pss==NULL || _shell_ast[pss->ss_istType].st_sttType!=STT_FLOAT) {
    // error
    ASSERT(FALSE);
    return;
  } 
  // set it
  *(FLOAT*)pss->ss_pvValue = fValue;
}

INDEX CShell::GetINDEX(const CTString &strName)
{
  // get the symbol
  CShellSymbol *pss = GetSymbol(strName, TRUE);

  // if it doesn't exist or is not of given type
  if (pss==NULL || _shell_ast[pss->ss_istType].st_sttType!=STT_INDEX) {
    // error
    ASSERT(FALSE);
    return -666;
  } 
  // get it
  return *(INDEX*)pss->ss_pvValue;
}

void CShell::SetINDEX(const CTString &strName, INDEX iValue)
{
  CShellSymbol *pss = GetSymbol(strName, TRUE);

  // if it doesn't exist or is not of given type
  if (pss==NULL || _shell_ast[pss->ss_istType].st_sttType!=STT_INDEX) {
    // error
    ASSERT(FALSE);
    return;
  } 
  // set it
  *(INDEX*)pss->ss_pvValue = iValue;
}

CTString CShell::GetString(const CTString &strName)
{
  // get the symbol
  CShellSymbol *pss = GetSymbol(strName, TRUE);

  // if it doesn't exist or is not of given type
  if (pss==NULL || _shell_ast[pss->ss_istType].st_sttType!=STT_STRING) {
    // error
    ASSERT(FALSE);
    return "<invalid>";
  } 
  // get it
  return *(CTString*)pss->ss_pvValue;
}

void CShell::SetString(const CTString &strName, const CTString &strValue)
{
  CShellSymbol *pss = GetSymbol(strName, TRUE);

  // if it doesn't exist or is not of given type
  if (pss==NULL || _shell_ast[pss->ss_istType].st_sttType!=STT_STRING) {
    // error
    ASSERT(FALSE);
    return;
  } 
  // set it
  *(CTString*)pss->ss_pvValue = strValue;
}


CTString CShell::GetValue(const CTString &strName)
{
  // get the symbol
  CShellSymbol *pss = GetSymbol(strName, TRUE);

  // if it doesn't exist
  if (pss==NULL) {
    // error
    ASSERT(FALSE);
    return "<invalid>";
  } 

  // get it
  ShellTypeType stt = _shell_ast[pss->ss_istType].st_sttType;
  CTString strValue;
  switch(stt) {
  case STT_STRING:
    strValue = *(CTString*)pss->ss_pvValue;
    break;
  case STT_INDEX:
    strValue.PrintF("%d", *(INDEX*)pss->ss_pvValue);
    break;
  case STT_FLOAT:
    strValue.PrintF("%g", *(FLOAT*)pss->ss_pvValue);
    break;
  default:
    ASSERT(FALSE);
    return "";
  }

  return strValue;
}

void CShell::SetValue(const CTString &strName, const CTString &strValue)
{
  // get the symbol
  CShellSymbol *pss = GetSymbol(strName, TRUE);

  // if it doesn't exist
  if (pss==NULL) {
    // error
    ASSERT(FALSE);
    return;
  } 
  // get it
  ShellTypeType stt = _shell_ast[pss->ss_istType].st_sttType;
  switch(stt) {
  case STT_STRING:
    *(CTString*)pss->ss_pvValue = strValue;
    break;
  case STT_INDEX:
    ((CTString&)strValue).ScanF("%d", (INDEX*)pss->ss_pvValue);
    break;
  case STT_FLOAT:
    ((CTString&)strValue).ScanF("%g", (FLOAT*)pss->ss_pvValue);
    break;
  default:
    ASSERT(FALSE);
  }

  return;
}

/*
 * Report error in shell script processing.
 */
void CShell::ErrorF(const char *strFormat, ...)
{
  // synchronize access to shell
  CTSingleLock slShell(&sh_csShell, TRUE);

  // print the error file and line
  const char *strName = ShellGetBufferName();
  int iLine = ShellGetBufferLineNumber();
  if (strName[0] == '<') {
    CPrintF("%s\n%s(%d): ", ShellGetBufferContents(), strName, iLine);
  } else {
    CPrintF("%s(%d): ", strName, iLine);
  }

  // format the message in buffer
  va_list arg;
  va_start(arg, strFormat);
  CTString strBuffer;
  strBuffer.VPrintF(strFormat, arg);
  va_end(arg);

  // print it to the main console
  CPrintF(strBuffer);
  // go to new line
  CPrintF("\n");
}

// Save shell commands to restore persistent symbols to a script file
void CShell::StorePersistentSymbols(const CTFileName &fnScript)
{
  // synchronize access to global shell
  CTSingleLock slShell(&sh_csShell, TRUE);

  try {
    // open the file
    CTFileStream fScript;
    fScript.Create_t(fnScript);

    // print header
    fScript.FPrintF_t("// automatically saved persistent symbols:\n");
    // for each of symbols in the shell
    FOREACHINDYNAMICARRAY(sh_assSymbols, CShellSymbol, itss) {
      CShellSymbol &ss = *itss;

      // if it is not persistent
      if (! (ss.ss_ulFlags & SSF_PERSISTENT)) {
        // skip it
        continue;
      }

      const char *strUser = (ss.ss_ulFlags & SSF_USER)?"user ":"";

      // get its type
      ShellType &st = _shell_ast[ss.ss_istType];
      // if array
      if (st.st_sttType==STT_ARRAY) {
        // get base type
        ShellType &stBase = _shell_ast[st.st_istBaseType];
        CTString strType;
        // if float
        if (stBase.st_sttType==STT_FLOAT) {
          // dump all members as floats
          for(INDEX i=0; i<st.st_ctArraySize; i++) {
            fScript.FPrintF_t("%s[%d]=(FLOAT)%g;\n", (const char *) ss.ss_strName, i, ((FLOAT*)ss.ss_pvValue)[i]);
          }
        // if index
        } else if (stBase.st_sttType==STT_INDEX) {
          // dump all members as indices
          for(INDEX i=0; i<st.st_ctArraySize; i++) {
            fScript.FPrintF_t("%s[%d]=(INDEX)%d;\n", (const char *) ss.ss_strName, i, ((INDEX*)ss.ss_pvValue)[i]);
          }
        // if string
        } else if (stBase.st_sttType==STT_STRING) {
          // dump all members
          for(INDEX i=0; i<st.st_ctArraySize; i++) {
            fScript.FPrintF_t("%s[%d]=\"%c\";\n", (const char *) ss.ss_strName, i, (ScriptEsc(*(CTString*)ss.ss_pvValue)[i]) );
          }
        // otherwise
        } else {
          ThrowF_t("%s is an array of wrong type", (const char *) ss.ss_strName);
        }
      // if float
      } else if (st.st_sttType==STT_FLOAT) {
        // dump as float
        fScript.FPrintF_t("persistent extern %sFLOAT %s=(FLOAT)%g;\n", strUser, (const char *) ss.ss_strName, *(FLOAT*)ss.ss_pvValue);
      // if index
      } else if (st.st_sttType==STT_INDEX) {
        // dump as index
        fScript.FPrintF_t("persistent extern %sINDEX %s=(INDEX)%d;\n", strUser, (const char *) ss.ss_strName, *(INDEX*)ss.ss_pvValue);
      // if string
      } else if (st.st_sttType==STT_STRING) {
        // dump as index
        fScript.FPrintF_t("persistent extern %sCTString %s=\"%s\";\n", strUser, (const char *) ss.ss_strName, (const char*)ScriptEsc(*(CTString*)ss.ss_pvValue) );
      // otherwise
      } else {
        ThrowF_t("%s of wrong type", (const char *) ss.ss_strName);
      }
    }
  } catch (const char *strError) {
    WarningMessage(TRANS("Cannot save persistent symbols:\n%s"), strError);
  }
}
