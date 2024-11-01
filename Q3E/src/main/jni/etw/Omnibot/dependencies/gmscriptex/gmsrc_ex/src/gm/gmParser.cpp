
/*  A Bison parser, made from gmparser.y with Bison version GNU Bison version 1.24
  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse gmparse
#define yylex gmlex
#define yyerror gmerror
#define yylval gmlval
#define yychar gmchar
#define yydebug gmdebug
#define yynerrs gmnerrs
#define	KEYWORD_LOCAL	258
#define	KEYWORD_GLOBAL	259
#define	KEYWORD_MEMBER	260
#define	KEYWORD_AND	261
#define	KEYWORD_OR	262
#define	KEYWORD_IF	263
#define	KEYWORD_ELSE	264
#define	KEYWORD_WHILE	265
#define	KEYWORD_FOR	266
#define	KEYWORD_FOREACH	267
#define	KEYWORD_IN	268
#define	KEYWORD_BREAK	269
#define	KEYWORD_CONTINUE	270
#define	KEYWORD_NULL	271
#define	KEYWORD_DOWHILE	272
#define	KEYWORD_RETURN	273
#define	KEYWORD_FUNCTION	274
#define	KEYWORD_TABLE	275
#define	KEYWORD_THIS	276
#define	KEYWORD_TRUE	277
#define	KEYWORD_FALSE	278
#define	KEYWORD_FORK	279
#define	KEYWORD_SWITCH	280
#define	KEYWORD_CASE	281
#define	KEYWORD_DEFAULT	282
#define	IDENTIFIER	283
#define	CONSTANT_HEX	284
#define	CONSTANT_BINARY	285
#define	CONSTANT_INT	286
#define	CONSTANT_CHAR	287
#define	CONSTANT_FLOAT	288
#define	CONSTANT_STRING	289
#define	SYMBOL_ASGN_BSR	290
#define	SYMBOL_ASGN_BSL	291
#define	SYMBOL_ASGN_ADD	292
#define	SYMBOL_ASGN_MINUS	293
#define	SYMBOL_ASGN_TIMES	294
#define	SYMBOL_ASGN_DIVIDE	295
#define	SYMBOL_ASGN_REM	296
#define	SYMBOL_ASGN_BAND	297
#define	SYMBOL_ASGN_BOR	298
#define	SYMBOL_ASGN_BXOR	299
#define	SYMBOL_RIGHT_SHIFT	300
#define	SYMBOL_LEFT_SHIFT	301
#define	SYMBOL_LTE	302
#define	SYMBOL_GTE	303
#define	SYMBOL_EQ	304
#define	SYMBOL_NEQ	305
#define	TOKEN_ERROR	306



#define YYPARSER
#include "gmConfig.h"
#include "gmCodeTree.h"
#define YYSTYPE gmCodeTreeNode *

extern gmCodeTreeNode * g_codeTree;

#define GM_BISON_DEBUG
#ifdef GM_BISON_DEBUG
#define YYDEBUG 1
#define YYERROR_VERBOSE
#endif // GM_BISON_DEBUG

//
// HELPERS
//

void ATTACH(gmCodeTreeNode * &a_res, gmCodeTreeNode * a_a, gmCodeTreeNode * a_b)
{
  YYSTYPE t = a_a;
  if(t != NULL)
  {
    while(t->m_sibling != NULL)
    {
      t = t->m_sibling;
    }
    t->m_sibling = a_b;
    if(a_b) { a_b->m_parent = t; }
    a_res = a_a;
  }
  else
  {
    a_res = a_b;
  }
}

gmCodeTreeNode * CreateOperation(int a_subTypeType, gmCodeTreeNode * a_left = NULL, gmCodeTreeNode * a_right = NULL)
{
  gmCodeTreeNode * node = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_OPERATION, gmlineno, a_subTypeType);
  node->SetChild(0, a_left);
  node->SetChild(1, a_right);
  return node;
}

gmCodeTreeNode * CreateAsignExpression(int a_subTypeType, gmCodeTreeNode * a_left, gmCodeTreeNode * a_right)
{
  // we need to evaluate the complexety of the l-value... if it is a function call, index or dot to the left of a dot or index, we need to cache
  // into a hidden variable.

  // todo

  gmCodeTreeNode * opNode = CreateOperation(a_subTypeType, a_left, a_right);
  return CreateOperation(CTNOT_ASSIGN, a_left, opNode);
}


#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#ifndef YYSTYPE
#define YYSTYPE int
#endif
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		265
#define	YYFLAG		-32768
#define	YYNTBASE	75

#define YYTRANSLATE(x) ((unsigned)(x) <= 306 ? yytranslate[x] : 117)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    73,     2,     2,     2,    71,    64,     2,    56,
    57,    69,    67,    74,    68,    61,    70,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    58,    54,    65,
    55,    66,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    59,     2,    60,    63,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    52,    62,    53,    72,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     4,     7,     9,    11,    13,    15,    17,    20,
    24,    28,    34,    36,    38,    40,    42,    45,    51,    59,
    67,    71,    74,    82,    86,    91,    95,    97,   100,   102,
   107,   111,   113,   116,   118,   120,   126,   132,   139,   147,
   155,   165,   168,   171,   174,   178,   180,   184,   188,   192,
   196,   200,   204,   208,   212,   216,   220,   224,   226,   229,
   231,   233,   237,   239,   243,   245,   249,   251,   255,   257,
   261,   263,   267,   271,   273,   277,   281,   285,   289,   291,
   295,   299,   301,   305,   309,   311,   315,   319,   323,   325,
   328,   330,   332,   334,   336,   338,   343,   347,   352,   358,
   365,   369,   371,   375,   379,   384,   387,   391,   396,   402,
   407,   409,   413,   415,   419,   425,   427,   429,   433,   435,
   439,   441,   444,   446,   448,   450,   452,   456,   458,   460,
   462,   464,   466,   468,   470,   472,   474,   476,   478
};

static const short yyrhs[] = {    76,
     0,    77,     0,    76,    77,     0,    81,     0,    79,     0,
    82,     0,    87,     0,    88,     0,    52,    53,     0,    52,
    76,    53,     0,    80,   114,    54,     0,    80,   114,    55,
    91,    54,     0,     3,     0,     4,     0,     5,     0,    54,
     0,    89,    54,     0,     8,    56,    91,    57,    78,     0,
     8,    56,    91,    57,    78,     9,    78,     0,     8,    56,
    91,    57,    78,     9,    82,     0,    24,   114,    78,     0,
    24,    78,     0,    25,    56,    91,    57,    52,    84,    53,
     0,    26,    85,    58,     0,    26,    85,    58,    78,     0,
    27,    58,    78,     0,    83,     0,    84,    83,     0,    86,
     0,    85,    59,    91,    60,     0,    85,    61,   114,     0,
   114,     0,    61,   114,     0,    21,     0,   115,     0,    10,
    56,    91,    57,    78,     0,    17,    56,    91,    57,    78,
     0,    11,    56,    81,    90,    57,    78,     0,    11,    56,
    81,    90,    89,    57,    78,     0,    12,    56,   114,    13,
    91,    57,    78,     0,    12,    56,   114,     6,   114,    13,
    91,    57,    78,     0,    15,    54,     0,    14,    54,     0,
    18,    54,     0,    18,    91,    54,     0,    92,     0,   104,
    55,    92,     0,   104,    35,    92,     0,   104,    36,    92,
     0,   104,    37,    92,     0,   104,    38,    92,     0,   104,
    39,    92,     0,   104,    40,    92,     0,   104,    41,    92,
     0,   104,    42,    92,     0,   104,    43,    92,     0,   104,
    44,    92,     0,    54,     0,    91,    54,     0,    92,     0,
    93,     0,    92,     7,    93,     0,    94,     0,    93,     6,
    94,     0,    95,     0,    94,    62,    95,     0,    96,     0,
    95,    63,    96,     0,    97,     0,    96,    64,    97,     0,
    98,     0,    97,    49,    98,     0,    97,    50,    98,     0,
    99,     0,    98,    65,    99,     0,    98,    66,    99,     0,
    98,    47,    99,     0,    98,    48,    99,     0,   100,     0,
    99,    46,   100,     0,    99,    45,   100,     0,   101,     0,
   100,    67,   101,     0,   100,    68,   101,     0,   102,     0,
   101,    69,   102,     0,   101,    70,   102,     0,   101,    71,
   102,     0,   104,     0,   103,   102,     0,    67,     0,    68,
     0,    72,     0,    73,     0,   113,     0,   104,    59,    91,
    60,     0,   104,    56,    57,     0,   104,    56,   105,    57,
     0,   104,    58,   114,    56,    57,     0,   104,    58,   114,
    56,   105,    57,     0,   104,    61,   114,     0,    91,     0,
   105,    74,    91,     0,    20,    56,    57,     0,    20,    56,
   108,    57,     0,    52,    53,     0,    52,   108,    53,     0,
    52,   108,    74,    53,     0,    19,    56,   111,    57,    78,
     0,    19,    56,    57,    78,     0,   109,     0,   108,    74,
   109,     0,    91,     0,   114,    55,    91,     0,    59,   110,
    60,    55,    91,     0,    31,     0,   112,     0,   111,    74,
   112,     0,   114,     0,   114,    55,    91,     0,   114,     0,
    61,   114,     0,    21,     0,   115,     0,   106,     0,   107,
     0,    56,    91,    57,     0,    28,     0,    29,     0,    30,
     0,    31,     0,    22,     0,    23,     0,    32,     0,    33,
     0,   116,     0,    16,     0,    34,     0,   116,    34,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   126,   133,   137,   144,   148,   152,   156,   160,   167,   171,
   179,   184,   193,   197,   201,   207,   211,   218,   224,   231,
   238,   244,   249,   258,   264,   271,   279,   283,   290,   294,
   298,   305,   309,   314,   318,   327,   333,   339,   346,   354,
   361,   372,   376,   380,   384,   392,   400,   404,   408,   412,
   416,   420,   424,   428,   432,   436,   440,   448,   452,   459,
   466,   470,   477,   481,   488,   492,   500,   504,   512,   516,
   524,   528,   532,   539,   543,   547,   551,   555,   562,   566,
   571,   579,   583,   588,   596,   600,   605,   610,   618,   622,
   631,   635,   639,   643,   650,   654,   658,   663,   669,   675,
   682,   689,   693,   700,   704,   709,   713,   718,   726,   732,
   740,   744,   751,   755,   759,   766,   775,   779,   786,   791,
   800,   804,   809,   813,   817,   821,   825,   832,   841,   846,
   851,   856,   861,   866,   912,   917,   921,   929,   943
};

static const char * const yytname[] = {   "$","error","$undefined.","KEYWORD_LOCAL",
"KEYWORD_GLOBAL","KEYWORD_MEMBER","KEYWORD_AND","KEYWORD_OR","KEYWORD_IF","KEYWORD_ELSE",
"KEYWORD_WHILE","KEYWORD_FOR","KEYWORD_FOREACH","KEYWORD_IN","KEYWORD_BREAK",
"KEYWORD_CONTINUE","KEYWORD_NULL","KEYWORD_DOWHILE","KEYWORD_RETURN","KEYWORD_FUNCTION",
"KEYWORD_TABLE","KEYWORD_THIS","KEYWORD_TRUE","KEYWORD_FALSE","KEYWORD_FORK",
"KEYWORD_SWITCH","KEYWORD_CASE","KEYWORD_DEFAULT","IDENTIFIER","CONSTANT_HEX",
"CONSTANT_BINARY","CONSTANT_INT","CONSTANT_CHAR","CONSTANT_FLOAT","CONSTANT_STRING",
"SYMBOL_ASGN_BSR","SYMBOL_ASGN_BSL","SYMBOL_ASGN_ADD","SYMBOL_ASGN_MINUS","SYMBOL_ASGN_TIMES",
"SYMBOL_ASGN_DIVIDE","SYMBOL_ASGN_REM","SYMBOL_ASGN_BAND","SYMBOL_ASGN_BOR",
"SYMBOL_ASGN_BXOR","SYMBOL_RIGHT_SHIFT","SYMBOL_LEFT_SHIFT","SYMBOL_LTE","SYMBOL_GTE",
"SYMBOL_EQ","SYMBOL_NEQ","TOKEN_ERROR","'{'","'}'","';'","'='","'('","')'","':'",
"'['","']'","'.'","'|'","'^'","'&'","'<'","'>'","'+'","'-'","'*'","'/'","'%'",
"'~'","'!'","','","program","statement_list","statement","compound_statement",
"var_statement","var_type","expression_statement","selection_statement","case_selection_statement",
"case_selection_statement_list","postfix_case_expression","case_expression",
"iteration_statement","jump_statement","assignment_expression","constant_expression_statement",
"constant_expression","logical_or_expression","logical_and_expression","inclusive_or_expression",
"exclusive_or_expression","and_expression","equality_expression","relational_expression",
"shift_expression","additive_expression","multiplicative_expression","unary_expression",
"unary_operator","postfix_expression","argument_expression_list","table_constructor",
"function_constructor","field_list","field","constant_field_index","parameter_list",
"parameter","primary_expression","identifier","constant","constant_string_list",
""
};
#endif

static const short yyr1[] = {     0,
    75,    76,    76,    77,    77,    77,    77,    77,    78,    78,
    79,    79,    80,    80,    80,    81,    81,    82,    82,    82,
    82,    82,    82,    83,    83,    83,    84,    84,    85,    85,
    85,    86,    86,    86,    86,    87,    87,    87,    87,    87,
    87,    88,    88,    88,    88,    89,    89,    89,    89,    89,
    89,    89,    89,    89,    89,    89,    89,    90,    90,    91,
    92,    92,    93,    93,    94,    94,    95,    95,    96,    96,
    97,    97,    97,    98,    98,    98,    98,    98,    99,    99,
    99,   100,   100,   100,   101,   101,   101,   101,   102,   102,
   103,   103,   103,   103,   104,   104,   104,   104,   104,   104,
   104,   105,   105,   106,   106,   106,   106,   106,   107,   107,
   108,   108,   109,   109,   109,   110,   111,   111,   112,   112,
   113,   113,   113,   113,   113,   113,   113,   114,   115,   115,
   115,   115,   115,   115,   115,   115,   115,   116,   116
};

static const short yyr2[] = {     0,
     1,     1,     2,     1,     1,     1,     1,     1,     2,     3,
     3,     5,     1,     1,     1,     1,     2,     5,     7,     7,
     3,     2,     7,     3,     4,     3,     1,     2,     1,     4,
     3,     1,     2,     1,     1,     5,     5,     6,     7,     7,
     9,     2,     2,     2,     3,     1,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     1,     2,     1,
     1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
     1,     3,     3,     1,     3,     3,     3,     3,     1,     3,
     3,     1,     3,     3,     1,     3,     3,     3,     1,     2,
     1,     1,     1,     1,     1,     4,     3,     4,     5,     6,
     3,     1,     3,     3,     4,     2,     3,     4,     5,     4,
     1,     3,     1,     3,     5,     1,     1,     3,     1,     3,
     1,     2,     1,     1,     1,     1,     3,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     2
};

static const short yydefact[] = {     0,
    13,    14,    15,     0,     0,     0,     0,     0,     0,   137,
     0,     0,     0,     0,   123,   132,   133,     0,     0,   128,
   129,   130,   131,   134,   135,   138,     0,    16,     0,     0,
    91,    92,    93,    94,     1,     2,     5,     0,     4,     6,
     7,     8,     0,    46,    61,    63,    65,    67,    69,    71,
    74,    79,    82,    85,     0,    89,   125,   126,    95,   121,
   124,   136,     0,     0,     0,     0,    43,    42,     0,    44,
     0,    60,    89,     0,     0,     0,    22,     0,     0,   106,
     0,   113,     0,   111,   121,     0,   122,     3,     0,    17,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    90,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,   139,     0,     0,     0,     0,     0,
    45,     0,     0,   117,   119,   104,     0,     9,     0,    21,
     0,   116,     0,   107,     0,     0,   127,    11,     0,    62,
    64,    66,    68,    70,    72,    73,    77,    78,    75,    76,
    81,    80,    83,    84,    86,    87,    88,    48,    49,    50,
    51,    52,    53,    54,    55,    56,    57,    47,    97,   102,
     0,     0,     0,   101,     0,     0,    58,     0,     0,     0,
     0,     0,   110,     0,     0,     0,   105,     0,    10,     0,
     0,   108,   112,   114,     0,    98,     0,     0,    96,    18,
    36,     0,     0,    59,     0,     0,    37,   109,   118,   120,
     0,     0,    12,   103,    99,     0,     0,    38,     0,     0,
     0,     0,     0,    27,     0,   115,   100,    19,    20,    39,
     0,    40,    34,     0,     0,    29,    32,    35,     0,    23,
    28,     0,    33,    24,     0,     0,    26,    41,    25,     0,
    31,    30,     0,     0,     0
};

static const short yydefgoto[] = {   263,
    35,    36,    77,    37,    38,    39,    40,   234,   235,   245,
   246,    41,    42,    43,   188,    82,    72,    45,    46,    47,
    48,    49,    50,    51,    52,    53,    54,    55,    73,   181,
    57,    58,    83,    84,   143,   133,   134,    59,    60,    61,
    62
};

static const short yypact[] = {   374,
-32768,-32768,-32768,   -52,   -47,   -41,    -6,    11,    31,-32768,
    16,   594,    45,    52,-32768,-32768,-32768,   -16,    54,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,   429,-32768,   979,    12,
-32768,-32768,-32768,-32768,   374,-32768,-32768,    12,-32768,-32768,
-32768,-32768,    60,    37,   109,    56,    68,    69,    42,   -24,
    48,    28,    18,-32768,   979,   246,-32768,-32768,-32768,-32768,
-32768,    98,   979,   979,   649,    12,-32768,-32768,   979,-32768,
    81,    37,    15,   -22,   484,   242,-32768,    84,   979,-32768,
   107,-32768,   -37,-32768,    86,    82,-32768,-32768,    43,-32768,
   979,   979,   979,   979,   979,   979,   979,   979,   979,   979,
   979,   979,   979,   979,   979,   979,   979,   979,-32768,   979,
   979,   979,   979,   979,   979,   979,   979,   979,   979,   979,
   704,    12,   979,    12,-32768,    87,    88,   759,     5,    89,
-32768,    84,   -18,-32768,    92,-32768,   -14,-32768,   308,-32768,
    91,-32768,    83,-32768,   539,   979,-32768,-32768,   979,   109,
    56,    68,    69,    42,   -24,   -24,    48,    48,    48,    48,
    28,    28,    18,    18,-32768,-32768,-32768,    37,    37,    37,
    37,    37,    37,    37,    37,    37,    37,    37,-32768,-32768,
   -10,    94,    93,-32768,    84,    84,-32768,   814,    97,    12,
   979,    84,-32768,    84,    12,   979,-32768,   869,-32768,   100,
    99,-32768,-32768,-32768,   103,-32768,   979,   924,-32768,   149,
-32768,    84,   102,-32768,   147,   104,-32768,-32768,-32768,-32768,
    73,   979,-32768,-32768,-32768,    -8,    38,-32768,    84,   979,
    84,    -2,   105,-32768,   -19,-32768,-32768,-32768,-32768,-32768,
   108,-32768,-32768,    12,    19,-32768,-32768,-32768,    84,-32768,
-32768,    84,-32768,    84,   979,    12,-32768,-32768,-32768,   106,
-32768,-32768,   167,   168,-32768
};

static const short yypgoto[] = {-32768,
    95,   -34,   -30,-32768,-32768,   110,   -58,   -65,-32768,-32768,
-32768,-32768,-32768,   -15,-32768,   -12,    10,    85,    80,    90,
   111,    79,     7,   -17,     4,     8,   -53,-32768,     3,   -31,
-32768,-32768,   112,  -140,-32768,-32768,    -9,-32768,    -5,   -54,
-32768
};


#define	YYLAST		1052


static const short yytable[] = {    71,
    88,   109,    56,    63,   203,    20,   232,   233,    64,    44,
   190,    20,    78,    10,    65,   144,    86,   191,   243,    16,
    17,    85,    98,    99,    87,    20,    21,    22,    23,    24,
    25,    26,    89,   250,   132,    76,   145,    56,   194,    20,
   100,   101,   197,    91,    44,     4,   206,   140,   237,    66,
   126,   127,   165,   166,   167,   195,   130,   203,   244,   198,
   129,    18,    19,   207,    67,   207,   141,    56,   135,    85,
   121,    69,   122,   123,    44,   124,   254,   255,    56,   256,
   157,   158,   159,   160,    68,    44,   106,   107,   108,    76,
    96,    97,   102,   103,   104,   105,   148,   149,   232,   233,
    74,   193,   155,   156,    88,   161,   162,    75,   180,    79,
   183,   163,   164,    90,    92,   189,   182,    93,   184,   168,
   169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
    94,   125,    95,   204,   131,    76,   205,   142,   147,    85,
   146,    56,   201,   185,   186,   192,   196,   200,    44,   208,
   214,   221,   209,   222,   210,   211,   223,   227,   229,   230,
   231,   217,   249,   218,   252,   262,   264,   265,   239,   251,
   139,   151,   213,   154,   128,   150,   226,   248,   216,     0,
     0,   228,   152,   220,   215,   219,   137,     0,     0,   135,
    56,     0,    85,     0,   224,   180,   238,    44,   240,     0,
   242,     0,     0,     0,   153,     0,     0,     0,     0,   236,
     0,     0,     0,     0,     0,     0,     0,   241,   257,     0,
     0,   258,     0,   259,     0,     0,   247,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,   253,     0,
     0,     0,   260,     0,     1,     2,     3,     0,     0,     4,
   261,     5,     6,     7,     0,     8,     9,    10,    11,    12,
    13,    14,    15,    16,    17,    18,    19,     0,     0,    20,
    21,    22,    23,    24,    25,    26,     0,     0,     0,     0,
   110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     0,     0,     0,    27,   138,    28,     0,    29,     0,     0,
   120,   121,    30,   122,   123,     0,   124,     0,    31,    32,
     1,     2,     3,    33,    34,     4,     0,     5,     6,     7,
     0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,     0,     0,    20,    21,    22,    23,    24,
    25,    26,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    27,
   199,    28,     0,    29,     0,     0,     0,     0,    30,     0,
     0,     0,     0,     0,    31,    32,     1,     2,     3,    33,
    34,     4,     0,     5,     6,     7,     0,     8,     9,    10,
    11,    12,    13,    14,    15,    16,    17,    18,    19,     0,
     0,    20,    21,    22,    23,    24,    25,    26,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    27,     0,    28,     0,    29,
     0,     0,     0,     0,    30,     0,     0,     0,     0,     0,
    31,    32,     0,     0,    10,    33,    34,    13,    14,    15,
    16,    17,     0,     0,     0,     0,    20,    21,    22,    23,
    24,    25,    26,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    27,    80,     0,     0,    29,     0,     0,    81,     0,    30,
     0,     0,     0,     0,     0,    31,    32,     0,     0,    10,
    33,    34,    13,    14,    15,    16,    17,     0,     0,     0,
     0,    20,    21,    22,    23,    24,    25,    26,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    27,     0,     0,     0,    29,
   136,     0,    81,     0,    30,     0,     0,     0,     0,     0,
    31,    32,     0,     0,    10,    33,    34,    13,    14,    15,
    16,    17,     0,     0,     0,     0,    20,    21,    22,    23,
    24,    25,    26,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    27,   202,     0,     0,    29,     0,     0,    81,     0,    30,
     0,     0,     0,     0,     0,    31,    32,     0,     0,    10,
    33,    34,    13,    14,    15,    16,    17,     0,     0,     0,
     0,    20,    21,    22,    23,    24,    25,    26,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    27,     0,    70,     0,    29,
     0,     0,     0,     0,    30,     0,     0,     0,     0,     0,
    31,    32,     0,     0,    10,    33,    34,    13,    14,    15,
    16,    17,     0,     0,     0,     0,    20,    21,    22,    23,
    24,    25,    26,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    27,     0,    28,     0,    29,     0,     0,     0,     0,    30,
     0,     0,     0,     0,     0,    31,    32,     0,     0,    10,
    33,    34,    13,    14,    15,    16,    17,     0,     0,     0,
     0,    20,    21,    22,    23,    24,    25,    26,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    27,     0,     0,     0,    29,
   179,     0,     0,     0,    30,     0,     0,     0,     0,     0,
    31,    32,     0,     0,    10,    33,    34,    13,    14,    15,
    16,    17,     0,     0,     0,     0,    20,    21,    22,    23,
    24,    25,    26,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    27,     0,   187,     0,    29,     0,     0,     0,     0,    30,
     0,     0,     0,     0,     0,    31,    32,     0,     0,    10,
    33,    34,    13,    14,    15,    16,    17,     0,     0,     0,
     0,    20,    21,    22,    23,    24,    25,    26,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    27,     0,     0,     0,    29,
   212,     0,     0,     0,    30,     0,     0,     0,     0,     0,
    31,    32,     0,     0,    10,    33,    34,    13,    14,    15,
    16,    17,     0,     0,     0,     0,    20,    21,    22,    23,
    24,    25,    26,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    27,     0,     0,     0,    29,     0,     0,    81,     0,    30,
     0,     0,     0,     0,     0,    31,    32,     0,     0,    10,
    33,    34,    13,    14,    15,    16,    17,     0,     0,     0,
     0,    20,    21,    22,    23,    24,    25,    26,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    27,     0,     0,     0,    29,
   225,     0,     0,     0,    30,     0,     0,     0,     0,     0,
    31,    32,     0,     0,    10,    33,    34,    13,    14,    15,
    16,    17,     0,     0,     0,     0,    20,    21,    22,    23,
    24,    25,    26,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    27,     0,     0,     0,    29,     0,     0,     0,     0,    30,
     0,     0,     0,     0,     0,    31,    32,     0,     0,     0,
    33,    34
};

static const short yycheck[] = {    12,
    35,    55,     0,    56,   145,    28,    26,    27,    56,     0,
     6,    28,    18,    16,    56,    53,    29,    13,    21,    22,
    23,    27,    47,    48,    30,    28,    29,    30,    31,    32,
    33,    34,    38,    53,    57,    52,    74,    35,    57,    28,
    65,    66,    57,     7,    35,     8,    57,    78,    57,    56,
    63,    64,   106,   107,   108,    74,    69,   198,    61,    74,
    66,    24,    25,    74,    54,    74,    79,    65,    74,    75,
    56,    56,    58,    59,    65,    61,    58,    59,    76,    61,
    98,    99,   100,   101,    54,    76,    69,    70,    71,    52,
    49,    50,    45,    46,    67,    68,    54,    55,    26,    27,
    56,   132,    96,    97,   139,   102,   103,    56,   121,    56,
   123,   104,   105,    54,     6,   128,   122,    62,   124,   110,
   111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
    63,    34,    64,   146,    54,    52,   149,    31,    57,   145,
    55,   139,    60,    57,    57,    57,    55,    57,   139,    56,
    54,    52,    60,    55,   185,   186,    54,     9,    57,    13,
    57,   192,    58,   194,    57,    60,     0,     0,   227,   235,
    76,    92,   188,    95,    65,    91,   208,   232,   191,    -1,
    -1,   212,    93,   196,   190,   195,    75,    -1,    -1,   195,
   188,    -1,   198,    -1,   207,   208,   227,   188,   229,    -1,
   231,    -1,    -1,    -1,    94,    -1,    -1,    -1,    -1,   222,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,   230,   249,    -1,
    -1,   252,    -1,   254,    -1,    -1,   232,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   244,    -1,
    -1,    -1,   255,    -1,     3,     4,     5,    -1,    -1,     8,
   256,    10,    11,    12,    -1,    14,    15,    16,    17,    18,
    19,    20,    21,    22,    23,    24,    25,    -1,    -1,    28,
    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,
    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
    -1,    -1,    -1,    52,    53,    54,    -1,    56,    -1,    -1,
    55,    56,    61,    58,    59,    -1,    61,    -1,    67,    68,
     3,     4,     5,    72,    73,     8,    -1,    10,    11,    12,
    -1,    14,    15,    16,    17,    18,    19,    20,    21,    22,
    23,    24,    25,    -1,    -1,    28,    29,    30,    31,    32,
    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
    53,    54,    -1,    56,    -1,    -1,    -1,    -1,    61,    -1,
    -1,    -1,    -1,    -1,    67,    68,     3,     4,     5,    72,
    73,     8,    -1,    10,    11,    12,    -1,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    -1,
    -1,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    52,    -1,    54,    -1,    56,
    -1,    -1,    -1,    -1,    61,    -1,    -1,    -1,    -1,    -1,
    67,    68,    -1,    -1,    16,    72,    73,    19,    20,    21,
    22,    23,    -1,    -1,    -1,    -1,    28,    29,    30,    31,
    32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    52,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
    -1,    -1,    -1,    -1,    -1,    67,    68,    -1,    -1,    16,
    72,    73,    19,    20,    21,    22,    23,    -1,    -1,    -1,
    -1,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    52,    -1,    -1,    -1,    56,
    57,    -1,    59,    -1,    61,    -1,    -1,    -1,    -1,    -1,
    67,    68,    -1,    -1,    16,    72,    73,    19,    20,    21,
    22,    23,    -1,    -1,    -1,    -1,    28,    29,    30,    31,
    32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    52,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
    -1,    -1,    -1,    -1,    -1,    67,    68,    -1,    -1,    16,
    72,    73,    19,    20,    21,    22,    23,    -1,    -1,    -1,
    -1,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    52,    -1,    54,    -1,    56,
    -1,    -1,    -1,    -1,    61,    -1,    -1,    -1,    -1,    -1,
    67,    68,    -1,    -1,    16,    72,    73,    19,    20,    21,
    22,    23,    -1,    -1,    -1,    -1,    28,    29,    30,    31,
    32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    52,    -1,    54,    -1,    56,    -1,    -1,    -1,    -1,    61,
    -1,    -1,    -1,    -1,    -1,    67,    68,    -1,    -1,    16,
    72,    73,    19,    20,    21,    22,    23,    -1,    -1,    -1,
    -1,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    52,    -1,    -1,    -1,    56,
    57,    -1,    -1,    -1,    61,    -1,    -1,    -1,    -1,    -1,
    67,    68,    -1,    -1,    16,    72,    73,    19,    20,    21,
    22,    23,    -1,    -1,    -1,    -1,    28,    29,    30,    31,
    32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    52,    -1,    54,    -1,    56,    -1,    -1,    -1,    -1,    61,
    -1,    -1,    -1,    -1,    -1,    67,    68,    -1,    -1,    16,
    72,    73,    19,    20,    21,    22,    23,    -1,    -1,    -1,
    -1,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    52,    -1,    -1,    -1,    56,
    57,    -1,    -1,    -1,    61,    -1,    -1,    -1,    -1,    -1,
    67,    68,    -1,    -1,    16,    72,    73,    19,    20,    21,
    22,    23,    -1,    -1,    -1,    -1,    28,    29,    30,    31,
    32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    52,    -1,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
    -1,    -1,    -1,    -1,    -1,    67,    68,    -1,    -1,    16,
    72,    73,    19,    20,    21,    22,    23,    -1,    -1,    -1,
    -1,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    52,    -1,    -1,    -1,    56,
    57,    -1,    -1,    -1,    61,    -1,    -1,    -1,    -1,    -1,
    67,    68,    -1,    -1,    16,    72,    73,    19,    20,    21,
    22,    23,    -1,    -1,    -1,    -1,    28,    29,    30,    31,
    32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    52,    -1,    -1,    -1,    56,    -1,    -1,    -1,    -1,    61,
    -1,    -1,    -1,    -1,    -1,    67,    68,    -1,    -1,    -1,
    72,    73
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */


