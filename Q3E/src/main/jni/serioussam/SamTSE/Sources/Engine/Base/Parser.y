%{
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

#include <Engine/StdH.h>

#include <Engine/Base/Console.h>
#include <Engine/Base/Shell.h>
#include "ParsingSymbols.h"

#include <Engine/Templates/DynamicStackArray.cpp>
#include <Engine/Templates/AllocationArray.cpp>

%}

%{
// turn off over-helpful bit of bison... --ryan.
#ifdef __GNUC__
#define __attribute__(x)
#endif

#define YYERROR_VERBOSE 1
// if error occurs in parsing
void yyerror(const char *str)
{
  // just report the string
  _pShell->ErrorF("%s", str);
};

static BOOL _bExecNextElse = FALSE;
FLOAT fDummy = -666.0f;

static INDEX _iStack = 0;
static UBYTE _ubStack[1024];

INDEX PushExpression(value &v)
{
  if (v.sttType==STT_FLOAT) {
    FLOAT f = v.fFloat;
    memcpy(_ubStack+_iStack, &f, sizeof(f));
    _iStack+=sizeof(f);
    return sizeof(f);
  } else if (v.sttType==STT_INDEX) {
    INDEX i = v.iIndex;
    memcpy(_ubStack+_iStack, &i, sizeof(i));
    _iStack+=sizeof(i);
    return sizeof(i);
  } else if (v.sttType==STT_STRING) {
    CTString &str = _shell_astrTempStrings.Push();
    str = v.strString;
    CTString *pstr = &str;
    memcpy(_ubStack+_iStack, &pstr, sizeof(pstr));
    _iStack+=sizeof(pstr);
    return sizeof(pstr);
  } else {
    return 0;
  }
}

BOOL MatchTypes(value &v1, value &v2)
{
  if (v1.sttType==STT_FLOAT && v2.sttType==STT_FLOAT) {
    return TRUE;
  } else if (v1.sttType==STT_STRING && v2.sttType==STT_STRING) {
    return TRUE;
  } else if (v1.sttType==STT_INDEX && v2.sttType==STT_INDEX) {
    return TRUE;
  } else {
    v1.sttType = STT_ILLEGAL;
    v2.sttType = STT_ILLEGAL;
    _pShell->ErrorF("Type mismatch");
    return FALSE;
  }
}

void Declaration(
  ULONG ulQualifiers, INDEX istType, CShellSymbol &ssNew,
  INDEX (*pPreFunc)(INDEX), void (*pPostFunc)(INDEX))
{
  // if external
  if (ulQualifiers&SSF_EXTERNAL) {
    // get it a new value
    if (_shell_ast[istType].st_sttType==STT_INDEX
      ||_shell_ast[istType].st_sttType==STT_FLOAT) {
      _pvNextToDeclare = &_shell_afExtFloats.Push();
    } else if (_shell_ast[istType].st_sttType==STT_STRING) {
      _pvNextToDeclare = &_shell_astrExtStrings.Push();
    } else {
      NOTHING;
    }
  }

  // if not parsing an external declaration
  if (_pvNextToDeclare==NULL) {
    // error
    _pShell->ErrorF("Only external declarations are supported");
    return;
  }

  // if the symbol is declared already
  if (ssNew.IsDeclared()) {
    // if the declaration is not identical
    if (!ShellTypeIsSame(ssNew.ss_istType, istType) || 
      ((ssNew.ss_ulFlags&SSF_CONSTANT)!=(ulQualifiers&SSF_CONSTANT))) {
      // error
      _pShell->ErrorF("Symbol '%s' is already declared diferrently", (const char *) ssNew.ss_strName);
      return;
    }

    // copy its value
    if (_shell_ast[ssNew.ss_istType].st_sttType==STT_INDEX) {
      *(INDEX*)_pvNextToDeclare = *(INDEX*)ssNew.ss_pvValue;
    } else if (_shell_ast[ssNew.ss_istType].st_sttType==STT_FLOAT) {
      *(FLOAT*)_pvNextToDeclare = *(FLOAT*)ssNew.ss_pvValue;
    } else if (_shell_ast[ssNew.ss_istType].st_sttType==STT_STRING) {
      *(CTString*)_pvNextToDeclare = *(CTString*)ssNew.ss_pvValue;
    } else if (_shell_ast[ssNew.ss_istType].st_sttType==STT_ARRAY) {
      NOTHING;  // array values are not retained
    } else if (_shell_ast[ssNew.ss_istType].st_sttType==STT_FUNCTION) {
      NOTHING;  // function values are not retained
    } else {
      // error
      _pShell->ErrorF("'%s': old value couldn't be retained", (const char *) ssNew.ss_strName);
      return;
    }
  }

  // set the type to given type
  if (!ssNew.IsDeclared()) {
    ssNew.ss_istType = ShellTypeMakeDuplicate(istType);
  }
  // set the value for the external symbol if not already set
  if (ssNew.ss_pvValue==NULL || !(ulQualifiers&SSF_EXTERNAL)) {
    ssNew.ss_pvValue = _pvNextToDeclare;
  }
  // remember qualifiers (if already predeclared - keep old flags)
  ssNew.ss_ulFlags |= ulQualifiers;
  // remember pre and post functions
  if (ssNew.ss_pPreFunc==NULL) {
    ssNew.ss_pPreFunc = (BOOL (*)(void *))pPreFunc;
  }
  if (ssNew.ss_pPostFunc==NULL) {
    ssNew.ss_pPostFunc = (void (*)(void *))pPostFunc;
  }
}

void DoComparison(value &vRes, value &v0, value &v1, int token)
{
  MatchTypes(v0, v1);

  vRes.sttType = STT_INDEX;
  if (v0.sttType == STT_FLOAT) {
    switch (token) {
    case '<': vRes.iIndex = v0.fFloat <v1.fFloat; break;
    case '>': vRes.iIndex = v0.fFloat >v1.fFloat; break;
    case '=': vRes.iIndex = v0.fFloat==v1.fFloat; break;
    case '!': vRes.iIndex = v0.fFloat!=v1.fFloat; break;
    case '}': vRes.iIndex = v0.fFloat>=v1.fFloat; break;
    case '{': vRes.iIndex = v0.fFloat<=v1.fFloat; break;
    default: ASSERT(FALSE);
      vRes.sttType = STT_INDEX;
      vRes.iIndex = 0;
    }
  } else if (v0.sttType == STT_INDEX) {
    switch (token) {
    case '<': vRes.iIndex = v0.iIndex <v1.iIndex; break;
    case '>': vRes.iIndex = v0.iIndex >v1.iIndex; break;
    case '=': vRes.iIndex = v0.iIndex==v1.iIndex; break;
    case '!': vRes.iIndex = v0.iIndex!=v1.iIndex; break;
    case '}': vRes.iIndex = v0.iIndex>=v1.iIndex; break;
    case '{': vRes.iIndex = v0.iIndex<=v1.iIndex; break;
    default: ASSERT(FALSE);
      vRes.sttType = STT_INDEX;
      vRes.iIndex = 0;
    }
  } else if (v0.sttType == STT_STRING) {
    switch (token) {
    case '<': vRes.iIndex = stricmp(v0.strString, v1.strString)  < 0; break;
    case '>': vRes.iIndex = stricmp(v0.strString, v1.strString)  > 0; break;
    case '=': vRes.iIndex = stricmp(v0.strString, v1.strString) == 0; break;
    case '!': vRes.iIndex = stricmp(v0.strString, v1.strString) != 0; break;
    case '}': vRes.iIndex = stricmp(v0.strString, v1.strString) >= 0; break;
    case '{': vRes.iIndex = stricmp(v0.strString, v1.strString) <= 0; break;
    default: ASSERT(FALSE);
      vRes.sttType = STT_INDEX;
      vRes.iIndex = 0;
    }
  } else {
    vRes.sttType = STT_INDEX;
    vRes.iIndex = 0;
  }
}
%}

