/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef _MSC_VER
#define CDECL __cdecl
#else
#define CDECL
#endif

/************ Begin %include sections from the grammar ************************/
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
/**************** End makeheaders token definitions ***************************/

/* The next section is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    XlatParseTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal 
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is XlatParseTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    XlatParseARG_SDECL     A static variable declaration for the %extra_argument
**    XlatParseARG_PDECL     A parameter declaration for the %extra_argument
**    XlatParseARG_STORE     Code to store %extra_argument into yypParser
**    XlatParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_MIN_REDUCE      Maximum value for reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned char
#define YYNOCODE 71
#define YYACTIONTYPE unsigned short int
#define XlatParseTOKENTYPE FParseToken
typedef union {
  int yyinit;
  XlatParseTOKENTYPE yy0;
  SpecialArgs yy7;
  MoreFilters * yy8;
  int yy32;
  ParseBoomArg yy63;
  ListFilter yy83;
  SpecialArg yy120;
  MoreLines * yy129;
  FBoomArg yy130;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define XlatParseARG_SDECL  FParseContext *context ;
#define XlatParseARG_PDECL , FParseContext *context 
#define XlatParseARG_FETCH  FParseContext *context  = yypParser->context 
#define XlatParseARG_STORE yypParser->context  = context 
#define YYNSTATE             100
#define YYNRULE              78
#define YY_MAX_SHIFT         99
#define YY_MIN_SHIFTREDUCE   155
#define YY_MAX_SHIFTREDUCE   232
#define YY_MIN_REDUCE        233
#define YY_MAX_REDUCE        310
#define YY_ERROR_ACTION      311
#define YY_ACCEPT_ACTION     312
#define YY_NO_ACTION         313
/************* End control #defines *******************************************/

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE
**
**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as either:
**
**    (A)   N = yy_action[ yy_shift_ofst[S] + X ]
**    (B)   N = yy_default[S]
**
** The (A) formula is preferred.  The B formula is used instead if:
**    (1)  The yy_shift_ofst[S]+X value is out of range, or
**    (2)  yy_lookahead[yy_shift_ofst[S]+X] is not equal to X, or
**    (3)  yy_shift_ofst[S] equal YY_SHIFT_USE_DFLT.
** (Implementation note: YY_SHIFT_USE_DFLT is chosen so that
** YY_SHIFT_USE_DFLT+X will be out of range for all possible lookaheads X.
** Hence only tests (1) and (2) need to be evaluated.)
**
** The formulas above are for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (358)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   218,  218,  218,  218,  218,  218,  218,  218,  218,   69,
 /*    10 */    10,  312,    1,   95,   67,  187,   42,   40,   41,   46,
 /*    20 */    47,   45,   44,   43,   97,   42,   40,   41,   46,   47,
 /*    30 */    45,   44,   43,   42,   40,   41,   46,   47,   45,   44,
 /*    40 */    43,   46,   47,   45,   44,   43,  210,   48,    4,  191,
 /*    50 */    82,  212,  213,  294,  227,  206,   28,   84,   39,   91,
 /*    60 */    67,  186,   93,  214,  155,   38,  228,   99,  215,   49,
 /*    70 */    96,   48,    4,   88,   57,   34,  193,  194,  195,  196,
 /*    80 */   197,   21,   20,   19,   85,  192,   13,   94,   67,  186,
 /*    90 */    93,   61,   27,   81,   81,   42,   40,   26,   46,   47,
 /*   100 */    45,   44,   43,   42,   40,   41,   46,   47,   45,   44,
 /*   110 */    43,   50,   54,   25,  183,   73,   42,   40,   41,   46,
 /*   120 */    47,   45,   44,   43,   42,   40,   41,   46,   47,   45,
 /*   130 */    44,   43,   62,  209,   12,   75,   42,   40,   41,   46,
 /*   140 */    47,   45,   44,   43,  198,   42,   40,   41,   46,   47,
 /*   150 */    45,   44,   43,  199,  205,   42,   40,   41,   46,   47,
 /*   160 */    45,   44,   43,   70,   31,    7,   89,   42,   40,   41,
 /*   170 */    46,   47,   45,   44,   43,   42,   40,   41,   46,   47,
 /*   180 */    45,   44,   43,  179,   32,  178,  177,   18,   16,   17,
 /*   190 */    22,   23,   21,   20,   19,   77,   90,   78,  184,   42,
 /*   200 */    40,   41,   46,   47,   45,   44,   43,   51,   72,    3,
 /*   210 */    52,   53,   42,   40,   41,   46,   47,   45,   44,   43,
 /*   220 */    42,   40,   41,   46,   47,   45,   44,   43,   11,   24,
 /*   230 */    55,  166,   42,   40,   41,   46,   47,   45,   44,   43,
 /*   240 */    45,   44,   43,  165,   66,   58,   42,   40,   41,   46,
 /*   250 */    47,   45,   44,   43,   18,   16,   17,   22,   23,   21,
 /*   260 */    20,   19,   16,   17,   22,   23,   21,   20,   19,   40,
 /*   270 */    41,   46,   47,   45,   44,   43,   17,   22,   23,   21,
 /*   280 */    20,   19,   41,   46,   47,   45,   44,   43,   22,   23,
 /*   290 */    21,   20,   19,   46,   47,   45,   44,   43,   15,   59,
 /*   300 */    39,   60,   39,   63,  173,   14,  155,   38,  155,   38,
 /*   310 */    83,   39,   68,   39,  174,   56,   64,  155,   38,  155,
 /*   320 */    38,   92,   65,  164,   86,  203,   56,   74,   76,   71,
 /*   330 */   211,  160,   29,  159,  158,   86,   87,   79,   80,  231,
 /*   340 */    30,  207,  208,    6,    9,  201,  189,    5,   33,  167,
 /*   350 */   172,    2,    8,  171,   35,   36,   37,   98,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    43,   44,   45,   46,   47,   48,   49,   50,   51,   52,
 /*    10 */    53,   41,   42,   56,   58,   59,    2,    3,    4,    5,
 /*    20 */     6,    7,    8,    9,   15,    2,    3,    4,    5,    6,
 /*    30 */     7,    8,    9,    2,    3,    4,    5,    6,    7,    8,
 /*    40 */     9,    5,    6,    7,    8,    9,   32,   65,   66,   67,
 /*    50 */    36,   37,   38,    0,    1,   32,   68,   34,    5,   57,
 /*    60 */    58,   59,   60,   32,   11,   12,   16,   14,   32,   19,
 /*    70 */    17,   65,   66,   67,   52,   22,   24,   25,   26,   27,
 /*    80 */    28,    7,    8,    9,   31,   63,   33,   57,   58,   59,
 /*    90 */    60,   58,   39,   54,   55,    2,    3,    4,    5,    6,
 /*   100 */     7,    8,    9,    2,    3,    4,    5,    6,    7,    8,
 /*   110 */     9,   52,   52,   20,   58,   58,    2,    3,    4,    5,
 /*   120 */     6,    7,    8,    9,    2,    3,    4,    5,    6,    7,
 /*   130 */     8,    9,   52,   32,   20,   58,    2,    3,    4,    5,
 /*   140 */     6,    7,    8,    9,   20,    2,    3,    4,    5,    6,
 /*   150 */     7,    8,    9,   29,   32,    2,    3,    4,    5,    6,
 /*   160 */     7,    8,    9,   58,   30,   22,   13,    2,    3,    4,
 /*   170 */     5,    6,    7,    8,    9,    2,    3,    4,    5,    6,
 /*   180 */     7,    8,    9,   58,   19,   58,   58,    2,    3,    4,
 /*   190 */     5,    6,    7,    8,    9,   58,   23,   58,   13,    2,
 /*   200 */     3,    4,    5,    6,    7,    8,    9,   52,   52,   12,
 /*   210 */    52,   52,    2,    3,    4,    5,    6,    7,    8,    9,
 /*   220 */     2,    3,    4,    5,    6,    7,    8,    9,   52,   19,
 /*   230 */    52,   13,    2,    3,    4,    5,    6,    7,    8,    9,
 /*   240 */     7,    8,    9,   13,   52,   52,    2,    3,    4,    5,
 /*   250 */     6,    7,    8,    9,    2,    3,    4,    5,    6,    7,
 /*   260 */     8,    9,    3,    4,    5,    6,    7,    8,    9,    3,
 /*   270 */     4,    5,    6,    7,    8,    9,    4,    5,    6,    7,
 /*   280 */     8,    9,    4,    5,    6,    7,    8,    9,    5,    6,
 /*   290 */     7,    8,    9,    5,    6,    7,    8,    9,    5,   52,
 /*   300 */     5,   52,    5,   52,   11,   12,   11,   12,   11,   12,
 /*   310 */    15,    5,   52,    5,   21,   52,   52,   11,   12,   11,
 /*   320 */    12,   15,   52,   52,   61,   62,   52,   52,   52,   52,
 /*   330 */    32,   52,   35,   52,   52,   61,   62,   52,   52,   55,
 /*   340 */    20,   32,   32,   19,   64,   23,   16,   18,   12,   18,
 /*   350 */    13,   12,   19,   13,   20,   20,   12,   15,
};
#define YY_SHIFT_USE_DFLT (358)
#define YY_SHIFT_COUNT    (99)
#define YY_SHIFT_MIN      (0)
#define YY_SHIFT_MAX      (344)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */   358,   53,  293,  293,   52,   52,  308,  308,  293,  308,
 /*    10 */     9,   14,  295,  297,  293,  293,  293,  293,  293,  293,
 /*    20 */   293,  293,  293,  293,  306,  308,  308,  308,  308,  308,
 /*    30 */   308,  308,  308,  308,  308,  308,  308,  308,  308,  308,
 /*    40 */   308,  308,  308,  308,  308,  308,  308,  308,  124,    9,
 /*    50 */    23,   31,   93,  101,  114,  122,  134,  143,  153,  165,
 /*    60 */   173,  185,  197,  210,  218,  230,  244,  252,  244,  244,
 /*    70 */   259,  266,   36,  272,  278,  283,  288,   74,   74,  233,
 /*    80 */   233,   50,  298,  309,  310,  320,  324,  322,  330,  329,
 /*    90 */   336,  337,  339,  333,  340,  334,  331,  335,  344,  342,
};
#define YY_REDUCE_USE_DFLT (-45)
#define YY_REDUCE_COUNT (49)
#define YY_REDUCE_MIN   (-44)
#define YY_REDUCE_MAX   (286)
static const short yy_reduce_ofst[] = {
 /*     0 */   -30,  -43,    2,   30,  -18,    6,  263,  274,  -44,   22,
 /*    10 */    39,  -12,   59,   60,   33,   56,   57,   77,  105,  125,
 /*    20 */   127,  128,  137,  139,   80,  155,  156,  158,  159,  176,
 /*    30 */   178,  192,  193,  247,  249,  251,  260,  264,  270,  271,
 /*    40 */   275,  276,  277,  279,  281,  282,  285,  286,  280,  284,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   295,  311,  266,  266,  268,  268,  311,  311,  311,  311,
 /*    10 */   307,  311,  311,  311,  311,  311,  311,  311,  311,  311,
 /*    20 */   311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
 /*    30 */   311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
 /*    40 */   311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
 /*    50 */   311,  311,  311,  311,  311,  311,  311,  278,  311,  311,
 /*    60 */   311,  311,  311,  311,  311,  311,  282,  263,  247,  248,
 /*    70 */   258,  239,  240,  260,  241,  259,  240,  254,  253,  235,
 /*    80 */   234,  311,  311,  311,  311,  311,  280,  311,  311,  311,
 /*    90 */   311,  311,  311,  310,  311,  311,  311,  246,  311,  311,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.  
** If a construct like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  yyStackEntry *yytos;          /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm;                    /* High-water mark of the stack */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt;                 /* Shifts left before out of the error */
#endif
  XlatParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
  yyStackEntry yystk0;          /* First stack entry */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void XlatParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "NOP",           "OR",            "XOR",         
  "AND",           "MINUS",         "PLUS",          "MULTIPLY",    
  "DIVIDE",        "MODULUS",       "NEG",           "NUM",         
  "LPAREN",        "RPAREN",        "DEFINE",        "SYM",         
  "RBRACE",        "ENUM",          "LBRACE",        "COMMA",       
  "EQUALS",        "TAG",           "LBRACKET",      "RBRACKET",    
  "FLAGS",         "ARG2",          "ARG3",          "ARG4",        
  "ARG5",          "OR_EQUAL",      "COLON",         "MAXLINESPECIAL",
  "SEMICOLON",     "SECTOR",        "NOBITMASK",     "BITMASK",     
  "CLEAR",         "LSHASSIGN",     "RSHASSIGN",     "LINEFLAG",    
  "error",         "main",          "translation_unit",  "external_declaration",
  "define_statement",  "enum_statement",  "linetype_declaration",  "boom_declaration",
  "sector_declaration",  "lineflag_declaration",  "sector_bitmask",  "maxlinespecial_def",
  "expr",          "enum_open",     "enum_list",     "single_enum", 
  "linetype_exp",  "special_args",  "exp_with_tag",  "special_arg", 
  "multi_special_arg",  "list_val",      "arg_list",      "boom_args",   
  "boom_op",       "boom_selector",  "boom_line",     "boom_body",   
  "sector_op",     "lineflag_op", 
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "expr ::= NUM",
 /*   1 */ "expr ::= expr PLUS expr",
 /*   2 */ "expr ::= expr MINUS expr",
 /*   3 */ "expr ::= expr MULTIPLY expr",
 /*   4 */ "expr ::= expr DIVIDE expr",
 /*   5 */ "expr ::= expr MODULUS expr",
 /*   6 */ "expr ::= expr OR expr",
 /*   7 */ "expr ::= expr AND expr",
 /*   8 */ "expr ::= expr XOR expr",
 /*   9 */ "expr ::= MINUS expr",
 /*  10 */ "expr ::= LPAREN expr RPAREN",
 /*  11 */ "define_statement ::= DEFINE SYM LPAREN expr RPAREN",
 /*  12 */ "enum_open ::= ENUM LBRACE",
 /*  13 */ "single_enum ::= SYM",
 /*  14 */ "single_enum ::= SYM EQUALS expr",
 /*  15 */ "linetype_exp ::= expr",
 /*  16 */ "linetype_declaration ::= linetype_exp EQUALS expr COMMA expr LPAREN special_args RPAREN",
 /*  17 */ "linetype_declaration ::= linetype_exp EQUALS expr COMMA SYM LPAREN special_args RPAREN",
 /*  18 */ "exp_with_tag ::= NUM",
 /*  19 */ "exp_with_tag ::= TAG",
 /*  20 */ "exp_with_tag ::= exp_with_tag PLUS exp_with_tag",
 /*  21 */ "exp_with_tag ::= exp_with_tag MINUS exp_with_tag",
 /*  22 */ "exp_with_tag ::= exp_with_tag MULTIPLY exp_with_tag",
 /*  23 */ "exp_with_tag ::= exp_with_tag DIVIDE exp_with_tag",
 /*  24 */ "exp_with_tag ::= exp_with_tag MODULUS exp_with_tag",
 /*  25 */ "exp_with_tag ::= exp_with_tag OR exp_with_tag",
 /*  26 */ "exp_with_tag ::= exp_with_tag AND exp_with_tag",
 /*  27 */ "exp_with_tag ::= exp_with_tag XOR exp_with_tag",
 /*  28 */ "exp_with_tag ::= MINUS exp_with_tag",
 /*  29 */ "exp_with_tag ::= LPAREN exp_with_tag RPAREN",
 /*  30 */ "special_arg ::= exp_with_tag",
 /*  31 */ "multi_special_arg ::= special_arg",
 /*  32 */ "multi_special_arg ::= multi_special_arg COMMA special_arg",
 /*  33 */ "special_args ::=",
 /*  34 */ "boom_declaration ::= LBRACKET expr RBRACKET LPAREN expr COMMA expr RPAREN LBRACE boom_body RBRACE",
 /*  35 */ "boom_body ::=",
 /*  36 */ "boom_body ::= boom_line boom_body",
 /*  37 */ "boom_line ::= boom_selector boom_op boom_args",
 /*  38 */ "boom_selector ::= FLAGS",
 /*  39 */ "boom_selector ::= ARG2",
 /*  40 */ "boom_selector ::= ARG3",
 /*  41 */ "boom_selector ::= ARG4",
 /*  42 */ "boom_selector ::= ARG5",
 /*  43 */ "boom_op ::= EQUALS",
 /*  44 */ "boom_op ::= OR_EQUAL",
 /*  45 */ "boom_args ::= expr",
 /*  46 */ "boom_args ::= expr LBRACKET arg_list RBRACKET",
 /*  47 */ "arg_list ::= list_val",
 /*  48 */ "arg_list ::= list_val COMMA arg_list",
 /*  49 */ "list_val ::= expr COLON expr",
 /*  50 */ "maxlinespecial_def ::= MAXLINESPECIAL EQUALS expr SEMICOLON",
 /*  51 */ "sector_declaration ::= SECTOR expr EQUALS expr SEMICOLON",
 /*  52 */ "sector_declaration ::= SECTOR expr EQUALS SYM SEMICOLON",
 /*  53 */ "sector_declaration ::= SECTOR expr EQUALS expr NOBITMASK SEMICOLON",
 /*  54 */ "sector_bitmask ::= SECTOR BITMASK expr sector_op expr SEMICOLON",
 /*  55 */ "sector_bitmask ::= SECTOR BITMASK expr SEMICOLON",
 /*  56 */ "sector_bitmask ::= SECTOR BITMASK expr CLEAR SEMICOLON",
 /*  57 */ "sector_op ::= LSHASSIGN",
 /*  58 */ "sector_op ::= RSHASSIGN",
 /*  59 */ "lineflag_declaration ::= LINEFLAG expr EQUALS expr SEMICOLON",
 /*  60 */ "lineflag_declaration ::= LINEFLAG expr AND expr SEMICOLON",
 /*  61 */ "main ::= translation_unit",
 /*  62 */ "translation_unit ::=",
 /*  63 */ "translation_unit ::= translation_unit external_declaration",
 /*  64 */ "external_declaration ::= define_statement",
 /*  65 */ "external_declaration ::= enum_statement",
 /*  66 */ "external_declaration ::= linetype_declaration",
 /*  67 */ "external_declaration ::= boom_declaration",
 /*  68 */ "external_declaration ::= sector_declaration",
 /*  69 */ "external_declaration ::= lineflag_declaration",
 /*  70 */ "external_declaration ::= sector_bitmask",
 /*  71 */ "external_declaration ::= maxlinespecial_def",
 /*  72 */ "external_declaration ::= NOP",
 /*  73 */ "enum_statement ::= enum_open enum_list RBRACE",
 /*  74 */ "enum_list ::=",
 /*  75 */ "enum_list ::= single_enum",
 /*  76 */ "enum_list ::= enum_list COMMA single_enum",
 /*  77 */ "special_args ::= multi_special_arg",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
static int yyGrowStack(yyParser *p){
  int newSize;
  int idx;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  idx = p->yytos ? (int)(p->yytos - p->yystack) : 0;
  if( p->yystack==&p->yystk0 ){
    pNew = (yyStackEntry *)malloc(newSize*sizeof(pNew[0]));
    if( pNew ) pNew[0] = p->yystk0;
  }else{
    pNew = (yyStackEntry *)realloc(p->yystack, newSize*sizeof(pNew[0]));
  }
  if( pNew ){
    p->yystack = pNew;
    p->yytos = &p->yystack[idx];
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows from %d to %d entries.\n",
              yyTracePrompt, p->yystksz, newSize);
      fflush(yyTraceFILE);
    }
#endif
    p->yystksz = newSize;
  }
  return pNew==0; 
}
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to XlatParseAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to XlatParse and XlatParseFree.
*/
void *XlatParseAlloc(void *(CDECL *mallocProc)(YYMALLOCARGTYPE)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(yyParser) );
  if( pParser ){
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyhwm = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yytos = NULL;
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    if( yyGrowStack(pParser) ){
      pParser->yystack = &pParser->yystk0;
      pParser->yystksz = 1;
    }
#endif
#ifndef YYNOERRORRECOVERY
    pParser->yyerrcnt = -1;
#endif
    pParser->yytos = pParser->yystack;
    pParser->yystack[0].stateno = 0;
    pParser->yystack[0].major = 0;
  }
  return pParser;
}

