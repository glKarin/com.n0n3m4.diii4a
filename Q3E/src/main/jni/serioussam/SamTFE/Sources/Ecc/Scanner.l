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

// rcg10042001 Changed to specify Ecc directory...
#include "Ecc/StdH.h"
#include "Ecc/Main.h"
#include "Ecc/Parser.h"

#define YY_NEVER_INTERACTIVE 1


int iLastLine = -1;

#define RETURN(symbolname)            \
  if (iLastLine==_iLinesCt || !_bTrackLineInformation){\
    yylval = yytext;                  \
  } else {                            \
    char strLine[512];                \
    sprintf(strLine, "\n#line %d \"%s\"\n", _iLinesCt, _strInputFileName);\
    yylval = SType(strLine)+yytext;  \
    iLastLine=_iLinesCt;             \
  }                                  \
  return symbolname;

char *_strCppBlock;

%}

%x COMMENT
%x CPPBLOCK

DIGIT		[0-9]
HEXDIGIT [0-9A-Fa-f]
IDENTIFIERFIRST [A-Za-z_]
IDENTIFIEROTHER [A-Za-z0-9_]
DOUBLEQUOTE	\"
NONEXP_FLT  ({DIGIT}+"."{DIGIT}*)
EXP_FLT (({DIGIT}+("."({DIGIT}*)?)?)("E"|"e")("+"|"-")?{DIGIT}+)

%%

 /* standard cpp-keywords */
"while"     {RETURN(k_while);}
"switch"    {RETURN(k_switch);}
"case"      {RETURN(k_case);}
"for"       {RETURN(k_for);}
"if"        {RETURN(k_if);}
"else"      {RETURN(k_else);}
"enum"      {RETURN(k_enum);}
"class"     {RETURN(k_class);}
"do"        {RETURN(k_do);}
"void"      {RETURN(k_void);}
"const"     {RETURN(k_const);}
"inline"    {RETURN(k_inline);}
"static"    {RETURN(k_static);}
"virtual"   {RETURN(k_virtual);}
"return"    {RETURN(k_return);}
"autowait"  {RETURN(k_autowait);}
"autocall"  {RETURN(k_autocall);}
"waitevent" {RETURN(k_waitevent);}

 /* aditional keywords */
"event"     {RETURN(k_event);}
"name"      {RETURN(k_name);}
"thumbnail" {RETURN(k_thumbnail);}
"features"  {RETURN(k_features);}
"uses"      {RETURN(k_uses);}
"export"    {RETURN(k_export);}

"texture" {RETURN(k_texture);}
"sound"   {RETURN(k_sound);}
"model"   {RETURN(k_model);}

"properties"  {RETURN(k_properties);}
"components"  {RETURN(k_components);}
"functions"   {RETURN(k_functions);}
"procedures"  {RETURN(k_procedures);}

"wait"      {RETURN(k_wait     );}
"on"        {RETURN(k_on       );}
"otherwise" {RETURN(k_otherwise);}

"call"      {RETURN(k_call     );}
"jump"      {RETURN(k_jump     );}
"stop"      {RETURN(k_stop     );}
"resume"    {RETURN(k_resume   );}
"pass"      {RETURN(k_pass     );}

 /* special data types */
"CTString"          {RETURN(k_CTString);}
"CTStringTrans"     {RETURN(k_CTStringTrans);}
"CTFileName"        {RETURN(k_CTFileName);}
"CTFileNameNoDep"   {RETURN(k_CTFileNameNoDep);}
"BOOL"              {RETURN(k_BOOL);}
"COLOR"             {RETURN(k_COLOR);}
"FLOAT"             {RETURN(k_FLOAT);}
"INDEX"             {RETURN(k_INDEX);}
"RANGE"             {RETURN(k_RANGE);}
"CEntityPointer"    {RETURN(k_CEntityPointer);}
"CModelObject"      {RETURN(k_CModelObject);}
"CModelInstance"    {RETURN(k_CModelInstance);}
"CAnimObject"       {RETURN(k_CAnimObject);}
"CSoundObject"      {RETURN(k_CSoundObject);}
"CPlacement3D"      {RETURN(k_CPlacement3D);}
"FLOATaabbox3D"     {RETURN(k_FLOATaabbox3D);}
"FLOATmatrix3D"     {RETURN(k_FLOATmatrix3D);}
"FLOATquat3D"       {RETURN(k_FLOATquat3D);}
"ANGLE"             {RETURN(k_ANGLE);};
"ANIMATION"         {RETURN(k_ANIMATION);};
"ILLUMINATIONTYPE"  {RETURN(k_ILLUMINATIONTYPE);};
"FLOAT3D"           {RETURN(k_FLOAT3D);};
"ANGLE3D"           {RETURN(k_ANGLE3D);};
"FLOATplane3D"      {RETURN(k_FLOATplane3D);};
"flags"             {RETURN(k_FLAGS);};

 /* single character operators and punctuations */
";"|"("|")"|"{"|"}"|"="|"+"|"-"|"<"|">"|"!"|"|"|"&"|"*"|"/"|"%"|"^"|"["|"]"|":"|","|"."|"?"|"~" {
  RETURN(yytext[0]);}

 /* constants */
{DIGIT}+                  { RETURN(c_int); }
"0x"{HEXDIGIT}+           { RETURN(c_int)};
{NONEXP_FLT}("f"|"F")?    { RETURN(c_int); }
{EXP_FLT}("f"|"F")?       { RETURN(c_int); }
(\"[^\n"]+\")|(\"\")      { RETURN(c_string); }
(\'[^\n']+\')|(\'\')      { RETURN(c_char); }

 /* identifier */
{IDENTIFIERFIRST}{IDENTIFIEROTHER}* {RETURN(identifier);}

 /* specially bracketed cpp blocks */
"%{" {
  char strLine[512];
  sprintf(strLine, "#line %d \"%s\"\n", _iLinesCt, _strInputFileName );
  _strCppBlock = strdup(strLine);
  BEGIN(CPPBLOCK);
}
<CPPBLOCK>"%}" {
  BEGIN(INITIAL);
  yylval = SType(_strCppBlock);
  return cppblock;
}
<CPPBLOCK>\n	{
  _iLinesCt++;
  _strCppBlock = (SType(_strCppBlock)+yytext).strString;
 }
<CPPBLOCK>.*   {
  _strCppBlock = (SType(_strCppBlock)+yytext).strString;
}

 /* eat up comments */
"/*"          { BEGIN(COMMENT); }
<COMMENT>"*/" { BEGIN(INITIAL); }
<COMMENT>.    {}
"//"[^\n]*\n { _iLinesCt++; }

 /* eat up whitespace */
[ \t]+	 {
}
 /* eat up linefeeds and count lines in all conditions */
<*>\n	{
  _iLinesCt++;
 }
 /* for all unrecognized characters */
. {
  // report an error
  char strError[80];
  sprintf(strError, "Unrecognized character '%c' (ASCII 0x%02x)\n", 
	  yytext[0], yytext[0] );
  yyerror(strError);
}

%%