/* BISON Declarations */

// we need to be reentrant!
%pure_parser

%union {
  value val;                  // for constants and expressions
  arguments arg;               // for function input arguments
  ULONG ulFlags;              // for declaration qualifiers
  INDEX istType;              // for types
  CShellSymbol *pssSymbol;    // for symbols
  struct LValue lvLValue;
  INDEX (*pPreFunc)(INDEX);  // pre-set function for a variable
  void (*pPostFunc)(INDEX); // post-set function for a variable
}

%{
  extern int yylex(YYSTYPE *lvalp);
%}

%token <val> c_float
%token <val> c_int
%token <val> c_string
%token <val> c_char

%token <pssSymbol> identifier

%token k_INDEX
%token k_FLOAT
%token k_CTString
%token k_void
%token k_const
%token k_user
%token k_persistent
%token k_extern
%token k_pre
%token k_post
%token k_help
%token <ulFlags> k_if
%token k_else
%token <ulFlags> k_else_if
%token SHL
%token SHR
%token EQ
%token NEQ
%token GEQ
%token LEQ
%token LOGAND
%token LOGOR
%token block_beg
%token block_end

%type <lvLValue> lvalue
%type <val> expression
%type <ulFlags> declaration_qualifiers
%type <val> opt_string
%type <istType> type_specifier
%type <istType> parameter_list
%type <istType> parameter_list_opt
%type <pPreFunc> pre_func_opt
%type <pPostFunc> post_func_opt
%type <arg> argument_expression_list_opt
%type <arg> argument_expression_list

