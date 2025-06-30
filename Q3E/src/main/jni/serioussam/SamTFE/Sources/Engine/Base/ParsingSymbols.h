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

#include <Engine/Base/Shell_internal.h>
#include <Engine/Templates/AllocationArray.h>

// needed for parser and scanner
#ifdef PLATFORM_WIN32
#define alloca _alloca
#endif

// for static linking mojo...
#define yyparse yyparse_engine_base_parser
#define yyerror yyerror_engine_base_parser

extern void yyerror(const char *s);
extern int yyparse(void);

typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string(const char *str);
void yy_delete_buffer(YY_BUFFER_STATE b);

// declarations for recursive shell script parsing
#define SHELL_MAX_INCLUDE_LEVEL 64 // this is both for includes _and_ nested parsing!

extern int _bExecNextBlock;

#define YY_NEVER_INTERACTIVE 1

// used for communication between CShell and parser
// value for declaring external symbol (defined in exe or dll code)
extern void *_pvNextToDeclare; 

extern const char * _shell_strFileName;
// temporary values for parsing
extern CDynamicStackArray<CTString> _shell_astrTempStrings;
// values for extern declarations
extern CDynamicStackArray<CTString> _shell_astrExtStrings;
extern CDynamicStackArray<FLOAT> _shell_afExtFloats;


// all types are allocated here
extern CAllocationArray<ShellType> _shell_ast;
extern INDEX _shell_istUndeclared;

// make a new type for a basic type
INDEX ShellTypeNewVoid(void);
INDEX ShellTypeNewIndex(void);
INDEX ShellTypeNewFloat(void);
INDEX ShellTypeNewString(void);
INDEX ShellTypeNewByType(enum ShellTypeType stt);

// make a new pointer type
INDEX ShellTypeNewPointer(INDEX istBaseType);
// make a new array type
INDEX ShellTypeNewArray(INDEX istMemberType, int iArraySize);
// make a new function type
INDEX ShellTypeNewFunction(INDEX istReturnType);

// add an argument to a function from the left
void ShellTypeAddFunctionArgument(INDEX istFunction, INDEX istArgument);

// make a copy of a type tree
INDEX ShellTypeMakeDuplicate(INDEX istOriginal);

// compare two types
BOOL ShellTypeIsSame(INDEX ist1, INDEX ist2);

// delete an entire type tree
void ShellTypeDelete(INDEX istToDelete);

// print c-like syntax description of the type
char *ShellTypeSPrintF(char *strDestination, char *strDeclarator, INDEX istToPrint);

BOOL ShellTypeIsPointer(INDEX ist);
BOOL ShellTypeIsArray(INDEX ist);
BOOL ShellTypeIsIntegral(INDEX ist);

struct LValue {
  enum ShellTypeType lv_sttType;
  CShellSymbol *lv_pssSymbol;
  void *lv_pvAddress;
};

struct value {
  enum ShellTypeType sttType;
  float fFloat;               // for float constants
  INDEX iIndex;               // for index constants
  const char *strString;      // for string constants
};

struct arguments {
  INDEX istType;    // type of arguments
  INDEX ctBytes;    // total number of bytes taken by arguments on stack
};

void ShellPushBuffer(const char *strName, const char *strBuffer, BOOL bParserEnd);
BOOL ShellPopBuffer(void);
const char *ShellGetBufferName(void);
int ShellGetBufferLineNumber(void);
const char *ShellGetBufferContents(void);
int ShellGetBufferStackDepth(void);

void ShellCountOneLine(void);