/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  XlatParseARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
      /* TERMINAL Destructor */
    case 1: /* NOP */
    case 2: /* OR */
    case 3: /* XOR */
    case 4: /* AND */
    case 5: /* MINUS */
    case 6: /* PLUS */
    case 7: /* MULTIPLY */
    case 8: /* DIVIDE */
    case 9: /* MODULUS */
    case 10: /* NEG */
    case 11: /* NUM */
    case 12: /* LPAREN */
    case 13: /* RPAREN */
    case 14: /* DEFINE */
    case 15: /* SYM */
    case 16: /* RBRACE */
    case 17: /* ENUM */
    case 18: /* LBRACE */
    case 19: /* COMMA */
    case 20: /* EQUALS */
    case 21: /* TAG */
    case 22: /* LBRACKET */
    case 23: /* RBRACKET */
    case 24: /* FLAGS */
    case 25: /* ARG2 */
    case 26: /* ARG3 */
    case 27: /* ARG4 */
    case 28: /* ARG5 */
    case 29: /* OR_EQUAL */
    case 30: /* COLON */
    case 31: /* MAXLINESPECIAL */
    case 32: /* SEMICOLON */
    case 33: /* SECTOR */
    case 34: /* NOBITMASK */
    case 35: /* BITMASK */
    case 36: /* CLEAR */
    case 37: /* LSHASSIGN */
    case 38: /* RSHASSIGN */
    case 39: /* LINEFLAG */
{
#line 37 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"

#line 674 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
      break;
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(yyParser *pParser){
  yyStackEntry *yytos;
  assert( pParser->yytos!=0 );
  assert( pParser->yytos > pParser->yystack );
  yytos = pParser->yytos--;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
    fflush(yyTraceFILE);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void XlatParseFree(
  void *p,                    /* The parser to be deleted */
  void (CDECL *freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
#ifndef YYPARSEFREENEVERNULL
  if( pParser==0 ) return;
#endif
  while( pParser->yytos>pParser->yystack ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  if( pParser->yystack!=&pParser->yystk0 ) free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int XlatParseStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyhwm;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static unsigned int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yytos->stateno;
 
  if( stateno>=YY_MIN_REDUCE ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
  do{
    i = yy_shift_ofst[stateno];
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
          fflush(yyTraceFILE);
        }
#endif
        assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
        iLookAhead = iFallback;
        continue;
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
          j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
          j<YY_ACTTAB_COUNT &&
#endif
          yy_lookahead[j]==YYWILDCARD && iLookAhead>0
        ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead],
               yyTokenName[YYWILDCARD]);
            fflush(yyTraceFILE);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
      return yy_default[stateno];
    }else{
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser){
   XlatParseARG_FETCH;
   yypParser->yytos--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
     fflush(yyTraceFILE);
   }
#endif
   while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
/******** End %stack_overflow code ********************************************/
   XlatParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser *yypParser, int yyNewState){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%sShift '%s', go to state %d\n",
         yyTracePrompt,yyTokenName[yypParser->yytos->major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%sShift '%s'\n",
         yyTracePrompt,yyTokenName[yypParser->yytos->major]);
    }
    fflush(yyTraceFILE);
  }
}
#else
# define yyTraceShift(X,Y)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  XlatParseTOKENTYPE yyMinor        /* The minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yytos++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
    yypParser->yyhwm++;
    assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack) );
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yytos>=&yypParser->yystack[YYSTACKDEPTH] ){
    yyStackOverflow(yypParser);
    return;
  }