/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         -2
#define YYEOF           0
#define YYACCEPT        return(0)
#define YYABORT         return(1)
#define YYERROR         goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL          goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do                                                              \
  if (yychar == YYEMPTY && yylen == 1)                          \
    { yychar = (token), yylval = (value);                       \
      yychar1 = YYTRANSLATE (yychar);                           \
      YYPOPSTACK;                                               \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    { yyerror ("syntax error: cannot back up"); YYERROR; }      \
while (0)

#define YYTERROR        1
#define YYERRCODE       256

#ifndef YYPURE
#define YYLEX           yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX           yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX           yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX           yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX           yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int     yychar;                 /*  the lookahead symbol                */
YYSTYPE yylval;                 /*  the semantic value of the           */
                                /*  lookahead symbol                    */

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;                 /*  location data for the lookahead     */
                                /*  symbol                              */
#endif

int yynerrs;                    /*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;                    /*  nonzero means print parse trace     */
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks       */

#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1                /* GNU C and GNU C++ define this.  */
#define __yy_memcpy(FROM,TO,COUNT)      __builtin_memcpy(TO,FROM,COUNT)
#else                           /* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif



/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#else
#define YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#endif

int
yyparse(YYPARSE_PARAM)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;      /*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;              /*  lookahead token as an internal (translated) token number */

  short yyssa[YYINITDEPTH];     /*  the state stack                     */
  YYSTYPE yyvsa[YYINITDEPTH];   /*  the semantic value stack            */

  short *yyss = yyssa;          /*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;        /*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];   /*  the location stack                  */
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;                /*  the variable used to return         */
                                /*  semantic values from the action     */
                                /*  routines                            */

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;             /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = (short) yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size =(int)( yyssp - yyss + 1); // _GD_ cast for 64bit build

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
         the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
         but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
                 &yyss1, size * sizeof (*yyssp),
                 &yyvs1, size * sizeof (*yyvsp),
                 &yyls1, size * sizeof (*yylsp),
                 &yystacksize);
