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

#include "ParsingSymbols.h"
#include "Parser.h"

#include <Engine/Base/FileName.h>
#include <Engine/Base/CTString.h>
#include <Engine/Base/CTString.inl>
#include <Engine/Base/Shell.h>
#include <Engine/Base/Console.h>

#include <Engine/Templates/DynamicStackArray.cpp>

#define YY_DECL int yylex (YYSTYPE *lvalp)
#define yylval (*lvalp)

#ifdef __cplusplus
extern "C" { int yywrap(void); }
#endif

int yywrap(void)
{
  // no more bufers
  return 1;
};

static int _iBlockDepth = 0;
static int _iDummyDepth = 0;
static CTString _strCmd = ""; // currently parsed command
static int _ctCmdParam = 1; // current parameter index
static BOOL _bCmdParamCountErrorReported = FALSE;

void TranscriptEsc(CTString &str)
{
  char *pchSrc = (char *)(const char *)str;
  char *pchDst = (char *)(const char *)str;
  // if quoted
  if (pchDst[0] == '"') {
    int len = strlen(pchDst);
    pchDst[len-1] = 0;
    memmove(pchDst, pchDst+1, len-1);
  }
  for (;;pchSrc++, pchDst++) {
    if (*pchSrc==0) {
      break;
    }
    if (*pchSrc!='\\') {
      *pchDst = *pchSrc;
      continue;
    }
    pchSrc++;
    switch(*pchSrc) {
    case 'n': *pchDst = 10; break;
    case 'r': *pchDst = 13; break;
    default: *pchDst = *pchSrc; break;
    }
  }
  *pchDst=0;
}

%}

%x COMMENT
%x DUMMYBLOCK
%x INCLUDE
%x COMMAND