#else
  if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz] ){
    if( yyGrowStack(yypParser) ){
      yyStackOverflow(yypParser);
      return;
    }
  }
#endif
  if( yyNewState > YY_MAX_SHIFT ){
    yyNewState += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
  }
  yytos = yypParser->yytos;
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor.yy0 = yyMinor;
  yyTraceShift(yypParser, yyNewState);
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 52, 1 },
  { 52, 3 },
  { 52, 3 },
  { 52, 3 },
  { 52, 3 },
  { 52, 3 },
  { 52, 3 },
  { 52, 3 },
  { 52, 3 },
  { 52, 2 },
  { 52, 3 },
  { 44, 5 },
  { 53, 2 },
  { 55, 1 },
  { 55, 3 },
  { 56, 1 },
  { 46, 8 },
  { 46, 8 },
  { 58, 1 },
  { 58, 1 },
  { 58, 3 },
  { 58, 3 },
  { 58, 3 },
  { 58, 3 },
  { 58, 3 },
  { 58, 3 },
  { 58, 3 },
  { 58, 3 },
  { 58, 2 },
  { 58, 3 },
  { 59, 1 },
  { 60, 1 },
  { 60, 3 },
  { 57, 0 },
  { 47, 11 },
  { 67, 0 },
  { 67, 2 },
  { 66, 3 },
  { 65, 1 },
  { 65, 1 },
  { 65, 1 },
  { 65, 1 },
  { 65, 1 },
  { 64, 1 },
  { 64, 1 },
  { 63, 1 },
  { 63, 4 },
  { 62, 1 },
  { 62, 3 },
  { 61, 3 },
  { 51, 4 },
  { 48, 5 },
  { 48, 5 },
  { 48, 6 },
  { 50, 6 },
  { 50, 4 },
  { 50, 5 },
  { 68, 1 },
  { 68, 1 },
  { 49, 5 },
  { 49, 5 },
  { 41, 1 },
  { 42, 0 },
  { 42, 2 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 45, 3 },
  { 54, 0 },
  { 54, 1 },
  { 54, 3 },
  { 57, 1 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  unsigned int yyruleno        /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  XlatParseARG_FETCH;
  yymsp = yypParser->yytos;
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfo[yyruleno].nrhs;
    fprintf(yyTraceFILE, "%sReduce [%s], go to state %d.\n", yyTracePrompt,
      yyRuleName[yyruleno], yymsp[-yysize].stateno);
    fflush(yyTraceFILE);
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfo[yyruleno].nrhs==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
      yypParser->yyhwm++;
      assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack));
    }