#else
      yyoverflow("parser stack overflow",
                 &yyss1, size * sizeof (*yyssp),
                 &yyvs1, size * sizeof (*yyvsp),
                 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
        {
          yyerror("parser stack overflow");
          return 2;
        }
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
        yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
        fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
        YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
        fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)              /* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;           /* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
        fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
        {
          fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
          /* Give the individual parser a way to print the precise meaning
             of a token, for further debugging info.  */
#ifdef YYPRINT
          YYPRINT (stderr, yychar, yylval);
#endif
          fprintf (stderr, ")\n");
        }
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
               yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
        fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
{
      g_codeTree = yyvsp[0];
    ;
    break;}
case 2:
{
      yyval = yyvsp[0];
    ;
    break;}
case 3:
{
      ATTACH(yyval, yyvsp[-1], yyvsp[0]);
    ;
    break;}
case 4:
{
      yyval = yyvsp[0];
    ;
    break;}
case 5:
{
      yyval = yyvsp[0];
    ;
    break;}
case 6:
{
      yyval = yyvsp[0];
    ;
    break;}
case 7:
{
      yyval = yyvsp[0];
    ;
    break;}
case 8:
{
      yyval = yyvsp[0];
    ;
    break;}
case 9:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_COMPOUND, gmlineno);
    ;
    break;}
