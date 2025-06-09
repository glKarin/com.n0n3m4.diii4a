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

// needed for parser and scanner
#ifdef __GNUC__
extern int engine_ska_yylex(void);
extern void engine_ska_yyerror(const char *s);
extern int engine_ska_yyparse(void);
extern void engine_ska_yyrestart(FILE *f);
#else
extern int yylex(void);
extern void yyerror(const char *s);
extern int syyparse(void);
extern void syyrestart(FILE *f);
#endif

#define YY_NEVER_INTERACTIVE 1

#define SMC_MAX_INCLUDE_LEVEL 32
// temporary values for parsing
extern INDEX _yy_iIndex;
extern CModelInstance *_yy_mi;

void SMCPushBuffer(const char *strName, const char *strBuffer, BOOL bParserEnd);
BOOL SMCPopBuffer(void);
const char *SMCGetBufferName(void);
int SMCGetBufferLineNumber(void);
const char *SMCGetBufferContents(void);
int SMCGetBufferStackDepth(void);

void SMCCountOneLine(void);