#endif
#if YYSTACKDEPTH>0 
    if( yypParser->yytos>=&yypParser->yystack[YYSTACKDEPTH-1] ){
      yyStackOverflow(yypParser);
      return;
    }
#else
    if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz-1] ){
      if( yyGrowStack(yypParser) ){
        yyStackOverflow(yypParser);
        return;
      }
      yymsp = yypParser->yytos;
    }
#endif
  }

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
        YYMINORTYPE yylhsminor;
      case 0: /* expr ::= NUM */
#line 67 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yylhsminor.yy32 = yymsp[0].minor.yy0.val; }
#line 1066 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yymsp[0].minor.yy32 = yylhsminor.yy32;
        break;
      case 1: /* expr ::= expr PLUS expr */
#line 68 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yylhsminor.yy32 = yymsp[-2].minor.yy32 + yymsp[0].minor.yy32; }
#line 1072 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,6,&yymsp[-1].minor);
  yymsp[-2].minor.yy32 = yylhsminor.yy32;
        break;
      case 2: /* expr ::= expr MINUS expr */
#line 69 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yylhsminor.yy32 = yymsp[-2].minor.yy32 - yymsp[0].minor.yy32; }
#line 1079 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,5,&yymsp[-1].minor);
  yymsp[-2].minor.yy32 = yylhsminor.yy32;
        break;
      case 3: /* expr ::= expr MULTIPLY expr */
#line 70 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yylhsminor.yy32 = yymsp[-2].minor.yy32 * yymsp[0].minor.yy32; }
#line 1086 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,7,&yymsp[-1].minor);
  yymsp[-2].minor.yy32 = yylhsminor.yy32;
        break;
      case 4: /* expr ::= expr DIVIDE expr */
#line 71 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ if (yymsp[0].minor.yy32 != 0) yylhsminor.yy32 = yymsp[-2].minor.yy32 / yymsp[0].minor.yy32; else context->PrintError("Division by zero"); }
#line 1093 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,8,&yymsp[-1].minor);
  yymsp[-2].minor.yy32 = yylhsminor.yy32;
        break;
      case 5: /* expr ::= expr MODULUS expr */
#line 72 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ if (yymsp[0].minor.yy32 != 0) yylhsminor.yy32 = yymsp[-2].minor.yy32 % yymsp[0].minor.yy32; else context->PrintError("Division by zero"); }
#line 1100 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,9,&yymsp[-1].minor);
  yymsp[-2].minor.yy32 = yylhsminor.yy32;
        break;
      case 6: /* expr ::= expr OR expr */