case 10:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_COMPOUND, gmlineno);
      yyval->SetChild(0, yyvsp[-1]);
    ;
    break;}
case 11:
{
      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_VARIABLE, gmlineno, (gmptr) yyvsp[-2]);
      yyval->SetChild(0, yyvsp[-1]);
    ;
    break;}
case 12:
{
      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_VARIABLE, gmlineno, (gmptr) yyvsp[-4]);
      yyval->SetChild(0, yyvsp[-3]);
      ATTACH(yyval, yyval, CreateOperation(CTNOT_ASSIGN, yyvsp[-3], yyvsp[-1]));
    ;
    break;}
case 13:
{
      yyval = (YYSTYPE) CTVT_LOCAL;
    ;
    break;}
case 14:
{
      yyval = (YYSTYPE) CTVT_GLOBAL;
    ;
    break;}
case 15:
{
      yyval = (YYSTYPE) CTVT_MEMBER;
    ;
    break;}
case 16:
{
      yyval = NULL;
    ;
    break;}
case 17:
{
      yyval = yyvsp[-1];
    ;
    break;}
case 18:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_IF, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    ;
    break;}
case 19:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_IF, (yyvsp[-4]) ? yyvsp[-4]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-4]);
      yyval->SetChild(1, yyvsp[-2]);
      yyval->SetChild(2, yyvsp[0]);
    ;
    break;}
