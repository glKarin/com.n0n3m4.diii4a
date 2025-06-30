%option prefix="syy"
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

#include "ParsingSmbs.h"
#include "smcPars.h"

#include <Engine/Base/CTString.h>
#include <Engine/Base/CTString.inl>
#include <Engine/Base/FileName.h>
#include <Engine/Base/ErrorReporting.h>

#include <Engine/Templates/DynamicStackArray.cpp>
extern CTFileName _fnmApplicationPath;

int syywrap(void)
{
  // no more buffers
  return 1;
};

// declarations for recursive SMC script parsing
struct BufferStackEntry {
  YY_BUFFER_STATE bse_bs;
  const char *bse_strName;
  const char *bse_strContents;
  int bse_iLineCt;
  BOOL bse_bParserEnd;
};

static BufferStackEntry _abseBufferStack[SMC_MAX_INCLUDE_LEVEL];
static int _ibsBufferStackTop = -1;

void SMCPushBuffer(const char *strName, const char *strBuffer, BOOL bParserEnd)
{
  _ibsBufferStackTop++;

  _abseBufferStack[_ibsBufferStackTop].bse_strContents = strdup(strBuffer);
  _abseBufferStack[_ibsBufferStackTop].bse_strName = strdup(strName);
  _abseBufferStack[_ibsBufferStackTop].bse_iLineCt = 1;
  _abseBufferStack[_ibsBufferStackTop].bse_bParserEnd = bParserEnd;

  _abseBufferStack[_ibsBufferStackTop].bse_bs = syy_scan_string((char*)(const char*)strBuffer);

  syy_switch_to_buffer(_abseBufferStack[_ibsBufferStackTop].bse_bs);
}
BOOL SMCPopBuffer(void)
{
  syy_delete_buffer( _abseBufferStack[_ibsBufferStackTop].bse_bs);
  free((void*)_abseBufferStack[_ibsBufferStackTop].bse_strName);
  free((void*)_abseBufferStack[_ibsBufferStackTop].bse_strContents);
  BOOL bParserEnd = _abseBufferStack[_ibsBufferStackTop].bse_bParserEnd;

  _ibsBufferStackTop--;

  if (_ibsBufferStackTop>=0) {
    syy_switch_to_buffer(_abseBufferStack[_ibsBufferStackTop].bse_bs);
  }
  return bParserEnd;
}
const char *SMCGetBufferName(void)
{
  return _abseBufferStack[_ibsBufferStackTop].bse_strName;
}
int SMCGetBufferLineNumber(void)
{
  return _abseBufferStack[_ibsBufferStackTop].bse_iLineCt;
}
int SMCGetBufferStackDepth(void)
{
  return _ibsBufferStackTop;
}
const char *SMCGetBufferContents(void)
{
  return _abseBufferStack[_ibsBufferStackTop].bse_strContents;
}
void SMCCountOneLine(void)
{
  _abseBufferStack[_ibsBufferStackTop].bse_iLineCt++;
}
%}

%x COMMENT
%x INCLUDE

DIGIT		[0-9]
HEXDIGIT [0-9A-Fa-f]
DOUBLEQUOTE	\"
STRINGCONTENT	([^\"]|(\\\"))
NONEXP_FLT  ({DIGIT}+"."{DIGIT}*)
EXP_FLT (({DIGIT}+("."({DIGIT}*)?)?)("E"|"e")("+"|"-")?{DIGIT}+)

%%

"#INCLUDE"               BEGIN(INCLUDE);
"SE_SMC"                { return(k_SE_SMC); }
"SE_END"                { return(k_SE_END); }
"TFNM"                  { return(k_TFNM);   }
"NAME"                  { return(k_NAME);   }
"MESH"                  { return(k_MESH);   }
"SKELETON"              { return(k_SKELETON);}
"ANIMSET"               { return(k_ANIMSET);}
"ANIMATION"             { return(K_ANIMATION);}
"TEXTURES"              { return(k_TEXTURES);}
"PARENTBONE"            { return(k_PARENTBONE);}
"OFFSET"                { return(k_OFFSET);}
"COLISION"              { return(k_COLISION);}
"ANIMSPEED"             { return(k_ANIMSPEED);}
"COLOR"                 { return(k_COLOR);}
"ALLFRAMESBBOX"         { return(k_ALLFRAMESBBOX);}

<INCLUDE>[ \t]*"\""      /* eat the whitespace */
<INCLUDE>[^"\""]*"\""   { /* got the include file name */

  if (SMCGetBufferStackDepth() >= SMC_MAX_INCLUDE_LEVEL) {
    ThrowF_t("File '%s' line %d\nIncludes nested too deeply '%s'",SMCGetBufferName(), SMCGetBufferLineNumber(),yytext);
  }
  char strFileName[256];
  strcpy(strFileName, yytext);
  strFileName[strlen(strFileName)-1] = 0;

  CTString strIncludeFile;
  try {
    strIncludeFile.Load_t(CTString(strFileName));
    SMCPushBuffer(strFileName, strIncludeFile, FALSE);

  } catch(char *strError) {
    (void)strError;
    ThrowF_t("File '%s'\n Could not open '%s' (line %d)",SMCGetBufferName(), strFileName, SMCGetBufferLineNumber());
  }
  BEGIN(INITIAL);
}
<INCLUDE>.    {  /* something unrecognized inside include statement */
  BEGIN(INITIAL);
  ThrowF_t("File '%s'\n Wrong syntax for include statement",SMCGetBufferName());
}
<<EOF>> {
  if (SMCPopBuffer()) {
    yyterminate();
  }
}


 /* single character operators and punctuations */
";"|","|"{"|"}" {
  return(yytext[0]);}

 /* constants */

"-"?{DIGIT}+                  { syylval.i = atoi(yytext); return(c_int); }
"0x"{HEXDIGIT}+               { syylval.i = strtoul(yytext+2, NULL, 16); return(c_int);}
"-"?{NONEXP_FLT}("f"|"F")?    { syylval.f = (float) atof(yytext); return(c_float); }
"-"?{EXP_FLT}("f"|"F")?       { syylval.f = (float) atof(yytext); return(c_float); }
"\""{STRINGCONTENT}*"\""  { 
  char *strNew;
  // remove double-quotes
  yytext[strlen(yytext)-1] = 0;
  strNew = yytext+1;
  syylval.str = (const char*)strNew;
  return(c_string); 
}

 /* eat up comments */
"/*"          { BEGIN(COMMENT); }
<COMMENT>"* /" { BEGIN(INITIAL); }
<COMMENT>.    {}
"//"[^\n]*\n { SMCCountOneLine(); }

 /* eat up whitespace */
[ \t]+	 {
}
 /* eat up linefeeds and count lines in all conditions */
<*>\n	{
  SMCCountOneLine();;
}

 /* for all unrecognized characters */
. {
  // report an error
  ThrowF_t("File '%s'\n Unrecognized character '%c' (line %d)", SMCGetBufferName(), yytext[0], SMCGetBufferLineNumber());
  //ThrowF_t("Unrecognized character '%c' in line %d)", yytext[0], _yy_iLine );
}

%%