#line 73 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yylhsminor.yy32 = yymsp[-2].minor.yy32 | yymsp[0].minor.yy32; }
#line 1107 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,2,&yymsp[-1].minor);
  yymsp[-2].minor.yy32 = yylhsminor.yy32;
        break;
      case 7: /* expr ::= expr AND expr */
#line 74 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yylhsminor.yy32 = yymsp[-2].minor.yy32 & yymsp[0].minor.yy32; }
#line 1114 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,4,&yymsp[-1].minor);
  yymsp[-2].minor.yy32 = yylhsminor.yy32;
        break;
      case 8: /* expr ::= expr XOR expr */
#line 75 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yylhsminor.yy32 = yymsp[-2].minor.yy32 ^ yymsp[0].minor.yy32; }
#line 1121 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,3,&yymsp[-1].minor);
  yymsp[-2].minor.yy32 = yylhsminor.yy32;
        break;
      case 9: /* expr ::= MINUS expr */
{  yy_destructor(yypParser,5,&yymsp[-1].minor);
#line 76 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[-1].minor.yy32 = -yymsp[0].minor.yy32; }
#line 1129 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 10: /* expr ::= LPAREN expr RPAREN */
      case 29: /*exp_with_tag ::= LPAREN exp_with_tag RPAREN */ yytestcase(yyruleno==29);
{  yy_destructor(yypParser,12,&yymsp[-2].minor);
#line 77 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[-2].minor.yy32 = yymsp[-1].minor.yy32; }
#line 1137 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,13,&yymsp[0].minor);
}
        break;
      case 11: /* define_statement ::= DEFINE SYM LPAREN expr RPAREN */
{  yy_destructor(yypParser,14,&yymsp[-4].minor);
#line 87 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	context->AddSym (yymsp[-3].minor.yy0.sym, yymsp[-1].minor.yy32);
}
#line 1147 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,12,&yymsp[-2].minor);
  yy_destructor(yypParser,13,&yymsp[0].minor);
}
        break;
      case 12: /* enum_open ::= ENUM LBRACE */
{  yy_destructor(yypParser,17,&yymsp[-1].minor);
#line 100 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	context->EnumVal = 0;
}
#line 1158 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,18,&yymsp[0].minor);
}
        break;
      case 13: /* single_enum ::= SYM */
#line 109 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	context->AddSym (yymsp[0].minor.yy0.sym, context->EnumVal++);
}
#line 1167 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
        break;
      case 14: /* single_enum ::= SYM EQUALS expr */
#line 114 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	context->AddSym (yymsp[-2].minor.yy0.sym, yymsp[0].minor.yy32);
	context->EnumVal = yymsp[0].minor.yy32+1;
}
#line 1175 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,20,&yymsp[-1].minor);
        break;
      case 15: /* linetype_exp ::= expr */
#line 127 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yylhsminor.yy32 = static_cast<XlatParseContext *>(context)->DefiningLineType = yymsp[0].minor.yy32;
}
#line 1183 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yymsp[0].minor.yy32 = yylhsminor.yy32;
        break;
      case 16: /* linetype_declaration ::= linetype_exp EQUALS expr COMMA expr LPAREN special_args RPAREN */
#line 132 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	static_cast<XlatParseContext *>(context)->Translator->SimpleLineTranslations.SetVal(yymsp[-7].minor.yy32, 
		FLineTrans(yymsp[-3].minor.yy32&0xffff, yymsp[-5].minor.yy32+yymsp[-1].minor.yy7.addflags, yymsp[-1].minor.yy7.args[0], yymsp[-1].minor.yy7.args[1], yymsp[-1].minor.yy7.args[2], yymsp[-1].minor.yy7.args[3], yymsp[-1].minor.yy7.args[4]));
	static_cast<XlatParseContext *>(context)->DefiningLineType = -1;
}
#line 1193 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,20,&yymsp[-6].minor);
  yy_destructor(yypParser,19,&yymsp[-4].minor);
  yy_destructor(yypParser,12,&yymsp[-2].minor);
  yy_destructor(yypParser,13,&yymsp[0].minor);
        break;
      case 17: /* linetype_declaration ::= linetype_exp EQUALS expr COMMA SYM LPAREN special_args RPAREN */
#line 139 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	Printf ("%s, line %d: %s is undefined\n", context->SourceFile, context->SourceLine, yymsp[-3].minor.yy0.sym);
	static_cast<XlatParseContext *>(context)->DefiningLineType = -1;
}
#line 1205 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,20,&yymsp[-6].minor);
  yy_destructor(yypParser,19,&yymsp[-4].minor);
  yy_destructor(yypParser,12,&yymsp[-2].minor);
  yy_destructor(yypParser,13,&yymsp[0].minor);
        break;
      case 18: /* exp_with_tag ::= NUM */
#line 145 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(yymsp[0].minor.yy0.val); yylhsminor.yy32 = static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(XEXP_Const); }
#line 1214 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yymsp[0].minor.yy32 = yylhsminor.yy32;
        break;
      case 19: /* exp_with_tag ::= TAG */
{  yy_destructor(yypParser,21,&yymsp[0].minor);
#line 146 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[0].minor.yy32 = static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(XEXP_Tag); }
#line 1221 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 20: /* exp_with_tag ::= exp_with_tag PLUS exp_with_tag */
#line 147 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[-2].minor.yy32 = static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(XEXP_Add); }
#line 1227 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,6,&yymsp[-1].minor);
        break;
      case 21: /* exp_with_tag ::= exp_with_tag MINUS exp_with_tag */
#line 148 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[-2].minor.yy32 = static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(XEXP_Sub); }
#line 1233 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,5,&yymsp[-1].minor);
        break;
      case 22: /* exp_with_tag ::= exp_with_tag MULTIPLY exp_with_tag */
#line 149 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[-2].minor.yy32 = static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(XEXP_Mul); }
#line 1239 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,7,&yymsp[-1].minor);
        break;
      case 23: /* exp_with_tag ::= exp_with_tag DIVIDE exp_with_tag */
#line 150 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[-2].minor.yy32 = static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(XEXP_Div); }
#line 1245 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,8,&yymsp[-1].minor);
        break;
      case 24: /* exp_with_tag ::= exp_with_tag MODULUS exp_with_tag */
#line 151 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[-2].minor.yy32 = static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(XEXP_Mod); }
#line 1251 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,9,&yymsp[-1].minor);
        break;
      case 25: /* exp_with_tag ::= exp_with_tag OR exp_with_tag */
#line 152 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[-2].minor.yy32 = static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(XEXP_Or);  }
#line 1257 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,2,&yymsp[-1].minor);
        break;
      case 26: /* exp_with_tag ::= exp_with_tag AND exp_with_tag */
#line 153 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[-2].minor.yy32 = static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(XEXP_And); }
#line 1263 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,4,&yymsp[-1].minor);
        break;
      case 27: /* exp_with_tag ::= exp_with_tag XOR exp_with_tag */
#line 154 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[-2].minor.yy32 = static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(XEXP_Xor); }
#line 1269 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,3,&yymsp[-1].minor);
        break;
      case 28: /* exp_with_tag ::= MINUS exp_with_tag */
{  yy_destructor(yypParser,5,&yymsp[-1].minor);
#line 155 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[-1].minor.yy32 = static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Push(XEXP_Neg); }
#line 1276 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 30: /* special_arg ::= exp_with_tag */
#line 162 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	if (static_cast<XlatParseContext *>(context)->Translator->XlatExpressions[yymsp[0].minor.yy32] == XEXP_Tag)
	{ // Store tags directly
		yylhsminor.yy120.arg = 0;
		yylhsminor.yy120.argop = ARGOP_Tag;
		static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Delete(yymsp[0].minor.yy32);
	}
	else
	{ // Try and evaluate it. If it's a constant, store it and erase the
	  // expression. Otherwise, store the index to the expression. We make
	  // no attempt to simplify non-constant expressions.
		FXlatExprState state;
		int val;
		const int *endpt;
		int *xnode;
		
		state.linetype = static_cast<XlatParseContext *>(context)->DefiningLineType;
		state.tag = 0;
		state.bIsConstant = true;
		xnode = &static_cast<XlatParseContext *>(context)->Translator->XlatExpressions[yymsp[0].minor.yy32];
		endpt = XlatExprEval[*xnode](&val, xnode, &state);
		if (state.bIsConstant)
		{
			yylhsminor.yy120.arg = val;
			yylhsminor.yy120.argop = ARGOP_Const;
			endpt++;
			assert(endpt >= &static_cast<XlatParseContext *>(context)->Translator->XlatExpressions[0]);
			static_cast<XlatParseContext *>(context)->Translator->XlatExpressions.Resize((unsigned)(endpt - &static_cast<XlatParseContext *>(context)->Translator->XlatExpressions[0]));
		}
		else
		{
			yylhsminor.yy120.arg = yymsp[0].minor.yy32;
			yylhsminor.yy120.argop = ARGOP_Expr;
		}
	}
}
#line 1317 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yymsp[0].minor.yy120 = yylhsminor.yy120;
        break;
      case 31: /* multi_special_arg ::= special_arg */
#line 202 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yylhsminor.yy7.addflags = yymsp[0].minor.yy120.argop << LINETRANS_TAGSHIFT;
	yylhsminor.yy7.argcount = 1;
	yylhsminor.yy7.args[0] = yymsp[0].minor.yy120.arg;
	yylhsminor.yy7.args[1] = 0;
	yylhsminor.yy7.args[2] = 0;
	yylhsminor.yy7.args[3] = 0;
	yylhsminor.yy7.args[4] = 0;
}
#line 1331 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yymsp[0].minor.yy7 = yylhsminor.yy7;
        break;
      case 32: /* multi_special_arg ::= multi_special_arg COMMA special_arg */