case 20:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_IF, (yyvsp[-4]) ? yyvsp[-4]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-4]);
      yyval->SetChild(1, yyvsp[-2]);
      yyval->SetChild(2, yyvsp[0]);
    ;
    break;}
case 21:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FORK, (yyvsp[-1]) ? yyvsp[-1]->m_lineNumber : gmlineno );
      yyval->SetChild(0, yyvsp[0] );
      yyval->SetChild(1, yyvsp[-1] );
    ;
    break;}
case 22:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FORK, (yyvsp[0]) ? yyvsp[0]->m_lineNumber : gmlineno );
      yyval->SetChild(0, yyvsp[0] );
    ;
    break;}
case 23:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_SWITCH, (yyvsp[-4]) ? yyvsp[-4]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-4]);
      yyval->SetChild(1, yyvsp[-1]);
    ;
    break;}
case 24:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_CASE, (yyvsp[-1]) ? yyvsp[-1]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-1]);
    ;
    break;}
case 25:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_CASE, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    ;
    break;}
case 26:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_DEFAULT, (yyvsp[0]) ? yyvsp[0]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[0]);
    ;
    break;}
case 27:
{
      yyval = yyvsp[0];
    ;
    break;}
case 28:
{
      ATTACH(yyval, yyvsp[-1], yyvsp[0]);
    ;
    break;}