%right '='
%left LOGAND LOGOR
%left '&' '^' '|'
%left '<' '>' EQ NEQ LEQ GEQ
%left SHL
%left SHR
%left '-' '+'
%left '*' '/' '%'
%left TYPECAST
%left SIGN '!'

%start program

%%

/*/////////////////////////////////////////////////////////
 * Global structure of the source file.
 */
program 
: declaration
| statements
;

block
: block_beg statements block_end
| block_beg statements block_end
;

statements
: /* null */
| statement statements
;

declaration_qualifiers
: /* nothing */ {
  $$ = 0;
}
| declaration_qualifiers k_const {
  $$ = $1 | SSF_CONSTANT;
}
| declaration_qualifiers k_user {
  $$ = $1 | SSF_USER;
}
| declaration_qualifiers k_persistent {
  $$ = $1 | SSF_PERSISTENT;
}
| declaration_qualifiers k_extern {
  $$ = $1 | SSF_EXTERNAL;
}

opt_string
: /* nothing */ {
  $$.strString = "";
}
| c_string {
  // !!!! remove this option
  //_pShell->ErrorF("Warning: symbol comments are not supported");
  $$.strString = $1.strString;
}

type_specifier 
: k_FLOAT {
  $$ = ShellTypeNewFloat();
}
| k_INDEX {
  $$ = ShellTypeNewIndex();
}
| k_CTString {
  $$ = ShellTypeNewString();
}
| k_void {
  $$ = ShellTypeNewVoid();
}

pre_func_opt
: {
  $$ = NULL;
}
| k_pre ':' identifier {
  if (_shell_ast[$3->ss_istType].st_sttType!=STT_FUNCTION
    ||_shell_ast[_shell_ast[$3->ss_istType].st_istBaseType].st_sttType!=STT_INDEX
    ||_shell_ast[$3->ss_istType].st_istFirstArgument!=_shell_ast[$3->ss_istType].st_istLastArgument
    ||_shell_ast[_shell_ast[$3->ss_istType].st_istFirstArgument].st_sttType!=STT_INDEX) {
    _pShell->ErrorF("'%s' must return 'INDEX' and take 'INDEX' as input", (const char *) $3->ss_strName);
  } else {
    void *pv = $3->ss_pvValue;
    $$ = (INDEX(*)(INDEX))$3->ss_pvValue;
  }
}

post_func_opt
: {
  $$ = NULL;
}
| k_post ':' identifier {
  if (_shell_ast[$3->ss_istType].st_sttType!=STT_FUNCTION
    ||_shell_ast[_shell_ast[$3->ss_istType].st_istBaseType].st_sttType!=STT_VOID
    ||_shell_ast[$3->ss_istType].st_istFirstArgument!=_shell_ast[$3->ss_istType].st_istLastArgument
    ||_shell_ast[_shell_ast[$3->ss_istType].st_istFirstArgument].st_sttType!=STT_INDEX) {
    _pShell->ErrorF("'%s' must return 'void' and take 'INDEX' as input", (const char *) $3->ss_strName);
  } else {
    $$ = (void(*)(INDEX))$3->ss_pvValue;
  }
}
;

parameter_list_opt
: /* nothing */ {
  $$ = ShellTypeNewFunction(0);
  ShellTypeAddFunctionArgument($$, ShellTypeNewVoid());
}
| parameter_list {
  $$ = $1;
}
;

parameter_list
: type_specifier {
  $$ = ShellTypeNewFunction(0);
  ShellTypeAddFunctionArgument($$, $1);
}
/*| identifier type_specifier {
  $$ = ShellTypeNewFunction(0);
  ShellTypeAddFunctionArgument($$, $2);
}*/
| parameter_list ',' type_specifier {
  $$ = $1;
  ShellTypeAddFunctionArgument($$, $3);
}
/*| parameter_list ',' identifier type_specifier {
  $$ = $1;
  ShellTypeAddFunctionArgument($$, $4);
} */
;