#line 212 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yylhsminor.yy7 = yymsp[-2].minor.yy7;
	if (yylhsminor.yy7.argcount < LINETRANS_MAXARGS)
	{
		yylhsminor.yy7.addflags |= yymsp[0].minor.yy120.argop << (LINETRANS_TAGSHIFT + yylhsminor.yy7.argcount * TAGOP_NUMBITS);
		yylhsminor.yy7.args[yylhsminor.yy7.argcount] = yymsp[0].minor.yy120.arg;
		yylhsminor.yy7.argcount++;
	}
	else if (yylhsminor.yy7.argcount++ == LINETRANS_MAXARGS)
	{
		context->PrintError("Line special has too many arguments\n");
	}
}
#line 1349 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,19,&yymsp[-1].minor);
  yymsp[-2].minor.yy7 = yylhsminor.yy7;
        break;
      case 33: /* special_args ::= */
#line 229 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yymsp[1].minor.yy7.addflags = 0;
	yymsp[1].minor.yy7.argcount = 0;
	yymsp[1].minor.yy7.args[0] = 0;
	yymsp[1].minor.yy7.args[1] = 0;
	yymsp[1].minor.yy7.args[2] = 0;
	yymsp[1].minor.yy7.args[3] = 0;
	yymsp[1].minor.yy7.args[4] = 0;
}
#line 1364 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
        break;
      case 34: /* boom_declaration ::= LBRACKET expr RBRACKET LPAREN expr COMMA expr RPAREN LBRACE boom_body RBRACE */
{  yy_destructor(yypParser,22,&yymsp[-10].minor);
#line 256 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	int i;
	MoreLines *probe;

	if (static_cast<XlatParseContext *>(context)->Translator->NumBoomish == MAX_BOOMISH)
	{
		MoreLines *probe = yymsp[-1].minor.yy129;

		while (probe != NULL)
		{
			MoreLines *next = probe->next;
			delete probe;
			probe = next;
		}
		Printf ("%s, line %d: Too many BOOM translators\n", context->SourceFile, context->SourceLine);
	}
	else
	{
		static_cast<XlatParseContext *>(context)->Translator->Boomish[static_cast<XlatParseContext *>(context)->Translator->NumBoomish].FirstLinetype = yymsp[-6].minor.yy32;
		static_cast<XlatParseContext *>(context)->Translator->Boomish[static_cast<XlatParseContext *>(context)->Translator->NumBoomish].LastLinetype = yymsp[-4].minor.yy32;
		static_cast<XlatParseContext *>(context)->Translator->Boomish[static_cast<XlatParseContext *>(context)->Translator->NumBoomish].NewSpecial = yymsp[-9].minor.yy32;
		
		for (i = 0, probe = yymsp[-1].minor.yy129; probe != NULL; i++)
		{
			MoreLines *next = probe->next;
			static_cast<XlatParseContext *>(context)->Translator->Boomish[static_cast<XlatParseContext *>(context)->Translator->NumBoomish].Args.Push(probe->arg);
			delete probe;
			probe = next;
		}
		static_cast<XlatParseContext *>(context)->Translator->NumBoomish++;
	}
}
#line 1401 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,23,&yymsp[-8].minor);
  yy_destructor(yypParser,12,&yymsp[-7].minor);
  yy_destructor(yypParser,19,&yymsp[-5].minor);
  yy_destructor(yypParser,13,&yymsp[-3].minor);
  yy_destructor(yypParser,18,&yymsp[-2].minor);
  yy_destructor(yypParser,16,&yymsp[0].minor);
}
        break;
      case 35: /* boom_body ::= */
#line 290 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yymsp[1].minor.yy129 = NULL;
}
#line 1415 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
        break;
      case 36: /* boom_body ::= boom_line boom_body */
#line 294 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yylhsminor.yy129 = new MoreLines;
	yylhsminor.yy129->next = yymsp[0].minor.yy129;
	yylhsminor.yy129->arg = yymsp[-1].minor.yy130;
}
#line 1424 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yymsp[-1].minor.yy129 = yylhsminor.yy129;
        break;
      case 37: /* boom_line ::= boom_selector boom_op boom_args */