case 29:
{
      yyval = yyvsp[0];
    ;
    break;}
case 30:
{
      yyval = CreateOperation(CTNOT_ARRAY_INDEX, yyvsp[-3], yyvsp[-1]);
    ;
    break;}
case 31:
{
      yyval = CreateOperation(CTNOT_DOT, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 32:
{
      yyval = yyvsp[0];
    ;
    break;}
case 33:
{
      yyval = yyvsp[0];
      yyval->m_flags |= gmCodeTreeNode::CTN_MEMBER;
    ;
    break;}
case 34:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_THIS, gmlineno);
    ;
    break;}
case 35:
{
      yyval = yyvsp[0];
    ;
    break;}
case 36:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_WHILE, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    ;
    break;}
case 37:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_DOWHILE, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    ;
    break;}
case 38:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FOR, (yyvsp[-3]) ? yyvsp[-3]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-3]);
      yyval->SetChild(1, yyvsp[-2]);
      yyval->SetChild(3, yyvsp[0]);
    ;
    break;}
case 39:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FOR, (yyvsp[-4]) ? yyvsp[-4]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-4]);
      yyval->SetChild(1, yyvsp[-3]);
      yyval->SetChild(2, yyvsp[-2]);
      yyval->SetChild(3, yyvsp[0]);
    ;
    break;}