declaration
: declaration_qualifiers type_specifier identifier pre_func_opt post_func_opt opt_string ';' {
  Declaration($1, $2, *$3, $4, $5);
  ShellTypeDelete($2);
}
| declaration_qualifiers type_specifier identifier '(' parameter_list_opt ')' opt_string ';' {
  // take function from the parameter list and set its return type
  _shell_ast[$5].st_istBaseType = $2;
  $2 = $5;
  // declare it
  Declaration($1, $2, *$3, NULL, NULL);
  // free the type (declaration will make a copy)
  ShellTypeDelete($2);
}
| declaration_qualifiers type_specifier identifier '[' expression  ']' pre_func_opt post_func_opt opt_string ';' {
  if ($5.sttType!=STT_INDEX) {
    _pShell->ErrorF("Array size is not integral");
  }
  $2 = ShellTypeNewArray($2, $5.iIndex);
  Declaration($1, $2, *$3, NULL, NULL);
  ShellTypeDelete($2);
}

statement 
: ';' {
  // dummy
}
| block {
  // dummy
}
| expression ';' {
  // print its value
  if ($1.sttType == STT_VOID) {
    NOTHING;
  } else if ($1.sttType == STT_FLOAT) {
    CPrintF("%g\n", $1.fFloat);
  } else if ($1.sttType == STT_STRING) {
    CPrintF("\"%s\"\n", $1.strString);
  } else if ($1.sttType == STT_INDEX) {
    CPrintF("%d(0x%08X)\n", $1.iIndex, $1.iIndex);
  } else {
    _pShell->ErrorF("Expression cannot be printed");
  }
}
| lvalue '=' expression ';' {
  // if it is constant
  if ($1.lv_pssSymbol->ss_ulFlags&SSF_CONSTANT) {
    _pShell->ErrorF("Symbol '%s' is a constant", (const char *) $1.lv_pssSymbol->ss_strName);
  // if it is not constant
  } else {
    // if it can be changed
    if ($1.lv_pssSymbol->ss_pPreFunc==NULL || $1.lv_pssSymbol->ss_pPreFunc($1.lv_pvAddress)) {
      // if floats
      if ($1.lv_sttType == STT_FLOAT && $3.sttType==STT_FLOAT) {
        // assign value
        *(FLOAT*)$1.lv_pvAddress = $3.fFloat;
      // if indices
      } else if ($1.lv_sttType == STT_INDEX && $3.sttType==STT_INDEX) {
        // assign value
        *(INDEX*)$1.lv_pvAddress = $3.iIndex;

      // if strings
      } else if ($1.lv_sttType == STT_STRING && $3.sttType==STT_STRING) {
        // assign value
        *(CTString*)$1.lv_pvAddress = $3.strString;

      // if assigning index to float
      } else if ($1.lv_sttType == STT_FLOAT && $3.sttType==STT_INDEX) {
        *(FLOAT*)$1.lv_pvAddress = $3.iIndex;
      // otherwise
      } else {
        // error
        _pShell->ErrorF("Cannot assign: different types");
      }

      // call post-change function
      if ($1.lv_pssSymbol->ss_pPostFunc!=NULL) {
        $1.lv_pssSymbol->ss_pPostFunc($1.lv_pvAddress);
      }
    }
  }
}
| declaration_qualifiers type_specifier identifier '=' expression ';' {
  Declaration($1, $2, *$3, NULL, NULL);
  ShellTypeDelete($2);

  CShellSymbol &ssSymbol = *$3;
  // if it is constant
  if (ssSymbol.ss_ulFlags&SSF_CONSTANT) {
    // error
    _pShell->ErrorF("Symbol '%s' is a constant", (const char *) ssSymbol.ss_strName);
  }

  // get symbol type
  ShellTypeType stt = _shell_ast[$2].st_sttType;

  // if floats
  if (stt == STT_FLOAT && $5.sttType==STT_FLOAT) {
    // assign value
    *(FLOAT*)ssSymbol.ss_pvValue = $5.fFloat;
  // if indices
  } else if (stt == STT_INDEX && $5.sttType==STT_INDEX) {
    // assign value
    *(INDEX*)ssSymbol.ss_pvValue = $5.iIndex;
  // if strings
  } else if (stt == STT_STRING && $5.sttType==STT_STRING) {
    // assign value
    *(CTString*)ssSymbol.ss_pvValue = $5.strString;
  // !!!! remove this conversion
  } else if (stt == STT_FLOAT && $5.sttType==STT_INDEX) {
    _pShell->ErrorF("Warning: assigning INDEX to FLOAT!");  
    *(FLOAT*)ssSymbol.ss_pvValue = $5.iIndex;
  } else {
    _pShell->ErrorF("Symbol '%s' and its initializer have different types", (const char *) ssSymbol.ss_strName);
  }
}
| k_help identifier { 
extern void PrintShellSymbolHelp(const CTString &strSymbol);
  PrintShellSymbolHelp($2->ss_strName);
}
| k_help identifier '(' ')' { 
extern void PrintShellSymbolHelp(const CTString &strSymbol);
  PrintShellSymbolHelp($2->ss_strName);
}
| k_help identifier '[' ']' { 
extern void PrintShellSymbolHelp(const CTString &strSymbol);
  PrintShellSymbolHelp($2->ss_strName);
}
| k_if '(' expression ')' { 
  _bExecNextBlock = FALSE;
  if ($3.sttType == STT_INDEX) {
    _bExecNextBlock = $3.iIndex!=0;
  } else if ($3.sttType == STT_FLOAT) {
    _bExecNextBlock = $3.fFloat!=0;
  } else {
    _pShell->ErrorF("If expression is not integral");
  }
  $1 = _bExecNextBlock;
} block {
  _bExecNextElse = !$1;
  _bExecNextBlock = TRUE;
} opt_else
;