#line 301 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yylhsminor.yy130.bOrExisting = (yymsp[-1].minor.yy32 == OR_EQUAL);
	yylhsminor.yy130.bUseConstant = (yymsp[0].minor.yy63.filters == NULL);
	yylhsminor.yy130.ListSize = 0;
	yylhsminor.yy130.ArgNum = yymsp[-2].minor.yy32;
	yylhsminor.yy130.ConstantValue = yymsp[0].minor.yy63.constant;
	yylhsminor.yy130.AndValue = yymsp[0].minor.yy63.mask;

	if (yymsp[0].minor.yy63.filters != NULL)
	{
		int i;
		MoreFilters *probe;

		for (i = 0, probe = yymsp[0].minor.yy63.filters; probe != NULL; i++)
		{
			MoreFilters *next = probe->next;
			if (i < 15)
			{
				yylhsminor.yy130.ResultFilter[i] = probe->filter.filter;
				yylhsminor.yy130.ResultValue[i] = probe->filter.value;
			}
			else if (i == 15)
			{
				context->PrintError ("Lists can only have 15 elements");
			}
			delete probe;
			probe = next;
		}
		yylhsminor.yy130.ListSize = i > 15 ? 15 : i;
	}
}
#line 1460 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yymsp[-2].minor.yy130 = yylhsminor.yy130;
        break;
      case 38: /* boom_selector ::= FLAGS */
{  yy_destructor(yypParser,24,&yymsp[0].minor);
#line 333 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[0].minor.yy32 = 4; }
#line 1467 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 39: /* boom_selector ::= ARG2 */
{  yy_destructor(yypParser,25,&yymsp[0].minor);
#line 334 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[0].minor.yy32 = 0; }
#line 1474 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 40: /* boom_selector ::= ARG3 */
{  yy_destructor(yypParser,26,&yymsp[0].minor);
#line 335 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[0].minor.yy32 = 1; }
#line 1481 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 41: /* boom_selector ::= ARG4 */
{  yy_destructor(yypParser,27,&yymsp[0].minor);
#line 336 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[0].minor.yy32 = 2; }
#line 1488 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 42: /* boom_selector ::= ARG5 */
{  yy_destructor(yypParser,28,&yymsp[0].minor);
#line 337 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[0].minor.yy32 = 3; }
#line 1495 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 43: /* boom_op ::= EQUALS */
{  yy_destructor(yypParser,20,&yymsp[0].minor);
#line 339 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[0].minor.yy32 = '='; }
#line 1502 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 44: /* boom_op ::= OR_EQUAL */
{  yy_destructor(yypParser,29,&yymsp[0].minor);
#line 340 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[0].minor.yy32 = OR_EQUAL; }
#line 1509 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 45: /* boom_args ::= expr */
#line 343 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yylhsminor.yy63.constant = yymsp[0].minor.yy32;
	yylhsminor.yy63.filters = NULL;
}
#line 1518 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yymsp[0].minor.yy63 = yylhsminor.yy63;
        break;
      case 46: /* boom_args ::= expr LBRACKET arg_list RBRACKET */
#line 348 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yylhsminor.yy63.mask = yymsp[-3].minor.yy32;
	yylhsminor.yy63.filters = yymsp[-1].minor.yy8;
}
#line 1527 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,22,&yymsp[-2].minor);
  yy_destructor(yypParser,23,&yymsp[0].minor);
  yymsp[-3].minor.yy63 = yylhsminor.yy63;
        break;
      case 47: /* arg_list ::= list_val */
#line 354 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yylhsminor.yy8 = new MoreFilters;
	yylhsminor.yy8->next = NULL;
	yylhsminor.yy8->filter = yymsp[0].minor.yy83;
}
#line 1539 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yymsp[0].minor.yy8 = yylhsminor.yy8;
        break;
      case 48: /* arg_list ::= list_val COMMA arg_list */
#line 360 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yylhsminor.yy8 = new MoreFilters;
	yylhsminor.yy8->next = yymsp[0].minor.yy8;
	yylhsminor.yy8->filter = yymsp[-2].minor.yy83;
}
#line 1549 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,19,&yymsp[-1].minor);
  yymsp[-2].minor.yy8 = yylhsminor.yy8;
        break;
      case 49: /* list_val ::= expr COLON expr */
#line 367 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	yylhsminor.yy83.filter = yymsp[-2].minor.yy32;
	yylhsminor.yy83.value = yymsp[0].minor.yy32;
}
#line 1559 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,30,&yymsp[-1].minor);
  yymsp[-2].minor.yy83 = yylhsminor.yy83;
        break;
      case 50: /* maxlinespecial_def ::= MAXLINESPECIAL EQUALS expr SEMICOLON */
{  yy_destructor(yypParser,31,&yymsp[-3].minor);
#line 379 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	// Just kill all specials higher than the max.
	// If the translator wants to redefine some later, just let it.
	static_cast<XlatParseContext *>(context)->Translator->SimpleLineTranslations.Resize(yymsp[-1].minor.yy32+1);
}
#line 1571 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,20,&yymsp[-2].minor);
  yy_destructor(yypParser,32,&yymsp[0].minor);
}
        break;
      case 51: /* sector_declaration ::= SECTOR expr EQUALS expr SEMICOLON */
{  yy_destructor(yypParser,33,&yymsp[-4].minor);
#line 394 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	FSectorTrans tr(yymsp[-1].minor.yy32, true);
	static_cast<XlatParseContext *>(context)->Translator->SectorTranslations.SetVal(yymsp[-3].minor.yy32, tr);
}
#line 1583 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,20,&yymsp[-2].minor);
  yy_destructor(yypParser,32,&yymsp[0].minor);
}
        break;
      case 52: /* sector_declaration ::= SECTOR expr EQUALS SYM SEMICOLON */
{  yy_destructor(yypParser,33,&yymsp[-4].minor);
#line 400 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	Printf("Unknown constant '%s'\n", yymsp[-1].minor.yy0.sym);
}
#line 1594 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,20,&yymsp[-2].minor);
  yy_destructor(yypParser,32,&yymsp[0].minor);
}
        break;
      case 53: /* sector_declaration ::= SECTOR expr EQUALS expr NOBITMASK SEMICOLON */
{  yy_destructor(yypParser,33,&yymsp[-5].minor);
#line 405 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	FSectorTrans tr(yymsp[-2].minor.yy32, false);
	static_cast<XlatParseContext *>(context)->Translator->SectorTranslations.SetVal(yymsp[-4].minor.yy32, tr);
}
#line 1606 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,20,&yymsp[-3].minor);
  yy_destructor(yypParser,34,&yymsp[-1].minor);
  yy_destructor(yypParser,32,&yymsp[0].minor);
}
        break;
      case 54: /* sector_bitmask ::= SECTOR BITMASK expr sector_op expr SEMICOLON */
{  yy_destructor(yypParser,33,&yymsp[-5].minor);
#line 411 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	FSectorMask sm = { yymsp[-3].minor.yy32, yymsp[-2].minor.yy32, yymsp[-1].minor.yy32};
	static_cast<XlatParseContext *>(context)->Translator->SectorMasks.Push(sm);
}
#line 1619 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,35,&yymsp[-4].minor);
  yy_destructor(yypParser,32,&yymsp[0].minor);
}
        break;
      case 55: /* sector_bitmask ::= SECTOR BITMASK expr SEMICOLON */
{  yy_destructor(yypParser,33,&yymsp[-3].minor);
#line 417 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	FSectorMask sm = { yymsp[-1].minor.yy32, 0, 0};
	static_cast<XlatParseContext *>(context)->Translator->SectorMasks.Push(sm);
}
#line 1631 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,35,&yymsp[-2].minor);
  yy_destructor(yypParser,32,&yymsp[0].minor);
}
        break;
      case 56: /* sector_bitmask ::= SECTOR BITMASK expr CLEAR SEMICOLON */
{  yy_destructor(yypParser,33,&yymsp[-4].minor);
#line 423 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	FSectorMask sm = { yymsp[-2].minor.yy32, 0, 1};
	static_cast<XlatParseContext *>(context)->Translator->SectorMasks.Push(sm);
}
#line 1643 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,35,&yymsp[-3].minor);
  yy_destructor(yypParser,36,&yymsp[-1].minor);
  yy_destructor(yypParser,32,&yymsp[0].minor);
}
        break;
      case 57: /* sector_op ::= LSHASSIGN */
{  yy_destructor(yypParser,37,&yymsp[0].minor);
#line 428 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[0].minor.yy32 = 1; }
#line 1653 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 58: /* sector_op ::= RSHASSIGN */
{  yy_destructor(yypParser,38,&yymsp[0].minor);
#line 429 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{ yymsp[0].minor.yy32 = -1; }
#line 1660 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 59: /* lineflag_declaration ::= LINEFLAG expr EQUALS expr SEMICOLON */
{  yy_destructor(yypParser,39,&yymsp[-4].minor);
#line 434 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	if (yymsp[-3].minor.yy32 >= 0 && yymsp[-3].minor.yy32 < 16)
	{
		static_cast<XlatParseContext *>(context)->Translator->LineFlagTranslations[yymsp[-3].minor.yy32].newvalue = yymsp[-1].minor.yy32;
		static_cast<XlatParseContext *>(context)->Translator->LineFlagTranslations[yymsp[-3].minor.yy32].ismask = false;
	}
}
#line 1673 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,20,&yymsp[-2].minor);
  yy_destructor(yypParser,32,&yymsp[0].minor);
}
        break;
      case 60: /* lineflag_declaration ::= LINEFLAG expr AND expr SEMICOLON */
{  yy_destructor(yypParser,39,&yymsp[-4].minor);
#line 443 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
	if (yymsp[-3].minor.yy32 >= 0 && yymsp[-3].minor.yy32 < 16)
	{
		static_cast<XlatParseContext *>(context)->Translator->LineFlagTranslations[yymsp[-3].minor.yy32].newvalue = yymsp[-1].minor.yy32;
		static_cast<XlatParseContext *>(context)->Translator->LineFlagTranslations[yymsp[-3].minor.yy32].ismask = true;
	}
}
#line 1688 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,4,&yymsp[-2].minor);
  yy_destructor(yypParser,32,&yymsp[0].minor);
}
        break;
      case 72: /* external_declaration ::= NOP */
{  yy_destructor(yypParser,1,&yymsp[0].minor);
#line 56 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
}
#line 1698 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
}
        break;
      case 73: /* enum_statement ::= enum_open enum_list RBRACE */