case 40:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FOREACH, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[-4]);
      yyval->SetChild(3, yyvsp[0]);
    ;
    break;}
case 41:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FOREACH, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[-4]);
      yyval->SetChild(2, yyvsp[-6]);
      yyval->SetChild(3, yyvsp[0]);
    ;
    break;}
case 42:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_CONTINUE, gmlineno);
    ;
    break;}
case 43:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_BREAK, gmlineno);
    ;
    break;}
case 44:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_RETURN, gmlineno);
    ;
    break;}
case 45:
{
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_RETURN, gmlineno);
      yyval->SetChild(0, yyvsp[-1]);
    ;
    break;}
case 46:
{
      yyval = yyvsp[0];
      if(yyval)
      {
        yyval->m_flags |= gmCodeTreeNode::CTN_POP;
      }
    ;
    break;}
case 47:
{
      yyval = CreateOperation(CTNOT_ASSIGN, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 48:
{
      yyval = CreateAsignExpression(CTNOT_SHIFT_RIGHT, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 49:
{
      yyval = CreateAsignExpression(CTNOT_SHIFT_LEFT, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 50:
{
      yyval = CreateAsignExpression(CTNOT_ADD, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 51:
{
      yyval = CreateAsignExpression(CTNOT_MINUS, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 52:
{
      yyval = CreateAsignExpression(CTNOT_TIMES, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 53:
{
      yyval = CreateAsignExpression(CTNOT_DIVIDE, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 54:
{
      yyval = CreateAsignExpression(CTNOT_REM, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 55:
{
      yyval = CreateAsignExpression(CTNOT_BIT_AND, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 56:
{
      yyval = CreateAsignExpression(CTNOT_BIT_OR, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 57:
{
      yyval = CreateAsignExpression(CTNOT_BIT_XOR, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 58:
{
      yyval = NULL;
    ;
    break;}
case 59:
{
      yyval = yyvsp[-1];
    ;
    break;}
case 60:
{
      yyval = yyvsp[0];
    ;
    break;}
case 61:
{
      yyval = yyvsp[0];
    ;
    break;}
case 62:
{
      yyval = CreateOperation(CTNOT_OR, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 63:
{
      yyval = yyvsp[0];
    ;
    break;}
case 64:
{
      yyval = CreateOperation(CTNOT_AND, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 65:
{
      yyval = yyvsp[0];
    ;
    break;}
case 66:
{
      yyval = CreateOperation(CTNOT_BIT_OR, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    ;
    break;}
case 67:
{
      yyval = yyvsp[0];
    ;
    break;}
case 68:
{
      yyval = CreateOperation(CTNOT_BIT_XOR, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    ;
    break;}
case 69:
{
      yyval = yyvsp[0];
    ;
    break;}
case 70:
{
      yyval = CreateOperation(CTNOT_BIT_AND, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    ;
    break;}
case 71:
{
      yyval = yyvsp[0];
    ;
    break;}
case 72:
{
      yyval = CreateOperation(CTNOT_EQ, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 73:
{
      yyval = CreateOperation(CTNOT_NEQ, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 74:
{
      yyval = yyvsp[0];
    ;
    break;}
case 75:
{
      yyval = CreateOperation(CTNOT_LT, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 76:
{
      yyval = CreateOperation(CTNOT_GT, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 77:
{
      yyval = CreateOperation(CTNOT_LTE, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 78:
{
      yyval = CreateOperation(CTNOT_GTE, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 79:
{
      yyval = yyvsp[0];
    ;
    break;}
case 80:
{
      yyval = CreateOperation(CTNOT_SHIFT_LEFT, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    ;
    break;}
case 81:
{
      yyval = CreateOperation(CTNOT_SHIFT_RIGHT, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    ;
    break;}
case 82:
{
      yyval = yyvsp[0];
    ;
    break;}
case 83:
{
      yyval = CreateOperation(CTNOT_ADD, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    ;
    break;}
case 84:
{
      yyval = CreateOperation(CTNOT_MINUS, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    ;
    break;}
case 85:
{
      yyval = yyvsp[0];
    ;
    break;}
case 86:
{
      yyval = CreateOperation(CTNOT_TIMES, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    ;
    break;}
case 87:
{
      yyval = CreateOperation(CTNOT_DIVIDE, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    ;
    break;}
case 88:
{
      yyval = CreateOperation(CTNOT_REM, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    ;
    break;}
case 89:
{
      yyval = yyvsp[0];
    ;
    break;}
case 90:
{
      yyval = yyvsp[-1];
      yyval->SetChild(0, yyvsp[0]);
      yyval->ConstantFold();
    ;
    break;}
case 91:
{
      yyval = CreateOperation(CTNOT_UNARY_PLUS);
    ;
    break;}
case 92:
{
      yyval = CreateOperation(CTNOT_UNARY_MINUS);
    ;
    break;}
case 93:
{
      yyval = CreateOperation(CTNOT_UNARY_COMPLEMENT);
    ;
    break;}
case 94:
{
      yyval = CreateOperation(CTNOT_UNARY_NOT);
    ;
    break;}
case 95:
{
      yyval = yyvsp[0];
    ;
    break;}
case 96:
{
      yyval = CreateOperation(CTNOT_ARRAY_INDEX, yyvsp[-3], yyvsp[-1]);
    ;
    break;}
case 97:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CALL, gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
    ;
    break;}
case 98:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CALL, gmlineno);
      yyval->SetChild(0, yyvsp[-3]);
      yyval->SetChild(1, yyvsp[-1]);
    ;
    break;}
case 99:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CALL, gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(2, yyvsp[-4]);
    ;
    break;}
case 100:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CALL, gmlineno);
      yyval->SetChild(0, yyvsp[-3]);
      yyval->SetChild(1, yyvsp[-1]);
      yyval->SetChild(2, yyvsp[-5]);
    ;
    break;}
case 101:
{
      yyval = CreateOperation(CTNOT_DOT, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 102:
{
      yyval = yyvsp[0];
    ;
    break;}
case 103:
{
      ATTACH(yyval, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 104:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_TABLE, gmlineno);
    ;
    break;}
case 105:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_TABLE, gmlineno);
      yyval->SetChild(0, yyvsp[-1]);
    ;
    break;}
case 106:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_TABLE, gmlineno);
    ;
    break;}
case 107:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_TABLE, gmlineno);
      yyval->SetChild(0, yyvsp[-1]);
    ;
    break;}
case 108:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_TABLE, gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
    ;
    break;}
case 109:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_FUNCTION, gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    ;
    break;}
case 110:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_FUNCTION, gmlineno);
      yyval->SetChild(1, yyvsp[0]);
    ;
    break;}
case 111:
{
      yyval = yyvsp[0];
    ;
    break;}
case 112:
{
      ATTACH(yyval, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 113:
{
      yyval = yyvsp[0];
    ;
    break;}
case 114:
{
      yyval = CreateOperation(CTNOT_ASSIGN_FIELD, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 115:
{
	  yyval = CreateOperation(CTNOT_ASSIGN_INDEX, yyvsp[-3], yyvsp[0]);
	;
    break;}
case 116:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);
      yyval->m_data.m_iValue = atoi(gmtext);
    ;
    break;}
case 117:
{
      yyval = yyvsp[0];
    ;
    break;}
case 118:
{
      ATTACH(yyval, yyvsp[-2], yyvsp[0]);
    ;
    break;}
case 119:
{
      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_PARAMETER, gmlineno);
      yyval->SetChild(0, yyvsp[0]);
    ;
    break;}
case 120:
{
      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_PARAMETER, gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    ;
    break;}
case 121:
{
      yyval = yyvsp[0];
    ;
    break;}
case 122:
{
      yyval = yyvsp[0];
      yyval->m_flags |= gmCodeTreeNode::CTN_MEMBER;
    ;
    break;}
case 123:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_THIS, gmlineno);
    ;
    break;}
case 124:
{
      yyval = yyvsp[0];
    ;
    break;}
case 125:
{
      yyval = yyvsp[0];
    ;
    break;}
case 126:
{
      yyval = yyvsp[0];
    ;
    break;}
case 127:
{
      yyval = yyvsp[-1];
    ;
    break;}
case 128:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_IDENTIFIER, gmlineno);
      yyval->m_data.m_string = (char *) gmCodeTree::Get().Alloc((int)strlen(gmtext) + 1);
      strcpy(yyval->m_data.m_string, gmtext);
    ;
    break;}
case 129:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);
      yyval->m_data.m_iValue = strtoul(gmtext + 2, NULL, 16);
    ;
    break;}
case 130:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);
      yyval->m_data.m_iValue = strtoul(gmtext + 2, NULL, 2);
    ;
    break;}
case 131:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);
      yyval->m_data.m_iValue = atoi(gmtext);
    ;
    break;}
case 132:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);
      yyval->m_data.m_iValue = 1;
    ;
    break;}
case 133:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);
      yyval->m_data.m_iValue = 0;
    ;
    break;}
case 134:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);

      char * c = (char *) gmCodeTree::Get().Alloc((int)strlen(gmtext) + 1);
      strcpy(c, gmtext);
      int result = 0;
      int shr = 0;

      while(*c)
      {
        if(c[0] == '\'')
        {
          ++c;
          continue;
        }
        else if(c[0] == '\\')
        {
          if(shr) result <<= 8;
          switch(c[1])
          {
            case 'a' : result |= (unsigned char) '\a'; break;
            case 'b' : result |= (unsigned char) '\b'; break;
            case 'f' : result |= (unsigned char) '\f'; break;
            case 'n' : result |= (unsigned char) '\n'; break;
            case 'r' : result |= (unsigned char) '\r'; break;
            case 't' : result |= (unsigned char) '\t'; break;
            case 'v' : result |= (unsigned char) '\v'; break;
            case '\'' : result |= (unsigned char) '\''; break;
            case '\"' : result |= (unsigned char) '\"'; break;
            case '\\' : result |= (unsigned char) '\\'; break;
            default: result |= (unsigned char) c[1];
          }
          ++shr;
          c += 2;
          continue;
        }
        if(shr) result <<= 8;
        result |= (unsigned char) *(c++);
        ++shr;
      }

      if(shr > 4 && gmCodeTree::Get().GetLog()) gmCodeTree::Get().GetLog()->LogEntry("truncated char, line %d", gmlineno);

      yyval->m_data.m_iValue = result;
    ;
    break;}
case 135:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_FLOAT);
      yyval->m_data.m_fValue = (float) atof(gmtext);
    ;
    break;}
case 136:
{
      yyval = yyvsp[0];
    ;
    break;}
case 137:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_NULL);
      yyval->m_data.m_iValue = 0;
    ;
    break;}
case 138:
{
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_STRING);
      yyval->m_data.m_string = (char *) gmCodeTree::Get().Alloc((int)strlen(gmtext) + 1);
      strcpy(yyval->m_data.m_string, gmtext);
      if(gmtext[0] == '"')
      {
        gmProcessDoubleQuoteString(yyval->m_data.m_string);
      }
      else if(gmtext[0] == '`')
      {
        gmProcessSingleQuoteString(yyval->m_data.m_string);
      }
    ;
    break;}
case 139:
{
      yyval = yyvsp[-1];
      int alen = (int)strlen(yyval->m_data.m_string);
      int blen = (int)strlen(gmtext);
      char * str = (char *) gmCodeTree::Get().Alloc(alen + blen + 1);
      if(str)
      {
        memcpy(str, yyvsp[-1]->m_data.m_string, alen);
        memcpy(str + alen, gmtext, blen);
        str[alen + blen] = '\0';
        if(str[alen] == '"')
        {
          gmProcessDoubleQuoteString(str + alen);
        }
        else if(str[alen] == '`')
        {
          gmProcessSingleQuoteString(str + alen);
        }
        yyval->m_data.m_string = str;
      }
    ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */


  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
        fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
        {
          int size = 0;
          char *msg;
          int x, count;

          count = 0;
          /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
          for (x = (yyn < 0 ? -yyn : 0);
               x < (int)(sizeof(yytname) / sizeof(char *)); x++) //_GD_
            if (yycheck[x + yyn] == x)
              size += (int)strlen(yytname[x]) + 15, count++; // _GD_ add cast for 64bit build
          //_GD_ msg = (char *) malloc(size + 15);
          msg = GM_NEW( char [size + 15] );
          if (msg != 0)
            {
              strcpy(msg, "parse error");

              if (count < 5)
                {
                  count = 0;
                  for (x = (yyn < 0 ? -yyn : 0);
                       x < (int)(sizeof(yytname) / sizeof(char *)); x++)
                    if (yycheck[x + yyn] == x)
                      {
                        strcat(msg, count == 0 ? ", expecting `" : " or `");
                        strcat(msg, yytname[x]);
                        strcat(msg, "'");
                        count++;
                      }
                }
              yyerror(msg);
              //_GD_ free(msg);
              delete [] msg;
            }
          else
            yyerror ("parse error; also virtual memory exceeded");
        }
      else
#endif /* YYERROR_VERBOSE */
        yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
        YYABORT;

#if YYDEBUG != 0
      if (yydebug)
        fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;              /* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
        fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
        goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}


#include <stdio.h>