opt_else
: /* nothing */

| k_else_if '(' expression ')' {
  if (_bExecNextElse) {  
    _bExecNextBlock = FALSE;
    if ($3.sttType == STT_INDEX) {
      _bExecNextBlock = $3.iIndex!=0;
    } else if ($3.sttType == STT_FLOAT) {
      _bExecNextBlock = $3.fFloat!=0;
    } else {
      _pShell->ErrorF("If expression is not integral");
    }
    $1 = _bExecNextBlock;
  } else {
    _bExecNextBlock = FALSE;
    _bExecNextElse = FALSE;
    $1 = TRUE;
  }
} block {
  _bExecNextElse = !$1;
  _bExecNextBlock = TRUE;
} opt_else

| k_else {
  _bExecNextBlock = _bExecNextElse;  
} block  {
  _bExecNextBlock = TRUE;
}
;

lvalue 
: identifier {
  CShellSymbol &ssSymbol = *$1;
  const ShellType &stType = _shell_ast[ssSymbol.ss_istType];

  $$.lv_pssSymbol = &ssSymbol;
  if (!ssSymbol.IsDeclared()) {
    // error
    _pShell->ErrorF("Identifier '%s' is not declared", (const char *) $1->ss_strName);
    fDummy = -666;
    $$.lv_sttType = STT_VOID;
    $$.lv_pvAddress = &fDummy;
  // if the identifier is a float, int or string
  } else if (stType.st_sttType==STT_FLOAT || stType.st_sttType==STT_INDEX || stType.st_sttType==STT_STRING) {
    // get its value and type
    $$.lv_sttType = stType.st_sttType;
    $$.lv_pvAddress = ssSymbol.ss_pvValue;
  // if the identifier is something else
  } else {
    // error
    _pShell->ErrorF("'%s' doesn't have a value", (const char *) $1->ss_strName);
    fDummy = -666.0f;
    $$.lv_sttType = STT_VOID;
    $$.lv_pvAddress = &fDummy;
  }
}
| identifier '[' expression ']' {
  CShellSymbol &ssSymbol = *$1;
  const ShellType &stType = _shell_ast[ssSymbol.ss_istType];
  $$.lv_pssSymbol = &ssSymbol;

  int iIndex = 0;
  // if subscript is index
  if ($3.sttType==STT_INDEX) {
    // get the index
    iIndex = $3.iIndex;
  // if subscript is not index
  } else {
    // error
    _pShell->ErrorF("Array subscript is not integral");
  }
  // if the symbol is array 
  if (stType.st_sttType==STT_ARRAY) {
    const ShellType &stBase = _shell_ast[stType.st_istBaseType];
    // if it is float or int array
    if (stBase.st_sttType==STT_FLOAT || stBase.st_sttType==STT_INDEX) {
      // if the index is out of range
      if (iIndex<0 || iIndex>=stType.st_ctArraySize) {
        _pShell->ErrorF("Array member out of range");
        fDummy = -666.0f;
        $$.lv_pvAddress = &fDummy;
      } else {
        // get its value and type
        $$.lv_sttType = stBase.st_sttType;
        $$.lv_pvAddress = (FLOAT*)ssSymbol.ss_pvValue+iIndex;
      }
    }
  } else {
    _pShell->ErrorF("'%s[]' doesn't have a value", (const char *) $1->ss_strName);
    fDummy = -666.0f;
    $$.lv_pvAddress = &fDummy;
  }
}
;