#line 97 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
}
#line 1705 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,16,&yymsp[0].minor);
        break;
      case 76: /* enum_list ::= enum_list COMMA single_enum */
#line 106 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
{
}
#line 1712 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
  yy_destructor(yypParser,19,&yymsp[-1].minor);
        break;
      default:
      /* (61) main ::= translation_unit */ yytestcase(yyruleno==61);
      /* (62) translation_unit ::= */ yytestcase(yyruleno==62);
      /* (63) translation_unit ::= translation_unit external_declaration */ yytestcase(yyruleno==63);
      /* (64) external_declaration ::= define_statement */ yytestcase(yyruleno==64);
      /* (65) external_declaration ::= enum_statement */ yytestcase(yyruleno==65);
      /* (66) external_declaration ::= linetype_declaration */ yytestcase(yyruleno==66);
      /* (67) external_declaration ::= boom_declaration */ yytestcase(yyruleno==67);
      /* (68) external_declaration ::= sector_declaration */ yytestcase(yyruleno==68);
      /* (69) external_declaration ::= lineflag_declaration */ yytestcase(yyruleno==69);
      /* (70) external_declaration ::= sector_bitmask */ yytestcase(yyruleno==70);
      /* (71) external_declaration ::= maxlinespecial_def */ yytestcase(yyruleno==71);
      /* (74) enum_list ::= */ yytestcase(yyruleno==74);
      /* (75) enum_list ::= single_enum */ yytestcase(yyruleno==75);
      /* (77) special_args ::= multi_special_arg */ yytestcase(yyruleno==77);
       break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0]) );
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact <= YY_MAX_SHIFTREDUCE ){
    if( yyact>YY_MAX_SHIFT ){
      yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
    }
    yymsp -= yysize-1;
    yypParser->yytos = yymsp;
    yymsp->stateno = (YYACTIONTYPE)yyact;
    yymsp->major = (YYCODETYPE)yygoto;
    yyTraceShift(yypParser, yyact);
  }else{
    assert( yyact == YY_ACCEPT_ACTION );
    yypParser->yytos -= yysize;
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  XlatParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
    fflush(yyTraceFILE);
  }
#endif
  while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  XlatParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  XlatParseTOKENTYPE yyminor         /* The minor type of the error token */
){
  XlatParseARG_FETCH;
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/
#line 40 "F:/qobj/droid/DIII4A/Q3E/src/main/jni/gzdoom/src/gamedata/xlat/xlat_parser.y"
 context->PrintError("syntax error");
#line 1789 "F:/qobj/droid/DIII4A/Q3E/.cxx/RelWithDebInfo/4p2dq705/arm64-v8a/gzdoom/src/xlat_parser.c"
/************ End %syntax_error code ******************************************/
  XlatParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  XlatParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
    fflush(yyTraceFILE);
  }
#endif
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
#if 0
  assert( yypParser->yytos==yypParser->yystack );
#else
  while (yypParser->yytos>yypParser->yystack) yy_pop_parser_stack(yypParser);
#endif
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
  XlatParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "XlatParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void XlatParse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  XlatParseTOKENTYPE yyminor       /* The value for the token */
  XlatParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  unsigned int yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  yypParser = (yyParser*)yyp;
  assert( yypParser->yytos!=0 );
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif
  XlatParseARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput '%s'\n",yyTracePrompt,yyTokenName[yymajor]);
    fflush(yyTraceFILE);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact <= YY_MAX_SHIFTREDUCE ){
      yy_shift(yypParser,yyact,yymajor,yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      yymajor = YYNOCODE;
    }else if( yyact <= YY_MAX_REDUCE ){
      yy_reduce(yypParser,yyact-YY_MIN_REDUCE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
      yyminorunion.yy0 = yyminor;
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
        fflush(yyTraceFILE);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminor);
      }
      yymx = yypParser->yytos->major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
          fflush(yyTraceFILE);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
        yymajor = YYNOCODE;
      }else{
        while( yypParser->yytos >= yypParser->yystack
            && yymx != YYERRORSYMBOL
            && (yyact = yy_find_reduce_action(
                        yypParser->yytos->stateno,
                        YYERRORSYMBOL)) >= YY_MIN_REDUCE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yytos < yypParser->yystack || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
          yypParser->yyerrcnt = -1;
#endif
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yypParser,yyact,YYERRORSYMBOL,yyminor);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor, yyminor);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor, yyminor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
        yypParser->yyerrcnt = -1;
#endif
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yytos>yypParser->yystack );
#ifndef NDEBUG
  if( yyTraceFILE ){
    yyStackEntry *i;
    char cDiv = '[';
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=&yypParser->yystack[1]; i<=yypParser->yytos; i++){
      fprintf(yyTraceFILE,"%c%s", cDiv, yyTokenName[i->major]);
      cDiv = ' ';
    }
    fprintf(yyTraceFILE,"]\n");
    fflush(yyTraceFILE);
  }
#endif
  return;
}
