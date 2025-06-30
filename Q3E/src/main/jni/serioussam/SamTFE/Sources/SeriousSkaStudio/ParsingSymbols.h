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
extern int yylex(void);
extern void yyerror(const char *strFormat, ...);
extern int yyparse(void);
extern void yyrestart(FILE *f);
extern FILE *yyin;

#define YY_NEVER_INTERACTIVE 1
#define MAX_INCLUDE_DEPTH 32

// temporary values for parsing
extern INDEX _yy_iLine;
extern CMesh *_yy_pMesh;
extern CSkeleton *_yy_pSkeleton;
extern CAnimSet *_yy_pAnimSet;
extern INDEX _yy_iIndex;
extern INDEX _yy_jIndex;
extern INDEX _yy_iWIndex; // index for weightmaps
extern INDEX _yy_iMIndex; // index for mophmaps