argument_expression_list_opt
: /* nothing */ {
  $$.istType = ShellTypeNewFunction(ShellTypeNewVoid());
  ShellTypeAddFunctionArgument($$.istType, ShellTypeNewVoid());
  $$.ctBytes = 0;
}
| argument_expression_list {
  $$ = $1;
}
;

argument_expression_list
: expression {
  $$.istType = ShellTypeNewFunction(ShellTypeNewVoid());
  ShellTypeAddFunctionArgument($$.istType, ShellTypeNewByType($1.sttType));
  $$.ctBytes = PushExpression($1);
}
| argument_expression_list ',' expression {
  $$ = $1;
  ShellTypeAddFunctionArgument($$.istType, ShellTypeNewByType($3.sttType));
  $$.ctBytes += PushExpression($3);
}

expression
: c_float {
  $$.sttType = STT_FLOAT;  
  $$.fFloat = $1.fFloat;
}
| c_int {
  $$.sttType = STT_INDEX;  
  $$.iIndex = $1.iIndex;
}
| c_string {
  $$.sttType = STT_STRING;  
  $$.strString = $1.strString;
}
| lvalue {
  // get its value
  $$.sttType = $1.lv_sttType;
  if ($1.lv_sttType==STT_VOID) {
    NOTHING;
  } else if ($1.lv_sttType==STT_FLOAT) {
    $$.fFloat = *(FLOAT*)$1.lv_pvAddress;
  } else if ($1.lv_sttType==STT_INDEX) {
    $$.iIndex = *(INDEX*)$1.lv_pvAddress;
  } else if ($1.lv_sttType==STT_STRING) {
    $$.strString = (const char*)*(CTString*)$1.lv_pvAddress;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
    _pShell->ErrorF("'%s' is of wrong type", (const char *) $1.lv_pssSymbol->ss_strName);
  }
}
/* shift */
| expression SHL expression {

  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_INDEX) {
    $$.iIndex = $1.iIndex<<$3.iIndex;
  } else {
    _pShell->ErrorF("Wrong arguments for '<<'");
    $$.sttType = STT_INDEX;
    $$.iIndex = -666;
  }
}
| expression SHR expression {

  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_INDEX) {
    $$.iIndex = $1.iIndex>>$3.iIndex;
  } else {
    _pShell->ErrorF("Wrong arguments for '>>'");
    $$.sttType = STT_INDEX;
    $$.iIndex = -666;
  }
}
/* bitwise operators */
| expression '&' expression {

  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_FLOAT) {
    _pShell->ErrorF("'&' is illegal for FLOAT values");
  } else if ($1.sttType == STT_INDEX) {
    $$.iIndex = $1.iIndex&$3.iIndex;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }
}
| expression '|' expression {

  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_FLOAT) {
    _pShell->ErrorF("'|' is illegal for FLOAT values");
  } else if ($1.sttType == STT_INDEX) {
    $$.iIndex = $1.iIndex|$3.iIndex;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }
}
| expression '^' expression {

  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_FLOAT) {
    _pShell->ErrorF("'^' is illegal for FLOAT values");
  } else if ($1.sttType == STT_INDEX) {
    $$.iIndex = $1.iIndex^$3.iIndex;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }
}

/* logical operators */
| expression LOGAND expression {

  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_FLOAT) {
    _pShell->ErrorF("'&&' is illegal for FLOAT values");
  } else if ($1.sttType == STT_INDEX) {
    $$.iIndex = $1.iIndex&&$3.iIndex;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }
}
| expression LOGOR expression {

  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_FLOAT) {
    _pShell->ErrorF("'||' is illegal for FLOAT values");
  } else if ($1.sttType == STT_INDEX) {
    $$.iIndex = $1.iIndex||$3.iIndex;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }
}
/* addition */
| expression '+' expression {

  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_FLOAT) {
    $$.fFloat = $1.fFloat+$3.fFloat;
  } else if ($1.sttType == STT_INDEX) {
    $$.iIndex = $1.iIndex+$3.iIndex;
  } else if ($1.sttType == STT_STRING) {
    CTString &strNew = _shell_astrTempStrings.Push();
    strNew = CTString($1.strString)+$3.strString;
    $$.strString = (const char*)strNew;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }
}
/* substraction */
| expression '-' expression {

  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_FLOAT) {
    $$.fFloat = $1.fFloat-$3.fFloat;
  } else if ($1.sttType == STT_INDEX) {
    $$.iIndex = $1.iIndex-$3.iIndex;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }
}