DIGIT		[0-9]
HEXDIGIT [0-9A-Fa-f]
IDENTIFIERFIRST [A-Za-z_]
IDENTIFIEROTHER [A-Za-z0-9_]
DOUBLEQUOTE	\"
STRINGCONTENT	([^\"]|(\\\"))
CHARCONTENT	([^\']|(\\\'))
NONEXP_FLT  ({DIGIT}+"."{DIGIT}*)
EXP_FLT (({DIGIT}+("."({DIGIT}*)?)?)("E"|"e")("+"|"-")?{DIGIT}+)

PARAMCONTENT	([^\ \n\";]|(\\\ ))

%%

 /*\0 yyterminate();*/

 /* Include file parsing. */
"include"       BEGIN(INCLUDE);

<INCLUDE>[ \t]*"\""    /* eat the whitespace until first quote */
<INCLUDE>[^"\""]*"\""   { /* get the include file name until second quote */
  if (ShellGetBufferStackDepth() >= SHELL_MAX_INCLUDE_LEVEL) {
    _pShell->ErrorF("Script files nested too deeply");
  }
  char strFileName[256];
  strcpy(strFileName, yytext);
  strFileName[strlen(strFileName)-1] = 0;

  CTString strIncludeFile;
  try {
    strIncludeFile.Load_t(CTString(strFileName));
    ShellPushBuffer(strFileName, strIncludeFile, FALSE);

  } catch (const char *strError) {
    _pShell->ErrorF("Cannot load script file '%s': %s", yytext, strError);
  }
  BEGIN(INITIAL);
}
<INCLUDE>.    {  /* something unrecognized inside include statement */
  _pShell->ErrorF("Wrong syntax for include statement");
  BEGIN(INITIAL);
}
 
 /* command parsing */
<COMMAND>{PARAMCONTENT}*|"\""{STRINGCONTENT}*"\"" { // for each parameter
  _ctCmdParam++;
  CTString strParam = yytext;
  TranscriptEsc(strParam);
  // insert the parameter in the command string
  CTString strParamNo = CTString(1, "%%%d", _ctCmdParam);
  if (strParam.FindSubstr(strParamNo)!=-1) {
    _pShell->ErrorF("Parameter substitution recursion detected!");
  } else {
    INDEX ctFound;
    for(ctFound=0;; ctFound++) {
      if (!_strCmd.ReplaceSubstr(strParamNo, strParam)) {
        break;
      }
    }
    // if not found and parameter count error not reported yet
    if (ctFound==0 && !_bCmdParamCountErrorReported) {
      // report error
      _bCmdParamCountErrorReported = TRUE;
      _pShell->ErrorF("Too many parameters for command expansion");
    }
  }
}
<COMMAND>\n|; { 
                  // at the end of the command

  // switch to the new input buffer with that command
  ShellPushBuffer(ShellGetBufferName(), _strCmd, FALSE);
  BEGIN(INITIAL);
}

<<EOF>> {
  if (ShellPopBuffer()) {
    yyterminate();
  }
}

 /* special data types */
"FLOAT"             { return(k_FLOAT);}
"INDEX"             { return(k_INDEX);}
"CTString"          { return(k_CTString);}

 /* keywords */
"void"              { return(k_void); }
"const"             { return(k_const); }
"user"              { return(k_user); }
"persistent"        { return(k_persistent); }
"extern"            { return(k_extern); }
"pre"               { return(k_pre); }
"post"              { return(k_post); }
"help"              { return(k_help); }
"if"                { return(k_if); }
"else"              { return(k_else); }
"else"" "*"if"      { return(k_else_if); }

"<=" { return (LEQ); }
">=" { return (GEQ); }
"==" { return (EQ);  }
"!=" { return (NEQ); }

">>" { return (SHR); }
"<<" { return (SHL); }

"&&" { return (LOGAND); }
"||" { return (LOGOR); }

 /* single character operators and punctuations */
";"|"("|")"|"="|"+"|"-"|"<"|">"|"!"|"|"|"&"|"*"|"/"|"%"|"^"|"["|"]"|":"|","|"."|"?"|"~" {
  return(yytext[0]);}

 /* constants */
{DIGIT}+                  { yylval.val.iIndex = atoi(yytext); return(c_int); }
"0x"{HEXDIGIT}+           { yylval.val.iIndex = strtoul(yytext+2, NULL, 16); return(c_int); }
{NONEXP_FLT}("f"|"F")?    { yylval.val.fFloat = (float) atof(yytext); return(c_float); }
{EXP_FLT}("f"|"F")?       { yylval.val.fFloat = (float) atof(yytext); return(c_float); }
"\""{STRINGCONTENT}*"\""  { 
  CTString &strNew = _shell_astrTempStrings.Push();
  // remove double-quotes
  strNew = yytext;
  // parse escape symbols and remove double quotes
  TranscriptEsc(strNew);
  yylval.val.strString = (const char*)strNew;
  return(c_string); 
}
"'"{CHARCONTENT}"'"       { return(c_char); }

 /* identifier */
{IDENTIFIERFIRST}{IDENTIFIEROTHER}* { 
  // get the symbol
  yylval.pssSymbol =  _pShell->GetSymbol(yytext, FALSE);
  BOOL bCommand = FALSE;
  // if it is string
  if (_shell_ast[yylval.pssSymbol->ss_istType].st_sttType==STT_STRING) {
    // get the value
    CTString str = *(CTString*)yylval.pssSymbol->ss_pvValue;
    // if the value tells that it is a command
    if (str.RemovePrefix("!command ")) {
      // parse the command
      bCommand = TRUE;
      _strCmd = str;
      _ctCmdParam = 0;
      _bCmdParamCountErrorReported = FALSE;
      BEGIN(COMMAND);
    }
  }
  // normally, just return the identifier
  if (!bCommand) {
    return(identifier);
  }
}
${IDENTIFIERFIRST}{IDENTIFIEROTHER}* { // special case of identifier, used to bypass command parsing
  // get the symbol
  yylval.pssSymbol =  _pShell->GetSymbol(yytext+1, FALSE);
  return(identifier);
}

 /* eat up or execute blocks */
"{" { 
  _iBlockDepth++;
  if (!_bExecNextBlock) {
    _iDummyDepth++;
    BEGIN(DUMMYBLOCK); 
  }
  return block_beg;
}

"}" { 
  _iBlockDepth--;
  if (_iBlockDepth<0) {
   _pShell->ErrorF("Mismatched '}'");
  }
  return block_end;
}
<DUMMYBLOCK>"{" {
  _iBlockDepth++;
  _iDummyDepth++;
}
<DUMMYBLOCK>"}" { 
  _iBlockDepth--;
  _iDummyDepth--;
  if (_iDummyDepth==0) {
    BEGIN(INITIAL); 
    return block_end;
  } 
}
<DUMMYBLOCK>.    {}

 /* eat up comments */
"/*"          { BEGIN(COMMENT); }
<COMMENT>"*/" { BEGIN(INITIAL); }
<COMMENT>.    {}
"//"[^\n]*\n { ShellCountOneLine(); }


 /* eat up whitespace */
<INITIAL,COMMAND>[ \t]+	 {
}
 /* eat up linefeeds and count lines in all conditions */
<*>\n	{
  ShellCountOneLine();
}

 /* for all unrecognized characters */
<INITIAL,COMMAND>. {
  // report an error
  _pShell->ErrorF("Unrecognized character '%c' (ASCII 0x%02x)", yytext[0], yytext[0] );
}

%%