/* multiplication */
| expression '*' expression {

  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_FLOAT) {
    $$.fFloat = $1.fFloat*$3.fFloat;
  } else if ($1.sttType == STT_INDEX) {
    $$.iIndex = $1.iIndex*$3.iIndex;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }

}
/* division */
| expression '/' expression {

  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_FLOAT) {
    $$.fFloat = $1.fFloat/$3.fFloat;
  } else if ($1.sttType == STT_INDEX) {
    if ($3.iIndex==0) {
      _pShell->ErrorF("Division by zero!\n");
      $$.iIndex = 0;
    } else {
      $$.iIndex = $1.iIndex/$3.iIndex;
    }
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }

}

/* modulo */
| expression '%' expression {
  MatchTypes($1, $3);

  $$.sttType = $1.sttType;
  if ($1.sttType == STT_FLOAT) {
    _pShell->ErrorF("'%' is illegal for FLOAT values");
  } else if ($1.sttType == STT_INDEX) {
    $$.iIndex = $1.iIndex%$3.iIndex;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }

}

/* comparisons */
| expression '<' expression {
  DoComparison($$, $1, $3, '<');
}
| expression '>' expression {
  DoComparison($$, $1, $3, '>');
}
| expression EQ expression {
  DoComparison($$, $1, $3, '=');
}
| expression NEQ expression {
  DoComparison($$, $1, $3, '!');
}
| expression GEQ expression {
  DoComparison($$, $1, $3, '}');
}
| expression LEQ expression {
  DoComparison($$, $1, $3, '{');
}

// unary minus

| '-' expression %prec SIGN {
  $$.sttType = $2.sttType;
  if ($2.sttType == STT_FLOAT) {
    $$.fFloat = -$2.fFloat;
  } else if ($2.sttType == STT_INDEX) {
    $$.iIndex = -$2.iIndex;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }
}

// unary plus

| '+' expression %prec SIGN {
  $$.sttType = $2.sttType;
  if ($2.sttType == STT_FLOAT) {
    $$.fFloat = $2.fFloat;
  } else if ($2.sttType == STT_INDEX) {
    $$.iIndex = $2.iIndex;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }
}

| '!' expression %prec SIGN {
  $$.sttType = $2.sttType;
  if ($2.sttType == STT_FLOAT) {
    _pShell->ErrorF("'!' is illegal for FLOAT values");
    $$.fFloat = $2.fFloat;
  } else if ($2.sttType == STT_INDEX) {
    $$.iIndex = !$2.iIndex;
  } else {
    $$.sttType = STT_FLOAT;
    $$.fFloat = -666.0f;
  }
}

| '(' k_FLOAT ')' expression %prec TYPECAST {
  $$.sttType = STT_FLOAT;
  if ($4.sttType == STT_FLOAT) {
    $$.fFloat = $4.fFloat;
  } else if ($4.sttType == STT_INDEX) {
    $$.fFloat = FLOAT($4.iIndex);
  } else if ($4.sttType == STT_STRING) {
    $$.fFloat = atof($4.strString);
  } else {
    _pShell->ErrorF("Cannot convert to FLOAT");
    $$.sttType = STT_VOID;
  }
}

| '(' k_INDEX ')' expression %prec TYPECAST {
  $$.sttType = STT_INDEX;
  if ($4.sttType == STT_FLOAT) {
    $$.iIndex = INDEX($4.fFloat);
  } else if ($4.sttType == STT_INDEX) {
    $$.iIndex = $4.iIndex;
  } else if ($4.sttType == STT_STRING) {
    $$.iIndex = atol($4.strString);
  } else {
    _pShell->ErrorF("Cannot convert to INDEX");
    $$.sttType = STT_VOID;
  }
}

| '(' k_CTString ')' expression %prec TYPECAST {
  CTString &strNew = _shell_astrTempStrings.Push();
  $$.sttType = STT_STRING;
  if ($4.sttType == STT_FLOAT) {
    strNew.PrintF("%g", $4.fFloat);
  } else if ($4.sttType == STT_INDEX) {
    strNew.PrintF("%d", $4.iIndex);
  } else if ($4.sttType == STT_STRING) {
    strNew = $4.strString;
  } else {
    _pShell->ErrorF("Cannot convert to CTString");
    $$.sttType = STT_VOID;
  }
  $$.strString = (const char*)strNew;
}

// function call
| identifier '(' argument_expression_list_opt ')' {
  // if the identifier is not declared
  if (!$1->IsDeclared()) {
    // error
    _pShell->ErrorF("Identifier '%s' is not declared", (const char *) $1->ss_strName);
  // if the identifier is declared
  } else {
    // get its type
    ShellType &stFunc = _shell_ast[$1->ss_istType];

    // if the identifier is a function
    if (stFunc.st_sttType==STT_FUNCTION) {
      // determine result type
      ShellType &stResult =  _shell_ast[stFunc.st_istBaseType];
      // match argument list result to that result
      _shell_ast[_shell_ast[$3.istType].st_istBaseType].st_sttType = stResult.st_sttType;
      // if types are same
      if (ShellTypeIsSame($3.istType, $1->ss_istType)) {
        bool callfunc = true;

// !!! FIXME: maybe just dump the win32 codepath here? This will break on Win64, and maybe break on different compilers/compiler versions, etc.
#ifdef PLATFORM_WIN32
        #define CALLPARAMS
        #define FUNCSIG void
        #define PUSHPARAMS memcpy(_alloca($3.ctBytes), _ubStack+_iStack-$3.ctBytes, $3.ctBytes);
#else
        // This is possibly more portable, but no less scary than the alloca hack.
        #define MAXSCRIPTFUNCARGS 5
        void *ARG[MAXSCRIPTFUNCARGS];
        if (($3.ctBytes > sizeof (ARG)))
        {
            _pShell->ErrorF("Function '%s' has too many arguments!", (const char *) $1->ss_strName);
            callfunc = false;
        }
        else
        {
            memcpy(ARG, _ubStack+_iStack-$3.ctBytes, $3.ctBytes);
            memset(((char *) ARG) + $3.ctBytes, '\0', sizeof (ARG) - $3.ctBytes);
        }
        #define PUSHPARAMS
        #define FUNCSIG void*, void*, void*, void*, void*
        #define CALLPARAMS ARG[0], ARG[1], ARG[2], ARG[3], ARG[4]
#endif
  
        if (callfunc) {
          // if void
          if (stResult.st_sttType==STT_VOID) {
            // just call the function
            $$.sttType = STT_VOID;
            PUSHPARAMS;
            ((void (*)(FUNCSIG))$1->ss_pvValue)(CALLPARAMS);
          // if index
          } else if (stResult.st_sttType==STT_INDEX) {
            // call the function and return result
            $$.sttType = STT_INDEX;
            PUSHPARAMS;
            $$.iIndex = ((INDEX (*)(FUNCSIG))$1->ss_pvValue)(CALLPARAMS);
          // if float
          } else if (stResult.st_sttType==STT_FLOAT) {
            // call the function and return result
            $$.sttType = STT_FLOAT;
            PUSHPARAMS;
            $$.fFloat = ((FLOAT (*)(FUNCSIG))$1->ss_pvValue)(CALLPARAMS);
          // if string
          } else if (stResult.st_sttType==STT_STRING) {
            // call the function and return result
            $$.sttType = STT_STRING;
            CTString &strNew = _shell_astrTempStrings.Push();
            PUSHPARAMS;
            strNew = ((CTString (*)(FUNCSIG))$1->ss_pvValue)(CALLPARAMS);
            $$.strString = (const char*)strNew;
          } else {
            ASSERT(FALSE);
            $$.sttType = STT_FLOAT;
            $$.fFloat = -666.0f;
          }
        }
      // if types are different
      } else {
        // error
        $$.sttType = STT_VOID;
        _pShell->ErrorF("Wrong parameters for '%s'", (const char *) $1->ss_strName);
      }
    // if the identifier is something else
    } else {
      // error
      $$.sttType = STT_VOID;
      _pShell->ErrorF("Can't call '%s'", (const char *) $1->ss_strName);
    }
  }

  // pop arguments and free type info
  _iStack-=$3.ctBytes;
  ShellTypeDelete($3.istType);
}
// brackets
| '(' expression ')' {
  $$ = $2;
}
;

%%
